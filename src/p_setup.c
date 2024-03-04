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
/// \brief Do all the WAD I/O, get map description, set up initial state and misc. LUTs

#include "doomdef.h"
#include "d_main.h"
#include "byteptr.h"
#include "g_game.h"

#include "p_local.h"
#include "p_setup.h"
#include "p_spec.h"

#include "i_sound.h" // for I_PlayCD()..
#include "i_video.h" // for I_FinishUpdate()..
#include "r_sky.h"

#include "r_data.h"
#include "r_things.h"
#include "r_sky.h"

#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"

#include "hu_stuff.h"
#include "console.h"

#include "m_misc.h"
#include "m_fixed.h"
#include "m_random.h"

#include "dehacked.h" // for map headers
#include "r_main.h"

#include "m_argv.h"

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(_WIN64)
#include <malloc.h>
#include <math.h>
#endif
#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#endif

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

int numvertexes, numsegs, numsectors, numsubsectors, numnodes, numlines, numsides, nummapthings;
vertex_t* vertexes;
seg_t* segs;
sector_t* sectors;
subsector_t* subsectors;
node_t* nodes;
line_t* lines;
side_t* sides;
mapthing_t* mapthings;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int bmapwidth, bmapheight; // size in mapblocks

long* blockmap; // int for large maps
// offsets in blockmap are from here
long* blockmaplump; // Big blockmap

// origin of block map
fixed_t bmaporgx, bmaporgy;
// for thing chains
mobj_t** blocklinks;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed LineOf Sight calculation.
// Without special effect, this could be used as a PVS lookup as well.
//
byte* rejectmatrix;

// Maintain single and multi player starting spots.
int numdmstarts, numcoopstarts, numredctfstarts, numbluectfstarts;
mapthing_t* deathmatchstarts[MAX_DM_STARTS];
mapthing_t* playerstarts[MAXPLAYERS];
mapthing_t* bluectfstarts[MAXPLAYERS];
mapthing_t* redctfstarts[MAXPLAYERS];

#ifdef _WINDOWS
#include "win32/win_main.h"
#endif

/** Logs an error about a map being corrupt, then terminate.
  * This allows reporting highly technical errors for usefulness, without
  * confusing a novice map designer who simply needs to run ZenNode.
  *
  * If logging is disabled in this compile, or the log file is not opened, the
  * full technical details are printed in the I_Error() message.
  *
  * \param msg The message to log. This message can safely result from a call
  *            to va(), since that function is not used here.
  * \todo Fix the I_Error() message. On some implementations the logfile may
  *       not be called log.txt.
  * \sa CON_LogMessage, I_Error
  */
FUNCNORETURN static void CorruptMapError(const char* msg)
{
#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
	{
		// don't use va() because the calling function probably uses it
		char mapnum[10];

		sprintf(mapnum, "%hd", gamemap);
		CON_LogMessage("Map ");
		CON_LogMessage(mapnum);
		CON_LogMessage(" is corrupt: ");
		CON_LogMessage(msg);
		CON_LogMessage("\n");
		I_Error("Invalid or corrupt map.\nSee log.txt for technical details.");
	}
	else
#endif
		I_Error("Invalid or corrupt map (%d): %s", gamemap, msg);
}

/** Clears the data from a single map header.
  *
  * \param i Map number to clear header for.
  * \sa P_ClearMapHeaderInfo, P_LoadMapHeader
  */
static void P_ClearSingleMapHeaderInfo(short i)
{
	mapheaderinfo[i-1].actnum = 0;
	mapheaderinfo[i-1].nozone = false;
	mapheaderinfo[i-1].forcecharacter = 255;
	mapheaderinfo[i-1].lvlttl[0] = '\0';
	mapheaderinfo[i-1].musicslot = (short)((short)(mus_map01m + i) - 1);
	mapheaderinfo[i-1].nextlevel = (short)(i + 1);
	mapheaderinfo[i-1].typeoflevel = 0;
	mapheaderinfo[i-1].weather = 0;
	mapheaderinfo[i-1].skynum = i;
	mapheaderinfo[i-1].countdown = 0;
	mapheaderinfo[i-1].precutscenenum = 0;
	mapheaderinfo[i-1].hideinmenu = 0;
	mapheaderinfo[i-1].nossmusic = 0;
	mapheaderinfo[i-1].cutscenenum = 0;
	mapheaderinfo[i-1].interscreen[0] = '#';
	mapheaderinfo[i-1].scriptname[0] = '#';
	mapheaderinfo[i-1].scriptislump = false;
}

/** Clears the data from the map headers for all levels.
  *
  * \sa P_ClearSingleMapHeaderInfo, P_InitMapHeaders
  */
void P_ClearMapHeaderInfo(void)
{
	short i;

	for(i = 1; i <= NUMMAPS; i++)
		P_ClearSingleMapHeaderInfo(i);
}

/** Initializes the map headers.
  * Only new, Dehacked format map headers (MAPxxD) are loaded here. Old map
  * headers (MAPxxN) are no longer supported.
  *
  * \sa P_ClearMapHeaderInfo, P_LoadMapHeader
  */
void P_InitMapHeaders(void)
{
	char mapheader[7];
	int lumpnum, moremapnumbers, mapnum;

	for(mapnum = 1; mapnum <= NUMMAPS; mapnum++)
	{
		moremapnumbers = mapnum - 1;

		strncpy(mapheader, G_BuildMapName(mapnum), 5);

		mapheader[5] = 'D'; // New header
		mapheader[6] = '\0';

		lumpnum = W_CheckNumForName(mapheader);

		if(!(lumpnum == -1 || W_LumpLength(lumpnum) <= 0))
			DEH_LoadDehackedLump(lumpnum);
	}
}

/** Sets up the data in a single map header.
  *
  * \param mapnum Map number to load header for.
  * \sa P_ClearSingleMapHeaderInfo, P_InitMapHeaders
  */
static inline void P_LoadMapHeader(short mapnum)
{
	char mapheader[7];
	int lumpnum;

	strncpy(mapheader, G_BuildMapName(mapnum), 5);

	mapheader[5] = 'D'; // New header
	mapheader[6] = '\0';

	lumpnum = W_CheckNumForName(mapheader);

	if(!(lumpnum == -1 || W_LumpLength(lumpnum) <= 0))
	{
		P_ClearSingleMapHeaderInfo(mapnum);
		DEH_LoadDehackedLump(lumpnum);
		return;
	}
}

/** Loads the vertexes for a level.
  *
  * \param lump VERTEXES lump number.
  * \sa ML_VERTEXES
  */
static inline void P_LoadVertexes(int lump)
{
	byte* data;
	int i;
	mapvertex_t* ml;
	vertex_t* li;

	// Determine number of lumps:
	//  total lump length / vertex record length.
	numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

	if(numvertexes <= 0)
		I_Error("Level has no vertices"); // instead of crashing

	// Allocate zone memory for buffer.
	vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);

	// Load data into cache.
	data = W_CacheLumpNum(lump, PU_STATIC);

	ml = (mapvertex_t*)data;
	li = vertexes;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for(i = 0; i < numvertexes; i++, li++, ml++)
	{
		li->x = SHORT(ml->x)<<FRACBITS;
		li->y = SHORT(ml->y)<<FRACBITS;
	}

	// Free buffer memory.
	Z_Free(data);
}

// Debugger function.
static inline void P_LoadDBGVertexes(int lump)
{
	byte* data;
	int i;
	dbgmapvertex_t* ml;
	vertex_t* li;

	// Determine number of lumps:
	//  total lump length / vertex record length.
	numvertexes = W_LumpLength(lump) / sizeof(dbgmapvertex_t);

	if(numvertexes <= 0)
		I_Error("Level has no vertices"); // instead of crashing

	// Allocate zone memory for buffer.
	vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);

	// Load data into cache.
	data = W_CacheLumpNum(lump, PU_STATIC);

	ml = (dbgmapvertex_t*)data;
	li = vertexes;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for(i = 0; i < numvertexes; i++, li++, ml++)
	{
		li->x = LONG(ml->x)<<FRACBITS;
		li->y = LONG(ml->y)<<FRACBITS;
	}

	// Free buffer memory.
	Z_Free(data);
}

