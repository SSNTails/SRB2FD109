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
/// \brief Floor animation, elevators

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"
#include "g_game.h"
#include "r_main.h"

// ==========================================================================
//                              FLOORS
// ==========================================================================

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e T_MovePlane(sector_t* sector, fixed_t speed, fixed_t dest, boolean crush,
	int floorOrCeiling, int direction)
{
	boolean flag;
	fixed_t lastpos;
	fixed_t destheight; // used to keep floors/ceilings from moving through each other

	switch(floorOrCeiling)
	{
		case 0:
			// moving a floor
			switch(direction)
			{
				case -1:
					// Moving a floor down
					if(sector->floorheight - speed < dest)
					{
						lastpos = sector->floorheight;
						sector->floorheight = dest;
						flag = P_CheckSector(sector,crush);
						if(flag && sector->numattached)
						{
							sector->floorheight =lastpos;
							P_CheckSector(sector,crush);
						}
						return pastdest;
					}
					else
					{
						lastpos = sector->floorheight;
						sector->floorheight -= speed;
						flag = P_CheckSector(sector,crush);
						if(flag && sector->numattached)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;

				case 1:
					// Moving a floor up
					// keep floor from moving through ceilings
					destheight = (dest < sector->ceilingheight) ? dest : sector->ceilingheight;
					if(sector->floorheight + speed > destheight)
					{
						lastpos = sector->floorheight;
						sector->floorheight = destheight;
						flag = P_CheckSector(sector,crush);
						if(flag)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						// crushing is possible
						lastpos = sector->floorheight;
						sector->floorheight += speed;
						flag = P_CheckSector(sector, crush);
						if(flag)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;
			}
			break;

		case 1:
			// moving a ceiling
			switch(direction)
			{
				case -1:
					// moving a ceiling down
					// keep ceiling from moving through floors
					destheight = (dest > sector->floorheight) ? dest : sector->floorheight;
					if(sector->ceilingheight - speed < destheight)
					{
						lastpos = sector->ceilingheight;
						sector->ceilingheight = destheight;
						flag = P_CheckSector(sector,crush);

						if(flag)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						// crushing is possible
						lastpos = sector->ceilingheight;
						sector->ceilingheight -= speed;
						flag = P_CheckSector(sector, crush);

						if(flag)
						{
							if(crush) /// \note why is this here?
								return crushed;
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;

				case 1:
					// moving a ceiling up
					if(sector->ceilingheight + speed > dest)
					{
						lastpos = sector->ceilingheight;
						sector->ceilingheight = dest;
						flag = P_CheckSector(sector, crush);
						if(flag && sector->numattached)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						lastpos = sector->ceilingheight;
						sector->ceilingheight += speed;
						flag = P_CheckSector(sector, crush);
						if(flag && sector->numattached)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;
			}
			break;
	}
	return ok;
}

//
// MOVE A FLOOR TO ITS DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* floor)
{
	result_e res = 0;
	boolean dontupdate = false;

	res = T_MovePlane(floor->sector,
	                  floor->speed,
	                  floor->floordestheight,
	                  floor->crush, 0, floor->direction);

	if(floor->type == bounceFloor)
	{
		if(abs(floor->sector->floorheight - lines[floor->texture].frontsector->floorheight) < abs(floor->sector->floorheight - lines[floor->texture].backsector->floorheight))
			floor->speed = abs(floor->sector->floorheight - lines[floor->texture].frontsector->floorheight)/25 + FRACUNIT/4;
		else
			floor->speed = abs(floor->sector->floorheight - lines[floor->texture].backsector->floorheight)/25 + FRACUNIT/4;

		floor->speed *= floor->origspeed/(ELEVATORSPEED/2);
	}

	if(res == pastdest)
	{
		if(floor->direction == 1)
		{
			switch(floor->type)
			{
				case moveFloorByFrontSector:
					if(floor->texture < -1) // chained linedef executing
						P_LinedefExecute(floor->texture + 32769, NULL, NULL);
				case instantMoveFloorByFrontSector:
					if(floor->texture > -1) // flat changing
						floor->sector->floorpic = floor->texture;
			        break;
				case bounceFloor: // Graue 03-12-2004
					if(floor->floordestheight == lines[floor->texture].frontsector->floorheight)
						floor->floordestheight = lines[floor->texture].backsector->floorheight;
					else
						floor->floordestheight = lines[floor->texture].frontsector->floorheight;
					floor->direction = (floor->floordestheight < floor->sector->floorheight) ? -1 : 1;
					floor->sector->floorspeed = floor->speed * floor->direction;
		//			P_RecalcPrecipInSector(floor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case bounceFloorCrush: // Graue 03-27-2004
					if(floor->floordestheight == lines[floor->texture].frontsector->floorheight)
					{
						floor->floordestheight = lines[floor->texture].backsector->floorheight;
						floor->speed = floor->origspeed = abs(lines[floor->texture].dy)/4; // return trip, use dy
					}
					else
					{
						floor->floordestheight = lines[floor->texture].frontsector->floorheight;
						floor->speed = floor->origspeed = abs(lines[floor->texture].dx)/4; // forward again, use dx
					}
					floor->direction = (floor->floordestheight < floor->sector->floorheight) ? -1 : 1;
					floor->sector->floorspeed = floor->speed * floor->direction;
		//			P_RecalcPrecipInSector(floor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				default:
					break;
			}
		}
		else if(floor->direction == -1)
		{
			switch(floor->type)
			{
				case moveFloorByFrontSector:
					if(floor->texture < -1) // chained linedef executing
						P_LinedefExecute(floor->texture + 32769, NULL, NULL);
				case instantMoveFloorByFrontSector:
					if(floor->texture > -1) // flat changing
						floor->sector->floorpic = floor->texture;
					break;
				case bounceFloor: // Graue 03-12-2004
					if(floor->floordestheight == lines[floor->texture].frontsector->floorheight)
						floor->floordestheight = lines[floor->texture].backsector->floorheight;
					else
						floor->floordestheight = lines[floor->texture].frontsector->floorheight;
					floor->direction = (floor->floordestheight < floor->sector->floorheight) ? -1 : 1;
					floor->sector->floorspeed = floor->speed * floor->direction;
		//			P_RecalcPrecipInSector(floor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case bounceFloorCrush: // Graue 03-27-2004
					if(floor->floordestheight == lines[floor->texture].frontsector->floorheight)
					{
						floor->floordestheight = lines[floor->texture].backsector->floorheight;
						floor->speed = floor->origspeed = abs(lines[floor->texture].dy)/4; // return trip, use dy
					}
					else
					{
						floor->floordestheight = lines[floor->texture].frontsector->floorheight;
						floor->speed = floor->origspeed = abs(lines[floor->texture].dx)/4; // forward again, use dx
					}
					floor->direction = (floor->floordestheight < floor->sector->floorheight) ? -1 : 1;
					floor->sector->floorspeed = floor->speed * floor->direction;
		//			P_RecalcPrecipInSector(floor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				default:
					break;
			}
		}

		floor->sector->floordata = NULL; // Clear up the thinker so others can use it
		P_RemoveThinker(&floor->thinker);
		floor->sector->floorspeed = 0;
		dontupdate = true;
	}
	if(!dontupdate)
		floor->sector->floorspeed = floor->speed*floor->direction;
	else
		floor->sector->floorspeed = 0;

//	P_RecalcPrecipInSector(floor->sector);
}

//
// T_MoveElevator
//
// Move an elevator to it's destination (up or down)
// Called once per tick for each moving floor.
//
// Passed an elevator_t structure that contains all pertinent info about the
// move. See p_spec.h for fields.
// No return.
//
// The function moves the planes differently based on direction, so if it's
// traveling really fast, the floor and ceiling won't hit each other and
// stop the lift.
void T_MoveElevator(elevator_t* elevator)
{
	result_e res = 0;
	boolean dontupdate = false;

	if(elevator->direction < 0) // moving down
	{
		if(elevator->type == elevateContinuous)
		{
			// Slow down when reaching destination Tails 12-06-2000
			if(abs(elevator->sector->floorheight - elevator->floorwasheight) < abs(elevator->sector->floorheight - elevator->floordestheight))
				elevator->speed = abs(elevator->sector->floorheight - elevator->floorwasheight)/25 + FRACUNIT/4;
			else
				elevator->speed = abs(elevator->sector->floorheight - elevator->floordestheight)/25 + FRACUNIT/4;

			if(elevator->origspeed)
				elevator->speed *= elevator->origspeed/(ELEVATORSPEED/2);

			if(elevator->origspeed)
			{
				if(elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if(elevator->speed < 1)
					elevator->speed = 1/NEWTICRATERATIO;
			}
			else
			{
				if(elevator->speed > ((3*FRACUNIT)/NEWTICRATERATIO))
					elevator->speed = ((3*FRACUNIT)/NEWTICRATERATIO);
				if(elevator->speed < 1)
					elevator->speed = 1/NEWTICRATERATIO;
			}
		}

		res = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
			elevator->sector,
			elevator->speed,
			elevator->ceilingdestheight,
			0,
			1,                          // move floor
			elevator->direction
		);

		if(res==ok || res==pastdest) // jff 4/7/98 don't move ceil if blocked
			T_MovePlane
			(
				elevator->sector,
				elevator->speed,
				elevator->floordestheight,
				0,
				0,                        // move ceiling
				elevator->direction
			);
	}
	else // moving up
	{
		if(elevator->type == elevateContinuous)
		{
			// Slow down when reaching destination Tails 12-06-2000
			if(abs(elevator->sector->ceilingheight - elevator->ceilingwasheight) < abs(elevator->sector->ceilingheight - elevator->ceilingdestheight))
				elevator->speed = abs(elevator->sector->ceilingheight - elevator->ceilingwasheight)/25 + FRACUNIT/4;
			else
				elevator->speed = abs(elevator->sector->ceilingheight - elevator->ceilingdestheight)/25 + FRACUNIT/4;

			if(elevator->origspeed)
				elevator->speed *= elevator->origspeed/(ELEVATORSPEED/2);

			if(elevator->origspeed)
			{
				if(elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if(elevator->speed < 1)
					elevator->speed = 1/NEWTICRATERATIO;
			}
			else
			{
				if(elevator->speed > ((3*FRACUNIT)/NEWTICRATERATIO))
					elevator->speed = ((3*FRACUNIT)/NEWTICRATERATIO);
				if(elevator->speed < 1)
					elevator->speed = 1/NEWTICRATERATIO;
			}
		}

		res = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			0,
			0,                          // move ceiling
			elevator->direction
		);
		if(res==ok || res==pastdest) // jff 4/7/98 don't move floor if blocked
			T_MovePlane
			(
				elevator->sector,
				elevator->speed,
				elevator->ceilingdestheight,
				0,
				1,                        // move floor
				elevator->direction
			);
	}
/*
  // make floor move sound
  if(!(leveltime&7))
    S_StartSound((mobj_t *)&elevator->sector->soundorg, sfx_stnmov);
*/
	if(res == pastdest)            // if destination height acheived
	{
		elevator->sector->floordata = NULL;     //jff 2/22/98
		elevator->sector->ceilingdata = NULL;   //jff 2/22/98
		if(elevator->type == elevateContinuous)
		{
			if(elevator->direction > 0)
			{
				elevator->high = 1;
				elevator->low = 0;
				elevator->direction = -1;

				if(elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = ((3*FRACUNIT)/NEWTICRATERATIO);

				if(elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				elevator->floorwasheight = elevator->sector->floorheight;
				elevator->ceilingwasheight = elevator->sector->ceilingheight;
				T_MoveElevator(elevator);
			}
			else
			{
				elevator->high = 0;
				elevator->low = 1;
				elevator->direction = 1;

				if(elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = ((3*FRACUNIT)/NEWTICRATERATIO);

				if(elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				elevator->floorwasheight = elevator->sector->floorheight;
				elevator->ceilingwasheight = elevator->sector->ceilingheight;
				T_MoveElevator(elevator);
			}
		}
		else
		{
			P_RemoveThinker(&elevator->thinker);    // remove elevator from actives
			dontupdate = true;
		}
		// make floor stop sound
		// S_StartSound((mobj_t *)&elevator->sector->soundorg, sfx_pstop);
	}
	if(!dontupdate)
	{
		elevator->sector->floorspeed = elevator->speed*elevator->direction;
		elevator->sector->ceilspeed = 42;
	}
	else
	{
		elevator->sector->floorspeed = 0;
		elevator->sector->ceilspeed = 0;
		elevator->sector->floordata = NULL;
		elevator->sector->ceilingdata = NULL;
	}

	P_RecalcPrecipInSector(elevator->actionsector);
}

//
// T_ContinuousFalling
//
// A sector that continuously falls until its ceiling
// is below that of its actionsector's floor, then
// it instantly returns to its original position and
// falls again.
//
// Useful for things like intermittent falling lava.
//
void T_ContinuousFalling(elevator_t* elevator)
{
	T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
	(
		elevator->sector,
		elevator->speed,
		elevator->ceilingdestheight,
		0,
		1,                          // move floor
		elevator->direction
	);

	T_MovePlane
	(
		elevator->sector,
		elevator->speed,
		elevator->floordestheight,
		0,
		0,                        // move ceiling
		elevator->direction
	);

	if(elevator->sector->ceilingheight <= elevator->floordestheight)            // if destination height acheived
	{
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->floorheight = elevator->floorwasheight;
	}
}

//
// P_SectorCheckWater
//
// Like P_MobjCheckWater, but takes a sector instead of a mobj.
static inline fixed_t P_SectorCheckWater (sector_t* analyzesector, sector_t* elevatorsec)
{
	fixed_t   watertop;

	// Default if no water exists.
	watertop = -1;

    //
    // see if we are in water, and set some flags for later
    //
    if(analyzesector->ffloors)
    {
      ffloor_t*  rover;

      for(rover = analyzesector->ffloors; rover; rover = rover->next)
      {
        if(!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
          continue;

		// Set the watertop and waterbottom Tails 02-03-2002
		watertop = *rover->topheight;
      }
    }

	if(watertop < analyzesector->floorheight + abs((elevatorsec->ceilingheight - elevatorsec->floorheight)>>1))
		watertop = -42;

	return watertop;
}

//////////////////////////////////////////////////
// T_BounceCheese ////////////////////////////////
//////////////////////////////////////////////////
// Bounces a floating cheese

void T_BounceCheese(elevator_t* elevator)
{
	fixed_t halfheight;
	fixed_t waterheight;
	boolean nobounce = false;

	if(elevator->sector->crumblestate == 2) // Oops! Crumbler says to remove yourself!
	{
		elevator->sector->crumblestate = 1;
		P_RemoveThinker(&elevator->thinker); // remove elevator from actives
		return;
	}

	halfheight = abs(elevator->sector->ceilingheight - elevator->sector->floorheight) >> 1;

	waterheight = P_SectorCheckWater(elevator->actionsector, elevator->sector);

	if(waterheight == -42)
	{
		nobounce = true;
		elevator->ceilingwasheight = elevator->actionsector->floorheight +
			abs(elevator->sector->ceilingheight - elevator->sector->floorheight);
		elevator->floorwasheight = elevator->actionsector->floorheight;
	}
	else if(waterheight > elevator->sector->ceilingheight - halfheight && elevator->sector->ceilingheight >= elevator->actionsector->ceilingheight) // Tails 01-08-2004
	{
		elevator->sector->ceilingheight = elevator->actionsector->ceilingheight;
		elevator->sector->floorheight = elevator->sector->ceilingheight - (halfheight*2);
		P_RecalcPrecipInSector(elevator->actionsector);
		return;
	}
	else if(waterheight < elevator->sector->floorheight + halfheight && elevator->sector->floorheight <= elevator->actionsector->floorheight)
	{
		elevator->sector->ceilingheight = elevator->actionsector->floorheight + (halfheight*2);
		elevator->sector->floorheight = elevator->actionsector->floorheight;
		P_RecalcPrecipInSector(elevator->actionsector);
		return;
	}
	else
	{
		elevator->ceilingwasheight = waterheight + halfheight;
		elevator->floorwasheight = waterheight - halfheight;
	}

	T_MovePlane(elevator->sector, elevator->speed/2, elevator->sector->ceilingheight -
		70*FRACUNIT, 0, 1, -1); // move floor
	T_MovePlane(elevator->sector, elevator->speed/2, elevator->sector->floorheight - 70*FRACUNIT,
		0, 0, -1); // move ceiling

	if(elevator->sector->ceilingheight < elevator->ceilingwasheight && elevator->low == 0) // Down
	{
		if(nobounce)
		{
			elevator->speed = 0;
			goto done;
		}

		if(abs(elevator->speed) < 6*FRACUNIT)
			elevator->speed -= elevator->speed/3;
		else
			elevator->speed -= elevator->speed/2;

		elevator->low = 1;
		if(abs(elevator->speed) > 6*FRACUNIT)
		{
			elevator->actionsector->soundorg.z = elevator->sector->floorheight;
			S_StartSound((mobj_t *)&elevator->actionsector->soundorg, sfx_splash);
		}
	}
	else if(elevator->sector->ceilingheight > elevator->ceilingwasheight && elevator->low) // Up
	{
		if(abs(elevator->speed) < 6*FRACUNIT)
			elevator->speed -= elevator->speed/3;
		else
			elevator->speed -= elevator->speed/2;

		elevator->low = 0;
		if(abs(elevator->speed) > 6*FRACUNIT)
		{
			elevator->actionsector->soundorg.z = elevator->sector->floorheight;
			S_StartSound((mobj_t *)&elevator->actionsector->soundorg, sfx_splash);
		}
	}

	if(elevator->sector->ceilingheight < elevator->ceilingwasheight) // Down
	{
		elevator->speed -= elevator->distance;
	}
	else if(elevator->sector->ceilingheight > elevator->ceilingwasheight) // Up
	{
		elevator->speed += gravity;
	}

	if(elevator->speed < 2*FRACUNIT && elevator->speed > -2*FRACUNIT
		&& elevator->sector->ceilingheight < elevator->ceilingwasheight + FRACUNIT/4
		&& elevator->sector->ceilingheight > elevator->ceilingwasheight - FRACUNIT/4)
	{
done:
		elevator->sector->floorheight = elevator->floorwasheight;
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->crumblestate = 0;
		P_RemoveThinker(&elevator->thinker);    // remove elevator from actives
	}

	if(elevator->distance > 0)
		elevator->distance--;

	P_RecalcPrecipInSector(elevator->actionsector);
}

//////////////////////////////////////////////////
// T_StartCrumble ////////////////////////////////
//////////////////////////////////////////////////
// Crumbling platform Tails 03-11-2002

void T_StartCrumble(elevator_t* elevator)
{
	if(elevator->distance != 0)
	{
		if(elevator->distance > 0)
		{
			elevator->distance--;
			if(elevator->distance == 0)
				elevator->distance = -15*TICRATE;
			else
			{
				P_RecalcPrecipInSector(elevator->actionsector);
				return;
			}
		}
		else if(++elevator->distance == 0)
		{
			ffloor_t* rover;

			for(rover = elevator->actionsector->ffloors; rover; rover = rover->next)
			{
				if(rover->flags & FF_CRUMBLE && rover->flags & FF_FLOATBOB && *rover->topheight == elevator->sector->ceilingheight
					&& *rover->bottomheight == elevator->sector->floorheight)
				{
					rover->alpha = elevator->origspeed;

					if(rover->alpha == 0xff)
						rover->flags &= ~FF_TRANSLUCENT;
				}
			}

			elevator->direction = 1; // Up!
			P_RecalcPrecipInSector(elevator->actionsector);
			return;
		}

		if(elevator->distance > -224 && (leveltime % (abs(elevator->distance)/8 + 1) == 0))
		{
			ffloor_t* rover;

			for(rover = elevator->actionsector->ffloors; rover; rover = rover->next)
			{
				if(!(rover->flags & FF_NORETURN) && rover->flags & FF_CRUMBLE && rover->flags & FF_FLOATBOB && *rover->topheight == elevator->sector->ceilingheight
					&& *rover->bottomheight == elevator->sector->floorheight)
				{
					if(rover->alpha == elevator->origspeed)
					{
						rover->flags |= FF_TRANSLUCENT;
						rover->alpha = 0x00;
					}
					else
					{
						if(elevator->origspeed == 0xff)
							rover->flags &= ~FF_TRANSLUCENT;

						rover->alpha = elevator->origspeed;
					}
				}
			}
		}

		if(elevator->distance < 0 && elevator->distance > -3 && elevator->high == 42)
			elevator->sector->crumblestate = 2; // makes T_BounceCheese remove itself
	}

	if(elevator->direction == 1 && elevator->type == elevateContinuous) // No return crumbler
		return;

	if(elevator->direction == -1) // Down
	{
		if(elevator->high == 42 && elevator->low == 0)
		{
			elevator->sector->crumblestate = 0; // Allow floating now.
			elevator->low = 1;
		}
		else if(elevator->high != 42)
		{
			elevator->speed += gravity; // Gain more and more speed

			if(!(elevator->sector->ceilingheight < elevator->floordestheight))
			{
				T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
				(
				  elevator->sector,
				  elevator->speed,
				  elevator->sector->ceilingheight - 70*FRACUNIT,
				  0,
				  1,                          // move floor
				  elevator->direction
				);

				  T_MovePlane
				  (
					elevator->sector,
					elevator->speed,
					elevator->sector->floorheight - 70*FRACUNIT,
					0,
					0,                        // move ceiling
					elevator->direction
				  );
			}
		}
	}
	else // Up (restore to original position)
	{
		if(elevator->high == 42)
			elevator->sector->crumblestate = 1;
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->ceilingdata = NULL;
		elevator->sector->floorheight = elevator->floorwasheight;
		elevator->sector->floordata = NULL;
		P_RemoveThinker(&elevator->thinker);
	}
	P_RecalcPrecipInSector(elevator->actionsector);
}

//////////////////////////////////////////////////
// T_AirBob //////////////////////////////////////
//////////////////////////////////////////////////
// Like the still platforms in GHZ and EHZ Tails 03-11-2002

void T_AirBob(elevator_t* elevator)
{
	msecnode_t *node;
	boolean roverfound;

	roverfound = false;

	if(elevator->player && elevator->player->mo)
	{
		// Check if the player is still touching the sector.
		for (node = elevator->player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if(node->m_sector && node->m_sector->ffloors)
			{
				ffloor_t* rover;

				for(rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if(rover->flags & FF_AIRBOB)
					{
						if(rover->master->frontsector == elevator->sector) // Found it!
						{
							roverfound = true; // He's still touching it
						}
					}
				}
			}
		}
	}

	if(roverfound == false && elevator->sector->floorheight == elevator->floorwasheight)
	{
		elevator->sector->ceilingdata = NULL;
		P_RemoveThinker(&elevator->thinker);
	}

  if(elevator->player && elevator->player->mo && elevator->player->mo->z == elevator->sector->ceilingheight)
  {
	  elevator->low = 1;
	  elevator->direction = -1; // Down
  }
  else
  {
	  elevator->low = 0;
	  elevator->direction = 1; // Up
  }


  	if(elevator->sector->floorheight <= elevator->distance) // Reached the sag point
	{
		if(elevator->sector->floordata) // Falling at the moment
		{
			P_RecalcPrecipInSector(elevator->actionsector);
			return;
		}

		// Restore original position
		elevator->sector->ceilingheight = elevator->distance + elevator->sector->ceilingheight - elevator->sector->floorheight;
		elevator->sector->floorheight = elevator->distance;
		elevator->speed = 0;
	}
	if(elevator->sector->floorheight >= elevator->floorwasheight)
	{
		elevator->sector->floorheight = elevator->floorwasheight;
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->ceilingdata = NULL;
		P_RemoveThinker(&elevator->thinker);
	}

	elevator->speed = FRACUNIT/NEWTICRATERATIO; // I'm paranoid, okay?

  if(elevator->low)
  {
		T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
		  elevator->sector,
		  elevator->speed,
		  elevator->sector->ceilingheight - 70*FRACUNIT,
		  0,
		  1,                          // move floor
		  elevator->direction
		);

		  T_MovePlane
		  (
			elevator->sector,
			elevator->speed,
			elevator->sector->floorheight - 70*FRACUNIT,
			0,
			0,                        // move ceiling
			elevator->direction
		  );
  }
  else // push up!
  {
		T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
		  elevator->sector,
		  elevator->speed,
		  elevator->sector->ceilingheight + 70*FRACUNIT,
		  0,
		  1,                          // move floor
		  elevator->direction
		);

		  T_MovePlane
		  (
			elevator->sector,
			elevator->speed,
			elevator->sector->floorheight + 70*FRACUNIT,
			0,
			0,                        // move ceiling
			elevator->direction
		  );
  }
  P_RecalcPrecipInSector(elevator->actionsector);
}

// Graue 11-07-2003
// Someday I'll discover a better way to do this. In the meantime, enjoy an extra
// function almost exactly the same as T_AirBob. Hooray for bloat!
void T_AirBobReverse(elevator_t* elevator)
{
	msecnode_t *node;
	boolean roverfound;

	roverfound = false;

	if(elevator->player && elevator->player->mo)
	{
		// Check if the player is still touching the sector.
		for (node = elevator->player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if(node->m_sector && node->m_sector->ffloors)
			{
				ffloor_t* rover;

				for(rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if(rover->flags & FF_AIRBOB)
					{
						if(rover->master->frontsector == elevator->sector) // Found it!
						{
							roverfound = true; // He's still touching it
						}
					}
				}
			}
		}
	}

	if(roverfound == false && elevator->sector->floorheight == elevator->floorwasheight)
	{
		elevator->sector->ceilingdata = NULL;
		P_RemoveThinker(&elevator->thinker);
	}

	if(elevator->player && elevator->player->mo && elevator->player->mo->z == elevator->sector->ceilingheight)
	{
		elevator->low = 0;
		elevator->direction = 1; // Up
	}
	else
	{
		elevator->low = 1;
		elevator->direction = -1; // Down
	}


  	if(elevator->sector->floorheight >= elevator->distance) // Reached the sag point
	{
		if(elevator->sector->floordata) // Rising at the moment
		{
			P_RecalcPrecipInSector(elevator->actionsector);
			return;
		}

		// Restore original position
		elevator->sector->ceilingheight = elevator->distance + elevator->sector->ceilingheight - elevator->sector->floorheight;
		elevator->sector->floorheight = elevator->distance;
		elevator->speed = 0;
	}
	if(elevator->sector->floorheight <= elevator->floorwasheight)
	{
		elevator->sector->floorheight = elevator->floorwasheight;
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->ceilingdata = NULL;
		P_RemoveThinker(&elevator->thinker);
	}

	elevator->speed = FRACUNIT/NEWTICRATERATIO; // I'm paranoid, okay? - Yes, I too.

  if(elevator->low)
  {
		T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
		  elevator->sector,
		  elevator->speed,
		  elevator->sector->ceilingheight - 70*FRACUNIT,
		  0,
		  1,                          // move floor
		  elevator->direction
		);

		  T_MovePlane
		  (
			elevator->sector,
			elevator->speed,
			elevator->sector->floorheight - 70*FRACUNIT,
			0,
			0,                        // move ceiling
			elevator->direction
		  );
  }
  else // push up!
  {
		T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
		  elevator->sector,
		  elevator->speed,
		  elevator->sector->ceilingheight + 70*FRACUNIT,
		  0,
		  1,                          // move floor
		  elevator->direction
		);

		  T_MovePlane
		  (
			elevator->sector,
			elevator->speed,
			elevator->sector->floorheight + 70*FRACUNIT,
			0,
			0,                        // move ceiling
			elevator->direction
		  );
  }
  P_RecalcPrecipInSector(elevator->actionsector);
}

//////////////////////////////////////////////////
// T_MarioBlock //////////////////////////////////
//////////////////////////////////////////////////
// Mario hits a block! Tails 03-11-2002

void T_MarioBlock(elevator_t* elevator)
{
		T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
		  elevator->sector,
		  elevator->speed,
		  elevator->sector->ceilingheight + 70*FRACUNIT * elevator->direction,
		  0,
		  1,                          // move floor
		  elevator->direction
		);

		  T_MovePlane
		  (
			elevator->sector,
			elevator->speed,
			elevator->sector->floorheight + 70*FRACUNIT * elevator->direction,
			0,
			0,                        // move ceiling
			elevator->direction
		  );

		  if(elevator->sector->ceilingheight >= elevator->ceilingwasheight + 32*FRACUNIT) // Go back down now..
			  elevator->direction = -elevator->direction;
		  else if(elevator->sector->ceilingheight <= elevator->ceilingwasheight)
		  {
			  elevator->sector->ceilingheight = elevator->ceilingwasheight;
			  elevator->sector->floorheight = elevator->floorwasheight;
			  P_RemoveThinker(&elevator->thinker);
			  elevator->sector->floordata = NULL;     //jff 2/22/98
			  elevator->sector->ceilingdata = NULL;   //jff 2/22/98
		  }
		  P_RecalcPrecipInSector(elevator->actionsector);
}

// Tails 09-20-2002
void T_SpikeSector(elevator_t* elevator)
{
    mobj_t   *thing;
    msecnode_t* node;

    node = elevator->sector->touching_thinglist; // things touching this sector
    for ( ; node ; node = node->m_snext)
    {
		thing = node->m_thing;
		if(!thing->player)
			continue;

		if(!thing->momz > 0 && thing->z <= node->m_sector->floorheight)
		{
			mobj_t* killer;
			killer = P_SpawnMobj(thing->x, thing->y, thing->z, MT_DISS);
			killer->threshold = 43; // Special flag that it was spikes which hurt you.

			P_DamageMobj(thing, killer, killer, 1);
			break;
		}
	}
}

void T_FloatSector(elevator_t* elevator)
{
	fixed_t cheeseheight;
	boolean tofloat;
	boolean floatanyway; // Ignore the crumblestate setting.

	tofloat = false;
	floatanyway = false;

	cheeseheight = elevator->sector->floorheight + (elevator->sector->ceilingheight - elevator->sector->floorheight)/2;

	if(elevator->actionsector->ffloors)
	{
		ffloor_t*  rover;

		for(rover = elevator->actionsector->ffloors; rover; rover = rover->next)
		{
			if(!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
				continue;

			if(cheeseheight != *rover->topheight)
				tofloat = true;
		}
	}

	if(tofloat && (!elevator->sector->crumblestate || floatanyway))
		EV_BounceSector(elevator->sector, FRACUNIT, elevator->actionsector, false);

	P_RecalcPrecipInSector(elevator->actionsector);
}

void T_MarioBlockChecker(elevator_t* elevator)
{
	mobj_t* thing = NULL;
	msecnode_t* node = NULL;
	line_t* masterline;

	masterline = elevator->sourceline;

	node = elevator->sector->touching_thinglist; // things touching this sector

	if(node)
	{
		thing = node->m_thing;

		if(thing && (thing->health))
		{
			if((thing->flags & MF_MONITOR) && thing->threshold == 68)
				sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
			else
				sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].bottomtexture;
		}
		else
			sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
	}
	else
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
}

// This is the Thwomp's 'brain'. It looks around for players nearby, and if
// it finds any, **SMASH**!!! Muahahhaa....
void T_ThwompSector(elevator_t* elevator)
{
	fixed_t thwompx, thwompy;

	// If you just crashed down, wait a second before coming back up.
	if(--elevator->distance > 0)
	{
		sides[elevator->sourceline->sidenum[0]].midtexture = sides[elevator->sourceline->sidenum[0]].bottomtexture;
		return;
	}

	thwompx = elevator->actionsector->soundorg.x;
	thwompy = elevator->actionsector->soundorg.y;

	if(elevator->direction > 0) // Moving back up..
	{
		result_e res = 0;

		// Set the texture from the lower one (normal)
		sides[elevator->sourceline->sidenum[0]].midtexture = sides[elevator->sourceline->sidenum[0]].bottomtexture;
		/// \note this should only have to be done once, but is already done repeatedly, above

		elevator->speed = 2*FRACUNIT/NEWTICRATERATIO;

		res = T_MovePlane
		(
			elevator->sector,         // sector
			elevator->speed,          // speed
			elevator->floorwasheight, // dest
			0,                        // crush
			0,                        // floor or ceiling (0 for floor)
			elevator->direction       // direction
		);

		if(res == ok || res == pastdest)
			T_MovePlane
			(
				elevator->sector,           // sector
				elevator->speed,            // speed
				elevator->ceilingwasheight, // dest
				0,                          // crush
				1,                          // floor or ceiling (1 for ceiling)
				elevator->direction         // direction
			);

		if(res == pastdest)
			elevator->direction = 0; // stop moving
	}
	else if(elevator->direction < 0) // Crashing down!
	{
		result_e res = 0;

		// Set the texture from the upper one (angry)
		sides[elevator->sourceline->sidenum[0]].midtexture = sides[elevator->sourceline->sidenum[0]].toptexture;

		elevator->speed = 10*FRACUNIT/NEWTICRATERATIO;

		res = T_MovePlane
		(
			elevator->sector,   // sector
			elevator->speed,    // speed
			P_FloorzAtPos(thwompx, thwompy, elevator->sector->floorheight,
				elevator->sector->ceilingheight - elevator->sector->floorheight), // dest
			0,                  // crush
			0,                  // floor or ceiling (0 for floor)
			elevator->direction // direction
		);

		if(res == ok || res == pastdest)
			T_MovePlane
			(
				elevator->sector,   // sector
				elevator->speed,    // speed
				P_FloorzAtPos(thwompx, thwompy, elevator->sector->floorheight,
					elevator->sector->ceilingheight
					- (elevator->sector->floorheight + elevator->speed))
					+ (elevator->sector->ceilingheight
					- (elevator->sector->floorheight + elevator->speed/2)), // dest
				0,                  // crush
				1,                  // floor or ceiling (1 for ceiling)
				elevator->direction // direction
			);

		if(res == pastdest)
		{
			S_StartSound((mobj_t*)&elevator->actionsector->soundorg, sfx_thwomp);
			elevator->direction = 1; // start heading back up
			elevator->distance = TICRATE; // but only after a small delay
		}
	}
	else // Not going anywhere, so look for players.
	{
		thinker_t* th;
		mobj_t* mo;

		// scan the thinkers to find players!
		for(th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if(th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo = (mobj_t*)th;
			if(mo->type == MT_PLAYER && mo->health && mo->z <= elevator->sector->ceilingheight
				&& P_AproxDistance(thwompx - mo->x, thwompy - mo->y) <= 96*FRACUNIT)
			{
				elevator->direction = -1;
				break;
			}
		}
	}
	P_RecalcPrecipInSector(elevator->actionsector);
}

void T_CameraScanner(elevator_t* elevator)
{
	// leveltime is compared to make multiple scanners in one map function correctly.
	static tic_t lastleveltime = 32000; // any number other than 0 should do here
	static boolean camerascanned, camerascanned2;

	if(leveltime != lastleveltime) // Back on the first camera scanner
	{
		camerascanned = camerascanned2 = false;
		lastleveltime = leveltime;
	}

	if(players[displayplayer].mo)
	{
		if(players[displayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if(t_cam_dist == -42)
				t_cam_dist = cv_cam_dist.value;
			if(t_cam_height == -42)
				t_cam_height = cv_cam_height.value;
			if(t_cam_rotate == -42)
				t_cam_rotate = cv_cam_rotate.value;
			CV_SetValue(&cv_cam_height, elevator->sector->floorheight>>FRACBITS);
			CV_SetValue(&cv_cam_dist, elevator->sector->ceilingheight>>FRACBITS);
			CV_SetValue(&cv_cam_rotate, elevator->distance);
			camerascanned = true;
		}
		else if(!camerascanned)
		{
			if(t_cam_height != -42 && cv_cam_height.value != t_cam_height)
				CV_Set(&cv_cam_height, va("%f", (float)t_cam_height/FRACUNIT));
			if(t_cam_dist != -42 && cv_cam_dist.value != t_cam_dist)
				CV_Set(&cv_cam_dist, va("%f", (float)t_cam_dist/FRACUNIT));
			if(t_cam_rotate != -42 && cv_cam_rotate.value != t_cam_rotate)
				CV_Set(&cv_cam_rotate, va("%f", (float)t_cam_rotate));

			t_cam_dist = t_cam_height = t_cam_rotate = -42;
		}
	}

	if(cv_splitscreen.value && players[secondarydisplayplayer].mo)
	{
		if(players[secondarydisplayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if(t_cam2_rotate == -42)
				t_cam2_dist = cv_cam2_dist.value;
			if(t_cam2_rotate == -42)
				t_cam2_height = cv_cam2_height.value;
			if(t_cam2_rotate == -42)
				t_cam2_rotate = cv_cam2_rotate.value;
			CV_SetValue(&cv_cam2_height, elevator->sector->floorheight>>FRACBITS);
			CV_SetValue(&cv_cam2_dist, elevator->sector->ceilingheight>>FRACBITS);
			CV_SetValue(&cv_cam2_rotate, elevator->distance);
			camerascanned2 = true;
		}
		else if(!camerascanned2)
		{
			if(t_cam2_height != -42 && cv_cam2_height.value != t_cam2_height)
				CV_Set(&cv_cam2_height, va("%f", (float)t_cam2_height/FRACUNIT));
			if(t_cam2_dist != -42 && cv_cam2_dist.value != t_cam2_dist)
				CV_Set(&cv_cam2_dist, va("%f", (float)t_cam2_dist/FRACUNIT));
			if(t_cam2_rotate != -42 && cv_cam2_rotate.value != t_cam2_rotate)
				CV_Set(&cv_cam2_rotate, va("%f", (float)t_cam2_rotate));

			t_cam2_dist = t_cam2_height = t_cam2_rotate = -42;
		}
	}
}

//
// EV_DoFloor
//
// Set up and start a floor thinker.
//
// Called by P_ProcessLineSpecial (linedef executors), P_ProcessSpecialSector
// (egg capsule button), P_PlayerInSpecialSector (buttons),
// and P_SpawnSpecials (continuous floor movers and instant lower).
//
int EV_DoFloor(line_t* line, floor_e floortype)
{
	int secnum = -1, rtn = 0, firstone = 1;
	sector_t* sec;
	floormove_t* floor;

	while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		if(sec->floordata) // if there's already a thinker on this floor,
			continue; // then don't add another one

		// new floor thinker
		rtn = 1;
		floor = Z_Malloc(sizeof(*floor), PU_LEVSPEC, 0);
		P_AddThinker(&floor->thinker);

		// make sure another floor thinker won't get started over this one
		sec->floordata = floor;

		// set up some generic aspects of the floormove_t
		floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
		floor->type = floortype;
		floor->crush = false; // default: types that crush will change this
		floor->sector = sec;

		switch(floortype)
		{
			// Used for generic buttons 1-20 (sector types 690-709), button 21
			// (a special used in THZ2, sector type 710), and the slime lower
			// button at the end of THZ2 (sector type 986).
			case lowerFloorToLowest:
				floor->direction = -1; // down
				floor->speed = FLOORSPEED*2; // 2 fracunits per tic
				floor->floordestheight = P_FindLowestFloorSurrounding(sec);
				break;

			// Used for part of the Egg Capsule, when an FOF with type 666 is
			// contacted by the player.
			case raiseFloorToNearestFast:
				floor->direction = -1; // down
				floor->speed = FLOORSPEED*4; // 4 fracunits per tic
				floor->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				break;

			// Used for button 21 (sector type 710) in THZ2, as well as for
			// changing the sign via the THZ2 slime lower button (sector type
			// 986), and for sectors tagged to 26 linedefs (effectively
			// changing the base height for placing things in that sector).
			case instantLower:
				floor->direction = -1; // down
				floor->speed = MAXINT/2; // "instant" means "takes one tic"
				floor->floordestheight = P_FindLowestFloorSurrounding(sec);
				break;

			// Linedef executor command, linetype 101.
			// Front sector floor = destination height.
			case instantMoveFloorByFrontSector:
				floor->speed = MAXINT/2; // as above, "instant" is one tic
				floor->floordestheight = line->frontsector->floorheight;

				if(floor->floordestheight >= sec->floorheight)
					floor->direction = 1; // up
				else
					floor->direction = -1; // down

				// New for 1.09: now you can use the no climb flag to
				// DISABLE the flat changing. This makes it work
				// totally opposite the way linetype 106 does. Yet
				// another reason I'll be glad to break backwards
				// compatibility for the final.
				if(line->flags & ML_NOCLIMB)
					floor->texture = -1; // don't mess with the floorpic
				else
					floor->texture = line->frontsector->floorpic;

			// Linedef executor command, linetype 106.
			// Line length = speed, front sector floor = destination height.
			case moveFloorByFrontSector:
				floor->speed = P_AproxDistance(line->dx, line->dy)/8;
				floor->floordestheight = line->frontsector->floorheight;

				if(floor->floordestheight >= sec->floorheight)
					floor->direction = 1; // up
				else
					floor->direction = -1; // down

				// chained linedef executing ability
				if(line->flags & ML_BLOCKMONSTERS)
				{
					// Only set it on one of the moving sectors (the
					// smallest numbered) and only if the front side
					// x offset is positive, indicating a valid tag.
					if(firstone && sides[line->sidenum[0]].textureoffset > 0)
						floor->texture = (sides[line->sidenum[0]].textureoffset>>FRACBITS) - 32769;
					else
						floor->texture = -1;
				}

				// flat changing ability
				else if(line->flags & ML_NOCLIMB)
					floor->texture = line->frontsector->floorpic;
				else
					floor->texture = -1; // nothing special to do after movement completes

				break;

			// Linedef executor command, linetype 108.
			// dx = speed, dy = amount to lower.
			case lowerFloorByLine:
				floor->direction = -1; // down
				floor->speed = abs(line->dx)/8;
				floor->floordestheight = sec->floorheight - abs(line->dy);
				if(floor->floordestheight > sec->floorheight) // wrapped around
					I_Error("Can't lower sector %d\n", sec - sectors);
				break;

			// Linedef executor command, linetype 109.
			// dx = speed, dy = amount to raise.
			case raiseFloorByLine:
				floor->direction = 1; // up
				floor->speed = abs(line->dx)/8;
				floor->floordestheight = sec->floorheight + abs(line->dy);
				if(floor->floordestheight < sec->floorheight) // wrapped around
					I_Error("Can't raise sector %d\n", sec - sectors);
				break;

			// Linetypes 2/3.
			// Move floor up and down indefinitely like the old elevators.
			case bounceFloor:
				floor->speed = P_AproxDistance(line->dx, line->dy)/4; // same speed as elevateContinuous
				floor->speed /= NEWTICRATERATIO;
				floor->origspeed = floor->speed; // it gets slowed down at the top and bottom
				floor->floordestheight = line->frontsector->floorheight;

				if(floor->floordestheight >= sec->floorheight)
					floor->direction = 1; // up
				else
					floor->direction = -1; // down

				floor->texture = (fixed_t)(line - lines); // hack: store source line number
				break;

			// Linetypes 6/7.
			// Like 2/3, but no slowdown at the top and bottom of movement,
			// and the speed is line->dx the first way, line->dy for the
			// return trip. Good for crushers.
			case bounceFloorCrush:
				floor->speed = abs(line->dx)/4;
				floor->speed /= NEWTICRATERATIO;
				floor->origspeed = floor->speed;
				floor->floordestheight = line->frontsector->floorheight;

				if(floor->floordestheight >= sec->floorheight)
					floor->direction = 1; // up
				else
					floor->direction = -1; // down

				floor->texture = (fixed_t)(line - lines); // hack: store source line number
				break;

			default:
				break;
		}

		firstone = 0;
	}

	return rtn;
}

// SoM: Boom elevator support.
//
// EV_DoElevator
//
// Handle elevator linedef types
//
// Passed the linedef that triggered the elevator and the elevator action
//
// jff 2/22/98 new type to move floor and ceiling in parallel
//
int EV_DoElevator(line_t* line, elevator_e elevtype, boolean customspeed)
{
	int secnum = -1, rtn = 0;
	sector_t* sec;
	elevator_t* elevator;

	// act on all sectors with the same tag as the triggering linedef
	while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
	{
		sec = &sectors[secnum];

		// If either floor or ceiling is already activated, skip it
		if(sec->floordata || sec->ceilingdata)
			continue;

		// create and initialize new elevator thinker
		rtn = 1;
		elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
		P_AddThinker(&elevator->thinker);
		sec->floordata = elevator;
		sec->ceilingdata = elevator;
		elevator->thinker.function.acp1 = (actionf_p1)T_MoveElevator;
		elevator->type = elevtype;

		// set up the fields according to the type of elevator action
		switch(elevtype)
		{
			// elevator down to next floor
			case elevateDown:
				elevator->direction = -1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/2; // half speed
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator up to next floor
			case elevateUp:
				elevator->direction = 1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/4; // quarter speed
				elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator up to highest floor
			case elevateHighest:
				elevator->direction = 1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/4; // quarter speed
				elevator->floordestheight = P_FindHighestFloorSurrounding(sec);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator to floor height of activating switch's front sector
			case elevateCurrent:
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED;
				elevator->floordestheight = line->frontsector->floorheight;
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				elevator->direction = elevator->floordestheight > sec->floorheight?  1 : -1;
				break;

			case elevateContinuous:
				if(customspeed)
				{
					elevator->origspeed = P_AproxDistance(line->dx, line->dy)/4;
					elevator->origspeed /= NEWTICRATERATIO;
					elevator->speed = elevator->origspeed;
				}
				else
				{
					elevator->speed = ELEVATORSPEED/2;
					elevator->origspeed = elevator->speed;
				}

				elevator->sector = sec;
				elevator->low = !(line->flags & ML_NOCLIMB); // go down first unless noclimb is on
				if(elevator->low)
				{
					elevator->direction = 1;
					elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
					elevator->ceilingdestheight = elevator->floordestheight
						+ sec->ceilingheight - sec->floorheight;
				}
				else
				{
					elevator->direction = -1;
					elevator->floordestheight =	P_FindNextLowestFloor(sec,sec->floorheight);
					elevator->ceilingdestheight = elevator->floordestheight
						+ sec->ceilingheight - sec->floorheight;
				}
				elevator->floorwasheight = elevator->sector->floorheight;
				elevator->ceilingwasheight = elevator->sector->ceilingheight;
				break;

			case bridgeFall:
				elevator->direction = -1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED*4; // quadruple speed
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			default:
				break;
		}
	}
	return rtn;
}

void EV_CrumbleChain(sector_t* sec, ffloor_t* rover)
{
	int i;
	int leftmostvertex;
	int rightmostvertex;
	int topmostvertex;
	int bottommostvertex;
	fixed_t leftx, rightx;
	fixed_t topy, bottomy;
	fixed_t topz;
	fixed_t a, b, c;

	S_StartSound(&sec->soundorg, sfx_crumbl);

	// Find the leftmost vertex in the subsector.
	leftmostvertex = 0;
	for(i=0; i<sec->linecount; i++)
	{
		if((sec->lines[i]->v1->x < sec->lines[leftmostvertex]->v1->x))
		{
			leftmostvertex = i;
		}
	}
	// Find the rightmost vertex in the subsector.
	rightmostvertex = 0;
	for(i=0; i<sec->linecount; i++)
	{
		if((sec->lines[i]->v1->x > sec->lines[rightmostvertex]->v1->x))
		{
			rightmostvertex = i;
		}
	}
	// Find the topmost vertex in the subsector.
	topmostvertex = 0;
	for(i=0; i<sec->linecount; i++)
	{
		if((sec->lines[i]->v1->y > sec->lines[topmostvertex]->v1->y))
		{
			topmostvertex = i;
		}
	}
	// Find the bottommost vertex in the subsector.
	bottommostvertex = 0;
	for(i=0; i<sec->linecount; i++)
	{
		if((sec->lines[i]->v1->y < sec->lines[bottommostvertex]->v1->y))
		{
			bottommostvertex = i;
		}
	}

	leftx = sec->lines[leftmostvertex]->v1->x+(16<<FRACBITS);
	rightx = sec->lines[rightmostvertex]->v1->x;
	topy = sec->lines[topmostvertex]->v1->y-(16<<FRACBITS);
	bottomy = sec->lines[bottommostvertex]->v1->y;
	topz = *rover->topheight-(16<<FRACBITS);

	for(a = leftx; a < rightx; a += (32<<FRACBITS))
	{
		for(b = topy; b > bottomy; b -= (32<<FRACBITS))
		{
			if(R_PointInSubsector(a, b)->sector == sec)
			{
				for(c = topz; c > *rover->bottomheight; c -= (32<<FRACBITS))
				{
					// If the control sector has a special
					// of 1500-1515, use the custom debris.
					if(rover->master->frontsector->special >= 1500 && rover->master->frontsector->special <= 1515)
						P_SpawnMobj(a, b, c, MT_ROCKCRUMBLE1+(rover->master->frontsector->special-1500));
					else
						P_SpawnMobj(a, b, c, MT_ROCKCRUMBLE1);
				}
			}
		}
	}

	// Throw the floor into the ground
	rover->master->frontsector->floorheight = sec->floorheight - 128*FRACUNIT;
	rover->master->frontsector->ceilingheight = sec->floorheight - 64*FRACUNIT;

	rover->flags |= FF_SOLID;
	rover->flags &= ~FF_BUSTUP;
	rover->master->frontsector->crumblestate = 1;
}

// Used for bobbing platforms on the water
int EV_BounceSector(sector_t* sec, fixed_t momz, sector_t* blocksector, boolean player)
{
	elevator_t* elevator;

	// create and initialize new elevator thinker

	player = 0;
	if(sec->crumblestate == 1) // One at a time, ma'am.
		return 0;

	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);
	elevator->thinker.function.acp1 = (actionf_p1)T_BounceCheese;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = abs(momz)/2;
	elevator->actionsector = blocksector;
	elevator->distance = FRACUNIT;
	elevator->low = 1;

	elevator->sector->crumblestate = 1; // Currently bobbing.

	return 1;
}

// For T_ContinuousFalling special
int EV_DoContinuousFall(sector_t* sec, fixed_t floordestheight, fixed_t speed)
{
	elevator_t* elevator;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);
	elevator->thinker.function.acp1 = (actionf_p1)T_ContinuousFalling;
	elevator->type = elevateDown;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = speed;
	elevator->floorwasheight = elevator->sector->floorheight;
	elevator->ceilingwasheight = elevator->sector->ceilingheight;
	elevator->floordestheight = floordestheight;
	elevator->direction = -1;

	return 1;
}

// Some other 3dfloor special things Tails 03-11-2002 (Search p_mobj.c for description)
int EV_StartCrumble(sector_t* sec, sector_t* roversector, fixed_t roverheight, boolean floating,
	player_t* player, fixed_t origalpha)
{
	elevator_t* elevator;

	// If floor is already activated, skip it
	if(sec->floordata)
		return 0;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);
	sec->floordata = elevator;
	elevator->thinker.function.acp1 = (actionf_p1)T_StartCrumble;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = 0;
	elevator->direction = -1; // Down
	elevator->floorwasheight = elevator->sector->floorheight;
	elevator->ceilingwasheight = elevator->sector->ceilingheight;
	elevator->floordestheight = roversector->floorheight;
	elevator->actionsector = roversector;
	elevator->distance = TICRATE; // Used for delay time
	elevator->low = 0;
	elevator->player = player;
	elevator->origspeed = origalpha;

	sec->ceilingdata = sec->floordata = elevator;

	if(floating)
	{
		elevator->high = 42;
		elevator->sector->crumblestate = 1;
	}
	else
	{
		elevator->high = 0;
		elevator->sector->crumblestate = 0;
	}

	P_SpawnMobj(roversector->soundorg.x, roversector->soundorg.y, roverheight, MT_CRUMBLEOBJ);

	return 1;
}

int EV_StartNoReturnCrumble(sector_t*  sec, sector_t* roversector, fixed_t roverheight,
	boolean floating, player_t* player)
{
	elevator_t* elevator;

	// If floor is already activated, skip it
	if(sec->floordata)
		return 0;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);
	sec->floordata = elevator;
	elevator->thinker.function.acp1 = (actionf_p1)T_StartCrumble;
	elevator->type = elevateContinuous;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = 0;
	elevator->direction = -1; // Down
	elevator->floorwasheight = elevator->sector->floorheight;
	elevator->ceilingwasheight = elevator->sector->ceilingheight;
	elevator->floordestheight = roversector->floorheight;
	elevator->actionsector = roversector;
	elevator->distance = TICRATE; // Used for delay time
	elevator->low = 0;
	elevator->player = player;

	sec->ceilingdata = sec->floordata = elevator;

	if(floating)
	{
		elevator->high = 42;
		elevator->sector->crumblestate = 1;
	}
	else
	{
		elevator->high = 0;
		elevator->sector->crumblestate = 0;
	}

	P_SpawnMobj(roversector->soundorg.x, roversector->soundorg.y, roverheight, MT_CRUMBLEOBJ);

	return 1;
}

int EV_AirBob(sector_t* sec, player_t* player, int amount, boolean reverse)
{
	elevator_t* elevator;

	// If ceiling is already activated, skip it
	if(sec->ceilingdata)
		return 0;

	// create and initialize new elevator thinker
	elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
	P_AddThinker(&elevator->thinker);
	sec->ceilingdata = elevator;

	// Graue 11-07-2003
	if(reverse)
		elevator->thinker.function.acp1 = (actionf_p1)T_AirBobReverse;
	else
		elevator->thinker.function.acp1 = (actionf_p1)T_AirBob;

	elevator->type = elevateBounce;

	elevator->player = player; // Pass the reference!

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = FRACUNIT/NEWTICRATERATIO;
	elevator->direction = -1; // Down
	elevator->floorwasheight = P_FindHighestFloorSurrounding(elevator->sector);
	elevator->ceilingwasheight = P_FindHighestCeilingSurrounding(elevator->sector);
	// use amount instead of just 16 all the time Graue 11-04-2003
	elevator->distance = elevator->floorwasheight - amount*FRACUNIT
		* (reverse ? -1 : 1); // How far to sag down
	elevator->low = 1;
	elevator->high = 0;

	return 1;
}

int EV_MarioBlock(sector_t* sec, sector_t* roversector, fixed_t topheight,
	struct line_s* masterline, mobj_t* puncher)
{
	elevator_t* elevator;
	mobj_t *thing;
	fixed_t oldx = 0, oldy = 0, oldz = 0;

	masterline = NULL;
	if(sec->floordata || sec->ceilingdata)
		return 0;

	if(sec->touching_thinglist && sec->touching_thinglist->m_thing)
	{
		thing = sec->touching_thinglist->m_thing;
		// things are touching this sector
		if((thing->flags & MF_MONITOR) && thing->threshold == 68)
		{
			// "Thunk!" sound
			S_StartSound(puncher, sfx_tink); // Puncher is "close enough".
			return 1;
		}
		// create and initialize new elevator thinker

		elevator = Z_Malloc(sizeof(*elevator), PU_LEVSPEC, 0);
		P_AddThinker(&elevator->thinker);
		sec->floordata = elevator;
		sec->ceilingdata = elevator;
		elevator->thinker.function.acp1 = (actionf_p1)T_MarioBlock;
		elevator->type = elevateBounce;

		// set up the fields according to the type of elevator action
		elevator->sector = sec;
		elevator->speed = (4*FRACUNIT)/NEWTICRATERATIO;
		elevator->direction = 1; // Up
		elevator->floorwasheight = elevator->sector->floorheight;
		elevator->ceilingwasheight = elevator->sector->ceilingheight;
		elevator->distance = FRACUNIT;
		elevator->low = 1;

		if((thing->flags & MF_MONITOR))
		{
			oldx = thing->x;
			oldy = thing->y;
			oldz = thing->z;
		}

		P_UnsetThingPosition(thing);
		thing->x = roversector->soundorg.x;
		thing->y = roversector->soundorg.y;
		thing->z = topheight;
		thing->momz = 6*FRACUNIT;
		P_SetThingPosition(thing);
		if(thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, puncher, puncher, 1);
		else if(thing->type == MT_RING || thing->type == MT_COIN)
		{
			thing->momz = 3*FRACUNIT;
			P_TouchSpecialThing(thing, puncher, false);
			// "Thunk!" sound
			S_StartSound(puncher, sfx_tink); // Puncher is "close enough"
		}
		else
		{
			// "Powerup rise" sound
			S_StartSound(puncher, sfx_cgot); // Puncher is "close enough"
		}

		if((thing->flags & MF_MONITOR))
		{
			P_UnsetThingPosition(thing->tracer);
			thing->tracer->x = oldx;
			thing->tracer->y = oldy;
			thing->tracer->z = oldz;
			thing->tracer->momx = 1;
			thing->tracer->momy = 1;
			P_SetThingPosition(thing->tracer);
		}
	}
	else
		S_StartSound(puncher, sfx_tink); // "Thunk!" sound - puncher is "close enough".

	return 1;
}
