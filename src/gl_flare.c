#include "quakedef.h"

flare	cl_flares[MAX_FLARES];

void ClearFlares(void)
{
	int	i;
	for (i=0 ; i<MAX_FLARES ; i++) {
		cl_flares[i].die = 0;
		cl_flares[i].key = 0;
	}
}

flare	*CL_AllocFlare (int key)
{
	int		i;
	flare	*fl;

	if (key)
	{
		fl = cl_flares;
		for (i=0 ; i<MAX_FLARES ; i++, fl++)
		{
			if (fl->key == key)
			{
				memset (fl, 0, sizeof(*fl));
				fl->key = key;
				return fl;
			}
		}
	}

// then look for anything else
	fl = cl_flares;
	for (i=0 ; i<MAX_FLARES ; i++, fl++)
	{
		if (fl->die < cl.time)
		{
			memset (fl, 0, sizeof(*fl));
			fl->key = key;
			return fl;
		}
	}

	fl = &cl_flares[0];
	memset (fl, 0, sizeof(*fl));
	fl->key = key;
	return fl;
}

void R_RenderFlares (void)
{
	int		i;
	flare	*l;

	glDepthMask (GL_FALSE);
	glDisable (GL_TEXTURE_2D);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);

	l = cl_flares;
	for (i=0 ; i<MAX_FLARES ; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		l->radius += l->decay;
//		color change is now made in R_RenderDlight
//		l->color[3] = l->color[3] * 0.5;
		R_RenderDlight ((dlight_t *)l);
	}

	glColor3ubv (color_white);
	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (GL_TRUE);
}

void R_Flare (vec3_t start, vec3_t end, int type)
{
	vec3_t		vec, v;
	float		len;
	flare		*fl;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	v[0] = (start[0] + end[0]) / 2;
	v[1] = (start[1] + end[1]) / 2;
	v[2] = (start[2] + end[2]) / 2;

	VectorNormalize (v);
	
	if (len > 0)
	{
		switch (type)
		{
		case 0:
			fl = CL_AllocFlare (0);
			VectorCopy (end, fl->origin);
			fl->radius = 20;
			fl->die = cl.time + 0.5;
			fl->decay = -1;
			fl->color[0] = 0.9;
			fl->color[1] = 0.7;
			fl->color[2] = 0.3;
			fl->color[3] = 1.0;
			fl->type = lt_rocket_trail_1;
			fl = CL_AllocFlare (0);
			VectorCopy (end, fl->origin);
			fl->radius = 50;
			fl->die = cl.time + 0.5;
			fl->decay = -2;
			fl->color[0] = 0.3;
			fl->color[1] = 0.2;
			fl->color[2] = 0.1;
			fl->color[3] = 0.2;
			fl->type = lt_rocket_trail_2;
			break;

// FIXME This part is never used?
/*		case 1:
			fl = CL_AllocFlare (0);
			VectorCopy (end, fl->origin);
			fl->radius = 50;
			fl->die = cl.time + 0.5;
			fl->decay = -2;
			fl->color[0] = 0.1;
			fl->color[1] = 0.1;
			fl->color[2] = 0.1;
			fl->color[3] = 0.2;
			fl = CL_AllocFlare (0);
			VectorCopy (v, fl->origin);
			fl->radius = 50;
			fl->die = cl.time + 0.5;
			fl->decay = -2;
			fl->color[0] = 0.1;
			fl->color[1] = 0.1;
			fl->color[2] = 0.1;
			fl->color[3] = 0.2;
			break;
		case 2:
			fl = CL_AllocFlare (0);
			VectorCopy (end, fl->origin);
			fl->radius = 20;
			fl->die = cl.time + 0.5;
			fl->decay = -2;
			fl->color[0] = 0.3;
			fl->color[1] = 0.0;
			fl->color[2] = 0.0;
			fl->color[3] = 0.1;
			break; */
		}
	}
}
