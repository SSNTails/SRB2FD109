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
/// \brief SRB2 Network game communication and protocol, all OS independent parts.

#if !defined(UNDER_CE)
#include <time.h>
#endif
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "i_net.h"
#include "i_system.h"
#include "i_video.h"
#include "d_net.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "keys.h"
#include "m_menu.h"
#include "console.h"
#include "d_netfil.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "z_zone.h"
#include "p_local.h"
#include "m_misc.h"
#include "am_map.h"
#include "m_random.h"
#include "mserv.h"
#include "i_tcp.h"
#include "y_inter.h"
#include "r_main.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tic that hasn't had control made for it yet
// server:
//   nettics is the tic for each node
//   firstticstosend is the lowest value of nettics
// client:
//   neededtic is the tic needed by the client for run the game
//   firstticstosend is used to optimize a condition
// normaly maketic>=gametic>0,

#define PREDICTIONQUEUE BACKUPTICS
#define PREDICTIONMASK (PREDICTIONQUEUE-1)

boolean server = true; // true or false but !server == client
boolean admin = false; // Remote Administration 09-22-2003
boolean nodownload = false;
static boolean serverrunning = false;
int serverplayer;
char adminpassword[9];

// server specific vars
static byte playernode[MAXPLAYERS];
static tic_t cl_maketic[MAXNETNODES];
static signed char nodetoplayer[MAXNETNODES];
static signed char nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
static byte playerpernode[MAXNETNODES]; // used specialy for scplitscreen
static boolean nodeingame[MAXNETNODES]; // set false as nodes leave game
static tic_t nettics[MAXNETNODES]; // what tic the client have received
static tic_t supposedtics[MAXNETNODES]; // nettics prevision for smaller packet
static byte nodewaiting[MAXNETNODES];
static tic_t firstticstosend; // min of the nettics
static short consistancy[BACKUPTICS];
static tic_t tictoclear = 0; // optimize d_clearticcmd
static tic_t maketic;
#ifdef CLIENTPREDICTION2
tic_t localgametic;
#endif

// client specific
static ticcmd_t localcmds;
static ticcmd_t localcmds2;
static boolean cl_packetmissed;
// here it is for the secondary local player (splitscreen)
static byte mynode; // my address pointofview server

static byte localtextcmd[MAXTEXTCMD];
static byte localtextcmd2[MAXTEXTCMD]; // splitscreen
static tic_t neededtic;
signed char servernode; // the number of the server node
/// \brief do we accept new players?
/// \todo WORK!
boolean acceptnewnode = true;

// engine
ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
static byte textcmds[BACKUPTICS][MAXPLAYERS][MAXTEXTCMD];

consvar_t cv_playdemospeed = {"playdemospeed", "0", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

// some software don't support largest packet
// (original sersetup, not exactely, but the probabylity of sending a packet
// of 512 octet is like 0.1)
USHORT software_MAXPACKETLENGTH;

tic_t ExpandTics(int low)
{
	int delta;

	delta = low - (maketic & 0xff);

	if(delta >= -64 && delta <= 64)
		return (maketic & ~0xff) + low;
	if(delta > 64)
		return (maketic & ~0xff) - 256 + low;
	if(delta < -64)
		return (maketic & ~0xff) + 256 + low;
#ifdef PARANOIA
	I_Error("ExpandTics: strange value %i at maketic %i", low, maketic);
#endif
	return 0;
}

// -----------------------------------------------------------------
// Some extra data function for handle textcmd buffer
// -----------------------------------------------------------------

static void (*listnetxcmd[MAXNETXCMD])(char** p, int playernum);

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(char** p, int playernum))
{
#ifdef PARANOIA
	if(id >= MAXNETXCMD)
		I_Error("command id %d too big", id);
	if(listnetxcmd[id] != 0)
		I_Error("Command id %d already used", id);
#endif
	listnetxcmd[id] = cmd_f;
}

void SendNetXCmd(netxcmd_t id, void* param, size_t nparam)
{
	if(demoplayback)
		return;

	if(localtextcmd[0]+1+nparam > MAXTEXTCMD)
	{
		I_Error("No more place in the buffer for netcmd %d\nlocaltextcmd is %d\nnparam is %d",
			id, localtextcmd, nparam);
		return;
	}
	localtextcmd[0]++;
	localtextcmd[localtextcmd[0]] = (unsigned char)id;
	if(param && nparam)
	{
		memcpy(&localtextcmd[localtextcmd[0]+1], param, nparam);
		localtextcmd[0] = (byte)(localtextcmd[0] + (byte)nparam);
	}
}

// splitscreen player
void SendNetXCmd2(netxcmd_t id, void* param, size_t nparam)
{
	if(demoplayback)
		return;

	if(localtextcmd2[0]+1+nparam>MAXTEXTCMD)
	{
		I_Error("No more place in the buffer for netcmd %d\n",id);
		return;
	}
	localtextcmd2[0]++;
	localtextcmd2[localtextcmd2[0]] = (unsigned char)id;
	if(param && nparam)
	{
		memcpy(&localtextcmd2[localtextcmd2[0]+1], param, nparam);
		localtextcmd2[0] = (byte)(localtextcmd2[0] + (byte)nparam);
	}
}

static void ExtraDataTicker(void)
{
	int i, tic;
	byte* curpos;
	byte* bufferend;

	tic = gametic % BACKUPTICS;

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i] || i == 0)
		{
			curpos = (byte*)&(textcmds[tic][i]);
			bufferend = &curpos[curpos[0]+1];
			curpos++;
			while(curpos < bufferend)
			{
				if(*curpos < MAXNETXCMD && listnetxcmd[*curpos])
				{
					byte id = *curpos;
					curpos++;
					DEBFILE(va("executing x_cmd %d ply %d ", id, i));
					(listnetxcmd[id])((char**)&curpos, i);
					DEBFILE("done\n");
				}
				else
				{
					char buf[3];

					buf[0] = (char)i;
					buf[1] = KICK_MSG_CON_FAIL;
					SendNetXCmd(XD_KICK, &buf, 2);
					DEBFILE(va("player %d kicked [gametic=%d] reason as follows:\n", i, gametic));
					CONS_Printf("Got unknown net command [%d]=%d (max %d)\n",
						curpos - (byte*)&(textcmds[tic][i]), *curpos, textcmds[tic][i][0]);
					return;
				}
			}
		}
}

static void D_Clearticcmd(tic_t tic)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		textcmds[tic%BACKUPTICS][i][0] = 0;
		netcmds[tic%BACKUPTICS][i].angleturn = 0;
	}
	DEBFILE(va("clear tic %5d (%2d)\n", tic, tic%BACKUPTICS));
}

// -----------------------------------------------------------------
// end of extra data function
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// extra data function for lmps
// -----------------------------------------------------------------

// if extradatabit is set, after the ziped tic you find this:
//
//   type   |  description
// ---------+--------------
//   byte   | size of the extradata
//   byte   | the extradata (xd) bits: see XD_...
//            with this byte you know what parameter folow
// if(xd & XDNAMEANDCOLOR)
//   byte   | color
//   char[MAXPLAYERNAME] | name of the player
// endif
// if(xd & XD_WEAPON_PREF)
//   byte   | original weapon switch: boolean, true if use the old
//          | weapon switch methode
//   char[NUMWEAPONS] | the weapon switch priority
//   byte   | autoaim: true if use the old autoaim system
// endif
boolean AddLmpExtradata(byte** demo_point, int playernum)
{
	int tic;

	tic = gametic % BACKUPTICS;
	if(textcmds[tic][playernum][0] == 0)
		return false;

	memcpy(*demo_point, textcmds[tic][playernum], textcmds[tic][playernum][0]+1);
	*demo_point += textcmds[tic][playernum][0]+1;
	return true;
}

void ReadLmpExtraData(byte** demo_pointer, int playernum)
{
	unsigned char nextra;

	if(!demo_pointer)
	{
		textcmds[gametic%BACKUPTICS][playernum][0] = 0;
		return;
	}
	nextra = **demo_pointer;
	memcpy(textcmds[gametic%BACKUPTICS][playernum], *demo_pointer, nextra + 1);
	// increment demo pointer
	*demo_pointer += nextra + 1;
}

// -----------------------------------------------------------------
// end extra data function for lmps
// -----------------------------------------------------------------

static short Consistancy(void);

typedef enum
{
	cl_searching,
	cl_downloadfiles,
	cl_askjoin,
	cl_waitjoinresponse,
	cl_downloadsavegame,
	cl_connected
} cl_mode_t;

static void GetPackets(void);

static cl_mode_t cl_mode = cl_searching;

