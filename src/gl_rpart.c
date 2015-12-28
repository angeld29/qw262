/*
Copyright (C) 2002-2003, Dr Labman, A. Nourai

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

#include "quakedef.h"
#include <assert.h>

cvar_t gl_part_explosions	= {"gl_part_explosions", "0"};
cvar_t gl_part_trails		= {"gl_part_trails", "0"};
cvar_t gl_part_spikes		= {"gl_part_spikes", "0"};
cvar_t gl_part_gunshots		= {"gl_part_gunshots", "0"};
cvar_t gl_part_blood		= {"gl_part_blood", "0"};
cvar_t gl_part_telesplash	= {"gl_part_telesplash", "0"};
cvar_t gl_part_blobs		= {"gl_part_blobs", "0"};
cvar_t gl_part_lavasplash	= {"gl_part_lavasplash", "0"};
cvar_t gl_part_inferno		= {"gl_part_inferno", "0"};
cvar_t gl_part_sparks		= {"gl_part_sparks", "1"};
cvar_t gl_part_traillen		= {"gl_part_trail_lenght", "1"};
cvar_t gl_part_traildetail	= {"gl_part_trail_detail", "1"};
cvar_t gl_part_trailwidth	= {"gl_part_trail_width",  "3"};
cvar_t gl_weather_rain		= {"gl_weather_rain", "0"};
cvar_t gl_weather_rain_fast = {"gl_weather_rain_fast", "0"};

#define DEFAULT_NUM_PARTICLES				4096
#define ABSOLUTE_MIN_PARTICLES				512
#define ABSOLUTE_MAX_PARTICLES				32768

typedef byte col_t[4];

typedef enum {
	p_spark, p_smoke, p_fire, p_bubble, p_lavasplash, p_gunblast, p_chunk, p_shockwave,
	p_inferno_flame, p_inferno_trail, p_sparkray, p_staticbubble, p_trailpart,
	p_dpsmoke, p_dpfire, p_teleflare, p_blood1, p_blood2, p_blood3,
	p_telespark, p_gas, p_old_trail,
	//VULT PARTICLES
	p_rain, p_2dshockwave, p_streak, p_streaktrail,
	num_particletypes,
} part_type_t;

typedef enum {
	pm_static, pm_normal, pm_bounce, pm_die, pm_nophysics, pm_float,
	//VULT PARTICLES
	pm_rain, pm_streak,
} part_move_t;

typedef enum {
	ptex_none, ptex_smoke, ptex_bubble, ptex_generic, ptex_dpsmoke, ptex_lava,
	ptex_blueflare, ptex_blood1, ptex_blood2, ptex_blood3, ptex_trail,
	//VULT PARTICLES
	ptex_shockwave,
	num_particletextures,
} part_tex_t;

typedef enum {
	pd_spark, pd_sparkray, pd_billboard, pd_billboard_vel, pd_old_trail,
	//VULT PARTICLES
	pd_beam, pd_hide, pd_normal,
} part_draw_t;

typedef struct QMB_particle_s {
	struct QMB_particle_s *next;
	vec3_t		org, endorg;
	col_t		color;
	float		growth;
	vec3_t		vel;
	float		rotangle;
	float		rotspeed;
	float		size;
	float		start;
	float		die;
	byte		hit;
	byte		texindex;
	byte		bounces;
} QMB_particle_t;

typedef struct QMB_particle_tree_s {
	QMB_particle_t	*start;
	part_type_t	id;
	part_draw_t	drawtype;
	int			SrcBlend;
	int			DstBlend;
	part_tex_t	texture;
	float		startalpha;
	float		grav;
	float		accel;
	part_move_t	move;
	float		custom;
} QMB_particle_type_t;


int		trail_tex;

#define	MAX_PTEX_COMPONENTS		8
typedef struct particle_texture_s {
	int			texnum;
	int			components;
	float		coords[MAX_PTEX_COMPONENTS][4];
} particle_texture_t;


static float sint[7] = {0.000000, 0.781832, 0.974928, 0.433884, -0.433884, -0.974928, -0.781832};
static float cost[7] = {1.000000, 0.623490, -0.222521, -0.900969, -0.900969, -0.222521, 0.623490};

static QMB_particle_t *particles, *free_particles;
static QMB_particle_type_t particle_types[num_particletypes];
static int particle_type_index[num_particletypes];
static particle_texture_t particle_textures[num_particletextures];

static int r_numparticles;		
static vec3_t zerodir = {22, 22, 22};
static int particle_count = 0;
static float particle_time;
static vec3_t trail_stop;

qboolean qmb_initialized = false;
qboolean gl_water_splash;

cvar_t gl_clipparticles = {"gl_clipparticles", "1"};
cvar_t gl_bounceparticles = {"gl_bounceparticles", "1"};

#define lhrandom(MIN,MAX) ((rand() & 32767) * (((MAX)-(MIN)) * (1.0f / 32767.0f)) + (MIN))

static void WeatherEffect (void);
static void RainSplash(vec3_t org);
static void SparkGen (vec3_t org, byte col[2], float count, float size, float life);
static void VX_ParticleTrail (vec3_t start, vec3_t end, float size, float time, col_t color);
static void R_CalcBeamVerts (float *vert, vec3_t org1, vec3_t org2, float width);

void GL_Particles_f (void)
{
	char*	arg = Cmd_Argv(1);

	if (!Q_stricmp(arg, "classic")) {
		Cvar_Set(&gl_part_explosions, "0");
		Cvar_Set(&gl_part_trails, "0");
		Cvar_Set(&gl_part_spikes, "0");
		Cvar_Set(&gl_part_gunshots, "0");
		Cvar_Set(&gl_part_blood, "0");
		Cvar_Set(&gl_part_telesplash, "0");
		Cvar_Set(&gl_part_blobs, "0");
		Cvar_Set(&gl_part_lavasplash, "0");
		Cvar_Set(&gl_part_inferno, "0");
	} else if (!Q_stricmp(arg, "fuh")) {
		Cvar_Set(&gl_part_explosions, "1");
		Cvar_Set(&gl_part_trails, "1");
		Cvar_Set(&gl_part_spikes, "1");
		Cvar_Set(&gl_part_gunshots, "1");
		Cvar_Set(&gl_part_blood, "1");
		Cvar_Set(&gl_part_telesplash, "1");
		Cvar_Set(&gl_part_blobs, "1");
		Cvar_Set(&gl_part_lavasplash, "1");
		Cvar_Set(&gl_part_inferno, "1");
	} else if (!Q_stricmp(arg, "old_qmb")){
		Cvar_Set(&gl_part_explosions, "2");
		Cvar_Set(&gl_part_trails, "2");
		Cvar_Set(&gl_part_spikes, "2");
		Cvar_Set(&gl_part_gunshots, "2");
		Cvar_Set(&gl_part_blood, "2");
		Cvar_Set(&gl_part_telesplash, "2");
		Cvar_Set(&gl_part_blobs, "2");
		Cvar_Set(&gl_part_lavasplash, "2");
		Cvar_Set(&gl_part_inferno, "0");
	} else {
		Cvar_Set(&gl_part_explosions, "1");
		Cvar_Set(&gl_part_trails, "1");
		Cvar_Set(&gl_part_spikes, "1");
		Cvar_Set(&gl_part_gunshots, "1");
		Cvar_Set(&gl_part_blood, "1");
		Cvar_Set(&gl_part_telesplash, "1");
		Cvar_Set(&gl_part_blobs, "1");
		Cvar_Set(&gl_part_lavasplash, "1");
		Cvar_Set(&gl_part_inferno, "2");
	}
}

#define TruePointContents(p) PM_HullPointContents(&cl.worldmodel->hulls[0], 0, p)

#define ISUNDERWATER(x) ((x) == CONTENTS_WATER || (x) == CONTENTS_SLIME || (x) == CONTENTS_LAVA)

static qboolean TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal) {
	trace_t trace;

	memset (&trace, 0, sizeof(trace));
	trace.fraction = 1;
	if (PM_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, start, end, &trace))
		return false;
	VectorCopy (trace.endpos, impact);
	if (normal)
		VectorCopy (trace.plane.normal, normal);

	return true;
}

static byte *ColorForParticle(part_type_t type) {
	int lambda;
	static col_t color;
	byte *colourByte;

	switch (type) {
	case p_spark:
		color[0] = 224 + (rand() & 31); color[1] = 100 + (rand() & 31); color[2] = 0;
		break;
	case p_smoke:
		color[0] = color[1] = color[2] = 255;
		break;
	case p_fire:
		color[0] = 255; color[1] = 142; color[2] = 62;
		break;
	case p_bubble:
	case p_staticbubble:
		color[0] = color[1] = color[2] = 192 + (rand() & 63);
		break;
	case p_teleflare:
	case p_lavasplash:
		color[0] = color[1] = color[2] = 128 + (rand() & 127);
		break;
	case p_gunblast:
		color[0]= 224 + (rand() & 31); color[1] = 170 + (rand() & 31); color[2] = 0;
		break;
	case p_chunk:
		color[0] = color[1] = color[2] = (32 + (rand() & 127));
		break;
	case p_shockwave:
		color[0] = color[1] = color[2] = 64 + (rand() & 31);
		break;
	case p_inferno_flame:
	case p_inferno_trail:
		color[0] = 255; color[1] = 77; color[2] = 13;
		break;
	case p_sparkray:
		color[0] = 255; color[1] = 102; color[2] = 25;
		break;
	case p_dpsmoke:
		color[0] = color[1] = color[2] = 48 + (((rand() & 0xFF) * 48) >> 8);
		break;
	case p_dpfire:
		lambda = rand() & 0xFF;
		color[0] = 160 + ((lambda * 48) >> 8); color[1] = 16 + ((lambda * 148) >> 8); color[2] = 16 + ((lambda * 16) >> 8);
		break;
	case p_blood1:
	case p_blood2:
		color[0] = color[1] = color[2] = 180 + (rand() & 63);
		break;
	case p_blood3:
		color[0] = (100 + (rand() & 31)); color[1] = color[2] = 0;
		break;
	case p_telespark:
		color[0] = color[1] = color[2] = 225;
		break;
	case p_gas:
		colourByte = (byte *)&d_8to24table[224 + (rand()&7)];
		color[0] = colourByte[0];
		color[1] = colourByte[1];
		color[2] = colourByte[2];
		break;
	case p_old_trail:
		color[0] = color[1] = color[2] = 255;
		break;
	default:
		assert(!"ColorForParticle: unexpected type");
		break;
	}
	return color;
}


#define ADD_PARTICLE_TEXTURE(_ptex, _texnum, _texindex, _components, _s1, _t1, _s2, _t2)	\
do {																						\
	particle_textures[_ptex].texnum = _texnum;												\
	particle_textures[_ptex].components = _components;										\
	particle_textures[_ptex].coords[_texindex][0] = (_s1 + 1) / 256.0;						\
	particle_textures[_ptex].coords[_texindex][1] = (_t1 + 1) / 256.0;						\
	particle_textures[_ptex].coords[_texindex][2] = (_s2 - 1) / 256.0;						\
	particle_textures[_ptex].coords[_texindex][3] = (_t2 - 1) / 256.0;						\
} while(0);

#define ADD_PARTICLE_TYPE(_id, _drawtype, _SrcBlend, _DstBlend, _texture, _startalpha, _grav, _accel, _move, _custom)	\
do {																													\
	particle_types[count].id = (_id);																					\
	particle_types[count].drawtype = (_drawtype);																		\
	particle_types[count].SrcBlend = (_SrcBlend);																		\
	particle_types[count].DstBlend = (_DstBlend);																		\
	particle_types[count].texture = (_texture);																			\
	particle_types[count].startalpha = (_startalpha);																	\
	particle_types[count].grav = 9.8 * (_grav);																			\
	particle_types[count].accel = (_accel);																				\
	particle_types[count].move = (_move);																				\
	particle_types[count].custom = (_custom);																			\
	particle_type_index[_id] = count;																					\
	count++;																											\
} while(0);

void QMB_InitParticles (void) {
	int	i, count = 0, particlefont;
	int shockwave_texture;

	Cvar_RegisterVariable (&gl_clipparticles);
	Cvar_RegisterVariable (&gl_bounceparticles);

	Cvar_RegisterVariable (&gl_part_explosions);
	Cvar_RegisterVariable (&gl_part_trails);
	Cvar_RegisterVariable (&gl_part_spikes);
	Cvar_RegisterVariable (&gl_part_gunshots);
	Cvar_RegisterVariable (&gl_part_blood);
	Cvar_RegisterVariable (&gl_part_telesplash);
	Cvar_RegisterVariable (&gl_part_blobs);
	Cvar_RegisterVariable (&gl_part_lavasplash);
	Cvar_RegisterVariable (&gl_part_inferno);
	Cvar_RegisterVariable (&gl_part_sparks);
	Cvar_RegisterVariable (&gl_part_traillen);
	Cvar_RegisterVariable (&gl_part_trailwidth);
	Cvar_RegisterVariable (&gl_part_traildetail);
	Cvar_RegisterVariable (&gl_weather_rain);
	Cvar_RegisterVariable (&gl_weather_rain_fast);

	Cmd_AddCommand ("gl_particles", GL_Particles_f);

	if (!(particlefont = loadtexture_24bit ("particlefont", LOADTEX_QMB_PARTICLE))) 
		return;

	{ // making texture for trails
		int		i, j, k, centreX, centreY, separation, max;
		byte	data[128][128][4];

		max=(int)(sqrt((64-0)*(64-0) + (64-0)*(64-0)));
		for(j=0;j<128;j++){
			for(i=0;i<128;i++){
				data[i][j][0] = 255;
				data[i][j][1] = 255;
				data[i][j][2]= 255;
				separation = (int) sqrt((64-i)*(64-i) + (64-j)*(64-j));
				data[i][j][3] = (max - separation)*2;
			}
		}
		
		//Add 128 random 4 unit circles
		for(k=0;k<128;k++){
			centreX = rand()%122+3;
			centreY = rand()%122+3;
			for(j=-3;j<3;j++){
				for(i=-3;i<3;i++){
					separation = (int) sqrt((i*i) + (j*j));
					if (separation <= 5)
						data[i+centreX][j+centreY][3] += (5-separation);
				}
			}
		trail_tex = GL_LoadTexture ("qmb:trail_part", 128, 128, &data[0][0][0], false, true, false, 4);
	}


	}
	ADD_PARTICLE_TEXTURE(ptex_none, 0, 0, 1, 0, 0, 0, 0);
	ADD_PARTICLE_TEXTURE(ptex_blood1, particlefont, 0, 1, 0, 0, 64, 64);
	ADD_PARTICLE_TEXTURE(ptex_blood2, particlefont, 0, 1, 64, 0, 128, 64);
	ADD_PARTICLE_TEXTURE(ptex_lava, particlefont, 0, 1, 128, 0, 192, 64);
	ADD_PARTICLE_TEXTURE(ptex_blueflare, particlefont, 0, 1, 192, 0, 256, 64);
	ADD_PARTICLE_TEXTURE(ptex_generic, particlefont, 0, 1, 0, 96, 96, 192);
	ADD_PARTICLE_TEXTURE(ptex_smoke, particlefont, 0, 1, 96, 96, 192, 192);
	ADD_PARTICLE_TEXTURE(ptex_blood3, particlefont, 0, 1, 192, 96, 256, 160);
	ADD_PARTICLE_TEXTURE(ptex_bubble, particlefont, 0, 1, 192, 160, 224, 192);
	for (i = 0; i < 8; i++)
		ADD_PARTICLE_TEXTURE(ptex_dpsmoke, particlefont, i, 8, i * 32, 64, (i + 1) * 32, 96);

	ADD_PARTICLE_TEXTURE(ptex_trail, trail_tex, 0, 1, 0, 0, 128, 128); // BorisU

	if ((i = COM_CheckParm ("-particles")) && i + 1 < com_argc) {
		r_numparticles = (int) (Q_atoi(com_argv[i + 1]));
		r_numparticles = bound(ABSOLUTE_MIN_PARTICLES, r_numparticles, ABSOLUTE_MAX_PARTICLES);
	} else {
		r_numparticles = DEFAULT_NUM_PARTICLES;
	}
	particles = Hunk_AllocName(r_numparticles * sizeof(QMB_particle_t), "qmb:particles");

	//VULT PARTICLES
	if (!(shockwave_texture = loadtexture_24bit ("shockwavetex", LOADTEX_QMB_PARTICLE)))
	{
		Con_Printf ("Missing shockwavetex texture, rain water splash disabled.\n");
		gl_water_splash = false;
	}
	else
	{
		ADD_PARTICLE_TEXTURE(ptex_shockwave, shockwave_texture, 0, 1, 0, 0, 128, 128);
		ADD_PARTICLE_TYPE(p_2dshockwave, pd_normal, GL_SRC_ALPHA, GL_ONE, ptex_shockwave, 255, 0, 0, pm_static, 0);
		gl_water_splash = true;
	}
	
	ADD_PARTICLE_TYPE(p_spark, pd_spark, GL_SRC_ALPHA, GL_ONE, ptex_none, 255, -32, 0, pm_bounce, 1.3);
	ADD_PARTICLE_TYPE(p_sparkray, pd_sparkray, GL_SRC_ALPHA, GL_ONE, ptex_none, 255, -0, 0, pm_nophysics, 0);
	ADD_PARTICLE_TYPE(p_gunblast, pd_spark, GL_SRC_ALPHA, GL_ONE, ptex_none, 255, -16, 0, pm_bounce, 1.3);

	ADD_PARTICLE_TYPE(p_fire, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 204, 0, -2.95, pm_die, 0);
	ADD_PARTICLE_TYPE(p_chunk, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 255, -16, 0, pm_bounce, 1.475);
	ADD_PARTICLE_TYPE(p_shockwave, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 255, 0, -4.85, pm_nophysics, 0);
	ADD_PARTICLE_TYPE(p_inferno_flame, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 153, 0, 0, pm_static, 0);
	ADD_PARTICLE_TYPE(p_inferno_trail, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 204, 0, 0, pm_die, 0);
	ADD_PARTICLE_TYPE(p_trailpart, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_generic, 230, 0, 0, pm_static, 0);
	ADD_PARTICLE_TYPE(p_smoke, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_smoke, 140, 3, 0, pm_normal, 0);
	ADD_PARTICLE_TYPE(p_dpfire, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_dpsmoke, 144, 0, 0, pm_die, 0);
	ADD_PARTICLE_TYPE(p_dpsmoke, pd_billboard, GL_SRC_ALPHA, GL_ONE, ptex_dpsmoke, 85, 3, 0, pm_die, 0);

	ADD_PARTICLE_TYPE(p_teleflare, pd_billboard, GL_ONE, GL_ONE, ptex_blueflare, 255, 0, 0, pm_die, 0);

	ADD_PARTICLE_TYPE(p_blood1, pd_billboard, GL_ZERO, GL_ONE_MINUS_SRC_COLOR, ptex_blood1, 255, -20, 0, pm_die, 0);
	ADD_PARTICLE_TYPE(p_blood2, pd_billboard_vel, GL_ZERO, GL_ONE_MINUS_SRC_COLOR, ptex_blood2, 255, -25, 0, pm_die, 0.018);

	ADD_PARTICLE_TYPE(p_lavasplash, pd_billboard, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, ptex_lava, 170, 0, 0, pm_nophysics, 0);
	ADD_PARTICLE_TYPE(p_blood3, pd_billboard, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, ptex_blood3, 255, -20, 0, pm_normal, 0);
	ADD_PARTICLE_TYPE(p_bubble, pd_billboard, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, ptex_bubble, 204, 8, 0, pm_float, 0);
	ADD_PARTICLE_TYPE(p_staticbubble, pd_billboard, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, ptex_bubble, 204, 0, 0, pm_static, 0);

// BorisU -->
	ADD_PARTICLE_TYPE(p_telespark, pd_spark, GL_SRC_ALPHA, GL_ONE, ptex_none, 255, -32, 0, pm_die, 1.3);
	ADD_PARTICLE_TYPE(p_gas, pd_billboard, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, ptex_generic, 255, 0, 0, pm_nophysics, 0);
	ADD_PARTICLE_TYPE(p_old_trail, pd_old_trail, GL_SRC_ALPHA, GL_ONE, ptex_trail, 255, 0, 0, pm_static, 0);
// <-- BorisU

	//VULT particles
	ADD_PARTICLE_TYPE(p_rain, pd_hide, GL_SRC_ALPHA, GL_ONE, ptex_none, 100, 0, 0, pm_rain, 0);
	ADD_PARTICLE_TYPE(p_streak, pd_hide, GL_SRC_ALPHA, GL_ONE, ptex_none, 255, -64, 0, pm_streak, 1.5); //grav was -64
	ADD_PARTICLE_TYPE(p_streaktrail, pd_beam, GL_SRC_ALPHA, GL_ONE, ptex_none, 128, 0, 0, pm_die, 0);
	qmb_initialized = true;
}

void QMB_ClearParticles (void) {
	int	i;

	if (!qmb_initialized)
		return;

	particle_count = 0;
	memset(particles, 0, r_numparticles * sizeof(QMB_particle_t));
	free_particles = &particles[0];

	for (i = 0; i + 1 < r_numparticles; i++)
		particles[i].next = &particles[i + 1];
	particles[r_numparticles - 1].next = NULL;

	for (i = 0; i < num_particletypes; i++)
		particle_types[i].start = NULL;
}

static void QMB_UpdateParticles(void) {
	int i, contents;
	float grav, bounce;
	vec3_t oldorg, stop, normal;
	QMB_particle_type_t *pt;
	QMB_particle_t *p, *kill;

	particle_count = 0;
	grav = movevars.gravity / 800.0;

	//VULT PARTICLES
	WeatherEffect();

	for (i = 0; i < num_particletypes; i++) {
		pt = &particle_types[i];
		
		if (pt->start) {
			for (p = pt->start; p && p->next; ) {
				kill = p->next;
				if (kill->die <= particle_time) {
					p->next = kill->next;
					kill->next = free_particles;
					free_particles = kill;
				} else {
					p = p->next;
				}
			}
			if (pt->start->die <= particle_time) {
				kill = pt->start;
				pt->start = kill->next;
				kill->next = free_particles;
				free_particles = kill;
			}
		}

		for (p = pt->start; p; p = p->next) {
			if (particle_time < p->start)
				continue;
			
			particle_count++;
			
			p->size += p->growth * cls.frametime;
			
			if (p->size <= 0) {
				p->die = 0;
				continue;
			}
			
			//VULT PARTICLE
			if (pt->id == p_streaktrail)
				p->color[3] = p->bounces * ((p->die - particle_time) / (p->die - p->start));
			else
				p->color[3] = pt->startalpha * ((p->die - particle_time) / (p->die - p->start));
			
			p->rotangle += p->rotspeed * cls.frametime;
			
			if (p->hit)
				continue;
			
			//VULT - switched these around so velocity is scaled before gravity is applied
			VectorScale(p->vel, 1 + pt->accel * cls.frametime, p->vel);
			p->vel[2] += pt->grav * grav * cls.frametime;
			
			switch (pt->move) {
			case pm_static:
				break;
			case pm_normal:
				VectorCopy(p->org, oldorg);
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents (p->org)) {
					p->hit = 1;
					VectorCopy(oldorg, p->org);
					VectorClear(p->vel);
				}
				break;
			case pm_float:
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				p->org[2] += p->size + 1;		
				contents = TruePointContents(p->org);
				if (!ISUNDERWATER(contents))
					p->die = 0;
				p->org[2] -= p->size + 1;
				break;
			case pm_nophysics:
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				break;
			case pm_die:
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents (p->org))
					p->die = 0;
				break;
			case pm_bounce:
				
				if (!gl_bounceparticles.value || p->bounces) {
					VectorMA(p->org, cls.frametime, p->vel, p->org);
					if (CONTENTS_SOLID == TruePointContents (p->org))
						p->die = 0;
				} else {
					VectorCopy(p->org, oldorg);
					VectorMA(p->org, cls.frametime, p->vel, p->org);
					if (CONTENTS_SOLID == TruePointContents (p->org)) {
						if (TraceLineN(oldorg, p->org, stop, normal)) {
							VectorCopy(stop, p->org);
							bounce = -pt->custom * DotProduct(p->vel, normal);
							VectorMA(p->vel, bounce, normal, p->vel);
							p->bounces++;
						}
					}
				}
				break;
			case pm_rain:
				VectorCopy(p->org, oldorg);
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				contents = TruePointContents(p->org);
				if (ISUNDERWATER(contents) || contents == CONTENTS_SOLID)
				{
					if (!gl_weather_rain_fast.value || gl_weather_rain_fast.value == 2)
					{
						vec3_t rorg;
						VectorCopy(oldorg, rorg);
						//Find out where the rain should actually hit
						//This is a slow way of doing it, I'll fix it later maybe...
						while (1)
						{
							rorg[2] = rorg[2] - 0.5f;
							contents = TruePointContents(rorg);
							if (contents == CONTENTS_WATER)
							{
								if (!gl_water_splash || gl_weather_rain_fast.value == 2)
									break;
								rorg[2] += 1;
								RainSplash(rorg);
								break;
							}
							else if (contents == CONTENTS_SOLID)
							{
								byte col[3] = {128,128,128};
								SparkGen (rorg, col, 3, 50, 0.15);
								break;
							}
						}
						VectorCopy(rorg, p->org);
						VX_ParticleTrail (oldorg, p->org, p->size, 0.2, p->color);
					}
					p->die = 0;
				}
				else
					VX_ParticleTrail (oldorg, p->org, p->size, 0.2, p->color);
				break;
			case pm_streak:
				VectorCopy(p->org, oldorg);
				VectorMA(p->org, cls.frametime, p->vel, p->org);
				if (CONTENTS_SOLID == TruePointContents (p->org)) 
				{
					if (TraceLineN(oldorg, p->org, stop, normal)) 
					{
						VectorCopy(stop, p->org);
						bounce = -pt->custom * DotProduct(p->vel, normal);
						VectorMA(p->vel, bounce, normal, p->vel);
						//VULT - Prevent crazy sliding
/*						p->vel[0] = 2 * p->vel[0] / 3;
						p->vel[1] = 2 * p->vel[1] / 3;
						p->vel[2] = 2 * p->vel[2] / 3;*/
					}
				}
				VX_ParticleTrail (oldorg, p->org, p->size, 0.2, p->color);
				if (VectorLength(p->vel) == 0)
					p->die = 0;
				break;
			default:
				assert(!"QMB_UpdateParticles: unexpected pt->move");
				break;
			}
		}
	}
}


