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
// sv_user.c -- server code for moving users

#include "qwsvdef.h"

edict_t	*sv_player;

usercmd_t	cmd;

cvar_t	sv_spectalk = {"sv_spectalk", "1"};

cvar_t	sv_mapcheck	= {"sv_mapcheck", "1"};

extern	vec3_t	player_mins;

extern int fp_messages, fp_persecond, fp_secondsdead;
extern char fp_msg[];
extern cvar_t pausable;

extern double	sv_frametime;

/*
============================================================

USER STRINGCMD EXECUTION

host_client and sv_player will be valid.
============================================================
*/


/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
#ifdef QW262
#include "sv_user262.inc"
#endif

extern qboolean IsMDserver; // BorisU
extern	cvar_t	allow_old_clients; // BorisU

void SV_New_f (void)
{
	char		*gamedir;
	int			playernum;

	if (host_client->state == cs_spawned)
		return;

	host_client->state = cs_connected;
	host_client->connection_started = svs.realtime;

	// send the info about the new client to all connected clients
//	SV_FullClientUpdate (host_client, &sv.reliable_datagram);
//	host_client->sendinfo = true;

	gamedir = Info_ValueForKey (svs.info, "*gamedir");
	if (!gamedir[0])
		gamedir = "qw";

//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
//spawns.  These functions are written to not overflow
	if (host_client->num_backbuf) {
		Con_Printf("WARNING %s: [SV_New] Back buffered (%d0, clearing", host_client->name, host_client->netchan.message.cursize); 
		host_client->num_backbuf = 0;
		SZ_Clear(&host_client->netchan.message);
	}

	// send the serverdata
	MSG_WriteByte (&host_client->netchan.message, svc_serverdata);
// BorisU -->
#ifdef QW262
	MSG_WriteLong (&host_client->netchan.message, host_client->old_client ? OLD_PROTOCOL_VERSION : PROTOCOL_VERSION); // BorisU
#else
	MSG_WriteLong (&host_client->netchan.message, OLD_PROTOCOL_VERSION);
#endif
// <-- BorisU
	MSG_WriteLong (&host_client->netchan.message, svs.spawncount);
	MSG_WriteString (&host_client->netchan.message, gamedir);

	playernum = NUM_FOR_EDICT(host_client->edict)-1;
	if (host_client->spectator)
		playernum |= 128;
	MSG_WriteByte (&host_client->netchan.message, playernum);

	// send full levelname
	MSG_WriteString(&host_client->netchan.message,
#ifdef USE_PR2
		PR2_GetString(sv.edicts->v.message)
#else
		PR1_GetString(sv.edicts->v.message)
#endif
		);

	// send the movevars
	MSG_WriteFloat(&host_client->netchan.message, movevars.gravity);
	MSG_WriteFloat(&host_client->netchan.message, movevars.stopspeed);
	MSG_WriteFloat(&host_client->netchan.message, movevars.maxspeed);
	MSG_WriteFloat(&host_client->netchan.message, movevars.spectatormaxspeed);
	MSG_WriteFloat(&host_client->netchan.message, movevars.accelerate);
	MSG_WriteFloat(&host_client->netchan.message, movevars.airaccelerate);
	MSG_WriteFloat(&host_client->netchan.message, movevars.wateraccelerate);
	MSG_WriteFloat(&host_client->netchan.message, movevars.friction);
	MSG_WriteFloat(&host_client->netchan.message, movevars.waterfriction);
	MSG_WriteFloat(&host_client->netchan.message, movevars.entgravity);


#ifdef QW262
	{
		int			i;
		SV_New_f262();
	}
#endif
	
	// send music
	MSG_WriteByte (&host_client->netchan.message, svc_cdtrack);
	MSG_WriteByte (&host_client->netchan.message, sv.edicts->v.sounds);

	// send server info string
	MSG_WriteByte (&host_client->netchan.message, svc_stufftext);
	MSG_WriteString (&host_client->netchan.message, va("fullserverinfo \"%s\"\n", svs.info) );
}

