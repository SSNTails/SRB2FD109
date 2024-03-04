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
/// \brief Main program, simply calls D_SRB2Main and D_SRB2Loop, the high level loop.

#include "../doomdef.h"

#include "../m_argv.h"
#include "../d_main.h"
#include "../i_system.h"

#ifdef __GNUC__
#include <unistd.h>
#endif

#ifdef SDLMAIN
#include <SDL/SDL_main.h>
#endif

#ifdef LOGMESSAGES
#ifdef SDLIO
#undef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE NULL
SDL_RWops* logstream = INVALID_HANDLE_VALUE;
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

/**	\brief low-level file handle to log file
*/
int logstream = INVALID_HANDLE_VALUE;
#endif
#endif

#ifndef DOXYGEN
#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0
#endif
#endif

#ifdef _arch_dreamcast
#include <arch/arch.h>
KOS_INIT_FLAGS(INIT_DEFAULT);
#endif

/**	\brief	The main function

	\param	argc	number of arg
	\param	*argv	string table

	\return	int

	
*/
int main(int argc, char **argv)
{
	const char* logdir = NULL;
	myargc = argc;
	myargv = argv; /// TODO pull out path to exe from this string

	logdir = D_Home();
#ifdef LOGMESSAGES
#ifdef SDLIO
#ifdef _WIN32_WCE
	logstream = SDL_RWFromFile("/Storage Card/SRB2DEMO/srb2log.txt", "a+");
#else
	if(logdir)
	{
		logstream = SDL_RWFromFile(va("%s"/DEFAULTDIR"/srb2log.txt",logdir), "a+");
	}
	else
		logstream = SDL_RWFromFile("./srb2log.txt", "a+");
#endif	
#else
	//unlink("./srb2log.txt"); //Alam: poo! why can't open TRUNC the log file?
	//Hurdler: only write log if we have the permission in the current directory
	logstream = open("./srb2log.txt", O_WRONLY|(0?O_APPEND:O_TRUNC)|O_CREAT|O_SEQUENTIAL|O_TEXT,0666);
	if (logstream < 0)
	{
		logstream = INVALID_HANDLE_VALUE; // so we haven't to change the current source code
	}
#endif
#endif
	//CONS_Printf ("I_StartupSystem() ...\n");
	I_StartupSystem();
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#if defined(__MINGW32__) || defined(__MINGW64__)
	LoadLibrary("exchndl.dll");
#endif
#ifdef USEASM
	{
		// Disable write-protection of code segment
		DWORD OldRights;
		BYTE *pBaseOfImage = (BYTE *)GetModuleHandle(0);
		IMAGE_OPTIONAL_HEADER *pHeader = (IMAGE_OPTIONAL_HEADER *)
			(pBaseOfImage + ((IMAGE_DOS_HEADER*)pBaseOfImage)->e_lfanew +
			sizeof(IMAGE_NT_SIGNATURE) + sizeof(IMAGE_FILE_HEADER));
		if(!VirtualProtect(pBaseOfImage+pHeader->BaseOfCode,pHeader->SizeOfCode,PAGE_EXECUTE_READWRITE,&OldRights))
			I_Error ("Could not make code writable\n");
	}
#endif
#endif
	// startup SRB2
	CONS_Printf ("Setting up SRB2...\n");
	D_SRB2Main();
	CONS_Printf ("Entering main game loop...\n");
	// never return
	D_SRB2Loop();

	// return to OS
	return 0;
}
