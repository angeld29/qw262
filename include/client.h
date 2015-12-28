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
// client.h


typedef struct
{
	char		name[16];
	qboolean	failedload;		// the name isn't a valid skin
	cache_user_t	cache;
} skin_t;

// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int			messagenum;		// all player's won't be updated each frame

	double		state_time;		// not the same as the packet time,
								// because player commands come asyncronously
	usercmd_t	command;		// last command for prediction

	vec3_t		origin;
	vec3_t		viewangles;		// only for demos, not from server
	vec3_t		velocity;
	int			weaponframe;

	int			modelindex;
	int			frame;
	int			skinnum;
	int			effects;

	int			flags;			// dead, gib, etc

	float		waterjumptime;
	int			onground;		// -1 = in air, else pmove entity number
	int			oldbuttons;
	int			jump_msec;		// fix bunny-hop flickering; Tonik
} player_state_t;


#define	MAX_SCOREBOARDNAME	16
#define MAX_NAMELENGTH		32
typedef struct player_info_s
{
	int		userid;
	char	userinfo[MAX_INFO_STRING];

	// scoreboard information
	char	name[MAX_SCOREBOARDNAME];
	char	longname[MAX_NAMELENGTH];
	float	entertime;
	int		frags;
	int		ping;
	byte	pl;
	char	team[16+1];
	// skin information
	int		topcolor;
	int		bottomcolor;
	skin_t	*skin;

	// old skin information
	int		_topcolor;
	int		_bottomcolor;
	skin_t	*_skin;

#ifdef GLQUAKE
	int			skintexslot; // BorisU
#endif

	byte	translations[VID_GRADES*256];

	int		spectator;
	
// Highlander -->
	int		stats[MAX_CL_STATS];	// health, etc
	int		prevcount; // for delta update from previous
// <-- Highlander
#ifdef USE_AUTH
// ezquake -->
	qboolean	validated;		//for authentication
	char		f_server[16];	//for f_server responses
// <-- ezquake
#endif
} player_info_t;


typedef struct
{
	// generated on client side
	usercmd_t	cmd;		// cmd that generated the frame
	double		senttime;	// time cmd was sent off
	int			delta_sequence;		// sequence number to delta from, -1 = full update

	// received from server
	double		receivedtime;	// time message was received, or -1
	player_state_t	playerstate[MAX_CLIENTS];	// message received that reflects performing
							// the usercmd
	packet_entities_t	packet_entities;
	qboolean	invalid;		// true if the packet_entities delta was invalid
} frame_t;

typedef struct
{
	entity_state_t	baseline;
	entity_state_t	current;

	vec3_t			old_origin;
	vec3_t			old_angles;
	int				oldframe;

	int				sequence;

	double			startlerp;
	double			deltalerp;
	double			frametime;
} centity_t;

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4


//
// client_state_t should hold all pieces of the client state
//
#define	MAX_DLIGHTS		64

// Tonik -->
typedef enum { lt_default, lt_blue, lt_red, lt_redblue, lt_green, lt_white, lt_muzzleflash,
lt_explosion, lt_rocket, lt_rocket_trail_1, lt_rocket_trail_2, NUM_DLIGHTTYPES } dlighttype_t;
// <-- Tonik

typedef struct
{
	int		key;				// so entities can reuse same entry
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		type;				// Tonik
	int		bubble;				// non zero means no flashblend bubble; fuh
} dlight_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

dlighttype_t dlightColor(float f, dlighttype_t def, qboolean random);

#define	MAX_EFRAGS		512

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
	ca_disconnected, 	// full screen console with no connection
	ca_demostart,		// starting up a demo
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_onserver,		// processing data lists, donwloading, etc
	ca_active			// everything is in, so frames can be rendered
} cactive_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
// connection information
	cactive_t	state;
	
	int			framecount;
	double		realtime;		// scaled by cl_demospeed
	double		demotime;		// scaled by cl_demospeed, reset when starting a demo
	double		trueframetime;	// time since last frame
	double		frametime;		// time since last frame, scaled by cl_demospeed
	
// network stuff
	netchan_t	netchan;

// private userinfo for sending to masterless servers
	char		userinfo[MAX_INFO_STRING];

	char		servername[MAX_OSPATH];	// name of server from original connect

	int			qport;

	FILE		*download;		// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;
	int			downloadrate;

// bliP -->
	FILE		*upload;		// file transfer to server
	char		uploadname[MAX_OSPATH];
	int			uploadpercent;
	int			uploadrate;
	qboolean	is_file;
	byte		*mem_upload;
	int			upload_pos;
	int			upload_size;
// <-- bliP

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
// Highlander -->
	qboolean	mvdplayback; // playing mvd 
	int			lastto;
	int			lasttype;
	qboolean	findtrack;
// <-- Highlander
	qboolean	timedemo;
	FILE		*demoplayfile;
	FILE		*demorecordfile;
	double		demostarttime;		// fuh
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

	int			challenge;

	float		latency;		// rolling average

#ifdef GLQUAKE
	int		allow24bit;
	int		allow_transparent_water;
#endif

} client_static_t;

extern client_static_t	cls;

