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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value
	
	"svc_serverdata",		// [long] version ...
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",		// [byte] impact [byte] blood [vec3] from
	
	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",
	
	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",

	"svc_cdtrack",
	"svc_sellscreen",

	"svc_smallkick",
	"svc_bigkick",

	"svc_updateping",
	"svc_updateentertime",

	"svc_updatestatlong",
	"svc_muzzleflash",
	"svc_updateuserinfo",
	"svc_download",
	"svc_playerinfo",
	"svc_nails",
	"svc_choke",
	"svc_modellist",
	"svc_soundlist",
	"svc_packetentities",
 	"svc_deltapacketentities",
	"svc_maxspeed",
	"svc_entgravity",

	"svc_setinfo",
	"svc_serverinfo",
	"svc_updatepl",

	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",

	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL"
};

int	oldparsecountmod;
int	parsecountmod;
double	parsecounttime;

//=============================================================================

int packet_latency[NET_TIMINGS];
int packet_loss;

int CL_CalcNet (void)
{
	int		a, i;
	frame_t	*frame;
	int lost;

	for (i=cls.netchan.outgoing_sequence-UPDATE_BACKUP+1
		; i <= cls.netchan.outgoing_sequence
		; i++)
	{
		frame = &cl.frames[i&UPDATE_MASK];
		if (frame->receivedtime == -1)
			packet_latency[i&NET_TIMINGSMASK] = 9999;	// dropped
		else if (frame->receivedtime == -2)
			packet_latency[i&NET_TIMINGSMASK] = 10000;	// choked
		else if (frame->invalid)
			packet_latency[i&NET_TIMINGSMASK] = 9998;	// invalid delta
		else
			packet_latency[i&NET_TIMINGSMASK] = (frame->receivedtime - frame->senttime)*20;
	}

	lost = 0;
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (cls.netchan.outgoing_sequence-a) & NET_TIMINGSMASK;
		if (packet_latency[i] == 9999)
			lost++;
	}
	return (packet_loss = lost * 100 / NET_TIMINGS);
}

//=============================================================================

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean	CL_CheckOrDownloadFile (char *filename)
{
	FILE	*f;

	if (strstr (filename, ".."))
	{
		Con_Printf ("Refusing to download a path with ..\n");
		return true;
	}

	COM_FOpenFile (filename, &f);
	if (f)
	{	// it exists, no need to download
		fclose (f);
		return true;
	}

	//ZOID - can't download when recording
	if (cls.demorecording) {
		Con_Printf("Unable to download %s in record mode.\n", cls.downloadname);
		return true;
	}
	//ZOID - can't download when playback
	if (cls.demoplayback)
		return true;

	strcpy (cls.downloadname, filename);
	Con_Printf ("Downloading %s...\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("download %s", cls.downloadname));

	cls.downloadnumber++;

	return false;
}

/*
=================
Model_NextDownload
=================
*/
void Model_NextDownload (void)
{
	char	*s;
	int		i;
	extern	char gamedirfile[];

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking models...\n");
		cls.downloadnumber = 1;
#ifdef GLQUAKE
		lightmap_bytes = gl_colorlights.value ? 3 : 1;
#endif
	}

	cls.downloadtype = dl_model;
	for ( 
		; cl.model_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.model_name[cls.downloadnumber];
			
		if (s[0] == '*')
			continue;	// inline brush model
		if (!CL_CheckOrDownloadFile(s))
			return;		// started a download
		if (cls.downloadnumber == 1) {
			char mapnamestr[MAX_QPATH];
			COM_StripExtension (COM_SkipPath (cl.model_name[1]), mapnamestr);
			Cvar_SetROM (&mapname, mapnamestr);
		}
	}

	for (i=1 ; i<MAX_MODELS ; i++)
	{
		if (!cl.model_name[i][0])
			break;

		cl.model_precache[i] = Mod_ForName (cl.model_name[i], false);

		if (!cl.model_precache[i])
		{
			Con_Printf ("\nThe required model file '%s' could not be found or downloaded.\n\n"
				, cl.model_name[i]);
			Con_Printf ("You may need to download or purchase a %s client "
				"pack in order to play on this server.\n\n", gamedirfile);
			CL_Disconnect ();
			return;
		}
	}

	// all done
	cl.worldmodel = cl.model_precache[1];	
	R_NewMap ();
	Hunk_Check ();		// make sure nothing is hurt

	// done with modellist, request first of static signon messages
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
//	MSG_WriteString (&cls.netchan.message, va("prespawn %i 0 %i", cl.servercount, cl.worldmodel->checksum2));
	MSG_WriteString (&cls.netchan.message, va(prespawn_name, cl.servercount, cl.worldmodel->checksum2));
}

