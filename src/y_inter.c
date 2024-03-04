// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2004-2005 by Sonic Team Junior.
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
/// \brief Intermission

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_net.h"
#include "i_video.h"
#include "p_tick.h"
#include "r_defs.h"
#include "r_things.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"

#include "r_local.h"
#include "p_local.h"

typedef union
{
	struct
	{
		int score; // fake score
		int timebonus, ringbonus, total;
		int min, sec, tics;
		patch_t* ttlnum; // act number being displayed
		patch_t* ptimebonus; // TIME BONUS
		patch_t* pringbonus; // RING BONUS
		patch_t* ptotal; // TOTAL
		char passed1[13]; // KNUCKLES GOT
		char passed2[12]; // THROUGH ACT
		int passedx1, passedx2;
		int gotlife; // Player # that got an extra life
	} coop;

	struct
	{
		int score; // fake score
		int ringbonus;
		int headx;
		patch_t* cemerald; // CHAOS EMERALDS (or GOT THEM ALL!)
		patch_t* nowsuper; // SONIC CAN NOW BE SUPER SONIC
		patch_t* pringbonus; // RING BONUS
		patch_t* cscore; // SCORE
	} spec;

	struct
	{
		int scores[MAXPLAYERS]; // Winner's score
		int* color[MAXPLAYERS]; // Winner's color #
		int* character[MAXPLAYERS]; // Winner's character #
		int num[MAXPLAYERS]; // Winner's player #
		char* name[MAXPLAYERS]; // Winner's name
		patch_t* result; // RESULT
		patch_t* blueflag;
		patch_t* redflag; // int_ctf uses this struct too.
		int numplayers; // Number of players being displayed
		char levelstring[40]; // holds levelnames up to 32 characters
	} match;

	struct
	{
		int scores[4]; // player scores
		int timemin[4]; // time (minutes)
		int timesec[4]; // time (seconds)
		int timetic[4]; // time (tics)
		int rings[4]; // rings
		int totalrings[4]; // total rings
		int itemboxes[4]; // item boxes
		int totalwins[4]; // how many wins each player has
		int numplayersshown; // how many players are being displayed (1-4)
		int playersshown[4]; // the player numbers of these players
		const char *winnerstrings[5]; // string for the winner in each category
		int winner; // the overall winner's player number
		patch_t* result; // RESULT
		char levelstring[40]; // holds levelnames up to 32 characters
	} race;

} y_data;

static y_data data;

// graphics
static patch_t* bgpatch = NULL;     // INTERSCR
static patch_t* widebgpatch = NULL; // INTERSCW
static patch_t* bgtile = NULL;      // SPECTILE/SRB2BACK
static patch_t* interpic = NULL;    // a custom picture defined in the map header
static boolean usetile;
static boolean useinterpic;
static int timer;
static boolean gottimebonus;

static int intertic;
static int endtic = -1;

static enum
{
	int_none,
	int_coop,     // Single Player/Cooperative
	int_match,    // Match
	int_teammatch,// Team Match
//	int_tag,      // Tag
	int_ctf,      // CTF
//	int_chaos,    // Chaos
	int_spec,     // Special Stage
	int_race,     // Race
	int_timerace, // Race (Time Only)
} inttype = int_none;

static void Y_AwardCoopBonuses(void);
static void Y_AwardSpecialStageBonus(void);
static void Y_CalculateRaceWinners(void);
static void Y_CalculateTimeRaceWinners(void);
static void Y_CalculateMatchWinners(void);
static void Y_DrawScaledNum(int x, int y, int flags, int num);
#define Y_DrawNum(x,y,n) Y_DrawScaledNum(x, y, 0, n)
static void Y_FollowIntermission(void);
static void Y_UnloadData(void);