//
// Computes the line length in fracunits, the glide render needs this
//
#define crapmul (1.0f / 65536.0f)

/** Computes the length of a seg in fracunits.
  * This is needed for splats.
  *
  * \param seg Seg to compute length for.
  * \return Length in fracunits.
  */
float P_SegLength(seg_t* seg)
{
	double dx, dy;

	// make a vector (start at origin)
	dx = (seg->v2->x - seg->v1->x)*crapmul;
	dy = (seg->v2->y - seg->v1->y)*crapmul;

	return (float)(sqrt(dx*dx + dy*dy)*FRACUNIT);
}

/** Loads the SEGS resource from a level.
  *
  * \param lump Lump number of the SEGS resource.
  * \sa ::ML_SEGS
  */
static void P_LoadSegs(int lump)
{
	byte* data;
	int i, linedef, side;
	mapseg_t* ml;
	seg_t* li;
	line_t* ldef;

	numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
	if(numsegs <= 0)
		I_Error("Level has no segs"); // instead of crashing
	segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
	memset(segs, 0, numsegs * sizeof(seg_t));
	data = W_CacheLumpNum(lump, PU_STATIC);

	ml = (mapseg_t*)data;
	li = segs;
	for(i = 0; i < numsegs; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];

#ifdef HWRENDER // not win32 only 19990829 by Kin
		// used for the hardware render
		if(rendermode != render_soft && rendermode != render_none)
		{
			li->length = P_SegLength(li);
			//Hurdler: 04/12/2000: for now, only used in hardware mode
			li->lightmaps = NULL; // list of static lightmap for this seg
		}
#endif

		li->angle = (SHORT(ml->angle))<<FRACBITS;
		li->offset = (SHORT(ml->offset))<<FRACBITS;
		linedef = SHORT(ml->linedef);
		ldef = &lines[linedef];
		li->linedef = ldef;
		li->side = side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if(ldef-> flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;

		li->numlights = 0;
		li->rlights = NULL;
	}

	Z_Free(data);
}

/** Loads the SSECTORS resource from a level.
  *
  * \param lump Lump number of the SSECTORS resource.
  * \sa ::ML_SSECTORS
  */
static inline void P_LoadSubsectors(int lump)
{
	byte* data;
	int i;
	mapsubsector_t* ms;
	subsector_t* ss;

	numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
	if(numsubsectors <= 0)
		I_Error("Level has no subsectors (did you forget to run it through a nodesbuilder?)");
	subsectors = Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
	data = W_CacheLumpNum(lump,PU_STATIC);

	ms = (mapsubsector_t*)data;
	memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
	ss = subsectors;

	for(i = 0; i < numsubsectors; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
	}

	Z_Free(data);
}

//
// P_LoadSectors
//

//
// levelflats
//
#define MAXLEVELFLATS 256

int numlevelflats;
levelflat_t* levelflats;

//SoM: Other files want this info.
int P_PrecacheLevelFlats(void)
{
	int i, lump, flatmemory = 0;

	//SoM: 4/18/2000: New flat code to make use of levelflats.
	for(i = 0; i < numlevelflats; i++)
	{
		lump = levelflats[i].lumpnum;
		if(devparm)
			flatmemory += W_LumpLength(lump);
		R_GetFlat(lump);
	}
	return flatmemory;
}

#if 0
static inline int P_FlatNumForName(char* flatname)
{
	return P_AddLevelFlat(flatname, levelflats);
}
#endif

// help function for P_LoadSectors, find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
//
int P_AddLevelFlat(const char* flatname, levelflat_t* levelflat)
{
	union
	{
		char s[9];
		int x[2];
	} name8;
	int i, v1, v2;

	strncpy(name8.s, flatname, 8); // make it two ints for fast compares
	name8.s[8] = 0; // in case the name was one that filled 8 chars
	strupr(name8.s); // case insensitive
	v1 = name8.x[0];
	v2 = name8.x[1];

	//
	//  first scan through the already found flats
	//
	for(i = 0; i < numlevelflats; i++, levelflat++)
		if(*(int*)levelflat->name == v1 && *(int*)&levelflat->name[4] == v2)
			break;

	// that flat was already found in the level, return the id
	if(i == numlevelflats)
	{
		// store the name
		*((int*)levelflat->name) = v1;
		*((int*)&levelflat->name[4]) = v2;

		// store the flat lump number
		levelflat->lumpnum = R_GetFlatNumForName(flatname);

		if(devparm)
			CONS_Printf("flat #%03d: %s\n", numlevelflats, name8.s);

		numlevelflats++;

		if(numlevelflats >= MAXLEVELFLATS)
			I_Error("Too many flats in level\n");
	}

	// level flat id
	return i;
}

// Debugger function
static void P_LoadDBGSectors(int lump)
{
	byte* data;
	int i;
	dbgmapsector_t* ms;
	sector_t* ss;
	levelflat_t* foundflats;

	numsectors = W_LumpLength(lump) / sizeof(dbgmapsector_t);
	if(numsectors <= 0)
		I_Error("Level has no sectors");
	sectors = Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
	memset(sectors, 0, numsectors*sizeof(sector_t));
	data = W_CacheLumpNum(lump,PU_STATIC);

	//Fab:FIXME: allocate for whatever number of flats
	//           512 different flats per level should be plenty
	foundflats = alloca(sizeof(levelflat_t) * MAXLEVELFLATS);
	if(!foundflats)
		I_Error("Ran out of memory while loading sectors\n");
	memset(foundflats, 0, sizeof(levelflat_t) * MAXLEVELFLATS);

	numlevelflats = 0;

	ms = (dbgmapsector_t*)data;
	ss = sectors;
	for(i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = LONG(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = LONG(ms->ceilingheight)<<FRACBITS;

		//
		//  flats
		//
		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);

		ss->thinglist = NULL;
		ss->touching_thinglist = NULL;
		ss->preciplist = NULL;
		ss->touching_preciplist = NULL;

		ss->heightsec = -1;
		ss->altheightsec = 0;
		ss->floorlightsec = -1;
		ss->ceilinglightsec = -1;
		ss->ffloors = NULL;
		ss->lightlist = NULL;
		ss->numlights = 0;
		ss->attached = NULL;
		ss->numattached = 0;
		ss->moved = true;
		ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
		ss->bottommap = ss->midmap = ss->topmap = -1;
		ss->gravity = NULL;

		// ----- for special tricks with HW renderer -----
		ss->pseudoSector = false;
		ss->virtualFloor = false;
		ss->virtualCeiling = false;
		ss->sectorLines = NULL;
		ss->stackList = NULL;
		ss->lineoutLength = -1.0;
		// ----- end special tricks -----
	}

	Z_Free(data);

	// set the sky flat num
	skyflatnum = P_AddLevelFlat("F_SKY1", foundflats);

	// copy table for global usage
	levelflats = Z_Malloc(numlevelflats * sizeof(levelflat_t), PU_LEVEL, 0);
	memcpy(levelflats, foundflats, numlevelflats * sizeof(levelflat_t));

	// search for animated flats and set up
	P_SetupLevelFlatAnims();
}

