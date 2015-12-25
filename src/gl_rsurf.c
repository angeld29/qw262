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
// r_surf.c: surface-related refresh code

#include "quakedef.h"

int		blacktexturenum;

int		lightmap_bytes;		// 1 or 3
int		gl_internal_lightmap_format;

int		lightmap_textures;

unsigned	blocklights[18*18*3];

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	64
int		active_lightmaps;

typedef struct glRect_s {
	unsigned char l,t,w,h;
} glRect_t;

glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
qboolean	lightmap_modified[MAX_LIGHTMAPS];
glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[4*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

// For gl_texsort 0
msurface_t	*skychain = NULL;
msurface_t	*waterchain = NULL;

// QF -->
msurface_t **waterchain_tail = &waterchain;
msurface_t **skychain_tail = &skychain;

#define CHAIN_SURF_F2B(surf,chain)				\
	{											\
		*(chain##_tail) = (surf);				\
		(chain##_tail) = &(surf)->texturechain;	\
		*(chain##_tail) = 0;					\
	}

#define CHAIN_SURF_B2F(surf,chain) 				\
	{											\
		(surf)->texturechain = (chain);			\
		(chain) = (surf);						\
	}

#if 1
# define CHAIN_SURF CHAIN_SURF_F2B
#else
# define CHAIN_SURF CHAIN_SURF_B2F
#endif
// <-- QF

glpoly_t	*caustics_polys; // BorisU

// BorisU ->
// support for luma textures
glpoly_t	*luma_polys[MAX_GLTEXTURES];
qboolean	drawluma = false;
// <-- BorisU

// Tonik -->
glpoly_t	*fullbright_polys[MAX_GLTEXTURES];
qboolean	drawfullbrights = false;

void DrawGLPoly (glpoly_t *p);

void R_RenderFullbrights (void)
{
	int			i;
	glpoly_t	*p;
	
	if ((!drawfullbrights && !drawluma) || !gl_fb_bmodels.value)
		return;

	GL_DisableMultitexture ();
	
	glDepthMask (GL_FALSE);	// don't bother writing Z
	if (gl_fb_depthhack.value)
	{
		float			depthdelta;
		extern cvar_t	gl_ztrick;
		extern int		gl_ztrickframe;

		if (gl_ztrick.value)
			depthdelta = gl_ztrickframe ? - 1.0/16384 : 1.0/16384;
		else
			depthdelta = -1.0/8192;
		
		// hack depth range to prevent flickering of fullbrights
		glDepthRange (gldepthmin + depthdelta, gldepthmax + depthdelta);
	}
	
	glEnable (GL_BLEND);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_ALPHA_TEST);
	for (i=1 ; i<MAX_GLTEXTURES ; i++) {
		if (!fullbright_polys[i])
			continue;
		GL_Bind (i);
		for (p = fullbright_polys[i]; p; p = p->fb_chain) {
			DrawGLPoly (p);
		}
		fullbright_polys[i] = NULL;
	}

	glBlendFunc(GL_ONE, GL_ONE);

// BorisU -->
	for (i=1 ; i<MAX_GLTEXTURES ; i++) {
		if (!luma_polys[i])
			continue;
		GL_Bind (i);
		for (p = luma_polys[i]; p; p = p->luma_chain) {
			DrawGLPoly (p);
		}
		luma_polys[i] = NULL;
	}
// <-- BorisU
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glDepthMask (GL_TRUE);
	if (gl_fb_depthhack.value)
		glDepthRange (gldepthmin, gldepthmax);

	drawluma = false;
	drawfullbrights = false;
}
// <-- Tonik

// Tonik -->
//=============================================================
// Dynamic lights

typedef struct dlightinfo_s {
	int local[2];
	int rad;
	int minlight;	// rad - minlight
	int type;
} dlightinfo_t;

static dlightinfo_t dlightlist[MAX_DLIGHTS];
static int	numdlights;

/*
===============
R_BuildDlightList
===============
*/
void R_BuildDlightList (msurface_t *surf)
{
	int			lnum;
	float		dist;
	vec3_t		impact;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	int			irad, iminlight;
	int			local[2];
	int			tdmin, sdmin, distmin;
	dlightinfo_t	*light;

	numdlights = 0;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		irad = (cl_dlights[lnum].radius - fabs(dist)) * 256;
		iminlight = cl_dlights[lnum].minlight * 256;
		if (irad < iminlight)
			continue;

		iminlight = irad - iminlight;
		
		for (i=0 ; i<3 ; i++) {
			impact[i] = cl_dlights[lnum].origin[i] -
				surf->plane->normal[i]*dist;
		}
		
		local[0] = DotProduct (impact, tex->vecs[0]) +
			tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct (impact, tex->vecs[1]) +
			tex->vecs[1][3] - surf->texturemins[1];
		
		// check if this dlight will touch the surface
		if (local[1] > 0) {
			tdmin = local[1] - (tmax<<4);
			if (tdmin < 0)
				tdmin = 0;
		} else
			tdmin = -local[1];

		if (local[0] > 0) {
			sdmin = local[0] - (smax<<4);
			if (sdmin < 0)
				sdmin = 0;
		} else
			sdmin = -local[0];

		if (sdmin > tdmin)
			distmin = (sdmin<<8) + (tdmin<<7);
		else
			distmin = (tdmin<<8) + (sdmin<<7);

		if (distmin < iminlight)
		{
			// save dlight info
			light = &dlightlist[numdlights];
			light->minlight = iminlight;
			light->rad = irad;
			light->local[0] = local[0];
			light->local[1] = local[1];
			light->type = cl_dlights[lnum].type;
			numdlights++;
		}
	}
}

int dlightcolor[NUM_DLIGHTTYPES][3] = {
	{ 100, 90, 80 },	// dimlight or brightlight
	{ 0, 0, 128 },		// blue
	{ 128, 0, 0 },		// red
	{ 128, 0, 128 },	// red + blue
	{ 0, 128, 0},		// green
	{ 128, 128, 128},	// white
	{ 100, 50, 10 },	// muzzleflash
	{ 100, 50, 10 },	// explosion
	{ 90, 60, 7 }		// rocket
};

// <-- Tonik

void R_RenderDynamicLightmaps (msurface_t *fa);

/*
===============
R_AddDynamicLights

NOTE: R_BuildDlightList must be called first!
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			i;
	int			smax, tmax;
	int			s, t;
	int			sd, td;
	int			_sd, _td;
	int			irad, idist, iminlight;
	dlightinfo_t	*light;
	unsigned	*dest;
	int			color[3];
	int			tmp;
	
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	
	for (i=0,light=dlightlist ; i<numdlights ; i++,light++)
	{
		if (lightmap_bytes != 1) {
			color[0] = dlightcolor[light->type][0];
			color[1] = dlightcolor[light->type][1];
			color[2] = dlightcolor[light->type][2];
		}
		
		irad = light->rad;
		iminlight = light->minlight;
		
		_td = light->local[1];
		dest = blocklights;
		for (t = 0 ; t<tmax ; t++)
		{
			td = _td;
			if (td < 0)	td = -td;
			_td -= 16;
			_sd = light->local[0];
			if (lightmap_bytes == 1) {
				for (s=0 ; s<smax ; s++) {
					sd = _sd < 0 ? -_sd : _sd;
					_sd -= 16;
					if (sd > td)
						idist = (sd<<8) + (td<<7);
					else
						idist = (td<<8) + (sd<<7);
					if (idist < iminlight)
						*dest += irad - idist;
					dest++;
				}
			}
			else {
				for (s=0 ; s<smax ; s++) {
					sd = _sd < 0 ? -_sd : _sd;
					_sd -= 16;
					if (sd > td)
						idist = (sd<<8) + (td<<7);
					else
						idist = (td<<8) + (sd<<7);
					if (idist < iminlight) {
						tmp = (irad - idist) >> 7;
						dest[0] += tmp * color[0];
						dest[1] += tmp * color[1];
						dest[2] += tmp * color[2];
					}
					dest += 3;
				}
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride) {
	int			smax, tmax;
	int			t;
	int			i, j, size, blocksize;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	unsigned	*bl;

	surf->cached_dlight = !!numdlights;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	blocksize = size * lightmap_bytes;
	lightmap = surf->samples;

// set to full bright if no light data
	if (!cl.worldmodel->lightdata) {
		for (i=0 ; i<blocksize ; i++)
			blocklights[i] = 255*256;
		goto store;
	}

// clear to no light
	memset (blocklights, 0, blocksize * sizeof(int));

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ; maps++) {
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			bl = blocklights;
			for (i = 0 ; i < blocksize ; i++)
				*bl++ += lightmap[i] * scale;
			lightmap += blocksize;		// skip to next lightmap
		}

// add all the dynamic lights
	if (numdlights)
		R_AddDynamicLights (surf);

// bound, invert, and shift
store:
	if (lightmap_bytes == 3) {
		bl = blocklights;
		stride -= smax * 3;
		for (i=0 ; i<tmax ; i++, dest += stride) {
			if (lightmode == 2)
				for (j = smax; j ; j--) {
					t = bl[0]; t = (t >> 8) + (t >> 9); if (t > 255) t = 255;
					dest[0] = 255 - t;
					t = bl[1]; t = (t >> 8) + (t >> 9); if (t > 255) t = 255;
					dest[1] = 255 - t;
					t = bl[2]; t = (t >> 8) + (t >> 9); if (t > 255) t = 255;
					dest[2] = 255 - t;
					bl += 3;
					dest += 3;
				}
			else
				for (j = smax; j; j--) {
					t = bl[0]; t = t >> 7; if (t > 255) t = 255;
					dest[0] = 255 - t;
					t = bl[1]; t = t >> 7; if (t > 255) t = 255;
					dest[1] = 255 - t;
					t = bl[2]; t = t >> 7; if (t > 255) t = 255;
					dest[2] = 255 - t;
					bl += 3;
					dest += 3;
				}
		}
	} else {
		bl = blocklights;
		stride -= smax;
		for (i=0 ; i<tmax ; i++, dest += stride) {
			if (lightmode == 2)
				for (j=0 ; j<smax ; j++) {
					t = *bl++;
					t = (t >> 8) + (t >> 9);
					if (t > 255)
						t = 255;
					*dest++ = 255-t;
				}
			else
				for (j=0 ; j<smax ; j++) {
					t = *bl++ >> 7;
					if (t > 255)
						t = 255;
					*dest++ = 255-t;
				}
		}
	}
}

/*
===============
R_UploadLightMap
===============
*/
void R_UploadLightMap (int lightmapnum)
{
	glRect_t	*theRect;

	lightmap_modified[lightmapnum] = false;
	theRect = &lightmap_rectchange[lightmapnum];
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, theRect->t, 
		BLOCK_WIDTH, theRect->h, gl_lightmap_format, GL_UNSIGNED_BYTE,
		lightmaps+(lightmapnum*BLOCK_HEIGHT + theRect->t)*BLOCK_WIDTH*lightmap_bytes);
	theRect->l = BLOCK_WIDTH;
	theRect->t = BLOCK_HEIGHT;
	theRect->h = 0;
	theRect->w = 0;
}

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	int		reletive;
	int		count;

	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}
	
	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;	
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}


