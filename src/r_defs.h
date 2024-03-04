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
/// \brief Refresh/rendering module, shared data struct definitions

#ifndef __R_DEFS__
#define __R_DEFS__

// Some more or less basic data types we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "screen.h" // MAXVIDWIDTH, MAXVIDHEIGHT

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
	int first;
	int last;
} cliprange_t;

// Silhouette, needed for clipping segs (mainly) and sprites representing things.
#define SIL_NONE   0
#define SIL_BOTTOM 1
#define SIL_TOP    2
#define SIL_BOTH   3

// This could be wider for >8 bit display.
// Indeed, true color support is possible precalculating 24bpp lightmap/colormap LUT
//  from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef byte lighttable_t;

// ExtraColormap type. Use for extra_colormaps from now on.
typedef struct
{
	unsigned short maskcolor, fadecolor;
	double maskamt;
	unsigned short fadestart, fadeend;
	int fog;

	// rgba is used in hw mode for colored sector lighting
	int rgba; // similar to maskcolor in sw mode

	lighttable_t*   colormap;
} extracolormap_t;

//
// INTERNAL MAP TYPES used by play and refresh
//

/** Your plain vanilla vertex.
  * Transformed values are not buffered locally, like soom DOOM-alikes (wt,
  * WebView) did.
  */
typedef struct
{
	fixed_t x; ///< X coordinate.
	fixed_t y; ///< Y coordinate.
	fixed_t z; ///< Z coordinate.
} vertex_t;

// Forward of linedefs, for sectors.
struct line_s;

/** Degenerate version of ::mobj_t, storing only a location.
  * Used for sound origins in sectors, hoop centers, and the like. Does not
  * handle sound from moving objects (doppler), because position is probably
  * just buffered, not updated.
  */
typedef struct
{
	thinker_t thinker; ///< Not used for anything.
	fixed_t x;         ///< X coordinate.
	fixed_t y;         ///< Y coordinate.
	fixed_t z;         ///< Z coordinate.
} degenmobj_t;

// Store fake planes in a resizable array insted of just by
// heightsec. Allows for multiple fake planes.
/** Flags describing 3Dfloor behavior and appearance.
  */
typedef enum
{
	FF_EXISTS            = 0x1,       ///< Always set, to check for validity.
	FF_SOLID             = 0x2,       ///< Clips things.
	FF_RENDERSIDES       = 0x4,       ///< Renders the sides.
	FF_RENDERPLANES      = 0x8,       ///< Renders the floor/ceiling.
	FF_RENDERALL         = 0xC,       ///< Renders everything.
	FF_SWIMMABLE         = 0x10,      ///< Is a water block.
	FF_NOSHADE           = 0x20,      ///< Messes with the lighting?
	FF_CUTSOLIDS         = 0x40,      ///< Cuts out hidden solid pixels.
	FF_CUTEXTRA          = 0x80,      ///< Cuts out hidden translucent pixels.
	FF_CUTLEVEL          = 0xC0,      ///< Cuts out all hidden pixels.
	FF_CUTSPRITES        = 0x100,     ///< Final step in making 3D water.
	FF_BOTHPLANES        = 0x200,     ///< Renders both planes all the time.
	FF_EXTRA             = 0x400,     ///< Gets cut by ::FF_CUTEXTRA.
	FF_TRANSLUCENT       = 0x800,     ///< See through!
	FF_FOG               = 0x1000,    ///< Fog "brush."
	FF_INVERTPLANES      = 0x2000,    ///< Reverse the plane visibility rules.
	FF_ALLSIDES          = 0x4000,    ///< Render inside and outside sides.
	FF_INVERTSIDES       = 0x8000,    ///< Only render inside sides.
	FF_DOUBLESHADOW      = 0x10000,   ///< Make two lightlist entries to reset light?
	FF_FLOATBOB          = 0x20000,   ///< Floats on water and bobs if you step on it.
	FF_NORETURN          = 0x40000,   ///< Used with ::FF_CRUMBLE. Will not return to its original position after falling.
	FF_CRUMBLE           = 0x80000,   ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	FF_AIRBOB            = 0x100000,  ///< Moves down by 16 units when stepped on, goes back to original position when weight is removed.
	FF_MARIO             = 0x200000,  ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	FF_BUSTUP            = 0x400000,  ///< You can spin through/punch this block and it will crumble!
	FF_QUICKSAND         = 0x800000,  ///< Quicksand!
	FF_PLATFORM          = 0x1000000, ///< You can jump up through this to the top.
	FF_SHATTER           = 0x2000000, ///< Used with ::FF_BUSTUP. Thinks everyone's Knuckles.
	FF_SPINBUST          = 0x4000000, ///< Used with ::FF_BUSTUP. Jump or fall onto it while curled in a ball.
} ffloortype_e;

