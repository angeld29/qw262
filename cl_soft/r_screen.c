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
#include "r_local.h"

/*
============================================================================== 

						SCREEN SHOTS 

============================================================================== 
*/


/*
==============
WritePCXfile
==============
*/ 
void WritePCXfile (char *filename, byte *data, int width, int height,
	int rowbytes, byte *palette, qboolean upload) 
{
	int		i, j, length;
	pcx_t	*pcx;
	byte	*pack;
	FILE	*fp;

	pcx = Hunk_TempAlloc (width*height*2+1000);
	if (pcx == NULL)
	{
		Con_Printf("SCR_ScreenShot_f: not enough memory\n");
		return;
	} 
 
	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width-1));
	pcx->ymax = LittleShort((short)(height-1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	memset (pcx->filler,0,sizeof(pcx->filler));

// pack the image
	pack = &pcx->data;
	
	for (i=0 ; i<height ; i++)
	{
		for (j=0 ; j<width ; j++)
		{
			if ( (*data & 0xc0) != 0xc0)
				*pack++ = *data++;
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data += rowbytes - width;
	}
			
// write the palette
	*pack++ = 0x0c;	// palette ID byte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;
		
// write output file 
	length = pack - (byte *)pcx;
	if (upload)
		CL_StartUpload((void *)pcx, length);
	else {
		if (!(fp = fopen (filename, "wb")))
			Sys_Error ("Error opening %s", filename);
			fwrite (pcx, 1, length, fp);
			fclose (fp);
	}
}

void SCR_ScreenShot(char *fname)
{
	extern byte	current_pal[768];	// Tonik
// 
// save the pcx file 
// 
	D_EnableBackBufferAccess ();	// enable direct drawing of console to back
									// buffer

	WritePCXfile (fname, vid.buffer, vid.width, vid.height, vid.rowbytes,
					  current_pal, false);

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//	for linear writes all the time
}

/*
================== 
SCR_MovieShot_f
================== 
*/
void SCR_MovieShot_f (void) 
{
	char	fname[MAX_OSPATH]; 

	sprintf(fname,"%s%06ld.pcx",capture_path,capture_num);

	if (++capture_num == 1000000) {
		Con_Printf ("Too many screenshots taken\n"); 
		capture_num = -1; // stop capture
		return;
	}

	SCR_ScreenShot(fname);
}


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void) 
{
	char			name[MAX_OSPATH]; 
	char			fname[MAX_OSPATH]; 
	int				i;

	Q_snprintfz (fname, sizeof(name), "%s/screenshots/", com_gamedir);
	COM_CreatePath (fname);

	if (Cmd_Argc() == 2) {
		strncpy (name, Cmd_Argv(1), sizeof(name));
		if (!COM_CheckFilename(name)) {
			Con_Printf("Invalid name for screenshot\n");
			return;
		}
		COM_ForceExtension (name, ".pcx");
		Q_snprintfz (fname, sizeof(fname), "%s/screenshots/%s", com_gamedir, name);
	} else {
// 
// find a file name to save it to 
// 
		for (i=0 ; i<=9999 ; i++) { 
			Q_snprintfz (fname, sizeof(fname), "%s/screenshots/quake%.4i.pcx", com_gamedir, i);
			if (!Sys_FileExists(fname)) {
				Q_snprintfz (name, sizeof(name), "quake%.4i.pcx", i);
				break;  // file doesn't exist
			}
		} 
		if (i==10000) {
			Con_Printf ("Too many screenshots taken\n"); 
			return;
		}
	}

	SCR_ScreenShot(fname);
	Con_Printf ("Wrote %s\n", name);
}

/*
Find closest color in the palette for named color
*/
int MipColor(int r, int g, int b)
{
	int i;
	float dist;
	int best = 0;
	float bestdist;
	int r1, g1, b1;
	static int lr = -1, lg = -1, lb = -1;
	static int lastbest;

	if (r == lr && g == lg && b == lb)
		return lastbest;

	bestdist = 256*256*3;

	for (i = 0; i < 256; i++) {
		r1 = host_basepal[i*3] - r;
		g1 = host_basepal[i*3+1] - g;
		b1 = host_basepal[i*3+2] - b;
		dist = r1*r1 + g1*g1 + b1*b1;
		if (dist < bestdist) {
			bestdist = dist;
			best = i;
		}
	}
	lr = r; lg = g; lb = b;
	lastbest = best;
	return best;
}

// in draw.c
extern byte		*draw_chars;				// 8*8 graphic characters

void SCR_DrawCharToSnap (int num, byte *dest, int width)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
//			if (source[x]!=255)
				if(source[x])
					dest[x] = source[x];
//				else
//					dest[x] = 98;
		source += 128;
		dest += width;
	}

}

