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
// MERCHANTABILITFY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief Moving object handling. Spawn functions

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "p_local.h"
#include "p_setup.h" // levelflats to test if mobj in water sector
#include "r_main.h"
#include "r_things.h"
#include "r_sky.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_random.h"
#include "info.h"

const double deg2rad = 0.017453293;

//Real Prototypes to A_*
void A_Boss1Chase(mobj_t* actor);
void A_Boss2Chase(mobj_t* actor);
void A_Boss2Pogo(mobj_t* actor);

// protos.
static CV_PossibleValue_t viewheight_cons_t[] = {{16, "MIN"}, {56, "MAX"}, {0, NULL}};

consvar_t cv_viewheight = {"viewheight", VIEWHEIGHTS, 0, viewheight_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_splats = {"splats", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static const fixed_t FloatBobOffsets[64] =
{
	0, 51389, 102283, 152192,
	200636, 247147, 291278, 332604,
	370727, 405280, 435929, 462380,
	484378, 501712, 514213, 521763,
	524287, 521763, 514213, 501712,
	484378, 462380, 435929, 405280,
	370727, 332604, 291278, 247147,
	200636, 152192, 102283, 51389,
	-1, -51390, -102284, -152193,
	-200637, -247148, -291279, -332605,
	-370728, -405281, -435930, -462381,
	-484380, -501713, -514215, -521764,
	-524288, -521764, -514214, -501713,
	-484379, -462381, -435930, -405280,
	-370728, -332605, -291279, -247148,
	-200637, -152193, -102284, -51389
};

//
// P_SetMobjState
// Returns true if the mobj is still present.
//
// Separate from P_SetMobjState because of the pw_flashing check
//
boolean P_SetPlayerMobjState(mobj_t* mobj, statenum_t state)
{
	state_t* st;

	// remember states seen, to detect cycles:
	static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
	statenum_t* seenstate = seenstate_tab; // pointer to table
	static int recursion; // detects recursion
	statenum_t i; // initial state
	boolean ret = true; // return value
	statenum_t tempstate[NUMSTATES]; // for use with recursion

#ifdef PARANOIA
	if(!mobj->player)
		I_Error("P_SetPlayerMobjState called with a non-player mobj!\n");
#endif

	// Catch state changes for Super Sonic
	if(mobj->player->powers[pw_super])
	{
		switch(state)
		{
			case S_PLAY_STND:
				P_SetPlayerMobjState(mobj, S_PLAY_SUPERSTAND);
				return true;
				break;
			case S_PLAY_RUN1:
				P_SetPlayerMobjState(mobj, S_PLAY_SUPERWALK1);
				return true;
				break;
			case S_PLAY_SPD1:
				P_SetPlayerMobjState(mobj, S_PLAY_SUPERFLY1);
				return true;
				break;
			case S_PLAY_TEETER1:
				P_SetPlayerMobjState(mobj, S_PLAY_SUPERTEETER);
				return true;
				break;
			case S_PLAY_CARRY:
				P_SetPlayerMobjState(mobj, S_PLAY_SUPERSTAND);
				return true;
				break;
			case S_PLAY_ATK1:
			case S_PLAY_PLG1:
			case S_PLAY_FALL1:
			case S_PLAY_GASP:
				return true;
				break;
			default:
				break;
		}
	}
	else if(mobj->state == &states[S_PLAY_PAIN] && mobj->player->powers[pw_flashing] == flashingtics)
		mobj->player->powers[pw_flashing] = flashingtics-1;

	if(recursion++) // if recursion detected,
		memset(seenstate = tempstate, 0, sizeof tempstate); // clear state table

	i = state;

	do
	{
		if(state == S_NULL)
		{
			mobj->state = (state_t*)S_NULL;
			P_RemoveMobj(mobj);
			ret = false;
			break;
		}

		st = &states[state];
		mobj->state = st;
		mobj->tics = st->tics;
		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set

		if(st->action.acp1)
			st->action.acp1(mobj);

		seenstate[state] = 1 + st->nextstate;

		state = st->nextstate;
	} while(!mobj->tics && !seenstate[state]);

	if(ret && !mobj->tics)
		CONS_Printf("Warning: State Cycle Detected");

	if(!--recursion)
		for(;(state = seenstate[i]) > S_NULL; i = state - 1)
			seenstate[i] = S_NULL; // erase memory of states

	return ret;
}


boolean P_SetMobjState(mobj_t* mobj, statenum_t state)
{
	state_t* st;

	// remember states seen, to detect cycles:
	static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
	statenum_t* seenstate = seenstate_tab; // pointer to table
	static int recursion; // detects recursion
	statenum_t i = state; // initial state
	boolean ret = true; // return value
	statenum_t tempstate[NUMSTATES]; // for use with recursion
#ifdef PARANOIA
	if(mobj->player)
		I_Error("P_SetMobjState used for player mobj. Use P_SetPlayerMobjState instead!\n(State called: %i)", state);
#endif

	if(recursion++) // if recursion detected,
		memset(seenstate = tempstate, 0, sizeof tempstate); // clear state table

	do
	{
		if(state == S_NULL)
		{
			mobj->state = (state_t*)S_NULL;
			P_RemoveMobj(mobj);
			ret = false;
			break;
		}

		st = &states[state];
		mobj->state = st;
		mobj->tics = st->tics;
		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set

		if(st->action.acp1)
			st->action.acp1(mobj);

		seenstate[state] = 1 + st->nextstate;

		state = st->nextstate;
	} while(!mobj->tics && !seenstate[state]);

	if(ret && !mobj->tics)
		CONS_Printf("Warning: State Cycle Detected");

	if(!--recursion)
		for(;(state = seenstate[i]) > S_NULL; i = state - 1)
			seenstate[i] = S_NULL; // erase memory of states

	return ret;
}

//----------------------------------------------------------------------------
//
// FUNC P_SetMobjStateNF
//
// Same as P_SetMobjState, but does not call the state function.
//
//----------------------------------------------------------------------------

boolean P_SetMobjStateNF(mobj_t* mobj, statenum_t state)
{
	state_t* st;

	if(state == S_NULL)
	{ // Remove mobj
		P_RemoveMobj(mobj);
		return false;
	}
	st = &states[state];
	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame;
	return true;
}

static boolean P_SetPrecipMobjState(precipmobj_t* mobj, statenum_t state)
{
	state_t* st;

	if(state == S_NULL)
	{ // Remove mobj
		P_RemovePrecipMobj(mobj);
		return false;
	}
	st = &states[state];
	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame;
	return true;
}

//
// P_ExplodeMissile
//
void P_ExplodeMissile(mobj_t* mo)
{
	mobj_t*	explodemo;
	mo->momx = mo->momy = mo->momz = 0;

	P_SetMobjState(mo, mobjinfo[mo->type].deathstate);

	if(mo->type == MT_DETON)
	{
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		explodemo->momx += (P_Random() % 32) * FRACUNIT/8;
		explodemo->momy += (P_Random() % 32) * FRACUNIT/8;
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		explodemo->momx += (P_Random() % 64) * FRACUNIT/8;
		explodemo->momy -= (P_Random() % 64) * FRACUNIT/8;
		S_StartSound(explodemo, sfx_dmpain);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		explodemo->momx -= (P_Random() % 128) * FRACUNIT/8;
		explodemo->momy += (P_Random() % 128) * FRACUNIT/8;
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		explodemo->momx -= (P_Random() % 96) * FRACUNIT/8;
		explodemo->momy -= (P_Random() % 96) * FRACUNIT/8;
		S_StartSound(explodemo, sfx_cybdth);
	}

	mo->tics -= P_Random()&3;

	if(mo->tics < 1)
		mo->tics = 1;

	mo->flags &= ~MF_MISSILE;

	if(mo->info->deathsound)
		S_StartSound(mo, mo->info->deathsound);
}

// P_InsideANonSolidFFloor
//
// Returns TRUE if mobj is inside a non-solid 3d floor.
static boolean P_InsideANonSolidFFloor(mobj_t* mobj, ffloor_t* rover)
{
	if((rover->flags & FF_SOLID))
		return false;

	if(mobj->z > *rover->topheight)
		return false;

	if(mobj->z + mobj->height < *rover->bottomheight)
		return false;

	return true;
}

#define STOPSPEED (0xffff/NEWTICRATERATIO)
#define FRICTION (0xe800/NEWTICRATERATIO) // 0.90625
#define FRICTION_LOW (0xf900/NEWTICRATERATIO)
#define FRICTION_FLY (0xeb00/NEWTICRATERATIO)

//
// P_XYFriction
//
// adds friction on the xy plane
//
static void P_XYFriction(mobj_t* mo, fixed_t oldx, fixed_t oldy)
{
	player_t* player = mo->player; // valid only if player avatar

	if(player)
	{
		if(player->rmomx > -STOPSPEED && player->rmomx < STOPSPEED
			&& player->rmomy > -STOPSPEED && player->rmomy < STOPSPEED
			&& (!player->cmd.forwardmove && !player->cmd.sidemove && !player->mfspinning))
		{
			// if in a walking frame, stop moving
			if((player && player->walking == 1) && (mo->type != MT_SPIRIT))
				P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			mo->momx = player->cmomx;
			mo->momy = player->cmomy;
		}
		else
		{
			if((oldx == mo->x) && (oldy == mo->y)) // didn't go anywhere
			{
				mo->momx = FixedMul(mo->momx, ORIG_FRICTION);
				mo->momy = FixedMul(mo->momy, ORIG_FRICTION);
			}
			else
			{
				mo->momx = FixedMul(mo->momx, mo->friction);
				mo->momy = FixedMul(mo->momy, mo->friction);
			}
			mo->friction = ORIG_FRICTION;
		}
	}
	else
	{
		if(mo->momx > -STOPSPEED && mo->momx < STOPSPEED
			&& mo->momy > -STOPSPEED && mo->momy < STOPSPEED)
		{
			mo->momx = 0;
			mo->momy = 0;
		}
		else
		{
			if((oldx == mo->x) && (oldy == mo->y)) // didn't go anywhere
			{
				mo->momx = FixedMul(mo->momx, ORIG_FRICTION);
				mo->momy = FixedMul(mo->momy, ORIG_FRICTION);
			}
			else
			{
				mo->momx = FixedMul(mo->momx, mo->friction);
				mo->momy = FixedMul(mo->momy, mo->friction);
			}
			mo->friction = ORIG_FRICTION;
		}
	}
}

//
// P_SceneryXYFriction
//
static inline void P_SceneryXYFriction(mobj_t* mo, fixed_t oldx, fixed_t oldy)
{
	if(mo->momx > -STOPSPEED && mo->momx < STOPSPEED &&
		mo->momy > -STOPSPEED && mo->momy < STOPSPEED)
	{
		mo->momx = 0;
		mo->momy = 0;
	}
	else
	{
		if((oldx == mo->x) && (oldy == mo->y)) // didn't go anywhere
		{
			mo->momx = FixedMul(mo->momx,ORIG_FRICTION);
			mo->momy = FixedMul(mo->momy,ORIG_FRICTION);
		}
		else
		{
			mo->momx = FixedMul(mo->momx,mo->friction);
			mo->momy = FixedMul(mo->momy,mo->friction);
		}
		mo->friction = ORIG_FRICTION;
	}
}

//
// P_XYMovement
//
void P_XYMovement(mobj_t* mo)
{
	fixed_t ptryx, ptryy;
	player_t* player;
	fixed_t xmove, ymove;
	fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls
	boolean moved;

	moved = true;

	if(mo->type == MT_FLINGRING && (mo->subsector->sector->special == 16
		|| mo->subsector->sector->special == 5)
		&& mo->z == mo->subsector->sector->floorheight)
	{
		mo->fuse = 1; // Remove flingrings if in death pit.
	}
	else if((mo->type == MT_REDFLAG || mo->type == MT_BLUEFLAG
		|| (mo->flags & MF_PUSHABLE))
		&& (mo->subsector->sector->special == 16
		|| mo->subsector->sector->special == 5
		|| mo->subsector->sector->special == 7
		|| mo->subsector->sector->special == 4
		|| mo->subsector->sector->special == 11)
		&& mo->z == mo->subsector->sector->floorheight)
	{
		// Remove CTF flag if in death pit
		mo->fuse = 1;
	}

	// if it's stopped
	if(!mo->momx && !mo->momy)
	{
		if(mo->flags2 & MF2_SKULLFLY)
		{
			// the skull slammed into something
			mo->flags2 &= ~MF2_SKULLFLY;
			mo->momx = mo->momy = mo->momz = 0;

			// set in 'search new direction' state?
			if(mo->type != MT_EGGMOBILE)
				P_SetMobjState(mo, mo->info->spawnstate);

			return;
		}
	}

	player = mo->player; //valid only if player avatar

	if(mo->momx > MAXMOVE)
		mo->momx = MAXMOVE;
	else if(mo->momx < -MAXMOVE)
		mo->momx = -MAXMOVE;

	if(mo->momy > MAXMOVE)
		mo->momy = MAXMOVE;
	else if(mo->momy < -MAXMOVE)
		mo->momy = -MAXMOVE;

	xmove = mo->momx;
	ymove = mo->momy;

	oldx = mo->x;
	oldy = mo->y;

	do
	{
		if(xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
		{
			ptryx = mo->x + xmove/2;
			ptryy = mo->y + ymove/2;
			xmove >>= 1;
			ymove >>= 1;
		}
		else
		{
			ptryx = mo->x + xmove;
			ptryy = mo->y + ymove;
			xmove = ymove = 0;
		}

		if(!P_TryMove(mo, ptryx, ptryy, true) && !tmsprung)
		{
			// blocked move

			if(mo->player)
				moved = false;

			if(mo->flags & MF_BOUNCE)
			{
				P_BounceMove(mo);
				xmove = ymove = 0;
			}
			else if((mo->player && !twodlevel) || (mo->flags & MF_SLIDEME)
				|| (mo->flags & MF_PUSHABLE))
			{ // try to slide along it
				P_SlideMove(mo);
				xmove = ymove = 0;
			}
			else if(mo->flags & MF_MISSILE)
			{
				// explode a missile
				if(ceilingline && ceilingline->backsector
					&& ceilingline->backsector->ceilingpic == skyflatnum
					&& ceilingline->frontsector
					&& ceilingline->frontsector->ceilingpic == skyflatnum
					&& mo->subsector->sector->ceilingheight == mo->ceilingz)
				{
					if(mo->z > ceilingline->backsector->ceilingheight) // demos
					{
						// Hack to prevent missiles exploding
						// against the sky.
						// Does not handle sky floors.
						// Check frontsector as well.

						P_SetMobjState(mo, S_DISS);
						//P_RemoveMobj(mo);
						return;
					}
				}

				// draw damage on wall
				//SPLAT TEST ----------------------------------------------------------
#ifdef WALLSPLATS
				if(blockingline && mo->type != MT_REDRING
					&& !(mo->flags2 & MF2_AUTOMATIC) && !(mo->flags2 & MF2_RAILRING)
					&& !(mo->flags2 & MF2_HOMING) && !(mo->flags2 & MF2_EXPLOSION))
					// set by last P_TryMove() that failed
				{
					divline_t divl;
					divline_t misl;
					fixed_t frac;

					P_MakeDivline(blockingline, &divl);
					misl.x = mo->x;
					misl.y = mo->y;
					misl.dx = mo->momx;
					misl.dy = mo->momy;
					frac = P_InterceptVector(&divl, &misl);
					R_AddWallSplat(blockingline, P_PointOnLineSide(mo->x,mo->y,blockingline),
						"A_DMG3", mo->z, frac, SPLATDRAWMODE_SHADE);
				}
#endif
				// --------------------------------------------------------- SPLAT TEST

				P_ExplodeMissile(mo);
			}
			else if(mo->type == MT_FIREBALL)
			{
				// explode a missile
				if(ceilingline &&
					ceilingline->backsector &&
					ceilingline->backsector->ceilingpic == skyflatnum &&
					ceilingline->frontsector &&
					ceilingline->frontsector->ceilingpic == skyflatnum &&
					mo->subsector->sector->ceilingheight == mo->ceilingz)
				{
					if(mo->z > ceilingline->backsector->ceilingheight) // demos
					{
						// Hack to prevent missiles exploding
						// against the sky.
						// Does not handle sky floors.
						// Check frontsector as well.

						P_SetMobjState(mo, S_DISS);
						return;
					}
				}

				S_StartSound(mo, sfx_tink);

				P_ExplodeMissile(mo);
			}
			else
				mo->momx = mo->momy = 0;
		}
		else if(mo->player)
			moved = true;

	} while(xmove || ymove);

	if(mo->player && !moved && mo->player->nightsmode && mo->target)
	{
		int radius;
		P_UnsetThingPosition(mo);
		mo->player->angle_pos = mo->player->old_angle_pos;
		mo->player->speed /= 5;
		mo->player->speed *= 4;
		player->flyangle += 180;
		player->flyangle %= 360;

		radius = mo->target->info->radius;

		mo->x = (fixed_t)(mo->target->x + cos(mo->player->old_angle_pos * deg2rad) * radius);
		mo->y = (fixed_t)(mo->target->y + sin(mo->player->old_angle_pos * deg2rad) * radius);

		mo->momx = mo->momy = 0;
		P_SetThingPosition(mo);
	}

	if(mo->type == MT_FIREBALL || mo->type == MT_SHELL)
		return;

	if((((mo->flags & MF_MISSILE) || (mo->flags2 & MF2_SKULLFLY)) || mo->type == MT_SNOWBALL)
		&& !mo->type == MT_DETON)
	{
		return; // no friction for missiles ever
	}

	if(mo->flags & MF_MISSILE)
		return;

	if(mo->player && mo->player->homing) // no friction for homing
		return;

	if(mo->z > mo->floorz && !(mo->flags2 & MF2_ONMOBJ))
		return; // no friction when airborne

	// spinning friction
	if(player)
	{
		if(player->mfspinning == 1 && (player->rmomx || player->rmomy) && !player->mfstartdash)
		{
			mo->momx = FixedMul(mo->momx, (fixed_t)(FRICTION*1.098f));
			mo->momy = FixedMul(mo->momy, (fixed_t)(FRICTION*1.098f));
			return;
		}
	}

	if(mo->z > mo->floorz && mo->type != MT_CRAWLACOMMANDER && mo->type != MT_EGGMOBILE && mo->type != MT_EGGMOBILE2)
		return; // no friction when airborne

	P_XYFriction(mo, oldx, oldy);
}

static void P_RingXYMovement(mobj_t* mo)
{
	fixed_t ptryx, ptryy, xmove, ymove;
	fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls

	if(mo->momx > MAXMOVE)
		mo->momx = MAXMOVE;
	else if(mo->momx < -MAXMOVE)
		mo->momx = -MAXMOVE;

	if(mo->momy > MAXMOVE)
		mo->momy = MAXMOVE;
	else if(mo->momy < -MAXMOVE)
		mo->momy = -MAXMOVE;

	xmove = mo->momx;
	ymove = mo->momy;

	oldx = mo->x;
	oldy = mo->y;

	do
	{
		if(xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
		{
			ptryx = mo->x + xmove/2;
			ptryy = mo->y + ymove/2;
			xmove >>= 1;
			ymove >>= 1;
		}
		else
		{
			ptryx = mo->x + xmove;
			ptryy = mo->y + ymove;
			xmove = ymove = 0;
		}

		if(!P_SceneryTryMove(mo, ptryx, ptryy))
			P_SlideMove(mo);
	} while(xmove || ymove);
}

static void P_SceneryXYMovement(mobj_t* mo)
{
	fixed_t ptryx, ptryy, xmove, ymove;
	fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls

	if(mo->momx > MAXMOVE)
		mo->momx = MAXMOVE;
	else if(mo->momx < -MAXMOVE)
		mo->momx = -MAXMOVE;

	if(mo->momy > MAXMOVE)
		mo->momy = MAXMOVE;
	else if(mo->momy < -MAXMOVE)
		mo->momy = -MAXMOVE;

	xmove = mo->momx;
	ymove = mo->momy;

	oldx = mo->x;
	oldy = mo->y;

	do
	{
		if(xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
		{
			ptryx = mo->x + xmove/2;
			ptryy = mo->y + ymove/2;
			xmove >>= 1;
			ymove >>= 1;
		}
		else
		{
			ptryx = mo->x + xmove;
			ptryy = mo->y + ymove;
			xmove = ymove = 0;
		}

		if(!P_SceneryTryMove(mo, ptryx, ptryy))
			mo->momx = mo->momy = 0; // blocked move

	} while(xmove || ymove);

	if(mo->z > mo->floorz && !(mo->flags2 & MF2_ONMOBJ))
		return; // no friction when airborne

	if(mo->z > mo->floorz)
		return; // no friction when airborne

	P_SceneryXYFriction(mo, oldx, oldy);
}

static void P_RingZMovement(mobj_t* mo)
{
	// Intercept the stupid 'fall through 3dfloors' bug
	if(mo->subsector->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = mo->z + mo->height;

		for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if((!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) || (rover->flags & FF_SWIMMABLE)))
				continue;

			if(rover->flags & FF_QUICKSAND)
			{
				if(mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					mo->floorz = mo->z;
					mo->floorff = NULL; /// \bug breaks moving quicksand?
					continue;
				}
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > mo->floorz && abs(delta1) < abs(delta2))
			{
				mo->floorz = *rover->topheight;
				mo->floorff = rover;
			}
			if(*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
				mo->ceilingff = rover;
			}
		}
	}

	// adjust height
	if(mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	if(!mo->momx && !mo->momy)
		return;

	// clip movement
	if(mo->z <= mo->floorz)
	{
		mo->z = mo->floorz;
		mo->momz = 0;
	}
	else if(mo->z + mo->height > mo->ceilingz)
	{
		mo->momz = 0;
		mo->z = mo->ceilingz - mo->height;
	}
}

//
// P_ZMovement
//
static void P_ZMovement(mobj_t* mo)
{
	fixed_t dist, delta;

	// Intercept the stupid 'fall through 3dfloors' bug
	if(mo->subsector->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2, thingtop = mo->z + mo->height;

		for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags && FF_EXISTS)
				|| (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND)
				|| (rover->flags & FF_SWIMMABLE)))
			{
				continue;
			}

			if(rover->flags & FF_QUICKSAND)
			{
				if(mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					mo->floorz = mo->z;
					mo->floorff = NULL; /// \bug breaks moving quicksand?
					continue;
				}
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > mo->floorz && abs(delta1) < abs(delta2))
			{
				mo->floorz = *rover->topheight;
				mo->floorff = rover;
			}
			if(*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
				mo->ceilingff = rover;
			}
		}
	}

	// adjust height
	if(mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	// skims don't bounce
	if(mo->type == MT_SKIM && mo->z > mo->watertop && mo->z - mo->momz <= mo->watertop)
	{
		mo->z = mo->watertop;
		mo->momz = 0;
		mo->flags |= MF_NOGRAVITY;
	}

	switch(mo->type)
	{
		case MT_GOOP:
			if(mo->z <= mo->floorz && mo->momz)
			{
				P_SetMobjState(mo, S_GOOP3);
				mo->momx = mo->momy = 0;
				if(mo->info->painsound) S_StartSound(mo, mo->info->painsound);
			}
			break;
		case MT_SMALLBUBBLE:
			if(mo->z <= mo->floorz) // Hit the floor, so POP!
			{
				int random;

				P_SetMobjState(mo, S_DISS);

				random = P_Random();

				if(mo->threshold == 42) // Don't make pop sound.
					break;

				if(random <= 51)
					S_StartSound(mo, sfx_bubbl1);
				else if(random <= 102)
					S_StartSound(mo, sfx_bubbl2);
				else if(random <= 153)
					S_StartSound(mo, sfx_bubbl3);
				else if(random <= 204)
					S_StartSound(mo, sfx_bubbl4);
				else
					S_StartSound(mo, sfx_bubbl5);
			}
			break;
		case MT_MEDIUMBUBBLE:
			if(mo->z <= mo->floorz) // Hit the floor, so split!
			{
				// split
				mobj_t* explodemo;

				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx += (P_Random() % 32) * FRACUNIT/8;
				explodemo->momy += (P_Random() % 32) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx += (P_Random() % 64) * FRACUNIT/8;
				explodemo->momy -= (P_Random() % 64) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx -= (P_Random() % 128) * FRACUNIT/8;
				explodemo->momy += (P_Random() % 128) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx -= (P_Random() % 96) * FRACUNIT/8;
				explodemo->momy -= (P_Random() % 96) * FRACUNIT/8;
			}
			break;
		case MT_RING: // Ignore still rings
		case MT_COIN:
		case MT_FLINGCOIN:
		case MT_FLINGRING:
		case MT_HOMINGRING:
		case MT_AUTOMATICRING:
		case MT_INFINITYRING:
		case MT_RAILRING:
		case MT_EXPLOSIONRING:
		case MT_NIGHTSWING:
			if(!(mo->momx || mo->momy || mo->momz))
				return;
			break;
		default:
			break;
	}

	if(mo->flags & MF_FLOAT && mo->target && mo->health && !(mo->type == MT_DETON ||
		mo->type == MT_JETTBOMBER || mo->type == MT_JETTGUNNER || mo->type == MT_CRAWLACOMMANDER
		|| mo->type == MT_EGGMOBILE2))
	{
		// float down towards target if too close
		if(!(mo->flags2 & MF2_SKULLFLY) && !(mo->flags & MF_INFLOAT))
		{
			dist = P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y);

			delta = (mo->target->z + (mo->height>>1)) - mo->z;

			if(delta < 0 && dist < -(delta*3))
				mo->z -= FLOATSPEED;
			else if(delta > 0 && dist < (delta*3))
				mo->z += FLOATSPEED;

			if(mo->type == MT_EGGMOBILE && mo->z < mo->floorz+33*FRACUNIT)
				mo->z = mo->floorz+33*FRACUNIT;
		}

	}

	// clip movement
	if(mo->z <= mo->floorz)
	{
		// hit the floor
		if(mo->flags & MF_MISSILE)
		{
			mo->z = mo->floorz;
			if(!(mo->flags & MF_NOCLIP))
			{
				P_ExplodeMissile(mo);
				return;
			}
		}
		else if(mo->type == MT_FIREBALL)
			mo->momz = 5*FRACUNIT;

		mo->z = mo->floorz;

		// Note (id):
		//  somebody left this after the setting momz to 0,
		//  kinda useless there.
		if(mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
			mo->momz = -mo->momz;

		// Mine explodes upon ground contact
		if((mo->type == MT_MINE) && (mo->z <= mo->floorz) && !(mo->state == &states[S_MINE_BOOM1]
			|| mo->state == &states[S_MINE_BOOM2] || mo->state == &states[S_MINE_BOOM3]
			|| mo->state == &states[S_MINE_BOOM4] || mo->state == &states[S_DISS]))
		{
			P_ExplodeMissile(mo);
		}

		if(mo->momz < 0) // falling
		{
			if(mo->type == MT_SEED)
			{
				byte random;
				random = P_Random();
				if(random < 64)
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER3);
				else if(random < 192)
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER1);
				else
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER2);

				P_SetMobjState(mo, S_DISS);
				return;
			}
			// set it once and not continuously
			if(tmfloorthing)
			{
				// Bouncing boxes
				if(tmfloorthing->z > tmfloorthing->floorz)
				{
					if((tmfloorthing->flags & MF_MONITOR) || (tmfloorthing->flags & MF_PUSHABLE))
						mo->momz = 4*FRACUNIT;
				}
			}
			if((mo->z <= mo->floorz) && (!(tmfloorthing) || (((tmfloorthing->flags & MF_PUSHABLE)
				|| (tmfloorthing->flags2 & MF2_STANDONME)) || tmfloorthing->type == MT_PLAYER
				|| tmfloorthing->type == MT_FLOORSPIKE)))
			{
				if(!tmfloorthing || mo->momz)
				{
					mo->eflags |= MF_JUSTHITFLOOR;
#ifdef ANNOYINGSTEP
					mo->eflags &= ~MF_STEPPEDUP;
#endif
				}
			}

			// Flingrings bounce
			if(mo->type == MT_FLINGRING
				|| mo->type == MT_FLINGCOIN
				|| mo->type == MT_HOMINGRING
				|| mo->type == MT_AUTOMATICRING
				|| mo->type == MT_INFINITYRING
				|| mo->type == MT_RAILRING
				|| mo->type == MT_EXPLOSIONRING)
			{
				if(maptol & TOL_NIGHTS)
					mo->momz = (fixed_t)(-mo->momz * 0.1f);
				else
					mo->momz = (fixed_t)(-mo->momz * 0.85f);
			}
			else if(!(tmfloorthing) || (((tmfloorthing->flags & MF_PUSHABLE) || (tmfloorthing->flags2 & MF2_STANDONME)) || tmfloorthing->type == MT_PLAYER || tmfloorthing->type == MT_FLOORSPIKE))
				mo->momz = 0;
		}

		if(mo->type == MT_STEAM)
			return;

		mo->z = mo->floorz;
	}
	else if(!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		fixed_t gravityadd = 0;
		boolean no3dfloorgrav;

		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MF_JUSTHITFLOOR;

		// Custom gravity
		no3dfloorgrav = true;
		if(mo->subsector->sector->ffloors) // Check for 3D floor gravity too
		{
			ffloor_t* rover;

			for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_EXISTS))
					continue;

				if(P_InsideANonSolidFFloor(mo, rover))
				{
					if(rover->master->frontsector->gravity)
					{
						gravityadd = -FixedMul(gravity,
							(FixedDiv(*rover->master->frontsector->gravity >> FRACBITS, 1000)));
						no3dfloorgrav = false;
						break;
					}
				}
			}
		}

		if(no3dfloorgrav)
		{
			if(mo->subsector->sector->gravity)
				gravityadd = -FixedMul(gravity,
					(FixedDiv(*mo->subsector->sector->gravity >> FRACBITS, 1000)));
			else
				gravityadd = -gravity;
		}

		// Less gravity underwater.
		if(mo->eflags & MF_UNDERWATER)
			gravityadd /= 3;

		if(!mo->momz) // mobj at stop, no floor, so feel the push of gravity!
			gravityadd <<= 1;

		if(mo->type == MT_CEILINGSPIKE)
		{
			gravityadd *= -1; // Reverse gravity for ceiling spikes
			if(mo->z + mo->height >= mo->ceilingz)
				gravityadd = 0;
		}

		mo->momz += gravityadd/NEWTICRATERATIO;
		if(mo->type == MT_SKIM && mo->z + mo->momz <= mo->watertop && mo->z >= mo->watertop)
		{
			mo->momz = 0;
			mo->flags |= MF_NOGRAVITY;
		}
	}

	if(mo->z + mo->height > mo->ceilingz)
	{
		if(mo->momz > 0)
		{
			// hit the ceiling
			mo->momz = 0;
		}

		mo->z = mo->ceilingz - mo->height;

		if(mo->flags2 & MF2_SKULLFLY)
		{ // the skull slammed into something
			mo->momz = -mo->momz;
		}

		if(mo->type == MT_FIREBALL)
		{
			// Don't explode on the sky!
			if(mo->subsector->sector->ceilingpic == skyflatnum &&
				mo->subsector->sector->ceilingheight == mo->ceilingz)
			{
				P_SetMobjState(mo, S_DISS);
				return;
			}

			S_StartSound(mo, sfx_tink);

			P_ExplodeMissile(mo);
			return;
		}
		else if((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
		{
			// Don't explode on the sky!
			if(mo->subsector->sector->ceilingpic == skyflatnum &&
				mo->subsector->sector->ceilingheight == mo->ceilingz)
			{
				P_SetMobjState(mo, S_DISS);
				return;
			}

			P_ExplodeMissile(mo);
			return;
		}
	}
}

static void P_PlayerZMovement(mobj_t* mo)
{
	fixed_t gravityadd = 0;

	if(!mo->player)
		return; // mobj was removed

	// Commented these out - was causing side-FOF bug
//	mo->ceilingz = mo->subsector->sector->ceilingheight; // recalculate disregarding FF_PLATFORMs
//	mo->ceilingff = NULL; /// \bug part of the reason of ceilingff is to fix FF_PLATFORMs.

	// Intercept the stupid 'fall through 3dfloors' bug
	if(mo->subsector->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = mo->z + mo->height;

		for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if((!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) && !(mo->player && !mo->player->nightsmode && (mo->player->skin == 1 || mo->player->powers[pw_super]) && (rover->flags & FF_SWIMMABLE) && !mo->player->mfspinning && mo->player->speed > 28 && /*mo->ceilingz - *rover->topheight >= mo->height && */mo->z < *rover->topheight + 30*FRACUNIT && mo->z > *rover->topheight - 30*FRACUNIT)))
				continue;

			if(rover->flags & FF_QUICKSAND)
			{
				if(mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					mo->floorz = mo->z;
					mo->floorff = NULL; /// \bug breaks moving quicksand?
				}
				continue; // This is so you can jump/spring up through quicksand from below.
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > mo->floorz && abs(delta1) < abs(delta2))
			{
				mo->floorz = *rover->topheight;
				mo->floorff = rover;
			}
			if(*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
				mo->ceilingff = rover;
			}
		}
	}

	// check for smooth step up
