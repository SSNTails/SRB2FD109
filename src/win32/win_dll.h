// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_dll.h,v 1.3 2001/04/04 20:19:07 judgecutor Exp $
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
//
//
// $Log: win_dll.h,v $
// Revision 1.3  2001/04/04 20:19:07  judgecutor
// Added support for the 3D Sound
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//-----------------------------------------------------------------------------
/// \file
/// \brief load/unload a DLL at run-time

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>

// this is made extern so win_dbg.c can log the load address
// of the dynamically loaded DLL functions
typedef struct loadfunc_s {
	const char*   fnName;
	void*   fnPointer;
} loadfunc_t;
#ifdef HWRENDER
extern HINSTANCE  hwdInstance;
extern loadfunc_t hwdFuncTable[];
#endif

#ifdef HW3SOUND
extern HINSTANCE  hwsInstance;
extern loadfunc_t hwsFuncTable[];
#endif


HINSTANCE LoadDLL (const char* dllName, loadfunc_t* funcTable);
void UnloadDLL (HINSTANCE* pInstance);

#ifdef HWRENDER
BOOL Init3DDriver (const char* dllName);
void Shutdown3DDriver (void);
#endif

#ifdef HW3SOUND
BOOL Init3DSDriver(const char *dllName);
void Shutdown3DSDriver(void);
#endif
