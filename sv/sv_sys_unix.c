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
#include <dirent.h>

#ifdef NeXT
#include <libc.h>
#endif

#include <sys/types.h>
#include <netinet/in.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#else
#include <sys/dir.h>
#endif

#include <fcntl.h>
#include <paths.h>

#include <dlfcn.h>

#include "sv_io.h"


cvar_t	sys_nostdout = {"sys_nostdout","0"};
cvar_t	sys_sleep = {"sys_sleep","0"};

/*
===============================================================================

				REQUIRED SYS FUNCTIONS

===============================================================================
*/

/*
============
Sys_FileExists
============
*/
qboolean Sys_FileExists (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return false;
	
	return true;
}

int Sys_FileSize (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return 0;
	
	return buf.st_size;
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

/*
============
Sys_mkdir
============
*/
void Sys_mkdir (char *path)
{
	if (mkdir (path, 0777) != -1)
		return;
	if (errno != EEXIST)
		Sys_Error ("mkdir %s: %s",path, strerror(errno)); 
}

// mvdsv -->
/*
================
Sys_remove
================
*/
int Sys_remove (const char *path)
{
	return unlink(path);
}

//bliP: rmdir ->
/*
================
Sys_rmdir
================
*/
int Sys_rmdir (char *path)
{
	return rmdir(path);
}

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
	static file_t list[MAX_DIRFILES];
	dir_t	dir;
	int	i, extsize1, extsize2;
	char	pathname[MAX_OSPATH];
	DIR	*d;
	DIR	*testdir; //bliP: list dir
	struct dirent *oneentry;
	qboolean all;

	memset(list, 0, sizeof(list));
	memset(&dir, 0, sizeof(dir));
	dir.files = list;
	extsize1 = strlen(ext1);
	extsize2 = strlen(ext2);
	all = !strncmp(ext1, ".*", 3) || !strncmp(ext2, ".*", 3);

	if (!(d = opendir(path)))
		return dir;
	while ((oneentry = readdir(d)))
	{
		if (!strcmp(oneentry->d_name, ".") || !strcmp(oneentry->d_name, ".."))
			continue;
		if (( (i = strlen(oneentry->d_name)) < extsize1 ?
				1 : Q_stricmp(oneentry->d_name + i - extsize1, ext1)
			) &&
			( (i = strlen(oneentry->d_name)) < extsize2 ?
				1 : Q_stricmp(oneentry->d_name + i - extsize2, ext2)
			) && !all
		   )
				continue;

		snprintf(pathname, sizeof(pathname), "%s/%s", path, oneentry->d_name);
		if ((testdir = opendir(pathname)))
		{
			dir.numdirs++;
			list[dir.numfiles].isdir = true;
			list[dir.numfiles].size = 0;
			list[dir.numfiles].time = 0;
			closedir(testdir);
		}
		else
		{
			list[dir.numfiles].isdir = false;
			list[dir.numfiles].time = Sys_FileTime(pathname);
			dir.size += (list[dir.numfiles].size = Sys_FileSize(pathname));
		} 
		strlcpy (list[dir.numfiles].name, oneentry->d_name, MAX_DEMO_NAME);

		if (++dir.numfiles == MAX_DIRFILES - 1)
			break;
	}

	closedir(d);
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
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	printf ("Fatal error: %s\n",string);

	Log_Printf(SV_ERRORLOG, "Fatal error: %s\n", string);

	exit (1);
}

/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	static char		text[2048];
	unsigned char		*p;

	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

	if (sys_nostdout.value)
		return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
	fflush(stdout);
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	exit (0);		// appkit isn't running
}

/*
================
Sys_ConsoleInput

Checks for a complete line of text typed in at the console, then forwards
it to the host command processor
================
*/
char *Sys_ConsoleInput (void)
{
	static char	text[256];
	int		len;

	if (!(IO_status & IO_STDIN_AVAIL))
		return NULL;		// the select didn't say it was ready

	len = read (0, text, sizeof(text));
	if (len == 0)
		// end of file
		return NULL;
	if (len < 1)
		return NULL;
	text[len-1] = 0;	// rip off the /n and terminate
	
	return text;
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
	Cvar_RegisterVariable (&sys_nostdout);
	Cvar_RegisterVariable (&sys_sleep);
}

// Archi -->
/*
=============
Sys_WritePid

Writes pid to a file
=============
*/
void Sys_WritePid(const char *pidfile)
{
	FILE *f;

	Con_Printf("Writing pid to %s ... ", pidfile);

	if (!(f = fopen(pidfile, "w"))) {
		Con_Printf("failed.\n");
		return;
	}

	if (fprintf(f, "%d\n", getpid()) < 0) {
		Con_Printf("failed.\n");
		fclose(f);
		return;
	}

	if (fclose(f))
		Con_Printf("failed.\n");
	else
		Con_Printf("succeeded.\n");
}

/*
=============
Sys_Daemonize

Makes server to be a daemon
=============
*/
void Sys_Daemonize(void)
{
	int fd;

	Con_Printf("\nFiring up a daemon hookah ...\n");

	switch (fork()) {
	case -1:
		Con_Printf("Daemonize: fork() failed\n");
		return;
	case 0:
		break;
	default:
		exit(0);
	}

	if (setsid() == -1) {
		Con_Printf("Daemonize: setsid() failed\n");
		return;
	}

	if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);

		if (fd > 2)
			close(fd);
	}
}

int Sys_SpawnChild(const char *path, const char *workdir, char *const argv[])
{
	int rc = vfork();

	switch (rc)
	{
	case 0:
		break;
	default:
		return rc;
	}

	if (!chdir(workdir))
		execv(path, argv);

	_exit(1);
}
// <-- Archi

// mvdsv -->
int Sys_Script(const char *gamedir, const char *path, const char *args)
{
	char str[1024], *argv[4];
	
	Q_snprintfz(str, sizeof(str), "./%s.qws %s", path, args);
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = str;
	argv[3] = NULL;

	if (Sys_SpawnChild(argv[0], gamedir, argv) == -1)
		return 0;
	
	return 1;
}
// <-- mvdsv

DL_t Sys_DLOpen(const char *path)
{
	return dlopen(path,
#ifdef OS_OPENBSD
		DL_LAZY
#else
		RTLD_NOW
#endif
		);
}

qboolean Sys_DLClose(DL_t dl)
{
	return !dlclose(dl);
}

void *Sys_DLProc(DL_t dl, const char *name)
{
	return dlsym(dl, name);
}

/*
=============
main
=============
*/
int main(int argc, char *argv[])
{
	double		time, oldtime, newtime;
	quakeparms_t	parms;
	int		j;

	memset (&parms, 0, sizeof(parms));

	COM_InitArgv (argc, argv);	
	parms.argc = com_argc;
	parms.argv = com_argv;

	parms.memsize = 32*1024*1024;

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (Q_atof(com_argv[j+1]) * 1024 * 1024);
	if ((parms.membase = malloc (parms.memsize)) == NULL)
		Sys_Error("Can't allocate %ld\n", parms.memsize);

	parms.basedir = ".";

	parms.io_timeout = 1000;
	parms.io_doconsole = 1;

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame (0.1);

//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;
	while (1)
	{
	// do network IO

		IO_Query();

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);		
		
	// extrasleep is just a way to generate a fucked up connection on purpose
		if (sys_sleep.value)
			usleep (1000*sys_sleep.value);
	}	
}