#ifdef CLIENTPREDICTION2
	if(mo->player && mo->z < mo->floorz && mo->type != MT_PLAYER)
#else
	if(mo->z < mo->floorz && mo->type != MT_SPIRIT)
#endif
	{
		mo->player->viewheight -= mo->floorz - mo->z;

		mo->player->deltaviewheight =
			((cv_viewheight.value<<FRACBITS) - mo->player->viewheight)>>3;
	}

	// adjust height
	if(mo->pmomz && mo->z != mo->floorz && !mo->player->mfjumped)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	// Have player fall through floor?
	if(mo->player->playerstate == PST_DEAD)
		goto playergravity;

	// clip movement
	if(mo->z <= mo->floorz)
	{
		if(mo->player && mo->player->nightsmode)
		{
			if(mo->player->flyangle < 90 || mo->player->flyangle >= 270)
				mo->player->flyangle += 90;
			else
				mo->player->flyangle -= 90;
			mo->z = mo->floorz;
			mo->player->speed /= 5;
			mo->player->speed *= 4;
			goto nightsdone;
		}
		// Get up if you fell.
		if(mo->state == &states[S_PLAY_PAIN] || mo->state == &states[S_PLAY_SUPERHIT])
			P_SetPlayerMobjState(mo, S_PLAY_STND);

		mo->z = mo->floorz;

		if(mo->momz < 0) // falling
		{
			if(mo->player && (mo->momz < -8*FRACUNIT))
			{
				// Squat down. Decrease viewheight for a moment after hitting the ground (hard),
				// and utter appropriate sound.
				mo->player->deltaviewheight = mo->momz>>3;
			}

			if((maptol & TOL_ADVENTURE) && mo->momz < -2*FRACUNIT && mo->info->activesound)
				S_StartSound(mo, mo->info->activesound);

			// set it once and not continuously
			if(tmfloorthing)
			{
				if((tmfloorthing->flags & MF_MONITOR) || (tmfloorthing->flags & MF_PUSHABLE)
					|| (tmfloorthing->flags2 & MF2_STANDONME))
				{
					if(mo->player)
					{
						if(!(mo->player->mfjumped))
						tmfloorthing = 0;
					}
				}
			}
			if((mo->z <= mo->floorz) && (!(tmfloorthing) || (((tmfloorthing->flags & MF_PUSHABLE)
				|| (tmfloorthing->flags2 & MF2_STANDONME)) || tmfloorthing->type == MT_PLAYER
				|| tmfloorthing->type == MT_FLOORSPIKE))) // Spin Attack
			{
				if((tmfloorthing && mo->momz) || !tmfloorthing)
				{
					mo->eflags |= MF_JUSTHITFLOOR; // Spin Attack
#ifdef ANNOYINGSTEP
					mo->eflags &= ~MF_STEPPEDUP;
#endif
				}

				if(mo->eflags & MF_JUSTHITFLOOR)
				{
					// Cut momentum in half when you hit the ground and
					// aren't pressing any controls.
					if(!(mo->player->cmd.forwardmove || mo->player->cmd.sidemove) && !mo->player->cmomx && !mo->player->cmomy && !mo->player->mfspinning)
					{
						mo->momx = mo->momx/2;
						mo->momy = mo->momy/2;
					}
				}

				if(mo->health)
				{
					if(!mo->player->mfspinning || !mo->player->usedown || mo->player->mfjumped)
					{
						const int runspeed = 28;

						if(mo->player->cmomx || mo->player->cmomy)
						{
							if(mo->player->speed > runspeed && !mo->player->running)
								P_SetPlayerMobjState(mo, S_PLAY_SPD1);
							else if((mo->player->rmomx > STOPSPEED
								|| mo->player->rmomy > STOPSPEED) && !mo->player->walking)
								P_SetPlayerMobjState(mo, S_PLAY_RUN1);
							else if((mo->player->rmomx < -STOPSPEED
								|| mo->player->rmomy < -STOPSPEED) && !mo->player->walking)
								P_SetPlayerMobjState(mo, S_PLAY_RUN1);
							else if((mo->player->rmomx < FRACUNIT
								&& mo->player->rmomx > -FRACUNIT && mo->player->rmomy < FRACUNIT && mo->player->rmomy > -FRACUNIT) && !(mo->player->walking || mo->player->running))
								P_SetPlayerMobjState(mo, S_PLAY_STND);
						}
						else
						{
							if(mo->player->speed > runspeed && !mo->player->running)
								P_SetPlayerMobjState(mo, S_PLAY_SPD1);
							else if((mo->momx || mo->momy) && !mo->player->walking)
								P_SetPlayerMobjState(mo, S_PLAY_RUN1);
							else if(!mo->momx && !mo->momy && !(mo->player->walking || mo->player->running))
								P_SetPlayerMobjState(mo, S_PLAY_STND);
						}
					}

					if(mo->player->mfjumped)
						mo->player->mfspinning = 0;
					else if(!mo->player->usedown)
						mo->player->mfspinning = 0;

					P_ResetScore(mo->player);
					mo->player->mfjumped = 0; // Spin Attack
					mo->player->thokked = false;
					mo->player->gliding = 0;
					mo->player->glidetime = 0;
					mo->player->climbing = 0;
				}
			}
			if(mo->player && !mo->player->mfspinning)
				mo->player->mfstartdash = 0; // dashing stuff

			if(!(tmfloorthing) || (((tmfloorthing->flags & MF_PUSHABLE) || (tmfloorthing->flags2 & MF2_STANDONME))|| tmfloorthing->type == MT_PLAYER || tmfloorthing->type == MT_FLOORSPIKE))
				mo->momz = 0;
		}
	}
	else if(!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		boolean no3dfloorgrav = true; // Custom gravity

		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MF_JUSTHITFLOOR;

		if(mo->subsector->sector->ffloors) // Check for 3D floor gravity too.
		{
			ffloor_t* rover;

			for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_EXISTS))
					continue;

				if(P_InsideANonSolidFFloor(mo, rover))
				{
					if(rover->master->frontsector->gravity)
					{
						gravityadd = -FixedMul(gravity,
							(FixedDiv(*rover->master->frontsector->gravity >> FRACBITS, 1000)));
						no3dfloorgrav = false;
						break;
					}
				}
			}
		}

		if(no3dfloorgrav)
		{
			if(mo->subsector->sector->gravity)
				gravityadd = -FixedMul(gravity,
					(FixedDiv(*mo->subsector->sector->gravity >> FRACBITS, 1000)));
			else
				gravityadd = -gravity;
		}

		// Less gravity underwater.
		if(mo->eflags & MF_UNDERWATER)
			gravityadd = gravityadd/3;

		if(!mo->momz) // mobj at stop, no floor, so feel the push of gravity!
			gravityadd <<= 1;

playergravity:
		if((mo->player->charability == 1) && ((mo->player->powers[pw_tailsfly]) || (mo->player->mo->state == &states[S_PLAY_SPC1]) || (mo->player->mo->state == &states[S_PLAY_SPC2]) || (mo->player->mo->state == &states[S_PLAY_SPC3]) || (mo->player->mo->state == &states[S_PLAY_SPC4])))
			gravityadd = gravityadd/3; // less gravity while flying
		if(mo->player->gliding)
			gravityadd = gravityadd/3; // less gravity while gliding
		if(mo->player->climbing)
			gravityadd = 0;
		if(mo->player->nightsmode)
			gravityadd = 0;
		if(mo->player->playerstate == PST_DEAD)
		{
			// Custom gravity
			if(mo->subsector->sector->gravity)
				gravityadd = -FixedMul(gravity,
					(FixedDiv(*mo->subsector->sector->gravity >> FRACBITS, 1000)));
			else
				gravityadd = -gravity;

			mo->momz += gravityadd/NEWTICRATERATIO;
			return;
		}

		mo->momz += gravityadd/NEWTICRATERATIO;
	}

nightsdone:

	if(mo->z + mo->height > mo->ceilingz)
	{
		if(mo->player && mo->player->nightsmode)
		{
			if(mo->player->flyangle < 90 || mo->player->flyangle >= 270)
				mo->player->flyangle -= 90;
			else
				mo->player->flyangle += 90;
			mo->player->flyangle %= 360;
			mo->z = mo->ceilingz - mo->height;
			mo->player->speed /= 5;
			mo->player->speed *= 4;
		}

		// Check for "Mario" blocks to hit and bounce them
		if(mo->momz > 0)
		{
			msecnode_t* node;

			if(CheckForMarioBlocks && mo->player) // Only let the player punch
			{
				// Search the touching sectors, from side-to-side...
				for(node = mo->touching_sectorlist; node; node = node->m_snext)
				{
					if(node->m_sector->ffloors)
					{
	 					ffloor_t* rover;

						for(rover = node->m_sector->ffloors; rover; rover = rover->next)
						{
							// Come on, it's time to go...
							if(rover->flags & FF_MARIO
								&& *rover->bottomheight == mo->ceilingz) // The player's head hit the bottom!
							{
								// DO THE MARIO!
								EV_MarioBlock(rover->master->frontsector, node->m_sector, *rover->topheight, rover->master, mo);
							}
						}
					}
				} // Ugly ugly billions of braces! Argh!
			}

			// hit the ceiling
			if(mariomode)
				S_StartSound(mo, sfx_mario1);

			mo->momz = 0;
		}

		mo->z = mo->ceilingz - mo->height;
	}
}

