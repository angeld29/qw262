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
// cmd.c -- Quake script command processing module

#include <ctype.h>
#include "quakedef.h"

#ifndef SERVERONLY
void Cmd_ForwardToServer (void);
qboolean CL_CheckServerCommand (void);
#endif

cmdalias_t	*cmd_alias;
cmdalias_t	*cmd_alias_hash[HT_SIZE];

qboolean	cmd_wait;

cvar_t cl_warncmd = {"cl_warncmd", "0"};

// Added by BorisU -->
cvar_t cl_warnexec = {"cl_warnexec", "1"};
#ifndef SERVERONLY
cvar_t cl_completion_sort = {"cl_completion_sort", "1"};
qboolean OnPrefixCharChange (cvar_t *var, const char *value);
cvar_t cl_prefixchar = {"cl_prefixchar", "@", 0, OnPrefixCharChange};
char	Cmd_PrefixChar = '@';

qboolean OnPrefixCharChange (cvar_t *var, const char *value)
{
	char buf[2];

	switch (*value) {
	case '@':
	case '$':
		Cmd_PrefixChar = *value;
		break;
	default:
		Con_Printf("Invalid prefix char\n");
		return true;
	}
	
	buf[0] = Cmd_PrefixChar;
	buf[1] = '\0';

	Cvar_Set (var, buf);
	return true;
}

#endif
// <-- BorisU

cbuf_t	cbuf_main;
#ifndef SERVERONLY
cbuf_t	cbuf_svc;
cbuf_t	cbuf_trig;
#endif

cbuf_t	*cbuf_current = NULL;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{	
	//if (cbuf_current)
		cbuf_current->wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

void Cbuf_AddText (char *text) { Cbuf_AddTextEx (cbuf_current?cbuf_current:&cbuf_main, text); }
void Cbuf_InsertText (char *text) { Cbuf_InsertTextEx (cbuf_current?cbuf_current:&cbuf_main, text); }
void Cbuf_Execute () { Cbuf_ExecuteEx (cbuf_current?cbuf_current:&cbuf_main); }

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	cbuf_current = &cbuf_main;
	cbuf_main.text_start = cbuf_main.text_end = MAXCMDBUF / 2;
	cbuf_main.wait = false;

#ifndef SERVERONLY
	cbuf_trig.text_start = cbuf_trig.text_end = MAXCMDBUF / 2;
	cbuf_trig.wait = false;
	cbuf_svc.text_start = cbuf_svc.text_end = MAXCMDBUF / 2;
	cbuf_svc.wait = false;
#endif
}

static 
char* Cbuf_Name (cbuf_t *cbuf)
{
	if (cbuf == &cbuf_main)
		return "MAIN";
#ifndef SERVERONLY
	else if (cbuf == &cbuf_trig)
		return "TRIG";
	else if (cbuf == &cbuf_svc)
		return "SVC";
#endif
	else
		return "BUG!"; // :)
}

static void Cbuf_Dump (cbuf_t *cbuf)
{
	FILE*	dump_file;
	dump_file = fopen (va("%s/%s",com_gamedir, "cbuf.dmp"), "wb");
	fwrite (cbuf->text_buf + cbuf->text_start, cbuf->text_end - cbuf->text_start, 1, dump_file);
	fclose (dump_file);
}

/*
============
Cbuf_AddTextEx

Adds command text at the end of the buffer
============
*/
void Cbuf_AddTextEx (cbuf_t *cbuf, char *text)
{
	int		len;
	int		new_start;
	int		new_bufsize;
	
	len = strlen (text);

	if (cbuf->text_end + len <= MAXCMDBUF) {
		memcpy (cbuf->text_buf + cbuf->text_end, text, len);
		cbuf->text_end += len;
		return;
	}

	new_bufsize = cbuf->text_end-cbuf->text_start+len;
	if (new_bufsize > MAXCMDBUF) {
		Con_Printf ("Cbuf_AddText: overflow (%s)\n", Cbuf_Name(cbuf));
		Cbuf_Dump (cbuf);
		cbuf->text_start = cbuf->text_end = MAXCMDBUF / 2;
		cbuf->wait = false;
		return;
	}

	// Calculate optimal position of text in buffer
	new_start = (MAXCMDBUF - new_bufsize) / 2;

	memcpy (cbuf->text_buf + new_start, cbuf->text_buf + cbuf->text_start, cbuf->text_end-cbuf->text_start);
	memcpy (cbuf->text_buf + new_start + cbuf->text_end-cbuf->text_start, text, len);
	cbuf->text_start = new_start;
	cbuf->text_end = cbuf->text_start + new_bufsize;
}


