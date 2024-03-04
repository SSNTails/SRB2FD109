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
//---------------------------------------------------------------------
/// \file
/// \brief Zone Memory Allocation, perhaps NeXT ObjectiveC inspired

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC               1 // static entire execution time
#define PU_SOUND                2 // static while playing
#define PU_MUSIC                3 // static while playing

#define PU_HWRPATCHINFO         5 // Hardware GlidePatch_t struct for OpenGL/Glide texture cache
#define PU_HWRPATCHCOLMIPMAP    6 // Hardware GlideMipmap_t struct colromap variation of patch

#define PU_LEVEL               50 // static until level exited
#define PU_LEVSPEC             51 // a special thinker in a level
#define PU_HWRPLANE            52
// Tags >= PU_PURGELEVEL are purgable whenever needed.
#define PU_PURGELEVEL         100
#define PU_CACHE              101
#define PU_HWRCACHE           102 // 'second-level' cache for graphics
                                  // stored in hardware format and downloaded as needed

void Z_Init(void);
void Z_FreeTags(int lowtag, int hightag);
void Z_DumpHeap(int lowtag, int hightag);
void Z_FileDumpHeap(FILE* f);
void Z_CheckHeap(int i);
void Z_ChangeTag2(void* ptr, int tag);

// returns number of bytes allocated for one tag type
size_t Z_TagUsage(int tagnum);

void Z_FreeMemory(size_t* realfree, size_t* cachemem, size_t* usedmem, size_t* largefreeblock);

#ifdef ZDEBUG
#define Z_Free(p) Z_Free2(p, __FILE__, __LINE__)
void Z_Free2(void* ptr, char* file, int line);
#define Z_Malloc(s,t,p) Z_Malloc2(s, t, p, 0, __FILE__, __LINE__)
#define Z_MallocAlign(s,t,p,a) Z_Malloc2(s, t, p, a, __FILE__, __LINE__)
void* Z_Malloc2(size_t size, int tag, void* ptr, int alignbits, char* file, int line);
#else
void Z_Free (void* ptr);
void* Z_MallocAlign(size_t size, int tag, void* user, int alignbits);
#define Z_Malloc(s,t,p) Z_MallocAlign(s, t, p, 0)
#endif

char* Z_Strdup(const char* s, int tag, void** user);

typedef struct memblock_s
{
	size_t size; // including the header and possibly tiny fragments
	void** user; // NULL if a free block
	int tag;     // purgelevel
	int id;      // should be ZONEID
#ifdef ZDEBUG
	char *ownerfile;
	int ownerline;
#endif
	struct memblock_s* next;
	struct memblock_s* prev;
} memblock_t;

// This is used to get the local FILE:LINE info from CPP
// prior to really call the function in question.
//
#ifdef PARANOIA
#define Z_ChangeTag(p,t) \
{ \
	if(((memblock_t*)((byte*)(p) - sizeof(memblock_t)))->id!=0x1d4a11) \
		I_Error("Z_CT at "__FILE__":%i",__LINE__); \
	Z_ChangeTag2(p, t); \
};
#else
#define Z_ChangeTag(p,t) Z_ChangeTag2(p, t)
#endif

#endif
