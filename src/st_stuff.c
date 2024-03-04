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
//
// DESCRIPTION:
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------
/// \file
/// \brief Status bar code
/// 
///	Does the face/direction indicator animatin.
///	Does palette indicators as well (red pain/berserk, bright pickup)

#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "p_local.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "i_system.h"
#include "m_menu.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

//
// STATUS BAR DATA
//

// Palette indices.
#define STARTBONUSPALS 9
#define NUMBONUSPALS 4

patch_t* tallnum[10]; // 0-9, tall numbers
static patch_t* nightsnum[10]; // NiGHTS timer numbers

patch_t* faceprefix[MAXSKINS]; // face status patches
patch_t* facenameprefix[MAXSKINS]; // face background

// ------------------------------------------
//             status bar overlay
// ------------------------------------------

// icons for overlay
patch_t* sboscore; // Score logo
patch_t* sbotime; // Time logo
patch_t* sbocolon; // Colon for time
static patch_t* sborings;
static patch_t* sboover;
static patch_t* timeover;
static patch_t* stlivex;
static patch_t* rrings;
static patch_t*	getall; // Special Stage HUD
static patch_t*	timeup; // Special Stage HUD
static patch_t* homing1; // Emerald hunt indicators
static patch_t* homing2; // Emerald hunt indicators
static patch_t* homing3; // Emerald hunt indicators
static patch_t* homing4; // Emerald hunt indicators
static patch_t* homing5; // Emerald hunt indicators
static patch_t* homing6; // Emerald hunt indicators
static patch_t* supersonic;
static patch_t* ttlnum;
static patch_t* nightslink;
static patch_t* count5;
static patch_t* count4;
static patch_t* count3;
static patch_t* count2;
static patch_t* count1;
static patch_t* count0;
static patch_t* homingring;
static patch_t* autoring;
static patch_t* explosionring;
static patch_t* railring;
static patch_t* infinityring;
static patch_t* jumpshield;
static patch_t* fireshield;
static patch_t* ringshield;
static patch_t* watershield;
static patch_t* bombshield;
static patch_t* invincibility;
static patch_t* sneakers;
static patch_t* bluestat;
static patch_t* byelstat;
static patch_t* orngstat;
static patch_t* redstat;
static patch_t* yelstat;
static patch_t* nbracket;
static patch_t* nhud[12];
static patch_t* narrow[9];
static patch_t* minicaps;

static boolean facefreed[MAXPLAYERS];
static boolean prefixfreed[MAXPLAYERS];

hudinfo_t hudinfo[NUMHUDITEMS] = 
{
	{	52,		192	},			// HUD_LIVESNAME
	{   16,		192	},			// HUD_LIVESPIC
	{   88,		192	},		// HUD_LIVESNUM
	{   56,		192	},		// HUD_LIVESX
	{   220,	10	},		// HUD_RINGSSPLIT
	{   288,	10	},		// HUD_RINGSNUMSPLIT
	{   16,		42	},			// HUD_RINGS
	{   112,	42	},		// HUD_RINGSNUM
	{   16,		10	},			// HUD_SCORE
	{   128,	10	},		// HUD_SCORENUM
	{   136,	10	},		// HUD_TIMESPLIT
	{   204,	10	},		// HUD_LOWSECONDSSPLIT
	{   212,	10	},		// HUD_SECONDSSPLIT
	{   188,	10	},		// HUD_MINUTESSPLIT
	{   188,	10	},		// HUD_TIMECOLONSPLIT
	{   17,		26	},			// HUD_TIME
	{   104,	26	},		// HUD_LOWSECONDS
	{   112,	26	},		// HUD_SECONDS
	{   88,		26	},			// HUD_MINUTES
	{   88,		26	},			// HUD_TIMECOLON
	{   288,	40	},		// HUD_SS_TOTALRINGS_SPLIT
	{   112,	56	},		// HUD_SS_TOTALRINGS
	{   100,	90	},		// HUD_GETRINGS
	{   160,	93	},		// HUD_GETRINGSNUM
	{   124,	160	},	// HUD_TIMELEFT
	{   168,	176	},	// HUD_TIMELEFTNUM
	{   125,	90	},		// HUD_TIMEUP
	{   132,	168	},	// HUD_HUNTPIC1
	{   152,	168	},	// HUD_HUNTPIC2
	{   172,	168	},	// HUD_HUNTPIC3
	{   224,	6	},		// HUD_LIGHTDASHBOX
	{   260,	8	},		// HUD_LIGHTDASHSTRING1
	{   260,	20	},		// HUD_LIGHTDASHSTRING2
};

//
// STATUS BAR CODE
//

boolean ST_SameTeam(player_t* a, player_t* b)
{
	if(gametype == GT_CTF)
		return a->skincolor == b->skincolor;

	switch(cv_teamplay.value)
	{
		case 0:
			return false;
		case 1:
			return a->skincolor == b->skincolor;
		case 2:
			return a->skin == b->skin;
	}
	return false;
}

static boolean st_stopped = true;

void ST_Ticker(void)
{
	if(st_stopped)
		return;
}

static int st_palette = 0;

void ST_doPaletteStuff(void)
{
	int palette;

	if(stplyr->bonuscount)
	{
		palette = (stplyr->bonuscount+7)>>3;

		if(palette >= NUMBONUSPALS)
			palette = NUMBONUSPALS - 1;

		palette += STARTBONUSPALS;
	}
	else
		palette = 0;

	if(palette != st_palette)
	{
		st_palette = palette;

#ifdef HWRENDER
		if((rendermode == render_opengl) || (rendermode == render_d3d))

		//faB - NOW DO ONLY IN SOFTWARE MODE, LETS FIX THIS FOR GOOD OR NEVER
		//      idea: use a true color gradient from frame to frame, because we
		//            are in true color in HW3D, we can have smoother palette change
		//            than the palettes defined in the wad

			HWR_SetPaletteColor(0x0);
		else
#endif
		if(rendermode != render_none)
		{
			if(palette >= STARTBONUSPALS && palette <= STARTBONUSPALS + NUMBONUSPALS)
				V_SetPaletteLump("FLASHPAL");
			else
				V_SetPaletteLump("PLAYPAL");

			if(!cv_splitscreen.value || !palette)
				V_SetPalette(palette);
		}
	}
}

static void ST_overlayDrawer(void);

void ST_Drawer(boolean refresh)
{
	// force a set of the palette by using doPaletteStuff()
	refresh = 0; //?
	if(vid.recalc)
		st_palette = -1;

	// Do red-/gold-shifts from damage/items
#ifdef HWRENDER
	//25/08/99: Hurdler: palette changes is done for all players,
	//                   not only player1! That's why this part
	//                   of code is moved somewhere else.
	if(rendermode == render_soft)
#endif
		if(rendermode != render_none) ST_doPaletteStuff();

	if(st_overlay)
	{
		// No deadview!
		stplyr = &players[displayplayer];
		ST_overlayDrawer();

		if(cv_splitscreen.value)
		{
			stplyr = &players[secondarydisplayplayer];
			ST_overlayDrawer();
		}
	}
}

static void ST_LoadGraphics(void)
{
	int i;
	char namebuf[9];

	// Load the numbers, tall and short
	for(i = 0; i < 10; i++)
	{
		sprintf(namebuf, "STTNUM%d", i);
		tallnum[i] = (patch_t*)W_CachePatchName(namebuf, PU_STATIC);
		sprintf(namebuf, "NGTNUM%d", i);
		nightsnum[i] = (patch_t*) W_CachePatchName(namebuf, PU_STATIC);
	}

	// the original Doom uses 'STF' as base name for all face graphics
	// Graue 04-08-2004: face/name graphics are now indexed by skins
	//                   but load them in R_AddSkins, that gets called
	//                   first anyway
}

// made separate so that skins code can reload custom face graphics
// Graue 04-07-2004: index by skins
void ST_LoadFaceGraphics(char* facestr, int skinnum)
{
	char namelump[9];

	// hack: make sure base face name is no more than 8 chars
	if(strlen(facestr) > 8)
		facestr[8] = '\0';
	strcpy(namelump, facestr); // copy base name

	faceprefix[skinnum] = W_CachePatchName(namelump, PU_STATIC);
	facefreed[skinnum] = false;
}

