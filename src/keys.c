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
#include "quakedef.h"
#ifdef _WIN32
#include <windows.h>
#endif

#include <ctype.h>

/*

key up events are sent even if in console mode

*/

cvar_t	cl_chatmode = {"cl_chatmode", "2"}; // Tonik

char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		shift_down=false;
int		key_lastpress;

int		edit_line=0;
int		history_line=0;

keydest_t	key_dest = key_console;

int		key_count;			// incremented every key event

char	*keybindings[256];
qboolean	consolekeys[256];	// if true, can't be rebound while in console
qboolean	menubound[256];	// if true, can't be rebound while in menu
int		keyshift[256];		// key to map to if shift held down in console
int		key_repeats[256];	// if > 1, it is autorepeating
byte	keydown[256];

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"LALT", K_LALT},
	{"RALT", K_RALT},
	{"CTRL", K_CTRL},
	{"LCTRL", K_LCTRL},
	{"RCTRL", K_RCTRL},
	{"SHIFT", K_SHIFT},
	{"LSHIFT", K_LSHIFT},
	{"RSHIFT", K_RSHIFT},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
// fuh ->
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},
	{"MOUSE6", K_MOUSE6},
	{"MOUSE7", K_MOUSE7},
	{"MOUSE8", K_MOUSE8},
// <- fuh

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

// Fuh -->
	{"WINKEY", K_WIN},
	{"LWINKEY", K_LWIN},
	{"RWINKEY", K_RWIN},
	{"POPUPMENU", K_MENU},
// <-- Fuh

// BorisU -->
	// Keypad stuff..
	// Tonik's code

	{"NUMLOCK", KP_NUMLOCK},
	{"KP_NUMLCK", KP_NUMLOCK},
	{"KP_NUMLOCK", KP_NUMLOCK},
	{"KP_SLASH", KP_SLASH},
	{"KP_DIVIDE", KP_SLASH},
	{"KP_STAR", KP_STAR},
	{"KP_MULTIPLY", KP_STAR},

	{"KP_MINUS", KP_MINUS},

	{"KP_HOME", KP_HOME},
	{"KP_7", KP_HOME},
	{"KP_UPARROW", KP_UPARROW},
	{"KP_8", KP_UPARROW},
	{"KP_PGUP", KP_PGUP},
	{"KP_9", KP_PGUP},

	{"KP_PLUS", KP_PLUS},

	{"KP_LEFTARROW", KP_LEFTARROW},
	{"KP_4", KP_LEFTARROW},
	{"KP_5", KP_5},
	{"KP_RIGHTARROW", KP_RIGHTARROW},
	{"KP_6", KP_RIGHTARROW},

	{"KP_END", KP_END},
	{"KP_1", KP_END},
	{"KP_DOWNARROW", KP_DOWNARROW},
	{"KP_2", KP_DOWNARROW},
	{"KP_PGDN", KP_PGDN},
	{"KP_3", KP_PGDN},

	{"KP_INS", KP_INS},
	{"KP_0", KP_INS},
	{"KP_DEL", KP_DEL},
	{"KP_DOT", KP_DEL},
	{"KP_ENTER", KP_ENTER},
	
	{"CAPSLOCK", K_CAPSLOCK},
	{"PRINTSCR", K_PRINTSCR},
	{"SCRLCK", K_SCRLCK},
	{"SCROLLCK", K_SCRLCK},
// <-- BorisU

#ifdef __APPLE__
	{"COMMAND", K_CMD},
	{"PARA", K_PARA},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},
	{"KP_EQUAL", KP_EQUAL},
#endif

	{"PAUSE", K_PAUSE},

	{"MWHEELUP", K_MWHEELUP},
	{"MWHEELDOWN", K_MWHEELDOWN},

	{"TWHEELLEFT", K_TWHEELLEFT},
	{"TWHEELRIGHT", K_TWHEELRIGHT},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};

// ezquake -->
#define COLUMNWIDTH 20
#define MINCOLUMNWIDTH 18	// the last column may be slightly smaller

