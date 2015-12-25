/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "quakedef.h"

#include "gl_image.h"

int		gl_lightmap_format = 4;
int		gl_solid_format = 3;
int		gl_alpha_format = 4;

qboolean OnPicMipChange (cvar_t *var, const char *value);
qboolean OnChange_gl_texturemode (cvar_t *var, const char *string);
qboolean OnTextureBitsChange (cvar_t *var, const char *value);

cvar_t	gl_picmip		= {"gl_picmip", "0", 0, OnPicMipChange};
cvar_t	gl_texturemode	= {"gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", 0, OnChange_gl_texturemode};
cvar_t	gl_lerpimages	= {"r_lerpimages", "1"};
cvar_t	gl_texturebits	= {"gl_texturebits", "0", 0, OnTextureBitsChange };

extern unsigned char d_15to8table[65536];
extern unsigned d_8to24table[256];
extern unsigned d_8to24table2[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

int		texels;

#define	MAX_GLTEXTURES	1024

gltexture_t	gltextures[MAX_GLTEXTURES];
int			numgltextures;

gltexture_t	*current_texture;
byte		*already_loaded = (byte *)0xFFFFFFFF;

int		fb_texnum;
int		luma_texnum;

int		base_numgltextures;
/*
================
GL_FindTexture
================
*/
// changed by BorisU
gltexture_t* GL_FindTexture (char *identifier)
{
	int		i;
	gltexture_t *glt;

	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (!strcmp (identifier, glt->identifier))
			return glt;
	}

	return NULL;
}

void GL_UploadS3TC (unsigned *data, int width, int height,  qboolean mipmap, qboolean alpha)
{
	byte	*bdata;
	int		miplevel;

	glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, image.format, image.width, image.height, 0, 
			image.size, data);

	
	if (mipmap) {
		bdata = (byte*)data;
		for (miplevel = 1; miplevel < image.mipmaps; miplevel++) {
			bdata += image.size;
			image.width /= 2; if (!image.width) image.width = 1;
			image.height /=2; if (!image.height) image.height = 1;
			image.size = size_dxtc(image.width, image.height);

			glCompressedTexImage2DARB(GL_TEXTURE_2D, miplevel, image.format, image.width, image.height, 0, 
				image.size, bdata);
		}
	}

	if (mipmap) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if (gl_ext_anisotropy_level.value)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_ext_anisotropy_level.value);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	image.format = 0;
}

int loadtextureimage (char *identifier, int mode)
{
	int			i, j, texnum;
	qboolean	transparent = false/*, fb = false*/;

	if (image.format) {
		texnum = GL_LoadTexture (identifier, image.width, image.height,
							image.data, mode & TEX_MIPMAP, transparent, false, -4); // -4 means S3TC!
		goto done;
	}

	image.size = image.width * image.height;

	if (vid_gamma != 1)
		for (i = 0; i < image.size; i++){
			image.data[4*i  ] = vid_gamma_table[image.data[4*i  ]];
			image.data[4*i+1] = vid_gamma_table[image.data[4*i+1]];
			image.data[4*i+2] = vid_gamma_table[image.data[4*i+2]];
		}

	if (mode & TEX_ALPHA) {
		for (j = 0; j < image.size; j++) {
			if ( image.data[4*j + 3] < 255 ) {
				transparent = true;
				break;
			}
		}
	}

	if (mode & TEX_LUMA) {
		// make black areas transparent 
		// alpha test will be used 
		transparent = true;
		for (j = 0; j < image.size; j++) { 
			if (((unsigned *) image.data)[j] == LittleLong(0xFF000000)){
				((unsigned *) image.data)[j] = 0x00000000;
			}
		}
	}

	if (mode & TEX_CHARSET) {
		char	*buf = calloc(image.size*2, 4);
		char	*src = image.data;
		char	*dest = buf;

		// HACK: top-left pixel sets the transparent color
		// if there is no alpha-channel in texture
		if (!transparent){
			char	r = image.data[0];
			char	g = image.data[1];
			char	b = image.data[2];

			for (i=0 ; i< image.size; i++)
				if (( image.data[i*4]     == r) &&
					( image.data[i*4 + 1] == g) &&
					( image.data[i*4 + 2] == b))
					image.data[i*4 + 3] = 0;
		}		

		for (i=0 ; i<16 ; i++) {
			memcpy (dest, src, image.size / 4);
			src += image.size / 4;
			dest += image.size / 2;
		}
		texnum = GL_LoadTexture (identifier, image.width, image.height*2,
							buf, false, true, false, 4);
		free (buf);
	} else
		texnum = GL_LoadTexture (identifier, image.width, image.height,
							image.data, mode & TEX_MIPMAP, transparent, false, 4);

/*	24bit fullbrights turned off for now
	if ((mode & TEX_FULLBRIGHT) && (cls.allow24bit & ALLOW_24BIT_FB_TEXTURES)) {
		transparent = true;
		for (j = 0; j < image.size; j++) {
			for (i = 224; i < 256; i++) {
				if (d_8to24table[i] == ((unsigned *) image.data)[j]) {
					fb = true;
					break;
				}
			}
			if (i == 256)
				((unsigned *) image.data)[j] &= 0x00FFFFFF;
		}
		if (fb) {
			char fb_identifier[128];
			Q_strcpy(Q_strcpy(fb_identifier, "@fb_"),identifier);
			Con_Printf("FB: %s\n",identifier);
			fb_texnum = GL_LoadTexture (fb_identifier, image.width, image.height,
							image.data, mode & TEX_MIPMAP, transparent, false, 4);
		} else
			fb_texnum = 0;
	} else */
		fb_texnum = 0;

done:
	free(image.data);
	return texnum;
}