// Tails 03-15-2002
// made separate so that skins code can reload custom face graphics
// Graue 04-07-2004: index by skins
void ST_LoadFaceNameGraphics(char* facestr, int skinnum)
{
	char namelump[9];

	// hack: make sure base face name is no more than 8 chars
	if(strlen(facestr) > 8)
		facestr[8] = '\0';
	strcpy(namelump, facestr); // copy base name

	facenameprefix[skinnum] = W_CachePatchName(namelump, PU_STATIC);
	prefixfreed[skinnum] = false;
}

static inline void ST_InitData(void)
{
	// 'link' the statusbar display to a player, which could be
	// another player than consoleplayer, for example, when you
	// change the view in a multiplayer demo with F12.
	stplyr = &players[displayplayer];

	st_palette = -1;
}

static void ST_Stop(void)
{
	if(st_stopped)
		return;

	V_SetPalette(0);

	st_stopped = true;
}

void ST_Start(void)
{
	if(!st_stopped)
		ST_Stop();

	ST_InitData();
	st_stopped = false;
}

//
// Initializes the status bar, sets the defaults border patch for the window borders.
//

// used by Glide mode, holds lumpnum of flat used to fill space around the viewwindow
int st_borderpatchnum;

void ST_Init(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		facefreed[i] = true;
		prefixfreed[i] = true;
	}

	if(dedicated)
		return;

	// screens[4] is allocated at videomode setup, and
	// set at V_Init(), the first time being at SCR_Recalc()

	// SRB2 border patch
	st_borderpatchnum = W_GetNumForName("FLOOR0_3");
	scr_borderpatch = W_CacheLumpNum(st_borderpatchnum, PU_STATIC);

	ST_LoadGraphics();

	// cache the status bar overlay icons (fullscreen mode)
	sborings = W_CachePatchName("SBORINGS", PU_STATIC);
	sboscore = W_CachePatchName("SBOSCORE", PU_STATIC);
	sboover = W_CachePatchName("SBOOVER", PU_STATIC);
	timeover = W_CachePatchName("TIMEOVER", PU_STATIC);
	stlivex = W_CachePatchName("STLIVEX", PU_STATIC);
	rrings = W_CachePatchName("RRINGS", PU_STATIC);
	sbotime = W_CachePatchName("SBOTIME", PU_STATIC); // Time logo
	sbocolon = W_CachePatchName("SBOCOLON", PU_STATIC); // Colon for time
	getall = W_CachePatchName("GETALL", PU_STATIC); // Special Stage HUD
	timeup = W_CachePatchName("TIMEUP", PU_STATIC); // Special Stage HUD
	homing1	= W_CachePatchName("HOMING1", PU_STATIC); // Emerald hunt indicators
	homing2	= W_CachePatchName("HOMING2", PU_STATIC); // Emerald hunt indicators
	homing3	= W_CachePatchName("HOMING3", PU_STATIC); // Emerald hunt indicators
	homing4	= W_CachePatchName("HOMING4", PU_STATIC); // Emerald hunt indicators
	homing5	= W_CachePatchName("HOMING5", PU_STATIC); // Emerald hunt indicators
	homing6	= W_CachePatchName("HOMING6", PU_STATIC); // Emerald hunt indicators
	supersonic = W_CachePatchName("SUPERICO", PU_STATIC);
	nightslink = W_CachePatchName("NGHTLINK", PU_STATIC);
	count5 = W_CachePatchName("CNTFA0", PU_STATIC);
	count4 = W_CachePatchName("CNTEA0", PU_STATIC);
	count3 = W_CachePatchName("CNTDA0", PU_STATIC);
	count2 = W_CachePatchName("CNTCA0", PU_STATIC);
	count1 = W_CachePatchName("CNTBA0", PU_STATIC);
	count0 = W_CachePatchName("CNTAA0", PU_STATIC);

	homingring = W_CachePatchName("HOMNIND", PU_STATIC);
	autoring = W_CachePatchName("AUTOIND", PU_STATIC);
	explosionring = W_CachePatchName("BOMBIND", PU_STATIC);
	railring = W_CachePatchName("RAILIND", PU_STATIC);
	infinityring = W_CachePatchName("INFNIND", PU_STATIC);
	jumpshield = W_CachePatchName("WHTVB0", PU_STATIC);
	fireshield = W_CachePatchName("RDTVB0", PU_STATIC);
	ringshield = W_CachePatchName("YLTVB0", PU_STATIC);
	watershield = W_CachePatchName("BLTVB0", PU_STATIC);
	bombshield = W_CachePatchName("BKTVB0", PU_STATIC);
	invincibility = W_CachePatchName("PINVB0", PU_STATIC);
	sneakers = W_CachePatchName("SHTVB0", PU_STATIC);

	// NiGHTS HUD things
	bluestat = W_CachePatchName("BLUESTAT", PU_STATIC);
	byelstat = W_CachePatchName("BYELSTAT", PU_STATIC);
	orngstat = W_CachePatchName("ORNGSTAT", PU_STATIC);
	redstat = W_CachePatchName("REDSTAT", PU_STATIC);
	yelstat = W_CachePatchName("YELSTAT", PU_STATIC);
	nbracket = W_CachePatchName("NBRACKET", PU_STATIC);
	nhud[0] = W_CachePatchName("NHUD1", PU_STATIC);
	nhud[1] = W_CachePatchName("NHUD2", PU_STATIC);
	nhud[2] = W_CachePatchName("NHUD3", PU_STATIC);
	nhud[3] = W_CachePatchName("NHUD4", PU_STATIC);
	nhud[4] = W_CachePatchName("NHUD5", PU_STATIC);
	nhud[5] = W_CachePatchName("NHUD6", PU_STATIC);
	nhud[6] = W_CachePatchName("NHUD7", PU_STATIC);
	nhud[7] = W_CachePatchName("NHUD8", PU_STATIC);
	nhud[8] = W_CachePatchName("NHUD9", PU_STATIC);
	nhud[9] = W_CachePatchName("NHUD10", PU_STATIC);
	nhud[10] = W_CachePatchName("NHUD11", PU_STATIC);
	nhud[11] = W_CachePatchName("NHUD12", PU_STATIC);
	minicaps = W_CachePatchName("MINICAPS", PU_STATIC);

	narrow[0] = W_CachePatchName("NARROW1", PU_STATIC);
	narrow[1] = W_CachePatchName("NARROW2", PU_STATIC);
	narrow[2] = W_CachePatchName("NARROW3", PU_STATIC);
	narrow[3] = W_CachePatchName("NARROW4", PU_STATIC);
	narrow[4] = W_CachePatchName("NARROW5", PU_STATIC);
	narrow[5] = W_CachePatchName("NARROW6", PU_STATIC);
	narrow[6] = W_CachePatchName("NARROW7", PU_STATIC);
	narrow[7] = W_CachePatchName("NARROW8", PU_STATIC);

	// non-animated version
	narrow[8] = W_CachePatchName("NARROW9", PU_STATIC);
}

// change the status bar too, when pressing F12 while viewing a demo.
void ST_changeDemoView(void)
{
	// the same routine is called at multiplayer deathmatch spawn
	// so it can be called multiple times
	ST_Start();
}

// =========================================================================
//                         STATUS BAR OVERLAY
// =========================================================================

boolean st_overlay;

static int SCY(int y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	y = (int)(y * vid.fdupy); // scale to resolution
	if(cv_splitscreen.value)
	{
		y >>= 1;
		if(stplyr != &players[displayplayer])
			y += vid.height / 2;
	}
	return y;
}

static int STRINGY(int y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	if(cv_splitscreen.value)
	{
		y >>= 1;
		if(stplyr != &players[displayplayer])
			y += BASEVIDHEIGHT / 2;
	}
	return y;
}

static inline int SCX(int x)
{
	return (int)(x * vid.fdupx);
}