/*
==================
SV_Soundlist_f
==================
*/
void SV_Soundlist_f (void)
{
	char		**s;
	int			n;

	if (host_client->state != cs_connected)
	{
		Con_Printf ("soundlist not valid -- allready spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		SV_ClearReliable (host_client);
		Con_Printf ("SV_Soundlist_f from different level\n");
		SV_New_f ();
		return;
	}

	n = atoi(Cmd_Argv(2));
	if (n + 1>= MAX_SOUNDS) {
		SV_ClearReliable (host_client);
		SV_ClientPrintf (host_client, PRINT_HIGH, "Cmd_Soundlist_f: Invalid soundlist index\n");
		SV_DropClient (host_client);
		return;
	}

//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
//spawns.  These functions are written to not overflow
	if (host_client->num_backbuf) {
		Con_Printf("WARNING %s: [SV_Soundlist] Back buffered (%d0, clearing", host_client->name, host_client->netchan.message.cursize); 
		host_client->num_backbuf = 0;
		SZ_Clear(&host_client->netchan.message);
	}

	MSG_WriteByte (&host_client->netchan.message, svc_soundlist);
	MSG_WriteByte (&host_client->netchan.message, n);
	for (s = sv.sound_precache+1 + n ; 
		*s && host_client->netchan.message.cursize < (MAX_MSGLEN/2); 
		s++, n++)
		MSG_WriteString (&host_client->netchan.message, *s);

	MSG_WriteByte (&host_client->netchan.message, 0);

	// next msg
	if (*s)
		MSG_WriteByte (&host_client->netchan.message, n);
	else
		MSG_WriteByte (&host_client->netchan.message, 0);
}

/*
==================
SV_Modellist_f
==================
*/
void SV_Modellist_f (void)
{
	char		**s;
	int			n;

	if (host_client->state != cs_connected)
	{
		Con_Printf ("modellist not valid -- allready spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		SV_ClearReliable (host_client);
		Con_Printf ("SV_Modellist_f from different level\n");
		SV_New_f ();
		return;
	}

	n = atoi(Cmd_Argv(2));
	if (n + 1 >= MAX_MODELS) {
		SV_ClearReliable (host_client);
		SV_ClientPrintf (host_client, PRINT_HIGH, "Cmd_Modellist_f: Invalid modellist index\n");
		SV_DropClient (host_client);
		return;
	}

//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
//spawns.  These functions are written to not overflow
	if (host_client->num_backbuf) {
		Con_Printf("WARNING %s: [SV_Modellist] Back buffered (%d0, clearing", host_client->name, host_client->netchan.message.cursize); 
		host_client->num_backbuf = 0;
		SZ_Clear(&host_client->netchan.message);
	}

	MSG_WriteByte (&host_client->netchan.message, svc_modellist);
	MSG_WriteByte (&host_client->netchan.message, n);
	for (s = sv.model_precache+1+n ; 
		*s && host_client->netchan.message.cursize < (MAX_MSGLEN/2); 
		s++, n++)
		MSG_WriteString (&host_client->netchan.message, *s);
	MSG_WriteByte (&host_client->netchan.message, 0);

	// next msg
	if (*s)
		MSG_WriteByte (&host_client->netchan.message, n);
	else
		MSG_WriteByte (&host_client->netchan.message, 0);
}

/*
==================
SV_PreSpawn_f
==================
*/
void SV_PreSpawn_f (void)
{
	unsigned	buf;
	unsigned	check;

	if (host_client->state != cs_connected)
	{
		Con_Printf ("prespawn not valid -- allready spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		SV_ClearReliable (host_client);
		Con_Printf ("SV_PreSpawn_f from different level\n");
		SV_New_f ();
		return;
	}
	
	buf = atoi(Cmd_Argv(2));
	if (buf >= sv.num_signon_buffers)
		buf = 0;

	if (!buf) {
		// should be three numbers following containing checksums
		check = atoi(Cmd_Argv(3));

//		Con_DPrintf("Client check = %d\n", check);

		if (sv_mapcheck.value && check != sv.worldmodel->checksum &&
			check != sv.worldmodel->checksum2) {
			SV_ClientPrintf (host_client, PRINT_HIGH, 
				"Map model file does not match (%s), %i != %i/%i.\n"
				"You may need a new version of the map, or the proper install files.\n",
				sv.modelname, check, sv.worldmodel->checksum, sv.worldmodel->checksum2);
			SV_DropClient (host_client); 
			return;
		}
		host_client->checksum = check;
	}

//NOTE:  This doesn't go through ClientReliableWrite since it's before the user
//spawns.  These functions are written to not overflow
	if (host_client->num_backbuf) {
		Con_Printf("WARNING %s: [SV_PreSpawn] Back buffered (%d0, clearing", host_client->name, host_client->netchan.message.cursize); 
		host_client->num_backbuf = 0;
		SZ_Clear(&host_client->netchan.message);
	}

	SZ_Write (&host_client->netchan.message, 
		sv.signon_buffers[buf],
		sv.signon_buffer_size[buf]);

	buf++;
	if (buf == sv.num_signon_buffers)
	{	// all done prespawning
		MSG_WriteByte (&host_client->netchan.message, svc_stufftext);
		MSG_WriteString (&host_client->netchan.message, va("cmd spawn %i 0\n",svs.spawncount) );
	}
	else
	{	// need to prespawn more
		MSG_WriteByte (&host_client->netchan.message, svc_stufftext);
		MSG_WriteString (&host_client->netchan.message, 
			va("cmd prespawn %i %i\n", svs.spawncount, buf) );
	}
}


/*
==================
SV_Spawn_f
==================
*/
extern void SV_FullClientUpdateToClient (client_t *client, client_t *cl);
void SV_Spawn_f (void)
{
	int			i;
	client_t	*client;
	edict_t		*ent;
	eval_t		*val;
	unsigned	n;
#ifdef USE_PR2
	string_t	savenetname;
#endif

	if (host_client->state != cs_connected)
	{
		SV_ClearReliable (host_client);
		Con_Printf ("Spawn not valid -- allready spawned\n");
		return;
	}

// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Con_Printf ("SV_Spawn_f from different level\n");
		SV_New_f ();
		return;
	}

	n = atoi(Cmd_Argv(2));

	if (n >= MAX_CLIENTS) {
		SV_ClientPrintf (host_client, PRINT_HIGH, "SV_Spawn_f: Invalid client start\n");
		SV_DropClient (host_client); 
		return;
	}

// send all current names, colors, and frag counts
	// FIXME: is this a good thing?
	SZ_Clear (&host_client->netchan.message);

// send current status of all other players

	// normally this could overflow, but no need to check due to backbuf
	for (i=n, client = svs.clients + n ; i<MAX_CLIENTS ; i++, client++)
		SV_FullClientUpdateToClient (client, host_client);
	
// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		ClientReliableWrite_Begin (host_client, svc_lightstyle, 
			3 + (sv.lightstyles[i] ? strlen(sv.lightstyles[i]) : 1));
		ClientReliableWrite_Byte (host_client, (char)i);
		ClientReliableWrite_String (host_client, sv.lightstyles[i]);
	}

	// set up the edict
	ent = host_client->edict;

#ifdef USE_PR2
	if ( !sv_vm )
	{
#endif
		memset(&ent->v, 0, progs->entityfields * 4);
		ent->v.netname = PR1_SetString(host_client->name);
#ifdef USE_PR2
	}
	else
	{
		savenetname = ent->v.netname;
		memset(&ent->v, 0, pr_edict_size - sizeof(edict_t) +
				sizeof(entvars_t));
		ent->v.netname = savenetname;
		//host_client->name = PR2_GetString(ent->v.netname);
		//strlcpy(PR2_GetString(ent->v.netname), host_client->name, CLIENT_NAME_LEN);
	}
#endif
// so spec will have right goalentity - if speccing someone
// qqshka {
	if(host_client->spectator && host_client->spec_track > 0)
		ent->v.goalentity = EDICT_TO_PROG(svs.clients[host_client->spec_track-1].edict);

// }

	ent->v.colormap = NUM_FOR_EDICT(ent);
	ent->v.team = 0;	// FIXME

	host_client->entgravity = 1.0;
	val =
#ifdef USE_PR2
		PR2_GetEdictFieldValue(ent, "gravity");
#else
		PR1_GetEdictFieldValue(ent, "gravity");
#endif
	if (val)
		val->_float = 1.0;
	host_client->maxspeed = sv_maxspeed.value;
	val =
#ifdef USE_PR2
		PR2_GetEdictFieldValue(ent, "maxspeed");
#else
		PR1_GetEdictFieldValue(ent, "maxspeed");
#endif
	if (val)
		val->_float = sv_maxspeed.value;

//
// force stats to be updated
//
	memset (host_client->stats, 0, sizeof(host_client->stats));

	ClientReliableWrite_Begin (host_client, svc_updatestatlong, 6);
	ClientReliableWrite_Byte (host_client, STAT_TOTALSECRETS);
	ClientReliableWrite_Long (host_client, pr_global_struct->total_secrets);

	ClientReliableWrite_Begin (host_client, svc_updatestatlong, 6);
	ClientReliableWrite_Byte (host_client, STAT_TOTALMONSTERS);
	ClientReliableWrite_Long (host_client, pr_global_struct->total_monsters);

	ClientReliableWrite_Begin (host_client, svc_updatestatlong, 6);
	ClientReliableWrite_Byte (host_client, STAT_SECRETS);
	ClientReliableWrite_Long (host_client, pr_global_struct->found_secrets);

	ClientReliableWrite_Begin (host_client, svc_updatestatlong, 6);
	ClientReliableWrite_Byte (host_client, STAT_MONSTERS);
	ClientReliableWrite_Long (host_client, pr_global_struct->killed_monsters);

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	ClientReliableWrite_Begin (host_client, svc_stufftext, 8);
	ClientReliableWrite_String (host_client, "skins\n" );

}

/*
==================
SV_SpawnSpectator
==================
*/
void SV_SpawnSpectator(void)
{
	int		i;
	edict_t	*e;

	VectorCopy(vec3_origin, sv_player->v.origin);
	VectorCopy(vec3_origin, sv_player->v.view_ofs);
	sv_player->v.view_ofs[2] = 22;

	// search for an info_playerstart to spawn the spectator at
	for (i = MAX_CLIENTS - 1; i < sv.num_edicts; i++)
	{
		e = EDICT_NUM(i);
		if (
#ifdef USE_PR2 /* phucking Linux implements strcmp as a macro */
			!strcmp(PR2_GetString(e->v.classname), "info_player_start")
#else
			!strcmp(PR1_GetString(e->v.classname), "info_player_start")
#endif
			)
		{
			VectorCopy(e->v.origin, sv_player->v.origin);

			return;
		}
	}
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f(void)
{
	int i;

	if (host_client->state == cs_spawned)
		return; // don't begin again

	host_client->state = cs_spawned;
	
	// handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		SV_ClearReliable (host_client);
		Con_Printf("SV_Begin_f from different level\n");
		SV_New_f();
		return;
	}

	SV_AutoStartRecord();

	if (!host_client->spectator)
	{
		char *uid = Info_ValueForKey(host_client->userinfo, "uid");

		SV_AddNameToDemoInfo(host_client->name);
		if (uid[0] != '\0')
			SV_AddUidToDemoInfo(uid);
	}

	if (host_client->spectator)
	{
		SV_SpawnSpectator();

		if (SpectatorConnect
#ifdef USE_PR2
			||  (sv_vm )
#endif
			)
		{
			// copy spawn parms out of the client_t
			for (i = 0; i < NUM_SPAWN_PARMS; i++)
				(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];
	
			// call the spawn function
			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(sv_player);

#ifdef USE_PR2
			if ( sv_vm )
				PR2_GameClientConnect(1);
			else
#endif
				PR_ExecuteProgram(SpectatorConnect);
		}
	}
	else
	{
		// copy spawn parms out of the client_t
		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

		// call the spawn function
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);

#ifdef USE_PR2
		if ( sv_vm )
			PR2_GameClientConnect(0);
		else
#endif
			PR_ExecuteProgram(pr_global_struct->ClientConnect);

		// actually spawn the player
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);

