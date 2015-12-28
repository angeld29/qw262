#include "quakedef.h"

cvar_t	cl_triggers			= {"cl_triggers", "1"};
cvar_t	critical_health		= {"critical_health", "0"};
cvar_t	critical_armor		= {"critical_armor", "0"};
cvar_t	cl_currentweapon	= {"cl_currentweapon", "0"};
//cvar_t	cl_lastid			= {"last_id_nick", "", CVAR_ROM};
cvar_t	cl_setinfo_triggers	= {"cl_setinfo_triggers", ""};
cvar_t	cl_coords2loc		= {"cl_coords2loc", "0"};

cvar_t	cl_f_took_hack		= {"cl_f_took_hack", "1"};

cvar_t	re_sub[10]	= {	{"0", "", CVAR_INTERNAL},
						{"1", "", CVAR_INTERNAL},
						{"2", "", CVAR_INTERNAL},
						{"3", "", CVAR_INTERNAL},
						{"4", "", CVAR_INTERNAL},
						{"5", "", CVAR_INTERNAL},
						{"6", "", CVAR_INTERNAL},
						{"7", "", CVAR_INTERNAL},
						{"8", "", CVAR_INTERNAL},
						{"9", "", CVAR_INTERNAL}
 };

cvar_t	re_subi[10]	= {	{"internal0"},
						{"internal1"},
						{"internal2"},
						{"internal3"},
						{"internal4"},
						{"internal5"},
						{"internal6"},
						{"internal7"},
						{"internal8"},
						{"internal9"}
 };

static pcre_trigger_t			*re_triggers;
static pcre_internal_trigger_t	*internal_triggers;

double	last_respawntime = 0;
double	last_deathtime = 0;
double	last_gotammotime = 0;

qboolean	first_respawn_done;
qboolean	broken_maps_flag = false;

// for triggers that are called during gameplay
// trigger limitations are ON
void CL_ExecTrigger (char *s)
{
	if ((cls.demoplayback && cl_demoplay_restrictions.value) || 
		(cl.spectator && cl_spectator_restrictions.value) || 
		!cl_triggers.value || (cl.fpd & FPD_NO_SOUNDTRIGGERS)) return;

	Con_DPrintf("CL_ExecTrigger: %s\n",s);
	if (Cmd_FindAlias(s)) {
		Cbuf_InsertTextEx(&cbuf_trig,"\n");
		Cbuf_InsertTextEx(&cbuf_trig,s);
		Cbuf_ExecuteEx(&cbuf_trig);
	}
}

// trigger limitations are OFF
void CL_ExecTriggerSafe (char *s)
{
	if ((cls.demoplayback && cl_demoplay_restrictions.value) || 
		(cl.spectator && cl_spectator_restrictions.value) || 
		!cl_triggers.value || (cl.fpd & FPD_NO_SOUNDTRIGGERS)) return;

	Con_DPrintf("CL_ExecTriggerSafe: %s\n",s);
	if (Cmd_FindAlias(s)) {
		Cbuf_AddTextEx (&cbuf_main, s);
		Cbuf_AddTextEx (&cbuf_main, "\n");
		// maybe call Cbuf_Execute?
	}
}

void CL_ExecTriggerSetinfo (char *s, char *param1, char *param2, char *param3)
{
	char buf[256];
	
	if ((cls.demoplayback && cl_demoplay_restrictions.value) || 
		(cl.spectator && cl_spectator_restrictions.value) || 
		!cl_triggers.value || (cl.fpd & FPD_NO_SOUNDTRIGGERS)) return;
	
	if (Cmd_FindAlias(s)) {
		strcpy(buf,s);
		strcat(buf, " \"");
		strcat(buf, param1);
		strcat(buf, "\" \"");
		strcat(buf, param2);
		strcat(buf, "\" ");
		strcat(buf, param3);
		strcat(buf, "\n");

		Cbuf_InsertTextEx(&cbuf_trig,buf);
		Cbuf_ExecuteEx(&cbuf_trig);
	}
}