// Draw a number, scaled, over the view
// Always draw the number completely since it's overlay
//
static void ST_DrawOverlayNum(int x /* right border */, int y, int num,
	patch_t** numpat)
{
	int w = (numpat[0]->width);
	boolean neg;

	// special case for 0
	if(!num)
	{
		V_DrawScaledPatch(x - (w*vid.dupx), y, V_NOSCALESTART|V_TRANSLUCENT, numpat[0]);
		return;
	}

	neg = num < 0;

	if(neg)
		num = -num;

	// draw the number
	while(num)
	{
		x -= (w * vid.dupx);
		V_DrawScaledPatch(x, y, V_NOSCALESTART|V_TRANSLUCENT, numpat[num % 10]);
		num /= 10;
	}

	// draw a minus sign if necessary
	if(neg)
		V_DrawScaledPatch(x - (8*vid.dupx), y, V_NOSCALESTART|V_TRANSLUCENT,
			(patch_t*)W_CachePatchName("STTMINUS", PU_STATIC)); // Tails
}

// Draw a number, scaled, over the view
// Always draw the number completely since it's overlay
//
// Supports different colors! woo!
static void ST_DrawNightsOverlayNum(int x /* right border */, int y, int num,
	patch_t** numpat, int colornum)
{
	int w = (numpat[0]->width);
	byte* colormap;
	int flags = 0;

	flags = (flags & ~MF_TRANSLATION) | (colornum<<MF_TRANSSHIFT);

	// Uses the player colors.
	colormap = (byte*)defaulttranslationtables - 256
		+ ((flags & MF_TRANSLATION)>>(MF_TRANSSHIFT-8));

	// special case for 0
	if(!num)
	{
		V_DrawMappedPatch(x - (w*vid.dupx), y, V_NOSCALESTART, numpat[0], colormap);
		return;
	}

#ifdef PARANOIA
	if(num < 0)
		I_Error("ST_DrawNightsOverlayNum was asked to draw a negative number!");
#endif

	// draw the number
	while(num)
	{
		x -= (w * vid.dupx);
		V_DrawMappedPatch(x, y, V_NOSCALESTART, numpat[num % 10], colormap);
		num /= 10;
	}

	// Sorry chum, this function only draws UNSIGNED values!
}

static void ST_drawDebugInfo(void)
{
	char smomx[33];
	char smomy[33];
	char smomz[33];
	char sspeed[33];
	char sfloorz[33];
	char spmomz[33];
	char scability[33];
	char scharsped[33];
	char scharspin[33];
	char sstrcolor[33];
	char sendcolor[33];
	char sdedtimer[33];
	char sjumpfact[33];
	char sx[33];
	char sy[33];
	char sz[33];
	char sangle[33];
	char sunderwater[33];
	char smfjumped[33];
	char smfspinning[33];
	char smfstartdash[33];
	char sjumping[33];
	char sscoreadd[33];

	sprintf(smomx, "%d", stplyr->rmomx >> FRACBITS);
	sprintf(smomy, "%d", stplyr->rmomy >> FRACBITS);
	sprintf(smomz, "%d", stplyr->mo->momz >> FRACBITS);
	sprintf(sspeed, "%d", stplyr->speed);
	sprintf(sfloorz, "%d", stplyr->mo->ceilingz >> FRACBITS);
	sprintf(spmomz, "%d", stplyr->mo->pmomz >> FRACBITS);
	sprintf(scability, "%d", stplyr->charability);
	sprintf(scharsped, "%d", stplyr->normalspeed);
	sprintf(scharspin, "%d", stplyr->charspin);
	sprintf(sstrcolor, "%d", stplyr->starttranscolor);
	sprintf(sendcolor, "%d", stplyr->endtranscolor);
	sprintf(sdedtimer, "%d", stplyr->deadtimer);
	sprintf(sjumpfact, "%d", stplyr->jumpfactor);
	sprintf(sx, "%d", stplyr->mo->x >> FRACBITS);
	sprintf(sy, "%d", stplyr->mo->y >> FRACBITS);
	sprintf(sz, "%d", stplyr->mo->z >> FRACBITS);
	sprintf(sangle, "%d", stplyr->mo->angle >> FRACBITS);
	sprintf(sunderwater, "%d", stplyr->powers[pw_underwater]);
	sprintf(smfjumped, "%d", stplyr->mfjumped);
	sprintf(smfspinning, "%d", stplyr->mfspinning);
	sprintf(smfstartdash, "%d", stplyr->mfstartdash);
	sprintf(sjumping, "%d", stplyr->jumping);
	sprintf(sscoreadd, "%d", stplyr->scoreadd);
	V_DrawString(248, 0, 0, "MOMX =");
	V_DrawString(296, 0, 0, smomx);
	V_DrawString(248, 8, 0, "MOMY =");
	V_DrawString(296, 8, 0, smomy);
	V_DrawString(248, 16, 0, "MOMZ =");
	V_DrawString(296, 16, 0, smomz);
	V_DrawString(240, 24, 0, "SPEED =");
	V_DrawString(296, 24, 0, sspeed);
	V_DrawString(232, 32, 0, "CEILGZ=");
	V_DrawString(288, 32, 0, sfloorz);
	V_DrawString(240, 40, 0, "PMOMZ =");
	V_DrawString(296, 40, 0, spmomz);
	V_DrawString(216, 48, 0, "CABILITY =");
	V_DrawString(296, 48, 0, scability);
	V_DrawString(216, 56, 0, "CHARSPED =");
	V_DrawString(296, 56, 0, scharsped);
	V_DrawString(216, 64, 0, "CHARSPIN =");
	V_DrawString(296, 64, 0, scharspin);
	V_DrawString(216, 72, 0, "STRCOLOR =");
	V_DrawString(296, 72, 0, sstrcolor);
	V_DrawString(216, 80, 0, "ENDCOLOR =");
	V_DrawString(296, 80, 0, sendcolor);
	V_DrawString(216, 88, 0, "DEDTIMER =");
	V_DrawString(296, 88, 0, sdedtimer);
	V_DrawString(216, 96, 0, "JUMPFACT =");
	V_DrawString(296, 96, 0, sjumpfact);
	V_DrawString(240, 104, 0, "X =");
	V_DrawString(264, 104, 0, sx);
	V_DrawString(240, 112, 0, "Y =");
	V_DrawString(264, 112, 0, sy);
	V_DrawString(240, 120, 0, "Z =");
	V_DrawString(264, 120, 0, sz);
	V_DrawString(216, 128, 0, "Angle =");
	V_DrawString(272, 128, 0, sangle);
	V_DrawString(192, 152, 0, "Underwater =");
	V_DrawString(288, 152, 0, sunderwater);
	V_DrawString(192, 160, 0, "MF_JUMPED =");
	V_DrawString(288, 160, 0, smfjumped);
	V_DrawString(192, 168, 0, "MF_SPINNING =");
	V_DrawString(296, 168, 0, smfspinning);
	V_DrawString(192, 176, 0, "MF_STARDASH =");
	V_DrawString(296, 176, 0, smfstartdash);
	V_DrawString(192, 184, 0, "Jumping =");
	V_DrawString(288, 184, 0, sjumping);
	V_DrawString(192, 192, 0, "Scoreadd =");
	V_DrawString(288, 192, 0, sscoreadd);
}