#ifdef USE_PR2
		if ( sv_vm )
			PR2_GamePutClientInServer(0);
		else
#endif
			PR_ExecuteProgram(pr_global_struct->PutClientInServer);	
	}

	// clear the net statistics, because connecting gives a bogus picture
	host_client->netchan.frame_latency = 0;
	host_client->netchan.frame_rate = 0;
	host_client->netchan.drop_count = 0;
	host_client->netchan.good_count = 0;

	// if we are paused, tell the client
	if (sv.paused)
	{
		ClientReliableWrite_Begin(host_client, svc_setpause, 2);
		ClientReliableWrite_Byte(host_client, sv.paused);
		SV_ClientPrintf(host_client, PRINT_HIGH, "Server is paused.\n");
	}

#if 0
//
// send a fixangle over the reliable channel to make sure it gets there
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = EDICT_NUM(1 + (host_client - svs.clients));
	MSG_WriteByte(&host_client->netchan.message, svc_setangle);
	for (i = 0; i < 2 ;i++)
		MSG_WriteAngle(&host_client->netchan.message, ent->v.angles[i]);
	MSG_WriteAngle(&host_client->netchan.message, 0);
#endif /* 0 */

#ifdef QW262
	if (allow_old_clients.value)
	{
		if (host_client->old_client)
		{
			SV_ClientPrintf(host_client, PRINT_HIGH, "\x1You are highly "
				"recommended to use QW262 protocol on this server\n\x1You "
				"can download QW262 client from http://qw262.da.ru\n");
			SV_BroadcastPrintf (PRINT_HIGH, "\x1%s is using QW230 compatible "
				"protocol\n", host_client->name);
		}
		else
			SV_BroadcastPrintf (PRINT_HIGH, "\x1%s is using QW262 compatible "
				"protocol\n", host_client->name);
	}
#endif
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void)
{
	byte	buffer[FILE_TRANSFER_BUF_SIZE];
	int		r, tmp;
	int		percent;
	int		size;
	double	clear, frametime;

	if (!host_client->download)
		return;

	tmp = host_client->downloadsize - host_client->downloadcount;

	if ((clear = host_client->netchan.cleartime) < svs.realtime)
		clear = svs.realtime;

	frametime = bound (0, host_client->netchan.frame_rate, 0.05);
	
	r = (int)((svs.realtime + frametime - host_client->netchan.cleartime)/host_client->netchan.rate);
	if (r <= 10)
		r = 10;
	if (r > FILE_TRANSFER_BUF_SIZE)
		r = FILE_TRANSFER_BUF_SIZE;

	// don't send too much if already buffering
	if (host_client->num_backbuf)
		r = 10;

	if (r > tmp)
		r = tmp;

	r = fread (buffer, 1, r, host_client->download);
	ClientReliableWrite_Begin (host_client, svc_download, 6+r);
	ClientReliableWrite_Short (host_client, r);

	host_client->downloadcount += r;
	size = host_client->downloadsize;
	if (!size)
		size = 1;
	percent = host_client->downloadcount*100/size;
	ClientReliableWrite_Byte (host_client, percent);
	ClientReliableWrite_SZ (host_client, buffer, r);

	if (host_client->downloadcount != host_client->downloadsize)
		return;

	fclose (host_client->download);
	host_client->download = NULL;
	host_client->file_percent = 0; //bliP: file percent


	// if map changed tell the client to reconnect 
	// FIX ME
/*	if (host_client->spawncount != svs.spawncount)
	{
		char *str = "changing\nreconnect\n";

		ClientReliableWrite_Begin (host_client, svc_stufftext, strlen(str)+2);
		ClientReliableWrite_String (host_client, str);
	}*/
}

void OutofBandPrintf(netadr_t where, char *fmt, ...)
{
	va_list		argptr;
	char	send[1024];
	
	send[0] = 0xff;
	send[1] = 0xff;
	send[2] = 0xff;
	send[3] = 0xff;
	send[4] = A2C_PRINT;
	va_start (argptr, fmt);
#ifdef _WIN32
	_vsnprintf (send + 5, sizeof(send) - 6, fmt, argptr);
	send[sizeof(send) - 6] = '\0';
#else
	vsnprintf (send + 5, sizeof(send) - 5, fmt, argptr);
#endif // _WIN32
	va_end (argptr);

	NET_SendPacket (strlen(send)+1, send, where);
}

