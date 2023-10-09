/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_THREAD_H
#define DCPLUSPLUS_DCPP_THREAD_H

#pragma once

#include <processthreadsapi.h>
#include <synchapi.h>

#include <boost/utility.hpp>
#include <boost/thread.hpp>


#include "Exception.h"

#define CRITICAL_SECTION_SPIN_COUNT 2000

STANDARD_EXCEPTION(ThreadException);

class BaseThread
{
	public:
		enum Priority
		{
			IDLE = THREAD_PRIORITY_IDLE,
			LOW = THREAD_PRIORITY_BELOW_NORMAL,
			NORMAL = THREAD_PRIORITY_NORMAL
			         //HIGH = THREAD_PRIORITY_ABOVE_NORMAL
		};
		static long safeExchange(volatile long& target, long value)
		{
			return InterlockedExchange(&target, value);
		}
};

class Thread : public BaseThread
{
	public:
	
		static void sleepWithSpin(unsigned int& spin)
		{
			if (spin)
			{
				--spin;
				yield();
			}
			else
			{
				sleep(1);
			}
		}
		
		static bool failStateLock(volatile long& state)
		{
			return safeExchange(state, 1) == 1;
		}
		static void waitLockStateWithSpin(volatile long& state)
		{
			unsigned int spin = CRITICAL_SECTION_SPIN_COUNT;
			while (failStateLock(state))
			{
				sleepWithSpin(spin);
			}
		}
		static void waitLockState(volatile long& p_state
#ifdef _DEBUG
		                          , bool p_is_log = false
#endif
		                         )
		{
			while (failStateLock(p_state))
			{
				yield();
#ifdef _DEBUG
				if (p_is_log)
				{
					dcdebug("waitLockState");  // TODO в дебаге сделать лог для вывода инфы о конкуренции
				}
#endif
			}
		}
		static void unlockState(volatile long& state)
		{
			dcassert(state == 1);
			safeExchange(state, 0);
		}
		
		static void waitConditionWithSpin(const volatile long& condition)
		{
			unsigned int spin = CRITICAL_SECTION_SPIN_COUNT;
			while (condition)
			{
				sleepWithSpin(spin);
			}
		}
		static void waitCondition(const volatile long& condition)
		{
			while (condition)
			{
				yield();
			}
		}
		
		
		explicit Thread() : m_threadHandle(INVALID_HANDLE_VALUE) { }
		virtual ~Thread()
		{
			close_handle();
		}
		
		void start(unsigned int p_stack_size, const char* p_name = nullptr);
		void join(const DWORD dwMilliseconds = INFINITE);
		bool is_active(int p_wait = 0) const;
		static int getThreadsCount();
		void setThreadPriority(Priority p);
		static void sleep(DWORD p_millis)
		{
			::Sleep(p_millis);
		}
		static void yield()
		{
			::Sleep(0);
		}
		
	protected:
		virtual int run() = 0;
		
	private:
	
		HANDLE m_threadHandle;
		void close_handle();
		static unsigned int  WINAPI starter(void* p);
};
class CFlyStopThread
{
	public:
		CFlyStopThread() : m_stop(false)
		{
		}
		~CFlyStopThread()
		{
			m_stop = true;
		}
	protected:
		volatile bool m_stop;
		void stopThread(bool p_is_stop = true)
		{
			m_stop = p_is_stop;
		}
		bool isShutdown() const
		{
			if (m_stop)
				return true;
			extern volatile bool g_isBeforeShutdown;
			return g_isBeforeShutdown;
		}
};

typedef boost::recursive_mutex  CriticalSection;
typedef boost::lock_guard<boost::recursive_mutex> Lock;
typedef CriticalSection FastCriticalSection;
typedef Lock FastLock;


#define CFlyLock(cs) FastLock l_lock(cs);
#define CFlyFastLock(cs) FastLock l_lock(cs);


#endif // !defined(THREAD_H)

/**
 * @file
 * $Id: Thread.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
