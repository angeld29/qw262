/*
 *  QW262
 *  Copyright (C) 2004  [MAD]ApxuTekTop based on code by [2WP]BorisU
 *  Original idea of MouseWare hook by [EZH]FAN
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
 *  $Id: cl_mhooks.c,v 1.5 2004/03/14 18:12:32 rzhe Exp $
 */

#include <windows.h>

#include "quakedef.h"
#define MHOOKS_MANUAL_LOAD
#include "../mhooks/mhooks.h"


extern HWND mainwindow;
extern int MW_buttons_mapping[5];


#define MHOOKSDLLFNAME	"mhooks.dll"


qboolean OnCvarMHookChange(cvar_t *_pVar, const char *_pValue);
qboolean OnCvarMWHookChange(cvar_t *_pVar, const char *_pValue);

cvar_t m_hook = { "m_hook", "0", 0, OnCvarMHookChange };
cvar_t m_mwhook = { "m_mwhook", "0", 0, OnCvarMWHookChange };


UINT MH_SinkMsg = 0;


static HINSTANCE s_hMHooksDLL;
static const char *s_ppszHookArgStrings[] =
{
	"0",
	"mw",
	"ip"
};
static const char *s_ppszDriverNames[] = 
{
	"None",
	"MouseWare",
	"IntelliPoint"
};


static int ArgStringToType(const char *_pszArgString)
{
	int i;
	for (i = MHOOKS_HOOKTYPE_NONE; i <= MHOOKS_HOOKTYPE_LAST; i++)
		if (!Q_stricmp(_pszArgString, s_ppszHookArgStrings[i]))
			return i;

	return -1;
}

static inline int HookType()
{
	return MHooks_GetType != NULL ? MHooks_GetType() : MHOOKS_HOOKTYPE_NONE;
}

static void SetHook(int _nType)
{
	if (_nType == MHOOKS_HOOKTYPE_NONE || HookType() != MHOOKS_HOOKTYPE_NONE)
		return;

	s_hMHooksDLL = LoadLibrary(MHOOKSDLLFNAME);
	if (!s_hMHooksDLL)
	{
		Con_Printf("Error: Can't load %s!\n", MHOOKSDLLFNAME);

		return;
	}

	MHooks_SetHook = (MHooks_SetHook_t) GetProcAddress(s_hMHooksDLL,
		"MHooks_SetHook");
	MHooks_RemoveHook = (MHooks_RemoveHook_t) GetProcAddress(s_hMHooksDLL,
		"MHooks_RemoveHook");
	MHooks_GetType = (MHooks_RemoveHook_t) GetProcAddress(s_hMHooksDLL,
		"MHooks_GetType");

	if (!MHooks_SetHook || !MHooks_RemoveHook || !MHooks_GetType)
	{
		FreeLibrary(s_hMHooksDLL);

		Con_Printf("Error: Bad MHooks DLL. %s hook is not set.\n",
			s_ppszDriverNames[_nType]);
	}

	MHooks_SetHook(_nType, mainwindow, MH_SinkMsg);

	Con_Printf(HookType() == _nType ?	"%s hook set\n" :
		"Failed to set %s hook\n", s_ppszDriverNames[_nType]);
}

static void RemoveHook()
{
	int nType = HookType();
	if (nType != MHOOKS_HOOKTYPE_NONE)
	{
		MHooks_RemoveHook();
		FreeLibrary(s_hMHooksDLL);

		MHooks_SetHook = NULL;
		MHooks_RemoveHook = NULL;
		MHooks_GetType = NULL;

		Con_Printf("%s hook removed\n", s_ppszDriverNames[nType]);
	}
}

static qboolean OnHookChange(const char *_pValue)
{
	int nType = HookType();
	int nNewType = ArgStringToType(_pValue);

	if (nNewType < MHOOKS_HOOKTYPE_NONE || nNewType > MHOOKS_HOOKTYPE_LAST)
	{
		Con_Printf("m_hook: wrong parameter\n");

		return true;
	}

	if (nNewType == MHOOKS_HOOKTYPE_NONE && nType != MHOOKS_HOOKTYPE_NONE)
	{
		RemoveHook();

		if (HookType() != MHOOKS_HOOKTYPE_NONE)
			return true;
	}
	else if (nType == MHOOKS_HOOKTYPE_NONE)
	{
		SetHook(nNewType);

		if (HookType() != nNewType)
			return true;
	}
	else if (nNewType != nType)
	{
		Con_Printf("%s hook is set, use m_hook 0 to remove it first\n",
			s_ppszDriverNames[nType]);

		return true;
	}

	return false;
}

qboolean OnCvarMHookChange(cvar_t *_pVar, const char *_pValue)
{
	return OnHookChange(_pValue);
}

qboolean OnCvarMWHookChange(cvar_t *_pVar, const char *_pValue)
{
	int iValue = Q_atoi(_pValue);

	if (!iValue && HookType() != MHOOKS_HOOKTYPE_MW)
		return false;

	return OnHookChange(iValue ? "mw" : "0");
}

void MH_Init()
{
	MH_SinkMsg = RegisterWindowMessage("MHooksSinkMsg");

	Cvar_RegisterVariable(&m_hook);
	Cvar_RegisterVariable(&m_mwhook);
}

void MH_Shutdown()
{
	RemoveHook();
}

void MH_OnHookEvent(UINT _nEvents)
{
	switch (HookType())
	{
	case MHOOKS_HOOKTYPE_MW:
		if (_nEvents & MHOOKS_MW_XBUTTON1_DOWN)
			Key_Event(MW_buttons_mapping[0], true);
		if (_nEvents & MHOOKS_MW_XBUTTON1_UP)
			Key_Event(MW_buttons_mapping[0], false);
		if (_nEvents & MHOOKS_MW_XBUTTON2_DOWN)
			Key_Event(MW_buttons_mapping[1], true);
		if (_nEvents & MHOOKS_MW_XBUTTON2_UP)
			Key_Event(MW_buttons_mapping[1], false);
		if (_nEvents & MHOOKS_MW_XBUTTON3_DOWN)
			Key_Event(MW_buttons_mapping[2], true);
		if (_nEvents & MHOOKS_MW_XBUTTON3_UP)
			Key_Event(MW_buttons_mapping[2], false);
		if (_nEvents & MHOOKS_MW_XBUTTON4_DOWN)
			Key_Event(MW_buttons_mapping[3], true);
		if (_nEvents & MHOOKS_MW_XBUTTON4_UP)
			Key_Event(MW_buttons_mapping[3], false);
		if (_nEvents & MHOOKS_MW_XBUTTON5_DOWN)
			Key_Event(MW_buttons_mapping[4], true);
		if (_nEvents & MHOOKS_MW_XBUTTON5_UP)
			Key_Event(MW_buttons_mapping[4], false);

		break;

	case MHOOKS_HOOKTYPE_IP:
		if (_nEvents & MHOOKS_IP_TILTMOVE_LEFT)
		{
			Key_Event(K_TWHEELLEFT, true);
			Key_Event(K_TWHEELLEFT, false);
		}
		else if (_nEvents & MHOOKS_IP_TILTMOVE_RIGHT)
		{
			Key_Event(K_TWHEELRIGHT, true);
			Key_Event(K_TWHEELRIGHT, false);
		}
		else if (_nEvents & MHOOKS_IP_MBUTTON_UP)
		{
			Key_Event(K_MOUSE3, true);
			Key_Event(K_MOUSE3, false);
		}

		break;
	}
}
