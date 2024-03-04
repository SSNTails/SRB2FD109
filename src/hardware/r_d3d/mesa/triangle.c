/* $Id: triangle.c,v 3.10 1998/07/09 03:16:24 brianp Exp $ */

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
 * $Log: triangle.c,v $
 * Revision 3.10  1998/07/09 03:16:24  brianp
 * added Marten's latest texture triangle code
 *
 * Revision 3.9  1998/06/24 02:52:05  brianp
 * fixed texture coord interp problems.  lots of code clean-up
 *
 * Revision 3.8  1998/06/18 02:49:35  brianp
 * added Marten Stromberg's optimized textured triangle code
 *
 * Revision 3.7  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.6  1998/05/31 23:50:36  brianp
 * cleaned up a few Solaris compiler warnings
 *
 * Revision 3.5  1998/04/01 02:58:52  brianp
 * applied Miklos Fazekas's 3-31-98 Macintosh changes
 *
 * Revision 3.4  1998/03/27 04:26:44  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/20 04:54:02  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/04 00:44:29  brianp
 * fixed casts and conditional expression problems for Amiga StormC compiler
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:05:24  brianp
 * initial rev
 *
 */


/*
 * Triangle rasterizers
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "macros.h"
#include "span.h"
#include "texstate.h"
#include "triangle.h"
#include "types.h"
#include "vb.h"
#endif


/*
 * Put triangle in feedback buffer.
 */
static void feedback_triangle( GLcontext *ctx,
                               GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat color[4];
   GLuint i;
   GLuint texSet = ctx->Texture.CurrentTransformSet;

   FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POLYGON_TOKEN );
   FEEDBACK_TOKEN( ctx, (GLfloat) 3 );        /* three vertices */

   if (ctx->Light.ShadeModel==GL_FLAT) {
      /* flat shading - same color for each vertex */
      color[0] = (GLfloat) VB->Color[pv][0] / 255.0;
      color[1] = (GLfloat) VB->Color[pv][1] / 255.0;
      color[2] = (GLfloat) VB->Color[pv][2] / 255.0;
      color[3] = (GLfloat) VB->Color[pv][3] / 255.0;
   }

   for (i=0;i<3;i++) {
      GLfloat x, y, z, w;
      GLfloat tc[4];
      GLuint v;
      GLfloat invq;

      if (i==0)       v = v0;
      else if (i==1)  v = v1;
      else            v = v2;

      x = VB->Win[v][0];
      y = VB->Win[v][1];
      z = VB->Win[v][2] / DEPTH_SCALE;
      w = VB->Clip[v][3];

      if (ctx->Light.ShadeModel==GL_SMOOTH) {
         /* smooth shading - different color for each vertex */
         color[0] = (GLfloat) VB->Color[v][0] / 255.0;
         color[1] = (GLfloat) VB->Color[v][1] / 255.0;
         color[2] = (GLfloat) VB->Color[v][2] / 255.0;
         color[3] = (GLfloat) VB->Color[v][3] / 255.0;
      }

      if (VB->MultiTexCoord[texSet][v][3]==0.0)
         invq =  1.0;
      else
         invq = 1.0F / VB->MultiTexCoord[texSet][v][3];
      tc[0] = VB->MultiTexCoord[texSet][v][0] * invq;
      tc[1] = VB->MultiTexCoord[texSet][v][1] * invq;
      tc[2] = VB->MultiTexCoord[texSet][v][2] * invq;
      tc[3] = VB->MultiTexCoord[texSet][v][3];

      gl_feedback_vertex( ctx, x, y, z, w, color, (GLfloat) VB->Index[v], tc );
   }
}



/*
 * Put triangle in selection buffer.
 */
static void select_triangle( GLcontext *ctx,
                             GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   (void) pv;
   gl_update_hitflag( ctx, VB->Win[v0][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, VB->Win[v1][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, VB->Win[v2][2] / DEPTH_SCALE );
}



/*
 * Render a flat-shaded color index triangle.
 */
static void flat_ci_triangle( GLcontext *ctx,
                              GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
#define INTERP_Z 1

#define SETUP_CODE            \
   GLuint index = VB->Index[pv];    \
   if (!VB->MonoColor) {         \
      /* set the color index */        \
      (*ctx->Driver.Index)( ctx, index ); \
   }

#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLdepth zspan[MAX_WIDTH];           \
      if (n>0) {                 \
         for (i=0;i<n;i++) {           \
       zspan[i] = FixedToDepth(ffz);         \
       ffz += fdzdx;             \
         }                    \
         gl_write_monoindex_span( ctx, n, LEFT, Y,    \
                               zspan, index, GL_POLYGON );  \
      }                    \
   }

#include "tritemp.h"       
}



/*
 * Render a smooth-shaded color index triangle.
 */
static void smooth_ci_triangle( GLcontext *ctx,
                                GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;
#define INTERP_Z 1
#define INTERP_INDEX 1

#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLdepth zspan[MAX_WIDTH];           \
           GLuint index[MAX_WIDTH];          \
      if (n>0) {                 \
         for (i=0;i<n;i++) {           \
       zspan[i] = FixedToDepth(ffz);         \
                 index[i] = FixedToInt(ffi);       \
       ffz += fdzdx;             \
       ffi += fdidx;             \
         }                    \
         gl_write_index_span( ctx, n, LEFT, Y, zspan, \
                              index, GL_POLYGON );    \
      }                    \
   }

#include "tritemp.h"
}



/*
 * Render a flat-shaded RGBA triangle.
 */
static void flat_rgba_triangle( GLcontext *ctx,
                                GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
#define INTERP_Z 1

#define SETUP_CODE            \
   if (!VB->MonoColor) {         \
      /* set the color */        \
      GLubyte r = VB->Color[pv][0];    \
      GLubyte g = VB->Color[pv][1];    \
      GLubyte b = VB->Color[pv][2];    \
      GLubyte a = VB->Color[pv][3];    \
      (*ctx->Driver.Color)( ctx, r, g, b, a );  \
   }

#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLdepth zspan[MAX_WIDTH];           \
      if (n>0) {                 \
         for (i=0;i<n;i++) {           \
       zspan[i] = FixedToDepth(ffz);         \
       ffz += fdzdx;             \
         }                    \
              gl_write_monocolor_span( ctx, n, LEFT, Y, zspan, \
                             VB->Color[pv][0], VB->Color[pv][1],\
                             VB->Color[pv][2], VB->Color[pv][3],\
              GL_POLYGON );         \
      }                    \
   }

#include "tritemp.h"
}



/*
 * Render a smooth-shaded RGBA triangle.
 */
static void smooth_rgba_triangle( GLcontext *ctx,
                                  GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLdepth zspan[MAX_WIDTH];           \
      GLubyte rgba[MAX_WIDTH][4];            \
      if (n>0) {                 \
         for (i=0;i<n;i++) {           \
       zspan[i] = FixedToDepth(ffz);         \
       rgba[i][RCOMP] = FixedToInt(ffr);     \
       rgba[i][GCOMP] = FixedToInt(ffg);     \
       rgba[i][BCOMP] = FixedToInt(ffb);     \
       rgba[i][ACOMP] = FixedToInt(ffa);     \
       ffz += fdzdx;             \
       ffr += fdrdx;             \
       ffg += fdgdx;             \
       ffb += fdbdx;             \
       ffa += fdadx;             \
         }                    \
         gl_write_rgba_span( ctx, n, LEFT, Y, zspan,  \
                              rgba, GL_POLYGON );     \
      }                    \
   }

#include "tritemp.h"
}


/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T only w/out mipmapping or perspective correction.
 */
static void simple_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                      GLuint v2, GLuint pv )
{
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE                     \
   struct gl_texture_object *obj = ctx->Texture.Set[0].Current2D; \
   GLint b = obj->BaseLevel;                 \
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;         \
   GLfloat theight = (GLfloat) obj->Image[b]->Height;       \
   GLint twidth_log2 = obj->Image[b]->WidthLog2;         \
   GLubyte *texture = obj->Image[b]->Data;            \
   GLint smask = obj->Image[b]->Width - 1;            \
   GLint tmask = obj->Image[b]->Height - 1;
   (void) pv;

#ifdef USE_X86_ASM
# define RGB_TEX *(GLint *)rgba[i] = *(GLint *)(texture + pos) | 0xff000000
#else
# define RGB_TEX                         \
        rgba[i][RCOMP] = texture[pos];  \
        rgba[i][GCOMP] = texture[pos+1]; \
        rgba[i][BCOMP] = texture[pos+2]; \
        rgba[i][ACOMP] = 255
#endif



#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLubyte rgba[MAX_WIDTH][4];            \
      if (n>0) {                 \
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
         for (i=0;i<n;i++) {           \
                 GLint s = FixedToInt(ffs) & smask;      \
                 GLint t = FixedToInt(fft) & tmask;      \
                 GLint pos = (t << twidth_log2) + s;     \
                 pos = pos + pos  + pos;  /* multiply by 3 */  \
                 RGB_TEX;                                       \
       ffs += fdsdx;             \
       fft += fdtdx;             \
         }                    \
              (*ctx->Driver.WriteRGBASpan)( ctx, n, LEFT, Y,   \
                                             rgba, NULL );  \
      }                    \
   }

#include "tritemp.h"
}



/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T, GL_LESS depth test, w/out mipmapping or
 * perspective correction.
 */
