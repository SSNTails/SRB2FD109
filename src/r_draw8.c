// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
/// \brief 8bpp span/column drawer functions
/// 
///	NOTE: no includes because this is included as part of r_draw.c

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

int whereitsfrom = -1;

/**	\brief The R_DrawColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawColumn_8(void)
{
	int count, ccount;
	register byte* dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl + 1;

	if(count <= 0) // Zero length, column does not exceed a pixel.
		return;

	ccount = count;

//#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
	{
		CONS_Printf("R_DrawColumn_8: %d to %d at %d, from %d\n", dc_yl, dc_yh, dc_x, whereitsfrom);
		return;
	}
//#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Determine scaling, which is the only mapping to be done.

	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.

	{
		register const byte* source = dc_source;
		register const lighttable_t* colormap = dc_colormap;
		register int heightmask = dc_texheight-1;
		if(dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if(frac < 0)
				while((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				*dest = colormap[source[frac>>FRACBITS]];
				dest += vid.width;
				if((frac += fracstep) >= heightmask)
					frac -= heightmask;
			} while(--count);
		}
		else
		{
			while((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
			}
			if(count & 1)
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
		}
	}
}

/**	\brief The R_DrawSkyColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawSkyColumn_8(void)
{
	int count;
	register byte* dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl + 1;

	if(count <= 0) // Zero length, column does not exceed a pixel.
		return;

//#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
	{
		CONS_Printf
		/*I_Error*/("R_DrawSkyColumn_8: %d to %d at %d, from %d\n", dc_yl, dc_yh, dc_x, whereitsfrom);
		return;
	}
//#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.

	{
		register const byte* source = dc_source;
		register const lighttable_t* colormap = dc_colormap;
		register int heightmask = dc_texheight - 1;
		if(dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if(frac < 0)
				while((frac += heightmask) < 0)
					;
			else
				while(frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				*dest = colormap[source[frac>>FRACBITS]];
				dest += vid.width;
				if((frac += fracstep) >= heightmask)
					frac -= heightmask;
			} while(--count);
		}
		else
		{
			while((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
			}
			if(count & 1)
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
		}
	}
}

