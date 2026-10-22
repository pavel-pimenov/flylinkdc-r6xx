#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
theme-patches.py - the patches of the dark theme of the interface.

The sources of the project keep the Russian comments in CP1251, so the files are
read and written as raw bytes. The patterns are given with LF and are converted
to the line ending of the target file.
"""
import io
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PATCHES = []


def add(path, old, new, count=1):
    PATCHES.append((path, old, new, count))


# --- windows/WinUtil.cpp: the dark mode of the OS --------------------------------
add('windows/WinUtil.cpp', """	bool isDarkThemeWindowClass(const wchar_t* p_class)
	{
		static const wchar_t* const g_classes[] =
		{
			L"SysListView32", L"SysTreeView32", L"SysHeader32", L"ScrollBar",
			L"Edit", L"ComboBox", L"msctls_statusbar32", L"tooltips_class32"
		};
		for (const wchar_t* l_name : g_classes)
		{
			if (wcscmp(p_class, l_name) == 0)
			{
				return true;
			}
		}
		return false;
	}

	void applyDarkModeToWindow(HWND p_hWnd, bool p_dark)
	{
		if (!p_hWnd)
		{
			return;
		}
		wchar_t l_class[64] = { 0 };
		if (::GetClassName(p_hWnd, l_class, _countof(l_class)) && isDarkThemeWindowClass(l_class))
		{
			SetWindowTheme(p_hWnd, p_dark ? L"DarkMode_Explorer" : nullptr, nullptr);
		}
		for (HWND l_child = ::GetWindow(p_hWnd, GW_CHILD); l_child; l_child = ::GetWindow(l_child, GW_HWNDNEXT))
		{
			applyDarkModeToWindow(l_child, p_dark);
		}
	}

	BOOL CALLBACK applyDarkModeEnumProc""", """	BOOL CALLBACK applyDarkModeEnumProc""")

add('windows/WinUtil.cpp', """void WinUtil::applyDarkMode(HWND p_hWnd)
{
	typedef BOOL (WINAPI *AllowDarkModeForWindowFn)(HWND, BOOL);

	applyDarkTitleBar(p_hWnd);
	if (!isDarkModeSupported())
	{
		return;
	}
	const bool l_dark = SettingsManager::isDarkTheme();
	static const AllowDarkModeForWindowFn s_allowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowFn>(getUxThemeProc(MAKEINTRESOURCEA(133)));
	if (s_allowDarkModeForWindow && p_hWnd)
	{
		s_allowDarkModeForWindow(p_hWnd, l_dark ? TRUE : FALSE);
	}
	applyDarkModeToWindow(p_hWnd, l_dark);
}""", """void WinUtil::applyDarkMode(HWND p_hWnd)
{
	applyDarkTitleBar(p_hWnd);
	if (!isDarkModeSupported())
	{
		return;
	}
	const bool l_dark = SettingsManager::isDarkTheme();
	FlyTheme::allowDarkModeForWindow(p_hWnd, l_dark);
	FlyTheme::applyDarkModeToWindow(p_hWnd, l_dark);
}""")

# --- windows/main.cpp: the dark mode of the OS must be enabled before the windows
add('windows/main.cpp', """	SettingsManager::getInstance()->load();

	CrashReporter::init();""", """	SettingsManager::getInstance()->load();

	// the dark mode of the OS must be enabled before the first window of the
	// application is created, otherwise the menu and the toolbar are painted
	// with the light theme
	WinUtil::initDarkMode();

	CrashReporter::init();""")

# --- windows/PropPage.cpp: the text of the dialogs must be light ----------------
add('windows/PropPage.cpp', """LRESULT PropPage::OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (BOOLSETTING(SETTINGS_WINDOW_COLORIZE) || SettingsManager::isDarkTheme())
	{
		// the brush is reused for every WM_CTLCOLOR* message: create it only once
		// (and re-create it when the theme color changes), otherwise every repaint
		// of every control leaks a GDI brush
		if (!m_hDialogBrush || m_dialogBrushColor != Colors::g_bgColor)
		{
			if (m_hDialogBrush)
				DeleteObject(m_hDialogBrush);
			m_dialogBrushColor = Colors::g_bgColor;
			m_hDialogBrush = CreateSolidBrush(m_dialogBrushColor);
		}
		return LRESULT(m_hDialogBrush);
	}
	else
	{
		return 0;
	}
}
LRESULT PropPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (BOOLSETTING(SETTINGS_WINDOW_COLORIZE) || SettingsManager::isDarkTheme())
	{
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		return LRESULT(m_hDialogBrush);
	}
	else
	{
		return 0;
	}
}""", """LRESULT PropPage::OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (SettingsManager::isDarkTheme())
	{
		// the dark theme: the background of the dialog is dark and the text of
		// the controls is light (the system color COLOR_WINDOWTEXT is black and
		// it is not readable on the dark background)
		return FlyTheme::handleCtlColor(uMsg, wParam, lParam);
	}
	if (BOOLSETTING(SETTINGS_WINDOW_COLORIZE))
	{
		// the brush is reused for every WM_CTLCOLOR* message: create it only once
		// (and re-create it when the theme color changes), otherwise every repaint
		// of every control leaks a GDI brush
		if (!m_hDialogBrush || m_dialogBrushColor != Colors::g_bgColor)
		{
			if (m_hDialogBrush)
				DeleteObject(m_hDialogBrush);
			m_dialogBrushColor = Colors::g_bgColor;
			m_hDialogBrush = CreateSolidBrush(m_dialogBrushColor);
		}
		return LRESULT(m_hDialogBrush);
	}
	else
	{
		return 0;
	}
}
LRESULT PropPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (SettingsManager::isDarkTheme())
	{
		return FlyTheme::handleCtlColor(uMsg, wParam, lParam);
	}
	if (BOOLSETTING(SETTINGS_WINDOW_COLORIZE))
	{
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		return LRESULT(m_hDialogBrush);
	}
	else
	{
		return 0;
	}
}""")

# --- windows/NetworkPage.cpp: the handler must not block the chain --------------
add('windows/NetworkPage.cpp', """	*/
	return 0;
}""", """	*/
	// the message is not handled here: the chain to PropPage must not be
	// interrupted, otherwise the page is not painted with the theme of the
	// application
	bHandled = FALSE;
	return 0;
}""")

# --- windows/PropPageTextStyles.cpp: the handler must not block the chain -------
add('windows/PropPageTextStyles.cpp', """LRESULT PropPageTextStyles::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	HWND hWnd = (HWND)lParam;
	if (hWnd == ctrlTabExample.m_hWnd)
	{
		::SetBkMode((HDC)wParam, TRANSPARENT);
		HANDLE h = GetProp(hWnd, _T("fillcolor"));
		if (h != NULL)
		{
			return (LRESULT)h;
		}
		// the value of the message must be a brush - the default one is used
		return 0;
	}
	else
	{
		return FALSE;
	}
}""", """LRESULT PropPageTextStyles::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HWND hWnd = (HWND)lParam;
	if (hWnd == ctrlTabExample.m_hWnd)
	{
		::SetBkMode((HDC)wParam, TRANSPARENT);
		HANDLE h = GetProp(hWnd, _T("fillcolor"));
		if (h != NULL)
		{
			return (LRESULT)h;
		}
		// the value of the message must be a brush - the default one is used
		return 0;
	}
	else
	{
		// the message is not handled here: the chain to PropPage must not be
		// interrupted, otherwise the page is not painted with the theme of the
		// application
		bHandled = FALSE;
		return FALSE;
	}
}""")

# --- windows/CZDCPage.h: the page must use the common handler -------------------
add('windows/CZDCPage.h', """		BEGIN_MSG_MAP_EX(CZDCPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_WINAMP_HELP, BN_CLICKED, onClickedWinampHelp)
		END_MSG_MAP()""", """		BEGIN_MSG_MAP_EX(CZDCPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_WINAMP_HELP, BN_CLICKED, onClickedWinampHelp)
		CHAIN_MSG_MAP(PropPage)
		END_MSG_MAP()""")

# --- client/FlyFeatures/MiniHTML.h: the unknown color must not be black --------
add('client/FlyFeatures/MiniHTML.h', """				else clr = RGB(0, 0, 0);""",
    """				else clr = Colors::getSysColor(COLOR_WINDOWTEXT);""")

# --- windows/flattabctrl.h: the border of an inactive tab ----------------------
add('windows/flattabctrl.h', """					Gdiplus::Pen l_pen(Gdiplus::Color(160, 160, 160), 1);""",
    """					const COLORREF l_rgb_shadow = Colors::getSysColor(COLOR_BTNSHADOW);
					Gdiplus::Pen l_pen(Gdiplus::Color(GetRValue(l_rgb_shadow), GetGValue(l_rgb_shadow), GetBValue(l_rgb_shadow), 160), 1);""")


def main():
    failed = 0
    for path, old, new, count in PATCHES:
        full = os.path.join(REPO, path)
        with open(full, 'rb') as f:
            data = f.read()
        crlf = data.count(b'\r\n') > data.count(b'\n') - data.count(b'\r\n')
        ob = old.encode('utf-8')
        nb = new.encode('utf-8')
        if crlf:
            ob = ob.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
            nb = nb.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        n = data.count(ob)
        if n != count:
            if data.count(nb) == count:
                print('SKIP  %s: already applied: %s' % (path, old.split('\n')[0][:70]))
                continue
            print('ERROR %s: pattern found %d times (expected %d): %s' % (path, n, count, old.split('\n')[0][:70]))
            failed += 1
            continue
        data = data.replace(ob, nb, count)
        with open(full, 'wb') as f:
            f.write(data)
        print('OK    %s: %s' % (path, old.split('\n')[0][:70]))
    print('%d/%d applied' % (len(PATCHES) - failed, len(PATCHES)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