static void simple_z_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                        GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE                     \
   struct gl_texture_object *obj = ctx->Texture.Set[0].Current2D; \
   GLint b = obj->BaseLevel;                 \
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;         \
   GLfloat theight = (GLfloat) obj->Image[b]->Height;       \
   GLint twidth_log2 = obj->Image[b]->WidthLog2;         \
   GLubyte *texture = obj->Image[b]->Data;            \
   GLint smask = obj->Image[b]->Width - 1;            \
   GLint tmask = obj->Image[b]->Height - 1;
   (void) pv;

#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLubyte rgba[MAX_WIDTH][4];            \
           GLubyte mask[MAX_WIDTH];          \
      if (n>0) {                 \
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
         for (i=0;i<n;i++) {           \
                 GLdepth z = FixedToDepth(ffz);       \
                 if (z < zRow[i]) {          \
                    GLint s = FixedToInt(ffs) & smask;      \
                    GLint t = FixedToInt(fft) & tmask;      \
                    GLint pos = (t << twidth_log2) + s;     \
                    pos = pos + pos  + pos;  /* multiply by 3 */\
                    RGB_TEX;                                    \
          zRow[i] = z;           \
                    mask[i] = 1;          \
                 }                  \
                 else {                \
                    mask[i] = 0;          \
                 }                  \
       ffz += fdzdx;             \
       ffs += fdsdx;             \
       fft += fdtdx;             \
         }                    \
              (*ctx->Driver.WriteRGBASpan)( ctx, n, LEFT, Y,   \
                                             rgba, mask );  \
      }                    \
   }

#include "tritemp.h"
}


/*
 * Render an RGB/RGBA textured triangle without perspective correction.
 */
static void affine_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                  GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE                     \
   struct gl_texture_set *set = ctx->Texture.Set+0;                     \
   struct gl_texture_object *obj = set->Current2D;                      \
   GLint b = obj->BaseLevel;                 \
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;         \
   GLfloat theight = (GLfloat) obj->Image[b]->Height;       \
   GLint twidth_log2 = obj->Image[b]->WidthLog2;         \
   GLubyte *texture = obj->Image[b]->Data;            \
   GLint smask = obj->Image[b]->Width - 1;            \
   GLint tmask = obj->Image[b]->Height - 1;                             \
   GLint format = obj->Image[b]->Format;                                \
   GLint filter = obj->MinFilter;                                       \
   GLint envmode = set->EnvMode;                                        \
   GLint comp, tbytesline;                                              \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */        \
      er = FloatToFixed(set->EnvColor[0]);                              \
      eg = FloatToFixed(set->EnvColor[1]);                              \
      eb = FloatToFixed(set->EnvColor[2]);                              \
      ea = FloatToFixed(set->EnvColor[3]);                              \
   }                                                                    \
   switch (format) {                                                    \
   case GL_ALPHA:                                                       \
   case GL_LUMINANCE:                                                   \
   case GL_INTENSITY:                                                   \
      comp = 1;                                                         \
      break;                                                            \
   case GL_LUMINANCE_ALPHA:                                             \
      comp = 2;                                                         \
      break;                                                            \
   case GL_RGB:                                                         \
      comp = 3;                                                         \
      break;                                                            \
   case GL_RGBA:                                                        \
      comp = 4;                                                         \
      break;                                                            \
   default:                                                             \
      gl_problem(NULL, "Bad texture format in affine_texture_triangle");\
      return;                                                           \
   }                                                                    \
   tbytesline = obj->Image[b]->Width * comp;
   (void) pv;

  /* Instead of defining a function for each mode, a test is done 
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills 
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */ 

#define NEAREST_RGB    \
        tr = tex00[0]; \
        tg = tex00[1]; \
        tb = tex00[2]; \
        ta = 0xff

#define LINEAR_RGB                                                    \
   tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
   tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
   tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
   ta = 0xff

#define NEAREST_RGBA   \
        tr = tex00[0]; \
        tg = tex00[1]; \
        tb = tex00[2]; \
        ta = tex00[3]

#define LINEAR_RGBA                                                   \
   tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
   tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
   tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
   ta = (ti * (si * tex00[3] + sf * tex01[3]) +                    \
              tf * (si * tex10[3] + sf * tex11[3])) >> 2 * FIXED_SHIFT

