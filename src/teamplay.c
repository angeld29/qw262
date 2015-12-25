#include "quakedef.h"

char *TP_MapName(void)
{
	return mapname.string;
}

char *TP_GetMapGroupName(char *mapname, qboolean *system);

void TP_NewMap (void)
{
	char *groupname;
	qboolean system;

	Cvar_SetROM (&mapname, Info_ValueForKey(cl.serverinfo,"map"));

	groupname = TP_GetMapGroupName(mapname.string, &system);
	
	if (groupname && !system)
		Cvar_SetROM (&mapgroupname, groupname);
	else
		Cvar_SetROM (&mapgroupname, "");

	if (loc_loaded.value) 
		Loc_UnLoadLocFile();
	
	if (cl_loadlocs.value) {
		Loc_LoadLocFile(mapname.string);
		if (!loc_loaded.value && *mapgroupname.string)
			Loc_LoadLocFile(mapgroupname.string);
	}

	CL_ExecTriggerSafe ("f_mapstart");

	V_TF_ClearGrenadeEffects ();
	first_respawn_done = false;

}

/*
returns a combination of these values:
0 -- unknown (probably generated by the server)
1 -- normal
2 -- team message
4 -- spectator
8 -- spec team message
Note that sometimes we can't be sure who really sent the message,  e.g. when there's a 
player "unnamed" in your team and "(unnamed)" in the enemy team. The result will be 3 (1+2)
*/

int TP_CategorizeMessage (char *s, int *offset) {
	int i, msglen, len, flags, tracknum;
	player_info_t	*player;
	char *name, *team;

	
	tracknum = -1;
	if (cl.spectator && (tracknum = Cam_TrackNum()) != -1)
		team = cl.players[tracknum].team;
	else if (!cl.spectator)
		team = cl.players[cl.playernum].team;

	flags = 0;
	*offset = 0;
	if (!(msglen = strlen(s)))
		return 0;

	for (i = 0, player = cl.players; i < MAX_CLIENTS; i++, player++)	{
		if (!player->name[0])
			continue;
		name = Info_ValueForKey (player->userinfo, "name");
		len = bound(0, strlen(name), 31);	
		// check messagemode1
		if (len + 2 <= msglen && s[len] == ':' && s[len + 1] == ' ' && !strncmp(name, s, len))	{
			if (player->spectator)
				flags |= 4;
			else
				flags |= 1;
			*offset = len + 2;
		}
		// check messagemode2
		else if (s[0] == '(' && len + 4 <= msglen && !strncmp(s + len + 1, "): ", 3) && !strncmp(name, s + 1, len)
		 
		 && (!cl.spectator || tracknum != -1)
		) {
			// no team messages in teamplay 0, except for our own
			if (i == cl.playernum || ( cl.teamplay && !strcmp(team, player->team)) )
				flags |= 2;
			*offset = len + 4;
		}
		//check spec mm2
		else if (cl.spectator && !strncmp(s, "[SPEC] ", 7) && player->spectator && 
		 len + 9 <= msglen && s[len + 7] == ':' && s[len + 8] == ' ' && !strncmp(name, s + 7, len)) {
			flags |= 8;
			*offset = len + 9;
		}
	}
	return flags;
}

//===================================================================
//							MAP GROUPS
//===================================================================

// fuh -->
#define MAX_GROUP_MEMBERS	36

typedef struct mapgroup_s {
	char groupname[MAX_QPATH];
	char members[MAX_GROUP_MEMBERS][MAX_QPATH];
	struct mapgroup_s *next, *prev;
	qboolean system;
	int nummembers;
} mapgroup_t;

static mapgroup_t *mapgroups = NULL;	
static mapgroup_t *last_system_mapgroup = NULL;
static qboolean mapgroups_init = false;	

#define FIRSTUSERGROUP (last_system_mapgroup ? last_system_mapgroup->next : mapgroups)

static mapgroup_t *GetGroupWithName(char *groupname) {
	mapgroup_t *node;

	for (node = mapgroups; node; node = node->next) {
		if (!Q_stricmp(node->groupname, groupname))
			return node;
	}
	return NULL;
}

