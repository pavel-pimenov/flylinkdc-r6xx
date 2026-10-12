#!/usr/bin/env python3
# vendor-update.py - fetch upstream sources of vendored libs, sync them into the repo and commit.
# Usage: python vendor-update.py [--vendor NAME] [--dry-run] [--no-commit] [--list]
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parent
CACHE = REPO / 'build-win' / 'vendor-cache'
TIMEOUT = 600

VENDORS = (
    {
        'name': 'sqlite',
        'url': 'https://www.sqlite.org/2026/sqlite-amalgamation-3540000.zip',
        'local': 'client/sqlite',
        'strip': 1,
        'files': ['shell.c', 'sqlite3.c', 'sqlite3.h', 'sqlite3ext.h'],
        'patches': [
            ('sqlite3.c',
             '#ifndef SQLITE_AMALGAMATION\n#define SQLITE_CORE 1',
             '#include "sqlite_fly.h"\n\n'
             '#pragma warning(disable: 4127) //warning C4127: conditional expression is constant\n'
             '#pragma warning(disable: 4244) //conversion from \'x\' to \'y\', possible loss of data\n'
             '#pragma warning(disable: 4100) //unreferenced formal parameter\n\n'
             '#ifndef SQLITE_AMALGAMATION\n#define SQLITE_CORE 1'),
            ('sqlite3.h',
             '#ifndef SQLITE3_H\n#define SQLITE3_H\n'
             '#include <stdarg.h>     /* Needed for the definition of va_list */',
             '#pragma once //[+]FlylinkDC++\n\n'
             '#ifndef SQLITE3_H\n#define SQLITE3_H\n'
             '#include <stdarg.h>     /* Needed for the definition of va_list */\n\n\n'
             '#include "sqlite_fly.h" //[+]FlylinkDC++'),
        ],
        'version': {'paths': ['sqlite3.h'],
                    'pattern': r'#define\s+SQLITE_VERSION\s+"([^"]+)"'},
    },
    {
        'name': 'wtl',
        'url': 'https://sourceforge.net/projects/wtl/files/WTL%2010/'
               'WTL%2010.0.10320%20Release/WTL10_10320_Release.zip/download',
        'local': 'wtl',
        'map': {'Include': ''},
        'patches': [
            ('atlcrack.h',
             'BOOL m_bMsgHandled;',
             'BOOL m_bMsgHandled = FALSE;'),
            ('atldlgs.h',
             'CPropertyPageImpl(ATL::_U_STRINGorID title = (LPCTSTR)NULL)',
             'CPropertyPageImpl(ATL::_U_STRINGorID title = 0U)'),
            ('atldlgs.h',
             'CPropertyPage(ATL::_U_STRINGorID title = (LPCTSTR)NULL) : '
             'CPropertyPageImpl<CPropertyPage>(title)',
             'CPropertyPage(ATL::_U_STRINGorID title = 0U) : '
             'CPropertyPageImpl<CPropertyPage>(title)'),
            ('atlmisc.h',
             'ATL::CString strResult("file://");',
             'ATL::CString strResult(_T("file://"));'),
            ('atlribbon.h',
             '#include <atlmisc.h>    // for RecentDocumentList classes\n'
             '#include <atlframe.h>   // for Frame and UpdateUI classes\n'
             '#include <atlctrls.h>   // required for atlctrlw.h\n'
             '#include <atlctrlw.h>   // for CCommandBarCtrl',
             '#include "atlmisc.h"    // for RecentDocumentList classes\n'
             '#include "atlframe.h"   // for Frame and UpdateUI classes\n'
             '#include "atlctrls.h"   // required for atlctrlw.h\n'
             '#include "atlctrlw.h"   // for CCommandBarCtrl'),
            ('atlsplit.h',
             'enum { m_nPanesCount = 2, m_nPropMax = INT_MAX, m_cxyStep = 10 };',
             'enum { m_nPanesCount = 2, m_nPropMax = 10000, m_cxyStep = 10 };'),
            ('atlsplit.h',
             'return ((cxyTotal > 0) && (m_xySplitterPos >= 0)) ? '
             '::MulDiv(m_xySplitterPos, 100, cxyTotal) : -1;',
             'return ((cxyTotal > 0) && (m_xySplitterPos >= 0)) ? '
             '(::MulDiv(m_xySplitterPos, m_nPropMax, cxyTotal) / 100) : -1;'),
        ],
        'version': {'paths': ['atlapp.h', 'Include/atlapp.h'],
                    'pattern': r'//\s*version\s+([\d.]+)'},
    },
    {
        'name': 'zlib-ng',
        'url': 'https://github.com/zlib-ng/zlib-ng/archive/{pin}.zip',
        'pin': 'f18efb822a2fc24a1209a836d51bb217476fc08e',
        'local': 'zlib-ng',
        'strip': 1,
        'tree': True,
        'keep': ['zlib-ng.h', 'zconf-ng.h'],
        'version': {'paths': ['zlib-ng.h'],
                    'pattern': r'#define\s+ZLIBNG_VERSION\s+"([^"]+)"'},
    },
    {
        'name': 'jsoncpp',
        'url': 'https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.6.zip',
        'local': 'jsoncpp',
        'strip': 1,
        'generate': [sys.executable, 'amalgamate.py'],
        'select': {'dist/jsoncpp.cpp': 'jsoncpp.cpp',
                   'dist/json/json.h': 'include/json/json.h'},
        'diff': ['vendor-patches/jsoncpp.patch'],
        'version': {'paths': ['dist/json/json.h', 'include/json/json.h'],
                    'pattern': r'JSONCPP_VERSION_STRING\s+"([^"]+)"'},
    },
    {
        'name': 'maxminddb',
        'url': 'https://github.com/maxmind/libmaxminddb/releases/download/1.7.1/'
               'libmaxminddb-1.7.1.tar.gz',
        'local': 'maxminddb',
        'strip': 1,
        'select': {'src/data-pool.c': 'data-pool.c',
                   'src/data-pool.h': 'data-pool.h',
                   'src/maxminddb.c': 'maxminddb.c',
                   'src/maxminddb-compat-util.h': 'maxminddb-compat-util.h',
                   'include/maxminddb.h': 'maxminddb.h',
                   'projects/VS12/maxminddb_config.h': 'maxminddb_config.h'},
        'diff': ['vendor-patches/maxminddb.patch'],
        'version': {'paths': ['include/maxminddb.h', 'maxminddb.h'],
                    'pattern': r'#define\s+PACKAGE_VERSION\s+"([^"]+)"'},
    },
    {
        'name': 'boost',
        'url': 'https://archives.boost.io/release/1.88.0/source/boost_1_88_0.tar.gz',
        'local': 'boost',
        'strip': 1,
        'select': 'vendor-manifests/boost.txt',
        'diff': ['vendor-patches/boost.patch'],
        'version': {'paths': ['boost/version.hpp'],
                    'pattern': r'#define\s+BOOST_LIB_VERSION\s+"([^"]+)"'},
    },
    {
        'name': 'leveldb',
        'url': 'https://github.com/google/leveldb/archive/refs/tags/1.23.tar.gz',
        'local': 'leveldb',
        'strip': 1,
        'select': 'vendor-manifests/leveldb.txt',
        'diff': ['vendor-patches/leveldb.patch'],
        'version': {'paths': ['include/leveldb/db.h'],
                    'pattern': r'#define\s+LEVELDB_VER\s+"([^"]+)"'},
    },
    {
        'name': 'zenlib',
        'url': 'https://github.com/MediaArea/ZenLib/archive/refs/tags/v0.4.41.tar.gz',
        'local': 'Zenlib',
        'strip': 1,
        'select': 'vendor-manifests/zenlib.txt',
        'diff': ['vendor-patches/zenlib.patch'],
        'version': {'paths': ['History.txt'],
                    'pattern': r'^Version\s+([\d.]+)'},
    },
    {
        'name': 'bzip2',
        'url': 'https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz',
        'local': 'bzip2',
        'strip': 1,
        'files': ['blocksort.c', 'bzlib.c', 'bzlib.h', 'bzlib_private.h',
                  'compress.c', 'crctable.c', 'decompress.c', 'huffman.c',
                  'randtable.c', 'LICENSE'],
        'diff': ['vendor-patches/bzip2.patch'],
        'version': {'paths': ['bzlib.h'],
                    'pattern': r'version\s+([\d.]+)\s+of'},
    },
    {
        'name': 'lua',
        'url': 'https://www.lua.org/ftp/lua-5.4.6.tar.gz',
        'local': 'lua',
        'strip': 1,
        'select': 'vendor-manifests/lua.txt',
        'diff': ['vendor-patches/lua.patch'],
        'version': {'paths': ['src/lua.h', 'lua.h'],
                    'pattern': r'LUA_VERSION_MAJOR\s+"(\d+)"[\s\S]*?'
                               r'LUA_VERSION_MINOR\s+"(\d+)"[\s\S]*?'
                               r'LUA_VERSION_RELEASE\s+"(\d+)"'},
    },
    {
        'name': 'unrar',
        'url': 'https://www.rarlab.com/rar/unrarsrc-6.2.10.tar.gz',
        'local': 'unrar',
        'strip': 1,
        'select': 'vendor-manifests/unrar.txt',
        'diff': ['vendor-patches/unrar.patch'],
        'version': {'paths': ['version.hpp'],
                    'pattern': r'RARVER_MAJOR\s+(\d+)[\s\S]*?RARVER_MINOR\s+(\d+)'},
    },
    {
        'name': 'libtorrent',
        'url': 'https://github.com/arvidn/libtorrent/releases/download/v2.1.0/'
               'libtorrent-rasterbar-2.1.0.tar.gz',
        'local': 'libtorrent',
        'strip': 1,
        'select': 'vendor-manifests/libtorrent.txt',
        'diff': ['vendor-patches/libtorrent.patch'],
        'version': {'paths': ['include/libtorrent/version.hpp'],
                    'pattern': r'LIBTORRENT_VERSION_MAJOR\s+(\d+)[\s\S]*?'
                               r'LIBTORRENT_VERSION_MINOR\s+(\d+)[\s\S]*?'
                               r'LIBTORRENT_VERSION_TINY\s+(\d+)'},
    },
    {
        'name': 'mediainfolib',
        'url': 'https://github.com/MediaArea/MediaInfoLib/archive/refs/tags/v25.07.tar.gz',
        'local': 'mediainfolib',
        'strip': 1,
        'select': 'vendor-manifests/mediainfolib.txt',
        'diff': ['vendor-patches/mediainfolib.patch'],
        'version': {'paths': ['History_DLL.txt'],
                    'pattern': r'^Version\s+([\d.]+)'},
    },
    {
        'name': 'snappy',
        'url': 'https://github.com/google/snappy/archive/refs/tags/1.2.0.tar.gz',
        'local': 'snappy',
        'strip': 1,
        'select': 'vendor-manifests/snappy.txt',
        'diff': ['vendor-patches/snappy.patch'],
    },
    {
        'name': 'selene',
        'url': 'https://github.com/jeremyong/Selene/archive/{pin}.tar.gz',
        'pin': '5cde61c6c021ad1c096b03cc41c7d2325e0ca4eb',
        'local': 'selene',
        'strip': 1,
        'select': 'vendor-manifests/selene.txt',
        'diff': ['vendor-patches/selene.patch'],
    },
    {
        'name': 'openssl',
        'url': 'https://github.com/openssl/openssl/archive/{pin}.tar.gz',
        'pin': 'b372b1f76450acdfed1e2301a39810146e28b02c',
        'local': 'openssl',
        'strip': 1,
        'select': 'vendor-manifests/openssl.txt',
        'diff': ['vendor-patches/openssl.patch'],
        'version': {'paths': ['include/openssl/opensslv.h'],
                    'pattern': r'OPENSSL_VERSION_TEXT\s+"OpenSSL\s+(\S+)'},
    },
)


