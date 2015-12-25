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
// quakedef.h -- primary header for server

#define	QUAKE_GAME			// as opposed to utilities

//define	PARANOID			// speed sapping error checking

#ifdef _WIN32
#ifndef __GNUC__
#pragma warning( disable : 4244 4127 4201 4214 4514 4305 4115 4018)
#endif
#endif

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#else
#include <winsock.h>
#endif

#include "bothdefs.h"

#include "common.h"
#include "bspfile.h"
#include "sv_sys.h"
#include "zone.h"
#include "mathlib.h"

#include "cvar.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "model.h"
#include "crc.h"
#include "progs.h"

#include "server.h"
#include "pmove.h"
#include "world.h"

// BorisU -->
#include "hash.h"
// <-- BorisU

#ifdef USE_PR2
// Angel -->
#include "pr2_vm.h"
#include "pr2.h"
// <-- Angel
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;	// for development over ISDN lines
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
	int		io_timeout;	/* in milliseconds */
	int		io_doconsole;
} quakeparms_t;


//=============================================================================

//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;

void SV_Error (char *error, ...);
void SV_Init (quakeparms_t *parms);

void Con_Printf (char *fmt, ...);
void Con_DPrintf (char *fmt, ...);

void Log_Printf (int log, char *fmt, ...); // Archi
