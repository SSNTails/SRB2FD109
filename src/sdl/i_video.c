// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_video.c,v 1.13 2004/05/16 19:11:53 hurdler Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
//
// $Log: i_video.c,v $
// Revision 1.13  2004/05/16 19:11:53  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.12  2002/07/01 19:59:59  metzgermeister
// *** empty log message ***
//
// Revision 1.11  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
// .
//
// Revision 1.10  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.9  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.8  2001/04/28 14:25:03  metzgermeister
// fixed mouse and menu bug
//
// Revision 1.7  2001/04/27 13:32:14  bpereira
// no message
//
// Revision 1.6  2001/03/12 21:03:10  metzgermeister
//   * new symbols for rendererlib added in SDL
//   * console printout fixed for Linux&SDL
//   * Crash fixed in Linux SW renderer initialization
//
// Revision 1.5  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.4  2001/02/24 13:35:23  bpereira
// no message
//
// Revision 1.3  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.2  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.1  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 graphics stuff for SDL

#include <stdlib.h>

#include <SDL/SDL.h>
#ifdef HAVE_IMAGE
#include <SDL/SDL_image.h>
#endif

#include "../doomdef.h"

#if defined(_WIN32) || defined(_WIN64)
#include <SDL/SDL_syswm.h>
#endif

#ifdef _arch_dreamcast
//#include <kos.h>
#include <conio/conio.h>
#endif

#include "../doomstat.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_sound.h"  // midi pause/unpause
#include "../g_input.h"
#include "../st_stuff.h"
#include "../g_game.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"
// For dynamic referencing of HW rendering functions
#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif

#ifdef HAVE_FILTER
#define FILTERS
#include "filter/filters.h"
#endif

#if !defined(HAVE_IMAGE) && !(defined(DC) || defined(_WIN32_WCE))
#define LOAD_XPM //I want XPM!
#include "IMG_xpm.c" //Alam: I don't want to add SDL_Image.dll/so
#define HAVE_IMAGE //I have SDL_Image, sortof
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

// maximum number of windowed modes (see windowedModes[][])
#if defined (_WIN32_WCE) || defined(DC)
#define MAXWINMODES (1)
#else
#define MAXWINMODES (15)
#endif

/**	\brief
*/
static int numVidModes = 0;

/**	\brief
*/
static char vidModeName[33][32]; // allow 33 different modes

#ifdef __APPLE_CC__
rendermode_t rendermode=render_opengl;
#else
rendermode_t rendermode=render_soft;
#endif

boolean highcolor = false;

