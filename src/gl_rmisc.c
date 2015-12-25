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
// r_misc.c

#include "quakedef.h"

extern void R_InitBubble();

// fuh -->
void R_InitOtherTextures (void)
{
	underwatertexture = loadtexture_24bit ("water_caustic", LOADTEX_CAUSTIC);	
}
// <-- fuh

/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
}

// fuh -->
void R_InitParticleTexture (void)
{
	int				i, x, y;
	unsigned int	data[32][32];
	float			r2;

	particletexture = texture_extension_number++;
	GL_Bind(particletexture);

	// clear to transparent white
	for (i=0 ; i<32*32 ; i++)
		((unsigned *)data)[i] = LittleLong(0x00FFFFFF);

	// draw a circle in the top left corner
	for (x=0 ; x<16 ; x++)
		for (y=0 ; y<16 ; y++) {
			r2 = (x - 7.5)*(x - 7.5) + (y - 7.5)*(y - 7.5);
			// BorisU: soft edge
			if (r2 <= 6*6)
				data[y][x] = LittleLong(0xFFFFFFFF);	// solid white
			else if (r2 <= 7*7)
				data[y][x] = LittleLong(0xAAFFFFFF);	// partly transparent white
			else if (r2 <= 8*8)
				data[y][x] = LittleLong(0x55FFFFFF);	// even more transparent white

		}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_Upload32 ((unsigned *) data, 32, 32, true, true);
}
// <-- fuh


int fb_skins[MAX_CLIENTS]; // Tonik

// BorisU -->
qboolean OnPlayerMipChange (cvar_t *var, const char *value)
{
	int		newplayermip = Q_atoi(value);

	if (newplayermip < 0 || newplayermip > 4) {
		Con_Printf("Wrong gl_playermip\n");
		return true;
	}
	return false;
}
// <-- BorisU

/*
===============
R_Init
===============
*/
void R_Init (void)
{
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	
	Cmd_AddCommand ("loadsky", R_LoadSky_f);

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_shadows);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_RegisterVariable (&r_novis);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_netgraph);

// Tonik -->
	Cvar_RegisterVariable (&r_watervishack);
	Cvar_RegisterVariable (&r_fastsky);
	Cvar_RegisterVariable (&r_drawflame);
	Cvar_RegisterVariable (&r_fullbrightskins);
// <-- Tonik

	Cvar_RegisterVariable (&gl_clear);
	Cvar_RegisterVariable (&gl_texsort);

	if (gl_mtexable)
		Cvar_SetValue (&gl_texsort, 0.0);

	Cvar_RegisterVariable (&gl_cull);
	Cvar_RegisterVariable (&gl_smoothmodels);
	Cvar_RegisterVariable (&gl_affinemodels);

	Cvar_RegisterVariable (&gl_cachemodels);

	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&gl_flashblend);
	Cvar_RegisterVariable (&gl_playermip);
	Cvar_RegisterVariable (&gl_nocolors);
	Cvar_RegisterVariable (&gl_finish);

	Cvar_RegisterVariable (&gl_flares);

// Tonik -->
	Cvar_RegisterVariable (&gl_fb_depthhack);
	Cvar_RegisterVariable (&gl_fb_bmodels);
	Cvar_RegisterVariable (&gl_fb_models);

	Cvar_RegisterVariable (&gl_colorlights);
	Cvar_RegisterVariable (&gl_lightmode);
	Cvar_RegisterVariable (&r_skybox);
// <-- Tonik

// fuh -->
	Cvar_RegisterVariable (&gl_loadlitfiles);
	Cvar_RegisterVariable (&gl_caustics);
	Cvar_RegisterVariable (&gl_waterfog);
	Cvar_RegisterVariable (&gl_waterfog_density);
// <-- fuh

