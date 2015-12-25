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
// console.c

#include "quakedef.h"
#include <fcntl.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h>
#else
#include <io.h>
#endif

console_t	con_main;
console_t	con_chat;
console_t	*con;			// point to either con_main or con_chat

int 		con_linewidth;	// characters across screen
int			con_totallines;		// total lines in console scrollback

float		con_cursorspeed = 4;

cvar_t		con_notifytime = {"con_notifytime","3"};		//seconds

// BorisU -->
cvar_t		log_filename = {"log_filename",""};
cvar_t		log_prefix = {"log_prefix",""};
cvar_t		con_notify_lines = {"con_notifylines", "4"};
// <-- BorisU
extern	cvar_t	cl_parseWhiteText;

#define	NUM_CON_TIMES 8
float		con_times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines

int			con_vislines;
int			con_notifylines;		// scan lines to clear for notify lines

qboolean	con_debuglog;

#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;
		

qboolean	con_initialized;

// Added by Sender
extern  int		key_linelength;
extern  int		chat_bufferpos;

void Key_ClearTyping (void)
{
	key_lines[edit_line][1] = 0;    // clear any typing
	key_linepos = 1;
	key_linelength = 1;
}

// BorisU -->
void Log_Truncate_f (void)
{
	int fd;
	if (!*log_filename.string) return;
	fd = open(va("%s/%s",com_gamedir, log_filename.string), O_WRONLY | O_TRUNC , 0666);
	close(fd);
}
// <-- BorisU

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	Key_ClearTyping ();

	if (key_dest == key_console)
	{
		if (cls.state == ca_active)
			key_dest = key_game;
	}
	else
		key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (key_dest == key_console)
	{
		if (cls.state == ca_active)
			key_dest = key_game;
	}
	else
		key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	memset (con_main.text, ' ', CON_TEXTSIZE);
	memset (con_chat.text, ' ', CON_TEXTSIZE);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con_times[i] = 0;
}

						
/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = 0;
	key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = 1;
	key_dest = key_message;
}

/*
================
Con_MessageMode3_f
================
*/
extern cvar_t uid_cvar;
void Con_MessageMode3_f (void)
{
	if(!*uid_cvar.string) { Con_Printf(" You must initialized uid value first."); return; }
	chat_team = 2;
	key_dest = key_message;
}


input_t input;
/*
================
Con_Input_f
================
*/
void Con_Input_f (void)
{
	char	*var_name;

	if (Cmd_Argc() < 5) {
		Con_Printf ("usage: input <cvar> <x> <y> <len> [<bg>]\n");
		return;
	}

	var_name = Cmd_Argv (1);
	input.var = Cvar_FindVar (var_name);
	if (!input.var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	}

	input.x = atoi(Cmd_Argv (2));
	input.y = atoi(Cmd_Argv (3));
	input.len = atoi(Cmd_Argv (4));
	input.bg = atoi(Cmd_Argv (5));

	chat_team = 100;
	key_dest = key_message;
}