typedef struct ffloor_s
{
	fixed_t* topheight;
	fixed_t* toppic;
	short* toplightlevel;
	fixed_t* topxoffs;
	fixed_t* topyoffs;

	fixed_t* bottomheight;
	fixed_t* bottompic;
	fixed_t* bottomxoffs;
	fixed_t* bottomyoffs;

	fixed_t delta;

	int secnum;
	ffloortype_e flags;
	struct line_s* master;

	struct sector_s* target;

	struct ffloor_s* next;
	struct ffloor_s* prev;

	int lastlight;
	int alpha;
} ffloor_t;


// This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
typedef struct lightlist_s
{
	fixed_t height;
	short* lightlevel;
	extracolormap_t* extra_colormap;
	int flags;
	ffloor_t* caster;
} lightlist_t;


// This struct is used for rendering walls with shadows casted on them...
typedef struct r_lightlist_s
{
	fixed_t height;
	fixed_t heightstep;
	fixed_t botheight;
	fixed_t botheightstep;
	short lightlevel;
	extracolormap_t* extra_colormap;
	lighttable_t* rcolormap;
	ffloortype_e flags;
	int lightnum;
} r_lightlist_t;

// ----- for special tricks with HW renderer -----

//
// For creating a chain with the lines around a sector
//
typedef struct linechain_s
{
	struct line_s* line;
	struct linechain_s* next;
} linechain_t;
// ----- end special tricks -----



// ZDoom C++ to Legacy C conversion Tails 04-29-2002 (for slopes)
typedef struct secplane_t
{
	// the plane is defined as a*x + b*y + c*z + d = 0
	// ic is 1/c, for faster Z calculations

	fixed_t a, b, c, d, ic;
} secplane_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef struct sector_s
{
	fixed_t floorheight;
	fixed_t ceilingheight;
	fixed_t floorpic;
	fixed_t ceilingpic;
	short lightlevel;
	short special;
	short tag;
	int nexttag, firsttag; // for fast tag searches

	// origin for any sounds played by the sector
	// also considered the center for e.g. Mario blocks
	degenmobj_t soundorg;

	// if == validcount, already checked
	int validcount;

	// list of mobjs in sector
	mobj_t* thinglist;

	// thinker_ts for reversable actions
	void* floordata; // floor move thinker
	void* ceilingdata; // ceiling move thinker
	void* lightingdata; // lighting change thinker

	// floor and ceiling texture offsets
	fixed_t floor_xoffs, floor_yoffs;
	fixed_t ceiling_xoffs, ceiling_yoffs;

	int heightsec; // other sector, or -1 if no other sector
	int altheightsec; // Use old boom model? 1 for no 0 for yes.

	int floorlightsec, ceilinglightsec;
	int crumblestate; // used for crumbling and bobbing

	int bottommap, midmap, topmap; // dynamic colormaps

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	struct msecnode_s* touching_thinglist;

	int linecount;
	struct line_s** lines; // [linecount] size

	// Improved fake floor hack
	ffloor_t* ffloors;
	size_t* attached;
	int numattached;
	lightlist_t* lightlist;
	int numlights;
	boolean moved;

	// per-sector colormaps!
	extracolormap_t* extra_colormap;

	// ----- for special tricks with HW renderer -----
	boolean pseudoSector;
	boolean virtualFloor;
	fixed_t virtualFloorheight;
	boolean virtualCeiling;
	fixed_t virtualCeilingheight;
	linechain_t* sectorLines;
	struct sector_s** stackList;
	double lineoutLength;
	// ----- end special tricks -----

	// ZDoom C++ to Legacy C conversion (for slopes)
	// store floor and ceiling planes instead of heights
	secplane_t floorplane, ceilingplane;
	fixed_t	floortexz, ceilingtexz;	// [RH] used for wall texture mapping
	fixed_t floorangle;

	// This points to the master's floorheight, so it can be changed in realtime!
	fixed_t* gravity; // per-sector gravity

	// Current speed of ceiling/floor. For Knuckles to hold onto stuff.
	fixed_t floorspeed, ceilspeed;

	// list of precipitation mobjs in sector
	precipmobj_t* preciplist;
	struct mprecipsecnode_s* touching_preciplist;
} sector_t;

