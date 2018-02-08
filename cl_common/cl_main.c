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
// cl_main.c  -- client main loop

#include "quakedef.h"
#include "winquake.h"
#ifdef _WIN32
#include "winsock.h"
#else
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include <ctype.h>
#include "teamplay.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

qboolean	noclip_anglehack;		// remnant from old quake

cvar_t	rcon_password = {"rcon_password", ""};
cvar_t	rcon_address = {"rcon_address", ""};

cvar_t	cl_timeout = {"cl_timeout", "60"};

cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2

cvar_t	cl_sbar		= {"cl_sbar", "0",  CVAR_ARCHIVE};
cvar_t	cl_hudswap	= {"cl_hudswap", "0",  CVAR_ARCHIVE};
cvar_t	cl_maxfps	= {"cl_maxfps", "0",  CVAR_ARCHIVE};

cvar_t	lookspring = {"lookspring","0",  CVAR_ARCHIVE};
cvar_t	lookstrafe = {"lookstrafe","0",  CVAR_ARCHIVE};
cvar_t	sensitivity = {"sensitivity","3",  CVAR_ARCHIVE};

//Added by -=MD=-
cvar_t	uid_cvar	= {"uid", ""};

//Added by BorisU -->
int	mtfl = 0; // MTFL limitations

cvar_t	cl_version		= {"cl_version", "", CVAR_ROM};

cvar_t	mapname			= {"mapname", "", CVAR_ROM};
cvar_t	mapgroupname	= {"mapgroupname", "", CVAR_ROM};
cvar_t	date			= {"date", "", CVAR_ROM};
cvar_t	timevar			= {"time", "", CVAR_ROM};
cvar_t	cl_ping			= {"cl_ping", "", CVAR_ROM};

qboolean OnDateTimeFormatChange (cvar_t *var, const char *value);
cvar_t	date_format		= {"date_format", "0", 0, OnDateTimeFormatChange};
cvar_t	time_format		= {"time_format", "0", 0, OnDateTimeFormatChange};

cvar_t	cl_parsesay		= {"cl_parsesay", "1"};
cvar_t	cl_digits_format= {"cl_digits_format", "0"};
cvar_t	cl_nofake		= {"cl_nofake", "2"};
cvar_t	cl_parseWhiteText = {"cl_parsewhitetext", "1"};

cvar_t	cl_sbar_inv		= {"cl_sbar_inv", "1"};

cvar_t	cl_gamespy_showplayers  = {"cl_gamespy_showplayers", "0"};
cvar_t	cl_gamespy_showrules	= {"cl_gamespy_showrules", "0"};

cvar_t cl_substsinglequote	= {"cl_substsinglequote", "0"};
cvar_t	cl_stringescape		= {"cl_stringescape", "0"};
cvar_t	cl_curlybraces		= {"cl_curlybraces", "0"};

cvar_t	cl_demoplay_restrictions	= {"cl_demoplay_restrictions", "1"};
cvar_t	cl_spectator_restrictions	= {"cl_spectator_restrictions", "1"};

cvar_t	qizmo_dir = {"qizmo_dir", "qizmo"}; // Tonik
cvar_t	cl_demoflushtime = {"cl_demoflushtime", "5"};

cvar_t	capture_dir = {"capture_dir", "capture"};

long	capture_num = -1; // surmoclient
int		capture_fps = 0;

int	id_PrintScr; // Sergio (printscr)

cvar_t	cl_swapredblue = {"cl_swapredblue", "0"};
cvar_t	cl_ignore_topcolor = {"cl_ignore_topcolor", "0"};
// <-- BorisU

cvar_t	m_pitch = {"m_pitch","0.022", CVAR_ARCHIVE};
cvar_t	m_yaw = {"m_yaw","0.022", CVAR_ARCHIVE};
cvar_t	m_forward = {"m_forward","1"};
cvar_t	m_side = {"m_side","0.8"};

//cvar_t	entlatency = {"entlatency", "20"};
cvar_t	cl_predict_players = {"cl_predict_players", "1"};
cvar_t	cl_predict_players2 = {"cl_predict_players2", "1"};
cvar_t	cl_solid_players = {"cl_solid_players", "1"};

cvar_t	localid = {"localid", ""};

static qboolean allowremotecmd = true;
qboolean com_blockscripts; // ezquake

// Tonik -->
qboolean OnDemotimescaleChange (cvar_t *var, const char *value);
cvar_t	cl_demotimescale		= {"demotimescale", "1", 0, OnDemotimescaleChange};
cvar_t	r_rocketlight			= {"r_rocketlight", "1"};
cvar_t	cl_muzzleflash			= {"cl_muzzleflash", "1"};
cvar_t r_rockettrail			= {"r_rockettrail", "1"};
cvar_t r_grenadetrail			= {"r_grenadetrail", "1"};
cvar_t	r_explosionlightcolor	= {"r_explosionlightcolor", "0"};
cvar_t	r_explosionlight		= {"r_explosionlight", "1"};
cvar_t	r_explosiontype			= {"r_explosiontype", "0"};
cvar_t	r_powerupglow			= {"r_powerupglow", "1"};
cvar_t	cl_truelightning		= {"cl_truelightning", "0"};
// <-- Tonik

// fuh -->
cvar_t r_lightflicker		= {"r_lightflicker", "1"};
cvar_t r_rocketlightcolor	= {"r_rocketlightcolor", "0"};
cvar_t r_flagcolor			= {"r_flagcolor", "0"};

cvar_t cl_useproxy			= {"cl_useproxy", "0"};
cvar_t	sys_yieldcpu 		= {"sys_yieldcpu", "0"};

cvar_t cl_nolerp			= {"cl_nolerp", "1"};
// <-- fuh

//
// info mirrors
//
cvar_t	password = {"password", "", CVAR_USERINFO};
cvar_t	spectator = {"spectator", "", CVAR_USERINFO};
qboolean OnNameChange (cvar_t *var, const char *value);
cvar_t	name = {"name","unnamed", CVAR_ARCHIVE|CVAR_USERINFO, OnNameChange};
qboolean OnTeamChange (cvar_t *var, const char *value);
cvar_t	team = {"team","", CVAR_ARCHIVE|CVAR_USERINFO, OnTeamChange};
cvar_t	skin = {"skin","", CVAR_ARCHIVE|CVAR_USERINFO};
cvar_t	filter = {"filter","", CVAR_USERINFO};
cvar_t	topcolor = {"topcolor","0", CVAR_ARCHIVE|CVAR_USERINFO};
cvar_t	bottomcolor = {"bottomcolor","0", CVAR_ARCHIVE|CVAR_USERINFO};
cvar_t	rate = {"rate","2500", CVAR_ARCHIVE|CVAR_USERINFO};
cvar_t	msg = {"msg","1", CVAR_ARCHIVE|CVAR_USERINFO};

client_static_t	cls;
client_state_t	cl;

centity_t		cl_entities[MAX_EDICTS];
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

// refresh list
// this is double buffered so the last frame
// can be scanned for oldorigins of trailing objects
int				cl_numvisedicts, cl_oldnumvisedicts;
entity_t		*cl_visedicts, *cl_oldvisedicts;
entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

double			connect_time = -1;		// for connection retransmits

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution
qboolean	nomaster;

double		curtime;

int			host_hunklevel;

byte		*host_basepal;
byte		*host_colormap;

netadr_t	master_adr;				// address of the master server

cvar_t	host_speeds = {"host_speeds","0"};			// set for running times
cvar_t	developer = {"developer","0"};

int			fps_count;

jmp_buf 	host_abort;

void Master_Connect_f (void);

float	server_version = 0;	// version of server we connected to

