/*
 * Copyright (C) 2011-2022 FlylinkDC++ Team https://www.fly-server.ru
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_FLY_THEME_COLORS_H
#define DCPLUSPLUS_FLY_THEME_COLORS_H

#pragma once

#include <windows.h>

#include "../client/SettingsManager.h"

namespace FlyTheme
{
	// The dark palette of the application. The values are close to the ones used
	// by Windows 11 (dark mode), so the parts of the UI that are drawn by the OS
	// look consistent with the parts that are drawn by the application itself.
	inline COLORREF getSysColor(int nIndex)
	{
		if (!SettingsManager::isDarkTheme())
		{
			return ::GetSysColor(nIndex);
		}
		switch (nIndex)
		{
			case COLOR_SCROLLBAR:
			case COLOR_WINDOW:
			case COLOR_BTNFACE:
			case COLOR_MENU:
			case COLOR_MENUBAR:
			case COLOR_3DLIGHT:
			case COLOR_BTNHIGHLIGHT:
			case COLOR_APPWORKSPACE:
			case COLOR_DESKTOP:
				return RGB(36, 38, 40);
			case COLOR_WINDOWTEXT:
			case COLOR_MENUTEXT:
			case COLOR_BTNTEXT:
			case COLOR_CAPTIONTEXT:
			case COLOR_INACTIVECAPTIONTEXT:
				return RGB(224, 224, 225);
			case COLOR_HIGHLIGHT:
			case COLOR_MENUHILIGHT:
				return RGB(25, 62, 120);
			case COLOR_HIGHLIGHTTEXT:
				return RGB(255, 255, 255);
			case COLOR_GRAYTEXT:
			case COLOR_BTNSHADOW:
				return RGB(110, 112, 116);
			case COLOR_HOTLIGHT:
				return RGB(110, 170, 255);
			case COLOR_INFOBK:
				return RGB(42, 45, 50);
			case COLOR_INFOTEXT:
				return RGB(224, 224, 225);
			case COLOR_WINDOWFRAME:
			case COLOR_ACTIVEBORDER:
			case COLOR_INACTIVEBORDER:
				return RGB(20, 22, 24);
			case COLOR_3DDKSHADOW:
				return RGB(12, 13, 14);
			case COLOR_ACTIVECAPTION:
			case COLOR_GRADIENTACTIVECAPTION:
				return RGB(40, 44, 52);
			case COLOR_INACTIVECAPTION:
			case COLOR_GRADIENTINACTIVECAPTION:
				return RGB(36, 38, 40);
			default:
				return ::GetSysColor(nIndex);
		}
	}

	// The background of the dialogs and of the controls that are painted by the
	// application itself. In the dark theme the color does not depend on the
	// user settings: the text of the dialogs must always be readable.
	inline COLORREF getDialogBgColor()
	{
		return SettingsManager::isDarkTheme() ? RGB(36, 38, 40) : ::GetSysColor(COLOR_WINDOW);
	}

	inline COLORREF getDialogTextColor()
	{
		return SettingsManager::isDarkTheme() ? RGB(224, 224, 225) : ::GetSysColor(COLOR_WINDOWTEXT);
	}

	struct BrushCache
	{
		HBRUSH m_brush = nullptr;
		COLORREF m_color = 0xFFFFFFFF;
	};

	inline BrushCache& getBrushCache()
	{
		static BrushCache s_cache;
		return s_cache;
	}

	// The solid brush with the current dialog background color. The brush is
	// cached: creating a new GDI object for every WM_CTLCOLOR* message would
	// leak the handle on every repaint of every control.
	inline HBRUSH getDialogBrush()
	{
		BrushCache& l_cache = getBrushCache();
		const COLORREF l_color = getDialogBgColor();
		if (l_cache.m_brush == nullptr || l_cache.m_color != l_color)
		{
			if (l_cache.m_brush)
			{
				::DeleteObject(l_cache.m_brush);
			}
			l_cache.m_brush = ::CreateSolidBrush(l_color);
			l_cache.m_color = l_color;
		}
		return l_cache.m_brush;
	}

	inline void releaseDialogBrush()
	{
		BrushCache& l_cache = getBrushCache();
		if (l_cache.m_brush)
		{
			::DeleteObject(l_cache.m_brush);
			l_cache.m_brush = nullptr;
			l_cache.m_color = 0xFFFFFFFF;
		}
	}

	// The common part of the WM_CTLCOLOR* handlers of the dialogs: the
	// background becomes dark and the text of the controls becomes light.
	// The value that must be returned from the handler is the brush.
	inline LRESULT handleCtlColor(UINT p_uMsg, WPARAM p_wParam, LPARAM /*p_lParam*/)
	{
		HDC l_hdc = reinterpret_cast<HDC>(p_wParam);
		// the static controls are painted with the background of the dialog,
		// the rest of the controls paint their own background
		const bool l_isStatic = (p_uMsg == WM_CTLCOLORSTATIC);
		::SetTextColor(l_hdc, getDialogTextColor());
		::SetBkColor(l_hdc, getDialogBgColor());
		::SetBkMode(l_hdc, l_isStatic ? TRANSPARENT : OPAQUE);
		return (LRESULT)getDialogBrush();
	}

	// The undocumented API of uxtheme.dll that enables the dark mode of the OS
	// for a window (Windows 10 2004 and newer). The API is loaded dynamically:
	// the header is used by the client library too and it must not add a link
	// dependency to uxtheme.lib.
	inline void allowDarkModeForWindow(HWND p_hWnd, bool p_dark)
	{
		typedef BOOL (WINAPI *AllowDarkModeForWindowFn)(HWND, BOOL);
		static AllowDarkModeForWindowFn s_allowDarkModeForWindow = nullptr;
		static bool s_isInitialized = false;
		if (!s_isInitialized)
		{
			s_isInitialized = true;
			HMODULE l_uxtheme = ::GetModuleHandle(_T("uxtheme.dll"));
			if (!l_uxtheme)
			{
				l_uxtheme = ::LoadLibrary(_T("uxtheme.dll"));
			}
			if (l_uxtheme)
			{
				s_allowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowFn>(
					::GetProcAddress(l_uxtheme, MAKEINTRESOURCEA(133)));
			}
		}
		if (s_allowDarkModeForWindow && p_hWnd)
		{
			s_allowDarkModeForWindow(p_hWnd, p_dark ? TRUE : FALSE);
		}
	}

	inline void setWindowTheme(HWND p_hWnd, const wchar_t* p_theme)
	{
		typedef HRESULT (WINAPI *SetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);
		static SetWindowThemeFn s_setWindowTheme = nullptr;
		static bool s_isInitialized = false;
		if (!s_isInitialized)
		{
			s_isInitialized = true;
			HMODULE l_uxtheme = ::GetModuleHandle(_T("uxtheme.dll"));
			if (!l_uxtheme)
			{
				l_uxtheme = ::LoadLibrary(_T("uxtheme.dll"));
			}
			if (l_uxtheme)
			{
				s_setWindowTheme = reinterpret_cast<SetWindowThemeFn>(::GetProcAddress(l_uxtheme, "SetWindowTheme"));
			}
		}
		if (s_setWindowTheme)
		{
			s_setWindowTheme(p_hWnd, p_theme, nullptr);
		}
	}

	// The classes of the controls that are painted by the OS with the
	// "DarkMode_Explorer" theme.
	inline bool isDarkThemeWindowClass(const wchar_t* p_class)
	{
		static const wchar_t* const g_classes[] =
		{
			L"SysListView32", L"SysTreeView32", L"SysHeader32", L"ScrollBar",
			L"Edit", L"ComboBox", L"ComboBoxEx32", L"msctls_statusbar32", L"tooltips_class32",
			L"Button", L"Static", L"ListBox", L"SysTabControl32", L"ToolbarWindow32",
			L"ReBarWindow32", L"msctls_trackbar32", L"msctls_progress32", L"msctls_updown32",
			L"RichEdit20W", L"RichEdit50W", L"RICHEDIT20W", L"RICHEDIT50W", L"RichEdit", L"RICHEDIT"
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

	// Applies the dark mode of the OS to the window and to all its children.
	inline void applyDarkModeToWindow(HWND p_hWnd, bool p_dark)
	{
		if (!p_hWnd)
		{
			return;
		}
		wchar_t l_class[64] = { 0 };
		if (::GetClassName(p_hWnd, l_class, _countof(l_class)))
		{
			if (isDarkThemeWindowClass(l_class))
			{
				setWindowTheme(p_hWnd, p_dark ? L"DarkMode_Explorer" : nullptr);
			}
			// the per window opt-in is required for the controls that are not a
			// part of the "DarkMode_Explorer" theme
			allowDarkModeForWindow(p_hWnd, p_dark);
		}
		for (HWND l_child = ::GetWindow(p_hWnd, GW_CHILD); l_child; l_child = ::GetWindow(l_child, GW_HWNDNEXT))
		{
			applyDarkModeToWindow(l_child, p_dark);
		}
	}
}

#endif // DCPLUSPLUS_FLY_THEME_COLORS_H
