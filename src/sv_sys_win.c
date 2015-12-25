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
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <limits.h>
#ifndef __CYGWIN__
#include <conio.h>
#include <direct.h>
#endif

#include "qwsvdef.h"

#include "sv_io.h"

#ifdef __CYGWIN__
#define _stat stat
int _kbhit (void);
int _getch (void);
void putch (int);
void cygwin_console_init (void);
#endif

cvar_t	sys_sleep = {"sys_sleep", "8"}; // Tonik
cvar_t	sys_nostdout = {"sys_nostdout","0"};

/*
================
Sys_FileExists
================
*/
qboolean Sys_FileExists (char *path)
{
#if 0
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f) {
		fclose(f);
		return true;
	}
	
	return false;
#else
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return false;
	
	return true;
#endif
}

int Sys_FileSize (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return 0;
	
	return buf.st_size;
}

/*
================
Sys_FileTime
================
*/
int Sys_FileTime (char *path)
{
	struct	_stat	buf;
	
	if (_stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

/*
================
Sys_mkdir
================
*/
void Sys_mkdir (char *path)
{
#ifndef __CYGWIN__
 	_mkdir(path);
#else
	mkdir(path, 0777);
#endif
}

// mvdsv -->
/*
================
Sys_remove
================
*/
int Sys_remove (const char *path)
{
	return remove(path);
}

//bliP: rmdir ->
/*
================
Sys_rmdir
================
*/
int Sys_rmdir (char *path)
{
#ifndef __CYGWIN__
 	return _rmdir(path);
#else
	return rmdir(path);
#endif
}
//<-

/*
================
Sys_listdir
================
*/

dir_t Sys_listdir (char *path, char *ext, int sort_type)
{
	return Sys_listdir2 (path, ext, ext, sort_type);
}
dir_t Sys_listdir2 (char *path, char *ext1, char *ext2, int sort_type)
{
	static file_t	list[MAX_DIRFILES];
	dir_t	dir;
	HANDLE	h;
	WIN32_FIND_DATA fd;
	int		i, extsize1, extsize2;
	char	pathname[MAX_DEMO_NAME];
	qboolean all;

	memset(list, 0, sizeof(list));
	memset(&dir, 0, sizeof(dir));

	extsize1 = strlen(ext1);
	extsize2 = strlen(ext2);
	dir.files = list;
	all = !strncmp(ext1, ".*", 3) || !strncmp(ext2, ".*", 3);

	if ((h = FindFirstFile (va("%s/*.*", path), &fd)) == INVALID_HANDLE_VALUE)
		return dir;

	do {
		if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
			continue;
		if (( (i = strlen(fd.cFileName)) < extsize1 ?
				1 : Q_stricmp(fd.cFileName + i - extsize1, ext1)
			) &&
			( (i = strlen(fd.cFileName)) < extsize2 ?
				1 : Q_stricmp(fd.cFileName + i - extsize2, ext2)
			) && !all
		   )
				continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //bliP: list dir
		{
			dir.numdirs++;
			list[dir.numfiles].isdir = true;
			list[dir.numfiles].size = 0;
			list[dir.numfiles].time = 0;
		}
		else
		{
			list[dir.numfiles].isdir = false;
			snprintf(pathname, sizeof(pathname), "%s/%s", path, fd.cFileName);
			list[dir.numfiles].time = Sys_FileTime(pathname);
			dir.size += (list[dir.numfiles].size = fd.nFileSizeLow);
		}
		strlcpy (list[dir.numfiles].name, fd.cFileName, sizeof(list[0].name));

		if (++dir.numfiles == MAX_DIRFILES - 1)
			break;

	} while (FindNextFile(h, &fd));

	FindClose (h);

	switch (sort_type)
	{
		case SORT_NO: break;
		case SORT_BY_DATE:
			qsort((void *)list, dir.numfiles, sizeof(file_t), Sys_compare_by_date);
			break;
		case SORT_BY_NAME:
			qsort((void *)list, dir.numfiles, sizeof(file_t), Sys_compare_by_name);
			break;
	}
	return dir;
}
// <-- mvdsv

/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,error);
	vsprintf (text, error,argptr);
	va_end (argptr);

//	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	printf ("ERROR: %s\n", text);

	Log_Printf(SV_ERRORLOG, "ERROR: %s\n", text);

	exit (1);
}

// Tonik -->
static double pfreq;
static qboolean hwtimer = false;
static __int64 startcount;