static void P_LoadSectors(int lump)
{
	byte* data;
	int i;
	mapsector_t* ms;
	sector_t* ss;
	levelflat_t* foundflats;

	numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
	if(numsectors <= 0)
		I_Error("Level has no sectors");
	sectors = Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
	memset(sectors, 0, numsectors*sizeof(sector_t));
	data = W_CacheLumpNum(lump,PU_STATIC);

	//Fab:FIXME: allocate for whatever number of flats
	//           512 different flats per level should be plenty
	foundflats = alloca(sizeof(levelflat_t) * MAXLEVELFLATS);
	if(!foundflats)
		I_Error("Ran out of memory while loading sectors\n");
	memset(foundflats, 0, sizeof(levelflat_t) * MAXLEVELFLATS);

	numlevelflats = 0;

	ms = (mapsector_t*)data;
	ss = sectors;
	for(i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

		//
		//  flats
		//
		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);

		ss->thinglist = NULL;
		ss->touching_thinglist = NULL;
		ss->preciplist = NULL;
		ss->touching_preciplist = NULL;

		ss->heightsec = -1;
		ss->altheightsec = 0;
		ss->floorlightsec = -1;
		ss->ceilinglightsec = -1;
		ss->ffloors = NULL;
		ss->lightlist = NULL;
		ss->numlights = 0;
		ss->attached = NULL;
		ss->numattached = 0;
		ss->moved = true;
		ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
		ss->bottommap = ss->midmap = ss->topmap = -1;
		ss->gravity = NULL;

		// ----- for special tricks with HW renderer -----
		ss->pseudoSector = false;
		ss->virtualFloor = false;
		ss->virtualCeiling = false;
		ss->sectorLines = NULL;
		ss->stackList = NULL;
		ss->lineoutLength = -1.0;
		// ----- end special tricks -----

		// Keep players out of secret levels!
		if(ss->tag == 424 && !(grade & 8)) // Mario
			I_Error("You need to unlock this level first!\n");
		else if(ss->tag == 425 && !(grade & 16)) // NiGHTS
			I_Error("You need to unlock this level first!\n");
		else if(ss->tag == 426 && !(grade & 32)) // Christmas Hunt
			I_Error("You need to unlock this level first!\n");
		else if(ss->tag == 427 && !(grade & 64)) // Adventure
			I_Error("You need to unlock this level first!\n");
		else if(ss->tag == 428 && !(grade & 1024)) // Ultimate award
			I_Error("You need to unlock this level first!\n");
	}

	Z_Free(data);

	// set the sky flat num
	skyflatnum = P_AddLevelFlat("F_SKY1", foundflats);

	// copy table for global usage
	levelflats = Z_Malloc(numlevelflats * sizeof(levelflat_t), PU_LEVEL, 0);
	memcpy(levelflats, foundflats, numlevelflats * sizeof(levelflat_t));

	// search for animated flats and set up
	P_SetupLevelFlatAnims();
}

//
// P_LoadNodes
//
static void P_LoadNodes(int lump)
{
	byte* data;
	int i, j, k;
	mapnode_t* mn;
	node_t* no;

	numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
	if(numnodes <= 0)
		I_Error("Level has no nodes");
	nodes = Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
	data = W_CacheLumpNum(lump, PU_STATIC);

	mn = (mapnode_t*)data;
	no = nodes;

	for(i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = SHORT(mn->x)<<FRACBITS;
		no->y = SHORT(mn->y)<<FRACBITS;
		no->dx = SHORT(mn->dx)<<FRACBITS;
		no->dy = SHORT(mn->dy)<<FRACBITS;
		for(j = 0; j < 2; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for(k = 0; k < 4; k++)
				no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}

	Z_Free(data);
}

//
// P_LoadThings
//
static void P_LoadThings(int lump)
{
	int i;
	mapthing_t* mt;
	char* data;
	char* datastart;
	mobj_t* emblemmobj;

	data = datastart = W_CacheLumpNum(lump, PU_LEVEL);
	nummapthings = W_LumpLength(lump) / (5 * sizeof(short));
	mapthings = Z_Malloc(nummapthings * sizeof(mapthing_t), PU_LEVEL, NULL);

	tokenbits = 0;

	mt = mapthings;
	numhuntemeralds[0] = numhuntemeralds[1] = numhuntemeralds[2] = 0;
	for(i = 0; i < nummapthings; i++, mt++)
	{
		// Do spawn all other stuff.
		// Do this first so all the mapthing slots are filled!
		mt->x = READSHORT(data);
		mt->y = READSHORT(data);
		mt->angle = READSHORT(data);
		mt->type = READSHORT(data);
		mt->options = READSHORT(data);
		mt->mobj = NULL;

		if((mt->type <= 4028 && mt->type >= 4001) || mt->type == 11)
			mt->oldnum = mt->type;
		else
			mt->oldnum = 0;

		// Z for objects
		mt->z = (short)(R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)
			->sector->floorheight>>FRACBITS);

		P_SpawnMapThing(mt);
	}
	Z_Free(datastart);

	// random emeralds for hunt
	if(numhuntemeralds[0] && numhuntemeralds[1] && numhuntemeralds[2])
	{
		int whichone;

		whichone = P_Random() % numhuntemeralds[0];
		P_SpawnMobj(huntemeralds[0][whichone]->x<<FRACBITS,
			huntemeralds[0][whichone]->y<<FRACBITS,
			huntemeralds[0][whichone]->z<<FRACBITS, MT_EMERHUNT);

		whichone = P_Random() % numhuntemeralds[1];
		P_SpawnMobj(huntemeralds[1][whichone]->x<<FRACBITS,
			huntemeralds[1][whichone]->y<<FRACBITS,
			huntemeralds[1][whichone]->z<<FRACBITS, MT_EMESHUNT);

		whichone = P_Random() % numhuntemeralds[2];
		P_SpawnMobj(huntemeralds[2][whichone]->x<<FRACBITS,
			huntemeralds[2][whichone]->y<<FRACBITS,
			huntemeralds[2][whichone]->z<<FRACBITS, MT_EMETHUNT);
	}

	data = datastart = W_CacheLumpNum(lump, PU_LEVEL);

	// Run through the list of mapthings again to spawn hoops and rings
	mt = mapthings;
	for(i = 0; i<nummapthings; i++, mt++)
	{
		short thisx, thisy, thisangle, thistype, thisoptions;

		thisx = READSHORT(data);
		thisy = READSHORT(data);
		thisangle = READSHORT(data);
		thistype = READSHORT(data);
		thisoptions = READSHORT(data);

		switch(thistype)
		{
			case 57:
			case 37:
			case 2048:
			case 2010:
			case 2014:
			case 84:
			case 44:
			case 76:
			case 77:
			case 47:
			case 2007:
			case 2046:
			case 2047:
				mt->mobj = NULL;

				// Z for objects Tails 05-26-2002
				mt->z = (short)(R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)
					->sector->floorheight >> FRACBITS);

				P_SpawnHoopsAndRings (mt);
				break;
			default:
				break;
		}
	}
	Z_Free(datastart);

	// Now let's spawn those funky emblem things! Tails 12-08-2002
	if(netgame || multiplayer || (modifiedgame && !savemoddata)) // No cheating!!
		return;

	for(i = 0; i < numemblems - 2; i++)
	{
		if(emblemlocations[i].player != players[0].skin
			&& emblemlocations[i].player != 255)
			continue;

		if(emblemlocations[i].level != gamemap)
			continue;

		emblemmobj = P_SpawnMobj(emblemlocations[i].x<<FRACBITS, emblemlocations[i].y<<FRACBITS,
			emblemlocations[i].z<<FRACBITS, MT_EMBLEM);
		P_SetMobjStateNF(emblemmobj, emblemmobj->info->spawnstate);

		emblemmobj->health = i+1;

		if(emblemlocations[emblemmobj->health-1].collected)
		{
			P_UnsetThingPosition(emblemmobj);
			emblemmobj->flags |= MF_NOCLIP;
			emblemmobj->flags &= ~MF_SPECIAL;
			emblemmobj->flags |= MF_NOBLOCKMAP;
			emblemmobj->frame |= (tr_transmor<<FF_TRANSSHIFT);
			P_SetThingPosition(emblemmobj);
		}
		else
			emblemmobj->frame &= ~FF_TRANSMASK;
	}

	// Easter eggs!
	if(!eastermode)
		return;

	if(modifiedgame)
		return;

	for(i = 0; i < NUMEGGS; i++)
	{
		if(egglocations[i].level != gamemap)
			continue;

		if(foundeggs & 1<<i) // They already found this egg... drat!
			continue;

		emblemmobj = P_SpawnMobj(egglocations[i].x<<FRACBITS,egglocations[i].y<<FRACBITS,egglocations[i].z<<FRACBITS, MT_EASTEREGG);
		emblemmobj->health = 1<<i;
		emblemmobj->flags = (emblemmobj->flags & ~MF_TRANSLATION) | ((i % 13)<<MF_TRANSSHIFT);
	}
}

