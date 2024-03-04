/* $Id: accum.h,v 3.0 1998/01/31 20:46:29 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1997  Brian Paul
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
 * $Log: accum.h,v $
 * Revision 3.0  1998/01/31 20:46:29  brianp
 * initial rev
 *
 */


#ifndef ACCUM_H
#define ACCUM_H


#include "types.h"


extern void gl_alloc_accum_buffer( GLcontext *ctx );


extern void gl_Accum( GLcontext *ctx, GLenum op, GLfloat value );


extern void gl_ClearAccum( GLcontext *ctx, GLfloat red, GLfloat green,
                           GLfloat blue, GLfloat alpha );


extern void gl_clear_accum_buffer( GLcontext *ctx );


#endif