/*
=============================================================

	BRUSH MODELS

=============================================================
*/

extern	int		solidskytexture;
extern	int		alphaskytexture;
extern	float	speedscale;		// for top sky and bottom sky

qboolean mtexenabled = false;

void GL_SelectTexture (GLenum target);

void GL_DisableMultitexture(void) 
{
	if (mtexenabled) {
		glDisable(GL_TEXTURE_2D);
		SelectTextureMTex (GL_MTexture0);
		mtexenabled = false;
	}
}

void GL_EnableMultitexture(void) 
{
	if (gl_mtexable) {
		SelectTextureMTex (GL_MTexture1);
		glEnable(GL_TEXTURE_2D);
		mtexenabled = true;
	}
}

/*
================
R_DrawSequentialPoly

Systems that have fast state and texture changes can
just do everything as it passes with no need to sort
================
*/
void R_DrawSequentialPoly (msurface_t *s)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	texture_t	*t;
/*	glRect_t	*theRect;*/
	//
	// normal lightmaped poly
	//
	if (!(s->flags & (SURF_DRAWSKY|SURF_DRAWTURB)))
	{
		R_RenderDynamicLightmaps (s);
		if (gl_mtexable) {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
			// Binds world to texture env 0
			GL_SelectTexture(GL_MTexture0);
			GL_Bind (t->gl_texturenum);
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			// Binds lightmap to texenv 1
			GL_EnableMultitexture (); // Same as SelectTexture (TEXTURE1)
			GL_Bind (lightmap_textures + s->lightmaptexturenum);
			i = s->lightmaptexturenum;
			
			if (lightmap_modified[i])
				R_UploadLightMap (i);
			
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i < p->numverts ; i++, v += VERTEXSIZE)
			{
				MTexCoord2f (GL_MTexture0, v[3], v[4]);
				MTexCoord2f (GL_MTexture1, v[5], v[6]);
				glVertex3fv (v);
			}
			glEnd ();
		} else {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
			GL_Bind (t->gl_texturenum);
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[3], v[4]);
				glVertex3fv (v);
			}
			glEnd ();

			GL_Bind (lightmap_textures + s->lightmaptexturenum);
			i = s->lightmaptexturenum;
			if (lightmap_modified[i])
				R_UploadLightMap (i);

			glEnable (GL_BLEND);
			glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5], v[6]);
				glVertex3fv (v);
			}
			glEnd ();
			glDisable (GL_BLEND);
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		
		if (s->flags & SURF_UNDERWATER && gl_caustics.value) {
//			EmitUnderwaterPolys (s);
// BorisU: draw later in R_DrawCausticsChain
			s->polys->caustics_chain = caustics_polys;
			caustics_polys = s->polys;
		}

		if (t->fb_texturenum) {
			s->polys->fb_chain = fullbright_polys[t->fb_texturenum];
			fullbright_polys[t->fb_texturenum] = s->polys;
			drawfullbrights = true;
		}

		if (t->gl_lumitex) {
			s->polys->luma_chain = luma_polys[t->gl_lumitex];
			luma_polys[t->gl_lumitex] = s->polys;
			drawluma = true;
		}
		return;
	}

	//
	// subdivided water surface warp
	//
	if (s->flags & SURF_DRAWTURB)
	{
		GL_DisableMultitexture ();
		GL_Bind (s->texinfo->texture->gl_texturenum);
		EmitWaterPolys (s);
		return;
	}
	//
	// subdivided sky warp
	//
	if (s->flags & SURF_DRAWSKY)
	{
		GL_DisableMultitexture ();
		GL_Bind (solidskytexture);
		speedscale = cl.time*8;
		speedscale -= (int)speedscale & ~127;

		EmitSkyPolys (s);

		glEnable (GL_BLEND);
		GL_Bind (alphaskytexture);
		speedscale = cl.time*16;
		speedscale -= (int)speedscale & ~127;
		EmitSkyPolys (s);

		glDisable (GL_BLEND);
		return;
	}
}