// synchronize page flipping with screen refresh
// unused and for compatibilityy reason
consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_stretch = {"stretch", "On", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

byte graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static boolean disable_fullscreen = false;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
static boolean disable_mouse = false;
#define USE_MOUSEINPUT !disable_mouse && cv_usemouse.value && SDL_GetAppState() & SDL_APPACTIVE
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// first entry in the modelist which is not bigger than MAXVIDWIDTHxMAXVIDHEIGHT
static       int          firstEntry = 0;

// SDL vars
#ifndef HWRENDER //[segabor] !!! I had problem compiling this source with gcc 3.3
static       SDL_Surface *vidSurface = NULL;
#endif
static       SDL_Surface *bufSurface = NULL;
static       SDL_Surface *icoSurface = NULL;
static       SDL_Color    localPalette[256];
static       SDL_Rect   **modeList = NULL;
static       Uint8        BitsPerPixel = 8;
static       Uint16       realwidth = BASEVIDWIDTH;
static       Uint16       realheight = BASEVIDHEIGHT;
#ifdef _WIN32_WCE
static const Uint32       surfaceFlagsW = SDL_HWPALETTE; //Can't handle WinCE changing sides
#else
static const Uint32       surfaceFlagsW = SDL_HWPALETTE|SDL_RESIZABLE;
#endif
static const Uint32       surfaceFlagsF = SDL_HWPALETTE|SDL_FULLSCREEN;
static       SDL_bool     mousegrabok = SDL_FALSE;
#define HalfWarpMouse(x,y) SDL_WarpMouse((Uint16)(x/2),(Uint16)(y/2))
static       SDL_bool     videoblitok = SDL_FALSE;
static       SDL_bool     exposevideo = SDL_FALSE;

// windowed video modes from which to choose from.
static int windowedModes[MAXWINMODES][2] =
{
#if !defined (_WIN32_WCE) && !defined(DC)
	{MAXVIDWIDTH /*1920*/, MAXVIDHEIGHT/*1200*/}, //1.60, 6.00
	{1600,1200}, // 1.33,5.00
	{1600,1000}, // 1.60,5.00
	{1536,1152}, // 1.33,4.80
	{1280, 960}, // 1.33,4.00
	{1280, 800}, // 1.60,4.00
	{1024, 768}, // 1.33,3.20
	{960 , 600}, // 1.60,3.00
	{800 , 600}, // 1.33,2.50
	{640 , 480}, // 1.33,2.00
	{640 , 400}, // 1.60,2.00
	{512 , 384}, // 1.33,1.60
	{400 , 300}, // 1.33,1.25
	{320 , 240}, // 1.33,1.00
#endif
	{320 , 200}, // 1.60,1.00
};

static void SDL_SetMode(int width, int height, int bpp, Uint32 flags)
{
#ifdef _WIN32_WCE
	if(bpp < 16)
		bpp =16; // 256 mode poo
#endif
#ifdef FILTERS
	bpp = Setupf2x(width, height, bpp);
#endif
	if(cv_vidwait.value && videoblitok && SDL_VideoModeOK(width, height, bpp, flags|SDL_HWSURFACE|SDL_DOUBLEBUF) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_HWSURFACE|SDL_DOUBLEBUF);
	else if(videoblitok && SDL_VideoModeOK(width, height, bpp, flags|SDL_HWSURFACE) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_HWSURFACE);
	else if(SDL_VideoModeOK(width, height, bpp, flags|SDL_SWSURFACE) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_SWSURFACE);
	else return;
	realwidth = (Uint16)width;
	realheight = (Uint16)height;
#ifdef FILTERS
	if(vidSurface && preSurface && f2xSurface)
	{
		vid.width = width/2;
		vid.height = height/2;
	}
#endif
}

//
//  Translates the SDL key into SRB2 key
//

static int xlatekey(SDLKey sym)
{
	int rc=sym + 0x80;

	switch(sym)
	{
		case SDLK_LEFT:  rc = KEY_LEFTARROW;     break;
		case SDLK_RIGHT: rc = KEY_RIGHTARROW;    break;
		case SDLK_DOWN:  rc = KEY_DOWNARROW;     break;
		case SDLK_UP:    rc = KEY_UPARROW;       break;

		case SDLK_ESCAPE:   rc = KEY_ESCAPE;        break;
		case SDLK_SPACE:    rc = KEY_SPACE;         break;
		case SDLK_RETURN:   rc = KEY_ENTER;         break;
		case SDLK_TAB:      rc = KEY_TAB;           break;
		case SDLK_F1:       rc = KEY_F1;            break;
		case SDLK_F2:       rc = KEY_F2;            break;
		case SDLK_F3:       rc = KEY_F3;            break;
		case SDLK_F4:       rc = KEY_F4;            break;
		case SDLK_F5:       rc = KEY_F5;            break;
		case SDLK_F6:       rc = KEY_F6;            break;
		case SDLK_F7:       rc = KEY_F7;            break;
		case SDLK_F8:       rc = KEY_F8;            break;
		case SDLK_F9:       rc = KEY_F9;            break;
		case SDLK_F10:      rc = KEY_F10;           break;
		case SDLK_F11:      rc = KEY_F11;           break;
		case SDLK_F12:      rc = KEY_F12;           break;

		case SDLK_BACKSPACE: rc = KEY_BACKSPACE;    break;
		case SDLK_DELETE:    rc = KEY_DEL;          break;

		case SDLK_KP_EQUALS: //Alam & Logan: WTF? Mac KB haves one! XD 
		case SDLK_PAUSE:
			rc = KEY_PAUSE;
			break;

		case SDLK_EQUALS:
		case SDLK_PLUS:      rc = KEY_EQUALS;       break;

		case SDLK_MINUS:     rc = KEY_MINUS;        break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			rc = KEY_SHIFT;
			break;

		case SDLK_CAPSLOCK:  rc = KEY_CAPSLOCK;     break;

		case SDLK_LCTRL:
		case SDLK_RCTRL:
			rc = KEY_CTRL;
			break;

		case SDLK_LALT:
		case SDLK_RALT:
			rc = KEY_ALT;
			break;

		case SDLK_NUMLOCK:   rc = KEY_NUMLOCK;    break;
		case SDLK_SCROLLOCK: rc = KEY_SCROLLLOCK; break;

		case SDLK_PAGEUP:   rc = KEY_PGUP; break;
		case SDLK_PAGEDOWN: rc = KEY_PGDN; break;
		case SDLK_END:      rc = KEY_END;  break;
		case SDLK_HOME:     rc = KEY_HOME; break;
		case SDLK_INSERT:   rc = KEY_INS;  break;

		case SDLK_KP0: rc = KEY_KEYPAD0;  break;
		case SDLK_KP1: rc = KEY_KEYPAD1;  break;
		case SDLK_KP2: rc = KEY_KEYPAD2;  break;
		case SDLK_KP3: rc = KEY_KEYPAD3;  break;
		case SDLK_KP4: rc = KEY_KEYPAD4;  break;
		case SDLK_KP5: rc = KEY_KEYPAD5;  break;
		case SDLK_KP6: rc = KEY_KEYPAD6;  break;
		case SDLK_KP7: rc = KEY_KEYPAD7;  break;
		case SDLK_KP8: rc = KEY_KEYPAD8;  break;
		case SDLK_KP9: rc = KEY_KEYPAD9;  break;

		case SDLK_KP_PERIOD:   rc = KEY_KPADDEL;   break;
		case SDLK_KP_DIVIDE:   rc = KEY_KPADSLASH; break;
		case SDLK_KP_MULTIPLY: rc = '*';           break;
		case SDLK_KP_MINUS:    rc = KEY_MINUSPAD;  break;
		case SDLK_KP_PLUS:     rc = KEY_PLUSPAD;   break;
		case SDLK_KP_ENTER:    rc = KEY_ENTER;     break;

		case SDLK_LSUPER:
		case SDLK_LMETA:
			rc = KEY_LEFTWIN;  break;
		case SDLK_RSUPER:
		case SDLK_RMETA:
			rc = KEY_RIGHTWIN; break;

		case SDLK_MENU:        rc = KEY_MENU;      break;

		default:
			if (sym >= SDLK_SPACE && sym <= SDLK_DELETE)
				rc = sym - SDLK_SPACE + ' ';
			else if (sym >= 'A' && sym <= 'Z')
				rc = sym - 'A' + 'a';
			else if(sym && devparm)
				CONS_Printf("\2Unknown Keycode %i, Name: %s\n",sym, SDL_GetKeyName( sym ));
			else if(!sym) rc = 0;
			break;
	}

	return rc;
}

static void doGrabMouse(void)
{
	if(SDL_GRAB_OFF == SDL_WM_GrabInput(SDL_GRAB_QUERY))
	{
		if(mousegrabok) SDL_WM_GrabInput(SDL_GRAB_ON);
	}
}

static void doUngrabMouse(void)
{
	if(SDL_GRAB_ON == SDL_WM_GrabInput(SDL_GRAB_QUERY))
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf ("%d video mode(s) available(s)\n", VID_NumModes());
}

static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	if(infoSurface)
	{
		int vfBPP;
		vfBPP = infoSurface->format?infoSurface->format->BitsPerPixel:0;
		if(SurfaceText) CONS_Printf("%s:\n",SurfaceText);
		else CONS_Printf("Unknown Surface:\n");
		{ //w,h,bpp
			CONS_Printf(" %ix%i at %i bit color\n",infoSurface->w,infoSurface->h,vfBPP);
		}
		{
			SDL_Surface *VidSur;//flags
			VidSur  = SDL_GetVideoSurface();
			if(infoSurface->flags&SDL_HWSURFACE)
				CONS_Printf(" Stored in video memory\n");
			else if(infoSurface->flags&SDL_OPENGL)
				CONS_Printf(" Stored in an OpenGL context\n");
			else
			{
				if(infoSurface->flags&SDL_PREALLOC)
					CONS_Printf(" Uses preallocated memory\n");
				else
					CONS_Printf(" Stored in system memory\n");
			}
			if(infoSurface->flags&SDL_ASYNCBLIT)
				CONS_Printf(" Uses asynchronous blits if possible\n");
			else
				CONS_Printf(" Uses synchronous blits if possible\n");
			if(infoSurface->flags&SDL_ANYFORMAT)
				CONS_Printf(" Allows any pixel-format\n");
			if(infoSurface->flags&SDL_HWPALETTE)
				CONS_Printf(" Has exclusive palette access\n");
			else if(VidSur == infoSurface)
				CONS_Printf(" Has nonexclusive palette access\n");
			if(infoSurface->flags&SDL_DOUBLEBUF)
				CONS_Printf(" Double buffered\n");
			else if(VidSur == infoSurface)
				CONS_Printf(" No hardware flipping\n");
			if(infoSurface->flags&SDL_FULLSCREEN)
				CONS_Printf(" Full screen\n");
			else if(infoSurface->flags&SDL_RESIZABLE)
				CONS_Printf(" Resizable window\n");
			else if(VidSur == infoSurface)
				CONS_Printf(" Nonresizable window\n");
			if(infoSurface->flags&SDL_HWACCEL)
				CONS_Printf(" Uses hardware acceleration blit\n");
			if(infoSurface->flags&SDL_SRCCOLORKEY)
				CONS_Printf(" Use colorkey blitting\n");
			if(infoSurface->flags&SDL_RLEACCEL)
				CONS_Printf(" Colorkey RLE acceleration blit\n");
			if(infoSurface->flags&SDL_SRCALPHA)
				CONS_Printf(" Use alpha blending acceleration blit\n");
		}
		
	}
}

