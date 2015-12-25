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
 *  $Id: mhooks.h,v 1.2 2004/03/12 19:19:01 rzhe Exp $
 */

#ifndef __MHOOKS_H__
#define __MHOOKS_H__


#define MHOOKS_HOOKTYPE_NONE			0
#define MHOOKS_HOOKTYPE_MW				1
#define MHOOKS_HOOKTYPE_IP				2
#define MHOOKS_HOOKTYPE_LAST			MHOOKS_HOOKTYPE_IP

#define MHOOKS_MW_XBUTTON1_DOWN			0x00000001
#define MHOOKS_MW_XBUTTON1_UP			0x00000002
#define MHOOKS_MW_XBUTTON2_DOWN			0x00000004
#define MHOOKS_MW_XBUTTON2_UP			0x00000008
#define MHOOKS_MW_XBUTTON3_DOWN			0x00000010
#define MHOOKS_MW_XBUTTON3_UP			0x00000020
#define MHOOKS_MW_XBUTTON4_DOWN			0x00000040
#define MHOOKS_MW_XBUTTON4_UP			0x00000080
#define MHOOKS_MW_XBUTTON5_DOWN			0x00000100
#define MHOOKS_MW_XBUTTON5_UP			0x00000200

#define MHOOKS_IP_TILTMOVE_LEFT			0x00000400
#define MHOOKS_IP_TILTMOVE_RIGHT		0x00000800
#define MHOOKS_IP_MBUTTON_UP			0x00001000


#ifdef MHOOKS_MANUAL_LOAD

typedef BOOL (*MHooks_SetHook_t)(int _nType, HWND _hSinkWnd, UINT _uSinkMsg);
typedef BOOL (*MHooks_RemoveHook_t)();
typedef int (*MHooks_GetType_t)();

MHooks_SetHook_t MHooks_SetHook;
MHooks_RemoveHook_t MHooks_RemoveHook;
MHooks_GetType_t MHooks_GetType;

#else

#ifdef MHOOKS_EXPORTS
#define MHOOKS_API __declspec(dllexport)
#else
#define MHOOKS_API __declspec(dllimport)
#endif

BOOL MHOOKS_API MHooks_SetHook(int _nType, HWND _hSinkWnd, UINT _uSinkMsg);
BOOL MHOOKS_API MHooks_RemoveHook();
int MHOOKS_API MHooks_GetType();

#endif /* MHOOKS_MANUAL_LOAD */


#endif /* !__MHOOKS_H__ */
