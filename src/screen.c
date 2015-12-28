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
// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include <time.h>
#include <stdlib.h>
/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net 
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full
	

*/


// only the refresh window will be updated unless these variables are flagged 
int			scr_copytop;
int			scr_copyeverything;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

float		oldsbar=0;
float		oldsbarinv=0;

qboolean OnViewSizeChange (cvar_t *var, const char *value)
{
	float newvalue = Q_atof(value);

	if (newvalue > 120)
		newvalue = 120;
	else if (newvalue < 30)
		newvalue = 30;

	if (newvalue == scr_viewsize.value)
		return true;


	Cvar_SetValue(&scr_viewsize, newvalue);
	vid.recalc_refdef = 1;
	return true;
}

cvar_t		scr_viewsize = {"viewsize","100", CVAR_ARCHIVE, OnViewSizeChange};

// BorisU -->
qboolean OnFovChange (cvar_t *var, const char *value);
qboolean OnDefaultFovChange (cvar_t *var, const char *value);
cvar_t		scr_fov = {"fov","90",0, OnFovChange};	// 10 - 140
cvar_t		cl_fovtodemo = {"cl_fovtodemo", "1"};
cvar_t		cl_fovfromdemo = {"cl_fovfromdemo", "1"};
cvar_t		default_fov = {"default_fov","90", 0, OnDefaultFovChange};		
// <--BorisU
cvar_t		scr_consize = {"scr_consize","0.5"}; // Tonik
cvar_t		scr_conspeed = {"scr_conspeed","300"};
cvar_t		scr_centertime = {"scr_centertime","2"};
cvar_t		scr_centershift = {"scr_centershift", "0"}; // BorisU
cvar_t		scr_showram = {"showram","1"};
cvar_t		scr_showturtle = {"showturtle","0"};
cvar_t		scr_showpause = {"showpause","1"};
cvar_t		scr_printspeed = {"scr_printspeed","8"};

cvar_t	show_speed		= {"show_speed", "0"};		// BorisU
cvar_t	show_lagmeter	= {"show_lagmeter", "0"};	// BorisU
cvar_t	lagmeter_format	= {"lagmeter_format", "0"};	// BorisU
cvar_t	cl_hud			= {"cl_hud", "1"};			// BorisU
cvar_t	scr_democlock	= {"cl_democlock", "1"};	// fuh

cvar_t	show_fps = {"show_fps","0"};				// set for running times
//cvar_t		scr_allowsnap = {"scr_allowsnap", "1"};

qboolean	scr_initialized;		// ready to draw

mpic_t		*scr_ram;
mpic_t		*scr_net;
mpic_t		*scr_turtle;

int			scr_fullupdate;

int			clearconsole;
int			clearnotify;

extern int			sb_lines;

viddef_t	vid;				// global video state

vrect_t		*pconupdate;
vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;

qboolean	scr_skipupdate;

qboolean	block_drawing;

void SCR_ScreenShot_f (void);
void SCR_RSShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_EraseCenterString (void)
{
	int		y;

	if (scr_erase_center++ > vid.numpages)
	{
		scr_erase_lines = 0;
		return;
	}

	if (scr_center_lines <= 4)
		y = vid.conheight*0.35;
	else
		y = 48;

	scr_copytop = 1;
	Draw_TileClear (0, y, vid.conwidth, min(8*scr_erase_lines, vid.conheight - y - 1));
}

void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = vid.conheight*0.35;
	else
		y = 48 + scr_centershift.value*8; // BorisU

	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.conwidth - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);	
			if (!remaining--)
				return;
		}
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= cls.frametime;
	
	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

extern	cvar_t		v_idlescale;
char LocalCmdBuf[32]="";

qboolean	concussioned = false;

qboolean OnFovChange (cvar_t *var, const char *value)
{
	
	float newfov = Q_atof(value);

	if (newfov > 140)
		newfov = 140;
	else if (newfov < 10)
		newfov = 10;

	if (newfov == scr_fov.value)
		return true;

	if ( cbuf_current != &cbuf_svc) {
		if (concussioned && !cls.demoplayback)
			return true;
		if(cls.demorecording && cl_fovtodemo.value) {
			sprintf(LocalCmdBuf,"fov %s\n",value);
		}	
	} else {
		if (newfov != 90 && cl.teamfortress && v_idlescale.value >= 20) {
			concussioned = true;
			if (v_idlescale.value == 100)
				CL_ExecTrigger ("f_conc");
		} else if (newfov == 90 && cl.teamfortress) {
			concussioned = false;
		}
		if (cls.demoplayback && !cl_fovfromdemo.value)
			return true;
	}

	vid.recalc_refdef = true;
	if (newfov == 90) {
		Cvar_Set (&scr_fov,default_fov.string);
		return true;
	}

	Cvar_SetValue (&scr_fov, newfov);
	return true;
}

