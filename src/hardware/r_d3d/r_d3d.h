// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_d3d.h,v 1.16 2001/03/09 21:53:56 metzgermeister Exp $
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
// $Log: r_d3d.h,v $
//-----------------------------------------------------------------------------
/// \file
/// \brief Direct3D API for Doom Legacy


#ifndef _R_OPENGL_H_
#define _R_OPENGL_H_

#define  _CREATE_DLL_  // necessary for Unix AND Windows
#include "../../doomdef.h"

#include "../hw_drv.h"

#include "mesa/GL/gl.h"
#include "mesa/GL/glu.h"

#ifdef __MSC_VER__
#pragma warning (disable : 4200)
#endif


// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#define MIN(x,y) ( ((x)<(y)) ? (x) : (y) )
#define MAX(x,y) ( ((x)>(y)) ? (x) : (y) )

#undef DEBUG_TO_FILE            // maybe defined in previous *.h
#define DEBUG_TO_FILE           // output debugging msgs to d3dlog.txt

#ifndef MINI_GL_COMPATIBILITY
//    #define USE_PALETTED_TEXTURE
#define DRIVER_STRING "HWRAPI Init(): SRB2 D3D renderer" // Logan
#endif

#if !defined(_WIN32) && !defined(_WIN64)
typedef unsigned int    DWORD;
typedef char*           LPCTSTR;
typedef int             HANDLE;
#endif

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

void DBG_Printf(const LPCTSTR lpFmt, ...) FUNCPRINTF;                                          
void Flush(void);
int  isExtAvailable(char *extension);
boolean SetupPixelFormat(int WantColorBits, int WantStencilBits, int WantDepthBits);
void SetModelView(GLint w, GLint h);
void SetStates(void);

// ==========================================================================
//                                                                     GLOBAL
// ==========================================================================

extern const GLubyte    *gl_extensions;
extern RGBA_t           myPaletteData[];
#ifndef SDLIO
#ifdef SDL
extern int              logstream;
#else
extern HANDLE           logstream;
#endif
#endif
extern GLint            screen_width;
extern GLint            screen_height;
extern GLbyte           screen_depth;
extern int              oglflags;
extern GLint            textureformatGL;

typedef enum {
    GLF_NOZBUFREAD = 0x01,
    GLF_NOTEXENV   = 0x02,
} oglflags_t;

#endif
