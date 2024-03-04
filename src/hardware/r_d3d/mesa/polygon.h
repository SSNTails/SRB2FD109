/* $Id: polygon.h,v 3.1 1998/08/21 02:43:30 brianp Exp $ */

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
 * $Log: polygon.h,v $
 * Revision 3.1  1998/08/21 02:43:30  brianp
 * implemented true packing/unpacking of polygon stipples
 *
 * Revision 3.0  1998/01/31 21:01:27  brianp
 * initial rev
 *
 */


#ifndef POLYGON_H
#define POLYGON_H


#include "types.h"


extern void gl_CullFace( GLcontext *ctx, GLenum mode );

extern void gl_FrontFace( GLcontext *ctx, GLenum mode );

extern void gl_PolygonMode( GLcontext *ctx, GLenum face, GLenum mode );

extern void gl_PolygonOffset( GLcontext *ctx,
                              GLfloat factor, GLfloat units );

extern void gl_PolygonStipple( GLcontext *ctx, const GLuint pattern[32] );

extern void gl_GetPolygonStipple( GLcontext *ctx, GLubyte *mask );


#endif