static void ST_drawLevelTitle(void)
{
#define ZONE "ZONE"
	char* lvlttl = mapheaderinfo[gamemap-1].lvlttl;
	int lvlttlxpos;
	int ttlnumxpos;
	int zonexpos;
	int actnum = mapheaderinfo[gamemap-1].actnum;
	boolean nonumber = false;

	if(!(timeinmap > 1 && timeinmap < 111))
		return;

	if(actnum > 0)
	{
		ttlnum = W_CachePatchName(va("TTL%.2d", actnum), PU_CACHE);
		lvlttlxpos = ((BASEVIDWIDTH/2) - (V_LevelNameWidth(lvlttl)/2)) - ttlnum->width;
	}
	else
	{
		nonumber = true;
		lvlttlxpos = ((BASEVIDWIDTH/2) - (V_LevelNameWidth(lvlttl)/2));
	}

	ttlnumxpos = lvlttlxpos + V_LevelNameWidth(lvlttl);
	zonexpos = ttlnumxpos - V_LevelNameWidth(ZONE);

	if(timeinmap == 2)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(200), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 0, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 200, 0, ZONE);
	}
	else if(timeinmap == 3)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(188), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 12, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 188, 0, ZONE);
	}
	else if(timeinmap == 4)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(0), SCY(176), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 24, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 176, 0, ZONE);
	}
	else if(timeinmap == 5)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(164), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 36, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 164, 0, ZONE);
	}
	else if(timeinmap == 6)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(152), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 48, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 152, 0, ZONE);
	}
	else if(timeinmap == 7)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(140), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 60, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 140, 0, ZONE);
	}
	else if(timeinmap == 8)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(128), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 72, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 128, 0, ZONE);
	}
	else if(timeinmap == 106)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(80), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 104, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 80, 0, ZONE);
	}
	else if(timeinmap == 107)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(56), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 128, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 56, 0, ZONE);
	}
	else if(timeinmap == 108)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(32), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 152, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 32, 0, ZONE);
	}
	else if(timeinmap == 109)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(8), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 176, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 8, 0, ZONE);
	}
	else if(timeinmap == 110)
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(0), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 200, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 0, 0, ZONE);
	}
	else
	{
		if(!nonumber)
			V_DrawScaledPatch(SCX(ttlnumxpos), SCY(104), V_NOSCALESTART, ttlnum);
		V_DrawLevelTitle(lvlttlxpos, 80, 0, lvlttl);

		if(!mapheaderinfo[gamemap-1].nozone)
			V_DrawLevelTitle(zonexpos, 104, 0, ZONE);
	}
#undef ZONE
}

static void ST_drawFirstPersonHUD(void)
{
	player_t* player = stplyr;
	fixed_t yheight;

	yheight = SCY(120);
	/// \todo you wanna do something about those countdown drown numbers?

	// Graue 06-18-2004: no V_NOSCALESTART, no SCX, no SCY, snap to right
	if(player->powers[pw_jumpshield])
		V_DrawScaledPatch(304, STRINGY(32), V_SNAPTORIGHT|V_TRANSLUCENT, jumpshield);
	else if(player->powers[pw_fireshield])
		V_DrawScaledPatch(304, STRINGY(32), V_SNAPTORIGHT|V_TRANSLUCENT, fireshield);
	else if(player->powers[pw_watershield])
		V_DrawScaledPatch(304, STRINGY(32), V_SNAPTORIGHT|V_TRANSLUCENT, watershield);
	else if(player->powers[pw_bombshield])
		V_DrawScaledPatch(304, STRINGY(32), V_SNAPTORIGHT|V_TRANSLUCENT, bombshield);
	else if(player->powers[pw_ringshield])
		V_DrawScaledPatch(304, STRINGY(32), V_SNAPTORIGHT|V_TRANSLUCENT, ringshield);

	if(player->powers[pw_invulnerability] > 3*TICRATE || (player->powers[pw_invulnerability]
		&& leveltime & 1))
		V_DrawScaledPatch(304, STRINGY(56), V_SNAPTORIGHT|V_TRANSLUCENT, invincibility);

	if(player->powers[pw_sneakers] > 3*TICRATE || (player->powers[pw_sneakers]
		&& leveltime & 1))
		V_DrawScaledPatch(304, STRINGY(80), V_SNAPTORIGHT|V_TRANSLUCENT, sneakers);

	// Display the countdown drown numbers!
	if(!player->nightsmode)
	{
		if((player->powers[pw_underwater] <= 11*TICRATE + 1
			&& player->powers[pw_underwater] >= 10*TICRATE + 1)
			|| (player->powers[pw_spacetime] <= 11*TICRATE + 1
			&& player->powers[pw_spacetime] >= 10*TICRATE + 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count5->width/2)), yheight,
				V_NOSCALESTART, count5);
		}
		else if((player->powers[pw_underwater] <= 9*TICRATE + 1
			&& player->powers[pw_underwater] >= 8*TICRATE + 1)
			|| (player->powers[pw_spacetime] <= 9*TICRATE + 1
			&& player->powers[pw_spacetime] >= 8*TICRATE + 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count4->width/2)), yheight,
				V_NOSCALESTART, count4);
		}
		else if((player->powers[pw_underwater] <= 7*TICRATE + 1
			&& player->powers[pw_underwater] >= 6*TICRATE + 1)
			|| (player->powers[pw_spacetime] <= 7*TICRATE + 1
			&& player->powers[pw_spacetime] >= 6*TICRATE + 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count3->width/2)), yheight,
				V_NOSCALESTART, count3);
		}
		else if((player->powers[pw_underwater] <= 5*TICRATE + 1
			&& player->powers[pw_underwater] >= 4*TICRATE + 1)
			|| (player->powers[pw_spacetime] <= 5*TICRATE + 1
			&& player->powers[pw_spacetime] >= 4*TICRATE + 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count2->width/2)), yheight,
				V_NOSCALESTART, count2);
		}
		else if((player->powers[pw_underwater] <= 3*TICRATE + 1
			&& player->powers[pw_underwater] >= 2*TICRATE + 1)
			||
			(player->powers[pw_spacetime] <= 3*TICRATE + 1
			&& player->powers[pw_spacetime] >= 2*TICRATE + 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count1->width/2)), yheight,
				V_NOSCALESTART, count1);
		}
		else if((player->powers[pw_underwater] <= 1*TICRATE + 1
			&& player->powers[pw_underwater] > 1)
			||
			(player->powers[pw_spacetime] <= 1*TICRATE + 1
			&& player->powers[pw_spacetime] > 1))
		{
			V_DrawTranslucentPatch(SCX((BASEVIDWIDTH/2) - (count0->width/2)), yheight,
				V_NOSCALESTART, count0);
		}
	}
}