static void VID_Command_Info_f (void)
{
	const SDL_VideoInfo *videoInfo;
	videoInfo = SDL_GetVideoInfo(); //Alam: Double-Check
	if(videoInfo)
	{
		CONS_Printf("Video Interface Capabilities:\n");
		if(videoInfo->hw_available)
			CONS_Printf(" Hardware surfaces\n");
		if(videoInfo->wm_available)
		{
			char videodriver[4] = {'S','D','L',0};
			CONS_Printf(" Window manager\n");
			if(!M_CheckParm("-nomousegrab") && SDL_VideoDriverName(videodriver,4))
			{
				if(M_CheckParm("-mousegrab") ||
				strcasecmp("X11",videodriver)) //X11's XGrabPointer not good
					mousegrabok = SDL_TRUE;
				else
					mousegrabok = SDL_FALSE;
			}
		}
		//UnusedBits1  :6
		//UnusedBits2  :1
		if(videoInfo->blit_hw)
			CONS_Printf(" Accelerated blits HW-2-HW\n");
		if(videoInfo->blit_hw_CC)
			CONS_Printf(" Accelerated blits HW-2-HW with Colorkey\n");
		if(videoInfo->wm_available)
			CONS_Printf(" Accelerated blits HW-2-HW with Alpha\n");
		if(videoInfo->blit_sw)
		{
			CONS_Printf(" Accelerated blits SW-2-HW\n");
			if(!M_CheckParm("-noblit")) videoblitok = SDL_TRUE;
		}
		if(videoInfo->blit_sw_CC)
			CONS_Printf(" Accelerated blits SW-2-HW with Colorkey\n");
		if(videoInfo->blit_sw_A)
			CONS_Printf(" Accelerated blits SW-2-HW with Alpha\n");
		if(videoInfo->blit_fill)
			CONS_Printf(" Accelerated Color filling, Hmmm\n");
		//UnusedBits3  :16
		if(videoInfo->video_mem)
			CONS_Printf(" There %i MB of video memory\n",videoInfo->video_mem/1024);
		else
			CONS_Printf(" There no video memory for SDL\n");
		//*vfmt
	}
	else mousegrabok = SDL_TRUE; //Alam: ok....
	SurfaceInfo(bufSurface,"Current Engine Mode");
#ifdef FILTERS
	SurfaceInfo(preSurface,"Prebuffer Mode");
	SurfaceInfo(f2xSurface,"Postbuffer Mode");
#endif
	SurfaceInfo(vidSurface,"Current Video Mode");
}

