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
/// \brief player data structures

#ifndef __D_PLAYER__
#define __D_PLAYER__

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

//
// Player states.
//
typedef enum
{
	// Playing or camping.
	PST_LIVE,
	// Dead on the ground, view follows killer.
	PST_DEAD,
	// Ready to restart/respawn???
	PST_REBORN
} playerstate_t;

//
// Player internal flags, for cheats and debug.
//
typedef enum
{
	// No clipping, walk through barriers.
	CF_NOCLIP = 1,
	// No damage, no health loss.
	CF_GODMODE = 2,
	// Not really a cheat, just a debug aid.
	CF_NOMOMENTUM = 4,

	CF_FLYAROUND = 8
} cheat_t;

// Player powers. (don't edit this comment)
typedef enum
{
	pw_invulnerability,
	pw_sneakers,
	pw_flashing,
	pw_jumpshield, // jump shield
	pw_fireshield, // fire shield
	pw_tailsfly, // tails flying
	pw_underwater, // underwater timer
	pw_spacetime, // In space, no one can hear you spin!
	pw_extralife, // Extra Life timer
	pw_ringshield, // ring shield
	pw_bombshield, // bomb shield
	pw_watershield, // water shield
	pw_super, // Are you super?

	// Mario-specific
	pw_fireflower,

	// New Multiplayer Weapons
	pw_homingring,
	pw_railring,
	pw_infinityring,
	pw_automaticring,
	pw_explosionring,

	// NiGHTS powerups
	pw_superparaloop,
	pw_nightshelper,

	NUMPOWERS
} powertype_t;

// ========================================================================
//                          PLAYER STRUCTURE
// ========================================================================
typedef struct player_s
{
	mobj_t* mo;

#ifdef CLIENTPREDICTION2
	mobj_t* spirit; // for movement prediction
#endif
	playerstate_t playerstate;
	ticcmd_t cmd;

	// Determine POV, including viewpoint bobbing during movement.
	// Focal origin above r.z
	fixed_t viewz;
	// Base height above floor for viewz.
	fixed_t viewheight;
	// Bob/squat speed.
	fixed_t deltaviewheight;
	// bounded/scaled total momentum.
	fixed_t bob;

	// Mouse aiming, where the guy is looking at!
	// It is updated with cmd->aiming.
	angle_t aiming;

	angle_t awayviewaiming; // Used for cut-away view

	// This is only used between levels,
	// mo->health is used during levels.
	int health;

	// Power ups. invinc and invis are tic counters.
	int powers[NUMPOWERS];

	boolean autoaim_toggle;

	// True if button down last tic.
	boolean attackdown;
	boolean usedown;
	boolean jumpdown; // don't jump like a monkey!

	// Bit flags, for cheats and debug.
	// See cheat_t, above.
	cheat_t cheats;

	// Hint messages.
	const char* message;

	// For screen flashing (red or bright).
	int bonuscount;

	int specialsector; // lava/slime/water...

	// Player skin colorshift, 0-15 for which color to draw player.
	int skincolor;

	int skin;

	int score; // player score
	int dashspeed; // dashing speed

	int normalspeed; // Normal ground

	int thrustfactor; // Thrust = thrustfactor * acceleration
	int accelstart; // Starting acceleration if speed = 0.
	int acceleration; // Acceleration

	int charability; // Ability definition
	int charspin; // Is the player allowed to spin/spindash?
	int jumpfactor; // How high can the player jump?

	int boxindex; // Life icon index for 1up box (See A_1upThinker in p_enemy.c)

	int starttranscolor; // Start position for the changeable color of a skin
	int endtranscolor; // End position for the changeable color of a skin

	int prefcolor; // forced color in single player, default in multi

	int lives;
	int continues; // continues that player has acquired

	int superready; // Ready for Super?

	int xtralife; // Ring Extra Life counter

	int walking; // Are the walking frames playing?
	int running; // Are the running frames playing?
	int spinning; // Are the spinning frames playing?
	int speed; // Player's speed (distance formula of MOMX and MOMY values)
	int jumping; // Jump counter

	// Moved eflags to player ints
	int mfjumped;
	int mfspinning;
	int mfstartdash;

	int fly1; // Tails flying
	int scoreadd; // Used for multiple enemy attack bonus
	int gliding; // Are you gliding?
	int glidetime; // Glide counter for thrust
	int climbing; // Climbing on the wall
	int deadtimer; // End game if game over lasts too long
	int splish; // Don't make splish repeat tons
	int exiting; // Exitlevel timer
	int blackow;

	boolean homing; // Are you homing?

	////////////////////////////
	// Conveyor Belt Movement //
	////////////////////////////
	fixed_t cmomx; // Conveyor momx
	fixed_t cmomy; // Conveyor momy
	fixed_t rmomx; // "Real" momx (momx - cmomx)
	fixed_t rmomy; // "Real" momy (momy - cmomy)

	/////////////////////
	// Race Mode Stuff //
	/////////////////////
	int numboxes; // Number of item boxes obtained for Race Mode
	int totalring; // Total number of rings obtained for Race Mode
	int realtime; // integer replacement for leveltime
	int racescore; // Total of won categories

	////////////////////
	// Tag Mode Stuff //
	////////////////////
	int tagit; // The player is it! For Tag Mode
	int tagcount; // Number of tags player has avoided
	int tagzone; // Tag Zone timer
	int taglag; // Don't go back in the tag zone too early

	////////////////////
	// CTF Mode Stuff //
	////////////////////
	int ctfteam; // 1 == Red, 2 == Blue
	unsigned short gotflag; // 1 == Red, 2 == Blue Do you have the flag?

	int dbginfo; // Debugger
	int emeraldhunt; // # of emeralds found
	boolean snowbuster; // Snow Buster upgrade!
	int bustercount; // Charge for Snow Buster

	int weapondelay; // Delay (if any) to fire the weapon again
	int taunttimer; // Delay before you can use the taunt again

	// Starpost information
	int starpostx;
	int starposty;
	int starpostz;
	int starpostnum; // The number of the last starpost you hit
	int starposttime; // Your time when you hit the starpost
	int starpostangle; // Angle that the starpost is facing - you respawn facing this way
	unsigned int starpostbit; // List of starposts hit

	double angle_speed; // Speed for NiGHTS!
	double angle_pos;
	double old_angle_pos;
	boolean nightsmode; // Is the player in NiGHTS mode?

	boolean axishit;
	boolean axistransferred;
	boolean transfertoclosest;
	int flyangle;
	int drilltimer;
	int linkcount;
	int linktimer;
	int anotherflyangle;
	int transferangle;
	int nightstime; // How long you can fly as NiGHTS.
	boolean nightsfall; // Spill rings after falling
	mobj_t* lastaxis;
	int drillmeter;
	byte drilldelay;
	boolean drilling;
	boolean skiddown;
	boolean bonustime; // Capsule destroyed, now it's bonus time!
	mobj_t* capsule; // Go inside the capsule
	byte mare; // Current mare

	short lastsidehit, lastlinehit;

	boolean carried; // Tails pickup!

	int losscount; // # of times you've lost only 1 ring

	mobjtype_t currentthing; // For the object placement mode

	boolean lightdash; // Experimental fun thing
	boolean lightdashallowed;

	boolean thokked; // You already thokked on this jump.

	int onconveyor; // You are on a conveyor belt if nonzero

	mobj_t* awayviewmobj;
	int awayviewtics;
} player_t;

#endif
