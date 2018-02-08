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
// cvar.c -- dynamic variable tracking

#ifdef SERVERONLY 
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#include <ctype.h>

// BorisU -->
cvar_t	*cvar_hash[HT_SIZE];

#ifndef SERVERONLY
cvar_t cl_autoregister = {"cl_autoregister", "0"};
#endif
// <-- BorisU

cvar_t	*cvar_vars;
char	*cvar_null_string = "";

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
	unsigned h;

	h = Hash(var_name);
	for (var=cvar_hash[h] ; var ; var=var->hash_next)
		if (!Q_stricmp (var_name, var->name)) 
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
	// check exact match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strcmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


#ifdef SERVERONLY
void SV_SendServerInfoChange(char *key, const char *value);
#endif

/*
============
Cvar_Set
============
*/

// Tonik
void Cvar_Set (cvar_t *var, const char *value)
{
#ifndef SERVERONLY
	char	tmp[40];
#endif
	static qboolean	changing = false;

	if (!var)
		return;

	if (var->flags & CVAR_ROM)
		return;

#ifdef SERVERONLY
	if ((var->flags & CVAR_SERVERCFGCONST) && server_cfg_done)
		return;
#endif

	if (var->OnChange && !changing) {
		changing = true;
		if (var->OnChange(var, value)) {
			changing = false;
			return;
		}
		changing = false;
	}
	
#ifdef SERVERONLY
	if (var->flags & CVAR_SERVERINFO) {
		Info_SetValueForKey (svs.info, var->name, value, MAX_SERVERINFO_STRING);
		SV_SendServerInfoChange(var->name, value);
//		SV_BroadcastCommand ("fullserverinfo \"%s\"\n", svs.info);
	}
#else
	if (var->flags & CVAR_USERINFO)
	{
		Info_SetValueForKey (cls.userinfo, var->name, value, MAX_INFO_STRING);
		if (cls.state >= ca_connected)
		{
			MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
			SZ_Print (&cls.netchan.message, va("setinfo \"%s\" \"%s\"\n", var->name, value));
		}
	}
#endif
	
	if (var->string)
		Z_Free (var->string);	// free the old value string
	
#ifndef SERVERONLY
	if(*value==Cmd_PrefixChar && (value[1] == '-' || value[1] == '+')) {
		var->value+=Q_atof (value[1]=='+' ? value+2:value+1);
		sprintf(tmp,"%.8g",var->value);
		var->string = Z_StrDup(tmp);
	} else
#endif
	{
		var->string = Z_StrDup(value);
		var->value = Q_atof (var->string);
	}
}

/*
============
Cvar_SetROM
============
*/
void Cvar_SetROM (cvar_t *var, const char *value)
{
	unsigned saved_flags;

	if (!var)
		return;

	saved_flags = var->flags;
	var->flags &= ~CVAR_ROM;
	Cvar_Set (var, value);
	var->flags = saved_flags;
}

/*
============
Cvar_SetByName
============
*/
void Cvar_SetByName (char *var_name, char *value)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
	}

	Cvar_Set (var, value);
}

/*
============
Cvar_SetValue
============
*/
void	Cvar_SetValue (cvar_t *var, float value)
{
	char	val[32];
	
	if (fabs(value)<1e-6)
		value = 0;
	if (fabs(value) < 1e6) 
		sprintf (val, "%g", value); 
	else 
		sprintf (val, "%.8g", value); 

	Cvar_Set (var, val);
}

/*
============
Cvar_SetValueByName
============
*/
void Cvar_SetValueByName (char *var_name, float value)
{
	char	val[32];
	
	sprintf (val, "%.8g",value);
	Cvar_SetByName (var_name, val);
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
qboolean Cvar_RegisterVariable (cvar_t *variable)
{
	char	value[512];
	unsigned h;

// first check to see if it has already been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, already defined\n", variable->name);
		return false;
	}
	
// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return false;
	}

// BorisU -->
// check for overlap with an alias 	
	if (Cmd_FindAlias (variable->name)) {
		Con_Printf ("Cvar_RegisterVariable: %s is an alias\n", variable->name);
		return false;
	}
// <-- BorisU

// link the variable in
	h = Hash (variable->name);
	variable->hash_next = cvar_hash[h];
	cvar_hash[h] = variable;
	variable->next = cvar_vars;
	cvar_vars = variable;

#ifdef EMBED_TCL
	TCL_RegisterVariable (variable);
