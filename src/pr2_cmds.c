/*
 *  QW262
 *  Copyright (C) 2004  [sd] angel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: pr2_cmds.c,v 1.32 2007/01/07 19:05:54 angel Exp $
 */

#include <stdarg.h>

#include "qwsvdef.h"
#include "g_public.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifdef USE_PR2

#include "vm.h"

char   *pr2_ent_data_ptr;
vm_t   *sv_vm = NULL;


int PASSFLOAT( float f )
{
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
============
PR2_RunError

Aborts the currently executing function
============
*/
void PR2_RunError( char *error, ... )
{
	va_list argptr;
	char    string[1024];

	va_start( argptr, error );
	vsprintf( string, error, argptr );
	va_end( argptr );

        sv_error = true;
/*	if ( sv_vm->type == VM_BYTECODE )
		QVM_StackTrace( sv_vm->hInst );*/

	Con_Printf( "%s\n", string );

	SV_Error( "Program error" );
}

void PR2_CheckEmptyString( char *s )
{
	if ( !s || s[0] <= ' ' )
		PR2_RunError( "Bad string" );
}

void PF2_precache_sound( char*s )
{
	int     i;

	if ( sv.state != ss_loading )
		PR2_RunError( "PF_Precache_*: Precache can only be done in spawn " "functions" );
	PR2_CheckEmptyString( s );

	for ( i = 0; i < MAX_SOUNDS; i++ )
	{
		if ( !sv.sound_precache[i] )
		{
			sv.sound_precache[i] = s;
			return;
		}
		if ( !strcmp( sv.sound_precache[i], s ) )
			return;
	}
	PR2_RunError( "PF_precache_sound: overflow" );
}

void PF2_precache_model(char* s)
{
	int     i;

	if ( sv.state != ss_loading )
		PR2_RunError( "PF_Precache_*: Precache can only be done in spawn " "functions" );

	PR2_CheckEmptyString( s );

	for ( i = 0; i < MAX_MODELS; i++ )
	{
		if ( !sv.model_precache[i] )
		{
			sv.model_precache[i] = s;
			return;
		}
		if ( !strcmp( sv.model_precache[i], s ) )
			return;
	}

	PR2_RunError( "PF_precache_model: overflow" );
}

/*
=================
PF2_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/

void PF2_setorigin( edict_t* e, float x, float y, float z)
{
	vec3_t  origin;

	origin[0] = x;
	origin[1] = y;
	origin[2] = z;

	VectorCopy( origin, e->v.origin );
	SV_LinkEdict( e, false );
}

/*
=================
PF2_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF2_setsize( edict_t* e, float x1, float y1, float z1, float x2, float y2, float z2)
{
	//vec3_t min, max;
	e->v.mins[0] = x1;
	e->v.mins[1] = y1;
	e->v.mins[2] = z1;

	e->v.maxs[0] = x2;
	e->v.maxs[1] = y2;
	e->v.maxs[2] = z2;

	VectorSubtract( e->v.maxs, e->v.mins, e->v.size );

	SV_LinkEdict( e, false );
}

/*
=================
PF2_setmodel

setmodel(entity, model)
Also sets size, mins, and maxs for inline bmodels
=================
*/
void PF2_setmodel( edict_t* e, char*m)
{
	char  **check;
	int     i;
	model_t *mod;

	if ( !m )
		m = "";
	// check to see if model was properly precached
	for ( i = 0, check = sv.model_precache; *check; i++, check++ )
		if ( !strcmp( *check, m ) )
			break;

	if ( !*check )
		PR2_RunError( "no precache: %s\n", m );

	e->v.model = PR2_SetString( m );
	e->v.modelindex = i;

	// if it is an inline model, get the size information for it
	if ( m[0] == '*' )
	{
		mod = Mod_ForName( m, true );
		VectorCopy( mod->mins, e->v.mins );
		VectorCopy( mod->maxs, e->v.maxs );
		VectorSubtract( mod->maxs, mod->mins, e->v.size );
		SV_LinkEdict( e, false );
	}

}

/*
=================
PF2_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF2_sprint( int entnum, int level, char* s )
{
	client_t *client;

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to sprint to a non-client %d \n", entnum );
		return;
	}

	client = &svs.clients[entnum - 1];

	SV_ClientPrintf( client, level, "%s", s );
}

/*
=================
PF2_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF2_centerprint( int entnum, char* s )
{
	client_t *cl;

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to sprint to a non-client\n" );
		return;
	}

	cl = &svs.clients[entnum - 1];

	ClientReliableWrite_Begin( cl, svc_centerprint, 2 + strlen( s ) );
	ClientReliableWrite_String( cl, s );

	if ( sv.demorecording )
	{
		DemoWrite_Begin( dem_single, entnum - 1, 2 + strlen( s ) );
		MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, svc_centerprint );
		MSG_WriteString( ( sizebuf_t * ) demo.dbuf, s );
	}

}

/*
=================
PF2_ambientsound

=================
*/
void PF2_ambientsound( float x, float y, float z, char* samp, float vol, float attenuation)
{
	char  **check;
	int     i, soundnum;
	vec3_t  pos;

	pos[0] = x;
	pos[1] = y;
	pos[2] = z;

	if ( !samp )
		samp = "";

	// check to see if samp was properly precached
	for ( soundnum = 0, check = sv.sound_precache; *check; check++, soundnum++ )
		if ( !strcmp( *check, samp ) )
			break;

	if ( !*check )
	{
		Con_Printf( "no precache: %s\n", samp );
		return;
	}
	// add an svc_spawnambient command to the level signon packet
	MSG_WriteByte( &sv.signon, svc_spawnstaticsound );
	for ( i = 0; i < 3; i++ )
		MSG_WriteCoord( &sv.signon, pos[i] );

	MSG_WriteByte( &sv.signon, soundnum );

	MSG_WriteByte( &sv.signon, vol * 255 );
	MSG_WriteByte( &sv.signon, attenuation * 64 );

}

/*
=================
PF2_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
/*
=================
PF2_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF2_traceline( float v1_x, float v1_y, float v1_z, 
			float v2_x, float v2_y, float v2_z, 
			int nomonsters, edict_t* ent)
{
	trace_t trace;
	vec3_t  v1, v2;

	v1[0] = v1_x;
	v1[1] = v1_y;
	v1[2] = v1_z;

	v2[0] = v2_x;
	v2[1] = v2_y;
	v2[2] = v2_z;

	trace = SV_Trace( v1, vec3_origin, vec3_origin, v2, nomonsters, ent );

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy( trace.endpos, pr_global_struct->trace_endpos );
	VectorCopy( trace.plane.normal, pr_global_struct->trace_plane_normal );
	pr_global_struct->trace_plane_dist = trace.plane.dist;

	if ( trace.e.ent )
		pr_global_struct->trace_ent = EDICT_TO_PROG( trace.e.ent );
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG( sv.edicts );
}

/*
=================
PF2_TraceCapsule

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

void    trap_TraceCapsule( float v1_x, float v1_y, float v1_z, 
			float v2_x, float v2_y, float v2_z, 
			int nomonst, int edn ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z);

=================
*/
void PF2_TraceCapsule( float v1_x, float v1_y, float v1_z, 
			float v2_x, float v2_y, float v2_z, 
			int nomonsters, edict_t* ent,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z)
{
	trace_t trace;
	vec3_t  v1, v2, v3, v4;

	v1[0] = v1_x;
	v1[1] = v1_y;
	v1[2] = v1_z;

	v2[0] = v2_x;
	v2[1] = v2_y;
	v2[2] = v2_z;

	v3[0] = min_x;
	v3[1] = min_y;
	v3[2] = min_z;

	v4[0] = max_x;
	v4[1] = max_y;
	v4[2] = max_z;

	trace = SV_Trace( v1, v3, v4, v2, nomonsters, ent );

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy( trace.endpos, pr_global_struct->trace_endpos );
	VectorCopy( trace.plane.normal, pr_global_struct->trace_plane_normal );
	pr_global_struct->trace_plane_dist = trace.plane.dist;

	if ( trace.e.ent )
		pr_global_struct->trace_ent = EDICT_TO_PROG( trace.e.ent );
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG( sv.edicts );
}

/*
=================
PF2_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/

byte    checkpvs[MAX_MAP_LEAFS / 8];

int PF2_newcheckclient( int check )
{
	int     i;
	byte   *pvs;
	edict_t *ent;
	mleaf_t *leaf;
	vec3_t  org;

	// cycle to the next one
	if ( check < 1 )
		check = 1;
	if ( check > MAX_CLIENTS )
		check = MAX_CLIENTS;

	if ( check == MAX_CLIENTS )
		i = 1;
	else
		i = check + 1;

	for ( ;; i++ )
	{
		if ( i == MAX_CLIENTS + 1 )
			i = 1;

		ent = EDICT_NUM( i );

		if ( i == check )
			break;	// didn't find anything else

		if ( ent->free )
			continue;
		if ( ent->v.health <= 0 )
			continue;
		if ( ( int ) ent->v.flags & FL_NOTARGET )
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	VectorAdd( ent->v.origin, ent->v.view_ofs, org );
	leaf = Mod_PointInLeaf( org, sv.worldmodel );
	pvs = Mod_LeafPVS( leaf, sv.worldmodel );
	memcpy( checkpvs, pvs, ( sv.worldmodel->numleafs + 7 ) >> 3 );

	return i;
}


#define	MAX_CHECK	16
int     c_invis, c_notvis;


int PF2_checkclient()
{
	edict_t *ent, *self;
	mleaf_t *leaf;
	int     l;
	vec3_t  view;

	// find a new check if on a new frame
	if ( sv.time - sv.lastchecktime >= 0.1 )
	{
		sv.lastcheck = PF2_newcheckclient( sv.lastcheck );
		sv.lastchecktime = sv.time;
	}
	// return check if it might be visible  
	ent = EDICT_NUM( sv.lastcheck );
	if ( ent->free || ent->v.health <= 0 )
	{
		// RETURN_EDICT(sv.edicts);
		return NUM_FOR_EDICT( sv.edicts );
	}
	// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT( pr_global_struct->self );
	VectorAdd( self->v.origin, self->v.view_ofs, view );
	leaf = Mod_PointInLeaf( view, sv.worldmodel );
	l = ( leaf - sv.worldmodel->leafs ) - 1;
	if ( ( l < 0 ) || !( checkpvs[l >> 3] & ( 1 << ( l & 7 ) ) ) )
	{
		c_notvis++;
		return  NUM_FOR_EDICT( sv.edicts );

	}
	// might be able to see it
	c_invis++;

	return  NUM_FOR_EDICT( ent );
}

//============================================================================
// modified by Tonik
/*
=================
PF2_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF2_stuffcmd( int entnum, char* str)
{
	client_t *cl;
	char   *buf;


	if ( entnum < 1 || entnum > MAX_CLIENTS )
		PR2_RunError( "Parm 0 not a client" );

	if ( !str )
		PR2_RunError( "PF2_stuffcmd: NULL pointer" );

	cl = &svs.clients[entnum - 1];
	if ( !strcmp( str, "disconnect\n" ) )
	{
       		// so long and thanks for all the fish
       		cl->drop = true;
       		return;
	}

	buf = cl->stufftext_buf;
	if ( strlen( buf ) + strlen( str ) >= MAX_STUFFTEXT )
		PR2_RunError( "stufftext buffer overflow" );
	strcat( buf, str );
	if( strchr( buf, '\n' ) )
	{
	        ClientReliableWrite_Begin( cl, svc_stufftext, 2 + strlen( buf ) );
		ClientReliableWrite_String( cl, buf );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 2 + strlen( buf ) );
			MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, svc_stufftext );
			MSG_WriteString( ( sizebuf_t * ) demo.dbuf, buf );
		}
		buf[0] = 0;
	}
}

void PF2_executecmd( )
{
	int     old_other, old_self;	// mod_consolecmd will be executed, so we need to store this

	old_self = pr_global_struct->self;
	old_other = pr_global_struct->other;

	Cbuf_Execute(  );

	pr_global_struct->self = old_self;
	pr_global_struct->other = old_other;
}


/*
=================
PF2_readcmd

void readmcmd (string str,string buff, int sizeofbuff)
=================
*/

void PF2_readcmd( char*str, char*buf, int sizebuff)
{
	extern char outputbuf[];
	extern redirect_t sv_redirected;
	redirect_t old;

	Cbuf_Execute(  );
	Cbuf_AddText( str );

	old = sv_redirected;

	if ( old != RD_NONE )
		SV_EndRedirect(  );

	SV_BeginRedirect( RD_MOD );
	Cbuf_Execute(  );

	strlcpy( buf, outputbuf, sizebuff );

	SV_EndRedirect(  );

	if ( old != RD_NONE )
		SV_BeginRedirect( old );

}

/*
=================
PF2_redirectcmd

void redirectcmd (entity to, string str)
=================
*/



void PF2_redirectcmd( int entnum, char* str )
{
//      redirect_t old;

	extern redirect_t sv_redirected;

	if ( sv_redirected )
	{
		Cbuf_AddText( str );
		Cbuf_Execute(  );
		return;
	}


	if ( entnum < 1 || entnum > MAX_CLIENTS )
		PR2_RunError( "Parm 0 not a client" );


	SV_BeginRedirect( RD_MOD + entnum );
	Cbuf_AddText( str );
	Cbuf_Execute(  );
	SV_EndRedirect(  );

}

/*
===============
PF2_walkmove

float(float yaw, float dist) walkmove
===============
*/
int PF2_walkmove( edict_t* ent, float yaw, float dist )
//(int entn, float yaw, float dist)
{
	vec3_t  move;
    int ret;
	int     oldself;

	if ( !( ( int ) ent->v.flags & ( FL_ONGROUND | FL_FLY | FL_SWIM ) ) )
	{
		return 0;

	}

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos( yaw ) * dist;
	move[1] = sin( yaw ) * dist;
	move[2] = 0;

	// save program state, because SV_movestep may call other progs
//      oldf = pr_xfunction;
	oldself = pr_global_struct->self;

	ret = SV_movestep( ent, move, true );


	// restore program state
//      pr_xfunction = oldf;
	pr_global_struct->self = oldself;
	return ret;
}

/*
===============
PF2_droptofloor

void(entnum) droptofloor
===============
*/
int PF2_droptofloor( edict_t* ent )
{
	vec3_t  end;
	trace_t trace;


	VectorCopy( ent->v.origin, end );
	end[2] -= 256;

	trace = SV_Trace( ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent );

	if ( trace.fraction == 1 || trace.allsolid )
	{
		return 0;
	} else
	{
		VectorCopy( trace.endpos, ent->v.origin );
		SV_LinkEdict( ent, false );
		ent->v.flags = ( int ) ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = EDICT_TO_PROG( trace.e.ent );
		return 1;
	}
}

/*
===============
PF2_lightstyle

void(int style, string value) lightstyle
===============
*/
void PF2_lightstyle( int style, char* val)
{
	client_t *client;
	int     j;

// change the string in sv
	sv.lightstyles[style] = val;

// send message to all clients on this server
	if ( sv.state != ss_active )
		return;

	for ( j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++ )
		if ( client->state == cs_spawned )
		{
			ClientReliableWrite_Begin( client, svc_lightstyle, strlen( val ) + 3 );
			ClientReliableWrite_Char( client, style );
			ClientReliableWrite_String( client, val );
		}

	if ( sv.demorecording )
	{
		DemoWrite_Begin( dem_all, 0, strlen( val ) + 3 );
		MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, svc_lightstyle );
		MSG_WriteChar( ( sizebuf_t * ) demo.dbuf, style );
		MSG_WriteString( ( sizebuf_t * ) demo.dbuf, val );
	}
}

