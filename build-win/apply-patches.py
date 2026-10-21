#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
apply-patches.py - apply a list of byte level patches described in a JSON file.

The JSON file contains a list of objects:
    {"file": "windows/MessagePanel.cpp", "old": "...", "new": "..."}
All the strings are plain text with "\n" line breaks; they are converted to the
line ending of the target file automatically.

The optional key "enc" selects the encoding of the patched text: "utf-8" (default)
or "cp1251" - the sources of the project keep the Russian comments in CP1251,
so a patch that touches such a line must be encoded the same way.
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
        enc = p.get('enc', 'utf-8')
        ob = p['old'].encode(enc)
        nb = p['new'].encode(enc)
        if crlf:
            ob = ob.replace(b'\n', b'\r\n')
            nb = nb.replace(b'\n', b'\r\n')
        n = data.count(ob)
        count = p.get('count', 1)
        if count == 0:
            # replace all the occurrences of the pattern
            if n == 0:
                print('SKIP %s: pattern not found' % p['file'])
                continue
            data = data.replace(ob, nb)
        else:
            if n != count:
                print('SKIP %s: pattern found %d times (expected %d)' % (p['file'], n, count))
                continue
            data = data.replace(ob, nb, count)
        with open(full, 'wb') as f:
            f.write(data)
        print('patched %s' % p['file'])
        ok += 1
    print('%d/%d applied' % (ok, len(patches)))
    return 0 if ok == len(patches) else 1


if __name__ == '__main__':
    sys.exit(main())
