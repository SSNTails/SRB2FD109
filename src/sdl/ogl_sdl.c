// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ogl_sdl.c,v 1.6 2001/06/25 20:08:06 bock Exp $
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
// $Log: ogl_sdl.c,v $
// Revision 1.6  2001/06/25 20:08:06  bock
// Fix bug (BSD?) with color depth > 16 bpp
//
// Revision 1.5  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.3  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.2  2000/09/10 10:56:01  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//-----------------------------------------------------------------------------
/// \file
/// \brief SDL specific part of the OpenGL API for SRB2

#include <SDL/SDL.h>

#include "../doomdef.h"
#include "../hardware/r_opengl/r_opengl.h"
#include "ogl_sdl.h"
#include "../i_system.h"

#ifdef DEBUG_TO_FILE
#include <stdarg.h>
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef SDLIO
#include <fcntl.h>
#endif
#endif

/**	\brief SDL video display surface
*/
SDL_Surface *vidSurface = NULL;
int oglflags = 0;

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed videeo mode

	
*/
boolean OglSdlSurface(int w, int h, boolean isFullscreen)
{
	int cbpp;
	char *glvendor, *glrenderer, *glversion;
	cbpp = (cv_scr_depth.value<16)?16:cv_scr_depth.value;

	if(vidSurface)
	{
		//Alam: SDL_Video system free vidSurface for me
#ifdef VOODOOSAFESWITCHING
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
	}

	cbpp = SDL_VideoModeOK(w, h, cbpp, SDL_OPENGL|(isFullscreen?SDL_FULLSCREEN:SDL_RESIZABLE));
	if (cbpp < 16)
		return true; //Alam: Let just say we did, ok?
	vidSurface = SDL_SetVideoMode(w, h, cbpp, SDL_OPENGL|(isFullscreen?SDL_FULLSCREEN:SDL_RESIZABLE));
	if(!vidSurface)
		return false;

#ifdef __CYGWIN__
	glvendor = "";
	glrenderer = "";
	glversion = "";
	gl_extensions = "";
#else
	glvendor = strdup((const char *)glGetString(GL_VENDOR));
	// Get info and extensions.
	//BP: why don't we make it earlier ?
	//Hurdler: we cannot do that before intialising gl context
	glrenderer = strdup((const char *)glGetString(GL_RENDERER));
	glversion = strdup((const char *)glGetString(GL_VERSION));
	gl_extensions = glGetString(GL_EXTENSIONS);
#endif
	DBG_Printf("Vendor     : %s\n", glvendor );
	DBG_Printf("Renderer   : %s\n", glrenderer );
	DBG_Printf("Version    : %s\n", glversion );
	DBG_Printf("Extensions : %s\n", gl_extensions );
	oglflags = 0;

#ifndef __CYGWIN__
#if defined(_WIN32) || defined(_WIN64)
	// BP: disable advenced feature that don't work on somes hardware
	// Hurdler: Now works on G400 with bios 1.6 and certified drivers 6.04
	if( strstr(glrenderer, "810" ) )   oglflags |= GLF_NOZBUFREAD;
#elif defined(LINUX)
	// disable advanced features not working on somes hardware
	if( strstr(glrenderer, "G200" ) )   oglflags |= GLF_NOTEXENV;
	if( strstr(glrenderer, "G400" ) )   oglflags |= GLF_NOTEXENV;
#endif
	free(glvendor);
	free(glrenderer);
	free(glversion);
#endif
	DBG_Printf("oglflags   : 0x%X\n", oglflags );

#ifdef USE_PALETTED_TEXTURE
	usePalettedTexture = isExtAvailable("GL_EXT_paletted_texture");
	if(usePalettedTexture)
	{
		glColorTableEXT=(PFNGLCOLORTABLEEXTPROC)SDL_GL_GetProcAddress("glColorTableEXT");
		if (glColorTableEXT==NULL)
			usePalettedTexture = 0;
		else
			usePalettedTexture = 1;
	}
#ifdef PARANOIA
	if(usePalettedTexture)
		I_OutputMsg("r_opengl.c: USE_PALETED_TEXTURE works\n");
#endif
#endif

	SetModelView(w, h);
	SetStates();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	HWR_Startup();
#ifdef KOS_GL_COMPATIBILITY
	textureformatGL = GL_ARGB1555;
#else
	textureformatGL = (cbpp > 16)?GL_RGBA:GL_RGB5_A1;
#endif

#ifdef DEBUG_TO_FILE
	logstream = open("ogllog.txt",O_WRONLY|O_CREAT,0666);
#endif
	return true;
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for videi sync

	\return	void

	
*/
void OglSdlFinishUpdate(boolean vidwait)
{
	vidwait = 0;
	SDL_GL_SwapBuffers();
}

/**	\brief Shutdown OpenGL/SDL system
*/
void OglSdlShutdown(void)
{
	vidSurface = NULL; //Alam: SDL_Video system free vidSurface for me
#ifdef DEBUG_TO_FILE
	if(logstream!=-1) close(logstream);
#endif
}

EXPORT void HWRAPI( OglSdlSetPalette) (RGBA_t *palette, RGBA_t *gamma)
{
	int i = -1;
	byte redgamma = gamma->s.red, greengamma = gamma->s.green, bulegamma = gamma->s.blue;

	i = SDL_SetGamma(byteasfloat(redgamma), byteasfloat(greengamma), byteasfloat(bulegamma));
	if(i == 0) redgamma = greengamma = bulegamma = 0x7F; //Alam: cool
	for (i=0; i<256; i++)
	{
		myPaletteData[i].s.red   = (byte)MIN((palette[i].s.red   * redgamma)  /127, 255);
		myPaletteData[i].s.green = (byte)MIN((palette[i].s.green * greengamma)/127, 255);
		myPaletteData[i].s.blue  = (byte)MIN((palette[i].s.blue  * bulegamma) /127, 255);
		myPaletteData[i].s.alpha = palette[i].s.alpha;
	}
#ifdef USE_PALETTED_TEXTURE
	if (usePalettedTexture && glColorTableEXT)
	{
		for (i=0; i<256; i++)
		{
			palette_tex[(3*i)+0] = palette[i].s.red;
			palette_tex[(3*i)+1] = palette[i].s.green;
			palette_tex[(3*i)+2] = palette[i].s.blue;
		}
		glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
	}
#endif
	// on a chang�de palette, il faut recharger toutes les textures
	// jaja, und noch viel mehr ;-)
	Flush();
}
