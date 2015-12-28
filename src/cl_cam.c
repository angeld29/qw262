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
/* ZOID
 *
 * Player camera tracking in Spectator mode
 *
 * This takes over player controls for spectator automatic camera.
 * Player moves as a spectator, but the camera tracks and enemy player
 */

#include "quakedef.h"
#include "winquake.h"

static vec3_t desired_position; // where the camera wants to be
static qboolean locked = false;
static int oldbuttons;

// track high fragger
cvar_t cl_hightrack = {"cl_hightrack", "0" };

cvar_t cl_chasecam = {"cl_chasecam", "1"};

//cvar_t cl_camera_maxpitch = {"cl_camera_maxpitch", "10" };
//cvar_t cl_camera_maxyaw = {"cl_camera_maxyaw", "30" };

cvar_t cl_spec_id = {"cl_spec_id", "1" }; // BorisU

player_info_t *last_focus; // 5AT0H

qboolean cam_forceview;
vec3_t cam_viewangles;
double cam_lastviewtime;

int spec_track = 0; // player# of who we are tracking
int ideal_track = 0; // Highlander
float	last_lock = 0; // Highlander
int autocam = CAM_NONE;

void vectoangles(vec3_t vec, vec3_t ang)
{
	float	forward;
	float	yaw, pitch;
	
	if (vec[1] == 0 && vec[0] == 0)
	{
		yaw = 0;
		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(vec[1], vec[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0]*vec[0] + vec[1]*vec[1]);
		pitch = (int) (atan2(vec[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}
// Tonik -->
void Cam_SetViewPlayer (void)
{
	if (cl.spectator && autocam && locked && cl_chasecam.value)
		cl.viewplayernum = spec_track;
	else
		cl.viewplayernum = cl.playernum;
}
// <-- Tonik

// returns true if weapon model should be drawn in camera mode
// 5AT0H -->
qboolean Cam_DrawViewModel(void)
{
	if (!cl.spectator && !cls.mvdplayback)
		return true;

	if (autocam && locked && cl_chasecam.value)
		return true;

	return false;
}

extern int	oldparsecountmod;
extern int	parsecountmod;

// returns true if we should draw this player, we don't if we are chase camming
qboolean Cam_DrawPlayer(int playernum)
{
	if (cl.spectator && autocam && locked && cl_chasecam.value && spec_track == playernum)
		return false;

	return true;
}
// <-- 5AT0H

// Highlander -->
int Cam_TrackNum(void)
{
	if (!autocam)
		return -1;
	return spec_track;
}
// <-- Highlander

void Cam_Unlock(void)
{
	if (autocam) {
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "ptrack");
		autocam = CAM_NONE;
		locked = false;
		Sbar_Changed();
		last_focus = NULL; // BorisU
		if (cls.mvdplayback && cl.teamfortress) 
			V_TF_ClearGrenadeEffects (); // BorisU
	}
}

void Cam_Lock(int playernum)
{
	char st[40];

	sprintf(st, "ptrack %i", playernum);
	if (cls.mvdplayback) { // Highlander
		memcpy(cl.stats, cl.players[playernum].stats, sizeof(cl.stats));
		ideal_track = playernum;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, st);
	spec_track = playernum;
	cam_forceview = true;
	last_lock = cls.realtime;
	locked = false;
	Sbar_Changed();
}

trace_t Cam_DoTrace(vec3_t vec1, vec3_t vec2)
{
	VectorCopy (vec1, pmove.origin);
	return PM_PlayerTrace(pmove.origin, vec2);
}

// Returns distance or 9999 if invalid for some reason
static float Cam_TryFlyby(player_state_t *self, player_state_t *player, vec3_t vec, qboolean checkvis)
{
	vec3_t v;
	trace_t trace;
	float len;

	vectoangles(vec, v);
//	v[0] = -v[0];
	VectorCopy (v, pmove.angles);
	VectorNormalize(vec);
	VectorMA(player->origin, 800, vec, v);
	// v is endpos
	// fake a player move
	trace = Cam_DoTrace(player->origin, v);
	if (/*trace.inopen ||*/ trace.inwater)
		return 9999;
	VectorCopy(trace.endpos, vec);
	VectorSubtract(trace.endpos, player->origin, v);
	len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len < 32 || len > 800)
		return 9999;
	if (checkvis) {
		VectorSubtract(trace.endpos, self->origin, v);
		len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

		trace = Cam_DoTrace(self->origin, vec);
		if (trace.fraction != 1 || trace.inwater)
			return 9999;
	}
	return len;
}

// Is player visible?
static qboolean Cam_IsVisible(player_state_t *player, vec3_t vec)
{
	trace_t trace;
	vec3_t v;

	trace = Cam_DoTrace(player->origin, vec);
	if (trace.fraction != 1 || /*trace.inopen ||*/ trace.inwater)
		return false;
	// check distance, don't let the player get too far away or too close
	VectorSubtract(player->origin, vec, v);

	return ((v[0]*v[0]+v[1]*v[1]+v[2]*v[2]) >= 256);
}

static qboolean InitFlyby(player_state_t *self, player_state_t *player, int checkvis) 
{
	float f, max;
	vec3_t vec, vec2;
	vec3_t forward, right, up;

	VectorCopy(player->viewangles, vec);
	vec[0] = 0;
	AngleVectors (vec, forward, right, up);
//	for (i = 0; i < 3; i++)
//		forward[i] *= 3;

	max = 1000;
	VectorAdd(forward, up, vec2);
	VectorAdd(vec2, right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, up, vec2);
	VectorSubtract(vec2, right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(forward, right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, up, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(forward, up, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	// invert
	VectorSubtract(vec3_origin, forward, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorCopy(forward, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	// invert
	VectorSubtract(vec3_origin, right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorCopy(right, vec2);
	if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
		max = f;
		VectorCopy(vec2, vec);
	}

	// ack, can't find him
	if (max >= 1000) {
//		Cam_Unlock();
		return false;
	}
	locked = true;
	VectorCopy(vec, desired_position); 
	return true;
}

static void Cam_CheckHighTarget(void)
{
	int i, j, max;
	player_info_t	*s;

	j = -1;
	for (i = 0, max = -9999; i < MAX_CLIENTS; i++) {
		s = &cl.players[i];
		if (s->name[0] && !s->spectator && s->frags > max) {
			max = s->frags;
			j = i;
		}
	}
	if (j >= 0) {
		if (!locked || cl.players[j].frags > cl.players[spec_track].frags) {
			Cam_Lock(j);
			ideal_track = spec_track;
		}
	} else
		Cam_Unlock();
}


// 5AT0H -->
int is_corpse_frame (int frame)
{
	if (frame >= 41 && frame <= 102)
	return true;

	return false;
}

#define PLAYER_RADIUS 28

/*
==================
Cam_player_at_crosshair
==================
*/
entity_t *
Cam_player_at_crosshair (void)
{
	int		i;
	entity_t	*ent;
	
	for (i = 0; i < cl_numvisedicts; i++)
	{
		ent = &cl_visedicts[i];
		if (ent->model->type == mod_alias
			&& ent->scoreboard
			&& !is_corpse_frame (ent->frame))
		{
			vec3_t	vdist,		// distance from player at crosshair
					porigin;	// origin of player at crosshair
			vec3_t	v_forward, v_right, v_up;
			float	dist, boundangle, viewangle;

			AngleVectors (cl.viewangles, v_forward, v_right, v_up);
			VectorMA (cl.simorg, 8, v_forward, porigin);
			porigin[2] += 16;

			VectorSubtract (ent->origin, porigin, vdist);
			dist = VectorNormalize (vdist);
			boundangle = atan2 (PLAYER_RADIUS, dist);

			viewangle = acos (DotProduct (v_forward, vdist));

			if (viewangle < boundangle) // near crosshair
			{
				trace_t trace;

				trace = Cam_DoTrace (ent->origin, porigin);
				if (trace.fraction == 1 /*|| trace.inwater*/) // visible
					return (ent);
			}
		}
	}

	return (NULL);
}

/*
==================
Cam_player_id
==================
*/
void
Cam_player_id (void)
{
	char	idstr[MAX_SCOREBOARDNAME];
	entity_t *ent = Cam_player_at_crosshair ();

	if (!ent) return;

	last_focus = ent->scoreboard;

	strcpy (idstr, ent->scoreboard->name);

	SCR_CenterPrint (idstr);
}

int Cam_CheckCrosshair (void)
{
	if (!last_focus)
		return (0);
	
	if (!locked) {
		int i = last_focus - cl.players;
		Cam_Lock (i);
		ideal_track = i;
	} else
		Cam_Unlock ();

	return (1);
}

// <-- 5AT0H 

// ZOID
//
// Take over the user controls and track a player.
// We find a nice position to watch the player and move there
void Cam_Track(usercmd_t *cmd)
{
	player_state_t *player, *self;
	frame_t *frame;
	vec3_t vec;

	if (!cl.spectator)
		return;
	
	if (cl_hightrack.value && !locked)
		Cam_CheckHighTarget();

	if (!autocam || cls.state != ca_active)
		return;

	if (locked && (!cl.players[spec_track].name[0] || cl.players[spec_track].spectator)) {
		locked = false;
		if (cl_hightrack.value)
			Cam_CheckHighTarget();
		else
			Cam_Unlock();
		return;
	}

	frame = &cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK];

	if (autocam && cls.mvdplayback)
	{
		if (ideal_track != spec_track && cls.realtime - last_lock > 0.1 && 
			frame->playerstate[ideal_track].messagenum == cl.parsecount)
			Cam_Lock(ideal_track);

		if (frame->playerstate[spec_track].messagenum != cl.parsecount)
		{
			int i;

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				if (frame->playerstate[i].messagenum == cl.parsecount)
					break;
			}
			if (i < MAX_CLIENTS)
				Cam_Lock(i);
		}
	}

	player = frame->playerstate + spec_track;
	self = frame->playerstate + cl.playernum;

	if (!locked || !Cam_IsVisible(player, desired_position)) {
		if (!locked || cls.realtime - cam_lastviewtime > 0.1) {
			if (!InitFlyby(self, player, true))
				InitFlyby(self, player, false);
			cam_lastviewtime = cls.realtime;
		}
	} else
		cam_lastviewtime = cls.realtime;

	// couldn't track for some reason
	if (!locked || !autocam)
		return;

	if (cl_chasecam.value) {
		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;

		VectorCopy(player->viewangles, cl.viewangles);
		VectorCopy(player->origin, desired_position);

		if (memcmp(&desired_position, &self->origin, sizeof(desired_position)) != 0) {
			MSG_WriteByte (&cls.netchan.message, clc_tmove);
			MSG_WriteCoord (&cls.netchan.message, desired_position[0]);
			MSG_WriteCoord (&cls.netchan.message, desired_position[1]);
			MSG_WriteCoord (&cls.netchan.message, desired_position[2]);
			// move there locally immediately
			VectorCopy(desired_position, self->origin);
		}
		self->weaponframe = player->weaponframe;

	} else {
		// Ok, move to our desired position and set our angles to view
		// the player
		VectorSubtract(desired_position, self->origin, vec);
		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
		if (VectorLength(vec) > 16) { // close enough?
			MSG_WriteByte (&cls.netchan.message, clc_tmove);
			MSG_WriteCoord (&cls.netchan.message, desired_position[0]);
			MSG_WriteCoord (&cls.netchan.message, desired_position[1]);
			MSG_WriteCoord (&cls.netchan.message, desired_position[2]);
		}

		// move there locally immediately
		VectorCopy(desired_position, self->origin);
		VectorSubtract(player->origin, desired_position, vec);
		vectoangles(vec, cl.viewangles);
		cl.viewangles[0] = -cl.viewangles[0];
	}
}

void Cam_FinishMove(usercmd_t *cmd)
{
	int i;
	player_info_t	*s;
	int end;

	if (cls.state != ca_active)
		return;

	if (!cl.spectator && !cls.mvdplayback) // only in spectator mode
		return;

	if (cl_spec_id.value == 1 && !autocam)
		Cam_player_id(); // BorisU

	if (cmd->buttons & BUTTON_ATTACK) {
		if (!(oldbuttons & BUTTON_ATTACK)) {

			oldbuttons |= BUTTON_ATTACK;
			autocam++;

			if (autocam > CAM_TRACK) {
				Cam_Unlock();
				VectorCopy(cmd->angles,cl.viewangles);
				return;
			}

			if (Cam_CheckCrosshair ())	// 5AT0H
				return;					//

		} else
			return;
	} else {
		oldbuttons &= ~BUTTON_ATTACK;
		if (!autocam)
			return;
	}

	if (autocam && cl_hightrack.value) {
		Cam_CheckHighTarget();
		return;
	}

	if (locked) {
		if ((cmd->buttons & BUTTON_JUMP) && (oldbuttons & BUTTON_JUMP))
			return;		// don't pogo stick

		if (!(cmd->buttons & BUTTON_JUMP)) {
			oldbuttons &= ~BUTTON_JUMP;
			return;
		}
		oldbuttons |= BUTTON_JUMP;	// don't jump again until released
	}

//	Con_Printf("Selecting track target...\n");

	if (locked && autocam)
		end = (ideal_track + 1) % MAX_CLIENTS;
	else
		end = ideal_track;
	i = end;
	do {
		s = &cl.players[i];
		if (s->name[0] && !s->spectator) {
			if (cls.mvdplayback && cl.teamfortress) 
				V_TF_ClearGrenadeEffects(); // BorisU
			Cam_Lock(i);
			Con_Printf("tracking %s\n", s->name);
			ideal_track = i;
			return;
		}
		i = (i + 1) % MAX_CLIENTS;
	} while (i != end);
	// stay on same guy?
	i = ideal_track;
	s = &cl.players[i];
	if (s->name[0] && !s->spectator) {
		Cam_Lock(i);
		ideal_track = i;
		return;
	}
	Con_Printf("No target found ...\n");
	autocam = locked = false;
}

void Cam_Reset(void)
{
	autocam = CAM_NONE;
	spec_track = 0;
}

/*
====================
CL_Track_f

track <user or userid>

track after specified player
====================
*/
void CL_Track_f (void)
{
	int		uid;
	int		i;
	player_info_t	*target;
	
	if (cls.state != ca_active)
		return;

	if (!cl.spectator) // only in spectator mode
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi (Cmd_Argv(1));

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		target = &cl.players[i];
		
		if (!target->name[0])
			continue;

		if (target->userid == uid
			|| !strcmp (target->name, Cmd_Argv(1)))
		{
			if (target->spectator)
			{
				Con_Printf ("User is spectator.\n");
				return;
			}
				
			autocam = true;
			Cam_Lock (i);
			if (cls.mvdplayback && cl.teamfortress) 
				V_TF_ClearGrenadeEffects(); // BorisU
			return;
		}
	}
	Con_Printf ("User not in server.\n");
}
/*
===============
Cam_TryLock

Fixes spectator chasecam demos
===============
*/
void Cam_TryLock (void)
{
	int				i, j;
	player_state_t *state;
	int				old_autocam, old_spec_track;
	static	float	cam_lastlocktime;

	if (!cl.validsequence)
		return;

	if (!autocam)
		cam_lastlocktime = 0;

	old_autocam = autocam;
	old_spec_track = spec_track;

	state = cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].playerstate;
	for (i=0 ; i<MAX_CLIENTS ; i++) {
		if (!cl.players[i].name[0] || cl.players[i].spectator ||
			state[i].messagenum != cl.parsecount)
			continue;
		if (fabs(state[i].command.angles[0] - cl.viewangles[0]) < 2 &&
			fabs(state[i].command.angles[1] - cl.viewangles[1]) < 2)
		{
			for (j=0 ; j<3 ; j++)
				if (fabs(state[i].origin[j] - state[cl.playernum].origin[j]) > 200)
					break;	// too far
			if (j < 3)
				continue;
			autocam = CAM_TRACK;
			spec_track = i;
			locked = true;
			cam_lastlocktime = cls.realtime;
			Cvar_Set(&cl_chasecam, "1");
			break;
		}
	}

	if (cls.realtime - cam_lastlocktime > 0.3) {
		// Couldn't lock to any player for 0.3 seconds, so assume
		// the spectator switched to free spectator mode
		autocam = CAM_NONE;
		spec_track = 0;
		locked = false;
	}

	if (autocam != old_autocam || spec_track != old_spec_track)
		Sbar_Changed ();
}

void CL_InitCam(void)
{
	Cvar_RegisterVariable (&cl_hightrack);
	Cvar_RegisterVariable (&cl_chasecam);
//	Cvar_RegisterVariable (&cl_camera_maxpitch);
//	Cvar_RegisterVariable (&cl_camera_maxyaw);
	Cvar_RegisterVariable (&cl_spec_id);

	Cmd_AddCommand ("track", CL_Track_f); // 5AT0H
}