static void VID_Command_ModeList_f(void)
{
	int i;
#ifdef HWRENDER
	if(rendermode == render_opengl || M_CheckParm("-opengl"))
		modeList = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);
	else
#endif
	modeList = SDL_ListModes(NULL, surfaceFlagsF|SDL_HWSURFACE); //Alam: At least hardware surface

	if(modeList == (SDL_Rect **)0 && cv_fullscreen.value)
	{
		CONS_Printf("No video modes present\n");
		cv_fullscreen.value = 0;
	}
	else if(modeList != (SDL_Rect **)0)
	{
		numVidModes = 0;
		if(modeList == (SDL_Rect **)-1)
			numVidModes = -1; // should not happen with fullscreen modes
		else while(modeList[numVidModes])
			numVidModes++;
	}
	CONS_Printf("Found %d FullScreen Video Modes:\n", numVidModes);
	for (i=0 ; i<numVidModes; i++)
	{ // fullscreen modes
		int modeNum = firstEntry + i;
		if(modeNum >= numVidModes)
			break;
		
		CONS_Printf("%dx%d and ",
				modeList[modeNum]->w,
				modeList[modeNum]->h);
	}
	CONS_Printf("None\n");
}

static void VID_Command_Mode_f (void)
{
	int modenum;

	if (COM_Argc()!= 2)
	{
		CONS_Printf ("vid_mode <modenum> : set video mode, currect video mode %i\n",vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf ("No such video mode\n");
	else
		setmodeneeded = modenum+1; // request vid mode change
}

void I_GetEvent(void)
{
	SDL_Event inputEvent;
	static SDL_bool sdlquit = SDL_FALSE; //Alam: once, just once
	event_t event = {0,0,0,0};

	if(!graphics_started)
	{
		event.type = ev_keydown;
		event.data1 = I_ConsoleKey();
		I_OutputMsg("%i",event.data1);
		if(event.data1)
		{
			D_PostEvent(&event);
			event.type = ev_keyup;
			D_PostEvent(&event);
		}
		return;
	}

	while(SDL_PollEvent(&inputEvent))
	{
		memset(&event,0,sizeof(event_t));
		switch(inputEvent.type)
		{
			case SDL_ACTIVEEVENT:
				if(SDL_APPACTIVE & inputEvent.active.state || SDL_APPINPUTFOCUS & inputEvent.active.state)
				{
					// pause music when alt-tab
					if( inputEvent.active.gain /*&& !paused */)
					{
                        static SDL_bool firsttimeonmouse = SDL_TRUE;
						if(!firsttimeonmouse)
						{
                               if(cv_usemouse.value) I_StartupMouse();
                        }
                        else firsttimeonmouse = SDL_FALSE;       
						//if(!netgame && !con_destlines) paused = false;
						if(gamestate == GS_LEVEL || gamestate == GS_DEMOSCREEN)
							if(!paused) I_ResumeSong(0); //resume it
					}
					else /*if (!paused)*/
					{
						static SDL_bool windownnow = SDL_FALSE;
						doUngrabMouse();
						if(!netgame && gamestate == GS_LEVEL) paused = true;
						memset(gamekeydown, 0, NUMKEYS);
						//S_PauseSound();
						if(gamestate == GS_LEVEL || gamestate == GS_DEMOSCREEN)
							I_PauseSong(0); //pause it
						if(cv_fullscreen.value && windownnow)
						{
							if(SDL_WM_ToggleFullScreen(vidSurface))
							{
								cv_fullscreen.value = 0;
								vid.modenum = VID_GetModeForSize(realwidth,realheight);
							}
							else CV_SetValue(&cv_fullscreen,0);
						}
						else if(cv_fullscreen.value && !windownnow)
							windownnow = SDL_TRUE;
						else windownnow = SDL_FALSE;
					}
				}
				if(SDL_APPMOUSEFOCUS&inputEvent.active.state && USE_MOUSEINPUT && !inputEvent.active.gain)
					HalfWarpMouse(realwidth, realheight);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				/// \todo inputEvent.key.which?
				if(inputEvent.type == SDL_KEYUP)
					event.type = ev_keyup;
				else if(inputEvent.type == SDL_KEYDOWN)
					event.type = ev_keydown;
				else break;
				event.data1 = xlatekey(inputEvent.key.keysym.sym);
				if(event.data1) D_PostEvent(&event);
				break;
			case SDL_MOUSEMOTION:
				/// \todo inputEvent.motion.which
				if(USE_MOUSEINPUT)
				{
					// If the event is from warping the pointer back to middle
					// of the screen then ignore it.
					if ((inputEvent.motion.x == realwidth/2) &&
						(inputEvent.motion.y == realheight/2))
					{
						break;
					}
					else
					{
						event.data2 = +inputEvent.motion.xrel;
						event.data3 = -inputEvent.motion.yrel;
					}
					event.type = ev_mouse;
					D_PostEvent(&event);
					// Warp the pointer back to the middle of the window
					//  or we cannot move any further if it's at a border.
					if ((inputEvent.motion.x < (realwidth/2 )-(realwidth/4 )) ||
						 (inputEvent.motion.y < (realheight/2)-(realheight/4)) ||
						 (inputEvent.motion.x > (realwidth/2 )+(realwidth/4 )) ||
						 (inputEvent.motion.y > (realheight/2)+(realheight/4) ) )
					{
						//if(SDL_GRAB_ON == SDL_WM_GrabInput(SDL_GRAB_QUERY) || !mousegrabok)
							HalfWarpMouse(realwidth, realheight);
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				/// \todo inputEvent.button.which
				if(USE_MOUSEINPUT)
				{
					if(inputEvent.type == SDL_MOUSEBUTTONUP)
						event.type = ev_keyup;
					else if(inputEvent.type == SDL_MOUSEBUTTONDOWN)
						event.type = ev_keydown;
					else break;
					if(inputEvent.button.button==SDL_BUTTON_WHEELUP || inputEvent.button.button==SDL_BUTTON_WHEELDOWN)
					{
						if(inputEvent.type == SDL_MOUSEBUTTONUP)
							event.data1 = 0; //Alam: dumb! this could be a real button with some mice
						else
							event.data1 = KEY_MOUSEWHEELUP + inputEvent.button.button - SDL_BUTTON_WHEELUP;
					}
					else if(inputEvent.button.button <= MOUSEBUTTONS)
					{
						event.data1 = KEY_MOUSE1 + inputEvent.button.button - SDL_BUTTON_LEFT;
					}
					if(event.data1) D_PostEvent(&event);
				}
				break;
			case SDL_QUIT:
				if(!sdlquit)
				{
					sdlquit = SDL_TRUE;
					M_QuitResponse('y');
				}
				break;
			case SDL_VIDEORESIZE:
				if(gamestate == GS_LEVEL || gamestate == GS_DEMOSCREEN || gamestate == GS_TITLESCREEN || gamestate == GS_EVALUATION)
					setmodeneeded = VID_GetModeForSize(inputEvent.resize.w,inputEvent.resize.h)+1;
				{
#ifdef FILTERS
					int filtervalue = cv_filter.value;
					if(blitfilter) CV_SetValue(&cv_filter,1);
#endif
					SDL_SetMode(realwidth, realheight, vid.bpp*8, surfaceFlagsW);
					if(vidSurface) SDL_SetColors(vidSurface, localPalette, 0, 256);
#ifdef FILTERS
					CV_SetValue(&cv_filter,filtervalue);
#endif
				}
				if(!vidSurface)
					I_Error("Could not reset vidmode: %s\n",SDL_GetError());
				break;
			case SDL_VIDEOEXPOSE:
				//exposevideo = SDL_TRUE;
				break;
			default:
				break;
		}
	}
	//reset wheel like in win32, I don't understand it but works
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;
	if(!firsttimeonmouse)
		HalfWarpMouse(realwidth, realheight); // warp to center
	else
		firsttimeonmouse = SDL_FALSE;
	if(!disable_mouse && cv_usemouse.value)
	{
		doGrabMouse();
	}
	else
	{
		doUngrabMouse();
	}
}

//
// I_OsPolling
//
void I_OsPolling(void)
{

	if(SDL_WasInit(SDL_INIT_JOYSTICK)!=0) SDL_JoystickUpdate();
	I_GetJoystickEvents();
	I_GetJoystick2Events();
#if defined (LMOUSE2) || defined(_WIN32) || defined(_WIN64)
	I_GetMouseEvents();
#endif

	I_GetEvent();
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
	if(!vidSurface || !exposevideo)
		return;
	if(rendermode == render_soft)
		SDL_UpdateRect(vidSurface, 0, 0, 0, 0);
#ifdef HWRENDER
	else
		OglSdlFinishUpdate(cv_vidwait.value);
#endif
	exposevideo = SDL_FALSE;
}

#define FPSPOINTS  35
#define SCALE      4
#define PUTDOT(xx,yy,cc) screens[0][((yy)*vid.width+(xx))*vid.bpp]=(cc)

static int fpsgraph[FPSPOINTS];

static void displayticrate()
{
	int j,l,i;
	static tic_t lasttic;
	tic_t        tics,t;

	t = I_GetTime();
	tics = t - lasttic;
	lasttic = t;
	if (tics > 20) tics = 20;

	for (i=0;i<FPSPOINTS-1;i++)
		fpsgraph[i]=fpsgraph[i+1];
	fpsgraph[FPSPOINTS-1]=20-tics;

	if( rendermode == render_soft )
	{
		int k;
		// draw dots
		for(j=0;j<=20*SCALE*vid.dupy;j+=2*SCALE*vid.dupy)
		{
			l=(vid.height-1-j)*vid.width*vid.bpp;
			for (i=0;i<FPSPOINTS*SCALE*vid.dupx;i+=2*SCALE*vid.dupx)
				screens[0][l+i]=0xff;
		}

		// draw the graph
		for (i=0;i<FPSPOINTS;i++)
			for(k=0;k<SCALE*vid.dupx;k++)
				PUTDOT(i*SCALE*vid.dupx+k, vid.height-1-(fpsgraph[i]*SCALE*vid.dupy),0xff);
	}
#ifdef HWRENDER
	else
	{
		fline_t p;
		for(j=0;j<=20*SCALE*vid.dupy;j+=2*SCALE*vid.dupy)
		{
			l=(vid.height-1-j);
			for (i=0;i<FPSPOINTS*SCALE*vid.dupx;i+=2*SCALE*vid.dupx)
			{
				p.a.x = i;
				p.a.y = l;
				p.b.x = i+1;
				p.b.y = l;
				HWR_drawAMline(&p, 0xff);
			}
		}

		for (i=1;i<FPSPOINTS;i++)
		{
			p.a.x = SCALE * (i-1);
			p.a.y = vid.height-1-fpsgraph[i-1]*SCALE*vid.dupy;
			p.b.x = SCALE * i;
			p.b.y = vid.height-1-fpsgraph[i]*SCALE*vid.dupy;
			HWR_drawAMline(&p, 0xff);
		}
	}
#endif
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
	if(!vidSurface)
		return; //Alam: No software or OpenGl surface
	if (cv_ticrate.value)
		displayticrate();
	if(render_soft == rendermode && screens[0])
	{
		SDL_PixelFormat *vidformat = vidSurface->format;
		int lockedsf = 0, blited = 0, vfBPP;
		vfBPP = vidformat?vidformat->BitsPerPixel:0;

		if(!bufSurface) //Double-Check
		{
			if(vid.bpp == 1) bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,8,
				vid.rowbytes,0x00000000,0x00000000,0x00000000,0x00000000); // 256 mode
			else if(vid.bpp == 2) bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,15,
				vid.rowbytes,0x00007C00,0x000003E0,0x0000001F,0x00000000); // 555 mode
			if(bufSurface) SDL_SetColors(bufSurface, localPalette, 0, 256);
		}

#ifdef FILTERS
		FilterBlit(bufSurface);
		if(f2xSurface) //Alam: filter!
		{
			blited = SDL_BlitSurface(f2xSurface,NULL,vidSurface,NULL);
		}
		else
#endif
		if(((vfBPP == 8 && vid.bpp == 1 && !vidformat->Rmask &&
		 !vidformat->Gmask && !vidformat->Bmask) || (vfBPP == 15 && vid.bpp == 2
		 && vidformat->Rmask == 0x7C00 && vidformat->Gmask == 0x03E0 &&
		 vidformat->Bmask == 0x001F )) && !vidformat->Amask)//Alam: DOS Way
		{
			if(SDL_MUSTLOCK(vidSurface)) lockedsf = SDL_LockSurface(vidSurface);
			if(lockedsf == 0 && vidSurface->pixels)
			{
				VID_BlitLinearScreen (screens[0], vidSurface->pixels, vid.width*vid.bpp,
				                      vid.height, vid.rowbytes, vidSurface->pitch );
				if(SDL_MUSTLOCK(vidSurface)) SDL_UnlockSurface(vidSurface);
			}
		}
		else if(bufSurface && (videoblitok || vid.bpp != 1 || vfBPP < 8) ) //Alam: New Way to send video data
		{
			blited = SDL_BlitSurface(bufSurface,NULL,vidSurface,NULL);
		}
		else if(vid.bpp == 1)
		{
			Uint8 *bP,*vP; //Src, Dst
			Uint16 bW, vW; // Pitch Remainder
			Sint32 pH, pW; //Height, Width
			bP = (Uint8 *)screens[0];
			bW = (Uint16)(vid.rowbytes - vid.width);
			if(SDL_MUSTLOCK(vidSurface)) lockedsf = SDL_LockSurface(vidSurface);
			vP = (Uint8 *)vidSurface->pixels;
			vW = (Uint16)(vidSurface->pitch - vidSurface->w*vidformat->BytesPerPixel);
			if(lockedsf == 0 && vidSurface->pixels)
			{
				if(vidformat->BytesPerPixel == 2)
				{
					for(pH=0;pH < vidSurface->h;pH++)
					{
						for(pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint16 *)vP) = (Uint16)SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 2;
						}
						bP += bW;
						vP += vW;
					}
				}
				else if(vidformat->BytesPerPixel == 3)
				{
					for(pH=0;pH < vidSurface->h;pH++)
					{
						for(pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint32 *)vP) = SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 3;
						}
						bP += bW;
						vP += vW;
					}
				}
				else if(vidformat->BytesPerPixel == 4)
				{
					for(pH=0;pH < vidSurface->h;pH++)
					{
						for(pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint32 *)vP) = SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 4;
						}
						bP += bW;
						vP += vW;
					}
				}
				else
				{
					;//NOP
				}
			}
			if(SDL_MUSTLOCK(vidSurface)) SDL_UnlockSurface(vidSurface);
		}
		else /// \todo 15t15,15tN, others?
		{
			;//NOP
		}

		if(lockedsf == 0 && blited == 0 && vidSurface->flags&SDL_DOUBLEBUF)
			SDL_Flip(vidSurface);
		else if(blited != 2 && lockedsf == 0) //Alam: -2 for Win32 Direct, yea, i know
			SDL_UpdateRect(vidSurface, 0, 0, 0, 0); //Alam: almost always
		else if(devparm)
			I_OutputMsg("%s\n",SDL_GetError());
	}
