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
/// \brief Refresh of things, i.e. objects represented by sprites

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "r_local.h"
#include "sounds.h" // skin sounds
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_video.h" // rendermode
#include "r_things.h"
#include "p_tick.h"

static void R_InitSkins(void);

#define MINZ (FRACUNIT*4)
#define BASEYCENTER (BASEVIDHEIGHT/2)

// put this in transmap of visprite to draw a shade
#define VIS_SMOKESHADE ((void*)-1)

typedef struct
{
	int x1, x2;
	int column;
	int topclip, bottomclip;
} maskdraw_t;

// A drawnode is something that points to a 3D floor, 3D side, or masked
// middle texture. This is used for sorting with sprites.
typedef struct drawnode_s
{
	visplane_t* plane;
	drawseg_t* seg;
	drawseg_t* thickseg;
	ffloor_t* ffloor;
	vissprite_t* sprite;

	struct drawnode_s* next;
	struct drawnode_s* prev;
} drawnode_t;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t pspritescale;
fixed_t pspriteyscale;
fixed_t pspriteiscale;

static lighttable_t** spritelights;

// constant arrays used for psprite clipping and initializing clipping
short negonearray[MAXVIDWIDTH];
short screenheightarray[MAXVIDWIDTH];

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches
spritedef_t* sprites;
size_t numsprites;

static spriteframe_t sprtemp[64];
static int maxframe;
static const char* spritename;

// ==========================================================================
//
// Sprite loading routines: support sprites in pwad, dehacked sprite renaming,
// replacing not all frames of an existing sprite, add sprites at run-time,
// add wads at run-time.
//
// ==========================================================================

//
//
//
static void R_InstallSpriteLump(int lumppat,     // graphics patch
                                int lumpid,      // identifier
                                unsigned frame,
                                unsigned rotation,
                                boolean flipped)
{
	int r;

	if(frame >= 64 || rotation > 8)
		I_Error("R_InstallSpriteLump: Bad frame characters in lump %i", lumpid);

	if((int)frame > maxframe)
		maxframe = frame;

	if(rotation == 0)
	{
		// the lump should be used for all rotations
		if(sprtemp[frame].rotate == 0 && devparm)
			CONS_Printf("R_InitSprites: Sprite %s frame %c has multiple rot=0 lump\n",
				spritename, 'A'+frame);

		if(sprtemp[frame].rotate == 1 && devparm)
			CONS_Printf("R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump\n",
				spritename, 'A'+frame);

		sprtemp[frame].rotate = 0;
		for(r = 0; r < 8; r++)
		{
			sprtemp[frame].lumppat[r] = lumppat;
			sprtemp[frame].lumpid[r] = lumpid;
			sprtemp[frame].flip[r] = (byte)flipped;
		}
		return;
	}

	// the lump is only used for one rotation
	if(sprtemp[frame].rotate == 0 && devparm)
		CONS_Printf("R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump\n",
			spritename, 'A'+frame);

	sprtemp[frame].rotate = 1;

	// make 0 based
	rotation--;

	if(sprtemp[frame].lumpid[rotation] != -1 && devparm)
		CONS_Printf("R_InitSprites: Sprite %s: %c:%c has two lumps mapped to it\n",
			spritename, 'A'+frame, '1'+rotation);

	// lumppat & lumpid are the same for original Doom, but different
	// when using sprites in pwad : the lumppat points the new graphics
	sprtemp[frame].lumppat[rotation] = lumppat;
	sprtemp[frame].lumpid[rotation] = lumpid;
	sprtemp[frame].flip[rotation] = (byte)flipped;
}

// Install a single sprite, given its identifying name (4 chars)
//
// (originally part of R_AddSpriteDefs)
//
// Pass: name of sprite : 4 chars
//       spritedef_t
//       wadnum         : wad number, indexes wadfiles[], where patches
//                        for frames are found
//       startlump      : first lump to search for sprite frames
//       endlump        : AFTER the last lump to search
//
// Returns true if the sprite was succesfully added
//
static boolean R_AddSingleSpriteDef(const char* sprname, spritedef_t* spritedef, int wadnum, int startlump, int endlump)
{
    int l;
    int intname;
    int frame;
    int rotation;
    lumpinfo_t* lumpinfo;
    patch_t patch;

    intname = *(const int *)sprname;

    memset(sprtemp,-1, sizeof(sprtemp));
    maxframe = -1;

    // are we 'patching' a sprite already loaded ?
    // if so, it might patch only certain frames, not all
    if(spritedef->numframes) // (then spriteframes is not null)
    {
        // copy the already defined sprite frames
        memcpy(sprtemp, spritedef->spriteframes,
                spritedef->numframes * sizeof(spriteframe_t));
        maxframe = spritedef->numframes - 1;
    }

    // scan the lumps,
    //  filling in the frames for whatever is found
    lumpinfo = wadfiles[wadnum]->lumpinfo;
    if(endlump > wadfiles[wadnum]->numlumps)
        endlump = wadfiles[wadnum]->numlumps;

    for(l = startlump; l<endlump; l++)
    {
        if(*(int*)lumpinfo[l].name == intname)
        {
            frame = lumpinfo[l].name[4] - 'A';
            rotation = lumpinfo[l].name[5] - '0';

            // skip NULL sprites from very old dmadds pwads
            if(W_LumpLength((wadnum<<16)+l)<=8)
                continue;

            // store sprite info in lookup tables
            //FIXME:numspritelumps do not duplicate sprite replacements
            W_ReadLumpHeader ((wadnum<<16)+l, &patch, sizeof(patch_t));
            spritewidth[numspritelumps] = SHORT(patch.width)<<FRACBITS;
            spriteoffset[numspritelumps] = SHORT(patch.leftoffset)<<FRACBITS;
            spritetopoffset[numspritelumps] = SHORT(patch.topoffset)<<FRACBITS;
            spriteheight[numspritelumps] = SHORT(patch.height)<<FRACBITS;

#ifdef HWRENDER
            //BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
            if( rendermode != render_soft && rendermode != render_none // not for psprite
                && SHORT(patch.topoffset)>0 && SHORT(patch.topoffset)<SHORT(patch.height))
                // perfect is patch.height but sometime it is too high
                spritetopoffset[numspritelumps] = min(SHORT(patch.topoffset+4),SHORT(patch.height))<<FRACBITS;

#endif

            //----------------------------------------------------

            R_InstallSpriteLump((wadnum<<16)+l, numspritelumps, frame, rotation, false);

            if(lumpinfo[l].name[6])
            {
                frame = lumpinfo[l].name[6] - 'A';
                rotation = lumpinfo[l].name[7] - '0';
                R_InstallSpriteLump((wadnum<<16)+l, numspritelumps, frame, rotation, true);
            }

            if(++numspritelumps>=MAXSPRITELUMPS)
                I_Error("R_AddSingleSpriteDef: too much sprite replacements (numspritelumps)\n");
        }
    }

    //
    // if no frames found for this sprite
    //
    if(maxframe == -1)
    {
        // the first time (which is for the original wad),
        // all sprites should have their initial frames
        // and then, patch wads can replace it
        // we will skip non-replaced sprite frames, only if
        // they have already have been initially defined (original wad)

        //check only after all initial pwads added
        //if(spritedef->numframes == 0)
        //    I_Error("R_AddSpriteDefs: no initial frames found for sprite %s\n",
        //             namelist[i]);

        // sprite already has frames, and is not replaced by this wad
        return false;
    }

    maxframe++;

    //
    //  some checks to help development
    //
    for(frame = 0; frame < maxframe; frame++)
    {
        switch((int)sprtemp[frame].rotate)
        {
          case -1:
            // no rotations were found for that frame at all
            I_Error ("R_InitSprites: No patches found "
                     "for %s frame %c", sprname, frame+'A');
            break;

          case 0:
            // only the first rotation is needed
            break;

          case 1:
            // must have all 8 frames
            for (rotation=0 ; rotation<8 ; rotation++)
                // we test the patch lump, or the id lump whatever
                // if it was not loaded the two are -1
                if(sprtemp[frame].lumppat[rotation] == -1)
                    I_Error("R_InitSprites: Sprite %s frame %c "
                             "is missing rotations",
                             sprname, frame+'A');
            break;
        }
    }

    // allocate space for the frames present and copy sprtemp to it
    if(spritedef->numframes &&             // has been allocated
        spritedef->numframes < maxframe)   // more frames are defined ?
    {
        Z_Free(spritedef->spriteframes);
        spritedef->spriteframes = NULL;
    }

    // allocate this sprite's frames
    if(spritedef->spriteframes == NULL)
        spritedef->spriteframes =
            Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);

    spritedef->numframes = maxframe;
    memcpy(spritedef->spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));

    return true;
}