// BorisU -->
	Cvar_RegisterVariable (&gl_use_24bit_textures);
	Cvar_RegisterVariable (&gl_blend_sprites);
	Cvar_RegisterVariable (&gl_drawskyfirst);
	Cvar_RegisterVariable (&r_farclip);
	Cvar_RegisterVariable (&r_fastblack);
// <-- BorisU

	Cvar_RegisterVariable (&gl_keeptjunctions);
//	Cvar_RegisterVariable (&gl_reporttjunctions);

	Init_GL_textures();
	
	R_InitBubble();
	
	R_InitParticles ();
	R_InitParticleTexture ();

#ifdef GLTEST
	Test_Init ();
#endif

	netgraphtexture = texture_extension_number;
	texture_extension_number++;

	playertextures = texture_extension_number;
	texture_extension_number += MAX_CLIENTS;

	// fullbrights
	texture_extension_number += MAX_CLIENTS;

	Skin_SlotsInit();

	skyboxtextures = texture_extension_number;
	texture_extension_number += 6;

	R_InitOtherTextures ();
}

// BorisU -->
static	int		old_swapredblue = 0, swapredblue;
void ResetOldSkin(player_info_t *player)
{
	player->_topcolor = player->topcolor;
	player->_bottomcolor = player->bottomcolor;
	player->_skin = player->skin;
}

void R_UpdateSkins (void)
{
	int				i, playermip,slot;
	player_info_t	*player;
	int				top, bottom;

	playermip = (int)gl_playermip.value;
	swapredblue = (int)cl_swapredblue.value;

	for (i=0; i<MAX_CLIENTS; i++) {
		player = &cl.players[i];
		if (player->_topcolor != player->topcolor ||
			player->_bottomcolor != player->bottomcolor || !player->skin ||
			player->skin != player->_skin ||		// Sergio
			skinslots[player->skintexslot].mip != playermip ||
			swapredblue != old_swapredblue) {	// BorisU
			if (player->name[0] && !player->spectator) {
//				Con_Printf("R_UpdateSkins: %s\n", player->name);
				slot = -1;
				if (!player->skin)
					Skin_Find(player);
				if (cl_ignore_topcolor.value)
					player->topcolor = player->bottomcolor;
				top = player->topcolor;
				bottom = player->bottomcolor;
				top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
				bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
				if (swapredblue) { 
					if (top == 4) 
						top = 13;
					else if (top == 13)
						top = 4;
					if (bottom == 4) 
						bottom = 13;
					else if (bottom == 13)
						bottom = 4;
				}

				if (player->skin)
					slot = Skin_FindSlot(player->skin->name, top, bottom);
				else {
					slot = Skin_FindSlot("base", top, bottom);
//					Con_Printf("R_UpdateSkins: NULL skin %s\n", player->name);
				}
				
				if ( slot != -1) {
					// found slot with the required skin, just use it
					if (skinslots[slot].mip != playermip) {
					// if playermip changed we should upload anyway
						R_TranslatePlayerSkin (i);
						continue;
					}
					player->skintexslot = slot;
					fb_skins[i] = skinslots[slot].fb_skin;
					ResetOldSkin(player);
					continue;
				}
				player->skintexslot = Skin_FindFreeSlot();
				R_TranslatePlayerSkin (i);
			}
		}
	}
	old_swapredblue = swapredblue;
}
// <-- BorisU
/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int		top, bottom;
	byte	translate[256];
	unsigned	translate32[256];
	int		i, j;
	byte	*original;
#ifdef __APPLE__
	static		// OS X 10.2 has too small stack segment to hold this array (512k)
#endif
	unsigned	pixels[512*256];
	unsigned	*out;
	int			scaled_width, scaled_height;
	int			inwidth, inheight;
	int			tinwidth, tinheight;
	unsigned	playermip;
	byte		*inrow;
	unsigned	frac, fracstep;
	player_info_t *player;
	extern	byte	player_8bit_texels[320*200];
	
	GL_DisableMultitexture();

	player = &cl.players[playernum];
	playermip = (int)gl_playermip.value;