#define DRAW_PARTICLE_BILLBOARD(_ptex, _p, _coord)			\
	glPushMatrix();											\
	glTranslatef(_p->org[0], _p->org[1], _p->org[2]);		\
	glScalef(_p->size, _p->size, _p->size);					\
	if (_p->rotspeed)										\
		glRotatef(_p->rotangle, vpn[0], vpn[1], vpn[2]);	\
															\
	glColor4ubv(_p->color);									\
															\
	glBegin(GL_QUADS);										\
	glTexCoord2f(_ptex->coords[_p->texindex][0], ptex->coords[_p->texindex][3]); glVertex3fv(_coord[0]);	\
	glTexCoord2f(_ptex->coords[_p->texindex][0], ptex->coords[_p->texindex][1]); glVertex3fv(_coord[1]);	\
	glTexCoord2f(_ptex->coords[_p->texindex][2], ptex->coords[_p->texindex][1]); glVertex3fv(_coord[2]);	\
	glTexCoord2f(_ptex->coords[_p->texindex][2], ptex->coords[_p->texindex][3]); glVertex3fv(_coord[3]);	\
	glEnd();			\
						\
	glPopMatrix();

void QMB_DrawParticles (void) {
	int	i, j, k, drawncount;
	vec3_t v, up, right, billboard[4], velcoord[4], neworg;
	QMB_particle_t *p;
	QMB_particle_type_t *pt;
	particle_texture_t *ptex;
	int texture = 0;
	float varray_vertex[16];

	particle_time = cl.time;

	if (!cl.paused)
		QMB_UpdateParticles();

	VectorAdd(vup, vright, billboard[2]);
	VectorSubtract(vright, vup, billboard[3]);
	VectorNegate(billboard[2], billboard[0]);
	VectorNegate(billboard[3], billboard[1]);

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glShadeModel(GL_SMOOTH);

	for (i = 0; i < num_particletypes; i++) {
		pt = &particle_types[i];
		if (!pt->start)
			continue;
		if (pt->drawtype == pd_hide)
			continue;

		glBlendFunc(pt->SrcBlend, pt->DstBlend);

		switch(pt->drawtype) {
		case pd_spark:
			glDisable(GL_TEXTURE_2D);
			for (p = pt->start; p; p = p->next) {
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				if (!TraceLineN(p->endorg, p->org, neworg, NULL)) 
					VectorCopy(p->org, neworg);

				glBegin(GL_TRIANGLE_FAN);
				glColor4ubv(p->color);
				glVertex3fv(p->org);
				glColor4ub(p->color[0] >> 1, p->color[1] >> 1, p->color[2] >> 1, 0);
				for (j = 7; j >= 0; j--) {
					for (k = 0; k < 3; k++)
						v[k] = p->org[k] - p->vel[k] / 8 + vright[k] * cost[j % 7] * p->size + vup[k] * sint[j % 7] * p->size;
					glVertex3fv(v);
				}
				glEnd();
			}
			break;
		case pd_sparkray:
			glDisable(GL_TEXTURE_2D);
			for (p = pt->start; p; p = p->next) {
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				if (!TraceLineN(p->endorg, p->org, neworg, NULL)) 
					VectorCopy(p->org, neworg);

				glBegin (GL_TRIANGLE_FAN);
				glColor4ubv(p->color);
				glVertex3fv(p->endorg);
				glColor4ub(p->color[0] >> 1, p->color[1] >> 1, p->color[2] >> 1, 0);
				for (j = 7; j >= 0; j--) {
					for (k = 0; k < 3; k++)
						v[k] = neworg[k] + vright[k] * cost[j % 7] * p->size + vup[k] * sint[j % 7] * p->size;
					glVertex3fv (v);
				}
				glEnd();
			}
			break;
		case pd_billboard:
			ptex = &particle_textures[pt->texture];
			glEnable(GL_TEXTURE_2D);
			if (texture != ptex->texnum)
			{
				GL_Bind(ptex->texnum);
				texture = ptex->texnum;
			}
			drawncount = 0;
			for (p = pt->start; p; p = p->next) {
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				if (gl_clipparticles.value) {
					if (drawncount >= 3 && VectorSupCompare(p->org, r_origin, 30))
						continue;
					drawncount++;
				}
				DRAW_PARTICLE_BILLBOARD(ptex, p, billboard);
			}
			break;
		case pd_billboard_vel:
			ptex = &particle_textures[pt->texture];
			glEnable(GL_TEXTURE_2D);
			if (texture != ptex->texnum)
			{
				GL_Bind(ptex->texnum);
				texture = ptex->texnum;
			}
			for (p = pt->start; p; p = p->next) {
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				VectorCopy (p->vel, up);
				CrossProduct(vpn, up, right);
				VectorNormalize(right);
				VectorScale(up, pt->custom, up);

				VectorAdd(up, right, velcoord[2]);
				VectorSubtract(right, up, velcoord[3]);
				VectorNegate(velcoord[2], velcoord[0]);
				VectorNegate(velcoord[3], velcoord[1]);
				DRAW_PARTICLE_BILLBOARD(ptex, p, velcoord);
			}
			break;
		case pd_old_trail:
			ptex = &particle_textures[pt->texture];
			glDisable(GL_CULL_FACE);
			glEnable(GL_TEXTURE_2D);
			GL_Bind(ptex->texnum);
			glBegin (GL_QUADS);
			for (p = pt->start; p; p = p->next) {
				if (particle_time < p->start || particle_time >= p->die)
					continue;
					glColor4ubv(p->color);
					glTexCoord2f (0, 1);
					VectorMA (p->org, p->size, vup, v);
					VectorMA (p->org, p->size, vright, v);
					glVertex3fv (v);

					glTexCoord2f (0, 0);
					VectorMA (p->org, -p->size, vright, v);
					glVertex3fv (v);

					glTexCoord2f (1, 0);
					VectorMA (p->endorg, -p->size, vright, v);
					glVertex3fv (v);

					glTexCoord2f (1, 1);
					VectorMA (p->endorg, p->size, vup, v);
					VectorMA (p->endorg, p->size, vright, v);
					glVertex3fv (v);

			}
			glEnd ();
			if (gl_cull.value)
				glEnable(GL_CULL_FACE);
			break;
		//VULT PARTICLES
		case pd_beam:
			ptex = &particle_textures[pt->texture];
			glEnable(GL_TEXTURE_2D);
			//VULT PARTICLES
			if (texture != ptex->texnum)
			{
				GL_Bind(ptex->texnum);
				texture = ptex->texnum;
			}

			for (p = pt->start; p; p = p->next) 
			{
				int l;
				if (particle_time < p->start || particle_time >= p->die)
					continue;
				glColor4ubv(p->color);
				for (l=gl_part_traildetail.value; l>0 ;l--)
				{
					R_CalcBeamVerts(varray_vertex, p->org, p->endorg, p->size/(l*gl_part_trailwidth.value));
					glBegin (GL_QUADS);
					glTexCoord2f (1,0);
					glVertex3f(varray_vertex[ 0], varray_vertex[ 1], varray_vertex[ 2]);
					glTexCoord2f (1,1);
					glVertex3f(varray_vertex[ 4], varray_vertex[ 5], varray_vertex[ 6]);
					glTexCoord2f (0,1);
					glVertex3f(varray_vertex[ 8], varray_vertex[ 9], varray_vertex[10]);
					glTexCoord2f (0,0);
					glVertex3f(varray_vertex[12], varray_vertex[13], varray_vertex[14]);
					glEnd();
				}
			}
			break;
		case pd_normal:
			ptex = &particle_textures[pt->texture];
			glEnable(GL_TEXTURE_2D);
			//VULT PARTICLES
			if (texture != ptex->texnum)
			{
				GL_Bind(ptex->texnum);
				texture = ptex->texnum;
			}
			for (p = pt->start; p; p = p->next) 
			{
				if (particle_time < p->start || particle_time >= p->die)
					continue;

				glPushMatrix();
				glTranslatef(p->org[0], p->org[1], p->org[2]);
				glScalef(p->size, p->size, p->size);
				glRotatef(p->endorg[0], 0, 1, 0);
				glRotatef(p->endorg[1], 0, 0, 1);
				glRotatef(p->endorg[2], 1, 0, 0);
				glColor4ubv(p->color);
				glBegin (GL_QUADS);
				glTexCoord2f (0,0);
				glVertex3f (-p->size, -p->size, 0);
				glTexCoord2f (1,0);
				glVertex3f (p->size, -p->size, 0);
				glTexCoord2f (1,1);
				glVertex3f (p->size, p->size, 0);
				glTexCoord2f (0,1);
				glVertex3f (-p->size, p->size, 0);
				glEnd();

				//And since quads seem to be one sided...
				glRotatef(180, 1, 0, 0);
				glColor4ubv(p->color);
				glBegin (GL_QUADS);
				glTexCoord2f (0,0);
				glVertex3f (-p->size, -p->size, 0);
				glTexCoord2f (1,0);
				glVertex3f (p->size, -p->size, 0);
				glTexCoord2f (1,1);
				glVertex3f (p->size, p->size, 0);
				glTexCoord2f (0,1);
				glVertex3f (-p->size, p->size, 0);
				glEnd();
				glPopMatrix();
			}
			break;
		default:
			assert(!"QMB_DrawParticles: unexpected drawtype");
			break;
		}
	}

	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

#define	INIT_NEW_PARTICLE(_pt, _p, _color, _size, _time)	\
		_p = free_particles;								\
		free_particles = _p->next;							\
		_p->next = _pt->start;								\
		_pt->start = _p;									\
		_p->size = _size;									\
		_p->hit = 0;										\
		_p->start = cl.time;								\
		_p->die = _p->start + _time;						\
		_p->growth = 0;										\
		_p->rotspeed = 0;									\
		_p->texindex = (rand() % particle_textures[_pt->texture].components);	\
		_p->bounces = 0;									\
		VectorCopy(_color, _p->color);


static void AddParticle(part_type_t type, vec3_t org, int count, float size, float time, col_t col, vec3_t dir) {
	byte *color;
	int i, j;
	float tempSize;
	QMB_particle_t *p;
	QMB_particle_type_t *pt;

	if (!qmb_initialized)
		Sys_Error("QMB particle added without initialization");

	assert(size > 0 && time > 0);

	if (type < 0 || type >= num_particletypes)
		Sys_Error("AddParticle: Invalid type (%d)", type);

	pt = &particle_types[particle_type_index[type]];

	for (i = 0; i < count && free_particles; i++) {
		if (type == p_2dshockwave && !gl_water_splash) continue;

		color = col ? col : ColorForParticle(type);
		INIT_NEW_PARTICLE(pt, p, color, size, time);

		switch (type) {
		case p_spark:
			p->size = 1.175;
			VectorCopy(org, p->org);
			tempSize = size * 2;
			p->vel[0] = (rand() % (int) tempSize) - ((int) tempSize / 2);
			p->vel[1] = (rand() % (int) tempSize) - ((int) tempSize / 2);
			p->vel[2] = (rand() % (int) tempSize) - ((int) tempSize / 3);
			break;
		case p_smoke:
			for (j = 0; j < 3; j++)
				p->org[j] = org[j] + ((rand() & 31) - 16) / 2.0;
			for (j = 0; j < 3; j++)
				p->vel[j] = ((rand() % 10) - 5) / 20.0;
			break;
		case p_fire:
			VectorCopy(org, p->org);
			for (j = 0; j < 3; j++)
				p->vel[j] = ((rand() % 160) - 80) * (size / 25.0);
			break;
		case p_bubble:
			p->start += (rand() & 15) / 36.0;
			p->org[0] = org[0] + ((rand() & 31) - 16);
			p->org[1] = org[1] + ((rand() & 31) - 16);
			p->org[2] = org[2] + ((rand() & 63) - 32);
			VectorClear(p->vel);
			break;
		case p_lavasplash:
			VectorCopy(org, p->org);
			VectorCopy(dir, p->vel);
			break;
		case p_gunblast:
			p->size = 1;
			VectorCopy(org, p->org);
			p->vel[0] = (rand() & 127) - 64;
			p->vel[1] = (rand() & 127) - 64;
			p->vel[2] = (rand() & 127) - 10;
			break;
		case p_chunk:
			VectorCopy(org, p->org);
			p->vel[0] = (rand() % 40) - 20;
			p->vel[1] = (rand() % 40) - 20;
			p->vel[2] = (rand() % 40) - 5;
			break;
		case p_shockwave:
			VectorCopy(org, p->org);
			VectorCopy(dir, p->vel);
			break;
		case p_inferno_trail:
			for (j = 0; j < 3; j++)
				p->org[j] = org[j] + (rand() & 15) - 8;
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() & 3) - 2;
			p->growth = -1.5;
			break;
		case p_inferno_flame:
			VectorCopy(org, p->org);
			VectorClear(p->vel);
			p->growth = -30;
			break;
		case p_sparkray:
			VectorCopy(org, p->endorg);
			VectorCopy(dir, p->org);	
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() & 127) - 64;
			p->growth = -16;
			break;
		case p_staticbubble:
			VectorCopy(org, p->org);
			VectorClear(p->vel);
			break;
		case p_teleflare:
			VectorCopy(org, p->org);
			VectorCopy(dir, p->vel);	
			p->growth = 1.75;
			break;
		case p_blood1:
		case p_blood2:
			for (j = 0; j < 3; j++)
				p->org[j] = org[j] + (rand() & 15) - 8;
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() & 63) - 32;
			break;
		case p_blood3:
			p->size = size * (rand() % 20) / 5.0;
			VectorCopy(org, p->org);
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() % 40) - 20;
			break;
		case p_telespark:
			p->size = 1.175;
			VectorCopy(org, p->org);
			tempSize = size * 2;
			p->vel[0] = (rand() % (int) tempSize) - ((int) tempSize / 2);
			p->vel[1] = (rand() % (int) tempSize) - ((int) tempSize / 2);
			p->vel[2] = (rand() % (int) tempSize) - ((int) tempSize / 3);
			break;
		case p_gas:
			VectorCopy(org, p->org);
			p->vel[0] = (rand()%40)-20;
			p->vel[1] = (rand()%40)-20;
			p->vel[2] = (rand()%80)-5;
			break;
		//VULT PARTICLES
		case p_rain:
			p->size = 1;
			VectorCopy(org, p->org);
			p->vel[0] = (rand() % 10 + 85)*size;
			p->vel[1] = (rand() % 10 + 85)*size;
			p->vel[2] = (rand() % -100 - 2000)*(size/3);
			break;
		case p_streaktrail:
			VectorCopy(org, p->org);
			VectorCopy(dir, p->endorg);