extern netadr_t	net_local_adr;
extern void SV_ReplaceChar(char *s, char from, char to);
/*
==================
SV_NextUpload
==================
*/
void SV_NextUpload (void)
{
	byte	buffer[1024];
	int		percent;
	int		size;
	char	*name = host_client->uploadfn;

	SV_ReplaceChar(name, '\\', '/');
	if (!*name || !strncmp(name, "../", 3) || strstr(name, "/../") || *name == '/'
#ifdef _WIN32
	|| (name[1] == ':' && (*name >= 'a' && *name <= 'z' || *name >= 'A' && *name <= 'Z'))
#endif //_WIN32
		) {

		SV_ClientPrintf(host_client, PRINT_HIGH, "Upload denied\n");
		ClientReliableWrite_Begin (host_client, svc_stufftext, 8);
		ClientReliableWrite_String (host_client, "stopul");

		// suck out rest of packet
		size = MSG_ReadShort ();
		MSG_ReadByte ();
		msg_readcount += size;
		return;
	}

	size = MSG_ReadShort ();
	percent = MSG_ReadByte ();

	if (!host_client->upload)
	{
		host_client->sendImportantCommand=0;
		host_client->upload = fopen(name, "wb");
		if (!host_client->upload) {
			Sys_Printf("Can't create %s\n", name);
			ClientReliableWrite_Begin (host_client, svc_stufftext, 8);
			ClientReliableWrite_String (host_client, "stopul");
			*name = 0;
			return;
		}
		Sys_Printf("Receiving %s from %d...\n", name, host_client->userid);
		if (host_client->remote_snap)
			SV_ClientPrintf(host_client->snap_from,PRINT_HIGH, "\x1""Server receiving %s from %d...\n", 
				name, host_client->userid);
	}

	fwrite (net_message.data + msg_readcount, 1, size, host_client->upload);
	msg_readcount += size;

	Con_DPrintf ("UPLOAD: %d received; percent=%d\n", size, percent);

	if (percent != 100) {
		ClientReliableWrite_Begin (host_client, svc_stufftext, 8);
		ClientReliableWrite_String (host_client, "nextul\n");
	} else {
		fclose (host_client->upload);
		host_client->upload = NULL;
//		host_client->file_percent = 0; //bliP: file percent

		Sys_Printf("%s upload completed.\n", host_client->uploadfn);

		if (host_client->remote_snap) {
			char *p;

			if ((p = strchr(name, '/')) != NULL)
				p++;
			else
				p = name;
			SV_ClientPrintf(host_client->snap_from,PRINT_HIGH, "\x1""%s upload completed.\n", 
				host_client->uploadfn);
			if (!NET_CompareBaseAdr(net_local_adr, host_client->snap_from->netchan.remote_address)) {
//	screenshot is requested from the local client
//				SV_ClientPrintf(host_client->snap_from,PRINT_HIGH, "\x1""No need to download\n");
//			} else {
				sprintf(buffer,"download %s\n",p);
				ClientReliableWrite_Begin (host_client->snap_from, svc_stufftext, strlen(buffer)+1);
				ClientReliableWrite_String (host_client->snap_from, buffer);
			}
		} 
	}
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char	*name, n[MAX_OSPATH], *val;
	int		i;
	
	extern	cvar_t	allow_download;
	extern	cvar_t	allow_download_skins;
	extern	cvar_t	allow_download_models;
	extern	cvar_t	allow_download_sounds;
	extern	cvar_t	allow_download_maps;
	extern	cvar_t	allow_download_demos;
	extern	cvar_t	sv_demoDir;
	
	extern	int		file_from_pak; // ZOID did file come from pak?

	if (Cmd_Argc() != 2)
	{
		Con_Printf("download [filename]\n");
		return;
	}
	name = Cmd_Argv(1);
	SV_ReplaceChar(name, '\\', '/');

// hacked by zoid to allow more conrol over download
		// first off, no .. or global allow check
	if (

		(
		// leading dot is no good
		*name == '.' 
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// no ..
		|| strstr (name, "..") 
#ifdef _WIN32
		// no leading X:
		|| ( name[1] == ':' && (*name >= 'a' && *name <= 'z' ||
				*name >= 'A' && *name <= 'Z') )
#endif //_WIN32
		)

		||
		
		(
		!host_client->special &&
		(
		// global allow check
		!allow_download.value
		// next up, skin check
		|| (strncmp(name, "skins/", 6) == 0 && !allow_download_skins.value)
		// now models
		|| (strncmp(name, "progs/", 6) == 0 && !allow_download_models.value)
		// now sounds
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds.value)
		// now maps (note special case for maps, must not be in pak)
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps.value)
		// now demos
		|| (strncmp(name, "demos/", 6) == 0 && !allow_download_demos.value)
		|| (strncmp(name, "demonum/", 8) == 0 && !allow_download_demos.value)
		// MUST be in a subdirectory	
		|| !strstr (name, "/") 
		// no logs 
		|| ( (i = strlen(name)) < 4 ? 0 : !Q_strnicmp(name + i - 4, ".log", 4) )
		)
		)
		)	
	{	// don't allow anything with .. path
		ClientReliableWrite_Begin (host_client, svc_download, 4);
		ClientReliableWrite_Short (host_client, -1);
		ClientReliableWrite_Byte (host_client, 0);
		return;
	}

	if (host_client->download) {
		fclose (host_client->download);
		host_client->download = NULL;
		val = Info_ValueForKey (host_client->userinfo, "rate");
		host_client->netchan.rate = 1.0/SV_BoundRate(false, atoi(val));
	}

// mvdsv -->
	if ( !strncmp(name, "demos/", 6) && sv_demoDir.string[0]) {
		Q_snprintfz (n, sizeof(n), "%s/%s", sv_demoDir.string, name + 6);
		name = n;
	} else if (!strncmp(name, "demonum/", 8)) {
		int num;

		if ((num = atoi(name + 8)) == 0 && name[8] != '0')
		{
			Con_Printf("usage: download demonum/num\n");
			
			ClientReliableWrite_Begin (host_client, svc_download, 4);
			ClientReliableWrite_Short (host_client, -1);
			ClientReliableWrite_Byte (host_client, 0);
			return;
		}
		name = SV_DemoNum(num);
		if (!name) {
			Con_Printf("demo num %d not found\n", num);

			ClientReliableWrite_Begin (host_client, svc_download, 4);
			ClientReliableWrite_Short (host_client, -1);
			ClientReliableWrite_Byte (host_client, 0);
			return;
		}
		Con_Printf("downloading demos/%s\n",name);
		
		Q_snprintfz (n, sizeof(n), "download demos/%s\n", name);

		ClientReliableWrite_Begin (host_client, svc_stufftext,strlen(n) + 2);
		ClientReliableWrite_String (host_client, n);
		return;
	}
// <-- mvdsv
	
	// lowercase name (needed for casesen file systems)
	{
		char *p;

		for (p = name; *p; p++)
			*p = (char)tolower(*p);
	}

// bliP: special download - fixme check this works.... -->
// techlogin download uses simple path from quake folder
	if (host_client->special) {
		host_client->download = fopen (name, "rb");
		if (host_client->download) {
			if (developer.value)
				Sys_Printf ("FindFile: %s\n", name);
			host_client->downloadsize = COM_FileLength (host_client->download);
		}
	} else
// <-- bliP
		host_client->downloadsize = COM_FOpenFile (name, &host_client->download);
	host_client->downloadcount = 0;

	if (!host_client->download
		// special check for maps, if it came from a pak file, don't allow
		// download  ZOID
		|| (strncmp(name, "maps/", 5) == 0 && file_from_pak))
	{
		if (host_client->download) {
			fclose(host_client->download);
			host_client->download = NULL;
		}

		Sys_Printf ("Couldn't download %s to %s\n", name, host_client->name);
		ClientReliableWrite_Begin (host_client, svc_download, 4);
		ClientReliableWrite_Short (host_client, -1);
		ClientReliableWrite_Byte (host_client, 0);
		return;
	}

// mvdsv -->
	val = Info_ValueForKey (host_client->userinfo, "drate");
	if (*val && atoi(val))
		host_client->netchan.rate = 1.0/SV_BoundRate(true, atoi(val));
// <-- mvdsv

	SV_NextDownload_f ();
	Sys_Printf ("Downloading %s to %s\n", name, host_client->name);

	SV_ClientPrintf (host_client, PRINT_HIGH, "File %s is %.0fKB (%.2fMB)\n",
		name, (float)host_client->downloadsize / 1024,
		(float)host_client->downloadsize / 1024 / 1024);
}

//=============================================================================

/*
==================
SV_Say
==================
*/
short int SayTrueMessage=0;
void SV_Say (int team)
{
	client_t *client;
	int		j, tmp, cls = 0;
	char	*p;
	char	text[2048];
	char	t1[32], *t2;
	int		id = 0; // shut up compiler :)
	char	*uid;

	if(team==3 && Cmd_Argc()<3) {
		SV_ClientPrintf(host_client,PRINT_HIGH,"Usage:  say_id <id> <message>\nor   :  say_id <uid> <message>\n");
		return;
	} else if(team==4 && Cmd_Argc()<3) {
		SV_ClientPrintf(host_client,PRINT_HIGH,"Usage:  say_to_team <team> <message>\n");
		return;
	} else if (Cmd_Argc () < 2)
		return;

	j=strlen(Cmd_Args());
	if (j>0 && Cmd_Args()[j-1]=='\r') {
		Cmd_Args()[j-1]=0;
		j--;
	}
	if (j>0 && Cmd_Args()[j-1]=='\"') {
		Cmd_Args()[j-1]=0;
	}

	if(strlen(Cmd_Args())>900) {
		SV_ClientPrintf(host_client,PRINT_HIGH,"Say string too long\n");
		return;
	}

	if (team==1) {
		strncpy (t1, Info_ValueForKey (host_client->userinfo, "team"), 31);
		t1[31] = 0;
	}

	if (team==4) {
		strncpy (t1, Cmd_Argv(1), 31);
		t1[31] = 0;
	}

#ifdef USE_PR2
	if ( sv_vm )
	{
		qboolean ret;

		SV_EndRedirect ();

		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);

		ret = PR2_ClientSay(team);

		SV_BeginRedirect (RD_CLIENT);

		if (ret)
			return; // say/say_team was handled by mod
	}
