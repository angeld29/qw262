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
#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#include <time.h>
#include "qwsvdef.h"

#include "sv_io.h"


extern int NET_Sendto(const unsigned char *data, unsigned size,
		const struct sockaddr_in *addr);


quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution (compatability)

double		curtime;

int			host_hunklevel;

netadr_t	master_adr[MAX_MASTERS];	// address of group servers

client_t	*host_client;			// current client

cvar_t	sv_mintic = {"sv_mintic","0.03"};	// bound the size of the
cvar_t	sv_maxtic = {"sv_maxtic","0.1"};	// physics time tic 

cvar_t	timeout = {"timeout","45"};		// seconds without any message
cvar_t	zombietime = {"zombietime", "2"};	// seconds to sink messages
											// after disconnect

cvar_t	rcon_password = {"rcon_password", ""};	// password for remote server commands
cvar_t	password = {"password", ""};	// password for entering the game
cvar_t	spectator_password = {"spectator_password", ""};	// password for entering as a sepctator

char	master_password[128] = "";	// master password for remote server commands
char	bot_password[128] = "";		// password for rcon bot

cvar_t	allow_download = {"allow_download", "1"};
cvar_t	allow_download_skins = {"allow_download_skins", "1"};
cvar_t	allow_download_models = {"allow_download_models", "1"};
cvar_t	allow_download_sounds = {"allow_download_sounds", "1"};
cvar_t	allow_download_maps = {"allow_download_maps", "1"};
cvar_t	allow_download_demos = {"allow_download_demos", "1"}; // mvdsv

cvar_t	developer = {"info_messages","0"};		// show extra messages

cvar_t sv_highchars = {"sv_highchars", "1"};

cvar_t sv_phs = {"sv_phs", "1"};

cvar_t pausable	= {"pausable", "1"};

// BorisU -->
client_t *WatcherId=NULL; // -=MD=-
netadr_t BotIP;
qboolean IsBot;
cvar_t	sv_nailhack	= {"sv_nailhack", "0"};
cvar_t	sv_maxrate = {"sv_maxrate", "10000"};
cvar_t	sv_loadentfiles = {"sv_loadentfiles", "0"};
cvar_t	sv_announcercon = {"sv_announcercon", "0"};
int		sv_cmd = SV_CMD_CONSOLE;
#ifdef QW262
cvar_t	allow_old_clients = {"allow_old_clients", "0"};
#endif
cvar_t	sv_newpause = {"sv_newpause", "1"};
double	unpause_begin;
int		unpause_counter;
// <-- BorisU

// Highlander -->
cvar_t	sv_demofps = {"sv_demofps", "20"};
cvar_t	sv_demoPings = {"sv_demopings", "3"};
cvar_t	sv_demoNoVis = {"sv_demonovis", "1"};
cvar_t	sv_demoUseCache = {"sv_demousecache", "0"};
cvar_t	sv_demoCacheSize = {"sv_democachesize", "0", CVAR_ROM};
cvar_t	sv_demoMaxSize  = {"sv_demomaxsize", "0"}; 
cvar_t	sv_demoMaxDirSize = {"sv_demomaxdirsize", "0"};
cvar_t	sv_demoExtraNames = {"sv_demoextranames", "0"};
cvar_t	sv_demoDir = {"sv_demodir", "demos", 0, sv_demoDir_OnChange};
// <-- Highlander

cvar_t	sv_demoMaxSizeOverwrite = {"sv_demomaxsizeoverwrite", "100"}; // BorisU

cvar_t	sv_maxdownloadrate = {"sv_maxdownloadrate", "0"}; // mvdsv

//
// game rules mirrored in svs.info
//
cvar_t	fraglimit = {"fraglimit","0",CVAR_SERVERINFO};
cvar_t	timelimit = {"timelimit","0",CVAR_SERVERINFO};
cvar_t	teamplay = {"teamplay","0",CVAR_SERVERINFO};
cvar_t	samelevel = {"samelevel","0"};
cvar_t	maxclients = {"maxclients","8", CVAR_SERVERINFO};
cvar_t	maxspectators = {"maxspectators","8", CVAR_SERVERINFO};
cvar_t	deathmatch = {"deathmatch","1", CVAR_SERVERINFO};			// 0, 1, or 2
cvar_t	spawn = {"spawn","0"};
cvar_t	watervis = {"watervis", "0", CVAR_SERVERINFO};

cvar_t	serverdemo = {"serverdemo","",CVAR_SERVERINFO | CVAR_ROM}; // Highlander

cvar_t	hostname = {"hostname","unnamed", CVAR_SERVERINFO};

// Archi ->
cvar_t	sv_logfname = {"sv_logfname", "qconsole"};
cvar_t	sv_fraglogfname = {"sv_fraglogfname", "frag"};
cvar_t	sv_errorlogfname = {"sv_errorlogfname", "qerror"};
cvar_t	sv_rconlogfname = {"sv_rconlogfname", "rcon"};

cvar_t	sv_logprefix = {"sv_logprefix", ""};
cvar_t	sv_fraglogprefix = {"sv_fraglogprefix", ""};
cvar_t	sv_errorlogprefix = {"sv_errorlogprefix", ""};
cvar_t	sv_rconlogprefix = {"sv_rconlogprefix", ""};

cvar_t	sv_logflush = {"sv_logflush", "1"};

cvar_t	scriptcmdallowed = {"scriptcmdallowed", "0", CVAR_SERVERCFGCONST};

logopts_t	sv_logopts[SV_MAXLOGS];
// <-- Archi

// Highlander -->
qboolean sv_error = 0;
// <-- Highlander

void SV_AcceptClient (netadr_t adr, int userid, char *userinfo);
void Master_Shutdown (void);


// BorisU -->
#ifdef QW262
qboolean IsMDserver=true;
#else
qboolean IsMDserver=false;
#endif
// <-- BorisU

qboolean server_cfg_done = false;


//============================================================================

qboolean ServerPaused(void)
{
	return sv.paused;
}

