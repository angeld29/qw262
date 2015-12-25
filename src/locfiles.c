#include "quakedef.h"

int			loc_loaded_type;

cvar_t loc_loaded = {"loc_loaded", "0", CVAR_ROM};
cvar_t cl_loadlocs = {"cl_loadlocs", "2"};
cvar_t cl_loctype = {"cl_loctype", "0"};
cvar_t cl_locnear = {"cl_locnear", "0"};
cvar_t cl_locmessage = {"cl_locmessage", "1"};
// Sergio --> (new locs)
cvar_t cl_blueteam = {"cl_blueteam", "blue"};
cvar_t cl_redteam = {"cl_redteam", "red"};
cvar_t cl_yellowteam = {"cl_yellowteam", "yellow"};
cvar_t cl_greenteam = {"cl_greenteam", "green"};
cvar_t loc_granularity = {"loc_granularity", "50"};
// <-- Sergio


// -=MD=- .loc
int		locpoints= 0;
int		mirror_x = 0;
int		mirror_y = 0;

int		loc_team = 0;
char	loc_teamnames[4][5];

char*			*locinfo;
player_loc_t	*location;

int				max_locmsgs;
int				max_locpoints;

int		loc_cur_block = -1;
int		loc_nearest_block = -1; // Sergio

// proxy .loc

locdata_t	*locdata;
int			loc_maxentries;
int			loc_numentries;

// common
vec3_t			PlaceOfLastDeath;

void Loc_Load262LocFile(char *filename)
{
	char	buf[32] = "exec ";
	float	old_warncmd = cl_warncmd.value;

	cl_warncmd.value=0;
	strcat(buf, filename);
	Cmd_ExecuteString(buf);
	cl_warncmd.value=old_warncmd;

	Cbuf_Execute ();

	if (locpoints) {
		Cvar_SetROM(&loc_loaded, "1");
		loc_loaded_type = 0;
	}
}

#define SKIPBLANKS(ptr) while (*ptr == ' ' || *ptr == 9 || *ptr == 13) ptr++
#define SKIPTOEOL(ptr) while (*ptr != 10 && *ptr == 0) ptr++

void Loc_LoadProxyLocFile(char *filename)
{
	char	*buf, *p;
	int		i, n, sign;
	int		line;
	int		nameindex;
	int		mark;

	mark = Hunk_LowMark ();
	buf = (char *) COM_LoadHunkFile (filename);

	if (!buf)
	{
		Con_Printf ("Could not load %s\n", filename);
		return;
	}

// Parse the whole file now

	loc_numentries = 0;

	p = buf;
	line = 1;

	while (1)
	{
//		while (*buf == ' ' || *buf == 9)
//			buf++;
		SKIPBLANKS(p);

		if (*p == 0)
			goto _endoffile;

		if (*p == 10 || (*p == '/' && p[1] == '/'))
		{
			p++;
			goto _endofline;
		}

		for (i = 0; i < 3; i++)
		{
			n = 0;
			sign = 1;
			while (1)
			{
				switch (*p++)
				{
				case ' ': case 9:
					goto _next;

				case '-':
					if (n)
					{
						Con_Printf ("Error in loc file on line #%i\n", line);
						SKIPTOEOL(p);		
						goto _endofline;
					}
					sign = -1;
					break;

				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					n = n*10 + (p[-1] - '0');
					break;

				default:	// including eol or eof
					Con_Printf ("Error in loc file on line #%i\n", line);
					SKIPTOEOL(p);		
					goto _endofline;
				}
			}
_next:
			n *= sign;
			locdata[loc_numentries].coord[i] = n;

			SKIPBLANKS(p);
		}


// Save the location's name
//
		nameindex = 0;

		while (1)
		{
			switch (*p)
			{
			case 13:
				p++;
				break;

			case 10: case 0:
				locdata[loc_numentries].name[nameindex] = 0;
				loc_numentries++;

				if (loc_numentries == loc_maxentries) {
					loc_maxentries*=2;
					locdata = realloc(locdata, loc_maxentries*sizeof(locdata_t));
				}
					

				// leave the 0 or 10 in buffer, so it is parsed properly
				goto _endofline;

			default:
				if (nameindex < MAX_LOC_NAME-1)
					locdata[loc_numentries].name[nameindex++] = *p;
				p++;
			}
		}
_endofline:
		line++;
	}
_endoffile:

	Hunk_FreeToLowMark (mark);

	Con_Printf ("Loaded %s (%i locations)\n", filename, loc_numentries);
	if (loc_numentries) {
		Cvar_SetROM(&loc_loaded, "1");
		loc_loaded_type = 1;
	}
}