void CL_StatChanged (int stat, int value)
{
	int i;

	switch (stat) {
	case STAT_HEALTH:
		if (value > 0) {
			if (cl.stats[stat] <= 0 ) {
				last_respawntime = cls.realtime;
				//if (cl.teamfortress)
					memset (&cshift_empty, 0, sizeof(cshift_empty));
					if (!cl.teamfortress || first_respawn_done) {
						concussioned = false;
						CL_ExecTriggerSafe ("f_respawn");
					}
			} else {
				if (value < critical_health.value 
						&& cl.stats[stat] >= critical_health.value) {
					cl.stats[stat] = value;
					CL_ExecTrigger ("f_low_health");
				}
				if (cl.teamfortress && !first_respawn_done /*&& 
					cl.stats[STAT_HEALTH] == 1 && cl.stats[STAT_ARMOR]== 0*/) {
					first_respawn_done = true;
					broken_maps_flag = false;
					CL_ExecTriggerSafe ("f_respawn");
				}
			}
			return;
		}
		if (cl.stats[stat] > 0) {		// We just died
			PlaceOfLastDeath[0]=cl.simorg[0];
			PlaceOfLastDeath[1]=cl.simorg[1];
			PlaceOfLastDeath[2]=cl.simorg[2];
			last_deathtime = cls.realtime;
			CL_ExecTriggerSafe ("f_death");
			if (broken_maps_flag || (cl.stats[STAT_ITEMS] & (IT_KEY1|IT_KEY2))) {
				CL_ExecTrigger ("f_drop");
				broken_maps_flag = false;
			}
		}
		break;
	case STAT_ARMOR:
		if (value < critical_armor.value 
				&& cl.stats[stat] >= critical_armor.value) {
			cl.stats[stat] = value;
			CL_ExecTrigger ("f_low_armor");
		}
		break;
	case STAT_ITEMS:
		i = value & ~cl.stats[stat];
		if ( (i & IT_KEY1) || (i & IT_KEY2) ) 
			CL_ExecTrigger ("f_took");
//		i = cl.stats[stat] & ~value;
//		if ( ((i & IT_KEY1) || (i & IT_KEY2)) && last_deathtime == cls.realtime){
//			CL_ExecTrigger ("f_drop");
//		}
		break;
	case STAT_SHELLS:
	case STAT_NAILS:
	case STAT_ROCKETS:
	case STAT_CELLS:
		if (cl.stats[stat] < value && last_gotammotime != cls.realtime ) { // do not trigger twice 
			CL_ExecTrigger ("f_got_ammo");
			last_gotammotime = cls.realtime;
		}
		break;
	case STAT_WEAPON:
//		Con_Printf("stat_weapon:%d %s %d\n",value, cl.model_precache[value]->name, cl.model_precache[value]->name?Hash(cl.model_precache[value]->name):0);
		if (!cl.model_precache[value]->name)
			i = 0; // dead or reload in TF
		else {
			switch (Hash(cl.model_precache[value]->name)) {
			// Attention: need to be changed if Hash function changed
			case 229: // v_axe.mdl		axe
			case 211: // v_medi.mdl		medikit
			case 102: // v_knife.mdl	knife
			case 214: // v_span.mdl		spanner
				i = 1; break;
			case 176: // v_shot.mdl		shotgun
			case 155: // v_rail.mdl		railgun
			case 218: // v_srifle.mdl	sniper rifle
			case 205: // v_tgun.mdl		tranquiliser gun in some TF modifications
				i = 2; break;
			case 149: // v_shot2.mdl	super shotgun
				i = 3; break;
			case 71:  // v_nail.mdl		nailgun
				i = 4; break;
			case 136: // v_nail2.mdl	super nailgun
				i = 5; break;
			case 145: // v_rock.mdl		grenade/pipe launcher, flamethrower
			case 161: // v_flame.mdl	flamethrower in some TF modifications
				i = 6; break;
			case 250: // v_rock2.mdl	rocket launcher, incendiary cannon
			case 207: // v_asscan.mdl	assault cannon
			case 30:  // v_pipe.mdl		pipe/launcher in some TF modifications
				i = 7; break;
			case 202: // v_light.mdl	lightning
				i = 8; break;
			default:
				i = 0;
			}
		}
		Cvar_SetValue (&cl_currentweapon, i);
//		Con_Printf(">WPN: %i %lf\n",i, cls.realtime);
		CL_ExecTrigger ("f_weapon_change");	
		break;
	}
}

