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

#include "quakedef.h"

cvar_t		baseskin = {"baseskin", "base"};
cvar_t		noskins = {"noskins", "0"};

char		allskins[128];
#define		MAX_CACHED_SKINS		128
skin_t		skins[MAX_CACHED_SKINS];
int			numskins;

/*
================
Skin_Find

  Determines the best skin for the given scoreboard
  slot, and sets scoreboard->skin

================
*/
void Skin_Find (player_info_t *sc)
{
	skin_t		*skin;
	int			i;
	char		name[128], *s;

	if (allskins[0])
		strcpy (name, allskins);
	else
	{
		s = Info_ValueForKey (sc->userinfo, "skin");
		if (s && s[0])
			strcpy (name, s);
		else
			strcpy (name, baseskin.string);
	}

	if (strstr (name, "..") || *name == '.')
		strcpy (name, "base");

	COM_StripExtension (name, name);

//	Con_Printf("Skin_Find: %s\n",name);

	for (i=0 ; i<numskins ; i++)
	{
		if (!strcmp (name, skins[i].name))
		{
			sc->skin = &skins[i];
			Skin_Cache (sc->skin);
			return;
		}
	}

	if (numskins == MAX_CACHED_SKINS)
	{	// ran out of spots, so flush everything
		Con_Printf ("Warning: numskins == MAX_CACHED_SKINS\n");
		Skin_Skins_f ();
		return;
	}

	skin = &skins[numskins];
	sc->skin = skin;
	numskins++;

	memset (skin, 0, sizeof(*skin));
	strncpy(skin->name, name, sizeof(skin->name) - 1);
}


/*
==========
Skin_Cache

Returns a pointer to the skin bitmap, or NULL to use the default
==========
*/
#ifdef QW262
#include "skin262.inc"
#endif


//extern char SAY_BADSKIN_AND_DISCONNECT[];
byte *Skin_Cache (skin_t *skin)
{
	char	Name[1024];
	byte	*raw;
	byte	*out, *pix;
	pcx_t	*pcx;
	int		x, y;
	int		dataByte;
	int		runLength;

	char   *SkinName=skin->name;

	if (cls.downloadtype == dl_skin)
		return NULL;		// use base until downloaded

	if (noskins.value==1) // JACK: So NOSKINS > 1 will show skins, but
		return NULL;	  // not download new ones.

	if (skin->failedload)
		return NULL;

	out = Cache_Check (&skin->cache);
	if (out)
		return out;

//
// load the pic from disk
//
	sprintf (Name, "skins/%s.pcx", skin->name);
	raw = COM_LoadTempFile (Name);
	if (!raw)
	{
		Con_Printf ("Couldn't load skin %s\n", Name);
		SkinName=baseskin.string;
		sprintf (Name, "skins/%s.pcx", baseskin.string);
		raw = COM_LoadTempFile (Name);
		if (!raw)
		{
			skin->failedload = true;
			return NULL;
		}
	} 

#ifdef QW262
	{
		int j;
		unsigned long ck,*cur;
		Skin_Cache262();
	}
#endif

//
// parse the PCX file
//
	pcx = (pcx_t *)raw;
#ifdef __APPLE__
	pcx->xmin = LittleShort (pcx->xmin);
	pcx->ymin = LittleShort (pcx->ymin);
	pcx->xmax = LittleShort (pcx->xmax);
	pcx->ymax = LittleShort (pcx->ymax);
	pcx->hres = LittleShort (pcx->hres);
	pcx->vres = LittleShort (pcx->vres);
	pcx->bytes_per_line = LittleShort (pcx->bytes_per_line);
	pcx->palette_type = LittleShort (pcx->palette_type);
#endif
	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 320
		|| pcx->ymax >= 200)
	{
		skin->failedload = true;
		Con_Printf ("Bad skin %s\n", Name);
		return NULL;
	}
	
	out = Cache_Alloc (&skin->cache, 320*200, skin->name);
	if (!out)
		Sys_Error ("Skin_Cache: couldn't allocate");

	pix = out;
	memset (out, 0, 320*200);

	for (y=0 ; y<pcx->ymax ; y++, pix += 320)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			if (raw - (byte*)pcx > com_filesize) 
			{
				Cache_Free (&skin->cache);
				skin->failedload = true;
				Con_Printf ("Skin %s was malformed.  You should delete it.\n", Name);
				return NULL;
			}
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (raw - (byte*)pcx > com_filesize) 
				{
					Cache_Free (&skin->cache);
					skin->failedload = true;
					Con_Printf ("Skin %s was malformed.  You should delete it.\n", Name);
					return NULL;
				}
				dataByte = *raw++;
			}
			else
				runLength = 1;

			// skin sanity check
			if (runLength + x > pcx->xmax + 2) {
				Cache_Free (&skin->cache);
				skin->failedload = true;
				Con_Printf ("Skin %s was malformed.  You should delete it.\n", Name);
				return NULL;
			}
			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > com_filesize)
	{
		Cache_Free (&skin->cache);
		skin->failedload = true;
		Con_Printf ("Skin %s was malformed.  You should delete it.\n", Name);
		return NULL;
	}

	skin->failedload = false;

	return out;
}


/*
=================
Skin_NextDownload
=================
*/