def log(msg):
    print(msg, flush=True)


def download(url, dest):
    log(f'  download {url}')
    req = urllib.request.Request(url, headers={'User-Agent': 'vendor-update.py'})
    with urllib.request.urlopen(req, timeout=TIMEOUT) as resp:
        with open(dest, 'wb') as f:
            shutil.copyfileobj(resp, f)
    log(f'  ok {dest.stat().st_size} bytes')


def extract(archive, dest):
    log(f'  extract {archive.name} -> {dest.parent.name}/{dest.name}')
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as z:
            z.extractall(dest)
    elif tarfile.is_tarfile(archive):
        with tarfile.open(archive) as t:
            t.extractall(dest)
    else:
        raise SystemExit(f'cannot extract {archive}')


def normalize_lf(root):
    for p in root.rglob('*'):
        if not p.is_file():
            continue
        data = p.read_bytes()
        if b'\x00' in data:
            continue
        fixed = data.replace(b'\r\n', b'\n')
        if fixed != data:
            p.write_bytes(fixed)


def source_root(vendor, extract_dir):
    root = Path(extract_dir)
    if vendor.get('strip'):
        kids = [p for p in root.iterdir() if p.is_dir()]
        if len(kids) == 1:
            root = kids[0]
    return root


def url(vendor):
    return vendor['url'].format(pin=vendor.get('pin', '')) if '{pin}' in vendor['url'] else vendor['url']


