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
// screen.h

void SCR_Init (void);
void SCR_RSShot_f (void);
void SCR_UpdateScreen (void);
void SCR_MovieShot_f (void);

void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_BringDownConsole (void);
void SCR_CenterPrint (char *str);

int SCR_ModalMessage (char *text);

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			scr_fullupdate;	// set to 0 to force full redraw
extern	int			sb_lines;

extern	int			clearnotify;	// set to 0 whenever notify text is drawn
extern	qboolean	scr_disabled_for_loading;

extern	cvar_t		scr_viewsize;

extern cvar_t		scr_viewsize;

// BorisU -->
extern cvar_t		show_speed; 
extern float		display_speed;
extern vec3_t		SpeedVec;
// <-- BorisU

// only the refresh window will be updated unless these variables are flagged 
extern	int			scr_copytop;
extern	int			scr_copyeverything;

qboolean	scr_skipupdate;

qboolean	block_drawing;

void SCR_SetUpToDrawConsole (void);
void SCR_EraseCenterString (void);
void SCR_DrawConsole (void);
void SCR_DrawNotifyString (void);
void SCR_CheckDrawCenterString (void);
void SCR_DrawRam(void);
void SCR_DrawTurtle (void);
void SCR_DrawNet (void);
void SCR_DrawPause (void);
void SCR_DrawFPS (void);
void SCR_DrawLagmeter (void);
void SCR_DrawHud (void);
void SCR_DrawDemoClock (void); // fuh

// Added by BorisU
// Hud

typedef char* (*Hud_Func)();

typedef struct hud_element_s {
	struct hud_element_s*	next;
	char					*name;
	unsigned				flags;
	signed char				coords[4]; // pos_type, x, y, bg
	unsigned				width;
	float					blink;
	void*					contents;
	int						charset;
	float					alpha;
	char					*f_hover, *f_button;
	unsigned				scr_width, scr_height;
} hud_element_t;

#define		HUD_CVAR		1
#define		HUD_FUNC		2
#define		HUD_STRING		4
#define		HUD_BLINK_F		8
#define		HUD_BLINK_B		16
#define		HUD_IMAGE		32

#define		HUD_ENABLED		512

extern char ClockStr[16];

void Hud_Init (void);
char* Hud_SpeedStr(void); 
hud_element_t *Hud_FindElement(char *name);
void Hud_MouseEvent (int x, int y, int buttons);

extern qboolean	concussioned;