// Experimental groovy write function!
void P_WriteThings(int lump)
{
	int i;
	size_t length;
	mapthing_t* mt;
	char* data;
	char* datastart;
	byte* save_p;
	byte* savebuffer;

	data = datastart = W_CacheLumpNum(lump, PU_LEVEL);
	save_p = savebuffer = (byte*)malloc(nummapthings * sizeof(mapthing_t));

	if(!save_p)
	{
		CONS_Printf("No more free memory for thingwriting!\n");
		return;
	}

	mt = mapthings;
	for(i = 0; i < nummapthings; i++, mt++)
	{
		WRITESHORT(save_p, mt->x);
		WRITESHORT(save_p, mt->y);
		WRITESHORT(save_p, mt->angle);

		// For 4001-4028 player starts
		if(mt->oldnum != 0)
			WRITESHORT(save_p, mt->oldnum);
		else
			WRITESHORT(save_p, mt->type);

		WRITESHORT(save_p, mt->options);
	}

	Z_Free(datastart);

	length = save_p - savebuffer;

	FIL_WriteFile("newthings.lmp", savebuffer, length);
	free(savebuffer);
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
static void P_LoadLineDefs(int lump)
{
	byte* data;
	int i;
	maplinedef_t* mld;
	line_t* ld;
	vertex_t* v1;
	vertex_t* v2;

	numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
	if(numlines <= 0)
		I_Error("Level has no linedefs");
	lines = Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
	memset(lines, 0, numlines * sizeof(line_t));
	data = W_CacheLumpNum(lump, PU_STATIC);

	mld = (maplinedef_t*)data;
	ld = lines;
	for(i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);
		v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
		v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;

		if(!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if(!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else
		{
			if(FixedDiv(ld->dy , ld->dx) > 0)
				ld->slopetype = ST_POSITIVE;
			else
				ld->slopetype = ST_NEGATIVE;
		}

		if(v1->x < v2->x)
		{
			ld->bbox[BOXLEFT] = v1->x;
			ld->bbox[BOXRIGHT] = v2->x;
		}
		else
		{
			ld->bbox[BOXLEFT] = v2->x;
			ld->bbox[BOXRIGHT] = v1->x;
		}

		if(v1->y < v2->y)
		{
			ld->bbox[BOXBOTTOM] = v1->y;
			ld->bbox[BOXTOP] = v2->y;
		}
		else
		{
			ld->bbox[BOXBOTTOM] = v2->y;
			ld->bbox[BOXTOP] = v1->y;
		}

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);

		if(ld->sidenum[0] != -1 && ld->special)
			sides[ld->sidenum[0]].special = ld->special;
	}

	Z_Free(data);
}

static inline void P_LoadLineDefs2(void)
{
	int i;
	line_t* ld = lines;
	for(i = 0; i < numlines; i++, ld++)
	{
		if(ld->sidenum[0] != -1)
			ld->frontsector = sides[ld->sidenum[0]].sector;
		else
			ld->frontsector = 0;

		if(ld->sidenum[1] != -1)
			ld->backsector = sides[ld->sidenum[1]].sector;
		else
			ld->backsector = 0;
	}
}

//
// P_LoadSideDefs
//
static inline void P_LoadSideDefs(int lump)
{
	numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
	if(numsides <= 0)
		I_Error("Level has no sidedefs");
	sides = Z_Malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
	memset(sides, 0, numsides * sizeof(side_t));
}

// Delay loading texture names until after loaded linedefs.

static void P_LoadSideDefs2(int lump)
{
	byte* data = W_CacheLumpNum(lump,PU_STATIC);
	int i, num, mapnum;

	for(i = 0; i < numsides; i++)
	{
		register mapsidedef_t* msd = (mapsidedef_t*)data + i;
		register side_t* sd = sides + i;
		register sector_t* sec;

		sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

		// refined to allow colormaps to work as wall textures if invalid as colormaps
		// but valid as textures.

		sd->sector = sec = &sectors[SHORT(msd->sector)];

		if(M_CheckParm("-nocolormaps"))
		{
			switch(sd->special)
			{
				case 44: // 3D floor (solid, translucent)
				case 45: // Water (translucent)
				case 52: // 3D floor (intangible, translucent)
				case 74: // Water (translucent, no sides)
				case 77: // Platform (translucent, no sides)
				case 81: // Platform (translucent)
				case 82: // Crumbling platform (translucent, returns)
				case 83: // Crumbling platform (translucent, never returns)
				case 84: // Translucent spin bust block
				case 86: // Translucent shatter block
				case 44+256: // note these are all duplicated below
				case 45+256:
				case 52+256:
				case 74+256:
				case 77+256:
				case 81+256:
				case 82+256:
				case 83+256:
				case 84+256:
				case 86+256:
					if(msd->toptexture[0] == '#')
					{
						char* col = msd->toptexture;
						sd->toptexture = sd->bottomtexture =
							((col[1]-'0')*100 + (col[2]-'0')*10 + col[3]-'0') + 1;
					}
					else
						sd->toptexture = sd->bottomtexture = 0;
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					break;

				default: // normal cases
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->toptexture = R_TextureNumForName(msd->toptexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
					break;
			}
		}
		else
		{
			switch(sd->special)
			{
				case 242: // variable colormap via 242 linedef
				case 14+256: //SoM: 3/22/2000: New water type.
				case 14: // Tails
					if(rendermode == render_soft || rendermode == render_none)
					{
						num = R_CheckTextureNumForName(msd->toptexture);
						if(num == -1)
						{
							sec->topmap = mapnum = R_ColormapNumForName(msd->toptexture);
							sd->toptexture = 0;
						}
						else
							sd->toptexture = num;

						num = R_CheckTextureNumForName(msd->midtexture);
						if(num == -1)
						{
							sec->midmap = mapnum = R_ColormapNumForName(msd->midtexture);
							sd->midtexture = 0;
						}
						else
							sd->midtexture = num;

						num = R_CheckTextureNumForName(msd->bottomtexture);
						if(num == -1)
						{
							sec->bottommap = mapnum = R_ColormapNumForName(msd->bottomtexture);
							sd->bottomtexture = 0;
						}
						else
							sd->bottomtexture = num;

						break;
					}
#ifdef HWRENDER
					else
					{
						if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
							sd->toptexture = 0;
						else
							sd->toptexture = num;

						if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
							sd->midtexture = 0;
						else
							sd->midtexture = num;

						if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
							sd->bottomtexture = 0;
						else
							sd->bottomtexture = num;

						break;
					}
#endif

				case 272: //SoM: 4/4/2000: Just colormap transfer
				case 16: // Tails
					// SoM: R_CreateColormap will only create a colormap in software mode...
					// Perhaps we should just call it instead of doing the calculations here.
					if(rendermode == render_soft || rendermode == render_none)
					{
						if(msd->toptexture[0] == '#' || msd->bottomtexture[0] == '#')
						{
							sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture,
								msd->bottomtexture);
							sd->toptexture = sd->bottomtexture = 0;
						}
						else
						{
							if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
								sd->toptexture = 0;
							else
								sd->toptexture = num;
							if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
								sd->midtexture = 0;
							else
								sd->midtexture = num;
							if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
								sd->bottomtexture = 0;
							else
								sd->bottomtexture = num;
						}
						break;
				}
#ifdef HWRENDER
				else
				{
					// for now, full support of toptexture only
					if(msd->toptexture[0] == '#')
					{
						char* col = msd->toptexture;

						sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture,
							msd->bottomtexture);
						sd->toptexture = sd->bottomtexture = 0;
#define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : 0)
						sec->extra_colormap = &extra_colormaps[sec->midmap];
						sec->extra_colormap->rgba =
							(HEX2INT(col[1]) << 4) + (HEX2INT(col[2]) << 0) +
							(HEX2INT(col[3]) << 12) + (HEX2INT(col[4]) << 8) +
							(HEX2INT(col[5]) << 20) + (HEX2INT(col[6]) << 16) +
							(ALPHA2INT(col[7]) << 24);
#undef ALPHA2INT
#undef HEX2INT
					}
					else
					{
						if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
							sd->toptexture = 0;
						else
							sd->toptexture = num;

						if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
							sd->midtexture = 0;
						else
							sd->midtexture = num;

						if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
							sd->bottomtexture = 0;
						else
							sd->bottomtexture = num;
					}
					break;
				}
#endif

				case 289:
					num = R_CheckTextureNumForName(msd->midtexture);
					if(num == -1)
						sd->midtexture = 1;
					else
						sd->midtexture = num;

					num = R_CheckTextureNumForName(msd->toptexture);
					if(num == -1)
						sd->toptexture = 1;
					else
						sd->toptexture = num;

					num = R_CheckTextureNumForName(msd->bottomtexture);
					if(num == -1)
						sd->bottomtexture = 1;
					else
						sd->bottomtexture = num;
					break;

				//Hurdler: added for alpha value with translucent 3D-floors/water
				case 44: // 3D floor (solid, translucent)
				case 45: // Water (translucent)
				case 52: // 3D floor (intangible, translucent)
				case 74: // Water (translucent, no sides)
				case 77: // Platform (translucent, no sides)
				case 81: // Platform (translucent)
				case 82: // Crumbling platform (translucent, returns)
				case 83: // Crumbling platform (translucent, never returns)
				case 84: // Translucent spin bust block
				case 86: // Translucent shatter block
				case 44+256: // note these are all duplicated above
				case 45+256:
				case 52+256:
				case 74+256:
				case 77+256:
				case 81+256:
				case 82+256:
				case 83+256:
				case 84+256:
				case 86+256:
					if(msd->toptexture[0] == '#')
					{
						char* col = msd->toptexture;
						sd->toptexture = sd->bottomtexture =
							((col[1]-'0')*100 + (col[2]-'0')*10 + col[3]-'0') + 1;
					}
					else
						sd->toptexture = sd->bottomtexture = 0;
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					break;

				default: // normal cases
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->toptexture = R_TextureNumForName(msd->toptexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
					break;
			}
		}
	}

	Z_Free(data);
}

//
// killough 10/98:
//
// Rewritten to use faster algorithm.
//
// New procedure uses Bresenham-like algorithm on the linedefs, adding the
// linedef to each block visited from the beginning to the end of the linedef.
//
// The algorithm's complexity is on the order of nlines*total_linedef_length.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
static void P_CreateBlockMap(void)
{
	register unsigned int i;
	fixed_t minx = MAXINT, miny = MAXINT, maxx = MININT, maxy = MININT;

	// First find limits of map

	// Shut up, compiler! SHUT UP!
	for(i = 0; i < (unsigned)numvertexes; i++)
	{
		if(vertexes[i].x>>FRACBITS < minx)
			minx = vertexes[i].x>>FRACBITS;
		else if(vertexes[i].x>>FRACBITS > maxx)
			maxx = vertexes[i].x>>FRACBITS;
		if(vertexes[i].y>>FRACBITS < miny)
			miny = vertexes[i].y>>FRACBITS;
		else if(vertexes[i].y>>FRACBITS > maxy)
			maxy = vertexes[i].y>>FRACBITS;
	}

	// Save blockmap parameters
	bmaporgx = minx << FRACBITS;
	bmaporgy = miny << FRACBITS;
	bmapwidth = ((maxx-minx) >> MAPBTOFRAC) + 1;
	bmapheight = ((maxy-miny) >> MAPBTOFRAC) + 1;

	// Compute blockmap, which is stored as a 2d array of variable-sized lists.
	//
	// Pseudocode:
	//
	// For each linedef:
	//
	//   Map the starting and ending vertices to blocks.
	//
	//   Starting in the starting vertex's block, do:
	//
	//     Add linedef to current block's list, dynamically resizing it.
	//
	//     If current block is the same as the ending vertex's block, exit loop.
	//
	//     Move to an adjacent block by moving towards the ending block in
	//     either the x or y direction, to the block which contains the linedef.

	{
		typedef struct
		{
			int n, nalloc;
			int* list;
		} bmap_t; // blocklist structure

		unsigned tot = bmapwidth * bmapheight; // size of blockmap
		bmap_t* bmap = calloc(sizeof *bmap, tot); // array of blocklists

		for(i = 0; i < (unsigned)numlines; i++)
		{
			// starting coordinates
			int x = (lines[i].v1->x >> FRACBITS) - minx;
			int y = (lines[i].v1->y >> FRACBITS) - miny;

			// x-y deltas
			int adx = lines[i].dx >> FRACBITS, dx = adx < 0 ? -1 : 1;
			int ady = lines[i].dy >> FRACBITS, dy = ady < 0 ? -1 : 1;

			// difference in preferring to move across y (>0) instead of x (<0)
			int diff = !adx ? 1 : !ady ? -1 : (((x >> MAPBTOFRAC) << MAPBTOFRAC) +
				(dx > 0 ? MAPBLOCKUNITS-1 : 0) - x) * (ady = abs(ady)) * dx -
				(((y >> MAPBTOFRAC) << MAPBTOFRAC) +
				(dy > 0 ? MAPBLOCKUNITS-1 : 0) - y) * (adx = abs(adx)) * dy;

			// starting block, and pointer to its blocklist structure
			int b = (y >> MAPBTOFRAC) * bmapwidth + (x >> MAPBTOFRAC);

			// ending block
			int bend = (((lines[i].v2->y >> FRACBITS) - miny) >> MAPBTOFRAC) *
				bmapwidth + (((lines[i].v2->x >> FRACBITS) - minx) >> MAPBTOFRAC);

			// delta for pointer when moving across y
			dy *= bmapwidth;

			// deltas for diff inside the loop
			adx <<= MAPBTOFRAC;
			ady <<= MAPBTOFRAC;

			// Now we simply iterate block-by-block until we reach the end block.
			while((unsigned)b < tot) // failsafe -- should ALWAYS be true
			{
				// Increase size of allocated list if necessary
				if(bmap[b].n >= bmap[b].nalloc)
				{
					// Graue 02-29-2004: make code more readable, don't realloc a null pointer
					// (because it crashes for me, and because the comp.lang.c FAQ says so)
					if(bmap[b].nalloc == 0)
					{
						bmap[b].nalloc = 8;
						bmap[b].list = malloc(bmap[b].nalloc * sizeof(*bmap->list));
					}
					else
					{
						bmap[b].nalloc = bmap[b].nalloc * 2;
						bmap[b].list = realloc(bmap[b].list, bmap[b].nalloc * sizeof(*bmap->list));
					}
				}

				// Add linedef to end of list
				bmap[b].list[bmap[b].n++] = i;

				// If we have reached the last block, exit
				if(b == bend)
					break;

				// Move in either the x or y direction to the next block
				if(diff < 0)
					diff += ady, b += dx;
				else
					diff -= adx, b += dy;
			}
		}

		// Compute the total size of the blockmap.
		//
		// Compression of empty blocks is performed by reserving two offset words
		// at tot and tot+1.
		//
		// 4 words, unused if this routine is called, are reserved at the start.
		{
			int count = tot + 6; // we need at least 1 word per block, plus reserved's

			for(i = 0; i < tot; i++)
				if(bmap[i].n)
					count += bmap[i].n + 2; // 1 header word + 1 trailer word + blocklist

			// Allocate blockmap lump with computed count
			blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);
		}

		// Now compress the blockmap.
		{
			int ndx = tot += 4; // Advance index to start of linedef lists
			bmap_t* bp = bmap; // Start of uncompressed blockmap

			blockmaplump[ndx++] = 0; // Store an empty blockmap list at start
			blockmaplump[ndx++] = -1; // (Used for compression)

			for(i = 4; i < tot; i++, bp++)
				if(bp->n) // Non-empty blocklist
				{
					blockmaplump[blockmaplump[i] = ndx++] = 0; // Store index & header
					do
						blockmaplump[ndx++] = bp->list[--bp->n]; // Copy linedef list
					while(bp->n);
					blockmaplump[ndx++] = -1; // Store trailer
					free(bp->list); // Free linedef list
				}
				else // Empty blocklist: point to reserved empty blocklist
					blockmaplump[i] = tot;

			free(bmap); // Free uncompressed blockmap
		}
	}
	{
		int count;
		// clear out mobj chains (copied from from P_LoadBlockMap)
		count = sizeof(*blocklinks) * bmapwidth * bmapheight;
		blocklinks = Z_Malloc(count,PU_LEVEL, 0);
		memset(blocklinks, 0, count);
		blockmap = blockmaplump + 4;
	}
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
static void P_GroupLines(void)
{
	line_t** linebuffer;
	int i, j, total;
	line_t* li;
	sector_t* sector;
	subsector_t* ss;
	seg_t* seg;
	fixed_t bbox[4];

	// look up sector number for each subsector
	ss = subsectors;
	for(i = 0; i < numsubsectors; i++, ss++)
	{
		if(ss->firstline < 0 || ss->firstline >= numsegs)
			CorruptMapError(va("P_GroupLines: ss->firstline invalid "
				"(subsector %d, firstline refers to %d of %d)", ss - subsectors, ss->firstline,
				numsegs));
		seg = &segs[ss->firstline];
		if(!seg->sidedef)
			CorruptMapError(va("P_GroupLines: seg->sidedef is NULL "
				"(subsector %d, firstline is %d)", ss - subsectors, ss->firstline));
		if(seg->sidedef - sides < 0 || seg->sidedef - sides > numsides)
			CorruptMapError(va("P_GroupLines: seg->sidedef refers to sidedef %d of %d "
				"(subsector %d, firstline is %d)", seg->sidedef - sides, numsides,
				ss - subsectors, ss->firstline));
		if(!seg->sidedef->sector)
			CorruptMapError(va("P_GroupLines: seg->sidedef->sector is NULL "
				"(subsector %d, firstline is %d, sidedef is %d)", ss - subsectors, ss->firstline,
				seg->sidedef - sides));
		ss->sector = seg->sidedef->sector;
	}

	// count number of lines in each sector
	li = lines;
	total = 0;
	for(i = 0; i < numlines; i++, li++)
	{
		total++;
		li->frontsector->linecount++;

		if(li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}

	// build line tables for each sector
	linebuffer = Z_Malloc(total * 4, PU_LEVEL, 0);
	sector = sectors;
	for(i = 0; i < numsectors; i++, sector++)
	{
		M_ClearBox(bbox);
		sector->lines = linebuffer;
		li = lines;
		for(j = 0; j < numlines; j++, li++)
		{
			if(li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox(bbox, li->v1->x, li->v1->y);
				M_AddToBox(bbox, li->v2->x, li->v2->y);
			}
		}
		if(linebuffer - sector->lines != sector->linecount)
			CorruptMapError("P_GroupLines: miscounted");

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT])/2;
		sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM])/2;
	}
}