/*
================
SV_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (void)
{
	int i;

	Master_Shutdown ();

	if (sv.demorecording)
		SV_Stop_f();

	IO_Shutdown();

	NET_Shutdown ();

	for (i = 0; i < SV_MAXLOGS; i++)
		if (sv_logopts[i].f)
		{
			fclose(sv_logopts[i].f);
			sv_logopts[i].f = NULL;
		}
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void SV_Error (char *error, ...)
{
	va_list		argptr;
	static	char		string[1024];
	static	qboolean inerror = false;

	sv_error = true;
	
	if (inerror)
		Sys_Error ("SV_Error: recursively entered (%s)", string);

	inerror = true;

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	Con_Printf ("SV_Error: %s\n",string);

	SV_FinalMessage (va("server crashed: %s\n", string));
		
	SV_Shutdown ();

	Sys_Error ("SV_Error: %s\n",string);
}

/*
==================
SV_FinalMessage

Used by SV_Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message)
{
	int			i;
	client_t	*cl;
	
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_print);
	MSG_WriteByte (&net_message, PRINT_HIGH);
	MSG_WriteString (&net_message, message);
	MSG_WriteByte (&net_message, svc_disconnect);

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
		if (cl->state >= cs_spawned)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);
}



/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/

void SV_DropClient (client_t *drop)
{
	// add the disconnect
#ifdef USE_PR2
        if( drop->isBot )
        {
			extern void RemoveBot(client_t *cl);
			RemoveBot(drop);
			return;  
        }
#endif
	MSG_WriteByte(&drop->netchan.message, svc_disconnect);

	if (drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
		pr_global_struct->self = EDICT_TO_PROG(drop->edict);
		PR_GameClientDisconnect(drop->spectator);
	}

	if (drop->spectator)
		Con_Printf("Spectator %s removed\n",drop->name);
	else
		Con_Printf("Client %s removed\n",drop->name);

	if (drop->download)
	{
		fclose(drop->download);
		drop->download = NULL;
	}

	if (drop->upload)
	{
		fclose(drop->upload);
		drop->upload = NULL;
	}

	*drop->uploadfn = 0;

	drop->state = cs_zombie;		// become free in a few seconds
	drop->connection_started = svs.realtime;	// for zombie timeout


// MD -->
	if (drop == WatcherId) 
		WatcherId = NULL; 
	drop->sendImportantCommand = 0; 
	drop->detector = NULL;
// <-- MD

	drop->old_frags = 0;
	drop->edict->v.frags = 0;
	drop->name[0] = 0;
	memset(drop->userinfo, 0, sizeof(drop->userinfo));
	memset(drop->userinfoshort, 0, sizeof(drop->userinfoshort));
	
// send notification to all remaining clients
	SV_FullClientUpdate(drop, &sv.reliable_datagram);

	SV_AutoStopRecord();
}


//====================================================================

/*
===================
SV_CalcPing

===================
*/
int SV_CalcPing (client_t *cl)
{
	float		ping;
	int			i;
	int			count;
	register	client_frame_t *frame;

	ping = 0;
	count = 0;
#ifdef USE_PR2
	if( cl->isBot )
		return sv_mintic.value*1000;
#endif
	for (frame = cl->frames, i=0 ; i<UPDATE_BACKUP ; i++, frame++)
	{
		if (frame->ping_time > 0)
		{
			ping += frame->ping_time;
			count++;
		}
	}
	if (!count)
		return 9999;
	ping /= count;

	return ping*1000;
}

/*
===================
SV_FullClientUpdate

Writes all update values to a sizebuf
===================
*/
void SV_FullClientUpdate (client_t *client, sizebuf_t *buf)
{
	int		i;
	char	info[MAX_INFO_STRING];

	i = client - svs.clients;

//Sys_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	MSG_WriteByte (buf, svc_updatefrags);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, client->old_frags);
	
	MSG_WriteByte (buf, svc_updateping);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, SV_CalcPing (client));
	
	MSG_WriteByte (buf, svc_updatepl);
	MSG_WriteByte (buf, i);
	MSG_WriteByte (buf, client->lossage);
	
	MSG_WriteByte (buf, svc_updateentertime);
	MSG_WriteByte (buf, i);
	MSG_WriteFloat (buf, svs.realtime - client->connection_started);

	strcpy (info, client->userinfoshort);
	Info_RemovePrefixedKeys (info, '_');	// server passwords, etc

	MSG_WriteByte (buf, svc_updateuserinfo);
	MSG_WriteByte (buf, i);
	MSG_WriteLong (buf, client->userid);
	MSG_WriteString (buf, info);
}

/*
===================
SV_FullClientUpdateToClient

Writes all update values to a client's reliable stream
===================
*/
void SV_FullClientUpdateToClient (client_t *client, client_t *cl)
{
	ClientReliableCheckBlock(cl, 24 + strlen(client->userinfo));
	if (cl->num_backbuf) {
		SV_FullClientUpdate (client, &cl->backbuf);
		ClientReliable_FinishWrite(cl);
	} else
		SV_FullClientUpdate (client, &cl->netchan.message);
}

//Returns a unique userid in 1..SV_MAX_USERID range
#define SV_MAX_USERID 1000

int SV_GenerateUserID (void)
{
	int i;
	client_t *cl;

	do {
		svs.lastuserid++;
		if (svs.lastuserid == SV_MAX_USERID)
			svs.lastuserid = 1;
		for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
			if (cl->state != cs_free && cl->userid == svs.lastuserid)
				break;
	} while (i != MAX_CLIENTS);
	
	return svs.lastuserid;
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/

qboolean qpluginCommand=false;
void SVC_Status (void)
{
	int		i;
	client_t	*cl;
	int		ping;
	int		top, bottom;

	Cmd_TokenizeString ("status");
	SV_BeginRedirect (RD_PACKET);
	Con_Printf ("%s\n", svs.info);
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state >= cs_preconnected) && !cl->spectator)
		{
			top = atoi(Info_ValueForKey (cl->userinfo, "topcolor"));
			bottom = atoi(Info_ValueForKey (cl->userinfo, "bottomcolor"));
			top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
			bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
			ping = SV_CalcPing (cl);
			Con_Printf ("%i %i %i %i \"%s\" \"%s\" %i %i\n", cl->userid, 
				cl->old_frags, (int)(svs.realtime - cl->connection_started)/60,
				ping, cl->name, Info_ValueForKey (cl->userinfo, "skin"), top, bottom);
		}
	}
//	qpluginCommand=true;
	SV_EndRedirect ();
//	qpluginCommand=false;
}

/*
===================
SV_CheckLog

===================
*/
#define	LOG_HIGHWATER	4096
#define	LOG_FLUSH		10*60
void SV_CheckLog (void)
{
	sizebuf_t	*sz;

	sz = &svs.log[svs.logsequence&1];

	// bump sequence if allmost full, or ten minutes have passed and
	// there is something still sitting there
	if (sz->cursize > LOG_HIGHWATER
	|| (svs.realtime - svs.logtime > LOG_FLUSH && sz->cursize) )
	{
		// swap buffers and bump sequence
		svs.logtime = svs.realtime;
		svs.logsequence++;
		sz = &svs.log[svs.logsequence&1];
		sz->cursize = 0;
		Con_Printf ("beginning fraglog sequence %i\n", svs.logsequence);
	}

}