#define CHECK_SAME_TEXTURE \
	if (current_texture && current_texture->pathname \
			&& !strcmp(com_netpath,current_texture->pathname)) {\
		fclose(f); \
		return already_loaded; \
	}
//		Con_Printf ("reload!: %s\n",current_texture->identifier); 


byte* attempt_loadtexturepixels(char* name)
{
	FILE	*f;
	char	fullname[128];
	char	*name_end;

	name_end = Q_strcpy(fullname, name);

	Con_DPrintf("attempt_loadtexturepixels: %s\n", name);

	strcpy(name_end, ".link");
	COM_FOpenFile (fullname, &f);
	if (f) {
		char link[128];
		int len;
		fgets(link, 128, f);
		fclose (f);
		len = strlen(link);
		// strip endline
		if (link[len-1] == '\n') {
			link[len-1] = '\0';
			--len;
		}
		if (link[len-1] == '\r') {
			link[len-1] = '\0';
			--len;
		}
		
		Q_strcpy(Q_strcpy(fullname, "textures/"),link);
		Con_DPrintf("link: %s -> %s\n", name, link);
		COM_FOpenFile (fullname, &f);
		if (f) {
			CHECK_SAME_TEXTURE;
			if (glCompressedTexImage2DARB && !Q_stricmp(link + len - 3, "dds")) {
				return LoadDDS (f, fullname);
			} else 
#ifdef __USE_JPG__			
			if (!Q_stricmp(link + len - 3, "jpg")) {
				return LoadJPG (f, fullname);
			} else 
#endif			
#ifdef __USE_PNG__				
			if (!Q_stricmp(link + len - 3, "png")) {
				return LoadPNG (f, fullname);
			} else 
#endif
			if (!Q_stricmp(link + len - 3, "tga")) {
				return LoadTGA (f, fullname);
			} else if (!Q_stricmp(link + len - 3, "pcx")) {
				return LoadPCX (f, fullname);
			} else
				Con_Printf("Wrong texture link: %s -> %s\n",name,link);
			
			fclose(f);
			f = NULL;
		}
	}

	if (glCompressedTexImage2DARB) {
		strcpy(name_end, ".dds");
		COM_FOpenFile (fullname, &f);
		if (f) {
			CHECK_SAME_TEXTURE;
			return LoadDDS (f, fullname);
		}
	}

#ifdef __USE_JPG__
	strcpy(name_end, ".jpg");
	COM_FOpenFile (fullname, &f);
	if (f) {
		CHECK_SAME_TEXTURE;
		return LoadJPG (f, fullname);
	}
#endif

#ifdef __USE_PNG__
	strcpy(name_end, ".png");
	COM_FOpenFile (fullname, &f);
	if (f) {
		CHECK_SAME_TEXTURE;
		return LoadPNG (f, fullname);
	}
#endif

	strcpy(name_end, ".tga");
	COM_FOpenFile (fullname, &f);
	if (f) {
		CHECK_SAME_TEXTURE;
		return LoadTGA (f, fullname);
	}
	strcpy(name_end, ".pcx");
	COM_FOpenFile (fullname, &f);
	if (f) {
		CHECK_SAME_TEXTURE;
		return LoadPCX (f, fullname);
	}

	return NULL; // not found
}

