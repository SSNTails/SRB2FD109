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
/// \brief Rendering main loop and setup functions,
///	utility functions (BSP, geometry, trigonometry).
///	See tables.c, too.

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h" // faB(21jan): testing
#include "r_sky.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "p_local.h"
#include "am_map.h"
#include "d_main.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// increment every time a check is made
int validcount = 1;

int centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection;
fixed_t projectiony; // aspect ratio

// just for profiling purposes
int framecount;

int sscount, linecount, loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
player_t* viewplayer;

//
// precalculated math tables
//
angle_t clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[MAXVIDWIDTH+1];

// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.

fixed_t* finecosine = &finesine[FINEANGLES/4];

lighttable_t* scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t* scalelightfixed[MAXLIGHTSCALE];
lighttable_t* zlight[LIGHTLEVELS][MAXLIGHTZ];

// Hack to support extra boom colormaps.
int num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];

static CV_PossibleValue_t precipdensity_cons_t[] = {{1, "Thick"}, {2, "Heavy"}, {3, "Moderate"}, {4, "Light"}, {0, NULL}};
static CV_PossibleValue_t grtranslucenthud_cons_t[] = {{1, "MIN"}, {255, "MAX"}, {0, NULL}};
static CV_PossibleValue_t viewsize_cons_t[] = {{3, "MIN"}, {12, "MAX"}, {0, NULL}};

static void Homing_OnChange(void);
static void LightDash_OnChange(void);
static void TimeAttacked_OnChange(void);
static void SplitScreen_OnChange(void);