/*
=============
PF2_pointcontents
=============
*/
int PF2_pointcontents( float x, float y, float z )
{
	vec3_t  origin;

	origin[0] = x;
	origin[1] = y;
	origin[2] = z;

	return SV_PointContents( origin );
}

/*
=============
PF2_nextent

entity nextent(entity)
=============
*/
int PF2_nextent( int i )
{
	edict_t *ent;

	while ( 1 )
	{
		i++;
		if ( i >= sv.num_edicts )
		{
			return 0;
		}
		ent = EDICT_NUM( i );
		if ( !ent->free )
		{
			return i;
		}
	}
}

/*
=============
PF2_nextclient

fast walk over spawned clients
 
entity nextclient(entity)
=============
*/
intptr_t PF2_nextclient(int i)
{
	edict_t	*ent;

	while (1)
	{
		i++;
		if (i < 1 || i > MAX_CLIENTS)
		{
			return 0;
		}
		ent = EDICT_NUM(i);
		if (!ent->free) // actually that always true for clients edicts
		{
			if (svs.clients[i-1].state == cs_spawned) // client in game
			{
				return VM_Ptr2VM(ent);
			}
		}
	}
}

/*
=============
PF2_find

entity find(start,fieldoff,str)
=============
*/
intptr_t PF2_Find( edict_t* ed, int fofs, char*str)
{
	int     e;
	char   *t;

	e = NUM_FOR_EDICT( ed );

	if ( !str )
		PR2_RunError( "PF2_Find: bad search string" );

	for ( e++; e < sv.num_edicts; e++ )
	{
		ed = EDICT_NUM( e );
		if ( ed->free )
			continue;
		t = VM_ArgPtr(  *( int * ) ( ( char * ) ed + fofs ) );
		if ( !t )
			continue;
		if ( !strcmp( t, str ) )
		{
			return VM_Ptr2VM( ed );
		}

	}
	return 0;
}