void Loc_LoadLocFile(char *filename)
{
	int		i, type;
	char	locname[MAX_OSPATH];
	char	buf[MAX_OSPATH];
	
	if (strlen(filename) > 32) {
		Con_Printf("loadloc: locfile name is too long\n");
		return;
	}

	strcpy (locname, filename);
	type = (int)cl_loctype.value;
	i = 0;
	do {
		if (i != type)
			strcpy (buf, "locs/");
		else
			strcpy (buf, "loc/"); 
		strcat (buf, locname);
		COM_ForceExtension (buf, ".loc");
		if (i != type)
			Loc_LoadProxyLocFile(buf);
		else
			Loc_Load262LocFile(buf);
	} while (!loc_loaded.value && (int)cl_loadlocs.value == 2 && i++ == 0);
}

void Loc_UnLoad262LocFile(void)
{
	int		i;

	for(i=0;i<max_locmsgs;i++) 
		if(locinfo[i]) 
		{ 
			Z_Free((void *)locinfo[i]); 
			locinfo[i]=(char *)NULL; 
		}

// Sergio --> (new locs)
	for(i = 0; i < locpoints; i++)
		if(location[i].name)
		{
			Z_Free((void *)location[i].name);
			location[i].name = NULL;
		}
	Cvar_SetValue(&loc_granularity, 50); // default value
// <-- Sergio

	locpoints = 0;
	mirror_x = 0;
	mirror_y = 0;
	Cvar_SetROM(&loc_loaded, "0");
	loc_cur_block = -1;
}

void Loc_UnLoadProxyLocFile(void)
{
	Cvar_SetROM(&loc_loaded, "0");
	loc_numentries = 0;
}

void Loc_UnLoadLocFile(void)
{
	switch (loc_loaded_type) {
	case 0: Loc_UnLoad262LocFile(); break;
	case 1: Loc_UnLoadProxyLocFile(); break;
	}
}

double			last_locationtime = 0;
static char		*last_location = NULL;
static vec3_t	last_locationvec;

static 
char *GetLocation (vec3_t *l)
{
	if( loc_loaded_type == 0) 
		return Location262(l);
	else
		return LocationProxy(l);
}

char *InsertLocation(vec3_t *l, char *n, int m, qboolean own)
{
	int		len;
	char	buf[128];
	vec3_t	*loc_vec;
	char	*loc_msg;

	if (!loc_loaded.value) return n; // no loc - insert nothing

	if (own) {
		if (cls.realtime-last_locationtime > 0.15 
			|| last_deathtime > last_locationtime
			|| last_respawntime > last_locationtime ) {
			last_location = GetLocation(l);
			last_locationtime = cls.realtime;
			VectorCopy ((*l), last_locationvec) 
		}
		loc_vec = &last_locationvec;
		loc_msg = last_location;
	} else {
		loc_vec = l;
		loc_msg = GetLocation(l);
	}

	len = 0;
	if (!cl_locmessage.value) {
		len = sprintf (buf, "[%i %i %i]", (int)(*loc_vec)[0], (int)(*loc_vec)[1], (int)(*loc_vec)[2]);
	} else if (loc_msg) {
		Cmd_ExpandString(loc_msg, buf, sizeof(buf));
		len = strlen(buf);
	}

	if(len>m || len==0) 
		return n;
	else { 
		strcpy(n,buf); 
		return n+len; 
	}
}