#endif

	if (variable->flags&CVAR_USER_CREATED)
		return true;
	
// copy the value off, because future sets will Z_Free it
	strcpy (value, variable->string);
	variable->string = Z_Malloc (1);	
	
// set it through the function to be consistant
	Cvar_SetROM (variable, value);
	return true;
}

#ifndef SERVERONLY
// BorisU -->
cvar_t* Cvar_Create (char* name)
{
	cvar_t	*new_variable;

	new_variable=Z_Malloc (sizeof(cvar_t));
	new_variable->name=Z_StrDup(name);
	new_variable->string=Z_StrDup("NULL");
	new_variable->flags = CVAR_USER_CREATED;
	if (!Cvar_RegisterVariable(new_variable)) {
		Z_Free (new_variable->string);
		Z_Free (new_variable->name);
		Z_Free (new_variable);
		new_variable = NULL;
	}

	return new_variable;
}

// <-- BorisU
//Added by -=MD=-
qboolean ValidCvarName(char *name)
{
	while (*name) {
		if (isalpha(*name) || isdigit(*name) || *name == '_')
			name++;
		else
			return false;
	}
	
	return true;
}

void Cvar_Register_f(void)
{
	int i;
	char *name;

	if(Cmd_Argc()<2) {
		Con_Printf("Usage: register <name> [<name2>..]\n");
		return;
	}

	for (i=1; i<Cmd_Argc(); i++) {
		name = Cmd_Argv(i);
		if (ValidCvarName(name))
			Cvar_Create (Cmd_Argv(i));
		else
			Con_Printf("register: \"%s\" - invalid name for cvar\n", name);
	}
}

void Cvar_UnRegister_f(void)
{
	cvar_t		*var;
	char		*name;
	int			i;
	qboolean	re_search;


	if(Cmd_Argc()<2) {
		Con_Printf("Usage: unregister <name> [<name2>..]\n");
		return;
	}
	
	for (i=1; i<Cmd_Argc(); i++) {
		name = Cmd_Argv(i);
		
		if ((re_search = IsRegexp(name))) 
			if(!ReSearchInit(name))
				continue;

		if (re_search) {
			for (var = cvar_vars ; var ; var=var->next) {
				if (ReSearchMatch(var->name))
					Cvar_Delete(var->name);
			}
		} else {
			var = Cvar_FindVar(name);
			if (!var) {
				Con_Printf("Can't unregister \"%s\": no such cvar\n", name);
				continue;
			}
			Cvar_Delete(name);
		}
		if (re_search)
			ReSearchDone();
	}
}
#endif

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t	*v;
	int		argc;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;

	argc = Cmd_Argc();
// perform a variable print or set
	if (argc == 1) {
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	if (argc == 2) {
		Cvar_Set (v, Cmd_Argv(1));
	} else {
		char	*value;
		int		i;

		value = Cmd_Args();
		i = strlen (value);

		if (i > 1 && *value == '\"' && value[i-1] == '\"') {
			value[i-1] = '\0';
			value++;
		}
		Cvar_Set (v, value);
	}

	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = var->next)
		if (var->flags & CVAR_ARCHIVE)
			fprintf (f, "%s \"%s\"\n", var->name, var->string);
}

#ifndef SERVERONLY
// Tonik