/*
================
SVC_Log

Responds with all the logged frags for ranking programs.
If a sequence number is passed as a parameter and it is
the same as the current sequence, an A2A_NACK will be returned
instead of the data.
================
*/
void SVC_Log (void)
{
	int		seq;
	char	data[MAX_DATAGRAM+64];

	if (Cmd_Argc() == 2)
		seq = atoi(Cmd_Argv(1));
	else
		seq = -1;

	if (seq == svs.logsequence-1 || !sv_logopts[SV_FRAGLOG].f)
	{	// they allready have this data, or we aren't logging frags
		data[0] = A2A_NACK;
		NET_SendPacket (1, data, net_from);
		return;
	}

	Con_DPrintf ("sending log %i to %s\n", svs.logsequence-1, NET_AdrToString(net_from));

	sprintf (data, "stdlog %i\n", svs.logsequence-1);
	strcat (data, (char *)svs.log_buf[((svs.logsequence-1)&1)]);

	NET_SendPacket (strlen(data)+1, data, net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	char	data;

	data = A2A_ACK;

	NET_SendPacket (1, &data, net_from);
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = (rand() << 16) ^ rand();
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = svs.realtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (net_from, "%c%i", S2C_CHALLENGE, 
			svs.challenges[i].challenge);
}

// BorisU -->
#ifdef QW262
qboolean IsOldClient(netadr_t addr)
{
	int			i;
	client_t	*cl;

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++) {
		if (cl->state != cs_free && NET_CompareAdr(addr, cl->netchan.remote_address)) {
			if (cl->old_client) {
				return true;
			} else {
				return false;
			}
		}
	}
	return false;// should not be reached?
}
#endif
// <-- BorisU

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
#ifdef USE_PR2
extern char clientnames[MAX_CLIENTS][CLIENT_NAME_LEN];
//int     userid;
#endif
void SVC_DirectConnect (void)
{
	char		userinfo[1024];
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	edict_t		*ent;
	int			edictnum;
	char		*s;
	int			clients, spectators;
	qboolean	spectator;
	int			qport;
	int			version;
	int			challenge;
	qboolean	old_client = false;
	extern	char *shortinfotbl[];

	version = atoi(Cmd_Argv(1));
#ifdef QW262
	if (version != PROTOCOL_VERSION) {
		if (allow_old_clients.value && version == OLD_PROTOCOL_VERSION) {
			old_client = true;
		} else 
#else
		if (version != OLD_PROTOCOL_VERSION) {
#endif
		{
			Netchan_OutOfBandPrint (net_from, "%c\nServer is version %4.2f.\n", A2C_PRINT, VERSION);
			Con_Printf ("* rejected connect from version %i\n", version);
			return;
		}
	}

	qport = atoi(Cmd_Argv(2));

	challenge = atoi(Cmd_Argv(3));

	// note an extra byte is needed to replace spectator key
	strlcpy (userinfo, Cmd_Argv(4), sizeof(userinfo)-1);
	
	// see if the challenge is valid
	for (i=0 ; i<MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
		{
			if (challenge == svs.challenges[i].challenge)
				break;		// good
			Netchan_OutOfBandPrint (net_from, "%c\nBad challenge.\n", A2C_PRINT);
			return;
		}
	}
	if (i == MAX_CHALLENGES)
	{
		Netchan_OutOfBandPrint (net_from, "%c\nNo challenge for address.\n", A2C_PRINT);
		return;
	}

	// check for password or spectator_password
	s = Info_ValueForKey (userinfo, "spectator");
	if (s[0] && strcmp(s, "0"))
	{
		if (spectator_password.string[0] && 
			Q_stricmp(spectator_password.string, "none") &&
			strcmp(spectator_password.string, s) )
		{	// failed
			Con_Printf ("%s:spectator password failed\n", NET_AdrToString (net_from));
			Netchan_OutOfBandPrint (net_from, "%c\nrequires a spectator password\n\n", A2C_PRINT);
			return;
		}
		Info_RemoveKey (userinfo, "spectator"); // remove passwd
		Info_SetValueForStarKey (userinfo, "*spectator", "1", MAX_INFO_STRING);
		spectator = true;
	}
	else
	{
		s = Info_ValueForKey (userinfo, "password");
		if (password.string[0] && 
			Q_stricmp(password.string, "none") &&
			strcmp(password.string, s) )
		{
			Con_Printf ("%s:password failed\n", NET_AdrToString (net_from));
			Netchan_OutOfBandPrint (net_from, "%c\nserver requires a password\n\n", A2C_PRINT);
			return;
		}
		spectator = false;
		Info_RemoveKey (userinfo, "password"); // remove passwd
	}

	adr = net_from;
	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netchan.remote_address)
			&& ( cl->netchan.qport == qport
			|| adr.port == cl->netchan.remote_address.port ))
		{
			if (cl->state == cs_connected || cl->state == cs_preconnected) {
				Con_Printf("%s:dup connect\n", NET_AdrToString (adr));
//				userid--;
				return;
			}

			Con_Printf ("%s:reconnect\n", NET_AdrToString (adr));
			if (cl->state == cs_spawned) {
				SV_DropClient (cl);
				SV_ClearReliable (cl);	// don't send the disconnect
			}
			cl->state = cs_free;
			break;
		}
	}

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	newcl = NULL;
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free) {
			if (!newcl)
				newcl = cl;		// grab first available slot
			continue;
		}
		if (cl->spectator)
			spectators++;
		else
			clients++;
	}

	// if at server limits, refuse connection
	if ( maxclients.value > MAX_CLIENTS )
		Cvar_SetValue (&maxclients, MAX_CLIENTS);
	if (maxspectators.value > MAX_CLIENTS)
		Cvar_SetValue (&maxspectators, MAX_CLIENTS);
	if (maxspectators.value + maxclients.value > MAX_CLIENTS)
		Cvar_SetValue (&maxspectators, MAX_CLIENTS - maxspectators.value + maxclients.value);
	if ( (spectator && spectators >= (int)maxspectators.value)
		|| (!spectator && clients >= (int)maxclients.value)
		|| !newcl)
	{
		Con_Printf ("%s:full connect\n", NET_AdrToString (adr));
		Netchan_OutOfBandPrint (adr, "%c\nserver is full\n\n", A2C_PRINT);
		return;
	}

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	memset (newcl, 0, sizeof(*newcl));
	newcl->userid = SV_GenerateUserID();
	newcl->old_client = old_client; // BorisU

	strlcpy (newcl->userinfo, userinfo, sizeof(newcl->userinfo));

	Netchan_OutOfBandPrint (adr, "%c", S2C_CONNECTION );

	Netchan_Setup (&newcl->netchan , adr, qport);

	newcl->state = cs_preconnected;

	newcl->datagram.allowoverflow = true;
	newcl->datagram.data = newcl->datagram_buf;
	newcl->datagram.maxsize = sizeof(newcl->datagram_buf);

	// spectator mode can ONLY be set at join time
	newcl->spectator = spectator;

	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	ent->free = false;
	newcl->edict = ent;
	