/*
============
Cbuf_InsertTextEx

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertTextEx (cbuf_t *cbuf, char *text)
{
	int		len;
	int		new_start;
	int		new_bufsize;

	len = strlen(text);

	if (len < cbuf->text_start)
	{
		memcpy (cbuf->text_buf + (cbuf->text_start - len), text, len);
		cbuf->text_start -= len;
		return;
	}

	new_bufsize = cbuf->text_end - cbuf->text_start + len;
	if (new_bufsize > MAXCMDBUF)
	{
		Con_Printf ("Cbuf_InsertText: overflow (%s)\n", Cbuf_Name(cbuf));
		Cbuf_Dump (cbuf);
		cbuf->text_start = cbuf->text_end = MAXCMDBUF / 2;
		cbuf->wait = false;
		return;
	}

	// Calculate optimal position of text in buffer
	new_start = (MAXCMDBUF - new_bufsize) / 2;

	memmove (cbuf->text_buf + (new_start + len), cbuf->text_buf + cbuf->text_start, cbuf->text_end-cbuf->text_start);
	memcpy (cbuf->text_buf + new_start, text, len);
	cbuf->text_start = new_start;
	cbuf->text_end = cbuf->text_start + new_bufsize;
}

/*
============
Cbuf_ExecuteEx
============
*/
void Cbuf_ExecuteEx (cbuf_t *cbuf)
{
	int		i;
	char	*text, *lineptr;
	char	line[1024];
	int		quotes;
	int		cursize;
	cbuf_t	*cbuf_old;

	cbuf_old = cbuf_current;
	cbuf_current = cbuf;

	while (cbuf->text_end > cbuf->text_start) {
// find a \n or ; line break
		text = (char *)cbuf->text_buf + cbuf->text_start;
		lineptr = line;
		cursize = cbuf->text_end - cbuf->text_start;
		quotes = 0;
		for (i=0 ; i<cursize && (lineptr - line) < sizeof(line) - 10; i++) {
#ifndef SERVERONLY
			if (text[i] == '\\') {
				if (text[i+1] == '\"' && cl_stringescape.value) {
					*lineptr++ = text[i++];
					*lineptr++ = text[i];
					continue;
				} else if (text[i+1] == '\n') { // escaped endline
					i++;
					continue;
				} else if (text[i+1] == '\r' && text[i+2] == '\n') { // escaped dos endline
					i+=2;
					continue;
				}
			} else 
#endif
// Sergio -->
				if (text[i] == '\"' && quotes <= 0) {
					if (!quotes)
						quotes = -1;
					else
						quotes = 0;
				} 
#ifndef SERVERONLY
				else if (quotes >= 0) {
					if (text[i] == '{')
						quotes++;
					else if (text[i] == '}')
						quotes--;
				}
#endif
		if ( !quotes && text[i] == ';')
// <-- Sergio
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;

			*lineptr++ = text[i];
		}

		if ((lineptr - line) >= sizeof(line) - 10)
			Con_Printf("WARNING: script line too long!\n");
// Tonik
// don't execute lines without ending \n; this fixes problems with
// partially stuffed aliases not being executed properly

#ifndef SERVERONLY
		if (cbuf_current == &cbuf_svc && i == cursize)
			break;
#endif

		*lineptr = '\0';
		if (lineptr > line && lineptr[-1] == '\r')
			lineptr[-1] = '\0';	// remove DOS ending CR; Tonik
	
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cursize) {
				cbuf->text_start = cbuf->text_end = MAXCMDBUF/2;
		} else {
			i++;
			cbuf->text_start += i;
		}

// execute the command line

		Cmd_ExecuteString (line);

		if (cbuf->wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cbuf->wait = false;
			break;
		}
	}

	cbuf_current = cbuf_old;
}

// BorisU -->
#ifndef SERVERONLY
/*
============
Cbuf_FullExecuteEx
============
*/
void Cbuf_FullExecuteEx (cbuf_t *cbuf)
{
	while (cbuf->text_start != cbuf->text_end) {
		Cbuf_ExecuteEx (cbuf);
	}
}

void Cbuf_FullExecute (void)
{
	Cbuf_FullExecuteEx(&cbuf_main);
	Cbuf_FullExecuteEx(&cbuf_svc);
	Cbuf_FullExecuteEx(&cbuf_trig);
}
#endif
// <-- BorisU

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds_f (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;
		
// build the combined string to parse from
	s = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += strlen (com_argv[i]) + 1;
	}
	if (!s)
		return;
		
	text = Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		strcat (text,com_argv[i]);
		if (i != com_argc-1)
			strcat (text, " ");
	}
	
// pull out the commands
	build = Z_Malloc (s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			strcat (build, text+i);
			strcat (build, "\n");
			text[j] = c;
			i = j-1;
		}
	}
	
	if (build[0])
		Cbuf_InsertText (build);
	
	Z_Free (text);
	Z_Free (build);
}

/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	// FIXME: is this safe freeing the hunk here???
	mark = Hunk_LowMark ();
	f = (char *)COM_LoadHunkFile (Cmd_Argv(1));
	if (!f)	{
		if(((cl_warncmd.value && cl_warnexec.value) || developer.value))
			Con_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	if (((cl_warncmd.value && cl_warnexec.value) || developer.value))
		Con_Printf ("execing %s\n",Cmd_Argv(1));

#ifndef SERVERONLY
	if (cbuf_current == &cbuf_svc) {
		Cbuf_AddTextEx (&cbuf_main, f);
		Cbuf_AddTextEx (&cbuf_main, "\n");
	} else
#endif
	{
		Cbuf_InsertText ("\n");
		Cbuf_InsertText (f);
	}

	Hunk_FreeToLowMark (mark);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
#ifdef SERVERONLY
	Con_Printf ("%s\n",Cmd_Args());
#else	
	int		i;
	char	*str;
	char	args[SAY_TEAM_CHAT_BUFFER_SIZE];
	char	buf[SAY_TEAM_CHAT_BUFFER_SIZE];

	
	args[0]='\0';

	str = Q_strcat(args, Cmd_Argv(1));
	for (i=2 ; i<Cmd_Argc() ; i++) {
		str = Q_strcat(str, " ");
		str = Q_strcat(str, Cmd_Argv(i));
	}

	str = ParseSay(args);
	strcpy(buf,str);
	CL_SearchForMsgTriggers (buf, RE_PRINT_ECHO); 	// BorisU
	Print_flags[Print_current] |= PR_TR_SKIP;
	Con_Printf ("%s\n", buf);
#endif
}

cmdalias_t *Cmd_FindAlias (char *name)
{
	cmdalias_t	*alias;
	unsigned	h; // BorisU

	h = Hash(name);

	for (alias = cmd_alias_hash[h]; alias ; alias = alias->hash_next)
	{
		if (!Q_stricmp(name, alias->name))
			return alias;
	}
	return NULL;
}

// Added by BorisU (for message triggers)
char *Cmd_AliasString (char *name)
{
	cmdalias_t	*alias;
	unsigned	h; // BorisU

	h = Hash(name);

	for (alias = cmd_alias_hash[h]; alias ; alias = alias->hash_next)
	{
		if (!Q_stricmp(name, alias->name))
#ifdef EMBED_TCL
			if (!(alias->flags & ALIAS_TCL))
#endif
			return alias->value;
	}
	return NULL;
}

#ifndef SERVERONLY
static cmdalias_t* Cmd_AliasCreate (char* name)
{
	cmdalias_t	*a;
	unsigned	h;

	h = Hash (name);
	a = Z_Malloc (sizeof(cmdalias_t));
	a->next = cmd_alias;
	cmd_alias = a;
	a->hash_next = cmd_alias_hash[h];
	cmd_alias_hash[h] = a;
	
	strcpy (a->name, name); // FIXME: check size
	return a;
}
#endif