qboolean OnDefaultFovChange (cvar_t *var, const char *value)
{
	float newfov = Q_atof(value);
	
	if (newfov < 10.0 || newfov > 140.0){
		Con_Printf("Invalid default_fov\n");
		return true;
	}
	return false;
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue (&scr_viewsize,scr_viewsize.value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue (&scr_viewsize,scr_viewsize.value-10);
}

//============================================================================
extern void SCR_ConWidth_f (void);

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	Cvar_RegisterVariable (&scr_fov);
	Cvar_RegisterVariable (&cl_fovtodemo);
	Cvar_RegisterVariable (&cl_fovfromdemo);
	Cvar_RegisterVariable (&default_fov);
	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_consize); // Tonik
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showram);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_centershift); // BorisU
	Cvar_RegisterVariable (&scr_printspeed);

// BorisU -->
	Cvar_RegisterVariable (&show_fps);
	Cvar_RegisterVariable (&show_speed);
	Cvar_RegisterVariable (&show_lagmeter);
	Cvar_RegisterVariable (&lagmeter_format);
	Cvar_RegisterVariable (&cl_hud);
// <-- BorisU

	Cvar_RegisterVariable (&scr_democlock); // fuh

//	Cvar_RegisterVariable (&scr_allowsnap);

#ifdef GLQUAKE
	Cvar_RegisterVariable (&gl_triplebuffer);
// BorisU -->
	Cvar_RegisterVariable (&gl_screenshot_format);
#ifdef __USE_JPG__
	Cvar_RegisterVariable (&gl_screenshot_jpeg_quality);
#endif
#ifdef __USE_PNG__
	Cvar_RegisterVariable (&gl_screenshot_png_compression);
#endif
// <-- BorisU
#endif

//
// register our commands
//
	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
//	Cmd_AddCommand ("snap",SCR_RSShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

#ifdef GLQUAKE
	Cmd_AddCommand ("conwidth",SCR_ConWidth_f); // BorisU

	scr_ram = Draw_CacheWadPic ("ram");
	scr_net = Draw_CacheWadPic ("net");
	scr_turtle = Draw_CacheWadPic ("turtle");
#else
	scr_ram = W_GetLumpName ("ram");
	scr_net = W_GetLumpName ("net");
	scr_turtle = W_GetLumpName ("turtle");
#endif

	scr_initialized = true;
}

/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;

	Draw_Pic (scr_vrect.x+32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;
	
	if (!scr_showturtle.value)
		return;

	if (cls.frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged < UPDATE_BACKUP-1)
		return;
	if (cls.demoplayback)
		return;

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

// BorisU -->
static char FpsStr[16];
double lastframetime;

char* Hud_FpsStr(void)
{
	extern int fps_count;
	static int lastfps;
	double t;

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 1.0) {
		lastfps = fps_count;
		fps_count = 0;
		lastframetime = t;
	}
	sprintf(FpsStr, "%3d FPS", lastfps);

	return FpsStr;
}

void SCR_DrawFPS (void)
{
	int x, y;

	Hud_FpsStr();

	if (!show_fps.value)
		return;

	x = vid.conwidth - strlen(FpsStr) * 8 - 8;
	y = vid.conheight - sb_lines - 8;
//	Draw_TileClear(x, y, strlen(st) * 8, 8);
	Draw_String(x, y, FpsStr);
}


static char LagmeterStr[80];
extern int packet_loss;
double last_lagmeter_time = 0;

char* Hud_LagmeterStr(void)
{
	frame_t *frame;
	double latency;
	int lat;
	char pl[4];
	char pingstr[16];

	if (cls.realtime - last_lagmeter_time >= 1.0) {
		last_lagmeter_time = cls.realtime;
		frame = &cl.frames[cls.netchan.incoming_acknowledged & UPDATE_MASK];
		latency = /*frame->receivedtime*/ cls.realtime - frame->senttime; // FIXME
		lat = (int)(latency * 1000);
		if (lat > 999) lat = 999;
		sprintf(pingstr, "%d", lat);
		Cvar_SetROM(&cl_ping, pingstr);

		pl[0] = packet_loss / 10 + '0';
		pl[1] = packet_loss % 10 + '0';
		pl[2] = '\0';
		if (packet_loss > 25) { 
			pl[0] |= 0x80;
			pl[1] |= 0x80;
		}
		switch ((int)lagmeter_format.value) {
		case 1:
			sprintf(LagmeterStr, "%d:%s", lat, pl); break;
		default:
			sprintf(LagmeterStr, "%dms:%s%%", lat, pl); break;
		}
	}
	return LagmeterStr;
}