//
// SendClJoin
//
// send a special packet for declare how many player in local
// used only in arbitratrenetstart()
static boolean CL_SendJoin(void)
{
	CONS_Printf("Send join request...\n");
	netbuffer->packettype = PT_CLIENTJOIN;

	netbuffer->u.clientcfg.localplayers = (byte)((byte)cv_splitscreen.value + 1);
	netbuffer->u.clientcfg.version = VERSION;
	netbuffer->u.clientcfg.subversion = SUBVERSION;

	return HSendPacket(servernode, true, 0, sizeof(clientconfig_pak));
}

static void SV_SendServerInfo(int node, tic_t time)
{
	byte* p;

	netbuffer->packettype = PT_SERVERINFO;
	netbuffer->u.serverinfo.version = VERSION;
	netbuffer->u.serverinfo.subversion = SUBVERSION;
	// return back the time value so client can compute there ping
	netbuffer->u.serverinfo.time = time;
	netbuffer->u.serverinfo.numberofplayer = (unsigned char)(SHORT(doomcom->numplayers));
	netbuffer->u.serverinfo.maxplayer = (byte)cv_maxplayers.value;
	netbuffer->u.serverinfo.load = 0; // unused for the moment
	netbuffer->u.serverinfo.gametype = (byte)gametype;
	netbuffer->u.serverinfo.modifiedgame = (byte)modifiedgame;
	strncpy(netbuffer->u.serverinfo.servername, cv_servername.string, MAXSERVERNAME);
	if(gamemapname[0])
		strcpy(netbuffer->u.serverinfo.mapname, gamemapname);
	else
		strcpy(netbuffer->u.serverinfo.mapname, G_BuildMapName(gamemap));

	p = (byte *)PutFileNeeded();

	HSendPacket(node, false, 0, p - ((byte*)&netbuffer->u));
}

static boolean SV_SendServerConfig(int node)
{
	int i, playermask = 0;
	byte* p;

	netbuffer->packettype = PT_SERVERCFG;
	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i])
			playermask |= 1<<i;

	netbuffer->u.servercfg.version = VERSION;
	netbuffer->u.servercfg.subversion = SUBVERSION;

	netbuffer->u.servercfg.serverplayer = (byte)serverplayer;
	netbuffer->u.servercfg.totalplayernum = (unsigned char)(SHORT(doomcom->numplayers));
	netbuffer->u.servercfg.playerdetected = playermask;
	netbuffer->u.servercfg.gametic = gametic;
	netbuffer->u.servercfg.clientnode = (byte)node;
	netbuffer->u.servercfg.gamestate = (byte)gamestate;
	netbuffer->u.servercfg.gametype = (byte)gametype;
	netbuffer->u.servercfg.modifiedgame = (byte)modifiedgame;
	p = netbuffer->u.servercfg.netcvarstates;
	CV_SaveNetVars((char**)&p);

	return HSendPacket(node, true, 0, sizeof(serverconfig_pak)
		+ (p - netbuffer->u.servercfg.netcvarstates));
}

#define JOININGAME
#ifdef JOININGAME
#define SAVEGAMESIZE (512*1024)

static void SV_SendSaveGame(int node)
{
	size_t length;
	byte* savebuffer;

	// first save it in a malloced buffer
	save_p = savebuffer = (byte*)malloc(SAVEGAMESIZE);
	if(!save_p)
	{
		CONS_Printf("No more free memory for savegame\n");
		return;
	}

	P_SaveNetGame();

	length = save_p - savebuffer;
	if(length > SAVEGAMESIZE)
	{
		free(save_p);
		save_p = NULL;
		I_Error ("Savegame buffer overrun");
	}

	// then send it!
	SendRam(node, savebuffer, length, SF_RAM, 0);
}

