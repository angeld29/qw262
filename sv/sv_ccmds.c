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

#include "qwsvdef.h"

qboolean	sv_allow_cheats;

int fp_messages=4, fp_persecond=4, fp_secondsdead=10;
char fp_msg[255] = { 0 };
extern cvar_t cl_warncmd;
extern		redirect_t	sv_redirected;


/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Make a master server current
====================
*/
extern qboolean qpluginCommand;
void SV_SetMaster_f (void)
{
	char	data[2];
	int		i;

	memset (&master_adr, 0, sizeof(master_adr));

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		if (!strcmp(Cmd_Argv(i), "none") || !NET_StringToAdr (Cmd_Argv(i), &master_adr[i-1]))
		{
			Con_Printf ("Setting nomaster mode.\n");
			return;
		}
		if (master_adr[i-1].port == 0)
			master_adr[i-1].port = BigShort (27000);

		Con_Printf ("Master server at %s\n", NET_AdrToString (master_adr[i-1]));

		Con_Printf ("Sending a ping.\n");

		data[0] = A2A_PING;
		data[1] = 0;
//		qpluginCommand=true;
		NET_SendPacket (2, data, master_adr[i-1]);
//		qpluginCommand=false;
	}

	svs.last_heartbeat = -99999;
}


/*
==================
SV_Quit_f
==================
*/
void SV_Quit_f (void)
{
	SV_FinalMessage ("server shutdown\n");
	Con_Printf ("Shutting down.\n");
	SV_Shutdown ();
	Sys_Quit ();
}


#ifdef QW262
#include "sv_ccmds262.inc"
#endif

/*
============
SV_Logfile_f
============
*/
void SV_Logfile_f (void)
{
	char	name[MAX_OSPATH];

	if (sv_logopts[SV_CONSOLELOG].f)
	{
		Con_Printf ("File logging off.\n");
		fclose (sv_logopts[SV_CONSOLELOG].f);
		sv_logopts[SV_CONSOLELOG].f = NULL;
		return;
	}

	sprintf (name, "%s/%s.log", com_gamedir, sv_logfname.string);
	Con_Printf ("Logging text to %s.\n", name);
	sv_logopts[SV_CONSOLELOG].f = fopen (name, "a");
	if (!sv_logopts[SV_CONSOLELOG].f)
		Con_Printf ("failed.\n");

	sv_logopts[SV_CONSOLELOG].flags = SV_LOGFLAGS_NEWLINE;
	sv_logopts[SV_CONSOLELOG].prefix = sv_logprefix.string[0] != '\0' ?
		sv_logprefix.string : NULL;
}

/*
============
SV_ErrorLogfile_f
============
*/
void SV_ErrorLogfile_f (void)
{
	char	name[MAX_OSPATH];

	if (sv_logopts[SV_ERRORLOG].f)
	{
		Con_Printf ("Error logging off.\n");
		fclose (sv_logopts[SV_ERRORLOG].f);
		sv_logopts[SV_ERRORLOG].f = NULL;
		return;
	}

	sprintf (name, "%s/%s.log", com_gamedir, sv_errorlogfname.string);
	Con_Printf ("Logging errors to %s.\n", name);
	sv_logopts[SV_ERRORLOG].f = fopen (name, "a");
	if (!sv_logopts[SV_ERRORLOG].f)
		Con_Printf ("failed.\n");

	sv_logopts[SV_ERRORLOG].flags = SV_LOGFLAGS_NEWLINE;
	sv_logopts[SV_ERRORLOG].prefix = sv_errorlogprefix.string[0] != '\0' ?
		sv_errorlogprefix.string : NULL;
}

