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
/// \brief Implements special effects:
///	 - Texture animation, height or lighting changes
///	 according to adjacent sectors, respective
///	 utility functions, etc.
///	 - Line Tag handling. Line and Sector triggers.

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "p_setup.h" // levelflats for flat animation
#include "r_data.h"
#include "m_random.h"
#include "p_mobj.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "dstrings.h" //SoM: 3/10/2000
#include "r_main.h" //Two extra includes.
#include "r_sky.h"

#include "hardware/hw3sound.h"

static void P_AddFakeFloorsByLine(int line, int ffloorflags);
static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void Add_Pusher(pushertype_e type, int x_mag, int y_mag, mobj_t* source, int affectee, int referrer); //SoM: 3/9/2000
static void P_AddBlockThinker(sector_t* sec, sector_t* actionsector, line_t* sourceline);
static void P_AddFloatThinker(sector_t* sec, sector_t* actionsector);

/** Animated texture descriptor
  * This keeps track of an animated texture or an animated flat.
  * \sa P_UpdateSpecials, P_InitPicAnims, animdef_t
  */
typedef struct
{
	boolean istexture; ///< ::true for a texture, ::false for a flat
	int picnum;        ///< The end flat number
	int basepic;       ///< The start flat number
	int numpics;       ///< Number of frames in the animation
	int speed;         ///< Number of tics for which each frame is shown
} anim_t;

#ifndef __GNUC__
#pragma pack(1) //Hurdler: 04/04/2000: I think pragma is more portable
#endif

/** Animated texture definition.
  * Used for ::harddefs and for loading an ANIMATED lump from a wad.
  *
  * Animations are defined by the first and last frame (i.e., flat or texture).
  * The animation sequence uses all flats between the start and end entry, in
  * the order found in the wad.
  *
  * \sa anim_t
  */
typedef struct
{
	char istexture    /*ATTRPACK*/; ///< True for a texture, false for a flat.
	char endname[9]     ATTRPACK;   ///< Name of the last frame, null-terminated.
	char startname[9] /*ATTRPACK*/; ///< Name of the first frame, null-terminated.
	int speed           ATTRPACK;   ///< Number of tics for which each frame is shown.
} animdef_t;

#ifndef __GNUC__
#pragma pack()
#endif

#define MAXANIMS 64

//SoM: 3/7/2000: New sturcture without limits.
static anim_t* lastanim;
static anim_t* anims;
static size_t maxanims;

//
// P_InitPicAnims
//
/** Hardcoded animation sequences.
  * Used if no ANIMATED lump is found in a loaded wad.
  */
static animdef_t harddefs[] =
{
	// DOOM II flat animations.
	{false,     "NUKAGE3",      "NUKAGE1",      4},
	{false,     "FWATER16",     "FWATER1",      4},
	{false,     "BWATER16",     "BWATER1",      4},
	{false,     "LWATER16",     "LWATER1",      4},
	{false,     "WATER7",       "WATER0",       4},
	{false,     "SWATER4",      "SWATER1",      8},
	{false,     "LAVA4",        "LAVA1",        8},
	{false,     "BLOOD3",       "BLOOD1",       8},

	{false,     "RROCK08",      "RROCK05",      8},
	{false,     "QUIKSN16",    "QUIKSN01",      4}, // Quicksand
	{false,     "CHEMG16",      "CHEMG01",      4}, // THZ Chemical gunk
	{false,     "GOOP16",       "GOOP01",       4}, // Green chemical gunk
	{false,     "SLIME08",      "SLIME05",      4},
	{false,     "THZBOXF4",     "THZBOXF1",     2}, // Moved up with the flats
	{false,     "ALTBOXF4",     "ALTBOXF1",     2},
	{false,     "CHEMG16",      "CHEMG01",      4}, // THZ Chemical gunk

	{false,     "BLUE3",        "BLUE1",        4},
	{false,     "GREY3",        "GREY1",        4},

	// animated textures
	{true,      "GFALL4",       "GFALL1",       2}, // Short waterfall
	{true,      "CFALL4",       "CFALL1",       2}, // Long waterfall
	{true,      "TFALL4",       "TFALL1",       2}, // THZ Chemical fall
	{true,      "AFALL4",       "AFALL1",       2}, // Green Chemical fall
	{true,      "THZBOX04",     "THZBOX01",     2},
	{true,      "ALTBOX04",     "ALTBOX01",     2},
	{true,      "SFALL4",       "SFALL1",       4}, // Lava fall
	{true,      "BFALL4",       "BFALL1",       2}, // HPZ waterfall
	{true,      "GREYW3",       "GREYW1",       4},
	{true,      "BLUEW3",       "BLUEW1",       4},
	{true,      "COMP6",        "COMP4",        4},
	{true,      "RED3",         "RED1",         4},
	{true,      "YEL3",         "YEL1",         4},

	// Begin dummy slots
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},
	{false,     "DUMYSLOT",    "DUMYSLOT",    8},

	{   -1,             "",            "",    0},
};

// Animating line specials

// Init animated textures
// - now called at level loading P_SetupLevel()

static animdef_t* animdefs;

/** Sets up texture and flat animations.
  *
  * Converts an ::animdef_t array loaded from ::harddefs or a lump into
  * ::anim_t format.
  *
  * Issues an error if any animation cycles are invalid.
  *
  * \sa P_FindAnimatedFlat, P_SetupLevelFlatAnims
  * \author Steven McGranahan
  */
void P_InitPicAnims(void)
{
	// Init animation
	int i;

	if(W_CheckNumForName("ANIMATED") != -1)
		animdefs = (animdef_t*)W_CacheLumpName("ANIMATED", PU_STATIC);
	else
		animdefs = harddefs;

	for (i = 0; animdefs[i].istexture != (char)-1; i++, maxanims++);

	anims = (anim_t*)malloc(sizeof(anim_t)*(maxanims + 1));
	if(!anims)
		I_Error("No free memory for ANIMATED data");

	lastanim = anims;
	for(i = 0; animdefs[i].istexture != (char)-1; i++)
	{
		if(animdefs[i].istexture)
		{
			if(R_CheckTextureNumForName(animdefs[i].startname) == -1)
				continue;

			lastanim->picnum = R_TextureNumForName(animdefs[i].endname);
			lastanim->basepic = R_TextureNumForName(animdefs[i].startname);
		}
		else
		{
			if((W_CheckNumForName(animdefs[i].startname)) == -1)
				continue;

			lastanim->picnum = R_FlatNumForName(animdefs[i].endname);
			lastanim->basepic = R_FlatNumForName(animdefs[i].startname);
		}

		lastanim->istexture = (boolean)animdefs[i].istexture;
		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		if(lastanim->numpics < 2)
		{
			free(anims);
			I_Error("P_InitPicAnims: bad cycle from %s to %s",
				animdefs[i].startname, animdefs[i].endname);
		}

		lastanim->speed = LONG(animdefs[i].speed) * NEWTICRATERATIO;
		lastanim++;
	}
	lastanim->istexture = (boolean)-1;

	if(animdefs != harddefs)
		Z_ChangeTag(animdefs, PU_CACHE);
}

/** Checks for flats in levelflats that are part of a flat animation sequence
  * and sets them up for animation.
  *
  * \param animnum Index into ::anims to find flats for.
  * \sa P_SetupLevelFlatAnims
  */
static inline void P_FindAnimatedFlat(int animnum)
{
	int i, startflatnum, endflatnum;
	levelflat_t* foundflats;

	foundflats = levelflats;
	startflatnum = anims[animnum].basepic;
	endflatnum = anims[animnum].picnum;

	// note: high word of lumpnum is the wad number
	if((startflatnum>>16) != (endflatnum>>16))
		I_Error("AnimatedFlat start %s not in same wad as end %s\n",
			animdefs[animnum].startname, animdefs[animnum].endname);

	//
	// now search through the levelflats if this anim flat sequence is used
	//
	for(i = 0; i < numlevelflats; i++, foundflats++)
	{
		// is that levelflat from the flat anim sequence ?
		if(foundflats->lumpnum >= startflatnum && foundflats->lumpnum <= endflatnum)
		{
			foundflats->baselumpnum = startflatnum;
			foundflats->animseq = foundflats->lumpnum - startflatnum;
			foundflats->numpics = endflatnum - startflatnum + 1;
			foundflats->speed = anims[animnum].speed;

			if(devparm)
				CONS_Printf("animflat: #%03d name:%.8s animseq:%d numpics:%d speed:%d\n",
					i, foundflats->name, foundflats->animseq,
					foundflats->numpics,foundflats->speed);
		}
	}
}

/** Sets up all flats used in a level.
  *
  * \sa P_InitPicAnims, P_FindAnimatedFlat
  */
void P_SetupLevelFlatAnims(void)
{
	int i;

	// the original game flat anim sequences
	for(i = 0; anims[i].istexture != (boolean)-1; i++)
	{
		if(!anims[i].istexture)
			P_FindAnimatedFlat(i);
	}
}

//
// UTILITIES
//

/** Gets a side from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the side_t of the side you want.
  * \sa getSector, twoSided, getNextSector
  */