/*
================
Con_Resize

================
*/
void Con_Resize (console_t *con)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = (vid.conwidth >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
#ifdef GLQUAKE
		if ((i = COM_CheckParm("-conwidth")) != 0)
			width = (Q_atoi(com_argv[i+1])>>3) - 2;
#endif
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		memset (con->text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;
	
		if (con_linewidth < numchars)
			numchars = con_linewidth;

		memcpy (tbuf, con->text, CON_TEXTSIZE);
		memset (con->text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con->text[(con_totallines - 1 - i) * con_linewidth + j] =
						tbuf[((con->current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con->current = con_totallines - 1;
	con->display = con->current;
}

					
/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	Con_Resize (&con_main);
	Con_Resize (&con_chat);
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con_debuglog = COM_CheckParm("-condebug");

	con = &con_main;
	con_linewidth = -1;
	Con_CheckResize ();
	
	Con_Printf ("Console initialized.\n");

//
// register our commands
//
	Cvar_RegisterVariable (&con_notifytime);
	Cvar_RegisterVariable (&con_notify_lines);
	Cvar_RegisterVariable (&log_filename);
	Cvar_RegisterVariable (&log_prefix);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("input", Con_Input_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("log_truncate", Log_Truncate_f);

	con_initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con->x = 0;
	if (con->display == con->current)
		con->display++;
	con->current++;
	memset (&con->text[(con->current%con_totallines)*con_linewidth]
	, ' ', con_linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
static void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask,andmask=255;
	char	*parse_white = NULL;
	qboolean team_chat = false;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;

	if (Print_flags[Print_current] & PR_IS_CHAT)
	{
		mask |= 128;
		if (*txt == '(') team_chat = true;
	}

	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con_linewidth ; l++)
			if ( txt[l] <= ' ' || txt[l]=='~')
				break;

	// word wrap
		if (l != con_linewidth && (con->x + l > con_linewidth) )
			con->x = 0;

		txt++;

		if (cr)
		{
			con->current--;
			cr = false;
		}

		
		if (!con->x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con->current >= 0)
				con_times[con->current % NUM_CON_TIMES] = cls.realtime;
		}

		if ((Print_flags[Print_current] & PR_IS_CHAT)
			&& (cl_parseWhiteText.value == 1 || (cl_parseWhiteText.value == 2 && team_chat)))
		{
			if (c == '{')
			{
				if (NULL != (parse_white = strchr (txt, '}')))
				{
					andmask = 127;
					continue;
				}
			}
			else if (parse_white && c == '}')
			{
				parse_white = NULL;
				andmask = 0;
				continue;
			}
		}

		switch (c)
		{
		case '\n':
			con->x = 0;
			break;

		case '\r':
			con->x = 0;
			cr = 1;
			break;
		case ' ':
			if (!parse_white) andmask=0;
		case '~':
			if (!parse_white)
			{
				andmask = andmask ? 127:255; 
				c=' ';
			}
		default:	// display character and advance
			y = con->current % con_totallines;
			con->text[y*con_linewidth+con->x] = (c | mask) & andmask;
			con->x++;
			if (con->x >= con_linewidth)
				con->x = 0;
			break;
		}
		
	}
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
#define	MAXPRINTMSG	4096
// FIXME: make a buffer size safe vsprintf?

unsigned	Print_flags[16];
int			Print_current = 0;

void Con_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	char		prefix[128];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	
// also echo to debugging console
	Sys_Printf ("%s", msg);	// also echo to debugging console

// Triggers with mask 64
	if (!(Print_flags[Print_current] & PR_TR_SKIP))
		CL_SearchForMsgTriggers (msg, RE_PRINT_INTERNAL);
	
// log all messages to file
	if (!(Print_flags[Print_current] & PR_LOG_SKIP)) {
		if (con_debuglog)
			Sys_DebugLog(va("%s/qconsole.log",com_gamedir), "%s", msg);

		if (log_filename.string[0]) {
			Cmd_ExpandString (log_prefix.string, prefix, sizeof(prefix));
			Sys_DebugLog(va("%s/%s",com_gamedir, log_filename.string), "%s%s", prefix, msg);
		}
	}

	if (!con_initialized)
		return;
		
// write it to the scrollable buffer

	if (!(Print_flags[Print_current] & PR_SKIP))
		Con_Print (msg);
	
	Print_flags[Print_current] = 0;

// update the screen immediately if the console is displayed
	if (cls.state != ca_active)
	{
	// protect against infinite loop if something in SCR_UpdateScreen calls
	// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen ();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if (!developer.value)
		return;			// don't confuse non-developers with techie stuff...

	Print_flags[Print_current] |= PR_TR_SKIP;
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	
	Con_Printf ("%s", msg);
}

/*
==============================================================================

DRAWING

==============================================================================
*/

int	con_linestart = 0; // Sergio

void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;
//	char	t;

	if (key_dest != key_console && cls.state == ca_active)
		return;		// don't draw anything (allways draw if not active)

// Changed by Sergio -->
	text = key_lines[edit_line];

	//	prestep if horizontally scrolling
	if (key_linepos == 1)
		con_linestart = 0;
	else {
		if (key_linepos-con_linestart >= con_linewidth) 
			con_linestart = key_linepos - con_linewidth+1;
		else if (key_linepos < con_linestart)
			con_linestart = key_linepos;
		text += con_linestart;
	}
		
	y = con_vislines-22;
	// draw text
	for (i=0 ; text[i] && i < con_linewidth; i++)
		Draw_Character ( (i+1)<<3, y, text[i]);

	// add the cursor frame	
	Draw_Character ( (key_linepos - con_linestart + 1)<<3, y, 10+((int)(curtime*con_cursorspeed)&1));
// <-- Sergio	
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int	x, v;
	char	*text;
	int	i;
	float	time;
	char	*s;
	int	skip = 0;
	char	t;

	v = 0;
	for (i= con->current-(int)con_notify_lines.value+1 ; i<=con->current ; i++) {
		if (i < 0)
			continue;
		time = con_times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime.value)
			continue;
		text = con->text + (i % con_totallines)*con_linewidth;
		
		clearnotify = 0;
		scr_copytop = 1;

		for (x = 0 ; x < con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, v, text[x]);

		v += 8;
	}

	if (key_dest == key_message && chat_team != 100)
	{
		clearnotify = 0;
		scr_copytop = 1;
	
		if (chat_team==1)
		{
			Draw_String (8, v, "say_team:");
			skip = 10;
		}
		else if (chat_team==2)
		{
			Draw_String (8, v, "say_id:");
			skip = 8;
		}
		else if (chat_team==3)
		{
			Draw_String (8, v, "msg:");
			skip = 5;
		}
		else if (chat_team==0)
		{
			Draw_String (8, v, "say:");
			skip = 5;
		}
// Sergio --> (new locs)
		else if (chat_team==4)
		{
			// loc block name
			Draw_String (8, v, "name:");
			skip = 6;
		}
		else if (chat_team==5)
		{
			// loc parameter edit
			extern char loc_param[];

			Draw_String (8, v, loc_param);
			skip = strlen(loc_param)+1;
		}
// <-- Sergio

		s = chat_buffer[chat_edit];
		t = chat_buffer[chat_edit][chat_bufferpos];
		i = chat_bufferpos;
		if (chat_bufferpos > (vid.conwidth>>3)-(skip+1)) {
			s += chat_bufferpos - ((vid.conwidth>>3)-(skip+1));
			i = ((vid.conwidth>>3)-(skip+1));
		}

		x = 0;
		while(s[x])
		{
			Draw_Character ( (x+skip)<<3, v, s[x]);
			x++;
		}
		Draw_Character ( (i+skip)<<3, v, 10+((int)(curtime*con_cursorspeed)&1));
		v += 8;
	}
	
	if (v > con_notifylines)
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (int lines)
{
	int				i, j, x, y, n;
	int				rows;
	char			*text;
	int				row;
	char			dlbar[1024];
	
	if (lines <= 0)
		return;

// draw the background
	Draw_ConsoleBackground (lines);

// draw the text
	con_vislines = lines;
	
// changed to line things up better
	rows = (lines-22)>>3;		// rows of text to draw

	y = lines - 30;

// draw from the bottom up
	if (con->display != con->current)
	{
	// draw arrows to show the buffer is backscrolled
		for (x=0 ; x<con_linewidth ; x+=4)
			Draw_Character ( (x+1)<<3, y, '^');
	
		y -= 8;
		rows--;
	}
	
	row = con->display;
	for (i=0 ; i<rows ; i++, y-=8, row--)
	{
		if (row < 0)
			break;
		if (con->current - row >= con_totallines)
			break;		// past scrollback wrap point
			
		text = con->text + (row % con_totallines)*con_linewidth;

		for (x=0 ; x<con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, y, text[x]);
	}

	// draw the download bar
	// figure out width
	if (cls.download || cls.upload) { // bliP
		if (cls.download) {
			if ((text = strrchr(cls.downloadname, '/')) != NULL)
				text++;
			else
				text = cls.downloadname;
		}
// bliP -->
		else if (cls.upload) {
			if ((text = strrchr(cls.uploadname, '/')) != NULL)
				text++;
			else
				text = cls.uploadname;
		}
		else
			return; //lol? - no ;-)
// <-- bliP

		x = con_linewidth - ((con_linewidth * 7) / 40);
		y = x - strlen(text) - 10;
		i = con_linewidth/3;
		if (strlen(text) > i) {
			y = x - i - 13;
			strlcpy (dlbar, text, i+1);
			strcat(dlbar, "...");
		} else
			strcpy(dlbar, text);
		strcat(dlbar, ": ");
		i = strlen(dlbar);
		dlbar[i++] = '\x80';
		// where's the dot go?
		if (cls.download)
			if (cls.downloadpercent == 0)
				n = 0;
			else
				n = y * cls.downloadpercent / 100;
		else if (cls.upload)
			if (cls.uploadpercent == 0)
				n = 0;
			else
				n = y * cls.uploadpercent / 100;
			
		for (j = 0; j < y; j++)
			if (j == n)
				dlbar[i++] = '\x83';
			else
				dlbar[i++] = '\x81';
		dlbar[i++] = '\x82';
		dlbar[i] = 0;

		i = strlen(dlbar);
		if (cls.download)
			sprintf(dlbar + i, " %02d%%(%dkb/s)", cls.downloadpercent, cls.downloadrate);
		else if (cls.upload)
			sprintf(dlbar + i, " %02d%%(%dkb/s)", cls.uploadpercent, cls.uploadrate);
		else
			return;

		// draw it
		y = con_vislines-22 + 8;
		for (i = 0; i < strlen(dlbar); i++)
			Draw_Character ( (i+1)<<3, y, dlbar[i]);

	}


// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}


/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox (char *text)
{
	double		t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf (text);

	Con_Printf ("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	key_dest = key_console;

	do
	{
		t1 = Sys_DoubleTime ();
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();
		t2 = Sys_DoubleTime ();
		curtime += t2-t1;		// make the cursor blink 
	} while (key_count < 0);

	Con_Printf ("\n");
	key_dest = key_game;
	curtime = 0;				// put the cursor back to invisible 
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
	int			temp;
		
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;
	Con_Printf ("%s", msg);
	scr_disabled_for_loading = temp;
}

