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


#define		TEXTURE0_SGIS				0x835E
#define		TEXTURE1_SGIS				0x835F

#define		GL_MAX_TEXTURES_SGIS		0x835D

#define		GL_TEXTURE0_ARB 			0x84C0
#define		GL_TEXTURE1_ARB 			0x84C1

#define		GL_MAX_TEXTURE_UNITS_ARB	0x84E2

typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
extern lpMTexFUNC MTexCoord2f;
extern lpSelTexFUNC SelectTextureMTex;

extern qboolean gl_mtexable;
extern GLenum oldtarget;

extern GLenum GL_MTexture0;
extern GLenum GL_MTexture1;

extern int oldtexture;
