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
/// \brief Archiving: SaveGame I/O

#include "doomdef.h"
#include "byteptr.h"
#include "d_main.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_things.h"
#include "r_state.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "r_main.h"
#include "r_sky.h"

byte* save_p;

// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
#ifdef SGI
// BP: this stuff isn't be removed but i think it will no more work
//     anyway what processor can't read/write unaligned data ?
#define PADSAVEP() save_p += (4 - ((int) save_p & 3)) & 3
#else
#define PADSAVEP()
#endif

typedef enum
{
	ATTACKDWN  = 0x0001,
	USEDWN     = 0x0002,
	JMPDWN     = 0x0004,
	AUTOAIM    = 0x0008,
	LASTAXIS   = 0x0010,
	RFLAGPOINT = 0x0020,
	BFLAGPOINT = 0x0040,
	CAPSULE    = 0x0080,
	AWAYVIEW   = 0x0100,
} player_saveflags;

//
// P_ArchivePlayers
//
static void P_ArchivePlayers(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i])
			continue;

		PADSAVEP();
		WRITELONG(save_p, players[i].bonuscount);
		WRITEBYTE(save_p, players[i].skincolor);
		WRITEBYTE(save_p, players[i].skin);

		WRITELONG(save_p, players[i].score);
		WRITEBYTE(save_p, players[i].charability);
		WRITEBYTE(save_p, players[i].charspin);
		WRITELONG(save_p, players[i].lives);
		WRITELONG(save_p, players[i].continues);
		WRITEBYTE(save_p, players[i].superready);
		WRITEBYTE(save_p, players[i].snowbuster);
		WRITELONG(save_p, players[i].xtralife);
	}
}

//
// P_UnArchivePlayers
//
static void P_UnArchivePlayers(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		// memset(&players[i],0 , sizeof(player_t));
		if(!playeringame[i])
			continue;

		PADSAVEP();
		players[i].bonuscount = READLONG(save_p);
		players[i].skincolor = READBYTE(save_p);
		players[i].skin = READBYTE(save_p);

		players[i].score = READLONG(save_p);
		players[i].charability = READBYTE(save_p);
		players[i].charspin = READBYTE(save_p);
		players[i].lives = READLONG(save_p);
		players[i].continues = READLONG(save_p);
		players[i].superready = READBYTE(save_p);
		players[i].snowbuster = READBYTE(save_p);
		players[i].xtralife = READLONG(save_p);
	}
}

//
// P_NetArchivePlayers
//
static void P_NetArchivePlayers(void)
{
	int i, j, q, flags;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i])
			continue;

		PADSAVEP();

		flags = 0;

		WRITEANGLE(save_p, players[i].aiming);
		WRITEANGLE(save_p, players[i].awayviewaiming);
		WRITELONG(save_p, players[i].awayviewtics);
		WRITELONG(save_p, players[i].health);

		for(j = 0; j < NUMPOWERS; j++)
			WRITELONG(save_p, players[i].powers[j]);

		WRITEBYTE(save_p, players[i].playerstate);

		if(players[i].attackdown)
			flags |= ATTACKDWN;
		if(players[i].usedown)
			flags |= USEDWN;
		if(players[i].jumpdown)
			flags |= JMPDWN;
		if(players[i].autoaim_toggle)
			flags |= AUTOAIM;

		WRITELONG(save_p, players[i].bonuscount);
		WRITELONG(save_p, players[i].specialsector);
		WRITELONG(save_p, players[i].skincolor);
		WRITELONG(save_p, players[i].skin);
		WRITELONG(save_p, players[i].score);
		WRITELONG(save_p, players[i].dashspeed);
		WRITELONG(save_p, players[i].lives);
		WRITELONG(save_p, players[i].continues);
		WRITELONG(save_p, players[i].superready);
		WRITELONG(save_p, players[i].xtralife);
		WRITELONG(save_p, players[i].walking);
		WRITELONG(save_p, players[i].running);
		WRITELONG(save_p, players[i].spinning);
		WRITELONG(save_p, players[i].speed);
		WRITELONG(save_p, players[i].jumping);
		WRITELONG(save_p, players[i].mfjumped);
		WRITELONG(save_p, players[i].mfspinning);
		WRITELONG(save_p, players[i].mfstartdash);
		WRITELONG(save_p, players[i].fly1);
		WRITELONG(save_p, players[i].scoreadd);
		WRITELONG(save_p, players[i].gliding);
		WRITELONG(save_p, players[i].glidetime);
		WRITELONG(save_p, players[i].climbing);
		WRITELONG(save_p, players[i].deadtimer);
		WRITELONG(save_p, players[i].splish);
		WRITELONG(save_p, players[i].exiting);
		WRITELONG(save_p, players[i].blackow);
		WRITEBYTE(save_p, players[i].homing);

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		WRITEFIXED(save_p, players[i].cmomx); // Conveyor momx
		WRITEFIXED(save_p, players[i].cmomy); // Conveyor momy
		WRITEFIXED(save_p, players[i].rmomx); // "Real" momx (momx - cmomx)
		WRITEFIXED(save_p, players[i].rmomy); // "Real" momy (momy - cmomy)

		/////////////////////
		// Race Mode Stuff //
		/////////////////////
		WRITELONG(save_p, players[i].numboxes);
		WRITELONG(save_p, players[i].totalring);
		WRITELONG(save_p, players[i].realtime);
		WRITELONG(save_p, players[i].racescore);

		////////////////////
		// Tag Mode Stuff //
		////////////////////
		WRITELONG(save_p, players[i].tagit);
		WRITELONG(save_p, players[i].tagcount);
		WRITELONG(save_p, players[i].tagzone);
		WRITELONG(save_p, players[i].taglag);

		////////////////////
		// CTF Mode Stuff //
		////////////////////
		WRITELONG(save_p, players[i].ctfteam);
		WRITEUSHORT(save_p, players[i].gotflag);

		WRITELONG(save_p, players[i].dbginfo);
		WRITELONG(save_p, players[i].emeraldhunt);
		WRITELONG(save_p, players[i].snowbuster);
		WRITELONG(save_p, players[i].bustercount);

		WRITELONG(save_p, players[i].weapondelay);
		WRITELONG(save_p, players[i].taunttimer);

		WRITELONG(save_p, players[i].starposttime);
		WRITELONG(save_p, players[i].starpostx);
		WRITELONG(save_p, players[i].starposty);
		WRITELONG(save_p, players[i].starpostz);
		WRITELONG(save_p, players[i].starpostnum);
		WRITEANGLE(save_p, players[i].starpostangle);
		WRITEULONG(save_p, players[i].starpostbit);

		WRITEDOUBLE(save_p, players[i].angle_speed);
		WRITEDOUBLE(save_p, players[i].angle_pos);
		WRITEDOUBLE(save_p, players[i].old_angle_pos);
		WRITEBYTE(save_p, players[i].nightsmode);

		WRITEBYTE(save_p, players[i].axishit);
		WRITEBYTE(save_p, players[i].axistransferred);
		WRITEBYTE(save_p, players[i].transfertoclosest);
		WRITELONG(save_p, players[i].flyangle);
		WRITELONG(save_p, players[i].drilltimer);
		WRITELONG(save_p, players[i].linkcount);
		WRITELONG(save_p, players[i].linktimer);
		WRITELONG(save_p, players[i].anotherflyangle);
		WRITELONG(save_p, players[i].transferangle);
		WRITELONG(save_p, players[i].nightstime);
		WRITEBYTE(save_p, players[i].nightsfall);
		WRITELONG(save_p, players[i].drillmeter);
		WRITEBYTE(save_p, players[i].drilldelay);
		WRITEBYTE(save_p, players[i].drilling);
		WRITEBYTE(save_p, players[i].skiddown);
		WRITEBYTE(save_p, players[i].bonustime);
		WRITEBYTE(save_p, players[i].mare);

		if(players[i].lastaxis)
			flags |= LASTAXIS;

		if(players[i].capsule)
			flags |= CAPSULE;

		if(players[i].awayviewmobj)
			flags |= AWAYVIEW;

		WRITESHORT(save_p, players[i].lastsidehit);
		WRITESHORT(save_p, players[i].lastlinehit);

		WRITEBYTE(save_p, players[i].carried);

		WRITELONG(save_p, players[i].losscount);

		WRITEBYTE(save_p, players[i].lightdash);
		WRITEBYTE(save_p, players[i].lightdashallowed);

		WRITEBYTE(save_p, players[i].thokked);

		WRITELONG(save_p, players[i].onconveyor);

		if(i == 0)
		{
			for(q = 0; q < nummapthings; q++)
			{
				if(&mapthings[q] == rflagpoint)
				{
					flags |= RFLAGPOINT;
					break;
				}
			}

			for(q = 0; q < nummapthings; q++)
			{
				if(&mapthings[q] == bflagpoint)
				{
					flags |= BFLAGPOINT;
					break;
				}
			}
		}

		WRITEUSHORT(save_p, flags);

		if(flags & LASTAXIS)
			WRITEULONG(save_p, players[i].lastaxis->mobjnum);

		if(flags & CAPSULE)
			WRITEULONG(save_p, players[i].capsule->mobjnum);

		if(flags & AWAYVIEW)
			WRITEULONG(save_p, players[i].awayviewmobj->mobjnum);

		if(flags & RFLAGPOINT)
			for(q = 0; q < nummapthings; q++)
			{
				if(&mapthings[q] == rflagpoint)
				{
					WRITEULONG(save_p, q);
					break;
				}
			}

		if(flags & BFLAGPOINT)
			for(q = 0; q < nummapthings; q++)
			{
				if(&mapthings[q] == bflagpoint)
				{
					WRITEULONG(save_p, q);
					break;
				}
			}
	}
}