char emodel_name[] = 
	{ 'e' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char pmodel_name[] = 
	{ 'p' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char prespawn_name[] = 
	{ 'p'^0xff, 'r'^0xff, 'e'^0xff, 's'^0xff, 'p'^0xff, 'a'^0xff, 'w'^0xff, 'n'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '0'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char modellist_name[] = 
	{ 'm'^0xff, 'o'^0xff, 'd'^0xff, 'e'^0xff, 'l'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff, 
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char soundlist_name[] = 
	{ 's'^0xff, 'o'^0xff, 'u'^0xff, 'n'^0xff, 'd'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff, 
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };

unsigned short pmodel_crc, emodel_crc;

#ifdef QW262
#include "cl_main262.inc"
#endif

// BorisU -->
qboolean OnNameChange (cvar_t *var, const char *value)
{
	if (strlen(value) > 31) {
		Con_Printf("Name is too long\n");
		return true;
	}
	return false;
}

qboolean OnTeamChange (cvar_t *var, const char *value)
{	
	int	i;
	// do not allow to change team at client side
	if (cl.teamfortress && cbuf_current != &cbuf_svc)
		return true; 
	
	loc_team = 0;
	for (i=0; i<4; i++) {
		if (!Q_stricmp(loc_teamnames[i], value)) {
		//	Con_Printf("Setting team to %i=%s\n", i, loc_teamnames[i]);
			loc_team = i;
			break;
		}
	}
	return false;
}

void Set_date_time_vars()
{
	time_t aclock;
	struct tm* now;
	char buf[16];

	time(&aclock);
	now = localtime( &aclock);

	switch ((int)date_format.value) {
	default:
	case 0: sprintf ( buf, "%.2d%.2d%.4d", now->tm_mday, now->tm_mon+1, now->tm_year+1900); break; 
	case 1: sprintf ( buf, "%.2d%.2d", now->tm_mday, now->tm_mon+1); break;
	case 2: sprintf ( buf, "%.2d-%.2d-%.4d", now->tm_mday, now->tm_mon+1, now->tm_year+1900); break; 
	case 3: sprintf ( buf, "%.2d-%.2d", now->tm_mday, now->tm_mon+1); break;
	}

	Cvar_SetROM (&date, buf);

	switch ((int)time_format.value) {
	default:
	case 0: if ((cl.fpd & FPD_NO_TIMERS)) 
				now->tm_sec = 0;
			sprintf ( buf, "%.2d%.2d%.2d", now->tm_hour, now->tm_min, now->tm_sec); break;
	case 1: sprintf ( buf, "%.2d%.2d", now->tm_hour, now->tm_min); break;
	case 2: if ((cl.fpd & FPD_NO_TIMERS)) 
				now->tm_sec = 0;
			sprintf ( buf, "%.2d-%.2d-%.2d", now->tm_hour, now->tm_min, now->tm_sec); break;
	case 3: sprintf ( buf, "%.2d-%.2d", now->tm_hour, now->tm_min); break;
	}

	Cvar_SetROM (&timevar, buf);
	sprintf ( ClockStr, "%.2d:%.2d", now->tm_hour, now->tm_min);
}

qboolean OnDateTimeFormatChange (cvar_t *var, const char *value)
{
	Cvar_Set (var, value);
	Set_date_time_vars ();
	return true;
}
qboolean OnDemotimescaleChange (cvar_t *var, const char *value)
{
	float newvalue = Q_atof(value);
	if (newvalue >= 0.0 && newvalue <= 20.0) 
		return false;

	Con_Printf ("Invalid demoplaying speed\n");
	return true;
}
// <-- BorisU

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	if (1 /* key_dest != key_console */ /* && cls.state != ca_dedicated */)
	{
		M_Menu_Quit_f ();
		return;
	}
	CL_Disconnect ();
	Sys_Quit ();
}


/*
==================
CL_SayID_f
==================
*/
void CL_SayID_f (void)
{
	if (Cmd_Argc() == 1)
		return;

	if(!*uid_cvar.string)
	{
		Con_Printf("uid value no initialized\n");
		return;
	}
	Cbuf_AddText("say_id ");
	Cbuf_AddText( uid_cvar.string);
	Cbuf_AddText(" \"");
	
	Cbuf_AddText(Cmd_Args());
	Cbuf_AddText("\"\n");
}

// BorisU -->
/*
============
CL_Userdir_f

============
*/
void CL_Userdir_f (void)
{
	if (cls.state > ca_disconnected || Cmd_Argc() == 1) 
		Con_Printf("Current userdir: %s\n", userdirfile);
	else {
		int u = Q_atoi(Cmd_Argv(2));
		if (u < 0 || u > 5)
			Con_Printf("Invalid userdir type\n");
		else
			COM_SetUserDirectory (Cmd_Argv(1), Cmd_Argv(2));
	}
}
// <-- BorisU 

/*
=======================
CL_Version_f
======================
*/

qboolean IsMDserver=false;

// BorisU --> 
// added parameter for version command
void CL_Version_f (void)
{

#ifdef QW262
	int ver;

	if (cls.state >= ca_connected) 
		Con_Printf ("You should be disconnected to change protocol version\n");
	else if(Cmd_Argc() < 2)
		IsMDserver=!IsMDserver;
	else {
		ver = atoi(Cmd_Argv(1));
		if (ver == 262)
			IsMDserver = true;
		else if (ver == 230)
			IsMDserver = false;
	}
#endif

	Con_Printf ("%4.2f compatible version\n", IsMDserver?VERSION:2.3);
//	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
}

void CL_SND_Restart_f (void)
{
	Con_Printf("Restarting sound system....\n");
	SNDDMA_Shutdown ();
	SNDDMA_Init ();
}

void CL_SetModels (void)
{
	char st[16];

	sprintf(st, "%d", (int) pmodel_crc);
	Info_SetValueForKey (cls.userinfo, pmodel_name, IsMDserver ? "" : st, MAX_INFO_STRING);
	sprintf(st, "%d", (int) emodel_crc);
	Info_SetValueForKey (cls.userinfo, emodel_name, IsMDserver ? "" : st, MAX_INFO_STRING);
}
// <-- BorisU

/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
void CL_SendConnectPacket (void)
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;
// JACK: Fixed bug where DNS lookups would cause two connects real fast
//       Now, adds lookup time to the connect time.
//		 Should I add it to cls.realtime instead?!?!

	if (cls.state != ca_disconnected)
		return;

	t1 = Sys_DoubleTime ();

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		connect_time = -1;
		return;
	}

	if (!NET_IsClientLegal(&adr))
	{
		Con_Printf ("Illegal server address\n");
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (27500);
	t2 = Sys_DoubleTime ();

	connect_time = cls.realtime+t2-t1;	// for retransmit requests

	cls.qport = Cvar_VariableValue("qport");

	//Info_SetValueForStarKey (cls.userinfo, "*ip", NET_AdrToString(adr), MAX_INFO_STRING);

//	Con_Printf ("Connecting to %s...\n", cls.servername);

	CL_SetModels();
	sprintf (data, "%c%c%c%cconnect %i %i %i \"%s\"\n",
		255, 255, 255, 255,	IsMDserver?PROTOCOL_VERSION:OLD_PROTOCOL_VERSION, 
		cls.qport, cls.challenge, cls.userinfo);
	NET_SendPacket (strlen(data), data, adr);
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out

=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;

	if (connect_time == -1)
		return;
	if (cls.state != ca_disconnected)
		return;
	if (connect_time && cls.realtime - connect_time < 5.0)
		return;

	t1 = Sys_DoubleTime ();
	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_Printf ("Bad server address\n");
		connect_time = -1;
		return;
	}
	if (!NET_IsClientLegal(&adr))
	{
		Con_Printf ("Illegal server address\n");
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (27500);
	t2 = Sys_DoubleTime ();

	connect_time = cls.realtime+t2-t1;	// for retransmit requests

	Con_Printf ("Connecting to %s...\n", cls.servername);
	sprintf (data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);
	NET_SendPacket (strlen(data), data, adr);
}

void CL_BeginServerConnect(void)
{
	connect_time = 0;
	CL_CheckForResend();
}

// Added by -=MD=-
void CL_GameSpy_f (void)
{
	if(Cmd_Argc()!=2)
		Con_Printf("Usage :  gamespy address:port\n");
	else
	{
		Cbuf_AddText("packet ");
		Cbuf_AddText(Cmd_Argv(1));
		Cbuf_AddText(" status\n");
	}
}

qboolean CL_ConnectedToProxy(void)
{
	cmdalias_t *alias = NULL;
	char **s, *qizmo_aliases[] = {	"ezcomp", "ezcomp2", "ezcomp3", 
									"f_sens", "f_fps", "f_tj", "f_ta", NULL};

	if (cls.state < ca_active)
		return false;
	for (s = qizmo_aliases; *s; s++) {
		if (!(alias = Cmd_FindAlias(*s)) || !(alias->flags & ALIAS_SERVER))
			return false;
	}
	return true;
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f (void)
{
	qboolean	proxy;

	if (Cmd_Argc() != 2) {
		Con_Printf ("usage: connect <server>\n");
		return;	
	}
	
	proxy = cl_useproxy.value && CL_ConnectedToProxy();

	if (proxy) {
		Cbuf_AddText(va("say ,connect %s", Cmd_Argv(1)));
	} else {
		CL_Disconnect ();
		strlcpy (cls.servername, Cmd_Argv (1), sizeof(cls.servername));
		CL_BeginServerConnect();
	}
}

// fuh ->
void CL_Join_f (void) {
	qboolean proxy;

	proxy = cl_useproxy.value && CL_ConnectedToProxy();

	if (Cmd_Argc() > 2) {
		Con_Printf ("Usage: %s [server]\n", Cmd_Argv(0));
		return; 
	}

	Cvar_Set(&spectator, "");

	if (Cmd_Argc() == 2)
		Cbuf_AddText(va("%s %s\n", proxy ? "say ,connect" : "connect", Cmd_Argv(1)));
	else
		Cbuf_AddText(va("%s\n", proxy ? "say ,reconnect" : "reconnect"));
}

void CL_Observe_f (void)
{
	qboolean proxy;

	proxy = cl_useproxy.value && CL_ConnectedToProxy();

	if (Cmd_Argc() > 2) {
		Con_Printf ("Usage: %s [server]\n", Cmd_Argv(0));
		return; 
	}

	if (!spectator.string[0] || !strcmp(spectator.string, "0"))
		Cvar_SetValue(&spectator, 1);

	if (Cmd_Argc() == 2)
		Cbuf_AddText(va("%s %s\n", proxy ? "say ,connect" : "connect", Cmd_Argv(1)));
	else
		Cbuf_AddText(va("%s\n", proxy ? "say ,reconnect" : "reconnect"));
}
// <-- fuh

int		rcon_counter;
float	rcon_time = -9999;

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char	message[1024];
	int		i;
	netadr_t	to;

	if (!rcon_password.string)
	{
		Con_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255; 
	message[3] = 255;
	message[4] = 0;

	strcpy (message+4, "rcon ");
	if(rcon_password.string && *(rcon_password.string))
	{
		strcpy (message+9, rcon_password.string);
		strcat (message, " ");
	}

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		strcat (message, Cmd_Argv(i));
		strcat (message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_Printf ("You must either be connected,\n"
						"or set the 'rcon_address' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address.string, &to);
	}

	if(++rcon_counter >= 10) {
		if (cls.realtime < rcon_time + 3) 
			return;
		rcon_time = cls.realtime;
		rcon_counter = 0;
	}

	NET_SendPacket (strlen(message)+1, message, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	int			i;

	S_StopAllSounds (true);

	Con_DPrintf ("Clearing memory\n");
	D_FlushCaches ();
	Mod_ClearAll ();
	if (host_hunklevel)	// FIXME: check this...
		Hunk_FreeToLowMark (host_hunklevel);

	CL_ClearTEnts ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	SZ_Clear (&cls.netchan.message);

// clear other arrays	
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));

//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
extern float	last_lock, last_f_version, last_flash_time, demo_flushed;
extern double	cam_lastviewtime, last_lagmeter_time, last_speed_time;
extern float impulse_time, cmd_time, rcon_time, setinfo_time;
extern int impulse_counter, cmd_counter, rcon_counter, setinfo_counter;
void CL_Disconnect (void)
{
	byte	final[10];

	connect_time = -1;
	server_version = 0;
	if (cl.teamfortress)
		V_TF_ClearGrenadeEffects();
	cl.teamfortress = false;

#ifdef _WIN32
	SetWindowText (mainwindow, "QW262: disconnected");
#endif

// stop sounds (especially looping!)
	S_StopAllSounds (true);
	
// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state != ca_disconnected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		final[0] = clc_stringcmd;
		strcpy (final+1, "drop");
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);
		Netchan_Transmit (&cls.netchan, 6, final);

		cls.state = ca_disconnected;

		cls.mvdplayback = cls.demoplayback = cls.demorecording = cls.timedemo = false;

#ifdef QW262
		if(IsMDserver) CL_Disconnect262();
#endif

	}
	Cam_Reset();

	if (cls.download) {
		fclose(cls.download);
		cls.download = NULL;
	}

	CL_StopUpload();

// BorisU -->
	last_f_version = - 20.0;
	last_flash_time = - 10.0;
	last_lock = cam_lastviewtime = cl.last_ping_request = 
	last_lagmeter_time = last_speed_time = demo_flushed = 0.0;
	last_deathtime = last_respawntime = last_gotammotime = last_locationtime = 0.0;
	impulse_time = cmd_time = rcon_time = setinfo_time = 0.0;
	impulse_counter = cmd_counter = rcon_counter = setinfo_counter = 0;
	cls.realtime = 0; 
	capture_num = -1; // stop capture

	CL_RE_Trigger_ResetLasttime();
#ifdef GLQUAKE	
	ClearFlares();
	cls.allow24bit = 0;
#endif
	cl.paused = 0;
// <-- BorisU
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f (void)
{
	int		userid;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <uid / userid / name>\n");
		return;
	}

	userid = atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == userid
		|| !strcmp(Info_ValueForKey(cl.players[i].userinfo,"uid"), Cmd_Argv(1)) 
		|| !strcmp(cl.players[i].name, Cmd_Argv(1)))
		{
			Info_Print (cl.players[i].userinfo);
			Con_Printf ("%i bytes total\n", strlen(cl.players[i].userinfo));
			return;
		}
	}
	Con_Printf ("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f (void)
{
	int		i;
	int		c;

	c = 0;
	Con_Printf ("<uid> userid frags name\n");
	Con_Printf ("----- ------ ----- ----\n");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_Printf ("%-5s %-6i %-4i %s\n", Info_ValueForKey(cl.players[i].userinfo,"uid"),cl.players[i].userid, cl.players[i].frags, cl.players[i].name);
			c++;
		}
	}

	Con_Printf ("%i total users\n", c);
}