typedef void ReTrigger_func(pcre_trigger_t *);

void Trig_ReSearch_do(ReTrigger_func f)
{
	pcre_trigger_t *trig;	
	
	for(trig=re_triggers; trig; trig = trig->next) {
		if (ReSearchMatch(trig->name))
			f(trig);
	}
}

static pcre_trigger_t *prev;
pcre_trigger_t *CL_FindReTrigger (char *name)
{
	pcre_trigger_t *t;
	
	prev=NULL;
	for (t=re_triggers; t; t=t->next) {
		if (!strcmp(t->name, name))
			return t;
		prev = t;
	}
		return NULL;
}

static void DeleteReTrigger(pcre_trigger_t *t)
{
	if (t->regexp) (pcre_free)(t->regexp);
	if (t->regexp_extra) (pcre_free)(t->regexp_extra);
	if (t->regexpstr) Z_Free(t->regexpstr);
	Z_Free(t->name);
	Z_Free(t);
}

static void RemoveReTrigger(pcre_trigger_t *t)
{
// remove from list
	if (prev)
		prev->next = t->next;
	else
		re_triggers = t->next;
// free memory
	DeleteReTrigger(t);
}

void CL_RE_Trigger_f (void)
{
	int			c,i,m;
	char			*name;
	char			*regexpstr;
	pcre_trigger_t	*trig;
	pcre			*re;
	pcre_extra		*re_extra;
	const char		*error;
	int			error_offset;
	qboolean		newtrigger=false;
	qboolean		re_search = false;

	c = Cmd_Argc();
	if (c > 3) {
		Con_Printf ("re_trigger <trigger name> <regexp>\n");
		return;
	}

	if (c == 2 && IsRegexp(Cmd_Argv(1))) {
		re_search = true;
	}

	if (c == 1 || re_search) {
		if (!re_triggers)
			Con_Printf ("no regexp_triggers defined\n");
		else {
			if (re_search && !ReSearchInit(Cmd_Argv(1)))
				return;
			Con_Printf ("List of re_triggers:\n");
			for (trig=re_triggers, i=m=0; trig; trig=trig->next, i++)
				if (!re_search || ReSearchMatch(trig->name)) {
					Con_Printf ("%s : \"%s\" : %d\n", trig->name, trig->regexpstr, trig->counter);
					m++;
				}
			Con_Printf ("------------\n%i/%i re_triggers\n", m, i);
			if (re_search)
				ReSearchDone();
		}
		return;
	}

	name = Cmd_Argv(1);
	trig = CL_FindReTrigger (name);

	if (c == 2) {
		
		if (trig) {
			Con_Printf ("%s: \"%s\"\n", trig->name, trig->regexpstr);
			Con_Printf ("  options: mask=%d interval=%g%s%s%s%s%s\n", trig->flags & 0xFF,
				trig->min_interval,
				trig->flags & RE_FINAL ? " final" : "",
				trig->flags & RE_REMOVESTR ? " remove" : "",
				trig->flags & RE_NOLOG ? " nolog" : "",
				trig->flags & RE_ENABLED ? "" : " disabled",
				trig->flags & RE_NOACTION ? " noaction" : ""
				 );
			Con_Printf ("  matched %d times\n", trig->counter);
		}
		else
			Con_Printf ("re_trigger \"%s\" not found\n", name);
		return;
	}

	if (c == 3) {
		regexpstr = Cmd_Argv(2);
		if (!trig) {
			// allocate new trigger
			newtrigger = true;
			trig = Z_Malloc (sizeof(pcre_trigger_t));
			trig->next = re_triggers;
			re_triggers = trig;
			trig->name = Z_StrDup (name);
			trig->flags = RE_PRINT_ALL | RE_ENABLED; // catch all printed messages by default
		}

		error = NULL;
		if( (re = pcre_compile(regexpstr, 0, &error, &error_offset, NULL)) ) {
			error = NULL;
			re_extra = pcre_study(re, 0, &error);
			if (error)
				Con_Printf ("Regexp study error: %s\n", &error);
			else {
				if (!newtrigger) {
					(pcre_free)(trig->regexp);
					if (trig->regexp_extra)
						(pcre_free)(trig->regexp_extra);
					Z_Free(trig->regexpstr);
				}
				trig->regexpstr = Z_StrDup (regexpstr);
				trig->regexp = re;
				trig->regexp_extra = re_extra;
				return;
			}
		} else {
			Con_Printf ("Invalid regexp: %s\n", error);
		}
		
		prev = NULL;
		RemoveReTrigger(trig);
	}
}

