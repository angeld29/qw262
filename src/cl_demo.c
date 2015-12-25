/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

#include <ctype.h>

void CL_FinishTimeDemo (void);

// Tonik -->
// .qwz playback
// Linux code by BorisU
static qboolean	qwz_playback = false;
static qboolean	qwz_unpacking = false;
#ifdef _WIN32
#include "winquake.h"
static HANDLE	hQizmoProcess = NULL;
#else
#include <unistd.h>
#include <sys/wait.h>
pid_t	pidQizmoProcess = 0;
#endif
static char tempqwd_name[256] = ""; // this file must be deleted
// after playback is finished
static void CheckQizmoCompletion ();
static void StopQWZPlayback ();
// <-- Tonik

int			demo_loaddelay;			// Sergio
static float playback_recordtime;	// fuh

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback (void)
{
	if (!cls.demoplayback)
		return;

	fclose (cls.demoplayfile);
	cls.demoplayfile = NULL;
	cls.state = ca_disconnected;
	cls.demoplayback = 0;
	cls.mvdplayback = 0;

// Tonik -->
	if (qwz_playback)
		StopQWZPlayback ();
// <-- Tonik
	
	if (cls.timedemo)
		CL_FinishTimeDemo ();
	
	server_version = 0;
	cl.teamfortress = false;
}

extern cvar_t	cl_demoflushtime; // BorisU

// fuh
#define DEMOTIME	((float) (cls.demoplayback ? playback_recordtime : cls.realtime))

/*
====================
CL_WriteDemoCmd

Writes the current user cmd
====================
*/
float demo_flushed = 0;
void CL_WriteDemoCmd (usercmd_t *pcmd)
{
	int		i;
	float	fl, t[3];
	byte	c;
	usercmd_t cmd;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, cls.realtime);

	fl = LittleFloat(DEMOTIME);
	fwrite (&fl, sizeof(fl), 1, cls.demorecordfile);

	c = dem_cmd;
	fwrite (&c, sizeof(c), 1, cls.demorecordfile);

	// correct for byte order, bytes don't matter
	cmd = *pcmd;

	for (i = 0; i < 3; i++)
		cmd.angles[i] = LittleFloat(cmd.angles[i]);
	cmd.forwardmove	= LittleShort(cmd.forwardmove);
	cmd.sidemove	= LittleShort(cmd.sidemove);
	cmd.upmove		= LittleShort(cmd.upmove);

	fwrite(&cmd, sizeof(cmd), 1, cls.demorecordfile);

	t[0] = LittleFloat (cl.viewangles[0]);
	t[1] = LittleFloat (cl.viewangles[1]);
	t[2] = LittleFloat (cl.viewangles[2]);

	fwrite (t, 12, 1, cls.demorecordfile);

	if ( cls.realtime - demo_flushed > cl_demoflushtime.value) {
		fflush (cls.demorecordfile);
		demo_flushed = cls.realtime;
	}
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
extern char LocalCmdBuf[];
void CL_WriteDemoMessage (sizebuf_t *msg)
{
	int		len;
	float	fl;
	byte	c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, cls.realtime);

	if (!cls.demorecording)
		return;

	fl = LittleFloat((DEMOTIME));
	fwrite (&fl, sizeof(fl), 1, cls.demorecordfile);

	c = dem_read;
	fwrite (&c, sizeof(c), 1, cls.demorecordfile);

	len = LittleLong (msg->cursize);
	fwrite (&len, 4, 1, cls.demorecordfile);
	fwrite (msg->data, msg->cursize, 1, cls.demorecordfile);

	if (!cl_demoflushtime.value)
		fflush (cls.demorecordfile);
}

static sizebuf_t demo_message;
static byte		demo_message_buffer[8192];

void CL_WriteSVC (byte *data, size_t length)
{
	SZ_Write (&demo_message, data, length);
}

void CL_Write_SVCHeader (void)
{
	demo_message.maxsize = sizeof(demo_message_buffer);
	demo_message.data = demo_message_buffer;

	SZ_Clear (&demo_message);

	CL_WriteSVC (net_message.data, 8);
}

void CL_Write_SVCEnd (void)
{
	if (*LocalCmdBuf)
	{
		MSG_WriteByte (&demo_message, svc_stufftext);
		MSG_WriteString (&demo_message, LocalCmdBuf);
		*LocalCmdBuf='\0';
	}

	CL_WriteDemoMessage (&demo_message);
}

