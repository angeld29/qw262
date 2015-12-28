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

#include "qwsvdef.h"

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS/8];

void SV_AddToFatPVS (vec3_t org, mnode_t *node)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS ( (mleaf_t *)node, sv.worldmodel);
				for (i=0 ; i<fatbytes ; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}
	
		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS (org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *SV_FatPVS (vec3_t org)
{
	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, sv.worldmodel->nodes);
	return fatpvs;
}

//=============================================================================

// because there can be a lot of nails, there is a special
// network protocol for them
#define	MAX_NAILS	32
edict_t	*nails[MAX_NAILS];
int		numnails;
int		nailcount = 0; // Highlander

extern	int	sv_nailmodel, sv_supernailmodel, sv_playermodel;
extern cvar_t sv_nailhack;

qboolean SV_AddNailUpdate (edict_t *ent)
{
	if (sv_nailhack.value)
		return false;

	if (ent->v.modelindex != sv_nailmodel
		&& ent->v.modelindex != sv_supernailmodel)
		return false;
	if (numnails == MAX_NAILS)
		return true;
	nails[numnails] = ent;
	numnails++;
	return true;
}

void SV_EmitNailUpdate (sizebuf_t *msg, qboolean recorder)
{
	byte	bits[6];	// [48 bits] xyzpy 12 12 12 4 8 
	int		n, i;
	edict_t	*ent;
	int		x, y, z, pitch, yaw;

	if (!numnails)
		return;

	if (recorder)
		MSG_WriteByte (msg, svc_nails2); // Highlander
	else
		MSG_WriteByte (msg, svc_nails);

	MSG_WriteByte (msg, numnails);

	for (n=0 ; n<numnails ; n++)
	{
		ent = nails[n];
		if (recorder) { // Highlander
			if (!ent->v.colormap) {
				if (!((++nailcount)&255)) nailcount++;
				ent->v.colormap = nailcount&255;
			}

			MSG_WriteByte (msg, (byte)ent->v.colormap);
		}

		x = ((int)(ent->v.origin[0] + 4096 + 1) >> 1) & 4095;
		y = ((int)(ent->v.origin[1] + 4096 + 1) >> 1) & 4095;
		z = ((int)(ent->v.origin[2] + 4096 + 1) >> 1) & 4095;
		pitch = Q_rint(ent->v.angles[0]*(16.0/360.0)) & 15;
		yaw = Q_rint(ent->v.angles[1]*(256.0/360.0)) & 255;

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (pitch<<4);
		bits[5] = yaw;

		for (i=0 ; i<6 ; i++)
			MSG_WriteByte (msg, bits[i]);
	}
}

//=============================================================================


