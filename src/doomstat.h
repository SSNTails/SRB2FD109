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
/// \brief All the global variables that store the internal state.
/// 
///	Theoretically speaking, the internal state of the engine
///	should be found by looking at the variables collected
///	here, and every relevant module will have to include
///	this header file.
///	In practice, things are a bit messy.

#ifndef __DOOMSTAT__
#define __DOOMSTAT__

// We need globally shared data structures, for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"

// =============================
// Selected skill type, map etc.
// =============================

// Selected by user.
extern skill_t gameskill;
extern short gamemap;
extern short mapmusic;
extern short maptol;
extern int globalweather;

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;
extern boolean savemoddata; // This mod saves time/emblem data.

// Netgame? only true in a netgame
extern boolean netgame;
extern boolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern boolean multiplayer;

extern int gametype;
extern boolean cv_debug;

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean nomusic; // defined in d_main.c
extern boolean nosound;
extern boolean nofmod;
extern boolean music_disabled;
extern boolean sound_disabled;
extern boolean digital_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern boolean menuactive; // Menu overlaid?
extern boolean paused; // Game paused?

extern boolean nodrawers;
extern boolean noblit;

extern int viewwindowx, viewwindowy;
extern int viewwidth, scaledviewwidth;

extern boolean gamedataloaded;

// Player taking events, and displaying.
extern int consoleplayer;
extern int displayplayer;
extern int secondarydisplayplayer; // for splitscreen

// Maps of special importance
extern int spstage_start;
extern int sstage_start;
extern int sstage_end;
extern int racestage_start;

extern tic_t countdowntimer;
extern boolean countdowntimeup;

typedef struct
{
	byte numpics;
	char picname[8][8];
	boolean pichires[8];
	char* text;
	unsigned short xcoord[8];
	unsigned short ycoord[8];
	unsigned short picduration[8];
	short musicslot;
	unsigned short textxpos;
	unsigned short textypos;
} scene_t;

typedef struct
{
	scene_t scene[128]; // 128 scenes per cutscene.
	int numscenes; // Number of scenes in this cutscene
} cutscene_t;

extern cutscene_t cutscenes[128];

// For the Custom Exit linedef.
extern int nextmapoverride, nextmapgametype;
extern boolean skipstats;

extern int totalitems;
extern int totalrings; //  Total # of rings in a level

// Fun extra stuff
extern int lastmap; // Last level you were at (returning from special stages).
extern mapthing_t* rflagpoint; // Original flag spawn location
extern mapthing_t* bflagpoint; // Original flag spawn location
#define MF_REDFLAG 1
#define MF_BLUEFLAG 2

#define LEVELARRAYSIZE 1035+2
extern char lvltable[LEVELARRAYSIZE+3][64];

extern boolean twodlevel;

/** Map header information.
  */
typedef struct
{
	// The original eight.
	char lvlttl[33];      ///< Level name without "Zone".
	byte actnum;          ///< Act number or 0 for none.
	short typeoflevel;    ///< Combination of typeoflevel flags.
	short nextlevel;      ///< Map number of next level, or 1100-1102 to end.
	short musicslot;      ///< Music slot number to play. 0 for no music.
	byte forcecharacter;  ///< Skin number to switch to or 255 to disable.
	byte weather;         ///< 0 = sunny day, 1 = storm, 2 = snow, 3 = rain.
	short skynum;         ///< Sky number to use.

	// Extra information.
	char interscreen[8];  ///< 320x200 patch to display at intermission.
	char scriptname[192]; ///< Script to use when the map is switched to.
	boolean scriptislump; ///< True if the script is a lump, not a file.
	byte precutscenenum;  ///< Cutscene number to play BEFORE a level starts.
	byte cutscenenum;     ///< Cutscene number to use, 0 for none.
	short countdown;      ///< Countdown until level end?
	boolean nozone;       ///< True to hide "Zone" in level name.
	boolean hideinmenu;   ///< True to hide in the multiplayer menu.
	boolean nossmusic;    ///< True to disable Super Sonic music in this level.
} mapheader_t;

extern mapheader_t mapheaderinfo[NUMMAPS];

