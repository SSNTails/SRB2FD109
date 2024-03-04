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
/// \brief System specific network interface stuff.

#ifndef __I_NET__
#define __I_NET__

#ifdef __GNUG__
#pragma interface
#endif

#include "doomdef.h"
#include "command.h"

/// \brief program net id
#define DOOMCOM_ID 0x12345678l

/// \def MAXPACKETLENGTH
/// For use in a LAN
#define MAXPACKETLENGTH 1450
/// \def INETPACKETLENGTH
///  For use on the internet
#define INETPACKETLENGTH 512

extern short hardware_MAXPACKETLENGTH;
extern int   net_bandwidth; // in byte/s

typedef struct
{
	/// Supposed to be DOOMCOM_ID
	long id;

	/// SRB2 executes an int to execute commands.
	short intnum;
	/// Communication between SRB2 and the driver.
	/// Is CMD_SEND or CMD_GET.
	short command;
	/// Is dest for send, set by get (-1 = no packet).
	short remotenode;

	/// Number of bytes in doomdata to be sent
	short datalength;

	/// Info common to all nodes.
	/// Console is always node 0.
	short numnodes;
	/// Flag: 1 = no duplication, 2-5 = dup for slow nets.
	short ticdup;
	/// Flag: 1 = send a backup tic in every packet.
	short extratics;
	/// deathmatch type \deprecated obsolete
	short deathmatch;
	/// kind of game
	short gametype;
	/// Flag: -1 = new game, 0-5 = load savegame
	short savegame;
	/// currect map
	short map;
	/// skill level 1-5
	short skill;

	/// Info specific to this node.
	short consoleplayer;
	/// Number total of players
	short numplayers;

	/// The packet data to be sent.
	char data[MAXPACKETLENGTH];
} doomcom_t;

extern doomcom_t *doomcom;

/**	\brief return packet in doomcom struct
*/
extern void (*I_NetGet)(void);

/**	\brief send packet within doomcom struct
*/
extern void (*I_NetSend)(void);

/**	\brief ask to driver if all is ok to send data now
*/
extern boolean (*I_NetCanSend)(void);

/**	\brief	close a connection

	\param	nodenum	node to be closed

	\return	void

	
*/
extern void (*I_NetFreeNodenum)(int nodenum);

/**	\brief	open a connection with spécified address

	\param	address	address to connect to

	\return	number of node

	
*/
extern signed char (*I_NetMakeNode)(const char *address);

/**	\brief open connection
*/
extern boolean (*I_NetOpenSocket)(void);

/**	\brief close all connections no more allow geting any packet
*/
extern void (*I_NetCloseSocket)(void);

/// \brief Called by D_SRB2Main to be defined by extern network driver
boolean I_InitNetwork(void);

#endif
