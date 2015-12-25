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
// cl_ents.c -- entity parsing and management

#include "quakedef.h"

extern	cvar_t	cl_predict_players;
extern	cvar_t	cl_predict_players2;
extern	cvar_t	cl_solid_players;
extern	cvar_t	cl_nolerp;

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
// Highlander -->
	qboolean predict;
	vec3_t	oldo;
	vec3_t	olda;
	vec3_t	vel;
	player_state_t *oldstate;
// <-- Highlander
} predicted_players[MAX_CLIENTS];

//============================================================

// fuh -->
dlighttype_t dlightColor(float f, dlighttype_t def, qboolean random) {
	dlighttype_t colors[NUM_DLIGHTTYPES - 4] = {lt_red, lt_blue, lt_redblue, lt_green, lt_white};

	if ((int) f == 1)
		return lt_red;
	else if ((int) f == 2)
		return lt_blue;
	else if ((int) f == 3) 
		return lt_redblue;
	else if ((int) f == 4)
		return lt_green;
	else if ((int) f == 5)
		return lt_white;
	else if (((int) f == NUM_DLIGHTTYPES - 3) && random)
		return colors[rand() % (NUM_DLIGHTTYPES - 4)];
	else
		return def;
}
// <-- fuh

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_NewDlight
===============
*/
// Tonik -->
void CL_NewDlight (int key, vec3_t origin, float radius, float time, int type, int bubble)
{
	dlight_t	*dl;
	
	dl = CL_AllocDlight (key);
	VectorCopy (origin, dl->origin);
	dl->radius = radius;
	dl->die = cl.time + time;
	dl->type = type;
	dl->bubble = bubble;		
}
// <-- Tonik

/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		dl->radius -= cls.frametime*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
=========================================================================

PACKET ENTITY PARSING / LINKING