char *Insert3Int(char *p,int n)
{
	byte shift;

	if (cl_digits_format.value)
		shift = 48;
	else
		shift = 18;
	if(n<0) n=0;
	n%=1000;
	if(n>99)
		*p++=shift+(n/100); 
	if(n>9) { 
		n%=100; 
		*p++=shift+(n/10);
		n%=10;
	}
	*p++=shift+n;
	return p;
}

char *Insert6Str(char *p,char *s)
{
	if(p[-1]==' ') p[-1]='~';
	while(*s) *p++=(*s++);
	return p;
}

char *InsertAmmoStr(char *p, char **cc, int stat)
{
	char		ch;
	char		*c;
	unsigned	s;

	c = *cc;

	if((c[1]=='-' || c[1]=='+') && c[2]>='0' && c[2]<='9') {
		ch=c[1];
		s=0;
		c+=2;
		while(*c>='0' && *c<='9') {
			s*=10;
			s+=*c-'0';
			c++;
		}
		*cc = c-1;
		if(s>999) s=999;
		p=Insert3Int(p, ch=='-' ? cl.stats[stat]>s ? cl.stats[stat]-s : 0 : cl.stats[stat]<s ? s-cl.stats[stat] : 0);
		return p;
	} 

	p=Insert3Int(p,cl.stats[stat]);
	return p;
} 