#define TMPSAVENAME "badmath.sav"
static consvar_t cv_dumpconsistency = {"dumpconsistency", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void SV_SavedGame(void)
{
	size_t length;
	byte* savebuffer;
	char tmpsave[256];

	if(!cv_dumpconsistency.value)
		return;
	
#if defined(LINUX) || defined(__MACH__)
	sprintf(tmpsave, "%s/"TMPSAVENAME, srb2home);
#else
	sprintf(tmpsave, "%s\\"TMPSAVENAME, srb2home);
#endif

	// first save it in a malloced buffer
	save_p = savebuffer = (byte*)malloc(SAVEGAMESIZE);
	if(!save_p)
	{
		CONS_Printf("No more free memory for savegame\n");
		return;
	}

	P_SaveNetGame();

	length = save_p - savebuffer;
	if(length > SAVEGAMESIZE)
	{
		free(savebuffer);
		savebuffer = NULL;
		I_Error ("Savegame buffer overrun");
	}

	// then save it!
	FIL_WriteFile(tmpsave, savebuffer, length);

	free(savebuffer);
	save_p = NULL;
}

#undef  TMPSAVENAME
#define TMPSAVENAME "$$$.sav"

static void CL_LoadReceivedSavegame(void)
{
	byte* savebuffer;
	int length;
	char tmpsave[256];

#if defined(LINUX) || defined(__MACH__)
	sprintf(tmpsave, "%s/"TMPSAVENAME, srb2home);
#else
	sprintf(tmpsave, "%s\\"TMPSAVENAME, srb2home);
#endif

	length = FIL_ReadFile(tmpsave, &savebuffer);

	CONS_Printf("loading savegame length %d\n", length);
	if(!length)
	{
		I_Error("Can't read savegame sent");
		return;
	}

	save_p = savebuffer;

	paused = false;
	demoplayback = false;
	automapactive = false;

	// load a base level
	playerdeadview = false;

	if(!P_LoadNetGame())
	{
		CONS_Printf("Can't load the level!\n");
		return;
	}

	// done
	Z_Free(savebuffer);
	if(unlink(tmpsave) == -1)
		CONS_Printf("WARNING: Can't delete %s", tmpsave);
	consistancy[gametic%BACKUPTICS] = Consistancy();
	CON_ToggleOff();
}
#endif

static void SendAskInfo(int node)
{
	netbuffer->packettype = PT_ASKINFO;
	netbuffer->u.askinfo.version = VERSION;
	netbuffer->u.askinfo.time = LONG(I_GetTime());
	HSendPacket(node, false, 0, sizeof(askinfo_pak));
}

serverelem_t serverlist[MAXSERVERLIST];
unsigned int serverlistcount = 0;

static void SL_ClearServerList(int connectedserver)
{
	unsigned int i;
	for(i = 0; i < serverlistcount; i++)
		if(connectedserver != serverlist[i].node)
		{
			Net_CloseConnection(serverlist[i].node);
			serverlist[i].node = 0;
		}
	serverlistcount = 0;
}

static unsigned int SL_SearchServer(int node)
{
	unsigned int i;
	for(i = 0; i < serverlistcount; i++ )
		if(serverlist[i].node == node)
			return i;

	return (unsigned int)-1;
}

static void SL_InsertServer(serverinfo_pak* info, signed char node)
{
	unsigned int i;
	boolean moved;

	// search if not already on it
	i = SL_SearchServer(node);
	if(i == (unsigned int)-1)
	{
		// not found add it
		if(serverlistcount >= MAXSERVERLIST)
			return; // list full
		i = serverlistcount++;
	}

	serverlist[i].info = *info;
	serverlist[i].node = node;

	// list is sorted by time (ping)
	// so move the entry until it is sorted
	do
	{
		moved = false;
		if(i > 0 && serverlist[i].info.time < serverlist[i-1].info.time)
		{
			serverelem_t s;
			s = serverlist[i];
			serverlist[i] = serverlist[i-1];
			serverlist[i-1] = s;
			i--;
			moved = true;
		}
		else if(i < serverlistcount - 1 && serverlist[i].info.time > serverlist[i + 1].info.time)
		{
			serverelem_t s;
			s = serverlist[i];
			serverlist[i] = serverlist[i+1];
			serverlist[i+1] = s;
			i++;
			moved = true;
		}
	} while(moved);
}

void CL_UpdateServerList(boolean internetsearch)
{
	SL_ClearServerList(0);

	if(!netgame && I_NetOpenSocket)
	{
		I_NetOpenSocket();
		netgame = true;
		multiplayer = true;
	}
	// search for local servers
	SendAskInfo(BROADCASTADDR);

	if(internetsearch)
	{
		const msg_server_t* server_list;
		int i;

		server_list = GetShortServersList();
		if(server_list)
		{
			for(i = 0; server_list[i].header[0]; i++)
			{
				int node;
				char addr_str[24];

				// insert ip (and optionaly port) in node list
				sprintf(addr_str, "%s:%s", server_list[i].ip, server_list[i].port);
				node = I_NetMakeNode(addr_str);
				if(node == -1)
					break; // no more node free
				SendAskInfo(node);
			}
		}
	}
}

// use adaptive send using net_bandwidth and stat.sendbytes
static void CL_ConnectToServer(void)
{
	int numnodes, nodewaited = doomcom->numnodes, i;
	boolean waitmore;
	tic_t asksent, oldtic;
#ifdef JOININGAME
	char tmpsave[256];

#if defined(LINUX) || defined(__MACH__)
	sprintf(tmpsave, "%s/"TMPSAVENAME, srb2home);
#else
	sprintf(tmpsave, "%s\\"TMPSAVENAME, srb2home);
#endif
#endif

	cl_mode = cl_searching;

#ifdef JOININGAME
	// don't get a corrupt savegame error because $$$.sav already exists
	if(!access(tmpsave, R_OK) && unlink(tmpsave) == -1)
		I_Error("Can't delete %s", tmpsave);
#endif

	CONS_Printf("Press ESC to abort\n");
	if(servernode < 0 || servernode >= MAXNETNODES)
		CONS_Printf("Searching the server...\n");
	else
		CONS_Printf("Contacting the server...\n");

	if(gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // cleanup

	DEBFILE(va("waiting %d nodes\n", doomcom->numnodes));
	gamestate = wipegamestate = GS_WAITINGPLAYERS;

	numnodes = 1;
	oldtic = I_GetTime() - 1;
	asksent = (tic_t)-TICRATE;
	SL_ClearServerList(servernode);
	do
	{
		switch(cl_mode)
		{
			case cl_searching:
				// serverlist is updated by GetPacket function
				if(serverlistcount > 0)
				{
					// this can be a responce to our broadcast request
					if(servernode == -1 || servernode >= MAXNETNODES)
					{
						i = 0;
						servernode = serverlist[i].node;
						CONS_Printf("Found, ");
					}
					else
					{
						i = SL_SearchServer(servernode);
						if(i < 0)
							break; // the case
					}
					D_ParseFileneeded(serverlist[i].info.fileneedednum,
						(char *)serverlist[i].info.fileneeded);
					CONS_Printf("Checking files...\n");
					i = CL_CheckFiles();
					if(i == 2) // cannot join for some reason
					{
						CL_Reset();
						D_StartTitle();
						return;
					}
					else if(i == 1)
						cl_mode = cl_askjoin;
					else
					{ // must download something
						// no problem if can't send packet, we will retry later
						if(SendRequestFile())
							cl_mode = cl_downloadfiles;
					}
					break;
				}
				// ask the info to the server (askinfo packet)
				if(asksent + TICRATE < I_GetTime())
				{
					SendAskInfo(servernode);
					asksent = I_GetTime();
				}
				break;
			case cl_downloadfiles:
				waitmore = false;
				for(i = 0; i < fileneedednum; i++)
					if(fileneeded[i].status == FS_DOWNLOADING
						|| fileneeded[i].status == FS_REQUESTED)
					{
						waitmore = true;
						break;
					}
				if(waitmore)
					break; // exit the case

				cl_mode = cl_askjoin; // don't break case continue to cljoin request now
			case cl_askjoin:
				CL_LoadServerFiles();
#ifdef JOININGAME
				// prepare structures to save the file
				// WARNING: this can be useless in case of server not in GS_LEVEL
				// but since the network layer doesn't provide ordered packets...
				CL_PrepareDownloadSaveGame(tmpsave);
#endif
				if(CL_SendJoin())
					cl_mode = cl_waitjoinresponse;
				break;
#ifdef JOININGAME
			case cl_downloadsavegame:
				if(fileneeded[0].status == FS_FOUND)
				{
					CL_LoadReceivedSavegame();
					gamestate = GS_LEVEL;
					cl_mode = cl_connected;
				} // don't break case continue to cl_connected
				else
					break;
#endif
			case cl_waitjoinresponse:
			case cl_connected:
				break;
		}

		GetPackets();
		// connection closed by cancel or timeout
		if(!server && !netgame)
		{
			cl_mode = cl_searching;
			return;
		}
		Net_AckTicker();

		// call it only one by tic
		if(oldtic != I_GetTime())
		{
			int key;

			I_OsPolling();
			key = I_GetKey();
			if(key == KEY_ESCAPE)
			{
				M_StartMessage("Network game synchronization aborted.\n\nPress ESC\n", NULL, MM_NOTHING);
				CL_Reset();
				D_StartTitle();
				return;
			}
			if(key == 's' && server)
				doomcom->numnodes = (short)numnodes;

			FiletxTicker();
			oldtic = I_GetTime();

			CON_Drawer();
			I_FinishUpdate(); // page flip or blit buffer
		}
		else I_Sleep();

		if(server)
		{
			numnodes = 0;
			for(i = 0; i < MAXNETNODES; i++)
				if(nodeingame[i]) numnodes++;
		}
	} while(!(cl_mode == cl_connected && (!server || (server && nodewaited <= numnodes))));

	DEBFILE(va("Synchronisation Finished\n"));

	displayplayer = consoleplayer;
}

static void Command_connect(void)
{
	if(COM_Argc() < 2)
	{
		CONS_Printf("connect <serveraddress> : connect to a server\n"
			"connect ANY : connect to the first lan server found\n"
			"connect SELF: connect to self server\n");
		return;
	}

	if(gamestate != GS_INTRO && gamestate != GS_INTRO2 && gamestate != GS_TITLESCREEN && cl_mode != cl_searching)
	{
		CONS_Printf("You cannot connect while in a game\nEnd this game first\n");
		return;
	}

	server = false;

	if(!stricmp(COM_Argv(1), "self"))
	{
		servernode = 0;
		server = true;
		/// \bug should be but...
		//SV_SpawnServer();
	}
	else
	{
		// used in menu to connect to a server in the list
		if(netgame && !stricmp(COM_Argv(1), "node"))
			servernode = (char)atoi(COM_Argv(2));
		else if(netgame)
		{
			CONS_Printf("You cannot connect while in netgame\nLeave this game first\n");
			return;
		}
		else if(I_NetOpenSocket)
		{
			I_NetOpenSocket();
			netgame = true;
			multiplayer = true;

			if(!stricmp(COM_Argv(1), "any"))
				servernode = BROADCASTADDR;
			else if(I_NetMakeNode)
				servernode = I_NetMakeNode(COM_Argv(1));
			else
			{
				CONS_Printf("There is no server identification with this network driver\n");
				D_CloseConnection();
				return;
			}
		}
		else
			CONS_Printf("There is no network driver\n");
	}
	CL_ConnectToServer();
}

static void ResetNode(int node);

static void CL_RemovePlayer(int playernum)
{
	player_t* theplayer;

	if(server && !demoplayback)
	{
		int node = playernode[playernum];
		playerpernode[node]--;
		if(playerpernode[node] <= 0)
		{
			nodeingame[playernode[playernum]] = false;
			Net_CloseConnection(playernode[playernum]);
			ResetNode(node);
		}
	}

	// we should use a reset player but there is not such function
	if(gametype == GT_CTF)
		P_PlayerFlagBurst(&players[playernum]); // Don't take the flag with you!

	// remove avatar of player
	if(players[playernum].mo)
	{
		players[playernum].mo->player = NULL;
		players[playernum].mo->flags = 0;
		P_RemoveMobj(players[playernum].mo);
	}
	players[playernum].mo = NULL;
	playeringame[playernum] = false;
	playernode[playernum] = (byte)-1;
	while(!playeringame[doomcom->numplayers-1] && doomcom->numplayers > 1)
		doomcom->numplayers--;
/*
	// Hey, it's cheap, but it works!
	{
		int i, realplayers = 0;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				realplayers++;
		if(realplayers != doomcom->numplayers)
			doomcom->numplayers = (short)realplayers;
	}
*/
	theplayer = &players[playernum];
	// Wipe the player_t CLEAN!
	memset(theplayer, 0, sizeof(*theplayer));

	if(playernum == adminplayer)
		adminplayer = -1; // don't stay admin after you're gone

	if(playernum == displayplayer)
		displayplayer = consoleplayer; // don't look through someone's view who isn't there
}

void CL_Reset(void)
{
	if(demorecording)
		G_CheckDemoStatus();

	// reset client/server code
	DEBFILE(va("\n-=-=-=-=-=-=-= Client reset =-=-=-=-=-=-=-\n\n"));

	if(servernode > 0 && servernode < MAXNETNODES)
	{
		nodeingame[(byte)servernode] = false;
		Net_CloseConnection(servernode);
	}
	D_CloseConnection(); // netgame = false
	multiplayer = false;
	servernode = 0;
	server = true;
	doomcom->numnodes = 1;
	doomcom->numplayers = 1;
	SV_StopServer();
	SV_ResetServer();

	// D_StartTitle should get done now, but the calling function will handle it
}

static void Command_GetPlayerNum(void)
{
	int i;

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i])
		{
			if(serverplayer == i)
				CONS_Printf("\2num:%2d  node:%2d  %s\n", i, playernode[i], player_names[i]);
			else
				CONS_Printf("num:%2d  node:%2d  %s\n", i, playernode[i], player_names[i]);
		}
}

