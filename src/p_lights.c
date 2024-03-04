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
/// \brief Sector lighting effects
/// 
///	Fire flicker, light flash, strobe flash, lightning flash, glow, and fade.

#include "doomdef.h"
#include "p_local.h"
#include "r_state.h"
#include "z_zone.h"
#include "m_random.h"

/** Removes any active lighting effects in a sector.
  *
  * \param sector The sector to remove effects from.
  */
static void P_RemoveLighting(sector_t* sector)
{
	if(sector->lightingdata)
	{
		// The thinker is the first member in all the lighting action structs,
		// so just let the thinker get freed, and that will free the whole
		// structure.
		P_RemoveThinker(&((elevator_t*)sector->lightingdata)->thinker);
		sector->lightingdata = NULL;
	}
}

// =========================================================================
//                           FIRELIGHT FLICKER
// =========================================================================

/** Thinker function for fire flicker.
  *
  * \param flick Action structure for this effect.
  * \sa P_SpawnFireFlicker, P_SpawnAdjustableFireFlicker
  */
void T_FireFlicker(fireflicker_t* flick)
{
	short amount;

	if(--flick->count)
		return;

	amount = (short)((byte)(P_Random() & 3) * 16);

	if(flick->sector->lightlevel - amount < flick->minlight)
		flick->sector->lightlevel = (short)flick->minlight;
	else
		flick->sector->lightlevel = (short)((short)flick->maxlight - amount);

	flick->count = flick->resetcount;
}

/** Spawns a fire flicker effect in a sector.
  *
  * \param sector Spawn the effect here.
  * \sa T_FireFlicker, P_SpawnAdjustableFireFlicker
  */
void P_SpawnFireFlicker(sector_t* sector)
{
	fireflicker_t* flick;

	// Note that we are resetting sector attributes.
	// Nothing special about it during gameplay.
	sector->special &= ~31; // Clear non-generalized sector type

	P_RemoveLighting(sector); // out with the old, in with the new
	flick = Z_Malloc(sizeof(*flick), PU_LEVSPEC, 0);

	P_AddThinker(&flick->thinker);

	flick->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
	flick->sector = sector;
	flick->maxlight = sector->lightlevel;
	flick->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel) + 16;
	flick->count = flick->resetcount = 4;
	sector->lightingdata = flick;
}

/** Spawns an adjustable fire flicker effect in a sector.
  *
  * \param minsector Sector whose light level is used as the darkest.
  * \param maxsector Sector whose light level is used as the brightest,
  *                  and also the target sector for the effect.
  * \param length    Four times the number of tics between flickers.
  * \sa T_FireFlicker, P_SpawnFireFlicker
  */
fireflicker_t* P_SpawnAdjustableFireFlicker(sector_t* minsector, sector_t* maxsector, int length)
{
	fireflicker_t* flick;

	P_RemoveLighting(maxsector); // out with the old, in with the new
	flick = Z_Malloc(sizeof(*flick), PU_LEVSPEC, 0);

	P_AddThinker(&flick->thinker);

	flick->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
	flick->sector = maxsector;
	flick->maxlight = maxsector->lightlevel;
	flick->minlight = minsector->lightlevel;
	if(flick->minlight > flick->maxlight)
	{
		// You mixed them up, you dummy.
		int oops = flick->minlight;
		flick->minlight = flick->maxlight;
		flick->maxlight = oops;
	}
	flick->count = flick->resetcount = length/4;
	maxsector->lightingdata = flick;

	// input bounds checking and stuff
	if(!flick->resetcount)
		flick->resetcount = 1;
	if(flick->minlight == flick->maxlight)
	{
		if(flick->minlight > 0)
			flick->minlight--;
		if(flick->maxlight < 255)
			flick->maxlight++;
	}

	return flick;
}

//
// LIGHTNING FLASH EFFECT
//

/** Thinker function for a lightning flash storm effect.
  *
  * \param flash The effect being considered.
  * \sa P_SpawnLightningFlash
  */
void T_LightningFlash(lightflash_t* flash)
{
	flash->sector->lightlevel -= 4/NEWTICRATERATIO;

	if(flash->sector->lightlevel <= flash->minlight)
	{
		flash->sector->lightlevel = (short)flash->minlight;
		P_RemoveLighting(flash->sector);
	}
}