/*
=================
Sound_NextDownload
=================
*/
void Sound_NextDownload (void)
{
	char	*s;
	int		i;

	if (cls.downloadnumber == 0)
	{
		Con_Printf ("Checking sounds...\n");
		cls.downloadnumber = 1;
	}

	cls.downloadtype = dl_sound;
	for ( 
		; cl.sound_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.sound_name[cls.downloadnumber];
		if (!CL_CheckOrDownloadFile(va("sound/%s",s)))
			return;		// started a download
	}

	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.sound_name[i][0])
			break;
		cl.sound_precache[i] = S_PrecacheSound (cl.sound_name[i]);
	}

	// done with sounds, request models now
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	for (i = 0; i < cl_num_modelindices; i++)
		cl_modelindices[i] = -1;
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
//	MSG_WriteString (&cls.netchan.message, va("modellist %i 0", cl.servercount));
	MSG_WriteString (&cls.netchan.message, va(modellist_name, cl.servercount, 0));
}


/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload (void)
{
	switch (cls.downloadtype)
	{
	case dl_single:
		break;
	case dl_skin:
		Skin_NextDownload ();
		break;
	case dl_model:
		Model_NextDownload ();
		break;
	case dl_sound:
		Sound_NextDownload ();
		break;
	case dl_none:
	default:
		Con_DPrintf("Unknown download type.\n");
	}
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	byte	name[1024];
	int		r;
	static float time = 0;
	static int s = 0;

	// read the data
	size = MSG_ReadShort ();
	percent = MSG_ReadByte ();

	s += size;
	if (cls.realtime - time > 1) {
		cls.downloadrate = s/(1024*(cls.realtime - time));
		time = cls.realtime;
		s = 0;
	}

	if (cls.demoplayback) {
		if (size > 0)
			msg_readcount += size;
		return; // not in demo playback
	}

	if (size == -1) {
		Con_Printf ("File not found.\n");
		if (cls.download) {
			Con_Printf ("cls.download shouldn't have been set\n");
			fclose (cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		if (strncmp(cls.downloadtempname,"skins/",6))
			Q_snprintfz (name, sizeof(name), "%s/%s", com_gamedir, cls.downloadtempname);
		else
			Q_snprintfz (name, sizeof(name), "qw/%s", cls.downloadtempname);

		COM_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			msg_readcount += size;
			Con_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message.data + msg_readcount, 1, size, cls.download);
	msg_readcount += size;

	if (percent != 100)
	{
// change display routines by zoid
		// request next block
#if 0
		Con_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Con_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

#if 0
		Con_Printf ("100%%\n");
#endif

		fclose (cls.download);

		// rename the temp file to it's final name
		if (strcmp(cls.downloadtempname, cls.downloadname)) {
			if (strncmp(cls.downloadtempname,"skins/",6)) {
				sprintf (oldn, "%s/%s", com_gamedir, cls.downloadtempname);
				sprintf (newn, "%s/%s", com_gamedir, cls.downloadname);
			} else {
				sprintf (oldn, "qw/%s", cls.downloadtempname);
				sprintf (newn, "qw/%s", cls.downloadname);
			}
			r = rename (oldn, newn);
			if (r)
				Con_Printf ("failed to rename.\n");
		}

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

/*
=====================================================================

  UPLOAD FILE FUNCTIONS

=====================================================================
*/

void CL_NextUpload(void)
{
	static byte	buffer[FILE_TRANSFER_BUF_SIZE];
	int		r;
	int		percent;
	int		size;
	static int s = 0;
	static double	time;

	if ((!cls.is_file && !cls.mem_upload) || (cls.is_file && !cls.upload))
		return;

	r = min(cls.upload_size - cls.upload_pos, sizeof(buffer));
	MSG_WriteByte (&cls.netchan.message, clc_upload);
	MSG_WriteShort (&cls.netchan.message, r);

	if (cls.is_file) {
		fread(buffer, 1, r, cls.upload);
	} else {
		memcpy(buffer, cls.mem_upload + cls.upload_pos, r);
	}
	cls.upload_pos += r;
	size = cls.upload_size ? cls.upload_size : 1;
	percent = cls.upload_pos * 100 / size;
	cls.uploadpercent = percent;
	MSG_WriteByte (&cls.netchan.message, percent);
	SZ_Write (&cls.netchan.message, buffer, r);

	Con_DPrintf ("UPLOAD: %6d: %d written\n", cls.upload_pos - r, r);

	s += r;
	if (cls.realtime - time > 1) {
		cls.uploadrate = s/(1024*(cls.realtime - time));
		time = cls.realtime;
		s = 0;
	}
	if (cls.upload_pos != cls.upload_size)
		return;

	Con_Printf ("Upload completed\n");

	if (cls.is_file) {
		fclose (cls.upload);
		cls.upload = NULL;
	} else {
		free(cls.mem_upload);
		cls.mem_upload = 0;
	}
	cls.upload_pos = 0;
	cls.upload_size = 0;
}

void CL_StartUpload (byte *data, int size)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected

	cls.is_file = false;

	// override
	if (cls.mem_upload)
		free(cls.mem_upload);
	cls.mem_upload = malloc (size);
	memcpy(cls.mem_upload, data, size);
	cls.upload_size = size;
	cls.upload_pos = 0;
	Con_Printf ("Upload starting of %d...\n", cls.upload_size);

	CL_NextUpload();
}

void CL_StartFileUpload (void)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected
	cls.is_file = true;
	// override
	if (cls.upload) {
		fclose(cls.upload);
		cls.upload = NULL;
	}
	strlcpy(cls.uploadname, Cmd_Argv(2), sizeof(cls.uploadname));
	cls.upload = fopen(cls.uploadname, "rb"); // BINARY
	if (!cls.upload) {
		Con_Printf ("Bad file \"%s\"\n", cls.uploadname);
		return;
	}
	cls.upload_size = COM_FileLength(cls.upload);
	cls.upload_pos = 0;
	Con_Printf ("Upload starting: %s (%d bytes)...\n", cls.uploadname, cls.upload_size);
	CL_NextUpload();
}

qboolean CL_IsUploading(void)
{
	if ((!cls.is_file && cls.mem_upload) || (cls.is_file && cls.upload))
		return true;
	return false;
}

void CL_StopUpload(void)
{
	if (cls.is_file) {
		if (cls.upload) {
			fclose(cls.upload);
			cls.upload = NULL;
		}
	} else {
		if (cls.mem_upload) {
			free(cls.mem_upload);
			cls.mem_upload = NULL;
		}
	}
}

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

void ResetTeamFortress (void)
{
		extern cvar_t	v_iyaw_cycle, v_iroll_cycle, v_ipitch_cycle,
			v_iyaw_level, v_iroll_level, v_ipitch_level, v_idlescale;

		cbuf_current = &cbuf_svc;	// hack on :)
		Cvar_SetValue (&v_iyaw_cycle, 2);
		Cvar_SetValue (&v_iroll_cycle, 0.5);
		Cvar_SetValue (&v_ipitch_cycle, 1);
		Cvar_SetValue (&v_iyaw_level, 0.3);
		Cvar_SetValue (&v_iroll_level, 0.1);
		Cvar_SetValue (&v_ipitch_level, 0.3);
		Cvar_SetValue (&v_idlescale, 0);
		cbuf_current = &cbuf_main; // hack off ;)
		if (mtfl && v_gamma.value > 0.55)
			Cvar_SetValue (&v_gamma, 0.55);
}

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	char			*str;
	qboolean		cflag = false;
	extern	char	gamedirfile[MAX_OSPATH];
	int				protover;
	extern float	nextdemotime, olddemotime;
	
	static qboolean	first_connect = true; // BorisU

	Con_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
// allow 2.2 and 2.29 demos to play
	protover = MSG_ReadLong ();

#ifdef QW262
	if(protover != (IsMDserver ? PROTOCOL_VERSION : OLD_PROTOCOL_VERSION)) {
	if ((protover == OLD_PROTOCOL_VERSION && IsMDserver) ||
		(protover == PROTOCOL_VERSION && !IsMDserver)) {
		IsMDserver = !IsMDserver;
	} else if (!(cls.demoplayback && (protover == 26 || protover == 27)))
			Host_EndGame ("Server returned version %i\nYou probably need to upgrade.\n", protover);
		else
			IsMDserver = false;
	}
#else
	if (protover != OLD_PROTOCOL_VERSION) {
		if (!(cls.demoplayback && (protover == 26 || protover == 27)))
			Host_EndGame ("Server returned version %i, not %i\nYou probably need to upgrade.\n", protover, PROTOCOL_VERSION);
	}
#endif

	cl.servercount = MSG_ReadLong ();

	// game directory
	str = MSG_ReadString ();
	cl.teamfortress = !Q_stricmp(str, "fortress"); // BorisU

	if (cl.teamfortress) 
		ResetTeamFortress();

	cbuf_current = &cbuf_main;
	if (Q_stricmp(gamedirfile, str)) {
		// save current config
		Host_WriteConfiguration (); 
		cflag = true;
	}

	COM_Gamedir(str);

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if (cflag || first_connect) {
// conditional execution of configs is not necessary? BorisU
//		sprintf(fn, "%s/config.cfg", com_gamedir);
//		if ((f = fopen(fn, "r")) != NULL) {
//			fclose(f);
			Cbuf_AddText("cl_warncmd 0\nexec config.cfg\nexec frontend.cfg\n262_autoexec\ncl_warncmd 1\n");
			Cbuf_Execute();
			first_connect = false;
//		}
	}

	if (cls.mvdplayback) {
		cls.netchan.last_received = nextdemotime = olddemotime = MSG_ReadFloat();
		cl.playernum = MAX_CLIENTS - 1;
		cl.spectator = true;
	} else {
		// parse player slot, high bit means spectator
		cl.playernum = MSG_ReadByte ();
		if (cl.playernum & 128)
		{
			cl.spectator = true;
			cl.playernum &= ~128;
		}
	}

	// get the full level name
	str = MSG_ReadString ();
	strlcpy (cl.levelname, str, sizeof(cl.levelname));

	// get the movevars
	movevars.gravity			= MSG_ReadFloat();
	movevars.stopspeed          = MSG_ReadFloat();
	movevars.maxspeed           = MSG_ReadFloat();
	movevars.spectatormaxspeed  = MSG_ReadFloat();
	movevars.accelerate         = MSG_ReadFloat();
	movevars.airaccelerate      = MSG_ReadFloat();
	movevars.wateraccelerate    = MSG_ReadFloat();
	movevars.friction           = MSG_ReadFloat();
	movevars.waterfriction      = MSG_ReadFloat();
	movevars.entgravity         = MSG_ReadFloat();

	// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

	// ask for the sound list next
	memset(cl.sound_name, 0, sizeof(cl.sound_name));
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
//	MSG_WriteString (&cls.netchan.message, va("soundlist %i 0", cl.servercount));
	MSG_WriteString (&cls.netchan.message, va(soundlist_name, cl.servercount, 0));

	// now waiting for downloads, etc
	cls.state = ca_onserver;
}

/*
==================
CL_ParseSoundlist
==================
*/
void CL_ParseSoundlist (void)
{
	int	numsounds;
	char	*str;
	int n;

// precache sounds
//	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));

	numsounds = MSG_ReadByte();

	for (;;) {
		str = MSG_ReadString ();
		if (!str[0])
			break;
		numsounds++;
		if (numsounds == MAX_SOUNDS)
			Host_EndGame ("Server sent too many sound_precache");
		strcpy (cl.sound_name[numsounds], str);
	}

	n = MSG_ReadByte();

	if (n) {
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
//		MSG_WriteString (&cls.netchan.message, va("soundlist %i %i", cl.servercount, n));
		MSG_WriteString (&cls.netchan.message, va(soundlist_name, cl.servercount, n));
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_sound;
	Sound_NextDownload ();
}

/*
==================
CL_ParseModellist
==================
*/
void CL_ParseModellist (void)
{
	int	nummodels;
	char	*str;
	int n;

// precache models and note certain default indexes
	nummodels = MSG_ReadByte();

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		nummodels++;
		if (nummodels==MAX_MODELS)
			Host_EndGame ("Server sent too many model_precache");

		strlcpy (cl.model_name[nummodels], str, sizeof(cl.model_name[nummodels]));

		for (n = 0; n < cl_num_modelindices; n++) {
			if (!strcmp(cl_modelnames[n], cl.model_name[nummodels])) {
				cl_modelindices[n] = nummodels;
				break;
			}
		}

	}

	n = MSG_ReadByte();

	if (n) {
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, va(modellist_name, cl.servercount, n));
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_model;
	Model_NextDownload ();
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_state_t *es)
{
	int			i;
	
	es->modelindex = MSG_ReadByte ();
	es->frame = MSG_ReadByte ();
	es->colormap = MSG_ReadByte();
	es->skinnum = MSG_ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		es->origin[i] = MSG_ReadCoord ();
		es->angles[i] = MSG_ReadAngle ();
	}
}