/*
==================
SV_WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void SV_WriteDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force)
{
	int		bits;
	int		i;
	float	miss;

// send an update
	bits = 0;
	
	for (i=0 ; i<3 ; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if ( miss < -0.1 || miss > 0.1 )
			bits |= U_ORIGIN1<<i;
	}

	if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;
		
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
		
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;
		
	if ( to->colormap != from->colormap )
		bits |= U_COLORMAP;
		
	if ( to->skinnum != from->skinnum )
		bits |= U_SKIN;
		
	if ( to->frame != from->frame )
		bits |= U_FRAME;
	
	if ( to->effects != from->effects )
		bits |= U_EFFECTS;
	
	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;

	if (bits & 511)
		bits |= U_MOREBITS;

	if (to->flags & U_SOLID)
		bits |= U_SOLID;

	//
	// write the message
	//
	if (!to->number)
		SV_Error ("Unset entity number");
	if (to->number >= 512)
		SV_Error ("Entity number >= 512");

	if (!bits && !force)
		return;		// nothing to send!
	i = to->number | (bits&~511);
	if (i & U_REMOVE)
		Sys_Error ("U_REMOVE");
	MSG_WriteShort (msg, i);
	
	if (bits & U_MOREBITS)
		MSG_WriteByte (msg, bits&255);
	if (bits & U_MODEL)
		MSG_WriteByte (msg,	to->modelindex);
	if (bits & U_FRAME)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		MSG_WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		MSG_WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		MSG_WriteByte (msg, to->effects);
	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);		
	if (bits & U_ANGLE1)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(msg, to->angles[2]);
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of a packet_entities_t to the message.

=============
*/
void SV_EmitPacketEntities (client_t *client, packet_entities_t *to, sizebuf_t *msg)
{
	edict_t	*ent;
	client_frame_t	*fromframe;
	packet_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (client->delta_sequence != -1)
	{
		fromframe = &client->frames[client->delta_sequence & UPDATE_MASK];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		MSG_WriteByte (msg, svc_deltapacketentities);
		MSG_WriteByte (msg, client->delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		MSG_WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (&from->entities[oldindex], &to->entities[newindex], msg, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			ent = EDICT_NUM(newnum);
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (&ent->baseline, &to->entities[newindex], msg, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			MSG_WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0);	// end of packetentities
}

/*
=============
SV_WritePlayersToClient

=============
*/

#define TruePointContents(p) PM_HullPointContents(&sv.worldmodel->hulls[0], 0, p)

#define ISUNDERWATER(x) ((x) == CONTENTS_WATER || (x) == CONTENTS_SLIME || (x) == CONTENTS_LAVA)

static qboolean		disable_updates;	// disables sending entities to the client
										// when server flash is on

void SV_WritePlayersToClient (client_t *client, edict_t *clent, byte *pvs, sizebuf_t *msg)
{
	int			i, j;
	client_t	*cl;
	edict_t		*ent;
	int			msec;
	usercmd_t	cmd;
	int			pflags;
// Highlander -->
	demo_frame_t *demo_frame;
	demo_client_t *dcl;
// <-- Highlander

	demo_frame = &demo.frames[demo.parsecount&DEMO_FRAMES_MASK];

	for (j=0,cl=svs.clients, dcl = demo_frame->clients; j<MAX_CLIENTS ; j++,cl++, dcl++)
	{
		if (cl->state != cs_spawned)
			continue;

		ent = cl->edict;

// Highlander -->
		if (clent == NULL)
		{
			if (cl->spectator)
				continue;

			dcl->parsecount = demo.parsecount;

			VectorCopy(ent->v.origin, dcl->info.origin);
			VectorCopy(ent->v.angles, dcl->info.angles);
			dcl->info.angles[0] *= -3;
#ifdef USE_PR2
			if( cl->isBot )
				VectorCopy(ent->v.v_angle, dcl->info.angles);
#endif
            
			dcl->info.angles[2] = 0; // no roll angle

			if (ent->v.health <= 0)
			{	// don't show the corpse looking around...
				dcl->info.angles[0] = 0;
				dcl->info.angles[1] = ent->v.angles[1];
				dcl->info.angles[2] = 0;
			}

			dcl->info.skinnum = ent->v.skin;
			dcl->info.effects = ent->v.effects;
			dcl->info.weaponframe = ent->v.weaponframe;
			dcl->info.model = ent->v.modelindex;
			dcl->sec = sv.time - cl->localtime;
			dcl->frame = ent->v.frame;
			dcl->flags = 0;
			dcl->cmdtime = cl->localtime;
			dcl->fixangle = demo.fixangle[j];
			demo.fixangle[j] = 0;

			if (ent->v.health <= 0)
				dcl->flags |= DF_DEAD;
			if (ent->v.mins[2] != -24)
				dcl->flags |= DF_GIB;
			continue;
		}
// <-- Highlander
		
		// ZOID visibility tracking
		if (ent != clent &&
			!(client->spec_track && client->spec_track - 1 == j)) 
		{
			if (cl->spectator)
				continue;

			// ignore if not touching a PV leaf
			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;
			if (i == ent->num_leafs)
				continue;		// not visible
		}

		if (disable_updates && client != cl) { // Vladis
			continue;
		}

		pflags = PF_MSEC | PF_COMMAND;

		if (ent->v.modelindex != sv_playermodel)
			pflags |= PF_MODEL;
		for (i=0 ; i<3 ; i++)
			if (ent->v.velocity[i])
				pflags |= PF_VELOCITY1<<i;
		if (ent->v.effects)
			pflags |= PF_EFFECTS;
		if (ent->v.skin)
			pflags |= PF_SKINNUM;
		if (ent->v.health <= 0)
			pflags |= PF_DEAD;
		if (ent->v.mins[2] != -24)
			pflags |= PF_GIB;

		if (cl->spectator)
		{	// only sent origin and velocity to spectators
			pflags &= PF_VELOCITY1 | PF_VELOCITY2 | PF_VELOCITY3;
		}
		else if (ent == clent)
		{	// don't send a lot of data on personal entity
			pflags &= ~(PF_MSEC|PF_COMMAND);
			if (ent->v.weaponframe)
				pflags |= PF_WEAPONFRAME;
		}
		if (client->spec_track && client->spec_track - 1 == j &&
			ent->v.weaponframe) 
			pflags |= PF_WEAPONFRAME;

		MSG_WriteByte (msg, svc_playerinfo);
		MSG_WriteByte (msg, j);
		MSG_WriteShort (msg, pflags);

		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, ent->v.origin[i]);
		
		MSG_WriteByte (msg, ent->v.frame);

		if (pflags & PF_MSEC)
		{
			msec = 1000*(sv.time - cl->localtime);
			if (msec > 255)
				msec = 255;
			MSG_WriteByte (msg, msec);
		}
		
		if (pflags & PF_COMMAND)
		{
			cmd = cl->lastcmd;

			if (ent->v.health <= 0)
			{	// don't show the corpse looking around...
				cmd.angles[0] = 0;
				cmd.angles[1] = ent->v.angles[1];
				cmd.angles[0] = 0;
			}

			cmd.buttons = 0;	// never send buttons
			cmd.impulse = 0;	// never send impulses

			MSG_WriteDeltaUsercmd (msg, &nullcmd, &cmd);
		}

		for (i=0 ; i<3 ; i++)
			if (pflags & (PF_VELOCITY1<<i) )
				MSG_WriteShort (msg, ent->v.velocity[i]);

		if (pflags & PF_MODEL)
			MSG_WriteByte (msg, ent->v.modelindex);

		if (pflags & PF_SKINNUM)
			MSG_WriteByte (msg, ent->v.skin);

		if (pflags & PF_EFFECTS)
			MSG_WriteByte (msg, ent->v.effects);

		if (pflags & PF_WEAPONFRAME)
			MSG_WriteByte (msg, ent->v.weaponframe);
	}
}


/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a svc_packetentities messages and possibly
a svc_nails message and
svc_playerinfo messages
=============
*/

void SV_WriteEntitiesToClient (client_t *client, sizebuf_t *msg, qboolean recorder)
{
	int		e, i, max_packet_entities = MAX_PACKET_ENTITIES;
	byte	*pvs;
	vec3_t	org;
	edict_t	*ent;
	packet_entities_t	*pack;
	edict_t	*clent;
	client_frame_t	*frame;
	entity_state_t	*state;
	client_t	*cl;
	extern	cvar_t	sv_demoNoVis;
	int		max_edicts;


	// this is the frame we are creating
	frame = &client->frames[client->netchan.incoming_sequence & UPDATE_MASK];

	// find the client's PVS
	clent = client->edict;
	pvs = NULL;
	if (!recorder) {
		VectorAdd (clent->v.origin, clent->v.view_ofs, org);
		pvs = SV_FatPVS (org);
	} else { // mvdsv
		max_packet_entities = MVD_MAX_PACKET_ENTITIES;

		for (i=0, cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (cl->state != cs_spawned)
				continue;

			if (cl->spectator)
				continue;

			if (pvs == NULL)
			{
				VectorAdd (cl->edict->v.origin, cl->edict->v.view_ofs, org);
				pvs = SV_FatPVS (org);
			} else {
				VectorAdd (cl->edict->v.origin, cl->edict->v.view_ofs, org);
				SV_AddToFatPVS (org, sv.worldmodel->nodes);
			}
		}
	}

	if (clent && client->disable_updates_stop > svs.realtime) { // Vladis
		int where = TruePointContents(clent->v.origin); // server flash should not work underwater
		disable_updates = !ISUNDERWATER(where);
	} else {
		disable_updates = false;
	}

	// send over the players in the PVS
	SV_WritePlayersToClient (client, clent, pvs, msg);
	
	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;

	numnails = 0;

	if (!disable_updates) {// Vladis, server flash

		// Tonik
		// QW protocol can only handle 512 entities. Any entity with number >= 512 will be invisible
		max_edicts = min (sv.num_edicts, 512);

		for (e=MAX_CLIENTS+1, ent=EDICT_NUM(e) ; e<max_edicts; e++, ent = NEXT_EDICT(ent))
		{
			// ignore ents without visible models
			if (!ent->v.modelindex || !*
#ifdef USE_PR2
					PR2_GetString(ent->v.model)
#else
					PR_GetString(ent->v.model)
#endif
					)
				continue;

			if (!sv_demoNoVis.value || !recorder) { // mvdsv
				// ignore if not touching a PV leaf
				for (i=0 ; i < ent->num_leafs ; i++)
					if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
						break;
					
				if (i == ent->num_leafs)
					continue;		// not visible
			}

			if (SV_AddNailUpdate (ent))
				continue;	// added to the special update list

			// add to the packetentities
			if (pack->num_entities == max_packet_entities)
				continue;	// all full

			state = &pack->entities[pack->num_entities];
			pack->num_entities++;

			state->number = e;
			state->flags = 0;
			VectorCopy (ent->v.origin, state->origin);
			VectorCopy (ent->v.angles, state->angles);
			state->modelindex = ent->v.modelindex;
			state->frame = ent->v.frame;
			state->colormap = ent->v.colormap;
			state->skinnum = ent->v.skin;
			state->effects = ent->v.effects;
		}
	} // server flash

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities (client, pack, msg);

	// now add the specialized nail update
	SV_EmitNailUpdate (msg, recorder);
}

// BorisU -->
void SV_WriteEntitiesToDemo (client_t *client, sizebuf_t *msg)
{
	int		e, i, max_packet_entities = MAX_PACKET_ENTITIES;
	byte	*pvs;
	vec3_t	org;
	edict_t	*ent;
	packet_entities_t	*pack;
	edict_t	*clent;
	client_frame_t	*frame;
	entity_state_t	*state;
	client_t	*cl;
	extern	cvar_t	sv_demoNoVis;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.incoming_sequence & UPDATE_MASK];

	// find the client's PVS
	clent = client->edict;
	pvs = NULL;
	{ // mvdsv
		max_packet_entities = MVD_MAX_PACKET_ENTITIES;

		for (i=0, cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (cl->state != cs_spawned)
				continue;

			if (cl->spectator)
				continue;

			if (pvs == NULL)
			{
				VectorAdd (cl->edict->v.origin, cl->edict->v.view_ofs, org);
				pvs = SV_FatPVS (org);
			} else {
				VectorAdd (cl->edict->v.origin, cl->edict->v.view_ofs, org);
				SV_AddToFatPVS (org, sv.worldmodel->nodes);
			}
		}
	}

	// send over the players in the PVS
	SV_WritePlayersToClient (client, clent, pvs, msg);
	
	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;

	numnails = 0;

	if (client->disable_updates_stop <= svs.realtime) // Vladis

	for (e=MAX_CLIENTS+1, ent=EDICT_NUM(e) ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		// ignore ents without visible models
		if (!ent->v.modelindex || !*
#ifdef USE_PR2
				PR2_GetString(ent->v.model)
#else
				PR_GetString(ent->v.model)
#endif
				)
			continue;

		if (!sv_demoNoVis.value) { // mvdsv
			// ignore if not touching a PV leaf
			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;
				
			if (i == ent->num_leafs)
				continue;		// not visible
		}

		if (SV_AddNailUpdate (ent))
			continue;	// added to the special update list

		// add to the packetentities
		if (pack->num_entities == max_packet_entities)
			continue;	// all full

		state = &pack->entities[pack->num_entities];
		pack->num_entities++;

		state->number = e;
		state->flags = 0;
		VectorCopy (ent->v.origin, state->origin);
		VectorCopy (ent->v.angles, state->angles);
		state->modelindex = ent->v.modelindex;
		state->frame = ent->v.frame;
		state->colormap = ent->v.colormap;
		state->skinnum = ent->v.skin;
		state->effects = ent->v.effects;
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities (client, pack, msg+1);	// to second msg in array

	// now add the specialized nail update
	SV_EmitNailUpdate (msg+2, true);				// to third msg in array
}
// <-- BorisU
