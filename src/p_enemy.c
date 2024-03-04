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
/// \brief Enemy thinking, AI
/// 
///	Action Pointer Functions that are associated with states/frames

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "r_things.h"

#include "hardware/hw3sound.h"

player_t* stplyr;

typedef enum
{
	DI_NODIR = -1,
	DI_EAST = 0,
	DI_NORTHEAST = 1,
	DI_NORTH = 2,
	DI_NORTHWEST = 3,
	DI_WEST = 4,
	DI_SOUTHWEST = 5,
	DI_SOUTH = 6,
	DI_SOUTHEAST = 7,
	NUMDIRS = 8,
} dirtype_t;

//
// P_NewChaseDir related LUT.
//
static dirtype_t opposite[] =
{
	DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
	DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

static dirtype_t diags[] =
{
	DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

//Real Prototypes to A_*
void A_Fall(mobj_t* actor);
void A_Look(mobj_t* actor);
void A_Chase(mobj_t* actor);
void A_SkimChase(mobj_t* actor);
void A_FaceTarget(mobj_t* actor);
void A_CyberAttack(mobj_t* actor);
void A_SkullAttack(mobj_t* actor);
void A_BossZoom(mobj_t* actor);
void A_BossScream(mobj_t* actor);
void A_Scream(mobj_t* actor);
void A_Pain(mobj_t* actor);
void A_1upThinker(mobj_t* actor);
void A_MonitorPop(mobj_t* actor);
void A_Explode(mobj_t* actor);
void A_BossDeath(mobj_t* actor);
void A_CustomPower(mobj_t* actor);
void A_JumpShield(mobj_t* actor);
void A_RingShield(mobj_t* actor);
void A_RingBox(mobj_t* actor);
void A_Invincibility(mobj_t* actor);
void A_SuperSneakers(mobj_t* actor);
void A_ExtraLife(mobj_t* actor);
void A_BombShield(mobj_t* actor);
void A_WaterShield(mobj_t* actor);
void A_FireShield(mobj_t* actor);
void A_ScoreRise(mobj_t* actor);
void A_BunnyHop(mobj_t* actor);
void A_BubbleSpawn(mobj_t* actor);
void A_BubbleRise(mobj_t* actor);
void A_BubbleCheck(mobj_t* actor);
void A_AttractChase(mobj_t* actor);
void A_DropMine(mobj_t* actor);
void A_FishJump(mobj_t* actor);
void A_SignPlayer(mobj_t* actor);
void A_ThrownRing(mobj_t* actor);
void A_SetSolidSteam(mobj_t* actor);
void A_UnsetSolidSteam(mobj_t* actor);
void A_JetChase(mobj_t* actor);
void A_JetbThink(mobj_t* actor);
void A_JetgShoot(mobj_t* actor);
void A_JetgThink(mobj_t* actor);
void A_ShootBullet(mobj_t* actor);
void A_MouseThink(mobj_t* actor);
void A_DetonChase(mobj_t* actor);
void A_CapeChase(mobj_t* actor);
void A_RotateSpikeBall(mobj_t* actor);
void A_SnowBall(mobj_t* actor);
void A_CrawlaCommanderThink(mobj_t* actor);
void A_RingExplode(mobj_t* actor);
void A_MixUp(mobj_t* actor);
void A_PumaJump(mobj_t* actor);
void A_Invinciblerize(mobj_t* actor);
void A_DeInvinciblerize(mobj_t* actor);
void A_Boss2PogoSFX(mobj_t* actor);
void A_EggmanBox(mobj_t* actor);
void A_TurretFire(mobj_t* actor);
void A_SuperTurretFire(mobj_t* actor);
void A_TurretStop(mobj_t* actor);
void A_SparkFollow(mobj_t* actor);
void A_BuzzFly(mobj_t* actor);
void A_SetReactionTime(mobj_t* actor);
void A_LinedefExecute(mobj_t* actor);
void A_PlaySeeSound(mobj_t* actor);
void A_PlayAttackSound(mobj_t* actor);
void A_PlayActiveSound(mobj_t* actor);
void A_SmokeTrailer(mobj_t* actor);
//for p_enemy.c
void A_Boss1Chase(mobj_t* actor);
void A_Boss2Chase(mobj_t* actor);
void A_Boss2Pogo(mobj_t* actor);

//
// ENEMY THINKING
// Enemies are always spawned with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players, but some can be made preaware.
//

//
// P_CheckMeleeRange
//
static boolean P_CheckMeleeRange(mobj_t* actor)
{
	mobj_t* pl;
	fixed_t dist;

	if(!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	switch(actor->type)
	{
		case MT_JETTBOMBER:
			if(dist >= (actor->radius + pl->radius)*2)
				return false;
			break;
		case MT_DETON:
			if(dist >= actor->radius+pl->radius)
				return false;
			break;
		default:
			if(dist >= MELEERANGE - 20*FRACUNIT + pl->info->radius)
				return false;
			break;
	}

	// check height now, so that damn crawlas cant attack
	// you if you stand on a higher ledge.
	if(actor->type == MT_JETTBOMBER)
	{
		if(pl->z + pl->height > actor->z - (48<<FRACBITS))
			return false;
	}
	else if(actor->type == MT_SKIM)
	{
		if(pl->z + pl->height > actor->z - (24<<FRACBITS))
			return false;
	}
	else
	{
		if((pl->z > actor->z + actor->height) || (actor->z > pl->z + pl->height))
			return false;

		if(actor->type != MT_JETTBOMBER && actor->type != MT_SKIM
			&& !P_CheckSight(actor, actor->target))
		{
			return false;
		}
	}

	return true;
}

//
// P_CheckMissileRange
//
static boolean P_CheckMissileRange(mobj_t* actor)
{
	fixed_t dist;

	if(!P_CheckSight(actor, actor->target))
		return false;

	if(actor->reactiontime)
		return false; // do not attack yet

	// OPTIMIZE: get this from a global checksight
	dist = P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) - 64*FRACUNIT;

	if(!actor->info->meleestate)
		dist -= 128*FRACUNIT; // no melee attack, so fire more

	dist >>= 16;

	if(actor->type == MT_EGGMOBILE)
		dist >>= 1;

	if(dist > 200)
		dist = 200;

	if(actor->type == MT_EGGMOBILE && dist > 160)
		dist = 160;

	if(P_Random() < dist)
		return false;

	return true;
}

/** Checks for water in a sector.
  * Used by Skim movements.
  *
  * \param x X coordinate on the map.
  * \param y Y coordinate on the map.
  * \return True if there's water at this location, false if not.
  * \todo What if there's no sector at this location (can happen on invalid
  *       maps)? Take care of that case.
  * \sa ::MT_SKIM
  */
static boolean P_WaterInSector(fixed_t x, fixed_t y)
{
	sector_t* sector;

	sector = R_PointInSubsector(x, y)->sector;

	if(sector->ffloors)
	{
		ffloor_t* rover;

		for(rover = sector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_EXISTS))
				continue;

			if(rover->flags & FF_SWIMMABLE)
				return true;
		}
	}

	return false;
}

static const fixed_t xspeed[NUMDIRS] = {FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000};
static const fixed_t yspeed[NUMDIRS] = {0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000};

/** Moves an actor in its current direction.
  *
  * \param actor Actor object to move.
  * \return False if the move is blocked, otherwise true.
  */
static boolean P_Move(mobj_t* actor)
{
	fixed_t tryx, tryy;
	dirtype_t movedir = actor->movedir;

	if(movedir == DI_NODIR)
		return false;

#ifdef PARANOIA
	if(movedir >= NUMDIRS)
		I_Error("Weird movedir! (movedir: %i\nmobjtype: %i)", movedir, actor->type);
#endif

	tryx = actor->x + actor->info->speed*xspeed[movedir];
	tryy = actor->y + actor->info->speed*yspeed[movedir];

	if(actor->type == MT_SKIM && !P_WaterInSector(tryx, tryy)) // bail out if sector lacks water
		return false;

	if(!P_TryMove(actor, tryx, tryy, false))
	{
		// open any specials
		if(actor->flags & MF_FLOAT && floatok)
		{
			// must adjust height
			if(actor->z < tmfloorz)
				actor->z += FLOATSPEED;
			else
				actor->z -= FLOATSPEED;

			actor->flags |= MF_INFLOAT;
			return true;
		}

		if(!numspechit)
			return false;

		actor->movedir = DI_NODIR;
		return false;
	}
	else
		actor->flags &= ~MF_INFLOAT;

	if(!(actor->flags & MF_FLOAT) && actor->type != MT_SKIM) // Don't lower skims!
		actor->z = actor->floorz;
	return true;
}

/** Attempts to move an actor on in its current direction.
  * If the move succeeds, the actor's move count is reset
  * randomly to a value from 0 to 15.
  *
  * \param actor Actor to move.
  * \return True if the move succeeds, false if the move is blocked.
  */