//
// P_NetUnArchivePlayers
//
static void P_NetUnArchivePlayers(void)
{
	int i, j, flags;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		memset(&players[i], 0, sizeof(player_t));
		if(!playeringame[i])
			continue;

		PADSAVEP();

		players[i].aiming = READANGLE(save_p);
		players[i].awayviewaiming = READANGLE(save_p);
		players[i].awayviewtics = READLONG(save_p);
		players[i].health = READLONG(save_p);

		for(j = 0; j < NUMPOWERS; j++)
			players[i].powers[j] = READLONG(save_p);

		players[i].playerstate = READBYTE(save_p);

		players[i].bonuscount = READLONG(save_p);

		players[i].specialsector = READLONG(save_p);

		players[i].skincolor = READLONG(save_p);
		players[i].skin = READLONG(save_p);

		players[i].score = READLONG(save_p);

		players[i].dashspeed = READLONG(save_p); // dashing speed

		players[i].lives = READLONG(save_p);
		players[i].continues = READLONG(save_p); // continues that player has acquired

		players[i].superready = READLONG(save_p); // Ready for Super?

		players[i].xtralife = READLONG(save_p); // Ring Extra Life counter

		players[i].walking = READLONG(save_p); // Are the walking frames playing?
		players[i].running = READLONG(save_p); // Are the running frames playing?
		players[i].spinning = READLONG(save_p); // Are the spinning frames playing?
		players[i].speed = READLONG(save_p); // Player's speed (distance formula of MOMX and MOMY values)
		players[i].jumping = READLONG(save_p); // Jump counter

		// Moved eflags to player ints
		players[i].mfjumped = READLONG(save_p);
		players[i].mfspinning = READLONG(save_p);
		players[i].mfstartdash = READLONG(save_p);

		players[i].fly1 = READLONG(save_p); // Tails flying
		players[i].scoreadd = READLONG(save_p); // Used for multiple enemy attack bonus
		players[i].gliding = READLONG(save_p); // Are you gliding?
		players[i].glidetime = READLONG(save_p); // Glide counter for thrust
		players[i].climbing = READLONG(save_p); // Climbing on the wall
		players[i].deadtimer = READLONG(save_p); // End game if game over lasts too long
		players[i].splish = READLONG(save_p); // Don't make splish repeat tons
		players[i].exiting = READLONG(save_p); // Exitlevel timer
		players[i].blackow = READLONG(save_p);

		players[i].homing = READBYTE(save_p); // Are you homing?

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		players[i].cmomx = READFIXED(save_p); // Conveyor momx
		players[i].cmomy = READFIXED(save_p); // Conveyor momy
		players[i].rmomx = READFIXED(save_p); // "Real" momx (momx - cmomx)
		players[i].rmomy = READFIXED(save_p); // "Real" momy (momy - cmomy)

		/////////////////////
		// Race Mode Stuff //
		/////////////////////
		players[i].numboxes = READLONG(save_p); // Number of item boxes obtained for Race Mode
		players[i].totalring = READLONG(save_p); // Total number of rings obtained for Race Mode
		players[i].realtime = READLONG(save_p); // integer replacement for leveltime
		players[i].racescore = READLONG(save_p); // Total of won categories

		////////////////////
		// Tag Mode Stuff //
		////////////////////
		players[i].tagit = READLONG(save_p); // The player is it! For Tag Mode
		players[i].tagcount = READLONG(save_p); // Number of tags player has avoided
		players[i].tagzone = READLONG(save_p); // Tag Zone timer
		players[i].taglag = READLONG(save_p); // Don't go back in the tag zone too early

		////////////////////
		// CTF Mode Stuff //
		////////////////////
		players[i].ctfteam = READLONG(save_p); // 1 == Red, 2 == Blue
		players[i].gotflag = READUSHORT(save_p); // 1 == Red, 2 == Blue Do you have the flag?

		players[i].dbginfo = READLONG(save_p); // Debugger
		players[i].emeraldhunt = READLONG(save_p); // # of emeralds found
		players[i].snowbuster = READLONG(save_p); // Snow Buster upgrade!
		players[i].bustercount = READLONG(save_p); // Charge for Snow Buster

		players[i].weapondelay = READLONG(save_p);
		players[i].taunttimer = READLONG(save_p);

		players[i].starposttime = READLONG(save_p);
		players[i].starpostx = READLONG(save_p);
		players[i].starposty = READLONG(save_p);
		players[i].starpostz = READLONG(save_p);
		players[i].starpostnum = READLONG(save_p);
		players[i].starpostangle = READANGLE(save_p);
		players[i].starpostbit = READULONG(save_p);

		players[i].angle_speed = READDOUBLE(save_p);
		players[i].angle_pos = READDOUBLE(save_p);
		players[i].old_angle_pos = READDOUBLE(save_p);
		players[i].nightsmode = READBYTE(save_p);

		players[i].axishit = READBYTE(save_p);
		players[i].axistransferred = READBYTE(save_p);
		players[i].transfertoclosest = READBYTE(save_p);
		players[i].flyangle = READLONG(save_p);
		players[i].drilltimer = READLONG(save_p);
		players[i].linkcount = READLONG(save_p);
		players[i].linktimer = READLONG(save_p);
		players[i].anotherflyangle = READLONG(save_p);
		players[i].transferangle = READLONG(save_p);
		players[i].nightstime = READLONG(save_p);
		players[i].nightsfall = READBYTE(save_p);
		players[i].drillmeter = READLONG(save_p);
		players[i].drilldelay = READBYTE(save_p);
		players[i].drilling = READBYTE(save_p);
		players[i].skiddown = READBYTE(save_p);
		players[i].bonustime = READBYTE(save_p);
		players[i].mare = READBYTE(save_p);

		players[i].lastsidehit = READSHORT(save_p);
		players[i].lastlinehit = READSHORT(save_p);

		players[i].carried = READBYTE(save_p);

		players[i].losscount = READLONG(save_p);

		players[i].lightdash = READBYTE(save_p);
		players[i].lightdashallowed = READBYTE(save_p);

		players[i].thokked = READBYTE(save_p);

		players[i].onconveyor = READLONG(save_p);

		flags = READUSHORT(save_p);

		players[i].attackdown = (flags & ATTACKDWN) != 0;
		players[i].usedown = (flags & USEDWN) != 0;
		players[i].jumpdown = (flags & JMPDWN) != 0;
		players[i].autoaim_toggle = (flags & AUTOAIM) != 0;

		if(flags & LASTAXIS)
			players[i].lastaxis = (mobj_t*)(size_t)READULONG(save_p);

		if(flags & CAPSULE)
			players[i].capsule = (mobj_t*)(size_t)READULONG(save_p);

		if(flags & AWAYVIEW)
			players[i].awayviewmobj = (mobj_t*)(size_t)READULONG(save_p);

		if(i == 0)
		{
			if(flags & RFLAGPOINT)
				rflagpoint = &mapthings[READULONG(save_p)];

			if(flags & BFLAGPOINT)
				bflagpoint = &mapthings[READULONG(save_p)];
		}

		players[i].viewheight = cv_viewheight.value<<FRACBITS;

		SetPlayerSkinByNum(i, players[i].skin);
	}
}

#define SD_FLOORHT  0x01
#define SD_CEILHT   0x02
#define SD_FLOORPIC 0x04
#define SD_CEILPIC  0x08
#define SD_LIGHT    0x10
#define SD_SPECIAL  0x20
#define SD_DIFF2    0x40

// diff2 flags
#define SD_FXOFFS    0x01
#define SD_FYOFFS    0x02
#define SD_CXOFFS    0x04
#define SD_CYOFFS    0x08
#define SD_TAG       0x10

#define LD_FLAG     0x01
#define LD_SPECIAL  0x02
#define LD_S1TEXOFF 0x08
#define LD_S1TOPTEX 0x10
#define LD_S1BOTTEX 0x20
#define LD_S1MIDTEX 0x40
#define LD_DIFF2    0x80

// diff2 flags
#define LD_S2TEXOFF 0x01
#define LD_S2TOPTEX 0x02
#define LD_S2BOTTEX 0x04
#define LD_S2MIDTEX 0x08

