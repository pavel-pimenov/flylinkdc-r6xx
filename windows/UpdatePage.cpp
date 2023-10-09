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

#include "stdafx.h"

#include "UpdatePage.h"
#include "CommandDlg.h"

#include "../client/FlyFeatures/AutoUpdate.h"

PropPage::TextItem UpdatePage::texts[] =
{
	{ IDC_AUTOUPDATE_URL_LABEL, ResourceManager::AUTOUPDATE_URL_LABEL},
	{ IDC_AUTOUPDATE_SCHEDULE_LABEL, ResourceManager::AUTOUPDATE_SCHEDULE_LABEL},
	{ IDC_AUTOUPDATE_COMPONENT_LABEL, ResourceManager::AUTOUPDATE_COMPONENT_LABEL},
	{ IDC_AUTOUPDATE_SERVER_BETA, ResourceManager::AUTOUPDATE_TO_BETA_SERV },
	{ IDC_AUTOUPDATE_USE_CUSTOM_SERVER, ResourceManager::AUTOUPDATE_USE_CUSTOM_URL },
// End of Addition.
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item UpdatePage::items[] =
{
	{ IDC_AUTOUPDATE_SERVER_BETA, SettingsManager::AUTOUPDATE_TO_BETA, PropPage::T_BOOL },
	{ IDC_AUTOUPDATE_URL, SettingsManager::AUTOUPDATE_SERVER_URL, PropPage::T_STR},
	{ IDC_AUTOUPDATE_USE_CUSTOM_SERVER, SettingsManager::AUTOUPDATE_USE_CUSTOM_URL, PropPage::T_BOOL},
// End of Addition.
	{ 0, 0, PropPage::T_END }
};

UpdatePage::ListItem UpdatePage::listItems[] =
{
	{ SettingsManager::AUTOUPDATE_SHOWUPDATEREADY, ResourceManager::AUTOUPDATE_SHOWUPDATEREADY},
	{ SettingsManager::AUTOUPDATE_FORCE_RESTART, ResourceManager::AUTOUPDATE_FORCE_RESTART},
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

UpdatePage::ListItem UpdatePage::listComponents[] =
{
	{ SettingsManager::AUTOUPDATE_EXE, ResourceManager::AUTOUPDATE_EXE},
	{ SettingsManager::AUTOUPDATE_ANTIVIRUS_DB, ResourceManager::AUTOUPDATE_ANTIVIRUS_DB},
	{ SettingsManager::AUTOUPDATE_UTILITIES, ResourceManager::AUTOUPDATE_UTILITIES},
	{ SettingsManager::AUTOUPDATE_LANG, ResourceManager::AUTOUPDATE_LANG},
	{ SettingsManager::AUTOUPDATE_PORTALBROWSER, ResourceManager::AUTOUPDATE_PORTALBROWSER},
	{ SettingsManager::AUTOUPDATE_EMOPACKS, ResourceManager::AUTOUPDATE_EMOPACKS},
	{ SettingsManager::AUTOUPDATE_SOUNDS, ResourceManager::AUTOUPDATE_SOUNDS},
	{ SettingsManager::AUTOUPDATE_ICONTHEMES, ResourceManager::AUTOUPDATE_ICONTHEMES},
	{ SettingsManager::AUTOUPDATE_COLORTHEMES, ResourceManager::AUTOUPDATE_COLORTHEMES},
	{ SettingsManager::AUTOUPDATE_DOCUMENTATION, ResourceManager::AUTOUPDATE_DOCUMENTATION},
	{ SettingsManager::AUTOUPDATE_GEOIP, ResourceManager::AUTOUPDATE_GEOIP},
	{ SettingsManager::AUTOUPDATE_CUSTOMLOCATION, ResourceManager::AUTOUPDATE_CUSTOMLOCATION},
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT UpdatePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read(*this, items, listItems, GetDlgItem(IDC_AUTOUPDATE_LIST));
	PropPage::read(*this, NULL, listComponents, GetDlgItem(IDC_AUTOUPDATE_COMPONENTS));
	
	ctrlComponents.Attach(GetDlgItem(IDC_AUTOUPDATE_LIST));
	ctrlAutoupdates.Attach(GetDlgItem(IDC_AUTOUPDATE_COMPONENTS));
	EnableAutoUpdate(BOOLSETTING(AUTOUPDATE_ENABLE));
	
	CheckUseCustomURL();
	// Do specialized reading here
	return TRUE;
}

void UpdatePage::write()
{
	PropPage::write(*this, items, listItems, GetDlgItem(IDC_AUTOUPDATE_LIST));
	PropPage::write(*this, NULL, listComponents, GetDlgItem(IDC_AUTOUPDATE_COMPONENTS));
}

void UpdatePage::EnableAutoUpdate(BOOL isEnabled)
{
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_URL_LABEL), isEnabled);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_SERVER_BETA), isEnabled);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_SCHEDULE_LABEL), isEnabled);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_LIST), isEnabled);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_COMPONENT_LABEL), isEnabled);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_COMPONENTS), isEnabled);
}

LRESULT UpdatePage::onClickedUseCustomURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CheckUseCustomURL();
	return 0;
}

void UpdatePage::CheckUseCustomURL()
{
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_URL), IsDlgButtonChecked(IDC_AUTOUPDATE_USE_CUSTOM_SERVER) == BST_CHECKED);
	::EnableWindow(GetDlgItem(IDC_AUTOUPDATE_SERVER_BETA), IsDlgButtonChecked(IDC_AUTOUPDATE_USE_CUSTOM_SERVER) != BST_CHECKED);
}