/*
============
SV_RconLogfile_f
============
*/
void SV_RconLogfile_f (void)
{
	char	name[MAX_OSPATH];

	if (sv_logopts[SV_RCONLOG].f)
	{
		if (sv_cmd == SV_CMD_RCON) {
			Con_Printf ("You can't set logging off.\n");
			return;
		}
		Con_Printf ("Rcon logging off.\n");
		fclose (sv_logopts[SV_RCONLOG].f);
		sv_logopts[SV_RCONLOG].f = NULL;
		return;
	}

	sprintf (name, "%s/%s.log", com_gamedir, sv_rconlogfname.string);
	Con_Printf ("Logging rcon attemps to %s.\n", name);
	sv_logopts[SV_RCONLOG].f = fopen (name, "a");
	if (!sv_logopts[SV_RCONLOG].f)
		Con_Printf ("failed.\n");

	sv_logopts[SV_RCONLOG].flags = SV_LOGFLAGS_NEWLINE;
	sv_logopts[SV_RCONLOG].prefix = sv_rconlogprefix.string[0] != '\0' ?
		sv_rconlogprefix.string : NULL;
}

/*
============
SV_Fraglogfile_f
============
*/
void SV_Fraglogfile_f (void)
{
	char	name[MAX_OSPATH];
	int		i;
	FILE	*f;
	
	if (sv_logopts[SV_FRAGLOG].f)
	{
		Con_Printf ("Frag file logging off.\n");
		fclose (sv_logopts[SV_FRAGLOG].f);
		sv_logopts[SV_FRAGLOG].f = NULL;
		return;
	}

	// find an unused name
	for (i=0 ; i<1000 ; i++)
	{
		sprintf (name, "%s/%s_%i.log", com_gamedir, sv_fraglogfname.string, i);
		f = fopen (name, "r");
		if (!f)
		{	// can't read it, so create this one
			f = fopen (name, "w");
			if (!f)
				i=1000;	// give error
			else
			{
				sv_logopts[SV_FRAGLOG].f = f;
				sv_logopts[SV_FRAGLOG].flags =
					SV_LOGFLAGS_NEWLINE;
				sv_logopts[SV_FRAGLOG].prefix =
					sv_fraglogprefix.string[0] != '\0' ?
					sv_fraglogprefix.string : NULL;
			}

			break;
		}
		fclose (f);
	}
	if (i==1000)
		Con_Printf ("Can't open any logfiles.\n");
	else
		Con_Printf ("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;

	idnum = atoi(Cmd_Argv(1));

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->edict;
			return true;
		}
	}
	Con_Printf ("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
void SV_God_f (void)
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE) )
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode OFF\n");
	else
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode ON\n");
}


void SV_Noclip_f (void)
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip OFF\n");
	}
}


/*
==================
SV_Give_f
==================
*/
void SV_Give_f (void)
{
	char	*t;
	int		v;
	
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}
	
	if (!SV_SetPlayer ())
		return;

	t = Cmd_Argv(2);
	v = atoi (Cmd_Argv(3));
	
	switch (t[0])
	{
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		sv_player->v.items = (int)sv_player->v.items | IT_SHOTGUN<< (t[0] - '2');
		break;
	
	case 's':
		sv_player->v.ammo_shells = v;
		break;		
	case 'n':
		sv_player->v.ammo_nails = v;
		break;		
	case 'r':
		sv_player->v.ammo_rockets = v;
		break;		
	case 'h':
		sv_player->v.health = v;
		break;		
	case 'c':
		sv_player->v.ammo_cells = v;
		break;		
	}
}


/*
======================
SV_Map_f

handle a 
map <mapname>
command from the console or progs.
======================
*/
void SV_Map_f (void)
{
	char	level[MAX_QPATH];
	char	expanded[MAX_QPATH];
	FILE	*f;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("map <levelname> : continue game on a new level\n");
		return;
	}
	strlcpy (level, Cmd_Argv(1), MAX_QPATH - 10);

	// check to make sure the level exists
	sprintf (expanded, "maps/%s.bsp", level);
	COM_FOpenFile (expanded, &f);
	if (!f)
	{
		Con_Printf ("Can't find %s\n", expanded);
		return;
	}
	fclose (f);

	if (sv.demorecording)
		SV_Stop_f();

	SV_BroadcastCommand ("changing\n");
	SV_SendMessagesToAll ();

	SV_SpawnServer (level);

	SV_BroadcastCommand ("reconnect\n");
}