extern cvar_t team;
char *Location262(vec3_t *l)
{
	int		i, min_i;
	int		dist, min_dist;
	char	*r = NULL;
	lvect3	loc;

// Changed by Sergio (new locs) -->
	loc.x = (int)((*l)[0]/loc_granularity.value);
	loc.y = (int)((*l)[1]/loc_granularity.value);
	loc.z = (int)((*l)[2]/loc_granularity.value);

#define x loc.x
#define y loc.y
#define z loc.z

	min_dist = -1;
	min_i = -1;

	// finding smaller block where we are
	for(i=0; i<locpoints; i++) {
		if(x >= location[i].x1 && x < location[i].x2 
			&& y >= location[i].y1 && y < location[i].y2 
			&& z >= location[i].z1 && z < location[i].z2 
			&& (min_dist < 0 || location[i].volume < min_dist)) {
			min_dist = location[i].volume;
			min_i = i;
		}
	}

	if(min_i < 0 && (cl_locnear.value || !loc_nearest_block)) {
		// finding nearest block
		min_dist = 10000000; // larger than any real distance
		for(i=0; i<locpoints; i++) {
			dist = max( abs(2*x-location[i].x1-location[i].x2) - abs(location[i].x1-location[i].x2),
				max( abs(2*y-location[i].y1-location[i].y2) - abs(location[i].y1-location[i].y2),
				abs(2*z-location[i].z1-location[i].z2) - abs(location[i].z1-location[i].z2) ) );
			if(dist<min_dist) {
				min_dist = dist;
				min_i = i;
			}
		}
	}

	if (min_i < 0) {
		if (!(r = locinfo[0])) // try to get default name from loc_msg 0
			r = "some place";
	} else
		if (!(r = location[min_i].name)) // check for name at first
			r = locinfo[location[min_i].msg[loc_team]]; // if fail get message

	if (!loc_nearest_block)
		loc_nearest_block = min_i;
// <-- Sergio

#undef x
#undef y
#undef z

	return r;
}

char *LocationProxy(vec3_t *l)
{
	int		i;
	int		min_num;
	vec_t	min_dist,dist;
	vec3_t	vec;
	vec3_t	org;

	for (i = 0; i < 3; i++)
		org[i] = 8*(*l)[i];

	min_num = 0;
	min_dist = 9999999;

	for (i = 0; i < loc_numentries; i++)
	{
//		Con_DPrintf ("%f %f %f: %s\n", locdata[i].coord[0], locdata[i].coord[1], locdata[i].coord[2], locdata[i].name);
		VectorSubtract (org, locdata[i].coord, vec);
		if ((dist = VectorLength(vec)) < min_dist)
		{
			min_num = i;
			min_dist = dist;
		}
	}
	
	return locdata[min_num].name;
}

void ReallocLocMsgs(int n)
{
	int new_max_locmsgs = max(2*max_locmsgs,n);
	locinfo = realloc(locinfo, new_max_locmsgs*sizeof(char*));
	memset(locinfo+max_locmsgs,0,(new_max_locmsgs-max_locmsgs)*sizeof(char*));
	max_locmsgs = new_max_locmsgs;
}

void ReallocLocPoints(void)
{
	max_locpoints = 2*max_locpoints;
	location = realloc(location, max_locpoints*sizeof(player_loc_t));
}

// Sergio --> (new locs)
void Loc_SortCoord(player_loc_t *loc)
{
	int a,b;

	a = loc->x1;
	b = loc->x2;
	loc->x1 = min(a,b);
	loc->x2 = max(a,b);

	a = loc->y1;
	b = loc->y2;
	loc->y1 = min(a,b);
	loc->y2 = max(a,b);

	a = loc->z1;
	b = loc->z2;
	loc->z1 = min(a,b);
	loc->z2 = max(a,b);

	loc->volume = abs((loc->x2-loc->x1) * (loc->y2-loc->y1) * (loc->z2-loc->z1));
}

void Cmd_LocDeleteBlock_f(void)
{

	int i, n1, n2;

	if ((loc_cur_block == -1 && Cmd_Argc() < 2) || Cmd_Argc() > 3)
	{
		Con_Printf("Usage: loc_deleteblock [<start block>] [<end block>]");
		return;
	}

	if (Cmd_Argc() > 1)
	{
		n1 = n2 = atoi(Cmd_Argv(1));
		if (Cmd_Argc() == 3)
			n2 = atoi(Cmd_Argv(2));
		if (n1 < 0 || n2 < 0 || n2 >= locpoints || n2 >= locpoints || n2 < n1)
		{
			Con_Printf("loc_deleteblock ERROR: bad block number\n");
			return;
		}
	} else
		n1 = n2 = loc_cur_block;

	if (loc_cur_block > n2)
		loc_cur_block -= (n2-n1+1);
	else if (loc_cur_block >= n1)
		loc_cur_block = -1;

	for (i = n1; i <= n2; i++)
	{
		if (location[i].name)
		{
			Z_Free((void *)location[i].name);
			location[i].name = NULL;
		}
	}
	memmove(&location[n1], &location[n2+1], sizeof(player_loc_t)*(locpoints-n2-1));
	locpoints -= (n2-n1+1);
	if (n2 == n1)
		Con_Printf("Block %i deleted\n", n1);
	else
		Con_Printf("Blocks %i - %i deleted\n", n1, n2);
}

