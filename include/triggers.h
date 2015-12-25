// Message triggers. Original idea - Turba, first implementation - Tonik

#include "pcre.h"

extern double	last_respawntime;
extern double	last_deathtime;
extern double	last_gotammotime;

typedef struct pcre_trigger_s {
	char					*name;
	char					*regexpstr;
	struct pcre_trigger_s*	next;
	pcre*					regexp;
	pcre_extra*				regexp_extra;
	unsigned				flags;
	float					min_interval;
	double					lasttime;
	int						counter;
} pcre_trigger_t;

typedef void internal_trigger_func(char *s);

typedef struct pcre_internal_trigger_s {
	struct pcre_internal_trigger_s		*next;
	pcre								*regexp;
	pcre_extra							*regexp_extra;
	internal_trigger_func				*func;
	unsigned							flags;
} pcre_internal_trigger_t;

#define		RE_PRINT_LOW		1
#define		RE_PRINT_NEDIUM		2
#define		RE_PRINT_HIGH		4
#define		RE_PRINT_CHAT		8
#define		RE_PRINT_CENTER		16
#define		RE_PRINT_ECHO		32
#define		RE_PRINT_INTERNAL	64

// all of the above except internal
#define		RE_PRINT_ALL		31

// do not look for other triggers if matching is succesful
#define		RE_FINAL			256
// do not display string if matching is succesful
#define		RE_REMOVESTR		512
// do not log string if matching is succesful
#define		RE_NOLOG			1024
// trigger is enabled
#define		RE_ENABLED			2048
// do not call alias
#define		RE_NOACTION			4096

void CL_InitTriggers();
void CL_NewMap();

qboolean CL_SearchForMsgTriggers (char *s, unsigned trigger_type);
// if true, string should not be displayed

qboolean AllowedImpulse(int imp);

extern qboolean	first_respawn_done;
extern cvar_t	cl_setinfo_triggers;

void CL_StatChanged (int stat, int value);
void CL_ExecTrigger (char *s);
void CL_ExecTriggerSafe (char *s);
void CL_ExecTriggerSetinfo (char *s, char *param1, char *param2, char *param3);
pcre_trigger_t *CL_FindReTrigger (char *name);
void CL_RE_Trigger_ResetLasttime (void);