void SCR_DrawStringToSnap (const char *s, byte *buf, int x, int y, int width)
{
	byte *dest;
	const unsigned char *p;

	dest = buf + ((y * width) + x);

	p = (const unsigned char *)s;
	while (*p) {
		SCR_DrawCharToSnap(*p++, dest, width);
		dest += 8;
	}
}


/* 
================== 
SCR_RSShot_f
================== 
*/  
void SCR_RSShot_f (void) 
{ 
	int				x, y;
	unsigned char	*src, *dest;
	char			pcxname[80]; 
	unsigned char	*newbuf;
	int				w, h;
	int				dx, dy, dex, dey, nx;
	int				r, b, g;
	int				count;
	float			fracw, frach;
	char			st[80];
//	time_t now;

	if (CL_IsUploading())
		return; // already one pending

	if (cls.state < ca_onserver)
		return; // gotta be connected

/*	if (!scr_allowsnap.value) {
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "snap\n");
		Con_Printf("Refusing remote screen shot request.\n");
		return;
	} */

	Con_Printf("Remote screen shot requested.\n");

#if 0
// 
// find a file name to save it to 
// 
	strcpy(pcxname,"mquake00.pcx");
		
	for (i=0 ; i<=99 ; i++) 
	{ 
		pcxname[6] = i/10 + '0'; 
		pcxname[7] = i%10 + '0'; 
		sprintf (checkname, "%s/%s", com_gamedir, pcxname);
		if (!Sys_FileExists(checkname))
			break;	// file doesn't exist
	} 
	if (i==100) 
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a PCX"); 
		return;
	}
#endif

// 
// save the pcx file 
// 
	D_EnableBackBufferAccess ();	// enable direct drawing of console to back
									//  buffer

	w = (vid.width < RSSHOT_WIDTH) ? vid.width : RSSHOT_WIDTH;
	h = (vid.height < RSSHOT_HEIGHT) ? vid.height : RSSHOT_HEIGHT;

	fracw = (float)vid.width / (float)w;
	frach = (float)vid.height / (float)h;

	newbuf = malloc(w*h);

	for (y = 0; y < h; y++) {
		dest = newbuf + (w * y);

		for (x = 0; x < w; x++) {
			r = g = b = 0;

			dx = x * fracw;
			dex = (x + 1) * fracw;
			if (dex == dx) dex++; // at least one
			dy = y * frach;
			dey = (y + 1) * frach;
			if (dey == dy) dey++; // at least one

			count = 0;
			for (/* */; dy < dey; dy++) {
				src = vid.buffer + (vid.rowbytes * dy) + dx;
				for (nx = dx; nx < dex; nx++) {
					r += host_basepal[*src * 3];
					g += host_basepal[*src * 3+1];
					b += host_basepal[*src * 3+2];
					src++;
					count++;
				}
			}
			r /= count;
			g /= count;
			b /= count;
			*dest++ = MipColor(r, g, b);
		}
	}

	//time(&now);
	//strcpy(st, ctime(&now));
	//st[strlen(st) - 1] = 0;
	//SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, 0, w);

	//strncpy(st, cls.servername, sizeof(st));
	//st[sizeof(st) - 1] = 0;
	//SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, 10, w);

	strncpy(st, name.string, sizeof(st));
	st[sizeof(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, 10, w);

	WritePCXfile (pcxname, newbuf, w, h, w, host_basepal, true);

	free(newbuf);

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time

//	Con_Printf ("Wrote %s\n", pcxname);
	Con_Printf ("Sending shot to server...\n");
} 

extern	float		oldsbar;
extern	float		oldsbarinv;
extern	qboolean	scr_initialized;
extern	qboolean	scr_drawdialog;
extern	cvar_t		scr_fov;
extern	cvar_t		default_fov;
extern	cvar_t		cl_sbar_inv;
extern float CalcFov (float fov_x, float width, float height);

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	vrect_t		vrect;
	float		size;

	scr_fullupdate = 0;		// force a background redraw
	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

//========================================

// bound field of view
	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