char	*UnionColor[]=
{
	"white",
	"----",
	"----",
	"----",
	"red",
	"----",
	"----",
	"----",
	"----",
	"----",
	"----",
	"green",
	"yellow",
	"blue"
};

// BorisU -->
char *ParseSay (char *string)
{
	static char	buf[SAY_TEAM_CHAT_BUFFER_SIZE];
	char		*s;
	char		*b;
	char		ch;
	unsigned	color;

	if (!cl_parsesay.value || (cl.fpd & FPD_NO_TEAM_MACROS) )
		return string;

	s = string;
	b = buf;

	while (*s && b < buf + SAY_TEAM_CHAT_BUFFER_SIZE - 1) {
		if (*s == '%') {
			s++;
			switch (*s) {
			case 'h': // health
				b=Insert3Int(b,cl.stats[STAT_HEALTH]); break;
			case 'a': // armor
				b=Insert3Int(b,cl.stats[STAT_ARMOR]); 
				break;
			case 't': // armor type
				b=Insert6Str(b, (cl.stats[STAT_ITEMS] & IT_ARMOR1) ? "green" : 
								(cl.stats[STAT_ITEMS] & IT_ARMOR2) ? "yellow" : 
								(cl.stats[STAT_ITEMS] & IT_ARMOR3) ? "red" : "no" );
				break;
			case 'p': // powerup
				if(cl.stats[STAT_ITEMS] & IT_QUAD)				b=Insert6Str(b,"Quad‘ ");
				if(cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)	b=Insert6Str(b,"Ž666‘Ž ");
				if(cl.stats[STAT_ITEMS] & IT_INVISIBILITY)		b=Insert6Str(b,"Eyes‘ ");
				if(cl.stats[STAT_ITEMS] & (IT_KEY1|IT_KEY2))	b=Insert6Str(b,"flag");
				if(b[-1]==' ') b--;
				break;
			case 's': // shells
				b=InsertAmmoStr(b,&s,STAT_SHELLS);
				break;
			case 'n': // shells
				b=InsertAmmoStr(b,&s,STAT_NAILS);
				break;
			case 'r': // shells
				b=InsertAmmoStr(b,&s,STAT_ROCKETS);
				break;
			case 'c': // cells
				b=InsertAmmoStr(b,&s,STAT_CELLS);
				break;
			case 'b': // bottomcolor
				color=bottomcolor.value<0.0 ? 0: bottomcolor.value>13.0? 13: (int) bottomcolor.value;
				b=Insert6Str(b,UnionColor[color]);
				break;
			case 'l': // current location
				b=InsertLocation(&cl.simorg,b,SAY_TEAM_CHAT_BUFFER_SIZE-(b-buf),true);
				break;
			case 'd': // place of last death
				b=InsertLocation(&PlaceOfLastDeath,b,SAY_TEAM_CHAT_BUFFER_SIZE-(b-buf),false);
				break;
			case 'L': // current location or place of last death if died less than 5 sec ago
				if (cls.realtime-last_deathtime > 5.0)
					b=InsertLocation(&cl.simorg,b,SAY_TEAM_CHAT_BUFFER_SIZE-(b-buf),true);
				else
					b=InsertLocation(&PlaceOfLastDeath,b,SAY_TEAM_CHAT_BUFFER_SIZE-(b-buf),false);
				break;
			default:
				*b++ = s[-1];
				continue;
			}
			s++;
		} else if (*s == Cmd_PrefixChar) {
			ch = 0;
			switch (s[1]) {
				case '\\': if(!Q_stricmp(Cmd_Argv(0), "say_team") ||
							  !Q_stricmp(Cmd_Argv(0), "say_to_team")) ch = 0x0D; break;
				case ':': ch = 0x0A; break;
				case '[': ch = 0x90; break;
				case ']': ch = 0x91; break;
				case 'G': ch = 0x86; break; // ocrana leds
				case 'R': ch = 0x87; break;
				case 'Y': ch = 0x88; break;
				case 'B': ch = 0x89; break;
			}
			if (ch) {
				*b++ = ch;
				s += 2;
			} else
				*b++ = *s++;
		} else
			*b++ = *s++;
	}

	*b = '\0';
	return	buf;
}

char capture_path[MAX_OSPATH];
// from surmoclient
void CL_Capture_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("usage: capture fps\n or capture 0 (disable capturing)\n");
		return;
	}

	if (!cls.demoplayback) {
		Con_Printf("capture works only during demo playback\n");
		return;
	}

	capture_fps = atoi(Cmd_Argv(1));
	if (capture_fps < 0){
		capture_fps = 30;
	} else if(capture_fps == 0)	{
		capture_num = -1;
		return;
	}
// Start capturing
	capture_num = 0;
	sprintf(capture_path, "%s/%s-%s/",capture_dir.string, date.string, timevar.string);
	COM_CreatePath(capture_path);
}
// <-- BorisU 

void CL_Color_f (void)
{
	// just for quake compatability...
	int		top, bottom;
	char	num[16];

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%s %s\"\n",
			Info_ValueForKey (cls.userinfo, "topcolor"),
			Info_ValueForKey (cls.userinfo, "bottomcolor") );
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}
	
	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;
	
	sprintf (num, "%i", top);
	Cvar_Set (&topcolor, num);
	sprintf (num, "%i", bottom);
	Cvar_Set (&bottomcolor, num);
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
void CL_FullServerinfo_f (void)
{
	char	*p;
	float	jf = 0;

	if (Cmd_Argc() != 2)
		return;

	strlcpy (cl.serverinfo, Cmd_Argv(1), sizeof(cl.serverinfo));

	if ((p = Info_ValueForKey(cl.serverinfo, "*z_version")) && *p) {
		if (!server_version)
				Con_Printf ("ZQuake %s server\n", p);
		server_version = 2.40;
		jf = 1;
	} else if ((p = Info_ValueForKey(cl.serverinfo, "*qf_version")) && *p) {
		if (!server_version)
			Con_Printf ("QuakeForge %s server\n", p);
		server_version = 2.40;
		jf = 1;
	} else if ((p = Info_ValueForKey(cl.serverinfo, "*qwe_version")) && *p) {
		if (!server_version)
			Con_Printf ("QuakeWorld Extended %s server\n", p);
		server_version = 2.40;
		jf = 1;
	} else if ((p = Info_ValueForKey(cl.serverinfo, "*262ver")) && *p) {
		if (!server_version)
			Con_Printf ("QW262 %s server\n", p);
		server_version = 2.62;
		jf = 1;
	} else if ((p = Info_ValueForKey(cl.serverinfo, "*version")) && *p) {
		float v;
		v = Q_atof(p);
		if (v) {
			if (!server_version)
				Con_Printf ("QuakeWorld %1.2f server\n", v);
			server_version = v;
			jf = 0;
		}
	} else { // should not happen
		Con_Printf("Strange server\n");
		server_version = 2.30;
		jf = 0;
	}

	p = Info_ValueForKey (cl.serverinfo, "*cheats");
	if (*p)
		Con_Printf ("== Cheats are enabled ==\n");

	CL_ProcessServerInfo ();

	if (pm_jumpfix.value < 2)
		Cvar_SetValue(&pm_jumpfix, jf);

	cbuf_current = &cbuf_main;

	TP_NewMap ();
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
Casey was here :)
*/
void CL_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING);
	}
}