// BorisU -->
void R_BlackPolys (msurface_t *s)
// HACK! HACK! HACK!
// There is no need to use lightmap at black surface :)
{
	glpoly_t	*p;
	float		*v;
	int			i;

	GL_DisableMultitexture ();
	glDisable(GL_TEXTURE_2D);
	glColor3ubv(color_black);
	for ( ; s ; s=s->texturechain) {
		p = s->polys;
		glBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i < p->numverts ; i++, v += VERTEXSIZE) {
			glVertex3fv (v);
		}

		glEnd ();
	}
	glColor3ubv(color_white);
	glEnable(GL_TEXTURE_2D);
//	GL_EnableMultitexture ();
}

void R_DrawChain (msurface_t *s)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	texture_t	*t;
/*	glRect_t	*theRect;*/

	if (!gl_mtexable) { // old gl_texsort 1 path
		for ( ; s ; s=s->texturechain)
			R_DrawSequentialPoly (s);
		return;
	}

	// new path
	t = R_TextureAnimation (s->texinfo->texture);

	// Binds world to texture env 0
	GL_SelectTexture(GL_MTexture0);
	GL_Bind (t->gl_texturenum);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Binds lightmap to texenv 1
	GL_EnableMultitexture (); // Same as SelectTexture (TEXTURE1)
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

	for ( ; s ; s=s->texturechain) {
		p = s->polys;
		GL_Bind (lightmap_textures + s->lightmaptexturenum);

		R_RenderDynamicLightmaps (s);

		i = s->lightmaptexturenum;
		
		if (lightmap_modified[i])
			R_UploadLightMap (i);
		
		glBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i < p->numverts ; i++, v += VERTEXSIZE)
		{
			MTexCoord2f (GL_MTexture0, v[3], v[4]);
			MTexCoord2f (GL_MTexture1, v[5], v[6]);
			glVertex3fv (v);
		}
		glEnd ();

		if (s->flags & SURF_UNDERWATER && gl_caustics.value) {
//			EmitUnderwaterPolys (s);
// BorisU: draw later in R_DrawCausticsChain
			s->polys->caustics_chain = caustics_polys;
			caustics_polys = s->polys;
		}

		if (t->fb_texturenum) {
			s->polys->fb_chain = fullbright_polys[t->fb_texturenum];
			fullbright_polys[t->fb_texturenum] = s->polys;
			drawfullbrights = true;
		}

		if (t->gl_lumitex) {
			s->polys->luma_chain = luma_polys[t->gl_lumitex];
			luma_polys[t->gl_lumitex] = s->polys;
			drawluma = true;
		}
	}
}
// <-- BorisU

