 /*
 *  MHooks -- hooks MouseWare X-buttons and IntelliPoint tilt wheel and
 *  middle button messages
 *  Copyright (C) 2004  [MAD]ApxuTekTop
 *
 *  Used X-buttons messages data from MouseWare hook by [2WP]BorisU
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: mhooks.c,v 1.5 2004/03/14 18:11:56 rzhe Exp $
 */

#include <windows.h>
#include <tchar.h>

#pragma hdrstop


#include "mhooks.h"


#define MHOOKS_FMNAME			_T("MHooksFileMapping")

#define MW_WINDOW_CLASSNAME		_T("Logitech GetMessage Hook")
#define	MW_WINDOW_CAPTION		_T("Logitech GetMessage Hook")
#define MW_MESSAGE				(WM_USER + 1221)
#define MW_BUTTONS_MASK			0xFFF8
#define MW_XBUTTON1_MASK		0x0008
#define MW_XBUTTON2_MASK		0x0010
#define MW_XBUTTON3_MASK		0x0020
#define MW_XBUTTON4_MASK		0x0040
#define MW_XBUTTON5_MASK		0x0080

#define IP_WINDOW_CLASSNAME		_T("CommandServerWindow")
#define	IP_WINDOW_CAPTION		_T("mouse")
#define IP_MESSAGE				(WM_USER + 300)
#define IP_TWHEEL_WPARAM		0x01FD044D
#define IP_TWHEEL_LPARAM_MASK	0xFFFFFFF4
#define IP_MBUTTON_WPARAM_MASK	0x00050000
#define IP_MBUTTONUP_LPARAM		0x00000000


typedef struct tagHookData
{
	HINSTANCE	m_hInstance;

	int			m_nType;
	HWND		m_hSinkWnd;
	UINT		m_uSinkMsg;
	HWND		m_hHookedWnd;
	HHOOK		m_hHook;

	LONG		m_nMWOldButtons;
} HookData, *PHookData;


static HINSTANCE s_hInstance = NULL;
static HANDLE s_hMapping = NULL;
static PHookData s_pHookData = NULL;


LRESULT CALLBACK MW_GetMsgProc(int _nCode, WPARAM _wParam, LPARAM _lParam)
{
	if (_nCode >= 0)
	{
		PMSG pMsg = (PMSG) _lParam;
		if (pMsg->hwnd == s_pHookData->m_hHookedWnd &&
			pMsg->message == MW_MESSAGE)
		{
			LONG nButtons, nChangedButtons;
			WPARAM wp;

			nButtons = pMsg->lParam & MW_BUTTONS_MASK;
			nChangedButtons = nButtons ^ s_pHookData->m_nMWOldButtons;

			wp = 0;
			if (nChangedButtons & MW_XBUTTON1_MASK)
			{
				if (nButtons & MW_XBUTTON1_MASK)
					wp |= (WPARAM) MHOOKS_MW_XBUTTON1_DOWN;
				else
					wp |= (WPARAM) MHOOKS_MW_XBUTTON1_UP;
			}
			if (nChangedButtons & MW_XBUTTON2_MASK)
			{
				if (nButtons & MW_XBUTTON2_MASK)
					wp |= (WPARAM) MHOOKS_MW_XBUTTON2_DOWN;
				else
					wp |= (WPARAM) MHOOKS_MW_XBUTTON2_UP;
			}
			if (nChangedButtons & MW_XBUTTON3_MASK)
			{
				if (nButtons & MW_XBUTTON3_MASK)
					wp |= (WPARAM) MHOOKS_MW_XBUTTON3_DOWN;
				else
					wp |= (WPARAM) MHOOKS_MW_XBUTTON3_UP;
			}
			if (nChangedButtons & MW_XBUTTON4_MASK)
			{
				if (nButtons & MW_XBUTTON4_MASK)
					wp |= (WPARAM) MHOOKS_MW_XBUTTON4_DOWN;
				else
					wp |= (WPARAM) MHOOKS_MW_XBUTTON4_UP;
			}
			if (nChangedButtons & MW_XBUTTON5_MASK)
			{
				if (nButtons & MW_XBUTTON5_MASK)
					wp |= (WPARAM) MHOOKS_MW_XBUTTON5_DOWN;
				else
					wp |= (WPARAM) MHOOKS_MW_XBUTTON5_UP;
			}

			PostMessage(s_pHookData->m_hSinkWnd, s_pHookData->m_uSinkMsg,
				wp, pMsg->lParam);

			s_pHookData->m_nMWOldButtons = nButtons;

			pMsg->message = WM_NULL;
		}
	}

	return CallNextHookEx(s_pHookData->m_hHook, _nCode, _wParam, _lParam);
}