//
// Y_IntermissionDrawer
//
// Called by D_Display. Nothing is modified here; all it does is draw. Neat concept, huh?
//
void Y_IntermissionDrawer(void)
{
	if(inttype == int_none || rendermode == render_none)
		return;

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);

	if(useinterpic)
		V_DrawScaledPatch(0, 0, 0, interpic);
	else if(!usetile)
	{
		if(widebgpatch && rendermode == render_soft && vid.width / vid.dupx == 400)
			V_DrawScaledPatch(0, 0, V_SNAPTOLEFT, widebgpatch);
		else
			V_DrawScaledPatch(0, 0, 0, bgpatch);
	}
	else
		V_DrawPatchFill(bgtile);

	if(inttype == int_coop)
	{
		// draw score
		V_DrawScaledPatch(hudinfo[HUD_SCORE].x, hudinfo[HUD_SCORE].y, V_SNAPTOLEFT, sboscore);
		Y_DrawScaledNum(hudinfo[HUD_SCORENUM].x, hudinfo[HUD_SCORENUM].y, V_SNAPTOLEFT, data.coop.score);

		// draw time
		V_DrawScaledPatch(hudinfo[HUD_TIME].x, hudinfo[HUD_TIME].y, V_SNAPTOLEFT, sbotime);
		if(cv_timetic.value)
			Y_DrawScaledNum(hudinfo[HUD_SECONDS].x, hudinfo[HUD_SECONDS].y, V_SNAPTOLEFT, data.coop.tics);
		else
		{
			if(data.coop.sec < 10)
				Y_DrawScaledNum(hudinfo[HUD_LOWSECONDS].x, hudinfo[HUD_LOWSECONDS].y, V_SNAPTOLEFT, 0);
			Y_DrawScaledNum(hudinfo[HUD_SECONDS].x, hudinfo[HUD_SECONDS].y, V_SNAPTOLEFT, data.coop.sec);
			Y_DrawScaledNum(hudinfo[HUD_MINUTES].x, hudinfo[HUD_MINUTES].y, V_SNAPTOLEFT, data.coop.min);
			V_DrawScaledPatch(hudinfo[HUD_TIMECOLON].x, hudinfo[HUD_TIMECOLON].y, V_SNAPTOLEFT, sbocolon);
		}

		// draw the "got through act" lines and act number
		V_DrawLevelTitle(data.coop.passedx1, 49, 0, data.coop.passed1);
		V_DrawLevelTitle(data.coop.passedx2, 49+V_LevelNameHeight(data.coop.passed2)+2, 0, data.coop.passed2);
		
		if(mapheaderinfo[gamemap-1].actnum)
			V_DrawScaledPatch(244, 57, 0, data.coop.ttlnum);

		V_DrawScaledPatch(68, 84 + 3*tallnum[0]->height/2, 0, data.coop.ptimebonus);
		Y_DrawNum(BASEVIDWIDTH - 68, 85 + 3*tallnum[0]->height/2, data.coop.timebonus);
		V_DrawScaledPatch(68, 84 + 3*tallnum[0]->height, 0, data.coop.pringbonus);
		Y_DrawNum(BASEVIDWIDTH - 68, 85 + 3*tallnum[0]->height, data.coop.ringbonus);
		V_DrawScaledPatch(88, 84 + 6*tallnum[0]->height, 0, data.coop.ptotal);
		Y_DrawNum(BASEVIDWIDTH - 68, 85 + 6*tallnum[0]->height, data.coop.total);

		if(gottimebonus && endtic != -1)
			V_DrawCenteredString(BASEVIDWIDTH/2, 136, V_WHITEMAP, "TIME BONUS UNLOCKED!");
	}
	else if(inttype == int_spec)
	{
		// draw the header
		if(endtic != -1 && ALL7EMERALDS && data.spec.nowsuper != NULL)
			V_DrawScaledPatch(48, 32, 0, data.spec.nowsuper);
		else
			V_DrawScaledPatch(data.spec.headx, 26, 0, data.spec.cemerald);

		// draw the emeralds
		if(intertic & 1)
		{
			if(emeralds & EMERALD1)
				V_DrawScaledPatch(80, 92, 0, emerald1);
			if(emeralds & EMERALD2)
				V_DrawScaledPatch(104, 92, 0, emerald2);
			if(emeralds & EMERALD3)
				V_DrawScaledPatch(128, 92, 0, emerald3);
			if(emeralds & EMERALD4)
				V_DrawScaledPatch(152, 92, 0, emerald4);
			if(emeralds & EMERALD5)
				V_DrawScaledPatch(176, 92, 0, emerald5);
			if(emeralds & EMERALD6)
				V_DrawScaledPatch(200, 92, 0, emerald6);
			if(emeralds & EMERALD7)
				V_DrawScaledPatch(224, 92, 0, emerald7);
			if(emeralds & EMERALD8)
				V_DrawScaledPatch(248, 92, 0, emerald8);
		}

		V_DrawScaledPatch(80, 132, 0, data.spec.pringbonus);
		Y_DrawNum(232, 133, data.spec.ringbonus);
		V_DrawScaledPatch(80, 148, 0, data.spec.cscore);
		Y_DrawNum(232, 149, data.spec.score);
	}
	else if(inttype == int_teammatch)
	{
		int scorelines;
		int whiteplayer;
		playersort_t tab[MAXPLAYERS];

		whiteplayer = demoplayback ? displayplayer : consoleplayer;

		// draw the header
		V_DrawScaledPatch(112, 2, 0, data.match.result);

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, 24, 0, data.match.levelstring);

		V_DrawFill(4, 42, 312, 1, 4);

		scorelines = HU_CreateTeamScoresTbl(tab, NULL);

		if(scorelines > 9)
			scorelines = 9; // only show 10 best teams

		// Code reuse - it's what OOP is all about, right?
		HU_DrawTabRankings(40, 48, tab, scorelines, whiteplayer);
	}
	else if(inttype == int_match
		|| inttype == int_timerace || inttype == int_ctf)
	{
		int i;
		int x = 4;
		int y = 48;
		byte* colormap;
		char name[MAXPLAYERNAME];

		// In CTF, show the team flags and the team score
		// at the top instead of "RESULTS"
		if(inttype == int_ctf)
		{
			V_DrawSmallScaledPatch(128 - data.match.blueflag->width/4, 2, 0, data.match.blueflag, colormaps);
			V_DrawCenteredString(128, 16, 0, va("%i", bluescore));

			V_DrawSmallScaledPatch(192 - data.match.redflag->width/4, 2, 0, data.match.redflag, colormaps);
			V_DrawCenteredString(192, 16, 0, va("%i", redscore));
		}
		else
		{
			// draw the header
			V_DrawScaledPatch(112, 2, 0, data.match.result);
		}

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, inttype == int_ctf ? 24 : 20, 0, data.match.levelstring);

		V_DrawFill(4, 42, 312, 1, 4);

		if(data.match.numplayers > 9)
		{
			V_DrawFill(160, 32, 1, 152, 4);

			if(inttype == int_timerace)
				V_DrawRightAlignedString(x+152, 32, V_WHITEMAP, "TIME");
			else
				V_DrawRightAlignedString(x+152, 32, V_WHITEMAP, "SCORE");

			V_DrawCenteredString(x+(BASEVIDWIDTH/2)+6, 32, V_WHITEMAP, "#");
			V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_WHITEMAP, "NAME");
		}

		V_DrawCenteredString(x+6, 32, V_WHITEMAP, "#");
		V_DrawString(x+36, 32, V_WHITEMAP, "NAME");

		if(inttype == int_timerace)
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_WHITEMAP, "TIME");
		else
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_WHITEMAP, "SCORE");
		
		for(i=0; i<data.match.numplayers; i++)
		{
			V_DrawCenteredString(x+6, y, 0, va("%i", i+1));

			if(playeringame[data.match.num[i]])
			{
				if(data.match.color[i]==0)
					colormap = colormaps;
				else
					colormap = (byte *) translationtables[*data.match.character[i]] - 256 + (*data.match.color[i]<<8);

				V_DrawSmallScaledPatch (x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap); // Tails 03-11-2000
				
				strncpy(name, data.match.name[i], data.match.numplayers > 9 ? 8 : MAXPLAYERNAME);
				V_DrawString(x+36, y, 0, name);

				if(data.match.numplayers > 9)
				{
					if(inttype == int_match || inttype == int_ctf) 
						V_DrawRightAlignedString(x+152, y, 0, va("%i", data.match.scores[i]));
					else if(inttype == int_timerace)
					{
						char time[10];

						sprintf(time, "%d:%02d.%02d", data.match.scores[i]/(60*TICRATE),
								data.match.scores[i]/TICRATE % 60, data.match.scores[i]%TICRATE);
						V_DrawRightAlignedString(x+152, y, 0, time);
					}
				}
				else
				{
					if(inttype == int_match || inttype == int_ctf)
						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, va("%i", data.match.scores[i]));
					else if(inttype == int_timerace)
					{
						char time[10];

						sprintf(time, "%d:%02d.%02d", data.match.scores[i]/(60*TICRATE),
								data.match.scores[i]/TICRATE % 60, data.match.scores[i]%TICRATE);
						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, time);
					}
				}
			}

			y += 16;

			if(y > 176)
			{
				y = 48;
				x += BASEVIDWIDTH/2;
			}
		}
	}
	else if(inttype == int_race)
	{
		char name[9] = "xxxxxxxx";
		int i;

		// draw the header
		V_DrawScaledPatch(112, 8, 0, data.race.result);

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, 28, 0, data.race.levelstring);

		// draw the category names
		V_DrawString(8, 66, 0, "SCORE");
		V_DrawString(8, 78, 0, "TIME");
		V_DrawString(8, 90, 0, "RING");
		V_DrawString(8, 102, 0, "TOT. RING");
		V_DrawString(8, 114, 0, "ITEM BOX");
		V_DrawString(0, 138, 0, "* TOTAL *");

		// draw the W
		V_DrawCharacter(304, 50, 'W');

		// draw the winner in each category
		V_DrawCenteredString(308, 66, V_WHITEMAP, data.race.winnerstrings[0]);
		V_DrawCenteredString(308, 78, V_WHITEMAP, data.race.winnerstrings[1]);
		V_DrawCenteredString(308, 90, V_WHITEMAP, data.race.winnerstrings[2]);
		V_DrawCenteredString(308, 102, V_WHITEMAP, data.race.winnerstrings[3]);
		V_DrawCenteredString(308, 114, V_WHITEMAP, data.race.winnerstrings[4]);

		// draw the overall winner
		if(data.race.winner == -1)
			V_DrawCenteredString(BASEVIDWIDTH/2, 163, V_WHITEMAP, "TIED");
		else
		{
			V_DrawCenteredString(BASEVIDWIDTH/2, 159, V_WHITEMAP,
				player_names[data.race.winner]);
			V_DrawCenteredString(BASEVIDWIDTH/2, 167, V_WHITEMAP, "WINS");
		}

		for(i = 0; i < data.race.numplayersshown; i++)
		{
			// draw the player's name (max 8 chars)
			strncpy(name, player_names[data.race.playersshown[i]], 8);
			V_DrawRightAlignedString(104 + 64*i, 46, 0, name);

			// draw 1P/2P/3P/4P
			name[2] = '\0';
			name[1] = 'P';
			name[0] = (char)('1' + (char)i);
			V_DrawRightAlignedString(104 + 64*i, 54, 0, name);

			// draw score
			sprintf(name, "%d", data.race.scores[i]);
			V_DrawRightAlignedString(104 + 64*i, 66, 0, name);

			// draw time
			sprintf(name, "%d:%02d.%02d", data.race.timemin[i],
				data.race.timesec[i], data.race.timetic[i]);
			V_DrawRightAlignedString(104 + 64*i, 78, 0, name);

			// draw ring count
			sprintf(name, "%d", data.race.rings[i]);
			V_DrawRightAlignedString(104 + 64*i, 90, 0, name);

			// draw total ring count
			sprintf(name, "%d", data.race.totalrings[i]);
			V_DrawRightAlignedString(104 + 64*i, 102, 0, name);

			// draw item box count
			sprintf(name, "%d", data.race.itemboxes[i]);
			V_DrawRightAlignedString(104 + 64*i, 114, 0, name);

			// draw total number of wins
			sprintf(name, "%d", data.race.totalwins[i]);
			V_DrawRightAlignedString(104 + 64*i, 138, 0, name);
		}
	}

	if(timer)
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, V_WHITEMAP,
			va("start in %d seconds", timer/TICRATE));
}

