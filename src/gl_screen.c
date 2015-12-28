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

#include "gl_image.h"

int		glx, gly, glwidth, glheight;

cvar_t	gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};
qboolean OnScreenShotFormatChange (cvar_t *var, const char *value);
cvar_t	gl_screenshot_format = {"gl_screenshot_format", "0", 0, OnScreenShotFormatChange};

extern	cvar_t	crosshair;

extern int		sb_lines;

extern viddef_t	vid; // global video state

extern vrect_t	scr_vrect;

qboolean		scr_disabled_for_loading;
qboolean		scr_drawloading;
float			scr_disabled_time;

qboolean		block_drawing;

void SCR_ScreenShot_f (void);
void SCR_RSShot_f (void);
extern mpic_t*	conback;

// BorisU -->
// changing conwidth on the fly
void SCR_ConWidth_f (void)
{
	int new_width = Q_atoi (Cmd_Argv(1));

	if (Cmd_Argc() <2)
		return;

	new_width = bound (320, new_width, 1024); // fixme vid.width ?
	new_width &= 0xfff8; // make it a multiple of eight

	vid.conwidth = new_width;
	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4; // fixme aspect?

	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	Con_Resize (&con_main);
	Con_Resize (&con_chat);

	vid.recalc_refdef = 1;
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	mpic_t	*pic;

	if (!scr_drawloading)
		return;
		
	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (vid.conwidth - pic->width)/2, 
		(vid.conheight - 48 - pic->height)/2, pic);
}


/*
============================================================================== 

						SCREEN SHOTS 

============================================================================== 
*/

extern unsigned short	ramps[3][256]; // gamma ramps

static void SCR_GetPixels(void)
{
	image.data = malloc(glwidth*glheight*3);
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, image.data);
}

static void SCR_ApplyGamma(qboolean BGR)
{
	int		i;
	byte	temp;

	if (
#ifdef USE_HWGAMMA
	!vid_hwgamma_enabled &&
#endif 
	!BGR)
		return;

	for (i=0 ; i < glwidth*glheight*3 ; i+=3) {
#ifdef USE_HWGAMMA
// gamma ramp effect
		if (vid_hwgamma_enabled){
			image.data[i  ] = (ramps[0][image.data[i  ]])/256;
			image.data[i+1] = (ramps[1][image.data[i+1]])/256;
			image.data[i+2] = (ramps[2][image.data[i+2]])/256;
		}
#endif
// swap rgb to bgr
		if (BGR) {
		temp = image.data[i];
		image.data[i] = image.data[i+2];
		image.data[i+2] = temp;
		}
	}
}

/*
================== 
SCR_ScreenShot
================== 
*/
qboolean OnScreenShotFormatChange (cvar_t *var, const char *value)
{
	float newvalue = Q_atof(value);

	if ((int)newvalue == 0) 
		return false;

#ifdef __USE_PNG__
	else if ((int)newvalue == 1) 
		return false;
#endif

#ifdef __USE_JPG__
	else if ((int)newvalue == 2) 
		return false;
#endif

	Con_Printf ("Invalid screenshot format\n");
	return true;
}

void SCR_ScreenShotTGA(char *filename)
{
	byte	buf[18];
	FILE*	fp;

	if (!(fp = fopen (filename, "wb")))
			Sys_Error ("Error opening %s", filename);

	memset (buf, 0, 18);
	buf[2] = 2;			// uncompressed type
	buf[12] = glwidth&255;
	buf[13] = glwidth>>8;
	buf[14] = glheight&255;
	buf[15] = glheight>>8;
	buf[16] = 24;		// pixel size

	SCR_GetPixels();
	SCR_ApplyGamma(true);

	fwrite (buf, 1, 18, fp);
	fwrite (image.data, 1, glwidth*glheight*3, fp);
	fclose (fp);
	free (image.data);
}

#ifdef __USE_PNG__
qboolean OnPNGCompressionChange (cvar_t *var, const char *value)
{
	int newvalue = (int)Q_atof(value);

	if (newvalue >= 0 && newvalue <= 9) 
		return false;

	Con_Printf ("Invalid PNG compression level\n");
	return true;
}

cvar_t	gl_screenshot_png_compression = {"gl_screenshot_png_compression", "0", 0, OnPNGCompressionChange};

