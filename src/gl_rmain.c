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
// r_main.c

#include "quakedef.h"

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;
static entity_t gun;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture 

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins
int			skyboxtextures;		
int			underwatertexture;

qboolean	r_fov_greater_than_90;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;
mleaf_t		*r_viewleaf2, *r_oldviewleaf2;	// Tonik, for watervis hack

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_shadows = {"r_shadows","0"};
cvar_t	r_wateralpha = {"r_wateralpha","1"};
cvar_t	r_novis = {"r_novis","0"};
cvar_t	r_netgraph = {"r_netgraph","0"};

// Tonik -->
cvar_t	r_watervishack = {"r_watervishack", "1"};
cvar_t	r_fastsky = {"r_fastsky", "0"};
cvar_t	r_drawflame = {"r_drawflame","1"};
cvar_t	r_fullbrightskins = {"r_fullbrightskins", "0"};
// <-- Tonik

cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_texsort = {"gl_texsort","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};

cvar_t	gl_cachemodels = {"gl_cachemodels","0"}; // BorisU

cvar_t	gl_polyblend = {"gl_polyblend","1"};
cvar_t	gl_flashblend = {"gl_flashblend","1"};
qboolean OnPlayerMipChange (cvar_t *var, const char *value);
cvar_t	gl_playermip = {"gl_playermip","0", 0, OnPlayerMipChange}; // BorisU
cvar_t	gl_nocolors = {"gl_nocolors","0"};
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","1"};
//cvar_t	gl_reporttjunctions = {"gl_reporttjunctions","0"};
cvar_t	gl_finish = {"gl_finish","0"};

cvar_t	gl_flares = {"gl_flares","0"}; // -=MD=-

// Tonik -->
cvar_t	gl_fb_depthhack		= {"gl_fb_depthhack","0"};
cvar_t	gl_fb_bmodels		= {"gl_fb_bmodels","1"};
cvar_t	gl_fb_models		= {"gl_fb_models","1"};

cvar_t	gl_colorlights		= {"gl_colorlights","0"};
cvar_t	gl_lightmode		= {"gl_lightmode","0"};
qboolean OnSkyboxChange (cvar_t *var, const char *value);
cvar_t	r_skybox			= {"r_skybox", "", 0, OnSkyboxChange};
int		lightmode = 0;
// <-- Tonik

// fuh -->
cvar_t	gl_loadlitfiles = {"gl_loadlitfiles", "1"};
cvar_t	gl_caustics = {"gl_caustics", "1"};
cvar_t	gl_waterfog = {"gl_waterfog", "2"};
qboolean OnFogDensityChange (cvar_t *var, const char *value);
cvar_t  gl_waterfog_density = {"gl_waterfogdensity", "0.33", 0, OnFogDensityChange};
// <-- fuh

// BorisU -->
cvar_t	gl_use_24bit_textures = {"gl_use_24bit_textures", "1"};
cvar_t	gl_blend_sprites = {"gl_blend_sprites", "1"};
cvar_t	gl_drawskyfirst = {"gl_drawskyfirst", "0"};
cvar_t	r_farclip = {"r_farclip", "4096"};
cvar_t	r_fastblack = {"r_fastblack", "1"};
// <-- BorisU

extern	cvar_t	gl_ztrick;
extern	cvar_t	cl_nolerp;

static int deathframes[] = { 49, 60, 69, 77, 84, 93, 102, 0 }; // for interpolation

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}

/*
=================
R_CullSphere

Returns true if the sphere is completely outside the frustum
=================
*/
qboolean R_CullSphere (vec3_t centre, float radius)
{
	int		i;
	mplane_t *p;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if ( DotProduct (centre, p->normal) - p->dist <= -radius )
			return true;
	}

	return false;
}


void R_RotateForEntity (entity_t *e)
{
	glTranslatef (e->origin[0],  e->origin[1],	e->origin[2]);

	glRotatef (e->angles[1],  0, 0, 1);
	glRotatef (-e->angles[0],  0, 1, 0);
	glRotatef (e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = currententity->model->cache.data;
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	float		*up, *right;
	vec3_t		v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	GL_Bind(frame->gl_texturenum);

	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);
	
	glEnd ();
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
float	shadescale = 0; // Tonik
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];