/*
================
DrawGLPoly
================
*/
void DrawGLPoly (glpoly_t *p)
{
	int		i;
	float	*v;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[3], v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}


/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps (void)
{
	int			i, j;
	glpoly_t	*p;
	float		*v;
/*	glRect_t	*theRect;*/

	if (!gl_texsort.value)
		return;

	glDepthMask (GL_FALSE);		// don't bother writing Z
	glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glEnable (GL_BLEND);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		p = lightmap_polys[i];
		if (!p)
			continue;
		GL_Bind(lightmap_textures+i);
		if (lightmap_modified[i])
			R_UploadLightMap (i);
		for ( ; p ; p=p->chain)
		{
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5], v[6]);
				glVertex3fv (v);
			}
			glEnd ();
		}
	}

	glDisable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (GL_TRUE);		// back to normal Z buffering
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	texture_t	*t;
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;
	qboolean	lightstyle_modified = false; // Tonik

	c_brush_polys++;

	if (fa->flags & SURF_DRAWSKY)
	{	// warp texture, no lightmaps
		EmitBothSkyLayers (fa);
		return;
	}

	t = R_TextureAnimation (fa->texinfo->texture);
	GL_Bind (t->gl_texturenum);

	if (fa->flags & SURF_DRAWTURB)
	{	// warp texture, no lightmaps
		EmitWaterPolys (fa);
		return;
	}

	DrawGLPoly (fa->polys);

	// add the poly to the proper lightmap chain

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	if (t->fb_texturenum) {
		fa->polys->fb_chain = fullbright_polys[t->fb_texturenum];
		fullbright_polys[t->fb_texturenum] = fa->polys;
		drawfullbrights = true;
	}

	if (t->gl_lumitex) {
		fa->polys->luma_chain = luma_polys[t->gl_lumitex];
		luma_polys[t->gl_lumitex] = fa->polys;
		drawluma = true;
	}

	if (fa->flags & SURF_UNDERWATER && gl_caustics.value) {
//			EmitUnderwaterPolys (s);
// BorisU: draw later in R_DrawCausticsChain
		fa->polys->caustics_chain = caustics_polys;
		caustics_polys = fa->polys;
	}

	// check for lightmap modification
	for (maps=0 ; maps<MAXLIGHTMAPS && fa->styles[maps] != 255 ; maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps]) {
			lightstyle_modified = true;
			break;
		}
		
	if (fa->dlightframe == r_framecount)
		R_BuildDlightList (fa);
	else
		numdlights = 0;
	
	if (numdlights == 0 && !fa->cached_dlight && !lightstyle_modified)
		return;
	
	lightmap_modified[fa->lightmaptexturenum] = true;
	theRect = &lightmap_rectchange[fa->lightmaptexturenum];
	if (fa->light_t < theRect->t) {
		if (theRect->h)
			theRect->h += theRect->t - fa->light_t;
		theRect->t = fa->light_t;
	}
	if (fa->light_s < theRect->l) {
		if (theRect->w)
			theRect->w += theRect->l - fa->light_s;
		theRect->l = fa->light_s;
	}
	smax = (fa->extents[0]>>4)+1;
	tmax = (fa->extents[1]>>4)+1;
	if ((theRect->w + theRect->l) < (fa->light_s + smax))
		theRect->w = (fa->light_s-theRect->l)+smax;
	if ((theRect->h + theRect->t) < (fa->light_t + tmax))
		theRect->h = (fa->light_t-theRect->t)+tmax;
	base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
	R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
}