qboolean Loc_GranularityOnChange(cvar_t *var, const char *value)
{
	float	v,n;
	int		i;

	v = atof(value);
	if (v < 1)
		v = 1;
	else if (v > 64)
		v = 64;

	n = var->value / v;
	if (v != var->value) {
		Cvar_SetValue(var, v);
		if (loc_loaded.value) {
			// Recalculate coordinates with new loc_granularity
			mirror_x *= n;
			mirror_y *= n;
			for (i = 0; i < locpoints; i++) {
				location[i].x1 *= n;
				location[i].y1 *= n;
				location[i].z1 *= n;
				location[i].x2 *= n;
				location[i].y2 *= n;
				location[i].z2 *= n;
				Loc_SortCoord(&location[i]);
			}
		}
	}
	return true;
}
// <-- Sergio

void Cmd_LocMsg_f(void)
{
	int n;
	if(Cmd_Argc()!=3)
		Con_Printf("Usage : loc_msg <num> \"<msg>\"\n");
	else {
		n=atoi(Cmd_Argv(1));
		if(n<0) {
			Con_Printf("loc_msg ERROR: invalid message number %d\n",n);
			return;
		} else if(n>=max_locmsgs)
			ReallocLocMsgs(n);

		if(strlen(Cmd_Argv(2))>=MAX_MESSAGELEN) {
			Con_Printf("loc_msg ERROR: msg(%d) length out of range\n",n);
			(Cmd_Argv(2))[MAX_MESSAGELEN]='\0';
		}

		if(locinfo[n]) Z_Free(locinfo[n]);
		locinfo[n]=Z_StrDup(Cmd_Argv(2));
	}
}

void Cmd_LocPoint_f(void)
{
// Changed by Sergio --> (new locs)
	int		i,a;

	if(locpoints==max_locpoints)
		ReallocLocPoints();

	if(Cmd_Argc() < 8)
		Con_Printf("Usage :  loc_point <x1> <y1> <z1> <x2> <y2> <z2> (<\"name\">|<msg_num>)\n");
	else
	{
		location[locpoints].x1=atoi(Cmd_Argv(1));
		location[locpoints].x2=atoi(Cmd_Argv(4));

		location[locpoints].y1=atoi(Cmd_Argv(2));
		location[locpoints].y2=atoi(Cmd_Argv(5));

		location[locpoints].z1=atoi(Cmd_Argv(3));
		location[locpoints].z2=atoi(Cmd_Argv(6));

		Loc_SortCoord(&location[locpoints]);

		if (!strchr(Cmd_Args(), '\"'))
		{
			// Old locs format
			location[locpoints].name = NULL;
			for(i=7;i<Cmd_Argc() && i<10;i++)
			{
				a=atoi(Cmd_Argv(i));
				if(a<0 || a>=max_locmsgs)
				{
					Con_Printf("loc_point WARNING: msg_num out of range\n");
					a=max_locmsgs-1;
				}
				location[locpoints].msg[i-7]=a;
			}
			for(;i<=10;i++)
				location[locpoints].msg[i-7]=location[locpoints].msg[0];
		} else {
			// New locs format
			if (strlen(Cmd_Argv(7)) > MAX_MESSAGELEN)
				Cmd_Argv(7)[MAX_MESSAGELEN-1] = 0;
			location[locpoints].name = Z_StrDup(Cmd_Argv(7));
		}
		locpoints++;
	}
// <-- Sergio
}

