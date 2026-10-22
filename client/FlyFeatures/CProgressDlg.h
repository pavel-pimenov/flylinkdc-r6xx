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
#include "../../windows/FlyThemeDlg.h"

// Чтобы поток мог делать только то, что здесь разрешим
class IProgress
{
	public:
	
		// Установить пределы прогресса
		virtual void SetProgressRange(int minval, int maxval, int currval) = 0;
		
		// Установить текущий прогресс выполнения
		virtual void SetProgress(int val) = 0;
};

template<class T> struct ProgressParam
{
	T* pParams;
	IProgress* pProgress;
};



template<class T, int idd, int time> class CProgressDlg :
	public CDialogImpl<CProgressDlg<T, idd, time> >,
	public CFlyThemeDlg,
	public IProgress
{
	private:
		// The worker thread must not use the dialog after it has been destroyed.
		// The progress interface is therefore a separate object on the heap: the
		// dialog detaches itself from it in the destructor.
		class CProgressProxy : public IProgress
		{
			public:
				explicit CProgressProxy(IProgress* p_target) : m_target(p_target)
				{
					::InitializeCriticalSection(&m_cs);
				}
				~CProgressProxy()
				{
					::DeleteCriticalSection(&m_cs);
				}
				void SetProgressRange(int minval, int maxval, int currval) override
				{
					::EnterCriticalSection(&m_cs);
					IProgress* l_target = m_target;
					if (l_target)
						l_target->SetProgressRange(minval, maxval, currval);
					::LeaveCriticalSection(&m_cs);
				}
				void SetProgress(int val) override
				{
					::EnterCriticalSection(&m_cs);
					IProgress* l_target = m_target;
					if (l_target)
						l_target->SetProgress(val);
					::LeaveCriticalSection(&m_cs);
				}
				void detach()
				{
					::EnterCriticalSection(&m_cs);
					m_target = nullptr;
					::LeaveCriticalSection(&m_cs);
				}
			private:
				CRITICAL_SECTION m_cs;
				IProgress* m_target;
		};

		BEGIN_MSG_MAP_EX(CAboutDlg)
		CHAIN_MSG_MAP(CFlyThemeDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_TIMER(OnTimer)
		END_MSG_MAP()
		
		// При инициализации запустим поток
		LRESULT OnInitDialog(HWND hwnd, LPARAM lParam)
		{
			m_Progress = GetDlgItem(IDC_PROGRESS);
			
			unsigned ThreadId = 0;
			m_progress_proxy = new CProgressProxy(this);
			m_FuncParams = new ProgressParam<T>();
			m_FuncParams->pParams = m_pParams;
			m_FuncParams->pProgress = m_progress_proxy;
			
			SetProgressRange(0, 100, 0);
			
			SetWindowText(m_Capture);
			
			m_FirstTime = GetTickCount();
			m_Dt = 0;
			
			m_hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void *))m_Func,
			                                   ((void*)(m_FuncParams)), 0, &ThreadId);
			                                   
			if (!m_hThread)
			{
				MessageBox(_T("Ошибка создания потока"), _T("Ошибка"), MB_OK);
				return 0;
			}
			
			if (SetTimer(m_TimerId, 100) == NULL)
			{
				MessageBox(_T("Ошибка инициализации таймера"), _T("Ошибка"), MB_OK);
				return 0;
			}
			
			return 0;
		}
		
		// По таймеру проверяем, завершился ли поток
		void OnTimer(UINT TimerId)
		{
			if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
			{
				// Если поток не работает, закрываем диалог
				
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
		
		// Первое время до запуска потока
		DWORD m_FirstTime;
		
		// Время на один шаг
		DWORD m_Dt;
		DWORD m_MaxTime;
		
		// Именно это передаем в функцию потока, а не те аргументы, который void*
		// the parameters are allocated on the heap: the worker thread uses them and
		// they must stay alive until the thread has finished
		ProgressParam<T>* m_FuncParams = nullptr;
		CProgressProxy* m_progress_proxy = nullptr;
	public:
		enum { IDD = idd };
		enum { IDTIME = time};
		
		CProgressDlg(LPTHREAD_START_ROUTINE func, T* params, LPCTSTR capture): m_Func(func),
			m_TimerId(101), m_hThread(0), m_pParams(params), m_Capture(capture),
			m_FirstTime(0), m_Dt(0), m_MaxTime(0)
		{
		}
		
		virtual ~CProgressDlg()
		{
			// the worker thread must not use the dialog any more: the progress proxy
			// detaches the dialog and the thread is awaited before the members of the
			// dialog are destroyed
			if (m_progress_proxy)
				m_progress_proxy->detach();
			if (m_hThread)
			{
				::WaitForSingleObject(m_hThread, 60000);
				::CloseHandle(m_hThread);
				m_hThread = NULL;
			}
			if (m_progress_proxy)
			{
				delete m_progress_proxy;
				m_progress_proxy = NULL;
			}
			delete m_FuncParams;
			m_FuncParams = nullptr;
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
				swprintf(text, maxcount, _T("%d сек"), l_time);
				
				SetDlgItemText(IDTIME, text);
			}
		}
};

#endif //