/*==================
SV_ReplaceChar
Replace char in string
==================*/
void SV_ReplaceChar(char *s, char from, char to)
{
	if (s)
		for ( ;*s ; ++s)
			if (*s == from)
				*s = to;
}

// bliP: ls, rm, rmdir, chmod -->
/*==================
SV_ListFiles_f
Lists files
==================*/
void SV_ListFiles_f (void)
{
	dir_t	dir;
	file_t	*list;
	char	*key;
	char	*dirname;
	int	i;

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't use ls command.\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("ls <directory> <match>\n");
		return;
	}

	dirname = Cmd_Argv(1);
	SV_ReplaceChar(dirname, '\\', '/');

	if (	!strncmp(dirname, "../", 3) || strstr(dirname, "/../") || *dirname == '/'
	||	( (i = strlen(dirname)) < 3 ? 0 : !strncmp(dirname + i - 3, "/..", 4) )
#ifdef _WIN32
	||	( dirname[1] == ':' && (*dirname >= 'a' && *dirname <= 'z' ||
					*dirname >= 'A' && *dirname <= 'Z')
		)
#endif //_WIN32
	   )
	{
		Con_Printf("Unable to list %s\n", dirname);
		return;
	}

	Con_Printf("Content of %s/*.*\n", dirname);
	dir = Sys_listdir(va("%s", dirname), ".*", SORT_BY_NAME);
	list = dir.files;
	if (!list->name[0])
	{
		Con_Printf("No files\n");
		return;
	}

	key = (Cmd_Argc() == 3) ? Cmd_Argv(2) : "";

//directories...
	for (; list->name[0]; list++)
	{
		if (!strstr(list->name, key) || !list->isdir)
			continue;
		Con_Printf("- %s\n", list->name);
	}

	list = dir.files;

//files...
	for (; list->name[0]; list++)
	{
		if (!strstr(list->name, key) || list->isdir)
			continue;
		if ((int)list->size / 1024 > 0)
			Con_Printf("%s %.0fKB (%.2fMB)\n", list->name,
				(float)list->size / 1024, (float)list->size / 1024 / 1024);
		else
			Con_Printf("%s %dB\n", list->name, list->size);
	}
	Con_Printf("Total: %d files, %.0fKB (%.2fMB)\n", dir.numfiles,
		(float)dir.size / 1024, (float)dir.size / 1024 / 1024);
}

/*==================
SV_RemoveDirectory_f
Removes an empty directory
==================*/
void SV_RemoveDirectory_f (void)
{
	char	*dirname;

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't use rmdir command.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf("rmdir <directory>\n");
		return;
	}

	dirname = Cmd_Argv(1);
	SV_ReplaceChar(dirname, '\\', '/');

	if (	!strncmp(dirname, "../", 3) || strstr(dirname, "/../") || *dirname == '/'
#ifdef _WIN32
	||	( dirname[1] == ':' && (*dirname >= 'a' && *dirname <= 'z' ||
					*dirname >= 'A' && *dirname <= 'Z')
		)
#endif //_WIN32
	   )
	{
		Con_Printf("Unable to remove\n");
		return;
	}

	if (!Sys_rmdir(dirname))
		Con_Printf("Directory %s succesfully removed\n", dirname);
	else 
		Con_Printf("Unable to remove directory %s\n", dirname);
}