//			if (detailtrails)
//				p->size = size * 3;
			VectorClear(p->vel);
			p->growth = -p->size/time;
			p->bounces = color[3];
			break;
		case p_streak:
			VectorCopy(org, p->org);
			VectorCopy(dir, p->vel);
			break;
		case p_2dshockwave:
			VectorCopy (org, p->org);
			VectorCopy (dir, p->endorg);
			VectorClear(p->vel);
			p->size = 1;
			p->growth = size;
			break;
		default:
			Sys_Error("AddParticle: Invalid type (%d)", type);
			break;
		}
	}
}

static void AddParticleTrail(part_type_t type, vec3_t start, vec3_t end, float size, float time, col_t col) {
	byte *color;
	int i, j, num_particles;
	float count, length;
	vec3_t point, delta;
	QMB_particle_t *p;
	QMB_particle_type_t *pt;

	if (!qmb_initialized)
		Sys_Error("QMB particle added without initialization");

	assert(size > 0 && time > 0);

	if (type < 0 || type >= num_particletypes)
		Sys_Error("AddParticle: Invalid type (%d)", type);

	pt = &particle_types[particle_type_index[type]];

	VectorCopy(start, point);
	VectorSubtract(end, start, delta);
	if (!(length = VectorLength(delta)))
		goto done;

	switch(type) {
	case p_trailpart:
		count = length / 1.1;
		break;
	case p_blood3:
		count = length / 8;
		break;
	case p_smoke:
		count = length / 3.8;
		break;
	case p_dpsmoke:
		count = length / 2.5;
		break;
	case p_dpfire:
		count = length / 2.8;
		break;
	default:
		assert(!"AddParticleTrail: unexpected type");
		break;
	}

	if (!(num_particles = (int) count))
		goto done;

	VectorScale(delta, 1.0 / num_particles, delta);

	for (i = 0; i < num_particles && free_particles; i++) {
		color = col ? col : ColorForParticle(type);
		INIT_NEW_PARTICLE(pt, p, color, size, time);

		switch (type) {
		case p_trailpart:
			VectorCopy (point, p->org);
			VectorClear(p->vel);
			p->growth = -size / time;
			break;
		case p_blood3:
			VectorCopy (point, p->org);
			for (j = 0; j < 3; j++)
				p->org[j] += ((rand() & 15) - 8) / 8.0;
			for (j = 0; j < 3; j++)
				p->vel[j] = ((rand() & 15) - 8) / 2.0;
			p->size = size * (rand() % 20) / 10.0;
			p->growth = 6;
			break;
		case p_smoke:
			VectorCopy (point, p->org);
			for (j = 0; j < 3; j++)
				p->org[j] += ((rand() & 7) - 4) / 8.0;
			p->vel[0] = p->vel[1] = 0;
			p->vel[2] = rand() & 3;
			p->growth = 4.5;
			p->rotspeed = (rand() & 63) + 96;
			break;
		case p_dpsmoke:
			VectorCopy (point, p->org);
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() % 10) - 5;
			p->growth = 3;
			p->rotspeed = (rand() & 63) + 96;
			break;
		case p_dpfire:
			VectorCopy (point, p->org);
			for (j = 0; j < 3; j++)
				p->vel[j] = (rand() % 40) - 20;
			break;
		default:
			assert(!"AddParticleTrail: unexpected type");
			break;
		}

		VectorAdd(point, delta, point);
	}