void CL_RE_Trigger_Options_f (void)
{
	int		c,i;
	char*	name;
	pcre_trigger_t	*trig;

	c = Cmd_Argc();
	if (c < 3) {
		Con_Printf ("re_trigger_options <trigger name> <option1> <option2>\n");
		return;
	}

	name = Cmd_Argv(1);
	trig = CL_FindReTrigger (name);

	if (!trig) {
		Con_Printf ("re_trigger \"%s\" not found\n", name);
		return;
	}

	for(i=2; i<c; i++) {
		if ( !strcmp(Cmd_Argv(i), "final") )
			trig->flags |= RE_FINAL;
		else if ( !strcmp(Cmd_Argv(i), "remove") )
			trig->flags |= RE_REMOVESTR;
		else if ( !strcmp(Cmd_Argv(i), "notfinal") )
			trig->flags &= ~RE_FINAL;
		else if ( !strcmp(Cmd_Argv(i), "noremove") )
			trig->flags &= ~RE_REMOVESTR;
		else if ( !strcmp(Cmd_Argv(i), "mask") ) {
			trig->flags &= ~0xFF;
			trig->flags |= 0xFF & atoi(Cmd_Argv(i+1));
			i++;
			}
		else if ( !strcmp(Cmd_Argv(i), "interval") ) {
			trig->min_interval = atof(Cmd_Argv(i+1));
			i++;
			}
		else if ( !strcmp(Cmd_Argv(i), "enable") ) 
			trig->flags |= RE_ENABLED;
		else if ( !strcmp(Cmd_Argv(i), "disable") ) 
			trig->flags &= ~RE_ENABLED;
		else if ( !strcmp(Cmd_Argv(i), "noaction") ) 
			trig->flags |= RE_NOACTION;
		else if ( !strcmp(Cmd_Argv(i), "action") ) 
			trig->flags &= ~RE_NOACTION;
		else if ( !strcmp(Cmd_Argv(i), "nolog") )
			trig->flags |= RE_NOLOG;
		else if ( !strcmp(Cmd_Argv(i), "log") )
			trig->flags &= ~RE_NOLOG;

		else 
			Con_Printf("re_trigger_options: invalid option.\n"
			"valid options:\n  final\n  notfinal\n  remove\n"
			"  noremove\n  mask <trigger_mask>\n  interval <min_interval>)\n"
			"  enable\n  disable\n  noaction\n  action\n  nolog\n  log\n");
	}
}

void CL_RE_Trigger_Delete_f (void)
{
	pcre_trigger_t	*trig, *next_trig;
	char			*name;
	int				i;
	
	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			prev = NULL;
			for(trig=re_triggers; trig; ) {
				if (ReSearchMatch(trig->name)) {
					next_trig = trig->next;
					RemoveReTrigger(trig);
					trig = next_trig;
				} else {
					prev = trig;
					trig = trig->next;
				}
			}
			ReSearchDone();
		} else {
			if ((trig = CL_FindReTrigger(name)))
				RemoveReTrigger(trig);
		}
	}
}

void Trig_Enable(pcre_trigger_t	*trig)
{
	trig->flags |= RE_ENABLED;
}