/*
=================
PF2_findradius

Returns a chain of entities that have origins within a spherical area
gedict_t *findradius( gedict_t * start, vec3_t org, float rad );
=================
*/

static intptr_t PF2_FindRadius( int e, float* org, float rad )
{
	int     j;

	edict_t *ed;
	vec3_t	eorg;

	for ( e++; e < sv.num_edicts; e++ )
	{
		ed = EDICT_NUM( e );

		if ( ed->free )
			continue;
		if (ed->v.solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ed->v.origin[j] + (ed->v.mins[j] + ed->v.maxs[j])*0.5);			
		if (VectorLength(eorg) > rad)
			continue;
		return VM_Ptr2VM( ed );
	}
	return 0;
}

/*
=============
PF2_aim ??????

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
/*
==============
PF2_changeyaw ???

This was a major timewaster in progs, so it was converted to C
==============
*/

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/


#define	MSG_BROADCAST	0	// unreliable to all
#define	MSG_ONE			1	// reliable to one (msg_entity)
#define	MSG_ALL			2	// reliable to all
#define	MSG_INIT		3	// write to the init string
#define	MSG_MULTICAST	4	// for multicast()


sizebuf_t *WriteDest2( int dest )
{
//      int             entnum;
//      int             dest;
//      edict_t *ent;

	//dest = G_FLOAT(OFS_PARM0);
	switch ( dest )
	{
	case MSG_BROADCAST:
		return &sv.datagram;

	case MSG_ONE:
		SV_Error( "Shouldn't be at MSG_ONE" );
#if 0
		ent = PROG_TO_EDICT( pr_global_struct->msg_entity );
		entnum = NUM_FOR_EDICT( ent );
		if ( entnum < 1 || entnum > MAX_CLIENTS )
			PR2_RunError( "WriteDest: not a client" );
		return &svs.clients[entnum - 1].netchan.message;
#endif

	case MSG_ALL:
		return &sv.reliable_datagram;

	case MSG_INIT:
		if ( sv.state != ss_loading )
			PR2_RunError( "PF_Write_*: MSG_INIT can only be written in spawn " "functions" );
		return &sv.signon;

	case MSG_MULTICAST:
		return &sv.multicast;

	default:
		PR2_RunError( "WriteDest: bad destination" );
		break;
	}

	return NULL;
}

static client_t *Write_GetClient( void )
{
	int     entnum;
	edict_t *ent;

	ent = PROG_TO_EDICT( pr_global_struct->msg_entity );
	entnum = NUM_FOR_EDICT( ent );
	if ( entnum < 1 || entnum > MAX_CLIENTS )
		PR2_RunError( "WriteDest: not a client" );
	return &svs.clients[entnum - 1];
}

void PF2_WriteByte( int to, int data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 1 );
		ClientReliableWrite_Byte( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 1 );
			MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteByte( WriteDest2( to ), data );
}

void PF2_WriteChar( int to, int data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 1 );
		ClientReliableWrite_Char( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 1 );
			MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteChar( WriteDest2( to ), data );
}