void CL_WriteDemoDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg)
{
	int		bits;
	int		i;
	float	miss;

// write an update
	bits = 0;
	
	for (i=0 ; i<3 ; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if ( miss < -0.1 || miss > 0.1 )
			bits |= U_ORIGIN1<<i;
	}

	if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;
		
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
		
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;
		
	if ( to->colormap != from->colormap )
		bits |= U_COLORMAP;
		
	if ( to->skinnum != from->skinnum )
		bits |= U_SKIN;
		
	if ( to->frame != from->frame )
		bits |= U_FRAME;
	
	if ( to->effects != from->effects )
		bits |= U_EFFECTS;
	
	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;

	if (bits & 511)
		bits |= U_MOREBITS;

	if (to->flags & U_SOLID)
		bits |= U_SOLID;

	//
	// write the message
	//
	if (!to->number)
//		SV_Error ("Unset entity number");
		return;
	if (to->number >= 512)
//		SV_Error ("Entity number >= 512");
		return;

	i = to->number | (bits&~511);
	MSG_WriteShort (msg, i);
	
	if (bits & U_MOREBITS)
		MSG_WriteByte (msg, bits&255);
	if (bits & U_MODEL)
		MSG_WriteByte (msg,	to->modelindex);
	if (bits & U_FRAME)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		MSG_WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		MSG_WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		MSG_WriteByte (msg, to->effects);
	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);		
	if (bits & U_ANGLE1)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(msg, to->angles[2]);
}

void CL_WriteDemoEntities (void)
{
	int			packet;
	entity_state_t	*ent;
	int			ent_index, ent_total;

	MSG_WriteByte (&demo_message, svc_packetentities);

	packet = cls.netchan.incoming_sequence & UPDATE_MASK;
	ent = cl.frames[packet].packet_entities.entities;
	ent_total = cl.frames[packet].packet_entities.num_entities;

	for (ent_index = 0; ent_index < ent_total; ent_index++, ent++)
	{
		CL_WriteDemoDelta (&cl_entities[ent->number].baseline, ent, &demo_message);
	}

	MSG_WriteShort (&demo_message, 0);    // end of packetentities
}


