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
/// \brief Preparation of data for rendering,generation of lookups, caching, retrieval by name

#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h" // levelflats
#include "v_video.h" // pLocalPalette

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(_WIN64)
#include <malloc.h> // alloca(sizeof)
#endif

//
// Graphics.
// SRB2 graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

int numspritelumps;

// textures
int numtextures = 0; // total number of textures found,
// size of following tables

texture_t** textures = NULL;
static unsigned int** texturecolumnofs; // column offset lookup table for each texture
static byte** texturecache; // graphics data for each generated full-size texture

// texture width is a power of 2, so it can easily repeat along sidedefs using a simple mask
static int* texturewidthmask;

fixed_t* textureheight; // needed for texture pegging

int* flattranslation; // for global animation
int* texturetranslation;

// needed for pre rendering
fixed_t* spritewidth;
fixed_t* spriteoffset;
fixed_t* spritetopoffset;
fixed_t* spriteheight;

lighttable_t* colormaps;

// for debugging/info purposes
static int flatmemory, spritememory, texturememory;

// highcolor stuff
short color8to16[256]; // remap color index to highcolor rgb value
short* hicolormaps; // test a 32k colormap remaps high -> high

//
// MAPTEXTURE_T CACHING
// When a texture is first needed, it counts the number of composite columns
//  required in the texture and allocates space for a column directory and
//  any new columns.
// The directory will simply point inside other patches if there is only one
//  patch in a given column, but any columns with multiple patches will have
//  new column_ts generated.
//

//
// R_DrawColumnInCache
// Clip and draw a column from a patch into a cached post.
//
static inline void R_DrawColumnInCache(column_t* patch, byte* cache, int originy, int cacheheight)
{
	int count, position;
	byte* source;
	byte* dest;

	dest = (byte*)cache;

	while(patch->topdelta != 0xff)
	{
		source = (byte*)patch + 3;
		count = patch->length;
		position = originy + patch->topdelta;

		if(position < 0)
		{
			count += position;
			position = 0;
		}

		if(position + count > cacheheight)
			count = cacheheight - position;

		if(count > 0)
			memcpy(cache + position, source, count);

		patch = (column_t*)((byte*)patch + patch->length + 4);
	}
}

//
// R_GenerateTexture
//
// Allocate space for full size texture, either single patch or 'composite'
// Build the full textures from patches.
// The texture caching system is a little more hungry of memory, but has
// been simplified for the sake of highcolor, dynamic ligthing, & speed.
//
// This is not optimised, but it's supposed to be executed only once
// per level, when enough memory is available.
//
static byte* R_GenerateTexture(int texnum)
{
	byte* block;
	byte* blocktex;
	texture_t* texture;
	texpatch_t* patch;
	patch_t* realpatch;
	int x, x1, x2, i, blocksize;
	column_t* patchcol;
	unsigned int* colofs;

	texture = textures[texnum];

	// allocate texture column offset lookup
	// single-patch textures can have holes in them and may be used on
	// 2sided lines so they need to be kept in 'packed' format
	if(texture->patchcount == 1)
	{
		patch = texture->patches;
		blocksize = W_LumpLength(patch->patch);
		realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);

		block = Z_Malloc(blocksize, PU_STATIC, // will change tag at end of this function
			&texturecache[texnum]);
		memcpy(block, realpatch, blocksize);
		texturememory += blocksize;

		// use the patch's column lookup
		colofs = (unsigned int*)(block + 8);
		texturecolumnofs[texnum] = colofs;
		blocktex = block;
		for(i = 0; i < texture->width; i++)
			colofs[i] += 3;
		goto done;
	}

	// multi-patch textures (or 'composite')
	blocksize = (texture->width * 4) + (texture->width * texture->height);
	texturememory += blocksize;
	block = Z_Malloc(blocksize, PU_STATIC, &texturecache[texnum]);

	// columns lookup table
	colofs = (unsigned int*)block;
	texturecolumnofs[texnum] = colofs;

	// texture data before the lookup table
	blocktex = block + (texture->width*4);

	// Composite the columns together.
	patch = texture->patches;

	for(i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
		x1 = patch->originx;
		x2 = x1 + SHORT(realpatch->width);

		if(x1 < 0)
			x = 0;
		else
			x = x1;

		if(x2 > texture->width)
			x2 = texture->width;

		for(; x < x2; x++)
		{
			patchcol = (column_t*)((byte*)realpatch + LONG(realpatch->columnofs[x-x1]));

			// generate column ofset lookup
			colofs[x] = (x * texture->height) + (texture->width*4);

			R_DrawColumnInCache(patchcol, block + colofs[x], patch->originy, texture->height);
		}
	}

