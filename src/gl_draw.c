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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include <ctype.h>

extern cvar_t crosshair, cl_crossx, cl_crossy, crosshaircolor;
extern cvar_t cl_cross_size, cl_cross_width; // BorisU

int	gl_max_size_hardware; // BorisU

qboolean OnChange_gl_smoothfont (cvar_t *var, const char *string);
qboolean OnChange_gl_max_size (cvar_t *var, const char *string);

cvar_t		gl_nobind		= {"gl_nobind", "0"};
cvar_t		gl_max_size		= {"gl_max_size", "1024", 0, OnChange_gl_max_size};
cvar_t		gl_conalpha		= {"gl_conalpha", "0.8"};
cvar_t		gl_smoothfont	= {"gl_smoothfont", "0", 0, OnChange_gl_smoothfont};
cvar_t		scr_coloredText = {"scr_coloredText", "1"}; // fuh

byte		*draw_chars;				// 8*8 graphic characters
mpic_t		*draw_disc;
mpic_t		*draw_backtile;

int			translate_texture;
int			char_texture;
int			cs_texture;  // crosshair texture
// BorisU
int			cs3_texture; // crosshair texture 
int			crosshair_texture; // TGA crosshair texture

static byte cs_data[64] = {
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// BorisU -->
// crosshair 3
static byte cs3_data[64] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// <-- BorisU

int GL_LoadPicTexture (char *name, mpic_t *pic, byte *data);

mpic_t	conback_data;
mpic_t	*conback = &conback_data;


void GL_Bind (int texnum)
{
	if (gl_nobind.value)
		texnum = char_texture;
	if (currenttexture == texnum)
		return;
	currenttexture = texnum;
#ifdef _WIN32
	bindTexFunc (GL_TEXTURE_2D, texnum);
#else
	glBindTexture (GL_TEXTURE_2D, texnum);
#endif
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

// some cards have low quality of alpha pics, so load the pics
// without transparent pixels into a different scrap block.
// scrap 0 is solid pics, 1 is transparent
#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
int			scrap_dirty = 0;	// bit mask
int			scrap_texnum;

// returns false if allocation failed
qboolean Scrap_AllocBlock (int scrapnum, int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	if (scrapnum) w+=2; // BorisU: hack for better sbar font drawing

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;
		
		for (j=0 ; j<w ; j++)
		{
			if (scrap_allocated[scrapnum][i+j] >= best)
				break;
			if (scrap_allocated[scrapnum][i+j] > best2)
				best2 = scrap_allocated[scrapnum][i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}
	
	if (best + h > BLOCK_HEIGHT)
		return false;
	
	for (i=0 ; i<w ; i++)
		scrap_allocated[scrapnum][*x + i] = best + h;

	scrap_dirty |= (1 << scrapnum);

	return true;
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	int i;

	scrap_uploads++;
	for (i=0 ; i<2 ; i++) {
		if ( !(scrap_dirty & (1 << i)) )
			continue;
		scrap_dirty &= ~(1 << i);
		GL_Bind(scrap_texnum + i);
		GL_Upload8 (scrap_texels[i], BLOCK_WIDTH, BLOCK_HEIGHT, false, i, false);
	}
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	mpic_t		pic;
	qboolean	valid;
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	cachepics[MAX_CACHED_PICS];
int			numcachepics;

byte		menuplyr_pixels[4096];

int		pic_texels;
int		pic_count;

/*
ideally all mpic_t's should have a flushcount field that would be
checked against a global gl_flushcount; the pic would be reloaded
if the two don't match.
currently, the pics are only updated when Draw_CachePic is called,
and wad pics are never updated.
*/
void Draw_FlushCache (void)
{
	int i;

	if (!draw_chars)
		return;		// not initialized yet

	for (i = 0; i < numcachepics; i++)
		cachepics[i].valid = false;		// force it to be reloaded

	// I hope this doesn't cause texture memory leaks
//	Draw_LoadConback ();
//	Draw_LoadCharset ();
}

mpic_t *Draw_CacheWadPic (char *name)
{
	qpic_t	*p;
	mpic_t	*pic;
	int		texnum;
	
	p = W_GetLumpName (name);
	pic = (mpic_t *)p;

	texnum = loadtexture_24bit(name, LOADTEX_GFX);
	if (texnum){
		pic->texnum = texnum;
		pic->sl = 0;
		pic->sh = 1;
		pic->tl = 0;
		pic->th = 1;
		return pic;
	}

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int		x, y;
		int		i, j, k;

		texnum = memchr(p->data, 255, p->width*p->height) != NULL;
		if (!Scrap_AllocBlock (texnum, p->width, p->height, &x, &y)) {
			GL_LoadPicTexture (name, pic, p->data);
			return pic;
		}
		k = 0;
		for (i=0 ; i<p->height ; i++)
			for (j=0 ; j<p->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = p->data[k];
		texnum += scrap_texnum;
		pic->texnum = texnum;
		pic->sl = (x+0.01)/(float)BLOCK_WIDTH;
		pic->sh = (x+p->width-0.01)/(float)BLOCK_WIDTH;
		pic->tl = (y+0.01)/(float)BLOCK_WIDTH;
		pic->th = (y+p->height-0.01)/(float)BLOCK_WIDTH;

		pic_count++;
		pic_texels += p->width*p->height;
	} else 
		GL_LoadPicTexture (name, pic, p->data);

	return pic;
}

/*
================
Draw_CachePic
================
*/
mpic_t *Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	int		texnum;

	for (pic=cachepics, i=0 ; i<numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name) && pic->valid)
			return &pic->pic;

	if (numcachepics == MAX_CACHED_PICS)
		Sys_Error ("numcachepics == MAX_CACHED_PICS");
	numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path);
	if (!dat)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	pic->valid = true;

	texnum = loadtexture_24bit(path, LOADTEX_GFX);
	if (texnum){
		pic->pic.texnum = texnum;
		pic->pic.sl = 0;
		pic->pic.sh = 1;
		pic->pic.tl = 0;
		pic->pic.th = 1;
		return &pic->pic;
	}

	GL_LoadPicTexture (path, &pic->pic, dat->data);

	return &pic->pic;
}

void Remove_Transparent (byte* data, int size)
{
	byte *p;

	for (p = data; p<data+size; p++)
		if (*p == 255)
			*p = 105;
}

void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--) {
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}

