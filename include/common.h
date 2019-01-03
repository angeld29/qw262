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
// comndef.h  -- general definitions

#if !defined(__GNUC__) || !defined(_WINDOWS_H)
typedef unsigned char 		byte;
#endif
#define _DEF_BYTE_

// KJB Undefined true and false defined in SciTech's DEBUG.H header
#undef true
#undef false

typedef enum {false, true}	qboolean;

#ifdef _MSC_VER
	#include <io.h>
	typedef __int64 int64_t;
	typedef __int32 int32_t;
	typedef __int16 int16_t;
	typedef __int8 int8_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int8 uint8_t;
#else
	#include <stdint.h>
#endif

#define	MAX_INFO_STRING	        255
#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768

//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

char *com_args_original; // ezquake
void COM_StoreOriginalCmdline(int argc, char **argv);

void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define bound(a,b,c) ((a) >= (c) ? (a) : \
					(b) < (a) ? (a) : (b) > (c) ? (c) : (b))

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	qboolean		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

struct usercmd_s;

extern struct usercmd_s nullcmd;

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_GetReadCount(void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);
char *MSG_ReadStringLine (void);

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);
float MSG_ReadAngle16 (void);
void MSG_ReadDeltaUsercmd (struct usercmd_s *from, struct usercmd_s *cmd);

//============================================================================

#if defined(_WIN32) && !defined(__CYGWIN__)
#define Q_strlwr(s) _strlwr((s)) 
#define Q_stricmp(s1, s2) _stricmp((s1), (s2))
#define Q_strnicmp(s1, s2, n) _strnicmp((s1), (s2), (n))
#define snprintf _snprintf
#else
char* Q_strlwr (char *s); // BorisU
#define Q_stricmp(s1, s2) strcasecmp((s1), (s2))
#define Q_strnicmp(s1, s2, n) strncasecmp((s1), (s2), (n))
#endif

// BorisU -->
// same as standard functions but
// return pointer to the end of dest string
char* Q_strcpy(char* dest, const char* src);
char* Q_strcat(char* dest, const char* src);
// <-- BorisU

int Q_atoi (const char *str);
float Q_atof (const char *str);

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif
#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif

void Q_snprintfz (char *dest, size_t size, const char *fmt, ...); // Tonik

//============================================================================
//#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
//#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

extern	char		com_token[MAX_TOKEN_CHARS];
extern	qboolean	com_eof;

char *COM_Parse (char *data);

extern	int		com_argc;
extern	char	**com_argv;

int COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);
void COM_ForceExtension (char *path, char *extension);
qboolean COM_CheckFilename (char *filename);
int COM_FileLength (FILE *f); //bliP: init

char	*va(char *format, ...);
// does a varargs printf into a temp buffer


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern char		gamedirfile[MAX_OSPATH];
extern	char	com_gamedir[MAX_OSPATH];
extern	char	com_basedir[MAX_OSPATH];

// BorisU -->
#ifndef SERVERONLY
#define UserdirSet (userdirfile[0] != '\0')
extern	char	userdirfile[MAX_OSPATH];
extern	char	com_userdir[MAX_OSPATH];
void COM_SetUserDirectory (char *dir, char *type); 
#endif
// <-- BorisU

char	com_netpath[MAX_OSPATH]; // BorisU
void COM_WriteFile (char *filename, void *data, int len);
int COM_FOpenFile (char *filename, FILE **file);
void COM_CloseFile (FILE *h);

byte *COM_LoadStackFile (char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (char *path);
byte *COM_LoadHunkFile (char *path);
void COM_LoadCacheFile (char *path, struct cache_user_s *cu);
void COM_CreatePath (char *path);
char *COM_NextPath (char *prevpath);
void COM_Gamedir (char *dir);

extern	struct cvar_s	registered;
extern qboolean		standard_quake, rogue, hipnotic;

char *Info_ValueForKey (char *s, char *key);
void Info_RemoveKey (char *s, char *key);
void Info_RemovePrefixedKeys (char *start, char prefix);
void Info_SetValueForKey (char *s, char *key, const char *value, int maxsize);
void Info_SetValueForStarKey (char *s, char *key, const char *value, int maxsize);
void Info_Print (char *s);
void Info_CopyStarKeys (char *from, char *to);

unsigned Com_BlockChecksum (void *buffer, int length);
void Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf);
byte	COM_BlockSequenceCheckByte (byte *base, int length, int sequence, unsigned mapchecksum);
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

int build_number( void );

#ifndef SERVERONLY

// regexp match support for group operations in scripts
qboolean IsRegexp(char *str);
qboolean ReSearchInit (char *wildcard);
qboolean ReSearchMatch (char *str);
void ReSearchDone (void);

char CharToBrown(char ch);
char CharToWhite(char ch);
void CharsToBrown(char* start, char* end);
void CharsToWhite(char* start, char* end);

#else

#define IsRegexp(name) (false)
#define ReSearchInit(wildcard) (true)
#define ReSearchMatch(str) (false)
#define ReSearchDone() {}

#endif
