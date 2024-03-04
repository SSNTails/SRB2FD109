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
/// \brief win32 system i/o
///
///	Startup & Shutdown routines for music,sound,timer,keyboard,...
///	Signal handler to trap errors and exit cleanly.

#include "../doomdef.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdarg.h>
#include <direct.h>

#include <mmsystem.h>

#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"

#include "../d_net.h"
#include "../g_game.h"

#include "../d_main.h"

#include "../m_argv.h"

#include "../w_wad.h"
#include "../z_zone.h"
#include "../g_input.h"

#include "../keys.h"

#include "../screen.h"

// Wheel support for Win95/WinNT3.51
#include <zmouse.h>

// Taken from Win98/NT4.0
#ifndef SM_MOUSEWHEELPRESENT
#define SM_MOUSEWHEELPRESENT 75
#endif

#ifndef MSH_MOUSEWHEEL
#ifdef UNICODE
#define MSH_MOUSEWHEEL L"MSWHEEL_ROLLMSG"
#else
#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
#endif
#endif

#include "win_main.h"
#include "../i_joy.h"

#define DIRECTINPUT_VERSION     0x700
// Force dinput.h to generate old DX3 headers.
#define DXVERSION_NTCOMPATIBLE  0x0300
#include <dinput.h>

#include "fabdxlib.h"

#ifdef __DEBUG__
#undef NDEBUG
#endif

/// \brief max number of joystick buttons
#define JOYBUTTONS_MAX 32 // rgbButtons[32]
/// \brief max number of joystick button events
#define JOYBUTTONS_MIN min((JOYBUTTONS),(JOYBUTTONS_MAX))

/// \brief max number of joysick axies
#define JOYAXISSET_MAX 4 // (lX, lY), (lZ ,lRx), (lRy, lRz), rglSlider[2] is very diff
/// \brief max number ofjoystick axis events
#define JOYAXISSET_MIN min((JOYAXISSET),(JOYAXISSET_MAX))

/// \brief max number of joystick hats
#define JOYHATS_MAX 4 // rgdwPOV[4];
/// \brief max number of joystick hat events
#define JOYHATS_MIN min((JOYHATS),(JOYHATS_MAX))

/// \brief max number of mouse buttons
#define MOUSEBUTTONS_MAX 8 // 8 bit of BYTE and DIMOFS_BUTTON7
/// \brief max number of muse button events
#define MOUSEBUTTONS_MIN min((MOUSEBUTTONS),(MOUSEBUTTONS_MAX))

// ==================
// DIRECT INPUT STUFF
// ==================
BOOL bDX0300; // if true, we created a DirectInput 0x0300 version
static LPDIRECTINPUT lpDI = NULL;
static LPDIRECTINPUTDEVICE lpDIK = NULL;   // Keyboard
static LPDIRECTINPUTDEVICE lpDIM = NULL;   // mice
static LPDIRECTINPUTDEVICE lpDIJ = NULL;   // joystick 1P
static LPDIRECTINPUTEFFECT lpDIE[NumberofForces];   // joystick 1Es
static LPDIRECTINPUTDEVICE2 lpDIJA = NULL; // joystick 1I
static LPDIRECTINPUTDEVICE lpDIJ2 = NULL;  // joystick 2P
static LPDIRECTINPUTEFFECT lpDIE2[NumberofForces];  // joystick 1Es
static LPDIRECTINPUTDEVICE2 lpDIJ2A = NULL;// joystick 2I

// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
byte graphics_started = 0;
byte keyboard_started = 0;
byte sound_started = 0;
static byte timer_started = 0;
static boolean mouse_enabled = false;
static boolean joystick_detected = false;
static boolean joystick2_detected = false;

static void I_ShutdownKeyboard(void);
static void I_GetKeyboardEvents(void);
static void I_ShutdownJoystick(void);
static void I_ShutdownJoystick2 (void);

//
// Why would this be system specific?? hmmmm....
//
// it is for virtual reality system, next incoming feature :)
static ticcmd_t emptycmd;
ticcmd_t* I_BaseTiccmd(void)
{
	return &emptycmd;
}

static ticcmd_t emptycmd2;
ticcmd_t* I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

// Allocates the base zone memory,
// this function returns a valid pointer and size,
// else it should interrupt the program immediately.
//
// now checks if mem could be allocated, this is still
// prehistoric... there's a lot to do here: memory locking, detection
// of win95 etc...
//

BOOL win9x;

/**	\brief WinNT system platform
*/
static BOOL winnt;

static void I_DetectWin9x(void)
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	winnt = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
	// 95 or 98 what the hell
	win9x = true;
}

// return free and total memory in the system
ULONG I_GetFreeMem(ULONG* total)
{
	MEMORYSTATUS info;

	info.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&info);
	if(total)
		*total = (ULONG)info.dwTotalPhys;
	return (ULONG)info.dwAvailPhys;
}

// ---------
// I_Profile
// Two little functions to profile our code using the high resolution timer
// ---------
static LARGE_INTEGER ProfileCount;
void I_BeginProfile(void)
{
	if(!QueryPerformanceCounter(&ProfileCount))
		I_Error ("I_BeginProfile failed"); // can't profile without the high res timer
}

// we're supposed to use this to measure very small amounts of time,
// that's why we return a DWORD and not a 64bit value
DWORD I_EndProfile(void)
{
	LARGE_INTEGER CurrTime;
	DWORD ret;
	if(!QueryPerformanceCounter (&CurrTime))
		I_Error("I_EndProfile failed");
	if(CurrTime.QuadPart - ProfileCount.QuadPart > (LONGLONG)0xFFFFFFFFUL)
		I_Error("I_EndProfile overflow");
	ret = (DWORD)(CurrTime.QuadPart - ProfileCount.QuadPart);
	// we can call I_EndProfile() several time, I_BeginProfile() need be called just once
	ProfileCount = CurrTime;

	return ret;
}

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windows NT
// ---------
static long hacktics = 0; // used locally for keyboard repeat keys
static DWORD starttickcount = 0; // hack for win2k time bug

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
		else
		{
			currtime.LowPart = timeGetTime();
			if(!basetime.LowPart)
				basetime.LowPart = currtime.LowPart;
			newtics = ((currtime.LowPart - basetime.LowPart)/(1000/TICRATE));
		}
	}
	else
		newtics = (GetTickCount() - starttickcount)/(1000/TICRATE);

	hacktics = newtics; // a local counter for keyboard repeat key
	return newtics;
}


void I_Sleep(void)
{
	if(cv_sleep.value != -1)
		Sleep(cv_sleep.value);
}


// should move to i_video
void I_WaitVBL(int count)
{
	count = 0;
}

// this is probably to activate the 'loading' disc icon
// it should set a flag, that I_FinishUpdate uses to know
// whether it draws a small 'loading' disc icon on the screen or not
//
// also it should explicitly draw the disc because the screen is
// possibly not refreshed while loading
//
void I_BeginRead(void) {}

// see above, end the 'loading' disc icon, set the flag false
//
void I_EndRead(void) {}

byte* I_AllocLow(int length)
{
	byte* mem;

	mem = (byte*)malloc(length);
	ZeroMemory(mem, length);
	return mem;
}

// ===========================================================================================
//                                                                                      EVENTS
// ===========================================================================================

// ----------
// I_GetEvent
// Post new events for all sorts of user-input
// ----------
void I_GetEvent(void)
{
	I_GetKeyboardEvents();
	I_GetMouseEvents();
	I_GetJoystickEvents();
	I_GetJoystick2Events();
}

// ----------
// I_OsPolling
// ----------
void I_OsPolling(void)
{
	MSG msg;

	// we need to dispatch messages to the window
	// so the window procedure can respond to messages and PostEvent() for keys
	// during D_SRB2Main startup.
	// this one replaces the main loop of windows since I_OsPolling is called in the main loop
	do
	{
		while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
			}
			else // winspec : this is quit message
				I_Quit();
		}
		if(!appActive && !netgame)
			WaitMessage();
	} while(!appActive && !netgame);

	// this is called by the network synchronization,
	// check keys and allow escaping
	I_GetEvent();

	// reset "emulated keys"
	gamekeydown[KEY_MOUSEWHEELUP] = 0;
	gamekeydown[KEY_MOUSEWHEELDOWN] = 0;
}

// ===========================================================================================
//                                                                              TIMER
// ===========================================================================================

#if 0
static void I_ShutdownTimer(void)
{
	timer_started = false;
}
#endif

//
// Installs the timer interrupt handler with timer speed as TICRATE.
//
#define TIMER_ID 1
#define TIMER_RATE (1000/TICRATE)
void I_StartupTimer(void)
{
	timer_started = true;

	// for win2k time bug
	if(M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("Using GetTickCount()\n");
	}
}

// ===========================================================================================
//                                                                   EXIT CODE, ERROR HANDLING
// ===========================================================================================

static int errorcount = 0; // phuck recursive errors
static int shutdowning = false;

//
// Used to trap various signals, to make sure things get shut down cleanly.
//
#ifdef NDEBUG
static void signal_handler(int num)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	const char* sigmsg;
	char sigdef[64];

	D_QuitNetGame(); // Fix server freezes
	I_ShutdownSystem();

	switch(num)
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
			sigmsg = "software termination signal from kill";
			break;
		case SIGBREAK:
			sigmsg = "Ctrl-Break sequence";
			break;
		case SIGABRT:
			sigmsg = "abnormal termination triggered by abort call";
			break;
		default:
			sprintf(sigdef, "signal number %d", num);
			sigmsg = sigdef;
	}

#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
	{
		FPrintf(logstream, "signal_handler() error: %s\n", sigmsg);
		CloseHandle(logstream);
		logstream = INVALID_HANDLE_VALUE;
	}
#endif

	MessageBox(hWndMain, va("signal_handler(): %s", sigmsg), "SRB2 error", MB_OK|MB_ICONERROR);

	signal(num, SIG_DFL); // default signal action
	raise(num);
}
#endif

//
// put an error message (with format) on stderr
//
void I_OutputMsg(const char *error, ...)
{
	va_list argptr;
	char txt[8192];

	va_start(argptr, error);
	vsprintf(txt, error, argptr);
	va_end(argptr);

	fprintf(stderr, "Error: %s\n", txt);
	// don't flush the message!

#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
		FPrintf(logstream, "%s", txt);
#endif
}

