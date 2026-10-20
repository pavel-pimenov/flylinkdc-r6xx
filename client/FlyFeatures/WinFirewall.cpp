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

#include "stdinc.h"
#include "WinFirewall.h"
#include <comutil.h>
#include <comdef.h>

namespace
{
	// Releases the COM interface when it goes out of scope - without it every query
	// of the firewall leaked 2..5 COM references
	template<class T>
	class CFlyComPtr
	{
		public:
			CFlyComPtr() : m_ptr(nullptr)
			{
			}
			~CFlyComPtr()
			{
				reset();
			}
			void reset()
			{
				if (m_ptr)
				{
					m_ptr->Release();
					m_ptr = nullptr;
				}
			}
			// use it as the out parameter of the COM getters
			T** getAddr()
			{
				reset();
				return &m_ptr;
			}
			T* get() const
			{
				return m_ptr;
			}
			T* operator->() const
			{
				return m_ptr;
			}
			operator T*() const
			{
				return m_ptr;
			}
		private:
			CFlyComPtr(const CFlyComPtr&);
			CFlyComPtr& operator=(const CFlyComPtr&);
			T* m_ptr;
	};
}

// TODO - заменить на chromium\home\src_tarball\tarball\chromium\src\third_party\libjingle\source\talk\base
// winfirewall.cc
// winfirewall.h
BOOL WinFirewall::IsWindowsFirewallAppEnabled(const TCHAR* in_pAuthorizedFilename)
{
	BOOL bRet = FALSE;
	if (!in_pAuthorizedFilename) throw E_INVALIDARG;
	try
	{
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		CFlyComPtr<INetFwAuthorizedApplications> l_fwAuthorizedApplicationsPtr;
		CFlyComPtr<INetFwAuthorizedApplication> l_fwAuthorizedApplicationPtr;
		HRESULT hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		hr = l_fwProfilePtr->get_AuthorizedApplications(l_fwAuthorizedApplicationsPtr.getAddr());
		if (FAILED(hr)) throw E_UNEXPECTED;
		hr = l_fwAuthorizedApplicationsPtr->Item(_bstr_t(in_pAuthorizedFilename), l_fwAuthorizedApplicationPtr.getAddr());
		if (SUCCEEDED(hr))
		{
			VARIANT_BOOL fwEnabled;
			hr = l_fwAuthorizedApplicationPtr->get_Enabled(&fwEnabled);
			if (FAILED(hr)) throw E_UNEXPECTED;
			bRet = (fwEnabled != VARIANT_FALSE) ? TRUE : FALSE;
		}
		else
		{
			// No such Application (Disabled)
		}
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
	return bRet;
}

void WinFirewall::WindowFirewallSetAppAuthorization(const TCHAR* in_pAuthorizedDisplayname, const TCHAR* in_pAuthorizedFilename, bool bEnableService /* = true */)
{
	if (!in_pAuthorizedDisplayname) throw E_INVALIDARG;
	if (!in_pAuthorizedFilename) throw E_INVALIDARG;
	try
	{
		BOOL bAppStatus = IsWindowsFirewallAppEnabled(in_pAuthorizedFilename);
		if (bAppStatus && bEnableService) // Already enabled
			return;
		if (!bAppStatus && !bEnableService) // Already disabled
			return;
			
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		CFlyComPtr<INetFwAuthorizedApplications> l_fwAuthorizedApplicationsPtr;
		CFlyComPtr<INetFwAuthorizedApplication> l_fwAuthorizedApplicationPtr;
		HRESULT hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		hr = l_fwProfilePtr->get_AuthorizedApplications(l_fwAuthorizedApplicationsPtr.getAddr());
		if (FAILED(hr)) throw E_UNEXPECTED;
		//CLSID clsID;
		//hr = CLSIDFromProgID(OLESTR("HNetCfg.FwAuthorizedApplication"),&clsID);
		//if (FAILED(hr)) throw E_UNEXPECTED;
		//hr = pFwAuthorizedApplicationPtr.CreateInstance(clsID);
		hr = CoCreateInstance(
		         __uuidof(NetFwAuthorizedApplication),
		         NULL,
		         CLSCTX_INPROC_SERVER,
		         __uuidof(INetFwAuthorizedApplication),
		         reinterpret_cast<void**>(l_fwAuthorizedApplicationPtr.getAddr()));
		if (FAILED(hr)) throw E_UNEXPECTED;
		l_fwAuthorizedApplicationPtr->put_Name(_bstr_t(in_pAuthorizedDisplayname));
		l_fwAuthorizedApplicationPtr->put_ProcessImageFileName(_bstr_t(in_pAuthorizedFilename));
		if (bEnableService)
			l_fwAuthorizedApplicationPtr->put_Enabled(VARIANT_TRUE);
		else
			l_fwAuthorizedApplicationPtr->put_Enabled(VARIANT_FALSE);
			
		hr = l_fwAuthorizedApplicationsPtr->Add(l_fwAuthorizedApplicationPtr.get());
		if (FAILED(hr)) throw E_UNEXPECTED;
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
}

BOOL WinFirewall::IsWindowsFirewallIsOn()
{
	HRESULT hr = S_OK;
	BOOL bRet = FALSE;
	VARIANT_BOOL fwEnabled;
	try
	{
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		hr = l_fwProfilePtr->get_FirewallEnabled(&fwEnabled);
		if (FAILED(hr)) throw E_UNEXPECTED;
		if (fwEnabled != VARIANT_FALSE)
			bRet = TRUE;
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
	return bRet;
}


BOOL WinFirewall::IsWindowsFirewallPortEnabled(long in_lPort, NET_FW_IP_PROTOCOL in_protocol)
{
	BOOL bRet = FALSE;
	try
	{
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		HRESULT hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		CFlyComPtr<INetFwOpenPort> l_fwOpenPort;
		CFlyComPtr<INetFwOpenPorts> l_fwOpenPorts;
		// the result of the call must be checked - otherwise a failed call leaves
		// the pointer uninitialized and it is dereferenced below
		hr = l_fwProfilePtr->get_GloballyOpenPorts(l_fwOpenPorts.getAddr());
		if (FAILED(hr)) throw E_UNEXPECTED;
		if (!l_fwOpenPorts.get()) throw E_UNEXPECTED;
		hr = l_fwOpenPorts->Item(in_lPort, in_protocol, l_fwOpenPort.getAddr());
		if (SUCCEEDED(hr))
		{
			VARIANT_BOOL fwEnabled;
			hr = l_fwOpenPort->get_Enabled(&fwEnabled);
			if (FAILED(hr)) throw hr;
			bRet = (fwEnabled != VARIANT_FALSE) ? TRUE : FALSE;
		}
		else
		{
			// No port in Firewall, so it disabled.
		}
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
	return bRet;
}

#ifdef FLYLINKDC_USE_DEAD_CODE
void WinFirewall::TurnWindowsFirewall(bool bTurnOn /*= true*/)
{
	try
	{
		BOOL bFireWallStatus = IsWindowsFirewallIsOn();
		if (bTurnOn && bFireWallStatus) // Already Turn on
			return;
		if (!bTurnOn && !bFireWallStatus) // Alreadt turn off
			return;
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		HRESULT hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		l_fwProfilePtr->put_FirewallEnabled(bTurnOn ? VARIANT_TRUE : VARIANT_FALSE);
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
}

void WinFirewall::WindowsFirewallPortAdd(TCHAR* in_sportName, long in_portNumber, bool bEnable /*= true*/, NET_FW_IP_PROTOCOL in_protocol /*= NetFwTypeLib::NET_FW_IP_PROTOCOL_TCP*/)
{
	if (!in_sportName) throw E_INVALIDARG;
	if (!in_portNumber) throw E_INVALIDARG;
	try
	{
		BOOL bIsPortEnable = IsWindowsFirewallPortEnabled(in_portNumber, in_protocol);
		if (bEnable && bIsPortEnable)
			return;
		if (!bEnable && !bIsPortEnable)
			return;
		CFlyComPtr<INetFwProfile> l_fwProfilePtr;
		HRESULT hr = GetFwProfile(l_fwProfilePtr.getAddr());
		if (FAILED(hr)) throw hr;
		CFlyComPtr<INetFwOpenPort> l_fwOpenPort;
		CFlyComPtr<INetFwOpenPorts> l_fwOpenPorts;
		hr = l_fwProfilePtr->get_GloballyOpenPorts(l_fwOpenPorts.getAddr());
		if (FAILED(hr)) throw E_UNEXPECTED;
		//CLSID clsID;
		//hr = CLSIDFromProgID(OLESTR("HNetCfg.FwOpenPort"),&clsID);
		//if (FAILED(hr)) throw E_UNEXPECTED;
		//hr = fwOpenPort.CreateInstance(clsID);
		hr = CoCreateInstance(
		         __uuidof(NetFwOpenPort),
		         NULL,
		         CLSCTX_INPROC_SERVER,
		         __uuidof(INetFwOpenPort),
		         reinterpret_cast<void**>(l_fwOpenPort.getAddr()));
		if (FAILED(hr)) throw E_UNEXPECTED;
		l_fwOpenPort->put_Port(in_portNumber);
		l_fwOpenPort->put_Protocol(in_protocol);
		l_fwOpenPort->put_Name(_bstr_t(in_sportName));
		l_fwOpenPort->put_Enabled(bEnable);
		hr = l_fwOpenPorts->Add(l_fwOpenPort.get());
		if (FAILED(hr)) throw E_UNEXPECTED;
	}
	catch (const HRESULT& getHResult)
	{
		switch (getHResult)
		{
			case E_NOINTERFACE:  // No such Interface in system
				break;
			default:
				throw;
		}
	}
}
#endif

HRESULT WinFirewall::GetFwProfile(INetFwProfile** out_fwProfile)
{
	HRESULT hRet = S_OK;
	try
	{
		CFlyComPtr<INetFwMgr> l_fwMgrPtr;
		CFlyComPtr<INetFwPolicy> l_fwPolicyPtr;
		// CLSID clsID;
		//HRESULT hr= CLSIDFromProgID(OLESTR("HNetCfg.FwMgr"), &clsID);
		//if(FAILED(hr)) throw E_NOINTERFACE;
		// hr = pFwMgrPtr.CreateInstance(clsID);
		HRESULT hr = CoCreateInstance(
		                 __uuidof(NetFwMgr),
		                 NULL,
		                 CLSCTX_INPROC_SERVER,
		                 __uuidof(INetFwMgr),
		                 reinterpret_cast<void**>(l_fwMgrPtr.getAddr())
		             );
		if (FAILED(hr)) throw hr;
		hr = l_fwMgrPtr->get_LocalPolicy(l_fwPolicyPtr.getAddr());
		if (FAILED(hr)) throw E_UNEXPECTED;
		hr = l_fwPolicyPtr->get_CurrentProfile(out_fwProfile);
		if (FAILED(hr)) throw E_UNEXPECTED;
	}
	catch (const HRESULT& getHResult)
	{
		hRet = getHResult;
	}
	catch (...)
	{
		hRet = E_UNEXPECTED;
	}
	return hRet;
}
