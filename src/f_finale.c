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
/// \brief Title screen, intro, game evaluation, and credits.

#include "doomdef.h"
#include "doomstat.h"
#include "am_map.h"
#include "dstrings.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "m_menu.h"
#include "dehacked.h"
#include "keys.h"
#include "console.h"
#include "m_random.h"

credit_t credits[19];

// Stage of animation:
// 0 = text, 1 = art screen
static int finalestage, finalecount;

static int timetonext; // Delay between screen changes
static int finaletextcount;
static int animtimer; // Used for some animation timings

static int deplete, stoptimer;

#define TEXTSPEED 3
#define TEXTWAIT 250

static const char* finaletext = NULL;
static const char* finaleflat;
static boolean keypressed = false;

static patch_t* background;
static patch_t* currentanim;
static patch_t* nextanim;
static patch_t* first;
static patch_t* second;
static patch_t* third;

// Demo end stuff
static patch_t* desonic;
static patch_t* dehand1;
static patch_t* dehand2;
static patch_t* dehand3;
static patch_t* deblink1;
static patch_t* deblink2;

// De-Demo'd Title Screen
static patch_t* ttbanner; // white banner with "robo blast" and "2"
static patch_t* ttwing; // wing background
static patch_t* ttsonic; // "SONIC"
static patch_t* ttswave1; // Title Sonics
static patch_t* ttswave2;
static patch_t* ttswip1;
static patch_t* ttsprep1;
static patch_t* ttsprep2;
static patch_t* ttspop1;
static patch_t* ttspop2;
static patch_t* ttspop3;
static patch_t* ttspop4;
static patch_t* ttspop5;
static patch_t* ttspop6;
static patch_t* ttspop7;

static void F_SkyScroll(void);

static boolean drawemblem = false, drawchaosemblem = false, runningprecutscene = false, precutresetplayer = false;

typedef struct
{
	int frame;
	int tics;
} mouth_t;

static mouth_t* mouthframe;
static int mouthtics;

#define NUMMOUTHSTATES 534

// Talking mouth on demo end screen
static mouth_t mouthstates[NUMMOUTHSTATES] =
{
	// 1st 'finalstage'
	{2,3},
	{1,3},
	{0,4},
	{3,2},
	{1,1},
	{2,4},
	{1,2},
	{2,5},
	{1,2},
	{0,2},
	{2,3},
	{1,1},
	{2,5},
	{1,2},
	{3,6},
	{0,1},
	{1,1},
	{2,2},
	{1,1},
	{0,2},
	{3,9},
	{0,10},
	{1,1},
	{2,2},
	{1,4},
	{2,3},
	{1,3},
	{4,3},
	{2,1},
	{1,1},
	{0,3},
	{1,1},
	{2,3},
	{1,1},
	{2,3},
	{1,2},
	{0,2},
	{1,3},
	{0,3},
	{1,2},
	{3,3},
	{0,3},
	{1,1},
	{2,2},
	{0,1},
	{2,1},
	{1,2},
	{0,3},
	{1,1},
	{2,2},
	{0,5},
	{1,1},
	{4,3},
	{1,1},
	{0,3},
	{1,1},
	{3,4},
	{0,20},
	{1,1},
	{2,2},
	{1,1},
	{0,1},
	{1,4},
	{2,2},
	{1,2},
	{0,1},
	{1,3},
	{0,1},
	{2,1},
	{1,1},
	{0,2},
	{1,3},
	{0,3},
	{1,6},
	{0,1},
	{3,3},
	{0,1},
	{1,4},
	{0,1},
	{3,5},
	{0,1},
	{3,2},
	{0,2},
	{1,7},
	{0,2},
	{3,3},
	{1,1},
	{2,1},
	{4,2},
	{2,1},
	{1,1},
	{0,2},
	{1,4},
	{0,140},

	// 2nd 'finalstage'
	{0,1},
	{1,1},
	{0,1},
	{2,1},
	{1,1},
	{0,1},
	{1,2},
	{0,3},
	{1,1},
	{0,1},
	{1,1},
	{2,3},
	{1,1},
	{0,1},
	{1,2},
	{2,3},
	{1,1},
	{0,2},
	{1,3},
	{0,3},
	{3,3},
	{1,1},
	{2,1},
	{1,1},
	{0,1},
	{2,3},
	{1,1},
	{0,3},
	{1,3},
	{2,4},
	{1,1},
	{0,1},
	{1,5},
	{0,7},
	{3,2},
	{0,1},
	{1,2},
	{3,5},
	{0,1},
	{3,2},
	{0,1},
	{1,2},
	{3,4},
	{0,1},
	{1,1},
	{2,2},
	{0,1},
	{1,2},
	{3,4},
	{0,2},
	{1,1},
	{2,3},
	{1,2},
	{0,5},
	{1,3},
	{1,1},
	{2,1},
	{1,3},
	{0,2},
	{2,2},
	{1,2},
	{0,2},
	{1,3},
	{0,3},
	{3,7},
	{0,1},
	{2,2},
	{1,3},
	{0,3},
	{3,6},
	{1,3},
	{0,14},
	{3,5},
	{1,4},
	{0,3},
	{2,2},
	{1,3},
	{0,5},
	{1,1},
	{2,2},
	{1,3},
	{0,140},

	// 3rd 'finalstage'
	{0,1},
	{1,1},
	{3,2},
	{1,1},
	{0,3},
	{1,1},
	{0,1},
	{2,2},
	{1,1},
	{0,1},
	{1,2},
	{0,1},
	{1,1},
	{2,4},
	{1,1},
	{0,1},
	{2,3},
	{1,1},
	{2,2},
	{1,1},
	{2,3},
	{1,2},
	{0,3},
	{1,2},
	{0,2},
	{2,2},
	{1,1},
	{0,1},
	{1,3},
	{0,2},
	{1,2},
	{0,1},
	{1,1},
	{2,2},
	{1,1},
	{0,1},
	{1,5},
	{0,28},
	{2,3},
	{1,2},
	{3,4},
	{2,1},
	{1,2},
	{0,1},
	{2,4},
	{1,1},
	{0,3},
	{3,4},
	{1,1},
	{0,3},
	{1,1},
	{4,4},
	{1,2},
	{0,3},
	{1,1},
	{3,6},
	{1,5},
	{2,2},
	{0,2},
	{1,6},
	{0,14},
	{1,1},
	{2,2},
	{1,1},
	{2,2},
	{0,3},
	{1,6},
	{0,7},
	{1,1},
	{2,4},
	{1,1},
	{0,3},
	{4,4},
	{7,7},
	{1,5},
	{0,140},

	// 4th 'finalstage'
	{0,2},
	{1,5},
	{4,3},
	{1,1},
	{0,3},
	{2,3},
	{1,2},
	{0,1},
	{1,2},
	{0,4},
	{1,2},
	{2,2},
	{4,2},
	{2,1},
	{1,1},
	{3,3},
	{1,1},
	{0,2},
	{1,3},
	{0,2},
	{2,2},
	{1,2},
	{0,2},
	{1,4},
	{0,2},
	{3,6},
	{0,2},
	{1,2},
	{2,3},
	{1,2},
	{0,2},
	{1,1},
	{3,8},
	{0,25},
	{1,2},
	{0,2},
	{1,2},
	{2,3},
	{1,2},
	{0,2},
	{1,4},
	{0,2},
	{1,1},
	{2,5},
	{1,1},
	{2,6},
	{1,3},
	{0,2},
	{1,7},
	{0,8},
	{1,1},
	{2,2},
	{1,1},
	{0,1},
	{1,2},
	{0,1},
	{2,3},
	{1,3},
	{0,2},
	{3,4},
	{1,2},
	{0,2},
	{1,2},
	{2,3},
	{1,1},
	{0,2},
	{3,1},
	{2,2},
	{1,3},
	{0,4},
	{1,1},
	{2,2},
	{1,2},
	{0,3},
	{1,1},
	{2,1},
	{1,1},
	{0,1},
	{1,2},
	{0,2},
	{3,2},
	{0,3},
	{1,2},
	{2,4},
	{1,1},
	{2,1},
	{1,1},
	{0,2},
	{1,1},
	{2,2},
	{1,1},
	{0,4},
	{1,1},
	{2,2},
	{1,2},
	{0,2},
	{3,3},
	{1,3},
	{0,240},

	// 5th 'finalstage'
	{0,1},
	{1,1},
	{2,6},
	{1,4},
	{0,11},
	{2,1},
	{4,2},
	{2,1},
	{1,1},
	{3,4},
	{0,3},
	{1,2},
	{0,1},
	{1,3},
	{0,1},
	{1,3},
	{2,2},
	{1,2},
	{0,2},
	{1,1},
	{2,2},
	{1,2},
	{0,3},
	{3,3},
	{1,2},
	{0,2},
	{1,1},
	{2,4},
	{1,1},
	{0,1},
	{1,2},
	{0,1},
	{1,2},
	{0,3},
	{1,6},
	{0,19},
	{1,2},
	{0,2},
	{3,4},
	{1,1},
	{2,3},
	{0,2},
	{1,1},
	{0,1},
	{1,1},
	{2,3},
	{1,1},
	{0,1},
	{1,4},
	{0,1},
	{1,5},
	{0,16},
	{1,1},
	{2,3},
	{1,1},
	{0,2},
	{1,1},
	{2,2},
	{1,1},
	{0,1},
	{1,2},
	{0,1},
	{1,1},
	{2,1},
	{1,2},
	{2,1},
	{1,1},
	{0,2},
	{1,1},
	{2,1},
	{1,1},
	{0,2},
	{1,3},
	{0,4},
	{1,1},
	{2,3},
	{1,1},
	{0,3},
	{1,3},
	{0,3},
	{2,4},
	{1,3},
	{0,3},
	{2,1},
	{4,1},
	{2,1},
	{1,1},
	{0,1},
	{1,2},
	{0,3},
	{1,2},
	{2,2},
	{4,6},
	{2,1},
	{1,1},
	{0,2},
	{1,4},
	{0,4},
	{1,2},
	{0,2},
	{1,2},
	{0,1},
	{2,2},
	{1,1},
	{0,2},
	{1,2},
	{2,3},
	{1,1},
	{0,1},
	{1,2},
	{0,1},
	{2,3},
	{1,1},
	{0,1},
	{1,5},
	{0,4},
	{1,4},
	{0,20},
	{1,1},
	{2,2},
	{1,1},
	{0,2},
	{3,3},
	{1,2},
	{0,2},
	{1,1},
	{2,2},
	{1,1},
	{2,4},
	{1,1},
	{0,2},
	{3,6},
	{1,1},
	{0,3},
	{1,2},
	{2,2},
	{1,1},
	{2,3},
	{1,2},
	{0,2},
	{1,2},
	{0,1},
	{2,3},
	{1,2},
	{0,1},
	{1,3},
	{2,5},
	{1,2},
	{0,3},
	{1,5},
	{0,420},
};

