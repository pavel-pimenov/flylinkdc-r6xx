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

#ifndef DCPLUSPLUS_FLY_THEME_DLG_H
#define DCPLUSPLUS_FLY_THEME_DLG_H

#pragma once

#include <atlbase.h>
#include <atlwin.h>

#include "Colors.h"

// The mixin that paints a dialog and its controls with the dark theme of the
// application. The dialog background becomes dark and the text of the controls
// becomes light - without it the text of the dialogs is painted with the
// system color COLOR_WINDOWTEXT (black) and it is not readable on the dark
// background.
//
// The class must be added to the list of the base classes of the dialog and the
// message map of the dialog must chain to it:
//
//   class MyDlg : public CDialogImpl<MyDlg>, public CFlyThemeDlg
//   {
//       BEGIN_MSG_MAP(MyDlg)
//       CHAIN_MSG_MAP(CFlyThemeDlg)
//       MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
//       END_MSG_MAP()
//   };
//
// The chain must be the first entry of the message map: the WM_CTLCOLOR*
// messages must be handled before the handlers of the dialog itself.
class CFlyThemeDlg
{
	public:
		LRESULT onInitDialog(UINT /*p_uMsg*/, WPARAM p_wParam, LPARAM /*p_lParam*/, BOOL& p_bHandled)
		{
			// the dark mode of the OS must be applied to the controls of the
			// dialog: the buttons, the statics and the lists of the dialog are
			// painted by the OS and they are light without the opt-in
			const HWND l_hWnd = reinterpret_cast<HWND>(p_wParam);
			if (SettingsManager::isDarkTheme() && l_hWnd)
			{
				FlyTheme::applyDarkModeToWindow(l_hWnd, true);
			}
			// the dialog handles the message itself
			p_bHandled = FALSE;
			return 0;
		}

		LRESULT onCtlColor(UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam, BOOL& p_bHandled)
		{
			if (!SettingsManager::isDarkTheme())
			{
				// the message is not handled: the dialog is painted with the
				// system colors of the light theme
				p_bHandled = FALSE;
				return 0;
			}
			return FlyTheme::handleCtlColor(p_uMsg, p_wParam, p_lParam);
		}

		BEGIN_MSG_MAP(CFlyThemeDlg)
			MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
			MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLORBTN, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLORSCROLLBAR, onCtlColor)
			MESSAGE_HANDLER(WM_CTLCOLORMSGBOX, onCtlColor)
		END_MSG_MAP()
};

#endif // DCPLUSPLUS_FLY_THEME_DLG_H