/*==================
SV_RemoveFile_f
Remove a file
==================*/
void SV_RemoveFile_f (void)
{
	char *dirname;
	char *filename;
	int i;

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't use rm command.\n");
		return;
	}

	if (Cmd_Argc() < 3)
	{
		Con_Printf("rm <directory> {<filename> | *<token> | *} - removes a file | with token | all\n");
		return;
	}

	dirname = Cmd_Argv(1);
	filename = Cmd_Argv(2);
	SV_ReplaceChar(dirname, '\\', '/');
	SV_ReplaceChar(filename, '\\', '/');

	if (	!strncmp(dirname, "../", 3) || strstr(dirname, "/../")
	||	*dirname == '/'             || strchr(filename, '/')
	||	( (i = strlen(filename)) < 3 ? 0 : !strncmp(filename + i - 3, "/..", 4) )
#ifdef _WIN32
	||	( dirname[1] == ':' && (*dirname >= 'a' && *dirname <= 'z' ||
					*dirname >= 'A' && *dirname <= 'Z')
		)
#endif //_WIN32
	   )
	{
		Con_Printf("Unable to remove\n");
		return;
	}

	if (*filename == '*') //token, many files
	{
		dir_t dir;
		file_t *list;

		// remove all files with specified token
		filename++;

		dir = Sys_listdir(va("%s", dirname), ".*", SORT_BY_NAME);
		list = dir.files;
		for (i = 0; list->name[0]; list++)
		{
			if (!list->isdir && strstr(list->name, filename))
			{
				if (!Sys_remove(va("%s/%s", dirname, list->name)))
				{
					Con_Printf("Removing %s...\n", list->name);
					i++;
				}
			}
		}
		if (i)
			Con_Printf("%d files removed\n", i);
		else
			Con_Printf("No matching found\n");
	}
	else // 1 file
	{
		if (!Sys_remove(va("%s/%s", dirname, filename)))
			Con_Printf("File %s succesfully removed\n", filename);
		else
			Con_Printf("Unable to remove file %s\n", filename);
	}
}

//==================
//SV_ChmodFile_f
//Chmod a script
//==================#ifndef _WIN32
//void SV_ChmodFile_f (void)
//{
//	unsigned char	*_mode, *filename;
//	unsigned int	mode, m;
//
//	if (Cmd_Argc() != 3)
//	{
//		Con_Printf("chmod <mode> <file>\n");
//		return;
//	}
//
//	_mode = Cmd_Argv(1);
//	filename = Cmd_Argv(2);
//
//	if (!strncmp(filename, "../",  3) || strstr(filename, "/../") ||
//		*filename == '/'              || strlen(_mode) != 3 ||
//		( (m = strlen(filename)) < 3 ? 0 : !strncmp(filename + m - 3, "/..", 4) ))
//	{
//		Con_Printf("Unable to chmod\n");
//		return;
//	}
//	for (mode = 0; *_mode; _mode++)
//	{
//		m = *_mode - '0';
//		if (m > 7)
//		{
//			Con_Printf("Unable to chmod\n");
//			return;
//		}
//		mode = (mode << 3) + m;
//	}
//
//	if (chmod(filename, mode))
//		Con_Printf("Unable to chmod %s\n", filename);
//	else 
//		Con_Printf("Chmod %s succesful\n", filename);
//}
//#endif //_WIN32
// <-- bliP

void SV_OutOfTimeKick(client_t *cl)
{
#ifdef QW262
//	cl->sendImportantCommand=0;
	if (cl->old_client) return;
	SV_BroadcastPrintf(PRINT_HIGH,"%s was kicked for using suspicious quake client\n",cl->name);
	SV_ClientPrintf(cl,PRINT_HIGH,"\nYou were kicked for ignoring server commands!\nProbably you need to upgrade your client.\n");
	SV_DropClient(cl); 
#endif
}


// Added by -=MD=-
void SV_StuffText_f(void)
{
	int			i;
	client_t	*cl;
	int			userid;
	char		*cmd;

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't use stufftext\n");
		return;
	}

	if(Cmd_Argc()<3)
	{
		Con_Printf("usage: stufftext  <id>  <text>\n");
		return;
	}
	userid = atoi(Cmd_Argv(1));
	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == userid || !strcmp(Info_ValueForKey(cl->userinfo,"uid"),Cmd_Argv(1))) {
			char buf[128];
			cmd=Cmd_Args()+strlen(Cmd_Argv(1))+1;
			strncpy(buf, cmd, 128);
			strcat(buf, "\n");
			ClientReliableWrite_Begin (cl, svc_stufftext, strlen(buf)+1);
			ClientReliableWrite_String(cl, buf);
			return;
		}
	}

	Con_Printf ("Couldn't find user '%s'\n", Cmd_Argv(1));
}