//
// F_StartFinale
//
void F_StartFinale(void)
{
	gamestate = GS_FINALE;

	// IWAD dependent stuff.
	// This has been changed severely, and some stuff might have changed in the process.

	switch(gamemap)
	{
		case 4:
			finaletext = C1TEXT;
			break;
		case 5:
			finaletext = C2TEXT;
			break;
		case 24:
			finaletext = C3TEXT;
			break;
		case 25:
			finaletext = C4TEXT;
			break;
		case 26:
			finaletext = C5TEXT;
			break;
		case 27:
			finaletext = C6TEXT;
			break;

		// Indeterminate.
		default:
			finaletext = C1TEXT; // FIXME - other text, music?
			break;
	}

	finaleflat = "FWATER1";

	finalestage = 0;
	finalecount = 0;
}

// De-Demo'd Title Screen
void F_StartTitleScreen(void)
{
	gamestate = GS_TITLESCREEN;
	CON_ClearHUD();

	// IWAD dependent stuff.

	S_ChangeMusic(mus_titles, false);

	finalecount = 0;
	finalestage = 0;
	animtimer = 0;

	ttbanner = W_CachePatchName("TTBANNER", PU_LEVEL);
	ttwing = W_CachePatchName("TTWING", PU_LEVEL);
	ttsonic = W_CachePatchName("TTSONIC", PU_LEVEL);
	ttswave1 = W_CachePatchName("TTSWAVE1", PU_LEVEL);
	ttswave2 = W_CachePatchName("TTSWAVE2", PU_LEVEL);
	ttswip1 = W_CachePatchName("TTSWIP1", PU_LEVEL);
	ttsprep1 = W_CachePatchName("TTSPREP1", PU_LEVEL);
	ttsprep2 = W_CachePatchName("TTSPREP2", PU_LEVEL);
	ttspop1 = W_CachePatchName("TTSPOP1", PU_LEVEL);
	ttspop2 = W_CachePatchName("TTSPOP2", PU_LEVEL);
	ttspop3 = W_CachePatchName("TTSPOP3", PU_LEVEL);
	ttspop4 = W_CachePatchName("TTSPOP4", PU_LEVEL);
	ttspop5 = W_CachePatchName("TTSPOP5", PU_LEVEL);
	ttspop6 = W_CachePatchName("TTSPOP6", PU_LEVEL);
	ttspop7 = W_CachePatchName("TTSPOP7", PU_LEVEL);
}

// Demo end thingy
void F_StartDemoEnd(void)
{
	int i;

	if(modifiedgame)
		D_StartTitle();

	gamestate = GS_DEMOEND;

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	S_StopMusic();

	finalestage = 1;
	finalecount = 0;
	timetonext = 8*TICRATE;

	// Patch the animation array to keep in-sync if the framerate has been changed.
	for(i = 0; i < NUMMOUTHSTATES; i++)
		mouthstates[i].tics *= NEWTICRATERATIO;

	mouthframe = &mouthstates[0];
	mouthtics = mouthframe->tics;

	// load all the graphics
	desonic = W_CachePatchName("SONCDEND", PU_CACHE);
	dehand1 = W_CachePatchName("DEHAND1", PU_CACHE);
	dehand2 = W_CachePatchName("DEHAND2", PU_CACHE);
	dehand3 = W_CachePatchName("DEHAND3", PU_CACHE);
	deblink1 = W_CachePatchName("DEBLINK1", PU_CACHE);
	deblink2 = W_CachePatchName("DEBLINK2", PU_CACHE);
}

void F_StartGameEvaluation(void)
{
	gamestate = GS_EVALUATION;

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();

	if(ALL7EMERALDS)
		animtimer = 64;

	finalecount = 0;
}

