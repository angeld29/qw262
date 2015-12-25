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
// sbar.c -- status bar code

#include "quakedef.h"

int			sb_updates;		// if >= vid.numpages, no update needed

#define STAT_MINUS		10	// num frame for '-' stats digit
mpic_t		*sb_nums[2][11];
mpic_t		*sb_colon, *sb_slash;
mpic_t		*sb_ibar;
mpic_t		*sb_sbar;
mpic_t		*sb_scorebar;

mpic_t		*sb_weapons[7][8];	// 0 is active, 1 is owned, 2-5 are flashes
mpic_t		*sb_ammo[4];
mpic_t		*sb_sigil[4];
mpic_t		*sb_armor[3];
mpic_t		*sb_items[32];

mpic_t	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
mpic_t	*sb_face_invis;
mpic_t	*sb_face_quad;
mpic_t	*sb_face_invuln;
mpic_t	*sb_face_invis_invuln;

qboolean	sb_showscores;
qboolean	sb_showteamscores;

int			sb_lines;			// scan lines to draw

static qboolean largegame = false;

static int	sbar_xofs;
// fuh -->
cvar_t	scr_centerSbar = {"scr_centerSbar", "0"};

cvar_t	scr_compactHud = {"scr_compactHud", "0"};
cvar_t	scr_compactHudAlign = {"scr_compactHudAlign", "0"};

cvar_t	scr_drawHFrags = {"scr_drawHFrags", "1"};
cvar_t	scr_drawVFrags = {"scr_drawVFrags", "1"};

//cvar_t	scr_scoreboard_forcecolors ={"scr_scoreboard_forcecolors", "1"};
//cvar_t	scr_scoreboard_showfrags =	{"scr_scoreboard_showfrags", "1"};

cvar_t	scr_scoreboard_teamsort =		{"scr_scoreboard_teamsort", "1"};
cvar_t	scr_scoreboard_drawtitle =		{"scr_scoreboard_drawtitle", "1"};
cvar_t	scr_scoreboard_borderless =		{"scr_scoreboard_borderless", "0"};
cvar_t	scr_scoreboard_new =			{"scr_scoreboard_new", "1"};

#ifdef GLQUAKE
cvar_t	scr_scoreboard_fillalpha =		{"scr_scoreboard_fillalpha", "0.7"};
cvar_t	scr_scoreboard_fillcolored =	{"scr_scoreboard_fillcolored", "2"};
#endif
// <-- fuh

/*
===============
Sbar_ShowTeamScores

Tab key down
===============
*/
void Sbar_ShowTeamScores (void)
{
	if (sb_showteamscores)
		return;

	sb_showteamscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowTeamScores

Tab key up
===============
*/
void Sbar_DontShowTeamScores (void)
{
	sb_showteamscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (sb_showscores)
		return;

	sb_showscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	sb_showscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed (void)
{
	sb_updates = 0;	// update next frame
	scr_fullupdate =0;
}

/*
===============
Sbar_Init
===============
*/
void Sbar_Init (void)
{
	int		i;

	for (i=0 ; i<10 ; i++)
	{
		sb_nums[0][i] = Draw_CacheWadPic (va("num_%i",i));
		sb_nums[1][i] = Draw_CacheWadPic (va("anum_%i",i));
	}

	sb_nums[0][10] = Draw_CacheWadPic ("num_minus");
	sb_nums[1][10] = Draw_CacheWadPic ("anum_minus");

	sb_colon = Draw_CacheWadPic ("num_colon");
	sb_slash = Draw_CacheWadPic ("num_slash");

	sb_weapons[0][0] = Draw_CacheWadPic ("inv_shotgun");
	sb_weapons[0][1] = Draw_CacheWadPic ("inv_sshotgun");
	sb_weapons[0][2] = Draw_CacheWadPic ("inv_nailgun");
	sb_weapons[0][3] = Draw_CacheWadPic ("inv_snailgun");
	sb_weapons[0][4] = Draw_CacheWadPic ("inv_rlaunch");
	sb_weapons[0][5] = Draw_CacheWadPic ("inv_srlaunch");
	sb_weapons[0][6] = Draw_CacheWadPic ("inv_lightng");
	
	sb_weapons[1][0] = Draw_CacheWadPic ("inv2_shotgun");
	sb_weapons[1][1] = Draw_CacheWadPic ("inv2_sshotgun");
	sb_weapons[1][2] = Draw_CacheWadPic ("inv2_nailgun");
	sb_weapons[1][3] = Draw_CacheWadPic ("inv2_snailgun");
	sb_weapons[1][4] = Draw_CacheWadPic ("inv2_rlaunch");
	sb_weapons[1][5] = Draw_CacheWadPic ("inv2_srlaunch");
	sb_weapons[1][6] = Draw_CacheWadPic ("inv2_lightng");
	
	for (i=0 ; i<5 ; i++)
	{
		sb_weapons[2+i][0] = Draw_CacheWadPic (va("inva%i_shotgun",i+1));
		sb_weapons[2+i][1] = Draw_CacheWadPic (va("inva%i_sshotgun",i+1));
		sb_weapons[2+i][2] = Draw_CacheWadPic (va("inva%i_nailgun",i+1));
		sb_weapons[2+i][3] = Draw_CacheWadPic (va("inva%i_snailgun",i+1));
		sb_weapons[2+i][4] = Draw_CacheWadPic (va("inva%i_rlaunch",i+1));
		sb_weapons[2+i][5] = Draw_CacheWadPic (va("inva%i_srlaunch",i+1));
		sb_weapons[2+i][6] = Draw_CacheWadPic (va("inva%i_lightng",i+1));
	}

	sb_ammo[0] = Draw_CacheWadPic ("sb_shells");
	sb_ammo[1] = Draw_CacheWadPic ("sb_nails");
	sb_ammo[2] = Draw_CacheWadPic ("sb_rocket");
	sb_ammo[3] = Draw_CacheWadPic ("sb_cells");

	sb_armor[0] = Draw_CacheWadPic ("sb_armor1");
	sb_armor[1] = Draw_CacheWadPic ("sb_armor2");
	sb_armor[2] = Draw_CacheWadPic ("sb_armor3");

	sb_items[0] = Draw_CacheWadPic ("sb_key1");
	sb_items[1] = Draw_CacheWadPic ("sb_key2");
	sb_items[2] = Draw_CacheWadPic ("sb_invis");
	sb_items[3] = Draw_CacheWadPic ("sb_invuln");
	sb_items[4] = Draw_CacheWadPic ("sb_suit");
	sb_items[5] = Draw_CacheWadPic ("sb_quad");

	sb_sigil[0] = Draw_CacheWadPic ("sb_sigil1");
	sb_sigil[1] = Draw_CacheWadPic ("sb_sigil2");
	sb_sigil[2] = Draw_CacheWadPic ("sb_sigil3");
	sb_sigil[3] = Draw_CacheWadPic ("sb_sigil4");

	sb_faces[4][0] = Draw_CacheWadPic ("face1");
	sb_faces[4][1] = Draw_CacheWadPic ("face_p1");
	sb_faces[3][0] = Draw_CacheWadPic ("face2");
	sb_faces[3][1] = Draw_CacheWadPic ("face_p2");
	sb_faces[2][0] = Draw_CacheWadPic ("face3");
	sb_faces[2][1] = Draw_CacheWadPic ("face_p3");
	sb_faces[1][0] = Draw_CacheWadPic ("face4");
	sb_faces[1][1] = Draw_CacheWadPic ("face_p4");
	sb_faces[0][0] = Draw_CacheWadPic ("face5");
	sb_faces[0][1] = Draw_CacheWadPic ("face_p5");

	sb_face_invis = Draw_CacheWadPic ("face_invis");
	sb_face_invuln = Draw_CacheWadPic ("face_invul2");
	sb_face_invis_invuln = Draw_CacheWadPic ("face_inv2");
	sb_face_quad = Draw_CacheWadPic ("face_quad");

// fuh -->
	Cvar_RegisterVariable (&scr_centerSbar);

	Cvar_RegisterVariable (&scr_compactHud);
	Cvar_RegisterVariable (&scr_compactHudAlign);

	Cvar_RegisterVariable (&scr_drawHFrags);
	Cvar_RegisterVariable (&scr_drawVFrags);
	Cvar_RegisterVariable (&scr_scoreboard_teamsort);
//	Cvar_RegisterVariable (&scr_scoreboard_forcecolors);
//	Cvar_RegisterVariable (&scr_scoreboard_showfrags);
	Cvar_RegisterVariable (&scr_scoreboard_drawtitle);
	Cvar_RegisterVariable (&scr_scoreboard_borderless);
	Cvar_RegisterVariable (&scr_scoreboard_new);

#ifdef GLQUAKE
	Cvar_RegisterVariable (&scr_scoreboard_fillalpha);
	Cvar_RegisterVariable (&scr_scoreboard_fillcolored);
#endif
// <-- fuh

	Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

	Cmd_AddCommand ("+showteamscores", Sbar_ShowTeamScores);
	Cmd_AddCommand ("-showteamscores", Sbar_DontShowTeamScores);

	sb_sbar = Draw_CacheWadPic ("sbar");
	sb_ibar = Draw_CacheWadPic ("ibar");
	sb_scorebar = Draw_CacheWadPic ("scorebar");
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic
=============
*/
void Sbar_DrawPic (int x, int y, mpic_t *pic)
{
	Draw_Pic (x + sbar_xofs, y + (vid.conheight-SBAR_HEIGHT), pic);
}

/*
=============
Sbar_DrawSubPic
=============
JACK: Draws a portion of the picture in the status bar.
*/
void Sbar_DrawSubPic(int x, int y, mpic_t *pic, int srcx, int srcy, int width, int height) 
{
	Draw_SubPic (x, y+(vid.conheight-SBAR_HEIGHT), pic, srcx, srcy, width, height);
}

/*
=============
Sbar_DrawTransPic
=============
*/
void Sbar_DrawTransPic (int x, int y, mpic_t *pic)
{
	Draw_TransPic (x + sbar_xofs, y + (vid.conheight-SBAR_HEIGHT), pic);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (x + 4 + sbar_xofs, y + vid.conheight-SBAR_HEIGHT, num);
}

/*
================
Sbar_DrawString
================
*/
void Sbar_DrawString (int x, int y, char *str)
{
	Draw_String (x + sbar_xofs, y+ vid.conheight-SBAR_HEIGHT, str);
}

static void Sbar_DrawAltString (int x, int y, char *str)
{
	Draw_Alt_String (x + sbar_xofs, y + vid.conheight - SBAR_HEIGHT, str);
}

/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int		pow10;
	int		dig;
	
	str = buf;
	
	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}
	
	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;
	
	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);
	
	*str = 0;
	
	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}