void PF2_WriteShort( int to, int data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 2 );
		ClientReliableWrite_Short( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 2 );
			MSG_WriteShort( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteShort( WriteDest2( to ), data );
}

void PF2_WriteLong( int to, int data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 4 );
		ClientReliableWrite_Long( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 4 );
			MSG_WriteLong( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteLong( WriteDest2( to ), data );
}

void PF2_WriteAngle( int to, float data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 1 );
		ClientReliableWrite_Angle( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 1 );
			MSG_WriteByte( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteAngle( WriteDest2( to ), data );
}

void PF2_WriteCoord( int to, float data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 2 );
		ClientReliableWrite_Coord( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 2 );
			MSG_WriteCoord( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteCoord( WriteDest2( to ), data );
}

void PF2_WriteString( int to, char* data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 1 + strlen( data ) );
		ClientReliableWrite_String( cl, data );
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 1 + strlen( data ) );
			MSG_WriteString( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteString( WriteDest2( to ), data );
}


void PF2_WriteEntity( int to, int data )
{
	if ( to == MSG_ONE )
	{
		client_t *cl = Write_GetClient(  );

		ClientReliableCheckBlock( cl, 2 );
		ClientReliableWrite_Short( cl, data );	//G_EDICTNUM(OFS_PARM1)
		if ( sv.demorecording )
		{
			DemoWrite_Begin( dem_single, cl - svs.clients, 2 );
			MSG_WriteShort( ( sizebuf_t * ) demo.dbuf, data );
		}
	} else
		MSG_WriteShort( WriteDest2( to ), data );
}

//=============================================================================

int     SV_ModelIndex( char *name );

/*
==================
PF2_makestatic 

==================
*/
void PF2_makestatic( edict_t* ent )
{
	int     i;

	MSG_WriteByte( &sv.signon, svc_spawnstatic );

	MSG_WriteByte( &sv.signon, SV_ModelIndex( VM_ArgPtr( ent->v.model ) ) );

	MSG_WriteByte( &sv.signon, ent->v.frame );
	MSG_WriteByte( &sv.signon, ent->v.colormap );
	MSG_WriteByte( &sv.signon, ent->v.skin );
	for ( i = 0; i < 3; i++ )
	{
		MSG_WriteCoord( &sv.signon, ent->v.origin[i] );
		MSG_WriteAngle( &sv.signon, ent->v.angles[i] );
	}

	// throw the entity away now
	ED_Free( ent );
}

//=============================================================================

/*
==============
PF2_setspawnparms
==============
*/
void PF2_setspawnparms( int entnum )
{
	int     i;

	//edict_t               *ent;
	client_t *client;

	//ent = EDICT_NUM(entnum);

	if ( entnum < 1 || entnum > MAX_CLIENTS )
		PR2_RunError( "Entity is not a client" );

	// copy spawn parms out of the client_t
	client = svs.clients + ( entnum - 1 );

	for ( i = 0; i < NUM_SPAWN_PARMS; i++ )
		( &pr_global_struct->parm1 )[i] = client->spawn_parms[i];
}

/*
==============
PF2_changelevel
==============
*/
void PF2_changelevel( char* s )
{
	static int last_spawncount;

// make sure we don't issue two changelevels
	if ( svs.spawncount == last_spawncount )
		return;
	last_spawncount = svs.spawncount;

	Cbuf_AddText( va( "map %s\n", s ) );
}

/*
==============
PF2_logfrag

logfrag (killer, killee)
==============
*/
void PF2_logfrag( int e1, int e2 )
{
//      edict_t *ent1, *ent2;
	char   *s;

	//ent1 = G_EDICT(OFS_PARM0);
	//ent2 = G_EDICT(OFS_PARM1);

	if ( e1 < 1 || e1 > MAX_CLIENTS || e2 < 1 || e2 > MAX_CLIENTS )
		return;

	s = va( "\\%s\\%s\\\n", svs.clients[e1 - 1].name, svs.clients[e2 - 1].name );

	SZ_Print( &svs.log[svs.logsequence & 1], s );
	Log_Printf( SV_FRAGLOG, s );
}

/*
==============
PF2_getinfokey

string(entity e, string key) infokey
==============
*/
void PF2_infokey( int e1, char* key, char* valbuff, int sizebuff )
//(int e1, char *key, char *valbuff, int sizebuff)
{
	static char ov[256];

	char   *value;

	if ( !key || !valbuff )
		PR2_RunError( "PF2_infokey: NULL pointer" );

	if ( e1 == 0 )
	{
		if ( ( value = Info_ValueForKey( svs.info, key ) ) == NULL || !*value )
			value = Info_ValueForKey( localinfo, key );
	} else if ( e1 <= MAX_CLIENTS )
	{
		if ( !strcmp( key, "ip" ) )
		{
			strlcpy( ov, NET_BaseAdrToString( svs.clients[e1 - 1].netchan.remote_address ),
				 sizeof( ov ) );
			value = ov;
		} else if ( !strcmp( key, "ping" ) )
		{
			int     ping = SV_CalcPing( &svs.clients[e1 - 1] );

			sprintf( ov, "%d", ping );
			value = ov;
		} else if ( !strcmp( key, "*userid" ) )
		{
			sprintf( ov, "%d", svs.clients[e1 - 1].userid );
			value = ov;
		} else
			value = Info_ValueForKey( svs.clients[e1 - 1].userinfo, key );
	} else
		value = "";

	if ( strlen( value ) > sizebuff )
		Con_DPrintf( "PR2_infokey: buffer size too small\n" );
	strlcpy( valbuff, value, sizebuff );
//      RETURN_STRING(value);
}

/*
==============
PF2_multicast

void(vector where, float set) multicast
==============
*/
void PF2_multicast( float x, float y, float z, int to)
//(vec3_t o, int to)
{
	vec3_t  o;

	o[0] = x;
	o[1] = y;
	o[2] = z;
	SV_Multicast( o, to );
}

/*
==============
PF2_disable_updates

void(entiny whom, float time) disable_updates
==============
*/
void PF2_disable_updates( int entnum, float time )
//(int entnum, float time)
{
	client_t *client;

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to disable_updates to a non-client\n" );
		return;
	}

	client = &svs.clients[entnum - 1];

	client->disable_updates_stop = svs.realtime + time;
}


#define MAX_PR2_FILES 8

typedef struct {
	char    name[256];
	FILE   *handle;
	fsMode_t accessmode;
} pr2_fopen_files_t;

pr2_fopen_files_t pr2_fopen_files[MAX_PR2_FILES];
int     pr2_num_open_files = 0;

char   *cmodes[] = { "rb", "r", "wb", "w", "ab", "a" };