void F_StartCredits(void)
{
	int i = 0;
	gamestate = GS_CREDITS;

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	S_StopMusic();
	S_ChangeMusic(mus_credit, false);
	finalecount = 0;
	animtimer = 0;

	// "Modification By" - see dehacked.c
	if(modcredits)
		timetonext = 165*NEWTICRATERATIO;
	else
		timetonext = 5*TICRATE-1;

	// Initalize the credits table
	strcpy(credits[i].header, "Sonic Team Junior\n");
	strcpy(credits[i].fakenames[0], "Staff\n");
	strcpy(credits[i].realnames[0], "Staff\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Producer\n");
	strcpy(credits[i].fakenames[0], "SSNTails\n");
	strcpy(credits[i].realnames[0], "Art Freda\n");
	strcpy(credits[i].fakenames[1], "\n");
	strcpy(credits[i].realnames[1], "\n");
	strcpy(credits[i].fakenames[2], "Director\n");
	strcpy(credits[i].realnames[2], "Director\n");
	strcpy(credits[i].fakenames[3], "Sonikku\n");
	strcpy(credits[i].realnames[3], "Johnny Wallbank\n");
	credits[i].numnames = 4;
	i++;
	strcpy(credits[i].header, "Game Designers\n");
	strcpy(credits[i].fakenames[0], "Sonikku\n");
	strcpy(credits[i].fakenames[1], "SSNTails\n");
	strcpy(credits[i].fakenames[2], "Mystic\n");
	strcpy(credits[i].realnames[0], "Johnny Wallbank\n");
	strcpy(credits[i].realnames[1], "Art Freda\n");
	strcpy(credits[i].realnames[2], "Ben Geyer\n");
	credits[i].numnames = 3;
	i++;
	strcpy(credits[i].header, "Character Designers\n");
	strcpy(credits[i].fakenames[0], "Sonikku\n");
	strcpy(credits[i].fakenames[1], "Instant Sonic\n");
	strcpy(credits[i].realnames[0], "Johnny Wallbank\n");
	strcpy(credits[i].realnames[1], "David Spencer Jr\n");
	credits[i].numnames = 2;
	i++;
	strcpy(credits[i].header, "Visual Design\n");
	strcpy(credits[i].fakenames[0], "SSNTails\n");
	strcpy(credits[i].realnames[0], "Art Freda\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Landscape Design\n");
	strcpy(credits[i].fakenames[0], "Sonikku\n");
	strcpy(credits[i].realnames[0], "Johnny Wallbank\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Chief Programmer\n");
	strcpy(credits[i].fakenames[0], "SSNTails\n");
	strcpy(credits[i].realnames[0], "Art Freda\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Programmers\n");
	strcpy(credits[i].fakenames[0], "Alam_GBC\n");
	strcpy(credits[i].fakenames[1], "Graue\n");
	strcpy(credits[i].fakenames[2], "Orospakr\n");
	strcpy(credits[i].realnames[0], "Alam Arias\n");
	strcpy(credits[i].realnames[1], "Scott Feeney\n");
	strcpy(credits[i].realnames[2], "Andrew Clunis\n");
	credits[i].numnames = 3;
	i++;
	strcpy(credits[i].header, "Coding Assistants\n");
	strcpy(credits[i].fakenames[0], "StroggOnMeth\n");
	strcpy(credits[i].fakenames[1], "Cyan Helkaraxe\n");
	strcpy(credits[i].fakenames[2], "Logan_GBA\n");
	strcpy(credits[i].realnames[0], "Steven McGranahan\n");
	strcpy(credits[i].realnames[1], "Cyan Helkaraxe\n");
	strcpy(credits[i].realnames[2], "Logan Arias\n");
	credits[i].numnames = 3;
	i++;
	strcpy(credits[i].header, "Levels\n");
	strcpy(credits[i].fakenames[0], "Sonikku\n");
	strcpy(credits[i].fakenames[1], "SSNTails\n");
	strcpy(credits[i].fakenames[2], "Mystic\n");
	strcpy(credits[i].fakenames[3], "Digiku\n");
	strcpy(credits[i].realnames[0], "Johnny Wallbank\n");
	strcpy(credits[i].realnames[1], "Art Freda\n");
	strcpy(credits[i].realnames[2], "Ben Geyer\n");
	strcpy(credits[i].realnames[3], "Marco Zafra\n");
	credits[i].numnames = 4;
	i++;
	strcpy(credits[i].header, "Texture Artists\n");
	strcpy(credits[i].fakenames[0], "KinkaJoy\n");
	strcpy(credits[i].fakenames[1], "SSNTails\n");
	strcpy(credits[i].fakenames[2], "Blaze Hedgehog\n");
	strcpy(credits[i].realnames[0], "Buddy Fischer\n");
	strcpy(credits[i].realnames[1], "Art Freda\n");
	strcpy(credits[i].realnames[2], "Ryan Bloom\n");
	credits[i].numnames = 3;
	i++;
	strcpy(credits[i].header, "Music Production\n");
	strcpy(credits[i].fakenames[0], "Omni Echidna\n");
	strcpy(credits[i].fakenames[1], "Arrow\n");
	strcpy(credits[i].fakenames[2], "Stuf\n");
	strcpy(credits[i].fakenames[3], "SSNTails\n");
	strcpy(credits[i].fakenames[4], "Cyan Helkaraxe\n");
	strcpy(credits[i].fakenames[5], "Red XVI\n");
	strcpy(credits[i].realnames[0], "David Bulmer\n");
	strcpy(credits[i].realnames[1], "Jarel Jones\n");
	strcpy(credits[i].realnames[2], "Stefan Rimalia\n");
	strcpy(credits[i].realnames[3], "Art Freda\n");
	strcpy(credits[i].realnames[4], "Cyan Helkaraxe\n");
	strcpy(credits[i].realnames[5], "Malcolm Brown\n");
	credits[i].numnames = 6;
	i++;
	strcpy(credits[i].header, "Lead Guitar\n");
	strcpy(credits[i].fakenames[0], "Big Wave Dave\n");
	strcpy(credits[i].realnames[0], "David Spencer Sr\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Sound Effects\n");
	strcpy(credits[i].fakenames[0], "Sega\n");
	strcpy(credits[i].fakenames[1], "Instant Sonic\n");
	strcpy(credits[i].fakenames[2], "Various Sources\n");
	strcpy(credits[i].realnames[0], "Sega\n");
	strcpy(credits[i].realnames[1], "David Spencer Jr\n");
	strcpy(credits[i].realnames[2], "Various Sources\n");
	credits[i].numnames = 3;
	i++;
	strcpy(credits[i].header, "Official Mascot\n");
	strcpy(credits[i].fakenames[0], "Mr Encyclopedia\n");
	strcpy(credits[i].realnames[0], "Jason Butz\n");
	credits[i].numnames = 1;
	i++;
	strcpy(credits[i].header, "Beta Testing\n");
	strcpy(credits[i].fakenames[0], "Tets\n");
	strcpy(credits[i].fakenames[1], "Mystic\n");
	strcpy(credits[i].fakenames[2], "Digiku\n");
	strcpy(credits[i].fakenames[3], "Fox Unit\n");
	strcpy(credits[i].fakenames[4], "Omega Hedgehog\n");
	strcpy(credits[i].fakenames[5], "Furious Fox\n");
	strcpy(credits[i].realnames[0], "Bill Reed\n");
	strcpy(credits[i].realnames[1], "Ben Geyer\n");
	strcpy(credits[i].realnames[2], "Marco Zafra\n");
	strcpy(credits[i].realnames[3], "Alex Strout\n");
	strcpy(credits[i].realnames[4], "Mike Meredith\n");
	strcpy(credits[i].realnames[5], "Hank Brannock\n");
	credits[i].numnames = 6;
	i++;
	strcpy(credits[i].header, "Special Thanks\n");
	strcpy(credits[i].fakenames[0], "Doom Legacy Project\n");
	strcpy(credits[i].fakenames[1], "iD Software\n");
	strcpy(credits[i].fakenames[2], "Dave Perry\n");
	strcpy(credits[i].fakenames[3], "The Chaos Emerald\n");
	strcpy(credits[i].fakenames[4], "Cinossu\n");
	strcpy(credits[i].fakenames[5], "MistaED\n");
	strcpy(credits[i].realnames[0], "Doom Legacy Project\n");
	strcpy(credits[i].realnames[1], "iD Software\n");
	strcpy(credits[i].realnames[2], "Dave Perry\n");
	strcpy(credits[i].realnames[3], "The Chaos Emerald\n");
	strcpy(credits[i].realnames[4], "Marc Gordon\n");
	strcpy(credits[i].realnames[5], "Alex Fuller\n");
	credits[i].numnames = 6;
	i++;
	strcpy(credits[i].header, "In Fond Memory Of\n");
	strcpy(credits[i].fakenames[0], "Naoto Oshima\n");
	strcpy(credits[i].fakenames[1], "Howard Drossin\n");
	strcpy(credits[i].fakenames[2], "\n");
	strcpy(credits[i].fakenames[3], "\n");
	strcpy(credits[i].realnames[0], "Naoto Oshima\n");
	strcpy(credits[i].realnames[1], "Howard Drossin\n");
	strcpy(credits[i].realnames[2], "\n");
	strcpy(credits[i].realnames[3], "\n");
	credits[i].numnames = 4;
	i++;
}

static int scenenum, cutnum;
static int picxpos, picypos, picnum, pictime;
static int textxpos, textypos;

void F_StartCustomCutscene(int cutscenenum, boolean precutscene, boolean resetplayer)
{
	gamestate = GS_CUTSCENE;

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	finaletext = cutscenes[cutscenenum].scene[0].text;

	CON_ClearHUD();

	runningprecutscene = precutscene;

	if(runningprecutscene)
		precutresetplayer = resetplayer;

	scenenum = picnum = 0;
	cutnum = cutscenenum;
	picxpos = cutscenes[cutnum].scene[0].xcoord[0];
	picypos = cutscenes[cutnum].scene[0].ycoord[0];
	textxpos = cutscenes[cutnum].scene[0].textxpos;
	textypos = cutscenes[cutnum].scene[0].textypos;

	pictime = cutscenes[cutnum].scene[0].picduration[0];

	keypressed = false;
	finalestage = 0;
	finalecount = 0;
	finaletextcount = 0;
	timetonext = 0;
	animtimer = cutscenes[cutnum].scene[0].picduration[0]; // Picture duration
	stoptimer = 0;
	mouthtics = BASEVIDWIDTH - 64; // Crap.. I forgot what this is for! I'd better not touch it...

	if(cutscenes[cutnum].scene[scenenum].musicslot != 0)
		S_ChangeMusic(cutscenes[cutnum].scene[scenenum].musicslot, false);
}

static void F_IntroTextWrite(void);
// Introduction
void F_StartIntro(void)
{
	if(introtoplay)
	{
		F_StartCustomCutscene(introtoplay - 1, false, false);
		return;
	}

	gamestate = GS_INTRO;
	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	finaletext = E0TEXT;

	finalestage = finalecount = finaletextcount = timetonext = animtimer = stoptimer = 0;
	mouthtics = BASEVIDWIDTH - 64;
}

boolean F_Responder(event_t* event)
{
	if(!finalestage)
	{
		if(event->type != ev_keydown)
			return false;

		if(keypressed)
			return false;

		keypressed = true;
		return true;
	}
	return false;
}

// Intro
boolean F_IntroResponder(event_t* event)
{
	if(event->type != ev_keydown && event->data1 != 301)
		return false;

	if(event->data1 != 27 && event->data1 != KEY_ENTER && event->data1 != KEY_SPACE)
		return false;

	if(keypressed)
		return false;

	keypressed = true;
	return true;
}

boolean F_CutsceneResponder(event_t* event)
{
	if(cutnum == introtoplay-1)
		return F_IntroResponder(event);

	return false;
}

boolean F_CreditResponder(event_t* event)
{
	if(!(grade & 1))
		return false;

	if(event->type != ev_keydown)
		return false;

	if(event->data1 != 27 && event->data1 != KEY_ENTER && event->data1 != KEY_SPACE)
		return false;

	if(keypressed)
		return true;

	keypressed = true;
	return true;
}