done:
	VectorCopy (point, trail_stop);
}

static void AddOldParticleTrail(part_type_t type, vec3_t start, vec3_t end, float size, float time, col_t col) {
	QMB_particle_t *p;
	QMB_particle_type_t *pt;
	byte *color;

	if (!free_particles)
		return;

	pt = &particle_types[particle_type_index[type]];
	color = col ? col : ColorForParticle(type);
	INIT_NEW_PARTICLE(pt, p, color, size, time);

	VectorCopy(start, p->org);
	VectorCopy(end, p->endorg);
}

#define ONE_FRAME_ONLY	(0.0001)

void QMB_ParticleExplosion (vec3_t org) {
	int contents;

	if (gl_part_explosions.value == 1) {
		contents = TruePointContents(org);
		if (ISUNDERWATER(contents)) {
			AddParticle(p_fire, org, 12, 14, 0.8, NULL, zerodir);
			AddParticle(p_bubble, org, 6, 3.0, 2.5, NULL, zerodir);
			AddParticle(p_bubble, org, 4, 2.35, 2.5, NULL, zerodir);
			if (r_explosiontype.value != 1) {
				AddParticle(p_spark, org, 50, 100, 0.75, NULL, zerodir);
				AddParticle(p_spark, org, 25, 60, 0.75, NULL, zerodir);
			}
		} else {
			AddParticle(p_fire, org, 16, 18, 1, NULL, zerodir);
			if (r_explosiontype.value != 1) {
				AddParticle(p_spark, org, 50, 250, 0.925f, NULL, zerodir);
				AddParticle(p_spark,org, 25, 150, 0.925f,  NULL, zerodir);
			}
		}
	} else {
		AddParticle(p_fire, org, 20, 16, 1.0f, NULL, zerodir);
		AddParticle(p_spark, org, 64, 300, 1.0f, NULL, zerodir);
		AddParticle(p_spark,org, 32, 200, 1.0f,  NULL, zerodir);
	}
}