static float	r_modelalpha;
static float	r_framelerp;
static qboolean r_limitlerp;	// this variable is used to prevent interpolation
								// glitches in some viewmodels
static float	r_lerpdistance;

int	lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum, qboolean mtex)
{
	float 		l;
	trivertx_t	*verts;
	int			*order;
	int			count;

	lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	if (r_modelalpha < 1)
		glEnable(GL_BLEND);

	while ((count = *order++)) {
		// get the vertex count and primitive type
		if (count < 0) {
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		} else
			glBegin (GL_TRIANGLE_STRIP);

		do		{
			// texture coordinates come from the draw list
			if (mtex) {
				MTexCoord2f (GL_MTexture0, ((float *) order)[0], ((float *) order)[1]);
				MTexCoord2f (GL_MTexture1, ((float *) order)[0], ((float *) order)[1]);

			} else {
				glTexCoord2f (((float *) order)[0], ((float *) order)[1]);
			}

			order += 2;

			// normals and vertexes come from the frame list
			l = (shadedots[verts->lightnormalindex] * shadelight + ambientlight) / 256;
			
			if (l > 1)
				l = 1;

			//glColor3f (l, l, l);
			glColor4f (l, l, l, r_modelalpha);
			glVertex3f (verts->v[0], verts->v[1], verts->v[2]);
			verts++;
		} while (--count);

		glEnd ();
	}

	if (r_modelalpha < 1)
		glDisable(GL_BLEND);

}

// model interpolation
/*
=============
GL_DrawAliasBlendedFrame
=============
*/
void GL_DrawAliasBlendedFrame (aliashdr_t *paliashdr, int pose1, int pose2, qboolean mtex)
{
	float 		l;
	trivertx_t	*verts1, *verts2;
	int			*order;
	int			count;
	vec3_t		interpolated_verts;
	float		lerpfrac;

	lerpfrac = r_framelerp;
	lastposenum = (lerpfrac >= 0.5) ? pose2 : pose1;	

	verts1 = (trivertx_t *)((byte *) paliashdr + paliashdr->posedata);
	verts2 = verts1;

	verts1 += pose1 * paliashdr->poseverts;
	verts2 += pose2 * paliashdr->poseverts;

	order = (int *)((byte *) paliashdr + paliashdr->commands);

	if (r_modelalpha < 1)
		glEnable(GL_BLEND);

	while ((count = *order++)) {
		// get the vertex count and primitive type
		if (count < 0) {
			count = -count;
			glBegin(GL_TRIANGLE_FAN);
		} else {
			glBegin(GL_TRIANGLE_STRIP);
		}
		
		do {
			// texture coordinates come from the draw list
			if (mtex) {
				MTexCoord2f (GL_MTexture0, ((float *) order)[0], ((float *) order)[1]);
				MTexCoord2f (GL_MTexture1, ((float *) order)[0], ((float *) order)[1]);
			} else {
				glTexCoord2f (((float *) order)[0], ((float *) order)[1]);
			}

			order += 2;

			if (r_limitlerp)
				lerpfrac = VectorL2Compare(verts1->v, verts2->v, r_lerpdistance) ? r_framelerp : 1;

			// normals and vertexes come from the frame list
			l = FloatInterpolate (shadedots[verts1->lightnormalindex], lerpfrac, shadedots[verts2->lightnormalindex]); // FIXME -- should we divide it by 127 as fuh does?
			l = (l * shadelight + ambientlight) / 256;
			
			if (l > 1)
				l = 1;

			glColor4f (l, l, l, r_modelalpha);

			VectorInterpolate (verts1->v, lerpfrac, verts2->v, interpolated_verts);
			glVertex3fv (interpolated_verts);

			verts1++;
			verts2++;
		} while (--count);

		glEnd();
	}	

	if (r_modelalpha < 1)
		glDisable(GL_BLEND);
}

/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	trivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			glVertex3fv (point);

			verts++;
		} while (--count);

		glEnd ();
	}
}

/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr, qboolean mtex)
{
	int		pose, numposes;
	float	interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
//		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose, mtex);
}

