#define MAX_FLARES 128

typedef struct
{
	int		key;				// so entities can reuse same entry
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		type;				// Tonik
	float	color[4];
} flare;

extern	cvar_t	gl_flares; // -=MD=-

void	ClearFlares(void);
flare	*CL_AllocFlare (int key);
void	R_RenderFlare (flare *light);
void	R_RenderFlares (void);
void	R_Flare (vec3_t start, vec3_t end, int type);