void SCR_DrawLagmeter (void)
{
	int		x, y;

	Hud_LagmeterStr(); // update cl_ping cvar

	if (!show_lagmeter.value)
		return;

	x = vid.conwidth - strlen(LagmeterStr) * 8 - 8;
	y = vid.conheight - sb_lines - 16;
	Draw_String(x, y, LagmeterStr);
}

// is modified every second by Set_date_time_vars() function from cl_main.c
char ClockStr[16];

char* Hud_ClockStr(void)
{
	return ClockStr;
}

static char SpeedStr[16];

float	display_speed = 0;
double	last_speed_time = 0;
vec3_t	SpeedVec;

char* Hud_SpeedStr(void) 
{
	static float maxspeed = 0;
	float speed;

	if (cls.mvdplayback)
		speed = sqrt(SpeedVec[0]*SpeedVec[0] + SpeedVec[1]*SpeedVec[1]);
	else
		speed = sqrt(cl.simvel[0]*cl.simvel[0] + cl.simvel[1]*cl.simvel[1]);

	if (speed > maxspeed)
		maxspeed = speed;

	sprintf (SpeedStr, "%3d", (int)display_speed);

	if (cls.realtime - last_speed_time > 0.15) {
		last_speed_time = cls.realtime;
		display_speed = maxspeed;
		maxspeed = 0;
	}

	return SpeedStr;
}
// <-- BorisU

// fuh -->
void SCR_DrawDemoClock (void) 
{
	char	str[80];
	int		time_int;

	if (!cls.demoplayback || !scr_democlock.value)
		return;

	if (scr_democlock.value == 1)
		time_int = cls.realtime - cls.demostarttime;
	else
		time_int = cls.realtime;
	strlcpy (str, SecondsToHourString(time_int), sizeof(str));

	Draw_String (0, vid.conheight - sb_lines - 8*(1+cls.mvdplayback), str);
}
// <-- fuh

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	mpic_t	*pic;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	if (!cl.paused)
		return;

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (vid.conwidth - pic->width)/2, 
		(vid.conheight - 48 - pic->height)/2, pic);
}


//=============================================================================

/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	Con_CheckResize ();
	
// decide on the height of the console
	if (cls.state != ca_active)
	{
		scr_conlines = vid.conheight;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console) {
		scr_conlines = vid.conheight * scr_consize.value;
		if (scr_conlines < 30)
			scr_conlines = 30;
		if (scr_conlines > vid.conheight - 10)
			scr_conlines = vid.conheight - 10;
	} else
		scr_conlines = 0;				// none visible
	
	if (scr_conlines != scr_con_current)
	{
#ifndef GLQUAKE
		vid.recalc_refdef = true;
#endif
		if (scr_conlines < scr_con_current)
		{
			scr_con_current -= scr_conspeed.value*cls.trueframetime;
			if (scr_conlines > scr_con_current)
				scr_con_current = scr_conlines;
		}
		else if (scr_conlines > scr_con_current)
		{
			scr_con_current += scr_conspeed.value*cls.trueframetime;
			if (scr_conlines < scr_con_current)
				scr_con_current = scr_conlines;
		}
	}

	if (clearconsole++ < vid.numpages)
	{
		scr_copytop = 1;
		Draw_TileClear (0,(int)scr_con_current,vid.conwidth, vid.conheight - (int)scr_con_current);
		Sbar_Changed ();
	}
	else if (clearnotify++ < vid.numpages)
	{
		scr_copytop = 1;
		Draw_TileClear (0,0,vid.conwidth, con_notifylines);
	}
	else
		con_notifylines = 0;
}
	
/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole (scr_con_current);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


//=============================================================================

char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;

	start = scr_notifystring;

	y = vid.conheight*0.35;

	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.conwidth - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);	
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.  
==================
*/
int SCR_ModalMessage (char *text)
{
	scr_notifystring = text;
 
// draw a fresh screen
	scr_fullupdate = 0;
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;
	
	S_ClearBuffer ();		// so dma doesn't loop current sound

	do
	{
		key_count = -1;		// wait for a key down and up
		Sys_SendKeyEvents ();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	scr_fullupdate = 0;
	SCR_UpdateScreen ();

	return key_lastpress == 'y';
}


//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int		i;
	
	scr_centertime_off = 0;
	
	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.cshifts[0].percent = 0;		// no area contents palette on next frame
	VID_SetPalette (host_basepal);
}

