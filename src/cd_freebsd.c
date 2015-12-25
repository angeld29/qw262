/*
Copyright (C) 2003 Alexey Dokuchaev.
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
// cd_freebsd.c

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/cdio.h>

#include "quakedef.h"
#include "cdaudio.h"
#include "sound.h"

/* Obtained from /usr/include/linux/cdrom.h */
#define CDROM_DATA_TRACK	4

static qboolean cdValid = false;
static qboolean	playing = false;
static qboolean	wasPlaying = false;
static qboolean	initialized = false;
static qboolean	enabled = true;
static qboolean playLooping = false;
static float	cdvolume;
static byte 	remap[100];
static byte		playTrack;
static byte		maxTrack;

static int cdfile = -1;
static char cd_dev[64] = "/dev/cdrom";

static void CDAudio_Eject(void)
{
	if (cdfile == -1 || !enabled)
		return; // no cd init'd

	ioctl(cdfile, CDIOCALLOW);

	if ( ioctl(cdfile, CDIOCEJECT) == -1 ) 
		Con_DPrintf ("ioctl cdioceject failed\n");
}


static void CDAudio_CloseDoor(void)
{
	if (cdfile == -1 || !enabled)
		return; // no cd init'd

	ioctl(cdfile, CDIOCALLOW);

	if ( ioctl(cdfile, CDIOCCLOSE) == -1 ) 
		Con_DPrintf ("ioctl cdiocclose failed\n");
}

static int CDAudio_GetAudioDiskInfo(void)
{
	struct ioc_toc_header tochdr;

	cdValid = false;

	if ( ioctl(cdfile, CDIOREADTOCHEADER, &tochdr) == -1 ) 
    {
      Con_DPrintf ("ioctl cdioreadtocheader failed\n");
	  return -1;
    }

	if (tochdr.starting_track < 1)
	{
		Con_DPrintf ("CDAudio: no music tracks\n");
		return -1;
	}

	cdValid = true;
	maxTrack = tochdr.ending_track;

	return 0;
}


void CDAudio_Play(byte track, qboolean looping)
{
	struct ioc_read_toc_single_entry entry;
	struct ioc_play_track ti;

	if (cdfile == -1 || !enabled)
		return;
	
	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	track = remap[track];

	if (track < 1 || track > maxTrack)
	{
		Con_DPrintf ("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	entry.track = track;
	entry.address_format = CD_MSF_FORMAT;
    if ( ioctl(cdfile, CDIOREADTOCENTRY, &entry) == -1 )
	{
		Con_DPrintf ("ioctl cdioreadtocentry failed\n");
		return;
	}
	if (entry.entry.control == CDROM_DATA_TRACK)
	{
		Con_Printf ("CDAudio: track %i is not audio\n", track);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;
		CDAudio_Stop();
	}

	ti.start_track = track;
	ti.end_track = track;
	ti.start_index = 1;
	ti.start_index = 99;

	if ( ioctl(cdfile, CDIOCPLAYTRACKS, &ti) == -1 ) 
    {
		Con_DPrintf ("ioctl cdiocplaytracks failed\n");
		return;
    }

	if ( ioctl(cdfile, CDIOCRESUME) == -1 ) 
		Con_DPrintf ("ioctl cdiocresume failed\n");

	playLooping = looping;
	playTrack = track;
	playing = true;

	if (cdvolume == 0.0)
		CDAudio_Pause ();
}


void CDAudio_Stop(void)
{
	if (cdfile == -1 || !enabled)
		return;
	
	if (!playing)
		return;

	if ( ioctl(cdfile, CDIOCSTOP) == -1 )
		Con_DPrintf ("ioctl cdiocstop failed (%d)\n", errno);

	ioctl(cdfile, CDIOCALLOW);

	wasPlaying = false;
	playing = false;
}

void CDAudio_Pause(void)
{
	if (cdfile == -1 || !enabled)
		return;

	if (!playing)
		return;

	if ( ioctl(cdfile, CDIOCPAUSE) == -1 ) 
		Con_DPrintf ("ioctl cdiocpause failed\n");

	wasPlaying = playing;
	playing = false;
}


void CDAudio_Resume(void)
{
	if (cdfile == -1 || !enabled)
		return;
	
	if (!cdValid)
		return;

	if (!wasPlaying)
		return;
	
	if ( ioctl(cdfile, CDIOCRESUME) == -1 ) 
		Con_DPrintf ("ioctl cdiocresume failed\n");
	playing = true;
}

static void CD_f (void)
{
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_stricmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_stricmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_stricmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		for (n = 0; n < 100; n++)
			remap[n] = n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (Q_stricmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
				if (remap[n] != n)
					Con_Printf ("  %u -> %u\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			remap[n] = Q_atoi(Cmd_Argv (n+1));
		return;
	}

	if (Q_stricmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			Con_Printf ("No CD in player.\n");
			return;
		}
	}

	if (Q_stricmp(command, "play") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), false);
		return;
	}

	if (Q_stricmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), true);
		return;
	}

	if (Q_stricmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_stricmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_stricmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_stricmp(command, "eject") == 0)
	{
		if (playing)
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = false;
		return;
	}

	if (Q_stricmp(command, "info") == 0)
	{
		Con_Printf ("%u tracks\n", maxTrack);
		if (playing)
			Con_Printf ("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf ("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		Con_Printf ("Volume is %f\n", cdvolume);
		return;
	}
}

void CDAudio_Update(void)
{
	struct ioc_read_subchannel subchnl;
	struct cd_sub_channel_info data;
	static time_t lastchk;

	if (!enabled)
		return;

	if (bgmvolume.value != cdvolume)
	{
		if (cdvolume)
		{
			Cvar_SetValue (&bgmvolume, 0.0);
			cdvolume = bgmvolume.value;
			CDAudio_Pause ();
		}
		else
		{
			Cvar_SetValue (&bgmvolume, 1.0);
			cdvolume = bgmvolume.value;
			CDAudio_Resume ();
		}
	}

	if (playing && lastchk < time(NULL)) {
		lastchk = time(NULL) + 2; //two seconds between chks
		bzero(&subchnl, sizeof(subchnl));
		subchnl.data = &data;
		subchnl.data_len = sizeof(data);
		subchnl.address_format = CD_MSF_FORMAT;
		subchnl.data_format = CD_CURRENT_POSITION;
		if (ioctl(cdfile, CDIOCREADSUBCHANNEL, &subchnl) == -1 ) {
			Con_DPrintf ("ioctl cdiocreadsubchannel failed\n");
			playing = false;
			return;
		}
		if (subchnl.data->header.audio_status != CD_AS_PLAY_IN_PROGRESS &&
			subchnl.data->header.audio_status != CD_AS_PLAY_PAUSED) {
			playing = false;
			if (playLooping)
				CDAudio_Play(playTrack, true);
		}
	}
}

int CDAudio_Init(void)
{
	int i;

	if (!COM_CheckParm("-cdaudio"))
		return -1;

	if ((i = COM_CheckParm("-cddev")) != 0 && i < com_argc - 1)
		strlcpy (cd_dev, com_argv[i + 1], sizeof(cd_dev));

	if ((cdfile = open(cd_dev, O_RDONLY)) == -1) {
		Con_Printf ("CDAudio_Init: open of \"%s\" failed (%i)\n", cd_dev, errno);
		cdfile = -1;
		return -1;
	}

	for (i = 0; i < 100; i++)
		remap[i] = i;
	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf ("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	Con_Printf ("CD Audio Initialized\n");

	return 0;
}


void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;
	CDAudio_Stop();
	close(cdfile);
	cdfile = -1;
}