done:
	// Now that the texture has been built in column cache, it is purgable from zone memory.
	Z_ChangeTag(block, PU_CACHE);

	return blocktex;
}

//
// R_GetColumn
//
byte* R_GetColumn(fixed_t tex, int col)
{
	byte* data;

	col &= texturewidthmask[tex];
	data = texturecache[tex];

	if(!data)
		data = R_GenerateTexture(tex);

	return data + texturecolumnofs[tex][col];
}

// convert flats to hicolor as they are requested
//
byte* R_GetFlat(int flatlumpnum)
{
	return W_CacheLumpNum(flatlumpnum, PU_CACHE);
}

//
// Empty the texture cache (used for load wad at runtime)
//
void R_FlushTextureCache(void)
{
	int i;

	if(numtextures > 0)
		for(i = 0; i < numtextures; i++)
			if(texturecache[i])
				Z_Free(texturecache[i]);
}

//
// R_InitTextures
// Initializes the texture list with the textures from the world map.
//
void R_LoadTextures(void)
{
	maptexture_t* mtexture;
	texture_t* texture;
	mappatch_t* mpatch;
	texpatch_t* patch;
	char name[9];
	char* name_p;
	char* pnames;
	int* maptex;
	int* maptex2;
	int* maptex1;
	int* patchlookup;
	int* directory;
	int i, j, nummappatches, offset, maxoff, maxoff2, numtextures1, numtextures2;

	// free previous memory before numtextures change

	if(numtextures > 0)
		for(i = 0; i < numtextures; i++)
		{
			if(textures[i])
				Z_Free(textures[i]);
			if(texturecache[i])
				Z_Free(texturecache[i]);
		}

	// Load the patch names from pnames.lmp.
	name[8] = 0;
	pnames = W_CacheLumpName("PNAMES", PU_STATIC);
	nummappatches = LONG(*((int*)pnames));
	name_p = pnames+4;
	patchlookup = alloca(nummappatches*sizeof(*patchlookup));

	for(i = 0; i < nummappatches; i++)
	{
		strncpy(name, name_p+i*8, 8);
		patchlookup[i] = W_CheckNumForName(name);
	}
	Z_Free(pnames);

	// Load the map texture definitions from textures.lmp.
	// The data is contained in one or two lumps, TEXTURE1 and TEXTURE2.
	maptex = maptex1 = W_CacheLumpName("TEXTURE1", PU_STATIC);
	numtextures1 = LONG(*maptex);
	maxoff = W_LumpLength(W_GetNumForName("TEXTURE1"));
	directory = maptex+1;

	if(W_CheckNumForName("TEXTURE2") != -1)
	{
		maptex2 = W_CacheLumpName("TEXTURE2", PU_STATIC);
		numtextures2 = LONG(*maptex2);
		maxoff2 = W_LumpLength(W_GetNumForName("TEXTURE2"));
	}
	else
	{
		maptex2 = NULL;
		numtextures2 = maxoff2 = 0;
	}
	numtextures = numtextures1 + numtextures2;

	// there are actually 5 buffers allocated in one for convenience
	if(textures)
		Z_Free(textures);

	textures = Z_Malloc(numtextures*4*5, PU_STATIC, 0);

	texturecolumnofs = (void*)((int*)textures + numtextures);
	texturecache = (void*)((int*)textures + numtextures*2);
	texturewidthmask = (void*)((int*)textures + numtextures*3);
	textureheight = (void*)((int*)textures + numtextures*4);

	for(i = 0; i < numtextures; i++, directory++)
	{
		if(i == numtextures1)
		{
			// Start looking in second texture file.
			maptex = maptex2;
			maxoff = maxoff2;
			directory = maptex+1;
		}

		// offset to the current texture in TEXTURESn lump
		offset = LONG(*directory);

		if(offset > maxoff)
			I_Error("R_LoadTextures: bad texture directory");

		// maptexture describes texture name, size, and
		// used patches in z order from bottom to top
		mtexture = (maptexture_t*)((byte*)maptex + offset);

		texture = textures[i] = Z_Malloc(sizeof(texture_t)
			+ sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1), PU_STATIC, 0);

		texture->width = SHORT(mtexture->width);
		texture->height = SHORT(mtexture->height);
		texture->patchcount = SHORT(mtexture->patchcount);

		memcpy(texture->name, mtexture->name, sizeof(texture->name));
		mpatch = &mtexture->patches[0];
		patch = &texture->patches[0];

		for(j = 0; j < texture->patchcount; j++, mpatch++, patch++)
		{
			patch->originx = SHORT(mpatch->originx);
			patch->originy = SHORT(mpatch->originy);
			patch->patch = patchlookup[SHORT(mpatch->patch)];
			if(patch->patch == -1)
				I_Error("R_LoadTextures: Missing patch in texture %s", texture->name);
		}

		j = 1;
		while(j*2 <= texture->width)
			j <<= 1;

		texturewidthmask[i] = j - 1;
		textureheight[i] = texture->height<<FRACBITS;
	}

	Z_Free(maptex1);
	if(maptex2)
		Z_Free(maptex2);

	// This takes 90% of texture loading time. Precalculate whatever possible.
	for(i = 0; i < numtextures; i++)
		texturecache[i] = NULL;

	// Create translation table for global animation.
	if(texturetranslation)
		Z_Free(texturetranslation);

	texturetranslation = Z_Malloc((numtextures+1)*4, PU_STATIC, 0);

	for(i = 0; i < numtextures; i++)
		texturetranslation[i] = i;
}

