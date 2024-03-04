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
/// \brief SRB2 standard types
/// 
///	Simple basic typedefs, isolated here to make it easier
///	separating modules.

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#if defined(_WIN32) || (defined(_WIN32_WCE) && !defined(__GNUC__)) || defined (_WIN64)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#ifndef _OS2EMX_H
#ifndef __MINGW32__
#ifndef __MINGW64__
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif
#endif
#endif // _OS2EMX_H

#if defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_C) || defined(__DECC)
#define INT64  long long
#elif defined(_MSC_VER)
#define INT64  __int64
#else
"Warning, need 64 bit type for this compiler"
#endif

#ifdef __APPLE_CC__
#define __MACOS__
#define DIRECTFULLSCREEN
#define DEBUG_LOG
#define HWRENDER
#define NOIPX
#endif

#if defined(_MSC_VER) || defined( __OS2__)
	// Microsoft VisualC++
#ifdef _MSC_VER
	#define vsnprintf               _vsnprintf
#endif
	#define strncasecmp             strnicmp
	#define strcasecmp              stricmp
	#define inline                  __inline
#elif defined(__WATCOMC__)
	#include <dos.h>
	#include <sys\types.h>
	#include <direct.h>
	#include <malloc.h>
	#define strncasecmp             strnicmp
	#define strcasecmp              strcmpi
#endif
// added for Linux 19990220 by Kin
#ifdef LINUX 
	#undef stricmp
	#define stricmp(x,y) strcasecmp(x,y)
	#undef strnicmp
	#define strnicmp(x,y,n) strncasecmp(x,y,n)
	#define lstrlen(x) strlen(x)
#endif
#ifdef _WIN32_WCE
#ifndef __GNUC__
	#define stricmp(x,y)            _stricmp(x,y)
	#define strnicmp                _strnicmp
#endif
	#define strdup                  _strdup
	#define strupr                  _strupr
	#define strlwr                  _strlwr
#endif

#ifdef __MACOS__                //skip all boolean/Boolean crap
	#define true 1
	#define false 0
	#define min(x,y) ( ((x)<(y)) ? (x) : (y) )
	#define max(x,y) ( ((x)>(y)) ? (x) : (y) )
	#define lstrlen(x) strlen(x)

	#define stricmp strcmp
	#define strnicmp strncmp

	#define __BYTEBOOL__
	typedef unsigned char byte;
	#define boolean int

	#ifndef O_BINARY
	#define O_BINARY 0
	#endif
#endif //__MACOS__

#if (defined (LINUX) && !defined(__CYGWIN__) && !defined(_arch_dreamcast)) || defined(__MACH__)
int strupr(char* n); // from dosstr.c
int strlwr(char* n); // from dosstr.c
#endif

#ifndef __BYTEBOOL__
	#define __BYTEBOOL__

	// Fixed to use builtin bool type with C++.
	//#ifdef __cplusplus
	//    typedef bool boolean;
	//#else
	#ifndef __MINGW32__
	#ifndef __MINGW64__
		typedef unsigned char byte;
	#endif
	#endif
	//faB: clean that up !!
	#if defined (_WIN32) || (defined(_WIN32_WCE) && !defined(__GNUC__)) || defined(_WIN64)
		#define false   FALSE           // use windows types
		#define true    TRUE
		#define boolean BOOL
	#else
		typedef enum {false, true} boolean;
	#endif
	//#endif // __cplusplus
#endif // __BYTEBOOL__

typedef ULONG tic_t;

// Predefined with some OS.
#ifndef _WIN32_WCE
#ifndef _WIN32
#ifndef _WIN64
#ifndef __MACOS__
#ifndef FREEBSD
#ifndef __CYGWIN__
#ifndef __OS2__
#ifndef _arch_dreamcast
	#include <values.h>
#else
	#include <limits.h>
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#ifdef MAXCHAR
#undef MAXCHAR
#endif
#ifdef MAXSHORT
#undef MAXSHORT
#endif
#ifdef MAXINT
#undef MAXINT
#endif
#ifdef MAXLONG
#undef MAXLONG
#endif
#ifdef MINCHAR
#undef MINCHAR
#endif
#ifdef MINSHORT
#undef MINSHORT
#endif
#ifdef MININT
#undef MININT
#endif
#ifdef MINLONG
#undef MINLONG
#endif

#define MAXCHAR  ((char)0x7f)
#define MAXSHORT ((short)0x7fff)
#define MAXINT   ((int)0x7fffffff)
#define MAXLONG  ((long)0x7fffffff)
#define MINCHAR  ((char)0x80)
#define MINSHORT ((short)-0x8000)
#define MININT   ((int)0x80000000)
#define MINLONG  ((long)0x80000000)

union FColorRGBA
{
	ULONG rgba;
	struct
	{
		byte red;
		byte green;
		byte blue;
		byte alpha;
	} s;
};
typedef union FColorRGBA RGBA_t;

#ifdef __BIG_ENDIAN__
#define UINT2RGBA(a) a
#else
#define UINT2RGBA(a) ((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((ULONG)a&0xff000000)>>24)
#endif

#ifdef __GNUC__ // __attribute__ ((X))
#define FUNCPRINTF __attribute__ ((format(printf, 1, 0)))
#define FUNCNORETURN __attribute__ ((noreturn))
#define FUNCIERROR __attribute__ ((format(printf, 1, 0),noreturn))
#define FUNCDEAD __attribute__ ((deprecated))
#define FUNCINLINE __attribute__((always_inline))
#define FUNCNOINLINE __attribute__((noinline))
#define ATTRPACK __attribute__ ((packed))
#elif defined(_MSC_VER)
#define ATTRINLINE __forceinline
#endif

#ifndef FUNCPRINTF
#define FUNCPRINTF
#endif
#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif
#ifndef FUNCIERROR
#define FUNCIERROR
#endif
#ifndef FUNCDEAD
#define FUNCDEAD
#endif
#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef ATTRPACK
#define ATTRPACK
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif

#endif //__DOOMTYPE__