/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps (msurface_t *fa)
{
	byte		*base;
	int			maps;
	glRect_t	*theRect;
	int			smax, tmax;
	qboolean	lightstyle_modified = false;

	c_brush_polys++;

	if (fa->flags & ( SURF_DRAWSKY | SURF_DRAWTURB) )
		return;

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;


	// check for lightmap modification
	for (maps=0 ; maps<MAXLIGHTMAPS && fa->styles[maps] != 255 ; maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps]) {
			lightstyle_modified = true;
			break;
		}

		if (fa->dlightframe == r_framecount)
			R_BuildDlightList (fa);
		else
			numdlights = 0;
		
		if (numdlights == 0 && !fa->cached_dlight && !lightstyle_modified)
			return;

		lightmap_modified[fa->lightmaptexturenum] = true;
		theRect = &lightmap_rectchange[fa->lightmaptexturenum];
		if (fa->light_t < theRect->t) {
			if (theRect->h)
				theRect->h += theRect->t - fa->light_t;
			theRect->t = fa->light_t;
		}
		if (fa->light_s < theRect->l) {
			if (theRect->w)
				theRect->w += theRect->l - fa->light_s;
			theRect->l = fa->light_s;
		}
		smax = (fa->extents[0]>>4)+1;
		tmax = (fa->extents[1]>>4)+1;
		if ((theRect->w + theRect->l) < (fa->light_s + smax))
			theRect->w = (fa->light_s-theRect->l)+smax;
		if ((theRect->h + theRect->t) < (fa->light_t + tmax))
			theRect->h = (fa->light_t-theRect->t)+tmax;
		base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
		base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
		R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
}