//
// P_NetArchiveWorld
//
static void P_NetArchiveWorld(void)
{
	int i, statsec = 0, statline = 0;
	line_t* li;
	side_t* si;
	byte* put;

	// reload the map just to see difference
	mapsector_t* ms;
	mapsidedef_t* msd;
	maplinedef_t* mld;
	sector_t* ss;
	byte diff, diff2;

	ms = W_CacheLumpNum(lastloadedmaplumpnum+ML_SECTORS, PU_CACHE);
	ss = sectors;
	put = save_p;

	for(i = 0; i < numsectors; i++, ss++, ms++)
	{
		diff = diff2 = 0;
		if(ss->floorheight != SHORT(ms->floorheight)<<FRACBITS)
			diff |= SD_FLOORHT;
		if(ss->ceilingheight != SHORT(ms->ceilingheight)<<FRACBITS)
			diff |= SD_CEILHT;
		//
		// flats
		//
		// P_AddLevelFlat should not add but just return the number
		if(ss->floorpic != P_AddLevelFlat(ms->floorpic, levelflats))
			diff |= SD_FLOORPIC;
		if(ss->ceilingpic != P_AddLevelFlat(ms->ceilingpic, levelflats))
			diff |= SD_CEILPIC;

		if(ss->lightlevel != SHORT(ms->lightlevel))
			diff |= SD_LIGHT;
		if(ss->special != SHORT(ms->special))
			diff |= SD_SPECIAL;

		/// \todo this makes Flat Alignment (linetype 66) increase the savegame size!
		if(ss->floor_xoffs != 0)
			diff2 |= SD_FXOFFS;
		if(ss->floor_yoffs != 0)
			diff2 |= SD_FYOFFS;
		if(ss->ceiling_xoffs != 0)
			diff2 |= SD_CXOFFS;
		if(ss->ceiling_yoffs != 0)
			diff2 |= SD_CYOFFS;

		if(ss->tag != SHORT(ms->tag))
			diff2 |= SD_TAG;

		if(diff2)
			diff |= SD_DIFF2;

		if(diff)
		{
			statsec++;

			WRITESHORT(put, i);
			WRITEBYTE(put, diff);
			if(diff & SD_DIFF2)
				WRITEBYTE(put, diff2);
			if(diff & SD_FLOORHT)
				WRITEFIXED(put, ss->floorheight);
			if(diff & SD_CEILHT)
				WRITEFIXED(put, ss->ceilingheight);
			if(diff & SD_FLOORPIC)
			{
				memcpy(put, levelflats[ss->floorpic].name, 8);
				put += 8;
			}
			if(diff & SD_CEILPIC)
			{
				memcpy(put, levelflats[ss->ceilingpic].name, 8);
				put += 8;
			}
			if(diff & SD_LIGHT)
				WRITESHORT(put, (short)ss->lightlevel);
			if(diff & SD_SPECIAL)
				WRITESHORT(put, (short)ss->special);

			if(diff2 & SD_FXOFFS)
				WRITEFIXED(put, ss->floor_xoffs);
			if(diff2 & SD_FYOFFS)
				WRITEFIXED(put, ss->floor_yoffs);
			if(diff2 & SD_CXOFFS)
				WRITEFIXED(put, ss->ceiling_xoffs);
			if(diff2 & SD_CYOFFS)
				WRITEFIXED(put, ss->ceiling_yoffs);
			if(diff2 & SD_TAG)
			{
				WRITESHORT(put, ss->tag);
				WRITELONG(put, ss->firsttag);
				WRITELONG(put, ss->nexttag);
			}
		}
	}

	WRITEUSHORT(put, 0xffff);

	mld = W_CacheLumpNum(lastloadedmaplumpnum+ML_LINEDEFS, PU_CACHE);
	msd = W_CacheLumpNum(lastloadedmaplumpnum+ML_SIDEDEFS, PU_CACHE);
	li = lines;
	// do lines
	for(i = 0; i < numlines; i++, mld++, li++)
	{
		diff = diff2 = 0;

		if(li->special != SHORT(mld->special))
			diff |= LD_SPECIAL;

		if(li->sidenum[0] != -1)
		{
			si = &sides[li->sidenum[0]];
			if(si->textureoffset != SHORT(msd[li->sidenum[0]].textureoffset)<<FRACBITS)
				diff |= LD_S1TEXOFF;
			//SoM: 4/1/2000: Some textures are colormaps. Don't worry about invalid textures.
			if(R_CheckTextureNumForName(msd[li->sidenum[0]].toptexture) != -1
				&& si->toptexture != R_TextureNumForName(msd[li->sidenum[0]].toptexture))
				diff |= LD_S1TOPTEX;
			if(R_CheckTextureNumForName(msd[li->sidenum[0]].bottomtexture) != -1
				&& si->bottomtexture != R_TextureNumForName(msd[li->sidenum[0]].bottomtexture))
				diff |= LD_S1BOTTEX;
			if(R_CheckTextureNumForName(msd[li->sidenum[0]].midtexture) != -1
				&& si->midtexture != R_TextureNumForName(msd[li->sidenum[0]].midtexture))
				diff |= LD_S1MIDTEX;
		}
		if(li->sidenum[1] != -1)
		{
			si = &sides[li->sidenum[1]];
			if(si->textureoffset != SHORT(msd[li->sidenum[1]].textureoffset)<<FRACBITS)
				diff2 |= LD_S2TEXOFF;
			if(R_CheckTextureNumForName(msd[li->sidenum[1]].toptexture) != -1
				&& si->toptexture != R_TextureNumForName(msd[li->sidenum[1]].toptexture))
				diff2 |= LD_S2TOPTEX;
			if(R_CheckTextureNumForName(msd[li->sidenum[1]].bottomtexture) != -1
				&& si->bottomtexture != R_TextureNumForName(msd[li->sidenum[1]].bottomtexture))
				diff2 |= LD_S2BOTTEX;
			if(R_CheckTextureNumForName(msd[li->sidenum[1]].midtexture) != -1
				&& si->midtexture != R_TextureNumForName(msd[li->sidenum[1]].midtexture))
				diff2 |= LD_S2MIDTEX;
			if(diff2)
				diff |= LD_DIFF2;
		}

		if(diff)
		{
			statline++;
			WRITESHORT(put, (short)i);
			WRITEBYTE(put, diff);
			if(diff & LD_DIFF2)
				WRITEBYTE(put, diff2);
			if(diff & LD_FLAG)
				WRITESHORT(put, li->flags);
			if(diff & LD_SPECIAL)
				WRITESHORT(put, li->special);

			si = &sides[li->sidenum[0]];
			if(diff & LD_S1TEXOFF)
				WRITEFIXED(put, si->textureoffset);
			if(diff & LD_S1TOPTEX)
				WRITEFIXED(put, si->toptexture);
			if(diff & LD_S1BOTTEX)
				WRITEFIXED(put, si->bottomtexture);
			if(diff & LD_S1MIDTEX)
				WRITEFIXED(put, si->midtexture);

			si = &sides[li->sidenum[1]];
			if(diff2 & LD_S2TEXOFF)
				WRITEFIXED(put, si->textureoffset);
			if(diff2 & LD_S2TOPTEX)
				WRITEFIXED(put, si->toptexture);
			if(diff2 & LD_S2BOTTEX)
				WRITEFIXED(put, si->bottomtexture);
			if(diff2 & LD_S2MIDTEX)
				WRITEFIXED(put, si->midtexture);
		}
	}
	WRITEUSHORT(put, 0xffff);

	save_p = put;
}