void Cmd_LocMirror_f(void)
{
// Changed by Sergio --> (new locs)
	unsigned	i,c;
	char		*p, *t1 = NULL, *t2 = NULL;
	char		ss[MAX_MESSAGELEN]; 
	int			axis, tn1=0;
	player_loc_t *loc1, *loc2;

	if(Cmd_Argc() != 2 && Cmd_Argc() != 4) {
		Con_Printf("Usage :  loc_mirror <axis> [<team1> <team2>]\n");
		return;
	}

	Q_strlwr(p=Cmd_Argv(1));
	axis = (*p != 'x' ? (*p != 'y' ? 4 : 2) : 1) + (p[1] ? (p[1] == 'y' ? 2 : 4 ) : 0);
	if(axis>3) { 
		Con_Printf("Unknown axis identification, must be: X, Y or XY\n"); 
		return;
	}

	if (Cmd_Argc() == 4) {
		t1 = Cmd_Argv(2);
		t2 = Cmd_Argv(3);
		tn1 = strlen(t1);
	}

	if (2*locpoints > max_locpoints) // not enogh memory for mirror - expanding
		ReallocLocPoints();

	loc2 = location + locpoints;
	c = locpoints;

	// not to mirror blocks that cross defined axis
	for(i=0, loc1 = location; i<c; i++, loc1++)
		if ((axis==3 && (loc1->x2<mirror_x || loc1->x1>mirror_x || loc1->y2<mirror_y || loc1->y1>mirror_y))
			|| (axis==1 && (loc1->x2 < mirror_x || loc1->x1 > mirror_x))
			|| (axis==2 && (loc1->y2 < mirror_y || loc1->y1 > mirror_y))) {
		if (loc1->name) {
			if (tn1) {
				p=loc1->name;
				while ( (p=strchr(p, '@')) )
					if (!Q_strnicmp(++p, t1, tn1)) {
						strncpy(ss, loc1->name, p - loc1->name);
						ss[p - loc1->name] = 0;
						strcat(ss, t2);
						strcat(ss, &p[tn1]);
						loc2->name = Z_StrDup(ss);
						break;
					}
				if (!p)
					continue;
			} else
				loc2->name = Z_StrDup(loc1->name);
		} else if (tn1)
			continue;
		else
			loc2->name = NULL;

		if(axis & 1) {
			loc2->x1 = 2*mirror_x-loc1->x2;
			loc2->x2 = 2*mirror_x-loc1->x1;
		} else {
			loc2->x1 = loc1->x1;
			loc2->x2 = loc1->x2;
		}
		if(axis & 2) {
			loc2->y1 = 2*mirror_y-loc1->y2;
			loc2->y2 = 2*mirror_y-loc1->y1;
		} else {
			loc2->y1 = loc1->y1;
			loc2->y2 = loc1->y2;
		}
		loc2->z1 = loc1->z1;
		loc2->z2 = loc1->z2;
		loc2->msg[0] = loc1->msg[1];
		loc2->msg[1] = loc1->msg[0]; 

		Loc_SortCoord(loc2);
		locpoints++;
		loc2++;
	}
	Con_DPrintf("%d blocks mirrored\n", locpoints-c);
// <-- Sergio
}

void Cmd_LocSelectBlock_f(void)
{
	if (loc_nearest_block < 0) {
		if(Cmd_Argc()>2) {
			Con_Printf("loc_selectblock [<blocknum>]\n");
			return;
		} else if (Cmd_Argc() == 1) {
			loc_cur_block = -1;
			return;
		}

		loc_cur_block = atoi(Cmd_Argv(1));

		if (loc_cur_block>=locpoints || loc_cur_block<0) {
			loc_cur_block = -1;
			Con_Printf("Invalid block number\n");
			return;
		}
	} else
		loc_nearest_block = -1;

// Sergio --> (new locs)
	Con_Printf( "Block %d selected\n"
				"x1=%4d y1=%4d z1=%4d\n"
				"x2=%4d y2=%4d z2=%4d\n",
				loc_cur_block,
				location[loc_cur_block].x1, location[loc_cur_block].y1, location[loc_cur_block].z1,
				location[loc_cur_block].x2, location[loc_cur_block].y2, location[loc_cur_block].z2);

	if (location[loc_cur_block].name)
		Con_Printf("name= \"%s\"\n", location[loc_cur_block].name);
	else
		Con_Printf("msg1= %i, \"%s\"\nmsg2= %i, \"%s\"\n", location[loc_cur_block].msg[0], locinfo[location[loc_cur_block].msg[0]], 
			location[loc_cur_block].msg[1], locinfo[location[loc_cur_block].msg[1]]);
// <-- Sergio
}

// Sergio -->
void Cmd_LocSelectNearestBlock_f(void)
{
	loc_nearest_block = 0;
	Location262(&cl.simorg);
	loc_cur_block = loc_nearest_block; 
	Cmd_LocSelectBlock_f();
}
// <-- Sergio


