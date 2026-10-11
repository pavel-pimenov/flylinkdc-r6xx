#ifndef UserListColours_H
#define UserListColours_H

#pragma once


#include "PropPage.h"

class UserListColours : public CPropertyPage<IDD_USERLIST_COLOURS_PAGE>, public PropPage
{
	public:
	
		explicit UserListColours() : PropPage(TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES) + _T('\\') + TSTRING(SETTINGS_USER_LIST))
		{
			SetTitle(m_title.c_str());
			m_psp.dwFlags |= PSP_RTLREADING;
		}
		
		~UserListColours()
		{
		}
		
		BEGIN_MSG_MAP(UserListColours)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_CHANGE_COLOR, BN_CLICKED, onChangeColour)
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)
		COMMAND_ID_HANDLER(IDC_USERS_LINK, onLinkClick)
		CHAIN_MSG_MAP(PropPage)
		REFLECT_NOTIFICATIONS()
		END_MSG_MAP()
		
		LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
		LRESULT onChangeColour(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		LRESULT onLinkClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		
		// Common PropPage interface
		PROPSHEETPAGE *getPSP()
		{
			return (PROPSHEETPAGE *) * this;
		}
		void write();
		void cancel()
		{
			cancel_check();
		}
		CRichEditCtrl n_Preview;
	private:
		void BrowseForPic(int DLGITEM);
		
		void refreshPreview();
		
		CListBox n_lsbList;
		int normalColour = 0;
		int favoriteColour = 0;
		int favEnemyColour = 0;
		int reservedSlotColour = 0;
		int ignoredColour = 0;
		int fastColour = 0;
		int serverColour = 0;
		int pasiveColour = 0;
		int opColour = 0;
		int fullCheckedColour = 0;
		int badClientColour = 0;
		int badFilelistColour = 0;
		
		ExCImage m_png_users;
		CFlyHyperLink m_hlink_users;
	protected:
		static Item items[];
		static TextItem texts[];
		
};

#endif //UserListColours_H

/**
 * @file
 * $Id: UserlistColours.h 499 2010-05-16 11:01:37Z bigmuscle $
 */
