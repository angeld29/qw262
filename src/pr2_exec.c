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
 *  $Id: pr2_exec.c,v 1.11 2006/11/26 21:30:36 angel Exp $
 */

#include <stdarg.h>

#include "qwsvdef.h"
#include "g_public.h"


#ifdef USE_PR2

#include "vm_local.h"

gameData_t *gamedata;

cvar_t	sv_progtype = {"sv_progtype","0"};	// bound the size of the
#ifdef QVM_PROFILE
extern cvar_t sv_enableprofile;
#endif
//int usedll;

void ED2_PrintEdicts (void);
void PR2_Profile_f (void);
void ED2_PrintEdict_f (void);
void ED_Count (void);
void PR_CleanLogText_Init(); 
void VM_VmInfo_f( void );
void VM_VmProfile_f( void );
void PR2_Init(void)
{
	int p;
	int usedll;
	Cvar_RegisterVariable(&sv_progtype);
	Cvar_RegisterVariable(&sv_progsname);
#ifdef QVM_PROFILE
	//Cvar_RegisterVariable(&sv_enableprofile);
#endif	

	p = COM_CheckParm ("-progtype");

	if (p && p < com_argc)
	{
		usedll = atoi(com_argv[p + 1]);

		if (usedll > VMI_COMPILED)
			usedll = VMI_NONE;
		Cvar_SetValue(&sv_progtype,usedll);
	}


	Cmd_AddCommand ("edict", ED2_PrintEdict_f);
	Cmd_AddCommand ("edicts", ED2_PrintEdicts);
	Cmd_AddCommand ("edictcount", ED_Count);
//	Cmd_AddCommand ("profile", PR2_Profile_f);
    
	Cmd_AddCommand ("mod", PR2_GameConsoleCommand);
	Cmd_AddCommand( "vmprofile", VM_VmProfile_f );
	Cmd_AddCommand( "vminfo", VM_VmInfo_f );


	PR_CleanLogText_Init();

}

//===========================================================================
// PR2_GetString
//===========================================================================
char *PR2_GetString(intptr_t num)
{

    if(!sv_vm)
        return PR_GetString(num);

    switch (sv_vm->type)
    {
        case VMI_NONE:
            return PR_GetString(num);

        case VMI_NATIVE:
            if (num)
                return (char *) num;
            else
                return "";

        case VMI_BYTECODE:
        case VMI_COMPILED:
            if (num<=0)
                return "";
            return VM_ExplicitArgPtr( sv_vm, num );
    }

    return NULL;
}

//===========================================================================
// PR2_SetString
// FIXME for VM 
//===========================================================================
intptr_t PR2_SetString(char *s) 
{
    if(!sv_vm)
        return PR_SetString(s);

    switch (sv_vm->type)
    {
        case VMI_NONE:
            return PR_SetString(s);

        case VMI_NATIVE:
            return (int) s;

        case VMI_BYTECODE:
        case VMI_COMPILED:
            return VM_ExplicitPtr2VM( sv_vm, s );
    }

    return 0;
}

/*
=================
PR2_LoadEnts
=================
*/
extern char *pr2_ent_data_ptr;