//
// Move clipping aid for linedefs.
//
typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

typedef struct line_s
{
	// Vertices, from v1 to v2.
	vertex_t* v1;
	vertex_t* v2;

	fixed_t dx, dy; // Precalculated v2 - v1 for side checking.

	// Animation related.
	short flags;
	short special;
	short tag;

	// Visual appearance: sidedefs.
	short sidenum[2]; // sidenum[1] will be -1 if one-sided

	fixed_t bbox[4]; // bounding box for the extent of the linedef

	// To aid move clipping.
	slopetype_t slopetype;

	// Front and back sector.
	// Note: redundant? Can be retrieved from SideDefs.
	sector_t* frontsector;
	sector_t* backsector;

	int validcount; // if == validcount, already checked
	void* splats; // wallsplat_t list
	int firsttag, nexttag; // improves searches for tags.
} line_t;

//
// The SideDef.
//

typedef struct
{
	// add this to the calculated texture column
	fixed_t textureoffset;

	// add this to the calculated texture top
	fixed_t rowoffset;

	// Texture indices.
	// We do not maintain names here.
	int toptexture;
	int bottomtexture;
	int midtexture;

	// Sector the SideDef is facing.
	sector_t* sector;

	int special; // the special of the linedef this side belongs to
} side_t;

//
// A subsector.
// References a sector.
// Basically, this is a list of linesegs, indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
	sector_t* sector;
	short numlines;
	short firstline;
	void* splats; // floorsplat_t list
	int validcount;
} subsector_t;

// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
	sector_t*          m_sector; // a sector containing this object
	struct mobj_s*     m_thing;  // this object
	struct msecnode_s* m_tprev;  // prev msecnode_t for this thing
	struct msecnode_s* m_tnext;  // next msecnode_t for this thing
	struct msecnode_s* m_sprev;  // prev msecnode_t for this sector
	struct msecnode_s* m_snext;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} msecnode_t;

typedef struct mprecipsecnode_s
{
	sector_t*                m_sector; // a sector containing this object
	struct precipmobj_s*     m_thing;  // this object
	struct mprecipsecnode_s* m_tprev;  // prev msecnode_t for this thing
	struct mprecipsecnode_s* m_tnext;  // next msecnode_t for this thing
	struct mprecipsecnode_s* m_sprev;  // prev msecnode_t for this sector
	struct mprecipsecnode_s* m_snext;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} mprecipsecnode_t;

// for now, only used in hardware mode
// maybe later for software as well?
// that's why it's moved here
typedef struct light_s
{
	USHORT type;          // light,... (cfr #define in hwr_light.c)

	float light_xoffset;
	float light_yoffset;  // y offset to adjust corona's height

	ULONG corona_color;   // color of the light for static lighting
	float corona_radius;  // radius of the coronas

	ULONG dynamic_color;  // color of the light for dynamic lighting
	float dynamic_radius; // radius of the light ball
	float dynamic_sqrradius; // radius^2 of the light ball
} light_t;

typedef struct lightmap_s
{
	float s[2], t[2];
	light_t* light;
	struct lightmap_s* next;
} lightmap_t;

//
// The lineseg.
//
typedef struct
{
	vertex_t* v1;
	vertex_t* v2;

	int side;

	fixed_t offset;

	angle_t angle;

	side_t* sidedef;
	line_t* linedef;

	// Sector references.
	// Could be retrieved from linedef, too. backsector is NULL for one sided lines
	sector_t* frontsector;
	sector_t* backsector;

	float length; // length of the seg, used by hardware renderer

	lightmap_t* lightmaps; // for static lightmap

	// Why slow things down by calculating lightlists for every thick side?
	int numlights;
	r_lightlist_t* rlights;
} seg_t;

//
// BSP node.
//
typedef struct
{
	// Partition line.
	fixed_t x, y;
	fixed_t dx, dy;

	// Bounding box for each child.
	fixed_t bbox[2][4];

	// If NF_SUBSECTOR its a subsector.
	unsigned short children[2];
} node_t;

// posts are runs of non masked source pixels
typedef struct
{
	byte topdelta; // -1 is the last post in a column
	byte length;   // length data bytes follows
} post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;