int loadtexture_24bit (char* name, int type)
{
	char	pathname[128];
	char	stripped_name[128];
	char	id[64];
	byte	*data_ok = NULL;
	char	*p;
	int	mode = 0;

	Con_DPrintf("loadtexture_24bit: %s\n", name);

	for (p = name; *p; p++){
		if (*p == '*')
			*p = '#';
	}

	if (type & (LOADTEX_GFX | LOADTEX_ALIAS_MODEL | LOADTEX_SPRITE))
		COM_StripExtension(name, stripped_name);

	if (type & (LOADTEX_ALIAS_MODEL | LOADTEX_SPRITE | LOADTEX_GFX ))
		p = COM_SkipPath(stripped_name);

	memset(&image, 0, sizeof(image));

	switch (type) {
	case LOADTEX_BSP_NORMAL:
	case LOADTEX_BSP_TURB:
	case LOADTEX_SPRITE:
	case LOADTEX_ALIAS_MODEL: // ????
		strcpy (id, name);
		break;
	case LOADTEX_CONBACK:
	case LOADTEX_GFX:
	case LOADTEX_CAUSTIC:
	case LOADTEX_CHARS:
		Q_strcpy(Q_strcpy(id, "pic:"),name);
		break;
	case LOADTEX_CHARS_MAIN:
		Q_strcpy(id, "pic:main:charset");
		break;
	case LOADTEX_CROSSHAIR:
		Q_strcpy(id, "pic:crosshair5");
		break;
	case LOADTEX_QMB_PARTICLE:
		Q_strcpy(Q_strcpy(id, "qmb:"),name);
	}

	current_texture = GL_FindTexture(id); // Check if this texture already present

	switch (type) {
	case LOADTEX_BSP_NORMAL:
		sprintf(pathname, "textures/%s/%s", mapname.string, name);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok && *mapgroupname.string) {
			sprintf(pathname, "textures/%s/%s", mapgroupname.string, name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/bmodels/"), name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/"), name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok)
			mode = TEX_MIPMAP | TEX_FULLBRIGHT;
		break;
	case LOADTEX_BSP_TURB:
		Q_strcpy(stripped_name, name);
		sprintf(pathname, "textures/%s/%s", mapname.string, stripped_name);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok && *mapgroupname.string) {
			sprintf(pathname, "textures/%s/%s", mapgroupname.string, stripped_name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/bmodels/"), stripped_name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/"), stripped_name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok)
			mode = TEX_MIPMAP;
		break;
	case LOADTEX_ALIAS_MODEL:
		Q_strcpy(Q_strcpy(pathname, "textures/models/"), p);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/progs/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok){
			mode = TEX_MIPMAP | TEX_FULLBRIGHT;
		}
		break;
	case LOADTEX_SKYBOX:
		// FIX ME! integrate skybox loading into loadtextureimage
		p = Q_strcpy(Q_strcpy(pathname, "textures/skybox/"),name); // FIXME change to skyboxes?
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok) {
			p[1] = '\0'; p[0] = p[-1]; p[-1] = p[-2]; p[-2] = '_';
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			p = Q_strcpy(Q_strcpy(pathname, "env/"),name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			p[1] = '\0'; p[0] = p[-1]; p[-1] = p[-2]; p[-2] = '_';
			data_ok = attempt_loadtexturepixels(pathname);
		}

		if (data_ok) {
			glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, image.width, image.height, 
				0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

			free (image.data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			return 1;
		}
		break;
	case LOADTEX_SPRITE:
		Q_strcpy(Q_strcpy(pathname, "textures/sprites/"), p);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/progs/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok)
			mode = TEX_MIPMAP | TEX_ALPHA;
		break;
	case LOADTEX_CONBACK:
		data_ok = attempt_loadtexturepixels("textures/gfx/conback");
		if (data_ok)
			mode = 0;
		break;
	case LOADTEX_CHARS:
	case LOADTEX_CHARS_MAIN:
		Q_strcpy(Q_strcpy(pathname, "textures/charsets/"), name);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/"), name);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok) {
			mode = TEX_CHARSET | TEX_ALPHA;
		} else {
			Con_Printf ("Couldn't load %s\n", pathname);
		}
		break;
	case LOADTEX_GFX:
		p = Q_strcpy(Q_strcpy(pathname, "textures/gfx/"), p);
		data_ok = attempt_loadtexturepixels(pathname);
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "textures/wad/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (!data_ok) {
			Q_strcpy(Q_strcpy(pathname, "gfx/"), p);
			data_ok = attempt_loadtexturepixels(pathname);
		}
		if (data_ok)
			mode = TEX_ALPHA;
		break;
	case LOADTEX_CAUSTIC:
		data_ok = attempt_loadtexturepixels("textures/water_caustic");
		if (data_ok) {
			mode = TEX_ALPHA | TEX_MIPMAP;
		} else {
			Con_Printf ("Couldn't load water_caustic texture\nWater caustics disabled\n", name);
			Cvar_Set(&gl_caustics, "0");
			Cvar_SetRO(gl_caustics);
		}
		break;
	case LOADTEX_CROSSHAIR:
		p = Q_strcpy(Q_strcpy(pathname, "textures/crosshairs/"), name);
		data_ok = attempt_loadtexturepixels(pathname);
		if (data_ok) {
			mode = TEX_ALPHA;
		} else {
			Con_Printf ("Couldn't load %s\n", pathname);
		}
		break;
	case LOADTEX_QMB_PARTICLE:
		p = Q_strcpy(Q_strcpy(pathname, "textures/particles/"), name);
		data_ok = attempt_loadtexturepixels(pathname);
		if (data_ok) {
			mode = TEX_MIPMAP | TEX_ALPHA;
		} else {
			Con_Printf ("Couldn't load particle texture: %s\n", name);
		}
	}

	if (data_ok) {
		int texnum;
		if (data_ok == already_loaded)
			texnum = current_texture->texnum;
		else
			texnum = loadtextureimage(id, mode);
		// loading LUMA texture
		if (type == LOADTEX_BSP_NORMAL && 
				( (cls.allow24bit & ALLOW_24BIT_FB_TEXTURES) ||
				(name[0] == '+') ||
				strstr(name, "light") ||  strstr(name, "lit") ||  strstr(name, "rune") ||
				strstr(name, "ceil1") ) ) {
			Q_strcpy(Q_strcpy(id, "luma:"),name);
			current_texture = GL_FindTexture(id);
			Q_strcat(pathname, "_luma");
			data_ok = attempt_loadtexturepixels(pathname);
			if (data_ok){
				if (data_ok == already_loaded)
					luma_texnum = current_texture->texnum;
				else {
					Con_DPrintf("LUMA loaded: %s\n",pathname);
					luma_texnum = loadtextureimage(id, TEX_MIPMAP | TEX_LUMA);
				}
			} else
				luma_texnum = 0;
		} else
			luma_texnum = 0;
		return texnum;
	} else {
		com_netpath[0] = '\0';
		return 0;
	}
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture (char *identifier, int width, int height, byte *data, qboolean mipmap, qboolean alpha, qboolean brighten, int bytesperpixel) {
	int		i, crc = 0;
	gltexture_t	*glt;

	if (lightmode != 2)
		brighten = false;

	// see if the texture is allready present
	if (identifier[0]) {
		if (bytesperpixel>0)
			crc = CRC_Block (data, width * height * bytesperpixel);
		else
			crc = CRC_Block (data, image.size);
		for (i = 0, glt = gltextures ; i < numgltextures ; i++, glt++) {
			if (!strncmp (identifier, glt->identifier, sizeof(glt->identifier) - 1)) {
				if (width == glt->width && height == glt->height && crc == glt->crc && 
				brighten == glt->brighten && glt->bytesperpixel == bytesperpixel)
					return gltextures[i].texnum;	//texture cached
				else
					goto GL_LoadTexture_setup;		// reload the texture into the same slot
			}
		}
	}

	glt = &gltextures[numgltextures];
	if (numgltextures == MAX_GLTEXTURES)
		Sys_Error ("GL_LoadTexture: numgltextures == MAX_GLTEXTURES");
	numgltextures++;

	strlcpy (glt->identifier, identifier, sizeof(glt->identifier));
	glt->texnum = texture_extension_number;
	texture_extension_number++;

GL_LoadTexture_setup:
	glt->width = width;
	glt->height = height;
	glt->mipmap = mipmap;
	glt->brighten = brighten;
	glt->crc = crc;
	glt->bytesperpixel = bytesperpixel;

	GL_Bind(glt->texnum);

	if (bytesperpixel == 1)
		GL_Upload8 (data, width, height, mipmap, alpha, brighten);
	else if (bytesperpixel == 4)
		GL_Upload32 ((void *) data, width, height, mipmap, alpha);
	else if (bytesperpixel == -4)
		GL_UploadS3TC ((void *) data, width, height, mipmap, alpha);
	else
		Sys_Error("GL_LoadTexture: unknown bytesperpixel\n");

	if (glt->pathname)
		Z_Free(glt->pathname);
	if (com_netpath[0]) {
		glt->pathname = Z_StrDup(com_netpath);
		com_netpath[0]='\0'; // clear the path
	} else
		glt->pathname = NULL;
	return glt->texnum;
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture (char *name, mpic_t *pic, byte *data)
{
	int		glwidth, glheight;
	int		i;
	char	fullname[64] = "pic:";

	for (glwidth = 1 ; glwidth < pic->width ; glwidth<<=1)
		;
	for (glheight = 1 ; glheight < pic->height ; glheight<<=1)
		;
	strlcpy (fullname + 4, name, sizeof(fullname)-4);

	if (glwidth == pic->width && glheight == pic->height) {
		pic->texnum = GL_LoadTexture (fullname, glwidth, glheight, data,
						false, true, false, 1);
		pic->sl = 0.05; // BorisU: hack for better drawing of TextBox
		pic->sh = 0.95;
		pic->tl = 0.05;
		pic->th = 0.95;
	} else {
		byte *src, *dest;
		byte *buf;

		buf = malloc (glwidth*glheight);

		memset (buf, 255, glwidth*glheight);
		src = data;
		dest = buf;
		for (i=0 ; i<pic->height ; i++) {
			memcpy (dest, src, pic->width);
			src += pic->width;
			dest += glwidth;
		}

		pic->texnum = GL_LoadTexture (fullname, glwidth, glheight, buf,
						false, true, false, 1);
		pic->sl = 0;
		pic->sh = (float)pic->width / glwidth;
		pic->tl = 0;
		pic->th = (float)pic->height / glheight;

		free (buf);
	}

	return pic->texnum;
}

void R_ResampleTextureLerpLine (byte *in, byte *out, int inwidth, int outwidth)
{
	int		j, xi, oldx = 0, f, fstep, endx;
	fstep = (int) (inwidth*65536.0f/outwidth);
	endx = (inwidth-1);
	for (j = 0,f = 0;j < outwidth;j++, f += fstep)
	{
		xi = (int) f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			int lerp = f & 0xFFFF;
			*out++ = (byte) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (byte) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (byte) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (byte) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}

/*
================
GL_ResampleTexture
================
GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);
*/
void GL_ResampleTexture (unsigned *indata, int inwidth, int inheight,
		unsigned *outdata, int outwidth, int outheight)
{
	if (gl_lerpimages.value)
	{
		int		i, j, yi, oldy, f, fstep, endy = (inheight-1);
		byte	*inrow, *out, *row1, *row2;
		out = (byte *) outdata;
		fstep = (int) (inheight*65536.0f/outheight);

		row1 = malloc(outwidth*4);
		row2 = malloc(outwidth*4);
		inrow = (byte *) indata;
		oldy = 0;
		R_ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);
		R_ResampleTextureLerpLine (inrow + inwidth*4, row2, inwidth,
				outwidth);
		for (i = 0, f = 0;i < outheight;i++,f += fstep)
		{
			yi = f >> 16;
			if (yi < endy)
			{
				int lerp = f & 0xFFFF;
				if (yi != oldy)
				{
					inrow = (byte *)indata + inwidth*4*yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth*4);
					else
						R_ResampleTextureLerpLine (inrow, row1, inwidth,
								outwidth);
					R_ResampleTextureLerpLine (inrow + inwidth*4, row2,
							inwidth, outwidth);
					oldy = yi;
				}
				for (j=outwidth ; j ; j--)
				{
					out[0] = (byte) ((((row2[ 0] - row1[ 0]) * lerp) >> 16)
							+ row1[ 0]);
					out[1] = (byte) ((((row2[ 1] - row1[ 1]) * lerp) >> 16)
							+ row1[ 1]);
					out[2] = (byte) ((((row2[ 2] - row1[ 2]) * lerp) >> 16)
							+ row1[ 2]);
					out[3] = (byte) ((((row2[ 3] - row1[ 3]) * lerp) >> 16)
							+ row1[ 3]);
					out += 4;
					row1 += 4;
					row2 += 4;
				}
				row1 -= outwidth*4;
				row2 -= outwidth*4;
			}
			else
			{
				if (yi != oldy)
				{
					inrow = (byte *)indata + inwidth*4*yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth*4);
					else
						R_ResampleTextureLerpLine (inrow, row1, inwidth,
								outwidth);
					oldy = yi;
				}
				memcpy(out, row1, outwidth * 4);
			}
		}
		free(row1);
		free(row2);
	}
	else
	{
		int i, j;
		unsigned frac, fracstep;
		unsigned int *inrow, *out;
		out = outdata;

		fracstep = inwidth*0x10000/outwidth;
		for (i = 0;i < outheight;i++)
		{
			inrow = (int *)indata + inwidth*(i*inheight/outheight);
			frac = fracstep >> 1;
			for (j = outwidth >> 2 ; j ; j--)
			{
				out[0] = inrow[frac >> 16]; frac += fracstep;
				out[1] = inrow[frac >> 16]; frac += fracstep;
				out[2] = inrow[frac >> 16]; frac += fracstep;
				out[3] = inrow[frac >> 16]; frac += fracstep;
				out += 4;
			}
			for (j = outwidth & 3 ; j ; j--)
			{
				*out = inrow[frac >> 16]; frac += fracstep;
				out++;
			}
		}
	}
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture (unsigned char *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	char *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;
	unsigned short	r,g,b;
	byte	*at1, *at2, *at3, *at4;

	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
		for (j=0 ; j<width ; j+=2, out+=1, in+=2)
		{
			at1 = (byte *) &d_8to24table[in[0]];
			at2 = (byte *) &d_8to24table[in[1]];
			at3 = (byte *) &d_8to24table[in[width+0]];
			at4 = (byte *) &d_8to24table[in[width+1]];

			r = (at1[0]+at2[0]+at3[0]+at4[0]); r>>=5;
			g = (at1[1]+at2[1]+at3[1]+at4[1]); g>>=5;
			b = (at1[2]+at2[2]+at3[2]+at4[2]); b>>=5;

			out[0] = d_15to8table[(r<<0) + (g<<5) + (b<<10)];
		}
}