/*
int	trap_FS_OpenFile(char*name, fileHandle_t* handle, fsMode_t fmode );
*/
//FIX ME read from paks
int PF2_FS_OpenFile(char* name, fileHandle_t* handle, fsMode_t fmode)
{
	int     i;
	char    fname[MAX_OSPATH];
	char   *gpath = NULL;
	char   *gamedir;
    int fsize;

	if ( pr2_num_open_files >= MAX_PR2_FILES )
	{
		return -1;
	}

	*handle = 0;
	for ( i = 0; i < MAX_PR2_FILES; i++ )
		if ( !pr2_fopen_files[i].handle )
			break;
	if ( i == MAX_PR2_FILES )	//too many already open
	{
		return -1;
	}

	if ( name[1] == ':' ||	//dos filename absolute path specified - reject.
	     *name == '\\' || *name == '/' ||	//absolute path was given - reject
	     strstr( name, ".." ) )	//someone tried to be cleaver.
	{
		return -1;
	}
	strlcpy( pr2_fopen_files[i].name, name, sizeof( pr2_fopen_files[i].name ) );
	pr2_fopen_files[i].accessmode = fmode;
	switch ( fmode )
	{
	case FS_READ_BIN:
	case FS_READ_TXT:

		while ( ( gpath = COM_NextPath( gpath ) ) )
		{
			Q_snprintfz( fname, sizeof( fname ), "%s/%s", gpath, name );
			pr2_fopen_files[i].handle = fopen( fname, cmodes[fmode] );
			if ( pr2_fopen_files[i].handle )
			{

				Con_DPrintf( "PF2_FS_OpenFile %s\n", fname );
				break;
			}
		}

		if ( !pr2_fopen_files[i].handle )
		{
			return -1;
		}
		fseek( pr2_fopen_files[i].handle, 0, SEEK_END );
		fsize = ftell( pr2_fopen_files[i].handle );
		fseek( pr2_fopen_files[i].handle, 0, 0 );

		break;
	case FS_WRITE_BIN:
	case FS_WRITE_TXT:
	case FS_APPEND_BIN:
	case FS_APPEND_TXT:

		gamedir = Info_ValueForKey( svs.info, "*gamedir" );
		if ( !gamedir[0] )
			gamedir = "qw";

		Q_snprintfz( fname, sizeof( fname ), "%s/%s", gamedir, name );
		pr2_fopen_files[i].handle = fopen( fname, cmodes[fmode] );
		if ( !pr2_fopen_files[i].handle )
		{
			return -1;
		}
		Con_DPrintf( "PF2_FS_OpenFile %s\n", fname );
		fsize = ftell( pr2_fopen_files[i].handle );
		break;
	default:
		return -1;

	}

	*handle = i + 1;
	pr2_num_open_files++;
    return fsize;
}

/*
void	trap_FS_CloseFile( fileHandle_t handle );
*/
void PF2_FS_CloseFile(fileHandle_t fnum)
{
	fnum--;
	if ( fnum < 0 || fnum >= MAX_PR2_FILES )
		return;		//out of range
	if ( !pr2_num_open_files )
		return;
	if ( !( pr2_fopen_files[fnum].handle ) )
		return;
	fclose( pr2_fopen_files[fnum].handle );
	pr2_fopen_files[fnum].handle = NULL;
	pr2_num_open_files--;
}

int     seek_origin[] = { SEEK_CUR, SEEK_END, SEEK_SET };

/*
int	trap_FS_SeekFile( fileHandle_t handle, int offset, int type );
*/

int PF2_FS_SeekFile(fileHandle_t fnum, int offset, fsOrigin_t type)
{
	fnum--;

	if ( fnum < 0 || fnum >= MAX_PR2_FILES )
		return -1;		//out of range

	if ( !pr2_num_open_files )
		return -1;

	if ( !( pr2_fopen_files[fnum].handle ) )
		return -1;
	if ( type < 0 || type > 2 )
		return -1;
	return fseek( pr2_fopen_files[fnum].handle, offset, seek_origin[type] );
}

/*
int	trap_FS_TellFile( fileHandle_t handle );
*/

int PF2_FS_TellFile(fileHandle_t fnum)
{
	fnum--;

	if ( fnum < 0 || fnum >= MAX_PR2_FILES )
		return -1;		//out of range

	if ( !pr2_num_open_files )
		return -1;

	if ( !( pr2_fopen_files[fnum].handle ) )
		return -1;
	return ftell( pr2_fopen_files[fnum].handle );
}

/*
int	trap_FS_WriteFile( char*src, int quantity, fileHandle_t handle );
*/
int PF2_FS_WriteFile(char*dest, int quantity, fileHandle_t fnum)
{
	fnum--;
	if ( fnum < 0 || fnum >= MAX_PR2_FILES )
		return 0;		//out of range

	if ( !pr2_num_open_files )
		return 0;

	if ( !( pr2_fopen_files[fnum].handle ) )
		return 0;

	return fwrite( dest, quantity, 1, pr2_fopen_files[fnum].handle );

}

/*
int	trap_FS_ReadFile( char*dest, int quantity, fileHandle_t handle );
*/
int PF2_FS_ReadFile(char*dest, int quantity, fileHandle_t fnum)
{

	fnum--;
	if ( fnum < 0 || fnum >= MAX_PR2_FILES )
		return 0;		//out of range

	if ( !pr2_num_open_files )
		return 0;

	if ( !( pr2_fopen_files[fnum].handle ) )
		return 0;

	return fread( dest, quantity, 1, pr2_fopen_files[fnum].handle );
}

void PR2_FS_Restart(  )
{
	int     i;

	if ( pr2_num_open_files )
	{
		for ( i = 0; i <= MAX_PR2_FILES; i++ )
		{
			if ( pr2_fopen_files[i].handle )
			{
				fclose( pr2_fopen_files[i].handle );
				pr2_num_open_files--;
				pr2_fopen_files[i].handle = NULL;
			}
		}
	}
	if ( pr2_num_open_files )
		Sys_Error( "PR2_fcloseall: pr2_num_open_files != 0\n" );
	pr2_num_open_files = 0;
	memset( pr2_fopen_files, 0, sizeof( pr2_fopen_files ) );
}

/*
int 	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
*/
int PF2_FS_GetFileList(char* path, char*ext, char* listbuff, int buffsize)
{
	char    fname[MAX_OSPATH];
	char    *dirptr;
	char   *gamedir;

#ifdef _WIN32
	HANDLE  h;
	WIN32_FIND_DATA fd;
#else
	DIR    *d;
	struct dirent *dstruct;
	struct stat fileinfo;
#endif
	int     numfiles = 0;


	dirptr = listbuff;
	*dirptr = 0;
	gamedir = Info_ValueForKey( svs.info, "*gamedir" );
	if ( !gamedir[0] )
		gamedir = "qw";

	Q_snprintfz( fname, sizeof( fname ), "%s/%s/*%s", gamedir, path, ext );

#ifdef _WIN32
	h = FindFirstFile( fname, &fd );
	if ( h == INVALID_HANDLE_VALUE )
	{
#else
	if ( !( d = opendir( fname ) ) )
	{
#endif
		return 0;
	}
#ifndef _WIN32
	dstruct = readdir( d );
#endif
	do
	{
		int     size;
		int     namelen;
		char   *filename;
		qboolean is_dir;

#ifdef _WIN32
		filename = fd.cFileName;
		is_dir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		size = fd.nFileSizeLow;
#else
		filename = dstruct->d_name;
		Q_snprintfz( fname, sizeof( fname ), "%s/%s/%s", gamedir, path, filename );
		stat( fname, &fileinfo );
		is_dir = S_ISDIR( fileinfo.st_mode );
		size = fileinfo.st_size;
#endif
		if ( is_dir )
			continue;

		namelen = strlen( filename ) + 1;
		if ( dirptr + namelen > listbuff + buffsize )
			break;
		strlcpy( dirptr, filename, namelen );
		dirptr += namelen;
		numfiles++;
#ifdef _WIN32
	}
	while ( FindNextFile( h, &fd ) );
	FindClose( h );
#else
	}
	while ( ( dstruct = readdir( d ) ) );
	closedir( d );
#endif
	return numfiles;
}