/*
=====================
CL_ParseStatic

Static entities are non-interactive world objects
like torches
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	int		i;
	entity_state_t	es;

	CL_ParseBaseline (&es);
		
	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_EndGame ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;

// copy it to the current state
	ent->model = cl.model_precache[es.modelindex];
	ent->frame = es.frame;
	ent->colormap = vid.colormap;
	ent->skinnum = es.skinnum;

	VectorCopy (es.origin, ent->origin);
	VectorCopy (es.angles, ent->angles);
	
	R_AddEfrags (ent);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}



/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    float 	attenuation;  
 	int		i;
	           
    channel = MSG_ReadShort(); 

    if (channel & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (channel & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	sound_num = MSG_ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();
 
	ent = (channel>>3)&1023;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);
	
    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}       


/*
==================
CL_ParseClientdata

Server information pertaining to this client only, sent every frame
==================
*/
void CL_ParseClientdata (void)
{
	int				i;
	float		latency;
	frame_t		*frame;

// calculate simulated time of message
	oldparsecountmod = parsecountmod;

	i = cls.netchan.incoming_acknowledged;
	if (cls.mvdplayback) // Highlander
		cl.oldparsecount = i - 1;
	cl.parsecount = i;
	i &= UPDATE_MASK;
	parsecountmod = i;
	frame = &cl.frames[i];
	if (cls.mvdplayback) // Highlander
		frame->senttime = cls.realtime - cls.frametime;//cls.realtime;
	parsecounttime = cl.frames[i].senttime;

	frame->receivedtime = cls.realtime;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
//		Con_Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
	// drift the average latency towards the observed latency
		if (latency < cls.latency)
			cls.latency = latency;
		else
			cls.latency += 0.001;	// drift up, so correction are needed
	}	
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_ProcessUserInfo (int slot, player_info_t *player)
{
	strlcpy (player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name));
