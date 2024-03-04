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
/// \brief TCP driver, socket code.
/// 
///	This is not really OS-dependent because all OSes have the same socket API.
///	Just use ifdef for OS-dependent parts.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#ifdef __OS2__
#include <sys/types.h>
#include <sys/time.h>
#endif // __OS2__

#include "doomdef.h"

#if defined(NOMD5) && !defined(NONET)
#define NONET
#endif

#if !defined(NONET) && !defined(NOIPX)
#define USEIPX //Alam: Remline to turn off IPX support
#endif

#ifndef NONET
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(_WIN64)
#include <winsock.h>
#ifdef USEIPX
#include <wsipx.h>
#endif
#else
#if !defined(SCOUW2) && !defined(SCOUW7) && !defined(__OS2__)
#include <arpa/inet.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#define STD_STRING_LEN 256 // Just some standard length for a char string

#ifdef LINUX
	#include <sys/time.h>
	#ifdef __GLIBC__
		#include <netipx/ipx.h>
	#else
		#ifdef USEIPX
			#ifdef FREEBSD
				#include <netipx/ipx.h>
			#elif defined(__CYGWIN__)
				#include <wsipx.h>
			#else
				#include <linux/ipx.h>
			#endif
		#endif // USEIPX
	#endif // glibc
	#ifndef __CYGWIN__
	typedef struct sockaddr_ipx SOCKADDR_IPX, *PSOCKADDR_IPX;
	#endif
#endif // linux
#endif // win32

#if defined(_WIN32_WCE) || defined(_WIN32) || defined(_WIN64)
	// some undefined under win32
	#undef errno
	//#define errno WSAGetLastError() //Alam_GBC: this is the correct way, right?
	#define errno h_errno // some very strange things happen when not using h_error?!?
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define EMSGSIZE WSAEMSGSIZE
	#define ECONNREFUSED WSAECONNREFUSED
	#ifndef IOC_VENDOR
	#define IOC_VENDOR 0x18000000
	#endif
	#ifndef _WSAIOW
	#define _WSAIOW(x,y) (IOC_IN|(x)|(y))
	#endif
	#ifndef SIO_UDP_CONNRESET
	#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
	#endif
#endif

#ifdef __DJGPP__
#ifdef WATTCP // Alam_GBC: Wattcp may need this
#include <tcp.h>
#define strerror strerror_s
#else // wattcp
#include <lsck/lsck.h>
#endif // libsocket
#endif // djgpp

#ifdef USEIPX

#if defined(__DJGPP__) || defined(__OS2__)
// ipx not yet supported in libsocket (cut and pasted from wsipx.h (winsock)
typedef struct sockaddr_ipx
{
	short sa_family;
	char sa_netnum[4];
	char sa_nodenum[6];
	unsigned short sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX;
#define NSPROTO_IPX 1000
#endif

#ifndef AF_IPX
#define AF_IPX 23 // Novell Internet Protocol
#endif

#ifndef PF_IPX
#define PF_IPX AF_IPX
#endif

#ifndef NSPROTO_IPX
#define NSPROTO_IPX PF_IPX
#endif

#endif

typedef union
{
	struct sockaddr_in ip;
#ifdef USEIPX
	struct sockaddr_ipx ipx;
#endif
} mysockaddr_t;

#endif // !NONET

static int ipx = 0;

#define MAXBANS 20

#include "i_system.h"
#include "i_net.h"
#include "d_net.h"
#include "i_tcp.h"
#ifndef NONET
static mysockaddr_t clientaddress[MAXNETNODES+1];
static boolean nodeconnected[MAXNETNODES+1];
static mysockaddr_t banned[MAXBANS];
#endif
#include "m_argv.h"

#include "doomstat.h"

// win32 or djgpp
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__DJGPP__) || defined(_WIN64)
	// winsock stuff (in winsock a socket is not a file)
	#define ioctl ioctlsocket
	#define close closesocket

	#ifdef _WIN32_WCE
	#include "sdl/SRB2CE/cehelp.h"
	#endif

#endif

#ifndef IPPORT_USERRESERVED
#define IPPORT_USERRESERVED 5000
#endif

