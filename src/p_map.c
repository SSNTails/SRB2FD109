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
/// \brief Movement, collision handling
/// 
///	Shooting and aiming

#include "doomdef.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "r_state.h"
#include "r_main.h"
#include "r_sky.h"
#include "s_sound.h"

#include "r_splats.h"

#include "z_zone.h"

fixed_t tmbbox[4];
mobj_t* tmthing;
static int tmflags;
static fixed_t tmx;
static fixed_t tmy;

static precipmobj_t* tmprecipthing;
static fixed_t preciptmx;
static fixed_t preciptmy;
static fixed_t preciptmbbox[4];
static int preciptmflags;

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
boolean floatok;

fixed_t tmfloorz, tmceilingz;
static fixed_t tmdropoffz;
static ffloor_t* tmfloorff;
static ffloor_t* tmceilingff;
mobj_t* tmfloorthing; // the thing corresponding to tmfloorz or NULL if tmfloorz is from a sector

// used at P_ThingHeightClip() for moving sectors
static fixed_t tmsectorfloorz;
fixed_t tmsectorceilingz;

// turned on or off in PIT_CheckThing
boolean tmsprung;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t* ceilingline;

// set by PIT_CheckLine() for any line that stopped the PIT_CheckLine()
// that is, for any line which is 'solid'
line_t* blockingline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
size_t* spechit;
int numspechit;

msecnode_t* sector_list = NULL;
mprecipsecnode_t* precipsector_list = NULL;
static camera_t* mapcampointer;

//
// TELEPORT MOVE
//

// P_GetMoveFactor() returns the value by which the x, y
// movements are multiplied to add to player movement.

int P_GetMoveFactor(mobj_t* mo)
{
	int movefactor = ORIG_FRICTION_FACTOR;

	// If the floor is icy or muddy, it's harder to get moving. This is where
	// the different friction factors are applied to 'trying to move'. In
	// p_mobj.c, the friction factors are applied as you coast and slow down.

	int momentum, friction;

	if(!(mo->flags & (MF_NOGRAVITY | MF_NOCLIP)))
	{
		friction = mo->friction;
		if(friction == ORIG_FRICTION) // normal floor
			;
		else if(friction > ORIG_FRICTION) // ice
		{
			movefactor = mo->movefactor;
			mo->movefactor = ORIG_FRICTION_FACTOR; // reset
		}
		else // sludge
		{
			// phares 3/11/98: you start off slowly, then increase as
			// you get better footing

			momentum = P_AproxDistance(mo->momx, mo->momy);
			movefactor = mo->movefactor;
			if(momentum > MORE_FRICTION_MOMENTUM<<2)
				movefactor <<= 3;

			else if(momentum > MORE_FRICTION_MOMENTUM<<1)
				movefactor <<= 2;

			else if(momentum > MORE_FRICTION_MOMENTUM)
				movefactor <<= 1;

			mo->movefactor = ORIG_FRICTION_FACTOR; // reset
		}
	}
	return movefactor;
}

//
// P_TeleportMove
//
boolean P_TeleportMove(mobj_t* thing, fixed_t x, fixed_t y, fixed_t z)
{
	// the move is ok,
	// so link the thing into its new position
	P_UnsetThingPosition(thing);

	thing->floorz = tmfloorz;
	thing->floorff = tmfloorff;
	thing->ceilingz = tmceilingz;
	thing->ceilingff = tmceilingff;
	thing->x = x;
	thing->y = y;
	thing->z = z;

	P_SetThingPosition(thing);

	P_CheckPosition(thing, thing->x, thing->y);

	return true;
}

// =========================================================================
//                       MOVEMENT ITERATOR FUNCTIONS
// =========================================================================

static void add_spechit(line_t* ld)
{
	static int spechit_max = 0;

	if(numspechit >= spechit_max)
	{
		spechit_max = spechit_max ? spechit_max*2 : 16;
		spechit = (size_t*)realloc(spechit, sizeof(size_t) * spechit_max);
	}

	spechit[numspechit] = ld - lines;
	numspechit++;
}

static void P_DoSpring(mobj_t* spring, mobj_t* object)
{
	switch(spring->type)
	{
		case MT_YELLOWDIAG: // Yellow diagonal spring (pointing up from ground)
		case MT_REDDIAG: // Red diagonal spring (pointing up from ground)
			spring->flags &= ~MF_SOLID; // De-solidify
			object->momx = object->momy = object->momz = 0;
			P_UnsetThingPosition(object);
			object->x = spring->x;
			object->y = spring->y;
			object->z = spring->z + spring->height + 1;
			P_SetThingPosition(object);
			object->momz = spring->info->speed;
			P_InstaThrustEvenIn2D(object, spring->angle, spring->info->damage);
			P_SetMobjState(spring, spring->info->seestate);
			spring->flags |= MF_SOLID; // Re-solidify
			if(object->player)
			{
				if(!(object->player->cmd.forwardmove != 0 || object->player->cmd.sidemove != 0))
				{
					object->player->mo->angle = spring->angle;

					if(object->player == &players[consoleplayer])
						localangle = spring->angle;
					else if(cv_splitscreen.value && object->player == &players[secondarydisplayplayer])
						localangle2 = spring->angle;
				}

				P_ResetPlayer(object->player);
				P_SetPlayerMobjState(object, S_PLAY_PLG1);
			}
			break;
		case MT_YELLOWDIAGDOWN: // Yellow diagonal spring (upside-down)
		case MT_REDDIAGDOWN: // Yellow diagonal spring (upside-down)
			spring->flags &= ~MF_SOLID; // De-solidify
			object->momx = object->momy = object->momz = 0;
			P_UnsetThingPosition(object);
			object->x = spring->x;
			object->y = spring->y;
			object->z = spring->z - object->height - 1;
			P_SetThingPosition(object);
			object->momz = spring->info->speed;
			P_InstaThrustEvenIn2D(object, spring->angle, spring->info->damage);
			P_SetMobjState(spring, spring->info->seestate);
			spring->flags |= MF_SOLID; // Re-solidify
			if(object->player)
			{
				if(!(object->player->cmd.forwardmove != 0 || object->player->cmd.sidemove != 0))
				{
					object->player->mo->angle = spring->angle;
					if(object->player==&players[consoleplayer])
						localangle = spring->angle;
					else if(cv_splitscreen.value && object->player==&players[secondarydisplayplayer])
						localangle2 = spring->angle;
				}
				P_ResetPlayer(object->player);
			}
			break;
		case MT_YELLOWSPRING: // Yellow vertical spring (pointing up)
		case MT_REDSPRING: // Red vertical spring (pointing up)
		case MT_BLUESPRING: // Blue vertical spring (pointing up) Graue 01-03-2004
			object->z = spring->z + spring->height + 1;
			object->momz = spring->info->speed;
			P_SetMobjState(spring, spring->info->seestate);
			if(object->player)
			{
				boolean homing = object->player->homing;
				P_ResetPlayer(object->player);
				P_SetPlayerMobjState(object, S_PLAY_PLG1);

				if(maptol & TOL_ADVENTURE && homing) // Mimic SA
				{
					object->momx = object->momy = 0;
					P_UnsetThingPosition(object);
					object->x = spring->x;
					object->y = spring->y;
					P_SetThingPosition(object);
				}
				break;
			}
			break;
		case MT_YELLOWSPRINGDOWN: // Yellow vertical spring (pointing down)
		case MT_REDSPRINGDOWN: // Red vertical spring (pointing down)
			object->momz = spring->info->speed;
			P_SetMobjState(spring, spring->info->seestate);
			if(object->player)
			{
				boolean homing = object->player->homing;
				P_ResetPlayer(object->player);
				P_SetPlayerMobjState(object, S_PLAY_FALL1);

				if(maptol & TOL_ADVENTURE && homing) // Mimic SA
				{
					object->momx = object->momy = 0;
					P_UnsetThingPosition(object);
					object->x = spring->x;
					object->y = spring->y;
					P_SetThingPosition(object);
				}
			}
			break;
		default: // Must be a user-defined spring
			spring->flags &= ~MF_SOLID; // De-solidify
			object->momx = object->momy = object->momz = 0;
			P_UnsetThingPosition(object);
			object->x = spring->x;
			object->y = spring->y;
			object->z = spring->z;
			P_SetThingPosition(object);
			object->momz = spring->info->speed;
			P_InstaThrustEvenIn2D(object, spring->angle, spring->info->damage);
			P_SetMobjState(spring, spring->info->seestate);
			spring->flags |= MF_SOLID; // Re-solidify
			if(object->player)
			{
				if(!(object->player->cmd.forwardmove != 0 || object->player->cmd.sidemove != 0))
				{
					object->player->mo->angle = spring->angle;

					if(object->player == &players[consoleplayer])
						localangle = spring->angle;
					else if(cv_splitscreen.value && object->player == &players[secondarydisplayplayer])
						localangle2 = spring->angle;
				}

				P_ResetPlayer(object->player);
				P_SetPlayerMobjState(object, S_PLAY_PLG1);
			}
			break;
	}
}