static inline int R_CheckNumForNameList(char* name, lumplist_t* list, int listsize)
{
	int i, lump;

	for(i = listsize - 1; i > -1; i--)
	{
		lump = W_CheckNumForNamePwad(name, list[i].wadfile, list[i].firstlump);
		if((lump & 0xffff) > (list[i].firstlump + list[i].numlumps) || lump == -1)
			continue;
		else
			return lump;
	}
	return -1;
}

static lumplist_t* colormaplumps;
static int numcolormaplumps;

static inline void R_InitExtraColormaps()
{
	int startnum, endnum, cfile, clump;

	numcolormaplumps = 0;
	colormaplumps = NULL;
	cfile = clump = 0;

	for(; cfile < numwadfiles; cfile++, clump = 0)
	{
		startnum = W_CheckNumForNamePwad("C_START", cfile, clump);
		if(startnum == -1)
			continue;

		endnum = W_CheckNumForNamePwad("C_END", cfile, clump);

		if(endnum == -1)
			I_Error("R_InitExtraColormaps: C_START without C_END\n");

		if((startnum >> 16) != (endnum >> 16))
			I_Error("R_InitExtraColormaps: C_START and C_END in different wad files!\n");

		colormaplumps = (lumplist_t*)realloc(colormaplumps,
			sizeof(lumplist_t) * (numcolormaplumps + 1));
		colormaplumps[numcolormaplumps].wadfile = startnum >> 16;
		colormaplumps[numcolormaplumps].firstlump = (startnum & 0xFFFF) + 1;
		colormaplumps[numcolormaplumps].numlumps = endnum - (startnum + 1);
		numcolormaplumps++;
	}
}

static lumplist_t* flats;
static int numflatlists;

static void R_InitFlats()
{
	int startnum, endnum, cfile, clump;

	numflatlists = 0;
	flats = NULL;
	cfile = clump = 0;

	for(; cfile < numwadfiles; cfile++, clump = 0)
	{
		startnum = W_CheckNumForNamePwad("F_START", cfile, clump);
		if(startnum == -1)
		{
			clump = 0;
			startnum = W_CheckNumForNamePwad("FF_START", cfile, clump);

			if(startnum == -1) // If STILL -1, search the whole file!
			{
				flats = (lumplist_t*)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
				flats[numflatlists].wadfile = cfile;
				flats[numflatlists].firstlump = 0;
				flats[numflatlists].numlumps = 0xffff; // Search the entire file!
				numflatlists++;
				continue;
			}
		}

		endnum = W_CheckNumForNamePwad("F_END", cfile, clump);
		if(endnum == -1)
			endnum = W_CheckNumForNamePwad("FF_END", cfile, clump);

		if(endnum == -1 || (startnum & 0xFFFF) > (endnum & 0xFFFF))
		{
			flats = (lumplist_t*)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
			flats[numflatlists].wadfile = cfile;
			flats[numflatlists].firstlump = 0;
			flats[numflatlists].numlumps = 0xffff; // Search the entire file!
			numflatlists++;
			continue;
		}

		flats = (lumplist_t*)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
		flats[numflatlists].wadfile = startnum >> 16;
		flats[numflatlists].firstlump = (startnum & 0xFFFF) + 1;
		flats[numflatlists].numlumps = endnum - (startnum + 1);
		numflatlists++;
		continue;
	}

	if(!numflatlists)
		I_Error("R_InitFlats: No flats found!\n");
}