#ifdef USE_PR2
//restore pointer to client name
//for -progtype 0 (VM_NONE) names stored in clientnames array
//for -progtype 1 (VM_NAITVE) and -progtype 2 (VM_BYTECODE)  stored in mod memory
        if(sv_vm)
        {
          newcl->name = PR2_GetString(ent->v.netname);
        }else
        {
          newcl->name = clientnames[edictnum-1];
        }
        memset(newcl->name, 0, CLIENT_NAME_LEN);
#endif
	// parse some info from the info strings
	SV_ExtractFromUserinfo (newcl, true);

	for (i = 0; shortinfotbl[i] != NULL; i++)
	{
		s = Info_ValueForKey(newcl->userinfo, shortinfotbl[i]);
		Info_SetValueForStarKey (newcl->userinfoshort, shortinfotbl[i], s, MAX_INFO_STRING);
	}

	// move star keys to infoshort
	Info_CopyStarKeys(newcl->userinfo, newcl->userinfoshort);
	
	// JACK: Init the floodprot stuff.
	for (i=0; i<10; i++)
		newcl->whensaid[i] = 0.0;
	newcl->whensaidhead = 0;
	newcl->lockedtill = 0;

	newcl->disable_updates_stop = -1.0; // Vladis

	newcl->logincount = 0;	// bliP

	// call the progs to get default spawn parms for the new client
	PR_GameSetNewParms();
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		newcl->spawn_parms[i] = (&pr_global_struct->parm1)[i];

	if (newcl->spectator)
		Con_Printf ("Spectator %s connected\n", newcl->name);
	else
		Con_DPrintf ("Client %s connected\n", newcl->name);
	newcl->sendinfo = true;
}

int Rcon_Validate (void)
{
	if ( !*rcon_password.string && !master_password[0])
		return 0;

	if (!strcmp (Cmd_Argv(1), bot_password)) {
		sv_cmd = SV_CMD_BOT;
		return 1;
	}

	if (!strcmp (Cmd_Argv(1), master_password)){
		sv_cmd = SV_CMD_MASTER;
		return 1;
	}

	if (!strcmp (Cmd_Argv(1), rcon_password.string)) {
		sv_cmd = SV_CMD_RCON;
		return 1;
	}

	return 0;
}

// mvdsv -->
int Master_Rcon_Validate (void)
{
	if (!master_password[0])
		return 0;

	if (!strcmp (Cmd_Argv(1), master_password)) {
		return 1;
	} else
		return 0;
}
// <-- mvdsv

// Added by -=MD=-
void SV_Admin_f(void)
{
	client_t *cl;
	int i=0;
	
	if(Cmd_Argc()==2 && !Q_stricmp(Cmd_Argv(1),"off") && WatcherId && 
		NET_CompareAdr(WatcherId->netchan.remote_address, net_from) ) {
		Con_Printf("Rcon Watch stopped\n");
		WatcherId=NULL;
		return;
	}

	if(WatcherId)
		Con_Printf("Rcon Watch is already being made by %s\n",WatcherId->name);
	else 
	{
		for(cl=svs.clients;i<MAX_CLIENTS;i++,cl++) {
			if (cl->state != cs_spawned)
				continue;
			if(NET_CompareAdr(cl->netchan.remote_address, net_from)) break;
		}
		if(i==MAX_CLIENTS) {
			Con_Printf("You are not connected to server!\n"); 
			return;
		}
		WatcherId=cl;
		Con_Printf("Rcon Watch started for %s\n",cl->name);
	}
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/

void SVC_RemoteCommand (void)
{
	int	i;
	char	remote_command[1024], masked_data[1024], *p, *d;

	RconerName();

	remote_command[0] = '\0';
	for (i=2 ; i<Cmd_Argc() ; i++) {
		strlcat (remote_command, Cmd_Argv(i), sizeof(remote_command));
		strlcat (remote_command, " ", sizeof(remote_command));
	}

	d = net_message.data + 4;
	p = strchr(d, ' ');
	if (p)
	{
		p++;
		strlcpy(masked_data, d, p - d + 1);

		strlcat(masked_data, "<password>", sizeof(masked_data));

		d = p;
		p = strchr(d, ' ');
		if (p)
			strlcat(masked_data, p, sizeof(masked_data));
		else
			strcpy(masked_data, "Bad rcon command data");
	}
	else
		strcpy(masked_data, "Bad rcon command data");

	if (!IsBot)
		Log_Printf(SV_RCONLOG, "cmd=%s IP=%s", net_message.data+4, NET_AdrToString (net_from));

	if (!Rcon_Validate ()) {
		if (sv_logopts[SV_RCONLOG].f && !IsBot) {
			if (RconNameBuf[0])
				Log_Printf(SV_RCONLOG, " Nick=%s BAD RCON!\n", RconNameBuf);
			else
				Log_Printf(SV_RCONLOG, " BAD RCON!\n");
		}

		Con_Printf ("Bad rcon from %s:\n%s\n"
			, NET_AdrToString (net_from), masked_data);
		if (RconNameBuf[0])
				Con_Printf("Rconer Name: %s\n", RconNameBuf);

		if(WatcherId && !IsBot &&
			!NET_CompareAdr(WatcherId->netchan.remote_address, net_from) ){
			SV_ClientPrintf(WatcherId,PRINT_HIGH,"Bad rcon from %s\nrcon * %s\n",
			NET_AdrToString (net_from), remote_command);
			if (RconNameBuf[0])
				SV_ClientPrintf(WatcherId,PRINT_HIGH,"Rconer Name: %s\n", RconNameBuf);
		}

		SV_BeginRedirect (RD_PACKET);

		Con_Printf ("Bad rcon_password.\n");

	} else {
		if (!IsBot) {
			if(WatcherId && (sv_cmd != SV_CMD_MASTER) &&
				!NET_CompareAdr(WatcherId->netchan.remote_address, net_from) ) {
				SV_ClientPrintf(WatcherId,PRINT_HIGH,"Rcon from %s\nrcon * %s\n",
				NET_AdrToString (net_from), remote_command);
				if (RconNameBuf[0])
					SV_ClientPrintf(WatcherId,PRINT_HIGH,"Rconer Name: %s\n", RconNameBuf);
			}

			if (sv_logopts[SV_RCONLOG].f) {
				if (RconNameBuf[0])
					Log_Printf(SV_RCONLOG, " Nick=%s\n", RconNameBuf);
				else
					Log_Printf(SV_RCONLOG, "\n");
			}

			Con_Printf ("Rcon from %s:\n%s\n", NET_AdrToString (net_from), masked_data);
			if (RconNameBuf[0])
				Con_Printf("Rconer Name: %s\n", RconNameBuf);
		}

		SV_BeginRedirect (RD_PACKET);

	
		if (!RconNameBuf[0] && sv_cmd == SV_CMD_BOT && !strcmp(Cmd_Argv(2),"RegisterBot")) {
			BotIP = net_from;
		}

		if (Cmd_Argc() > 2) {
			if (sv_announcercon.value && !IsBot)
				SV_BroadcastPrintf (PRINT_HIGH, "\x01RCON command from %d.%d.%d.%d: %s\n", 
				net_from.ip[0],net_from.ip[1],net_from.ip[2],net_from.ip[3], remote_command);
			Cmd_ExecuteString (remote_command);
		}
	}

	SV_EndRedirect ();

	sv_cmd = SV_CMD_CONSOLE;
	RconNameBuf[0]=0;
}


/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	MSG_BeginReading ();
	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();

	Cmd_TokenizeString (s);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") || ( c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')) )
	{
		SVC_Ping ();
		return;
	}
	if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n') )
	{
		Con_Printf ("A2A_ACK from %s\n", NET_AdrToString (net_from));
		return;
	}
	else if (!strcmp(c,"status"))
	{
		SVC_Status ();
		return;
	}
	else if (!strcmp(c,"log"))
	{
		SVC_Log ();
		return;
	}
	else if (!strcmp(c,"connect"))
	{
		SVC_DirectConnect ();
		return;
	}
	else if (!strcmp(c,"getchallenge"))
	{
		SVC_GetChallenge ();
		return;
	}
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else
		Con_Printf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (net_from), s);
}

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/


