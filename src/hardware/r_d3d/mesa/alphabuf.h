/* $Id: alphabuf.h,v 3.2 1998/03/28 03:57:13 brianp Exp $ */

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
 * $Log: alphabuf.h,v $
 * Revision 3.2  1998/03/28 03:57:13  brianp
 * added CONST macro to fix IRIX compilation problems
 *
 * Revision 3.1  1998/03/27 04:40:28  brianp
 * added a few const keywords
 *
 * Revision 3.0  1998/01/31 20:45:34  brianp
 * initial rev
 *
 */



#ifndef ALPHABUF_H
#define ALPHABUF_H


#include "types.h"


extern void gl_alloc_alpha_buffers( GLcontext *ctx );


extern void gl_clear_alpha_buffers( GLcontext *ctx );


extern void gl_write_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                                 CONST GLubyte rgba[][4], const GLubyte mask[] );


extern void gl_write_mono_alpha_span( GLcontext *ctx,
                                      GLuint n, GLint x, GLint y,
                                      GLubyte alpha, const GLubyte mask[] );



extern void gl_write_alpha_pixels( GLcontext* ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] );


extern void gl_write_mono_alpha_pixels( GLcontext* ctx,
                                        GLuint n, const GLint x[],
                                        const GLint y[], GLubyte alpha,
                                        const GLubyte mask[] );


extern void  gl_read_alpha_span( GLcontext* ctx,
                                 GLuint n, GLint x, GLint y,
                                 GLubyte rgba[][4] );


extern void gl_read_alpha_pixels( GLcontext* ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] );


#endif