// model interpolation
/*
=================
R_SetupAliasBlendedFrame
=================
*/
void R_SetupAliasBlendedFrame (maliasframedesc_t *oldframe, maliasframedesc_t *frame,
							   aliashdr_t *paliashdr, qboolean mtex)
{
	int		oldpose, pose, numposes;
	float	interval;

	oldpose = oldframe->firstpose;
	numposes = oldframe->numposes;
	if (numposes > 1)
	{
		interval = oldframe->interval;
		oldpose += (int)(cl.time / interval) % numposes;
	}

	pose = frame->firstpose;
	numposes = frame->numposes;
	if (numposes > 1)
	{
		interval = frame->interval;
		pose += (int) (cl.time / interval) % numposes;
	}

	GL_DrawAliasBlendedFrame (paliashdr, oldpose, pose, mtex);
}


/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *ent) // FIXME full_light? 
{
	int			i;
	int			lnum;
	vec3_t		dist;
	float		add;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	int			anim, skinnum;
	qboolean	full_light;
	model_t		*clmodel = ent->model;
	maliasframedesc_t *oldframe, *frame;
	int			texture, fb_texture = 0;
	float		fbskins;

	fbskins = bound(0, r_fullbrightskins.value, cl.fbskins);

	// locate the proper data

	paliashdr = (aliashdr_t *) Mod_Extradata (clmodel);
	c_alias_polys += paliashdr->numtris;

	// frame interpolation

	if (ent->frame >= paliashdr->numframes || ent->frame < 0) {
		Con_DPrintf ("R_DrawAliasModel: no such frame %d\n", ent->frame);
		ent->frame = 0;
	}
	if (ent->oldframe >= paliashdr->numframes || ent->oldframe < 0) {
		Con_DPrintf ("R_DrawAliasModel: no such oldframe %d\n", ent->oldframe);
		ent->oldframe = 0;
	}

	frame = &paliashdr->frames[ent->frame];
	oldframe = &paliashdr->frames[ent->oldframe];

	if (cl_nolerp.value || ent->framelerp < 0 || ent->oldframe == ent->frame)
		r_framelerp = 1.0;
	else
		r_framelerp = min (ent->framelerp, 1);

	// culling

	if (!(ent->flags & RF_WEAPONMODEL))
	{
		if (ent->angles[0] || ent->angles[1] || ent->angles[2])
		{
			if (R_CullSphere (ent->origin, clmodel->radius))
				return;
		}
		else
		{
			if (r_framelerp == 1) {	
				VectorAdd (ent->origin, frame->bboxmin.v, mins);
				VectorAdd (ent->origin, frame->bboxmax.v, maxs);
			} else {
				for (i = 0; i < 3; i++) {
					mins[i] = ent->origin[i] + min (oldframe->bboxmin.v[i], frame->bboxmin.v[i]);
					maxs[i] = ent->origin[i] + max (oldframe->bboxmax.v[i], frame->bboxmax.v[i]);
				}
			}

			if (R_CullBox (mins, maxs))
				return;
		}
	}
	else
	{
	}

	VectorCopy (ent->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//

// make thunderbolt and torches full light
	if (clmodel->modhint == MOD_THUNDERBOLT) {
		ambientlight = 210;
		shadelight = 0;
		full_light = true;
	} else if (clmodel->modhint == MOD_FLAME) {
		ambientlight = 255;
		shadelight = 0;
		full_light = true;
	} else {
		// normal lighting 
		full_light = false;
		ambientlight = shadelight = R_LightPoint (ent->origin);
		
		for (lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
			if (cl_dlights[lnum].die < cl.time || 
				!cl_dlights[lnum].radius)
				continue;

			VectorSubtract (ent->origin,
				cl_dlights[lnum].origin,
				dist);
			add = cl_dlights[lnum].radius - VectorLength(dist);
			
			if (add > 0)
				ambientlight += add;
		}
		
		// clamp lighting so it doesn't overbright as much
		if (ambientlight > 128)
			ambientlight = 128;
		if (ambientlight + shadelight > 192)
			shadelight = 192 - ambientlight;
		
		// always give the gun some light
		if ((ent->flags & RF_WEAPONMODEL) && ambientlight < 24)
			ambientlight = shadelight = 24;
		
		// never allow players to go totally black
		if (clmodel->modhint == MOD_PLAYER) {
			if (ambientlight < 8)
				ambientlight = shadelight = 8;
		}
	}

	if (clmodel->modhint == MOD_PLAYER && fbskins) {
		ambientlight = max(ambientlight, 8 + fbskins * 120);
		shadelight = max(shadelight, 8 + fbskins * 120);
		if (fbskins == 1)
			full_light = true;
	}

	shadedots = r_avertexnormal_dots[((int)(ent->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

	glPushMatrix ();
	R_RotateForEntity (ent);

	if (clmodel->modhint == MOD_EYES) {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - (22 + 8));
	// double size of eyes, since they are really hard to see in gl
		glScalef (paliashdr->scale[0]*2, paliashdr->scale[1]*2, paliashdr->scale[2]*2);
	} else if (ent->flags & RF_WEAPONMODEL) {
		float scale = 0.5 + bound(0, r_viewmodelsize.value, 1) / 2;
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		glScalef (paliashdr->scale[0] * scale, paliashdr->scale[1], paliashdr->scale[2]);
	} else {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	}

	anim = (int)(cl.time*10) & 3;
	skinnum = ent->skinnum;
	if ((skinnum >= paliashdr->numskins) || (skinnum < 0)) {
		Con_DPrintf ("R_DrawAliasModel: no such skin # %d\n", skinnum);
		skinnum = 0;
	}

	texture = paliashdr->gl_texturenum[skinnum][anim];
	fb_texture = paliashdr->fb_texturenum[skinnum][anim];

	if ((ent->flags & RF_WEAPONMODEL) && gl_mtexable) {
		r_modelalpha = (r_drawviewmodel.value > 1.0f) ? (r_drawviewmodel.value - 1.0f) : r_drawviewmodel.value;
	} else
		r_modelalpha = 1.0f;


	// we can't dynamically colormap textures, so they are cached
	// separately for the players.  Heads are just uncolored.
	if (ent->scoreboard && !gl_nocolors.value)
	{
		i = ent->scoreboard - cl.players;

		if (!ent->scoreboard->skin) {
			Skin_Find(ent->scoreboard);
			// R_TranslatePlayerSkin will be called in R_UpdateSkins 
			//R_TranslatePlayerSkin(i); 
		}

		if (i >= 0 && i < MAX_CLIENTS) {
			texture = playertextures + ent->scoreboard->skintexslot;
			fb_texture = fb_skins[i];
		}
	}

	if (!gl_fb_models.value)
		fb_texture = 0;

	if (fb_texture && gl_mtexable && !full_light) {
		GL_SelectTexture (GL_MTexture0);
		GL_Bind (texture);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_EnableMultitexture ();
		GL_Bind (fb_texture);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	}
	else
	{
		GL_Bind (texture);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (gl_smoothmodels.value)
		glShadeModel (GL_SMOOTH);

	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	if (r_framelerp != 1) {
		// model interpolation
		R_SetupAliasBlendedFrame (oldframe, frame, paliashdr, (fb_texture && gl_mtexable && !full_light));
	} else {
		R_SetupAliasFrame (ent->frame, paliashdr, (fb_texture && gl_mtexable && !full_light));
	}

	if (fb_texture && gl_mtexable) {
		GL_DisableMultitexture ();
	}

	if (!full_light && gl_fb_models.value && !gl_mtexable) {
		if ((clmodel->modhint == MOD_PLAYER) && ent->scoreboard) {
			i = ent->scoreboard - cl.players;

			if (i >= 0 && i < MAX_CLIENTS)
				fb_texture = fb_skins[i];
		} else
			fb_texture = paliashdr->fb_texturenum[skinnum][anim];

		if (fb_texture) {
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//			glEnable (GL_BLEND);
			glEnable (GL_ALPHA_TEST);
			GL_Bind (fb_texture);

			if (r_framelerp != 1) {
				// model interpolation
				R_SetupAliasBlendedFrame (oldframe, frame, paliashdr, false);
			} else
				R_SetupAliasFrame (ent->frame, paliashdr, false);

//			glDisable (GL_BLEND);
			glDisable (GL_ALPHA_TEST);
		}
	}

	glShadeModel (GL_FLAT);
	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPopMatrix ();

	if (r_shadows.value && !full_light && ent != &gun)
	{
		float an = -ent->angles[1] / 180 * M_PI;
		
		if (!shadescale)
			shadescale = 1/sqrt(2);

		shadevector[0] = cos(an) * shadescale;
		shadevector[1] = sin(an) * shadescale;
		shadevector[2] = shadescale;

		glPushMatrix ();

		glTranslatef (ent->origin[0],  ent->origin[1],  ent->origin[2]);
		glRotatef (ent->angles[1],  0, 0, 1);

		//FIXME glRotatef (-ent->angles[0],  0, 1, 0);
		//FIXME glRotatef (ent->angles[2],  1, 0, 0);

		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glColor4f (0, 0, 0, 0.5);
		GL_DrawAliasShadow (paliashdr, lastposenum);
		glEnable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
		glPopMatrix ();
	}

	glColor3ubv (color_white);
}

//==================================================================================

/*
=============
R_SetSpritesState
=============
*/
void R_SetSpritesState (qboolean state)
{
	static qboolean r_state = false;

	if (r_state == state)
		return;

	r_state = state;

	if (state) {
		GL_DisableMultitexture ();
		glEnable (GL_ALPHA_TEST);
		if (gl_blend_sprites.value) {
			glEnable (GL_BLEND);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glDepthMask (GL_FALSE);
//			not needed, it is default
//			glAlphaFunc(GL_GREATER, alpha_test_threshold);
		} else {
			glAlphaFunc(GL_GREATER, alpha_test_normal);
		}
	} else {
		glDisable (GL_ALPHA_TEST);
		if (gl_blend_sprites.value) {
			glDisable (GL_BLEND);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glDepthMask (GL_TRUE);
//			glAlphaFunc(GL_GREATER, alpha_test_normal);
		} else {
			glAlphaFunc(GL_GREATER, alpha_test_threshold);
		}
	}
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities.value)
		return;

	r_limitlerp = false;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = &cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;

		case mod_brush:
			R_DrawBrushModel (currententity);
			break;

		default:
			break;
		}
	}
}

#define MAX_SPRITES 256
int compare_sprites(const void* s1, const void* s2)
{
	entity_t	*sprite;
	vec3_t		rel;
	vec_t		dist1, dist2;

	sprite = *((entity_t**)s1);
	VectorSubtract(sprite->origin, cl.simorg, rel);
	dist1 = VectorLength(rel);

	sprite = *((entity_t**)s2);
	VectorSubtract(sprite->origin, cl.simorg, rel);
	dist2 = VectorLength(rel);

	return dist2 - dist1;
}

// draw sprites seperately, because of alpha blending
void R_DrawEntitiesSprites (void)
{
	int		i, n;
	entity_t	*sprites[MAX_SPRITES];

	if (!r_drawentities.value)
		return;

	R_SetSpritesState (true);

	if (!gl_blend_sprites.value){
		for (i=0 ; i<cl_numvisedicts ; i++) {
			currententity = &cl_visedicts[i];
			if (currententity->model->type == mod_sprite) {
				R_DrawSpriteModel (currententity);
			}
		}
	} else {
		for (n=i=0 ; i<cl_numvisedicts ; i++) {
			currententity = &cl_visedicts[i];
			if (currententity->model->type == mod_sprite) {
				sprites[n++] = currententity;
				if (n == MAX_SPRITES){
					Con_Printf("BUG: too many sprites\n");
					break;
				}
			}
		}

		qsort(sprites, n, sizeof(entity_t*), compare_sprites);

		for (i=0; i<n; i++){
			// currententity is used somewhere! 
			// dumb design!
			currententity = sprites[i]; 
			R_DrawSpriteModel (currententity);
		}
	}

	R_SetSpritesState (false);
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;
	centity_t	*cent;

	if (!r_drawviewmodel.value || 
		(r_fov_greater_than_90 && r_drawviewmodel.value > 1.0f) || 
		!Cam_DrawViewModel())
		return;

	if (envmap)
		return;

	if (!r_drawentities.value || !cl.viewent.current.modelindex)
		return;

	if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	memset (&gun, 0, sizeof(gun));
	cent = &cl.viewent;
	currententity = &gun;

	VectorCopy(cent->current.origin, gun.origin);
	VectorCopy(cent->current.angles, gun.angles);

	if (cl_crosshair_hack.value)
		currententity->origin[2] -= 7.8;

	gun.model = cl.model_precache[cent->current.modelindex];
	if (!gun.model)
		Host_Error ("R_DrawViewModel: bad modelindex");

	gun.frame = cent->current.frame;
	gun.colormap = vid.colormap;
	gun.flags = RF_WEAPONMODEL | RF_NOSHADOW;
	
	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - VectorLength(dist);
		if (add > 0)
			ambientlight += add;
	}

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	if (cent->frametime >= 0 && cent->frametime <= cl.time) {
		currententity->oldframe = cent->oldframe;
		currententity->framelerp = (cl.time - cent->frametime) * 10;
	} else {
		currententity->oldframe = currententity->frame;
		currententity->framelerp = -1;
	}
	
	if (cent->current.modelindex == cl_modelindices[mi_vaxe]
		|| cent->current.modelindex == cl_modelindices[mi_vbio]
		|| cent->current.modelindex == cl_modelindices[mi_vgrap]
		|| cent->current.modelindex == cl_modelindices[mi_vknife]
		|| cent->current.modelindex == cl_modelindices[mi_vknife2]
		|| cent->current.modelindex == cl_modelindices[mi_vmedi]
		|| cent->current.modelindex == cl_modelindices[mi_vspan])
	{
		r_limitlerp = false;
	}
	else {
		r_limitlerp = true;
		gun.flags |= RF_LIMITLERP;
		r_lerpdistance =  135;
	}

	// hack the depth range to prevent view model from poking into walls
	glDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	R_DrawAliasModel (currententity);
	glDepthRange (gldepthmin, gldepthmax);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
#ifdef USE_HWGAMMA
	if (vid_hwgamma_enabled && vid_hwgammacontrol.value == 1)
		return;
#endif
	if (!v_blend[3])
		return;

//Con_Printf("R_PolyBlend(): %4.2f %4.2f %4.2f %4.2f\n",v_blend[0], v_blend[1],	v_blend[2],	v_blend[3]);

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);

	glColor4fv (v_blend);

	glBegin (GL_QUADS);
	glVertex2f (r_refdef.vrect.x, r_refdef.vrect.y);
	glVertex2f (r_refdef.vrect.x + r_refdef.vrect.width, r_refdef.vrect.y);
	glVertex2f (r_refdef.vrect.x + r_refdef.vrect.width, r_refdef.vrect.y + r_refdef.vrect.height);
	glVertex2f (r_refdef.vrect.x, r_refdef.vrect.y + r_refdef.vrect.height);
	glEnd ();

	glEnable (GL_TEXTURE_2D);

	glColor3ubv (color_white);
}