char nametonum(const char* name)
{
	int playernum, i;

	if(!strcmp(name, "0"))
		return 0;

	playernum = (char)atoi(name);

	if(playernum)
	{
		if(playeringame[playernum])
			return (char)playernum;
		else
			return -1;
	}

	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i] && !stricmp(player_names[i], name))
			return (char)i;

	CONS_Printf("there is no player named \"%s\"\n", name);

	return -1;
}

static void Command_Ban(void)
{
	char buf[3];

	if(COM_Argc() != 2)
	{
		CONS_Printf("ban <playername> or <playernum> : ban and kick a player\n");
		return;
	}

	if(server || admin)
	{
		buf[0] = nametonum(COM_Argv(1));
		if(buf[0] == (char)-1)
			return;
		if(!I_Ban(playernode[(int)buf[0]]))
		{
			CONS_Printf("Too many bans! Geez, that's a lot of people you're excluding...\n");
			buf[1] = KICK_MSG_GO_AWAY;
		}
		else
			buf[1] = KICK_MSG_BANNED;
		SendNetXCmd(XD_KICK, &buf, 2);
	}
	else
		CONS_Printf("You are not the server\n");

}

static void Command_ClearBans(void)
{
	I_ClearBans();
}

static void Command_Kick(void)
{
	char buf[3];

	if(COM_Argc() != 2)
	{
		CONS_Printf ("kick <playername> or <playernum> : kick a player\n");
		return;
	}

	if(server || admin)
	{
		buf[0] = nametonum(COM_Argv(1));
		if(buf[0] == (char)-1)
			return;
		buf[1] = KICK_MSG_GO_AWAY;
		SendNetXCmd(XD_KICK, &buf, 2);
	}
	else
		CONS_Printf("You are not the server\n");
}

static void Got_KickCmd(char** p, int playernum)
{
	int pnum, msg;

	pnum = READBYTE(*p);
	msg = READBYTE(*p);

	if(pnum == serverplayer && playernum == adminplayer)
	{
		CONS_Printf("Server is remotely shutting down. Goodbye!\n");

		if(server)
			COM_BufAddText("quit\n");

		return;
	}

	// Is playernum authorized to make this kick?
	if(playernum != serverplayer && playernum != adminplayer
		&& !(playerpernode[playernode[playernum]] == 2
		&& nodetoplayer2[playernode[playernum]] == pnum))
	{
		// We received a kick command from someone who isn't the server or admin,
		//  and who isn't in splitscreen removing player 2.
		// Thus, it's probably some hax0ring jerk.
		// Kick him instead, ha ha.
		CONS_Printf("Illegal kick command received from %s for player %d\n",
			player_names[playernum], pnum);
		CONS_Printf("So, you must be asking, why is this an illegal kick?\n"
		            "Well, let's take a look at the facts, shall we?\n"
		            "\n"
		            "playernum (this is the guy who did it), he's %d.\n"
		            "pnum (the guy he's trying to kick) is %d.\n"
		            "playernum's node is %d.\n"
		            "That node has %d players.\n"
		            "Player 2 on that node is %d.\n"
		            "pnum's node is %d.\n"
		            "That node has %d players.\n"
		            "Player 2 on that node is %d.\n"
		            "\n"
		            "So, I think it's now quite clear I need to get a frickin' life.\n",
			playernum, pnum,
			playernode[playernum], playerpernode[playernode[playernum]],
			nodetoplayer2[playernode[playernum]],
			playernode[pnum], playerpernode[playernode[pnum]],
			nodetoplayer2[playernode[pnum]]); /// \todo remove this garbage
		pnum = playernum;
		msg = KICK_MSG_CON_FAIL;
	}

	CONS_Printf("\2%s ", player_names[pnum]);

	switch(msg)
	{
		case KICK_MSG_GO_AWAY:
			CONS_Printf("has been kicked (Go away)\n");
			break;
		case KICK_MSG_CON_FAIL:
			CONS_Printf("has been kicked (Consistency failure)\n");
			break;
		case KICK_MSG_TIMEOUT:
			CONS_Printf("left the game (Connection timeout)\n");
			break;
		case KICK_MSG_PLAYER_QUIT:
			CONS_Printf("left the game\n");
			break;
		case KICK_MSG_BANNED:
			CONS_Printf("has been kicked (Banned)\n");
			break;
	}

	if(pnum == consoleplayer)
	{
		if(msg == KICK_MSG_CON_FAIL) SV_SavedGame();
		CL_Reset();
		D_StartTitle();
		if(msg == KICK_MSG_CON_FAIL)
		{
			M_StartMessage("You have been kicked\n(consistency failure)\nPress ESC\n", NULL,
				MM_NOTHING);
		}
		else if(msg == KICK_MSG_BANNED)
			M_StartMessage("You have been banned by the server\n\nPress ESC\n", NULL, MM_NOTHING);
		else
			M_StartMessage("You have been kicked by the server\n\nPress ESC\n", NULL, MM_NOTHING);
	}
	else
		CL_RemovePlayer(pnum);
}

static CV_PossibleValue_t maxplayers_cons_t[] = {{1, "MIN"}, {32, "MAX"}, {0, NULL}};
static CV_PossibleValue_t maxsend_cons_t[] = {{0, "MIN"}, {51200, "MAX"}, {0, NULL}};