//
// Search for sprites replacements in a wad whose names are in namelist
//
void R_AddSpriteDefs(const char** namelist, int wadnum)
{
    size_t i;
    int start, end;
    int addsprites;

    // find the sprites section in this pwad
    // we need at least the S_END
    // (not really, but for speedup)

    start = W_CheckNumForNamePwad("S_START", wadnum, 0);
    if(start == -1)
        start = W_CheckNumForNamePwad("SS_START", wadnum, 0); //deutex compatib.
    if(start == -1)
        start = 0; // search frames from start of wad (lumpnum low word is 0)
    else
        start++;   // just after S_START

    start &= 0xFFFF;    // 0 based in lumpinfo

    end = W_CheckNumForNamePwad("S_END",wadnum,0);
    if(end == -1)
        end = W_CheckNumForNamePwad("SS_END",wadnum,0);     //deutex compatib.
    if(end == -1)
    {
        if(devparm)
            CONS_Printf("no sprites in pwad %d\n", wadnum);
        return;
        //I_Error ("R_AddSpriteDefs: S_END, or SS_END missing for sprites "
        //         "in pwad %d\n",wadnum);
    }
    end &= 0xFFFF;

    //
    // scan through lumps, for each sprite, find all the sprite frames
    //
    addsprites = 0;
    for(i = 0; i < numsprites; i++)
    {
        spritename = namelist[i];

        if(R_AddSingleSpriteDef(spritename, &sprites[i], wadnum, start, end))
        {
            // if a new sprite was added (not just replaced)
            addsprites++;
            if(devparm)
                CONS_Printf("sprite %s set in pwad %d\n", namelist[i], wadnum);//Fab
        }
    }

	CONS_Printf("%d sprites added from file %s\n", addsprites, wadfiles[wadnum]->filename);
}

//
// GAME FUNCTIONS
//
static vissprite_t vissprites[MAXVISSPRITES];
static vissprite_t* vissprite_p;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(const char** namelist)
{
	int i;
	const char** check;

	for(i = 0; i < MAXVIDWIDTH; i++)
	{
		negonearray[i] = -1;
	}

	//
	// count the number of sprite names, and allocate sprites table
	//
	check = namelist;
	while(*check != NULL)
		check++;
	numsprites = check - namelist;

	if(!numsprites)
		I_Error("R_AddSpriteDefs: no sprites in namelist\n");

	sprites = Z_Malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);
	memset(sprites, 0, numsprites * sizeof(*sprites));

	// find sprites in each -file added pwad
	for(i = 0; i < numwadfiles; i++)
		R_AddSpriteDefs(namelist, i);

	//
	// now check for skins
	//

	// it can be is do before loading config for skin cvar possible value
	R_InitSkins();
	for(i=0; i<numwadfiles; i++)
		R_AddSkins(i);

	//
	// check if all sprites have frames
	//
	/*
	for(i = 0; i < numsprites; i++)
		if(sprites[i].numframes<1)
			CONS_Printf ("R_InitSprites: sprite %s has no frames at all\n", sprnames[i]);
	*/
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void)
{
	vissprite_p = vissprites;
}

//
// R_NewVisSprite
//
static vissprite_t overflowsprite;

static vissprite_t* R_NewVisSprite(void)
{
	if(vissprite_p == &vissprites[MAXVISSPRITES])
		return &overflowsprite;

	vissprite_p++;
	return vissprite_p-1;
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short* mfloorclip;
short* mceilingclip;

fixed_t spryscale, sprtopscreen, sprbotscreen;
fixed_t windowtop, windowbottom;

void R_DrawMaskedColumn(column_t* column)
{
	int topscreen;
	int bottomscreen;
	fixed_t basetexturemid;

	basetexturemid = dc_texturemid;

	for(; column->topdelta != 0xff; )
	{
		// calculate unclipped screen coordinates
		// for post
		topscreen = sprtopscreen + spryscale*column->topdelta;
		bottomscreen = sprbotscreen == MAXINT ? topscreen + spryscale*column->length
		                                      : sprbotscreen + spryscale*column->length;

		dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		whereitsfrom = 13;
		dc_yh = (bottomscreen-1)>>FRACBITS;

		if(windowtop != MAXINT && windowbottom != MAXINT)
		{
			if(windowtop > topscreen)
			{
				dc_yl = (windowtop + FRACUNIT - 1) >> FRACBITS;
				whereitsfrom = 15;
			}
			if(windowbottom < bottomscreen)
				dc_yh = (windowbottom - 1) >> FRACBITS;
		}

		if(dc_yh >= mfloorclip[dc_x])
			dc_yh = mfloorclip[dc_x]-1;
		if(dc_yl <= mceilingclip[dc_x])
		{
			dc_yl = mceilingclip[dc_x]+1;
			// don't set whereitsfrom here, dc_yl can't be too low now unless
			// it was already too low before
		}
		if(dc_yl < 0)
			dc_yl = 0;
		if(dc_yh >= vid.height)
			dc_yh = vid.height - 1;

		if(dc_yl <= dc_yh && dc_yl < vid.height && dc_yh > 0)
		{
			dc_source = (byte*)column + 3;
			dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);

			// Drawn by R_DrawColumn.
			// This stuff is a likely cause of the splitscreen water crash bug.
			// FIXTHIS: Figure out what "something more proper" is and do it.
			// quick fix... something more proper should be done!!!
			if(ylookup[dc_yl])
				colfunc();
			else if(colfunc == R_DrawColumn_8
#ifdef USEASM
			|| colfunc == R_DrawColumn_8_ASM || colfunc == R_DrawColumn_8_Pentium
			|| colfunc == R_DrawColumn_8_NOMMX || colfunc == R_DrawColumn_8_K6_MMX
#endif
			)
			{
				static int first = 1;
				if(first)
				{
					CONS_Printf("WARNING: avoiding a crash in %s %d\n", __FILE__, __LINE__);
					first = 0;
				}
			}
		}
		column = (column_t*)((byte*)column + column->length + 4);
	}

	dc_texturemid = basetexturemid;
}

