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

#ifndef __P_SPEC__
#define __P_SPEC__

// at game start
void P_InitPicAnims(void);

// at map load (sectors)
void P_SetupLevelFlatAnims(void);

// at map load
void P_SpawnSpecials(void);

// every tic
void P_UpdateSpecials(void);
void P_PlayerInSpecialSector(player_t* player);

int twoSided(int sector, int line);
sector_t* getSector(int currentSector, int line, int side);
side_t* getSide(int currentSector, int line, int side);

fixed_t P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t P_FindHighestFloorSurrounding(sector_t* sec);

fixed_t P_FindNextHighestFloor(sector_t* sec, int currentheight);
fixed_t P_FindNextLowestFloor(sector_t* sec, int currentheight);

fixed_t P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec);

int P_FindSectorFromLineTag(line_t* line, int start);
int P_FindSectorFromTag(int tag, int start);
int P_FindLineFromLineTag(const line_t* line, int start);
int P_FindSpecialLineFromTag(short special, short tag, int start);
int P_FindLineFromTag(int tag, int start);
int P_FindMinSurroundingLight(sector_t* sector, int max);

void P_SetupSignExit(player_t* player);

sector_t* getNextSector(line_t* line, sector_t* sec);

fixed_t P_FindNextHighestCeiling(sector_t* sec, int currentheight);
fixed_t P_FindNextLowestCeiling(sector_t* sec, int currentheight);
fixed_t P_FindShortestUpperAround(int secnum);
fixed_t P_FindShortestTextureAround(int secnum);
sector_t* P_FindModelFloorSector(fixed_t floordestheight, int secnum);
sector_t* P_FindModelCeilingSector(fixed_t ceildestheight, int secnum);

void P_SwitchWeather(int weathernum);

void P_LinedefExecute(int tag, mobj_t* actor, sector_t* caller);
void P_ProcessLineSpecial(line_t* line, mobj_t* mo);
void P_ChangeSectorTag(int sector, int newtag);

//
// P_LIGHTS
//
/** Fire flicker action structure.
  */
typedef struct
{
	thinker_t thinker; ///< The thinker in use for the effect.
	sector_t* sector;  ///< The sector where action is taking place.
	int count;
	int resetcount;
	int maxlight;      ///< The brightest light level to use.
	int minlight;      ///< The darkest light level to use.
} fireflicker_t;

typedef struct
{
	thinker_t thinker;
	sector_t* sector;
	int maxlight;
	int minlight;
} lightflash_t;

/** Laser block thinker.
  */
typedef struct
{
	thinker_t thinker; ///< Thinker structure for laser.
	ffloor_t* ffloor;  ///< 3Dfloor that is a laser.
	sector_t* sector;  ///< Sector in which the effect takes place.
} laserthink_t;

/** Strobe light action structure..
  */
typedef struct
{
	thinker_t thinker; ///< The thinker in use for the effect.
	sector_t* sector;  ///< The sector where the action is taking place.
	int count;
	int minlight;      ///< The minimum light level to use.
	int maxlight;      ///< The maximum light level to use.
	int darktime;      ///< How long to use minlight.
	int brighttime;    ///< How long to use maxlight.
} strobe_t;

typedef struct
{
	thinker_t thinker;
	sector_t* sector;
	int minlight;
	int maxlight;
	int direction;
	int speed;
} glow_t;

/** Thinker struct for fading lights.
  */
typedef struct
{
	thinker_t thinker; ///< Thinker in use for the effect.
	sector_t* sector;  ///< Sector where action is taking place.
	int destlevel;     ///< Light level we're fading to.
	int speed;         ///< Speed at which to change light level.
} lightlevel_t;

#define GLOWSPEED 8
#define STROBEBRIGHT 5
#define FASTDARK 15
#define SLOWDARK 35

void T_FireFlicker(fireflicker_t* flick);
void P_SpawnFireFlicker(sector_t* sector);
fireflicker_t* P_SpawnAdjustableFireFlicker(sector_t* minsector, sector_t* maxsector, int length);
void T_LightningFlash(lightflash_t* flash);
void T_StrobeFlash(strobe_t* flash);

void P_SpawnLightningFlash(sector_t* sector);
void P_SpawnStrobeFlash(sector_t* sector, int fastOrSlow, int inSync);

void T_Glow(glow_t* g);
void P_SpawnGlowingLight(sector_t* sector);
glow_t* P_SpawnAdjustableGlowingLight(sector_t* minsector, sector_t* maxsector, int length);

void P_FadeLight(int tag, int destvalue, int speed);
void T_LightFade(lightlevel_t* ll);

typedef enum
{
	floor_special,
	ceiling_special,
	lighting_special,
} special_e;