// changed by Sergio --> (new locs)
char			loc_param[32];
int 			*loc_var;
player_loc_t	*loc_block;

void Cmd_LocEditBlock_f(void)
{
	extern int chat_bufferpos;

	if (loc_cur_block == -1) 
	{
		Con_Printf("No block selected\n");
		return;
	} else
		loc_block = location + loc_cur_block;

	if(Cmd_Argc() < 2 || Cmd_Argc() > 3)
	{
		Con_Printf("loc_editblock <parameter> [<value>]\n");
		return;
	}

	strncpy(loc_param, Cmd_Argv(1), 24);
	Q_strlwr(loc_param);

	if (!strcmp(loc_param,"x1"))
		loc_var = &(loc_block->x1);
	else if (!strcmp(loc_param,"y1"))
		loc_var = &(loc_block->y1);
	else if (!strcmp(loc_param,"z1"))
		loc_var = &(loc_block->z1);
	else if (!strcmp(loc_param,"x2"))
		loc_var = &(loc_block->x2);
	else if (!strcmp(loc_param,"y2"))
		loc_var = &(loc_block->y2);
	else if (!strcmp(loc_param,"z2"))
		loc_var = &(loc_block->z2);
	else if (!strcmp(loc_param,"msg1"))
		loc_var = &(loc_block->msg[0]);
	else if (!strcmp(loc_param,"msg2"))
		loc_var = &(loc_block->msg[1]);
	else if (strcmp(loc_param,"name")) {
		Con_Printf("loc_editblock: bad parameter\nParameter can be: x1|y1|z1|x2|y2|z2|msg1|msg2|name\n");
		return;
	}

	if (Cmd_Argc() == 3) {
		if (*loc_param == 'n') {
			// new name
			if (loc_block->name) 
				Z_Free((void *)loc_block->name);
			loc_block->name = Z_StrDup(Cmd_Argv(2));
			memset(&loc_block->msg, 0, sizeof(loc_block->msg)); // clear messages
		} else if (*loc_param == 'm') {
			// new message number
			*loc_var = atoi(Cmd_Argv(2));
			if (loc_block->name) 
			{
				// delete name if it exists
				Z_Free((void *)loc_block->name); 
				loc_block->name = NULL;
			}
		} else {
			// coordinates increment
			*loc_var += atoi(Cmd_Argv(2));
			Loc_SortCoord(loc_block);
		}
		return;
	} else {
		if (*loc_param == 'n') {
			if (!loc_block->name)
				*chat_buffer[chat_edit] = 0;
			else
				strncpy(chat_buffer[chat_edit], loc_block->name, MAXCMDLINE-1);
		} else
			sprintf(chat_buffer[chat_edit], "%d", *loc_var);
	}

	// ask parameter's value
	strcat(loc_param, ": ");
	chat_bufferpos = strlen(chat_buffer[chat_edit]);
	chat_bufferlen = chat_bufferpos;
	chat_team = 5;
	key_dest = key_message;
}

int		 finalmark=0;
int		 marks[6];
qboolean markcenter = false;