//
// F_Ticker
//
void F_Ticker(void)
{
	// advance animation
	finalecount++;

	switch(finalestage)
	{
		case 0:
			// check for skipping
			if(keypressed)
			{
				keypressed = false;
				if(finaletext && (unsigned)finalecount < strlen(finaletext)*TEXTSPEED)
					finalecount += MAXINT/2; // force text to be written
				else
				{
					gameaction = ga_worlddone;
					finalecount = MININT; // wait until map is launched
				}
			}
			break;
		default:
			break;
	}
}

// De-Demo'd Title Screen
void F_TitleScreenTicker(void)
{
	finalecount++;
	finalestage += 8;
}

// Demo end thingy
//
// F_DemoEndTicker
//
void F_DemoEndTicker(void)
{
	if(timetonext > 0)
		timetonext--;
	else // Switch finalestages
	{
		finalestage++;
		switch(finalestage)
		{
			case 2:
				finalecount = 0;
				timetonext = 7*TICRATE;
				mouthframe++;
				mouthtics = mouthframe->tics;
				S_StartSound(NULL, sfx_annon2);
				break;
			case 3:
				finalecount = 0;
				timetonext = 7*TICRATE;
				mouthframe++;
				mouthtics = mouthframe->tics;
				S_StartSound(NULL, sfx_annon3);
				break;
			case 4:
				finalecount = 0;
				timetonext = (9*TICRATE);
				mouthframe++;
				mouthtics = mouthframe->tics;
				S_StartSound(NULL, sfx_annon4);
				break;
			case 5:
				finalecount = 0;
				timetonext = 11*TICRATE;
				mouthframe++;
				mouthtics = mouthframe->tics;
				S_StartSound(NULL, sfx_annon5);
				break;
			case 6:
				finalecount = 0;
				timetonext = 5*TICRATE;
				break;
			case 7:
				D_StartTitle();
				break;
			default:
				break;
		}
	}
}

void F_GameEvaluationTicker(void)
{
	finalecount++;

	if(finalecount > 10*TICRATE)
		F_StartDemoEnd();
}

void F_CreditTicker(void)
{
	finalecount++;

	if(finalecount > 90*TICRATE)
		F_StartGameEvaluation();
}

//
// F_IntroTicker
//
void F_IntroTicker(void)
{
	// advance animation
	finalecount++;
	finaletextcount++;

	if(finalecount % 3 == 0)
		mouthtics--;

	// check for skipping
	if(keypressed)
	{
		keypressed = false;
		finaletextcount += 64;
		if(timetonext)
			timetonext = 2;
	}
}

void F_CutsceneTicker(void)
{
	int i;

	// advance animation
	finalecount++;
	finaletextcount++;

	for(i=0; i<MAXPLAYERS; i++)
	{
		if(players[i].cmd.buttons & BT_USE)
		{
			keypressed = false;
			finaletextcount += 64;
			if(timetonext)
				timetonext = 2;
		}
	}
}

