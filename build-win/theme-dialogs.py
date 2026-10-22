#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
theme-dialogs.py - add the dark theme support (CFlyThemeDlg) to the dialogs of
the project.

The sources keep the Russian comments in CP1251, so the file is read and written
as raw bytes. The script is idempotent: a dialog that already has the mixin is
skipped.
"""
import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# file -> (include line, [ (class declaration old, class declaration new), ... ],
#          [ message map anchors, ... ])
EDITS = [
    ("windows/ADLSProperties.h", '#include "FlyThemeDlg.h"', [
        ("class ADLSProperties : public CDialogImpl<ADLSProperties>",
         "class ADLSProperties : public CDialogImpl<ADLSProperties>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(ADLSProperties)"]),
    ("windows/AboutCmdsDlg.h", '#include "FlyThemeDlg.h"', [
        ("class AboutCmdsDlg : public CDialogImpl<AboutCmdsDlg>",
         "class AboutCmdsDlg : public CDialogImpl<AboutCmdsDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(AboutCmdsDlg)"]),
    ("windows/AboutDlg.h", '#include "FlyThemeDlg.h"', [
        ("class AboutDlg : public CDialogImpl<AboutDlg>",
         "class AboutDlg : public CDialogImpl<AboutDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(AboutDlg)"]),
    ("windows/AboutDlgIndex.h", '#include "FlyThemeDlg.h"', [
        ("class AboutDlgIndex : public CDialogImpl<AboutDlgIndex>",
         "class AboutDlgIndex : public CDialogImpl<AboutDlgIndex>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(AboutDlgIndex)"]),
    ("windows/AboutLogDlg.h", '#include "FlyThemeDlg.h"', [
        ("class AboutLogDlg : public CDialogImpl<AboutLogDlg>, CFlyRichEditLoader",
         "class AboutLogDlg : public CDialogImpl<AboutLogDlg>, public CFlyThemeDlg, CFlyRichEditLoader"),
    ], ["BEGIN_MSG_MAP(AboutLogDlg)"]),
    ("windows/AboutStatDlg.h", '#include "FlyThemeDlg.h"', [
        ("class AboutStatDlg : public CDialogImpl<AboutStatDlg>",
         "class AboutStatDlg : public CDialogImpl<AboutStatDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(AboutStatDlg)"]),
    ("windows/AddMagnet.h", '#include "FlyThemeDlg.h"', [
        ("class AddMagnet : public CDialogImpl<AddMagnet>",
         "class AddMagnet : public CDialogImpl<AddMagnet>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(AddMagnet)"]),
    ("windows/CFlyLocationDlg.h", '#include "FlyThemeDlg.h"', [
        ("class CFlyLocationDlg : public CDialogImpl<CFlyLocationDlg>",
         "class CFlyLocationDlg : public CDialogImpl<CFlyLocationDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(CFlyLocationDlg)"]),
    ("windows/CheckTargetDlg.h", '#include "FlyThemeDlg.h"', [
        ("class CheckTargetDlg : public CDialogImpl< CheckTargetDlg >",
         "class CheckTargetDlg : public CDialogImpl< CheckTargetDlg >, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(CheckTargetDlg)"]),
    ("windows/CommandDlg.h", '#include "FlyThemeDlg.h"', [
        ("class CommandDlg : public CDialogImpl<CommandDlg>",
         "class CommandDlg : public CDialogImpl<CommandDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(CommandDlg)"]),
    ("windows/EmoticonsDlg.h", '#include "FlyThemeDlg.h"', [
        ("class EmoticonsDlg : public CDialogImpl<EmoticonsDlg>",
         "class EmoticonsDlg : public CDialogImpl<EmoticonsDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(EmoticonsDlg)"]),
    ("windows/FavDirDlg.h", '#include "FlyThemeDlg.h"', [
        ("class FavDirDlg : public CDialogImpl<FavDirDlg>",
         "class FavDirDlg : public CDialogImpl<FavDirDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP_EX(FavDirDlg)"]),
    ("windows/FavHubGroupsDlg.h", '#include "FlyThemeDlg.h"', [
        ("class FavHubGroupsDlg : public CDialogImpl<FavHubGroupsDlg>",
         "class FavHubGroupsDlg : public CDialogImpl<FavHubGroupsDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(FavHubGroupsDlg)"]),
    ("windows/FavHubProperties.h", '#include "FlyThemeDlg.h"', [
        ("class FavHubProperties : public CDialogImpl<FavHubProperties>",
         "class FavHubProperties : public CDialogImpl<FavHubProperties>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP_EX(FavHubProperties)"]),
    ("windows/FlyUpdateDlg.h", '#include "FlyThemeDlg.h"', [
        ("class FlyUpdateDlg : public CDialogImpl<FlyUpdateDlg>, CFlyRichEditLoader",
         "class FlyUpdateDlg : public CDialogImpl<FlyUpdateDlg>, public CFlyThemeDlg, CFlyRichEditLoader"),
    ], ["BEGIN_MSG_MAP(FlyUpdateDlg)"]),
    ("windows/HashProgressDlg.h", '#include "FlyThemeDlg.h"', [
        ("class HashProgressDlg : public CDialogImpl<HashProgressDlg>, private CFlyTimerAdapter",
         "class HashProgressDlg : public CDialogImpl<HashProgressDlg>, public CFlyThemeDlg, private CFlyTimerAdapter"),
    ], ["BEGIN_MSG_MAP(HashProgressDlg)"]),
    ("windows/LimitEditDlg.h", '#include "FlyThemeDlg.h"', [
        ("class LimitEditDlg: public CDialogImpl<LimitEditDlg>",
         "class LimitEditDlg: public CDialogImpl<LimitEditDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(LimitEditDlg)"]),
    ("windows/LineDlg.h", '#include "FlyThemeDlg.h"', [
        ("class LineDlg : public CDialogImpl<LineDlg>",
         "class LineDlg : public CDialogImpl<LineDlg>, public CFlyThemeDlg"),
        ("class KickDlg : public CDialogImpl<KickDlg>",
         "class KickDlg : public CDialogImpl<KickDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(LineDlg)", "BEGIN_MSG_MAP(KickDlg)"]),
    ("windows/MagnetDlg.h", '#include "FlyThemeDlg.h"', [
        ("class MagnetDlg : public CDialogImpl<MagnetDlg >",
         "class MagnetDlg : public CDialogImpl<MagnetDlg >, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(MagnetDlg)"]),
    ("windows/PreviewDlg.h", '#include "FlyThemeDlg.h"', [
        ("class PreviewDlg : public CDialogImpl<PreviewDlg>",
         "class PreviewDlg : public CDialogImpl<PreviewDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP_EX(CommandDlg)"]),
    ("windows/PreviewLogDlg.h", '#include "FlyThemeDlg.h"', [
        ("class PreviewLogDlg : public CDialogImpl<PreviewLogDlg>",
         "class PreviewLogDlg : public CDialogImpl<PreviewLogDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(PreviewLogDlg)"]),
    ("windows/RebuildMediainfoProgressDlg.h", '#include "FlyThemeDlg.h"', [
        ("class RebuildMediainfoProgressDlg : public CDialogImpl<RebuildMediainfoProgressDlg>, private CFlyTimerAdapter",
         "class RebuildMediainfoProgressDlg : public CDialogImpl<RebuildMediainfoProgressDlg>, public CFlyThemeDlg, private CFlyTimerAdapter"),
    ], ["BEGIN_MSG_MAP(RebuildMediainfoProgressDlg)"]),
    ("windows/SpeedVolDlg.h", '#include "FlyThemeDlg.h"', [
        ("class SpeedVolDlg: public CDialogImpl<SpeedVolDlg>",
         "class SpeedVolDlg: public CDialogImpl<SpeedVolDlg>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(SpeedVolDlg)"]),
    ("windows/dclstGenDlg.h", '#include "FlyThemeDlg.h"', [
        ("class DCLSTGenDlg : public CDialogImpl< DCLSTGenDlg >, public Thread",
         "class DCLSTGenDlg : public CDialogImpl< DCLSTGenDlg >, public CFlyThemeDlg, public Thread"),
    ], ["BEGIN_MSG_MAP(DCLSTGenDlg)"]),
    ("windows/TreePropertySheet.h", '#include "FlyThemeDlg.h"', [
        ("class TreePropertySheet : public CPropertySheetImpl<TreePropertySheet>,\n\tprotected CFlyTimerAdapter",
         "class TreePropertySheet : public CPropertySheetImpl<TreePropertySheet>,\n\tpublic CFlyThemeDlg,\n\tprotected CFlyTimerAdapter"),
    ], ["BEGIN_MSG_MAP(TreePropertySheet)"]),
    ("windows/Wizards/WzDownload.h", '#include "../FlyThemeDlg.h"', [
        ("class WzDownload : public CPropertyPageImpl<WzDownload>",
         "class WzDownload : public CPropertyPageImpl<WzDownload>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(WzDownload)"]),
    ("windows/Wizards/WzFinish.h", '#include "../FlyThemeDlg.h"', [
        ("class WzFinish : public CPropertyPageImpl<WzFinish>",
         "class WzFinish : public CPropertyPageImpl<WzFinish>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(WzFinish)"]),
    ("windows/Wizards/WzNetActive.h", '#include "../FlyThemeDlg.h"', [
        ("class WzNetActive : public CPropertyPageImpl<WzNetActive>",
         "class WzNetActive : public CPropertyPageImpl<WzNetActive>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(WzNetActive)"]),
    ("windows/Wizards/WzNick.h", '#include "../FlyThemeDlg.h"', [
        ("class WzNick : public CPropertyPageImpl<WzNick>",
         "class WzNick : public CPropertyPageImpl<WzNick>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(WzNick)"]),
    ("windows/Wizards/WzSetIP.h", '#include "../FlyThemeDlg.h"', [
        ("class WzSetIP : public CPropertyPageImpl<WzSetIP>",
         "class WzSetIP : public CPropertyPageImpl<WzSetIP>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(FlyWizard)"]),
    ("windows/Wizards/WzShare.h", '#include "../FlyThemeDlg.h"', [
        ("class WzShare : public CPropertyPageImpl<WzShare>",
         "class WzShare : public CPropertyPageImpl<WzShare>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(WzShare)"]),
    ("windows/Wizards/WzStartup.h", '#include "../FlyThemeDlg.h"', [
        ("class WzStartup : public CPropertyPageImpl<WzStartup>",
         "class WzStartup : public CPropertyPageImpl<WzStartup>, public CFlyThemeDlg"),
    ], ["BEGIN_MSG_MAP(FlyWizard)"]),
    ("client/FlyFeatures/CFlyServerDialogNavigator.h", '#include "../../windows/FlyThemeDlg.h"', [
        ("\tpublic CAxDialogImpl<CFlyServerDialogNavigator>\n",
         "\tpublic CAxDialogImpl<CFlyServerDialogNavigator>,\n\tpublic CFlyThemeDlg\n"),
        ("\tpublic CDialogImpl<CFlyServerDialogNavigator>,\n\tpublic CDialogResize<CFlyServerDialogNavigator>\n",
         "\tpublic CDialogImpl<CFlyServerDialogNavigator>,\n\tpublic CDialogResize<CFlyServerDialogNavigator>,\n\tpublic CFlyThemeDlg\n"),
    ], ["BEGIN_MSG_MAP(CFlyServerDialogNavigator)"]),
    ("client/FlyFeatures/CFlyTorrentDialog.h", '#include "../../windows/FlyThemeDlg.h"', [
        ("\tpublic CDialogImpl<CFlyTorrentDialog>\n",
         "\tpublic CDialogImpl<CFlyTorrentDialog>,\n\tpublic CFlyThemeDlg\n"),
    ], ["BEGIN_MSG_MAP(CFlyTorrentDialog)"]),
    ("client/FlyFeatures/CProgressDlg.h", '#include "../../windows/FlyThemeDlg.h"', [
        ("\tpublic CDialogImpl<CProgressDlg<T, idd, time> >,\n",
         "\tpublic CDialogImpl<CProgressDlg<T, idd, time> >,\n\tpublic CFlyThemeDlg,\n"),
    ], ["BEGIN_MSG_MAP_EX(CAboutDlg)"]),
]


def read_text(path):
    with open(path, 'rb') as f:
        return f.read().decode('cp1251')


def write_text(path, text):
    with open(path, 'wb') as f:
        f.write(text.encode('cp1251'))


def main():
    failed = []
    for rel, include_line, decls, maps in EDITS:
        full = os.path.join(REPO, rel)
        if not os.path.exists(full):
            failed.append('%s: file not found' % rel)
            continue
        raw = read_text(full)
        if 'CFlyThemeDlg' in raw:
            print('SKIP  %s (already patched)' % rel)
            continue
        eol = '\r\n' if '\r\n' in raw else '\n'
        text = raw.replace('\r\n', '\n')
        # 1. the base class of the dialog
        for old, new in decls:
            if text.count(old) != 1:
                failed.append('%s: declaration not found (%d): %s' % (rel, text.count(old), old[:60]))
                continue
            text = text.replace(old, new, 1)
        # 2. the chain in the message map
        for anchor in maps:
            if text.count(anchor) != 1:
                failed.append('%s: message map not found (%d): %s' % (rel, text.count(anchor), anchor))
                continue
            lines = text.split('\n')
            for i, line in enumerate(lines):
                if anchor in line:
                    indent = line[:len(line) - len(line.lstrip())]
                    lines.insert(i + 1, indent + 'CHAIN_MSG_MAP(CFlyThemeDlg)')
                    break
            text = '\n'.join(lines)
        # 3. the include
        if include_line not in text:
            lines = text.split('\n')
            last = -1
            for i, line in enumerate(lines):
                if line.lstrip().startswith('#include'):
                    last = i
            if last < 0:
                # the file has no include at all (the precompiled header is used):
                # the include is placed before the first class declaration
                for i, line in enumerate(lines):
                    if line.lstrip().startswith('class ') or line.lstrip().startswith('struct '):
                        last = i - 1
                        break
            if last < 0:
                failed.append('%s: no place for the include' % rel)
                continue
            lines.insert(last + 1, include_line)
            text = '\n'.join(lines)
        if eol != '\n':
            text = text.replace('\n', eol)
        write_text(full, text)
        print('OK    %s' % rel)
    if failed:
        print('')
        for f in failed:
            print('ERROR %s' % f)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
