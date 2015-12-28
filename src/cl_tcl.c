/*
 *  QuakeWorld embedded Tcl interpreter
 *
 *  Copyright (C) 2005 Se7en
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id: cl_tcl.c,v 1.7 2006/01/14 16:16:02 angel Exp $
 */

#include <tcl.h>
#include "quakedef.h"

extern cmd_function_t	*impulse_cmd;
extern cmdalias_t		*cmd_alias;
extern cmdalias_t		*cmd_alias_hash[HT_SIZE];

cvar_t tcl_version = {"tcl_version", "", CVAR_ROM|CVAR_INTERNAL};

#ifdef USE_TCL_STUBS
static DL_t			TCL_lib;
#endif
static Tcl_Interp*	interp;
static Tcl_Encoding	qw_enc;

static Tcl_Obj* TCL_QStringToObj (const char* src);

static Tcl_Obj* TCL_QStringToObj (const char* src);

/*
=============================================================================
 Tcl-to-QuakeWorld commands
=============================================================================
*/

static int
TCL_Alias (ClientData data, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[])
{
	Tcl_CmdInfo info;
	int			rc;
	cmdalias_t	*a;
	const char	*name, *s;
	unsigned	h;

	if (objc != 4)
	{
		Tcl_WrongNumArgs (interp, 1, objv, "name args body");
		return (TCL_ERROR);
	}
	name = Tcl_GetString (objv[1]);

	if (strlen (name) >= MAX_ALIAS_NAME)
	{
		Tcl_SetResult (interp, "alias: name is too long", TCL_STATIC);
		return (TCL_ERROR);
	}

	if (Cvar_FindVar (name))
	{
		Tcl_SetResult (interp, "alias: unable to realias cvar", TCL_STATIC);
		return (TCL_ERROR);
	}

	if (Cmd_FindCommand (name))
	{
		Tcl_SetResult (interp, "alias: unable to realias command", TCL_STATIC);
		return (TCL_ERROR);
	}

	rc = Tcl_GetCommandInfo (interp, "proc", &info);
	if (!rc)
	{
		Tcl_SetResult (interp, "alias: unable to create Tcl proc", TCL_STATIC);
		return (TCL_ERROR);
	}

	if (info.isNativeObjectProc)
	{
		rc = info.objProc (info.objClientData, interp, objc, objv);
	}
	else
	{
		char* argv[5];

		argv[0] = "proc";
		argv[1] = Tcl_GetString (objv[1]); // name
		argv[2] = Tcl_GetString (objv[2]); // args
		argv[3] = Tcl_GetString (objv[3]); // body
		argv[4] = NULL;

		rc = info.proc (info.clientData, interp, 4, argv);
	}

	if (rc != TCL_OK)
		return (rc);

	if (!Tcl_GetCommandInfo (interp, name, &info)
		|| !info.isNativeObjectProc)
	{
		Tcl_SetResult (interp, "alias: unable to get reference to Tcl proc", TCL_STATIC);
		return (TCL_ERROR);
	}

	h = Hash (name);
	s = name;
	// if the alias already exists, reuse it
	for (a = cmd_alias_hash[h] ; a ; a=a->hash_next) {
		if (!Q_stricmp(s, a->name)) {
			Z_Free (a->value);
			a->flags = 0;
			break;
		}
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

	a->flags = ALIAS_TCL;

	if (strlen (Tcl_GetString (objv[2])))
		a->flags |= ALIAS_HAS_PARAMETERS;
	
	a->value = Z_Malloc (sizeof (Tcl_CmdInfo));
	memcpy (a->value, &info, sizeof (Tcl_CmdInfo));

	Tcl_SetResult (interp, NULL, TCL_STATIC);

	return (TCL_OK);
}

static int
TCL_Cmd (ClientData data, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj		*args;
	Tcl_DString	str_byte;
	char		*str_utf, *line, *argv;
	size_t		str_utf_len;
	int			i;
	cmd_function_t *cmd;


	if (objc < 2)
	{
		Tcl_WrongNumArgs (interp, 1, objv, "qw_command");
		return (TCL_ERROR);
	}

	args = Tcl_DuplicateObj (objv[1]);
	for (i = 2; i < objc; ++i)
	{
		Tcl_AppendToObj (args, " ", 1);
		argv = Tcl_GetString (objv[i]);
		if (strchr (argv, ' ') && !strchr (argv, '"'))
		{
			// string contains spaces, so quote it
			Tcl_AppendToObj (args, "\"", 1);
			Tcl_AppendObjToObj (args, objv[i]);
			Tcl_AppendToObj (args, "\"", 1);
		}
		else
			Tcl_AppendObjToObj (args, objv[i]);
	}
	str_utf = Tcl_GetStringFromObj (args, &str_utf_len);
	Tcl_UtfToExternalDString (qw_enc, str_utf, str_utf_len, &str_byte);
	line = Tcl_DStringValue (&str_byte);
	Cmd_TokenizeString (line);

	cmd = Cmd_FindCommand (Cmd_Argv(0));
	if (!cmd)
	{
		Tcl_DStringFree (&str_byte);
		Tcl_SetResult (interp, "cmd: unknown command", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (cbuf_current == &cbuf_trig &&
		!((cmd->flags & CMD_ALLOWED_IN_TRIGGERS) ||
		  (cmd == impulse_cmd && AllowedImpulse (atoi(Cmd_Argv(1))))))
	{
		Tcl_DStringFree (&str_byte);
		Tcl_SetResult (interp, "cmd: invalid trigger command", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (cmd->function)
		cmd->function ();
	else
		Cmd_ForwardToServer ();

	Tcl_DStringFree (&str_byte);
	Tcl_SetResult (interp, NULL, TCL_STATIC);

	return (TCL_OK);
}

static int
TCL_DenyProc (ClientData data, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[])
{
	char result[80];
	const char* command = Tcl_GetString (objv[0]);

	Q_snprintfz (result, 80, "Tcl command \"%s\" not allowed in Quakeworld", command);
	Tcl_SetResult (interp, result, TCL_VOLATILE);
	return (TCL_ERROR);
}

static int
TCL_OpenProc (ClientData data, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[])
{
	Tcl_CmdInfo *info = (Tcl_CmdInfo*) data;

	if (!info)
	{
		Tcl_SetResult (interp, "open: invalid builtin procedure", TCL_STATIC);
		return (TCL_ERROR);
	}

	if (objc > 1)
	{
		char* filename = Tcl_GetString (objv[1]);
		if (filename[0] == '|')
		{
			Tcl_SetResult (interp, "open: pipelines not allowed in Quakeworld", TCL_STATIC);
			return (TCL_ERROR);
		}
	}

	return (info->objProc (info->objClientData, interp, objc, objv));
}

static int
TCL_ClockProc (ClientData data, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[])
{
	Tcl_CmdInfo *info = (Tcl_CmdInfo*) data;

	if (!info)
	{
		Tcl_SetResult (interp, "clock: invalid builtin procedure", TCL_STATIC);
		return (TCL_ERROR);
	}

	if ((cl.fpd & FPD_NO_TIMERS) && objc > 1)
	{
		char* param = Tcl_GetString (objv[1]);
		if (param[0] == 's' && param[1] == 'e')
		{
			Tcl_Obj		*result;
			int			rc;
			time_t		clock;
			
			rc = info->objProc (info->objClientData, interp, objc, objv);

			if (rc != TCL_OK) return (rc);
			result = Tcl_GetObjResult (interp);
			if (TCL_OK != Tcl_GetLongFromObj (NULL, result, &clock))
				return (rc);

			clock -= clock % 60; // round away seconds
			Tcl_SetLongObj (result, clock);
			Tcl_SetObjResult (interp, result);

			return (TCL_OK);
		}
		else if (param[0] == 'c')
		{
			Tcl_SetResult (interp, "clock clicks disabled by FPD", TCL_STATIC);
			return (TCL_ERROR);
		}
	}

	return (info->objProc (info->objClientData, interp, objc, objv));
}

/*
=============================================================================
 QuakeWorld-to-Tcl commands
=============================================================================
*/

static void
TCL_Eval_f (void)
{
	int			rc;
	const char	*result;
	Tcl_Obj		*script;

	if (cbuf_current == &cbuf_svc)
	{
		Con_Printf ("%s: server not allowed to exec Tcl commands\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("%s <text> : evaluate text as Tcl script\n", Cmd_Argv(0));
		return;
	}
	script = TCL_QStringToObj (Cmd_Argv(1));

	rc = Tcl_EvalObjEx (interp, script, TCL_EVAL_GLOBAL);
	result = Tcl_GetStringResult (interp);
	if (rc != TCL_OK)
	{
		Con_Printf ("Error in Tcl script: %s\n", result ? result : "unknown");
		return;
	}
	if (result && *result)
	{
		Tcl_DString dst;
		char *value;

		value = Tcl_UtfToExternalDString (qw_enc, result, strlen (result), &dst);
		Con_Printf ("%s\n", value);
		Tcl_DStringFree (&dst);
	}
}

static void
TCL_Exec_f (void)
{
	int		rc;
	int		mark;
	char	filename[MAX_OSPATH];
	char	*eval_buf;
	Tcl_Obj	*script;
	const char *result;

	if (cbuf_current == &cbuf_svc)
	{
		Con_Printf ("%s: server not allowed to exec Tcl commands\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("%s <filename> : execute file as Tcl script\n", Cmd_Argv(0));
		return;
	}
	strcpy (filename, Cmd_Argv(1));
	COM_DefaultExtension (filename, ".tcl");

	mark = Hunk_LowMark ();
	eval_buf = (char *)COM_LoadHunkFile (filename);
	if (!eval_buf)
	{
		Con_Printf ("%s: unable to load %s\n", Cmd_Argv(0), filename);
		return;
	}

	script = TCL_QStringToObj (eval_buf);
	Hunk_FreeToLowMark (mark);

	rc = Tcl_EvalObjEx (interp, script, TCL_EVAL_GLOBAL);
	if (rc != TCL_OK && rc != TCL_RETURN)
	{
		if (developer.value)
			result = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
		else
			result = Tcl_GetStringResult (interp);
		Con_Printf ("Error in Tcl script: %s\n", result ? result : "unknown");
		return;
	}
}

static void
TCL_Proc_f (void)
{
	int			i;
	int			argc;
	Tcl_CmdInfo info;
	int			rc;
	const char	*result;

	if (cbuf_current == &cbuf_svc)
	{
		Con_Printf ("%s: server not allowed to exec Tcl commands\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("tcl_proc <name> [args] : execute Tcl procedure <name>\n");
		return;
	}
	rc = Tcl_GetCommandInfo (interp, Cmd_Argv(1), &info);
	if (!rc)
	{
		Con_Printf ("tcl_proc: procedure %s not found\n", Cmd_Argv(1));
		return;
	}

	argc = min (Cmd_Argc()-1, 15); // FIXME: only 16 arguments accepted

	if (info.isNativeObjectProc)
	{
		struct Tcl_Obj* objv[17];
		for (i = 0; i < argc; ++i)
			objv[i] = TCL_QStringToObj (Cmd_Argv(i+1));
		objv[i] = NULL;

		rc = info.objProc (info.objClientData, interp, argc, objv);
	}
	else
	{
		Con_Printf ("tcl_proc: invalid Tcl proc %s\n", Cmd_Argv(1));
		return;
		/*
		char* argv[17];

		for (i = 0; i < argc; ++i)
			argv[i] = Cmd_Argv(i+1);
		argv[i] = NULL;

		rc = info.proc (info.clientData, interp, argc, argv);
		*/
	}

	if (rc != TCL_OK)
	{
		result = Tcl_GetStringResult (interp);
		Con_Printf ("Tcl error: %s\n", result ? result : "unknown");
		return;
	}
}

/*
=============================================================================
 Tcl internals
=============================================================================
*/

static Tcl_Obj*
TCL_QStringToObj (const char* src)
{
	Tcl_Obj*	obj;
	Tcl_DString str_utf;

	Tcl_ExternalToUtfDString (qw_enc, src, strlen (src), &str_utf);
	obj = Tcl_NewStringObj (Tcl_DStringValue (&str_utf), Tcl_DStringLength (&str_utf));
	Tcl_DStringFree (&str_utf);

	return (obj);
}

static char*
TCL_TraceVariable (ClientData data, Tcl_Interp* interp, const char* name1,
				   const char* name2, int flags)
{
	cvar_t	*var;
	char	*value;

	if (name1[0] == ':' && name1[1] == ':')
		name1 += 2; // skip root namespace specification

	var = Cvar_FindVar (name1);

	if (flags & TCL_TRACE_READS)
	{
		if (var)
		{
			Tcl_DString str_utf;
			value = var->string;
			Tcl_ExternalToUtfDString (qw_enc, value, strlen (value), &str_utf);
			Tcl_SetVar2 (interp, name1, name2, Tcl_DStringValue (&str_utf), TCL_GLOBAL_ONLY);
			Tcl_DStringFree (&str_utf);
		}
		else
		{
			// QW variable no longer exists
			Tcl_UntraceVar2 (interp, name1, name2,
							 TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
 							 TCL_TraceVariable, NULL);
		}
	}
	else if (flags & TCL_TRACE_WRITES)
	{
		if (var)
		{
			const char *value;
			Tcl_DString str_byte;
			value = Tcl_GetVar2 (interp, name1, name2, TCL_GLOBAL_ONLY);
			Tcl_UtfToExternalDString (qw_enc, value, strlen (value), &str_byte);
			Cvar_Set (var, Tcl_DStringValue (&str_byte));
			Tcl_DStringFree (&str_byte);
		}
		else
		{
			// QW variable no longer exists
			Tcl_UntraceVar2 (interp, name1, name2,
							 TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
 							 TCL_TraceVariable, NULL);
		}
	}
	else if ((flags & TCL_TRACE_UNSETS) && !(flags && TCL_INTERP_DESTROYED))
	{
		if (var && !Cvar_Delete (name1))
		{
			// cvar cannot be unset, restore variable
			Tcl_DString str_utf;
			char *value = var->string;
			Tcl_ExternalToUtfDString (qw_enc, value, strlen (value), &str_utf);
			Tcl_SetVar2 (interp, name1, name2, Tcl_DStringValue (&str_utf), TCL_GLOBAL_ONLY);
			Tcl_TraceVar (interp, name1,
						  TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
						  TCL_TraceVariable, NULL);
			Tcl_DStringFree (&str_utf);
		}
	}
	return NULL;
}

void
TCL_RegisterVariable (cvar_t *variable)
{
	int result;
	if (!interp || (variable->flags & CVAR_INTERNAL))
		return;

	Tcl_SetVar (interp, variable->name, variable->string, TCL_GLOBAL_ONLY);
	result = Tcl_TraceVar (interp, variable->name,
		 				   TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
		 				   TCL_TraceVariable, NULL);
}

void
TCL_UnregisterVariable (const char *name)
{
	if (!interp) return;

	Tcl_UntraceVar (interp, name,
  					TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS|TCL_GLOBAL_ONLY,
  					TCL_TraceVariable, NULL);
	Tcl_UnsetVar (interp, name, TCL_GLOBAL_ONLY);
}

void
TCL_ExecuteAlias (cmdalias_t *alias)
{
	Tcl_CmdInfo *info;
	Tcl_Obj*	objv[11];
	int			i, argc, rc;

	if (!interp) return;

	info = (Tcl_CmdInfo*) alias->value;

	objv[0] = Tcl_NewStringObj (alias->name, strlen (alias->name));

	if (alias->flags & ALIAS_HAS_PARAMETERS)
	{
		argc = min (Cmd_Argc(), 9);
		if (alias->name[0] == '+' || alias->name[0] == '-')
			--argc; // ignore key code
		for (i = 1; i < argc; ++i)
			objv[i] = TCL_QStringToObj (Cmd_Argv(i));
		objv[i] = NULL;
	}
	else
	{
		argc = 1;
		objv[1] = NULL;
	}

	rc = info->objProc (info->objClientData, interp, argc, objv);

	if (rc != TCL_OK)
	{
		const char *result = Tcl_GetStringResult (interp);
		Con_Printf ("Tcl alias error: %s\n", result);
		return;
	}
}

static void
TCL_DeleteProc (ClientData data)
{
	Tcl_Free (data);
}

static void
TCL_ReplaceProc (const char* command, Tcl_ObjCmdProc* proc)
{
	Tcl_CmdInfo info, *info_ptr;
	int			rc;

	rc = Tcl_GetCommandInfo (interp, command, &info);
	if (!rc || !info.isNativeObjectProc)
	{
		Tcl_CreateObjCommand (interp, command, proc, NULL, NULL);
		return;
	}
	info_ptr = (Tcl_CmdInfo*) Tcl_Alloc (sizeof (info));
	memcpy (info_ptr, &info, sizeof (info));
	Tcl_CreateObjCommand (interp, command, proc, info_ptr, TCL_DeleteProc);
}

static int
TCL_RawToUtfChars (ClientData client_data, const char *src, int src_len, int flags, 
			   	   Tcl_EncodingState *state_ptr, char *dst, int dst_len, int *src_read_ptr,
			   	   int *dst_wrote_ptr, int *dst_chars_ptr)
{
	char	buf[TCL_UTF_MAX];
	int		utf_len = 0;
	int		src_read = 0, dst_wrote = 0, dst_chars = 0;

	while (src_len && dst_len)
	{
		utf_len = Tcl_UniCharToUtf ((unsigned char)*src, buf);
		if (utf_len > dst_len)
			break;
		memcpy (dst, buf, utf_len);
		dst += utf_len;
		dst_len -= utf_len;
		++src;
		--src_len;
		++src_read;
		dst_wrote += utf_len;
		++dst_chars;
	}
	*src_read_ptr = src_read;
	*dst_wrote_ptr = dst_wrote;
	*dst_chars_ptr = dst_chars;
	if (src_len)
		return (TCL_CONVERT_NOSPACE);
	else
		return (TCL_OK);
}

static int
TCL_UtfToRawChars (ClientData client_data, const char *src, int src_len, int flags, 
			   	   Tcl_EncodingState *state_ptr, char *dst, int dst_len, int *src_read_ptr,
			   	   int *dst_wrote_ptr, int *dst_chars_ptr)
{
	Tcl_UniChar	uni_char;
	int			utf_len;
	int			src_read = 0, dst_wrote = 0, dst_chars = 0;

	while (src_len && dst_len)
	{
		utf_len = Tcl_UtfToUniChar (src, &uni_char);
		if (utf_len > src_len)
			break;
		*dst++ = (char) (uni_char & 0xff); // just strip off hi byte
		src += utf_len;
		src_len -= utf_len;
		src_read += utf_len;
		--dst_len;
		++dst_wrote;
		++dst_chars;
	}
	*src_read_ptr = src_read;
	*dst_wrote_ptr = dst_wrote;
	*dst_chars_ptr = dst_chars;

	if (src_len)
	{
		if (dst_len)
			return (TCL_CONVERT_MULTIBYTE);
		else
			return (TCL_CONVERT_NOSPACE);
	}
	else
		return (TCL_OK);
}

void
TCL_InterpInit (void)
{
	const char *version;
	static Tcl_EncodingType raw_enc = {
		"raw",				// encodingName
		TCL_RawToUtfChars,	// toUtfProc
		TCL_UtfToRawChars,	// fromUtfProc
		NULL,				// freeProc
		NULL,				// clientData
		1					// nullSize
	};

#ifdef USE_TCL_STUBS
	Tcl_Interp*	(*Tcl_CreateInterpP) (void);
	void		(*Tcl_FindExecutableP) (const char*);

	TCL_lib = Sys_DLOpen (TCL_LIB_NAME);
	if (!TCL_lib)
		return;

	Tcl_CreateInterpP = Sys_DLProc (TCL_lib, "Tcl_CreateInterp");
	Tcl_FindExecutableP = Sys_DLProc (TCL_lib, "Tcl_FindExecutable");

	if (!Tcl_CreateInterpP || !Tcl_FindExecutableP)
		Sys_Error ("Tcl initialization failed, probably incompatible "TCL_LIB_NAME" version\n");

	Tcl_FindExecutableP ("");
	interp = Tcl_CreateInterpP ();
#else
	Tcl_FindExecutable ("");
	interp = Tcl_CreateInterp ();
#endif

	if (!interp)
		Sys_Error ("Failed to create Tcl interpreter\n");

	version = Tcl_InitStubs (interp, TCL_VERSION, 0);
	if (!version)
		Sys_Error ("Incompatible Tcl version: %s required\n", TCL_VERSION);

	qw_enc = Tcl_CreateEncoding (&raw_enc);
	if (!qw_enc)
		Sys_Error ("Unable to initialize Tcl UTF-8 encoding\n");

	Cvar_RegisterVariable (&tcl_version);
	Cvar_SetROM (&tcl_version, version);

	Tcl_CreateObjCommand (interp, "cmd", TCL_Cmd, NULL, NULL);
	Tcl_CreateObjCommand (interp, "alias", TCL_Alias, NULL, NULL);

	Cmd_AddCommandTrig ("tcl_exec", TCL_Exec_f);
	Cmd_AddCommandTrig ("tcl_eval", TCL_Eval_f);
	Cmd_AddCommandTrig ("tcl_proc", TCL_Proc_f);

	TCL_ReplaceProc ("exec", TCL_DenyProc);
	TCL_ReplaceProc ("load", TCL_DenyProc);
	TCL_ReplaceProc ("exit", TCL_DenyProc);
	TCL_ReplaceProc ("interp", TCL_DenyProc);
	TCL_ReplaceProc ("open", TCL_OpenProc);
	TCL_ReplaceProc ("clock", TCL_ClockProc);
	
}

qboolean
TCL_InterpLoaded (void)
{
	return (interp != NULL);
}

void
TCL_Shutdown (void)
{
	if (interp)
	{
		Tcl_DeleteInterp (interp);
#ifdef USE_TCL_STUBS
		Sys_DLClose (TCL_lib);
#endif
	}
}
