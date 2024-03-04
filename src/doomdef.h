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
/// \brief  Internally used data structures for virtually everything,
///	key definitions, lots of other stuff.

#ifndef __DOOMDEF__
#define __DOOMDEF__

#ifdef _WINDOWS
#if !defined (HWRENDER) && !defined(NOHW)
#define HWRENDER
#endif
#endif

// judgecutor: 3D sound support
#if defined(_WINDOWS) && !defined(NOHS)
#define HW3SOUND
#endif

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(_WIN64)
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef _MSC_VER
#pragma warning(disable :  4127 4152 4213)
#endif
// warning level 4
// warning C4127: conditional expression is constant
// warning C4152: nonstandard extension, function/data pointer conversion in expression
// warning C4213: nonstandard extension used : cast on l-value

#if defined(_WIN32_WCE) && defined(DEBUG) && defined(ARM)
#if defined(ARMV4) || defined(ARMV4I)
//#pragma warning(disable : 1166)
// warning LNK1166: cannot adjust code at offset=
#endif
#endif


#include "doomtype.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#if !defined(_WIN32_WCE)
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <ctype.h>

#if (defined(_WIN32) && !defined(_WIN32_WCE)) || defined(__DJGPP__) || defined(_WIN64)
#include <io.h>
#endif

#ifdef PC_DOS
#include <conio.h>
#endif

//#define NOMD5

// Uncheck this to compile debugging code
//#define RANGECHECK
//#ifndef PARANOIA
//#define PARANOIA // do some tests that never fail but maybe
// turn this on by make etc.. DEBUGMODE=1 or use the Debug profile in the VC++ projects
//#endif
#if defined(_WIN32) || defined(SDLIO) || defined(LINUX) || defined(__MACH__) || defined(_WIN64)
#define LOGMESSAGES // write message in log.txt (win32 and Linux only for the moment)
#endif

#if defined(LOGMESSAGES) && !defined(_WINDOWS)
#undef INVALID_HANDLE_VALUE
#ifdef SDLIO
#define INVALID_HANDLE_VALUE NULL
#include <SDL/SDL_rwops.h>
extern SDL_RWops* logstream;
#else
#define INVALID_HANDLE_VALUE -1
extern int logstream;
#endif
#endif

#define VERSION 109 // Game version
#define VERSIONSTRING " v1.09"

// some tests, enable or disable it if it run or not
#define SPLITSCREEN
//#define CLIENTPREDICTION2 // different method
#define NEWLIGHT // compute lighting with bsp (in construction)

//#define SLOPENESS // Fun experimental slope stuff!

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

#define MAXPLAYERS 32
#define MAXSKINS MAXPLAYERS
#define PLAYERSMASK (MAXPLAYERS-1)
#define MAXPLAYERNAME 21
#define MAXSKINCOLORS 16

#define SAVESTRINGSIZE 24

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define OLDTICRATE 35
#define NEWTICRATERATIO 1 // try 4 for 140 fps :)
#define TICRATE (OLDTICRATE*NEWTICRATERATIO)

#define RING_DIST 512*FRACUNIT // how close you need to be to a ring to attract it
#define NIGHTS_DRAW_DIST 1536*FRACUNIT

#define PUSHACCEL (2*FRACUNIT) // Acceleration for MF2_SLIDEPUSH items.

// Name of local directory for config files and savegames
#if (defined(LINUX) && !defined(__CYGWIN__)) && !defined(__MACH__)
#define DEFAULTDIR ".srb2"
#else
#define DEFAULTDIR "srb2"
#endif

#include "g_state.h"

// commonly used routines - moved here for include convenience

/**	\brief	The I_Error function

	\param	error	the error message

	\return	void

	
*/
void I_Error(const char *error, ...) FUNCIERROR;

// console.h
void CONS_Printf(const char *fmt, ...) FUNCPRINTF;

#include "m_swap.h"

// m_misc.h
char *va(const char *format, ...) FUNCPRINTF;
char *Z_StrDup(const char *in);

// d_main.c
extern boolean devparm; // development mode (-debug)

// =======================
// Misc stuff for later...
// =======================

// if we ever make our alloc stuff...
#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)

// debug me in color (v_video.c)
void IO_Color(unsigned char color, unsigned char r, unsigned char g, unsigned char b);

// i_system.c, replace getchar() once the keyboard has been appropriated
int I_GetKey(void);

#ifndef min // Double-Check with WATTCP-32's cdefs.h
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max // Double-Check with WATTCP-32's cdefs.h
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef W_OK
#define W_OK 2 // win32 does not have W_OK in includes
#endif
#ifndef R_OK
#define R_OK 4 // win32 does not have R_OK in includes
#endif

#endif // __DOOMDEF__