/** Spawns a one-time lightning flash.
  *
  * \param sector Sector to light up.
  * \sa T_LightningFlash
  */
void P_SpawnLightningFlash(sector_t* sector)
{
	int minlight;
	lightflash_t* flash;

	minlight = sector->lightlevel;

	if(sector->lightingdata)
	{
		if(((lightflash_t*)sector->lightingdata)->thinker.function.acp1
			== (actionf_p1)T_LightningFlash)
		{
			// lightning was already flashing in this sector
			// save the original light level value
			minlight = ((lightflash_t*)sector->lightingdata)->minlight;
		}

		P_RemoveThinker(&((elevator_t*)sector->lightingdata)->thinker);
	}

	sector->lightingdata = NULL;

	flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);

	P_AddThinker(&flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LightningFlash;
	flash->sector = sector;
	flash->maxlight = 255;
	flash->minlight = minlight;
	sector->lightlevel = (short)flash->maxlight;

	sector->lightingdata = flash;
}

//
// STROBE LIGHT FLASHING
//

/** Thinker function for strobe light flashing.
  *
  * \param flash The effect under consideration.
  * \sa P_SpawnStrobeFlash
  */
void T_StrobeFlash(strobe_t* flash)
{
	if(--flash->count)
		return;

	if(flash->sector->lightlevel == flash->minlight)
	{
		flash->sector->lightlevel = (short)flash->maxlight;
		flash->count = flash->brighttime;
	}
	else
	{
		flash->sector->lightlevel = (short)flash->minlight;
		flash->count = flash->darktime;
	}
}

/** Spawns a strobe light flash effect.
  *
  * \param sector     Sector where the effect will take place.
  * \param fastOrSlow Time in tics for the light to be dark.
  * \param inSync     If true, the effect will be kept in sync
  *                   with other effects of this type, provided
  *                   they use the same fastOrSlow value, and
  *                   provided this function is called at level
  *                   load. Otherwise, the starting state of
  *                   the strobe flash is random.
  * \sa T_StrobeFlash
  */
void P_SpawnStrobeFlash(sector_t* sector, int fastOrSlow, int inSync)
{
	strobe_t* flash;

	P_RemoveLighting(sector); // out with the old, in with the new
	flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);

	P_AddThinker(&flash->thinker);

	flash->sector = sector;
	flash->darktime = fastOrSlow;
	flash->brighttime = STROBEBRIGHT;
	flash->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	flash->maxlight = sector->lightlevel;
	flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

	if(flash->minlight == flash->maxlight)
		flash->minlight = 0;

	// nothing special about it during gameplay
	sector->special &= ~31; // Clear non-generalized sector type

	if(!inSync)
		flash->count = (P_Random() & 7) + 1;
	else
		flash->count = 1;

	sector->lightingdata = flash;
}

/** Thinker function for glowing light.
  *
  * \param g Action structure for this effect.
  * \sa P_SpawnGlowingLight, P_SpawnAdjustableGlowingLight
  */
void T_Glow(glow_t* g)
{
	switch(g->direction)
	{
		case -1:
			// DOWN
			g->sector->lightlevel = (short)(g->sector->lightlevel - (short)g->speed);
			if(g->sector->lightlevel <= g->minlight)
			{
				g->sector->lightlevel = (short)(g->sector->lightlevel + (short)g->speed);
				g->direction = 1;
			}
			break;

		case 1:
			// UP
			g->sector->lightlevel = (short)(g->sector->lightlevel + (short)g->speed);
			if(g->sector->lightlevel >= g->maxlight)
			{
				g->sector->lightlevel = (short)(g->sector->lightlevel - (short)g->speed);
				g->direction = -1;
			}
			break;
	}
}

/** Spawns a glowing light effect in a sector.
  *
  * \param sector Spawn the effect here.
  * \sa T_Glow, P_SpawnAdjustableGlowingLight
  */
