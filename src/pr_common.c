
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <stdarg.h>
#include "qwsvdef.h"
#ifdef USE_PR2
cvar_t	sv_progtype = {"sv_progtype","0"};	// bound the size of the

prog_type_t allprogs[] = {
	{ VM_NONE, {
				   PR1_Init,
				   PR1_InitProg,
				   PR1_GetString,
				   PR1_SetString,
				   PR1_LoadEnts,
				   PR1_GameStartFrame,
				   PR1_GameClientConnect,
				   PR1_GamePutClientInServer,
				   PR1_GameClientDisconnect,
				   PR1_GameClientPreThink,
				   PR1_GameClientPostThink,
				   PR1_ClientCmd,
				   PR1_ClientKill,
				   PR1_GameSetNewParms,
				   PR1_GameSetChangeParms,
				   PR1_EdictTouch,
				   PR1_EdictThink,
				   PR1_EdictBlocked,
				   PR1_UserInfoChanged,
				   PR1_GameShutDown,
				   PR1_ClientSay,
				   PR1_UnLoadProgs,
				   PR1_LoadProgs,
				   PR1_SetString2,
				   ED1_ClearUserEdict,
				   PR1_GetEdictFieldValue
			   },},
	{ VM_NATIVE, {
				   PR2_Init,
				   PR2_InitProg,
				   PR2_GetString,
				   PR2_SetString,
				   PR2_LoadEnts,
				   PR2_GameStartFrame,
				   PR2_GameClientConnect,
				   PR2_GamePutClientInServer,
				   PR2_GameClientDisconnect,
				   PR2_GameClientPreThink,
				   PR2_GameClientPostThink,
				   PR2_ClientCmd,
				   PR2_ClientKill,
				   PR2_GameSetNewParms,
				   PR2_GameSetChangeParms,
				   PR2_EdictTouch,
				   PR2_EdictThink,
				   PR2_EdictBlocked,
				   PR2_UserInfoChanged,
				   PR2_GameShutDown,
				   PR2_ClientSay,
				   PR2_UnLoadProgs,
				   PR2_LoadProgs,
				   PR2_SetString2,
				   ED2_ClearUserEdict,
				   PR2_GetEdictFieldValue
			   },},
	{ VM_BYTECODE, {
				   PR2_Init,
				   PR2_InitProg,
				   PR2_GetString,
				   PR2_SetString,
				   PR2_LoadEnts,
				   PR2_GameStartFrame,
				   PR2_GameClientConnect,
				   PR2_GamePutClientInServer,
				   PR2_GameClientDisconnect,
				   PR2_GameClientPreThink,
				   PR2_GameClientPostThink,
				   PR2_ClientCmd,
				   PR2_ClientKill,
				   PR2_GameSetNewParms,
				   PR2_GameSetChangeParms,
				   PR2_EdictTouch,
				   PR2_EdictThink,
				   PR2_EdictBlocked,
				   PR2_UserInfoChanged,
				   PR2_GameShutDown,
				   PR2_ClientSay,
				   PR2_UnLoadProgs,
				   PR2_LoadProgs,
				   PR2_SetString2,
				   ED2_ClearUserEdict,
				   PR2_GetEdictFieldValue
			   },},
};
prog_type_t* active_prog = NULL;

void PR_Init(void)
{
	int p;
	int usedll;
	Cvar_RegisterVariable(&sv_progtype);
	Cvar_RegisterVariable(&sv_progsname);

	p = COM_CheckParm ("-progtype");

	if (p && p < com_argc)
	{
		usedll = atoi(com_argv[p + 1]);

		if (usedll > VM_MAX)
			usedll = VM_NONE;
		Cvar_SetValue(&sv_progtype,usedll);
	}
	PR2_Init();
}

void  	 PR_LoadProgs(void)
{
	int i;
	active_prog = NULL;	
	for(  i = 0; i<	(sizeof(allprogs)/sizeof(allprogs[0])); i++ )
	{
		if( allprogs[i].type == sv_progtype.value ){
			active_prog = allprogs+i;
			break;
		}
	}
	if( !active_prog ) 
		SV_Error ("PR_LoadProgs: couldn't init progtype %d", sv_progtype.value);
	active_prog->funcs.LoadProgs();
}
void  	 PR_InitProg(){
	if( active_prog && active_prog->funcs.InitProg ) active_prog->funcs.InitProg();
}
	