static void P_SceneryZMovement(mobj_t* mo)
{
	// Intercept the stupid 'fall through 3dfloors' bug
	if(mo->subsector->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = mo->z + mo->height;

		for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if((!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) || (rover->flags & FF_SWIMMABLE)))
				continue;

			if(rover->flags & FF_QUICKSAND)
			{
				if(mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					mo->floorz = mo->z;
					mo->floorff = NULL; /// \bug: breaks moving quicksand?
					continue;
				}
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > mo->floorz && abs(delta1) < abs(delta2))
			{
				mo->floorz = *rover->topheight;
				mo->floorff = rover;
			}
			if(*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
				mo->ceilingff = rover;
			}
		}
	}

	// adjust height
	if(mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	switch(mo->type)
	{
		case MT_SMALLBUBBLE:
			if(mo->z <= mo->floorz) // Hit the floor, so POP!
			{
				int random;

				P_SetMobjState(mo, S_DISS);

				if(mo->threshold == 42) // Don't make pop sound.
					break;

				random = P_Random();

				if(random <= 51)
					S_StartSound(mo, sfx_bubbl1);
				else if(random <= 102)
					S_StartSound(mo, sfx_bubbl2);
				else if(random <= 153)
					S_StartSound(mo, sfx_bubbl3);
				else if(random <= 204)
					S_StartSound(mo, sfx_bubbl4);
				else
					S_StartSound(mo, sfx_bubbl5);
			}
			break;
		case MT_MEDIUMBUBBLE:
			if(mo->z <= mo->floorz) // Hit the floor, so split!
			{
				// split
				mobj_t* explodemo;

				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx += (P_Random() % 32) * FRACUNIT/8;
				explodemo->momy += (P_Random() % 32) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx += (P_Random() % 64) * FRACUNIT/8;
				explodemo->momy -= (P_Random() % 64) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx -= (P_Random() % 128) * FRACUNIT/8;
				explodemo->momy += (P_Random() % 128) * FRACUNIT/8;
				explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
				explodemo->momx -= (P_Random() % 96) * FRACUNIT/8;
				explodemo->momy -= (P_Random() % 96) * FRACUNIT/8;
			}
			break;
		default:
			break;
	}

	// clip movement
	if(mo->z <= mo->floorz)
	{
		mo->z = mo->floorz;

		if(mo->momz < 0) // falling
		{
			if((!(tmfloorthing) || (((tmfloorthing->flags & MF_PUSHABLE)
				|| (tmfloorthing->flags2 & MF2_STANDONME)) || tmfloorthing->type == MT_PLAYER
				|| tmfloorthing->type == MT_FLOORSPIKE)))
			{
				if(!tmfloorthing || mo->momz)
				{
					mo->eflags |= MF_JUSTHITFLOOR; // Spin Attack
#ifdef ANNOYINGSTEP
					mo->eflags &= ~MF_STEPPEDUP;
#endif
				}
			}

			if(!tmfloorthing)
				mo->momz = 0;
		}

		mo->z = mo->floorz;
	}
	else if(!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		fixed_t gravityadd = 0;

		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MF_JUSTHITFLOOR;

		// Custom gravity
		if(mo->subsector->sector->gravity)
			gravityadd = -FixedMul(gravity,
				(FixedDiv(*mo->subsector->sector->gravity >> FRACBITS, 1000)));
		else
			gravityadd = -gravity;

		// Less gravity underwater.
		if(mo->eflags & MF_UNDERWATER)
			gravityadd = gravityadd/3;

		if(!mo->momz) // mobj at stop, no floor, so feel the push of gravity!
			gravityadd <<= 1;

		mo->momz += gravityadd/NEWTICRATERATIO;
	}

	if(mo->z + mo->height > mo->ceilingz)
	{
		if(mo->momz > 0)
		{
			// hit the ceiling
			mo->momz = 0;
		}

		mo->z = mo->ceilingz - mo->height;
	}
}

//
// P_MobjCheckWater
//
// Check for water, set stuff in mobj_t struct for movement code later.
// This is called either by P_MobjThinker() or P_PlayerThink()
void P_MobjCheckWater(mobj_t* mobj)
{
	sector_t* sector;
	fixed_t z;
	int oldeflags, wasinwater;

	if(mobj->type == MT_SPIRIT)
		return;

	wasinwater = mobj->eflags & MF_UNDERWATER; // important: not boolean!

	// Default if no water exists.
	mobj->watertop = mobj->waterbottom = mobj->subsector->sector->floorheight - 1000*FRACUNIT;

	// see if we are in water, and set some flags for later
	sector = mobj->subsector->sector;
	z = sector->floorheight;
	oldeflags = mobj->eflags;

	if(sector->ffloors) // 3D water
	{
		ffloor_t* rover;

		mobj->eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);

		for(rover = sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags && FF_EXISTS) || !(rover->flags & FF_SWIMMABLE)
				|| rover->flags & FF_SOLID)
				continue;
			if(*rover->topheight < mobj->z || *rover->bottomheight > (mobj->z + (mobj->info->height/2)))
				continue;

			if(mobj->z + mobj->info->height > *rover->topheight)
				mobj->eflags |= MF_TOUCHWATER;
			else
				mobj->eflags &= ~MF_TOUCHWATER;

			// Set the watertop and waterbottom
			mobj->watertop = *rover->topheight;
			mobj->waterbottom = *rover->bottomheight;

			if(mobj->z + (mobj->info->height/2) < *rover->topheight)
			{
				mobj->eflags |= MF_UNDERWATER;

				if(mobj->player)
				{
					if(!((mobj->player->powers[pw_super]) || (mobj->player->powers[pw_invulnerability])))
					{
						if(rover->master->frontsector->special != 7
							&& rover->master->frontsector->special != 519)
						{
							mobj->player->powers[pw_fireshield] = false;
						}

						mobj->player->powers[pw_ringshield] = false;
					}
					if(mobj->player->powers[pw_underwater] <= 0
						&& !(mobj->player->powers[pw_watershield])
						&& mobj->player->powers[pw_underwater] < underwatertics + 1)
					{
						mobj->player->powers[pw_underwater] = underwatertics + 1;
					}
				}
			}
			else
				mobj->eflags &= ~MF_UNDERWATER;
		}
	}
	else
		mobj->eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);

	if(leveltime < 1)
		wasinwater = mobj->eflags & MF_UNDERWATER;

	if((mobj->player || (mobj->flags & MF_PUSHABLE) ||
		(mobj->info->flags & MF_PUSHABLE && mobj->fuse))
		&& ((mobj->eflags & MF_UNDERWATER) != wasinwater))
	{
		int i, bubblecount;
		byte random[6];

		// Check to make sure you didn't just cross into a sector to jump out of
		// that has shallower water than the block you were originally in.
		if(mobj->watertop-mobj->floorz <= mobj->info->height/2)
			return;

		if(wasinwater && mobj->momz > 0)
		{
			mobj->momz = (fixed_t)((780*mobj->momz)/457); // Give the mobj a little out-of-water boost.
			if(mobj->player)
			{
				mobj->momz *= mobj->player->jumpfactor;
				mobj->momz /= 100;
			}
		}

		if(mobj->momz < 0)
		{
			if(mobj->z+(mobj->info->height/2)-mobj->momz >= mobj->watertop)
			{
				P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, MT_SPLISH); // Spawn a splash
			}
		}
		else if(mobj->momz > 0)
		{
			if(mobj->z+(mobj->info->height/2)-mobj->momz < mobj->watertop)
			{
				P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, MT_SPLISH); // Spawn a splash
			}
		}

		S_StartSound(mobj, sfx_splish); // And make a sound!
	
		bubblecount = abs(mobj->momz)>>FRACBITS;
		// Create tons of bubbles
		for(i = 0; i < bubblecount; i++)
		{
			// P_Random()s are called individually
			// to allow consistency across various
			// compilers, since the order of function
			// calls in C is not part of the ANSI
			// specification.
			random[0] = P_Random();
			random[1] = P_Random();
			random[2] = P_Random();
			random[3] = P_Random();
			random[4] = P_Random();
			random[5] = P_Random();

			if(random[0] < 32)
				P_SpawnMobj(mobj->x + (random[1]<<FRACBITS)/8 * (random[2]&1 ? 1 : -1),
					mobj->y + (random[3]<<FRACBITS)/8 * (random[4]&1 ? 1 : -1),
					mobj->z + (random[5]<<FRACBITS)/4, MT_MEDIUMBUBBLE)
					->momz = mobj->momz < 0 ? mobj->momz/16 : 0;
			else
				P_SpawnMobj(mobj->x + (random[1]<<FRACBITS)/8 * (random[2]&1 ? 1 : -1),
					mobj->y + (random[3]<<FRACBITS)/8 * (random[4]&1 ? 1 : -1),
					mobj->z + (random[5]<<FRACBITS)/4, MT_SMALLBUBBLE)
					->momz = mobj->momz < 0 ? mobj->momz/16 : 0;
		}
	}
}

static void P_SceneryCheckWater(mobj_t* mobj)
{
	sector_t* sector;
	fixed_t z;

	// Default if no water exists.
	mobj->watertop = mobj->waterbottom = mobj->subsector->sector->floorheight - 10000*FRACUNIT;

	// see if we are in water, and set some flags for later
	sector = mobj->subsector->sector;
	z = sector->floorheight;

	if(sector->ffloors)
	{
		ffloor_t* rover;

		mobj->eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);

		for(rover = sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
				continue;
			if(*rover->topheight <= mobj->z
				|| *rover->bottomheight > (mobj->z + (mobj->info->height >> 1)))
			{
				continue;
			}

			if(mobj->z + mobj->info->height > *rover->topheight)
				mobj->eflags |= MF_TOUCHWATER;
			else
				mobj->eflags &= ~MF_TOUCHWATER;

			// Set the watertop and waterbottom
			mobj->watertop = *rover->topheight;
			mobj->waterbottom = *rover->bottomheight;

			if(mobj->z + (mobj->info->height >> 1) < *rover->topheight)
				mobj->eflags |= MF_UNDERWATER;
			else
				mobj->eflags &= ~MF_UNDERWATER;
		}
	}
	else
		mobj->eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);
}

void P_DestroyRobots(void)
{
	// Search through all the thinkers for enemies.
	int count;
	mobj_t* mo;
	thinker_t* think;

	count = 0;
	for(think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if(think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t*)think;
		if(mo->health <= 0 || !(mo->flags & MF_ENEMY || mo->flags & MF_BOSS))
			continue; // not a valid enemy

		if(mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		// Found a target enemy
		P_DamageMobj(mo, players[consoleplayer].mo, players[consoleplayer].mo, 10000);
	}
}

//
// PlayerLandedOnThing
//
static void PlayerLandedOnThing(mobj_t* mo, mobj_t* onmobj)
{
	onmobj = NULL;
	mo->player->deltaviewheight = mo->momz>>3;
}

// P_CameraThinker
//
// Process the mobj-ish required functions of the camera
void P_CameraThinker(camera_t* thiscam)
{
	if(thiscam->momx || thiscam->momy)
	{
		fixed_t ptryx, ptryy, xmove, ymove;
		fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls

		if(thiscam->momx > MAXMOVE)
			thiscam->momx = MAXMOVE;
		else if(thiscam->momx < -MAXMOVE)
			thiscam->momx = -MAXMOVE;

		if(thiscam->momy > MAXMOVE)
			thiscam->momy = MAXMOVE;
		else if(thiscam->momy < -MAXMOVE)
			thiscam->momy = -MAXMOVE;

		xmove = thiscam->momx;
		ymove = thiscam->momy;

		oldx = thiscam->x;
		oldy = thiscam->y;

		do
		{
			if(xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
			{
				ptryx = thiscam->x + xmove/2;
				ptryy = thiscam->y + ymove/2;
				xmove >>= 1;
				ymove >>= 1;
			}
			else
			{
				ptryx = thiscam->x + xmove;
				ptryy = thiscam->y + ymove;
				xmove = ymove = 0;
			}

			if(!P_TryCameraMove(ptryx, ptryy, thiscam))
				P_SlideCameraMove(thiscam);

		} while(xmove || ymove);
	}

	thiscam->subsector = R_PointInSubsector(thiscam->x, thiscam->y);

	// always do the gravity bit now, that's simpler, BUT CheckPosition only if wasn't do before.
	if(thiscam->momz)
	{
		// Intercept the stupid 'fall through 3dfloors' bug
		if(thiscam->subsector->sector->ffloors)
		{
			ffloor_t* rover;
			fixed_t delta1, delta2;
			int thingtop = thiscam->z + thiscam->height;

			for(rover = thiscam->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) || !(rover->flags & FF_EXISTS))
					continue;

				delta1 = thiscam->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
				delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
				if(*rover->topheight > thiscam->floorz && abs(delta1) < abs(delta2))
					thiscam->floorz = *rover->topheight;
				if(*rover->bottomheight < thiscam->ceilingz && abs(delta1) >= abs(delta2))
					thiscam->ceilingz = *rover->bottomheight;
			}
		}

		// adjust height
		thiscam->z += thiscam->momz;

		// clip movement
		if(thiscam->z <= thiscam->floorz) // hit the floor
			thiscam->z = thiscam->floorz;

		if(thiscam->z + thiscam->height > thiscam->ceilingz)
		{
			if(thiscam->momz > 0)
			{
				// hit the ceiling
				thiscam->momz = 0;
			}

			thiscam->z = thiscam->ceilingz - thiscam->height;
		}
	}
}