static float	demotime; // made external by Sergio
static float	prevtime;
float			olddemotime, nextdemotime; // Highlander
/*
====================
CL_GetDemoMessage

  FIXME...
====================
*/
qboolean CL_GetDemoMessage (void)
{
	int		r, i, j;
	float	f;
	byte	c;
	usercmd_t *pcmd;
// Highlander -->
	int		size, tracknum;
	byte	newtime;
// <-- Highlander
	
// Tonik -->
	if (qwz_unpacking)
		return 0;
	
	if (cl.paused & 1)
		return 0;
// <-- Tonik

// Highlander -->
	if (prevtime < nextdemotime)
		prevtime = nextdemotime;

	if (!cls.mvdplayback)
		nextdemotime = (float) cls.realtime;

	if (cls.realtime + 1.0 < nextdemotime) {
		cls.realtime = nextdemotime - 1.0;
	}

nextdemomessage:

	size = 0;
	// read the time from the packet
	newtime = 0;
	if (cls.mvdplayback) {
		fread(&newtime, sizeof(newtime), 1, cls.demoplayfile);
		demotime =  prevtime + newtime*0.001;
	} else {
		fread(&demotime, sizeof(demotime), 1, cls.demoplayfile);
		demotime = LittleFloat(demotime);
		if (!nextdemotime)
			cls.realtime = nextdemotime = demotime;
	}

	if (cls.mvdplayback && cls.realtime - nextdemotime > 0.0001) {
		if (nextdemotime != demotime) {
			olddemotime = nextdemotime;
			cls.netchan.incoming_sequence++;
			cls.netchan.incoming_acknowledged++;
			cls.netchan.frame_latency = 0;
			cls.netchan.last_received = cls.realtime; // just to happy timeout check
			//Con_Printf("%d\n", cls.netchan.incoming_sequence);
		}
		nextdemotime = demotime;
	}

	playback_recordtime = demotime;

// decide if it is time to grab the next message		
	if (cls.timedemo) {
		if (cls.td_lastframe < 0)
			cls.td_lastframe = demotime;
		else if (demotime > cls.td_lastframe) {
			cls.td_lastframe = demotime;
			// rewind back to time
			if (cls.mvdplayback) {
				fseek(cls.demoplayfile, ftell(cls.demoplayfile) - sizeof(newtime),
					SEEK_SET);
			} else 
				fseek(cls.demoplayfile, ftell(cls.demoplayfile) - sizeof(demotime),
					SEEK_SET);
			return 0;		// already read this frame's message
		}
		if (!cls.td_starttime && cls.state == ca_active) {
			cls.td_starttime = Sys_DoubleTime();
			cls.td_startframe = cls.framecount;
		}
		cls.realtime = demotime; // warp
	} else if (!(cl.paused & 2) && cls.state >= ca_onserver) {	// allways grab until fully connected
		if (!cls.mvdplayback && cls.realtime + 1.0 < demotime) {
			// too far back
			cls.realtime = demotime - 1.0;
			// rewind back to time
			fseek(cls.demoplayfile, ftell(cls.demoplayfile) - sizeof(demotime),
					SEEK_SET);
			return 0;
		} else if (nextdemotime < demotime) {
			// rewind back to time
			if (cls.mvdplayback)
			{
				fseek(cls.demoplayfile, ftell(cls.demoplayfile) - sizeof(newtime),
					SEEK_SET);
			} else 
				fseek(cls.demoplayfile, ftell(cls.demoplayfile) - sizeof(demotime),
					SEEK_SET);
			return 0;		// don't need another message yet
		}
	} else
		cls.realtime = demotime; // we're warping

	prevtime = demotime;


	if (cls.state < ca_demostart)
		Host_Error ("CL_GetDemoMessage: cls.state != ca_active");
	
	// get the msg type
	if ((r = fread (&c, sizeof(c), 1, cls.demoplayfile)) != 1) {
		CL_StopPlayback ();
		return 0;
//		Host_Error ("Unexpected end of demo");
	}

	switch (c&7) {
	case dem_cmd :
		// user sent input
		i = cls.netchan.outgoing_sequence & UPDATE_MASK;
		pcmd = &cl.frames[i].cmd;
		r = fread (pcmd, sizeof(*pcmd), 1, cls.demoplayfile);
		if (r != 1)
		{
			CL_StopPlayback ();
			return 0;
		}
		// byte order stuff
		for (j = 0; j < 3; j++)
			pcmd->angles[j] = LittleFloat(pcmd->angles[j]);
		pcmd->forwardmove = LittleShort(pcmd->forwardmove);
		pcmd->sidemove    = LittleShort(pcmd->sidemove);
		pcmd->upmove      = LittleShort(pcmd->upmove);
		cl.frames[i].senttime = demotime;
		cl.frames[i].receivedtime = -1;		// we haven't gotten a reply yet
		cls.netchan.outgoing_sequence++;
		for (i=0 ; i<3 ; i++)
		{
			r = fread (&f, 4, 1, cls.demoplayfile);
			cl.viewangles[i] = LittleFloat (f);
		}
		if (cl.spectator)
			Cam_TryLock (); // Tonik

		if (cls.demorecording)
			CL_WriteDemoCmd(pcmd);

		goto nextdemomessage;
		
		break;

	case dem_read:
readit:
		// get the next message
		fread (&net_message.cursize, 4, 1, cls.demoplayfile);
		net_message.cursize = LittleLong (net_message.cursize);
		
		if (net_message.cursize > net_message.maxsize)
			Host_EndGame ("Demo message > MSG_BUF_SIZE");
		r = fread (net_message.data, net_message.cursize, 1, cls.demoplayfile);
		if (r != 1)
		{
			CL_StopPlayback ();
			return 0;
		}

		if (cls.mvdplayback)
		{
			tracknum = Cam_TrackNum();
		
			if (cls.lasttype == dem_multiple) {
				if (tracknum == -1)
					goto nextdemomessage;
	
				if (!(cls.lastto & (1 << (tracknum))))
					goto nextdemomessage;
			} else if (cls.lasttype == dem_single)
			{
				if (tracknum == -1 || cls.lastto != spec_track)
					goto nextdemomessage;
			}
		}
		break;

	case dem_set :
		fread (&i, 4, 1, cls.demoplayfile);
		cls.netchan.outgoing_sequence = LittleLong(i);
		fread (&i, 4, 1, cls.demoplayfile);
		cls.netchan.incoming_sequence = LittleLong(i);

		cls.demostarttime = demotime;

		if (cls.mvdplayback) {
			cls.netchan.incoming_acknowledged = cls.netchan.incoming_sequence;
			goto nextdemomessage;
		}
		break;

	case dem_multiple:
		r = fread (&i, 4, 1, cls.demoplayfile);
		if (r != 1)
		{
			CL_StopPlayback ();
			return 0;
		}

		cls.lastto = LittleLong(i);
		cls.lasttype = dem_multiple;
		goto readit;
		
	case dem_single:
		cls.lastto = c>>3;
		cls.lasttype = dem_single;
		goto readit;

	case dem_stats:
		cls.lastto = c>>3;
		cls.lasttype = dem_stats;
		goto readit;

	case dem_all:
		cls.lastto = 0;
		cls.lasttype = dem_all;
		goto readit;

	default :
		Con_Printf("Corrupted demo.\n");
		CL_StopPlayback ();
		return 0;
	}

	return 1;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
qboolean CL_GetMessage (void)
{
// Tonik -->
	CheckQizmoCompletion ();
// <-- Tonik
	
	if	(cls.demoplayback) {
// Sergio -->
		if (demo_loaddelay) {	
			if (cls.state == ca_active) {
				demo_loaddelay = 0;
				cls.realtime = demotime;
			}
			return false;
		} else 
// <-- Sergio
			return CL_GetDemoMessage ();
	}

	if (!NET_GetPacket ())
		return false;

//	CL_WriteDemoMessage (&net_message);
	
	return true;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteLong (&net_message, -1);	// -1 sequence means out of band
	MSG_WriteByte (&net_message, svc_disconnect);
	MSG_WriteString (&net_message, "EndOfDemo");
	CL_WriteDemoMessage (&net_message);

// finish up
	fclose (cls.demorecordfile);
	cls.demorecordfile = NULL;
	cls.demorecording = false;
	Con_Printf ("Completed demo\n");
}


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteRecordDemoMessage (sizebuf_t *msg, int seq)
{
	int		len;
	int		i;
	float	fl;
	byte	c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, cls.realtime);

	if (!cls.demorecording)
		return;

	fl = LittleFloat((float)cls.realtime);
	fwrite (&fl, sizeof(fl), 1, cls.demorecordfile);

	c = dem_read;
	fwrite (&c, sizeof(c), 1, cls.demorecordfile);

	len = LittleLong (msg->cursize + 8);
	fwrite (&len, 4, 1, cls.demorecordfile);

	i = LittleLong(seq);
	fwrite (&i, 4, 1, cls.demorecordfile);
	fwrite (&i, 4, 1, cls.demorecordfile);

	fwrite (msg->data, msg->cursize, 1, cls.demorecordfile);

	if (!cl_demoflushtime.value)
		fflush (cls.demorecordfile);
}