/*
===============
GL_Upload32
===============
*/
void GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean alpha)
{
	int		internalformat;
//	int		compressed;
	int		scaled_width, scaled_height;

	unsigned	*scaled = NULL;
	qboolean	dynamic = false;
	static	unsigned	static_scaled[1024*512];	// [512*256];

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;

	if (mipmap) {
		scaled_width >>= (int)gl_picmip.value;
		scaled_height >>= (int)gl_picmip.value;
	}

	scaled_width = bound(1, scaled_width, gl_max_size.value);
	scaled_height = bound(1, scaled_height, gl_max_size.value);

	if (scaled_width * scaled_height > 2048 * 2048) {
		Sys_Error ("GL_LoadTexture: too big");
	} else if (scaled_width * scaled_height > 512 * 256) {
		scaled = malloc(scaled_width * scaled_height * 4); 
		dynamic = true;
	} else {
		scaled = static_scaled;
		dynamic = false;
	}
	
	internalformat = alpha ? gl_alpha_format : gl_solid_format;
	
	if (gl_ext_texture_compress.value 
		&& ((scaled_width*scaled_height) >= (1<<(int)gl_ext_texture_compress.value)))
		internalformat = (alpha) ? GL_COMPRESSED_RGBA_ARB : GL_COMPRESSED_RGB_ARB;

	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height) {
		if (!mipmap) {
			glTexImage2D (GL_TEXTURE_2D, 0, internalformat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

	if (mipmap && gl_ext_automipmap.value){
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	}

	glTexImage2D (GL_TEXTURE_2D, 0, internalformat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

	if (mipmap && !gl_ext_automipmap.value) {
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1) {
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
			glTexImage2D (GL_TEXTURE_2D, miplevel, internalformat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}
	}

done:
	if (mipmap) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if (gl_ext_anisotropy_level.value)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_ext_anisotropy_level.value);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	if (dynamic)
		free(scaled);

//	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
//	/* if the compression has been successful */
//	if (compressed == GL_TRUE) {
//		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
//	}
//	Con_Printf("Compress: %i %i\n", compressed, internalformat);
}

void GL_Upload8_EXT (byte *data, int width, int height,  qboolean mipmap, qboolean alpha) 
{
	int			i, s;
	qboolean	noalpha;
	int			samples;
	static byte scaled[1024*512];	// [512*256];
	int			scaled_width, scaled_height;

	s = width*height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (alpha)
	{
		noalpha = true;
		for (i=0 ; i<s ; i++)
		{
			if (data[i] == 255)
				noalpha = false;
		}

		if (alpha && noalpha)
			alpha = false;
	}
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;

	if (mipmap) {
		scaled_width >>= (int)gl_picmip.value;
		scaled_height >>= (int)gl_picmip.value;
	}

	scaled_width = bound(1, scaled_width, gl_max_size.value);
	scaled_height = bound(1, scaled_height, gl_max_size.value);

	if (scaled_width * scaled_height > sizeof(scaled))
		Sys_Error ("GL_LoadTexture: too big");

	samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX , GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy (scaled, data, width*height);
	}
	else
		GL_Resample8BitTexture (data, width, height, scaled, scaled_width, scaled_height);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap8Bit ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
			glTexImage2D (GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
		}
	}

done:

	if (mipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if (gl_ext_anisotropy_level.value)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_ext_anisotropy_level.value);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

extern qboolean VID_Is8bit();
#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256
extern byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];