//
// P_PlayerMobjThinker
//
static void P_PlayerMobjThinker(mobj_t* mobj)
{
	msecnode_t* node;

	// Make sure player shows dead
	if(mobj->health <= 0)
	{
		if(mobj->state == &states[S_DISS])
		{
			P_RemoveMobj(mobj);
			return;
		}

		P_SetPlayerMobjState(mobj, S_PLAY_DIE3);
		mobj->flags2 &= ~MF2_DONTDRAW;
		P_PlayerZMovement(mobj);
		return;
	}

	P_MobjCheckWater(mobj);

	// momentum movement
	mobj->eflags &= ~MF_JUSTSTEPPEDDOWN;

#ifdef CLIENTPREDICTION2
	// move player mobj (not the spirit) to spirit position (sent by ticcmd)
	if((mobj->type == MT_PLAYER) && (mobj->player) &&
		((mobj->player->cmd.angleturn&(TICCMD_XY|TICCMD_RECEIVED))==(TICCMD_XY|TICCMD_RECEIVED)) &&
		(mobj->player->playerstate == PST_LIVE))
	{
		int oldx = mobj->x, oldy = mobj->y;

		if(oldx != mobj->player->cmd.x || oldy != mobj->player->cmd.y )
		{
			mobj->eflags |= MF_NOZCHECKING;
			// cross special lines and pick up things
			if(!P_TryMove(mobj, mobj->player->cmd.x, mobj->player->cmd.y, true))
			{
				// P_TryMove fail mean cannot change mobj position to requestied position
				// the mobj is blocked by something
				if(mobj->player-players==consoleplayer)
				{
					// reset spirit possition
					CL_ResetSpiritPosition(mobj);

					CONS_Printf("\2MissPrediction\n");
				}
			}
			mobj->eflags &= ~MF_NOZCHECKING;
		}
		P_XYFriction(mobj, oldx, oldy);
	}
	else
#endif
	if(mobj->momx || mobj->momy)
	{
		P_XYMovement(mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if((mobj->thinker.function.acv == (actionf_v)(-1)))
			return; // mobj was removed
	}
	else
		P_TryMove(mobj, mobj->x, mobj->y, true);

	// Crumbling platforms
	for(node = mobj->touching_sectorlist; node; node = node->m_snext)
	{
		ffloor_t* rover;

		for(rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if(*rover->topheight == mobj->z && rover->flags & FF_CRUMBLE) // You nut.
			{
				if(rover->flags & FF_FLOATBOB)
				{
					if(rover->flags & FF_NORETURN)
						EV_StartNoReturnCrumble(rover->master->frontsector, node->m_sector, *rover->topheight, true, mobj->player);
					else
						EV_StartCrumble(rover->master->frontsector, node->m_sector, *rover->topheight, true, mobj->player, rover->alpha);
				}
				else
				{
					if(rover->flags & FF_NORETURN)
						EV_StartNoReturnCrumble(rover->master->frontsector, node->m_sector, *rover->topheight, false, mobj->player);
					else
						EV_StartCrumble(rover->master->frontsector, node->m_sector, *rover->topheight, false, mobj->player, rover->alpha);
				}
			}
		}
	}

	// Check for floating water platforms and bounce them
	if(CheckForFloatBob && mobj->momz < 0)
	{
		msecnode_t* node;
		fixed_t watertop;
		fixed_t waterbottom;
		boolean roverfound;

		watertop = waterbottom = 0;
		roverfound = false;

		for(node = mobj->touching_sectorlist; node; node = node->m_snext)
		{
			if(node->m_sector->ffloors)
			{
				ffloor_t* rover;
				// Get water boundaries first
				for(rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if(rover->flags & FF_SWIMMABLE) // Is there water?
					{
						watertop = *rover->topheight;
						waterbottom = *rover->bottomheight;
						roverfound = true;
						break;
					}
				}

				// Support for bobbing platforms in "old" water
				if(!roverfound && node->m_sector->heightsec > -1 && node->m_sector->altheightsec == 1)
				{
					// Set the watertop and waterbottom
					watertop = sectors[node->m_sector->heightsec].floorheight;
					waterbottom = node->m_sector->floorheight;
				}
			}
		}
		if(watertop)
		{
			for(node = mobj->touching_sectorlist; node; node = node->m_snext)
			{
				if(node->m_sector->ffloors)
				{
					ffloor_t* rover;
					for(rover = node->m_sector->ffloors; rover; rover = rover->next)
					{
						if(rover->flags & FF_FLOATBOB
							&& *rover->topheight <= mobj->z+abs(mobj->momz)
							&& *rover->topheight >= mobj->z-abs(mobj->momz)) // The player is landing on the cheese!
						{
							// Initiate a 'bouncy' elevator function
							// which slowly diminishes.
							EV_BounceSector(rover->master->frontsector, mobj->momz, node->m_sector, true);
						}
					}
				}
			}
		} // Ugly ugly billions of braces! Argh!
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if(!(mobj->eflags & MF_ONGROUND) || mobj->z != mobj->floorz || mobj->momz)
	{
		mobj_t* onmo;
		onmo = P_CheckOnmobj(mobj);
		if(!onmo)
		{
			P_PlayerZMovement(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
			if(mobj->flags & MF2_ONMOBJ )
				mobj->flags2 &= ~MF2_ONMOBJ;
		}
		else
		{
			if(mobj->momz < -8*FRACUNIT)
				PlayerLandedOnThing(mobj, onmo);
			if(onmo->z + onmo->height - mobj->z <= 24*FRACUNIT)
			{
				mobj->player->viewheight -= onmo->z+onmo->height
					-mobj->z;
				mobj->player->deltaviewheight =
					(VIEWHEIGHT-mobj->player->viewheight)>>3;
				mobj->z = onmo->z+onmo->height;
				mobj->flags2 |= MF2_ONMOBJ;
				mobj->momz = 0;
			}
			else // hit the bottom of the blocking mobj
				mobj->momz = 0;
		}

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if(mobj->thinker.function.acv == (actionf_v)(-1))
			return; // mobj was removed
	}
	else
		mobj->eflags &= ~MF_JUSTHITFLOOR;

	// cycle through states,
	// calling action functions at transitions
	if(mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if(!mobj->tics)
			if(!P_SetPlayerMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

static void CalculatePrecipFloor(precipmobj_t* mobj)
{
	// recalculate floorz each time
	mobj->floorz = mobj->subsector->sector->floorheight;
	if(mobj->subsector->sector->ffloors)
	{
		ffloor_t* rover;

		for(rover = mobj->subsector->sector->ffloors; rover; rover = rover->next)
		{
			// If it exists, it'll get rained on.
			if(!(rover->flags & FF_EXISTS))
				continue;

			if(*rover->topheight > mobj->floorz)
				mobj->floorz = *rover->topheight;
		}
	}
}

void P_RecalcPrecipInSector(sector_t* sector)
{
	/// \todo Why doesn't this work?!
/*	precipmobj_t* precipthing;

	for(precipthing = sector->preciplist; precipthing; precipthing = precipthing->snext)
	{
		CalculatePrecipFloor(precipthing);
	}*/
	sector = NULL; // warning C4100: 'sector' : unreferenced formal parameter
}

void P_SnowThinker(precipmobj_t* mobj)
{
	// adjust height
	mobj->z += mobj->momz;

	if(mobj->z <= mobj->floorz)
		mobj->z = mobj->subsector->sector->ceilingheight;

	return;
}

void P_RainThinker(precipmobj_t* mobj)
{
	// adjust height
	mobj->z += mobj->momz;

	if(mobj->state != &states[S_RAIN1])
	{
		// cycle through states,
		// calling action functions at transitions
		if(mobj->tics != -1)
		{
			mobj->tics--;

			// you can cycle through multiple states in a tic
			if(!mobj->tics)
				if(!P_SetPrecipMobjState(mobj, mobj->state->nextstate))
					return; // freed itself
		}

		if(mobj->state == &states[S_RAINRETURN])
		{
			mobj->z = mobj->subsector->sector->ceilingheight;
			mobj->momz = mobjinfo[MT_RAIN].speed;
			P_SetPrecipMobjState(mobj, S_RAIN1);
		}
	}
	else if(mobj->z <= mobj->floorz && mobj->momz)
	{
		// no splashes on sky or bottomless pits
		if(mobj->z <= mobj->subsector->sector->floorheight
			&& (mobj->subsector->sector->special == 5 || mobj->subsector->sector->special == 16
			|| mobj->subsector->sector->floorpic == skyflatnum))
			mobj->z = mobj->subsector->sector->ceilingheight;
		else
		{
			mobj->momz = 0;
			mobj->z = mobj->floorz;
			P_SetPrecipMobjState(mobj, S_SPLASH1);
		}
	}

	return;
}

static void P_RingThinker(mobj_t* mobj)
{
	if(mobj->momx || mobj->momy)
	{
		P_RingXYMovement(mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if((mobj->thinker.function.acv == (actionf_v)(-1)))
			return; // mobj was removed
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if(/*!(mobj->eflags & MF_ONGROUND) || */(mobj->z != mobj->floorz) || mobj->momz)
	{
		P_RingZMovement(mobj);
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if(mobj->thinker.function.acv == (actionf_v)(-1))
			return; // mobj was removed
	}

	// cycle through states, calling action functions at transitions
	if(mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if(!mobj->tics)
			if(!P_SetMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

//
// P_Look4Players
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
static boolean P_Look4Players(mobj_t* actor, boolean allaround)
{
	int stop, c = 0;
	player_t* player;
	sector_t* sector;
	angle_t an;
	fixed_t dist;

	sector = actor->subsector->sector;

	// first time init, this allow minimum lastlook changes
	if(actor->lastlook < 0)
		actor->lastlook = P_Random() % MAXPLAYERS;

	stop = (actor->lastlook-1) & PLAYERSMASK;

	for(;; actor->lastlook = (actor->lastlook+1) & PLAYERSMASK)
	{
		// done looking
		if(actor->lastlook == stop)
			return false;

		if(!playeringame[actor->lastlook])
			continue;

		if(c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if(player->health <= 0)
			continue; // dead

		if(!player->mo)
			continue;

		if(!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if(!allaround)
		{
			an = R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y) - actor->angle;

			if(an > ANG90 && an < ANG270)
			{
				dist = P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y);
				// if real close, react anyway
				if(dist > MELEERANGE)
					continue; // behind back
			}
		}

		actor->target = player->mo;
		return true;
	}

	//return false;
}

// Finds the player no matter what they're hiding behind (even lead!)
boolean P_SupermanLook4Players(mobj_t* actor)
{
	int c, stop = 0;
	player_t* playersinthegame[MAXPLAYERS];

	for(c = 0; c < MAXPLAYERS; c++)
	{
		if(playeringame[c])
		{
			if(players[c].health <= 0)
				continue; // dead

			if(!players[c].mo)
				continue;

			playersinthegame[stop] = &players[c];
			stop++;
		}
	}

	if(!stop)
		return false;

	actor->target = playersinthegame[P_Random()%stop]->mo;
	return true;
}

// AI for the first boss.
static void P_Boss1Thinker(mobj_t* mobj)
{
	if(mobj->state->nextstate == mobj->info->spawnstate && mobj->tics == 1)
	{
		mobj->flags2 &= ~MF2_FRET;
		mobj->flags &= ~MF_TRANSLATION;
	}

	if(!mobj->tracer)
	{
		fixed_t jetx, jety;
		mobj_t* filler;

		jetx = mobj->x + P_ReturnThrustX(mobj, mobj->angle, -56*FRACUNIT);
		jety = mobj->y + P_ReturnThrustY(mobj, mobj->angle, -56*FRACUNIT);

		// Spawn the jets here
		filler = P_SpawnMobj(jetx, jety, mobj->z + 8*FRACUNIT, MT_JETFUME1);
		filler->target = mobj;
		filler->fuse = 56;

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(mobj, mobj->angle-ANG90, 24*FRACUNIT), jety + P_ReturnThrustY(mobj, mobj->angle-ANG90, 24*FRACUNIT), mobj->z + 32*FRACUNIT, MT_JETFUME1);
		filler->target = mobj;
		filler->fuse = 57;

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(mobj, mobj->angle+ANG90, 24*FRACUNIT), jety + P_ReturnThrustY(mobj, mobj->angle+ANG90, 24*FRACUNIT), mobj->z + 32*FRACUNIT, MT_JETFUME1);
		filler->target = mobj;
		filler->fuse = 58;

		mobj->tracer = filler;
	}

	if(!mobj->target || !(mobj->target->flags & MF_SHOOTABLE))
	{
		if(mobj->health <= 0)
		{
			// look for a new target
			if(P_Look4Players(mobj, true) && mobj->info->mass) // Bid farewell!
				S_StartSound(mobj, mobj->info->mass);
			return;
		}

		// look for a new target
		if(P_Look4Players(mobj, true) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if(mobj->state == &states[mobj->info->spawnstate])
		A_Boss1Chase(mobj);

	if(mobj->state == &states[mobj->info->meleestate]
		|| (mobj->state == &states[mobj->info->missilestate]
		&& mobj->health > mobj->info->damage))
	{
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
	}
}

// AI for the second boss.
// No, it does NOT convert "Boss" to a "Thinker". =P
static void P_Boss2Thinker(mobj_t* mobj)
{
	if(mobj->movecount)
		mobj->movecount--;

	if(!(mobj->movecount))
	{
		mobj->flags2 &= ~MF2_FRET;
		mobj->flags &= ~MF_TRANSLATION;
	}

	if(!mobj->tracer && gametype != GT_CHAOS)
	{
		mobj_t* filler;

		// Spawn the jets here
		filler = P_SpawnMobj(mobj->x, mobj->y, mobj->z - 15*FRACUNIT, MT_JETFUME2);
		filler->target = mobj;

		mobj->tracer = filler;
	}

	if(!mobj->target || !(mobj->target->flags & MF_SHOOTABLE))
	{
		if(mobj->health <= 0)
		{
			// look for a new target
			if(P_Look4Players(mobj, true) && mobj->info->mass) // Bid farewell!
				S_StartSound(mobj, mobj->info->mass);
			return;
		}

		// look for a new target
		if(P_Look4Players(mobj, true) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if(gametype == GT_CHAOS && (mobj->state == &states[S_EGGMOBILE2_POGO1]
		|| mobj->state == &states[S_EGGMOBILE2_POGO2]
		|| mobj->state == &states[S_EGGMOBILE2_POGO3]
		|| mobj->state == &states[S_EGGMOBILE2_POGO4]
		|| mobj->state == &states[S_EGGMOBILE2_STND])) // Chaos mode, he pogos only
	{
		mobj->flags &= ~MF_NOGRAVITY;
		A_Boss2Pogo(mobj);
	}
	else if(gametype != GT_CHAOS)
	{
		if(mobj->state == &states[mobj->info->spawnstate] && mobj->health > mobj->info->damage)
			A_Boss2Chase(mobj);
		else if(mobj->state == &states[mobj->info->raisestate]
			|| mobj->state == &states[S_EGGMOBILE2_POGO2]
			|| mobj->state == &states[S_EGGMOBILE2_POGO3]
			|| mobj->state == &states[S_EGGMOBILE2_POGO4]
			|| mobj->state == &states[mobj->info->spawnstate])
		{
			mobj->flags &= ~MF_NOGRAVITY;
			A_Boss2Pogo(mobj);
		}
	}
}

// Fun function stuff to make NiGHTS hoops!
// Thanks a TON, Hurdler!
typedef float TVector[4];
typedef float TMatrix[4][4];

static TVector* VectorMatrixMultiply(TVector v, TMatrix m)
{
	static TVector ret;

	ret[0] = v[0]*m[0][0] + v[1]*m[1][0] + v[2]*m[2][0] + v[3]*m[3][0];
	ret[1] = v[0]*m[0][1] + v[1]*m[1][1] + v[2]*m[2][1] + v[3]*m[3][1];
	ret[2] = v[0]*m[0][2] + v[1]*m[1][2] + v[2]*m[2][2] + v[3]*m[3][2];
	ret[3] = v[0]*m[0][3] + v[1]*m[1][3] + v[2]*m[2][3] + v[3]*m[3][3];

	return &ret;
}

//ok, now, here is how to compute the transformation regarding the tilt:
static TMatrix* RotateXMatrix(float rad)
{
	static TMatrix ret;
	float cosrad, sinrad;
	cosrad = (float)cos(rad);
	sinrad = (float)sin(rad);

	ret[0][0] = 1; ret[0][1] = 0;         ret[0][2] = 0;        ret[0][3] = 0;
	ret[1][0] = 0; ret[1][1] = cosrad;    ret[1][2] = sinrad;   ret[1][3] = 0;
	ret[2][0] = 0; ret[2][1] = -sinrad;   ret[2][2] = cosrad;   ret[2][3] = 0;
	ret[3][0] = 0; ret[3][1] = 0;         ret[3][2] = 0;        ret[3][3] = 1;

	return &ret;
}

#if 0
static TMatrix* RotateYMatrix(float rad)
{
	static TMatrix ret;
	float cosrad, sinrad;
	cosrad = (float)cos(rad);
	sinrad = (float)sin(rad);

	ret[0][0] = cosrad;   ret[0][1] = 0; ret[0][2] = -sinrad;   ret[0][3] = 0;
	ret[1][0] = 0;        ret[1][1] = 1; ret[1][2] = 0;         ret[1][3] = 0;
	ret[2][0] = sinrad;   ret[2][1] = 0; ret[2][2] = cosrad;    ret[2][3] = 0;
	ret[3][0] = 0;        ret[3][1] = 0; ret[3][2] = 0;         ret[3][3] = 1;

	return &ret;
}
#endif

static TMatrix* RotateZMatrix(float rad)
{
	static TMatrix ret;
	float cosrad, sinrad;
	cosrad = (float)cos(rad);
	sinrad = (float)sin(rad);

	ret[0][0] = cosrad;    ret[0][1] = sinrad; ret[0][2] = 0; ret[0][3] = 0;
	ret[1][0] = -sinrad;   ret[1][1] = cosrad;   ret[1][2] = 0; ret[1][3] = 0;
	ret[2][0] = 0;         ret[2][1] = 0;        ret[2][2] = 1; ret[2][3] = 0;
	ret[3][0] = 0;         ret[3][1] = 0;        ret[3][2] = 0; ret[3][3] = 1;

	return &ret;
}

static fixed_t double2fixed(double t)
{
	double fl = floor(t);
	return ((int)fl << 16) | (int)((t-fl)*65536.0);
}

//
// P_GetClosestAxis
//
// Finds the CLOSEST axis to the source mobj
mobj_t* P_GetClosestAxis(mobj_t* source)
{
	thinker_t* th;
	mobj_t* mo2;
	int first = 0;
	mobj_t* closestaxis = NULL;

	// scan the thinkers to find the closest axis point
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t*)th;

		if(first == 0)
		{
			if(mo2->type == MT_AXIS1 || mo2->type == MT_AXIS2 || mo2->type == MT_AXIS3
				|| mo2->type == MT_AXIS1A || mo2->type == MT_AXIS2A || mo2->type == MT_AXIS3A)
			{
				closestaxis = mo2;
				first++;
			}
		}
		else
		{
			if(mo2->type == MT_AXIS1 || mo2->type == MT_AXIS2 || mo2->type == MT_AXIS3
				|| mo2->type == MT_AXIS1A || mo2->type == MT_AXIS2A || mo2->type == MT_AXIS3A)
			{
				fixed_t dist1, dist2;

				dist1 = R_PointToDist2(source->x, source->y, mo2->x, mo2->y)-mo2->info->radius;
				dist2 = R_PointToDist2(source->x, source->y, closestaxis->x, closestaxis->y)-closestaxis->info->radius;

				if(dist1 < dist2)
					closestaxis = mo2;
			}
		}
	}

	if(closestaxis == NULL)
		CONS_Printf("ERROR: No axis points found!\n");

	return closestaxis;
}

static void P_GimmeAxisXYPos(mobj_t* closestaxis, degenmobj_t* mobj)
{
	double angle_pos;

	angle_pos = atan2((double)(mobj->y - closestaxis->y), (double)(mobj->x - closestaxis->x))/deg2rad;

	if(angle_pos < 0.0)
	{
		angle_pos += 360.0;
	}
	else if(angle_pos >= 360.0)
	{
		angle_pos -= 360.0;
	}

	mobj->x = (fixed_t)(closestaxis->x + cos(angle_pos * deg2rad) * closestaxis->info->radius);
	mobj->y = (fixed_t)(closestaxis->y + sin(angle_pos * deg2rad) * closestaxis->info->radius);
}

static void P_MoveHoop(mobj_t* mobj)
{
	TVector v;
	TVector* res;
	fixed_t finalx, finaly, finalz;
	fixed_t mthingx, mthingy, mthingz;

	mthingx = mobj->target->x;
	mthingy = mobj->target->y;
	mthingz = mobj->target->z+mobj->target->height/2;

	// Make the sprite travel towards the center of the hoop
	v[0] = (float)(cos(mobj->movedir*11.25*deg2rad) * (mobj->fuse * 8));
	v[1] = 0;
	v[2] = (float)(sin(mobj->movedir*11.25*deg2rad) * (mobj->fuse * 8));
	v[3] = 1;

	res = VectorMatrixMultiply(v, *RotateXMatrix((float)(mobj->target->movedir*deg2rad)));
	memcpy(&v, res, sizeof(v));
	res = VectorMatrixMultiply(v, *RotateZMatrix((float)(mobj->target->movecount*deg2rad)));
	memcpy(&v, res, sizeof(v));

	finalx = mthingx + double2fixed(v[0]);
	finaly = mthingy + double2fixed(v[1]);
	finalz = mthingz + double2fixed(v[2]);

	P_UnsetThingPosition(mobj);
	mobj->x = finalx;
	mobj->y = finaly;
	P_SetThingPosition(mobj);
	mobj->z = finalz - mobj->height/2;
}

void P_SpawnHoopOfSomething(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, int number, mobjtype_t type, int rotangle)
{
	mobj_t* mobj;
	int i;
	TVector v;
	TVector* res;
	fixed_t finalx, finaly, finalz;
	degenmobj_t hoopcenter;
	mobj_t* axis;
	short closestangle;
	degenmobj_t xypos;
	double degrees;

	hoopcenter.x = x;
	hoopcenter.y = y;
	hoopcenter.z = z;

	axis = P_GetClosestAxis((mobj_t*)&hoopcenter);

	if(!axis)
	{
		CONS_Printf("You forgot to put axis points in the map!\n");
		return;
	}

	xypos.x = x;
	xypos.y = y;

	P_GimmeAxisXYPos(axis, &xypos);

	x = xypos.x;
	y = xypos.y;

	hoopcenter.z = z - mobjinfo[type].height/2;

	hoopcenter.x = x;
	hoopcenter.y = y;

	closestangle = (short)(R_PointToAngle2(x, y, axis->x, axis->y)/ANGLE_1);

	degrees = 360.0/number;

	radius >>= FRACBITS;

	// Create the hoop!
	for(i = 0; i < number; i++)
	{
		v[0] = (float)(cos(i*(degrees)*deg2rad) * radius);
		v[1] = 0;
		v[2] = (float)(sin(i*(degrees)*deg2rad) * radius);
		v[3] = 1;

		res = VectorMatrixMultiply(v, *RotateXMatrix((float)(rotangle*deg2rad)));
		memcpy(&v, res, sizeof(v));
		res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
		memcpy(&v, res, sizeof(v));

		finalx = x + double2fixed(v[0]);
		finaly = y + double2fixed(v[1]);
		finalz = z + double2fixed(v[2]);

		mobj = P_SpawnMobj(finalx, finaly, finalz, type);
		mobj->z -= mobj->height/2;
	}
}
void P_SpawnParaloop(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, int number, mobjtype_t type, int rotangle)
{
	mobj_t* mobj;
	int i;
	TVector v;
	TVector* res;
	fixed_t finalx, finaly, finalz, dist;
	degenmobj_t hoopcenter;
	mobj_t* axis;
	short closestangle;
	degenmobj_t xypos;
	double degrees;

	hoopcenter.x = x;
	hoopcenter.y = y;
	hoopcenter.z = z;

	axis = P_GetClosestAxis((mobj_t*)&hoopcenter);

	if(!axis)
	{
		CONS_Printf("You forgot to put axis points in the map!\n");
		return;
	}

	xypos.x = x;
	xypos.y = y;

	P_GimmeAxisXYPos(axis, &xypos);

	x = xypos.x;
	y = xypos.y;

	hoopcenter.z = z - mobjinfo[type].height/2;

	hoopcenter.x = x;
	hoopcenter.y = y;

	closestangle = (short)(R_PointToAngle2(x, y, axis->x, axis->y)/ANGLE_1);

	degrees = 360.0/number;

	radius >>= FRACBITS;
	radius = (fixed_t)(0.8f*radius);

	// Create the hoop!
	for(i = 0; i < number; i++)
	{
		v[0] = (float)(cos(i*(degrees)*deg2rad) * radius);
		v[1] = 0;
		v[2] = (float)(sin(i*(degrees)*deg2rad) * radius);
		v[3] = 1;

		res = VectorMatrixMultiply(v, *RotateXMatrix((float)(rotangle*deg2rad)));
		memcpy(&v, res, sizeof(v));
		res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
		memcpy(&v, res, sizeof(v));

		finalx = x + double2fixed(v[0]);
		finaly = y + double2fixed(v[1]);
		finalz = z + double2fixed(v[2]);

		mobj = P_SpawnMobj(finalx, finaly, finalz, type);
		mobj->z -= mobj->height/2;

		// change angle
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, x, y);

		// change slope
		dist = P_AproxDistance(P_AproxDistance(x - mobj->x, y - mobj->y), z - mobj->z);

		if(dist < 1)
			dist = 1;

		mobj->momx = FixedMul(FixedDiv(x - mobj->x, dist), 5*FRACUNIT);
		mobj->momy = FixedMul(FixedDiv(y - mobj->y, dist), 5*FRACUNIT);
		mobj->momz = FixedMul(FixedDiv(z - mobj->z, dist), 5*FRACUNIT);
		mobj->fuse = radius/4 + 1;

		if(mobj->fuse <= 1)
			mobj->fuse = 2;

		mobj->flags |= MF_NOCLIPTHING;
		mobj->flags &= ~MF_SPECIAL;

		if(mobj->fuse > 7)
			mobj->tics = mobj->fuse - 7;
		else
			mobj->tics = 1;
	}
}

// Returns true if no boss with health is in the level.
// Used for Chaos mode
static boolean P_BossDoesntExist(void)
{
	thinker_t* th;
	mobj_t* mo2;

	// scan the thinkers
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t*)th;

		if(mo2->flags & MF_BOSS && mo2->health)
			return false;
	}

	// No boss found!
	return true;
}

void P_Attract(mobj_t* source, mobj_t* enemy, boolean nightsgrab) // Home in on your target
{
	fixed_t dist, speedmul;
	mobj_t* dest;

	if(!enemy->health || !enemy->player || !source->tracer)
		return;

	// adjust direction
	dest = source->tracer;

	if(!dest || dest->health <= 0)
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);

	// change slope
	dist = P_AproxDistance(P_AproxDistance(dest->x - source->x, dest->y - source->y),
		dest->z - source->z);

	if(dist < 1)
		dist = 1;

	if(nightsgrab)
		speedmul = P_AproxDistance(enemy->momx, enemy->momy) + 8*FRACUNIT;
	else
		speedmul = source->info->speed;

	source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), speedmul);
	source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), speedmul);
	source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), speedmul);

	return;
}

static void P_NightsItemChase(mobj_t* thing)
{
	if(!thing->tracer)
	{
		thing->tracer = NULL;
		thing->flags2 &= ~MF2_NIGHTSPULL;
		return;
	}

	if(!thing->tracer->player)
		return;

	P_Attract(thing, thing->tracer, true);
}

static boolean P_ShieldLook(mobj_t* thing, powertype_t power)
{
	if(thing->state == &states[S_DISS])
		return false;

	if(!thing->target || thing->target->health <= 0 || !thing->target->player
		|| !thing->target->player->powers[power] || thing->target->player->powers[pw_super]
		|| thing->target->player->powers[pw_invulnerability] > 1)
	{
		P_SetMobjState(thing, S_DISS);
		return false;
	}

	P_UnsetThingPosition(thing);
	thing->x = thing->target->x;
	thing->y = thing->target->y;
	thing->z = thing->target->z - (thing->target->info->height - thing->target->height) / 3;
	P_SetThingPosition(thing);

	return true;
}

void A_BossDeath(mobj_t* mo);
// AI for the Koopa boss.
static void P_KoopaThinker(mobj_t* koopa)
{
	P_MobjCheckWater(koopa);

	if(koopa->watertop > koopa->z + koopa->height + 128*FRACUNIT && koopa->health > 0)
	{
		A_BossDeath(koopa);
		P_SetMobjState(koopa, S_DISS);
		koopa->health = 0;
		return;
	}

	// Koopa moves ONLY on the X axis!
	if(koopa->threshold > 0)
	{
		koopa->threshold--;
		koopa->momx = FRACUNIT;

		if(!koopa->threshold)
			koopa->threshold = -TICRATE*2;
	}
	else if(koopa->threshold < 0)
	{
		koopa->threshold++;
		koopa->momx = -FRACUNIT;

		if(!koopa->threshold)
			koopa->threshold = TICRATE*2;
	}
	else
		koopa->threshold = TICRATE*2;

	P_XYMovement(koopa);

	if(P_Random() < 8 && koopa->z <= koopa->floorz)
		koopa->momz = 5*FRACUNIT;

	if(koopa->z > koopa->floorz)
		koopa->momz += FRACUNIT/4;

	if(P_Random() < 4)
	{
		mobj_t* flame;
		flame = P_SpawnMobj(koopa->x - koopa->radius + 5*FRACUNIT, koopa->y, koopa->z + (P_Random() >> FRACBITS)/4, MT_KOOPAFLAME);
		if(!flame)
			return;
		flame->momx = -flame->info->speed;
		S_StartSound(flame, sfx_koopfr);
	}
	else if(P_Random() > 250 && gameskill >= sk_hard)
	{
		mobj_t* hammer;
		hammer = P_SpawnMobj(koopa->x - koopa->radius, koopa->y, koopa->z + koopa->height, MT_HAMMER);
		if(!hammer)
			return;
		hammer->momx = -5*FRACUNIT;
		hammer->momz = 7*FRACUNIT;
	}
}

//
// P_MobjThinker
//
void P_MobjThinker(mobj_t* mobj)
{
	if(mobj->flags2 & MF2_NOTHINK)
		return;

	// Special thinker for scenery objects
	if(mobj->flags & MF_SCENERY)
	{
		switch(mobj->type)
		{
			case MT_BLACKORB:
			case MT_WHITEORB:
			case MT_REDORB:
			case MT_YELLOWORB:
			case MT_BLUEORB:
				if(!P_ShieldLook(mobj, mobj->info->speed))
					return;
				mobj->floorz = mobj->z;
				mobj->ceilingz = mobj->z+mobj->height;
				break;
			case MT_BUBBLES:
				P_SceneryCheckWater(mobj);
				break;
			case MT_SMALLBUBBLE:
			case MT_MEDIUMBUBBLE:
			case MT_EXTRALARGEBUBBLE:	// start bubble dissipate
				P_SceneryCheckWater(mobj);
				if(!(mobj->eflags & MF_UNDERWATER) || mobj->z + mobj->height >= mobj->ceilingz)
				{
					int random;

					P_SetMobjState(mobj, S_DISS);

					if(mobj->threshold == 42) // Don't make pop sound.
						break;

					random = P_Random();

					if(random <= 51)
						S_StartSound(mobj, sfx_bubbl1);
					else if(random <= 102)
						S_StartSound(mobj, sfx_bubbl2);
					else if(random <= 153)
						S_StartSound(mobj, sfx_bubbl3);
					else if(random <= 204)
						S_StartSound(mobj, sfx_bubbl4);
					else
						S_StartSound(mobj, sfx_bubbl5);
				}
				break;
			case MT_ZERO:
			case MT_ONE:
			case MT_TWO:
			case MT_THREE:
			case MT_FOUR:
			case MT_FIVE:
				if(!mobj->target)
				{
					P_SetMobjState(mobj, S_DISS);
					break;
				}
				if(!mobj->target->player || !(mobj->target->player->powers[pw_underwater] || mobj->target->player->powers[pw_spacetime]))
				{
					P_SetMobjState(mobj, S_DISS);
					break;
				}
				mobj->x = mobj->target->x;
				mobj->y = mobj->target->y;
				mobj->z = mobj->target->z + (mobj->target->height) + 8*FRACUNIT; // Adjust height for height changes
				if(mobj->threshold <= 35)
					mobj->flags2 |= MF2_DONTDRAW;
				else
					mobj->flags2 &= ~MF2_DONTDRAW;
				if(mobj->threshold <= 30)
					mobj->threshold = 40;
				mobj->threshold--;
				break;
			default:
				break;
		}

		P_SceneryThinker(mobj);
		return;
	}

	// if it's pushable, or if it would be pushable other than temporary disablement, use the
	// separate thinker
	if(mobj->flags & MF_PUSHABLE || (mobj->info->flags & MF_PUSHABLE && mobj->fuse))
	{
		P_MobjCheckWater(mobj);
		P_PushableThinker(mobj);
	}
	else switch(mobj->type)
	{
		case MT_HOOP:
			if(mobj->fuse > 1)
				P_MoveHoop(mobj);
			else if(mobj->fuse == 1)
				mobj->movecount = 1;

			if(mobj->movecount)
			{
				mobj->fuse++;

				if(mobj->fuse > 32)
					P_RemoveMobj(mobj);
			}
			else
				mobj->fuse--;
			return;
		case MT_HOOPCOLLIDE:
			// In Soviet Russia, hoop finds YOU!
			P_CheckHoopPosition(mobj, mobj->x, mobj->y, mobj->z + (mobj->height/2), mobj->radius);
			return;
		case MT_NIGHTSPARKLE:
			if(mobj->tics != -1)
			{
				mobj->tics--;

				// you can cycle through multiple states in a tic
				if(!mobj->tics)
					if(!P_SetMobjState(mobj, mobj->state->nextstate))
						return; // freed itself
			}
			if(mobj->flags & MF_SPECIAL)
				return;

			if(mobj->momx || mobj->momy)
			{
				P_XYMovement(mobj);

				// FIXME: decent NOP/NULL/Nil function pointer please.
				if((mobj->thinker.function.acv == (actionf_v)(-1)))
					return; // mobj was removed
			}

			if(mobj->momz)
				P_ZMovement(mobj);
			return;
		case MT_ROCKCRUMBLE1:
		case MT_ROCKCRUMBLE2:
		case MT_ROCKCRUMBLE3:
		case MT_ROCKCRUMBLE4:
		case MT_ROCKCRUMBLE5:
		case MT_ROCKCRUMBLE6:
		case MT_ROCKCRUMBLE7:
		case MT_ROCKCRUMBLE8:
		case MT_ROCKCRUMBLE9:
		case MT_ROCKCRUMBLE10:
		case MT_ROCKCRUMBLE11:
		case MT_ROCKCRUMBLE12:
		case MT_ROCKCRUMBLE13:
		case MT_ROCKCRUMBLE14:
		case MT_ROCKCRUMBLE15:
		case MT_ROCKCRUMBLE16:
			if(mobj->z <= mobj->floorz)
			{
				P_SetMobjState(mobj, mobj->info->deathstate);
				return;
			}
			break;
		case MT_SUPERTRANS:
			if(mobj->state == &states[S_NIGHTSDRONE1])
			{
				mobj->health = 0;
				P_SetMobjState(mobj, S_DISS);
			}
			break;
		case MT_SPINFIRE:
			mobj->z = mobj->floorz+1;
			break;
		case MT_EGGCAPSULE:
		case MT_HAMMER:
			if(mobj->z <= mobj->floorz)
				P_SetMobjState(mobj,S_DISS);
			break;
		case MT_KOOPA:
			P_KoopaThinker(mobj);
			break;
		case MT_STREETLIGHT:
			if(mobj->z + mobj->height != mobj->ceilingz && leveltime > 0)
			{
				mobj->momz = FRACUNIT;
				P_ZMovement(mobj);
				P_CheckPosition(mobj, mobj->x, mobj->y);
				mobj->z = mobj->ceilingz - mobj->height;
			}
			return;
		case MT_REDRING:
			if(((mobj->z < mobj->floorz) || (mobj->z + mobj->height > mobj->ceilingz))
				&& mobj->flags & MF_MISSILE)
			{
				P_ExplodeMissile(mobj);
			}
			break;
		case MT_BOSSFLYPOINT:
			return;
		case MT_NIGHTSCORE:
			mobj->flags = (mobj->flags & ~MF_TRANSLATION) | ((leveltime % 13)<<MF_TRANSSHIFT);
			break;
		case MT_JETFUME1:
			{
				fixed_t jetx, jety;

				if(!mobj->target)
				{
					P_SetMobjState(mobj, S_DISS);
					return;
				}

				if(gametype == GT_CHAOS && mobj->target->health <= 0)
					P_SetMobjState(mobj, S_DISS);

				jetx = mobj->target->x + P_ReturnThrustX(mobj->target, mobj->target->angle, -56*FRACUNIT);
				jety = mobj->target->y + P_ReturnThrustY(mobj->target, mobj->target->angle, -56*FRACUNIT);

				if(mobj->fuse == 56) // First one
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx;
					mobj->y = jety;
					mobj->z = mobj->target->z + 8*FRACUNIT;
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				else if(mobj->fuse == 57)
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle-ANG90, 24*FRACUNIT);
					mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle-ANG90, 24*FRACUNIT);
					mobj->z = mobj->target->z + 32*FRACUNIT;
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				else if(mobj->fuse == 58)
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle+ANG90, 24*FRACUNIT);
					mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle+ANG90, 24*FRACUNIT);
					mobj->z = mobj->target->z + 32*FRACUNIT;
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				mobj->fuse++;
			}
			break;
		case MT_JETFUME2:
			if(!mobj->target)
			{
				P_SetMobjState(mobj, S_DISS);
				return;
			}
			if(gametype == GT_CHAOS)
				P_SetMobjState(mobj, S_DISS);
			P_UnsetThingPosition(mobj);
			mobj->x = mobj->target->x;
			mobj->y = mobj->target->y;
			mobj->z = mobj->target->z - 15*FRACUNIT;
			if(mobj->target->health < 3)
				mobj->flags |= MF_NOSECTOR;
			if(mobj->target->health <= 0)
				mobj->flags &= ~MF_NOSECTOR;
			P_SetThingPosition(mobj);
			break;
		case MT_SEED:
			mobj->momz = mobj->info->speed;
			break;
		case MT_NIGHTSDRONE:
			if(mobj->tracer && mobj->tracer->player && !mobj->tracer->player->nightsmode)
				mobj->flags2 &= ~MF2_DONTDRAW;
			mobj->angle += ANGLE_1*10;
			if(mobj->z <= mobj->floorz)
				mobj->momz = 5*FRACUNIT;
			break;
		case MT_PLAYER:
			P_PlayerMobjThinker(mobj);
			mobj->pmomz = 0; // Needs reset here (fixes bug)
			return;
		case MT_FAN: // Fans spawn bubbles underwater
			// check mobj against possible water content
			P_MobjCheckWater(mobj);
			if(mobj->eflags & MF_UNDERWATER)
			{
				if(!(P_Random() % 16))
					P_SpawnMobj(mobj->x, mobj->y, mobj->z + (fixed_t)(mobj->height / 1.25f), MT_SMALLBUBBLE);
				else if(!(P_Random() % 96))
					P_SpawnMobj(mobj->x, mobj->y, mobj->z + (fixed_t)(mobj->height / 1.25f), MT_MEDIUMBUBBLE);
			}
			break;
		case MT_EGGMOBILE:
			if(mobj->health < mobj->info->damage+1 && leveltime & 1 && mobj->health > 0)
				P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_SMOK);
			if(mobj->flags2 & MF2_SKULLFLY)
			{
				mobj_t* spawnmobj;
				spawnmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->painchance);
				spawnmobj->flags = (spawnmobj->flags & ~MF_TRANSLATION) | (1<<MF_TRANSSHIFT);
			}
			P_Boss1Thinker(mobj);
			if(mobj->flags2 & MF2_BOSSFLEE)
				P_InstaThrust(mobj, mobj->angle, 12*FRACUNIT);
			break;
		case MT_EGGMOBILE2:
			P_Boss2Thinker(mobj);
			if(mobj->flags2 & MF2_BOSSFLEE)
				P_InstaThrust(mobj, mobj->angle, 12*FRACUNIT);
			break;
		case MT_SKIM:
			// check mobj against possible water content, before movement code
			P_MobjCheckWater(mobj);

			// Keep Skim at water surface
			if(mobj->z <= mobj->watertop)
			{
				mobj->flags |= MF_NOGRAVITY;
				if(mobj->z < mobj->watertop)
				{
					if(mobj->watertop - mobj->z <= mobj->info->speed*FRACUNIT)
						mobj->z = mobj->watertop;
					else
						mobj->momz = mobj->info->speed*FRACUNIT;
				}
			}
			else
			{
				mobj->flags &= ~MF_NOGRAVITY;
				if(mobj->z > mobj->watertop && mobj->z - mobj->watertop < MAXSTEPMOVE)
					mobj->z = mobj->watertop;
			}
			break;
		case MT_RING:
		case MT_COIN:
			// No need to check water. Who cares?
			P_RingThinker(mobj);
			if(mobj->flags2 & MF2_NIGHTSPULL)
				P_NightsItemChase(mobj);
			return;
		case MT_NIGHTSWING:
			if(mobj->flags2 & MF2_NIGHTSPULL)
				P_NightsItemChase(mobj);
			break;
		case MT_SHELL:
			if(mobj->threshold > TICRATE)
				mobj->threshold--;

			if(mobj->state != &states[S_SHELL])
			{
				P_InstaThrust(mobj, mobj->angle, mobj->info->speed);
			}
			break;
		case MT_TURRET:
			P_MobjCheckWater(mobj);
			if((mobj->eflags & MF_UNDERWATER) && mobj->health > 0)
			{
				P_SetMobjState(mobj, mobj->info->deathstate);
				mobj->health = 0;
				mobj->flags2 &= ~MF2_FIRING;
			}
			break;
		default:
			// check mobj against possible water content, before movement code
			P_MobjCheckWater(mobj);
			break;
	}

	if(mobj->flags2 & MF2_FIRING && mobj->target)
	{
		if(mobj->health > 0 && (leveltime & 1)) // Fire mode
		{
			mobj_t* missile;

			mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
			
			missile = P_SpawnMissile(mobj, mobj->target, MT_TURRETLASER);

			if(mobj->flags2 & MF2_SUPERFIRE)
				missile->flags2 |= MF2_SUPERFIRE;

			if(mobj->info->attacksound)
				S_StartSound(missile, mobj->info->attacksound);
		}
		else if(mobj->health > 0)
			mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
	}

	if(mobj->flags & MF_AMBIENT)
	{
		if(!(leveltime % mobj->health) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);
		return;
	}

	// Check fuse
	if(mobj->fuse)
	{
		mobj->fuse--;
		if(!mobj->fuse)
		{
			subsector_t* ss;
			fixed_t x, y, z;
			mobj_t*	flagmo;

			switch(mobj->type)
			{
				int random;

				// gargoyle and snowman handled in P_PushableThinker, not here
				case MT_BLUEFLAG:
				case MT_REDFLAG:
					x = mobj->spawnpoint->x << FRACBITS;
					y = mobj->spawnpoint->y << FRACBITS;
					ss = R_PointInSubsector(x, y);
					z = ss->sector->floorheight;
					if(mobj->spawnpoint->z)
						z += mobj->spawnpoint->z << FRACBITS;
					flagmo = P_SpawnMobj(x, y, z, mobj->type);
					flagmo->spawnpoint = mobj->spawnpoint;
					P_SetMobjState(mobj, S_DISS);
					break;
				case MT_YELLOWTV: // Ring shield box
				case MT_REDTV: // Fire shield box
				case MT_BLUETV: // Water shield box
				case MT_BLACKTV: // Bomb shield box
				case MT_WHITETV: // Jump shield box
				case MT_SNEAKERTV: // Super Sneaker box
				case MT_SUPERRINGBOX: // 10-Ring box
				case MT_GREYRINGBOX: // 25-Ring box
				case MT_INV: // Invincibility box
				case MT_MIXUPBOX: // Teleporter Mixup box
				case MT_PRUP: // 1up!
				case MT_EGGMANBOX:
					P_SetMobjState(mobj, S_DISS); // make sure they disappear

					if(mobj->flags & MF_AMBUSH)
					{
						mobjtype_t spawnchance[30];
						int i = 0;
						int oldi = 0;
						int numchoices = 0;

						random = P_Random(); // Gotta love those random numbers!

						if(cv_superring.value)
						{
							oldi = i;
							for(; i < oldi + cv_superring.value; i++)
							{
								spawnchance[i] = MT_SUPERRINGBOX;
								numchoices++;
							}
						}
						if(cv_silverring.value)
						{
							oldi = i;
							for(; i < oldi + cv_silverring.value; i++)
							{
								spawnchance[i] = MT_GREYRINGBOX;
								numchoices++;
							}
						}
						if(cv_supersneakers.value)
						{
							oldi = i;
							for(; i < oldi + cv_supersneakers.value; i++)
							{
								spawnchance[i] = MT_SNEAKERTV;
								numchoices++;
							}
						}
						if(cv_invincibility.value)
						{
							oldi = i;
							for(; i < oldi + cv_invincibility.value; i++)
							{
								spawnchance[i] = MT_INV;
								numchoices++;
							}
						}
						if(cv_jumpshield.value)
						{
							oldi = i;
							for(; i < oldi + cv_jumpshield.value; i++)
							{
								spawnchance[i] = MT_WHITETV;
								numchoices++;
							}
						}
						if(cv_watershield.value)
						{
							oldi = i;
							for(; i < oldi + cv_watershield.value; i++)
							{
								spawnchance[i] = MT_BLUETV;
								numchoices++;
							}
						}
						if(cv_ringshield.value)
						{
							oldi = i;
							for(; i < oldi + cv_ringshield.value; i++)
							{
								spawnchance[i] = MT_YELLOWTV;
								numchoices++;
							}
						}
						if(cv_fireshield.value)
						{
							oldi = i;
							for(; i < oldi + cv_fireshield.value; i++)
							{
								spawnchance[i] = MT_REDTV;
								numchoices++;
							}
						}
						if(cv_bombshield.value)
						{
							oldi = i;
							for(; i < oldi + cv_bombshield.value; i++)
							{
								spawnchance[i] = MT_BLACKTV;
								numchoices++;
							}
						}
						if(cv_1up.value && gametype == GT_RACE)
						{
							oldi = i;
							for(; i < oldi + cv_1up.value; i++)
							{
								spawnchance[i] = MT_PRUP;
								numchoices++;
							}
						}
						if(cv_teleporters.value)
						{
							oldi = i;
							for(; i < oldi + cv_teleporters.value; i++)
							{
								spawnchance[i] = MT_MIXUPBOX;
								numchoices++;
							}
						}

						if(!(cv_1up.value || cv_bombshield.value || cv_fireshield.value
							|| cv_ringshield.value || cv_watershield.value || cv_jumpshield.value
							|| cv_invincibility.value || cv_supersneakers.value || cv_silverring.value
							|| cv_superring.value || cv_teleporters.value))
							CONS_Printf("Note: All monitors turned off.\n");
						else
							P_SpawnMobj(mobj->x, mobj->y, mobj->z, spawnchance[random%numchoices])->flags |= MF_AMBUSH;
					}
					else
						P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->type);
					break;
				case MT_QUESTIONBOX:
					P_SetMobjState(mobj, S_DISS);
					P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_QUESTIONBOX);
					break;
				case MT_EGGTRAP: // Egg Capsule animal release
				{
					int i,j;
					fixed_t x, y, z;
					mobj_t* mo2;

					z = mobj->subsector->sector->floorheight + 64*FRACUNIT;
					for(j = 0; j < 2; j++)
					{
						for(i = 0; i < 32; i++)
						{
							x = (fixed_t)(mobj->x + sin(i*22.5) * 64 * FRACUNIT);
							y = (fixed_t)(mobj->y + cos(i*22.5) * 64 * FRACUNIT);

							mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
							mo2->momx = (fixed_t)(sin(i*22.5) * 4 * FRACUNIT);
							mo2->momy = (fixed_t)(cos(i*22.5) * 4 * FRACUNIT);

							if(i&1)
							{
								P_SpawnMobj(x, y, z, MT_BIRD);
								S_StartSound(mo2, sfx_pop);
							}
							else if(i < 16)
								P_SpawnMobj(x, y, z, MT_SQRL);
							else
								P_SpawnMobj(x, y, z, MT_MOUSE);
						}
						z -= 32*FRACUNIT;
					}
					// Mark all players with the time to exit thingy!
					for(i = 0; i < MAXPLAYERS; i++)
						players[i].exiting = (int)(2.8f*TICRATE + 1);
				}
				break;
				case MT_CHAOSSPAWNER: // Chaos Mode spawner thingy
				{
					// 8 enemies: Blue Crawla, Red Crawla, Crawla Commander,
					//            Jett-Synn Bomber, Jett-Synn Gunner, Skim,
					//            Egg Mobile, Egg Slimer.
					// Max. 3 chances per enemy.
					mobjtype_t spawnchance[8*3], enemy;
					mobj_t* spawnedmo;
					int i = 0, numchoices = 0, stop;
					fixed_t sfloorz, space, airspace, spawnz[8*3];

					sfloorz = mobj->floorz;
					space = mobj->ceilingz - sfloorz;

					// This makes the assumption there is no gravity-defying water.
					// A fair assumption to make, if you ask me.
					airspace = min(space, mobj->ceilingz - mobj->watertop);

					mobj->fuse = cv_chaos_spawnrate.value*TICRATE;
					random = P_Random(); // Gotta love those random numbers!

					if(cv_chaos_bluecrawla.value && space >= mobjinfo[MT_BLUECRAWLA].height)
					{
						stop = i + cv_chaos_bluecrawla.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_BLUECRAWLA;
							spawnz[i] = sfloorz;
							numchoices++;
						}
					}
					if(cv_chaos_redcrawla.value && space >= mobjinfo[MT_REDCRAWLA].height)
					{
						stop = i + cv_chaos_redcrawla.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_REDCRAWLA;
							spawnz[i] = sfloorz;
							numchoices++;
						}
					}
					if(cv_chaos_crawlacommander.value
						&& space >= mobjinfo[MT_CRAWLACOMMANDER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_crawlacommander.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_CRAWLACOMMANDER;
							spawnz[i] = sfloorz + 33*FRACUNIT;
							numchoices++;
						}
					}
					if(cv_chaos_jettysynbomber.value
						&& airspace >= mobjinfo[MT_JETTBOMBER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_jettysynbomber.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_JETTBOMBER;
							spawnz[i] = max(sfloorz, mobj->watertop) + 33*FRACUNIT;
							numchoices++;
						}
					}
					if(cv_chaos_jettysyngunner.value
						&& airspace >= mobjinfo[MT_JETTGUNNER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_jettysyngunner.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_JETTGUNNER;
							spawnz[i] = max(sfloorz, mobj->watertop) + 33*FRACUNIT;
							numchoices++;
						}
					}
					if(cv_chaos_skim.value
						&& mobj->watertop < mobj->ceilingz - mobjinfo[MT_SKIM].height
						&& mobj->watertop - sfloorz > mobjinfo[MT_SKIM].height/2)
					{
						stop = i + cv_chaos_skim.value;
						for(; i < stop; i++)
						{
							spawnchance[i] = MT_SKIM;
							spawnz[i] = mobj->watertop;
							numchoices++;
						}
					}
					if(P_BossDoesntExist())
					{
						if(cv_chaos_eggmobile1.value
							&& space >= mobjinfo[MT_EGGMOBILE].height + 33*FRACUNIT)
						{
							stop = i + cv_chaos_eggmobile1.value;
							for(; i < stop; i++)
							{
								spawnchance[i] = MT_EGGMOBILE;
								spawnz[i] = sfloorz + 33*FRACUNIT;
								numchoices++;
							}
						}
						if(cv_chaos_eggmobile2.value
							&& space >= mobjinfo[MT_EGGMOBILE2].height + 33*FRACUNIT)
						{
							stop = i + cv_chaos_eggmobile2.value;
							for(; i < stop; i++)
							{
								spawnchance[i] = MT_EGGMOBILE2;
								spawnz[i] = sfloorz + 33*FRACUNIT;
								numchoices++;
							}
						}
					}

					if(numchoices)
					{
						fixed_t fogz;

						i = random % numchoices;
						enemy = spawnchance[i];

						fogz = spawnz[i] - 32*FRACUNIT;
						if(fogz < sfloorz)
							fogz = sfloorz;

						spawnedmo = P_SpawnMobj(mobj->x, mobj->y, spawnz[i], enemy);
						P_SpawnMobj(mobj->x, mobj->y, fogz, MT_TFOG);

						P_SupermanLook4Players(spawnedmo);
						if(spawnedmo->target && spawnedmo->type != MT_SKIM)
							P_SetMobjState(spawnedmo, spawnedmo->info->seestate);

						if(spawnedmo->flags & MF_BOSS)
						{
							spawnedmo->flags2 |= MF2_CHAOSBOSS;
							spawnedmo->momx = spawnedmo->momy = 0;
						}
					}
					break;
				}
				default:
					if(mobj->info->deathstate)
						P_ExplodeMissile(mobj);
					else
						P_SetMobjState(mobj, S_DISS); // make sure they disappear
					break;
			}
		}
	}

	if(mobj->momx || mobj->momy || (mobj->flags2 & MF2_SKULLFLY))
	{
		P_XYMovement(mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if((mobj->thinker.function.acv == (actionf_v)(-1)))
			return; // mobj was removed
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if(!(mobj->eflags & MF_ONGROUND) || (mobj->z != mobj->floorz) || mobj->momz)
	{
		mobj_t* onmo;
		onmo = P_CheckOnmobj(mobj);
		if(!onmo)
		{
			P_ZMovement(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
		}

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if(mobj->thinker.function.acv == (actionf_v)(-1))
			return; // mobj was removed
	}
	else
		mobj->eflags &= ~MF_JUSTHITFLOOR;

	// cycle through states,
	// calling action functions at transitions
	if(mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if(!mobj->tics)
			if(!P_SetMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

// Quick, optimized function for the Rail Rings
void P_RailThinker(mobj_t* mobj)
{
	// momentum movement
	if(mobj->momx || mobj->momy)
	{
		P_XYMovement(mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if((mobj->thinker.function.acv == (actionf_v)(-1)))
			return; // mobj was removed
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if(/*!(mobj->eflags & MF_ONGROUND) || (mobj->z != mobj->floorz) || */mobj->momz)
	{
//		mobj_t* onmo;
//		onmo = P_CheckOnmobj(mobj);
//		if(!onmo)
//		{
			P_ZMovement(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
//		}

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if(mobj->thinker.function.acv == (actionf_v)(-1))
			return; // mobj was removed
	}

	if(mobj->flags2 & MF2_HOMING)
		A_ThrownRing(mobj);
}

// Unquick, unoptimized function for pushables
void P_PushableThinker(mobj_t* mobj)
{
	sector_t* sec;

	sec = mobj->subsector->sector;

	if(sec->special == 971 && mobj->z == sec->floorheight)
		P_LinedefExecute(sec->tag, mobj, sec);
//	else if(sec->special == 970)
	{
		sector_t* sec2;

		sec2 = P_ThingOnSpecial3DFloor(mobj);
		if(sec2 && sec2->special == 971)
			P_LinedefExecute(sec2->tag, mobj, sec2);
	}

	// it has to be pushable RIGHT NOW for this part to happen
	if(mobj->flags & MF_PUSHABLE && !(mobj->momx || mobj->momy))
		P_TryMove(mobj, mobj->x, mobj->y, true);

	if(mobj->fuse == 1) // it would explode in the MobjThinker code
	{
		// Left here just in case we'd
		// want to make pushable bombs
		// or something in the future.
	}
}

// Quick, optimized function for scenery
void P_SceneryThinker(mobj_t* mobj)
{
	if(mobj->flags & MF_MONITORICON)
	{
		if(mobj->z < mobj->floorz + mobj->info->damage)
		{
			mobj->momz = mobj->info->speed;
		}
		else
			mobj->momz = 0;
	}

	// momentum movement
	if(mobj->momx || mobj->momy)
	{
		P_SceneryXYMovement(mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if((mobj->thinker.function.acv == (actionf_v)(-1)))
			return; // mobj was removed
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if(!(mobj->eflags & MF_ONGROUND) || (mobj->z != mobj->floorz) || mobj->momz)
	{
		mobj_t* onmo;
		onmo = P_CheckOnmobj(mobj);
		if(!onmo)
		{
			P_SceneryZMovement(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
		}

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if(mobj->thinker.function.acv == (actionf_v)(-1))
			return; // mobj was removed
	}
	else
		mobj->eflags &= ~MF_JUSTHITFLOOR;

	// cycle through states, calling action functions at transitions
	if(mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if(!mobj->tics)
			if(!P_SetMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

static void P_MobjNullThinker(mobj_t* mobj)
{
	mobj = NULL;
}

//
// P_SpawnMobj
//
mobj_t* P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	mobj_t* mobj;
	state_t* st;
	mobjinfo_t* info;

	mobj = Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
	memset(mobj, 0, sizeof(*mobj));
	info = &mobjinfo[type];

	mobj->type = type;
	mobj->info = info;

	mobj->x = x;
	mobj->y = y;

	if(type == MT_RING || type == MT_COIN)
	{
		if(maptol & TOL_NIGHTS)
		{
			thinker_t* th;
			mobj_t* mo2;
			mobj_t* closestaxis = NULL;
			unsigned short first = 0;
			double angle_pos;

			// scan the thinkers to find the closest axis point
			for(th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if(th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t*)th;

				if(!first)
				{
					if(mo2->type == MT_AXIS1
						|| mo2->type == MT_AXIS2
						|| mo2->type == MT_AXIS3
						|| mo2->type == MT_AXIS1A
						|| mo2->type == MT_AXIS2A
						|| mo2->type == MT_AXIS3A)
					{
						closestaxis = mo2;
						first++;
					}
				}
				else
				{
					if(mo2->type == MT_AXIS1
						|| mo2->type == MT_AXIS2
						|| mo2->type == MT_AXIS3
						|| mo2->type == MT_AXIS1A
						|| mo2->type == MT_AXIS2A
						|| mo2->type == MT_AXIS3A)
					{
						fixed_t dist1, dist2;

						dist1 = R_PointToDist2(mobj->x, mobj->y, mo2->x, mo2->y)-mo2->info->radius;
						dist2 = R_PointToDist2(mobj->x, mobj->y, closestaxis->x, closestaxis->y)-closestaxis->info->radius;

						if(dist1 <= dist2)
							closestaxis = mo2;
					}
				}
			}

			if(closestaxis)
			{
				angle_pos = atan2((mobj->y - closestaxis->y), (mobj->x - closestaxis->x))/deg2rad;
				if(angle_pos < 0.0)
					angle_pos += 360.0;
				else if(angle_pos >= 360.0)
					angle_pos -= 360.0;

				mobj->x = (fixed_t)(closestaxis->x + cos(angle_pos * deg2rad) * closestaxis->info->radius);
				mobj->y = (fixed_t)(closestaxis->y + sin(angle_pos * deg2rad) * closestaxis->info->radius);
				mobj->angle = R_PointToAngle2(mobj->x, mobj->y, closestaxis->x, closestaxis->y);
			}
		}
	}
	else if(type == MT_DETON)
		mobj->movedir = 0; // used for up/down angle

	mobj->radius = info->radius;
	mobj->height = info->height;
	mobj->flags = info->flags;

	mobj->health = info->spawnhealth;

	if(gameskill < sk_nightmare)
		mobj->reactiontime = info->reactiontime;

	mobj->lastlook = -1; // stuff moved in P_enemy.P_LookForPlayer

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[info->spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..
	mobj->touching_sectorlist = NULL;
	mobj->friction = ORIG_FRICTION;

	mobj->movefactor = ORIG_FRICTION_FACTOR;

	// set subsector and/or block links
	P_SetThingPosition(mobj);

	mobj->floorz = mobj->subsector->sector->floorheight;
	mobj->ceilingz = mobj->subsector->sector->ceilingheight;
	mobj->floorff = mobj->ceilingff = NULL;

	if(z == ONFLOORZ)
	{
		// defaults onground
		mobj->eflags |= MF_ONGROUND;

		if((mobj->type == MT_RING || mobj->type == MT_COIN) && mobj->flags & MF_AMBUSH)
			mobj->z = mobj->floorz + 32*FRACUNIT;
		else
			mobj->z = mobj->floorz;
	}
	else if(z == ONCEILINGZ)
		mobj->z = mobj->ceilingz - mobj->height;
	else
		mobj->z = z;

	// special hack for spirit
	if(mobj->type == MT_SPIRIT)
		mobj->thinker.function.acv = (actionf_p1)P_MobjNullThinker;
	else
	{
		mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
		P_AddThinker(&mobj->thinker);
	}

	// Fuse for bunnies, squirrels, and flingrings
	if(mobj->type == MT_BIRD || mobj->type == MT_SQRL || mobj->type == MT_MOUSE)
		mobj->fuse = 300 + (P_Random() % 50);

	return mobj;
}

static inline precipmobj_t* P_SpawnRainMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	precipmobj_t* mobj;
	state_t* st;

	mobj = Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
	memset(mobj, 0, sizeof(*mobj));

	mobj->x = x;
	mobj->y = y;
	mobj->flags = mobjinfo[type].flags;

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[mobjinfo[type].spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..
	mobj->touching_sectorlist = NULL;

	// set subsector and/or block links
	P_SetPrecipitationThingPosition(mobj);

	mobj->floorz = mobj->subsector->sector->floorheight;

	mobj->z = z;
	mobj->momz = mobjinfo[type].speed;

	mobj->thinker.function.acp1 = (actionf_p1)P_RainThinker;
	P_AddThinker(&mobj->thinker);

	CalculatePrecipFloor(mobj);

	return mobj;
}

static precipmobj_t* P_SpawnSnowMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	precipmobj_t* mobj;
	state_t* st;

	mobj = Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
	memset(mobj, 0, sizeof(*mobj));

	mobj->x = x;
	mobj->y = y;
	mobj->flags = mobjinfo[type].flags;

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[mobjinfo[type].spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..
	mobj->touching_sectorlist = NULL;

	// set subsector and/or block links
	P_SetPrecipitationThingPosition(mobj);

	mobj->floorz = mobj->subsector->sector->floorheight;

	mobj->z = z;
	mobj->momz = mobjinfo[type].speed;

	mobj->thinker.function.acp1 = (actionf_p1)P_SnowThinker;
	P_AddThinker(&mobj->thinker);

	CalculatePrecipFloor(mobj);

	return mobj;
}

//
// P_RemoveMobj
//
mapthing_t* itemrespawnque[ITEMQUESIZE];
tic_t itemrespawntime[ITEMQUESIZE];
int iquehead, iquetail;

void P_RemoveMobj(mobj_t* mobj)
{
	// Rings only, please!
	if
	(
	 mobj->spawnpoint &&
	 (
	  mobj->type == MT_RING
	  || mobj->type == MT_COIN
	  || mobj->type == MT_HOMINGRING
	  || mobj->type == MT_RAILRING
	  || mobj->type == MT_INFINITYRING
	  || mobj->type == MT_AUTOMATICRING
	  || mobj->type == MT_EXPLOSIONRING
	 ) && !(mobj->flags2 & MF2_DONTRESPAWN)
	)
	{
		itemrespawnque[iquehead] = mobj->spawnpoint;
		itemrespawntime[iquehead] = leveltime;
		iquehead = (iquehead+1)&(ITEMQUESIZE-1);
		// lose one off the end?
		if(iquehead == iquetail)
			iquetail = (iquetail+1)&(ITEMQUESIZE-1);
	}

	// unlink from sector and block lists
	P_UnsetThingPosition(mobj);

	// Remove touching_sectorlist from mobj.
	if(sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	// stop any playing sound
	S_StopSound(mobj);

	// free block
	P_RemoveThinker((thinker_t*)mobj);
}

void P_RemovePrecipMobj(precipmobj_t* mobj)
{
	// unlink from sector and block lists
	P_UnsetPrecipThingPosition(mobj);

	if(precipsector_list)
	{
		P_DelPrecipSeclist(precipsector_list);
		precipsector_list = NULL;
	}

	// free block
	P_RemoveThinker((thinker_t*)mobj);
}

// Clearing out stuff for savegames
void P_RemoveSavegameMobj(mobj_t* mobj)
{
	// unlink from sector and block lists
	P_UnsetThingPosition(mobj);

	// Remove touching_sectorlist from mobj.
	if(sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	// stop any playing sound
	S_StopSound(mobj);

	// free block
	P_RemoveThinker((thinker_t*)mobj);
}

static void Respawn_OnChange(void)
{
	if(gametype == GT_COOP && cv_itemrespawn.value)
		CV_SetValue(&cv_itemrespawn, 0);
}

consvar_t cv_itemrespawntime = {"respawnitemtime", "30", CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_itemrespawn = {"respawnitem", "Off", CV_NETVAR|CV_CALL, CV_OnOff, Respawn_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_flagtime = {"flagtime", "30", CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_suddendeath = {"suddendeath", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void P_SpawnPrecipitation(void)
{
	int i;
	fixed_t x, y, height;

	if(cv_snow.value)
	{
		int z;
		subsector_t* snowsector;
		z = 0;

		for(i = 0; i < 1048576 / cv_numsnow.value; i++)
		{
			x = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;
			y = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;
			height = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;

			snowsector = R_IsPointInSubsector(x, y);

			if(!snowsector)
				continue;

			if(maptol & TOL_NIGHTS) // Spawn snow from normal ceilings
			{
				if(snowsector->sector->floorheight <= snowsector->sector->ceilingheight - 32)
					// don't do it if sector height is less than 32
				{
					while(height < snowsector->sector->floorheight ||
						height >= snowsector->sector->ceilingheight)
						height = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;

					z = rand() % 256; //P_Random();
					if(z < 64)
						P_SetPrecipMobjState(P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE), S_SNOW3);
					else if(z < 144)
						P_SetPrecipMobjState(P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE), S_SNOW2);
					else
						P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE);
				}
			}
			else
			{
				if(snowsector->sector->ceilingpic == skyflatnum &&
					snowsector->sector->floorheight <= snowsector->sector->ceilingheight - 32)
					// don't do it if sector height is less than 32
				{
					while(height < snowsector->sector->floorheight ||
						height >= snowsector->sector->ceilingheight)
						height = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;

					z = rand() % 256;
					if(z < 64)
						P_SetPrecipMobjState(P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE), S_SNOW3);
					else if(z < 144)
						P_SetPrecipMobjState(P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE), S_SNOW2);
					else
						P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE);
				}
			}
		}
	}
	else if(cv_storm.value || cv_rain.value)
	{

		subsector_t* rainsector;

		for(i = 0; i < 1048576 / cv_raindensity.value; i++)
		{
			x = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;
			y = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;
			height = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;

			rainsector = R_IsPointInSubsector(x, y);

			if(!rainsector)
				continue;

			if(rainsector->sector->ceilingpic == skyflatnum && rainsector->sector->floorheight < rainsector->sector->ceilingheight)
			{
				while(!(height < rainsector->sector->ceilingheight &&
					height > rainsector->sector->floorheight))
					height = ((rand() * (65536 / (int)RAND_MAX)) - 32768) << FRACBITS;

				P_SpawnRainMobj(x, y, height, MT_RAIN);
			}
		}
	}
}

//
// P_RespawnSpecials
//
void P_RespawnSpecials(void)
{
	fixed_t x, y, z;
	mobj_t* mo = NULL;
	mapthing_t* mthing = NULL;
	int i;

	// Rain spawning
	if(cv_storm.value || cv_rain.value)
	{
		int volume;

		volume = 255;

		if(players[displayplayer].mo->subsector->sector->ceilingpic == skyflatnum);
		else
		{
			fixed_t x, y, yl, yh, xl, xh;
			fixed_t closex, closey, closedist, newdist, adx, ady;

			// Essentially check in a 1024 unit radius of the player for an outdoor area.
			yl = players[displayplayer].mo->y - 1024*FRACUNIT;
			yh = players[displayplayer].mo->y + 1024*FRACUNIT;
			xl = players[displayplayer].mo->x - 1024*FRACUNIT;
			xh = players[displayplayer].mo->x + 1024*FRACUNIT;
			closex = players[displayplayer].mo->x + 2048*FRACUNIT;
			closey = players[displayplayer].mo->y + 2048*FRACUNIT;
			closedist = 2048*FRACUNIT;
			for(y = yl; y <= yh; y += FRACUNIT*64)
				for(x = xl; x <= xh; x += FRACUNIT*64)
				{
					if(R_PointInSubsector(x, y)->sector->ceilingpic == skyflatnum) // Found the outdoors!
					{
						adx = abs(players[displayplayer].mo->x - x);
						ady = abs(players[displayplayer].mo->y - y);
						newdist = adx + ady - ((adx < ady ? adx : ady)>>1);
						if(newdist < closedist)
						{
							closex = x;
							closey = y;
							closedist = newdist;
						}
					}
				}
			volume = 255 - (closedist>>FRACBITS)/4;
		}
		if(volume < 0)
			volume = 0;
		else if(volume > 255)
			volume = 255;

		if(!leveltime || leveltime % 80 == 1)
			S_StartSoundAtVolume(players[displayplayer].mo, sfx_rainin, volume);

		if(cv_storm.value)
		{
			if(netgame ? (P_Random() < 2) : (M_Random() < 2))
			{
				sector_t* ss;
				int i;
				ss = sectors;

				for(i = 0; i < numsectors; i++, ss++)
					if(ss->ceilingpic == skyflatnum) // Only for the sky.
						P_SpawnLightningFlash(ss); // Spawn a quick flash thinker

				i = rand() % 256; // This doesn't need to use P_Random().

				if(i < 128 && leveltime & 1)
					S_StartSoundAtVolume(players[displayplayer].mo, sfx_litng1, volume);
				else if(i < 128)
					S_StartSoundAtVolume(players[displayplayer].mo, sfx_litng2, volume);
				else if(leveltime & 1)
					S_StartSoundAtVolume(players[displayplayer].mo, sfx_litng3, volume);
				else
					S_StartSoundAtVolume(players[displayplayer].mo, sfx_litng4, volume);
			}
			else if(leveltime & 1)
			{
				int random;

				random = rand() % 256; // This doesn't need to use P_Random().

				if(random > 253)
				{
					if(random & 1)
						S_StartSoundAtVolume(players[displayplayer].mo, sfx_athun1, volume);
					else
						S_StartSoundAtVolume(players[displayplayer].mo, sfx_athun2, volume);
				}
			}
		}
	}

	// only respawn items when cv_itemrespawn is on
	if(!cv_itemrespawn.value)
		return;

	// Don't respawn in special stages!
	if(gamemap >= sstage_start && gamemap <= sstage_end)
		return;

	// nothing left to respawn?
	if(iquehead == iquetail)
		return;

	// the first item in the queue is the first to respawn
	// wait at least 30 seconds
	if(leveltime - itemrespawntime[iquetail] < (tic_t)cv_itemrespawntime.value*TICRATE)
		return;

	mthing = itemrespawnque[iquetail];

#ifdef PARANOIA
	if(!mthing)
		I_Error("itemrespawnque[iquetail] is NULL!");
#endif

	if(mthing)
	{
		x = mthing->x << FRACBITS;
		y = mthing->y << FRACBITS;

		// find which type to spawn
		for(i = 0; i < NUMMOBJTYPES; i++)
			if(mthing->type == mobjinfo[i].doomednum)
				break;

		z = mthing->z << FRACBITS;

		mo = P_SpawnMobj(x, y, z, i);
		mo->spawnpoint = mthing;
		mo->angle = ANG45 * (mthing->angle/45);
	}
	// pull it from the que
	iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged between levels.
//
// spawn it at a playerspawn mapthing
void P_SpawnPlayer(mapthing_t* mthing, int playernum)
{
	player_t* p;
	fixed_t x, y, z;
	mobj_t* mobj;

	// not playing?
	if(!playeringame[playernum])
		return;

#ifdef PARANOIA
	if(playernum < 0 && playernum >= MAXPLAYERS)
		I_Error("P_SpawnPlayer: playernum not in bound (%d)",playernum);
#endif

	p = &players[playernum];

	if(p->playerstate == PST_REBORN)
		G_PlayerReborn(playernum);

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	// Flagging a player's ambush will make them start on the ceiling
	if(mthing->options & MTF_AMBUSH)
		z = ONCEILINGZ;
	else if(mthing->options >> 5)
		z = R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 5) << FRACBITS);
	else
		z = mthing->z << FRACBITS;

	mthing->z = (short)(z >> FRACBITS);
	mobj = P_SpawnMobj(x, y, z, MT_PLAYER);
	mthing->mobj = mobj;

	// set color translations for player sprites
	mobj->flags |= (p->skincolor)<<MF_TRANSSHIFT;

	// set 'spritedef' override in mobj for player skins.. (see ProjectSprite)
	// (usefulness: when body mobj is detached from player (who respawns),
	// the dead body mobj retains the skin through the 'spritedef' override).
	mobj->skin = &skins[p->skin];

	mobj->angle = ANGLE_1*mthing->angle;
	if(playernum == consoleplayer)
		localangle = mobj->angle;
	else if(cv_splitscreen.value && playernum == secondarydisplayplayer)
		localangle2 = mobj->angle;
	mobj->player = p;
	mobj->health = p->health;

	p->mo = mobj;
	p->playerstate = PST_LIVE;
	p->message = NULL;
	p->bonuscount = 0;
	p->viewheight = cv_viewheight.value<<FRACBITS;
	p->viewz = p->mo->z + p->viewheight;

	if(playernum == consoleplayer)
	{
		// wake up the status bar
		ST_Start();
		// wake up the heads up text
		HU_Start();
	}

#ifdef CLIENTPREDICTION2
	//added 1-6-98 : for movement prediction
	if(p->spirit)
		CL_ResetSpiritPosition(mobj); // reset spirit possition
	else
		p->spirit = P_SpawnMobj(x, y, z, MT_SPIRIT);

	p->spirit->skin = mobj->skin;
	p->spirit->angle = mobj->angle;
	p->spirit->player = mobj->player;
	p->spirit->health = mobj->health;
	p->spirit->movedir = weapontobutton[p->readyweapon];
	p->spirit->flags2 |= MF2_DONTDRAW;
#endif
	SV_SpawnPlayer(playernum, mobj->x, mobj->y, mobj->angle);

	if(cv_chasecam.value)
	{
		if(displayplayer == playernum)
			P_ResetCamera(p, &camera);
	}
	if(cv_chasecam2.value && cv_splitscreen.value)
	{
		if(secondarydisplayplayer == playernum)
			P_ResetCamera(p, &camera2);
	}
}

void P_SpawnStarpostPlayer(mobj_t* mobj, int playernum)
{
	player_t* p;
	fixed_t x, y, z;
	angle_t angle;
	int starposttime;

	// not playing?
	if(!playeringame[playernum])
		return;

#ifdef PARANOIA
	if(playernum < 0 && playernum >= MAXPLAYERS)
		I_Error("P_SpawnPlayer: playernum not in bound (%d)",playernum);
#endif

	p = &players[playernum];

	x = p->starpostx << FRACBITS;
	y = p->starposty << FRACBITS;
	z = p->starpostz << FRACBITS;
	angle = p->starpostangle;
	starposttime = p->starposttime;

	if(p->playerstate == PST_REBORN)
		G_PlayerReborn(playernum);

	mobj = P_SpawnMobj(x, y, z, MT_PLAYER);

	// set color translations for player sprites
	mobj->flags |= (p->skincolor)<<MF_TRANSSHIFT;

	// set 'spritedef' override in mobjy for player skins.. (see ProjectSprite)
	// (usefulness : when body mobjy is detached from player (who respawns),
	// the dead body mobj retains the skin through the 'spritedef' override).
	mobj->skin = &skins[p->skin];

	mobj->angle = angle;
	if(playernum == consoleplayer)
		localangle = mobj->angle;
	else if(cv_splitscreen.value && playernum == secondarydisplayplayer)
		localangle2 = mobj->angle;
	mobj->player = p;
	mobj->health = p->health;

	p->mo = mobj;
	p->playerstate = PST_LIVE;
	p->message = NULL;
	p->bonuscount = 0;
	p->viewheight = cv_viewheight.value<<FRACBITS;
	p->viewz = p->mo->z + p->viewheight;

	if(playernum == consoleplayer)
	{
		// wake up the status bar
		ST_Start();
		// wake up the heads up text
		HU_Start();
	}

	SV_SpawnPlayer(playernum, mobj->x, mobj->y, mobj->angle);

	if(cv_chasecam.value)
	{
		if(displayplayer == playernum)
			P_ResetCamera(p, &camera);
	}
	if(cv_chasecam2.value && cv_splitscreen.value)
	{
		if(secondarydisplayplayer == playernum)
			P_ResetCamera(p, &camera2);
	}

	if(!(netgame || multiplayer))
		leveltime = starposttime;
}

#define MAXHUNTEMERALDS 64
mapthing_t* huntemeralds[3][MAXHUNTEMERALDS];
int numhuntemeralds[3];

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing(mapthing_t* mthing)
{
	int i, bit;
	mobj_t* mobj;
	fixed_t x, y, z;
	subsector_t* ss;

	if(!mthing->type)
		return; // Ignore type-0 things as NOPs

	// count deathmatch start positions
	if(mthing->type == 11)
	{
		if(numdmstarts < MAX_DM_STARTS)
		{
			deathmatchstarts[numdmstarts] = mthing;
			mthing->type = 0;
			numdmstarts++;
		}
		return;
	}

	else if(mthing->type == 87) // Red CTF Starts
	{
		if(numredctfstarts < MAXPLAYERS)
		{
			redctfstarts[numredctfstarts] = mthing;
			mthing->type = 0;
			numredctfstarts++;
		}
		return;
	}

	else if(mthing->type == 89) // Blue CTF Starts
	{
		if(numbluectfstarts < MAXPLAYERS)
		{
			bluectfstarts[numbluectfstarts] = mthing;
			mthing->type = 0;
			numbluectfstarts++;
		}
		return;
	}

	else if(mthing->type == 57 || mthing->type == 84
		|| mthing->type == 44 || mthing->type == 76
		|| mthing->type == 77 || mthing->type == 47
		|| mthing->type == 2014 || mthing->type == 47
		|| mthing->type == 2007 || mthing->type == 2048
		|| mthing->type == 2010 || mthing->type == 2046
		|| mthing->type == 2047 || mthing->type == 37)
	{
		// Don't spawn hoops, wings, or rings yet!
		return;
	}

	// check for players specially
	if((mthing->type > 0 && mthing->type <= 4) || (mthing->type <= 4028 && mthing->type >= 4001))
	{
		if(mthing->type > 4000) // This screwed up some debug code in April 2003.
			mthing->type = (short)(mthing->type - 4001 + 5);

		// save spots for respawning in network games
		playerstarts[mthing->type-1] = mthing;
		return;
	}

	// Ambient sound sequences
	if(mthing->type >= 1200 && mthing->type < 1300)
		return;

	if(mthing->type != 52 && mthing->type != 53 && mthing->type != 59
		&& mthing->type != 61 && mthing->type != 62 && mthing->type != 15
		&& mthing->type != 45 && mthing->type != 46 && mthing->type != 55
		&& mthing->type != 82 && mthing->type != 85 && mthing->type != 3006)
	{
		if(gameskill == sk_baby)
			bit = 1;
		else if(gameskill >= sk_nightmare)
			bit = 4;
		else
			bit = 1<<(gameskill-1);

		if(!(mthing->options & bit))
			return;
	}

	// find which type to spawn
	for(i = 0; i < NUMMOBJTYPES; i++)
		if(mthing->type == mobjinfo[i].doomednum)
			break;

	if(i == NUMMOBJTYPES)
	{
		CONS_Printf("\2P_SpawnMapThing: Unknown type %i at (%i, %i)\n", mthing->type,
			mthing->x, mthing->y);
		return;
	}

	if(i >= MT_EMERALD1 && i <= MT_EMERALD8) // Pickupable Emeralds
	{
		if(emeralds & mobjinfo[i].speed) // You already have this emerald!
			return;
	}

	if(!(gametype == GT_MATCH || gametype == GT_CTF	|| gametype == GT_TAG)
		&& (!cv_ringslinger.value || !cv_specialrings.value))
	{
		switch(i)
		{
			case MT_HOMINGRING:
			case MT_RAILRING:
			case MT_INFINITYRING:
			case MT_AUTOMATICRING:
			case MT_EXPLOSIONRING:
				return;
			default:
				break;
		}
	}

	// Hunt should only work in Cooperative.
	if(gametype != GT_COOP)
	{
		switch(i)
		{
			case MT_EMERHUNT:
			case MT_EMESHUNT:
			case MT_EMETHUNT:
				return;
			default:
				break;
		}
	}
	else if(i == MT_EMERHUNT)
	{
		if(numhuntemeralds[0] < MAXHUNTEMERALDS)
			huntemeralds[0][numhuntemeralds[0]++] = mthing;
		return;
	}
	else if(i == MT_EMESHUNT)
	{
		if(numhuntemeralds[1] < MAXHUNTEMERALDS)
			huntemeralds[1][numhuntemeralds[1]++] = mthing;
		return;
	}
	else if(i == MT_EMETHUNT)
	{
		if(numhuntemeralds[2] < MAXHUNTEMERALDS)
			huntemeralds[2][numhuntemeralds[2]++] = mthing;
		return;
	}

	if(gametype == GT_MATCH) // No enemies in match mode
		if((mobjinfo[i].flags & MF_ENEMY) || (mobjinfo[i].flags & MF_BOSS))
			return;

	// Set powerup boxes to user settings for race.
	if(gametype == GT_RACE)
	{
		if(cv_raceitemboxes.value) // not Normal
		{
			if(mobjinfo[i].flags & MF_MONITOR)
			{
				if(cv_raceitemboxes.value == 1) // Random
					i = MT_QUESTIONBOX;
				else if(cv_raceitemboxes.value == 2) // Teleports
					i = MT_MIXUPBOX;
				else if(cv_raceitemboxes.value == 3) // None
					return; // Don't spawn!
			}
		}
		else // cv_raceitemboxes.value == 0, Normal
		{
			if(i == MT_REDTV && !cv_fireshield.value) return;
			if(i == MT_YELLOWTV && !cv_ringshield.value) return;
			if(i == MT_BLUETV && !cv_watershield.value) return;
			if(i == MT_BLACKTV && !cv_bombshield.value) return;
			if(i == MT_WHITETV && !cv_jumpshield.value) return;
			if(i == MT_SNEAKERTV && !cv_supersneakers.value) return;
			if(i == MT_PRUP && !cv_1up.value) return;
			if(i == MT_SUPERRINGBOX && !cv_superring.value) return;
			if(i == MT_GREYRINGBOX && !cv_silverring.value) return;
			if(i == MT_INV && !cv_invincibility.value) return;
			if(i == MT_EGGMANBOX && !cv_eggmanbox.value) return;
			if(i == MT_MIXUPBOX && !cv_teleporters.value) return;
			if(i == MT_QUESTIONBOX && !cv_questionbox.value) return;
		}
	}

	// Set powerup boxes to user settings for other netplay modes
	else if(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
		|| gametype == GT_CHAOS)
	{
		if(cv_matchboxes.value) // not Normal
		{
			if(cv_matchboxes.value == 1) // Random
			{
				if(mobjinfo[i].flags & MF_MONITOR)
					i = MT_QUESTIONBOX;
			}
			else if(cv_matchboxes.value == 3) // Don't spawn
			{
				if(mobjinfo[i].flags & MF_MONITOR)
					return;
			}
			else // cv_matchboxes.value == 2, Non-Random
			{
				if(i == MT_REDTV && !cv_fireshield.value) return;
				if(i == MT_YELLOWTV && !cv_ringshield.value) return;
				if(i == MT_BLUETV && !cv_watershield.value) return;
				if(i == MT_BLACKTV && !cv_bombshield.value) return;
				if(i == MT_WHITETV && !cv_jumpshield.value) return;
				if(i == MT_SNEAKERTV && !cv_supersneakers.value) return;
				if(i == MT_PRUP) return; // no meaning in CTF/tag/chaos/match
				if(i == MT_SUPERRINGBOX && !cv_superring.value) return;
				if(i == MT_GREYRINGBOX && !cv_silverring.value) return;
				if(i == MT_INV && !cv_invincibility.value) return;
				if(i == MT_EGGMANBOX && !cv_eggmanbox.value) return;
				if(i == MT_MIXUPBOX && !cv_teleporters.value) return;
				if(i == MT_QUESTIONBOX) return; // don't spawn in Non-Random

				mthing->options &= ~MTF_AMBUSH; // no random respawning!
			}
		}
		else // cv_matchboxes.value == 0, Normal
		{
			if(i == MT_REDTV && !cv_fireshield.value) return;
			if(i == MT_YELLOWTV && !cv_ringshield.value) return;
			if(i == MT_BLUETV && !cv_watershield.value) return;
			if(i == MT_BLACKTV && !cv_bombshield.value) return;
			if(i == MT_WHITETV && !cv_jumpshield.value) return;
			if(i == MT_SNEAKERTV && !cv_supersneakers.value) return;
			if(i == MT_PRUP) return; // no meaning in CTF/tag/chaos/match
			if(i == MT_SUPERRINGBOX && !cv_superring.value) return;
			if(i == MT_GREYRINGBOX && !cv_silverring.value) return;
			if(i == MT_INV && !cv_invincibility.value) return;
			if(i == MT_EGGMANBOX && !cv_eggmanbox.value) return;
			if(i == MT_MIXUPBOX && !cv_teleporters.value) return;
			if(i == MT_QUESTIONBOX && !cv_questionbox.value) return;
		}
	}

	if(i == MT_SIGN && gametype != GT_COOP && gametype != GT_RACE)
		return; // Don't spawn the level exit sign when it isn't needed.

	if((i == MT_SUPERRINGBOX || i == MT_GREYRINGBOX || i == MT_REDTV
		|| i == MT_YELLOWTV || i == MT_BLUETV || i == MT_BLACKTV || i == MT_WHITETV)
		&& gameskill == sk_insane && !(gamemap >= sstage_start && gamemap <= sstage_end))
	{
		// Don't have rings/shields in Ultimate mode
		return;
	}

	if((i == MT_BLUEFLAG || i == MT_REDFLAG) && gametype != GT_CTF)
		return; // Don't spawn flags if you aren't in CTF Mode!

	if(i == MT_EMMY && (tokenbits == 30 || tokenlist & (1<<tokenbits) || gametype != GT_COOP))
		return; // you already got this token, or there are too many, or the gametype's not right

	// spawn it
	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	ss = R_PointInSubsector(x, y);

	if(mthing->type != 52 && mthing->type != 53 && mthing->type != 59
		&& mthing->type != 61 && mthing->type != 62 && mthing->type != 15
		&& mthing->type != 45 && mthing->type != 46 && mthing->type != 55
		&& mthing->type != 82 && mthing->type != 85)
	{
		if(mobjinfo[i].flags & MF_SPAWNCEILING)
		{
			// Move down with object heights
			if(mthing->options >> 4)
				z = ss->sector->ceilingheight - ((mthing->options >> 4) << FRACBITS)
					- mobjinfo[i].height; // Subtract the height too!
			else
				z = ONCEILINGZ;
		}
		else if(i == MT_SPECIALSPIKEBALL
			|| i == MT_HOMINGRING || i == MT_RAILRING
			|| i == MT_INFINITYRING || i == MT_AUTOMATICRING
			|| i == MT_EXPLOSIONRING)
		{
			z = ss->sector->floorheight;

			if(mthing->options & MTF_AMBUSH) // Special flag for rings
				z += 32*FRACUNIT;
			if(mthing->options >> 4)
				z += (mthing->options >> 4)*FRACUNIT;
		}
		else if(mthing->options >> 4)
			z = ss->sector->floorheight + ((mthing->options >> 4) << FRACBITS);
		else if(i == MT_CRAWLACOMMANDER || i == MT_DETON || i == MT_JETTBOMBER || i == MT_JETTGUNNER || i == MT_EGGMOBILE || i == MT_EGGMOBILE2)
			z = ss->sector->floorheight + 33*FRACUNIT;
		else if(i == MT_GOLDBUZZ || i == MT_REDBUZZ)
			z = ss->sector->floorheight + 288*FRACUNIT;
		else
			z = ONFLOORZ;

		if(z == ONFLOORZ)
			mthing->z = 0;
		else
			mthing->z = (short)(z >> FRACBITS);
	}
	else
		z = ONFLOORZ;

	mobj = P_SpawnMobj(x, y, z, i);
	mobj->spawnpoint = mthing;

	if(maptol & TOL_ADVENTURE && mobj->flags & MF_MONITOR)
	{
		mobj->flags |= MF_SPECIAL;
		mobj->flags &= ~MF_SOLID;
	}

	if(mobj->type == MT_FAN)
	{
		if(mthing->angle)
			mobj->health = mthing->angle;
		else
		{
			mobj->health = (int)(ss->sector->ceilingheight - .25f*(ss->sector->ceilingheight - ss->sector->floorheight));
			mobj->health -= ss->sector->floorheight;
			mobj->health >>= FRACBITS;
		}
	}
	else if(mobj->type == MT_GFZFISH || mobj->type == MT_PUMA) // Custom jump height
	{
		if(mthing->angle)
			mobj->threshold = mthing->angle;
		else
			mobj->threshold = 44;
	}

	if(mobj->flags & MF_BOSS)
	{
		if(mthing->options & MTF_MULTI) // No egg trap for this boss
			mobj->flags2 |= MF2_BOSSNOTRAP;

		// Special case - Bosses use the 5th bit, so rip it off!!!
		if(mthing->options >> 5)
			z = R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 5) << FRACBITS);

		mthing->z = (short)(z >> FRACBITS);
	}
	else if(mobj->type == MT_EGGCAPSULE)
	{
		mobj->health = mthing->angle & 1023;

		if(mthing->angle >> 10)
			mobj->threshold = mthing->angle >> 10;
	}

	// Special condition for the 2nd boss.
	if(mobj->type == MT_EGGMOBILE2)
		mobj->watertop = mobj->info->speed;
	else if(mobj->type == MT_CHAOSSPAWNER)
	{
		if(gametype != GT_CHAOS)
			return;
		mobj->fuse = P_Random()*2;
	}

	if(mthing->type == 52 || mthing->type == 53 || mthing->type == 59
		|| mthing->type == 61 || mthing->type == 62 || mthing->type == 15
		|| mthing->type == 45 || mthing->type == 46 || mthing->type == 55
		|| mthing->type == 82 || mthing->type == 85)
	{
		if(mthing->options >> 10)
			mobj->threshold = mthing->options >> 10;

		mobj->health = mthing->options & 1023;
	}
	else if(i == MT_EMMY)
	{
		mobj->health = 1 << tokenbits++;
		P_SpawnMobj(x, y, z, MT_TOKEN);
	}
	else if(i == MT_EGGMOBILE && mthing->options & MTF_AMBUSH)
	{
		mobj_t* spikemobj;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		spikemobj->target = mobj;
		spikemobj->angle = 0;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		spikemobj->target = mobj;
		spikemobj->angle = ANG90;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		spikemobj->target = mobj;
		spikemobj->angle = ANG180;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		spikemobj->target = mobj;
		spikemobj->angle = ANG270;
	}
	else if(i == MT_STARPOST)
	{
		mobj->health = (mthing->options & 31)+1;
	}

	// special push/pull stuff
	if(i == MT_PUSH || i == MT_PULL)
	{
		if(mthing->options & MTF_AMBUSH)
			mobj->health = 1; // If ambush is set, push using XYZ
		else if(mthing->options & MTF_MULTI)
			mobj->health = 2; // If multi is set, fade using XY
		else
			mobj->health = 0; // Default behaviour: pushing uses XY, fading uses XYZ
	}

	if(mobj->tics > 0)
		mobj->tics = 1 + (P_Random() % mobj->tics);
	if(mobj->flags & MF_COUNTITEM)
		totalitems++;

	mobj->angle = mthing->angle*ANGLE_1;
	if(mthing->options & MTF_AMBUSH)
	{
		switch(i)
		{
			case MT_YELLOWDIAG:
			case MT_YELLOWDIAGDOWN:
			case MT_REDDIAG:
			case MT_REDDIAGDOWN:
				mobj->angle += ANG45/2;
				break;
			default:
				break;
		}

		if(mobj->flags & MF_PUSHABLE)
		{
			mobj->flags &= ~MF_PUSHABLE;
			mobj->flags2 |= MF2_STANDONME;
		}

		if(mthing->type != 52 && mthing->type != 53 && mthing->type != 59
			&& mthing->type != 61 && mthing->type != 62 && mthing->type != 15
			&& mthing->type != 45 && mthing->type != 46 && mthing->type != 82
			&& mthing->type != 85 && mthing->type != 3006)
			mobj->flags |= MF_AMBUSH;
	}

	// Pushables bounce and slide coolly with multi flag set
	if((mthing->options & MTF_MULTI) && (mobj->flags & MF_PUSHABLE))
	{
		mobj->flags2 |= MF2_SLIDEPUSH;
		mobj->flags |= MF_BOUNCE;
	}

	mthing->mobj = mobj;
}

void P_SpawnHoopsAndRings(mapthing_t* mthing)
{
	mobj_t* mobj;
	int r;
	fixed_t x, y, z;

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	if(mthing->type == 57) // NiGHTS hoop!
	{
		int i;
		TVector v;
		TVector* res;
		fixed_t finalx, finaly, finalz;
		mobj_t* hoopcenter;
		mobj_t* nextmobj = NULL;
		mobj_t* axis = NULL;
		short closestangle;
		fixed_t mthingx, mthingy, mthingz;
		degenmobj_t xypos;

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		mthingz = mthing->options << FRACBITS;

		hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_HOOPCENTER);

		hoopcenter->flags2 |= MF2_NOTHINK;

		axis = P_GetClosestAxis(hoopcenter);

		if(!axis)
		{
			CONS_Printf("You forgot to put axis points in the map!\n");
			return;
		}

		xypos.x = mthingx;
		xypos.y = mthingy;

		P_GimmeAxisXYPos(axis, &xypos);

		mthingx = xypos.x;
		mthingy = xypos.y;

		mthingz += R_PointInSubsector(mthingx, mthingy)->sector->floorheight;

		hoopcenter->z = mthingz - hoopcenter->height/2;

		P_UnsetThingPosition(hoopcenter);
		hoopcenter->x = mthingx;
		hoopcenter->y = mthingy;
		P_SetThingPosition(hoopcenter);

		closestangle = (short)(R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)/ANGLE_1);

		hoopcenter->movedir = mthing->angle;
		hoopcenter->movecount = closestangle;

		// Create the hoop!
		for(i = 0; i < 32; i++)
		{
			v[0] = (float)(cos(i*11.25*deg2rad) * 96.0f);
			v[1] = 0;
			v[2] = (float)(sin(i*11.25*deg2rad) * 96.0f);
			v[3] = 1;

			res = VectorMatrixMultiply(v, *RotateXMatrix((float)(mthing->angle*deg2rad)));
			memcpy(&v, res, sizeof(v));
			res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
			memcpy(&v, res, sizeof(v));

			finalx = mthingx + double2fixed(v[0]);
			finaly = mthingy + double2fixed(v[1]);
			finalz = mthingz + double2fixed(v[2]);

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOP);
			mobj->z -= mobj->height/2;
			mobj->target = hoopcenter; // Link the sprite to the center.

			if(xmasmode && (i&1))
				mobj->flags = (mobj->flags & ~MF_TRANSLATION)
						 | ((MAXSKINCOLORS-1)<<MF_TRANSSHIFT); // Yellow

			// Link all the sprites in the hoop together
			if(nextmobj)
			{
				mobj->bprev = nextmobj;
				mobj->bprev->bnext = mobj;
			}

			nextmobj = mobj;
		}

		// Create the collision detectors!
		for(i = 0; i < 16; i++)
		{
			v[0] = (float)(cos(i*22.5*deg2rad) * 32.0f);
			v[1] = 0;
			v[2] = (float)(sin(i*22.5*deg2rad) * 32.0f);
			v[3] = 1;
			res = VectorMatrixMultiply(v, *RotateXMatrix((float)(mthing->angle*deg2rad)));
			memcpy(&v, res, sizeof(v));
			res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
			memcpy(&v, res, sizeof(v));

			finalx = mthingx + double2fixed(v[0]);
			finaly = mthingy + double2fixed(v[1]);
			finalz = mthingz + double2fixed(v[2]);

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOPCOLLIDE);
			mobj->z -= mobj->height/2;

			// Link all the collision sprites together.
			mobj->bprev = nextmobj;
			mobj->bprev->bnext = mobj;

			nextmobj = mobj;
		}
		// Create the collision detectors!
		for(i = 0; i < 16; i++)
		{
			v[0] = (float)(cos(i*22.5*deg2rad) * 64.0f);
			v[1] = 0;
			v[2] = (float)(sin(i*22.5*deg2rad) * 64.0f);
			v[3] = 1;
			res = VectorMatrixMultiply(v, *RotateXMatrix((float)(mthing->angle*deg2rad)));
			memcpy(&v, res, sizeof(v));
			res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
			memcpy(&v, res, sizeof(v));

			finalx = mthingx + double2fixed(v[0]);
			finaly = mthingy + double2fixed(v[1]);
			finalz = mthingz + double2fixed(v[2]);

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOPCOLLIDE);
			mobj->z -= mobj->height/2;

			// Link all the collision sprites together.
			mobj->bprev = nextmobj;
			mobj->bprev->bnext = mobj;

			nextmobj = mobj;
		}
		return;
	}
	else if(mthing->type == 37) // Wing logo item.
	{
		if(mthing->options >> 4)
			mthing->z = (short)((R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) >> FRACBITS);
		else
			mthing->z = (short)(R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight >> FRACBITS);

		mobj = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS,mthing->z << FRACBITS, MT_NIGHTSWING);
		mobj->spawnpoint = mthing;

		if(mobj->tics > 0)
			mobj->tics = 1 + (P_Random() % mobj->tics);
		mobj->angle = ANGLE_1*mthing->angle;
		mobj->flags |= MF_AMBUSH;
		mthing->mobj = mobj;
	}
	else if(mthing->type == 2048) // A ring of wing items (NiGHTS stuff)
	{
		int i;
		TVector v;
		TVector* res;
		fixed_t finalx, finaly, finalz;
		mobj_t* hoopcenter;
		mobj_t* axis = NULL;
		short closestangle;
		fixed_t mthingx, mthingy, mthingz;
		degenmobj_t xypos;

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		if(mthing->options >> 4)
			mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
		else
			mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

		hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

		axis = P_GetClosestAxis(hoopcenter);

		P_RemoveMobj(hoopcenter);

		if(!axis)
		{
			CONS_Printf("You forgot to put axis points in the map!\n");
			return;
		}

		xypos.x = mthingx;
		xypos.y = mthingy;

		P_GimmeAxisXYPos(axis, &xypos);

		mthingx = xypos.x;
		mthingy = xypos.y;

		closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

		// Create the hoop!
		for(i = 0; i < 8; i++)
		{
			v[0] = (float)(cos(i*45*deg2rad) * 96);
			v[1] = 0;
			v[2] = (float)(sin(i*45*deg2rad) * 96);
			v[3] = 1;

			res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
			memcpy(&v, res, sizeof(v));

			finalx = mthingx + double2fixed(v[0]);
			finaly = mthingy + double2fixed(v[1]);
			finalz = mthingz + double2fixed(v[2]);

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_NIGHTSWING);
			mobj->z -= mobj->height/2;
		}
		return;
	}
	else if(mthing->type == 2010) // A BIGGER ring of wing items (NiGHTS stuff)
	{
		int i;
		TVector v;
		TVector* res;
		fixed_t finalx, finaly, finalz;
		mobj_t* hoopcenter;
		mobj_t* axis = NULL;
		short closestangle;
		fixed_t mthingx, mthingy, mthingz;
		degenmobj_t xypos;

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		if(mthing->options >> 4)
			mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
		else
			mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

		hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

		axis = P_GetClosestAxis(hoopcenter);

		P_RemoveMobj(hoopcenter);

		if(!axis)
		{
			CONS_Printf("You forgot to put axis points in the map!\n");
			return;
		}

		xypos.x = mthingx;
		xypos.y = mthingy;

		P_GimmeAxisXYPos(axis, &xypos);

		mthingx = xypos.x;
		mthingy = xypos.y;

		closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

		// Create the hoop!
		for(i = 0; i < 16; i++)
		{
			v[0] = (float)(cos(i*45*deg2rad) * 192);
			v[1] = 0;
			v[2] = (float)(sin(i*45*deg2rad) * 192);
			v[3] = 1;

			res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
			memcpy(&v, res, sizeof(v));

			finalx = mthingx + double2fixed(v[0]);
			finaly = mthingy + double2fixed(v[1]);
			finalz = mthingz + double2fixed(v[2]);

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_NIGHTSWING);
			mobj->z -= mobj->height/2;
		}
		return;
	}
	else
	{
		if(gameskill >= sk_insane && !((gamemap >= sstage_start && gamemap <= sstage_end) || (maptol & TOL_NIGHTS))) // No rings in Ultimate!
			return;

		if(mthing->type == 2014) // Your basic ring.
		{
			int bit;
			if(gameskill == sk_baby)
				bit = 1;
			else if(gameskill >= sk_nightmare)
				bit = 4;
			else
				bit = 1<<(gameskill-1);

			if(!(mthing->options & bit))
				return;

			if(mthing->options & MTF_AMBUSH) // Special flag for rings
			{
				mthing->z = (short)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 32*FRACUNIT) >> FRACBITS);
				mthing->z = (short)(mthing->z + (mthing->options >> 4));
			}
			else if(mthing->options >> 4)
				mthing->z = (short)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) >> FRACBITS);
			else
				mthing->z = (short)(R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight >> FRACBITS);

			mobj = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS,mthing->z << FRACBITS, MT_RING);
			mobj->spawnpoint = mthing;

			if(mobj->tics > 0)
				mobj->tics = 1 + (P_Random() % mobj->tics);
			mobj->angle = ANGLE_1*mthing->angle;
			mobj->flags |= MF_AMBUSH;
			mthing->mobj = mobj;
		}
		else if(mthing->type == 10005) // Your basic coin.
		{
			int bit;

			if(gameskill == sk_baby)
				bit = 1;
			else if(gameskill >= sk_nightmare)
				bit = 4;
			else
				bit = 1<<(gameskill-1);

			if(!(mthing->options & bit))
				return;

			if(mthing->options & MTF_AMBUSH) // Special flag for rings
			{
				mthing->z = (short)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 32*FRACUNIT) >> FRACBITS);
				mthing->z = (short)(mthing->z + (mthing->options >> 4));
			}
			else if(mthing->options >> 4)
				mthing->z = (short)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) >> FRACBITS);
			else
				mthing->z = (short)(R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight >> FRACBITS);

			mobj = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS,mthing->z << FRACBITS, MT_COIN);
			mobj->spawnpoint = mthing;

			if(mobj->tics > 0)
				mobj->tics = 1 + (P_Random() % mobj->tics);
			mobj->angle = ANGLE_1*mthing->angle;
			mobj->flags |= MF_AMBUSH;
			mthing->mobj = mobj;
		}
		else if(mthing->type == 84)
		{
			for(r = 1; r <= 5; r++)
			{
				if(mthing->options >> 4)
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) + 64*FRACUNIT*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 64*FRACUNIT*r;

				mobj = P_SpawnMobj(x, y, z, MT_RING);

				if(mobj->tics > 0)
					mobj->tics = 1 + (P_Random() % mobj->tics);

				mobj->angle = ANGLE_1*mthing->angle;
				if(mthing->options & MTF_AMBUSH)
					mobj->flags |= MF_AMBUSH;
			}
		}
		else if(mthing->type == 44) // Vertical Rings - Stack of 5 (suitable for Red Spring)
		{
			for(r = 1; r <= 5; r++)
			{
				if(mthing->options >> 4)
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) + 128*FRACUNIT*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 128*FRACUNIT*r;
				mobj = P_SpawnMobj(x, y, z, MT_RING);

				if(mobj->tics > 0)
					mobj->tics = 1 + (P_Random() % mobj->tics);

				mobj->angle = ANGLE_1*mthing->angle;
				if(mthing->options & MTF_AMBUSH)
					mobj->flags |= MF_AMBUSH;
			}
		}
		else if(mthing->type == 76) // Diagonal rings (5)
		{
			angle_t angle = ANG45 * (mthing->angle/45);
			angle >>= ANGLETOFINESHIFT;

			for(r = 1; r <= 5; r++)
			{
				x += FixedMul(64*FRACUNIT, finecosine[angle]);
				y += FixedMul(64*FRACUNIT, finesine[angle]);
				if(mthing->options >> 4)
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) + 64*FRACUNIT*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 64*FRACUNIT*r;
				mobj = P_SpawnMobj(x, y, z, MT_RING);

				if(mobj->tics > 0)
					mobj->tics = 1 + (P_Random() % mobj->tics);

				mobj->angle = ANGLE_1*mthing->angle;
				if(mthing->options & MTF_AMBUSH)
					mobj->flags |= MF_AMBUSH;
			}
		}
		else if(mthing->type == 77) // Diagonal rings (10)
		{
			angle_t angle = ANG45 * (mthing->angle/45);
			angle >>= ANGLETOFINESHIFT;

			for(r = 1; r <= 10; r++)
			{
				x += FixedMul(64*FRACUNIT, finecosine[angle]);
				y += FixedMul(64*FRACUNIT, finesine[angle]);
				if(mthing->options >> 4)
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> 4) << FRACBITS)) + 64*FRACUNIT*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 64*FRACUNIT*r;
				mobj = P_SpawnMobj(x, y, z, MT_RING);

				if(mobj->tics > 0)
					mobj->tics = 1 + (P_Random() % mobj->tics);

				mobj->angle = ANGLE_1*mthing->angle;
				if(mthing->options & MTF_AMBUSH)
					mobj->flags |= MF_AMBUSH;
			}
		}
		else if(mthing->type == 47) // A ring of rings (NiGHTS stuff)
		{
			int i;
			TVector v;
			TVector* res;
			fixed_t finalx, finaly, finalz;
			mobj_t* hoopcenter;
			mobj_t* axis = NULL;
			short closestangle;
			fixed_t mthingx, mthingy, mthingz;
			degenmobj_t xypos;

			mthingx = mthing->x << FRACBITS;
			mthingy = mthing->y << FRACBITS;

			if(mthing->options >> 4)
				mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
			else
				mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

			hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

			axis = P_GetClosestAxis(hoopcenter);

			P_RemoveMobj(hoopcenter);

			if(!axis)
			{
				CONS_Printf("You forgot to put axis points in the map!\n");
				return;
			}

			xypos.x = mthingx;
			xypos.y = mthingy;

			P_GimmeAxisXYPos(axis, &xypos);

			mthingx = xypos.x;
			mthingy = xypos.y;

			closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

			// Create the hoop!
			for(i = 0; i < 8; i++)
			{
				v[0] = (float)(cos(i*45*deg2rad) * 96);
				v[1] = 0;
				v[2] = (float)(sin(i*45*deg2rad) * 96);
				v[3] = 1;

				res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
				memcpy(&v, res, sizeof(v));

				finalx = mthingx + double2fixed(v[0]);
				finaly = mthingy + double2fixed(v[1]);
				finalz = mthingz + double2fixed(v[2]);

				mobj = P_SpawnMobj(finalx, finaly, finalz, MT_RING);
				mobj->z -= mobj->height/2;
			}

			return;
		}
		else if(mthing->type == 2007) // A BIGGER ring of rings (NiGHTS stuff)
		{
			int i;
			TVector v;
			TVector* res;
			fixed_t finalx, finaly, finalz;
			mobj_t* hoopcenter;
			mobj_t* axis = NULL;
			short closestangle;
			fixed_t mthingx, mthingy, mthingz;
			degenmobj_t xypos;

			mthingx = mthing->x << FRACBITS;
			mthingy = mthing->y << FRACBITS;

			if(mthing->options >> 4)
				mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
			else
				mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

			hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

			axis = P_GetClosestAxis(hoopcenter);

			P_RemoveMobj(hoopcenter);

			if(!axis)
			{
				CONS_Printf("You forgot to put axis points in the map!\n");
				return;
			}

			xypos.x = mthingx;
			xypos.y = mthingy;

			P_GimmeAxisXYPos(axis, &xypos);

			mthingx = xypos.x;
			mthingy = xypos.y;

			closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

			// Create the hoop!
			for(i = 0; i < 16; i++)
			{
				v[0] = (float)(cos(i*22.5*deg2rad) * 192.0f);
				v[1] = 0;
				v[2] = (float)(sin(i*22.5*deg2rad) * 192.0f);
				v[3] = 1;

				res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
				memcpy(&v, res, sizeof(v));

				finalx = mthingx + double2fixed(v[0]);
				finaly = mthingy + double2fixed(v[1]);
				finalz = mthingz + double2fixed(v[2]);

				mobj = P_SpawnMobj(finalx, finaly, finalz, MT_RING);
				mobj->z -= mobj->height/2;
			}

			return;
		}
		else if(mthing->type == 2046) // A ring of rings and wings (alternating) (NiGHTS stuff)
		{
			int i;
			TVector v;
			TVector* res;
			fixed_t finalx, finaly, finalz;
			mobj_t* hoopcenter;
			mobj_t* axis = NULL;
			short closestangle;
			fixed_t mthingx, mthingy, mthingz;
			degenmobj_t xypos;

			mthingx = mthing->x << FRACBITS;
			mthingy = mthing->y << FRACBITS;

			if(mthing->options >> 4)
				mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
			else
				mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

			hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

			axis = P_GetClosestAxis(hoopcenter);

			P_RemoveMobj(hoopcenter);

			if(!axis)
			{
				CONS_Printf("You forgot to put axis points in the map!\n");
				return;
			}

			xypos.x = mthingx;
			xypos.y = mthingy;

			P_GimmeAxisXYPos(axis, &xypos);

			mthingx = xypos.x;
			mthingy = xypos.y;

			closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

			// Create the hoop!
			for(i = 0; i < 8; i++)
			{
				v[0] = (float)(cos(i*45*deg2rad) * 96);
				v[1] = 0;
				v[2] = (float)(sin(i*45*deg2rad) * 96);
				v[3] = 1;

				res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
				memcpy(&v, res, sizeof(v));

				finalx = mthingx + double2fixed(v[0]);
				finaly = mthingy + double2fixed(v[1]);
				finalz = mthingz + double2fixed(v[2]);

				if(i & 1)
					mobj = P_SpawnMobj(finalx, finaly, finalz, MT_RING);
				else
					mobj = P_SpawnMobj(finalx, finaly, finalz, MT_NIGHTSWING);

				mobj->z -= mobj->height/2;
			}

			return;
		}
		else if(mthing->type == 2047) // A BIGGER ring of rings and wings (alternating) (NiGHTS stuff)
		{
			int i;
			TVector v;
			TVector* res;
			fixed_t finalx, finaly, finalz;
			mobj_t* hoopcenter;
			mobj_t* axis = NULL;
			short closestangle;
			fixed_t mthingx, mthingy, mthingz;
			degenmobj_t xypos;

			mthingx = mthing->x << FRACBITS;
			mthingy = mthing->y << FRACBITS;

			if(mthing->options >> 4)
				mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> 4) << FRACBITS));
			else
				mthingz = R_PointInSubsector(mthingx, mthing->y << FRACBITS)->sector->floorheight;

			hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_DISS);

			axis = P_GetClosestAxis(hoopcenter);

			P_RemoveMobj(hoopcenter);

			if(!axis)
			{
				CONS_Printf("You forgot to put axis points in the map!\n");
				return;
			}

			xypos.x = mthingx;
			xypos.y = mthingy;

			P_GimmeAxisXYPos(axis, &xypos);

			mthingx = xypos.x;
			mthingy = xypos.y;

			closestangle = (short)((R_PointToAngle2(mthingx, mthingy, axis->x, axis->y)+ANG90)/ANGLE_1);

			// Create the hoop!
			for(i = 0; i < 16; i++)
			{
				v[0] = (float)(cos(i*22.5*deg2rad) * 192.0f);
				v[1] = 0;
				v[2] = (float)(sin(i*22.5*deg2rad) * 192.0f);
				v[3] = 1;

				res = VectorMatrixMultiply(v, *RotateZMatrix((float)(closestangle*deg2rad)));
				memcpy(&v, res, sizeof(v));

				finalx = mthingx + double2fixed(v[0]);
				finaly = mthingy + double2fixed(v[1]);
				finalz = mthingz + double2fixed(v[2]);

				if(i & 1)
					mobj = P_SpawnMobj(finalx, finaly, finalz, MT_RING);
				else
					mobj = P_SpawnMobj(finalx, finaly, finalz, MT_NIGHTSWING);
				mobj->z -= mobj->height/2;
			}

			return;
		}
	}
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_CheckMissileSpawn
// Moves the missile forward a bit and possibly explodes it right there.
//
boolean P_CheckMissileSpawn(mobj_t* th)
{
	th->tics -= P_Random()&3;
	if(th->tics < 1)
		th->tics = 1;

	// move a little forward so an angle can be computed if it immediately explodes
	th->x += th->momx>>1;
	th->y += th->momy>>1;
	th->z += th->momz>>1;

	if(!P_TryMove(th, th->x, th->y, true))
	{
		P_ExplodeMissile(th);
		return false;
	}
	return true;
}