#define MODULATE                                     \
        dest[0] = ffr * (tr + 1) >> (FIXED_SHIFT + 8); \
        dest[1] = ffg * (tg + 1) >> (FIXED_SHIFT + 8); \
        dest[2] = ffb * (tb + 1) >> (FIXED_SHIFT + 8); \
        dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define DECAL                                                                            \
   dest[0] = ((0xff - ta) * ffr + ((ta + 1) * tr << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
   dest[1] = ((0xff - ta) * ffg + ((ta + 1) * tg << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
   dest[2] = ((0xff - ta) * ffb + ((ta + 1) * tb << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
   dest[3] = FixedToInt(ffa)

#define BLEND                                                           \
        dest[0] = ((0xff - tr) * ffr + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[1] = ((0xff - tg) * ffg + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[2] = ((0xff - tb) * ffb + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
   dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define REPLACE       \
        dest[0] = tr; \
        dest[1] = tg; \
        dest[2] = tb; \
        dest[3] = ta

#define ADD                                                      \
        dest[0] = ((ffr << 8) + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[1] = ((ffg << 8) + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[2] = ((ffb << 8) + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
   dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

/* shortcuts */

#if defined(USE_X86_ASM)
/* assumes (i) unaligned load capacity, and (ii) little endian: */
# define NEAREST_RGB_REPLACE  *(GLint *)dest = (*(GLint *)tex00 & 0x00ffffff) \
                                          | ((ffa << (24 - FIXED_SHIFT)) & 0xff000000)
#else
# define NEAREST_RGB_REPLACE  NEAREST_RGB;REPLACE
#endif
#define NEAREST_RGBA_REPLACE  *(GLint *)dest = *(GLint *)tex00

#define SPAN1(DO_TEX,COMP)                                 \
   for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint pos = (t << twidth_log2) + s;             \
           GLubyte *tex00 = texture + COMP * pos;          \
      zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
      ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
      ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
      ffa += fdadx;                                   \
      ffs += fdsdx;                                   \
      fft += fdtdx;                                   \
           dest += 4;                                      \
   }

#define SPAN2(DO_TEX,COMP)                                 \
   for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint sf = ffs & FIXED_FRAC_MASK;               \
           GLint tf = fft & FIXED_FRAC_MASK;               \
           GLint si = FIXED_FRAC_MASK - sf;                \
           GLint ti = FIXED_FRAC_MASK - tf;                \
           GLint pos = (t << twidth_log2) + s;             \
           GLubyte *tex00 = texture + COMP * pos;          \
           GLubyte *tex10 = tex00 + tbytesline;            \
           GLubyte *tex01 = tex00 + COMP;                  \
           GLubyte *tex11 = tex10 + COMP;                  \
      zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
      ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
      ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
      ffa += fdadx;                                   \
      ffs += fdsdx;                                   \
      fft += fdtdx;                                   \
           dest += 4;                                      \
   }

/* here comes the heavy part.. (something for the compiler to chew on) */
#define INNER_LOOP( LEFT, RIGHT, Y )                      \
   {                              \
      GLint i, n = RIGHT-LEFT;                      \
      GLdepth zspan[MAX_WIDTH];                     \
      GLubyte rgba[MAX_WIDTH][4];                     \
      if (n>0) {                                      \
              GLubyte *dest = rgba[0];                     \
              ffs -= FIXED_HALF; /* off-by-one error? */   \
              fft -= FIXED_HALF;                           \
              switch (filter) {                            \
            case GL_NEAREST:                             \
       switch (format) {                         \
                 case GL_RGB:                              \
               switch (envmode) {                     \
               case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGB;MODULATE,3);      \
                       break;                              \
               case GL_DECAL:                         \
                    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGB_REPLACE,3);       \
                       break;                              \
                    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGB;BLEND,3);         \
                       break;                              \
                    case GL_ADD:                           \
                       SPAN1(NEAREST_RGB;ADD,3);           \
                       break;                              \
               }                                      \
                    break;                                 \
       case GL_RGBA:                             \
          switch(envmode) {                      \
          case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGBA;MODULATE,4);     \
                       break;                              \
          case GL_DECAL:                         \
                       SPAN1(NEAREST_RGBA;DECAL,4);        \
                       break;                              \
          case GL_BLEND:                         \
                       SPAN1(NEAREST_RGBA;BLEND,4);        \
                       break;                              \
          case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGBA_REPLACE,4);      \
                       break;                              \
          case GL_ADD:                           \
                       SPAN1(NEAREST_RGBA;ADD,4);          \
                       break;                              \
          }                                      \
                    break;                                 \
            }                                         \
                 break;                                    \
         case GL_LINEAR:                              \
                 ffs -= FIXED_HALF;                        \
                 fft -= FIXED_HALF;                        \
       switch (format) {                         \
       case GL_RGB:                              \
          switch (envmode) {                     \
          case GL_MODULATE:                      \
             SPAN2(LINEAR_RGB;MODULATE,3);       \
                       break;                              \
          case GL_DECAL:                         \
          case GL_REPLACE:                       \
                       SPAN2(LINEAR_RGB;REPLACE,3);        \
                       break;                              \
          case GL_BLEND:                         \
             SPAN2(LINEAR_RGB;BLEND,3);          \
             break;                              \
          case GL_ADD:                           \
             SPAN2(LINEAR_RGB;ADD,3);            \
             break;                              \
          }                                      \
          break;                                 \
       case GL_RGBA:                             \
          switch (envmode) {                     \
          case GL_MODULATE:                      \
             SPAN2(LINEAR_RGBA;MODULATE,4);      \
             break;                              \
          case GL_DECAL:                         \
             SPAN2(LINEAR_RGBA;DECAL,4);         \
             break;                              \
          case GL_BLEND:                         \
             SPAN2(LINEAR_RGBA;BLEND,4);         \
             break;                              \
          case GL_REPLACE:                       \
             SPAN2(LINEAR_RGBA;REPLACE,4);       \
             break;                              \
          case GL_ADD:                           \
             SPAN2(LINEAR_RGBA;ADD,4);           \
             break;                              \
          }                                      \
          break;                                 \
            }                                         \
                 break;                                    \
         }                                            \
              gl_write_rgba_span(ctx, n, LEFT, Y, zspan,   \
                                 rgba, GL_POLYGON);        \
              /* explicit kill of variables: */            \
              ffr = ffg = ffb = ffa = 0;                   \
           }                                               \
   }

#include "tritemp.h"
#undef SPAN1
#undef SPAN2
}


/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 */
static void persp_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;

#define SPAN1(DO_TEX,COMP)                                  \
         for( i = 0; i < n; i++ )                           \
         {                                                  \
            GLfloat  invQ = 1.0f / vv;                      \
            GLint     s = (int)(SS * invQ) & smask;         \
            GLint     t = (int)(TT * invQ) & tmask;         \
            GLint     pos = COMP * ((t << twidth_log2) + s);\
            GLubyte   *tex00 = texture + pos;               \
                                                            \
            zspan[i] = FixedToDepth(ffz);                   \
            DO_TEX;                                         \
            ffz += fdzdx;                                   \
            ffr += fdrdx;                                   \
            ffg += fdgdx;                                   \
            ffb += fdbdx;                                   \
            ffa += fdadx;                                   \
            SS += dSdx;                                     \
            TT += dTdx;                                     \
            vv += dvdx;                                     \
           dest += 4;                                       \
         }

#define SPAN2(DO_TEX,COMP)                                  \
         for( i = 0; i < n; i++ )                           \
         {                                                  \
            GLfloat  invQ = 1.0f / vv;                      \
            GLfixed  ffs = (int)(SS * invQ);                \
            GLfixed  fft = (int)(TT * invQ);                \
            GLint    s = FixedToInt(ffs) & smask;           \
            GLint    t = FixedToInt(fft) & tmask;           \
            GLint    sf = ffs & FIXED_FRAC_MASK;            \
            GLint    tf = fft & FIXED_FRAC_MASK;            \
            GLint    si = FIXED_FRAC_MASK - sf;             \
            GLint    ti = FIXED_FRAC_MASK - tf;             \
            GLint    pos = COMP * ((t << twidth_log2) + s); \
            GLubyte  *tex00 = texture + pos;                \
            GLubyte  *tex10 = tex00 + tbytesline;           \
            GLubyte  *tex01 = tex00 + COMP;                 \
            GLubyte  *tex11 = tex10 + COMP;                 \
                                                            \
            zspan[i] = FixedToDepth(ffz);                   \
            DO_TEX;                                         \
            ffz += fdzdx;                                   \
            ffr += fdrdx;                                   \
            ffg += fdgdx;                                   \
            ffb += fdbdx;                                   \
            ffa += fdadx;                                   \
            SS += dSdx;                                     \
            TT += dTdx;                                     \
            vv += dvdx;                                     \
            dest += 4;                                      \
         }
{
   typedef struct {
        GLint v0, v1;   /* Y(v0) < Y(v1) */
   GLfloat dx; /* X(v1) - X(v0) */
   GLfloat dy; /* Y(v1) - Y(v0) */
   GLfixed fdxdy; /* dx/dy in fixed-point */
   GLfixed fsx;   /* first sample point x coord */
   GLfixed fsy;
   GLfloat adjy;  /* adjust from v[0]->fy to fsy, scaled */
   GLint lines;   /* number of lines to be sampled on this edge */
   GLfixed fx0;   /* fixed pt X of lower endpoint */
   } EdgeT;

   struct vertex_buffer *VB = ctx->VB;
   EdgeT eMaj, eTop, eBot;
   GLfloat oneOverArea;
   int vMin, vMid, vMax;       /* vertex indexes:  Y(vMin)<=Y(vMid)<=Y(vMax) */

   /* find the order of the 3 vertices along the Y axis */
   {
      GLfloat y0 = VB->Win[v0][1];
      GLfloat y1 = VB->Win[v1][1];
      GLfloat y2 = VB->Win[v2][1];

      if (y0<=y1) {
    if (y1<=y2) {
       vMin = v0;   vMid = v1;   vMax = v2;   /* y0<=y1<=y2 */
    }
    else if (y2<=y0) {
       vMin = v2;   vMid = v0;   vMax = v1;   /* y2<=y0<=y1 */
    }
    else {
       vMin = v0;   vMid = v2;   vMax = v1;   /* y0<=y2<=y1 */
    }
      }
      else {
    if (y0<=y2) {
       vMin = v1;   vMid = v0;   vMax = v2;   /* y1<=y0<=y2 */
    }
    else if (y2<=y1) {
       vMin = v2;   vMid = v1;   vMax = v0;   /* y2<=y1<=y0 */
    }
    else {
       vMin = v1;   vMid = v2;   vMax = v0;   /* y1<=y2<=y0 */
    }
      }
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[v1] - vertex[v0] */
   eMaj.dx = VB->Win[vMax][0] - VB->Win[vMin][0];
   eMaj.dy = VB->Win[vMax][1] - VB->Win[vMin][1];
   eTop.dx = VB->Win[vMax][0] - VB->Win[vMid][0];
   eTop.dy = VB->Win[vMax][1] - VB->Win[vMid][1];
   eBot.dx = VB->Win[vMid][0] - VB->Win[vMin][0];
   eBot.dy = VB->Win[vMid][1] - VB->Win[vMin][1];

   /* compute oneOverArea */
   {
      GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;
      if (area>-0.05f && area<0.05f) {
         return;  /* very small; CULLED */
      }
      oneOverArea = 1.0F / area;
   }

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      /* fixed point Y coordinates */
      GLfixed vMin_fx = FloatToFixed(VB->Win[vMin][0] + 0.5F);
      GLfixed vMin_fy = FloatToFixed(VB->Win[vMin][1] - 0.5F);
      GLfixed vMid_fx = FloatToFixed(VB->Win[vMid][0] + 0.5F);
      GLfixed vMid_fy = FloatToFixed(VB->Win[vMid][1] - 0.5F);
      GLfixed vMax_fy = FloatToFixed(VB->Win[vMax][1] - 0.5F);

      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(vMax_fy + FIXED_ONE - FIXED_EPSILON - eMaj.fsy);
      if (eMaj.lines > 0) {
         GLfloat dxdy = eMaj.dx / eMaj.dy;
         eMaj.fdxdy = SignedFloatToFixed(dxdy);
         eMaj.adjy = (GLfloat) (eMaj.fsy - vMin_fy);  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (GLfixed) (eMaj.adjy * dxdy);
      }
      else {
         return;  /*CULLED*/
      }

      eTop.fsy = FixedCeil(vMid_fy);
      eTop.lines = FixedToInt(vMax_fy + FIXED_ONE - FIXED_EPSILON - eTop.fsy);
      if (eTop.lines > 0) {
         GLfloat dxdy = eTop.dx / eTop.dy;
         eTop.fdxdy = SignedFloatToFixed(dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * dxdy);
      }

      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(vMid_fy + FIXED_ONE - FIXED_EPSILON - eBot.fsy);
      if (eBot.lines > 0) {
         GLfloat dxdy = eBot.dx / eBot.dy;
         eBot.fdxdy = SignedFloatToFixed(dxdy);
         eBot.adjy = (GLfloat) (eBot.fsy - vMin_fy);  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (GLfixed) (eBot.adjy * dxdy);
      }
   }

   /*
    * Conceptually, we view a triangle as two subtriangles
    * separated by a perfectly horizontal line.  The edge that is
    * intersected by this line is one with maximal absolute dy; we
    * call it a ``major'' edge.  The other two edges are the
    * ``top'' edge (for the upper subtriangle) and the ``bottom''
    * edge (for the lower subtriangle).  If either of these two
    * edges is horizontal or very close to horizontal, the
    * corresponding subtriangle might cover zero sample points;
    * we take care to handle such cases, for performance as well
    * as correctness.
    *
    * By stepping rasterization parameters along the major edge,
    * we can avoid recomputing them at the discontinuity where
    * the top and bottom edges meet.  However, this forces us to
    * be able to scan both left-to-right and right-to-left. 
    * Also, we must determine whether the major edge is at the
    * left or right side of the triangle.  We do this by
    * computing the magnitude of the cross-product of the major
    * and top edges.  Since this magnitude depends on the sine of
    * the angle between the two edges, its sign tells us whether
    * we turn to the left or to the right when travelling along
    * the major edge to the top edge, and from this we infer
    * whether the major edge is on the left or the right.
    *
    * Serendipitously, this cross-product magnitude is also a
    * value we need to compute the iteration parameter
    * derivatives for the triangle, and it can be used to perform
    * backface culling because its sign tells us whether the
    * triangle is clockwise or counterclockwise.  In this code we
    * refer to it as ``area'' because it's also proportional to
    * the pixel area of the triangle.
    */

   {
      GLint ltor;    /* true if scanning left-to-right */
      GLfloat dzdx, dzdy;      GLfixed fdzdx;
      GLfloat drdx, drdy;      GLfixed fdrdx;
      GLfloat dgdx, dgdy;      GLfixed fdgdx;
      GLfloat dbdx, dbdy;      GLfixed fdbdx;
      GLfloat dadx, dady;      GLfixed fdadx;
      GLfloat dsdx, dsdy;
      GLfloat dtdx, dtdy;
      GLfloat dudx, dudy;
      GLfloat dvdx, dvdy;

      /*
       * Execute user-supplied setup code
       */
      struct gl_texture_set      *set = ctx->Texture.Set+0;                
      struct gl_texture_object   *obj = set->Current2D;                    
      GLint    b = obj->BaseLevel;                                         
      GLfloat  twidth = (GLfloat) obj->Image[b]->Width;                    
      GLfloat  theight = (GLfloat) obj->Image[b]->Height;                  
      GLint    twidth_log2 = obj->Image[b]->WidthLog2;                     
      GLubyte  *texture = obj->Image[b]->Data;                             
      GLint    smask = (obj->Image[b]->Width - 1);                         
      GLint    tmask = (obj->Image[b]->Height - 1);                        
      GLint    format = obj->Image[b]->Format;                             
      GLint    filter = obj->MinFilter;                                    
      GLint    envmode = set->EnvMode;                                     
      GLfloat  sscale, tscale;                                             
      GLint    comp, tbytesline;                                           
      GLfixed  er, eg, eb, ea;                                             
      GLint    tr, tg, tb, ta;                                             
      if (envmode == GL_BLEND || envmode == GL_ADD)                        
      {                                                                    
         er = FloatToFixed(set->EnvColor[0]);                              
         eg = FloatToFixed(set->EnvColor[1]);                              
         eb = FloatToFixed(set->EnvColor[2]);                              
         ea = FloatToFixed(set->EnvColor[3]);                              
      }                                                                    
      switch( format )                                                     
      {                                                                    
         case GL_ALPHA:                                                    
         case GL_LUMINANCE:                                                
         case GL_INTENSITY:                                                
            comp = 1;                                                      
            break;                                                         
         case GL_LUMINANCE_ALPHA:                                          
            comp = 2;                                                      
            break;                                                         
         case GL_RGB:                                                      
            comp = 3;                                                      
            break;                                                         
         case GL_RGBA:                                                     
            comp = 4;                                                      
            break;                                                         
         default:                                                          
            gl_problem(NULL, "Bad texture format in persp_texture_triangle"); 
            return;                                                        
      }                                                                    
      if ( filter == GL_NEAREST )                                          
      {                                                                    
         sscale = twidth;                                                  
         tscale = theight;                                                 
      }                                                                    
      else                                                                 
      {                                                                    
         sscale = FIXED_SCALE * twidth;                                    
         tscale = FIXED_SCALE * theight;                                   
      }                                                                    
      tbytesline = obj->Image[b]->Width * comp;

      ltor = (oneOverArea < 0.0F);

      /* compute d?/dx and d?/dy derivatives */
      {
         GLfloat eMaj_dz, eBot_dz;
         eMaj_dz = VB->Win[vMax][2] - VB->Win[vMin][2];
         eBot_dz = VB->Win[vMid][2] - VB->Win[vMin][2];
         dzdx = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (dzdx>DEPTH_SCALE || dzdx<-DEPTH_SCALE) {
            /* probably a sliver triangle */
            dzdx = 0.0;
            dzdy = 0.0;
         }
         else {
            dzdy = oneOverArea * (eMaj.dx * eBot_dz - eMaj_dz * eBot.dx);
         }
#if DEPTH_BITS==16
         fdzdx = SignedFloatToFixed(dzdx);
#else
         fdzdx = (GLint) dzdx;
#endif
      }
      {
         GLfloat eMaj_dr, eBot_dr;
         eMaj_dr = (GLint) VB->Color[vMax][0] - (GLint) VB->Color[vMin][0];
         eBot_dr = (GLint) VB->Color[vMid][0] - (GLint) VB->Color[vMin][0];
         drdx = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         fdrdx = SignedFloatToFixed(drdx);
         drdy = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
      }
      {
         GLfloat eMaj_dg, eBot_dg;
         eMaj_dg = (GLint) VB->Color[vMax][1] - (GLint) VB->Color[vMin][1];
    eBot_dg = (GLint) VB->Color[vMid][1] - (GLint) VB->Color[vMin][1];
         dgdx = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         fdgdx = SignedFloatToFixed(dgdx);
         dgdy = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
      }
      {
         GLfloat eMaj_db, eBot_db;
         eMaj_db = (GLint) VB->Color[vMax][2] - (GLint) VB->Color[vMin][2];
         eBot_db = (GLint) VB->Color[vMid][2] - (GLint) VB->Color[vMin][2];
         dbdx = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         fdbdx = SignedFloatToFixed(dbdx);
    dbdy = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
      }
      {
         GLfloat eMaj_da, eBot_da;
         eMaj_da = (GLint) VB->Color[vMax][3] - (GLint) VB->Color[vMin][3];
         eBot_da = (GLint) VB->Color[vMid][3] - (GLint) VB->Color[vMin][3];
         dadx = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         fdadx = SignedFloatToFixed(dadx);
         dady = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
      }
      {
         GLfloat wMax = 1.0F / VB->Clip[vMax][3];
         GLfloat wMin = 1.0F / VB->Clip[vMin][3];
         GLfloat wMid = 1.0F / VB->Clip[vMid][3];
         GLfloat eMaj_ds, eBot_ds;
         GLfloat eMaj_dt, eBot_dt;
         GLfloat eMaj_du, eBot_du;
         GLfloat eMaj_dv, eBot_dv;

         eMaj_ds = VB->MultiTexCoord[0][vMax][0]*wMax - VB->MultiTexCoord[0][vMin][0]*wMin;
         eBot_ds = VB->MultiTexCoord[0][vMid][0]*wMid - VB->MultiTexCoord[0][vMin][0]*wMin;
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

         eMaj_dt = VB->MultiTexCoord[0][vMax][1]*wMax - VB->MultiTexCoord[0][vMin][1]*wMin;
         eBot_dt = VB->MultiTexCoord[0][vMid][1]*wMid - VB->MultiTexCoord[0][vMin][1]*wMin;
         dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
         eMaj_du = VB->MultiTexCoord[0][vMax][2]*wMax - VB->MultiTexCoord[0][vMin][2]*wMin;
         eBot_du = VB->MultiTexCoord[0][vMid][2]*wMid - VB->MultiTexCoord[0][vMin][2]*wMin;
         dudx = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
         dudy = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);

         eMaj_dv = VB->MultiTexCoord[0][vMax][3]*wMax - VB->MultiTexCoord[0][vMin][3]*wMin;
         eBot_dv = VB->MultiTexCoord[0][vMid][3]*wMid - VB->MultiTexCoord[0][vMin][3]*wMin;
         dvdx = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
         dvdy = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
      }

      /*
       * We always sample at pixel centers.  However, we avoid
       * explicit half-pixel offsets in this code by incorporating
       * the proper offset in each of x and y during the
       * transformation to window coordinates.
       *
       * We also apply the usual rasterization rules to prevent
       * cracks and overlaps.  A pixel is considered inside a
       * subtriangle if it meets all of four conditions: it is on or
       * to the right of the left edge, strictly to the left of the
       * right edge, on or below the top edge, and strictly above
       * the bottom edge.  (Some edges may be degenerate.)
       *
       * The following discussion assumes left-to-right scanning
       * (that is, the major edge is on the left); the right-to-left
       * case is a straightforward variation.
       *
       * We start by finding the half-integral y coordinate that is
       * at or below the top of the triangle.  This gives us the
       * first scan line that could possibly contain pixels that are
       * inside the triangle.
       *
       * Next we creep down the major edge until we reach that y,
       * and compute the corresponding x coordinate on the edge. 
       * Then we find the half-integral x that lies on or just
       * inside the edge.  This is the first pixel that might lie in
       * the interior of the triangle.  (We won't know for sure
       * until we check the other edges.)
       *
       * As we rasterize the triangle, we'll step down the major
       * edge.  For each step in y, we'll move an integer number
       * of steps in x.  There are two possible x step sizes, which
       * we'll call the ``inner'' step (guaranteed to land on the
       * edge or inside it) and the ``outer'' step (guaranteed to
       * land on the edge or outside it).  The inner and outer steps
       * differ by one.  During rasterization we maintain an error
       * term that indicates our distance from the true edge, and
       * select either the inner step or the outer step, whichever
       * gets us to the first pixel that falls inside the triangle.
       *
       * All parameters (z, red, etc.) as well as the buffer
       * addresses for color and z have inner and outer step values,
       * so that we can increment them appropriately.  This method
       * eliminates the need to adjust parameters by creeping a
       * sub-pixel amount into the triangle at each scanline.
       */

      {
         int subTriangle;
         GLfixed fx, fxLeftEdge, fxRightEdge, fdxLeftEdge, fdxRightEdge;
         GLfixed fdxOuter;
         int idxOuter;
         float dxOuter;
         GLfixed fError, fdError;
         float adjx, adjy;
         GLfixed fy;
         int iy;
         GLdepth *zRow;
         int dZRowOuter, dZRowInner;  /* offset in bytes */
         GLfixed fz, fdzOuter, fdzInner;
         GLfixed fr, fdrOuter, fdrInner;
         GLfixed fg, fdgOuter, fdgInner;
         GLfixed fb, fdbOuter, fdbInner;
         GLfixed fa, fdaOuter, fdaInner;
         GLfloat sLeft, dsOuter, dsInner;
         GLfloat tLeft, dtOuter, dtInner;
         GLfloat uLeft, duOuter, duInner;
         GLfloat vLeft, dvOuter, dvInner;

         for (subTriangle=0; subTriangle<=1; subTriangle++) {
            EdgeT *eLeft, *eRight;
            int setupLeft, setupRight;
            int lines;

            if (subTriangle==0) {
               /* bottom half */
               if (ltor) {
                  eLeft = &eMaj;
                  eRight = &eBot;
                  lines = eRight->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
               else {
                  eLeft = &eBot;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
            }
            else {
               /* top half */
               if (ltor) {
                  eLeft = &eMaj;
                  eRight = &eTop;
                  lines = eRight->lines;
                  setupLeft = 0;
                  setupRight = 1;
               }
               else {
                  eLeft = &eTop;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 0;
               }
               if (lines==0) return;
            }

            if (setupLeft && eLeft->lines>0) {
               GLint vLower;
               GLfixed fsx = eLeft->fsx;
               fx = FixedCeil(fsx);
               fError = fx - fsx - FIXED_ONE;
               fxLeftEdge = fsx - FIXED_EPSILON;
               fdxLeftEdge = eLeft->fdxdy;
               fdxOuter = FixedFloor(fdxLeftEdge - FIXED_EPSILON);
               fdError = fdxOuter - fdxLeftEdge + FIXED_ONE;
               idxOuter = FixedToInt(fdxOuter);
               dxOuter = (float) idxOuter;

               fy = eLeft->fsy;
               iy = FixedToInt(fy);

               adjx = (float)(fx - eLeft->fx0);  /* SCALED! */
               adjy = eLeft->adjy;      /* SCALED! */

               vLower = eLeft->v0;

               /*
                * Now we need the set of parameter (z, color, etc.) values at
                * the point (fx, fy).  This gives us properly-sampled parameter
                * values that we can step from pixel to pixel.  Furthermore,
                * although we might have intermediate results that overflow
                * the normal parameter range when we step temporarily outside
                * the triangle, we shouldn't overflow or underflow for any
                * pixel that's actually inside the triangle.
                */

               {
                  GLfloat z0, tmp;
                  z0 = VB->Win[vLower][2] + ctx->PolygonZoffset;
#if DEPTH_BITS==16
                  /* interpolate fixed-pt values */
                  tmp = (z0 * FIXED_SCALE + dzdx * adjx + dzdy * adjy) + FIXED_HALF;
                  if (tmp < MAX_GLUINT/2)
                     fz = (GLfixed) tmp;
                  else
                     fz = MAX_GLUINT/2;
                  fdzOuter = SignedFloatToFixed(dzdy + dxOuter * dzdx);
#else
                  /* interpolate depth values exactly */
                  fz = (GLint) (z0 + dzdx*FixedToFloat(adjx) + dzdy*FixedToFloat(adjy));
                  fdzOuter = (GLint) (dzdy + dxOuter * dzdx);
#endif
                  zRow = Z_ADDRESS( ctx, FixedToInt(fxLeftEdge), iy );
                  dZRowOuter = (ctx->Buffer->Width + idxOuter) * sizeof(GLdepth);
               }
               fr = (GLfixed)(IntToFixed(VB->Color[vLower][0]) + drdx * adjx + drdy * adjy)
                    + FIXED_HALF;
               fdrOuter = SignedFloatToFixed(drdy + dxOuter * drdx);

               fg = (GLfixed)(IntToFixed(VB->Color[vLower][1]) + dgdx * adjx + dgdy * adjy)
                    + FIXED_HALF;
               fdgOuter = SignedFloatToFixed(dgdy + dxOuter * dgdx);

               fb = (GLfixed)(IntToFixed(VB->Color[vLower][2]) + dbdx * adjx + dbdy * adjy)
                    + FIXED_HALF;
               fdbOuter = SignedFloatToFixed(dbdy + dxOuter * dbdx);
               fa = (GLfixed)(IntToFixed(VB->Color[vLower][3]) + dadx * adjx + dady * adjy)
                    + FIXED_HALF;
               fdaOuter = SignedFloatToFixed(dady + dxOuter * dadx);
               {
                  GLfloat invW = 1.0F / VB->Clip[vLower][3];
                  GLfloat s0, t0, u0, v0;
                  s0 = VB->MultiTexCoord[0][vLower][0] * invW;
                  sLeft = s0 + (dsdx * adjx + dsdy * adjy) * (1.0F/FIXED_SCALE);
                  dsOuter = dsdy + dxOuter * dsdx;
                  t0 = VB->MultiTexCoord[0][vLower][1] * invW;
                  tLeft = t0 + (dtdx * adjx + dtdy * adjy) * (1.0F/FIXED_SCALE);
                  dtOuter = dtdy + dxOuter * dtdx;
                  u0 = VB->MultiTexCoord[0][vLower][2] * invW;
                  uLeft = u0 + (dudx * adjx + dudy * adjy) * (1.0F/FIXED_SCALE);
                  duOuter = dudy + dxOuter * dudx;
                  v0 = VB->MultiTexCoord[0][vLower][3] * invW;
                  vLeft = v0 + (dvdx * adjx + dvdy * adjy) * (1.0F/FIXED_SCALE);
                  dvOuter = dvdy + dxOuter * dvdx;
               }

            } /*if setupLeft*/


            if (setupRight && eRight->lines>0) {
               fxRightEdge = eRight->fsx - FIXED_EPSILON;
               fdxRightEdge = eRight->fdxdy;
            }

            if (lines==0) {
               continue;
            }


            /* Rasterize setup */
            dZRowInner = dZRowOuter + sizeof(GLdepth);
            fdzInner = fdzOuter + fdzdx;
            fdrInner = fdrOuter + fdrdx;
            fdgInner = fdgOuter + fdgdx;
            fdbInner = fdbOuter + fdbdx;
            fdaInner = fdaOuter + fdadx;
       dsInner = dsOuter + dsdx;
       dtInner = dtOuter + dtdx;
       duInner = duOuter + dudx;
       dvInner = dvOuter + dvdx;

            while (lines>0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
               GLfixed ffz = fz;
               /*GLdepth *zp = zRow;*/
               GLfixed ffr = fr,  ffg = fg,  ffb = fb;
               GLfixed ffa = fa;
               GLfloat ss = sLeft, tt = tLeft, uu = uLeft, vv = vLeft;
               GLint left = FixedToInt(fxLeftEdge);
               GLint right = FixedToInt(fxRightEdge);

               {
                  /* need this to accomodate round-off errors */
                  GLfixed ffrend = ffr+(right-left-1)*fdrdx;
                  GLfixed ffgend = ffg+(right-left-1)*fdgdx;
                  GLfixed ffbend = ffb+(right-left-1)*fdbdx;
                  if (ffrend<0) ffr -= ffrend;
                  if (ffgend<0) ffg -= ffgend;
                  if (ffbend<0) ffb -= ffbend;
                  if (ffr<0) ffr = 0;
                  if (ffg<0) ffg = 0;
                  if (ffb<0) ffb = 0;
               }
               {
                  GLfixed ffaend = ffa+(right-left-1)*fdadx;
                  if (ffaend<0) ffa -= ffaend;
                  if (ffa<0) ffa = 0;
               }

   {                                                        
      GLint    i, n = right - left;                         
      GLdepth  zspan[MAX_WIDTH];                            
      GLubyte  rgba[MAX_WIDTH][4];                          
           (void)uu; /* please GCC */                       
      if ( n > 0)                                           
      {                                                     
         GLfloat SS = ss * sscale;                          
         GLfloat TT = tt * tscale;                          
         GLfloat dSdx = dsdx * sscale;                      
         GLfloat dTdx = dtdx * tscale;                      
         GLubyte *dest = rgba[0];                           
         switch( filter )                                   
         {                                                    
            case GL_NEAREST:                                
               switch( format )                             
               {                                            
                  case GL_RGB:                              
                     switch( envmode )                      
                     {                                      
                        case GL_MODULATE:                   
                           SPAN1(NEAREST_RGB;MODULATE,3);   
                           break;                           
                        case GL_DECAL:                      
                        case GL_REPLACE:                    
                           SPAN1(NEAREST_RGB_REPLACE,3);    
                           break;                           
                        case GL_BLEND:                      
                           SPAN1(NEAREST_RGB;BLEND,3);      
                           break;                           
                        case GL_ADD:                        
                           SPAN1(NEAREST_RGB;ADD,3);        
                           break;                           
                     }                                      
                     break;                                 
                  case GL_RGBA:                             
                     switch( envmode )                      
                     {                                      
                        case GL_MODULATE:                   
                           SPAN1(NEAREST_RGBA;MODULATE,4);  
                           break;                           
                        case GL_DECAL:                      
                           SPAN1(NEAREST_RGBA;DECAL,4);     
                           break;                           
                        case GL_BLEND:                      
                           SPAN1(NEAREST_RGBA;BLEND,4);     
                           break;                           
                        case GL_REPLACE:                    
                           SPAN1(NEAREST_RGBA_REPLACE,4);   
                           break;                           
                        case GL_ADD:                        
                           SPAN1(NEAREST_RGBA;ADD,4);       
                           break;                           
                     }                                      
                     break;                                 
                  }                                         
                  break;                                    
                                                            
            case GL_LINEAR:                                 
               SS -= 0.5f * FIXED_SCALE * vv;               
               TT -= 0.5f * FIXED_SCALE * vv;               
               switch( format )                             
               {                                            
                  case GL_RGB:                              
                     switch( envmode )                      
                     {                                      
                        case GL_MODULATE:                   
                           SPAN2(LINEAR_RGB;MODULATE,3);    
                           break;                           
                        case GL_DECAL:                      
                        case GL_REPLACE:                    
                           SPAN2(LINEAR_RGB;REPLACE,3);     
                           break;                           
                        case GL_BLEND:                      
                           SPAN2(LINEAR_RGB;BLEND,3);       
                           break;                           
                        case GL_ADD:                        
                           SPAN2(LINEAR_RGB;ADD,3);         
                           break;                           
                     }                                      
                     break;                                 
                  case GL_RGBA:                             
                     switch( envmode )                      
                     {                                      
                        case GL_MODULATE:                   
                           SPAN2(LINEAR_RGBA;MODULATE,4);   
                           break;                           
                        case GL_DECAL:                      
                           SPAN2(LINEAR_RGBA;DECAL,4);      
                           break;                           
                        case GL_BLEND:                      
                           SPAN2(LINEAR_RGBA;BLEND,4);      
                           break;                           
                        case GL_REPLACE:                    
                           SPAN2(LINEAR_RGBA;REPLACE,4);    
                           break;                           
                        case GL_ADD:                        
                           SPAN2(LINEAR_RGBA;ADD,4);        
                           break;                           
                     }                                      
                     break;                                 
               }                                            
               break;                                       
         }                                                  
         gl_write_rgba_span( ctx, n, left, iy, zspan,  rgba, GL_POLYGON ); 
         ffr = ffg = ffb = ffa = 0;                         
      }                                                     
   }

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               iy++;
               lines--;

               fxLeftEdge += fdxLeftEdge;
               fxRightEdge += fdxRightEdge;


               fError += fdError;
               if (fError >= 0) {
                  fError -= FIXED_ONE;
                  zRow = (GLdepth*) ((GLubyte*)zRow + dZRowOuter);
                  fz += fdzOuter;
                  fr += fdrOuter;   fg += fdgOuter;   fb += fdbOuter;
                  fa += fdaOuter;
        sLeft += dsOuter;
        tLeft += dtOuter;
        uLeft += duOuter;
        vLeft += dvOuter;
               }
               else {
                  zRow = (GLdepth*) ((GLubyte*)zRow + dZRowInner);
                  fz += fdzInner;
                  fr += fdrInner;   fg += fdgInner;   fb += fdbInner;
                  fa += fdaInner;
        sLeft += dsInner;
        tLeft += dtInner;
        uLeft += duInner;
        vLeft += dvInner;
               }
            } /*while lines>0*/

         } /* for subTriangle */

      }
   }
}

#undef INNER_LOOP
#undef SPAN1
#undef SPAN2
}


/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                       GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define SETUP_CODE                  \
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT); \
   GLint r, g, b, a;                \
   if (flat_shade) {                \
      r = VB->Color[pv][0];               \
      g = VB->Color[pv][1];               \
      b = VB->Color[pv][2];               \
      a = VB->Color[pv][3];               \
   }
#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
      GLdepth zspan[MAX_WIDTH];           \
      GLubyte rgba[MAX_WIDTH][4];            \
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];   \
      if (n>0) {                 \
              if (flat_shade) {              \
                 for (i=0;i<n;i++) {            \
          GLdouble invQ = 1.0 / vv;       \
          zspan[i] = FixedToDepth(ffz);      \
          rgba[i][RCOMP] = r;          \
          rgba[i][GCOMP] = g;          \
          rgba[i][BCOMP] = b;          \
          rgba[i][ACOMP] = a;          \
          s[i] = ss*invQ;           \
          t[i] = tt*invQ;           \
          u[i] = uu*invQ;           \
          ffz += fdzdx;          \
          ss += dsdx;               \
          tt += dtdx;               \
          uu += dudx;               \
          vv += dvdx;               \
       }                \
              }                     \
              else {                \
                 for (i=0;i<n;i++) {            \
          GLdouble invQ = 1.0 / vv;       \
          zspan[i] = FixedToDepth(ffz);      \
          rgba[i][RCOMP] = FixedToInt(ffr);     \
          rgba[i][GCOMP] = FixedToInt(ffg);     \
          rgba[i][BCOMP] = FixedToInt(ffb);     \
          rgba[i][ACOMP] = FixedToInt(ffa);     \
          s[i] = ss*invQ;           \
          t[i] = tt*invQ;           \
          u[i] = uu*invQ;           \
          ffz += fdzdx;          \
          ffr += fdrdx;          \
          ffg += fdgdx;          \
          ffb += fdbdx;          \
          ffa += fdadx;          \
          ss += dsdx;               \
          tt += dtdx;               \
          uu += dudx;               \
          vv += dvdx;               \
       }                \
              }                     \
         gl_write_texture_span( ctx, n, LEFT, Y, zspan,  \
                                     s, t, u, NULL,      \
                                rgba, NULL, GL_POLYGON );   \
      }                    \
   }

#include "tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * color interpolation.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_spec_triangle1( GLcontext *ctx, GLuint v0,
                                             GLuint v1, GLuint v2, GLuint pv,
                                             GLdepth zspan[MAX_WIDTH],
                                             GLubyte rgba[MAX_WIDTH][4],
                                             GLubyte spec[MAX_WIDTH][4] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define SETUP_CODE                  \
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT); \
   GLint r, g, b, a, sr, sg, sb;          \
   if (flat_shade) {                \
      r = VB->Color[pv][0];               \
      g = VB->Color[pv][1];               \
      b = VB->Color[pv][2];               \
      a = VB->Color[pv][3];               \
      sr = VB->Specular[pv][0];           \
      sg = VB->Specular[pv][1];           \
      sb = VB->Specular[pv][2];           \
   }