/*
===============
GL_Upload8
===============
*/
void GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean alpha, qboolean brighten)
{
	int			i, s;
	int			p;
	unsigned	*trans, *table;

	if (VID_Is8bit() && !alpha && (data!=scrap_texels[0])) {
		GL_Upload8_EXT (data, width, height, mipmap, alpha);
		return;
	}

	if (brighten)
		table = d_8to24table2;
	else
		table = d_8to24table;

	s = width * height;

	trans = malloc (s * sizeof(*trans));

	if (alpha == 2)	{
	// this is a fullbright mask, so make all non-fullbright
	// colors transparent
		for (i = 0 ; i<s ; i++) {
			p = data[i];
			if (p < 224)
				trans[i] = table[p] & LittleLong(0x00FFFFFF);	// transparent 
			else
				trans[i] = table[p];				// fullbright
		}
	} else if (alpha) {
	// if there are no transparent pixels, make it a 4 component
	// texture even if it was specified as otherwise
		alpha = false;
		for (i = 0 ; i < s ; i++) {
			p = data[i];
			if (!alpha && p == 255)
				alpha = true;
			trans[i] = table[p];
		}
	} else {
		if (s & 3)
			Sys_Error ("GL_Upload8: s&3");
		for (i = 0 ; i < s ; i += 4) {
			trans[i] = table[data[i]];
			trans[i+1] = table[data[i+1]];
			trans[i+2] = table[data[i+2]];
			trans[i+3] = table[data[i+3]];
		}
	}

	GL_Upload32 (trans, width, height, mipmap, alpha);
	free (trans);
}