#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_WIN32)) && !defined(NONET)
typedef SOCKET SOCKET_TYPE;
#define BADSOCKET INVALID_SOCKET
#define ERRSOCKET (SOCKET_ERROR)
#else
typedef unsigned int SOCKET_TYPE;
#define BADSOCKET (SOCKET_TYPE)(~0)
#define ERRSOCKET (-1)
#endif

static SOCKET_TYPE mysocket = BADSOCKET;

static int numbans = 0;
boolean bannednode[MAXNETNODES+1]; /// \note do we really need the +1?
int init_tcp_driver = 0;

unsigned short sock_port = (IPPORT_USERRESERVED + 0x1d); // 5029

#ifdef WATTCP
static void wattcp_outch(char s)
{
	static char old = '\0';
	char pr[2] = {s,0};
	if(s == old && old == ' ') return;
	else old = s;
	if(s == '\r') CONS_Printf("\n");
	else if(s != '\n') CONS_Printf(pr);
}
#endif

#ifndef NONET
static char* SOCK_AddrToStr(mysockaddr_t* sk)
{
	static char s[50];
	if(sk->ip.sin_family == AF_INET)
	{
		sprintf(s,"%d.%d.%d.%d:%d",
			((byte*)(&(sk->ip.sin_addr.s_addr)))[0],
			((byte*)(&(sk->ip.sin_addr.s_addr)))[1],
			((byte*)(&(sk->ip.sin_addr.s_addr)))[2],
			((byte*)(&(sk->ip.sin_addr.s_addr)))[3],
			ntohs(sk->ip.sin_port));
	}
#ifdef USEIPX
	else
#if defined(LINUX) && !defined(__CYGWIN__)
	if(sk->ipx.sipx_family == AF_IPX)
	{
#ifndef FREEBSD
		sprintf(s,"%08x.%02x%02x%02x%02x%02x%02x:%d", sk->ipx.sipx_network,
			(byte)sk->ipx.sipx_node[0],
			(byte)sk->ipx.sipx_node[1],
			(byte)sk->ipx.sipx_node[2],
			(byte)sk->ipx.sipx_node[3],
			(byte)sk->ipx.sipx_node[4],
			(byte)sk->ipx.sipx_node[5],
			sk->ipx.sipx_port);
#else
		sprintf(s, "%s", ipx_ntoa(sk->ipx.sipx_addr));
#endif
	}
#else
	if(sk->ipx.sa_family == AF_IPX)
	{
		sprintf(s, "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%d",
			(byte)sk->ipx.sa_netnum[0],
			(byte)sk->ipx.sa_netnum[1],
			(byte)sk->ipx.sa_netnum[2],
			(byte)sk->ipx.sa_netnum[3],
			(byte)sk->ipx.sa_nodenum[0],
			(byte)sk->ipx.sa_nodenum[1],
			(byte)sk->ipx.sa_nodenum[2],
			(byte)sk->ipx.sa_nodenum[3],
			(byte)sk->ipx.sa_nodenum[4],
			(byte)sk->ipx.sa_nodenum[5],
			sk->ipx.sa_socket);
	}
#endif // linux
#endif // USEIPX
	else
		sprintf(s, "Unknown type");
	return s;
}
#endif

#ifdef USEIPX
static boolean IPX_cmpaddr(mysockaddr_t* a, mysockaddr_t* b)
{
#if defined(LINUX) && !defined(__CYGWIN__)
#ifdef FREEBSD
	return ipx_neteq(a->ipx.sipx_addr, b->ipx.sipx_addr)
		&& ipx_hosteq(a->ipx.sipx_addr, b->ipx.sipx_addr);
#else
	return ((!memcmp(&(a->ipx.sipx_network), &(b->ipx.sipx_network), 4))
		&& (!memcmp(&(a->ipx.sipx_node), &(b->ipx.sipx_node), 6)));
#endif
#else
	return ((!memcmp(&(a->ipx.sa_netnum), &(b->ipx.sa_netnum), 4))
		&& (!memcmp(&(a->ipx.sa_nodenum), &(b->ipx.sa_nodenum), 6)));
#endif // linux
}
#endif // USEIPX

