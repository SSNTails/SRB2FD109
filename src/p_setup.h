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
/// \brief Setup a game, startup stuff

#ifndef __P_SETUP__
#define __P_SETUP__

#include "doomdata.h"
#include "r_defs.h"

// Player spawn spots for deathmatch.
#define MAX_DM_STARTS 64
extern mapthing_t* deathmatchstarts[MAX_DM_STARTS];
extern int numdmstarts, numcoopstarts, numredctfstarts, numbluectfstarts;

extern int lastloadedmaplumpnum; // for comparative savegame
//
// MAP used flats lookup table
//
typedef struct
{
	char name[8]; // resource name from wad
	int lumpnum; // lump number of the flat

	// for flat animation
	int baselumpnum;
	int animseq; // start pos. in the anim sequence
	int numpics;
	int speed;
} levelflat_t;

extern int numlevelflats;
extern levelflat_t* levelflats;
int P_AddLevelFlat(const char* flatname, levelflat_t* levelflat);

extern int nummapthings;
extern mapthing_t* mapthings;

void P_SetupLevelSky(int skynum);
boolean P_SetupLevel(int map, skill_t skill, const char* mapname);
boolean P_AddWadFile(const char* wadfilename, char** firstmapname);
void P_WriteThings(int lump);
// P_PrecacheLevelFlats caches the flats.
int P_PrecacheLevelFlats(void);
void P_InitMapHeaders(void);
void P_ClearMapHeaderInfo(void);

#endif