/*
  int trap_Map_Extension( const char* ext_name, int mapto)
  return:
    0 	success maping
   -1	not found
   -2	cannot map
*/
int PF2_Map_Extension(char* name, int mapto)
{
	if ( mapto < _G__LASTAPI )
	{

		return -2;
	}

	return -1;
}

/////////Bot Functions
extern cvar_t maxclients, maxspectators;
int PF2_Add_Bot(char *name, int bottomcolor, int topcolor, char*skin)
{
	client_t *cl, *newcl = NULL;
	int     edictnum;
	int     clients, spectators, i;
	extern char *shortinfotbl[];
	char   *s;
	edict_t *ent;
	eval_t *val;
	string_t savenetname;
	int old_self;

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	for ( i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++ )
	{
		if ( cl->state == cs_free )
			continue;
		if ( cl->spectator )
			spectators++;
		else
			clients++;
	}

	// if at server limits, refuse connection
	if ( maxclients.value > MAX_CLIENTS )
		Cvar_SetValue( &maxclients, MAX_CLIENTS );
	if ( maxspectators.value > MAX_CLIENTS )
		Cvar_SetValue( &maxspectators, MAX_CLIENTS );
	if ( maxspectators.value + maxclients.value > MAX_CLIENTS )
		Cvar_SetValue( &maxspectators, MAX_CLIENTS - maxspectators.value + maxclients.value );

	if ( clients >= ( int ) maxclients.value )
	{
		return 0;
	}
	for ( i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++ )
	{
		if ( cl->state == cs_free )
		{
			newcl = cl;
			break;
		}
	}
	if ( !newcl )
	{
		return 0;
	}
	Q_snprintfz( newcl->userinfo, sizeof( newcl->userinfo ),
			"\\name\\%s\\topcolor\\%d\\bottomcolor\\%d\\emodel\\6967\\pmodel\\13845\\skin\\%s\\*bot\\1",
			name, topcolor, bottomcolor, skin );

	newcl->state = cs_spawned;
	newcl->userid = SV_GenerateUserID();
	newcl->datagram.allowoverflow = true;
	newcl->datagram.data = newcl->datagram_buf;
	newcl->datagram.maxsize = sizeof( newcl->datagram_buf );
	newcl->spectator = 0;
	newcl->isBot = 1;


	edictnum = ( newcl - svs.clients ) + 1;
	ent = EDICT_NUM( edictnum );
	savenetname = ent->v.netname;

	memset( &ent->v, 0, pr_edict_size - sizeof( edict_t ) + sizeof( entvars_t ) );
	ent->v.netname = savenetname;
	newcl->entgravity = 1.0;
	val = PR2_GetEdictFieldValue( ent, "gravity" );
	if ( val )
		val->_float = 1.0;
	host_client->maxspeed = sv_maxspeed.value;
	val = PR2_GetEdictFieldValue( ent, "maxspeed" );
	if ( val )
		val->_float = sv_maxspeed.value;


	newcl->edict = ent;
	ent->v.colormap = edictnum;
	val = PR2_GetEdictFieldValue( ent, "isBot" );
	if( val )
	       val->_int = 1;

	newcl->name = PR2_GetString( ent->v.netname );
	memset( newcl->stats, 0, sizeof( host_client->stats ) );
	SZ_Clear( &newcl->netchan.message );
	newcl->netchan.drop_count = 0;
	newcl->netchan.incoming_sequence = 1;

	SV_ExtractFromUserinfo( newcl, true );

	for ( i = 0; shortinfotbl[i] != NULL; i++ )
	{
		s = Info_ValueForKey( newcl->userinfo, shortinfotbl[i] );
		Info_SetValueForStarKey( newcl->userinfoshort, shortinfotbl[i], s, MAX_INFO_STRING );
	}
	// move star keys to infoshort
	Info_CopyStarKeys( newcl->userinfo, newcl->userinfoshort );

	newcl->disable_updates_stop = -1.0;	// Vladis


	SV_FullClientUpdate( newcl, &sv.reliable_datagram );


	old_self = pr_global_struct->self;
	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(newcl->edict);

	PR2_GameClientConnect(0);
	PR2_GamePutClientInServer(0);

	pr_global_struct->self = old_self;
    return edictnum;
}

void RemoveBot(client_t *cl)
{

        if( !cl->isBot )
                return;

        pr_global_struct->self = EDICT_TO_PROG(cl->edict);
       	if ( sv_vm )
       		PR2_GameClientDisconnect(0);

	cl->old_frags = 0;
	cl->edict->v.frags = 0;
	cl->name[0] = 0;
	cl->state = cs_free;
	memset( cl->userinfo, 0, sizeof( cl->userinfo ) );
	memset( cl->userinfoshort, 0, sizeof( cl->userinfoshort ) );
	SV_FullClientUpdate( cl, &sv.reliable_datagram );
	cl->isBot = 0;


}

void PF2_Remove_Bot(int entnum)
{
	client_t *cl;
    int old_self;
	

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to remove a non-botclient %d \n", entnum );
		return;
	}
	cl = &svs.clients[entnum - 1];
	if ( !cl->isBot )
	{
		Con_Printf( "tried to remove a non-botclient %d \n", entnum );
		return;
	}
	old_self = pr_global_struct->self; //save self

	pr_global_struct->self = entnum;
	RemoveBot(cl);
	pr_global_struct->self = old_self;

}

void PF2_SetBotUserInfo(int entnum, char*key, char*value)
{
	client_t *cl;
	int     i;
	extern char *shortinfotbl[];

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to change userinfo a non-botclient %d \n", entnum );
		return;
	}
	cl = &svs.clients[entnum - 1];
	if ( !cl->isBot )
	{
		Con_Printf( "tried to change userinfo a non-botclient %d \n", entnum );
		return;
	}
	Info_SetValueForKey( cl->userinfo, key, value, MAX_INFO_STRING );
	SV_ExtractFromUserinfo( cl, !strcmp( key, "name" ) );

	for ( i = 0; shortinfotbl[i] != NULL; i++ )
		if ( key[0] == '_' || !strcmp( key, shortinfotbl[i] ) )
		{
			char   *new = Info_ValueForKey( cl->userinfo, key );

			Info_SetValueForKey( cl->userinfoshort, key, new, MAX_INFO_STRING );

			i = cl - svs.clients;
			MSG_WriteByte( &sv.reliable_datagram, svc_setinfo );
			MSG_WriteByte( &sv.reliable_datagram, i );
			MSG_WriteString( &sv.reliable_datagram, key );
			MSG_WriteString( &sv.reliable_datagram, new );
			break;
		}
}