/*
==================
SCR_UpdateWholeScreen
==================
*/
void SCR_UpdateWholeScreen (void)
{
	scr_fullupdate = 0;
	SCR_UpdateScreen ();
}

// BorisU -->
// User-defined HUD 
static hud_element_t *hud_list=NULL;
static hud_element_t *prev;

hud_element_t *Hud_FindElement(char *name)
{
	hud_element_t *elem;

	prev=NULL;
	for(elem=hud_list; elem; elem = elem->next) {
		if (!Q_stricmp(name, elem->name))
			return elem;
		prev = elem;
	}

	return NULL;
}

static hud_element_t* Hud_NewElement(void)
{
	hud_element_t*	elem;
	elem = Z_Malloc (sizeof(hud_element_t));
	elem->next = hud_list;
	hud_list = elem;
	elem->name = Z_StrDup( Cmd_Argv(1) );
	return elem;
}

static void Hud_DeleteElement(hud_element_t *elem)
{
	if (elem->flags & (HUD_STRING|HUD_IMAGE))
		Z_Free(elem->contents);
	if (elem->f_hover)
		Z_Free(elem->f_hover);
	if (elem->f_button)
		Z_Free(elem->f_button);
	Z_Free(elem->name);
	Z_Free(elem);
}

typedef void Hud_Elem_func(hud_element_t*);

void Hud_ReSearch_do(Hud_Elem_func f)
{
	hud_element_t *elem;

	for(elem=hud_list; elem; elem = elem->next) {
		if (ReSearchMatch(elem->name))
			f(elem);
	}
}

void Hud_Add_f(void)
{
	hud_element_t*	elem;
	struct hud_element_s*	next = NULL;	// Sergio
	qboolean	hud_restore = false;	// Sergio
	cvar_t		*var;
	char		*a2, *a3;
	Hud_Func	func;
	unsigned	old_coords = 0;
	unsigned	old_width = 0;
	float		old_alpha = 1;
	qboolean	old_enabled = true;

	if (Cmd_Argc() != 4)
		Con_Printf("Usage: hud_add <name> <type> <param>\n");
	else {
		if ((elem=Hud_FindElement(Cmd_Argv(1)))) {
			if (elem) {
				old_coords = *((unsigned*)&(elem->coords));
				old_width = elem->width;
				old_alpha = elem->alpha;
				old_enabled = elem->flags & HUD_ENABLED;
				next = elem->next; // Sergio
				if (prev) {
					prev->next = elem->next;
					hud_restore = true; // Sergio
				} else
					hud_list = elem->next;
				Hud_DeleteElement(elem);
			}
		} 
		
		a2 = Cmd_Argv(2);
		a3 = Cmd_Argv(3);
		
		if (!Q_stricmp(a2, "cvar")) {
			if( (var = Cvar_FindVar(a3)) ) {
				elem = Hud_NewElement();
				elem->contents = var;
				elem->flags = HUD_CVAR | HUD_ENABLED;
			} else {
				Con_Printf("cvar \"%s\" not found\n", a3);
				return;
			} 
		} else if (!Q_stricmp(a2, "str")) {
			elem = Hud_NewElement();
			elem->contents = Z_StrDup( a3 );
			elem->flags = HUD_STRING | HUD_ENABLED;
		} else if (!Q_stricmp(a2, "std")) { // to add armor, health, ammo, speed
			if (!Q_stricmp(a3, "lag"))
				func = &Hud_LagmeterStr;
			else if (!Q_stricmp(a3, "fps"))
				func = &Hud_FpsStr;
			else if (!Q_stricmp(a3, "clock"))
				func = &Hud_ClockStr;
			else if (!Q_stricmp(a3, "speed"))
				func = &Hud_SpeedStr;
			else {
				Con_Printf("\"%s\" is not a standard hud function\n", a3);
				return;
			}
			elem = Hud_NewElement();
			elem->contents = func;
			elem->flags = HUD_FUNC | HUD_ENABLED;
		} else if (!Q_stricmp(a2, "img")) {
#ifdef GLQUAKE
			mpic_t *hud_image;
			int texnum = loadtexture_24bit(a3, LOADTEX_GFX);
			if (!texnum) {
				Con_Printf("Unable to load hud image \"%s\"\n", a3);
				return;
			}
			hud_image = Z_Malloc (sizeof(mpic_t));
			hud_image->texnum = texnum;
			if (current_texture) {
				hud_image->width = current_texture->width;
				hud_image->height = current_texture->height;
			}
			else {
				hud_image->width = image.width;
				hud_image->height = image.height;
			}
			hud_image->sl = 0;
			hud_image->sh = 1;
			hud_image->tl = 0;
			hud_image->th = 1;
			elem = Hud_NewElement();
			elem->contents = hud_image;
			elem->flags = HUD_IMAGE | HUD_ENABLED;
#else
			Con_Printf("Hud images not available in software version\n");
			return;
#endif
		} else {
			Con_Printf("\"%s\" is not a valid hud type\n", a2);
			return;
		}

		*((unsigned*)&(elem->coords)) = old_coords;
		elem->width = old_width;
		elem->alpha = old_alpha;
		if (!old_enabled)
			elem->flags &= ~HUD_ENABLED;

// Sergio -->
// Restoring old hud place in hud_list
		if (hud_restore) {
			hud_list = elem->next;
			elem->next = next;
			prev->next = elem;
			}
// <-- Sergio
	}
}

