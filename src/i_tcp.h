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
/// \brief TCP driver, sockets.

#ifndef __I_TCP__
#define __I_TCP__

extern int init_tcp_driver;
extern unsigned short sock_port;

boolean I_InitTcpNetwork(void);
// temporary addition for master server
void I_InitTcpDriver(void);
void I_ShutdownTcpDriver(void);

boolean I_Ban(int node);
void I_ClearBans(void);

extern boolean bannednode[MAXNETNODES + 1];

#endif