//
// Y_Ticker
//
// Manages fake score tally for single player end of act, and decides when intermission is over.
//
void Y_Ticker(void)
{
	if(inttype == int_none)
		return;

	if(paused)
		return;

	intertic++;

	// multiplayer uses timer (based on cv_inttime)
	if(timer)
	{
		if(!--timer)
		{
			Y_EndIntermission();
			Y_FollowIntermission();
			return;
		}
	}
	// single player is hardcoded to go away after awhile
	else if(intertic == endtic)
	{
		Y_EndIntermission();
		Y_FollowIntermission();
		return;
	}

	if(endtic != -1)
		return; // tally is done

	if(inttype == int_coop) // coop or single player, normal level
	{
		if(!intertic) // first time only
			S_ChangeMusic(mus_lclear, false); // don't loop it

		if(intertic < TICRATE) // one second pause before tally begins
			return;

		if(data.coop.ringbonus || data.coop.timebonus)
		{
			int i;
			boolean skip = false;

			if(!(intertic & 1))
				S_StartSound(NULL, sfx_menu1); // tally sound effect

			for(i=0; i<MAXPLAYERS; i++)
			{
				if(players[i].cmd.buttons & BT_USE)
				{
					skip = true;
					break;
				}
			}

			// ring and time bonuses count down by 222 each tic
			if(data.coop.ringbonus)
			{
				data.coop.ringbonus -= 222;
				data.coop.total += 222;
				data.coop.score += 222;
				if(data.coop.ringbonus < 0 || skip == true) // went too far
				{
					data.coop.score += data.coop.ringbonus;
					data.coop.total += data.coop.ringbonus;
					data.coop.ringbonus = 0;

					if(skip == true && (data.coop.gotlife == consoleplayer || data.coop.gotlife == secondarydisplayplayer))
					{
						// lives are already added since tally is fake, but play the music
						if(mariomode)
							S_StartSound(NULL, sfx_marioa);
						else
						{
							S_StopMusic(); // otherwise it won't restart if this is done twice in a row
							S_ChangeMusic(mus_xtlife, false);
						}
					}
				}
			}
			if(data.coop.timebonus)
			{
				data.coop.timebonus -= 222;
				data.coop.total += 222;
				data.coop.score += 222;
				if(data.coop.timebonus < 0 || skip == true)
				{
					data.coop.score += data.coop.timebonus;
					data.coop.total += data.coop.timebonus;
					data.coop.timebonus = 0;
				}
			}

			if(!data.coop.timebonus && !data.coop.ringbonus)
			{
				endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
				S_StartSound(NULL, sfx_chchng); // cha-ching!
			}

			if(data.coop.score % 50000 < 222) // just passed a 50000 point mark
			{
				// lives are already added since tally is fake, but play the music
				if(mariomode)
					S_StartSound(NULL, sfx_marioa);
				else
				{
					S_StopMusic(); // otherwise it won't restart if this is done twice in a row
					S_ChangeMusic(mus_xtlife, false);
				}
			}
		}
		else
		{
			endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
			S_StartSound(NULL, sfx_chchng); // cha-ching!
		}
	}
	else if(inttype == int_spec) // coop or single player, special stage
	{
		if(!intertic) // first time only
			S_ChangeMusic(mus_lclear, false); // don't loop it

		if(intertic < TICRATE) // one second pause before tally begins
			return;

		if(data.spec.ringbonus)
		{
			int i;
			boolean skip = false;

			if(!(intertic & 1))
				S_StartSound(NULL, sfx_menu1); // tally sound effect

			for(i=0; i<MAXPLAYERS; i++)
			{
				if(players[i].cmd.buttons & BT_USE)
				{
					skip = true;
					break;
				}
			}

			// ring bonus counts down by 222 each tic
			data.spec.ringbonus -= 222;
			data.spec.score += 222;
			if(data.spec.ringbonus < 0 || skip == true) // went too far
			{
				data.spec.score += data.spec.ringbonus;
				data.spec.ringbonus = 0;

				if(skip == true && (data.coop.gotlife == consoleplayer || data.coop.gotlife == secondarydisplayplayer))
				{
					// lives are already added since tally is fake, but play the music
					if(mariomode)
						S_StartSound(NULL, sfx_marioa);
					else
					{
						S_StopMusic(); // otherwise it won't restart if this is done twice in a row
						S_ChangeMusic(mus_xtlife, false);
					}
				}
			}

			if(!data.spec.ringbonus)
			{
				endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
				S_StartSound(NULL, sfx_chchng); // cha-ching!
			}

			if(data.spec.score % 50000 < 222 && skip == false) // just passed a 50000 point mark
			{
				// lives are already added since tally is fake, but play the music
				if(mariomode)
					S_StartSound(NULL, sfx_marioa);
				else
				{
					S_StopMusic(); // otherwise it won't restart if this is done twice in a row
					S_ChangeMusic(mus_xtlife, false);
				}
			}
		}
		else
		{
			endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
			S_StartSound(NULL, sfx_chchng); // cha-ching!
		}
	}
	else if(inttype == int_match || inttype == int_ctf || inttype == int_teammatch) // match
	{
		if(!intertic) // first time only
			S_ChangeMusic(mus_racent, true); // loop it
	}
	else if(inttype == int_race || inttype == int_timerace) // race
	{
		if(!intertic) // first time only
			S_ChangeMusic(mus_racent, true); // loop it
	}
}

