// Emacs style mode select   -*- C++ -*-
// 
//-----------------------------------------------------------------------------
//
// $Id: i_system.c,v 1.12 2004/03/23 17:38:51 araftis Exp $
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
// $Log: i_system.c,v $
// Revision Mac  2004/03/23 17:38:51  araftis
// Added support for Mac OS X (version 10.3+). Also fixed a number of endian
// bugs that were introduced by the over zealous use of the SHORT macro. This
// version also removes support for Mac OS 9 and earlier.	
//
// Revision 1.12  2003/05/04 04:24:08  sburke
// Add Solaris support.
//
// Revision 1.11  2002/01/03 19:20:07  bock
// Add FreeBSD code to I_GetFreeMem.
// Modified Files:
//     makefile linux_x/i_system.c sdl/i_system.c
//
// Revision 1.10  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
// .
//
// Revision 1.9  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.8  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.7  2001/03/12 21:03:10  metzgermeister
//   * new symbols for rendererlib added in SDL
//   * console printout fixed for Linux&SDL
//   * Crash fixed in Linux SW renderer initialization
//
// Revision 1.6  2001/02/24 13:35:23  bpereira
// no message
//
// Revision 1.5  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.4  2000/10/16 21:20:53  hurdler
// remove unecessary code
//
// Revision 1.3  2000/09/26 17:58:06  metzgermeister
// I_Getkey implemented
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 system stuff for SDL

#ifndef _WIN32_WCE
#include <signal.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define RPC_NO_WINDOWS_H
#include <windows.h>
typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes);
typedef DWORD (WINAPI *MyFunc2) (void);
#endif	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#elif defined(_MSC)
#include <direct.h>
#endif
#ifndef _WIN32_WCE
#if !defined(SDLIO) || defined(LINUX)
#include <fcntl.h>
#endif
#endif

#ifdef _arch_dreamcast
//#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <conio/conio.h>
void __set_fpscr(long); // in libgcc / kernel's startup.s?
#elif !defined (SDLMAIN)
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#endif
#endif

#include <SDL/SDL.h>
#if (SDL_MAJOR_VERSION > 1) || (SDL_MINOR_VERSION >= 3) || (SDL_MINOR_VERSION == 2 && SDL_PATCHLEVEL >= 7)
#ifndef __APPLE_CC__
#include <SDL/SDL_cpuinfo.h> // 1.2.7 or greater
#define HAVE_SDLCPUINFO
#endif
#endif

#if defined(LINUX) && !defined(_arch_dreamcast)
#if !defined(FREEBSD) && !defined(__APPLE_CC__)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#include <kvm.h>
#include <nlist.h>
#include <sys/vmmeter.h>
#endif
#endif

#ifdef LMOUSE2
#include <termios.h>
#endif

#ifdef _WIN32_WCE
#include "SRB2CE/cehelp.h"
#endif

// Locations for searching the srb2.srb
#ifdef _arch_dreamcast
#include "SRB2DC/VMU.xbm"
#define DEFAULTWADLOCATION1 "/cdrom"
#define DEFAULTWADLOCATION2 "/pc"
#define DEFAULTSEARCHPATH1 "/cdrom"
#define DEFAULTSEARCHPATH2 "/pc"
#elif defined(LINUX) || defined(__MACH__)
#define DEFAULTWADLOCATION1 "/usr/local/games/srb2"
#define DEFAULTWADLOCATION2 "/usr/games/srb2"
#define DEFAULTSEARCHPATH1 "/usr/local"
#define DEFAULTSEARCHPATH2 "/usr/games"
#elif defined(_WIN32)  || defined(_WIN64)
#define DEFAULTWADLOCATION1 "c:\\games\\srb2"
#define DEFAULTWADLOCATION2 "\\games\\srb2"
#define DEFAULTSEARCHPATH1 "c:\\games"
#define DEFAULTSEARCHPATH2 "\\games"
#endif

/**	\brief WAD file to look for
*/
#define WADKEYWORD1 "srb2.srb"
#define WADKEYWORD2 "srb2.wad"
/**	\brief holds wad path
*/
static char returnWadPath[256];

//Alam_GBC: SDL

#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../screen.h" //vid.WndParent
#include "../d_net.h"
#include "../g_game.h"

#include "endtxt.h"

#include "../i_joy.h"

#include "../m_argv.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

/**	\brief	The JoyInfo_s struct

  info about joystick
*/
struct SDLJoyInfo_s
{
	/// Joystick handle
	SDL_Joystick *dev;
	/// number of old joystick
	int oldjoy;
	/// number of axies
	int axises;
	/// scale of axises
	int scale;
	/// number of buttons
	int buttons;
	/// number of hats
	int hats;
	
};
typedef struct SDLJoyInfo_s SDLJoyInfo_t;

/**	\brief	The JoyReset function

	\param	JoySet	Joystick info to reset

	\return	void

	
*/
static void JoyReset(SDLJoyInfo_t *JoySet)
{
	if(JoySet->dev) SDL_JoystickClose(JoySet->dev);
	JoySet->dev = NULL;
	JoySet->oldjoy = -1;
	JoySet->axises = JoySet->buttons = JoySet->hats = 0;
	//JoySet->scale
}

/**	\brief First joystick up and running
*/
static int joystick_started  = 0;

/**	\brief SDL info about joystick 1
*/
static SDLJoyInfo_t JoyInfo;

/**	\brief joystick axis deadzone
*/
#define DEADZONE 153

/**	\brief Second joystick up and running
*/
static int joystick2_started = 0;

/**	\brief SDL inof about joystick 2
*/
static SDLJoyInfo_t JoyInfo2;

#ifdef LMOUSE2
static int fdmouse2 = -1;
static int mouse2_started = 0;
#endif


/// \brief max number of joystick buttons
#define JOYBUTTONS_MIN JOYBUTTONS
/// \brief max number of joystick button events
#define JOYBUTTONS_MAX JOYBUTTONS
/// \brief max number of joystick axies
#define JOYAXISES_MIN  JOYAXISSET
/// \brief max number ofjoystick axis events
#define JOYAXISES_MAX  JOYAXISSET
/// \brief max number of joystick hats
#define JOYHATS_MIN    JOYHATS
/// \brief max number of joystick hat events
#define JOYHATS_MAX    JOYHATS