#define INNER_LOOP( LEFT, RIGHT, Y )            \
   {                    \
      GLint i, n = RIGHT-LEFT;            \
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];   \
      if (n>0) {                 \
              if (flat_shade) {              \
                 for (i=0;i<n;i++) {            \
          GLdouble invQ = 1.0 / vv;       \
          zspan[i] = FixedToDepth(ffz);      \
          rgba[i][RCOMP] = r;          \
          rgba[i][GCOMP] = g;          \
          rgba[i][BCOMP] = b;          \
          rgba[i][ACOMP] = a;          \
          spec[i][RCOMP] = sr;         \
          spec[i][GCOMP] = sg;         \
          spec[i][BCOMP] = sb;         \
          s[i] = ss*invQ;           \
          t[i] = tt*invQ;           \
          u[i] = uu*invQ;           \
          ffz += fdzdx;          \
          ss += dsdx;               \
          tt += dtdx;               \
          uu += dudx;               \
          vv += dvdx;               \
       }                \
              }                     \
              else {                \
                 for (i=0;i<n;i++) {            \
          GLdouble invQ = 1.0 / vv;       \
          zspan[i] = FixedToDepth(ffz);      \
          rgba[i][RCOMP] = FixedToInt(ffr);     \
          rgba[i][GCOMP] = FixedToInt(ffg);     \
          rgba[i][BCOMP] = FixedToInt(ffb);     \
          rgba[i][ACOMP] = FixedToInt(ffa);     \
          spec[i][RCOMP] = FixedToInt(ffsr);    \
          spec[i][GCOMP] = FixedToInt(ffsg);    \
          spec[i][BCOMP] = FixedToInt(ffsb);    \
          s[i] = ss*invQ;           \
          t[i] = tt*invQ;           \
          u[i] = uu*invQ;           \
          ffz += fdzdx;          \
          ffr += fdrdx;          \
          ffg += fdgdx;          \
          ffb += fdbdx;          \
          ffa += fdadx;          \
          ffsr += fdsrdx;           \
          ffsg += fdsgdx;           \
          ffsb += fdsbdx;           \
          ss += dsdx;               \
          tt += dtdx;               \
          uu += dudx;               \
          vv += dvdx;               \
       }                \
              }                     \
         gl_write_texture_span( ctx, n, LEFT, Y, zspan,  \
                                     s, t, u, NULL,      \
                                rgba, spec, GL_POLYGON );   \
      }                    \
   }