def archive_name(vendor):
    u = url(vendor)
    return vendor['name'] + '-' + hashlib.sha1(u.encode()).hexdigest()[:12] + \
        ('.zip' if '.zip' in u else '.tar.gz')


def read_version(root, spec):
    for rel in spec['paths']:
        path = root / rel
        if path.is_file():
            break
    else:
        return None
    try:
        text = path.read_text(encoding='utf-8')
    except UnicodeDecodeError:
        text = path.read_text(encoding='latin-1')
    m = re.search(spec['pattern'], text, re.MULTILINE)
    if not m:
        return None
    groups = m.groups()
    return '.'.join(g for g in groups if g) if len(groups) > 1 else m.group(1)


def normalize(p):
    return p.as_posix()


def rel_files(root):
    return {normalize(p.relative_to(root)): p for p in root.rglob('*') if p.is_file()}


def same_file(a, b):
    if not b.is_file() or a.stat().st_size != b.stat().st_size:
        return False
    return hashlib.sha256(a.read_bytes()).digest() == hashlib.sha256(b.read_bytes()).digest()


def plan(vendor, src, local):
    keep = set(vendor.get('keep', []))
    plan_copy = {}
    plan_delete = []
    if 'files' in vendor:
        for rel in vendor['files']:
            plan_copy[rel] = rel
    elif 'map' in vendor:
        for prefix, out in vendor['map'].items():
            base = src / prefix
            for rel, p in rel_files(base).items():
                out_rel = (out + '/' + rel) if out else rel
                src_rel = (prefix + '/' + rel) if prefix else rel
                plan_copy[out_rel] = src_rel
    elif 'select' in vendor:
        sel = vendor['select']
        if isinstance(sel, str):
            manifest = REPO / sel
            items = []
            for line in manifest.read_text(encoding='utf-8').splitlines():
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split()
                    items.append((parts[0], parts[1]) if len(parts) > 1 else (parts[0], parts[0]))
        elif isinstance(sel, (list, tuple)):
            items = [(p, p) for p in sel]
        else:
            items = list(sel.items())
        for src_rel, out_rel in items:
            s = src / src_rel
            if s.is_dir():
                for rel, p in rel_files(s).items():
                    out_ = (out_rel + '/' + rel) if out_rel else rel
                    src_ = (src_rel + '/' + rel) if src_rel else rel
                    plan_copy[out_] = src_
            elif s.is_file():
                plan_copy[out_rel] = src_rel
            else:
                raise SystemExit(f'{vendor["name"]}: select source missing: {src_rel}')
    elif vendor.get('tree'):
        up = rel_files(src)
        for rel, p in up.items():
            plan_copy[rel] = rel
        for rel, p in rel_files(local).items():
            if rel not in up and rel not in keep:
                plan_delete.append(rel)
    else:
        raise SystemExit(f'{vendor["name"]}: no import mode configured')

    added, updated, unchanged = [], [], []
    for out_rel, src_rel in plan_copy.items():
        s = src / src_rel
        d = local / out_rel
        if d.is_file():
            (unchanged if same_file(s, d) else updated).append(out_rel)
        else:
            added.append(out_rel)
    return {'copy': plan_copy, 'delete': plan_delete,
            'added': added, 'updated': updated, 'unchanged': unchanged}