//
// Y_StartIntermission
//
// Called by G_DoCompleted. Sets up data for intermission drawer/ticker.
//
void Y_StartIntermission(void)
{
	intertic = -1;

#ifdef PARANOIA
	if(endtic != -1)
		I_Error("endtic is dirty");
#endif

	if(!multiplayer)
	{
		timer = 0;

		if(gamemap >= sstage_start && gamemap <= sstage_end)
			inttype = int_spec;
		else
			inttype = int_coop;
	}
	else
	{
		if(cv_inttime.value == 0 && gametype == GT_COOP)
			timer = 0;
		else
		{
			timer = cv_inttime.value*TICRATE;

			if(!timer)
				timer = 1;
		}

		if(gametype == GT_COOP)
		{
			if(gamemap >= sstage_start && gamemap <= sstage_end)
				inttype = int_spec;
			else
				inttype = int_coop;
		}
		else if(gametype == GT_MATCH || gametype == GT_CHAOS
			|| gametype == GT_TAG)
		{
			if(gametype != GT_TAG && cv_teamplay.value) // Team Match
				inttype = int_teammatch;
			else
				inttype = int_match;
		}
		else if(gametype == GT_RACE)
		{
			if(cv_racetype.value) // Time Only
				inttype = int_timerace;
			else // Full
				inttype = int_race;
		}
		else if(gametype == GT_CTF)
		{
			inttype = int_ctf;
		}
	}

	switch(inttype)
	{
		case int_coop: // coop or single player, normal level
		{
			// award time and ring bonuses
			Y_AwardCoopBonuses();

			// setup time data
			data.coop.tics = players[consoleplayer].realtime; // used if cv_timetic is on
			data.coop.sec = players[consoleplayer].realtime / TICRATE;
			data.coop.min = data.coop.sec / 60;
			data.coop.sec %= 60;

			gottimebonus = false;

			if((!modifiedgame || savemoddata) && !multiplayer)
			{
				if(((unsigned)players[consoleplayer].realtime < timedata[gamemap-1].time) ||
					(timedata[gamemap-1].time == 0))
					timedata[gamemap-1].time = players[consoleplayer].realtime;

				if(!savemoddata && !(grade & 256))
				{
					int seconds = 0;
					int i;

					for(i=0; i<9; i++)
					{
						if(timedata[i].time > 0)
							seconds += timedata[i].time;
						else
							seconds += 800*TICRATE;
					}

					seconds /= TICRATE;

					if(seconds <= 390)
						gottimebonus = true;
				}
				G_SaveGameData();
			}

			// get act number
			if(mapheaderinfo[prevmap].actnum)
				data.coop.ttlnum = W_CachePatchName(va("TTL%.2d", mapheaderinfo[prevmap].actnum),
					PU_STATIC);
			else
				data.coop.ttlnum = W_CachePatchName("TTL01", PU_STATIC);

			// get background patches
			widebgpatch = W_CachePatchName("INTERSCW", PU_STATIC);
			bgpatch = W_CachePatchName("INTERSCR", PU_STATIC);

			// grab an interscreen if appropriate
			if(mapheaderinfo[gamemap-1].interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1].interscreen, PU_STATIC);
				useinterpic = true;
			}
			else
				useinterpic = false;
			usetile = false;

			// get single player specific patches
			data.coop.ptotal = W_CachePatchName("YTOTAL", PU_STATIC);
			data.coop.ptimebonus = W_CachePatchName("YTMBONUS", PU_STATIC);
			data.coop.pringbonus = W_CachePatchName("YRINGBNS", PU_STATIC);

			// set up the "got through act" message according to skin name
			if(strlen(skins[players[consoleplayer].skin].name) <= 8)
			{
				sprintf(data.coop.passed1, "%s GOT", skins[players[consoleplayer].skin].name);
				data.coop.passedx1 = 62 + (176 - V_LevelNameWidth(data.coop.passed1))/2;
				strcpy(data.coop.passed2, "THROUGH ACT");
				data.coop.passedx2 = 62 + (176 - V_LevelNameWidth(data.coop.passed2))/2;
				// The above value is not precalculated because it needs only be computed once
				// at the start of intermission, and precalculating it would preclude mods
				// changing the font to one of a slightly different width.
			}
			else
			{
				strcpy(data.coop.passed1, skins[players[consoleplayer].skin].name);
				data.coop.passedx1 = 62 + (176 - V_LevelNameWidth(data.coop.passed1))/2;
				strcpy(data.coop.passed2, "PASSED ACT");
				data.coop.passedx2 = 62 + (176 - V_LevelNameWidth(data.coop.passed2))/2;
			}
			break;
		}

		case int_match:
		case int_teammatch:
		{
			// Calculate who won
			Y_CalculateMatchWinners();

			// set up the levelstring
			if(mapheaderinfo[prevmap].actnum)
				sprintf(data.match.levelstring, "%.32s * %d *",
					mapheaderinfo[prevmap].lvlttl, mapheaderinfo[prevmap].actnum);
			else
				sprintf(data.match.levelstring, "* %.32s *",
					mapheaderinfo[prevmap].lvlttl);

			// get RESULT header
			data.match.result = W_CachePatchName("RESULT", PU_STATIC);

			bgtile = W_CachePatchName("SRB2BACK", PU_STATIC);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_timerace: // time-only race
		{
			// Calculate who won
			Y_CalculateTimeRaceWinners();

			// set up the levelstring
			if(mapheaderinfo[prevmap].actnum)
				sprintf(data.match.levelstring, "%.32s * %d *",
					mapheaderinfo[prevmap].lvlttl, mapheaderinfo[prevmap].actnum);
			else
				sprintf(data.match.levelstring, "* %.32s *",
					mapheaderinfo[prevmap].lvlttl);

			// get RESULT header
			data.match.result = W_CachePatchName("RESULT", PU_STATIC);

			bgtile = W_CachePatchName("SRB2BACK", PU_STATIC);
			usetile = true;
			useinterpic = false;
			break;
		}
		
		case int_ctf:
		{
			// Calculate who won
			Y_CalculateMatchWinners();

			// set up the levelstring
			if(mapheaderinfo[prevmap].actnum)
				sprintf(data.match.levelstring, "%.32s * %d *",
					mapheaderinfo[prevmap].lvlttl, mapheaderinfo[prevmap].actnum);
			else
				sprintf(data.match.levelstring, "* %.32s *",
					mapheaderinfo[prevmap].lvlttl);

			data.match.redflag = W_CachePatchName("RFLAGICO", PU_STATIC);
			data.match.blueflag = W_CachePatchName("BFLAGICO", PU_STATIC);
			bgtile = W_CachePatchName("SRB2BACK", PU_STATIC);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_spec: // coop or single player, special stage
		{
			// give out ring bonuses
			Y_AwardSpecialStageBonus();

			// get background tile
			bgtile = W_CachePatchName("SPECTILE", PU_STATIC);

			// grab an interscreen if appropriate
			if(mapheaderinfo[gamemap-1].interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1].interscreen, PU_STATIC);
				useinterpic = true;
			}
			else
				useinterpic = false;

			// tile if using the default background
			usetile = !useinterpic;

			// get special stage specific patches
			if(ALL7EMERALDS)
			{
				data.spec.cemerald = W_CachePatchName("GOTEMALL", PU_STATIC);
				data.spec.headx = 70;
				data.spec.nowsuper = players[consoleplayer].skin
					? NULL : W_CachePatchName("NOWSUPER", PU_STATIC);
			}
			else
			{
				data.spec.cemerald = W_CachePatchName("CEMERALD", PU_STATIC);
				data.spec.headx = 48;
				data.spec.nowsuper = NULL;
			}
			data.spec.pringbonus = W_CachePatchName("YRINGBNS", PU_STATIC);
			data.spec.cscore = W_CachePatchName("CSCORE", PU_STATIC);
			break;
		}

		case int_race: // full race
		{
			// find out who won
			Y_CalculateRaceWinners();

			// set up the levelstring
			if(mapheaderinfo[prevmap].actnum)
				sprintf(data.race.levelstring, "%.32s * %d *",
					mapheaderinfo[prevmap].lvlttl, mapheaderinfo[prevmap].actnum);
			else
				sprintf(data.race.levelstring, "* %.32s *",
					mapheaderinfo[prevmap].lvlttl);

			// get background tile
			bgtile = W_CachePatchName("SRB2BACK", PU_STATIC);
			usetile = true;
			useinterpic = false;

			// get RESULT header
			data.race.result = W_CachePatchName("RESULT", PU_STATIC);
			break;
		}

		case int_none:
		default:
			break;
	}
}