#include "tritemp.h"
}



/*
 * Compute the lambda value (texture level value) for a fragment.
 */
static GLfloat compute_lambda( GLfloat s, GLfloat dsdx, GLfloat dsdy,
                               GLfloat t, GLfloat dtdx, GLfloat dtdy,
                               GLfloat invQ, GLfloat dqdx, GLfloat dqdy,
                               GLfloat width, GLfloat height ) 
{
   GLfloat dudx, dudy, dvdx, dvdy;
   GLfloat r1, r2, rho2;
   GLfloat invQ_width = invQ * width;
   GLfloat invQ_height = invQ * height;

   dudx = (dsdx - s*dqdx) * invQ_width;
   dudy = (dsdy - s*dqdy) * invQ_width;
   dvdx = (dtdx - t*dqdx) * invQ_height;
   dvdy = (dtdy - t*dqdy) * invQ_height;

   r1 = dudx * dudx + dudy * dudy;
   r2 = dvdx * dvdx + dvdy * dvdy;

   rho2 = r1 + r2;     /* used to be:  rho2 = MAX2(r1,r2); */
   ASSERT( rho2 >= 0.0 );

   /* return log base 2 of rho */
   return log(rho2) * 1.442695 * 0.5;       /* 1.442695 = 1/log(2) */
}