void CL_WriteSetDemoMessage (void)
{
	int		len;
	float	fl;
	byte	c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, cls.realtime);

	if (!cls.demorecording)
		return;

	fl = LittleFloat((float)cls.realtime);
	fwrite (&fl, sizeof(fl), 1, cls.demorecordfile);

	c = dem_set;
	fwrite (&c, sizeof(c), 1, cls.demorecordfile);

	len = LittleLong(cls.netchan.outgoing_sequence);
	fwrite (&len, 4, 1, cls.demorecordfile);
	len = LittleLong(cls.netchan.incoming_sequence);
	fwrite (&len, 4, 1, cls.demorecordfile);

	if (!cl_demoflushtime.value)
		fflush (cls.demorecordfile);
}

/*
====================
CL_Record_f

record <demoname> <server>
====================
*/
void CL_Record_f (void)
{
	char	name[MAX_OSPATH];
	sizebuf_t	buf;
	char	buf_data[MAX_MSGLEN];
	int n, i, j;
	char *s;
	entity_t *ent;
	entity_state_t *es, blankes;
	player_info_t *player;
	extern	char gamedirfile[];
	int seq = 1;

	if (Cmd_Argc() < 2) {
		Con_Printf ("record <demoname>\n");
		return;
	}

	if (cls.mvdplayback) {
		Con_Printf ("Can't record while playing MVD demo.\n");
		return;
	}
	if (cls.state != ca_active) {
		Con_Printf ("You must be connected to record.\n");
		return;
	}

	if (cls.demorecording)
		CL_Stop_f();

// BorisU -->
// cleaning name
	for (s=Cmd_Args(); *s ; s++)	{
		char c;
		*s &= 0x7F;		// strip high bit
		c = *s;
		if (c==16) *s = '[';
		else if (c==17) *s = ']';
		else if (c>=18 && c<=27) *s=c+30;
		else if (c<=' ' || c=='?' || c=='*' || c=='\\' || c=='/' || c==':'
			|| c=='<' || c=='>' || c=='"' || c=='|')
			*s = '_';
	}
// <-- BorisU

	Q_snprintfz (name, sizeof(name), "%s/demos/", com_gamedir);
	COM_CreatePath (name);
	Q_snprintfz (name, sizeof(name), "%s/demos/%s", com_gamedir, Cmd_Args());
	
//
// open the demo file
//
	COM_DefaultExtension (name, ".qwd");

	cls.demorecordfile = fopen (name, "wb");
	if (!cls.demorecordfile) {
		Con_Printf ("ERROR: couldn't open %s.\n", name);
		return;
	}

	Con_Printf ("recording to %s.qwd\n", Cmd_Args());
	cls.demorecording = true;

/*-------------------------------------------------*/

// serverdata
	// send the info about the new client to all connected clients
	memset(&buf, 0, sizeof(buf));
	buf.data = buf_data;
	buf.maxsize = sizeof(buf_data);

// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, IsMDserver ? PROTOCOL_VERSION:OLD_PROTOCOL_VERSION);
	MSG_WriteLong (&buf, cl.servercount);
	MSG_WriteString (&buf, gamedirfile);

	if (cl.spectator)
		MSG_WriteByte (&buf, cl.playernum | 128);
	else
		MSG_WriteByte (&buf, cl.playernum);

	// send full levelname
	MSG_WriteString (&buf, cl.levelname);

	// send the movevars
	MSG_WriteFloat(&buf, movevars.gravity);
	MSG_WriteFloat(&buf, movevars.stopspeed);
	MSG_WriteFloat(&buf, movevars.maxspeed);
	MSG_WriteFloat(&buf, movevars.spectatormaxspeed);
	MSG_WriteFloat(&buf, movevars.accelerate);
	MSG_WriteFloat(&buf, movevars.airaccelerate);
	MSG_WriteFloat(&buf, movevars.wateraccelerate);
	MSG_WriteFloat(&buf, movevars.friction);
	MSG_WriteFloat(&buf, movevars.waterfriction);
	MSG_WriteFloat(&buf, movevars.entgravity);

	// send music
	MSG_WriteByte (&buf, svc_cdtrack);
	MSG_WriteByte (&buf, 0); // none in demos

	// send server info string
	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, va("fullserverinfo \"%s\"\n", cl.serverinfo) );

	// flush packet
	CL_WriteRecordDemoMessage (&buf, seq++);
	SZ_Clear (&buf); 