//
// Y_AwardCoopBonuses
//
// Awards the time and ring bonuses.
//
void Y_AwardCoopBonuses(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		int secs, bonus, oldscore;

		if(!playeringame[i])
			continue;

		// calculate time bonus
		secs = players[i].realtime / TICRATE;
		if(secs < 30)
			bonus = 50000;
		else if(secs < 45)
			bonus = 10000;
		else if(secs < 60)
			bonus = 5000;
		else if(secs < 90)
			bonus = 4000;
		else if(secs < 120)
			bonus = 3000;
		else if(secs < 180)
			bonus = 2000;
		else if(secs < 240)
			bonus = 1000;
		else if(secs < 300)
			bonus = 500;
		else
			bonus = 0;

		if(i == consoleplayer)
		{
			data.coop.timebonus = bonus;
			if(players[i].health)
				data.coop.ringbonus = (players[i].health-1) * 100;
			else
				data.coop.ringbonus = 0;
			data.coop.total = 0;
			data.coop.score = players[i].score;
		}

		oldscore = players[i].score;

		players[i].score += bonus;
		if(players[i].health)
			players[i].score += (players[i].health-1) * 100; // ring bonus

		// grant extra lives right away since tally is faked
		P_GivePlayerLives(&players[i], (players[i].score/50000) - (oldscore/50000));

		if((players[i].score/50000) - (oldscore/50000) > 0)
			data.coop.gotlife = i;
		else
			data.coop.gotlife = -1;
	}
}

