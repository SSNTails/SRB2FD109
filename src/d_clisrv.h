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
/// \brief high level networking stuff

#ifndef __D_CLISRV__
#define __D_CLISRV__

#include "d_ticcmd.h"
#include "d_netcmd.h"
#include "tables.h"

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// more precise version number to compare in network
#define SUBVERSION 000

// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.

// Networking and tick handling related.
#define BACKUPTICS 32
#define MAXTEXTCMD 256
//
// Packet structure
//
typedef enum
{
	PT_NOTHING,       // To send a nop through the network. ^_~
	PT_SERVERCFG,     // Server config used in start game
	                  // (must stay 1 for backwards compatibility).
	                  // This is a positive response to a CLIENTJOIN request.
	PT_CLIENTCMD,     // Ticcmd of the client.
	PT_CLIENTMIS,     // Same as above with but saying resend from.
	PT_CLIENT2CMD,    // 2 cmds in the packet for splitscreen.
	PT_CLIENT2MIS,    // Same as above with but saying resend from
	PT_NODEKEEPALIVE, // Same but without ticcmd and consistancy
	PT_NODEKEEPALIVEMIS,
	PT_SERVERTICS,    // All cmds for the tic.
	PT_SERVERREFUSE,  // Server refuses joiner (reason inside).
	PT_SERVERSHUTDOWN,
	PT_CLIENTQUIT,    // Client closes the connection.

	PT_ASKINFO,       // Anyone can ask info of the server.
	PT_SERVERINFO,    // Send game & server info (gamespy).
	PT_REQUESTFILE,   // Client requests a file transfer

	PT_CANFAIL,       // This is kind of a priority. Anything bigger than CANFAIL
	                  // allows HSendPacket(,true,,) to return false.
	                  // In addition, this packet can't occupy all the available slots.

	PT_FILEFRAGMENT = PT_CANFAIL, // A part of a file.

	PT_TEXTCMD,       // Extra text commands from the client.
	PT_TEXTCMD2,      // Splitscreen text commands.
	PT_CLIENTJOIN,    // Client wants to join; used in start game.
	PT_NODETIMEOUT,   // Packet sent to self if the connection times out.
	NUMPACKETTYPE
} packettype_t;

// client to server packet
typedef struct
{
	byte client_tic;
	byte resendfrom;
	short consistancy;
	ticcmd_t cmd;
} clientcmd_pak;

// splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
typedef struct
{
	byte client_tic;
	byte resendfrom;
	short consistancy;
	ticcmd_t cmd, cmd2;
} client2cmd_pak;

// Server to client packet
// this packet is too large
typedef struct
{
	byte starttic;
	byte numtics;
	byte numplayers;
	ticcmd_t cmds[45]; // normally [BACKUPTIC][MAXPLAYERS] but too large
} servertics_pak;

typedef struct
{
	byte version; // different versions don't work
	ULONG subversion; // contains build version

	// server launch stuffs
	byte serverplayer;
	byte totalplayernum;
	tic_t gametic;
	byte clientnode;
	byte gamestate;

	ULONG playerdetected; // playeringame vector in bit field
	byte gametype;
	byte modifiedgame;
	byte netcvarstates[0];
} serverconfig_pak;

typedef struct
{
	byte version; // different versions don't work
	ULONG subversion; // contains build version
	byte localplayers;
	byte mode;
} clientconfig_pak;

typedef struct
{
	char fileid;
	ULONG position;
	USHORT size;
	byte data[100]; // size is variable using hardare_MAXPACKETLENGTH
} filetx_pak;

#define MAXSERVERNAME 32
typedef struct
{
	byte version;
	ULONG subversion;
	byte numberofplayer;
	byte maxplayer;
	byte deathmatch;
	byte gametype;
	byte modifiedgame;
	tic_t time;
	float load; // unused for the moment
	char mapname[8];
	char servername[MAXSERVERNAME];
	byte fileneedednum;
	byte fileneeded[4096]; // is filled with writexxx (byteptr.h)
} serverinfo_pak;

#define MAXSERVERLIST 32 // depends only on the display
typedef struct
{
	serverinfo_pak info;
	signed char node;
} serverelem_t;

extern serverelem_t serverlist[MAXSERVERLIST];
extern unsigned int serverlistcount;
extern int mapchangepending;

typedef struct
{
	byte version;
	tic_t time; // used for ping evaluation
} askinfo_pak;

typedef struct
{
	char reason[255];
} serverrefuse_pak;

//
// Network packet data.
//
typedef struct
{
	unsigned checksum;
	byte ack; // if not null the node asks for acknowledgement, the receiver must resend the ack
	byte ackreturn; // the return of the ack number

	byte packettype;
	byte reserved; // padding
	union
	{
		clientcmd_pak clientpak;
		client2cmd_pak client2pak;
		servertics_pak serverpak;
		serverconfig_pak servercfg;
		byte textcmd[MAXTEXTCMD+1];
		filetx_pak filetxpak;
		clientconfig_pak clientcfg;
		serverinfo_pak serverinfo;
		serverrefuse_pak serverrefuse;
		askinfo_pak askinfo;
	} u;
} doomdata_t;

// points inside doomcom
extern doomdata_t* netbuffer;

extern consvar_t cv_playdemospeed;

#define BASEPACKETSIZE ((size_t)&(((doomdata_t*)0)->u))
#define FILETXHEADER ((size_t)((filetx_pak*)0)->data)
#define BASESERVERTICSSIZE ((size_t)&(((doomdata_t*)0)->u.serverpak.cmds[0]))

#define KICK_MSG_GO_AWAY     1
#define KICK_MSG_CON_FAIL    2
#define KICK_MSG_PLAYER_QUIT 3
#define KICK_MSG_TIMEOUT     4
#define KICK_MSG_BANNED      5

extern boolean server;
extern boolean admin; // Remote Administration
extern boolean dedicated; // for dedicated server
extern USHORT software_MAXPACKETLENGTH;
extern boolean acceptnewnode;
extern signed char servernode;

extern consvar_t cv_disallownewplayer, cv_allownewplayer, cv_maxplayers, cv_maxsend;

// used in d_net, the only dependence
tic_t ExpandTics(int low);
void D_ClientServerInit(void);

// initialise the other field
void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(char **p, int playernum));
void SendNetXCmd(netxcmd_t id, void *param, size_t nparam);
void SendNetXCmd2(netxcmd_t id, void *param, size_t nparam); // splitsreen player

// Create any new ticcmds and broadcast to other players.
void NetUpdate(void);

boolean SV_AddWaitingPlayers(void);
void SV_StartSinglePlayerServer(void);
boolean SV_SpawnServer(void);
void SV_SpawnPlayer(int playernum, int x, int y, angle_t angle);
void SV_StopServer(void);
void SV_ResetServer(void);

void CL_AddSplitscreenPlayer(void);
void CL_RemoveSplitscreenPlayer(void);
void CL_Reset(void);
void CL_UpdateServerList(boolean internetsearch);
// is there a game running
boolean Playing(void);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame(void);

//? how many ticks to run?
void TryRunTics(tic_t realtic);

// extra data for lmps
boolean AddLmpExtradata(byte** demo_p, int playernum);
void ReadLmpExtraData(byte** demo_pointer, int playernum);

// translate a playername in a player number return -1 if not found and
// print a error message in the console
char nametonum(const char *name);

extern char adminpassword[9];

#endif
