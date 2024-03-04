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
/// \brief Button/action code definitions, ticcmd_t

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "m_fixed.h"
#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

// Button/action code definitions.
typedef enum
{
	BT_ATTACK = 1, // shoot rings
	BT_USE = 2, // spin
	BT_TAUNT = 4,
	BT_CAMLEFT = 8, // turn camera left
	BT_CAMRIGHT = 16, // turn camera right
	BT_LIGHTDASH = 32,
	BT_JUMP = 64,
	BT_FIRENORMAL = 128 // Fire a normal ring no matter what
} buttoncode_t;


// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

// bits in angleturn
#define TICCMD_RECEIVED 1
#define TICCMD_XY 2
#define BT_FLYDOWN 4
typedef struct
{
#ifdef CLIENTPREDICTION2
	fixed_t x, y;
#endif
	signed char forwardmove; // *2048 for move
	signed char sidemove; // *2048 for move
	short angleturn; // <<16 for angle delta - SAVED AS A BYTE into demos
	signed short aiming; // mouse aiming, see G_BuildTicCmd
	byte buttons;
} ticcmd_t;

#endif