//		Con_Printf("TPS:%s %s %i %i\n",player->name, player->skin, player->topcolor, player->bottomcolor);

	bottom = player->bottomcolor;
	top = player->topcolor;

	top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
	bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);

	if (swapredblue) { 
		if (top == 4) 
			top = 13;
		else if (top == 13)
			top = 4;
		if (bottom == 4) 
			bottom = 13;
		else if (bottom == 13)
			bottom = 4;
	}

	top *= 16;
	bottom *= 16;

	for (i = 0; i < 256; i++)
		translate[i] = i;

	for (i = 0; i < 16; i++) {
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			translate[TOP_RANGE+i] = top+i;
		else
			translate[TOP_RANGE+i] = top+15-i;

		if (bottom < 128)
			translate[BOTTOM_RANGE+i] = bottom+i;
		else
			translate[BOTTOM_RANGE+i] = bottom+15-i;
	}

	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];

	//
	// locate the original skin pixels
	//
	// real model width
	tinwidth = 296;
	tinheight = 194;

	if (!player->skin)
		Skin_Find(player);
	if ((original = Skin_Cache(player->skin)) != NULL) {
		//skin data width
		inwidth = 320;
		inheight = 200;
	} else {
		original = player_8bit_texels;
		inwidth = 296;
		inheight = 194;
	}

	// because this happens during gameplay, do it fast
	// instead of sending it through gl_upload 8
	scaled_width = gl_max_size.value < 512 ? gl_max_size.value : 512;
	scaled_height = gl_max_size.value < 256 ? gl_max_size.value : 256;
	// allow users to crunch sizes down even more if they want
	scaled_width >>= playermip;
	scaled_height >>= playermip;
	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