static qboolean ValidUid(char* value)
{
	while (*value) {
		if (isalpha(*value) || isdigit(*value) || *value == '_')
			value++;
		else
			return false;
	}
	return true;
}

int		setinfo_counter;
float	setinfo_time = -9999;

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f (void)
{
	char	*key;
	char	*value;

	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		Con_Printf ("setinfo: %i bytes total\n", strlen(cls.userinfo));
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}

	key = Cmd_Argv(1);
	value = Cmd_Argv(2);

	if(!strcmp(key,"pmodel") || !strcmp(key,"emodel"))
	{
		Con_Printf("words pmodel&emodel were reserved by ID-Software\n");
		return;
	}
	if(!strcmp(key,"uid") && (!ValidUid(value) || strlen(value) > 5)) {
		Con_Printf("Invalid uid\n");
		return;
	}

	
	if(++setinfo_counter >= 30) {
		if (cls.realtime < setinfo_time + 5 && !cls.demoplayback)
			return;
		setinfo_time = cls.realtime;
		setinfo_counter = 0;
	}

	Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING);

	if (cls.state >= ca_connected)
		Cmd_ForwardToServer ();
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("packet <destination> <contents>\n");
		return;
	}

	if (!NET_StringToAdr (Cmd_Argv(1), &adr))
	{
		Con_Printf ("Bad address\n");
		return;
	}

	if(!adr.port) 	adr.port=BigShort (27500);

	in = Cmd_Argv(2);
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = 0xff;

	l = strlen (in);
	for (i=0 ; i<l ; i++)
	{
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
			*out++ = in[i];
	}
	*out = 0;

	NET_SendPacket (out-send, send, adr);
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
//			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	Q_snprintfz (str, sizeof(str), "playdemo %s\n", cls.demos[cls.demonum]);

	Cbuf_InsertText (str);
	cls.demonum++;
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/

void CL_Changing_f (void)
{
	if (cls.download)  // don't change when downloading
		return;

	S_StopAllSounds (true);
	cl.intermission = 0;
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Con_Printf ("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	if (cls.download)  // don't change when downloading
		return;

	S_StopAllSounds (true);

	if (cls.state == ca_connected) {
		Con_Printf ("reconnecting...\n");
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (!*cls.servername) {
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	CL_Disconnect();
	CL_BeginServerConnect();
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s,*p;
	int		c,i;

	MSG_BeginReading ();
	MSG_ReadLong ();		// skip the -1

	c = MSG_ReadByte ();
	if (!cls.demoplayback && c!=A2C_PRINT)
		Con_Printf ("%s: ", NET_AdrToString (net_from));
//	Con_DPrintf ("%s", net_message.data + 5);
	if (c == S2C_CONNECTION)
	{
		Con_Printf ("connection\n");
		if (cls.state >= ca_connected)
		{
			if (!cls.demoplayback)
				Con_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup (&cls.netchan, net_from, cls.qport);
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");	
		cls.state = ca_connected;
		Con_Printf ("Connected.\n");
		allowremotecmd = false; // localid required now for remote cmds
		return;
	}
	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)
	{
		char	cmdtext[2048];

		Con_Printf ("client command\n");

		if ((*(unsigned *)net_from.ip != *(unsigned *)net_local_adr.ip
			&& *(unsigned *)net_from.ip != htonl(INADDR_LOOPBACK)) )
		{
			Con_Printf ("Command packet from remote host.  Ignored.\n");
			return;
		}
#ifdef _WIN32
		ShowWindow (mainwindow, SW_RESTORE);
		SetForegroundWindow (mainwindow);
#endif
		s = MSG_ReadString ();

		strncpy(cmdtext, s, sizeof(cmdtext) - 1);
		cmdtext[sizeof(cmdtext) - 1] = 0;

		s = MSG_ReadString ();

		while (*s && isspace(*s))
			s++;
		while (*s && isspace(s[strlen(s) - 1]))
			s[strlen(s) - 1] = 0;

		if (!allowremotecmd && (!*localid.string || strcmp(localid.string, s))) {
			if (!*localid.string) {
				Con_Printf("===========================\n");
				Con_Printf("Command packet received from local host, but no "
					"localid has been set.  You may need to upgrade your server "
					"browser.\n");
				Con_Printf("===========================\n");
				return;
			}
			Con_Printf("===========================\n");
			Con_Printf("Invalid localid on command packet received from local host. "
				"\n|%s| != |%s|\n"
				"You may need to reload your server browser and QuakeWorld.\n",
				s, localid.string);
			Con_Printf("===========================\n");
			Cvar_Set (&localid, "");
			return;
		}

		Cbuf_AddText (cmdtext);
		allowremotecmd = false;
		return;
	}
	// print command from somewhere
	if (c == A2C_PRINT)
	{
		s = MSG_ReadString ();
		if(*s=='\\') {
#ifdef GLQUAKE
			Con_Printf("\x1%-32s",Info_ValueForKey(s,"hostname"));
			Con_Printf("     %-14s",Info_ValueForKey(s,"map")); 
#else
			Con_Printf("\x1%-21s",Info_ValueForKey(s,"hostname"));
			Con_Printf(" %-11s",Info_ValueForKey(s,"map")); 
#endif
			p=s;
			c=-1;
			while(*p) if(*p++=='\n') c++;
			Con_Printf("%d/%s\n",c,Info_ValueForKey(s,"maxclients"));

			p = strchr(s,'\n');
			if (cl_gamespy_showrules.value) {
				*p = '\0';
				Info_Print(s);
			}
			p++;

			if (cl_gamespy_showplayers.value) {
				int id, frags, time, ping, topcolor, bottomcolor, result;
				char name[32], skin[16];
#ifdef GLQUAKE
				Con_Printf ("name             skin     team frags ping time\n"
					"----------------------------------------------\n");
#else
				Con_Printf ("name             skin team frags time\n"
					"-------------------------------------\n");
#endif
				for (i=0;i<c;i++) {
					result = sscanf(p, "%d %d %d %d \"%[^\"]\" \"%[^\"]\" %d %d", 
						&id, &frags, &time, &ping, &name, &skin, &topcolor, &bottomcolor);
					if (result == 5) { // skin is not set!
						sscanf(p, "%d %d %d %d \"%[^\"]\" \"\" %d %d", 
						&id, &frags, &time, &ping, &name, &topcolor, &bottomcolor);
						skin[0] = 0;
					}
					Print_flags[Print_current] |= PR_TR_SKIP;
#ifdef GLQUAKE
					Con_Printf("%-16.16s %-8.8s %-6.6s %3d %3d %2d\n", 
						name, strncmp(skin, "tf_", 3) ? skin : skin+3, UnionColor[bottomcolor], frags, ping, time);
#else
					Con_Printf("%-16.16s %-5.5s %-4.4s %3d %2d\n", 
						name, strncmp(skin, "tf_", 3) ? skin : skin+3, UnionColor[bottomcolor], frags, time);
#endif
					p = strchr(p,'\n')+1;
				}
			}
		} else {
			//Con_Printf("Other printing:\n");
			Con_Printf ("%s: print\n%s",NET_AdrToString (net_from),s);
		}
		return;
	}

	// ping from somewhere
	if (c == A2A_PING)
	{
		char	data[6];

		Con_Printf ("ping\n");

		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;
		
		NET_SendPacket (6, &data, net_from);
		return;
	}

	if (c == S2C_CHALLENGE) {
		Con_Printf ("challenge\n");

		s = MSG_ReadString ();
		cls.challenge = atoi(s);
		CL_SendConnectPacket ();
		return;
	}

	if (c == svc_disconnect) {
		if (cls.demoplayback) {
			Con_Printf ("\n======== End of demo ========\n\n");
			Host_EndGame ("End of demo");
		}
		else
			Host_EndGame ("Server disconnected");
		return;
	}


	Con_Printf ("unknown:  %c\n", c);
}


void CL_WriteDemoMessage (sizebuf_t *msg);
/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
//	while (NET_GetPacket ())
	while (CL_GetMessage())
	{
		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			CL_WriteDemoMessage (&net_message);
			continue;
		}

		if (net_message.cursize < 8 && !cls.mvdplayback)
		{
			Con_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!cls.demoplayback && 
			!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Con_DPrintf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}
		if (!cls.mvdplayback) {
			if (!Netchan_Process(&cls.netchan))
				continue;		// wasn't accepted for some reason
		} else {
			MSG_BeginReading ();
		}
		
		CL_ParseServerMessage ();

//		if (cls.demoplayback && cls.state >= ca_active && !CL_DemoBehind())
//			return;
	}

	//
	// check timeout
	//
	if (!cls.demoplayback && cls.state >= ca_connected
	 && curtime - cls.netchan.last_received > cl_timeout.value)
	{
		Con_Printf ("\nServer connection timed out.\n");
		CL_Disconnect ();
		Cbuf_Init ();
		return;
	}
	
}

//=============================================================================

/*
=====================
CL_Download_f
=====================
*/
void CL_Download_f (void)
{
	char *p, *q;

	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: download <datafile>\n");
		return;
	}

	Q_snprintfz (cls.downloadname, sizeof(cls.downloadname),
		"%s/%s", com_gamedir, Cmd_Argv(1));

	p = cls.downloadname;
	for (;;) {
		if ((q = strchr(p, '/')) != NULL) {
			*q = 0;
			Sys_mkdir(cls.downloadname);
			*q = '/';
			p = q + 1;
		} else
			break;
	}

	strcpy(cls.downloadtempname, cls.downloadname);
	cls.download = fopen (cls.downloadname, "wb");
	cls.downloadtype = dl_single;

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, va("download %s\n",Cmd_Argv(1)));
}