char* 	 PR_GetString(int num){
	if( active_prog ) return active_prog->funcs.GetString(num);
	return NULL;
}
int  	 PR_SetString(char *s) {
	if( active_prog ) return active_prog->funcs.SetString(s);
	return 0;
}
void  	 PR_LoadEnts(char *data){
	if( active_prog && active_prog->funcs.LoadEnts ) active_prog->funcs.LoadEnts(data);
}
void  	 PR_GameStartFrame(void){
	if( active_prog && active_prog->funcs.GameStartFrame ) active_prog->funcs.GameStartFrame();
}
void  	 PR_GameClientConnect(int spec){
	if( active_prog && active_prog->funcs.GameClientConnect ) active_prog->funcs.GameClientConnect(spec);
}
void  	 PR_GamePutClientInServer(int spec){
	if( active_prog && active_prog->funcs.GamePutClientInServer ) active_prog->funcs.GamePutClientInServer(spec);
}
void  	 PR_GameClientDisconnect(int spec){
	if( active_prog && active_prog->funcs.GameClientDisconnect ) active_prog->funcs.GameClientDisconnect(spec);
}
void  	 PR_GameClientPreThink(int spec){
	if( active_prog && active_prog->funcs.GameClientPreThink ) active_prog->funcs.GameClientPreThink(spec);
}
void  	 PR_GameClientPostThink(int spec){
	if( active_prog && active_prog->funcs.GameClientPostThink ) active_prog->funcs.GameClientPostThink(spec);
}
qboolean   PR_ClientCmd(void){
	if( active_prog ) return active_prog->funcs.ClientCmd();
	return false;
}
void  	 PR_ClientKill(void){
	if( active_prog && active_prog->funcs.ClientKill ) active_prog->funcs.ClientKill();
}
void  	 PR_GameSetNewParms(void){
	if( active_prog && active_prog->funcs.GameSetNewParms ) active_prog->funcs.GameSetNewParms();
}
void  	 PR_GameSetChangeParms(void){
	if( active_prog && active_prog->funcs.GameSetChangeParms ) active_prog->funcs.GameSetChangeParms();
}
void  	 PR_EdictTouch(func_t f){
	if( active_prog && active_prog->funcs.EdictTouch ) active_prog->funcs.EdictTouch(f);
}
void  	 PR_EdictThink(func_t f){
	if( active_prog && active_prog->funcs.EdictThink ) active_prog->funcs.EdictThink(f);
}
void  	 PR_EdictBlocked(func_t f){
	if( active_prog && active_prog->funcs.EdictBlocked ) active_prog->funcs.EdictBlocked(f);
}
qboolean   PR_UserInfoChanged(void){
	if( active_prog ) return active_prog->funcs.UserInfoChanged();
	return false;
}
void  	 PR_GameShutDown(void){
	if( active_prog && active_prog->funcs.GameShutDown ) active_prog->funcs.GameShutDown();
}
qboolean   PR_ClientSay(int isTeamSay) {
	if( active_prog ) return active_prog->funcs.ClientSay(isTeamSay) ;
	return false;
}
/*void  	 PR_GameConsoleCommand(void){
	if( active_prog && active_prog->funcs.GameConsoleCommand ) active_prog->funcs.GameConsoleCommand();
}*/
void  	 PR_UnLoadProgs(void){
	if( active_prog && active_prog->funcs.UnLoadProgs ) active_prog->funcs.UnLoadProgs();
}
void  	 PR_SetString2(int* num,char *s,int len){
	if( active_prog && active_prog->funcs.SetString2 ) active_prog->funcs.SetString2(num,s,len);
}
void 	 ED_ClearUserEdict(edict_t *e, client_t *cl){
	if( active_prog && active_prog->funcs.ClearUserEdict ) active_prog->funcs.ClearUserEdict(e, cl);
}
eval_t *PR_GetEdictFieldValue(edict_t *ed, char *field){
	if( active_prog && active_prog->funcs.GetEdictFieldValue) active_prog->funcs.GetEdictFieldValue(ed, field);
	return NULL;
}

#endif /* USE_PR2 */