LRESULT CALLBACK IP_GetMsgProc(int _nCode, WPARAM _wParam, LPARAM _lParam)
{
	if (_nCode >= 0)
	{
		PMSG pMsg = (PMSG) _lParam;
		if (pMsg->hwnd == s_pHookData->m_hHookedWnd &&
			pMsg->message == IP_MESSAGE)
		do
		{
			WPARAM wp;

			if (pMsg->wParam == IP_TWHEEL_WPARAM)
			{
				LPARAM lBits = pMsg->lParam & IP_TWHEEL_LPARAM_MASK;

				if (lBits == IP_TWHEEL_LPARAM_MASK)
					wp = (WPARAM) MHOOKS_IP_TILTMOVE_LEFT;
				else if (lBits == 0)
					wp = (WPARAM) MHOOKS_IP_TILTMOVE_RIGHT;
				else
					break;
			}
			else if ((pMsg->wParam & IP_MBUTTON_WPARAM_MASK) ==
				IP_MBUTTON_WPARAM_MASK && pMsg->lParam == IP_MBUTTONUP_LPARAM)
			{
				wp = (WPARAM) MHOOKS_IP_MBUTTON_UP;
			}
			else
				break;

			PostMessage(s_pHookData->m_hSinkWnd, s_pHookData->m_uSinkMsg,
				wp, pMsg->lParam);

			pMsg->message = WM_NULL;
		}
		while (0);
	}

	return CallNextHookEx(s_pHookData->m_hHook, _nCode, _wParam, _lParam);
}

BOOL MHOOKS_API MHooks_SetHook(int _nType, HWND _hSinkWnd, UINT _uSinkMsg)
{
	LPCTSTR pszClassName;
	LPCTSTR pszCaption;
	HOOKPROC pHookProc;

	if (s_pHookData->m_nType != MHOOKS_HOOKTYPE_NONE)
		return FALSE;

	s_pHookData->m_hInstance = s_hInstance;
	s_pHookData->m_hSinkWnd = _hSinkWnd;
	s_pHookData->m_uSinkMsg = _uSinkMsg;

	switch (_nType)
	{
	case MHOOKS_HOOKTYPE_MW:
		pszClassName = MW_WINDOW_CLASSNAME;
		pszCaption = MW_WINDOW_CAPTION;
		pHookProc = (HOOKPROC) MW_GetMsgProc;

		s_pHookData->m_nMWOldButtons = 0;

		break;

	case MHOOKS_HOOKTYPE_IP:
		pszClassName = IP_WINDOW_CLASSNAME;
		pszCaption = IP_WINDOW_CAPTION;
		pHookProc = (HOOKPROC) IP_GetMsgProc;

		break;

	default:
		return FALSE;
	}

	s_pHookData->m_hHookedWnd = FindWindow(pszClassName, pszCaption);
	if (!s_pHookData->m_hHookedWnd)
		return FALSE;

	s_pHookData->m_nType = _nType;

	s_pHookData->m_hHook = SetWindowsHookEx(WH_GETMESSAGE, pHookProc,
		s_hInstance, 0);
	if (!s_pHookData->m_hHook)
	{
		s_pHookData->m_nType = MHOOKS_HOOKTYPE_NONE;

		return FALSE;
	}

	return TRUE;
}

BOOL MHOOKS_API MHooks_RemoveHook()
{
	if (s_pHookData->m_nType == MHOOKS_HOOKTYPE_NONE)
		return FALSE;

	UnhookWindowsHookEx(s_pHookData->m_hHook);

	s_pHookData->m_nType = MHOOKS_HOOKTYPE_NONE;

	return TRUE;
}

int MHOOKS_API MHooks_GetType()
{
	return s_pHookData->m_nType;
}

BOOL WINAPI DllMain(HANDLE _hInstDLL, DWORD _dwReason, LPVOID _pReserved)
{
	BOOL bNewMapping = FALSE;

	switch (_dwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_hInstance = _hInstDLL;

		s_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, sizeof(HookData), MHOOKS_FMNAME);
		if (!s_hMapping)
			return FALSE;
		if (GetLastError() != ERROR_ALREADY_EXISTS)
			bNewMapping = TRUE;

		s_pHookData = (PHookData) MapViewOfFile(s_hMapping,
			FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (!s_pHookData)
			return FALSE;
		if (bNewMapping)
			ZeroMemory(s_pHookData, sizeof(HookData));

		break;

	case DLL_PROCESS_DETACH:
		if (s_pHookData->m_hInstance == s_hInstance)
			MHooks_RemoveHook();

		UnmapViewOfFile((LPCVOID) s_pHookData);
		CloseHandle(s_hMapping);

		break;
	}

	return TRUE;
}