//
// P_SpawnXYZMissile
//
// Spawns missile at specific coords
//
mobj_t* P_SpawnXYZMissile(mobj_t* source, mobj_t* dest, mobjtype_t type,
	fixed_t x, fixed_t y, fixed_t z)
{
	mobj_t* th;
	angle_t an;
	int dist;

#ifdef PARANOIA
	if(!source)
		I_Error("P_SpawnXYZMissile: no source");
	if(!dest)
		I_Error("P_SpawnXYZMissile: no dest");
#endif

	th = P_SpawnMobj(x, y, z, type);

	if(th->info->seesound)
		S_StartSound(th, th->info->seesound);

	th->target = source; // where it came from
	an = R_PointToAngle2(x, y, dest->x, dest->y);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(th->info->speed, finecosine[an]);
	th->momy = FixedMul(th->info->speed, finesine[an]);

	dist = P_AproxDistance(dest->x - x, dest->y - y);
	dist = dist / th->info->speed;

	if(dist < 1)
		dist = 1;

	th->momz = (dest->z - z) / dist;

	dist = P_CheckMissileSpawn(th);
	return dist ? th : NULL;
}

//
// P_SpawnMissile
//
mobj_t* P_SpawnMissile(mobj_t* source, mobj_t* dest, mobjtype_t type)
{
	mobj_t* th;
	angle_t an;
	int dist;
	fixed_t z;

#ifdef PARANOIA
	if(!source)
		I_Error("P_SpawnMissile: no source");
	if(!dest)
		I_Error("P_SpawnMissile: no dest");
#endif
	switch(type)
	{
		case MT_JETTBULLET:
			if(source->type == MT_JETTGUNNER)
				z = source->z - 12*FRACUNIT;
			else
				z = source->z + source->height/2;
			break;
		case MT_TURRETLASER:
			z = source->z + source->height/2;
		default:
			z = source->z + 32*FRACUNIT;
			break;
	}

	th = P_SpawnMobj(source->x, source->y, z, type);

	if(th->info->seesound)
		S_StartSound(th, th->info->seesound);

	th->target = source; // where it came from

	if(type == MT_TURRETLASER) // More accurate!
		an = R_PointToAngle2(source->x, source->y,
			dest->x + (dest->momx*(fixed_t)(3*gameskill)),
			dest->y + (dest->momy*(fixed_t)(3*gameskill)));
	else
		an = R_PointToAngle2(source->x, source->y, dest->x, dest->y);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(th->info->speed, finecosine[an]);
	th->momy = FixedMul(th->info->speed, finesine[an]);

	if(type == MT_TURRETLASER) // More accurate!
		dist = P_AproxDistance(dest->x+(dest->momx*(fixed_t)(3*gameskill)) - source->x, dest->y+(dest->momy*(fixed_t)(3*gameskill)) - source->y);
	else
		dist = P_AproxDistance(dest->x - source->x, dest->y - source->y);

	dist = dist / th->info->speed;

	if(dist < 1)
		dist = 1;

	if(type == MT_TURRETLASER) // More accurate!
		th->momz = (dest->z + (dest->momz*(fixed_t)(3*gameskill)) - source->z) / dist;
	else
		th->momz = (dest->z - source->z) / dist;

	dist = P_CheckMissileSpawn(th);
	return dist ? th : NULL;
}