#if 0
static char* levellumps[] =
{
	"label",        // ML_LABEL,    A separator, name, MAPxx
	"THINGS",       // ML_THINGS,   Enemies, items..
	"LINEDEFS",     // ML_LINEDEFS, Linedefs, from editing
	"SIDEDEFS",     // ML_SIDEDEFS, Sidedefs, from editing
	"VERTEXES",     // ML_VERTEXES, Vertices, edited and BSP splits generated
	"SEGS",         // ML_SEGS,     Linesegs, from linedefs split by BSP
	"SSECTORS",     // ML_SSECTORS, Subsectors, list of linesegs
	"NODES",        // ML_NODES,    BSP nodes
	"SECTORS",      // ML_SECTORS,  Sectors, from editing
	"REJECT",       // ML_REJECT,   LUT, sector-sector visibility
};

/** Checks a lump and returns whether it is a valid start-of-level marker.
  *
  * \param lumpnum Lump number to check.
  * \return True if the lump is a valid level marker, false if not.
  */
static inline boolean P_CheckLevel(int lumpnum)
{
	int i, file, lump;

	for(i = ML_THINGS; i <= ML_REJECT; i++)
	{
		file = lumpnum >> 16;
		lump = (lumpnum & 0xffff) + i;
		if(file > numwadfiles || lump > wadfiles[file]->numlumps ||
			strncmp(wadfiles[file]->lumpinfo[lump].name, levellumps[i], 8) )
		return false;
	}
	return true; // all right
}
#endif