// BorisU -->
	strlcpy (player->longname, Info_ValueForKey (player->userinfo, "name"), sizeof(player->longname));
// Hack: long name in userinfo is truncated to 31 chars
	Info_SetValueForKey(player->userinfo,"name",player->longname,MAX_INFO_STRING);
// <-- BorisU
	player->topcolor = atoi(Info_ValueForKey (player->userinfo, "topcolor"));
	player->bottomcolor = atoi(Info_ValueForKey (player->userinfo, "bottomcolor"));
	strlcpy (player->team, Info_ValueForKey (player->userinfo, "team"), sizeof(player->team));

	if (Info_ValueForKey (player->userinfo, "*spectator")[0])
		player->spectator = true;
	else
		player->spectator = false;

	if (cls.state == ca_active)
		Skin_Find (player);

	Sbar_Changed ();
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_UpdateUserinfo (void)
{
	int		slot;
	player_info_t	*player;

	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];
	player->userid = MSG_ReadLong ();
	strncpy (player->userinfo, MSG_ReadString(), sizeof(player->userinfo)-1);

	CL_ProcessUserInfo (slot, player);
}

/*
==============
CL_SetInfo
==============
*/
void CL_SetInfo (void)
{
	int		slot;
	player_info_t	*player;
	char key[MAX_MSGLEN];
	char value[MAX_MSGLEN];
	extern cvar_t team;
	
	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_setinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];

	strncpy (key, MSG_ReadString(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	strncpy (value, MSG_ReadString(), sizeof(value) - 1);
	value[sizeof(value) - 1] = 0;

	Info_SetValueForKey (player->userinfo, key, value, MAX_INFO_STRING);

	if ( ((!cl.teamfortress || !strcmp(team.string, Info_ValueForKey(player->userinfo, "team")) ||
		 cl.spectator || cls.demoplayback ||
		(strcmp(key,"skin") &&
		 strcmp(key,"topcolor") && 
		 strcmp(key,"bottomcolor"))) &&
		 key[0] != '_' ) &&
		 ( strcmp(key,"apw") && strcmp(key,"adminpwd")) ) {
		char buf[80];
		Con_DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);
		strcpy(buf,"f_setinfo_");
		strcpy(buf+10,key);
		if (strstr(cl_setinfo_triggers.string,key))
			CL_ExecTriggerSetinfo (buf, player->longname, value, va("%i",player->userid));
	}

	CL_ProcessUserInfo (slot, player);
}