//=============================================================================

int		fragsort[MAX_CLIENTS];
int		scoreboardlines;

#define SCR_TEAM_T_MAXTEAMSIZE	(16 + 1)

typedef struct {
	char team[SCR_TEAM_T_MAXTEAMSIZE];
	int frags;
	int players;
	int plow, phigh, ptotal;
	int topcolor, bottomcolor;
	qboolean myteam;
} team_t;

team_t teams[MAX_CLIENTS];

int teamsort[MAX_CLIENTS];
int scoreboardteams;

static inline int	Sbar_ColorForMap (int m)
{
	m = bound(0, m, 13);

	return 16 * m + 8;
}

static inline int Sbar_PlayerNum(void) {
	int mynum;

	if (!cl.spectator || (mynum = Cam_TrackNum()) == -1)
		mynum = cl.playernum;

	return mynum;
}

static inline qboolean Sbar_IsSpectator(int mynum)
{
	return (mynum == cl.playernum) ? cl.spectator : cl.players[mynum].spectator;
}

/*
===============
Sbar_SortFrags
===============
*/
static void Sbar_SortFrags (qboolean includespec)
{
	int		i, j, k;
		
// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<MAX_CLIENTS ; i++) {
		if (cl.players[i].name[0] &&
			(!cl.players[i].spectator || includespec)) {
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
			if (cl.players[i].spectator)
				cl.players[i].frags = -999;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.players[fragsort[j]].frags < cl.players[fragsort[j+1]].frags) {
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}

static void Sbar_SortTeams (void)
{
	int				i, j, k, mynum, playertoteam[MAX_CLIENTS];
	player_info_t	*s;
	char			t[SCR_TEAM_T_MAXTEAMSIZE];

// request new ping times every two second
	scoreboardteams = 0;

	if (!cl.teamplay)
		return;

	scoreboardteams = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
		playertoteam[i] = -1;

	// sort the teams
	memset(teams, 0, sizeof(teams));
	for (i = 0; i < MAX_CLIENTS; i++)
		teams[i].plow = 999;

	mynum = Sbar_PlayerNum();

	for (i = 0; i < MAX_CLIENTS; i++) {
		s = &cl.players[i];
		if (!s->name[0] || s->spectator)
			continue;

		// find his team in the list
		strlcpy (t, s->team, sizeof(t));
		if (!t[0])
			continue; // not on team

		for (j = 0; j < scoreboardteams; j++) {
			if (!strcmp (teams[j].team, t)) {
				playertoteam[i] = j;

				teams[j].frags += s->frags;
				teams[j].players++;
				if (!cl.teamfortress && i == mynum) {
//					teams[j].topcolor = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
//					teams[j].bottomcolor = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
					teams[j].topcolor = s->topcolor;
					teams[j].bottomcolor = s->bottomcolor;
				}
				break;
			}
		}
		if (j == scoreboardteams) { // must add him
			j = scoreboardteams++;
			playertoteam[i] = j;

			teams[j].frags = s->frags;
			teams[j].players = 1;
			if (cl.teamfortress) {
				teams[j].topcolor = teams[j].bottomcolor = Utils_TF_TeamToColor(t);
			} else {
//				teams[j].bottomcolor = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
//				teams[j].topcolor = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
				teams[j].bottomcolor = s->bottomcolor;
				teams[j].topcolor = s->topcolor;
			}
			strlcpy (teams[j].team, t, sizeof(teams[j].team));

			if (!Sbar_IsSpectator(mynum) && !strncmp(cl.players[mynum].team, t, sizeof(t) - 1))
				teams[j].myteam = true;
			else
				teams[j].myteam = false;
		}
		teams[j].plow = min(s->ping, teams[j].plow);
		teams[j].phigh = max(s->ping, teams[j].phigh);
		teams[j].ptotal += s->ping;
	}

	//prepare for sort
	for (i = 0; i < scoreboardteams; i++)
		teamsort[i] = i;

	// good 'ol bubble sort
	for (i = 0; i < scoreboardteams - 1; i++) {
		for (j = i + 1; j < scoreboardteams; j++) {
			if (teams[teamsort[i]].frags < teams[teamsort[j]].frags) {
				k = teamsort[i];
				teamsort[i] = teamsort[j];
				teamsort[j] = k;
			}
		}
	}

	
	if (cl.teamfortress) {
		for (i = 0; i < scoreboardteams; i++) {
			float frags = (float) teams[i].frags / teams[i].players;

			if (frags != (int) frags)
				return;

			for (j = 0; j < MAX_CLIENTS; j++) {
				s = &cl.players[j];
				if (!s->name[0] || s->spectator || playertoteam[j] != i)
					continue;

				if (s->frags != frags)
					return;
			}
		}

		
		for (i = 0; i < scoreboardteams; i++)
			teams[i].frags /= teams[i].players;
	}
}

static int Sbar_SortTeamsAndFrags_Compare(int a, int b) {
	int team_comp, frag_comp;
	char *team_one, *team_two;
	player_info_t *p1, *p2;

	p1 = &cl.players[a];
	p2 = &cl.players[b];

	if (p1->spectator)
		return p2->spectator ? Q_stricmp(p1->name, p2->name) : 1;
	else if (p2->spectator)
		return -1;

//	team_one = cl.teamfortress ? Utils_TF_ColorToTeam(p1->real_bottomcolor) : p1->team;
//	team_two = cl.teamfortress ? Utils_TF_ColorToTeam(p2->real_bottomcolor) : p2->team;
	team_one = cl.teamfortress ? Utils_TF_ColorToTeam(p1->bottomcolor) : p1->team;
	team_two = cl.teamfortress ? Utils_TF_ColorToTeam(p2->bottomcolor) : p2->team;

	if ((team_comp = strcmp(team_one, team_two))) {
		int i;
		team_t *t1 = NULL, *t2 = NULL;

		for (i = 0; i < scoreboardteams; i++) {
			if (!strcmp(team_one, teams[i].team)) {
				t1 = &teams[i];
				break;
			}
		}
		for (i = 0; i < scoreboardteams; i++) {
			if (!strcmp(team_two, teams[i].team)) {
				t2 = &teams[i];
				break;
			}
		}
		return (t1 && t2 && t1->frags != t2->frags) ? (t2->frags - t1->frags) : team_comp;
	} else {
		return (frag_comp = p2->frags - p1->frags) ? frag_comp : Q_stricmp(p1->name, p2->name);
	}
}

static void Sbar_SortTeamsAndFrags(qboolean specs)
{
	int i, j, k;
//	qboolean real_teamplay;

//	real_teamplay = cl.teamplay && (TP_CountPlayers() > 2);

//	if (!real_teamplay || !scr_scoreboard_teamsort.value) {
	if (!cl.teamplay || !scr_scoreboard_teamsort.value) {
		Sbar_SortFrags(specs);
		return;
	}

	scoreboardlines = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cl.players[i].name[0] && (specs || !cl.players[i].spectator)) {
			fragsort[scoreboardlines++] = i;
			if (cl.players[i].spectator)
				cl.players[i].frags = -999;
		}
	}

	Sbar_SortTeams();

	for (i = 0; i < scoreboardlines; i++) {
		for (j = 0; j < scoreboardlines - 1 - i ; j++) {
			if (Sbar_SortTeamsAndFrags_Compare(fragsort[j], fragsort[j + 1]) > 0) {
				k = fragsort[j];
				fragsort[j] = fragsort[j + 1];
				fragsort[j + 1] = k;
			}
		}
	}
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
static void Sbar_DrawInventory (void)
{
	int i, flashon;
	char num[6];
	float time;
	qboolean headsup, hudswap;

	headsup = !(cl_sbar.value || scr_viewsize.value < 100);
	hudswap = cl_hudswap.value; // Get that nasty float out :)

	if (!headsup)
		Sbar_DrawPic (0, -24, sb_ibar);
	// weapons
	for (i = 0; i < 7; i++) {
		if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << i) ) {
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time) * 10);
			if (flashon < 0)
				flashon = 0;
			if (flashon >= 10)
				flashon = (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i) ) ? 1 : 0;
			else
				flashon = (flashon % 5) + 2;

			if (headsup) {
				if (i || vid.conheight > 200)
					Sbar_DrawSubPic (hudswap ? 0 : vid.conwidth - 24,-68 - (7 - i) * 16 , sb_weapons[flashon][i], 0, 0, 24, 16);
			
			} else {
				Sbar_DrawPic (i * 24, -16, sb_weapons[flashon][i]);
			}

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

	// ammo counts
	for (i = 0; i < 4; i++) {
		Q_snprintfz (num, sizeof(num), "%3i", cl.stats[STAT_SHELLS + i]);
		if (headsup) {
			Sbar_DrawSubPic(hudswap ? 0 : vid.conwidth - 42, -24 - (4 - i) * 11, sb_ibar, 3 + (i * 48), 0, 42, 11);
			if (num[0] != ' ')
				Draw_Character (hudswap ? 7: vid.conwidth - 35, vid.conheight - SBAR_HEIGHT - 24 - (4 - i) * 11, 18 + num[0] - '0');
			if (num[1] != ' ')
				Draw_Character (hudswap ? 15: vid.conwidth - 27, vid.conheight - SBAR_HEIGHT - 24 - (4 - i) * 11, 18 + num[1] - '0');
			if (num[2] != ' ')
				Draw_Character (hudswap ? 23: vid.conwidth - 19, vid.conheight - SBAR_HEIGHT - 24 - (4 - i) * 11, 18 + num[2] - '0');
		} else {
			if (num[0] != ' ')
				Sbar_DrawCharacter ((6 * i + 1) * 8 - 2, -24, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter ((6 * i + 2) * 8 - 2, -24, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter ((6 * i + 3) * 8 - 2, -24, 18 + num[2] - '0');
		}
	}

	flashon = 0;
	// items
	for (i = 0; i < 6; i++) {
		if (cl.stats[STAT_ITEMS] & (1 << (17 + i))) {
			time = cl.item_gettime[17 + i];
			if (time &&	time > cl.time - 2 && flashon)
				// flash frame
				sb_updates = 0;
			else
				Sbar_DrawPic (192 + i * 16, -16, sb_items[i]);	

			if (time &&	time > cl.time - 2)
				sb_updates = 0;
		}
	}

	// sigils
	for (i = 0; i < 4; i++) {
		if (cl.stats[STAT_ITEMS] & (1 << (28 + i)))	{
			time = cl.item_gettime[28 + i];
			if (time && time > cl.time - 2 && flashon )
				// flash frame
				sb_updates = 0;
			else
				Sbar_DrawPic (320 - 32 + i * 8, -16, sb_sigil[i]);	

			if (time && time > cl.time - 2)
				sb_updates = 0;
		}
	}
}

//=============================================================================
static void Sbar_DrawFrags_DrawCell(int x, int y, int topcolor, int bottomcolor, int frags, int brackets)
{
	char num[4];

	// draw background
	Draw_Fill (sbar_xofs + x * 8 + 10, y, 28, 4, Sbar_ColorForMap (topcolor));
	Draw_Fill (sbar_xofs + x * 8 + 10, y + 4, 28, 3, Sbar_ColorForMap (bottomcolor));

	// draw number
	Q_snprintfz (num, sizeof(num), "%3i", frags);

	Sbar_DrawCharacter ((x + 1) * 8 , -24, num[0]);
	Sbar_DrawCharacter ((x + 2) * 8 , -24, num[1]);
	Sbar_DrawCharacter ((x + 3) * 8 , -24, num[2]);

	if (brackets) {
		Sbar_DrawCharacter (x * 8 + 2, -24, 16);
		Sbar_DrawCharacter ((x + 4) * 8 - 4, -24, 17);
	}
}

/*
===============
Sbar_DrawFrags
===============
*/
static void Sbar_DrawFrags (void)
{
	int i, k, l, top, bottom, x, y, mynum, myteam = -1;
	player_info_t *s;
	team_t *tm;
	qboolean drawn_self = false, drawn_self_team = false;

	if (!scr_drawHFrags.value)
		return;

	x = 23;
	y = vid.conheight - SBAR_HEIGHT - 23;

	mynum = Sbar_PlayerNum();

	if (scr_drawHFrags.value != 2 || !cl.teamplay) {

		Sbar_SortFrags (false);
		l = min(scoreboardlines, 4);

		for (i = 0; i < l; i++) {
			if (i + 1 == l && !drawn_self && !Sbar_IsSpectator(mynum))
				k = mynum;
			else
				k = fragsort[i];

			s = &cl.players[k];

			if (!s->name[0] || s->spectator)
				continue;

//			top = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
//			bottom = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
			top = s->topcolor;
			bottom = s->bottomcolor;
			top = bound(0, top, 13);
			bottom = bound(0, bottom, 13);

			Sbar_DrawFrags_DrawCell(x, y, top, bottom, s->frags, k == mynum);
			
			if (k == mynum)
				drawn_self = true;

			x += 4;
		}
	} else {
		Sbar_SortTeams();

		for (i = 0; i < scoreboardteams; i++) {
			if (teams[i].myteam) {
				myteam = i;
				break;
			}
		}

		l = min(scoreboardteams, 4);
		for (i = 0; i < l; i++) {
			if (myteam != -1 && i + 1 == l && !drawn_self_team)	
				k = myteam;
			else
				k = teamsort[i];

			tm = &teams[k];
			Sbar_DrawFrags_DrawCell(x, y, tm->topcolor, tm->bottomcolor, tm->frags, tm->myteam);

			if (k == myteam)
				drawn_self_team = true;

			x += 4;
		}

		if (l <= 2 && !Sbar_IsSpectator(mynum)) {
			
			x += 4 * (3 - l);

			s = &cl.players[mynum];

//			top = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
//			bottom = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
			top = s->topcolor;
			bottom = s->bottomcolor;
			top = bound(0, top, 13);
			bottom = bound(0, bottom, 13);

			Sbar_DrawFrags_DrawCell(x, y, top, bottom, s->frags, 1);
		}
	}
}

/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (void)
{
	int		f, anim;

	if ( (cl.stats[STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_QUAD) 
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY) 
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY) 
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[STAT_HEALTH] / 20;
	
	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
=============
Sbar_DrawNormal
=============
*/
static void Sbar_DrawNormal (void) {
	if (cl_sbar.value || scr_viewsize.value < 100)
		Sbar_DrawPic (0, 0, sb_sbar);

	// armor
	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)	{
		Sbar_DrawNum (24, 0, 666, 3, 1);
		Sbar_DrawPic (0, 0, draw_disc);
	} else {
		Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
		if (cl.stats[STAT_ITEMS] & IT_ARMOR3)
			Sbar_DrawPic (0, 0, sb_armor[2]);
		else if (cl.stats[STAT_ITEMS] & IT_ARMOR2)
			Sbar_DrawPic (0, 0, sb_armor[1]);
		else if (cl.stats[STAT_ITEMS] & IT_ARMOR1)
			Sbar_DrawPic (0, 0, sb_armor[0]);
	}

	// face
	Sbar_DrawFace ();
	
	// health
	Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

	// ammo icon
	if (cl.stats[STAT_ITEMS] & IT_SHELLS)
		Sbar_DrawPic (224, 0, sb_ammo[0]);
	else if (cl.stats[STAT_ITEMS] & IT_NAILS)
		Sbar_DrawPic (224, 0, sb_ammo[1]);
	else if (cl.stats[STAT_ITEMS] & IT_ROCKETS)
		Sbar_DrawPic (224, 0, sb_ammo[2]);
	else if (cl.stats[STAT_ITEMS] & IT_CELLS)
		Sbar_DrawPic (224, 0, sb_ammo[3]);

// BorisU -->	
	if (show_speed.value != 1) {
		Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);
	} else {
		Hud_SpeedStr();
		Sbar_DrawNum (248, 0, display_speed, 3, 1);
	}
// <-- BorisU
}