void CL_RE_Trigger_Enable_f (void)
{
	pcre_trigger_t	*trig;
	char			*name;
	int				i;
	
	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			Trig_ReSearch_do(Trig_Enable);
			ReSearchDone();
		} else {
			if ((trig = CL_FindReTrigger(name)))
				Trig_Enable(trig);	
		}
	}
}

void Trig_Disable(pcre_trigger_t	*trig)
{
	trig->flags &= ~RE_ENABLED;
}

void CL_RE_Trigger_Disable_f (void)
{
	pcre_trigger_t	*trig;
	char			*name;
	int				i;
	
	for (i=1; i<Cmd_Argc(); i++) {	
		name = Cmd_Argv(i);
		if (IsRegexp(name)) {
			if(!ReSearchInit(name))
				return;
			Trig_ReSearch_do(Trig_Disable);
			ReSearchDone();
		} else {
			if ((trig = CL_FindReTrigger(name)))
				Trig_Disable(trig);	
		}
	}
}

void CL_RE_Trigger_ResetLasttime (void)
{
	pcre_trigger_t	*trig;

	for (trig=re_triggers; trig; trig=trig->next)
		trig->lasttime = 0.0;
}

float	impulse_time = -9999;
int		impulse_counter;

cmd_function_t		*impulse_cmd;
qboolean AllowedImpulse(int imp)
{

	static int AllowedImpulses[] = {
		135, 99, 101, 102, 103, 104, 105, 106, 107, 108, 109, 23, 144, 145
	};

	int i;

	if (!cl.teamfortress) return false;
	for (i=0; i<sizeof(AllowedImpulses)/sizeof(AllowedImpulses[0]); i++) {
		if (AllowedImpulses[i] == imp) {
			if(++impulse_counter >= 30) {
				if (cls.realtime < impulse_time + 5 && !cls.demoplayback) {
					return false;
				}
				impulse_time = cls.realtime;
				impulse_counter = 0;
			}
			return true;
		}
	}
	return false;
}

void Re_Trigger_Copy_Subpatterns(char *s, int* offsets, int num, cvar_t	*re_sub)
{
	int		i;
	char	tmp;

	for (i=0;i<2*num;i+=2) {
		tmp = s[offsets[i+1]];
		s[offsets[i+1]] = '\0';
		Cvar_Set(&re_sub[i/2],s+offsets[i]);
		s[offsets[i+1]] = tmp;
	}
}

void CL_RE_Trigger_Match_f (void)
{
	int				c;
	char			*tr_name;
	char			*s;
	pcre_trigger_t	*rt;
	char			*string;	
	int result;
	int offsets[99];
	
	c = Cmd_Argc();

	if (c != 3) {
		Con_Printf ("re_trigger_match <trigger name> <string>\n");
		return;
	}

	tr_name = Cmd_Argv(1);
	s = Cmd_Argv(2);

	for (rt=re_triggers; rt; rt=rt->next) 
		if ( !strcmp(rt->name, tr_name) ) {
			result = pcre_exec(rt->regexp, rt->regexp_extra, s, strlen(s), 0, 0, offsets, 99);
			if (result>=0) {
				rt->lasttime = cls.realtime;
				rt->counter++;
				Re_Trigger_Copy_Subpatterns(s, offsets, min(result,10), re_sub);
				if (!(rt->flags & RE_NOACTION)) {
					string = Cmd_AliasString (rt->name);
					if (string) {
						Cbuf_InsertTextEx(&cbuf_trig,"\nwait\n");
						Cbuf_InsertTextEx(&cbuf_trig,string);
						Cbuf_ExecuteEx(&cbuf_trig);
					} else
						Con_Printf ("re_trigger \"%s\" has no matching alias\n", rt->name);
				}
				
			}
			return;
		} 
	Con_Printf ("re_trigger \"%s\" not found\n", tr_name);
}

qboolean allow_re_triggers;

