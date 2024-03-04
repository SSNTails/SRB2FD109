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
/// \brief WAD I/O functions, wad resource definitions (some)

#ifndef __W_WAD__
#define __W_WAD__

#ifdef HWRENDER
#include "hardware/hw_data.h"
#else
typedef void GlidePatch_t;
#endif
#ifdef SDLIO
#include <SDL/SDL_rwops.h>
#endif

#ifdef __GNUG__
#pragma interface
#endif

// ==============================================================
//               WAD FILE STRUCTURE DEFINITIONS
// ==============================================================

typedef long lumpnum_t; // 16:16 long (wad num: lump num)

// header of a wad file

typedef struct
{
	char identification[4]; // should be "IWAD" or "PWAD"
	int numlumps; // how many resources
	int infotableofs; // the 'directory' of resources
} wadinfo_t;

// an entry of the wad directory

typedef struct
{
	int filepos; // file offset of the resource
	int size; // size of the resource
	char name[8]; // name of the resource
} filelump_t;

// in memory: initialised at game startup

typedef struct
{
	char name[8]; // filelump_t name[]
	int position; // filelump_t filepos
	int size; // filelump_t size
} lumpinfo_t;

// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

#define WADFILENUM(lump) (lump>>16) // wad file number in upper word
#define LUMPNUM(lump) (lump&0xffff) // lump number for this pwad

#define MAX_WADPATH 128
#define MAX_WADFILES 48 // maximum of wad files used at the same time
// (there is a max of simultaneous open files anyway, and this should be plenty)

#define lumpcache_t void*

typedef struct wadfile_s
{
	char* filename;
	lumpinfo_t* lumpinfo;
	lumpcache_t* lumpcache;
	GlidePatch_t* hwrcache; // patches are cached in renderer's native format
	int numlumps; // this wad's number of resources
#ifdef SDLIO
	SDL_RWops* handle;
#else
	int handle;
#endif
	ULONG filesize; // for network
	unsigned char md5sum[16];
} wadfile_t;

extern int numwadfiles;
extern wadfile_t* wadfiles[MAX_WADFILES];

// =========================================================================

void W_Shutdown(void);

// Load and add a wadfile to the active wad files, return wad file number.
// You can get a wadfile_t pointer: the return value indexes wadfiles[].
int W_LoadWadFile(char* filename);

// W_InitMultipleFiles returns 1 if all is okay, 0 otherwise,
// so that it stops with a message if a file was not found, but not if all is okay.
int W_InitMultipleFiles(char** filenames);

int W_CheckNumForName(const char* name);
int W_CheckNumForNamePwad(const char* name, int wadid, int startlump); // checks only in one pwad
int W_GetNumForName(const char* name);

int W_LumpLength(int lump);
int W_ReadLumpHeader(int lump, void* dest, int size); // read all or a part of a lump
void W_ReadLump(int lump, void* dest);

void* W_CacheLumpNum(int lump, int tag);
void* W_CacheLumpName(const char* name, int tag);

void* W_CachePatchName(const char* name, int tag);

#ifdef HWRENDER
void* W_CachePatchNum(int lump, int tag); // return a patch_t
#else
#define W_CachePatchNum(lump, tag) W_CacheLumpNum(lump, tag)
#endif

void W_VerifyFileMD5(int wadfilenum, const char* matchmd5);
int W_VerifyNMUSlumps(const char* filename);

// Store lists of lumps for F_START/F_END etc.
typedef struct
{
	int wadfile;
	int firstlump;
	int numlumps;
} lumplist_t;

#endif // __W_WAD__