#ifndef NONET
static boolean UDP_cmpaddr(mysockaddr_t* a, mysockaddr_t* b)
{
	return (a->ip.sin_addr.s_addr == b->ip.sin_addr.s_addr && a->ip.sin_port == b->ip.sin_port);
}

static boolean (*SOCK_cmpaddr)(mysockaddr_t* a, mysockaddr_t* b);

static signed char getfreenode(void)
{
	signed char j;

	for(j = 0; j < MAXNETNODES; j++)
		if(!nodeconnected[j])
		{
			nodeconnected[j] = true;
			return j;
		}
	return -1;
}
#endif

#ifndef NONET
static void SOCK_Get(void)
{
	int i, j, c;
#if defined (WATTCP) || defined(_WIN32) || defined(__APPLE_CC__) || defined(_WIN64)
	int fromlen;
#else
	size_t fromlen;
#endif
	mysockaddr_t fromaddress;

	fromlen = sizeof(fromaddress);
	c = recvfrom(mysocket, (char*)&doomcom->data, MAXPACKETLENGTH, 0,
		(struct sockaddr*)&fromaddress, &fromlen);
	if(c == ERRSOCKET)
	{
		if((errno == EWOULDBLOCK) || (errno == EMSGSIZE) || (errno == ECONNREFUSED))
		{
			doomcom->remotenode = -1; // no packet
			return;
		}
#if defined(_WIN32) || defined(_WIN64)
		else if(errno == WSAECONNRESET) // 2k has some extra errors
		{
			DEBFILE("Connection reset (likely that the server isn't running)\n"); //Alam_GBC: how about DEBFILE instead of annoying the user?
			//D_QuitNetGame(); // Graue 07-04-2004: win32 only and quit
			doomcom->remotenode = -1;      // no packet too
			return;
			/// \todo see if the D_QuitNetGame actually fixes it, or whether it crashes or something
			/// Alam_GBC: this WSAECONNRESET happends alot when talking to a masterlist server, i am guess when talking too much at a time
			/// Later, hmmm, SIO_UDP_CONNRESET turned off should fix this
		}
#endif
		I_Error("SOCK_Get: %s\n", strerror(errno));
	}

	// find remote node number
	for(i = 0; i < MAXNETNODES; i++)
		if(SOCK_cmpaddr(&fromaddress, &(clientaddress[i])))
		{
			doomcom->remotenode = (short)i; // good packet from a game player
			doomcom->datalength = (short)c;
			return;
		}

	// not found

	// find a free slot
	j = getfreenode();
	if(j > 0)
	{
		memcpy(&clientaddress[j], &fromaddress, fromlen);
		DEBFILE(va("New node detected: node:%d address:%s\n", j,
				SOCK_AddrToStr(&clientaddress[j])));
		doomcom->remotenode = (short)j; // good packet from a game player
		doomcom->datalength = (short)c;

		// check if it's a banned dude so we can send a refusal later
		for(i = 0; i < numbans; i++)
			if(SOCK_cmpaddr(&fromaddress, &banned[i]))
			{
				bannednode[j] = true;
				DEBFILE("This dude has been banned\n");
				break;
			}
		if(i == numbans)
			bannednode[j] = false;
		return;
	}

	DEBFILE("New node detected: No more free slots\n");
	doomcom->remotenode = -1; // no packet
}
#endif

// check if we can send (do not go over the buffer)
#ifndef NONET

static fd_set set;

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__CYGWIN__) || defined(_WIN64) //|| defined(WATTCP)
static boolean SOCK_CanSend(void)
{
	static struct timeval timeval_for_select = {0, 0};
	// huh Boris, are you sure about the 1th argument:
	// it is the highest-numbered descriptor in any of the three
	// sets, plus 1 (I suppose mysocket + 1).
	// BP:ok, no prob since it is ignored in windows :)
	int rselect = select((int)(mysocket + 1), NULL, &set, NULL, &timeval_for_select);
	return (boolean)( rselect != ERRSOCKET && rselect != 0);
}
#endif
#endif