qboolean CL_SearchForMsgTriggers (char *s, unsigned trigger_type)
{
	pcre_trigger_t			*rt;
	pcre_internal_trigger_t	*irt;
	cmdalias_t				*trig_alias;
	qboolean				removestr = false;
	int						result;
	int						offsets[99];
	int						len;
	
	if ((cls.demoplayback && cl_demoplay_restrictions.value) || 
		(cl.spectator && cl_spectator_restrictions.value) || (cl.fpd & FPD_NO_SOUNDTRIGGERS)) 
		return false;

	len = strlen(s);
// internal triggers
	if (cl.teamfortress && trigger_type < RE_PRINT_ECHO) {
		allow_re_triggers = true;
		for (irt=internal_triggers; irt; irt=irt->next) {
				if (irt->flags & trigger_type) {
				result = pcre_exec(irt->regexp, irt->regexp_extra, s, len, 0, 0, offsets, 99);
				if (result>=0) {
					Re_Trigger_Copy_Subpatterns(s, offsets, min(result,10), re_subi);
					irt->func(s);
				}
			}
		}
		if (!allow_re_triggers) return false;
	}
	
// regexp triggers
	for (rt=re_triggers; rt; rt=rt->next) 
		if ( (rt->flags & RE_ENABLED) &&					// enabled
			 (rt->flags & trigger_type) &&					// mask fits
			 rt->regexp &&									// regexp not empty
			 (rt->min_interval == 0.0 ||
			 cls.realtime >= rt->min_interval + rt->lasttime))	// not too fast
			{
			result = pcre_exec(rt->regexp, rt->regexp_extra, s, len, 0, 0, offsets, 99);
			if (result>=0) {
				rt->lasttime = cls.realtime;
				rt->counter++;
				Re_Trigger_Copy_Subpatterns(s, offsets, min(result,10), re_sub);
				if (!(rt->flags & RE_NOACTION)) {
					trig_alias = Cmd_FindAlias (rt->name);
					Print_current++;
					if (trig_alias) {
						Cbuf_InsertTextEx(&cbuf_trig,"\nwait\n");
						Cbuf_InsertTextEx(&cbuf_trig,rt->name);
						Cbuf_ExecuteEx(&cbuf_trig);
					} else
						Con_Printf ("re_trigger \"%s\" has no matching alias\n", rt->name);
					Print_current--;
				}
				if (rt->flags & RE_REMOVESTR) 
					removestr = true;
				if (rt->flags & RE_NOLOG) 
					Print_flags[Print_current] |= PR_LOG_SKIP;
				if (rt->flags & RE_FINAL) 
					break;
			}
		} 

	if (removestr)
		Print_flags[Print_current] |= PR_SKIP;

	return removestr;
}

// Internal triggers
void AddInternalTrigger(char* regexpstr, unsigned mask, internal_trigger_func func)
{
	pcre_internal_trigger_t	*trig;
	const char		*error;
	int			error_offset;

	trig = Z_Malloc (sizeof(pcre_internal_trigger_t));
	trig->next = internal_triggers;
	internal_triggers = trig;

	trig->regexp = pcre_compile(regexpstr, 0, &error, &error_offset, NULL);
	trig->regexp_extra = pcre_study(trig->regexp, 0, &error);
	trig->func = func;
	trig->flags = mask;
}

void TF_id_trig(char *s)
{
	if (!strcmp(re_subi[2].string, "Enemy")) {
		allow_re_triggers = false;
	}
}

void TF_Disable_trig(char *s)
{
	allow_re_triggers = false;
	Print_flags[Print_current] |= PR_LOG_SKIP;
}

void TF_Coords_trig(char *s)
{
	char	*b;
	int		len;
	vec3_t	pos;
	float	old_cl_locmessage;
	char buf[SAY_TEAM_CHAT_BUFFER_SIZE];

	if (!cl_coords2loc.value)
		return;

	len = strlen(re_subi[0].string);
	b = strstr(s,re_subi[0].string);
	strcpy(buf,b+len);
	pos[0] = atoi(re_subi[1].string);
	pos[1] = atoi(re_subi[2].string);
	pos[2] = atoi(re_subi[3].string);

	old_cl_locmessage = cl_locmessage.value;
	cl_locmessage.value = 1;
	b=InsertLocation(&pos,b,SAY_TEAM_CHAT_BUFFER_SIZE-(b-s),false);
	cl_locmessage.value = old_cl_locmessage;
	strcpy(b,buf);
}