#endif
	if (host_client->spectator && (!sv_spectalk.value || team))
		sprintf (text, "[SPEC] %s: ", host_client->name);
	else if (team==1)
		sprintf (text, "(%s): ", host_client->name);
	else if(team==3)
		sprintf (text, "{%s}: ", host_client->name);
	else if(team==4)
		sprintf (text, "<%s>: ", host_client->name);
	else if(!team)
		sprintf (text, "%s: ", host_client->name);
	else 
		*text='\0';
	

	if (fp_messages) {
		if (!sv.paused && svs.realtime<host_client->lockedtill) {
			SV_ClientPrintf(host_client, PRINT_CHAT,
				"You can't talk for %d more seconds\n", 
					(int) (host_client->lockedtill - svs.realtime));
			return;
		}
		tmp = host_client->whensaidhead - fp_messages + 1;
		if (tmp < 0)
			tmp = 10+tmp;
		if (!sv.paused &&
			host_client->whensaid[tmp] && (svs.realtime-host_client->whensaid[tmp] < fp_persecond)) {
			host_client->lockedtill = svs.realtime + fp_secondsdead;
			if (fp_msg[0])
				SV_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: %s\n", fp_msg);
			else
				SV_ClientPrintf(host_client, PRINT_CHAT,
					"FloodProt: You can't talk for %d seconds.\n", fp_secondsdead);
			return;
		}
		host_client->whensaidhead++;
		if (host_client->whensaidhead > 9)
			host_client->whensaidhead = 0;
		host_client->whensaid[host_client->whensaidhead] = svs.realtime;
	}
	
	uid = p = Cmd_Args();
	if(team==3 || team==4) { //remove first argument
		while(*p!=' ') p++; 
		*p++='\0'; 
		while(*p==' ') p++; 
		if (team==3) id = atoi(uid);
		if (team==4) SV_ClientPrintf(host_client, PRINT_CHAT, "to %s team: %s\n",uid,p);
	}
	
	if (*p == '"')
	{
		p++;
		//p[strlen(p)-1] = 0; 
		// last " was removed earlier
	}
	
	strcat(text, p);
	if(text[strlen(text)-4]=='#') text[strlen(text)-4]='\0';
	strcat(text, "\n");
	if(team==1) 
		p+=strlen(p)-4; // p contains channel, including #


	Sys_Printf ("%s", text);
	Log_Printf (SV_CONSOLELOG, text); // BorisU

	for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		if (host_client->spectator && !sv_spectalk.value)
			if (!client->spectator)
				continue;

			if (team==1 || team==4) {
			// the spectator team
				if (host_client->spectator) {
					if (!client->spectator)
						continue;
				} else {
					t2 = Info_ValueForKey (client->userinfo, "team");
					if (strcmp(t1, t2) || client->spectator)
						continue;	// on different teams
					if(*p=='#' && host_client!=client) {
						t2 = Info_ValueForKey (client->userinfo, "filter");
						if(*t2 && !strstr(t2,p)) continue;
					}
				}
			} else if(team==3) {
				if(id==client->userid || !strcmp(uid,Info_ValueForKey(client->userinfo,"uid"))) {
					SV_ClientPrintf(host_client, PRINT_CHAT, "to %s: %s\n",client->name,p);
					SV_ClientPrintf(client, PRINT_CHAT, "%s", text);
					break;
				} else continue;
			}

			cls |= 1 << j;
			SV_ClientPrintf2(client, PRINT_CHAT, "%s", text);
	}
	if(j==MAX_CLIENTS && team==3) SV_ClientPrintf(host_client, PRINT_HIGH,
				"userid '%s' not found\n",uid);

	if (!sv.demorecording || !cls)
		return;

	// non-team messages should be seen allways, even if not tracking any player
	if (!team && ((host_client->spectator && sv_spectalk.value) || !host_client->spectator))
	{
		DemoWrite_Begin (dem_all, 0, strlen(text)+3);
	} else 
		DemoWrite_Begin (dem_multiple, cls, strlen(text)+3);

	MSG_WriteByte ((sizebuf_t*)demo.dbuf, svc_print);
	MSG_WriteByte ((sizebuf_t*)demo.dbuf, PRINT_CHAT);
	MSG_WriteString ((sizebuf_t*)demo.dbuf, text);

}

/*
==================
SV_IDSay_f
==================
*/
void SV_IDSay_f(void)
{
	SV_Say (3);
}

/*
==================
SV_ISay_f
==================
*/
void SV_Info_f(void)
{
	char *text;
	text=MSG_ReadString();
	SV_BroadcastPrintf (PRINT_HIGH, "%s\n", text); 
}


/*
==================
SV_Say_f
==================
*/
void SV_Say_f(void)
{
	SV_Say (0);
}
/*
==================
SV_Say_Team_f
==================
*/
void SV_Say_Team_f(void)
{
	SV_Say (1);
}

// BorisU
/*
==================
SV_Say_To_Team_f
==================
*/
void SV_Say_To_Team_f(void)
{
	SV_Say (4);
}


//============================================================================

/*
=================
SV_Pings_f

The client is showing the scoreboard, so send new ping times for all
clients
=================
*/
void SV_Pings_f (void)
{
	client_t *client;
	int		j;

	for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;

		ClientReliableWrite_Begin (host_client, svc_updateping, 4);
		ClientReliableWrite_Byte (host_client, j);
		ClientReliableWrite_Short (host_client, SV_CalcPing(client));
		ClientReliableWrite_Begin (host_client, svc_updatepl, 4);
		ClientReliableWrite_Byte (host_client, j);
		ClientReliableWrite_Byte (host_client, client->lossage);
	}
}

/*
==================
SV_Kill_f
==================
*/
void SV_Kill_f(void)
{
	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintf(host_client, PRINT_HIGH, "Can't suicide -- allready"
			"dead!\n");
		return;
	}
	
	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
#ifdef USE_PR2
	if ( !sv_vm )
#endif
		PR1_ClientKill();
#ifdef USE_PR2
	else
		PR2_ClientKill();
#endif
}

/*
==================
SV_TogglePause
==================
*/
void SV_TogglePause (const char *msg)
{
	int i;
	client_t *cl;

	sv.paused ^= 1;

	if (msg)
		SV_BroadcastPrintf (PRINT_HIGH, "%s", msg);

	// send notification to all clients
	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
	{
		if (cl->state < cs_preconnected)
			continue;
		ClientReliableWrite_Begin (cl, svc_setpause, 2);
		ClientReliableWrite_Byte (cl, sv.paused);
	}
}

/*
==================
SV_Pause_f
==================
*/
void SV_Pause_f (void)
{
	char st[CLIENT_NAME_LEN + 32];


	if (!pausable.value) {
		SV_ClientPrintf (host_client, PRINT_HIGH, "Pause not allowed.\n");
		return;
	}

	if (host_client->spectator) {
		SV_ClientPrintf (host_client, PRINT_HIGH, "Spectators can not pause.\n");
		return;
	}

	if (!sv.paused)
		sprintf (st, "%s paused the game\n", host_client->name);
	else {
		if (unpause_counter)
			return; // double unpause
		sprintf (st, "%s unpaused the game\n", host_client->name);
		if (sv_newpause.value) {
			SV_BroadcastPrintf (PRINT_HIGH, "%s", st);
			unpause_begin = Sys_DoubleTime ();
			unpause_counter = 6;
			return;
		}
	}
	SV_TogglePause(st);
}