//fuh : png screenshots 
void SCR_ScreenShotPNG(char *filename) {
	char	name[MAX_OSPATH];
	int		i;
	FILE	*fp;
	png_structp		png_ptr;
	png_infop		info_ptr;
	png_byte		**row_pointers;

	if (!(fp = fopen (filename, "wb"))) {
		COM_CreatePath (name);
		if (!(fp = fopen (filename, "wb")))
			Sys_Error ("Error opening %s", filename);
	}

	if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		fclose(fp);
		return;
	}

	if (!(info_ptr = png_create_info_struct(png_ptr))) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fclose(fp);
		return;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return;
	} 

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, glwidth, glheight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	if (gl_screenshot_png_compression.value)
		png_set_compression_level(png_ptr, gl_screenshot_png_compression.value);

	png_write_info(png_ptr, info_ptr);

	row_pointers = malloc (4 * glheight);

	SCR_GetPixels();
	SCR_ApplyGamma(false);

	for (i = 0; i < glheight; i++)
		row_pointers[glheight - i - 1] = image.data + i * glwidth * 3;
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	free(image.data);
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}
#endif

#ifdef __USE_JPG__
qboolean OnJPGQualityChange (cvar_t *var, const char *value)
{
	int newvalue = (int)Q_atof(value);

	if (newvalue >= 0 && newvalue <= 100) 
		return false;

	Con_Printf ("Invalid JPEG quality\n");
	return true;
}

cvar_t	gl_screenshot_jpeg_quality = {"gl_screenshot_jpeg_quality", "0", 0, OnJPGQualityChange};

void SCR_ScreenShotJPG(char *filename)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	FILE	*fp;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if (!(fp = fopen (filename, "wb"))) {
		COM_CreatePath (filename);
		if (!(fp = fopen (filename, "wb")))
			Sys_Error ("Error opening %s", filename);
	}

	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = glwidth;
	cinfo.image_height = glheight;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	if (gl_screenshot_jpeg_quality.value)
		jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality.value, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	SCR_GetPixels();
	SCR_ApplyGamma(false);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = image.data + (glheight - 1 - cinfo.next_scanline) * glwidth * 3;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(fp);
	free (image.data);
}
#endif

// screenshot extensions
static char	*ext[3] = {".tga", ".png", ".jpg"};
/*
================== 
SCR_MovieShot_f
================== 
*/
void SCR_MovieShot_f (void) 
{
	char	fname[MAX_OSPATH]; 
	int		extn;
	
	extn = gl_screenshot_format.value;

	sprintf(fname, "%s%06ld%s", capture_path, capture_num, ext[extn]);

	if (++capture_num == 1000000) {
		Con_Printf ("Too many screenshots taken\n"); 
		capture_num = -1; // stop capture
		return;
	}

	switch (extn){
#ifdef __USE_PNG__
	case 1:
		SCR_ScreenShotPNG(fname);
		break;
#endif
#ifdef __USE_JPG__
	case 2:
		SCR_ScreenShotJPG(fname);
		break;
#endif
	default:
		SCR_ScreenShotTGA(fname);
		break;
	}
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
	int				extn;

	extn = gl_screenshot_format.value;

	Q_snprintfz (fname, sizeof(name), "%s/screenshots/", com_gamedir);
	COM_CreatePath (fname);

	if (Cmd_Argc() == 2) {
		strncpy (name, Cmd_Argv(1), sizeof(name));
		if (!COM_CheckFilename(name)) {
			Con_Printf("Invalid name for screenshot\n");
			return;
		}
		COM_ForceExtension (name, ext[extn]);
		Q_snprintfz (fname, sizeof(fname), "%s/screenshots/%s", com_gamedir, name);
	} else {
// 
// find a file name to save it to 
// 
		for (i=0 ; i<=9999 ; i++) { 
			Q_snprintfz (fname, sizeof(fname), "%s/screenshots/quake%.4i%s", com_gamedir, i, ext[extn]);
			if (!Sys_FileExists(fname)) {
				Q_snprintfz (name, sizeof(name), "quake%.4i%s", i, ext[extn]);
				break;  // file doesn't exist
			}
		} 
		if (i==10000) {
			Con_Printf ("Too many screenshots taken\n"); 
			return;
		}
	}

	switch (extn){
#ifdef __USE_PNG__
	case 1:
		SCR_ScreenShotPNG(fname);
		break;
#endif
#ifdef __USE_JPG__
	case 2:
		SCR_ScreenShotJPG(fname);
		break;
#endif
	default:
		SCR_ScreenShotTGA(fname);
		break;
	}

	Con_Printf ("Wrote %s\n", name);
}

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
	byte		*pack;
	  
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

	data += rowbytes * (height - 1);

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
		data -= rowbytes * 2;
	}
			