typedef struct
{
	unsigned	mask;
	unsigned	compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int			numipfilters;

cvar_t	filterban = {"filterban", "1"};

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			Con_Printf ("Bad filter address: %s\n", s);
			return false;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;
	
	return true;
}


/*
=================
SV_AddIP_f
=================
*/
void SV_AddIP_f (void)
{
	int		i;
	
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Con_Printf ("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}
	
	if (!StringToFilter (Cmd_Argv(1), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void SV_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (!StringToFilter (Cmd_Argv(1), &f))
		return;
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].mask == f.mask
		&& ipfilters[i].compare == f.compare)
		{
			for (j=i+1 ; j<numipfilters ; j++)
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			Con_Printf ("Removed.\n");
			return;
		}
	Con_Printf ("Didn't find %s.\n", Cmd_Argv(1));
}

/*
=================
SV_ListIP_f
=================
*/
void SV_ListIP_f (void)
{
	int		i;
	byte	b[4];

	Con_Printf ("Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		Con_Printf ("%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SV_WriteIP_f (void)
{
	FILE	*f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;

	sprintf (name, "%s/listip.cfg", com_gamedir);

	Con_Printf ("Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f)
	{
		Con_Printf ("Couldn't open %s\n", name);
		return;
	}
	
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		fprintf (f, "addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	fclose (f);
}

/*
=================
SV_SendBan
=================
*/
void SV_SendBan (void)
{
	char		data[128];

	data[0] = data[1] = data[2] = data[3] = 0xff;
	data[4] = A2C_PRINT;
	data[5] = 0;
	strcat (data, "\nbanned.\n");
	
	NET_SendPacket (strlen(data), data, net_from);
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket (void)
{
	int		i;
	unsigned	in; 
	
	in = *(unsigned *)net_from.ip;

	for (i=0 ; i<numipfilters ; i++)
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterban.value;

	return !filterban.value;
}

// mvdsv -->
char *DecodeArgs(char *args)
{
	static char string[1024];
	char *p, key[32], *s, *value, ch;
	extern char chartbl2[256];// defined in pr_cmds.c

	string[0] = 0;
	p = string;

	while (*args) {
		// skip whitespaces
		while (*args && *args <= 32)
			*p++ = *args++;

		if (*args == '\"') {
			do *p++ = *args++; while (*args && *args != '\"'); 
			*p++ = '\"';
			if (*args)
				args++;
		} else if (*args == '@' || *args == '$')
		{
			// get the key and read value from localinfo
			ch = *args;
			s = key;
			args++;
			while (*args > 32)
				*s++ = *args++;
			*s = 0;

			if ((value = Info_ValueForKey (svs.info, key)) == NULL || !*value)
				value = Info_ValueForKey(localinfo, key);

			*p++ = '\"';
			if (ch == '$') {
				if (value) while (*value)
					*p++ = chartbl2[(byte)*value++];
			} else {
				if (value) while (*value)
					*p++ = *value++;
			}
			*p++ = '\"';
		} else while (*args > 32)
			*p++ = *args++;
	}

	*p = 0;

	return string;
}

void SV_Script (const char *gamedir)
{
	char *path, *p;
	extern redirect_t sv_redirected;

	if (Cmd_Argc() < 2) {
		Con_Printf("usage: script <path> [<args>]\n");
		return;
	}

	path = Cmd_Argv(1);

	if (!strncmp(path, "../", 3) || !strncmp(path, "..\\", 3))
		path += 3;

	if (strstr(path,"../") || strstr(path,"..\\")) {
		Con_Printf("invalid path\n");
		return;
	}

	path = Cmd_Argv(1);
	
	p = Cmd_Args();
	while (*p > 32)
		p++;
	while (*p && *p <= 32)
		p++;

	p = DecodeArgs(p);

	if (sv_redirected != RD_MOD)
		Sys_Printf("Running %s.qws\n", path);

	Sys_Script(gamedir, path, va("%d %s",sv_redirected, p));
}

void SV_Script_f (void)
{
	if (!scriptcmdallowed.value)
	{
		Con_Printf("executing scripts disabled\n");
		return;
	}

	SV_Script(com_gamedir);
}
// <-- mvdsv

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets (void)
{
	int		i;
	client_t	*cl;
	qboolean	good;
	int		qport;

	good = false;
	while (NET_GetPacket ())
	{
		if (SV_FilterPacket ())
		{
			SV_SendBan ();	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}
		
		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading ();
		MSG_ReadLong ();		// sequence number
		MSG_ReadLong ();		// sequence number
		qport = MSG_ReadShort () & 0xffff;

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (cl->state == cs_free)
				continue;
#ifdef USE_PR2
			if (cl->isBot)
				continue;
#endif
			if (!NET_CompareBaseAdr (net_from, cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Con_DPrintf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}
			if (Netchan_Process(&cl->netchan))
			{	// this is a valid, sequenced packet, so process it
				svs.stats.packets++;
				good = true;
				cl->send_message = true;	// reply at end of frame
				if (cl->state != cs_zombie)
					SV_ExecuteClientMessage (cl);
			}
			break;
		}
		
		if (i != MAX_CLIENTS)
			continue;
	
		// packet is not from a known client
		//	Con_Printf ("%s:sequenced packet without connection\n"
		// ,NET_AdrToString(net_from));
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in timeout.value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	float	droptime;
	int	nclients;
	
	droptime = svs.realtime - timeout.value;
	nclients = 0;

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
#ifdef USE_PR2
                if( cl->isBot ) 
                     continue;
#endif
		if (cl->state >= cs_preconnected) {
			if (!cl->spectator)
				nclients++;
			if (cl->netchan.last_received < droptime) {
				SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
				SV_DropClient (cl); 
				cl->state = cs_free;	// don't bother with zombie state
			}
		}
		if (cl->state == cs_zombie && 
			svs.realtime - cl->connection_started > zombietime.value)
		{
			cl->state = cs_free;	// can now be reused
		}
	}
	if (sv.paused && !nclients) {
		// nobody left, unpause the server
		SV_TogglePause("Pause released since no players are left.\n");
	}
}

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands (void)
{
	char	*cmd;

	if ((cmd = Sys_ConsoleInput()))
	{
		Cbuf_AddText (cmd);
		Cbuf_AddText ("\n");
	}
}

/*
===================
SV_BoundRate
===================
*/
// mvdsv ->
int SV_BoundRate (qboolean dl, int rate)
{
	if (!rate)
		rate = 2500;
	if (dl)
	{
		if (!sv_maxdownloadrate.value && sv_maxrate.value && rate > sv_maxrate.value)
			rate = sv_maxrate.value;

		if (sv_maxdownloadrate.value && rate > sv_maxdownloadrate.value)
			rate = sv_maxdownloadrate.value;
	} else
		if (sv_maxrate.value && rate > sv_maxrate.value)
			rate = sv_maxrate.value;

	if (rate < 500)
		rate = 500;
	if (rate > 100000)
		rate = 100000;
	return rate;
}
// <-- mvdsv

/*
===================
SV_CheckVars

===================
*/
void SV_CheckVars (void)
{
	static char		*pw, *spw;
	int				v;
	static float	old_maxrate = 0, old_maxdlrate = 0; // Tonik, mvdsv

	if (password.string != pw || spectator_password.string != spw) {
		pw = password.string;
		spw = spectator_password.string;

		v = 0;
		if (pw && pw[0] && strcmp(pw, "none"))
			v |= 1;
		if (spw && spw[0] && strcmp(spw, "none"))
			v |= 2;

		Con_Printf ("Updated needpass.\n");
		if (!v)
			Info_SetValueForKey (svs.info, "needpass", "", MAX_SERVERINFO_STRING);
		else
			Info_SetValueForKey (svs.info, "needpass", va("%i",v), MAX_SERVERINFO_STRING);
	}
// Tonik, mvdsv -->
// check sv_maxrate
	if (sv_maxrate.value != old_maxrate || sv_maxdownloadrate.value != old_maxdlrate ) {
		client_t	*cl;
		int			i;
		char		*val;

		old_maxrate = sv_maxrate.value;
		old_maxdlrate = sv_maxdownloadrate.value;

		for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
		{
			if (cl->state < cs_preconnected)
				continue;

			if (cl->download) {
				val = Info_ValueForKey (cl->userinfo, "drate");
				if (!*val)
					val = Info_ValueForKey (cl->userinfo, "rate");
			} else
				val = Info_ValueForKey (cl->userinfo, "rate");

			cl->netchan.rate = 1.0 / SV_BoundRate (cl->download != NULL, atoi(val));
		}
	}
// <-- Tonik, mvdsv
}

/*
==================
SV_Frame

==================
*/
void SV_Frame (float time)
{
	static double	start, end;
	double	demo_start, demo_end;

	start = Sys_DoubleTime ();
	svs.stats.idle += start - end;

// keep the random time dependent
	rand ();

	curtime += time;

// decide the simulation time
	if (!sv.paused) {
		svs.realtime += time;
		sv.time += time;
	}

	if (sv.paused && unpause_counter && Sys_DoubleTime () > unpause_begin +  6 - unpause_counter) {
		if (unpause_counter == 1) {
			SV_BroadcastPrintf (PRINT_CHAT, "-=< GO! >=-\n", unpause_counter - 1);
			SV_TogglePause(NULL);
		} else {
			SV_BroadcastPrintf (PRINT_CHAT, "-=<  %i  >=-\n", unpause_counter - 1);
		}
		unpause_counter--;
	}


// check timeouts
	SV_CheckTimeouts ();

// toggle the log buffer if full
	SV_CheckLog ();

// move autonomous things around if enough time has passed
	if (!sv.paused)
		SV_Physics ();

// get packets
	if (IO_status & IO_SOCKREAD_AVAIL)
		SV_ReadPackets ();

// check for commands typed to the host
	SV_GetConsoleCommands ();
	
// process console commands
	Cbuf_Execute ();

	SV_CheckVars ();

// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

// Highlander -->
	demo_start = Sys_DoubleTime ();
	SV_SendDemoMessage();
	demo_end = Sys_DoubleTime ();
	svs.stats.demo += demo_end - demo_start;
// <-- Highlander

// send a heartbeat to the master if needed
	Master_Heartbeat ();

// collect timing statistics
	end = Sys_DoubleTime ();
	svs.stats.active += end-start;
	if (++svs.stats.count == STATFRAMES)
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle = svs.stats.idle;
		svs.stats.latched_packets = svs.stats.packets;
		svs.stats.latched_demo = svs.stats.demo;
		svs.stats.active = 0;
		svs.stats.idle = 0;
		svs.stats.packets = 0;
		svs.stats.count = 0;
		svs.stats.demo = 0;
	}
}

/*
===============
SV_InitLocal
===============
*/
void SV_InitLocal (void)
{
	int		i;
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_spectatormaxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_airaccelerate;
	extern	cvar_t	sv_wateraccelerate;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_waterfriction;

	Cvar_Init (); // BorisU

	SV_InitOperatorCommands	();
	SV_UserInit ();
	
	Cvar_RegisterVariable (&rcon_password);
	Cvar_RegisterVariable (&password);
	Cvar_RegisterVariable (&spectator_password);

	Cvar_RegisterVariable (&sv_mintic);
	Cvar_RegisterVariable (&sv_maxtic);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&maxclients);
	Cvar_RegisterVariable (&maxspectators);
	Cvar_RegisterVariable (&hostname);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&spawn);
	Cvar_RegisterVariable (&watervis);

	Cvar_RegisterVariable (&developer);

	Cvar_RegisterVariable (&timeout);
	Cvar_RegisterVariable (&zombietime);

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_spectatormaxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_airaccelerate);
	Cvar_RegisterVariable (&sv_wateraccelerate);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_waterfriction);

	Cvar_RegisterVariable (&filterban);
	
	Cvar_RegisterVariable (&allow_download);
	Cvar_RegisterVariable (&allow_download_skins);
	Cvar_RegisterVariable (&allow_download_models);
	Cvar_RegisterVariable (&allow_download_sounds);
	Cvar_RegisterVariable (&allow_download_maps);
	Cvar_RegisterVariable (&allow_download_demos);
	
	Cvar_RegisterVariable (&sv_highchars);

	Cvar_RegisterVariable (&sv_phs);

	Cvar_RegisterVariable (&pausable);

	Cvar_RegisterVariable (&sv_nailhack);		// Tonik
	Cvar_RegisterVariable (&sv_loadentfiles);	// Tonik
	Cvar_RegisterVariable (&sv_maxrate);		// BorisU
	Cvar_RegisterVariable (&sv_announcercon);	// BorisU
#ifdef QW262
	Cvar_RegisterVariable (&allow_old_clients);	// BorisU
#endif
	Cvar_RegisterVariable (&sv_newpause);		// BorisU

	Cmd_AddCommand ("admin", SV_Admin_f);	// -=MD=-

	Cmd_AddCommand ("addip", SV_AddIP_f);
	Cmd_AddCommand ("removeip", SV_RemoveIP_f);
	Cmd_AddCommand ("listip", SV_ListIP_f);
	Cmd_AddCommand ("writeip", SV_WriteIP_f);

// Highlander -->
	Cvar_RegisterVariable (&serverdemo);

	Cvar_RegisterVariable (&sv_demoMaxSizeOverwrite);

	Cmd_AddCommand ("record", SV_Record_f);
	Cmd_AddCommand ("easyrecord", SV_EasyRecord_f);
	Cmd_AddCommand ("stop", SV_Stop_f);
	Cmd_AddCommand ("cancel", SV_Cancel_f);
	Cmd_AddCommand ("demolist", SV_DemoList_f);
	Cmd_AddCommand ("rmdemo", SV_DemoRemove_f);
	Cmd_AddCommand ("rmdemonum", SV_DemoRemoveNum_f);
	Cmd_AddCommand ("script", SV_Script_f);
	Cmd_AddCommand ("demoinfoadd", SV_DemoInfoAdd_f);
	Cmd_AddCommand ("demoinforemove", SV_DemoInfoRemove_f);
	Cmd_AddCommand ("demoinfo", SV_DemoInfo_f);
// <-- Highlander

	Cvar_RegisterVariable (&sv_maxdownloadrate); // mvdsv

// Archi -->
	Cvar_RegisterVariable (&sv_logfname);
	Cvar_RegisterVariable (&sv_fraglogfname);
	Cvar_RegisterVariable (&sv_errorlogfname);
	Cvar_RegisterVariable (&sv_rconlogfname);

	Cvar_RegisterVariable (&sv_logprefix);
	Cvar_RegisterVariable (&sv_fraglogprefix);
	Cvar_RegisterVariable (&sv_errorlogprefix);
	Cvar_RegisterVariable (&sv_rconlogprefix);

	Cvar_RegisterVariable (&sv_logflush);
// <-- Archi

	for (i=0 ; i<MAX_MODELS ; i++)
		sprintf (localmodels[i], "*%i", i);

	Info_SetValueForStarKey (svs.info, "*version", va("%4.2f", VERSION), MAX_SERVERINFO_STRING);
	Info_SetValueForStarKey (svs.info, "*262ver", va("rev.%d %s",MINOR_VERSION, RELEASE), MAX_SERVERINFO_STRING);

	// init fraglog stuff
	svs.logsequence = 1;
	svs.logtime = svs.realtime;
	svs.log[0].data = svs.log_buf[0];
	svs.log[0].maxsize = sizeof(svs.log_buf[0]);
	svs.log[0].cursize = 0;
	svs.log[0].allowoverflow = true;
	svs.log[1].data = svs.log_buf[1];
	svs.log[1].maxsize = sizeof(svs.log_buf[1]);
	svs.log[1].cursize = 0;
	svs.log[1].allowoverflow = true;
}


//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void)
{
	char		string[256];
	int			active;
	int			i;

	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS)
		return;		// not time to send yet

	svs.last_heartbeat = svs.realtime;

	//
	// count active users
	//
	active = 0;
	for (i=0 ; i<MAX_CLIENTS ; i++)
		if (svs.clients[i].state == cs_connected ||
		svs.clients[i].state == cs_spawned )
			active++;

	svs.heartbeat_sequence++;
	sprintf (string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
		svs.heartbeat_sequence, active);


	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Con_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
			qpluginCommand=true;
			NET_SendPacket (strlen(string), string, master_adr[i]);
			qpluginCommand=false;
		}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{
	char		string[2];
	int			i;

	string[0]=S2M_SHUTDOWN;
	string[1]='\n';

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Con_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
//			qpluginCommand=true;
			NET_SendPacket (2, string, master_adr[i]);
//			qpluginCommand=false;
		}
}

