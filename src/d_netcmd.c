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
///	other miscellaneous commands (at the end)

#include "doomdef.h"

#include "console.h"
#include "command.h"

#include "i_system.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "m_menu.h"
#include "r_local.h"
#include "r_things.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "mserv.h"
#include "md5.h"

#ifdef JOHNNYFUNCODE
#define CV_JOHNNY CV_NETVAR
#else
#define CV_JOHNNY 0
#endif

// ------
// protos
// ------

static void Got_NameAndcolor(char** cp, int playernum);
static void Got_WeaponPref(char** cp, int playernum);
static void Got_Mapcmd(char** cp, int playernum);
static void Got_ExitLevelcmd(char** cp, int playernum);
static void Got_Addfilecmd(char** cp, int playernum);
static void Got_LoadGamecmd(char** cp, int playernum);
static void Got_SaveGamecmd(char** cp, int playernum);
static void Got_Pause(char** cp, int playernum);
static void Got_RandomSeed(char** cp, int playernum);
static void Got_PizzaOrder(char** cp, int playernum);

static void Got_Teamchange(char** cp, int playernum);
static void Got_Clearscores(char** cp, int playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void ObjectPlace_OnChange(void);
static void Mute_OnChange(void);

static void Playerspeed_OnChange(void);
static void Ringslinger_OnChange(void);
static void Startrings_OnChange(void);
static void Startlives_OnChange(void);
static void Startcontinues_OnChange(void);
static void Gravity_OnChange(void);
static void ForceSkin_OnChange(void);
static void Skin_OnChange(void);
static void Skin2_OnChange(void);
static void DummyConsvar_OnChange(void);

//#define FISHCAKE /// \todo Remove this to disable cheating. Remove for release!

#ifdef FISHCAKE
static void Fishcake_OnChange(void);
#endif

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_Map_f(void);
static void Command_Teleport_f(void);

static void Command_OrderPizza_f(void);

static void Command_Addfile(void);
static void Command_Pause(void);

static void Command_Version_f(void);
static void Command_ShowGametype_f(void);
static void Command_Nodes_f(void);
static void Command_Quit_f(void);
static void Command_Playintro_f(void);
static void Command_Writethings_f(void);

static void Command_Displayplayer_f(void);
static void Command_Tunes_f(void);

static void Command_ExitLevel_f(void);
static void Command_Load_f(void);
static void Command_Save_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);
static void Command_SetForcedSkin_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Login(char** cp, int playernum);
static void Got_Verification(char** cp, int playernum);
static void Command_Verify_f(void);

static void Command_Isgamemodified_f(void);

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

static void SendWeaponPref(void);
static void SendNameAndColor(void);
static void SendNameAndColor2(void);

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};
#ifdef LMOUSE2
static CV_PossibleValue_t mouse2port_cons_t[] = {{0, "/dev/gpmdata"}, {1, "/dev/ttyS0"},
	{2, "/dev/ttyS1"}, {3, "/dev/ttyS2"}, {4, "/dev/ttyS3"}, {0, NULL}};