//
// OTHER TYPES
//

#ifndef MAXFFLOORS
#define MAXFFLOORS 40
#endif

//
// ?
//
typedef struct drawseg_s
{
	seg_t* curline;
	int x1;
	int x2;

	fixed_t scale1;
	fixed_t scale2;
	fixed_t scalestep;

	int silhouette; // 0 = none, 1 = bottom, 2 = top, 3 = both

	fixed_t bsilheight; // do not clip sprites above this
	fixed_t tsilheight; // do not clip sprites below this

	// Pointers to lists for sprite clipping, all three adjusted so [x1] is first value.
	short* sprtopclip;
	short* sprbottomclip;
	short* maskedtexturecol;

	struct visplane_s* ffloorplanes[MAXFFLOORS];
	int                numffloorplanes;
	struct ffloor_s*   thicksides[MAXFFLOORS];
	short*             thicksidecol;
	int                numthicksides;
	fixed_t            frontscale[MAXVIDWIDTH];
} drawseg_t;


// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURE1 list of patches.
//
// WARNING: this structure is cloned in GlidePatch_t
struct patch_s
{
	short width;          // bounding box size
	short height;
	short leftoffset;     // pixels to the left of origin
	short topoffset;      // pixels below the origin
	int columnofs[8];     // only [width] used
	// the [0] is &columnofs[width]
};
typedef struct patch_s patch_t;

typedef enum
{
	PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
	INTENSITY       = 1,  // 1 byte intensity
	INTENSITY_ALPHA = 2,  // 2 byte: alpha then intensity
	RGB24           = 3,  // 24 bit rgb
	RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

// a pic is an unmasked block of pixels, stored in horizontal way
typedef struct
{
	short width;
	byte zero;       // set to 0 allow autodetection of pic_t
	                 // mode instead of patch or raw
	byte mode;       // see pic_mode_t above
	short height;
	short reserved1; // set to 0
	byte data[0];
} pic_t;


typedef enum
{
	SC_NONE = 0,
	SC_TOP = 1,
	SC_BOTTOM = 2
} spritecut_e;

// A vissprite_t is a thing that will be drawn during a refresh,
// i.e. a sprite object that is partly visible.
typedef struct vissprite_s
{
	// Doubly linked list.
	struct vissprite_s* prev;
	struct vissprite_s* next;

	mobj_t* mobj; // for easy access

	int x1, x2;

	fixed_t gx, gy; // for line side calculation
	fixed_t gz, gzt; // global bottom/top for silhouette clipping
	fixed_t pz, pzt; // physical bottom/top for sorting with 3D floors

	fixed_t startfrac; // horizontal position of x1
	fixed_t scale;
	fixed_t xiscale; // negative if flipped

	fixed_t texturemid;
	int patch;

	lighttable_t* colormap; // for color translation and shadow draw
	                        // maxbright frames as well

	byte* transmap; // for MF2_SHADOW sprites, which translucency table to use

	int mobjflags;

	int heightsec; // height sector for underwater/fake ceiling support

	extracolormap_t* extra_colormap; // global colormaps

	fixed_t xscale;

	// Precalculated top and bottom screen coords for the sprite.
	fixed_t thingheight; // The actual height of the thing (for 3D floors)
	sector_t* sector; // The sector containing the thing.
	fixed_t sz, szt;

	int cut; // 0 for none, bit 1 for top, bit 2 for bottom

	boolean precip;
} vissprite_t;

//
// Sprites are patches with a special naming convention so they can be
//  recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation,
//  x = 0, 1-7.
// The sprite and frame specified by a thing_t is range checked at run time.
// A sprite is a patch_t that is assumed to represent a three dimensional
//  object and may have multiple rotations predrawn.
// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used for all views: NNNNF0
//
typedef struct
{
	// If false use 0 for any position.
	// Note: as eight entries are available, we might as well insert the same
	//  name eight times.
	boolean rotate;

	// Lump to use for view angles 0-7.
	int lumppat[8]; // lump number 16:16 wad:lump
	int lumpid[8]; // id in the spriteoffset, spritewidth, etc. tables

	// Flip bit (1 = flip) to use for view angles 0-7.
	byte flip[8];
} spriteframe_t;

//
// A sprite definition:  a number of animation frames.
//
typedef struct
{
	int numframes;
	spriteframe_t* spriteframes;
} spritedef_t;

#endif