/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_ExtractFromUserinfo (client_t *cl, qboolean namechanged)
{
	char	*val, *p, *q;
	int		i;
	client_t	*client;
	int		dupc = 1;
	char	newname[80];


	if (namechanged) {
		// Detect for cheater's client
		// BorisU - for 262 server only
		i = *Info_ValueForKey(cl->userinfo,"pmodel");
		if(IsMDserver && !cl->old_client && ( i || *Info_ValueForKey(cl->userinfo,"emodel") )) {
		//	SV_BroadcastPrintf(PRINT_HIGH,"Cheater here\n",Info_ValueForKey (cl->userinfo, "name"));
			Cmd_TokenizeString(va("addip %d.%d.%d.%d",cl->netchan.remote_address.ip[0],cl->netchan.remote_address.ip[1],cl->netchan.remote_address.ip[2],cl->netchan.remote_address.ip[3]));
			SV_AddIP_f();
		}

		// name for C code
		val = Info_ValueForKey (cl->userinfo, "name");

		// trim user name
		strlcpy (newname, val, sizeof(newname));

		for (p = newname; (*p == ' ' || *p == '\r' || *p == '\n') && *p; p++)
			;

		if (p != newname && !*p) {
			//white space only
			strcpy(newname, "unnamed");
			p = newname;
		}

		if (p != newname && *p) {
			for (q = newname; *p; *q++ = *p++)
				;
			*q = 0;
		}
		for (p = newname + strlen(newname) - 1; p != newname && (*p == ' ' || *p == '\r' || *p == '\n') ; p--)
			;
		p[1] = 0;

		if (strcmp(val, newname)) {
			Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
			val = Info_ValueForKey (cl->userinfo, "name");
		}

		if (!val[0] || (!Q_stricmp(val, "console") || !Q_stricmp(val, "RconBot"))) {
			Info_SetValueForKey (cl->userinfo, "name", "unnamed", MAX_INFO_STRING);
			val = Info_ValueForKey (cl->userinfo, "name");
		}

		// check to see if another user by the same name exists
		while (1) {
			for (i=0, client = svs.clients ; i<MAX_CLIENTS ; i++, client++) {
				if (client->state != cs_spawned || client == cl)
					continue;
				if (!Q_stricmp(client->name, val))
					break;
			}
			if (i != MAX_CLIENTS) { // dup name

				if (strlen(val) > CLIENT_NAME_LEN - 1)
					val[CLIENT_NAME_LEN - 4] = 0;
				p = val;

				if (val[0] == '(') {
					if (val[2] == ')')
						p = val + 3;
					else if (val[3] == ')')
						p = val + 4;
				}

				sprintf(newname, "(%d)%-.40s", dupc++, p);
				Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
				val = Info_ValueForKey (cl->userinfo, "name");
			} else
				break;
		}
		
		if (strncmp(val, cl->name, strlen(cl->name))) {
			if (!sv.paused) {
				if (!cl->lastnametime || svs.realtime - cl->lastnametime > 5) {
					cl->lastnamecount = 0;
					cl->lastnametime = svs.realtime;
				} else if (cl->lastnamecount++ > 4) {
					SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked for name spam\n", cl->name);
					SV_ClientPrintf (cl, PRINT_HIGH, "You were kicked from the game for name spamming\n");
					SV_DropClient (cl); 
					return;
				}
			}
					
			if (cl->state >= cs_spawned && !cl->spectator)
				SV_BroadcastPrintf (PRINT_HIGH, "%s changed name to %s\n", cl->name, val);
		}

			strlcpy (cl->name, val, CLIENT_NAME_LEN);

	}

	// rate command
	if (cl->download) {
		val = Info_ValueForKey (cl->userinfo, "drate");
		if (!atoi(val))
			val = Info_ValueForKey (cl->userinfo, "rate");
	} else
		val = Info_ValueForKey (cl->userinfo, "rate");
	cl->netchan.rate = 1.0 / SV_BoundRate (cl->download != NULL, atoi(val));


	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

}