// Highlander -->
typedef struct
{
	qboolean	interpolate;
	vec3_t		origin;
	vec3_t		angles;
	int			oldindex;
} interpolate_t;
// <-- BorisU

#define	MAX_PROJECTILES	32

#define FPD_NO_TEAM_MACROS		1
#define FPD_NO_TIMERS			2
#define FPD_NO_SOUNDTRIGGERS	4			// disables triggers

#define FPD_LIMIT_PITCH			(1 << 14)	// disables script rj
#define FPD_LIMIT_YAW			(1 << 15)	// disables script fwrj


//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			servercount;	// server identification for prespawns

	char		serverinfo[MAX_SERVERINFO_STRING];

// Tonik -->
	qboolean	teamfortress;	// TeamFortess game
	int			fpd;			// proxy flags
	int			teamplay;
	float		maxfps;			// maximum fps allowed
// <-- Tonik

	int			parsecount;		// server message counter
	int			oldparsecount;	// previouse server message used for interpolation (Highlander)
	int			validsequence;	// this is the sequence number of the last good
								// packetentity_t we got.  If this is 0, we can't
								// render a frame yet
// Tonik -->
	int			oldvalidsequence;
	int			delta_sequence;	// sequence number of the packet we can request
								// delta from
// <-- Tonik
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame

	int			spectator;

	double		last_ping_request;	// while showing scoreboard
	double		last_servermessage;

// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK] = cmd
	frame_t		frames[UPDATE_BACKUP];

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;		// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  And only reset at level change
// and teleport times
	vec3_t		viewangles;

// the client simulates or interpolates movement to get these values
	double		time;			// this is the time value that the client
								// is rendering at.  allways <= realtime
	vec3_t		simorg;
	vec3_t		simvel;
	vec3_t		simangles;

// pitch drifting vars
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;


	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server

	float		punchangle;		// temporar yview kick from weapon firing
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched ffrom time at intermission start
	
//
// information that is static for the entire time connected to a server
//
	char		model_name[MAX_MODELS][MAX_QPATH];
	char		sound_name[MAX_SOUNDS][MAX_QPATH];

	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			playernum;
	int			viewplayernum;	// either playernum or spec_track (in chase camera mode) Tonik

// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	struct efrag_s	*free_efrags;
	int			num_entities;	// stored bottom up in cl_entities array
	int			num_statics;	// stored top down in cl_entitiers

	int			cdtrack;		// cd audio

	centity_t	viewent;		// weapon model

// all player information
	player_info_t	players[MAX_CLIENTS];

// Highlander -->
// interpolation stuff
	interpolate_t	int_projectiles[MAX_PROJECTILES];
	int				int_packet;
	int				int_prevnum[MAX_CLIENTS];
// <-- Highlander

	float		fbskins;  // fuh
	float		truelightning;
} client_state_t;

#ifdef GLQUAKE
// model interpolation
typedef struct
{
	int frame;
	int oldframe;
	float blend;
	float start_time;
} interpol_t;

extern interpol_t playerframes[MAX_CLIENTS];
#endif

//
// cvars
//

extern	cvar_t	cl_warncmd;

// fuh -->
extern cvar_t r_lightflicker;
extern cvar_t r_rocketlightcolor;
extern cvar_t r_flagcolor;
// <-- fuh

// BorisU -->
extern	cvar_t	cl_warnexec;
extern	cvar_t	cl_prefixchar;
extern	char	Cmd_PrefixChar;
extern	cvar_t	cl_substsinglequote;
extern	cvar_t	cl_stringescape;
extern	cvar_t	cl_curlybraces;
extern	cvar_t	cl_loadlocs;
extern	cvar_t	cl_loctype;
extern	cvar_t	cl_demoplay_restrictions;
extern	cvar_t	cl_spectator_restrictions;
extern	cvar_t	cl_swapredblue;
extern	cvar_t	cl_ignore_topcolor;
// <-- BorisU

// -=MD=- -->
#ifndef SERVERONLY /* Fix the curses.h conflict --rzhe */
extern cvar_t filter;
extern cvar_t uid_cvar;
#endif
// <-- -=MD=-

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_sbar;
extern	cvar_t	cl_hudswap;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

extern cvar_t	_windowed_mouse;

extern	cvar_t	name;

// Tonik -->
extern	cvar_t	r_fastsky;
extern	cvar_t	r_rocketlight;
extern	cvar_t	cl_muzzleflash;
extern	cvar_t	r_rockettrail;
extern	cvar_t	r_grenadetrail;
extern	cvar_t	cl_explosion;
extern	cvar_t	r_explosionlightcolor;
extern	cvar_t	r_explosionlight;
extern	cvar_t	r_explosiontype;
extern	cvar_t	r_powerupglow;
extern	cvar_t	cl_truelightning;
extern	cvar_t	cl_nofake;
// <-- Tonik

#define	MAX_STATIC_ENTITIES	128			// torches, etc

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	centity_t		cl_entities[MAX_EDICTS];
extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];

extern	qboolean	nomaster;
extern float	server_version;	// version of server we connected to

//=============================================================================