#ifndef NONET
static void SOCK_Send(void)
{
	int c;

	if(!nodeconnected[doomcom->remotenode])
		return;

	c = sendto(mysocket, (char*)&doomcom->data, doomcom->datalength, 0,
		(struct sockaddr*)&clientaddress[doomcom->remotenode], sizeof(struct sockaddr));

	if(c == ERRSOCKET && errno != ECONNREFUSED && errno != EWOULDBLOCK)
		I_Error("SOCK_Send sending to node %d (%s): %s", doomcom->remotenode,
			SOCK_AddrToStr(&clientaddress[doomcom->remotenode]), strerror(errno));
}
#endif

#ifndef NONET
static void SOCK_FreeNodenum(int numnode)
{
	// can't disconnect from self :)
	if(!numnode)
		return;

	DEBFILE(va("Free node %d (%s)\n", numnode, SOCK_AddrToStr(&clientaddress[numnode])));

	nodeconnected[numnode] = false;

	// put invalid address
	memset(&clientaddress[numnode], 0, sizeof(clientaddress[numnode]));
}
#endif

//
// UDPsocket
//
#ifndef NONET
static SOCKET_TYPE UDP_Socket(void)
{
	SOCKET_TYPE s = BADSOCKET;
	struct sockaddr_in address;
#ifdef WATTCP
	char trueval = true;
#else
	unsigned long trueval =true;
#endif
	int i;
#if defined (WATTCP) || defined(_WIN32) || defined(__APPLE_CC__) || defined(_WIN64)
	int j;
#else
	size_t j;
#endif

	// allocate a socket
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == (SOCKET_TYPE)ERRSOCKET || s == BADSOCKET)
		I_Error("UDP_Socket: Can't create socket: %s", strerror(errno));

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;

	//Hurdler: I'd like to put a server and a client on the same computer
	//BP: in fact for client we can use any free port we want i have read
	//    in some doc that connect in udp can do it for us...
	if(M_CheckParm("-clientport"))
	{
		if(!M_IsNextParm())
			I_Error("syntax: -clientport <portnum>");
		address.sin_port = htons((unsigned short)atoi(M_GetNextParm()));
	}
	else
		address.sin_port = htons(sock_port);

	if(bind(s, (struct sockaddr*)&address, sizeof(address)) == ERRSOCKET)
	{
#if defined(_WIN32) || defined(_WIN64)
		if(errno == 10048)
			I_Error("UDP_Socket error: The address and port SRB2 had attempted to bind to is already in use.\n"
				"\nThis isn't a normal error, and probably indicates that something network-related\n"
				"on your computer is configured improperly.");
#endif
		I_Error("UDP_Socket error: %s", strerror(errno));
	}

	// make it non blocking
	ioctl(s, FIONBIO, &trueval);
#if defined(_WIN32) || defined(_WIN64)
	{
		unsigned long falseval = false; // Alam_GBC: disable the new UDP connection reset behavior for Win2k and up
		ioctl(s, SIO_UDP_CONNRESET, &falseval);
	}
#endif

	// make it broadcastable
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&trueval, sizeof(trueval));

	j = sizeof(i);
	getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&i, &j); // FIXME: so an int value is written to a (char *); portability!!!!!!!
	CONS_Printf("Network system buffer: %dKb\n", i>>10);

	if(i < 64<<10) // 64k
	{
		i = 64<<10;
		if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&i, sizeof(i)))
			CONS_Printf("Can't set buffer length to 64k, file transfer will be bad\n");
		else
			CONS_Printf("Network system buffer set to: %dKb\n",i>>10);
	}

	// ip + udp
	packetheaderlength = 20 + 8; // for stats

	clientaddress[0].ip.sin_family = AF_INET;
	clientaddress[0].ip.sin_port = htons(sock_port);
	clientaddress[0].ip.sin_addr.s_addr = INADDR_LOOPBACK; //GetLocalAddress(); // my own ip
	// setup broadcast adress to BROADCASTADDR entry
	clientaddress[BROADCASTADDR].ip.sin_family = AF_INET;
	clientaddress[BROADCASTADDR].ip.sin_port = htons(sock_port);
	clientaddress[BROADCASTADDR].ip.sin_addr.s_addr = INADDR_BROADCAST;

	doomcom->extratics = 1; // internet is very high ping

	SOCK_cmpaddr = UDP_cmpaddr;
	return s;
}
#endif