static void ST_drawNiGHTSHUD(void)
{
	if(stplyr->nightsmode)
	{
		if(!(stplyr->drillmeter & 1))
		{
			V_DrawFill(14, STRINGY(142), 100, 8, 64);
			V_DrawFill(16, STRINGY(144), 96, 4, 0);
			V_DrawFill(16, STRINGY(144), stplyr->drillmeter/20, 4, 112);
		}
		else
		{
			V_DrawFill(14, STRINGY(142), 100, 8, 144);
			V_DrawFill(16, STRINGY(144), 96, 4, 88);
			V_DrawFill(16, STRINGY(144), stplyr->drillmeter/20, 4, 116);
		}
	}

	if(stplyr->bonustime > 1)
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(100), 0, "BONUS TIME START!");

	V_DrawScaledPatch(SCX(16), SCY(8), V_NOSCALESTART, nbracket);
	V_DrawScaledPatch(SCX(24), (int)(SCY(8) + 8*vid.fdupy), V_NOSCALESTART, nhud[(leveltime/2)%12]);

	if(stplyr->capsule)
	{
		int amount;
		int origamount;
		const int length = 88;

		V_DrawScaledPatch(SCX(72), SCY(8), V_NOSCALESTART, nbracket);
		V_DrawScaledPatch(SCX(74), (int)(SCY(8) + 4*vid.fdupy), V_NOSCALESTART,
			minicaps);

		if(stplyr->capsule->reactiontime != 0)
		{
			int r;
			const int orblength = 20;

			for(r = 0; r < 5; r++)
			{
				V_DrawScaledPatch(SCX(230 - (7*r)), SCY(144), V_NOSCALESTART,
					redstat);
				V_DrawScaledPatch(SCX(188 - (7*r)), SCY(144), V_NOSCALESTART,
					orngstat);
				V_DrawScaledPatch(SCX(146 - (7*r)), SCY(144), V_NOSCALESTART,
					yelstat);
				V_DrawScaledPatch(SCX(104 - (7*r)), SCY(144), V_NOSCALESTART,
					byelstat);
			}
			origamount = stplyr->capsule->spawnpoint->angle & 1023;

			amount = (origamount - stplyr->capsule->health);

			amount = (int)(amount * ((float)orblength/origamount));

			if(amount > 0)
			{
				int t;

				// Fill up the bar with blue orbs... in reverse! (yuck)
				for(r = amount; r >= 0; r--)
				{
					t = r;

					if(r > 14)
						t += 1;
					if(r > 9)
						t += 1;
					if(r > 4)
						t += 1;

					V_DrawScaledPatch(SCX(76 + (7*t)), SCY(144), V_NOSCALESTART,
						bluestat);
				}
			}
		}
		else
		{
			// Lil' white box!
			V_DrawFill(15, STRINGY(8) + 34, length + 2, 5, 4);
			V_DrawFill(16, STRINGY(8)+35, length/4, 3, 231);
			V_DrawFill(16 + length/4, STRINGY(8) + 35, length/4, 3, 248);
			V_DrawFill(16 + (length/4)*2, STRINGY(8) + 35, length/4, 3, 215);
			V_DrawFill(16 + (length/4)*3, STRINGY(8) + 35, length/4, 3, 179);
			origamount = stplyr->capsule->spawnpoint->angle & 1023;

			amount = (origamount - stplyr->capsule->health);
			amount = (int)(amount * ((float)length/origamount));

			if(amount > 0)
				V_DrawFill(16, STRINGY(8) + 35, amount, 3, 197);
		}
		V_DrawScaledPatch(SCX(40), (int)(SCY(8) + 5*vid.fdupy), V_NOSCALESTART, narrow[(leveltime/2)%8]);
	}
	else
		V_DrawScaledPatch(SCX(40), (int)(SCY(8) + 5*vid.fdupy), V_NOSCALESTART, narrow[8]);

	ST_DrawOverlayNum(SCX(68), (int)(SCY(8) + 11*vid.fdupy), stplyr->health > 0 ? stplyr->health - 1 : 0, tallnum);
	ST_DrawNightsOverlayNum(SCX(288), SCY(12), stplyr->score, nightsnum, 7); // Blue

	if(stplyr->nightstime > 0)
	{
		int numbersize;

		if(stplyr->nightstime < 10)
			numbersize = SCX(16)/2;
		else if(stplyr->nightstime < 100)
			numbersize = SCX(32)/2;
		else
			numbersize = SCX(48)/2;

		if(stplyr->nightstime < 10)
			ST_DrawNightsOverlayNum(SCX(160) + numbersize, SCY(32), stplyr->nightstime,
				nightsnum, 6); // Red
		else
			ST_DrawNightsOverlayNum(SCX(160) + numbersize, SCY(32), stplyr->nightstime,
				nightsnum, MAXSKINCOLORS-1); // Yellow
	}
}

static void ST_drawTagHUD(void)
{
	if(stplyr->tagit)
	{
		char stagit[33];
		sprintf(stagit, "%d", stplyr->tagit/TICRATE);

		if(cv_splitscreen.value)
			V_DrawString(120, STRINGY(168), 0, "YOU'RE IT!");
		else
			V_DrawString(120, STRINGY(176), 0, "YOU'RE IT!");

		V_DrawString(158 - (V_StringWidth(stagit))/2, STRINGY(184), 0, stagit);
	}

	if(stplyr->tagzone)
	{
		char stagzone[33];
		sprintf(stagzone, "%d", stplyr->tagzone/TICRATE);
		if(cv_splitscreen.value)
		{
			V_DrawString(201, STRINGY(168), 0, "IN NO-TAG ZONE");
			V_DrawString(254 - (V_StringWidth(stagzone))/2, STRINGY(184), 0, stagzone);
		}
		else
		{
			V_DrawString(104, STRINGY(160), 0, "IN NO-TAG ZONE");
			V_DrawString(158 - (V_StringWidth(stagzone))/2, STRINGY(168), 0, stagzone);
		}
	}
	else if(stplyr->taglag)
	{
		char staglag[33];
		sprintf(staglag, "%i", stplyr->taglag/TICRATE);

		if(cv_splitscreen.value)
		{
			V_DrawString(30, STRINGY(168), 0, "NO-TAG LAG");
			V_DrawString(62 - (V_StringWidth(staglag))/2, STRINGY(184), 0, staglag);
		}
		else
		{
			V_DrawString(120, STRINGY(160), 0, "NO-TAG LAG");
			V_DrawString(158 - (V_StringWidth(staglag))/2, STRINGY(168), 0, staglag);
		}
	}
}

static void ST_drawCTFHUD(void)
{
	int i, team;
	unsigned short whichflag;
	team = whichflag = 0;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(players[i].gotflag & MF_REDFLAG)
		{
			team = players[i].ctfteam;
			whichflag = players[i].gotflag;
			break; // break, don't continue.
		}
	}

	if(stplyr->ctfteam != team && team > 0 && ((stplyr->ctfteam == 1 && whichflag & MF_REDFLAG)
		|| (stplyr->ctfteam == 2 && whichflag & MF_BLUEFLAG)))
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(120), V_WHITEMAP|V_TRANSLUCENT,
				"OTHER TEAM HAS YOUR FLAG!");
		else
			V_DrawString(128, STRINGY(160), V_WHITEMAP, "OTHER TEAM HAS YOUR FLAG!");
	}
	else if(stplyr->ctfteam == team && team > 0)
	{
		if((stplyr->ctfteam == 1 && whichflag & MF_REDFLAG) || (stplyr->ctfteam == 2
			&& whichflag & MF_BLUEFLAG))
		{
			if(cv_splitscreen.value)
				V_DrawString(128, STRINGY(120), V_TRANSLUCENT, "YOUR TEAM HAS YOUR FLAG!");
			else
				V_DrawString(128, STRINGY(160), 0, "YOUR TEAM HAS YOUR FLAG!");
		}
		else
		{
			if(cv_splitscreen.value)
				V_DrawString(128, STRINGY(136), V_TRANSLUCENT, "YOUR TEAM HAS ENEMY FLAG!");
			else
				V_DrawString(128, STRINGY(168), 0, "YOUR TEAM HAS ENEMY FLAG!");
		}
	}

	team = whichflag = 0;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(players[i].gotflag & MF_BLUEFLAG)
		{
			team = players[i].ctfteam;
			whichflag = players[i].gotflag;
			break; // break, don't continue.
		}
	}
	if(stplyr->ctfteam != team && team > 0 && ((stplyr->ctfteam == 1 && whichflag & MF_REDFLAG)
		|| (stplyr->ctfteam == 2 && whichflag & MF_BLUEFLAG)))
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(120), V_WHITEMAP|V_TRANSLUCENT, "OTHER TEAM HAS YOUR FLAG!");
		else
			V_DrawString(128, STRINGY(160), V_WHITEMAP, "OTHER TEAM HAS YOUR FLAG!");
	}
	else if(stplyr->ctfteam == team && team > 0)
	{
		if((stplyr->ctfteam == 1 && whichflag & MF_REDFLAG) || (stplyr->ctfteam == 2
			&& whichflag & MF_BLUEFLAG))
		{
			if(cv_splitscreen.value)
				V_DrawString(128, STRINGY(120), V_TRANSLUCENT, "YOUR TEAM HAS YOUR FLAG!");
			else
				V_DrawString(128, STRINGY(160), 0, "YOUR TEAM HAS YOUR FLAG!");
		}
		else
		{
			if(cv_splitscreen.value)
				V_DrawString(128, STRINGY(136), V_TRANSLUCENT, "YOUR TEAM HAS ENEMY FLAG!");
			else
				V_DrawString(128, STRINGY(168), 0, "YOUR TEAM HAS ENEMY FLAG!");
		}
	}

	if(stplyr->gotflag & MF_REDFLAG)
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(152), V_TRANSLUCENT, "YOU HAVE THE RED FLAG");
		else
			V_DrawString(128, STRINGY(176), 0, "YOU HAVE THE RED FLAG");
	}
	else if(stplyr->gotflag & MF_BLUEFLAG)
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(168), V_TRANSLUCENT, "YOU HAVE THE BLUE FLAG");
		else
			V_DrawString(128, STRINGY(184), 0, "YOU HAVE THE BLUE FLAG");
	}
	if(stplyr->ctfteam == 1)
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(184), V_TRANSLUCENT, "YOU'RE ON THE RED TEAM");
		else
			V_DrawString(128, STRINGY(192), 0, "YOU'RE ON THE RED TEAM");
	}
	else if(stplyr->ctfteam == 2)
	{
		if(cv_splitscreen.value)
			V_DrawString(128, STRINGY(184), V_TRANSLUCENT, "YOU'RE ON THE BLUE TEAM");
		else
			V_DrawString(128, STRINGY(192), 0, "YOU'RE ON THE BLUE TEAM");
	}
	else
	{
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(88), V_TRANSLUCENT, "You are a spectator.");
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(104), V_TRANSLUCENT, "Jump on a team base to choose.");
	}
}

