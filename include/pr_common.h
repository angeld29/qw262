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
void 		ED1_ClearUserEdict (edict_t *e, client_t *cl);
#ifndef USE_PR2
	#define PR_LoadProgs PR1_LoadProgs
	#define PR_InitProg PR1_InitProg
	#define PR_GameShutDown PR1_GameShutDown
	#define PR_UnLoadProgs PR1_UnLoadProgs

//	#define PR_Init PR1_Init
	#define PR_GetString PR1_GetString
	#define PR_SetString PR1_SetString
	#define PR_SetString2 PR1_SetString2
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
	#define ED_ClearUserEdict ED1_ClearUserEdict
#else
	void 	 PR_Init(void);
	void 	 PR_InitProg();
	char* 	 PR_GetString(int num);
	int  	 PR_SetString(char *s) ;
	void  	 PR_LoadEnts(char *data);
	void  	 PR_GameStartFrame(void);
	void  	 PR_GameClientConnect(int spec);
	void  	 PR_GamePutClientInServer(int spec);
	void  	 PR_GameClientDisconnect(int spec);
	void  	 PR_GameClientPreThink(int spec);
	void  	 PR_GameClientPostThink(int spec);
	qboolean   PR_ClientCmd(void);
	void  	 PR_ClientKill(void);
	void  	 PR_GameSetNewParms(void);
	void  	 PR_GameSetChangeParms(void);
	void  	 PR_EdictTouch(func_t f);
	void  	 PR_EdictThink(func_t f);
	void  	 PR_EdictBlocked(func_t f);
	qboolean   PR_UserInfoChanged(void);
	void  	 PR_GameShutDown(void);
	qboolean   PR_ClientSay(int isTeamSay) ;
	void  	 PR_GameConsoleCommand(void);
	void  	 PR_UnLoadProgs(void);
	void  	 PR_LoadProgs(void);
	void  	 PR_SetString2(int*,char *,int);
	void 	 ED_ClearUserEdict (edict_t *e, client_t *cl);
	eval_t *PR_GetEdictFieldValue(edict_t *ed, char *field);

typedef enum vm_type_e
{
	VM_NONE,
	VM_NATIVE,
	VM_BYTECODE,
	VM_MAX
} vm_type_t;

typedef struct prog_funcs_s
{
	void 	(*Init)(void);
	void    (*InitProg)();
	char* 	(*GetString)(int num);
	int  	(*SetString)(char *s) ;
	void  	(*LoadEnts)(char *data);
	void  	(*GameStartFrame)(void);
	void  	(*GameClientConnect)(int spec);
	void  	(*GamePutClientInServer)(int spec);
	void  	(*GameClientDisconnect)(int spec);
	void  	(*GameClientPreThink)(int spec);
	void  	(*GameClientPostThink)(int spec);
	qboolean  (*ClientCmd)(void);
	void  	(*ClientKill)(void);
	void  	(*GameSetNewParms)(void);
	void  	(*GameSetChangeParms)(void);
	void  	(*EdictTouch)(func_t f);
	void  	(*EdictThink)(func_t f);
	void  	(*EdictBlocked)(func_t f);
	qboolean  (*UserInfoChanged)(void);
	void  	(*GameShutDown)(void);
	qboolean  (*ClientSay)(int isTeamSay) ;
	void  	(*UnLoadProgs)(void);
	void  	(*LoadProgs)(void);
	void  	(*SetString2)(int*,char *,int);
	void 	(*ClearUserEdict)(edict_t *e, client_t *cl);
	eval_t *(*GetEdictFieldValue)(edict_t *ed, char *field);
//	void  	(*GameConsoleCommand)(void);
} prog_funcs_t;

typedef struct prog_type_s
{
	vm_type_t type;
	prog_funcs_t funcs;
} prog_type_t;
extern cvar_t	sv_progtype;
#endif



#endif /* !__PR_COMMON_H__*/