//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet (void)
{
	int	port;
	int	p;

	port = PORT_SERVER;
	p = COM_CheckParm ("-port");
	if (p && p < com_argc)
	{
		port = atoi(com_argv[p+1]);
		Con_Printf ("Port: %i\n", port);
	}
	NET_Init (port);

	Netchan_Init ();

	// heartbeats will allways be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
}


/*
====================
SV_Init
====================
*/
void SV_Init(quakeparms_t *parms)
{
#ifndef _WIN32
	int i;
#endif

	COM_InitArgv(parms->argc, parms->argv);
	COM_AddParm("-game");
	COM_AddParm("qw");

	if (COM_CheckParm("-minmemory"))
		parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	if (parms->memsize < MINIMUM_MEMORY)
		SV_Error("Only %4.1f megs of memory reported, can't execute game",
			parms->memsize / (float)0x100000);

	Memory_Init(parms->membase, parms->memsize);
	Cbuf_Init();
	Cmd_Init();	

	COM_Init();

	PR_Init();
	
	Mod_Init();

	SV_InitNet();

	SV_InitLocal();
	Sys_Init();
	PM_Init();

	Demo_Init(); // Highlander

	Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark();

	Cbuf_InsertText("exec server.cfg\n");

	host_initialized = true;
	
	Con_Printf("Exe: "__TIME__" "__DATE__"\n");
	Con_Printf("%4.1f megabyte heap\n",
		(float)parms->memsize / (1024 * 1024));	

	Con_Printf("\nServer Version %4.2f\n  by [2WP]BorisU (rev.%d %s)\n"
		"http://2wp.vei.ru/qw262\n\n", VERSION, MINOR_VERSION, RELEASE);

	Con_Printf("===== QW262 Edition of QuakeWorld ====\n");

// Archi -->
#ifndef _WIN32
	if (COM_CheckParm("-daemon"))
	{
		Sys_Daemonize();
		parms->io_doconsole = 0;
	}

	if ((i = COM_CheckParm("-pidfile")))
		Sys_WritePid(com_argv[i + 1]);
#endif
// <-- Archi

	IO_Init(net_socket, parms->io_doconsole, NET_Sendto,
		parms->io_timeout);

// process command line arguments
	Cmd_StuffCmds_f();
	Cbuf_Execute();

	if (!master_password[0])
		strcpy(master_password, rcon_password.string);

	server_cfg_done  = true;

// if a map wasn't specified on the command line, spawn start.map
	if (sv.state == ss_dead)
		Cmd_ExecuteString("map start");
	if (sv.state == ss_dead)
		SV_Error("Couldn't spawn a server");
}

// mvdsv -->
/*
============
Sys_compare_by functions for sort files in list
============
*/
int Sys_compare_by_date(const void *a, const void *b)
{
	return (int)(((file_t *)a)->time - ((file_t *)b)->time);
}

int Sys_compare_by_name(const void *a, const void *b)
{
	return strncmp(((file_t *)a)->name, ((file_t *)b)->name, MAX_DEMO_NAME);
}
// <-- mvdsv
