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
#if !defined(MESSAGE_PANEL_H)
#define MESSAGE_PANEL_H

#pragma once

#include "WinUtil.h"
#include "EmoticonsDlg.h"

class MessagePanel
{
		BEGIN_MSG_MAP(MessagePanel)
		COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
		COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + GetEmotionMenuItemsCount(), onEmoPackChange)
		END_MSG_MAP()
		
	public:
		explicit MessagePanel(CEdit*& p_ctrlMessage);
		~MessagePanel();
		LRESULT  InitPanel(HWND& p_hWnd, RECT &p_rcDefault);
		void DestroyPanel(bool p_is_shutdown);
		LRESULT  UpdatePanel(CRect& rect);
		static int GetPanelWidth();
		int GetEmotionMenuItemsCount()
		{
			return g_emoMenu.GetItemsCount();
		}
		LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
		LRESULT onEmoPackChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		BOOL OnContextMenu(POINT& pt, WPARAM& wParam);
		
	private:
		CFlyToolTipCtrl m_tooltip;
		CEdit*& m_ctrlMessage;
		CButton ctrlShowUsers;
		CButton ctrlEmoticons;
		CButton ctrlSendMessageBtn;
		CButton ctrlMultiChatBtn;
		CButton ctrlBoldBtn;
		CButton ctrlStrikeBtn;
		CButton ctrlItalicBtn;
		CButton ctrlUnderlineBtn;
#ifdef FLYLINKDC_USE_BB_SIZE_CODE
		CComboBox ctrlSizeSel;
#endif
		CButton ctrlTransCodeBtn;
#ifdef SCALOLAZ_BB_COLOR_BUTTON
		CButton ctrlColorBtn;
#endif
		CButton ctrlOSAGOBtn;
		static CEmotionMenu g_emoMenu;
		HWND m_hWnd;
		bool m_isShutdown;
		
		static HIconWrapper g_hSendMessageIco;
		static HIconWrapper g_hMultiChatIco;
		static HIconWrapper g_hEmoticonIco;
		static HIconWrapper g_hBoldIco;
		static HIconWrapper g_hUndelineIco;
		static HIconWrapper g_hStrikeIco;
		static HIconWrapper g_hItalicIco;
		static HIconWrapper g_hTransCodeIco;
#ifdef SCALOLAZ_BB_COLOR_BUTTON
		static HIconWrapper g_hColorIco;
#endif
		
};


#endif //MESSAGE_PANEL_H