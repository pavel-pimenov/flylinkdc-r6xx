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
	inline COLORREF getSysColor(int nIndex)
	{
		if (!SettingsManager::isDarkTheme())
		{
			return ::GetSysColor(nIndex);
		}
		switch (nIndex)
		{
			case COLOR_WINDOW:
			case COLOR_BTNFACE:
			case COLOR_MENU:
			case COLOR_3DLIGHT:
			case COLOR_BTNHIGHLIGHT:
			case COLOR_APPWORKSPACE:
				return RGB(36, 38, 40);
			case COLOR_WINDOWTEXT:
			case COLOR_MENUTEXT:
			case COLOR_BTNTEXT:
			case COLOR_CAPTIONTEXT:
				return RGB(224, 224, 225);
			case COLOR_HIGHLIGHT:
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
				return RGB(20, 22, 24);
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
}

#endif // DCPLUSPLUS_FLY_THEME_COLORS_H