/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_triangle1( GLcontext *ctx, GLuint v0, GLuint v1,
                                       GLuint v2, GLuint pv,
                                       GLfloat s[MAX_WIDTH],
                                       GLfloat t[MAX_WIDTH],
                                       GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1

#define SETUP_CODE                     \
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);    \
   GLint r, g, b, a;                   \
   GLfloat twidth, theight;                  \
   if (ctx->Texture.Enabled & TEXTURE0_3D) {          \
      twidth = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Width;   \
      theight = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Height; \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE0_2D) {        \
      twidth = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Width;   \
      theight = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Height; \
   }                          \
   else {                        \
      twidth = (GLfloat) ctx->Texture.Set[0].Current1D->Image[0]->Width;   \
      theight = 1.0;                   \
   }                          \
   if (flat_shade) {                   \
      r = VB->Color[pv][0];                  \
      g = VB->Color[pv][1];                  \
      b = VB->Color[pv][2];                  \
      a = VB->Color[pv][3];                  \
   }

#define INNER_LOOP( LEFT, RIGHT, Y )               \
   {                       \
      GLint i, n = RIGHT-LEFT;               \
      GLdepth zspan[MAX_WIDTH];              \
      GLubyte rgba[MAX_WIDTH][4];               \
      GLfloat lambda[MAX_WIDTH];             \
      if (n>0) {                    \
         if (flat_shade) {                \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = r;             \
          rgba[i][GCOMP] = g;             \
          rgba[i][BCOMP] = b;             \
          rgba[i][ACOMP] = a;             \
          s[i] = ss*invQ;              \
          t[i] = tt*invQ;              \
          u[i] = uu*invQ;              \
          lambda[i] = compute_lambda( s[i], dsdx, dsdy,  \
                  t[i], dtdx, dtdy, \
                  invQ, dvdx, dvdy, \
                  twidth, theight );   \
          ffz += fdzdx;             \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
       }                   \
              }                        \
              else {                   \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = FixedToInt(ffr);        \
          rgba[i][GCOMP] = FixedToInt(ffg);        \
          rgba[i][BCOMP] = FixedToInt(ffb);        \
          rgba[i][ACOMP] = FixedToInt(ffa);        \
          s[i] = ss*invQ;              \
          t[i] = tt*invQ;              \
          u[i] = uu*invQ;              \
          lambda[i] = compute_lambda( s[i], dsdx, dsdy,  \
                  t[i], dtdx, dtdy, \
                  invQ, dvdx, dvdy, \
                  twidth, theight );   \
          ffz += fdzdx;             \
          ffr += fdrdx;             \
          ffg += fdgdx;             \
          ffb += fdbdx;             \
          ffa += fdadx;             \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
       }                   \
              }                        \
         gl_write_texture_span( ctx, n, LEFT, Y, zspan,     \
                                     s, t, u, lambda,       \
                                rgba, NULL, GL_POLYGON );      \
      }                       \
   }