void PaddedPrint (char *s) {	
	extern int con_linewidth;
	int	nextcolx = 0;

	if (con->x)
		nextcolx = (int)((con->x + COLUMNWIDTH)/COLUMNWIDTH)*COLUMNWIDTH;

	if (nextcolx > con_linewidth - MINCOLUMNWIDTH
		|| (con->x && nextcolx + strlen(s) >= con_linewidth))
		Con_Printf ("\n");

	if (con->x)
		Con_Printf (" ");
	while (con->x % COLUMNWIDTH)
		Con_Printf (" ");
	Con_Printf ("%s", s);
}
// <-- ezquake

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

qboolean CheckForCommand (void)
{
	char	command[128];
	char	*cmd, *s;
	int		i;

	s = key_lines[edit_line]+1;

	for (i=0 ; i<127 ; i++)
		if (s[i] <= ' ')
			break;
		else
			command[i] = s[i];
	command[i] = 0;

	cmd = Cmd_CompleteCommand (command,false);
	if (!cmd  || Q_stricmp (cmd, command) )
		return false;		// just a chat message
	return true;
}


int			key_linelength;
#ifdef _WIN32
static int			j;
#endif
static qboolean		completing = false;
static int			ehelp;

// Sergio -->
extern	int			num_alternatives;	

static char	ed_cmd[MAXCMDLINE];
static char	ed_line_start[MAXCMDLINE];
static char	ed_line_end[MAXCMDLINE];

qboolean IsDivider(char ch)
{ // return true, if char ch is word's divider
	
	char s[] = " ";

	if (!ch)
		return false;
	s[0] = ch;
	return (!strcspn(s, " \"\\/'()+-*;|@$"));
}
void CompleteCommand (void)
{
	char	*s;
	
	if (!completing) {
		ehelp=1;
		strcpy(ed_line_start, key_lines[edit_line]);
		s = ed_line_start + key_linepos;
		strcpy(ed_line_end, s);
		*s = '\0';
		while (--s > ed_line_start && !IsDivider(*s));

		if (*s == '+' || *s == '-')
			s--;

		if (*strcpy(ed_cmd, ++s) == '\0')
			return;
		
		*s = '\0';
		completing = true;
	} else {
		if (shift_down) { // back-rotate on SHIFT-TAB)
			if (--ehelp < 2)
				ehelp += num_alternatives; 
		} else
			ehelp++;
	}
	
	s = Cmd_CompleteCommand (ed_cmd, ehelp);
	strcpy(key_lines[edit_line], ed_line_start);
	if (s) {
		if (!key_lines[edit_line][1]) {
			key_lines[edit_line][1] = '/';
			key_lines[edit_line][2] = '\0';
		}
		strcat(key_lines[edit_line], s);
		if (!IsDivider(*ed_line_end))
			strcat(key_lines[edit_line], " ");
	} else {
		completing = false;
		strcat(key_lines[edit_line], ed_cmd);
	}
	
	key_linepos = strlen(key_lines[edit_line]);
	strcat(key_lines[edit_line], ed_line_end);
	key_linelength = strlen(key_lines[edit_line]);
}
// <-- Sergio