void Hud_Elem_Remove(hud_element_t *elem)
{
	if (prev) 
		prev->next = elem->next;
	else
		hud_list = elem->next;
	Hud_DeleteElement(elem);
}

void Hud_Remove_f(void)
{
	hud_element_t	*elem, *next_elem;
	char			*name;
	int				i;
	
	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			prev = NULL;
			for(elem=hud_list; elem; ) {
				if (ReSearchMatch(elem->name)) {
					next_elem = elem->next;
					Hud_Elem_Remove(elem);
					elem = next_elem;
				} else {
					prev = elem;
					elem = elem->next;
				}
			}
			ReSearchDone();
		} else {
			if ((elem = Hud_FindElement(name)))
				Hud_Elem_Remove(elem);
			else
				Con_Printf("HudElement \"%s\" not found\n", name);
		}
	}
}

void Hud_Position_f(void)
{
	hud_element_t *elem;

	if (Cmd_Argc() != 5) {
		Con_Printf("Usage: hud_position <name> <pos_type> <x> <y>\n");
		return;
	}
	if (!(elem = Hud_FindElement(Cmd_Argv(1)))) {
		Con_Printf("HudElement \"%s\" not found\n", Cmd_Argv(1));
		return;
	}

	elem->coords[0] = atoi(Cmd_Argv(2));
	elem->coords[1] = atoi(Cmd_Argv(3));
	elem->coords[2] = atoi(Cmd_Argv(4));
}

void Hud_Elem_Bg(hud_element_t *elem)
{
	elem->coords[3] = atoi(Cmd_Argv(2));
}

void Hud_Bg_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);
	
	if (Cmd_Argc() != 3) 
		Con_Printf("Usage: hud_bg <name> <bgcolor>\n");
	else if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Bg);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Bg(elem);
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}

void Hud_Elem_Move(hud_element_t *elem)
{
	elem->coords[1] += atoi(Cmd_Argv(2));
	elem->coords[2] += atoi(Cmd_Argv(3));
}

void Hud_Move_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);
		
	if (Cmd_Argc() != 4)
		Con_Printf("Usage: hud_move <name> <dx> <dy>\n");
	else if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Move);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Move(elem);	
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}

void Hud_Elem_Width(hud_element_t *elem)
{
	if (elem->flags & HUD_IMAGE) {
		mpic_t *pic = elem->contents;
		int width = atoi(Cmd_Argv(2))*8;
		int height = width * pic->height / pic->width;
		pic->height = height;
		pic->width = width;
	}
	elem->width = max(min(atoi(Cmd_Argv(2)), 128), 0);
}

void Hud_Width_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);
		
	if (Cmd_Argc() != 3)
		Con_Printf("Usage: hud_width <name> <width>\n");
	else if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Width);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Width(elem);	
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}

#ifdef GLQUAKE
extern int char_texture;

void Hud_Elem_Font(hud_element_t *elem)
{
	if (elem->flags & HUD_IMAGE)
		return;

	elem->charset = loadtexture_24bit (Cmd_Argv(2), LOADTEX_CHARS);
}

void Hud_Font_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);
		
	if (Cmd_Argc() != 3)
		Con_Printf("Usage: hud_font <name> <font>\n");
	else if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Font);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Font(elem);	
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}

void Hud_Elem_Alpha(hud_element_t *elem)
{
	float alpha = atof (Cmd_Argv(2));
	elem->alpha = bound (0, alpha, 1);
}