/*
===============
Cmd_Viewalias_f

===============
*/
void Cmd_Viewalias_f(void)
{
	cmdalias_t	*a;
	char		*name;
	int			i,m;

	if (Cmd_Argc() < 2) {
		Con_Printf ("viewalias <aliasname> : view body of alias\n");
		return;
	}

	name = Cmd_Argv(1);

	if ( IsRegexp(name) ) {
		if (!ReSearchInit(name))
			return;
		Con_Printf ("Current alias commands:\n");

		for (a = cmd_alias, i=m=0; a ; a=a->next, i++)
			if (ReSearchMatch(a->name)) {
#ifdef EMBED_TCL
				if (a->flags & ALIAS_TCL)
					Con_Printf ("%s : Tcl procedure\n", a->name);
				else
#endif
				Con_Printf ("%s : %s\n", a->name, a->value);
				m++;
			}

		Con_Printf ("------------\n%i/%i aliases\n", m, i);
		ReSearchDone();
		

	} else {
		a = Cmd_FindAlias(name);

		if (a)
#ifdef EMBED_TCL
			if (a->flags & ALIAS_TCL)
				Con_Printf ("%s : Tcl procedure\n", name);
			else
#endif
			Con_Printf ("%s : \"%s\"\n", name, a->value);
		else
			Con_Printf ("No such alias: %s\n", name);
	}

}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/	

void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		*cmd;
	int			i, c;
	char		*name;
	char		*v;
	char		*s;
	unsigned	h; // BorisU

	c = Cmd_Argc();
	name = Cmd_Argv(1);

	if (c == 1) {
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias, i=0; a ; a=a->next, i++)
#ifdef EMBED_TCL
			if (a->flags & ALIAS_TCL)
				Con_Printf ("%s : Tcl procedure\n", a->name);
			else
#endif
			Con_Printf ("%s : %s\n", a->name, a->value);
		Con_Printf ("------------\n%i aliases\n", i);
		return;
	}

	s = name;
	if (strlen(s) >= MAX_ALIAS_NAME) {
		Con_Printf ("Alias name is too long\n");
		return;
	}

	h = Hash(s);
	// if the alias already exists, reuse it
	for (a = cmd_alias_hash[h] ; a ; a=a->hash_next) {
		if (!Q_stricmp(s, a->name)){
			Z_Free (a->value);
			a->flags = 0;
			break;
		}
	}

	if(Cvar_FindVar(s)) {
		Con_Printf("You can't realias cvar_variable \"%s\"\n",s);
		return;
	}

	if(Cmd_FindCommand(s)) {
		Con_Printf("You can't realias command \"%s\"\n",s);
		return;
	}