//
// Y_AwardSpecialStageBonus
//
// Gives a ring bonus only.
void Y_AwardSpecialStageBonus(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		int oldscore;

		if(!playeringame[i])
			continue;

		if(i == consoleplayer)
		{
			if(players[i].health)
				data.spec.ringbonus = (players[i].health-1) * 100;
			else
				data.spec.ringbonus = 0;
			data.spec.score = players[i].score;
		}

		oldscore = players[i].score;

		if(players[i].health)
			players[i].score += (players[i].health-1) * 100; // ring bonus

		// grant extra lives right away since tally is faked
		P_GivePlayerLives(&players[i], (players[i].score/50000) - (oldscore/50000));

		if((players[i].score/50000) - (oldscore/50000) > 0)
			data.coop.gotlife = i;
		else
			data.coop.gotlife = -1;
	}
}

//
// Y_CalculateMatchWinners
//
void Y_CalculateMatchWinners(void)
{
	int i, j;
	boolean completed[MAXPLAYERS];

	// Initialize variables
	memset(data.match.scores, 0, sizeof(data.match.scores));
	memset(data.match.color, 0, sizeof(data.match.color));
	memset(data.match.character, 0, sizeof(data.match.character));
	memset(completed, 0, sizeof(completed));
	data.match.numplayers = 0;
	i = j = 0;

	for(j=0; j<MAXPLAYERS; j++)
	{
		if(!playeringame[j])
			continue;

		for(i=0; i<MAXPLAYERS; i++)
		{
			if(!playeringame[i])
				continue;

			if(players[i].score >= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.scores[data.match.numplayers] = players[i].score;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}

//
// Y_CalculateTimeRaceWinners
//
void Y_CalculateTimeRaceWinners(void)
{
	int i, j;
	boolean completed[MAXPLAYERS];

	// Initialize variables

	for(i=0; i<MAXPLAYERS; i++)
		data.match.scores[i] = MAXINT;

	memset(data.match.color, 0, sizeof(data.match.color));
	memset(data.match.character, 0, sizeof(data.match.character));
	memset(completed, 0, sizeof(completed));
	data.match.numplayers = 0;
	i = j = 0;

	for(j=0; j<MAXPLAYERS; j++)
	{
		if(!playeringame[j])
			continue;

		for(i=0; i<MAXPLAYERS; i++)
		{
			if(!playeringame[i])
				continue;

			if(players[i].realtime <= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.scores[data.match.numplayers] = players[i].realtime;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}

//
// Y_CalculateRaceWinners
//
void Y_CalculateRaceWinners(void)
{
	int winners[4], numwins[MAXPLAYERS];
	int i, n, score, time, ring, totalring, itembox, wins;
	int numplayersingame;

	// Everyone has zero wins.
	memset(numwins, 0, sizeof(int)*MAXPLAYERS);

	// No one has won anything.
	winners[0] = winners[1] = winners[2] = winners[3] = winners[4] = -1;

	// Find the highest score.
	for(i = 0, n = 0, score = -1; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || players[i].score < score)
			continue;

		if(players[i].score == score)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this score
		score = players[i].score; // best score so far
		winners[0] = i; // winner so far
	}

	if(n == 1)
		numwins[winners[0]]++;
	else
		winners[0] = -1; // tie

	// Find the lowest time.
	for(i = 0, n = 0, time = leveltime; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || players[i].realtime > time)
			continue;

		if(players[i].realtime == time)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this time
		time = players[i].realtime; // best time so far
		winners[1] = i; // winner so far
	}

	if(n == 1)
		numwins[winners[1]]++;
	else
		winners[1] = -1; // tie

	// Find the highest ring count.
	for(i = 0, n = 0, ring = -1; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || (players[i].health?players[i].health-1:0) < ring)
			continue;

		if((players[i].health?players[i].health-1:0) == ring)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this many rings
		ring = (players[i].health?players[i].health-1:0); // best ring count so far
		winners[2] = i; // winner so far
	}

	if(n == 1)
		numwins[winners[2]]++;
	else
		winners[2] = -1; // tie

	// Find the highest total ring count.
	for(i = 0, n = 0, totalring = -1; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || players[i].totalring < totalring)
			continue;

		if(players[i].totalring == totalring)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this many total rings
		totalring = players[i].totalring; // best total ring count so far
		winners[3] = i; // winner so far
	}

	if(n == 1)
		numwins[winners[3]]++;
	else
		winners[3] = -1; // tie

	// Find the highest item box count.
	for(i = 0, n = 0, itembox = -1; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || players[i].numboxes < itembox)
			continue;

		if(players[i].numboxes == itembox)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this many item boxes
		itembox = players[i].numboxes; // best item box count so far
		winners[4] = i; // winner so far
	}

	if(n == 1)
		numwins[winners[4]]++;
	else
		winners[4] = -1; // tie

	numplayersingame = 0;
	for(i=0; i<MAXPLAYERS; i++)
	{
		if(playeringame[i])
			numplayersingame++;
	}

	// Decide which players to display in the list.
	if(numplayersingame <= 4) // This is easy!
	{
		data.race.numplayersshown = numplayersingame;
		for(i = 0, n = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				data.race.playersshown[n++] = i;

		for(i = data.race.numplayersshown; i < 4; i++)
			data.race.playersshown[i] = -1; // no player here
	}
	else // This is hard!
	{
		int j, k;

		data.race.numplayersshown = 4;
		for(i = 0, n = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
			{
				if(n < 4)
				{
					data.race.playersshown[n++] = i;
					continue;
				}

				// n == 4, meaning all four slots are full.

				if(!numwins[i])
					continue;

				// But this player won at least one category, so maybe he can
				// replace one who didn't.

				for(j = 3; j >= 0; j--)
					if(!numwins[j])
						break;

				if(j < 0)
					break; // Five winners, four slots. Sorry, you get stiffed.

				// j (0 <= j <= 3) is a slot whose player did not win any
				// categories. Player i can go here instead.

				// If j < 3, first we need to move the other displayed players
				// back one.

				for(k = j; k < 3; k++)
					data.race.playersshown[k] = data.race.playersshown[k+1];

				// Now player i gets his rightful position.

				data.race.playersshown[3] = i;
			}
	}

	// Set up the winner string for each category.
	//  "1P", "2P", "3P", "4P", "5P" or "T"
	for(i = 0; i < 5; i++)
	{
		if(winners[i] == -1)
			data.race.winnerstrings[i] = "T";
		else if(winners[i] == data.race.playersshown[0])
			data.race.winnerstrings[i] = "1P";
		else if(winners[i] == data.race.playersshown[1])
			data.race.winnerstrings[i] = "2P";
		else if(winners[i] == data.race.playersshown[2])
			data.race.winnerstrings[i] = "3P";
		else if(winners[i] == data.race.playersshown[3])
			data.race.winnerstrings[i] = "4P";
		else
		{
			CONS_Printf("Winners[%i] is %i\n", i, winners[i]);
			data.race.winnerstrings[i] = "5P";
		}
	}

	// Set up the display slot data.
	for(i = 0; i < data.race.numplayersshown; i++)
	{
		data.race.scores[i] = players[data.race.playersshown[i]].score;
		data.race.timemin[i] = players[data.race.playersshown[i]].realtime / (60*TICRATE);
		data.race.timesec[i] = (players[data.race.playersshown[i]].realtime / TICRATE) % 60;
		data.race.timetic[i] = players[data.race.playersshown[i]].realtime % TICRATE;
		data.race.rings[i] = players[data.race.playersshown[i]].health ? players[data.race.playersshown[i]].health-1 : 0;
		data.race.totalrings[i] = players[data.race.playersshown[i]].totalring;
		data.race.itemboxes[i] = players[data.race.playersshown[i]].numboxes;
		data.race.totalwins[i] = numwins[data.race.playersshown[i]];

		// Make sure it's in bounds so it won't screw up the display.
		if(data.race.scores[i] > 999999)
			data.race.scores[i] = 999999;
		if(data.race.timemin[i] > 99)
			data.race.timemin[i] = 99;
		if(data.race.rings[i] > 999)
			data.race.rings[i] = 999;
		if(data.race.totalrings[i] > 999)
			data.race.totalrings[i] = 999;
		if(data.race.itemboxes[i] > 999)
			data.race.itemboxes[i] = 999;
	}

	// Find the overall winner.
	for(i = 0, n = 0, wins = 0; i < MAXPLAYERS; i++)
	{
		if(!playeringame[i] || numwins[i] < wins)
			continue;

		if(numwins[i] == wins)
		{
			n++;
			continue;
		}

		n = 1; // number of players with this many wins
		wins = numwins[i]; // best number of wins so far
		data.race.winner = i; // winner so far
	}

	if(n != 1 || !wins)
		data.race.winner = -1; // tie
}

//
// Y_DrawScaledNum
//
// Dumb display function for positive numbers.
// Like ST_DrawOverlayNum, but scales the start and isn't translucent.
//
static void Y_DrawScaledNum(int x, int y, int flags, int num)
{
	int w = (tallnum[0]->width);

	// special case for 0
	if(!num)
	{
		V_DrawScaledPatch(x - w, y, flags, tallnum[0]);
		return;
	}

#ifdef PARANOIA
	if(num < 0)
		I_Error("Intermission drawer used negative number!");
#endif

	// draw the number
	while(num)
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, tallnum[num % 10]);
		num /= 10;
	}
}

//
// Y_EndIntermission
//
void Y_EndIntermission(void)
{
	Y_UnloadData();

	endtic = -1;
	inttype = int_none;
}

//
// Y_FollowIntermission
//
void Y_FollowIntermission(void)
{
	if(nextmap < 1100-1)
	{
		// normal level
		G_AfterIntermission();
		return;
	}

	// Start a custom cutscene if there is one.
	if(mapheaderinfo[gamemap-1].cutscenenum)
	{
		F_StartCustomCutscene(mapheaderinfo[gamemap-1].cutscenenum-1, false, false);
		return;
	}

	// Only do evaluation and credits in coop games.
	if(gametype == GT_COOP)
	{
		if(nextmap == 1102-1) // end game with credits
		{
			F_StartCredits();
			return;
		}
		if(nextmap == 1101-1) // end game with evaluation
		{
			F_StartGameEvaluation();
			return;
		}
	}

	// 1100 or competitive multiplayer, so go back to title screen.
	D_StartTitle();
}

#define UNLOAD(x) { if(x) { Z_ChangeTag(x, PU_CACHE); x = NULL; } }

//
// Y_UnloadData
//
void Y_UnloadData(void)
{
	// In hardware mode, don't Z_ChangeTag a pointer returned by W_CachePatchName().
	// It doesn't work and is unnecessary.
	if(rendermode != render_soft)
		return;

	// unload the background patches
	UNLOAD(bgpatch);
	UNLOAD(widebgpatch);
	UNLOAD(bgtile);
	UNLOAD(interpic);

	switch(inttype)
	{
		case int_coop:
			// unload the coop and single player patches
			UNLOAD(data.coop.ttlnum);
			UNLOAD(data.coop.ptimebonus);
			UNLOAD(data.coop.pringbonus);
			UNLOAD(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			UNLOAD(data.spec.cemerald);
			UNLOAD(data.spec.pringbonus);
			UNLOAD(data.spec.cscore);
			UNLOAD(data.spec.nowsuper);
			break;
		case int_match:
			UNLOAD(data.match.result);
			break;
		case int_ctf:
			UNLOAD(data.match.blueflag);
			UNLOAD(data.match.redflag);
			break;
		case int_race:
		case int_timerace:
			// unload the RESULT patch
			UNLOAD(data.race.result);
			break;
		default:
			//without this default, int_none, int_tag, int_ctf,
			///int_chaos, and int_timerace are not handled
			break;
	}
}