/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	if (!waterchain)
		return;

	if (r_wateralpha.value == 1.0 && gl_texsort.value)
		return;

	//
	// go back to the world matrix
	//

	glLoadMatrixf (r_world_matrix);

	if (r_wateralpha.value < 1.0) {
		glEnable (GL_BLEND);
		glColor4f (1, 1, 1, r_wateralpha.value);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		if (r_wateralpha.value < 0.9)
			glDepthMask (GL_FALSE);
	}

	if (!gl_texsort.value) {
		for ( s = waterchain ; s ; s=s->texturechain) {
			GL_Bind (s->texinfo->texture->gl_texturenum);
			EmitWaterPolys (s);
		}
		
		waterchain = NULL;
		waterchain_tail = &waterchain;

	} else {

		for (i=0 ; i<cl.worldmodel->numtextures ; i++)
		{
			t = cl.worldmodel->textures[i];
			if (!t)
				continue;
			s = t->texturechain;
			if (!s)
				continue;
			if ( !(s->flags & SURF_DRAWTURB ) )
				continue;

			// set modulate mode explicitly
			
			GL_Bind (t->gl_texturenum);

			for ( ; s ; s=s->texturechain)
				EmitWaterPolys (s);
			
			t->texturechain = NULL;
			t->texturechain_tail = &t->texturechain;
		}

	}

	if (r_wateralpha.value < 1.0) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor3ubv (color_white);
		glDisable (GL_BLEND);
		if (r_wateralpha.value < 0.9)
			glDepthMask (GL_TRUE);
	}

}


