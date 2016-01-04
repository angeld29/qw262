/*
 *  QW262
 *  Copyright (C) 2004  [sd] angel
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
 *  $Id: pr_common.h,v 1.6 2006/11/26 21:30:35 angel Exp $
 */

#ifndef __PR_COMMON_H__
#define __PR_COMMON_H__
#ifndef USE_PR2
	#define PR_LoadProgs PR1_LoadProgs
	#define PR_InitProg PR1_InitProg
	#define PR_GameShutDown PR1_GameShutDown
	#define PR_UnLoadProgs PR1_UnLoadProgs

	#define PR_Init PR1_Init
	#define PR_GetString PR1_GetString
	#define PR_SetString PR1_SetString
	#define PR_GetEdictFieldValue PR1_GetEdictFieldValue

	#define PR_GameClientDisconnect PR1_GameClientDisconnect
	#define PR_GameClientConnect PR1_GameClientConnect
	#define PR_GamePutClientInServer PR1_GamePutClientInServer
	#define PR_GameClientPreThink PR1_GameClientPreThink
	#define PR_GameClientPostThink PR1_GameClientPostThink
	#define PR_ClientSay PR1_ClientSay
	#define PR_ClientCmd PR1_ClientCmd

	#define PR_GameSetChangeParms PR1_GameSetChangeParms
	#define PR_GameSetNewParms PR1_GameSetNewParms
	#define PR_GameStartFrame PR1_GameStartFrame
	#define PR_ClientKill PR1_ClientKill
	#define PR_UserInfoChanged PR1_UserInfoChanged
	#define PR_LoadEnts PR1_LoadEnts
	#define PR_EdictThink PR1_EdictThink
	#define PR_EdictTouch PR1_EdictTouch
	#define PR_EdictBlocked PR1_EdictBlocked
#else
	#define PR_LoadProgs PR2_LoadProgs
	#define PR_InitProg PR2_InitProg
	#define PR_GameShutDown PR2_GameShutDown
	#define PR_UnLoadProgs PR2_UnLoadProgs

	#define PR_Init PR2_Init
	#define PR_GetString PR2_GetString
	#define PR_SetString PR2_SetString
	#define PR_GetEdictFieldValue PR2_GetEdictFieldValue

	#define PR_GameClientDisconnect PR2_GameClientDisconnect
	#define PR_GameClientConnect PR2_GameClientConnect
	#define PR_GamePutClientInServer PR2_GamePutClientInServer
	#define PR_GameClientPreThink PR2_GameClientPreThink
	#define PR_GameClientPostThink PR2_GameClientPostThink
	#define PR_ClientSay PR2_ClientSay
	#define PR_ClientCmd PR2_ClientCmd

	#define PR_GameSetChangeParms PR2_GameSetChangeParms
	#define PR_GameSetNewParms PR2_GameSetNewParms
	#define PR_GameStartFrame PR2_GameStartFrame
	#define PR_ClientKill PR2_ClientKill
	#define PR_UserInfoChanged PR2_UserInfoChanged
	#define PR_LoadEnts PR2_LoadEnts
	#define PR_EdictThink PR2_EdictThink
	#define PR_EdictTouch PR2_EdictTouch
	#define PR_EdictBlocked PR2_EdictBlocked
#endif

#endif /* !__PR_COMMON_H__*/