/*
=================
SV_Drop_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Drop_f (void)
{
	SV_EndRedirect ();
	if (!host_client->spectator)
		SV_BroadcastPrintf (PRINT_HIGH, "%s dropped\n", host_client->name);
	SV_DropClient (host_client);	
}

/*
=================
SV_PTrack_f

Change the bandwidth estimate for a client
=================
*/
void SV_PTrack_f (void)
{
	int		i;
	edict_t *ent, *tent;
	
	if (!host_client->spectator)
		return;

	if (Cmd_Argc() != 2)
	{
		// turn off tracking
		host_client->spec_track = 0;
		ent = EDICT_NUM(host_client - svs.clients + 1);
		tent = EDICT_NUM(0);
		ent->v.goalentity = EDICT_TO_PROG(tent);
		return;
	}
	
	i = atoi(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CLIENTS || svs.clients[i].state != cs_spawned ||
		svs.clients[i].spectator) {
		SV_ClientPrintf (host_client, PRINT_HIGH, "Invalid client to track\n");
		host_client->spec_track = 0;
		ent = EDICT_NUM(host_client - svs.clients + 1);
		tent = EDICT_NUM(0);
		ent->v.goalentity = EDICT_TO_PROG(tent);
		return;
	}
	host_client->spec_track = i + 1; // now tracking

	ent = EDICT_NUM(host_client - svs.clients + 1);
	tent = EDICT_NUM(i + 1);
	ent->v.goalentity = EDICT_TO_PROG(tent);
}


/*
=================
SV_Rate_f

Change the bandwidth estimate for a client
=================
*/
void SV_Rate_f (void)
{
	int		rate;
	
	if (Cmd_Argc() != 2)
	{
		SV_ClientPrintf (host_client, PRINT_HIGH, "Current rate is %i\n",
			(int)(1.0/host_client->netchan.rate + 0.5));
		return;
	}

	rate = SV_BoundRate (host_client->download != NULL, atoi(Cmd_Argv(1)));

	SV_ClientPrintf (host_client, PRINT_HIGH, "Net rate set to %i\n", rate);
	host_client->netchan.rate = 1.0/rate;
}


/*
=================
SV_Msg_f

Change the message level for a client
=================
*/
void SV_Msg_f (void)
{	
	if (Cmd_Argc() != 2)
	{
		SV_ClientPrintf (host_client, PRINT_HIGH, "Current msg level is %i\n",
			host_client->messagelevel);
		return;
	}
	
	host_client->messagelevel = atoi(Cmd_Argv(1));

	SV_ClientPrintf (host_client, PRINT_HIGH, "Msg level set to %i\n", host_client->messagelevel);
}

char *shortinfotbl[] =
{
	"name",
	"team",
	"skin",
	"topcolor",
	"bottomcolor",
	"uid",
	NULL
};

/*
==================
SV_SetInfo_f

Allow clients to change userinfo
==================
*/
void SV_SetInfo_f (void)
{
	int		i;
	char	oldval[MAX_INFO_STRING];
	char	*key;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("User info settings:\n");
		Info_Print (host_client->userinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}

	key = Cmd_Argv(1);

	if (key[0] == '*')
		return;		// don't set priveledged values

	strcpy(oldval, Info_ValueForKey(host_client->userinfo, key));

#ifdef USE_PR2
        if(sv_vm)
        {
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);

		if( PR2_UserInfoChanged() )
			return;
        }
#endif

	Info_SetValueForKey (host_client->userinfo, key, Cmd_Argv(2), MAX_INFO_STRING);

	if (!strcmp(Info_ValueForKey(host_client->userinfo, key), oldval))
		return; // key hasn't changed


	// process any changed values
	SV_ExtractFromUserinfo (host_client, !strcmp(key, "name"));

	for (i = 0; shortinfotbl[i] != NULL; i++)
		if (key[0] == '_' || !strcmp(key, shortinfotbl[i]))
		{
			char *new = Info_ValueForKey(host_client->userinfo, key);
			Info_SetValueForKey (host_client->userinfoshort, key, new, MAX_INFO_STRING);

			i = host_client - svs.clients;
			MSG_WriteByte (&sv.reliable_datagram, svc_setinfo);
			MSG_WriteByte (&sv.reliable_datagram, i);
			MSG_WriteString (&sv.reliable_datagram, key);
			MSG_WriteString (&sv.reliable_datagram, new);
			break;
		}

	if (host_client->state == cs_spawned && !host_client->spectator)
	{
		if (!strcmp(key, "name"))
			SV_AddNameToDemoInfo(host_client->name);

		if (!strcmp(key, "uid"))
		{
			char *uid = Info_ValueForKey(host_client->userinfo,
				"uid");

			if (uid[0] != '\0')
				SV_AddUidToDemoInfo(uid);
		}
	}
}

/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void)
{
	Info_Print (svs.info);
}

extern void SV_OutOfTimeKick(client_t *cl);
void SV_NoSnap_f(void)
{
	if (*host_client->uploadfn) {
		*host_client->uploadfn = 0;
		SV_BroadcastPrintf (PRINT_HIGH, "%s refused remote screenshot\n", host_client->name);
		SV_OutOfTimeKick(host_client);
	}
}

void SV_Unknown_f(void)
{
	SV_OutOfTimeKick(host_client);
}

// mvdsv -->
void SV_StopDownload_f(void)
{
	if (host_client->download) {
		fclose (host_client->download);
		host_client->download = NULL;
	}
}

//bliP: upload files
/*
=================
SV_TechLogin_f
Login to upload
=================
*/
int Master_Rcon_Validate (void);
void SV_TechLogin_f (void)
{	
	if (host_client->logincount > 4) //denied
		return;

	if (Cmd_Argc() < 2)
	{
		host_client->special = false;
		host_client->logincount = 0;
		return;
	}

	if (!Master_Rcon_Validate()) //don't even let them know they're wrong
	{
		host_client->logincount++;
		return;
	}

	host_client->special = true;
	SV_ClientPrintf (host_client, PRINT_HIGH, "Logged in.\n");
}

/*
================
SV_ClientUpload_f
================
*/
void SV_ClientUpload_f (void)
{
	char str[MAX_OSPATH];

	if (host_client->state != cs_spawned)
		return;

	if (!host_client->special)
	{
		Con_Printf ("Client not tagged to upload.\n");
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("upload [local filename] [remote filename]\n");
		return;
	}

	snprintf(host_client->uploadfn, sizeof(host_client->uploadfn), "%s", Cmd_Argv(2));

	if (!host_client->uploadfn[0])
	{ //just in case..
		Con_Printf ("Bad file name.\n");
		return;
	}

	if (Sys_FileTime(host_client->uploadfn) != -1)
	{
		Con_Printf ("File already exists.\n");
		return;
	}

	host_client->remote_snap = false;
	COM_CreatePath(host_client->uploadfn); //fixed, need to create path
	snprintf(str, sizeof(str), "cmd fileul \"%s\"\n", Cmd_Argv(1));
	ClientReliableWrite_Begin (host_client, svc_stufftext, strlen(str) + 2);
	ClientReliableWrite_String (host_client, str);
}

// <-- mvdsv