/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	texture_t	*t;

	if (skychain && !r_skyboxloaded && gl_drawskyfirst.value) {
		R_DrawSkyChain(skychain);
		skychain = NULL;
		skychain_tail = &skychain;
	}

	if (!gl_texsort.value) {
		for (i=0 ; i<cl.worldmodel->numtextures ; i++) {
			t = cl.worldmodel->textures[i];
			if (!t)
				continue;
			s = t->texturechain;
			if (!s)
				continue;
			if (i == skytexturenum)
				continue;
			else if (i == blacktexturenum && r_fastblack.value)
				R_BlackPolys(s);
			else
			{
				if (s->flags & SURF_DRAWTURB)
					continue;
				
				R_DrawChain (s);
			}
			t->texturechain = NULL;
			t->texturechain_tail = &t->texturechain;
		}
	} else {
		// old gl_texsort path
		for (i=0 ; i<cl.worldmodel->numtextures ; i++) {
			t = cl.worldmodel->textures[i];
			if (!t)
				continue;
			s = t->texturechain;
			if (!s)
				continue;
			if (i == skytexturenum ) {
				;
				//if (!r_skyboxloaded)
				//	R_DrawSkyChain (s);
			} else if (i == blacktexturenum && r_fastblack.value)
				R_BlackPolys(s);
			else {
				if ((s->flags & SURF_DRAWTURB) && r_wateralpha.value != 1.0)
					continue;	// draw translucent water later
				for ( ; s ; s=s->texturechain)
					R_RenderBrushPoly (s);
			}
			t->texturechain = NULL;
			t->texturechain_tail = &t->texturechain;
		}
	}

	if (skychain && !r_skyboxloaded && !gl_drawskyfirst.value) {
		R_DrawSkyChain(skychain);
		skychain = NULL;
		skychain_tail = &skychain;
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	int			i;
	int			k;
	vec3_t		mins, maxs;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	qboolean	rotated;

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	glColor3ubv (color_white);
	memset (lightmap_polys, 0, sizeof(lightmap_polys));

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

// calculate dynamic lighting for bmodel if it's not an
// instanced model
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		for (k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if ((cl_dlights[k].die < cl.time) ||
				(!cl_dlights[k].radius))
				continue;

			R_MarkLights (&cl_dlights[k], 1<<k,
				clmodel->nodes + clmodel->hulls[0].firstclipnode);
		}
	}

	glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug

	//
	// draw texture
	//
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (gl_texsort.value)
				R_RenderBrushPoly (psurf);
			else
				R_DrawSequentialPoly (psurf);
		}
	}

	R_BlendLightmaps ();

	R_RenderFullbrights ();

	glPopMatrix ();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	int			clipped;
	mplane_t	*clipplane;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid
	if (node->visframe != r_visframecount)
		return;
	for (c=0,clipplane=frustum ; c<4 ; c++,clipplane++)
	{
		if (!(clipflags & (1<<c)))
			continue;	// don't need to clip against it
		
		clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);
		if (clipped == 2)
			return;
		else if (clipped == 1)
			clipflags &= ~(1<<c);	// node is entirely on screen
	}
	
// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mleaf_t *)node;

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

	// deal with model fragments in this leaf
		if (pleaf->efrags)
			R_StoreEfrags (&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	if (plane->type < 3)
		dot = modelorg[plane->type] - plane->dist;
	else
		dot = DotProduct (modelorg, plane->normal) - plane->dist;

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side], clipflags);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = cl.worldmodel->surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			for ( ; c ; c--, surf++) {
				if (surf->visframe != r_framecount)
					continue;

				if ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK))
					continue;		// wrong side

				if ((surf->flags & SURF_DRAWSKY) && r_skyboxloaded) {
					R_AddSkyBoxSurface (surf);
				}

				if (surf->flags & SURF_DRAWSKY) {
					CHAIN_SURF (surf, skychain);
				} else if (surf->flags & SURF_DRAWTURB && !gl_texsort.value) {
					CHAIN_SURF (surf, waterchain);
				} else {
					CHAIN_SURF (surf, surf->texinfo->texture->texturechain);
				}
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side], clipflags);
}