static void ST_drawChaosHUD(void)
{
	char chains[33];
	sprintf(chains, "CHAINS: %d", stplyr->scoreadd);
	V_DrawString(8, STRINGY(184), V_TRANSLUCENT, chains);
}

static void ST_drawSpecialStageHUD(void)
{
	if(cv_splitscreen.value)
		ST_DrawOverlayNum(SCX(hudinfo[HUD_SS_TOTALRINGS_SPLIT].x), SCY(hudinfo[HUD_SS_TOTALRINGS_SPLIT].y), totalrings, tallnum);
	else
		ST_DrawOverlayNum(SCX(hudinfo[HUD_SS_TOTALRINGS].x), SCY(hudinfo[HUD_SS_TOTALRINGS].y), totalrings, tallnum);

	if(leveltime < 5*TICRATE)
	{
		V_DrawScaledPatch(hudinfo[HUD_GETRINGS].x, (int)(SCY(hudinfo[HUD_GETRINGS].y)/vid.fdupy), V_TRANSLUCENT, getall);
		ST_DrawOverlayNum(SCX(hudinfo[HUD_GETRINGSNUM].x), SCY(hudinfo[HUD_GETRINGSNUM].y), totalrings, tallnum);
	}

	if(sstimer)
	{
		V_DrawString(hudinfo[HUD_TIMELEFT].x, STRINGY(hudinfo[HUD_TIMELEFT].y), 0, "TIME LEFT");
		ST_DrawNightsOverlayNum(SCX(hudinfo[HUD_TIMELEFTNUM].x), SCY(hudinfo[HUD_TIMELEFTNUM].y), sstimer/TICRATE, tallnum, 13);
	}
	else
		V_DrawScaledPatch(SCX(hudinfo[HUD_TIMEUP].x), SCY(hudinfo[HUD_TIMEUP].y), V_NOSCALESTART|V_TRANSLUCENT, timeup);
}

static void ST_drawEmeraldHuntIcon(mobj_t* hunt, int graphic)
{
	patch_t* p;
	int interval;
	fixed_t dist = P_AproxDistance(P_AproxDistance(stplyr->mo->x - hunt->x, stplyr->mo->y - hunt->y),
		stplyr->mo->z - hunt->z);

	if(dist < 128<<FRACBITS)
	{
		p = homing6;
		interval = 5;
	}
	else if(dist < 512<<FRACBITS)
	{
		p = homing5;
		interval = 10;
	}
	else if(dist < 1024<<FRACBITS)
	{
		p = homing4;
		interval = 20;
	}
	else if(dist < 2048<<FRACBITS)
	{
		p = homing3;
		interval = 30;
	}
	else if(dist < 3072<<FRACBITS)
	{
		p = homing2;
		interval = 35;
	}
	else
	{
		p = homing1;
		interval = 0;
	}

	V_DrawScaledPatch(hudinfo[graphic].x, STRINGY(hudinfo[graphic].y), V_TRANSLUCENT, p);
	if(interval > 0 && leveltime % interval == 0)
		S_StartSound(0, sfx_shotgn);
}