// soundlist
	MSG_WriteByte (&buf, svc_soundlist);
	MSG_WriteByte (&buf, 0);

	n = 0;
	s = cl.sound_name[n+1];
	while (*s) {
		MSG_WriteString (&buf, s);
		if (buf.cursize > MAX_MSGLEN/2) {
			MSG_WriteByte (&buf, 0);
			MSG_WriteByte (&buf, n);
			CL_WriteRecordDemoMessage (&buf, seq++);
			SZ_Clear (&buf); 
			MSG_WriteByte (&buf, svc_soundlist);
			MSG_WriteByte (&buf, n + 1);
		}
		n++;
		s = cl.sound_name[n+1];
	}
	if (buf.cursize) {
		MSG_WriteByte (&buf, 0);
		MSG_WriteByte (&buf, 0);
		CL_WriteRecordDemoMessage (&buf, seq++);
		SZ_Clear (&buf); 
	}

// modellist
	MSG_WriteByte (&buf, svc_modellist);
	MSG_WriteByte (&buf, 0);

	n = 0;
	s = cl.model_name[n+1];
	while (*s) {
		MSG_WriteString (&buf, s);
		if (buf.cursize > MAX_MSGLEN/2) {
			MSG_WriteByte (&buf, 0);
			MSG_WriteByte (&buf, n);
			CL_WriteRecordDemoMessage (&buf, seq++);
			SZ_Clear (&buf); 
			MSG_WriteByte (&buf, svc_modellist);
			MSG_WriteByte (&buf, n + 1);
		}
		n++;
		s = cl.model_name[n+1];
	}
	if (buf.cursize) {
		MSG_WriteByte (&buf, 0);
		MSG_WriteByte (&buf, 0);
		CL_WriteRecordDemoMessage (&buf, seq++);
		SZ_Clear (&buf); 
	}

// spawnstatic

	for (i = 0; i < cl.num_statics; i++) {
		ent = cl_static_entities + i;

		MSG_WriteByte (&buf, svc_spawnstatic);

		for (j = 1; j < MAX_MODELS; j++)
			if (ent->model == cl.model_precache[j])
				break;
		if (j == MAX_MODELS)
			MSG_WriteByte (&buf, 0);
		else
			MSG_WriteByte (&buf, j);

		MSG_WriteByte (&buf, ent->frame);
		MSG_WriteByte (&buf, 0);
		MSG_WriteByte (&buf, ent->skinnum);
		for (j=0 ; j<3 ; j++)
		{
			MSG_WriteCoord (&buf, ent->origin[j]);
			MSG_WriteAngle (&buf, ent->angles[j]);
		}

		if (buf.cursize > MAX_MSGLEN/2) {
			CL_WriteRecordDemoMessage (&buf, seq++);
			SZ_Clear (&buf); 
		}
	}

// spawnstaticsound
	// static sounds are skipped in demos, life is hard

// baselines

	memset(&blankes, 0, sizeof(blankes));
	for (i = 0; i < MAX_EDICTS; i++) {
		es = &cl_entities[i].baseline;

		if (memcmp(es, &blankes, sizeof(blankes))) {
			MSG_WriteByte (&buf,svc_spawnbaseline);		
			MSG_WriteShort (&buf, i);

			MSG_WriteByte (&buf, es->modelindex);
			MSG_WriteByte (&buf, es->frame);
			MSG_WriteByte (&buf, es->colormap);
			MSG_WriteByte (&buf, es->skinnum);
			for (j=0 ; j<3 ; j++)
			{
				MSG_WriteCoord(&buf, es->origin[j]);
				MSG_WriteAngle(&buf, es->angles[j]);
			}

			if (buf.cursize > MAX_MSGLEN/2) {
				CL_WriteRecordDemoMessage (&buf, seq++);
				SZ_Clear (&buf); 
			}
		}
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, va("cmd spawn %i 0\n", cl.servercount) );

	if (buf.cursize) {
		CL_WriteRecordDemoMessage (&buf, seq++);
		SZ_Clear (&buf); 
	}