//
// P_CEILNG
//
typedef enum
{
	raiseToHighest,
	lowerToLowest,
	raiseToLowest,
	lowerToLowestFast,

	instantRaise, // instant-move for ceilings

	lowerAndCrush,
	crushAndRaise,
	fastCrushAndRaise,

	moveCeilingByFrontSector,
	instantMoveCeilingByFrontSector,

	lowerCeilingByLine,
	raiseCeilingByLine,

	bounceCeiling,
	bounceCeilingCrush,
} ceiling_e;

/** Ceiling movement structure.
  */
typedef struct
{
	thinker_t thinker;    ///< Thinker for the type of movement.
	ceiling_e type;       ///< Type of movement.
	sector_t* sector;     ///< Sector where the action is taking place.
	fixed_t bottomheight; ///< The lowest height to move to.
	fixed_t topheight;    ///< The highest height to move to.
	fixed_t speed;        ///< Ceiling speed.
	fixed_t oldspeed;
	boolean crush;        ///< Whether to crush things or not.

	fixed_t texture;      ///< The number of a flat to use when done.
	int direction;        ///< 1 = up, 0 = waiting, -1 = down.

	// ID
	int tag;
	int olddirection;

	struct ceilinglist* list;

	fixed_t origspeed;    ///< The original, "real" speed.
} ceiling_t;

#define CEILSPEED (FRACUNIT/NEWTICRATERATIO)

int EV_DoCeiling(line_t* line, ceiling_e type);

int EV_DoCrush(line_t* line, ceiling_e type);
void T_CrushCeiling(ceiling_t* ceiling);

void T_MoveCeiling(ceiling_t* ceiling);

//
// P_FLOOR
//
typedef enum
{
	// lower floor to lowest surrounding floor
	lowerFloorToLowest,

	// raise floor to next highest surrounding floor
	raiseFloorToNearestFast,

	// move the floor down instantly
	instantLower,

	moveFloorByFrontSector,
	instantMoveFloorByFrontSector,

	lowerFloorByLine,
	raiseFloorByLine,

	bounceFloor,
	bounceFloorCrush,
} floor_e;

typedef enum
{
	elevateUp,
	elevateDown,
	elevateCurrent,
	elevateContinuous,
	elevateBounce,
	elevateHighest,
	bridgeFall,
} elevator_e;

typedef struct
{
	thinker_t thinker;
	floor_e type;
	boolean crush;
	sector_t* sector;
	int direction;
	fixed_t texture;
	fixed_t floordestheight;
	fixed_t speed;
	fixed_t origspeed;
} floormove_t;

typedef struct
{
	thinker_t thinker;
	elevator_e type;
	sector_t* sector;
	sector_t* actionsector; // The sector the rover action is taking place in.
	int direction;
	fixed_t floordestheight;
	fixed_t ceilingdestheight;
	fixed_t speed;
	fixed_t origspeed;
	fixed_t low;
	fixed_t high;
	fixed_t distance;
	fixed_t floorwasheight; // Height the floor WAS at
	fixed_t ceilingwasheight; // Height the ceiling WAS at
	player_t* player; // Player who initiated the thinker (used for airbob)
	line_t* sourceline;
} elevator_t;

#define ELEVATORSPEED (FRACUNIT*4/NEWTICRATERATIO)
#define FLOORSPEED (FRACUNIT/NEWTICRATERATIO)

typedef enum
{
	ok,
	crushed,
	pastdest
} result_e;

result_e T_MovePlane(sector_t* sector, fixed_t speed, fixed_t dest, boolean crush,
	int floorOrCeiling, int direction);
int EV_DoFloor(line_t* line, floor_e floortype);
int EV_DoElevator(line_t* line, elevator_e elevtype, boolean customspeed);
void EV_CrumbleChain(sector_t* sec, ffloor_t* rover);
int EV_BounceSector(sector_t* sector, fixed_t momz, sector_t* blocksector,
	boolean player);

// Some other special 3dfloor types
int EV_StartCrumble(sector_t* sector, sector_t* roversector, fixed_t roverheight,
	boolean floating, player_t* player, fixed_t origalpha);

int EV_DoContinuousFall(sector_t* sec, fixed_t floordestheight, fixed_t speed);

int EV_StartNoReturnCrumble(sector_t* sector, sector_t* roversector, fixed_t roverheight,
	boolean floating, player_t* player);

int EV_AirBob(sector_t* sector, player_t* player, int amount, boolean reverse);

int EV_MarioBlock(sector_t* sector, sector_t* roversector, fixed_t topheight,
	struct line_s* masterline, mobj_t* puncher);

void T_MoveFloor(floormove_t* floor);