//
// F_TextWrite
//
static void F_TextWrite(void)
{
	int w, count, c, cx, cy;
	const char* ch;

	// erase the entire screen with a tiled background
	V_DrawFlatFill(0, 0, vid.width, vid.height, W_GetNumForName(finaleflat));

	// DRAW A FULL PIC INSTEAD OF FLAT!
	switch(gamemap)
	{
		case 4:
			V_DrawScaledPatch(0, 0, 0, W_CacheLumpName("INTERSCR", PU_CACHE));
			break;
		case 5:
			V_DrawScaledPatch(0, 0, 0, W_CacheLumpName("INTERSCR", PU_CACHE));
			break;
		default:
			break;
	}

	// draw some of the text onto the screen
	cx = cy = 10;
	ch = finaletext;

	count = (finalecount - 10)/TEXTSPEED;
	if(count < 0 || !ch)
		count = 0;
	for(; count; count--)
	{
		c = *ch++;
		if(!c)
			break;
		if(c == '\n')
		{
			cx = 10;
			cy += 11;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if(c < 0 || (c >= HU_REALFONTSIZE && c != '~' - HU_FONTSTART && c != '`' - HU_FONTSTART))
		{ /// \note font end hack
			cx += 4;
			continue;
		}

		w = hu_font[c]->width;
		if(cx + w > vid.width)
			break;
		V_DrawScaledPatch(cx, cy, 0, hu_font[c]);
		cx += w;
	}
}

//
// F_WriteText
//
static void F_WriteText(int cx, int cy)
{
	int w, count, c, originalx;
	const char* ch;

	originalx = cx;

	// draw some of the text onto the screen
	ch = finaletext;

	count = (finaletextcount - 10)/2;

	if(count < 0)
		count = 0;
	if(timetonext == 1 || !ch)
	{
		finaletextcount = 0;
		timetonext = 0;
		mouthtics = BASEVIDWIDTH - 64;
		return;
	}

	for(; count; count--)
	{
		c = *ch++;
		if(!c)
			break;

		if(c == '#')
		{
			if(!timetonext)
			{
				if(finaletext == T5TEXT)
					timetonext = 12*TICRATE + 1;
				else
					timetonext = 5*TICRATE + 1;
			}
			break;
		}
		if(c == '\n')
		{
			cx = originalx;
			cy += 12;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if(c < 0 || (c >= HU_REALFONTSIZE && c != '~' - HU_FONTSTART && c != '`' - HU_FONTSTART)) /// \note font end hack
		{
			cx += 4;
			continue;
		}

		w = hu_font[c]->width;
		if(cx + w > vid.width)
			break;
		V_DrawScaledPatch(cx, cy, 0, hu_font[c]);
		cx += w;
	}
}

static void F_WriteCutsceneText(void)
{
	int w, count, c, originalx, cx = textxpos, cy = textypos;
	const char* ch;

	originalx = cx;

	// draw some of the text onto the screen
	ch = finaletext;

	count = (finaletextcount - 10)/2;

	if(count < 0)
		count = 0;

	if(timetonext == 1 || !ch)
	{
		finaletextcount = 0;
		timetonext = 0;
		mouthtics = BASEVIDWIDTH - 64;
		return;
	}

	for(; count; count--)
	{
		c = *ch++;
		if(!c)
			break;

		if(c == '#')
		{
			if(!timetonext)
				timetonext = 5*TICRATE + 1;
			break;
		}
		if(c == '\n')
		{
			cx = originalx;
			cy += 12;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if(c < 0 || (c >= HU_REALFONTSIZE && c != '~' - HU_FONTSTART && c != '`' - HU_FONTSTART))
		{ /// \note font end hack
			cx += 4;
			continue;
		}

		w = hu_font[c]->width;
		if(cx + w > vid.width)
			break;
		V_DrawScaledPatch(cx, cy, 0, hu_font[c]);
		cx += w;
	}
}

//
// F_IntroTextWrite
//
static void F_IntroTextWrite(void)
{
	boolean nobg = false, highres = false;
	int cx = 8, cy = 128;

	// DRAW A FULL PIC INSTEAD OF FLAT!
	if(finaletext == E0TEXT)
		nobg = true;
	else if(finaletext == E1TEXT)
		background = W_CachePatchName("INTRO1", PU_CACHE);
	else if(finaletext == E2TEXT)
	{
		background = W_CachePatchName("INTRO2", PU_CACHE);
		highres = true;
	}
	else if(finaletext == E3TEXT)
		background = W_CachePatchName("INTRO3", PU_CACHE);
	else if(finaletext == E4TEXT)
		background = W_CachePatchName("INTRO4", PU_CACHE);
	else if(finaletext == C1TEXT)
	{
		background = W_CachePatchName("DRAT", PU_CACHE);
		highres = true;
	}
	else if(finaletext == C2TEXT)
	{
		background = W_CachePatchName("INTRO6", PU_CACHE);
		cx = 180;
		cy = 8;
	}
	else if(finaletext == C3TEXT)
		background = W_CachePatchName("SGRASS1", PU_CACHE);
	else if(finaletext == C4TEXT)
	{
		background = W_CachePatchName("WATCHING", PU_CACHE);
		highres = true;
	}
	else if(finaletext == C5TEXT)
	{
		background = W_CachePatchName("ZOOMING", PU_CACHE);
		highres = true;
	}
	else if(finaletext == C6TEXT)
		nobg = true;
	else if(finaletext == T1TEXT)
		background = W_CachePatchName("INTRO5", PU_CACHE);
	else if(finaletext == T2TEXT)
	{
		background = W_CachePatchName("REVENGE", PU_CACHE);
		highres = true;
	}
	else if(finaletext == T3TEXT)
	{
		nobg = true;
		cx = 8;
		cy = 8;
	}
	else if(finaletext == T4TEXT)
	{
		background = W_CachePatchName("SONICDO1", PU_CACHE);
		highres = true;
		cx = 224;
		cy = 8;
	}
	else if(finaletext == T5TEXT)
	{
		background = W_CachePatchName("INTRO7", PU_CACHE);
		highres = true;
	}

	V_DrawFill(0, 0, vid.width, vid.height, 0);

	if(finaletext == E0TEXT)
	{
		V_DrawCreditString(160 - (V_CreditStringWidth("SONIC TEAM JR")/2), 80, 0, "SONIC TEAM JR");
		V_DrawCreditString(160 - (V_CreditStringWidth("PRESENTS")/2), 96, 0, "PRESENTS");
	}
	else if(finaletext == C6TEXT)
	{
		if(finaletextcount > 8*TICRATE && finaletextcount < 9*TICRATE)
		{
			if(!(finalecount & 3))
				background = W_CachePatchName("BRITEGG1", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG1", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else if(finaletextcount > 10*TICRATE && finaletextcount < 11*TICRATE)
		{
			if(!(finalecount & 3))
				background = W_CachePatchName("BRITEGG2", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG2", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else if(finaletextcount > 12*TICRATE && finaletextcount < 13*TICRATE)
		{
			if(!(finalecount & 3))
				background = W_CachePatchName("BRITEGG3", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG3", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else
		{
			F_SkyScroll();
			if(timetonext == 7)
			{
				stoptimer = finalecount;
				animtimer = finalecount % 16;
			}
			else if(timetonext > 1 && timetonext < 7)
			{
				animtimer = stoptimer;
				deplete -= 32;
			}
			else
			{
				animtimer = finalecount % 16;
				deplete = 160;
			}

			if(finalecount & 1)
			{
				V_DrawScaledPatch(deplete, 8, 0, W_CachePatchName("RUN2", PU_CACHE));
				V_DrawScaledPatch(deplete, 72, 0, W_CachePatchName("PEELOUT2", PU_CACHE));
			}
			else
			{
				V_DrawScaledPatch(deplete, 8, 0, W_CachePatchName("RUN1", PU_CACHE));
				V_DrawScaledPatch(deplete, 72, 0, W_CachePatchName("PEELOUT1", PU_CACHE));
			}
			V_DrawFill(0, 112, vid.width, (int)(vid.height - 128*vid.fdupy), 0);
		}
	}
	else if(finaletext == C3TEXT && timetonext > 0 && timetonext <= 4*TICRATE
		&& finaletextcount > 0)
	{
		background = W_CachePatchName("SGRASS5", PU_CACHE);
	}

	if(!nobg)
	{
		if(highres)
			V_DrawSmallScaledPatch(0, 0, 0, background,
				(byte*)defaulttranslationtables - 256 + (12<<8));
		else
			V_DrawScaledPatch(0, 0, 0, background);
	}

	if(finaletext == T3TEXT)
	{
		V_DrawFill(0, 0, vid.width, vid.height, 0);
		V_DrawSmallScaledPatch(144, 0, 0, W_CachePatchName("TAILSSAD", PU_CACHE),
			(byte*)defaulttranslationtables - 256 + (12<<8));
	}
	else if(finaletext == E4TEXT) // The asteroid SPINS!
	{
		if(mouthtics >= 0)
			V_DrawScaledPatch(mouthtics, 24, 0,
				W_CachePatchName(va("ROID00%.2d", stoptimer%35), PU_CACHE));
	}
	else if(finaletext == C1TEXT)
	{
		if(stoptimer == 5*TICRATE + (TICRATE/5)*3)
		{
			if(rendermode == render_soft)
			{
				tic_t nowtime, tics, wipestart;
				int y;
				boolean done;

				F_WriteText(cx, cy);

				F_WipeStartScreen();

				V_DrawScaledPatch(0, 0, 0, W_CachePatchName("RADAR", PU_CACHE));
				// draw some of the text onto the screen
				F_WriteText(cx, cy);

				F_WipeEndScreen(0, 0, vid.width, vid.height);

				wipestart = I_GetTime() - 1;
				y = wipestart + TICRATE; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);
					wipestart = nowtime;
					done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(!done && I_GetTime() < (unsigned)y);
			}
#ifdef HWRENDER
			else if(rendermode != render_none) // Delay the hardware modes as well
			{
				tic_t nowtime, tics, wipestart;
				int y;

				wipestart = I_GetTime() - 1;
				y = wipestart + 32; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(I_GetTime() < (unsigned)y);
			}
#endif
		}
		else if(stoptimer > 5*TICRATE+(TICRATE/5)*3)
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("RADAR", PU_CACHE));
	}
	else if(finaletext == T2TEXT)
	{
		if(stoptimer == 9*TICRATE)
		{
			if(rendermode == render_soft)
			{
				tic_t nowtime, tics, wipestart;
				int y;
				boolean done;

				F_WriteText(cx, cy);

				F_WipeStartScreen();

				V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("CONFRONT", PU_CACHE),
					(byte*)defaulttranslationtables - 256 + (12<<8));
				// draw some of the text onto the screen
				F_WriteText(cx, cy);

				F_WipeEndScreen(0, 0, vid.width, vid.height);

				wipestart = I_GetTime() - 1;
				y = wipestart + TICRATE; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);
					wipestart = nowtime;
					done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(!done && I_GetTime() < (unsigned)y);
			}
#ifdef HWRENDER
			else if(rendermode != render_none) // Delay the hardware modes as well
			{
				tic_t nowtime, tics, wipestart;
				int y;

				wipestart = I_GetTime() - 1;
				y = wipestart + 32; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(I_GetTime() < (unsigned)y);
			}
#endif
		}
		else if(stoptimer > 9*TICRATE)
			V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("CONFRONT", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
	}
	else if(finaletext == T4TEXT)
	{
		if(stoptimer == 7*TICRATE)
		{
			if(rendermode == render_soft)
			{
				tic_t nowtime, tics, wipestart;
				int y;
				boolean done;

				F_WriteText(cx, cy);

				F_WipeStartScreen();

				V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("SONICDO2", PU_CACHE),
					(byte*)defaulttranslationtables - 256 + (12<<8));
				// draw some of the text onto the screen
				F_WriteText(cx, cy);

				F_WipeEndScreen(0, 0, vid.width, vid.height);

				wipestart = I_GetTime() - 1;
				y = wipestart + TICRATE; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);
					wipestart = nowtime;
					done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(!done && I_GetTime() < (unsigned)y);
			}
#ifdef HWRENDER
			else if(rendermode != render_none) // Delay the hardware modes as well
			{
				tic_t nowtime, tics, wipestart;
				int y;

				wipestart = I_GetTime() - 1;
				y = wipestart+32; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(I_GetTime() < (unsigned)y);
			}
#endif
		}
		else if(stoptimer > 7*TICRATE)
			V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("SONICDO2", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
	}

	if(animtimer)
		animtimer--;

	if(finaletext == C3TEXT && timetonext && finaletextcount > 0)
	{
		first = W_CachePatchName("SGRASS2", PU_CACHE);
		second = W_CachePatchName("SGRASS3", PU_CACHE);
		third = W_CachePatchName("SGRASS4", PU_CACHE);

		if(timetonext == 3*TICRATE)
		{
			currentanim = first;
			nextanim = second;
			animtimer = TICRATE/7 + 1;
		}
		else if(animtimer == 1 && currentanim == first)
		{
			currentanim = second;
			nextanim = third;
			animtimer = TICRATE/7 + 1;
		}
		else if(animtimer == 1 && currentanim == second)
			currentanim = third;

		if(currentanim)
			V_DrawScaledPatch(123, 4, 0, currentanim);
	}

	F_WriteText(cx, cy);
}

//
// F_DrawPatchCol
//
static void F_DrawPatchCol(int x, patch_t* patch, int col, int yrepeat)
{
	column_t* column;
	byte* source;
	byte* dest = NULL;
	byte* desttop;
	int count;

	column = (column_t*)((byte*)patch + LONG(patch->columnofs[col]));
	desttop = screens[0] + x*vid.dupx;

	// step through the posts in a column
	while(column->topdelta != 0xff)
	{
		source = (byte*)column + 3;
		dest = desttop + column->topdelta*vid.width;
		count = column->length;

		while(count--)
		{
			int dupycount = vid.dupy;

			while(dupycount--)
			{
				int dupxcount = vid.dupx;
				while(dupxcount--)
					*dest++ = *source;

				dest += (vid.width - vid.dupx);
			}
			source++;
		}
		column = (column_t*)((byte*)column + column->length + 4);
	}

	// repeat a second time, for yrepeat number of pixels
	if(yrepeat)
	{
		column = (column_t*)((byte*)patch + LONG(patch->columnofs[col]));
		while(column->topdelta != 0xff)
		{
			source = (byte*)column + 3;
			count = column->length;

			while(count--)
			{
				int dupycount = vid.dupy;

				while(dupycount--)
				{
					int dupxcount = vid.dupx;
					while(dupxcount--)
						*dest++ = *source;

					dest += (vid.width - vid.dupx);
				}
				source++;
			}
			if(!--yrepeat)
				break;
			column = (column_t*)((byte*)column + column->length + 4);
		}
	}
}

#if BASEVIDWIDTH != 320
"BASEVIDWIDTH has changed. Update F_SkyScroll to deal with the change"
#endif

//
// F_SkyScroll
//
static void F_SkyScroll(void)
{
	int scrolled, x, mx, fakedwidth;
	patch_t* pat;

	pat = W_CachePatchName("TITLESKY", PU_CACHE);

	animtimer = finalecount % 64;

	fakedwidth = vid.width / vid.dupx;

	scrolled = 320 - (animtimer)*5;
	if(scrolled > 320)
		scrolled = 320;
	if(scrolled < 0)
		scrolled = 0;

	if(rendermode == render_soft)
	{
		int yr = 0;

		if(vid.fdupy > vid.dupy)
			yr = vid.height - vid.dupy*BASEVIDHEIGHT;

		for(x = 0, mx = 0; x < fakedwidth; x++, mx++)
		{
			if(mx == 320)
				mx = 0;

			if(mx + scrolled < 320)
				F_DrawPatchCol(x, pat, mx + scrolled, yr);
			else
				F_DrawPatchCol(x, pat, mx + scrolled - 320, yr);
		}
	}
#ifdef HWRENDER
	else if(rendermode != render_none) /// \note do equivalent for hw mode?
	{
		if(scrolled > 0)
			V_DrawScaledPatch(320 - scrolled, 0, 0, pat);

		if(scrolled < 320)
			V_DrawScaledPatch(-scrolled, 0, 0, pat);
	}
#endif
}

//
// F_Drawer
//
void F_Drawer(void)
{
	if(!finalestage)
		F_TextWrite();
	else
		V_DrawScaledPatch(0, 0, 0, W_CachePatchName("HELP2", PU_CACHE));
}

// De-Demo'd Title Screen
void F_TitleScreenDrawer(void)
{
	// Draw that sky!
	F_SkyScroll();

	V_DrawScaledPatch(30, 14, 0, ttwing);

	if(finalecount < 57)
	{
		if(finalecount == 35)
			V_DrawScaledPatch(115, 15, 0, ttspop1);
		else if(finalecount == 36)
			V_DrawScaledPatch(114, 15, 0,ttspop2);
		else if(finalecount == 37)
			V_DrawScaledPatch(113, 15, 0,ttspop3);
		else if(finalecount == 38)
			V_DrawScaledPatch(112, 15, 0,ttspop4);
		else if(finalecount == 39)
			V_DrawScaledPatch(111, 15, 0,ttspop5);
		else if(finalecount == 40)
			V_DrawScaledPatch(110, 15, 0, ttspop6);
		else if(finalecount >= 41 && finalecount <= 44)
			V_DrawScaledPatch(109, 15, 0, ttspop7);
		else if(finalecount >= 45 && finalecount <= 48)
			V_DrawScaledPatch(108, 12, 0, ttsprep1);
		else if(finalecount >= 49 && finalecount <= 52)
			V_DrawScaledPatch(107, 9, 0, ttsprep2);
		else if(finalecount >= 53 && finalecount <= 56)
			V_DrawScaledPatch(106, 6, 0, ttswip1);
		V_DrawScaledPatch(93, 106, 0, ttsonic);
	}
	else
	{
		V_DrawScaledPatch(93, 106, 0,ttsonic);
		if(finalecount/5 & 1)
			V_DrawScaledPatch(100, 3, 0,ttswave1);
		else
			V_DrawScaledPatch(100,3, 0,ttswave2);
	}

	V_DrawScaledPatch(48, 142, 0,ttbanner);
}

// Demo End Screen
//
// F_DemoEndDrawer
//
void F_DemoEndDrawer(void)
{
	patch_t* mouth;

	// anim the mouth
	if(--mouthtics <= 0)
	{
		mouthframe++;
		mouthtics = mouthframe->tics;
	}

	// Decide which mouth to project
	switch(mouthframe->frame)
	{
		case 1:
			mouth = W_CachePatchName("MOUTB0", PU_CACHE);
			break;
		case 2:
			mouth = W_CachePatchName("MOUTC0", PU_CACHE);
			break;
		case 3:
			mouth = W_CachePatchName("MOUTD0", PU_CACHE);
			break;
		case 4:
			mouth = W_CachePatchName("MOUTE0", PU_CACHE);
			break;
		default:
			mouth = W_CachePatchName("MOUTA0", PU_CACHE);
			break;
	}

	// advance animation
	finalecount++;
	finaletextcount++;

	if(finalestage == 1 && finalecount == 2)
		S_StartSound(NULL, sfx_annon1);

	V_DrawFill(0, 0, 184, 200, *((byte*)colormaps + 0xd7)); // Orange
	V_DrawFill(184, 0, 136, 200, *((byte*)colormaps + 0x04)); // White

	V_DrawScaledPatch(216, 36, 0, desonic); // Sonic

	// Have Sonic blink every so often. (animtimer is used for this)
	if(animtimer)
		animtimer--;

	if(M_Random() == 255 && animtimer == 0 && timetonext & 1)
		animtimer = 3;

	switch(animtimer)
	{
		case 3: // Start to blink..
		case 1: // Opening back up...
			V_DrawScaledPatch(230, 79, 0, deblink1);
			break;
		case 2: // Eyes shut
			V_DrawScaledPatch(230, 79, 0, deblink2);
			break;
		default:
			break;
	}

#define HANDX 189
#define HANDY 111

	// Sonic puts his hand out...
	if((finalestage == 1 && timetonext <= TICRATE/2) || (finalestage >= 2 && finalestage <= 4)
		|| (finalestage == 5 && timetonext > 3*TICRATE))
		V_DrawScaledPatch(HANDX, HANDY, 0, dehand3);
	else if(finalestage == 1 && timetonext >= TICRATE/2 + 1*NEWTICRATERATIO
		&& timetonext <= TICRATE/2 + 5*NEWTICRATERATIO)
		V_DrawScaledPatch(HANDX, HANDY, 0, dehand2);
	else if(finalestage == 1 && timetonext >= TICRATE/2 + 6*NEWTICRATERATIO
		&& timetonext <= TICRATE/2 + 8*NEWTICRATERATIO)
		V_DrawScaledPatch(HANDX, HANDY, 0, dehand1);

	// And brings it back in.
	if(finalestage == 5 && timetonext <= 3*TICRATE-1*NEWTICRATERATIO
		&& timetonext >= 3*TICRATE-3*NEWTICRATERATIO)
	{
		V_DrawScaledPatch(HANDX, HANDY, 0, dehand1);
	}

	// Tough part -- SYNCING THE MOUTH!
	// Animation is handled up above.
	V_DrawScaledPatch(254, 98, 0, mouth);

	// Draw the text over everything else
	switch(finalestage)
	{
		case 6:
		case 5:
			V_DrawString(8, 184, 0, "Returning to title screen...");
		case 4:
			V_DrawString(8, 152, 0, "3) Stop by the #srb2 chatroom\nat irc.esper.net.");
		case 3:
			V_DrawString(8, 120, 0, "2) Poke around the addons\nsection of the website.");
		case 2:
			V_DrawString(8, 80, 0, "1) Visit the SRB2 website\nat www.srb2.org for\nnews and updates.");
		case 1:
			V_DrawString(8, 4, 0, "Thanks for playing the SRB2\ndemo. This is the last release\nbefore the final game comes\nout, but here are several\nthings you you can do to\ntide you over:");
		default:
			break;
	}
}

#define INTERVAL 50
#define TRANSLEVEL V_8020TRANS

void F_GameEvaluationDrawer(void)
{
	int x, y;
	const int radius = 48;

	V_DrawFill(0, 0, vid.width, vid.height, 0);

	// Draw all the good crap here.
	if(animtimer == 64)
	{
		V_DrawString(114, 16, 0, "GOT THEM ALL!");

		if(gameskill <= sk_easy)
		{
			V_DrawCenteredString(BASEVIDWIDTH/2, 100, V_WHITEMAP, "Or have you? Play on Normal");
			V_DrawCenteredString(BASEVIDWIDTH/2, 116, V_WHITEMAP, "or higher to collect them all!");
		}
	}
	else
		V_DrawString(124, 16, 0, "TRY AGAIN!");

	finalestage++;
	timetonext = finalestage;

	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD1)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMGA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMGA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD2)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMOA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMOA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD3)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMPA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMPA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD4)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMBA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMBA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD5)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMRA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMRA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD6)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMLA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMLA0", PU_CACHE));

	timetonext += INTERVAL;
	x = (int)(160 + cos(timetonext * deg2rad) * radius);
	y = (int)(100 + sin(timetonext * deg2rad) * radius);

	if(emeralds & EMERALD7)
		V_DrawScaledPatch(x, y, 0, W_CachePatchName("CEMYA0", PU_CACHE));
	else
		V_DrawTranslucentPatch(x, y, TRANSLEVEL|V_TOPLEFT, W_CachePatchName("CEMYA0", PU_CACHE));

	if(finalestage >= 360)
		finalestage -= 360;

	if(finalecount == 5*TICRATE)
	{
		if((!modifiedgame || savemoddata) && mapheaderinfo[gamemap-1].nextlevel == 1102)
		{
			boolean alreadyplayed = false;
			int bitcount = 0;
			int i;

			if(!(emblemlocations[MAXEMBLEMS-2].collected))
			{
				emblemlocations[MAXEMBLEMS-2].collected = true; // For completing the game.
				S_StartSound(NULL, sfx_ncitem);
				alreadyplayed = true;
				drawemblem = true;
			}

			if(ALL7EMERALDS)
			{
				if(gameskill <= sk_easy)
					emeralds = 0; // Bye bye!
				else
				{
					if(!(emblemlocations[MAXEMBLEMS-1].collected))
					{
						emblemlocations[MAXEMBLEMS-1].collected = true;

						if(!alreadyplayed)
							S_StartSound(NULL, sfx_ncitem);

						drawchaosemblem = true;
					}
					grade |= 8; // Now you can access Mario!
				}
			}

			for(i=0; i<MAXEMBLEMS; i++)
			{
				if(emblemlocations[i].collected)
					bitcount++;
			}

			if(bitcount == numemblems) // Got ALL emblems!
				grade |= 16;

			grade |= 128; // Just for completing the game.
			grade |= 1;

			timesbeaten++;

			// An award for beating ultimate mode, perhaps?
			if(gameskill >= sk_insane)
				grade |= 1024;
		}
		else if(!modifiedgame && gamemap == 29) // Cleared NiGHTS
			grade |= 64; // Eh??
		else if(!modifiedgame && gamemap == 32) // Cleared Mario
			grade |= 32;
	}

	if(savemoddata)
		grade = 0;

	G_SaveGameData();

	if(finalecount >= 5*TICRATE)
	{
		if(drawemblem)
			V_DrawScaledPatch(160, 192, 0, W_CachePatchName("NWNGA0", PU_CACHE));

		if(drawchaosemblem)
			V_DrawScaledPatch(160, 192, 0, W_CachePatchName("NWNGA0", PU_CACHE));

		V_DrawString(8, 16, V_WHITEMAP, "Unlocked:");

		if(grade & 8)
			V_DrawString(8, 32, 0, "Mario");

		if(grade & 16)
			V_DrawString(8, 40, 0, "NiGHTS");

		if(grade & 32)
			V_DrawString(8, 48, 0, "Christmas Hunt");

		if(grade & 64)
			V_DrawString(8, 56, 0, "Adventure");

		if(grade & 128)
			V_DrawString(8, 64, 0, "Level Select");

		if(veryhardcleared)
			V_DrawString(8, 72, 0, "'Ultimate' Skill");

		if(grade & 256)
			V_DrawString(8, 80, 0, "Time Attack Reward");

		if(grade & 512)
			V_DrawString(8, 88, 0, "Easter Egg Reward");

		if(netgame)
			V_DrawString(8, 96, V_WHITEMAP, "Prizes only\nawarded in\nsingle player!");
		else if(modifiedgame)
			V_DrawString(8, 96, V_WHITEMAP, "Prizes not\nawarded in\nmodified games!");
	}
}