void PR2_LoadEnts(char *data)
{
	pr2_ent_data_ptr = data;

	//Init parse
	VM_Call(sv_vm, 0, GAME_LOADENTS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GameStartFrame
//===========================================================================
void PR2_GameStartFrame()
{
	VM_Call(sv_vm, 1, GAME_START_FRAME, (int) (sv.time * 1000), 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0);
}

//===========================================================================
// GameClientConnect
//===========================================================================
void PR2_GameClientConnect(int spec)
{
	VM_Call(sv_vm, 1, GAME_CLIENT_CONNECT, spec, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GamePutClientInServer
//===========================================================================
void PR2_GamePutClientInServer(int spec)
{
	VM_Call(sv_vm, 1, GAME_PUT_CLIENT_IN_SERVER, spec, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GameClientDisconnect
//===========================================================================
void PR2_GameClientDisconnect(int spec)
{
	VM_Call(sv_vm, 1, GAME_CLIENT_DISCONNECT, spec, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GameClientPreThink
//===========================================================================
void PR2_GameClientPreThink(int spec)
{
	VM_Call(sv_vm, 1, GAME_CLIENT_PRETHINK, spec, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GameClientPostThink
//===========================================================================
void PR2_GameClientPostThink(int spec)
{
	VM_Call(sv_vm, 1, GAME_CLIENT_POSTTHINK, spec, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// ClientCmd return false on unknown command
//===========================================================================
qboolean PR2_ClientCmd()
{
	return VM_Call(sv_vm, 0, GAME_CLIENT_COMMAND, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}


//===========================================================================
// GameSetNewParms
//===========================================================================
void PR2_GameSetNewParms()
{
	VM_Call(sv_vm, 0, GAME_SETNEWPARMS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// GameSetNewParms
//===========================================================================
void PR2_GameSetChangeParms()
{
	VM_Call(sv_vm, 0, GAME_SETCHANGEPARMS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}


//===========================================================================
// EdictTouch
//===========================================================================
void PR2_EdictTouch()
{
	VM_Call(sv_vm, 0, GAME_EDICT_TOUCH, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// EdictThink
//===========================================================================
void PR2_EdictThink()
{
	VM_Call(sv_vm, 0, GAME_EDICT_THINK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// EdictBlocked
//===========================================================================
void PR2_EdictBlocked()
{
	VM_Call(sv_vm, 0, GAME_EDICT_BLOCKED, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// UserInfoChanged
//===========================================================================
qboolean PR2_UserInfoChanged()
{
	return VM_Call(sv_vm, 0, GAME_CLIENT_USERINFO_CHANGED, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

//===========================================================================
// UserInfoChanged
//===========================================================================
void PR2_GameShutDown()
{
	VM_Call(sv_vm, 0, GAME_SHUTDOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    VM_Free(sv_vm);
}

//=========================================================================== 
// ClientSay return false if say unhandled by mod 
//=========================================================================== 
qboolean PR2_ClientSay(int isTeamSay) 
{ 
        return VM_Call(sv_vm, 1, GAME_CLIENT_SAY, isTeamSay, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); 
} 

//===========================================================================
// UserInfoChanged
//===========================================================================
void PR2_GameConsoleCommand(void)
{
	int     old_other, old_self;	
	client_t	*cl;
	int			i;

	
        if( sv_vm )
        {
        	old_self = pr_global_struct->self;
        	old_other = pr_global_struct->other;
        	pr_global_struct->other = sv_cmd;
        	pr_global_struct->self = 0;

		for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++) {
			if (!cl->state)
				continue;
			if ( cl->isBot )
				continue;

			if (NET_CompareAdr(cl->netchan.remote_address, net_from)){
				pr_global_struct->self = EDICT_TO_PROG(cl->edict);
				break;
			}
		}

		VM_Call(sv_vm, 0, GAME_CONSOLE_COMMAND, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        	pr_global_struct->self = old_self;
        	pr_global_struct->other = old_other;
        }
}
extern field_t *fields;
void PR2_FS_Restart();

void PR2_InitProg(  )
{
	PR2_FS_Restart(  );

	gamedata = ( gameData_t * ) VM_Call( sv_vm, 2, GAME_INIT, ( int ) ( sv.time * 1000 ),
					     ( int ) ( Sys_DoubleTime(  ) * 100000 ), 0, 0, 0, 0, 0, 0, 0, 0,
					     0, 0 );

	if ( !gamedata )
		SV_Error( "PR2_InitProg gamedata == NULL" );

	gamedata = ( gameData_t * ) PR2_GetString( ( int ) gamedata );
	if ( gamedata->APIversion < 8 || gamedata->APIversion > GAME_API_VERSION )
		SV_Error( "PR2_InitProg: Incorrect API version" );

	sv.edicts = ( edict_t * ) PR2_GetString( ( int ) gamedata->ents );
	pr_global_struct = ( globalvars_t * ) PR2_GetString( ( int ) gamedata->global );

	pr_globals = ( float * ) pr_global_struct;
	fields = ( field_t * ) PR2_GetString( ( int ) gamedata->fields );
	pr_edict_size = gamedata->sizeofent;
}

#endif /* USE_PR2 */