void T_MoveElevator(elevator_t* elevator);
void T_ContinuousFalling(elevator_t* elevator);
void T_BounceCheese(elevator_t* elevator);
void T_StartCrumble(elevator_t* elevator);
void T_AirBob(elevator_t* elevator);
void T_AirBobReverse(elevator_t* elevator);
void T_MarioBlock(elevator_t* elevator);
void T_SpikeSector(elevator_t* elevator);
void T_FloatSector(elevator_t* elevator);
void T_MarioBlockChecker(elevator_t* elevator);
void T_ThwompSector(elevator_t* elevator);
void T_CameraScanner(elevator_t* elevator);

typedef struct
{
	thinker_t thinker;	// Thinker for linedef executor delay
	line_t* line;		// Pointer to line that is waiting.
	mobj_t* caller;		// Pointer to calling mobj
	int timer;			// Delay timer
} executor_t;

void T_ExecutorDelay(executor_t* e);

// bits and shifts for generalized sector types
#define DAMAGE_MASK    0x60
#define DAMAGE_SHIFT   5
#define FRICTION_MASK  0x100
#define FRICTION_SHIFT 8
#define PUSH_MASK      0x200
#define PUSH_SHIFT     9

/** Generalized scroller.
  */
typedef struct
{
	thinker_t thinker;   ///< Thinker structure for scrolling.
	fixed_t dx, dy;      ///< (dx,dy) scroll speeds.
	int affectee;        ///< Number of affected sidedef or sector.
	int control;         ///< Control sector (-1 if none) used to control scrolling.
	fixed_t last_height; ///< Last known height of control sector.
	fixed_t vdx, vdy;    ///< Accumulated velocity if accelerative.
	int accel;           ///< Whether it's accelerative.
	/** Types of generalized scrollers.
	*/
	enum
	{
		sc_side,          ///< Scroll wall texture on a sidedef.
		sc_floor,         ///< Scroll floor.
		sc_ceiling,       ///< Scroll ceiling.
		sc_carry,         ///< Carry objects on floor.
		sc_carry_ceiling, ///< Carry objects on ceiling (for 3Dfloor conveyors).
	} type;
} scroll_t;

void T_Scroll(scroll_t* s);
void T_LaserFlash(laserthink_t* elevator);

/** Friction for ice/sludge effects.
  */
typedef struct
{
	thinker_t thinker;     ///< Thinker structure for friction.
	int friction;          ///< Friction value, 0xe800 = normal.
	int movefactor;        ///< Inertia factor when adding to momentum.
	int affectee;          ///< Number of affected sector.
	boolean roverfriction; ///< flag for whether friction originated from a FOF or not
} friction_t;

// Friction defines.
#define MORE_FRICTION_MOMENTUM 15000  ///< Mud factor based on momentum.
#define ORIG_FRICTION          0xE800 ///< Original value.
#define ORIG_FRICTION_FACTOR   2048   ///< Original value.

void T_Friction(friction_t* f);

typedef enum
{
	p_push,        ///< Point pusher or puller.
	p_wind,        ///< Wind.
	p_current,     ///< Current.
	p_upcurrent,   ///< Upwards current.
	p_downcurrent, ///< Downwards current.
	p_upwind,      ///< Upwards wind.
	p_downwind     ///< Downwards wind.
} pushertype_e;

// Model for pushers for push/pull effects
typedef struct
{
	thinker_t thinker;   ///< Thinker structure for push/pull effect.
	/** Types of push/pull effects.
	*/
	pushertype_e type;   ///< Type of push/pull effect.
	mobj_t* source;      ///< Point source if point pusher/puller.
	int x_mag;           ///< X strength.
	int y_mag;           ///< Y strength.
	int magnitude;       ///< Vector strength for point pusher/puller.
	int radius;          ///< Effective radius for point pusher/puller.
	int x, y, z;         ///< Point source if point pusher/puller.
	int affectee;     ///< Number of affected sector.
	boolean roverpusher; ///< flag for whether pusher originated from a FOF or not
	int referrer;     /// If roverpusher == true, then this will contain the sector # of the control sector where the effect was applied.
} pusher_t;

// Prototype functions for pushers
boolean PIT_PushThing(mobj_t* thing);
void T_Pusher(pusher_t* p);
mobj_t* P_GetPushThing(size_t s);

mobj_t* P_GetTeleportDestThing(size_t s);
mobj_t* P_GetStarpostThing(size_t s);
mobj_t* P_GetAltViewThing(size_t s); // This is getting ridiculous.

void P_CalcHeight(player_t* player);

sector_t* P_ThingOnSpecial3DFloor(mobj_t* mo);

#endif