void Cmd_Mark_f(void)
{
	int i;
	FILE *f;

//	if(!cl.spectator || cls.state<ca_connected)
//		Con_Printf("This command reserved for spectators\n");
//	else 

	if(Cmd_Argc() < 2)
		Con_Printf("Usage :  mark <point> | <break> | <save> | <position> | <center> | <status> | <remove> | <list> [start] [len] | <point2>\n"); 
	else if(!Q_stricmp(Cmd_Argv(1),"point") || !Q_stricmp(Cmd_Argv(1),"point2")) {
		if (markcenter) {
			markcenter = false;
			finalmark = 0;
		}
		if (locpoints == 0) { // starting block marking for the map
			locinfo[0] = Z_StrDup("some place");
			if (loc_loaded.value && (loc_loaded_type == 1))
				Loc_UnLoadProxyLocFile();
			Cvar_SetROM(&loc_loaded, "1");
			loc_loaded_type = 0;
		}
		if(locpoints == max_locpoints) 
			ReallocLocPoints();

		marks[finalmark] = (int)(cl.simorg[0]/loc_granularity.value);
		marks[finalmark+1] = (int)(cl.simorg[1]/loc_granularity.value);
		marks[finalmark+2] = (int)(cl.simorg[2]/loc_granularity.value);
		if(finalmark) 
		{
			chat_team = Cmd_Argv(1)[5]?4:3; // ask msg or name
			key_dest = key_message;
		}
		Con_Printf("Marked %s point\n",finalmark?"second":"first");
		finalmark = 3 - finalmark;
	}
	else if(!Q_stricmp(Cmd_Argv(1),"break"))
	{
		Con_Printf("Mark %s reset\n", markcenter?"center":"point");
		finalmark=0;
		markcenter = false;
	}
	else if(!Q_stricmp(Cmd_Argv(1),"save"))
	{
		Sys_mkdir(va("%s/loc",com_gamedir));
		f = fopen (va("%s/loc/%s.loc",com_gamedir,Info_ValueForKey(cl.serverinfo,"map")), "w+");
		if(!f) { Con_Printf("Can't open .loc file to write\n",name); return; }

		fprintf(f, "mark center %d %d\nloc_granularity %g\n\n",mirror_x,mirror_y,loc_granularity.value);

		for(i=0;i<max_locmsgs;i++)
			if(locinfo[i]) {
				fprintf(f, "loc_msg %d \"%s\"\n", i, locinfo[i]);
			}

		fprintf(f, "\n");
		for(i=0;i<locpoints;i++)
		{
			if (location[i].name)
				fprintf(f,"loc_point %5d %5d %5d %5d %5d %5d \"%s\"\n", 
					location[i].x1, location[i].y1, location[i].z1,
					location[i].x2,location[i].y2,location[i].z2,location[i].name);
			else
				fprintf(f,"loc_point %5d %5d %5d %5d %5d %5d %2d %2d %2d %2d\n",
					location[i].x1,location[i].y1,location[i].z1,
					location[i].x2,location[i].y2,location[i].z2,
					location[i].msg[0],location[i].msg[1],location[i].msg[2],location[i].msg[3]);
		}
		fclose(f);
		Con_Printf("%i locations saved loc/%s.loc\n", locpoints, Info_ValueForKey(cl.serverinfo,"map"));
	}
	else if (!Q_stricmp(Cmd_Argv(1),"list"))
	{
		int n1 = 0, n2 = locpoints; 

		if (Cmd_Argc() >= 3)
		{
			n1 = atoi(Cmd_Argv(2));
			if (n1 < 0)
				n1 = 0;
			else if (n1 >= locpoints)
				n1 = locpoints-1;
		}
		if (Cmd_Argc() == 4)
		{
			n2 = n1 + atoi(Cmd_Argv(3));
			if (n2 < n1)
				n2 = n1+1;
			if (n2 > locpoints)
				n2 = locpoints;
		};
		for(i = n1; i < n2; i++)
			if (location[i].name)
				Con_Printf("block %2d: \"%s\"\n", i, location[i].name);
			else
				Con_Printf("block %2d: msg: %2d %2d %2d %2d\n", i,location[i].msg[0],location[i].msg[1],location[i].msg[2],location[i].msg[3]);
		Con_Printf("\n~%d control blocks are in use\n",locpoints);
	}
	else if(!Q_stricmp(Cmd_Argv(1),"status"))
	{
		for(i=0;i<max_locmsgs;i++)
			if(locinfo[i]) Con_Printf("loc_msg~%d \"%s\"\n",i,locinfo[i]);
		Con_Printf("~%d control blocks are in use\n",locpoints);
	}
	else if(!Q_stricmp(Cmd_Argv(1),"position"))
		Con_Printf("%d:%d:%d\n",(int)(cl.simorg[0]/loc_granularity.value),
			(int)(cl.simorg[1]/loc_granularity.value),(int)(cl.simorg[2]/loc_granularity.value));
	else if(!Q_stricmp(Cmd_Argv(1),"center"))
	{
		if(Cmd_Argc()!=4)
		{
			if (!markcenter) {
				markcenter = true;
				finalmark = 0;
			}
			marks[finalmark] = (int)cl.simorg[0];
			marks[finalmark+1] = (int)cl.simorg[1];
			marks[finalmark+2] = (int)cl.simorg[2];
			Con_Printf("Marked %s center point\n",finalmark?"second":"first");
			if (finalmark) {
				mirror_x = (int)(marks[0]+(marks[3]-marks[0])/2) / loc_granularity.value;
				mirror_y = (int)(marks[1]+(marks[4]-marks[1])/2) / loc_granularity.value;
				Con_Printf("\nVirtual mirror center is %d:%d\n",mirror_x,mirror_y);
				markcenter = false;
			}
			finalmark = 3 - finalmark;
		}
		 else 
		{
			mirror_x=atoi(Cmd_Argv(2));
			mirror_y=atoi(Cmd_Argv(3));
			markcenter = false;
			finalmark = 0;
		}
	}
	else if(!Q_stricmp(Cmd_Argv(1),"remove"))
	{

		if (!locpoints)
			Con_Printf("points list is empty\n");
		else { 
			if (location[locpoints].name) {
				Z_Free((void *)location[locpoints].name);
				location[locpoints].name = NULL;
			}
			locpoints--;
			Con_Printf("last point removed from list\n"); 
		}
	}
	else Con_Printf("Unknown command %s\n",Cmd_Argv(1));
}
// <-- Sergio

