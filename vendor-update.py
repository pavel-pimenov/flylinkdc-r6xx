#!/usr/bin/env python3
# vendor-update.py - update vendored libs (WTL, zlib-ng, sqlite) from upstream URLs
from pathlib import Path
import sys, os, tempfile, urllib.request, zipfile, tarfile, shutil

REPO = Path(__file__).resolve().parent if '__file__' in globals() else Path('.').resolve()
BUILD_WIN = REPO / 'build-win'

def download(url, dest):
    print(f'Downloading {url} -> {dest}')
    req = urllib.request.urlopen(url, timeout=120)
    with open(dest, 'wb') as f:
        shutil.copyfileobj(req, f)
    print(f'  OK {dest.stat().st_size} bytes')

def main():
    print('vendor-update starting in', REPO)

if __name__ == '__main__':
    main()
