//////////////////////////////////////////////////////////////////////
// (c) http://jenyay.net/Programming/ProgressDlg

#if  !defined(_CPROGRESSBAR_H)
#define _CPROGRESSBAR_H

#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlcrack.h>
#include <atlctrls.h>

#include <process.h>

// ����� ����� ��� ������ ������ ��, ��� ����� ��������
class IProgress
{
	public:
	
		// ���������� ������� ���������
		virtual void SetProgressRange(int minval, int maxval, int currval) = 0;
		
		// ���������� ������� �������� ����������
		virtual void SetProgress(int val) = 0;
};

template<class T> struct ProgressParam
{
	T* pParams;
	IProgress* pProgress;
};



template<class T, int idd, int time> class CProgressDlg :
	public CDialogImpl<CProgressDlg<T, idd, time> >,
	public IProgress
{
	private:
		BEGIN_MSG_MAP_EX(CAboutDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_TIMER(OnTimer)
		END_MSG_MAP()
		
		// ��� ������������� �������� �����
		LRESULT OnInitDialog(HWND hwnd, LPARAM lParam)
		{
			m_Progress = GetDlgItem(IDC_PROGRESS);
			
			unsigned ThreadId;
			m_FuncParams.pParams = m_pParams;
			m_FuncParams.pProgress = this;
			
			SetProgressRange(0, 100, 0);
			
			SetWindowText(m_Capture);
			
			m_FirstTime = GetTickCount();
			m_Dt = 0;
			
			m_hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void *))m_Func,
			                                   ((void*)(&m_FuncParams)), 0, &ThreadId);
			                                   
			if (!m_hThread)
			{
				MessageBox(_T("������ �������� ������"), _T("������"), MB_OK);
				return 0;
			}
			
			if (SetTimer(m_TimerId, 100) == NULL)
			{
				MessageBox(_T("������ ������������� �������"), _T("������"), MB_OK);
				return 0;
			}
			
			return 0;
		}
		
		// �� ������� ���������, ���������� �� �����
		void OnTimer(UINT TimerId)
		{
			if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
			{
				// ���� ����� �� ��������, ��������� ������
				
				KillTimer(m_TimerId);
				CloseHandle(m_hThread);
				EndDialog(0);
			}
		}
		
		
		int m_TimerId;
		
		HANDLE m_hThread;
		LPTHREAD_START_ROUTINE m_Func;
		T *m_pParams;
		LPCTSTR m_Capture;
		CProgressBarCtrl m_Progress;
		
		// ������ ����� �� ������� ������
		DWORD m_FirstTime;
		
		// ����� �� ���� ���
		DWORD m_Dt;
		DWORD m_MaxTime;
		
		// ������ ��� �������� � ������� ������, � �� �� ���������, ������� void*
		ProgressParam<T> m_FuncParams;
	public:
		enum { IDD = idd };
		enum { IDTIME = time};
		
		CProgressDlg(LPTHREAD_START_ROUTINE func, T* params, LPCTSTR capture): m_Func(func),
			m_TimerId(101), m_hThread(0), m_pParams(params), m_Capture(capture),
			m_FirstTime(0)
		{
		}
		
		virtual ~CProgressDlg()
		{
		}
		
		void Start()
		{
			DoModal();
		}
		
		virtual void SetProgressRange(int minval, int maxval, int currval)
		{
			m_Progress.SetRange(minval, maxval);
			m_Progress.SetPos(currval);
		}
		
		virtual void SetProgress(int val)
		{
			m_Progress.SetPos(val);
			
			if (!m_Dt)
			{
				m_Dt = GetTickCount() - m_FirstTime;
			}
			
			
			PBRANGE range;
			m_Progress.GetRange(&range);
			
			int progress = abs(val - range.iLow);
			
			if (progress)
			{
				const int maxcount = 256;
				const int l_time = m_Dt * (abs(range.iHigh - range.iLow) - progress) / 1000;
				
				TCHAR text[maxcount];
				swprintf(text, maxcount, _T("%d ���"), l_time);
				
				SetDlgItemText(IDTIME, text);
			}
		}
};

#endif //
