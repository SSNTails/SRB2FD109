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
/// \brief Thing frame/state LUT

// Data.
#include "doomdef.h"
#include "sounds.h"
#include "p_mobj.h"
#include "z_zone.h"
#include "d_player.h"
#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

const char* sprnames[NUMSPRITES + 1] =
{
	"MISL","PLAY","POSS","SPOS","EGGM","BON1","SRBX","GRBX","EMMY","PINV",
	"BLTV","SPRY","SUDY","SHTV","FANS","BUBL","YLTV","FWR1","SPRR","SUDR",
	"SMOK","SPLA","TNT1","BIRD","SQRL","YORB","BORB","KORB","SPRK","IVSP",
	"IVSQ","DISS","BUBP","BUBO","BUBN","BUBM","CNTA","CNTB","CNTC","CNTD",
	"CNTE","CNTF","POPP","PRUP","BKTV","SCRA","SCRB","SCRC","SCRD","SSPK",
	"GRAS","YSPR","RSPR","YSUD","RSUD","SKIM","MINE","FISH","GARG","SPLH",
	"THOK","THZP","SIGN","RRNG","TTAG","STEM","RFLG","BFLG","GFLG","TOKE",
	"CEMG","CEMO","CEMP","CEMB","CEMR","CEML","CEMY","JETB","JETG","JBUL",
	"MOUS","DETN","XPLD","CHAN","CAPE","SNO1","SANT","EMER","EMES","EMET",
	"SBLL","SPIK","CCOM","RAIN","DSPK","USPK","STPT","RNGM","RNGR","RNGS",
	"RNGA","RNGE","TAEH","TAER","THER","TAHR","THOM","TAUT","TEXP","BUS1",
	"BUS2","FWR2","FWR3","MIXU","QUES","MTEX","FLAM","PUMA","HAMM","KOOP",
	"SHLL","MAXE","BFLM","FBLL","FFWR","NSPK","SUPE","SUPZ","NDRN","NDRL",
	"SEED","JETF","HOOP","NSCR","NWNG","EGGN","GOOP","BPLD","ALRM","RDTV",
	"RORB","EGGB","SFLM","TNKA","TNKB","SPNK","TFOG","EEGG","LITE","TRET",
	"TRLS","FWR4","GOOM","BGOM","MUS1","MUS2","TOAD","COIN","CPRK","XMS1",
	"XMS2","XMS3","CAPS","SUPT","ROIA","ROIB","ROIC","ROID","ROIE","ROIF",
	"ROIG","ROIH","ROII","ROIJ","ROIK","ROIL","ROIM","ROIN","ROIO","ROIP",
	"NPRA","NPRB","NPRC","REDX","SPRB","BUZZ","RBUZ","CEMK","WHTV","WORB",
	"TURR",
};