#ifdef HWRENDER
	else
	{
		OglSdlFinishUpdate(cv_vidwait.value);
	}
#endif
	exposevideo = SDL_FALSE;
}


//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
	if (rendermode != render_soft)
		I_Error ("I_ReadScreen: called while in non-software mode");
	else
		VID_BlitLinearScreen ( vid.buffer, scr,
				vid.width*vid.bpp, vid.height,
				vid.rowbytes, vid.rowbytes );
}

//
// I_SetPalette
//
void I_SetPalette(RGBA_t* palette)
{
	int i;
	for(i=0; i<256; i++)
	{
		localPalette[i].r = palette[i].s.red;
		localPalette[i].g = palette[i].s.green;
		localPalette[i].b = palette[i].s.blue;
	}
	if(vidSurface) SDL_SetColors(vidSurface, localPalette, 0, 256);
	if(bufSurface) SDL_SetColors(bufSurface, localPalette, 0, 256);
}

// return number of fullscreen + X11 modes
int VID_NumModes(void)
{
	if(USE_FULLSCREEN && numVidModes != -1)
		return numVidModes - firstEntry;
	else
		return MAXWINMODES;
}

const char *VID_GetModeName(int modeNum)
{
	if(USE_FULLSCREEN && numVidModes != -1) // fullscreen modes
	{
		modeNum += firstEntry;
		if(modeNum >= numVidModes)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
		        modeList[modeNum]->w,
		        modeList[modeNum]->h);
	}
	else // windowed modes
	{
		if(modeNum > MAXWINMODES)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
		        windowedModes[modeNum][0],
		        windowedModes[modeNum][1]);
	}
	return &vidModeName[modeNum][0];
}