void P_SpawnGlowingLight(sector_t* sector)
{
	glow_t* g;

	P_RemoveLighting(sector); // out with the old, in with the new
	g = Z_Malloc(sizeof(*g), PU_LEVSPEC, 0);

	P_AddThinker(&g->thinker);

	g->sector = sector;
	g->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
	g->maxlight = sector->lightlevel;
	g->thinker.function.acp1 = (actionf_p1)T_Glow;
	g->direction = -1;
	g->speed = GLOWSPEED;

	sector->lightingdata = g;
	sector->special &= ~31; // Reset only non-generic types.
}

/** Spawns an adjustable glowing light effect in a sector.
  *
  * \param minsector Sector whose light level is used as the darkest.
  * \param maxsector Sector whose light level is used as the brightest,
  *                  and also the target sector for the effect.
  * \param length    The speed of the effect.
  * \sa T_Glow, P_SpawnGlowingLight
  */
glow_t* P_SpawnAdjustableGlowingLight(sector_t* minsector, sector_t* maxsector, int length)
{
	glow_t* g;

	P_RemoveLighting(maxsector); // out with the old, in with the new
	g = Z_Malloc(sizeof(*g), PU_LEVSPEC, 0);

	P_AddThinker(&g->thinker);

	g->sector = maxsector;
	g->minlight = minsector->lightlevel;
	g->maxlight = maxsector->lightlevel;
	if(g->minlight > g->maxlight)
	{
		// You mixed them up, you dummy.
		int oops = g->minlight;
		g->minlight = g->maxlight;
		g->maxlight = oops;
	}
	g->thinker.function.acp1 = (actionf_p1)T_Glow;
	g->direction = 1;
	g->speed = length/4;
	if(g->speed > (g->maxlight - g->minlight)/2) // don't make it ridiculous speed
		g->speed = (g->maxlight - g->minlight)/2;

	while(g->speed < 1)
	{
		if(g->minlight > 0)
			g->minlight--;
		if(g->maxlight < 255)
			g->maxlight++;

		g->speed = (g->maxlight - g->minlight)/2;
	}

	maxsector->lightingdata = g;

	return g;
}

/** Fades all the lights in sectors with a particular tag to a new
  * value.
  *
  * \param tag       Tag to look for sectors by.
  * \param destvalue The final light value in these sectors.
  * \param speed     Speed of the fade; the change to the ligh
  *                  level in each sector per tic.
  * \todo Calculate speed better so that it is possible to specify
  *       the time for completion of the fade, and all lights fade
  *       in this time regardless of initial values.
  * \sa T_LightFade
  */
void P_FadeLight(int tag, int destvalue, int speed)
{
	int i;
	lightlevel_t* ll;

	// search all sectors for ones with tag
	for(i = -1; (i = P_FindSectorFromTag(tag, i)) >= 0;)
	{
		sector_t* sector = &sectors[i];

		P_RemoveLighting(sector); // remove the old lighting effect first
		ll = Z_Malloc(sizeof(*ll), PU_LEVSPEC, 0);
		ll->thinker.function.acp1 = (actionf_p1)T_LightFade;
		sector->lightingdata = ll; // set it to the lightlevel_t

		P_AddThinker(&ll->thinker); // add thinker

		ll->sector = sector;
		ll->destlevel = destvalue;
		ll->speed = speed;
	}
}

/** Fades the light level in a sector to a new value.
  *
  * \param ll The fade effect under consideration.
  * \sa P_FadeLight
  */
void T_LightFade(lightlevel_t* ll)
{
	if(ll->sector->lightlevel < ll->destlevel)
	{
		// increase the lightlevel
		if(ll->sector->lightlevel + ll->speed >= ll->destlevel)
		{
			// stop changing light level
			ll->sector->lightlevel = (short)ll->destlevel; // set to dest lightlevel

			P_RemoveLighting(ll->sector); // clear lightingdata, remove thinker
		}
		else
			ll->sector->lightlevel = (short)(ll->sector->lightlevel + (short)ll->speed); // move lightlevel
	}
	else
	{
		// decrease lightlevel
		if(ll->sector->lightlevel - ll->speed <= ll->destlevel)
		{
			// stop changing light level
			ll->sector->lightlevel = (short)ll->destlevel; // set to dest lightlevel

			P_RemoveLighting(ll->sector); // clear lightingdata, remove thinker
		}
		else
			ll->sector->lightlevel = (short)(ll->sector->lightlevel - (short)ll->speed); // move lightlevel
	}
}
