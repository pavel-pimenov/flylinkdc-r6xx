#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check-diff.py - verify that the diff of the working tree contains only the intended
changes: every changed line pair must either be a code change or a line with a
comment (the restored CP1251 comments).
"""
import io
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def is_comment(line):
    s = line.strip()
    return s.startswith('//') or s.startswith('/*') or s.startswith('*') or s.startswith('*/')


def main() -> int:
    diff = subprocess.check_output(['git', 'diff', '-U0'], cwd=REPO)
    text = diff.decode('utf-8', errors='replace')
    cur_file = None
    problems = []
    removed = []
    added = []
    for line in text.split('\n'):
        if line.startswith('+++ b/'):
            cur_file = line[6:]
            removed = []
            added = []
            continue
        if line.startswith('--- ') or line.startswith('@@') or line.startswith('diff ') or line.startswith('index '):
            continue
        if line.startswith('-') and not line.startswith('---'):
            removed.append(line[1:])
        elif line.startswith('+') and not line.startswith('+++'):
            added.append(line[1:])
        else:
            continue
    # the pairs are not aligned here - a simple check: every removed and every added
    # line must be a comment or must have a counterpart
    return 0


def main2():
    diff = subprocess.check_output(['git', 'diff', '-U0'], cwd=REPO).decode('utf-8', errors='replace')
    blocks = re.split(r'(?m)^(?=diff --git )', diff)
    only_comments = []
    with_code = []
    for b in blocks:
        if not b.strip():
            continue
        name = b.split('\n')[0].split(' b/')[-1]
        rem = [l[1:] for l in b.split('\n') if l.startswith('-') and not l.startswith('---')]
        add = [l[1:] for l in b.split('\n') if l.startswith('+') and not l.startswith('+++')]
        if all(is_comment(l) for l in rem) and all(is_comment(l) for l in add):
            only_comments.append(name)
        else:
            with_code.append((name, len(rem), len(add)))
    print('files with code changes (%d):' % len(with_code))
    for n, r, a in sorted(with_code, key=lambda x: -max(x[1], x[2])):
        print('   %-45s -%d +%d' % (n, r, a))
    print()
    print('files with comment-only changes (%d):' % len(only_comments))
    for n in only_comments:
        print('   %s' % n)
    return 0


if __name__ == '__main__':
    sys.exit(main2())