// BorisU -->
qboolean OnAnisotropyChange(cvar_t *var, const char *value)
{
	int			i;
	gltexture_t	*glt;
	float		newvalue = Q_atof(value);
	
	if (newvalue < 0 || newvalue > gl_max_anisotropy) {
		Con_Printf("Invalid anisotropy level\n");
		return true;
	}
	// change all the existing texture objects
	// TODO affect only some textures?
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->mipmap)
		{
			GL_Bind (glt->texnum);
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, newvalue );
		}
	}

	return false;
}

qboolean OnLODbiasChange(cvar_t *var, const char *value)
{
	float	newvalue = Q_atof(value);
	
	if (newvalue < -3.0f || newvalue > 3.0f) {
		Con_Printf("Invalid LOD bias\n");
		return true;
	}

	glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, newvalue );

	return false;
}

qboolean OnAutomipmapChange(cvar_t *var, const char *value)
{
	int			i;
	gltexture_t	*glt;
	int automipmap = ( Q_atof(value) != 0 );
	
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++) {
		if (glt->mipmap)
		{
			GL_Bind (glt->texnum);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, automipmap );
		}
	}

	return false;
}

qboolean OnPicMipChange (cvar_t *var, const char *value)
{
//	if (Q_atof(value) > 3.0)
//		return true;
//	else
		return false;
}