typedef struct
{
	char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] =
{
	{"new", SV_New_f},
	{"modellist", SV_Modellist_f},
	{"soundlist", SV_Soundlist_f},
	{"prespawn", SV_PreSpawn_f},
	{"spawn", SV_Spawn_f},
	{"begin", SV_Begin_f},

	{"drop", SV_Drop_f},
	{"pings", SV_Pings_f},

// issued by hand at client consoles	
	{"rate", SV_Rate_f},
	{"kill", SV_Kill_f},
	{"pause", SV_Pause_f},
	{"msg", SV_Msg_f},

	{"say_id", SV_IDSay_f}, // -=MD=-
	{"say", SV_Say_f},
	{"say_team", SV_Say_Team_f},
	{"say_to_team", SV_Say_To_Team_f}, // BorisU

	{"setinfo", SV_SetInfo_f},

	{"serverinfo", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{"ptrack", SV_PTrack_f}, //ZOID - used with autocam

	{"snap", SV_NoSnap_f},

// mvdsv -->
	{"techlogin", SV_TechLogin_f}, // do we need it?
	{"upload", SV_ClientUpload_f},
	{"stopdownload", SV_StopDownload_f},
	{"demolist", SV_DemoList_f},
// <-- mvdsv


#ifdef QW262
	ucmds262
#endif
	
	{NULL, NULL}
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand(char *s)
{
	ucmd_t *u;
	
	Cmd_TokenizeString(s);
	sv_player = host_client->edict;

	SV_BeginRedirect(RD_CLIENT);

	for (u = ucmds; u->name; u++)
		if (!strcmp(Cmd_Argv(0), u->name))
		{
			u->func();
			break;
		}

	if (!u->name)
#ifdef USE_PR2
	{
		if ( sv_vm )
		{
			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(sv_player);
			if (!PR2_ClientCmd())
				Con_Printf("Bad user command: %s\n", Cmd_Argv(0));
		}
		else
#endif
			Con_Printf("Bad user command: %s\n", Cmd_Argv(0));
#ifdef USE_PR2
	}
#endif

	SV_EndRedirect();
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
====================
AddLinksToPmove

====================
*/
static void AddLinksToPmove ( areanode_t *node )
{
	link_t		*l, *next;
	edict_t		*check;
	int			pl;
	int			i;
	physent_t	*pe;
	vec3_t		pmove_mins, pmove_maxs;

	for (i=0 ; i<3 ; i++)
	{
		pmove_mins[i] = pmove.origin[i] - 256;
		pmove_maxs[i] = pmove.origin[i] + 256;
	}

	pl = EDICT_TO_PROG(sv_player);

	// touch linked edicts
	for (l = node->solid_edicts.next ; l != &node->solid_edicts ; l = next)
	{
		next = l->next;
		check = EDICT_FROM_AREA(l);

		if (check->v.owner == pl)
			continue;		// player's own missile
		if (check->v.solid == SOLID_BSP 
			|| check->v.solid == SOLID_BBOX 
			|| check->v.solid == SOLID_SLIDEBOX)
		{
			if (check == sv_player)
				continue;

			for (i=0 ; i<3 ; i++)
				if (check->v.absmin[i] > pmove_maxs[i]
				|| check->v.absmax[i] < pmove_mins[i])
					break;
			if (i != 3)
				continue;
			if (pmove.numphysent == MAX_PHYSENTS)
				return;
			pe = &pmove.physents[pmove.numphysent];
			pmove.numphysent++;

			VectorCopy (check->v.origin, pe->origin);
			pe->info = NUM_FOR_EDICT(check);
			if (check->v.solid == SOLID_BSP) {
				if ((unsigned)check->v.modelindex >= MAX_MODELS)
					SV_Error ("AddLinksToPmove: check->v.modelindex >= MAX_MODELS");
				pe->model = sv.models[(int)(check->v.modelindex)];
				if (!pe->model)
					SV_Error ("SOLID_BSP with a non-bsp model");
			}
			else
			{
				pe->model = NULL;
				VectorCopy (check->v.mins, pe->mins);
				VectorCopy (check->v.maxs, pe->maxs);
			}
		}
	}
	
// recurse down both sides
	if (node->axis == -1)
		return;

	if ( pmove_maxs[node->axis] > node->dist )
		AddLinksToPmove ( node->children[0] );
	if ( pmove_mins[node->axis] < node->dist )
		AddLinksToPmove ( node->children[1] );
}

/*
===========
SV_PreRunCmd
===========
Done before running a player command.  Clears the touch array
*/
static byte playertouch[(MAX_EDICTS+7)/8];

void SV_PreRunCmd(void)
{
	memset(playertouch, 0, sizeof(playertouch));
}

/*
===========
SV_RunCmd
===========
*/
void SV_RunCmd (usercmd_t *ucmd, qboolean inside) //bliP: 24/9
{
	int			i, n;
	vec3_t		offset;
	//bliP: 24/9 anti speed ->
	int			tmp_time;

	if (!inside)
	{
		/* AM101 method */
		tmp_time = Q_rint((svs.realtime - host_client->last_check) * 1000); // ie. Old 'timepassed'
		if (tmp_time)
		{
			if (ucmd->msec > tmp_time)
			{
				tmp_time += host_client->msecs; // use accumulated msecs
				if (ucmd->msec > tmp_time)
				{ // If still over...
					ucmd->msec = tmp_time;
					host_client->msecs = 0;
				}
				else
				{
					host_client->msecs = tmp_time - ucmd->msec; // readjust to leftovers
				}
			}
			else
			{
				// Add up extra msecs
				host_client->msecs += (tmp_time - ucmd->msec);
			}
		}

		host_client->last_check = svs.realtime;

		/* Cap it */
		if (host_client->msecs > 500)
			host_client->msecs = 500;
		else if (host_client->msecs < 0)
			host_client->msecs = 0;
	}
	//<-

	cmd = *ucmd;

	// chop up very long commands
	if (cmd.msec > 50)
	{
		int		oldmsec;
		oldmsec = ucmd->msec;
		cmd.msec = oldmsec / 2;
		SV_RunCmd(&cmd, true);

		cmd.msec = oldmsec / 2;
		cmd.impulse = 0;
		SV_RunCmd(&cmd, true);

		return;
	}

	// copy humans' intentions to progs
	sv_player->v.button0 = ucmd->buttons & 1;
	sv_player->v.button2 = (ucmd->buttons & 2) >> 1;
	sv_player->v.button1 = (ucmd->buttons & 4) >> 2;
	if (ucmd->impulse)
		sv_player->v.impulse = ucmd->impulse;

	// clamp view angles
	if (ucmd->angles[PITCH] > 80.0)
		ucmd->angles[PITCH] = 80.0;
	if (ucmd->angles[PITCH] < -70.0)
		ucmd->angles[PITCH] = -70.0;
	if (!sv_player->v.fixangle)
		VectorCopy (ucmd->angles, sv_player->v.v_angle);

//
// angles
// show 1/3 the pitch angle and all the roll angle	
	if (sv_player->v.health > 0)
	{
		if (!sv_player->v.fixangle)
		{
			sv_player->v.angles[PITCH] = -sv_player->v.v_angle[PITCH] / 3;
			sv_player->v.angles[YAW] = sv_player->v.v_angle[YAW];
		}
		sv_player->v.angles[ROLL] = 0;
	}

	sv_frametime = ucmd->msec * 0.001;
	if (sv_frametime > 0.1)
		sv_frametime = 0.1;

	if (!host_client->spectator)
	{
		pr_global_struct->frametime = sv_frametime;
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
#ifdef USE_PR2
		if ( sv_vm )
			PR2_GameClientPreThink(0);
		else
#endif
			PR1_GameClientPreThink(0);

		SV_RunThink(sv_player);
	}

	// copy player state to pmove
	VectorSubtract (sv_player->v.mins, player_mins, offset);
	VectorAdd (sv_player->v.origin, offset, pmove.origin);
	VectorCopy (sv_player->v.velocity, pmove.velocity);
	VectorCopy (sv_player->v.v_angle, pmove.angles);

	pmove.spectator = host_client->spectator;
	pmove.waterjumptime = sv_player->v.teleport_time;
	pmove.cmd = *ucmd;
	pmove.dead = sv_player->v.health <= 0;
	pmove.oldbuttons = host_client->oldbuttons;

	// build physent list
	pmove.numphysent = 1;
	pmove.physents[0].model = sv.worldmodel;
	AddLinksToPmove( sv_areanodes );

	// fill in movevars
	movevars.entgravity = host_client->entgravity;
	movevars.maxspeed = host_client->maxspeed;
	movevars.bunnyspeedcap = pm_bunnyspeedcap.value;

	// do the move
	PM_PlayerMove ();

	
	// get player state back out of pmove
	host_client->oldbuttons = pmove.oldbuttons;
	sv_player->v.teleport_time = pmove.waterjumptime;
	sv_player->v.waterlevel = waterlevel;
	sv_player->v.watertype = watertype;
	if (onground != -1)
	{
		sv_player->v.flags = (int)sv_player->v.flags | FL_ONGROUND;
		sv_player->v.groundentity = EDICT_TO_PROG(EDICT_NUM(pmove.physents[onground].info));
	}
	else
		sv_player->v.flags = (int)sv_player->v.flags & ~FL_ONGROUND;

	VectorSubtract (pmove.origin, offset, sv_player->v.origin);
	VectorCopy (pmove.velocity, sv_player->v.velocity);
	VectorCopy (pmove.angles, sv_player->v.v_angle);

	if (!host_client->spectator)
	{
		// link into place and touch triggers
		SV_LinkEdict(sv_player, true);

		// touch other objects
		for (i = 0; i < pmove.numtouch; i++)
		{
			edict_t *ent;
			n = pmove.physents[pmove.touchindex[i]].info;
			ent = EDICT_NUM(n);
			if (!ent->v.touch || (playertouch[n / 8] & (1 << (n % 8))))
				continue;
			pr_global_struct->self = EDICT_TO_PROG(ent);
			pr_global_struct->other = EDICT_TO_PROG(sv_player);
#ifdef USE_PR2
			if ( sv_vm )
				PR2_EdictTouch(ent->v.touch);
			else
#endif
				PR1_EdictTouch(ent->v.touch);
			playertouch[n / 8] |= 1 << (n % 8);
		}
	}
}

/*
===========
SV_PostRunCmd
===========
Done after running a player command.
*/
void SV_PostRunCmd(void)
{
	// run post-think

	if (!host_client->spectator)
	{
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
#ifdef USE_PR2
		if ( sv_vm )
			PR2_GameClientPostThink(0);
		else
#endif
			PR_ExecuteProgram(pr_global_struct->PlayerPostThink);
		SV_RunNewmis();
	}
	else if (SpectatorThink
#ifdef USE_PR2
		||  ( sv_vm )
#endif
		)
	{
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
#ifdef USE_PR2
		if ( sv_vm )
			PR2_GameClientPostThink(1);
		else
#endif
			PR_ExecuteProgram(SpectatorThink);
	}
}

/*
===================
SV_ExecuteClientMove

Run one or more client move commands (more than one if some
packets were dropped)
===================
*/
static void SV_ExecuteClientMove (client_t *cl, usercmd_t oldest, usercmd_t oldcmd, usercmd_t newcmd)
{
	int net_drop;
	
	if (sv.paused)
		return;

	SV_PreRunCmd();

	net_drop = cl->netchan.dropped;
	if (net_drop < 20)
	{
		while (net_drop > 2)
		{
			SV_RunCmd (&cl->lastcmd, false);
			net_drop--;
		}
	}
	if (net_drop > 1)
		SV_RunCmd (&oldest, false);
	if (net_drop > 0)
		SV_RunCmd (&oldcmd, false);
	SV_RunCmd (&newcmd, false);
	
	SV_PostRunCmd();
}

/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage (client_t *cl)
{
	int		c;
	char	*s;
	usercmd_t	oldest, oldcmd, newcmd;
	client_frame_t	*frame;
	vec3_t o;
	qboolean	move_issued = false; //only allow one move command
	int		checksumIndex;
	byte	checksum, calculatedChecksum;
	int		seq_hash;

	// calc ping time
	frame = &cl->frames[cl->netchan.incoming_acknowledged & UPDATE_MASK];
	frame->ping_time = svs.realtime - frame->senttime;

	// make sure the reply sequence number matches the incoming
	// sequence number 
	if (cl->netchan.incoming_sequence >= cl->netchan.outgoing_sequence)
		cl->netchan.outgoing_sequence = cl->netchan.incoming_sequence;
	else
		cl->send_message = false;	// don't reply, sequences have slipped		

	// save time for ping calculations
	cl->frames[cl->netchan.outgoing_sequence & UPDATE_MASK].senttime = svs.realtime;
	cl->frames[cl->netchan.outgoing_sequence & UPDATE_MASK].ping_time = -1;

	host_client = cl;
	sv_player = host_client->edict;

	seq_hash = cl->netchan.incoming_sequence;

	// mark time so clients will know how much to predict
	// other players
 	cl->localtime = sv.time;
	cl->delta_sequence = -1;	// no delta unless requested

	if(cl->sendImportantCommand && svs.realtime-cl->sendImportantCommand>4)
		SV_OutOfTimeKick(cl);
	
	while (1)
	{
		if (msg_badread)
		{
			Con_Printf ("SV_ReadClientMessage: badread\n");
			SV_DropClient (cl);
			return;
		}	

		c = MSG_ReadByte ();
		if (c == -1)
			break;
				
		switch (c)
		{
		default:
			Con_Printf ("SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case clc_nop:
			break;

		case clc_delta:
			cl->delta_sequence = MSG_ReadByte ();
			break;

		case clc_move:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = true;

			checksumIndex = MSG_GetReadCount();
			checksum = (byte)MSG_ReadByte ();

			// read loss percentage
			//bliP: file percent ->
			cl->lossage = MSG_ReadByte();
			if (cl->file_percent)
				cl->lossage = cl->file_percent;
			//<-

			MSG_ReadDeltaUsercmd (&nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&oldcmd, &newcmd);

			if ( cl->state != cs_spawned )
				break;

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte(
				net_message.data + checksumIndex + 1,
				MSG_GetReadCount() - checksumIndex - 1,
				seq_hash);

			if (calculatedChecksum != checksum)
			{
				Con_DPrintf ("Failed command checksum for %s(%d) (%d != %d)\n", 
					cl->name, cl->netchan.incoming_sequence, checksum, calculatedChecksum);
				return;
			}

			SV_ExecuteClientMove (cl, oldest, oldcmd, newcmd);

			cl->lastcmd = newcmd;
			cl->lastcmd.buttons = 0; // avoid multiple fires on lag
			break;

		case clc_stringcmd:	
			s = MSG_ReadString ();
			s[1023] = 0;
			SV_ExecuteUserCommand (s);
			break;

		case clc_tmove:
			o[0] = MSG_ReadCoord();
			o[1] = MSG_ReadCoord();
			o[2] = MSG_ReadCoord();
			// only allowed by spectators
			if (host_client->spectator) {
				VectorCopy(o, sv_player->v.origin);
				SV_LinkEdict(sv_player, false);
			}
			break;

		case clc_upload:
			SV_NextUpload();
			break;
#ifdef QW262
		SV_ExecuteClientMessage262();
#endif


		}
	}
}

/*
==============
SV_UserInit
==============
*/
void SV_UserInit (void)
{
	Cvar_RegisterVariable (&sv_spectalk);
	Cvar_RegisterVariable (&sv_mapcheck);
}


