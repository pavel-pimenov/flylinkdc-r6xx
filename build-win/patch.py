#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
patch.py - byte level patch helper.

The sources of the project store the Cyrillic comments in CP1251, so any tool that
decodes the file as UTF-8 destroys them. This helper works on raw bytes: the old
and the new text are given as files (or as arguments) and are replaced verbatim.

Usage:
    python build-win/patch.py --file <path> --old <text> --new <text> [--count N]
    python build-win/patch.py --file <path> --old-file <path> --new-file <path>

The strings are interpreted as UTF-8 text and encoded back to bytes, so only
ASCII (or explicitly written) text may be used.
"""
import io
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main() -> int:
    args = sys.argv[1:]
    path = None
    old = None
    new = None
    count = 1
    i = 0
    while i < len(args):
        if args[i] == '--file':
            path = args[i + 1]
            i += 2
        elif args[i] == '--old':
            old = args[i + 1]
            i += 2
        elif args[i] == '--new':
            new = args[i + 1]
            i += 2
        elif args[i] == '--old-file':
            with io.open(args[i + 1], 'r', encoding='utf-8', newline='') as f:
                old = f.read()
            i += 2
        elif args[i] == '--new-file':
            with io.open(args[i + 1], 'r', encoding='utf-8', newline='') as f:
                new = f.read()
            i += 2
        elif args[i] == '--count':
            count = int(args[i + 1])
            i += 2
        else:
            print('unknown argument: %s' % args[i], file=sys.stderr)
            return 2

    if path is None or old is None or new is None:
        print(__doc__, file=sys.stderr)
        return 2

    full = path if os.path.isabs(path) else os.path.join(REPO, path)
    with open(full, 'rb') as f:
        data = f.read()

    # the working tree uses CRLF - the patterns are given with LF
    crlf = data.count(b'\r\n') > data.count(b'\n') - data.count(b'\r\n')
    ob = old.encode('utf-8')
    nb = new.encode('utf-8')
    if crlf:
        ob = ob.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        nb = nb.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    n = data.count(ob)
    if n == 0:
        print('PATTERN NOT FOUND in %s' % path, file=sys.stderr)
        return 1
    if count and n > count:
        print('PATTERN FOUND %d times (expected %d) in %s' % (n, count, path), file=sys.stderr)
        return 1
    data = data.replace(ob, nb, count if count else -1)
    with open(full, 'wb') as f:
        f.write(data)
    print('patched %s (%d occurrence(s))' % (path, n))
    return 0


if __name__ == '__main__':
    sys.exit(main())