static void Sbar_DrawCompact(void)
{
	int i, align, old_sbar_xofs;
	static char *weapons[7] = {"sg", "bs", "ng", "sn", "gl", "rl", "lg"};
	char str[4];

	if (cl_sbar.value || scr_viewsize.value < 100)
		Sbar_DrawPic (0, 0, sb_sbar);

	old_sbar_xofs = sbar_xofs;
	sbar_xofs = scr_centerSbar.value ? (vid.conwidth - 306) >> 1: 0;

	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
		Sbar_DrawNum (2, 0, 666, 3, 1);
	else
		Sbar_DrawNum (2, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);

	Sbar_DrawNum (86, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

	align = scr_compactHudAlign.value ? 1 : 0;
	for (i = 0; i < 4; i++) {
		Q_snprintfz(str, sizeof(str), "%d", cl.stats[STAT_SHELLS + i]);
		if (cl.stats[STAT_SHELLS + i] < 5)
			Sbar_DrawAltString(align * 8 * (3 - strlen(str)) + 174 + 32 * i, 3, str);
		else
			Sbar_DrawString(align * 8 * (3 - strlen(str)) + 174 + 32 * i, 3, str);
	}
	for (i = 0; i < 7; i++) {
		if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << i)) {
			if (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i))
				Sbar_DrawAltString(166 + 5 * 4 * i, 14, weapons[i]);
			else
				Sbar_DrawString(166 + 5 * 4 * i, 14, weapons[i]);
		}
	}
	sbar_xofs = old_sbar_xofs;
}