//
// P_NetUnArchiveWorld
//
static void P_NetUnArchiveWorld(void)
{
	int i;
	line_t* li;
	side_t* si;
	byte* get;
	byte diff, diff2;

	get = save_p;

	for(;;)
	{
		i = READUSHORT(get);

		if(i == 0xffff)
			break;

		diff = READBYTE(get);
		if(diff & SD_DIFF2)
			diff2 = READBYTE(get);
		else
			diff2 = 0;

		if(diff & SD_FLOORHT)
			sectors[i].floorheight = READFIXED(get);
		if(diff & SD_CEILHT)
			sectors[i].ceilingheight = READFIXED(get);
		if(diff & SD_FLOORPIC)
		{
			sectors[i].floorpic = P_AddLevelFlat((char *)get, levelflats);
			get += 8;
		}
		if(diff & SD_CEILPIC)
		{
			sectors[i].ceilingpic = P_AddLevelFlat((char *)get, levelflats);
			get += 8;
		}
		if(diff & SD_LIGHT)
			sectors[i].lightlevel = READSHORT(get);
		if(diff & SD_SPECIAL)
			sectors[i].special = READSHORT(get);

		if(diff2 & SD_FXOFFS)
			sectors[i].floor_xoffs = READFIXED(get);
		if(diff2 & SD_FYOFFS)
			sectors[i].floor_yoffs = READFIXED(get);
		if(diff2 & SD_CXOFFS)
			sectors[i].ceiling_xoffs = READFIXED(get);
		if(diff2 & SD_CYOFFS)
			sectors[i].ceiling_yoffs = READFIXED(get);
		if(diff2 & SD_TAG)
		{
			int tag;
			tag = READSHORT(get);
			sectors[i].firsttag = READLONG(get);
			sectors[i].nexttag = READLONG(get);
			P_ChangeSectorTag(i, tag);
		}
	}

	for(;;)
	{
		i = READUSHORT(get);

		if(i == 0xffff)
			break;
		diff = READBYTE(get);
		li = &lines[i];

		if(diff & LD_DIFF2)
			diff2 = READBYTE(get);
		else
			diff2 = 0;
		if(diff & LD_FLAG)
			li->flags = READSHORT(get);
		if(diff & LD_SPECIAL)
			li->special = READSHORT(get);

		si = &sides[li->sidenum[0]];
		if(diff & LD_S1TEXOFF)
			si->textureoffset = READFIXED(get);
		if(diff & LD_S1TOPTEX)
			si->toptexture = READFIXED(get);
		if(diff & LD_S1BOTTEX)
			si->bottomtexture = READFIXED(get);
		if(diff & LD_S1MIDTEX)
			si->midtexture = READFIXED(get);

		si = &sides[li->sidenum[1]];
		if(diff2 & LD_S2TEXOFF)
			si->textureoffset = READFIXED(get);
		if(diff2 & LD_S2TOPTEX)
			si->toptexture = READFIXED(get);
		if(diff2 & LD_S2BOTTEX)
			si->bottomtexture = READFIXED(get);
		if(diff2 & LD_S2MIDTEX)
			si->midtexture = READFIXED(get);
	}

	save_p = get;
}

//
// Thinkers
//

typedef enum
{
	MD_SPAWNPOINT  = 0x0000001,
	MD_POS         = 0x0000002,
	MD_TYPE        = 0x0000004,
	MD_MOM         = 0x0000008,
	MD_RADIUS      = 0x0000010,
	MD_HEIGHT      = 0x0000020,
	MD_FLAGS       = 0x0000040,
	MD_HEALTH      = 0x0000080,
	MD_RTIME       = 0x0000100,
	MD_STATE       = 0x0000200,
	MD_TICS        = 0x0000400,
	MD_SPRITE      = 0x0000800,
	MD_FRAME       = 0x0001000,
	MD_EFLAGS      = 0x0002000,
	MD_PLAYER      = 0x0004000,
	MD_MOVEDIR     = 0x0008000,
	MD_MOVECOUNT   = 0x0010000,
	MD_THRESHOLD   = 0x0020000,
	MD_LASTLOOK    = 0x0040000,
	MD_TARGET      = 0x0080000,
	MD_TRACER      = 0x0100000,
	MD_FRICTION    = 0x0200000,
	MD_MOVEFACTOR  = 0x0400000,
	MD_FLAGS2      = 0x0800000,
	MD_FUSE        = 0x1000000,
	MD_WATERTOP    = 0x2000000,
	MD_WATERBOTTOM = 0x4000000,
} mobj_diff_t;

typedef enum
{
	tc_mobj,
	tc_ceiling,
	tc_floor,
	tc_flash,
	tc_strobe,
	tc_glow,
	tc_fireflicker,
	tc_thwomp,
	tc_camerascanner,
	tc_elevator,
	tc_continuousfalling,
	tc_bouncecheese,
	tc_startcrumble,
	tc_airbob,
	tc_marioblock,
	tc_spikesector,
	tc_floatsector,
	tc_crushceiling,
	tc_scroll,
	tc_friction,
	tc_pusher,
	tc_laserflash,
	tc_lightfade,
	tc_executor,
	tc_end
} specials_e;

//
// P_NetArchiveThinkers
//
//
static void P_NetArchiveThinkers(void)
{
	thinker_t* th;
	mobj_t* mobj;
	ULONG diff;
	int i = 0;

	// Assign the mobjnumber for pointer tracking
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			mobj = (mobj_t*)th;
			mobj->mobjnum = i++;
		}
	}

	// save off the current thinkers
	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if(th->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			mobj = (mobj_t*)th;
			if(mobj->spawnpoint && (mobj->info->doomednum != -1))
			{
				// spawnpoint is not modified but we must save it since it is an identifier
				diff = MD_SPAWNPOINT;

				if((mobj->x != mobj->spawnpoint->x << FRACBITS) ||
					(mobj->y != mobj->spawnpoint->y << FRACBITS) ||
					(mobj->angle != (unsigned)(ANG45 * (mobj->spawnpoint->angle/45))))
					diff |= MD_POS;
				if(mobj->info->doomednum != mobj->spawnpoint->type)
					diff |= MD_TYPE;
			}
			else
				diff = MD_POS | MD_TYPE; // not a map spawned thing so make it from scratch

			// not the default but the most probable
			if(mobj->momx != 0 || mobj->momy != 0 || mobj->momz != 0)
				diff |= MD_MOM;
			if(mobj->radius != mobj->info->radius)
				diff |= MD_RADIUS;
			if(mobj->height != mobj->info->height)
				diff |= MD_HEIGHT;
			if(mobj->flags != mobj->info->flags)
				diff |= MD_FLAGS;
			diff |= MD_FLAGS2; // Force saving of flags2
			if(mobj->health != mobj->info->spawnhealth)
				diff |= MD_HEALTH;
			if(mobj->reactiontime != mobj->info->reactiontime)
				diff |= MD_RTIME;
			if((statenum_t)(mobj->state-states) != mobj->info->spawnstate)
				diff |= MD_STATE;
			if(mobj->tics != mobj->state->tics)
				diff |= MD_TICS;
			if(mobj->sprite != mobj->state->sprite)
				diff |= MD_SPRITE;
			if(mobj->frame != mobj->state->frame)
				diff |= MD_FRAME;
			if(mobj->eflags)
				diff |= MD_EFLAGS;
			if(mobj->player)
				diff |= MD_PLAYER;

			if(mobj->movedir)
				diff |= MD_MOVEDIR;
			if(mobj->movecount)
				diff |= MD_MOVECOUNT;
			if(mobj->threshold)
				diff |= MD_THRESHOLD;
			if(mobj->lastlook != -1)
				diff |= MD_LASTLOOK;
			if(mobj->target)
				diff |= MD_TARGET;
			if(mobj->tracer)
				diff |= MD_TRACER;
			if(mobj->friction != ORIG_FRICTION)
				diff |= MD_FRICTION;
			if(mobj->movefactor != ORIG_FRICTION_FACTOR)
				diff |= MD_MOVEFACTOR;
			if(mobj->fuse)
				diff |= MD_FUSE;
			if(mobj->watertop)
				diff |= MD_WATERTOP;
			if(mobj->waterbottom)
				diff |= MD_WATERBOTTOM;

			PADSAVEP();
			WRITEBYTE(save_p, tc_mobj);
			WRITEULONG(save_p, diff);

			// save pointer, at load time we will search this pointer to reinitilize pointers
			WRITEULONG(save_p, (ULONG)(size_t)mobj);

			WRITEFIXED(save_p, mobj->z); // Force this so 3dfloor problems don't arise.
			WRITEFIXED(save_p, mobj->floorz); /// \todo floorff too?
			WRITEFIXED(save_p, mobj->ceilingz);

			if(diff & MD_SPAWNPOINT)
			{
				int z;

				for(z = 0; z < nummapthings; z++)
					if(&mapthings[z] == mobj->spawnpoint)
						WRITESHORT(save_p, z);
			}

			if(diff & MD_TYPE)
				WRITEULONG(save_p, mobj->type);
			if(diff & MD_POS)
			{
				WRITEFIXED(save_p, mobj->x);
				WRITEFIXED(save_p, mobj->y);
				WRITEANGLE(save_p, mobj->angle);
			}
			if(diff & MD_MOM)
			{
				WRITEFIXED(save_p, mobj->momx);
				WRITEFIXED(save_p, mobj->momy);
				WRITEFIXED(save_p, mobj->momz);
			}
			if(diff & MD_RADIUS)
				WRITEFIXED(save_p, mobj->radius);
			if(diff & MD_HEIGHT)
				WRITEFIXED(save_p, mobj->height);
			if(diff & MD_FLAGS)
				WRITELONG(save_p, mobj->flags);
			if(diff & MD_FLAGS2)
				WRITELONG(save_p, mobj->flags2);
			if(diff & MD_HEALTH)
				WRITELONG(save_p, mobj->health);
			if(diff & MD_RTIME)
				WRITELONG(save_p, mobj->reactiontime);
			if(diff & MD_STATE)
				WRITEUSHORT(save_p, mobj->state-states);
			if(diff & MD_TICS)
				WRITELONG(save_p, mobj->tics);
			if(diff & MD_SPRITE)
				WRITEUSHORT(save_p, mobj->sprite);
			if(diff & MD_FRAME)
				WRITEULONG(save_p, mobj->frame);
			if(diff & MD_EFLAGS)
				WRITEULONG(save_p, mobj->eflags);
			if(diff & MD_PLAYER)
				WRITEBYTE(save_p, mobj->player-players);
			if(diff & MD_MOVEDIR)
				WRITELONG(save_p, mobj->movedir);
			if(diff & MD_MOVECOUNT)
				WRITELONG(save_p, mobj->movecount);
			if(diff & MD_THRESHOLD)
				WRITELONG(save_p, mobj->threshold);
			if(diff & MD_LASTLOOK)
				WRITELONG(save_p, mobj->lastlook);
			if(diff & MD_TARGET)
				WRITEULONG(save_p, mobj->target->mobjnum);
			if(diff & MD_TRACER)
				WRITEULONG(save_p, mobj->tracer->mobjnum);
			if(diff & MD_FRICTION)
				WRITELONG(save_p, mobj->friction);
			if(diff & MD_MOVEFACTOR)
				WRITELONG(save_p, mobj->movefactor);
			if(diff & MD_FUSE)
				WRITELONG(save_p, mobj->fuse);
			if(diff & MD_WATERTOP)
				WRITELONG(save_p, mobj->watertop);
			if(diff & MD_WATERBOTTOM)
				WRITELONG(save_p, mobj->waterbottom);

			// Special case for use of the block and sector links
			if(mobj->type == MT_HOOPCOLLIDE || mobj->type == MT_HOOP)
			{
				if(mobj->bnext)
					WRITEULONG(save_p, mobj->bnext->mobjnum);
				else
					WRITEULONG(save_p, 0xFFFFFFFF);

				if(mobj->bprev)
					WRITEULONG(save_p, mobj->bprev->mobjnum);
				else
					WRITEULONG(save_p, 0xFFFFFFFF);

				if(mobj->snext)
					WRITEULONG(save_p, mobj->snext->mobjnum);
				else
					WRITEULONG(save_p, 0xFFFFFFFF);

				if(mobj->sprev)
					WRITEULONG(save_p, mobj->sprev->mobjnum);
				else
					WRITEULONG(save_p, 0xFFFFFFFF);
			}

			WRITEULONG(save_p, mobj->mobjnum);
		}