=========================================================================
*/

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
int	bitcounts[32];	/// just for protocol profiling
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int bits)
{
	int			i;

	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = bits & 511;
	bits &= ~511;

	if (bits & U_MOREBITS)
	{	// read in the low order bits
		i = MSG_ReadByte ();
		bits |= i;
	}

	// count the bits for net profiling
	for (i=0 ; i<16 ; i++)
		if (bits&(1<<i))
			bitcounts[i]++;

	to->flags = bits;
	
	if (bits & U_MODEL)
		to->modelindex = MSG_ReadByte ();
		
	if (bits & U_FRAME)
		to->frame = MSG_ReadByte ();

	if (bits & U_COLORMAP)
		to->colormap = MSG_ReadByte();

	if (bits & U_SKIN)
		to->skinnum = MSG_ReadByte();

	if (bits & U_EFFECTS)
		to->effects = MSG_ReadByte();

	if (bits & U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord ();
		
	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle();

	if (bits & U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord ();
		
	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle();

	if (bits & U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord ();
		
	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle();

	if (bits & U_SOLID)
	{
		// FIXME
	}
}


/*
=================
FlushEntityPacket
=================
*/
void FlushEntityPacket (void)
{
	int			word;
	entity_state_t	olde, newe;

	Con_DPrintf ("FlushEntityPacket\n");

	memset (&olde, 0, sizeof(olde));

	cl.delta_sequence = 0;
	cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		word = (unsigned short)MSG_ReadShort ();
		if (msg_badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
			break;	// done

		CL_ParseDelta (&olde, &newe, word);
	}
}

#define IS_SPIKE(_modelindex) (_modelindex == cl_modelindices[mi_spike] \
 							   || _modelindex == cl_modelindices[mi_bigspike] \
							   || _modelindex == cl_modelindices[mi_espike])

// fuh -->
void CL_SetupPacketEntity (int number, entity_state_t *state, qboolean changed)
{
	centity_t *cent;

	cent = &cl_entities[number];

	if (!cl.oldvalidsequence || cl.oldvalidsequence != cent->sequence ||
		state->modelindex != cent->current.modelindex ||
		!VectorL2Compare(state->origin, cent->current.origin,
						 (IS_SPIKE(state->modelindex) ? 350 : 200))
						// nails are extremely fast, so we increase lerp
						// threshold for them
	) {
		cent->startlerp = cl.time;
		cent->deltalerp = -1;
		cent->frametime = -1;
 	} else {
		if (state->frame != cent->current.frame) {
			cent->frametime = cl.time;
			cent->oldframe = cent->current.frame;
		}
		
		if (changed) {
			if (!VectorCompare(state->origin, cent->current.origin) || !VectorCompare(state->angles, cent->current.angles)) {
				VectorCopy(cent->current.origin, cent->old_origin);
				VectorCopy(cent->current.angles, cent->old_angles);

				cent->deltalerp = cl.time - cent->startlerp;
				cent->startlerp = cl.time;
			}
		}
	}
 
	cent->current = *state;
	cent->sequence = cl.validsequence;
}
// <-- fuh

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
// Tonik implementation with several fixes
void CL_ParsePacketEntities (qboolean delta)
{
	int			oldpacket, newpacket;
	packet_entities_t	*oldp, *newp;
	packet_entities_t	dummy;
	int			oldindex, newindex;
	int			word, newnum, oldnum;
	qboolean	full;
	byte		from;

	newpacket = cls.netchan.incoming_sequence&UPDATE_MASK;
	newp = &cl.frames[newpacket].packet_entities;
	cl.frames[newpacket].invalid = false;

	if (delta)
	{
		from = MSG_ReadByte ();

		oldpacket = cl.frames[newpacket].delta_sequence;
		if (cls.mvdplayback)
			from = oldpacket = (cls.netchan.incoming_sequence-1);

		if (cls.netchan.outgoing_sequence - cls.netchan.incoming_sequence >= UPDATE_BACKUP-1)
		{	// there are no valid frames left, so drop it
			FlushEntityPacket ();
			cl.validsequence = 0;
			return;
		}

		if ( (from&UPDATE_MASK) != (oldpacket&UPDATE_MASK) ) {
			Con_DPrintf ("WARNING: from mismatch\n");
			FlushEntityPacket ();
			cl.validsequence = 0;
			return;
		}

		if (cls.netchan.outgoing_sequence - oldpacket >= UPDATE_BACKUP-1)
		{	// we can't use this, it is too old
			FlushEntityPacket ();
			// don't clear cl.validsequence, so that frames can
			// still be rendered; it is possible that a fresh packet will
			// be received before (outgoing_sequence - incoming_sequence)
			// exceeds UPDATE_BACKUP-1
			return;
		}

		oldp = &cl.frames[oldpacket&UPDATE_MASK].packet_entities;
		full = false;
		cl.int_packet = newpacket;
	}
	else
	{	// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		full = true;
	}

	cl.oldvalidsequence = cl.validsequence;
	cl.validsequence = cls.netchan.incoming_sequence;
	cl.delta_sequence = cl.validsequence;

	oldindex = 0;
	newindex = 0;
	newp->num_entities = 0;

	while (1)
	{
		word = (unsigned short)MSG_ReadShort ();
		if (msg_badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{	// copy all the rest of the entities from the old packet
//Con_Printf ("copy %i\n", oldp->entities[oldindex].number);
				if ((newindex >= MAX_PACKET_ENTITIES && !cls.mvdplayback) ||
					 newindex >= MVD_MAX_PACKET_ENTITIES)
					Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
				newp->entities[newindex] = oldp->entities[oldindex];
				CL_SetupPacketEntity(newp->entities[newindex].number, &newp->entities[newindex], false); // fuh
				newindex++;
				oldindex++;
			}
			break;
		}
		newnum = word&511;
		oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

		while (newnum > oldnum)
		{
			if (full)
			{
				Con_Printf ("WARNING: oldcopy on full update");
				FlushEntityPacket ();
				cl.validsequence = 0;	// can't render a frame
				return;
			}

//Con_Printf ("copy %i\n", oldnum);
			// copy one of the old entities over to the new packet unchanged
			if ((newindex >= MAX_PACKET_ENTITIES && !cls.mvdplayback) ||
				 newindex >= MVD_MAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
			newp->entities[newindex] = oldp->entities[oldindex];
			CL_SetupPacketEntity(oldnum, &newp->entities[newindex], word > 511); // fuh
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;
		}

		if (newnum < oldnum)
		{	// new from baseline
//Con_Printf ("baseline %i\n", newnum);
			if (word & U_REMOVE)
			{
				if (full)
				{
					cl.validsequence = 0;
					Con_Printf ("WARNING: U_REMOVE on full update\n");
					FlushEntityPacket ();
					cl.validsequence = 0;	// can't render a frame
					return;
				}
				continue;
			}
			if ((newindex >= MAX_PACKET_ENTITIES && !cls.mvdplayback) ||
				 newindex >= MVD_MAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
			CL_ParseDelta (&cl_entities[newnum].baseline, &newp->entities[newindex], word);
			CL_SetupPacketEntity (newnum, &newp->entities[newindex], word > 511); // fuh
			newindex++;
			continue;
		}

		if (newnum == oldnum)
		{	// delta from previous
			if (full)
			{
				cl.validsequence = 0;
				cl.delta_sequence = 0;
				Con_Printf ("WARNING: delta on full update");
			}
			if (word & U_REMOVE)
			{
				oldindex++;
				continue;
			}
//Con_Printf ("delta %i\n",newnum);
			CL_ParseDelta (&oldp->entities[oldindex], &newp->entities[newindex], word);
			CL_SetupPacketEntity (newnum, &newp->entities[newindex], word > 511); // fuh

			newindex++;
			oldindex++;
		}

	}

	newp->num_entities = newindex;
}

/*
===============
CL_LinkPacketEntities

===============
*/
void CL_LinkPacketEntities (void)
{
	entity_t			*ent;
	centity_t			*cent;
	packet_entities_t	*pack;
	entity_state_t		*s1;
	model_t				*model;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					pnum;
	int					flicker; // fuh
	float				lerp; // fuh

	pack = &cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;

	autorotate = anglemod(100*cl.time);

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		s1 = &pack->entities[pnum];
		cent = &cl_entities[s1->number];

		// control powerup glow for bots. Tonik
		if (s1->modelindex != cl_modelindices[mi_player] || r_powerupglow.value) {
			flicker = r_lightflicker.value ? (rand() & 31) : 15; // fuh
			// spawn light flashes, even ones coming from invisible objects
			if ((s1->effects & (EF_BLUE | EF_RED)) == (EF_BLUE | EF_RED))
				CL_NewDlight (s1->number, s1->origin, 200 + flicker, 0.1, lt_redblue, 0);
			else if (s1->effects & EF_BLUE)
				CL_NewDlight (s1->number, s1->origin, 200 + flicker, 0.1, lt_blue, 0);
			else if (s1->effects & EF_RED)
				CL_NewDlight (s1->number, s1->origin, 200 + flicker, 0.1, lt_red, 0);
			else if (s1->effects & EF_BRIGHTLIGHT) {
				vec3_t	tmp;
				VectorCopy (s1->origin, tmp);
				tmp[2] += 16;
				CL_NewDlight (s1->number, tmp, 400 + flicker, 0.1, lt_default, 0);
			} else if (s1->effects & EF_DIMLIGHT) {
				dlighttype_t dimlightcolor;
				if (cl.teamfortress && (s1->modelindex == cl_modelindices[mi_tf_flag] 
									|| s1->modelindex == cl_modelindices[mi_tf_stan]
									|| s1->modelindex == cl_modelindices[mi_tf_basbkey]
									|| s1->modelindex == cl_modelindices[mi_tf_basrkey])
									)
					dimlightcolor = dlightColor(r_flagcolor.value, lt_default, false);
				else if (s1->modelindex == cl_modelindices[mi_flag])
					dimlightcolor = dlightColor(r_flagcolor.value, lt_default, false);
				else
					dimlightcolor = lt_default;
				CL_NewDlight (s1->number, s1->origin, 200 + flicker, 0.1, dimlightcolor, 0);
			}
		}
		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		// create a new entity
		if (cl_numvisedicts == MAX_VISEDICTS)
			break;		// object list is full

		ent = &cl_visedicts[cl_numvisedicts];
		cl_numvisedicts++;

		ent->keynum = s1->number;
		ent->model = model = cl.model_precache[s1->modelindex];
	
		// set colormap
		if (s1->colormap && (s1->colormap < MAX_CLIENTS) 
			&& ent->model->modhint == MOD_PLAYER)
		{
			ent->colormap = cl.players[s1->colormap-1].translations;
			ent->scoreboard = &cl.players[s1->colormap-1];
		}
		else
		{
			ent->colormap = vid.colormap;
			ent->scoreboard = NULL;
		}

		// set skin
		ent->skinnum = s1->skinnum;
		
		// set frame
		ent->frame = s1->frame;

		// fuh -->
		if (cent->frametime >= 0 && cent->frametime <= cl.time) {
			ent->oldframe = cent->oldframe;
			ent->framelerp = (cl.time - cent->frametime) * 10;
		} else {
			ent->oldframe = ent->frame;
			ent->framelerp = -1;
		}
	
		if ((cl_nolerp.value && !cls.mvdplayback) || cent->deltalerp <= 0) {
			lerp = -1;
			VectorCopy(cent->current.origin, ent->origin);
		} else {
			lerp = (cl.time - cent->startlerp) / cent->deltalerp;
			if (lerp > 1) lerp = 1;
			VectorInterpolate (cent->old_origin, lerp, cent->current.origin, ent->origin);
		}
		// <-- fuh

		// rotate binary objects locally
		if (model->flags & EF_ROTATE)
		{
			ent->angles[0] = 0;
			ent->angles[1] = autorotate;
			ent->angles[2] = 0;
		}
		else
		{
			if (lerp != -1) {
				AngleInterpolate(cent->old_angles, lerp, cent->current.angles, ent->angles);
			} else {
				VectorCopy(cent->current.angles, ent->angles);
			}
		}

	#ifdef GLQUAKE
		if (qmb_initialized) {
			if (s1->modelindex == cl_modelindices[mi_explod1] || s1->modelindex == cl_modelindices[mi_explod2]) {
				if (gl_part_inferno.value) {
					cl_numvisedicts--; // Do not show the sprite
					QMB_InfernoFlame (ent->origin);
					continue;
				}
			}

			if (s1->modelindex == cl_modelindices[mi_bubble]) {
				cl_numvisedicts--; // Do not show the sprite
				QMB_StaticBubble (ent);
				continue;
			}
		}
	#endif

		// add automatic particle trails
		if (!(model->flags & ~EF_ROTATE))
			continue;

		// scan the old entity display list for a matching
		for (i=0 ; i<cl_oldnumvisedicts ; i++)
		{
			if (cl_oldvisedicts[i].keynum == ent->keynum)
			{
				VectorCopy (cl_oldvisedicts[i].origin, old_origin);
				break;
			}
		}
		if (i == cl_oldnumvisedicts)
			continue;		// not in last message

		for (i=0 ; i<3 ; i++)
			if ( abs(old_origin[i] - ent->origin[i]) > 128)
			{	// no trail if too far
				VectorCopy (ent->origin, old_origin);
				break;
			}

		if (model->flags & EF_ROCKET)
		{
#ifdef GLQUAKE
			R_Flare (old_origin, ent->origin, 0);
#endif
			if (r_rockettrail.value) {
				if (r_rockettrail.value == 2)
					R_ParticleTrail (old_origin, ent->origin, GRENADE_TRAIL);
				else if (r_rockettrail.value == 4)
					R_ParticleTrail (old_origin, ent->origin, BLOOD_TRAIL);
				else if (r_rockettrail.value == 5)
					R_ParticleTrail (old_origin, ent->origin, BIG_BLOOD_TRAIL);
				else if (r_rockettrail.value == 6)
					R_ParticleTrail (old_origin, ent->origin, TRACER1_TRAIL);
				else if (r_rockettrail.value == 7)
					R_ParticleTrail (old_origin, ent->origin, TRACER2_TRAIL);
				else if (r_rockettrail.value == 8)
					R_ParticleTrail (old_origin, ent->origin, VOOR_TRAIL);
				else if (r_rockettrail.value == 3)
					R_ParticleTrail (old_origin, ent->origin, ALT_ROCKET_TRAIL);
				else
					R_ParticleTrail (old_origin, ent->origin, ROCKET_TRAIL);
			}

			if (r_rocketlight.value) {
				dlighttype_t rocketlightcolor;
				float rocketlightsize;
				rocketlightcolor = dlightColor(r_rocketlightcolor.value, lt_rocket, false);
				rocketlightsize = 100 * (1 + bound(0, r_rocketlight.value, 1));
				CL_NewDlight (s1->number, ent->origin, rocketlightsize, 0.1, rocketlightcolor, 1);
			}

		}
		else if (model->flags & EF_GRENADE) {
			if (r_grenadetrail.value)
				R_ParticleTrail (old_origin, ent->origin, GRENADE_TRAIL);
		} else if (model->flags & EF_GIB)
			R_ParticleTrail (old_origin, ent->origin, BLOOD_TRAIL);
		else if (model->flags & EF_ZOMGIB)
			R_ParticleTrail (old_origin, ent->origin, BIG_BLOOD_TRAIL);
		else if (model->flags & EF_TRACER)
			R_ParticleTrail (old_origin, ent->origin, TRACER1_TRAIL);
		else if (model->flags & EF_TRACER2)
			R_ParticleTrail (old_origin, ent->origin, TRACER2_TRAIL);
		else if (model->flags & EF_TRACER3)
			R_ParticleTrail (old_origin, ent->origin, VOOR_TRAIL);
	}
}


/*
=========================================================================

PROJECTILE PARSING / LINKING

=========================================================================
*/

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	vec3_t	angles;
	int		num;
} projectile_t;

#define	MAX_PROJECTILES	32
projectile_t	cl_projectiles[MAX_PROJECTILES], cl_oldprojectiles[MAX_PROJECTILES];
int				cl_num_projectiles, cl_num_oldprojectiles;

void CL_ClearProjectiles (void)
{
	static int parsecount = 0;

	if (parsecount == cl.parsecount)
		return;

	parsecount = cl.parsecount;

	memset(cl.int_projectiles, 0, sizeof(cl.int_projectiles));

	cl_num_oldprojectiles = cl_num_projectiles;
	memcpy(cl_oldprojectiles, cl_projectiles, sizeof(cl_projectiles));
	cl_num_projectiles = 0;
}

/*
=====================
CL_ParseProjectiles

Nails are passed as efficient temporary entities
=====================
*/
// Highlander -->
void CL_ParseProjectiles (qboolean nail2)
{
	int		i, c, j, num;
	byte	bits[6];
	projectile_t	*pr;
	interpolate_t	*inter;

	c = MSG_ReadByte ();
	for (i=0 ; i<c ; i++)
	{
		if (nail2)
			num = MSG_ReadByte();
		else
			num = 0;

		for (j=0 ; j<6 ; j++)
			bits[j] = MSG_ReadByte ();

		if (cl_num_projectiles == MAX_PROJECTILES)
			continue;

		pr = &cl_projectiles[cl_num_projectiles];
		inter = &cl.int_projectiles[cl_num_projectiles];
		cl_num_projectiles++;

		pr->modelindex = cl_modelindices[mi_spike];
		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->angles[0] = 360*(bits[4]>>4)/16;
		pr->angles[1] = 360*bits[5]/256;
		pr->num = num;
		if (!num) 
			continue;

		for (j = 0; j < cl_num_oldprojectiles; j++)
			if (cl_oldprojectiles[j].num == num)
			{
				inter->interpolate = true;
				inter->oldindex = j;
				VectorCopy(pr->origin, inter->origin);
				if (!cl_oldprojectiles[j].origin[0])
					Con_Printf("parse:%d, %d, %d\n", j, num, cl_oldprojectiles[j].num);
				break;
			}
	}
}
// <-- Highlander

/*
=============
CL_LinkProjectiles

=============
*/
void CL_LinkProjectiles (void)
{
	int		i;
	projectile_t	*pr;
	entity_t		*ent;

	for (i=0, pr=cl_projectiles ; i<cl_num_projectiles ; i++, pr++)
	{
		// grab an entity to fill in
		if (cl_numvisedicts == MAX_VISEDICTS)
			break;		// object list is full
		ent = &cl_visedicts[cl_numvisedicts];
		cl_numvisedicts++;
		memset (ent, 0, sizeof(entity_t));

		if (pr->modelindex < 1)
			continue;
		ent->model = cl.model_precache[pr->modelindex];
		ent->colormap = vid.colormap;
		VectorCopy (pr->origin, ent->origin);
		VectorCopy (pr->angles, ent->angles);
	}
}

//========================================

entity_t *CL_NewTempEntity (void);

// Highlander -->
int TranslateFlags(int src)
{
	int dst = 0;

	if (src & DF_EFFECTS)
		dst |= PF_EFFECTS;
	if (src & DF_SKINNUM)
		dst |= PF_SKINNUM;
	if (src & DF_DEAD)
		dst |= PF_DEAD;
	if (src & DF_GIB)
		dst |= PF_GIB;
	if (src & DF_WEAPONFRAME)
		dst |= PF_WEAPONFRAME;
	if (src & DF_MODEL)
		dst |= PF_MODEL;

	return dst;
}
// <-- Highlander

// fuh -->
void CL_SetupPlayerEntity(int num, player_state_t *state, player_state_t *oldstate)
{
	centity_t *cent;

	cent = &cl_entities[num];

	if (!oldstate->messagenum || oldstate->messagenum != cent->sequence ||
		state->modelindex != cent->current.modelindex ||
		!VectorL2Compare(state->origin, cent->current.origin, 200))
	{
		cent->startlerp = cl.time;
		cent->deltalerp = -1;
		cent->frametime = -1;
	} else {

		if (state->frame != cent->current.frame) {
			cent->frametime = cl.time;
			cent->oldframe = cent->current.frame;
		}
		
		if (!VectorCompare(state->origin, cent->current.origin)
			|| !VectorCompare(state->viewangles, cent->current.angles))
		{
			VectorCopy(cent->current.origin, cent->old_origin);
			VectorCopy(cent->current.angles, cent->old_angles);

			cent->deltalerp = cl.time - cent->startlerp;
			cent->startlerp = cl.time;
		}
	}
	
	
	VectorCopy(state->origin, cent->current.origin);
	VectorCopy(state->viewangles, cent->current.angles);
	cent->current.frame = state->frame;
	cent->sequence = state->messagenum;	
	cent->current.modelindex = state->modelindex;	
}
// <-- fuh

/*
===================
CL_ParsePlayerinfo
===================
*/
extern int parsecountmod;
extern double parsecounttime;
void CL_ParsePlayerinfo (void)
{
	int			msec;
	int			flags;
	player_info_t	*info;
	player_state_t	*state, *prevstate, dummy;
	int			num;
	int			i;

	num = MSG_ReadByte ();
	if (num > MAX_CLIENTS)
		Sys_Error ("CL_ParsePlayerinfo: bad num");

	info = &cl.players[num];

	memset(&dummy, 0, sizeof(dummy));

	state = &cl.frames[parsecountmod].playerstate[num];

// Highlander -->
	if (info->prevcount > cl.parsecount || !cl.parsecount) {
		prevstate = &dummy;
	} else {
		if (cl.parsecount - info->prevcount >= UPDATE_BACKUP-1)
			prevstate = &dummy;
		else 
			prevstate = &cl.frames[info->prevcount&UPDATE_MASK].playerstate[num];
	}

	info->prevcount = cl.parsecount;

	if (cls.mvdplayback)
	{
		if (cls.findtrack && info->stats[STAT_HEALTH] != 0)
		{
			extern int autocam;
			extern int ideal_track;

			autocam = CAM_TRACK;
			Cam_Lock(num);
			ideal_track = num;
			cls.findtrack = false;

		}

		memcpy(state, prevstate, sizeof(player_state_t));
		flags = MSG_ReadShort ();
		state->flags = TranslateFlags(flags);

		state->messagenum = cl.parsecount;
		state->command.msec = 0;

		state->frame = MSG_ReadByte ();

		state->state_time = parsecounttime;
		state->command.msec = 0;

		for (i=0; i <3; i++)
			if (flags & (DF_ORIGIN << i))
				state->origin[i] = MSG_ReadCoord ();

		for (i=0; i <3; i++)
			if (flags & (DF_ANGLES << i))
				state->command.angles[i] = MSG_ReadAngle16 ();


		if (flags & DF_MODEL)
			state->modelindex = MSG_ReadByte ();
			
		if (flags & DF_SKINNUM)
			state->skinnum = MSG_ReadByte ();
			
		if (flags & DF_EFFECTS)
			state->effects = MSG_ReadByte ();
		
		if (flags & DF_WEAPONFRAME)
			state->weaponframe = MSG_ReadByte ();
			
		VectorCopy (state->command.angles, state->viewangles);
		CL_SetupPlayerEntity(num + 1, state, prevstate);
		return;
	}
// <-- Highlander
	
	flags = state->flags = MSG_ReadShort ();

	state->messagenum = cl.parsecount;
	state->origin[0] = MSG_ReadCoord ();
	state->origin[1] = MSG_ReadCoord ();
	state->origin[2] = MSG_ReadCoord ();

	state->frame = MSG_ReadByte ();

	// the other player's last move was likely some time
	// before the packet was sent out, so accurately track
	// the exact time it was valid at
	if (flags & PF_MSEC)
	{
		msec = MSG_ReadByte ();
		state->state_time = parsecounttime - msec*0.001;
	}
	else
		state->state_time = parsecounttime;

	if (flags & PF_COMMAND)
		MSG_ReadDeltaUsercmd (&nullcmd, &state->command);

	for (i=0 ; i<3 ; i++)
	{
		if (flags & (PF_VELOCITY1<<i) )
			state->velocity[i] = MSG_ReadShort();
		else
			state->velocity[i] = 0;
	}
	if (flags & PF_MODEL)
		state->modelindex = MSG_ReadByte ();
	else
		state->modelindex = cl_modelindices[mi_player];

	if (flags & PF_SKINNUM)
		state->skinnum = MSG_ReadByte ();
	else
		state->skinnum = 0;

	if (flags & PF_EFFECTS)
		state->effects = MSG_ReadByte ();
	else
		state->effects = 0;

	if (flags & PF_WEAPONFRAME)
		state->weaponframe = MSG_ReadByte ();
	else
		state->weaponframe = 0;

	VectorCopy (state->command.angles, state->viewangles);

	CL_SetupPlayerEntity(num + 1, state, prevstate);
}


/*
================
CL_AddFlagModels

Called when the CTF flags are set
================
*/
void CL_AddFlagModels (entity_t *ent, int team)
{
	int		i;
	float	f;
	vec3_t	v_forward, v_right, v_up;
	entity_t	*newent;

	if (cl_modelindices[mi_flag] == -1)
		return;

	f = 14;
	if (ent->frame >= 29 && ent->frame <= 40) {
		if (ent->frame >= 29 && ent->frame <= 34) { //axpain
			if      (ent->frame == 29) f = f + 2; 
			else if (ent->frame == 30) f = f + 8;
			else if (ent->frame == 31) f = f + 12;
			else if (ent->frame == 32) f = f + 11;
			else if (ent->frame == 33) f = f + 10;
			else if (ent->frame == 34) f = f + 4;
		} else if (ent->frame >= 35 && ent->frame <= 40) { // pain
			if      (ent->frame == 35) f = f + 2; 
			else if (ent->frame == 36) f = f + 10;
			else if (ent->frame == 37) f = f + 10;
			else if (ent->frame == 38) f = f + 8;
			else if (ent->frame == 39) f = f + 4;
			else if (ent->frame == 40) f = f + 2;
		}
	} else if (ent->frame >= 103 && ent->frame <= 118) {
		if      (ent->frame >= 103 && ent->frame <= 104) f = f + 6;  //nailattack
		else if (ent->frame >= 105 && ent->frame <= 106) f = f + 6;  //light 
		else if (ent->frame >= 107 && ent->frame <= 112) f = f + 7;  //rocketattack
		else if (ent->frame >= 112 && ent->frame <= 118) f = f + 7;  //shotattack
	}

	newent = CL_NewTempEntity ();
	newent->model = cl.model_precache[cl_modelindices[mi_flag]];
	newent->skinnum = team;

	AngleVectors (ent->angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2]; // reverse z component
	for (i=0 ; i<3 ; i++)
		newent->origin[i] = ent->origin[i] - f*v_forward[i] + 22*v_right[i];
	newent->origin[2] -= 16;

	VectorCopy (ent->angles, newent->angles)
	newent->angles[2] -= 45;
}

/*
=============
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CL_LinkPlayers (void)
{
	int				j;
	player_info_t	*info;
	player_state_t	*state;
	player_state_t	exact;
	double			playertime;
	entity_t		*ent;
	centity_t		*cent;
	int				msec;
	frame_t			*frame;
	int				oldphysent;
	vec3_t			org;
	int				flicker; // fuh

	playertime = cls.realtime - cls.latency + 0.02;
	if (playertime > cls.realtime)
		playertime = cls.realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	for (j=0, info=cl.players, state=frame->playerstate ; j < MAX_CLIENTS 
		; j++, info++, state++)
	{
		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		cent = &cl_entities[j + 1];

		// spawn light flashes, even ones coming from invisible objects
#ifdef GLQUAKE
		if (!gl_flashblend.value || j != cl.playernum) {
#endif
			if (r_powerupglow.value && !(r_powerupglow.value == 2 && j == cl.viewplayernum)) { // Tonik
				if (j == cl.playernum) {
					VectorCopy (cl.simorg, org);
				} else
					VectorCopy (state->origin, org);

				flicker = r_lightflicker.value ? (rand()&31) : 15; // fuh

				if ((state->effects & (EF_BLUE | EF_RED)) == (EF_BLUE | EF_RED))
					CL_NewDlight (j+1, org, 200 + flicker, 0.1, lt_redblue, 0);
				else if (state->effects & EF_BLUE)
					CL_NewDlight (j+1, org, 200 + flicker, 0.1, lt_blue, 0);
				else if (state->effects & EF_RED)
					CL_NewDlight (j+1, org, 200 + flicker, 0.1, lt_red, 0);
				else if (state->effects & EF_BRIGHTLIGHT) {
					vec3_t	tmp;
					VectorCopy (org, tmp);
					tmp[2] += 16;
					CL_NewDlight (j+1, tmp, 400 + flicker, 0.1, lt_default, 0);
				} else if (state->effects & EF_DIMLIGHT) {
					dlighttype_t dimlightcolor;
					if (cl.teamfortress)	
						dimlightcolor = dlightColor(r_flagcolor.value, lt_default, false);
					else if (state->effects & (EF_FLAG1|EF_FLAG2))
						dimlightcolor = dlightColor(r_flagcolor.value, lt_default, false);
					else
						dimlightcolor = lt_default;
					CL_NewDlight (j + 1, org, 200 + flicker, 0.1, dimlightcolor, 0);
				}
			}
#ifdef GLQUAKE
		}
#endif

		// the player object never gets added
		if (j == cl.playernum)
			continue;

		if (!state->modelindex)
			continue;

		if (!Cam_DrawPlayer(j))	{
			continue;
		}

		// grab an entity to fill in
		if (cl_numvisedicts == MAX_VISEDICTS) {
			break;		// object list is full
		}
		ent = &cl_visedicts[cl_numvisedicts];
		cl_numvisedicts++;

		memset (ent, 0, sizeof(entity_t));
		ent->model = cl.model_precache[state->modelindex];
		ent->skinnum = state->skinnum;

		ent->frame = state->frame;
		ent->colormap = info->translations;
		if (state->modelindex == cl_modelindices[mi_player])
			ent->scoreboard = info;		// use custom skin
		else
			ent->scoreboard = NULL;

		if (cent->frametime >= 0 && cent->frametime <= cl.time) {
			ent->oldframe = cent->oldframe;
			ent->framelerp = (cl.time - cent->frametime) * 10;
		} else {
			ent->oldframe = ent->frame;
			ent->framelerp = -1;
		}

		//
		// angles
		//
		ent->angles[PITCH] = -state->viewangles[PITCH]/3;
		ent->angles[YAW] = state->viewangles[YAW];
		ent->angles[ROLL] = 0;
		ent->angles[ROLL] = V_CalcRoll (ent->angles, state->velocity)*4;

		// only predict half the move to minimize overruns
		msec = 500*(playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players.value && !cl_predict_players2.value))
		{
			VectorCopy (state->origin, ent->origin);
//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 255)
				msec = 255;
			state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = pmove.numphysent;
			CL_SetSolidPlayers (j);
			CL_PredictUsercmd (state, &exact, &state->command, false);
			pmove.numphysent = oldphysent;
			VectorCopy (exact.origin, ent->origin);
		}

		if (state->effects & EF_FLAG1)
			CL_AddFlagModels (ent, 0);
		else if (state->effects & EF_FLAG2)
			CL_AddFlagModels (ent, 1);

	}
}

//======================================================================

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
===============
*/
void CL_SetSolidEntities (void)
{
	int		i;
	frame_t	*frame;
	packet_entities_t	*pak;
	entity_state_t		*state;

	pmove.physents[0].model = cl.worldmodel;
	VectorCopy (vec3_origin, pmove.physents[0].origin);
	pmove.physents[0].info = 0;
	pmove.numphysent = 1;

	frame = &cl.frames[parsecountmod];
	pak = &frame->packet_entities;

	for (i=0 ; i<pak->num_entities ; i++)
	{
		state = &pak->entities[i];

		if (!state->modelindex)
			continue;
		if (!cl.model_precache[state->modelindex])
			continue;
		if ( cl.model_precache[state->modelindex]->hulls[1].firstclipnode )
		{
			pmove.physents[pmove.numphysent].model = cl.model_precache[state->modelindex];
			VectorCopy (state->origin, pmove.physents[pmove.numphysent].origin);
			pmove.numphysent++;
		}
	}

}

// Highlander -->
static float timediff;
extern float nextdemotime;

static float adjustangle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (ideal > current)
	{

		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	move *= fraction;

	return (current + move);
}

#define ISDEAD(i) ( (i) >=41 && (i) <=102 )

void CL_Interpolate(void)
{
	int i, j;
	frame_t	*frame, *oldframe;
	struct predicted_player *pplayer;
	player_state_t	*state, *oldstate;
	float	f;

	if (!cl.validsequence || !timediff)
		return;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];
	oldframe = &cl.frames[(cl.oldparsecount)&UPDATE_MASK];

	f = (cls.realtime - nextdemotime)/(timediff);
	if (f > 0.0)
		f = 0.0;
	if (f < -1.0)
		f = -1.0;

	// interpolate nails
	for (i=0; i < cl_num_projectiles; i++)
	{
		if (!cl.int_projectiles[i].interpolate)
			continue;

		if (!cl_oldprojectiles[cl.int_projectiles[i].oldindex].origin[0]) 
			Con_Printf("%d %d, %d, %d\n", i, cl.int_projectiles[i].oldindex, cl_projectiles[i].num, i >= cl_num_oldprojectiles);
		for (j=0;j<3;j++) {
			cl_projectiles[i].origin[j] = cl.int_projectiles[i].origin[j] + f*(cl.int_projectiles[i].origin[j] - cl_oldprojectiles[cl.int_projectiles[i].oldindex].origin[j]);
		}
	}

	// interpolate clients
	for (i=0, pplayer = predicted_players, state=frame->playerstate, oldstate=oldframe->playerstate; 
		i < MAX_CLIENTS;
		i++, pplayer++, state++, oldstate++)
	{
		if (pplayer->predict)
		{
			for (j=0;j<3;j++) {
				state->viewangles[j] = adjustangle(oldstate->command.angles[j], pplayer->olda[j], 1.0+f);
				state->origin[j] = pplayer->oldo[j] + f*(pplayer->oldo[j]-oldstate->origin[j]);
			}
			if (i == spec_track) {
				VectorCopy(pplayer->vel, SpeedVec);
			}
		}
	}
}
int	fixangle;

void CL_InitInterpolation(float next, float old)
{
	float	f;
	int		i,j;
	struct predicted_player *pplayer;
	frame_t	*frame, *oldframe;
	player_state_t	*state, *oldstate;
	vec3_t	dist;

	if (!cl.validsequence)
		 return;

	timediff = next-old;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];
	oldframe = &cl.frames[(cl.oldparsecount)&UPDATE_MASK];

	if (!timediff) {
		return;
	}

	f = (cls.realtime - nextdemotime)/(timediff);

	if (f > 0.0)
		f = 0.0;
	if (f < -1.0)
		f = -1.0;

	// clients
	for (i=0, pplayer = predicted_players, state=frame->playerstate, oldstate=oldframe->playerstate; 
		i < MAX_CLIENTS;
		i++, pplayer++, state++, oldstate++) {

		if (pplayer->predict)
		{
			VectorCopy(pplayer->oldo, oldstate->origin);
			VectorCopy(pplayer->olda, oldstate->command.angles);
		}

		pplayer->predict = false;

		if (fixangle & 1 << i)
		{
			if (i == Cam_TrackNum()) {
				VectorCopy(cl.viewangles, state->command.angles);
				VectorCopy(cl.viewangles, state->viewangles);
			}

			// no angle interpolation
			VectorCopy(state->command.angles, oldstate->command.angles);
			
			fixangle &= ~(1 << i);
			//continue;
		}

		if (state->messagenum != cl.parsecount) {
			continue;	// not present this frame
		}

		if (oldstate->messagenum != cl.oldparsecount || !oldstate->messagenum) {
			continue;	// not present last frame
		}

		// we dont interpolate ourself if we are spectating
		if (i == cl.playernum) {
			if (cl.spectator)
				continue;
		}

		if (!ISDEAD(state->frame) && ISDEAD(oldstate->frame))
			continue;
		
		VectorSubtract(state->origin, oldstate->origin, dist);
		if (VectorLength(dist) > 150)
			continue;

		VectorCopy(state->origin, pplayer->oldo);
		VectorCopy(state->command.angles, pplayer->olda);

		pplayer->oldstate = oldstate;
		pplayer->predict = true;
		
		for (j=0;j<3;j++) {
			pplayer->vel[j] = (state->origin[j] - oldstate->origin[j])/timediff;
		}
	}

	// nails
	for (i=0; i < cl_num_projectiles; i++) {
		if (!cl.int_projectiles[i].interpolate)
			continue;
		VectorCopy(cl.int_projectiles[i].origin, cl_projectiles[i].origin);
	}
}