void CL_LoadLocFile_f (void)
{
	if (Cmd_Argc() != 2) {
		Con_Printf ("loadloc <filename> : load a loc file\n");
		return;
	}

	if (loc_loaded.value)				// Sergio
		Loc_UnLoadLocFile();

	Loc_LoadLocFile (Cmd_Argv(1));
}

void Loc_Coords2Loc_f (void)
{
	cvar_t	*var;
	char	*var_name;
	vec3_t	pos;
	char	buf[128];
	float	old_cl_locmessage;
	
	if (Cmd_Argc() != 5){
		Con_Printf ("usage: loc_coords2loc <var> <x> <y> <z>\n");
		return;
	}

	var_name = Cmd_Argv(1);
	var = Cvar_FindVar (var_name);

	if (!var) {
		Con_Printf ("Unknown variable \"%s\"\n", var_name);
		return;
	}

	pos[0] = atoi (Cmd_Argv(2));
	pos[1] = atoi (Cmd_Argv(3));
	pos[2] = atoi (Cmd_Argv(4));

	old_cl_locmessage = cl_locmessage.value;
	cl_locmessage.value = 1;
	InsertLocation (&pos,buf,sizeof(buf),false);
	cl_locmessage.value = old_cl_locmessage;
	Cvar_Set (var, buf);
}

void CL_InitLocFiles(void)
{
	Cvar_RegisterVariable (&loc_loaded);
	Cvar_RegisterVariable (&cl_loadlocs);
	Cvar_RegisterVariable (&cl_loctype);
	Cvar_RegisterVariable (&cl_locnear);
	Cvar_RegisterVariable (&cl_locmessage);

	// Sergio --> (new locs)
	Cvar_RegisterVariable (&cl_blueteam);
	Cvar_RegisterVariable (&cl_redteam);
	Cvar_RegisterVariable (&cl_yellowteam);
	Cvar_RegisterVariable (&cl_greenteam);

	Cvar_RegisterVariable (&loc_granularity);
	loc_granularity.OnChange = Loc_GranularityOnChange;

	Cmd_AddCommand("loc_deleteblock", Cmd_LocDeleteBlock_f);
	Cmd_AddCommand("loc_unload", Loc_UnLoadLocFile);
// <-- Sergio

	Cmd_AddCommand ("loc_msg", Cmd_LocMsg_f);
	Cmd_AddCommand ("loc_mirror", Cmd_LocMirror_f);
	Cmd_AddCommand ("loc_point",Cmd_LocPoint_f);
	Cmd_AddCommand ("loc_selectblock", Cmd_LocSelectBlock_f);
	Cmd_AddCommand ("loc_selectnearestblock", Cmd_LocSelectNearestBlock_f); // Sergio
	Cmd_AddCommand ("loc_editblock", Cmd_LocEditBlock_f);
	Cmd_AddCommand ("mark",Cmd_Mark_f);

	Cmd_AddCommand ("loadloc", CL_LoadLocFile_f);

	Cmd_AddCommandTrig ("loc_coords2loc", Loc_Coords2Loc_f);

	max_locmsgs = MAX_LOC_MESSAGES;
	max_locpoints = MAX_LOC_MESSAGES;

	location = malloc(max_locpoints*sizeof(player_loc_t));
	locinfo = calloc(max_locmsgs,sizeof(char*));

	loc_maxentries = MAX_LOC_ENTRIES;
	locdata = malloc(sizeof(locdata_t)*loc_maxentries);
}