void Sys_InitDoubleTime (void)
{
	__int64 freq;

	if (!COM_CheckParm("-nohwtimer") &&
		QueryPerformanceFrequency ((LARGE_INTEGER *)&freq) && freq > 0)
	{
		// hardware timer available
		pfreq = (double)freq;
		hwtimer = true;
		QueryPerformanceCounter ((LARGE_INTEGER *)&startcount);
	}
	else
	{
		// make sure the timer is high precision, otherwise
		// NT gets 18ms resolution
		timeBeginPeriod (1);
	}
}

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	__int64 pcount;

	static DWORD starttime;
	static qboolean first = true;
	DWORD now;

	if (hwtimer)
	{
		QueryPerformanceCounter ((LARGE_INTEGER *)&pcount);
		// TODO: check for wrapping; is it necessary?
		return (pcount - startcount) / pfreq;
	}

	now = timeGetTime();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}
	
	if (now < starttime) // wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
}
// <-- Tonik

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	int		c;

	// read a line out
	while (_kbhit())
	{
		c = _getch();
		putch (c);
		if (c == '\r')
		{
			text[len] = 0;
			putch ('\n');
			len = 0;
			return text;
		}
		if (c == 8)
		{
			if (len)
			{
				putch (' ');
				putch (c);
				len--;
				text[len] = 0;
			}
			continue;
		}
		text[len] = c;
		len++;
		text[len] = 0;
		if (len == sizeof(text))
			len = 0;
	}

	return NULL;
}


/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	
	if (sys_nostdout.value)
		return;
		
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	exit (0);
}


/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	qboolean		WinNT;
	OSVERSIONINFO	vinfo;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
	{
		Sys_Error ("QuakeWorld requires at least Win95 or NT 4.0");
	}

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else
		WinNT = false;

	Cvar_RegisterVariable(&sys_sleep);
	Cvar_RegisterVariable(&sys_nostdout);

	if (COM_CheckParm ("-nopriority"))
	{
		Cvar_Set (&sys_sleep, "0");
	}
	else
	{
		if ( ! SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS))
			Con_Printf ("SetPriorityClass() failed\n");
		else
			Con_Printf ("Process priority class set to HIGH\n");

		// sys_sleep > 0 seems to cause packet loss on WinNT (why?)
		if (WinNT)
			Cvar_Set (&sys_sleep, "0");
	}

	Sys_InitDoubleTime ();
}

// mvdsv -->
int Sys_Script(const char *gamedir, const char *path, const char *args)
{
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;
	char cmdline[1024], curdir[MAX_OSPATH];

	memset (&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMINNOACTIVE;

	GetCurrentDirectory(sizeof(curdir), curdir);
	
	Q_snprintfz(cmdline, sizeof(cmdline), "%s\\sh.exe ./%s.qws %s", curdir, path, args);
	strlcat(curdir,va("\\%s", gamedir+2),sizeof(curdir));

	CreateProcess (NULL, cmdline, NULL, NULL,
		FALSE, 0/*DETACHED_PROCESS CREATE_NEW_CONSOLE*/ , NULL, curdir, &si, &pi);

	return 1;
}
// <-- mvdsv

DL_t Sys_DLOpen(const char *path)
{
	return LoadLibrary(path);
}

qboolean Sys_DLClose(DL_t dl)
{
	return FreeLibrary(dl);
}

void *Sys_DLProc(DL_t dl, const char *name)
{
	return GetProcAddress(dl, name);
}

/*
==================
main

==================
*/
char	*newargv[256];

int main (int argc, char **argv)
{
	quakeparms_t	parms;
	double		newtime, time, oldtime;
	static	char	cwd[1024];
	int		t;
	int		sleep_msec; // Tonik

	COM_InitArgv (argc, argv);
	
	parms.argc = com_argc;
	parms.argv = com_argv;

	parms.memsize = 32*1024*1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024 * 1024;

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error("Insufficient memory.\n");

	parms.basedir = ".";
	parms.cachedir = NULL;

	parms.io_timeout = 1000;
	parms.io_doconsole = 0;

#ifdef __CYGWIN__
	cygwin_console_init();
#endif

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame (0.1);
	
//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;
	while (1)
	{
// Tonik -->
		sleep_msec = sys_sleep.value;
		if (sleep_msec > 0)
		{
			if (sleep_msec > 13)
				sleep_msec = 13;
			Sleep (sleep_msec);
		}
// <-- Tonik

	// do network IO

		IO_Query();

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);				
	}	

	return true;
}