qboolean OnTextureBitsChange (cvar_t *var, const char *value)
{
	int tb = Q_atoi(value);

	if (tb != 0 && tb != 16 && tb != 32){
		Con_Printf ("Invalid gl_texturebits value\n");
		return true;
	}
	switch (tb){
	case 16:
		gl_solid_format = GL_RGB5;
		gl_alpha_format = GL_RGBA4;
		break;
	case 32:
		gl_solid_format = GL_RGB8;
		gl_alpha_format = GL_RGBA8;
		break;
	default:
		gl_solid_format = 3;
		gl_alpha_format = 4;
	}

	return false;
}
// <-- BorisU

// fuh -->
qboolean OnChange_gl_texturemode (cvar_t *var, const char *string)
{
	int		i;
	gltexture_t	*glt;

	for (i=0 ; i<6 ; i++) {
		if (!Q_stricmp (modes[i].name, string ) )
			break;
	}

	if (i == 6)
	{
		Con_Printf ("bad filter name: %s\n", string);
		return true;	// don't change the cvar
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++) {
		if (glt->mipmap) {
			GL_Bind (glt->texnum);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}

	return false;
}
// <-- fuh

// BorisU -->
void GL_LoadCrosshair_f(void)
{
	extern cvar_t crosshair;
	if (Cmd_Argc() != 2) {
		Con_Printf("usage: load_crosshair <name>\n");
		return;
	}
	crosshair_texture = loadtexture_24bit(Cmd_Argv(1), LOADTEX_CROSSHAIR);
	if (crosshair_texture)
		Cvar_Set(&crosshair, "5");
}

void GL_LoadCharset_f(void)
{
	int		ct;
	if (Cmd_Argc() != 2) {
		Con_Printf("usage: load_charset <name>\n");
		return;
	}

	ct = loadtexture_24bit (Cmd_Argv(1), LOADTEX_CHARS_MAIN);
}

void GL_TexturesList_f(void)
{
	gltexture_t *t;
	int isResident;
	for (t=gltextures; t<gltextures+MAX_GLTEXTURES ; t++) {
		if (!t->texnum) continue;
		GL_Bind(t->texnum);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &isResident);
		Con_Printf("%4i %4i x%4i res=%i %s\n",t-gltextures, t->width, t->height, isResident, t->identifier);
		Con_Printf("path=%s\n",t->pathname);
	}
}
// <-- BorisU

