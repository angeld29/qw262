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

#include "quakedef.h"
#include "modules.h"
#include "version.h"

typedef struct registeredModule_s {
	qlib_id_t id;
	qboolean loaded;
	qlib_shutdown_fn shutdown;
} registeredModule_t;

char _temp_modulename[MAX_OSPATH];
static registeredModule_t registeredModules[qlib_nummodules];

#ifdef __linux
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define SECURITY_GETFUNC(f) (Security_##f = (Security_##f##_t) GetProcAddress(hSecurity, "Security_" #f))
#else
#define SECURITY_GETFUNC(f) (Security_##f = (Security_##f##_t) dlsym(hSecurity, "Security_" #f))
#endif

Security_Init_t	Security_Init;
Security_Shutdown_t Security_Shutdown;
Security_Supported_Binaries_t Security_Supported_Binaries;

Security_Verify_Response_t Security_Verify_Response;
Security_Generate_Crc_t Security_Generate_Crc;
Security_IsModelModified_t Security_IsModelModified;

static qboolean security_loaded;

qboolean Modules_SecurityLoaded(void) {
	return security_loaded;
}

qboolean VerifyData(signed_buffer_t *p) {
	return p ? true : false;
}

#ifdef _WIN32
static HINSTANCE hSecurity = NULL;
#else
static void *hSecurity = NULL;
#endif

void Modules_Init(void) {
	qboolean have_security;
	char *version_string, binary_type[32], *renderer;
	int retval;

	have_security = security_loaded = false;

#ifdef _WIN32
	if (!(hSecurity = LoadLibrary("qw-security.dll"))) {
		Con_Printf("\x02" "Security module not found\n");
		goto fail;
	}
#else
	if (!(hSecurity = dlopen("./qw-security.so", RTLD_NOW))) {
		Con_Printf("\x02" "Security module not found\n");
		goto fail;
	}
#endif
	have_security = true;

	SECURITY_GETFUNC(Init);
	SECURITY_GETFUNC(Verify_Response);
	SECURITY_GETFUNC(Generate_Crc);
	SECURITY_GETFUNC(IsModelModified);
	SECURITY_GETFUNC(Supported_Binaries);
	SECURITY_GETFUNC(Shutdown);

	security_loaded	=	Security_Verify_Response && 
						Security_Generate_Crc && 
						Security_IsModelModified &&
						Security_Supported_Binaries &&
						Security_Init &&
						Security_Shutdown;

	if (!security_loaded) {
		Con_Printf("\x02" "Security module not initialized\n");
		goto fail;
	}

	strlcpy(binary_type, "qw262cl", sizeof(binary_type));

	renderer = RENDER;
	version_string = va ("%s-p:%s-r:%s-b:%d", binary_type, PLATFORM, renderer, RELEASE);

	retval = Security_Init(version_string);
	security_loaded = (retval == 0) ? true : false;
	Con_Printf("buffer: %s\n", retval);

	if (!security_loaded) {
		switch (retval) {
			case SECURITY_INIT_ERROR:
				Con_Printf("\x02" "Error initializing security module\n"); break;
			case SECURITY_INIT_BAD_VERSION:
				Con_Printf("\x02" "Incompatible client version\n"); break;
			case SECURITY_INIT_BAD_CHECKSUM:
				Con_Printf("\x02" "Modified client binary detected\n"); break;
			case SECURITY_INIT_NOPROC:
				Con_Printf("\x02" "Proc filesystem not found\n"); break;
		}
		Con_Printf("\x02" "Security module not initialized\n");
		goto fail;
	}

	Con_Printf("Security module initialized\n");
	return;

fail:
	security_loaded = false;
	if (hSecurity) {
		#ifdef _WIN32
		FreeLibrary(hSecurity);
		#else
		dlclose(hSecurity);
		#endif
		hSecurity = NULL;
	}
}

void Modules_Shutdown(void) {
	if (Security_Shutdown && Modules_SecurityLoaded())
		Security_Shutdown();
	if (hSecurity) {
		#ifdef _WIN32
		FreeLibrary(hSecurity);
		#else
		dlclose(hSecurity);
		#endif
		hSecurity = NULL;
	}
}

void QLib_Init(void) {
	int i;

	for (i = 0; i < qlib_nummodules; i++) {
		registeredModules[i].id = i;
		registeredModules[i].loaded = false;
		registeredModules[i].shutdown = NULL;
	}
}

void QLib_Shutdown(void) {
	int i;

	for (i = 0; i < qlib_nummodules; i++) {
		if (registeredModules[i].loaded) {
			registeredModules[i].shutdown();
			registeredModules[i].loaded = false;
		}
	}
}

void QLib_RegisterModule(qlib_id_t module, qlib_shutdown_fn shutdown) {
	if (module < 0 || module >= qlib_nummodules)
		Sys_Error("QLib_isModuleLoaded: bad module %d", module);

	registeredModules[module].loaded = true;
	registeredModules[module].shutdown = shutdown;
}

qboolean QLib_isModuleLoaded(qlib_id_t module) {
	if (module < 0 || module >= qlib_nummodules)
		Sys_Error("QLib_isModuleLoaded: bad module %d", module);

	return registeredModules[module].loaded;
}

qboolean QLib_ProcessProcdef(QLIB_HANDLETYPE_T handle, qlib_dllfunction_t *procdefs, int size) {
	int i;

	for (i = 0; i < size; i++) {
		if (!(*procdefs[i].function = QLIB_GETPROCADDRESS(handle, procdefs[i].name))) {
			for (i = 0; i < size; i++)
				procdefs[i].function = NULL;
			return false;
		}
	}
	return true;
}

void QLib_MissingModuleError(int errortype, char *libname, char *cmdline, char *features) {
	if (!COM_CheckParm("-showliberrors"))
		return;
	switch (errortype) {
	case QLIB_ERROR_MODULE_NOT_FOUND:
		Sys_Error(
			"QW262 couldn't load the required \"%s" QLIB_LIBRARY_EXTENSION "\" library.  You must\n"
			"specify \"%s\" on the cmdline to disable %s.",
			libname, cmdline, features
			);
		break;
	case QLIB_ERROR_MODULE_MISSING_PROC:
		Sys_Error("Broken \"%s" QLIB_LIBRARY_EXTENSION "\" library - required function missing.", libname);
		break;
	default:
		Sys_Error("QLib_MissingModuleError: unknown error type (%d)", errortype);
	}
}