int R_GetFlatNumForName(const char* name)
{
	int lump = W_CheckNumForName(name);

	if(lump == -1)
	{
		if(strcmp(name, "F_SKY1"))
			CONS_Printf("R_GetFlatNumForName: Could not find flat %.8s\n", name);
		lump = W_CheckNumForName("REDFLR");
	}

	return lump;
}

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad, so the sprite does not need to be
// cached completely, just for having the header info ready during rendering.
//

//
// allocate sprite lookup tables
//
static void R_InitSpriteLumps(void)
{
	// Doom used to set numspritelumps from S_END - S_START + 1

	// FIXME: find a better solution for adding new sprites dynamically
	numspritelumps = 0;

	spritewidth = Z_Malloc(MAXSPRITELUMPS*4, PU_STATIC, 0);
	spriteoffset = Z_Malloc(MAXSPRITELUMPS*4, PU_STATIC, 0);
	spritetopoffset = Z_Malloc(MAXSPRITELUMPS*4, PU_STATIC, 0);
	spriteheight = Z_Malloc(MAXSPRITELUMPS*4, PU_STATIC, 0);
}

//
// R_InitColormaps
//
static void R_InitColormaps(void)
{
	int lump;

	// Load in the light tables, now 64k aligned for smokie...
	lump = W_GetNumForName("COLORMAP");
	colormaps = Z_MallocAlign(W_LumpLength (lump), PU_STATIC, 0, 16);
	W_ReadLump(lump, colormaps);

	// Init Boom colormaps.
	R_ClearColormaps();
	R_InitExtraColormaps();
}

static int foundcolormaps[MAXCOLORMAPS];

static char colormapFixingArray[MAXCOLORMAPS][3][9];
static int carrayindex;

//
// R_ClearColormaps
//
// Clears out extra colormaps between levels.
//
void R_ClearColormaps(void)
{
	int i;

	num_extra_colormaps = 0;

	carrayindex = 0;

	for(i = 0; i < MAXCOLORMAPS; i++)
		foundcolormaps[i] = -1;
	memset(extra_colormaps, 0, sizeof(extra_colormaps));
}