void Hud_Alpha_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);

	if (Cmd_Argc() != 3)
	{
		Con_Printf("hud_alpha <name> <value> : set HUD transparency (0..1)\n");
		return;
	}
	if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Alpha);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Alpha(elem);
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}
#endif

void Hud_Elem_Blink(hud_element_t *elem)
{
	double		blinktime;
	unsigned	mask;

	blinktime = atof(Cmd_Argv(2))/1000.0;
	mask = atoi(Cmd_Argv(3));

	if (mask < 0 || mask > 3) return; // bad mask
	if (blinktime < 0.0 || blinktime > 5.0) return;

	elem->blink = blinktime;
	elem->flags = (elem->flags & (~(HUD_BLINK_F | HUD_BLINK_B))) | (mask << 3);
}

void Hud_Blink_f(void)
{
	hud_element_t *elem;
	char	*name = Cmd_Argv(1);
		
	if (Cmd_Argc() != 4)
		Con_Printf("Usage: hud_blink <name> <ms> <mask>\n");
	else if (IsRegexp(name)) {
		if(!ReSearchInit(name))
			return;
		Hud_ReSearch_do(Hud_Elem_Blink);
		ReSearchDone();
	} else {
		if ((elem = Hud_FindElement(name)))
			Hud_Elem_Blink(elem);	
		else
			Con_Printf("HudElement \"%s\" not found\n", name);
	}
}

void Hud_Elem_Disable(hud_element_t *elem)
{
	elem->flags &= ~HUD_ENABLED;
}

void Hud_Disable_f(void)
{
	hud_element_t	*elem;
	char			*name;
	int				i;

	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			Hud_ReSearch_do(Hud_Elem_Disable);
			ReSearchDone();
		} else {
			if ((elem = Hud_FindElement(name)))
				Hud_Elem_Disable(elem);	
			else
				Con_Printf("HudElement \"%s\" not found\n", name);
		}
	}
}

void Hud_Elem_Enable(hud_element_t *elem)
{
	elem->flags |= HUD_ENABLED;
}

void Hud_Enable_f(void)
{
	hud_element_t	*elem;
	char			*name;
	int				i;

	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			Hud_ReSearch_do(Hud_Elem_Enable);
			ReSearchDone();
		} else {
			if ((elem = Hud_FindElement(name)))
				Hud_Elem_Enable(elem);	
			else
				Con_Printf("HudElement \"%s\" not found\n", name);
		}
	}
}

void Hud_List_f(void)
{
	hud_element_t	*elem;
	char			*type;
	char			*param;
	int				c, i, m;

	c = Cmd_Argc();
	if (c>1)
		if (!ReSearchInit(Cmd_Argv(1)))
			return;

	Con_Printf ("List of hud elements:\n");
	for(elem=hud_list, i=m=0; elem; elem = elem->next, i++) {
		if (c==1 || ReSearchMatch(elem->name)) {
			if (elem->flags & HUD_CVAR) {
				type = "cvar";
				param = ((cvar_t*)elem->contents)->name;
			} else if (elem->flags & HUD_STRING) {
				type = "str";
				param = elem->contents;
			} else if (elem->flags & HUD_FUNC) {
				type = "std";
				param = "***";
			} else if (elem->flags & HUD_IMAGE) {
				type = "img";
				param = "***";
			} else {
				type = "invalid type";
				param = "***";
			}
			m++;
			Con_Printf("%s : %s : %s\n", elem->name, type, param);
		}
	}
	
	Con_Printf ("------------\n%i/%i hud elements\n", m, i);
	if (c>1)
		ReSearchDone();
}

void Hud_BringToFront_f(void)
{
	hud_element_t *elem, *start, *end;

	if (Cmd_Argc() != 2) {
		Con_Printf("Usage: hud_bringtofront <name>\n");
		return;
	}

	end = Hud_FindElement(Cmd_Argv(1));
	if (end) {
		if (end->next) {
			start = hud_list;
			hud_list = end->next;
			end->next = NULL;
			for(elem=hud_list; elem->next; elem = elem->next) {}
			elem->next = start;
		}
	} else {
		Con_Printf("HudElement \"%s\" not found\n", Cmd_Argv(1));
	}
}

void Hud_Hover_f (void)
{
	hud_element_t *elem;

	if (Cmd_Argc() != 3) {
		Con_Printf("hud_hover <name> <alias> : call alias when mouse is over hud\n");
		return;
	}

	elem = Hud_FindElement(Cmd_Argv(1));
	if (elem) {
		if (elem->f_hover)
			Z_Free (elem->f_hover);
		elem->f_hover = Z_StrDup (Cmd_Argv(2));
	} else {
		Con_Printf("HudElement \"%s\" not found\n", Cmd_Argv(1));
	}
}