// send current status of all other players

	for (i = 0; i < MAX_CLIENTS; i++) {
		player = cl.players + i;

		MSG_WriteByte (&buf, svc_updatefrags);
		MSG_WriteByte (&buf, i);
		MSG_WriteShort (&buf, player->frags);
		
		MSG_WriteByte (&buf, svc_updateping);
		MSG_WriteByte (&buf, i);
		MSG_WriteShort (&buf, player->ping);
		
		MSG_WriteByte (&buf, svc_updatepl);
		MSG_WriteByte (&buf, i);
		MSG_WriteByte (&buf, player->pl);
		
		MSG_WriteByte (&buf, svc_updateentertime);
		MSG_WriteByte (&buf, i);
		MSG_WriteFloat (&buf, player->entertime);

		MSG_WriteByte (&buf, svc_updateuserinfo);
		MSG_WriteByte (&buf, i);
		MSG_WriteLong (&buf, player->userid);
		MSG_WriteString (&buf, player->userinfo);

		if (buf.cursize > MAX_MSGLEN/2) {
			CL_WriteRecordDemoMessage (&buf, seq++);
			SZ_Clear (&buf); 
		}
	}
	
// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		MSG_WriteByte (&buf, svc_lightstyle);
		MSG_WriteByte (&buf, (char)i);
		MSG_WriteString (&buf, cl_lightstyle[i].map);
	}

	for (i = 0; i < MAX_CL_STATS; i++) {
		MSG_WriteByte (&buf, svc_updatestatlong);
		MSG_WriteByte (&buf, i);
		MSG_WriteLong (&buf, cl.stats[i]);
		if (buf.cursize > MAX_MSGLEN/2) {
			CL_WriteRecordDemoMessage (&buf, seq++);
			SZ_Clear (&buf); 
		}
	}

#if 0
	MSG_WriteByte (&buf, svc_updatestatlong);
	MSG_WriteByte (&buf, STAT_TOTALMONSTERS);
	MSG_WriteLong (&buf, cl.stats[STAT_TOTALMONSTERS]);

	MSG_WriteByte (&buf, svc_updatestatlong);
	MSG_WriteByte (&buf, STAT_SECRETS);
	MSG_WriteLong (&buf, cl.stats[STAT_SECRETS]);

	MSG_WriteByte (&buf, svc_updatestatlong);
	MSG_WriteByte (&buf, STAT_MONSTERS);
	MSG_WriteLong (&buf, cl.stats[STAT_MONSTERS]);
#endif

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, va("skins\n") );

	CL_WriteRecordDemoMessage (&buf, seq++);

	CL_WriteSetDemoMessage();

	// done
}

/*
====================
CL_ReRecord_f

record <demoname>
====================
*/
void CL_ReRecord_f (void)
{
	int		c;
	char	name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf ("rerecord <demoname>\n");
		return;
	}

	if (!*cls.servername) {
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	if (cls.demorecording)
		CL_Stop_f();

	Q_snprintfz (name, sizeof(name), "%s/%s", com_gamedir, Cmd_Argv(1));

//
// open the demo file
//
	COM_DefaultExtension (name, ".qwd");

	cls.demorecordfile = fopen (name, "wb");
	if (!cls.demorecordfile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	Con_Printf ("recording to %s.\n", name);
	cls.demorecording = true;

	CL_Disconnect();
	CL_BeginServerConnect();
}

// Tonik -->
//==================================================================
// .QWZ demos playback (via Qizmo)
//
static void CheckQizmoCompletion ()
{
#ifdef _WIN32
	DWORD ExitCode;

	if (!hQizmoProcess)
		return;

	if (!GetExitCodeProcess (hQizmoProcess, &ExitCode)) {
		Con_Printf ("WARINING: GetExitCodeProcess failed\n");
		hQizmoProcess = NULL;
		qwz_unpacking = false;
		qwz_playback = false;
		cls.demoplayback = false;
		StopQWZPlayback ();
		return;
	}
	
	if (ExitCode == STILL_ACTIVE)
		return;

	hQizmoProcess = NULL;
#else
	int 		wait_ret;
	int 		status;
	qboolean	failed = false;

	if (!pidQizmoProcess)
		return;
	
	wait_ret = waitpid (pidQizmoProcess, &status, WNOHANG);

	if (wait_ret == 0) // still active
		return;
	
	if (wait_ret == -1) {
		Con_Printf ("WARINING: waitpid failed\n");
		failed = true;
	}

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
		Con_Printf("unpacking error, may be wrong qizmo_dir?\n");
		failed = true;
		}
	} else {
		return; // still running
	}

	pidQizmoProcess = 0;
	if (failed) {
		qwz_unpacking = false;
		qwz_playback = false;
		cls.demoplayback = false;
		cls.state = ca_disconnected;
		StopQWZPlayback ();
		return;
	}
	
#endif

	Con_Printf("unpacking finished\n");
	if (!qwz_unpacking || !cls.demoplayback) {
		StopQWZPlayback ();
		return;
	}

	qwz_unpacking = false;
	
	cls.demoplayfile = fopen (tempqwd_name, "rb");
	if (!cls.demoplayfile) {
		Con_Printf ("Couldn't open %s\n", tempqwd_name);
		qwz_playback = false;
		cls.demoplayback = false;
		return;
	}

	// start playback
	cls.demoplayback = true;
	cls.state = ca_demostart;
	Netchan_Setup (&cls.netchan, net_from, 0);
	cls.realtime = 0;
}