#include "tritemp.h"
}



/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * interpolation.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_spec_triangle1( GLcontext *ctx, GLuint v0,
                                            GLuint v1, GLuint v2, GLuint pv,
                                            GLfloat s[MAX_WIDTH],
                                            GLfloat t[MAX_WIDTH],
                                            GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1

#define SETUP_CODE                     \
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);    \
   GLint r, g, b, a, sr, sg, sb;             \
   GLfloat twidth, theight;                  \
   if (ctx->Texture.Enabled & TEXTURE0_3D) {          \
      twidth = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Width;   \
      theight = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Height; \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE0_2D) {        \
      twidth = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Width;   \
      theight = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Height; \
   }                          \
   else {                        \
      twidth = (GLfloat) ctx->Texture.Set[0].Current1D->Image[0]->Width;   \
      theight = 1.0;                   \
   }                          \
   if (flat_shade) {                   \
      r = VB->Color[pv][0];                  \
      g = VB->Color[pv][1];                  \
      b = VB->Color[pv][2];                  \
      a = VB->Color[pv][3];                  \
      sr = VB->Specular[pv][0];                 \
      sg = VB->Specular[pv][1];                 \
      sb = VB->Specular[pv][2];                 \
   }

#define INNER_LOOP( LEFT, RIGHT, Y )               \
   {                       \
      GLint i, n = RIGHT-LEFT;               \
      GLdepth zspan[MAX_WIDTH];              \
      GLubyte spec[MAX_WIDTH][4];               \
           GLubyte rgba[MAX_WIDTH][4];             \
      GLfloat lambda[MAX_WIDTH];             \
      if (n>0) {                    \
         if (flat_shade) {                \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = r;             \
          rgba[i][GCOMP] = g;             \
          rgba[i][BCOMP] = b;             \
          rgba[i][ACOMP] = a;             \
          spec[i][RCOMP] = sr;            \
          spec[i][GCOMP] = sg;            \
          spec[i][BCOMP] = sb;            \
          s[i] = ss*invQ;              \
          t[i] = tt*invQ;              \
          u[i] = uu*invQ;              \
          lambda[i] = compute_lambda( s[i], dsdx, dsdy,  \
                  t[i], dtdx, dtdy, \
                  invQ, dvdx, dvdy, \
                  twidth, theight );   \
          ffz += fdzdx;             \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
       }                   \
              }                        \
              else {                   \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = FixedToInt(ffr);        \
          rgba[i][GCOMP] = FixedToInt(ffg);        \
          rgba[i][BCOMP] = FixedToInt(ffb);        \
          rgba[i][ACOMP] = FixedToInt(ffa);        \
          spec[i][RCOMP] = FixedToInt(ffsr);       \
          spec[i][GCOMP] = FixedToInt(ffsg);       \
          spec[i][BCOMP] = FixedToInt(ffsb);       \
          s[i] = ss*invQ;              \
          t[i] = tt*invQ;              \
          u[i] = uu*invQ;              \
          lambda[i] = compute_lambda( s[i], dsdx, dsdy,  \
                  t[i], dtdx, dtdy, \
                  invQ, dvdx, dvdy, \
                  twidth, theight );   \
          ffz += fdzdx;             \
          ffr += fdrdx;             \
          ffg += fdgdx;             \
          ffb += fdbdx;             \
          ffa += fdadx;             \
          ffsr += fdsrdx;              \
          ffsg += fdsgdx;              \
          ffsb += fdsbdx;              \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
       }                   \
              }                        \
         gl_write_texture_span( ctx, n, LEFT, Y, zspan,     \
                                     s, t, u, lambda,       \
                                rgba, spec, GL_POLYGON );      \
      }                       \
   }

#include "tritemp.h"
}



/*
 * This is the big one!
 * Interpolate Z, RGB, Alpha, and two sets of texture coordinates.
 * Yup, it's slow.
 */
static void lambda_multitextured_triangle1( GLcontext *ctx, GLuint v0,
                                      GLuint v1, GLuint v2, GLuint pv,
                                      GLubyte rgba[MAX_WIDTH][4],
                                      GLfloat s[MAX_TEX_COORD_SETS][MAX_WIDTH],
                                      GLfloat t[MAX_TEX_COORD_SETS][MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define INTERP_STUV1 1

#define SETUP_CODE                     \
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);    \
   GLint r, g, b, a;                   \
   GLfloat twidth0, theight0;                \
   GLfloat twidth1, theight1;                \
   if (ctx->Texture.Enabled & TEXTURE0_3D) {          \
      twidth0 = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Width;  \
      theight0 = (GLfloat) ctx->Texture.Set[0].Current3D->Image[0]->Height;   \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE0_2D) {        \
      twidth0 = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Width;  \
      theight0 = (GLfloat) ctx->Texture.Set[0].Current2D->Image[0]->Height;   \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE0_1D) {        \
      twidth0 = (GLfloat) ctx->Texture.Set[0].Current1D->Image[0]->Width;  \
      theight0 = 1.0;                     \
   }                          \
   if (ctx->Texture.Enabled & TEXTURE1_3D) {          \
      twidth1 = (GLfloat) ctx->Texture.Set[1].Current3D->Image[0]->Width;  \
      theight1 = (GLfloat) ctx->Texture.Set[1].Current3D->Image[0]->Height;   \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE1_2D) {        \
      twidth1 = (GLfloat) ctx->Texture.Set[1].Current2D->Image[0]->Width;  \
      theight1 = (GLfloat) ctx->Texture.Set[1].Current2D->Image[0]->Height;   \
   }                          \
   else if (ctx->Texture.Enabled & TEXTURE1_1D) {        \
      twidth1 = (GLfloat) ctx->Texture.Set[1].Current1D->Image[0]->Width;  \
      theight1 = 1.0;                     \
   }                          \
   if (flat_shade) {                   \
      r = VB->Color[pv][0];                  \
      g = VB->Color[pv][1];                  \
      b = VB->Color[pv][2];                  \
      a = VB->Color[pv][3];                  \
   }

#define INNER_LOOP( LEFT, RIGHT, Y )               \
   {                       \
      GLint i, n = RIGHT-LEFT;               \
      GLdepth zspan[MAX_WIDTH];              \
           GLfloat u[MAX_TEX_COORD_SETS][MAX_WIDTH];        \
           GLfloat lambda[MAX_TEX_COORD_SETS][MAX_WIDTH];      \
      if (n>0) {                    \
         if (flat_shade) {                \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          GLdouble invQ1 = 1.0 / vv1;           \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = r;             \
          rgba[i][GCOMP] = g;             \
          rgba[i][BCOMP] = b;             \
          rgba[i][ACOMP] = a;             \
          s[0][i] = ss*invQ;              \
          t[0][i] = tt*invQ;              \
          u[0][i] = uu*invQ;              \
          lambda[0][i] = compute_lambda( s[0][i], dsdx, dsdy,  \
                     t[0][i], dtdx, dtdy, \
                     invQ, dvdx, dvdy, \
                     twidth0, theight0 ); \
          s[1][i] = ss1*invQ1;            \
          t[1][i] = tt1*invQ1;            \
          u[1][i] = uu1*invQ1;            \
          lambda[1][i] = compute_lambda( s[1][i], ds1dx, ds1dy,   \
                     t[1][i], dt1dx, dt1dy,  \
                     invQ, dvdx, dvdy, \
                     twidth1, theight1 ); \
          ffz += fdzdx;             \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
          ss1 += ds1dx;             \
          tt1 += dt1dx;             \
          uu1 += du1dx;             \
          vv1 += dv1dx;             \
       }                   \
              }                        \
              else {                   \
       for (i=0;i<n;i++) {             \
          GLdouble invQ = 1.0 / vv;          \
          GLdouble invQ1 = 1.0 / vv1;           \
          zspan[i] = FixedToDepth(ffz);         \
          rgba[i][RCOMP] = FixedToInt(ffr);        \
          rgba[i][GCOMP] = FixedToInt(ffg);        \
          rgba[i][BCOMP] = FixedToInt(ffb);        \
          rgba[i][ACOMP] = FixedToInt(ffa);        \
          s[0][i] = ss*invQ;              \
          t[0][i] = tt*invQ;              \
          u[0][i] = uu*invQ;              \
          lambda[0][i] = compute_lambda( s[0][i], dsdx, dsdy,  \
                     t[0][i], dtdx, dtdy, \
                     invQ, dvdx, dvdy, \
                     twidth0, theight0 ); \
          s[1][i] = ss1*invQ1;            \
          t[1][i] = tt1*invQ1;            \
          u[1][i] = uu1*invQ1;            \
          lambda[1][i] = compute_lambda( s[1][i], ds1dx, ds1dy,   \
                     t[1][i], dt1dx, dt1dy,  \
                     invQ1, dvdx, dvdy,   \
                     twidth1, theight1 ); \
          ffz += fdzdx;             \
          ffr += fdrdx;             \
          ffg += fdgdx;             \
          ffb += fdbdx;             \
          ffa += fdadx;             \
          ss += dsdx;                  \
          tt += dtdx;                  \
          uu += dudx;                  \
          vv += dvdx;                  \
          ss1 += ds1dx;             \
          tt1 += dt1dx;             \
          uu1 += du1dx;             \
          vv1 += dv1dx;             \
       }                   \
              }                        \
         gl_write_multitexture_span( ctx, 2, n, LEFT, Y, zspan,   \
                                          s, t, u, lambda,     \
                                     rgba, NULL, GL_POLYGON ); \
      }                       \
   }