/*
=============
Cvar_Toggle_f
=============
*/
void Cvar_Toggle_f (void)
{
	cvar_t *var;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("toggle <cvar> : toggle a cvar on/off\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	if (!var)
	{
		Con_Printf ("Unknown variable \"%s\"\n", Cmd_Argv(1));
		return;
	}

	Cvar_Set (var, var->value ? "0" : "1");
}
#endif

/*
===============
Cvar_CvarList_f
===============
List cvars
*/
void Cvar_CvarList_f (void)
{
	cvar_t	*var;
	int i, m, c;

	c = Cmd_Argc();
	if (c>1)
		if (!ReSearchInit(Cmd_Argv(1)))
			return;
	
	Con_Printf ("List of cvars:\n");
	for (var=cvar_vars, i=m=0 ; var ; var=var->next, i++)
		if (c==1 || ReSearchMatch(var->name)) {
			Con_Printf("%c%c%c %s = %s\n",
				var->flags & CVAR_ARCHIVE ? '*' : ' ',
				var->flags & CVAR_USERINFO ? 'u' : ' ',
				var->flags & CVAR_SERVERINFO ? 's' : ' ',
				var->name, var->string); 
			m++;
		}
	if (c>1)
		ReSearchDone();
	Con_Printf ("------------\n%i/%i variables\n", m, i);
}

#ifndef SERVERONLY
/*
===========
Cvar_Delete
===========
returns true if the cvar was found (and deleted)
*/
qboolean Cvar_Delete (char *name)
{
	cvar_t		*var, *prev;
	unsigned	h;

	h = Hash (name);

	prev = NULL;
	for (var = cvar_hash[h] ; var ; var=var->hash_next) {
		if (!Q_stricmp(var->name, name)) {
			if (!(var->flags & CVAR_USER_CREATED)) {
				Con_Printf("Can't unregister internal cvar\n");
				return false;
			}
			// unlink from hash
			if (prev)
				prev->hash_next = var->hash_next;
			else
				cvar_hash[h] = var->hash_next;
			break;
		}
		prev = var;
	}

	if (!var)
		return false;

	prev = NULL;
	for (var = cvar_vars ; var ; var=var->next) {
		if (!Q_stricmp(var->name, name)) {
			// unlink from cvar list
			if (prev)
				prev->next = var->next;
			else
				cvar_vars = var->next;

#ifdef EMBED_TCL
			TCL_UnregisterVariable (name);
#endif
			// free
			Z_Free (var->string);
			Z_Free (var->name);
			Z_Free (var);
			return true;
		}
		prev = var;
	}

	Sys_Error ("Cvar list broken");
	return false;	// shut up compiler
}

void Cvar_Set_f (void)
{
	cvar_t	*var;
	char	*var_name;

	if (Cmd_Argc() != 3) {
		Con_Printf ("usage: set <cvar> <value>\n");
		return;
	}

	var_name = Cmd_Argv (1);
	var = Cvar_FindVar (var_name);

	if (cl_autoregister.value && !var)
		var = Cvar_Create(var_name);

	if (var)
		Cvar_Set (var, Cmd_Argv(2));
	else 
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
}

void Cvar_Set_ex_f (void)
{
	cvar_t	*var;
	char	*var_name;
	char	text_exp[1024];

	if (Cmd_Argc() != 3) {
		Con_Printf ("usage: set_ex <cvar> <value>\n");
		return;
	}

	var_name = Cmd_Argv (1);
	var = Cvar_FindVar (var_name);

	if (cl_autoregister.value && !var)
		var = Cvar_Create(var_name);

	if (var) {
		Cmd_ExpandString (Cmd_Argv(2), text_exp, 1024);
		Cvar_Set (var, ParseSay(text_exp));
	} else 
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
}

void Cvar_Set_Alias_Str_f (void)
{
	cvar_t		*var;
	cmdalias_t	*alias;
	char		*var_name;
	char		*alias_name;
	char		str[1024];
	char		*v,*s;
	if (Cmd_Argc() != 3) {
		Con_Printf ("usage: set_alias_str <cvar> <alias>\n");
		return;
	}

	var_name = Cmd_Argv (1);
	alias_name = Cmd_Argv (2);
	var = Cvar_FindVar (var_name);
	alias = Cmd_FindAlias (alias_name);

	if (cl_autoregister.value && !var)
		var = Cvar_Create(var_name);

	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	} else if (!alias) {
		Con_Printf ("Unknown alias \"%s\"\n", alias_name);
		return;
	} else {
		s = str; v = alias->value;
		while (*v) {
			if (*v == '\"') // " should be escaped
				*s++ = '\\';
			*s++ = *v++;
		}
		*s = '\0'; 
		Cvar_Set (var, str);
	}
		
}

void Cvar_Set_Bind_Str_f (void)
{
	cvar_t		*var;
	int			keynum;
	char		*var_name;
	char		*key_name;
	char		str[1024];
	char		*v,*s;

	if (Cmd_Argc() != 3) {
		Con_Printf ("usage: set_bind_str <cvar> <key>\n");
		return;
	}

	var_name = Cmd_Argv (1);
	key_name = Cmd_Argv (2);
	var = Cvar_FindVar (var_name);
	keynum = Key_StringToKeynum( key_name );

	if (cl_autoregister.value && !var)
		var = Cvar_Create(var_name);

	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	} else if (keynum == -1) {
		Con_Printf ("Unknown key \"%s\"\n", key_name);
		return;
	} else {
		if (keybindings[keynum]){
		s = str; v = keybindings[keynum];
			while (*v) {
				if (*v == '\"') // " should be escaped
					*s++ = '\\';
				*s++ = *v++;
			}
			*s = '\0'; 
			Cvar_Set (var, str);
		} else
			Cvar_Set (var, "");
	}
}