#ifdef _WIN32
#include <windows.h>
/*
=================
CL_Minimize_f
=================
*/
void CL_Windows_f (void) {
//	if (modestate == MS_WINDOWED)
//		ShowWindow(mainwindow, SW_MINIMIZE);
//	else
		SendMessage(mainwindow, WM_SYSKEYUP, VK_TAB, 1 | (0x0F << 16) | (1<<29));
}
#endif

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	cls.state = ca_disconnected;

	Info_SetValueForKey (cls.userinfo, "name", "unnamed", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "topcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "rate", "2500", MAX_INFO_STRING);
	Info_SetValueForKey (cls.userinfo, "msg", "1", MAX_INFO_STRING);
	//sprintf (st, "%4.2f-%04d", VERSION, build_number());
	//Info_SetValueForStarKey (cls.userinfo, "*ver", st, MAX_INFO_STRING);

	CL_InitInput ();
	CL_InitEnts ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	CL_InitCam ();
	PM_Init ();
	
//
// register our commands
//
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&developer);

	Cvar_RegisterVariable (&cl_warncmd);

// BorisU -->
	Cvar_RegisterVariable (&cl_warnexec);
	Cvar_RegisterVariable (&cl_substsinglequote);
	Cvar_RegisterVariable (&cl_stringescape);
	Cvar_RegisterVariable (&cl_curlybraces);
	Cvar_RegisterVariable (&cl_demoplay_restrictions);
	Cvar_RegisterVariable (&cl_spectator_restrictions);
	Cvar_RegisterVariable (&qizmo_dir);
	Cvar_RegisterVariable (&cl_demoflushtime);
	Cvar_RegisterVariable (&capture_dir);
	Cvar_RegisterVariable (&cl_swapredblue);
	Cvar_RegisterVariable (&cl_ignore_topcolor);
// <-- BorisU

	Cvar_RegisterVariable (&cl_upspeed);
	Cvar_RegisterVariable (&cl_forwardspeed);
	Cvar_RegisterVariable (&cl_backspeed);
	Cvar_RegisterVariable (&cl_sidespeed);
	Cvar_RegisterVariable (&cl_movespeedkey);
	Cvar_RegisterVariable (&cl_yawspeed);
	Cvar_RegisterVariable (&cl_pitchspeed);
	Cvar_RegisterVariable (&cl_anglespeedkey);
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_sbar);
	Cvar_RegisterVariable (&cl_hudswap);
	Cvar_RegisterVariable (&cl_maxfps);
	Cvar_RegisterVariable (&cl_timeout);
	Cvar_RegisterVariable (&lookspring);
	Cvar_RegisterVariable (&lookstrafe);
	Cvar_RegisterVariable (&sensitivity);

	Cvar_RegisterVariable (&uid_cvar);

// BorisU -->
	Cvar_RegisterVariable (&mapname);
	Cvar_RegisterVariable (&mapgroupname);
	Cvar_RegisterVariable (&date);
	Cvar_RegisterVariable (&timevar);
	Cvar_RegisterVariable (&date_format);
	Cvar_RegisterVariable (&time_format);
	Cvar_RegisterVariable (&cl_ping);
	Cvar_RegisterVariable (&cl_parsesay);
	Cvar_RegisterVariable (&cl_parseWhiteText);
	Cvar_RegisterVariable (&cl_digits_format);
	Cvar_RegisterVariable (&cl_nofake);
	Cvar_RegisterVariable (&cl_sbar_inv);
	Cvar_RegisterVariable (&cl_gamespy_showrules);
	Cvar_RegisterVariable (&cl_gamespy_showplayers);
// <-- BorisU

	Cvar_RegisterVariable (&m_pitch);
	Cvar_RegisterVariable (&m_yaw);
	Cvar_RegisterVariable (&m_forward);
	Cvar_RegisterVariable (&m_side);

	Cvar_RegisterVariable (&rcon_password);
	Cvar_RegisterVariable (&rcon_address);

//	Cvar_RegisterVariable (&entlatency);
	Cvar_RegisterVariable (&cl_predict_players2);
	Cvar_RegisterVariable (&cl_predict_players);
	Cvar_RegisterVariable (&cl_solid_players);

	Cvar_RegisterVariable (&localid);

	Cvar_RegisterVariable (&baseskin);
	Cvar_RegisterVariable (&noskins);
	
// Tonik -->
	Cvar_RegisterVariable (&cl_demotimescale);
	Cvar_RegisterVariable (&r_rocketlight);
	Cvar_RegisterVariable (&cl_muzzleflash);
	Cvar_RegisterVariable (&r_rockettrail);
	Cvar_RegisterVariable (&r_grenadetrail);
	Cvar_RegisterVariable (&r_explosiontype);
	Cvar_RegisterVariable (&r_explosionlight);
	Cvar_RegisterVariable (&r_powerupglow);
	Cvar_RegisterVariable (&cl_truelightning);
// <-- Tonik

