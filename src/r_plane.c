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
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------
/// \file
/// \brief Here is a core component: drawing the floors and ceilings,
///	while maintaining a per column clipping list only.
///	Moreover, the sky areas have to be determined.

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "r_data.h"
#include "r_local.h"
#include "r_state.h"
#include "r_splats.h" // faB(21jan):testing
#include "r_sky.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_setup.h" // levelflats

//
// opening
//

//SoM: 3/23/2000: Use Boom visplane hashing.
#define MAXVISPLANES 512

static visplane_t *visplanes[MAXVISPLANES];
static visplane_t *freetail;
static visplane_t **freehead = &freetail;

visplane_t* floorplane;
visplane_t* ceilingplane;
static visplane_t* currentplane;

planemgr_t ffloor[MAXFFLOORS];
int numffloors;

//SoM: 3/23/2000: Boom visplane hashing routine.
#define visplane_hash(picnum,lightlevel,height) \
  ((unsigned)((picnum)*3+(lightlevel)+(height)*7) & (MAXVISPLANES-1))

//SoM: 3/23/2000: Use boom opening limit removal
size_t maxopenings;
short *openings, *lastopening;

//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
short floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
fixed_t frontscale[MAXVIDWIDTH];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
static int spanstart[MAXVIDHEIGHT];

//
// texture mapping
//
static lighttable_t** planezlight;
static fixed_t planeheight;

//added:10-02-98: yslopetab is what yslope used to be,
//                yslope points somewhere into yslopetab,
//                now (viewheight/2) slopes are calculated above and
//                below the original viewheight for mouselook
//                (this is to calculate yslopes only when really needed)
//                (when mouselookin', yslope is moving into yslopetab)
//                Check R_SetupFrame, R_SetViewSize for more...
fixed_t                 yslopetab[MAXVIDHEIGHT*4];
fixed_t*                yslope;

fixed_t                 distscale[MAXVIDWIDTH];
fixed_t                 basexscale;
fixed_t                 baseyscale;

fixed_t                 cachedheight[MAXVIDHEIGHT];
fixed_t                 cacheddistance[MAXVIDHEIGHT];
fixed_t                 cachedxstep[MAXVIDHEIGHT];
fixed_t                 cachedystep[MAXVIDHEIGHT];

static fixed_t   xoffs, yoffs;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
  // Doh!
}


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
         long long mycount;
         long long mytotal = 0;
         unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//  xoffs
//  yoffs
//
// BASIC PRIMITIVE
//