#ifdef USEIPX
static SOCKET_TYPE IPX_Socket(void)
{
	SOCKET_TYPE s = BADSOCKET;
	SOCKADDR_IPX address;
#ifdef WATTCP
	char trueval = true;
#else
	unsigned long trueval = true;
#endif
	int i;
#if defined(__APPLE_CC__) || defined(WATTCP) || defined(_WIN32) || defined(_WIN64)
	int j;
#else
	size_t j;
#endif

	// allocate a socket
	s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
	if(s == (SOCKET_TYPE)ERRSOCKET || s == BADSOCKET)
		I_Error("IPX_socket: Can't create socket: %s", strerror(errno));

	memset(&address, 0, sizeof(address));
#if defined(LINUX) && !defined(__CYGWIN__)
	address.sipx_family = AF_IPX;
	address.sipx_port = htons(sock_port);
#else
	address.sa_family = AF_IPX;
	address.sa_socket = htons(sock_port);
#endif // linux
	if(bind(s, (struct sockaddr*)&address, sizeof(address)) == ERRSOCKET)
		I_Error("IPX_Bind: %s", strerror(errno));

	// make it non blocking
	ioctl(s, FIONBIO, &trueval);

	// make it broadcastable
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&trueval, sizeof(trueval));

	// set receive buffer to 64Kb
	j = sizeof(i);
	getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&i, &j);
	CONS_Printf("Network system receive buffer: %dKb\n",i>>10);
	if(i < 128<<10)
	{
		i = 64<<10;
		if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&i, sizeof(i)))
			CONS_Printf("Can't set receive buffer length to 64k, file transfer will be bad\n");
		else
			CONS_Printf("Network system receive buffer set to: %dKb\n",i>>10);
	}

	// ipx header
	packetheaderlength = 30; // for stats

	// setup broadcast adress to BROADCASTADDR entry
#if defined(LINUX) && !defined(__CYGWIN__)
	clientaddress[BROADCASTADDR].ipx.sipx_family = AF_IPX;
	clientaddress[BROADCASTADDR].ipx.sipx_port = htons(sock_port);
#ifndef FREEBSD
	clientaddress[BROADCASTADDR].ipx.sipx_network = 0;
	for(i = 0; i < 6; i++)
		clientaddress[BROADCASTADDR].ipx.sipx_node[i] = (byte)0xFF;
#else
	clientaddress[BROADCASTADDR].ipx.sipx_addr.x_net.s_net[0] = 0;
	clientaddress[BROADCASTADDR].ipx.sipx_addr.x_net.s_net[1] = 0;
	for(i = 0; i < 6; i++)
		clientaddress[BROADCASTADDR].ipx.sipx_addr.x_host.c_host[i] = (byte)0xFF;
#endif
#else
	clientaddress[BROADCASTADDR].ipx.sa_family = AF_IPX;
	clientaddress[BROADCASTADDR].ipx.sa_socket = htons(sock_port);
	for(i = 0; i < 4; i++)
		clientaddress[BROADCASTADDR].ipx.sa_netnum[i] = 0;
	for(i = 0; i < 6; i++)
		clientaddress[BROADCASTADDR].ipx.sa_nodenum[i] = (byte)0xFF;
#endif // linux
	SOCK_cmpaddr = IPX_cmpaddr;
	return s;
}
#endif // USEIPX