void QMB_RunParticleEffect (vec3_t org, vec3_t dir, int col, int count) {
	col_t color;
	vec3_t neworg;
	int i, scale, blastcount, blastsize, sparksize, sparkcount, chunkcount, particlecount, bloodcount;
	float blasttime, sparktime;

	if (col == 73) {
		bloodcount = Q_rint(count / 28.0);
		bloodcount = bound(2, bloodcount, 8);
		for (i = 0; i < bloodcount; i++) {
			AddParticle(p_blood1, org, 1, 4.5, 2 + (rand() & 15) / 16.0, NULL, zerodir);
			AddParticle(p_blood2, org, 1, 4.5, 2 + (rand() & 15) / 16.0, NULL, zerodir);
		}
		return;
	} else if (col == 225) {	
		VectorClear(color);
		scale = (count > 130) ? 3 : (count > 20) ? 2  : 1;
		scale *= 0.758;
		for (i = 0; i < (count >> 1); i++) {
			neworg[0] = org[0] + scale * ((rand() & 15) - 8);
			neworg[1] = org[1] + scale * ((rand() & 15) - 8);
			neworg[2] = org[2] + scale * ((rand() & 15) - 8);
			color[0] = 50 + (rand() & 63);
			AddParticle(p_blood3, org, 1, 1, 2.3, NULL, zerodir);
		}
		return;
	} else if (col == 20 && count == 30) {
		color[0] = 51; color[2] = 51; color[1] = 255;
		AddParticle(p_chunk, org, 1, 1, 0.75, color, zerodir);  
		AddParticle(p_spark, org, 12 , 75, 0.4, color, zerodir);
		return;
	} else if (col == 226 && count == 20) {	
		color[0] = 230; color[1] = 204; color[2] = 26;
		AddParticle(p_chunk, org, 1, 1, 0.75, color, zerodir);  
		AddParticle(p_spark, org, 12 , 75, 0.4, color, zerodir);
		return;
	}

	switch (count) {
	case 10:	
		AddParticle(p_spark, org, 4, 70, 0.9, NULL, zerodir);
		break;
	case 20:	
		AddParticle(p_spark, org, 8, 85, 0.9, NULL, zerodir);
		break;
	case 30:	
		AddParticle(p_chunk, org, 10, 1, 4, NULL, zerodir);
		AddParticle(p_spark, org, 8, 105, 0.9, NULL, zerodir);
		break;
	default:	
		if (count > 130) {
			scale = 2.274;
			blastcount = 50; blastsize = 50; blasttime = 0.4;
			sparkcount = 6; sparksize = 70; sparktime = 0.6;
			chunkcount = 14;
		} else if (count > 20) {
			scale = 1.516;
			blastcount = 30; blastsize = 35; blasttime = 0.25;
			sparkcount = 4; sparksize = 60; sparktime = 0.4;
			chunkcount = 7;
		} else {
			scale = 0.758;
			blastcount = 15; blastsize = 5; blasttime = 0.15;
			sparkcount = 2; sparksize = 50; sparktime = 0.25;
			chunkcount = 3;
		}
		particlecount = max(1, count >> 1);
		AddParticle(p_gunblast, org, blastcount, blastsize, blasttime, NULL, zerodir);
		for (i = 0; i < particlecount; i++) {
			neworg[0] = org[0] + scale * ((rand() & 15) - 8);
			neworg[1] = org[1] + scale * ((rand() & 15) - 8);
			neworg[2] = org[2] + scale * ((rand() & 15) - 8);
			AddParticle(p_smoke, neworg, 1, 4, 0.825f + ((rand() % 10) - 5) / 40.0, NULL, zerodir);
			if (i % (particlecount / chunkcount) == 0)
				AddParticle(p_chunk, neworg, 1, 0.75, 3.75, NULL, zerodir);
		}
	}
}