// write the palette
	*pack++ = 0x0c;	// palette ID byte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;
		
// write output file 
	length = pack - (byte *)pcx;

	if (upload)
		CL_StartUpload((void *)pcx, length);
	else
		COM_WriteFile (filename, pcx, length);
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

// from gl_draw.c
byte		*draw_chars;				// 8*8 graphic characters

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
			if (source[x] != 255) {
				if(source[x])
					dest[x] = source[x];
				else
					dest[x] = 98;
			}
		source += 128;
		dest -= width;
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
	int     x, y;
	unsigned char		*src, *dest;
	char		pcxname[80]; 
	unsigned char		*newbuf;
	int w, h;
	int dx, dy, dex, dey, nx;
	int r, b, g;
	int count;
	float fracw, frach;
	char st[80];
	time_t now;

	if (CL_IsUploading())
		return; // already one pending

	if (cls.state < ca_onserver)
		return; // gotta be connected

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
	newbuf = malloc(glheight * glwidth * 3);

	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, newbuf ); 

	w = (vid.width < RSSHOT_WIDTH) ? glwidth : RSSHOT_WIDTH;
	h = (vid.height < RSSHOT_HEIGHT) ? glheight : RSSHOT_HEIGHT;

	fracw = (float)glwidth / (float)w;
	frach = (float)glheight / (float)h;

	for (y = 0; y < h; y++) {
		dest = newbuf + (w*3 * y);

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
				src = newbuf + (glwidth * 3 * dy) + dx * 3;
				for (nx = dx; nx < dex; nx++) {
					r += *src++;
					g += *src++;
					b += *src++;
					count++;
				}
			}
			r /= count;
			g /= count;
			b /= count;
			*dest++ = r;
			*dest++ = g;
			*dest++ = b;
		}
	}

	// convert to eight bit
	for (y = 0; y < h; y++) {
		src = newbuf + (w * 3 * y);
		dest = newbuf + (w * y);

		for (x = 0; x < w; x++) {
			*dest++ = MipColor(src[0], src[1], src[2]);
			src += 3;
		}
	}

	time(&now);
	strcpy(st, ctime(&now));
	st[strlen(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, h - 1, w);

	strncpy(st, cls.servername, sizeof(st));
	st[sizeof(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, h - 11, w);

	strncpy(st, name.string, sizeof(st));
	st[sizeof(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - strlen(st)*8, h - 21, w);

	WritePCXfile (pcxname, newbuf, w, h, w, host_basepal, true);

	free(newbuf);

	Con_Printf ("Sending shot to server...\n");
//	Con_Printf ("Wrote %s\n", pcxname);
}

//=============================================================================

void SCR_TileClear (void)
{
	if (r_refdef.vrect.x > 0) {
		// left
		Draw_TileClear (0, 0, r_refdef.vrect.x, vid.conheight - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width, 0, 
			vid.conwidth - r_refdef.vrect.x + r_refdef.vrect.width, 
			vid.conheight - sb_lines);
	}
	if (r_refdef.vrect.y > 0) {
		// top
		Draw_TileClear (r_refdef.vrect.x, 0, 
			r_refdef.vrect.x + r_refdef.vrect.width, 
			r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
			r_refdef.vrect.y + r_refdef.vrect.height, 
			r_refdef.vrect.width, 
			vid.conheight - sb_lines - 
			(r_refdef.vrect.height + r_refdef.vrect.y));
	}
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
extern qboolean	r_fov_greater_than_90;
static void SCR_CalcRefdef (void)
{
	float		size;
	int 		h;
	qboolean	full = false;


	scr_fullupdate = 0; // force a background redraw
	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

//========================================


// intermission is always full screen   
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0;           // no status bar at all
	else if (size >= 110 || cl_sbar_inv.value == 0) // BorisU
		sb_lines = 24;          // no inventory
	else
		sb_lines = 24+16+8;

	if (scr_viewsize.value >= 100.0) {
		full = true;
		size = 100.0;
	} else
		size = scr_viewsize.value;
	if (cl.intermission)
	{
		full = true;
		size = 100.0;
		sb_lines = 0;
	}
	size /= 100.0;

	if (!cl_sbar.value && full)
		h = vid.conheight;
	else
		h = vid.conheight - sb_lines;

	r_refdef.vrect.width = vid.conwidth * size;
	if (r_refdef.vrect.width < 96)
	{
		size = 96.0 / r_refdef.vrect.width;
		r_refdef.vrect.width = 96;      // min for icons
	}

	r_refdef.vrect.height = vid.conheight * size;
	if (cl_sbar.value || !full) {
		if (r_refdef.vrect.height > vid.conheight - sb_lines)
			r_refdef.vrect.height = vid.conheight - sb_lines;
	} else if (r_refdef.vrect.height > vid.conheight)
			r_refdef.vrect.height = vid.conheight;
	r_refdef.vrect.x = (vid.conwidth - r_refdef.vrect.width)/2;
	if (full)
		r_refdef.vrect.y = 0;
	else 
		r_refdef.vrect.y = (h - r_refdef.vrect.height)/2;

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	r_fov_greater_than_90 = scr_fov.value > 90.0f;

	scr_vrect = r_refdef.vrect;
}

// BorisU -->
void SCR_DrawSpeedometer (void)
{
	int x, y, length, charsize, speed, width;
	float scale, red, green, blue;

	if(show_speed.value < 2) return;

	speed = display_speed;
	charsize = (float)vid.height / (float)vid.conheight * 8;
	length = vid.conwidth / 3;
	scale = length / 1000.0;
	x = (vid.conwidth - length) / 2;
	y = vid.conheight - sb_lines - charsize - 1;
	width = (speed % 1000) * scale;

	// draw the coloured indicator strip
	glDisable(GL_TEXTURE_2D);
	if(speed <= 300) red = 0, green = speed / 600.0, blue = 0;
	else if(speed <= 600) red = 0, green = (600 - speed) / 600.0,
			blue = (speed - 300) / 400.0;
	else red = (speed > 900 ? 1 : (speed - 600) / 400.0), green = 0,
			blue = (speed > 900 ? 0 : (900 - speed) / 400.0);
	glColor3f(red, green, blue);
	glBegin(GL_QUADS);
	glVertex2f (x, y);
	glVertex2f (x + width, y);
	glVertex2f (x + width, y + charsize);
	glVertex2f (x, y + charsize);
	glEnd();
	glEnable (GL_TEXTURE_2D);

	// now draw the border
	glColor3f(1, 1, 0);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_BACK, GL_LINE);
	glRecti(x, y, x + length, y + charsize);
	glBegin(GL_LINES);
	glVertex2f(x + length / 4, y);
	glVertex2f(x + length / 4, y + charsize / 4);
	glVertex2f(x + length / 4, y + charsize * 3 / 4 + 1);
	glVertex2f(x + length / 4, y + charsize);

	glVertex2f(vid.conwidth / 2, y);
	glVertex2f(vid.conwidth / 2, y + charsize * 3 / 8);
	glVertex2f(vid.conwidth / 2, y + charsize);
	glVertex2f(vid.conwidth / 2, y + charsize * 5 / 8);

	glVertex2f(x + length * 3 / 4, y);
	glVertex2f(x + length * 3 / 4, y + charsize / 4);
	glVertex2f(x + length * 3 / 4, y + charsize * 3 / 4 + 1);
	glVertex2f(x + length * 3 / 4, y + charsize);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_BACK, GL_FILL);
	glColor3ubv (color_white);

	if(show_speed.value > 2)
	{
		Draw_String(x + 1, y - 1, Hud_SpeedStr());
	}
}

typedef struct scr_name_s {
	double	x, y;
	char	name[MAX_SCOREBOARDNAME];
	char	info[16];
} scr_name_t;

int			scr_numplayers;
scr_name_t	scr_players[MAX_CLIENTS];

/*
==================
SCR_SetupDrawNames
==================
*/
void SCR_SetupDrawNames (void)
{
	int			i,j;
	entity_t	*cur_ent;

	GLdouble	modelMatrix[16], projectMatrix[16];
	GLint		viewport[4];
	GLdouble	winz;
	vec3_t		org;
	trace_t	trace;

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projectMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	VectorCopy (cl.simorg, org); org[2] += 16; // viewpoint

	scr_numplayers = 0;
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		cur_ent = &cl_visedicts[i];
		if (cur_ent->model->modhint == MOD_PLAYER) {
		
			if (R_CullSphere(cur_ent->origin,0))
				continue;
			if (cur_ent->frame >= 41 && cur_ent->frame <= 102)
				continue;
	
			trace = PM_TraceLine (org, cur_ent->origin);
			if (trace.fraction < 1)
					continue;

			gluProject(cur_ent->origin[0],cur_ent->origin[1], cur_ent->origin[2] + 28, 
				modelMatrix, projectMatrix, viewport, 
				&scr_players[scr_numplayers].x, &scr_players[scr_numplayers].y, &winz);
			
			strcpy(scr_players[scr_numplayers].name, cur_ent->scoreboard->name);
			if (cl_spec_id.value > 2 && cls.mvdplayback) {
				for (j = 0; i < MAX_CLIENTS; j++) { // FIXME: is there a better way?
					if (cl.players[j].name[0] && 
						!strncmp(cur_ent->scoreboard->name, cl.players[j].name, MAX_SCOREBOARDNAME - 1))
						break;
				}

				sprintf(scr_players[scr_numplayers].info, "%ix%i",
					cl.players[j].stats[STAT_ARMOR],cl.players[j].stats[STAT_HEALTH]);
			}
			++scr_numplayers;
		}
	}
}
/*
==================
SCR_DrawNames
==================
*/
void SCR_DrawNames (void)
{
	int			i, x, y;
	scr_name_t	*p;

	for (i=0; i<scr_numplayers; i++) {
		p = scr_players + i;
		x = vid.conwidth*(p->x)/glwidth; // FIX ME! window mode!
		y = vid.conheight*(glheight - p->y)/glheight;
		//Draw_Fill(x, y, 1, 1, 79);
		if (cl_spec_id.value > 2 && cls.mvdplayback) {
			Draw_String(x - strlen(p->name)*4, y-12, p->name);
			Draw_String(x - strlen(p->info)*4, y-4, p->info);
		} else {
			Draw_String(x - strlen(p->name)*4, y-8, p->name);
		}

	}
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
	if (block_drawing)
		return;

	vid.numpages = 2 + gl_triplebuffer.value;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_disabled_for_loading)
	{
		if (cls.realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;                         // not initialized yet


	if (oldsbar != cl_sbar.value || oldsbarinv != cl_sbar_inv.value) {
		oldsbar = cl_sbar.value;
		oldsbarinv = cl_sbar_inv.value;
		vid.recalc_refdef = true;
	}

	//
	// determine size of refresh window
	//

	if (vid.recalc_refdef)
		SCR_CalcRefdef ();

	if (v_contrast.value > 1 
#ifdef USE_HWGAMMA		
		&& !vid_hwgamma_enabled
#endif
		) {
		// scr_fullupdate = true;
		Sbar_Changed ();
	}

//
// do 3D refresh drawing, and then update the screen
//
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	SCR_SetUpToDrawConsole ();
	
	V_RenderView ();

	if ((cls.demoplayback || cl.spectator) && cl_spec_id.value > 1 && 
		cls.state == ca_active && cl.validsequence)
		SCR_SetupDrawNames ();

	GL_Set2D ();

	if ((cls.demoplayback || cl.spectator) && cl_spec_id.value > 1 && 
		cls.state == ca_active && cl.validsequence)
		SCR_DrawNames ();

	R_PolyBlend ();
	
	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear ();

	if (r_netgraph.value)
		R_NetGraph ();

	if (scr_drawdialog)
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
		scr_copyeverything = true;
	}
	else if (scr_drawloading)
	{
		SCR_DrawLoading ();
		Sbar_Draw ();
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
		if (crosshair.value)
			Draw_Crosshair();
		
		SCR_DrawRam ();
		SCR_DrawNet ();
		SCR_DrawFPS ();
		SCR_DrawLagmeter ();
		SCR_DrawSpeedometer ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		if (show_speed.value == 1) Sbar_Changed(); // forcing sbar redraw. BorisU
		Sbar_Draw ();
		SCR_CheckDrawCenterString ();
		SCR_DrawDemoClock (); // fuh
		SCR_DrawHud ();
		SCR_DrawConsole ();
		M_Draw ();
	}
	

	R_BrightenScreen ();

	V_UpdatePalette ();

	GL_EndRendering ();
}