//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
static void R_DrawVisSprite(vissprite_t* vis, int x1, int x2)
{
    column_t* column;
    int texturecolumn;
    fixed_t frac;
    patch_t* patch;


    //Fab:R_InitSprites now sets a wad lump number
	 x1 = x2 = 0;
    patch = W_CacheLumpNum(vis->patch, PU_CACHE);

    dc_colormap = vis->colormap;
	if(vis->mobj && vis->mobjflags & MF_TRANSLATION && vis->transmap) // Map it to the color of the player that thokked it. Tails 08-20-2002
	{
		colfunc = transtransfunc;
		dc_transmap = vis->transmap;
		dc_translation = defaulttranslationtables - 256 +
			((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8));
	}
    else if(vis->transmap==VIS_SMOKESHADE)
        // shadecolfunc uses 'colormaps'
        colfunc = shadecolfunc;
    else if(vis->transmap)
    {
        colfunc = fuzzcolfunc;
        dc_transmap = vis->transmap;    //Fab:29-04-98: translucency table
    }
    else if(vis->mobjflags & MF_TRANSLATION)
    {
        // translate green skin to another color
        colfunc = transcolfunc;

		// New colormap stuff for skins Tails 06-07-2002
		if(vis->mobj->player) // This thing is a player!
		{
			if(vis->mobj->player->skincolor)
				dc_translation = translationtables[vis->mobj->player->skin] - 256 +
					((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8));
			else
			{
				static int firsttime = 1;
				colfunc = basecolfunc; // Graue 04-08-2004
				if(firsttime)
				{
					CONS_Printf("Abandoning!\n");
					firsttime = 0;
				}
			}
		}
		else if((vis->mobj->flags & MF_BOSS) && (vis->mobj->flags2 & MF2_FRET) && (leveltime & 1)) // Bosses "flash"
		{
			dc_translation = bosstranslationtables;
		}
		else // Use the defaults
		{
			dc_translation = defaulttranslationtables - 256 +
				((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8));
		}
    }

	if(vis->extra_colormap)
	{
		if(!dc_colormap)
			dc_colormap = vis->extra_colormap->colormap;
		else
			dc_colormap = &vis->extra_colormap->colormap[dc_colormap - colormaps];
	}
	if(!dc_colormap)
		dc_colormap = colormaps;

	//dc_iscale = abs(vis->xiscale)>>detailshift;  ???
	dc_iscale = FixedDiv(FRACUNIT, vis->scale);
	dc_texturemid = vis->texturemid;
	dc_texheight = 0;

	frac = vis->startfrac;
	spryscale = vis->scale;
	sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
	windowtop = windowbottom = sprbotscreen = MAXINT;
	if(vis->mobjflags & MF_HIRES)
	{
		spryscale /= 2;
		vis->scale /= 2;
		dc_iscale = FixedDiv(FRACUNIT, vis->scale);
		vis->xiscale *= 2;
		dc_hires = 1;
	}

	for(dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale)
	{
		if(dc_x < 0) dc_x=0;
		if(dc_x>=vid.width) dc_x = vid.width-1;
		texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
		if(texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
			I_Error("R_DrawSpriteRange: bad texturecolumn");
#endif
		column = (column_t*)((byte*)patch + LONG(patch->columnofs[texturecolumn]));
		R_DrawMaskedColumn(column);
	}

	colfunc = basecolfunc;
	if(vis->mobjflags & MF_HIRES)
	{
		spryscale *= 2;
		vis->scale *= 2;
		dc_iscale /= 2;
		vis->xiscale /= 2;
		dc_hires = 0;
	}
}

// Special precipitation drawer Tails 08-18-2002
static void R_DrawPrecipitationVisSprite(vissprite_t* vis, int x1, int x2)
{
	column_t* column;
	int texturecolumn;
	fixed_t frac;
	patch_t* patch;

	//Fab:R_InitSprites now sets a wad lump number
	patch = W_CacheLumpNum(vis->patch, PU_CACHE);

	dc_colormap = vis->colormap;
	if(vis->transmap)
	{
		colfunc = fuzzcolfunc;
		dc_transmap = vis->transmap;    //Fab:29-04-98: translucency table
	}

	dc_colormap = colormaps;

	dc_iscale = FixedDiv(FRACUNIT, vis->scale);
	dc_texturemid = vis->texturemid;
	dc_texheight = 0;

	frac = vis->startfrac;
	spryscale = vis->scale;
	sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
	windowtop = windowbottom = sprbotscreen = MAXINT;

	for(dc_x = x1; dc_x <= x2; dc_x++, frac += vis->xiscale)
	{
		if(dc_x < 0) dc_x=0;
		if(dc_x>=vid.width) dc_x = vid.width-1;
		texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
		if(texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
			I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
		column = (column_t*)((byte*)patch + LONG(patch->columnofs[texturecolumn]));
		R_DrawMaskedColumn(column);
	}

	colfunc = basecolfunc;
}

//
// R_SplitSprite
// runs through a sector's lightlist and
static void R_SplitSprite(vissprite_t* sprite, mobj_t* thing)
{
	int i, lightnum, index;
	fixed_t cutfrac;
	sector_t* sector;
	vissprite_t* newsprite;

	sector = sprite->sector;

	for(i = 1; i < sector->numlights; i++)
	{
		if(sector->lightlist[i].height >= sprite->gzt || !(sector->lightlist[i].caster->flags & FF_CUTSPRITES))
			continue;
		if(sector->lightlist[i].height <= sprite->gz)
			return;

		cutfrac = (centeryfrac - FixedMul(sector->lightlist[i].height - viewz, sprite->scale)) >> FRACBITS;
		if(cutfrac < 0)
			continue;
		if(cutfrac > vid.height)
			return;

		// Found a split! Make a new sprite, copy the old sprite to it, and
		// adjust the heights.
		newsprite = R_NewVisSprite();
		memcpy(newsprite, sprite, sizeof(vissprite_t));

		sprite->cut |= SC_BOTTOM;
		sprite->gz = sector->lightlist[i].height;

		newsprite->gzt = sprite->gz;

		sprite->sz = cutfrac;
		newsprite->szt = sprite->sz - 1;

		if(sector->lightlist[i].height < sprite->pzt && sector->lightlist[i].height > sprite->pz)
			sprite->pz = newsprite->pzt = sector->lightlist[i].height;
		else
		{
			newsprite->pz = newsprite->gz;
			newsprite->pzt = newsprite->gzt;
		}

		newsprite->cut |= SC_TOP;
		if(!(sector->lightlist[i].caster->flags & FF_NOSHADE))
		{
			lightnum = (*sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT);

			if(lightnum < 0)
				spritelights = scalelight[0];
			else if(lightnum >= LIGHTLEVELS)
				spritelights = scalelight[LIGHTLEVELS-1];
			else
				spritelights = scalelight[lightnum];

			newsprite->extra_colormap = sector->lightlist[i].extra_colormap;

			if(thing->frame & FF_SMOKESHADE)
				;
			else
			{
/*        if(thing->frame & FF_TRANSMASK)
          ;
        else if(thing->flags2 & MF2_SHADOW)
          ;*/

				if(!((thing->frame & (FF_FULLBRIGHT|FF_TRANSMASK) || thing->flags2 & MF2_SHADOW)
					&& (!newsprite->extra_colormap || !newsprite->extra_colormap->fog)))
				{
					index = sprite->xscale>>(LIGHTSCALESHIFT);

					if(index >= MAXLIGHTSCALE)
						index = MAXLIGHTSCALE-1;
					newsprite->colormap = spritelights[index];
				}
			}
		}
		sprite = newsprite;
	}
}

//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
static void R_ProjectSprite(mobj_t* thing)
{
    fixed_t tr_x, tr_y;
	fixed_t gxt, gyt;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added:02-02-98:aaargll..if I were a math-guy!!!

	int x1, x2;

	spritedef_t* sprdef;
	spriteframe_t* sprframe;
	int lump;

    unsigned rot;
    boolean flip;

    int index;

    vissprite_t* vis;

    angle_t ang;
    fixed_t iscale;

    //SoM: 3/17/2000
    fixed_t gzt;
    int heightsec;
    int light = 0;

	// transform the origin point
    tr_x = thing->x - viewx;
    tr_y = thing->y - viewy;

    gxt = FixedMul(tr_x, viewcos);
    gyt = -FixedMul(tr_y, viewsin);

    tz = gxt-gyt;

    // thing is behind view plane?
    if(tz < MINZ)
        return;

    // aspect ratio stuff :
	xscale = FixedDiv(projection, tz);
	yscale = FixedDiv(projectiony, tz);

    gxt = -FixedMul(tr_x,viewsin);
    gyt = FixedMul(tr_y,viewcos);
    tx = -(gyt+gxt);

    // too far off the side?
    if(abs(tx)>(tz<<2))
        return;

    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if((unsigned)thing->sprite >= numsprites)
        I_Error("R_ProjectSprite: invalid sprite number %i ",
                 thing->sprite);
#endif

    //Fab:02-08-98: 'skin' override spritedef currently used for skin
    if(thing->skin)
        sprdef = &((skin_t*)thing->skin)->spritedef;
    else
        sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
    if((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame %i : %i for %s",
                 thing->sprite, thing->frame, sprnames[thing->sprite]);
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

#ifdef PARANOIA
    //heretic hack
    if(!sprframe)
        I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

    if(sprframe->rotate)
    {
        // choose a different rotation based on player view
        ang = R_PointToAngle (thing->x, thing->y);
        rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
        //Fab: lumpid is the index for spritewidth,spriteoffset... tables
        lump = sprframe->lumpid[rot];
        flip = (boolean)sprframe->flip[rot];
    }
    else
    {
        // use single rotation for all views
        rot = 0;                        //Fab: for vis->patch below
        lump = sprframe->lumpid[0];     //Fab: see note above
        flip = (boolean)sprframe->flip[0];
    }

    // calculate edges of the shape
	if(thing->flags & MF_HIRES)
		tx -= spriteoffset[lump]/2;
	else
		tx -= spriteoffset[lump];
    x1 = (centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    // off the right side?
    if(x1 > viewwidth)
        return;

	if(thing->flags & MF_HIRES)
		tx += spritewidth[lump]/2;
	else
		tx += spritewidth[lump];
    x2 = ((centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    // off the left side
    if(x2 < 0)
        return;

    //SoM: 3/17/2000: Disreguard sprites that are out of view..
	if(thing->flags & MF_HIRES)
		gzt = thing->z + spritetopoffset[lump]/2;
	else
		gzt = thing->z + spritetopoffset[lump];

	if(thing->subsector->sector->numlights)
	{
		int lightnum;
		light = R_GetPlaneLight(thing->subsector->sector, gzt, false);
		lightnum = (*thing->subsector->sector->lightlist[light].lightlevel >> LIGHTSEGSHIFT);

		if(lightnum < 0)
			spritelights = scalelight[0];
		else if(lightnum >= LIGHTLEVELS)
			spritelights = scalelight[LIGHTLEVELS-1];
		else
			spritelights = scalelight[lightnum];
	}

	heightsec = thing->subsector->sector->heightsec;

    if(heightsec != -1)   // only clip things which are in special sectors
    {
      int phs = viewplayer->mo->subsector->sector->heightsec;
      if(phs != -1 && viewz < sectors[phs].floorheight ?
          thing->z >= sectors[heightsec].floorheight :
          gzt < sectors[heightsec].floorheight)
        return;
      if(phs != -1 && viewz > sectors[phs].ceilingheight ?
          gzt < sectors[heightsec].ceilingheight &&
          viewz >= sectors[heightsec].ceilingheight :
          thing->z >= sectors[heightsec].ceilingheight)
        return;
    }

    // store information in a vissprite
    vis = R_NewVisSprite();
    vis->heightsec = heightsec; //SoM: 3/17/2000
    vis->mobjflags = thing->flags;
    vis->scale = yscale;           //<<detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
	if(thing->flags & MF_HIRES)
		vis->gz = gzt - spriteheight[lump]/2;
	else
		vis->gz = gzt - spriteheight[lump];
    vis->gzt = gzt;
    vis->thingheight = thing->height;
        vis->pz = thing->z;
        vis->pzt = vis->pz + vis->thingheight;
    vis->texturemid = vis->gzt - viewz;

	vis->mobj = thing; // Easy access! Tails 06-07-2002

    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
    vis->xscale = xscale; //SoM: 4/17/2000
    vis->sector = thing->subsector->sector;
    vis->szt = (centeryfrac - FixedMul(vis->gzt - viewz, yscale)) >> FRACBITS;
    vis->sz = (centeryfrac - FixedMul(vis->gz - viewz, yscale)) >> FRACBITS;
    vis->cut = false;
    if(thing->subsector->sector->numlights)
      vis->extra_colormap = thing->subsector->sector->lightlist[light].extra_colormap;
    else
      vis->extra_colormap = thing->subsector->sector->extra_colormap;

    iscale = FixedDiv(FRACUNIT, xscale);

    if(flip)
    {
		vis->startfrac = spritewidth[lump]-1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if(vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);

    //Fab: lumppat is the lump number of the patch to use, this is different
    //     than lumpid for sprites-in-pwad : the graphics are patched
    vis->patch = sprframe->lumppat[rot];

//
// determine the colormap (lightlevel & special effects)
//
    vis->transmap = NULL;

    // specific translucency
    if(thing->frame & FF_SMOKESHADE)
        // not realy a colormap ... see R_DrawVisSprite
        vis->colormap = VIS_SMOKESHADE;
    else
    {
        if(thing->frame & FF_TRANSMASK)
            vis->transmap = (thing->frame & FF_TRANSMASK) - 0x10000 + transtables;
        else if(thing->flags2 & MF2_SHADOW)
            // actually only the player should use this (temporary invisibility)
            // because now the translucency is set through FF_TRANSMASK
            vis->transmap = ((tr_transhi-1)<<FF_TRANSSHIFT) + transtables;

		if(((thing->frame & (FF_FULLBRIGHT|FF_TRANSMASK)) || (thing->flags2 & MF2_SHADOW))
			&& (!vis->extra_colormap || !vis->extra_colormap->fog))
		{
			// full bright: goggles
			vis->colormap = colormaps;
		}
        else
        {

            // diminished light
            index = xscale>>(LIGHTSCALESHIFT);

            if(index >= MAXLIGHTSCALE)
                index = MAXLIGHTSCALE-1;

            vis->colormap = spritelights[index];
        }
    }

	vis->precip = false;

    if(thing->subsector->sector->numlights)
      R_SplitSprite(vis, thing);

}

static void R_ProjectPrecipitationSprite(precipmobj_t* thing)
{
	fixed_t tr_x, tr_y;
	fixed_t gxt, gyt;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added:02-02-98:aaargll..if I were a math-guy!!!

	int x1, x2;

	spritedef_t* sprdef;
	spriteframe_t* sprframe;
	int lump;

	vissprite_t* vis;

	fixed_t iscale;

	//SoM: 3/17/2000
	fixed_t gzt;
	//int light = 0;

	// transform the origin point
	tr_x = thing->x - viewx;
	tr_y = thing->y - viewy;

	gxt = FixedMul(tr_x,viewcos);
	gyt = -FixedMul(tr_y,viewsin);

	tz = gxt-gyt;

	// thing is behind view plane?
	if(tz < MINZ)
		return;

	// aspect ratio stuff :
	xscale = FixedDiv(projection, tz);
	yscale = FixedDiv(projectiony, tz);

	gxt = -FixedMul(tr_x,viewsin);
	gyt = FixedMul(tr_y,viewcos);
	tx = -(gyt+gxt);

	// too far off the side?
	if(abs(tx)>(tz<<2))
		return;

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if((unsigned)thing->sprite >= numsprites)
		I_Error("R_ProjectSprite: invalid sprite number %i ",
			thing->sprite);
#endif

	sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
	if((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
		I_Error("R_ProjectSprite: invalid sprite frame %i : %i for %s",
			thing->sprite, thing->frame, sprnames[thing->sprite]);
#endif
	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

#ifdef PARANOIA
	//heretic hack
	if(!sprframe)
		I_Error("sprframes NULL for sprite %d\n", thing->sprite);
#endif

	// use single rotation for all views
	lump = sprframe->lumpid[0];     //Fab: see note above

	// calculate edges of the shape
	tx -= spriteoffset[lump];
	x1 = (centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

	// off the right side?
	if(x1 > viewwidth)
		return;

	tx += spritewidth[lump];
	x2 = ((centerxfrac + FixedMul (tx,xscale)) >>FRACBITS) - 1;

	// off the left side
	if(x2 < 0)
		return;

	//SoM: 3/17/2000: Disreguard sprites that are out of view..
	gzt = thing->z + spritetopoffset[lump];

	// store information in a vissprite
	vis = R_NewVisSprite();
	vis->scale = yscale;           //<<detailshift;
	vis->gx = thing->x;
	vis->gy = thing->y;
	vis->gz = gzt - spriteheight[lump];
	vis->gzt = gzt;
	vis->thingheight = 4*FRACUNIT;
	vis->pz = thing->z;
	vis->pzt = vis->pz + vis->thingheight;
	vis->texturemid = vis->gzt - viewz;

	vis->x1 = x1 < 0 ? 0 : x1;
	vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis->xscale = xscale; //SoM: 4/17/2000
	vis->sector = thing->subsector->sector;
	vis->szt = (centeryfrac - FixedMul(vis->gzt - viewz, yscale)) >> FRACBITS;
	vis->sz = (centeryfrac - FixedMul(vis->gz - viewz, yscale)) >> FRACBITS;

	iscale = FixedDiv(FRACUNIT, xscale);

	vis->startfrac = 0;
	vis->xiscale = iscale;

	if(vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);

	//Fab: lumppat is the lump number of the patch to use, this is different
	//     than lumpid for sprites-in-pwad : the graphics are patched
	vis->patch = sprframe->lumppat[0];

	// specific translucency
	if(thing->frame & FF_TRANSMASK)
		vis->transmap = (thing->frame & FF_TRANSMASK) - 0x10000 + transtables;

	// Fullbright
	vis->colormap = colormaps;
	vis->precip = true;
}

// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites(sector_t* sec, int lightlevel)
{
	mobj_t* thing;
	precipmobj_t* precipthing; // Tails 08-25-2002
	int lightnum;
	fixed_t adx, ady, approx_dist;

	if(rendermode != render_soft)
		return;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if(sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	if(!sec->numlights)
	{
		if(sec->heightsec == -1) lightlevel = sec->lightlevel;

		lightnum = (lightlevel >> LIGHTSEGSHIFT);

		if(lightnum < 0)
			spritelights = scalelight[0];
		else if(lightnum >= LIGHTLEVELS)
			spritelights = scalelight[LIGHTLEVELS-1];
		else
			spritelights = scalelight[lightnum];
	}

	// Handle all things in sector.

	// NiGHTS stages have a draw distance limit because of the
	// HUGE number of SPRiTES!
	if(maptol & TOL_NIGHTS)
	{
		// Special function for precipitation Tails 08-18-2002
		for(thing = sec->thinglist; thing; thing = thing->snext)
		{
			if(!thing)
				continue;

			if((thing->flags2 & MF2_DONTDRAW)==0)
			{
				adx = abs(players[displayplayer].mo->x - thing->x);
				ady = abs(players[displayplayer].mo->y - thing->y);

				// From _GG1_ p.428. Approx. eucledian distance fast.
				approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

				if(approx_dist < NIGHTS_DRAW_DIST)
					R_ProjectSprite(thing);
				else if(cv_splitscreen.value && players[secondarydisplayplayer].mo)
				{
					adx = abs(players[secondarydisplayplayer].mo->x - thing->x);
					ady = abs(players[secondarydisplayplayer].mo->y - thing->y);

					// From _GG1_ p.428. Approx. eucledian distance fast.
					approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

					if(approx_dist < NIGHTS_DRAW_DIST)
						R_ProjectSprite (thing);
				}
			}
		}
	}
	else
	{
		for(thing = sec->thinglist; thing; thing = thing->snext)
		{
			if(!thing)
				continue;

			if((thing->flags2 & MF2_DONTDRAW)==0)
				R_ProjectSprite(thing);

			if(!thing->snext)
				break;
		}
	}

	// Special function for precipitation Tails 08-18-2002
	for(precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
	{
		if(!precipthing)
			continue;

		adx = abs(players[displayplayer].mo->x - precipthing->x);
		ady = abs(players[displayplayer].mo->y - precipthing->y);

		// From _GG1_ p.428. Approx. eucledian distance fast.
		approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

		// Only draw the precipitation oh-so-far from the player.
		if(approx_dist < (cv_precipdist.value << FRACBITS))
			R_ProjectPrecipitationSprite(precipthing);
		else if(cv_splitscreen.value && players[secondarydisplayplayer].mo)
		{
			adx = abs(players[secondarydisplayplayer].mo->x - precipthing->x);
			ady = abs(players[secondarydisplayplayer].mo->y - precipthing->y);

			// From _GG1_ p.428. Approx. eucledian distance fast.
			approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

			if(approx_dist < (cv_precipdist.value << FRACBITS))
				R_ProjectPrecipitationSprite (precipthing);
		}
	}
}

//
// R_SortVisSprites
//
static vissprite_t     vsprsortedhead;


void R_SortVisSprites(void)
{
    size_t              i, count;
    vissprite_t*        ds;
    vissprite_t*        best=NULL;      //shut up compiler
    vissprite_t         unsorted;
    fixed_t             bestscale;

    count = vissprite_p - vissprites;

    unsorted.next = unsorted.prev = &unsorted;

    if(!count)
        return;

    for(ds=vissprites ; ds<vissprite_p ; ds++)
    {
        ds->next = ds+1;
        ds->prev = ds-1;
    }

    vissprites[0].prev = &unsorted;
    unsorted.next = &vissprites[0];
    (vissprite_p-1)->next = &unsorted;
    unsorted.prev = vissprite_p-1;

    // pull the vissprites out by scale
    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for(i = 0; i < count; i++)
    {
        bestscale = MAXINT;
        for(ds = unsorted.next; ds != &unsorted; ds=ds->next)
        {
            if(ds->scale < bestscale)
            {
                bestscale = ds->scale;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &vsprsortedhead;
        best->prev = vsprsortedhead.prev;
        vsprsortedhead.prev->next = best;
        vsprsortedhead.prev = best;
    }
}



//
// R_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static void R_CreateDrawNodes();
static drawnode_t* R_CreateDrawNode(drawnode_t* link);

static drawnode_t nodebankhead;
static drawnode_t nodehead;

static void R_CreateDrawNodes()
{
	drawnode_t* entry;
	drawseg_t* ds;
	int i, p, best, x1, x2;
	fixed_t bestdelta, delta;
	vissprite_t* rover;
	drawnode_t* r2;
	visplane_t* plane;
	int sintersect;
	fixed_t gzm;
	fixed_t scale;

    // Add the 3D floors, thicksides, and masked textures...
    for(ds = ds_p; ds-- > drawsegs;)
    {
      if(ds->numthicksides)
      {
        for(i = 0; i < ds->numthicksides; i++)
        {
          entry = R_CreateDrawNode(&nodehead);
          entry->thickseg = ds;
          entry->ffloor = ds->thicksides[i];
        }
      }
      if(ds->maskedtexturecol)
      {
        entry = R_CreateDrawNode(&nodehead);
        entry->seg = ds;
      }
      if(ds->numffloorplanes)
      {
        for(i = 0; i < ds->numffloorplanes; i++)
        {
          best = -1;
          bestdelta = 0;
          for(p = 0; p < ds->numffloorplanes; p++)
          {
            if(!ds->ffloorplanes[p])
              continue;
            plane = ds->ffloorplanes[p];
            R_PlaneBounds(plane);
            if(plane->low < con_clipviewtop || plane->high > vid.height || plane->high > plane->low)
            {
              ds->ffloorplanes[p] = NULL;
              continue;
            }

            delta = abs(plane->height - viewz);
            if(delta > bestdelta)
            {
              best = p;
              bestdelta = delta;
            }
          }
          if(best != -1)
          {
            entry = R_CreateDrawNode(&nodehead);
            entry->plane = ds->ffloorplanes[best];
            entry->seg = ds;
            ds->ffloorplanes[best] = NULL;
          }
          else
            break;
        }
      }
    }

    if(vissprite_p == vissprites)
      return;

    R_SortVisSprites();
    for(rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
    {
      if(rover->szt > vid.height || rover->sz < 0)
        continue;

      sintersect = (rover->x1 + rover->x2) / 2;
      gzm = (rover->gz + rover->gzt) / 2;

      for(r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
      {
        if(r2->plane)
        {
          if(r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
            continue;
          if(rover->szt > r2->plane->low || rover->sz < r2->plane->high)
            continue;

          if((r2->plane->height < viewz && rover->pz < r2->plane->height) ||
            (r2->plane->height > viewz && rover->pzt > r2->plane->height))
          {
            // SoM: NOTE: Because a visplane's shape and scale is not directly
            // bound to any single lindef, a simple poll of it's frontscale is
            // not adequate. We must check the entire frontscale array for any
            // part that is in front of the sprite.

            x1 = rover->x1;
            x2 = rover->x2;
            if(x1 < r2->plane->minx) x1 = r2->plane->minx;
            if(x2 > r2->plane->maxx) x2 = r2->plane->maxx;

            for(i = x1; i <= x2; i++)
            {
              if(r2->seg->frontscale[i] > rover->scale)
                break;
            }
            if(i > x2)
              continue;

            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->thickseg)
        {
          if(rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
            continue;

          scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
          if(scale <= rover->scale)
            continue;
          scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
          if(scale <= rover->scale)
            continue;

          if((*r2->ffloor->topheight > viewz && *r2->ffloor->bottomheight < viewz) ||
            (*r2->ffloor->topheight < viewz && rover->gzt < *r2->ffloor->topheight) ||
            (*r2->ffloor->bottomheight > viewz && rover->gz > *r2->ffloor->bottomheight))
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->seg)
        {
          if(rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
            continue;

          scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
          if(scale <= rover->scale)
            continue;
          scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

          if(rover->scale < scale)
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->sprite)
        {
          if(r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
            continue;
          if(r2->sprite->szt > rover->sz || r2->sprite->sz < rover->szt)
            continue;

          if(r2->sprite->scale > rover->scale)
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
      }
      if(r2 == &nodehead)
      {
        entry = R_CreateDrawNode(&nodehead);
        entry->sprite = rover;
      }
    }
}




static drawnode_t* R_CreateDrawNode (drawnode_t* link)
{
  drawnode_t* node;

  node = nodebankhead.next;
  if(node == &nodebankhead)
  {
    node = malloc(sizeof(drawnode_t));
    if(!node)
      I_Error("No more free memory to CreateDrawNode");
  }
  else
    (nodebankhead.next = node->next)->prev = &nodebankhead;

  if(link)
  {
    node->next = link;
    node->prev = link->prev;
    link->prev->next = node;
    link->prev = node;
  }

  node->plane = NULL;
  node->seg = NULL;
  node->thickseg = NULL;
  node->ffloor = NULL;
  node->sprite = NULL;
  return node;
}



static void R_DoneWithNode(drawnode_t* node)
{
  (node->next->prev = node->prev)->next = node->next;
  (node->next = nodebankhead.next)->prev = node;
  (node->prev = &nodebankhead)->next = node;
}



static void R_ClearDrawNodes()
{
  drawnode_t* rover;
  drawnode_t* next;

  for(rover = nodehead.next; rover != &nodehead; )
  {
    next = rover->next;
    R_DoneWithNode(rover);
    rover = next;
  }

  nodehead.next = nodehead.prev = &nodehead;
}



void R_InitDrawNodes(void)
{
  nodebankhead.next = nodebankhead.prev = &nodebankhead;
  nodehead.next = nodehead.prev = &nodehead;
}



//
// R_DrawSprite
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
void R_DrawSprite(vissprite_t* spr)
{
    drawseg_t*          ds;
    short               clipbot[MAXVIDWIDTH];
    short               cliptop[MAXVIDWIDTH];
    int                 x;
    int                 r1;
    int                 r2;
    fixed_t             scale;
    fixed_t             lowscale;
    int                 silhouette;

    for (x = spr->x1 ; x<=spr->x2 ; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    //SoM: 4/8/2000:
    // Pointer check was originally nonportable
    // and buggy, by going past LEFT end of array:

    //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
    for(ds=ds_p; ds-- > drawsegs; )
    {

        // determine if the drawseg obscures the sprite
        if(ds->x1 > spr->x2
         || ds->x2 < spr->x1
         || (!ds->silhouette
             && !ds->maskedtexturecol))
        {
            // does not cover sprite
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if(ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if(scale < spr->scale
            || (lowscale < spr->scale
                 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
        {
            // masked mid texture?
            /*if(ds->maskedtexturecol)
                R_RenderMaskedSegRange (ds, r1, r2);*/
            // seg is behind sprite
            continue;
        }

        // clip this piece of the sprite
        silhouette = ds->silhouette;

        if(spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if(spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;

        if(silhouette == 1)
        {
            // bottom sil
            for(x = r1; x <= r2; x++)
                if(clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if(silhouette == 2)
        {
            // top sil
            for(x = r1; x <= r2; x++)
                if(cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if(silhouette == 3)
        {
            // both
            for(x = r1; x <= r2; x++)
            {
                if(clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if(cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }
    }
    //SoM: 3/17/2000: Clip sprites in water.
    if(spr->heightsec != -1)  // only things in specially marked sectors
    {
        fixed_t h,mh;
        int phs = viewplayer->mo->subsector->sector->heightsec;
        if((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
           (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 &&
           (h >>= FRACBITS) < viewheight)
        {
            if(mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
            {                          // clip bottom
              for(x=spr->x1 ; x<=spr->x2 ; x++)
                if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = (short)h;
            }
            else                        // clip top
            {
              for(x=spr->x1 ; x<=spr->x2 ; x++)
                if(cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = (short)h;
            }
        }

        if((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
           (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 &&
           (h >>= FRACBITS) < viewheight)
        {
            if(phs != -1 && viewz >= sectors[phs].ceilingheight)
            {                         // clip bottom
              for(x=spr->x1 ; x<=spr->x2 ; x++)
                if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = (short)h;
            }
            else                       // clip top
            {
              for(x=spr->x1 ; x<=spr->x2 ; x++)
                if(cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = (short)h;
            }
        }
    }
    if(spr->cut & SC_TOP && spr->cut & SC_BOTTOM)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->szt;
        if(cliptop[x] == -2 || h > cliptop[x])
          cliptop[x] = (short)h;

        h = spr->sz;
        if(clipbot[x] == -2 || h < clipbot[x])
          clipbot[x] = (short)h;
      }
    }
    else if(spr->cut & SC_TOP)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->szt;
        if(cliptop[x] == -2 || h > cliptop[x])
          cliptop[x] = (short)h;
      }
    }
    else if(spr->cut & SC_BOTTOM)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->sz;
        if(clipbot[x] == -2 || h < clipbot[x])
          clipbot[x] = (short)h;
      }
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for(x = spr->x1 ; x<=spr->x2 ; x++)
    {
        if(clipbot[x] == -2)
            clipbot[x] = (short)viewheight;

        if(cliptop[x] == -2)
            //Fab:26-04-98: was -1, now clips against console bottom
            cliptop[x] = (short)con_clipviewtop;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite(spr, spr->x1, spr->x2);
}

// Special drawer for precipitation sprites Tails 08-18-2002
static void R_DrawPrecipitationSprite(vissprite_t* spr)
{
    drawseg_t*          ds;
    short               clipbot[MAXVIDWIDTH];
    short               cliptop[MAXVIDWIDTH];
    int                 x;
    int                 r1;
    int                 r2;
    fixed_t             scale;
    fixed_t             lowscale;
    int                 silhouette;

    for(x = spr->x1; x<=spr->x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    //SoM: 4/8/2000:
    // Pointer check was originally nonportable
    // and buggy, by going past LEFT end of array:

    //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
    for(ds=ds_p; ds-- > drawsegs; )
    {

        // determine if the drawseg obscures the sprite
        if(ds->x1 > spr->x2
         || ds->x2 < spr->x1
         || (!ds->silhouette
             && !ds->maskedtexturecol))
        {
            // does not cover sprite
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if(ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if(scale < spr->scale
            || (lowscale < spr->scale
                 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
        {
            // masked mid texture?
            /*if(ds->maskedtexturecol)
                R_RenderMaskedSegRange(ds, r1, r2);*/
            // seg is behind sprite
            continue;
        }

        // clip this piece of the sprite
        silhouette = ds->silhouette;

        if(silhouette == 1)
        {
            // bottom sil
            for(x = r1; x <= r2; x++)
                if(clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if(silhouette == 2)
        {
            // top sil
            for(x = r1; x <= r2; x++)
                if(cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if(silhouette == 3)
        {
            // both
            for(x = r1; x <= r2; x++)
            {
                if(clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if(cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1; x <= spr->x2; x++)
    {
        if(clipbot[x] == -2)
            clipbot[x] = (short)viewheight;

        if(cliptop[x] == -2)
            //Fab:26-04-98: was -1, now clips against console bottom
            cliptop[x] = (short)con_clipviewtop;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawPrecipitationVisSprite(spr, spr->x1, spr->x2);
}


//
// R_DrawMasked
//
void R_DrawMasked(void)
{
    drawnode_t* r2;
    drawnode_t* next;

    R_CreateDrawNodes();

    for(r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
    {
      if(r2->plane)
      {
        next = r2->prev;
        R_DrawSinglePlane(r2->plane, true);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->seg && r2->seg->maskedtexturecol != NULL)
      {
        next = r2->prev;
        R_RenderMaskedSegRange(r2->seg, r2->seg->x1, r2->seg->x2);
        r2->seg->maskedtexturecol = NULL;
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->thickseg)
      {
        next = r2->prev;
        R_RenderThickSideRange(r2->thickseg, r2->thickseg->x1, r2->thickseg->x2, r2->ffloor);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->sprite)
      {
        next = r2->prev;

		// Tails 08-18-2002
		if(r2->sprite->precip == true)
			R_DrawPrecipitationSprite(r2->sprite);
		else
			R_DrawSprite(r2->sprite);

        R_DoneWithNode(r2);
        r2 = next;
      }
    }
    R_ClearDrawNodes();
}

// ==========================================================================
//
//                              SKINS CODE
//
// ==========================================================================

int numskins = 0;
skin_t skins[MAXSKINS+1];
// FIXTHIS: don't work because it must be inistilised before the config load
//#define SKINVALUES
#ifdef SKINVALUES
CV_PossibleValue_t skin_cons_t[MAXSKINS+1];
#endif

static void Sk_SetDefaultValue(skin_t* skin)
{
	int i;
	//
	// setup Sonic as default skin
	//
	memset(skin, 0, sizeof(skin_t));
	strcpy(skin->name, DEFAULTSKIN);
	strcpy(skin->faceprefix, "SBOSLIFE");
	strcpy(skin->nameprefix, "STSONIC");
	strcpy(skin->spin, "1");
	strcpy(skin->starttranscolor, "112");
	strcpy(skin->endtranscolor, "127");
	strcpy(skin->prefcolor, "7");
	strcpy(skin->normalspeed, "36");
	strcpy(skin->thrustfactor, "5");
	strcpy(skin->accelstart, "64");
	strcpy(skin->acceleration, "40");
	strcpy(skin->jumpfactor, "100");
	strcpy(skin->boxindex, "1");
	strcpy(skin->ability, "0");
	strcpy(skin->highres, "0");

	for(i = 0; i < sfx_freeslot0; i++)
		if(S_sfx[i].skinsound != -1)
			skin->soundsid[S_sfx[i].skinsound] = i;
	memcpy(&skins[0].spritedef, &sprites[SPR_PLAY], sizeof(spritedef_t));
}

//
// Initialize the basic skins
//
void R_InitSkins(void)
{
#ifdef SKINVALUES
	int i;

	for(i = 0; i <= MAXSKINS; i++)
	{
		skin_cons_t[i].value = 0;
		skin_cons_t[i].strvalue = NULL;
	}
#endif

	// initialize free sfx slots for skin sounds
	S_InitRuntimeSounds();

	// skin[0] = Sonic skin
	Sk_SetDefaultValue(&skins[0]);
#ifdef SKINVALUES
	skin_cons_t[0].strvalue = skins[0].name;
#endif

	// make Sonic the default skin
	numskins = 1;

	// add face/facename graphics (special case: 1 to MAXSKINS-1 handled in R_AddSkins)
	ST_LoadFaceGraphics(skins[0].faceprefix, 0);
	ST_LoadFaceNameGraphics(skins[0].nameprefix, 0);
}

static void R_DoSkinTranslationInit(void)
{
	int i;

	for(i = 0; i <= numskins; i++)
		R_InitSkinTranslationTables(atoi(skins[i].starttranscolor), atoi(skins[i].endtranscolor), i);
}

// returns true if the skin name is found (loaded from pwad)
// warning return 0 (the default skin) if not found
int R_SkinAvailable(const char* name)
{
	int i;

	for(i = 0; i < numskins; i++)
	{
		if(stricmp(skins[i].name,name)==0)
			return i;
	}
	return 0;
}

// network code calls this when a 'skin change' is received
void SetPlayerSkin(int playernum, const char* skinname)
{
	int i;

	for(i = 0; i < numskins; i++)
	{
		// search in the skin list
		if(stricmp(skins[i].name, skinname) == 0)
		{
			// change the face graphics
			players[playernum].skin = i;
			if(players[playernum].mo)
			{
				players[playernum].mo->skin = &skins[i];
				if(atoi(skins[i].highres))
					players[playernum].mo->flags |= MF_HIRES;
				else
					players[playernum].mo->flags &= ~MF_HIRES;
			}

			players[playernum].charability = atoi(skins[i].ability);
			players[playernum].charspin = atoi(skins[i].spin);

			players[playernum].normalspeed = atoi(skins[i].normalspeed);
			players[playernum].thrustfactor = atoi(skins[i].thrustfactor);
			players[playernum].accelstart = atoi(skins[i].accelstart);
			players[playernum].acceleration = atoi(skins[i].acceleration);

			// Cheat checks!
			if(players[playernum].normalspeed > 36)
				players[playernum].normalspeed = 36;
			if(players[playernum].thrustfactor > 5)
				players[playernum].thrustfactor = 5;
			if(players[playernum].accelstart > 192)
				players[playernum].accelstart = 192;
			if(players[playernum].acceleration > 50)
				players[playernum].acceleration = 50;

			players[playernum].jumpfactor = atoi(skins[i].jumpfactor);

			if(players[playernum].jumpfactor > 100)
				players[playernum].jumpfactor = 100;

			players[playernum].boxindex = atoi(skins[i].boxindex);

			// Set the proper translation tables
			players[playernum].starttranscolor = atoi(skins[i].starttranscolor);
			players[playernum].endtranscolor = atoi(skins[i].endtranscolor);

			players[playernum].prefcolor = atoi(skins[i].prefcolor);

			return;
		}
	}

	CONS_Printf("Skin %s not found\n", skinname);
	players[playernum].skin = 0;

	// a copy of the skin value
	// so that dead body detached from respawning player keeps the skin
	if(players[playernum].mo)
		players[playernum].mo->skin = &skins[0];
}

// Same as SetPlayerSkin, but uses the skin #.
// network code calls this when a 'skin change' is received
void SetPlayerSkinByNum(int playernum, int skinnum)
{
	if(skinnum < numskins) // Make sure it exists!
	{
		players[playernum].skin = skinnum;
		if(players[playernum].mo)
		{
			players[playernum].mo->skin = &skins[skinnum];
			if(atoi(skins[skinnum].highres))
				players[playernum].mo->flags |= MF_HIRES;
			else
				players[playernum].mo->flags &= ~MF_HIRES;
		}

		players[playernum].charability = atoi(skins[skinnum].ability);
		players[playernum].charspin = atoi(skins[skinnum].spin);

		players[playernum].normalspeed = atoi(skins[skinnum].normalspeed);
		players[playernum].thrustfactor = atoi(skins[skinnum].thrustfactor);
		players[playernum].accelstart = atoi(skins[skinnum].accelstart);
		players[playernum].acceleration = atoi(skins[skinnum].acceleration);

		// Cheat checks!
		if(players[playernum].normalspeed > 36)
			players[playernum].normalspeed = 36;
		if(players[playernum].thrustfactor > 5)
			players[playernum].thrustfactor = 5;
		if(players[playernum].accelstart > 192)
			players[playernum].accelstart = 192;
		if(players[playernum].acceleration > 50)
			players[playernum].acceleration = 50;

		players[playernum].jumpfactor = atoi(skins[skinnum].jumpfactor);

		if(players[playernum].jumpfactor > 100)
			players[playernum].jumpfactor = 100;

		players[playernum].boxindex = atoi(skins[skinnum].boxindex);

		// Set the proper translation tables
		players[playernum].starttranscolor = atoi(skins[skinnum].starttranscolor);
		players[playernum].endtranscolor = atoi(skins[skinnum].endtranscolor);

		players[playernum].prefcolor = atoi(skins[skinnum].prefcolor);

		return;
	}

	CONS_Printf("Skin %d not found\n", skinnum);
	players[playernum].skin = 0;  // not found put the old marine skin

	// a copy of the skin value
	// so that dead body detached from respawning player keeps the skin
	if(players[playernum].mo)
		players[playernum].mo->skin = &skins[0];
}

static void SetSkinValues(consvar_t* var, char* valstr)
{
	if(var->PossibleValue)
	{
		int v =  atoi(valstr);

		if(!stricmp(var->PossibleValue[0].strvalue, "MIN"))
		{   // bounded cvar
			int i;
			// search for maximum
			for(i = 1; var->PossibleValue[i].strvalue != NULL; i++)
				if(!stricmp(var->PossibleValue[i].strvalue, "MAX"))
					break;

			if(v < var->PossibleValue[0].value)
			{
				v = var->PossibleValue[0].value;
				sprintf(valstr, "%d", v);
			}
			if(v > var->PossibleValue[i].value)
			{
				v = var->PossibleValue[i].value;
				sprintf(valstr, "%d", v);
			}
		}
		else
		{
			// waw spaghetti programming ! :)
			int i;

			// check first strings
			for(i = 0; var->PossibleValue[i].strvalue != NULL; i++)
				if(!stricmp(var->PossibleValue[i].strvalue, valstr))
					goto found;
			if(!v)
				if(strcmp(valstr, "0"))
					goto error;
			// check int now
			for(i = 0; var->PossibleValue[i].strvalue != NULL; i++)
				if(v == var->PossibleValue[i].value)
					goto found;

error:
			// not found
			CONS_Printf("\"%s\" is not a possible value for \"%s\"\n", valstr, var->name);
			if(var->defaultvalue == valstr)
				I_Error("Variable %s default value \"%s\" is not a possible value\n",
					var->name, var->defaultvalue);
			return;
found:
			var->value = var->PossibleValue[i].value;
			var->string = var->PossibleValue[i].strvalue;
			goto finish;
		}
	}

	// free the old value string
	if(var->zstring)
		Z_Free(var->zstring);

	var->string = var->zstring = Z_StrDup(valstr);

	var->value = atoi(var->string);

finish:
	var->flags |= CV_MODIFIED;
}

// For loading from saved games
void SetSavedSkin(int playernum, int skinnum, int skincolor)
{
	char val[32];

	players[playernum].skincolor = skincolor % MAXSKINCOLORS;
	sprintf(val, "%d", players[playernum].skincolor);

	SetSkinValues(&cv_skin, skins[skinnum].name);
	SetSkinValues(&cv_playercolor, val);

	if(players[playernum].mo)
		players[playernum].mo->flags = (players[playernum].mo->flags & ~MF_TRANSLATION)
			| ((players[playernum].skincolor)<<MF_TRANSSHIFT);

	SetPlayerSkinByNum(playernum, skinnum);
}

char* GetPlayerFacePic(int skinnum)
{
	return skins[skinnum].faceprefix;
}

//
// Add skins from a pwad, each skin preceded by 'S_SKIN' marker
//

// Does the same is in w_wad, but check only for
// the first 6 characters (this is so we can have S_SKIN1, S_SKIN2..
// for wad editors that don't like multiple resources of the same name)
//
static int W_CheckForSkinMarkerInPwad(int wadid, int startlump)
{
	int i, v1;
	lumpinfo_t* lump_p;

	union
	{
		char s[4];
		int x;
	} name4;

	strncpy(name4.s, "S_SK", 4);
	v1 = name4.x;

	// scan forward, start at <startlump>
	if(startlump < wadfiles[wadid]->numlumps)
	{
		lump_p = wadfiles[wadid]->lumpinfo + startlump;
		for(i = startlump; i < wadfiles[wadid]->numlumps; i++, lump_p++)
			if(*(int*)lump_p->name == v1 && lump_p->name[4] == 'I' && lump_p->name[5] == 'N')
				return ((wadid<<16)+i);
	}
	return -1; // not found
}

//
// Find skin sprites, sounds & optional status bar face, & add them
//
void R_AddSkins(int wadnum)
{
	int lumpnum;
	int lastlump = 0;

	lumpinfo_t* lumpinfo;
	char* sprname = NULL;
	int intname;

	char* buf;
	char* buf2;
	char* token;
	char* value;
	int i, size;
	boolean boxindexset;

	//
	// search for all skin markers in pwad
	//

	while((lumpnum = W_CheckForSkinMarkerInPwad(wadnum, lastlump)) != -1)
	{
		if(numskins > MAXSKINS)
		{
			CONS_Printf("ignored skin (%d skins maximum)\n", MAXSKINS);
			lastlump++;
			continue; // so we know how many skins couldn't be added
		}

		buf = W_CacheLumpNum(lumpnum, PU_CACHE);
		size = W_LumpLength(lumpnum);

		// for strtok
		buf2 = (char *)malloc(size+1);
		if(!buf2)
			I_Error("R_AddSkins: No more free memory\n");
		memcpy(buf2,buf,size);
		buf2[size] = '\0';

		// set defaults
		Sk_SetDefaultValue(&skins[numskins]);
		sprintf(skins[numskins].name, "skin %d",numskins);

		boxindexset = false;

		// parse
		token = strtok (buf2, "\r\n= ");
		while(token)
		{
			if(token[0] == '/' && token[1] == '/') // skip comments
			{
				token = strtok(NULL, "\r\n"); // skip end of line
				goto next_token;              // find the real next token
			}

			value = strtok(NULL, "\r\n= ");

			if(!value)
				I_Error("R_AddSkins: syntax error in S_SKIN lump# %d in WAD %s\n", lumpnum&0xFFFF, wadfiles[wadnum]->filename);

			if(!stricmp(token, "name"))
			{
				// the skin name must uniquely identify a single skin
				// I'm lazy so if name is already used I leave the 'skin x'
				// default skin name set above
				if(!R_SkinAvailable(value))
				{
					strncpy(skins[numskins].name, value, SKINNAMESIZE);
					skins[numskins].name[SKINNAMESIZE] = '\0';
					strlwr(skins[numskins].name);
				}
			}
			else if(!stricmp(token, "face"))
			{
				strncpy(skins[numskins].faceprefix, value, 8);
				skins[numskins].faceprefix[8] = 0;
				strupr(skins[numskins].faceprefix);
			}
			else if(!stricmp(token, "facename")) // Life icon name
			{
				strncpy(skins[numskins].nameprefix, value, 8); // Life icon name
				skins[numskins].nameprefix[8] = 0;
				strupr(skins[numskins].nameprefix);
			}
			// character type identification
			else if(!stricmp(token, "ability"))
			{
				strncpy(skins[numskins].ability, value, 1);
				strupr(skins[numskins].ability);
			}
			else if(!stricmp(token, "normalspeed"))
			{
				strncpy(skins[numskins].normalspeed, value, 2);
				strupr(skins[numskins].normalspeed);
			}
			else if(!stricmp(token, "thrustfactor"))
			{
				strncpy(skins[numskins].thrustfactor, value, 1);
				strupr(skins[numskins].thrustfactor);
			}
			else if(!stricmp(token, "accelstart"))
			{
				strncpy(skins[numskins].accelstart, value, 3);
				strupr(skins[numskins].accelstart);
			}
			else if(!stricmp(token, "acceleration"))
			{
				strncpy(skins[numskins].acceleration, value, 2);
				strupr(skins[numskins].acceleration);
			}
			else if(!stricmp(token, "spin"))
			{
				strncpy(skins[numskins].spin, value, 1);
				strupr(skins[numskins].spin);
			}

			// custom translation table
			else if(!stricmp(token, "startcolor"))
			{
				byte colorval;

				strncpy(skins[numskins].starttranscolor, value, 3);
				strupr(skins[numskins].starttranscolor);

				// Generate the ENDCOLOR based on the startcolor (startcolor+16)
				colorval = (byte)atoi(skins[numskins].starttranscolor);
				colorval += 16;

				sprintf(skins[numskins].endtranscolor, "%i", colorval);
			}
			else if(!strcmp(token, "endcolor")) // This is deprecated, but kept for backwards-compatibility.
			{
//				strncpy(skins[numskins].endtranscolor, value, 3);
//				strupr(skins[numskins].endtranscolor);
			}

			else if(!strcmp(token, "prefcolor"))
			{
				strncpy(skins[numskins].prefcolor, value, 2);
				strupr(skins[numskins].prefcolor);
			}
			else if(!strcmp(token, "jumpheight"))
			{
				strncpy(skins[numskins].jumpfactor, value, 3);
				strupr(skins[numskins].jumpfactor);
			}
			else if(!strcmp(token, "boxindex"))
			{
				strncpy(skins[numskins].boxindex, value, 3);
				strupr(skins[numskins].boxindex);

				boxindexset = true;
			}
			else if(!strcmp(token, "highres"))
			{
				strncpy(skins[numskins].highres, value, 1);
				strupr(skins[numskins].highres);
			}
			else if(!stricmp(token, "sprite"))
			{
				sprname = value;
				strupr(sprname);
			}
			else
			{
				int found = false;
				// copy name of sounds that are remapped for this skin
				for(i = 0; i < sfx_freeslot0; i++)
				{
					if(!S_sfx[i].name)
						continue;
					if(S_sfx[i].skinsound!=-1 &&
						!stricmp(S_sfx[i].name, token+2))
					{
						skins[numskins].soundsid[S_sfx[i].skinsound] =
							S_AddSoundFx(value+2,S_sfx[i].singularity,S_sfx[i].pitch);
						found = true;
					}
				}
				if(!found)
					I_Error("R_AddSkins: Unknown keyword '%s' in S_SKIN lump# %d (WAD %s)\n",token,lumpnum&0xFFFF,wadfiles[wadnum]->filename);
			}
next_token:
			token = strtok(NULL, "\r\n= ");
		}

		if(boxindexset == false && numskins>0)
			strcpy(skins[numskins].boxindex, "0");

		// if no sprite defined use spirte just after this one
		if(!sprname)
		{
			lumpnum &= 0xFFFF; // get rid of wad number
			lumpnum++;
			lumpinfo = wadfiles[wadnum]->lumpinfo;

			// get the base name of this skin's sprite (4 chars)
			sprname = lumpinfo[lumpnum].name;
			intname = *(int*)sprname;

			// skip to end of this skin's frames
			lastlump = lumpnum;
			while (*(int*)lumpinfo[lastlump].name == intname)
				lastlump++;
			// allocate (or replace) sprite frames, and set spritedef
			R_AddSingleSpriteDef(sprname, &skins[numskins].spritedef, wadnum, lumpnum, lastlump);
		}
		else
		{
			// search in the normal sprite tables
			const char** name;
			boolean found = false;
			for(name = sprnames;*name;name++)
				if(strcmp(*name, sprname) == 0)
				{
					found = true;
					skins[numskins].spritedef = sprites[sprnames-name];
				}

			// not found so make a new one
			if(!found)
				R_AddSingleSpriteDef(sprname, &skins[numskins].spritedef, wadnum, 0, MAXINT);
		}

		R_DoSkinTranslationInit();

		CONS_Printf("added skin '%s'\n", skins[numskins].name);
#ifdef SKINVALUES
		skin_cons_t[numskins].value=numskins;
		skin_cons_t[numskins].strvalue=skins[numskins].name;
#endif

		// add face/facename graphics
		ST_LoadFaceGraphics(skins[numskins].faceprefix, numskins);
		ST_LoadFaceNameGraphics(skins[numskins].nameprefix, numskins);

		numskins++;
		free(buf2);
	}
	return;
}