void I_InitTcpDriver(void)
{
#ifndef NONET
	if(!init_tcp_driver)
	{
#if defined(_WIN32) || defined(_WIN64)
		WSADATA winsockdata;
		if(WSAStartup(MAKEWORD(1,1),&winsockdata))
			I_Error("No Tcp/Ip driver detected");
#endif
#ifdef __DJGPP__
#ifdef WATTCP // Alam_GBC: survive bootp, dhcp, rarp and wattcp/pktdrv from failing to load
		survive_eth   = 1; // would be needed to not exit if pkt_eth_init() fails
		survive_bootp = 1; // ditto for BOOTP
		survive_dhcp  = 1; // ditto for DHCP/RARP
		survive_rarp  = 1;
		//_watt_do_exit = false;
		//_watt_handle_cbreak = false;
		//_watt_no_config = true;
		_outch = wattcp_outch;
		init_misc();
//#ifdef DEBUGFILE
		dbug_init();
//#endif
		switch(sock_init())
		{
			case 0:
				init_tcp_driver = 1;
				break;
			case 3:
				I_Error("No packet driver detected");
				break;
			case 4:
				I_Error("Error while talking to packet driver");
				break;
			case 5:
				I_Error("BOOTP failed");
				break;
			case 6:
				I_Error("DHCP failed");
				break;
			case 7:
				I_Error("RARP failed");
				break;
			case 8:
				I_Error("TCP/IP failed");
				break;
			case 9:
				I_Error("PPPoE login/discovery failed");
				break;
			default:
				I_Error("Unknown error with TCP/IP stack");
				break;
		}
		hires_timer(0);
#else // wattcp
		if(__lsck_init())
			init_tcp_driver = 1;
		else
			I_Error("No Tcp/Ip driver detected");
#endif // libsocket
#endif // __DJGPP__
#ifndef __DJGPP__
		init_tcp_driver = 1;
#endif
	}
#endif
}

#ifndef NONET
static void SOCK_CloseSocket(void)
{
	if(mysocket != (SOCKET_TYPE)ERRSOCKET && mysocket != BADSOCKET)
	{
// quick fix bug in libsocket 0.7.4 beta 4 under winsock 1.1 (win95)
#if !defined(__DJGPP__) || defined(WATTCP)
		close(mysocket);
#endif
	}
	mysocket = BADSOCKET;
}
#endif

void I_ShutdownTcpDriver(void)
{
#ifndef NONET
	SOCK_CloseSocket();

	if(init_tcp_driver)
	{
		CONS_Printf("I_ShutdownTcpDriver: ");
#if defined(_WIN32) || defined(_WIN64)
		WSACleanup();
#endif
#ifdef __DJGPP__
#ifdef WATTCP // wattcp
		//_outch = NULL;
		sock_exit();
#else
		__lsck_uninit();
#endif // libsocket
#endif // __DJGPP__
		CONS_Printf("shut down\n");
		init_tcp_driver = 0;
	}
#endif
}

#ifndef NONET
static signed char SOCK_NetMakeNode(const char *hostname)
{
	signed char newnode;
	char* localhostname = strdup(hostname);
	char* portchar;
	unsigned short portnum = htons(sock_port);

	// retrieve portnum from address!
	strtok(localhostname, ":");
	portchar = strtok(NULL, ":");
	if(portchar)
		portnum = htons((unsigned short)atoi(portchar));
	free(localhostname);

	// server address only in ip
#ifdef USEIPX
	if(ipx) // ipx only
		return BROADCASTADDR;
	else // tcp/ip
#endif
	{
		struct hostent* hostentry; // host information entry
		char* t;

		// remove the port in the hostname as we've it already
		t = localhostname = strdup(hostname);
		while((*t != ':') && (*t != '\0'))
			t++;
		*t = '\0';

		newnode = getfreenode();
		if(newnode == -1)
			return -1;
		// find ip of the server
		clientaddress[newnode].ip.sin_family = AF_INET;
		clientaddress[newnode].ip.sin_port = portnum;
		clientaddress[newnode].ip.sin_addr.s_addr = inet_addr(localhostname);

		if(clientaddress[newnode].ip.sin_addr.s_addr == INADDR_NONE) // not a ip ask to the dns
		{
			CONS_Printf("Resolving %s\n",localhostname);
			hostentry = gethostbyname(localhostname);
			if(!hostentry)
			{
				CONS_Printf("%s unknown\n", localhostname);
				I_NetFreeNodenum(newnode);
				free(localhostname);
				return -1;
			}
			clientaddress[newnode].ip.sin_addr.s_addr = *(int*)hostentry->h_addr_list[0];
		}
		CONS_Printf("Resolved %s\n",
			inet_ntoa(*(struct in_addr*)&clientaddress[newnode].ip.sin_addr.s_addr));
		free(localhostname);

		return newnode;
	}
}
#endif

