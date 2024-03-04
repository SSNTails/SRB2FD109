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
/// \brief Wipe screen special effect.

#include "i_video.h"
#include "v_video.h"
#include "r_draw.h" // transtable
#include "p_pspr.h" // tr_transxxx
#include "f_finale.h"

#ifdef _WIN32_WCE
#define NOWIPE
#endif

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

boolean WipeInAction = 0;
#ifndef NOWIPE

static byte* wipe_scr_start; //screens 2
static byte* wipe_scr_end; //screens 3
static byte* wipe_scr; //screens 0

/**	\brief	start the wipe

	\param	width	width of wipe
	\param	height	height of wipe
	\param	ticks	ticks for wipe

	\return	unknown

	
*/
static inline int F_InitWipe(int width, int height, tic_t ticks)
{
	ticks = 0;
	memcpy(wipe_scr, wipe_scr_start, width*height*scr_bpp);
	return 0;
}

/**	\brief	wipe ticker

	\param	width	width of wipe
	\param	height	height of wipe
	\param	ticks	ticks for wipe

	\return	the change in wipe

	
*/
static int F_DoWipe(int width, int height, tic_t ticks)
{
	boolean changed = false;
	byte* w;
	byte* e;
	byte newval;
	static int slowdown = 0;

	while(ticks--)
	{
		// slowdown
		if(slowdown++)
		{
			slowdown = 0;
			return false;
		}

		w = wipe_scr;
		e = wipe_scr_end;

		while(w != wipe_scr + width*height)
		{
			if(*w != *e)
			{
				if(((newval = transtables[(*e<<8) + *w + ((tr_transmor-1)<<FF_TRANSSHIFT)]) == *w)
					&& ((newval = transtables[(*e<<8) + *w + ((tr_transmed-1)<<FF_TRANSSHIFT)]) == *w)
					&& ((newval = transtables[(*w<<8) + *e + ((tr_transmor-1)<<FF_TRANSSHIFT)]) == *w))
				{
					newval = *e;
				}
				*w = newval;
				changed = true;
			}
			w++;
			e++;
		}
	}
	return !changed;
}
#endif

/** Save the "before" screen of a wipe.
  */
void F_WipeStartScreen(void)
{
#ifndef NOWIPE
	wipe_scr_start = screens[2];
	I_ReadScreen(wipe_scr_start);
#endif
}

/** Save the "after" screen of a wipe.
  *
  * \param x      Starting x coordinate of the starting screen to restore.
  * \param y      Starting y coordinate of the starting screen to restore.
  * \param width  Width of the starting screen to restore.
  * \param height Height of the starting screen to restore.
  */
void F_WipeEndScreen(int x, int y, int width, int height)
{
#ifdef NOWIPE
	x=y=width=height=0;
#else
	wipe_scr_end = screens[3];
	I_ReadScreen(wipe_scr_end);
	V_DrawBlock(x, y, 0, width, height, wipe_scr_start);
#endif
}

/**	\brief	wipe screen

	\param	x	x starting point
	\param	y	y starting point
	\param	width	width of wipe
	\param	height	height of wipe
	\param	ticks	ticks for wipe

	\return	if true, the wipe is done

	
*/

int F_ScreenWipe(int x, int y, int width, int height, tic_t ticks)
{
	int rc = 1;
	// initial stuff
	x = y = 0;
#ifdef NOWIPE
	width = height = ticks = 0;
#else
	if(!WipeInAction)
	{
		WipeInAction = true;
		wipe_scr = screens[0];
		F_InitWipe(width, height, ticks);
	}

	rc = F_DoWipe(width, height, ticks);

	if(rc)
		WipeInAction = false; //Alam: All done?
#endif
	return rc;
}
