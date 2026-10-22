/*
 * Copyright (C) 2016-2021 FlylinkDC++ Team
 */

#if !defined(ABOUT_LOG_DLG_H)
#define ABOUT_LOG_DLG_H

#pragma once

#include "wtl_flylinkdc.h"
#include "ExListViewCtrl.h"
#include "FlyThemeDlg.h"

class AboutLogDlg : public CDialogImpl<AboutLogDlg>, public CFlyThemeDlg, CFlyRichEditLoader
{
	public:
		enum { IDD = IDD_ABOUTCMDS };
		
		AboutLogDlg()
		{
		}
		~AboutLogDlg()
		{
		}
		
		BEGIN_MSG_MAP(AboutLogDlg)
		CHAIN_MSG_MAP(CFlyThemeDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_CODE_HANDLER(EN_LINK, OnLink)
		END_MSG_MAP()
		
		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			//CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
			//ctrlThanks.FmtLines(TRUE);
			//ctrlThanks.AppendText((l_thanks).c_str(), TRUE);
			//ctrlThanks.Detach();
			
			//size_t dataRTFSize = Util::getDataFromInetSafe(programUpdateDescription, m_rtfData);
			//m_rtfData.resize(dataRTFSize);
			//WinUtil::FillRichEditFromString((HWND)GetDlgItem(IDC_THANKS), m_rtfData);   // TODO please refactoring this to use unicode.
			
			LRESULT lResult = GetDlgItem(IDC_THANKS).SendMessage(EM_GETEVENTMASK, 0, 0);
			GetDlgItem(IDC_THANKS).SendMessage(EM_SETEVENTMASK, 0, lResult | ENM_LINK);
			GetDlgItem(IDC_THANKS).SendMessage(EM_AUTOURLDETECT, TRUE, 0);
			
			return TRUE;
		}
		
		LRESULT OnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
		{
			const ENLINK* pL = (const ENLINK*)pnmh;
			if (WM_LBUTTONDOWN == pL->msg)
			{
				// the length of the range is not known in advance: the buffer must be
				// allocated for it, otherwise EM_GETTEXTRANGE writes past its end
				const LONG l_len = pL->chrg.cpMax - pL->chrg.cpMin;
				if (l_len > 0)
				{
					std::vector<TCHAR> l_buf(static_cast<size_t>(l_len) + 1, 0);
					CRichEditCtrl(pL->nmhdr.hwndFrom).GetTextRange(pL->chrg.cpMin, pL->chrg.cpMax, l_buf.data());
					WinUtil::openFile(l_buf.data());
				}
			}
			else
				bHandled = FALSE;
			return 0;
		}
	private:
		std::string m_rtfData;
};

#endif // !defined(ABOUT_LOG_DLG_H)