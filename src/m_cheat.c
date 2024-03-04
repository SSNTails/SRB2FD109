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
/// \brief Cheat sequence checking

#include "doomdef.h"
#include "dstrings.h"

#include "am_map.h"
#include "m_cheat.h"
#include "g_game.h"

#include "r_local.h"
#include "p_local.h"

#include "m_cheat.h"

#include "i_sound.h" // for I_PlayCD()
#include "s_sound.h"
#include "v_video.h"
#include "st_stuff.h"
#include "w_wad.h"

// ==========================================================================
//                             CHEAT Structures
// ==========================================================================

static byte cheat_cd_seq[] =
{
	0xb2, 0x26, 0xe2, 0x26, 1, 0, 0, 0xff
};

static byte cheat_god_seq[] =
{
	SCRAMBLE('h'), SCRAMBLE('e'), SCRAMBLE('l'), SCRAMBLE('e'), SCRAMBLE('n'), 0xff // HELEN - Johnny's love! Tails
};

static unsigned char cheat_ammo_seq[] = // Demo
{
	SCRAMBLE('f'), SCRAMBLE('i'), SCRAMBLE('s'), SCRAMBLE('h'), SCRAMBLE('b'), SCRAMBLE('a'), SCRAMBLE('k'), SCRAMBLE('e'), 0xff
};

//
static byte cheat_commercial_noclip_seq[] =
{
	SCRAMBLE('d'), SCRAMBLE('e'), SCRAMBLE('v'), SCRAMBLE('m'), SCRAMBLE('o'), SCRAMBLE('d'), SCRAMBLE('e'), 0xff // id...
};

static byte cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

// Now what?
static cheatseq_t cheat_cd = { cheat_cd_seq, 0 };
static cheatseq_t cheat_god = { cheat_god_seq, 0 };
static cheatseq_t cheat_ammo = { cheat_ammo_seq, 0 };
static cheatseq_t cheat_commercial_noclip = { cheat_commercial_noclip_seq, 0 };

// ==========================================================================
//                        CHEAT SEQUENCE PACKAGE
// ==========================================================================

static byte cheat_xlate_table[256];

void cht_Init(void)
{
	int i;
	for(i = 0; i < 256; i++)
		cheat_xlate_table[i] = (byte)SCRAMBLE((byte)i);
}

//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int cht_CheckCheat(cheatseq_t* cht, char key)
{
	int rc = 0;

	if(!cht->p)
		cht->p = cht->sequence; // initialize if first time

	if(*cht->p == 0)
		*(cht->p++) = key;
	else if(cheat_xlate_table[(byte)key] == *cht->p)
		cht->p++;
	else
		cht->p = cht->sequence;

	if(*cht->p == 1)
		cht->p++;
	else if(*cht->p == 0xff) // end of sequence character
	{
		cht->p = cht->sequence;
		rc = 1;
	}

	return rc;
}

void cht_GetParam(cheatseq_t* cht, char* buffer)
{
	byte* p;
	byte c;

	p = cht->sequence;
	while(*(p++) != 1)
		;

	do
	{
		c = *p;
		*(buffer++) = c;
		*(p++) = 0;
	} while(c && *p != 0xff);

	if(*p == 0xff)
		*buffer = 0;
}

boolean cht_Responder(event_t* ev)
{
	static player_t *plyr;
	const char* msg = NULL;

	if(ev->type == ev_keydown)
	{

		plyr = &players[consoleplayer];
		// b. - enabled for more debug fun.

		if(cht_CheckCheat(&cheat_amap, (char)ev->data1))
			am_cheating = (am_cheating+1) % 3;
		// 'dqd' cheat for toggleable god mode
		else if(cht_CheckCheat(&cheat_god, (char)ev->data1))
		{
			if(!modifiedgame || savemoddata)
			{
				modifiedgame = true;
				savemoddata = false;
				if(!(netgame || multiplayer))
					CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
			}

			plyr->cheats ^= CF_GODMODE;
			if(plyr->cheats & CF_GODMODE)
				msg = "Smells bad!\n";
			else
				msg = "...smells REALLY bad!\n";
		}

		// 'cd' for changing cd track quickly
		// NOTE: the cheat uses the REAL track numbers, not remapped ones
		else if(cht_CheckCheat(&cheat_cd, (char)ev->data1))
		{
			char buf[3];

			cht_GetParam(&cheat_cd, buf);

			CONS_Printf("Changing cd track...\n");
			I_PlayCD((buf[0]-'0')*10 + (buf[1]-'0'), true);
		}

		// Simplified, accepting both "noclip" and "idspispopd".
		// no clipping mode cheat
		else if(cht_CheckCheat(&cheat_commercial_noclip, (char)ev->data1))
		{
			Command_Devmode_f();
		}

		else if(cht_CheckCheat(&cheat_ammo, (char)ev->data1))
		{
			Command_Resetemeralds_f();
		}

		// append a newline to the original doom messages
		if(msg)
			CONS_Printf("%s\n", msg);
	}
	return false;
}

// command that can be typed at the console!

void Command_CheatNoClip_f(void)
{
	player_t* plyr;
	if(multiplayer)
		return;

	plyr = &players[consoleplayer];
	plyr->cheats ^= CF_NOCLIP;
	CONS_Printf("No Clipping %s\n", plyr->cheats & CF_NOCLIP ? "On" : "Off");

	if(!modifiedgame || savemoddata)
	{
		modifiedgame = true;
		savemoddata = false;
		if(!(netgame || multiplayer))
			CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
	}
}

void Command_CheatGod_f(void)
{
	player_t* plyr;

	if(multiplayer)
		return;

	plyr = &players[consoleplayer];
	plyr->cheats ^= CF_GODMODE;
	CONS_Printf("Sissy Mode %s\n", plyr->cheats & CF_GODMODE ? "On" : "Off");

	if(!modifiedgame || savemoddata)
	{
		modifiedgame = true;
		savemoddata = false;
		if(!(netgame || multiplayer))
			CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
	}
}

void Command_Resetemeralds_f(void)
{
	if(netgame || multiplayer)
	{
		CONS_Printf("This only works in single player.\n");
		return;
	}

	emeralds = 0;

	CONS_Printf("Emeralds reset to zero.\n");
}

void Command_Devmode_f(void)
{
	if(netgame || multiplayer)
		return;

	if(!cv_debug)
		cv_debug = true;
	else
		cv_debug = false;

	if(!modifiedgame || savemoddata)
	{
		modifiedgame = true;
		savemoddata = false;
		if(!(netgame || multiplayer))
			CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
	}
}