qboolean OnChange_gl_smoothfont (cvar_t *var, const char *string)
{
	float	newval;

	newval = Q_atof (string);
	if (!newval == !gl_smoothfont.value || !char_texture)
		return false;

	GL_Bind(char_texture);
	if (newval) {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	return false;
}

void Draw_LoadCharset (void)
{
	int i;
	char	buf[128*256];
	char	*src, *dest;

	draw_chars = W_GetLumpName ("conchars");
	for (i=0 ; i < 256 * 64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// Convert the 128*128 conchars texture to 128*256 leaving
	// empty space between rows so that chars don't stumble on
	// each other because of texture smoothing.
	// This hack costs us 64K of GL texture memory
	memset (buf, 255, sizeof(buf));
	src = draw_chars;
	dest = buf;
	for (i=0 ; i<16 ; i++) {
		memcpy (dest, src, 128*8);
		src += 128*8;
		dest += 128*8*2;
	}
	
	char_texture = loadtexture_24bit ("charset", LOADTEX_CHARS_MAIN);
	if (char_texture == 0)
		char_texture = GL_LoadTexture ("charset", 128, 256, buf, false, true, false, 1);
	if (!gl_smoothfont.value) {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
}

qboolean OnChange_gl_max_size (cvar_t *var, const char *string)
{
	int		newval,i;

	newval = Q_atoi (string);

	if (newval < 256 || newval == gl_max_size.value) 
		return true;

	for (i=0; i<MAX_CLIENTS; i++)
		skinslots[i].mip = -1;

	for (i=256; i<=gl_max_size_hardware; i *= 2) {
		if (i >= newval) {
			Cvar_SetValue(&gl_max_size, i);
			return true;
		}
	}
	Cvar_SetValue(&gl_max_size, gl_max_size_hardware);
	return true;
}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	qpic_t	*cb;
	byte	*dest;
	int		x;
	char	ver[40];
	int		start;
	mpic_t	*gl;

	// BorisU: init transparent scrap
	memset(scrap_texels[1], 255, BLOCK_WIDTH*BLOCK_HEIGHT*4);

	Cvar_RegisterVariable (&gl_nobind);
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_conalpha);
	Cvar_RegisterVariable (&gl_smoothfont);
	Cvar_RegisterVariable (&scr_coloredText);

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_max_size_hardware);
	Con_Printf("Max texture size: %d\n",gl_max_size_hardware);
	Cvar_Set(&gl_max_size, va("%d",gl_max_size_hardware));

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture

	Draw_LoadCharset ();

	start = Hunk_LowMark ();

	cb = (qpic_t *)COM_LoadHunkFile ("gfx/conback.lmp");	
	if (!cb)
		Sys_Error ("Couldn't load gfx/conback.lmp");
	SwapPic (cb);

	if (cb->width != 320 || cb->height != 200)
		Sys_Error ("Draw_Init: conback.lmp size is not 320x200");

	Remove_Transparent(cb->data,200*320);
	
	sprintf (ver, "%4.2f", VERSION);

	dest = cb->data + 320 + 320*186 - 11 - 8*strlen(ver);

	for (x=0 ; x<strlen(ver) ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));

	conback->width = cb->width;
	conback->height = cb->height;
	gl = (mpic_t *) conback;
	gl->texnum = loadtexture_24bit ("conback", LOADTEX_CONBACK);
	if (gl->texnum == 0)
		GL_LoadPicTexture ("conback", conback, cb->data);
	else {
		gl->sl = 0;
		gl->sh = 1;
		gl->tl = 0;
		gl->th = 1;
	}
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	// free loaded console
	Hunk_FreeToLowMark (start);

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	// Load the crosshair pics
	cs3_texture = GL_LoadTexture ("crosshair3", 8, 8, cs3_data, false, true, false, 1); // BorisU
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	cs_texture = GL_LoadTexture ("crosshair", 8, 8, cs_data, false, true, false, 1);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//
	// get the other pics we need
	//
	draw_disc = Draw_CacheWadPic ("disc");
	draw_backtile = Draw_CacheWadPic ("backtile");
}