// Tonik -->
/*
================
R_BrightenScreen
================
*/
void R_BrightenScreen (void)
{
	extern float	vid_gamma;
	float	f;

#ifdef USE_HWGAMMA
	if (vid_hwgamma_enabled)
		return;
#endif

	if (v_contrast.value <= 1.0)
		return;

	f = pow (v_contrast.value, vid_gamma);
	
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_DST_COLOR, GL_ONE);
	glBegin (GL_QUADS);
	while (f > 1)
	{
		if (f >= 2)
			glColor3ubv (color_white);
		else
			glColor3f (f - 1, f - 1, f - 1);
		glVertex2f (0, 0);
		glVertex2f (vid.conwidth, 0);
		glVertex2f (vid.conwidth, vid.conheight);
		glVertex2f (0, vid.conheight);
		f *= 0.5;
	}
	glEnd ();
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
	glColor3ubv (color_white);
}
// <-- Tonik

int SignbitsForPlane (mplane_t *out)
{
	int bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);

		for (i=0 ; i<4 ; i++)
			VectorNormalize (frustum[i].normal);
	}
	else
	{

		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
// don't allow cheats in multiplayer
	if (!cls.allow_transparent_water)
		r_wateralpha.value = 1;

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	if (r_watervishack.value) {
		vec3_t	testorigin;
		mleaf_t	*testleaf;

		r_oldviewleaf2 = r_viewleaf2;
		r_viewleaf2 = NULL;
		VectorCopy (r_origin, testorigin);
		if (r_viewleaf->contents <= CONTENTS_WATER  &&
			r_viewleaf->contents >= CONTENTS_LAVA) {
			// Test the point 10 units above. 10 seems to be enough
			// for fov values up to 140
			testorigin[2] += 10;
			testleaf = Mod_PointInLeaf (testorigin, cl.worldmodel);
			if (testleaf->contents == CONTENTS_EMPTY)
				r_viewleaf2 = testleaf;
		} else if (r_viewleaf->contents == CONTENTS_EMPTY) {
			testorigin[2] -= 10;
			testleaf = Mod_PointInLeaf (testorigin, cl.worldmodel);
			if (testleaf->contents <= CONTENTS_WATER &&
				testleaf->contents >= CONTENTS_LAVA)
				r_viewleaf2 = testleaf;
		}
	} else
		r_viewleaf2 = r_oldviewleaf2 = NULL;

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
			 GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	x = r_refdef.vrect.x * glwidth/vid.conwidth;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.conwidth;
	y = (vid.conheight-r_refdef.vrect.y) * glheight/vid.conheight;
	y2 = (vid.conheight - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.conheight;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	glViewport (glx + x, gly + y2, w, h);
	screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
	MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  r_farclip.value);

	glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity ();

	glRotatef (-90,  1, 0, 0);	    // put Z going up
	glRotatef (90,  0, 0, 1);	    // put Z going up
	glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
	glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
	glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
	glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}


