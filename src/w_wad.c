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
/// \brief Handles WAD file header, directory, lump I/O

// added for linux 19990220 by Kin
#ifndef __APPLE_CC__
#ifndef FREEBSD
#include <malloc.h> // alloca(sizeof)
#endif
#endif
#ifndef SDLIO
#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#endif
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h" // rendermode
#include "d_netfil.h"
#include "dehacked.h"
#include "r_defs.h"
#include "i_system.h"

#ifndef NOMD5
#include "md5.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

//===========================================================================
//                                                                    GLOBALS
//===========================================================================
int numwadfiles; // number of active wadfiles
wadfile_t* wadfiles[MAX_WADFILES]; // 0 to numwadfiles-1 are valid

// W_Shutdown
// Closes all of the WAD files before quitting
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
void W_Shutdown(void)
{
	while(numwadfiles--)
	{
#ifdef SDLIO
		SDL_RWclose(wadfiles[numwadfiles]->handle);
#else
		close(wadfiles[numwadfiles]->handle);
#endif
	}
}

//===========================================================================
//                                                        LUMP BASED ROUTINES
//===========================================================================

// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

static inline void W_LoadDehackedLumps(int wadnum);

//  Allocate a wadfile, setup the lumpinfo (directory) and
//  lumpcache, add the wadfile to the current active wadfiles
//
//  now returns index into wadfiles[], you can get wadfile_t*
//  with:
//       wadfiles[<return value>]
//
//  return -1 in case of problem
//
// Can now load dehacked files (.soc)
//
int W_LoadWadFile(char *filename)
{
#ifdef SDLIO
	SDL_RWops* handle;
#else
	int handle;
	struct stat bufstat;
#endif
#ifndef NOMD5
	FILE* fhandle;
#endif
	lumpinfo_t* lumpinfo;
	lumpcache_t* lumpcache;
	wadfile_t* wadfile;
	int numlumps, i, length;
	char filenamebuf[MAX_WADPATH];
#ifdef HWRENDER
	GlidePatch_t* grPatch;
#endif

	//
	// check if limit of active wadfiles
	//
	if(numwadfiles >= MAX_WADFILES)
	{
		CONS_Printf("Maximum wad files reached\n");
		return -1;
	}

	strncpy(filenamebuf, filename, MAX_WADPATH);
	filename = filenamebuf;
	// open wad file
#ifdef SDLIO
	if(!(handle = SDL_RWFromFile(filename, "rb")))
#else
	if((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
	{
		nameonly(filename); // leave full path here
		if(findfile(filename, NULL, true))
		{
#ifdef SDLIO
			if(!(handle = SDL_RWFromFile(filename, "rb")))
#else
            if((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
			{
				CONS_Printf("Can't open %s\n", filename);
				return -1;
			}
		}
		else
		{
			CONS_Printf("File %s not found.\n", filename);
			return -1;
		}
	}

	// detect dehacked file with the "soc" extension
	if(!stricmp(&filename[strlen(filename) - 4], ".soc"))
	{
		// This code emulates a wadfile with one lump name "OBJCTCFG"
		// at position 0 and size of the whole file.
		// This allows soc files to be like all wads, copied by network and loaded at the console.
#ifndef SDLIO
		fstat(handle, &bufstat);
#endif
		numlumps = 1;
		lumpinfo = Z_Malloc(sizeof(lumpinfo_t), PU_STATIC, NULL);
		lumpinfo->position = 0;
#ifdef SDLIO
		{
			int currpos = SDL_RWtell(handle);
			SDL_RWseek(handle, 0, SEEK_END);
			lumpinfo->size = SDL_RWtell(handle);
			SDL_RWseek(handle, currpos, SEEK_SET);
		}
#else
		lumpinfo->size = bufstat.st_size;
#endif

		strcpy(lumpinfo->name, "OBJCTCFG");
	}
	else
	{
		// assume wad file
		wadinfo_t header;
		lumpinfo_t* lump_p;
		filelump_t* fileinfo;

		// read the header
#ifdef SDLIO
		if(SDL_RWread(handle, &header, 1, sizeof(header)) < sizeof(header))
#else
		if(read(handle, &header, sizeof(header)) < (int)sizeof(header))
#endif
		{
			CONS_Printf("Can't read wad header from %s\n", filename);
			return -1;
		}

		if(strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
		{
			CONS_Printf("%s doesn't have IWAD or PWAD id\n", filename);
			return -1;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// read wad file directory
		length = header.numlumps * sizeof(filelump_t);
		fileinfo = alloca(length);
#ifdef SDLIO
		if(SDL_RWseek(handle, header.infotableofs, SEEK_SET) == -1
			|| SDL_RWread(handle, fileinfo, 1, length) < length)
#else
		if(lseek(handle, header.infotableofs, SEEK_SET) == -1
			|| read(handle, fileinfo, length) < length)
#endif
		{
			CONS_Printf("%s wadfile directory is corrupt\n", filename);
			return -1;
		}

		numlumps = header.numlumps;

		// fill in lumpinfo for this wad
		lump_p = lumpinfo = Z_Malloc(numlumps * sizeof(lumpinfo_t), PU_STATIC, NULL);
		for(i = 0; i < numlumps; i++, lump_p++, fileinfo++)
		{
			lump_p->position = LONG(fileinfo->filepos);
			lump_p->size = LONG(fileinfo->size);
			strncpy(lump_p->name, fileinfo->name, 8);
		}
	}
	//
	// link wad file to search files
	//
#ifndef SDLIO
	fstat(handle, &bufstat);
#endif
	wadfile = Z_Malloc(sizeof(wadfile_t), PU_STATIC, NULL);
	wadfile->filename = Z_StrDup(filename);
	wadfile->handle = handle;
	wadfile->numlumps = numlumps;
	wadfile->lumpinfo = lumpinfo;
#ifdef SDLIO
	{
		int currpos = SDL_RWtell(handle);
		SDL_RWseek(handle,0,SEEK_END);
		wadfile->filesize = SDL_RWtell(handle);
		SDL_RWseek(handle,currpos,SEEK_SET);
	}
#else
	wadfile->filesize = bufstat.st_size;
#endif

	//
	// generate md5sum
	//
#ifndef NOMD5
	fhandle = fopen(filenamebuf, "rb");
	{
		int t = I_GetTime();
		md5_stream(fhandle, wadfile->md5sum);
		if(devparm)
			CONS_Printf("md5 calc for %s took %f second\n",
				wadfile->filename, (float)(I_GetTime() - t)/TICRATE);
	}
	fclose(fhandle);
#endif

	//
	// set up caching
	//
	length = numlumps * sizeof(lumpcache_t);
	lumpcache = Z_Malloc(length, PU_STATIC, NULL);

	memset(lumpcache, 0, length);
	wadfile->lumpcache = lumpcache;

#ifdef HWRENDER
	// allocates GlidePatch info structures STATIC from the start,
	// because these were causing a lot of fragmentation of the heap,
	// considering they are never freed.
	length = numlumps * sizeof(GlidePatch_t);
	grPatch = Z_Malloc(length, PU_HWRPATCHINFO, 0); // never freed
	// set mipmap.downloaded to false
	memset(grPatch, 0, length);
	for(i = 0; i < numlumps; i++)
	{
		// store the software patch lump number for each GlidePatch
		grPatch[i].patchlump = (numwadfiles<<16) + i;
	}
	wadfile->hwrcache = grPatch;
#endif

	//
	// add the wadfile
	//
	wadfiles[numwadfiles++] = wadfile;

	CONS_Printf("Added file %s (%i lumps)\n", filename, numlumps);
	W_LoadDehackedLumps(numwadfiles - 1);
	return numwadfiles - 1;
}

/** Tries to load a series of files.
  * All files are wads unless they have an extension of ".soc".
  *
  * Each file is optional, but at least one file must be found or an error will
  * result. Lump names can appear multiple times. The name searcher looks
  * backwards, so a later file overrides all earlier ones.
  *
  * \param filenames A null-terminated list of files to use.
  * \return 1 if all files were loaded, 0 if at least one was missing or
  *           invalid.
  */
int W_InitMultipleFiles(char** filenames)
{
	int rc = 1;

	// open all the files, load headers, and count lumps
	numwadfiles = 0;

	// will be realloced as lumps are added
	for(; *filenames; filenames++)
		rc &= (W_LoadWadFile(*filenames) != -1) ? 1 : 0;

	if(!numwadfiles)
		I_Error("W_InitMultipleFiles: no files found");

	return rc;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName(const char* name)
{
	union
	{
		char s[9];
		int x[2];
	} name8;
	int i, j, v1, v2;
	lumpinfo_t* lump_p;

	// make the name into two integers for easy compares
	strncpy(name8.s, name, 8);

	// in case the name was a full 8 chars
	name8.s[8] = 0;

	// case insensitive
	strupr(name8.s);

	v1 = name8.x[0];
	v2 = name8.x[1];

	// scan wad files backwards so patch lump files take precedence
	//
	for(i = numwadfiles - 1; i >= 0; i--)
	{
		lump_p = wadfiles[i]->lumpinfo;

		for(j = 0; j < wadfiles[i]->numlumps; j++, lump_p++)
		{
			if(*(int*)lump_p->name == v1 && *(int*)&lump_p->name[4] == v2)
			{
				// high word is the wad file number
				return (i<<16) + j;
			}
		}
	}

	// not found.
	return -1;
}

//
// Same as the original, but checks in one pwad only.
// wadid is a wad number
// (Used for sprites loading)
//
// 'startlump' is the lump number to start the search
//
int W_CheckNumForNamePwad(const char* name, int wadid, int startlump)
{
	union
	{
		char s[9];
		int x[2];
	} name8;

	int i, v1, v2;
	lumpinfo_t* lump_p;

	strncpy(name8.s, name, 8);
	name8.s[8] = 0;
	strupr(name8.s);

	v1 = name8.x[0];
	v2 = name8.x[1];

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if(startlump < wadfiles[wadid]->numlumps)
	{
		lump_p = wadfiles[wadid]->lumpinfo + startlump;
		for(i = startlump; i < wadfiles[wadid]->numlumps; i++, lump_p++)
		{
			if(*(int*)lump_p->name == v1 && *(int*)&lump_p->name[4] == v2)
				return (wadid<<16) + i;
		}
	}

	// not found.
	return -1;
}

//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(const char* name)
{
	int i;

	i = W_CheckNumForName(name);

	if(i == -1)
		I_Error("W_GetNumForName: %s not found!\n", name);

	return i;
}

/** Returns the buffer size needed to load the given lump.
  *
  * \param lump Lump number to look at.
  * \return Buffer size needed, in bytes.
  */
int W_LumpLength(int lump)
{
#ifdef PARANOIA
	if(lump < 0)
		I_Error("W_LumpLength: lump does not exist\n");

	if((lump & 0xFFFF) >= wadfiles[lump>>16]->numlumps)
		I_Error("W_LumpLength: %d >= numlumps", lump);
#endif
	return wadfiles[lump>>16]->lumpinfo[lump&0xFFFF].size;
}

/** Reads bytes from the head of a lump.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination.
  * \param size Number of bytes to read.
  * \return Number of bytes read (should equal size).
  * \sa W_ReadLump
  */
int W_ReadLumpHeader(int lump, void* dest, int size)
{
	int bytesread;
	lumpinfo_t* l;
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif

#ifdef PARANOIA
	if(lump < 0)
		I_Error("W_ReadLumpHeader: lump does not exist\n");

	if((lump & 0xFFFF) >= wadfiles[lump>>16]->numlumps)
		I_Error("W_ReadLumpHeader: %d >= numlumps", lump);
#endif
	l = wadfiles[lump>>16]->lumpinfo + (lump & 0xFFFF);

	// empty resource (usually markers like S_START, F_END ..)
	if(!l->size)
		return 0;

	handle = wadfiles[lump>>16]->handle;

	// zero size means read all the lump
	if(!size || size>l->size)
		size = l->size;

#ifdef SDLIO
	SDL_RWseek(handle, l->position, SEEK_SET);
	bytesread = SDL_RWread(handle, dest, 1, size);
#else
	lseek(handle, l->position, SEEK_SET);
	bytesread = read(handle, dest, size);
#endif

	return bytesread;
}

/** Reads a lump into memory.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination. Size must be >=
  *             W_LumpLength().
  * \sa W_ReadLumpHeader
  */
void W_ReadLump(int lump, void* dest)
{
	W_ReadLumpHeader(lump, dest, 0);
}

// ==========================================================================
// W_CacheLumpNum
// ==========================================================================
void* W_CacheLumpNum(int lump, int tag)
{
	byte* ptr;
	lumpcache_t* lumpcache;

	// Don't keep doing operations to the lump variable!
	int llump = lump & 0xffff;
	int lfile = lump >> 16;

#ifdef PARANOIA
	// check return value of a previous W_CheckNumForName()
	if(lfile >= numwadfiles)
		I_Error("W_CacheLumpNum: %d >= numwadfiles(%i)\n", lfile, numwadfiles);
	if(llump >= wadfiles[lfile]->numlumps)
		I_Error("W_CacheLumpNum: %d >= numlumps", llump);
	if(lump == -1)
		I_Error("W_CacheLumpNum: -1 passed!\n");
	if(llump < 0)
		I_Error("W_CacheLumpNum: %d < 0!\n", llump);
#endif

	lumpcache = wadfiles[lfile]->lumpcache;
	if(!lumpcache[llump])
	{
		// read the lump in
		ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[llump]);
		W_ReadLumpHeader(lump, lumpcache[llump], 0); // read full
	}
	else
		Z_ChangeTag(lumpcache[llump],tag);

	return lumpcache[llump];
}

// ==========================================================================
// W_CacheLumpName
// ==========================================================================
void* W_CacheLumpName(const char* name, int tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, and
// the heap memory cache then acts as a 'level 2' cache just after the
// graphics card memory.

//
// Cache a patch into heap memory, convert the patch format as necessary
//

// Software-only compile cache the data without conversion
#ifdef HWRENDER

void* W_CachePatchNum(int lump, int tag)
{
	GlidePatch_t* grPatch;

	if(rendermode == render_soft || rendermode == render_none)
		return W_CacheLumpNum(lump, tag);

#ifdef PARANOIA
	// check the return value of a previous W_CheckNumForName()
	if((lump == -1) || ((lump&0xffff) >= wadfiles[lump>>16]->numlumps))
		I_Error("W_CachePatchNum: %d >= numlumps", lump & 0xffff);
#endif

	grPatch = &(wadfiles[lump>>16]->hwrcache[lump & 0xffff]);

	if(grPatch->mipmap.grInfo.data)
	{
		if(tag == PU_CACHE)
			tag = PU_HWRCACHE;
		Z_ChangeTag(grPatch->mipmap.grInfo.data, tag);
	}
	else
	{
		// first time init grPatch fields
		// we need patch w,h,offset,...
		// this code will be executed latter in GetPatch, anyway
		// do it now
		patch_t* ptr = W_CacheLumpNum(grPatch->patchlump, PU_STATIC);
		HWR_MakePatch(ptr, grPatch, &grPatch->mipmap);
		Z_Free(ptr);
	}

	// return GlidePatch_t, which can be casted to (patch_t) with valid patch header info
	return (void*)grPatch;
}

#endif // HWRENDER Glide version

void* W_CachePatchName(const char* name, int tag)
{
	int num;

	num = W_CheckNumForName(name);

	if(num < 0)
		return W_CachePatchNum(W_GetNumForName("BRDR_MM"), tag);
	return W_CachePatchNum(num, tag);
}

// search for all DEHACKED lump in all wads and load it
static inline void W_LoadDehackedLumps(int wadnum)
{
	int clump = 0;

	// Check for MAINCFG
	for(;;)
	{
		clump = W_CheckNumForNamePwad("MAINCFG", wadnum, clump);
		if(clump == -1)
			break;
		CONS_Printf("Loading main config from %s\n", wadfiles[wadnum]->filename);
		DEH_LoadDehackedLump(clump);
		clump++;
	}

	// Check for OBJCTCFG
	for(;;)
	{
		clump = W_CheckNumForNamePwad("OBJCTCFG", wadnum, clump);
		if(clump == -1)
			break;
		CONS_Printf("Loading object config from %s\n", wadfiles[wadnum]->filename);
		DEH_LoadDehackedLump(clump);
		clump++;
	}
}

/** Verifies a file's MD5 is as it should be.
  * For releases, used as cheat prevention. If the MD5 doesn't match, a fatal
  * error is thrown. The game must be compiled with MD5 support for this to
  * work; i.e., it won't on Windows CE.
  *
  * \param wadfilenum Number of the loaded wad file to check.
  * \param matchmd5   The MD5 sum this wad should have.
  * \author Graue <graue@oceanbase.org>
  */
void W_VerifyFileMD5(int wadfilenum, const char* matchmd5)
{
#ifdef PARANOIA
	if(wadfilenum >= numwadfiles || wadfilenum < 0)
		I_Error("W_VerifyFileMD5 called with invalid wadfilenum");
#endif
#ifndef NOMD5
	if(memcmp(matchmd5, wadfiles[wadfilenum]->md5sum, 16))
		I_Error("File is corrupt or has been modified: %s (\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X\\x%X)", wadfiles[wadfilenum]->filename, wadfiles[wadfilenum]->md5sum[0],
																																				wadfiles[wadfilenum]->md5sum[1],
																																				wadfiles[wadfilenum]->md5sum[2],
																																				wadfiles[wadfilenum]->md5sum[3],
																																				wadfiles[wadfilenum]->md5sum[4],
																																				wadfiles[wadfilenum]->md5sum[5],
																																				wadfiles[wadfilenum]->md5sum[6],
																																				wadfiles[wadfilenum]->md5sum[7],
																																				wadfiles[wadfilenum]->md5sum[8],
																																				wadfiles[wadfilenum]->md5sum[9],
																																				wadfiles[wadfilenum]->md5sum[10],
																																				wadfiles[wadfilenum]->md5sum[11],
																																				wadfiles[wadfilenum]->md5sum[12],
																																				wadfiles[wadfilenum]->md5sum[13],
																																				wadfiles[wadfilenum]->md5sum[14],
																																				wadfiles[wadfilenum]->md5sum[15]);
#endif
}

/** Checks a wad for lumps other than music and sound.
  * Used during game load to verify music.dta is a good file and during a
  * netgame join (on the server side) to see if a wad is important enough to
  * be sent.
  *
  * \param filename Filename of the wad to check.
  * \return 1 if file contains only music/sound lumps, 0 if it contains other
  *         stuff (maps, sprites, dehacked lumps, and so on).
  * \author Alam Arias
  * \todo A bit of cleanup here couldn't hurt.
  */
int W_VerifyNMUSlumps(const char* filename)
{
#ifdef SDLIO
	SDL_RWops* handle;
#else
	int handle;
#endif

	int numlumps, i, length, goodfile = false;
	char filenamebuf[MAX_WADPATH];

	strncpy(filenamebuf, filename, MAX_WADPATH);
	filename = filenamebuf;
	// open wad file
#ifdef SDLIO
	if(!(handle = SDL_RWFromFile(filename,"rb")))
#else
	if((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
	{
		nameonly(filenamebuf); // leave full path here
		if(findfile(filenamebuf, NULL, true))
		{
#ifdef SDLIO
			if(!(handle = SDL_RWFromFile(filename, "rb")))
#else
			if((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
				goodfile = true;
		}
		else
			goodfile = true;
	}

	// detect dehacked file with the "soc" extension
	if(!stricmp(&filename[strlen(filename) - 4], ".soc"))
		goodfile = false; // no way!
	else if(!goodfile)
	{
		// assume wad file
		wadinfo_t header;
		lumpinfo_t lump_p;
		filelump_t* fileinfo;

		// read the header
#ifdef SDLIO
		SDL_RWread(handle, &header, 1, sizeof(header));
#else
		read(handle, &header, sizeof(header));
#endif
		if(strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
		{
#ifdef SDLIO
			SDL_RWclose(handle);
#else
			close(handle);
#endif
			return true;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// read wad file directory
		length = header.numlumps*sizeof(filelump_t);
		fileinfo = alloca(length);
#ifdef SDLIO
		SDL_RWseek(handle, header.infotableofs, SEEK_SET);
		SDL_RWread(handle, fileinfo, 1, length);
#else
		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);
#endif
		numlumps = header.numlumps;

		// fill in lumpinfo for this wad
		memset(&lump_p, 0, sizeof(lumpinfo_t));
		goodfile = true;
		for(i = 0; i < numlumps; i++, fileinfo++)
		{
			lump_p.position = LONG(fileinfo->filepos);
			lump_p.size = LONG(fileinfo->size);
			strncpy(lump_p.name, fileinfo->name, 8);
			if((strncmp(lump_p.name, "D_", 2)) && // MIDI
				(strncmp(lump_p.name, "O_", 2)) && // MOD/S3M/IT/XM/OGG/MP3/WAV
				(strncmp(lump_p.name, "DS", 2))) // WAVE SFX
			{
				goodfile = false;
				break;
			}
		}
	}
#ifdef SDLIO
	SDL_RWclose(handle);
#else
	close(handle);
#endif
	return goodfile;
}