#ifdef PARANOIA
		else if(th->function.acp1 == (actionf_p1)P_RainThinker
			|| th->function.acp1 == (actionf_p1)P_SnowThinker);
#endif
		else if(th->function.acp1 == (actionf_p1)T_MoveCeiling)
		{
			ceiling_t* ceiling;
			WRITEBYTE(save_p, tc_ceiling);
			PADSAVEP();
			ceiling = (ceiling_t*)save_p;
			memcpy(ceiling, th, sizeof(*ceiling));
			save_p += sizeof(*ceiling);
			ceiling->sector = (sector_t*)(ceiling->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_CrushCeiling)
		{
			ceiling_t* ceiling;
			WRITEBYTE(save_p, tc_crushceiling);
			PADSAVEP();
			ceiling = (ceiling_t*)save_p;
			memcpy(ceiling, th, sizeof(*ceiling));
			save_p += sizeof(*ceiling);
			ceiling->sector = (sector_t*)(ceiling->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_MoveFloor)
		{
			floormove_t* floor;
			WRITEBYTE(save_p, tc_floor);
			PADSAVEP();
			floor = (floormove_t*)save_p;
			memcpy(floor, th, sizeof(*floor));
			save_p += sizeof(*floor);
			floor->sector = (sector_t*)(floor->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_LightningFlash)
		{
			lightflash_t* flash;
			WRITEBYTE(save_p, tc_flash);
			PADSAVEP();
			flash = (lightflash_t*)save_p;
			memcpy(flash, th, sizeof(*flash));
			save_p += sizeof(*flash);
			flash->sector = (sector_t*)(flash->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_StrobeFlash)
		{
			strobe_t* strobe;
			WRITEBYTE(save_p, tc_strobe);
			PADSAVEP();
			strobe = (strobe_t*)save_p;
			memcpy(strobe, th, sizeof(*strobe));
			save_p += sizeof(*strobe);
			strobe->sector = (sector_t*)(strobe->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_Glow)
		{
			glow_t* glow;
			WRITEBYTE(save_p, tc_glow);
			PADSAVEP();
			glow = (glow_t*)save_p;
			memcpy (glow, th, sizeof(*glow));
			save_p += sizeof(*glow);
			glow->sector = (sector_t*)(glow->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_FireFlicker)
		{
			fireflicker_t* fireflicker;
			WRITEBYTE(save_p, tc_fireflicker);
			PADSAVEP();
			fireflicker = (fireflicker_t*)save_p;
			memcpy(fireflicker, th, sizeof(*fireflicker));
			save_p += sizeof(*fireflicker);
			fireflicker->sector = (sector_t*)(fireflicker->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_MoveElevator)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_elevator);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_ContinuousFalling)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_continuousfalling);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_ThwompSector)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_thwomp);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->actionsector = (sector_t*)(elevator->actionsector - sectors);
			elevator->sourceline = (line_t*)(elevator->sourceline - lines);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_CameraScanner)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_camerascanner);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->actionsector = (sector_t*)(elevator->actionsector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_Scroll)
		{
			WRITEBYTE(save_p, tc_scroll);
			memcpy(save_p, th, sizeof(scroll_t));
			save_p += sizeof(scroll_t);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_Friction)
		{
			WRITEBYTE(save_p, tc_friction);
			memcpy(save_p, th, sizeof(friction_t));
			save_p += sizeof(friction_t);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_Pusher)
		{
			WRITEBYTE(save_p, tc_pusher);
			memcpy(save_p, th, sizeof(pusher_t));
			save_p += sizeof(pusher_t);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_BounceCheese)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_bouncecheese);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->actionsector = (sector_t*)(elevator->actionsector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_StartCrumble)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_startcrumble);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->actionsector = (sector_t*)(elevator->actionsector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_AirBob)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_airbob);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->player = (player_t*)(elevator->player - players);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_MarioBlock)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_marioblock);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy (elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_SpikeSector)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_spikesector);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_FloatSector)
		{
			elevator_t* elevator;
			WRITEBYTE(save_p, tc_floatsector);
			PADSAVEP();
			elevator = (elevator_t*)save_p;
			memcpy(elevator, th, sizeof(*elevator));
			save_p += sizeof(*elevator);
			elevator->sector = (sector_t*)(elevator->sector - sectors);
			elevator->actionsector = (sector_t*)(elevator->actionsector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_LaserFlash)
		{
			laserthink_t* laser;
			WRITEBYTE(save_p, tc_laserflash);
			PADSAVEP();
			laser = (laserthink_t*)save_p;
			memcpy(laser, th, sizeof(*laser));
			save_p += sizeof(*laser);
			laser->ffloor = (ffloor_t*)(laser->ffloor - laser->sector->ffloors);
			laser->sector = (sector_t*)(laser->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_LightFade)
		{
			lightlevel_t* ll;
			WRITEBYTE(save_p, tc_lightfade);
			PADSAVEP();
			ll = (lightlevel_t*)save_p;
			memcpy(ll, th, sizeof(*ll));
			save_p += sizeof(*ll);
			ll->sector = (sector_t*)(ll->sector - sectors);
			continue;
		}
		else if(th->function.acp1 == (actionf_p1)T_ExecutorDelay)
		{
			executor_t* e;
			WRITEBYTE(save_p, tc_executor);
			PADSAVEP();
			e = (executor_t*)save_p;
			memcpy(e, th, sizeof(*e));
			save_p += sizeof(*e);
			e->caller = (mobj_t*)(size_t)e->caller->mobjnum;
			e->line = (line_t*)(e->line - lines);
			continue;
		}
#ifdef PARANOIA
		else if((int)th->function.acp1 != -1) // wait garbage collection
			I_Error("unknown thinker type 0x%X", th->function.acp1);
#endif
	}

	WRITEBYTE(save_p, tc_end);
}

// Now save the pointers, tracer and target, but at load time we must
// relink to this; the savegame contains the old position in the pointer
// field copyed in the info field temporarily, but finally we just search
// for the old position and relink to it.
static mobj_t* FindNewPosition(size_t oldposition)
{
	thinker_t* th;
	mobj_t* mobj;

	for(th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		mobj = (mobj_t*)th;
		if(mobj->mobjnum == (int)oldposition)
			return mobj;
	}
	if(devparm)
		CONS_Printf("\2not found\n");
	DEBFILE("not found\n");
	return NULL;
}