char RconNameBuf[1024] = {0};

void RconerName(void)
{
	client_t	*cl;
	int			i;
	
	RconNameBuf[0]=0;

	IsBot = NET_CompareAdr(BotIP, net_from);

	if (IsBot)
		strcpy(RconNameBuf,"RconBot");
	else
		for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++) {
			if (!cl->state)
				continue;
			if (NET_CompareAdr(cl->netchan.remote_address, net_from)){
				strcpy(RconNameBuf,cl->name);
				return;
			}
		}
	return;
}

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	int			i;
	client_t	*cl;
	int			userid;

	userid = atoi(Cmd_Argv(1));

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (!cl->state)
			continue;

		if (cl->userid == userid || !strcmp(Info_ValueForKey(cl->userinfo,"uid"),Cmd_Argv(1)))
		{
			if (RconNameBuf[0])
				SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked by %s\n", 
					cl->name, RconNameBuf);
			else
				SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SV_BroadcastPrintf message
			SV_ClientPrintf (cl, PRINT_HIGH,"You were kicked from this game\n");
			SV_DropClient (cl); 
			return;
		}
	}

	Con_Printf ("Couldn't find user '%s'\n", Cmd_Argv(1));
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int			i, j, l;
	client_t	*cl;
	float		cpu, demo, avg, pak;
	char		*s;


	cpu = (svs.stats.latched_active+svs.stats.latched_idle);
	if (cpu) {
		demo = 100*svs.stats.latched_demo/cpu;
		cpu = 100*svs.stats.latched_active/cpu;
	} else
		demo = 0;

	avg = 1000*svs.stats.latched_active / STATFRAMES;
	pak = (float)svs.stats.latched_packets / STATFRAMES;

	Con_Printf ("net address                 : %s\n",NET_AdrToString (net_local_adr));
	Con_Printf ("cpu utilization (overall)   : %3i%%\n",(int)cpu);
	Con_Printf ("cpu utilization (recording) : %3i%%\n", (int)demo);
	Con_Printf ("avg response time           : %i ms\n",(int)avg);
	Con_Printf ("packets/frame               : %5.2f (%d)\n", pak, num_prstr);
	
// min fps lat drp
	if (sv_redirected != RD_NONE) {
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		Con_Printf ("name               userid frags\n");
		Con_Printf ("  address          rate ping drop\n");
		Con_Printf ("  ---------------- ---- ---- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;

			Con_Printf ("%-16.16s  ", cl->name);

			Con_Printf ("%6i %5i", cl->userid, (int)cl->edict->v.frags);
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
				Con_Printf("\n");

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("  %-16.16s", s);
			if (cl->state == cs_connected || cl->state == cs_preconnected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %5.2f\n"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence);
		}
	} else {
		Con_Printf ("frags userid address         name            rate ping drop  qport\n");
		Con_Printf ("----- ------ --------------- --------------- ---- ---- ----- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;
			Con_Printf ("%5i %6i ", (int)cl->edict->v.frags,  cl->userid);

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("%s", s);
			l = 16 - strlen(s);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			
			Con_Printf ("%s", cl->name);
			l = 16 - strlen(cl->name);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			if (cl->state == cs_connected  || cl->state == cs_preconnected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %3.1f %4i"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence
				, cl->netchan.qport);
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
				Con_Printf("\n");

				
		}
	}
	Con_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	if (sv_cmd == SV_CMD_RCON)
		return;

	if (IsBot)
		strcpy (text, "RconBot: ");
	else
		strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf2(client, PRINT_CHAT, "%s\n", text);
	}

	if (sv.demorecording) {
		DemoWrite_Begin (dem_all, 0, strlen(text)+3);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, svc_print);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, PRINT_CHAT);
		MSG_WriteString ((sizebuf_t*)demo.dbuf, text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999;
}