static mapgroup_t *GetGroupWithMember(char *member) {
	int j;
	mapgroup_t *node;

	for (node = mapgroups; node; node = node->next) {
		for (j = 0; j < node->nummembers; j++) {
			if (!Q_stricmp(node->members[j], member))
				return node;
		}
	}
	return NULL;
}

static void DeleteMapGroup(mapgroup_t *group) {
	if (!group)
		return;

	if (group->prev)
		group->prev->next = group->next;
	if (group->next)
		group->next->prev = group->prev;

	
	if (group == mapgroups) {
		mapgroups = mapgroups->next;		
		if (group == last_system_mapgroup)
			last_system_mapgroup = NULL;
	} else if (group == last_system_mapgroup) {
		last_system_mapgroup = last_system_mapgroup->prev;
	}

	free(group);
}

static void ResetGroupMembers(mapgroup_t *group) {
	int i;

	if (!group)
		return;

	for (i = 0; i < group->nummembers; i++)
		group->members[i][0] = 0;

	group->nummembers = 0;
}

static void DeleteGroupMember(mapgroup_t *group, char *member) {
	int i;

	if (!group)
		return;

	for (i = 0; i < group->nummembers; i++) {
		if (!Q_stricmp(member, group->members[i]))
			break;
	}

	if (i == group->nummembers)	
		return;

	if (i < group->nummembers - 1)
		memmove(group->members[i], group->members[i + 1], (group->nummembers - 1 - i) * sizeof(group->members[0]));

	group->nummembers--;
}

static void AddGroupMember(mapgroup_t *group, char *member) {
	int i;

	if (!group || group->nummembers == MAX_GROUP_MEMBERS)
		return;

	for (i = 0; i < group->nummembers; i++) {		
		if (!Q_stricmp(member, group->members[i]))
			return;
	}

	strlcpy(group->members[group->nummembers], member, sizeof(group->members[group->nummembers]));
	group->nummembers++;
}

void TP_MapGroup_f(void) {
	int i, c, j;
	qboolean removeflag = false;
	mapgroup_t *node, *group, *tempnode;
	char *groupname, *member;

	if ((c = Cmd_Argc()) == 1) {		
		if (!FIRSTUSERGROUP) {
			Con_Printf("No map groups defined\n");
		} else {
			for (node = FIRSTUSERGROUP; node; node = node->next) {
				Con_Printf("\x02%s: ", node->groupname);
				for (j = 0; j < node->nummembers; j++)
					Con_Printf("%s ", node->members[j]);
				Con_Printf("\n");
			}
		}
		return;
	}

	groupname = Cmd_Argv(1);

	if (c == 2 && !Q_stricmp(groupname, "clear")) {	
		for (node = FIRSTUSERGROUP; node; node = tempnode) {
			tempnode = node->next;
			DeleteMapGroup(node);
		}
		return;
	}

	if (Util_Is_Valid_Filename(groupname) == false) {
		Con_Printf("Error: %s is not a valid map group name\n", groupname);
		return;
	}

	group = GetGroupWithName(groupname);

	if (c == 2) {	
		if (!group) {
			Con_Printf("No map group named \"%s\"\n", groupname);
		} else {
			Con_Printf("\x02%s: ", groupname);
			for (j = 0; j < group->nummembers; j++)
				Con_Printf("%s ", group->members[j]);
			Con_Printf("\n");
		}
		return;
	}

	if (group && group->system) {
		Con_Printf("Cannot modify system group \"%s\"\n", groupname);
		return;
	}

	if (c == 3 && !Q_stricmp(Cmd_Argv(2), "clear")) {	
		if (!group)
			Con_Printf("\"%s\" is not a map group name\n", groupname);
		else
			DeleteMapGroup(group);
		return;
	}

	

	if (!group) {	
		group = calloc(1, sizeof(mapgroup_t));
		strlcpy(group->groupname, groupname, sizeof(group->groupname));
		group->system = !mapgroups_init;
		if (mapgroups) {	
			for (tempnode = mapgroups; tempnode->next; tempnode = tempnode->next)
				;
			tempnode->next = group;
			group->prev = tempnode;
		} else {
			mapgroups = group;
		}
	} else {		
		member = Cmd_Argv(2);
		if (member[0] != '+' && member[0] != '-')
			ResetGroupMembers(group);
	}

	for (i = 2; i < c; i++) {
		member = Cmd_Argv(i);
		if (member[0] == '+') {
			removeflag = false;
			member++;
		} else if (member[0] == '-') {
			removeflag = true;
			member++;
		}

		if (!removeflag && (tempnode = GetGroupWithMember(member)) && tempnode != group) {
			if (cl_warncmd.value || developer.value)
				Con_Printf("Warning: \"%s\" is already a member of group \"%s\"...ignoring\n", member, tempnode->groupname);
			continue;
		}

		if (removeflag)
			DeleteGroupMember(group, member);
		else
			AddGroupMember(group, member);
	}

	if (!group->nummembers)	
		DeleteMapGroup(group);
}