/** Sets up a sky texture to use for the level.
  * The sky texture is used instead of F_SKY1.
  */
void P_SetupLevelSky(int skynum)
{
	char skytexname[12];

	sprintf(skytexname, "SKY%d", skynum);
	skytexture = R_TextureNumForName(skytexname);
	levelskynum = skynum;

	// scale up the old skies, if needed
	R_SetupSkyDraw();
}

static char* maplumpname;
int lastloadedmaplumpnum; // for comparative savegame

/** Loads a level from a lump or external wad.
  *
  * \param map     Map number.
  * \param skill   Skill level.
  * \param wadname Name of an external wadfile to load on the fly, or "\2" to
  *                not spawn precipitation.
  * \todo Clean up, refactor, split up; get rid of the bloat.
  * \todo Remove ugly wadname "\2" hack for not spawning precipitation. The
  *       hack is used by P_NetUnArchiveMisc() to avoid precipitation getting
  *       clobbered during a net savegame. After this hack is removed, the
  *       wadname parameter will no longer be necessary.
  */
boolean P_SetupLevel(int map, skill_t skill, const char* wadname) // for wad files
{
	int i, loadprecip = 1;

	// This is needed. Don't touch.
	maptol = mapheaderinfo[gamemap-1].typeoflevel;

	skill = 0; //gameskill?
	HU_clearChatChars();

	CON_Drawer(); // let the user know what we are going to do
	I_FinishUpdate(); // page flip or blit buffer

	// Initialize sector node list.
	P_Initsecnode();

	totalitems = totalrings = timeinmap = 0;

	mapmusic = mapheaderinfo[gamemap-1].musicslot;

	if(cv_runscripts.value && mapheaderinfo[gamemap-1].scriptname[0] != '#')
	{
		if(mapheaderinfo[gamemap-1].scriptislump)
		{
			int lumpnum;
			char newname[9];

			strncpy(newname, mapheaderinfo[gamemap-1].scriptname, 8);

			newname[8] = '\0';

			lumpnum = W_CheckNumForName(newname);

			if(lumpnum == -1 || W_LumpLength(lumpnum) <= 0)
			{
				CONS_Printf("SOC Error: script lump %s not found/not valid.\n", newname);
				goto noscript;
			}

			COM_BufInsertText(W_CacheLumpNum(lumpnum, PU_CACHE));
		}
		else
			COM_BufAddText(va("exec %s\n", mapheaderinfo[gamemap-1].scriptname));
	}
noscript:

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(netgame || multiplayer)
		{
			// In Co-Op, replenish a user's lives if they are depleted.
			if(players[i].lives <= 0)
			{
				switch(gameskill)
				{
					case sk_insane:
						players[i].lives = 1;
						break;
					case sk_nightmare:
					case sk_hard:
					case sk_medium:
						players[i].lives = 3;
						break;
					case sk_easy:
						players[i].lives = 5;
						break;
					case sk_baby:
						players[i].lives = 9;
						break;
					default: // Oops!?
						CONS_Printf("ERROR: GAME SKILL UNDETERMINED!");
						break;
				}
			}
		}

		countdown = countdown2 = 0;

		players[i].xtralife = players[i].deadtimer = players[i].numboxes = players[i].totalring = players[i].mare = 0;
        players[i].health = 1;

		if(gametype == GT_RACE && players[i].lives < 3)
			players[i].lives = 3;

		players[i].exiting = 0;
		if(!modifiedgame && players[i].dbginfo && gamemap == 0x01)
			players[i].dbginfo = 0;
		P_ResetPlayer(&players[i]);

		players[i].mo = NULL;
#ifdef CLIENTPREDICTION2
		players[i].spirit = NULL;
#endif
	}

	hunt1 = hunt2 = hunt3 = NULL;

	if(mapheaderinfo[gamemap-1].forcecharacter != 255)
	{
		char skincmd[33];
		if(cv_splitscreen.value)
		{
			sprintf(skincmd, "skin2 %s\n", skins[mapheaderinfo[gamemap-1].forcecharacter].name);
			CV_Set(&cv_skin2, skins[mapheaderinfo[gamemap-1].forcecharacter].name);
		}

		sprintf(skincmd, "skin %s\n", skins[mapheaderinfo[gamemap-1].forcecharacter].name);

		COM_BufAddText(skincmd);

		if(!netgame)
		{
			if(cv_splitscreen.value)
			{
				SetPlayerSkinByNum(secondarydisplayplayer, mapheaderinfo[gamemap-1].forcecharacter);
				if(cv_playercolor2.value != players[secondarydisplayplayer].prefcolor)
				{
					CV_StealthSetValue(&cv_playercolor2, players[secondarydisplayplayer].prefcolor);
					players[secondarydisplayplayer].skincolor = players[secondarydisplayplayer].prefcolor;

					// a copy of color
					if(players[secondarydisplayplayer].mo)
						players[secondarydisplayplayer].mo->flags = (players[secondarydisplayplayer].mo->flags & ~MF_TRANSLATION) | ((players[secondarydisplayplayer].skincolor)<<MF_TRANSSHIFT);
				}
			}

			SetPlayerSkinByNum(consoleplayer, mapheaderinfo[gamemap-1].forcecharacter);
			// normal player colors in single player
			if(cv_playercolor.value != players[consoleplayer].prefcolor)
			{
				CV_StealthSetValue(&cv_playercolor, players[consoleplayer].prefcolor);
				players[consoleplayer].skincolor = players[consoleplayer].prefcolor;

				// a copy of color
				if(players[consoleplayer].mo)
					players[consoleplayer].mo->flags = (players[consoleplayer].mo->flags & ~MF_TRANSLATION) | ((players[consoleplayer].skincolor)<<MF_TRANSSHIFT);
			}
		}
	}

	if(!dedicated)
	{
		// chasecam on in chaos, race, coop
		// chasecam off in match, tag, capture the flag
		if(!cv_chasecam.changed)
			CV_SetValue(&cv_chasecam,
				(gametype == GT_RACE || gametype == GT_CHAOS || gametype == GT_COOP));

		// same for second player
		if(!cv_chasecam2.changed)
			CV_SetValue(&cv_chasecam2, cv_chasecam.value);
    }

	// mario mode
	if(maptol & TOL_MARIO)
		mariomode = true;
	else
		mariomode = false;

	// 2d mode
	if(maptol & TOL_2D)
		twodlevel = true;
	else
		twodlevel = false;

	// xmas mode
	if(maptol & TOL_XMAS)
		xmasmode = true;
	else if(!xmasoverride)
		xmasmode = false;

	// Initial height of PointOfView
	// will be set by player think.
	players[consoleplayer].viewz = 1;

	// Make sure all sounds are stopped before Z_FreeTags.
	S_StopSounds();

	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