int VID_GetModeForSize(int w, int h)
{
	int matchMode = -1, i;
	if(USE_FULLSCREEN && numVidModes != -1)
	{
		for(i=firstEntry; i<numVidModes; i++)
		{
			if(modeList[i]->w == w &&
				modeList[i]->h == h)
			{
				matchMode = i;
				break;
			}
		}
		if(-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for(i=firstEntry; i<numVidModes; i++)
			{
				if(modeList[i]->w == w &&
					modeList[i]->h == h)
				{
					matchMode = i;
					break;
				}
			}
			if(-1 == matchMode) // use smallest mode
				matchMode = numVidModes-1;
		}
		matchMode -= firstEntry;
	}
	else
	{
		for(i=0; i<MAXWINMODES; i++)
		{
			if(windowedModes[i][0] == w &&
				windowedModes[i][1] == h)
			{
				matchMode = i;
				break;
			}
		}
		if(-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for(i=0; i<MAXWINMODES; i++)
			{
				if(windowedModes[i][0] == w &&
					windowedModes[i][1] == h)
				{
					matchMode = i;
					break;
				}
			}
			if(-1 == matchMode) // use smallest mode
				matchMode = MAXWINMODES-1;
		}
	}
	return matchMode;
}

void VID_PrepareModeList(void)
{
	int i;

	firstEntry = 0;
	if(disable_fullscreen?0:cv_fullscreen.value) // only fullscreen needs preparation
	{
		if(-1 != numVidModes)
		{
			for(i=0; i<numVidModes; i++)
			{
				if(modeList[i]->w <= MAXVIDWIDTH &&
					modeList[i]->h <= MAXVIDHEIGHT)
				{
					firstEntry = i;
					break;
				}
			}
		}
	}
	allow_fullscreen = true;
}