void R_RenderLocBlock(int block)
{
	// Sergio (new locs) -->
	float _x,_y,_z;
	extern cvar_t loc_granularity;
	float x1 = (location[block].x1)*loc_granularity.value;
	float x2 = (location[block].x2)*loc_granularity.value;
	float y1 = (location[block].y1)*loc_granularity.value;
	float y2 = (location[block].y2)*loc_granularity.value;
	float z1 = (location[block].z1)*loc_granularity.value;
	float z2 = (location[block].z2)*loc_granularity.value;
// <-- Sergio

	glColor4f ( 0, 0, 1, 0.15);
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glDisable (GL_CULL_FACE);
	glDisable (GL_DEPTH_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

// Changed by Sergio (different colors) -->
	glColor4f ( 0, 0, 1, 0.2);
	glVertex3f(x1,y1,z1);
	glVertex3f(x2,y1,z1);
	glVertex3f(x2,y1,z2);
	glVertex3f(x1,y1,z2);

	glVertex3f(x1,y2,z1);
	glVertex3f(x2,y2,z1);
	glVertex3f(x2,y2,z2);
	glVertex3f(x1,y2,z2);

	glColor4f ( 0, 0.5, 1, 0.2);
	glVertex3f(x2,y1,z1);
	glVertex3f(x2,y2,z1);
	glVertex3f(x2,y2,z2);
	glVertex3f(x2,y1,z2);

	glVertex3f(x1,y1,z1);
	glVertex3f(x1,y2,z1);
	glVertex3f(x1,y2,z2);
	glVertex3f(x1,y1,z2);
// <-- Sergio

	glEnd();
	
	glColor4f ( 1, 0, 0, 0.5);	
	
	glPointSize(3.0);
	glBegin(GL_POINTS);
	
	glVertex3f(x1,y1,z1);
	glVertex3f(x2,y1,z1);
	glVertex3f(x1,y2,z1);
	glVertex3f(x2,y2,z1);

	glVertex3f(x1,y1,z2);
	glVertex3f(x2,y1,z2);
	glVertex3f(x1,y2,z2);
	glVertex3f(x2,y2,z2);
	
	glEnd();

	glEnable (GL_DEPTH_TEST);
	glBegin(GL_LINES);
	
	glVertex3f(x1,y1,z1);
	glVertex3f(x2,y2,z1);

	glVertex3f(x2,y1,z1);
	glVertex3f(x1,y2,z1);

	glVertex3f(x1,y1,z2);
	glVertex3f(x2,y2,z2);

	glVertex3f(x2,y1,z2);
	glVertex3f(x1,y2,z2);

	glEnd();

// Sergio --> (draw names of axises)
	glDisable (GL_DEPTH_TEST);
	glColor4f ( 1, 1, 0, 0.75);
	glBegin(GL_LINES);
	if (z1 != z2)
	{
		_z = max(z1, z2) - abs(z1-z2)/2 - 5;
		if (x1 != x2)
		{
			_x = max(x1, x2) - abs(x1-x2)/2 - 3;
			// x'
			glVertex3f(_x,y1,_z);
			glVertex3f(_x+6,y1,_z+10);

			glVertex3f(_x,y1,_z+10);
			glVertex3f(_x+6,y1,_z);

			glVertex3f(_x+8,y1,_z+10);
			glVertex3f(_x+8,y1,_z+6);

			// x"
			glVertex3f(_x,y2,_z);
			glVertex3f(_x+6,y2,_z+10);

			glVertex3f(_x,y2,_z+10);
			glVertex3f(_x+6,y2,_z);

			glVertex3f(_x+8,y2,_z+10);
			glVertex3f(_x+8,y2,_z+6);
			glVertex3f(_x+10,y2,_z+10);
			glVertex3f(_x+10,y2,_z+6);
		}
		if (y1 != y2)
		{
			_y = max(y1, y2) - abs(y1-y2)/2 - 3;
			// y'
			glVertex3f(x1,_y,_z+10);
			glVertex3f(x1,_y+3,_z+5);

			glVertex3f(x1,_y+3,_z+5);
			glVertex3f(x1,_y+6,_z+10);

			glVertex3f(x1,_y+3,_z+5);
			glVertex3f(x1,_y+3,_z);

			glVertex3f(x1,_y+8,_z+10);
			glVertex3f(x1,_y+8,_z+6);

			// y"
			glVertex3f(x2,_y,_z+10);
			glVertex3f(x2,_y+3,_z+5);

			glVertex3f(x2,_y+3,_z+5);
			glVertex3f(x2,_y+6,_z+10);

			glVertex3f(x2,_y+3,_z+5);
			glVertex3f(x2,_y+3,_z);

			glVertex3f(x2,_y+8,_z+10);
			glVertex3f(x2,_y+8,_z+6);
			glVertex3f(x2,_y+10,_z+10);
			glVertex3f(x2,_y+10,_z+6);
		}

	}
	glEnd();
// <-- Sergio

	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glColor3ubv (color_white);
}


void R_RenderLoc(void)
{
	if (loc_cur_block != -1 && loc_loaded.value && loc_loaded_type == 0)
		R_RenderLocBlock(loc_cur_block);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GL_DisableMultitexture();

	R_RenderLoc(); 
}


/*
=============
R_Clear
=============
*/
int gl_ztrickframe = 0;
void R_Clear (void)
{
	static qboolean cleartogray;
	qboolean	clear = false;

	if (gl_clear.value) {
		clear = true;
		if (cleartogray) {
			glClearColor (1, 0, 0, 0);
			cleartogray = false;
		}
	}
	else if (
#ifdef USE_HWGAMMA		
		!vid_hwgamma_enabled && 
#endif		
		v_contrast.value > 1) {
		clear = true;
		if (!cleartogray) {
			glClearColor (0.1, 0.1, 0.1, 0);
			cleartogray = true;
		}
	}

	if (gl_ztrick.value)
	{
		if (clear)
			glClear (GL_COLOR_BUFFER_BIT);

		gl_ztrickframe = !gl_ztrickframe;
		if (gl_ztrickframe)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (clear)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRange (gldepthmin, gldepthmax);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1 = 0, time2;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		glFinish ();
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	if (gl_finish.value)
		glFinish ();

	R_Clear ();

	// render normal view
	R_RenderScene ();
	
	R_DrawWaterSurfaces ();

	R_DrawEntitiesSprites ();

	R_DrawParticles ();

	R_RenderDlights ();

	if (gl_flares.value)
		R_RenderFlares ();

	R_DrawViewModel ();

	if (r_speeds.value)	{
		time2 = Sys_DoubleTime ();
		Print_flags[Print_current] |= PR_TR_SKIP;
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}