#if defined (WALLSPLATS) || defined(FLOORSPLATS)
	// clear the splats from previous level
	R_ClearLevelSplats();
#endif

	P_InitThinkers();

	/// \note very ugly hack for not spawning precipitation here
	if(wadname && wadname[0] == '\2')
	{
		wadname = NULL;
		loadprecip = 0;
	}

	// load the map from an internal game resource or external wad file
	if(wadname)
	{
		char* firstmap = NULL;

		// go back to title screen if no map is loaded
		if(!P_AddWadFile(wadname, &firstmap) || firstmap == NULL)
			return false; // no maps were found

		// P_AddWadFile() sets lumpname
		lastloadedmaplumpnum = W_GetNumForName(firstmap);
		maplumpname = firstmap;
	}
	else
	{
		// internal game map
		lastloadedmaplumpnum = W_GetNumForName(maplumpname = G_BuildMapName(map));
	}

	leveltime = 0;

	if(mapheaderinfo[gamemap-1].countdown)
		countdowntimer = mapheaderinfo[gamemap-1].countdown * TICRATE;

	countdowntimeup = false; // DuuuuuuuuuhhhH!H!H!!

	R_ClearColormaps();

	// We've loaded the music lump, start the music.
	S_Start();

	// now part of level loading since in future each level may have
	// its own anim texture sequences, switches etc.
	P_InitPicAnims();

	// SRB2 determines the sky texture to be used depending on the map header.
	P_SetupLevelSky(mapheaderinfo[gamemap-1].skynum);
	globallevelskynum = levelskynum;

	// note: most of this ordering is important

	if(!modifiedgame && gamemap == 0x10)
	{
		if(!players[0].dbginfo)
			I_Error("DBGInfo == 0!");

		P_LoadDBGVertexes(lastloadedmaplumpnum + ML_VERTEXES);
		P_LoadDBGSectors(lastloadedmaplumpnum + ML_SECTORS);
	}
	else
	{
		P_LoadVertexes(lastloadedmaplumpnum + ML_VERTEXES);
		P_LoadSectors(lastloadedmaplumpnum + ML_SECTORS);
	}
	P_LoadSideDefs(lastloadedmaplumpnum + ML_SIDEDEFS);

	P_LoadLineDefs(lastloadedmaplumpnum + ML_LINEDEFS);
	P_CreateBlockMap(); // Graue 02-29-2004
	P_LoadSideDefs2(lastloadedmaplumpnum + ML_SIDEDEFS);


	R_MakeColormaps();
	P_LoadLineDefs2();
	P_LoadSubsectors(lastloadedmaplumpnum + ML_SSECTORS);
	P_LoadNodes(lastloadedmaplumpnum + ML_NODES);
	P_LoadSegs(lastloadedmaplumpnum + ML_SEGS);
	rejectmatrix = W_CacheLumpNum(lastloadedmaplumpnum + ML_REJECT, PU_LEVEL);
	P_GroupLines();

#ifdef HWRENDER // not win32 only 19990829 by Kin
	if(rendermode != render_soft && rendermode != render_none)
	{
		// BP: reset light between levels (we draw preview frame lights on current frame)
		HWR_ResetLights();
		// Correct missing sidedefs & deep water trick
		HWR_CorrectSWTricks();
		HWR_CreatePlanePolygons(numnodes - 1);
	}