//	Con_Printf("uploading player:%i slot:%i\n",playernum, player->skintexslot);
	GL_Bind(playertextures + player->skintexslot);

	if (VID_Is8bit()) { // 8bit texture upload
		byte *out2;

		out2 = (byte *)pixels;
		memset(pixels, 0, sizeof(pixels));
		fracstep = tinwidth*0x10000/scaled_width;
		for (i=0 ; i<scaled_height ; i++, out2 += scaled_width) {
			inrow = original + inwidth*(i*tinheight/scaled_height);
			frac = fracstep >> 1;
			for (j=0 ; j<scaled_width ; j+=4) {
				out2[j] = translate[inrow[frac>>16]];
				frac += fracstep;
				out2[j+1] = translate[inrow[frac>>16]];
				frac += fracstep;
				out2[j+2] = translate[inrow[frac>>16]];
				frac += fracstep;
				out2[j+3] = translate[inrow[frac>>16]];
				frac += fracstep;
			}
		}

		GL_Upload8_EXT ((byte *)pixels, scaled_width, scaled_height, false, false);
	} else {
		out = pixels;
		memset(pixels, 0, sizeof(pixels));
		fracstep = tinwidth*0x10000/scaled_width;
		for (i=0 ; i<scaled_height ; i++, out += scaled_width) {
			inrow = original + inwidth*(i*tinheight/scaled_height);
			frac = fracstep >> 1;
			for (j=0 ; j<scaled_width ; j+=4) {
				out[j] = translate32[inrow[frac>>16]];
				frac += fracstep;
				out[j+1] = translate32[inrow[frac>>16]];
				frac += fracstep;
				out[j+2] = translate32[inrow[frac>>16]];
				frac += fracstep;
				out[j+3] = translate32[inrow[frac>>16]];
				frac += fracstep;
			}
		}

		if (gl_ext_automipmap.value) {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		if (playermip == skinslots[player->skintexslot].mip) {
			// texture of this size was already uploaded, some speedup
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, scaled_width, scaled_height, 
				GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		} else {
			glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, 
				scaled_width, scaled_height, 0, GL_RGBA, 
				GL_UNSIGNED_BYTE, pixels);
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (Img_HasFullbrights ((byte *)original, inwidth*inheight)) {
		fb_skins[playernum] = playertextures + MAX_CLIENTS + player->skintexslot;
		skinslots[player->skintexslot].fb_skin = fb_skins[playernum];
		GL_Bind(fb_skins[playernum]);

		out = pixels;
		memset(pixels, 0, sizeof(pixels));
		fracstep = tinwidth*0x10000/scaled_width;

		// make all non-fullbright colors transparent
		for (i=0 ; i<scaled_height ; i++, out += scaled_width) {
			inrow = original + inwidth*(i*tinheight/scaled_height);
			frac = fracstep >> 1;
			for (j=0 ; j<scaled_width ; j+=4) {
				if (inrow[frac>>16] < 224)
					out[j] = translate32[inrow[frac>>16]] & LittleLong(0x00FFFFFF); // transparent
				else
					out[j] = translate32[inrow[frac>>16]]; // fullbright
				frac += fracstep;
				if (inrow[frac>>16] < 224)
					out[j+1] = translate32[inrow[frac>>16]] & LittleLong(0x00FFFFFF);
				else
					out[j+1] = translate32[inrow[frac>>16]];
				frac += fracstep;
				if (inrow[frac>>16] < 224)
					out[j+2] = translate32[inrow[frac>>16]] & LittleLong(0x00FFFFFF);
				else
					out[j+2] = translate32[inrow[frac>>16]];
				frac += fracstep;
				if (inrow[frac>>16] < 224)
					out[j+3] = translate32[inrow[frac>>16]] & LittleLong(0x00FFFFFF);
				else
					out[j+3] = translate32[inrow[frac>>16]];
				frac += fracstep;
			}
		}

		if (gl_ext_automipmap.value) {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		if (playermip == skinslots[player->skintexslot].mip) {
			// texture of this size was already uploaded, some speedup
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, scaled_width, scaled_height, 
				GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		} else {
			glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 
				scaled_width, scaled_height, 0, GL_RGBA, 
				GL_UNSIGNED_BYTE, pixels);
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		fb_skins[playernum] = 0;
		skinslots[player->skintexslot].fb_skin = 0;
	}

	ResetOldSkin(player);
//	Con_Printf ("Setting slot %i: {%s} (%i,%i)\n", player->skintexslot, player->skin->name, top/16, bottom/16);
	skinslots[player->skintexslot].mip = playermip;
	skinslots[player->skintexslot].topcolor = top/16;
	skinslots[player->skintexslot].bottomcolor = bottom/16;
	strlcpy (skinslots[player->skintexslot].skinname, player->skin->name, 16);
}
/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	texture_t  *tex;
	
	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	R_ClearParticles ();

	GL_BuildLightmaps ();

	// identify sky texture
	// BorisU: and black texture 
	skytexturenum = blacktexturenum = -1;
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		tex = cl.worldmodel->textures[i];
		if (!tex)
			continue;
		if (!strncmp(tex->name,"sky",3) )
			skytexturenum = i;
		else if (!strcmp(tex->name,"black") )
			blacktexturenum = i;

		tex->texturechain = NULL;
		tex->texturechain_tail = &tex->texturechain;
	}

}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;

	GL_EndRendering ();

	start = Sys_DoubleTime ();
	for (i = 0; i < 128; i++) {
		GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
		r_refdef.viewangles[1] = i * (360.0 / 128.0);
		R_RenderView ();
		GL_EndRendering ();
	}

	stop = Sys_DoubleTime ();
	time = stop - start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128 / time);

	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
}

void D_FlushCaches (void)
{
	// maybe it's not the right place for this code, but it serves
	// its purpose - set lightmode to gl_lightmode before loading
	// any models for a new map
	lightmode = gl_lightmode.value;
	if (lightmode < 0 || lightmode > 2)
		lightmode = 2;
}

