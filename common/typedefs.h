#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include "defines.h"

typedef struct _RGB
{
	uint8_t red, green, blue;
} RGB;

typedef struct _2D_POINT_t {
 double x, y;
} double2;

typedef struct _3D_POINT_t {
 double x, y, z;
} double3;

typedef struct {
	double3 (*pixels)[SCREEN_WIDTH];
	double2 viewpoint; // read only
	double viewscale; // read only
	double2 viewscale_lens; // read only
	// double screen_bounds[4]; // xmin, xmax, ymin, ymax   read only
	double2 screen_bounds_min;
	double2 screen_bounds_max;
} screen;

#endif