void Cvar_Set_Regexp_f (void)
{
	char	*s, *re;
	char	*var_name;
	cvar_t	*var;
	char	regexp[1024];
	char	text_exp[1024];
	int		c = Cmd_Argc();

	if ( c < 2 || c > 3) {
		Con_Printf ("usage: set_regexp <cvar> [<string>]\n");
		return;
	}

	var_name = Cmd_Argv(1);
	var = Cvar_FindVar (var_name);

	if (cl_autoregister.value && !var && c == 3)
		var = Cvar_Create(var_name);

	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	}

	if (c == 3)
		Cmd_ExpandString (Cmd_Argv(2), text_exp, sizeof(text_exp));
	else
		strcpy(text_exp, var->string);

	s = text_exp;
	re = regexp;

	while (*s) {
		switch (*s) {
		case ' ':
			*re++ = '\\';
			*re++ = 's';
			++s;
			break;

		case '\"':
		case '{':
		case '}':
		case '$':
		case '@': 
		case '\'':
		case ';':
			{
				char minor_digit;
				*re++ = '\\';
				*re++ = 'x';
				*re++ = (*s)/16 + '0';
				minor_digit = (*s)%16;
				*re++ = ( minor_digit < 10) ? minor_digit + '0': minor_digit + 'a' - 10;
				++s;
			}
			break;

		case '|':
		case '-':
		case '+':
		case '.':
		case '*':
		case '?':
		case '^':
		case '(':
		case ')':
		case '[':
		case ']':
		case '\\':
			*re++ = '\\';
		default: // fallthrough!
			*re++ = *s++;
		}
	}
	*re = '\0';
	
	Cvar_Set (var, regexp);
}

void Cvar_Set_Calc_f(void)
{
	cvar_t	*var, *var2;
	char	*var_name;
	char	*op;
	char	*a2, *a3;
	float	num1;
	float	num2;
	float	result;
	int		division_by_zero = 0;
	int		pos, len;
	char	buf[1024];

	var_name = Cmd_Argv (1);
	var = Cvar_FindVar (var_name);
	
	if (cl_autoregister.value && !var)
		var = Cvar_Create(var_name);
	
	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	}
	
	a2 = Cmd_Argv(2);
	a3 = Cmd_Argv(3);
	if (!strcmp (a2, "strlen")) {
		Cvar_SetValue (var, strlen(a3));
		return;
	} else if (!strcmp (a2, "int")) {
		Cvar_SetValue (var, (int)Q_atof(a3));
		return;
	} else if (!strcmp (a2, "substr")) {
		int		var2len;
		var2 = Cvar_FindVar (a3);
		if (!var2) {
			Con_Printf ("Unknown variable \"%s\"\n", a3);
			return;
		}

		pos = atoi(Cmd_Argv(4));
		if (Cmd_Argc() < 6)
			len = 1;
		else
			len = atoi(Cmd_Argv(5));

		if ( len == 0 ) {
			Cvar_Set(var, "");
			return;
		}

		if ( len < 0 || pos < 0) {
			Con_Printf("substr: invalid len or pos\n");
			return;
		}

		var2len = strlen(var2->string);
		if ( var2len < pos) {
			Con_Printf("substr: string length exceeded\n");
			return;
		}

		len = min ( var2len - pos, len );
		strncpy(buf, var2->string+pos, len);
		buf[len] = '\0';
		Cvar_Set(var, buf);
		return;
	} else if (!strcmp (a2, "set_substr")) {
		int		var1len,var2len;
		char	buf[1024];
		var2 = Cvar_FindVar (a3);
		if (!var2) {
			Con_Printf ("Unknown variable \"%s\"\n", a3);
			return;
		}
		var1len = strlen(var->string);
		var2len = strlen(var2->string);
		pos = atoi(Cmd_Argv(4));
		
		if (pos + var2len > sizeof(buf)-1) {
			Con_Printf ("set_substr length overflow\n");
			return;
		}

		strcpy(buf,var->string);
		if (pos + var2len > var1len){ // need to expand
			int i;
			for (i = var1len; i < pos+var2len; i++)
				buf[i] = ' ';
			buf[pos+var2len] = 0;
		}
		
		strncpy(buf+pos, var2->string, var2len);
		Cvar_Set(var, buf);
		return;
	} else if (!strcmp (a2, "pos")) {
		var2 = Cvar_FindVar (a3);
		if (!var2) {
			Con_Printf ("Unknown variable \"%s\"\n", a3);
			return;
		}
		op = strstr (var2->string, Cmd_Argv(4));
		if (op)
			Cvar_SetValue (var, op - var2->string);
		else
			Cvar_SetValue (var, -1);
		return;
	} else if (!strcmp (a2, "tobrown")) {
		strcpy(buf, var->string);
		CharsToBrown(buf, buf + strlen(buf));
		Cvar_Set(var, buf);
		return;
	} else if (!strcmp (a2, "towhite")) {
		strcpy(buf, var->string);
		CharsToWhite(buf, buf + strlen(buf));
		Cvar_Set(var, buf);
		return;
	}

	num1 = Q_atof(a2);
	op = a3;
	num2 = Q_atof(Cmd_Argv(4));

