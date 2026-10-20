#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
repair-cp1251.py - restore the CP1251 comments that were damaged by a UTF-8 edit.

The sources of the project store the Cyrillic comments in CP1251 (not valid UTF-8).
A tool that decodes the file as UTF-8 and writes it back replaces every invalid byte
with U+FFFD. This script restores the original bytes of every damaged line using a
pristine copy from git as the source.

The reference revisions are tried in order: HEAD first, then older commits (the
commit 32a9e4ed "refactor: defensive _snprintf/strcpy/memcpy fixes" introduced the
damage in the repository, so its parent still has the intact CP1251 comments).

Usage:
    python build-win/repair-cp1251.py            # repair
    python build-win/repair-cp1251.py --check    # only report
"""
import io
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FFFD = b'\xef\xbf\xbd'
NONASCII = re.compile(rb'[\x80-\xff]+')
REVISIONS = ['HEAD', '32a9e4ed^', '3f329713', '58ebbbf8']


def skeleton(line: bytes) -> bytes:
    """the line with every run of non-ascii bytes replaced by a single marker"""
    return NONASCII.sub(b'\x01', line.rstrip(b'\r'))


def skeleton_text(line: bytes) -> bytes:
    """the same for a line that was decoded as UTF-8 (with U+FFFD)"""
    out = []
    in_run = False
    text = line.rstrip(b'\r').decode('utf-8', errors='replace')
    for ch in text:
        if ord(ch) >= 0x80:
            if not in_run:
                out.append('\x01')
                in_run = True
        else:
            out.append(ch)
            in_run = False
    return ''.join(out).encode('utf-8')


def git_show(rev: str, rel: str):
    try:
        return subprocess.check_output(['git', 'show', rev + ':' + rel], cwd=REPO,
                                       stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return None


def build_indexes(rel: str):
    """skeleton -> list of (revision, line index) for every reference revision"""
    index = {}
    for rev in REVISIONS:
        data = git_show(rev, rel)
        if data is None:
            continue
        for i, line in enumerate(data.replace(b'\r\n', b'\n').split(b'\n')):
            index.setdefault(skeleton(line), []).append((rev, i))
    return index


def repair(path: str, check_only: bool) -> int:
    rel = os.path.relpath(path, REPO).replace('\\', '/')
    with open(path, 'rb') as f:
        cur = f.read()

    fixed = 0
    unresolved = []

    if FFFD in cur:
        index = build_indexes(rel)
        cache = {}
        cur_lines = cur.replace(b'\r\n', b'\n').split(b'\n')
        out = []
        for n, line in enumerate(cur_lines):
            if FFFD not in line:
                out.append(line)
                continue
            sk = skeleton_text(line)
            candidates = index.get(sk, [])
            best = None
            for rev, i in candidates:
                if rev not in cache:
                    data = git_show(rev, rel)
                    cache[rev] = data.replace(b'\r\n', b'\n').split(b'\n') if data else []
                restored = cache[rev][i]
                if FFFD in restored:
                    # this revision is damaged as well - look for an intact one
                    continue
                if best is None or abs(i - n) < abs(best[1] - n):
                    best = (rev, i, restored)
            if best is None:
                unresolved.append((n, line))
                out.append(line)
                continue
            out.append(best[2])
            fixed += 1

        for n, line in unresolved:
            print('UNRESOLVED %s:%d  %s' % (rel, n + 1, line[:90]))
        cur = b'\n'.join(out)

    # the working tree uses CRLF
    normalized = cur.replace(b'\r\n', b'\n').replace(b'\r', b'\n').replace(b'\n', b'\r\n')
    changed = normalized != cur
    if not check_only and (fixed or changed):
        with open(path, 'wb') as f:
            f.write(normalized)
    print('%-45s repaired=%d unresolved=%d eol=%s' % (rel, fixed, len(unresolved),
                                                     'crlf' if changed else 'ok'))
    return fixed


def main() -> int:
    check_only = '--check' in sys.argv
    files = subprocess.check_output(['git', 'ls-files'], cwd=REPO).decode().split()
    files = [f for f in files if f.endswith(('.cpp', '.h'))]
    total = 0
    for rel in files:
        path = os.path.join(REPO, rel)
        try:
            with open(path, 'rb') as f:
                if FFFD not in f.read():
                    continue
        except IOError:
            continue
        total += repair(path, check_only)
    print('total repaired lines: %d' % total)
    return 0


if __name__ == '__main__':
    sys.exit(main())
