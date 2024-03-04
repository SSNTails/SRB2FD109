/* $Id: drawpix.h,v 3.0 1998/01/31 20:51:03 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: drawpix.h,v $
 * Revision 3.0  1998/01/31 20:51:03  brianp
 * initial rev
 *
 */


#ifndef DRAWPIXELS_H
#define DRAWPIXELS_H


#include "types.h"


extern GLboolean
gl_direct_DrawPixels( GLcontext *ctx, 
                      const struct gl_pixelstore_attrib *unpack,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type, const GLvoid *pixels );


#if 000
extern void gl_DrawPixels( GLcontext *ctx, GLsizei width, GLsizei height,
                           GLenum format, GLenum type, const GLvoid *pixels );
#endif


extern void gl_DrawPixels( GLcontext *ctx, struct gl_image *image );


#endif