/*
=====================
CL_SetStat
=====================
*/
void CL_SetStat (int stat, int value)
{
	int	j;
	if (stat < 0 || stat >= MAX_CL_STATS)
		Sys_Error ("CL_SetStat: %i is invalid", stat);

	if (cls.mvdplayback) {
		cl.players[cls.lastto].stats[stat]=value;
		if ( Cam_TrackNum() != cls.lastto )
			return;
	}

	Sbar_Changed ();
	
	if (stat == STAT_ITEMS)
	{	// set flash times
//		Sbar_Changed (); commented by BorisU
		for (j=0 ; j<32 ; j++)
			if ( (value & (1<<j)) && !(cl.stats[stat] & (1<<j)))
				cl.item_gettime[j] = cl.time; 
	}
	CL_StatChanged(stat, value);	// Tonik Triggers...
	cl.stats[stat] = value;
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash (void)
{
	vec3_t		fv, rv, uv;
	dlight_t	*dl;
	int			i;
	player_state_t	*pl;

	i = MSG_ReadShort ();

	if (!cl_muzzleflash.value)
		return;
	
	if ((unsigned)(i-1) >= MAX_CLIENTS)
		return;

#ifdef GLQUAKE
	// don't draw our own muzzle flash in gl if flashblending
	if (i-1 == cl.playernum && gl_flashblend.value)
		return;
#endif

	if (cl_muzzleflash.value == 2 && i-1 == cl.viewplayernum)
		return;
	
	pl = &cl.frames[cls.mvdplayback ? oldparsecountmod : parsecountmod].playerstate[i-1];

	dl = CL_AllocDlight (-i);
	VectorCopy (pl->origin,  dl->origin);
	AngleVectors (pl->viewangles, fv, rv, uv);
		
	VectorMA (dl->origin, 18, fv, dl->origin);
	dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time + 0.1;
	dl->type = lt_muzzleflash;
}

// BorisU -->
// Tonik's code
float last_f_version=-20.0;
void CL_CheckVersionRequest(char *s)
{
	extern cvar_t cl_version;

	if (cls.realtime - last_f_version < 20.0) 
		return;

	while (1)
	{
		switch (*s++)
		{
		case 0:
		case '\n':
			return;
		case ':':
			goto ok;
		}
	}
	return;

ok:
	if (!strncmp(s, " f_version\n", 11)) {
		Cbuf_AddText ("say ");
		Cbuf_AddText (cl_version.string);
#ifdef EMBED_TCL
		Cbuf_AddText ("/Tcl");
#endif
		switch (mtfl){
		case 1: Cbuf_AddText ("+\n"); break;
		case 2: Cbuf_AddText ("*\n"); break;
		default: Cbuf_AddText ("\n");
		}
		last_f_version = cls.realtime;
	}
}
// <-- BorisU

#ifdef QW262
#include "cl_parse262.inc"
#endif

// Tonik's code
/*
==============
CL_ProcessServerInfo

Called by CL_FullServerinfo_f and CL_ParseServerInfoChange
==============
*/
void CL_ProcessServerInfo (void)
{
	char	*p;
	int		fpd;

	cl.maxfps = Q_atof(Info_ValueForKey(cl.serverinfo, "maxfps"));
	movevars.bunnyspeedcap = Q_atof(Info_ValueForKey(cl.serverinfo, "pm_bunnyspeedcap"));

	if (pm_jumpfix.value < 2) {
		p = Info_ValueForKey(cl.serverinfo, "pm_jumpfix");
		if (p && *p)
			pm_jumpfix.value = Q_atof(p);
	}

	if (cls.demoplayback)
		fpd = 0;
	else
		fpd = atoi(Info_ValueForKey(cl.serverinfo, "fpd"));

	if (fpd != cl.fpd){
		Con_Printf("ÆÐÄ settings:\n");
		if (fpd & (FPD_NO_TEAM_MACROS | FPD_NO_SOUNDTRIGGERS | FPD_NO_TIMERS |
					FPD_LIMIT_PITCH | FPD_LIMIT_YAW)) {
			if (fpd & FPD_NO_TEAM_MACROS)
				Con_Printf("    team macros disabled\n");
			if (fpd & FPD_NO_SOUNDTRIGGERS)
				Con_Printf("    triggers disabled\n");
			if (fpd & FPD_NO_TIMERS)
				Con_Printf("    timers disabled\n");
			if (fpd & FPD_LIMIT_PITCH)
				Con_Printf("    script RJ disabled\n");
			if (fpd & FPD_LIMIT_YAW)
				Con_Printf("    script FWRJ disabled\n");
		}
		cl.fpd = fpd;
	}
	

#ifdef GLQUAKE
	cls.allow24bit = 0;

	if (gl_use_24bit_textures.value)
		cls.allow24bit |= ALLOW_24BIT_TEXTURES;

	if (!strcmp(Info_ValueForKey(cl.serverinfo, "24bit_fbs"), "1") || 
		!strcmp(Info_ValueForKey(cl.serverinfo, "allow_24bit_fullbrights"), "1") 
		|| cls.demoplayback)
		cls.allow24bit |= ALLOW_24BIT_FB_TEXTURES;

	if (cls.demoplayback || cl.spectator)
		cls.allow_transparent_water = 1;
	else
		cls.allow_transparent_water = Q_atoi(Info_ValueForKey(cl.serverinfo, "watervis"));

	if (cls.allow_transparent_water)
		Cvar_SetValue(&r_wateralpha, r_wateralpha.value);
#endif

	if (cls.demoplayback) {
		cl.fbskins = 1;
		cl.truelightning = 1;
	} else {
		p = Info_ValueForKey(cl.serverinfo, "fbskins");
		if (p && *p) {
			cl.fbskins = bound(0, Q_atof(p), 1);
			if (cl.fbskins)
				Con_Printf("Fullbright skins are allowed! (%g)\n", cl.fbskins);
		} else
			cl.fbskins = 0;
		p = Info_ValueForKey(cl.serverinfo, "truelightning");
		if (p && *p) {
			cl.truelightning = bound(0, Q_atof(p), 1);
			if (cl.truelightning)
				Con_Printf("Truelightning is allowed! (%g)\n", cl.truelightning);
		} else
			cl.truelightning = 0;
	}

	cl.teamplay = atoi(Info_ValueForKey(cl.serverinfo, "teamplay"));
	
	p = Info_ValueForKey(cl.serverinfo, "team1");
	if (!*p)
		p = Info_ValueForKey(cl.serverinfo, "t1");
	if (*p)
		strlcpy(loc_teamnames[0], p, 5);
	else
		strcpy(loc_teamnames[0], "blue");

	p = Info_ValueForKey(cl.serverinfo, "team2");
	if (!*p)
		p = Info_ValueForKey(cl.serverinfo, "t2");
	if (*p)
		strlcpy(loc_teamnames[1], p, 5);
	else
		strcpy(loc_teamnames[1], "red");

	p = Info_ValueForKey(cl.serverinfo, "team3");
	if (!*p)
		p = Info_ValueForKey(cl.serverinfo, "t3");
	if (*p)
		strlcpy(loc_teamnames[2], p, 5);
	else
		strcpy(loc_teamnames[2], "yell");

	p = Info_ValueForKey(cl.serverinfo, "team4");
	if (!*p)
		p = Info_ValueForKey(cl.serverinfo, "t4");
	if (*p)
		strlcpy(loc_teamnames[3], p, 5);
	else
		strcpy(loc_teamnames[3], "gren");

}

/*
==============
CL_ParseServerInfoChange
==============
*/
void CL_ParseServerInfoChange (void)
{
	char key[MAX_INFO_STRING];
	char value[MAX_INFO_STRING];

	strlcpy (key, MSG_ReadString(), sizeof(key));
	strlcpy (value, MSG_ReadString(), sizeof(value));

	Con_DPrintf ("SERVERINFO: %s=%s\n", key, value);

	Info_SetValueForKey (cl.serverinfo, key, value, MAX_SERVERINFO_STRING);

	CL_ProcessServerInfo ();
}

/*
==============
CL_ParsePrint
==============
*/
void CL_ParsePrint (int level, char *s)
{
	static char sprint_buf[2048];
	char	str[2048];
	char	*p;
	int		i,len;

	int		flags = 0, offset = 0;

	if (cl_nofake.value == 1 || (cl_nofake.value == 2 && (*s != '(' && *s != '<'))) {
		for (p = s; *p; p++)
			if (*p == 13)
				*p = '#'; 
	}

	strlcat (sprint_buf, s, sizeof(sprint_buf));

	while ( (p=strchr(sprint_buf, '\n')) != NULL ) {
		len = p - sprint_buf + 1;
		memcpy(str, sprint_buf, len);
		str[len] = '\0';
		strcpy (sprint_buf, p+1);
		
		if (level == PRINT_CHAT)
		{
			i=strlen(str);
			if(!IsMDserver && i>4 && str[i-5]=='#') {
				str[i-1]='\0';
				if(*filter.string && !strstr(filter.string,str+i-5))
					break;
				*(short int *)(str+i-5)='\n';
			}
			Print_flags[Print_current] |= PR_IS_CHAT;
#ifdef USE_AUTH
			flags = TP_CategorizeMessage (s, &offset);
			FChecks_CheckRequest(str);
			Auth_CheckResponse (str, flags, offset);
#else
			CL_CheckVersionRequest(str);	// Tonik
#endif
		}

		Print_flags[Print_current] |= PR_TR_SKIP;
		if (!CL_SearchForMsgTriggers (str, 1<<level) && (Print_flags[Print_current] & PR_IS_CHAT)) 
			S_LocalSound ("misc/talk.wav");

		Con_Printf ("%s", str);
	}

}

/*
==============
CL_ParseStufftext
==============
*/
void CL_ParseStufftext (void)
{
	char	*s;

	s = MSG_ReadString ();

	Con_DPrintf ("stufftext: %s\n", s);

	if (!strncmp (s, "exec ", 5)) {
		Cbuf_AddTextEx (&cbuf_main, s);
		return;
	}

	Cbuf_AddTextEx (&cbuf_svc, s);

	// QW servers send this without the ending \n
	if (  ((s[0]=='c' && s[1]=='m' && s[2]=='d') ||
		// QuakeForge servers up to Beta 6 send this without the \n
		!strncmp (s, "r_skyname ", 10) ) && !strchr (s, '\n') )
	{
		Cbuf_AddTextEx (&cbuf_svc, "\n");
	}
}

#define SHOWNET(x) if(cl_shownet.value==2)\
{Print_flags[Print_current] |= PR_TR_SKIP;\
Con_Printf ("%3i:%s\n", msg_readcount-1, x);}

/*
=====================
CL_ParseServerMessage
=====================
*/
int	received_framecount;

void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i, j;
	int			msg_svc_start;
	qboolean	demorecording = cls.demorecording;

	received_framecount = cls.framecount;
	cl.last_servermessage = cls.realtime;

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1) {
		Print_flags[Print_current] |= PR_TR_SKIP;
		Con_Printf ("%i ",net_message.cursize);
	} else if (cl_shownet.value == 2) {
		Print_flags[Print_current] |= PR_TR_SKIP;
		Con_Printf ("------------------\n");
	}

	CL_ParseClientdata ();
	CL_ClearProjectiles ();

	if (demorecording)
		CL_Write_SVCHeader ();