void CL_ClearPredict(void)
{
	memset(predicted_players, 0, sizeof(predicted_players));
	fixangle = 0;
}
// <-- Highlander

/*
===
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
===
*/

void CL_SetUpPlayerPrediction(qboolean dopred)
{
	int				j;
	player_state_t	*state, *oldstate;
	player_state_t	exact;
	double			playertime;
	int				msec;
	frame_t			*frame, *oldframe;
	struct predicted_player *pplayer;

	playertime = cls.realtime - cls.latency + 0.02;
	if (playertime > cls.realtime)
		playertime = cls.realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];
	oldframe = &cl.frames[(cl.oldparsecount)&UPDATE_MASK];
	
	for (j=0, pplayer = predicted_players, state=frame->playerstate, oldstate=oldframe->playerstate; 
		j < MAX_CLIENTS;
		j++, pplayer++, state++, oldstate++) {
	
		pplayer->active = false;

		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum) {
			VectorCopy(cl.frames[cls.netchan.outgoing_sequence&UPDATE_MASK].playerstate[cl.playernum].origin,
				pplayer->origin);
		} else {
			// only predict half the move to minimize overruns
			msec = 500*(playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players.value && !cl_predict_players2.value) ||
				!dopred || cls.mvdplayback) // Highlander
			{
				VectorCopy (state->origin, pplayer->origin);
	//Con_DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
					msec = 255;
				state->command.msec = msec;
	//Con_DPrintf ("predict: %i\n", msec);

				CL_PredictUsercmd (state, &exact, &state->command, false);
				VectorCopy (exact.origin, pplayer->origin);
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers (int playernum)
{
	int		j;
	extern	vec3_t	player_mins;
	extern	vec3_t	player_maxs;
	struct predicted_player *pplayer;
	physent_t *pent;

	if (!cl_solid_players.value)
		return;

	pent = pmove.physents + pmove.numphysent;

	for (j=0, pplayer = predicted_players; j < MAX_CLIENTS;	j++, pplayer++) {

		if (!pplayer->active)
			continue;	// not present this frame

		// the player object never gets added
		if (j == playernum)
			continue;

		if (pplayer->flags & PF_DEAD)
			continue; // dead players aren't solid

		pent->model = 0;
		VectorCopy(pplayer->origin, pent->origin);
		VectorCopy(player_mins, pent->mins);
		VectorCopy(player_maxs, pent->maxs);
		pmove.numphysent++;
		pent++;
	}
}


/*
===============
CL_EmitEntities

Builds the visedicts array for cl.time

Made up of: clients, packet_entities, nails, and tents
===============
*/
void CL_EmitEntities (void)
{
	static int list_index = 0;
	
	if (cls.state != ca_active)
		return;
	if (!cl.validsequence)
		return;

	// swap visedict lists
	cl_oldnumvisedicts = cl_numvisedicts;
	cl_oldvisedicts = cl_visedicts_list[list_index];
	list_index = 1 - list_index;
	cl_visedicts = cl_visedicts_list[list_index];

	cl_numvisedicts = 0;

	CL_LinkPlayers ();
	CL_LinkPacketEntities ();
	CL_LinkProjectiles ();
	CL_UpdateTEnts ();
}


char *cl_modelnames[cl_num_modelindices];
int cl_modelindices[cl_num_modelindices];

void CL_InitEnts(void)
{
//	byte *memalloc;

	memset(cl_modelnames, 0, sizeof(cl_modelnames));

	cl_modelnames[mi_spike] = "progs/spike.mdl";
	cl_modelnames[mi_bigspike] = "progs/s_spike.mdl";
	cl_modelnames[mi_espike] = "progs/e_spike1.mdl";
	cl_modelnames[mi_player] = "progs/player.mdl";
	cl_modelnames[mi_flag] = "progs/flag.mdl";
	cl_modelnames[mi_tf_flag] = "progs/tf_flag.mdl";
	cl_modelnames[mi_tf_stan] = "progs/tf_stan.mdl";
	cl_modelnames[mi_tf_basbkey] = "progs/basbkey.bsp";
	cl_modelnames[mi_tf_basrkey] = "progs/basrkey.bsp";
	cl_modelnames[mi_explod1] = "progs/s_explod.spr";
	cl_modelnames[mi_explod2] = "progs/s_expl.spr";
	cl_modelnames[mi_h_player] = "progs/h_player.mdl";
	cl_modelnames[mi_gib1] = "progs/gib1.mdl";
	cl_modelnames[mi_gib2] = "progs/gib2.mdl";
	cl_modelnames[mi_gib3] = "progs/gib3.mdl";
	cl_modelnames[mi_rocket] = "progs/missile.mdl";
	cl_modelnames[mi_grenade] = "progs/grenade.mdl";
	cl_modelnames[mi_bubble] = "progs/s_bubble.spr";
	cl_modelnames[mi_vaxe] = "progs/v_axe.mdl";
	cl_modelnames[mi_vbio] = "progs/v_bio.mdl";
	cl_modelnames[mi_vgrap] = "progs/v_grap.mdl";
	cl_modelnames[mi_vknife] = "progs/v_knife.mdl";
	cl_modelnames[mi_vknife2] = "progs/v_knife2.mdl";
	cl_modelnames[mi_vmedi] = "progs/v_medi.mdl";
	cl_modelnames[mi_vspan] = "progs/v_span.mdl";
	

//#ifdef GLQUAKE
//	cl_firstpassents.max = 64;
//	cl_firstpassents.alpha = 0;
//
//	cl_visents.max = 256;
//	cl_visents.alpha = 0;
//
//	cl_alphaents.max = 64;
//	cl_alphaents.alpha = 1;
//
//	memalloc = Hunk_AllocName((cl_firstpassents.max + cl_visents.max + cl_alphaents.max) * sizeof(entity_t), "visents");
//	cl_firstpassents.list = (entity_t *) memalloc;
//	cl_visents.list = (entity_t *) memalloc + cl_firstpassents.max;
//	cl_alphaents.list = (entity_t *) memalloc + cl_firstpassents.max + cl_visents.max;
//#else
//	cl_visents.max = 256;
//	cl_visents.alpha = 0;
//
//	cl_visbents.max = 256;
//	cl_visbents.alpha = 0;
//
//	memalloc = Hunk_AllocName((cl_visents.max + cl_visbents.max) * sizeof(entity_t), "visents");
//	cl_visents.list = (entity_t *) memalloc;
//	cl_visbents.list = (entity_t *) memalloc + cl_visents.max;
//#endif
//
//	CL_ClearScene();
}