side_t* getSide(int currentSector, int line, int side)
{
	return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

/** Gets a sector from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the ::sector_t of the sector on that side.
  * \sa getSide, twoSided, getNextSector
  */
sector_t* getSector(int currentSector, int line, int side)
{
	return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

/** Determines whether a sector line is two-sided.
  * Uses the Boom method, checking if the line's back side is set to -1, rather
  * than looking for ::ML_TWOSIDED.
  *
  * \param sector The sector.
  * \param line   Line index within the sector.
  * \return 1 if the sector is two-sided, 0 otherwise.
  * \sa getSide, getSector, getNextSector
  */
int twoSided(int sector, int line)
{
	return (sectors[sector].lines[line])->sidenum[1] != -1;
}

/** Finds sector next to current.
  *
  * \param line Pointer to the line to cross.
  * \param sec  Pointer to the current sector.
  * \return Pointer to a ::sector_t of the adjacent sector, or NULL if the line
  *         is one-sided.
  * \sa getSide, getSector, twoSided
  * \author Steven McGranahan
  */
sector_t* getNextSector(line_t* line, sector_t* sec)
{
	if(line->frontsector == sec)
	{
		if(line->backsector != sec)
			return line->backsector;
		else
			return NULL;
	}
	return line->frontsector;
}

/** Finds lowest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest floor height in an adjacent sector.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindLowestCeilingSurrounding
  */
fixed_t P_FindLowestFloorSurrounding(sector_t* sec)
{
	int i;
	line_t* check;
	sector_t* other;
	fixed_t floor;

	floor = sec->floorheight;

	for(i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if(!other)
			continue;

		if(other->floorheight < floor)
			floor = other->floorheight;
	}
	return floor;
}

/** Finds highest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest floor height in an adjacent sector.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindHighestCeilingSurrounding
  */
fixed_t P_FindHighestFloorSurrounding(sector_t* sec)
{
	int i;
	line_t* check;
	sector_t* other;
	fixed_t floor = -500*FRACUNIT;
	int foundsector = 0;

	for(i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if(!other)
			continue;

		if(other->floorheight > floor || !foundsector)
			floor = other->floorheight;

		if(!foundsector)
			foundsector = 1;
	}
	return floor;
}

/** Finds next highest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest floor height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindNextHighestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextHighestFloor(sector_t* sec, int currentheight)
{
	sector_t* other;
	int i;

	for(i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if(other && other->floorheight > currentheight)
		{
			int height = other->floorheight;
			while(++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if(other &&
					other->floorheight < height &&
					other->floorheight > currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

////////////////////////////////////////////////////
// SoM: Start new Boom functions
////////////////////////////////////////////////////

/** Finds next lowest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest floor height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindNextLowestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextLowestFloor(sector_t* sec, int currentheight)
{
	sector_t* other;
	int i;

	for(i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if(other && other->floorheight < currentheight)
		{
			int height = other->floorheight;
			while(++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if(other &&	other->floorheight > height
					&& other->floorheight < currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

/** Finds next lowest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest ceiling height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindNextLowestFloor
  * \author Lee Killough
  */
fixed_t P_FindNextLowestCeiling(sector_t* sec, int currentheight)
{
	sector_t* other;
	int i;

	for(i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if(other &&	other->ceilingheight < currentheight)
		{
			int height = other->ceilingheight;
			while(++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if(other &&	other->ceilingheight > height
					&& other->ceilingheight < currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}

/** Finds next highest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest ceiling height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindNextHighestFloor
  * \author Lee Killough
  */
fixed_t P_FindNextHighestCeiling(sector_t* sec, int currentheight)
{
	sector_t* other;
	int i;

	for(i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if(other && other->ceilingheight > currentheight)
		{
			int height = other->ceilingheight;
			while(++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if(other && other->ceilingheight < height
					&& other->ceilingheight > currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}

////////////////////////////
// End New Boom functions
////////////////////////////

/** Finds lowest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest ceiling height in an adjacent sector.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindLowestFloorSurrounding
  */
fixed_t P_FindLowestCeilingSurrounding(sector_t* sec)
{
	int i;
	line_t* check;
	sector_t* other;
	fixed_t height = MAXINT;
	int foundsector = 0;

	height = 32000*FRACUNIT; //SoM: 3/7/2000: Remove ovf

	for(i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if(!other)
			continue;

		if(other->ceilingheight < height || !foundsector)
			height = other->ceilingheight;

		if(!foundsector)
			foundsector = 1;
	}
	return height;
}

/** Finds Highest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest ceiling height in an adjacent sector.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindHighestFloorSurrounding
  */
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec)
{
	int i;
	line_t* check;
	sector_t* other;
	fixed_t height = 0;
	int foundsector = 0;

	for(i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if(!other)
			continue;

		if(other->ceilingheight > height || !foundsector)
			height = other->ceilingheight;

		if(!foundsector)
			foundsector = 1;
	}
	return height;
}

//SoM: 3/7/2000: UTILS.....
//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
//
fixed_t P_FindShortestTextureAround(int secnum)
{
	int minsize = MAXINT;
	side_t* side;
	int i;
	sector_t* sec;

	sec = &sectors[secnum];

	minsize = 32000<<FRACBITS;

	for(i = 0; i < sec->linecount; i++)
	{
		if(twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if(side->bottomtexture > 0)
				if(textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
			side = getSide(secnum,i,1);
			if(side->bottomtexture > 0)
				if(textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000: Stuff.... (can you tell I'm getting tired? It's 12:30!)
//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
//
fixed_t P_FindShortestUpperAround(int secnum)
{
	int minsize = MAXINT;
	side_t* side;
	int i;
	sector_t* sec = &sectors[secnum];

	minsize = 32000<<FRACBITS;

	for(i = 0; i < sec->linecount; i++)
	{
		if(twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if(side->toptexture > 0)
				if(textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
			side = getSide(secnum,i,1);
			if(side->toptexture > 0)
				if(textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000
//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
sector_t* P_FindModelFloorSector(fixed_t floordestheight, int secnum)
{
	int i;
	sector_t* sec = NULL;
	int linecount;

	sec = &sectors[secnum];
	linecount = sec->linecount;
	for(i = 0; i < linecount; i++)
	{
		if(twoSided(secnum, i))
		{
			if(getSide(secnum,i,0)->sector-sectors == secnum)
				sec = getSector(secnum,i,1);
			else
				sec = getSector(secnum,i,0);

			if(sec->floorheight == floordestheight)
				return sec;
		}
	}
	return NULL;
}

//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
sector_t* P_FindModelCeilingSector(fixed_t ceildestheight, int secnum)
{
	int i;
	sector_t* sec = NULL;
	int linecount;

	sec = &sectors[secnum];
	linecount = sec->linecount;
	for(i = 0; i < linecount; i++)
	{
		if(twoSided(secnum, i))
		{
			if(getSide(secnum, i, 0)->sector - sectors == secnum)
				sec = getSector(secnum, i, 1);
			else
				sec = getSector(secnum, i, 0);

			if(sec->ceilingheight == ceildestheight)
				return sec;
		}
	}
	return NULL;
}

/** Searches the tag lists for the next sector tagged to a line.
  *
  * \param line  Tagged line used as a reference.
  * \param start -1 to start at the beginning, or the result of a previous call
  *              to keep searching.
  * \return Number of the next tagged sector found.
  * \sa P_FindSectorFromTag, P_FindLineFromLineTag
  */
int P_FindSectorFromLineTag(line_t* line, int start)
{
	start = start >= 0 ? sectors[start].nexttag :
		sectors[(unsigned)line->tag % (unsigned)numsectors].firsttag;
	while(start >= 0 && sectors[start].tag != line->tag)
		start = sectors[start].nexttag;
	return start;
}

/** Searches the tag lists for the next sector with a given tag.
  *
  * \param tag   Tag number to look for.
  * \param start -1 to start anew, or the result of a previous call to keep
  *              searching.
  * \return Number of the next tagged sector found.
  * \sa P_FindSectorFromLineTag
  */
int P_FindSectorFromTag(int tag, int start)
{
	start = start >= 0 ? sectors[start].nexttag :
		sectors[(unsigned)tag % (unsigned)numsectors].firsttag;
	while(start >= 0 && sectors[start].tag != tag)
		start = sectors[start].nexttag;
	return start;
}

/** Searches the tag lists for the next line tagged to a line.
  *
  * \param line  Tagged line used as a reference.
  * \param start -1 to start anew, or the result of a previous call to keep
  *              searching.
  * \return Number of the next tagged line found.
  * \sa P_FindSectorFromLineTag
  */
int P_FindLineFromLineTag(const line_t* line, int start)
{
	start = start >= 0 ? lines[start].nexttag :
		lines[(unsigned)line->tag % (unsigned)numlines].firsttag;
	while(start >= 0 && lines[start].tag != line->tag)
		start = lines[start].nexttag;
	return start;
}

/** Searches the tag lists for the next line with a given tag and special.
  *
  * \param tag     Tag number.
  * \param start   -1 to start anew, or the result of a previous call to keep
  *                searching.
  * \return Number of next suitable line found.
  * \sa P_FindLineFromLineTag
  * \author Graue <graue@oceanbase.org>
  */
int P_FindLineFromTag(int tag, int start)
{
	start = start >= 0 ? lines[start].nexttag :
		lines[(unsigned)tag % (unsigned)numlines].firsttag;
	while(start >= 0 && lines[start].tag != tag)
		start = lines[start].nexttag;
	return start;
}

//
// P_FindSpecialLineFromTag
//
int P_FindSpecialLineFromTag(short special, short tag, int start)
{
	start = start >= 0 ? lines[start].nexttag :
		lines[(unsigned)tag % (unsigned)numlines].firsttag;
	while(start >= 0 && (lines[start].tag != tag || lines[start].special != special))
		start = lines[start].nexttag;
	return start;
}

/** Changes a sector's tag.
  * Used by the linedef executor tag changer and by crumblers.
  *
  * \param sector Sector whose tag will be changed.
  * \param newtag New tag number for this sector.
  * \sa P_InitTagLists, P_FindSectorFromTag
  * \author Graue <graue@oceanbase.org>
  */
void P_ChangeSectorTag(int sector, int newtag)
{
	int oldtag = sectors[sector].tag;

	if(oldtag == newtag)
		return;

	// first you have to remove it from the old tag's taglist
	{
		int i;

		i = sectors[(unsigned)oldtag % (unsigned)numsectors].firsttag;

		if(i == -1) // shouldn't happen
			I_Error("Corrupt tag list for sector %d\n", sector);
		else if(i == sector)
			sectors[(unsigned)oldtag % (unsigned)numsectors].firsttag = sectors[sector].nexttag;
		else
		{
			while(sectors[i].nexttag < sector && sectors[i].nexttag != -1)
				i = sectors[i].nexttag;

			sectors[i].nexttag = sectors[sector].nexttag;
		}
	}

	sectors[sector].tag = (short)newtag;

	// now add it to the new tag's taglist
	if(sectors[(unsigned)newtag % (unsigned)numsectors].firsttag > sector)
	{
		sectors[sector].nexttag = sectors[(unsigned)newtag % (unsigned)numsectors].firsttag;
		sectors[(unsigned)newtag % (unsigned)numsectors].firsttag = sector;
	}
	else
	{
		int i;
		i = sectors[(unsigned)newtag % (unsigned)numsectors].firsttag;

		if(i == -1)
		{
			sectors[(unsigned)newtag % (unsigned)numsectors].firsttag = sector;
			sectors[sector].nexttag = -1;
		}
		else
		{
			while(sectors[i].nexttag < sector && sectors[i].nexttag != -1)
				i = sectors[i].nexttag;

			sectors[sector].nexttag = sectors[i].nexttag;
			sectors[i].nexttag = sector;
		}
	}
}

/** Hashes the sector tags across the sectors and linedefs.
  *
  * \sa P_FindSectorFromTag, P_ChangeSectorTag
  * \author Lee Killough
  */
static void P_InitTagLists(void)
{
	register int i;

	for(i = numsectors - 1; i >= 0; i--)
		sectors[i].firsttag = -1;
	for(i = numsectors - 1; i >= 0; i--)
	{
		int j = (unsigned)sectors[i].tag % (unsigned)numsectors;
		sectors[i].nexttag = sectors[j].firsttag;
		sectors[j].firsttag = i;
	}

	for(i = numlines - 1; i >= 0; i--)
		lines[i].firsttag = -1;
	for(i = numlines - 1; i >= 0; i--)
	{
		int j = (unsigned)lines[i].tag % (unsigned)numlines;
		lines[i].nexttag = lines[j].firsttag;
		lines[j].firsttag = i;
	}
}

/** Finds minimum light from an adjacent sector.
  *
  * \param sector Sector to start in.
  * \param max    Maximum value to return.
  * \return Minimum light value from an adjacent sector, or max if the minimum
  *         light value is greater than max.
  */
int P_FindMinSurroundingLight(sector_t* sector, int max)
{
	int i, min;
	line_t* line;
	sector_t* check;

	min = max;
	for(i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if(!check)
			continue;

		if(check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

void T_ExecutorDelay(executor_t* e)
{
	if(--e->timer <= 0)
	{
		P_ProcessLineSpecial(e->line, e->caller);
		P_RemoveThinker(&e->thinker);
	}
}

static void P_AddExecutorDelay(line_t* line, mobj_t* mobj)
{
	executor_t* e;

	if(!line->backsector)
		I_Error("P_AddExecutorDelay: Line has no backsector!\n");

	e = Z_Malloc(sizeof *e, PU_LEVSPEC, 0);

	e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
	e->line = line;
	e->timer = (line->backsector->ceilingheight>>FRACBITS)+(line->backsector->floorheight>>FRACBITS);
	e->caller = mobj;
	P_AddThinker(&e->thinker);
}

static sector_t* triplinecaller;

/** Runs a linedef executor.
  * Can be called by:
  *   - a player moving into a special sector or FOF.
  *   - a pushable object moving into a special sector or FOF.
  *   - a ceiling or floor movement from a previous linedef executor finishing.
  *   - any object in a state with the A_LinedefExecute() action.
  *
  * \param tag Tag of the linedef executor to run.
  * \param actor Object initiating the action; should not be NULL.
  * \param caller Sector in which the action was started. May be NULL.
  * \sa P_ProcessLineSpecial
  * \author Graue <graue@oceanbase.org>
  */
void P_LinedefExecute(int tag, mobj_t* actor, sector_t* caller)
{
	sector_t* ctlsector;
	int i, masterline, linecnt;
	short specialtype;

	for(masterline = 0; masterline < numlines; masterline++)
	{
		if(lines[masterline].tag != tag)
			continue;

		if(!(lines[masterline].special == 96
			|| lines[masterline].special == 97
			|| lines[masterline].special == 98))
			continue;

		specialtype = lines[masterline].special;

		// Special type 97 only works once when you hit floor
		if(specialtype == 97 && actor && !(actor->eflags & (MF_JUSTHITFLOOR | MF_JUSTSTEPPEDDOWN)))
			return;

		triplinecaller = caller;
		ctlsector = lines[masterline].frontsector;
		linecnt = ctlsector->linecount;

		if(lines[masterline].flags & ML_ALLTRIGGER) // disregard order for efficiency
		{
			for(i = 0; i < linecnt; i++)
				if(ctlsector->lines[i]->special > 98)
				{
					if(ctlsector->lines[i]->flags & ML_DONTPEGTOP)
						P_AddExecutorDelay(ctlsector->lines[i], actor);
					else
						P_ProcessLineSpecial(ctlsector->lines[i], actor);
				}
		}
		else // walk around the sector in a defined order
		{
			boolean backwards = false;
			int j, masterlineindex = -1;

			for(i = 0; i < linecnt; i++)
				if(ctlsector->lines[i] == &lines[masterline])
				{
					masterlineindex = i;
					break;
				}

	#ifdef PARANOIA
			if(masterlineindex == -1)
				I_Error("Line %d isn't linked into its front sector", ctlsector->lines[i] - lines);
	#endif

			// i == masterlineindex
			for(;;)
			{
				if(backwards) // v2 to v1
				{
					for(j = 0; j < linecnt; j++)
					{
						if(i == j)
							continue;
						if(ctlsector->lines[i]->v1 == ctlsector->lines[j]->v2)
						{
							i = j;
							break;
						}
						if(ctlsector->lines[i]->v1 == ctlsector->lines[j]->v1)
						{
							i = j;
							backwards = false;
							break;
						}
					}
					if(j == linecnt)
					{
						CONS_Printf("Warning: Sector %s is not closed at vertex %d (%d, %d)\n",
							ctlsector - sectors, ctlsector->lines[i]->v1 - vertexes,
							ctlsector->lines[i]->v1->x, ctlsector->lines[i]->v1->y);
						return; // abort
					}
				}
				else // v1 to v2
				{
					for(j = 0; j < linecnt; j++)
					{
						if(i == j)
							continue;
						if(ctlsector->lines[i]->v2 == ctlsector->lines[j]->v1)
						{
							i = j;
							break;
						}
						if(ctlsector->lines[i]->v2 == ctlsector->lines[j]->v2)
						{
							i = j;
							backwards = true;
							break;
						}
					}
					if(j == linecnt)
					{
						CONS_Printf("Warning: Sector %s is not closed at vertex %d (%d, %d)\n",
							ctlsector - sectors, ctlsector->lines[i]->v2 - vertexes,
							ctlsector->lines[i]->v2->x, ctlsector->lines[i]->v2->y);
						return; // abort
					}
				}

				if(i == masterlineindex)
					break;

				if(ctlsector->lines[i]->special > 98)
				{
					if(ctlsector->lines[i]->flags & ML_DONTPEGTOP)
						P_AddExecutorDelay(ctlsector->lines[i], actor);
					else
						P_ProcessLineSpecial(ctlsector->lines[i], actor);
				}
			}
		}

		// Special type 98 only works once
		if(specialtype == 98)
		{
			lines[masterline].special = 0; // Clear it out
			if(caller && caller->special >= 971 && caller->special <= 975)
				caller->special = 0; // Clear that out, too, so this function doesn't get run forever
		}
	}
}

//
// P_SwitchWeather
//
// Switches the weather!
//
void P_SwitchWeather(int weathernum)
{
	boolean purge = false;

	switch(weathernum)
	{
		case 0: // None
			if(cv_snow.value == 0
				&& cv_rain.value == 0
				&& cv_storm.value == 0)
				return; // Nothing to do.
			purge = true;
			break;
		case 1: // Storm
		case 3: // Rain
			if(cv_snow.value)
				purge = true;
			break;
		case 2: // Snow
			if(cv_snow.value == 1)
				return; // Nothing to do.
			if(cv_rain.value || cv_storm.value)
				purge = true; // Need to delete the other precips.
			break;
		default:
			CONS_Printf("Unknown weather type %i.\n", weathernum);
			break;
	}

	if(purge)
	{
		thinker_t* think;
		precipmobj_t* precipmobj;

		for(think = thinkercap.next; think != &thinkercap; think = think->next)
		{
			if((think->function.acp1 != (actionf_p1)P_SnowThinker)
				&& (think->function.acp1 != (actionf_p1)P_RainThinker))
				continue; // not a precipmobj thinker

			precipmobj = (precipmobj_t*)think;

			P_RemovePrecipMobj(precipmobj);
		}
	}

	if(weathernum == 2) // snow
	{
		cv_snow.value = 1;
		CV_SetValue(&cv_snow, true);
		cv_storm.value = 0;
		CV_SetValue(&cv_storm, false);
		cv_rain.value = 0;
		CV_SetValue(&cv_rain, false);
		P_SpawnPrecipitation();
	}
	else if(weathernum == 3) // rain
	{
		boolean dontspawn = false;

		if(cv_rain.value || cv_storm.value)
			dontspawn = true;

		cv_rain.value = 1;
		CV_SetValue(&cv_rain, true);
		cv_storm.value = 0;
		CV_SetValue(&cv_storm, false);
		cv_snow.value = 0;
		CV_SetValue(&cv_snow, false);
		
		if(!dontspawn)
			P_SpawnPrecipitation();
	}
	else if(weathernum == 1) // storm
	{
		boolean dontspawn = false;

		if(cv_rain.value || cv_storm.value)
			dontspawn = true;

		cv_storm.value = 1;
		CV_SetValue(&cv_storm, true);
		cv_snow.value = 0;
		CV_SetValue(&cv_snow, false);
		cv_rain.value = 0;
		CV_SetValue(&cv_rain, false);

		if(!dontspawn)
			P_SpawnPrecipitation();
	}
	else
	{
		cv_storm.value = 0;
		cv_snow.value = 0;
		cv_rain.value = 0;
		CV_SetValue(&cv_snow, false);
		CV_SetValue(&cv_storm, false);
		CV_SetValue(&cv_rain, false);
	}
}

/** Processes the line special triggered by an object.
  * The external variable ::triplinecaller points to the sector in which the
  * action was initiated; it can be NULL. Because of the A_LinedefExecute()
  * action, even if non-NULL, this sector might not have the same tag as the
  * linedef executor, and it might not have the linedef executor sector type.
  *
  * \param line Line with the special command on it.
  * \param mo   mobj that triggered the line. Must be valid and non-NULL.
  * \todo Get rid of the secret parameter and make ::triplinecaller actually get
  *       passed to the function.
  * \todo Handle mo being NULL gracefully. T_MoveFloor() and T_MoveCeiling()
  *       don't have an object to pass.
  * \todo Split up into multiple functions.
  * \sa P_LinedefExecute
  * \author Graue <graue@oceanbase.org>
  */
void P_ProcessLineSpecial(line_t* line, mobj_t* mo)
{
	int secnum = -1;

	// note: no commands with linedef types <= 98 can be used
	switch(line->special)
	{
		case 101: // Set tagged sector's floor height/pic
			EV_DoFloor(line, instantMoveFloorByFrontSector);
			break;

		case 102: // Set tagged sector's ceiling height/pic
			EV_DoCeiling(line, instantMoveCeilingByFrontSector);
			break;

		case 103: // Set tagged sector's light level
			{
				short newlightlevel;
				int newfloorlightsec, newceilinglightsec;

				newlightlevel = line->frontsector->lightlevel;
				newfloorlightsec = line->frontsector->floorlightsec;
				newceilinglightsec = line->frontsector->ceilinglightsec;

				// act on all sectors with the same tag as the triggering linedef
				while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
				{
					if(sectors[secnum].lightingdata)
					{
						// Stop the lighting madness going on in this sector!
						P_RemoveThinker(&((elevator_t*)sectors[secnum].lightingdata)->thinker);
						sectors[secnum].lightingdata = NULL;

						// No, it's not an elevator_t, but any struct with a thinker_t named
						// 'thinker' at the beginning will do here. (We don't know what it
						// actually is: could be lightlevel_t, fireflicker_t, glow_t, etc.)
					}

					sectors[secnum].lightlevel = newlightlevel;
					sectors[secnum].floorlightsec = newfloorlightsec;
					sectors[secnum].ceilinglightsec = newceilinglightsec;
				}
			}
			break;

		case 104: // Teleport the player or thing
			{
				mobj_t* dest;

				if(!mo) // nothing to teleport
					return;

				if((secnum = P_FindSectorFromLineTag(line, -1)) < 0)
					return;

				dest = P_GetTeleportDestThing(secnum);
				if(!dest)
					return;

				if(line->flags & ML_BLOCKMONSTERS)
					P_Teleport(mo, dest->x, dest->y, dest->z, (line->flags & ML_NOCLIMB) ?  mo->angle : dest->angle, false, (line->flags & ML_PASSUSE));
				else
				{
					P_Teleport(mo, dest->x, dest->y, dest->z, (line->flags & ML_NOCLIMB) ?  mo->angle : dest->angle, true, (line->flags & ML_PASSUSE));
					// Play the 'bowrwoosh!' sound
					S_StartSound(dest, sfx_mixup);
				}
			}
			break;

		case 105: // Change music
			if(mo && ((cv_splitscreen.value && mo->player == &players[secondarydisplayplayer]) || mo->player == &players[consoleplayer])) // console player only
			{
				fixed_t musicnum;

				musicnum = P_AproxDistance(line->dx, line->dy)>>FRACBITS;

				if(line->flags & ML_BLOCKMONSTERS)
					musicnum += 2048;

				if((musicnum & ~2048) < NUMMUSIC && (musicnum & ~2048) > mus_None)
					S_ChangeMusic(musicnum & 2047, !(line->flags & ML_NOCLIMB));
				else
					S_StopMusic();

				mapmusic = (short)musicnum; // but it gets reset if you die

				// Except, you can use the ML_BLOCKMONSTERS flag to change this behavior.
				// If(mapmusic & 2048) then it won't reset the music in G_PlayerReborn as usual.
				// This is why I do the crazy anding with musicnum above.
			}
			break;

		case 106: // Move floor, linelen = speed, frontsector floor = dest height
			EV_DoFloor(line, moveFloorByFrontSector);
			break;

		case 107: // Move ceiling, linelen = speed, frontsector ceiling = dest height
			EV_DoCeiling(line, moveCeilingByFrontSector);
			break;

		case 108: // Lower floor by line, dx = speed, dy = amount to lower
			EV_DoFloor(line, lowerFloorByLine);
			break;

		case 109: // Raise floor by line, dx = speed, dy = amount to raise
			EV_DoFloor(line, raiseFloorByLine);
			break;

		case 110: // Lower ceiling by line, dx = speed, dy = amount to lower
			EV_DoCeiling(line, lowerCeilingByLine);
			break;

		case 111: // Raise ceiling by line, dx = speed, dy = amount to raise
			EV_DoCeiling(line, raiseCeilingByLine);
			break;

		case 112: // Change calling sector's tag
		//	if(triplinecaller)
				P_ChangeSectorTag(P_FindSectorFromLineTag(line, -1)/*triplinecaller - sectors*/, P_AproxDistance(line->dx, line->dy)>>FRACBITS);
			break;

		case 113: // Run a script
			if(cv_runscripts.value)
			{
				int lumpnum, scrnum;
				char newname[9];

				strcpy(newname, G_BuildMapName(gamemap));
				newname[0] = 'S';
				newname[1] = 'C';
				newname[2] = 'R';

				scrnum = line->frontsector->floorheight >> FRACBITS;
				if(scrnum > 999)
				{
					scrnum = 0;
					newname[5] = newname[6] = newname[7] = '0';
				}
				else
				{
					newname[5] = (char)('0' + (char)((scrnum/100)));
					newname[6] = (char)('0' + (char)((scrnum%100)/10));
					newname[7] = (char)('0' + (char)(scrnum%10));
				}
				newname[8] = '\0';

				lumpnum = W_CheckNumForName(newname);

				if(lumpnum == -1 || W_LumpLength(lumpnum) <= 0)
					CONS_Printf("SOC Error: script lump %s not found/not valid.\n", newname);
				else
					COM_BufInsertText(W_CacheLumpNum(lumpnum, PU_CACHE));
			}
			break;

		case 114: // Change front sector's tag
			P_ChangeSectorTag((int)(line->frontsector - sectors), P_AproxDistance(line->dx, line->dy)>>FRACBITS);
			break;

		case 115: // Play SFX
			{
				fixed_t sfxnum;

				sfxnum = P_AproxDistance(line->dx, line->dy) >> FRACBITS;

				if(sfxnum < NUMSFX && sfxnum > sfx_None)
				{
					if(line->flags & ML_NOCLIMB)
					{
						// play the sound from nowhere, but only if display player triggered it
						if(mo && mo->player && (mo->player == &players[displayplayer] || mo->player == &players[secondarydisplayplayer]))
							S_StartSound(NULL, sfxnum);
					}
					else if(line->flags & ML_PASSUSE)
					{
						// play the sound from nowhere
						S_StartSound(NULL, sfxnum);
					}
					else if(line->flags & ML_BLOCKMONSTERS)
					{
						// play the sound from calling sector's soundorg
						if(triplinecaller)
							S_StartSound(&triplinecaller->soundorg, sfxnum);
						else if(mo)
							S_StartSound(&mo->subsector->sector->soundorg, sfxnum);
					}
					else if(mo)
					{
						// play the sound from mobj that triggered it
						S_StartSound(mo, sfxnum);
					}
				}
			}
			break;

		case 116: // Stop floor/ceiling movement in tagged sector(s)
			while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if(sectors[secnum].floordata)
				{
					if(sectors[secnum].floordata == sectors[secnum].ceilingdata) // elevator
					{
						P_RemoveThinker(&((elevator_t*)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = sectors[secnum].ceilingdata = NULL;
						sectors[secnum].floorspeed = sectors[secnum].ceilspeed = 0;
					}
					else // floormove
					{
						P_RemoveThinker(&((floormove_t*)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = NULL;
						sectors[secnum].floorspeed = 0;
					}
				}

				if(sectors[secnum].ceilingdata) // ceiling
				{
					P_RemoveThinker(&((ceiling_t*)sectors[secnum].ceilingdata)->thinker);
					sectors[secnum].ceilingdata = NULL;
					sectors[secnum].ceilspeed = 0;
				}
			}
			break;

		case 117: // Fade light levels in tagged sectors to new value
			P_FadeLight(line->tag, sectors[secnum].lightlevel, P_AproxDistance(line->dx, line->dy));
			break;

		case 118: // Stop lighting effect in tagged sectors
			while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
				if(sectors[secnum].lightingdata)
				{
					P_RemoveThinker(&((elevator_t*)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}
			break;

		case 119: // Spawn adjustable fire flicker
			while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if(line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableFireFlicker expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					fireflicker_t* flick;
					short reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					flick = P_SpawnAdjustableFireFlicker(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);

					// Make sure the starting light level is in range.
					if(reallightlevel < flick->minlight)
						reallightlevel = (short)flick->minlight;
					else if(reallightlevel > flick->maxlight)
						reallightlevel = (short)flick->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 61 does it.
					P_SpawnAdjustableFireFlicker(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);
				}
			}
			break;

		case 120: // Spawn adjustable glowing light
			while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if(line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableGlowingLight expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					glow_t* glow;
					short reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					glow = P_SpawnAdjustableGlowingLight(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);

					// Make sure the starting light level is in range.
					if(reallightlevel < glow->minlight)
						reallightlevel = (short)glow->minlight;
					else if(reallightlevel > glow->maxlight)
						reallightlevel = (short)glow->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 60 does it.
					P_SpawnAdjustableGlowingLight(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);
				}
			}
			break;

		case 121: // Cut away to another view
			{
				mobj_t* altview;

				if(!mo || !mo->player) // only players have views
					return;

				if((secnum = P_FindSectorFromLineTag(line, -1)) < 0)
					return;

				altview = P_GetAltViewThing(secnum);
				if(!altview)
					return;

				mo->player->awayviewmobj = altview;
				mo->player->awayviewtics = P_AproxDistance(line->dx, line->dy)>>FRACBITS;

				if(line->flags & ML_NOCLIMB) // lets you specify a vertical angle
				{
					int aim;

					aim = sides[line->sidenum[0]].textureoffset>>FRACBITS;
					while(aim < 0)
						aim += 360;
					while(aim >= 360)
						aim -= 360;
					aim *= (ANG90>>8);
					aim /= 90;
					aim <<= 8;
					mo->player->awayviewaiming = (angle_t)aim;
				}
				else
					mo->player->awayviewaiming = 0; // straight ahead
			}
			break;

		case 122: // Moves the mobj to its sector's soundorg and on the floor, and stops it
			if(!mo)
				return;

			mo->x = mo->subsector->sector->soundorg.x;
			mo->y = mo->subsector->sector->soundorg.y;
			mo->z = mo->floorz;

			mo->momx = mo->momy = mo->momz = 1;

			if(mo->player)
			{
				if(cv_splitscreen.value && cv_chasecam2.value && mo->player == &players[secondarydisplayplayer])
					P_ResetCamera(mo->player, &camera2);
				else if(cv_chasecam.value && mo->player == &players[displayplayer])
					P_ResetCamera(mo->player, &camera);

				mo->player->rmomx = mo->player->rmomy = 1;
				mo->player->cmomx = mo->player->cmomy = 0;
				P_ResetPlayer(mo->player);
				P_SetPlayerMobjState(mo, S_PLAY_STND);
			}
			break;

		case 123: // Change Sky
			if((mo && (mo->player == &players[consoleplayer]
				|| (cv_splitscreen.value && mo->player == &players[secondarydisplayplayer]))) || (line->flags & ML_NOCLIMB))
			{
				if(line->flags & ML_NOCLIMB)
					globallevelskynum = line->frontsector->floorheight>>FRACBITS;

				levelskynum = line->frontsector->floorheight>>FRACBITS;
				P_SetupLevelSky(levelskynum);
			}
			break;

		case 124: // Change Weather - Extremely CPU-Intense.
			if((mo && (mo->player == &players[consoleplayer]
				|| (cv_splitscreen.value && mo->player == &players[secondarydisplayplayer]))) || (line->flags & ML_NOCLIMB))
			{
				if(line->flags & ML_NOCLIMB)
					globalweather = (line->frontsector->floorheight>>FRACBITS)/10;
				
				P_SwitchWeather((line->frontsector->floorheight>>FRACBITS)/10);
			}
			break;

		case 125: // Calls P_SetMobjState on calling mobj
			if(mo)
				P_SetMobjState(mo, P_AproxDistance(line->dx, line->dy) >> FRACBITS);
			break;

		default:
			break;
	}
}

//
// P_SetupSignExit
//
// Finds the exit sign in the current sector and
// sets its target to the player who passed the map.
//
void P_SetupSignExit(player_t* player)
{
    mobj_t* thing;
    msecnode_t* node;

    node = player->mo->subsector->sector->touching_thinglist; // things touching this sector
    for ( ; node ; node = node->m_snext)
    {
		thing = node->m_thing;
		if(thing->type != MT_SIGN)
			continue;

		if(thing->state != &states[thing->info->spawnstate])
			continue;

		thing->target = player->mo;
		P_SetMobjState(thing, S_SIGN1);
		if(thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);
	}
}

//
// P_IsFlagAtBase
//
// Checks to see if a flag is at its base.
//
static boolean P_IsFlagAtBase(mobjtype_t flag)
{
	thinker_t* think;
	mobj_t* mo;
	int specialnum = 0;

	for(think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if(think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t*)think;

		if(mo->type != flag)
			continue;

		if(mo->type == MT_REDFLAG)
			specialnum = 988;
		else if(mo->type == MT_BLUEFLAG)
			specialnum = 989;

		if(mo->subsector->sector->special == specialnum)
			return true;
		else if(mo->subsector->sector->ffloors) // Check the 3D floors
		{
			ffloor_t* rover;

			for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_EXISTS))
					continue;

				if(rover->master->frontsector->special != specialnum)
					continue;

				if(mo->z <= *rover->topheight
					&& mo->z >= *rover->bottomheight)
					return true;
			}
		}
	}
	return false;
}

/** Applies a sector special to a player.
  *
  * \param player       Player in the sector.
  * \param sector       Sector with the special.
  * \param roverspecial If true, sector is actually an FOF; otherwise, sector
  *                     is being physically contacted by the player.
  * \todo Split up into multiple functions.
  * \sa P_PlayerInSpecialSector, P_PlayerOnSpecial3DFloor
  */
static void P_ProcessSpecialSector(player_t* player, sector_t* sector, boolean roverspecial)
{
	mobj_t*	mo;
	line_t junk;
	int i = 0;

	// Conveyor stuff
	if(sector->special == 512 || sector->special == 984 || sector->special == 985
		|| sector->special == 519)
	{
		player->onconveyor = sector->special;
	}

	switch(sector->special)
	{
		case 6: // Space countdown
			if(!player->powers[pw_watershield] && !player->powers[pw_spacetime])
			{
				player->powers[pw_spacetime] = spacetimetics + 1;

				if((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer])
					S_ChangeMusic(mus_drown, false);
			}
			return;
		case 10: // Instant kill
			P_DamageMobj(player->mo, NULL, NULL, 10000);
			return;
		case 18: // Electrical damage
			if(!player->powers[pw_ringshield])
				P_DamageMobj(player->mo, NULL, NULL, 1);
			return;
		case 33: // Special stage GOAL sector
			if(sstimer > 6 && gamemap >= sstage_start && gamemap <= sstage_end)
				sstimer = 6; // Just let P_Ticker take care of the rest.
			return;
		case 666: // Egg trap capsule
		{
			thinker_t* th;
			mobj_t* mo2;

			if(sector->ceilingdata || sector->floordata)
				return;
			junk.tag = 680;
			EV_DoElevator(&junk, elevateDown, false);
			junk.tag = 681;
			EV_DoFloor(&junk, raiseFloorToNearestFast);
			junk.tag = 682;
			EV_DoCeiling(&junk, lowerToLowestFast);
			sector->special = 0;

			// Find the center of the Eggtrap and release all the pretty animals!
			// The chimps are my friends.. heeheeheheehehee..... - LouisJM
			for(th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if(th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t*)th;
				if(mo2->type == MT_EGGTRAP)
				{
					mo2->fuse = TICRATE/7;
					break;
				}
			}
			return;
		}
		case 969: // Super Sonic transformer
			if(player->charability == 0 && !player->powers[pw_super] && ALL7EMERALDS)
				P_DoSuperTransformation(player, true);
			return;
		case 970: // Tells pushable things to check FOFs
		case 971: // Linedef executor for pushable things
			return;
		case 972: // Linedef executor requires all players present+doesn't require touching floor
		case 973: // Linedef executor requires all players present
			/// \todo take FOFs into account (+continues for proper splitscreen support?)
			for(i = 0; i < MAXPLAYERS; i++)
				if(playeringame[i] && players[i].mo && (gametype != GT_COOP || players[i].lives > 0) && players[i].mo->subsector->sector != sector)
					return;
		case 974: // Linedef executor that doesn't require touching floor
		case 975: // Linedef executor
			P_LinedefExecute(sector->tag, player->mo, sector);
			return;
		case 976: // Speed pad w/o spin
		case 977: // Speed pad w/ spin
			if(player->powers[pw_flashing] != 0 && player->powers[pw_flashing] < TICRATE/2)
				return;

			i = P_FindSpecialLineFromTag(65, sector->tag, -1);

			if(i != -1)
			{
				angle_t lineangle;
				fixed_t linespeed;

				lineangle = R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y);
				linespeed = P_AproxDistance(lines[i].v2->x-lines[i].v1->x, lines[i].v2->y-lines[i].v1->y);

				if(linespeed > MAXMOVE)
					linespeed = MAXMOVE;

				player->mo->angle = lineangle;

				if(player == &players[consoleplayer])
					localangle = player->mo->angle;
				else if(player == &players[secondarydisplayplayer])
					localangle2 = player->mo->angle;

				if(!(lines[i].flags & ML_PASSUSE))
				{
					P_UnsetThingPosition(player->mo);
					if(roverspecial) // make FOF speed pads work
					{
						player->mo->x = player->mo->subsector->sector->soundorg.x;
						player->mo->y = player->mo->subsector->sector->soundorg.y;
					}
					else
					{
						player->mo->x = sector->soundorg.x;
						player->mo->y = sector->soundorg.y;
					}
					P_SetThingPosition(player->mo);
				}

				P_InstaThrust(player->mo, player->mo->angle, linespeed);

				if(sector->special == 977 && player->charspin)
				{
					if(!player->mfspinning)
					{
						P_ResetScore(player);
						player->mfspinning = 1;
					}

					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				}

				player->powers[pw_flashing] = TICRATE/3;
				S_StartSound(player->mo, sfx_spdpad);
			}
			return;
		case 978: // Ring drainer: lose one ring per 15 tics
		case 980: // Another ring drainer, not requiring floor touch
			if(leveltime % 15 == 0 && player->mo->health > 1)
			{
				player->mo->health--;
				player->health--;
				S_StartSound(player->mo, sfx_itemup);
			}
			return;
		case 979: // Make player spin
			/// Question: Do we really need to check z with floorz here?
			// Answer: YES.
			if(!player->mfspinning && player->mo->z == player->mo->floorz && player->charspin)
			{
				P_ResetScore(player);
				player->mfspinning = 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				S_StartAttackSound(player->mo, sfx_spin);

				if((player->rmomx < 5*FRACUNIT/NEWTICRATERATIO
				&& player->rmomx > -5*FRACUNIT/NEWTICRATERATIO)
				&& (player->rmomy < 5*FRACUNIT/NEWTICRATERATIO
				&& player->rmomy > -5*FRACUNIT/NEWTICRATERATIO))
					P_InstaThrust(player->mo, player->mo->angle, 10*FRACUNIT);
			}
			return;
		case 981: // Water rise stuff
			junk.tag = 744;
			EV_DoCeiling(&junk, raiseToHighest);
			return;
		case 982: // Exit (for FOF exits; others are handled in P_PlayerThink in p_user.c)
			if(roverspecial)
			{
				// this is mostly a copy/paste from P_PlayerThink handling of exits
				/// \todo only have this code in one place
				int lineindex;

				P_DoPlayerExit(player);

				P_SetupSignExit(player);
				// important: use sector->tag on next line instead of player->mo->subsector->tag
				// this part is different from in P_PlayerThink, this is what was causing
				// FOF custom exits not to work.
				lineindex = P_FindSpecialLineFromTag(71, sector->tag, -1);

				if(gametype != GT_RACE && lineindex != -1) // Custom exit!
				{
					// Special goodies with the block monsters flag depending on emeralds collected
					if((lines[lineindex].flags & ML_BLOCKMONSTERS) && ALL7EMERALDS)
					{
							if(emeralds & EMERALD8) // The secret eighth emerald. Use linedef length.
								nextmapoverride = P_AproxDistance(lines[lineindex].dx, lines[lineindex].dy);
							else // The first seven, not bad. Use front sector's ceiling.
								nextmapoverride = lines[lineindex].frontsector->ceilingheight;
					}
					else
						nextmapoverride = lines[lineindex].frontsector->floorheight;

					nextmapoverride >>= FRACBITS;

					if(lines[lineindex].flags & ML_NOCLIMB)
						skipstats = true;

					// change the gametype using front x offset if passuse flag is given
					// ...but not in single player!
					if(multiplayer && lines[lineindex].flags & ML_PASSUSE)
					{
						int xofs = sides[lines[lineindex].sidenum[0]].textureoffset;
						if(xofs > 0 && xofs < NUMGAMETYPES)
							nextmapgametype = xofs;
					}
				}
			}
			return;
		case 983:
		case 984: // Slime!
			if(player->powers[pw_underwater] && !player->powers[pw_watershield])
				P_DamageMobj(player->mo, NULL, NULL, 1);
			return;
		case 985: // Conveyor belt
		case 987: // No Tag Zone
			return;
		case 988: // Red Team's Base
			if(gametype == GT_CTF && player->mo->z == player->mo->floorz)
			{
				if(!player->ctfteam)
				{
					if(player == &players[consoleplayer])
					{
						COM_BufAddText("changeteam red");
						COM_BufExecute();
					}
					else if(cv_splitscreen.value && player == &players[secondarydisplayplayer])
					{
						COM_BufAddText("changeteam2 red");
						COM_BufExecute();
					}
					return;
				}

				if(player->ctfteam == 1)
				{
					if(player->gotflag & MF_BLUEFLAG)
					{
						// Make sure the red team still has their own
						// flag at their base so they can score.
						for(i=0; i<MAXPLAYERS; i++)
						{
							if(!playeringame[i])
								continue;

							if(players[i].gotflag & MF_REDFLAG)
								return;
						}

						if(!P_IsFlagAtBase(MT_REDFLAG))
							return;

						redscore += 1;

						if(cv_pointlimit.value)
							P_CheckPointLimit(player);

						mo = P_SpawnMobj(player->mo->x,
										player->mo->y,
										player->mo->z,
										MT_BLUEFLAG);
						player->gotflag &= ~MF_BLUEFLAG;
						mo->flags &= ~MF_SPECIAL;
						mo->fuse = TICRATE;
						mo->spawnpoint = bflagpoint;
						CONS_Printf("%s captured the blue flag.\n", player_names[player-players]);
					}
					if(player->gotflag & MF_REDFLAG) // Returning a flag to your base
					{
						mo = P_SpawnMobj(player->mo->x,
										player->mo->y,
										player->mo->z,
										MT_REDFLAG);
						mo->flags &= ~MF_SPECIAL;
						player->gotflag &= ~MF_REDFLAG;
						mo->fuse = TICRATE;
						mo->spawnpoint = rflagpoint;
					}
				}
				return;
			}
			return;
		case 989: // Blue Team's Base
			if(gametype == GT_CTF && player->mo->z == player->mo->floorz)
			{
				if(!player->ctfteam)
				{
					if(player == &players[consoleplayer])
					{
						COM_BufAddText("changeteam blue");
						COM_BufExecute();
					}
					else if(cv_splitscreen.value && player == &players[secondarydisplayplayer])
					{
						COM_BufAddText("changeteam2 blue");
						COM_BufExecute();
					}
					return;
				}

				if(player->ctfteam == 2)
				{
					if(player->gotflag & MF_REDFLAG)
					{
						for(i=0; i<MAXPLAYERS; i++)
						{
							if(!playeringame[i])
								continue;

							if(players[i].gotflag & MF_BLUEFLAG)
								return;
						}

						if(!P_IsFlagAtBase(MT_BLUEFLAG))
							return;

						bluescore += 1;

						if(cv_pointlimit.value)
							P_CheckPointLimit(player);

						mo = P_SpawnMobj(player->mo->x,
										player->mo->y,
										player->mo->z,
										MT_REDFLAG);
						player->gotflag &= ~MF_REDFLAG;
						mo->flags &= ~MF_SPECIAL;
						mo->fuse = TICRATE;
						mo->spawnpoint = rflagpoint;
						CONS_Printf("%s captured the red flag.\n", player_names[player-players]);
					}
					if(player->gotflag & MF_BLUEFLAG) // Returning a flag to your base
					{
						mo = P_SpawnMobj(player->mo->x,
										player->mo->y,
										player->mo->z,
										MT_BLUEFLAG);
						mo->flags &= ~MF_SPECIAL;
						player->gotflag &= ~MF_BLUEFLAG;
						mo->fuse = TICRATE;
						mo->spawnpoint = bflagpoint;
					}
				}
			}
			return;
		case 990: // Special Stage Time/Rings
		case 991: // Custom Gravity
		case 992: // Ramp Sector (obsolete)
			return;
		case 993: // Starpost Activator
			{
				mobj_t* post = P_GetStarpostThing(sector - sectors);

				if(!post)
					return;

				P_TouchSpecialThing(post, player->mo, false);
			}
			return;
		case 996: // Non-Ramp Sector
			return;
		case 997: // Fan sector
			player->mo->momz = mobjinfo[MT_FAN].speed;
			P_ResetPlayer(player);
			if(player->mo->state != &states[S_PLAY_FALL1]
				&& player->mo->state != &states[S_PLAY_FALL2])
			{
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
			}
			return;
	}

	// Lava & Current
	if(sector->special == 519)
	{
		if(!player->powers[pw_fireshield])
			P_DamageMobj(player->mo, NULL, NULL, 1);
		return;
	}

	if(sector->special < 32)
	{
		// Has hitten ground.
		switch(sector->special)
		{
			case 5: // instant death
				// HELLSLIME DAMAGE
				P_DamageMobj(player->mo, NULL, NULL, 10000);
				break;

			case 7: // lava and the like
				// NUKAGE DAMAGE
				if(!player->powers[pw_fireshield])
					P_DamageMobj(player->mo, NULL, NULL, 1);
				break;

			case 16: // instant death (again)
				// SUPER HELLSLIME DAMAGE
				P_DamageMobj(player->mo, NULL, NULL, 10000);
				break;

			case 4: // Spikes
				break;

			case 9:
				// Special Stage Hurt Sector
				mo = player->mo;
				if(player->powers[pw_fireshield] || player->powers[pw_ringshield] || player->powers[pw_jumpshield] || player->powers[pw_watershield] || player->powers[pw_bombshield])
				{
					player->powers[pw_fireshield] = false;
					player->powers[pw_jumpshield] = false;
					player->powers[pw_watershield] = false;
					player->powers[pw_ringshield] = false;

					if(player->powers[pw_bombshield]) // Give them what's coming to them!
					{
						player->blackow = 1; // BAM!
						player->powers[pw_bombshield] = false;
						player->jumpdown = true;
					}
					player->mo->z++;

					if(player->mo->eflags & MF_UNDERWATER)
						player->mo->momz = (10511*FRACUNIT)/2600;
					else
						player->mo->momz = (69*FRACUNIT)/10;

					P_InstaThrust(player->mo, player->mo->angle-ANG180, 4*FRACUNIT);

					P_SetPlayerMobjState(player->mo, player->mo->info->painstate);
					player->powers[pw_flashing] = flashingtics;

					player->powers[pw_fireflower] = false;
					player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
						| ((player->skincolor)<<MF_TRANSSHIFT);

					P_ResetPlayer(player);

					S_StartSound(player->mo, sfx_shldls); // Ba-Dum! Shield loss.

					if(gametype == GT_CTF && (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
						P_PlayerFlagBurst(player);
				}
				if(mo->health > 1)
				{
					if(player->powers[pw_flashing])
						return;
					player->powers[pw_flashing] = flashingtics;
					P_PlayRinglossSound(mo);
					if(mo->health > 10)
						mo->health -= 10;
					else
						mo->health = 1;
					player->health = mo->health;
					mo->z++;
					if(mo->eflags & MF_UNDERWATER)
						mo->momz = (10511*FRACUNIT)/2600;
					else
						mo->momz = (69*FRACUNIT)/10;
					P_InstaThrust (mo, mo->angle - ANG180, 4*FRACUNIT);
					P_ResetPlayer(player);
					P_SetPlayerMobjState(mo, S_PLAY_PAIN);
				}
				// P_DamageMobj (player->mo, NULL, NULL, 10);
				break;

			case 11: // Straight damage.
				P_DamageMobj(player->mo, NULL, NULL, 1);
				break;

			case 14: // Bouncy sector
				break;

			default:
				// Just ignore.
				break;
		};
	}
/*	else // Extended sector types for secrets and damage
	{
		switch((sector->special & DAMAGE_MASK) >> DAMAGE_SHIFT)
		{
			case 0: // no damage
				break;
			case 1: // 2/5 damage per 31 ticks
				P_DamageMobj(player->mo, NULL, NULL, 5);
				break;
			case 2: // 5/10 damage per 31 ticks
				P_DamageMobj(player->mo, NULL, NULL, 10);
				break;
			case 3: // 10/20 damage per 31 ticks
				if(P_Random() < 5) // take damage even with suit
				{
					P_DamageMobj(player->mo, NULL, NULL, 20);
				}
				break;
		}
	}*/
}

/** Checks if an object is standing on or is inside a special 3D floor.
  * If so, the sector is returned.
  *
  * \param mo Object to check.
  * \return Pointer to the sector with a special type, or NULL if no special 3D
  *         floors are being contacted.
  * \sa P_PlayerOnSpecial3DFloor
  */
sector_t* P_ThingOnSpecial3DFloor(mobj_t* mo)
{
	sector_t* sector;
	ffloor_t* rover;

	sector = mo->subsector->sector;
	if(!sector->ffloors)
		return NULL;

	for(rover = sector->ffloors; rover; rover = rover->next)
	{
		if(!rover->master->frontsector->special)
			continue;

		// Check the 3D floor's type...
		if(rover->flags & FF_SOLID)
		{
			// Thing must be on top of the floor to be affected...
			if(mo->z != *rover->topheight)
				continue;
		}
		else
		{
			// Water and intangible FOFs
			if(mo->z > *rover->topheight || (mo->z + mo->height) < *rover->bottomheight)
				continue;
		}

		return rover->master->frontsector;
	}

	return NULL;
}

/** Checks if a player is standing on or is inside a 3D floor (e.g. water) and
  * applies any specials.
  *
  * \param player Player to check.
  * \sa P_ThingOnSpecial3DFloor, P_PlayerInSpecialSector
  */
static inline void P_PlayerOnSpecial3DFloor(player_t* player)
{
	sector_t* sector;
	ffloor_t* rover;

	sector = player->mo->subsector->sector;
	if(!sector->ffloors)
		return;

	for(rover = sector->ffloors; rover; rover = rover->next)
	{
		if(!rover->master->frontsector->special)
			continue;

		// Check the 3D floor's type...
		if(rover->flags & FF_SOLID)
		{
			// Player must be on top of the floor to be affected...
			if(player->mo->z != *rover->topheight)
				continue;
		}
		else
		{
			//Water and DEATH FOG!!! heh
			if(player->mo->z > *rover->topheight || (player->mo->z + player->mo->height) < *rover->bottomheight)
				continue;
		}

		P_ProcessSpecialSector(player, rover->master->frontsector, true);
	}
}

#define VDOORSPEED (FRACUNIT*2/NEWTICRATERATIO)

/** Checks if the player is in a special sector or FOF and apply any specials.
  *
  * \param player Player to check.
  * \sa P_PlayerOnSpecial3DFloor, P_ProcessSpecialSector
  */
void P_PlayerInSpecialSector(player_t* player)
{
	sector_t* sector;
	line_t junk; // buttons and more
/*
	if(!player->mo->subsector->sector->special)
		player->onconveyor = 0;
*/
	// Check 3D floors...
	P_PlayerOnSpecial3DFloor(player);

	sector = player->mo->subsector->sector;

	if(sector->special == 666) // Egg trap capsule -- should only be for 3dFloors!
		return;

	// keep track of what sector type the player's currently in
	player->specialsector = sector->special;

	if(!player->specialsector) // nothing special, exit
		return;

	// Prettify the list of specials that activate without floor touch
	switch(player->specialsector)
	{
		case 6: // Space countdown
		case 10: // Instant kill
		case 969: // Super Sonic Transform
		case 972: // Linedef executor
		case 974: // Linedef executor
		case 980: // Ring drainer that doesn't require floor touch
		case 981: // Water rise stuff
		case 983: // Damage (water)
		case 984: // Damage (water) + current
		case 993: // Starpost activator
		case 997: // Fan sector
			P_ProcessSpecialSector(player, sector, false);
			return;
	}
	// 976 and 977 should NOT be in above list!
	// List updated 10-30-2004

	// Falling, not all the way down yet?
	// Damage if in slimey water!
	if(sector->heightsec != -1 && !(player->specialsector == 712 || player->specialsector == 713))
	{
		if(player->mo->z > sectors[sector->heightsec].floorheight)
			return;
	}
	if(player->mo->z != sector->floorheight) // Only go further if on the ground
		return;

	if(player->specialsector >= 690 && player->specialsector <= 709)
	{
		junk.tag = (short)(player->specialsector * 2 - 680);
		junk.dx = VDOORSPEED;
		EV_DoCeiling(&junk, raiseToLowest);
		junk.tag++;
		EV_DoFloor(&junk, lowerFloorToLowest);
		return;
	}

	switch(player->specialsector)
	{
		// start button 21 (special used for THZ2)
		case 710:
			junk.tag = 740;
			EV_DoFloor(&junk, instantLower);
			junk.tag = 741;
			EV_DoFloor(&junk, lowerFloorToLowest);
			junk.tag = 742;
			junk.dx = VDOORSPEED;
			EV_DoCeiling(&junk, raiseToLowest);
			junk.tag = 745;
			{
				int secnum;
				sector_t* sec;
				fixed_t temp;

				secnum = -1;
				// act on all sectors with the same tag as the triggering linedef
				while((secnum = P_FindSectorFromLineTag(&junk,secnum)) >= 0)
				{
					sec = &sectors[secnum];

					temp = P_FindNextLowestFloor(sec, sec->floorheight);
					sec->ceilingheight = temp + sec->ceilingheight - sec->floorheight;
					sec->floorheight = temp;
				}
			}
			return;
		// end button 21 (special used for THZ2)
		// start door closer
		case 711:
			junk.tag = 743;
			junk.dx = VDOORSPEED*4;
			EV_DoCeiling(&junk, lowerAndCrush);
			return;
		// end door closer

		// Start THZ2-specific Slime Lower button
		case 986:
			// Button
			junk.tag = 712;
			EV_DoFloor(&junk, lowerFloorToLowest);

			// Water
			junk.tag = 713;
			EV_DoCeiling(&junk, lowerToLowest);

			// Platforms floating on the water *not required?*
			junk.tag = 714;
			EV_DoElevator(&junk, elevateDown, false);

			// The door
			junk.tag = 715;
			junk.dx = VDOORSPEED/12; // Open slowly now...
			EV_DoCeiling(&junk, raiseToLowest);

			// I saw the sign!
			junk.tag = 716;
			EV_DoFloor(&junk, instantLower);
			return;
		// End THZ2-specific Slime Lower button
	}

	P_ProcessSpecialSector(player, sector, false);
}

/** Animate planes, scroll walls, etc. and keeps track of level timelimit and exits if time is up.
  *
  * \sa cv_timelimit, P_CheckPointLimit
  */
void P_UpdateSpecials(void)
{
	anim_t* anim;
	int i, pic;

	levelflat_t* foundflats; // for flat animation

	// LEVEL TIMER
	// Only exit if the time is exactly equal to the timelimit.
	// Why? Suppose after playing for three and a half minutes you decide you want a timelimit
	//   of 10 minutes. You go to the menu item for timelimit, press right, and the round is
	//   suddenly over, because more than one minute has passed. Oops.
	if(cv_timelimit.value && timelimitintics == leveltime)
		G_ExitLevel();

	// ANIMATE TEXTURES
	for(anim = anims; anim < lastanim; anim++)
	{
		for(i = anim->basepic; i < anim->basepic + anim->numpics; i++)
		{
			pic = anim->basepic + ((leveltime/anim->speed + i) % anim->numpics);
			if(anim->istexture)
				texturetranslation[i] = pic;
		}
	}

	// ANIMATE FLATS
	/// \todo do not check the non-animate flat.. link the animated ones?
	/// \note its faster than the original anywaysince it animates only
	///    flats used in the level, and there's usually very few of them
	foundflats = levelflats;
	for(i = 0; i < numlevelflats; i++, foundflats++)
	{
		if(foundflats->speed) // it is an animated flat
		{
			// update the levelflat lump number
			foundflats->lumpnum = foundflats->baselumpnum +
				((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
		}
	}
}

// Adds a sector's floor and ceiling to a sector's ffloor list
static inline void P_AddFFloor(sector_t* sec, ffloor_t* ffloor);

static void Add_Friction(int friction, int movefactor, int affectee, boolean roverfriction);

/** Adds a 3Dfloor.
  *
  * \param sec    Target sector.
  * \param sec2   Control sector.
  * \param master Control linedef.
  * \param flags  Options affecting this 3Dfloor.
  * \return Pointer to the new 3Dfloor.
  * \sa P_AddFFloor, P_AddFakeFloorsByLine, P_SpawnSpecials
  */
static ffloor_t* P_AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags)
{
	ffloor_t* ffloor;
	thinker_t* th;
	friction_t* f;
	pusher_t* p;

	if(sec2->ceilingheight < sec2->floorheight)
		I_Error("One of your FOFs with a tag of %i has a top height less than that of the bottom.\n", master->tag);

	if(sec2->numattached == 0)
	{
		sec2->attached = malloc(sizeof(size_t));
		if(!sec2->attached)
			I_Error("No more free memory to AddFakeFloor");
		sec2->attached[0] = sec - sectors;
		sec2->numattached = 1;
	}
	else
	{
		int i;

		for(i = 0; i < sec2->numattached; i++)
			if(sec2->attached[i] == (size_t)(sec - sectors))
				return NULL;

		sec2->attached = realloc(sec2->attached, sizeof(size_t)*(sec2->numattached + 1));
		sec2->attached[sec2->numattached] = sec - sectors;
		sec2->numattached++;
	}

	// Add the floor
	ffloor = Z_Malloc(sizeof(ffloor_t), PU_LEVEL, NULL);
	ffloor->secnum = (int)(sec2 - sectors);
	ffloor->target = sec;
	ffloor->bottomheight = &sec2->floorheight;
	ffloor->bottompic = &sec2->floorpic;
	ffloor->bottomxoffs = &sec2->floor_xoffs;
	ffloor->bottomyoffs = &sec2->floor_yoffs;

	// Add the ceiling
	ffloor->topheight = &sec2->ceilingheight;
	ffloor->toppic = &sec2->ceilingpic;
	ffloor->toplightlevel = &sec2->lightlevel;
	ffloor->topxoffs = &sec2->ceiling_xoffs;
	ffloor->topyoffs = &sec2->ceiling_yoffs;

	ffloor->flags = flags;
	ffloor->master = master;

	// scan the thinkers
	// to see if this FOF should have friction
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 != (actionf_p1)T_Friction)
			continue;

		f = (friction_t*)th;

		if(&sectors[f->affectee] == sec2)
			Add_Friction(f->friction, f->movefactor, (int)(sec-sectors), true);
	}

	// scan the thinkers
	// to see if this FOF should have wind/current/pusher
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 != (actionf_p1)T_Pusher)
			continue;

		p = (pusher_t*)th;

		if(&sectors[p->affectee] == sec2)
			Add_Pusher(p->type, p->x_mag<<FRACBITS, p->y_mag<<FRACBITS, p->source, (int)(sec-sectors), p->affectee);
	}

	if(flags & FF_TRANSLUCENT)
	{
		if(sides[master->sidenum[0]].toptexture > 0)
			ffloor->alpha = sides[master->sidenum[0]].toptexture;
		else
			ffloor->alpha = 0x80;
	}
	else
		ffloor->alpha = 0xff;

	if(flags & FF_QUICKSAND)
		CheckForQuicksand = true;

	if(flags & FF_AIRBOB)
		CheckForAirBob = true;

	if((flags & FF_BUSTUP) || (flags & FF_SHATTER) || (flags & FF_SPINBUST))
		CheckForBustableBlocks = true;

	if((flags & FF_MARIO))
	{
		P_AddBlockThinker(sec2, sec, master);
		CheckForMarioBlocks = true;
	}

	if((flags & FF_CRUMBLE))
		sec2->crumblestate = 1;

	if((flags & FF_FLOATBOB))
	{
		P_AddFloatThinker(sec2, sec);
		CheckForFloatBob = true;
	}

	P_AddFFloor(sec, ffloor);

	return ffloor;
}

/** Adds a newly formed 3Dfloor structure to a sector's ffloors list.
  *
  * \param sec    Target sector.
  * \param ffloor Newly formed 3Dfloor structure.
  * \todo Give this function a less confusing name.
  * \sa P_AddFakeFloor
  */
static inline void P_AddFFloor(sector_t* sec, ffloor_t* ffloor)
{
	ffloor_t* rover;

	if(!sec->ffloors)
	{
		sec->ffloors = ffloor;
		ffloor->next = 0;
		ffloor->prev = 0;
		return;
	}

	for(rover = sec->ffloors; rover->next; rover = rover->next);

	rover->next = ffloor;
	ffloor->prev = rover;
	ffloor->next = 0;
}

//
// SPECIAL SPAWNING
//

/** Adds a spike thinker.
  * Sector type 4 will result in this effect.
  *
  * \param sec Sector in which to add the thinker.
  * \sa P_SpawnSpecials, T_SpikeSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddSpikeThinker(sector_t* sec)
{
	elevator_t* elevator;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_SpikeSector;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;

	return;
}

/** Adds a float thinker.
  * Float thinkers cause solid 3Dfloors to float on water.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \sa P_SpawnSpecials, T_FloatSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddFloatThinker(sector_t* sec, sector_t* actionsector)
{
	elevator_t* elevator;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_FloatSector;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->actionsector = actionsector;

	return;
}

/** Adds a Mario block thinker, which changes the block's texture between blank
  * and ? depending on whether it has contents.
  * Needed in case objects respawn inside.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_MarioBlockChecker
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddBlockThinker(sector_t* sec, sector_t* actionsector, line_t* sourceline)
{
	elevator_t* elevator;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_MarioBlockChecker;
	elevator->sourceline = sourceline;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->actionsector = actionsector;

	return;
}

/** Adds a thwomp thinker.
  * Even thwomps need to think!
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_ThwompSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddThwompThinker(sector_t* sec, sector_t* actionsector, line_t* sourceline)
{
	elevator_t* elevator;

	// You *probably* already have a thwomp in this sector. If you've combined it with something
	// else that uses the floordata/ceilingdata, you must be weird.
	if(sec->floordata || sec->ceilingdata)
		return;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_ThwompSector;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->actionsector = actionsector;
	elevator->floorwasheight = elevator->sector->floorheight;
	elevator->ceilingwasheight = elevator->sector->ceilingheight;
	elevator->direction = 0;
	elevator->distance = 1;
	elevator->sourceline = sourceline;
	elevator->sector->floordata = elevator;
	elevator->sector->ceilingdata = elevator;

	return;
}

/** Adds a camera scanner.
  *
  * \param sourcesec    Control sector.
  * \param actionsector Target sector.
  * \param angle        Angle of the source line.
  * \sa P_SpawnSpecials, T_CameraScanner
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddCameraScanner(sector_t* sourcesec, sector_t* actionsector, angle_t angle)
{
	elevator_t* elevator; // Why not? LOL

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sourcesec;
	elevator->actionsector = actionsector;
	elevator->distance = angle;
	return;
}

/** Flashes a laser block.
  *
  * \param flash Thinker structure for this laser.
  * \sa EV_AddLaserThinker
  * \author SSNTails <http://www.ssntails.org>
  */
void T_LaserFlash(laserthink_t* flash)
{
	msecnode_t* node;
	mobj_t* thing;
	sector_t* sourcesec;
	fixed_t zplusheight;

	if(leveltime & 1)
		flash->ffloor->flags |= FF_RENDERALL;
	else
		flash->ffloor->flags &= ~FF_RENDERALL;

	sourcesec = flash->ffloor->master->frontsector; // Less to type!

	// Seek out objects to DESTROY! MUAHAHHAHAHAA!!!*cough*
	for(node = flash->sector->touching_thinglist; node; node = node->m_snext)
	{
		thing = node->m_thing;

		zplusheight = thing->z + thing->height;

		if(thing->flags & MF_SHOOTABLE && ((thing->z < sourcesec->ceilingheight
			&& thing->z > sourcesec->floorheight) || (zplusheight < sourcesec->ceilingheight
			&& zplusheight > sourcesec->floorheight) || (thing->z < sourcesec->floorheight
			&& zplusheight > sourcesec->ceilingheight)))
		{
			P_DamageMobj(thing, NULL, NULL, 1);
		}
	}
}

/** Adds a laser thinker to a 3Dfloor.
  *
  * \param ffloor 3Dfloor to turn into a laser block.
  * \param sector Target sector.
  * \sa T_LaserFlash
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void EV_AddLaserThinker(ffloor_t* ffloor, sector_t* sector)
{
	laserthink_t* flash;

	if(!ffloor)
		return;

	flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);

	P_AddThinker(&flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
	flash->ffloor = ffloor;
	flash->sector = sector; // For finding mobjs
}

//
// axtoi
//
// Converts an ASCII Hex string into an integer. Thanks, Borland!
//
// This probably shouldn't be here, but in the utility files...?
//
static int axtoi(char *hexStg)
{
	int n = 0;
	int m = 0;
	int count;
	int intValue = 0;
	int digit[5];
	while (n < 4)
	{
		if(hexStg[n] == '\0')
			break;
		if(hexStg[n] > 0x29 && hexStg[n] < 0x40) // 0-9
			digit[n] = (hexStg[n] & 0x0f);
		else if(hexStg[n] >= 'a' && hexStg[n] <= 'f') // a-f
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else if(hexStg[n] >= 'A' && hexStg[n] <= 'F') // A-F
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else
			break;
		n++;
	}
	count = n;
	m = n - 1;
	n = 0;
	while(n < count)
	{
		intValue = intValue | (digit[n] << (m << 2));
		m--;
		n++;
	}
	return intValue;
}

/** After the map has loaded, scans for specials that spawn 3Dfloors and
  * thinkers.
  *
  * \todo Split up into multiple functions.
  * \todo Get rid of all the magic numbers.
  * \sa P_SpawnPrecipitation, P_SpawnFriction, P_SpawnPushers, P_SpawnScrollers
  */
void P_SpawnSpecials(void)
{
	sector_t* sector;
	int i, j;

	// Set the default gravity. Custom gravity overrides this setting.
	gravity = FRACUNIT/2;

	// Defaults in case levels don't have them set.
	sstimer = 90*TICRATE + 6;
	totalrings = 1;

	CheckForAirBob = CheckForBustableBlocks = CheckForBouncySector = CheckForQuicksand = CheckForMarioBlocks = CheckForFloatBob = false;

	// Init special SECTORs.
	sector = sectors;
	for(i = 0; i < numsectors; i++, sector++)
	{
		if(!sector->special)
			continue;

		if(sector->special == 990) // Time for special stage
		{
			sstimer = (sector->floorheight >> FRACBITS) * TICRATE + 6; // Time to finish
			totalrings = sector->ceilingheight >> FRACBITS; // Ring count for special stage
		}
		else if(sector->special == 991) // Custom gravity!
			gravity = sector->floorheight/1000;
		else if(sector->special == 4) // Spikes
			P_AddSpikeThinker(sector);

		switch(sector->special)
		{
			case 2:
				// STROBE FAST
				P_SpawnStrobeFlash(sector, FASTDARK, 0);
				break;

			case 3:
				// STROBE SLOW
				P_SpawnStrobeFlash(sector, SLOWDARK, 0);
				break;

			case 8:
				// GLOWING LIGHT
				P_SpawnGlowingLight(sector);
				break;

			case 12:
				// SYNC STROBE SLOW
				P_SpawnStrobeFlash(sector, SLOWDARK, 1);
				break;

			case 13:
				// SYNC STROBE FAST
				P_SpawnStrobeFlash(sector, FASTDARK, 1);
				break;

			// Bouncy sector
			case 14:
				CheckForBouncySector = true;
				break;

			case 17:
				P_SpawnFireFlicker(sector);
				break;
		}
	}

	if(xmasmode || mapheaderinfo[gamemap-1].weather == 2) // snow
	{
		cv_snow.value = 1;
		CV_SetValue(&cv_snow, true);
		cv_storm.value = 0;
		CV_SetValue(&cv_storm, false);
		cv_rain.value = 0;
		CV_SetValue(&cv_rain, false);
	}
	else if(mapheaderinfo[gamemap-1].weather == 3) // rain
	{
		cv_rain.value = 1;
		CV_SetValue(&cv_rain, true);
		cv_storm.value = 0;
		CV_SetValue(&cv_storm, false);
		cv_snow.value = 0;
		CV_SetValue(&cv_snow, false);
	}
	else if(mapheaderinfo[gamemap-1].weather == 1) // storm
	{
		cv_storm.value = 1;
		CV_SetValue(&cv_storm, true);
		cv_snow.value = 0;
		CV_SetValue(&cv_snow, false);
		cv_rain.value = 0;
		CV_SetValue(&cv_rain, false);
	}
	else
	{
		cv_storm.value = 0;
		cv_snow.value = 0;
		cv_rain.value = 0;
		CV_SetValue(&cv_snow, false);
		CV_SetValue(&cv_storm, false);
		CV_SetValue(&cv_rain, false);
	}

	P_InitTagLists();   // Create xref tables for tags
	P_SpawnScrollers(); // Add generalized scrollers
	P_SpawnFriction();  // Friction model using linedefs
	P_SpawnPushers();   // Pusher model using linedefs

	// Look for disable linedefs
	for(i = 0; i < numlines; i++)
	{
		if(lines[i].special == 73)
		{
			for(j = -1; (j = P_FindLineFromLineTag(&lines[i], j)) >= 0;)
			{
				lines[j].tag = 0;
				lines[j].special = 0;
			}
			lines[i].special = 0;
			lines[i].tag = 0;
		}
	}

	// Init line EFFECTs
	for(i = 0; i < numlines; i++)
	{
		// Disable special lines in certain difficulty modes.

		// actually set the line specials to 0, otherwise linedef executors aren't affected
		if((lines[i].flags & ML_NOEASY) && gameskill <= sk_easy)
		{
			lines[i].special = 0;
			continue;
		}
		if((lines[i].flags & ML_NONORMAL) && gameskill == sk_medium)
		{
			lines[i].special = 0;
			continue;
		}
		if((lines[i].flags & ML_NOHARD) && gameskill >= sk_hard)
		{
			lines[i].special = 0;
			continue;
		}

		// set line specials to 0 here too, same reason as above
		if(!(netgame || multiplayer))
		{
			if(players[consoleplayer].charability == 0 && (lines[i].flags & ML_NOSONIC))
			{
				lines[i].special = 0;
				continue;
			}
			if(players[consoleplayer].charability == 1 && (lines[i].flags & ML_NOTAILS))
			{
				lines[i].special = 0;
				continue;
			}
			if(players[consoleplayer].charability == 2 && (lines[i].flags & ML_NOKNUX))
			{
				lines[i].special = 0;
				continue;
			}
		}

		lines[i].special &= 255;

		switch(lines[i].special)
		{
			int s, sec;
			int ffloorflags;
#ifdef SLOPENESS
			case 243:
				sec = sides[*lines[i].sidenum].sector-sectors;
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					int counting;

					sectors[s].floorangle = ANG45;
					for(counting = 0; counting < sectors[s].linecount/2; counting++)
					{
						sectors[s].lines[counting]->v1->z = sectors[sec].floorheight;
						CONS_Printf("Set it to %i\n", sectors[s].lines[counting]->v1->z>>FRACBITS);
					}

					for(counting = sectors[s].linecount/2; counting < sectors[s].linecount; counting++)
					{
						sectors[s].lines[counting]->v1->z = sectors[sec].ceilingheight;
						CONS_Printf("Set it to %i\n", sectors[s].lines[counting]->v1->z>>FRACBITS);
					}
					sectors[s].special = 16384;
					CONS_Printf("Found & Set slope!\n");
				}
				break;
#endif

			case 1: // Air bobbing platform that will crumble and bob on
			        // the water when it falls and hits, then never return
				sec = (int)(sides[*lines[i].sidenum].sector-sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB|FF_AIRBOB|FF_CRUMBLE|FF_NORETURN);
				}
				break;

			case 2: // New super cool and awesome moving floor and ceiling type
			case 3: // New super cool and awesome moving floor type
				if(lines[i].backsector)
					EV_DoFloor(&lines[i], bounceFloor);
				if(lines[i].special == 3)
					break;
			case 4: // New super cool and awesome moving ceiling type
				if(lines[i].backsector)
					EV_DoCeiling(&lines[i], bounceCeiling);
				break;

			case 5: // ceiling lighting independently
				sec = (int)(sides[*lines[i].sidenum].sector-sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					sectors[s].ceilinglightsec = sec;
				break;

			case 6: // New super cool and awesome moving floor and ceiling crush type
			case 7: // New super cool and awesome moving floor crush type
				if(lines[i].backsector)
					EV_DoFloor(&lines[i], bounceFloorCrush);
				if(lines[i].special == 3)
					break;
			case 8: // New super cool and awesome moving ceiling crush type
				if(lines[i].backsector)
					EV_DoCeiling(&lines[i], bounceCeilingCrush);
				break;

			case 16: // HACK! Copy colormaps. Just plain colormaps.
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					sectors[s].midmap = lines[i].frontsector->midmap;
					sectors[s].altheightsec = 2;
				}
				break;

			case 24: // Instant raise for ceilings
				EV_DoCeiling(&lines[i], instantRaise);
				break;

			case 25: // FOF (solid, opaque, shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL);
				break;

			case 26: // Insta-Lower Sector
				EV_DoFloor(&lines[i], instantLower);
				break;

			case 33: // FOF (solid, opaque, no shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL);
				break;

			case 34: // Float/bob platform
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB);
				}
				break;

			case 35: // Crumbling platform that will not return
				P_AddFakeFloorsByLine(i,
					FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_NORETURN);
				break;

			case 36: // Crumbling platform
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE);
				break;

			case 37: // Crumbling platform that will float when it hits water
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_FLOATBOB);
				}
				break;

			case 38: // Air bobbing platform
			case 68: // Adjustable air bobbing platform
			case 72: // Adjustable air bobbing platform in reverse
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_AIRBOB);
				break;
			/// \todo use FF_ flags for reverse and adjustable, instead of using the linetype elsewhere

			case 39: // Air bobbing platform that will crumbleand bob on the water when it falls and hits
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB|FF_AIRBOB|FF_CRUMBLE);
				}
				break;

			case 40: // Air bobbing platform that will crumble
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_AIRBOB|FF_CRUMBLE);
				break;

			case 41: // Mario Block
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_MARIO);
				}
				break;

			case 42: // Crumbling platform that will float when it hits water, but not return
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_FLOATBOB|FF_NORETURN);
				}
				break;

			case 43: // Crusher!
				EV_DoCrush(&lines[i], crushAndRaise);
				break;

			case 44: // TL block: FOF (solid, translucent)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA);
				break;

			case 45: // TL water
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES);
				break;

			case 46: // Fog
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				// SoM: Because it's fog, check for an extra colormap and set
				// the fog flag...
				if(sectors[sec].extra_colormap)
					sectors[sec].extra_colormap->fog = 1;
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_RENDERALL|FF_FOG|FF_BOTHPLANES|FF_INVERTPLANES|FF_ALLSIDES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES);
				break;

			case 47: // Light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES);
				break;

			case 48: // Opaque water
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES);
				break;

			case 49: // Double light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES|FF_DOUBLESHADOW);
				break;

			case 50: // Crusher (up and then down)!
				EV_DoCrush(&lines[i], fastCrushAndRaise);
				break;

			case 51: // 3D Floor type that doesn't draw sides
				// If line has no-climb set, give it shadows, otherwise don't
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERPLANES|FF_CUTLEVEL;
				if(!(lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 52: // FOF (intangible, translucent)
				// If line has no-climb set, give it shadows, otherwise don't
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA|FF_CUTSPRITES;
				if(!(lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 53: // Laser block
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);

				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					EV_AddLaserThinker(P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_RENDERALL|FF_NOSHADE|FF_EXTRA|FF_CUTEXTRA), &sectors[s]);
				break;

			case 54: // A THWOMP!
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
				{
					P_AddThwompThinker(&sectors[sec], &sectors[s], &lines[i]);
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL);
				}
				break;

			case 55: // Bustable block
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP);
				break;

			case 56: // Quicksand
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_QUICKSAND|FF_RENDERALL|FF_ALLSIDES|FF_CUTSPRITES);
				break;

			case 57: // FOF (solid, invisible)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_NOSHADE);
				break;

			case 58: // FOF (intangible, invisible) - for combining specials in a sector
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_NOSHADE);
				break;

			case 59: // 'Platform' - You can jump up through it
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 60: // Adjustable pulsating light
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					P_SpawnAdjustableGlowingLight(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 61: // Adjustable flickering light
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					P_SpawnAdjustableFireFlicker(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 62: // Like opaque water, but not swimmable. (Good for snow effect on FOFs)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES);
				break;

			case 63: // Change camera info
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					P_AddCameraScanner(&sectors[sec], &sectors[s], R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y)/ANGLE_1);
				break;

			case 64: // Definable gravity per sector
				sec = (int)(sides[*lines[i].sidenum].sector - sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					sectors[s].gravity = &sectors[sec].floorheight; // This allows it to change in realtime!
				break;

			case 65: // Speed pad (combines with sector special 976 or 977)
				break;

			case 66: // Flat alignment
				if(!(lines[i].flags & ML_BLOCKMONSTERS)) // Align floor unless BLOCKMONSTERS flag is set
				{
					for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					{
						sectors[s].floor_xoffs += lines[i].dx;
						sectors[s].floor_yoffs += lines[i].dy;
					}
				}

				if(!(lines[i].flags & ML_NOCLIMB)) // Align ceiling unless NOCLIMB flag is set
				{
					for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					{
						sectors[s].ceiling_xoffs += lines[i].dx;
						sectors[s].ceiling_yoffs += lines[i].dy;
					}
				}
				break;

			case 67: // FOF with no floor/ceiling (good for GFZGRASS effect on FOFs)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERSIDES|FF_NOSHADE|FF_ALLSIDES);
				break;

			// case 68 is handled above

			case 69: // Solid FOF with no floor/ceiling (quite possibly useless)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERSIDES|FF_NOSHADE|FF_CUTLEVEL);
				break;

			case 70: // Ideya time
			{
				thinker_t* th;
				mobj_t* mo2;

				// scan thinkers to find Nights drone
				for(th = thinkercap.next; th != &thinkercap; th=th->next)
				{
					if(th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t*)th;

					if(mo2->type == MT_NIGHTSDRONE)
						mo2->health = P_AproxDistance(lines[i].dx,lines[i].dy)>>FRACBITS;
				}
			}
			break;

			case 71: // Custom exit
				break;

			// case 72 is handled above

#ifdef PARANOIA
			case 73: // Disable tags if level not cleared
				I_Error("Failed to catch a disable linedef");
				break;
#endif

			case 74: // TL water, no sides
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERPLANES|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES);
				break;

			case 75: // Opaque water, no sides
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERPLANES|FF_SWIMMABLE|FF_BOTHPLANES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES);
				break;

			case 76: // Shatter block (breaks when touched)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SHATTER);
				break;

			case 77: // Translucent "platform" with no sides
				ffloorflags = FF_EXISTS|FF_SOLID|FF_CUTLEVEL|FF_RENDERPLANES|FF_TRANSLUCENT|FF_PLATFORM|FF_BOTHPLANES;
				if(lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;
				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 78: // Spin bust block (breaks when jumped or spun downwards onto)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST);
				break;

			case 79: // "Platform" that crumbles and returns
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 80: // "Platform" that crumbles and doesn't return
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_NORETURN|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 81: // Translucent "platform"
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 82: // Translucent "platform" that crumbles and returns
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 83: // Translucent "platform" that crumbles and doesn't return
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_NORETURN|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES;
				if(lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags);
				break;

			case 84: // Translucent spin bust block (see 78)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST|FF_TRANSLUCENT);
				break;

			// 85 is used for a scroller

			case 86: // Translucent shatter block (see 76)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SHATTER|FF_TRANSLUCENT);
				break;

			case 87: // Make-Your-Own FOF!
				if(lines[i].sidenum[1] != -1)
				{
					byte* data = W_CacheLumpNum(lastloadedmaplumpnum + ML_SIDEDEFS,PU_STATIC);
					int b;

					for(b = 0; b < numsides; b++)
					{
						register mapsidedef_t* msd = (mapsidedef_t*)data + b;

						if(b == lines[i].sidenum[1])
						{
							if((msd->toptexture[0] >= '0' && msd->toptexture[0] <= '9')
								|| (msd->toptexture[0] >= 'A' && msd->toptexture[0] <= 'F'))
							{
								int FOF_Flags;

								FOF_Flags = axtoi(msd->toptexture);

								P_AddFakeFloorsByLine(i, FOF_Flags);
								break;
							}
							else
								I_Error("Make-Your-Own-FOF (tag %i) needs a value in the linedef's second side upper texture field.", lines[i].tag);
						}
					}
					Z_Free(data);
				}
				else
					I_Error("Make-Your-Own FOF (tag %i) found without a 2nd linedef side!", lines[i].tag);
				break;

			case 88: // Continuously Falling sector
				EV_DoContinuousFall(lines[i].frontsector, lines[i].backsector->floorheight, P_AproxDistance(lines[i].dx, lines[i].dy));
				break;

			case 96: // Linedef executor (combines with sector special 974/975) and commands
			case 97:
			case 98:
			// 100 is used for a scroller
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:
			case 113:
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 123:
			case 124:
			case 125:
				break;

			// 200-205 are used for scrollers

			case 213: // floor lighting independently (e.g. lava)
				sec = (int)(sides[*lines[i].sidenum].sector-sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					sectors[s].floorlightsec = sec;
				break;

			// 214-218 are used for scrollers
			// 223 is used for friction
			// 224 is used for wind
			// 225 is used for current
			// 226 is used for push/pull
			// 227 is used for upwards current
			// 228 is used for downwards current
			// 229 is used for upwards wind
			// 230 is used for downwards wind

			case 232: // Activate floating platform
				EV_DoElevator(&lines[i], elevateContinuous, false);
				break;

			case 233: // Floating platform with adjustable speed
				EV_DoElevator(&lines[i], elevateContinuous, true);
				break;

			case 242: // support for drawn heights coming from different sector
				sec = (int)(sides[*lines[i].sidenum].sector-sectors);
				for(s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
					sectors[s].heightsec = sec;
				break;

			// 245-255 are used for scrollers

			default:
				break;
		}
	}
}

/** Adds 3Dfloors as appropriate based on a common control linedef.
  *
  * \param line        Control linedef to use.
  * \param ffloorflags 3Dfloor flags to use.
  * \sa P_SpawnSpecials, P_AddFakeFloor
  * \author Graue <graue@oceanbase.org>
  */
static void P_AddFakeFloorsByLine(int line, int ffloorflags)
{
	int s, sec;

	sec = (int)(sides[*lines[line].sidenum].sector-sectors);
	for(s = -1; (s = P_FindSectorFromLineTag(lines+line, s)) >= 0;)
		P_AddFakeFloor(&sectors[s], &sectors[sec], lines+line, ffloorflags);
}

/*
 SoM: 3/8/2000: General scrolling functions.
 T_Scroll,
 Add_Scroller,
 Add_WallScroller,
 P_SpawnScrollers
*/

/** Processes an active scroller.
  * This function, with the help of r_plane.c and r_bsp.c, supports generalized
  * scrolling floors and walls, with optional mobj-carrying properties, e.g.
  * conveyor belts, rivers, etc. A linedef with a special type affects all
  * tagged sectors the same way, by creating scrolling and/or object-carrying
  * properties. Multiple linedefs may be used on the same sector and are
  * cumulative, although the special case of scrolling a floor and carrying
  * things on it requires only one linedef.
  *
  * The linedef's direction determines the scrolling direction, and the
  * linedef's length determines the scrolling speed. This was designed so an
  * edge around a sector can be used to control the direction of the sector's
  * scrolling, which is usually what is desired.
  *
  * \param s Thinker for the scroller to process.
  * \todo Split up into multiple functions.
  * \todo Use attached lists to make ::sc_carry_ceiling case faster and
  *       cleaner.
  * \sa Add_Scroller, Add_WallScroller, P_SpawnScrollers
  * \author Steven McGranahan
  * \author Graue <graue@oceanbase.org>
  */
void T_Scroll(scroll_t* s)
{
	fixed_t dx = s->dx, dy = s->dy;

	if(s->control != -1)
	{ // compute scroll amounts based on a sector's height changes
		fixed_t height = sectors[s->control].floorheight +
			sectors[s->control].ceilingheight;
		fixed_t delta = height - s->last_height;
		s->last_height = height;
		dx = FixedMul(dx, delta);
		dy = FixedMul(dy, delta);
	}

	if(s->accel)
	{
		s->vdx = dx += s->vdx;
		s->vdy = dy += s->vdy;
	}

	if(!(dx | dy)) // no-op if both (x,y) offsets 0
		return;

	switch(s->type)
	{
		side_t* side;
		sector_t* sec;
		fixed_t height, waterheight;
		msecnode_t* node;
		mobj_t* thing;
		line_t* line;
		int i, sect;

		case sc_side: // scroll wall texture
			side = sides + s->affectee;
			side->textureoffset += dx;
			side->rowoffset += dy;
			break;

		case sc_floor: // scroll floor texture
			sec = sectors + s->affectee;
			sec->floor_xoffs += dx;
			sec->floor_yoffs += dy;
			break;

		case sc_ceiling: // scroll ceiling texture
			sec = sectors + s->affectee;
			sec->ceiling_xoffs += dx;
			sec->ceiling_yoffs += dy;
			break;

		case sc_carry:
			sec = sectors + s->affectee;
			height = sec->floorheight;
			waterheight = sec->heightsec != -1 &&
				sectors[sec->heightsec].floorheight > height ?
				sectors[sec->heightsec].floorheight : MININT;

			for(node = sec->touching_thinglist; node; node = node->m_snext)
				if(!((thing = node->m_thing)->flags & MF_NOCLIP) &&
					(!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
					thing->z < waterheight))
				{
					// Move objects only if on floor or underwater,
					// non-floating, and clipped.
					thing->momx += dx;
					thing->momy += dy;
					if(thing->player)
					{
						thing->player->cmomx += dx;
						thing->player->cmomy += dy;
						thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
						thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
					}
				}
			break;

		case sc_carry_ceiling: // carry on ceiling (FOF scrolling)
			sec = sectors + s->affectee;
			height = sec->ceilingheight;
			waterheight = sec->heightsec != -1 &&
				sectors[sec->heightsec].floorheight > height ?
				sectors[sec->heightsec].floorheight : MININT;

			// sec is the control sector, find the real sector(s) to use
			for(i = 0; i < sec->linecount; i++)
			{
				boolean is3dblock;

				line = sec->lines[i];
				switch(line->special)
				{
					case 25: // 3D floor: solid, opaque, shadowcasting
					case 33: // 3D floor: solid, opaque, no shadows
					case 44: // 3D floor: solid, translucent
					case 51: // 3D floor: solid, no sides
					case 57: // 3D floor: solid, invisible
					case 69: // 3D floor: solid, sides only
						is3dblock = true;
						break;
					default: // Ignore specials that are not solid 3D blocks
						is3dblock = false;
					break;
				}

				if(!is3dblock)
					continue;

				for(sect = -1; (sect = P_FindSectorFromTag(line->tag, sect)) >= 0;)
				{
					sector_t* sec;
					sec = sectors + sect;

					for(node = sec->touching_thinglist; node; node = node->m_snext)
					{
						thing = node->m_thing;

						if(!(thing->flags & MF_NOCLIP)) // Thing must be clipped
						if(!(thing->flags & MF_NOGRAVITY || thing->z != height) ||
							thing->z < waterheight) // Thing must a) be non-floating and have z == height
							                        // or b) be underwater
						{
							// Move objects only if on floor or underwater,
							// non-floating, and clipped.
							thing->momx += dx;
							thing->momy += dy;
							if(thing->player)
							{
								thing->player->cmomx += dx;
								thing->player->cmomy += dy;
								thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
								thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
							}
						};
					} // end of for loop through touching_thinglist
				} // end of loop through sectors
			}
			break; // end of sc_carry_ceiling
	} // end of switch
}

/** Adds a generalized scroller to the thinker list.
  *
  * \param type     The enumerated type of scrolling.
  * \param dx       x speed of scrolling or its acceleration.
  * \param dy       y speed of scrolling or its acceleration.
  * \param control  Sector whose heights control this scroller's effect
  *                 remotely, or -1 if there is no control sector.
  * \param affectee Index of the affected object, sector or sidedef.
  * \param accel    Nonzero for an accelerative effect.
  * \sa Add_WallScroller, P_SpawnScrollers, T_Scroll
  */
static void Add_Scroller(int type, fixed_t dx, fixed_t dy, int control, int affectee, int accel)
{
	scroll_t* s = Z_Malloc(sizeof *s, PU_LEVSPEC, 0);
	s->thinker.function.acp1 = (actionf_p1)T_Scroll;
	s->type = type;
	s->dx = dx;
	s->dy = dy;
	s->accel = accel;
	s->vdx = s->vdy = 0;
	if((s->control = control) != -1)
		s->last_height = sectors[control].floorheight + sectors[control].ceilingheight;
	s->affectee = affectee;
	P_AddThinker(&s->thinker);
}

/** Adds a wall scroller.
  * Scroll amount is rotated with respect to wall's linedef first, so that
  * scrolling towards the wall in a perpendicular direction is translated into
  * vertical motion, while scrolling along the wall in a parallel direction is
  * translated into horizontal motion.
  *
  * \param dx      x speed of scrolling or its acceleration.
  * \param dy      y speed of scrolling or its acceleration.
  * \param l       Line whose front side will scroll.
  * \param control Sector whose heights control this scroller's effect
  *                remotely, or -1 if there is no control sector.
  * \param accel   Nonzero for an accelerative effect.
  * \sa Add_Scroller, P_SpawnScrollers
  */
static void Add_WallScroller(fixed_t dx, fixed_t dy, const line_t* l, int control, int accel)
{
	fixed_t x = abs(l->dx), y = abs(l->dy), d;
	if(y > x)
		d = x, x = y, y = d;
	d = FixedDiv(x, finesine[(tantoangle[FixedDiv(y, x) >> DBITS] + ANG90) >> ANGLETOFINESHIFT]);
	x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
	y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
	Add_Scroller(sc_side, x, y, control, *l->sidenum, accel);
}

// Amount (dx, dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*0.09375f))

/** Initializes the scrollers.
  *
  * \todo Get rid of all the magic numbers.
  * \sa P_SpawnSpecials, Add_Scroller, Add_WallScroller
  */
static void P_SpawnScrollers(void)
{
	int i;
	line_t* l = lines;

	for(i = 0; i < numlines; i++, l++)
	{
		fixed_t dx = l->dx >> SCROLL_SHIFT; // direction and speed of scrolling
		fixed_t dy = l->dy >> SCROLL_SHIFT;
		int control = -1, accel = 0; // no control sector or acceleration
		int special = l->special;

		// Types 245-249 are same as 250-254 except that the
		// first side's sector's heights cause scrolling when they change, and
		// this linedef controls the direction and speed of the scrolling. The
		// most complicated linedef since donuts, but powerful :)

		if(special >= 245 && special <= 249) // displacement scrollers
		{
			special += 250-245;
			control = (int)(sides[*l->sidenum].sector - sectors);
		}
		else if(special >= 214 && special <= 218) // accelerative scrollers
		{
			accel = 1;
			special += 250-214;
			control = (int)(sides[*l->sidenum].sector - sectors);
		}
		else if(special == 200 || special == 201) // displacement scrollers
		{
			special += 2;
			control = (int)(sides[*l->sidenum].sector - sectors);
		}
		else if(special == 204 || special == 205) // accelerative scrollers
		{
			accel = 1;
			special -= 2;
			control = (int)(sides[*l->sidenum].sector - sectors);
		}

		switch(special)
		{
			register int s;

			case 250: // scroll effect ceiling
			case 202: // scroll and carry objects on ceiling
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Scroller(sc_ceiling, -dx, dy, control, s, accel);
				if(special != 202)
					break;

			case 203:	// carry objects on ceiling
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Scroller(sc_carry_ceiling, dx, dy, control, s, accel);
				break;

			case 251: // scroll effect floor
			case 253: // scroll and carry objects on floor
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Scroller(sc_floor, -dx, dy, control, s, accel);
				if(special != 253)
					break;

			case 252:	// carry objects on floor
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Scroller(sc_carry, dx, dy, control, s, accel);
				break;

			// scroll wall according to linedef
			// (same direction and speed as scrolling floors)
			case 254:
				for(s = -1; (s = P_FindLineFromLineTag(l, s)) >= 0;)
					if(s != i)
						Add_WallScroller(dx, dy, lines+s, control, accel);
				break;

			case 255:
				s = lines[i].sidenum[0];
				Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, s, accel);
				break;

			case 100: // scroll first side
				Add_Scroller(sc_side, FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
				break;

			case 85: // jff 1/30/98 2-way scroll
				Add_Scroller(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
				break;
		}
	}
}

/*
 SoM: 3/8/2000: Friction functions start.
 Add_Friction,
 T_Friction,
 P_SpawnFriction
*/

/** Adds friction thinker.
  *
  * \param friction      Friction value, 0xe800 is normal.
  * \param movefactor    Inertia factor.
  * \param affectee      Target sector.
  * \param roverfriction FOF or not
  * \sa T_Friction, P_SpawnFriction
  */
static void Add_Friction(int friction, int movefactor, int affectee, boolean roverfriction)
{
	friction_t* f = Z_Malloc(sizeof *f, PU_LEVSPEC, 0);

	f->thinker.function.acp1 = (actionf_p1)T_Friction;
	f->friction = friction;
	f->movefactor = movefactor;
	f->affectee = affectee;
	f->roverfriction = roverfriction;
	P_AddThinker(&f->thinker);
}

/** Applies friction to all things in a sector.
  *
  * \param f Friction thinker.
  * \sa Add_Friction
  */
void T_Friction(friction_t* f)
{
	sector_t* sec;
	mobj_t* thing;
	msecnode_t* node;

	sec = sectors + f->affectee;

	// Assign the friction value to players on the floor, non-floating,
	// and clipped. Normally the object's friction value is kept at
	// ORIG_FRICTION and this thinker changes it for icy or muddy floors.

	// In Phase II, you can apply friction to Things other than players.

	// When the object is straddling sectors with the same
	// floorheight that have different frictions, use the lowest
	// friction value (muddy has precedence over icy).

	node = sec->touching_thinglist; // things touching this sector
	while(node)
	{
		thing = node->m_thing;
		// apparently, all I had to do was comment out part of the next line and
		// friction works for all mobj's
		// (or at least MF_PUSHABLEs, which is all I care about anyway)
		if(!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) && thing->z == thing->floorz)
		{
			if(f->roverfriction == true && sec->ffloors)
			{
				ffloor_t* rover;
				thinker_t* th;
				friction_t* FOFfriction;

				for(th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if(th->function.acp1 != (actionf_p1)T_Friction)
						continue;

					FOFfriction = (friction_t*)th;

					for(rover = sec->ffloors; rover; rover = rover->next)
					{
						if(*rover->topheight != thing->floorz)
							continue;

						if(&sectors[FOFfriction->affectee] == rover->master->frontsector)
						{
							if((thing->friction == ORIG_FRICTION) || // normal friction?
								(FOFfriction->friction < thing->friction))
							{
								thing->friction = FOFfriction->friction;
								thing->movefactor = FOFfriction->movefactor;
							}
						}
					}
				}
			}

			if((f->roverfriction == false && sec->floorheight == thing->floorz) && ((thing->friction == ORIG_FRICTION) || // normal friction?
				(f->friction < thing->friction)))
			{
				thing->friction = f->friction;
				thing->movefactor = f->movefactor;
			}
		}
		node = node->m_snext;
	}
}

/** Spawns all friction effects.
  *
  * \sa P_SpawnSpecials, Add_Friction
  */
static void P_SpawnFriction(void)
{
	int i;
	line_t* l = lines;
	register int s;
	int length; // line length controls magnitude
	int friction; // friction value to be applied during movement
	int movefactor; // applied to each player move to simulate inertia

	for(i = 0; i < numlines; i++, l++)
		if(l->special == 223)
		{
			length = P_AproxDistance(l->dx, l->dy)>>FRACBITS;
			friction = (0x1EB8*length)/0x80 + 0xD000;

			if(friction > FRACUNIT)
				friction = FRACUNIT;
			if(friction < 0)
				friction = 0;

			// The following check might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.

			if(friction > ORIG_FRICTION) // ice
				movefactor = ((0x10092 - friction)*(0x70))/0x158;
			else
				movefactor = ((friction - 0xDB34)*(0xA))/0x80;

			// killough 8/28/98: prevent odd situations
			if(movefactor < 32)
				movefactor = 32;

			for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
				Add_Friction(friction,movefactor,s,false);
		}
}

/*
 SoM: 3/8/2000: Push/Pull/Wind/Current functions.
 Add_Pusher,
 PIT_PushThing,
 T_Pusher,
 P_GetPushThing,
 P_SpawnPushers
*/

#define PUSH_FACTOR 7

/** Adds a pusher.
  *
  * \param type     Type of push/pull effect.
  * \param x_mag    X magnitude.
  * \param y_mag    Y magnitude.
  * \param source   For a point pusher/puller, the source object.
  * \param affectee Target sector.
  * \param referrer What sector set it
  * \sa T_Pusher, P_GetPushThing, P_SpawnPushers
  */
static void Add_Pusher(pushertype_e type, int x_mag, int y_mag, mobj_t* source, int affectee, int referrer)
{
	pusher_t* p = Z_Malloc(sizeof *p, PU_LEVSPEC, 0);

	p->thinker.function.acp1 = (actionf_p1)T_Pusher;
	p->source = source;
	p->type = type;
	p->x_mag = x_mag>>FRACBITS;
	p->y_mag = y_mag>>FRACBITS;

	if(referrer != -1)
	{
		p->roverpusher = true;
		p->referrer = referrer;
	}

	// "The right triangle of the square of the length of the hypotenuse is equal to the sum of the squares of the lengths of the other two sides."
	// "Bah! Stupid brains! Don't you know anything besides the Pythagorean Theorem?" - Earthworm Jim
	if(type == p_downcurrent || type == p_upcurrent || type == p_upwind || type == p_downwind)
		p->magnitude = P_AproxDistance(p->x_mag,p->y_mag)<<(FRACBITS-PUSH_FACTOR);
	else
		p->magnitude = P_AproxDistance(p->x_mag,p->y_mag);
	if(source) // point source exist?
	{
		p->radius = (p->magnitude)<<(FRACBITS+1); // where force goes to zero
		p->x = p->source->x;
		p->y = p->source->y;
		p->z = p->source->z;
	}
	p->affectee = affectee;
	P_AddThinker(&p->thinker);
}


// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
static pusher_t* tmpusher; // pusher structure for blockmap searches

/** Applies a point pusher/puller to a thing.
  *
  * \param thing Thing to be pushed.
  * \return True if the thing was pushed.
  * \todo Make a more robust P_BlockThingsIterator() so the hidden parameter
  *       ::tmpusher won't need to be used.
  * \sa T_Pusher
  */
boolean PIT_PushThing(mobj_t* thing)
{
	if(thing->player &&	!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
	{
		int dist;
		int speed;
		int sx, sy, sz;

		sx = tmpusher->x;
		sy = tmpusher->y;
		sz = tmpusher->z;

		// don't fade wrt Z if health & 2 (mapthing has multi flag)
		if(tmpusher->source->health & 2)
			dist = P_AproxDistance(thing->x - sx,thing->y - sy);
		else
		{
			// Make sure the Z is in range
			if(thing->z < sz - tmpusher->radius || thing->z > sz + tmpusher->radius)
				return false;

			dist = P_AproxDistance(P_AproxDistance(thing->x - sx, thing->y - sy),
				thing->z - sz);
		}

		speed = (tmpusher->magnitude - ((dist>>FRACBITS)>>1))<<(FRACBITS - PUSH_FACTOR - 1);

		// If speed <= 0, you're outside the effective radius. You also have
		// to be able to see the push/pull source point.

		// Written with bits and pieces of P_HomingAttack
		if((speed > 0) && (P_CheckSight(thing, tmpusher->source)))
		{
			// only push wrt Z if health & 1 (mapthing has ambush flag)
			if(tmpusher->source->health & 1)
			{
				fixed_t tmpmomx, tmpmomy, tmpmomz;

				tmpmomx = FixedMul(FixedDiv(sx - thing->x, dist), speed);
				tmpmomy = FixedMul(FixedDiv(sy - thing->y, dist), speed);
				tmpmomz = FixedMul(FixedDiv(sz - thing->z, dist), speed);
				if(tmpusher->source->type == MT_PUSH) // away!
				{
					tmpmomx *= -1;
					tmpmomy *= -1;
					tmpmomz *= -1;
				}

				thing->momx += tmpmomx;
				thing->momy += tmpmomy;
				thing->momz += tmpmomz;

				if(thing->player)
				{
					thing->player->cmomx += tmpmomx;
					thing->player->cmomy += tmpmomy;
					thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
					thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
				}
			}
			else
			{
				angle_t pushangle;

				pushangle = R_PointToAngle2(thing->x, thing->y, sx, sy);
				if(tmpusher->source->type == MT_PUSH)
					pushangle += ANG180; // away
				pushangle >>= ANGLETOFINESHIFT;
				thing->momx += FixedMul(speed, finecosine[pushangle]);
				thing->momy += FixedMul(speed, finesine[pushangle]);

				if(thing->player)
				{
					thing->player->cmomx += FixedMul(speed,finecosine[pushangle]);
					thing->player->cmomy += FixedMul(speed,finesine[pushangle]);
					thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
					thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
				}
			}

		}
	}
	return true;
}

/** Applies a pusher to all affected objects.
  *
  * \param p Thinker for the pusher effect.
  * \todo Split up into multiple functions.
  * \sa Add_Pusher, PIT_PushThing
  */
void T_Pusher(pusher_t* p)
{
	sector_t* sec;
	mobj_t* thing;
	msecnode_t* node;
	int xspeed = 0,yspeed = 0;
	int xl, xh, yl, yh, bx, by;
	int radius;
	//int ht = 0;
	boolean inFOF;
	boolean touching;
	boolean foundfloor = false;

	inFOF = touching = false;

	xspeed = yspeed = 0;

	sec = sectors + p->affectee;

	// Be sure the special sector type is still turned on. If so, proceed.
	// Else, bail out; the sector type has been changed on us.

	if(p->roverpusher)
	{
		if(sec->ffloors)
		{
			ffloor_t* rover;

			for(rover = sec->ffloors; rover; rover = rover->next)
			{
				// Do some small extra checks here to possibly save unneeded work.
				if(!(rover->master->frontsector->special & PUSH_MASK) || rover->master->frontsector->special == 983)
					continue;

				if(&sectors[p->referrer] != rover->master->frontsector)
					continue;

				foundfloor = true;
			}
		}
	}
	else if(!(sec->special & PUSH_MASK))
		return;

	if(p->roverpusher && foundfloor == false) // Not even a 3d floor has the PUSH_MASK.
		return;

	// For constant pushers (wind/current) there are 3 situations:
	//
	// 1) Affected Thing is above the floor.
	//
	//    Apply the full force if wind, no force if current.
	//
	// 2) Affected Thing is on the ground.
	//
	//    Apply half force if wind, full force if current.
	//
	// 3) Affected Thing is below the ground (underwater effect).
	//
	//    Apply no force if wind, full force if current.
	//
	// Apply the effect to clipped players only for now.
	//
	// In Phase II, you can apply these effects to Things other than players.

	if(p->type == p_push)
	{

		// Seek out all pushable things within the force radius of this
		// point pusher. Crosses sectors, so use blockmap.

		tmpusher = p; // MT_PUSH/MT_PULL point source
		radius = p->radius; // where force goes to zero
		tmbbox[BOXTOP]    = p->y + radius;
		tmbbox[BOXBOTTOM] = p->y - radius;
		tmbbox[BOXRIGHT]  = p->x + radius;
		tmbbox[BOXLEFT]   = p->x - radius;

		xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
		xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
		yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
		yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
		for(bx = xl; bx <= xh; bx++)
			for(by = yl; by <= yh; by++)
				P_BlockThingsIterator(bx,by,PIT_PushThing);
		return;
	}

	// constant pushers p_wind and p_current
	node = sec->touching_thinglist; // things touching this sector
	for(; node; node = node->m_snext)
	{
		thing = node->m_thing;
		if(thing->flags & (MF_NOGRAVITY | MF_NOCLIP))
			continue;

		if(!(thing->flags & MF_PUSHABLE) && (thing->type != MT_PLAYER))
			continue;

		// Find the area that the 'thing' is in
		if(p->roverpusher)
		{
			if((!inFOF || !touching) // Only if both aren't true
				&& sec->ffloors)
			{
				ffloor_t* rover;

				for(rover = sec->ffloors; rover; rover = rover->next)
				{
					if(&sectors[p->referrer] != rover->master->frontsector)
						continue;

					if(*rover->topheight < thing->z || *rover->bottomheight > (thing->z + (thing->height >> 1)))
						continue;

					if(!(rover->master->frontsector->special & PUSH_MASK) || rover->master->frontsector->special == 983)
						continue;

					if(thing->z + thing->height > *rover->topheight)
						touching = true;

					if(thing->z + (thing->height >> 1) < *rover->topheight)
						inFOF = true;
				}
			}
		}
		else // Treat the entire sector as one big FOF
		{
			if(thing->z == thing->subsector->sector->floorheight)
				touching = true;
			else if(p->type != p_current)
				inFOF = true;
		}

		if(p->type == p_wind)
		{
			if(touching) // on ground
			{
				xspeed = (p->x_mag)>>1; // half force
				yspeed = (p->y_mag)>>1;
			}
			else if(inFOF)
			{
				xspeed = (p->x_mag); // full force
				yspeed = (p->y_mag);
			}
			else
				xspeed = yspeed = 0;
		}
		else if(p->type == p_upwind)
		{
			if(touching) // on ground
				thing->momz += (p->magnitude)>>1;
			else if(inFOF)
				thing->momz += p->magnitude;
			else
				xspeed = yspeed = 0;
		}
		else if(p->type == p_downwind)
		{
			if(touching) // on ground
				thing->momz -= (p->magnitude)>>1;
			else if(inFOF)
				thing->momz -= p->magnitude;
			else
				xspeed = yspeed = 0;
		}
		else // p_current
		{
			if(!touching && !inFOF) // Not in water at all
				xspeed = yspeed = 0; // no force
			else // underwater / touching water
			{
				if(p->type == p_upcurrent)
					thing->momz += p->magnitude;
				else if(p->type == p_downcurrent)
					thing->momz -= p->magnitude;
				else
				{
					xspeed = p->x_mag; // full force
					yspeed = p->y_mag;
				}
			}
		}

		if(p->type != p_downcurrent && p->type != p_upcurrent
			&& p->type != p_upwind && p->type != p_downwind)
		{
			thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
			thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
			if(thing->player)
			{
				thing->player->cmomx += xspeed<<(FRACBITS-PUSH_FACTOR);
				thing->player->cmomy += yspeed<<(FRACBITS-PUSH_FACTOR);
				thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
				thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
			}
		}
	}
}


/** Gets a push/pull object.
  *
  * \param s Sector number to look in.
  * \return Pointer to the first ::MT_PUSH or ::MT_PULL object found in the
  *         sector.
  * \sa P_GetTeleportDestThing, P_GetStarpostThing, P_GetAltViewThing
  */
mobj_t* P_GetPushThing(size_t s)
{
	mobj_t* thing;
	sector_t* sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while(thing)
	{
		switch(thing->type)
		{
			case MT_PUSH:
			case MT_PULL:
				return thing;
			default:
				break;
		}
		thing = thing->snext;
	}
	return NULL;
}

/** Spawns pushers.
  *
  * \todo Remove magic numbers.
  * \sa P_SpawnSpecials, Add_Pusher
  */
static void P_SpawnPushers(void)
{
	int i;
	line_t* l = lines;
	register int s;
	mobj_t* thing;

	for(i = 0; i < numlines; i++, l++)
		switch(l->special)
		{
			case 224: // wind
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_wind, l->dx, l->dy, NULL, s, -1);
				break;
			case 225: // current
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_current, l->dx, l->dy, NULL, s, -1);
				break;
			case 226: // push/pull
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
				{
					thing = P_GetPushThing(s);
					if(thing) // No MT_P* means no effect
						Add_Pusher(p_push, l->dx, l->dy, thing, s, -1);
				}
				break;
			case 227: // current up
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_upcurrent, l->dx, l->dy, NULL, s, -1);
				break;
			case 228: // current down
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_downcurrent, l->dx, l->dy, NULL, s, -1);
				break;
			case 229: // wind up
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_upwind, l->dx, l->dy, NULL, s, -1);
				break;
			case 230: // wind down
				for(s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
					Add_Pusher(p_downwind, l->dx, l->dy, NULL, s, -1);
				break;
		}
}