static void TP_AddMapGroups(void) {
	char exmy_group[256] = {0}, exmy_map[6] = {'e', 0, 'm', 0, ' ', 0};
	int i, j;
	mapgroup_t *tempnode;

	strcat(exmy_group, "mapgroup exmy start ");
	for (i = 1; i <= 4; i++) {
		for (j = 1; j <= 8; j++) {		
			exmy_map[1] = i + '0';
			exmy_map[3] = j + '0';
			strcat(exmy_group, exmy_map);
		}
	}
	Cmd_TokenizeString(exmy_group);
	TP_MapGroup_f();
	for (tempnode = mapgroups; tempnode->next; tempnode = tempnode->next)
		;
	last_system_mapgroup = tempnode;
	mapgroups_init = true;
}

char *TP_GetMapGroupName(char *mapname, qboolean *system) {
	mapgroup_t *group;

	group = GetGroupWithMember(mapname);
	if (group && strcmp(mapname, group->groupname)) {
		if (system)
			*system = group->system;
		return group->groupname;
	} else {
		return NULL;
	}
}

/*void DumpMapGroups(FILE *f) {
	mapgroup_t *node;
	int j;

	if (!FIRSTUSERGROUP) {
		fprintf(f, "mapgroup clear\n");
		return;
	}
	for (node = FIRSTUSERGROUP; node; node = node->next) {
		fprintf(f, "mapgroup %s ", node->groupname);
		for (j = 0; j < node->nummembers; j++)
			fprintf(f, "%s ", node->members[j]);
		fprintf(f, "\n");
	}
}*/
// <-- fuh

/*********************************** MACROS ***********************************/

cvar_t	tp_name_armortype_ra = {"tp_name_armortype_ra", "r"};
cvar_t	tp_name_armortype_ya = {"tp_name_armortype_ya", "y"};
cvar_t	tp_name_armortype_ga = {"tp_name_armortype_ga", "g"};
cvar_t	tp_name_none = {"tp_name_none", ""};
cvar_t	tp_name_ra = {"tp_name_ra", "ra"};
cvar_t	tp_name_ya = {"tp_name_ya", "ya"};
cvar_t	tp_name_ga = {"tp_name_ga", "ga"};
cvar_t	tp_name_flag = {"tp_name_flag", "flag"};
cvar_t	tp_name_quad = {"tp_name_quad", "quad"};
cvar_t	tp_name_pent = {"tp_name_pent", "pent"};
cvar_t	tp_name_ring = {"tp_name_ring", "ring"};
cvar_t	tp_name_suit = {"tp_name_suit", "suit"};
cvar_t	tp_name_separator = {"tp_name_separator", "/"};		

#define MAX_MACRO_VALUE	256
static char	macro_buf[MAX_MACRO_VALUE] = "";

char *Macro_Quote_f (void) {
	return "\"";
}

char *Macro_Latency (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", Q_rint(cls.latency * 1000));
	return macro_buf;
}

char *Macro_Health (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_HEALTH]);
	return macro_buf;
}

char *Macro_Armor (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_ARMOR]);
	return macro_buf;
}