// New alias
	if (!a)
	{
		a = Z_Malloc (sizeof(cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;
		a->hash_next = cmd_alias_hash[h];
		cmd_alias_hash[h] = a;
	}

	strcpy (a->name, s);

	cmd = Cmd_MakeArgs(2);

	v = Cmd_Argv(2);
//	if ( Cmd_Argc() == 3 && !strchr(v, ' ') && !strchr(v, '\t') &&
//		(Cmd_FindCommand(v) && a->value[0] != '+' && a->value[0] != '-') ) {
//		// simple alias like "alias e echo"
//		a->flags |= ALIAS_HAS_PARAMETERS;
//		strcat (cmd, " %0");
//	} else {
	{
		s=cmd;
		while (*s) {
			if (*s == '%' && ( s[1]>='0' || s[1]<='9')) {
				a->flags |= ALIAS_HAS_PARAMETERS;
				break;
			}
			++s;
		}
	}
	
#ifndef SERVERONLY
	if (cbuf_current == &cbuf_svc)
		a->flags |= ALIAS_SERVER;
#endif

	a->value = Z_StrDup (cmd);
}

// Tonik -->
qboolean Cmd_DeleteAlias (char *name)
{
	cmdalias_t	*a, *prev;
	unsigned		h;

	h = Hash(name);

	prev = NULL;
	for (a = cmd_alias_hash[h] ; a ; a = a->hash_next)
	{
		if (!Q_stricmp(a->name, name))
		{
			// unlink from hash
			if (prev)
				prev->hash_next = a->hash_next;
			else
				cmd_alias_hash[h] = a->hash_next;
			break;
		}
		prev = a;
	}

	if (!a)
		return false;	// not found

	prev = NULL;
	for (a = cmd_alias ; a ; a = a->next)
	{
		if (!Q_stricmp(a->name, name))
		{
			// unlink from alias list
			if (prev)
				prev->next = a->next;
			else
				cmd_alias = a->next;

			// free
			Z_Free (a->value);
			Z_Free (a);			
			return true;
		}
		prev = a;
	}

	Sys_Error ("Cmd_DeleteAlias: alias list broken");
	return false;	// shut up compiler
}

void Cmd_UnAlias_f (void)
{
	int i;
	char		*name;
	cmdalias_t	*a;
	qboolean	re_search;
	
	for (i=1; i<Cmd_Argc(); i++) {
		name = Cmd_Argv(i);
		
		if ((re_search = IsRegexp(name))) 
			if(!ReSearchInit(name))
				continue;
		
		if (re_search) {
			for (a = cmd_alias ; a ; a=a->next) {
				if (ReSearchMatch(a->name))
					Cmd_DeleteAlias(a->name);
			}
		} else {
			if (!Cmd_DeleteAlias(Cmd_Argv(i)))
				Con_Printf ("unalias: unknown alias \"%s\"\n", Cmd_Argv(i));	
		}

		if (re_search)
			ReSearchDone();
	}
}
// <-- Tonik

void Cmd_RemoveAliases_f(void)
{
	unsigned i;
	cmdalias_t	*a,*n;

	for (a = cmd_alias ; a ; a=n)
	{
		n=a->next;
		Z_Free(a->value);
		Z_Free(a);
	}
	cmd_alias=NULL;

	for (i = 0; i<HT_SIZE ; i++)
		cmd_alias_hash[i] = NULL;
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	char		*cmd_args = NULL;


cmd_function_t	*cmd_hash[HT_SIZE];
cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char *Cmd_Argv (int arg)
{
	if ( arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char *Cmd_Args (void)
{
	if (!cmd_args)
		return "";
	return cmd_args;
}

//Returns a single string containing argv(start) to argv(argc() - 1)
//Unlike Cmd_Args, shrinks spaces between argvs
char *Cmd_MakeArgs (int start) 
{
	int i, c;
	
	static char	text[1024];

	text[0] = 0;
	c = Cmd_Argc();
	for (i = start; i < c; i++) {
		if (i > start)
			strlcat (text, " ", sizeof(text));
		strlcat (text, Cmd_Argv(i), sizeof(text));
	}

	return text;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (char *text)
{
	int		i;
	
// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);
		
	cmd_argc = 0;
	cmd_args = NULL;
	//if(developer.value) Con_Printf("%s\n",text);
	for (;;)
	{
// skip whitespace up to a /n
		while ( *text == ' ' || *text == '\t' || *text == '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;
	
		if (cmd_argc == 1)
			 cmd_args = text;
			
		text = COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc < MAX_ARGS) {
			cmd_argv[cmd_argc] = Z_StrDup(com_token);
			cmd_argc++;
		}
	}
	
}

/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd;
	unsigned h; // BorisU
	
	if (host_initialized)	// because hunk allocation would get stomped
		Sys_Error ("Cmd_AddCommand after host_initialized");
		
// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	
// fail if the command already exists
	if (Cmd_Exists (cmd_name)){
		Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
		return;
	}

	cmd = Hunk_Alloc (sizeof(cmd_function_t));
	cmd->name = cmd_name;
// BorisU --> inserting into hash table
	h = Hash(cmd_name);
	cmd->hash_next = cmd_hash[h];
	cmd_hash[h] = cmd;
// <-- BorisU
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

// BorisU -->
void	Cmd_AddCommandTrig (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd;

	Cmd_AddCommand (cmd_name, function);
	cmd = Cmd_FindCommand (cmd_name);
	if (cmd)
		cmd->flags |= CMD_ALLOWED_IN_TRIGGERS;
}
// <-- BorisU

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (char *cmd_name)
{
	cmd_function_t	*cmd;
	unsigned h;

	h = Hash(cmd_name);
	for (cmd=cmd_hash[h] ; cmd ; cmd=cmd->hash_next)
	if (!Q_stricmp (cmd_name, cmd->name))
		return true;
	
	return false;
}

/*
============
Cmd_FindCommand
============
*/
cmd_function_t *Cmd_FindCommand (char *cmd_name)
{
	cmd_function_t	*cmd;
	unsigned h;

	h = Hash(cmd_name);
	for (cmd=cmd_hash[h] ; cmd ; cmd=cmd->hash_next)
		if (!Q_stricmp (cmd_name, cmd->name))
			return cmd;

	return NULL;
}

/*
============
Cmd_CompleteCommand
============
*/
#define		MAXALTERNATIVES		8
char	*Alternatives[MAXALTERNATIVES];
int		num_alternatives;

extern cvar_t	*cvar_hash[HT_SIZE];

char *Cmd_CompleteCommand (char *partial,int ehelp)
{
	cmd_function_t	*cmd;
	int				len;
	cmdalias_t		*a;
	cvar_t			*cvar;
	unsigned		h;
	int				num=0;

#ifndef	SERVERONLY
	char	*first=NULL;
	int		i,j;
#endif

	if (ehelp > 1) return Alternatives[(ehelp-1)%num_alternatives];

	len = strlen(partial);
	
	if (!len)
		return NULL;
	
	num_alternatives = 1;

// check for exact match 
	h=Hash(partial);
// correct by Sergio -->
	Alternatives[0] = NULL;
	for (cmd=cmd_hash[h] ; cmd && !num ; cmd=cmd->hash_next)
		if (!Q_stricmp(partial,cmd->name)) {
			Alternatives[num++]=cmd->name;
			}

	for (cvar = cvar_hash[h]; cvar && !num; cvar = cvar->hash_next)
		if (!Q_stricmp (partial,cvar->name) && !(cvar->flags & CVAR_INTERNAL)) {
			Alternatives[num++]=cvar->name; 
			}

	for (a = cmd_alias_hash[h]; a && !num ; a = a->hash_next)
		if (!Q_stricmp(partial, a->name)) {
			Alternatives[num++]=a->name;
			}

	if (!ehelp) return Alternatives[0];
// <-- Sergio 

#ifndef SERVERONLY
// check for partial match
	for (cmd=cmd_functions; cmd && num<MAXALTERNATIVES; cmd=cmd->next)
		if (!Q_strnicmp (partial,cmd->name, len) && (!num || Q_stricmp(cmd->name, Alternatives[0])))
			Alternatives[num++]=cmd->name;
	for (cvar=cvar_vars; cvar && num<MAXALTERNATIVES; cvar=cvar->next)
		if (!Q_strnicmp (partial,cvar->name, len) && (!num || Q_stricmp(cvar->name, Alternatives[0])))
			Alternatives[num++]=cvar->name;
	for (a=cmd_alias; a && num<MAXALTERNATIVES; a=a->next)
		if (!Q_strnicmp (partial,a->name, len) && (!num || Q_stricmp(a->name, Alternatives[0])))
			Alternatives[num++]=a->name;

	if (cl_completion_sort.value)
		for(i=0;i<num-1;i++)
			for(j=i+1;j<num;j++)
				if(strlen(Alternatives[j])<strlen(Alternatives[i]))
				{
					first=Alternatives[i];
					Alternatives[i]=Alternatives[j];
					Alternatives[j]=first;
				}

	if(num>1)
	{
		Con_Printf("\x1\x8E\x8E""Choice\x8E\x8E\n");
		for(i=0;i<num;i++)
			Con_Printf(" %s\n",Alternatives[i]);
	}
	if(num) {
		num_alternatives = num;
		return Alternatives[0];
	}
#endif
	
	return NULL;
}

void Cmd_CmdList_f (void)
{
	int i, m, c;
	cmd_function_t	*cmd;
	
	c = Cmd_Argc();
	if (c>1)
		if (!ReSearchInit(Cmd_Argv(1)))
			return;

	Con_Printf ("List of commands:\n");
	for (cmd=cmd_functions, i=m=0 ; cmd ; cmd=cmd->next, i++)
		if (c==1 || ReSearchMatch(cmd->name)) {
			Con_Printf("%s\n", cmd->name);
			m++;
		}
	if (c>1)
		ReSearchDone();

	Con_Printf ("------------\n%i/%i commands\n", m,i);
}

#ifndef SERVERONLY		// FIXME

pcre*		qizmo_cmd_re;
pcre_extra*	qizmo_cmd_re_extra;
void FilterSay(char* s)
{
	int result;
	int offsets[99];

	result = pcre_exec(qizmo_cmd_re, qizmo_cmd_re_extra, s, strlen(s), 0, 0, offsets, 99);
	if (result > 0){
		CharsToBrown(s + offsets[0], s + offsets[1]);
	}

}

float	cmd_time = -9999;
int		cmd_counter;
/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state == ca_disconnected) {
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback) {
		// Tonik:
		if ( ! Q_stricmp(Cmd_Argv(0), "pause"))
			cl.paused ^= 1;
		return;
	}

	if(++cmd_counter >= 30) {
		if (cls.realtime < cmd_time + 5 && !cls.demoplayback)
			return;
		cmd_time = cls.realtime;
		cmd_counter = 0;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, Cmd_Argv(0));

	if (Cmd_Argc() > 1) {
		SZ_Print (&cls.netchan.message, " ");
		if  (!Q_strnicmp(Cmd_Argv(0), "say", 3) ) {
			char		*s;
			s = ParseSay(Cmd_Args());
			if (cl.teamfortress && cls.state == ca_active)
				FilterSay(s);
			if (*s && (*s < 32 || *s > 127)) {
				SZ_Print (&cls.netchan.message, "\"");
				SZ_Print (&cls.netchan.message, s);
				SZ_Print (&cls.netchan.message, "\"");
			} else {
				SZ_Print (&cls.netchan.message, s);
			}
			return;
		}
		
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}

}

#ifdef QW262
#include "cmd262.inc"
#endif

// don't forward the first argument
void Cmd_ForwardToServer_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (cls.demoplayback)
		return;		// not really connected

	if (Q_stricmp(Cmd_Argv(1), "snap") == 0) {
		SCR_RSShot_f();
		//Cbuf_InsertText ("snap\n");
		return;
	}

// bliP -->
		if (Q_stricmp(Cmd_Argv(1), "fileul") == 0) {
			CL_StartFileUpload ();
			return;
		}
// <--

#ifdef QW262
	{
		int i;
		Cmd_ForwardToServer_f262();
	}
#endif

	if (Cmd_Argc() > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}
#else
void Cmd_ForwardToServer (void)
{
}
#endif

#ifndef SERVERONLY

#define MAX_MACROS 64

typedef struct {
	char name[32];
	char *(*func) (void);
} macro_command_t;

static macro_command_t macro_commands[MAX_MACROS];
static int macro_count = 0;

void Cmd_AddMacro(char *s, char *(*f)(void)) {
	if (macro_count == MAX_MACROS)
		Sys_Error("Cmd_AddMacro: macro_count == MAX_MACROS");
	strlcpy(macro_commands[macro_count].name, s, sizeof(macro_commands[macro_count].name));
	macro_commands[macro_count++].func = f;
}

char *Cmd_MacroString (char *s, int *macro_length) {	
	int i;
	macro_command_t	*macro;

	for (i = 0; i < macro_count; i++) {
		macro = &macro_commands[i];
		if (!Q_strnicmp(s, macro->name, strlen(macro->name))) {
			*macro_length = strlen(macro->name);
			return macro->func();
		}
		macro++;
	}
	*macro_length = 0;
	return NULL;
}

int Cmd_MacroCompare (const void *p1, const void *p2) {
	return strcmp((*((macro_command_t **) p1))->name, (*((macro_command_t **) p2))->name);
}

void Cmd_MacroList_f (void) {
	int	i;
	static qboolean sorted = false;
	static macro_command_t *sorted_macros[MAX_MACROS];

	if (!macro_count) {
		Con_Printf("No macros!");
		return;
	}

	if (!sorted) {
		for (i = 0; i < macro_count; i++)
			sorted_macros[i] = &macro_commands[i];
		sorted = true;
		qsort(sorted_macros, macro_count, sizeof (macro_command_t *), Cmd_MacroCompare);
	}

	for (i = 0; i < macro_count; i++)
		Con_Printf ("%c%s\n", Cmd_PrefixChar, sorted_macros[i]->name);

	Con_Printf ("---------\n%d macros\n", macro_count);
}

// BorisU -->
// based on Tonik code
void Cmd_ExpandString (char *data, char *dest, int max_length)
{
	char	c;
	char	buf[256], *str;
	int		i, len = 0;
	cvar_t	*var, *bestvar;
	int		buflen;
	int		quotes = 0;
	int		bestvar_length, macro_length,name_length;

// parse a regular word
	while ( (c = *data) != 0) {
		if (c == '\\' && data[1] == '\"' && cl_stringescape.value) {
			if (len >= max_length - 3) break;
			dest[len++]='\\';
			dest[len++]='\"';
			data+=2;
			continue;
		}
		if (c == '"')
			quotes++;

		if (c == Cmd_PrefixChar && !(quotes&1)) {
			bestvar_length = 0;
			data++;
			// Copy the text after Cmd_PrefixChar to a temp buffer
			i = 0;
			buf[0] = 0;
			buflen = 0;
			bestvar = NULL;
			while (isalpha(c = *data) || isdigit(c) || c == '_') {
				if (c == Cmd_PrefixChar)
					break;
				data++;
				buflen++;
				buf[i++] = c;
				buf[i] = 0;
				if ( (var = Cvar_FindVar(buf)) != NULL ) {
					bestvar = var;
					bestvar_length = i;
				}
				if (i==256) break;
			}
			str = Cmd_MacroString (buf, &macro_length);
			name_length = macro_length;

			if (bestvar && (!str || (bestvar_length > macro_length))) {
				str = bestvar->string;
				name_length = bestvar_length;
			}

			if (str) {
				int str_len = strlen(str);
				// check buffer size
				if (len +  str_len >= max_length-1)
					break;

				strcpy(&dest[len], str);
				len += str_len;
				i = name_length;
				while (buf[i])
					dest[len++] = buf[i++];
			} else {
				// no matching cvar or macro name was found
				dest[len++] = Cmd_PrefixChar;
				if (len + buflen >= max_length-1)
					break;
				strcpy (&dest[len], buf);
				len += strlen(buf);
			}
		} else if (c == '\'' && !(quotes&1) && cl_substsinglequote.value) {
			if (data[1] == '\'') {	// double ' changed to '
				dest[len++] = '\'';
				data += 2;
			} else {				// single ' changed to "
				data++;
				dest[len++] = '\"';
			}
		} else {
			dest[len] = c;
			data++;
			len++;
		}
		if (len >= max_length-1)
			break;
	};
	dest[len] = 0;
}
// <-- BorisU
#endif

extern cmd_function_t	*impulse_cmd;
/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text)
{	
	cmd_function_t	*cmd;
	cmdalias_t		*a;
	unsigned		h; // BorisU
	char			buf[1024],*n,*s;

// BorisU -->
#ifndef SERVERONLY
	char text_exp[1024];
	Cmd_ExpandString (text, text_exp, sizeof(text_exp));
	Cmd_TokenizeString (text_exp);
#else
	Cmd_TokenizeString (text);
#endif
// <-- BorisU	
	
// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

#ifndef SERVERONLY
	if (cbuf_current == &cbuf_svc) {
		if (CL_CheckServerCommand())
			return;
	}
#endif

// check functions
	h = Hash(cmd_argv[0]);
	for (cmd=cmd_hash[h] ; cmd ; cmd=cmd->hash_next) {
		if (!Q_stricmp (cmd_argv[0], cmd->name)) {
#ifndef SERVERONLY
			if ( cbuf_current != &cbuf_trig || 
				(cmd && (cmd->flags & CMD_ALLOWED_IN_TRIGGERS)) ||
				(cmd == impulse_cmd && AllowedImpulse(atoi(Cmd_Argv(1)))) ) {
#endif
				if (cmd->function)
					cmd->function ();
#ifndef SERVERONLY
				else
					Cmd_ForwardToServer ();
			} else if (cmd != impulse_cmd)
				Con_Printf ("Invalid trigger command \"%s\"\n", cmd_argv[0]);
#endif
			return;
		}
	}

// check cvars
	if (Cvar_Command ()) return;

// check alias
	for (a=cmd_alias_hash[h] ; a ; a=a->hash_next) {
		char	*p;

		if (!Q_stricmp (cmd_argv[0], a->name))	{

#ifdef EMBED_TCL
			if (a->flags & ALIAS_TCL)
			{
				TCL_ExecuteAlias (a);
				return;
			}
#endif
			if (a->value[0]=='\0') return; // alias is empty

			if(a->flags & ALIAS_HAS_PARAMETERS) { // %parameters are given in alias definition
				s=a->value;
				buf[0] = '\0';
				do {
					n = strchr(s, '%');
					if(n) {
						if(*++n >= '1' && *n <= '9') {
							n[-1] = 0;
							strlcat(buf, s, sizeof(buf));
							n[-1] = '%';
							// insert numbered parameter
							strlcat(buf,Cmd_Argv(*n-'0'), sizeof(buf));
						} else if (*n == '0') {
							n[-1] = 0;
							strlcat(buf, s, sizeof(buf));
							n[-1] = '%';
							// insert all parameters
							strlcat(buf, Cmd_Args(), sizeof(buf));
						} else if (*n == '%') {
							n[0] = 0;
							strlcat(buf, s, sizeof(buf));
							n[0] = '%';
						} else {
							if (*n) {
								char tmp = n[1];
								n[1] = 0;
								strlcat(buf, s, sizeof(buf));
								n[1] = tmp;
							} else
								strlcat(buf, s, sizeof(buf));
						}
						s=n+1;
					}
				} while(n);
				strlcat(buf, s, sizeof(buf));
				p = buf;
//			} else if (Cmd_Argc() > 1 && 
//						a->name[0] != '+' && a->name[0] != '-') {
//				p = Q_strcpy(buf, a->value);
//				*p++ = ' ';
//				Q_strcpy(p, Cmd_Args());
//				p = buf; 
			} else  // alias has no parameters
				p = a->value;
#ifndef SERVERONLY
			if (cbuf_current == &cbuf_svc) {
				Cbuf_AddText (p);
				Cbuf_AddText ("\n");
			} else
#endif
			{
				Cbuf_InsertText ("\n");
				Cbuf_InsertText (p);
			}

			return;
		}
	}

	if (cl_warncmd.value || developer.value)
		Con_Printf ("Unknown command \"%s\"\n",Cmd_Argv(0));
	
}

/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/
int Cmd_CheckParm (char *parm)
{
	int i;
	
	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! Q_stricmp (parm, Cmd_Argv (i)))
			return i;
			
	return 0;
}

extern cvar_t	*cvar_hash[HT_SIZE];
// added by BorisU
/*
============
Hash_Stat_f

Prints hash table statistics
============
*/
void Hash_Stat_f(void)
{
	unsigned		h, length;
	unsigned		stat[16];
	cvar_t*			cvar;
	cmd_function_t*	cmd;
	cmdalias_t*		alias;

// Cvars
	for(h=0;h<16;h++) stat[h] = 0;
	Con_Printf("cvar hash\n=========\n");
	for(h=0; h<HT_SIZE; h++) {
		length=0;
		Con_Printf("%.3d: ", h);
		for (cvar = cvar_hash[h]; cvar; cvar = cvar->hash_next) {
			length++;
			Con_Printf("(%s)", cvar->name);
		}
		Con_Printf("\n");
		stat[length]++;
	}
	Con_Printf("============\n");
	for(h=0;h<16;h++) {
		if (stat[h])
			Con_Printf("%d: %d (%d)\n", h, stat[h], stat[h]*h);
	}

// Cmds
	for(h=0;h<16;h++) stat[h] = 0;
	Con_Printf("cmd hash\n=========\n");
	for(h=0; h<HT_SIZE; h++) {
		length=0;
		Con_Printf("%.3d: ", h);
		for (cmd = cmd_hash[h]; cmd; cmd = cmd->hash_next) {
			length++;
			Con_Printf("(%s)", cmd->name);
		}
		Con_Printf("\n");
		stat[length]++;
	}
	Con_Printf("============\n");
	for(h=0;h<16;h++) {
		if (stat[h])
			Con_Printf("%d: %d (%d)\n", h, stat[h], stat[h]*h);
	}

// Aliases
	for(h=0;h<16;h++) stat[h] = 0;
	Con_Printf("Alias hash\n=========\n");
	for(h=0; h<HT_SIZE; h++) {
		length=0;
		Con_Printf("%.3d: ", h);
		for (alias = cmd_alias_hash[h]; alias; alias = alias->hash_next) {
			length++;
			Con_Printf("(%s)", alias->name);
		}
		Con_Printf("\n");
		stat[length]++;
	}
	Con_Printf("============\n");
	for(h=0;h<16;h++) {
		if (stat[h])
			Con_Printf("%d: %d (%d)\n", h, stat[h], stat[h]*h);
	}

}


#ifndef SERVERONLY

static qboolean is_numeric (char *c)
{	
	return (*c >= '0' && *c <= '9') ||
		((*c == '-' || *c == '+') && (c[1] == '.' || (c[1]>='0' && c[1]<='9'))) ||
		(*c == '.' && (c[1]>='0' && c[1]<='9'));
}

void Re_Trigger_Copy_Subpatterns (char *s, int* offsets, int num, cvar_t *re_sub);
extern cvar_t re_sub[10];

/*
================
Cmd_If_f

conditional execution
============
*/
void Cmd_If_f (void)
{
	int		i, c;
	char	*op;
	qboolean	result;
	char	buf[2048];

	c = Cmd_Argc ();
	if (c < 5) {
		Con_Printf ("usage: if <expr1> <op> <expr2> <command> [else <command>]\n");
		return;
	}

	op = Cmd_Argv (2);
	if (!strcmp(op, "==") || !strcmp(op, "=") || !strcmp(op, "!=")
		|| !strcmp(op, "<>"))
	{
		if (is_numeric(Cmd_Argv(1)) && is_numeric(Cmd_Argv(3)))
			result = Q_atof(Cmd_Argv(1)) == Q_atof(Cmd_Argv(3));
		else
			result = !strcmp(Cmd_Argv(1), Cmd_Argv(3));

		if (op[0] != '=')
			result = !result;
	}
	else if (!strcmp(op, ">"))
		result = Q_atof(Cmd_Argv(1)) > Q_atof(Cmd_Argv(3));
	else if (!strcmp(op, "<"))
		result = Q_atof(Cmd_Argv(1)) < Q_atof(Cmd_Argv(3));
	else if (!strcmp(op, ">="))
		result = Q_atof(Cmd_Argv(1)) >= Q_atof(Cmd_Argv(3));
	else if (!strcmp(op, "<="))
		result = Q_atof(Cmd_Argv(1)) <= Q_atof(Cmd_Argv(3));
	else if (!strcmp(op, "=~") || !strcmp(op, "!~"))
	{
		pcre*		regexp;
		const char	*error;
		int			error_offset;
		int			rc;
		int			offsets[99];

		regexp = pcre_compile (Cmd_Argv(3), 0, &error, &error_offset, NULL);
		if (!regexp)
		{
			Con_Printf ("Error in regexp: %s\n", error);
			return;
		}
		rc = pcre_exec (regexp, NULL, Cmd_Argv(1), strlen(Cmd_Argv(1)),
						0, 0, offsets, 99);
		if (rc >= 0)
		{
			Re_Trigger_Copy_Subpatterns (Cmd_Argv(1), offsets, min(rc, 10), re_sub);
			result = true;
		}
		else
			result = false;

		if (op[0] != '=')
			result = !result;

		pcre_free (regexp);
	}
	else {
		Con_Printf ("unknown operator: %s\n", op);
		Con_Printf ("valid operators are ==, =, !=, <>, >, <, >=, <=, =~, !~\n");
		return;
	}

	buf[0] = '\0';
	if (result)
	{
		for (i=4; i < c ; i++) {
			if ((i == 4) && !Q_stricmp(Cmd_Argv(i), "then"))
				continue;
			if (!Q_stricmp(Cmd_Argv(i), "else"))
				break;
			if (buf[0])
				strcat (buf, " ");
			strcat (buf, Cmd_Argv(i));
		}
	}
	else
	{
		for (i=4; i < c ; i++) {
			if (!Q_stricmp(Cmd_Argv(i), "else"))
				break;
		}

		if (i == c)
			return;
		
		for (i++ ; i < c ; i++) {
			if (buf[0])
				strcat (buf, " ");
			strcat (buf, Cmd_Argv(i));
		}
	}
	strcat (buf, "\n");
	Cbuf_InsertText (buf);
}


void Cmd_If_Exists_f(void)
{
	int			argc;
	char		*type;
	char		*name;
	qboolean	exists;
	qboolean	iscvar, isalias, istrigger, ishud;

	argc = Cmd_Argc();
	if ( argc < 4 || argc > 5) {
		Con_Printf ("if_exists <type> <name> <cmd1> [<cmd2>] - conditional execution\n");
		return;
	}
	
	type = Cmd_Argv(1);
	name = Cmd_Argv(2);
	if ( ( (iscvar = !strcmp(type, "cvar")) && Cvar_FindVar (name) )			|| 
		 ( (isalias = !strcmp(type, "alias")) && Cmd_FindAlias (name) )			||
		 ( (istrigger = !strcmp(type, "trigger")) && CL_FindReTrigger (name) )	||
		 ( (ishud = !strcmp(type, "hud")) && Hud_FindElement (name) ) ) 
		exists = true;
	else {
		exists = false;
		if (!(iscvar || isalias || istrigger || ishud)) {
			Con_Printf("if_exists: <type> can be cvar, alias, trigger, hud\n");
			return;
		}
	}
	if (exists) {
		Cbuf_InsertText ("\n");
		Cbuf_InsertText (Cmd_Argv(3));
	} else if (argc == 5){
		Cbuf_InsertText ("\n");
		Cbuf_InsertText(Cmd_Argv(4));
	} else
		return;
}

void Z_Stat_f(void)
{
	Z_Stat(mainzone);
}	


static qboolean do_in(char *buf, char *orig, char *str, int options)
{
	if ((options & 2) && strstr(orig, str))
		return false;
	
	if (options & 1) {
		strcat(strcpy(buf, orig),str);
	} else {
		strcat(strcpy(buf, str), orig);
	}
	return true;
}

static qboolean do_out(char *orig, char *str, int options)
{
	char	*p;
	int		len = strlen(str);

	if (!(p=strstr(orig, str)))
		return false;
	
	if (!(options & 1))
		memmove(p, p+len, strlen(p)+1);
	return true;
}

void Cmd_Alias_In_f (void)
{
	cmdalias_t	*alias;
	cvar_t		*var;
	char		buf[1024];
	char		*alias_name;
	int			options;

	if (Cmd_Argc() < 3 || Cmd_Argc() > 4) {
		Con_Printf ("alias_in <alias> <cvar> [<options>]\n");
		return;
	}

	alias_name = Cmd_Argv(1);
	alias = Cmd_FindAlias(alias_name);
	options = atoi(Cmd_Argv(3));

	if (!alias) {
		if ((options & 8)) {
			alias = Cmd_AliasCreate(alias_name);
			if (!alias)
				return;
			alias->value = Z_StrDup("");
		} else {
			Con_Printf ("alias_in: unknown alias \"%s\"\n", alias_name);
			return;
		}
	}

	var = Cvar_FindVar(Cmd_Argv(2));
	if (!var) {
		Con_Printf ("alias_in: unknown cvar \"%s\"\n", Cmd_Argv(2));
		return;
	}

	if (!do_in (buf, alias->value, var->string, options)) {
		if (options & 4)
			Con_Printf ("alias_in: already inserted\n");
		return;
	}

	Z_Free (alias->value);
	alias->value = Z_StrDup(buf);
	if (strchr(buf, '%'))
		alias->flags |= ALIAS_HAS_PARAMETERS; 
}

void Cmd_Alias_Out_f (void)
{
	cmdalias_t	*alias;
	cvar_t		*var;
	int			options;

	if (Cmd_Argc() < 3 || Cmd_Argc() > 4) {
		Con_Printf ("alias_out <alias> <cvar> [options]\n");
		return;
	}

	options = atoi(Cmd_Argv(3));

	alias = Cmd_FindAlias(Cmd_Argv(1));
	if (!alias) {
		Con_Printf ("alias_out: unknown alias \"%s\"\n", Cmd_Argv(1));
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(2));
	if (!var) {
		Con_Printf ("alias_out: unknown cvar \"%s\"\n", Cmd_Argv(2));
		return;
	}

	if (!do_out (alias->value, var->string, options)) {
		if (!(options & 2))
			Con_Printf ("alias_out: not found\n");
		return;
	}
}

void Cmd_Cvar_In_f (void)
{
	cvar_t		*var1;
	cvar_t		*var2;
	char		buf[1024];
	char		*var_name;
	int			options;

	if (Cmd_Argc() < 3 || Cmd_Argc() > 4) {
		Con_Printf ("cvar_in <cvar1> <cvar2> [<options>]\n");
		return;
	}

	var_name = Cmd_Argv(1);
	options = atoi(Cmd_Argv(3));

	var1 = Cvar_FindVar(var_name);
	if (!var1) {
		if ((options & 8)) {
			var1 = Cvar_Create (var_name);
			if (!var1)
				return;
			Cvar_Set (var1, "");
		} else {
			Con_Printf ("cvar_in: unknown cvar \"%s\"\n", var_name);
			return;
		}
	}

	var2 = Cvar_FindVar(Cmd_Argv(2));
	if (!var2) {
		Con_Printf ("cvar_in: unknown cvar \"%s\"\n", Cmd_Argv(2));
		return;
	}

	if (!do_in (buf, var1->string, var2->string, options)) {
		if (options & 4)
			Con_Printf ("cvar_in: already inserted\n");
		return;
	}

	Cvar_Set (var1, buf);
}

void Cmd_Cvar_Out_f (void)
{
	cvar_t		*var1;
	cvar_t		*var2;
	char		buf[1024];
	int			options;

	if (Cmd_Argc()<3 || Cmd_Argc()>4){
		Con_Printf ("cvar_out <cvar1> <cvar2> [<options>]\n");
		return;
	}
	
	options = atoi(Cmd_Argv(3));
	var1 = Cvar_FindVar(Cmd_Argv(1));
	if (!var1) {
		Con_Printf ("cvar_out: unknown cvar \"%s\"\n", Cmd_Argv(1));
		return;
	}

	var2 = Cvar_FindVar(Cmd_Argv(2));
	if (!var2) {
		Con_Printf ("cvar_out: unknown cvar \"%s\"\n", Cmd_Argv(2));
		return;
	}

	strcpy (buf, var1->string);
	if (!do_out (buf, var2->string, options)) {
		if (!(options & 2))
			Con_Printf ("cvar_out: not found\n");
		return;
	}

	Cvar_Set (var1, buf);
}

#endif // SERVERONLY

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommandTrig ("exec",Cmd_Exec_f);
	Cmd_AddCommandTrig ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("viewalias", Cmd_Viewalias_f);
	Cmd_AddCommandTrig ("alias",Cmd_Alias_f);
	Cmd_AddCommandTrig ("unalias",Cmd_UnAlias_f);
	Cmd_AddCommand ("removealiases",Cmd_RemoveAliases_f); // -=MD=-
	Cmd_AddCommand ("cmdlist", Cmd_CmdList_f); // Tonik
	Cmd_AddCommandTrig ("wait", Cmd_Wait_f);
	Cmd_AddCommand ("hash_stat", Hash_Stat_f); // BorisU

#ifndef SERVERONLY
	Cmd_AddCommand ("macrolist", Cmd_MacroList_f);

// BorisU -->
	Cmd_AddCommandTrig ("if", Cmd_If_f);
	Cmd_AddCommandTrig ("if_exists", Cmd_If_Exists_f);
	Cmd_AddCommandTrig ("alias_in", Cmd_Alias_In_f);
	Cmd_AddCommandTrig ("alias_out", Cmd_Alias_Out_f);
	Cmd_AddCommandTrig ("cvar_in", Cmd_Cvar_In_f);
	Cmd_AddCommandTrig ("cvar_out", Cmd_Cvar_Out_f);
	Cmd_AddCommand ("z_stat", Z_Stat_f);

	Cvar_RegisterVariable (&cl_completion_sort);
	Cvar_RegisterVariable (&cl_prefixchar);
// <-- BorisU

	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
	{	
		const char	*error;
		int		error_offset;

		qizmo_cmd_re = pcre_compile("(\\.|,|:)(stuff|gamma)", 0, &error, &error_offset, NULL);
		qizmo_cmd_re_extra = pcre_study(qizmo_cmd_re, 0, &error);
	}
#endif
}