// Doesn't work with g++, needs actionf_p1 (don't modify this comment)
state_t states[NUMSTATES] =
{
	// frame is masked through FF_FRAMEMASK
	// FF_FULLBRIGHT (0x8000) activates the fullbright colormap
	// Keep this comment directly above S_NULL.
	{SPR_DISS, 0, -1, {NULL}, S_NULL}, // S_NULL
	{SPR_MISL, 32768, 1, {A_SmokeTrailer}, S_ROCKET}, // S_ROCKET

	// Player
	{SPR_PLAY, 0, 105, {NULL}, S_PLAY_TAP1},       // S_PLAY_STND
	{SPR_PLAY, 1, 16, {NULL}, S_PLAY_TAP2},        // S_PLAY_TAP1
	{SPR_PLAY, 2, 16, {NULL}, S_PLAY_TAP1},        // S_PLAY_TAP2
	{SPR_PLAY, 3, 4, {NULL}, S_PLAY_RUN2},         // S_PLAY_RUN1
	{SPR_PLAY, 4, 4, {NULL}, S_PLAY_RUN3},         // S_PLAY_RUN2
	{SPR_PLAY, 5, 4, {NULL}, S_PLAY_RUN4},         // S_PLAY_RUN3
	{SPR_PLAY, 6, 4, {NULL}, S_PLAY_RUN5},         // S_PLAY_RUN4
	{SPR_PLAY, 7, 4, {NULL}, S_PLAY_RUN6},         // S_PLAY_RUN5
	{SPR_PLAY, 8, 4, {NULL}, S_PLAY_RUN7},         // S_PLAY_RUN6
	{SPR_PLAY, 9, 4, {NULL}, S_PLAY_RUN8},         // S_PLAY_RUN7
	{SPR_PLAY, 10, 4, {NULL}, S_PLAY_RUN1},        // S_PLAY_RUN8
	{SPR_PLAY, 11, 1, {NULL}, S_PLAY_ATK2},        // S_PLAY_ATK1
	{SPR_PLAY, 12, 1, {NULL}, S_PLAY_ATK3},        // S_PLAY_ATK2
	{SPR_PLAY, 13, 1, {NULL}, S_PLAY_ATK4},        // S_PLAY_ATK3
	{SPR_PLAY, 14, 1, {NULL}, S_PLAY_ATK1},        // S_PLAY_ATK4
	{SPR_PLAY, 15, -1, {NULL}, S_NULL},            // S_PLAY_PLG1
	{SPR_PLAY, 16, 2, {NULL}, S_PLAY_SPD2},        // S_PLAY_SPD1
	{SPR_PLAY, 17, 2, {NULL}, S_PLAY_SPD3},        // S_PLAY_SPD2
	{SPR_PLAY, 18, 2, {NULL}, S_PLAY_SPD4},        // S_PLAY_SPD3
	{SPR_PLAY, 19, 2, {NULL}, S_PLAY_SPD1},        // S_PLAY_SPD4
	{SPR_PLAY, 20, 2, {NULL}, S_PLAY_ABL2},        // S_PLAY_ABL1
	{SPR_PLAY, 21, 2, {NULL}, S_PLAY_ABL1},        // S_PLAY_ABL2
	{SPR_PLAY, 22, 6, {NULL}, S_PLAY_SPC2},        // S_PLAY_SPC1
	{SPR_PLAY, 23, 6, {NULL}, S_PLAY_SPC3},        // S_PLAY_SPC2
	{SPR_PLAY, 24, 6, {NULL}, S_PLAY_SPC4},        // S_PLAY_SPC3
	{SPR_PLAY, 25, 6, {NULL}, S_PLAY_SPC1},        // S_PLAY_SPC4
	{SPR_PLAY, 22, -1, {NULL}, S_NULL},            // S_PLAY_CLIMB1
	{SPR_PLAY, 23, 5, {NULL}, S_PLAY_CLIMB3},      // S_PLAY_CLIMB2
	{SPR_PLAY, 24, 5, {NULL}, S_PLAY_CLIMB4},      // S_PLAY_CLIMB3
	{SPR_PLAY, 25, 5, {NULL}, S_PLAY_CLIMB5},      // S_PLAY_CLIMB4
	{SPR_PLAY, 24, 5, {NULL}, S_PLAY_CLIMB2},      // S_PLAY_CLIMB5
	{SPR_PLAY, 26, 14, {NULL}, S_PLAY_RUN1},       // S_PLAY_GASP
	{SPR_PLAY, 27, -1, {NULL}, S_PLAY_STND},       // S_PLAY_PAIN
	{SPR_PLAY, 28, 8, {A_Fall}, S_PLAY_DIE2},      // S_PLAY_DIE1
	{SPR_PLAY, 28, 7, {NULL}, S_PLAY_DIE3},        // S_PLAY_DIE2
	{SPR_PLAY, 28, -1, {NULL}, S_NULL},            // S_PLAY_DIE3
	{SPR_PLAY, 29, 12, {NULL}, S_PLAY_TEETER2},    // S_PLAY_TEETER1
	{SPR_PLAY, 30, 12, {NULL}, S_PLAY_TEETER1},    // S_PLAY_TEETER2
	{SPR_PLAY, 31, 2, {NULL}, S_PLAY_FALL2},       // S_PLAY_FALL1
	{SPR_PLAY, 32, 2, {NULL}, S_PLAY_FALL1},       // S_PLAY_FALL2
	{SPR_PLAY, 33, -1, {NULL}, S_NULL},            // S_PLAY_CARRY
	{SPR_PLAY, 20, -1, {NULL}, S_PLAY_SUPERSTAND}, // S_PLAY_SUPERSTAND
	{SPR_PLAY, 20, 7, {NULL}, S_PLAY_SUPERWALK2},  // S_PLAY_SUPERWALK1
	{SPR_PLAY, 21, 7, {NULL}, S_PLAY_SUPERWALK1},  // S_PLAY_SUPERWALK2
	{SPR_PLAY, 22, 7, {NULL}, S_PLAY_SUPERFLY2},   // S_PLAY_SUPERFLY1
	{SPR_PLAY, 23, 7, {NULL}, S_PLAY_SUPERFLY1},   // S_PLAY_SUPERFLY2
	{SPR_PLAY, 24, 12, {NULL}, S_PLAY_SUPERTEETER},// S_PLAY_SUPERTEETER
	{SPR_PLAY, 25, -1, {NULL}, S_PLAY_SUPERSTAND}, // S_PLAY_SUPERHIT

	// Blue Crawla
	{SPR_POSS, 0, 5, {A_Look}, S_POSS_STND2},   // S_POSS_STND
	{SPR_POSS, 0, 5, {A_Look}, S_POSS_STND},    // S_POSS_STND2
	{SPR_POSS, 0, 3, {A_Chase}, S_POSS_RUN2},   // S_POSS_RUN1
	{SPR_POSS, 1, 3, {A_Chase}, S_POSS_RUN3},   // S_POSS_RUN2
	{SPR_POSS, 2, 3, {A_Chase}, S_POSS_RUN4},   // S_POSS_RUN3
	{SPR_POSS, 3, 3, {A_Chase}, S_POSS_RUN5},   // S_POSS_RUN4
	{SPR_POSS, 4, 3, {A_Chase}, S_POSS_RUN6},   // S_POSS_RUN5
	{SPR_POSS, 5, 3, {A_Chase}, S_POSS_RUN7},   // S_POSS_RUN6
	{SPR_POSS, 6, 3, {A_Chase}, S_POSS_RUN8},   // S_POSS_RUN7
	{SPR_POSS, 7, 3, {A_Chase}, S_POSS_RUN9},   // S_POSS_RUN8
	{SPR_POSS, 8, 3, {A_Chase}, S_POSS_RUN10},  // S_POSS_RUN9
	{SPR_POSS, 9, 3, {A_Chase}, S_POSS_RUN11},  // S_POSS_RUN10
	{SPR_POSS, 10, 3, {A_Chase}, S_POSS_RUN12}, // S_POSS_RUN11
	{SPR_POSS, 11, 3, {A_Chase}, S_POSS_RUN13}, // S_POSS_RUN12
	{SPR_POSS, 12, 3, {A_Chase}, S_POSS_RUN14}, // S_POSS_RUN13
	{SPR_POSS, 13, 3, {A_Chase}, S_POSS_RUN15}, // S_POSS_RUN14
	{SPR_POSS, 14, 3, {A_Chase}, S_POSS_RUN16}, // S_POSS_RUN15
	{SPR_POSS, 15, 3, {A_Chase}, S_POSS_RUN17}, // S_POSS_RUN16
	{SPR_POSS, 16, 3, {A_Chase}, S_POSS_RUN1},  // S_POSS_RUN17
	{SPR_POSS, 17, 1, {A_Scream}, S_POSS_DIE2}, // S_POSS_DIE1
	{SPR_POSS, 18, 5, {NULL}, S_POSS_DIE3},     // S_POSS_DIE2
	{SPR_POSS, 19, 5, {NULL}, S_POSS_DIE4},     // S_POSS_DIE3
	{SPR_POSS, 20, 5, {NULL}, S_DISS},          // S_POSS_DIE4

	// Red Crawla
	{SPR_SPOS, 0, 5, {A_Look}, S_SPOS_STND2},   // S_SPOS_STND
	{SPR_SPOS, 0, 5, {A_Look}, S_SPOS_STND},    // S_SPOS_STND2
	{SPR_SPOS, 0, 1, {A_Chase}, S_SPOS_RUN2},   // S_SPOS_RUN1
	{SPR_SPOS, 1, 1, {A_Chase}, S_SPOS_RUN3},   // S_SPOS_RUN2
	{SPR_SPOS, 2, 1, {A_Chase}, S_SPOS_RUN4},   // S_SPOS_RUN3
	{SPR_SPOS, 3, 1, {A_Chase}, S_SPOS_RUN5},   // S_SPOS_RUN4
	{SPR_SPOS, 4, 1, {A_Chase}, S_SPOS_RUN6},   // S_SPOS_RUN5
	{SPR_SPOS, 5, 1, {A_Chase}, S_SPOS_RUN7},   // S_SPOS_RUN6
	{SPR_SPOS, 6, 1, {A_Chase}, S_SPOS_RUN8},   // S_SPOS_RUN7
	{SPR_SPOS, 7, 1, {A_Chase}, S_SPOS_RUN9},   // S_SPOS_RUN8
	{SPR_SPOS, 8, 1, {A_Chase}, S_SPOS_RUN10},  // S_SPOS_RUN9
	{SPR_SPOS, 9, 1, {A_Chase}, S_SPOS_RUN11},  // S_SPOS_RUN10
	{SPR_SPOS, 10, 1, {A_Chase}, S_SPOS_RUN12}, // S_SPOS_RUN11
	{SPR_SPOS, 11, 1, {A_Chase}, S_SPOS_RUN13}, // S_SPOS_RUN12
	{SPR_SPOS, 12, 1, {A_Chase}, S_SPOS_RUN14}, // S_SPOS_RUN13
	{SPR_SPOS, 13, 1, {A_Chase}, S_SPOS_RUN15}, // S_SPOS_RUN14
	{SPR_SPOS, 14, 1, {A_Chase}, S_SPOS_RUN16}, // S_SPOS_RUN15
	{SPR_SPOS, 15, 1, {A_Chase}, S_SPOS_RUN17}, // S_SPOS_RUN16
	{SPR_SPOS, 16, 1, {A_Chase}, S_SPOS_RUN1},  // S_SPOS_RUN17
	{SPR_SPOS, 17, 1, {A_Scream}, S_SPOS_DIE2}, // S_SPOS_DIE1
	{SPR_SPOS, 18, 5, {NULL}, S_SPOS_DIE3},     // S_SPOS_DIE2
	{SPR_SPOS, 19, 5, {NULL}, S_SPOS_DIE4},     // S_SPOS_DIE3
	{SPR_SPOS, 20, 5, {NULL}, S_DISS},          // S_SPOS_DIE4

	// Boss 1
	{SPR_EGGM, 0, 1, {NULL}, S_EGGMOBILE_STND},           // S_EGGMOBILE_STND
	{SPR_EGGM, 1, 35, {NULL}, S_EGGMOBILE_ATK2},          // S_EGGMOBILE_ATK1
	{SPR_EGGM, 2, 35, {A_CyberAttack}, S_EGGMOBILE_STND}, // S_EGGMOBILE_ATK2
	{SPR_EGGM, 3, 35, {NULL}, S_EGGMOBILE_ATK4},          // S_EGGMOBILE_ATK3
	{SPR_EGGM, 4, 35, {A_CyberAttack}, S_EGGMOBILE_STND}, // S_EGGMOBILE_ATK4
	{SPR_EGGM, 1, 35, {NULL}, S_EGGMOBILE_PANIC2},        // S_EGGMOBILE_PANIC1
	{SPR_EGGM, 2, 35, {A_SkullAttack}, S_EGGMOBILE_STND}, // S_EGGMOBILE_PANIC2
	{SPR_EGGM, 5, 24, {A_Pain}, S_EGGMOBILE_STND},        // S_EGGMOBILE_PAIN
	{SPR_EGGM, 6, 8, {A_Fall}, S_EGGMOBILE_DIE2},         // S_EGGMOBILE_DIE1
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE3},   // S_EGGMOBILE_DIE2
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE4},   // S_EGGMOBILE_DIE3
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE5},   // S_EGGMOBILE_DIE4
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE6},   // S_EGGMOBILE_DIE5
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE7},   // S_EGGMOBILE_DIE6
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE8},   // S_EGGMOBILE_DIE7
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE9},   // S_EGGMOBILE_DIE8
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE10},  // S_EGGMOBILE_DIE9
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE11},  // S_EGGMOBILE_DIE10
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE12},  // S_EGGMOBILE_DIE11
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE13},  // S_EGGMOBILE_DIE12
	{SPR_EGGM, 6, 8, {A_BossScream}, S_EGGMOBILE_DIE14},  // S_EGGMOBILE_DIE13
	{SPR_EGGM, 6, -1, {A_BossDeath}, S_NULL},             // S_EGGMOBILE_DIE14
	{SPR_EGGM, 7, 5, {NULL}, S_EGGMOBILE_FLEE2},          // S_EGGMOBILE_FLEE1
	{SPR_EGGM, 8, 5, {NULL}, S_EGGMOBILE_FLEE1},          // S_EGGMOBILE_FLEE2

	// Ring
	{SPR_BON1, 0, 1, {A_AttractChase}, S_BON1A},  // S_BON1
	{SPR_BON1, 1, 1, {A_AttractChase}, S_BON1B},  // S_BON1A
	{SPR_BON1, 2, 1, {A_AttractChase}, S_BON1C},  // S_BON1B
	{SPR_BON1, 3, 1, {A_AttractChase}, S_BON1D},  // S_BON1C
	{SPR_BON1, 4, 1, {A_AttractChase}, S_BON1E},  // S_BON1D
	{SPR_BON1, 5, 1, {A_AttractChase}, S_BON1F},  // S_BON1E
	{SPR_BON1, 6, 1, {A_AttractChase}, S_BON1G},  // S_BON1F
	{SPR_BON1, 7, 1, {A_AttractChase}, S_BON1H},  // S_BON1G
	{SPR_BON1, 8, 1, {A_AttractChase}, S_BON1I},  // S_BON1H
	{SPR_BON1, 9, 1, {A_AttractChase}, S_BON1J},  // S_BON1I
	{SPR_BON1, 10, 1, {A_AttractChase}, S_BON1K}, // S_BON1J
	{SPR_BON1, 11, 1, {A_AttractChase}, S_BON1L}, // S_BON1K
	{SPR_BON1, 12, 1, {A_AttractChase}, S_BON1M}, // S_BON1L
	{SPR_BON1, 13, 1, {A_AttractChase}, S_BON1N}, // S_BON1M
	{SPR_BON1, 14, 1, {A_AttractChase}, S_BON1O}, // S_BON1N
	{SPR_BON1, 15, 1, {A_AttractChase}, S_BON1P}, // S_BON1O
	{SPR_BON1, 16, 1, {A_AttractChase}, S_BON1Q}, // S_BON1P
	{SPR_BON1, 17, 1, {A_AttractChase}, S_BON1R}, // S_BON1Q
	{SPR_BON1, 18, 1, {A_AttractChase}, S_BON1S}, // S_BON1R
	{SPR_BON1, 19, 1, {A_AttractChase}, S_BON1T}, // S_BON1S
	{SPR_BON1, 20, 1, {A_AttractChase}, S_BON1U}, // S_BON1T
	{SPR_BON1, 21, 1, {A_AttractChase}, S_BON1V}, // S_BON1U
	{SPR_BON1, 22, 1, {A_AttractChase}, S_BON1W}, // S_BON1V
	{SPR_BON1, 23, 1, {A_AttractChase}, S_BON1},  // S_BON1W

	// Super Ring Box
	{SPR_SRBX, 0, 2, {NULL}, S_SUPERRINGBOX1},          // S_SUPERRINGBOX
	{SPR_MTEX, 0, 1, {NULL}, S_SUPERRINGBOX},           // S_SUPERRINGBOX1
	{SPR_SRBX, 1, 18, {A_MonitorPop}, S_SUPERRINGBOX3}, // S_SUPERRINGBOX2
	{SPR_SRBX, 1, 18, {A_RingBox}, S_DISS},             // S_SUPERRINGBOX3

	// Silver Ring Box
	{SPR_GRBX, 0, 2, {NULL}, S_GREYRINGBOX1},          // S_GREYRINGBOX
	{SPR_MTEX, 0, 1, {NULL}, S_GREYRINGBOX},           // S_GREYRINGBOX1
	{SPR_GRBX, 1, 18, {A_MonitorPop}, S_GREYRINGBOX3}, // S_GREYRINGBOX2
	{SPR_GRBX, 1, 18, {A_RingBox}, S_DISS},            // S_GREYRINGBOX3

	// Special Stage Token
	{SPR_EMMY, 32768, 2, {NULL}, S_EMMY2}, // S_EMMY1
	{SPR_EMMY, 32769, 2, {NULL}, S_EMMY3}, // S_EMMY2
	{SPR_EMMY, 32770, 2, {NULL}, S_EMMY4}, // S_EMMY3
	{SPR_EMMY, 32771, 2, {NULL}, S_EMMY5}, // S_EMMY4
	{SPR_EMMY, 32772, 2, {NULL}, S_EMMY6}, // S_EMMY5
	{SPR_EMMY, 32773, 2, {NULL}, S_EMMY7}, // S_EMMY6
	{SPR_EMMY, 32774, 2, {NULL}, S_EMMY1}, // S_EMMY7

	// Invincibility Box
	{SPR_PINV, 0, 2, {NULL}, S_PINV2},            // S_PINV
	{SPR_MTEX, 0, 1, {NULL}, S_PINV},             // S_PINV2
	{SPR_PINV, 1, 18, {A_MonitorPop}, S_PINV4},   // S_PINV3
	{SPR_PINV, 1, 18, {A_Invincibility}, S_DISS}, // S_PINV4

	// Water Shield Box
	{SPR_BLTV, 0, 2, {NULL}, S_BLTV1},          // S_BLTV
	{SPR_MTEX, 0, 1, {NULL}, S_BLTV},           // S_BLTV1
	{SPR_BLTV, 1, 18, {A_MonitorPop}, S_BLTV3}, // S_BLTV2
	{SPR_BLTV, 1, 18, {A_WaterShield}, S_DISS}, // S_BLTV3

	// Yellow Spring
	{SPR_SPRY, 0, -1, {NULL}, S_NULL},           // S_YELLOWSPRING
	{SPR_SPRY, 4, 4, {A_Pain}, S_YELLOWSPRING3}, // S_YELLOWSPRING2
	{SPR_SPRY, 3, 1, {NULL}, S_YELLOWSPRING4},   // S_YELLOWSPRING3
	{SPR_SPRY, 2, 1, {NULL}, S_YELLOWSPRING5},   // S_YELLOWSPRING4
	{SPR_SPRY, 1, 1, {NULL}, S_YELLOWSPRING},    // S_YELLOWSPRING5

	{SPR_SUDY, 0, -1, {NULL}, S_NULL},             // S_YELLOWSPRINGUD
	{SPR_SUDY, 4, 4, {A_Pain}, S_YELLOWSPRINGUD3}, // S_YELLOWSPRINGUD2
	{SPR_SUDY, 3, 1, {NULL}, S_YELLOWSPRINGUD4},   // S_YELLOWSPRINGUD3
	{SPR_SUDY, 2, 1, {NULL}, S_YELLOWSPRINGUD5},   // S_YELLOWSPRINGUD4
	{SPR_SUDY, 1, 1, {NULL}, S_YELLOWSPRINGUD},    // S_YELLOWSPRINGUD5

	// Super Sneakers Box
	{SPR_SHTV, 0, 2, {NULL}, S_SHTV1},            // S_SHTV
	{SPR_MTEX, 0, 1, {NULL}, S_SHTV},             // S_SHTV1
	{SPR_SHTV, 1, 18, {A_MonitorPop}, S_SHTV3},   // S_SHTV2
	{SPR_SHTV, 1, 18, {A_SuperSneakers}, S_DISS}, // S_SHTV3

	// Fan
	{SPR_FANS, 0, 1, {NULL}, S_FAN2}, // S_FAN
	{SPR_FANS, 1, 1, {NULL}, S_FAN3}, // S_FAN2
	{SPR_FANS, 2, 1, {NULL}, S_FAN4}, // S_FAN3
	{SPR_FANS, 3, 1, {NULL}, S_FAN5}, // S_FAN4
	{SPR_FANS, 4, 1, {NULL}, S_FAN},  // S_FAN5

	// Bubble Source
	{SPR_BUBL, 0, 8, {A_BubbleSpawn}, S_BUBBLES2}, // S_BUBBLES1
	{SPR_BUBL, 1, 8, {A_BubbleCheck}, S_BUBBLES1}, // S_BUBBLES2

	// Ring Shield Box
	{SPR_YLTV, 0, 2, {NULL}, S_YLTV1},           // S_YLTV
	{SPR_MTEX, 0, 1, {NULL}, S_YLTV},            // S_YLTV1
	{SPR_YLTV, 1, 18, {A_MonitorPop}, S_YLTV3},  // S_YLTV2
	{SPR_YLTV, 1, 18, {A_RingShield}, S_DISS},   // S_YLTV3

	// GFZ Flower
	{SPR_FWR1, 0, 14, {NULL}, S_GFZFLOWERA2}, // S_GFZFLOWERA
	{SPR_FWR1, 1, 14, {NULL}, S_GFZFLOWERA},  // S_GFZFLOWERA2

	// Red Spring
	{SPR_SPRR, 0, -1, {NULL}, S_NULL},        // S_REDSPRING
	{SPR_SPRR, 4, 4, {A_Pain}, S_REDSPRING3}, // S_REDSPRING2
	{SPR_SPRR, 3, 1, {NULL}, S_REDSPRING4},   // S_REDSPRING3
	{SPR_SPRR, 2, 1, {NULL}, S_REDSPRING5},   // S_REDSPRING4
	{SPR_SPRR, 1, 1, {NULL}, S_REDSPRING},    // S_REDSPRING5

	// Upside-Down Red Spring
	{SPR_SUDR, 0, -1, {NULL}, S_NULL},          // S_REDSPRINGUD
	{SPR_SUDR, 4, 4, {A_Pain}, S_REDSPRINGUD3}, // S_REDSPRINGUD2
	{SPR_SUDR, 3, 1, {NULL}, S_REDSPRINGUD4},   // S_REDSPRINGUD3
	{SPR_SUDR, 2, 1, {NULL}, S_REDSPRINGUD5},   // S_REDSPRINGUD4
	{SPR_SUDR, 1, 1, {NULL}, S_REDSPRINGUD},    // S_REDSPRINGUD5

	// Smoke
	{SPR_SMOK, 0, 4, {NULL}, S_SMOK2}, // S_SMOK1
	{SPR_SMOK, 1, 5, {NULL}, S_SMOK3}, // S_SMOK2
	{SPR_SMOK, 2, 6, {NULL}, S_SMOK4}, // S_SMOK3
	{SPR_SMOK, 3, 7, {NULL}, S_SMOK5}, // S_SMOK4
	{SPR_SMOK, 4, 8, {NULL}, S_NULL},  // S_SMOK5

	// Water Splash
	{SPR_SPLA, 0, 3, {NULL}, S_SPLASH2},    // S_SPLASH1
	{SPR_SPLA, 1, 3, {NULL}, S_SPLASH3},    // S_SPLASH2
	{SPR_SPLA, 2, 3, {NULL}, S_RAINRETURN}, // S_SPLASH3
	{SPR_TNT1, 0, -1, {NULL}, S_TNT1},      // S_TNT1

	// Freed Birdie
	{SPR_BIRD, 0, 4, {NULL}, S_BIRD2},    // S_BIRD1
	{SPR_BIRD, 0, 4, {A_Chase}, S_BIRD3}, // S_BIRD2
	{SPR_BIRD, 1, 4, {A_Chase}, S_BIRD2}, // S_BIRD3

	// Freed Squirrel
	{SPR_SQRL, 0, 4, {NULL}, S_SQRL2},       // S_SQRL1
	{SPR_SQRL, 0, 64, {NULL}, S_SQRL3},      // S_SQRL2
	{SPR_SQRL, 1, 2, {A_BunnyHop}, S_SQRL4}, // S_SQRL3
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL5},    // S_SQRL4
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL6},    // S_SQRL5
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL7},    // S_SQRL6
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL8},    // S_SQRL7
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL9},    // S_SQRL8
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL10},   // S_SQRL9
	{SPR_SQRL, 1, 2, {A_Chase}, S_SQRL2},    // S_SQRL10

	// Yellow Shield Orb
	{SPR_YORB, 0, 1, {NULL}, S_YORB2}, // S_YORB1
	{SPR_YORB, 1, 1, {NULL}, S_YORB3}, // S_YORB1
	{SPR_YORB, 2, 1, {NULL}, S_YORB4}, // S_YORB1
	{SPR_YORB, 3, 1, {NULL}, S_YORB5}, // S_YORB1
	{SPR_YORB, 4, 1, {NULL}, S_YORB6}, // S_YORB1
	{SPR_YORB, 5, 1, {NULL}, S_YORB7}, // S_YORB1
	{SPR_YORB, 6, 1, {NULL}, S_YORB8}, // S_YORB1
	{SPR_YORB, 7, 1, {NULL}, S_YORB1}, // S_YORB1

	// Blue Shield Orb
	{SPR_BORB, 0, 1, {NULL}, S_BORB2}, // S_BORB1
	{SPR_BORB, 1, 1, {NULL}, S_BORB3}, // S_BORB2
	{SPR_BORB, 2, 1, {NULL}, S_BORB4}, // S_BORB3
	{SPR_BORB, 3, 1, {NULL}, S_BORB5}, // S_BORB4
	{SPR_BORB, 4, 1, {NULL}, S_BORB6}, // S_BORB5
	{SPR_BORB, 5, 1, {NULL}, S_BORB7}, // S_BORB6
	{SPR_BORB, 6, 1, {NULL}, S_BORB8}, // S_BORB7
	{SPR_BORB, 7, 1, {NULL}, S_BORB1}, // S_BORB8

	// Black Shield Orb
	{SPR_KORB, 0, 1, {NULL}, S_KORB2}, // S_KORB1
	{SPR_KORB, 1, 1, {NULL}, S_KORB3}, // S_KORB2
	{SPR_KORB, 2, 1, {NULL}, S_KORB4}, // S_KORB3
	{SPR_KORB, 3, 1, {NULL}, S_KORB5}, // S_KORB4
	{SPR_KORB, 4, 1, {NULL}, S_KORB6}, // S_KORB5
	{SPR_KORB, 5, 1, {NULL}, S_KORB7}, // S_KORB6
	{SPR_KORB, 6, 1, {NULL}, S_KORB8}, // S_KORB7
	{SPR_KORB, 7, 1, {NULL}, S_KORB1}, // S_KORB8

	// White Shield Orb
	{SPR_WORB, 0, 1, {NULL}, S_WORB2}, // S_WORB1
	{SPR_WORB, 1, 1, {NULL}, S_WORB3}, // S_WORB2
	{SPR_WORB, 2, 1, {NULL}, S_WORB4}, // S_WORB3
	{SPR_WORB, 3, 1, {NULL}, S_WORB5}, // S_WORB4
	{SPR_WORB, 4, 1, {NULL}, S_WORB6}, // S_WORB5
	{SPR_WORB, 5, 1, {NULL}, S_WORB7}, // S_WORB6
	{SPR_WORB, 6, 1, {NULL}, S_WORB8}, // S_WORB7
	{SPR_WORB, 7, 1, {NULL}, S_WORB1}, // S_WORB8


	// Red Shield Orb
	{SPR_RORB, 0, 1, {NULL}, S_RORB2}, // S_RORB1
	{SPR_RORB, 1, 1, {NULL}, S_RORB3}, // S_RORB2
	{SPR_RORB, 2, 1, {NULL}, S_RORB4}, // S_RORB3
	{SPR_RORB, 3, 1, {NULL}, S_RORB5}, // S_RORB4
	{SPR_RORB, 4, 1, {NULL}, S_RORB6}, // S_RORB5
	{SPR_RORB, 5, 1, {NULL}, S_RORB7}, // S_RORB6
	{SPR_RORB, 6, 1, {NULL}, S_RORB8}, // S_RORB7
	{SPR_RORB, 7, 1, {NULL}, S_RORB1}, // S_RORB8

	// Spark
	{SPR_SPRK, 0, 1, {NULL}, S_SPRK2},  // S_SPRK1
	{SPR_SPRK, 1, 1, {NULL}, S_SPRK3},  // S_SPRK2
	{SPR_SPRK, 2, 1, {NULL}, S_SPRK4},  // S_SPRK3
	{SPR_SPRK, 3, 1, {NULL}, S_SPRK5},  // S_SPRK4
	{SPR_SPRK, 0, 1, {NULL}, S_SPRK6},  // S_SPRK5
	{SPR_SPRK, 1, 1, {NULL}, S_SPRK7},  // S_SPRK6
	{SPR_SPRK, 2, 1, {NULL}, S_SPRK8},  // S_SPRK7
	{SPR_SPRK, 3, 1, {NULL}, S_SPRK9},  // S_SPRK8
	{SPR_SPRK, 0, 1, {NULL}, S_SPRK10}, // S_SPRK9
	{SPR_SPRK, 1, 1, {NULL}, S_SPRK11}, // S_SPRK10
	{SPR_SPRK, 2, 1, {NULL}, S_SPRK12}, // S_SPRK11
	{SPR_SPRK, 3, 1, {NULL}, S_SPRK13}, // S_SPRK12
	{SPR_SPRK, 0, 1, {NULL}, S_SPRK14}, // S_SPRK13
	{SPR_SPRK, 1, 1, {NULL}, S_SPRK15}, // S_SPRK14
	{SPR_SPRK, 2, 1, {NULL}, S_SPRK16}, // S_SPRK15
	{SPR_SPRK, 3, 1, {NULL}, S_NULL},   // S_SPRK16

	// Invincibility Sparkles
	{SPR_IVSP, 0, 1, {NULL}, S_IVSP2},   // S_IVSP1
	{SPR_IVSP, 1, 1, {NULL}, S_IVSP3},   // S_IVSP2
	{SPR_IVSP, 2, 1, {NULL}, S_IVSP4},   // S_IVSP3
	{SPR_IVSP, 3, 1, {NULL}, S_IVSP5},   // S_IVSP4
	{SPR_IVSP, 4, 1, {NULL}, S_IVSP6},   // S_IVSP5
	{SPR_IVSP, 5, 1, {NULL}, S_IVSP7},   // S_IVSP6
	{SPR_IVSP, 6, 1, {NULL}, S_IVSP8},   // S_IVSP7
	{SPR_IVSP, 7, 1, {NULL}, S_IVSP9},   // S_IVSP8
	{SPR_IVSP, 8, 1, {NULL}, S_IVSP10},  // S_IVSP9
	{SPR_IVSP, 9, 1, {NULL}, S_IVSP11},  // S_IVSP10
	{SPR_IVSP, 10, 1, {NULL}, S_IVSP12}, // S_IVSP11
	{SPR_IVSP, 11, 1, {NULL}, S_IVSP13}, // S_IVSP12
	{SPR_IVSP, 12, 1, {NULL}, S_IVSP14}, // S_IVSP13
	{SPR_IVSP, 13, 1, {NULL}, S_IVSP15}, // S_IVSP14
	{SPR_IVSP, 14, 1, {NULL}, S_IVSP16}, // S_IVSP15
	{SPR_IVSP, 15, 1, {NULL}, S_IVSP17}, // S_IVSP16
	{SPR_IVSP, 16, 1, {NULL}, S_IVSP18}, // S_IVSP17
	{SPR_IVSP, 17, 1, {NULL}, S_IVSP19}, // S_IVSP18
	{SPR_IVSP, 18, 1, {NULL}, S_IVSP20}, // S_IVSP19
	{SPR_IVSP, 19, 1, {NULL}, S_IVSP21}, // S_IVSP20
	{SPR_IVSP, 20, 1, {NULL}, S_IVSP22}, // S_IVSP21
	{SPR_IVSP, 21, 1, {NULL}, S_IVSP23}, // S_IVSP22
	{SPR_IVSP, 22, 1, {NULL}, S_IVSP24}, // S_IVSP23
	{SPR_IVSP, 23, 1, {NULL}, S_IVSP25}, // S_IVSP24
	{SPR_IVSP, 24, 1, {NULL}, S_IVSP26}, // S_IVSP25
	{SPR_IVSP, 25, 1, {NULL}, S_IVSP27}, // S_IVSP26
	{SPR_IVSP, 26, 1, {NULL}, S_IVSP28}, // S_IVSP27
	{SPR_IVSP, 27, 1, {NULL}, S_IVSP29}, // S_IVSP28
	{SPR_IVSP, 28, 1, {NULL}, S_IVSQ1},  // S_IVSP29

	// Invincibility Sparkles Finish
	{SPR_IVSQ, 0, 1, {NULL}, S_IVSQ2}, // S_IVSQ1
	{SPR_IVSQ, 1, 1, {NULL}, S_IVSQ3}, // S_IVSQ2
	{SPR_IVSQ, 2, 1, {NULL}, S_NULL},  // S_IVSQ3

	// Dissipating Item
	{SPR_DISS, 0, 1, {NULL}, S_NULL}, // S_DISS

	// Bubbles
	{SPR_BUBP, 0, 1, {A_BubbleRise}, S_SMALLBUBBLE1},  // S_SMALLBUBBLE
	{SPR_BUBP, 0, 1, {A_BubbleRise}, S_SMALLBUBBLE},   // S_SMALLBUBBLE1
	{SPR_BUBO, 0, 1, {A_BubbleRise}, S_MEDIUMBUBBLE1}, // S_MEDIUMBUBBLE
	{SPR_BUBO, 0, 1, {A_BubbleRise}, S_MEDIUMBUBBLE},  // S_MEDIUMBUBBLE1

	// Extra Large Bubble (breathable)
	{SPR_BUBN, 32768, 16, {A_BubbleRise}, S_EXTRALARGEBUBBLE},  // S_LARGEBUBBLE
	{SPR_BUBM, 32768, 16, {A_BubbleRise}, S_EXTRALARGEBUBBLE1}, // S_EXTRALARGEBUBBLE
	{SPR_BUBM, 32768, 16, {A_BubbleRise}, S_EXTRALARGEBUBBLE},  // S_EXTRALARGEBUBBLE1

	// Drowning Timer Numbers
	{SPR_CNTA, 0, 40, {NULL}, S_DISS}, // S_ZERO1
	{SPR_CNTB, 0, 40, {NULL}, S_DISS}, // S_ONE1
	{SPR_CNTC, 0, 40, {NULL}, S_DISS}, // S_TWO1
	{SPR_CNTD, 0, 40, {NULL}, S_DISS}, // S_THREE1
	{SPR_CNTE, 0, 40, {NULL}, S_DISS}, // S_FOUR1
	{SPR_CNTF, 0, 40, {NULL}, S_DISS}, // S_FIVE1

	// Extra Large Bubble goes POP!
	{SPR_POPP, 0, 16, {NULL}, S_DISS}, // S_POP1

	// 1-Up Box
	{SPR_PRUP, 0, 2, {A_1upThinker}, S_PRUP2},  // S_PRUP1
	{SPR_MTEX, 0, 1, {NULL}, S_PRUP1},          // S_PRUP2
	{SPR_PRUP, 1, 18, {A_MonitorPop}, S_PRUP4}, // S_PRUP3
	{SPR_PRUP, 1, 18, {A_ExtraLife}, S_DISS},   // S_PRUP4

	// Bomb Shield Box
	{SPR_BKTV, 0, 2, {NULL}, S_BKTV2},          // S_BKTV1
	{SPR_MTEX, 0, 1, {NULL}, S_BKTV1},          // S_BKTV2
	{SPR_BKTV, 1, 18, {A_MonitorPop}, S_BKTV4}, // S_BKTV3
	{SPR_BKTV, 1, 18, {A_BombShield}, S_DISS},  // S_BKTV4

	// Jump Shield Box
	{SPR_WHTV, 0, 2, {NULL}, S_WHTV2},          // S_WHTV1
	{SPR_MTEX, 0, 1, {NULL}, S_WHTV1},          // S_WHTV2
	{SPR_WHTV, 1, 18, {A_MonitorPop}, S_WHTV4}, // S_WHTV3
	{SPR_WHTV, 1, 18, {A_JumpShield}, S_DISS},  // S_WHTV4

	// Fire Shield Box
	{SPR_RDTV, 0, 2, {NULL}, S_RDTV2},          // S_RDTV1
	{SPR_MTEX, 0, 1, {NULL}, S_RDTV1},          // S_RDTV2
	{SPR_RDTV, 1, 18, {A_MonitorPop}, S_RDTV4}, // S_RDTV3
	{SPR_RDTV, 1, 18, {A_FireShield}, S_DISS},  // S_RDTV4

	{SPR_SCRA, 0, 2, {NULL}, S_SCRA2},        // S_SCRA
	{SPR_SCRA, 0, 30, {A_ScoreRise}, S_DISS}, // S_SCRA2
	{SPR_SCRB, 0, 2, {NULL}, S_SCRB2},        // S_SCRB
	{SPR_SCRB, 0, 30, {A_ScoreRise}, S_DISS}, // S_SCRB2
	{SPR_SCRC, 0, 2, {NULL}, S_SCRC2},        // S_SCRC
	{SPR_SCRC, 0, 30, {A_ScoreRise}, S_DISS}, // S_SCRC2
	{SPR_SCRD, 0, 2, {NULL}, S_SCRD2},        // S_SCRD
	{SPR_SCRD, 0, 30, {A_ScoreRise}, S_DISS}, // S_SCRD2

	// Super Sonic Spark
	{SPR_SSPK, 0, 2, {NULL}, S_SSPK2}, // S_SSPK1
	{SPR_SSPK, 1, 2, {NULL}, S_SSPK3}, // S_SSPK2
	{SPR_SSPK, 2, 2, {NULL}, S_SSPK4}, // S_SSPK3
	{SPR_SSPK, 1, 2, {NULL}, S_SSPK5}, // S_SSPK4
	{SPR_SSPK, 0, 2, {NULL}, S_DISS},  // S_SSPK5

	// Grass Debris
	{SPR_GRAS, 0, 2, {NULL}, S_GRASS2}, // S_GRASS1
	{SPR_GRAS, 1, 2, {NULL}, S_GRASS3}, // S_GRASS2
	{SPR_GRAS, 2, 2, {NULL}, S_GRASS4}, // S_GRASS3
	{SPR_GRAS, 3, 2, {NULL}, S_GRASS5}, // S_GRASS4
	{SPR_GRAS, 4, 2, {NULL}, S_GRASS6}, // S_GRASS5
	{SPR_GRAS, 5, 2, {NULL}, S_GRASS7}, // S_GRASS6
	{SPR_GRAS, 6, 2, {NULL}, S_DISS},   // S_GRASS7

	// Yellow Diagonal Spring
	{SPR_YSPR, 0, -1, {NULL}, S_NULL},    // S_YDIAG1
	{SPR_YSPR, 1, 1, {A_Pain}, S_YDIAG3}, // S_YDIAG2
	{SPR_YSPR, 2, 1, {NULL}, S_YDIAG4},   // S_YDIAG3
	{SPR_YSPR, 3, 1, {NULL}, S_YDIAG5},   // S_YDIAG4
	{SPR_YSPR, 4, 1, {NULL}, S_YDIAG6},   // S_YDIAG5
	{SPR_YSPR, 3, 1, {NULL}, S_YDIAG7},   // S_YDIAG6
	{SPR_YSPR, 2, 1, {NULL}, S_YDIAG8},   // S_YDIAG7
	{SPR_YSPR, 1, 1, {NULL}, S_YDIAG1},   // S_YDIAG8

	// Red Diagonal Spring
	{SPR_RSPR, 0, -1, {NULL}, S_NULL},    // S_RDIAG1
	{SPR_RSPR, 1, 1, {A_Pain}, S_RDIAG3}, // S_RDIAG2
	{SPR_RSPR, 2, 1, {NULL}, S_RDIAG4},   // S_RDIAG3
	{SPR_RSPR, 3, 1, {NULL}, S_RDIAG5},   // S_RDIAG4
	{SPR_RSPR, 4, 1, {NULL}, S_RDIAG6},   // S_RDIAG5
	{SPR_RSPR, 3, 1, {NULL}, S_RDIAG7},   // S_RDIAG6
	{SPR_RSPR, 2, 1, {NULL}, S_RDIAG8},   // S_RDIAG7
	{SPR_RSPR, 1, 1, {NULL}, S_RDIAG1},   // S_RDIAG8

	// Yellow Upside-Down Diagonal Spring
	{SPR_YSUD, 0, -1, {NULL}, S_NULL},     // S_YDIAGD1
	{SPR_YSUD, 1, 1, {A_Pain}, S_YDIAGD3}, // S_YDIAGD2
	{SPR_YSUD, 2, 1, {NULL}, S_YDIAGD4},   // S_YDIAGD3
	{SPR_YSUD, 3, 1, {NULL}, S_YDIAGD5},   // S_YDIAGD4
	{SPR_YSUD, 4, 1, {NULL}, S_YDIAGD6},   // S_YDIAGD5
	{SPR_YSUD, 3, 1, {NULL}, S_YDIAGD7},   // S_YDIAGD6
	{SPR_YSUD, 2, 1, {NULL}, S_YDIAGD8},   // S_YDIAGD7
	{SPR_YSUD, 1, 1, {NULL}, S_YDIAGD1},   // S_YDIAGD8

	// Red Upside-Down Diagonal Spring
	{SPR_RSUD, 0, -1, {NULL}, S_NULL},     // S_RDIAGD1
	{SPR_RSUD, 1, 1, {A_Pain}, S_RDIAGD3}, // S_RDIAGD2
	{SPR_RSUD, 2, 1, {NULL}, S_RDIAGD4},   // S_RDIAGD3
	{SPR_RSUD, 3, 1, {NULL}, S_RDIAGD5},   // S_RDIAGD4
	{SPR_RSUD, 4, 1, {NULL}, S_RDIAGD6},   // S_RDIAGD5
	{SPR_RSUD, 3, 1, {NULL}, S_RDIAGD7},   // S_RDIAGD6
	{SPR_RSUD, 2, 1, {NULL}, S_RDIAGD8},   // S_RDIAGD7
	{SPR_RSUD, 1, 1, {NULL}, S_RDIAGD1},   // S_RDIAGD8

	// Skim Mine Dropper
	{SPR_SKIM, 0, 1, {A_SkimChase}, S_SKIM2},    // S_SKIM1
	{SPR_SKIM, 0, 1, {A_SkimChase}, S_SKIM1},    // S_SKIM2
	{SPR_SKIM, 1, 1, {NULL}, S_SKIM4},           // S_SKIM3
	{SPR_SKIM, 2, 1, {NULL}, S_SKIM5},           // S_SKIM4
	{SPR_SKIM, 3, 1, {NULL}, S_SKIM6},           // S_SKIM5
	{SPR_SKIM, 4, 1, {NULL}, S_SKIM7},           // S_SKIM6
	{SPR_SKIM, 5, 1, {NULL}, S_SKIM8},           // S_SKIM7
	{SPR_SKIM, 6, 1, {NULL}, S_SKIM9},           // S_SKIM8
	{SPR_SKIM, 7, 1, {NULL}, S_SKIM10},          // S_SKIM9
	{SPR_SKIM, 8, 1, {NULL}, S_SKIM11},          // S_SKIM10
	{SPR_SKIM, 9, 1, {NULL}, S_SKIM12},          // S_SKIM11
	{SPR_SKIM, 10, 1, {NULL}, S_SKIM13},         // S_SKIM12
	{SPR_SKIM, 11, 1, {NULL}, S_SKIM14},         // S_SKIM13
	{SPR_SKIM, 12, 1, {NULL}, S_SKIM15},         // S_SKIM14
	{SPR_SKIM, 13, 1, {NULL}, S_SKIM16},         // S_SKIM15
	{SPR_SKIM, 14, 1, {NULL}, S_SKIM17},         // S_SKIM16
	{SPR_SKIM, 14, 1, {A_DropMine}, S_SKIM18},   // S_SKIM17
	{SPR_SKIM, 13, 1, {NULL}, S_SKIM19},         // S_SKIM18
	{SPR_SKIM, 12, 1, {NULL}, S_SKIM20},         // S_SKIM19
	{SPR_SKIM, 11, 1, {NULL}, S_SKIM21},         // S_SKIM20
	{SPR_SKIM, 10, 1, {NULL}, S_SKIM22},         // S_SKIM21
	{SPR_SKIM, 9, 1, {NULL}, S_SKIM23},          // S_SKIM22
	{SPR_SKIM, 8, 1, {NULL}, S_SKIM24},          // S_SKIM23
	{SPR_SKIM, 7, 1, {NULL}, S_SKIM25},          // S_SKIM24
	{SPR_SKIM, 6, 1, {NULL}, S_SKIM26},          // S_SKIM25
	{SPR_SKIM, 5, 1, {NULL}, S_SKIM27},          // S_SKIM26
	{SPR_SKIM, 4, 1, {NULL}, S_SKIM28},          // S_SKIM27
	{SPR_SKIM, 3, 1, {NULL}, S_SKIM29},          // S_SKIM28
	{SPR_SKIM, 2, 1, {NULL}, S_SKIM30},          // S_SKIM29
	{SPR_SKIM, 1, 1, {NULL}, S_SKIM1},           // S_SKIM30
	{SPR_SKIM, 15, 1, {A_Fall}, S_SKIM_BOOM2},   // S_SKIM_BOOM1
	{SPR_SKIM, 16, 5, {A_Scream}, S_SKIM_BOOM3}, // S_SKIM_BOOM2
	{SPR_SKIM, 17, 5, {NULL}, S_SKIM_BOOM4},     // S_SKIM_BOOM3
	{SPR_SKIM, 18, 5, {NULL}, S_DISS},           // S_SKIM_BOOM4

	// Skim Mine (also dropped by Jetty-Syn bomber)
	{SPR_MINE, 0, -1, {NULL}, S_NULL},           // S_MINE1
	{SPR_MINE, 1, 1, {A_Fall}, S_MINE_BOOM2},    // S_MINE_BOOM1
	{SPR_MINE, 2, 3, {A_Scream}, S_MINE_BOOM3},  // S_MINE_BOOM2
	{SPR_MINE, 3, 3, {A_Explode}, S_MINE_BOOM4}, // S_MINE_BOOM3
	{SPR_MINE, 4, 3, {NULL}, S_DISS},            // S_MINE_BOOM4

	// Greenflower Fish
	{SPR_FISH, 1, 1, {NULL}, S_FISH2},         // S_FISH1
	{SPR_FISH, 1, 1, {A_FishJump}, S_FISH1},   // S_FISH2
	{SPR_FISH, 0, 1, {NULL}, S_FISH4},         // S_FISH3
	{SPR_FISH, 0, 1, {A_FishJump}, S_FISH3},   // S_FISH4
	{SPR_FISH, 2, 1, {A_Fall}, S_FISH_DIE2},   // S_FISH_DIE1
	{SPR_FISH, 3, 5, {A_Scream}, S_FISH_DIE3}, // S_FISH_DIE2
	{SPR_FISH, 4, 5, {NULL}, S_FISH_DIE4},     // S_FISH_DIE3
	{SPR_FISH, 5, 5, {NULL}, S_DISS},          // S_FISH_DIE4

	// Deep Sea Gargoyle
	{SPR_GARG, 0, -1, {NULL}, S_NULL},  // S_GARGOYLE

	// Water Splish
	{SPR_SPLH, 0, 2, {NULL}, S_SPLISH2}, // S_SPLISH1
	{SPR_SPLH, 1, 2, {NULL}, S_SPLISH3}, // S_SPLISH2
	{SPR_SPLH, 2, 2, {NULL}, S_SPLISH4}, // S_SPLISH3
	{SPR_SPLH, 3, 2, {NULL}, S_SPLISH5}, // S_SPLISH4
	{SPR_SPLH, 4, 2, {NULL}, S_SPLISH6}, // S_SPLISH5
	{SPR_SPLH, 5, 2, {NULL}, S_SPLISH7}, // S_SPLISH6
	{SPR_SPLH, 6, 2, {NULL}, S_SPLISH8}, // S_SPLISH7
	{SPR_SPLH, 7, 2, {NULL}, S_SPLISH9}, // S_SPLISH8
	{SPR_SPLH, 8, 2, {NULL}, S_DISS},    // S_SPLISH9

	// Thok
	{SPR_THOK, 0, 8, {NULL}, S_NULL}, // S_THOK1

	// THZ Plant
	{SPR_THZP, 0, 4, {NULL}, S_THZPLANT2}, // S_THZPLANT1
	{SPR_THZP, 1, 4, {NULL}, S_THZPLANT3}, // S_THZPLANT1
	{SPR_THZP, 2, 4, {NULL}, S_THZPLANT4}, // S_THZPLANT1
	{SPR_THZP, 3, 4, {NULL}, S_THZPLANT1}, // S_THZPLANT1

	// Level End Sign
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN2},         // S_SIGN1
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN3},         // S_SIGN2
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN4},         // S_SIGN3
	{SPR_SIGN, 5, 1, {NULL}, S_SIGN5},         // S_SIGN4
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN6},         // S_SIGN5
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN7},         // S_SIGN6
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN8},         // S_SIGN7
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN9},         // S_SIGN8
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN10},        // S_SIGN9
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN11},        // S_SIGN10
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN12},        // S_SIGN11
	{SPR_SIGN, 4, 1, {NULL}, S_SIGN13},        // S_SIGN12
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN14},        // S_SIGN13
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN15},        // S_SIGN14
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN16},        // S_SIGN15
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN17},        // S_SIGN16
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN18},        // S_SIGN17
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN19},        // S_SIGN18
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN20},        // S_SIGN19
	{SPR_SIGN, 6, 1, {NULL}, S_SIGN21},        // S_SIGN20
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN22},        // S_SIGN21
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN23},        // S_SIGN22
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN24},        // S_SIGN23
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN25},        // S_SIGN24
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN26},        // S_SIGN25
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN27},        // S_SIGN26
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN28},        // S_SIGN27
	{SPR_SIGN, 5, 1, {NULL}, S_SIGN29},        // S_SIGN28
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN30},        // S_SIGN29
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN31},        // S_SIGN30
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN32},        // S_SIGN31
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN33},        // S_SIGN32
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN34},        // S_SIGN33
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN35},        // S_SIGN34
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN36},        // S_SIGN35
	{SPR_SIGN, 4, 1, {NULL}, S_SIGN37},        // S_SIGN36
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN38},        // S_SIGN37
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN39},        // S_SIGN38
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN40},        // S_SIGN39
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN41},        // S_SIGN40
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN42},        // S_SIGN41
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN43},        // S_SIGN42
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN44},        // S_SIGN43
	{SPR_SIGN, 6, 1, {NULL}, S_SIGN45},        // S_SIGN44
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN46},        // S_SIGN45
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN47},        // S_SIGN46
	{SPR_SIGN, 2, 1, {NULL}, S_SIGN48},        // S_SIGN47
	{SPR_SIGN, 3, 1, {NULL}, S_SIGN49},        // S_SIGN48
	{SPR_SIGN, 0, 1, {NULL}, S_SIGN50},        // S_SIGN49
	{SPR_SIGN, 1, 1, {NULL}, S_SIGN51},        // S_SIGN50
	{SPR_SIGN, 2, 1, {A_SignPlayer}, S_SIGN1}, // S_SIGN51
	{SPR_SIGN, 3, -1, {NULL}, S_NULL},         // S_SIGN52 Eggman
	{SPR_SIGN, 4, -1, {NULL}, S_NULL},         // S_SIGN53 Sonic
	{SPR_SIGN, 5, -1, {NULL}, S_NULL},         // S_SIGN54 Tails
	{SPR_SIGN, 6, -1, {NULL}, S_NULL},         // S_SIGN55 Knux
	{SPR_SIGN, 7, -1, {NULL}, S_NULL},         // S_SIGN56 Zim
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN57 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN58 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN59 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN60 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN61 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN62 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN63 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN64 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN65 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN66 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN67 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN68 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN69 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN70 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN71 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN72 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN73 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN74 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN75 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN76 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN77 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN78 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN79 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN80 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN81 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN82 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN83 User
	{SPR_SIGN, 8, -1, {NULL}, S_NULL},         // S_SIGN84 User

	// Red Rings (thrown)
	{SPR_RRNG, 32768, 1, {A_ThrownRing}, S_RRNG2}, // S_RRNG1
	{SPR_RRNG, 32769, 1, {A_ThrownRing}, S_RRNG3}, // S_RRNG2
	{SPR_RRNG, 32770, 1, {A_ThrownRing}, S_RRNG4}, // S_RRNG3
	{SPR_RRNG, 32771, 1, {A_ThrownRing}, S_RRNG5}, // S_RRNG4
	{SPR_RRNG, 32772, 1, {A_ThrownRing}, S_RRNG6}, // S_RRNG5
	{SPR_RRNG, 32773, 1, {A_ThrownRing}, S_RRNG7}, // S_RRNG6
	{SPR_RRNG, 32774, 1, {A_ThrownRing}, S_RRNG1}, // S_RRNG7

	{SPR_TTAG, 32768, 2, {NULL}, S_DISS}, // S_TTAG1

	// Steam Riser
	{SPR_STEM, 0, 2, {A_SetSolidSteam}, S_STEAM2},   // S_STEAM1
	{SPR_STEM, 1, 2, {A_UnsetSolidSteam}, S_STEAM3}, // S_STEAM2
	{SPR_STEM, 2, 2, {NULL}, S_STEAM4},              // S_STEAM3
	{SPR_STEM, 3, 2, {NULL}, S_STEAM5},              // S_STEAM4
	{SPR_STEM, 4, 2, {NULL}, S_STEAM6},              // S_STEAM5
	{SPR_STEM, 5, 2, {NULL}, S_STEAM7},              // S_STEAM6
	{SPR_STEM, 6, 2, {NULL}, S_STEAM8},              // S_STEAM7
	{SPR_STEM, 7, 18, {NULL}, S_STEAM1},             // S_STEAM8

	// CTF Flags
	{SPR_RFLG, 0, -1, {NULL}, S_NULL}, // S_REDFLAG
	{SPR_BFLG, 0, -1, {NULL}, S_NULL}, // S_BLUEFLAG

	// CTF Sign
	{SPR_GFLG, 0, 1, {NULL}, S_GOTFLAG2}, // S_GOTFLAG1
	{SPR_GFLG, 1, 1, {NULL}, S_DISS},     // S_GOTFLAG2
	{SPR_GFLG, 0, 1, {NULL}, S_GOTFLAG4}, // S_GOTFLAG3
	{SPR_GFLG, 2, 1, {NULL}, S_DISS},     // S_GOTFLAG4

	// Special Stage Token
	{SPR_TOKE, 32768, -1, {NULL}, S_TOKEN2}, // S_TOKEN
	{SPR_TOKE, 32769, 1, {NULL}, S_TOKEN},   // S_TOKEN2

	// Intangible Chaos Emeralds (Used for visual confirmation at end of Special Stages)
	{SPR_CEMG, 32768, -1, {NULL}, S_NULL}, // S_CEMG
	{SPR_CEMO, 32768, -1, {NULL}, S_NULL}, // S_CEMO
	{SPR_CEMP, 32768, -1, {NULL}, S_NULL}, // S_CEMP
	{SPR_CEMB, 32768, -1, {NULL}, S_NULL}, // S_CEMB
	{SPR_CEMR, 32768, -1, {NULL}, S_NULL}, // S_CEMR
	{SPR_CEML, 32768, -1, {NULL}, S_NULL}, // S_CEML
	{SPR_CEMY, 32768, -1, {NULL}, S_NULL}, // S_CEMY

	// Jetty-Syn Bomber
	{SPR_JETB, 0, 4, {A_Look}, S_JETBLOOK2},      // S_JETBLOOK1
	{SPR_JETB, 1, 4, {A_Look}, S_JETBLOOK1},      // S_JETBLOOK2
	{SPR_JETB, 0, 1, {A_JetbThink}, S_JETBZOOM2}, // S_JETBZOOM1
	{SPR_JETB, 1, 1, {A_JetbThink}, S_JETBZOOM1}, // S_JETBZOOM2

	// Jetty-Syn Gunner
	{SPR_JETG, 0, 4, {A_Look}, S_JETGLOOK2},       // S_JETGLOOK1
	{SPR_JETG, 1, 4, {A_Look}, S_JETGLOOK1},       // S_JETGLOOK2
	{SPR_JETG, 0, 1, {A_JetgThink}, S_JETGZOOM2},  // S_JETGZOOM1
	{SPR_JETG, 1, 1, {A_JetgThink}, S_JETGZOOM1},  // S_JETGZOOM2
	{SPR_JETG, 2, 1, {A_JetgShoot}, S_JETGSHOOT2}, // S_JETGSHOOT1
	{SPR_JETG, 3, 1, {NULL}, S_JETGZOOM1},         // S_JETGSHOOT2

	// Jetty-Syn Bullet
	{SPR_JBUL, 32768, 1, {NULL}, S_JETBULLET2}, // S_JETBULLET1
	{SPR_JBUL, 32769, 1, {NULL}, S_JETBULLET1}, // S_JETBULLET2

	// Freed Mouse
	{SPR_MOUS, 0, 2, {A_MouseThink}, S_MOUSE2}, // S_MOUSE1
	{SPR_MOUS, 1, 2, {A_MouseThink}, S_MOUSE1}, // S_MOUSE2

	// Deton
	{SPR_DETN, 0, 35, {A_Look}, S_DETON1},       // S_DETON1
	{SPR_DETN, 0, 1, {A_DetonChase}, S_DETON3},  // S_DETON2
	{SPR_DETN, 1, 1, {A_DetonChase}, S_DETON4},  // S_DETON3
	{SPR_DETN, 2, 1, {A_DetonChase}, S_DETON5},  // S_DETON4
	{SPR_DETN, 3, 1, {A_DetonChase}, S_DETON6},  // S_DETON5
	{SPR_DETN, 4, 1, {A_DetonChase}, S_DETON7},  // S_DETON6
	{SPR_DETN, 5, 1, {A_DetonChase}, S_DETON8},  // S_DETON7
	{SPR_DETN, 6, 1, {A_DetonChase}, S_DETON9},  // S_DETON8
	{SPR_DETN, 7, 1, {A_DetonChase}, S_DETON10}, // S_DETON9
	{SPR_DETN, 6, 1, {A_DetonChase}, S_DETON11}, // S_DETON10
	{SPR_DETN, 5, 1, {A_DetonChase}, S_DETON12}, // S_DETON11
	{SPR_DETN, 4, 1, {A_DetonChase}, S_DETON13}, // S_DETON12
	{SPR_DETN, 3, 1, {A_DetonChase}, S_DETON14}, // S_DETON13
	{SPR_DETN, 2, 1, {A_DetonChase}, S_DETON15}, // S_DETON14
	{SPR_DETN, 1, 1, {A_DetonChase}, S_DETON2},  // S_DETON15

	// Robot Explosion
	{SPR_XPLD, 0, 1, {A_Scream}, S_XPLD2}, // S_XPLD1
	{SPR_XPLD, 1, 5, {NULL}, S_XPLD3},     // S_XPLD2
	{SPR_XPLD, 2, 5, {NULL}, S_XPLD4},     // S_XPLD3
	{SPR_XPLD, 3, 5, {NULL}, S_DISS},      // S_XPLD4

	// CEZ Chain
	{SPR_CHAN, 0, -1, {NULL}, S_NULL}, // S_CEZCHAIN

	// Super Sonic Cape
	{SPR_CAPE, 0, 1, {A_CapeChase}, S_CAPE2}, // S_CAPE1
	{SPR_CAPE, 0, 1, {A_CapeChase}, S_CAPE1}, // S_CAPE2

	// Snowflake
	{SPR_SNO1, 0, -1, {NULL}, S_NULL}, // S_SNOW1
	{SPR_SNO1, 1, -1, {NULL}, S_NULL}, // S_SNOW2
	{SPR_SNO1, 2, -1, {NULL}, S_NULL}, // S_SNOW3

	// Santa
	{SPR_SANT, 0, -1, {NULL}, S_NULL}, // S_SANTA1

	// Emeralds (for hunt)
	{SPR_EMER, 0, -1, {NULL}, S_NULL}, // S_EMER1
	{SPR_EMES, 0, -1, {NULL}, S_NULL}, // S_EMES1
	{SPR_EMET, 0, -1, {NULL}, S_NULL}, // S_EMET1

	// Snowball for Snow Buster
	{SPR_SBLL, 0, 1, {A_SnowBall}, S_SBLL2}, // S_SBLL1
	{SPR_SBLL, 0, 1, {A_SnowBall}, S_SBLL1}, // S_SBLL2

	// Spike Ball
	{SPR_SPIK, 0, 1, {A_RotateSpikeBall}, S_SPIKEBALL2}, // S_SPIKEBALL1
	{SPR_SPIK, 1, 1, {A_RotateSpikeBall}, S_SPIKEBALL3}, // S_SPIKEBALL2
	{SPR_SPIK, 2, 1, {A_RotateSpikeBall}, S_SPIKEBALL4}, // S_SPIKEBALL3
	{SPR_SPIK, 3, 1, {A_RotateSpikeBall}, S_SPIKEBALL5}, // S_SPIKEBALL4
	{SPR_SPIK, 4, 1, {A_RotateSpikeBall}, S_SPIKEBALL6}, // S_SPIKEBALL5
	{SPR_SPIK, 5, 1, {A_RotateSpikeBall}, S_SPIKEBALL7}, // S_SPIKEBALL6
	{SPR_SPIK, 6, 1, {A_RotateSpikeBall}, S_SPIKEBALL8}, // S_SPIKEBALL7
	{SPR_SPIK, 7, 1, {A_RotateSpikeBall}, S_SPIKEBALL1}, // S_SPIKEBALL8

	// Crawla Commander
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND1
	{SPR_CCOM, 1, 1, {A_CrawlaCommanderThink}, S_CCOMMAND1}, // S_CCOMMAND2
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND4}, // S_CCOMMAND3
	{SPR_CCOM, 1, 1, {A_CrawlaCommanderThink}, S_CCOMMAND3}, // S_CCOMMAND4
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND5
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND6
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND7
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND8
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND9
	{SPR_CCOM, 0, 1, {A_CrawlaCommanderThink}, S_CCOMMAND2}, // S_CCOMMAND10

	{SPR_DISS, 0, 35, {NULL}, S_CRUMBLE2},  // S_CRUMBLE1
	{SPR_DISS, 0, 105, {A_Scream}, S_DISS}, // S_CRUMBLE2

	// Rain
	{SPR_RAIN, 32768, -1, {NULL}, S_NULL}, // S_RAIN1
	{SPR_RAIN, 32768, 1, {NULL}, S_RAIN1}, // S_RAINRETURN

	// Ceiling Spike
	{SPR_DSPK, 0, -1, {NULL}, S_NULL}, // S_CEILINGSPIKE

	// Floor Spike
	{SPR_USPK, 0,-1, {NULL}, S_NULL}, // S_FLOORSPIKE

	// Starpost
	{SPR_STPT, 0, -1, {NULL}, S_NULL},       // S_STARPOST1
	{SPR_STPT, 0, 2, {NULL}, S_STARPOST3},   // S_STARPOST2
	{SPR_STPT, 1, 2, {NULL}, S_STARPOST2},   // S_STARPOST3
	{SPR_STPT, 2, 1, {NULL}, S_STARPOST5},   // S_STARPOST4
	{SPR_STPT, 3, 1, {NULL}, S_STARPOST6},   // S_STARPOST5
	{SPR_STPT, 4, 1, {NULL}, S_STARPOST7},   // S_STARPOST6
	{SPR_STPT, 5, 1, {NULL}, S_STARPOST8},   // S_STARPOST7
	{SPR_STPT, 6, 1, {NULL}, S_STARPOST9},   // S_STARPOST8
	{SPR_STPT, 7, 1, {NULL}, S_STARPOST10},  // S_STARPOST9
	{SPR_STPT, 8, 1, {NULL}, S_STARPOST11},  // S_STARPOST10
	{SPR_STPT, 9, 1, {NULL}, S_STARPOST12},  // S_STARPOST11
	{SPR_STPT, 10, 1, {NULL}, S_STARPOST13}, // S_STARPOST12
	{SPR_STPT, 11, 1, {NULL}, S_STARPOST14}, // S_STARPOST13
	{SPR_STPT, 12, 1, {NULL}, S_STARPOST15}, // S_STARPOST14
	{SPR_STPT, 13, 1, {NULL}, S_STARPOST16}, // S_STARPOST15
	{SPR_STPT, 14, 1, {NULL}, S_STARPOST17}, // S_STARPOST16
	{SPR_STPT, 15, 1, {NULL}, S_STARPOST18}, // S_STARPOST17
	{SPR_STPT, 16, 1, {NULL}, S_STARPOST19}, // S_STARPOST18
	{SPR_STPT, 0, 1, {NULL}, S_STARPOST20},  // S_STARPOST19
	{SPR_STPT, 2, 1, {NULL}, S_STARPOST21},  // S_STARPOST20
	{SPR_STPT, 3, 1, {NULL}, S_STARPOST22},  // S_STARPOST21
	{SPR_STPT, 4, 1, {NULL}, S_STARPOST23},  // S_STARPOST22
	{SPR_STPT, 5, 1, {NULL}, S_STARPOST24},  // S_STARPOST23
	{SPR_STPT, 6, 1, {NULL}, S_STARPOST25},  // S_STARPOST24
	{SPR_STPT, 7, 1, {NULL}, S_STARPOST26},  // S_STARPOST25
	{SPR_STPT, 8, 1, {NULL}, S_STARPOST27},  // S_STARPOST26
	{SPR_STPT, 9, 1, {NULL}, S_STARPOST28},  // S_STARPOST27
	{SPR_STPT, 10, 1, {NULL}, S_STARPOST29}, // S_STARPOST28
	{SPR_STPT, 11, 1, {NULL}, S_STARPOST30}, // S_STARPOST29
	{SPR_STPT, 12, 1, {NULL}, S_STARPOST31}, // S_STARPOST30
	{SPR_STPT, 13, 1, {NULL}, S_STARPOST32}, // S_STARPOST31
	{SPR_STPT, 14, 1, {NULL}, S_STARPOST33}, // S_STARPOST32
	{SPR_STPT, 15, 1, {NULL}, S_STARPOST34}, // S_STARPOST33
	{SPR_STPT, 16, 1, {NULL}, S_STARPOST2},  // S_STARPOST34

	// Homing Ring
	{SPR_RNGM, 0, 1, {NULL}, S_HOMINGRING2},   // S_HOMINGRING1
	{SPR_RNGM, 1, 1, {NULL}, S_HOMINGRING3},   // S_HOMINGRING2
	{SPR_RNGM, 2, 1, {NULL}, S_HOMINGRING4},   // S_HOMINGRING3
	{SPR_RNGM, 3, 1, {NULL}, S_HOMINGRING5},   // S_HOMINGRING4
	{SPR_RNGM, 4, 1, {NULL}, S_HOMINGRING6},   // S_HOMINGRING5
	{SPR_RNGM, 5, 1, {NULL}, S_HOMINGRING7},   // S_HOMINGRING6
	{SPR_RNGM, 6, 1, {NULL}, S_HOMINGRING8},   // S_HOMINGRING7
	{SPR_RNGM, 7, 1, {NULL}, S_HOMINGRING9},   // S_HOMINGRING8
	{SPR_RNGM, 8, 1, {NULL}, S_HOMINGRING10},  // S_HOMINGRING9
	{SPR_RNGM, 9, 1, {NULL}, S_HOMINGRING11},  // S_HOMINGRING10
	{SPR_RNGM, 10, 1, {NULL}, S_HOMINGRING12}, // S_HOMINGRING11
	{SPR_RNGM, 11, 1, {NULL}, S_HOMINGRING13}, // S_HOMINGRING12
	{SPR_RNGM, 12, 1, {NULL}, S_HOMINGRING14}, // S_HOMINGRING13
	{SPR_RNGM, 13, 1, {NULL}, S_HOMINGRING15}, // S_HOMINGRING14
	{SPR_RNGM, 14, 1, {NULL}, S_HOMINGRING16}, // S_HOMINGRING15
	{SPR_RNGM, 15, 1, {NULL}, S_HOMINGRING17}, // S_HOMINGRING16
	{SPR_RNGM, 16, 1, {NULL}, S_HOMINGRING18}, // S_HOMINGRING17
	{SPR_RNGM, 17, 1, {NULL}, S_HOMINGRING19}, // S_HOMINGRING18
	{SPR_RNGM, 18, 1, {NULL}, S_HOMINGRING20}, // S_HOMINGRING19
	{SPR_RNGM, 19, 1, {NULL}, S_HOMINGRING21}, // S_HOMINGRING20
	{SPR_RNGM, 20, 1, {NULL}, S_HOMINGRING22}, // S_HOMINGRING21
	{SPR_RNGM, 21, 1, {NULL}, S_HOMINGRING23}, // S_HOMINGRING22
	{SPR_RNGM, 22, 1, {NULL}, S_HOMINGRING24}, // S_HOMINGRING23
	{SPR_RNGM, 23, 1, {NULL}, S_HOMINGRING25}, // S_HOMINGRING24
	{SPR_RNGM, 24, 1, {NULL}, S_HOMINGRING26}, // S_HOMINGRING25
	{SPR_RNGM, 25, 1, {NULL}, S_HOMINGRING27}, // S_HOMINGRING26
	{SPR_RNGM, 26, 1, {NULL}, S_HOMINGRING28}, // S_HOMINGRING27
	{SPR_RNGM, 27, 1, {NULL}, S_HOMINGRING29}, // S_HOMINGRING28
	{SPR_RNGM, 28, 1, {NULL}, S_HOMINGRING30}, // S_HOMINGRING29
	{SPR_RNGM, 29, 1, {NULL}, S_HOMINGRING31}, // S_HOMINGRING30
	{SPR_RNGM, 30, 1, {NULL}, S_HOMINGRING32}, // S_HOMINGRING31
	{SPR_RNGM, 31, 1, {NULL}, S_HOMINGRING33}, // S_HOMINGRING32
	{SPR_RNGM, 32, 1, {NULL}, S_HOMINGRING34}, // S_HOMINGRING33
	{SPR_RNGM, 33, 1, {NULL}, S_HOMINGRING35}, // S_HOMINGRING34
	{SPR_RNGM, 34, 1, {NULL}, S_HOMINGRING1},  // S_HOMINGRING35

	// Rail Ring
	{SPR_RNGR, 0, 1, {NULL}, S_RAILRING2},   // S_RAILRING1
	{SPR_RNGR, 1, 1, {NULL}, S_RAILRING3},   // S_RAILRING2
	{SPR_RNGR, 2, 1, {NULL}, S_RAILRING4},   // S_RAILRING3
	{SPR_RNGR, 3, 1, {NULL}, S_RAILRING5},   // S_RAILRING4
	{SPR_RNGR, 4, 1, {NULL}, S_RAILRING6},   // S_RAILRING5
	{SPR_RNGR, 5, 1, {NULL}, S_RAILRING7},   // S_RAILRING6
	{SPR_RNGR, 6, 1, {NULL}, S_RAILRING8},   // S_RAILRING7
	{SPR_RNGR, 7, 1, {NULL}, S_RAILRING9},   // S_RAILRING8
	{SPR_RNGR, 8, 1, {NULL}, S_RAILRING10},  // S_RAILRING9
	{SPR_RNGR, 9, 1, {NULL}, S_RAILRING11},  // S_RAILRING10
	{SPR_RNGR, 10, 1, {NULL}, S_RAILRING12}, // S_RAILRING11
	{SPR_RNGR, 11, 1, {NULL}, S_RAILRING13}, // S_RAILRING12
	{SPR_RNGR, 12, 1, {NULL}, S_RAILRING14}, // S_RAILRING13
	{SPR_RNGR, 13, 1, {NULL}, S_RAILRING15}, // S_RAILRING14
	{SPR_RNGR, 14, 1, {NULL}, S_RAILRING16}, // S_RAILRING15
	{SPR_RNGR, 15, 1, {NULL}, S_RAILRING17}, // S_RAILRING16
	{SPR_RNGR, 16, 1, {NULL}, S_RAILRING18}, // S_RAILRING17
	{SPR_RNGR, 17, 1, {NULL}, S_RAILRING19}, // S_RAILRING18
	{SPR_RNGR, 18, 1, {NULL}, S_RAILRING20}, // S_RAILRING19
	{SPR_RNGR, 19, 1, {NULL}, S_RAILRING21}, // S_RAILRING20
	{SPR_RNGR, 20, 1, {NULL}, S_RAILRING22}, // S_RAILRING21
	{SPR_RNGR, 21, 1, {NULL}, S_RAILRING23}, // S_RAILRING22
	{SPR_RNGR, 22, 1, {NULL}, S_RAILRING24}, // S_RAILRING23
	{SPR_RNGR, 23, 1, {NULL}, S_RAILRING25}, // S_RAILRING24
	{SPR_RNGR, 24, 1, {NULL}, S_RAILRING26}, // S_RAILRING25
	{SPR_RNGR, 25, 1, {NULL}, S_RAILRING27}, // S_RAILRING26
	{SPR_RNGR, 26, 1, {NULL}, S_RAILRING28}, // S_RAILRING27
	{SPR_RNGR, 27, 1, {NULL}, S_RAILRING29}, // S_RAILRING28
	{SPR_RNGR, 28, 1, {NULL}, S_RAILRING30}, // S_RAILRING29
	{SPR_RNGR, 29, 1, {NULL}, S_RAILRING31}, // S_RAILRING30
	{SPR_RNGR, 30, 1, {NULL}, S_RAILRING32}, // S_RAILRING31
	{SPR_RNGR, 31, 1, {NULL}, S_RAILRING33}, // S_RAILRING32
	{SPR_RNGR, 32, 1, {NULL}, S_RAILRING34}, // S_RAILRING33
	{SPR_RNGR, 33, 1, {NULL}, S_RAILRING35}, // S_RAILRING34
	{SPR_RNGR, 34, 1, {NULL}, S_RAILRING1},  // S_RAILRING35

	// Infinity Ring
	{SPR_RNGS, 0, 1, {NULL}, S_INFINITYRING2},   // S_INFINITYRING1
	{SPR_RNGS, 1, 1, {NULL}, S_INFINITYRING3},   // S_INFINITYRING2
	{SPR_RNGS, 2, 1, {NULL}, S_INFINITYRING4},   // S_INFINITYRING3
	{SPR_RNGS, 3, 1, {NULL}, S_INFINITYRING5},   // S_INFINITYRING4
	{SPR_RNGS, 4, 1, {NULL}, S_INFINITYRING6},   // S_INFINITYRING5
	{SPR_RNGS, 5, 1, {NULL}, S_INFINITYRING7},   // S_INFINITYRING6
	{SPR_RNGS, 6, 1, {NULL}, S_INFINITYRING8},   // S_INFINITYRING7
	{SPR_RNGS, 7, 1, {NULL}, S_INFINITYRING9},   // S_INFINITYRING8
	{SPR_RNGS, 8, 1, {NULL}, S_INFINITYRING10},  // S_INFINITYRING9
	{SPR_RNGS, 9, 1, {NULL}, S_INFINITYRING11},  // S_INFINITYRING10
	{SPR_RNGS, 10, 1, {NULL}, S_INFINITYRING12}, // S_INFINITYRING11
	{SPR_RNGS, 11, 1, {NULL}, S_INFINITYRING13}, // S_INFINITYRING12
	{SPR_RNGS, 12, 1, {NULL}, S_INFINITYRING14}, // S_INFINITYRING13
	{SPR_RNGS, 13, 1, {NULL}, S_INFINITYRING15}, // S_INFINITYRING14
	{SPR_RNGS, 14, 1, {NULL}, S_INFINITYRING16}, // S_INFINITYRING15
	{SPR_RNGS, 15, 1, {NULL}, S_INFINITYRING17}, // S_INFINITYRING16
	{SPR_RNGS, 16, 1, {NULL}, S_INFINITYRING18}, // S_INFINITYRING17
	{SPR_RNGS, 17, 1, {NULL}, S_INFINITYRING19}, // S_INFINITYRING18
	{SPR_RNGS, 18, 1, {NULL}, S_INFINITYRING20}, // S_INFINITYRING19
	{SPR_RNGS, 19, 1, {NULL}, S_INFINITYRING21}, // S_INFINITYRING20
	{SPR_RNGS, 20, 1, {NULL}, S_INFINITYRING22}, // S_INFINITYRING21
	{SPR_RNGS, 21, 1, {NULL}, S_INFINITYRING23}, // S_INFINITYRING22
	{SPR_RNGS, 22, 1, {NULL}, S_INFINITYRING24}, // S_INFINITYRING23
	{SPR_RNGS, 23, 1, {NULL}, S_INFINITYRING25}, // S_INFINITYRING24
	{SPR_RNGS, 24, 1, {NULL}, S_INFINITYRING26}, // S_INFINITYRING25
	{SPR_RNGS, 25, 1, {NULL}, S_INFINITYRING27}, // S_INFINITYRING26
	{SPR_RNGS, 26, 1, {NULL}, S_INFINITYRING28}, // S_INFINITYRING27
	{SPR_RNGS, 27, 1, {NULL}, S_INFINITYRING29}, // S_INFINITYRING28
	{SPR_RNGS, 28, 1, {NULL}, S_INFINITYRING30}, // S_INFINITYRING29
	{SPR_RNGS, 29, 1, {NULL}, S_INFINITYRING31}, // S_INFINITYRING30
	{SPR_RNGS, 30, 1, {NULL}, S_INFINITYRING32}, // S_INFINITYRING31
	{SPR_RNGS, 31, 1, {NULL}, S_INFINITYRING33}, // S_INFINITYRING32
	{SPR_RNGS, 32, 1, {NULL}, S_INFINITYRING34}, // S_INFINITYRING33
	{SPR_RNGS, 33, 1, {NULL}, S_INFINITYRING35}, // S_INFINITYRING34
	{SPR_RNGS, 34, 1, {NULL}, S_INFINITYRING1},  // S_INFINITYRING35

	// Automatic Ring
	{SPR_RNGA, 0, 1, {NULL}, S_AUTOMATICRING2},   // S_AUTOMATICRING1
	{SPR_RNGA, 1, 1, {NULL}, S_AUTOMATICRING3},   // S_AUTOMATICRING2
	{SPR_RNGA, 2, 1, {NULL}, S_AUTOMATICRING4},   // S_AUTOMATICRING3
	{SPR_RNGA, 3, 1, {NULL}, S_AUTOMATICRING5},   // S_AUTOMATICRING4
	{SPR_RNGA, 4, 1, {NULL}, S_AUTOMATICRING6},   // S_AUTOMATICRING5
	{SPR_RNGA, 5, 1, {NULL}, S_AUTOMATICRING7},   // S_AUTOMATICRING6
	{SPR_RNGA, 6, 1, {NULL}, S_AUTOMATICRING8},   // S_AUTOMATICRING7
	{SPR_RNGA, 7, 1, {NULL}, S_AUTOMATICRING9},   // S_AUTOMATICRING8
	{SPR_RNGA, 8, 1, {NULL}, S_AUTOMATICRING10},  // S_AUTOMATICRING9
	{SPR_RNGA, 9, 1, {NULL}, S_AUTOMATICRING11},  // S_AUTOMATICRING10
	{SPR_RNGA, 10, 1, {NULL}, S_AUTOMATICRING12}, // S_AUTOMATICRING11
	{SPR_RNGA, 11, 1, {NULL}, S_AUTOMATICRING13}, // S_AUTOMATICRING12
	{SPR_RNGA, 12, 1, {NULL}, S_AUTOMATICRING14}, // S_AUTOMATICRING13
	{SPR_RNGA, 13, 1, {NULL}, S_AUTOMATICRING15}, // S_AUTOMATICRING14
	{SPR_RNGA, 14, 1, {NULL}, S_AUTOMATICRING16}, // S_AUTOMATICRING15
	{SPR_RNGA, 15, 1, {NULL}, S_AUTOMATICRING17}, // S_AUTOMATICRING16
	{SPR_RNGA, 16, 1, {NULL}, S_AUTOMATICRING18}, // S_AUTOMATICRING17
	{SPR_RNGA, 17, 1, {NULL}, S_AUTOMATICRING19}, // S_AUTOMATICRING18
	{SPR_RNGA, 18, 1, {NULL}, S_AUTOMATICRING20}, // S_AUTOMATICRING19
	{SPR_RNGA, 19, 1, {NULL}, S_AUTOMATICRING21}, // S_AUTOMATICRING20
	{SPR_RNGA, 20, 1, {NULL}, S_AUTOMATICRING22}, // S_AUTOMATICRING21
	{SPR_RNGA, 21, 1, {NULL}, S_AUTOMATICRING23}, // S_AUTOMATICRING22
	{SPR_RNGA, 22, 1, {NULL}, S_AUTOMATICRING24}, // S_AUTOMATICRING23
	{SPR_RNGA, 23, 1, {NULL}, S_AUTOMATICRING25}, // S_AUTOMATICRING24
	{SPR_RNGA, 24, 1, {NULL}, S_AUTOMATICRING26}, // S_AUTOMATICRING25
	{SPR_RNGA, 25, 1, {NULL}, S_AUTOMATICRING27}, // S_AUTOMATICRING26
	{SPR_RNGA, 26, 1, {NULL}, S_AUTOMATICRING28}, // S_AUTOMATICRING27
	{SPR_RNGA, 27, 1, {NULL}, S_AUTOMATICRING29}, // S_AUTOMATICRING28
	{SPR_RNGA, 28, 1, {NULL}, S_AUTOMATICRING30}, // S_AUTOMATICRING29
	{SPR_RNGA, 29, 1, {NULL}, S_AUTOMATICRING31}, // S_AUTOMATICRING30
	{SPR_RNGA, 30, 1, {NULL}, S_AUTOMATICRING32}, // S_AUTOMATICRING31
	{SPR_RNGA, 31, 1, {NULL}, S_AUTOMATICRING33}, // S_AUTOMATICRING32
	{SPR_RNGA, 32, 1, {NULL}, S_AUTOMATICRING34}, // S_AUTOMATICRING33
	{SPR_RNGA, 33, 1, {NULL}, S_AUTOMATICRING35}, // S_AUTOMATICRING34
	{SPR_RNGA, 34, 1, {NULL}, S_AUTOMATICRING1},  // S_AUTOMATICRING35

	// Explosion Ring
	{SPR_RNGE, 0, 1, {NULL}, S_EXPLOSIONRING2},   // S_EXPLOSIONRING1
	{SPR_RNGE, 1, 1, {NULL}, S_EXPLOSIONRING3},   // S_EXPLOSIONRING2
	{SPR_RNGE, 2, 1, {NULL}, S_EXPLOSIONRING4},   // S_EXPLOSIONRING3
	{SPR_RNGE, 3, 1, {NULL}, S_EXPLOSIONRING5},   // S_EXPLOSIONRING4
	{SPR_RNGE, 4, 1, {NULL}, S_EXPLOSIONRING6},   // S_EXPLOSIONRING5
	{SPR_RNGE, 5, 1, {NULL}, S_EXPLOSIONRING7},   // S_EXPLOSIONRING6
	{SPR_RNGE, 6, 1, {NULL}, S_EXPLOSIONRING8},   // S_EXPLOSIONRING7
	{SPR_RNGE, 7, 1, {NULL}, S_EXPLOSIONRING9},   // S_EXPLOSIONRING8
	{SPR_RNGE, 8, 1, {NULL}, S_EXPLOSIONRING10},  // S_EXPLOSIONRING9
	{SPR_RNGE, 9, 1, {NULL}, S_EXPLOSIONRING11},  // S_EXPLOSIONRING10
	{SPR_RNGE, 10, 1, {NULL}, S_EXPLOSIONRING12}, // S_EXPLOSIONRING11
	{SPR_RNGE, 11, 1, {NULL}, S_EXPLOSIONRING13}, // S_EXPLOSIONRING12
	{SPR_RNGE, 12, 1, {NULL}, S_EXPLOSIONRING14}, // S_EXPLOSIONRING13
	{SPR_RNGE, 13, 1, {NULL}, S_EXPLOSIONRING15}, // S_EXPLOSIONRING14
	{SPR_RNGE, 14, 1, {NULL}, S_EXPLOSIONRING16}, // S_EXPLOSIONRING15
	{SPR_RNGE, 15, 1, {NULL}, S_EXPLOSIONRING17}, // S_EXPLOSIONRING16
	{SPR_RNGE, 16, 1, {NULL}, S_EXPLOSIONRING18}, // S_EXPLOSIONRING17
	{SPR_RNGE, 17, 1, {NULL}, S_EXPLOSIONRING19}, // S_EXPLOSIONRING18
	{SPR_RNGE, 18, 1, {NULL}, S_EXPLOSIONRING20}, // S_EXPLOSIONRING19
	{SPR_RNGE, 19, 1, {NULL}, S_EXPLOSIONRING21}, // S_EXPLOSIONRING20
	{SPR_RNGE, 20, 1, {NULL}, S_EXPLOSIONRING22}, // S_EXPLOSIONRING21
	{SPR_RNGE, 21, 1, {NULL}, S_EXPLOSIONRING23}, // S_EXPLOSIONRING22
	{SPR_RNGE, 22, 1, {NULL}, S_EXPLOSIONRING24}, // S_EXPLOSIONRING23
	{SPR_RNGE, 23, 1, {NULL}, S_EXPLOSIONRING25}, // S_EXPLOSIONRING24
	{SPR_RNGE, 24, 1, {NULL}, S_EXPLOSIONRING26}, // S_EXPLOSIONRING25
	{SPR_RNGE, 25, 1, {NULL}, S_EXPLOSIONRING27}, // S_EXPLOSIONRING26
	{SPR_RNGE, 26, 1, {NULL}, S_EXPLOSIONRING28}, // S_EXPLOSIONRING27
	{SPR_RNGE, 27, 1, {NULL}, S_EXPLOSIONRING29}, // S_EXPLOSIONRING28
	{SPR_RNGE, 28, 1, {NULL}, S_EXPLOSIONRING30}, // S_EXPLOSIONRING29
	{SPR_RNGE, 29, 1, {NULL}, S_EXPLOSIONRING31}, // S_EXPLOSIONRING30
	{SPR_RNGE, 30, 1, {NULL}, S_EXPLOSIONRING32}, // S_EXPLOSIONRING31
	{SPR_RNGE, 31, 1, {NULL}, S_EXPLOSIONRING33}, // S_EXPLOSIONRING32
	{SPR_RNGE, 32, 1, {NULL}, S_EXPLOSIONRING34}, // S_EXPLOSIONRING33
	{SPR_RNGE, 33, 1, {NULL}, S_EXPLOSIONRING35}, // S_EXPLOSIONRING34
	{SPR_RNGE, 34, 1, {NULL}, S_EXPLOSIONRING1},  // S_EXPLOSIONRING35

	// Thrown Weapon Rings
	{SPR_TAEH, 32768, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING2}, // S_THROWNAUTOMATICEXPLOSIONHOMING1
	{SPR_TAEH, 32769, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING3}, // S_THROWNAUTOMATICEXPLOSIONHOMING2
	{SPR_TAEH, 32770, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING4}, // S_THROWNAUTOMATICEXPLOSIONHOMING3
	{SPR_TAEH, 32771, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING5}, // S_THROWNAUTOMATICEXPLOSIONHOMING4
	{SPR_TAEH, 32772, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING6}, // S_THROWNAUTOMATICEXPLOSIONHOMING5
	{SPR_TAEH, 32773, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING7}, // S_THROWNAUTOMATICEXPLOSIONHOMING6
	{SPR_TAEH, 32774, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSIONHOMING1}, // S_THROWNAUTOMATICEXPLOSIONHOMING7

	{SPR_TAER, 32768, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION2}, // S_THROWNAUTOMATICEXPLOSION1
	{SPR_TAER, 32769, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION3}, // S_THROWNAUTOMATICEXPLOSION2
	{SPR_TAER, 32770, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION4}, // S_THROWNAUTOMATICEXPLOSION3
	{SPR_TAER, 32771, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION5}, // S_THROWNAUTOMATICEXPLOSION4
	{SPR_TAER, 32772, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION6}, // S_THROWNAUTOMATICEXPLOSION5
	{SPR_TAER, 32773, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION7}, // S_THROWNAUTOMATICEXPLOSION6
	{SPR_TAER, 32774, 1, {A_ThrownRing}, S_THROWNAUTOMATICEXPLOSION1}, // S_THROWNAUTOMATICEXPLOSION7

	{SPR_TAHR, 32768, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING2}, // S_THROWNAUTOMATICHOMING1
	{SPR_TAHR, 32769, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING3}, // S_THROWNAUTOMATICHOMING2
	{SPR_TAHR, 32770, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING4}, // S_THROWNAUTOMATICHOMING3
	{SPR_TAHR, 32771, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING5}, // S_THROWNAUTOMATICHOMING4
	{SPR_TAHR, 32772, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING6}, // S_THROWNAUTOMATICHOMING5
	{SPR_TAHR, 32773, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING7}, // S_THROWNAUTOMATICHOMING6
	{SPR_TAHR, 32774, 1, {A_ThrownRing}, S_THROWNAUTOMATICHOMING1}, // S_THROWNAUTOMATICHOMING7

	{SPR_THER, 32768, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION2}, // S_THROWNHOMINGEXPLOSION1
	{SPR_THER, 32769, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION3}, // S_THROWNHOMINGEXPLOSION2
	{SPR_THER, 32770, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION4}, // S_THROWNHOMINGEXPLOSION3
	{SPR_THER, 32771, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION5}, // S_THROWNHOMINGEXPLOSION4
	{SPR_THER, 32772, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION6}, // S_THROWNHOMINGEXPLOSION5
	{SPR_THER, 32773, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION7}, // S_THROWNHOMINGEXPLOSION6
	{SPR_THER, 32774, 1, {A_ThrownRing}, S_THROWNHOMINGEXPLOSION1}, // S_THROWNHOMINGEXPLOSION7

	{SPR_THOM, 32768, 1, {A_ThrownRing}, S_THROWNHOMING2}, // S_THROWNHOMING1
	{SPR_THOM, 32769, 1, {A_ThrownRing}, S_THROWNHOMING3}, // S_THROWNHOMING2
	{SPR_THOM, 32770, 1, {A_ThrownRing}, S_THROWNHOMING4}, // S_THROWNHOMING3
	{SPR_THOM, 32771, 1, {A_ThrownRing}, S_THROWNHOMING5}, // S_THROWNHOMING4
	{SPR_THOM, 32772, 1, {A_ThrownRing}, S_THROWNHOMING6}, // S_THROWNHOMING5
	{SPR_THOM, 32773, 1, {A_ThrownRing}, S_THROWNHOMING7}, // S_THROWNHOMING6
	{SPR_THOM, 32774, 1, {A_ThrownRing}, S_THROWNHOMING1}, // S_THROWNHOMING7

	{SPR_TAUT, 32768, 1, {A_ThrownRing}, S_THROWNAUTOMATIC2}, // S_THROWNAUTOMATIC1
	{SPR_TAUT, 32769, 1, {A_ThrownRing}, S_THROWNAUTOMATIC3}, // S_THROWNAUTOMATIC2
	{SPR_TAUT, 32770, 1, {A_ThrownRing}, S_THROWNAUTOMATIC4}, // S_THROWNAUTOMATIC3
	{SPR_TAUT, 32771, 1, {A_ThrownRing}, S_THROWNAUTOMATIC5}, // S_THROWNAUTOMATIC4
	{SPR_TAUT, 32772, 1, {A_ThrownRing}, S_THROWNAUTOMATIC6}, // S_THROWNAUTOMATIC5
	{SPR_TAUT, 32773, 1, {A_ThrownRing}, S_THROWNAUTOMATIC7}, // S_THROWNAUTOMATIC6
	{SPR_TAUT, 32774, 1, {A_ThrownRing}, S_THROWNAUTOMATIC1}, // S_THROWNAUTOMATIC7

	{SPR_RNGE, 32768, 1, {A_ThrownRing}, S_THROWNEXPLOSION2}, // S_THROWNEXPLOSION1
	{SPR_RNGE, 32769, 1, {A_ThrownRing}, S_THROWNEXPLOSION3}, // S_THROWNEXPLOSION2
	{SPR_RNGE, 32770, 1, {A_ThrownRing}, S_THROWNEXPLOSION4}, // S_THROWNEXPLOSION3
	{SPR_RNGE, 32771, 1, {A_ThrownRing}, S_THROWNEXPLOSION5}, // S_THROWNEXPLOSION4
	{SPR_RNGE, 32772, 1, {A_ThrownRing}, S_THROWNEXPLOSION6}, // S_THROWNEXPLOSION5
	{SPR_RNGE, 32773, 1, {A_ThrownRing}, S_THROWNEXPLOSION7}, // S_THROWNEXPLOSION6
	{SPR_RNGE, 32774, 1, {A_ThrownRing}, S_THROWNEXPLOSION1}, // S_THROWNEXPLOSION7

	{SPR_DISS, 0, 1, {A_RingExplode}, S_XPLD1}, // S_RINGEXPLODE

	// Greenflower Scenery
	{SPR_BUS1, 0, -1, {NULL}, S_NULL},       // S_BERRYBUSH
	{SPR_BUS2, 0, -1, {NULL}, S_NULL},       // S_BUSH
	{SPR_FWR2, 0, 7, {NULL}, S_GFZFLOWERB2}, // S_GFZFLOWERB1
	{SPR_FWR2, 1, 7, {NULL}, S_GFZFLOWERB1}, // S_GFZFLOWERB1
	{SPR_FWR3, 0, -1, {NULL}, S_NULL},       // S_GFZFLOWERC1

	// Teleport Box
	{SPR_MIXU, 0, 2, {NULL}, S_MIXUPBOX2},          // S_MIXUPBOX1
	{SPR_MTEX, 0, 1, {NULL}, S_MIXUPBOX1},          // S_MIXUPBOX2
	{SPR_MIXU, 1, 18, {A_MonitorPop}, S_MIXUPBOX4}, // S_MIXUPBOX3
	{SPR_MIXU, 1, 18, {A_MixUp}, S_DISS},           // S_MIXUPBOX4

	// Question Box
	{SPR_QUES, 0, 2, {NULL}, S_RANDOMBOX2},   // S_RANDOMBOX1
	{SPR_MTEX, 0, 1, {NULL}, S_RANDOMBOX1},   // S_RANDOMBOX2
	{SPR_QUES, 0, 1, {A_MonitorPop}, S_DISS}, // S_RANDOMBOX3

	// Monitor Explosion
	{SPR_MTEX, 1, 2, {NULL}, S_MONITOREXPLOSION2}, // S_MONITOREXPLOSION1
	{SPR_MTEX, 2, 2, {NULL}, S_MONITOREXPLOSION3}, // S_MONITOREXPLOSION2
	{SPR_MTEX, 3, 2, {NULL}, S_MONITOREXPLOSION4}, // S_MONITOREXPLOSION3
	{SPR_MTEX, 4, 2, {NULL}, S_MONITOREXPLOSION5}, // S_MONITOREXPLOSION4
	{SPR_MTEX, 5, -1, {NULL}, S_NULL},             // S_MONITOREXPLOSION5

	// Flame (has corona)
	{SPR_FLAM, 32768, 3, {NULL}, S_FLAME2}, // S_FLAME1
	{SPR_FLAM, 32769, 3, {NULL}, S_FLAME3}, // S_FLAME2
	{SPR_FLAM, 32770, 3, {NULL}, S_FLAME4}, // S_FLAME3
	{SPR_FLAM, 32771, 3, {NULL}, S_FLAME1}, // S_FLAME4

	// Puma (Mario fireball)
	{SPR_PUMA, 32768, 3, {A_PumaJump}, S_PUMA2}, // S_PUMA1
	{SPR_PUMA, 32769, 3, {A_PumaJump}, S_PUMA3}, // S_PUMA2
	{SPR_PUMA, 32770, 3, {A_PumaJump}, S_PUMA1}, // S_PUMA3
	{SPR_PUMA, 32771, 3, {A_PumaJump}, S_PUMA5}, // S_PUMA4
	{SPR_PUMA, 32772, 3, {A_PumaJump}, S_PUMA6}, // S_PUMA5
	{SPR_PUMA, 32773, 3, {A_PumaJump}, S_PUMA4}, // S_PUMA6

	// Hammer
	{SPR_HAMM, 0, 3, {NULL}, S_HAMMER2}, // S_HAMMER1
	{SPR_HAMM, 1, 3, {NULL}, S_HAMMER3}, // S_HAMMER2
	{SPR_HAMM, 2, 3, {NULL}, S_HAMMER4}, // S_HAMMER3
	{SPR_HAMM, 3, 3, {NULL}, S_HAMMER1}, // S_HAMMER4

	// Koopa
	{SPR_KOOP, 0, -1, {NULL}, S_NULL},   // S_KOOPA1
	{SPR_KOOP, 1, 24, {NULL}, S_KOOPA1}, // S_KOOPA2

	// Turtle Shell
	{SPR_SHLL, 0, -1, {NULL}, S_NULL},  // S_SHELL
	{SPR_SHLL, 0, 2, {NULL}, S_SHELL2}, // S_SHELL1
	{SPR_SHLL, 1, 2, {NULL}, S_SHELL3}, // S_SHELL2
	{SPR_SHLL, 2, 2, {NULL}, S_SHELL4}, // S_SHELL3
	{SPR_SHLL, 3, 2, {NULL}, S_SHELL1}, // S_SHELL4

	// Axe
	{SPR_MAXE, 0, 3, {NULL}, S_AXE2}, // S_AXE1
	{SPR_MAXE, 1, 3, {NULL}, S_AXE3}, // S_AXE2
	{SPR_MAXE, 2, 3, {NULL}, S_AXE1}, // S_AXE3

	{SPR_BFLM, 0, 3,{NULL},S_KOOPAFLAME2}, // S_KOOPAFLAME1
	{SPR_BFLM, 1, 3,{NULL},S_KOOPAFLAME3}, // S_KOOPAFLAME2
	{SPR_BFLM, 2, 3,{NULL},S_KOOPAFLAME1}, // S_KOOPAFLAME3

	// Thrown Mario Fireball
	{SPR_FBLL, 32768, 3, {NULL}, S_FIREBALL2},    // S_FIREBALL1
	{SPR_FBLL, 32769, 3, {NULL}, S_FIREBALL3},    // S_FIREBALL2
	{SPR_FBLL, 32770, 3, {NULL}, S_FIREBALL4},    // S_FIREBALL3
	{SPR_FBLL, 32771, 3, {NULL}, S_FIREBALL1},    // S_FIREBALL4
	{SPR_FBLL, 32772, 3, {NULL}, S_FIREBALLEXP2}, // S_FIREBALLEXP1
	{SPR_FBLL, 32773, 3, {NULL}, S_FIREBALLEXP3}, // S_FIREBALLEXP2
	{SPR_FBLL, 32774, 3, {NULL}, S_NULL},         // S_FIREBALLEXP3

	// Fire Flower
	{SPR_FFWR, 0, 3, {NULL}, S_FIREFLOWER2}, // S_FIREFLOWER1
	{SPR_FFWR, 1, 3, {NULL}, S_FIREFLOWER3}, // S_FIREFLOWER2
	{SPR_FFWR, 2, 3, {NULL}, S_FIREFLOWER4}, // S_FIREFLOWER3
	{SPR_FFWR, 3, 3, {NULL}, S_FIREFLOWER1}, // S_FIREFLOWER4

	// Nights Sparkle
	{SPR_NSPK, 32768, 140, {NULL}, S_NIGHTSPARKLE2}, // S_NIGHTSPARKLE1
	{SPR_NSPK, 32769, 7, {NULL}, S_NIGHTSPARKLE3},   // S_NIGHTSPARKLE2
	{SPR_NSPK, 32770, 7, {NULL}, S_NIGHTSPARKLE4},   // S_NIGHTSPARKLE3
	{SPR_NSPK, 32771, 7, {NULL}, S_NULL},            // S_NIGHTSPARKLE4

	// Nights Drone
	{SPR_NDRN, 0, -1, {NULL}, S_NIGHTSDRONE2}, // S_NIGHTSDRONE1
	{SPR_NDRN, 0, -1, {NULL}, S_NIGHTSDRONE1}, // S_NIGHTSDRONE2

	// Nights Player, Flying
	{SPR_SUPE, 0, 1, {NULL}, S_NIGHTSFLY1B},  // S_NIGHTSFLY1A
	{SPR_SUPE, 1, 1, {NULL}, S_NIGHTSFLY1A},  // S_NIGHTSFLY1B
	{SPR_SUPE, 2, 1, {NULL}, S_NIGHTSFLY2B},  // S_NIGHTSFLY2A
	{SPR_SUPE, 3, 1, {NULL}, S_NIGHTSFLY2A},  // S_NIGHTSFLY2B
	{SPR_SUPE, 4, 1, {NULL}, S_NIGHTSFLY3B},  // S_NIGHTSFLY3A
	{SPR_SUPE, 5, 1, {NULL}, S_NIGHTSFLY3A},  // S_NIGHTSFLY3B
	{SPR_SUPE, 6, 1, {NULL}, S_NIGHTSFLY4B},  // S_NIGHTSFLY4A
	{SPR_SUPE, 7, 1, {NULL}, S_NIGHTSFLY4A},  // S_NIGHTSFLY4B
	{SPR_SUPE, 8, 1, {NULL}, S_NIGHTSFLY5B},  // S_NIGHTSFLY5A
	{SPR_SUPE, 9, 1, {NULL}, S_NIGHTSFLY5A},  // S_NIGHTSFLY5B
	{SPR_SUPE, 10, 1, {NULL}, S_NIGHTSFLY6B}, // S_NIGHTSFLY6A
	{SPR_SUPE, 11, 1, {NULL}, S_NIGHTSFLY6A}, // S_NIGHTSFLY6B
	{SPR_SUPE, 12, 1, {NULL}, S_NIGHTSFLY7B}, // S_NIGHTSFLY7A
	{SPR_SUPE, 13, 1, {NULL}, S_NIGHTSFLY7A}, // S_NIGHTSFLY7B
	{SPR_SUPE, 14, 1, {NULL}, S_NIGHTSFLY8B}, // S_NIGHTSFLY8A
	{SPR_SUPE, 15, 1, {NULL}, S_NIGHTSFLY8A}, // S_NIGHTSFLY8B
	{SPR_SUPE, 16, 1, {NULL}, S_NIGHTSFLY9B}, // S_NIGHTSFLY9A
	{SPR_SUPE, 17, 1, {NULL}, S_NIGHTSFLY9A}, // S_NIGHTSFLY9B

	// Nights Player, Falling
	{SPR_SUPZ, 0, 1, {NULL}, S_NIGHTSHURT2},   // S_NIGHTSHURT1
	{SPR_SUPZ, 1, 1, {NULL}, S_NIGHTSHURT3},   // S_NIGHTSHURT2
	{SPR_SUPZ, 2, 1, {NULL}, S_NIGHTSHURT4},   // S_NIGHTSHURT3
	{SPR_SUPZ, 3, 1, {NULL}, S_NIGHTSHURT5},   // S_NIGHTSHURT4
	{SPR_SUPZ, 4, 1, {NULL}, S_NIGHTSHURT6},   // S_NIGHTSHURT5
	{SPR_SUPZ, 5, 1, {NULL}, S_NIGHTSHURT7},   // S_NIGHTSHURT6
	{SPR_SUPZ, 6, 1, {NULL}, S_NIGHTSHURT8},   // S_NIGHTSHURT7
	{SPR_SUPZ, 7, 1, {NULL}, S_NIGHTSHURT9},   // S_NIGHTSHURT8
	{SPR_SUPZ, 8, 1, {NULL}, S_NIGHTSHURT10},  // S_NIGHTSHURT9
	{SPR_SUPZ, 9, 1, {NULL}, S_NIGHTSHURT11},  // S_NIGHTSHURT10
	{SPR_SUPZ, 10, 1, {NULL}, S_NIGHTSHURT12}, // S_NIGHTSHURT11
	{SPR_SUPZ, 11, 1, {NULL}, S_NIGHTSHURT13}, // S_NIGHTSHURT12
	{SPR_SUPZ, 12, 1, {NULL}, S_NIGHTSHURT14}, // S_NIGHTSHURT13
	{SPR_SUPZ, 13, 1, {NULL}, S_NIGHTSHURT15}, // S_NIGHTSHURT14
	{SPR_SUPZ, 14, 1, {NULL}, S_NIGHTSHURT16}, // S_NIGHTSHURT15
	{SPR_SUPZ, 15, 1, {NULL}, S_NIGHTSHURT17}, // S_NIGHTSHURT16
	{SPR_SUPZ, 0, 1, {NULL}, S_NIGHTSHURT18},  // S_NIGHTSHURT17
	{SPR_SUPZ, 1, 1, {NULL}, S_NIGHTSHURT19},  // S_NIGHTSHURT18
	{SPR_SUPZ, 2, 1, {NULL}, S_NIGHTSHURT20},  // S_NIGHTSHURT19
	{SPR_SUPZ, 3, 1, {NULL}, S_NIGHTSHURT21},  // S_NIGHTSHURT20
	{SPR_SUPZ, 4, 1, {NULL}, S_NIGHTSHURT22},  // S_NIGHTSHURT21
	{SPR_SUPZ, 5, 1, {NULL}, S_NIGHTSHURT23},  // S_NIGHTSHURT22
	{SPR_SUPZ, 6, 1, {NULL}, S_NIGHTSHURT24},  // S_NIGHTSHURT23
	{SPR_SUPZ, 7, 1, {NULL}, S_NIGHTSHURT25},  // S_NIGHTSHURT24
	{SPR_SUPZ, 8, 1, {NULL}, S_NIGHTSHURT26},  // S_NIGHTSHURT25
	{SPR_SUPZ, 9, 1, {NULL}, S_NIGHTSHURT27},  // S_NIGHTSHURT26
	{SPR_SUPZ, 10, 1, {NULL}, S_NIGHTSHURT28}, // S_NIGHTSHURT27
	{SPR_SUPZ, 11, 1, {NULL}, S_NIGHTSHURT29}, // S_NIGHTSHURT28
	{SPR_SUPZ, 12, 1, {NULL}, S_NIGHTSHURT30}, // S_NIGHTSHURT29
	{SPR_SUPZ, 13, 1, {NULL}, S_NIGHTSHURT31}, // S_NIGHTSHURT30
	{SPR_SUPZ, 14, 1, {NULL}, S_NIGHTSHURT32}, // S_NIGHTSHURT31
	{SPR_SUPZ, 15, 1, {NULL}, S_NIGHTSFLY1A},  // S_NIGHTSHURT32

	// Nights Player, Drilling
	{SPR_NDRL, 0, 2, {NULL}, S_NIGHTSDRILL1B},  // S_NIGHTSDRILL1A
	{SPR_NDRL, 1, 2, {NULL}, S_NIGHTSDRILL1C},  // S_NIGHTSDRILL1B
	{SPR_NDRL, 2, 2, {NULL}, S_NIGHTSDRILL1D},  // S_NIGHTSDRILL1C
	{SPR_NDRL, 3, 2, {NULL}, S_NIGHTSDRILL1A},  // S_NIGHTSDRILL1D
	{SPR_NDRL, 4, 2, {NULL}, S_NIGHTSDRILL2B},  // S_NIGHTSDRILL2A
	{SPR_NDRL, 5, 2, {NULL}, S_NIGHTSDRILL2C},  // S_NIGHTSDRILL2B
	{SPR_NDRL, 6, 2, {NULL}, S_NIGHTSDRILL2D},  // S_NIGHTSDRILL2C
	{SPR_NDRL, 7, 2, {NULL}, S_NIGHTSDRILL2A},  // S_NIGHTSDRILL2D
	{SPR_NDRL, 8, 2, {NULL}, S_NIGHTSDRILL3B},  // S_NIGHTSDRILL3A
	{SPR_NDRL, 9, 2, {NULL}, S_NIGHTSDRILL3C},  // S_NIGHTSDRILL3B
	{SPR_NDRL, 10, 2, {NULL}, S_NIGHTSDRILL3D}, // S_NIGHTSDRILL3C
	{SPR_NDRL, 11, 2, {NULL}, S_NIGHTSDRILL3A}, // S_NIGHTSDRILL3D
	{SPR_NDRL, 12, 2, {NULL}, S_NIGHTSDRILL4B}, // S_NIGHTSDRILL4A
	{SPR_NDRL, 13, 2, {NULL}, S_NIGHTSDRILL4C}, // S_NIGHTSDRILL4B
	{SPR_NDRL, 14, 2, {NULL}, S_NIGHTSDRILL4D}, // S_NIGHTSDRILL4C
	{SPR_NDRL, 15, 2, {NULL}, S_NIGHTSDRILL4A}, // S_NIGHTSDRILL4D
	{SPR_NDRL, 16, 2, {NULL}, S_NIGHTSDRILL5B}, // S_NIGHTSDRILL5A
	{SPR_NDRL, 17, 2, {NULL}, S_NIGHTSDRILL5C}, // S_NIGHTSDRILL5B
	{SPR_NDRL, 18, 2, {NULL}, S_NIGHTSDRILL5D}, // S_NIGHTSDRILL5C
	{SPR_NDRL, 19, 2, {NULL}, S_NIGHTSDRILL5A}, // S_NIGHTSDRILL5D
	{SPR_NDRL, 20, 2, {NULL}, S_NIGHTSDRILL6B}, // S_NIGHTSDRILL6A
	{SPR_NDRL, 21, 2, {NULL}, S_NIGHTSDRILL6C}, // S_NIGHTSDRILL6B
	{SPR_NDRL, 22, 2, {NULL}, S_NIGHTSDRILL6D}, // S_NIGHTSDRILL6C
	{SPR_NDRL, 23, 2, {NULL}, S_NIGHTSDRILL6A}, // S_NIGHTSDRILL6D
	{SPR_NDRL, 24, 2, {NULL}, S_NIGHTSDRILL7B}, // S_NIGHTSDRILL7A
	{SPR_NDRL, 25, 2, {NULL}, S_NIGHTSDRILL7C}, // S_NIGHTSDRILL7B
	{SPR_NDRL, 26, 2, {NULL}, S_NIGHTSDRILL7D}, // S_NIGHTSDRILL7C
	{SPR_NDRL, 27, 2, {NULL}, S_NIGHTSDRILL7A}, // S_NIGHTSDRILL7D
	{SPR_NDRL, 28, 2, {NULL}, S_NIGHTSDRILL8B}, // S_NIGHTSDRILL8A
	{SPR_NDRL, 29, 2, {NULL}, S_NIGHTSDRILL8C}, // S_NIGHTSDRILL8B
	{SPR_NDRL, 30, 2, {NULL}, S_NIGHTSDRILL8D}, // S_NIGHTSDRILL8C
	{SPR_NDRL, 31, 2, {NULL}, S_NIGHTSDRILL8A}, // S_NIGHTSDRILL8D
	{SPR_NDRL, 32, 2, {NULL}, S_NIGHTSDRILL9B}, // S_NIGHTSDRILL9A
	{SPR_NDRL, 33, 2, {NULL}, S_NIGHTSDRILL9C}, // S_NIGHTSDRILL9B
	{SPR_NDRL, 34, 2, {NULL}, S_NIGHTSDRILL9D}, // S_NIGHTSDRILL9C
	{SPR_NDRL, 35, 2, {NULL}, S_NIGHTSDRILL9A}, // S_NIGHTSDRILL9D

	// Flower Seed
	{SPR_SEED, 32768, -1, {NULL}, S_NULL}, // S_SEED

	{SPR_JETF, 32768, 1, {NULL}, S_JETFUME2}, // S_JETFUME1
	{SPR_DISS, 32768, 1, {NULL}, S_JETFUME1}, // S_JETFUME2
	{SPR_JETF, 32769, 1, {NULL}, S_JETFUME4}, // S_JETFUME3
	{SPR_DISS, 32768, 1, {NULL}, S_JETFUME3}, // S_JETFUME4

	{SPR_HOOP, 0, -1, {NULL}, S_NULL}, // S_HOOP

	{SPR_NSCR, 32768, -1, {NULL}, S_NULL}, // S_NIGHTSCORE10
	{SPR_NSCR, 32769, -1, {NULL}, S_NULL}, // S_NIGHTSCORE20
	{SPR_NSCR, 32770, -1, {NULL}, S_NULL}, // S_NIGHTSCORE30
	{SPR_NSCR, 32771, -1, {NULL}, S_NULL}, // S_NIGHTSCORE40
	{SPR_NSCR, 32772, -1, {NULL}, S_NULL}, // S_NIGHTSCORE50
	{SPR_NSCR, 32773, -1, {NULL}, S_NULL}, // S_NIGHTSCORE60
	{SPR_NSCR, 32774, -1, {NULL}, S_NULL}, // S_NIGHTSCORE70
	{SPR_NSCR, 32775, -1, {NULL}, S_NULL}, // S_NIGHTSCORE80
	{SPR_NSCR, 32776, -1, {NULL}, S_NULL}, // S_NIGHTSCORE90
	{SPR_NSCR, 32777, -1, {NULL}, S_NULL}, // S_NIGHTSCORE100

	{SPR_NWNG, 0, -1, {NULL}, S_NULL}, // S_NIGHTSWING

	// Boss 2
	{SPR_EGGN, 0, -1, {NULL}, S_NULL},                         // S_EGGMOBILE2_STND
	{SPR_EGGN, 1, 1, {NULL}, S_EGGMOBILE2_POGO2},              // S_EGGMOBILE2_POGO1
	{SPR_EGGN, 0, 1, {A_Boss2PogoSFX}, S_EGGMOBILE2_POGO3},    // S_EGGMOBILE2_POGO2
	{SPR_EGGN, 1, 1, {NULL}, S_EGGMOBILE2_POGO4},              // S_EGGMOBILE2_POGO3
	{SPR_EGGN, 2, -1, {NULL}, S_NULL},                         // S_EGGMOBILE2_POGO4
	{SPR_EGGN, 3, 23, {A_Invinciblerize}, S_EGGMOBILE2_PAIN2},  // S_EGGMOBILE2_PAIN
	{SPR_EGGN, 3, 1, {A_DeInvinciblerize}, S_EGGMOBILE2_STND}, // S_EGGMOBILE2_PAIN2
	{SPR_EGGN, 4, 8, {A_Fall}, S_EGGMOBILE2_DIE2},             // S_EGGMOBILE2_DIE1
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE3},       // S_EGGMOBILE2_DIE2
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE4},       // S_EGGMOBILE2_DIE3
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE5},       // S_EGGMOBILE2_DIE4
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE6},       // S_EGGMOBILE2_DIE5
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE7},       // S_EGGMOBILE2_DIE6
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE8},       // S_EGGMOBILE2_DIE7
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE9},       // S_EGGMOBILE2_DIE8
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE10},      // S_EGGMOBILE2_DIE9
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE11},      // S_EGGMOBILE2_DIE10
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE12},      // S_EGGMOBILE2_DIE11
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE13},      // S_EGGMOBILE2_DIE12
	{SPR_EGGN, 4, 8, {A_BossScream}, S_EGGMOBILE2_DIE14},      // S_EGGMOBILE2_DIE13
	{SPR_EGGN, 4, -1, {A_BossDeath}, S_NULL},                  // S_EGGMOBILE2_DIE14
	{SPR_EGGN, 5, 5, {NULL}, S_EGGMOBILE2_FLEE2},              // S_EGGMOBILE2_FLEE1
	{SPR_EGGN, 6, 5, {NULL}, S_EGGMOBILE2_FLEE1},              // S_EGGMOBILE2_FLEE2

	// Boss 2 Goop
	{SPR_GOOP, 32768, 2, {NULL}, S_GOOP2}, // S_GOOP1
	{SPR_GOOP, 32769, 2, {NULL}, S_GOOP1}, // S_GOOP2
	{SPR_GOOP, 32770, -1, {NULL}, S_DISS}, // S_GOOP3

	// Boss Explosion
	{SPR_BPLD, 32768, 5, {NULL}, S_BPLD2}, // S_BPLD1
	{SPR_BPLD, 32769, 5, {NULL}, S_BPLD3}, // S_BPLD2
	{SPR_BPLD, 32770, 5, {NULL}, S_BPLD4}, // S_BPLD3
	{SPR_BPLD, 32771, 5, {NULL}, S_BPLD5}, // S_BPLD4
	{SPR_BPLD, 32772, 5, {NULL}, S_BPLD6}, // S_BPLD5
	{SPR_BPLD, 32773, 5, {NULL}, S_BPLD7}, // S_BPLD6
	{SPR_BPLD, 32774, 5, {NULL}, S_DISS},  // S_BPLD7

	// THZ Alarm
	{SPR_ALRM, 32768, 35, {A_Scream}, S_ALARM1}, // S_ALARM1

	// Emblem
	{SPR_NWNG, 0, -1, {NULL}, S_NULL}, // S_EMBLEM1

	// Eggman Box
	{SPR_EGGB, 0, 2, {NULL}, S_EGGTV2},          // S_EGGTV1
	{SPR_MTEX, 0, 1, {NULL}, S_EGGTV1},          // S_EGGTV2
	{SPR_EGGB, 1, 18, {A_MonitorPop}, S_EGGTV4}, // S_EGGTV3
	{SPR_EGGB, 1, 18, {A_EggmanBox}, S_DISS},    // S_EGGTV4

	// Red Shield's Spawn
	{SPR_SFLM, 32768, 2, {NULL}, S_SPINFIRE2}, // S_SPINFIRE1
	{SPR_SFLM, 32769, 2, {NULL}, S_SPINFIRE3}, // S_SPINFIRE2
	{SPR_SFLM, 32770, 2, {NULL}, S_SPINFIRE4}, // S_SPINFIRE3
	{SPR_SFLM, 32771, 2, {NULL}, S_SPINFIRE5}, // S_SPINFIRE4
	{SPR_SFLM, 32772, 2, {NULL}, S_SPINFIRE6}, // S_SPINFIRE5
	{SPR_SFLM, 32773, 2, {NULL}, S_SPINFIRE1}, // S_SPINFIRE6

	{SPR_TNKA, 0, 35, {NULL}, S_DISS}, // S_BOSSTANK1
	{SPR_TNKB, 0, 35, {NULL}, S_DISS}, // S_BOSSTANK2
	{SPR_SPNK, 0, 35, {NULL}, S_DISS}, // S_BOSSSPIGOT

	{SPR_TFOG, 32768, 2, {NULL}, S_FOG2},  // S_FOG1
	{SPR_TFOG, 32769, 2, {NULL}, S_FOG3},  // S_FOG2
	{SPR_TFOG, 32770, 2, {NULL}, S_FOG4},  // S_FOG3
	{SPR_TFOG, 32771, 2, {NULL}, S_FOG5},  // S_FOG4
	{SPR_TFOG, 32772, 2, {NULL}, S_FOG6},  // S_FOG5
	{SPR_TFOG, 32773, 2, {NULL}, S_FOG7},  // S_FOG6
	{SPR_TFOG, 32774, 2, {NULL}, S_FOG8},  // S_FOG7
	{SPR_TFOG, 32775, 2, {NULL}, S_FOG9},  // S_FOG8
	{SPR_TFOG, 32776, 2, {NULL}, S_FOG10}, // S_FOG9
	{SPR_TFOG, 32777, 2, {NULL}, S_FOG11}, // S_FOG10
	{SPR_TFOG, 32778, 2, {NULL}, S_FOG12}, // S_FOG11
	{SPR_TFOG, 32779, 2, {NULL}, S_FOG13}, // S_FOG12
	{SPR_TFOG, 32780, 2, {NULL}, S_FOG14}, // S_FOG13
	{SPR_TFOG, 32781, 2, {NULL}, S_DISS},  // S_FOG14

	// Easter Egg
	{SPR_EEGG, 0, -1, {NULL}, S_NULL}, // S_EEGG

	{SPR_LITE, 0, -1, {NULL}, S_LITE}, // S_LITE

	// THZ Turret
	{SPR_TRET, 32768, 105, {A_TurretStop}, S_TURRETFIRE}, // S_TURRET
	{SPR_TRET, 32768, 105, {A_TurretFire}, S_TURRET},     // S_TURRETFIRE
	{SPR_TRET, 32769, 7, {A_Pain}, S_TURRETSHOCK2},       // S_TURRETSHOCK1
	{SPR_TRET, 32770, 7, {NULL}, S_TURRETSHOCK3},         // S_TURRETSHOCK2
	{SPR_TRET, 32771, 7, {NULL}, S_TURRETSHOCK4},         // S_TURRETSHOCK3
	{SPR_TRET, 32772, 7, {NULL}, S_TURRETSHOCK5},         // S_TURRETSHOCK4
	{SPR_TRET, 32769, 7, {NULL}, S_TURRETSHOCK6},         // S_TURRETSHOCK5
	{SPR_TRET, 32770, 7, {A_Pain}, S_TURRETSHOCK7},       // S_TURRETSHOCK6
	{SPR_TRET, 32771, 7, {NULL}, S_TURRETSHOCK8},         // S_TURRETSHOCK7
	{SPR_TRET, 32772, 7, {NULL}, S_TURRETSHOCK9},         // S_TURRETSHOCK8
	{SPR_TRET, 32772, 7, {NULL}, S_XPLD1},                // S_TURRETSHOCK9
	{SPR_TRLS, 32768, 1, {NULL}, S_TURRETLASER},          // S_TURRETLASER
	{SPR_TRLS, 32769, 2, {NULL}, S_TURRETLASEREXPLODE2},  // S_TURRETLASEREXPLODE1
	{SPR_TRLS, 32770, 2, {NULL}, S_DISS},                 // S_TURRETLASEREXPLODE2

	// GFZ Flower
	{SPR_FWR4, 0, -1, {NULL}, S_NULL}, // S_GFZFLOWERD1

	// Goomba
	{SPR_GOOM, 0, 6, {A_Look}, S_GOOMBA1B}, // S_GOOMBA1
	{SPR_GOOM, 1, 6, {A_Look}, S_GOOMBA1},  // S_GOOMBA1B
	{SPR_GOOM, 0, 3, {A_Chase}, S_GOOMBA3}, // S_GOOMBA2
	{SPR_GOOM, 0, 3, {A_Chase}, S_GOOMBA4}, // S_GOOMBA3
	{SPR_GOOM, 1, 3, {A_Chase}, S_GOOMBA5}, // S_GOOMBA4
	{SPR_GOOM, 1, 3, {A_Chase}, S_GOOMBA6}, // S_GOOMBA5
	{SPR_GOOM, 0, 3, {A_Chase}, S_GOOMBA7}, // S_GOOMBA6
	{SPR_GOOM, 0, 3, {A_Chase}, S_GOOMBA8}, // S_GOOMBA7
	{SPR_GOOM, 1, 3, {A_Chase}, S_GOOMBA9}, // S_GOOMBA8
	{SPR_GOOM, 1, 3, {A_Chase}, S_GOOMBA2}, // S_GOOMBA9
	{SPR_GOOM, 2, 16, {A_Scream}, S_DISS},  // S_GOOMBA_DEAD

	// Blue Goomba
	{SPR_BGOM, 0, 6, {A_Look}, S_BLUEGOOMBA1B}, // BLUEGOOMBA1
	{SPR_BGOM, 1, 6, {A_Look}, S_BLUEGOOMBA1},  // BLUEGOOMBA1B
	{SPR_BGOM, 0, 3, {A_Chase}, S_BLUEGOOMBA3}, // S_BLUEGOOMBA2
	{SPR_BGOM, 0, 3, {A_Chase}, S_BLUEGOOMBA4}, // S_BLUEGOOMBA3
	{SPR_BGOM, 1, 3, {A_Chase}, S_BLUEGOOMBA5}, // S_BLUEGOOMBA4
	{SPR_BGOM, 1, 3, {A_Chase}, S_BLUEGOOMBA6}, // S_BLUEGOOMBA5
	{SPR_BGOM, 0, 3, {A_Chase}, S_BLUEGOOMBA7}, // S_BLUEGOOMBA6
	{SPR_BGOM, 0, 3, {A_Chase}, S_BLUEGOOMBA8}, // S_BLUEGOOMBA7
	{SPR_BGOM, 1, 3, {A_Chase}, S_BLUEGOOMBA9}, // S_BLUEGOOMBA8
	{SPR_BGOM, 1, 3, {A_Chase}, S_BLUEGOOMBA2}, // S_BLUEGOOMBA9
	{SPR_BGOM, 2, 16, {A_Scream}, S_DISS},      // S_BLUEGOOMBA_DEAD

	{SPR_MUS1, 0, -1, {NULL}, S_NULL}, // S_MARIOBUSH1
	{SPR_MUS2, 0, -1, {NULL}, S_NULL}, // S_MARIOBUSH2
	{SPR_TOAD, 0, -1, {NULL}, S_NULL}, // S_TOAD

	// Coin
	{SPR_COIN, 32768, 5, {A_AttractChase}, S_COIN2}, // S_COIN1
	{SPR_COIN, 32769, 5, {A_AttractChase}, S_COIN3}, // S_COIN2
	{SPR_COIN, 32770, 5, {A_AttractChase}, S_COIN1}, // S_COIN3

	// Coin Sparkle
	{SPR_CPRK, 32768, 5, {NULL}, S_COINSPARKLE2}, // S_COINSPARKLE1
	{SPR_CPRK, 32769, 5, {NULL}, S_COINSPARKLE3}, // S_COINSPARKLE2
	{SPR_CPRK, 32770, 5, {NULL}, S_COINSPARKLE4}, // S_COINSPARKLE3
	{SPR_CPRK, 32771, 5, {NULL}, S_DISS},         // S_COINSPARKLE4

	// Xmas-specific stuff
	{SPR_XMS1, 0, -1, {NULL}, S_NULL}, // S_XMASPOLE
	{SPR_XMS2, 0, -1, {NULL}, S_NULL}, // S_CANDYCANE
	{SPR_XMS3, 0, -1, {NULL}, S_NULL}, // S_SNOWMAN

	{SPR_CAPS, 0, -1, {NULL}, S_NULL}, // S_EGGCAPSULE

	{SPR_SUPT, 0, 4, {NULL}, S_SUPERTRANS2},     // S_SUPERTRANS1
	{SPR_SUPT, 1, 4, {NULL}, S_SUPERTRANS3},     // S_SUPERTRANS2
	{SPR_SUPT, 32770, 4, {NULL}, S_SUPERTRANS4}, // S_SUPERTRANS3
	{SPR_SUPT, 3, 3, {NULL}, S_SUPERTRANS5},     // S_SUPERTRANS4
	{SPR_SUPT, 4, 3, {NULL}, S_SUPERTRANS6},     // S_SUPERTRANS5
	{SPR_SUPT, 5, 3, {NULL}, S_SUPERTRANS7},     // S_SUPERTRANS6
	{SPR_SUPT, 6, 3, {NULL}, S_SUPERTRANS8},     // S_SUPERTRANS7
	{SPR_SUPT, 7, 3, {NULL}, S_SUPERTRANS9},     // S_SUPERTRANS8
	{SPR_SUPT, 8, 16, {NULL}, S_NIGHTSDRONE1},   // S_SUPERTRANS9

	{SPR_ROIA, 0, 2, {NULL}, S_ROCKCRUMBLEA2}, // S_ROCKCRUMBLEA1
	{SPR_ROIA, 1, 2, {NULL}, S_ROCKCRUMBLEA3}, // S_ROCKCRUMBLEA2
	{SPR_ROIA, 2, 2, {NULL}, S_ROCKCRUMBLEA4}, // S_ROCKCRUMBLEA3
	{SPR_ROIA, 3, 2, {NULL}, S_ROCKCRUMBLEA5}, // S_ROCKCRUMBLEA4
	{SPR_ROIA, 4, 2, {NULL}, S_ROCKCRUMBLEA1}, // S_ROCKCRUMBLEA5

	{SPR_ROIB, 0, 2, {NULL}, S_ROCKCRUMBLEB2}, // S_ROCKCRUMBLEB1
	{SPR_ROIB, 1, 2, {NULL}, S_ROCKCRUMBLEB3}, // S_ROCKCRUMBLEB2
	{SPR_ROIB, 2, 2, {NULL}, S_ROCKCRUMBLEB4}, // S_ROCKCRUMBLEB3
	{SPR_ROIB, 3, 2, {NULL}, S_ROCKCRUMBLEB5}, // S_ROCKCRUMBLEB4
	{SPR_ROIB, 4, 2, {NULL}, S_ROCKCRUMBLEB1}, // S_ROCKCRUMBLEB5

	{SPR_ROIC, 0, 2, {NULL}, S_ROCKCRUMBLEC2}, // S_ROCKCRUMBLEC1
	{SPR_ROIC, 1, 2, {NULL}, S_ROCKCRUMBLEC3}, // S_ROCKCRUMBLEC2
	{SPR_ROIC, 2, 2, {NULL}, S_ROCKCRUMBLEC4}, // S_ROCKCRUMBLEC3
	{SPR_ROIC, 3, 2, {NULL}, S_ROCKCRUMBLEC5}, // S_ROCKCRUMBLEC4
	{SPR_ROIC, 4, 2, {NULL}, S_ROCKCRUMBLEC1}, // S_ROCKCRUMBLEC5

	{SPR_ROID, 0, 2, {NULL}, S_ROCKCRUMBLED2}, // S_ROCKCRUMBLED1
	{SPR_ROID, 1, 2, {NULL}, S_ROCKCRUMBLED3}, // S_ROCKCRUMBLED2
	{SPR_ROID, 2, 2, {NULL}, S_ROCKCRUMBLED4}, // S_ROCKCRUMBLED3
	{SPR_ROID, 3, 2, {NULL}, S_ROCKCRUMBLED5}, // S_ROCKCRUMBLED4
	{SPR_ROID, 4, 2, {NULL}, S_ROCKCRUMBLED1}, // S_ROCKCRUMBLED5

	{SPR_ROIE, 0, 2, {NULL}, S_ROCKCRUMBLEE2}, // S_ROCKCRUMBLEE1
	{SPR_ROIE, 1, 2, {NULL}, S_ROCKCRUMBLEE3}, // S_ROCKCRUMBLEE2
	{SPR_ROIE, 2, 2, {NULL}, S_ROCKCRUMBLEE4}, // S_ROCKCRUMBLEE3
	{SPR_ROIE, 3, 2, {NULL}, S_ROCKCRUMBLEE5}, // S_ROCKCRUMBLEE4
	{SPR_ROIE, 4, 2, {NULL}, S_ROCKCRUMBLEE1}, // S_ROCKCRUMBLEE5

	{SPR_ROIF, 0, 2, {NULL}, S_ROCKCRUMBLEF2}, // S_ROCKCRUMBLEF1
	{SPR_ROIF, 1, 2, {NULL}, S_ROCKCRUMBLEF3}, // S_ROCKCRUMBLEF2
	{SPR_ROIF, 2, 2, {NULL}, S_ROCKCRUMBLEF4}, // S_ROCKCRUMBLEF3
	{SPR_ROIF, 3, 2, {NULL}, S_ROCKCRUMBLEF5}, // S_ROCKCRUMBLEF4
	{SPR_ROIF, 4, 2, {NULL}, S_ROCKCRUMBLEF1}, // S_ROCKCRUMBLEF5

	{SPR_ROIG, 0, 2, {NULL}, S_ROCKCRUMBLEG2}, // S_ROCKCRUMBLEG1
	{SPR_ROIG, 1, 2, {NULL}, S_ROCKCRUMBLEG3}, // S_ROCKCRUMBLEG2
	{SPR_ROIG, 2, 2, {NULL}, S_ROCKCRUMBLEG4}, // S_ROCKCRUMBLEG3
	{SPR_ROIG, 3, 2, {NULL}, S_ROCKCRUMBLEG5}, // S_ROCKCRUMBLEG4
	{SPR_ROIG, 4, 2, {NULL}, S_ROCKCRUMBLEG1}, // S_ROCKCRUMBLEG5

	{SPR_ROIH, 0, 2, {NULL}, S_ROCKCRUMBLEH2}, // S_ROCKCRUMBLEH1
	{SPR_ROIH, 1, 2, {NULL}, S_ROCKCRUMBLEH3}, // S_ROCKCRUMBLEH2
	{SPR_ROIH, 2, 2, {NULL}, S_ROCKCRUMBLEH4}, // S_ROCKCRUMBLEH3
	{SPR_ROIH, 3, 2, {NULL}, S_ROCKCRUMBLEH5}, // S_ROCKCRUMBLEH4
	{SPR_ROIH, 4, 2, {NULL}, S_ROCKCRUMBLEH1}, // S_ROCKCRUMBLEH5

	{SPR_ROII, 0, 2, {NULL}, S_ROCKCRUMBLEI2}, // S_ROCKCRUMBLEI1
	{SPR_ROII, 1, 2, {NULL}, S_ROCKCRUMBLEI3}, // S_ROCKCRUMBLEI2
	{SPR_ROII, 2, 2, {NULL}, S_ROCKCRUMBLEI4}, // S_ROCKCRUMBLEI3
	{SPR_ROII, 3, 2, {NULL}, S_ROCKCRUMBLEI5}, // S_ROCKCRUMBLEI4
	{SPR_ROII, 4, 2, {NULL}, S_ROCKCRUMBLEI1}, // S_ROCKCRUMBLEI5

	{SPR_ROIJ, 0, 2, {NULL}, S_ROCKCRUMBLEJ2}, // S_ROCKCRUMBLEJ1
	{SPR_ROIJ, 1, 2, {NULL}, S_ROCKCRUMBLEJ3}, // S_ROCKCRUMBLEJ2
	{SPR_ROIJ, 2, 2, {NULL}, S_ROCKCRUMBLEJ4}, // S_ROCKCRUMBLEJ3
	{SPR_ROIJ, 3, 2, {NULL}, S_ROCKCRUMBLEJ5}, // S_ROCKCRUMBLEJ4
	{SPR_ROIJ, 4, 2, {NULL}, S_ROCKCRUMBLEJ1}, // S_ROCKCRUMBLEJ5

	{SPR_ROIK, 0, 2, {NULL}, S_ROCKCRUMBLEK2}, // S_ROCKCRUMBLEK1
	{SPR_ROIK, 1, 2, {NULL}, S_ROCKCRUMBLEK3}, // S_ROCKCRUMBLEK2
	{SPR_ROIK, 2, 2, {NULL}, S_ROCKCRUMBLEK4}, // S_ROCKCRUMBLEK3
	{SPR_ROIK, 3, 2, {NULL}, S_ROCKCRUMBLEK5}, // S_ROCKCRUMBLEK4
	{SPR_ROIK, 4, 2, {NULL}, S_ROCKCRUMBLEK1}, // S_ROCKCRUMBLEK5

	{SPR_ROIL, 0, 2, {NULL}, S_ROCKCRUMBLEL2}, // S_ROCKCRUMBLEL1
	{SPR_ROIL, 1, 2, {NULL}, S_ROCKCRUMBLEL3}, // S_ROCKCRUMBLEL2
	{SPR_ROIL, 2, 2, {NULL}, S_ROCKCRUMBLEL4}, // S_ROCKCRUMBLEL3
	{SPR_ROIL, 3, 2, {NULL}, S_ROCKCRUMBLEL5}, // S_ROCKCRUMBLEL4
	{SPR_ROIL, 4, 2, {NULL}, S_ROCKCRUMBLEL1}, // S_ROCKCRUMBLEL5

	{SPR_ROIM, 0, 2, {NULL}, S_ROCKCRUMBLEM2}, // S_ROCKCRUMBLEM1
	{SPR_ROIM, 1, 2, {NULL}, S_ROCKCRUMBLEM3}, // S_ROCKCRUMBLEM2
	{SPR_ROIM, 2, 2, {NULL}, S_ROCKCRUMBLEM4}, // S_ROCKCRUMBLEM3
	{SPR_ROIM, 3, 2, {NULL}, S_ROCKCRUMBLEM5}, // S_ROCKCRUMBLEM4
	{SPR_ROIM, 4, 2, {NULL}, S_ROCKCRUMBLEM1}, // S_ROCKCRUMBLEM5

	{SPR_ROIN, 0, 2, {NULL}, S_ROCKCRUMBLEN2}, // S_ROCKCRUMBLEN1
	{SPR_ROIN, 1, 2, {NULL}, S_ROCKCRUMBLEN3}, // S_ROCKCRUMBLEN2
	{SPR_ROIN, 2, 2, {NULL}, S_ROCKCRUMBLEN4}, // S_ROCKCRUMBLEN3
	{SPR_ROIN, 3, 2, {NULL}, S_ROCKCRUMBLEN5}, // S_ROCKCRUMBLEN4
	{SPR_ROIN, 4, 2, {NULL}, S_ROCKCRUMBLEN1}, // S_ROCKCRUMBLEN5

	{SPR_ROIO, 0, 2, {NULL}, S_ROCKCRUMBLEO2}, // S_ROCKCRUMBLEO1
	{SPR_ROIO, 1, 2, {NULL}, S_ROCKCRUMBLEO3}, // S_ROCKCRUMBLEO2
	{SPR_ROIO, 2, 2, {NULL}, S_ROCKCRUMBLEO4}, // S_ROCKCRUMBLEO3
	{SPR_ROIO, 3, 2, {NULL}, S_ROCKCRUMBLEO5}, // S_ROCKCRUMBLEO4
	{SPR_ROIO, 4, 2, {NULL}, S_ROCKCRUMBLEO1}, // S_ROCKCRUMBLEO5

	{SPR_ROIP, 0, 2, {NULL}, S_ROCKCRUMBLEP2}, // S_ROCKCRUMBLEP1
	{SPR_ROIP, 1, 2, {NULL}, S_ROCKCRUMBLEP3}, // S_ROCKCRUMBLEP2
	{SPR_ROIP, 2, 2, {NULL}, S_ROCKCRUMBLEP4}, // S_ROCKCRUMBLEP3
	{SPR_ROIP, 3, 2, {NULL}, S_ROCKCRUMBLEP5}, // S_ROCKCRUMBLEP4
	{SPR_ROIP, 4, 2, {NULL}, S_ROCKCRUMBLEP1}, // S_ROCKCRUMBLEP5

	// NiGHTS Paraloop Powerups
	{SPR_DISS, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP1
	{SPR_NPRA, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP2
	{SPR_DISS, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP3
	{SPR_NPRB, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP4
	{SPR_DISS, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP5
	{SPR_NPRC, 0, -1, {NULL}, S_NULL}, // S_NIGHTSPOWERUP6

	{SPR_NSCR, 32778, -1, {NULL}, S_NULL}, // S_NIGHTSCORE10_2
	{SPR_NSCR, 32779, -1, {NULL}, S_NULL}, // S_NIGHTSCORE20_2
	{SPR_NSCR, 32780, -1, {NULL}, S_NULL}, // S_NIGHTSCORE30_2
	{SPR_NSCR, 32781, -1, {NULL}, S_NULL}, // S_NIGHTSCORE40_2
	{SPR_NSCR, 32782, -1, {NULL}, S_NULL}, // S_NIGHTSCORE50_2
	{SPR_NSCR, 32783, -1, {NULL}, S_NULL}, // S_NIGHTSCORE60_2
	{SPR_NSCR, 32784, -1, {NULL}, S_NULL}, // S_NIGHTSCORE70_2
	{SPR_NSCR, 32785, -1, {NULL}, S_NULL}, // S_NIGHTSCORE80_2
	{SPR_NSCR, 32786, -1, {NULL}, S_NULL}, // S_NIGHTSCORE90_2
	{SPR_NSCR, 32787, -1, {NULL}, S_NULL}, // S_NIGHTSCORE100_2

	// Hyper Sonic Spark
	{SPR_SSPK, 0, 1, {A_SparkFollow}, S_HSPK2}, // S_HSPK1
	{SPR_SSPK, 1, 1, {A_SparkFollow}, S_HSPK3}, // S_HSPK2
	{SPR_SSPK, 2, 1, {A_SparkFollow}, S_HSPK4}, // S_HSPK3
	{SPR_SSPK, 1, 1, {A_SparkFollow}, S_HSPK5}, // S_HSPK4
	{SPR_SSPK, 0, 1, {A_SparkFollow}, S_HSPK1}, // S_HSPK5

	{SPR_REDX, 0, -1, {NULL}, S_NULL}, // S_REDXVI

	// Blue Spring
	{SPR_SPRB, 0, -1, {NULL}, S_NULL},         // S_BLUESPRING
	{SPR_SPRB, 4, 4, {A_Pain}, S_BLUESPRING3}, // S_BLUESPRING2
	{SPR_SPRB, 3, 1, {NULL}, S_BLUESPRING4},   // S_BLUESPRING3
	{SPR_SPRB, 2, 1, {NULL}, S_BLUESPRING5},   // S_BLUESPRING4
	{SPR_SPRB, 1, 1, {NULL}, S_BLUESPRING},    // S_BLUESPRING5

	// Gold Buzz
	{SPR_BUZZ, 0, 2, {A_Look}, S_BUZZLOOK2},   // S_BUZZLOOK1
	{SPR_BUZZ, 1, 2, {A_Look}, S_BUZZLOOK1},   // S_BUZZLOOK2
	{SPR_BUZZ, 0, 2, {A_BuzzFly}, S_BUZZFLY2}, // S_BUZZFLY1
	{SPR_BUZZ, 1, 2, {A_BuzzFly}, S_BUZZFLY1}, // S_BUZZFLY2

	// Red Buzz
	{SPR_RBUZ, 0, 2, {A_Look}, S_RBUZZLOOK2},   // S_RBUZZLOOK1
	{SPR_RBUZ, 1, 2, {A_Look}, S_RBUZZLOOK1},   // S_RBUZZLOOK2
	{SPR_RBUZ, 0, 2, {A_BuzzFly}, S_RBUZZFLY2}, // S_RBUZZFLY1
	{SPR_RBUZ, 1, 2, {A_BuzzFly}, S_RBUZZFLY1}, // S_RBUZZFLY2

	// 8th Emerald
	{SPR_CEMK, 32768, -1, {NULL}, S_NULL}, // S_CEMK

	// 1-Up Box
	{SPR_PRUP, 2, 2, {NULL}, S_PRUP2},			// S_PRUPAUX1
	{SPR_PRUP, 3, 18, {NULL}, S_PRUPAUX3},		// S_PRUPAUX2
	{SPR_PRUP, 3, 18, {A_ExtraLife}, S_DISS},   // S_PRUPAUX3
	{SPR_PRUP, 4, 2, {NULL}, S_PRUP2},			// S_PRUPAUX4
	{SPR_PRUP, 5, 18, {NULL}, S_PRUPAUX6},		// S_PRUPAUX5
	{SPR_PRUP, 5, 18, {A_ExtraLife}, S_DISS},   // S_PRUPAUX6
	{SPR_PRUP, 6, 2, {NULL}, S_PRUP2},			// S_PRUPAUX7
	{SPR_PRUP, 7, 18, {NULL}, S_PRUPAUX9},		// S_PRUPAUX8
	{SPR_PRUP, 7, 18, {A_ExtraLife}, S_DISS},   // S_PRUPAUX9
	{SPR_PRUP, 8, 2, {NULL}, S_PRUP2},			// S_PRUPAUX10
	{SPR_PRUP, 9, 18, {NULL}, S_PRUPAUX12},		// S_PRUPAUX11
	{SPR_PRUP, 9, 18, {A_ExtraLife}, S_DISS},   // S_PRUPAUX12
	{SPR_PRUP, 10, 2, {NULL}, S_PRUP2},			// S_PRUPAUX13
	{SPR_PRUP, 11, 18, {NULL}, S_PRUPAUX15},	// S_PRUPAUX14
	{SPR_PRUP, 11, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX15
	{SPR_PRUP, 12, 2, {NULL}, S_PRUP2},			// S_PRUPAUX16
	{SPR_PRUP, 13, 18, {NULL}, S_PRUPAUX18},	// S_PRUPAUX17
	{SPR_PRUP, 13, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX18
	{SPR_PRUP, 14, 2, {NULL}, S_PRUP2},			// S_PRUPAUX19
	{SPR_PRUP, 15, 18, {NULL}, S_PRUPAUX21},	// S_PRUPAUX20
	{SPR_PRUP, 15, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX21
	{SPR_PRUP, 16, 2, {NULL}, S_PRUP2},			// S_PRUPAUX22
	{SPR_PRUP, 17, 18, {NULL}, S_PRUPAUX24},	// S_PRUPAUX23
	{SPR_PRUP, 17, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX24
	{SPR_PRUP, 18, 2, {NULL}, S_PRUP2},			// S_PRUPAUX25
	{SPR_PRUP, 19, 18, {NULL}, S_PRUPAUX27},	// S_PRUPAUX26
	{SPR_PRUP, 19, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX27
	{SPR_PRUP, 20, 2, {NULL}, S_PRUP2},			// S_PRUPAUX28
	{SPR_PRUP, 21, 18, {NULL}, S_PRUPAUX30},	// S_PRUPAUX29
	{SPR_PRUP, 21, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX30
	{SPR_PRUP, 22, 2, {NULL}, S_PRUP2},			// S_PRUPAUX31
	{SPR_PRUP, 23, 18, {NULL}, S_PRUPAUX33},	// S_PRUPAUX32
	{SPR_PRUP, 23, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX33
	{SPR_PRUP, 24, 2, {NULL}, S_PRUP2},			// S_PRUPAUX34
	{SPR_PRUP, 25, 18, {NULL}, S_PRUPAUX36},	// S_PRUPAUX35
	{SPR_PRUP, 25, 18, {A_ExtraLife}, S_DISS},  // S_PRUPAUX36

	{SPR_TURR, 0, 35,{A_Look}, S_TURRETLOOK},          // S_TURRETLOOK
	{SPR_TURR, 1, 3, {A_FaceTarget}, S_TURRETPOPUP2},  // S_TURRETPOPUP1
	{SPR_TURR, 2, 3, {A_FaceTarget}, S_TURRETPOPUP3},  // S_TURRETPOPUP2
	{SPR_TURR, 3, 3, {A_FaceTarget}, S_TURRETPOPUP4},  // S_TURRETPOPUP3
	{SPR_TURR, 4, 14,{A_FaceTarget}, S_TURRETSHOOT},   // S_TURRETPOPUP4
	{SPR_TURR, 4, 14,{A_JetgShoot}, S_TURRETPOPDOWN1}, // S_TURRETSHOOT
	{SPR_TURR, 3, 3, {NULL}, S_TURRETPOPDOWN2},        // S_TURRETPOPDOWN1
	{SPR_TURR, 2, 3, {NULL}, S_TURRETPOPDOWN3},        // S_TURRETPOPDOWN2
	{SPR_TURR, 1, 3, {NULL}, S_TURRETPOPDOWN4},        // S_TURRETPOPDOWN3
	{SPR_TURR, 0, 35,{NULL}, S_TURRETLOOK},            // S_TURRETPOPDOWN4
};

mobjinfo_t mobjinfo[NUMMOBJTYPES] =
{
	{           // MT_PLAYER
		-1,             // doomednum
		S_PLAY_STND,    // spawnstate
		1,              // spawnhealth
		S_PLAY_RUN1,    // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_thok,       // attacksound
		S_PLAY_PAIN,    // painstate
		MT_THOK,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_PLAY_ATK1,    // missilestate
		S_PLAY_DIE1,    // deathstate
		S_PLAY_DIE1,    // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1000,           // mass
		MT_THOK,        // damage
		sfx_sahitg,     // activesound
		MF_SOLID|MF_SHOOTABLE, // flags
		MT_THOK         // raisestate
	},

	{           // MT_BLUECRAWLA
		3004,           // doomednum
		S_POSS_STND,    // spawnstate
		1,              // spawnhealth
		S_POSS_RUN1,    // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDCRAWLA
		9,              // doomednum
		S_SPOS_STND,    // spawnstate
		1,              // spawnhealth
		S_SPOS_RUN1,    // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		170,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE
		16,                // doomednum
		S_EGGMOBILE_STND,  // spawnstate
		8,                 // spawnhealth
		S_EGGMOBILE_STND,  // seestate
		0,                 // seesound
		8,                 // reactiontime
		sfx_telept,        // attacksound
		S_EGGMOBILE_PAIN,  // painstate
		MT_THOK,           // painchance
		sfx_dmpain,        // painsound
		S_EGGMOBILE_ATK3,  // meleestate
		S_EGGMOBILE_ATK1,  // missilestate
		S_EGGMOBILE_DIE1,  // deathstate
		S_EGGMOBILE_FLEE1, // xdeathstate
		sfx_cybdth,        // deathsound
		4,                 // speed
		24*FRACUNIT,       // radius
		48*FRACUNIT,       // height
		0,                 // mass
		2,                 // damage
		0,                 // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_FLOAT|MF_NOGRAVITY|MF_BOSS, // flags
		S_EGGMOBILE_PANIC1 // raisestate
	},

	{           // MT_ROCKET
		-1,             // doomednum
		S_ROCKET,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_rlaunc,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_rxplod,     // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		100,            // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RING
		2014,           // doomednum
		S_BON1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPERRINGBOX
		2011,           // doomednum
		S_SUPERRINGBOX, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SUPERRINGBOX, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SUPERRINGBOX2,// deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_RINGICO,     // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_GREYRINGBOX
		2012,           // doomednum
		S_GREYRINGBOX,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_GREYRINGBOX,  // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_GREYRINGBOX2, // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_SRINGICO,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMMY
		2013,           // doomednum
		S_EMMY1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_INV
		2022,           // doomednum
		S_PINV,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_PINV,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_PINV3,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_INVCICO,     // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUETV
		2028,           // doomednum
		S_BLTV,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_BLTV,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BLTV2,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_BSHIELDICO,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_FAN
		32,             // doomednum
		S_FAN,          // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5*FRACUNIT,     // speed
		16*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_BUBBLES
		33,             // doomednum
		S_BUBBLES1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER1
		36,             // doomednum
		S_GFZFLOWERA,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWTV
		48,             // doomednum
		S_YLTV,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_YLTV,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_YLTV2,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_YSHIELDICO,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDTV
		2002,           // doomednum
		S_RDTV1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RDTV1,        // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RDTV3,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_RSHIELDICO,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWSPRING
		28,             // doomednum
		S_YELLOWSPRING, // spawnstate
		1000,           // spawnhealth
		S_YELLOWSPRING2,// seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNEAKERTV
		25,             // doomednum
		S_SHTV,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_SHTV,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SHTV2,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_SHOESICO,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDSPRING
		79,             // doomednum
		S_REDSPRING,    // spawnstate
		1000,           // spawnhealth
		S_REDSPRING2,   // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		32*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING, // flags
		S_NULL          // raisestate
	},

	// chase camera
	{           // MT_CHASECAM
		-1,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_FLOAT|MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	// spirit for movement prediction
	{           // MT_SPIRIT
		-1,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_SMOK
		-1,             // doomednum
		S_SMOK1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUESPRING
		5004,           // doomednum
		S_BLUESPRING,   // spawnstate
		1000,           // spawnhealth
		S_BLUESPRING2,  // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		11*FRACUNIT,    // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING, // flags
		S_NULL          // raisestate
	},

	// for use with wind and current effects
	{           // MT_PUSH
		5001,           // doomednum
		S_TNT1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY,  // flags
		S_NULL          // raisestate
	},

	// for use with wind and current effects
	{           // MT_PULL
		5002,           // doomednum
		S_TNT1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGRING
		-1,             // doomednum
		S_BON1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGRING,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_RING,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	// freed birdie
	{           // MT_BIRD
		-1,             // doomednum
		S_BIRD1,        // spawnstate
		1000,           // spawnhealth
		S_BIRD1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING|MF_FLOAT|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	// freed squirrel
	{           // MT_SQRL
		-1,             // doomednum
		S_SQRL1,        // spawnstate
		1000,           // spawnhealth
		S_SQRL1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIPTHING|MF_FLOAT, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDORB
		-1,             // doomednum
		S_RORB1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		pw_fireshield,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_YELLOWORB
		-1,             // doomednum
		S_YORB1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		pw_ringshield,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEORB
		-1,             // doomednum
		S_BORB1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		pw_watershield, // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLACKORB
		-1,             // doomednum
		S_KORB1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		pw_bombshield,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},
	
	{           // MT_WHITEORB
		-1,             // doomednum
		S_WORB1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		pw_jumpshield,  // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPARK
		-1,             // doomednum
		S_SPRK1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// invincibility sparkles
	{           // MT_IVSP
		-1,             // doomednum
		S_IVSP1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_IVSQ
		-1,             // doomednum
		S_IVSQ1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// dissipating object
	{           // MT_DISS
		-1,             // doomednum
		S_DISS,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SMALLBUBBLE
		-1,             // doomednum
		S_SMALLBUBBLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MEDIUMBUBBLE
		-1,             // doomednum
		S_MEDIUMBUBBLE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXTRALARGEBUBBLE
		-1,             // doomednum
		S_LARGEBUBBLE,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// drown counters
	{           // MT_ZERO
		-1,             // doomednum
		S_ZERO1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ONE
		-1,             // doomednum
		S_ONE1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TWO
		-1,             // doomednum
		S_TWO1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THREE
		-1,             // doomednum
		S_THREE1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FOUR
		-1,             // doomednum
		S_FOUR1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIVE
		-1,             // doomednum
		S_FIVE1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_POP
		-1,             // doomednum
		S_POP1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// 1-up box
	{           // MT_PRUP
		41,             // doomednum
		S_PRUP1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_PRUP1,        // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_PRUP3,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_1UPICO,      // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	// bomb shield box
	{           // MT_BLACKTV
		2018,           // doomednum
		S_BKTV1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_BKTV1,        // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_BKTV3,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_KSHIELDICO,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	// jump shield box
	{           // MT_WHITETV
		35,           // doomednum
		S_WHTV1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_WHTV1,        // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_WHTV3,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_WSHIELDICO,  // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	// 100pt score logo
	{           // MT_SCRA
		-1,             // doomednum
		S_SCRA,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		3*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// 200pt score logo
	{           // MT_SCRB
		-1,             // doomednum
		S_SCRB,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// 500pt score logo
	{           // MT_SCRC
		-1,             // doomednum
		S_SCRC,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// 1000pt score logo
	{           // MT_SCRD
		-1,             // doomednum
		S_SCRD,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPERSPARK
		-1,             // doomednum
		S_SSPK1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GRASSDEBRIS
		-1,             // doomednum
		S_GRASS1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// yellow diagonal spring
	{           // MT_YELLOWDIAG
		2015,           // doomednum
		S_YDIAG1,       // spawnstate
		1,              // spawnhealth
		S_YDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		20*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING, // flags
		S_NULL          // raisestate
	},

	// red diagonal spring
	{           // MT_REDDIAG
		38,             // doomednum
		S_RDIAG1,       // spawnstate
		1,              // spawnhealth
		S_RDIAG2,       // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		32*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		32*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPRING, // flags
		S_NULL          // raisestate
	},

	// ambient water 1a (large)
	{           // MT_AWATERA
		2026,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr1,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 1b (large)
	{           // MT_AWATERB
		2024,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr2,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 2a (medium)
	{           // MT_AWATERC
		2023,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr3,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 2b (medium)
	{           // MT_AWATERD
		2045,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr4,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 3a (small)
	{           // MT_AWATERE
		83,             // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr5,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 3b (small)
	{           // MT_AWATERF
		2019,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr6,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 4a (extra large)
	{           // MT_AWATERG
		2025,           // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr7,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	// ambient water 4b (extra large)
	{           // MT_AWATERH
		27,             // doomednum
		S_NULL,         // spawnstate
		35,             // spawnhealth
		S_NULL,         // seestate
		sfx_amwtr8,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_SKIM
		56,             // doomednum
		S_SKIM1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_SKIM3,        // meleestate
		S_NULL,         // missilestate
		S_SKIM_BOOM1,   // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_NOGRAVITY|MF_SHOOTABLE, // flags
		MT_MINE         // raisestate
	},

	{           // MT_MINE
		-1,             // doomednum
		S_MINE1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_MINE_BOOM1,   // deathstate
		S_MINE_BOOM1,   // xdeathstate
		sfx_cybdth,     // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		10*FRACUNIT,    // height
		100,            // mass
		64,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFISH
		58,             // doomednum
		S_FISH2,        // spawnstate
		1,              // spawnhealth
		S_FISH1,        // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FISH3,        // meleestate
		S_NULL,         // missilestate
		S_FISH_DIE1,    // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		28*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_GARGOYLE
		81,             // doomednum
		S_GARGOYLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		21*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPLISH
		-1,             // doomednum
		S_SPLISH1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		6*FRACUNIT,     // radius
		FRACUNIT,       // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THOK
		-1,             // doomednum
		S_THOK1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY|MF_TRANSLATION, // flags
		S_NULL          // raisestate
	},

	{           // MT_THZPLANT
		2035,           // doomednum
		S_THZPLANT1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SIGN
		86,             // doomednum
		S_SIGN52,       // spawnstate
		1000,           // spawnhealth
		S_SIGN53,       // seestate
		sfx_lvpass,      // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDRING
		-1,             // doomednum
		S_RRNG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TAG
		-1,             // doomednum
		S_TTAG1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STEAM
		30,             // doomednum
		S_STEAM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_steam2,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_steam1,     // deathsound
		20*FRACUNIT,    // speed
		32*FRACUNIT,    // radius
		1*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_REDFLAG
		31,             // doomednum
		S_REDFLAG,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,     // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEFLAG
		34,             // doomednum
		S_BLUEFLAG,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		16*FRACUNIT,     // radius
		64*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTFLAG
		-1,             // doomednum
		S_GOTFLAG1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOTFLAG2
		-1,             // doomednum
		S_GOTFLAG3,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		64*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TOKEN
		-1,             // doomednum
		S_TOKEN,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_GREENEMERALD
		-1,             // doomednum
		S_CEMG,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ORANGEEMERALD
		-1,             // doomednum
		S_CEMO,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_PINKEMERALD
		-1,             // doomednum
		S_CEMP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEEMERALD
		-1,             // doomednum
		S_CEMB,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDEMERALD
		-1,             // doomednum
		S_CEMR,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_LIGHTBLUEEMERALD
		-1,             // doomednum
		S_CEML,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GREYEMERALD
		-1,             // doomednum
		S_CEMY,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETTBOMBER
		3005,           // doomednum
		S_JETBLOOK1,    // spawnstate
		1,              // spawnhealth
		S_JETBZOOM1,    // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		MT_MINE         // raisestate
	},

	{           // MT_JETTGUNNER
		22,             // doomednum
		S_JETGLOOK1,    // spawnstate
		1,              // spawnhealth
		S_JETGZOOM1,    // seestate
		0,              // seesound
		5,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		0,              // painsound
		0,              // meleestate
		S_JETGSHOOT1,   // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		MT_JETTBULLET   // raisestate
	},

	{           // MT_JETTBULLET
		-1,             // doomednum
		S_JETBULLET1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		1,              // damage
		0,              // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	// freed mouse
	{           // MT_MOUSE
		-1,             // doomednum
		S_MOUSE1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		0,              // seesound
		1,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_NOCLIPTHING, // flags
		S_NULL          // raisestate
	},

	{           // MT_DETON
		71,             // doomednum
		S_DETON1,       // spawnstate
		1,              // spawnhealth
		S_DETON2,       // seestate
		0,              // seesound
		1,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		1*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		0,              // activesound
		MF_ENEMY|MF_SHOOTABLE|MF_NOGRAVITY|MF_MISSILE, // flags
		ANG90/6         // raisestate: largest angle to turn in one tic (here, 15 degrees)
	},

	{           // MT_EXPLODE
		-1,             // doomednum
		S_XPLD1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// CEZ chain
	{           // MT_CHAIN
		49,             // doomednum
		S_CEZCHAIN,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		128*FRACUNIT,   // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CAPE
		-1,             // doomednum
		S_CAPE1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_NOBLOCKMAP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWFLAKE
		-1,             // doomednum
		S_SNOW1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-2*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_SANTA
		63,             // doomednum
		S_SANTA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	// first hunt emerald
	{           // MT_EMERHUNT
		64,             // doomednum
		S_EMER1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	// second hunt emerald
	{           // MT_EMESHUNT
		3002,           // doomednum
		S_EMES1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	// third hunt emerald
	{           // MT_EMETHUNT
		3001,           // doomednum
		S_EMET1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWBALL
		-1,             // doomednum
		S_SBLL1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPIKEBALL
		-1,             // doomednum
		S_SPIKEBALL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRAWLACOMMANDER
		21,             // doomednum
		S_CCOMMAND1,    // spawnstate
		2,              // spawnhealth
		S_CCOMMAND3,    // seestate
		0,              // seesound
		2*TICRATE,      // reactiontime
		0,              // attacksound
		S_CCOMMAND1,    // painstate
		200,            // painchance
		sfx_dmpain,     // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		3,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_CRUMBLEOBJ
		-1,             // doomednum
		S_CRUMBLE1,     // spawnstate
		1000,           // spawnhealth
		S_CRUMBLE1,     // seestate
		0,              // seesound
		1,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_crumbl,     // deathsound
		3,              // speed
		1*FRACUNIT,     // radius
		1*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	// yellow diagonal upside-down spring
	{           // MT_YELLOWDIAGDOWN
		20,             // doomednum
		S_YDIAGD1,      // spawnstate
		1,              // spawnhealth
		S_YDIAGD2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-20*FRACUNIT,   // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		20*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_SPRING|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	// red diagonal upside-down spring
	{           // MT_REDDIAGDOWN
		39,             // doomednum
		S_RDIAGD1,      // spawnstate
		1,              // spawnhealth
		S_RDIAGD2,      // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-32*FRACUNIT,   // speed
		16*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		32*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_SPRING|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	// upside-down yellow spring
	{           // MT_YELLOWSPRINGDOWN
		65,             // doomednum
		S_YELLOWSPRINGUD, // spawnstate
		1000,           // spawnhealth
		S_YELLOWSPRINGUD2, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-20*FRACUNIT,   // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_SPRING|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	// upside-down red spring
	{           // MT_REDSPRINGDOWN
		66,             // doomednum
		S_REDSPRINGUD,  // spawnstate
		1000,           // spawnhealth
		S_REDSPRINGUD2, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_spring,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-32*FRACUNIT,   // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_NOGRAVITY|MF_SPRING|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAIN
		-1,             // doomednum
		S_RAIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-24*FRACUNIT,   // speed
		1*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_CEILINGSPIKE
		67,             // doomednum
		S_CEILINGSPIKE, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		42*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SPAWNCEILING, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLOORSPIKE
		68,             // doomednum
		S_FLOORSPIKE,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		1*FRACUNIT,     // radius
		42*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID,       // flags
		S_NULL          // raisestate
	},

	{           // MT_STARPOST
		3006,           // doomednum
		S_STARPOST1,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		80*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SPECIALFLAGS, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPECIALSPIKEBALL
		23,             // doomednum
		S_SPIKEBALL1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOMINGRING
		69,             // doomednum
		S_HOMINGRING1,  // spawnstate
		30*TICRATE,     // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RAILRING
		3003,           // doomednum
		S_RAILRING1,    // spawnstate
		45*TICRATE,     // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_INFINITYRING
		80,             // doomednum
		S_INFINITYRING1,// spawnstate
		15*TICRATE,     // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_AUTOMATICRING
		26,             // doomednum
		S_AUTOMATICRING1, // spawnstate
		45*TICRATE,     // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EXPLOSIONRING
		54,             // doomednum
		S_EXPLOSIONRING1, // spawnstate
		30*TICRATE,     // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNAUTOMATICEXPLOSIONHOMING
		-1,             // doomednum
		S_THROWNAUTOMATICEXPLOSIONHOMING1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNAUTOMATICEXPLOSION
		-1,             // doomednum
		S_THROWNAUTOMATICEXPLOSION1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNAUTOMATICHOMING
		-1,             // doomednum
		S_THROWNAUTOMATICHOMING1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNHOMINGEXPLOSION
		-1,             // doomednum
		S_THROWNHOMINGEXPLOSION1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNHOMING
		-1,             // doomednum
		S_THROWNHOMING1,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		20*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNAUTOMATIC
		-1,             // doomednum
		S_THROWNAUTOMATIC1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_SPRK1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_THROWNEXPLOSION
		-1,             // doomednum
		S_THROWNEXPLOSION1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_thok,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RINGEXPLODE,  // deathstate
		S_NULL,         // xdeathstate
		sfx_itemup,     // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BERRYBUSH
		74,             // doomednum
		S_BERRYBUSH,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_BUSH
		75,             // doomednum
		S_BUSH,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER2
		70,             // doomednum
		S_GFZFLOWERB1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		96*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER3
		73,             // doomednum
		S_GFZFLOWERC1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MIXUPBOX
		78,             // doomednum
		S_MIXUPBOX1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_MIXUPBOX1,    // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_MIXUPBOX3,    // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_MIXUPICO,    // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_QUESTIONBOX
		3000,           // doomednum
		S_RANDOMBOX1,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_RANDOMBOX1,   // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_RANDOMBOX3,   // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_MONITOREXPLOSION
		-1,             // doomednum
		S_MONITOREXPLOSION1, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_MONITOREXPLOSION1, // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_RINGICO
		-1,             // doomednum
		S_SUPERRINGBOX2, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_itemup,     // seesound
		10,             // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SRINGICO
		-1,             // doomednum
		S_GREYRINGBOX2, // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_itemup,     // seesound
		25,             // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_1UPICO
		-1,             // doomednum
		S_PRUP3,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_BSHIELDICO
		-1,             // doomednum
		S_BLTV2,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_YSHIELDICO
		-1,             // doomednum
		S_YLTV2,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_KSHIELDICO
		-1,             // doomednum
		S_BKTV3,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},
	
	{           // MT_WSHIELDICO
		-1,             // doomednum
		S_WHTV3,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_RSHIELDICO
		-1,             // doomednum
		S_RDTV3,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_shield,     // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_INVCICO
		-1,             // doomednum
		S_PINV3,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_MIXUPICO
		-1,             // doomednum
		S_MIXUPBOX3,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SHOESICO
		-1,             // doomednum
		S_SHTV2,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLAME
		24,             // doomednum
		S_FLAME1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_PUMA
		29,             // doomednum
		S_PUMA1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},
	{           // MT_HAMMER
		-1,             // doomednum
		S_HAMMER1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},
	{           // MT_KOOPA
		19,             // doomednum
		S_KOOPA1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_DISS,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		48*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_SHELL
		10,             // doomednum
		S_SHELL,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXE
		12,             // doomednum
		S_AXE1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_KOOPAFLAME
		-1,             // doomednum
		S_KOOPAFLAME1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_MISSILE|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREBALL
		-1,             // doomednum
		S_FIREBALL1,    // spawnstate
		1000,           // spawnhealth
		S_FIREBALLEXP1, // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_FIREBALLEXP1, // meleestate
		S_FIREBALLEXP1, // missilestate
		S_FIREBALLEXP1, // deathstate
		S_FIREBALLEXP1, // xdeathstate
		sfx_None,       // deathsound
		10*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_FIREFLOWER
		50,             // doomednum
		S_FIREFLOWER1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSPARKLE
		-1,             // doomednum
		S_NIGHTSPARKLE1,// spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	// axis for NiGHTS maps
	{           // MT_AXIS1
		52,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		1024*FRACUNIT,  // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS2
		53,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20,             // speed
		512*FRACUNIT,   // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS3
		59,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5,              // speed
		2048*FRACUNIT,  // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS1A
		62,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		1024*FRACUNIT,  // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_AMBUSH, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS2A
		15,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		20,             // speed
		512*FRACUNIT,   // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_AMBUSH, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXIS3A
		45,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		5,              // speed
		2048*FRACUNIT,  // radius
		FRACUNIT,       // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOCLIP|MF_AMBUSH, // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFER
		61,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		16*FRACUNIT,    // radius
		1,              // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERCONDITION
		82,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		16*FRACUNIT,    // radius
		1,              // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERCONDITION2
		85,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		16*FRACUNIT,    // radius
		1,              // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERCLOSEST
		46,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		32*FRACUNIT,    // radius
		1,              // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_AXISTRANSFERTOLAST
		55,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		10,             // speed
		1,              // radius
		1,              // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR,    // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRONE
		60,             // doomednum
		S_NIGHTSDRONE1, // spawnstate
		120,            // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSCHAR
		-1,             // doomednum
		S_NIGHTSFLY1A,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NIGHTSFLY1A,  // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NIGHTSFLY1A,  // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP, // flags
		S_NULL          // raisestate
	},

	{           // MT_SEED
		-1,             // doomednum
		S_SEED,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		-2*FRACUNIT,    // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETFUME1
		-1,             // doomednum
		S_JETFUME1,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_JETFUME2
		-1,             // doomednum
		S_JETFUME3,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOP
		-1,             // doomednum
		S_HOOP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCOLLIDE
		-1,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_HOOPCENTER
		-1,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSCORE
		-1,             // doomednum
		S_NIGHTSCORE10, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NIGHTSCORE10_2, // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSWING
		37,             // doomednum
		S_NIGHTSWING,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSFLYPOINT
		17,             // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMOBILE2
		2008,              // doomednum
		S_EGGMOBILE2_STND, // spawnstate
		8,                 // spawnhealth
		S_EGGMOBILE2_STND, // seestate
		0,                 // seesound
		8,                 // reactiontime
		sfx_gspray,        // attacksound
		S_EGGMOBILE2_PAIN, // painstate
		MT_GOOP,           // painchance
		sfx_dmpain,        // painsound
		S_EGGMOBILE2_STND, // meleestate
		S_EGGMOBILE2_STND, // missilestate
		S_EGGMOBILE2_DIE1, // deathstate
		S_EGGMOBILE2_FLEE1,// xdeathstate
		sfx_cybdth,        // deathsound
		4*FRACUNIT,        // speed
		24*FRACUNIT,       // radius
		48*FRACUNIT,       // height
		0,                 // mass
		2,                 // damage
		sfx_pogo,          // activesound
		MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY|MF_BOSS, // flags
		S_EGGMOBILE2_POGO1 // raisestate
	},

	{           // MT_GOOP
		-1,             // doomednum
		S_GOOP1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_ghit,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		4*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSEXPLODE
		-1,             // doomednum
		S_BPLD1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGTRAP
		2049,           // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_CHAOSSPAWNER
		8,              // doomednum
		S_NULL,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_ALARM
		2006,           // doomednum
		S_ALARM1,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_alarm,      // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SPAWNCEILING|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EMBLEM
		-1,             // doomednum
		S_EMBLEM1,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		4,              // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMANBOX
		2005,           // doomednum
		S_EGGTV1,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_EGGTV1,       // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EGGTV3,       // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		MT_MONITOREXPLOSION, // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		MT_EGGMANICO,   // damage
		sfx_None,       // activesound
		MF_SOLID|MF_SHOOTABLE|MF_MONITOR, // flags
		S_NULL          // raisestate
	},

	{           // MT_EGGMANICO
		-1,             // doomednum
		S_EGGTV3,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2*FRACUNIT,     // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		62*FRACUNIT,    // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY|MF_NOGRAVITY|MF_MONITORICON, // flags
		S_NULL          // raisestate
	},

	{           // MT_SPINFIRE
		-1,             // doomednum
		S_SPINFIRE1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		14*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY|MF_FIRE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSTANK1
		-1,             // doomednum
		S_BOSSTANK1,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		64*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSTANK2
		-1,             // doomednum
		S_BOSSTANK2,    // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		64*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_BOSSSPIGOT
		-1,             // doomednum
		S_BOSSSPIGOT,   // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_TFOG
		-1,             // doomednum
		S_FOG1,         // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIP, // flags
		S_NULL          // raisestate
	},

	{           // MT_EASTEREGG
		-1,             // doomednum
		S_EEGG,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SPECIAL|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_STREETLIGHT
		2003,           // doomednum
		S_LITE,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		0,              // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		2*FRACUNIT,     // radius
		4*FRACUNIT,     // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_TURRET
		2004,           // doomednum
		S_TURRET,       // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_trfire,     // attacksound
		0,              // painstate
		0,              // painchance
		sfx_fizzle,     // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TURRETSHOCK1, // deathstate
		S_NULL,         // xdeathstate
		sfx_turpop,     // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_trpowr,     // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_TURRETLASER
		-1,             // doomednum
		S_TURRETLASER,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_TURRETLASEREXPLODE1, // deathstate
		S_NULL,         // xdeathstate
		sfx_turhit,     // deathsound
		50*FRACUNIT,    // speed
		12*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GFZFLOWER4
		2001,           // doomednum
		S_GFZFLOWERD1,  // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_GOOMBA
		10000,          // doomednum
		S_GOOMBA1,      // spawnstate
		1,              // spawnhealth
		S_GOOMBA2,      // seestate
		0,              // seesound
		32,             // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		200,            // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_GOOMBA_DEAD,  // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		6,              // speed
		24*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL          // raisestate
	},

	{           // MT_BLUEGOOMBA
		10001,             // doomednum
		S_BLUEGOOMBA1,     // spawnstate
		1,                 // spawnhealth
		S_BLUEGOOMBA2,     // seestate
		0,                 // seesound
		32,                // reactiontime
		0,                 // attacksound
		S_NULL,            // painstate
		170,               // painchance
		0,                 // painsound
		0,                 // meleestate
		S_NULL,            // missilestate
		S_BLUEGOOMBA_DEAD, // deathstate
		S_NULL,            // xdeathstate
		sfx_pop,           // deathsound
		6,                 // speed
		24*FRACUNIT,       // radius
		32*FRACUNIT,       // height
		100,               // mass
		0,                 // damage
		0,                 // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		S_NULL             // raisestate
	},

	{           // MT_MARIOBUSH1
		10002,          // doomednum
		S_MARIOBUSH1,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_MARIOBUSH2
		10003,          // doomednum
		S_MARIOBUSH2,   // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_TOAD
		10004,          // doomednum
		S_TOAD,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_COIN
		10005,          // doomednum
		S_COIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGCOIN,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_COINSPARKLE
		-1,             // doomednum
		S_COINSPARKLE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		32*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_FLINGCOIN
		-1,             // doomednum
		S_COIN1,        // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		MT_FLINGCOIN,   // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		MT_COIN,        // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		60*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_XMASPOLE
		5,              // doomednum
		S_XMASPOLE,     // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_CANDYCANE
		13,             // doomednum
		S_CANDYCANE,    // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		32*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_SNOWMAN
		6,              // doomednum
		S_SNOWMAN,      // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		25*FRACUNIT,    // speed
		16*FRACUNIT,    // radius
		40*FRACUNIT,    // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_SLIDEME|MF_SOLID|MF_PUSHABLE, // flags
		S_NULL          // raisestate
	},
	{           // MT_EGGCAPSULE
		40,             // doomednum
		S_EGGCAPSULE,   // spawnstate
		20,             // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		72*FRACUNIT,    // radius
		144*FRACUNIT,   // height
		100,            // mass
		1,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_FLOAT|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},

	{           // MT_SUPERTRANS
		-1,             // doomednum
		S_SUPERTRANS1,  // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		56*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_RANDOMAMBIENT
		14,             // doomednum
		S_NULL,         // spawnstate
		1075,           // spawnhealth: repeat speed
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	},

	{           // MT_ROCKCRUMBLE1
		-1,             // doomednum
		S_ROCKCRUMBLEA1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE2
		-1,             // doomednum
		S_ROCKCRUMBLEB1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE3
		-1,             // doomednum
		S_ROCKCRUMBLEC1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE4
		-1,             // doomednum
		S_ROCKCRUMBLED1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE5
		-1,             // doomednum
		S_ROCKCRUMBLEE1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE6
		-1,             // doomednum
		S_ROCKCRUMBLEF1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE7
		-1,             // doomednum
		S_ROCKCRUMBLEG1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE8
		-1,             // doomednum
		S_ROCKCRUMBLEH1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE9
		-1,             // doomednum
		S_ROCKCRUMBLEI1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE10
		-1,             // doomednum
		S_ROCKCRUMBLEJ1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE11
		-1,             // doomednum
		S_ROCKCRUMBLEK1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE12
		-1,             // doomednum
		S_ROCKCRUMBLEL1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE13
		-1,             // doomednum
		S_ROCKCRUMBLEM1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE14
		-1,             // doomednum
		S_ROCKCRUMBLEN1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE15
		-1,             // doomednum
		S_ROCKCRUMBLEO1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

{           // MT_ROCKCRUMBLE16
		-1,             // doomednum
		S_ROCKCRUMBLEP1, // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_ambint,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP,  // flags
		S_NULL          // raisestate
	},

	{           // MT_TELEPORTMAN
		5003,           // doomednum
		S_TNT1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSSUPERLOOP
		3007,           // doomednum
		S_NIGHTSPOWERUP1, // spawnstate
		1000,           // spawnhealth
		S_NIGHTSPOWERUP2, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSDRILLREFILL
		3008,           // doomednum
		S_NIGHTSPOWERUP3, // spawnstate
		1000,           // spawnhealth
		S_NIGHTSPOWERUP4, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_NIGHTSHELPER
		3009,           // doomednum
		S_NIGHTSPOWERUP5, // spawnstate
		1000,           // spawnhealth
		S_NIGHTSPOWERUP6, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_DISS,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		16*FRACUNIT,    // radius
		32*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY,   // flags
		S_NULL          // raisestate
	},

	{           // MT_HYPERSPARK
		-1,             // doomednum
		S_HSPK1,        // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		40*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		100,            // mass
		20,             // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_SCENERY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDXVI
		-1,             // doomednum
		S_REDXVI,       // spawnstate
		1,              // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		32*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		100,            // mass
		20,             // damage
		sfx_None,       // activesound
		MF_SPECIAL,     // flags
		S_NULL          // raisestate
	},

	{           // MT_GOLDBUZZ
		5005,           // doomednum
		S_BUZZLOOK1,    // spawnstate
		1,              // spawnhealth
		S_BUZZFLY1,     // seestate
		0,              // seesound
		2,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		4*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},

	{           // MT_REDBUZZ
		5006,           // doomednum
		S_RBUZZLOOK1,   // spawnstate
		1,              // spawnhealth
		S_RBUZZFLY1,    // seestate
		0,              // seesound
		2,              // reactiontime
		0,              // attacksound
		S_NULL,         // painstate
		3072,           // painchance
		0,              // painsound
		0,              // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		8*FRACUNIT,     // speed
		20*FRACUNIT,    // radius
		24*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		0,              // activesound
		MF_SLIDEME|MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD1
		420,            // doomednum
		S_CEMG,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		1,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD2
		421,            // doomednum
		S_CEMO,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		2,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD3
		422,            // doomednum
		S_CEMP,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		4,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD4
		423,            // doomednum
		S_CEMB,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		8,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD5
		424,            // doomednum
		S_CEMR,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		16,             // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD6
		425,            // doomednum
		S_CEML,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		32,             // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD7
		426,            // doomednum
		S_CEMY,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		64,             // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{           // MT_EMERALD8
		427,            // doomednum
		S_CEMK,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		256,            // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		16,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOGRAVITY|MF_SPECIAL, // flags
		S_NULL          // raisestate
	},
	{			// MT_ALTVIEWMAN
		5007,           // doomednum
		S_TNT1,         // spawnstate
		1000,           // spawnhealth
		S_NULL,         // seestate
		sfx_None,       // seesound
		8,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		0,              // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8,              // radius
		8,              // height
		10,             // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOBLOCKMAP|MF_NOGRAVITY, // flags
		S_NULL          // raisestate
	},
	{           // MT_POPUPTURRET
		42,             // doomednum
		S_TURRETLOOK,   // spawnstate
		1,              // spawnhealth
		S_TURRETPOPUP1, // seestate
		sfx_None,       // seesound
		0,              // reactiontime
		sfx_trfire,     // attacksound
		S_NULL,         // painstate
		1024,           // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_XPLD1,        // deathstate
		S_NULL,         // xdeathstate
		sfx_pop,        // deathsound
		0,              // speed
		12*FRACUNIT,    // radius
		64*FRACUNIT,    // height
		100,            // mass
		0,              // damage
		sfx_None,       // activesound
		MF_ENEMY|MF_SPECIAL|MF_SHOOTABLE, // flags
		MT_JETTBULLET   // raisestate
	},
	{           // MT_RANDOMAMBIENT2
		43,             // doomednum
		S_NULL,         // spawnstate
		220,           // spawnhealth: repeat speed
		S_NULL,         // seestate
		sfx_ambin2,     // seesound
		0,              // reactiontime
		sfx_None,       // attacksound
		S_NULL,         // painstate
		255,            // painchance
		sfx_None,       // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		sfx_None,       // deathsound
		0,              // speed
		8*FRACUNIT,     // radius
		16*FRACUNIT,    // height
		1000,           // mass
		0,              // damage
		sfx_None,       // activesound
		MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY|MF_AMBIENT, // flags
		S_NULL          // raisestate
	}
};


/** Patches the mobjinfo table and state table.
  * Free slots are emptied out and set to initial values.
  * If NEWTICRATERATIO is not 1, times are recomputed.
  */
void P_PatchInfoTables(void)
{
	int i;
	char *tempname;

#if NUMSPRITEFREESLOTS > 1000
"Update P_PatchInfoTables, you big dumb head"
#endif

	// empty out free slots
	for(i = SPR_FIRSTFREESLOT; i <= SPR_LASTFREESLOT; i++)
	{
		sprnames[i] = tempname = Z_Malloc(5, PU_STATIC, NULL);
		tempname[0] = 'F';
		tempname[1] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)/100));
		tempname[2] = (char)('0' + (char)(((i-SPR_FIRSTFREESLOT+1)/10)%10));
		tempname[3] = (char)('0' + (char)((i-SPR_FIRSTFREESLOT+1)%10));
		tempname[4] = '\0';

#ifdef HWRENDER
		t_lspr[i] = &lspr[NOLIGHT];
#endif
	}
	sprnames[i] = NULL; // i == NUMSPRITES
	memset(&states[S_FIRSTFREESLOT], 0, sizeof(state_t) * NUMSTATEFREESLOTS);
	memset(&mobjinfo[MT_FIRSTFREESLOT], 0, sizeof(mobjinfo_t) * NUMMOBJFREESLOTS);
	for(i = MT_FIRSTFREESLOT; i <= MT_LASTFREESLOT; i++)
		mobjinfo[i].doomednum = -1;

#if NEWTICRATERATIO != 1
	for(i = 0; i < MT_FIRSTFREESLOT; i++)
		mobjinfo[i].reactiontime *= NEWTICRATERATIO;

	for(i = 0; i < NUMSTATES; i++)
		states[i].tics *= NEWTICRATERATIO;
#endif
}