int R_ColormapNumForName(char *name)
{
	int lump, i;

	if(num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_ColormapNumForName: Too many colormaps!\n");

	lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
	if(lump == -1)
		I_Error("R_ColormapNumForName: Cannot find colormap lump %.8s\n", name);

	for(i = 0; i < num_extra_colormaps; i++)
		if(lump == foundcolormaps[i])
			return i;

	foundcolormaps[num_extra_colormaps] = lump;

	// aligned on 8 bit for asm code
	extra_colormaps[num_extra_colormaps].colormap = Z_MallocAlign (W_LumpLength (lump), PU_LEVEL, 0, 16);
	W_ReadLump(lump, extra_colormaps[num_extra_colormaps].colormap);

	// We set all params of the colormap to normal because there
	// is no real way to tell how GL should handle a colormap lump anyway..
	extra_colormaps[num_extra_colormaps].maskcolor = 0xffff;
	extra_colormaps[num_extra_colormaps].fadecolor = 0x0;
	extra_colormaps[num_extra_colormaps].maskamt = 0x0;
	extra_colormaps[num_extra_colormaps].fadestart = 0;
	extra_colormaps[num_extra_colormaps].fadeend = 33;
	extra_colormaps[num_extra_colormaps].fog = 0;

	num_extra_colormaps++;
	return num_extra_colormaps - 1;
}

//
// R_CreateColormap
//
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.
//
static double deltas[256][3], map[256][3];

static unsigned char NearestColor(unsigned char r, unsigned char g, unsigned char b);
static int RoundUp(double number);

int R_CreateColormap(char *p1, char *p2, char *p3)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double r, g, b, cbrightness, maskamt = 0, othermask = 0;
	int mask, i, fog = 0, mapnum = num_extra_colormaps;
	unsigned int cr, cg, cb, maskcolor, fadecolor;
	unsigned int fadestart = 0, fadeend = 33, fadedist = 33;

#define HEX2INT(x) (unsigned int)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
	if(p1[0] == '#')
	{
		cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
		cmaskr = cr;
		cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
		cmaskg = cg;
		cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));
		cmaskb = cb;
		// Create a rough approximation of the color (a 16 bit color)
		maskcolor = ((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11);
		if(p1[7] >= 'a' && p1[7] <= 'z')
			mask = (p1[7] - 'a');
		else if(p1[7] >= 'A' && p1[7] <= 'Z')
			mask = (p1[7] - 'A');
		else
			mask = 24;

		maskamt = mask/24.0;

		othermask = 1 - maskamt;
		maskamt /= 0xff;
		cmaskr *= maskamt;
		cmaskg *= maskamt;
		cmaskb *= maskamt;
	}
	else
	{
		cmaskr = cmaskg = cmaskb = 0xff;
		maskamt = 0;
		maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)
	if(p2[0] == '#')
	{
		// Get parameters like fadestart, fadeend, and the fogflag
		fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
		fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
		if(fadestart > 32)
			fadestart = 0;
		if(fadeend > 33 || fadeend < 1)
			fadeend = 33;
		fadedist = fadeend - fadestart;
		fog = NUMFROMCHAR(p2[1]) ? 1 : 0;
	}
#undef getnum

	if(p3[0] == '#')
	{
		cdestr = cr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
		cdestg = cg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
		cdestb = cb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));
		fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
	}
	else
		cdestr = cdestg = cdestb = fadecolor = 0;
#undef HEX2INT

	for(i = 0; i < num_extra_colormaps; i++)
	{
		if(foundcolormaps[i] != -1)
			continue;
		if(maskcolor == extra_colormaps[i].maskcolor
			&& fadecolor == extra_colormaps[i].fadecolor
			&& maskamt == extra_colormaps[i].maskamt
			&& fadestart == extra_colormaps[i].fadestart
			&& fadeend == extra_colormaps[i].fadeend
			&& fog == extra_colormaps[i].fog)
		{
			return i;
		}
	}

	if(num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_CreateColormap: Too many colormaps!\n");

	strncpy(colormapFixingArray[num_extra_colormaps][0], p1, 8);
	strncpy(colormapFixingArray[num_extra_colormaps][1], p2, 8);
	strncpy(colormapFixingArray[num_extra_colormaps][2], p3, 8);

	num_extra_colormaps++;

	if(rendermode == render_soft)
	{
		for(i = 0; i < 256; i++)
		{
			r = pLocalPalette[i].s.red;
			g = pLocalPalette[i].s.green;
			b = pLocalPalette[i].s.blue;
			cbrightness = sqrt((r*r) + (g*g) + (b*b));

			map[i][0] = (cbrightness * cmaskr) + (r * othermask);
			if(map[i][0] > 255.0)
				map[i][0] = 255.0;
			deltas[i][0] = (map[i][0] - cdestr) / (double)fadedist;

			map[i][1] = (cbrightness * cmaskg) + (g * othermask);
			if(map[i][1] > 255.0)
				map[i][1] = 255.0;
			deltas[i][1] = (map[i][1] - cdestg) / (double)fadedist;

			map[i][2] = (cbrightness * cmaskb) + (b * othermask);
			if(map[i][2] > 255.0)
				map[i][2] = 255.0;
			deltas[i][2] = (map[i][2] - cdestb) / (double)fadedist;
		}
	}

	foundcolormaps[mapnum] = -1;

	// aligned on 8 bit for asm code
	extra_colormaps[mapnum].colormap = NULL;
	extra_colormaps[mapnum].maskcolor = (unsigned short)maskcolor;
	extra_colormaps[mapnum].fadecolor = (unsigned short)fadecolor;
	extra_colormaps[mapnum].maskamt = maskamt;
	extra_colormaps[mapnum].fadestart = (unsigned short)fadestart;
	extra_colormaps[mapnum].fadeend = (unsigned short)fadeend;
	extra_colormaps[mapnum].fog = fog;

	return mapnum;
}