char *Macro_ArmorType (void) {
	if (cl.stats[STAT_ITEMS] & IT_ARMOR1)
		return tp_name_armortype_ga.string;
	else if (cl.stats[STAT_ITEMS] & IT_ARMOR2)
		return tp_name_armortype_ya.string;
	else if (cl.stats[STAT_ITEMS] & IT_ARMOR3)
		return tp_name_armortype_ra.string;
	else
		return tp_name_none.string;
}

char *Macro_Shells (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_SHELLS]);
	return macro_buf;
}

char *Macro_Nails (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_NAILS]);
	return macro_buf;
}

char *Macro_Rockets (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_ROCKETS]);
	return macro_buf;
}

char *Macro_Cells (void) {
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%i", cl.stats[STAT_CELLS]);
	return macro_buf;
}

char *Macro_Powerups (void) {
	int effects;

	macro_buf[0] = 0;

	if (cl.stats[STAT_ITEMS] & IT_QUAD)
		strlcpy(macro_buf, tp_name_quad.string, sizeof(macro_buf));

	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY) {
		if (macro_buf[0])
			strlcat(macro_buf, tp_name_separator.string, sizeof(macro_buf));
		strlcat(macro_buf, tp_name_pent.string, sizeof(macro_buf));
	}

	if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY) {
		if (macro_buf[0])
			strlcat(macro_buf, tp_name_separator.string, sizeof(macro_buf));
		strlcat(macro_buf, tp_name_ring.string, sizeof(macro_buf));
	}

	effects = cl.frames[cl.parsecount & UPDATE_MASK].playerstate[cl.playernum].effects;
	if ( (effects & (EF_FLAG1|EF_FLAG2)) /* CTF */ ||		
		(cl.teamfortress && cl.stats[STAT_ITEMS] & (IT_KEY1|IT_KEY2)) /* TF */ ) {
		if (macro_buf[0])
			strlcat(macro_buf, tp_name_separator.string, sizeof(macro_buf));
		strlcat(macro_buf, tp_name_flag.string, sizeof(macro_buf));
	}

	if (!macro_buf[0])			
		strlcpy(macro_buf, tp_name_none.string, sizeof(macro_buf));

	return macro_buf;
}

// BorisU -->
char *Macro_Blue (void) {
	return cl_blueteam.string;
}

char *Macro_Red (void) {
	return cl_redteam.string;
}

char *Macro_Yellow (void) {
	return cl_yellowteam.string;
}

char *Macro_Green (void) {
	return cl_greenteam.string;
}

char *Macro_NetIn (void) {
	return va("%.2lfM",(double)net_traffic_in/(1024.0*1024.0));
}

char *Macro_NetOut (void) {
	return va("%.2lfM",(double)net_traffic_out/(1024.0*1024.0));
}
// <-- BorisU

// Note: longer macro names like "armortype" must be defined
// _before_ the shorter ones like "armor" to be parsed properly
void TP_AddMacros(void) {
	Cmd_AddMacro("qt", Macro_Quote_f);

	Cmd_AddMacro("latency", Macro_Latency);
	Cmd_AddMacro("health", Macro_Health);
	Cmd_AddMacro("armortype", Macro_ArmorType);
	Cmd_AddMacro("armor", Macro_Armor);

	Cmd_AddMacro("shells", Macro_Shells);
	Cmd_AddMacro("nails", Macro_Nails);
	Cmd_AddMacro("rockets", Macro_Rockets);
	Cmd_AddMacro("cells", Macro_Cells);

	Cmd_AddMacro("powerups", Macro_Powerups);

	// Team names
	Cmd_AddMacro("blue", Macro_Blue);
	Cmd_AddMacro("red", Macro_Red);
	Cmd_AddMacro("yell", Macro_Yellow);
	Cmd_AddMacro("green", Macro_Green);

	Cmd_AddMacro("net_in", Macro_NetIn);
	Cmd_AddMacro("net_out", Macro_NetOut);
};

void TP_Init (void)
{
	TP_AddMacros();

	TP_AddMapGroups();
	Cmd_AddCommand ("mapgroup", TP_MapGroup_f);
}
