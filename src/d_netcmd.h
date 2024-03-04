// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
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
/// \brief host/client network commands
///	commands are executed through the command buffer
///	like console commands

#ifndef __D_NETCMD__
#define __D_NETCMD__

#include "command.h"

// console vars
extern consvar_t cv_playername;
extern consvar_t cv_playercolor;
extern consvar_t cv_usemouse;
extern consvar_t cv_usejoystick;
extern consvar_t cv_usejoystick2;
#ifdef LJOYSTICK
extern consvar_t cv_joyport;
extern consvar_t cv_joyport2;
#endif
extern consvar_t cv_joyscale;
extern consvar_t cv_joyscale2;
extern consvar_t cv_controlperkey;

// splitscreen with second mouse
extern consvar_t cv_mouse2port;
extern consvar_t cv_usemouse2;
#ifdef LMOUSE2
extern consvar_t cv_mouse2opt;
#endif
extern consvar_t cv_invertmouse2;
extern consvar_t cv_alwaysfreelook2;
extern consvar_t cv_mousemove2;
extern consvar_t cv_mousesens2;
extern consvar_t cv_mlooksens2;

// normally in p_mobj but the .h is not read
extern consvar_t cv_itemrespawntime;
extern consvar_t cv_itemrespawn;

extern consvar_t cv_flagtime;
extern consvar_t cv_suddendeath;

extern consvar_t cv_skin;

// secondary splitscreen player
extern consvar_t cv_playername2;
extern consvar_t cv_playercolor2;
extern consvar_t cv_skin2;

extern consvar_t cv_teamplay;
extern consvar_t cv_teamdamage;
extern consvar_t cv_pointlimit;
extern consvar_t cv_timelimit;
extern ULONG timelimitintics;
extern consvar_t cv_allowexitlevel;

extern consvar_t cv_useranalog, cv_useranalog2;
extern consvar_t cv_analog, cv_analog2;

extern consvar_t cv_netstat;
extern consvar_t cv_translucency;
extern consvar_t cv_splats;

extern consvar_t cv_countdowntime;
extern consvar_t cv_runscripts;
extern consvar_t cv_mute;
extern consvar_t cv_killingdead;
extern consvar_t cv_pause, cv_friendlyfire;
extern consvar_t cv_timeattacked;

extern consvar_t cv_teleporters, cv_superring, cv_silverring, cv_supersneakers, cv_invincibility;
extern consvar_t cv_jumpshield, cv_watershield, cv_ringshield, cv_fireshield, cv_bombshield;
extern consvar_t cv_1up, cv_eggmanbox, cv_questionbox;

extern consvar_t cv_objectplace, cv_objflags, cv_mapthingnum, cv_speed, cv_snapto, cv_grid;

extern consvar_t cv_inttime, cv_advancemap;
extern consvar_t cv_soniccd;
extern consvar_t cv_match_scoring;

extern consvar_t cv_realnames;

extern consvar_t cv_autoaim, cv_autoaim2;
extern consvar_t cv_playerspeed, cv_ringslinger, cv_startrings, cv_startlives, cv_startcontinues;

extern consvar_t cv_specialrings, cv_matchboxes, cv_racetype, cv_raceitemboxes;
extern consvar_t cv_chaos_spawnrate, cv_chaos_bluecrawla, cv_chaos_redcrawla;
extern consvar_t cv_chaos_crawlacommander, cv_chaos_jettysynbomber, cv_chaos_jettysyngunner;
extern consvar_t cv_chaos_eggmobile1, cv_chaos_eggmobile2, cv_chaos_skim;

extern consvar_t cv_sleep;

typedef enum
{
	XD_NAMEANDCOLOR = 1,
	XD_WEAPONPREF,
	XD_KICK,
	XD_NETVAR,
	XD_SAY,
	XD_MAP,
	XD_EXITLEVEL,
	XD_ADDFILE,
	XD_LOADGAME,
	XD_SAVEGAME,
	XD_PAUSE,
	XD_ADDPLAYER,
	XD_TEAMCHANGE,
	XD_CLEARSCORES,
	XD_LOGIN,
	XD_VERIFIED,
	XD_RANDOMSEED,
	XD_ORDERPIZZA,
	MAXNETXCMD
} netxcmd_t;

// add game commands, needs cleanup
void D_RegisterServerCommands (void);
void D_RegisterClientCommands(void);
void D_SendPlayerConfig(void);
void Command_ExitGame_f(void);
void D_GameTypeChanged(int lastgametype); // not a real _OnChange function anymore
void D_MapChange(int mapnum, int gametype, skill_t skill, int resetplayers, int delay, boolean skipprecutscene);

#endif