#define TOL_COOP        1 ///< Cooperative
#define TOL_RACE        2 ///< Race
#define TOL_MATCH       4 ///< Match
#define TOL_TAG         8 ///< Tag
#define TOL_CTF        16 ///< Capture the Flag
#define TOL_CHAOS      32 ///< Chaos
#define TOL_NIGHTS     64 ///< NiGHTS
#define TOL_ADVENTURE 128 ///< Adventure
#define TOL_MARIO     256 ///< Mario
#define TOL_2D        512 ///< 2D
#define TOL_XMAS     1024 ///< Xmas
#define TOL_SP       4096 ///< Single Player

// Gametypes
#define GT_COOP  0 // also used in single player
#define GT_MATCH 1
#define GT_RACE  2
#define GT_TAG   3
#define GT_CTF   4 // capture the flag
#define GT_CHAOS 5
#define NUMGAMETYPES 5
// If you alter this list, update gametype_cons_t in m_menu.c

// Emeralds stored as bits to throw savegame hackers off.
extern unsigned short emeralds;
extern    tic_t    totalplaytime;
#define EMERALD1 1
#define EMERALD2 2
#define EMERALD3 4
#define EMERALD4 8
#define EMERALD5 16
#define EMERALD6 32
#define EMERALD7 64
#define EMERALD8 256
#define ALL7EMERALDS ((emeralds & (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7)) == (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7))

#define MAXEMBLEMS 512 // If you have more emblems than this in your game, you seriously need to get a life.
extern int numemblems;

#define NUMEGGS 12
extern int foundeggs;

/** Hidden emblem/egg structure.
  */
typedef struct
{
	signed short x; ///< X coordinate.
	signed short y; ///< Y coordinate.
	signed short z; ///< Z coordinate.
	byte player;    ///< Player who can access this emblem.
	signed short level;     ///< Level on which this emblem/egg can be found.
	boolean collected; ///< Do you have this emblem?
} emblem_t;

extern emblem_t emblemlocations[MAXEMBLEMS];

extern emblem_t egglocations[NUMEGGS]; // Easter eggs... literally!

/** Time attack information, currently a very small structure.
  */
typedef struct
{
	tic_t time; ///< Time in which the level was finished.
} timeattack_t;

extern timeattack_t timedata[NUMMAPS];
extern boolean mapvisited[NUMMAPS];

extern int token; ///< Number of tokens collected in a level
extern int tokenlist; ///< List of tokens collected
extern int tokenbits; ///< Used for setting token bits
extern int sstimer; ///< Time allotted in the special stage
extern int bluescore; ///< Blue Team Scores
extern int redscore;  ///< Red Team Scores

// Eliminates unnecessary searching.
extern boolean CheckForAirBob;
extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;

// Powerup durations
extern int invulntics;
extern int sneakertics;
extern int flashingtics;
extern int tailsflytics;
extern int underwatertics;
extern int spacetimetics;
extern int extralifetics;
// NiGHTS Powerups
extern int paralooptics;
extern int helpertics;

extern byte introtoplay;

// Emerald hunt
extern mobj_t* hunt1;
extern mobj_t* hunt2;
extern mobj_t* hunt3;

// For racing
extern int countdown;
extern int countdown2;

extern fixed_t gravity;

// Grading
// 0 = No grade
// 1 = F
// 2 = E
// 3 = D
// 4 = C
// 5 = B
// 6 = A
// 7 = A+
extern int grade;

extern boolean veryhardcleared; // Ultimate skill available?

extern int timesbeaten; // # of times the game has been beaten.

// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern tic_t gametic;
#ifdef CLIENTPREDICTION2
extern tic_t localgametic;
#else
#define localgametic leveltime
#endif

// Player spawn spots.
extern mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
extern mapthing_t *bluectfstarts[MAXPLAYERS]; // CTF
extern mapthing_t *redctfstarts[MAXPLAYERS]; // CTF

// Parameters for intermission.
extern int prevmap, nextmap;

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined(__MACOS__) &&  !defined(__MACH__)
#define DEBFILE(msg) I_OutputMsg(msg)
extern FILE* debugfile;
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if(debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
extern FILE* debugfile;
#else
#define DEBFILE(msg) {}
extern FILE* debugfile;
#endif
#endif //__MACOS__ && !__MACH__

#ifdef DEBUGFILE
extern int debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;

// debug flag to cancel adaptiveness
extern boolean singletics;

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_timetic; // display high resolution timer
extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_nodownloading; // keep clients from downloading WADs.
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern int adminplayer, serverplayer;

/// \note put these in d_clisrv outright?

#endif //__DOOMSTAT__