static boolean SOCK_OpenSocket(void)
{
#ifndef NONET
	int i;

	memset(clientaddress, 0, sizeof(clientaddress));

	for(i = 0; i < MAXNETNODES; i++)
		nodeconnected[i] = false;

	nodeconnected[0] = true; // always connected to self
	nodeconnected[BROADCASTADDR] = true;
	I_NetSend = SOCK_Send;
	I_NetGet = SOCK_Get;
	I_NetCloseSocket = SOCK_CloseSocket;
	I_NetFreeNodenum = SOCK_FreeNodenum;
	I_NetMakeNode = SOCK_NetMakeNode;

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__CYGWIN__) || defined(_WIN64) //|| defined(WATTCP)
	// seem like not work with libsocket nor linux :(
	I_NetCanSend = SOCK_CanSend;
#endif

	// build the socket
#ifdef USEIPX
	if(ipx)
	{
		mysocket = IPX_Socket();
		net_bandwidth = 800000;
		hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
	}
	else
#endif // USEIPX
		mysocket = UDP_Socket();
	// for select
	FD_ZERO(&set);
	FD_SET(mysocket,&set);
#endif
	return (boolean)(mysocket != (SOCKET_TYPE)ERRSOCKET && mysocket != BADSOCKET);
}

boolean I_Ban(int node)
{
#ifdef NONET
	node = 0;
#else
	if(numbans == MAXBANS)
		return false;

	memcpy(&banned[numbans], &clientaddress[node], sizeof(mysockaddr_t));
	numbans++;
#endif
	return true;
}

void I_ClearBans(void)
{
	numbans = 0;
}

boolean I_InitTcpNetwork(void)
{
	char serverhostname[255];
	boolean ret = false;
#ifdef USEIPX
	ipx = M_CheckParm("-ipx");
#endif
	// initilize the driver
	I_InitTcpDriver();
	I_AddExitFunc(I_ShutdownTcpDriver);
	if(!init_tcp_driver)
		return false;

	if(M_CheckParm("-udpport"))
		sock_port = (unsigned short)atoi(M_GetNextParm());

	// parse network game options,
	if(M_CheckParm("-server") || dedicated)
	{
		server = true;

		// If a number of clients (i.e. nodes) is specified, the server will wait for the clients
		// to connect before starting.
		// If no number is specified here, the server starts with 1 client, and others can join
		// in-game.
		// Since Boris has implemented join in-game, there is no actual need for specifying a
		// particular number here.
		// FIXME: for dedicated server, numnodes needs to be set to 0 upon start
		if(M_IsNextParm())
			doomcom->numnodes = (short)atoi(M_GetNextParm());
		else if(dedicated)
			doomcom->numnodes = 0;
		else
			doomcom->numnodes = 1;

		if(doomcom->numnodes < 0)
			doomcom->numnodes = 0;
		if(doomcom->numnodes > MAXNETNODES)
			doomcom->numnodes = MAXNETNODES;

		// server
		servernode = 0;
		// FIXME:
		// ??? and now ?
		// server on a big modem ??? 4*isdn
		net_bandwidth = 16000;
		hardware_MAXPACKETLENGTH = INETPACKETLENGTH;

		ret = true;
	}
	else if(M_CheckParm("-connect"))
	{
		if(M_IsNextParm())
			strcpy(serverhostname, M_GetNextParm());
		else
			serverhostname[0] = 0; // assuming server in the LAN, use broadcast to detect it

		// server address only in ip
		if(serverhostname[0] && !ipx)
		{
			COM_BufAddText("connect \"");
			COM_BufAddText(serverhostname);
			COM_BufAddText("\"\n");

			// probably modem
			hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
		}
		else
		{
			// so we're on a LAN
			COM_BufAddText("connect any\n");

			net_bandwidth = 800000;
			hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
		}
	}

	I_NetOpenSocket = SOCK_OpenSocket;

	return ret;
}