static void SDLWMSet(void)
{
#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)
	SDL_SysWMinfo SDLWM;
	memset(&SDLWM,0,sizeof(SDL_SysWMinfo));
	SDL_VERSION(&SDLWM.version)
	if(SDL_GetWMInfo(&SDLWM))
		vid.WndParent = SDLWM.window;
	else
		vid.WndParent = NULL;
	if(vid.WndParent)
	{
		SetFocus(vid.WndParent);
		ShowWindow(vid.WndParent, SW_SHOW);
	}
#endif
}

int VID_SetMode(int modeNum)
{
	doUngrabMouse();
	vid.recalc = true;
	BitsPerPixel = (Uint8)cv_scr_depth.value;
	//vid.bpp = BitsPerPixel==8?1:2;
	// Window title
	SDL_WM_SetCaption("SRB2"VERSIONSTRING, "SRB2");

	if(render_soft == rendermode)
	{
		//Alam: SDL_Video system free vidSurface for me
		if(vid.buffer) free(vid.buffer);
		vid.buffer = NULL;
		if(bufSurface) SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	if(USE_FULLSCREEN)
	{
		if(numVidModes != -1)
		{
			modeNum += firstEntry;
			vid.width = modeList[modeNum]->w;
			vid.height = modeList[modeNum]->h;
		}
		else
		{
			vid.width = windowedModes[modeNum][0];
			vid.height = windowedModes[modeNum][1];
		}
		if(render_soft == rendermode)
		{
			SDL_SetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsF);

			if(!vidSurface)
			{
				cv_fullscreen.value = 0;
				modeNum = VID_GetModeForSize(vid.width,vid.height);
				vid.width = windowedModes[modeNum][0];
				vid.height = windowedModes[modeNum][1];
				SDL_SetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
				if(!vidSurface)
					I_Error("Could not set vidmode: %s\n",SDL_GetError());
			}
		}
#ifdef HWRENDER
		else // (render_soft != rendermode)
		{
			if(!OglSdlSurface(vid.width, vid.height, true))
			{
				cv_fullscreen.value = 0;
				modeNum = VID_GetModeForSize(vid.width,vid.height);
				vid.width = windowedModes[modeNum][0];
				vid.height = windowedModes[modeNum][1];
				if(!OglSdlSurface(vid.width, vid.height,false))
					I_Error("Could not set vidmode: %s\n",SDL_GetError());
			}

			realwidth = (Uint16)vid.width;
			realheight = (Uint16)vid.height;
		}
#endif
	}
	else //(cv_fullscreen.value)
	{
		vid.width = windowedModes[modeNum][0];
		vid.height = windowedModes[modeNum][1];

		if(render_soft == rendermode)
		{
			SDL_SetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
			if(!vidSurface)
				I_Error("Could not set vidmode: %s\n",SDL_GetError());
		}
#ifdef HWRENDER
		else //(render_soft != rendermode)
		{
			if(!OglSdlSurface(vid.width, vid.height, false))
				I_Error("Could not set vidmode: %s\n",SDL_GetError());
			realwidth = (Uint16)vid.width;
			realheight = (Uint16)vid.height;
		}
#endif
	}

	vid.modenum = VID_GetModeForSize(vidSurface->w,vidSurface->h);

	if(render_soft == rendermode)
	{
		vid.rowbytes = vid.width*vid.bpp;
		vid.buffer = malloc(vid.rowbytes*vid.height*NUMSCREENS);
		if(!vid.buffer) I_Error ("Not enough memory for video buffer\n");
	}

	if(!cv_stretch.value && (float)vid.width/vid.height != ((float)BASEVIDWIDTH/BASEVIDHEIGHT))
		vid.height = (int)(vid.width * ((float)BASEVIDHEIGHT/BASEVIDWIDTH));// Adjust the height to match

	I_StartupMouse();

	SDLWMSet();

	return true;
}