// fuh -->
	Cvar_RegisterVariable (&r_lightflicker);
	Cvar_RegisterVariable (&r_rocketlightcolor);
	Cvar_RegisterVariable (&r_explosionlightcolor);
	Cvar_RegisterVariable (&r_flagcolor);
	Cvar_RegisterVariable (&cl_useproxy);
	Cvar_RegisterVariable (&sys_yieldcpu);
	Cvar_RegisterVariable (&cl_nolerp);
// <-- fuh
	//
	// info mirrors
	//
	Cvar_RegisterVariable (&name);
	Cvar_RegisterVariable (&password);
	Cvar_RegisterVariable (&spectator);
	Cvar_RegisterVariable (&skin);
	Cvar_RegisterVariable (&team);
	Cvar_RegisterVariable (&filter);
	Cvar_RegisterVariable (&topcolor);
	Cvar_RegisterVariable (&bottomcolor);
	Cvar_RegisterVariable (&rate);
	Cvar_RegisterVariable (&msg);

	// ezquake -->
	com_blockscripts = false;

//	Q_snprintfz(st, sizeof(st), "ezQuake %i", build_number());

	if (COM_CheckParm("-noscripts"))
	{
		com_blockscripts = true;
//		strcat(st, " noscripts");
	}

// 	Info_SetValueForStarKey (cls.userinfo, "*client", st, MAX_INFO_STRING);
// <-- ezquake

	Cmd_AddCommand ("version", CL_Version_f);
//	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommandTrig ("record", CL_Record_f);
	Cmd_AddCommandTrig ("rerecord", CL_ReRecord_f);
	Cmd_AddCommandTrig ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);

	Cmd_AddCommand ("skins", Skin_Skins_f);
	Cmd_AddCommand ("allskins", Skin_AllSkins_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);

// fuh -->
	Cmd_AddCommand ("join", CL_Join_f);
	Cmd_AddCommand ("observe", CL_Observe_f);
// <-- fuh

	Cmd_AddCommand ("gamespy", CL_GameSpy_f); // -=MD=-

	Cmd_AddCommandTrig ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("packet", CL_Packet_f);
	Cmd_AddCommandTrig ("user", CL_User_f);
	Cmd_AddCommandTrig ("users", CL_Users_f);

	Cmd_AddCommandTrig ("setinfo", CL_SetInfo_f);
	Cmd_AddCommand ("fullinfo", CL_FullInfo_f);
//	Cmd_AddCommand ("fullserverinfo", CL_FullServerinfo_f);

	Cmd_AddCommand ("color", CL_Color_f);
	Cmd_AddCommand ("download", CL_Download_f);

//	Cmd_AddCommand ("nextul", CL_NextUpload);
//	Cmd_AddCommand ("stopul", CL_StopUpload);

	Cmd_AddCommandTrig ("sayid", CL_SayID_f); // -=MD=-
//	Cmd_AddCommand ("version", CL_SayID_f);

// BorisU -->
	Cmd_AddCommand ("userdir", CL_Userdir_f);
	Cmd_AddCommand ("snd_restart", CL_SND_Restart_f);
	Cmd_AddCommand ("capture", CL_Capture_f); // surmoclient
// <--BorisU

//
// forward to server commands
//
	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("pause", NULL);
	Cmd_AddCommandTrig ("say_id", NULL); // -=MD=-
	Cmd_AddCommandTrig ("say", NULL);
	Cmd_AddCommandTrig ("say_team", NULL);
	Cmd_AddCommandTrig ("say_to_team", NULL); // BorisU
//	Cmd_AddCommand ("~say", NULL);
//	Cmd_AddCommand ("~say_true", NULL);
	Cmd_AddCommand ("detect", NULL); // -=MD=-
	Cmd_AddCommand ("serverinfo", NULL);

	Cvar_RegisterVariable (&cl_version);
	Cvar_SetROM (&cl_version, va("QW262 (rev.%d %s) %s:%s",
 								 MINOR_VERSION, RELEASE, PLATFORM, RENDER));
//
//  Windows commands
//
#ifdef _WIN32
	Cmd_AddCommand ("windows", CL_Windows_f);
#endif
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	vsprintf (string,message,argptr);
	va_end (argptr);

	Con_DPrintf ("Host_EndGame: %s\n",string);

	CL_Disconnect ();

	longjmp (host_abort, 1);
}

/*
================
Host_Error

This shuts down the client and exits qwcl
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;
	
	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;
	
	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	Con_Printf ("\n===========================\n");
	Con_Printf ("Host_Error: %s\n",string);
	Con_Printf ("===========================\n\n");

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;
	longjmp (host_abort, 1);
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
	FILE	*f;

	if (host_initialized)
	{
		if (UserdirSet)
			f = fopen (va("%s/config.cfg",com_userdir), "w");
		else
			f = fopen (va("%s/config.cfg",com_gamedir), "w");
		if (!f)
		{
			Con_Printf ("Couldn't write config.cfg.\n");
			return;
		}
		
		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		fclose (f);
	}
}


//============================================================================

// Tonik -->
static double CL_MinFrameTime (void)
{
	double fps, fpscap;

	if (cls.timedemo || capture_num >= 0 /*|| Movie_IsCapturing()*/)
		return 0;

	if (cls.demoplayback) {
		if (!cl_maxfps.value)
			return 0;
		fps = max (30.0, cl_maxfps.value);
	} else {
		fpscap = cl.maxfps ? max (30.0, cl.maxfps) : 72.0; // Rulesets_MaxFPS();

		if (cl_maxfps.value)
			fps = bound (30.0, cl_maxfps.value, fpscap);
		else
			fps = bound (30.0, rate.value/80.0, fpscap);
	}

	return 1 / fps;
}
// <-- Tonik

/*
==================
Host_Frame

Runs all active servers
==================
*/
int		nopacketcount;
void Host_Frame (float time)
{
	static double	time1 = 0;
	static double	time2 = 0;
	static double	time3 = 0;
	int				pass1, pass2, pass3;
	static double	time_set = 0;

	static double extratime = 0.001;
	double minframetime;

	if (setjmp (host_abort) )
		return;			// something bad happened, or the server disconnected

	curtime += time;

	extratime += time;
	minframetime = CL_MinFrameTime();

	if (extratime < minframetime) {
		extern cvar_t sys_yieldcpu;
		if (sys_yieldcpu.value)
#ifdef _WIN32
			Sys_MSleep(0);
#else
			Sys_MSleep(1);	
#endif
		return;
	}

	cls.trueframetime = extratime - 0.001;
	cls.trueframetime = max(cls.trueframetime, minframetime);
	extratime -= cls.trueframetime;

	if (capture_num >= 0)
		cls.frametime = 1.0/capture_fps;
	else
		cls.frametime = min(0.2, cls.trueframetime);

	if (cls.demoplayback) {
		if (cl.paused)
			cls.frametime = 0;
		else if (!cls.timedemo)
			cls.frametime *= bound(0, cl_demotimescale.value, 20);
	}

	cls.realtime += cls.frametime;

	if (!cl.paused)
		cl.time += cls.frametime;

// BorisU -->
	cbuf_current = &cbuf_main;

	if (curtime-time_set > 1.0) {
		Set_date_time_vars();
		time_set = curtime;
	}
// <-- BorisU   

	// get new key events
	Sys_SendKeyEvents ();

	// allow mice or other external controllers to add commands
	IN_Commands ();

	// process console commands
	Cbuf_ExecuteEx (&cbuf_main);

	// fetch results from server
	CL_ReadPackets ();

	R_UpdateSkins (); // BorisU

// Highlander -->
	if (cls.mvdplayback)
	{
		static float	old;
		extern	float	nextdemotime, olddemotime;

		player_state_t *self, *oldself;
		self = &cl.frames[cl.parsecount & UPDATE_MASK].playerstate[cl.playernum];
		oldself = &cl.frames[(cls.netchan.outgoing_sequence-1) & UPDATE_MASK].playerstate[cl.playernum];
		self->messagenum = cl.parsecount;
		VectorCopy(oldself->origin, self->origin);
		VectorCopy(oldself->velocity, self->velocity);
		VectorCopy(oldself->viewangles, self->viewangles);

		if (old != nextdemotime) // FIXME: use oldparcecount != cl.parsecount?
		{
			old = nextdemotime;
			CL_InitInterpolation(nextdemotime, olddemotime);
		}

		CL_ParseClientdata();
		
		cls.netchan.outgoing_sequence = cl.parsecount+1;
		CL_Interpolate();
	}
// <-- Highlander

	// Tonik
	// process stuffed commands
	Cbuf_ExecuteEx (&cbuf_svc);

	// send intentions now
	// resend a connection request if necessary
	if (cls.state == ca_disconnected) {
		CL_CheckForResend ();
	} else
		CL_SendCmd ();

	if (cls.state >= ca_onserver) { // !!! Tonik

		Cam_SetViewPlayer ();
		// Set up prediction for other players
		CL_SetUpPlayerPrediction(false);

		// do client side motion prediction
		CL_PredictMove ();

		// Set up prediction for other players
		CL_SetUpPlayerPrediction(true);

		// build a refresh entity list
		CL_EmitEntities ();
	}

	// update video
	if (host_speeds.value)
		time1 = Sys_DoubleTime ();

	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_DoubleTime ();
		
	// update audio
	if (cls.state == ca_active)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	
	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Print_flags[Print_current] |= PR_TR_SKIP;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}

	if(capture_num >= 0) {
		SCR_MovieShot_f(); // surmoclient
	}

	cls.framecount++;
	fps_count++;
}

