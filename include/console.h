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

//
// console
//

#define		CON_TEXTSIZE	32768
typedef struct
{
	char	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line
} console_t;

extern	console_t	con_main;
extern	console_t	con_chat;
extern	console_t	*con;			// point to either con_main or con_chat

extern int con_totallines;
extern qboolean con_initialized;
extern byte *con_chars;
extern	int	con_notifylines;		// scan lines to clear for notify lines
extern int	con_linestart;			// Sergio

void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (int lines);
void Con_Printf (char *fmt, ...);

// BorisU -->
extern unsigned	Print_flags[16];
extern int		Print_current;

#define		PR_SKIP		1
#define		PR_LOG_SKIP	2
#define		PR_TR_SKIP	4
#define		PR_IS_CHAT	8

typedef struct
{
	cvar_t	*var;
	byte	x, y, len, bg;
} input_t;

extern input_t input;
// <-- BorisU

void Con_DPrintf (char *fmt, ...);
void Con_SafePrintf (char *fmt, ...);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

void Con_NotifyBox (char *text);	// during startup for sound / cd warnings

void Con_Resize (console_t *con);