static void Sbar_DrawCompact_TF(void)
{
	int i, align, old_sbar_xofs;
	char str[4];

	if (cl_sbar.value || scr_viewsize.value < 100)
		Sbar_DrawPic (0, 0, sb_sbar);

	old_sbar_xofs = sbar_xofs;
	sbar_xofs = scr_centerSbar.value ? (vid.conwidth - 222) >> 1: 0;

	align = scr_compactHudAlign.value ? 1 : 0;
	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
		Sbar_DrawNum (2, 0, 666, 3, 1);
	else
		Sbar_DrawNum (2, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
	Sbar_DrawNum (86, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);
	for (i = 0; i < 4; i++) {
			Q_snprintfz(str, sizeof(str), "%d", cl.stats[STAT_SHELLS + i]);
			if (cl.stats[STAT_SHELLS + i] < 5)
				Sbar_DrawAltString(align * 8 * (3 - strlen(str)) + 166 + 32 * (i % 2), i >= 2 ? 14 : 3, str);
			else
				Sbar_DrawString(align * 8 * (3 - strlen(str)) + 166 + 32 * (i % 2), i >= 2 ? 14 : 3, str);
	}
	sbar_xofs = old_sbar_xofs;
}

static void Sbar_DrawCompact_Bare (void)
{
	int old_sbar_xofs;

	if (cl_sbar.value || scr_viewsize.value < 100)
		Sbar_DrawPic (0, 0, sb_sbar);

	old_sbar_xofs = sbar_xofs;
	sbar_xofs = scr_centerSbar.value ? (vid.conwidth - 158) >> 1: 0;

	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
		Sbar_DrawNum (2, 0, 666, 3, 1);
	else
		Sbar_DrawNum (2, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
	Sbar_DrawNum (86, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);
	sbar_xofs = old_sbar_xofs;
}

/*
===============
Sbar_SoloScoreboard
===============
*/
void Sbar_SoloScoreboard (void)
{
	char	str[80];
	int		minutes, seconds, tens, units;

	Sbar_DrawPic (0, 0, sb_scorebar);

	// time
	minutes = cl.time / 60;
	seconds = cl.time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);
}

// fuh -->
#define SCOREBOARD_LASTROW		(vid.conheight - 34)

#define	RANK_WIDTH_BASICSTATS	(11 * 8)
#define	RANK_WIDTH_TEAMSTATS	(4 * 8)
#define	RANK_WIDTH_TCHSTATS		(5 * 8)
#define	RANK_WIDTH_CAPSTATS		(5 * 8)

#define RANK_WIDTH_DM				(-8 + 168 + (MAX_SCOREBOARDNAME * 8))
#define RANK_WIDTH_TEAM				(-8 + 208 + (MAX_SCOREBOARDNAME * 8))

#ifdef GLQUAKE
#define SCOREBOARD_ALPHA			(0.5 * bound(0, scr_scoreboard_fillalpha.value, 1))
#define SCOREBOARD_HEADINGALPHA		(bound(0, scr_scoreboard_fillalpha.value, 1))
#else
#define Draw_AlphaFill(a, b, c, d, e, f)
#endif
// <-- fuh

/*
==================
Sbar_DeathmatchOverlay

ping time frags name
==================
*/
static void Sbar_DeathmatchOverlay_Fuh (int start)
{
	int scoreboardsize, colors_thickness;
	int i, k, top, bottom, x, y, xofs, total, minutes, p, skip = 10;
	int rank_width, leftover, startx, tempx, mynum;
	char num[12], scorerow[64], team[5], name[MAX_SCOREBOARDNAME];
	player_info_t *s;
	mpic_t *pic;

	if (cls.state == ca_active && !cls.demoplayback) {
		// request new ping times every two seconds
		if (cls.realtime - cl.last_ping_request >= 2) {
			cl.last_ping_request = cls.realtime;
			MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
			SZ_Print (&cls.netchan.message, "pings");
		}
	}

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	mynum = Sbar_PlayerNum();

	rank_width = (cl.teamplay ? RANK_WIDTH_TEAM : RANK_WIDTH_DM);

	leftover = rank_width;
	rank_width = bound(0, rank_width, vid.conwidth - 16);
	leftover = max(0, leftover - rank_width);
	xofs = ((vid.conwidth - rank_width) >> 1);

	if (!start) {
		if (scr_scoreboard_drawtitle.value) {
			pic = Draw_CachePic ("gfx/ranking.lmp");
			Draw_Pic (xofs + (rank_width - pic->width) / 2, 0, pic);
			start = 36;
		} else {
			start = 12;
		}
	}

	y = start;

	if (!scr_scoreboard_borderless.value)
		Draw_Fill (xofs - 1, y - 9, rank_width + 2, 1, 0);						//Border - Top
	Draw_AlphaFill (xofs, y - 8, rank_width, 9, 1, SCOREBOARD_HEADINGALPHA);	//Draw heading row

	Draw_String(xofs + 1, y - 8, cl.teamplay ? " ping pl time frags team name" : " ping pl time frags name");

	Draw_Fill (xofs -1, y + 1, rank_width + 2, 1, 0);	//Border - Top (under header)
	y += 2;												//dont go over the black border, move the rest down
	if (!scr_scoreboard_borderless.value) {
		Draw_Fill (xofs - 1, y - 10, 1, 10, 0);						//Border - Left
		Draw_Fill (xofs - 1 + rank_width + 1, y - 10, 1, 10, 0);	//Border - Right
	}
	startx = x = xofs + 8;

	tempx = xofs + rank_width - MAX_SCOREBOARDNAME * 8 - (48) + leftover;

	Sbar_SortTeamsAndFrags (true);	// scores

	for (i = scoreboardsize = 0; i < scoreboardlines; i++) {
		k = fragsort[i];
		s = &cl.players[k];

		if (s->name[0])
			scoreboardsize++;
	}

	if (y + (scoreboardsize - 1) * skip > SCOREBOARD_LASTROW) {
		colors_thickness = 3;
		skip = 8;
	} else {
		colors_thickness = 4;
	}

	for (i = 0; i < scoreboardlines && y <= SCOREBOARD_LASTROW; i++) {
		k = fragsort[i];
		s = &cl.players[k];

		if (!s->name[0])
			continue;

		//render the main background transparencies behind players row
//		top = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
//		bottom = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
		top = s->topcolor;
		bottom = s->bottomcolor;

#ifdef GLQUAKE
		if (
			!cl.teamplay || s->spectator ||
			!scr_scoreboard_fillcolored.value || (scr_scoreboard_fillcolored.value == 2 && !scr_scoreboard_teamsort.value)
			) {
				Draw_AlphaFill (xofs, y, rank_width, skip, 2, SCOREBOARD_ALPHA);
			} else {
				strlcpy (team, s->team, sizeof(team));
				if (k == mynum) {
					float alpha;
					alpha = 1.7 * SCOREBOARD_ALPHA;
					alpha = min(alpha, 0.75);
					Draw_AlphaFill (xofs, y, rank_width, skip, Sbar_ColorForMap(bottom), alpha);
				} else {
					Draw_AlphaFill (xofs, y, rank_width, skip, Sbar_ColorForMap(bottom), SCOREBOARD_ALPHA);
				}
			}
#endif

			if (!scr_scoreboard_borderless.value) {
				Draw_Fill (xofs - 1, y, 1, skip, 0);					//Border - Left
				Draw_Fill (xofs - 1 + rank_width + 1, y, 1, skip, 0);	//Border - Right
			}

			// draw ping
			p = s->ping;
			if (p < 0 || p > 999)
				p = 999;

			Q_snprintfz (num, sizeof(num), "&cAAD%4i", p);
			Draw_ColoredString(x, y, num, 0);
			x += 32; // move it forward, ready to print next column

			// draw pl
			p = s->pl;

			if (p > 25) {
				Q_snprintfz (num, sizeof(num), "&cD33%3i", p);
				Draw_ColoredString (x, y, num, 1);
			} else {
				Q_snprintfz (num, sizeof(num), "&cDB0%3i", p);
				Draw_ColoredString (x, y, num, 0);
			}

			x += 32;

			if (s->spectator) {
				Draw_ColoredString (x, y, "&cF20s&cF50p&cF80e&c883c&cA85t&c668a&c55At&c33Bo&c22Dr", 0);

				x += cl.teamplay ? 128 : 88; // move across to print the name

				strlcpy(name, s->name, sizeof(name));
				if (leftover > 0) {
					int truncate = (leftover / 8) + (leftover % 8 ? 1 : 0);

					truncate = bound(0, truncate, sizeof(name) - 1);
					name[sizeof(name) - 1 - truncate] = 0;
				}
				Draw_String (x, y, name);	// draw name

				y += skip;
				x = startx;
				continue;
			}

			// draw time
			total = (cl.intermission ? cl.completed_time : cls.demoplayback ? cls.demotime : cls.realtime) - s->entertime;
			minutes = (int) total / 60;

			// print the shirt/pants colour bars
			Draw_Fill (cl.teamplay ? tempx - 40 : tempx, y + 4 - colors_thickness, 40, colors_thickness, Sbar_ColorForMap (top));
			Draw_Fill (cl.teamplay ? tempx - 40 : tempx, y + 4, 40, 4, Sbar_ColorForMap (bottom));

			// name
			strlcpy(name, s->name, sizeof(name));
			if (leftover > 0) {
				int truncate = (leftover / 8) + (leftover % 8 ? 1 : 0);

				truncate = bound(0, truncate, sizeof(name) - 1);
				name[sizeof(name) - 1 - truncate] = 0;
			}

			// team
			if (cl.teamplay) {
				strlcpy  (team, s->team, sizeof(team));
				Q_snprintfz (scorerow, sizeof(scorerow), " %3i  %3i  %-4s %s", minutes, s->frags, team, name);
			} else {
				Q_snprintfz (scorerow, sizeof(scorerow), " %3i  %3i  %s", minutes, s->frags, name);
			}
			Draw_String (x, y, scorerow);


			if (k == mynum) {
				Draw_Character (x + 40, y, 16);
				Draw_Character (x + 40 + 32, y, 17);
			}

			y += skip;
			x = startx;
	}

	if (!scr_scoreboard_borderless.value)
		Draw_Fill (xofs - 1, y , rank_width + 2, 1, 0); //Border - Bottom
}

void Sbar_DeathmatchOverlay (int start)
{
	mpic_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	player_info_t	*s;
	int				total;
	int				minutes;
	int				p;
	int				teamplay;
	char			team[5];
	int				skip = 10;
	int				xofs; // Tonik

	if (scr_scoreboard_new.value) {
		Sbar_DeathmatchOverlay_Fuh (start);
		return;
	}

	if (largegame)
		skip = 8;

// request new ping times every two second
	if (cls.realtime - cl.last_ping_request > 2)
	{
		cl.last_ping_request = cls.realtime;
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "pings");
	}

	teamplay = atoi(Info_ValueForKey(cl.serverinfo, "teamplay"));

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	xofs = (vid.conwidth - 320)>>1;

	if (!start) {
		pic = Draw_CachePic ("gfx/ranking.lmp");
		Draw_Pic (xofs + 160 - pic->width/2, 0, pic);
	}

// scores	
	Sbar_SortFrags (true);

// draw the text
	l = scoreboardlines;

	if (start)
		y = start;
	else
		y = 24;
	if (teamplay)
	{
		x = xofs + 4;
//                            0    40 64   104   152  192 
		Draw_String ( x , y, "ping pl time frags team name");
		y += 8;
//		Draw_String ( x , y, "---- -- ---- ----- ---- ----------------");
		Draw_String ( x , y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f");
		y += 8;
	}
	else
	{
		x = xofs + 16;
//                            0    40 64   104   152
		Draw_String ( x , y, "ping pl time frags name");
		y += 8;
//		Draw_String ( x , y, "---- -- ---- ----- ----------------");
		Draw_String ( x , y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f");
		y += 8;
	}

	for (i=0 ; i<l && y <= vid.conheight-10 ; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

		// draw ping
		p = s->ping;
		if (p < 0 || p > 999)
			p = 999;
		sprintf (num, "%4i", p);
		Draw_String ( x, y, num);

		// draw pl
		p = s->pl;
		sprintf (num, "%3i", p);
		if (p > 25)
			Draw_Alt_String ( x+32, y, num);
		else
			Draw_String ( x+32, y, num);

		if (s->spectator)
		{
			Draw_String (x+64, y, "(spectator)");
			// draw name
			if (teamplay)
				Draw_String (x+152+40, y, s->longname);
			else
				Draw_String (x+152, y, s->longname);
			y += skip;
			continue;
		}


		// draw time
		if (cl.intermission)
			total = cl.completed_time - s->entertime;
		else
			total = cls.realtime - s->entertime;
		minutes = (int)total/60;
		sprintf (num, "%4i", minutes);
		Draw_String ( x+64 , y, num);

		// draw background
		top = s->topcolor;
		bottom = s->bottomcolor;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);
	
		if (largegame)
			Draw_Fill ( x+104, y+1, 40, 3, top);
		else
			Draw_Fill ( x+104, y, 40, 4, top);
		Draw_Fill ( x+104, y+4, 40, 4, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);
		
		Draw_Character ( x+112 , y, num[0]);
		Draw_Character ( x+120 , y, num[1]);
		Draw_Character ( x+128 , y, num[2]);

		if (k == cl.playernum)
		{
			Draw_Character ( x + 104, y, 16);
			Draw_Character ( x + 136, y, 17);
		}
		
		// team
		if (teamplay)
		{
			team[4] = 0;
			strncpy (team, Info_ValueForKey(s->userinfo, "team"), 4);
			Draw_String (x+152, y, team);
		}

		// draw name
		if (teamplay)
			Draw_String (x+152+40, y, s->longname);
		else
			Draw_String (x+152, y, s->longname);
		
		y += skip;
	}

	if (y >= vid.conheight-10) // we ran over the screen size, squish
		largegame = true;
}

void Sbar_TeamOverlay_Fuh (void)
{
	int i, k, l, x, y, xofs, plow, phigh, pavg, rank_width, skip = 10;
	char num[12], team[5];
	team_t *tm;
	mpic_t *pic;

	if (!cl.teamplay) {
		Sbar_DeathmatchOverlay(0);
		return;
	}

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	rank_width = cl.teamplay ? RANK_WIDTH_TEAM : RANK_WIDTH_DM;
	rank_width = bound(0, rank_width, vid.conwidth - 16);

	xofs = (vid.conwidth - rank_width) >> 1;

	if (scr_scoreboard_drawtitle.value) {
		pic = Draw_CachePic ("gfx/ranking.lmp");
		Draw_Pic (xofs + (rank_width - pic->width) / 2, 0, pic);
		y = 26;
	} else {
		y = 2;
	}

	if (!scr_scoreboard_borderless.value)
		Draw_Fill (xofs - 1, y + 1, rank_width + 2, 1, 0);							//Border - Top
	Draw_AlphaFill (xofs, y + 2, rank_width, skip, 1, SCOREBOARD_HEADINGALPHA);	//draw heading row
	Draw_Fill (xofs - 1, y + 11, rank_width + 2, 1, 0);							//Border - Top (under header)
	y += 2;																		//dont go over the black border, move the rest down
	if (!scr_scoreboard_borderless.value) {
		Draw_Fill (xofs - 1, y, 1, skip, 0);										//Border - Left
		Draw_Fill (xofs - 1 + rank_width + 1, y, 1, skip, 0);						//Border - Right
	}

	x = xofs + rank_width * 0.1 + 8;

	Draw_String(x, y, "low/avg/high team total players");

	y += 10;

	Sbar_SortTeams();		// sort the teams

	l = scoreboardlines;	// draw the text

	for (i = 0; i < scoreboardteams && y <= SCOREBOARD_LASTROW; i++)	{
		k = teamsort[i];
		tm = teams + k;

		Draw_AlphaFill (xofs, y, rank_width, skip, 2, SCOREBOARD_ALPHA);

		if (!scr_scoreboard_borderless.value) {
			Draw_Fill (xofs - 1, y, 1, skip, 0);						//Border - Left
			Draw_Fill (xofs - 1 + rank_width + 1, y, 1, skip, 0);		//Border - Right
		}

		// draw pings
		plow = tm->plow;
		if (plow < 0 || plow > 999)
			plow = 999;

		phigh = tm->phigh;
		if (phigh < 0 || phigh > 999)
			phigh = 999;

		pavg = tm->players ? tm->ptotal / tm->players : 999;
		if (pavg < 0 || pavg > 999)
			pavg = 999;

		Q_snprintfz (num, sizeof(num), "%3i/%3i/%3i", plow, pavg, phigh);
		Draw_String (x, y, num);

		// draw team
		strlcpy (team, tm->team, sizeof(team));
		Draw_String (x + 104, y, team);

		// draw total
		Q_snprintfz (num, sizeof(num), "%5i", tm->frags);
		Draw_String (x + 104 + 40, y, num);

		// draw players
		Q_snprintfz (num, sizeof(num), "%5i", tm->players);
		Draw_String (x + 104 + 88, y, num);

		if (tm->myteam) {
			Draw_Character (x + 104 - 8, y, 16);
			Draw_Character (x + 104 + 32, y, 17);
		}

		y += skip;
	}

	if (!scr_scoreboard_borderless.value)
		Draw_Fill (xofs - 1, y, rank_width + 2, 1, 0);		//Border - Bottom

	y += 14;
	Sbar_DeathmatchOverlay(y);
}

/*
==================
Sbar_TeamOverlay

team frags
added by Zoid
==================
*/
void Sbar_TeamOverlay (void)
{
	mpic_t			*pic;
	int				i, k, l;
	int				x, y;
	char			num[12];
	char			team[5];
	team_t			*tm;
	int plow, phigh, pavg;
	int				xofs; // Tonik

	if (scr_scoreboard_new.value) {
		Sbar_TeamOverlay_Fuh ();
		return;
	}

// request new ping times every two second

	if (!cl.teamplay) {
		Sbar_DeathmatchOverlay(0);
		return;
	}

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	xofs = (vid.conwidth - 320)>>1;

	pic = Draw_CachePic ("gfx/ranking.lmp");
	Draw_Pic ( xofs + 160 - pic->width/2, 0, pic);

	y = 24;
	x = xofs + 36;
	Draw_String(x, y, "low/avg/high team total players");
	y += 8;
//	Draw_String(x, y, "------------ ---- ----- -------");
	Draw_String(x, y, "\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1f");
	y += 8;

// sort the teams
	Sbar_SortTeams();

// draw the text
	l = scoreboardlines;

	for (i=0 ; i < scoreboardteams && y <= vid.conheight-10 ; i++)
	{
		k = teamsort[i];
		tm = teams + k;

	// draw pings
		plow = tm->plow;
		if (plow < 0 || plow > 999)
			plow = 999;
		phigh = tm->phigh;
		if (phigh < 0 || phigh > 999)
			phigh = 999;
		if (!tm->players)
			pavg = 999;
		else
			pavg = tm->ptotal / tm->players;
		if (pavg < 0 || pavg > 999)
			pavg = 999;

		sprintf (num, "%3i/%3i/%3i", plow, pavg, phigh);
		Draw_String ( x, y, num);

	// draw team
		team[4] = 0;
		strncpy (team, tm->team, 4);
		Draw_String (x + 104, y, team);

	// draw total
		sprintf (num, "%5i", tm->frags);
		Draw_String (x + 104 + 40, y, num);
		
	// draw players
		sprintf (num, "%5i", tm->players);
		Draw_String (x + 104 + 88, y, num);
		
		if (!strncmp(Info_ValueForKey(cl.players[cl.playernum].userinfo,
			"team"), tm->team, 16)) {
			Draw_Character ( x + 104 - 8, y, 16);
			Draw_Character ( x + 104 + 32, y, 17);
		}
		
		y += 8;
	}
	y += 8;
	Sbar_DeathmatchOverlay(y);
}

/*
==================
Sbar_MiniDeathmatchOverlay

frags name
frags team name
displayed to right of status bar if there's room
==================
*/
static void Sbar_MiniDeathmatchOverlay (void)
{
	int i, k, top, bottom, x, y, mynum, numlines;
	char num[4 + 1], name[16 + 1], team[4 + 1];
	player_info_t *s;
	team_t *tm;

	if (!scr_drawVFrags.value)
		return;

	if (!sb_lines)
		return; // not enuff room

	scr_copyeverything = 1;
	scr_fullupdate = 0;

	mynum = Sbar_PlayerNum();

	x = 324;

	
	if (vid.conwidth < 640 && cl.teamplay && scr_drawVFrags.value == 2)
		goto drawteams;

	// scores
	Sbar_SortFrags (false);

	if (!scoreboardlines)
		return; // no one there?

	// draw the text
	y = vid.conheight - sb_lines - 1;
	numlines = sb_lines / 8;
	if (numlines < 3)
		return; // not enough room

	// find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == mynum)
			break;

	if (i == scoreboardlines)	// we're not there, we are probably a spectator, just display top
		i = 0;
	else						// figure out start
		i = i - numlines / 2;

	i = bound(0, i, scoreboardlines - numlines);

	for ( ; i < scoreboardlines && y < vid.conheight - 8 + 1; i++) {
		k = fragsort[i];
		s = &cl.players[k];

		if (!s->name[0])
			continue;

//		top = scr_scoreboard_forcecolors.value ? s->topcolor : s->real_topcolor;
//		bottom = scr_scoreboard_forcecolors.value ? s->bottomcolor : s->real_bottomcolor;
		top = s->topcolor;
		bottom = s->bottomcolor;

		Draw_Fill (x, y + 1, 40, 3, Sbar_ColorForMap (top));
		Draw_Fill (x, y + 4, 40, 4, Sbar_ColorForMap (bottom));

		// draw number
		Q_snprintfz (num, sizeof(num), "%3i", s->frags);

		Draw_Character (x + 8 , y, num[0]);
		Draw_Character (x + 16, y, num[1]);
		Draw_Character (x + 24, y, num[2]);

		if (k == mynum) {
			Draw_Character (x, y, 16);
			Draw_Character (x + 32, y, 17);
		}

		// team
		if (cl.teamplay) {
			strlcpy (team, s->team, sizeof(team));
			Draw_String (x + 48, y, team);
		}

		// draw name
		strlcpy (name, s->name, sizeof(name));
		if (cl.teamplay)
			Draw_String (x + 48 + 40, y, name);
		else
			Draw_String (x + 48, y, name);

		y += 8;
	}

	// draw teams if room
	if (vid.conwidth < 640 || !cl.teamplay)
		return;

	Sbar_SortTeams();
	if (!scoreboardteams)
		return;	

	// draw seperator
	x += 208;
	for (y = vid.conheight - sb_lines; y < vid.conheight - 6; y += 2)
		Draw_Character(x, y, 14);

	x += 16;

drawteams:
	Sbar_SortTeams();	

	y = vid.conheight - sb_lines;
	for (i = 0; i < scoreboardteams && y < vid.conheight - 8 + 1; i++) {
		k = teamsort[i];
		tm = teams + k;

		//draw name
		strlcpy (team, tm->team, sizeof(team));
		Draw_String (x, y, team);

		// draw total
		Draw_Fill (x + 40, y + 1, 48, 3, Sbar_ColorForMap (tm->topcolor));
		Draw_Fill (x + 40, y + 4, 48, 4, Sbar_ColorForMap (tm->bottomcolor));
		Q_snprintfz (num, sizeof(num), "%4i", tm->frags);
		Draw_Character (x + 40 + 8 , y, num[0]);
		Draw_Character (x + 40 + 16, y, num[1]);
		Draw_Character (x + 40 + 24, y, num[2]);
		Draw_Character (x + 40 + 32, y, num[3]);

		if (tm->myteam) {
			Draw_Character (x + 40, y, 16);
			Draw_Character (x + 40 + 40, y, 17);
		}

		y += 8;
	}
}


/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay (void)
{
	scr_copyeverything = 1;
	scr_fullupdate = 0;

	if (cl.teamplay && !sb_showscores)
		Sbar_TeamOverlay ();
	else
		Sbar_DeathmatchOverlay (0);
}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay (void)
{
	mpic_t	*pic;

	scr_copyeverything = 1;

	pic = Draw_CachePic ("gfx/finale.lmp");
	Draw_TransPic ( (vid.conwidth-pic->width)/2, 16, pic);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	qboolean headsup;
	char st[512];

	headsup = !(cl_sbar.value || scr_viewsize.value<100);
	if ((sb_updates >= vid.numpages) && !headsup)
		return;

	if (scr_con_current == vid.conheight)
		return;		// console is full screen

	scr_copyeverything = 1;

	sb_updates++;

	if (scr_centerSbar.value)
		sbar_xofs = (vid.conwidth - 320) >> 1;
	else
		sbar_xofs = 0;

// top line
	if (sb_lines > 24) {
		if (!cl.spectator || autocam == CAM_TRACK)
			Sbar_DrawInventory ();
		if (!headsup || vid.conwidth<512)
			Sbar_DrawFrags ();
	}	

// main area
	if (sb_lines > 0) {
		if (cl.spectator) {
			if (autocam != CAM_TRACK) {
				Sbar_DrawPic (0, 0, sb_scorebar);
				Sbar_DrawString (160-7*8,4, "SPECTATOR MODE");
				Sbar_DrawString(160-14*8+4, 12, "Press [ATTACK] for AutoCamera");
			} else {
				if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
					Sbar_SoloScoreboard ();
				else if (scr_compactHud.value == 3)
					Sbar_DrawCompact_Bare();
				else if (scr_compactHud.value == 2)
					Sbar_DrawCompact_TF();
				else if (scr_compactHud.value == 1)
					Sbar_DrawCompact();
				else
					Sbar_DrawNormal ();
// 5AT0H -->
				sprintf (st, "Tracking %-.13s", cl.players[spec_track].name);
				if (!cls.demoplayback)
					strcat (st, ", [JUMP] for next");
// <-- 5AT0H 
				Sbar_DrawString(0, -8, st);
			}
		} else if (sb_showscores || sb_showteamscores || cl.stats[STAT_HEALTH] <= 0) {
			Sbar_SoloScoreboard();
		} else if (scr_compactHud.value == 3) {
			Sbar_DrawCompact_Bare();
		} else if (scr_compactHud.value == 2) {
			Sbar_DrawCompact_TF();
		} else if (scr_compactHud.value == 1) {
			Sbar_DrawCompact();
		} else {
			Sbar_DrawNormal();
		}
	}

// main screen deathmatch rankings
	// if we're dead show team scores in team games
	if (cl.stats[STAT_HEALTH] <= 0 && !cl.spectator)
		if (cl.teamplay && !sb_showscores)
			Sbar_TeamOverlay();
		else
			Sbar_DeathmatchOverlay (0);
	else if (sb_showscores)
		Sbar_DeathmatchOverlay (0);
	else if (sb_showteamscores)
		Sbar_TeamOverlay();

#ifdef GLQUAKE
	if (sb_showscores || sb_showteamscores || cl.stats[STAT_HEALTH] <= 0)
		sb_updates = 0;
	// clear unused areas in GL
	if (vid.conwidth > 320 && !headsup) {
		if (scr_centerSbar.value)	// left
			Draw_TileClear (0, vid.conheight - sb_lines, sbar_xofs, sb_lines);
		Draw_TileClear (320 + sbar_xofs, vid.conheight - sb_lines, vid.conwidth - (320 + sbar_xofs), sb_lines);	// right
	}
	if (!headsup && cl.spectator && autocam != CAM_TRACK && sb_lines > SBAR_HEIGHT)
		Draw_TileClear (sbar_xofs, vid.conheight - sb_lines, 320, sb_lines - SBAR_HEIGHT);
#endif

	if (vid.conwidth >= 512 && sb_lines > 0 && !scr_centerSbar.value)
		Sbar_MiniDeathmatchOverlay ();
}
