/*
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

#ifndef __GL_TEXTURE_H_

#define __GL_TEXTURE_H_

#define TEX_COMPLAIN		1
#define TEX_MIPMAP			(1<<1)
#define TEX_ALPHA			(1<<2)
#define TEX_LUMA			(1<<3)
#define TEX_FULLBRIGHT		(1<<4)
#define TEX_CHARSET			(1<<5)
#define TEX_NOSCALE			(1<<6)
#define TEX_BRIGHTEN		(1<<7)
#define TEX_NOCOMPRESS		(1<<8)

#define LOADTEX_BSP_TURB		(1<<1)
#define LOADTEX_BSP_NORMAL		(1<<2)
#define LOADTEX_SPRITE			(1<<3)
#define LOADTEX_ALIAS_MODEL		(1<<4)
#define LOADTEX_SKYBOX			(1<<5)
#define LOADTEX_GFX				(1<<6)
#define LOADTEX_CHARS			(1<<7)
#define LOADTEX_CONBACK			(1<<8)
#define LOADTEX_CAUSTIC			(1<<9)
#define LOADTEX_CROSSHAIR		(1<<10)
#define LOADTEX_QMB_PARTICLE	(1<<11)
#define LOADTEX_CHARS_MAIN		(1<<12)

typedef struct
{
	int				texnum;
	char			identifier[64];
	int				width, height;
	qboolean		mipmap;
	qboolean		brighten;
	unsigned short	crc;
	int				bytesperpixel;
	char			*pathname;
} gltexture_t;

void Init_GL_textures (void);

int loadtexture_24bit (char* name, int type);

int loadtextureimage (char* filename, int mode);

byte* loadimagepixels (char* filename, int mode);

extern int	fb_texnum;
extern int	luma_texnum;

extern cvar_t	gl_texturebits;

extern int		numgltextures;
extern int		base_numgltextures;

extern gltexture_t	*current_texture;
#endif	//__GL_TEXTURE_H_