void R_MakeColormaps(void)
{
	int i;

	carrayindex = num_extra_colormaps;
	num_extra_colormaps = 0;

	for(i = 0; i < carrayindex; i++)
		R_CreateColormap2(colormapFixingArray[i][0], colormapFixingArray[i][1],
			colormapFixingArray[i][2]);
}

void R_CreateColormap2(char *p1, char *p2, char *p3)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double r, g, b, cbrightness;
	double maskamt = 0, othermask = 0;
	int mask, i, p, fog = 0, mapnum = num_extra_colormaps;
	char* colormap_p;
	unsigned int cr, cg, cb, maskcolor, fadecolor;
	unsigned int fadestart = 0, fadeend = 33, fadedist = 33;

#define HEX2INT(x) (unsigned int)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
	if(p1[0] == '#')
	{
		cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
		cmaskr = cr;
		cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
		cmaskg = cg;
		cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));
		cmaskb = cb;
		// Create a rough approximation of the color (a 16 bit color)
		maskcolor = ((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11);
		if(p1[7] >= 'a' && p1[7] <= 'z')
			mask = (p1[7] - 'a');
		else if(p1[7] >= 'A' && p1[7] <= 'Z')
			mask = (p1[7] - 'A');
		else
			mask = 24;

		maskamt = mask/24.0;

		othermask = 1 - maskamt;
		maskamt /= 0xff;
		cmaskr *= maskamt;
		cmaskg *= maskamt;
		cmaskb *= maskamt;
	}
	else
	{
		cmaskr = cmaskg = cmaskb = 0xff;
		maskamt = 0;
		maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)
	if(p2[0] == '#')
	{
		// Get parameters like fadestart, fadeend, and the fogflag
		fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
		fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
		if(fadestart > 32)
			fadestart = 0;
		if(fadeend > 33 || fadeend < 1)
			fadeend = 33;
		fadedist = fadeend - fadestart;
		fog = NUMFROMCHAR(p2[1]) ? 1 : 0;
	}
#undef getnum

	if(p3[0] == '#')
	{
		cdestr = cr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
		cdestg = cg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
		cdestb = cb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));
		fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
	}
	else
		cdestr = cdestg = cdestb = fadecolor = 0;