//
// P_NetUnArchiveThinkers
//
static void P_NetUnArchiveThinkers(void)
{
	thinker_t* currentthinker;
	thinker_t* next;
	mobj_t* mobj;
	ULONG diff;
	int i;
	byte tclass;
	boolean restoreNum = false;

	// remove all the current thinkers
	currentthinker = thinkercap.next;
	for(currentthinker = thinkercap.next; currentthinker != &thinkercap; currentthinker = next)
	{
		next = currentthinker->next;

		mobj = (mobj_t*)currentthinker;
		if(currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
			P_RemoveSavegameMobj((mobj_t*)currentthinker); // item isn't saved, don't remove it
		else
			Z_Free(currentthinker);
	}

	// we don't want the removed mobjs to come back
	iquetail = iquehead = 0 ;
	P_InitThinkers();

	// read in saved thinkers
	for(;;)
	{
		tclass = READBYTE(save_p);

		if(tclass == tc_end)
			break; // leave the saved thinker reading loop

		switch(tclass)
		{
			case tc_mobj:
				PADSAVEP();

				diff = READULONG(save_p);
				next = (void*)(size_t)READULONG(save_p);

				mobj = Z_Malloc(sizeof(mobj_t), PU_LEVEL, NULL);
				memset(mobj, 0, sizeof(mobj_t));

				mobj->z = READFIXED(save_p); // Force this so 3dfloor problems don't arise.
				mobj->floorz = READFIXED(save_p); /// \todo floorff too?
				mobj->ceilingz = READFIXED(save_p);
				mobj->floorff = NULL;
				mobj->ceilingff = NULL;

				if(diff & MD_SPAWNPOINT)
				{
					short spawnpointnum = READSHORT(save_p);
					mobj->spawnpoint = &mapthings[spawnpointnum];
					mapthings[spawnpointnum].mobj = mobj;
				}
				if(diff & MD_TYPE)
					mobj->type = READULONG(save_p);
				else
				{
					for(i = 0; i < NUMMOBJTYPES; i++)
						if(mobj->spawnpoint->type == mobjinfo[i].doomednum)
							break;
					if(i == NUMMOBJTYPES)
					{
						CONS_Printf("found mobj with unknown map thing type %d\n",
							mobj->spawnpoint->type);
						I_Error("Savegame corrupted");
					}
					mobj->type = i;
				}
				mobj->info = &mobjinfo[mobj->type];
				if(diff & MD_POS)
				{
					mobj->x = READFIXED(save_p);
					mobj->y = READFIXED(save_p);
					mobj->angle = READANGLE(save_p);
				}
				else
				{
					mobj->x = mobj->spawnpoint->x << FRACBITS;
					mobj->y = mobj->spawnpoint->y << FRACBITS;
					mobj->angle = ANG45 * (mobj->spawnpoint->angle/45); /// \bug unknown
				}
				if(diff & MD_MOM)
				{
					mobj->momx = READFIXED(save_p);
					mobj->momy = READFIXED(save_p);
					mobj->momz = READFIXED(save_p);
				} // otherwise they're zero, and the memset took care of it

				if(diff & MD_RADIUS)
					mobj->radius = READFIXED(save_p);
				else
					mobj->radius = mobj->info->radius;
				if(diff & MD_HEIGHT)
					mobj->height = READFIXED(save_p);
				else
					mobj->height = mobj->info->height;
				if(diff & MD_FLAGS)
					mobj->flags = READLONG(save_p);
				else
					mobj->flags = mobj->info->flags;
				mobj->flags2 = READLONG(save_p);
				if(diff & MD_HEALTH)
					mobj->health = READLONG(save_p);
				else
					mobj->health = mobj->info->spawnhealth;
				if(diff & MD_RTIME)
					mobj->reactiontime = READLONG(save_p);
				else
					mobj->reactiontime = mobj->info->reactiontime;

				if(diff & MD_STATE)
					mobj->state = &states[READUSHORT(save_p)];
				else
					mobj->state = &states[mobj->info->spawnstate];
				if(diff & MD_TICS)
					mobj->tics = READLONG(save_p);
				else
					mobj->tics = mobj->state->tics;
				if(diff & MD_SPRITE)
					mobj->sprite = READUSHORT(save_p);
				else
					mobj->sprite = mobj->state->sprite;
				if(diff & MD_FRAME)
					mobj->frame = READULONG(save_p);
				else
					mobj->frame = mobj->state->frame;
				if(diff & MD_EFLAGS)
					mobj->eflags = READULONG(save_p);
				if(diff & MD_PLAYER)
				{
					i = READBYTE(save_p);
					mobj->player = &players[i];
					mobj->player->mo = mobj;
					// added for angle prediction
					if(consoleplayer == i)
						localangle = mobj->angle;
					if(secondarydisplayplayer == i)
						localangle2 = mobj->angle;
				}
				if(diff & MD_MOVEDIR)
					mobj->movedir = READLONG(save_p);
				if(diff & MD_MOVECOUNT)
					mobj->movecount = READLONG(save_p);
				if(diff & MD_THRESHOLD)
					mobj->threshold = READLONG(save_p);
				if(diff & MD_LASTLOOK)
					mobj->lastlook = READLONG(save_p);
				else
					mobj->lastlook = -1;
				if(diff & MD_TARGET)
					mobj->target = (mobj_t*)(size_t)READULONG(save_p);
				if(diff & MD_TRACER)
					mobj->tracer = (mobj_t*)(size_t)READULONG(save_p);
				if(diff & MD_FRICTION)
					mobj->friction = READLONG(save_p);
				else
					mobj->friction = ORIG_FRICTION;
				if(diff & MD_MOVEFACTOR)
					mobj->movefactor = READLONG(save_p);
				else
					mobj->movefactor = ORIG_FRICTION_FACTOR;
				if(diff & MD_FUSE)
					mobj->fuse = READLONG (save_p);
				if(diff & MD_WATERTOP)
					mobj->watertop = READLONG(save_p);
				if(diff & MD_WATERBOTTOM)
					mobj->waterbottom = READLONG(save_p);

				// now set deductable field
				/// \todo save this too
				mobj->skin = NULL;

				// set sprev, snext, bprev, bnext, subsector
				P_SetThingPosition(mobj);

				// Special case for use of the block and sector links
				if(mobj->type == MT_HOOPCOLLIDE || mobj->type == MT_HOOP)
				{
					size_t value;

					value = READULONG(save_p);
					if(value != 0xFFFFFFFF)
						mobj->bnext = (mobj_t*)value;

					value = READULONG(save_p);
					if(value != 0xFFFFFFFF)
						mobj->bprev = (mobj_t*)value;

					value = READULONG(save_p);
					if(value != 0xFFFFFFFF)
						mobj->snext = (mobj_t*)value;

					value = READULONG(save_p);
					if(value != 0xFFFFFFFF)
						mobj->sprev = (mobj_t*)value;
				}

				mobj->mobjnum = READULONG(save_p);

				if(mobj->player)
					mobj->player->viewz = mobj->player->mo->z + mobj->player->viewheight;

				mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
				P_AddThinker(&mobj->thinker);

				mobj->info = (mobjinfo_t*)next; // temporarily, set when leave this function
				break;

			case tc_ceiling:
				PADSAVEP();
				{
					ceiling_t* ceiling;

					ceiling = Z_Malloc(sizeof(*ceiling), PU_LEVEL, NULL);
					memcpy(ceiling, save_p, sizeof(*ceiling));
					save_p += sizeof(*ceiling);
					ceiling->sector = &sectors[(size_t)ceiling->sector];
					ceiling->sector->ceilingdata = ceiling;

					if(ceiling->thinker.function.acp1)
						ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

					P_AddThinker(&ceiling->thinker);
				}
				break;

			case tc_crushceiling:
				PADSAVEP();
				{
					ceiling_t* ceiling;

					ceiling = Z_Malloc(sizeof(*ceiling), PU_LEVEL, NULL);
					memcpy(ceiling, save_p, sizeof(*ceiling));
					save_p += sizeof(*ceiling);
					ceiling->sector = &sectors[(size_t)ceiling->sector];
					ceiling->sector->ceilingdata = ceiling;

					if(ceiling->thinker.function.acp1)
						ceiling->thinker.function.acp1 = (actionf_p1)T_CrushCeiling;

					P_AddThinker(&ceiling->thinker);
				}
				break;

			case tc_floor:
				PADSAVEP();
				{
					floormove_t* floor;

					floor = Z_Malloc(sizeof(*floor), PU_LEVEL, NULL);
					memcpy(floor, save_p, sizeof(*floor));
					save_p += sizeof(*floor);
					floor->sector = &sectors[(size_t)floor->sector];
					floor->sector->floordata = floor;
					floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
					P_AddThinker(&floor->thinker);
				}
				break;

			case tc_flash:
				PADSAVEP();
				{
					lightflash_t* flash;

					flash = Z_Malloc(sizeof(*flash), PU_LEVEL, NULL);
					memcpy(flash, save_p, sizeof(*flash));
					save_p += sizeof(*flash);
					flash->sector = &sectors[(size_t)flash->sector];
					flash->sector->lightingdata = flash;
					flash->thinker.function.acp1 = (actionf_p1)T_LightningFlash;
					P_AddThinker(&flash->thinker);
				}
				break;

			case tc_strobe:
				PADSAVEP();
				{
					strobe_t* strobe;

					strobe = Z_Malloc(sizeof(*strobe), PU_LEVEL, NULL);
					memcpy(strobe, save_p, sizeof(*strobe));
					save_p += sizeof(*strobe);
					strobe->sector = &sectors[(size_t)strobe->sector];
					strobe->sector->lightingdata = strobe;
					strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
					P_AddThinker(&strobe->thinker);
				}
				break;

			case tc_glow:
				PADSAVEP();
				{
					glow_t* glow;

					glow = Z_Malloc(sizeof(*glow), PU_LEVEL, NULL);
					memcpy(glow, save_p, sizeof(*glow));
					save_p += sizeof(*glow);
					glow->sector = &sectors[(size_t)glow->sector];
					glow->sector->lightingdata = glow;
					glow->thinker.function.acp1 = (actionf_p1)T_Glow;
					P_AddThinker(&glow->thinker);
				}
				break;

			case tc_fireflicker:
				PADSAVEP();
				{
					fireflicker_t* ff;

					ff = Z_Malloc(sizeof(*ff), PU_LEVEL, NULL);
					memcpy(ff, save_p, sizeof(*ff));
					save_p += sizeof(*ff);
					ff->sector = &sectors[(size_t)ff->sector];
					ff->sector->lightingdata = ff;
					ff->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
					P_AddThinker(&ff->thinker);
				}
				break;

			case tc_elevator:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->sector->floordata = elevator;
					elevator->sector->ceilingdata = elevator;
					elevator->thinker.function.acp1 = (actionf_p1)T_MoveElevator;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_continuousfalling:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->sector->floordata = elevator;
					elevator->sector->ceilingdata = elevator;
					elevator->thinker.function.acp1 = (actionf_p1)T_ContinuousFalling;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_thwomp:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->actionsector = &sectors[(size_t)elevator->actionsector];
					elevator->sourceline = &lines[(size_t)elevator->sourceline];
					elevator->sector->floordata = elevator;
					elevator->sector->ceilingdata = elevator;
					elevator->thinker.function.acp1 = (actionf_p1)T_ThwompSector;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_camerascanner:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->actionsector = &sectors[(size_t)elevator->actionsector];
					elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
					P_AddThinker(&elevator->thinker);
				}
				break;

			/// \todo rewrite all the shit that uses an elevator_t but isn't an elevator
			case tc_bouncecheese:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->actionsector = &sectors[(size_t)elevator->actionsector];
					elevator->thinker.function.acp1 = (actionf_p1)T_BounceCheese;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_startcrumble:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->actionsector = &sectors[(size_t)elevator->actionsector];
					elevator->sector->floordata = elevator;
					elevator->thinker.function.acp1 = (actionf_p1)T_StartCrumble;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_airbob:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->sector->ceilingdata = elevator;
					elevator->player = &players[(size_t)elevator->player];
					elevator->thinker.function.acp1 = (actionf_p1)T_AirBob;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_marioblock:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->sector->floordata = elevator;
					elevator->sector->ceilingdata = elevator;
					elevator->thinker.function.acp1 = (actionf_p1)T_MarioBlock;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_spikesector:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->thinker.function.acp1 = (actionf_p1)T_SpikeSector;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_floatsector:
				PADSAVEP();
				{
					elevator_t* elevator;

					elevator = Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
					memcpy(elevator, save_p, sizeof(elevator_t));
					save_p += sizeof(elevator_t);
					elevator->sector = &sectors[(size_t)elevator->sector];
					elevator->actionsector = &sectors[(size_t)elevator->actionsector];
					elevator->thinker.function.acp1 = (actionf_p1)T_FloatSector;
					P_AddThinker(&elevator->thinker);
				}
				break;

			case tc_laserflash:
				PADSAVEP();
				{
					laserthink_t* laser;

					laser = Z_Malloc(sizeof(laserthink_t), PU_LEVEL, NULL);
					memcpy(laser, save_p, sizeof(laserthink_t));
					save_p += sizeof(laserthink_t);
					laser->sector = &sectors[(size_t)laser->sector];
					laser->ffloor = &(laser->sector->ffloors)[(size_t)laser->ffloor];
					laser->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
					P_AddThinker(&laser->thinker);
				}
				break;

			case tc_lightfade:
				PADSAVEP();
				{
					lightlevel_t* ll;
					ll = Z_Malloc(sizeof(lightlevel_t), PU_LEVEL, NULL);
					memcpy(ll, save_p, sizeof(lightlevel_t));
					save_p += sizeof(lightlevel_t);
					ll->sector = &sectors[(size_t)ll->sector];
					ll->sector->lightingdata = ll;
					ll->thinker.function.acp1 = (actionf_p1)T_LightFade;
					P_AddThinker(&ll->thinker);
				}
				break;

			case tc_executor:
				PADSAVEP();
				{
					executor_t* e;
					e = Z_Malloc(sizeof(executor_t), PU_LEVEL, NULL);
					memcpy(e, save_p, sizeof(executor_t));
					save_p += sizeof(executor_t);
					e->line = &lines[(size_t)e->line];
					restoreNum = true;
//					e->caller = FindNewPosition((size_t)e->caller);
					e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
					P_AddThinker(&e->thinker);
				}
				break;

			case tc_scroll:
				{
					scroll_t* scroll;

					scroll = Z_Malloc(sizeof(scroll_t), PU_LEVEL, NULL);
					memcpy(scroll, save_p, sizeof(scroll_t));
					save_p += sizeof(scroll_t);
					scroll->thinker.function.acp1 = (actionf_p1)T_Scroll;
					P_AddThinker(&scroll->thinker);
				}
				break;

			case tc_friction:
				{
					friction_t* friction;

					friction = Z_Malloc(sizeof(friction_t), PU_LEVEL, NULL);
					memcpy(friction, save_p, sizeof(friction_t));
					save_p += sizeof(friction_t);
					friction->thinker.function.acp1 = (actionf_p1)T_Friction;
					P_AddThinker(&friction->thinker);
				}
				break;

			case tc_pusher:
				{
					pusher_t* pusher;

					pusher = Z_Malloc(sizeof(pusher_t), PU_LEVEL, NULL);
					memcpy(pusher, save_p, sizeof(pusher_t));
					save_p += sizeof(pusher_t);
					pusher->thinker.function.acp1 = (actionf_p1)T_Pusher;
					pusher->source = P_GetPushThing(pusher->affectee);
					P_AddThinker(&pusher->thinker);
				}
				break;

			default:
				I_Error("P_UnarchiveSpecials: Unknown tclass %i in savegame", tclass);
		}
	}

	if(restoreNum)
	{
		for(currentthinker = thinkercap.next; currentthinker != &thinkercap;
			currentthinker = currentthinker->next)
		{
			if(currentthinker->function.acp1 == (actionf_p1)T_ExecutorDelay)
			{
				((executor_t*)currentthinker)->caller = FindNewPosition((size_t)((executor_t*)currentthinker)->caller);
			}
		}
	}
}

