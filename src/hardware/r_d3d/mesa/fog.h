/* $Id: fog.h,v 3.0 1998/01/31 20:52:49 brianp Exp $ */

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
 * $Log: fog.h,v $
 * Revision 3.0  1998/01/31 20:52:49  brianp
 * initial rev
 *
 */



#ifndef FOG_H
#define FOG_H


#include "types.h"


extern void gl_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params );


extern void gl_fog_rgba_vertices( const GLcontext *ctx, GLuint n,
                                  GLfloat v[][4], GLubyte color[][4] );

extern void gl_fog_ci_vertices( const GLcontext *ctx, GLuint n,
                                GLfloat v[][4], GLuint indx[] );


extern void gl_fog_rgba_pixels( const GLcontext *ctx,
                                GLuint n, const GLdepth z[],
                                GLubyte rgba[][4] );

extern void gl_fog_ci_pixels( const GLcontext *ctx,
                              GLuint n, const GLdepth z[], GLuint indx[] );


#endif