void GL_TextureInfo_f(void)
{
	gltexture_t *t;
	int	i = Q_atoi(Cmd_Argv(1));
	int Rsize, Gsize, Bsize, Asize, width, height, size, format;
	int isResident, isCompressed/*, RealSize*/;

	t= gltextures + i;
	if (i<0 || i >= MAX_GLTEXTURES || t->texnum == 0) {
		Con_Printf("Invalid texture number\n");
		return;
	}

	t= gltextures + i;
	Con_Printf("identifier=%s width=%i height=%i\n", t->identifier, t->width, t->height);
	GL_Bind(t->texnum);
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &Rsize);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &Gsize);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &Bsize);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &Asize);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &isResident);

	isCompressed = 0;
	if (gl_has_compression){
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &isCompressed);
	}

	if (isCompressed) {
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_IMAGE_SIZE_ARB, &size);
	} else {
		size = (Rsize+Gsize+Bsize+Asize)/8;

		if (size==3) size=4;

		size =  size * width * height;
	}

	Con_Printf("WxH=%ix%i R:G:B:A=%i:%i:%i:%i format=%x\n", width, height, Rsize, Gsize, Bsize, Asize, format);
	Con_Printf("IsResident=%i IsCompressed=%i Size=%iK\n ", isResident, isCompressed, size/1024);
}

int GL_TextureSize(int texnum)
{
	int isCompressed, size;
	GL_Bind(texnum);
	
	isCompressed = 0;
	if (gl_has_compression){
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &isCompressed);
	}

	if (isCompressed) {
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_IMAGE_SIZE_ARB, &size);
	} else {
		int Rsize, Gsize, Bsize, Asize, width, height;

		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &Rsize);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &Gsize);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &Bsize);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &Asize);

		size = (Rsize+Gsize+Bsize+Asize)/8;
		
		if (size==3) size=4;

		size =  size * width * height;
	}
	return size;
}

void GL_TexturesSize_f(void)
{
	int size = 0, res_size = 0, s, isResident;
	gltexture_t *t;
	for (t=gltextures; t<gltextures+MAX_GLTEXTURES ; t++) {
		if (!t->texnum) continue;
		s = GL_TextureSize(t->texnum);
		if (t->mipmap)
			s = s * 4 / 3; 
		
		Con_DPrintf("%s -> %i\n",t->identifier, s);
		
		size += s;

		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &isResident);
		if (isResident)
			res_size += s;
	}

	Con_Printf("Total size of textures   : %gMb\n", size/1024.0/1024.0);
	Con_Printf("Size of resident textures: %gMb\n", res_size/1024.0/1024.0);
}

void GL_Flush_Textures_f(void)
{
	gltexture_t *t;
	GLsizei		num = 0;
	GLuint		texnums[MAX_GLTEXTURES];

	if (cls.state >= ca_connected) {
		Con_Printf ("You should be disconnected to flush textures\n");
		return;
	}

	for (t=gltextures + base_numgltextures; t<gltextures+MAX_GLTEXTURES ; t++) {
		if (!t->texnum || t->pathname == NULL) continue;
		Con_DPrintf("FLUSH: %s -> {%s}\n", t->identifier, t->pathname);
		texnums[num++] = t->texnum;
		t->brighten = t->bytesperpixel = t->crc = 
			t->mipmap = t->texnum = t->width = t->height = 0;
		t->identifier[0] = '\0';
		if (t->pathname)
			Z_Free(t->pathname);
		t->pathname = NULL;
	}
	if (num)
		glDeleteTextures(num, texnums);
	numgltextures = base_numgltextures;
}

void Init_GL_textures (void)
{
	Cvar_RegisterVariable (&gl_picmip);
	Cvar_RegisterVariable (&gl_lerpimages);
	Cvar_RegisterVariable (&gl_texturemode);
	Cvar_RegisterVariable (&gl_texturebits);

	Cmd_AddCommandTrig ("load_crosshair", GL_LoadCrosshair_f);
	Cmd_AddCommand ("load_charset", GL_LoadCharset_f);

	Cmd_AddCommand ("gl_textures_list", GL_TexturesList_f);
	Cmd_AddCommand ("gl_texture_info", GL_TextureInfo_f);
	Cmd_AddCommand ("gl_textures_size", GL_TexturesSize_f);
	Cmd_AddCommand ("gl_flush_textures", GL_Flush_Textures_f);
}