#endif

	numdmstarts = numredctfstarts = numbluectfstarts = 0;
	// reset the player starts
	for(i = 0; i < MAXPLAYERS; i++)
		playerstarts[i] = NULL;

	P_LoadThings(lastloadedmaplumpnum + ML_THINGS);

	for(numcoopstarts = 0; numcoopstarts < MAXPLAYERS; numcoopstarts++)
		if(!playerstarts[numcoopstarts])
			break;

	// set up world state
	P_SpawnSpecials();

	if(loadprecip) //  ugly hack for P_NetUnArchiveMisc (and P_LoadNetGame)
		P_SpawnPrecipitation();

	globalweather = mapheaderinfo[gamemap-1].weather;

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i])
		{
			if(gametype == GT_MATCH || gametype == GT_CHAOS)
			{
				players[i].mo = NULL;
				G_DoReborn(i);
			}
			else // gametype is GT_COOP or GT_RACE
			{
				players[i].mo = NULL;

				if(players[i].starposttime)
				{
					G_CoopSpawnPlayer(i, true);
					P_ClearStarPost(&players[i], players[i].starpostnum);
				}
				else
					G_CoopSpawnPlayer(i, false);
			}
		}


	if(!dedicated)
	{
		camera.x = players[displayplayer].mo ? players[displayplayer].mo->x : 0;
		camera.y = players[displayplayer].mo ? players[displayplayer].mo->y : 0;
		camera.z = players[displayplayer].mo ? players[displayplayer].mo->z : 0;
		camera.angle = players[displayplayer].mo ? players[displayplayer].mo->angle : 0;

		if(!cv_cam_height.changed)
			CV_Set(&cv_cam_height, cv_cam_height.defaultvalue);

		if(!cv_cam_dist.changed)
			CV_Set(&cv_cam_dist, cv_cam_dist.defaultvalue);

		if(!cv_cam_rotate.changed)
			CV_Set(&cv_cam_rotate, cv_cam_rotate.defaultvalue);

		if(!cv_cam2_height.changed)
			CV_Set(&cv_cam2_height, cv_cam2_height.defaultvalue);

		if(!cv_cam2_dist.changed)
			CV_Set(&cv_cam2_dist, cv_cam2_dist.defaultvalue);

		if(!cv_cam2_rotate.changed)
			CV_Set(&cv_cam2_rotate, cv_cam2_rotate.defaultvalue);

		if(!cv_useranalog.value)
		{
			if(!cv_analog.changed)
				CV_SetValue(&cv_analog, maptol & TOL_ADVENTURE ? 1 : 0);
			if(!cv_analog2.changed)
				CV_SetValue(&cv_analog2, maptol & TOL_ADVENTURE ? 1 : 0);
		}

#ifdef HWRENDER
		if(rendermode != render_soft && rendermode != render_none)
			CV_Set(&cv_grfov, cv_grfov.defaultvalue);
#endif

		displayplayer = consoleplayer; // Start with your OWN view, please!
	}

	if(server)
	{
		CV_SetValue(&cv_homing, false);
		CV_SetValue(&cv_lightdash, false);
	}

	if(maptol & TOL_ADVENTURE)
	{
		CV_SetValue(&cv_cam_height, 64);
		CV_SetValue(&cv_cam_dist, 192);

		if(server)
		{
			CV_SetValue(&cv_homing, true);
			CV_SetValue(&cv_lightdash, true);
		}

		if(!netgame)
		{
			if(cv_splitscreen.value)
				CV_SetValue(&cv_analog2, true);

			CV_SetValue(&cv_analog, true);
		}
	}

	if(cv_useranalog.value)
		CV_SetValue(&cv_analog, true);

	if(cv_splitscreen.value && cv_useranalog2.value)
		CV_SetValue(&cv_analog2, true);

	if(twodlevel)
	{
		CV_SetValue(&cv_cam_dist, 320);
		CV_SetValue(&cv_analog2, false);
		CV_SetValue(&cv_analog, false);
	}

	// clear special respawning que
	iquehead = iquetail = 0;

	// Fab:19-07-98:start cd music for this level (note: can be remapped)
	I_PlayCD(map + 1, false);

	// preload graphics
#ifdef HWRENDER // not win32 only 19990829 by Kin
	if(rendermode != render_soft && rendermode != render_none)
	{
		HWR_PrepLevelCache(numtextures);
		HWR_CreateStaticLightmaps(numnodes - 1);
	}
#endif

	if(precache || dedicated)
		R_PrecacheLevel();

	nextmapoverride = 0;
	nextmapgametype = -1;
	skipstats = false;

	if(!(netgame || multiplayer) && (!modifiedgame || savemoddata))
		mapvisited[gamemap-1] = true;

	return true;
}

//
// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
boolean P_AddWadFile(const char* wadfilename, char** firstmapname)
{
	int i, j, wadfilenum, replaces;
	short firstmapreplaced, num;
	wadfile_t* wadfile;
	char* name;
	lumpinfo_t* lumpinfo;
	boolean texturechange;
	char addwadfile[MAX_WADPATH];

	strcpy(addwadfile,wadfilename);

	if((wadfilenum = W_LoadWadFile(addwadfile)) == -1)
	{
		CONS_Printf("couldn't load wad file %s\n", wadfilename);
		return false;
	}
	wadfile = wadfiles[wadfilenum];

	//
	// search for sound replacements
	//
	lumpinfo = wadfile->lumpinfo;
	replaces = 0;
	texturechange = false;
	for(i = 0; i < wadfile->numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		if(name[0] == 'D' && name[1] == 'S')
		{
			for(j = 1; j < NUMSFX; j++)
			{
				if(S_sfx[j].name && !strnicmp(S_sfx[j].name, name + 2, 6))
				{
					// the sound will be reloaded when needed,
					// since sfx->data will be NULL
					if(devparm)
						CONS_Printf("Sound %.8s replaced\n", name);

					I_FreeSfx(&S_sfx[j]);

					replaces++;
				}
			}
		}
		else if(!memcmp(name, "TEXTURE1", 8) || !memcmp(name, "TEXTURE2", 8)
			|| !memcmp(name, "PNAMES", 6))
			texturechange = true;
	}
	if(!devparm && replaces)
		CONS_Printf("%d sounds replaced\n", replaces);

	//
	// search for music replacements
	//
	lumpinfo = wadfile->lumpinfo;
	replaces = 0;
	for(i = 0; i < wadfile->numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		if(name[0] == 'D' && name[1] == '_')
		{
			if(devparm)
				CONS_Printf("Music %.8s replaced\n", name);
			replaces++;
		}
	}
	if(!devparm && replaces)
		CONS_Printf("%d musics replaced\n", replaces);

	//
	// search for sprite replacements
	//
	R_AddSpriteDefs(sprnames, numwadfiles - 1);

	//
	// search for texturechange replacements
	//
	if(texturechange) // initialized in the sound check
		R_LoadTextures(); // numtexture changes
	else
		R_FlushTextureCache(); // just reload it from file

	//
	// look for skins
	//
	R_AddSkins(wadfilenum); // faB: wadfile index in wadfiles[]

	//
	// search for maps
	//
	lumpinfo = wadfile->lumpinfo;
	firstmapreplaced = 0;
	for(i = 0; i < wadfile->numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		num = firstmapreplaced;

		if(name[0] == 'M' && name[1] == 'A' && name[2] == 'P') // Ignore the headers
		{
			num = (short)M_MapNumber(name[3], name[4]);

			if(name[5] == 'D')
				P_LoadMapHeader(num);
			else if(name[5]!='\0')
				continue;

			CONS_Printf("%s\n", name);
		}

		if(num && (num < firstmapreplaced || !firstmapreplaced))
		{
			firstmapreplaced = num;
			if(firstmapname)
				*firstmapname = name;
		}
	}
	if(!firstmapreplaced)
		CONS_Printf("no maps added\n");

	// reload status bar (warning should have valid player!)
	if(gamestate == GS_LEVEL)
		ST_Start();

	return true;
}