void SV_SendServerInfoChange(char *key, const char *value)
{
	if (!sv.state)
		return;

	MSG_WriteByte (&sv.reliable_datagram, svc_serverinfo);
	MSG_WriteString (&sv.reliable_datagram, key);
	MSG_WriteString (&sv.reliable_datagram, value);
}

/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Server info settings:\n");
		Info_Print (svs.info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't change serverinfo\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (svs.info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING);

	// if this is a cvar, change it too	
	var = Cvar_FindVar (Cmd_Argv(1));
	if (var)
	{
		Z_Free (var->string);	// free the old value string	
		var->string = Z_StrDup (Cmd_Argv(2));
		var->value = Q_atof (var->string);
	}

	SV_SendServerInfoChange(Cmd_Argv(1), Cmd_Argv(2));
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Localinfo_f (void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Local info settings:\n");
		Info_Print (localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't change localinfo\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (localinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_LOCALINFO_STRING);
}


/*
===========
SV_User_f

Examine a users info strings
===========
*/
void SV_User_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Info_Print (host_client->userinfo);
}

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
void SV_Gamedir (void)
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current *gamedir: %s\n", Info_ValueForKey (svs.info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't change sv_gamedir\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING);
}

/*
================
SV_Floodprot_f

Sets the gamedir and path to a different directory.
================
*/

void SV_Floodprot_f (void)
{
	int arg1, arg2, arg3;

	if (Cmd_Argc() == 1)
	{
		if (fp_messages) {
			Con_Printf ("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n",
				fp_messages, fp_persecond, fp_secondsdead);
			return;
		} else
			Con_Printf ("No floodprots enabled.\n");
	}

	if (Cmd_Argc() != 4)
	{
		Con_Printf ("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		Con_Printf ("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	arg1 = atoi(Cmd_Argv(1));
	arg2 = atoi(Cmd_Argv(2));
	arg3 = atoi(Cmd_Argv(3));

	if (arg1<=0 || arg2 <= 0 || arg3<=0) {
		Con_Printf ("All values must be positive numbers\n");
		return;
	}

// BorisU -->
	if (arg2 > 10) arg2 = 10; 
	if (arg3 > 10) arg3 = 10;
// <-- BorisU

	if (arg1 > 10) {
		Con_Printf ("Can only track up to 10 messages.\n");
		return;
	}

	fp_messages	= arg1;
	fp_persecond = arg2;
	fp_secondsdead = arg3;
}

void SV_Floodprotmsg_f (void)
{
	if (Cmd_Argc() == 1) {
		Con_Printf("Current msg: %s\n", fp_msg);
		return;
	} else if (Cmd_Argc() != 2) {
		Con_Printf("Usage: floodprotmsg \"<message>\"\n");
		return;
	}
	Q_snprintfz(fp_msg, 255, "%s", Cmd_Argv(1));
}

/*
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
================
*/
void SV_Gamedir_f (void)
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current gamedir: %s\n", com_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: gamedir <newdir>\n");
		return;
	}

	if (sv_cmd == SV_CMD_RCON) {
		Con_Printf ("You can't change gamedir\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir (dir);
	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING);
}

