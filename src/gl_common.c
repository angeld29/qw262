#include "quakedef.h"
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

byte	color_white[4] = { 255, 255, 255, 0 };
byte	color_black[4] = { 0, 0, 0, 0 };

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

float		vid_gamma = 1.0;
qboolean	is8bit = false;

// remove? BorisU
//static qboolean fullsbardraw = false;

unsigned		d_8to24table[256];
unsigned		d_8to24table2[256];
unsigned char	d_15to8table[65536];
unsigned char	vid_gamma_table[256];

// multitexturing
GLenum GL_MTexture0;
GLenum GL_MTexture1;
GLenum oldtarget;

lpMTexFUNC MTexCoord2f = NULL;
lpSelTexFUNC SelectTextureMTex = NULL;

qboolean gl_mtexable = false;

int gl_textureunits = 0;

// Alpha test
float	alpha_test_threshold = 0.01;
float	alpha_test_normal	 = 0.666;

// Texture compression
int gl_has_compression = false;

// GL extensions cvars
float	gl_max_anisotropy = 0;
qboolean OnAnisotropyChange(cvar_t *var, const char *value);
qboolean OnLODbiasChange(cvar_t *var, const char *value);
qboolean OnAutomipmapChange(cvar_t *var, const char *value);

cvar_t	gl_ext_anisotropy_level	= {"gl_ext_anisotropy_level", "0", 0, OnAnisotropyChange};
cvar_t	gl_ext_lod_bias			= {"gl_ext_lod_bias", "0", 0, OnLODbiasChange};
cvar_t	gl_ext_texture_compress	= {"gl_ext_texture_compress", "0"};
cvar_t	gl_ext_automipmap		= {"gl_ext_automipmap", "0", 0, OnAutomipmapChange};

void GL_SelectTexture (GLenum target) 
{
	if (!gl_mtexable)
		return;
	
	SelectTextureMTex(target);

	if (target == oldtarget) 
		return;
	cnttextures[oldtarget-GL_MTexture0] = currenttexture;
	currenttexture = cnttextures[target-GL_MTexture0];
	oldtarget = target;
}

BINDTEXFUNCPTR bindTexFunc;

//PROC glArrayElementEXT;
//PROC glColorPointerEXT;
//PROC glTexCoordPointerEXT;
//PROC glVertexPointerEXT;

#ifndef __APPLE__
PFNGLCOLORTABLEEXTPROC glColorTableEXTFunc;
#else
void (*glColorTableEXTFunc) (GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
#endif

// BorisU -->
void* GL_GetProcAddress (char* ExtName)
{
#ifdef _WIN32
			return (void *) wglGetProcAddress(ExtName);
#else
#ifdef __APPLE__
	// Mac OS X don't have an OpenGL extension fetch function. Isn't that silly?

	static CFBundleRef cl_gBundleRefOpenGL = 0;
	if (cl_gBundleRefOpenGL == 0)
	{
		cl_gBundleRefOpenGL = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
		if (cl_gBundleRefOpenGL == 0)
			Sys_Error("Unable to find com.apple.opengl bundle");
	}

	return CFBundleGetFunctionPointerForName(
		cl_gBundleRefOpenGL,
		CFStringCreateWithCStringNoCopy(
			0,
			ExtName,
			CFStringGetSystemEncoding(),
			0));
#else
			return (void *) glXGetProcAddressARB(ExtName);
#endif /* __APPLE__ */
#endif /* _WIN32 */
}

qboolean GL_CheckExtension (char* ExtName)
{
// gl_extensions must be initialized before!
	char	*p;
	char	c;
	
	while ( (p = strstr(gl_extensions, ExtName) ) ){
		c = p[strlen(p)];
		if (*p == ' ' || *p == '\0'){
				break;
		}
			return true;
	}
	return false;
}

#ifdef _WIN32
void CheckTextureExtensions (void)
{
	qboolean	texture_ext;

	/* check for texture extension */
	texture_ext = GL_CheckExtension("GL_EXT_texture_object");

	if (!texture_ext || COM_CheckParm ("-gl11") ) {
	
		HINSTANCE	hInstGL = LoadLibrary("opengl32.dll");
		
		if (hInstGL == NULL)
			Sys_Error ("Couldn't load opengl32.dll\n");

		bindTexFunc = (void *)GetProcAddress(hInstGL,"glBindTexture");

		if (!bindTexFunc)
			Sys_Error ("No texture objects!");
		return;
	}

/* load library and get procedure adresses for texture extension API */
	if ((bindTexFunc = (BINDTEXFUNCPTR)
		wglGetProcAddress((LPCSTR) "glBindTextureEXT")) == NULL)
	{
		Sys_Error ("GetProcAddress for BindTextureEXT failed");
		return;
	}
}
#endif
// <-- BorisU

/*void CheckArrayExtensions (void)
{
	char		*tmp;

	tmp = (unsigned char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, "GL_EXT_vertex_array", strlen("GL_EXT_vertex_array")) == 0)
		{
			if (
((glArrayElementEXT = wglGetProcAddress("glArrayElementEXT")) == NULL) ||
((glColorPointerEXT = wglGetProcAddress("glColorPointerEXT")) == NULL) ||
((glTexCoordPointerEXT = wglGetProcAddress("glTexCoordPointerEXT")) == NULL) ||
((glVertexPointerEXT = wglGetProcAddress("glVertexPointerEXT")) == NULL) )
			{
				Sys_Error ("GetProcAddress for vertex extension failed");
				return;
			}
			return;
		}
		tmp++;
	}

	Sys_Error ("Vertex array extension not present");
}*/

int		texture_extension_number = 1;

qboolean CheckMultitextureExtensionsSGIS (void)
{
	if (GL_CheckExtension("GL_SGIS_multitexture")) {
		Con_Printf("SGIS Multitexture extensions found\n");
		MTexCoord2f = GL_GetProcAddress("glMTexCoord2fSGIS");
		SelectTextureMTex = GL_GetProcAddress("glSelectTextureSGIS");
		glGetIntegerv(GL_MAX_TEXTURES_SGIS, &gl_textureunits);
		Con_Printf("%d texture units found\n",gl_textureunits);
		gl_mtexable = true;
		oldtarget= GL_MTexture0 = TEXTURE0_SGIS;
		GL_MTexture1 = TEXTURE1_SGIS;
		return true;
	}
	return false;
}


qboolean CheckMultitextureExtensionsARB (void)
{
	if (GL_CheckExtension("GL_ARB_multitexture")) {
		Con_Printf("ARB Multitexture extensions found\n");
		MTexCoord2f = GL_GetProcAddress("glMultiTexCoord2fARB");
		SelectTextureMTex = GL_GetProcAddress("glActiveTextureARB");
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_textureunits);
		Con_Printf("%d texture units found\n",gl_textureunits);
		gl_mtexable = true;
		oldtarget = GL_MTexture0 = GL_TEXTURE0_ARB;
		GL_MTexture1 = GL_TEXTURE1_ARB;
		return true;
	}
	return false;
}