consvar_t cv_allownewplayer = {"allowjoin", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL	};
consvar_t cv_disallownewplayer = {"disallowjoin", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; /// \todo not done
consvar_t cv_maxplayers = {"maxplayers", "8", CV_SAVE, maxplayers_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// max file size to send to a player (in kilobytes)
consvar_t cv_maxsend = {"maxsend", "200", CV_SAVE, maxsend_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static void Got_AddPlayer(char** p, int playernum);

// called one time at init
void D_ClientServerInit(void)
{
	DEBFILE(va("- - -== SRB2 v%d.%.2d.%d"VERSIONSTRING" debugfile ==- - -\n",
		VERSION/100, VERSION%100, SUBVERSION));

	COM_AddCommand("getplayernum", Command_GetPlayerNum);
	COM_AddCommand("kick", Command_Kick);
	COM_AddCommand("ban", Command_Ban);
	COM_AddCommand("clearbans", Command_ClearBans);
	COM_AddCommand("connect", Command_connect);

	RegisterNetXCmd(XD_KICK, Got_KickCmd);
	RegisterNetXCmd(XD_ADDPLAYER, Got_AddPlayer);
	CV_RegisterVar(&cv_allownewplayer);
	CV_RegisterVar(&cv_disallownewplayer);
	CV_RegisterVar(&cv_dumpconsistency);

	gametic = 0;
	localgametic = 0;

	// do not send anything before the real begin
	SV_StopServer();
	SV_ResetServer();
	if(dedicated)
		SV_SpawnServer();
}

static void ResetNode(int node)
{
	nodeingame[node] = false;
	nodetoplayer[node] = -1;
	nodetoplayer2[node] = -1;
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	cl_maketic[node] = maketic;
	nodewaiting[node] = 0;
	playerpernode[node] = 0;
}

void SV_ResetServer(void)
{
	int i;

	// +1 because this command will be executed in com_executebuffer in
	// tryruntic so gametic will be incremented, anyway maketic > gametic
	// is not a issue

	maketic = gametic + 1;
	neededtic = maketic;
#ifdef CLIENTPREDICTION2
	localgametic = gametic;
#endif
	tictoclear = maketic;

	for(i = 0; i < MAXNETNODES; i++)
		ResetNode(i);

	for(i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = false;
		playernode[i] = (byte)-1;
	}

	mynode = 0;
	cl_packetmissed = false;

	if(dedicated)
	{
		nodeingame[0] = true;
		serverplayer = -1;
	}
	else
		serverplayer = consoleplayer;

	if(server)
		servernode = 0;

	doomcom->numplayers = 0;

	DEBFILE(va("\n-=-=-=-=-=-=-= Server Reset =-=-=-=-=-=-=-\n\n"));
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	if(!netgame)
		return;

	DEBFILE("===========================================================================\n"
	        "                  Quitting Game, closing connection\n"
	        "===========================================================================\n");

	// abort send/receive of files
	CloseNetFile();

	if(server)
	{
		int i;

		netbuffer->packettype = PT_SERVERSHUTDOWN;
		for(i = 0; i < MAXNETNODES; i++)
			if(nodeingame[i])
				HSendPacket(i, true, 0, 0);
		if(serverrunning && cv_internetserver.value)
			UnregisterServer();
	}
	else if(servernode > 0 && servernode < MAXNETNODES && nodeingame[(byte)servernode]!=0)
	{
		netbuffer->packettype = PT_CLIENTQUIT;
		HSendPacket(servernode, true, 0, 0);
	}

	D_CloseConnection();

	DEBFILE("===========================================================================\n"
	        "                         Log finish\n"
	        "===========================================================================\n");
#ifdef DEBUGFILE
	if(debugfile)
	{
		fclose(debugfile);
		debugfile = NULL;
	}
#endif
}

// add a node to the game (player will follow at map change or at savegame....)
static inline void SV_AddNode(int node)
{
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	cl_maketic[node] = maketic;
	// little hack because the server connect to itself and put
	// nodeingame when connected not here
	if(node)
		nodeingame[node] = true;
}

// Xcmd XD_ADDPLAYER
static void Got_AddPlayer(char** p, int playernum)
{
	int node, newplayernum;
	boolean splitscreenplayer;
	static ULONG sendconfigtic = 0xffffffff;

	if(playernum != serverplayer && playernum != adminplayer)
	{
		// protect against hacked/buggy client
		CONS_Printf("Illegal add player command received from %s\n", player_names[playernum]);
		if(server)
		{
			char buf[2];

			buf[0] = (char)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	node = READBYTE(*p);
	newplayernum = READBYTE(*p);
	splitscreenplayer = newplayernum & 0x80;
	newplayernum &= ~0x80;

	playeringame[newplayernum] = true;
	G_AddPlayer(newplayernum);
	if( newplayernum+1>doomcom->numplayers )
        doomcom->numplayers=(short)(newplayernum+1);

#ifdef PARANOIA
	{
		int i, realplayers = 0;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i])
				realplayers++;
		if(realplayers != doomcom->numplayers)
			I_Error("Realplayers is %d, numplayers is %d", realplayers, doomcom->numplayers);
	}
#endif

	CONS_Printf("Player %d is in the game (node %d)\n", newplayernum+1, node);

	// the server is creating my player
	if(node == mynode)
	{
		playernode[newplayernum] = 0; // for information only
		if(!splitscreenplayer)
		{
			consoleplayer = newplayernum;
			displayplayer = newplayernum;
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning me\n");
		}
		else
		{
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning my brother\n");
		}
		addedtogame = true;
	}

	// the new player sends his config
	// and the old players send their configs to the new one
	/// \todo fixthis
	/// WARNING: this can cause a bottleneck in the txtcmd
	///          this can also produce a consistency failure if the packet gets lost
	///          because everybody knows the actual config except the joiner

	// don't send the config more than once per tic (multiple players join)
	if(sendconfigtic != gametic)
	{
		sendconfigtic = gametic;
		D_SendPlayerConfig();
	}
}

boolean SV_AddWaitingPlayers(void)
{
	int node, n, newplayer = false;
	byte buf[2], newplayernum;

	newplayernum = 0;
	for(node = 0; node < MAXNETNODES; node++)
	{
		// splitscreen can allow 2 player in one node
		for(; nodewaiting[node] > 0; nodewaiting[node]--)
		{
			newplayer = true;

			// search for a free playernum
			// we can't use playeringame since it is not updated here
			for(; newplayernum < MAXPLAYERS; newplayernum++)
			{
				for(n = 0; n < MAXNETNODES; n++)
					if(nodetoplayer[n] == newplayernum || nodetoplayer2[n] == newplayernum)
						break;
				if(n == MAXNETNODES)
					break;
			}

#ifdef PARANOIA
			// should never happen since we check the playernum before accept
			// the join
			if(newplayernum == MAXPLAYERS)
				I_Error("SV_AddWaitingPlayers: newplayernum == MAXPLAYERS\n");
#endif
			playernode[newplayernum] = (char)node;

			buf[0] = (byte)node;
			buf[1] = newplayernum;
			if(playerpernode[node] < 1)
				nodetoplayer[node] = newplayernum;
			else
			{
				nodetoplayer2[node] = newplayernum;
				buf[1] |= 0x80;
			}
			playerpernode[node]++;

			SendNetXCmd(XD_ADDPLAYER, &buf, 2);

			DEBFILE(va("Server added player %d node %d\n", newplayernum, node));
			// use the next free slot (we can't put playeringame[newplayernum] = true here)
			newplayernum++;
		}
	}

	return newplayer;
}

void CL_AddSplitscreenPlayer(void)
{
	if(cl_mode == cl_connected)
		CL_SendJoin();
}

void CL_RemoveSplitscreenPlayer(void)
{
	char buf[2];

	if(cl_mode != cl_connected)
		return;

	buf[0] = (char)secondarydisplayplayer;
	buf[1] = KICK_MSG_PLAYER_QUIT;
	SendNetXCmd(XD_KICK, &buf, 2);
}

// is there a game running
boolean Playing(void)
{
	return (server && serverrunning) || (!server && cl_mode == cl_connected);
}

boolean SV_SpawnServer(void)
{
	if(demoplayback)
		G_StopDemo(); // reset engine parameter

	if(!serverrunning)
	{
		CONS_Printf("Starting Server....\n");
		serverrunning = true;
		SV_ResetServer();
		if(netgame && I_NetOpenSocket)
		{
			I_NetOpenSocket();
			if(cv_internetserver.value)
				RegisterServer(0, 0);
		}

		// non dedicated server just connect to itself
		if(!dedicated)
			CL_ConnectToServer();
	}

	return SV_AddWaitingPlayers();
}

void SV_StopServer(void)
{
	tic_t i;

	if(gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;

	localtextcmd[0] = 0;
	localtextcmd2[0] = 0;

	for(i = 0; i < BACKUPTICS; i++)
		D_Clearticcmd(i);

	consoleplayer = 0;
	cl_mode = cl_searching;
	maketic = gametic+1;
	neededtic = maketic;
	serverrunning = false;
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = false;
	gametype = GT_COOP;

	// no more tic the game with this settings!
	SV_StopServer();

	if(cv_splitscreen.value)
		multiplayer = true;
}

static void SV_SendRefuse(int node, const char* reason)
{
	CONS_Printf("Refusing node %d: %s\n", node, reason); /// \todo remove
	strcpy(netbuffer->u.serverrefuse.reason, reason);

	netbuffer->packettype = PT_SERVERREFUSE;
	HSendPacket(node, true, 0, strlen(netbuffer->u.serverrefuse.reason) + 1);
	Net_CloseConnection(node);
}

// used at txtcmds received to check packetsize bound
static size_t TotalTextCmdPerTic(tic_t tic)
{
	size_t i, total = 1; // num of textcmds in the tic (ntextcmd byte)

	tic %= BACKUPTICS;

	for(i = 0; i < MAXPLAYERS; i++)
		if((!i || playeringame[i]) && textcmds[tic][i][0])
			total += 2 + textcmds[tic][i][0]; // "+2" for size and playernum

	return total;
}

/**	\brief GetPackets

  \todo  break this 300 line function into multiple functions
*/
static void GetPackets(void)
{
	int netconsole;
	signed char node;
	tic_t realend,realstart;
	int p = maketic%BACKUPTICS;
	byte* pak;
	byte* txtpak;
	byte numtxtpak;

	while(HGetPacket())
	{
		node = (signed char)doomcom->remotenode;
		if(netbuffer->packettype == PT_CLIENTJOIN && server)
		{
			if(bannednode[node])
				SV_SendRefuse(node, "You have been banned from the server");
			else if(netbuffer->u.clientcfg.version != VERSION
				|| netbuffer->u.clientcfg.subversion != SUBVERSION)
				SV_SendRefuse(node, va("Different SRB2 versions cannot play a netgame! "
				"(server version %d.%.2d.%d)", VERSION/100, VERSION%100, SUBVERSION));
			else if(!cv_allownewplayer.value && !node)
				SV_SendRefuse(node, "The server is not accepting joins for the moment");
//			else if(doomcom->numplayers >= cv_maxplayers.value)
//				SV_SendRefuse(node, va("Maximum players reached (max:%d)", cv_maxplayers.value));
			else
			{
				boolean newnode = false;
				int playersinthegame = 0;
				int i;

				for(i=0; i<MAXPLAYERS; i++)
				{
					if(playeringame[i])
						playersinthegame++;
				}

				if(playersinthegame >= cv_maxplayers.value)
				{
					SV_SendRefuse(node, va("Maximum players reached (max:%d)", cv_maxplayers.value));
					return;
				}

				// client authorised to join
				nodewaiting[node] = (byte)(netbuffer->u.clientcfg.localplayers - playerpernode[node]);
				if(!nodeingame[node])
				{
					gamestate_t backupstate = gamestate;
					newnode = true;
					SV_AddNode(node);
					if(cv_disallownewplayer.value && gameaction == ga_nothing)
						gamestate = GS_WAITINGPLAYERS;
					if(!SV_SendServerConfig(node))
					{
						gamestate = backupstate;
						/// \todo fix this !!!
						CONS_Printf("Internal Error 5: client lost\n");
						continue; // the while
					}
					gamestate = backupstate;
					DEBFILE("new node joined\n");
				}
#ifdef JOININGAME
				if(nodewaiting[node])
				{
					if(gamestate == GS_LEVEL && newnode)
					{
						SV_SendSaveGame(node);
						DEBFILE("send savegame\n");
					}
					SV_AddWaitingPlayers();
				}
#endif
			}
			continue;
		} // end of PT_CLIENTJOIN
		if(netbuffer->packettype == PT_SERVERSHUTDOWN && node == servernode
			&& !server && cl_mode != cl_searching)
		{
			M_StartMessage("Server has shutdown\n\nPress Esc", NULL, MM_NOTHING);
			CL_Reset();
			D_StartTitle();
			continue;
		}
		if(netbuffer->packettype == PT_NODETIMEOUT && node == servernode
			&& !server && cl_mode != cl_searching)
		{
			M_StartMessage("Server Timeout\n\nPress Esc", NULL, MM_NOTHING);
			CL_Reset();
			D_StartTitle();
			continue;
		}

		if(netbuffer->packettype == PT_SERVERINFO)
		{
			// compute ping in ms
			netbuffer->u.serverinfo.time = (I_GetTime()
				- netbuffer->u.serverinfo.time)*1000/TICRATE;
			netbuffer->u.serverinfo.servername[MAXSERVERNAME-1] = 0;

			SL_InsertServer(&netbuffer->u.serverinfo, node);
			continue;
		}

		if(!nodeingame[node])
		{
			if(node != servernode)
				DEBFILE(va("Received packet from unknown host %d\n", node));

			// anyone trying to join
			switch(netbuffer->packettype)
			{
				case PT_ASKINFO:
					if(server && serverrunning)
					{
						SV_SendServerInfo(node, netbuffer->u.askinfo.time);
						Net_CloseConnection(node);
					}
					break;
				case PT_SERVERREFUSE: // negative response of client join request
					if(cl_mode == cl_waitjoinresponse)
					{
						M_StartMessage(va("Server refuses connection\n\nReason:\n%s",
							netbuffer->u.serverrefuse.reason), NULL, MM_NOTHING);
						CL_Reset();
						D_StartTitle();
					}
					break;
				case PT_SERVERCFG: // positive response of client join request
				{
					int j;
					byte* p;

					/// \note how would this happen? and is it doing the right thing if it does?
					if(cl_mode != cl_waitjoinresponse)
						break;

					if(!server)
					{
						maketic = gametic = neededtic = netbuffer->u.servercfg.gametic;
						gametype = netbuffer->u.servercfg.gametype;
						modifiedgame = netbuffer->u.servercfg.modifiedgame;
					}
#ifdef CLIENTPREDICTION2
					localgametic = gametic;
#endif
					nodeingame[(byte)servernode] = true;
					serverplayer = netbuffer->u.servercfg.serverplayer;
					doomcom->numplayers = SHORT(netbuffer->u.servercfg.totalplayernum);
					mynode = netbuffer->u.servercfg.clientnode;
					if(serverplayer >= 0)
						playernode[(byte)serverplayer] = servernode;

					CONS_Printf("Join accepted, wait next map change...\n");
					DEBFILE(va("Server accept join gametic=%d mynode=%d\n", gametic, mynode));

					for(j = 0; j < MAXPLAYERS; j++)
						playeringame[j] = (netbuffer->u.servercfg.playerdetected & (1<<j)) != 0;

					p = netbuffer->u.servercfg.netcvarstates;
					CV_LoadNetVars((char**)&p);
#ifdef JOININGAME
					if(netbuffer->u.servercfg.gamestate == GS_LEVEL)
						cl_mode = cl_downloadsavegame;
					else
#endif
						cl_mode = cl_connected;
					break;
				}
				// handled in d_netfil.c
				case PT_FILEFRAGMENT:
					if(!server)
						Got_Filetxpak();
					break;
				case PT_REQUESTFILE:
					if(server)
						Got_RequestFilePak(node);
					break;
				case PT_NODETIMEOUT:
				case PT_CLIENTQUIT:
					if(server)
						Net_CloseConnection(node);
					break;
				case PT_SERVERTICS:
					// do not remove my own server (we have just get a out of order packet)
					if(node == servernode)
						break;
				case PT_CLIENTCMD:
					break; // this is not an "unknown packet"
				default:
					DEBFILE(va("unknown packet received (%d) from unknown host\n",
						netbuffer->packettype));
					Net_CloseConnection(node);
					break; // ignore it
			} // switch
			continue; //while
		}
		netconsole = nodetoplayer[node];
#ifdef PARANOIA
		if(netconsole >= MAXPLAYERS)
			I_Error("bad table nodetoplayer: node %d player %d", doomcom->remotenode, netconsole);
#endif

		switch(netbuffer->packettype)
		{
// -------------------------------------------- SERVER RECEIVE ----------
			case PT_CLIENTCMD:
			case PT_CLIENT2CMD:
			case PT_CLIENTMIS:
			case PT_CLIENT2MIS:
			case PT_NODEKEEPALIVE:
			case PT_NODEKEEPALIVEMIS:
				if(!server)
					break;

				// to save bytes, only the low byte of tic numbers are sent
				// Figure out what the rest of the bytes are
				realstart = ExpandTics(netbuffer->u.clientpak.client_tic);
				realend = ExpandTics(netbuffer->u.clientpak.resendfrom);

				if(netbuffer->packettype == PT_CLIENTMIS || netbuffer->packettype == PT_CLIENT2MIS
					|| netbuffer->packettype == PT_NODEKEEPALIVEMIS
					|| supposedtics[node] < realend)
				{
					supposedtics[node] = realend;
				}
				// discard out of order packet
				if(nettics[node] > realend)
				{
					DEBFILE(va("out of order ticcmd discarded nettics = %d\n", nettics[node]));
					break;
				}

				// update the nettics
				nettics[node] = realend;

				// don't do anything for packets of type NODEKEEPALIVE?
				if(netconsole == -1 || netbuffer->packettype == PT_NODEKEEPALIVE
					|| netbuffer->packettype == PT_NODEKEEPALIVEMIS)
					break;

				// check consistancy
				if(realstart <= gametic && realstart > gametic - BACKUPTICS+1 &&
					consistancy[realstart%BACKUPTICS] != netbuffer->u.clientpak.consistancy)
				{
					char buf[3];

					buf[0] = (char)netconsole;
					buf[1] = KICK_MSG_CON_FAIL;
					SV_SavedGame();
					SendNetXCmd(XD_KICK, &buf, 2);
					DEBFILE(va("player %d kicked (consistency failure) [%d] %d!=%d\n",
						netconsole, realstart, consistancy[realstart%BACKUPTICS],
						netbuffer->u.clientpak.consistancy));
				}

				// copy ticcmd
				memcpy(&netcmds[maketic%BACKUPTICS][netconsole], &netbuffer->u.clientpak.cmd,
					sizeof(ticcmd_t));

				if(netbuffer->packettype == PT_CLIENT2CMD && nodetoplayer2[node] >= 0)
					memcpy(&netcmds[maketic%BACKUPTICS][(byte)nodetoplayer2[node]],
						&netbuffer->u.client2pak.cmd2, sizeof(ticcmd_t));

				break;
			case PT_TEXTCMD2: // splitscreen special
				netconsole = nodetoplayer2[node];
			case PT_TEXTCMD:
				if(!server)
					break;

				if(netconsole < 0 || netconsole >= MAXPLAYERS)
					Net_UnAcknowledgPacket(node);
				else
				{
					size_t j;
					tic_t tic = maketic;

					// check if tic that we are making isn't too large else we cannot send it :(
					// doomcom->numplayers+1 "+1" since doomcom->numplayers can change within this time and sent time
					j = software_MAXPACKETLENGTH
						- (netbuffer->u.textcmd[0]+2+BASESERVERTICSSIZE
						+ (doomcom->numplayers+1)*sizeof(ticcmd_t));

					// search a tic that have enougth space in the ticcmd
					while((TotalTextCmdPerTic(tic) > j || netbuffer->u.textcmd[0]
						+ textcmds[tic%BACKUPTICS][netconsole][0] > MAXTEXTCMD)
						&& tic < firstticstosend + BACKUPTICS)
						tic++;

					if(tic >= firstticstosend + BACKUPTICS)
					{
						DEBFILE(va("GetPacket: Textcmd too long (max %d, used %d, mak %d, "
							"tosend %d, node %d, player %d)\n", j, TotalTextCmdPerTic(maketic),
							maketic, firstticstosend, node, netconsole));
						Net_UnAcknowledgPacket(node);
						break;
					}
					DEBFILE(va("textcmd put in tic %d at position %d (player %d) ftts %d mk %d\n",
						tic, textcmds[p][netconsole][0]+1, netconsole, firstticstosend, maketic));
					p = tic % BACKUPTICS;
					memcpy(&textcmds[p][netconsole][textcmds[p][netconsole][0]+1],
						netbuffer->u.textcmd+1, netbuffer->u.textcmd[0]);
					textcmds[p][netconsole][0] = (byte)(textcmds[p][netconsole][0] + (byte)netbuffer->u.textcmd[0]);
				}
				break;
			case PT_NODETIMEOUT:
			case PT_CLIENTQUIT:
				if(!server)
					break;

				// nodeingame will be put false in the execution of kick command
				// this allow to send some packets to the quitting client to have their ack back
				nodewaiting[node] = 0;
				if(netconsole != -1 && playeringame[netconsole])
				{
					char buf[2];
					buf[0] = (char)netconsole;
					if(netbuffer->packettype == PT_NODETIMEOUT)
						buf[1] = KICK_MSG_TIMEOUT;
					else
						buf[1] = KICK_MSG_PLAYER_QUIT;
					SendNetXCmd(XD_KICK, &buf, 2);
					nodetoplayer[node] = -1;
					if(nodetoplayer2[node] != -1 && nodetoplayer2[node] >= 0
						&& playeringame[(byte)nodetoplayer2[node]])
					{
						buf[0] = nodetoplayer2[node];
						SendNetXCmd(XD_KICK, &buf, 2);
						nodetoplayer2[node] = -1;
					}
				}
				Net_CloseConnection(node);
				nodeingame[node] = false;
				break;
// -------------------------------------------- CLIENT RECEIVE ----------
			case PT_SERVERTICS:
				realstart = ExpandTics(netbuffer->u.serverpak.starttic);
				realend = realstart + netbuffer->u.serverpak.numtics;

				txtpak = (unsigned char *)&netbuffer->u.serverpak.cmds[netbuffer->u.serverpak.numplayers
					* netbuffer->u.serverpak.numtics];

				if(realend > gametic + BACKUPTICS)
					realend = gametic + BACKUPTICS;
				cl_packetmissed = realstart > neededtic;

				if(realstart <= neededtic && realend > neededtic)
				{
					tic_t i, j;
					pak = (unsigned char *)&netbuffer->u.serverpak.cmds;

					for(i = realstart; i < realend; i++)
					{
						// clear first
						D_Clearticcmd(i);

						// copy the tics
						memcpy(netcmds[i%BACKUPTICS], pak,
							netbuffer->u.serverpak.numplayers*sizeof(ticcmd_t));
						pak += netbuffer->u.serverpak.numplayers*sizeof(ticcmd_t);

						// copy the textcmds
						numtxtpak = *txtpak++;
						for(j = 0; j < numtxtpak; j++)
						{
							int k = *txtpak++; // playernum

							memcpy(textcmds[i%BACKUPTICS][k], txtpak, txtpak[0]+1);
							txtpak += txtpak[0]+1;
						}
					}

					neededtic = realend;
				}
				else
					DEBFILE(va("frame not in bound: %u\n", neededtic));
				break;
			case PT_SERVERCFG:
				break;
			case PT_FILEFRAGMENT:
				if(!server)
					Got_Filetxpak();
				break;
			default:
				DEBFILE(va("UNKNOWN PACKET TYPE RECEIVED %f from host %d\n",
					netbuffer->packettype, node));
		} // end switch
	} // end while
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
// no more use random generator, because at very first tic isn't yet synchronized
static short Consistancy(void)
{
	short ret = 0;
	int i;

	DEBFILE(va("TIC %d ", gametic));
	for(i = 0; i < MAXPLAYERS; i++)
		if(playeringame[i] && players[i].mo)
		{
			DEBFILE(va("p[%d].x = %f ", i, FIXED_TO_FLOAT(players[i].mo->x)));
			ret = (short)(ret + players[i].mo->x);
		}
	DEBFILE(va("pos = %d, rnd %d\n", ret, P_GetRandIndex()));
	ret = (short)(ret + P_GetRandIndex());

	return ret;
}

// send the client packet to the server
static void CL_SendClientCmd(void)
{
	int packetsize = 0;

	netbuffer->packettype = PT_CLIENTCMD;

	if(cl_packetmissed)
		netbuffer->packettype++;
	netbuffer->u.clientpak.resendfrom = (unsigned char)neededtic;
	netbuffer->u.clientpak.client_tic = (unsigned char)gametic;

	if(gamestate == GS_WAITINGPLAYERS)
	{
		// send NODEKEEPALIVE packet
		netbuffer->packettype += 4;
		packetsize = sizeof(clientcmd_pak) - sizeof(ticcmd_t) - sizeof(short);
		HSendPacket(servernode, false, 0, packetsize);
	}
	else if(gamestate != GS_NULL)
	{
		memcpy(&netbuffer->u.clientpak.cmd, &localcmds, sizeof(ticcmd_t));
		netbuffer->u.clientpak.consistancy = consistancy[gametic%BACKUPTICS];

		// send a special packet with 2 cmd for splitscreen
		if(cv_splitscreen.value)
		{
			netbuffer->packettype += 2;
			memcpy(&netbuffer->u.client2pak.cmd2, &localcmds2, sizeof(ticcmd_t));
			packetsize = sizeof(client2cmd_pak);
		}
		else
			packetsize = sizeof(clientcmd_pak);

		HSendPacket(servernode, false, 0, packetsize);
	}

	if(cl_mode == cl_connected)
	{
		// send extra data if needed
		if(localtextcmd[0])
		{
			netbuffer->packettype = PT_TEXTCMD;
			memcpy(netbuffer->u.textcmd,localtextcmd, localtextcmd[0]+1);
			// all extra data have been sended
			if(HSendPacket(servernode, true, 0, localtextcmd[0]+1)) // send can fail...
				localtextcmd[0] = 0;
		}

		// send extra data if needed for player 2 (splitscreen)
		if(localtextcmd2[0])
		{
			netbuffer->packettype = PT_TEXTCMD2;
			memcpy(netbuffer->u.textcmd, localtextcmd2, localtextcmd2[0]+1);
			// all extra data have been sended
			if(HSendPacket(servernode, true, 0, localtextcmd2[0]+1)) // send can fail...
				localtextcmd2[0] = 0;
		}
	}
}

// send the server packet
// send tic from firstticstosend to maketic-1
static void SV_SendTics(void)
{
	tic_t realfirsttic, lasttictosend, i;
	ULONG n;
	int j;
	size_t packsize;
	char* bufpos;
	char* ntextcmd;

	// send to all client but not to me
	// for each node create a packet with x tics and send it
	// x is computed using supposedtics[n], max packet size and maketic
	for(n = 1; n < MAXNETNODES; n++)
		if(nodeingame[n])
		{
			lasttictosend = maketic;

			// assert supposedtics[n]>=nettics[n]
			realfirsttic = supposedtics[n];
			if(realfirsttic >= maketic)
			{
				// well we have sent all tics we will so use extrabandwidth
				// to resent packet that are supposed lost (this is necessary since lost
				// packet detection work when we have received packet with firsttic > neededtic
				// (getpacket servertics case)
				DEBFILE(va("Nothing to send node %d mak=%u sup=%u net=%u \n",
					n, maketic, supposedtics[n], nettics[n]));
				realfirsttic = nettics[n];
				if(realfirsttic >= maketic || (I_GetTime() + n)&3)
					// all tic are ok
					continue;
				DEBFILE(va("Sent %d anyway\n", realfirsttic));
			}
			if(realfirsttic < firstticstosend)
				realfirsttic = firstticstosend;

			// compute the length of the packet and cut it if too large
			packsize = BASESERVERTICSSIZE;
			for(i = realfirsttic; i < lasttictosend; i++)
			{
				packsize += sizeof(ticcmd_t) * doomcom->numplayers;
				packsize += TotalTextCmdPerTic(i);

				if(packsize > software_MAXPACKETLENGTH)
				{
					DEBFILE(va("packet too large (%d) at tic %d (should be from %d to %d)\n",
						packsize, i, realfirsttic, lasttictosend));
					lasttictosend = i;

					// too bad: too much player have send extradata and there is too
					//          much data in one tic.
					// To avoid it put the data on the next tic. (see getpacket
					// textcmd case) but when numplayer changes the computation can be different
					if(lasttictosend == realfirsttic)
					{
						if(packsize > MAXPACKETLENGTH)
							I_Error("Too many players: can't send %d data for %d players to node %d\n"
							        "Well sorry nobody is perfect....\n",
							        packsize, doomcom->numplayers, n);
						else
						{
							lasttictosend++; // send it anyway!
							DEBFILE("sending it anyway\n");
						}
					}
					break;
				}
			}

			// Send the tics
			netbuffer->packettype = PT_SERVERTICS;
			netbuffer->u.serverpak.starttic = (unsigned char)realfirsttic;
			netbuffer->u.serverpak.numtics = (unsigned char)(lasttictosend - realfirsttic);
			netbuffer->u.serverpak.numplayers = (unsigned char)SHORT(doomcom->numplayers);
			bufpos = (char*)&netbuffer->u.serverpak.cmds;

			for(i = realfirsttic; i < lasttictosend; i++)
			{
				memcpy(bufpos, netcmds[i%BACKUPTICS], doomcom->numplayers * sizeof(ticcmd_t));
				bufpos += doomcom->numplayers * sizeof(ticcmd_t);
			}

			// add textcmds
			for(i = realfirsttic; i < lasttictosend; i++)
			{
				ntextcmd = bufpos++;
				*ntextcmd = 0;
				for(j = 0; j < MAXPLAYERS; j++)
				{
					int size = textcmds[i%BACKUPTICS][j][0];

					if((!j || playeringame[j]) && size)
					{
						(*ntextcmd)++;
						*bufpos++ = (char)j;
						memcpy(bufpos, textcmds[i%BACKUPTICS][j], size + 1);
						bufpos += size + 1;
					}
				}
			}
			packsize = bufpos - (char*)&(netbuffer->u);

			HSendPacket(n, false, 0, packsize);
			// when tic are too large, only one tic is sent so don't go backward!
			if(lasttictosend-doomcom->extratics > realfirsttic)
				supposedtics[n] = lasttictosend-doomcom->extratics;
			else
				supposedtics[n] = lasttictosend;
			if(supposedtics[n] < nettics[n]) supposedtics[n] = nettics[n];
		}
	// node 0 is me !
	supposedtics[0] = maketic;
}

//
// TryRunTics
//
static void Local_Maketic(int realtics)
{
	I_OsPolling(); // I_Getevent
	D_ProcessEvents(); // menu responder, cons responder,
	                   // game responder calls HU_Responder, AM_Responder, F_Responder,
	                   // and G_MapEventsToControls
	if(dedicated) return;
	rendergametic = gametic;
	// translate inputs (keyboard/mouse/joystick) into game controls
	G_BuildTiccmd(&localcmds, realtics);
	if(cv_splitscreen.value)
		G_BuildTiccmd2(&localcmds2,realtics);

#ifdef CLIENTPREDICTION2
	if(!paused && localgametic < gametic + BACKUPTICS)
	{
		P_MoveSpirit(&players[consoleplayer], &localcmds, realtics);
		localgametic += realtics;
	}
#endif
	localcmds.angleturn |= TICCMD_RECEIVED;
}

void SV_SpawnPlayer(int playernum, int x, int y, angle_t angle)
{
	// for future copytic use the good x, y, and angle!
	if(server)
	{
#ifdef CLIENTPREDICTION2
		netcmds[maketic%BACKUPTICS][playernum].x = x;
		netcmds[maketic%BACKUPTICS][playernum].y = y;
#else
		x = y = 0;
#endif
		netcmds[maketic%BACKUPTICS][playernum].angleturn = (short)((short)(angle>>16) | TICCMD_RECEIVED);
	}
}

// create missed tic
static void SV_Maketic(void)
{
	int j;

	for(j = 0; j < MAXNETNODES; j++)
		if(playerpernode[j])
		{
			int player = nodetoplayer[j];
			if((netcmds[maketic%BACKUPTICS][player].angleturn & TICCMD_RECEIVED) == 0)
			{ // we didn't receive this tic
				int i;

				DEBFILE(va("MISS tic%4u for node %d\n", maketic, j));
#ifdef PARANOIA
				if(devparm)
					CONS_Printf("\2Client Misstic %d\n", maketic);
#endif
				// copy the old tic
				for(i = 0; i < playerpernode[j]; i++, player = nodetoplayer2[j])
				{
					netcmds[maketic%BACKUPTICS][player] = netcmds[(maketic-1)%BACKUPTICS][player];
					netcmds[maketic%BACKUPTICS][player].angleturn &= ~TICCMD_RECEIVED;
				}
			}
		}

	// all tic are now proceed make the next
	maketic++;
}

void TryRunTics(tic_t realtics)
{
	// the machine has lagged but it is not so bad
	if(realtics > TICRATE/7) // FIXME: consistency failure!!
	{
		if(server)
			realtics = 1;
		else
			realtics = TICRATE/7;
	}

	if(singletics)
		realtics = 1;

	if(realtics >= 1)
	{
		COM_BufExecute();
		if(mapchangepending)
			D_MapChange(-1, 0, 0, 0, 2, false); // finish the map change
	}

	NetUpdate();

	if(demoplayback)
	{
		neededtic = gametic + realtics + cv_playdemospeed.value;
		// start a game after a demo
		maketic += realtics;
		firstticstosend = maketic;
		tictoclear = firstticstosend;
	}

	GetPackets();

#ifdef DEBUGFILE
	if(debugfile && (realtics || neededtic>gametic))
	{
		//SoM: 3/30/2000: Need long int in the format string for args 4 & 5.
		//Shut up stupid warning!
		fprintf(debugfile, "------------ Tryruntic: REAL:%li NEED:%li GAME:%li LOAD: %i\n",
			realtics, neededtic, gametic, debugload);
		debugload = 100000;
	}
#endif

	if(neededtic > gametic)
	{
		if(advancedemo)
			D_StartTitle();
		else
			// run the count * tics
			while(neededtic > gametic)
			{
				DEBFILE(va("============ Running tic %u (local %d)\n", gametic, localgametic));

				G_Ticker();
				ExtraDataTicker();
				gametic++;
				// skip paused tic in a demo
				if(demoplayback && paused)
					neededtic++;
				else
					consistancy[gametic%BACKUPTICS] = Consistancy();
			}
	}
}

void NetUpdate(void)
{
	static tic_t gametime = 0;
	tic_t nowtime;
	int i;
	int realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if(realtics <= 0) // nothing new to update
		return;
	if(realtics > 5)
	{
		if(server)
			realtics = 1;
		else
			realtics = 5;
	}

	gametime = nowtime;

	if(!server)
		maketic = neededtic;

	Local_Maketic(realtics); // make local tic, and call menu?
	if(server && !demoplayback && !dedicated )
		CL_SendClientCmd(); // send it
	GetPackets(); // get packet from client or from server

	// client send the command after a receive of the server
	// the server send before because in single player is beter

	if(!server)
		CL_SendClientCmd(); // send tic cmd
	else
	{
		// acking the master server
		if(cv_internetserver.value)
			SendPingToMasterServer();

		if(!demoplayback)
		{
			int counts;

			firstticstosend = gametic;
			for(i = 0; i < MAXNETNODES; i++)
				if(nodeingame[i] && nettics[i] < firstticstosend)
					firstticstosend = nettics[i];

			// Don't erase tics not acknowledged
			counts = realtics;

			if(maketic + counts >= firstticstosend + BACKUPTICS)
				counts = firstticstosend+BACKUPTICS-maketic-1;

			for(i = 0; i < counts; i++)
				SV_Maketic(); // create missed tics and increment maketic

			for(; tictoclear < firstticstosend; tictoclear++) // clear only when acknoledged
				D_Clearticcmd(tictoclear);                    // clear the maketic the new tic

			SV_SendTics();

			neededtic = maketic; // the server is a client too
		}
	}
	Net_AckTicker();
	M_Ticker();
	CON_Ticker();
	FiletxTicker();
}