/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	memset (&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy (r_refdef.vieworg, modelorg);

	currententity = &ent;
	currenttexture = -1;

	glColor3ubv (color_white);
	memset (lightmap_polys, 0, sizeof(lightmap_polys));
	if (gl_fb_bmodels.value) {
		memset (fullbright_polys, 0, sizeof(fullbright_polys));
		memset (luma_polys, 0, sizeof(luma_polys));
	}
	caustics_polys = NULL; // BorisU

	R_ClearSkyBox ();

	R_RecursiveWorldNode (cl.worldmodel->nodes, 15);

	R_DrawSkyBox ();

	if (r_skyboxloaded && skychain) { 
		R_DrawSkyChainZ (skychain); // BorisU
		skychain = NULL;
		skychain_tail = &skychain;
	}
	
	DrawTextureChains ();

	R_BlendLightmaps ();

	R_DrawCausticsChain (); // BorisU

	R_RenderFullbrights ();

	GL_DisableMultitexture ();
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;
	byte	solid[4096];

	if (!r_novis.value && r_oldviewleaf == r_viewleaf
		&& r_oldviewleaf2 == r_viewleaf2)	// watervis hack
		return;

	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value) {
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	} else {
		vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);

		if (r_viewleaf2) {
			int			i, count;
			unsigned	*src, *dest;

			// merge visibility data for two leafs
			count = (cl.worldmodel->numleafs+7)>>3;
			memcpy (solid, vis, count);
			src = (unsigned *) Mod_LeafPVS (r_viewleaf2, cl.worldmodel);
			dest = (unsigned *) solid;
			count = (count + 3)>>2;
			for (i=0 ; i<count ; i++)
				*dest++ |= *src++;
			vis = solid;
		}
	}		

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_LIGHTMAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("AllocBlock: full");
	return 0;
}


mvertex_t	*r_pcurrentvertbase;
model_t		*currentmodel;

int	nColinElim;

/*
================
BuildSurfaceDisplayList
================
*/
void BuildSurfaceDisplayList (msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER) )
	{
		for (i = 0 ; i < lnumverts ; ++i)
		{
			vec3_t v1, v2;
			float *prev, *this, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			this = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract( this, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((fabs( v1[0] - v2[0] ) <= COLINEAR_EPSILON) &&
				(fabs( v1[1] - v2[1] ) <= COLINEAR_EPSILON) && 
				(fabs( v1[2] - v2[2] ) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;
	R_BuildLightMap (surf, base, BLOCK_WIDTH*lightmap_bytes);
}


/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int		i, j;
	model_t	*m;

	memset (allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	if (!lightmap_textures)
	{
		lightmap_textures = texture_extension_number;
		texture_extension_number += MAX_LIGHTMAPS;
	}

	if (lightmap_bytes == 3) {
		switch ((int)gl_texturebits.value){
		case 32:
			gl_internal_lightmap_format = GL_RGB8; break;
		case 16:
			gl_internal_lightmap_format = GL_RGB5; break;
		default:
			gl_internal_lightmap_format = GL_RGB;
		}
		gl_lightmap_format = GL_RGB;
	} else {
		gl_internal_lightmap_format = gl_lightmap_format = GL_LUMINANCE;
	}
	
	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i=0 ; i<m->numsurfaces ; i++)
		{
			GL_CreateSurfaceLightmap (m->surfaces + i);
			if ( m->surfaces[i].flags & SURF_DRAWTURB )
				continue;
#ifndef QUAKE2
			if ( m->surfaces[i].flags & SURF_DRAWSKY )
				continue;
#endif
			BuildSurfaceDisplayList (m->surfaces + i);
		}
	}

 	if (!gl_texsort.value)
 		GL_SelectTexture(GL_MTexture1);

	//
	// upload all lightmaps that were filled
	//
	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		GL_Bind(lightmap_textures + i);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D (GL_TEXTURE_2D, 0, gl_internal_lightmap_format, BLOCK_WIDTH, BLOCK_HEIGHT, 0, 
		gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*lightmap_bytes);
	}

 	if (!gl_texsort.value)
 		GL_SelectTexture(GL_MTexture0);

}