inline static void Draw_CharPoly(int x, int y, int num)
{
	float frow, fcol;

	frow = (num >> 4) * 0.0625;
	fcol = (num & 15) * 0.0625;

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + 0.0625, frow);
	glVertex2f (x + 8, y);
	glTexCoord2f (fcol + 0.0625, frow + 0.03125);
	glVertex2f (x + 8, y + 8);
	glTexCoord2f (fcol, frow + 0.03125);
	glVertex2f (x, y + 8);
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, int num)
{
	if (num == 32)
		return;		// space

	num &= 255;
	
	if (y <= -8)
		return;		// totally off screen

	GL_Bind (char_texture);

	glBegin (GL_QUADS);
	Draw_CharPoly(x, y, num);
	glEnd ();
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	int num;

	if (y <= -8)
		return;			// totally off screen
	
	if (!str || !str[0])
		return;

	GL_Bind (char_texture);

	glBegin (GL_QUADS);

	while (*str) {	// stop rendering when out of characters		
		if ((num = *str++) != 32)	// skip spaces
			Draw_CharPoly(x, y, num);

		x += 8;
	}

	glEnd ();
}

/*
================
Draw_AlphaString
================
*/
void Draw_AlphaString (int x, int y, char *str, float alpha)
{
	int num;

	alpha = bound (0, alpha, 1);
	if (!alpha)
		return;

	if (y <= -8)
		return;			// totally off screen
	
	if (!str || !str[0])
		return;

	glColor4f (1, 1, 1, alpha);

	GL_Bind (char_texture);

	glBegin (GL_QUADS);

	while (*str) {	// stop rendering when out of characters		
		if ((num = *str++) != 32)	// skip spaces
			Draw_CharPoly(x, y, num);

		x += 8;
	}

	glEnd ();
	glColor3ubv (color_white);
}

/*
================
Draw_Alt_String
================
*/
void Draw_Alt_String (int x, int y, char *str)
{
	int num;

	if (y <= -8)
		return;			// totally off screen
	if (!str || !str[0])
		return;

	GL_Bind (char_texture);

	glBegin (GL_QUADS);

	while (*str) {// stop rendering when out of characters
		if ((num = *str++ | 128) != (32 | 128))	// skip spaces
			Draw_CharPoly(x, y, num);

		x += 8;
	}

	glEnd ();
}

// fuh -->
static int HexToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	else if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	else
		return -1;
}

