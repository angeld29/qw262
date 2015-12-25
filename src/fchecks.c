/*

Copyright (C) 2001-2002       A Nourai

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

//#ifdef _WIN32
//#include <windows.h>
//#endif
//
//#include "quakedef.h"
//#include "version.h"
//
//#include "common.h"
//#include "auth.h"
//#include "utils.h"
//#include "fmod.h"
//#include "modules.h"
//#include "rulesets.h"

#include "quakedef.h"

static float f_system_reply_time, f_cmdline_reply_time, f_scripts_reply_time, f_ruleset_reply_time, f_reply_time, f_mod_reply_time, f_version_reply_time, f_skins_reply_time, f_server_reply_time;

extern cvar_t r_fullbrightskins;
extern cvar_t allow_scripts;

//cvar_t allow_f_system  = {"allow_f_system",  "1"};
cvar_t allow_f_cmdline = {"allow_f_cmdline", "1"};

extern char * SYSINFO_GetString(void);

void FChecks_VersionResponse(void) {
	if (Modules_SecurityLoaded())
		Cbuf_AddText (va("say QW262 version (rev.%d %s) " PLATFORM ":" RENDER "  crc: %s\n", MINOR_VERSION, RELEASE, Auth_Generate_Crc()));
	else
		Cbuf_AddText (va("say QW262 version (rev.%d %s) " PLATFORM ":" RENDER "\n", MINOR_VERSION, RELEASE ));
}

void FChecks_FServerResponse (void) {
	netadr_t adr;

	if (!NET_StringToAdr (cls.servername, &adr))
		return;

	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

//	if (Modules_SecurityLoaded())
//		Cbuf_AddText(va("say QW262 f_server response: %s  crc: %s\n", NET_AdrToString(adr), Auth_Generate_Crc()));
//	else
		Cbuf_AddText(va("say QW262 f_server response: %s\n", NET_AdrToString(adr)));
}

void FChecks_SkinsResponse(float fbskins) {
	if (fbskins > 0) {
		Cbuf_AddText (va("say all skins %d%% fullbright\n", (int) (fbskins * 100)));	
	}
	else {
		Cbuf_AddText (va("say not using fullbright skins\n"));	
	}
}

void FChecks_ScriptsResponse (void)
{
	if (allow_scripts.value < 1)
		Cbuf_AddText("say not using rj/fwrj scripts\n");
	else if (allow_scripts.value < 2 || com_blockscripts)
		Cbuf_AddText("say using rj script\n");
	else
		Cbuf_AddText("say using rj/fwrj scripts\n");
}

qboolean FChecks_ScriptsRequest (char *s)
{
	if (cl.spectator || f_scripts_reply_time && cls.realtime - f_scripts_reply_time < 20)	
		return false;

	if (Util_F_Match(s, "f_scripts"))	{
		FChecks_ScriptsResponse();
		f_scripts_reply_time = cls.realtime;
		return true;
	}
	return false;
}


qboolean FChecks_VersionRequest (char *s) {
	if (cl.spectator || (f_version_reply_time && cls.realtime - f_version_reply_time < 20))
		return false;

	if (Util_F_Match(s, "f_version")) {
		FChecks_VersionResponse();
		f_version_reply_time = cls.realtime;
		return true;
	}
	return false;
}

qboolean FChecks_SkinRequest (char *s) {
	float fbskins;		

	fbskins = bound(0, r_fullbrightskins.value, cl.fbskins);	
	if (cl.spectator || f_skins_reply_time && cls.realtime - f_skins_reply_time < 20)	
		return false;

	if (Util_F_Match(s, "f_skins"))	{
		FChecks_SkinsResponse(fbskins);
		f_skins_reply_time = cls.realtime;
		return true;
	}
	return false;
}

qboolean FChecks_CheckFModRequest (char *s) {
	if (cl.spectator || (f_mod_reply_time && cls.realtime - f_mod_reply_time < 20))
		return false;

	if (Util_F_Match(s, "f_modified"))	{
		FMod_Response();
		f_mod_reply_time = cls.realtime;
		return true;
	}
	return false;
}

qboolean FChecks_CheckFServerRequest (char *s) {
	netadr_t adr;

	if (cl.spectator || (f_server_reply_time && cls.realtime - f_server_reply_time < 20))
		return false;

	if (Util_F_Match(s, "f_server") && NET_StringToAdr (cls.servername, &adr))	{
		FChecks_FServerResponse();
		f_server_reply_time = cls.realtime;
		return true;
	}
	return false;
}

//qboolean FChecks_CheckFRulesetRequest (char *s) {
//	if (cl.spectator || (f_ruleset_reply_time && cls.realtime - f_ruleset_reply_time < 20))
//		return false;
//
//	if (Util_F_Match(s, "f_ruleset"))	{
//		Cbuf_AddText(va("say ezQuake Ruleset: %s\n", Rulesets_Ruleset() ));
//		f_ruleset_reply_time = cls.realtime;
//		return true;
//	}
//	return false;
//}

qboolean FChecks_CmdlineRequest (char *s) {
	if (cl.spectator || (f_cmdline_reply_time && cls.realtime - f_cmdline_reply_time < 20))
		return false;

	if (Util_F_Match(s, "f_cmdline"))	{
	    if (!allow_f_cmdline.value) {
			Cbuf_AddText("say disabled\n");
		}
		else {
			Cbuf_AddText("say ");
			Cbuf_AddText(com_args_original);
			Cbuf_AddText("\n");
		}
		f_cmdline_reply_time = cls.realtime;
		return true;
	}
	return false;
}

//qboolean FChecks_SystemRequest (char *s) {
//	if (cl.spectator || (f_system_reply_time && cls.realtime - f_system_reply_time < 20))
//		return false;
//
//	#ifdef _WIN32
//
//	if (Util_F_Match(s, "f_system"))	{
//	    char *sys_string;
//
//		if (allow_f_system.value)
//			sys_string = SYSINFO_GetString();
//		else
//			sys_string = "disabled";
//
//		//if (sys_string != NULL && sys_string[0]) {
//			Cbuf_AddText("say ");
//			Cbuf_AddText(sys_string);
//			Cbuf_AddText("\n");
//		//}
//
//		f_system_reply_time = cls.realtime;
//		return true;
//	}
//	
//	#endif
//	
//	return false;
//}

void FChecks_CheckRequest(char *s)
{
	qboolean fcheck = false;

	fcheck |= FChecks_VersionRequest (s);
	fcheck |= FChecks_CmdlineRequest (s);
//	fcheck |= FChecks_SystemRequest (s);
	fcheck |= FChecks_SkinRequest (s);
	fcheck |= FChecks_ScriptsRequest (s);
	fcheck |= FChecks_CheckFModRequest (s);
	fcheck |= FChecks_CheckFServerRequest (s);
//	fcheck |= FChecks_CheckFRulesetRequest (s);
	if (fcheck)
		f_reply_time = cls.realtime;
}

void FChecks_Init(void) {
//	Cvar_RegisterVariable (&allow_f_system);
	Cvar_RegisterVariable (&allow_f_cmdline);
	FMod_Init();
	Cmd_AddCommand ("f_server", FChecks_FServerResponse);
}