void R_MapPlane
( int           y,              // t1
  int           x1,
  int           x2 )
{
    angle_t     angle;
    fixed_t     distance;
    fixed_t     length;
    unsigned    index;

#ifdef RANGECHECK
    if(x2 < x1
        || x1<0
        || x2>=viewwidth
        || (unsigned)y>viewheight)
    {
        I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
    }
#endif

//    Alam: from r_splats's R_RenderFloorSplat
    //if(x1<0) x1 = 0;
    if(x1>=vid.width) x1 = vid.width-1;

    if(planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
        ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
        ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
    }
    else
    {
        distance = cacheddistance[y];
        ds_xstep = cachedxstep[y];
        ds_ystep = cachedystep[y];
    }

    length = FixedMul (distance,distscale[x1]);
    angle = (currentplane->viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
    // Wouldn't it be faster just to add viewx and viewy to the plane's x/yoffs anyway??

    ds_xfrac = FixedMul(finecosine[angle], length) + xoffs;
    ds_yfrac = yoffs - FixedMul(finesine[angle], length);

	index = distance >> LIGHTZSHIFT;

	if(index >= MAXLIGHTZ)
		index = MAXLIGHTZ - 1;

	ds_colormap = planezlight[index];

	if(currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap + (ds_colormap - colormaps);

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	// profile drawer
#ifdef TIMING
	ProfZeroTimer();
#endif

	spanfunc();

#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add
	if(!(nombre--))
	I_Error("spanfunc() CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1), (int)mytotal);
#endif
}

//
// R_ClearPlanes
// At begining of frame.
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of the view hidden under the console
void R_ClearPlanes (player_t *player)
{
    int         i, p;
    angle_t     angle;

    // opening / clipping determination
	 player = NULL;
    for (i=0 ; i<viewwidth ; i++)
    {
        floorclip[i] = (short)viewheight;
        ceilingclip[i] = (short)con_clipviewtop;       //Fab:26-04-98: was -1
        frontscale[i] = MAXINT;
        for(p = 0; p < MAXFFLOORS; p++)
        {
          ffloor[p].f_clip[i] = (short)viewheight;
          ffloor[p].c_clip[i] = (short)con_clipviewtop;
        }
    }

    numffloors = 0;

	//SoM: 3/23/2000
	for(i = 0; i < MAXVISPLANES; i++)
		for(*freehead = visplanes[i], visplanes[i] = NULL; *freehead; )
			freehead = &(*freehead)->next;

    lastopening = openings;

    // texture calculation
    memset (cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;

    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = FixedDiv (finecosine[angle],centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],centerxfrac);
}


//SoM: 3/23/2000: New function, by Lee Killough
static visplane_t *new_visplane(unsigned hash)
{
  visplane_t *check = freetail;
  if(!check)
    check = calloc(1, sizeof *check);
  else
  {
    freetail = freetail->next;
    if(!freetail)
      freehead = &freetail;
  }
  check->next = visplanes[hash];
  visplanes[hash] = check;
  return check;
}



//
// R_FindPlane : cherche un visplane ayant les valeurs identiques:
//               meme hauteur, meme flattexture, meme lightlevel.
//               Sinon en alloue un autre.
//
visplane_t* R_FindPlane( fixed_t height,
                         fixed_t picnum,
                         int     lightlevel,
                         fixed_t xoff,
                         fixed_t yoff,
                         extracolormap_t* planecolormap,
                         ffloor_t* ffloor)
{
    visplane_t* check;
    unsigned    hash; //SoM: 3/23/2000

    xoff += viewx; // SoM
    yoff = -viewy + yoff;

    if(picnum == skyflatnum)
    {
        height = 0;                     // all skys map together
        lightlevel = 0;
    }


    //SoM: 3/23/2000: New visplane algorithm uses hash table -- killough
    hash = visplane_hash(picnum,lightlevel,height);

    for (check=visplanes[hash]; check; check=check->next)
      if(height == check->height &&
          picnum == check->picnum &&
          lightlevel == check->lightlevel &&
          xoff == check->xoffs &&
          yoff == check->yoffs &&
          planecolormap == check->extra_colormap &&
          !ffloor && !check->ffloor &&
          check->viewz == viewz &&
          check->viewangle == viewangle)
        return check;

    check = new_visplane(hash);

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = vid.width;
    check->maxx = -1;
    check->xoffs = xoff;
    check->yoffs = yoff;
    check->extra_colormap = planecolormap;
    check->ffloor = ffloor;
    check->viewz = viewz;
    check->viewangle = viewangle;

    memset (check->top, 0xff, sizeof(check->top));
    memset (check->bottom, 0x00, sizeof(check->bottom));

    return check;
}




//
// R_CheckPlane : return same visplane or alloc a new one if needed
//
visplane_t*  R_CheckPlane( visplane_t*   pl,
                           int           start,
                           int           stop )
{
    int         intrl;
    int         intrh;
    int         unionl;
    int         unionh;
    int         x;

    if(start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if(stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    //added 30-12-97 : 0xff ne vaut plus -1 avec un short...
    for (x=intrl ; x<= intrh ; x++)
        if(pl->top[x] != 0xffff || pl->bottom[x] != 0x0000)
            break;

    //SoM: 3/23/2000: Boom code
    if(x > intrh)
      pl->minx = unionl, pl->maxx = unionh;
    else
      {
        unsigned hash = visplane_hash(pl->picnum, pl->lightlevel, pl->height);
        visplane_t *new_pl = new_visplane(hash);

        new_pl->height = pl->height;
        new_pl->picnum = pl->picnum;
        new_pl->lightlevel = pl->lightlevel;
        new_pl->xoffs = pl->xoffs;           // killough 2/28/98
        new_pl->yoffs = pl->yoffs;
        new_pl->extra_colormap = pl->extra_colormap;
        new_pl->ffloor = pl->ffloor;
        new_pl->viewz = pl->viewz;
        new_pl->viewangle = pl->viewangle;
        pl = new_pl;
        pl->minx = start;
        pl->maxx = stop;
        memset(pl->top, 0xff, sizeof pl->top);
        memset(pl->bottom, 0x00, sizeof pl->bottom);
      }
    return pl;
}


//
// R_ExpandPlane
//
// SoM: This function basically expands the visplane or I_Errors
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
void R_ExpandPlane(visplane_t*  pl, int start, int stop)
{
    int         intrl;
    int         intrh;
    int         unionl;
    int         unionh;
	int			x;

    if(start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if(stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    for (x = start ; x <= stop ; x++)
        if(pl->top[x] != 0xffff || pl->bottom[x] != 0x0000)
            break;

    //SoM: 3/23/2000: Boom code
    if(x > stop)
      pl->minx = unionl, pl->maxx = unionh;
//    else
//      I_Error("R_ExpandPlane: planes in same subsector overlap?!\nminx: %i, maxx: %i, start: %i, stop: %i\n", pl->minx, pl->maxx, start, stop);

    pl->minx = unionl, pl->maxx = unionh;
}

//
// R_MakeSpans
//
void R_MakeSpans
( int           x,
  int           t1,
  int           b1,
  int           t2,
  int           b2 )
{
	//    Alam: from r_splats's R_RenderFloorSplat
    //if(t1<0) t1=0;
    //if(b1<0) b1=0;
    //if(t2<0) t2=0;
    //if(b2<0) b2=0;
    if(t1>=vid.height) t1 = vid.height-1;
    if(b1>=vid.height) b1 = vid.height-1;
    if(t2>=vid.height) t2 = vid.height-1;
    if(b2>=vid.height) b2 = vid.height-1;
    //if(x<1) x = 1;
    if(x-1>=vid.width) x = vid.width;

    while (t1 < t2 && t1<=b1)
    {
        R_MapPlane (t1,spanstart[t1],x-1);
        t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
        R_MapPlane (b1,spanstart[b1],x-1);
        b1--;
    }

    while (t2 < t1 && t2<=b2)
    {
        spanstart[t2] = x;
        t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
        spanstart[b2] = x;
        b2--;
    }
}

void R_DrawPlanes (void)
{
    visplane_t*         pl;
    int                 x;
    int                 angle;
    int                 i; //SoM: 3/23/2000

    spanfunc = basespanfunc;

    for (i=0;i<MAXVISPLANES;i++, pl++)
    for (pl=visplanes[i]; pl; pl=pl->next)
    {
        // sky flat
        if(pl->picnum == skyflatnum)
        {
            //added:12-02-98: use correct aspect ratio scale
            //dc_iscale = FixedDiv (FRACUNIT, pspriteyscale);
            dc_iscale = skyscale;

// Kik test non-moving sky .. weird
// cy = centery;
// centery = (viewheight/2);

            // Sky is allways drawn full bright,
            //  i.e. colormaps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
            dc_colormap = colormaps;
            dc_texturemid = skytexturemid;
            dc_texheight = textureheight[skytexture] >> FRACBITS;
            for (x=pl->minx ; x <= pl->maxx ; x++)
            {
                dc_yl = pl->top[x];
				whereitsfrom = 4;
                dc_yh = pl->bottom[x];

                if(dc_yl <= dc_yh)
                {
                    angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
                    dc_x = x;
                    dc_source = R_GetColumn(skytexture, angle);
                    skycolfunc ();
                }
            }
// centery = cy;
            continue;
        }

        if(pl->ffloor)
          continue;

        R_DrawSinglePlane(pl, true);
    }
}

void R_DrawSinglePlane(visplane_t* pl, boolean handlesource)
{
  int                 light = 0;
  int                 x;
  int                 stop;
  int                 angle;

  if(pl->minx > pl->maxx)
    return;

  spanfunc = basespanfunc;
  if(pl->ffloor)
  {
    if(pl->ffloor->flags & FF_TRANSLUCENT)
    {
      spanfunc = R_DrawTranslucentSpan_8;

	  // Hacked up support for alpha value in software mode Tails 09-24-2002
	  if(pl->ffloor->alpha < 64)
		  ds_transmap = ((3)<<FF_TRANSSHIFT) - 0x10000 + transtables; // Don't be so hard to see Tails 02-02-2002
	  else if(pl->ffloor->alpha < 128)
		  ds_transmap = ((2)<<FF_TRANSSHIFT) - 0x10000 + transtables; // Don't be so hard to see Tails 02-02-2002
	  else if(pl->ffloor->alpha < 192) // Graue 02-21-2004
		  ds_transmap = ((1)<<FF_TRANSSHIFT) - 0x10000 + transtables; // Don't be so hard to see Tails 02-02-2002
	  else // Opaque like in R_RenderThickSideRange Graue 02-21-2004
	      spanfunc = basespanfunc;

      if(pl->extra_colormap && pl->extra_colormap->fog)
        light = (pl->lightlevel >> LIGHTSEGSHIFT);
      else
        light = LIGHTLEVELS-1;
    }
    else if(pl->ffloor->flags & FF_FOG)
    {
      spanfunc = R_DrawFogSpan_8;
      light = (pl->lightlevel >> LIGHTSEGSHIFT);
    }
    else
      light = (pl->lightlevel >> LIGHTSEGSHIFT);
  }
  else
      light = (pl->lightlevel >> LIGHTSEGSHIFT);

  if(viewangle != pl->viewangle)
  {
    memset (cachedheight, 0, sizeof(cachedheight));

    angle = (pl->viewangle-ANG90)>>ANGLETOFINESHIFT;

    basexscale = FixedDiv (finecosine[angle],centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],centerxfrac);
    viewangle = pl->viewangle;
  }

  currentplane = pl;

  if(handlesource)
  {
	int size;
    ds_source = (byte *)W_CacheLumpNum(levelflats[pl->picnum].lumpnum,PU_STATIC); //Alam: Stay here until Z_ChangeTag

	size = W_LumpLength(levelflats[pl->picnum].lumpnum);

	switch(size)
	{
		case 4194304: // 2048x2048 lump
			flatsize = 2048;
			flatmask = 2047<<11;
			flatsubtract = 11;
			break;
		case 1048576: // 1024x1024 lump
			flatsize = 1024;
			flatmask = 1023<<10;
			flatsubtract = 10;
			break;
		case 262144:// 512x512 lump
			flatsize = 512;
			flatmask = 511<<9;
			flatsubtract = 9;
			break;
		case 65536: // 256x256 lump
			flatsize = 256;
			flatmask = 255<<8;
			flatsubtract = 8;
			break;
		case 16384: // 128x128 lump
			flatsize = 128;
			flatmask = 127<<7;
			flatsubtract = 7;
			break;
		case 1024: // 32x32 lump
			flatsize = 32;
			flatmask = 31<<5;
			flatsubtract = 5;
			break;
		default: // 64x64 lump
			flatsize = 64;
			flatmask = 0x3f<<6;
			flatsubtract = 6;
			break;
	}
  }

  xoffs = pl->xoffs;
  yoffs = pl->yoffs;
  planeheight = abs(pl->height - pl->viewz);

  if(light >= LIGHTLEVELS)
      light = LIGHTLEVELS-1;

  if(light < 0)
      light = 0;

  planezlight = zlight[light];

  //set the MAXIMUM value for unsigned
  pl->top[pl->maxx+1] = 0xffff;
  pl->top[pl->minx-1] = 0xffff;
  pl->bottom[pl->maxx+1] = 0x0000;
  pl->bottom[pl->minx-1] = 0x0000;

  stop = pl->maxx + 1;

  for (x=pl->minx ; x<= stop ; x++)
  {
    R_MakeSpans(x,pl->top[x-1],
                pl->bottom[x-1],
                pl->top[x],
                pl->bottom[x]);
  }

  if(handlesource)
    Z_ChangeTag (ds_source, PU_CACHE);
}


void R_PlaneBounds(visplane_t* plane)
{
  int  i;
  int  hi, low;

  hi = plane->top[plane->minx];
  low = plane->bottom[plane->minx];

  for(i = plane->minx + 1; i <= plane->maxx; i++)
  {
    if(plane->top[i] < hi)
      hi = plane->top[i];
    if(plane->bottom[i] > low)
      low = plane->bottom[i];
  }
  plane->high = hi;
  plane->low = low;
}
