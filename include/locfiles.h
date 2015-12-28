// Loc files support


// -=MD=- .loc
#define		MAX_LOC_POINTS		32
#define		MAX_LOC_MESSAGES	32
#define		MAX_MESSAGELEN		200

typedef struct LOCATION
{
	int x,y,z;
} lvect3;

typedef struct player_loc_s {
	int		x1,x2,y1,y2,z1,z2;
	int		msg[4];
// Sergio --> (new locs)
	char	*name;
	int		volume;
// <-- Sergio
} player_loc_t;

extern vec3_t		PlaceOfLastDeath;
extern double		last_locationtime;

extern int			loc_loaded_type;
extern int			locpoints, mirror_y, mirror_x; 

extern int			loc_team;
extern char			loc_teamnames[4][5];

extern char*		*locinfo;
extern player_loc_t	*location;

extern int			max_locmsgs;
extern int			max_locpoints;
extern int			loc_cur_block;

// proxy .loc
#define		MAX_LOC_NAME		32
#define		MAX_LOC_ENTRIES		128

typedef struct locdata_s {
	vec3_t coord;
	char name[MAX_LOC_NAME];
} locdata_t;

extern cvar_t loc_loaded;
extern cvar_t cl_locmessage;

void Loc_Load262LocFile(char *filename);
void Loc_LoadProxyLocFile(char *filename);
void Loc_LoadLocFile(char *filename);

void Loc_UnLoad262LocFile(void);
void Loc_UnLoadProxyLocFile(void);
void Loc_UnLoadLocFile(void);

extern cvar_t cl_blueteam, cl_redteam, cl_yellowteam, cl_greenteam;

char *InsertLocation(vec3_t *l, char *n, int m, qboolean own);
char *Location262(vec3_t *l);
char *LocationProxy(vec3_t *l);

void CL_InitLocFiles(void);
void Cmd_LocSelectBlock_f(void);