//
// parse the message
//
	while (1)
	{
		msg_svc_start = msg_readcount;

		if (msg_badread)
		{
			Host_EndGame ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			msg_readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			//Con_Printf("svc_unknown: %d\n",cmd);
			Host_EndGame ("CL_ParseServerMessage: Illegible server message");
			break;
			
		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;
			
		case svc_disconnect:
			if (cls.state == ca_connected)
				Host_EndGame ("Server disconnected\n"
					"Server version may not be compatible");
			else
				Host_EndGame ("Server disconnected");
			break;

		case svc_print:
			i = MSG_ReadByte ();
			s = MSG_ReadString();
			CL_ParsePrint (i, s);
			break;
			
		case svc_centerprint:
			s = MSG_ReadString();
			if(!CL_SearchForMsgTriggers (s, RE_PRINT_CENTER)) 
				SCR_CenterPrint (s);
			Print_flags[Print_current] = 0;
			break;
			
		case svc_stufftext:
			CL_ParseStufftext ();
			break;
			
		case svc_damage:
			V_ParseDamage ();
			break;
			
		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CL_ParseServerData ();
			vid.recalc_refdef = true;	// leave full screen intermission
			break;
			
		case svc_setangle:
			if (cls.mvdplayback) {
				extern int	fixangle;
				j = MSG_ReadByte();
				fixangle |= 1 << j;
				if (j != Cam_TrackNum())
					for (i=0; i<3; i++)
						MSG_ReadAngle();
			}

			if (!cls.mvdplayback || (cls.mvdplayback && j == Cam_TrackNum()))
			{
				for (i=0 ; i<3 ; i++)
					cl.viewangles[i] = MSG_ReadAngle ();
			}
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;
			
		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			cl_lightstyle[i].length = strlen(cl_lightstyle[i].map);
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.players[i].frags = MSG_ReadShort ();
			break;			

		case svc_updateping:
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateping > MAX_SCOREBOARD");
			cl.players[i].ping = MSG_ReadShort ();
			break;
			
		case svc_updatepl:
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatepl > MAX_SCOREBOARD");
			cl.players[i].pl = MSG_ReadByte ();
			break;
			
		case svc_updateentertime:
		// time is sent over as seconds ago
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateentertime > MAX_SCOREBOARD");
			cl.players[i].entertime = cls.realtime - MSG_ReadFloat ();
			break;
			
		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			CL_ParseBaseline (&cl_entities[i].baseline);
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			
		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			j = MSG_ReadByte ();
			CL_SetStat (i, j);
			break;
		case svc_updatestatlong:
			i = MSG_ReadByte ();
			j = MSG_ReadLong ();
			CL_SetStat (i, j);
			break;
			
		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			CDAudio_Play ((byte)cl.cdtrack, true);
			break;

		case svc_intermission:
			cl.intermission = 1;
			cl.completed_time = cls.realtime;
			vid.recalc_refdef = true;	// go to full screen
			for (i=0 ; i<3 ; i++)
				cl.simorg[i] = MSG_ReadCoord ();			
			for (i=0 ; i<3 ; i++)
				cl.simangles[i] = MSG_ReadAngle ();
			VectorCopy (vec3_origin, cl.simvel);
			CL_ExecTriggerSafe ("f_mapend");
			break;

		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cls.realtime;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (MSG_ReadString ());			
			break;
			
		case svc_sellscreen:
			Cmd_ExecuteString ("help");
			break;

		case svc_smallkick:
			cl.punchangle = -2;
			break;
		case svc_bigkick:
			cl.punchangle = -4;
			break;

		case svc_muzzleflash:
			CL_MuzzleFlash ();
			break;

		case svc_updateuserinfo:
			CL_UpdateUserinfo ();
			break;

		case svc_setinfo:
			CL_SetInfo ();
			break;

		case svc_serverinfo:
			CL_ParseServerInfoChange ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_playerinfo:
			CL_ParsePlayerinfo ();
			break;

		case svc_nails:
			CL_ParseProjectiles (false);
			break;
		case svc_nails2:
			CL_ParseProjectiles (true);
			break;

		case svc_chokecount:		// some preceding packets were choked
			i = MSG_ReadByte ();
			for (j=0 ; j<i ; j++)
				cl.frames[ (cls.netchan.incoming_acknowledged-1-j)&UPDATE_MASK ].receivedtime = -2;
			break;

		case svc_modellist:
			CL_ParseModellist ();
			break;

		case svc_soundlist:
			CL_ParseSoundlist ();
			break;

		case svc_packetentities:
			CL_ParsePacketEntities (false);
			if (cls.demoplayback && !cls.mvdplayback && (cls.state != ca_active))  // Sergio (demo delay)
				demo_loaddelay = 1; 
			break;

		case svc_deltapacketentities:
			CL_ParsePacketEntities (true);
			break;

		case svc_maxspeed :
			movevars.maxspeed = MSG_ReadFloat();
			break;

		case svc_entgravity :
			movevars.entgravity = MSG_ReadFloat();
			break;

		case svc_setpause:
			if (cls.demoplayback)
				cl.paused = (MSG_ReadByte () != 0) * 2;
			else
				cl.paused = (MSG_ReadByte () != 0);
			if (cl.paused)
				CDAudio_Pause ();
			else
				CDAudio_Resume ();
			break;

#ifdef QW262
		CL_ParseServerMessage262();
#endif
		}

		if (demorecording && cls.demorecording)
		{
			if (cmd != svc_deltapacketentities)
				CL_WriteSVC (net_message.data + msg_svc_start, msg_readcount - msg_svc_start);
			else
				CL_WriteDemoEntities ();
		}

	}

	if (demorecording && cls.demorecording)
		CL_Write_SVCEnd ();

	CL_SetSolidEntities ();
}