/**	\brief The R_DrawShadeColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawShadeColumn_8(void)
{
	register int count;
	register byte* dest;
	register fixed_t frac, fracstep;

	// check out coords for src*
	if((dc_yl < 0) || (dc_x >= vid.width))
		return;

	count = dc_yh - dc_yl;
	if(count < 0)
		return;

#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawShadeColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// FIXME. As above.
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Here we do an additional index re-mapping.
	do
	{
		*dest = *(colormaps + (dc_source[frac>>FRACBITS] <<8) + (*dest));
		dest += vid.width;
		frac += fracstep;
	} while(count--);
}

/**	\brief The R_DrawTranslucentColumn_8 function
	I've made an asm routine for the transparency, because it slows down
	a lot in 640x480 with big sprites (bfg on all screen, or transparent
	walls on fullscreen)
*/
void R_DrawTranslucentColumn_8(void)
{
	register int count;
	register byte* dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if(count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// FIXME. As above.
	dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.

	{
		register const byte* source = dc_source;
		register int heightmask = dc_texheight - 1;
		if(dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if(frac < 0)
				while((frac += heightmask) < 0)
					;
			else
				while(frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				// using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				*dest = dc_colormap[*(dc_transmap + (source[frac>>FRACBITS]<<8) + (*dest))];
				dest += vid.width;
				if((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while(--count);
		}
		else
		{
			while((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = dc_colormap[*(dc_transmap + (source[frac>>FRACBITS]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
				*dest = dc_colormap[*(dc_transmap + (source[frac>>FRACBITS]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
			}
			if(count & 1)
				*dest = dc_colormap[*(dc_transmap + (source[frac>>FRACBITS]<<8) + (*dest))];
		}
	}
}

/**	\brief The R_DrawTranslatedTranslucentColumn_8 function
	Spiffy function. Not only does it colormap a sprite, but does translucency as well.
	Uber-kudos to Cyan Helkaraxe
*/
void R_DrawTranslatedTranslucentColumn_8(void)
{
	register int count;
	register byte* dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if(count <= 0) // Zero length, column does not exceed a pixel.
		return;

	// FIXME. As above.
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl - centery)*fracstep;

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.

	{
		register int heightmask = dc_texheight - 1;
		if(dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if(frac < 0)
				while((frac += heightmask) < 0)
					;
			else
				while(frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];

				dest += vid.width;
				if((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while(--count);
		}
		else
		{
			while((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
				*dest = dc_colormap[*(dc_transmap
					+ (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest))];
				dest += vid.width;
				frac += fracstep;
			}
			if(count & 1)
				*dest = dc_colormap[*( dc_transmap + (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]] <<8) + (*dest) )];
		}
	}
}

/**	\brief The R_DrawShadeColumn_8 function
	Draw columns up to 128 high but remap the green ramp to other colors

  \warning STILL NOT IN ASM, TO DO..
*/
void R_DrawTranslatedColumn_8(void)
{
	register int count;
	register byte* dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl;
	if(count < 0)
		return;

#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// FIXME. As above.
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;
	if(dc_hires)
		frac = 0;

	// Here we do an additional index re-mapping.
	do
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		*dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];

		dest += vid.width;

		frac += fracstep;
	} while(count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

/**	\brief The R_DrawSpan_8 function
	Draws the actual span.
*/
void R_DrawSpan_8(void)
{
	register ULONG xfrac, yfrac;
	register byte* dest;
	register int count;

#ifdef RANGECHECK
	if(ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || (unsigned)ds_y > (unsigned)vid.height)
		I_Error("R_DrawSpan_8: %d to %d at %d", ds_x1, ds_x2, ds_y);
#endif

	xfrac = ds_xfrac & ((flatsize<<FRACBITS)-1);
	yfrac = ds_yfrac;

	dest = ylookup[ds_y] + columnofs[ds_x1];

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1;

	do
	{
		count = count;
		// Lookup pixel from flat texture tile, re-index using light/colormap.
		*dest++ = ds_colormap[ds_source[((yfrac>>(16-flatsubtract))&(flatmask)) | (xfrac>>16)]];

		// Next step in u, v.
		xfrac += ds_xstep;
		yfrac += ds_ystep;
		xfrac &= (flatsize<<FRACBITS)-1;
	} while(count--);
}

/**	\brief The R_DrawTranslucentSpan_8 function
	Draws the actual span with translucent.
*/
void R_DrawTranslucentSpan_8(void)
{
	fixed_t xfrac, yfrac, xstep, ystep;
	byte* dest;
	int count;

#ifdef RANGECHECK
	if(ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= vid.width || (unsigned)ds_y > (unsigned)vid.height)
		I_Error("R_DrawTranslucentSpan_8: %d to %d at %d", ds_x1, ds_x2, ds_y);
#endif

	xfrac = ds_xfrac & ((flatsize<<FRACBITS)-1);
	yfrac = ds_yfrac;

	dest = ylookup[ds_y] + columnofs[ds_x1];

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1 + 1;

	xstep = ds_xstep;
	ystep = ds_ystep;

	do
	{
		// Current texture index in u, v.

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = ds_colormap[*(ds_transmap + (ds_source[((yfrac>>(16-flatsubtract))&(flatmask))
			| (xfrac>>16)] << 8) + (*dest))];
		dest++;

		// Next step in u, v.
		xfrac += xstep;
		yfrac += ystep;
		xfrac &= ((flatsize<<FRACBITS)-1);
	} while(--count);
}

/**	\brief The R_DrawFogSpan_8 function
	Draws the actual span with fogging.
*/
void R_DrawFogSpan_8(void)
{
	byte* colormap;
	byte* transmap;
	byte* dest;

	unsigned count;

	colormap = ds_colormap;
	transmap = ds_transmap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	count = ds_x2 - ds_x1 + 1;

	while(count >= 4)
	{
		dest[0] = colormap[dest[0]];
		dest[1] = colormap[dest[1]];
		dest[2] = colormap[dest[2]];
		dest[3] = colormap[dest[3]];

		dest += 4;
		count -= 4;
	}

	while(count--)
	{
		*dest = colormap[*dest];
		dest++;
	}
}

/**	\brief The R_DrawFogColumn_8 function
	Fog wall.
*/
void R_DrawFogColumn_8(void)
{
	int count;
	byte* dest;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if(count < 0)
		return;

#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawFogColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Determine scaling, which is the only mapping to be done.
	do
	{
		//Simple. Apply the colormap to what's already on the screen.
		*dest = dc_colormap[*dest];
		dest += vid.width;
	} while(count--);
}

/**	\brief The R_DrawShadeColumn_8 function
	This is for 3D floors that cast shadows on walls.
 
	This function just cuts the column up into sections and calls R_DrawColumn_8
*/
void R_DrawColumnShadowed_8(void)
{
	int count, realyh, realyl, i, height, bheight = 0, solid = 0;

	realyh = dc_yh;
	realyl = dc_yl;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if(count < 0)
		return;

#ifdef RANGECHECK
	if((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawShadowedColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// This runs through the lightlist from top to bottom and cuts up the column accordingly.
	for(i = 0; i < dc_numlights; i++)
	{
		// If the height of the light is above the column, get the colormap
		// anyway because the lighting of the top should be affected.
		solid = dc_lightlist[i].flags & FF_CUTSOLIDS;

		height = dc_lightlist[i].height >> 12;
		if(solid)
			bheight = dc_lightlist[i].botheight >> 12;
		if(height <= dc_yl)
		{
			dc_colormap = dc_lightlist[i].rcolormap;
			if(solid && dc_yl < bheight)
			{
				dc_yl = bheight;
				// don't set whereitsfrom here, dc_yl can't be too low now unless
				// it was already too low before
			}
			continue;
		}
		// Found a break in the column!
		dc_yh = height;

		if(dc_yh > realyh)
			dc_yh = realyh;
		R_DrawColumn_8();
		if(solid)
		{
			dc_yl = bheight;
			whereitsfrom = 2;
		}
		else
		{
			dc_yl = dc_yh + 1;
			whereitsfrom = 3;
		}

		dc_colormap = dc_lightlist[i].rcolormap;
	}
	dc_yh = realyh;
	if(dc_yl <= realyh)
		R_DrawColumn_8();
}