void Skin_NextDownload (void)
{
	player_info_t	*sc;
	int			i;

	if (cls.downloadnumber == 0)
		Con_Printf ("Checking skins...\n");
	cls.downloadtype = dl_skin;

	for ( ; cls.downloadnumber != MAX_CLIENTS ; cls.downloadnumber++) {
		sc = &cl.players[cls.downloadnumber];
		if (!sc->name[0] /*|| sc->spectator*/)
			continue;
		Skin_Find (sc);
		if (noskins.value)
			continue;
		if ( sc->skin->name && sc->skin->name[0] && !strchr(sc->skin->name, ' ')) {
			if (!CL_CheckOrDownloadFile(va("skins/%s.pcx", sc->skin->name)))
				return;		// started a download
		} else {
			Con_Printf ("BAD SKIN!: %s <%s>\n", sc->name, sc->skin->name);
		}
	}

	cls.downloadtype = dl_none;

	// now load them in for real
	for (i=0 ; i<MAX_CLIENTS ; i++) {
		sc = &cl.players[i];
		if (!sc->name[0])
			continue;
		Skin_Cache (sc->skin);
#ifdef GLQUAKE
		sc->skin = NULL;
#endif

	}

// Tonik: only download when connecting
	if (cls.state == ca_onserver) {	// get next signon phase
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("begin %i", cl.servercount));
		Cache_Report ();		// print remaining memory
	}
}


/*
==========
Skin_Skins_f

Refind all skins, downloading if needed.
==========
*/
void Skin_Skins_f (void)
{
	int		i;

#ifdef GLQUAKE
	// BorisU: force slots update
	memset (skinslots, 0, sizeof(skinslots));
	for (i=0; i<MAX_CLIENTS; i++) // is it required?
		skinslots[i].mip = -1;
#endif

	for (i=0 ; i<numskins ; i++)
	{
		if (skins[i].cache.data)
			Cache_Free (&skins[i].cache);
	}
	numskins = 0;

	cls.downloadnumber = 0;
	cls.downloadtype = dl_skin;
	Skin_NextDownload ();
}


/*
==========
Skin_AllSkins_f

Sets all skins to one specific one
==========
*/
void Skin_AllSkins_f (void)
{
	strlcpy (allskins, Cmd_Argv(1), sizeof(allskins));
	Skin_Skins_f ();
}

// BorisU -->
#ifdef GLQUAKE
// Better skin handling for GL
// minimizing skin upload to video card during gameplay

skintexinfo_t skinslots[MAX_CLIENTS];

//void Skin_ShowSlots_f (void)
//{
//	skintexinfo_t	*slot;
//	player_info_t	*player;
//	int				use_count[MAX_CLIENTS];
//	
//	Con_Printf ("Skin Slots\n");
//	memset (use_count, 0, sizeof(use_count));
//	for (player = cl.players; player < cl.players + MAX_CLIENTS; player ++) {
//		if (player->name[0] && !player->spectator)
//			use_count[player->skintexslot]++;
//	}
//	
//	for (slot = skinslots; slot < skinslots + MAX_CLIENTS; slot++) {
//		Con_Printf("%i {%s} (%i,%i) used: %i\n",slot-skinslots, slot->skinname, slot->topcolor, slot->bottomcolor, use_count[slot-skinslots]);
//	}
//}

void Skin_SlotsInit()
{
	int i;
	
	for (i=0; i<MAX_CLIENTS; i++)
		skinslots[i].mip = -1;
//	Cmd_AddCommand ("showslots", Skin_ShowSlots_f);
}

int Skin_FindSlot(char* name, int topcolor, int bottomcolor)
{
	skintexinfo_t	*slot;
//	Con_Printf("Skin_FindSlot: %s (%i,%i)\n",name, topcolor, bottomcolor);
	for (slot = skinslots; slot < skinslots + MAX_CLIENTS; slot++) {
		if (slot->topcolor == topcolor &&
			slot->bottomcolor == bottomcolor &&
			!strcmp(slot->skinname, name)) {
//			Con_Printf("Skin_FindSlot: found %i\n", slot - skinslots);
			return slot - skinslots;
		}
	}
//	Con_Printf("Skin_FindSlot: NOT found\n");
	return -1;
}

int Skin_FindFreeSlot()
{
	skintexinfo_t	*slot;
	int				use_count[MAX_CLIENTS];
	player_info_t	*player;

//	Con_Printf("Skin_FindFreeSlot\n");
	
// First searching for slot that was never used
// those that were used can be reused later
	for (slot = skinslots; slot < skinslots + MAX_CLIENTS; slot++) {
		if (!slot->skinname[0])
			return slot - skinslots;
	}

	memset (use_count, 0, sizeof(use_count));

	for (player = cl.players; player < cl.players + MAX_CLIENTS; player ++) {
		if (player->name[0] && !player->spectator)
			use_count[player->skintexslot]++;
	}

// If not found - taking first slot that is currently not used
	for (slot = skinslots; slot < skinslots + MAX_CLIENTS; slot++) {
		if ( !use_count[slot - skinslots] )
			return slot - skinslots;
	}
	// should not happen
	Con_Printf("BUG: Skin_FindFreeSlot failed\n");
	return MAX_CLIENTS - 1; 
}
#endif 
// <-- BorisU