static void F_DrawCreditScreen(credit_t* creditpassed)
{
	int i, height = BASEVIDHEIGHT/(creditpassed->numnames+1);

	switch(animtimer)
	{
		case 1:
		case 2:
			V_DrawSmallScaledPatch(8, 112, 0, W_CachePatchName("CREDIT01", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 3:
		case 4:
			V_DrawSmallScaledPatch(240, 112, 0, W_CachePatchName("CREDIT02", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 5:
		case 6:
			V_DrawSmallScaledPatch(8, 0, 0, W_CachePatchName("CREDIT03", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 7:
		case 8:
			V_DrawSmallScaledPatch(8, 112, 0, W_CachePatchName("CREDIT04", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 9:
		case 10:
			V_DrawSmallScaledPatch(240, 8, 0, W_CachePatchName("CREDIT05", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 11:
		case 12:
			V_DrawSmallScaledPatch(120, 8, 0, W_CachePatchName("CREDIT06", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 13:
		case 14:
			V_DrawSmallScaledPatch(8, 100, 0, W_CachePatchName("CREDIT07", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 15:
		case 16:
			V_DrawSmallScaledPatch(8, 0, 0, W_CachePatchName("CREDIT08", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
		case 17:
		case 18:
			V_DrawSmallScaledPatch(112, 104, 0, W_CachePatchName("CREDIT09", PU_CACHE),
				(byte*)defaulttranslationtables - 256 + (12<<8));
			break;
	}

	if(creditpassed->numnames == 1) // Shift it up a bit
		height -= 16;

	V_DrawCreditString((BASEVIDWIDTH - V_CreditStringWidth(creditpassed->header))/2, height, 0,
		creditpassed->header);

	for(i = 0; i < creditpassed->numnames; i++)
	{
		if(cv_realnames.value)
			V_DrawCreditString((BASEVIDWIDTH - V_CreditStringWidth(creditpassed->realnames[i]))/2,
				height + (1+i)*24, 0, creditpassed->realnames[i]);
		else
			V_DrawCreditString((BASEVIDWIDTH - V_CreditStringWidth(creditpassed->fakenames[i]))/2,
				height + (1+i)*24, 0, creditpassed->fakenames[i]);
	}
}

void F_CreditDrawer(void)
{
	V_DrawFill(0, 0, vid.width, vid.height, 0);

	if(timetonext-- <= 0) // Fade to the next!
	{
		if(modcredits)
			timetonext = 165*NEWTICRATERATIO;
		else
			timetonext = 5*TICRATE-1;

		F_DrawCreditScreen(&credits[animtimer]);

		animtimer++;

		if(rendermode == render_soft)
		{
			tic_t nowtime, tics, wipestart;
			int y;
			boolean done;

			F_WipeStartScreen();
			V_DrawFill(0, 0, vid.width, vid.height, 0);
			F_DrawCreditScreen(&credits[animtimer]);
			F_WipeEndScreen(0, 0, vid.width, vid.height);

			wipestart = I_GetTime() - 1;
			y = wipestart + TICRATE; // init a timeout
			do
			{
				do
				{
					nowtime = I_GetTime();
					tics = nowtime - wipestart;
					if(!tics) I_Sleep();
				} while(!tics);
				wipestart = nowtime;
				done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
				I_OsPolling();
				I_UpdateNoBlit();
				M_Drawer(); // menu is drawn even on top of wipes
				I_FinishUpdate(); // page flip or blit buffer
			} while(!done && I_GetTime() < (unsigned)y);
		}
#ifdef HWRENDER
		else if(rendermode != render_none) // Delay the hardware modes as well
		{
			tic_t nowtime, tics, wipestart;
			int y;

			wipestart = I_GetTime() - 1;
			y = wipestart + 32; // init a timeout
			do
			{
				do
				{
					nowtime = I_GetTime();
					tics = nowtime - wipestart;
					if(!tics) I_Sleep();
				} while(!tics);

				I_OsPolling();
				I_UpdateNoBlit();
				M_Drawer(); // menu is drawn even on top of wipes
				I_FinishUpdate(); // page flip or blit buffer
			} while(I_GetTime() < (unsigned)y);
		}
#endif
	}
	else
		F_DrawCreditScreen(&credits[animtimer]);
}

//
// F_IntroDrawer
//
void F_IntroDrawer(void)
{
	if(timetonext)
		timetonext--;

	stoptimer++;

	if(timetonext == 1 && stoptimer > 0)
	{
		if(finaletext == E0TEXT)
		{
			S_ChangeMusic(mus_read_m, false);
			finaletext = E1TEXT;
		}
		else if(finaletext == E1TEXT)
			finaletext = E2TEXT;
		else if(finaletext == E2TEXT)
			finaletext = E3TEXT;
		else if(finaletext == E3TEXT)
		{
			finaletext = E4TEXT;
			mouthtics = BASEVIDWIDTH - 64;
		}
		else if(finaletext == E4TEXT)
			finaletext = C1TEXT;
		else if(finaletext == C1TEXT)
			finaletext = C2TEXT;
		else if(finaletext == C2TEXT)
			finaletext = C3TEXT;
		else if(finaletext == C3TEXT)
			finaletext = C4TEXT;
		else if(finaletext == C4TEXT)
			finaletext = C5TEXT;
		else if(finaletext == C5TEXT)
			finaletext = C6TEXT;
		else if(finaletext == C6TEXT)
		{
			// Play BOOM sound here

			if(rendermode == render_soft)
			{
				tic_t nowtime;
				tic_t tics;
				tic_t wipestart;
				int y;
				boolean done;

				F_WipeStartScreen();
				V_DrawFill(0, 0, vid.width, vid.height, 4);
				F_WipeEndScreen(0, 0, vid.width, vid.height);

				wipestart = I_GetTime() - 1;
				y = wipestart + TICRATE; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);
					wipestart = nowtime;
					done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(!done && I_GetTime() < (unsigned)y);
			}
#ifdef HWRENDER
			else if(rendermode != render_none) // Delay the hardware modes as well
			{
				tic_t nowtime;
				tic_t tics;
				tic_t wipestart;
				int y;

				wipestart = I_GetTime() - 1;
				y = wipestart + 32; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(I_GetTime() < (unsigned)y);
			}
#endif

			finaletext = T1TEXT;
		}
		else if(finaletext == T1TEXT)
			finaletext = T2TEXT;
		else if(finaletext == T2TEXT)
			finaletext = T3TEXT;
		else if(finaletext == T3TEXT)
			finaletext = T4TEXT;
		else if(finaletext == T4TEXT)
			finaletext = T5TEXT;
		else if(finaletext == T5TEXT)
		{
			if(rendermode == render_soft)
			{
				tic_t nowtime, tics, wipestart;
				int y;
				boolean done;

				F_WipeStartScreen();
				V_DrawFill(0, 0, vid.width, vid.height, 0);
				F_WipeEndScreen(0, 0, vid.width, vid.height);

				wipestart = I_GetTime() - 1;
				y = wipestart + TICRATE; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);
					wipestart = nowtime;
					done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(!done && I_GetTime() < (unsigned)y);
			}
#ifdef HWRENDER
			else if(rendermode != render_none) // Delay the hardware modes as well
			{
				tic_t nowtime;
				tic_t tics;
				tic_t wipestart;
				int y;

				wipestart = I_GetTime() - 1;
				y = wipestart + 32; // init a timeout
				do
				{
					do
					{
						nowtime = I_GetTime();
						tics = nowtime - wipestart;
						if(!tics) I_Sleep();
					} while(!tics);

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // page flip or blit buffer
				} while(I_GetTime() < (unsigned)y);
			}
#endif
			// Stay on black for a bit. =)
			{
				tic_t time;
				time = I_GetTime() + TICRATE*2; // Shortened the quit time, used to be 2 seconds
				while(time > I_GetTime())
				{
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
				}
			}

			D_StartTitle();
			return;
		}

		if(gamestate == GS_INTRO)
			gamestate = GS_INTRO2;
		else
			gamestate = GS_INTRO;

		if(rendermode == render_soft)
			F_WipeStartScreen();

		wipegamestate = -1;
		finaletextcount = 0;
		timetonext = 0;
		animtimer = 0;
		stoptimer = 0;
	}

	if(finaletext == C3TEXT && timetonext == 4*TICRATE) // Force a wipe here
	{
		if(gamestate == GS_INTRO)
			gamestate = GS_INTRO2;
		else
			gamestate = GS_INTRO;

		if(rendermode == render_soft)
			F_WipeStartScreen();

		wipegamestate = -1;
	}

	F_IntroTextWrite();
}

static void F_AdvanceToNextScene(void)
{
	scenenum++;

	if(scenenum < cutscenes[cutnum].numscenes)
	{
		picnum = 0;
		picxpos = cutscenes[cutnum].scene[scenenum].xcoord[picnum];
		picypos = cutscenes[cutnum].scene[scenenum].ycoord[picnum];
	}

	if(cutscenes[cutnum].scene[scenenum].musicslot != 0)
		S_ChangeMusic(cutscenes[cutnum].scene[scenenum].musicslot, false);

	if(rendermode == render_soft)
	{
		tic_t nowtime, tics, wipestart;
		int y;
		boolean done;

		F_WipeStartScreen();
		V_DrawFill(0,0, vid.width, vid.height, 0);
		if(scenenum < cutscenes[cutnum].numscenes)
		{
			if(cutscenes[cutnum].scene[scenenum].pichires[picnum])
			{
				byte* colormap;
				int flags = 0; /// \bug Graue 03-22-2004: something very wrong here

				flags = (flags & ~MF_TRANSLATION) | (12<<MF_TRANSSHIFT);

				colormap = (byte*)defaulttranslationtables - 256
					+ ((flags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));

				V_DrawSmallScaledPatch(picxpos, picypos, 0, W_CachePatchName(cutscenes[cutnum].scene[scenenum].picname[picnum], PU_CACHE), colormap);
			}
			else
				V_DrawScaledPatch(picxpos, picypos, 0, W_CachePatchName(cutscenes[cutnum].scene[scenenum].picname[picnum], PU_CACHE));
		}
		F_WipeEndScreen(0, 0, vid.width, vid.height);

		wipestart = I_GetTime() - 1;
		y = wipestart + TICRATE; // init a timeout
		do
		{
			do
			{
				nowtime = I_GetTime();
				tics = nowtime - wipestart;
				if(!tics) I_Sleep();
			} while(!tics);
			wipestart = nowtime;
			done = F_ScreenWipe(0, 0, vid.width, vid.height, tics);
			I_OsPolling();
			I_UpdateNoBlit();
			M_Drawer(); // menu is drawn even on top of wipes
			I_FinishUpdate(); // page flip or blit buffer
		} while(!done && I_GetTime() < (unsigned)y);
	}
#ifdef HWRENDER
	else if(rendermode != render_none) // Delay the hardware modes as well
	{
		tic_t nowtime, tics, wipestart;
		int y;

		wipestart = I_GetTime() - 1;
		y = wipestart + 32; // init a timeout
		do
		{
			do
			{
				nowtime = I_GetTime();
				tics = nowtime - wipestart;
				if(!tics) I_Sleep();
			} while(!tics);

			I_OsPolling();
			I_UpdateNoBlit();
			M_Drawer(); // menu is drawn even on top of wipes
			I_FinishUpdate(); // page flip or blit buffer
		} while(I_GetTime() < (unsigned)y);
	}
#endif

	finaletextcount = 0;
	timetonext = 0;
	stoptimer = 0;

	if(scenenum >= cutscenes[cutnum].numscenes)
	{
		F_EndCutScene();
		return;
	}

	finaletext = cutscenes[cutnum].scene[scenenum].text;

	picnum = 0;
	picxpos = cutscenes[cutnum].scene[scenenum].xcoord[picnum];
	picypos = cutscenes[cutnum].scene[scenenum].ycoord[picnum];
	textxpos = cutscenes[cutnum].scene[scenenum].textxpos;
	textypos = cutscenes[cutnum].scene[scenenum].textypos;

	animtimer = pictime = cutscenes[cutnum].scene[scenenum].picduration[picnum];
}

static void F_CutsceneTextWrite(void)
{
	V_DrawFill(0, 0, vid.width, vid.height, 0);

	if(cutscenes[cutnum].scene[scenenum].picname[picnum][0] != '\0')
	{
		if(cutscenes[cutnum].scene[scenenum].pichires[picnum])
		{
			byte* colormap;
			int flags = 0; /// \bug Graue 03-22-2004: something wrong here too

			flags = (flags & ~MF_TRANSLATION) | (12<<MF_TRANSSHIFT);

			colormap = (byte*)defaulttranslationtables - 256
				+ ((flags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));

			V_DrawSmallScaledPatch(picxpos, picypos, 0,
				W_CachePatchName(cutscenes[cutnum].scene[scenenum].picname[picnum], PU_CACHE),
				colormap);
		}
		else
			V_DrawScaledPatch(picxpos,picypos, 0,
				W_CachePatchName(cutscenes[cutnum].scene[scenenum].picname[picnum], PU_CACHE));
	}

	if(animtimer)
	{
		animtimer--;
		if(animtimer <= 0)
		{
			if(picnum < 7
				&& cutscenes[cutnum].scene[scenenum].picname[picnum+1][0] != '\0')
			{
				picnum++;
				picxpos = cutscenes[cutnum].scene[scenenum].xcoord[picnum];
				picypos = cutscenes[cutnum].scene[scenenum].ycoord[picnum];
				pictime = cutscenes[cutnum].scene[scenenum].picduration[picnum];
				animtimer = pictime;
			}
			else
				timetonext = 2;
		}
	}

	F_WriteCutsceneText();
}

void F_EndCutScene(void)
{
	if(runningprecutscene && server)
	{
		D_MapChange(gamemap, gametype, gameskill, precutresetplayer, 0, true);
	}
	else
	{
		if(cutnum == introtoplay-1)
		{
			D_StartTitle();
			return;
		}

		if(nextmap == 1102-1)
		{
			if(!modifiedgame && gameskill == sk_nightmare) // Very Hard cleared!
				veryhardcleared = true;

			if(gametype == GT_COOP)
				F_StartCredits();
			else
				D_StartTitle();
		}
		else if(nextmap == 1101-1) // Cut to the chase, captain.
		{
			if(gametype == GT_COOP)
				F_StartGameEvaluation();
			else
				D_StartTitle();
		}
		else if(nextmap == 1100-1) // Cut to the chase, captain.
			D_StartTitle();
		else
			G_NextLevel();
	}
}

//
// F_CutsceneDrawer
//
void F_CutsceneDrawer(void)
{
	if(timetonext)
		timetonext--;

	stoptimer++;

	if(timetonext == 1 && stoptimer > 0)
		F_AdvanceToNextScene();

	F_CutsceneTextWrite();
}