//
// P_SPMAngle
// Tries to aim at a nearby monster
//
mobj_t* P_SPMAngle(mobj_t* source, mobjtype_t type, angle_t angle)
{
	mobj_t* th;
	angle_t an;
	fixed_t x, y, z, slope = 0;

	// angle at which you fire, is player angle
	an = angle;

	if(source->player->autoaim_toggle && cv_allowautoaim.value
		&& !source->player->powers[pw_railring])
	{
		// see which target is to be aimed at
		slope = P_AimLineAttack(source, an, 16*64*FRACUNIT);

		if(!linetarget)
		{
			an += 1<<26;
			slope = P_AimLineAttack(source, an, 16*64*FRACUNIT);

			if(!linetarget)
			{
				an -= 2<<26;
				slope = P_AimLineAttack(source, an, 16*64*FRACUNIT);
			}
			if(!linetarget)
			{
				an = angle;
				slope = 0;
			}
		}
	}

	// if not autoaim, or if the autoaim didn't aim something, use the mouseaiming
	if((!(source->player->autoaim_toggle && cv_allowautoaim.value)
		|| (!linetarget)) || source->player->powers[pw_railring])
	{
		slope = AIMINGTOSLOPE(source->player->aiming);
	}

	x = source->x;
	y = source->y;
	z = source->z + source->height/3;

	th = P_SpawnMobj(x, y, z, type);

	if(th->info->seesound)
		S_StartSound(th, th->info->seesound);

	th->target = source;

	th->angle = an;
	th->momx = FixedMul(th->info->speed, finecosine[an>>ANGLETOFINESHIFT]);
	th->momy = FixedMul(th->info->speed, finesine[an>>ANGLETOFINESHIFT]);

	th->momx = FixedMul(th->momx,finecosine[source->player->aiming>>ANGLETOFINESHIFT]);
	th->momy = FixedMul(th->momy,finecosine[source->player->aiming>>ANGLETOFINESHIFT]);
	th->momz = FixedMul(th->info->speed, slope);

	slope = P_CheckMissileSpawn(th);

	return slope ? th : NULL;
}