void QMB_RunParticleEffect_Old (vec3_t org, vec3_t dir, int col, int count)
{
	byte		*colourByte;
	col_t		colour;

	if (col == 0) {
		int s, c;

		if (count == 10) {
			s = 2; c = 5;
		} else if (count == 20){
			s = 4; c = 8;
		} else if (count > 120) {
			s = 10; c = 20;
		} else {
			s = 5; c = 10;
		}

		AddParticle (p_chunk, org, c, 1, 4, NULL, zerodir);
		AddParticle (p_spark, org, s, 100, 1.0f, NULL, zerodir);
	} else {
		colourByte = (byte *)&d_8to24table[col];
		colour[0] = colourByte[0];
		colour[1] = colourByte[1];
		colour[2] = colourByte[2];

		//JHL:HACK; make blood brighter...
		if (col == 73)
			colour[0] = 255;
		AddParticle(p_blood3, org, count, 1, 2, colour, zerodir);
	}
}

void QMB_ParticleTrail (vec3_t start, vec3_t end, /*vec3_t *trail_origin,*/ trail_type_t type) {
	col_t color;

	switch (type) {
		case GRENADE_TRAIL:
			AddParticleTrail(p_smoke, start, end, 1.45, 0.825, NULL);
			if (gl_part_trails.value == 2)
				AddOldParticleTrail(p_old_trail, start, end, 1, 0.825f, NULL);
			break;
		case BLOOD_TRAIL:
		case BIG_BLOOD_TRAIL:
			AddParticleTrail(p_blood3, start, end, type == BLOOD_TRAIL ? 1.35 : 2.4, 2, NULL);
			break;
		case TRACER1_TRAIL:
			color[0] = 0; color[1] = 124; color[2] = 0;
			AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
			break;
		case TRACER2_TRAIL:
			color[0] = 255; color[1] = 77; color[2] = 0;
			AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
			break;
		case VOOR_TRAIL:
			color[0] = 77; color[1] = 0; color[2] = 255;
			AddParticleTrail (p_trailpart, start, end, 3.75, 0.5, color);
			break;
		case ALT_ROCKET_TRAIL:
			AddParticleTrail(p_dpfire, start, end, 3, 0.26, NULL);
			AddParticleTrail(p_dpsmoke, start, end, 3, 0.825, NULL);
			break;
		case ROCKET_TRAIL:
		default:
			color[0] = 255; color[1] = 56; color[2] = 9;
			AddParticleTrail(p_trailpart, start, end, 6.2, 0.31, color);
			AddParticleTrail(p_smoke, start, end, 1.8, 0.825, NULL);
			if (gl_part_trails.value == 2)
				AddOldParticleTrail(p_old_trail, start, end, 1, 0.825f, NULL);
			break;
	}

//	VectorCopy(trail_stop, *trail_origin);
}

void QMB_BlobExplosion (vec3_t org) {
	float theta;
	col_t color;
	vec3_t neworg, vel;

	if (gl_part_blobs.value == 2) {
		int i;
		color[0] = 100; color[1] = 100; color[2] = 255;
		AddParticle (p_spark, org, 44, 200, 1.5f, color, zerodir);
		for (i=0; i<10; i++) {
			color[0] = (rand()%45 + 75);
			color[1] = (rand()%45 + 45);
			AddParticle(p_fire, org, 1, 25, 1.0f, color, zerodir);
		}
		return;
	}

	color[0] = 60; color[1] = 100; color[2] = 240;
	AddParticle (p_spark, org, 44, 250, 1.15, color, zerodir);

	color[0] = 90; color[1] = 47; color[2] = 207;
	AddParticle(p_fire, org, 15, 30, 1.4, color, zerodir);

	vel[2] = 0;
	for (theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / 70) {
		color[0] = (60 + (rand() & 15)); color[1] = (65 + (rand() & 15)); color[2] = (200 + (rand() & 15));

		
		vel[0] = cos(theta) * 125;
		vel[1] = sin(theta) * 125;
		neworg[0] = org[0] + cos(theta) * 6;
		neworg[1] = org[1] + sin(theta) * 6;
		neworg[2] = org[2] + 0 - 10;
		AddParticle(p_shockwave, neworg, 1, 4, 0.8, color, vel);
		neworg[2] = org[2] + 0 + 10;
		AddParticle(p_shockwave, neworg, 1, 4, 0.8, color, vel);

		
		vel[0] *= 1.15;
		vel[1] *= 1.15;
		neworg[0] = org[0] + cos(theta) * 13;
		neworg[1] = org[1] + sin(theta) * 13;
		neworg[2] = org[2] + 0;
		AddParticle(p_shockwave, neworg, 1, 6, 1.0, color, vel);
	}
}

void QMB_LavaSplash (vec3_t org) {
	int	i, j;
	float vel;
	vec3_t dir, neworg;

	if (gl_part_lavasplash.value == 2) {
		AddParticle(p_spark, org, 64, 250, 1.0f, NULL, zerodir);
		for (i=-16 ; i<16 ; i++)
			for (j=-16 ; j<16 ; j++) {
				neworg[0] = org[0] + j*8 + (rand()&7) - 3;
				neworg[1] = org[1] + i*8 + (rand()&7) - 3;
				neworg[2] = org[2] + (rand()&15) - 8;
				AddParticle(p_gas, neworg, 1, 3, 3.0f, NULL, zerodir);
			}
		return;
	}

	for (i = -16; i < 16; i++) {
		for (j = -16; j < 16; j++) {
			dir[0] = j * 8 + (rand() & 7);
			dir[1] = i * 8 + (rand() & 7);
			dir[2] = 256;

			neworg[0] = org[0] + dir[0];
			neworg[1] = org[1] + dir[1];
			neworg[2] = org[2] + (rand() & 63);

			VectorNormalize (dir);
			vel = 50 + (rand() & 63);
			VectorScale (dir, vel, dir);

			AddParticle(p_lavasplash, neworg, 1, 4.5, 2.6 + (rand() & 31) * 0.02, NULL, dir);
		}
	}
}

