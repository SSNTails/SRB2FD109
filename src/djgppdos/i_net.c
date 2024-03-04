// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_net.c,v 1.9 2000/10/16 20:02:30 bpereira Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
// $Log: I_net.c,v $
// Revision 1.9  2000/10/16 20:02:30  bpereira
// no message
//
// Revision 1.8  2000/09/28 20:57:20  bpereira
// no message
//
// Revision 1.7  2000/09/15 19:49:23  bpereira
// no message
//
// Revision 1.6  2000/09/01 19:34:37  bpereira
// no message
//
// Revision 1.5  2000/09/01 18:59:55  hurdler
// fix some issues with latest network code changes
//
// Revision 1.4  2000/08/31 14:30:57  bpereira
// no message
//
// Revision 1.3  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//-----------------------------------------------------------------------------
/// \file
/// \brief doomcom network interface


#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <dos.h>
#include <sys/nearptr.h>

#include "../doomdef.h"

#include "../i_system.h"
#include "../d_event.h"
#include "../d_net.h"
#include "../m_argv.h"

#include "../doomstat.h"
#include "../z_zone.h"
#include "../i_net.h"
#include "../i_tcp.h"

//
// NETWORKING
//

typedef enum
{
	CMD_SEND     = 1,
	CMD_GET      = 2,
} command_t;

static void External_Driver_Get(void);
static void External_Driver_Send(void);
static void External_Driver_FreeNode(int nodenum);

static inline boolean External_Driver_OpenSocket()
{
	I_NetGet  = External_Driver_Get;
	I_NetSend = External_Driver_Send;
	I_NetCloseSocket = NULL;
	I_NetFreeNodenum = External_Driver_FreeNode;
	
	return true;
}

//
// I_InitNetwork
//
boolean I_InitNetwork (void)
{
	int netgamepar;

	netgamepar = M_CheckParm ("-net");
	if(!netgamepar)
		return false;

	// externals drivers specific

	__djgpp_nearptr_enable();

	// set up for network
	doomcom=(doomcom_t *)(__djgpp_conventional_base+atoi(myargv[netgamepar+1]));
	CONS_Printf("I_DosNet : Using int 0x%x for communication\n",doomcom->intnum);

	server = (doomcom->consoleplayer == 0);
	if(!server)
		COM_BufAddText("connect any\n");

	// ipx + time + 4 (padding)
	packetheaderlength=30+4+4;

	hardware_MAXPACKETLENGTH = 512;
	
	I_NetOpenSocket = External_Driver_OpenSocket;
	return true;
}

FUNCNORETURN static void External_Driver_Get(void)
{
	I_Error("External_Driver_Get not supported at this time");
}

FUNCNORETURN static void External_Driver_Send(void)
{
	I_Error("External_Driver_Send not supported at this time");
}

FUNCNORETURN static void External_Driver_FreeNode(int nodenum)
{
	nodenum = 0;
	I_Error("External_Driver_FreeNode not supported at this time");
}