def run_generate(cmd, cwd):
    shown = cmd if isinstance(cmd, str) else ' '.join(cmd)
    log(f'  generate: {shown}')
    r = subprocess.run(cmd, cwd=str(cwd), shell=isinstance(cmd, str),
                       capture_output=True, text=True)
    for line in (r.stdout or '').splitlines():
        log(f'    {line}')
    if r.returncode:
        print(r.stderr, file=sys.stderr)
        raise SystemExit(f'generate failed ({r.returncode})')


def apply_patches(local, patches):
    warnings = []
    for rel, old, new in patches:
        path = local / rel
        if not path.is_file():
            warnings.append(f'{rel}: file missing, patch not applied')
            continue
        try:
            text = path.read_text(encoding='utf-8')
        except UnicodeDecodeError:
            text = path.read_text(encoding='latin-1')
        if old not in text:
            warnings.append(f'{rel}: anchor not found, patch not applied (upstream changed?)')
            continue
        text = text.replace(old, new, 1)
        path.write_bytes(text.encode('utf-8'))
    for w in warnings:
        log(f'  ! patch warning: {w}')


def perform(vendor, src, local, plan_data):
    for rel in plan_data['delete']:
        p = local / rel
        p.unlink()
        log(f'  delete {rel}')
    for out_rel, src_rel in plan_data['copy'].items():
        s = src / src_rel
        d = local / out_rel
        if d.is_file() and same_file(s, d):
            continue
        d.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(s, d)
        log(f'  write {out_rel}')
    for empty in sorted([p for p in local.rglob('*') if p.is_dir() and not any(p.iterdir())],
                        key=lambda p: len(p.parts), reverse=True):
        empty.rmdir()
    for patch_rel in vendor.get('diff', []):
        patch = REPO / patch_rel
        log(f'  apply patch {patch_rel}')
        r = subprocess.run(['git', '-c', 'core.autocrlf=false', '-c', 'core.safecrlf=false',
                            'apply', '-p1', '--whitespace=nowarn',
                            '--directory=' + Path(vendor['local']).as_posix(), str(patch)],
                           cwd=str(REPO), capture_output=True, text=True)
        if r.returncode:
            print(r.stdout, file=sys.stdout)
            print(r.stderr, file=sys.stderr)
            raise SystemExit(f'{vendor["name"]}: patch failed: {patch_rel}')
    if vendor.get('patches'):
        log('  apply local patches')
        apply_patches(local, vendor['patches'])