// Draw the status bar overlay, customisable: the user chooses which
// kind of information to overlay
//
/// \todo: Split up this 1400 line function into multiple functions!
//
static void ST_overlayDrawer(void)
{
	int splity;

	if(cv_splitscreen.value)
		splity = 24;
	else
		splity = 0;

	if(stplyr->linkcount)
	{
		int colornum;

		colornum = (stplyr->linkcount / 5)%14;

		if(cv_splitscreen.value)
		{
			ST_DrawNightsOverlayNum(SCX(256), SCY(160), stplyr->linkcount, nightsnum, colornum);
			V_DrawMappedPatch(SCX(264), SCY(160), V_NOSCALESTART, nightslink,
				(byte*)defaulttranslationtables - 256 + ((((0 & ~MF_TRANSLATION)
				| (colornum<<MF_TRANSSHIFT)) & MF_TRANSLATION) >> (MF_TRANSSHIFT-8)));
		}
		else
		{
			ST_DrawNightsOverlayNum(SCX(160), SCY(176), stplyr->linkcount, nightsnum, colornum);
			V_DrawMappedPatch(SCX(168), SCY(176), V_NOSCALESTART, nightslink,
				(byte*)defaulttranslationtables - 256 + ((((0 & ~MF_TRANSLATION)
				| (colornum<<MF_TRANSSHIFT)) & MF_TRANSLATION) >> (MF_TRANSSHIFT-8)));
		}
	}

	// lives status
	if((gametype == GT_COOP || gametype == GT_RACE))
	{
		V_DrawScaledPatch(SCX(hudinfo[HUD_LIVESNAME].x), SCY(hudinfo[HUD_LIVESNAME].y) - (faceprefix[stplyr->skin]->height*vid.dupy),
			V_NOSCALESTART|V_TRANSLUCENT, facenameprefix[stplyr->skin]);

		if(stplyr->powers[pw_super])
			V_DrawScaledPatch(SCX(hudinfo[HUD_LIVESPIC].x), SCY(hudinfo[HUD_LIVESPIC].y) - (faceprefix[stplyr->skin]->height*vid.dupy),
			V_NOSCALESTART|V_TRANSLUCENT, supersonic);
		else
		{
			byte* colormap;

			if(!stplyr->skincolor) // 'default' color
				colormap = colormaps;
			else
				colormap = (byte*)translationtables[stplyr->skin] - 256 + (stplyr->skincolor<<8);

			V_DrawMappedPatch(SCX(hudinfo[HUD_LIVESPIC].x), SCY(hudinfo[HUD_LIVESPIC].y) - (faceprefix[stplyr->skin]->height*vid.dupy),
				V_NOSCALESTART|V_TRANSLUCENT,faceprefix[stplyr->skin], colormap);
		}

		// draw the number of lives
		ST_DrawOverlayNum(SCX(hudinfo[HUD_LIVESNUM].x), SCY(hudinfo[HUD_LIVESNUM].y) - (11*vid.dupy), stplyr->lives, tallnum);

		// now draw the "x"
		if(cv_splitscreen.value)
			V_DrawScaledPatch(SCX(hudinfo[HUD_LIVESX].x), SCY(hudinfo[HUD_LIVESX].y-16), V_NOSCALESTART|V_TRANSLUCENT, stlivex);
		else
			V_DrawScaledPatch(SCX(hudinfo[HUD_LIVESX].x), SCY(hudinfo[HUD_LIVESX].y) - (stlivex->height*vid.dupy),
				V_NOSCALESTART|V_TRANSLUCENT, stlivex);
	}

	if(maptol & TOL_NIGHTS)
	{
		ST_drawNiGHTSHUD();
	}
	else
	{
		if(cv_splitscreen.value)
		{
			// rings counter
			ST_DrawOverlayNum(SCX(hudinfo[HUD_RINGSNUMSPLIT].x), SCY(hudinfo[HUD_RINGSNUMSPLIT].y), stplyr->health > 0 ? stplyr->health - 1 : 0,
				tallnum);

			if(stplyr->health <= 1 && leveltime/5 & 1)
				V_DrawScaledPatch(SCX(hudinfo[HUD_RINGSSPLIT].x), SCY(hudinfo[HUD_RINGSSPLIT].y), V_NOSCALESTART|V_TRANSLUCENT, rrings);
			else
				V_DrawScaledPatch(SCX(hudinfo[HUD_RINGSSPLIT].x), SCY(hudinfo[HUD_RINGSSPLIT].y), V_NOSCALESTART|V_TRANSLUCENT, sborings);
		}
		else
		{
			if(gamemap >= sstage_start && gamemap <= sstage_end)
			{
				int ringscollected = 0; // Total # everyone has collected
				int i;

				for(i = 0; i < MAXPLAYERS; i++)
					if(playeringame[i] && players[i].mo && players[i].mo->health > 1)
						ringscollected += players[i].mo->health - 1;

				ST_DrawOverlayNum(SCX(hudinfo[HUD_RINGSNUM].x), SCY(hudinfo[HUD_RINGSNUM].y), ringscollected, tallnum);
			}
			else
			{
				ST_DrawOverlayNum(SCX(hudinfo[HUD_RINGSNUM].x), SCY(hudinfo[HUD_RINGSNUM].y), stplyr->health > 0 ? stplyr->health-1 : 0,
					tallnum);
			}

			if(stplyr->health <= 1 && leveltime/5 & 1)
				V_DrawScaledPatch(SCX(hudinfo[HUD_RINGS].x), SCY(hudinfo[HUD_RINGS].y), V_NOSCALESTART|V_TRANSLUCENT, rrings);
			else
				V_DrawScaledPatch(SCX(hudinfo[HUD_RINGS].x), SCY(hudinfo[HUD_RINGS].y), V_NOSCALESTART|V_TRANSLUCENT, sborings);
		}

		// draw score (same in splitscreen as normal, too!)
		ST_DrawOverlayNum(SCX(hudinfo[HUD_SCORENUM].x), SCY(hudinfo[HUD_SCORENUM].y), stplyr->score, tallnum);
		V_DrawScaledPatch(SCX(hudinfo[HUD_SCORE].x), SCY(hudinfo[HUD_SCORE].y), V_NOSCALESTART|V_TRANSLUCENT, sboscore);

		if(cv_splitscreen.value)
		{
			int seconds = stplyr->realtime/TICRATE % 60;

			if(seconds < 10)
				ST_DrawOverlayNum(SCX(hudinfo[HUD_LOWSECONDSSPLIT].x), SCY(hudinfo[HUD_LOWSECONDSSPLIT].y), 0, tallnum);

			// seconds time
			ST_DrawOverlayNum(SCX(hudinfo[HUD_SECONDSSPLIT].x), SCY(hudinfo[HUD_SECONDSSPLIT].y), stplyr->realtime/TICRATE % 60, tallnum);

			// minutes time
			ST_DrawOverlayNum(SCX(hudinfo[HUD_MINUTESSPLIT].x), SCY(hudinfo[HUD_MINUTESSPLIT].y), stplyr->realtime/(60*TICRATE), tallnum);

			// colon location
			V_DrawScaledPatch(SCX(hudinfo[HUD_TIMECOLONSPLIT].x), SCY(hudinfo[HUD_TIMECOLONSPLIT].y), V_NOSCALESTART|V_TRANSLUCENT, sbocolon);

			// TIME location
			V_DrawScaledPatch(SCX(hudinfo[HUD_TIMESPLIT].x), SCY(hudinfo[HUD_TIMESPLIT].y), V_NOSCALESTART|V_TRANSLUCENT, sbotime);
		}
		else
		{
			if(cv_timetic.value) // show tics instead of MM:SS
				ST_DrawOverlayNum(SCX(hudinfo[HUD_SECONDS].x), SCY(hudinfo[HUD_SECONDS].y), stplyr->realtime, tallnum);
			else
			{
				int seconds = stplyr->realtime/TICRATE % 60;

				if(seconds < 10)
					ST_DrawOverlayNum(SCX(hudinfo[HUD_LOWSECONDS].x), SCY(hudinfo[HUD_LOWSECONDS].y), 0, tallnum);

				// seconds time
				ST_DrawOverlayNum(SCX(hudinfo[HUD_SECONDS].x), SCY(hudinfo[HUD_SECONDS].y), stplyr->realtime/TICRATE % 60, tallnum);

				// minutes time
				ST_DrawOverlayNum(SCX(hudinfo[HUD_MINUTES].x), SCY(hudinfo[HUD_MINUTES].y), stplyr->realtime/(60*TICRATE), tallnum);

				// colon location
				V_DrawScaledPatch(SCX(hudinfo[HUD_TIMECOLON].x), SCY(hudinfo[HUD_TIMECOLON].y), V_NOSCALESTART|V_TRANSLUCENT, sbocolon);
			}

			// TIME location
			V_DrawScaledPatch(SCX(hudinfo[HUD_TIME].x), SCY(hudinfo[HUD_TIME].y), V_NOSCALESTART|V_TRANSLUCENT, sbotime);
		}
	}

	// GAME OVER pic
	if((gametype == GT_COOP || gametype == GT_RACE) && stplyr->lives <= 0)
	{
		patch_t* p;

		if(countdown == 1)
			p = timeover;
		else
			p = sboover;

		V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, STRINGY(BASEVIDHEIGHT/2 - (p->height/2)), 0, p);
	}

	if(cv_objectplace.value && stplyr->mo && stplyr->mo->target)
	{
		char x[8], y[8], z[8];
		char doomednum[8], thingflags[8];
		sprintf(x, "%d", stplyr->mo->x >> FRACBITS);
		sprintf(y, "%d", stplyr->mo->y >> FRACBITS);
		sprintf(z, "%d", stplyr->mo->z >> FRACBITS);
		sprintf(doomednum, "%d", stplyr->mo->target->info->doomednum);
		sprintf(thingflags, "%d", cv_objflags.value);
		V_DrawString(16, 98, 0, "X =");
		V_DrawString(48, 98, 0, x);
		V_DrawString(16, 108, 0, "Y =");
		V_DrawString(48, 108, 0, y);
		V_DrawString(16, 118, 0, "Z =");
		V_DrawString(48, 118, 0, z);
		V_DrawString(16, 128, 0, "thing # =");
		V_DrawString(16+84, 128, 0, doomednum);
		V_DrawString(16, 138, 0, "flags =");
		V_DrawString(16+56, 138, 0, thingflags);
		V_DrawString(16, 148, 0, "snap =");
		V_DrawString(16+48, 148, 0, cv_snapto.string);
	}

	if(!(netgame || multiplayer) && eastermode && !modifiedgame)
	{
		char eggsfound[20];
		unsigned v = (unsigned)foundeggs, found;

		// count the number of bits set in foundeggs, thanks to Brian Kernighan
		// (assume only lower 12 bits can be used)
		for(found = 0; v; found++)
			v &= v - 1;

		sprintf(eggsfound, "Eggs Found: %d/%d", (int)found, NUMEGGS);
		V_DrawString(184, STRINGY(168), V_TRANSLUCENT, eggsfound);
	}

	// Countdown timer for Race Mode
	if(countdown)
	{
		char scountdown[33];
		sprintf(scountdown, "%d", countdown/TICRATE);
		V_DrawCenteredString(BASEVIDWIDTH/2, STRINGY(176), 0, scountdown);
	}

	// Tag HUD Stuff
	if(gametype == GT_TAG)
		ST_drawTagHUD();
	// CTF HUD Stuff
	else if(gametype == GT_CTF)
		ST_drawCTFHUD();
	// Chaos HUD Stuff
	else if(gametype == GT_CHAOS)
		ST_drawChaosHUD();

	// Special Stage HUD
	if(gamemap >= sstage_start && gamemap <= sstage_end)
		ST_drawSpecialStageHUD();

	// Emerald Hunt Indicators
	if(hunt1 && hunt1->health)
		ST_drawEmeraldHuntIcon(hunt1, HUD_HUNTPIC1);
	if(hunt2 && hunt2->health)
		ST_drawEmeraldHuntIcon(hunt2, HUD_HUNTPIC2);
	if(hunt3 && hunt3->health)
		ST_drawEmeraldHuntIcon(hunt3, HUD_HUNTPIC3);

	if(stplyr->powers[pw_railring] > 5*TICRATE || (stplyr->powers[pw_railring] && leveltime & 1))
	{
		char railringpower[4];
		V_DrawScaledPatch(8, STRINGY(56 - splity), V_SNAPTOLEFT|V_TRANSLUCENT, railring);
		sprintf(railringpower, "%d", stplyr->powers[pw_railring]/TICRATE);
		V_DrawString(32, STRINGY(56 - splity + 4), V_TRANSLUCENT | V_SNAPTOLEFT, railringpower);
	}

	if(stplyr->powers[pw_homingring] > 5*TICRATE || (stplyr->powers[pw_homingring] && leveltime & 1))
	{
		char homingringpower[4];
		V_DrawScaledPatch(8, STRINGY(88 - splity), V_SNAPTOLEFT|V_TRANSLUCENT, homingring);
		sprintf(homingringpower, "%d", stplyr->powers[pw_homingring]/TICRATE);
		V_DrawString(32, STRINGY(88 - splity + 4), V_TRANSLUCENT | V_SNAPTOLEFT, homingringpower);
	}

	if(stplyr->powers[pw_infinityring] > 5*TICRATE || (stplyr->powers[pw_infinityring] && leveltime & 1))
	{
		char infinityringpower[4];
		V_DrawScaledPatch(8, STRINGY(120 - splity), V_SNAPTOLEFT|V_TRANSLUCENT, infinityring);
		sprintf(infinityringpower, "%d", stplyr->powers[pw_infinityring]/TICRATE);
		V_DrawString(32, STRINGY(120 - splity + 4), V_TRANSLUCENT | V_SNAPTOLEFT, infinityringpower);
	}

	if(stplyr->powers[pw_explosionring] > 5*TICRATE || (stplyr->powers[pw_explosionring] && leveltime & 1))
	{
		char explosionringpower[4];
		V_DrawScaledPatch(8, STRINGY(152 - splity), V_SNAPTOLEFT|V_TRANSLUCENT, explosionring);
		sprintf(explosionringpower, "%d", stplyr->powers[pw_explosionring]/TICRATE);
		V_DrawString(32, STRINGY(152 - splity + 4), V_TRANSLUCENT | V_SNAPTOLEFT, explosionringpower);
	}

	if(stplyr->powers[pw_automaticring] > 5*TICRATE || (stplyr->powers[pw_automaticring] && leveltime & 1))
	{
		char automaticringpower[4];
		V_DrawScaledPatch(8, STRINGY(184 - splity), V_SNAPTOLEFT|V_TRANSLUCENT, autoring);
		sprintf(automaticringpower, "%d", stplyr->powers[pw_automaticring]/TICRATE);
		V_DrawString(32, STRINGY(184 - splity + 4), V_TRANSLUCENT | V_SNAPTOLEFT, automaticringpower);
	}

	if(stplyr->lightdashallowed)
	{
		V_DrawFill(hudinfo[HUD_LIGHTDASHBOX].x, hudinfo[HUD_LIGHTDASHBOX].y, 72, 12, 119);
		V_DrawCenteredString(hudinfo[HUD_LIGHTDASHSTRING1].x, hudinfo[HUD_LIGHTDASHSTRING1].y, V_TRANSLUCENT, "ACTIVATE");
		V_DrawCenteredString(hudinfo[HUD_LIGHTDASHSTRING2].x, hudinfo[HUD_LIGHTDASHSTRING2].y, V_TRANSLUCENT, "LIGHT DASH");
	}

	// This is where we draw all the fun cheese if you have the chasecam off!
	if((stplyr == &players[consoleplayer] && !cv_chasecam.value)
		|| ((cv_splitscreen.value && stplyr == &players[secondarydisplayplayer])
		&& !cv_chasecam2.value))
	{
		ST_drawFirstPersonHUD();
	}

	if(mariomode && stplyr->exiting)
	{
		/// \todo doesn't belong in status bar code AT ALL
		thinker_t* th;
		mobj_t* mo2;
		boolean foundtoad = false;

		// scan the remaining thinkers
		// to find toad
		for(th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if(th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t*)th;
			if(mo2->type == MT_TOAD)
			{
				foundtoad = true;
				break;
			}
		}

		if(foundtoad)
		{
			V_DrawCenteredString(160, 32+16, 0, "Thank you!");
			V_DrawCenteredString(160, 44+16, 0, "But our earless leader is in");
			V_DrawCenteredString(160, 56+16, 0, "another castle!");
		}
	}

	// draw level title Tails
	if(*mapheaderinfo[gamemap-1].lvlttl != '\0')
		ST_drawLevelTitle();

	if(stplyr->deadtimer > 0 && (stplyr->deadtimer < gameovertics) && stplyr->lives <= 0)
	{
		if(!netgame && !multiplayer)
		{
			if(stplyr->continues != 0) // Player has continues, so let's use them!
			{
				char stimeleft[33];
				patch_t* contsonic;
				// Do continue screen here.
				// Initialize music
				// For some reason the code doesn't like a simple ==...
				if(stplyr->deadtimer < gameovertics && stplyr->deadtimer > gameovertics - 2)
				{
					tic_t wipestart, tics, nowtime;
					int y;
					boolean done;
					// Force a screen wipe

					if(rendermode == render_soft)
					{
						// First, read the current screen
						F_WipeStartScreen();

						// Then, draw what the new screen will look like.
						V_DrawFill(0, 0, vid.width, vid.height, 0);

						contsonic = W_CachePatchName("CONT1", PU_CACHE);
						V_DrawScaledPatch((320-contsonic->width)/2, 64, 0, contsonic);
						V_DrawString(128,128,0, "CONTINUE?");
						sprintf(stimeleft, "%d", (stplyr->deadtimer - (gameovertics-11*TICRATE))/TICRATE);
						V_DrawString(stplyr->deadtimer >= (gameovertics-TICRATE) ? 152 : 160,144,0, stimeleft);

						// Now, read the end screen we want to fade to.
						F_WipeEndScreen(0, 0, vid.width, vid.height);

						S_ChangeMusic(mus_contsc, false);

						// Do the wipe-io!
						wipestart = I_GetTime() - 1;
						y = wipestart + 2*TICRATE; // init a timeout
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
				}
				V_DrawFill(0, 0, vid.width, vid.height, 0);
				V_DrawString(128, 128, 0, "CONTINUE?");
				// Draw a Sonic!
				contsonic = W_CachePatchName("CONT1", PU_CACHE);
				V_DrawScaledPatch((320 - contsonic->width)/2, 64, 0, contsonic);
				sprintf(stimeleft, "%d", (stplyr->deadtimer - (gameovertics-11*TICRATE))/TICRATE);
				V_DrawString(stplyr->deadtimer >= (gameovertics-TICRATE) ? 152 : 160, 144, 0, stimeleft);
				if(stplyr->deadtimer < (gameovertics-10*TICRATE))
					Command_ExitGame_f();
				if(stplyr->deadtimer < gameovertics-TICRATE && (stplyr->cmd.buttons & BT_JUMP || stplyr->cmd.buttons & BT_USE))
				{
					if(stplyr->continues != -1)
						stplyr->continues--;

					// Reset score
					stplyr->score = 0;

					// Allow tokens to come back if not a netgame.
					if(!(netgame || multiplayer))
					{
						tokenlist = 0;
						token = 0;
					}

					// Reset # of lives
					switch(gameskill)
					{
						case sk_insane:
							stplyr->lives = 1;
							break;
						case sk_nightmare:
						case sk_hard:
						case sk_medium:
							stplyr->lives = 3;
							break;
						case sk_easy:
							stplyr->lives = 5;
							break;
						case sk_baby:
							stplyr->lives = 9;
							break;
						default: // Oops!?
							CONS_Printf("ERROR: PLAYER SKILL UNDETERMINED!");
							break;
					}
					// Clear any starpost data
					stplyr->starpostangle = 0;
					stplyr->starpostbit = 0;
					stplyr->starpostnum = 0;
					stplyr->starposttime = 0;
					stplyr->starpostx = 0;
					stplyr->starposty = 0;
					stplyr->starpostz = 0;
					contsonic = W_CachePatchName("CONT2", PU_CACHE);
					V_DrawScaledPatch((320 - contsonic->width)/2, 64, 0, contsonic);
				}
			}
			else // Just go to the title screen
				Command_ExitGame_f();
		}
	}

	if(cv_debug)
		ST_drawDebugInfo();
}