//
// PIT_CheckThing
//
static boolean PIT_CheckThing(mobj_t* thing)
{
	fixed_t blockdist, topz, tmtopz;
	boolean solid;
	int damage = 0;

	// don't clip against self

	tmsprung = false;

	if(!tmthing || !thing || thing == tmthing || thing->state == &states[S_DISS])
		return true;

	// Don't collide with your buddies while NiGHTS-flying.
	if(tmthing->player && thing->player && maptol & TOL_NIGHTS
		&& (tmthing->player->nightsmode || thing->player->nightsmode))
		return true;

	if(tmthing->player && tmthing->player->nightsmode)
	{
		if(thing->type == MT_AXISTRANSFER)
		{
			if(!tmthing->target)
				return true;

			if(tmthing->target->threshold != thing->threshold)
				return true;

			blockdist = thing->radius + tmthing->radius;

			if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
				return true; // didn't hit it

			if(tmthing->player->axishit && tmthing->player->lastaxis == thing)
			{
				tmthing->player->axishit = true;
				return true;
			}

			if(tmthing->target->health <= thing->health)
			{
				// Find the next axis with a ->health
				// +1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health ? tmthing->target->health + 1 : thing->health + 1);
			}
			else if(tmthing->target->health > thing->health)
			{
				// Find the next axis with a ->health
				// -1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health - 1);
			}
		}
		else if(thing->type == MT_AXISTRANSFERCONDITION)
		{
			if(!tmthing->target)
				return true;

			if(tmthing->target->threshold != thing->threshold)
				return true;

			blockdist = thing->radius + tmthing->radius;

			if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
				return true; // didn't hit it

			if(tmthing->player->axishit && tmthing->player->lastaxis == thing)
			{
				tmthing->player->axishit = true;
				return true;
			}

			if(tmthing->target->health == thing->health)
			{
				// Find the next axis with a ->health
				// +1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health + 1);
			}
			else if(tmthing->target->health == thing->health+1
				&& tmthing->player->flyangle > 90
				&& tmthing->player->flyangle < 270)
			{
				// Find the next axis with a ->health
				// -1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health - 1);
			}
		}
		else if(thing->type == MT_AXISTRANSFERCONDITION2)
		{
			if(!tmthing->target)
				return true;

			if(tmthing->target->threshold != thing->threshold)
				return true;

			blockdist = thing->radius + tmthing->radius;

			if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
				return true; // didn't hit it

			if(tmthing->player->axishit && tmthing->player->lastaxis == thing)
			{
				tmthing->player->axishit = true;
				return true;
			}

			if(tmthing->target->health == thing->health)
			{
				// Find the next axis with a ->health
				// +1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health + 1);
			}
			else if(tmthing->target->health == thing->health+1
				&& (tmthing->player->flyangle < 90
				|| tmthing->player->flyangle > 270))
			{
				// Find the next axis with a ->health
				// -1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, tmthing->target->health - 1);
			}
		}
		// Transfers you from the last axis to the first one and back.
		else if(thing->type == MT_AXISTRANSFERTOLAST)
		{
			if(!tmthing->target)
				return true;

			if(tmthing->target->threshold != thing->threshold)
				return true;

			blockdist = thing->radius + tmthing->radius;

			if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
				return true; // didn't hit it

			if(tmthing->player->axishit && tmthing->player->lastaxis == thing)
			{
				tmthing->player->axishit = true;
				return true;
			}

			if(tmthing->target->health == thing->health)
			{
				// Find the next axis with a ->health
				// +1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, 1);
			}
			else if(tmthing->target->health == 1)
			{
				// Find the next axis with a ->health
				// -1 from the current axis.
				tmthing->player->lastaxis = thing;
				P_TransferToAxis(tmthing->player, thing->health);
			}
		}
		else if(thing->type == MT_AXISTRANSFERCLOSEST)
		{
			if(!tmthing->target)
				return true;

			if(tmthing->target->threshold != thing->threshold)
				return true;

			blockdist = thing->radius + tmthing->radius;

			if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
				return true; // didn't hit it
			tmthing->player->transfertoclosest = true;
		}
	}

	if(!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
		return true;

#ifdef CLIENTPREDICTION2
	// mobj and spirit of a same player cannot colide
	if(thing->player && (thing->player->spirit == tmthing || thing->player->mo == tmthing))
		return true;
#endif

	if(thing->type == MT_SPARK || tmthing->type == MT_SPARK) // Don't collide with sparks, hehe!
		return true;

	blockdist = thing->radius + tmthing->radius;

	if(abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
		return true; // didn't hit it

	// check for skulls slamming into things
	if(tmthing->flags2 & MF2_SKULLFLY)
	{
		if(tmthing->type == MT_EGGMOBILE) // Don't make Eggman stop!
		{
			tmthing->momx = -tmthing->momx;
			tmthing->momy = -tmthing->momy;
			tmthing->momz = -tmthing->momz;
		}
		else
		{
			tmthing->flags2 &= ~MF2_SKULLFLY;
			tmthing->momx = tmthing->momy = tmthing->momz = 0;
			return false; // stop moving
		}
	}

	// Snowballs can hit other things
	if(tmthing->type == MT_SNOWBALL)
	{
		// see if it went over / under
		if(tmthing->z > thing->z + thing->height)
			return true; // overhead
		if(tmthing->z + tmthing->height < thing->z)
			return true; // underneath

		if(tmthing->target && tmthing->target->type == thing->type)
		{
			// Don't hit same species as originator.
			if(thing == tmthing->target)
				return true;

			if(thing->type != MT_PLAYER)
			{
				// Explode, but do no damage.
				// Let players missile other players.
				return false;
			}
		}

		if(!(thing->flags & MF_SHOOTABLE))
		{
			// didn't do any damage
			return !(thing->flags & MF_SOLID);
		}

		// damage / explode
		damage = 1;
		P_DamageMobj(thing, tmthing, tmthing->target, damage);

		// don't traverse any more
			return true;
	}

	// missiles can hit other things
	if(tmthing->flags & MF_MISSILE || tmthing->type == MT_SHELL || tmthing->type == MT_FIREBALL)
	{
		// see if it went over / under
		if(tmthing->z > thing->z + thing->height)
			return true; // overhead
		if(tmthing->z + tmthing->height < thing->z)
			return true; // underneath

		if(tmthing->type != MT_SHELL && tmthing->target && tmthing->target->type == thing->type)
		{
			// Don't hit same species as originator.
			if(thing == tmthing->target)
				return true;

			if(thing->type != MT_PLAYER)
			{
				// Explode, but do no damage.
				// Let players missile other players.
				return false;
			}
		}

		if(!(thing->flags & MF_SHOOTABLE))
		{
			// didn't do any damage
			return !(thing->flags & MF_SOLID);
		}

		if(tmthing->type == MT_SHELL && tmthing->threshold > TICRATE)
			return true;

		// damage / explode
		damage = 1;
		if(tmthing->flags & MF_ENEMY) // An actual ENEMY! (Like the deton, for example)
			P_DamageMobj(thing, tmthing, tmthing, damage);
		else
			P_DamageMobj(thing, tmthing, tmthing->target, damage);

		// don't traverse any more
		return false;
	}

	if(tmthing->z + tmthing->height > thing->z && tmthing->z < thing->z + thing->height
		&& thing->flags & MF_PUSHABLE) // Push thing!
	{
		if(thing->flags2 & MF2_SLIDEPUSH) // Make it slide
		{
			if(tmthing->momy > 0 && tmthing->momy > 4*FRACUNIT && tmthing->momy > thing->momy)
			{
				thing->momy += PUSHACCEL;
				tmthing->momy -= PUSHACCEL;
			}
			else if(tmthing->momy < 0 && tmthing->momy < -4*FRACUNIT
				&& tmthing->momy < thing->momy)
			{
				thing->momy -= PUSHACCEL;
				tmthing->momy += PUSHACCEL;
			}
			if(tmthing->momx > 0 && tmthing->momx > 4*FRACUNIT
				&& tmthing->momx > thing->momx)
			{
				thing->momx += PUSHACCEL;
				tmthing->momx -= PUSHACCEL;
			}
			else if(tmthing->momx < 0 && tmthing->momx < -4*FRACUNIT
				&& tmthing->momx < thing->momx)
			{
				thing->momx -= PUSHACCEL;
				tmthing->momx += PUSHACCEL;
			}

			if(thing->momx > thing->info->speed)
				thing->momx = thing->info->speed;
			else if(thing->momx < -(thing->info->speed))
				thing->momx = -(thing->info->speed);
			if(thing->momy > thing->info->speed)
				thing->momy = thing->info->speed;
			else if(thing->momy < -(thing->info->speed))
				thing->momy = -(thing->info->speed);
		}
		else
		{
			if(tmthing->momy > 0 && tmthing->momy > 4*FRACUNIT)
				tmthing->momy = 4*FRACUNIT;
			else if(tmthing->momy < 0 && tmthing->momy < -4*FRACUNIT)
				tmthing->momy = -4*FRACUNIT;
			if(tmthing->momx > 0 && tmthing->momx > 4*FRACUNIT)
				tmthing->momx = 4*FRACUNIT;
			else if(tmthing->momx < 0 && tmthing->momx < -4*FRACUNIT)
				tmthing->momx = -4*FRACUNIT;

			thing->momx = tmthing->momx;
			thing->momy = tmthing->momy;
		}

		thing->target = tmthing;
	}

	if(tmthing->type == MT_SPIKEBALL && thing->player)
		P_TouchSpecialThing(tmthing, thing, true);
	else if(thing->type == MT_SPIKEBALL && tmthing->player)
		P_TouchSpecialThing(thing, tmthing, true);

	// check for special pickup
	if(thing->flags & MF_SPECIAL)
	{
		solid = thing->flags & MF_SOLID;
		if(tmthing->player)
			P_TouchSpecialThing(thing, tmthing, true); // can remove thing
		return !solid;
	}
	// check again for special pickup
	if(tmthing->flags & MF_SPECIAL)
	{
		solid = tmthing->flags & MF_SOLID;
		if(thing->player)
			P_TouchSpecialThing(tmthing, thing, true); // can remove thing
		return !solid;
	}

	// Sprite Spikes!
	if(tmthing->type == MT_CEILINGSPIKE)
	{
		if(thing->z + thing->height == tmthing->z && thing->momz >= 0)
			P_DamageMobj(thing, tmthing, tmthing, 1); // Ouch!
	}
	else if(thing->type == MT_CEILINGSPIKE)
	{
		if(tmthing->z + tmthing->height == thing->z && tmthing->momz >= 0)
			P_DamageMobj(tmthing, thing, thing, 1);
	}
	else if(tmthing->type == MT_FLOORSPIKE)
	{
		if(thing->z == tmthing->z + tmthing->height + FRACUNIT && thing->momz <= 0)
		{
			tmthing->threshold = 43;
			P_DamageMobj(thing, tmthing, tmthing, 1);
		}
	}
	else if(thing->type == MT_FLOORSPIKE)
	{
		if(tmthing->z == thing->z + thing->height + FRACUNIT
			&& tmthing->momz <= 0)
		{
			thing->threshold = 43;
			P_DamageMobj(tmthing, thing, thing, 1);
		}
	}

	if((tmthing->flags & MF_PUSHABLE) && tmthing->z <= (thing->z + thing->height + FRACUNIT)
		&& (tmthing->z + tmthing->height) >= thing->z)
	{
		if(thing->flags & MF_SPRING)
		{
			if(tmthing->player && tmthing->player->nightsmode);
			else
			{
				P_DoSpring(thing, tmthing);
				tmsprung = true;
			}
		}
	}

	if(cv_tailspickup.value)
	{
		if(tmthing->player && thing->player)
		{
			if(tmthing->player->carried && tmthing->tracer == thing)
				return true;
			else if(thing->player->carried && thing->tracer == tmthing)
				return true;
			else if(tmthing->player->powers[pw_tailsfly]
					|| (tmthing->player->charability == 1 && (tmthing->state == &states[S_PLAY_SPC1] || tmthing->state == &states[S_PLAY_SPC2] || tmthing->state == &states[S_PLAY_SPC3] || tmthing->state == &states[S_PLAY_SPC4])))
			{
				if(thing->player->nightsmode)
					return true;

				if((tmthing->z <= thing->z + thing->height + FRACUNIT)
					&& tmthing->z > thing->z + thing->height*2/3
					&& thing->momz <= 0)
				{
					if(gametype == GT_CTF && ((!tmthing->player->ctfteam || !thing->player->ctfteam) || tmthing->player->ctfteam != thing->player->ctfteam))
						thing->player->carried = false;
					else
					{
						P_ResetPlayer(thing->player);
						P_ResetScore(thing->player);
						thing->tracer = tmthing;
						thing->player->carried = true;
						P_UnsetThingPosition(thing);
						thing->x = tmthing->x;
						thing->y = tmthing->y;
						P_SetThingPosition(thing);
					}
				}
				else
					thing->player->carried = false;
			}
			else
				thing->player->carried = false;

			return true;
		}
	}
	else if(thing->player)
		thing->player->carried = false;

	if(thing->player)
	{
		// Objects kill you if it falls from above.
		if(tmthing->z + tmthing->momz <= thing->z + thing->height
			&& tmthing->z + tmthing->momz > thing->z
			&& thing->z == thing->floorz)
		{
			if(tmthing->flags & MF_MONITOR && maptol & TOL_ADVENTURE);
			else if((tmthing->flags & MF_MONITOR) || (tmthing->flags & MF_PUSHABLE))
			{
				if(thing != tmthing->target)
					P_DamageMobj(thing, tmthing, tmthing->target, 10000);

				tmthing->momz = -tmthing->momz/2; // Bounce, just for fun!
				// The tmthing->target allows the pusher of the object
				// to get the point if he topples it on an opponent.
			}
		}

		// Tag Mode stuff
		if(gametype == GT_TAG && tmthing->player && (((thing->z <= tmthing->z + tmthing->height) && (thing->z + thing->height >= tmthing->z)) || (tmthing->z == thing->z + thing->height + FRACUNIT)))
		{
			if(thing->player->tagit < 298*TICRATE && thing->player->tagit > 0 && !(tmthing->player->powers[pw_flashing] || tmthing->player->tagzone || tmthing->player->powers[pw_invulnerability]))
			{
				P_DamageMobj(tmthing, thing, thing, 1); // Don't allow tag-backs
			}
			else if(tmthing->player->tagit < 298*TICRATE && tmthing->player->tagit > 0 && !(thing->player->powers[pw_flashing] || thing->player->tagzone || thing->player->powers[pw_invulnerability]))
			{
				P_DamageMobj(thing, tmthing, tmthing, 1); // Don't allow tag-backs
			}
		}

		if(thing->z >= tmthing->z) // Stuff where da player don't gotta move
		{
			switch(tmthing->type)
			{
				case MT_FAN: // fan
					if(thing->z <= tmthing->z + (tmthing->health << FRACBITS))
					{
						thing->momz = tmthing->info->speed;
						P_ResetPlayer(thing->player);
						if(!(thing->state == &states[S_PLAY_FALL1] || thing->state == &states[S_PLAY_FALL2]))
							P_SetPlayerMobjState(thing, S_PLAY_FALL1);
					}
					break;
				case MT_STEAM: // Steam
					if(tmthing->state == &states[S_STEAM1] && thing->z <= tmthing->z + 16*FRACUNIT) // Only when it bursts
					{
						thing->momz = tmthing->info->speed;
						P_ResetPlayer(thing->player);
						if(!(thing->state == &states[S_PLAY_FALL1] || thing->state == &states[S_PLAY_FALL2]))
							P_SetPlayerMobjState(thing, S_PLAY_FALL1);
					}
					break;
				default:
					break;
			}
		}
	}

	if(tmthing->player) // Is the moving/interacting object the player?
	{
		if(tmthing->z >= thing->z)
		{
			switch(thing->type)
			{
				case MT_FAN: // fan
					if(tmthing->z <= thing->z + (thing->health << FRACBITS))
					{
						tmthing->momz = thing->info->speed;
						P_ResetPlayer(tmthing->player);
						if(!(tmthing->state == &states[S_PLAY_FALL1] || tmthing->state == &states[S_PLAY_FALL2]))
							P_SetPlayerMobjState(tmthing, S_PLAY_FALL1);
					}
					break;
				case MT_STEAM: // Steam
					if(thing->state == &states[S_STEAM1] && tmthing->z <= thing->z + 16*FRACUNIT) // Only when it bursts
					{
						tmthing->momz = thing->info->speed;
						P_ResetPlayer(tmthing->player);
						if(!(tmthing->state == &states[S_PLAY_FALL1] || tmthing->state == &states[S_PLAY_FALL2]))
							P_SetPlayerMobjState(tmthing, S_PLAY_FALL1);
					}
					break;
				default:
					break;
			}
		}

		// Are you touching the side of the object you're interacting with?
		if((thing->z <= tmthing->z + tmthing->height
			&& thing->z + thing->height >= tmthing->z)
			|| tmthing->z == thing->z + thing->height + FRACUNIT)
		{
			if(thing->flags & MF_SPRING)
			{
				if(tmthing->player && tmthing->player->nightsmode);
				else
				{
					P_DoSpring(thing, tmthing);
					tmsprung = true;
				}
			}
			else if(thing->flags & MF_MONITOR
				&& (tmthing->player->mfjumped || tmthing->player->mfspinning))
			{
				// Going down? Then bounce back up.
				if(tmthing->momz < 0)
					tmthing->momz = -tmthing->momz;
				P_DamageMobj(thing, tmthing, tmthing, 1); // break the monitor
			}
			else if(thing->flags & MF_BOSS
				&& (tmthing->player->mfjumped || tmthing->player->mfspinning
				|| tmthing->player->powers[pw_invulnerability]
				|| tmthing->player->powers[pw_super]))
			{
				// Going down? Then bounce back up.
				if(tmthing->momz < 0)
					tmthing->momz = -tmthing->momz;
				// Also, bounce back.
				tmthing->momx = -tmthing->momx;
				tmthing->momy = -tmthing->momy;
				P_DamageMobj(thing, tmthing, tmthing, 1); // fight the boss!
			}
		}
	}

	// compatibility with old demos, it used to return with...
	// for version 112+, nonsolid things pass through other things
	if(!(tmthing->flags & MF_SOLID))
		return !(thing->flags & MF_SOLID);

	// z checking at last
	// Treat noclip things as non-solid!
	if((thing->flags & MF_SOLID) && (tmthing->flags & MF_SOLID) &&
		!(thing->flags & MF_NOCLIP) && !(tmthing->flags & MF_NOCLIP))
	{
		// pass under
		tmtopz = tmthing->z + tmthing->height;

		if(tmtopz < thing->z)
		{
			if(thing->z < tmceilingz)
			{
				tmceilingz = thing->z;
				tmceilingff = NULL;
			}
			return true;
		}

		topz = thing->z + thing->height + FRACUNIT;

		// block only when jumping not high enough,
		// (dont climb max. 24units while already in air)
		// if not in air, let P_TryMove() decide if it's not too high
		if(tmthing->player && tmthing->z < topz && tmthing->z > tmthing->floorz)
			return false; // block while in air

		if(topz > tmfloorz)
		{
			tmfloorz = topz;
			tmfloorff = NULL;
			tmfloorthing = thing; // thing we may stand on
		}
	}

	// not solid not blocked
	return true;
}

// PIT_CheckCameraLine
// Adjusts tmfloorz and tmceilingz as lines are contacted - FOR CAMERA ONLY
static boolean PIT_CheckCameraLine(line_t* ld)
{
	if(tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	{
		return true;
	}

	if(P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	// A line has been hit

	// The moving thing's destination position will cross
	// the given line.
	// If this should not be allowed, return false.
	// If the line is special, keep track of it
	// to process later if the move is proven ok.
	// NOTE: specials are NOT sorted by order,
	// so two special lines that are only 8 pixels apart
	// could be crossed in either order.

	// this line is out of the if so upper and lower textures can be hit by a splat
	blockingline = ld;
	if(!ld->backsector)
		return false; // one sided line

	// set openrange, opentop, openbottom
	P_CameraLineOpening(ld);

	// adjust floor / ceiling heights
	if(opentop < tmceilingz)
	{
		tmsectorceilingz = tmceilingz = opentop;
		tmceilingff = NULL; /// \note in the right place? needed for camera at all?
		ceilingline = ld;
	}

	if(openbottom > tmfloorz)
	{
		tmsectorfloorz = tmfloorz = openbottom;
		tmfloorff = NULL; /// \note in the right place? needed for camera at all?
	}

	if(lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	return true;
}

//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//
static boolean PIT_CheckLine(line_t* ld)
{
	if(tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	{
		return true;
	}

	if(P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	// A line has been hit

	// The moving thing's destination position will cross
	// the given line.
	// If this should not be allowed, return false.
	// If the line is special, keep track of it
	// to process later if the move is proven ok.
	// NOTE: specials are NOT sorted by order,
	// so two special lines that are only 8 pixels apart
	// could be crossed in either order.

	// this line is out of the if so upper and lower textures can be hit by a splat
	blockingline = ld;
	if(!ld->backsector)
	{
		if(tmthing->flags & MF_MISSILE && ld->special)
			add_spechit(ld);

		return false; // one sided line
	}

	// missiles can cross uncrossable lines
	if(!(tmthing->flags & MF_MISSILE))
	{
		if(ld->flags & ML_BLOCKING)
			return false; // explicitly blocking everything

		if(!(tmthing->player) && ld->flags & ML_BLOCKMONSTERS)
			return false; // block monsters only
	}

	// set openrange, opentop, openbottom
	P_LineOpening(ld);

	// adjust floor / ceiling heights
	if(opentop < tmceilingz)
	{
		tmsectorceilingz = tmceilingz = opentop;
		tmceilingff = NULL; /// \note in the right place?
		ceilingline = ld;
	}

	if(openbottom > tmfloorz)
	{
		tmsectorfloorz = tmfloorz = openbottom;
		tmfloorff = NULL; /// \note in the right place?
	}

	if(lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	// if contacted a special line, add it to the list
	if(ld->special)
		add_spechit(ld);

	return true;
}

// =========================================================================
//                         MOVEMENT CLIPPING
// =========================================================================

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  tmfloorz
//  tmceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//

// tmfloorz
//     the nearest floor or thing's top under tmthing
// tmceilingz
//     the nearest ceiling or thing's bottom over tmthing
//
boolean P_CheckPosition(mobj_t* thing, fixed_t x, fixed_t y)
{
	int xl, xh, yl, yh, bx, by;
	subsector_t* newsubsec;

	tmthing = thing;
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = blockingline = NULL;

	// The base floor / ceiling is from the subsector
	// that contains the point.
	// Any contacted lines the step closer together
	// will adjust them.
	tmfloorz = tmsectorfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = tmsectorceilingz = newsubsec->sector->ceilingheight;
	tmfloorff = tmceilingff = NULL;

	// Check list of fake floors and see if tmfloorz/tmceilingz need to be altered.
	if(newsubsec->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = thing->z + thing->height;

		for(rover = newsubsec->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if(!(rover->flags & FF_SOLID))
			{
				if(!(thing->player && !thing->player->nightsmode && (thing->player->skin == 1 || thing->player->powers[pw_super])
					&& !thing->player->mfspinning && thing->player->speed > 28
/*					&& thing->ceilingz - *rover->topheight >= thing->height*/
					&& thing->z < *rover->topheight + 30*FRACUNIT
					&& thing->z > *rover->topheight - 30*FRACUNIT
					&& (rover->flags & FF_SWIMMABLE))
					&& (!(thing->type == MT_SKIM && (rover->flags & FF_SWIMMABLE))))
					continue;
			}

			delta1 = thing->z - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > tmfloorz && abs(delta1) < abs(delta2))
			{
				tmfloorz = tmdropoffz = *rover->topheight;
				tmfloorff = rover;
			}
			if(*rover->bottomheight < tmceilingz && abs(delta1) >= abs(delta2)
				&& (/*thing->z + thing->height <= *rover->bottomheight
					|| */!(rover->flags & FF_PLATFORM))
				&& !(thing->type == MT_SKIM	&& (rover->flags & FF_SWIMMABLE)))
			{
				tmceilingz = *rover->bottomheight;
				tmceilingff = rover;
			}
		}
	}

	// tmfloorthing is set when tmfloorz comes from a thing's top
	tmfloorthing = NULL;

	validcount++;
	numspechit = 0;

	if(tmflags & MF_NOCLIP)
		return true;

	// Check things first, possibly picking things up.
	// The bounding box is extended by MAXRADIUS
	// because mobj_ts are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.

	if(thing->player && thing->player->nightsmode)
		thing->player->axishit = false;

	// MF_NOCLIPTHING: used by camera to not be blocked by things
	if(!(thing->flags & MF_NOCLIPTHING))
	{
		xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
		xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
		yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
		yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

		for(bx = xl; bx <= xh; bx++)
			for(by = yl; by <= yh; by++)
				if(!P_BlockThingsIterator(bx, by, PIT_CheckThing))
					return false;
	}

	if(thing->player && thing->player->nightsmode && !thing->player->axishit)
	{
		thing->player->axistransferred = false;
		thing->player->transferangle = -1;
	}

	// check lines
	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			if(!P_BlockLinesIterator(bx, by, PIT_CheckLine))
				return false;

	return true;
}

//
// P_CheckRailPosition
//
// Optimized code for rail rings
//
#if 0
static inline boolean P_CheckRailPosition(mobj_t* thing, fixed_t x, fixed_t y)
{
	int xl, xh, yl, yh, bx, by;
	subsector_t* newsubsec;

	tmthing = thing;
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = blockingline = NULL;

	// The base floor / ceiling is from the subsector
	// that contains the point.
	// Any contacted lines the step closer together
	// will adjust them.
	tmfloorz = tmsectorfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = tmsectorceilingz = newsubsec->sector->ceilingheight;
	tmfloorff = tmceilingff = NULL;

	// Check list of fake floors and see if tmfloorz/tmceilingz need to be altered.
	if(newsubsec->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = thing->z + thing->height;

		for(rover = newsubsec->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if(!(rover->flags & FF_SOLID))
				continue;

			delta1 = thing->z - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > tmfloorz && abs(delta1) < abs(delta2))
			{
				tmfloorz = tmdropoffz = *rover->topheight;
				tmfloorff = rover;
			}
			if(*rover->bottomheight < tmceilingz && abs(delta1) >= abs(delta2)
				&& (/*thing->z + thing->height <= *rover->bottomheight
					||*/ !(rover->flags & FF_PLATFORM))
				&& !(thing->type == MT_SKIM	&& rover->flags & FF_SWIMMABLE))
			{
				tmceilingz = *rover->bottomheight;
				tmceilingff = rover;
			}
		}
	}

	// tmfloorthing is set when tmfloorz comes from a thing's top
	tmfloorthing = NULL;

	validcount++;
	numspechit = 0;

	// Check things first, possibly picking things up.
	// The bounding box is extended by MAXRADIUS
	// because mobj_ts are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.

	// MF_NOCLIPTHING: used by camera to not be blocked by things
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			if(!P_BlockThingsIterator(bx, by, PIT_CheckThing))
				return false;

	// check lines
	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			if(!P_BlockLinesIterator(bx, by, PIT_CheckLine))
				return false;

	return true;
}
#endif

// P_CheckPosition optimized for the MT_HOOPCOLLIDE object. This needs to be as fast as possible!
boolean P_CheckHoopPosition(mobj_t* hoopthing, fixed_t x, fixed_t y, fixed_t z, fixed_t radius)
{
	int i;
	fixed_t blockdist;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || !players[i].mo)
			continue;

		blockdist = players[i].mo->radius + radius;

		if(abs(players[i].mo->x - x) >= blockdist ||
			abs(players[i].mo->y - y) >= blockdist)
			return true; // didn't hit it

		if(players[i].mo->z > z+radius || players[i].mo->z+players[i].mo->height < z-radius)
			return true; // Still didn't hit it.

		// check for pickup
		if(hoopthing->flags & MF_SPECIAL)
		{
			// can remove thing
			P_TouchSpecialThing(hoopthing, players[i].mo, true);
			return false;
		}
	}

	return true;
}

//
// P_CheckCameraPosition
//
static boolean P_CheckCameraPosition(fixed_t x, fixed_t y, camera_t* thiscam)
{
	int xl, xh, yl, yh, bx, by;
	subsector_t* newsubsec;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + thiscam->radius;
	tmbbox[BOXBOTTOM] = y - thiscam->radius;
	tmbbox[BOXRIGHT] = x + thiscam->radius;
	tmbbox[BOXLEFT] = x - thiscam->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = blockingline = NULL;

	// The base floor / ceiling is from the subsector
	// that contains the point.
	// Any contacted lines the step closer together
	// will adjust them.
	tmfloorz = tmsectorfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = tmsectorceilingz = newsubsec->sector->ceilingheight;
	tmfloorff = tmceilingff = NULL;

	// Check list of fake floors and see if tmfloorz/tmceilingz need to be altered.
	if(newsubsec->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = thiscam->z + thiscam->height;

		for(rover = newsubsec->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
				continue;

			delta1 = thiscam->z - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight
				+ ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > tmfloorz && abs(delta1) < abs(delta2))
			{
				tmfloorz = tmdropoffz = *rover->topheight;
				tmfloorff = rover; /// \todo not needed for camera?
			}
			if(*rover->bottomheight < tmceilingz && abs(delta1) >= abs(delta2))
			{
				tmceilingz = *rover->bottomheight;
				tmceilingff = rover; /// \todo not needed for camera?
			}
		}
	}

	// Check things first, possibly picking things up.
	// The bounding box is extended by MAXRADIUS
	// because mobj_ts are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.

	// check lines
	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			if(!P_BlockLinesIterator(bx, by, PIT_CheckCameraLine))
				return false;

	return true;
}

//
// CheckMissileImpact
//
static void CheckMissileImpact(mobj_t* mobj)
{
	if(!numspechit || !(mobj->flags & MF_MISSILE) || !mobj->target)
		return;

	if(!mobj->target->player)
		return;
}

//
// P_TryCameraMove
//
// Attempt to move the camera to a new position
//
boolean P_TryCameraMove(fixed_t x, fixed_t y, camera_t* thiscam)
{
	fixed_t oldx, oldy;

	floatok = false;

	if(!P_CheckCameraPosition(x, y, thiscam))
		return false; // solid wall or thing

	if(tmceilingz - tmfloorz < thiscam->height)
		return false; // doesn't fit

	floatok = true;

	if(tmceilingz - thiscam->z < thiscam->height)
		return false; // mobj must lower itself to fit

	if((tmfloorz - thiscam->z > MAXSTEPMOVE))
		return false; // too big a step up

	// the move is ok,
	// so link the thing into its new position

	oldx = thiscam->x;
	oldy = thiscam->y;
	thiscam->floorz = tmfloorz;
	thiscam->ceilingz = tmceilingz;
	thiscam->x = x;
	thiscam->y = y;

	return true;
}

//
// P_TryMove
// Attempt to move to a new position.
//
boolean P_TryMove(mobj_t* thing, fixed_t x, fixed_t y, boolean allowdropoff)
{
	fixed_t oldx, oldy;

	floatok = false;

	if(!P_CheckPosition(thing, x, y))
	{
		CheckMissileImpact(thing);
		return false; // solid wall or thing
	}

#ifdef CLIENTPREDICTION2
	if(!(thing->flags & MF_NOCLIP) && !(thing->eflags & MF_NOZCHECKING))
#else
	if(!(thing->flags & MF_NOCLIP))
#endif
	{
		fixed_t maxstep = MAXSTEPMOVE;

		// Don't 'step up' while springing.
		if(thing->player && thing->state == &states[S_PLAY_PLG1])
			maxstep = 0;

		// Only step up "if needed".
		if(thing->player && thing->momz > FRACUNIT)
			maxstep = 0;

#ifdef ANNOYINGSTEP
		if(thing->eflags & MF_STEPPEDUP)
			maxstep = 0;
#endif

		if(tmceilingz - tmfloorz < thing->height
			|| thing->z > tmceilingz || thing->z < tmfloorz)
		{
			if(thing->type == MT_SHELL)
			{
				int counting;

				counting = 0;
				thing->angle += ANG45;
				P_UnsetThingPosition(thing);
				thing->x -= thing->momx;
				thing->y -= thing->momy;
				P_SetThingPosition(thing);
				P_InstaThrust(thing, thing->angle, thing->info->speed);

				while(!P_TryMove(thing, thing->x, thing->y, true) && counting < 500)
				{
					thing->angle += ANG45;
					P_InstaThrust(thing, thing->angle, thing->info->speed);
					counting++;
				}
				S_StartSound(thing, sfx_tink);
				return true;
			}
		}

		if(tmceilingz - tmfloorz < thing->height)
		{
			CheckMissileImpact(thing);
			return false; // doesn't fit
		}

		floatok = true;

		if(tmceilingz - thing->z < thing->height)
		{
			CheckMissileImpact(thing);
			return false; // mobj must lower itself to fit
		}

		// Ramp test
		if(thing->player && (thing->player->specialsector != 996
			&& R_PointInSubsector(x, y)->sector->special != 996) && thing->z == thing->floorz
			&& tmfloorz < thing->z && thing->z - tmfloorz <= MAXSTEPMOVE)
		{
			// If the floor difference is MAXSTEPMOVE or less, and the sector isn't 996, ALWAYS
			// step down! Formerly required a 992 sector for the full MAXSTEPMOVE, but no more.
			thing->z = tmfloorz;
			thing->eflags |= MF_JUSTSTEPPEDDOWN;
		}

		// jump out of water
		if((thing->eflags & (MF_UNDERWATER|MF_TOUCHWATER)) == (MF_UNDERWATER|MF_TOUCHWATER))
			maxstep = 37*FRACUNIT;

		if((tmfloorz - thing->z > maxstep))
		{
			CheckMissileImpact(thing);
			return false; // too big a step up
		}

		if(tmfloorz > thing->z)
		{
#ifdef ANNOYINGSTEP
			thing->eflags |= MF_STEPPEDUP;
#endif
			if((thing->flags & MF_MISSILE))
				CheckMissileImpact(thing);
		}

		if(!allowdropoff)
			if(!(thing->flags & (MF_FLOAT)) && thing->type != MT_SKIM && !tmfloorthing
				&& tmfloorz - tmdropoffz > MAXSTEPMOVE)
				return false; // don't stand over a dropoff
	}

	// the move is ok,
	// so link the thing into its new position
	P_UnsetThingPosition(thing);

	oldx = thing->x;
	oldy = thing->y;
	thing->floorz = tmfloorz;
	thing->floorff = tmfloorff;
	thing->ceilingz = tmceilingz;
	thing->ceilingff = tmceilingff;
	thing->x = x;
	thing->y = y;

	if(tmfloorthing)
		thing->eflags &= ~MF_ONGROUND; // not on real floor
	else
		thing->eflags |= MF_ONGROUND;

	P_SetThingPosition(thing);
	return true;
}

boolean P_SceneryTryMove(mobj_t* thing, fixed_t x, fixed_t y)
{
	fixed_t oldx, oldy;

	if(!P_CheckPosition(thing, x, y))
		return false; // solid wall or thing

	if(!(thing->flags & MF_NOCLIP))
	{
		fixed_t maxstep = MAXSTEPMOVE;

		if(tmceilingz - tmfloorz < thing->height)
			return false; // doesn't fit

		if(tmceilingz - thing->z < thing->height)
			return false; // mobj must lower itself to fit

		if(tmfloorz - thing->z > maxstep)
			return false; // too big a step up
	}

	// the move is ok,
	// so link the thing into its new position
	P_UnsetThingPosition(thing);

	oldx = thing->x;
	oldy = thing->y;
	thing->floorz = tmfloorz;
	thing->floorff = tmfloorff;
	thing->ceilingz = tmceilingz;
	thing->ceilingff = tmceilingff;
	thing->x = x;
	thing->y = y;

	if(tmfloorthing)
		thing->eflags &= ~MF_ONGROUND; // not on real floor
	else
		thing->eflags |= MF_ONGROUND;

	P_SetThingPosition(thing);
	return true;
}

//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
static boolean P_ThingHeightClip(mobj_t* thing)
{
	fixed_t oldfloorz = thing->floorz;
	boolean onfloor = (thing->z <= thing->floorz);

	P_CheckPosition(thing, thing->x, thing->y);

	// what about stranding a monster partially off an edge?

	thing->floorz = tmfloorz;
	thing->floorff = tmfloorff;
	thing->ceilingz = tmceilingz;
	thing->ceilingff = tmceilingff;

	switch(thing->type)
	{
		case MT_RING:
		case MT_COIN:
		case MT_HOMINGRING:
		case MT_RAILRING:
		case MT_INFINITYRING:
		case MT_AUTOMATICRING:
		case MT_EXPLOSIONRING:
		case MT_NIGHTSWING:
		case MT_BLUEORB:
		case MT_BLACKORB:
		case MT_WHITEORB:
		case MT_YELLOWORB:
		case MT_REDORB:
		case MT_THOK:
			return true; // Ignore these items
		default:
			break;
	}

	// Have player fall through floor?
	if(thing->player && thing->player->playerstate == PST_DEAD)
		return true;

	if(!tmfloorthing && onfloor && !(thing->flags & MF_NOGRAVITY))
	{
		if(thing->watertop != oldfloorz)
		{
			// walking monsters rise and fall with the floor
			if(thing->floorz != thing->z && (thing->floorz - thing->z > 0))
			{
				thing->pmomz = thing->floorz - thing->z;
			}
			thing->z = thing->floorz;
		}
		else if(thing->z != thing->floorz)
		{
			if(thing->z - thing->floorz > 0 && thing->z - thing->floorz < MAXSTEPMOVE)
				thing->z = thing->floorz;
			else
				onfloor = false;
		}
	}
	else
	{
		// don't adjust a floating monster unless forced to
		if(!onfloor && thing->z + thing->height > tmceilingz)
			thing->z = thing->ceilingz - thing->height;
	}

	// debug: be sure it falls to the floor
	thing->eflags &= ~MF_ONGROUND;

	//added:28-02-98:
	// test sector bouding top & bottom, not things

	if(thing->ceilingz - thing->floorz < thing->height && thing->z >= thing->floorz)
		// BP: i know that this code cause many trouble but this fix alos
		// lot of problem, mainly this is implementation of the stepping
		// for mobj (walk on solid corpse without jumping or fake 3d bridge)
		// problem is imp into imp at map01 and monster going at top of others
		return false;

	return true;
}

//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//
static fixed_t bestslidefrac, secondslidefrac;
static line_t* bestslideline;
static line_t* secondslideline;
static mobj_t* slidemo;
static fixed_t tmxmove, tmymove;

//
// P_HitCameraSlideLine
//
static void P_HitCameraSlideLine(line_t* ld, camera_t* thiscam)
{
	int side;
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen, newlen;

	if(ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = 0;
		return;
	}

	if(ld->slopetype == ST_VERTICAL)
	{
		tmxmove = 0;
		return;
	}

	side = P_PointOnLineSide(thiscam->x, thiscam->y, ld);
	lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

	if(side == 1)
		lineangle += ANG180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle-lineangle;

	if(deltaangle > ANG180)
		deltaangle += ANG180;

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = P_AproxDistance(tmxmove, tmymove);
	newlen = FixedMul(movelen, finecosine[deltaangle]);

	tmxmove = FixedMul(newlen, finecosine[lineangle]);
	tmymove = FixedMul(newlen, finesine[lineangle]);
}

//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
static void P_HitSlideLine(line_t* ld)
{
	int side;
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen, newlen;

	if(ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = 0;
		return;
	}

	if(ld->slopetype == ST_VERTICAL)
	{
		tmxmove = 0;
		return;
	}

	side = P_PointOnLineSide(slidemo->x, slidemo->y, ld);

	lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

	if(side == 1)
		lineangle += ANG180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle-lineangle;

	if(deltaangle > ANG180)
		deltaangle += ANG180;

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = P_AproxDistance(tmxmove, tmymove);
	newlen = FixedMul(movelen, finecosine[deltaangle]);

	tmxmove = FixedMul(newlen, finecosine[lineangle]);
	tmymove = FixedMul(newlen, finesine[lineangle]);
}

//
// P_HitBounceLine
//
// Adjusts the xmove / ymove so that the next move will bounce off the wall.
//
static void P_HitBounceLine(line_t* ld)
{
	int side;
	angle_t lineangle, moveangle, deltaangle;
	fixed_t movelen;

	if(ld->slopetype == ST_HORIZONTAL)
	{
		tmymove = -tmymove;
		return;
	}

	if(ld->slopetype == ST_VERTICAL)
	{
		tmxmove = -tmxmove;
		return;
	}

	side = P_PointOnLineSide(slidemo->x, slidemo->y, ld);

	lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

	if(lineangle >= ANG180)
		lineangle -= ANG180;

	moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
	deltaangle = moveangle + 2*(lineangle - moveangle);

	lineangle >>= ANGLETOFINESHIFT;
	deltaangle >>= ANGLETOFINESHIFT;

	movelen = P_AproxDistance(tmxmove, tmymove);

	tmxmove = FixedMul(movelen, finecosine[deltaangle]);
	tmymove = FixedMul(movelen, finesine[deltaangle]);

	deltaangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
}

//
// PTR_SlideCameraTraverse
//
static boolean PTR_SlideCameraTraverse(intercept_t* in)
{
	line_t* li;

#ifdef PARANOIA
	if(!in->isaline)
		I_Error("PTR_SlideCameraTraverse: not a line?");
#endif

	li = in->d.line;

	if(!(li->flags & ML_TWOSIDED))
	{
		if(P_PointOnLineSide(mapcampointer->x, mapcampointer->y, li))
			return true; // don't hit the back side
		goto isblocking;
	}

	// set openrange, opentop, openbottom
	P_CameraLineOpening(li);

	if(openrange < mapcampointer->height)
		goto isblocking; // doesn't fit

	if(opentop - mapcampointer->z < mapcampointer->height)
		goto isblocking; // mobj is too high

	if(openbottom - mapcampointer->z > 24*FRACUNIT )
		goto isblocking; // too big a step up

	// this line doesn't block movement
	return true;

	// the line does block movement,
	// see if it is closer than best so far
isblocking:
	{
		if(in->frac < bestslidefrac)
		{
			secondslidefrac = bestslidefrac;
			secondslideline = bestslideline;
			bestslidefrac = in->frac;
			bestslideline = li;
		}
	}

	return false; // stop
}

//
// P_IsClimbingValid
//
static boolean P_IsClimbingValid(player_t* player, angle_t angle)
{
	fixed_t platx, platy;
	subsector_t* glidesector;
	boolean climb = true;

	platx = P_ReturnThrustX(player->mo, angle, player->mo->radius + 8*FRACUNIT);
	platy = P_ReturnThrustY(player->mo, angle, player->mo->radius + 8*FRACUNIT);

	glidesector = R_PointInSubsector(player->mo->x + platx, player->mo->y + platy);

	if(glidesector->sector != player->mo->subsector->sector)
	{
		boolean floorclimb = false, thrust = false, boostup = false;

		if(glidesector->sector->ffloors)
		{
			ffloor_t* rover;
			for(rover = glidesector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_SOLID))
					continue;

				floorclimb = true;

				if((*rover->bottomheight > player->mo->z) && ((player->mo->z - player->mo->momz) > *rover->bottomheight))
				{
					floorclimb = true;
					boostup = false;
				}
				if(*rover->bottomheight > player->mo->z + player->mo->height) // Waaaay below the ledge.
				{
					floorclimb = false;
					boostup = false;
					thrust = false;
				}
				if(*rover->topheight < player->mo->z + 16*FRACUNIT)
				{
					floorclimb = false;
					thrust = true;
					boostup = true;
				}

				if(floorclimb)
					break;
			}
		}

		if((glidesector->sector->ceilingheight >= player->mo->z)
			&& ((player->mo->z - player->mo->momz) >= glidesector->sector->ceilingheight))
			floorclimb = true;

		if(!floorclimb && glidesector->sector->floorheight < player->mo->z + 16*FRACUNIT
			&& (glidesector->sector->ceilingpic == skyflatnum
			|| glidesector->sector->ceilingheight
			> (player->mo->z + player->mo->height + 8*FRACUNIT)))
		{
			thrust = true;
			boostup = true;
			// Play climb-up animation here
		}
		if((glidesector->sector->ceilingheight < player->mo->z)
			&& glidesector->sector->ceilingpic == skyflatnum)
			return false;

		if((player->mo->z + 16*FRACUNIT < glidesector->sector->floorheight)
			|| (player->mo->z >= glidesector->sector->ceilingheight))
			floorclimb = true;

		climb = false;

		if(!floorclimb)
			return false;

		return true;
	}

	return false;
}

//
// PTR_SlideTraverse
//
static boolean PTR_SlideTraverse(intercept_t* in)
{
	line_t* li;

#ifdef PARANOIA
	if(!in->isaline)
		I_Error("PTR_SlideTraverse: not a line?");
#endif

	li = in->d.line;

	if(!(li->flags & ML_TWOSIDED))
	{
		if(P_PointOnLineSide(slidemo->x, slidemo->y, li))
			return true; // don't hit the back side
		goto isblocking;
	}

	// set openrange, opentop, openbottom
	P_LineOpening(li);

	if(openrange < slidemo->height)
		goto isblocking; // doesn't fit

	if(opentop - slidemo->z < slidemo->height)
		goto isblocking; // mobj is too high

	if(openbottom - slidemo->z > 24*FRACUNIT)
		goto isblocking; // too big a step up

	// this line doesn't block movement
	return true;

	// the line does block movement,
	// see if it is closer than best so far
isblocking:
	{
		// see about climbing on the wall
		if(slidemo->player && !(li->flags & ML_NOCLIMB)
			&& (slidemo->player->gliding || slidemo->player->climbing))
		{
			angle_t climbline, climbangle;
			int whichside;

			climbline = R_PointToAngle2(li->v1->x, li->v1->y, li->v2->x, li->v2->y);

			whichside = P_PointOnLineSide(slidemo->x, slidemo->y, li);
			if(whichside) // on second side?
				climbline += ANG180;

			if(((!slidemo->player->climbing
				&& abs(slidemo->angle - ANG90 - climbline) < ANG45) ||

				(slidemo->player->climbing == 1
				&& abs(slidemo->angle - climbline) < ANG90+ANG45))

				&& P_IsClimbingValid(slidemo->player, climbangle =
				R_PointToAngle2(li->v1->x, li->v1->y, li->v2->x, li->v2->y)
				+ (ANG90 * (whichside ? -1 : 1))))
			{
				slidemo->angle = climbangle;
				if(slidemo->player == &players[consoleplayer])
					localangle = slidemo->angle;
				else if(cv_splitscreen.value && slidemo->player ==
					&players[secondarydisplayplayer])
				{
					localangle2 = slidemo->angle;
				}

				if(!slidemo->player->climbing)
					slidemo->player->climbing = 5;

				slidemo->player->gliding = slidemo->player->glidetime = 0;
				slidemo->player->mfspinning = slidemo->player->mfjumped = 0;
				slidemo->player->thokked = false;

				if(slidemo->player->climbing > 1)
					slidemo->momz = slidemo->momx = slidemo->momy = 0;

				if(!whichside)
				{
					slidemo->player->lastsidehit = li->sidenum[whichside];
					slidemo->player->lastlinehit = (short)(li - lines);
				}

				P_Thrust(slidemo, slidemo->angle, 5*FRACUNIT);
			}
		}
		if(in->frac < bestslidefrac && (!slidemo->player || !slidemo->player->climbing))
		{
			secondslidefrac = bestslidefrac;
			secondslideline = bestslideline;
			bestslidefrac = in->frac;
			bestslideline = li;
		}
	}

	return false; // stop
}

//
// P_SlideCameraMove
//
// Tries to slide the camera along a wall.
//
void P_SlideCameraMove(camera_t* thiscam)
{
	fixed_t leadx, leady, trailx, traily, newx, newy;
	int hitcount = 0;

retry:
	if(++hitcount == 3)
		goto stairstep; // don't loop forever

	// trace along the three leading corners
	if(thiscam->momx > 0)
	{
		leadx = thiscam->x + thiscam->radius;
		trailx = thiscam->x - thiscam->radius;
	}
	else
	{
		leadx = thiscam->x - thiscam->radius;
		trailx = thiscam->x + thiscam->radius;
	}

	if(thiscam->momy > 0)
	{
		leady = thiscam->y + thiscam->radius;
		traily = thiscam->y - thiscam->radius;
	}
	else
	{
		leady = thiscam->y - thiscam->radius;
		traily = thiscam->y + thiscam->radius;
	}

	bestslidefrac = FRACUNIT+1;

	mapcampointer = thiscam;

	P_PathTraverse(leadx, leady, leadx + thiscam->momx, leady + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);
	P_PathTraverse(trailx, leady, trailx + thiscam->momx, leady + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);
	P_PathTraverse(leadx, traily, leadx + thiscam->momx, traily + thiscam->momy,
		PT_ADDLINES, PTR_SlideCameraTraverse);

	// move up to the wall
	if(bestslidefrac == FRACUNIT+1)
	{
		// the move must have hit the middle, so stairstep
stairstep:
		if(!P_TryCameraMove(thiscam->x, thiscam->y + thiscam->momy, thiscam)) // Allow things to
			P_TryCameraMove(thiscam->x + thiscam->momx, thiscam->y, thiscam); // drop off.
		return;
	}

	// fudge a bit to make sure it doesn't hit
	bestslidefrac -= 0x800;
	if(bestslidefrac > 0)
	{
		newx = FixedMul(thiscam->momx, bestslidefrac);
		newy = FixedMul(thiscam->momy, bestslidefrac);

		if(!P_TryCameraMove(thiscam->x + newx, thiscam->y + newy, thiscam))
			goto stairstep;
	}

	// Now continue along the wall.
	// First calculate remainder.
	bestslidefrac = FRACUNIT - (bestslidefrac+0x800);

	if(bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;

	if(bestslidefrac <= 0)
		return;

	tmxmove = FixedMul(thiscam->momx, bestslidefrac);
	tmymove = FixedMul(thiscam->momy, bestslidefrac);

	P_HitCameraSlideLine(bestslideline, thiscam); // clip the moves

	thiscam->momx = tmxmove;
	thiscam->momy = tmymove;

	if(!P_TryCameraMove(thiscam->x + tmxmove, thiscam->y + tmymove, thiscam))
		goto retry;
}

//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove(mobj_t* mo)
{
	fixed_t leadx, leady, trailx, traily, newx, newy;
	int hitcount = 0;

	slidemo = mo;

retry:
	if(++hitcount == 3)
		goto stairstep; // don't loop forever

	// trace along the three leading corners
	if(mo->momx > 0)
	{
		leadx = mo->x + mo->radius;
		trailx = mo->x - mo->radius;
	}
	else
	{
		leadx = mo->x - mo->radius;
		trailx = mo->x + mo->radius;
	}

	if(mo->momy > 0)
	{
		leady = mo->y + mo->radius;
		traily = mo->y - mo->radius;
	}
	else
	{
		leady = mo->y - mo->radius;
		traily = mo->y + mo->radius;
	}

	bestslidefrac = FRACUNIT+1;

	P_PathTraverse(leadx, leady, leadx + mo->momx, leady + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(trailx, leady, trailx + mo->momx, leady + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(leadx, traily, leadx + mo->momx, traily + mo->momy,
		PT_ADDLINES, PTR_SlideTraverse);

	// Some walls are bouncy even if you're not
	if(bestslideline && bestslideline->flags & ML_BOUNCY)
	{
		P_BounceMove(mo);
		return;
	}

	// move up to the wall
	if(bestslidefrac == FRACUNIT+1)
	{
		// the move must have hit the middle, so stairstep
stairstep:
		if(!P_TryMove(mo, mo->x, mo->y + mo->momy, true))
			P_TryMove(mo, mo->x + mo->momx, mo->y, true); //Allow things to drop off.
		return;
	}

	// fudge a bit to make sure it doesn't hit
	bestslidefrac -= 0x800;
	if(bestslidefrac > 0)
	{
		newx = FixedMul(mo->momx, bestslidefrac);
		newy = FixedMul(mo->momy, bestslidefrac);

		if(!P_TryMove(mo, mo->x + newx, mo->y + newy, true))
			goto stairstep;
	}

	// Now continue along the wall.
	// First calculate remainder.
	bestslidefrac = FRACUNIT - (bestslidefrac+0x800);

	if(bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;

	if(bestslidefrac <= 0)
		return;

	tmxmove = FixedMul(mo->momx, bestslidefrac);
	tmymove = FixedMul(mo->momy, bestslidefrac);

	P_HitSlideLine(bestslideline); // clip the moves

	mo->momx = tmxmove;
	mo->momy = tmymove;

	if(!P_TryMove(mo, mo->x + tmxmove, mo->y + tmymove, true))
		goto retry;
}

//
// P_BounceMove
//
// The momx / momy move is bad, so try to bounce off a wall.
//
void P_BounceMove(mobj_t* mo)
{
	fixed_t leadx, leady;
	fixed_t trailx, traily;
	fixed_t newx, newy;
	int hitcount;
	fixed_t mmomx = 0, mmomy = 0;

	slidemo = mo;
	hitcount = 0;

retry:
	if(++hitcount == 3)
		goto bounceback; // don't loop forever

	if(mo->player)
	{
		mmomx = mo->player->rmomx;
		mmomy = mo->player->rmomy;
	}
	else
	{
		mmomx = mo->momx;
		mmomy = mo->momy;
	}

	// trace along the three leading corners
	if(mo->momx > 0)
	{
		leadx = mo->x + mo->radius;
		trailx = mo->x - mo->radius;
	}
	else
	{
		leadx = mo->x - mo->radius;
		trailx = mo->x + mo->radius;
	}

	if(mo->momy > 0)
	{
		leady = mo->y + mo->radius;
		traily = mo->y - mo->radius;
	}
	else
	{
		leady = mo->y - mo->radius;
		traily = mo->y + mo->radius;
	}

	bestslidefrac = FRACUNIT + 1;

	P_PathTraverse(leadx, leady, leadx + mmomx, leady + mmomy, PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(trailx, leady, trailx + mmomx, leady + mmomy, PT_ADDLINES, PTR_SlideTraverse);
	P_PathTraverse(leadx, traily, leadx + mmomx, traily + mmomy, PT_ADDLINES, PTR_SlideTraverse);

	// move up to the wall
	if(bestslidefrac == FRACUNIT + 1)
	{
		// the move must have hit the middle, so bounce straight back
bounceback:
		if(P_TryMove(mo, mo->x - mmomx, mo->y - mmomy, true))
		{
			mo->momx *= -1;
			mo->momy *= -1;
			mo->momx = FixedMul(mo->momx, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
			mo->momy = FixedMul(mo->momy, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
			if(mo->player)
			{
				mo->player->cmomx *= -1;
				mo->player->cmomy *= -1;
				mo->player->cmomx = FixedMul(mo->player->cmomx,
					(FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
				mo->player->cmomy = FixedMul(mo->player->cmomy,
					(FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
			}
		}
		return;
	}

	// fudge a bit to make sure it doesn't hit
	bestslidefrac -= 0x800;
	if(bestslidefrac > 0)
	{
		newx = FixedMul(mmomx, bestslidefrac);
		newy = FixedMul(mmomy, bestslidefrac);

		if(!P_TryMove(mo, mo->x + newx, mo->y + newy, true))
			goto bounceback;
	}

	// Now continue along the wall.
	// First calculate remainder.
	bestslidefrac = FRACUNIT - bestslidefrac;

	if(bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;

	if(bestslidefrac <= 0)
		return;

	tmxmove = FixedMul(mmomx, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
	tmymove = FixedMul(mmomy, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));

	P_HitBounceLine(bestslideline); // clip the moves

	mo->momx = tmxmove;
	mo->momy = tmymove;

	if(mo->player)
	{
		mo->player->cmomx = tmxmove;
		mo->player->cmomy = tmymove;
	}

	if(!P_TryMove(mo, mo->x + tmxmove, mo->y + tmymove, true))
		goto retry;
}

mobj_t* linetarget; // who got hit (or NULL)
static mobj_t* shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
static fixed_t shootz;
static fixed_t lastz; // The last z height of the bullet when it crossed a line

fixed_t attackrange;
static fixed_t aimslope;

//
// PTR_AimTraverse
// Sets linetarget and aimslope when a target is aimed at.
//
//added:15-02-98: comment
// Returns true if the thing is not shootable, else continue through..
//
static boolean PTR_AimTraverse(intercept_t* in)
{
	line_t* li;
	mobj_t* th;
	fixed_t slope, thingtopslope, thingbottomslope, dist;
	int dir;

	if(in->isaline)
	{
		li = in->d.line;

		if(!(li->flags & ML_TWOSIDED))
			return false; // stop

		// Crosses a two sided line.
		// A two sided line will restrict
		// the possible target ranges.
		tmthing = NULL;
		P_LineOpening(li);

		if(openbottom >= opentop)
			return false; // stop

		dist = FixedMul(attackrange, in->frac);

		if(li->frontsector->floorheight != li->backsector->floorheight)
		{
			slope = FixedDiv(openbottom - shootz , dist);
			if(slope > bottomslope)
				bottomslope = slope;
		}

		if(li->frontsector->ceilingheight != li->backsector->ceilingheight)
		{
			slope = FixedDiv(opentop - shootz , dist);
			if(slope < topslope)
				topslope = slope;
		}

		if(topslope <= bottomslope)
			return false; // stop

		if(li->frontsector->ffloors || li->backsector->ffloors)
		{
			int frontflag;

			dir = aimslope > 0 ? 1 : aimslope < 0 ? -1 : 0;

			frontflag = P_PointOnLineSide(shootthing->x, shootthing->y, li);

			//SoM: Check 3D FLOORS!
			if(li->frontsector->ffloors)
			{
				ffloor_t* rover = li->frontsector->ffloors;
				fixed_t highslope, lowslope;

				for(; rover; rover = rover->next)
				{
					if(!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
						continue;

					highslope = FixedDiv(*rover->topheight - shootz, dist);
					lowslope = FixedDiv(*rover->bottomheight - shootz, dist);
					if((aimslope >= lowslope && aimslope <= highslope))
						return false;

					if(lastz > *rover->topheight && dir == -1 && aimslope < highslope)
						frontflag |= 0x2;

					if(lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
						frontflag |= 0x2;
				}
			}

			if(li->backsector->ffloors)
			{
				ffloor_t* rover = li->backsector->ffloors;
				fixed_t highslope, lowslope;

				for(; rover; rover = rover->next)
				{
					if(!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
						continue;

					highslope = FixedDiv(*rover->topheight - shootz, dist);
					lowslope = FixedDiv(*rover->bottomheight - shootz, dist);
					if((aimslope >= lowslope && aimslope <= highslope))
						return false;

					if(lastz > *rover->topheight && dir == -1 && aimslope < highslope)
						frontflag |= 0x4;

					if(lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
						frontflag |= 0x4;
				}
			}
			if((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
				return false;
		}

		lastz = FixedMul(aimslope, dist) + shootz;

		return true; // shot continues
	}

	// shoot a thing
	th = in->d.thing;
	if(th == shootthing)
		return true; // can't shoot self

	if(!(th->flags & MF_SHOOTABLE))
		return true; // corpse or something

	if(th->flags & MF_MONITOR)
		return true; // don't autoaim at monitors

	// check angles to see if the thing can be aimed at
	dist = FixedMul(attackrange, in->frac);
	thingtopslope = FixedDiv(th->z+th->height - shootz , dist);

	//added:15-02-98: bottomslope is negative!
	if(thingtopslope < bottomslope)
		return true; // shot over the thing

	thingbottomslope = FixedDiv(th->z - shootz, dist);

	if(thingbottomslope > topslope)
		return true; // shot under the thing

	// this thing can be hit!
	if(thingtopslope > topslope)
		thingtopslope = topslope;

	if(thingbottomslope < bottomslope)
		thingbottomslope = bottomslope;

	//added:15-02-98: find the slope just in the middle(y) of the thing!
	aimslope = (thingtopslope + thingbottomslope)/2;
	linetarget = th;

	return false; // don't go any farther
}

//
// P_AimLineAttack
//
fixed_t P_AimLineAttack(mobj_t* t1, angle_t angle, fixed_t distance)
{
	fixed_t x2, y2;

#ifdef PARANOIA
	if(!t1)
		I_Error("P_AimLineAttack: t1 is NULL");
#endif

	angle >>= ANGLETOFINESHIFT;
	shootthing = t1;

	if(t1->player)
	{
		fixed_t cosineaiming = finecosine[t1->player->aiming>>ANGLETOFINESHIFT];
		int aiming = ((int)t1->player->aiming)>>ANGLETOFINESHIFT;
		x2 = t1->x + FixedMul(FixedMul(distance, finecosine[angle]), cosineaiming);
		y2 = t1->y + FixedMul(FixedMul(distance, finesine[angle]), cosineaiming);

		topslope = 100*FRACUNIT/160+finetangent[(2048+aiming) & FINEMASK];
		bottomslope = -100*FRACUNIT/160+finetangent[(2048+aiming) & FINEMASK];
	}
	else
	{
		x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
		y2 = t1->y + (distance>>FRACBITS)*finesine[angle];

		//added:15-02-98: Fab comments...
		// Doom's base engine says that at a distance of 160,
		// the 2d graphics on the plane x, y correspond 1/1 with plane units
		topslope = 100*FRACUNIT/160;
		bottomslope = -100*FRACUNIT/160;
	}

	shootz = lastz = t1->z + (t1->height>>1) + 8*FRACUNIT;

	// can't shoot outside view angles
	attackrange = distance;
	linetarget = NULL;

	//added:15-02-98: comments
	// traverse all linedefs and mobjs from the blockmap containing t1,
	// to the blockmap containing the dest. point.
	// Call the function for each mobj/line on the way,
	// starting with the mobj/linedef at the shortest distance...
	P_PathTraverse(t1->x, t1->y, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_AimTraverse);

	//added:15-02-98: linetarget is only for mobjs, not for linedefs
	if(linetarget)
		return aimslope;

	return 0;
}

//
// RADIUS ATTACK
//
static int bombdamage;

//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
static boolean PIT_RadiusAttack(mobj_t* thing)
{
	fixed_t dx, dy, dz, dist;

	if(!(thing->flags & MF_SHOOTABLE))
		return true;

	if(thing->flags & MF_BOSS)
		return true;

	// Boss spider and cyborg
	// take no damage from concussion.
	switch(thing->type)
	{
		case MT_SKIM:
		case MT_JETTBOMBER: // Jetty-Syn Bomber
			return true;
		default:
			if(thing->flags & MF_MONITOR)
				return true;
			break;
	}

	dx = abs(thing->x - bombspot->x);
	dy = abs(thing->y - bombspot->y);

	dist = dx > dy ? dx : dy;
	dist -= thing->radius;

	//added:22-02-98: now checks also z dist for rockets exploding
	//                above yer head...
	dz = abs(thing->z + (thing->height>>1) - bombspot->z);
	dist = dist > dz ? dist : dz;
	dist >>= FRACBITS;

	if(dist < 0)
		dist = 0;

	if(dist >= bombdamage)
		return true; // out of range

	if(thing->floorz > bombspot->z && bombspot->ceilingz < thing->z)
		return true;

	if(thing->ceilingz < bombspot->z && bombspot->floorz > thing->z)
		return true;

	if(P_CheckSight(thing, bombspot))
	{
		int damage = bombdamage - dist;
		int momx = 0, momy = 0;
		if(dist)
		{
			momx = (thing->x - bombspot->x)/dist;
			momy = (thing->y - bombspot->y)/dist;
		}
		// must be in direct path
		P_DamageMobj(thing, bombspot, bombsource, damage); // Tails 01-11-2001
	}

	return true;
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack(mobj_t* spot, mobj_t* source, int damage)
{
	int x, y;
	int xl, xh, yl, yh;
	fixed_t dist;

	dist = (damage + MAXRADIUS)<<FRACBITS;
	yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
	bombspot = spot;

	bombsource = source;
	bombdamage = damage;

	for(y = yl; y <= yh; y++)
		for(x = xl; x <= xh; x++)
			P_BlockThingsIterator(x, y, PIT_RadiusAttack);
}

//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_CheckSector (? was P_ChangeSector - Graue) again
//  to undo the changes.
//
static boolean crushchange;
static boolean nofit;

//
// PIT_ChangeSector
//
static boolean PIT_ChangeSector(mobj_t* thing)
{
	if(P_ThingHeightClip(thing))
	{
		// keep checking
		return true;
	}

	if(!(thing->flags & MF_SHOOTABLE))
	{
		// assume it is bloody gibs or something
		return true;
	}

	nofit = true;

	// Crush the thing if necessary, and if it's a crumbling FOF that did it,
	// reward the player who made it crumble!
	if(thing->z + thing->height > thing->ceilingz && thing->z <= thing->ceilingz)
	{
		if(thing->subsector->sector->ffloors)
		{
			ffloor_t* rover;
			fixed_t delta1, delta2;
			int thingtop = thing->z + thing->height;

			for(rover = thing->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
					continue;

				delta1 = thing->z - (*rover->bottomheight + *rover->topheight)/2;
				delta2 = thingtop - (*rover->bottomheight + *rover->topheight)/2;
				if(*rover->bottomheight <= thing->ceilingz && abs(delta1) >= abs(delta2))
				{
					thinker_t* think;
					elevator_t* crumbler;

					for(think = thinkercap.next; think != &thinkercap; think = think->next)
					{
						if(think->function.acp1 != (actionf_p1)T_StartCrumble)
							continue;

						crumbler = (elevator_t*)think;

						if(crumbler->player && crumbler->player->mo
							&& crumbler->player->mo != thing
							&& crumbler->actionsector == thing->subsector->sector
							&& crumbler->sector == rover->master->frontsector
							&& (crumbler->type == elevateBounce
							|| crumbler->type == elevateContinuous))
						{
							P_DamageMobj(thing, crumbler->player->mo, crumbler->player->mo, 10000);
							return true;
						}
					}
				}
			}
		}

		// Instant-death, but no one to blame
		P_DamageMobj(thing, NULL, NULL, 10000);
	}

	if(crushchange && !(leveltime % (4*NEWTICRATERATIO)))
		P_DamageMobj(thing, NULL, NULL, 10);

	// keep checking (crush other things)
	return true;
}

//
// P_CheckSector
//
boolean P_CheckSector(sector_t* sector, boolean crunch)
{
	msecnode_t* n;

	nofit = false;
	crushchange = crunch;

	// killough 4/4/98: scan list front-to-back until empty or exhausted,
	// restarting from beginning after each thing is processed. Avoids
	// crashes, and is sure to examine all things in the sector, and only
	// the things which are in the sector, until a steady-state is reached.
	// Things can arbitrarily be inserted and removed and it won't mess up.
	//
	// killough 4/7/98: simplified to avoid using complicated counter

	if(sector->numattached)
	{
		int i;
		sector_t* sec;
		for(i = 0; i < sector->numattached; i ++)
		{
			sec = &sectors[sector->attached[i]];
			for(n = sec->touching_thinglist; n; n = n->m_snext)
				n->visited = false;

			sec->moved = true;

			do
			{
				for(n = sec->touching_thinglist; n; n = n->m_snext)
				if(!n->visited)
				{
					n->visited = true;
					if(!(n->m_thing->flags & MF_NOBLOCKMAP))
						PIT_ChangeSector(n->m_thing);
					break;
				}
			} while(n);
		}
	}
	// Mark all things invalid
	sector->moved = true;

	for(n = sector->touching_thinglist; n; n = n->m_snext)
		n->visited = false;

	do
	{
		for(n = sector->touching_thinglist; n; n = n->m_snext) // go through list
			if(!n->visited) // unprocessed thing found
			{
				n->visited = true; // mark thing as processed
				if(!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
					PIT_ChangeSector(n->m_thing); // process it
				break; // exit and start over
			}
	} while(n); // repeat from scratch until all things left are marked valid

	return nofit;
}

/*
 SoM: 3/15/2000
 Lots of new Boom functions that work faster and add functionality.
*/

static msecnode_t* headsecnode = NULL;
static mprecipsecnode_t* headprecipsecnode = NULL;

void P_Initsecnode(void)
{
	headsecnode = NULL;
	headprecipsecnode = NULL;
}

// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.

static msecnode_t* P_GetSecnode()
{
	msecnode_t* node;

	if(headsecnode)
	{
		node = headsecnode;
		headsecnode = headsecnode->m_snext;
	}
	else
		node = Z_Malloc(sizeof(*node), PU_LEVEL, NULL);
	return(node);
}

static mprecipsecnode_t* P_GetPrecipSecnode()
{
	mprecipsecnode_t* node;

	if(headprecipsecnode)
	{
		node = headprecipsecnode;
		headprecipsecnode = headprecipsecnode->m_snext;
	}
	else
		node = Z_Malloc(sizeof(*node), PU_LEVEL, NULL);
	return(node);
}

// P_PutSecnode() returns a node to the freelist.

static inline void P_PutSecnode(msecnode_t* node)
{
	node->m_snext = headsecnode;
	headsecnode = node;
}

// Tails 08-25-2002
static inline void P_PutPrecipSecnode(mprecipsecnode_t* node)
{
	node->m_snext = headprecipsecnode;
	headprecipsecnode = node;
}

// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.

static msecnode_t* P_AddSecnode(sector_t* s, mobj_t* thing, msecnode_t* nextnode)
{
	msecnode_t* node;

	node = nextnode;
	while(node)
	{
		if(node->m_sector == s) // Already have a node for this sector?
		{
			node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
			return nextnode;
		}
		node = node->m_tnext;
	}

	// Couldn't find an existing node for this sector. Add one at the head
	// of the list.

	node = P_GetSecnode();

	// mark new nodes unvisited.
	node->visited = 0;

	node->m_sector = s; // sector
	node->m_thing = thing; // mobj
	node->m_tprev = NULL; // prev node on Thing thread
	node->m_tnext = nextnode; // next node on Thing thread
	if(nextnode)
		nextnode->m_tprev = node; // set back link on Thing

	// Add new node at head of sector thread starting at s->touching_thinglist

	node->m_sprev = NULL; // prev node on sector thread
	node->m_snext = s->touching_thinglist; // next node on sector thread
	if(s->touching_thinglist)
		node->m_snext->m_sprev = node;
	s->touching_thinglist = node;
	return node;
}

// More crazy crap Tails 08-25-2002
static mprecipsecnode_t* P_AddPrecipSecnode(sector_t* s, precipmobj_t* thing, mprecipsecnode_t* nextnode)
{
	mprecipsecnode_t* node;

	node = nextnode;
	while(node)
	{
		if(node->m_sector == s) // Already have a node for this sector?
		{
			node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
			return nextnode;
		}
		node = node->m_tnext;
	}

	// Couldn't find an existing node for this sector. Add one at the head
	// of the list.

	node = P_GetPrecipSecnode();

	// mark new nodes unvisited.
	node->visited = 0;

	node->m_sector = s; // sector
	node->m_thing = thing; // mobj
	node->m_tprev = NULL; // prev node on Thing thread
	node->m_tnext = nextnode; // next node on Thing thread
	if(nextnode)
		nextnode->m_tprev = node; // set back link on Thing

	// Add new node at head of sector thread starting at s->touching_thinglist

	node->m_sprev = NULL; // prev node on sector thread
	node->m_snext = s->touching_preciplist; // next node on sector thread
	if(s->touching_preciplist)
		node->m_snext->m_sprev = node;
	s->touching_preciplist = node;
	return node;
}

// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.

static msecnode_t* P_DelSecnode(msecnode_t* node)
{
	msecnode_t* tp; // prev node on thing thread
	msecnode_t* tn; // next node on thing thread
	msecnode_t* sp; // prev node on sector thread
	msecnode_t* sn; // next node on sector thread

	if(node)
	{
		// Unlink from the Thing thread. The Thing thread begins at
		// sector_list and not from mobj_t->touching_sectorlist.

		tp = node->m_tprev;
		tn = node->m_tnext;
		if(tp)
			tp->m_tnext = tn;
		if(tn)
			tn->m_tprev = tp;

		// Unlink from the sector thread. This thread begins at
		// sector_t->touching_thinglist.

		sp = node->m_sprev;
		sn = node->m_snext;
		if(sp)
			sp->m_snext = sn;
		else
			node->m_sector->touching_thinglist = sn;
		if(sn)
			sn->m_sprev = sp;

		// Return this node to the freelist

		P_PutSecnode(node);
		return tn;
	}
	return NULL;
}

// Tails 08-25-2002
static mprecipsecnode_t* P_DelPrecipSecnode(mprecipsecnode_t* node)
{
	mprecipsecnode_t* tp; // prev node on thing thread
	mprecipsecnode_t* tn; // next node on thing thread
	mprecipsecnode_t* sp; // prev node on sector thread
	mprecipsecnode_t* sn; // next node on sector thread

	if(node)
	{
		// Unlink from the Thing thread. The Thing thread begins at
		// sector_list and not from mobj_t->touching_sectorlist.

		tp = node->m_tprev;
		tn = node->m_tnext;
		if(tp)
			tp->m_tnext = tn;
		if(tn)
			tn->m_tprev = tp;

		// Unlink from the sector thread. This thread begins at
		// sector_t->touching_thinglist.

		sp = node->m_sprev;
		sn = node->m_snext;
		if(sp)
			sp->m_snext = sn;
		else
			node->m_sector->touching_preciplist = sn;
		if(sn)
			sn->m_sprev = sp;

		// Return this node to the freelist

		P_PutPrecipSecnode(node);
		return tn;
	}
	return NULL;
}

// Delete an entire sector list
void P_DelSeclist(msecnode_t* node)
{
	while(node)
		node = P_DelSecnode(node);
}

// Tails 08-25-2002
void P_DelPrecipSeclist(mprecipsecnode_t* node)
{
	while(node)
		node = P_DelPrecipSecnode(node);
}

// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

static inline boolean PIT_GetSectors(line_t* ld)
{
	if(tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
		tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
		tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
		tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	return true;

	if(P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	// This line crosses through the object.

	// Collect the sector(s) from the line and add to the
	// sector_list you're examining. If the Thing ends up being
	// allowed to move to this position, then the sector_list
	// will be attached to the Thing's mobj_t at touching_sectorlist.

	sector_list = P_AddSecnode(ld->frontsector,tmthing,sector_list);

	// Don't assume all lines are 2-sided, since some Things
	// like MT_TFOG are allowed regardless of whether their radius takes
	// them beyond an impassable linedef.

	// Use sidedefs instead of 2s flag to determine two-sidedness.
	if(ld->backsector)
		sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

	return true;
}

// Tails 08-25-2002
static inline boolean PIT_GetPrecipSectors(line_t* ld)
{
	if(tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
		tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
		tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
		tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	return true;

	if(P_BoxOnLineSide(tmbbox, ld) != -1)
		return true;

	// This line crosses through the object.

	// Collect the sector(s) from the line and add to the
	// sector_list you're examining. If the Thing ends up being
	// allowed to move to this position, then the sector_list
	// will be attached to the Thing's mobj_t at touching_sectorlist.

	precipsector_list = P_AddPrecipSecnode(ld->frontsector, tmprecipthing, precipsector_list);

	// Don't assume all lines are 2-sided, since some Things
	// like MT_TFOG are allowed regardless of whether their radius takes
	// them beyond an impassable linedef.

	// Use sidedefs instead of 2s flag to determine two-sidedness.
	if(ld->backsector)
		precipsector_list = P_AddPrecipSecnode(ld->backsector, tmprecipthing, precipsector_list);

	return true;
}

// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.

void P_CreateSecNodeList(mobj_t* thing, fixed_t x, fixed_t y)
{
	int xl, xh, yl, yh, bx, by;
	msecnode_t* node;

	// First, clear out the existing m_thing fields. As each node is
	// added or verified as needed, m_thing will be set properly. When
	// finished, delete all nodes where m_thing is still NULL. These
	// represent the sectors the Thing has vacated.

	node = sector_list;
	while(node)
	{
		node->m_thing = NULL;
		node = node->m_tnext;
	}

	tmthing = thing;
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	validcount++; // used to make sure we only process a line once

	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			P_BlockLinesIterator(bx, by, PIT_GetSectors);

	// Add the sector of the (x, y) point to sector_list.
	sector_list = P_AddSecnode(thing->subsector->sector, thing, sector_list);

	// Now delete any nodes that won't be used. These are the ones where
	// m_thing is still NULL.
	node = sector_list;
	while(node)
	{
		if(!node->m_thing)
		{
			if(node == sector_list)
				sector_list = node->m_tnext;
			node = P_DelSecnode(node);
		}
		else
			node = node->m_tnext;
	}
}

// More crazy crap Tails 08-25-2002
void P_CreatePrecipSecNodeList(precipmobj_t* thing,fixed_t x,fixed_t y)
{
	int xl, xh, yl, yh, bx, by;
	mprecipsecnode_t* node;

	// First, clear out the existing m_thing fields. As each node is
	// added or verified as needed, m_thing will be set properly. When
	// finished, delete all nodes where m_thing is still NULL. These
	// represent the sectors the Thing has vacated.

	node = precipsector_list;
	while(node)
	{
		node->m_thing = NULL;
		node = node->m_tnext;
	}

	tmprecipthing = thing;
	preciptmflags = thing->flags;

	preciptmx = x;
	preciptmy = y;

	// Precipitation has a fixed radius of 2*FRACUNIT Tails 08-28-2002
	preciptmbbox[BOXTOP] = y + 2*FRACUNIT;
	preciptmbbox[BOXBOTTOM] = y - 2*FRACUNIT;
	preciptmbbox[BOXRIGHT] = x + 2*FRACUNIT;
	preciptmbbox[BOXLEFT] = x - 2*FRACUNIT;

	validcount++; // used to make sure we only process a line once

	xl = (preciptmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (preciptmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (preciptmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (preciptmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for(bx = xl; bx <= xh; bx++)
		for(by = yl; by <= yh; by++)
			P_BlockLinesIterator(bx, by, PIT_GetPrecipSectors);

	// Add the sector of the (x, y) point to sector_list.
	precipsector_list = P_AddPrecipSecnode(thing->subsector->sector, thing, precipsector_list);

	// Now delete any nodes that won't be used. These are the ones where
	// m_thing is still NULL.
	node = precipsector_list;
	while(node)
	{
		if(!node->m_thing)
		{
			if(node == precipsector_list)
				precipsector_list = node->m_tnext;
			node = P_DelPrecipSecnode(node);
		}
		else
			node = node->m_tnext;
	}
}

// P_FloorzAtPos
// Returns the floorz of the XYZ position
// Tails 05-26-2003
fixed_t P_FloorzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height)
{
	sector_t* sec;
	fixed_t floorz;

	sec = R_PointInSubsector(x, y)->sector;

	floorz = sec->floorheight;

	// Intercept the stupid 'fall through 3dfloors' bug Tails 03-17-2002
	if(sec->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = z + height;

		for(rover = sec->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if((!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) || (rover->flags & FF_SWIMMABLE)))
				continue;

			if(rover->flags & FF_QUICKSAND)
			{
				if(z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					floorz = z;
					continue;
				}
			}

			delta1 = z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if(*rover->topheight > floorz && abs(delta1) < abs(delta2))
				floorz = *rover->topheight;
		}
	}

	return floorz;
}

//
// P_FakeZMovement
//
// Fake the zmovement so that we can check if a move is legal
//
static void P_FakeZMovement(mobj_t* mo)
{
	int dist;
	int delta;

	// Intercept the stupid 'fall through 3dfloors' bug Tails 03-17-2002
	if(mo->subsector->sector->ffloors)
	{
		ffloor_t* rover;
		fixed_t delta1, delta2;
		int thingtop = mo->z + mo->height;

		for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if((!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND) && !(mo->player && !mo->player->nightsmode && (mo->player->skin == 1 || mo->player->powers[pw_super])  && (rover->flags & FF_SWIMMABLE) && !mo->player->mfspinning && mo->player->speed > 28 && /*mo->ceilingz - *rover->topheight >= mo->height &&*/ mo->z < *rover->topheight + 30*FRACUNIT && mo->z > *rover->topheight - 30*FRACUNIT)))
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

	// adjust height
	mo->z += mo->momz;
	if(mo->flags & MF_FLOAT && mo->target && mo->type != MT_EGGMOBILE
		&& mo->type != MT_EGGMOBILE2 && mo->type != MT_RING && mo->type != MT_COIN) // Tails
	{ // float down towards target if too close
		if(!(mo->flags2&MF2_SKULLFLY) && !(mo->flags&MF_INFLOAT))
		{
			dist = P_AproxDistance(mo->x-mo->target->x, mo->y-mo->target->y);
			delta = (mo->target->z + (mo->height>>1)) - mo->z;
			if(delta < 0 && dist < -(delta*3))
				mo->z -= FLOATSPEED;
			else if(delta > 0 && dist < (delta*3))
				mo->z += FLOATSPEED;
		}
	}

	// clip movement
	if(mo->z <= mo->floorz)
	{ // Hit the floor
		mo->z = mo->floorz;
		if(mo->momz < 0)
			mo->momz = 0;

		if(mo->flags2 & MF2_SKULLFLY) // The skull slammed into something
			mo->momz = -mo->momz;
	}
	else if(!(mo->flags & MF_NOGRAVITY))
	{
		if(!mo->momz)
			mo->momz = -gravity*2;
		else
			mo->momz -= gravity;
	}

	if(mo->z + mo->height > mo->ceilingz) // hit the ceiling
	{
		if(mo->momz > 0)
			mo->momz = 0;
		mo->z = mo->ceilingz - mo->height;
		if(mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
			mo->momz = -mo->momz;
	}
}

// P_CheckOnmobj
//
// Checks if the new Z position is legal
//
mobj_t* P_CheckOnmobj(mobj_t* thing)
{
	int xl, xh, yl, yh;
	subsector_t* newsubsec;
	fixed_t x, y;
	mobj_t oldmo;

	x = thing->x;
	y = thing->y;
	tmthing = thing;
	tmflags = thing->flags;
	oldmo = *thing; // save the old mobj before the fake zmovement
	P_FakeZMovement(tmthing);

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector(x, y);
	ceilingline = NULL;

	//
	// the base floor / ceiling is from the subsector that contains the
	// point. Any contacted lines the step closer together will adjust them
	//
	tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = newsubsec->sector->ceilingheight;
	tmfloorff = tmceilingff = NULL;

	validcount++;
	numspechit = 0;

	if(tmflags & MF_NOCLIP)
		return NULL;

	//
	// check things first, possibly picking things up
	// the bounding box is extended by MAXRADIUS because mobj_ts are grouped
	// into mapblocks based on their origin point, and can overlap into adjacent
	// blocks by up to MAXRADIUS units
	//
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	*tmthing = oldmo;
	return NULL;
}
