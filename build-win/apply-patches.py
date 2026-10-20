#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
apply-patches.py - apply a list of byte level patches described in a JSON file.

The JSON file contains a list of objects:
    {"file": "windows/MessagePanel.cpp", "old": "...", "new": "..."}
All the strings are plain text with "\n" line breaks; they are converted to the
line ending of the target file automatically.
"""
import io
import json
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main() -> int:
    spec_path = sys.argv[1]
    with io.open(spec_path, 'r', encoding='utf-8') as f:
        patches = json.load(f)
    ok = 0
    for p in patches:
        full = os.path.join(REPO, p['file'])
        with open(full, 'rb') as f:
            data = f.read()
        crlf = data.count(b'\r\n') > data.count(b'\n') - data.count(b'\r\n')
        ob = p['old'].encode('utf-8')
        nb = p['new'].encode('utf-8')
        if crlf:
            ob = ob.replace(b'\n', b'\r\n')
            nb = nb.replace(b'\n', b'\r\n')
        n = data.count(ob)
        if n != 1:
            print('SKIP %s: pattern found %d times' % (p['file'], n))
            continue
        data = data.replace(ob, nb, 1)
        with open(full, 'wb') as f:
            f.write(data)
        print('patched %s' % p['file'])
        ok += 1
    print('%d/%d applied' % (ok, len(patches)))
    return 0 if ok == len(patches) else 1


if __name__ == '__main__':
    sys.exit(main())