#undef HEX2INT

	for(i = 0; i < num_extra_colormaps; i++)
	{
		if(foundcolormaps[i] != -1)
			continue;
		if(maskcolor == extra_colormaps[i].maskcolor
			&& fadecolor == extra_colormaps[i].fadecolor
			&& maskamt == extra_colormaps[i].maskamt
			&& fadestart == extra_colormaps[i].fadestart
			&& fadeend == extra_colormaps[i].fadeend
			&& fog == extra_colormaps[i].fog)
		{
			return;
		}
	}

	if(num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_CreateColormap: Too many colormaps!\n");
	num_extra_colormaps++;

	if(rendermode == render_soft)
	{
		for(i = 0; i < 256; i++)
		{
			r = pLocalPalette[i].s.red;
			g = pLocalPalette[i].s.green;
			b = pLocalPalette[i].s.blue;
			cbrightness = sqrt((r*r) + (g*g) + (b*b));

			map[i][0] = (cbrightness * cmaskr) + (r * othermask);
			if(map[i][0] > 255.0)
				map[i][0] = 255.0;
			deltas[i][0] = (map[i][0] - cdestr) / (double)fadedist;

			map[i][1] = (cbrightness * cmaskg) + (g * othermask);
			if(map[i][1] > 255.0)
				map[i][1] = 255.0;
			deltas[i][1] = (map[i][1] - cdestg) / (double)fadedist;

			map[i][2] = (cbrightness * cmaskb) + (b * othermask);
			if(map[i][2] > 255.0)
				map[i][2] = 255.0;
			deltas[i][2] = (map[i][2] - cdestb) / (double)fadedist;
		}
	}

	foundcolormaps[mapnum] = -1;

	// aligned on 8 bit for asm code
	extra_colormaps[mapnum].colormap = NULL;
	extra_colormaps[mapnum].maskcolor = (unsigned short)maskcolor;
	extra_colormaps[mapnum].fadecolor = (unsigned short)fadecolor;
	extra_colormaps[mapnum].maskamt = maskamt;
	extra_colormaps[mapnum].fadestart = (unsigned short)fadestart;
	extra_colormaps[mapnum].fadeend = (unsigned short)fadeend;
	extra_colormaps[mapnum].fog = fog;

#define ABS2(x) ((x) < 0 ? -(x) : (x))
	if(rendermode == render_soft)
	{
		colormap_p = Z_MallocAlign((256 * 34) + 10, PU_LEVEL, 0, 16);
		extra_colormaps[mapnum].colormap = (unsigned char *)colormap_p;

		for(p = 0; p < 34; p++)
		{
			for(i = 0; i < 256; i++)
			{
				*colormap_p = NearestColor((unsigned char)RoundUp(map[i][0]),
					(unsigned char)RoundUp(map[i][1]),
					(unsigned char)RoundUp(map[i][2]));
				colormap_p++;

				if((unsigned int)p < fadestart)
					continue;

				if(ABS2(map[i][0] - cdestr) > ABS2(deltas[i][0]))
					map[i][0] -= deltas[i][0];
				else
					map[i][0] = cdestr;

				if(ABS2(map[i][1] - cdestg) > ABS2(deltas[i][1]))
					map[i][1] -= deltas[i][1];
				else
					map[i][1] = cdestg;

				if(ABS2(map[i][2] - cdestb) > ABS2(deltas[i][1]))
					map[i][2] -= deltas[i][2];
				else
					map[i][2] = cdestb;
			}
		}
	}
#undef ABS2

	return;
}