//	Con_Printf("n1=%g op=%s n2=%g\n", num1, op, num2);

	if (!strcmp(op, "+"))
		result = num1 + num2;
	else if (!strcmp(op, "-"))
		result = num1 - num2;
	else if (!strcmp(op, "*"))
		result = num1 * num2;
	else if (!strcmp(op, "/")) {
		if (num2 != 0)
			result = num1 / num2;
		else 
			division_by_zero = 1;
	} else if (!strcmp(op, "%")) {
		if ((int)num2 != 0)
			result = (int)num1 % (int)num2;
		else 
			division_by_zero = 1;
	} else if (!strcmp(op, "div")) {
		if ((int)num2 != 0)
			result = (int)num1 / (int)num2;
		else 
			division_by_zero = 1;
	} else if (!strcmp(op, "and")) {
		result = (int)num1 & (int)num2;
	} else if (!strcmp(op, "or")) {
		result = (int)num1 | (int)num2;
	} else if (!strcmp(op, "xor")) {
		result = (int)num1 ^ (int)num2;
	}else {
		Con_Printf("set_calc: unknown operator: %s\nvalid operators are: + - * / div %% and or xor\n", op);
		return;
	}
	
	if (division_by_zero) {
		Con_Printf("set_calc: can't divide by zero\n");
		result = 999;
	}

	Cvar_SetValue (var, result);
}

void Cvar_Inc_f (void)
{
	int		c;
	cvar_t	*var;
	float	delta;

	c = Cmd_Argc();
	if (c != 2 && c != 3) {
		Con_Printf ("inc <cvar> [value]\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));

	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", Cmd_Argv(1));
		return;
	}

	if (c == 3)
		delta = atof (Cmd_Argv(2));
	else
		delta = 1;

	Cvar_SetValue (var, var->value + delta);
}
void Cvar_Crypt_f (void)
{
	cvar_t	*var;
	char	*p;
	char	*k;

	if (Cmd_Argc() != 3) {
		Con_Printf ("crypt <cvar> <key>\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	
	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", Cmd_Argv(1));
		return;
	}
	
	p = var->string;
	k = Cmd_Argv(2);

	while (*p){
		*p = *p ^ (*k|128);
		p++;
		k++;
		if (!*k)
			k = Cmd_Argv(2);
	}
}
#endif

void Cvar_Init (void)
{
	Cmd_AddCommand ("cvarlist", Cvar_CvarList_f);

#ifndef SERVERONLY
	Cvar_RegisterVariable (&cl_autoregister);

	Cmd_AddCommandTrig ("register", Cvar_Register_f);
	Cmd_AddCommandTrig ("unregister", Cvar_UnRegister_f);
	
	Cmd_AddCommandTrig ("toggle", Cvar_Toggle_f);
	Cmd_AddCommandTrig ("set", Cvar_Set_f);
	Cmd_AddCommandTrig ("set_ex", Cvar_Set_ex_f);
	Cmd_AddCommandTrig ("set_alias_str", Cvar_Set_Alias_Str_f);
	Cmd_AddCommandTrig ("set_bind_str", Cvar_Set_Bind_Str_f);
	Cmd_AddCommandTrig ("set_regexp", Cvar_Set_Regexp_f);
	Cmd_AddCommandTrig ("set_calc", Cvar_Set_Calc_f);
	Cmd_AddCommandTrig ("inc", Cvar_Inc_f);
	Cmd_AddCommandTrig ("crypt", Cvar_Crypt_f);
#endif
}