void I_StartupGraphics(void)
{
	char SDLNOMOUSE[] = "SDL_NOMOUSE=1";
	char SDLVIDEOMID[] = "SDL_VIDEO_CENTERED=1";

	if(graphics_started || dedicated)
		return;

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand ("vid_info", VID_Command_Info_f);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_stretch);
#ifdef FILTERS
	CV_RegisterVar (&cv_filter);
#endif
	disable_mouse = M_CheckParm("-nomouse");
	if(disable_mouse)
		putenv(SDLNOMOUSE);
	disable_fullscreen = M_CheckParm("-win");
	//if(disable_fullscreen)
		putenv(SDLVIDEOMID);


	// Initialize Audio as well, otherwise Win32's DirectX can not use audio
	if(SDL_InitSubSystem(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
	{
		CONS_Printf("Couldn't initialize SDL's Audio/Video: %s\n", SDL_GetError());
		if(SDL_WasInit(SDL_INIT_VIDEO)==0) return;
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY>>1,SDL_DEFAULT_REPEAT_INTERVAL<<2);
	SDL_JoystickEventState(SDL_IGNORE);
	VID_Command_ModeList_f();
	vid.buffer = NULL;  // For software mode
	vid.width = BASEVIDWIDTH; // Default size for startup
	vid.height = BASEVIDHEIGHT; // BitsPerPixel is the SDL interface's
	vid.recalc = true; // Set up the console stufff
	vid.direct = NULL; // Maybe direct access?
	vid.bpp = 1; // This is the game engine's Bpp
	vid.WndParent = NULL; //For the window?

	// Window title
	SDL_WM_SetCaption("SRB2: Starting up", "SRB2");

	// Window icon
#ifdef HAVE_IMAGE
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif
	SDL_WM_SetIcon(icoSurface, NULL);

//[segabor]: Mac hack
//    if(M_CheckParm("-opengl"))
#ifdef HWRENDER
	if(M_CheckParm("-opengl") || rendermode == render_opengl)
	{
		rendermode = render_opengl;
		HWD.pfnInit             = hwSym("Init",NULL);
		HWD.pfnFinishUpdate     = hwSym("FinishUpdate",NULL);
		HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
		HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
		HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
		HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
		HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
		HWD.pfnReadRect         = hwSym("ReadRect",NULL);
		HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
		HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
		HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
		HWD.pfnSetPalette       = hwSym("SetPalette",NULL);
		HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
		HWD.pfnDrawMD2          = hwSym("DrawMD2",NULL);
		HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
		HWD.pfnGetRenderVersion = hwSym("GetRenderVersion",NULL);
		// check gl renderer lib
		if (HWD.pfnGetRenderVersion() != VERSION)
			I_Error ("The version of the renderer doesn't match the version of the executable\nBe sure you have installed SRB2 properly.\n");
		vid.width = realwidth = 640; // hack to make voodoo cards work in 640x480
		vid.height = realheight = 480;
		/*
		 * We want at least 1 bit R, G, and B,
		 * and at least 16 bpp. Why 1 bit? May be more?
		 */
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		if(!OglSdlSurface(vid.width, vid.height, (USE_FULLSCREEN)))
			if(!OglSdlSurface(vid.width, vid.height, !(USE_FULLSCREEN)))
				rendermode = render_soft;
	}
#endif
#ifdef _arch_dreamcast
	conio_shutdown();
#endif
	if(render_soft == rendermode)
	{
		SDL_SetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
		if(!vidSurface)
		{
			CONS_Printf("Could not set vidmode: %s\n",SDL_GetError());
			mousegrabok = SDL_TRUE; //Alam: ahhh.
			vid.rowbytes = 0; /// \todo handle no video data at startup
			return;
		}
		vid.rowbytes = vid.width * vid.bpp;
		vid.buffer = malloc(vid.width * vid.height * vid.bpp * NUMSCREENS);
		if(!vid.buffer) CONS_Printf ("Not enough memory for video buffer\n");
	}
	VID_Command_Info_f();
	if(!disable_mouse) SDL_ShowCursor(SDL_DISABLE);
	doUngrabMouse();

	SDLWMSet();

	keyboard_started = graphics_started = true;
}

void I_ShutdownGraphics(void)
{
	// was graphics initialized anyway?
	if (!graphics_started)
		return;
	CONS_Printf("I_ShutdownGraphics: ");
	doUngrabMouse();
	if(icoSurface) SDL_FreeSurface(icoSurface);
	icoSurface = NULL;
	if(render_soft == rendermode)
	{
		vidSurface = NULL; //Alam: SDL_Video system free vidSurface for me
		if(vid.buffer) free(vid.buffer);
		vid.buffer = NULL;
		if(bufSurface) SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
#ifdef FILTERS
		if(preSurface) SDL_FreeSurface(preSurface);
		preSurface = NULL;
		if(f2xSurface) SDL_FreeSurface(f2xSurface);
		f2xSurface = NULL;
#endif
	}
#ifdef HWRENDER
	else
	{
		OglSdlShutdown();
	}
#endif
	graphics_started = false;
	CONS_Printf("shut down\n");
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
