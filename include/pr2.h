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
 *  $Id: pr2.h,v 1.6 2006/11/26 21:30:35 angel Exp $
 */

#ifndef __PR2_H__
#define __PR2_H__


extern int sv_syscall(int arg, ...);
extern int sv_sys_callex(byte *data, unsigned int len, int fn, pr2val_t*arg);
typedef void (*pr2_trapcall_t)(byte* base, unsigned int mask, pr2val_t* stack, pr2val_t*retval);

//extern int usedll;
extern vm_t* sv_vm;

void		PR2_Init(void);
void		PR2_UnLoadProgs(void);
void		PR2_LoadProgs(void);
void		PR2_GameStartFrame(void);
void		PR2_LoadEnts(char *data);
void		PR2_GameClientConnect(int spec);
void		PR2_GamePutClientInServer(int spec);
void		PR2_GameClientDisconnect(int spec);
void		PR2_GameClientPreThink(int spec);
void		PR2_GameClientPostThink(int spec);
qboolean	PR2_ClientCmd(void);
void		PR2_ClientKill(void);
void		PR2_GameSetNewParms(void);
void		PR2_GameSetChangeParms(void);
void		PR2_EdictTouch(func_t f);
void		PR2_EdictThink(func_t f);
void		PR2_EdictBlocked(func_t f);
qboolean 	PR2_UserInfoChanged(void);
void 		PR2_GameShutDown(void);
void 		PR2_GameConsoleCommand(void);
qboolean	PR2_ClientSay(int isTeamSay);

char*		PR2_GetString(int);
int		PR2_SetString(char*s);
void		PR2_SetString2(int*,char*,int);
void		PR2_RunError(char *error, ...);
void		ED2_Free(edict_t *ed);
edict_t*	ED2_Alloc(void);
void		ED2_ClearEdict(edict_t *e);
eval_t*		PR2_GetEdictFieldValue(edict_t *ed, char *field);
void 		PR2_InitProg(void);
void		ED2_ClearUserEdict(edict_t *e, client_t *cl);

#endif /* !__PR2_H__ */