//
// cl_main
//
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);

void CL_Init (void);
void Host_WriteConfiguration (void);

void CL_EstablishConnection (char *host);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);
qboolean CL_DemoBehind(void);

void CL_BeginServerConnect(void); 

#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts, cl_oldnumvisedicts;
extern	entity_t		*cl_visedicts, *cl_oldvisedicts;
extern	entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

extern char emodel_name[], pmodel_name[], prespawn_name[], modellist_name[], soundlist_name[];
extern unsigned short pmodel_crc, emodel_crc;
extern qboolean IsMDserver;

extern int mtfl;

extern cvar_t	team;
extern cvar_t	mapname;
extern cvar_t	mapgroupname;
extern cvar_t	cl_ping;
extern long		capture_num; // surmoclient
char capture_path[MAX_OSPATH];

extern int id_PrintScr;
extern qboolean com_blockscripts;

//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void Demo_Init (void);

void CL_StopPlayback (void);
qboolean CL_GetMessage (void);
void CL_WriteDemoCmd (usercmd_t *pcmd);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

void CL_Write_SVCHeader (void);
void CL_WriteSVC (byte *data, size_t length);
void CL_WriteDemoEntities (void);
void CL_Write_SVCEnd (void);

extern float nextdemotime, olddemotime;		

//
// cl_parse.c
//
#define NET_TIMINGS 256
#define NET_TIMINGSMASK 255
extern int	packet_latency[NET_TIMINGS];
int CL_CalcNet (void);
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);
qboolean	CL_CheckOrDownloadFile (char *filename);
qboolean CL_IsUploading(void);
void CL_NextUpload(void);
void CL_StartUpload (byte *data, int size);
void CL_StopUpload(void);
void CL_StartFileUpload(void); // bliP
void CL_ProcessServerInfo (void);
void CL_ParseClientdata (void);

//
// view.c
//
extern cshift_t	cshift_empty; 
extern cvar_t v_idlescale;
extern cvar_t scr_fov;
extern cvar_t default_fov;

void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);
void V_CalcBlend (void);

//
// cl_tent
//
void CL_InitTEnts (void);
void CL_ClearTEnts (void);

//
// cl_ents.c
//
// fuh -->
typedef enum cl_modelindex_s {
	mi_spike, mi_player, mi_flag, 
	mi_tf_flag, mi_tf_stan, mi_tf_basbkey, mi_tf_basrkey,
	mi_explod1, mi_explod2, mi_h_player,
	mi_gib1, mi_gib2, mi_gib3, mi_rocket, mi_grenade, mi_bubble,
	mi_vaxe, mi_vbio, mi_vgrap, mi_vknife, mi_vknife2, mi_vmedi, mi_vspan,
	mi_bigspike, mi_espike,
	cl_num_modelindices,
} cl_modelindex_t;		

extern int cl_modelindices[cl_num_modelindices];
extern char *cl_modelnames[cl_num_modelindices];

void CL_InitEnts(void);
// <-- fuh

void CL_SetSolidPlayers (int playernum);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities (void);
void CL_ClearProjectiles (void);
void CL_ParseProjectiles (qboolean nails2);
void CL_ParsePacketEntities (qboolean delta);
void CL_SetSolidEntities (void);
void CL_ParsePlayerinfo (void);
void CL_InitInterpolation(float cur, float old); // Highlander
void CL_ClearPredict(void); // Highlander
void CL_Interpolate(void); // Highlander

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_PredictUsercmd (player_state_t *from, player_state_t *to, usercmd_t *u, qboolean spectator);

//
// cl_cam.c
//
#define CAM_NONE	0
#define CAM_TRACK	1

extern	int		autocam;
extern	int		spec_track; // player# of who we are tracking

extern cvar_t	cl_spec_id;

void vectoangles(vec3_t vec, vec3_t ang);
qboolean Cam_DrawViewModel(void);
qboolean Cam_DrawPlayer(int playernum);
void Cam_Track(usercmd_t *cmd);
void Cam_FinishMove(usercmd_t *cmd);
void Cam_Reset(void);
void CL_InitCam(void);
int Cam_TrackNum(void);
void Cam_Lock(int playernum);
void Cam_TryLock (void);
void Cam_SetViewPlayer (void);

//
// skin.c
//

extern	cvar_t		baseskin;
extern	cvar_t		noskins;

typedef struct
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char	reserved;
	char	color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char	filler[58];
	unsigned char	data;			// unbounded
} pcx_t;


void	Skin_Find (player_info_t *sc);
byte	*Skin_Cache (skin_t *skin);
void	Skin_Skins_f (void);
void	Skin_AllSkins_f (void);
void	Skin_NextDownload (void);

void	R_TranslatePlayerSkin (int playernum);
void	R_UpdateSkins (void); // BorisU


#define RSSHOT_WIDTH 320

#ifdef GLQUAKE
#define RSSHOT_HEIGHT 240
#else
#define RSSHOT_HEIGHT 200
#endif

#define		SAY_TEAM_CHAT_BUFFER_SIZE	512
char *ParseSay (char *string);

extern	int fb_skins[MAX_CLIENTS];