void PF2_SetBotCMD(int entnum, int msec, float a1,float a2, float a3, int forwardmove, int sidemove, int upmove, int buttons, int impulse)
{
	client_t *cl;

	if ( entnum < 1 || entnum > MAX_CLIENTS )
	{
		Con_Printf( "tried to set cmd a non-botclient %d \n", entnum );
		return;
	}
	cl = &svs.clients[entnum - 1];
	if ( !cl->isBot )
	{
		Con_Printf( "tried to set cmd a non-botclient %d \n", entnum );
		return;
	}
	cl->botcmd.msec = msec;
	cl->botcmd.angles[0] = a1;
	cl->botcmd.angles[1] = a2;
	cl->botcmd.angles[2] = a3;
	cl->botcmd.forwardmove = forwardmove;
	cl->botcmd.sidemove = sidemove;
	cl->botcmd.upmove = upmove;
	cl->botcmd.buttons = buttons;
	cl->botcmd.impulse = impulse;
	if ( cl->edict->v.fixangle)
	{
	     VectorCopy(cl->edict->v.angles, cl->botcmd.angles);
	     cl->botcmd.angles[PITCH] *= -3;
	     cl->edict->v.fixangle = 0;
	}
}
//=========================================
// some time support in QVM
//=========================================

/*
==============
PF2_QVMstrftime
==============
*/
int PF2_QVMstrftime(char *valbuff, int sizebuff, char *fmt, int offset)
{
	struct tm *newtime;
	time_t long_time;
    int ret;

	if (sizebuff <= 0 || !valbuff) {
		Con_DPrintf("PF2_QVMstrftime: wrong buffer\n");
		return 0;
	}

	time(&long_time);
	long_time += offset;
	newtime = localtime(&long_time);

	if (!newtime)
	{
		valbuff[0] = 0; // or may be better set to "#bad date#" ?
		return 0;
	}

	ret = strftime(valbuff, sizebuff-1, fmt, newtime);

	if (!ret) {
		valbuff[0] = 0; // or may be better set to "#bad date#" ?
		Con_DPrintf("PF2_QVMstrftime: buffer size too small\n");
		return 0;
	}
    return ret;
}

//===========================================================================
// SysCalls
//===========================================================================