void Hud_Button_f (void)
{
	hud_element_t *elem;

	if (Cmd_Argc() != 3) {
		Con_Printf("hud_button <name> <alias> : call alias when mouse button pressed on hud\n");
		return;
	}

	elem = Hud_FindElement(Cmd_Argv(1));
	if (elem) {
		if (elem->f_button)
			Z_Free (elem->f_button);
		elem->f_button = Z_StrDup (Cmd_Argv(2));
	} else {
		Con_Printf("HudElement \"%s\" not found\n", Cmd_Argv(1));
	}
}

qboolean Hud_TranslateCoords (hud_element_t *elem, int *x, int *y)
{
	int l;

	if (!elem->scr_width || !elem->scr_height)
		return false;

	l = elem->scr_width / 8;

	switch (elem->coords[0]) {
		case 1:	*x = elem->coords[1]*8 + 1;					// top left
				*y = elem->coords[2]*8;
				break;
		case 2:	*x = vid.conwidth - (elem->coords[1] + l)*8 -1;	// top right
				*y = elem->coords[2]*8;
				break;
		case 3:	*x = vid.conwidth - (elem->coords[1] + l)*8 -1;	// bottom right
				*y = vid.conheight - sb_lines - (elem->coords[2]+1)*8;
				break;
		case 4:	*x = elem->coords[1]*8 + 1;					// bottom left
				*y = vid.conheight - sb_lines - (elem->coords[2]+1)*8;
				break;
		case 5:	*x = vid.conwidth / 2 - l*4 + elem->coords[1]*8;// top center
				*y = elem->coords[2]*8;
				break;
		case 6:	*x = vid.conwidth / 2 - l*4 + elem->coords[1]*8;// bottom center
				*y = vid.conheight - sb_lines - (elem->coords[2]+1)*8;
				break;
		default:
				return false;
	}
	return true;
}

void SCR_DrawHud (void)
{
	hud_element_t*	elem;
	int				x,y,l;
	char			buf[256];
	char			*st = NULL;
	Hud_Func		func;
	double			tblink = 0;
	mpic_t			*img = NULL;

	if (hud_list && cl_hud.value &&
		!(cls.demoplayback && cl_demoplay_restrictions.value) && 
		!(cl.spectator && cl_spectator_restrictions.value) ) {

		for (elem = hud_list; elem; elem=elem->next) {
			if (!(elem->flags & HUD_ENABLED)) continue; // do not draw disabled elements

			elem->scr_height = 8;
		
			if (elem->flags & HUD_CVAR) {
				st = ((cvar_t*)elem->contents)->string;
				strlcpy (buf, st, sizeof(buf));
				st = buf;
				l = strlen (st);
			} else if (elem->flags & HUD_STRING) {
				Cmd_ExpandString(elem->contents, buf, sizeof(buf));
				st = ParseSay(buf);
				l = strlen(st);
			} else if (elem->flags & HUD_FUNC) {
				func = elem->contents;
				st =(*func)();
				l = strlen(st);
			} else if (elem->flags & HUD_IMAGE) {
				img = (mpic_t*)elem->contents;
				l = img->width/8;
				elem->scr_height = img->height;
			} else
				continue;

			if (elem->width && !(elem->flags & (HUD_FUNC|HUD_IMAGE))){
				if (elem->width < l) {
					l = elem->width;
					st[l] = '\0';
				} else {
					while (elem->width > l) {
						st[l++] = ' ';
					}
					st[l] = '\0';
				}
			}
			elem->scr_width = l*8;

			if (!Hud_TranslateCoords (elem, &x, &y))
				continue;

			if (elem->flags & (HUD_BLINK_B|HUD_BLINK_F))
				tblink = fmod(cls.realtime, elem->blink)/elem->blink;

			if (!(elem->flags & HUD_BLINK_B) || tblink < 0.5)
				if (elem->coords[3])
				{
#ifdef GLQUAKE
					if (elem->alpha < 1)
						Draw_AlphaFill(x, y, elem->scr_width, elem->scr_height, (unsigned char)elem->coords[3], elem->alpha);
					else
#endif
						Draw_Fill(x, y, elem->scr_width, elem->scr_height, (unsigned char)elem->coords[3]);
				}
			if (!(elem->flags & HUD_BLINK_F) || tblink < 0.5)
			{
				if (!(elem->flags & HUD_IMAGE))
				{
#ifdef GLQUAKE
					extern int char_texture;
					int std_charset = char_texture;
					if (elem->charset)
						char_texture = elem->charset;
					if (elem->alpha < 1)
						Draw_AlphaString (x, y, st, elem->alpha);
					else
#endif
						Draw_String (x, y, st);
#ifdef GLQUAKE
					char_texture = std_charset;
#endif
				}
				else
#ifdef GLQUAKE
					if (elem->alpha < 1)
						Draw_AlphaPic (x, y, img, elem->alpha);
					else
#endif
						Draw_Pic (x, y, img);
			}
		}
	}
// Draw Input
	if (key_dest == key_message && chat_team == 100) {
		extern float	con_cursorspeed;
		extern int		chat_bufferpos;

		int		i,j;
		char	*s;
		char	t;

		x = input.x*8 + 1;					// top left
		y = input.y*8;
	
		if (input.bg)
			Draw_Fill(x, y, input.len*8, 8, input.bg);

		s = chat_buffer[chat_edit];
		t = chat_buffer[chat_edit][chat_bufferpos];
		i = chat_bufferpos;

		if (chat_bufferpos > (input.len - 1)) {
			s += chat_bufferpos - (input.len -1);
			i = input.len - 1;
		}

		j = 0;
		while(s[j] && j<input.len)	{
			Draw_Character ( x+(j<<3), y, s[j]);
			j++;
		}
		Draw_Character ( x+(i<<3), y, 10+((int)(cls.realtime*con_cursorspeed)&1));
	}

}