#else
static CV_PossibleValue_t mouse2port_cons_t[] = {{1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
	{0, NULL}};
#endif

#ifdef LJOYSTICK
static CV_PossibleValue_t joyport_cons_t[] = {{1, "/dev/js0"}, {2, "/dev/js1"}, {3, "/dev/js2"},
	{4, "/dev/js3"}, {0, NULL}};
#else
// accept whatever value - it is in fact the joystick device number
#define usejoystick_cons_t NULL
#endif

static CV_PossibleValue_t teamplay_cons_t[] = {{0, "Off"}, {1, "Color"}, {2, "Skin"}, {0, NULL}};

static CV_PossibleValue_t ringlimit_cons_t[] = {{0, "MIN"}, {999999, "MAX"}, {0, NULL}};
static CV_PossibleValue_t liveslimit_cons_t[] = {{0, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sleeping_cons_t[] = {{-1, "MIN"}, {TICRATE/1000, "MAX"}, {0, NULL}};

static CV_PossibleValue_t racetype_cons_t[] = {{0, "Full"}, {1, "Time_Only"}, {0, NULL}};
static CV_PossibleValue_t raceitemboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Teleports"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t matchboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Non-Random"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t chances_cons_t[] = {{0, "Off"}, {1, "Low"}, {2, "Medium"}, {3, "High"},
	{0, NULL}};
static CV_PossibleValue_t snapto_cons_t[] = {{0, "Off"}, {1, "Floor"}, {2, "Ceiling"}, {3, "Halfway"},
	{0, NULL}};
static CV_PossibleValue_t match_scoring_cons_t[] = {{0, "Normal"}, {1, "Classic"}, {0, NULL}};
static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

#ifdef FISHCAKE
static consvar_t cv_fishcake = {"fishcake", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, Fishcake_OnChange, 0, NULL, NULL, 0, 0, NULL};
#endif
static consvar_t cv_dummyconsvar = {"dummyconsvar", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff,
	DummyConsvar_OnChange, 0, NULL, NULL, 0, 0, NULL};

static consvar_t cv_allowteamchange = {"allowteamchange", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_racetype = {"racetype", "Full", CV_NETVAR, racetype_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_raceitemboxes = {"race_itemboxes", "Random", CV_NETVAR, raceitemboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// these two are just meant to be saved to the config
consvar_t cv_playername = {"name", "Sonic", CV_CALL|CV_NOINIT, NULL, SendNameAndColor, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playercolor = {"color", "Blue", CV_CALL|CV_NOINIT, Color_cons_t, SendNameAndColor, 0, NULL, NULL, 0, 0, NULL};
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin = {"skin", DEFAULTSKIN, CV_CALL|CV_NOINIT, NULL, Skin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autoaim = {"autoaim", "Off", CV_CALL|CV_NOINIT, CV_OnOff, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autoaim2 = {"autoaim2", "Off", CV_CALL|CV_NOINIT, CV_OnOff, SendWeaponPref, 0, NULL, NULL, 0, 0, NULL};
// secondary player for splitscreen mode
consvar_t cv_playername2 = {"name2", "Tails", CV_CALL|CV_NOINIT, NULL, SendNameAndColor2, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playercolor2 = {"color2", "Orange", CV_CALL|CV_NOINIT, Color_cons_t, SendNameAndColor2, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_skin2 = {"skin2", "Tails", CV_CALL|CV_NOINIT, NULL, Skin2_OnChange, 0, NULL, NULL, 0, 0, NULL};

boolean cv_debug;

consvar_t cv_usemouse = {"use_mouse", "On", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usemouse2 = {"use_mouse2", "Off", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse2, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_usejoystick = {"use_joystick", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#if (defined(LJOYSTICK) || defined(SDL))
#ifdef LJOYSTICK
consvar_t cv_joyport = {"joyport", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyport2 = {"joyport2", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: for later
#endif
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale2, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
#endif
#ifdef LMOUSE2
consvar_t cv_mouse2port = {"mouse2port", "/dev/gpmdata", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mouse2opt = {"mouse2opt", "0", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_mouse2port = {"mouse2port", "COM2", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
consvar_t cv_matchboxes = {"matchboxes", "Normal", CV_NETVAR, matchboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_specialrings = {"specialrings", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_chaos_bluecrawla = {"chaos_bluecrawla", "High", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_redcrawla = {"chaos_redcrawla", "High", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_crawlacommander = {"chaos_crawlacommander", "Low", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_jettysynbomber = {"chaos_jettysynbomber", "Medium", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_jettysyngunner = {"chaos_jettysyngunner", "Low", CV_NETVAR, chances_cons_t,
	NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_eggmobile1 = {"chaos_eggmobile1", "Low", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_eggmobile2 = {"chaos_eggmobile2", "Low", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_skim = {"chaos_skim", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chaos_spawnrate = {"chaos_spawnrate", "30",CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_teleporters = {"teleporters", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_superring = {"superring", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_silverring = {"silverring", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_supersneakers = {"supersneakers", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invincibility = {"invincibility", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_jumpshield = {"jumpshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_watershield = {"watershield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ringshield = {"ringshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_fireshield = {"fireshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bombshield = {"bombshield", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_1up = {"1up", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_eggmanbox = {"eggmantv", "Medium", CV_NETVAR, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Question boxes aren't spawned by randomly respawning monitors, so there is no need
// for chances. Rather, a simple on/off is used.
consvar_t cv_questionbox = {"randomtv", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_playerspeed = {"playerspeed", "1", CV_NETVAR|CV_FLOAT|CV_NOSHOWHELP|CV_CALL, 0,
	Playerspeed_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ringslinger = {"ringslinger", "No", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, CV_YesNo,
	Ringslinger_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startrings = {"startrings", "0", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, ringlimit_cons_t,
	Startrings_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startlives = {"startlives", "0", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, liveslimit_cons_t,
	Startlives_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_startcontinues = {"startcontinues", "0",CV_NETVAR|CV_NOSHOWHELP|CV_CALL,
	liveslimit_cons_t, Startcontinues_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_gravity = {"gravity", "0.5", CV_NETVAR|CV_FLOAT|CV_CALL, NULL, Gravity_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_countdowntime = {"countdowntime", "60", CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_teamplay = {"teamplay", "Off", CV_NETVAR|CV_CALL, teamplay_cons_t, TeamPlay_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_teamdamage = {"teamdamage", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_timetic = {"timetic", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; // use tics in display
consvar_t cv_objectplace = {"objectplace", "Off", CV_CALL|CV_JOHNNY, CV_OnOff,
	ObjectPlace_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_snapto = {"snapto", "Off", CV_JOHNNY, snapto_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_speed = {"speed", "1", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_objflags = {"objflags", "7", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mapthingnum = {"mapthingnum", "0", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_grid = {"grid", "0", CV_JOHNNY, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

// Scoring type options
consvar_t cv_match_scoring = {"match_scoring", "Normal", CV_NETVAR, match_scoring_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_realnames = {"realnames", "Off", CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_pointlimit = {"pointlimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, CV_Unsigned,
	PointLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_timelimit = {"timelimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, CV_Unsigned,
	TimeLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_forceskin = {"forceskin", "No", CV_NETVAR|CV_CALL, CV_YesNo, ForceSkin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_nodownloading = {"nodownloading", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowexitlevel = {"allowexitlevel", "No", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_killingdead = {"killingdead", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_netstat = {"netstat", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; // show bandwidth statistics

// Intermission time Tails 04-19-2002
consvar_t cv_inttime = {"inttime", "15", CV_NETVAR, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_advancemap = {"advancemap", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_runscripts = {"runscripts", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_friendlyfire = {"friendlyfire", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_pause = {"pausepermission", "Server", CV_NETVAR, pause_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mute = {"mute", "Off", CV_NETVAR|CV_CALL, CV_OnOff, Mute_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_sleep = {"cpusleep", "-1", CV_SAVE, sleeping_cons_t, NULL, -1, NULL, NULL, 0, 0, NULL};

static int forcedskin = 0;
int gametype = GT_COOP;
int adminplayer = -1;

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndcolor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_PAUSE, Got_Pause);

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f);
	RegisterNetXCmd(XD_LOGIN, Got_Login);
	COM_AddCommand("login", Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("verify", Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", Command_Clearscores_f);
	COM_AddCommand("map", Command_Map_f);

	COM_AddCommand("exitgame", Command_ExitGame_f);
	COM_AddCommand("exitlevel", Command_ExitLevel_f);

	COM_AddCommand("addfile", Command_Addfile);
	COM_AddCommand("pause", Command_Pause);

	COM_AddCommand("showgametype", Command_ShowGametype_f);
	COM_AddCommand("version", Command_Version_f);
	COM_AddCommand("quit", Command_Quit_f);

	COM_AddCommand("saveconfig", Command_SaveConfig_f);
	COM_AddCommand("loadconfig", Command_LoadConfig_f);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f);

	COM_AddCommand("nodes", Command_Nodes_f);
	COM_AddCommand("isgamemodified", Command_Isgamemodified_f); // test

	// dedicated only, for setting what skin is forced when there isn't a server player
	if(dedicated)
		COM_AddCommand("setforcedskin", Command_SetForcedSkin_f);

	// for master server connection
	AddMServCommands();

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);
	CV_RegisterVar(&cv_flagtime);
	CV_RegisterVar(&cv_suddendeath);

	// misc
	CV_RegisterVar(&cv_teamplay);
	CV_RegisterVar(&cv_teamdamage);
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_timetic);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playdemospeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_nodownloading);

	CV_RegisterVar(&cv_specialrings);
	CV_RegisterVar(&cv_racetype);
	CV_RegisterVar(&cv_raceitemboxes);
	CV_RegisterVar(&cv_matchboxes);
	CV_RegisterVar(&cv_chaos_bluecrawla);
	CV_RegisterVar(&cv_chaos_redcrawla);
	CV_RegisterVar(&cv_chaos_crawlacommander);
	CV_RegisterVar(&cv_chaos_jettysynbomber);
	CV_RegisterVar(&cv_chaos_jettysyngunner);
	CV_RegisterVar(&cv_chaos_eggmobile1);
	CV_RegisterVar(&cv_chaos_eggmobile2);
	CV_RegisterVar(&cv_chaos_skim);
	CV_RegisterVar(&cv_chaos_spawnrate);

	CV_RegisterVar(&cv_teleporters);
	CV_RegisterVar(&cv_superring);
	CV_RegisterVar(&cv_silverring);
	CV_RegisterVar(&cv_supersneakers);
	CV_RegisterVar(&cv_invincibility);
	CV_RegisterVar(&cv_jumpshield);
	CV_RegisterVar(&cv_watershield);
	CV_RegisterVar(&cv_ringshield);
	CV_RegisterVar(&cv_fireshield);
	CV_RegisterVar(&cv_bombshield);
	CV_RegisterVar(&cv_1up);
	CV_RegisterVar(&cv_eggmanbox);

	CV_RegisterVar(&cv_playerspeed);
	CV_RegisterVar(&cv_ringslinger);
	CV_RegisterVar(&cv_startrings);
	CV_RegisterVar(&cv_startlives);
	CV_RegisterVar(&cv_startcontinues);

	CV_RegisterVar(&cv_countdowntime);
	CV_RegisterVar(&cv_runscripts);
	CV_RegisterVar(&cv_match_scoring);
	CV_RegisterVar(&cv_friendlyfire);
	CV_RegisterVar(&cv_pause);
	CV_RegisterVar(&cv_mute);

	COM_AddCommand("load", Command_Load_f);
	RegisterNetXCmd(XD_LOADGAME, Got_LoadGamecmd);
	COM_AddCommand("save", Command_Save_f);
	RegisterNetXCmd(XD_SAVEGAME, Got_SaveGamecmd);
	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	RegisterNetXCmd(XD_ORDERPIZZA, Got_PizzaOrder);

	CV_RegisterVar(&cv_allowexitlevel);
	CV_RegisterVar(&cv_allowautoaim);
	CV_RegisterVar(&cv_allowteamchange);
	CV_RegisterVar(&cv_killingdead);

	// d_clisrv
	CV_RegisterVar(&cv_maxplayers);
	CV_RegisterVar(&cv_maxsend);

	// In-game thing placing stuff
	CV_RegisterVar(&cv_objectplace);
	CV_RegisterVar(&cv_snapto);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_objflags);
	CV_RegisterVar(&cv_mapthingnum);
	CV_RegisterVar(&cv_grid);

	CV_RegisterVar(&cv_sleep);

	CV_RegisterVar(&cv_dummyconsvar);
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	int i;

	for(i = 0; i < MAXSKINCOLORS; i++)
		Color_cons_t[i].strvalue = Color_Names[i];

	if(dedicated)
		return;

	COM_AddCommand("changeteam", Command_Teamchange_f);
	COM_AddCommand("changeteam2", Command_Teamchange2_f);

	COM_AddCommand("playdemo", Command_Playdemo_f);
	COM_AddCommand("timedemo", Command_Timedemo_f);
	COM_AddCommand("stopdemo", Command_Stopdemo_f);
	COM_AddCommand("teleport", Command_Teleport_f);
	COM_AddCommand("playintro", Command_Playintro_f);
	COM_AddCommand("writethings", Command_Writethings_f);

	COM_AddCommand("orderpizza", Command_OrderPizza_f);

	COM_AddCommand("chatmacro", Command_Chatmacro_f); // hu_stuff.c
	COM_AddCommand("setcontrol", Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f);

	COM_AddCommand("screenshot", M_ScreenShot);

	CV_RegisterVar(&cv_splats);

	// register these so it is saved to config
	cv_playername.defaultvalue = I_GetUserName();
	if(!cv_playername.defaultvalue)
		cv_playername.defaultvalue = "Player";
	CV_RegisterVar(&cv_playername);
	CV_RegisterVar(&cv_playercolor);

	CV_RegisterVar(&cv_realnames);
	CV_RegisterVar(&cv_netstat);

#ifdef FISHCAKE
	CV_RegisterVar(&cv_fishcake);
#endif

	COM_AddCommand("displayplayer", Command_Displayplayer_f);
	COM_AddCommand("tunes",Command_Tunes_f);

	// r_things.c (skin NAME)
	CV_RegisterVar(&cv_skin);
	// secondary player (splitscreen)
	CV_RegisterVar(&cv_skin2);
	CV_RegisterVar(&cv_playername2);
	CV_RegisterVar(&cv_playercolor2);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_usegamma);

	// m_menu.c
	CV_RegisterVar(&cv_crosshair);
	CV_RegisterVar(&cv_invertmouse);
	CV_RegisterVar(&cv_alwaysfreelook);
	CV_RegisterVar(&cv_mousemove);
	CV_RegisterVar(&cv_showmessages);

	// see m_menu.c
	CV_RegisterVar(&cv_crosshair2);
	CV_RegisterVar(&cv_autoaim);
	CV_RegisterVar(&cv_autoaim2);

	// g_input.c
	CV_RegisterVar(&cv_usemouse2);
	CV_RegisterVar(&cv_invertmouse2);
	CV_RegisterVar(&cv_alwaysfreelook2);
	CV_RegisterVar(&cv_mousemove2);
	CV_RegisterVar(&cv_mousesens2);
	CV_RegisterVar(&cv_mlooksens2);
	CV_RegisterVar(&cv_sideaxis);
	CV_RegisterVar(&cv_turnaxis);
	CV_RegisterVar(&cv_moveaxis);
	CV_RegisterVar(&cv_lookaxis);
	CV_RegisterVar(&cv_sideaxis2);
	CV_RegisterVar(&cv_turnaxis2);
	CV_RegisterVar(&cv_moveaxis2);
	CV_RegisterVar(&cv_lookaxis2);

	// WARNING: the order is important when initialising mouse2
	// we need the mouse2port
	CV_RegisterVar(&cv_mouse2port);
#ifdef LMOUSE2
	CV_RegisterVar(&cv_mouse2opt);
#endif
	CV_RegisterVar(&cv_mousesens);
	CV_RegisterVar(&cv_mlooksens);
	CV_RegisterVar(&cv_controlperkey);

	CV_RegisterVar(&cv_usemouse);
	CV_RegisterVar(&cv_usejoystick);
	CV_RegisterVar(&cv_usejoystick2);
#ifdef LJOYSTICK
	CV_RegisterVar(&cv_joyport);
	CV_RegisterVar(&cv_joyport2);
#endif
	CV_RegisterVar(&cv_joyscale);
	CV_RegisterVar(&cv_joyscale2);

	// Analog Control
	CV_RegisterVar(&cv_analog);
	CV_RegisterVar(&cv_analog2);
	CV_RegisterVar(&cv_useranalog);
	CV_RegisterVar(&cv_useranalog2);

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_midimusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// i_cdmus.c
	CV_RegisterVar(&cd_volume);
	CV_RegisterVar(&cdUpdate);
#if defined(LINUX) && !defined(SDL)
	CV_RegisterVar(&cv_jigglecdvol);
#endif

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);

	// p_fab.c
	CV_RegisterVar(&cv_translucency);

	// add cheat commands
	COM_AddCommand("noclip", Command_CheatNoClip_f);
	COM_AddCommand("god", Command_CheatGod_f);
	COM_AddCommand("resetemeralds", Command_Resetemeralds_f);
	COM_AddCommand("devmode", Command_Devmode_f);
}

/** Sets a player's name or doesn't.
  * The process consists of removing leading or trailing spaces, then comparing
  * the new name case-insensitively to the empty string and the names of all
  * other players in the game. If there is no match, the new name with leading
  * and trailing spaces removed is copied to the player name table.
  *
  * We assume that if playernum is ::consoleplayer or ::secondarydisplayplayer
  * the console variable ::cv_playername or ::cv_playername2 respectively is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * If the name change succeeds, a message indicating such is printed to the
  * console.
  *
  * If playernum is ::consoleplayer or ::secondarydisplayplayer, we
  * unconditionally set the respective console variable to the final value in
  * the name table to ensure the name is given equivalently in both places.
  * Bear in mind that even a successful name change may have involved stripping
  * of spaces.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername or ::cv_playername2 if player is the
  *                  console or secondary display player, respectively.
  * \sa cv_playername, cv_playername2, SendNameAndColor, SendNameAndColor2
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(int playernum, char* newname)
{
	char* p;
	char* tmpname = NULL;
	char buf[MAXPLAYERNAME];
	int i;
	boolean namefailed = true;

	do
	{
		strcpy(buf, newname);

		p = newname;

		while(*p == ' ')
			p++; // remove leading spaces

		if(!strlen(p))
			break; // empty names not allowed

		tmpname = p;
		p = &tmpname[strlen(tmpname)-1]; // last character

		while(*p == ' ')
		{
			*p = '\0';
			p--; // remove trailing spaces
		}

		if(!strlen(tmpname))
			break; // another case of an empty name

		// no stealing another player's name
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i] && !strcasecmp(tmpname, player_names[i]))
				break;

		if(i < MAXPLAYERS)
			break;

		// name is okay then
		namefailed = false;
	} while(0);

	if(!namefailed)
	{
		if(gamestate != GS_INTRO && gamestate != GS_INTRO2)
			CONS_Printf("%s renamed to %s\n",player_names[playernum], tmpname);
		strcpy(player_names[playernum], tmpname);
	}

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if(playernum == consoleplayer)
		CV_StealthSet(&cv_playername, player_names[playernum]);
	else if(playernum == secondarydisplayplayer)
		CV_StealthSet(&cv_playername2, player_names[playernum]);
}

static int snacpending = 0, snac2pending = 0, chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(void)
{
	char buf[MAXPLAYERNAME+1+SKINNAMESIZE+1];
	char* p;
	byte extrainfo = 0; // color and (if applicable) CTF team

	if(netgame && !addedtogame)
		return;

	p = buf;

	// normal player colors in single player
	if(!multiplayer && !netgame && gamestate != GS_INTRO && gamestate != GS_INTRO2)
		if(cv_playercolor.value != players[consoleplayer].prefcolor)
			CV_StealthSetValue(&cv_playercolor, players[consoleplayer].prefcolor);

	// normal player colors in CTF
	if(gametype == GT_CTF)
	{
		if(players[consoleplayer].ctfteam == 1 && cv_playercolor.value != 6)
			CV_StealthSetValue(&cv_playercolor, 6);
		else if(players[consoleplayer].ctfteam == 2 && cv_playercolor.value != 7)
			CV_StealthSetValue(&cv_playercolor, 7);
		else if(!players[consoleplayer].ctfteam && cv_playercolor.value != 1)
			CV_StealthSetValue(&cv_playercolor, 1);
	}

	extrainfo = (byte)(extrainfo + (byte)cv_playercolor.value);

	// if you're not in a netgame, merely update the skin, color, and name as appropriate
	if(!netgame)
	{
		players[consoleplayer].skincolor = (cv_playercolor.value&31) % MAXSKINCOLORS;
		if(players[consoleplayer].mo)
			players[consoleplayer].mo->flags =
				(players[consoleplayer].mo->flags & ~MF_TRANSLATION)
				| ((players[consoleplayer].skincolor) << MF_TRANSSHIFT);

		SetPlayerName(0, cv_playername.zstring);
		cv_skin.value = R_SkinAvailable(cv_skin.string);
		SetPlayerSkin(consoleplayer, cv_skin.string);
		CV_StealthSet(&cv_skin, skins[cv_skin.value].name);
		return;
	}

	snacpending++;

	WRITEBYTE(p, extrainfo);

	WRITESTRINGN(p, cv_playername.string, MAXPLAYERNAME);
	*(p-1) = 0; // finish the string

	// Don't change skin if the server doesn't want you to.
	if(!server && cv_forceskin.value && !(admin && serverplayer == -1))
	{
		SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
		return;
	}

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin.value = R_SkinAvailable(cv_skin.string);
	if(!cv_skin.value)
		WRITESTRINGN(p, DEFAULTSKIN, SKINNAMESIZE)
	else
		WRITESTRINGN(p, cv_skin.string, SKINNAMESIZE);
	*(p-1) = 0; // finish the string

	SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
}

// splitscreen
static void SendNameAndColor2(void)
{
	char buf[MAXPLAYERNAME+1+SKINNAMESIZE+1];
	char* p;
	int secondplaya;
	byte extrainfo = 0;

	if(!cv_splitscreen.value)
		return; // can happen if skin2/color2/name2 changed

	if(netgame)
		secondplaya = secondarydisplayplayer;
	else
		secondplaya = 1;

	p = buf;

	if(gametype == GT_CTF)
	{
		if(players[secondplaya].ctfteam == 1 && cv_playercolor2.value != 6)
			CV_StealthSetValue(&cv_playercolor2, 6);
		else if(players[secondplaya].ctfteam == 2 && cv_playercolor2.value != 7)
			CV_StealthSetValue(&cv_playercolor2, 7);
		else if(!players[secondarydisplayplayer].ctfteam && cv_playercolor2.value != 1)
			CV_StealthSetValue(&cv_playercolor2, 1);
	}

	extrainfo = (byte)cv_playercolor2.value; // do this after, because the above might've changed it

	// Graue 09-07-2004: before any writing of bytes, etc gets done
	if(!netgame || (server && secondplaya == consoleplayer))
	{
		// don't use secondarydisplayplayer: the second player must be 1
		players[1].skincolor = cv_playercolor2.value;
		if(players[1].mo)
			players[1].mo->flags = (players[1].mo->flags & ~MF_TRANSLATION)	|
			((players[1].skincolor) << MF_TRANSSHIFT);

		SetPlayerName(1, cv_playername2.zstring);
		cv_skin2.value = R_SkinAvailable(cv_skin2.string);
		SetPlayerSkin(1, cv_skin2.string);
		return;
	}
	else if(!addedtogame || secondplaya == consoleplayer)
		return;

	snac2pending++;

	WRITEBYTE(p, extrainfo);

	WRITESTRINGN(p,cv_playername2.string,MAXPLAYERNAME);
	*(p-1) = 0; // finish the string

	// Don't change skin if the server doesn't want you to.
	// Note: Splitscreen player is never serverplayer. No exceptions!
	if(cv_forceskin.value)
	{
		SendNetXCmd2(XD_NAMEANDCOLOR,buf,p - buf);
		return;
	}

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin2.value = R_SkinAvailable(cv_skin2.string);
	if(!cv_skin2.value)
		WRITESTRINGN(p, DEFAULTSKIN, SKINNAMESIZE)
	else
		WRITESTRINGN(p, cv_skin2.string, SKINNAMESIZE);
	*(p-1) = 0; // finish the string

	SendNetXCmd2(XD_NAMEANDCOLOR, buf, p - buf);
}

static void Got_NameAndcolor(char** cp, int playernum)
{
	player_t* p = &players[playernum];
	int i;
	byte extrainfo;

#ifdef PARANOIA
	if(playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	if(playernum == consoleplayer)
		snacpending--;
	else if(playernum == secondarydisplayplayer)
		snac2pending--;

#ifdef PARANOIA
	if(snacpending < 0 || snac2pending < 0)
		I_Error("snacpending negative!");
#endif

	extrainfo = READBYTE(*cp);

	if(playernum == consoleplayer && ((extrainfo&31) % MAXSKINCOLORS) != cv_playercolor.value
		&& !snacpending && !chmappending)
	{
		I_Error("consoleplayer color received as %d, cv_playercolor.value is %d",
			(extrainfo&31) % MAXSKINCOLORS, cv_playercolor.value);
	}
	if(cv_splitscreen.value && playernum == secondarydisplayplayer
		&& ((extrainfo&31) % MAXSKINCOLORS) != cv_playercolor2.value && !snac2pending
		&& !chmappending)
	{
		I_Error("secondarydisplayplayer color received as %d, cv_playercolor2.value is %d",
			(extrainfo&31) % MAXSKINCOLORS, cv_playercolor2.value);
	}
	// color
	p->skincolor = (extrainfo&31) % MAXSKINCOLORS;

	// a copy of color
	if(p->mo)
		p->mo->flags = (p->mo->flags & ~MF_TRANSLATION) | ((p->skincolor)<<MF_TRANSSHIFT);

	// name
	if(strcasecmp(player_names[playernum], *cp))
		SetPlayerName(playernum, *cp);
	SKIPSTRING(*cp);

	// moving players cannot change skins
	if(cp && players[playernum].mo && (
		players[playernum].rmomx >= FRACUNIT/2 ||
		players[playernum].rmomx <= -FRACUNIT/2 ||
		players[playernum].rmomy >= FRACUNIT/2 ||
		players[playernum].rmomy <= -FRACUNIT/2 ||
		players[playernum].mo->momz >= FRACUNIT/2 ||
		players[playernum].mo->momz <= -FRACUNIT/2 ||
		players[playernum].powers[pw_tailsfly] ||
		players[playernum].mfjumped))
	{
		// if the skin is the same as before it's okay, though
		// (as during everyone's SendNameAndColor when a new player joins)
		if(strcasecmp(skins[players[playernum].skin].name, *cp))
		{
			SKIPSTRING(*cp);
			if(playernum == consoleplayer)
				CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
			else if(cv_splitscreen.value && playernum == secondarydisplayplayer)
				CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
			return;
		}
	}

	// skin
	if(cv_forceskin.value) // Server wants everyone to use the same player
	{
		if(playernum == serverplayer)
		{
			// serverplayer should be 0 in this case
#ifdef PARANOIA
			if(serverplayer)
				I_Error("serverplayer is %d, not zero!", serverplayer);
#endif

			SetPlayerSkin(0, *cp);
			forcedskin = players[0].skin;

			for(i = 1; i < MAXPLAYERS; i++)
			{
				if(playeringame[i])
				{
					SetPlayerSkinByNum(i, forcedskin);

					// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
					if(i == consoleplayer)
						CV_StealthSet(&cv_skin, skins[forcedskin].name);
					else if(i == secondarydisplayplayer)
						CV_StealthSet(&cv_skin2, skins[forcedskin].name);
				}
			}
		}
		else if(serverplayer == -1 && playernum == adminplayer)
		{
			// in this case the adminplayer's skin is used
			SetPlayerSkin(adminplayer, *cp);

			for(i = 0; i < MAXPLAYERS; i++)
			{
				if(i != adminplayer && playeringame[i])
				{
					SetPlayerSkinByNum(i, forcedskin);
					// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
					if(i == consoleplayer)
						CV_StealthSet(&cv_skin, skins[forcedskin].name);
					else if(i == secondarydisplayplayer)
						CV_StealthSet(&cv_skin2, skins[forcedskin].name);
				}
			}
		}
		else
			SetPlayerSkinByNum(playernum, forcedskin);

		SKIPSTRING(*cp);
	}
	else
	{
		SetPlayerSkin(playernum,*cp);
		SKIPSTRING(*cp);
	}
}

static void SendWeaponPref(void)
{
	char buf[1];

	buf[0] = (char)cv_autoaim.value;
	SendNetXCmd(XD_WEAPONPREF, buf, 1);

	if(cv_splitscreen.value)
	{
		buf[0] = (char)cv_autoaim2.value;
		SendNetXCmd2(XD_WEAPONPREF, buf, 1);
	}
}

static void Got_WeaponPref(char** cp,int playernum)
{
	players[playernum].autoaim_toggle = *(*cp)++;
}

void D_SendPlayerConfig(void)
{
	SendNameAndColor();
	if(cv_splitscreen.value)
		SendNameAndColor2();
	SendWeaponPref();
}

static void Command_OrderPizza_f(void)
{
	if(COM_Argc() < 6 || COM_Argc() > 7)
	{
		CONS_Printf("orderpizza -size <value> -address <value> -toppings <value> : order a pizza!\n");
		return;
	}

	SendNetXCmd(XD_ORDERPIZZA, 0, 0);
}

static void Got_PizzaOrder(char** cp, int playernum)
{
	cp = NULL;
	CONS_Printf("%s has ordered a pizza.\n", player_names[playernum]);
}

static void Command_Teleport_f(void)
{
	int intx, inty, intz;
	size_t i;
	player_t* p = &players[consoleplayer];
	subsector_t* ss;

	if(!(cv_debug || devparm))
		return;

	if(COM_Argc() < 3 || COM_Argc() > 7)
	{
		CONS_Printf("teleport -x <value> -y <value> : teleport to a location\n");
		return;
	}

	if(netgame)
	{
		CONS_Printf("You can't teleport while in a netgame!\n");
		return;
	}

	i = COM_CheckParm("-x");
	if(i)
		intx = atoi(COM_Argv(i + 1));
	else
	{
		CONS_Printf("X value not specified\n");
		return;
	}

	i = COM_CheckParm("-y");
	if(i)
		inty = atoi(COM_Argv(i + 1));
	else
	{
		CONS_Printf("Y value not specified\n");
		return;
	}

	ss = R_IsPointInSubsector(intx, inty);
	if(!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
	{
		CONS_Printf("Not a valid location\n");
		return;
	}
	i = COM_CheckParm("-z");
	if(i)
	{
		intz = atoi(COM_Argv(i + 1));
		intz <<= FRACBITS;
		if(intz < ss->sector->floorheight)
			intz = ss->sector->floorheight;
		if(intz > ss->sector->ceilingheight - p->mo->height)
			intz = ss->sector->ceilingheight - p->mo->height;
	}
	else
		intz = ss->sector->floorheight;

	CONS_Printf("Teleporting to %d, %d, %d...", intx, inty, intz>>FRACBITS);

	if(!P_TeleportMove(p->mo, intx*FRACUNIT, inty*FRACUNIT, intz))
		CONS_Printf("Unable to teleport to that spot!\n");
	else
		S_StartSound(p->mo, sfx_mixup);
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// byte* demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	char name[256];

	if(COM_Argc() != 2)
	{
		CONS_Printf("playdemo <demoname> : playback a demo\n");
		return;
	}

	// disconnect from server here?
	if(demoplayback)
		G_StopDemo();
	if(netgame)
	{
		CONS_Printf("\nYou can't play a demo while in net game\n");
		return;
	}

	// open the demo file
	strcpy(name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf("Playing back demo '%s'.\n", name);

	G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	char name[256];

	if(COM_Argc() != 2)
	{
		CONS_Printf("timedemo <demoname> : time a demo\n");
		return;
	}

	// disconnect from server here?
	if(demoplayback)
		G_StopDemo();
	if(netgame)
	{
		CONS_Printf("\nYou can't play a demo while in net game\n");
		return;
	}

	// open the demo file
	strcpy (name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf("Timing demo '%s'.\n", name);

	G_TimeDemo(name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf("Stopped demo.\n");
}

int mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum          Map number to change to.
  * \param gametype        Gametype to switch to.
  * \param skill           Skill level to use.
  * \param resetplayers    1 to reset player scores and lives and such, 0 not to.
  * \param delay           Determines how the function will be executed: 0 to do
  *                        it all right now (must not be done from a menu), 1 to
  *                        do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene To skip the precutscence or not?
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(int mapnum, int gametype, skill_t skill, int resetplayers, int delay, boolean skipprecutscene)
{
	static char buf[MAX_WADPATH+4];
#define MAPNAME &buf[4]

	if(devparm)
		CONS_Printf("Map change: mapnum=%d gametype=%d skill=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
			mapnum, gametype, skill, resetplayers, delay, skipprecutscene);
	if(delay != 2)
	{
		strncpy(MAPNAME, G_BuildMapName(mapnum), MAX_WADPATH);

		buf[0] = (char)skill;

		// bit 0 doesn't currently do anything
		buf[1] = 0;

		if(!resetplayers)
			buf[1] |= 2;

		// new gametype value
		buf[2] = (char)gametype;
	}

	if(delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if(!admin && SV_SpawnServer())
			buf[1] &= ~2;

		chmappending++;
		if(server && netgame)
		{
			byte seed = (byte)(totalplaytime % 256);
			SendNetXCmd(XD_RANDOMSEED, &seed, 1);
		}

		buf[3] = (char)skipprecutscene;

		SendNetXCmd(XD_MAP, buf, 4+strlen(MAPNAME)+1);
	}
#undef MAPNAME
}

// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
static void Command_Map_f(void)
{
	const char* mapname;
	size_t i;
	int j, newmapnum, newskill, newgametype, newresetplayers;

	// max length of command: map map03 -gametype coop -skill 3 -noresetplayers -force
	//                         1    2       3       4     5   6        7           8
	// = 8 arg max
	if(COM_Argc() < 2 || COM_Argc() > 8)
	{
		CONS_Printf("map <mapname> [-gametype <type>] [-skill <%d..%d>] [-noresetplayers] [-force]: warp to map\n",
			sk_easy, sk_insane);
		return;
	}

	if(!server && !admin)
	{
		CONS_Printf("Only the server can change the map\n");
		return;
	}

	// internal wad lump always: map command doesn't support external files as in doom legacy
	if(W_CheckNumForName(COM_Argv(1)) == -1)
	{
		CONS_Printf("\2Internal game map '%s' not found\n", COM_Argv(1));
		return;
	}

	if(!(netgame || multiplayer) && (!modifiedgame || savemoddata))
	{
		CONS_Printf("Sorry, map change disabled in single player.\n");
		return;
	}

	newresetplayers = !COM_CheckParm("-noresetplayers");

	mapname = COM_Argv(1);
	if(strlen(mapname) != 5 || mapname[0] != 'm' || mapname[1] != 'a' || mapname[2] != 'p'
	|| (newmapnum = M_MapNumber(mapname[3], mapname[4])) == 0)
	{
		CONS_Printf("Invalid map name %s\n", mapname);
		return;
	}
	// new skill value
	newskill = gameskill; // use current one by default
	i = COM_CheckParm("-skill");
	if(i)
	{
		for(j = 0; skill_cons_t[j].strvalue; j++)
			if(!strcasecmp(skill_cons_t[j].strvalue, COM_Argv(i+1)))
			{
				newskill = j + 1; // skill_cons_t starts with "Easy" which is 1
				break;
			}
		if(!skill_cons_t[j].strvalue) // reached end of the list with no match
		{
			if(veryhardcleared && !strcasecmp("Ultimate", COM_Argv(i+1)))
				newskill = sk_insane;
			else
			{
				j = atoi(COM_Argv(i+1));
				if((j >= sk_easy && j <= sk_nightmare)
					|| (veryhardcleared && j == sk_insane))
				{
					newskill = j;
				}
			}
		}
	}

	// new gametype value
	newgametype = gametype; // use current one by default
	i = COM_CheckParm("-gametype");
	if(i)
	{
		if(!multiplayer)
		{
			CONS_Printf("You can't switch gametypes in single player!\n");
			return;
		}

		for(j = 0; gametype_cons_t[j].strvalue; j++)
			if(!strcasecmp(gametype_cons_t[j].strvalue, COM_Argv(i+1)))
			{
				newgametype = gametype_cons_t[j].value;
				break;
			}
		if(!gametype_cons_t[j].strvalue) // reached end of the list with no match
		{
			// assume they gave us a gametype number, which is okay too
			for(j = 0; gametype_cons_t[j].strvalue != NULL; j++)
			{
				if(atoi(COM_Argv(i+1)) == gametype_cons_t[j].value)
				{
					newgametype = gametype_cons_t[j].value;
					break;
				}
			}
		}
	}

	// don't use a gametype the map doesn't support
	if(cv_debug || COM_CheckParm("-force"))
		;
	else if(multiplayer)
	{
		short tol = mapheaderinfo[newmapnum-1].typeoflevel, tolflag = 0;

		switch(newgametype)
		{
			case GT_MATCH: case 42: tolflag = TOL_MATCH; break;
			case GT_CHAOS: tolflag = TOL_CHAOS; break;
			case GT_COOP: tolflag = TOL_COOP; break;
			case GT_RACE: case 43: tolflag = TOL_RACE; break;
			case GT_CTF: tolflag = TOL_CTF; break;
			case GT_TAG: tolflag = TOL_TAG; break;
		}

		if(!(tol & tolflag))
		{
			int i;
			char gametypestring[32];

			for(i=0; gametype_cons_t[i].strvalue != NULL; i++)
			{
				if(gametype_cons_t[i].value == newgametype)
				{
					strcpy(gametypestring, gametype_cons_t[i].strvalue);
					break;
				}
			}

			CONS_Printf("That level doesn't support %s mode!\n(Use -force to override)\n",
				gametypestring);
			return;
		}
	}
	else if(!(mapheaderinfo[newmapnum-1].typeoflevel & TOL_SP))
	{
		CONS_Printf("That level doesn't support Single Player mode!\n");
		return;
	}

	D_MapChange(newmapnum, newgametype, newskill, newresetplayers, 0, false);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \todo At current, it is possible for a hacked server to send a map name
  *       with an extension, causing the local client to try to load an
  *       external wad -- fix this; the functionality is not needed.
  * \sa D_MapChange
  */
static void Got_Mapcmd(char** cp, int playernum)
{
	char mapname[MAX_WADPATH];
	int skill, resetplayer = 1, lastgametype;
	boolean skipprecutscene;

	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal map change received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	if(chmappending)
		chmappending--;
	skill = READBYTE(*cp);

	resetplayer = ((READBYTE(*cp) & 2) == 0);

	lastgametype = gametype;
	gametype = READBYTE(*cp);

	// Special Cases
	if(gametype == 42)
	{
		gametype = GT_MATCH;
		
		if(server)
			CV_SetValue(&cv_teamplay, 1);
	}
	else if(gametype == 43)
	{
		gametype = GT_RACE;

		if(server)
			CV_SetValue(&cv_racetype, 1);
	}

	if(gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	skipprecutscene = READBYTE(*cp);

	strcpy(mapname, *cp);
	*cp += strlen(mapname) + 1;

	if(!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [skill=%d resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			mapname, skill, resetplayer, lastgametype, gametype, chmappending));
		CONS_Printf("Warping to %s...\n", devparm?mapname:"level");
	}
	if(demoplayback && !timingdemo)
		precache = false;
	G_InitNew(skill, mapname, resetplayer, skipprecutscene);
	if(demoplayback && !timingdemo)
		precache = true;
	CON_ToggleOff();
	if(timingdemo)
		G_DoneLevelLoad();
}

static void Command_Pause(void)
{
	char buf;
	if(COM_Argc() > 1)
		buf = (char)(atoi(COM_Argv(1)) != 0);
	else
		buf = (char)(!paused);

	if(cv_pause.value || server || admin)
		SendNetXCmd(XD_PAUSE, &buf, 1);
	else
		CONS_Printf("Only the server can pause the game.\n");
}

static void Got_Pause(char** cp, int playernum)
{
	if(netgame && !cv_pause.value && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal pause command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	paused = READBYTE(*cp);

	if(!demoplayback)
	{
		if(netgame)
		{
			if(paused)
				CONS_Printf("Game paused by %s\n",player_names[playernum]);
			else
				CONS_Printf("Game unpaused by %s\n",player_names[playernum]);
		}

		if(paused)
		{
			if(!menuactive || netgame)
				S_PauseSound();
		}
		else
			S_ResumeSound();
	}
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(char** cp, int playernum)
{
	byte seed;

	seed = READBYTE(*cp);
	if(playernum != serverplayer) // it's not from the server, wtf?
		return;

	P_SetRandIndex(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if(!(server || admin))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(char** cp, int playernum)
{
	int i;

	cp = NULL;
	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal clear scores command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	for(i=0; i<MAXPLAYERS; i++)
		players[i].score = 0;

	CONS_Printf("Scores have been reset by the server.\n");
}

static void Command_Teamchange_f(void)
{
	char buf = 0;

	// Graue 06-19-2004: the last thing we need is more numbers to remember
	if(COM_Argc() < 2)
		buf = 0;
	else if(!strcasecmp(COM_Argv(1), "red"))
		buf = 1;
	else if(!strcasecmp(COM_Argv(1), "blue"))
		buf = 2;

	if(!(buf == 1 || buf == 2))
	{
		CONS_Printf("teamchange <color> : switch to a new team (red or blue)"); // Graue 06-19-2004
		return;
	}

	if(gametype != GT_CTF)
	{
		CONS_Printf("This command is only for capture the flag games.\n");
		return;
	}

	if(buf == players[consoleplayer].ctfteam)
	{
		CONS_Printf("You're already on that team!\n");
		return;
	}

	if(!cv_allowteamchange.value && players[consoleplayer].ctfteam)
	{
		CONS_Printf("Server does not allow team change.\n");
		return;
	}

	SendNetXCmd(XD_TEAMCHANGE, &buf, 1);
}

static void Command_Teamchange2_f(void)
{
	char buf = 0;

	// Graue 06-19-2004: the last thing we need is more numbers to remember
	if(COM_Argc() < 2)
		buf = 0;
	else if(!strcasecmp(COM_Argv(1), "red"))
		buf = 1;
	else if(!strcasecmp(COM_Argv(1), "blue"))
		buf = 2;

	if(!(buf == 1 || buf == 2))
	{
		CONS_Printf("teamchange2 <color> : switch to a new team (red or blue)"); // Graue 06-19-2004
		return;
	}

	if(gametype != GT_CTF)
	{
		CONS_Printf("This command is only for capture the flag games.\n");
		return;
	}

	if(buf == players[secondarydisplayplayer].ctfteam)
	{
		CONS_Printf("You're already on that team!\n");
		return;
	}

	if(!cv_allowteamchange.value && !players[secondarydisplayplayer].ctfteam)
	{
		CONS_Printf("Server does not allow team change.\n");
		return;
	}

	SendNetXCmd2(XD_TEAMCHANGE, &buf, 1);
}

static void Command_ServerTeamChange_f(void)
{
	char buf = 0;
	int playernum;

	if(!(server || admin))
	{
		CONS_Printf("You're not the server. You can't change players' teams.\n");
		return;
	}

	if(COM_Argc() < 2)
		buf = 0;
	else if(!strcasecmp(COM_Argv(2), "red"))
		buf = 1;
	else if(!strcasecmp(COM_Argv(2), "blue"))
		buf = 2;

	if(!buf)
	{
		CONS_Printf ("serverteamchange <playernum> <color> : change a player's team (red or blue)\n");
		return;
	}

	if(gametype != GT_CTF)
	{
		CONS_Printf("This command is only for capture the flag games.\n");
		return;
	}

	playernum = atoi(COM_Argv(1));

	if(buf == players[playernum].ctfteam)
	{
		CONS_Printf("The player is already on that team!\n");
		return;
	}

	buf |= 4; // This signals that it's a server change

	buf = (char)(buf + (char)(playernum << 3));

	SendNetXCmd(XD_TEAMCHANGE,&buf,1);
}

static void Got_Teamchange(char** cp, int playernum)
{
	int newteam;
	newteam = READBYTE(*cp);

	if(gametype != GT_CTF)
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Printf("Illegal team change received from player %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
	}

	// Prevent multiple changes in one go.
	if(players[playernum].ctfteam == newteam)
		return;

	if(newteam & 4) // Special marker that the server sent the request
	{
		if(playernum != serverplayer && (playernum != adminplayer || (newteam&3) == 0 || (newteam&3) == 3))
		{
			CONS_Printf("Illegal team change received from player %s\n", player_names[playernum]);
			if(server)
			{
				char buf[2];

				buf[0] = (char)playernum;
				buf[1] = KICK_MSG_CON_FAIL;
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			return;
		}
		playernum = newteam >> 3;
	}

	newteam &= 3; // We do this either way, since... who cares?
	if(server && (!newteam || newteam == 3))
	{
		char buf[2];

		buf[0] = (char)playernum;
		buf[1] = KICK_MSG_CON_FAIL;
		CONS_Printf("Player %s sent illegal team change to team %d\n",
			player_names[playernum], newteam);
		SendNetXCmd(XD_KICK, &buf, 2);
	}

	if(!players[playernum].ctfteam)
	{
		players[playernum].ctfteam = newteam;

		if(playernum == consoleplayer)
			displayplayer = consoleplayer;
	}

	if(players[playernum].mo) // Safety first!
		P_DamageMobj(players[playernum].mo, NULL, NULL, 10000);

	players[playernum].ctfteam = newteam;

	if(newteam == 1)
		CONS_Printf("%s switched to the red team\n", player_names[playernum]);
	else // newteam == 2
		CONS_Printf("%s switched to the blue team\n", player_names[playernum]);

	if(displayplayer != consoleplayer && players[consoleplayer].ctfteam)
		displayplayer = consoleplayer;

	if(playernum == consoleplayer)
		CV_SetValue(&cv_playercolor, newteam + 5);
	else if(playernum == secondarydisplayplayer)
		CV_SetValue(&cv_playercolor2, newteam + 5);
}

// Remote Administration
static void Command_Changepassword_f(void)
{
	if(!server) // cannot change remotely
	{
		CONS_Printf("You're not the server. You can't change this.\n");
		return;
	}

	if(COM_Argc() != 2)
	{
		CONS_Printf("password <password> : change password\n");
		return;
	}

	strncpy(adminpassword, COM_Argv(1), 8);

	// Pad the password
	if(strlen(COM_Argv(1)) < 8)
	{
		size_t i;
		for(i = strlen(COM_Argv(1)); i < 8; i++)
			adminpassword[i] = 'a';
	}
}

static void Command_Login_f(void)
{
	char password[9];

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.

	if(COM_Argc() != 2)
	{
		CONS_Printf("login <password> : Administrator login\n");
		return;
	}

	strncpy(password, COM_Argv(1), 8);

	// Pad the password
	if(strlen(COM_Argv(1)) < 8)
	{
		size_t i;
		for(i = strlen(COM_Argv(1)); i < 8; i++)
			password[i] = 'a';
	}

	password[8] = '\0';

	CONS_Printf("Sending Login...%s\n(Notice only given if password is correct.)\n", password);

	SendNetXCmd(XD_LOGIN, password, 9);
}

static void Got_Login(char** cp, int playernum)
{
	char compareword[9];

	strcpy(compareword, *cp);

	SKIPSTRING(*cp);

	if(!server)
		return;

	if(!strcmp(compareword, adminpassword))
	{
		CONS_Printf("%s passed authentication. (%s)\n", player_names[playernum], compareword);
		COM_BufInsertText(va("verify %d\n", playernum)); // do this immediately
	}
	else
		CONS_Printf("Password from %i failed (%s)\n", playernum, compareword);
}

static void Command_Verify_f(void)
{
	char buf[8]; // Should be plenty
	char* temp;
	int playernum;

	if(!server)
	{
		CONS_Printf("You're not the server... you can't give out admin privileges!\n");
		return;
	}

	if(COM_Argc() != 2)
	{
		CONS_Printf("verify <node> : give admin privileges to a node\n");
		return;
	}

	strncpy(buf, COM_Argv(1), 7);

	playernum = atoi(buf);

	temp = buf;

	WRITEBYTE(temp, playernum);

	SendNetXCmd(XD_VERIFIED, buf, 1);
}

static void Got_Verification(char** cp, int playernum)
{
	int num = READBYTE(*cp);

	if(playernum != serverplayer) // it's not from the server, ignore (hacker or bug)
	{
		/// \ debfile only?
		CONS_Printf("ERROR: Got invalid verification notice from player %d (serverplayer is %d)\n",
			playernum, serverplayer);
		return;
	}

	adminplayer = num;

	admin = false;

	if(num != consoleplayer)
		return;

	admin = true;
	CONS_Printf("Password correct. You are now an administrator.\n");
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  *
  * \todo This should be made to either not work in netgames (easy), or work in
  *       netgames only for the server and then synchronize the files, making
  *       sure everyone has them, and possibly sending them (hard).
  */
static void Command_Addfile(void)
{
	char buf[255];
	size_t length = 0;

	if(COM_Argc() != 2)
	{
		CONS_Printf("addfile <wadfile.wad> : load wad file\n");
		return;
	}

	if(netgame && !(server || adminplayer))
	{
		CONS_Printf("Sorry, only the server can do this.\n");
		return;
	}

	if(!modifiedgame && !W_VerifyNMUSlumps(COM_Argv(1)))
	{
		modifiedgame = true;
		if(!(netgame || multiplayer))
			CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
	}

	if(!(netgame || multiplayer))
	{
		P_AddWadFile(COM_Argv(1), NULL);
		return;
	}

	strcpy(buf, COM_Argv(1));
	length = strlen(COM_Argv(1))+1;

#ifndef NOMD5
	{
		FILE* fhandle;
		unsigned char md5sum[16];

		fhandle = fopen(COM_Argv(1), "rb");

		if(fhandle != NULL)
		{
			int t = I_GetTime();
			md5_stream(fhandle, md5sum);
			if(devparm)
				CONS_Printf("md5 calc for %s took %f second\n",
					COM_Argv(1), (float)(I_GetTime() - t)/TICRATE);
			fclose(fhandle);
		}
		else
		{
			CONS_Printf("File doesn't exist.\n");
			return;
		}
		strcpy(&buf[strlen(buf)+2], (const char *)md5sum);
		length += sizeof(md5sum)+1;
	}
#endif

	SendNetXCmd(XD_ADDFILE, buf, length);
}

static void Got_Addfilecmd(char** cp, int playernum)
{
	char filename[255];
#ifndef NOMD5
	unsigned char md5sum[16];
#endif

	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal addfile command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	strcpy(filename, *cp);
	SKIPSTRING(*cp);

#ifndef NOMD5
	(*cp)++;
	strncpy((char *)md5sum, *cp, 16);
	SKIPSTRING(*cp);
	{
		FILE* fhandle;
		unsigned char localmd5[16];

		fhandle = fopen(filename, "rb");

		if(fhandle != NULL)
		{
			int t = I_GetTime();
			md5_stream(fhandle, localmd5);
			if(devparm)
				CONS_Printf("md5 calc for %s took %f second\n",
					filename, (float)(I_GetTime() - t)/TICRATE);
			fclose(fhandle);
		}
		else
		{
			I_Error("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.", filename);
			return;
		}

		if(devparm)
		{
			CONS_Printf("Remote MD5 is %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i\n",
				md5sum[0], md5sum[1], md5sum[2], md5sum[3], md5sum[4], md5sum[5], md5sum[6], md5sum[7], md5sum[8], md5sum[9], md5sum[10], md5sum[11], md5sum[12], md5sum[13], md5sum[14], md5sum[15]);

			CONS_Printf("Local MD5 is %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i\n",
				localmd5[0], localmd5[1], localmd5[2], localmd5[3], localmd5[4], localmd5[5], localmd5[6], localmd5[7], localmd5[8], localmd5[9], localmd5[10], localmd5[11], localmd5[12], localmd5[13], localmd5[14], localmd5[15]);
		}

		if(strncmp((char *)md5sum, (char *)localmd5, 16))
		{
			I_Error("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n", filename);
			return;
		}
	}
#endif

	P_AddWadFile(filename, NULL);
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f (void)
{
	CONS_Printf("SRB2%s (" __TIME__ " " __DATE__ ")\n", VERSIONSTRING);
}

// Returns current gametype being used.
//
static void Command_ShowGametype_f (void)
{
	CONS_Printf("Current gametype is %i\n", gametype);
}

/** Lists all players and their player numbers.
  * This function is named wrong. It doesn't print node numbers.
  *
  * \todo Remove.
  * \sa Command_GetPlayerNum
  */
static void Command_Nodes_f(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if(playeringame[i])
			CONS_Printf("%d : %s\n", i, player_names[i]);
	}
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	F_StartIntro();
}

/** Writes the mapthings in the current level to a file.
  *
  * \sa cv_objectplace
  */
static void Command_Writethings_f(void)
{
	P_WriteThings(W_GetNumForName(G_BuildMapName(gamemap)) + ML_THINGS);
}

/** Quits the game immediately.
  */
FUNCNORETURN static void Command_Quit_f(void)
{
	I_Quit();
}

static void ObjectPlace_OnChange(void)
{
#ifndef JOHNNYFUNCODE
	if((netgame || multiplayer) && cv_objectplace.value) // You spoon!
	{
		CV_StealthSetValue(&cv_objectplace, 0);
		CONS_Printf("No, dummy, you can't use this in multiplayer!\n");
		return;
	}
#else
	if(cv_objectplace.value)
	{
		int i;

		for(i = 0; i < MAXPLAYERS; i++)
		{
			if(!playeringame[i])
				continue;

			if(!players[i].mo)
				continue;

			if(players[i].nightsmode)
				continue;

			players[i].mo->flags2 |= MF2_DONTDRAW;
			players[i].mo->flags |= MF_NOCLIP;
			players[i].mo->flags |= MF_NOGRAVITY;
			P_UnsetThingPosition(players[i].mo);
			players[i].mo->flags |= MF_NOBLOCKMAP;
			P_SetThingPosition(players[i].mo);
			if(!players[i].currentthing)
				players[i].currentthing = 1;
			if(!modifiedgame || savemoddata)
			{
				modifiedgame = true;
				savemoddata = false;
				if(!(netgame || multiplayer))
					CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
			}
		}
	}
	else if(players[0].mo)
	{
		int i;

		for(i = 0; i < MAXPLAYERS; i++)
		{
			if(!playeringame[i])
				continue;

			if(!players[i].mo)
				continue;

			if(!players[i].nightsmode)
			{
				if(players[i].mo->target)
					P_SetMobjState(players[i].mo->target, S_DISS);

				players[i].mo->flags2 &= ~MF2_DONTDRAW;
				players[i].mo->flags &= ~MF_NOGRAVITY;
			}

			players[i].mo->flags &= ~MF_NOCLIP;
			P_UnsetThingPosition(players[i].mo);
			players[i].mo->flags &= ~MF_NOBLOCKMAP;
			P_SetThingPosition(players[i].mo);
		}
	}
	return;
#endif

	if(cv_objectplace.value)
	{
		if(players[0].nightsmode)
			return;

		players[0].mo->flags2 |= MF2_DONTDRAW;
		players[0].mo->flags |= MF_NOCLIP;
		players[0].mo->flags |= MF_NOGRAVITY;
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags |= MF_NOBLOCKMAP;
		P_SetThingPosition(players[0].mo);
		if(!players[0].currentthing)
			players[0].currentthing = 1;
		players[0].mo->momx = players[0].mo->momy = players[0].mo->momz = 0;
	}
	else if(players[0].mo)
	{
		if(!players[0].nightsmode)
		{
			if(players[0].mo->target)
				P_SetMobjState(players[0].mo->target, S_DISS);

			players[0].mo->flags2 &= ~MF2_DONTDRAW;
			players[0].mo->flags &= ~MF_NOGRAVITY;
		}

		players[0].mo->flags &= ~MF_NOCLIP;
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags &= ~MF_NOBLOCKMAP;
		P_SetThingPosition(players[0].mo);
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_pointlimit, TimeLimit_OnChange
  */
static void PointLimit_OnChange(void)
{
	// Don't allow pointlimit in Single Player/Co-Op/Race!
	if(server && cv_pointlimit.value && (gametype == GT_COOP || gametype == GT_RACE))
	{
		CV_SetValue(&cv_pointlimit, 0);
		return;
	}

	if(cv_pointlimit.value) // Has to be greater than 0, since it's a CV_Unsigned
	{
		CONS_Printf("Levels will end after someone scores %d point%s.\n",
			cv_pointlimit.value, cv_pointlimit.value > 1 ? "s" : "");

		// Note the deliberate absence of any code preventing pointlimit and timelimit
		//   from being set simultaneously.
		// Such code previously existed, but Mystic specifically requested its removal.

		// Note also the absence of code checking immediately for pointlimit having been
		//   reached. I used to do that here -- you'd get "caught" in the menu as soon as
		//   you set pointlimit to 1 (or 50) and the round would be over. Stupid.
	}
	else
		CONS_Printf("Point limit disabled\n");
}

ULONG timelimitintics = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
static void TimeLimit_OnChange(void)
{
	// Don't allow timelimit in Single Player/Co-Op/Race! Tails 09-06-2001
	if(server && cv_timelimit.value && (gametype == GT_COOP || gametype == GT_RACE))
	{
		CV_SetValue(&cv_timelimit, 0);
		return;
	}

	if(cv_timelimit.value)
	{
		CONS_Printf("Levels will end after %d minute%s.\n",cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s"); // Graue 11-17-2003
		timelimitintics = cv_timelimit.value * 60 * TICRATE;

		// Note the deliberate absence of any code preventing pointlimit and timelimit
		//   from being set simultaneously.
		// Such code previously existed, but Mystic specifically requested its removal.
	}
	else
		CONS_Printf("Time limit disabled\n");
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(int lastgametype)
{
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if(server && (multiplayer || netgame))
	{
		if(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF
			|| gametype == GT_CHAOS)
		{
			CV_SetValue(&cv_itemrespawn, 1);
		}
		else
			CV_SetValue(&cv_itemrespawn, 0);

		if(gametype == GT_CHAOS)
		{
			if(!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
			{
				// default settings for chaos: timelimit 2 mins, no pointlimit
				CV_SetValue(&cv_pointlimit, 0);
				CV_SetValue(&cv_timelimit, 2);
			}
			if(!cv_itemrespawntime.changed)
				CV_SetValue(&cv_itemrespawntime, 90); // respawn sparingly in chaos
		}
		else if(gametype == GT_MATCH)
		{
			if(!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
			{
				// default settings for match: timelimit 5 mins, no pointlimit
				CV_SetValue(&cv_pointlimit, 0);
				CV_SetValue(&cv_timelimit, 5);
			}
			if(!cv_itemrespawntime.changed)
				CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
		}
		else if(gametype == GT_TAG)
		{
			if(!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
			{
				// default settings for match: timelimit 5 mins, no pointlimit
				CV_SetValue(&cv_timelimit, 0);
				CV_SetValue(&cv_pointlimit, 10);
			}
			if(!cv_itemrespawntime.changed)
				CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
		}
		else if(gametype == GT_CTF)
		{
			if(!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
			{
				// default settings for CTF: no timelimit, pointlimit 5
				CV_SetValue(&cv_timelimit, 0);
				CV_SetValue(&cv_pointlimit, 5);
			}
			if(!cv_itemrespawntime.changed)
				CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally

			if(!players[consoleplayer].ctfteam)
				SendNameAndColor(); // make sure a CTF team gets assigned
			if(cv_splitscreen.value && ((!netgame && !players[1].ctfteam)
				|| (secondarydisplayplayer != consoleplayer
				&& !players[secondarydisplayplayer].ctfteam)))
				SendNameAndColor2();
		}
	}
	else if(!multiplayer && !netgame)
	{
		gametype = GT_COOP;
		CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue);
		CV_SetValue(&cv_itemrespawn, 0);
	}

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if(server && (gametype == GT_RACE || gametype == GT_COOP))
	{
		if(cv_timelimit.value)
			CV_SetValue(&cv_timelimit, 0);
		if(cv_pointlimit.value)
			CV_SetValue(&cv_pointlimit, 0);
	}

	if((cv_pointlimit.changed || cv_timelimit.changed) && cv_pointlimit.value)
	{
		if((lastgametype == GT_CHAOS || lastgametype == GT_MATCH) &&
			(gametype == GT_TAG || gametype == GT_CTF))
			CV_SetValue(&cv_pointlimit, cv_pointlimit.value / 500);
		else if((lastgametype == GT_TAG || lastgametype == GT_CTF) &&
			(gametype == GT_CHAOS || gametype == GT_MATCH))
			CV_SetValue(&cv_pointlimit, cv_pointlimit.value * 500);
	}

	// don't retain CTF teams in other modes
	if(lastgametype == GT_CTF)
	{
		int i;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				players[i].ctfteam = 0;
	}
}

static void Playerspeed_OnChange(void)
{
	// If you've got a grade less than 2, you can't use this.
	if(grade < 2 && cv_playerspeed.value != 1)
		CV_SetValue(&cv_playerspeed, 1);

	if(cv_playerspeed.value != 65536) // Only if changed
	{
		if(!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if(!(netgame || multiplayer))
				CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
		}
	}
}

static void Ringslinger_OnChange(void)
{
	// If you've got a grade less than 3, you can't use this.
	if(grade < 3 && cv_ringslinger.value)
		CV_Set(&cv_ringslinger, "No");

	if(cv_ringslinger.value) // Only if it's been turned on
	{
		if(!modifiedgame || savemoddata)
		{
			modifiedgame = true;
			savemoddata = false;
			if(!(netgame || multiplayer))
				CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
		}
	}
}

static void Startrings_OnChange(void)
{
	// If you've got a grade less than 5, you can't use this.
	if((grade < 5 || (!netgame && !cv_debug)) && cv_startrings.value)
	{
		CV_SetValue(&cv_startrings, 0);
		return;
	}

	if(cv_startrings.value)
	{
		int i;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i] && players[i].mo)
			{
				players[i].mo->health = cv_startrings.value + 1;
				players[i].health = players[i].mo->health;
			}
	}
}

static void Startlives_OnChange(void)
{
	// If you've got a grade less than 4, you can't use this.
	if((grade < 4 || (!netgame && !cv_debug)) && cv_startlives.value)
	{
		CV_SetValue(&cv_startlives, 0);
		return;
	}

	if(cv_startlives.value)
	{
		int i;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				players[i].lives = cv_startlives.value;
	}
}

static void Startcontinues_OnChange(void)
{
	// If you've got a grade less than 4, you can't use this.
	if((grade < 4 || (!netgame && !cv_debug)) && cv_startcontinues.value)
	{
		CV_SetValue(&cv_startcontinues, 0);
		return;
	}

	if(cv_startcontinues.value)
	{
		int i;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				players[i].continues = cv_startcontinues.value;
	}
}

static void Gravity_OnChange(void)
{
	gravity = cv_gravity.value;
	if(!(grade & 2) || netgame)
		return;
	else if(cv_gravity.value != FRACUNIT/2)
		CV_Set(&cv_gravity, "0.5");
}

static void Command_ExitLevel_f(void)
{
	if(!(netgame || (multiplayer && gametype != GT_COOP)) && !cv_debug)
	{
		CONS_Printf("You can't use this in single player!\n");
		return;
	}
	if(!(server || admin))
	{
		CONS_Printf("Only the server can exit the level\n");
		return;
	}
	if(gamestate != GS_LEVEL || demoplayback)
		CONS_Printf("You should be in a level to exit it!\n");

	SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

static void Got_ExitLevelcmd(char** cp, int playernum)
{
	cp = NULL;
	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal exitlevel command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	G_ExitLevel();
}

/** Prints the number of the displayplayer.
  *
  * \todo Possibly remove this; it was useful for debugging at one point.
  */
static void Command_Displayplayer_f(void)
{
	CONS_Printf("Displayplayer is %d\n", displayplayer);
}

static void Command_Tunes_f(void)
{
	int tune;

	if(COM_Argc() != 2)
	{
		CONS_Printf("tunes <slot>: play a slot\n");
		return;
	}

	tune = atoi(COM_Argv(1));

	if(tune < mus_None || tune >= NUMMUSIC)
	{
		CONS_Printf("valid slots are 1 to %d, or 0 to stop music\n", NUMMUSIC - 1);
		return;
	}

	mapmusic = (short)(tune | 2048);

	if(tune == mus_None)
		S_StopMusic();
	else
		S_ChangeMusic(tune, true);
}

static void Command_Load_f(void)
{
	byte slot;

	if(COM_Argc() != 2)
	{
		CONS_Printf("load <slot> : load a saved game\n");
		return;
	}

	if(!(server || admin))
	{
		CONS_Printf("Only server can do a load game\n");
		return;
	}

	if(demoplayback)
		G_StopDemo();

	// spawn a server if needed
	if(!admin)
		SV_SpawnServer();

	slot = (byte)atoi(COM_Argv(1));
	SendNetXCmd(XD_LOADGAME, &slot, 1);
}

static void Got_LoadGamecmd(char** cp, int playernum)
{
	byte slot;

	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal load game command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	slot = *(*cp)++;
	G_DoLoadGame(slot);
}

static void Command_Save_f(void)
{
	char p[SAVESTRINGSIZE + 1];

	if(COM_Argc() != 3)
	{
		CONS_Printf("save <slot> <desciption> : save game\n");
		return;
	}

	if(!(server || admin))
	{
		CONS_Printf("Only server can do a save game\n");
		return;
	}

	p[0] = (char)atoi(COM_Argv(1));
	strcpy(&p[1], COM_Argv(2));
	SendNetXCmd(XD_SAVEGAME, &p, strlen(&p[1])+2);
}

static void Got_SaveGamecmd(char** cp, int playernum)
{
	byte slot;
	char description[SAVESTRINGSIZE];

	if(playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Printf("Illegal save game command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	slot = *(*cp)++;
	strcpy(description,*cp);
	*cp += strlen(description) + 1;

	G_DoSaveGame(slot,description);
}

/** Quits a game and returns to the title screen.
  *
  * \todo Move the ctfteam resetting to a better place.
  */
void Command_ExitGame_f(void)
{
	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();
	if(gametype == GT_CTF)
	{
		int i;
		for(i = 0; i < MAXPLAYERS; i++)
			players[i].ctfteam = 0;
	}
	CV_SetValue(&cv_splitscreen, 0);
	D_StartTitle();
}

#ifdef FISHCAKE
// Fishcake is back, but only if you've cleared Very Hard mode
static void Fishcake_OnChange(void)
{
	if(veryhardcleared)
	{
		cv_debug = cv_fishcake.value;
		// consvar_t's get changed to default when registered
		// so don't make modifiedgame always on!
		if(cv_debug)
		{
			if(!modifiedgame || savemoddata)
			{
				modifiedgame = true;
				savemoddata = false;
				if(!(netgame || multiplayer))
					CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
			}
		}
	}

	else if(cv_debug != (boolean)cv_fishcake.value)
		CV_SetValue(&cv_fishcake, cv_debug);
}
#endif

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if(savemoddata)
		CONS_Printf("modifiedgame is true, but you can save emblem and time data in this mod.\n");
	else if(modifiedgame)
		CONS_Printf("modifiedgame is true, secrets will not be unlocked\n");
	else
		CONS_Printf("modifiedgame is false, you can unlock secrets\n");
}

/** Sets forced skin.
  * Available only to a dedicated server owner, who doesn't have an actual
  * player.
  *
  * This is not available to a remote admin, but a skin change from the remote
  * admin is treated as authoritative in that case.
  *
  * \todo Make the change take effect immediately.
  * \sa ForceSkin_OnChange, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void Command_SetForcedSkin_f(void)
{
	int newforcedskin;

	if(COM_Argc() != 2)
	{
		CONS_Printf("setforcedskin <num> : set skin for clients to use\n");
		return;
	}

	newforcedskin = atoi(COM_Argv(1));

	if(newforcedskin < 0 || newforcedskin >= numskins)
	{
		CONS_Printf("valid skin numbers are 0 to %d\n", numskins - 1);
		return;
	}

	forcedskin = newforcedskin;
}

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \todo Move the enforcement code out of SendNameAndColor() so this hack
  *       isn't needed.
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void ForceSkin_OnChange(void)
{
	if(cv_forceskin.value)
		SendNameAndColor(); // have it take effect immediately
}

/** Sends a skin change for the console player, unless that player is moving.
  *
  * \sa cv_skin, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	// make sure the player is not moving
	if(!players[consoleplayer].mo || (
		players[consoleplayer].rmomx < FRACUNIT/2 &&
		players[consoleplayer].rmomx > -FRACUNIT/2 &&
		players[consoleplayer].rmomy < FRACUNIT/2 &&
		players[consoleplayer].rmomy > -FRACUNIT/2 &&
		players[consoleplayer].mo->momz < FRACUNIT/2 &&
		players[consoleplayer].mo->momz > -FRACUNIT/2 &&
		!players[consoleplayer].powers[pw_tailsfly] &&
		!players[consoleplayer].mfjumped))
	{
		SendNameAndColor();
	}
	else
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  *
  * \sa cv_skin2, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	// make sure the player is not moving
	if(secondarydisplayplayer && (!players[secondarydisplayplayer].mo || (
		players[secondarydisplayplayer].rmomx < FRACUNIT/2 &&
		players[secondarydisplayplayer].rmomx > -FRACUNIT/2 &&
		players[secondarydisplayplayer].rmomy < FRACUNIT/2 &&
		players[secondarydisplayplayer].rmomy > -FRACUNIT/2 &&
		players[secondarydisplayplayer].mo->momz < FRACUNIT/2 &&
		players[secondarydisplayplayer].mo->momz > -FRACUNIT/2 &&
		!players[secondarydisplayplayer].powers[pw_tailsfly] &&
		!players[secondarydisplayplayer].mfjumped)))
	{
		SendNameAndColor2();
	}
	else
		CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
static void Mute_OnChange(void)
{
	if(server || admin)
		return;

	if(cv_mute.value)
		CONS_Printf("Chat has been muted.\n");
	else
		CONS_Printf("Chat is no longer muted.\n");
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  * 
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
static void DummyConsvar_OnChange(void)
{
	if(cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}
