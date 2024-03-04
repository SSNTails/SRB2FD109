// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief Refresh, visplane stuff (floor, ceilings)

#ifndef __R_PLANE__
#define __R_PLANE__

#include "screen.h" // needs MAXVIDWIDTH/MAXVIDHEIGHT
#include "r_data.h"

//
// Now what is a visplane, anyway?
// Simple: kinda floor/ceiling polygon optimised for SRB2 rendering.
// 4124 bytes!
//
typedef struct visplane_s
{
	struct visplane_s* next;

	fixed_t height, viewz;
	angle_t viewangle;
	fixed_t picnum;
	int lightlevel;
	int minx, maxx;

	// colormaps per sector
	extracolormap_t* extra_colormap;

	// leave pads for [minx-1]/[maxx+1]

	// words sucks .. should get rid of that.. but eats memory
	// THIS IS UNSIGNED! VERY IMPORTANT!!
	unsigned short pad1;
	unsigned short top[MAXVIDWIDTH];
	unsigned short pad2;
	unsigned short pad3;
	unsigned short bottom[MAXVIDWIDTH];
	unsigned short pad4;

	int high, low; // R_PlaneBounds should set these.

	fixed_t xoffs, yoffs; // Scrolling flats.

	// SoM: frontscale should be stored in the first seg of the subsector
	// where the planes themselves are stored. I'm doing this now because
	// the old way caused trouble with the drawseg array was re-sized.
	int scaleseg;

	struct ffloor_s* ffloor;
} visplane_t;

extern visplane_t* floorplane;
extern visplane_t* ceilingplane;

// Visplane related.
extern short *lastopening, *openings;
extern size_t maxopenings;
typedef void (*planefunction_t)(int top, int bottom);

extern short floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
extern fixed_t frontscale[MAXVIDWIDTH], yslopetab[MAXVIDHEIGHT*4];
extern fixed_t cachedheight[MAXVIDHEIGHT];
extern fixed_t cacheddistance[MAXVIDHEIGHT];
extern fixed_t cachedxstep[MAXVIDHEIGHT];
extern fixed_t cachedystep[MAXVIDHEIGHT];
extern fixed_t basexscale, baseyscale;

extern fixed_t* yslope;
extern fixed_t distscale[MAXVIDWIDTH];

void R_InitPlanes(void);
void R_ClearPlanes(player_t* player);

void R_MapPlane(int y, int x1, int x2);
void R_MakeSpans(int x, int t1, int b1, int t2, int b2);
void R_DrawPlanes(void);
visplane_t* R_FindPlane(fixed_t height, fixed_t picnum, int lightlevel, fixed_t xoff, fixed_t yoff,
	extracolormap_t* planecolormap, ffloor_t* ffloor);
visplane_t* R_CheckPlane(visplane_t* pl, int start, int stop);
void R_ExpandPlane(visplane_t* pl, int start, int stop);
void R_PlaneBounds(visplane_t* plane);

// Draws a single visplane. If !handlesource, it won't allocate or remove ds_source.
void R_DrawSinglePlane(visplane_t* pl, boolean handlesource);

typedef struct planemgr_s
{
	visplane_t* plane;
	fixed_t height;
	boolean mark;
	fixed_t f_pos; // F for Front sector
	fixed_t b_pos; // B for Back sector
	fixed_t f_frac, f_step;
	fixed_t b_frac, b_step;
	short f_clip[MAXVIDWIDTH];
	short c_clip[MAXVIDWIDTH];

	struct ffloor_s* ffloor;
} planemgr_t;

extern planemgr_t ffloor[MAXFFLOORS];
extern int numffloors;
#endif