qboolean Hud_CheckBounds (hud_element_t *elem, int x, int y)
{
	int hud_x, hud_y, con_x, con_y;

	con_x = VID_ConsoleX (x);
	con_y = VID_ConsoleY (y);

	if (!Hud_TranslateCoords (elem, &hud_x, &hud_y))
		return false;

	if (con_x < hud_x || con_x >= (hud_x + elem->scr_width))
		return false;
	if (con_y < hud_y || con_y >= (hud_y + elem->scr_height))
		return false;

	return true;
}

void Hud_MouseEvent (int x, int y, int buttons)
{
	extern int		mouse_buttons;
	static int		old_x, old_y, old_buttons;
	hud_element_t	*elem;
	int				i;

	for (i=0 ; i<mouse_buttons ; i++)
	{
		if ((buttons & (1<<i)) && !(old_buttons & (1<<i)))
			break;
	}
	if (i < mouse_buttons)
		++i;
	else
		i = 0;

	for (elem = hud_list; elem; elem = elem->next)
	{
		if (!(elem->flags & HUD_ENABLED) || !elem->f_hover || !elem->f_button
			|| !elem->scr_width || !elem->scr_height)
			continue;

		if (Hud_CheckBounds (elem, x, y))
		{
			if (elem->f_hover && !Hud_CheckBounds (elem, old_x, old_y))
				Cbuf_AddText (va("%s 1 %s\n", elem->f_hover, elem->name));
			if (i && elem->f_button)
				Cbuf_AddText (va("%s %d %s\n", elem->f_button, i, elem->name));
		}
		else if (elem->f_hover && Hud_CheckBounds (elem, old_x, old_y))
			Cbuf_AddText (va("%s 0 %s\n", elem->f_hover, elem->name));
	}
	old_x = x;
	old_y = y;
	old_buttons = buttons;
}

void Hud_Init (void)
{
//
// register hud commands
//
	Cmd_AddCommandTrig ("hud_add",Hud_Add_f);
	Cmd_AddCommandTrig ("hud_remove",Hud_Remove_f);
	Cmd_AddCommandTrig ("hud_position",Hud_Position_f);
	Cmd_AddCommandTrig ("hud_bg",Hud_Bg_f);
	Cmd_AddCommandTrig ("hud_move",Hud_Move_f);
	Cmd_AddCommandTrig ("hud_width",Hud_Width_f);
#ifdef GLQUAKE
	Cmd_AddCommandTrig ("hud_font",Hud_Font_f);
	Cmd_AddCommandTrig ("hud_alpha",Hud_Alpha_f);
#endif
	Cmd_AddCommandTrig ("hud_blink",Hud_Blink_f);
	Cmd_AddCommandTrig ("hud_disable",Hud_Disable_f);
	Cmd_AddCommandTrig ("hud_enable",Hud_Enable_f);
	Cmd_AddCommandTrig ("hud_list",Hud_List_f);
	Cmd_AddCommandTrig ("hud_bringtofront",Hud_BringToFront_f);
	Cmd_AddCommandTrig ("hud_hover",Hud_Hover_f);
	Cmd_AddCommandTrig ("hud_button",Hud_Button_f);
}
// <-- BorisU