def git_run(args):
    r = subprocess.run(args, cwd=str(REPO), capture_output=True, text=True)
    if r.returncode:
        print(r.stderr, file=sys.stderr)
        raise SystemExit(r.returncode)
    return r


def commit(paths, message):
    log(f'  git add {" ".join(paths)}')
    git_run(['git', 'add', '--'] + paths)
    status = git_run(['git', 'status', '--porcelain', '--'] + paths)
    if not status.stdout.strip():
        log('  no changes, skip commit')
        return
    log(f'  git commit -m "{message}"')
    git_run(['git', 'commit', '-m', message])


def process(vendor, args):
    name = vendor['name']
    local = REPO / vendor['local']
    if 'version' in vendor:
        old = read_version(local, vendor['version']) if local.is_dir() else None
    else:
        old = None
    log(f'[{name}] current version: {old}')

    cache = CACHE
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / archive_name(vendor)
    if not archive.is_file():
        download(url(vendor), archive)

    work = Path(tempfile.mkdtemp(prefix='vendor-update-'))
    try:
        extract_dir = work / name
        extract(archive, extract_dir)
        normalize_lf(extract_dir)
        src = source_root(vendor, extract_dir)
        if vendor.get('generate'):
            run_generate(vendor['generate'], src)
        new_ver = read_version(src, vendor['version']) if 'version' in vendor else None
        log(f'[{name}] upstream version:  {new_ver}')

        plan_data = plan(vendor, src, local)
        log(f'[{name}] changes: add={len(plan_data["added"])} '
            f'update={len(plan_data["updated"])} delete={len(plan_data["delete"])} '
            f'unchanged={len(plan_data["unchanged"])}')
        if plan_data['added']:
            log('  + ' + ' '.join(plan_data['added'][:8]) + ('...' if len(plan_data['added']) > 8 else ''))
        if plan_data['updated']:
            log('  ~ ' + ' '.join(plan_data['updated'][:8]) + ('...' if len(plan_data['updated']) > 8 else ''))

        if args.dry_run:
            log(f'[{name}] dry-run: no files touched')
            return

        if not local.is_dir():
            local.mkdir(parents=True)

        dirty = bool(plan_data['added'] or plan_data['updated'] or plan_data['delete'])
        if dirty:
            perform(vendor, src, local, plan_data)
        else:
            log(f'[{name}] already up to date')

        if args.no_commit:
            return
        if args.version:
            new_ver = args.version
        if local.is_dir() and 'version' in vendor:
            new_ver = read_version(local, vendor['version']) or new_ver
        if old and new_ver and old != new_ver:
            message = f'* vendor-update: {name} {old} -> {new_ver}'
        elif old and new_ver:
            message = f'* vendor-update: {name} {new_ver}'
        else:
            message = f'* vendor-update: sync {name}'
        commit_paths = [vendor['local']]
        if isinstance(vendor.get('select'), str):
            commit_paths.append(vendor['select'])
        commit_paths += vendor.get('diff', [])
        commit(commit_paths, message)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description='Update vendored third-party libs from upstream URLs')
    ap.add_argument('--vendor', action='append', choices=[v['name'] for v in VENDORS],
                    help='process only given vendor(s)')
    ap.add_argument('--list', action='store_true', help='list configured vendors and pins')
    ap.add_argument('--dry-run', action='store_true', help='download and report, touch nothing')
    ap.add_argument('--no-commit', action='store_true', help='sync files but do not commit')
    ap.add_argument('--version', help='override version tag used in commit message')
    args = ap.parse_args()

    if args.list:
        for v in VENDORS:
            pin = v.get('pin', '(none)')
            print(f'{v["name"]:10} pin={pin:42} local={v["local"]}')
            print(f'{"":10} url={url(v)}')
        return

    names = args.vendor or [v['name'] for v in VENDORS]
    for v in VENDORS:
        if v['name'] in names:
            process(v, args)


if __name__ == '__main__':
    main()