static void StopQWZPlayback ()
{
#ifdef _WIN32
	if (!hQizmoProcess 
#else
	if (!pidQizmoProcess
#endif
		&& tempqwd_name[0]) {
		if (remove (tempqwd_name) != 0)
			Con_Printf ("Couldn't delete %s\n", tempqwd_name);
		tempqwd_name[0] = '\0';
	}
	qwz_playback = false;
}


void PlayQWZDemo (void)
{
	extern cvar_t	qizmo_dir;
#ifdef _WIN32
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;
#else
	//local vars
#endif
	char	*name;
	char	qwz_name[256];
	char	cmdline[512];
	char	*p;

#ifdef _WIN32
	if (hQizmoProcess) {
#else
	if (pidQizmoProcess) {
#endif
		Con_Printf ("Cannot unpack -- Qizmo still running!\n");
		return;
	}
	
	name = Cmd_Argv(1);

	if (!strncmp(name, "../", 3) || !strncmp(name, "..\\", 3))
		strncpy (qwz_name, va("%s/%s", com_basedir, name+3), sizeof(qwz_name));
	else
		if (name[0] == '/' || name[0] == '\\')
			strncpy (qwz_name, va("%s/%s", com_gamedir, name+1), sizeof(qwz_name));
		else
			strncpy (qwz_name, va("%s/%s", com_gamedir, name), sizeof(qwz_name));

	// check if the file exists
	cls.demoplayfile = fopen (qwz_name, "rb");
	if (!cls.demoplayfile)
	{
		Con_Printf ("Couldn't open %s\n", name);
		return;
	}
	fclose (cls.demoplayfile);
	
	strncpy (tempqwd_name, qwz_name, sizeof(tempqwd_name)-4);
#if 0
	// the right way
	strcpy (tempqwd_name + strlen(tempqwd_name) - 4, ".qwd");
#else
	// the way Qizmo does it, sigh
	p = strstr (tempqwd_name, ".qwz");
	if (!p)
		p = strstr (tempqwd_name, ".QWZ");
	if (!p)
		p = tempqwd_name + strlen(tempqwd_name);
	strcpy (p, ".qwd");
#endif

	cls.demoplayfile = fopen (tempqwd_name, "rb");
	if (cls.demoplayfile) {
		// .qwd already exists, so just play it
		cls.demoplayback = true;
		cls.state = ca_demostart;
		Netchan_Setup (&cls.netchan, net_from, 0);
		cls.realtime = 0;
		return;
	}
	
	Con_Printf ("unpacking %s...\n", name);
	
	// start Qizmo to unpack the demo

#ifdef _WIN32
	memset (&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	
	strncpy (cmdline, va("%s/%s/qizmo.exe -q -u -D \"%s\"", com_basedir,
		qizmo_dir.string, qwz_name), sizeof(cmdline));
	
	if (!CreateProcess (NULL, cmdline, NULL, NULL,
		FALSE, 0/* | HIGH_PRIORITY_CLASS*/,
		NULL, va("%s/%s", com_basedir, qizmo_dir.string), &si, &pi))
	{
		Con_Printf ("Couldn't execute %s/%s/qizmo.exe\n",
			com_basedir, qizmo_dir.string);
		return;
	}
	
	hQizmoProcess = pi.hProcess;
#else
	pidQizmoProcess = fork();
	if (pidQizmoProcess == 0) { // child process
		chdir (va("%s/%s", com_basedir, qizmo_dir.string));
		strncpy (cmdline, va("%s/%s/qizmo", com_basedir, qizmo_dir.string), sizeof(cmdline)); 
		execl (cmdline, cmdline, "-q", "-u", "-D", qwz_name, NULL);
		// exec failed
		_exit(EXIT_FAILURE);
	} else if (pidQizmoProcess < 0) { // fork failed
		Con_Printf("fork failed\n"); 
		pidQizmoProcess = 0;
		return;
	}
#endif

	qwz_unpacking = true;
	qwz_playback = true;

	// demo playback doesn't actually start yet, we just set
	// cls.demoplayback so that CL_StopPlayback() will be called
	// if CL_Disconnect() is issued
	cls.demoplayback = true;
	cls.demoplayfile = NULL;
	cls.state = ca_demostart;
	Netchan_Setup (&cls.netchan, net_from, 0);
	cls.realtime = 0;
}
// <-- Tonik

/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f (void)
{
	char	name[256], **s;
	static char *ext[] = {".qwd", ".mvd", NULL};

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("playdemo <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect ();

	
// BorisU -->
#ifdef GLQUAKE
// everything is allowed during demo playback
	cls.allow24bit = (gl_use_24bit_textures.value ? ALLOW_24BIT_TEXTURES : 0)
						| ALLOW_24BIT_FB_TEXTURES | ALLOW_24BIT_SKYBOX;
#endif
// <-- BorisU

//
// open the demo file
//
	strcpy (name, Cmd_Argv(1));

	if (strlen(name) > 4 && !Q_stricmp(name + strlen(name) - 4, ".qwz")) {
		PlayQWZDemo ();
		return;
	}

	for (s = ext; *s && !cls.demoplayfile; s++) {
		strcpy (name, Cmd_Argv(1));
		COM_DefaultExtension (name, *s);

		if (!strncmp(name, "../", 3) || !strncmp(name, "..\\", 3)) // Highlander
			cls.demoplayfile = fopen (va("%s/%s", com_basedir, name+3), "rb");
		else
			COM_FOpenFile (name, &cls.demoplayfile);
	}

	if (!cls.demoplayfile)
	{
		Con_Printf ("ERROR: couldn't open %s\n", Cmd_Argv(1));
		cls.demonum = -1;		// stop demo loop
		return;
	}

	Con_Printf ("Playing demo from %s.\n", name);

	cls.demoplayback = true;
	if (!Q_stricmp(name + strlen(name)-3, "mvd")) // Highlander
		cls.mvdplayback = true;

	cls.state = ca_demostart;
	Netchan_Setup (&cls.netchan, net_from, 0);
	cls.realtime = demotime = nextdemotime = prevtime = 0;
	demo_loaddelay = 0; // Sergio
// Highlander -->
	olddemotime = 0;
	cls.findtrack = true;
	cls.lasttype = 0;
	cls.lastto = 0;
	CL_ClearPredict();
// <-- Highlander
	key_dest = key_game; // automatically turn off console when start playing
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo (void)
{
	int		frames;
	float	time;
	
	cls.timedemo = false;
	
// the first frame didn't count
	frames = (cls.framecount - cls.td_startframe) - 1;
	time = Sys_DoubleTime() - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	
	if (cls.state != ca_demostart)
		return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted
	
	cls.timedemo = true;
	cls.td_starttime = 0;
	cls.td_startframe = cls.framecount;
	cls.td_lastframe = -1;		// get a new message this frame
}

// fuh -->
void Demo_SetSpeed_f (void) 
{
	extern cvar_t cl_demotimescale;
	char	*p;
	double	speed;

	if (Cmd_Argc() != 2) {
		Con_Printf("Usage: %s [+|-][speed %%]\n", Cmd_Argv(0));
		return;
	}

	p = Cmd_Argv(1);
	speed = atof(p);
	if (*p == '+' || *p == '-')
		Cvar_SetValue(&cl_demotimescale, cl_demotimescale.value + speed / 100.0);
	else
		Cvar_SetValue(&cl_demotimescale, speed / 100.0);
}

#define MAX_DEMO_JUMP 1000

void Demo_Jump_f (void) {
	qboolean	relative = false;
	int			seconds = 0;
	int			col_count;
	char		*text, *s;

	if (!cls.demoplayback) {
		Con_Printf("error : not playing a demo\n");
		return;
	}

	if (Cmd_Argc() != 2) {
		Con_Printf("Usage: %s [+][m:]<s> (seconds)\n", Cmd_Argv(0));
		return;
	}

	if ((cl.paused & 1)) { 
		Con_Printf("error : cannot jump whilst paused\n");
		return;
	}

	text = Cmd_Argv(1);
	for (col_count = 0, s = text; *s; s++) {
		if ((*s == '+' && s != text) || (*s != ':' && *s != '+' && !isdigit(*s))) {
			Con_Printf("Usage: %s [+][m:]<s> (seconds)\n", Cmd_Argv(0));
			return;
		}
		if (*s == ':') {
			if (++col_count >= 2) {
				Con_Printf("Usage: %s [+][m:]<s> (seconds)\n", Cmd_Argv(0));
				return;
			}
		}
	}

	if (text[0] == '+') {
		relative = true;
		text++;
	}
	if (strchr(text, ':')) {
		seconds += 60 * atoi(text);
		text = strchr(text, ':') + 1;
	}
	seconds += atoi(text);


	if (!relative) {
		seconds -= cls.realtime - cls.demostarttime;
		if (seconds < 0) {
			Con_Printf("error : cannot jump backwards\n");
			return;
		}
	}

	Cbuf_FullExecute (); // BorisU: empty exec buffers

	if (seconds > MAX_DEMO_JUMP) {
		char	buf[32];
		if (seconds > MAX_DEMO_JUMP) {
			sprintf(buf, "wait;demo_jump +%i\n", seconds - MAX_DEMO_JUMP);
			Cbuf_InsertTextEx(&cbuf_svc, buf);
			seconds = MAX_DEMO_JUMP;
		}
	}

	cls.realtime += seconds;
}

void Demo_Init (void) {
	Cmd_AddCommand("demo_setspeed", Demo_SetSpeed_f);
	Cmd_AddCommand("demo_jump", Demo_Jump_f);
}
// <-- fuh