void QMB_TeleportSplash (vec3_t org) {
	int i, j, k;
	vec3_t neworg, angle;
	col_t color;

	if (gl_part_telesplash.value == 2){
		AddParticle(p_telespark, org, 256, 200, 0.5f, NULL, zerodir);
		return;
	}

	for (i = -12; i <= 12; i += 6) {
		for (j = -12; j <= 12; j += 6) {
			for (k = -24; k <= 32; k += 8) {
				neworg[0] = org[0] + i + (rand() & 3) - 1;
				neworg[1] = org[1] + j + (rand() & 3) - 1;
				neworg[2] = org[2] + k + (rand() & 3) - 1;
				angle[0] = (rand() & 15) - 7;
				angle[1] = (rand() & 15) - 7;
				angle[2] = (rand() % 160) - 80;
				AddParticle(p_teleflare, neworg, 1, 1.8, 0.30 + (rand() & 7) * 0.02, NULL, angle);
			}
		}
	}

	VectorSet(color, 140, 140, 255);
	VectorClear(angle);
	for (i = 0; i < 5; i++) {
		angle[2] = 0;
		for (j = 0; j < 5; j++) {
			AngleVectors(angle, NULL, NULL, neworg);
			VectorMA(org, 70, neworg, neworg);	
			AddParticle(p_sparkray, org, 1, 6 + (i & 3), 5,  color, neworg);
			angle[2] += 360 / 5;
		}
		angle[0] += 180 / 5;
	}
}

void QMB_DetpackExplosion (vec3_t org) {
	int i, j, contents;
	float theta;
	vec3_t neworg, angle = {0, 0, 0};

	
	contents = TruePointContents(org);
	if (ISUNDERWATER(contents)) {
		AddParticle(p_bubble, org, 8, 2.8, 2.5, NULL, zerodir);
		AddParticle(p_bubble, org, 6, 2.2, 2.5, NULL, zerodir);
		AddParticle(p_fire, org, 10, 25, 0.75, ColorForParticle(p_inferno_flame), zerodir);
	} else {
		AddParticle(p_fire, org, 14, 33, 0.75, ColorForParticle(p_inferno_flame), zerodir);
	}

	
	for (i = 0; i < 5; i++) {
		angle[2] = 0;
		for (j = 0; j < 5; j++) {
			AngleVectors(angle, NULL, NULL, neworg);
			VectorMA(org, 90, neworg, neworg);	
			AddParticle(p_sparkray, org, 1, 9 + (i & 3), 0.75,  NULL, neworg);
			angle[2] += 360 / 5;
		}
		angle[0] += 180 / 5;
	}

	
	angle[2] = 0;
	for (theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / 90) {
		angle[0] = cos(theta) * 350;
		angle[1] = sin(theta) * 350;
		AddParticle(p_shockwave, org, 1, 14, 0.75, NULL, angle);
	}
}

void QMB_InfernoFlame (vec3_t org) {
	if (cls.frametime) {
		if (gl_part_inferno.value == 1)
			AddParticle(p_inferno_flame, org, 1, 30, 13.125 * cls.frametime,  NULL, zerodir);
		else
			AddParticle(p_inferno_flame, org, 1, 20, 13.125 * cls.frametime,  NULL, zerodir);
		AddParticle(p_inferno_trail, org, 2, 1.75, 45.0 * cls.frametime, NULL, zerodir);
		AddParticle(p_inferno_trail, org, 2, 1.0, 52.5 * cls.frametime, NULL, zerodir);
	}
}

void QMB_StaticBubble (entity_t *ent) {
	AddParticle(p_staticbubble, ent->origin, 1, ent->frame == 1 ? 1.85 : 2.9, ONE_FRAME_ONLY, NULL, zerodir);
}

//VULT PARTICLES
static void WeatherEffect (void)
{
	vec3_t org, start, impact, normal;
	int i;
	float trace;
	col_t colour = {128,128,128,75};

	if (!gl_weather_rain.value)
		return;

	for (i = 0; i <= (int)gl_weather_rain.value; i++)
	{
		VectorCopy(r_refdef.vieworg, org);
		org[0] = org[0] + (rand() % 3000) - 1500;
		org[1] = org[1] + (rand() % 3000) - 1500;
		VectorCopy(org, start);
		org[2] = org[2] + 15000;

		//Trace a line straight up, get the impact location

		//trace up slowly until we are in sky
		trace = TraceLineN(start, org, impact, normal);

		//fixme: see is surface above has SURF_DRAWSKY (we'll come back to that, when the important stuff is done fist, eh?)
		//if (Mod_PointInLeaf(impact, cl.worldmodel)->contents == CONTENTS_SKY)
		if (TruePointContents(impact) == CONTENTS_SKY && trace)
		{
			VectorCopy(impact, org);
			org[2] = org[2] - 1;
			AddParticle(p_rain, org, 1, 1, 15, colour, zerodir);
		}
	}
}

//VULT PARTICLES
static void RainSplash(vec3_t org)
{
	vec3_t pos;
	col_t colour={255,255,255,255};
	
	VectorCopy(org, pos);

	//VULT - Sometimes the shockwave effect isn't noticable enough?
	AddParticle(p_2dshockwave, pos, 1, 5, 0.5, colour, vec3_origin);
}

//VULT PARTICLES
//Cheap function which will generate some sparks for me to be called elsewhere
static void SparkGen (vec3_t org, byte col[2], float count, float size, float life)
{
	col_t color;
	vec3_t dir;
	int i,a;

	color[0] = col[0];
	color[1] = col[1];
	color[2] = col[2];

	if (gl_part_sparks.value)
	{
		for (a=0;a<count;a++)
		{
			for (i=0;i<3;i++)
				dir[i] = (rand() % (int)size*2) - ((int)size*2/3);
			AddParticle(p_streak, org, 1, 1, life, color, dir);
		}
	}
	else
		AddParticle(p_spark, org, count, size, life, color, zerodir);
}

//VULT PARTICLES
//VULT - These trails were my initial motivation behind AMFQUAKE.
static void VX_ParticleTrail (vec3_t start, vec3_t end, float size, float time, col_t color)
{

	time *= gl_part_traillen.value;
#if 0
	if (detailtrails)
	{
		float	len;
		col_t	color2={255,255,255};

		VectorSubtract (end, start, vec);
		len = VectorNormalize(vec);

		while (len > 0)
		{
			//ADD PARTICLE HERE
			AddParticle(p_streaktrail, start, 1, size, time, color, zerodir);
			len--;
			VectorAdd (start, vec, start);
		}
	}
#endif
	AddParticle(p_streaktrail, start, 1, size, time, color, end);

}

//from darkplaces engine - finds which corner of a particle goes where, so I don't have to :D
static void R_CalcBeamVerts (float *vert, vec3_t org1, vec3_t org2, float width)
{
	vec3_t right1, right2, diff, normal;

	VectorSubtract (org2, org1, normal);
	VectorNormalize (normal);

	//width = width / 2;
	// calculate 'right' vector for start
	VectorSubtract (r_origin, org1, diff);
	VectorNormalize (diff);
	CrossProduct (normal, diff, right1);

	// calculate 'right' vector for end
	VectorSubtract (r_origin, org2, diff);
	VectorNormalize (diff);
	CrossProduct (normal, diff, right2);

	vert[ 0] = org1[0] + width * right1[0];
	vert[ 1] = org1[1] + width * right1[1];
	vert[ 2] = org1[2] + width * right1[2];
	vert[ 4] = org1[0] - width * right1[0];
	vert[ 5] = org1[1] - width * right1[1];
	vert[ 6] = org1[2] - width * right1[2];
	vert[ 8] = org2[0] - width * right2[0];
	vert[ 9] = org2[1] - width * right2[1];
	vert[10] = org2[2] - width * right2[2];
	vert[12] = org2[0] + width * right2[0];
	vert[13] = org2[1] + width * right2[1];
	vert[14] = org2[2] + width * right2[2];
}

void R_ReadPointFile_f (void) { // Do we really need it?
//	FILE *f;
//	vec3_t org;
//	int r, c;
//	particle_t *p;
//	char name[MAX_OSPATH];
//
//	if (!com_serveractive)
//		return;
//
//	Q_snprintfz (name, sizeof(name), "maps/%s.pts", mapname.string);
//
//	if (FS_FOpenFile (name, &f) == -1) {
//		Com_Printf ("couldn't open %s\n", name);
//		return;
//	}
//
//	Com_Printf ("Reading %s...\n", name);
//	c = 0;
//	while (1) {
//		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
//		if (r != 3)
//			break;
//
//		c++;
//		if (!free_particles) {
//			Com_Printf ("Not enough free particles\n");
//			break;
//		}
//		p = free_particles;
//		free_particles = p->next;
//		p->next = active_particles;
//		active_particles = p;
//		
//		p->die = 99999;
//		p->color = (-c)&15;
//		p->type = pt_static;
//		VectorClear (p->vel);
//		VectorCopy (org, p->org);
//	}
//
//	fclose (f);
//	Com_Printf ("%i points read\n", c);
}

// ********************* Classic particle engine

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct classic_particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct classic_particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} classic_particle_t;