#define	VMV(x)	args[x], args[x+1], args[x+2]
#define	VME(x)	EDICT_NUM(args[x])
intptr_t PR2_GameSystemCalls( intptr_t *args ) {
	switch( args[0] ) {
        case G_GETAPIVERSION:
            return GAME_API_VERSION;
        case G_DPRINT:
            Con_DPrintf( "%s", (const char*)VMA(1) );
            return 0;
        case G_ERROR:
			PR2_RunError( VMA(1) );
            return 0;
        case G_GetEntityToken:
			pr2_ent_data_ptr = COM_Parse( pr2_ent_data_ptr );
			strlcpy( VMA(1), com_token, args[2]);
            return pr2_ent_data_ptr != NULL;
        case G_SPAWN_ENT:
            return NUM_FOR_EDICT( ED2_Alloc(  ) );
        case G_REMOVE_ENT:
			ED2_Free( EDICT_NUM( args[1] ) );
            return 0;
        case G_PRECACHE_SOUND:
			PF2_precache_sound(VMA(1));
            return 0;
        case G_PRECACHE_MODEL:
			PF2_precache_model(VMA(1));
            return 0;
        case G_LIGHTSTYLE:
            PF2_lightstyle( args[1], VMA(2) );
            return 0;
        case G_SETORIGIN:
            PF2_setorigin( EDICT_NUM(args[1]), VMV(2));
            return 0;
        case G_SETSIZE:
            PF2_setsize( EDICT_NUM(args[1]), VMV(2), VMV(5));
            return 0;
        case G_SETMODEL:
            PF2_setmodel( EDICT_NUM(args[1]), VMA(2));
            return 0;
        case G_BPRINT:
            SV_BroadcastPrintf( args[1], "%s", VMA(2) );
            return 0;
        case G_SPRINT:
            PF2_sprint( args[1], args[2], VMA(3));
            return 0;
        case G_CENTERPRINT:
            PF2_centerprint( args[1], VMA(2));
            return 0;
        case G_AMBIENTSOUND:
            PF2_ambientsound( VMV(1), VMA(4), VMF(5), VMF(6));
            return 0;
        case G_SOUND:
            SV_StartSound( EDICT_NUM( args[1]), args[2], VMA(3), VMF(4), VMF(4) );
            return 0;
        case G_TRACELINE:
            PF2_traceline( VMV(1), VMV(4), args[7], EDICT_NUM( args[8] ) );
            return 0;
        case G_CHECKCLIENT:
            return PF2_checkclient();
        case G_STUFFCMD:
            PF2_stuffcmd( args[1], VMA(2));
            return 0;
        case G_LOCALCMD:
            Cbuf_AddText( VMA(1) );
            return 0;
        case G_CVAR:
            PASSFLOAT( Cvar_VariableValue( VMA(1) ));
            return 0;
        case G_CVAR_SET:
            Cvar_SetByName( VMA(1), VMA(2) );
            return 0;
        case G_FINDRADIUS:
            return PF2_FindRadius( NUM_FOR_EDICT(VMA(1)), (float*)VMA(2), VMF(3));
        case G_WALKMOVE:
            return PF2_walkmove( VME(1), VMF(2), VMF(3));
        case G_DROPTOFLOOR:
            PF2_droptofloor( VME(1));
            return 0;
        case G_CHECKBOTTOM:
            return  SV_CheckBottom( VME(1));
        case G_POINTCONTENTS:
            return PF2_pointcontents( VMV(1) );
        case G_NEXTENT:
            return PF2_nextent( args[1] );
        case G_AIM:
            return 0;
        case G_MAKESTATIC:
            PF2_makestatic( VME(1) );
            return 0;
        case G_SETSPAWNPARAMS:
            PF2_setspawnparms( args[1] );
            return 0;
        case G_CHANGELEVEL:
            PF2_changelevel( VMA(1) );
            return 0;
        case G_LOGFRAG:
            PF2_logfrag( args[1], args[2] );
            return 0;
        case G_GETINFOKEY:
            PF2_infokey( args[1], VMA(2), VMA(3), args[4] );
            return 0;
        case G_MULTICAST:
            PF2_multicast( VMV(1), args[4] );
            return 0;
        case G_DISABLEUPDATES:
            PF2_disable_updates( args[1], VMF(1) );
            return 0;
        case G_WRITEBYTE:
            PF2_WriteByte( args[1], args[2] );
            return 0;
        case G_WRITECHAR:
            PF2_WriteChar( args[1], args[2] );
            return 0;
        case G_WRITESHORT:
            PF2_WriteShort( args[1], args[2] );
            return 0;
        case G_WRITELONG:
            PF2_WriteLong( args[1], args[2] );
            return 0;
        case G_WRITEANGLE:
            PF2_WriteAngle( args[1], VMF(2) );
            return 0;
        case G_WRITECOORD:
            PF2_WriteCoord( args[1], VMF(2) );
            return 0;
        case G_WRITESTRING:
            PF2_WriteString( args[1], VMA(2) );
            return 0;
        case G_WRITEENTITY:
            PF2_WriteEntity( args[1], args[2] );
            return 0;
        case G_FLUSHSIGNON:
            SV_FlushSignon(  );
            return 0;
        case g_memset:
            return PR2_SetString( memset( VMA(1), args[2], args[3]));
        case g_memcpy:
            return PR2_SetString( memcpy( VMA(1), VMA(2), args[3]));
        case g_strncpy:
            return PR2_SetString( strncpy( VMA(1), VMA(2), args[3]));
        case g_sin:
            return PASSFLOAT( sin( VMF(1)));
        case g_cos:
            return PASSFLOAT( cos( VMF(1)));
        case g_atan2:
            return PASSFLOAT( atan2( VMF(1), VMF(2)));
        case g_sqrt:
            return PASSFLOAT( sqrt( VMF(1)));
        case g_floor:
            return PASSFLOAT( floor( VMF(1)));
        case g_ceil:
            return PASSFLOAT( ceil( VMF(1)));
        case g_acos:
            return PASSFLOAT( acos( VMF(1)));
        case G_CMD_ARGC:
            return Cmd_Argc();
        case G_CMD_ARGV:
            strlcpy( VMA(1), Cmd_Argv( args[2]), args[3]);
            return 0;
        case G_TraceCapsule:
            PF2_TraceCapsule( VMV(1), VMV(4), args[7], EDICT_NUM( args[8] ), VMV(9), VMV(12) );
            return 0;
        case G_FSOpenFile:
            return PF2_FS_OpenFile(VMA(1), (fileHandle_t*) VMA(2), (fsMode_t)args[3]);
        case G_FSCloseFile:
            PF2_FS_CloseFile( (fileHandle_t) args[1]);
            return 0;
        case G_FSReadFile:
            return PF2_FS_ReadFile( VMA(1), args[2], (fileHandle_t)args[3]);
        case G_FSWriteFile:
            return PF2_FS_WriteFile( VMA(1), args[2], (fileHandle_t)args[3]);
        case G_FSSeekFile:
            return PF2_FS_SeekFile((fileHandle_t) args[1], args[2], (fsOrigin_t) args[3]);
        case G_FSTellFile:
            return PF2_FS_TellFile( (fileHandle_t) args[1]);
        case G_FSGetFileList:
            return PF2_FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
        case G_CVAR_SET_FLOAT:
            Cvar_SetValueByName( VMA(1), VMF(2) );
            return 0;
        case G_CVAR_STRING:
            strlcpy( VMA(1), Cvar_VariableString( VMA(3) ), args[2] );
            return 0;
        case G_Map_Extension:
            return PF2_Map_Extension(VMA(1), args[2]);
        case G_strcmp:
            return strcmp( VMA(1), VMA(2));
        case G_strncmp:
            return strncmp( VMA(1), VMA(2), args[3]);
        case G_stricmp:
            return Q_stricmp( VMA(1), VMA(2));
        case G_strnicmp:
            return Q_strnicmp( VMA(1), VMA(2), args[3]);
        case G_Find:
            return PF2_Find( (edict_t*)VMA(1), args[2], VMA(3));
        case G_executecmd:
            PF2_executecmd();
            return 0;
        case G_conprint:
            Sys_Printf( "%s", VMA(1) );
            return 0;
        case G_readcmd:
            PF2_readcmd( VMA(1), VMA(2), args[3]);
            return 0;
        case G_redirectcmd:
            PF2_redirectcmd( NUM_FOR_EDICT( VMA(1)), VMA(2));
            return 0;
        case G_Add_Bot:
            return PF2_Add_Bot( VMA(1), args[2], args[3], VMA(4));
        case G_Remove_Bot:
            PF2_Remove_Bot( args[1]);
            return 0;
        case G_SetBotUserInfo:
            PF2_SetBotUserInfo( args[1], VMA(2), VMA(3));
            return 0;
        case G_SetBotCMD:
            PF2_SetBotCMD( args[1], args[2], VMV(3), args[6], args[7], args[8], args[9], args[10]); 
            return 0;
        case G_QVMstrftime:
            return PF2_QVMstrftime( VMA(1), args[2], VMA(3), args[4]);
        case G_CMD_ARGS:
            strlcpy(VMA(1), Cmd_Args(), args[2]);
            return 0;
        case G_CMD_TOKENIZE:
            Cmd_TokenizeString(VMA(1));
            return 0;
        case g_strlcpy:
            return strlcpy( VMA(1), VMA(2), args[3]);
        case g_strlcat:
            return strlcat( VMA(1), VMA(2), args[3]);
        case G_MAKEVECTORS:
            AngleVectors (VMA(1), pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
            return 0;
        case G_NEXTCLIENT:
            PF2_nextclient( NUM_FOR_EDICT( VMA(1)));
            return 0;
        default:
            SV_Error( "Bad game system trap: %ld", (long int) args[0] );
    }
    return 0;
}
/*int sv_syscall( int arg, ... )	//must passed ints dll
{
	va_list ap;
	pr2val_t ret;

	if ( arg >= pr2_numAPI )
		PR2_RunError( "sv_syscall: Bad API call number" );

	va_start( ap, arg );

	pr2_API[arg] ( 0, ~0, ( pr2val_t * ) ap, &ret );

	return ret._int;
}

int sv_sys_callex( byte * data, unsigned int mask, int fn, pr2val_t * arg )//vm
{
	pr2val_t ret;

	if ( fn >= pr2_numAPI )
		PR2_RunError( "sv_sys_callex: Bad API call number" );

	pr2_API[fn] ( data, mask, arg, &ret );
	return ret._int;
}*/
/*
====================
SV_DllSyscall
====================
*/
/*static intptr_t QDECL SV_DllSyscall( intptr_t arg, ... ) {
#if !id386 || defined __clang__
	intptr_t	args[14]; // max.count for qagame
	va_list	ap;
	int i;

	args[0] = arg;
	va_start( ap, arg );
	for (i = 1; i < ARRAY_LEN( args ); i++ )
		args[ i ] = va_arg( ap, intptr_t );
	va_end( ap );

	return SV_GameSystemCalls( args );
#else
	return SV_GameSystemCalls( &arg );
#endif
}*/


#endif				/* USE_PR2 */