void TF_oppose1_f_took(char *s)
{
	if ( strcmp (mapname.string,"oppose1") || !cl_f_took_hack.value )
		return;
	CL_ExecTrigger ("f_took");
	broken_maps_flag = true;
}

void TF_broken_maps_capture(char *s)
{
	broken_maps_flag = false;
}

void TF_dkeep2_f_took(char *s)
{
	if ( strcmp (mapname.string,"dkeep2") || !cl_f_took_hack.value )
		return;
	CL_ExecTrigger ("f_took");
	broken_maps_flag = true;
}

void TF_warehau1_f_took(char *s)
{
	if ( (strcmp (mapname.string,"warehau1") && strcmp (mapname.string,"wareh1r"))
		|| !cl_f_took_hack.value )
		return;
	CL_ExecTrigger ("f_took");
	broken_maps_flag = true;
}

void InitInternalTriggers(void)
{
	AddInternalTrigger("(?m)\\n\\n\\n\\n(.+)\\n(Friendly|Enemy)", 16, TF_id_trig);	// id command
	AddInternalTrigger("^(Location :|Angles   :)", 4, TF_Disable_trig);				// showloc command
	AddInternalTrigger("^Enemies are using your dispenser!$", 16, TF_Disable_trig);

	// auto-parsing coordinates in chat
	AddInternalTrigger("\\[(\\-?\\d{1,4}) (\\-?\\d{1,4}) (\\-?\\d{1,4})\\]", 8, TF_Coords_trig);

	// f_took and f_drop support for broken maps
	AddInternalTrigger("Return to Base Immediately", 16, TF_oppose1_f_took);
	AddInternalTrigger("Good Work Soldier", 16, TF_broken_maps_capture);

	AddInternalTrigger("Return it to your Keep quickly!", 16, TF_dkeep2_f_took);
	AddInternalTrigger("Capture it again!", 16, TF_broken_maps_capture);

	AddInternalTrigger("Return (it )?to your base", 16, TF_warehau1_f_took);
	AddInternalTrigger("WooHoo!  Score 10!", 16, TF_broken_maps_capture);

	
}

typedef void *(*pcre_malloc_type)(size_t);

void CL_InitTriggers()
{
	unsigned i;

	// Using zone for PCRE library memory allocation
	//
	pcre_malloc = (pcre_malloc_type)Z_Malloc;
	pcre_free = Z_Free;

	Cvar_RegisterVariable (&cl_triggers);
	Cvar_RegisterVariable (&critical_health);
	Cvar_RegisterVariable (&critical_armor);
	Cvar_RegisterVariable (&cl_currentweapon);
//	Cvar_RegisterVariable (&cl_lastid);
	Cvar_RegisterVariable (&cl_setinfo_triggers);
	Cvar_RegisterVariable (&cl_coords2loc);
	Cvar_RegisterVariable (&cl_f_took_hack);

	for(i=0;i<10;i++)
		Cvar_RegisterVariable (re_sub+i);

	Cmd_AddCommandTrig ("re_trigger", CL_RE_Trigger_f);
	Cmd_AddCommandTrig ("re_trigger_options", CL_RE_Trigger_Options_f);
	Cmd_AddCommandTrig ("re_trigger_delete", CL_RE_Trigger_Delete_f);
	Cmd_AddCommandTrig ("re_trigger_enable", CL_RE_Trigger_Enable_f);
	Cmd_AddCommandTrig ("re_trigger_disable", CL_RE_Trigger_Disable_f);
	Cmd_AddCommandTrig ("re_trigger_match", CL_RE_Trigger_Match_f);

	impulse_cmd = Cmd_FindCommand("impulse");
	InitInternalTriggers();
}
