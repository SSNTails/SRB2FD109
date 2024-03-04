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

/// \file
/// \brief Tool for dynamic referencing of hardware rendering functions
///
///	Declaration and definition of the HW rendering 
///	functions do have the same name. Originally, the
///	implementation was stored in a separate library.
///	For SDL, we need some function to return the addresses,
///	otherwise we have a conflict with the compiler.

#include "hwsym_sdl.h"
#include "../doomdef.h"

#include <SDL/SDL.h>

#if (SDL_MAJOR_VERSION > 1) || (SDL_MINOR_VERSION >= 3) || (SDL_MINOR_VERSION == 2 && SDL_PATCHLEVEL >= 6)
#include <SDL/SDL_loadso.h> // 1.2.6+
#else //NOLOADSO
#define NOLOADSO
#endif

#define  _CREATE_DLL_  // necessary for Unix AND Windows

#ifdef HWRENDER
#include "../hardware/hw_drv.h"
#include "ogl_sdl.h"
#endif

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"
#endif

//
// 
/**	\brief	The *hwSym function

	Stupid function to return function addresses

	\param	funcName	the name of the function
	\param	handle	an object to look in(NULL for self)

	\return	void

	
*/
//
void *hwSym(const char *funcName,void *handle)
{
	void *funcPointer = NULL;
	if(0 == strcmp("FinishUpdate", funcName))
		return funcPointer; //&FinishUpdate;
#ifdef HWRENDER
	else if(0 == strcmp("Init", funcName))
		funcPointer = &Init;
	else if(0 == strcmp("Draw2DLine", funcName))
		funcPointer = &Draw2DLine;
	else if(0 == strcmp("DrawPolygon", funcName))
		funcPointer = &DrawPolygon;
	else if(0 == strcmp("SetBlend", funcName))
		funcPointer = &SetBlend;
	else if(0 == strcmp("ClearBuffer", funcName))
		funcPointer = &ClearBuffer;
	else if(0 == strcmp("SetTexture", funcName))
		funcPointer = &SetTexture;
	else if(0 == strcmp("ReadRect", funcName))
		funcPointer = &ReadRect;
	else if(0 == strcmp("GClipRect", funcName))
		funcPointer = &GClipRect;
	else if(0 == strcmp("ClearMipMapCache", funcName))
		funcPointer = &ClearMipMapCache;
	else if(0 == strcmp("SetSpecialState", funcName))
		funcPointer = &SetSpecialState;
	else if(0 == strcmp("SetPalette", funcName))
		funcPointer = &OglSdlSetPalette;
	else if(0 == strcmp("GetTextureUsed", funcName))
		funcPointer = &GetTextureUsed;
	else if(0 == strcmp("DrawMD2", funcName))
		funcPointer = &DrawMD2;
	else if(0 == strcmp("SetTransform", funcName))
		funcPointer = &SetTransform;
	else if(0 == strcmp("GetRenderVersion", funcName))
		funcPointer = &GetRenderVersion;
#endif
#ifdef STATIC3DS
	else if(0 == strcmp("Startup", funcName))
		funcPointer = &Startup;
	else if(0 == strcmp("Shutdown", funcName))
		funcPointer = &Shutdown;
	else if(0 == strcmp("Add3DSource", funcName))
		funcPointer = &Add3DSource;
	else if (0 == strcmp("Add2DSource", funcName))
		funcPointer = &Add2DSource;
	else if(0 == strcmp("StopSource", funcName))
		funcPointer = &StopSource;
	else if(0 == strcmp("StartSource", funcName))
		funcPointer = &StartSource;
	else if(0 == strcmp("GetHW3DSVersion", funcName))
		funcPointer = &GetHW3DSVersion;
	else if(0 == strcmp("BeginFrameUpdate", funcName))
		funcPointer = &BeginFrameUpdate;
	else if(0 == strcmp("EndFrameUpdate", funcName))
		funcPointer = &EndFrameUpdate;
	else if(0 == strcmp("IsPlaying", funcName))
		funcPointer = &IsPlaying;
	else if(0 == strcmp("UpdateListener", funcName))
		funcPointer = &UpdateListener;
	else if(0 == strcmp("UpdateListener2", funcName))
		funcPointer = &UpdateListener2;
	else if(0 == strcmp("SetGlobalSfxVolume", funcName))
		funcPointer = &SetGlobalSfxVolume;
	else if(0 == strcmp("SetCone", funcName))
		funcPointer = &SetCone;
	else if(0 == strcmp("Update2DSourceParms", funcName))
		funcPointer = &Update2DSoundParms;
	else if(0 == strcmp("Update3DSource", funcName))
		funcPointer = &Update3DSource;
	else if(0 == strcmp("UpdateSourceVolume", funcName))
		funcPointer = &UpdateSourceVolume;
	else if(0 == strcmp("Reload3DSource", funcName))
		funcPointer = &Reload3DSource;
	else if(0 == strcmp("KillSource", funcName))
		funcPointer = &KillSource;
#endif
#ifdef NOLOADSO
	else
		funcPointer = handle;
#else
	else if(handle)
		funcPointer = SDL_LoadFunction(handle,funcName);
#endif
	return funcPointer;
}

/**	\brief	The *hwOpen function

	\param	hwfile	Open a handle to the SO

	\return	Handle to SO

	
*/

void *hwOpen(const char *hwfile)
{
#ifdef NOLOADSO
	hwfile = NULL;
	return NULL;
#else
	void *tempso = NULL;
	tempso = SDL_LoadObject(hwfile);
	if(!tempso) CONS_Printf("hwOpen: Couldn't open object %s because %s\n",hwfile,SDL_GetError());
	return tempso;
#endif
}

/**	\brief	The hwClose function

	\param	handle	Close the handle of the SO

	\return	void

	
*/

void hwClose(void *handle)
{
#ifdef NOLOADSO
	handle = NULL;
#else
	SDL_UnloadObject(handle);
#endif
}