// intermission is always full screen	
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110 || cl_sbar_inv.value == 0) // BorisU
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24+16+8;

// these calculations mirror those in R_Init() for r_refdef, but take no
// account of water warping
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	R_SetVrect (&vrect, &scr_vrect, sb_lines);

// guard against going from one mode to another that's less than half the
// vertical resolution
	if (scr_con_current > vid.height)
		scr_con_current = vid.height;

// notify the refresh of the change
	R_ViewChanged (&vrect, sb_lines, vid.aspect);
}

// BorisU -->
void SCR_DrawSpeedometer (void)
{
	int x, y, length, charsize, speed, width;
	float scale;

	if(show_speed.value < 2) return;

	speed = display_speed;
	charsize = 8;
	length = vid.width / 3;
	scale = length / 1000.0;
	x = (vid.width - length) / 2;
	y = vid.height - sb_lines - charsize - 1;
	width = (speed % 1000) * scale;

	// draw the coloured indicator strip
	Draw_Fill(x, y, width, charsize, 184);

	Draw_Fill(x, y, length, 1, 192);
	Draw_Fill(x, y + charsize, length, 1, 192);
	Draw_Fill(x, y, 1, charsize, 192);
	Draw_Fill(x + length, y, 1, charsize + 1, 192);
	
	Draw_Fill(x+ length / 4, y, 1, charsize / 4, 192);
	Draw_Fill(vid.width / 2, y, 1, charsize *3 / 8, 192);
	Draw_Fill(x+ length * 3 / 4, y, 1, charsize / 4, 192);

	Draw_Fill(x+ length / 4, y + charsize * 3 / 4 + 1, 1,  charsize / 4, 192);
	Draw_Fill(vid.width / 2, y + charsize * 5 / 8 + 1, 1, charsize * 3 / 8, 192);
	Draw_Fill(x+ length * 3 / 4, y + charsize * 3 / 4 + 1, 1, charsize / 4, 192);


	if(show_speed.value > 2)
		Draw_String(x + 1, y , Hud_SpeedStr());
}
// <-- BorisU

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	vrect_t		vrect;

	if (scr_skipupdate || block_drawing)
		return;

	if (scr_disabled_for_loading)
		return;

#ifdef _WIN32
	{	// don't suck up any cpu if minimized
		extern int Minimized;

		if (Minimized)
			return;
	}
#endif

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

//
// check for vid changes
//
	if (oldsbar != cl_sbar.value || oldsbarinv != cl_sbar_inv.value) {
		oldsbar = cl_sbar.value;
		oldsbarinv = cl_sbar_inv.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
	{
		// something changed, so reorder the screen
		SCR_CalcRefdef ();
	}

//
// do 3D refresh drawing, and then update the screen
//
	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_fullupdate++ < vid.numpages)
	{	// clear the entire screen
		scr_copyeverything = 1;
//		Draw_TileClear (0,0,vid.width,vid.height);
		Sbar_Changed ();
	}

	pconupdate = NULL;


	SCR_SetUpToDrawConsole ();
	SCR_EraseCenterString ();
	
	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time

	VID_LockBuffer ();
	V_RenderView ();
	VID_UnlockBuffer ();

	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_drawdialog)
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
		scr_copyeverything = true;
	}
	else if (cl.intermission == 1 && key_dest == key_game)
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else
	{
		SCR_DrawRam ();
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_DrawFPS ();
		SCR_DrawLagmeter ();
		SCR_DrawSpeedometer ();
		if (show_speed.value) Sbar_Changed(); // forcing sbar redraw. BorisU
		Sbar_Draw ();
		SCR_CheckDrawCenterString ();
		SCR_DrawDemoClock (); // fuh
		SCR_DrawHud ();
		SCR_DrawConsole ();
		M_Draw ();
	}


	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time
	if (pconupdate)
	{
		D_UpdateRects (pconupdate);
	}

	V_UpdatePalette ();

//
// update one of three areas
//
	if (scr_copyeverything)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height;
		vrect.pnext = 0;
	
		VID_Update (&vrect);
	}
	else if (scr_copytop)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height - sb_lines;
		vrect.pnext = 0;
	
		VID_Update (&vrect);
	}	
	else
	{
		vrect.x = scr_vrect.x;
		vrect.y = scr_vrect.y;
		vrect.width = scr_vrect.width;
		vrect.height = scr_vrect.height;
		vrect.pnext = 0;
	
		VID_Update (&vrect);
	}	
}