void Draw_ColoredString (int x, int y, char *text, int red)
{
	int r, g, b, num;
	qboolean white = true;

	if (y <= -8)
		return;			// totally off screen

	if (!*text)
		return;

	GL_Bind (char_texture);

	if (scr_coloredText.value)
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glBegin (GL_QUADS);

	for ( ; *text; text++) {
		
		if (*text == '&') {
			if (text[1] == 'c' && text[2] && text[3] && text[4]) {
				r = HexToInt(text[2]);
				g = HexToInt(text[3]);
				b = HexToInt(text[4]);
				if (r >= 0 && g >= 0 && b >= 0) {
					if (scr_coloredText.value) {
						glColor3f(r / 16.0, g / 16.0, b / 16.0);
						white = false;
					}
					text += 5;
				}
			} else if (text[1] == 'r') {
				if (!white) {
					glColor3ubv(color_white);
					white = true;
				}
				text += 2;
			}
		}

		num = *text & 255;
		if (!scr_coloredText.value && red)
			num |= 128;

		if (num != 32 && num != (32 | 128))
			Draw_CharPoly(x, y, num);

		x += 8;
	}

	glEnd ();

	if (!white)
		glColor3ubv(color_white);

	if (scr_coloredText.value)
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
// <-- fuh

void Draw_Crosshair(void)
{
	float x, y;
	extern vrect_t		scr_vrect;
	unsigned char *pColor;

	x = scr_vrect.x + scr_vrect.width/2 + cl_crossx.value; 
	y = scr_vrect.y + scr_vrect.height/2 + cl_crossy.value;

	if (crosshair.value > 1) {
		pColor = (unsigned char *) &d_8to24table[(byte) crosshaircolor.value];
		glColor4ubv ( pColor );
	}

	if (crosshair.value == 2) {
		x -= 3;
		y -= 3;

		GL_Bind (cs_texture);

		glBegin (GL_QUADS);
		glTexCoord2f (0, 0);
		glVertex2f (x - 4, y - 4);
		glTexCoord2f (1, 0);
		glVertex2f (x+12, y-4);
		glTexCoord2f (1, 1);
		glVertex2f (x+12, y+12);
		glTexCoord2f (0, 1);
		glVertex2f (x - 4, y+12);
		glEnd ();
	} else if (crosshair.value == 3) {
		x -= 3;
		y -= 3;

		GL_Bind (cs3_texture);

		glBegin (GL_QUADS);
		glTexCoord2f (0, 0);
		glVertex2f (x - 4, y - 4);
		glTexCoord2f (1, 0);
		glVertex2f (x+12, y-4);
		glTexCoord2f (1, 1);
		glVertex2f (x+12, y+12);
		glTexCoord2f (0, 1);
		glVertex2f (x - 4, y+12);
		glEnd ();
	} else if (crosshair.value == 4) {
		int width, shift, correct; // BorisU
		byte c = (byte)crosshaircolor.value;
		width = cl_cross_width.value;
		shift = width/2;
		correct = 1-(width&1);
		Draw_Fill(x-shift, y - cl_cross_size.value -correct, width, cl_cross_size.value*2+2-(width&1), c);
		Draw_Fill(x-cl_cross_size.value -correct, y-shift, cl_cross_size.value*2+2-(width&1), width, c);

	} else if (crosshair.value == 5 && crosshair_texture) {
		float	xsize,ysize;

		xsize = ysize = cl_cross_size.value;

		GL_Bind(crosshair_texture);

		glBegin (GL_QUADS);
		glTexCoord2f (0, 0);
		glVertex2f (x-xsize, y-ysize);
		glTexCoord2f (1, 0);
		glVertex2f (x+xsize, y-ysize);
		glTexCoord2f (1,1);
		glVertex2f (x+xsize, y+ysize);
		glTexCoord2f (0,1);
		glVertex2f (x-xsize, y+ysize);
		glEnd ();
		
	} else if (crosshair.value)
		Draw_Character (x-4, y-4,'+');
	
	if (crosshair.value > 0)
		glColor3ubv (color_white);
}

/*
================
Draw_TextBox
================
*/
void Draw_TextBox (int x, int y, int width, int lines)
{
	mpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	Draw_TransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		Draw_TransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	Draw_TransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		Draw_TransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			Draw_TransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		Draw_TransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	Draw_TransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		Draw_TransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	Draw_TransPic (cx, cy+8, p);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, mpic_t *pic)
{
	if (scrap_dirty)
		Scrap_Upload ();
	GL_Bind (pic->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (pic->sl, pic->tl);
	glVertex2f (x, y);
	glTexCoord2f (pic->sh, pic->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (pic->sh, pic->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (pic->sl, pic->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, mpic_t *pic, float alpha)
{
	if (scrap_dirty)
		Scrap_Upload ();
//	glDisable(GL_ALPHA_TEST);
//	glEnable (GL_BLEND);
	glCullFace(GL_FRONT);
	glColor4f (1, 1, 1, alpha);
	GL_Bind (pic->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (pic->sl, pic->tl);
	glVertex2f (x, y);
	glTexCoord2f (pic->sh, pic->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (pic->sh, pic->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (pic->sl, pic->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
	glColor3ubv (color_white);
//	glEnable(GL_ALPHA_TEST);
//	glDisable (GL_BLEND);
}

void Draw_SubPic(int x, int y, mpic_t *pic, int srcx, int srcy, int width, int height)
{
	float newsl, newtl, newsh, newth;
	float oldglwidth, oldglheight;

	if (scrap_dirty)
		Scrap_Upload ();
	
	oldglwidth = pic->sh - pic->sl;
	oldglheight = pic->th - pic->tl;

	newsl = pic->sl + (srcx*oldglwidth)/pic->width;
	newsh = newsl + (width*oldglwidth)/pic->width;

	newtl = pic->tl + (srcy*oldglheight)/pic->height;
	newth = newtl + (height*oldglheight)/pic->height;
	
	GL_Bind (pic->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (newsl, newtl);
	glVertex2f (x, y);
	glTexCoord2f (newsh, newtl);
	glVertex2f (x+width, y);
	glTexCoord2f (newsh, newth);
	glVertex2f (x+width, y+height);
	glTexCoord2f (newsl, newth);
	glVertex2f (x, y+height);
	glEnd ();
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, mpic_t *pic)
{
	if (x < 0 || (unsigned)(x + pic->width) > vid.conwidth || y < 0 ||
		 (unsigned)(y + pic->height) > vid.conheight)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, mpic_t *pic, byte *translation)
{
	int				v, u, c;
	unsigned		trans[64*64], *dest;
	byte			*src;
	int				p;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (1, 0);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (1, 1);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (0, 1);
	glVertex2f (x, y+pic->height);
	glEnd ();
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	char ver[80];
	int x, y;

	if (lines == vid.conheight)
		Draw_Pic(0, lines - vid.conheight, conback);
	else {
		Draw_AlphaPic (0, lines - vid.conheight, conback, gl_conalpha.value);
//		glEnable (GL_BLEND);
	}
		

	if (!cls.download) {
		sprintf (ver, "QW262 %d:%s", MINOR_VERSION, RELEASE);
		x = vid.conwidth - (strlen(ver)*8 + 11) ; //- (vid.conwidth*8/320)*7;
		y = lines-14;
		Draw_Alt_String (x, y, ver);
	}
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	GL_Bind (draw_backtile->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();
}

// fuh -->
void Draw_AlphaFill (int x, int y, int w, int h, int c, float alpha)
{
	alpha = bound(0, alpha, 1);

	if (!alpha)
		return;

	glDisable (GL_TEXTURE_2D);
	if (alpha < 1) {
//		glEnable (GL_BLEND);
//		glDisable(GL_ALPHA_TEST);
		glColor4f (host_basepal[c * 3] / 255.0,  host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2] / 255.0, alpha);
	} else {
		glColor3f (host_basepal[c * 3] / 255.0, host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2]  /255.0);
	}

	glBegin (GL_QUADS);
	glVertex2f (x, y);
	glVertex2f (x + w, y);
	glVertex2f (x + w, y + h);
	glVertex2f (x, y + h);
	glEnd ();

	glEnable (GL_TEXTURE_2D);
//	if (alpha < 1) {
//		glEnable(GL_ALPHA_TEST);
//		glDisable (GL_BLEND);
//	}
	glColor3ubv (color_white);
}
// <-- fuh

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	glDisable (GL_TEXTURE_2D);
	glColor3ubv (&host_basepal[c*3]);

	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor3ubv (color_white);
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, 0.8);
	glBegin (GL_QUADS);

	glVertex2f (0,0);
	glVertex2f (vid.conwidth, 0);
	glVertex2f (vid.conwidth, vid.conheight);
	glVertex2f (0, vid.conheight);

	glEnd ();
	glColor3ubv (color_white);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;
	glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.conwidth - 24, 0, draw_disc);
	glDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	glViewport (glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0, vid.conwidth, vid.conheight, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity ();

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glEnable (GL_ALPHA_TEST);
// not needed, it is default
//	glAlphaFunc(GL_GREATER, alpha_test_threshold);
	glEnable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glColor3ubv (color_white);
}

//====================================================================