byte keyboard_started = false;

#if 0
static void signal_handler(int num)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	char*       sigmsg;
	char        sigdef[64];

	switch (num)
	{
	case SIGINT:
		sigmsg = "interrupt";
		break;
	case SIGILL:
		sigmsg = "illegal instruction - invalid function image";
		break;
	case SIGFPE:
		sigmsg = "floating point exception";
		break;
	case SIGSEGV:
		sigmsg = "segment violation";
		break;
	case SIGTERM:
		sigmsg = "Software termination signal from kill";
		break;
#if  !defined(LINUX)
	case SIGBREAK:
		sigmsg = "Ctrl-Break sequence";
		break;
#endif
	case SIGABRT:
		sigmsg = "abnormal termination triggered by abort call";
		break;
	default:
		sprintf(sigdef,"signal number %d", num);
		sigmsg = sigdef;
	}
	
#ifdef LOGMESSAGES
	if (logstream != INVALID_HANDLE_VALUE)
	{
		I_OutputMsg ("signal_handler() error: %s\n", sigmsg);
	}
#endif
	signal(num, SIG_DFL);               //default signal action
	raise(num);
	I_Quit();
}
#endif


//
// StartupKeyboard
//
void I_StartupKeyboard (void)
{
#ifdef NDEBUG
#ifdef SIGILL
//	signal(SIGILL , signal_handler);
#endif
#ifdef SIGINT
	signal(SIGINT , (void (*)(int)) I_Quit);
#endif
#ifdef SIGSEGV
//	signal(SIGSEGV , signal_handler);
#endif
#ifdef SIGBREAK
	signal(SIGBREAK , (void (*)(int)) I_Quit);
#endif
#ifdef SIGABRT
//	signal(SIGABRT , signal_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM , (void (*)(int)) I_Quit);
#endif
#endif
}

//
//I_OutputMsg
//
void I_OutputMsg(const char *fmt, ...)
{
#ifdef _arch_dreamcast
	conio_printf(fmt);
#else
	va_list  argptr;
	va_start(argptr,fmt);
	vfprintf(stderr,fmt,argptr);
	va_end(argptr);
	// 2004-03-03 AJR Since not all messages end in newline, some were getting displayed late.	
	fflush(stderr);
#endif
}

int I_ConsoleKey()
{
	//defined(__MACH__)?
#if defined(LINUX) && !defined (SDLMAIN)
	int key;
	key = getchar();
	if(key == '\n')
		return 0;
	return key;
#elif (defined (_WIN32) || defined(_WIN64)) && !defined (SDLMAIN)
	int key = getch();
	if(key == 0)
		key = getch() + 256;
	return key;
#else
	return 0;
#endif
}