classic_particle_t	*classic_active_particles, *classic_free_particles;

classic_particle_t	*classic_particles;
int					classic_r_numparticles;

int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

vec3_t				r_pright, r_pup, r_ppn;


/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	int		i;

	if ((i = COM_CheckParm ("-particles")) && i + 1 < com_argc)	{
		classic_r_numparticles = (int) (Q_atoi(com_argv[i + 1]));
		classic_r_numparticles = 
			bound(ABSOLUTE_MIN_PARTICLES, classic_r_numparticles, ABSOLUTE_MAX_PARTICLES);
	} else {
		classic_r_numparticles = DEFAULT_NUM_PARTICLES;
	}

	classic_particles = Hunk_AllocName (classic_r_numparticles * sizeof(classic_particle_t), "particles");

	QMB_InitParticles ();
	if (!qmb_initialized) {
		Cvar_SetRO(gl_part_explosions);
		Cvar_SetRO(gl_part_trails);
		Cvar_SetRO(gl_part_spikes);
		Cvar_SetRO(gl_part_gunshots);
		Cvar_SetRO(gl_part_blood);
		Cvar_SetRO(gl_part_telesplash);
		Cvar_SetRO(gl_part_blobs);
		Cvar_SetRO(gl_part_lavasplash);
		Cvar_SetRO(gl_part_inferno);
		Cvar_SetRO(gl_weather_rain);

		Con_Printf ("QMB particles disabled\n");
	}
}

void R_ClearParticles (void) 
{
	int		i;
	
	classic_free_particles = &classic_particles[0];
	classic_active_particles = NULL;

	for (i = 0;i < classic_r_numparticles; i++)
		classic_particles[i].next = &classic_particles[i+1];
	classic_particles[classic_r_numparticles - 1].next = NULL;

	QMB_ClearParticles ();
}

/*
===============
R_ParticleExplosion
===============
*/
void R_ParticleExplosion (vec3_t org)
{
	int			i, j;
	classic_particle_t	*p;

	if (gl_part_explosions.value){
		QMB_ParticleExplosion(org);
		return;
	}

	for (i=0 ; i<1024 ; i++)
	{
		if (!classic_free_particles)
			return;
		p = classic_free_particles;
		classic_free_particles = p->next;
		p->next = classic_active_particles;
		classic_active_particles = p;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}
/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion (vec3_t org)
{
	int			i, j;
	classic_particle_t	*p;

	if (gl_part_blobs.value){
		QMB_BlobExplosion(org);
		return;
	}
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!classic_free_particles)
			return;
		p = classic_free_particles;
		classic_free_particles = p->next;
		p->next = classic_active_particles;
		classic_active_particles = p;

		p->die = cl.time + 1 + (rand()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void Classic_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	classic_particle_t	*p;
	int			scale;

	if (count > 130)
		scale = 3;
	else if (count > 20)
		scale = 2;
	else
		scale = 1;

	for (i=0 ; i<count ; i++)
	{
		if (!classic_free_particles)
			return;
		p = classic_free_particles;
		classic_free_particles = p->next;
		p->next = classic_active_particles;
		classic_active_particles = p;

		p->die = cl.time + 0.1*(rand()%5);
		p->color = (color&~7) + (rand()&7);
		p->type = pt_grav;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + scale*((rand()&15)-8);
			p->vel[j] = dir[j]*15;// + (rand()%300)-150;
		}
	}
}

#define RunParticleEffect(var, org, dir, color, count)		\
	if (gl_part_##var.value == 1)							\
		QMB_RunParticleEffect(org, dir, color, count);		\
	else if (gl_part_##var.value == 2)						\
		QMB_RunParticleEffect_Old(org, dir, color, count);	\
	else													\
		Classic_RunParticleEffect(org, dir, color, count);

void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	if (color == 73 || color == 225) {
		RunParticleEffect(blood, org, dir, color, count);
		return;
	}

	switch (count) {
	case 10:
	case 20:
	case 30:
		RunParticleEffect(spikes, org, dir, color, count);
		break;
	default:
		RunParticleEffect(gunshots, org, dir, color, count);
	}
}

/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	classic_particle_t	*p;
	float		vel;
	vec3_t		dir;
	
	if (gl_part_lavasplash.value){
		QMB_LavaSplash (org);
		return;
	}

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!classic_free_particles)
					return;
				p = classic_free_particles;
				classic_free_particles = p->next;
				p->next = classic_active_particles;
				classic_active_particles = p;
		
				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_grav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	classic_particle_t	*p;
	float		vel;
	vec3_t		dir;

	if (gl_part_telesplash.value){
		QMB_TeleportSplash (org);
		return;
	}

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!classic_free_particles)
					return;
				p = classic_free_particles;
				classic_free_particles = p->next;
				p->next = classic_active_particles;
				classic_active_particles = p;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_grav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

void R_ParticleTrail (vec3_t start, vec3_t end, trail_type_t type)
{
	vec3_t	vec;
	float	len;
	int			j;
	classic_particle_t	*p;

	if (gl_part_trails.value){
		QMB_ParticleTrail (start, end, type);
		return;
	}

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	while (len > 0)
	{
		switch (type) {
		case ALT_ROCKET_TRAIL:
			len -= 1.5; break;
		case BLOOD_TRAIL:
			len -= 6; break;
		default:
			len -= 3; break;
		}

		if (!classic_free_particles)
			return;
		p = classic_free_particles;
		classic_free_particles = p->next;
		p->next = classic_active_particles;
		classic_active_particles = p;
		
		VectorCopy (vec3_origin, p->vel);
		p->die = cl.time + 2;

		if (type == BLOOD_TRAIL)
		{	// slight blood
			p->type = pt_slowgrav;
			p->color = 67 + (rand()&3);
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
		}
		else if (type == BIG_BLOOD_TRAIL)
		{	// blood
			p->type = pt_slowgrav;
			p->color = 67 + (rand()&3);
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
		}
		else if (type == VOOR_TRAIL)
		{	// voor trail
			p->color = 9*16 + 8 + (rand()&3);
			p->type = pt_static;
			p->die = cl.time + 0.3;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()&15)-8);
		}
		else if (type == GRENADE_TRAIL)
		{	// smoke smoke
			p->ramp = (rand()&3) + 2;
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
		}
		else if (type == ROCKET_TRAIL || type == ALT_ROCKET_TRAIL)
		{	// rocket trail
			p->ramp = (rand()&3);
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
		}
		else if (type == TRACER1_TRAIL || type == TRACER2_TRAIL)
		{	// tracer
			static int tracercount;

			p->die = cl.time + 0.5;
			p->type = pt_static;
			if (type == 3)
				p->color = 52 + ((tracercount&4)<<1);
			else
				p->color = 230 + ((tracercount&4)<<1);
			
			tracercount++;

			VectorCopy (start, p->org);
			if (tracercount & 1)
			{
				p->vel[0] = 30*vec[1];
				p->vel[1] = 30*-vec[0];
			}
			else
			{
				p->vel[0] = 30*-vec[1];
				p->vel[1] = 30*vec[0];
			}
			
		}
		
		VectorAdd (start, vec, start);
	}
}


/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	classic_particle_t		*p, *kill;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;
	unsigned char	*at;
	unsigned char	theAlpha;
	vec3_t			up, right;
	float			scale, r_partscale;
//	qboolean		alphaTestEnabled;
	
	r_partscale = 0.004 * tan (r_refdef.fov_x * (M_PI/180) * 0.5f); // Tonik

	QMB_DrawParticles ();

	GL_Bind(particletexture);
//	alphaTestEnabled = glIsEnabled(GL_ALPHA_TEST);
	
//	if (alphaTestEnabled)
//		glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDepthMask (GL_FALSE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBegin (GL_TRIANGLES);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	if (cl.paused)
		frametime = 0;
	else
		frametime = cls.frametime;
	
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * 800 * 0.05;
	dvel = 4*frametime;
	
	for ( ;; ) 
	{
		kill = classic_active_particles;
		if (kill && kill->die < cl.time)
		{
			classic_active_particles = kill->next;
			kill->next = classic_free_particles;
			classic_free_particles = kill;
			continue;
		}
		break;
	}

	for (p=classic_active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = classic_free_particles;
				classic_free_particles = kill;
				continue;
			}
			break;
		}

		// hack a scale up to keep particles from disapearing
		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * r_partscale;
		at = (byte *)&d_8to24table[(int)p->color];
		if (p->type==pt_fire)
			theAlpha = 255*(6-p->ramp)/6;
//			theAlpha = 192;
//		else if (p->type==pt_explode || p->type==pt_explode2)
//			theAlpha = 255*(8-p->ramp)/8;
		else
			theAlpha = 255;
		glColor4ub (*at, *(at+1), *(at+2), theAlpha);
//		glColor3ubv (at);
//		glColor3ubv ((byte *)&d_8to24table[(int)p->color]);
		glTexCoord2f (0,0);
		glVertex3fv (p->org);
		glTexCoord2f (1,0);
		glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
		glTexCoord2f (0,1);
		glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);

		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_slowgrav:
		case pt_grav:
			p->vel[2] -= grav;
			break;
		}
	}

	glEnd ();
	glDisable (GL_BLEND);
	glDepthMask (GL_TRUE);
//	if (alphaTestEnabled)
//		glEnable(GL_ALPHA_TEST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor3ubv (color_white);
}