//
// P_FinishMobjs
//
static void P_FinishMobjs()
{
	thinker_t* currentthinker;
	mobj_t* mobj;

	// put info field there real value
	for(currentthinker = thinkercap.next; currentthinker != &thinkercap;
		currentthinker = currentthinker->next)
	{
		if(currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			mobj = (mobj_t*)currentthinker;
			mobj->info = &mobjinfo[mobj->type];
		}
	}
}

static void P_RelinkPointers(void)
{
	thinker_t* currentthinker;
	mobj_t* mobj;

	// use info field (value = oldposition) to relink mobjs
	for(currentthinker = thinkercap.next; currentthinker != &thinkercap;
		currentthinker = currentthinker->next)
	{
		if(currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			mobj = (mobj_t*)currentthinker;
			if(mobj->tracer)
			{
				mobj->tracer = FindNewPosition((size_t)mobj->tracer);
				if(!mobj->tracer)
					CONS_Printf("tracer not found on %d\n", mobj->type);
			}
			if(mobj->target)
			{
				mobj->target = FindNewPosition((size_t)mobj->target);
				if(!mobj->target)
					CONS_Printf("target not found on %d\n", mobj->type);
			}
			if(mobj->type == MT_HOOPCOLLIDE || mobj->type == MT_HOOP)
			{
				if((size_t)mobj->bnext != (size_t)0xFFFFFFFF)
					mobj->bnext = FindNewPosition((size_t)mobj->bnext);
				else
					mobj->bnext = NULL;

				if((size_t)mobj->bprev != (size_t)0xFFFFFFFF)
					mobj->bprev = FindNewPosition((size_t)mobj->bprev);
				else
					mobj->bprev = NULL;

				if((size_t)mobj->snext != (size_t)0xFFFFFFFF)
					mobj->snext = FindNewPosition((size_t)mobj->snext);
				else
					mobj->snext = NULL;

				if((size_t)mobj->sprev != (size_t)0xFFFFFFFF)
					mobj->sprev = FindNewPosition((size_t)mobj->sprev);
				else
					mobj->sprev = NULL;

				if(!mobj->bnext)
					CONS_Printf("bnext not found on %d\n", mobj->type);

				if(!mobj->bprev)
					CONS_Printf("bprev not found on %d\n", mobj->type);

				if(!mobj->snext)
					CONS_Printf("snext not found on %d\n", mobj->type);

				if(!mobj->sprev)
					CONS_Printf("sprev not found on %d\n", mobj->type);
			}
			if(mobj->player && mobj->player->lastaxis)
			{
				mobj->player->lastaxis = FindNewPosition((size_t)mobj->player->lastaxis);
				if(!mobj->player->lastaxis)
					CONS_Printf("lastaxis not found on %d\n", mobj->type);
			}
			if(mobj->player && mobj->player->capsule)
			{
				mobj->player->capsule = FindNewPosition((size_t)mobj->player->capsule);
				if(!mobj->player->capsule)
					CONS_Printf("capsule not found on %d\n", mobj->type);
			}
			if(mobj->player && mobj->player->awayviewmobj)
			{
				mobj->player->awayviewmobj = FindNewPosition((size_t)mobj->player->awayviewmobj);
				if(!mobj->player->awayviewmobj)
					CONS_Printf("awayviewmobj not found on %d\n", mobj->type);
			}
		}
	}
}