// display error messy after shutdowngfx
//
void I_Error(const char* error, ...)
{
	va_list argptr;
	char txt[8192];

	// added 11-2-98 recursive error detecting
	if(shutdowning)
	{
		errorcount++;
		// try to shutdown each subsystem separately
		if(errorcount == 5)
			I_ShutdownGraphics();
		if(errorcount == 6)
			I_ShutdownSystem();
		if(errorcount == 7)
		{
			M_SaveConfig(NULL);
			G_SaveGameData();
		}
		if(errorcount > 20)
		{
			// Don't print garbage
			va_start(argptr,error);
			vsprintf(txt, error, argptr);
			va_end(argptr);

			MessageBox(hWndMain, txt, "SRB2 Recursive Error", MB_OK|MB_ICONERROR);
			exit(-1); // recursive errors detected
		}
	}
	shutdowning = true;

	// put message to stderr
	va_start(argptr, error);
	wvsprintf(txt, error, argptr);
	va_end(argptr);

	CONS_Printf("I_Error(): %s\n", txt);

	// uncomment this line to print to stderr as well
	//wsprintf(stderr, "I_Error(): %s\n", txt);

	// saving one time is enough!
	if(!errorcount)
	{
		M_SaveConfig(NULL); // save game config, cvars..
		G_SaveGameData();
	}

	// save demo, could be useful for debug
	// NOTE: demos are normally not saved here.
	if(demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame();

	// shutdown everything that was started
	I_ShutdownSystem();

#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
	{
		CloseHandle(logstream);
		logstream = INVALID_HANDLE_VALUE;
	}
#endif

	MessageBox(hWndMain, txt, "SRB2 Error", MB_OK|MB_ICONERROR);

	exit(-1);
}

//
// I_Quit: shutdown everything cleanly, in reverse order of Startup.
//
void I_Quit(void)
{
	// when recording a demo, should exit using 'q',
	// but sometimes we forget and use Alt+F4, so save here too.
	if(demorecording)
		G_CheckDemoStatus();

	M_SaveConfig(NULL); // save game config, cvars..
	G_SaveGameData();

	// maybe it needs that the ticcount continues,
	// or something else that will be finished by I_ShutdownSystem(),
	// so do it before.
	D_QuitNetGame();

	// shutdown everything that was started
	I_ShutdownSystem();

	if(shutdowning || errorcount)
		I_Error("Error detected (%d)", errorcount);

#ifdef LOGMESSAGES
	if(logstream != INVALID_HANDLE_VALUE)
	{
		FPrintf(logstream,"I_Quit(): end of logstream.\n");
		CloseHandle(logstream);
		logstream = INVALID_HANDLE_VALUE;
	}
#endif

	fflush(stderr);
	exit(0);
}

// --------------------------------------------------------------------------
// I_ShowLastError
// Displays a GetLastError() error message in a MessageBox
// --------------------------------------------------------------------------
void I_GetLastErrorMsgBox(void)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf, 0, NULL);

	// Display the string.
	MessageBox(NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

	// put it in console too and log if any 
	CONS_Printf("Error: %s\n", lpMsgBuf);

	// Free the buffer.
	LocalFree(lpMsgBuf);
}

// ===========================================================================================
// CLEAN STARTUP & SHUTDOWN HANDLING, JUST CLOSE EVERYTHING YOU OPENED.
// ===========================================================================================
//
//
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

// Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	int c;

	for(c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if(!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}

// Removes a function from the list that need to be called by I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	int c;

	for(c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if(quit_funcs[c] == func)
		{
			while(c < MAX_QUIT_FUNCS - 1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

// ===========================================================================================
// DIRECT INPUT HELPER CODE
// ===========================================================================================

// Create a DirectInputDevice interface,
// create a DirectInputDevice2 interface if possible
static void CreateDevice2(LPDIRECTINPUT di, REFGUID pguid, LPDIRECTINPUTDEVICE* lpDEV,
                          LPDIRECTINPUTDEVICE2* lpDEV2)
{
	HRESULT hr, hr2;
	LPDIRECTINPUTDEVICE lpdid1;
	LPDIRECTINPUTDEVICE2 lpdid2 = NULL;

	hr = di->lpVtbl->CreateDevice(di, pguid, &lpdid1, NULL);

	if(SUCCEEDED(hr))
	{
		// get Device2 but only if we are not in DirectInput version 3
		if(!bDX0300 && lpDEV2)
		{
			hr2 = lpdid1->lpVtbl->QueryInterface(lpdid1, &IID_IDirectInputDevice2,
				(void**)&lpdid2);
			if(FAILED(hr2))
			{
				CONS_Printf("\2Could not create IDirectInput device 2");
				lpdid2 = NULL;
			}
		}
	}
	else
		I_Error("Could not create IDirectInput device");

	*lpDEV = lpdid1;
	if(lpDEV2) // only if we requested it
		*lpDEV2 = lpdid2;
}

// ===========================================================================================
//                                                                          DIRECT INPUT MOUSE
// ===========================================================================================

#define DI_MOUSE_BUFFERSIZE 16 // number of data elements in mouse buffer

//
// Initialise the mouse.
//
static void I_ShutdownMouse(void);

void I_StartupMouse(void)
{
	// this gets called when cv_usemouse is initted
	// for the win32 version, we want to startup the mouse later
}

static HANDLE mouse2filehandle = (HANDLE)(-1);

static void I_ShutdownMouse2(void)
{
	if(mouse2filehandle != (HANDLE)(-1))
	{
		event_t event;
		int i;

		SetCommMask(mouse2filehandle, 0);

		EscapeCommFunction(mouse2filehandle, CLRDTR);
		EscapeCommFunction(mouse2filehandle, CLRRTS);

		PurgeComm(mouse2filehandle, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

		CloseHandle(mouse2filehandle);

		// emulate the up of all mouse buttons
		for(i = 0; i < MOUSEBUTTONS; i++)
		{
			event.type = ev_keyup;
			event.data1 = KEY_2MOUSE1 + i;
			D_PostEvent(&event);
		}

		mouse2filehandle = (HANDLE)(-1);
	}
}

#define MOUSECOMBUFFERSIZE 256
static int handlermouse2x, handlermouse2y, handlermouse2buttons;

static void I_PoolMouse2(void)
{
	byte buffer[MOUSECOMBUFFERSIZE];
	COMSTAT ComStat;
	DWORD dwErrorFlags, dwLength;
	char dx, dy;

	static int bytenum;
	static byte combytes[4];
	DWORD i;

	ClearCommError(mouse2filehandle, &dwErrorFlags, &ComStat);
	dwLength = min(MOUSECOMBUFFERSIZE, ComStat.cbInQue);

	if(dwLength > 0)
	{
		if(!ReadFile(mouse2filehandle, buffer, dwLength, &dwLength, NULL))
		{
			CONS_Printf("\2Read Error on secondary mouse port\n");
			return;
		}

		// parse the mouse packets
		for(i = 0; i < dwLength; i++)
		{
			if((buffer[i] & 64) == 64)
				bytenum = 0;

			if(bytenum < 4)
				combytes[bytenum] = buffer[i];
			bytenum++;

			if(bytenum == 1)
			{
				handlermouse2buttons &= ~3;
				handlermouse2buttons |= ((combytes[0] & (32+16)) >>4);
			}
			else if(bytenum == 3)
			{
				dx = (char)((combytes[0] &  3) << 6);
				dy = (char)((combytes[0] & 12) << 4);
				dx = (char)(dx + combytes[1]);
				dy = (char)(dy + combytes[2]);
				handlermouse2x += dx;
				handlermouse2y += dy;
			}
			else if(bytenum == 4) // fourth byte (logitech mouses)
			{
				if(buffer[i] & 32)
					handlermouse2buttons |= 4;
				else
					handlermouse2buttons &= ~4;
			}
		}
	}
}

// secondary mouse doesn't use DirectX, therefore forget all about grabbing, acquire, etc.
void I_StartupMouse2(void)
{
	DCB dcb;

	if(mouse2filehandle != (HANDLE)(-1))
		I_ShutdownMouse2();

	if(!cv_usemouse2.value)
		return;

	if(mouse2filehandle != (HANDLE)(-1))
	{
		// COM file handle
		mouse2filehandle = CreateFile(cv_mouse2port.string, GENERIC_READ|GENERIC_WRITE,
		                              0, // exclusive access
		                              NULL, // no security attrs
		                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(mouse2filehandle == (HANDLE)(-1))
		{
			int e = GetLastError();
			if(e == 5)
				CONS_Printf("\2Can't open %s: Access denied\n"
					"The port is probably already used by another device (mouse, modem,...)\n",
						cv_mouse2port.string);
			else
				CONS_Printf("\2Can't open %s: error %d\n", cv_mouse2port.string, e);
			return;
		}
	}

	// buffers
	SetupComm(mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);

	// purge buffers
	PurgeComm(mouse2filehandle, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

	// setup port to 1200 7N1
	dcb.DCBlength = sizeof(DCB);

	GetCommState(mouse2filehandle, &dcb);

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.fBinary = dcb.fParity = TRUE;

	SetCommState(mouse2filehandle, &dcb);

	I_AddExitFunc(I_ShutdownMouse2);
}

#define MAX_MOUSE_BTNS 5
static int center_x, center_y;
static int old_mparms[3], new_mparms[3] = {0, 0, 1};
static boolean restore_mouse = FALSE;
static int old_mouse_state = 0;
unsigned int MSHWheelMessage = 0;

static void I_DoStartupSysMouse(void)
{
	boolean valid;
	RECT w_rect;

	valid = SystemParametersInfo(SPI_GETMOUSE, 0, old_mparms, 0);
	if(valid)
	{
		new_mparms[2] = old_mparms[2];
		restore_mouse = SystemParametersInfo(SPI_SETMOUSE, 0, new_mparms, 0);
	}

	if(bAppFullScreen)
	{
		w_rect.top = 0;
		w_rect.left = 0;
	}
	else
	{
		w_rect.top = windowPosY;
		w_rect.left = windowPosX;
	}

	w_rect.bottom = w_rect.top + VIDHEIGHT;
	w_rect.right = w_rect.left + VIDWIDTH;
	center_x = w_rect.left + (VIDWIDTH >> 1);
	center_y = w_rect.top + (VIDHEIGHT >> 1);
	SetCursor(NULL);    
	SetCursorPos(center_x, center_y);
	SetCapture(hWndMain);
	ClipCursor(&w_rect);
}

static void I_ShutdownSysMouse(void)
{
	if(restore_mouse)
		SystemParametersInfo(SPI_SETMOUSE, 0, old_mparms, 0);
	ClipCursor(NULL);
	ReleaseCapture();
}

void I_RestartSysMouse(void)
{
	if(nodinput)
	{
		I_ShutdownSysMouse();
		I_DoStartupSysMouse();
	}
}

void I_GetSysMouseEvents(int mouse_state)
{
	int i;
	event_t event;
	int xmickeys = 0, ymickeys = 0;
	POINT c_pos;

	for(i = 0; i < MAX_MOUSE_BTNS; i++)
	{
		// check if button pressed
		if((mouse_state & (1 << i)) && !(old_mouse_state & ( 1 << i)))
		{
			event.type = ev_keydown;
			event.data1 = KEY_MOUSE1 + i;
			D_PostEvent(&event);
		}
		// check if button released
		if(!(mouse_state & ( 1 << i)) && (old_mouse_state & (1 << i)))
		{
			event.type = ev_keyup;
			event.data1 = KEY_MOUSE1 + i;
			D_PostEvent(&event);
		}
	}
	old_mouse_state = mouse_state;

	// proceed mouse movements
	GetCursorPos(&c_pos);
	xmickeys = c_pos.x - center_x;
	ymickeys = c_pos.y - center_y;

	if(xmickeys || ymickeys)
	{
		event.type  = ev_mouse;
		event.data1 = 0;
		event.data2 = xmickeys;
		event.data3 = -ymickeys;
		D_PostEvent(&event);
		SetCursorPos(center_x, center_y);
	}
}

// This is called just before entering the main game loop,
// when we are going fullscreen and the loading screen has finished.
void I_DoStartupMouse(void)
{
	DIPROPDWORD dip;

	// mouse detection may be skipped by setting usemouse false
	if(!cv_usemouse.value || M_CheckParm("-nomouse"))
	{
		mouse_enabled = false;
		return;
	}

	if(nodinput)
	{
		CONS_Printf("\tMouse will not use DirectInput.\n");
		// System mouse input will be initiated by VID_SetMode
		I_AddExitFunc(I_ShutdownMouse);

		MSHWheelMessage = RegisterWindowMessage(MSH_MOUSEWHEEL);
	}
	else if(!lpDIM) // acquire the mouse only once
	{
		CreateDevice2(lpDI, &GUID_SysMouse, &lpDIM, NULL);

		if(lpDIM)
		{
			if(FAILED(lpDIM->lpVtbl->SetDataFormat(lpDIM, &c_dfDIMouse)))
				I_Error("Couldn't set mouse data format");

			// create buffer for buffered data
			dip.diph.dwSize = sizeof(dip);
			dip.diph.dwHeaderSize = sizeof(dip.diph);
			dip.diph.dwObj = 0;
			dip.diph.dwHow = DIPH_DEVICE;
			dip.dwData = DI_MOUSE_BUFFERSIZE;
			if(FAILED(lpDIM->lpVtbl->SetProperty(lpDIM, DIPROP_BUFFERSIZE, &dip.diph)))
				I_Error("Couldn't set mouse buffer size");

			if(FAILED(lpDIM->lpVtbl->SetCooperativeLevel(lpDIM, hWndMain,
				DISCL_EXCLUSIVE|DISCL_FOREGROUND)))
			{
				I_Error("Couldn't set mouse coop level");
			}
		}
		else
			I_Error("Couldn't create mouse input");
	}

	if(lpDIM)
		I_AddExitFunc(I_ShutdownMouse);

	// if re-enabled while running, just set mouse_enabled true again,
	// do not acquire the mouse more than once
	mouse_enabled = true;
}

//
// Shutdown Mouse DirectInput device
//
static void I_ShutdownMouse(void)
{
	int i;
	event_t event;

	CONS_Printf("I_ShutdownMouse()\n");

	if(lpDIM)
	{
		lpDIM->lpVtbl->Unacquire(lpDIM);
		lpDIM->lpVtbl->Release(lpDIM);
		lpDIM = NULL;
	}

	// emulate the up of all mouse buttons
	for(i = 0; i < MOUSEBUTTONS; i++)
	{
		event.type = ev_keyup;
		event.data1 = KEY_MOUSE1 + i;
		D_PostEvent(&event);
	}
	if(nodinput)
		I_ShutdownSysMouse();

	mouse_enabled = false;
}

//
// Get buffered data from the mouse
//
void I_GetMouseEvents(void)
{
	DIDEVICEOBJECTDATA rgdod[DI_MOUSE_BUFFERSIZE];
	DWORD dwItems, d;
	HRESULT hr;

	event_t event;
	int xmickeys, ymickeys;

	if(mouse2filehandle != (HANDLE)(-1))
	{
		//mouse movement
		static byte lastbuttons2 = 0;

		I_PoolMouse2();
		// post key event for buttons
		if(handlermouse2buttons != lastbuttons2)
		{
			int i, j = 1, k;
			k = handlermouse2buttons ^ lastbuttons2; // only changed bit to 1
			lastbuttons2=(byte)handlermouse2buttons;

			for(i = 0; i < MOUSEBUTTONS; i++, j <<= 1)
				if(k & j)
				{
					if(handlermouse2buttons & j)
						event.type = ev_keydown;
					else
						event.type = ev_keyup;
					event.data1 = KEY_2MOUSE1 + i;
					D_PostEvent(&event);
				}
		}

		if(handlermouse2x || handlermouse2y)
		{
			event.type = ev_mouse2;
			event.data1 = 0;
			event.data2 = handlermouse2x<<1;
			event.data3 = -handlermouse2y<<1;
			handlermouse2x = 0;
			handlermouse2y = 0;

			D_PostEvent(&event);
		}
	}

	if(!mouse_enabled || nodinput)
		return;

getBufferedData:
	dwItems = DI_MOUSE_BUFFERSIZE;
	hr = lpDIM->lpVtbl->GetDeviceData(lpDIM, sizeof(DIDEVICEOBJECTDATA), rgdod, &dwItems, 0);

	// If data stream was interrupted, reacquire the device and try again.
	if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		hr = lpDIM->lpVtbl->Acquire(lpDIM);
		if(SUCCEEDED(hr))
			goto getBufferedData;
	}

	// We got buffered input, act on it
	if(SUCCEEDED(hr))
	{
		xmickeys = ymickeys = 0;

		// dwItems contains number of elements read (could be 0)
		for(d = 0; d < dwItems; d++)
		{
			if(rgdod[d].dwOfs >= DIMOFS_BUTTON0 &&
				rgdod[d].dwOfs <  DIMOFS_BUTTON0 + MOUSEBUTTONS)
			{
				if(rgdod[d].dwData & 0x80) // Button down
					event.type = ev_keydown;
				else
					event.type = ev_keyup; // Button up

				event.data1 = rgdod[d].dwOfs - DIMOFS_BUTTON0 + KEY_MOUSE1;
				D_PostEvent(&event);
			}
			else if(rgdod[d].dwOfs == DIMOFS_X)
				xmickeys += rgdod[d].dwData;
			else if(rgdod[d].dwOfs == DIMOFS_Y)
				ymickeys += rgdod[d].dwData;

			else if(rgdod[d].dwOfs == DIMOFS_Z) 
			{
				// z-axes the wheel
				if((int)rgdod[d].dwData>0)
					event.data1 = KEY_MOUSEWHEELUP;
				else
					event.data1 = KEY_MOUSEWHEELDOWN;
				event.type = ev_keydown;
				D_PostEvent(&event);
			}
			
		}

		if(xmickeys || ymickeys)
		{
			event.type = ev_mouse;
			event.data1 = 0;
			event.data2 = xmickeys;
			event.data3 = -ymickeys;
			D_PostEvent(&event);
		}
	}
}

// ===========================================================================================
//                                                                       DIRECT INPUT JOYSTICK
// ===========================================================================================

struct DIJoyInfo_s
{
	BYTE X,Y,Z,Rx,Ry,Rz,U,V;
	LONG ForceAxises;
};
typedef struct DIJoyInfo_s DIJoyInfo_t;

// private info
	static BYTE iJoyNum;        // used by enumeration
	static DIJoyInfo_t JoyInfo;
	static BYTE iJoy2Num;
	static DIJoyInfo_t JoyInfo2;

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
static BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext )
{
    DWORD* pdwNumForceFeedbackAxis = (DWORD*) pContext;

    if( (pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0 )
        (*pdwNumForceFeedbackAxis)++;

    return DIENUM_CONTINUE;
}


static HRESULT SetupForceTacile(LPDIRECTINPUTDEVICE2 DJI, LPDIRECTINPUTEFFECT *DJE, DWORD FFAXIS, FFType EffectType,REFGUID EffectGUID)
{
	HRESULT hr;
	DIEFFECT eff;
	DWORD rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
	LONG rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf = { 0 }; // LONG lMagnitude
	DIRAMPFORCE rf = {0,0}; // LONG lStart, lEnd;
	DIPERIODIC pf = {0,0,0,0};
	ZeroMemory( &eff, sizeof(eff) );
	if( FFAXIS > 2 )
		FFAXIS = 2; //up to 2 FFAXIS
	eff.dwSize                  = sizeof(DIEFFECT);
	eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS; // Cartesian and data format offsets
	eff.dwDuration              = INFINITE;
	eff.dwSamplePeriod          = 0;
	eff.dwGain                  = DI_FFNOMINALMAX;
	eff.dwTriggerButton         = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes                   = FFAXIS;
	eff.rgdwAxes                = rgdwAxes;
	eff.rglDirection            = rglDirection;
	eff.lpEnvelope              = NULL;
	eff.lpvTypeSpecificParams   = NULL;
	if(EffectType == ConstantForce)
	{
		eff.cbTypeSpecificParams    = sizeof(cf);
		eff.lpvTypeSpecificParams   = &cf;
	}
	else if(EffectType == RampForce)
	{
		eff.cbTypeSpecificParams    = sizeof(rf);
		eff.lpvTypeSpecificParams   = &rf;
	}
	else if(EffectType >= SquareForce && SawtoothDownForce >= EffectType)
	{
		eff.cbTypeSpecificParams    = sizeof(pf);
		eff.lpvTypeSpecificParams   = &pf;
	}
#if(DIRECTINPUT_VERSION >= 0x0600)
	//eff.dwStartDelay            = 0;
#endif

    // Create the prepared effect
    if( FAILED( hr = DJI->lpVtbl->CreateEffect(DJI, EffectGUID, 
                                              &eff, DJE, NULL ) ) )
    {
        return hr;
    }

    if( NULL == *DJE )
        return E_FAIL;

    return hr;
}

static BOOL CALLBACK DIEnumEffectsCallback1(LPCDIEFFECTINFO pdei, LPVOID pvRef)
{
	LPDIRECTINPUTEFFECT *DJE = pvRef;
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_CONSTANTFORCE)
	{
		if(SUCCEEDED(SetupForceTacile(lpDIJA,DJE, JoyInfo.ForceAxises, ConstantForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_RAMPFORCE)
	{
		if(SUCCEEDED(SetupForceTacile(lpDIJA,DJE, JoyInfo.ForceAxises, RampForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIEnumEffectsCallback2(LPCDIEFFECTINFO pdei, LPVOID pvRef)
{
	LPDIRECTINPUTEFFECT *DJE = pvRef;
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_CONSTANTFORCE)
	{
		if(SUCCEEDED(SetupForceTacile(lpDIJ2A,DJE, JoyInfo2.ForceAxises, ConstantForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_RAMPFORCE)
	{
		if(SUCCEEDED(SetupForceTacile(lpDIJ2A,DJE, JoyInfo2.ForceAxises, RampForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}

static REFGUID DIETable[] =
{
	&GUID_ConstantForce, //ConstantForce
	&GUID_RampForce,     //RampForce
	&GUID_Square,        //SquareForce
	&GUID_Sine,          //SineForce
	&GUID_Triangle,      //TriangleForce
	&GUID_SawtoothUp,    //SawtoothUpForce
	&GUID_SawtoothDown,  //SawtoothDownForce
	(REFGUID)-1,         //NumberofForces
};

static HRESULT SetupAllForces(LPDIRECTINPUTDEVICE2 DJI, LPDIRECTINPUTEFFECT DJE[], DWORD FFAXIS)
{
	FFType ForceType = EvilForce;
	if(DJI == lpDIJA)
	{
		DJI->lpVtbl->EnumEffects(DJI,DIEnumEffectsCallback1,&DJE[ConstantForce],DIEFT_CONSTANTFORCE);
		DJI->lpVtbl->EnumEffects(DJI,DIEnumEffectsCallback1,&DJE[RampForce],DIEFT_RAMPFORCE);
	}
	else if(DJI == lpDIJA)
	{
		DJI->lpVtbl->EnumEffects(DJI,DIEnumEffectsCallback2,&DJE[ConstantForce],DIEFT_CONSTANTFORCE);
		DJI->lpVtbl->EnumEffects(DJI,DIEnumEffectsCallback2,&DJE[RampForce],DIEFT_RAMPFORCE);
	}
	for(ForceType = SquareForce; ForceType >  NumberofForces && DIETable[ForceType] != (REFGUID)-1; ForceType++)
		if(DIETable[ForceType])
			SetupForceTacile(DJI,&DJE[ForceType], FFAXIS, ForceType, DIETable[ForceType]);
	return S_OK;
}

static void LimitEffect(LPDIEFFECT eff, FFType EffectType)
{
	LPDICONSTANTFORCE pCF = eff->lpvTypeSpecificParams;
	LPDIPERIODIC pDP= eff->lpvTypeSpecificParams;
	if(eff->rglDirection)
	{
	}
/*	if(eff->dwDuration != INFINITE && eff->dwDuration < 0)
	{
		eff->dwDuration = 0;
	}*/
	if(eff->dwGain != 0)
	{
		if(eff->dwGain > DI_FFNOMINALMAX)
			eff->dwGain = DI_FFNOMINALMAX;
		//else if(eff->dwGain < -DI_FFNOMINALMAX)
		//	eff->dwGain = DI_FFNOMINALMAX;
	}
	if(EffectType == ConstantForce && pCF->lMagnitude)
	{
	}
	else if(EffectType >= SquareForce && SawtoothDownForce >= EffectType && pDP)
	{
	}

}

static HRESULT SetForceTacile(LPDIRECTINPUTEFFECT SDIE, const JoyFF_t *FF,DWORD FFAXIS, FFType EffectType)
{
	DIEFFECT eff;
	HRESULT hr;
	LONG Magnitude;
	LONG rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf = { 0 }; // LONG lMagnitude
	DIRAMPFORCE rf = {0,0}; // LONG lStart, lEnd;
	DIPERIODIC pf = {0,0,0,0};
	if(!FF)
		SDIE->lpVtbl->Stop(SDIE);
	Magnitude = FF->Magnitude;
	ZeroMemory( &eff, sizeof(eff) );
	eff.dwSize                  = sizeof(eff);
	//DIEP_START
	eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS; // Cartesian and data format offsets
	//DIEP_DURATION
	eff.dwDuration              = FF->Duration;
	//DIEP_GAIN
	eff.dwGain                  = FF->Gain;
	//DIEP_DIRECTION
	eff.rglDirection            = rglDirection;
	//DIEP_TYPESPECIFICPARAMS
	if(FFAXIS > 1)
	{
		double dMagnitude;
		dMagnitude                  = (double)Magnitude;
		dMagnitude                  = sqrt( dMagnitude * dMagnitude + dMagnitude * dMagnitude );
		Magnitude                   = (DWORD)dMagnitude;
		rglDirection[0]             = FF->ForceX;
		rglDirection[1]             = FF->ForceY;
	}
	if(EffectType == ConstantForce)
	{
		cf.lMagnitude               = Magnitude;
		eff.cbTypeSpecificParams    = sizeof(cf);
		eff.lpvTypeSpecificParams   = &cf;
	}
	else if(EffectType == RampForce)
	{
		rf.lStart                   = FF->Start;
		rf.lEnd                     = FF->End;
		eff.cbTypeSpecificParams    = sizeof(rf);
		eff.lpvTypeSpecificParams   = &rf;
	}
	else if(EffectType >= SquareForce && SawtoothDownForce >= EffectType)
	{
		pf.dwMagnitude              = Magnitude;
		pf.lOffset                  = FF->Offset;
		pf.dwPhase                  = FF->Phase;
		pf.dwPeriod                 = FF->Period;
		eff.cbTypeSpecificParams    = sizeof(pf);
		eff.lpvTypeSpecificParams   = &pf;
	}

	LimitEffect(&eff, EffectType);

	hr = SDIE->lpVtbl->SetParameters(SDIE, &eff,
	 DIEP_START|DIEP_DURATION|DIEP_GAIN|DIEP_DIRECTION|DIEP_TYPESPECIFICPARAMS);
	return hr;
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	if(!lpDIJA) return;
	if(FAILED(lpDIJA->lpVtbl->Acquire(lpDIJA)))
		return;
	if(Type == EvilForce)
		lpDIJA->lpVtbl->SendForceFeedbackCommand(lpDIJA,DISFFC_STOPALL);
	if(Type <= EvilForce || Type > NumberofForces || !lpDIE[Type])
		return;
	SetForceTacile(lpDIE[Type], Effect, JoyInfo.ForceAxises, Type);
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	if(!lpDIJ2A) return;
	if(FAILED(lpDIJ2A->lpVtbl->Acquire(lpDIJ2A)))
		return;
	if(Type == EvilForce)
		lpDIJ2A->lpVtbl->SendForceFeedbackCommand(lpDIJ2A,DISFFC_STOPALL);
	if(Type <= EvilForce || Type > NumberofForces || !lpDIE2[Type])
		return;
	SetForceTacile(lpDIE2[Type],Effect, JoyInfo2.ForceAxises, Type);
}

// ------------------
// SetDIDwordProperty ( HELPER )
// Set a DWORD property on a DirectInputDevice.
// ------------------
static HRESULT SetDIDwordProperty( LPDIRECTINPUTDEVICE pdev,
                                   REFGUID guidProperty,
                                   DWORD dwObject,
                                   DWORD dwHow,
                                   DWORD dwValue)
{
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize       = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj        = dwObject;
	dipdw.diph.dwHow        = dwHow;
	dipdw.dwData            = dwValue;

	return pdev->lpVtbl->SetProperty( pdev, guidProperty, &dipdw.diph );
}


// ---------------
// DIEnumJoysticks
// There is no such thing as a 'system' joystick, contrary to mouse,
// we must enumerate and choose one joystick device to use
// ---------------
static BOOL CALLBACK DIEnumJoysticks ( LPCDIDEVICEINSTANCE lpddi,
                                       LPVOID pvRef )   //cv_usejoystick
{
	LPDIRECTINPUTDEVICE pdev;
	DIPROPRANGE         diprg;
	DIDEVCAPS_DX3       caps;
	BOOL                bUseThisOne = FALSE;

	iJoyNum++;

	//faB: if cv holds a string description of joystick, the value from atoi() is 0
	//     else, the value was probably set by user at console to one of the previously
	//     enumerated joysticks
	if ( ((consvar_t *)pvRef)->value == iJoyNum ||
		 !lstrcmp( ((consvar_t *)pvRef)->string, lpddi->tszProductName ) )
		bUseThisOne = TRUE;

	//CONS_Printf (" cv joy is %s\n", ((consvar_t *)pvRef)->string);

	// print out device name
	CONS_Printf ("%c%d: %s\n",
	             ( bUseThisOne ) ? '\2' : ' ',   // show name in white if this is the one we will use
	             iJoyNum,
	             //( GET_DIDEVICE_SUBTYPE(lpddi->dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD ) ? "Gamepad " : "Joystick",
	             lpddi->tszProductName ); // , lpddi->tszInstanceName );

	// use specified joystick (cv_usejoystick.value in pvRef)
	if ( !bUseThisOne )
		return DIENUM_CONTINUE;

	((consvar_t *)pvRef)->value = iJoyNum;
	if (lpDI->lpVtbl->CreateDevice (lpDI, &lpddi->guidInstance,
	                                &pdev, NULL) != DI_OK)
	{
		// if it failed, then we can't use this joystick for some
		// bizarre reason.  (Maybe the user unplugged it while we
		// were in the middle of enumerating it.)  So continue enumerating
		CONS_Printf ("DIEnumJoysticks(): CreateDevice FAILED\n");
		return DIENUM_CONTINUE;
	}


	// get the Device capabilities
	//
	caps.dwSize = sizeof(DIDEVCAPS_DX3);
	if ( FAILED( pdev->lpVtbl->GetCapabilities ( pdev, (DIDEVCAPS*)&caps ) ) )
	{
		CONS_Printf ("DIEnumJoysticks(): GetCapabilities FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}
	if ( !(caps.dwFlags & DIDC_ATTACHED) )   // should be, since we enumerate only attached devices
		return DIENUM_CONTINUE;
	
	Joystick.bJoyNeedPoll = (( caps.dwFlags & DIDC_POLLEDDATAFORMAT ) != 0);

	if ( caps.dwFlags & DIDC_FORCEFEEDBACK )
		JoyInfo.ForceAxises = 0;
	else
		JoyInfo.ForceAxises = -1;

	Joystick.bGamepadStyle = ( GET_DIDEVICE_SUBTYPE( caps.dwDevType ) == DIDEVTYPEJOYSTICK_GAMEPAD );
	//DEBUG CONS_Printf ("Gamepad: %d\n", Joystick.bGamepadStyle);


	CONS_Printf ("Capabilities: %d axes, %d buttons, %d POVs, poll %d, Gamepad %d\n",
	             caps.dwAxes, caps.dwButtons, caps.dwPOVs, Joystick.bJoyNeedPoll, Joystick.bGamepadStyle);
	

	// Set the data format to "simple joystick" - a predefined data format 
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing a
	// DIJOYSTATE structure to IDirectInputDevice::GetDeviceState.
	if (pdev->lpVtbl->SetDataFormat (pdev, &c_dfDIJoystick) != DI_OK)
	{
		CONS_Printf ("DIEnumJoysticks(): SetDataFormat FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}

	// Set the cooperativity level to let DirectInput know how
	// this device should interact with the system and with other
	// DirectInput applications.
	if (pdev->lpVtbl->SetCooperativeLevel (pdev, hWndMain,
	         DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
	{
		CONS_Printf ("DIEnumJoysticks(): SetCooperativeLevel FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}

	// set the range of the joystick axis
	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -JOYAXISRANGE;    // value for extreme left
	diprg.lMax              = +JOYAXISRANGE;    // value for extreme right

	diprg.diph.dwObj = DIJOFS_X;    // set the x-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//goto SetPropFail;
		JoyInfo.X = FALSE;
	}
	else JoyInfo.X = TRUE;

	diprg.diph.dwObj = DIJOFS_Y;    // set the y-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
//SetPropFail:
//		CONS_Printf ("DIEnumJoysticks(): SetProperty FAILED\n");
//		pdev->lpVtbl->Release (pdev);
//		return DIENUM_CONTINUE;
		JoyInfo.Y = FALSE;
	}
	else JoyInfo.Y = TRUE;

	diprg.diph.dwObj = DIJOFS_Z;    // set the z-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_Z not found\n");
		JoyInfo.Z = FALSE;
	}
	else JoyInfo.Z = TRUE;

	diprg.diph.dwObj = DIJOFS_RX;   // set the x-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RX (x-rudder) not found\n");
		JoyInfo.Rx = FALSE;
	}
	else JoyInfo.Rx = TRUE;

	diprg.diph.dwObj = DIJOFS_RY;   // set the y-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RY (y-rudder) not found\n");
		JoyInfo.Ry = FALSE;
	}
	else JoyInfo.Ry = TRUE;

	diprg.diph.dwObj = DIJOFS_RZ;   // set the z-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (z-rudder) not found\n");
		JoyInfo.Rz = FALSE;
	}
	else JoyInfo.Rz = TRUE;
	diprg.diph.dwObj = DIJOFS_SLIDER(0);   // set the x-misc range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (x-misc) not found\n");
		JoyInfo.U = FALSE;
	}
	else JoyInfo.U = TRUE;

	diprg.diph.dwObj = DIJOFS_SLIDER(1);   // set the y-misc range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (y-misc) not found\n");
		JoyInfo.V = FALSE;
	}
	else JoyInfo.V = TRUE;

	// set X axis dead zone to 25% (to avoid accidental turning)
	if ( !Joystick.bGamepadStyle )
	{
		if(JoyInfo.X)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_X,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for X DEAD ZONE");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.Y)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_Y,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for Y DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.Z)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_Z,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for Z DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.Rx)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RX,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for RX DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.Ry)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RY,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for RY DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.Rz)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RZ,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for RZ DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.U)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(0),
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for U DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo.V)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(1),
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for V DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
	}

	// query for IDirectInputDevice2 - we need this to poll the joystick 
	if ( bDX0300 )
	{
		FFType i = EvilForce;
		// we won't use the poll
		lpDIJA = NULL;
		for(i = 0; i > NumberofForces; i++)
			lpDIE[i] = NULL;
	}
	else
	{
		if (FAILED( pdev->lpVtbl->QueryInterface(pdev, &IID_IDirectInputDevice2,
		                                         (LPVOID *)&lpDIJA) ) )
		{
			CONS_Printf ("DIEnumJoysticks(): QueryInterface FAILED\n");
			pdev->lpVtbl->Release (pdev);
			return DIENUM_CONTINUE;
		}

		if(lpDIJA && JoyInfo.ForceAxises != -1)
		{
			// Since we will be playing force feedback effects, we should disable the
			// auto-centering spring.
			if( FAILED( SetDIDwordProperty( pdev, DIPROP_AUTOCENTER, 0, DIPH_DEVICE, FALSE) ) )
			{
				//NOP
			}
		
			// Enumerate and count the axes of the joystick 
			if ( FAILED( pdev->lpVtbl->EnumObjects( pdev, EnumAxesCallback, 
																	(VOID*)&JoyInfo.ForceAxises, DIDFT_AXIS ) ) )
			{
				JoyInfo.ForceAxises = -1;
			}
			else
			{
				SetupAllForces(lpDIJA,lpDIE,JoyInfo.ForceAxises);
			}
		}
	}
	
	// we successfully created an IDirectInputDevice.  So stop looking 
	// for another one.
	lpDIJ = pdev;
	return DIENUM_STOP;
}

// --------------
// I_InitJoystick
// This is called everytime the 'use_joystick' variable changes
// It is normally called at least once at startup when the config is loaded
// --------------
void I_InitJoystick(void)
{
	HRESULT hr;

	// cleanup
	I_ShutdownJoystick();

	//joystick detection can be skipped by setting use_joystick to 0
	if(M_CheckParm("-nojoy"))
	{
		CONS_Printf("Joystick disabled\n");
		return;
	}
	else
		// don't do anything at the registration of the joystick cvar,
		// until config is loaded
		 if(!lstrcmp(cv_usejoystick.string, "0" ))
		return;

	// acquire the joystick only once
	if(!lpDIJ)
	{
		joystick_detected = false;

		CONS_Printf("Looking for joystick devices:\n");
		iJoyNum = 0;
		hr = lpDI->lpVtbl->EnumDevices(lpDI, DIDEVTYPE_JOYSTICK, DIEnumJoysticks,
			(void*)&cv_usejoystick, // our user parameter is joystick number
			DIEDFL_ATTACHEDONLY);
		if(FAILED(hr))
		{
			CONS_Printf("\nI_InitJoystick(): EnumDevices FAILED\n");
			cv_usejoystick.value = 0;
			return;
		}

		if(!lpDIJ)
		{
			if(!iJoyNum)
				CONS_Printf("none found\n");
			else
			{
				CONS_Printf("none used\n");
				if(cv_usejoystick.value > 0 && cv_usejoystick.value > iJoyNum)
				{
					CONS_Printf("\2Set the use_joystick variable to one of the"
						" enumerated joystick numbers\n");
				}
			}
			cv_usejoystick.value = 0;
			return;
		}

		I_AddExitFunc(I_ShutdownJoystick);

		// set coop level
		if(FAILED(lpDIJ->lpVtbl->SetCooperativeLevel(lpDIJ, hWndMain,
		          DISCL_NONEXCLUSIVE|DISCL_FOREGROUND)))
		{
			I_Error("I_InitJoystick: SetCooperativeLevel FAILED");
		}
	}
	else
		CONS_Printf("Joystick already initialized\n");

	// we don't unacquire joystick, so let's just pretend we re-acquired it
	joystick_detected = true;
}
//Joystick 2

// ---------------
// DIEnumJoysticks2
// There is no such thing as a 'system' joystick, contrary to mouse,
// we must enumerate and choose one joystick device to use
// ---------------
static BOOL CALLBACK DIEnumJoysticks2 ( LPCDIDEVICEINSTANCE lpddi,
                                        LPVOID pvRef )   //cv_usejoystick
{
	LPDIRECTINPUTDEVICE pdev;
	DIPROPRANGE         diprg;
	DIDEVCAPS_DX3       caps;
	BOOL                bUseThisOne = FALSE;

	iJoy2Num++;

	//faB: if cv holds a string description of joystick, the value from atoi() is 0
	//     else, the value was probably set by user at console to one of the previsouly
	//     enumerated joysticks
	if ( ((consvar_t *)pvRef)->value == iJoy2Num ||
		 !lstrcmp( ((consvar_t *)pvRef)->string, lpddi->tszProductName ) )
		bUseThisOne = TRUE;

	//CONS_Printf (" cv joy2 is %s\n", ((consvar_t *)pvRef)->string);

	// print out device name
	CONS_Printf ("%c%d: %s\n",
	             ( bUseThisOne ) ? '\2' : ' ',   // show name in white if this is the one we will use
	             iJoy2Num,
	             //( GET_DIDEVICE_SUBTYPE(lpddi->dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD ) ? "Gamepad " : "Joystick",
	             lpddi->tszProductName ); // , lpddi->tszInstanceName );

	// use specified joystick (cv_usejoystick.value in pvRef)
	if ( !bUseThisOne )
		return DIENUM_CONTINUE;

	((consvar_t *)pvRef)->value = iJoy2Num;
	if (lpDI->lpVtbl->CreateDevice (lpDI, &lpddi->guidInstance,
	                                &pdev, NULL) != DI_OK)
	{
		// if it failed, then we can't use this joystick for some
		// bizarre reason.  (Maybe the user unplugged it while we
		// were in the middle of enumerating it.)  So continue enumerating
		CONS_Printf ("DIEnumJoysticks2(): CreateDevice FAILED\n");
		return DIENUM_CONTINUE;
	}


	// get the Device capabilities
	//
	caps.dwSize = sizeof(DIDEVCAPS_DX3);
	if ( FAILED( pdev->lpVtbl->GetCapabilities ( pdev, (DIDEVCAPS*)&caps ) ) )
	{
		CONS_Printf ("DIEnumJoysticks2(): GetCapabilities FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}
	if ( !(caps.dwFlags & DIDC_ATTACHED) )   // should be, since we enumerate only attached devices
		return DIENUM_CONTINUE;

	Joystick2.bJoyNeedPoll = (( caps.dwFlags & DIDC_POLLEDDATAFORMAT ) != 0);

	if ( caps.dwFlags & DIDC_FORCEFEEDBACK )
		JoyInfo2.ForceAxises = 0;
	else
		JoyInfo2.ForceAxises = -1;

	Joystick2.bGamepadStyle = ( GET_DIDEVICE_SUBTYPE( caps.dwDevType ) == DIDEVTYPEJOYSTICK_GAMEPAD );
	//DEBUG CONS_Printf ("Gamepad: %d\n", Joystick2.bGamepadStyle);


	CONS_Printf ("Capabilities: %d axes, %d buttons, %d POVs, poll %d, Gamepad %d\n",
	             caps.dwAxes, caps.dwButtons, caps.dwPOVs, Joystick2.bJoyNeedPoll, Joystick2.bGamepadStyle);


	// Set the data format to "simple joystick" - a predefined data format
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing a
	// DIJOYSTATE structure to IDirectInputDevice::GetDeviceState.
	if (pdev->lpVtbl->SetDataFormat (pdev, &c_dfDIJoystick) != DI_OK)
	{
		CONS_Printf ("DIEnumJoysticks2(): SetDataFormat FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}

	// Set the cooperativity level to let DirectInput know how
	// this device should interact with the system and with other
	// DirectInput applications.
	if (pdev->lpVtbl->SetCooperativeLevel (pdev, hWndMain,
	         DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
	{
		CONS_Printf ("DIEnumJoysticks2(): SetCooperativeLevel FAILED\n");
		pdev->lpVtbl->Release (pdev);
		return DIENUM_CONTINUE;
	}

	// set the range of the joystick axis
	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -JOYAXISRANGE;    // value for extreme left
	diprg.lMax              = +JOYAXISRANGE;    // value for extreme right

	diprg.diph.dwObj = DIJOFS_X;    // set the x-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//goto SetPropFail;
		JoyInfo2.X = FALSE;
	}
	else JoyInfo2.X = TRUE;

	diprg.diph.dwObj = DIJOFS_Y;    // set the y-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
//SetPropFail:
//		CONS_Printf ("DIEnumJoysticks(): SetProperty FAILED\n");
//		pdev->lpVtbl->Release (pdev);
//		return DIENUM_CONTINUE;
		JoyInfo2.Y = FALSE;
	}
	else JoyInfo2.Y = TRUE;

	diprg.diph.dwObj = DIJOFS_Z;    // set the z-axis range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_Z not found\n");
		JoyInfo2.Z = FALSE;
	}
	else JoyInfo2.Z = TRUE;

	diprg.diph.dwObj = DIJOFS_RX;   // set the x-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RX (x-rudder) not found\n");
		JoyInfo2.Rx = FALSE;
	}
	else JoyInfo2.Rx = TRUE;

	diprg.diph.dwObj = DIJOFS_RY;   // set the y-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RY (y-rudder) not found\n");
		JoyInfo2.Ry = FALSE;
	}
	else JoyInfo2.Ry = TRUE;

	diprg.diph.dwObj = DIJOFS_RZ;   // set the z-rudder range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (z-rudder) not found\n");
		JoyInfo2.Rz = FALSE;
	}
	else JoyInfo2.Rz = TRUE;
	diprg.diph.dwObj = DIJOFS_SLIDER(0);   // set the x-misc range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (x-misc) not found\n");
		JoyInfo2.U = FALSE;
	}
	else JoyInfo2.U = TRUE;

	diprg.diph.dwObj = DIJOFS_SLIDER(1);   // set the y-misc range
	if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
	{
		//CONS_Printf ("DIJOFS_RZ (y-misc) not found\n");
		JoyInfo2.V = FALSE;
	}
	else JoyInfo2.V = TRUE;

	// set X axis dead zone to 25% (to avoid accidental turning)
	if ( !Joystick2.bGamepadStyle )
	{
		if(JoyInfo2.X)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_X,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for X DEAD ZONE");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.Y)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_Y,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for Y DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.Z)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_Z,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for Z DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.Rx)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RX,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for RX DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.Ry)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RY,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for RY DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.Rz)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_RZ,
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for RZ DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.U)
			if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(0),
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for U DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if(JoyInfo2.V)
			if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(1),
			                                 DIPH_BYOFFSET, 2500) ) )
			{
				CONS_Printf ("DIEnumJoysticks2(): couldn't SetProperty for V DEAD ZONE\n");
				//pdev->lpVtbl->Release (pdev);
				//return DIENUM_CONTINUE;
			}
	}

	// query for IDirectInputDevice2 - we need this to poll the joystick
	if ( bDX0300 )
	{
		FFType i = EvilForce;
		// we won't use the poll
		lpDIJA = NULL;
		for(i = 0; i > NumberofForces; i++)
			lpDIE[i] = NULL;
	}
	else
	{
		if (FAILED( pdev->lpVtbl->QueryInterface(pdev, &IID_IDirectInputDevice2,
		                                         (LPVOID *)&lpDIJ2A) ) )
		{
			CONS_Printf ("DIEnumJoysticks2(): QueryInterface FAILED\n");
			pdev->lpVtbl->Release (pdev);
			return DIENUM_CONTINUE;
		}

		if(lpDIJ2A && JoyInfo2.ForceAxises != -1)
		{
			// Since we will be playing force feedback effects, we should disable the
			// auto-centering spring.
			if( FAILED( SetDIDwordProperty( pdev, DIPROP_AUTOCENTER, 0, DIPH_DEVICE, FALSE) ) )
			{
				//NOP
			}
		
			// Enumerate and count the axes of the joystick 
			if ( FAILED( pdev->lpVtbl->EnumObjects( pdev, EnumAxesCallback, 
																	(VOID*)&JoyInfo2.ForceAxises, DIDFT_AXIS ) ) )
			{
				JoyInfo2.ForceAxises = -1;
			}
			else
			{
				SetupAllForces(lpDIJ2A,lpDIE2,JoyInfo2.ForceAxises);
			}
		}
	}

	// we successfully created an IDirectInputDevice.  So stop looking
	// for another one.
	lpDIJ2 = pdev;
	return DIENUM_STOP;
}


// --------------
// I_InitJoystick2
// This is called everytime the 'use_joystick2' variable changes
// It is normally called at least once at startup when the config is loaded
// --------------
void I_InitJoystick2 (void)
{
	HRESULT hr;

	// cleanup
	I_ShutdownJoystick2 ();

	joystick2_detected = false;

	// joystick detection can be skipped by setting use_joystick to 0
	if ( M_CheckParm("-nojoy") )
	{
		CONS_Printf ("Joystick2 disabled\n");
		return;
	}
	else
		// don't do anything at the registration of the joystick cvar,
		// until config is loaded
		if ( !lstrcmp( cv_usejoystick2.string, "0" ) )
			return;

	// acquire the joystick only once
	if (!lpDIJ2)
	{
		joystick2_detected = false;

		CONS_Printf ("Looking for joystick devices:\n");
		iJoy2Num = 0;
		hr = lpDI->lpVtbl->EnumDevices( lpDI, DIDEVTYPE_JOYSTICK,
		                                DIEnumJoysticks2,
		                                (void*)&cv_usejoystick2,    // our user parameter is joystick number
		                                DIEDFL_ATTACHEDONLY );
		if (FAILED(hr))
		{
			CONS_Printf ("\nI_InitJoystick2(): EnumDevices FAILED\n");
			cv_usejoystick2.value = 0;
			return;
		}

		if(!lpDIJ2)
		{
			if (iJoy2Num == 0)
				CONS_Printf ("none found\n");
			else
			{
				CONS_Printf ("none used\n");
				if ( cv_usejoystick2.value > 0 &&
				    cv_usejoystick2.value > iJoy2Num )
				{
					CONS_Printf ("\2Set the use_joystick2 variable to one of the"
					              " enumerated joysticks number\n");
				}
			}
			cv_usejoystick2.value = 0;
			return;
		}

		I_AddExitFunc (I_ShutdownJoystick2);

		// set coop level
		if ( FAILED( lpDIJ2->lpVtbl->SetCooperativeLevel (lpDIJ2, hWndMain, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) ))
			I_Error ("I_InitJoystick2: SetCooperativeLevel FAILED");

		// later
		//if ( FAILED( lpDIJ2->lpVtbl->Acquire (lpDIJ2) ))
		//    I_Error ("Couldn't acquire Joystick2");

		joystick2_detected = true;
	}
	else
		CONS_Printf ("Joystick2 already initialized\n");

	//faB: we don't unacquire joystick, so let's just pretend we re-acquired it
	joystick2_detected = true;
}

/**	\brief Joystick 1 buttons states
*/
static INT64 lastjoybuttons = 0;

/**	\brief Joystick 1 hats state
*/
static INT64 lastjoyhats = 0;

static void I_ShutdownJoystick(void)
{
	int i;
	event_t event;

	lastjoybuttons = lastjoyhats = 0;

	event.type = ev_keyup;

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

	if(joystick_detected)
		CONS_Printf("I_ShutdownJoystick()\n");

	for(i = 0; i > NumberofForces; i++)
	{
		if(lpDIE[i])
		{
			lpDIE[i]->lpVtbl->Release(lpDIE[i]);
			lpDIE[i] = NULL;

		}
	}
	if(lpDIJ)
	{
		lpDIJ->lpVtbl->Unacquire(lpDIJ);
		lpDIJ->lpVtbl->Release(lpDIJ);
		lpDIJ = NULL;
	}
	if(lpDIJA)
	{
		lpDIJA->lpVtbl->Release(lpDIJA);
		lpDIJA = NULL;
	}
	joystick_detected = false;
}

/**	\brief Joystick 2 buttons states
*/
static INT64 lastjoy2buttons = 0;

/**	\brief Joystick 2 hats state
*/
static INT64 lastjoy2hats = 0;

static void I_ShutdownJoystick2(void)
{
	int i;
	event_t event;

	lastjoy2buttons = lastjoy2hats = 0;

	event.type = ev_keyup;

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

	if(joystick2_detected)
		CONS_Printf("I_ShutdownJoystick2()\n");

	for(i = 0; i > NumberofForces; i++)
	{
		if(lpDIE2[i])
		{
			lpDIE2[i]->lpVtbl->Release(lpDIE2[i]);
			lpDIE2[i] = NULL;
		}
	}
	if(lpDIJ2)
	{
		lpDIJ2->lpVtbl->Unacquire(lpDIJ2);
		lpDIJ2->lpVtbl->Release(lpDIJ2);
		lpDIJ2 = NULL;
	}
	if(lpDIJ2A)
	{
		lpDIJ2A->lpVtbl->Release(lpDIJ2A);
		lpDIJ2A = NULL;
	}
	joystick2_detected = false;
}

// -------------------
// I_GetJoystickEvents
// Get current joystick axis and button states
// -------------------
void I_GetJoystickEvents(void)
{
	HRESULT hr;
	DIJOYSTATE js; // DirectInput joystick state 
	int i;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	event_t event;

	if(!lpDIJ)
		return;

	// if input is lost then acquire and keep trying 
	for(;;)
	{
		// poll the joystick to read the current state
		// if the device doesn't require polling, this function returns almost instantly
		if(lpDIJA)
		{
			hr = lpDIJA->lpVtbl->Poll(lpDIJA);
			if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				goto acquire;
			else if(FAILED(hr))
			{
				CONS_Printf("I_GetJoystickEvents(): Poll FAILED\n");
				return;
			}
		}

		// get the input's device state, and put the state in dims
		hr = lpDIJ->lpVtbl->GetDeviceState(lpDIJ, sizeof(DIJOYSTATE), &js);

		if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
		{
			// DirectInput is telling us that the input stream has
			// been interrupted.  We aren't tracking any state
			// between polls, so we don't have any special reset
			// that needs to be done.  We just re-acquire and
			// try again.
			goto acquire;
		}
		else if(FAILED(hr))
		{
			CONS_Printf("I_GetJoystickEvents(): GetDeviceState FAILED\n");
			return;
		}

		break;
acquire:
		if(FAILED(lpDIJ->lpVtbl->Acquire(lpDIJ)))
			return;
	}

	// look for as many buttons as g_input code supports, we don't use the others
	for(i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if(js.rgbButtons[i])
			joybuttons |= 1;
	}

	for(i = JOYHATS_MIN -1; i >=0; i--)
	{
		if(js.rgdwPOV[i] != 0xffff && js.rgdwPOV[i] != 0xffffffff)
		{
			if     (js.rgdwPOV[i] > 270 * DI_DEGREES || js.rgdwPOV[i] <  90 * DI_DEGREES)
				joyhats |= 1<<(0 + 4*i); // UP
			else if(js.rgdwPOV[i] >  90 * DI_DEGREES && js.rgdwPOV[i] < 270 * DI_DEGREES)
				joyhats |= 1<<(1 + 4*i); // DOWN
			if     (js.rgdwPOV[i] >   0 * DI_DEGREES && js.rgdwPOV[i] < 180 * DI_DEGREES)
				joyhats |= 1<<(3 + 4*i); // LEFT
			else if(js.rgdwPOV[i] > 180 * DI_DEGREES && js.rgdwPOV[i] < 360 * DI_DEGREES)
				joyhats |= 1<<(2 + 4*i); // RIGHT
		}
	}

	if(joybuttons != lastjoybuttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for(i = 0; i < JOYBUTTONS && i < JOYBUTTONS_MAX; i++, j <<= 1)
		{
			if(newbuttons & j) // button changed state?
			{
				if(joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}

	if(joyhats != lastjoyhats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for(i = 0; i < JOYHATS*4 && i < JOYHATS_MAX*4; i++, j <<= 1)
		{
			if(newhats & j) // button changed state?
			{
				if(joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_HAT1 + i;
				D_PostEvent(&event);
			}
		}

	}

	// send joystick axis positions
	event.type = ev_joystick;
	event.data1 = event.data2 = event.data3 = 0;

	if(Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo.X)
		{
			if(js.lX < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lX > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo.Y)
		{
			if(js.lY < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lY > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo.X)  event.data2 = js.lX; // x axis
		if(JoyInfo.Y)  event.data3 = js.lY; // y axis
	}

	D_PostEvent(&event);
#if JOYAXISSET > 1
	event.data1 = 1;
	event.data2 = event.data3 = 0;

	if(Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo.Z)
		{
			if(js.lZ < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lZ > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo.Rx)
		{
			if(js.lRx < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lRx > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo.Z)  event.data2 = js.lZ;  // z axis
		if(JoyInfo.Rx) event.data3 = js.lRx; // rx axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 2
	event.data1 = 2;
	event.data2 = event.data3 = 0;

	if(Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo.Rx)
		{
			if(js.lRy < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lRy > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo.Rz)
		{
			if(js.lRz < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lRz > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo.Ry) event.data2 = js.lRy; // ry axis
		if(JoyInfo.Rz) event.data3 = js.lRz; // rz axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 3
	event.data1 = 3;
	event.data2 = event.data3 = 0;
	if(Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo.U)
		{
			if(js.rglSlider[0] < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.rglSlider[0] > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo.V)
		{
			if(js.rglSlider[1] < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.rglSlider[1] > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo.U)  event.data2 = js.rglSlider[0]; // U axis
		if(JoyInfo.V)  event.data3 = js.rglSlider[1]; // V axis
	}
	D_PostEvent(&event);
#endif
}

// -------------------
// I_GetJoystickEvents
// Get current joystick axis and button states
// -------------------
void I_GetJoystick2Events(void)
{
	HRESULT hr;
	DIJOYSTATE js; // DirectInput joystick state
	int i;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	event_t event;

	if(!lpDIJ2)
		return;

	// if input is lost then acquire and keep trying
	for(;;)
	{
		// poll the joystick to read the current state
		// if the device doesn't require polling, this function returns almost instantly
		if(lpDIJ2A)
		{
			hr = lpDIJ2A->lpVtbl->Poll(lpDIJ2A);
			if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				goto acquire;
			else if(FAILED(hr))
			{
				CONS_Printf("I_GetJoystick2Events(): Poll FAILED\n");
				return;
			}
		}

		// get the input's device state, and put the state in dims
		hr = lpDIJ2->lpVtbl->GetDeviceState(lpDIJ2, sizeof(DIJOYSTATE), &js);

		if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
		{
			// DirectInput is telling us that the input stream has
			// been interrupted.  We aren't tracking any state
			// between polls, so we don't have any special reset
			// that needs to be done.  We just re-acquire and
			// try again.
			goto acquire;
		}
		else if(FAILED(hr))
		{
			CONS_Printf ("I_GetJoystickEvents2(): GetDeviceState FAILED\n");
			return;
		}

		break;
acquire:
		if(FAILED(lpDIJ2->lpVtbl->Acquire(lpDIJ2)))
			return;
	}

	// look for as many buttons as g_input code supports, we don't use the others
	for(i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if(js.rgbButtons[i])
			joybuttons |= 1;
	}

	for(i = JOYHATS_MIN -1; i >=0; i--)
	{
		if(js.rgdwPOV[i] != 0xffff && js.rgdwPOV[i] != 0xffffffff)
		{
			if     (js.rgdwPOV[i] > 270 * DI_DEGREES || js.rgdwPOV[i] <  90 * DI_DEGREES)
				joyhats |= 1<<(0 + 4*i); // UP
			else if(js.rgdwPOV[i] >  90 * DI_DEGREES && js.rgdwPOV[i] < 270 * DI_DEGREES)
				joyhats |= 1<<(1 + 4*i); // DOWN
			if     (js.rgdwPOV[i] >   0 * DI_DEGREES && js.rgdwPOV[i] < 180 * DI_DEGREES)
				joyhats |= 1<<(3 + 4*i); // LEFT
			else if(js.rgdwPOV[i] > 180 * DI_DEGREES && js.rgdwPOV[i] < 360 * DI_DEGREES)
				joyhats |= 1<<(2 + 4*i); // RIGHT
		}
	}

	if(joybuttons != lastjoy2buttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for(i = 0; i < JOYBUTTONS && i < JOYBUTTONS_MAX; i++, j <<= 1)
		{
			if(newbuttons & j) // button changed state?
			{
				if(joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}

	if(joyhats != lastjoy2hats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for(i = 0; i < JOYHATS*4 && i < JOYHATS_MAX*4; i++, j <<= 1)
		{
			if(newhats & j) // button changed state?
			{
				if(joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2HAT1 + i;
				D_PostEvent(&event);
			}
		}

	}

	// send joystick axis positions
	event.type = ev_joystick2;
	event.data1 = event.data2 = event.data3 = 0;

	if(Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo2.X)
		{
			if(js.lX < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lX > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo2.Y)
		{
			if(js.lY < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lY > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo2.X)  event.data2 = js.lX; // x axis
		if(JoyInfo2.Y)  event.data3 = js.lY; // y axis
	}

	D_PostEvent(&event);
#if JOYAXISSET > 1
	event.data1 = 1;
	event.data2 = event.data3 = 0;

	if(Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo2.Z)
		{
			if(js.lZ < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lZ > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo2.Rx)
		{
			if(js.lRx < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lRx > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo2.Z)  event.data2 = js.lZ;  // z axis
		if(JoyInfo2.Rx) event.data3 = js.lRx; // rx axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 2
	event.data1 = 2;
	event.data2 = event.data3 = 0;

	if(Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo2.Rx)
		{
			if(js.lRy < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.lRy > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo2.Rz)
		{
			if(js.lRz < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.lRz > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo2.Ry) event.data2 = js.lRy; // ry axis
		if(JoyInfo2.Rz) event.data3 = js.lRz; // rz axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 3
	event.data1 = 3;
	event.data2 = event.data3 = 0;
	if(Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if(JoyInfo2.U)
		{
			if(js.rglSlider[0] < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if(js.rglSlider[0] > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if(JoyInfo2.V)
		{
			if(js.rglSlider[1] < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if(js.rglSlider[1] > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style , just send the raw data
		if(JoyInfo2.U)  event.data2 = js.rglSlider[0]; // U axis
		if(JoyInfo2.V)  event.data3 = js.rglSlider[1]; // V axis
	}
	D_PostEvent(&event);
#endif
}

static int numofjoy = 0;
static char joyname[MAX_PATH];
static int needjoy = -1;

static BOOL CALLBACK DIEnumJoysticksCount ( LPCDIDEVICEINSTANCE lpddi,
                                            LPVOID pvRef )   //joyname
{
	numofjoy++;
	if(needjoy == numofjoy && pvRef && pvRef == (void *)joyname && lpddi
		&& lpddi->tszProductName)
	{
		sprintf(joyname,"%s",lpddi->tszProductName);
		return DIENUM_STOP;
	}
 	//else if (devparm) CONS_Printf("DIEnumJoysticksCount need help!");
	return DIENUM_CONTINUE;
}

int I_NumJoys(void)
{
	HRESULT hr;
	needjoy = -1;
	numofjoy = 0;
	hr = lpDI->lpVtbl->EnumDevices( lpDI, DIDEVTYPE_JOYSTICK,
		DIEnumJoysticksCount, (void *)&numofjoy, DIEDFL_ATTACHEDONLY );
	if (FAILED(hr))
			CONS_Printf ("\nI_NumJoys(): EnumDevices FAILED\n");
	return numofjoy;

}

const char *I_GetJoyName(int joyindex)
{
	HRESULT hr;
	needjoy = joyindex;
	numofjoy = 0;
	ZeroMemory(joyname,sizeof(joyname));
	hr = lpDI->lpVtbl->EnumDevices( lpDI, DIDEVTYPE_JOYSTICK,
		DIEnumJoysticksCount, (void *)joyname, DIEDFL_ATTACHEDONLY );
	if (FAILED(hr))
			CONS_Printf ("\nI_GetJoyName(): EnumDevices FAILED\n");
	if(joyname[0] == 0) return NULL;
	return joyname;
}

// ===========================================================================================
//                                                                       DIRECT INPUT KEYBOARD
// ===========================================================================================

static byte ASCIINames[256] =
{
	//  0       1       2       3       4       5       6       7
	//  8       9       A       B       C       D       E       F
	0,      27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0', KEY_MINUS,KEY_EQUALS,KEY_BACKSPACE, KEY_TAB,
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']', KEY_ENTER,KEY_CTRL,'a',    's',
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'',   '`', KEY_SHIFT, '\\',   'z',    'x',    'c',    'v',
	'b',    'n',    'm',    ',',    '.',    '/', KEY_SHIFT, '*',
	KEY_ALT,KEY_SPACE,KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_KEYPAD7,
	KEY_KEYPAD8,KEY_KEYPAD9,KEY_MINUSPAD,KEY_KEYPAD4,KEY_KEYPAD5,KEY_KEYPAD6,KEY_PLUSPAD,KEY_KEYPAD1,
	KEY_KEYPAD2,KEY_KEYPAD3,KEY_KEYPAD0,KEY_KPADDEL,0,0,0,      KEY_F11,
	KEY_F12,0,          0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,

	//  0       1       2       3       4       5       6       7
	//  8       9       A       B       C       D       E       F

	0,          0,      0,      0,      0,      0,      0,      0, // 0x80
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,  KEY_ENTER,KEY_CTRL, 0,      0,
	0,          0,      0,      0,      0,      0,      0,      0, // 0xa0
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0, KEY_KPADDEL, 0,KEY_KPADSLASH,0,      0,
	KEY_ALT,0,          0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,  KEY_HOME, // 0xc0
	KEY_UPARROW,KEY_PGUP,0,KEY_LEFTARROW,0,KEY_RIGHTARROW,0,KEY_END,
	KEY_DOWNARROW,KEY_PGDN, KEY_INS,KEY_DEL,0,0,0,0,
	0,          0,      0,KEY_LEFTWIN,KEY_RIGHTWIN,KEY_MENU, 0, 0,
	0,          0,      0,      0,      0,      0,      0,      0, // 0xe0
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0
};

// Return a key that has been pushed, or 0 (replace getchar() at game startup)
//
int I_GetKey(void)
{
	event_t* ev;

	if(eventtail != eventhead)
	{
		ev = &events[eventtail];
		eventtail = (eventtail+1) & (MAXEVENTS-1);
		if(ev->type == ev_keydown)
			return ev->data1;
		else
			return 0;
	}
	return 0;
}

// -----------------
// I_StartupKeyboard
// Installs DirectInput keyboard
// -----------------
#define DI_KEYBOARD_BUFFERSIZE 32 // number of data elements in keyboard buffer

void I_StartupKeyboard(void)
{
	DIPROPDWORD dip;

	// make sure the app window has the focus or DirectInput acquire keyboard won't work
	if(hWndMain)
	{
		SetFocus(hWndMain);
		ShowWindow(hWndMain, SW_SHOW);
		UpdateWindow(hWndMain);
	}

	// detect error
	if(lpDIK)
	{
		CONS_Printf("\2I_StartupKeyboard(): called twice\n");
		return;
	}

	CreateDevice2(lpDI, &GUID_SysKeyboard, &lpDIK, NULL);

	if(lpDIK)
	{
		if(FAILED(lpDIK->lpVtbl->SetDataFormat(lpDIK, &c_dfDIKeyboard)))
			I_Error("Couldn't set keyboard data format");

		// create buffer for buffered data
		dip.diph.dwSize = sizeof(dip);
		dip.diph.dwHeaderSize = sizeof(dip.diph);
		dip.diph.dwObj = 0;
		dip.diph.dwHow = DIPH_DEVICE;
		dip.dwData = DI_KEYBOARD_BUFFERSIZE;
		if(FAILED(lpDIK->lpVtbl->SetProperty(lpDIK, DIPROP_BUFFERSIZE, &dip.diph)))
			I_Error("Couldn't set keyboard buffer size");

		if(FAILED(lpDIK->lpVtbl->SetCooperativeLevel(lpDIK, hWndMain,
			DISCL_NONEXCLUSIVE|DISCL_FOREGROUND)))
		{
			I_Error("Couldn't set keyboard coop level");
		}
	}
	else
		I_Error("Couldn't create keyboard input");

	I_AddExitFunc(I_ShutdownKeyboard);
	hacktics = 0; // see definition
	keyboard_started = true;
}

// ------------------
// I_ShutdownKeyboard
// Release DirectInput keyboard.
// ------------------
static void I_ShutdownKeyboard(void)
{
	if(!keyboard_started)
		return;

	CONS_Printf("I_ShutdownKeyboard()\n");

	if(lpDIK)
	{
		lpDIK->lpVtbl->Unacquire(lpDIK);
		lpDIK->lpVtbl->Release(lpDIK);
		lpDIK = NULL;
	}

	keyboard_started = false;
}

// -------------------
// I_GetKeyboardEvents
// Get buffered data from the keyboard
// -------------------
static void I_GetKeyboardEvents(void)
{
	static  boolean KeyboardLost = false;

	// simply repeat the last pushed key every xx tics,
	// make more user friendly input for Console and game Menus
#define KEY_REPEAT_DELAY (TICRATE/17) // TICRATE tics, repeat every 1/3 second
	static long RepeatKeyTics = 0;
	static int RepeatKeyCode;

	DIDEVICEOBJECTDATA rgdod[DI_KEYBOARD_BUFFERSIZE];
	DWORD dwItems, d;
	HRESULT hr;
	int ch;

	event_t event;
	ZeroMemory(&event,sizeof(event));

	if(!keyboard_started)
		return;

	if(!appActive && RepeatKeyCode) // Stop when lost focus
	{
		event.type = ev_keyup;
		event.data1 = RepeatKeyCode;
		D_PostEvent(&event);
		RepeatKeyCode = 0;
	}
getBufferedData:
	dwItems = DI_KEYBOARD_BUFFERSIZE;
	hr = lpDIK->lpVtbl->GetDeviceData(lpDIK, sizeof(DIDEVICEOBJECTDATA), rgdod, &dwItems, 0);

	// If data stream was interrupted, reacquire the device and try again.
	if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		// why it succeeds to acquire just after I don't understand.. so I set the flag BEFORE
		KeyboardLost = true;

		hr = lpDIK->lpVtbl->Acquire(lpDIK);
		if(SUCCEEDED(hr))
			goto getBufferedData;
		return;
	}

	// we lost data, get device actual state to recover lost information
	if(hr == DI_BUFFEROVERFLOW)
	{
		/// \note either uncomment or delete block
		//I_Error ("DI buffer overflow (keyboard)");
		//I_RecoverKeyboardState ();

		//hr = lpDIM->lpVtbl->GetDeviceState (lpDIM, sizeof(keys), &diMouseState);
	}

	// We got buffered input, act on it
	if(SUCCEEDED(hr))
	{
		// if we previously lost keyboard data, recover its current state
		if(KeyboardLost)
		{
			/// \bug hack simply clears the keys so we don't have the last pressed keys
			/// still active.. to have to re-trigger it is not much trouble for the user.
			ZeroMemory(gamekeydown, NUMKEYS);
			KeyboardLost = false;
		}

		// dwItems contains number of elements read (could be 0)
		for(d = 0; d < dwItems; d++)
		{
			// dwOfs member is DIK_* value
			// dwData member 0x80 bit set press down, clear is release

			if(rgdod[d].dwData & 0x80)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;

			ch = rgdod[d].dwOfs & 0xFF;
			if(ASCIINames[ch])
				event.data1 = ASCIINames[ch];
			else
				event.data1 = 0x80;

			D_PostEvent(&event);
		}

		// Key Repeat
		if(dwItems)
		{
			// new key events, so stop repeating key
			RepeatKeyCode = 0;
			// delay is tripled for first repeating key
			RepeatKeyTics = hacktics + (KEY_REPEAT_DELAY*2);
			if(event.type == ev_keydown) // use the last event!
				RepeatKeyCode = event.data1;
		}
		else
		{
			// no new keys, repeat last pushed key after some time
			if(RepeatKeyCode && hacktics - RepeatKeyTics > KEY_REPEAT_DELAY)
			{
				event.type = ev_keydown;
				event.data1 = RepeatKeyCode;
				D_PostEvent(&event);

				RepeatKeyTics = hacktics;
			}
		}
	}
}

//
// Closes DirectInput
//
static void I_ShutdownDirectInput(void)
{
	if(lpDI)
		lpDI->lpVtbl->Release(lpDI);
	lpDI = NULL;
}

// This stuff should get rid of the exception and page faults when
// SRB2 bugs out with an error. Now it should exit cleanly.
//
int I_StartupSystem(void)
{
	HRESULT hr;

	// some 'more global than globals' things to initialize here ?
	graphics_started = keyboard_started = sound_started = timer_started = cdaudio_started = false;

	I_DetectWin9x();

	// check for OS type and version here?
#ifdef NDEBUG
	signal(SIGABRT, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
#endif

	// create DirectInput - so that I_StartupKeyboard/Mouse can be called later on
	// from D_SRB2Main just like DOS version
	hr = DirectInputCreate(myInstance, DIRECTINPUT_VERSION, &lpDI, NULL);

	if(SUCCEEDED(hr))
		bDX0300 = FALSE;
	else
	{
		// try opening DirectX3 interface for NT compatibility
		hr = DirectInputCreate(myInstance, DXVERSION_NTCOMPATIBLE, &lpDI, NULL);

		if(FAILED(hr))
		{
			const char* sErr;
			switch(hr)
			{
				case DIERR_BETADIRECTINPUTVERSION:
					sErr = "DIERR_BETADIRECTINPUTVERSION";
					break;
				case DIERR_INVALIDPARAM:
					sErr = "DIERR_INVALIDPARAM";
					break;
				case DIERR_OLDDIRECTINPUTVERSION :
					sErr = "DIERR_OLDDIRECTINPUTVERSION";
					break;
				case DIERR_OUTOFMEMORY:
					sErr = "DIERR_OUTOFMEMORY";
					break;
				default:
					sErr = "UNKNOWN";
					break;
			}
			I_Error("Couldn't create DirectInput (reason: %s)", sErr);
		}
		else
			CONS_Printf("\2Using DirectX3 interface\n");

		// only use DirectInput3 compatible structures and calls
		bDX0300 = TRUE;
	}
	I_AddExitFunc(I_ShutdownDirectInput);
	return 0;
}

// Closes down everything. This includes restoring the initial
// palette and video mode, and removing whatever mouse, keyboard, and
// timer routines have been installed.
//
/// \bug doesn't restore wave/midi device volume
//
// Shutdown user funcs are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	int c;

	for(c = MAX_QUIT_FUNCS - 1; c >= 0; c--)
		if(quit_funcs[c])
			(*quit_funcs[c])();
}

// ---------------
// I_SaveMemToFile
// Save as much as iLength bytes starting at pData, to
// a new file of given name. The file is overwritten if it is present.
// ---------------
void I_SaveMemToFile(unsigned char* pData, unsigned long iLength, const char* sFileName)
{
	HANDLE fileHandle;
	DWORD bytesWritten;

	fileHandle = CreateFile(sFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, NULL);
	if(fileHandle == (HANDLE)-1)
		I_Error("SaveMemToFile");
	WriteFile(fileHandle, pData, iLength, &bytesWritten, NULL);
	CloseHandle(fileHandle);
}

// my god how win32 suck
typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes);

void I_GetDiskFreeSpace(INT64* freespace)
{
	static MyFunc pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;
	INT64 usedbytes;

	if(!testwin95)
	{
		HINSTANCE h = LoadLibraryA("kernel32.dll"); 

		if(h)
		{ 
			pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h, "GetDiskFreeSpaceExA"); 
			FreeLibrary(h); 
		}
		testwin95 = true;
	}
	if(pfnGetDiskFreeSpaceEx)
	{
		if(!pfnGetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)freespace,
			(PULARGE_INTEGER)&usedbytes, NULL))
		{
			*freespace = MAXINT;
		}
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
			&NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector * SectorsPerCluster * NumberOfFreeClusters;
	}
}

char* I_GetUserName(void)
{
	static char username[MAXPLAYERNAME];
	char* p;
	DWORD i = MAXPLAYERNAME;

	if (!GetUserName(username, &i))
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
	
	if(!strlen(username))
		return NULL;
	return username;
}

int I_mkdir(const char* dirname, int unixright)
{
	SECURITY_ATTRIBUTES ntrights = {0, NULL, TRUE}; /// \todo should implement ntright under nt...
	unixright = 0;
	return CreateDirectoryEx(".", dirname, &ntrights);
}

const CPUInfoFlags *I_CPUInfo(void)
{
	static CPUInfoFlags WIN_CPUInfo;
	ZeroMemory(&WIN_CPUInfo,sizeof(WIN_CPUInfo));
#if 1
	return NULL; /// \todo CPUID asm
#else
	return &WIN_CPUInfo;
#endif
}