static boolean P_TryWalk(mobj_t* actor)
{
	if(!P_Move(actor))
		return false;
	actor->movecount = P_Random() & 15;
	return true;
}

static void P_NewChaseDir(mobj_t* actor)
{
	fixed_t deltax, deltay;
	dirtype_t d[3];
	dirtype_t tdir=DI_NODIR, olddir, turnaround;

	if(!actor->target) /// \todo PARANOIA?
		I_Error("P_NewChaseDir: called with no target");

	olddir = actor->movedir;

	if(olddir >= NUMDIRS)
		olddir = DI_NODIR;

	if(olddir != DI_NODIR)
		turnaround = opposite[olddir];
	else
		turnaround = olddir;

	deltax = actor->target->x - actor->x;
	deltay = actor->target->y - actor->y;

	if(deltax > 10*FRACUNIT)
		d[1] = DI_EAST;
	else if(deltax < -10*FRACUNIT)
		d[1] = DI_WEST;
	else
		d[1] = DI_NODIR;

	if(deltay < -10*FRACUNIT)
		d[2] = DI_SOUTH;
	else if(deltay > 10*FRACUNIT)
		d[2] = DI_NORTH;
	else
		d[2] = DI_NODIR;

	// try direct route
	if(d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		dirtype_t newdir = diags[((deltay < 0)<<1) + (deltax > 0)];

		actor->movedir = newdir;
		if((newdir != turnaround) && P_TryWalk(actor))
			return;
	}

	// try other directions
	if(P_Random() > 200 || abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if(d[1] == turnaround)
		d[1] = DI_NODIR;
	if(d[2] == turnaround)
		d[2] = DI_NODIR;

	if(d[1] != DI_NODIR)
	{
		actor->movedir = d[1];

		if(P_TryWalk(actor))
			return; // either moved forward or attacked
	}

	if(d[2] != DI_NODIR)
	{
		actor->movedir = d[2];

		if(P_TryWalk(actor))
			return;
	}

	// there is no direct path to the player, so pick another direction.
	if(olddir != DI_NODIR)
	{
		actor->movedir =olddir;

		if(P_TryWalk(actor))
			return;
	}

	// randomly determine direction of search
	if(P_Random() & 1)
	{
		for(tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
		{
			if(tdir != turnaround)
			{
				actor->movedir = tdir;

				if(P_TryWalk(actor))
					return;
			}
		}
	}
	else
	{
		for(tdir=DI_SOUTHEAST; tdir >= DI_EAST; tdir--)
		{
			if(tdir != turnaround)
			{
				actor->movedir = tdir;

				if(P_TryWalk(actor))
					return;
			}
		}
	}

	if(turnaround != DI_NODIR)
	{
		actor->movedir = turnaround;

		if(P_TryWalk(actor))
			return;
	}

	actor->movedir = DI_NODIR; // cannot move
}

/** Looks for players to chase after, aim at, or whatever.
  *
  * \param actor     The object looking for flesh.
  * \param allaround Look all around? If false, only players in a 180-degree
  *                  range in front will be spotted.
  * \return True if a player is found, otherwise false.
  * \sa P_SupermanLook4Players
  */
static boolean P_LookForPlayers(mobj_t* actor, boolean allaround)
{
	int c = 0, stop;
	player_t* player;
	sector_t* sector;
	angle_t an;
	fixed_t dist;

	if(cv_objectplace.value)
		return false;

	sector = actor->subsector->sector;

	// BP: first time init, this allow minimum lastlook changes
	if(actor->lastlook < 0)
		actor->lastlook = P_Random() % MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for(;; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
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

/** Looks for a player with a ring shield.
  * Used by rings.
  *
  * \param actor Ring looking for a shield to be attracted to.
  * \return True if a player with ring shield is found, otherwise false.
  * \sa A_AttractChase
  */
static boolean P_LookForShield(mobj_t* actor)
{
	int c = 0, stop;
	player_t* player;
	sector_t* sector;

	sector = actor->subsector->sector;

	// BP: first time init, this allow minimum lastlook changes
	if(actor->lastlook < 0)
		actor->lastlook = P_Random() % MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for(;; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
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

		if(player->powers[pw_ringshield]
			&& ((P_AproxDistance(actor->x-player->mo->x, actor->y-player->mo->y) < RING_DIST
			&& abs(player->mo->z-actor->z) < RING_DIST) || RING_DIST == 0))
		{
			actor->target = player->mo;
			return true;
		}
		else
			continue;
	}

	//return false;
}

//
// ACTION ROUTINES
//

/** Looks for players until one is found.
  *
  * \param actor Object on the lookout.
  */
void A_Look(mobj_t* actor)
{
	if(!P_LookForPlayers(actor, false))
		return;

	// go into chase state
	P_SetMobjState(actor, actor->info->seestate);
	A_PlaySeeSound(actor);
}

//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase(mobj_t* actor)
{
	int delta;

	if(actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if(actor->threshold)
	{
		if(!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if(actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if(delta > 0)
			actor->angle -= ANG90/2;
		else if(delta < 0)
			actor->angle += ANG90/2;
	}

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if(actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if(actor->info->meleestate && P_CheckMeleeRange(actor))
	{
		if(actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if(actor->info->missilestate)
	{
		if(actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if(multiplayer && !actor->threshold && !P_CheckSight(actor, actor->target)
		&& P_LookForPlayers(actor, true))
		return; // got a new target

	// chase towards player
	if(--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

void A_SkimChase(mobj_t* actor)
{
	int delta;

	if(actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if(actor->threshold)
	{
		if(!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if(actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if(delta > 0)
			actor->angle -= ANG90/2;
		else if(delta < 0)
			actor->angle += ANG90/2;
	}

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		P_LookForPlayers(actor, true);

		// the spawnstate for skims already calls this function so just return either way
		// without changing state
		return;
	}

	// do not attack twice in a row
	if(actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if(actor->info->meleestate && P_CheckMeleeRange(actor))
	{
		if(actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if(actor->info->missilestate)
	{
		if(actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if(multiplayer && !actor->threshold && !P_CheckSight(actor, actor->target)
		&& P_LookForPlayers(actor, true))
		return; // got a new target

	// chase towards player
	if(--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

/** Immediately turns an actor in the direction of its target.
  *
  * \param actor The actor.
  */
void A_FaceTarget(mobj_t* actor)
{
	if(!actor->target)
		return;

	actor->flags &= ~MF_AMBUSH;

	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
}

void A_CyberAttack(mobj_t* actor)
{
	fixed_t x, y, z;

	if(!actor->target)
		return;

	A_FaceTarget(actor);

	if(P_Random() & 1)
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle-ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle-ANG90, 43*FRACUNIT);
	}
	else
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle+ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle+ANG90, 43*FRACUNIT);
	}

	z = actor->z + 48*FRACUNIT;

	P_SpawnXYZMissile(actor, actor->target, MT_ROCKET, x, y, z);

	if(!(actor->flags & MF_BOSS))
	{
		if(gameskill <= sk_medium)
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE;
	}
}

//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED (20*FRACUNIT)

void A_SkullAttack(mobj_t* actor)
{
	mobj_t* dest;
	angle_t an;
	int dist;

	if(!actor->target)
		return;

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if(actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
	A_FaceTarget(actor);
	an = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul(SKULLSPEED, finecosine[an]);
	actor->momy = FixedMul(SKULLSPEED, finesine[an]);
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / SKULLSPEED;

	if(dist < 1)
		dist = 1;
	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;
}

void A_BossZoom(mobj_t* actor)
{
	mobj_t* dest;
	angle_t an;
	int dist;

	if(!actor->target)
		return;

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if(actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
	A_FaceTarget(actor);
	an = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul(actor->info->speed*5*FRACUNIT, finecosine[an]);
	actor->momy = FixedMul(actor->info->speed*5*FRACUNIT, finesine[an]);
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / (actor->info->speed*5*FRACUNIT);

	if(dist < 1)
		dist = 1;
	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;
}

/** Spawns explosions and plays appropriate sounds around the defeated boss.
  *
  * \param actor The boss.
  */
void A_BossScream(mobj_t* actor)
{
	fixed_t x, y, z;

	actor->movecount += actor->info->speed*16;
	x = (fixed_t)(actor->x + cos(actor->movecount * deg2rad) * actor->info->radius);
	y = (fixed_t)(actor->y + sin(actor->movecount * deg2rad) * actor->info->radius);

	if(actor->movecount >= 360)
		actor->movecount -= 360;

	z = actor->z - 8*FRACUNIT + ((P_Random()<<FRACBITS) / 4);
	if(actor->info->deathsound) S_StartSound(P_SpawnMobj(x, y, z, MT_BOSSEXPLODE), actor->info->deathsound);
}

/** Starts an actor's death sound.
  *
  * \param actor The actor.
  * \todo For more modularity, don't hardcode ::MT_SHELL and ::MT_FIREBALL
  *       here; use a bit in the info tables.
  */
void A_Scream(mobj_t* actor)
{
	if(actor->tracer && (actor->tracer->type == MT_SHELL || actor->tracer->type == MT_FIREBALL))
		S_StartScreamSound(actor, sfx_lose);
	else if(actor->info->deathsound)
		S_StartScreamSound(actor, actor->info->deathsound);
}

/** Starts an actor's pain sound.
  *
  * \param actor The actor.
  */
void A_Pain(mobj_t* actor)
{
	if(actor->info->painsound)
		S_StartSound(actor, actor->info->painsound);
}

/** Changes a dying object's flags to reflect its having fallen to the ground.
  * Change this if corpse objects are meant to be obstacles.
  *
  * \param actor The dying object.
  */
void A_Fall(mobj_t* actor)
{
	// actor is on ground, it can be walked over
	actor->flags &= ~MF_SOLID;

	actor->flags |= MF_NOCLIP;
	actor->flags |= MF_NOGRAVITY;
	actor->flags |= MF_FLOAT;

	// So change this if corpse objects
	// are meant to be obstacles.
}

//
// Searches around for the closest player and changes its
// graphic to match that of the player!
//
void A_1upThinker(mobj_t* actor)
{
	int i;
	fixed_t dist = MAXINT;
	fixed_t temp;
	int closestplayer = 0;

	for(i=0; i<MAXPLAYERS; i++)
	{
		if(!playeringame[i])
			continue;

		if(!players[i].mo)
			continue;

		temp = P_AproxDistance(players[i].mo->x-actor->x, players[i].mo->y-actor->y);

		if(temp < dist)
		{
			closestplayer = i;
			dist = temp;
		}
	}

	if(players[closestplayer].boxindex != 0)
		P_SetMobjState(actor, S_PRUPAUX1+(players[closestplayer].boxindex*3)-3);
}

/** Falls, screams, and spawns an exploding monitor.
  *
  * \param actor The popped monitor.
  * \todo Cleanup, modularize, outsource some of the functionality (respawning
  *       chances shouldn't be here).
  */
void A_MonitorPop(mobj_t* actor)
{
	mobj_t* remains;
	mobjtype_t item;
	int random;
	mobjtype_t newbox;

	// de-solidify
	P_UnsetThingPosition(actor);
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
	actor->flags |= MF_NOBLOCKMAP;
	P_SetThingPosition(actor);

	remains = P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->speed);
	remains->type = actor->type; // Transfer type information
	remains->flags = actor->flags; // Transfer flags
	remains->fuse = actor->fuse; // Transfer respawn timer
	remains->threshold = 68;
	actor->flags2 |= MF2_BOSSNOTRAP; // Dummy flag to mark this as an exploded TV until it respawns
	actor->tracer = remains;

	if(actor->info->deathsound) S_StartSound(remains, actor->info->deathsound);

	switch(actor->type)
	{
		case MT_QUESTIONBOX: // Random!
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
			if(cv_eggmanbox.value)
			{
				oldi = i;
				for(; i < oldi + cv_eggmanbox.value; i++)
				{
					spawnchance[i] = MT_EGGMANBOX;
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
				|| cv_superring.value || cv_eggmanbox.value || cv_teleporters.value))
			{
				CONS_Printf("Note: All monitors turned off.\n");
				return;
			}

			newbox = spawnchance[random%numchoices];
			item = mobjinfo[newbox].damage;

			remains->flags &= ~MF_AMBUSH;
			break;
		}
		default:
			item = actor->info->damage;
			break;
	}

	if(item != 0)
	{
		mobj_t* newmobj;
		newmobj = P_SpawnMobj(actor->x, actor->y, actor->z + 13*FRACUNIT, item);
		newmobj->target = actor->target; // Transfer target

		if(item == MT_1UPICO && newmobj->target->player)
		{
			if(newmobj->target->player->boxindex != 0)
			{
				P_SetMobjState(newmobj, S_PRUPAUX2+(newmobj->target->player->boxindex*3)-3);
			}
		}
	}
	else
		CONS_Printf("Powerup item not defined in 'damage' field for A_MonitorPop\n");

	P_SetMobjState(actor, S_DISS);
}

/** Explodes an object, doing damage to any objects nearby.
  *
  * \param actor The exploding object. Its target is used as the source (the
  *              cause of the explosion) and its damage value is used as the
  *              amount of damage to be dealt.
  */
void A_Explode(mobj_t* actor)
{
	P_RadiusAttack(actor, actor->target, actor->info->damage);
}

/** Follows a state and returns the last state in the pattern.
  * Be careful what value you send this function; it can cause an infinite
  * loop.
  *
  * \param state The state to start with.
  */
static state_t* P_FinalState(statenum_t state)
{
	while(states[state].tics != -1)
		state = states[state].nextstate;

	return &states[state];
}

//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
void A_BossDeath(mobj_t* mo)
{
	thinker_t* th;
	mobj_t* mo2;
	line_t junk;
	int i;

	if(mo->type == MT_EGGMOBILE || mo->type == MT_EGGMOBILE2)
	{
		if(mo->flags2 & MF2_CHAOSBOSS)
		{
			mo->health = 0;
			P_SetMobjState(mo, S_DISS);
			return;
		}
	}

	// make sure there is a player alive for victory
	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i] && players[i].health > 0)
			break;

	if(i == MAXPLAYERS)
		return; // no one left alive, so do not end game

	// scan the remaining thinkers to see
	// if all bosses are dead
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t*)th;
		if(mo2 != mo && mo2->type == mo->type && mo2->state != P_FinalState(mo->info->deathstate))
			return; // other boss not dead
	}

	// victory!
	if(mo->type == MT_EGGMOBILE || mo->type == MT_EGGMOBILE2)
	{
		if(mo->flags2 & MF2_BOSSNOTRAP)
		{
			for(i = 0; i < MAXPLAYERS; i++)
				P_DoPlayerExit(&players[i]);
		}
		else
		{
			// Bring the egg trap up to the surface
			junk.tag = 680;
			EV_DoElevator(&junk, elevateHighest, false);
			junk.tag = 681;
			EV_DoElevator(&junk, elevateUp, false);
			junk.tag = 682;
			EV_DoElevator(&junk, elevateHighest, false);
		}

		// Stop exploding and prepare to run.
		P_SetMobjState(mo, mo->info->xdeathstate);

		mo->target = NULL;

		// Flee! Flee! Find a point to escape to! If none, just shoot upward!
		// scan the thinkers to find the runaway point
		for(th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if(th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t*)th;

			if(mo2->type == MT_BOSSFLYPOINT)
				mo->target = mo2;
		}

		mo->flags |= MF_NOGRAVITY|MF_NOCLIP;

		if(mo->target)
		{
			if(mo->z < mo->floorz + 64*FRACUNIT)
				mo->momz = 2*FRACUNIT;
			mo->angle = R_PointToAngle2(mo->x, mo->y, mo->target->x, mo->target->y);
			mo->flags2 |= MF2_BOSSFLEE;
			mo->momz = FixedMul(FixedDiv(mo->target->z - mo->z, P_AproxDistance(mo->x-mo->target->x,mo->y-mo->target->y)), 2*FRACUNIT);
		}
		else
			mo->momz = 2*FRACUNIT;

		if(mo->type == MT_EGGMOBILE2)
		{
			mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle - ANG90, 32*FRACUNIT),
				mo->y + P_ReturnThrustY(mo, mo->angle-ANG90, 24*FRACUNIT),
				mo->z + mo->height/2 - 8*FRACUNIT, MT_BOSSTANK1); // Right tank
			mo2->angle = mo->angle;
			P_InstaThrust(mo2, mo2->angle - ANG90, 4*FRACUNIT);
			mo2->momz = 4*FRACUNIT;

			mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle + ANG90, 32*FRACUNIT),
				mo->y + P_ReturnThrustY(mo, mo->angle-ANG90, 24*FRACUNIT),
				mo->z + mo->height/2 - 8*FRACUNIT, MT_BOSSTANK2); // Left tank
			mo2->angle = mo->angle;
			P_InstaThrust(mo2, mo2->angle + ANG90, 4*FRACUNIT);
			mo2->momz = 4*FRACUNIT;

			P_SpawnMobj(mo->x, mo->y, mo->z + mo->height + 32*FRACUNIT, MT_BOSSSPIGOT)->momz = 4*FRACUNIT;
			return;
		}
	}
	else if(mariomode && mo->type == MT_KOOPA)
	{
		junk.tag = 650;
		EV_DoCeiling(&junk, raiseToHighest);
		return;
	}
}

/** Provides a custom powerup.
  *
  * \param actor The powerup object. Its target, which must be a player mobj,
  *              is awarded the powerup. Its reactiontime setting is used as an
  *              index into the player powers array.
  */
void A_CustomPower(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	if(actor->info->reactiontime >= NUMPOWERS)
	{
		CONS_Printf("Power #%d out of range!\n", actor->info->reactiontime);
		return;
	}

	player = actor->target->player;

	player->powers[actor->info->reactiontime] = actor->info->painchance;
	if(actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player a jump shield.
  *
  * \param actor The shield object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_JumpShield(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	if(!(player->powers[pw_jumpshield]))
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_WHITEORB)
			->target = player->mo;

	player->powers[pw_jumpshield] = true;
	player->powers[pw_fireshield] = player->powers[pw_bombshield] = false;
	player->powers[pw_watershield] = player->powers[pw_ringshield] = false;
	S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player a ring shield.
  *
  * \param actor The shield object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_RingShield(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	if(!(player->powers[pw_ringshield]))
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_YELLOWORB)
			->target = player->mo;

	player->powers[pw_ringshield] = true;
	player->powers[pw_bombshield] = player->powers[pw_watershield] = false;
	player->powers[pw_fireshield] = player->powers[pw_jumpshield] = false;
	S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player 10 rings.
  *
  * \param actor The ringbox object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_RingBox(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	P_GivePlayerRings(player, actor->info->reactiontime, false);
	if(actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player invincibility.
  *
  * \param actor The invincibility monitor.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_Invincibility(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;
	player->powers[pw_invulnerability] = invulntics + 1;

	if(((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]) && !player->powers[pw_super])
	{
		S_StopMusic();
		if(mariomode)
			S_ChangeMusic(mus_minvnc, false);
		else
			S_ChangeMusic(mus_invinc, false);
	}
}

/** Awards the player super sneakers.
  *
  * \param actor The sneakers monitor.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_SuperSneakers(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	actor->target->player->powers[pw_sneakers] = sneakertics + 1;

	if((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer])
	{
		if(!player->powers[pw_super] && !mapheaderinfo[gamemap-1].nossmusic)
		{
			S_StopMusic();
			S_ChangeMusic(mus_shoes, false);
		}
	}
}

/** Awards the player an extra life.
  *
  * \param actor The life monitor.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_ExtraLife(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	P_GivePlayerLives(player, 1);

	if(mariomode)
		S_StartSound(player->mo, sfx_marioa);
	else
	{
		player->powers[pw_extralife] = extralifetics + 1;

		if((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer])
		{
			S_StopMusic();
			S_ChangeMusic(mus_xtlife, false);
		}
	}
}

/** Awards the player a bomb shield.
  *
  * \param actor The shield object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_BombShield(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	if(!(player->powers[pw_bombshield]))
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BLACKORB)
			->target = player->mo;

	player->powers[pw_bombshield] = true;
	player->powers[pw_watershield] = player->powers[pw_fireshield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;
	S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player a water shield.
  *
  * \param actor The shield object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_WaterShield(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	if(!(player->powers[pw_watershield]))
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BLUEORB)
			->target = player->mo;

	player->powers[pw_watershield] = true;
	player->powers[pw_bombshield] = player->powers[pw_fireshield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;
	if(((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]) && player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1)
	{
		if(player->powers[pw_super] && !mapheaderinfo[gamemap-1].nossmusic)
			S_ChangeMusic(mus_supers, true);
		else if(player->powers[pw_invulnerability])
		{
			if(mariomode)
				S_ChangeMusic(mus_minvnc, false);
			else
				S_ChangeMusic(mus_invinc, false);
		}
		else if(player->powers[pw_sneakers])
			S_ChangeMusic(mus_shoes, false);
		else
			S_ChangeMusic(mapmusic & 2047, true);
	}

	player->powers[pw_underwater] = 0;

	if(player->powers[pw_spacetime] > 1)
	{
		player->powers[pw_spacetime] = 0;

		if((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer])
		{
			if(player->powers[pw_super] && !mapheaderinfo[gamemap-1].nossmusic)
				S_ChangeMusic(mus_supers, true);
			else if(player->powers[pw_invulnerability])
				if(mariomode)
					S_ChangeMusic(mus_minvnc, false);
				else
					S_ChangeMusic(mus_invinc, false);
			else if(player->powers[pw_sneakers])
				S_ChangeMusic(mus_shoes, false);
			else
				S_ChangeMusic(mapmusic & 2047, true);
		}
	}
	S_StartSound(player->mo, actor->info->seesound);
}

/** Awards the player a fire shield.
  *
  * \param actor The shield object.
  * \todo Remove this function in favor of something like A_CustomPower().
  * \sa A_CustomPower
  */
void A_FireShield(mobj_t* actor)
{
	player_t* player;

	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	if(!(player->powers[pw_fireshield]))
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_REDORB)
			->target = player->mo;

	player->powers[pw_fireshield] = true;
	player->powers[pw_bombshield] = player->powers[pw_watershield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;
	S_StartSound(player->mo, actor->info->seesound);
}

void A_ScoreRise(mobj_t* actor)
{
	actor->momz = actor->info->speed; // make logo rise!
}

/** Makes an actor hop like a bunny, or squirrel.
  *
  * \param actor The hopping actor.
  */
void A_BunnyHop(mobj_t* actor)
{
	if(actor->z <= actor->floorz)
	{
		actor->momz = 6*FRACUNIT; // make it hop!
		actor->angle += P_Random()*FINEANGLES;
		P_InstaThrust(actor, actor->angle, 3*FRACUNIT); // Launch the hopping action! PHOOM!!
	}
}

/** Spawns a randomly sized bubble from the actor's location.
  * Bubbles are not spawned if the actor is not underwater.
  *
  * \param actor Bubble spawning source.
  * \sa A_BubbleRise
  */
void A_BubbleSpawn(mobj_t* actor)
{
	if(!(actor->eflags & MF_UNDERWATER))
	{
		// Don't draw or spawn bubbles above water
		actor->flags2 |= MF2_DONTDRAW;
		return;
	}

	actor->flags2 &= ~MF2_DONTDRAW;

	if(leveltime % (3*TICRATE) < 8)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_EXTRALARGEBUBBLE);
	else if(P_Random () > 128)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_SMALLBUBBLE);
	else if(P_Random () < 128 && P_Random () > 96)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_MEDIUMBUBBLE);
//	else if(P_Random () > 96 && P_Random () < 160)
//		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_EXTRALARGEBUBBLE);
}

/** Raises a bubble.
  *
  * \param actor The bubble.
  * \sa A_BubbleSpawn, A_BubbleCheck
  */
void A_BubbleRise(mobj_t* actor)
{
	if(actor->type == MT_EXTRALARGEBUBBLE)
		actor->momz = (fixed_t)(1.2f*FRACUNIT); // make bubbles rise!
	else
	{
		actor->momz += 1024; // make bubbles rise!

		// Move around slightly to make it look like it's bending around the water
		if(P_Random() < 32)
		{
			P_InstaThrust(actor, P_Random() & 1 ? actor->angle + ANG90 : actor->angle,
				P_Random() & 1? FRACUNIT/2 : -FRACUNIT/2);
		}
		else if(P_Random() < 32)
		{
			P_InstaThrust(actor, P_Random() & 1 ? actor->angle - ANG90 : actor->angle - ANG180,
				P_Random() & 1? FRACUNIT/2 : -FRACUNIT/2);
		}
	}
}

/** Checks if a bubble should be drawn or not.
  * Underwater bubbles are not drawn.
  *
  * \param actor The bubble to check.
  * \sa A_BubbleRise
  */
void A_BubbleCheck(mobj_t* actor)
{
	if(actor->eflags & MF_UNDERWATER)
		actor->flags2 &= ~MF2_DONTDRAW; // underwater so draw
	else
		actor->flags2 |= MF2_DONTDRAW; // above water so don't draw
}

/** Makes a ring chase after a player with a ring shield and causes
  * flingrings (spilled rings) to flicker.
  *
  * \param actor The ring or flingring.
  */
void A_AttractChase(mobj_t* actor)
{
	if(actor->flags2 & MF2_NIGHTSPULL)
		return;

	// spilled rings flicker before disappearing
	if(leveltime & 1 && actor->type == (mobjtype_t)actor->info->reactiontime && actor->fuse && actor->fuse < 2*TICRATE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if(actor->target && actor->target->player
		&& !actor->target->player->powers[pw_ringshield] && actor->type != (mobjtype_t)actor->info->reactiontime)
	{
		mobj_t* newring;
		newring = P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->reactiontime);
		newring->flags |= MF_COUNTITEM;
		newring->momx = actor->momx;
		newring->momy = actor->momy;
		newring->momz = actor->momz;
		P_SetMobjState(actor, S_DISS);
	}

	P_LookForShield(actor); // Go find 'em, boy!

	actor->tracer = actor->target;

	if(!actor->target)
	{
		actor->target = actor->tracer = NULL;
		return;
	}

	if(!actor->target->player)
		return;

	if(!actor->target->health)
		return;

	// If a FlingRing gets attracted by a shield, change it into a normal
	// ring, but don't count towards the total.
	if(actor->type == (mobjtype_t)actor->info->reactiontime)
	{
		P_SetMobjState(actor, S_DISS);
		if(actor->flags & MF_COUNTITEM)
			P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->painchance);
		else
			P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->painchance)
				->flags &= ~MF_COUNTITEM;
	}

	P_Attract(actor, actor->tracer, false);
}

/** Drops a mine.
  *
  * \param actor A Skim interested in dropping mines. Its raisestate, usually
  *              ::MT_MINE, is used for the object dropped.
  */
void A_DropMine(mobj_t* actor)
{
	// Use raisestate instead of MT_MINE
	P_SpawnMobj(actor->x, actor->y, actor->z - 12*FRACUNIT, actor->info->raisestate)
		->momz = actor->momz + actor->pmomz;
}

/** Makes the stupid harmless fish in Greenflower Zone jump.
  *
  * \param actor A dumb fish.
  */
void A_FishJump(mobj_t* actor)
{
	if((actor->z <= actor->floorz) || (actor->z <= actor->watertop - (64 << FRACBITS)))
	{
			actor->momz = (fixed_t)((actor->threshold*FRACUNIT)/4);
			P_SetMobjState(actor, actor->info->seestate);
	}

	if(actor->momz < 0)
		P_SetMobjState(actor, actor->info->meleestate);
}

/** Changes the state of a level end sign to reflect the player who hit it.
  *
  * \param actor The sign.
  */
void A_SignPlayer(mobj_t* actor)
{
	if(!actor->target)
		return;

	if(!actor->target->player)
		return;

	actor->state->nextstate = actor->info->seestate+actor->target->player->skin;
}

// Thrown ring thinker/sparkle trail
void A_ThrownRing(mobj_t* actor)
{
	int c;
	int stop;
	player_t* player;
	sector_t* sector;

	if(leveltime % 5 == 1)
	{
		if(actor->flags2 & MF2_EXPLOSION)
			P_SpawnMobj(actor->x, actor->y, actor->z, MT_SMOK);
		else if(!(actor->flags2 & MF2_RAILRING))
			P_SpawnMobj(actor->x, actor->y, actor->z, MT_SPARK);
	}

	// spilled rings flicker before disappearing
	if(leveltime & 1 && actor->fuse > 0 && actor->fuse < 2*TICRATE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if(actor->tracer && actor->tracer->health <= 0)
		actor->tracer = NULL;

	// Updated homing ring special capability
	// If you have a ring shield, all rings thrown
	// at you become homing (except rail)!

	// A non-homing ring getting attracted by a
	// magnetic player. If he gets too far away, make
	// sure to stop the attraction!
	if(actor->tracer &&	actor->tracer->player->powers[pw_ringshield]
		&& !(actor->flags2 & MF2_HOMING)
		&& P_AproxDistance(P_AproxDistance(actor->tracer->x-actor->x,
		actor->tracer->y-actor->y), actor->tracer->z-actor->z) > RING_DIST)
	{
		actor->tracer = NULL;
	}

	if((actor->tracer)
		&& ((actor->flags2 & MF2_HOMING) || actor->tracer->player->powers[pw_ringshield]))// Already found someone to follow.
	{
		int temp;

		temp = actor->threshold;
		actor->threshold = 32000;
		P_HomingAttack(actor, actor->tracer);
		actor->threshold = temp;
		return;
	}

	sector = actor->subsector->sector;

	// first time init, this allow minimum lastlook changes
	if(actor->lastlook < 0)
		actor->lastlook = P_Random () % MAXPLAYERS;

	c = 0;
	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for(;; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if(actor->lastlook == stop)
			return;

		if(!playeringame[actor->lastlook])
			continue;

		if(c++ == 2)
			return;

		player = &players[actor->lastlook];

		if(!player->mo)
			continue;

		if(player->mo->health <= 0)
			continue; // dead

		if(actor->target && actor->target->player)
		{
			if(player->mo == actor->target)
				continue;

			// Don't home in on teammates.
			if(gametype == GT_CTF 
				&& actor->target->player->ctfteam == player->ctfteam)
				continue;
		}

		// check distance
		if(actor->flags2 & MF2_RAILRING)
		{
			if(P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
				player->mo->y-actor->y), player->mo->z-actor->z) > RING_DIST/2)
			{
				continue;
			}
		}
		else if(P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
			player->mo->y-actor->y), player->mo->z-actor->z) > RING_DIST)
		{
			continue;
		}

		// do this after distance check because it's more computationally expensive
		if(!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if((actor->flags2 & MF2_HOMING) || player->powers[pw_ringshield] == true)
			actor->tracer = player->mo;
		return;
	}

	return;
}

void A_SetSolidSteam(mobj_t* actor)
{
	actor->flags &= ~MF_NOCLIP;
	actor->flags |= MF_SOLID;
	if(!(P_Random() % 8))
	{
		if(actor->info->deathsound)
			S_StartSound(actor, actor->info->deathsound); // Hiss!
	}
	else
	{
		if(actor->info->painsound)
			S_StartSound(actor, actor->info->painsound);
	}
	actor->momz++;
}

void A_UnsetSolidSteam(mobj_t* actor)
{
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
}

void A_JetChase(mobj_t* actor)
{
	fixed_t thefloor;

	if(actor->flags & MF_AMBUSH)
		return;

	if(actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if(actor->reactiontime)
		actor->reactiontime--;

	if(P_Random() % 32 == 1)
	{
		actor->momx = actor->momx / 2;
		actor->momy = actor->momy / 2;
		actor->momz = actor->momz / 2;
	}

	// Bounce if too close to floor or ceiling -
	// ideal for Jetty-Syns above you on 3d floors
	if(actor->momz && ((actor->z - (32<<FRACBITS)) < thefloor) && !((thefloor + 32*FRACUNIT + actor->height) > actor->ceilingz))
		actor->momz = -actor->momz;

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// modify target threshold
	if(actor->threshold)
	{
		if(!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	// check for melee attack
	if(actor->type == MT_JETTBOMBER && (actor->z > (actor->floorz + (32<<FRACBITS)))
		&& P_CheckMeleeRange (actor) && !actor->reactiontime
		&& (actor->target->z >= actor->floorz))
	{
		if(actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		// use raisestate instead of MT_MINE
		P_SpawnMobj(actor->x, actor->y, actor->z - (32<<FRACBITS), actor->info->raisestate)->target = actor;
		actor->reactiontime = TICRATE; // one second
	}

	if((multiplayer || netgame) && !actor->threshold && !P_CheckSight (actor, actor->target))
		if(P_LookForPlayers(actor, true))
			return; // got a new target

	// If the player is over 3072 fracunits away, then look for another player
	if(P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > 3072*FRACUNIT && P_LookForPlayers(actor, true))
	{
		return; // got a new target
	}

	// chase towards player
	if(gameskill <= sk_easy)
		P_Thrust(actor, actor->angle, actor->info->speed/6);
	else if(gameskill < sk_hard)
		P_Thrust(actor, actor->angle, actor->info->speed/4);
	else
		P_Thrust(actor, actor->angle, actor->info->speed/2);

	// must adjust height
	if(gameskill <= sk_medium)
	{
		if(actor->z < (actor->target->z + actor->target->height + (32<<FRACBITS)))
			actor->momz += FRACUNIT/2;
		else
			actor->momz -= FRACUNIT/2;
	}
	else
	{
		if(actor->z < (actor->target->z + actor->target->height + (64<<FRACBITS)))
			actor->momz += FRACUNIT/2;
		else
			actor->momz -= FRACUNIT/2;
	}
}

void A_JetbThink(mobj_t* actor)
{
	sector_t* nextsector;

	fixed_t thefloor;

	if(actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if(actor->target)
		A_JetChase (actor);
	else if(((actor->z - (32<<FRACBITS)) < thefloor) && !((thefloor + (32<<FRACBITS) + actor->height) > actor->ceilingz))
			actor->z = thefloor+(32<<FRACBITS);

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if(nextsector->ceilingheight < actor->height)
		actor->momz -= 5*FRACUNIT;
	else if(nextsector->floorheight > actor->z)
		actor->momz += 5*FRACUNIT;
}

void A_JetgShoot(mobj_t* actor)
{
	if(!actor->target)
		return;

	if(actor->reactiontime)
		return;

	if(P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y), actor->target->z - actor->z) > actor->info->painchance*FRACUNIT)
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, actor->info->raisestate);

	if(gameskill <= sk_medium)
		actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	else
		actor->reactiontime = actor->info->reactiontime*TICRATE;
	
	if(actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

void A_ShootBullet(mobj_t* actor)
{
	if(!actor->target)
		return;

	if(P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y), actor->target->z - actor->z) > actor->info->painchance*FRACUNIT)
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, actor->info->raisestate);

	if(actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

void A_JetgThink(mobj_t* actor)
{
	sector_t* nextsector;

	fixed_t thefloor;

	if(actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if(actor->target)
	{
		if(P_Random() <= 32 && !actor->reactiontime)
			P_SetMobjState(actor, actor->info->missilestate);
		else
			A_JetChase (actor);
	}
	else if(actor->z - (32<<FRACBITS) < thefloor && !(thefloor + (32<<FRACBITS)
		+ actor->height > actor->ceilingz))
	{
		actor->z = thefloor + (32<<FRACBITS);
	}

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if(nextsector->ceilingheight < actor->height)
		actor->momz -= 5*FRACUNIT;
	else if(nextsector->floorheight > actor->z)
		actor->momz += 5*FRACUNIT;
}

void A_MouseThink(mobj_t* actor)
{
	if(actor->reactiontime)
		actor->reactiontime--;

	if(actor->z == actor->floorz && !actor->reactiontime)
	{
		if(P_Random() & 1)
			actor->angle += ANG90;
		else
			actor->angle -= ANG90;

		P_InstaThrust(actor, actor->angle, actor->info->speed);
		actor->reactiontime = TICRATE/5;
	}
}

/** Chases a Deton after a player.
  *
  * \param actor A Deton.
  * \todo Move the heavy lifting to another function elsewhere.
  */
void A_DetonChase(mobj_t* actor)
{
/*
Distance between two points in 3D space:

dX = X2-X1
dY = Y2-Y1
dZ = Z2-Z1

Distance = Sqrt(dX^2 + dY^2 + dZ^2)

	actor->momx = abs(actor->x - actor->target->x)/5;
	actor->momy = abs(actor->y - actor->target->y)/5;
	actor->momz = abs(actor->z - actor->target->z)/5;
*/
	angle_t exact;
	fixed_t xydist, dist;
	mobj_t* oldtarget;

	oldtarget = actor->target;

	// modify target threshold
	if(!actor->target || actor->target->health <= 0)
		actor->threshold = 0;
	else
		actor->threshold = 1;

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	if(multiplayer && !actor->threshold && P_LookForPlayers(actor, true))
		return; // got a new target

	// Face movement direction if not doing so
	exact = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
	actor->angle = exact;
	if(exact != actor->angle)
	{
		if(exact - actor->angle > ANG180)
		{
			actor->angle -= actor->info->raisestate;
			if(exact - actor->angle < ANG180)
				actor->angle = exact;
		}
		else
		{
			actor->angle += actor->info->raisestate;
			if(exact - actor->angle > ANG180)
				actor->angle = exact;
		}
	}
	// movedir is up/down angle: how much it has to go up as it goes over to the player
	xydist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);
	exact = R_PointToAngle2(actor->x, actor->z, actor->x + xydist, actor->target->z);
	actor->movedir = (int)exact;
	if(exact != (angle_t)actor->movedir)
	{
		if(exact - (angle_t)actor->movedir > ANG180)
		{
			actor->movedir -= (int)actor->info->raisestate;
			if(exact - (angle_t)actor->movedir < ANG180)
				actor->movedir = (int)exact;
		}
		else
		{
			actor->movedir += (int)actor->info->raisestate;
			if(exact - (angle_t)actor->movedir > ANG180)
				actor->movedir = (int)exact;
		}
	}

	// check for melee attack
	if(P_CheckMeleeRange(actor))
	{
		P_ExplodeMissile(actor);
		P_RadiusAttack(actor, actor, 96);
		return;
	}

	// chase towards player
	if((dist = P_AproxDistance(xydist, actor->target->z-actor->z))
		> (actor->info->painchance << FRACBITS))
	{
		actor->target = NULL; // Too far away
		return;
	}

	if(actor->reactiontime == 0)
	{
		actor->reactiontime = actor->info->reactiontime;
		return;
	}

	if(actor->reactiontime > 1)
	{
		actor->reactiontime--;
		return;
	}

	if(actor->reactiontime > 0)
	{
		actor->reactiontime = -42;

		if(actor->info->seesound)
			S_StartScreamSound(actor, actor->info->seesound);
	}

	if(actor->reactiontime == -42)
	{
		fixed_t xyspeed;

		actor->reactiontime = -42;
		actor->tracer = actor->target;

		exact = ((angle_t)actor->movedir)>>ANGLETOFINESHIFT;
		xyspeed = FixedMul(actor->target->player->normalspeed*FRACUNIT*75/100, finecosine[exact]);
		actor->momz = FixedMul(actor->target->player->normalspeed*FRACUNIT*75/100, finesine[exact]);

		exact = actor->angle>>ANGLETOFINESHIFT;
		actor->momx = FixedMul(xyspeed, finecosine[exact]);
		actor->momy = FixedMul(xyspeed, finesine[exact]);
	}
}

/** Moves a fake little Super Sonic cape.
  *
  * \param actor The cape.
  */
void A_CapeChase(mobj_t* actor)
{
	fixed_t platx;
	fixed_t platy;

	if(actor->state == &states[S_DISS])
		return;

	if(!actor->target || (actor->target->health <= 0) || !actor->target->player || (actor->target->player && !actor->target->player->powers[pw_super]))
	{
		P_SetMobjState(actor, S_DISS);
		return;
	}
	else if(actor->target->player->powers[pw_super])
	{
		platx = P_ReturnThrustX(actor->target, actor->target->angle, -FRACUNIT*4);
		platy = P_ReturnThrustY(actor->target, actor->target->angle, -FRACUNIT*4);
		P_UnsetThingPosition (actor);
		actor->x = actor->target->x + platx;
		actor->y = actor->target->y + platy;
		actor->z = actor->target->z + actor->target->height/2;
		actor->angle = actor->target->angle;
		P_SetThingPosition (actor);
	}
}

/** Rotates a spike ball.
  *
  * \param actor The spike ball.
  */
void A_RotateSpikeBall(mobj_t* actor)
{
	double radius, conangle, sinangle;

	if(actor->type == MT_SPECIALSPIKEBALL)
		return;

	if(!actor->target) // This should NEVER happen.
	{
		CONS_Printf("Error: Spikeball has no target\n");
		P_SetMobjState(actor, S_DISS);
		return;
	}

	radius = 12.0*actor->info->speed;
	actor->angle += (fixed_t)((double)actor->info->speed/FRACUNIT);
	P_UnsetThingPosition(actor);
	{
		conangle = cos(actor->angle * deg2rad);
		sinangle = sin(actor->angle * deg2rad);
		actor->x = (fixed_t)(actor->target->x + conangle * radius);
		actor->y = (fixed_t)(actor->target->y + sinangle * radius);
		actor->z = (fixed_t)(actor->target->z + actor->target->height/2);
		P_SetThingPosition(actor);
	}
}

// Snow ball for Snow Buster
void A_SnowBall(mobj_t* actor)
{
	P_InstaThrust(actor, actor->angle, actor->info->speed);
	if(!actor->fuse)
		actor->fuse = 10*TICRATE;
}

void A_CrawlaCommanderThink(mobj_t* actor)
{
	fixed_t dist;
	sector_t* nextsector;
	fixed_t thefloor;

	if(actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if(actor->fuse & 1)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if(actor->reactiontime > 0)
		actor->reactiontime--;

	if(actor->fuse < 2)
	{
		actor->fuse = 0;
		actor->flags2 &= ~MF2_FRET;
	}

	// Hover mode
	if(actor->health > 1 || actor->fuse)
	{
		if(actor->z < thefloor + (16*FRACUNIT))
			actor->momz += FRACUNIT;
		else if(actor->z < thefloor + (32*FRACUNIT))
			actor->momz += FRACUNIT/2;
		else
			actor->momz += 16;
	}

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		if(actor->state != &states[actor->info->spawnstate])
			P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	dist = P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y);

	if(actor->target->player && actor->health > 1)
	{
		if(dist < 128*FRACUNIT
			&& (actor->target->player->mfjumped || actor->target->player->mfspinning))
		{
			// Auugh! He's trying to kill you! Strafe! STRAAAAFFEEE!!
			if(actor->target->momx || actor->target->momy)
			{
				P_InstaThrust(actor, actor->angle-ANG180, 20*FRACUNIT);
			}
			return;
		}
	}
/*
	if(actor->health < 2 && P_Random() < 2)
	{
		P_SpawnMissile (actor, actor->target, MT_JETTBULLET);
	}
*/
	// Face the player
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if(actor->threshold && dist > 256*FRACUNIT)
		actor->momx = actor->momy = 0;

	if(actor->reactiontime && actor->reactiontime <= 2*TICRATE && dist > actor->target->radius - FRACUNIT)
	{
		actor->threshold = 0;

		// Roam around, somewhat in the player's direction.
		actor->angle += (P_Random()<<10);
		actor->angle -= (P_Random()<<10);

		if(actor->health > 1)
			P_InstaThrust(actor, actor->angle, 10*FRACUNIT);
	}
	else if(!actor->reactiontime)
	{
		if(actor->health > 1) // Hover Mode
		{
			if(dist < 512*FRACUNIT)
			{
				actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
				P_InstaThrust(actor, actor->angle, 60*FRACUNIT);
				actor->threshold = 1;
			}
		}
		actor->reactiontime = 2*TICRATE + P_Random()/2;
	}

	if(actor->health == 1)
		P_Thrust(actor, actor->angle, 1);

	// Pogo Mode
	if(!actor->fuse && actor->health == 1 && actor->z <= actor->floorz)
	{
		if(dist < 256*FRACUNIT)
		{
			actor->momz = 15*FRACUNIT;
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
			P_InstaThrust(actor, actor->angle, 2*FRACUNIT);
			// pogo on player
		}
		else
		{
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_Random() & 1 ? -P_Random() : +P_Random());
			P_InstaThrust(actor, actor->angle, 10*FRACUNIT);
			actor->momz = 15*FRACUNIT; // Bounce up in air
		}
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if(nextsector->floorheight > actor->z && nextsector->floorheight - actor->z < 128*FRACUNIT)
		actor->momz += (nextsector->floorheight - actor->z) / 4;
}

// Spurt out rings in many directions
void A_RingExplode(mobj_t* actor)
{
	int i;
	mobj_t* mo;

	for(i = 0; i < 32; i++)
	{
		mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_REDRING);
		mo->target = actor->target; // Transfer target so player gets the points

		if(i > 15)
		{
			mo->momx = (fixed_t)(sin(i*22.5) * 30 * FRACUNIT);
			mo->momy = (fixed_t)(cos(i*22.5) * 30 * FRACUNIT);
			mo->momz = 30*FRACUNIT;
		}
		else
		{
			mo->momx = (fixed_t)(sin(i*22.5) * 30 * FRACUNIT);
			mo->momy = (fixed_t)(cos(i*22.5) * 30 * FRACUNIT);
		}

		mo->flags2 |= MF2_DEBRIS;
		mo->fuse = TICRATE/(OLDTICRATE/5);
	}

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_REDRING);

	mo->target = actor->target;
	mo->momz = 30*FRACUNIT;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/(OLDTICRATE/5);

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_REDRING);

	mo->target = actor->target;
	mo->momz = -30*FRACUNIT;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/(OLDTICRATE/5);

	return;
}

// Mix up all the player positions
void A_MixUp(mobj_t* actor)
{
	int i, numplayers = 0, random = 0;

	actor = NULL;
	if(!multiplayer)
		return;

	numplayers = 0;

	// Count the number of players in the game
	// and grab their xyz coords
	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i] && players[i].mo && players[i].playerstate == PST_LIVE
			&& !players[i].exiting)
		{
			numplayers++;
		}

	if(numplayers <= 1) // Not enough players to mix up.
		return;
	else if(numplayers == 2) // Special case -- simple swap
	{
		fixed_t x, y, z;
		angle_t angle;
		int one = -1, two = 0; // default value 0 to make the compiler shut up
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i] && players[i].mo && players[i].playerstate == PST_LIVE
				&& !players[i].exiting)
			{
				if(one == -1)
					one = i;
				else
				{
					two = i;
					break;
				}
			}

		x = players[one].mo->x;
		y = players[one].mo->y;
		z = players[one].mo->z;
		angle = players[one].mo->angle;

		P_MixUp(players[one].mo, players[two].mo->x, players[two].mo->y,
			players[two].mo->z, players[two].mo->angle);

		P_MixUp(players[two].mo, x, y, z, angle);
	}
	else
	{
		fixed_t position[MAXPLAYERS][3];
		angle_t anglepos[MAXPLAYERS];
		boolean picked[MAXPLAYERS];
		int pindex[MAXPLAYERS], counter = 0;

		for(i = 0; i < MAXPLAYERS; i++)
		{
			position[i][0] = position[i][1] = position[i][2] = anglepos[i] = pindex[i] = -1;
			picked[i] = false;
		}

		for(i = 0; i < MAXPLAYERS; i++)
		{
			if(playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && !players[i].exiting)
			{
				position[counter][0] = players[i].mo->x;
				position[counter][1] = players[i].mo->y;
				position[counter][2] = players[i].mo->z;
				pindex[counter] = i;
				anglepos[counter] = players[i].mo->angle;
				players[i].mo->momx = players[i].mo->momy = players[i].mo->momz =
					players[i].rmomx = players[i].rmomy = 1;
				players[i].cmomx = players[i].cmomy = 0;
				counter++;
			}
		}

		counter = 0;

		// Mix them up!
		for(;;)
		{
			if(counter > 255) // fail-safe to avoid endless loop
				break;
			random = P_Random();
			if(!(random % numplayers)) // Make sure it's not a useless mix
				break;
			counter++;
		}

		// Scramble!
		random %= numplayers; // I love modular arithmetic, don't you?
		counter = random;

		for(i = 0; i < MAXPLAYERS; i++)
		{
			if(playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && !players[i].exiting)
			{
				random = 0;
				while((pindex[counter] == i || picked[counter] == true)
					&& random < 255) // Failsafe
				{
					counter = P_Random() % numplayers;
					random++;
				}

				P_MixUp(players[i].mo, position[counter][0], position[counter][1],
					position[counter][2], anglepos[counter]);

				picked[counter] = true;
			}
		}
	}

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(playeringame[i] && players[i].mo && players[i].playerstate == PST_LIVE
			&& !players[i].exiting)
		{
			P_SetThingPosition(players[i].mo);
			P_CheckPosition(players[i].mo, players[i].mo->x, players[i].mo->y);
		}
	}

	// Play the 'bowrwoosh!' sound
	S_StartSound(NULL, sfx_mixup);
}

void A_PumaJump(mobj_t* actor)
{
	if((actor->z <= actor->floorz) || (actor->z <= actor->watertop-(64<<FRACBITS)))
	{
		actor->momz = (actor->threshold*FRACUNIT)/4;
	}

	if(actor->momz < 0
		&& (actor->state != &states[S_PUMA4] || actor->state != &states[S_PUMA5] || actor->state != &states[S_PUMA6]))
		P_SetMobjStateNF(actor, S_PUMA4);
	else if(actor->state != &states[S_PUMA1] || actor->state != &states[S_PUMA2] || actor->state != &states[S_PUMA3])
		P_SetMobjStateNF(actor, S_PUMA1);
}

//
// A_Boss1Chase
// it tries to close as fast as possible
//
void A_Boss1Chase(mobj_t* actor)
{
	int delta;

	if(actor->reactiontime)
		actor->reactiontime--;

	// turn towards movement direction if not there yet
	if(actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if(delta > 0)
			actor->angle -= ANG90/2;
		else if(delta < 0)
			actor->angle += ANG90/2;
	}

	// do not attack twice in a row
	if(actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	if(actor->movecount)
		goto nomissile;

	if(!P_CheckMissileRange(actor))
		goto nomissile;

	if(actor->reactiontime <= 0)
	{
		if(actor->health > actor->info->damage)
		{
			if(P_Random() & 1)
				P_SetMobjState(actor, actor->info->missilestate);
			else
				P_SetMobjState(actor, actor->info->meleestate);
		}
		else
			P_SetMobjState(actor, actor->info->raisestate);

		actor->flags2 |= MF2_JUSTATTACKED;
		actor->reactiontime = 2*TICRATE;
		return;
	}

	// ?
nomissile:
	// possibly choose another target
	if(multiplayer && P_Random() < 2)
	{
		if(P_LookForPlayers(actor, true))
			return; // got a new target
	}

	// chase towards player
	if(--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

// A_Boss2Chase
//
// Really doesn't 'chase', rather he goes in a circle.
//
void A_Boss2Chase(mobj_t* actor)
{
	int radius;
	boolean reverse = false;

	if(actor->health <= 0)
		return;

	// When reactiontime hits zero, he will go the other way
	if(actor->reactiontime)
		actor->reactiontime--;

	if(actor->reactiontime <= 0)
	{
		reverse = true;
		actor->reactiontime = 2*TICRATE + P_Random();
	}

	actor->target = P_GetClosestAxis(actor);

	if(!actor->target) // This should NEVER happen.
	{
		CONS_Printf("Error: Boss2 has no target!\n");
		A_BossDeath(actor);
		return;
	}

	if(reverse)
		actor->watertop = -actor->watertop;

	radius = 384*FRACUNIT;

	actor->target->angle += actor->watertop >> FRACBITS;
	P_UnsetThingPosition(actor);
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x + (fixed_t)(cos(actor->target->angle * deg2rad) * radius), actor->target->y + (fixed_t)(sin(actor->target->angle * deg2rad) * radius));
	actor->x = (fixed_t)(actor->target ->x + cos(actor->target->angle * deg2rad) * radius);
	actor->y = (fixed_t)(actor->target->y + sin(actor->target->angle * deg2rad) * radius);
	actor->z = actor->target->z + 64*FRACUNIT;
	P_SetThingPosition(actor);

	if(actor->angle > ANGLE_MAX)
		actor->angle -= ANGLE_MAX;

	// Spray goo once every second
	if(leveltime % 7 == 1)
	{
		mobj_t* goop;
		// actor->movedir is used to determine the last
		// direction goo was sprayed in. There are 8 possible
		// directions to spray. (45-degree increments)

		actor->movedir++;
		actor->movedir %= NUMDIRS;

		goop = P_SpawnMobj(actor->x, actor->y, actor->z+actor->height+56*FRACUNIT, actor->info->painchance);
		goop->momx = (fixed_t)(sin(actor->movedir*45.0) * 3 * FRACUNIT);
		goop->momy = (fixed_t)(cos(actor->movedir*45.0) * 3 * FRACUNIT);
		goop->momz = 4*FRACUNIT;
		goop->fuse = 30*TICRATE+P_Random();
		if(actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		if(P_Random() & 1)
		{
			goop->momx *= 2;
			goop->momy *= 2;
		}
		else if(P_Random() > 128)
		{
			goop->momx *= 3;
			goop->momy *= 3;
		}

		actor->flags2 |= MF2_JUSTATTACKED;
	}
}

// Pogo part of Boss 2 AI
//
void A_Boss2Pogo(mobj_t* actor)
{
	if(actor->z <= actor->floorz + 8*FRACUNIT && actor->momz <= 0)
	{
		P_SetMobjState(actor, actor->info->raisestate);
		// Pogo Mode
	}
	else if(actor->momz < 0 && actor->reactiontime)
	{
		mobj_t* goop;
		int i;
		// spray in all 8 directions!
		for(i = 0; i < 8; i++)
		{
			actor->movedir++;
			actor->movedir %= NUMDIRS;

			goop = P_SpawnMobj(actor->x, actor->y, actor->z+actor->height+56*FRACUNIT, actor->info->painchance);
			goop->momx = (fixed_t)(sin(actor->movedir*45.0) * 3 * FRACUNIT);
			goop->momy = (fixed_t)(cos(actor->movedir*45.0) * 3 * FRACUNIT);
			goop->momz = 4*FRACUNIT;

			if(gametype == GT_CHAOS)
				goop->fuse = 15*TICRATE;
			else
				goop->fuse = 30*TICRATE+P_Random();
		}
		actor->reactiontime = 0;
		if(actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);
		actor->flags2 |= MF2_JUSTATTACKED;
	}
}

// Special function for Boss 2 so you can't just sit and destroy him.
void A_Invinciblerize(mobj_t* actor)
{
	A_Pain(actor);
	actor->reactiontime = 1;
	actor->movecount = TICRATE;
}

void A_DeInvinciblerize(mobj_t* actor)
{
	actor->movecount = actor->state->tics+TICRATE;
}

void A_Boss2PogoSFX(mobj_t* actor)
{
	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		return;
	}

	// Boing!
	if(P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) < 256*FRACUNIT)
	{
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
		P_InstaThrust(actor, actor->angle, actor->info->speed/2);
		// pogo on player
	}
	else
	{
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_Random() & 1 ? -P_Random() : +P_Random());
		P_InstaThrust(actor, actor->angle, FixedMul(actor->info->speed,(5*FRACUNIT)/2));
	}
	if(actor->info->activesound) S_StartSound(actor, actor->info->activesound);
	actor->momz = 12*FRACUNIT; // Bounce up in air
	actor->reactiontime = 1;
}

void A_EggmanBox(mobj_t* actor)
{
	if(!actor->target || !actor->target->player)
	{
		CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	P_DamageMobj(actor->target, actor, actor, 1); // Ow!
}

void A_TurretFire(mobj_t* actor)
{
	int count = 0;

	while(P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if(P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < 2048*FRACUNIT)
		{
			actor->flags2 |= MF2_FIRING;
			break;
		}

		count++;
	}
}

void A_SuperTurretFire(mobj_t* actor)
{
	int count = 0;

	while(P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if(P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < 2048*FRACUNIT)
		{
			actor->flags2 |= MF2_FIRING;
			actor->flags2 |= MF2_SUPERFIRE;
			break;
		}

		count++;
	}
}

void A_TurretStop(mobj_t* actor)
{
	actor->flags2 &= ~MF2_FIRING;
	actor->flags2 &= ~MF2_SUPERFIRE;

	if(actor->target && actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
}

void A_SparkFollow(mobj_t* actor)
{
	int radius;

	if(actor->state == &states[S_DISS])
		return;

	if(!actor->target || (actor->target->health <= 0) || !actor->target->player || (actor->target->player && !actor->target->player->powers[pw_super]))
	{
		P_SetMobjState(actor, S_DISS);
		return;
	}

	radius = actor->info->speed;

	actor->angle += actor->info->damage;
	P_UnsetThingPosition(actor);
	actor->x = (fixed_t)(actor->target->x + cos(actor->angle * deg2rad) * radius);
	actor->y = (fixed_t)(actor->target->y + sin(actor->angle * deg2rad) * radius);
	actor->z = (fixed_t)(actor->target->z + actor->target->height/3 - actor->height);
	P_SetThingPosition(actor);
}

/** Makes an object slowly fly after a player, in the manner of a Buzz.
  *
  * \param actor A Buzz or equivalent.
  * \author Graue <graue@oceanbase.org>
  */
void A_BuzzFly(mobj_t* actor)
{
	if(actor->flags & MF_AMBUSH)
		return;

	if(actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if(actor->threshold)
	{
		if(!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	if(!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if(P_LookForPlayers(actor, true))
			return; // got a new target

		actor->momz = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if(actor->target->health <= 0 || (!actor->threshold && !P_CheckSight(actor, actor->target)))
	{
		if((multiplayer || netgame) && P_LookForPlayers(actor, true))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate); // Go back to looking around
		return;
	}

	// If the player is over 3072 fracunits away, then look for another player
	if(P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > 3072*FRACUNIT)
	{
		if(multiplayer || netgame)
			P_LookForPlayers(actor, true); // maybe get a new target

		return;
	}

	// chase towards player
	{
		int dist, realspeed;

		if(gameskill <= sk_easy)
			realspeed = actor->info->speed*4/5;
		else if(gameskill < sk_hard)
			realspeed = actor->info->speed;
		else
			realspeed = actor->info->speed*5/4;

		dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x,
			actor->target->y - actor->y), actor->target->z - actor->z);

		if(dist < 1)
			dist = 1;

		actor->momx = FixedMul(FixedDiv(actor->target->x - actor->x, dist), realspeed);
		actor->momy = FixedMul(FixedDiv(actor->target->y - actor->y, dist), realspeed);
		actor->momz = FixedMul(FixedDiv(actor->target->z - actor->z, dist), realspeed);

		if(actor->z+actor->momz >= actor->waterbottom && actor->watertop > actor->floorz
			&& actor->z+actor->momz > actor->watertop - 256*FRACUNIT)
		{
			actor->momz = 0;
			actor->z = actor->watertop;
		}
	}
}

/** Sets the actor's reaction time.
  * Useful for customizability.
  *
  * \param actor Actor.
  * \author Graue <graue@oceanbase.org>
  */
void A_SetReactionTime(mobj_t* actor)
{
	actor->reactiontime = actor->info->reactiontime;
}

/** Runs a linedef executor.
  *
  * \param actor The mobj sent with the linedef execution request. Its location
  *              is used to set the calling sector. The tag used is its state
  *              number (beginning from 0) plus 1000.
  * \sa A_LinedefExecute
  * \author Graue <graue@oceanbase.org>
  */
void A_LinedefExecute(mobj_t* actor)
{
	P_LinedefExecute((int)(1000 + (size_t)(actor->state - states)), actor, actor->subsector->sector);
}

/** Plays an actor's see sound.
  *
  * \param actor Actor.
  * \sa A_PlayAttackSound, A_PlayActiveSound
  * \author Graue <graue@oceanbase.org>
  */
void A_PlaySeeSound(mobj_t* actor)
{
	if(actor->info->seesound)
		S_StartScreamSound(actor, actor->info->seesound);
}

/** Plays an actor's attack sound.
  *
  * \param actor Actor.
  * \sa A_PlaySeeSound, A_PlayActiveSound
  * \author Graue <graue@oceanbase.org>
  */
void A_PlayAttackSound(mobj_t* actor)
{
	if(actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
}

/** Plays an actor's active sound.
  *
  * \param actor Actor.
  * \sa A_PlaySeeSound, A_PlayAttackSound
  * \author Graue <graue@oceanbase.org>
  */
void A_PlayActiveSound(mobj_t* actor)
{
	if(actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
}

//
// Action routine, for the ROCKET thing.
// This one adds trails of smoke to the rocket.
// The action pointer of the S_ROCKET state must point here to take effect.
// This routine is based on the Revenant Fireball Tracer code A_Tracer()
//
void A_SmokeTrailer(mobj_t* actor)
{
	mobj_t* th;

	if(gametic % (4*NEWTICRATERATIO))
		return;

	// add the smoke behind the rocket
	th = P_SpawnMobj(actor->x-actor->momx, actor->y-actor->momy, actor->z, MT_SMOK);

	th->momz = FRACUNIT;
	th->tics -= P_Random() & 3;
	if(th->tics < 1)
		th->tics = 1;
}