//
// I_GetKey
//
int I_GetKey (void)
{
	// Warning: I_GetKey empties the event queue till next keypress
	event_t* ev;
	int rc = 0;

	if(!graphics_started)
	{
		rc = I_ConsoleKey();
		if(rc)
			return rc;
		else
			return KEY_ENTER;
	}

	// return the first keypress from the event queue
	for(; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
	{
		ev = &events[eventtail];
		if(ev->type == ev_keydown)
		{
			rc = ev->data1;
			continue;
		}
	}

	return rc;
}

//
// I_JoyScale
//
void I_JoyScale()
{
	Joystick.bGamepadStyle = !cv_joyscale.value;
	JoyInfo.scale = (Joystick.bGamepadStyle)?1:cv_joyscale.value;
}

void I_JoyScale2()
{
	Joystick2.bGamepadStyle = !cv_joyscale2.value;
	JoyInfo2.scale = (Joystick2.bGamepadStyle)?1:cv_joyscale2.value;
}

/**	\brief Joystick 1 buttons states
*/
static INT64 lastjoybuttons = 0;

/**	\brief Joystick 1 hats state
*/
static INT64 lastjoyhats = 0;

/**	\brief	Shuts down joystick 1


	\return void

	
*/
static void I_ShutdownJoystick()
{
	int i;
	event_t event;
	event.type=ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoybuttons = lastjoyhats = 0;

	// emulate the up of all joystick buttons
	for(i=0;i<JOYBUTTONS;i++)
	{
		event.data1=KEY_JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for(i=0;i<JOYHATS*4;i++)
	{
		event.data1=KEY_HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick;
	for(i=0;i<JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	joystick_started = 0;
	JoyReset(&JoyInfo);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK)!=0)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if(cv_usejoystick.value==0) CONS_Printf("I_Joystick: SDL's Joystick system has been shutdown\n");
	}
}

void I_GetJoystickEvents(void)
{
	static event_t event = {0,0,0,0};
	int i = 0;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	int axisx, axisy;

	if(!joystick_started) return;

	if (!JoyInfo.dev) I_ShutdownJoystick();

	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo.dev,i))
			joybuttons |= 1;
	}

	for (i = JoyInfo.hats - 1; i >= 0; i--)
	{
		int hat = SDL_JoystickGetHat(JoyInfo.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=1<<(3 + 4*i);
	}

	if ( joybuttons != lastjoybuttons )
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for( i=0; i < JOYBUTTONS; i++, j<<=1 )
		{
			if ( newbuttons & j ) // button changed state ?
			{
				if ( joybuttons & j )
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_JOY1 + i;
				D_PostEvent (&event);
			}
		}
	}

	if ( joyhats != lastjoyhats )
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for( i=0; i < JOYHATS*4; i++, j<<=1 )
		{
			if ( newhats & j ) // hat changed state ?
			{
				if ( joyhats & j )
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_HAT1 + i;
				D_PostEvent (&event);
			}
		}
	}

	// send joystick axis positions
	event.type = ev_joystick;

	for(i = JOYAXISSET - 1; i >=0;i--)
	{
		event.data1 = i;
		if(i*2 + 1 <= JoyInfo.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 0);
		else axisx = 0;
		if(i*2 + 2 <= JoyInfo.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 1);
		else axisy = 0;

		axisx = axisx/32;
		axisy = axisy/32;

		if ( Joystick.bGamepadStyle )
		{
			// gamepad control type, on or off, live or die
			if ( axisx < -(JOYAXISRANGE/2) )
				event.data2 = -1;
			else if ( axisx > (JOYAXISRANGE/2) )
				event.data2 = 1;
			else event.data2 = 0;
			if ( axisy < -(JOYAXISRANGE/2) )
				event.data3 = -1;
			else if ( axisy > (JOYAXISRANGE/2) )
				event.data3 = 1;
			else event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo.scale?((axisx/JoyInfo.scale)*JoyInfo.scale):axisx;
			axisy = JoyInfo.scale?((axisy/JoyInfo.scale)*JoyInfo.scale):axisy;

			if(-DEADZONE <= axisx && axisx <= DEADZONE) axisx=0;
			if(-DEADZONE <= axisy && axisy <= DEADZONE) axisy=0;

			// analog control style , just send the raw data
			event.data2 = axisx;	// x axis
			event.data3 = axisy;	// y axis
		}
		D_PostEvent (&event);
	}
}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises

	
*/
static int joy_open(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if(joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf("Couldn't initialize SDL Joystick: %s\n", SDL_GetError());
			return 0;
		}
		else
		{
			num_joy = SDL_NumJoysticks();
		}

		if(num_joy < joyindex)
		{
			CONS_Printf("Unable to use that joystick #%d/(%s), it doesn't exist\n",joyindex,fname);
			for(i=0;i<num_joy;i++)
				CONS_Printf("#: %d, Name: %s\n", i, SDL_JoystickName(i));
			I_ShutdownJoystick();
			return 0;
		}
	}
	else
	{
		JoyReset(&JoyInfo);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}
	
	num_joy = SDL_NumJoysticks();

	if(joyindex <= 0 || num_joy == 0 || JoyInfo.oldjoy == joyindex)
	{
//		CONS_Printf("Unable to use that joystick #(%s), non-number\n",fname);
		if(num_joy != 0)
		{
			CONS_Printf("Hmmm, I was able to found %d joysticks on this system\n", num_joy);
			for(i=0;i<num_joy;i++)
				CONS_Printf("#: %d, Name: %s\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("Hmm, I was unable to found any joysticks on this system\n");
		if(joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf("Joystick: %s\n",SDL_JoystickName(joyindex-1));

	if(JoyInfo.dev == NULL)
	{
		CONS_Printf("Couldn't open joystick: %s\n", SDL_GetError());
		I_ShutdownJoystick();
		return 0;
	}
	else
	{
		JoyInfo.axises = SDL_JoystickNumAxes(JoyInfo.dev);
		if(JoyInfo.axises > JOYAXISSET*2)
			JoyInfo.axises = JOYAXISSET*2;
/*		if(joyaxes<2)
		{
			CONS_Printf("Not enought axes?\n");
			I_ShutdownJoystick();
			return 0;
		}*/

		JoyInfo.buttons = SDL_JoystickNumButtons(JoyInfo.dev);
		if(JoyInfo.buttons > JOYBUTTONS)
			JoyInfo.buttons = JOYBUTTONS;

		JoyInfo.hats = SDL_JoystickNumHats(JoyInfo.dev);
		if(JoyInfo.hats > JOYHATS)
			JoyInfo.hats = JOYHATS;

		//Joystick.bGamepadStyle = !strcmp ( SDL_JoystickName (SDL_JoystickIndex(JoyInfo.dev) ),"Pad");

		return JoyInfo.axises;
	}
}

//Joystick2

/**	\brief Joystick 2 buttons states
*/
static INT64 lastjoy2buttons = 0;

/**	\brief Joystick 2 hats state
*/
static INT64 lastjoy2hats = 0;

/**	\brief	Shuts down joystick 2


	\return	void

	
*/
static void I_ShutdownJoystick2()
{
	int i;
	event_t event;
	event.type=ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoy2buttons = lastjoy2hats = 0;

	// emulate the up of all joystick buttons
	for(i=0;i<JOYBUTTONS;i++)
	{
		event.data1=KEY_2JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for(i=0;i<JOYHATS*4;i++)
	{
		event.data1=KEY_2HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick2;
	for(i=0;i<JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	JoyReset(&JoyInfo2);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK)!=0)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if(cv_usejoystick2.value==0) CONS_Printf("I_Joystick2: SDL's Joystick system has been shutdown\n");
	}
}

void I_GetJoystick2Events(void)
{
	static event_t event = {0,0,0,0};
	int i = 0;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	int axisx, axisy;

	if(!joystick2_started) return;

	if (!JoyInfo2.dev) I_ShutdownJoystick2();

	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo2.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo2.dev,i))
			joybuttons |= 1;
	}

	for (i = JoyInfo2.hats - 1; i >= 0; i--)
	{
		int hat = SDL_JoystickGetHat(JoyInfo2.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=1<<(3 + 4*i);
	}

	if ( joybuttons != lastjoy2buttons )
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for( i=0; i < JOYBUTTONS; i++, j<<=1 )
		{
			if ( newbuttons & j ) // button changed state ?
			{
				if ( joybuttons & j )
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2JOY1 + i;
				D_PostEvent (&event);
			}
		}
	}

	if ( joyhats != lastjoy2hats )
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for( i=0; i < JOYHATS*4; i++, j<<=1 )
		{
			if ( newhats & j ) // hat changed state ?
			{
				if ( joyhats & j )
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2HAT1 + i;
				D_PostEvent (&event);
			}
		}
	}

	// send joystick axis positions
	event.type = ev_joystick2;

	for(i = JOYAXISSET - 1; i >=0;i--)
	{
		event.data1 = i;
		if(i*2 + 1 <= JoyInfo2.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 0);
		else axisx = 0;
		if(i*2 + 2 <= JoyInfo2.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 1);
		else axisy = 0;

		axisx = axisx/32;
		axisy = axisy/32;

		if ( Joystick2.bGamepadStyle )
		{
			// gamepad control type, on or off, live or die
			if ( axisx < -(JOYAXISRANGE/2) )
				event.data2 = -1;
			else if ( axisx > (JOYAXISRANGE/2) )
				event.data2 = 1;
			else
				event.data2 = 0;
			if ( axisy < -(JOYAXISRANGE/2) )
				event.data3 = -1;
			else if ( axisy > (JOYAXISRANGE/2) )
				event.data3 = 1;
			else
				event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo2.scale?((axisx/JoyInfo2.scale)*JoyInfo2.scale):axisx;
			axisy = JoyInfo2.scale?((axisy/JoyInfo2.scale)*JoyInfo2.scale):axisy;

			if(-DEADZONE <= axisx && axisx <= DEADZONE) axisx=0;
			if(-DEADZONE <= axisy && axisy <= DEADZONE) axisy=0;

			// analog control style , just send the raw data
			event.data2 = axisx;	// x axis
			event.data3 = axisy;	// y axis
		}
		D_PostEvent (&event);
	}

}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises

	
*/
static int joy_open2(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if(joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf("Couldn't initialize SDL Joystick: %s\n", SDL_GetError());
			return 0;
		}
		else
		{
			num_joy = SDL_NumJoysticks();
		}

		if(num_joy < joyindex)
		{
			CONS_Printf("Unable to use that joystick #%d/(%s), it doesn't exist\n",joyindex,fname);
			for(i=0;i<num_joy;i++)
				CONS_Printf("#: %d, Name: %s\n", i, SDL_JoystickName(i));
			I_ShutdownJoystick2();
			return 0;
		}
	}
	else
	{
		JoyReset(&JoyInfo2);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}

	num_joy = SDL_NumJoysticks();

	if(joyindex <= 0 || num_joy == 0 || JoyInfo2.oldjoy == joyindex)
	{
//		CONS_Printf("Unable to use that joystick #(%s), non-number\n",fname);
		if(num_joy != 0)
		{
			CONS_Printf("Hmmm, I was able to found %d joysticks on this system\n", num_joy);
			for(i=0;i<num_joy;i++)
				CONS_Printf("#: %d, Name: %s\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("Hmm, I was unable to found any joysticks on this system\n");
		if(joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo2.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf("Joystick2: %s\n",SDL_JoystickName(joyindex-1));

	if(!JoyInfo2.dev)
	{
		CONS_Printf("Couldn't open joystick2: %s\n", SDL_GetError());
		I_ShutdownJoystick2();
		return 0;
	}
	else
	{
		JoyInfo2.axises = SDL_JoystickNumAxes(JoyInfo2.dev);
		if(JoyInfo2.axises > JOYAXISSET*2)
			JoyInfo2.axises = JOYAXISSET*2;
/*		if(joyaxes<2)
		{
			CONS_Printf("Not enought axes?\n");
			I_ShutdownJoystick2();
			return 0;
		}*/

		JoyInfo2.buttons = SDL_JoystickNumButtons(JoyInfo2.dev);
		if(JoyInfo2.buttons > JOYBUTTONS)
			JoyInfo2.buttons = JOYBUTTONS;

		JoyInfo2.hats = SDL_JoystickNumHats(JoyInfo2.dev);
		if(JoyInfo2.hats > JOYHATS)
			JoyInfo2.hats = JOYHATS;

		//Joystick.bGamepadStyle = !strcmp ( SDL_JoystickName (SDL_JoystickIndex(JoyInfo2.dev) ),"Pad");

		return JoyInfo2.axises;
	}
}

//
// I_InitJoystick
//
void I_InitJoystick (void)
{
	I_ShutdownJoystick();
	if(!strcmp(cv_usejoystick.string,"0") || M_CheckParm("-nojoy"))
		return;
	if(joy_open(cv_usejoystick.string))
		JoyInfo.oldjoy = atoi(cv_usejoystick.string);
	else
	{
		cv_usejoystick.value = 0;
		return;
	}
	joystick_started = 1;
}

void I_InitJoystick2 (void)
{
	I_ShutdownJoystick2();
	if(!strcmp(cv_usejoystick2.string,"0") || M_CheckParm("-nojoy"))
		return;
	if(joy_open2(cv_usejoystick2.string))
		JoyInfo2.oldjoy = atoi(cv_usejoystick2.string);
	else
	{
		cv_usejoystick2.value = 0;
		return;
	}
	joystick2_started = 1;
}

int I_NumJoys(void)
{
	int numjoy = 0;
	if(SDL_WasInit(SDL_INIT_JOYSTICK)==0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			numjoy = SDL_NumJoysticks();
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		numjoy = SDL_NumJoysticks();
	return numjoy;
}

const char *I_GetJoyName(int joyindex)
{
	const char *joyname = "NA";
	joyindex--; //SDL 's Joystick System starts at 0, not 1
	if(SDL_WasInit(SDL_INIT_JOYSTICK)==0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			joyname = SDL_JoystickName(joyindex);
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		joyname = SDL_JoystickName(joyindex);
	return joyname;
}

#ifdef LMOUSE2

void I_GetMouseEvents(void)
{
	static unsigned char mdata[5];
	static int i = 0,om2b = 0;
	int di,j,mlp,button;
	event_t event;
	const int mswap[8] = {0,4,1,5,2,6,3,7};
	if(!mouse2_started) return;
	for(mlp=0;mlp<20;mlp++)
	{
		for(;i<5;i++)
		{
			di = read(fdmouse2,mdata+i,1);
			if(di==-1) return;
		}
		if((mdata[0]&0xf8)!=0x80)
		{
			for(j=1;j<5;j++)
				if((mdata[j]&0xf8)==0x80)
					for(i=0;i<5-j;i++) // shift
						mdata[i] = mdata[i+j];
			if(i<5) continue;
		}
		else
		{
			button = mswap[~mdata[0]&0x07];
			for(j=0;j<MOUSEBUTTONS;j++)
			{
				if(om2b&(1<<j))
				{
					if(!(button&(1<<j))) //keyup
					{
						event.type = ev_keyup;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
				else
				{
					if(button&(1<<j))
					{
						event.type = ev_keydown;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
			}
			event.data2 = ((signed char)mdata[1])+((signed char)mdata[3]);
			event.data3 = ((signed char)mdata[2])+((signed char)mdata[4]);
			if(event.data2&&event.data3)
			{
				event.type = ev_mouse2;
				event.data1 = 0;
				D_PostEvent(&event);
			}
		}
		i = 0;
	}
}

//
// I_ShutdownMouse2
//
void I_ShutdownMouse2()
{
	if(fdmouse2!=-1) close(fdmouse2);
	mouse2_started = 0;
}
#elif defined (_WIN32) || defined(_WIN64)

static HANDLE mouse2filehandle = (HANDLE)(-1);

static void I_ShutdownMouse2 (void)
{
	event_t event;
	int i;

	if(mouse2filehandle == (HANDLE)(-1))
		return;

	SetCommMask( mouse2filehandle, 0 ) ;

	EscapeCommFunction( mouse2filehandle, CLRDTR ) ;
	EscapeCommFunction( mouse2filehandle, CLRRTS ) ;

	PurgeComm( mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT |
								 PURGE_TXCLEAR | PURGE_RXCLEAR ) ;


	CloseHandle(mouse2filehandle);

	// emulate the up of all mouse buttons
	for(i=0;i<MOUSEBUTTONS;i++)
	{
		event.type=ev_keyup;
		event.data1=KEY_2MOUSE1+i;
		D_PostEvent(&event);
	}

	mouse2filehandle = (HANDLE)(-1);
}

#define MOUSECOMBUFFERSIZE 256
static int handlermouse2x,handlermouse2y,handlermouse2buttons;

static void I_PoolMouse2(void)
{
	byte buffer[MOUSECOMBUFFERSIZE];
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	char       dx,dy;

	static int     bytenum;
	static byte    combytes[4];
	DWORD      i;

	ClearCommError( mouse2filehandle, &dwErrorFlags, &ComStat ) ;
	dwLength = min( MOUSECOMBUFFERSIZE, ComStat.cbInQue ) ;

	if (dwLength > 0)
	{
		if(!ReadFile( mouse2filehandle, buffer, dwLength, &dwLength, NULL ))
		{
			CONS_Printf("\2Read Error on secondary mouse port\n");
			return;
		}

		// parse the mouse packets
		for(i=0;i<dwLength;i++)
		{
			if((buffer[i] & 64)== 64)
				bytenum = 0;
			
			if(bytenum<4)
				combytes[bytenum]=buffer[i];
			bytenum++;

			if(bytenum==1)
			{
				handlermouse2buttons &= ~3;
				handlermouse2buttons |= ((combytes[0] & (32+16)) >>4);
			}
			else
			if(bytenum==3)
			{
				dx = (char)((combytes[0] &  3) << 6);
				dy = (char)((combytes[0] & 12) << 4);
				dx = (char)(dx + combytes[1]);
				dy = (char)(dy + combytes[2]);
				handlermouse2x+= dx;
				handlermouse2y+= dy;
			}
			else
				if(bytenum==4) // fourth byte (logitech mouses)
				{
					if(buffer[i] & 32)
						handlermouse2buttons |= 4;
					else
						handlermouse2buttons &= ~4;
				}
		}
	}
}

void I_GetMouseEvents(void)
{
	static byte lastbuttons2=0; //mouse movement
	event_t         event;

	if(mouse2filehandle == (HANDLE)(-1))
		return;

	I_PoolMouse2();
	// post key event for buttons
	if (handlermouse2buttons!=lastbuttons2)
	{
		int i,j=1,k;
		k=(handlermouse2buttons ^ lastbuttons2); // only changed bit to 1
		lastbuttons2=(byte)handlermouse2buttons;

		for(i=0;i<MOUSEBUTTONS;i++,j<<=1)
			if(k & j)
			{
				if(handlermouse2buttons & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_2MOUSE1+i;
				D_PostEvent(&event);
			}
	}

	if ((handlermouse2x!=0)||(handlermouse2y!=0))
	{
		event.type=ev_mouse2;
		event.data1=0;
//		event.data1=buttons;    // not needed
		event.data2=handlermouse2x<<1;
		event.data3=-handlermouse2y<<1;
		handlermouse2x=0;
		handlermouse2y=0;

		D_PostEvent(&event);
	}
}
#endif

//
// I_StartupMouse2
// 
void I_StartupMouse2 (void)
{
#ifdef LMOUSE2
	struct termios m2tio;
	int i,dtr,rts;
	I_ShutdownMouse2();
	if(cv_usemouse2.value == 0) return;
	if((fdmouse2 = open(cv_mouse2port.string,O_RDONLY|O_NONBLOCK|O_NOCTTY))==-1)
	{
		CONS_Printf("Error opening %s!\n",cv_mouse2port.string);
		return;
	}
	tcflush(fdmouse2, TCIOFLUSH);
	m2tio.c_iflag = IGNBRK;
	m2tio.c_oflag = 0;
	m2tio.c_cflag = CREAD|CLOCAL|HUPCL|CS8|CSTOPB|B1200;
	m2tio.c_lflag = 0;
	m2tio.c_cc[VTIME] = 0;
	m2tio.c_cc[VMIN] = 1;
	tcsetattr(fdmouse2, TCSANOW, &m2tio);
	strupr(cv_mouse2opt.string);
	for(i=0,rts = dtr = -1;i<strlen(cv_mouse2opt.string);i++)
	{
		if(cv_mouse2opt.string[i]=='D')
		{
			if(cv_mouse2opt.string[i+1]=='-')
				dtr = 0;
			else
				dtr = 1;
		}
		if(cv_mouse2opt.string[i]=='R')
		{
			if(cv_mouse2opt.string[i+1]=='-')
				rts = 0;
			else
				rts = 1;
		}
		if((dtr!=-1)||(rts!=-1))
		{
			if(!ioctl(fdmouse2, TIOCMGET, &i))
			{
				if(!dtr)
					i &= ~TIOCM_DTR;
				else if(dtr>0)
					i |= TIOCM_DTR;
			}
			if(!rts)
				i &= ~TIOCM_RTS;
			else if(rts>0)
				i |= TIOCM_RTS;
			ioctl(fdmouse2, TIOCMSET, &i);
		}
	}
	mouse2_started = 1;
	I_AddExitFunc (I_ShutdownMouse2);
#elif defined(_WIN32) || defined(_WIN64)
	DCB        dcb ;

	if(mouse2filehandle != (HANDLE)(-1))
		I_ShutdownMouse2();

	if(cv_usemouse2.value==0)
		return;

	if(mouse2filehandle == (HANDLE)(-1))
	{
		// COM file handle
		mouse2filehandle = CreateFileA( cv_mouse2port.string, GENERIC_READ | GENERIC_WRITE,
		                               0,                     // exclusive access
		                               NULL,                  // no security attrs
		                               OPEN_EXISTING,
		                               FILE_ATTRIBUTE_NORMAL, 
		                               NULL );
		if( mouse2filehandle == (HANDLE)(-1) )
		{
			int e=GetLastError();
			if( e==5 )
				CONS_Printf("\2Can't open %s : Access denied\n"
				            "The port is probably already used by one other device (mouse, modem,...)\n",cv_mouse2port.string);
			else
				CONS_Printf("\2Can't open %s : error %d\n",cv_mouse2port.string,e);
			return;
		}
	}

	// getevent when somthing happens
	//SetCommMask( mouse2filehandle, EV_RXCHAR ) ;
	
	// buffers
	SetupComm( mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE ) ;
	
	// purge buffers
	PurgeComm( mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT |
	          PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// setup port to 1200 7N1
	dcb.DCBlength = sizeof( DCB ) ;

	GetCommState( mouse2filehandle, &dcb ) ;

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY ;
	dcb.StopBits = ONESTOPBIT ;

	dcb.fDtrControl = DTR_CONTROL_ENABLE ;
	dcb.fRtsControl = RTS_CONTROL_ENABLE ;

	dcb.fBinary = TRUE ;
	dcb.fParity = TRUE ;

	SetCommState( mouse2filehandle, &dcb ) ;
	I_AddExitFunc (I_ShutdownMouse2);
#endif
}

//
// I_Tactile
//
void I_Tactile(FFType FFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	FFType = EvilForce;
	FFEffect = NULL;
}

void I_Tactile2(FFType FFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	FFType = EvilForce;
	FFEffect = NULL;
}

/**	\brief empty ticcmd for player 1
*/
static ticcmd_t emptycmd;

ticcmd_t* I_BaseTiccmd(void)
{
	return &emptycmd;
}

/**	\brief empty ticcmd for player 2
*/
static ticcmd_t emptycmd2;

ticcmd_t* I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

#if (defined(_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
static DWORD starttickcount = 0; // hack for win2k time bug
static MyFunc2 pfntimeGetTime = NULL;

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windows NT
// ---------

tic_t I_GetTime(void)
{
	tic_t newtics = 0;

	if(!starttickcount) // high precision timer
	{
		LARGE_INTEGER currtime; // use only LowPart if high resolution counter is not available
		static LARGE_INTEGER basetime = {{0, 0}};

		// use this if High Resolution timer is found
		static LARGE_INTEGER frequency;

		if(!basetime.LowPart)
		{
			if(!QueryPerformanceFrequency(&frequency))
				frequency.QuadPart = 0;
			else
				QueryPerformanceCounter(&basetime);
		}

		if(frequency.LowPart && QueryPerformanceCounter(&currtime))
		{
			newtics = (int)((currtime.QuadPart - basetime.QuadPart) * TICRATE
				/ frequency.QuadPart);
		}
		else if (pfntimeGetTime)
		{
			currtime.LowPart = pfntimeGetTime();
			if(!basetime.LowPart)
				basetime.LowPart = currtime.LowPart;
			newtics = ((currtime.LowPart - basetime.LowPart)/(1000/TICRATE));
		}
	}
	else
		newtics = (GetTickCount() - starttickcount)/(1000/TICRATE);

	return newtics;
}
#else
//
// I_GetTime
// returns time in 1/TICRATE second tics
//
static const double divticks = TICRATE/1000.0;

tic_t I_GetTime (void)
{
	       Uint32 ticks;
	static Uint32 basetime=0;

	// milliseconds since SDL initialization
	ticks = SDL_GetTicks();

	if (!basetime)
		basetime = ticks;

	ticks -= basetime;

	ticks  = (Uint32)(ticks * divticks);

	return ticks;
}
#endif

//
//I_StartupTimer
//
void I_StartupTimer    (void)
{
#if (defined(_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
	// for win2k time bug
	if(M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("Using GetTickCount()\n");
	}
	{
		HINSTANCE h = LoadLibraryA("winmm.dll");
		if(h)
		{
			pfntimeGetTime = (MyFunc2)GetProcAddress(h,"timeGetTime");
			FreeLibrary(h);
		}
	}
#else
	if(SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
		I_Error("SRB2: Needs SDL_Timer!, Error: %s",SDL_GetError()); //Alam: Doh!
#endif
}



void I_Sleep(void)
{
	if(cv_sleep.value != -1)
		SDL_Delay(cv_sleep.value);
}

//
// I_Init
//
#if 0
void I_Init (void)
{
	char title[30];

	I_StartupSound();
	I_InitMusic();

	sprintf(title, "SRB2 %s", VERSIONSTRING);
	SDL_WM_SetCaption(title, "SRB2"); // Window title
}
#endif

//
//
//

int I_StartupSystem (void)
{
	if(SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
		I_Error("SRB2: SDL System Error: %s",SDL_GetError()); //Alam: Oh no....
#ifdef _arch_dreamcast
	char title[] = "SRB2 for Dreamcast!\n";
	vmu_set_icon(VMU_bits);
	__set_fpscr(0x00040000); /* ignore FPU underflow */
	//printf("\nHello world!\n\n");
	conio_init(CONIO_TTY_PVR,CONIO_INPUT_NONE);
	conio_set_theme(CONIO_THEME_MATRIX);
	conio_clear();
	conio_putstr(title);
	//printf("\nHello world!\n\n");
#endif
	return 0;
}


//
// I_Quit
//
void I_Quit (void)
{
	static int quiting=0;
	/* prevent recursive I_Quit() */
	if(quiting) exit(1);
	quiting = 1;
	M_SaveConfig (NULL);   //save game config, cvars..
	G_SaveGameData(); // Tails 12-08-2002
	//added:16-02-98: when recording a demo, should exit using 'q' key,
	//        but sometimes we forget and use 'F10'.. so save here too.
	if (demorecording)
		G_CheckDemoStatus();
	D_QuitNetGame ();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
	SDL_Quit();
	I_ShutdownSystem();
	/* if option -noendtxt is set, don't print the text */
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM")!=-1)
	{
		printf("\r");
		ShowEndTxt();
	}
	exit(0);
}

void I_WaitVBL(int count)
{
	count = 1;
	SDL_Delay(count);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte* I_AllocLow(int length)
{
	byte* mem;

	mem = (byte *)malloc (length);
	memset (mem,0,length);
	return mem;
}

//
// I_Error
//
/**	\brief phuck recursive errors
*/
static int     errorcount = 0;

/**	\brief recursive error detecting
*/
static boolean shutdowning = false;

void I_Error (const char *error, ...)
{
	va_list        argptr;
#if defined(MAC_ALERT) || defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)
	char    buffer[8192];
#endif

	// added 11-2-98 recursive error detecting
	if(shutdowning)
	{
		errorcount++;
		// try to shutdown each subsystem separately
		if(errorcount == 2)
			I_ShutdownMusic();
		if(errorcount == 3)
			I_ShutdownSound();
		if(errorcount == 4)
			I_ShutdownCD();
		if(errorcount == 5)
			I_ShutdownGraphics();
		if(errorcount == 6)
			SDL_Quit();
		if(errorcount == 7)
			I_ShutdownSystem();
		if(errorcount == 8)
		{
			M_SaveConfig(NULL);
			G_SaveGameData();
		}
		if(errorcount > 20)
		{
#ifdef MAC_ALERT
			va_start(argptr, error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
			// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
			MacShowAlert("Recursive Error", buffer, "Quit", NULL, NULL);
#elif defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)
			va_start(argptr,error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
			MessageBoxA(vid.WndParent, buffer, "SRB2 Recursive Error", MB_OK|MB_ICONERROR);
#else
			// Don't print garbage
			va_start(argptr,error);
			vfprintf (stderr,error,argptr);
			va_end(argptr);
#endif
			exit(-1); // recursive errors detected
		}
	}
	shutdowning = true;


#ifndef MAC_ALERT
	// Message first.
	va_start (argptr,error);
	fprintf (stderr, "Error: ");
	vfprintf (stderr,error,argptr);
	fprintf (stderr, "\n");
	va_end (argptr);

	fflush( stderr );
#endif
	M_SaveConfig (NULL);   //save game config, cvars..
	G_SaveGameData(); // Tails 12-08-2002

	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame ();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
	SDL_Quit();
	I_ShutdownSystem();
#ifdef MAC_ALERT
	va_start(argptr, error);
	vsprintf(buffer, error, argptr);
	va_end(argptr);
	// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
	MacShowAlert("Critical Error", buffer, "Quit", NULL, NULL);
#endif
#if defined (PARANOIA) && defined(__CYGWIN__)
		*(int *)2 = 4; //Alam: Debug!
#endif
	exit(-1);
}

/**	\brief quit function table
*/
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
				{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
				};
//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	int c;

	for (c=0; c<MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}


//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	int c;

	for (c=0; c<MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c<MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

//
//  Closes down everything. This includes restoring the initial
//  pallete and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem()
{
	int c;

	for (c=MAX_QUIT_FUNCS-1; c>=0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
#ifdef SDLIO
if(logstream) SDL_RWclose(logstream);
#endif

}

void I_GetDiskFreeSpace(INT64 *freespace)
{
#if defined (LINUX) && !defined(_arch_dreamcast)
#ifdef SOLARIS
	*freespace = MAXINT;
	return;
#else
	struct statfs stfs;
	if(statfs(".",&stfs)==-1)
	{
		*freespace = MAXINT;
		return;
	}
	*freespace = stfs.f_bavail*stfs.f_bsize;
#endif
#elif (defined (_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)

	static MyFunc pfnGetDiskFreeSpaceEx=NULL;
	static boolean testwin95 = false;

	INT64 usedbytes;

	if(!testwin95)
	{
		HINSTANCE h = LoadLibraryA("kernel32.dll");

		if(h)
		{
			pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h,"GetDiskFreeSpaceExA");
			FreeLibrary(h);
		}
		testwin95 = true;
	}
	if (pfnGetDiskFreeSpaceEx)
	{
		if (!pfnGetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)freespace,(PULARGE_INTEGER)&usedbytes,NULL))
			*freespace = MAXINT;
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
							&NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
	}
#elif defined(__DC__)
	*freespace = 16*1024*1024;
#else // Dummy for platform independent; 1GB should be enough
	*freespace = 1024*1024*1024;
#endif
}

char *I_GetUserName(void)
{
#ifndef _WIN32_WCE
	static char username[MAXPLAYERNAME];
	char  *p;
#if defined(_WIN32) || defined(_WIN64)
	DWORD i = MAXPLAYERNAME;

	if (!GetUserName(username, &i))
#endif
	{
		p = getenv("USER");
		if(!p)
		{
			p = getenv("user");
			if(!p)
			{
				p = getenv("USERNAME");
				if(!p)
				{
					p = getenv("username");
					if(!p)
					{
						return NULL;
					}
				}
			}
		}
		strncpy(username, p, MAXPLAYERNAME);
	}


	if(strcmp(username, "") != 0)
		return username;
#endif
	return NULL; // dummy for platform independent version
}

int I_mkdir(const char *dirname, int unixright)
{
//[segabor]
#if defined(LINUX) || defined(__MACH__) || defined (__CYGWIN__) || defined(__OS2__)
	return mkdir(dirname, unixright);
#elif defined(_WIN32) || (defined(_WIN32_WCE) && !defined(__GNUC__)) || defined(_WIN64)
	SECURITY_ATTRIBUTES ntrights= { 0, NULL, TRUE }; /// \todo should implement ntright under nt...
	unixright = 0;
#ifdef _WIN32_WCE
	return CreateDirectoryA(dirname,&ntrights);
#else
	return CreateDirectoryEx(".",dirname,&ntrights);
#endif
#else
	dirname = NULL;
	unixright = 0;
	return false;
#endif
}

/**	\brief	The isWadPathOk function

	\param	path	string path to check

	\return if true, wad file found

	
*/
static boolean isWadPathOk(const char *path)
{
	char wad3path[256];

	sprintf(wad3path, "%s/%s", path, WADKEYWORD1);

	if(!access(wad3path, R_OK))
		return true;

	sprintf(wad3path, "%s/%s", path, WADKEYWORD2);

	if(!access(wad3path, R_OK))
		return true;

	return false;
}

/**	\brief	search for srb2.srb in the given path

	\param	searchDir	starting path

	\return	WAD path if not NULL

	
*/
static const char *searchWad(const char *searchDir) 
{
	searchDir = NULL;
	return searchDir; /// \todo use filesearch?
}

/**	\brief go through all possible paths and look for srb2.srb

  \return path to srb2.srb if any
*/
static const char *locateWad(void)
{
	const char *WadPath;
	char *userhome;
	
	// does SRB2WADDIR exist?
	WadPath = getenv("SRB2WADDIR");
	if(WadPath && isWadPathOk(WadPath))
		return WadPath;
	
	// examine current dir
	strcpy(returnWadPath, ".");
	if(isWadPathOk(returnWadPath))
		return returnWadPath;
	
#ifdef DEFAULTWADLOCATION1
	// examine default dirs
	strcpy(returnWadPath, DEFAULTWADLOCATION1);
	if(isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION2
	strcpy(returnWadPath, DEFAULTWADLOCATION2);
	if(isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
	
	// find in $HOME
	userhome = getenv("HOME");
	if(userhome)
	{
		WadPath = searchWad(userhome);
		if(WadPath)
			return WadPath;
	}
#ifdef DEFAULTSEARCHPATH1
	// find in /usr/local
	WadPath = searchWad(DEFAULTSEARCHPATH1);
	if(WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH2
	// find in /usr/games
	WadPath = searchWad(DEFAULTSEARCHPATH2);
	if(WadPath)
		return WadPath;
#endif
	// if nothing was found
	return NULL;
}

void I_LocateWad(void)
{
	const char *waddir;

	waddir = locateWad();

	if(waddir) // change to the directory where we found srb2.srb
#if (defined(_WIN32) && !defined(_WIN32_WCE)) || defined(_WIN64)
		SetCurrentDirectory(waddir);
#elif !defined(_WIN32_WCE)
		chdir(waddir);
#else
		return;
#endif
}

#ifdef LINUX
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"
#endif

// quick fix for compil
ULONG I_GetFreeMem(ULONG *total)
{
#if defined(LINUX) && !defined(_arch_dreamcast)
	/* LINUX covers all the unix OS's.*/
#ifdef FREEBSD
	struct  vmmeter sum;
	kvm_t *kd;
	struct nlist namelist[]=
	{
#define X_SUM   0
		{"_cnt"},
		{ NULL }
	};
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	{
		*total = 0L;
		return 0;
	}
	if (kvm_nlist(kd, namelist) != 0)
	{
		kvm_close (kd);
		*total = 0L;
		return 0;
	}
	if (kvm_read(kd,namelist[X_SUM].n_value ,&sum, sizeof(sum)) != sizeof(sum))
	{
		kvm_close (kd);
		*total = 0L;
		return 0;
	}
	kvm_close (kd);

	if(total)
		*total = sum.v_page_count * sum.v_page_size;
	return sum.v_free_count * sum.v_page_size;
#else
#ifdef SOLARIS
	/* Just guess */
	*total = 32 << 20;
	return   32 << 20;
#else
	/* Linux */
	char buf[1024];    
	char *memTag;
	ULONG freeKBytes;
	ULONG totalKBytes;
	int n;
	int meminfo_fd = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if(n<0)
	{
		// Error
		*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if(NULL == (memTag = strstr(buf, MEMTOTAL)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof(MEMTOTAL);
	totalKBytes = atoi(memTag);

	if(NULL == (memTag = strstr(buf, MEMFREE)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof(MEMFREE);
	freeKBytes = atoi(memTag);

	if(total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#endif /* SOLARIS */
#endif /* FREEBSD */
#elif defined(_WIN32) || (defined(_WIN32_WCE) && !defined(__GNUC__)) || defined(_WIN64)
	MEMORYSTATUS info;

	info.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if(total)
		*total = (ULONG)info.dwTotalPhys;
	return (ULONG)info.dwAvailPhys;
#elif defined(__OS2__)
	ULONG pr_arena;

	if(total)
		DosQuerySysInfo( QSV_TOTPHYSMEM, QSV_TOTPHYSMEM,
							(PVOID) total, sizeof(ULONG));
	DosQuerySysInfo( QSV_MAXPRMEM, QSV_MAXPRMEM,
				(PVOID) &pr_arena, sizeof(ULONG));

	return pr_arena;
#else
	/*  Not Linux.*/
	if(total)
		*total = 16<<20;
	return 16<<20;
#endif /* LINUX */
}

const CPUInfoFlags *I_CPUInfo(void)
{
	static CPUInfoFlags SDL_CPUInfo;
	memset(&SDL_CPUInfo,0,sizeof(CPUInfoFlags));
#ifdef HAVE_SDLCPUINFO
	SDL_CPUInfo.RDTSC       = SDL_HasRDTSC();
	SDL_CPUInfo.MMX         = SDL_HasMMX();
	SDL_CPUInfo.MMXExt      = SDL_HasMMXExt();
	SDL_CPUInfo.AMD3DNow    = SDL_Has3DNow();
	SDL_CPUInfo.AMD3DNowExt = SDL_Has3DNowExt();
	SDL_CPUInfo.SSE         = SDL_HasSSE();
	SDL_CPUInfo.SSE2        = SDL_HasSSE2();
	SDL_CPUInfo.AltiVec     = SDL_HasAltiVec();
#else
	return NULL; /// \todo CPUID asm
#endif
	return &SDL_CPUInfo;
}