//
// P_NetArchiveSpecials
//
static void P_NetArchiveSpecials(void)
{
	int i, z;

	// itemrespawn queue for deathmatch
	i = iquetail;
	while(iquehead != i)
	{
		for(z = 0; z < nummapthings; z++)
		{
			if(&mapthings[z] == itemrespawnque[i])
			{
				WRITELONG(save_p, z);
				break;
			}
		}
		WRITELONG(save_p, itemrespawntime[i]);
		i = (i + 1) & (ITEMQUESIZE-1);
	}

	// end delimiter
	WRITELONG(save_p, 0xffffffff);

	// Sky number
	WRITELONG(save_p, globallevelskynum);

	// Current global weather type
	WRITEBYTE(save_p, globalweather);
}

//
// P_NetUnArchiveSpecials
//
static void P_NetUnArchiveSpecials(void)
{
	int i;

	// BP: added save itemrespawn queue for deathmatch
	iquetail = iquehead = 0 ;
	while((i = READLONG(save_p)) != (int)0xffffffff)
	{
		itemrespawnque[iquehead] = &mapthings[i];
		itemrespawntime[iquehead++] = READLONG(save_p);
	}

	i = READLONG(save_p);
	if(i != globallevelskynum)
	{
		globallevelskynum = levelskynum = i;
		P_SetupLevelSky(levelskynum);
	}

	globalweather = READBYTE(save_p);

	switch(globalweather)
	{
		case 1:
			cv_storm.value = 0;
			P_SwitchWeather(globalweather);
			break;
		case 2:
			cv_snow.value = 0;
			P_SwitchWeather(globalweather);
			break;
		case 3:
			cv_rain.value = 0;
			P_SwitchWeather(globalweather);
			break;
		default: // 0
			if((cv_snow.value || cv_rain.value || cv_storm.value))
				P_SwitchWeather(globalweather);
			break;
	}
}

// =======================================================================
//          Misc
// =======================================================================
static void P_ArchiveMisc()
{
	ULONG pig = 0;
	int i;

	WRITEBYTE(save_p, gameskill);
	WRITESHORT(save_p, gamemap);

	WRITEULONG(save_p, tokenlist);
	WRITEULONG (save_p, token);

	WRITEUSHORT(save_p, emeralds+357);
	WRITEBYTE(save_p, modifiedgame);
	WRITEBYTE(save_p, savemoddata);

	for(i = 0; i < MAXPLAYERS; i++)
		pig |= (playeringame[i] != 0)<<i;

	WRITEULONG(save_p, pig);
}

static boolean P_UnArchiveMisc()
{
	ULONG pig;
	int i;

	gameskill = READBYTE(save_p);
	gamemap = READSHORT(save_p);

	tokenlist = READULONG(save_p);
	token = READULONG(save_p);

	emeralds = (USHORT)(READUSHORT(save_p)-357);

	i = READBYTE(save_p);

	if(i && !modifiedgame)
		modifiedgame = i;

	i = READBYTE(save_p);

	if(!i && savemoddata)
		savemoddata = i;

	pig = READULONG(save_p);

	for(i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = (pig & (1<<i)) != 0;
		players[i].playerstate = PST_REBORN;
	}

	M_ClearRandom();

	levelstarttic = gametic; // for time calculation

	if(wipegamestate == GS_LEVEL)
		wipegamestate = -1; // force a wipe

	gamestate = GS_LEVEL;

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i])
		{
			players[i].starposttime = players[i].starpostx = players[i].starposty = 0;
			players[i].starpostz = players[i].starpostnum = 0;
			players[i].starpostangle = players[i].starpostbit = 0;
		}

	if(!P_SetupLevel(gamemap, gameskill, NULL))
		return false;

	gameaction = ga_nothing;

	return true;
}

static void P_NetArchiveMisc()
{
	ULONG pig = 0;
	int i;

	WRITEBYTE(save_p, gameskill);
	WRITESHORT(save_p, gamemap);

	WRITEULONG(save_p, tokenlist);

	for(i = 0; i < MAXPLAYERS; i++)
		pig |= (playeringame[i] != 0)<<i;

	WRITEULONG(save_p, pig);

	WRITEULONG(save_p, leveltime);
	WRITEULONG(save_p, totalrings);
	WRITEULONG(save_p, lastmap);

	WRITEUSHORT(save_p, emeralds);

	WRITEULONG(save_p, token);
	WRITEULONG(save_p, sstimer);
	WRITEULONG(save_p, bluescore);
	WRITEULONG(save_p, redscore);

	WRITEULONG(save_p, countdown);
	WRITEULONG(save_p, countdown2);

	WRITEFIXED(save_p, gravity);

	WRITEULONG(save_p, countdowntimer);
	WRITEBYTE(save_p, countdowntimeup);

	WRITEBYTE(save_p, P_GetRandIndex());
}

static boolean P_NetUnArchiveMisc()
{
	ULONG pig;
	int i;

	gameskill = READBYTE(save_p);
	gamemap = READSHORT(save_p);

	tokenlist = READULONG(save_p);

	pig = READULONG(save_p);

	for(i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = (pig & (1<<i)) != 0;
		players[i].playerstate = PST_REBORN;
	}

	if(!P_SetupLevel(gamemap, gameskill, "\2"))
		return false;
	// the "\2" instead of NULL is a hackishly hackish hack to avoid loading precipitation

	// get the time
	leveltime = READULONG(save_p);
	totalrings = READULONG(save_p);
	lastmap = READULONG(save_p);

	emeralds = READUSHORT(save_p);

	token = READULONG(save_p);
	sstimer = READULONG(save_p);
	bluescore = READULONG(save_p);
	redscore = READULONG(save_p);

	countdown = READULONG(save_p);
	countdown2 = READULONG(save_p);

	gravity = READFIXED(save_p);

	countdowntimer = READULONG(save_p);
	countdowntimeup = READBYTE(save_p);

	P_SetRandIndex(READBYTE(save_p));

	return true;
}

void P_SaveGame(void)
{
	P_ArchiveMisc();
	P_ArchivePlayers();

	WRITEBYTE(save_p, 0x1d); // consistency marker
}

void P_SaveNetGame(void)
{

	CV_SaveNetVars((char**)&save_p);
	P_NetArchiveMisc();
	P_NetArchivePlayers();
	P_NetArchiveWorld();
	P_NetArchiveThinkers();
	P_NetArchiveSpecials();

	// Is it paused?
	/// \todo can we archive this in misc?
	if(paused)
		WRITEBYTE(save_p, 0x2f);
	else
		WRITEBYTE(save_p, 0x2e);

	WRITEBYTE(save_p, 0x1d); // consistency marker
}

boolean P_LoadGame(void)
{
	int i;

	if(gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = GS_NULL; // should be changed in P_UnArchiveMisc

	if(!P_UnArchiveMisc())
		return false;
	P_UnArchivePlayers();
	P_FinishMobjs();

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i])
			SetSavedSkin(i, players[i].skin, players[i].skincolor);

	return READBYTE(save_p) == 0x1d;
}

boolean P_LoadNetGame(void)
{

	CV_LoadNetVars((char**)&save_p);
	if(!P_NetUnArchiveMisc())
		return false;
	P_NetUnArchivePlayers();
	P_NetUnArchiveWorld();
	P_NetUnArchiveThinkers();
	P_NetUnArchiveSpecials();
	P_RelinkPointers();
	P_FinishMobjs();

	// The precipitation would normally be spawned in P_SetupLevel, which is called by
	// P_NetUnArchiveMisc above. However, that would place it up before P_NetUnArchiveThinkers,
	// so the thinkers would be deleted later. Therefore, P_SetupLevel will *not* spawn
	// precipitation when loading a netgame save. Instead, precip has to be spawned here.
	// This is done in P_NetUnArchiveSpecials now.

	// Is it paused?
	if(READBYTE(save_p) == 0x2f)
		paused = true;

	return READBYTE(save_p) == 0x1d;
}