// Thanks to quake2 source!
// utils3/qdata/images.c
static unsigned char NearestColor(unsigned char r, unsigned char g, unsigned char b)
{
	int dr, dg, db;
	int distortion, bestdistortion = 256 * 256 * 4, bestcolor = 0, i;

	for(i = 0; i < 256; i++)
	{
		dr = r - pLocalPalette[i].s.red;
		dg = g - pLocalPalette[i].s.green;
		db = b - pLocalPalette[i].s.blue;
		distortion = dr*dr + dg*dg + db*db;
		if(distortion < bestdistortion)
		{
			if(!distortion)
				return (unsigned char)i;

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return (unsigned char)bestcolor;
}

// Rounds off floating numbers and checks for 0 - 255 bounds
static int RoundUp(double number)
{
	if(number > 255.0)
		return 255;
	if(number < 0)
		return 0;

	if((int)number <= (int)(number - 0.5f))
		return (int)number + 1;

	return (int)number;
}

const char *R_ColormapNameForNum(int num)
{
	if(num == -1)
		return "NONE";

	if(num < 0 || num > MAXCOLORMAPS)
		I_Error("R_ColormapNameForNum: num is invalid!\n");

	if(foundcolormaps[num] == -1)
		return "INLEVEL";

	return wadfiles[foundcolormaps[num]>>16]->lumpinfo[foundcolormaps[num] & 0xffff].name;
}


//
// build a table for quick conversion from 8bpp to 15bpp
//

//
// added "static inline" keywords, linking with the debug version
// of allegro, it have a makecol15 function of it's own, now
// with "static inline" keywords,it sloves this problem ;)
//
static inline int makecol15(int r, int g, int b)
{
	return (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
}

static void R_Init8to16(void)
{
	byte* palette;
	int i;

	palette = W_CacheLumpName("PLAYPAL",PU_CACHE);

	for(i = 0; i < 256; i++)
	{
		// PLAYPAL uses 8 bit values
		color8to16[i] = (short)makecol15(palette[0], palette[1], palette[2]);
		palette += 3;
	}

	// test a big colormap
	hicolormaps = Z_Malloc(32768, PU_STATIC, 0);
	for(i = 0; i < 16384; i++)
		hicolormaps[i] = (short)(i<<1);
}

//
// R_InitData
//
// Locates all the lumps that will be used by all views
// Must be called after W_Init.
//
void R_InitData(void)
{
	if(highcolor)
	{
		CONS_Printf("\nInitHighColor...");
		R_Init8to16();
	}

	CONS_Printf("\nInitTextures...");
	R_LoadTextures();
	CONS_Printf("\nInitFlats...");
	R_InitFlats();

	CONS_Printf("\nInitSprites...\n");
	R_InitSpriteLumps();
	R_InitSprites(sprnames);

	CONS_Printf("\nInitColormaps...\n");
	R_InitColormaps();
}

//
// R_CheckTextureNumForName
//
// Check whether texture is available. Filter out NoTexture indicator.
//
int R_CheckTextureNumForName(const char *name)
{
	int i;

	// "NoTexture" marker.
	if(name[0] == '-')
		return 0;

	for(i = 0; i < numtextures; i++)
		if(!strncasecmp(textures[i]->name, name, 8))
			return i;

	CONS_Printf("WARNING: R_TextureNumForName: %.8s not found.\nDefaulting to REDWALL.\n", name);

	// Use a dummy texture for those not found.
	for(i = 0; i < numtextures; i++)
		if(!strncasecmp(textures[i]->name, "REDWALL", 8))
			return i;

	return -1;
}

//
// R_TextureNumForName
//
// Calls R_CheckTextureNumForName, aborts with error message.
//
int R_TextureNumForName(const char* name)
{
	int i;

	i = R_CheckTextureNumForName(name);

	if(i == -1)
	{
		CONS_Printf("WARNING: R_TextureNumForName: %.8s not found\n", name);
		return 1;
	}
	return i;
}

//
// R_PrecacheLevel
//
// Preloads all relevant graphics for the level.
//
void R_PrecacheLevel(void)
{
	char* texturepresent;
	char* spritepresent;

	int j, k, lump;
	size_t i;

	thinker_t* th;
	spriteframe_t* sf;

	if(demoplayback)
		return;

	// do not flush the memory, Z_Malloc twice with same user will cause error in Z_CheckHeap()
	if(rendermode != render_soft)
		return;

	// Precache flats.
	flatmemory = P_PrecacheLevelFlats();

	//
	// Precache textures.
	//
	// no need to precache all software textures in 3D mode
	// (note they are still used with the reference software view)
	texturepresent = alloca(numtextures);
	memset(texturepresent, 0, numtextures);

	for(j = 0; j < numsides; j++)
	{
		// huh, a potential bug here????
		if(sides[j].toptexture < numtextures)
			texturepresent[sides[j].toptexture] = 1;
		if(sides[j].midtexture < numtextures)
			texturepresent[sides[j].midtexture] = 1;
		if(sides[j].bottomtexture < numtextures)
			texturepresent[sides[j].bottomtexture] = 1;
	}

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to indicate a sky floor/ceiling as a flat,
	// while the sky texture is stored like a wall texture, with a skynum dependent name.
	texturepresent[skytexture] = 1;

	texturememory = 0;
	for(j = 0; j < numtextures; j++)
	{
		if(!texturepresent[j])
			continue;

		if(!texturecache[j])
			R_GenerateTexture(j);
		// pre-caching individual patches that compose textures became obsolete,
		// since we cache entire composite textures
	}

	//
	// Precache sprites.
	//
	spritepresent = alloca(numsprites);
	memset(spritepresent, 0, numsprites);

	for(th = thinkercap.next; th != &thinkercap; th = th->next)
		if(th->function.acp1 == (actionf_p1)P_MobjThinker)
			spritepresent[((mobj_t*)th)->sprite] = 1;

	spritememory = 0;
	for(i = 0; i < numsprites; i++)
	{
		if(!spritepresent[i])
			continue;

		for(j = 0; j < sprites[i].numframes; j++)
		{
			sf = &sprites[i].spriteframes[j];
			for(k = 0; k < 8; k++)
			{
				// see R_InitSprites for more about lumppat,lumpid
				lump = sf->lumppat[k];
				if(devparm)
					spritememory += W_LumpLength(lump);
				W_CachePatchNum(lump, PU_CACHE);
			}
		}
	}

	// FIXME: this is no more correct with glide render mode
	if(devparm)
	{
		CONS_Printf("Precache level done:\n"
			"flatmemory:    %ld k\n"
			"texturememory: %ld k\n"
			"spritememory:  %ld k\n", flatmemory>>10, texturememory>>10, spritememory>>10);
	}
}