#include "tritemp.h"
}


/*
 * These wrappers are needed to deal with the 32KB / stack frame limit
 * on Mac / PowerPC systems.
 */

static void general_textured_spec_triangle(GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv)
{
   GLdepth zspan[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4], spec[MAX_WIDTH][4];
   general_textured_spec_triangle1(ctx,v0,v1,v2,pv,zspan,rgba,spec);
}

static void lambda_textured_triangle( GLcontext *ctx, GLuint v0,
                                      GLuint v1, GLuint v2, GLuint pv )
{
   GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];
   lambda_textured_triangle1(ctx,v0,v1,v2,pv,s,t,u);
}

static void lambda_textured_spec_triangle( GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv )
{
   GLfloat s[MAX_WIDTH];
   GLfloat t[MAX_WIDTH];
   GLfloat u[MAX_WIDTH];
   lambda_textured_spec_triangle1(ctx,v0,v1,v2,pv,s,t,u);
}

static void lambda_multitextured_triangle( GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv)
{
   GLubyte rgba[MAX_WIDTH][4];
   GLfloat s[MAX_TEX_COORD_SETS][MAX_WIDTH];
   GLfloat t[MAX_TEX_COORD_SETS][MAX_WIDTH];
   lambda_multitextured_triangle1(ctx,v0,v1,v2,pv,rgba,s,t);
}



/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                           GLuint v2, GLuint pv )
{
   (void) ctx;
   (void) v0;
   (void) v1;
   (void) v2;
   (void) pv;
}


#if 0
# define dputs(s) puts(s)
#else
# define dputs(s)
#endif

/*
 * Determine which triangle rendering function to use given the current
 * rendering context.
 */
void gl_set_triangle_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;

   if ( ctx->RenderMode==GL_RENDER ) 
   {
      if ( ctx->NoRaster ) 
      {
         ctx->Driver.TriangleFunc = null_triangle;
         return;
      }

      if ( ctx->Driver.TriangleFunc ) 
      {
         /* Device driver will draw triangles. */
      }
      else if ( ctx->Texture.Enabled ) 
      {
         /* Ugh, we do a _lot_ of tests to pick the best textured tri func */
         int   format, 
               filter;
         const struct gl_texture_object   *current2Dtex = ctx->Texture.Set[0].Current2D;
         const struct gl_texture_image    *image;

         /* First see if we can used an optimized 2-D texture function */
         if ( (ctx->Texture.Enabled == TEXTURE0_2D) && 
              (ctx->Texture.Set[0].Current) && (ctx->Texture.Set[0].Current->Complete) && 
              (current2Dtex->WrapS==GL_REPEAT) && (current2Dtex->WrapT==GL_REPEAT) && 
              (image = current2Dtex->Image[current2Dtex->BaseLevel]) && /* correct? */
              (image->Border == 0) && 
              ((format = image->Format)== GL_RGB || format == GL_RGBA) &&
              /* && ctx->TextureMatrixType[0]==MATRIX_IDENTITY  -- OK? */
              ((filter = current2Dtex->MinFilter)==current2Dtex->MagFilter)  &&
              (ctx->Light.Model.ColorControl==GL_SINGLE_COLOR )) 
         {


            if (ctx->Hint.PerspectiveCorrection == GL_FASTEST) 
            {
               if ( (filter == GL_NEAREST) && 
                    (format == GL_RGB) && 
                    (ctx->Texture.Set[0].EnvMode==GL_REPLACE || ctx->Texture.Set[0].EnvMode==GL_DECAL) &&
                    ((ctx->RasterMask==DEPTH_BIT   && ctx->Depth.Func==GL_LESS && ctx->Depth.Mask==GL_TRUE) || ctx->RasterMask==0) &&
                    (ctx->Polygon.StippleFlag==GL_FALSE) ) 
               {
                  if (ctx->RasterMask == DEPTH_BIT) 
                  {
                     ctx->Driver.TriangleFunc = simple_z_textured_triangle;
                     dputs("simple_z_textured_triangle");
                  }
                  else 
                  {
                     ctx->Driver.TriangleFunc = simple_textured_triangle;
                     dputs("simple_textured_triangle");
                  }
               }
               else 
               {
                     ctx->Driver.TriangleFunc = affine_textured_triangle;
                     dputs("affine_textured_triangle");
               }
            }
            else 
            {
               ctx->Driver.TriangleFunc = persp_textured_triangle;
               dputs("persp_textured_triangle");
            }
         }
         else 
         {
            /* More complicated textures (mipmap, multi-tex, sep specular) */
            GLboolean needLambda = GL_FALSE;
            /* if mag filter != min filter we need to compute lambda */
            GLuint   i;

            for( i = 0; i < MAX_TEX_SETS; i++ ) 
            {
               GLuint mask = (TEXTURE0_1D|TEXTURE0_2D|TEXTURE0_3D) << (i*4);
               if ( ctx->Texture.Enabled & mask ) 
               {
                  if (ctx->Texture.Set[i].Current->MinFilter != ctx->Texture.Set[i].Current->MagFilter) 
                  {
                     needLambda = GL_TRUE;
                  }
               }
            }
            if (ctx->Texture.Enabled >= TEXTURE1_1D) 
            {
               /* multi-texture! */
               ctx->Driver.TriangleFunc = lambda_multitextured_triangle;
               dputs("lambda_multitextured_triangle");
            }
            else if ( ctx->Light.Model.ColorControl==GL_SINGLE_COLOR ) 
            {
               if ( needLambda ) 
               {
                  ctx->Driver.TriangleFunc = lambda_textured_triangle;
                  dputs("lambda_textured_triangle");
               }
               else 
               {
                  ctx->Driver.TriangleFunc = general_textured_triangle;
                  dputs("general_textured_triangle");
               }
            }
            else 
            {
               /* seprate specular color interpolation */
               if ( needLambda ) 
               {
                  ctx->Driver.TriangleFunc = lambda_textured_spec_triangle;
                  dputs("lambda_textured_spec_triangle");
               }
               else 
               {
                  ctx->Driver.TriangleFunc = general_textured_spec_triangle;
                  dputs("general_textured_spec_triangle");
               }
            }
         }
      }
      else 
      {
         if ( ctx->Light.ShadeModel==GL_SMOOTH ) 
         {
            /* smooth shaded, no texturing, stippled or some raster ops */
            if ( rgbmode )
               ctx->Driver.TriangleFunc = smooth_rgba_triangle;
            else
               ctx->Driver.TriangleFunc = smooth_ci_triangle;
         }
         else 
         {
            /* flat shaded, no texturing, stippled or some raster ops */
            if ( rgbmode )
               ctx->Driver.TriangleFunc = flat_rgba_triangle;
            else
               ctx->Driver.TriangleFunc = flat_ci_triangle;
         }
      }
   }
   else if ( ctx->RenderMode==GL_FEEDBACK ) 
   {
      ctx->Driver.TriangleFunc = feedback_triangle;
   }
   else 
   {
      /* GL_SELECT mode */
      ctx->Driver.TriangleFunc = select_triangle;
   }
}