static void simple_crypt(char *buf, int len)
{
	while (len--)
		*buf++ ^= 0xff;
}

void Host_FixupModelNames(void)
{
	simple_crypt(emodel_name, sizeof(emodel_name) - 1);
	simple_crypt(pmodel_name, sizeof(pmodel_name) - 1);
	
	simple_crypt(prespawn_name,  sizeof(prespawn_name)  - 1);
	simple_crypt(modellist_name, sizeof(modellist_name) - 1);
	simple_crypt(soundlist_name, sizeof(soundlist_name) - 1);
}

//============================================================================

/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
	COM_InitArgv (parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("qw");

	Sys_mkdir("qw");

#ifdef QW262
	if (COM_CheckParm ("-ver262"))
		IsMDserver=true;
#endif
// <-- BorisU

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	if (parms->memsize < MINIMUM_MEMORY)
		Sys_Error ("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);

	Memory_Init (parms->membase, parms->memsize);
#ifdef EMBED_TCL
	// interpreter should be initialized
	// before any cvar definitions
	TCL_InterpInit ();
#endif
	Cbuf_Init ();
	Cmd_Init ();
	Cvar_Init ();

	V_Init ();

	COM_Init ();

	Host_FixupModelNames();
	
	NET_Init (PORT_CLIENT);
	Netchan_Init ();

	Sys_Init (); // Fuh

	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();
	M_Init ();
	Mod_Init ();

#ifdef USE_AUTH
	Modules_Init();
	FChecks_Init();	// ezquake
#endif

//	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	Con_Printf ("%4.1f megs RAM used.\n",parms->memsize/ (1024*1024.0));
	
	R_InitTextures ();

	host_basepal = (byte *)COM_LoadHunkFile ("gfx/palette.lmp");
	if (!host_basepal)
		Sys_Error ("Couldn't load gfx/palette.lmp");

#ifdef USE_AUTH
	FMod_CheckModel("gfx/palette.lmp", host_basepal, com_filesize);
#endif

	host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp");
	if (!host_colormap)
		Sys_Error ("Couldn't load gfx/colormap.lmp");

#ifdef USE_AUTH
	FMod_CheckModel("gfx/colormap.lmp", host_colormap, com_filesize);
#endif

#ifndef _WIN32
	IN_Init ();
	CDAudio_Init ();
	VID_Init (host_basepal);
	Draw_Init ();
	SCR_Init ();
	R_Init ();

#ifdef __APPLE__
	S_Init ();		// S_Init is now done as part of VID. Sigh.
#endif

	cls.state = ca_disconnected;
	Sbar_Init ();

	CL_Init ();
#else
	VID_Init (host_basepal);

// Sergio (printscr) -->
	id_PrintScr = GlobalAddAtom("K_PRINTSCR"); // get unique id for hot-key
	RegisterHotKey(mainwindow, id_PrintScr, 0, VK_SNAPSHOT);
// <-- Sergio

	Draw_Init ();
	SCR_Init ();
	R_Init ();
//	S_Init ();		// S_Init is now done as part of VID. Sigh.
#ifdef GLQUAKE
	S_Init();
#endif

	cls.state = ca_disconnected;
	CDAudio_Init ();
	Sbar_Init ();
// BorisU -->
#ifdef GLQUAKE
	base_numgltextures = numgltextures;
#endif
// <-- BorisU
	CL_Init ();
	IN_Init ();
	MH_Init ();
#endif /* !_WIN32 */

	CL_InitLocFiles ();
	TP_Init();
	Hud_Init();
	CL_InitTriggers ();

#ifdef EMBED_TCL
	if (!TCL_InterpLoaded())
		Con_Printf ("Could not load "TCL_LIB_NAME", embedded Tcl disabled\n");
#endif

	Demo_Init(); // fuh
#ifdef USE_AUTH
	Auth_Init(); // ezquake
#endif
	Cbuf_InsertText ("exec quake.rc\n262_quakerc\n");
//	Cbuf_AddText ("echo Type connect <internet address> or use GameSpy to connect to a game.\n");
	Cbuf_AddText ("cl_warncmd 1\n");

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	host_initialized = true;

	Con_Printf ("\nÑ×”˜” by ”wð‘ÂorisÕ\nhttp://qw262.da.ru/\n\n");
//	Con_Printf ("€ ­½ÍÄ½­ Edition of QuakeWorld ‚\n"
//				"         Console editor by Ž™‘ŽÓendeÒ\n"
//				);	
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;
	
	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	Host_WriteConfiguration (); 
		
	CDAudio_Shutdown ();
	NET_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();

#ifdef _WIN32
	MH_Shutdown();
#endif

#ifdef EMBED_TCL
	TCL_Shutdown ();
#endif

// Sergio -->
#ifdef _WIN32
	if (mainwindow) {
		UnregisterHotKey(mainwindow, id_PrintScr);
		GlobalDeleteAtom((ATOM)id_PrintScr);
	}
#endif
// <-- Sergio

	if (host_basepal)
		VID_Shutdown();
}


typedef struct {
	char	*name;
	void	(*func) (void);
} svcmd_t;

svcmd_t svcmds[] =
{
	{"changing", CL_Changing_f},
	{"fullserverinfo", CL_FullServerinfo_f},
	{"nextul", CL_NextUpload},
	{"stopul", CL_StopUpload},
//	{"fov", CL_Fov_f},
//	{"r_drawviewmodel", CL_R_DrawViewModel_f},
	{NULL, NULL}
};

/*
================
CL_CheckServerCommand

Called by Cmd_ExecuteString if cbuf_current==&cbuf_svc
================
*/
qboolean CL_CheckServerCommand ()
{
	svcmd_t	*cmd;
	char	*s;

	s = Cmd_Argv (0);
	for (cmd=svcmds ; cmd->name ; cmd++)
		if (!strcmp (s, cmd->name) ) {
			cmd->func ();
			return true;
		}

	return false;
}