/*
================
SV_Snap
================
*/
void SV_Snap (int uid)
{
	client_t *cl,*clf;
	char		pcxname[80];
	char		checkname[MAX_OSPATH];
	int			i;

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (cl->state < cs_preconnected)
			continue;
		if (cl->userid == uid)
			break;
	}
	if (i >= MAX_CLIENTS) {
		Con_Printf ("userid not found\n");
		return;
	}

	sprintf(pcxname, "%d-00.pcx", uid);

	sprintf(checkname, "%s/snap", gamedirfile);
	Sys_mkdir(gamedirfile);
	Sys_mkdir(checkname);

	for (i=0 ; i<=99 ; i++)
	{
		pcxname[strlen(pcxname) - 6] = i/10 + '0';
		pcxname[strlen(pcxname) - 5] = i%10 + '0';
		sprintf (checkname, "%s/snap/%s", gamedirfile, pcxname);
		if (!Sys_FileExists(checkname))
			break;	// file doesn't exist
	}
	if (i==100)
	{
		Con_Printf ("Snap: Couldn't create a file, clean some out.\n");
		return;
	}
	strcpy(cl->uploadfn, checkname);

	for(i=0,clf=svs.clients;i<MAX_CLIENTS;i++,clf++)
	{
		if (!cl->state)
			continue;
		if(*(unsigned *)clf->netchan.remote_address.ip==*(unsigned *)net_from.ip) 
			break;
	}
	if(i==MAX_CLIENTS) clf=NULL;

	//memcpy(&cl->snap_from, &net_from, sizeof(net_from));
	cl->snap_from=clf;

	if (sv_redirected != RD_NONE)
		cl->remote_snap = true;
	else
		cl->remote_snap = false;

	cl->sendImportantCommand=svs.realtime-2.0;
	ClientReliableWrite_Begin (cl, svc_stufftext, 24);
	ClientReliableWrite_String (cl, "cmd snap");
	Con_Printf ("Requesting snap from user %d...\n", uid);
}

/*
================
SV_Snap_f
================
*/
void SV_Snap_f (void)
{
	int			uid;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage:  snap <userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	SV_Snap(uid);
}

/*
================
SV_Snap
================
*/
void SV_SnapAll_f (void)
{
	client_t *cl;
	int			i;

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (cl->state < cs_preconnected || cl->spectator)
			continue;
		SV_Snap(cl->userid);
	}
}

// BorisU -->
/*
================
SV_MasterPassword
================
*/
void SV_MasterPassword_f (void)
{
	if (!server_cfg_done)
		strlcpy(master_password, Cmd_Argv(1), sizeof(master_password));
	else
		Con_Printf("master_password can be set only in server.cfg\n");
}
void SV_BotPassword_f (void)
{
	if (!server_cfg_done)
		strlcpy(bot_password, Cmd_Argv(1), sizeof(bot_password));
	else
		Con_Printf("bot_password can be set only in server.cfg\n");
}
// <-- BorisU


/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	if (COM_CheckParm ("-cheats"))
	{
		sv_allow_cheats = true;
		Info_SetValueForStarKey (svs.info, "*cheats", "ON", MAX_SERVERINFO_STRING);
	}

	Cmd_AddCommand ("logfile", SV_Logfile_f);
	Cmd_AddCommand ("fraglogfile", SV_Fraglogfile_f);
	Cmd_AddCommand ("logerrors", SV_ErrorLogfile_f);
	Cmd_AddCommand ("logrcon", SV_RconLogfile_f);


#ifdef QW262
	SV_InitOperatorCommands262();
#endif

	Cmd_AddCommand ("snap", SV_Snap_f);
	Cmd_AddCommand ("snapall", SV_SnapAll_f);
	Cmd_AddCommand ("stufftext", SV_StuffText_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);

// bliP -->
	Cmd_AddCommand ("rmdir", SV_RemoveDirectory_f);
	Cmd_AddCommand ("rm", SV_RemoveFile_f);
	Cmd_AddCommand ("ls", SV_ListFiles_f);
// <-- bliP

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	Cmd_AddCommand ("say", SV_ConSay_f);
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("quit", SV_Quit_f);
	Cmd_AddCommand ("god", SV_God_f);
	Cmd_AddCommand ("give", SV_Give_f);
	Cmd_AddCommand ("noclip", SV_Noclip_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("localinfo", SV_Localinfo_f);
	Cmd_AddCommand ("user", SV_User_f);
	Cmd_AddCommand ("gamedir", SV_Gamedir_f);
	Cmd_AddCommand ("sv_gamedir", SV_Gamedir);
	Cmd_AddCommand ("floodprot", SV_Floodprot_f);
	Cmd_AddCommand ("floodprotmsg", SV_Floodprotmsg_f);

	Cmd_AddCommand ("master_password", SV_MasterPassword_f);
	Cmd_AddCommand ("bot_password", SV_BotPassword_f);

	cl_warncmd.value = 1; // ????? FIXME!!!
}