void CheckMultiTextureExtensions (void)
{
	gl_mtexable = false;
	gl_textureunits = 1;
	
	if (COM_CheckParm("-nomtex"))
		Con_Printf("Multitexture extensions disabled\n");
	else if (COM_CheckParm ("-sgis")) {
		if (!CheckMultitextureExtensionsSGIS ()) {
			if (!CheckMultitextureExtensionsARB ())
				Con_Printf("No multitexture extensions available\n");
		}
	} else {
		if (!CheckMultitextureExtensionsARB ()) {
			if (!CheckMultitextureExtensionsSGIS ())
				Con_Printf("No multitexture extensions available\n");
		}
	}

	// start up with the correct texture selected!
	if (gl_mtexable) 
	{
		glDisable(GL_TEXTURE_2D);
		SelectTextureMTex (GL_MTexture0);
	}
}

qboolean VID_Is8bit() {
	return is8bit;
}

void CheckAnisotropyExtensions (void)
{
	if (GL_CheckExtension("GL_EXT_texture_filter_anisotropic")) {
		Con_Printf("Anisotropic filtering extension found\n");
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max_anisotropy);
		Con_Printf("Max anisotropy level: %g\n", gl_max_anisotropy);
		Cvar_RegisterVariable (&gl_ext_anisotropy_level);
	}

	if (GL_CheckExtension("GL_EXT_texture_lod_bias")) {
//		float max_lod;
//		glGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &max_lod);
		Con_Printf("LOD bias extension found\n");
//		Con_Printf("%g\n",max_lod);
		Cvar_RegisterVariable (&gl_ext_lod_bias);
	}
}
void CheckAutoMipmapExtension (void)
{
	if (GL_CheckExtension("GL_SGIS_generate_mipmap")) {
		Con_Printf("Auto mipmap generation extension found\n");
		Cvar_RegisterVariable (&gl_ext_automipmap);
		Cvar_Set(&gl_ext_automipmap, "1");
		glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
	}
}

// Mac OS X 10.3 OpenGL Framework defines this symbol by itself
#ifndef __APPLE__
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB = NULL;
#endif

void CheckCompressionExtension (void)
{
	/*int param;*/

	if (GL_CheckExtension("GL_ARB_texture_compression")) {
		Con_Printf("Texture compression extension found\n");
		gl_has_compression = true;
		Cvar_RegisterVariable (&gl_ext_texture_compress);
#ifndef __APPLE__
		glCompressedTexImage2DARB = GL_GetProcAddress("glCompressedTexImage2DARB");
#endif
//		if (glCompressedTexImage2DARB){
//			Con_Printf("DDS textures are supported\n");
//		}
	}
}

#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB

void VID_Build15to8table (void)
{
	byte		*pal;
	unsigned	r,g,b;
	unsigned	v;
	int			r1,g1,b1;
	int			j,k,l;
	int			i;

	// 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk ?
	for (i=0; i < (1<<15); i++) {
		/* Maps
			000000000000000
			000000000011111 = Red  = 0x1F
			000001111100000 = Blue = 0x03E0
			111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,l=10000*10000; v<256; v++,pal+=4) {
			r1 = r-pal[0];
			g1 = g-pal[1];
			b1 = b-pal[2];
			j = (r1*r1)+(g1*g1)+(b1*b1);
			if (j<l) {
				k=v;
				l=j;
			}
		}
		d_15to8table[i]=k;
	}
}

void VID_Init8bitPalette() 
{
	// Check for 8bit Extensions and initialize them.
	int i;
	char thePalette[256*3];
	char *oldPalette, *newPalette;

	glColorTableEXTFunc = GL_GetProcAddress("glColorTableEXT");
	if (!glColorTableEXTFunc || !GL_CheckExtension("GL_EXT_shared_texture_palette") ||
		!COM_CheckParm("-use8bit"))
		return;

	Con_Printf("8-bit GL extensions enabled\n");
	glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
	oldPalette = (char *) d_8to24table;
	newPalette = thePalette;
	for (i=0;i<256;i++) {
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		oldPalette++;
	}
	glColorTableEXTFunc(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE,
		(void *) thePalette);
	is8bit = true;

	VID_Build15to8table ();
}

void	VID_SetPalette (unsigned char *palette)
{
	byte		*pal;
	unsigned	r,g,b;
	unsigned	v;
	unsigned	i;
	unsigned	*table;

//
// 8 8 8 encoding
//
// Macintosh has different byte order
// 
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		v = LittleLong ((255<<24) + (r<<0) + (g<<8) + (b<<16));
		*table++ = v;
	}
#ifndef __APPLE__
	d_8to24table[255] = 0;		// 255 is transparent
#else
	d_8to24table[255] &= 0xffffff00;
#endif

	// Tonik: create a brighter palette for bmodel textures
	pal = palette;
	table = d_8to24table2;

	for (i=0 ; i<256 ; i++)
	{
		r = pal[0] * (2.0 / 1.5); if (r > 255) r = 255;
		g = pal[1] * (2.0 / 1.5); if (g > 255) g = 255;
		b = pal[2] * (2.0 / 1.5); if (b > 255) b = 255;
		pal += 3;
		*table++ = LittleLong ((255<<24) + (r<<0) + (g<<8) + (b<<16));
	}
#ifndef __APPLE__
	d_8to24table2[255] = 0;	// 255 is transparent
#else
	d_8to24table2[255] &= 0xffffff00;
#endif
}

void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if (COM_CheckParm("-mtfl"))
		vid_gamma = 1;
	else if ((i = COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = Q_atof(com_argv[i+1]);

	Con_Printf("vid_gamma set to %g\n",vid_gamma);

	if (vid_gamma != 1){
		for ( i=0 ; i <256 ; i++){
			f = pow ( i / 255.0 , vid_gamma);
			inf = f*255 + 0.5;
			if (inf > 255)
				inf = 255;
			vid_gamma_table[i] = inf;
		}
	} else {
		for ( i=0 ; i <256 ; i++) {
			vid_gamma_table[i] = i;
		}
	}

	for (i=0 ; i<768 ; i++) 
		palette[i] = vid_gamma_table[pal[i]];

	memcpy (pal, palette, sizeof(palette));
}

#ifdef _WIN32
extern void CheckVsyncControlExtension(void);
#endif

void GL_InitExt (void)
{
	int i;
	glGetIntegerv(GL_DEPTH_BITS, &i);;
	Con_Printf("Z-buffer : %i bits\n",i);

	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);

	gl_extensions = glGetString (GL_EXTENSIONS);
	if (COM_CheckParm ("-gl_ext"))
		Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

// remove? BorisU
//	if (Q_strnicmp(gl_renderer,"PowerVR",7)==0)
//		fullsbardraw = true;

#ifdef _WIN32
	CheckTextureExtensions ();
#endif
	
	CheckMultiTextureExtensions ();

	CheckAnisotropyExtensions ();

	CheckAutoMipmapExtension ();

	CheckCompressionExtension ();

#ifdef _WIN32
	CheckVsyncControlExtension ();
#endif

#if 0
	CheckArrayExtensions ();

	glEnable (GL_VERTEX_ARRAY_EXT);
	glEnable (GL_TEXTURE_COORD_ARRAY_EXT);
	glVertexPointerEXT (3, GL_FLOAT, 0, 0, &glv.x);
	glTexCoordPointerEXT (2, GL_FLOAT, 0, 0, &glv.s);
	glColorPointerEXT (3, GL_FLOAT, 0, 0, &glv.r);
#endif
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	GL_InitExt();

#ifndef __APPLE__
	glClearColor (1,0,0,0);
#else
	glClearColor (0.2,0.2,0.2,1.0);
#endif
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, alpha_test_threshold);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