/** Gets a teleport destination object.
  *
  * \param s Sector number to look in.
  * \return Pointer to the first ::MT_TELEPORTMAN object found in the sector.
  * \sa P_GetPushThing, P_GetStarpostThing, P_GetAltViewThing
  */
mobj_t* P_GetTeleportDestThing(size_t s)
{
	mobj_t* thing;
	sector_t* sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while(thing)
	{
		if(thing->type == MT_TELEPORTMAN)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}

/** Gets a starpost object.
  *
  * \param s Sector number to look in.
  * \return Pointer to the first ::MT_STARPOST found in the sector.
  * \todo Merge this with P_GetTeleportDestThing() and P_GetAltViewThing().
  * \sa P_GetPushThing, P_GetTeleportDestThing, P_GetAltViewThing
  */
mobj_t* P_GetStarpostThing(size_t s)
{
	mobj_t* thing;
	sector_t* sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while(thing)
	{
		if(thing->type == MT_STARPOST)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}

/** Gets an alternate view object.
  *
  * \param s Sector number to look in.
  * \return Pointer to the first ::MT_ALTVIEWMAN in the sector.
  * \sa P_GetPushThing, P_GetTeleportDestThing, P_GetStarpostThing
  */
mobj_t* P_GetAltViewThing(size_t s)
{
	mobj_t* thing;
	sector_t* sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while(thing)
	{
		if(thing->type == MT_ALTVIEWMAN)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}