// Enter key was pressed in the console, do the appropriate action
static void HandleEnter (void)
{
	enum { COMMAND, CHAT, TEAMCHAT } type;
	char	*p;

	key_lines[edit_line][key_linelength] = 0;

	// decide whether to treat the text as chat or command
	if (keydown[K_CTRL])
		type = TEAMCHAT;
	else if (keydown[K_SHIFT])
		type = (cl_chatmode.value == 1) ? COMMAND : CHAT;
	else if (key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/')
		type = COMMAND;
	else if (cl_chatmode.value && cls.state >= ca_connected
			&& (cl_chatmode.value == 1 || !CheckForCommand()))
		type = CHAT;
	else
		type = COMMAND;

	// do appropriate action
	switch (type) {
	case CHAT:
	case TEAMCHAT:
		for (p = key_lines[edit_line] + 1; *p; p++) {
			if (*p != ' ')
				break;
		}
		if (!*p)
			break;		// just whitespace

		Cbuf_AddText (type == TEAMCHAT ? "say_team " : "say ");
		Cbuf_AddText (key_lines[edit_line]+1);
		Cbuf_AddText ("\n");
		break;

	case COMMAND:
		p = key_lines[edit_line] + 1;	// skip the "]" prompt char
		if (*p == '\\' || (*p == '/' && p[1] != '/'))
			p++;
		Cbuf_AddText (p);
		Cbuf_AddText ("\n");
		break;
	}

	Con_Printf ("%s\n",key_lines[edit_line]);
	
	if (strcmp(key_lines[edit_line], key_lines[(edit_line-1) & 31])) // Sergio
		edit_line = (edit_line + 1) & 31;
	history_line = edit_line;
	key_lines[edit_line][0] = ']';
	key_lines[edit_line][1] = 0;	// Sergio
	key_linelength = key_linepos = 1;

	if (cls.state == ca_disconnected)
		SCR_UpdateScreen ();	// force an update, because the command
								// may take some time

		return;
}
/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int key)
{
#ifdef _WIN32
	HANDLE	th;
	char	*clipText, *textCopied;
#endif
	int		i;

	// Correct by Sergio - K_SHIFT
	if (key != K_TAB && key != K_SHIFT && completing) completing = false; // BorisU

	if (key == K_ENTER) {
		HandleEnter();
		return;
	}

// Sergio -->
		if (key == K_TAB) {	// command completion
			CompleteCommand ();
			return;
		}
// <-- Sergio

	if (key == K_LEFTARROW)	{
		if (keydown[K_CTRL]) {
			// word left
// Sergio -->
			while (key_linepos > 1 && IsDivider(key_lines[edit_line][key_linepos-1]))
				key_linepos--;
			while (key_linepos > 1 && !IsDivider(key_lines[edit_line][key_linepos-1]))
				key_linepos--;
// <-- Sergio
		} else {
			if (key_linepos > 1) {
		// Correct by Sergio -->
				key_linepos--;
				if(key_linepos+1==key_linelength && key_lines[edit_line][key_linepos]==' ') {
					key_lines[edit_line][key_linepos]='\0';
					key_linelength--;
				}
		// <-- Sergio
			}
		}
		return;
	}

	if (key == K_RIGHTARROW) {			
		if (keydown[K_CTRL]) {
			// word right
// Correct by Sergio -->
			while (key_linepos < key_linelength && !IsDivider(key_lines[edit_line][key_linepos]))
				key_linepos++;
			while (key_linepos < key_linelength && IsDivider(key_lines[edit_line][key_linepos]))
				key_linepos++;
// <-- Sergio
		} else {
			if (key_linepos < key_linelength)
				key_linepos++;
		}
		return;
	}

	if (key == K_BACKSPACE ) {
		if (key_linepos > 1) {
			for (i = key_linepos; i < key_linelength; i++)
				key_lines[edit_line][i-1]=key_lines[edit_line][i];
			key_linepos--;
			key_linelength--;
			key_lines[edit_line][key_linelength] = 0;
		}
		return;
	}

	if (key == K_DEL) {
		if (key_linepos < key_linelength) {
			key_linepos++;
			for (i = key_linepos; i < key_linelength; i++)
				key_lines[edit_line][i-1]=key_lines[edit_line][i];
			key_linepos--;
			key_linelength--;
			key_lines[edit_line][key_linelength] = 0;
		}
		return;
	}

	if (key == K_UPARROW) {
		do {
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linelength = key_linepos = strlen(key_lines[edit_line]);		
		con_linestart = 0; // Sergio
		return;
	}

	if (key == K_DOWNARROW) {
		if (history_line == edit_line) return;
		do {
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		con_linestart = 0; // Sergio
		if (history_line == edit_line) {
			key_lines[edit_line][0] = ']';
			key_lines[edit_line][1] = 0; // Sergio
			key_linelength = key_linepos = 1;
		} else {
			strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linelength = key_linepos = strlen(key_lines[edit_line]);
		}
		return;
	}

	if (key == K_PGUP || key==K_MWHEELUP) {
		con->display -= 2;
		return;
	}

	if (key == K_PGDN || key==K_MWHEELDOWN) {
		con->display += 2;
		if (con->display > con->current)
			con->display = con->current;
		return;
	}

	if (key == K_HOME) {
		if (key_linelength == 1 || keydown[K_CTRL]) 
			con->display = con->current - con_totallines + 10;
		else
			key_linepos = 1;
		return;
	}

	if (key == K_END) {
		if (key_linelength == 1 || keydown[K_CTRL])
			con->display = con->current;
// Sergio -->
		else {
			while (key_lines[edit_line][key_linelength-1] == ' ') {
				key_linelength--;
				key_lines[edit_line][key_linelength] = 0;
			}
			key_linepos = key_linelength;
		}
// <-- Sergio
		return;
	}
	
#ifdef _WIN32
	if ((key=='V' || key=='v') && GetKeyState(VK_CONTROL)<0) {
		if (OpenClipboard(NULL)) {
			th = GetClipboardData(CF_TEXT);
			if (th) {
				clipText = GlobalLock(th);
				if (clipText) {
					textCopied = malloc(GlobalSize(th)+1);
					strcpy(textCopied, clipText);
	/* Substitutes a NULL for every token */strtok(textCopied, "\n\r\b");
					i = strlen(textCopied);
					if (i+key_linelength>=MAXCMDLINE)
						i=MAXCMDLINE-key_linelength;
					if (i>0) {
						textCopied[i]=0;
						//strcat(key_lines[edit_line], textCopied);
						for (j = key_linelength; j >= key_linepos; j--)
								key_lines[edit_line][j] = key_lines[edit_line][j-i];
						for (j = key_linepos; j < key_linepos+i; j++)
							key_lines[edit_line][j] = textCopied[j - key_linepos];
						key_linepos+=i;
						key_linelength+=i;
					}
					free(textCopied);
				}
				GlobalUnlock(th);
			}
			CloseClipboard();
		return;
		}
	}
#endif

	if (key < 32 || key > 127)
		return;	// non printable
		
	if (keydown[K_CTRL]) {
		if (key >= '0' && key <= '9')
			key = key - '0' + 0x92;	// yellow number
		else if (key == '[')
			key = 0x90;
		else if (key == ']')
			key = 0x91;
		else if (key == 'g')
			key = 0x86;
		else if (key == 'r')
			key = 0x87;
		else if (key == 'y')
			key = 0x88;
		else if (key == 'b')
			key = 0x89;
	}

	if (keydown[K_ALT])
		key |= 128;		// red char

	if (key_linelength < MAXCMDLINE-1) {
		for (i = key_linelength; i >= key_linepos; i--)
			key_lines[edit_line][i] = key_lines[edit_line][i-1];
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
		key_linelength++;
		key_lines[edit_line][key_linelength] = 0;
	}
}

//============================================================================

short int	chat_team;
char		chat_buffer[CHAT_HISTORY][MAXCMDLINE];
int			chat_bufferlen = 0;

// 5ATOH -->
int			chat_edit = 0;
int			chat_history = 0;
// <--5ATOH

int			chat_bufferpos = 0;


extern cvar_t uid_cvar;
extern int marks[];
void Key_Message (int key)
{
	char s[100];
	int i;
	if (key == K_ENTER || key == KP_ENTER) {
		if (chat_team==1)
			Cbuf_AddText ("say_team \"");
		else if (chat_team==2)
		{
			Cbuf_AddText ("say_id ");
			Cbuf_AddText (uid_cvar.string);
			Cbuf_AddText (" \"");
		}
		else if(chat_team==3)
		{
			if(*chat_buffer[chat_edit]=='%') { 
				i=atoi(chat_buffer[chat_edit]+1); 
				if(i<0 || i>=max_locmsgs) { 
					Con_Printf("msg_num out of range\n"); 
					i=max_locmsgs-1; 
				}
			} else {
				for(i=0;i<max_locmsgs && locinfo[i];i++) ; // serching for empty slot
				sprintf(s,"loc_msg %d \"%s\"\n",i,chat_buffer[chat_edit]);
				Cbuf_AddText(s);
			}
			//locpoints++;
			sprintf(s,"loc_point %d %d %d %d %d %d %d\n",marks[0],marks[1],marks[2],marks[3],marks[4],marks[5],i);
			Cbuf_AddText(s);
			sprintf(s,"echo added point~%d \"",locpoints);
			Cbuf_AddText(s);
		}
// Sergio --> (new locs)
		else if (chat_team == 4)
		{
			sprintf(s,"loc_point %d %d %d %d %d %d \"%s\"\n",marks[0],marks[1],marks[2],marks[3],marks[4],marks[5],chat_buffer[chat_edit]);
			Cbuf_AddText(s);
			sprintf(s,"echo added point~%d \"",locpoints);
			Cbuf_AddText(s);
		}
		else if(chat_team==5)
		{
			extern char loc_param[];
			extern int	*loc_var;
			extern player_loc_t *loc_block;
			void Loc_SortCoord(player_loc_t *loc);

			if (*loc_param == 'n')
			{
				if (loc_block->name)
					Z_Free((void *)loc_block->name);
				loc_block->name = Z_StrDup(chat_buffer[chat_edit]);
				memset(&loc_block->msg, 0, sizeof(loc_block->msg)); // clear messages
			} else {
				*loc_var = atoi(chat_buffer[chat_edit]);
				Loc_SortCoord(loc_block);
				if (*loc_param == 'm' && loc_block->name) {
					// clear name
					Z_Free((void *)loc_block->name);
					loc_block->name = NULL;
				}
			}
			// update selected block info
			Con_Printf("\n");
			sprintf(s, "* %i", loc_cur_block);
			Cmd_TokenizeString(s);
			Cmd_LocSelectBlock_f();
		}
// <-- Sergio
		else if (chat_team==0)
		 Cbuf_AddText ("say \"");
// BorisU -->
		if (chat_team==100){ 
			Cvar_Set(input.var, chat_buffer[chat_edit]);
			key_dest = key_game;
		} else if (chat_team != 5) { // Sergio (new locs)
// <-- BorisU
			Cbuf_AddText(chat_buffer[chat_edit]);
			Cbuf_AddText("\"\n");
		}

		key_dest = key_game;

		// 5ATOH
		if (strcmp (chat_buffer[chat_edit], chat_buffer[(chat_edit - 1) & CHAT_HISTORY_MASK]))
			chat_edit = (chat_edit + 1) & CHAT_HISTORY_MASK;
		
		chat_history = chat_edit; // Sergio
		chat_bufferlen = 0;
		chat_bufferpos = 0;

		chat_buffer[chat_edit][0] = 0; // 5ATOH
		CL_ExecTriggerSafe ("f_input_done");	
		return;
	}

	if (key == K_ESCAPE)
	{
		if (key_dest == key_message)
			CL_ExecTriggerSafe ("f_input_done");	

		key_dest = key_game;
		chat_bufferlen = 0;
		chat_bufferpos = 0;
		chat_buffer[chat_edit][0] = 0;
		return;
	}

	if (key == K_LEFTARROW)
		if (chat_bufferpos) chat_bufferpos--;

	if (key == K_RIGHTARROW)
		if (chat_bufferpos < chat_bufferlen) chat_bufferpos++;

	if (key == K_HOME)
		chat_bufferpos = 0;

	if (key == K_END)
		chat_bufferpos = chat_bufferlen;

	if (key == K_DEL)
	{
		if (chat_bufferpos < chat_bufferlen)
		{
			chat_bufferpos++;
			for (i = chat_bufferpos-1; i < chat_bufferlen; i++)
				chat_buffer[chat_edit][i] = chat_buffer[chat_edit][i+1];
			chat_bufferlen--;
			chat_bufferpos--;
		}
	}
	
	if (key == K_UPARROW || key == KP_UPARROW)
	{
		do	{
			chat_history = (chat_history - 1) & CHAT_HISTORY_MASK;
		} while (chat_history != chat_edit
				&& !chat_buffer[chat_history][0]);

		if (chat_history == chat_edit) {
			chat_history = (chat_edit+1) & CHAT_HISTORY_MASK;
		} else {
			strcpy(chat_buffer[chat_edit], chat_buffer[chat_history]);
			chat_bufferlen = chat_bufferpos = strlen(chat_buffer[chat_edit]);
		}
		return;
	}

	if (key == K_DOWNARROW || key == KP_DOWNARROW)
	{
		if (chat_history == chat_edit) return;
		do {
			chat_history = (chat_history + 1) & CHAT_HISTORY_MASK;
		} while (chat_history != chat_edit
				&& !chat_buffer[chat_history][0]);

		if (chat_history == chat_edit) {
			chat_buffer[chat_edit][0] = 0;
			chat_bufferlen = chat_bufferpos = 0;
		} else {
			strcpy(chat_buffer[chat_edit], chat_buffer[chat_history]);
			chat_bufferlen = chat_bufferpos = strlen(chat_buffer[chat_edit]);
		}
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (key == K_BACKSPACE)
	{
		if (chat_bufferpos)
		{
			for (i = chat_bufferpos-1; i < chat_bufferlen; i++)
				chat_buffer[chat_edit][i] = chat_buffer[chat_edit][i+1];
			chat_bufferlen--;
			chat_bufferpos--;
			//chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	
	if (chat_bufferlen == sizeof(chat_buffer)-1)
		return; // all full

	for (i = chat_bufferlen; i >= chat_bufferpos; i--)
		chat_buffer[chat_edit][i] = chat_buffer[chat_edit][i-1];
	chat_buffer[chat_edit][chat_bufferpos++] = key;
	chat_bufferlen++;
	chat_buffer[chat_edit][chat_bufferlen] = 0;
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!Q_stricmp(str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127 && keynum != ';')
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, char *binding)
{
	if (keynum == -1)
		return;

#ifndef __APPLE__
	if (keynum == K_CTRL || keynum == K_ALT || keynum == K_SHIFT || keynum == K_WIN) {
		Key_SetBinding(keynum + 1, binding);
		Key_SetBinding(keynum + 2, binding);
		return;
	}
#endif

// free old bindings
	if (keybindings[keynum]) {
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding

	keybindings[keynum] = Z_StrDup(binding);
}

/*
===================
Key_Unbind
===================
*/
void Key_Unbind (int keynum)
{
	if (keynum == -1)
		return;

	if (keynum == K_CTRL || keynum == K_ALT || keynum == K_SHIFT || keynum == K_WIN) {
		
		Key_Unbind(keynum + 1);
		Key_Unbind(keynum + 2);
		return;
	}

	if (keybindings[keynum]) {
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_Unbind (b);
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			Key_Unbind (i);
}

static void Key_PrintBindInfo(int keynum, char *keyname)
{
	if (!keyname)
		keyname = Key_KeynumToString(keynum);

	if (keynum == -1) {
		Con_Printf ("\"%s\" isn't a valid key\n", keyname);
		return;
	}

	if (keybindings[keynum])
		Con_Printf ("\"%s\" = \"%s\"\n", keyname, keybindings[keynum]);
	else
		Con_Printf ("\"%s\" is not bound\n", keyname);
}

//checks if LCTRL and RCTRL are both bound and bound to the same thing
qboolean Key_IsLeftRightSameBind(int b) {
	if (b < 0 || b >= sizeof(keybindings))
		return false;

	return	(b == K_CTRL || b == K_ALT || b == K_SHIFT || b == K_WIN) &&
			(keybindings[b + 1] && keybindings[b + 2] && !strcmp(keybindings[b + 1], keybindings[b + 2]));
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c < 2)
	{
		Con_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2) {
#ifndef __APPLE__
		if ((b == K_CTRL || b == K_ALT || b == K_SHIFT || b == K_WIN) && (keybindings[b + 1] || keybindings[b + 2])) {
			
			if (keybindings[b + 1] && keybindings[b + 2] && !strcmp(keybindings[b + 1], keybindings[b + 2])) {
				Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b + 1]);
			} else {
				Key_PrintBindInfo(b + 1, NULL);
				Key_PrintBindInfo(b + 2, NULL);
			}
		} else
#endif
		{
			
			//		and the following should print "ctrl (etc) is not bound" since K_CTRL cannot be bound
			Key_PrintBindInfo(b, Cmd_Argv(1));
		}
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}

/*
===================
Key_BindList_f
===================
*/
void Key_BindList_f (void)
{
	int i;

	for (i = 0; i < 256; i++) {
		if (Key_IsLeftRightSameBind(i)) {
			Con_Printf ("%s \"%s\"\n", Key_KeynumToString(i), keybindings[i + 1]);
			i += 2;	
		} else {
			if (keybindings[i])
				Con_Printf ("%s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
		}
	}
}


/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int i, leftright;

	for (i = 0; i < 256; i++) {
		leftright = Key_IsLeftRightSameBind(i) ? 1 : 0;
		if (leftright || keybindings[i]) {
			if (i == ';')
				fprintf (f, "bind \";\" \"%s\"\n", keybindings[i]);
			else
				fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[leftright ? i + 1 : i]);

			if (leftright)
				i += 2;
		}
	}
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<32 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	key_linelength = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_ALT] = true;
	consolekeys[K_LALT] = true;
	consolekeys[K_RALT] = true;
	consolekeys[K_CTRL] = true;
	consolekeys[K_LCTRL] = true;
	consolekeys[K_RCTRL] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_LSHIFT] = true;
	consolekeys[K_RSHIFT] = true;
#ifdef __APPLE__
	consolekeys[K_CMD] = true;
#endif
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys[K_TWHEELLEFT] = true;
	consolekeys[K_TWHEELRIGHT] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i=0 ; i<256 ; i++)
		keyshift[i] = i;
	for (i='a' ; i<='z' ; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bindlist",Key_BindList_f);
	Cmd_AddCommandTrig ("bind",Key_Bind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);

	Cvar_RegisterVariable (&cl_chatmode); // Tonik
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down)
{
	char	*kb;
	char	cmd[1024];
	byte	old_keydown = 0;

//	Con_Printf ("%i \"%s\": %i\n", key, Key_KeynumToString(key), down); //@@@

	if (key == K_LALT || key == K_RALT)
		Key_Event (K_ALT, down);
	else if (key == K_LCTRL || key == K_RCTRL)
		Key_Event (K_CTRL, down);
	else if (key == K_LSHIFT || key == K_RSHIFT)
		Key_Event (K_SHIFT, down);
	else if (key == K_LWIN || key == K_RWIN)
		Key_Event (K_WIN, down);

	if (!down) {
		key_repeats[key] = 0;
		old_keydown = keydown[key];
		keydown[key] = false;
	} else
		keydown[key] = key_dest;

	key_lastpress = key;
	key_count++;
	if (key_count <= 0) {
		return;		// just catching keys for Con_NotifyBox
	}

// update auto-repeat status
	if (down) {
		if(++key_repeats[key] > 1)
			if ((key_dest == key_game && cls.state == ca_active) ||
				(key != K_BACKSPACE 
				&& key != K_PGUP 
				&& key != K_PGDN
				&& key != K_LEFTARROW
				&& key != K_RIGHTARROW
				&& key != K_DEL)
				)
				return;	// ignore most autorepeats
	}

	if (key == K_SHIFT)
		shift_down = down;

//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
		if (!down)
			return;
		switch (key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
		case key_console:
			if (scr_conlines < vid.height) {	// in-game console
				Con_ToggleConsole_f ();			// just close console
				break;		
			}
		case key_game:
			M_ToggleMenu_f ();
			break;
		default:
			Sys_Error ("Bad key_dest");
		}
		return;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		kb = keybindings[key];
		
		if (kb && (!consolekeys[key] || old_keydown == key_game))
		{
			if (*kb==Cmd_PrefixChar) kb++;
			if (*kb == '+') {
				//sprintf (cmd, "-%s\n", kb+1);
				sprintf (cmd, "-%s %i\n", kb+1, key);
				Cbuf_AddText (cmd);
			}
			if (keyshift[key] != key)
			{
				kb = keybindings[keyshift[key]];
				if (kb && kb[0] == '+') {
					//sprintf (cmd, "-%s\n", kb+1);
					sprintf (cmd, "-%s %i\n", kb+1, key);
					Cbuf_AddText (cmd);
				}
			}
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (cls.demoplayback && !cls.mvdplayback && down && consolekeys[key] && key_dest == key_game && cl_demoplay_restrictions.value)
	{
		M_ToggleMenu_f ();
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if ( (key_dest == key_menu && menubound[key])
	|| (key_dest == key_console && !consolekeys[key])
	|| (key_dest == key_game && ( cls.state == ca_active || !consolekeys[key] ) ) ) {
		kb = keybindings[key];
		if (kb) {
			if (kb[0] == '+') {	// button commands add keynum as a parm
				sprintf (cmd, "%s %i\n", kb, key);
				Cbuf_AddText (cmd);
			} else {
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (!down)
		return;		// other systems only care about key down events

	if (shift_down)
		key = keyshift[key];

	switch (key_dest)
	{
	case key_message:
		Key_Message (key);
		break;
	case key_menu:
		M_Keydown (key);
		break;

	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Sys_Error ("Bad key_dest");
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
	{
		keydown[i] = false;
		key_repeats[i] = false;
	}
}