consvar_t cv_tailspickup = {"tailspickup", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam = {"chasecam", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasecam2 = {"chasecam2", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_homing = {"homing", "Off", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, CV_OnOff, Homing_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_lightdash = {"lightdash", "Off", CV_NETVAR|CV_NOSHOWHELP|CV_CALL, CV_OnOff, LightDash_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_shadow = {"shadow", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_numsnow = {"numsnow", "Moderate", CV_SAVE, precipdensity_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_raindensity = {"raindensity", "Heavy", CV_SAVE, precipdensity_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_storm = {"storm", "Off", CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_rain = {"rain", "Off", CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_snow = {"snow", "Off", CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_soniccd = {"soniccd", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_timeattacked = {"timeattacked", "Off", CV_NETVAR, CV_OnOff, TimeAttacked_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowmlook = {"allowmlook", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_precipdist = {"precipdist", "1024", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_viewsize = {"viewsize", "11", CV_SAVE|CV_CALL, viewsize_cons_t, R_SetViewSize, 0, NULL, NULL, 0, 0, NULL};      //3-12
consvar_t cv_grtranslucenthud = {"gr_translucenthud", "255", CV_SAVE|CV_CALL,
	grtranslucenthud_cons_t, R_SetViewSize, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_splitscreen = {"splitscreen", "Off", CV_CALL, CV_OnOff, SplitScreen_OnChange, 0, NULL, NULL, 0, 0, NULL};

static void Homing_OnChange(void)
{
	if(maptol & TOL_ADVENTURE)
		return;

	if(!(server || admin))
		return;

	if(cv_debug)
		return;

	if(cv_homing.value)
		CV_SetValue(&cv_homing, false);
}

static void LightDash_OnChange(void)
{
	if(maptol & TOL_ADVENTURE)
		return;

	if(!(server || admin))
		return;

	if(cv_debug)
		return;

	if(cv_lightdash.value)
		CV_SetValue(&cv_lightdash, false);
}

static void TimeAttacked_OnChange(void)
{
	if(!modifiedgame || savemoddata)
	{
		modifiedgame = true;
		savemoddata = false;
		if(!(netgame || multiplayer))
			CONS_Printf("WARNING: Game must be restarted to record statistics.\n");
	}
}

static void SplitScreen_OnChange(void)
{
	// recompute screen size
	R_ExecuteSetViewSize();

	// change the menu
	M_SwitchSplitscreen();

	if(!demoplayback)
	{
		if(cv_splitscreen.value)
			CL_AddSplitscreenPlayer();
		else
			CL_RemoveSplitscreenPlayer();

		if(server && !netgame)
			multiplayer = cv_splitscreen.value;
	}
	else
	{
		int i;
		secondarydisplayplayer = consoleplayer;
		for(i = 0; i < MAXPLAYERS; i++)
			if(playeringame[i] && i != consoleplayer)
			{
				secondarydisplayplayer = i;
				break;
			}
	}
}

//
// R_PointOnSide
// Traverse BSP (sub) tree, check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int R_PointOnSide(fixed_t x, fixed_t y, node_t* node)
{
	fixed_t dx, dy, left, right;

	if(!node->dx)
	{
		if(x <= node->x)
			return node->dy > 0;

		return node->dy < 0;
	}
	if(!node->dy)
	{
		if(y <= node->y)
			return node->dx < 0;

		return node->dx > 0;
	}

	dx = (x - node->x);
	dy = (y - node->y);

	// Try to quickly decide by looking at sign bits.
	if((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
	{
		if((node->dy ^ dx) & 0x80000000)
			return 1; // left is negative
		return 0;
	}

	left = FixedMul(node->dy>>FRACBITS, dx);
	right = FixedMul(dy, node->dx>>FRACBITS);

	if(right < left)
		return 0; // front side
	return 1; // back side
}

int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t* line)
{
	fixed_t lx, ly, ldx, ldy, dx, dy, left, right;

	lx = line->v1->x;
	ly = line->v1->y;

	ldx = line->v2->x - lx;
	ldy = line->v2->y - ly;

	if(!ldx)
	{
		if(x <= lx)
			return ldy > 0;

		return ldy < 0;
	}
	if(!ldy)
	{
		if(y <= ly)
			return ldx < 0;

		return ldx > 0;
	}

	dx = (x - lx);
	dy = (y - ly);

	// Try to quickly decide by looking at sign bits.
	if((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
	{
		if((ldy ^ dx) & 0x80000000)
			return 1; // left is negative
		return 0;
	}

	left = FixedMul(ldy>>FRACBITS, dx);
	right = FixedMul(dy, ldx>>FRACBITS);

	if(right < left)
		return 0; // front side
	return 1; // back side
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

angle_t R_PointToAngle2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1)
{
	x1 -= x2;
	y1 -= y2;

	if((!x1) && (!y1))
		return 0;

	if(x1 >= 0)
	{
		// x >=0
		if(y1 >= 0)
		{
			// y >= 0
			if(x1 > y1)
				return tantoangle[SlopeDiv(y1, x1)]; // octant 0
			else
				return ANG90 - 1 - tantoangle[SlopeDiv(x1, y1)]; // octant 1
		}
		else
		{
			// y < 0
			y1 = -y1;

			if(x1 > y1)
				return -(signed)tantoangle[SlopeDiv(y1,x1)]; // octant 8
			else
				return ANG270+tantoangle[ SlopeDiv(x1,y1)]; // octant 7
		}
	}
	else
	{
		// x < 0
		x1 = -x1;

		if(y1 >= 0)
		{
			// y >= 0
			if(x1 > y1)
				return ANG180 - 1 - tantoangle[SlopeDiv(y1, x1)]; // octant 3
			else
				return ANG90 + tantoangle[SlopeDiv(x1, y1)]; // octant 2
		}
		else
		{
			// y < 0
			y1 = -y1;

			if(x1 > y1)
				return ANG180+tantoangle[ SlopeDiv(y1,x1)]; // octant 4
			else
				return ANG270-1-tantoangle[ SlopeDiv(x1,y1)]; // octant 5
		}
	}
	//return 0;
}

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return R_PointToAngle2(viewx, viewy, x, y);
}

fixed_t R_PointToDist2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1)
{
	int angle;
	fixed_t dx, dy, dist;

	dx = abs(x1 - x2);
	dy = abs(y1 - y2);

	if(dy > dx)
	{
		fixed_t temp;

		temp = dx;
		dx = dy;
		dy = temp;
	}
	if(!dy)
		return dx;

	angle = (tantoangle[FixedDiv(dy, dx)>>DBITS] + ANG90) >> ANGLETOFINESHIFT;

	// use as cosine
	dist = FixedDiv(dx, finesine[angle]);

	return dist;
}

// Little extra utility. Works in the same way as R_PointToAngle2
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

/***************************************
*** Zdoom C++ to Legacy C conversion ***
****************************************/

// Utility to find the Z height at an XY location in a sector (for slopes)
fixed_t R_SecplaneZatPoint(secplane_t* secplane, fixed_t x, fixed_t y)
{
	return FixedMul(secplane->ic, -secplane->d - DMulScale16(secplane->a, x, secplane->b, y));
}

// Returns the value of z at (x,y) if d is equal to dist
fixed_t R_SecplaneZatPointDist (secplane_t* secplane, fixed_t x, fixed_t y, fixed_t dist)
{
	return FixedMul(secplane->ic, -dist - DMulScale16(secplane->a, x, secplane->b, y));
}

// Flips the plane's vertical orientiation, so that if it pointed up,
// it will point down, and vice versa.
void R_SecplaneFlipVert(secplane_t* secplane)
{
	secplane->a = -secplane->a;
	secplane->b = -secplane->b;
	secplane->c = -secplane->c;
	secplane->d = -secplane->d;
	secplane->ic = -secplane->ic;
}

// Returns true if 2 planes are the same
boolean R_ArePlanesSame(secplane_t* original, secplane_t* other)
{
	return original->a == other->a && original->b == other->b
		&& original->c == other->c && original->d == other->d;
}

// Returns true if 2 planes are different
boolean R_ArePlanesDifferent(secplane_t* original, secplane_t* other)
{
	return original->a != other->a || original->b != other->b
		|| original->c != other->c || original->d != other->d;
}

// Moves a plane up/down by hdiff units
void R_SecplaneChangeHeight(secplane_t* secplane, fixed_t hdiff)
{
	secplane->d = secplane->d - FixedMul(hdiff, secplane->c);
}

// Returns how much this plane's height would change if d were set to oldd
fixed_t R_SecplaneHeightDiff(secplane_t* secplane, fixed_t oldd)
{
	return FixedMul(oldd - secplane->d, secplane->ic);
}

fixed_t R_SecplanePointToDist(secplane_t* secplane, fixed_t x, fixed_t y, fixed_t z)
{
	return -TMulScale16(secplane->a, x, y, secplane->b, z, secplane->c);
}

fixed_t R_SecplanePointToDist2(secplane_t* secplane, fixed_t x, fixed_t y, fixed_t z)
{
	return -TMulScale16(secplane->a, x, secplane->b, y, z, secplane->c);
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// note: THIS IS USED ONLY FOR WALLS!
fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
	fixed_t scale, num;
	int anglea, angleb, sinea, sineb, den;

	anglea = ANG90 + visangle - viewangle;
	angleb = ANG90 + visangle - rw_normalangle;

	// both sines are always positive
	sinea = finesine[anglea>>ANGLETOFINESHIFT];
	sineb = finesine[angleb>>ANGLETOFINESHIFT];
	// use projectiony instead of projection for correct aspect ratio!
	num = FixedMul(projectiony, sineb);
	den = FixedMul(rw_distance, sinea);

	if(den > num>>16)
	{
		scale = FixedDiv(num, den);

		if(scale > 64*FRACUNIT)
			scale = 64*FRACUNIT;
		else if(scale < 256)
			scale = 256;
	}
	else
		scale = 64*FRACUNIT;

	return scale;
}

//
// R_InitTables
//
static inline void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int         i;
    float       a;
    float       fv;
    int         t;

    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        a = (i-FINEANGLES/4+0.5f)*PI*2/FINEANGLES;
        fv = FRACUNIT*tan (a);
        t = fv;
        finetangent[i] = t;
    }

    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
        // OPTIMIZE: mirror...
        a = (i+0.5f)*PI*2/FINEANGLES;
        t = FRACUNIT*sin (a);
        finesine[i] = t;
    }
#endif

}

//
// R_InitTextureMapping
//
static void R_InitTextureMapping (void)
{
    int                 i;
    int                 x;
    int                 t;
    fixed_t             focallength;

    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv (centerxfrac,
                            finetangent[FINEANGLES/4+/*cv_fov.value*/ FIELDOFVIEW/2] );

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        if(finetangent[i] > FRACUNIT*2)
            t = -1;
        else if(finetangent[i] < -FRACUNIT*2)
            t = viewwidth+1;
        else
        {
            t = FixedMul (finetangent[i], focallength);
            t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

            if(t < -1)
                t = -1;
            else if(t>viewwidth+1)
                t = viewwidth+1;
        }
        viewangletox[i] = t;
    }

    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.
    for (x=0;x<=viewwidth;x++)
    {
        i = 0;
        while (viewangletox[i]>x)
            i++;
        xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        t = FixedMul (finetangent[i], focallength);
        t = centerx - t;

        if(viewangletox[i] == -1)
            viewangletox[i] = 0;
        else if(viewangletox[i] == viewwidth+1)
            viewangletox[i]  = viewwidth;
    }

    clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP         2

static inline void R_InitLightTables (void)
{
    int         i;
    int         j;
    int         level;
    int         startmap;
    int         scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTZ ; j++)
        {
            //added:02-02-98:use BASEVIDWIDTH, vid.width is not set already,
            // and it seems it needs to be calculated only once.
            scale = FixedDiv ((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
            scale >>= LIGHTSCALESHIFT;
            level = startmap - scale/DISTMAP;

            if(level < 0)
                level = 0;

            if(level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            zlight[i][j] = colormaps + level*256;
        }
    }
}


//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean         setsizeneeded;

void R_SetViewSize (void)
{
    setsizeneeded = true;
}


//
// R_ExecuteSetViewSize
//


// now uses screen variables cv_viewsize, cv_detaillevel
//
void R_ExecuteSetViewSize (void)
{
	fixed_t     cosadj;
	fixed_t     dy;
	int         i;
 	int         j;
	int         level;
	int         startmap;

	int         aspectx;  //added:02-02-98:for aspect ratio calc. below...

	setsizeneeded = false;

	if(rendermode == render_none)
		return;

	// no reduced view in splitscreen mode
	if( cv_splitscreen.value && cv_viewsize.value < 11 )
		CV_SetValue (&cv_viewsize, 11);

	// added by Hurdler
	if((rendermode!=render_soft) && (cv_viewsize.value < 6))
		CV_SetValue (&cv_viewsize, 6);

	// status bar overlay at viewsize 11
	st_overlay = cv_viewsize.value == 11;

	// full screen view, without statusbar
	if(cv_viewsize.value > 10)
	{
		scaledviewwidth = vid.width;
		viewheight = vid.height;
	}
	else
	{
		// always a multiple of eight
		scaledviewwidth = (cv_viewsize.value*vid.width/10)&~7;
		// make viewheight multiple of 2, because sometimes
		// a line is not refreshed by R_DrawViewBorder()
		viewheight = (cv_viewsize.value*(vid.height)/10)&~7;
	}

	if(cv_splitscreen.value)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	centery = viewheight/2;
	centerx = viewwidth/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	projection = centerxfrac;
	projectiony = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width)<<FRACBITS;

	R_InitViewBuffer(scaledviewwidth, viewheight);

	R_InitTextureMapping();

#ifdef HWRENDER
	if(rendermode != render_soft)
		HWR_InitTextureMapping();
#endif

	pspritescale = (viewwidth<<FRACBITS)/BASEVIDWIDTH;
	pspriteiscale = (BASEVIDWIDTH<<FRACBITS)/viewwidth; // x axis scale
	pspriteyscale = (((vid.height*viewwidth)/vid.width)<<FRACBITS)/BASEVIDHEIGHT;

	// thing clipping
	for(i = 0; i < viewwidth; i++)
		screenheightarray[i] = (short)viewheight;

	// setup sky scaling (uses pspriteyscale)
	R_SetSkyScale();

	// planes
	aspectx = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width);

	if(rendermode == render_soft)
	{
		// this is only used for planes rendering in software mode
		j = viewheight*4;
		for(i = 0; i < j; i++)
		{
			dy = ((i - viewheight*2)<<FRACBITS) + FRACUNIT/2;
			dy = abs(dy);
			yslopetab[i] = FixedDiv(aspectx*FRACUNIT, dy);
		}
	}

	for(i = 0; i < viewwidth; i++)
	{
		cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
		distscale[i] = FixedDiv(FRACUNIT, cosadj);
	}

	// Calculate the light levels to use for each level/scale combination.
	for(i = 0; i< LIGHTLEVELS ; i++)
	{
		startmap = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for(j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmap - j*vid.width/(viewwidth)/DISTMAP;

			if(level < 0)
				level = 0;

			if(level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}

	// continue to do the software setviewsize as long as we use the reference software view
#ifdef HWRENDER
	if(rendermode != render_soft)
		HWR_SetViewSize(cv_viewsize.value);
#endif

	am_recalc = true;
}

//
// R_Init
//

void R_Init(void)
{
	R_LoadSkinTable();

	// screensize independent
	if(devparm)
		CONS_Printf("\nR_InitData");
	R_InitData();

	if(devparm)
		CONS_Printf("\nR_InitTables");
	R_InitTables();
	R_InitViewBorder();
	R_SetViewSize(); // setsizeneeded is set true

	if(devparm)
		CONS_Printf("\nR_InitPlanes");
	R_InitPlanes();

	// this is now done by SCR_Recalc() at the first mode set
	if(devparm)
		CONS_Printf("\nR_InitLightTables");
	R_InitLightTables();

	if(devparm)
		CONS_Printf("\nR_InitTranslationTables\n");
	R_InitTranslationTables();

	R_InitDrawNodes();

	framecount = 0;
}

//
// R_PointInSubsector
//
subsector_t* R_PointInSubsector ( fixed_t       x,
                                  fixed_t       y )
{
    node_t*     node;
    int         side;
    int         nodenum;

    // single subsector is a special case
    if(!numnodes)
        return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_IsPointInSubsector, same as above but returns 0 if not in subsector
//
subsector_t* R_IsPointInSubsector(fixed_t x, fixed_t y)
{
	node_t* node;
	int side, nodenum, i;
	subsector_t* ret;

	// single subsector is a special case
	if(!numnodes)
		return subsectors;

	nodenum = numnodes - 1;

	while(!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = R_PointOnSide(x, y, node);
		nodenum = node->children[side];
	}

	ret = &subsectors[nodenum & ~NF_SUBSECTOR];
	for(i = 0; i < ret->numlines; i++)
		if(R_PointOnSegSide(x,y,&segs[ret->firstline + i]))
			return 0;

	return ret;
}

//
// R_SetupFrame
//

static mobj_t* viewmobj;

// WARNING: a should be unsigned but to add with 2048, it isn't!
#define AIMINGTODY(a) ((finetangent[(2048+(((int)a)>>ANGLETOFINESHIFT)) & FINEMASK]*160)>>FRACBITS)

void R_SetupFrame(player_t* player)
{
	int dy = 0;
	camera_t* thiscam;

	if(cv_splitscreen.value && player == &players[secondarydisplayplayer]
		&& player != &players[consoleplayer])
	{
		thiscam = &camera2;
	}
	else
		thiscam = &camera;

	if(cv_chasecam.value && thiscam == &camera && !thiscam->chase)
	{
		P_ResetCamera(player, &camera);
		thiscam->chase = true;
	}
	else if(cv_chasecam2.value && thiscam == &camera2 && !thiscam->chase)
	{
		P_ResetCamera(player, &camera2);
		thiscam->chase = true;
	}
	else if(!cv_chasecam.value && thiscam == &camera)
		thiscam->chase = false;
	else if(!cv_chasecam2.value && thiscam == &camera2)
		thiscam->chase = false;

	if(player->awayviewtics)
	{
		// cut-away view stuff
		viewmobj = player->awayviewmobj; // should be a MT_ALTVIEWMAN
		viewz = viewmobj->z + 20*FRACUNIT;
		aimingangle = player->awayviewaiming;
		viewangle = viewmobj->angle;
	}
	else if((cv_chasecam.value && thiscam == &camera)
		|| (cv_chasecam2.value && thiscam == &camera2))
	// use outside cam view
	{
		viewmobj = player->mo; // LIES! FILTHY STINKING LIES!!!
#ifdef PARANOIA
		if(!viewmobj)
			I_Error("no mobj for the camera");
#endif
		viewz = thiscam->z + (thiscam->height>>1);
		aimingangle = thiscam->aiming;
		viewangle = thiscam->angle;
	}
	else
	// use the player's eyes view
	{
		viewz = player->viewz;

#ifdef CLIENTPREDICTION2
		if(demoplayback || !player->spirit)
		{
			viewmobj = player->mo;
			CONS_Printf("\2No spirit\n");
		}
		else
			viewmobj = player->spirit;
#else
		viewmobj = player->mo;
#endif
		aimingangle = player->aiming;
		viewangle = viewmobj->angle;

		if(!demoplayback && player->playerstate!=PST_DEAD)
		{
			if(player == &players[consoleplayer])
			{
				viewangle = localangle; // WARNING: camera uses this
				aimingangle = localaiming;
			}
			else if(player == &players[secondarydisplayplayer])
			{
				viewangle = localangle2;
				aimingangle = localaiming2;
			}
		}
	}

#ifdef PARANOIA
	if(!viewmobj)
		I_Error("R_SetupFrame: viewmobj null (player %d)", player - players);
#endif
	viewplayer = player;

	if(((cv_chasecam.value && thiscam == &camera) || (cv_chasecam2.value && thiscam == &camera2))
		&& !player->awayviewtics)
	{
		viewx = thiscam->x;
		viewy = thiscam->y;
	}
	else
	{
		viewx = viewmobj->x;
		viewy = viewmobj->y;
	}

	viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
	viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

	sscount = 0;

	// recalc necessary stuff for mouseaiming
	// slopes are already calculated for the full possible view (which is 4*viewheight).

	if(rendermode == render_soft)
	{
		// clip it in the case we are looking a hardware 90 degrees full aiming
		// (lmps, network and use F12...)
		G_ClipAimingPitch((int *)&aimingangle);

		if(!cv_splitscreen.value)
			dy = AIMINGTODY(aimingangle)* viewheight/BASEVIDHEIGHT;
		else
			dy = AIMINGTODY(aimingangle)* viewheight*2/BASEVIDHEIGHT;

		yslope = &yslopetab[(3*viewheight/2) - dy];
	}
	centery = (viewheight/2) + dy;
	centeryfrac = centery<<FRACBITS;

	framecount++;
	validcount++;
}

// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void R_RenderPlayerView(player_t* player)
{
	R_SetupFrame(player);

	// Clear buffers.
	R_ClearClipSegs();
	R_ClearDrawSegs();
	R_ClearPlanes(player); // needs player for waterheight in occupied sector
	R_ClearSprites();

#ifdef FLOORSPLATS
	R_ClearVisibleFloorSplats();
#endif

	// check for new console commands.
	NetUpdate();

	// The head node is the last node output.

//profile stuff ---------------------------------------------------------
#ifdef TIMING
	mytotal = 0;
	ProfZeroTimer();
#endif
	R_RenderBSPNode(numnodes - 1);
#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add

	CONS_Printf("RenderBSPNode: 0x%d %d\n", *((int*)&mytotal + 1), (int)mytotal);
#endif
//profile stuff ---------------------------------------------------------

	// Check for new console commands.
	NetUpdate();

	R_DrawPlanes();

	// Check for new console commands.
	NetUpdate();

#ifdef FLOORSPLATS
	R_DrawVisibleFloorSplats();
#endif

	// draw mid texture and sprite
	// And now 3D floors/sides!
	R_DrawMasked();

	// Check for new console commands.
	NetUpdate();
	player->mo->flags &= ~MF_NOSECTOR; // don't show self (uninit) clientprediction code
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	CV_RegisterVar(&cv_gravity);
	CV_RegisterVar(&cv_homing);
	CV_RegisterVar(&cv_lightdash);
	CV_RegisterVar(&cv_tailspickup);
	CV_RegisterVar(&cv_soniccd);
	CV_RegisterVar(&cv_timeattacked);
	CV_RegisterVar(&cv_allowmlook);

	//Alam: Hate!
	CV_RegisterVar(&cv_storm);
	CV_RegisterVar(&cv_rain);
	CV_RegisterVar(&cv_snow);

	// Enough for dedicated server
	if(dedicated)
		return;

	CV_RegisterVar(&cv_precipdist);
	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);
	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_numsnow);
	CV_RegisterVar(&cv_raindensity);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_speed);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);

	CV_RegisterVar(&cv_viewsize);
	CV_RegisterVar(&cv_splitscreen);

	// Default viewheight is changeable,
	// initialized to standard viewheight
	CV_RegisterVar(&cv_viewheight);
	CV_RegisterVar(&cv_grtranslucenthud);

#ifdef HWRENDER
	// GL-specific Commands
	CV_RegisterVar (&cv_grgammablue);
	CV_RegisterVar (&cv_grgammagreen);
	CV_RegisterVar (&cv_grgammared);
	CV_RegisterVar (&cv_grfovchange); // Tails
	CV_RegisterVar (&cv_grfog);
	CV_RegisterVar (&cv_grcrappymlook);
	CV_RegisterVar (&cv_voodoocompatibility);
	CV_RegisterVar (&cv_grfogcolor);
	CV_RegisterVar (&cv_grstaticlighting);
	CV_RegisterVar (&cv_grdynamiclighting);
	CV_RegisterVar (&cv_grcoronas);
	CV_RegisterVar (&cv_grcoronasize);
#endif

#ifdef HWRENDER
	if(rendermode != render_soft && rendermode != render_none)
		HWR_AddCommands();
#endif
}
