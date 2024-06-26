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

#if !defined(_INTEGRATION_PAGE_H_)
#define _INTEGRATION_PAGE_H_

#pragma once

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class IntegrationPage : public CPropertyPage<IDD_INTEGRATION_PAGE>, public PropPage
{
	public:
		explicit IntegrationPage() : PropPage(TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_INTEGRATION_PROP))
			, _isShellIntegration(false)
			, _isStartupIntegration(false)
			, _canShellIntegration(false)
		{
			SetTitle(m_title.c_str());
			m_psp.dwFlags |= PSP_RTLREADING;
		}
		
		~IntegrationPage()
		{
			m_ctrlList.Detach();
		}
		
		BEGIN_MSG_MAP_EX(IntegrationPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		NOTIFY_HANDLER(IDC_INTEGRATION_BOOLEANS, NM_CUSTOMDRAW, m_ctrlList.onCustomDraw)
		COMMAND_ID_HANDLER(IDC_INTEGRATION_IFACE_BTN_AUTOSTART, OnClickedMakeStartup)
		CHAIN_MSG_MAP(PropPage)
		END_MSG_MAP()
		
		LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnClickedMakeStartup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		
		// Common PropPage interface
		PROPSHEETPAGE *getPSP()
		{
			return (PROPSHEETPAGE *) * this;
		}
		void write(); // {}
		void cancel()
		{
			cancel_check();
		}
	protected:
		void CheckStartupIntegration();
		
	private:
		bool _canShellIntegration;
		bool _isShellIntegration;
		bool _isStartupIntegration;
		
		
		static Item items[];
		static TextItem texts[];
		static ListItem listItems[];
		
		ExListViewCtrl m_ctrlList;
		
};



#endif //_INTEGRATION_PAGE_H_
