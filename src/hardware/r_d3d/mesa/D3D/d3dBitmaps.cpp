/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                       Build 0.5 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <stdio.h>
#include "clip.h"
#include "context.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "pb.h"
#include "points.h"
#include "types.h"
#include "vb.h"
#include "vbrender.h"
#include "xform.h"
#include "image.h"
#include "D3DMesa.h"
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
/*===========================================================================*/
/* Local function prototypes.                                                */
/*===========================================================================*/
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
/*===========================================================================*/
/*  This function renders a bitmap using D3D points.  It could be done better*/
/* using triangles and some kind of run length encoding of pixels.  For now  */
/* this work as we can't use direct buffer access because bitmaps use HSR.   */
/*  I made the point buffer 64 just because it will fit a 8x8 font nicely so */
/* why not...                                                                */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
GLboolean RenderBitmap( GLcontext *ctx,	GLint x, GLint y, GLsizei width, GLsizei height,
			const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap )
{
  static D3DTLVERTEX	TLVertice[64];
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  GLint			cVertice, 
                        row, 
                        col,
                        r, g, b, a;
  DWORD			dwColor;
  float 		pz;
  GLubyte		bitmask;

  DPF(( DBG_FUNC, "   d3d--> RenderBitmap" ));

  /* Check for a NULL bitmap. */
  if ( bitmap == NULL )
    return FALSE;

  /* Make sure our states are current. */
  if ( ctx->NewState ) 
  {
    gl_update_state(ctx);
  }

  /* I only support RGBA. */
  dwColor = ((DWORD)(ctx->Current.RasterColor[3] * 255)<<24) | 
            ((DWORD)(ctx->Current.RasterColor[0] * 255)<<16) | 
            ((DWORD)(ctx->Current.RasterColor[1] * 255)<<8)  | 
             (DWORD)(ctx->Current.RasterColor[2] * 255);

  /* Get the current Z value. */
  pz = (float)( ctx->Current.RasterPos[2] * DEPTH_SCALE );

  for( row = 0, cVertice = -1; row < height; row++ ) 
  {
    /* Unpack the bitmap so that we can render it. */
    GLubyte *src = (GLubyte *)gl_pixel_addr_in_image( unpack, 
						      bitmap, 
						      width, 
						      height, 
						      GL_COLOR_INDEX, 
						      GL_BITMAP, 
						      0, 
						      row, 
						      0 );

    if ( unpack->LsbFirst ) 
    {
      /* Lsb first */
      bitmask = 1;

      for( col = 0; col < width; col++ ) 
      {
	if ( *src & bitmask ) 
	{
	  if ( ++cVertice == (sizeof(TLVertice) / sizeof(D3DTLVERTEX)) )
	  {
	    pContext->lpD3DDevice->DrawPrimitive( D3DPT_POINTLIST,
						  D3DFVF_TLVERTEX,
						  (LPVOID)&TLVertice[0],
						  cVertice, 
						  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
	    cVertice = 0;
	  }

	  TLVertice[cVertice].sx    = (float)(x + col);
	  TLVertice[cVertice].sy    = (float)FLIP( (y + row)  );
	  TLVertice[cVertice].sz    = pz;
	  TLVertice[cVertice].color = dwColor;
	}

	bitmask = bitmask << 1;
	if ( bitmask==0 ) 
	{
	  src++;
	  bitmask = 1;
	}
      }

      /* get ready for next row */
      if (bitmask!=1)
	src++;
    }
    else 
    {
      /* Msb first */
      bitmask = 128;

      for( col = 0; col < width; col++ ) 
      {
	if ( *src & bitmask ) 
	{
	  if ( ++cVertice == (sizeof(TLVertice) / sizeof(D3DTLVERTEX)) )
	  {
	    pContext->lpD3DDevice->DrawPrimitive( D3DPT_POINTLIST,
						  D3DFVF_TLVERTEX,
						  (LPVOID)&TLVertice[0],
						  cVertice, 
						  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
	    cVertice = 0;
	  }

	  TLVertice[cVertice].sx    = (float)(x + col);
	  TLVertice[cVertice].sy    = (float)FLIP( (y + row)  );
	  TLVertice[cVertice].sz    = pz;
	  TLVertice[cVertice].color = dwColor;
	}
	bitmask = bitmask >> 1;
	if ( bitmask == 0 ) 
	{
	  src++;
	  bitmask = 128;
	}
      }

      /* get ready for next row */
      if (bitmask!=128)
	src++;
    }
  }

  /* Flush the buffer. */
  if ( cVertice > 0 )
  {
    pContext->lpD3DDevice->DrawPrimitive( D3DPT_POINTLIST,
					  D3DFVF_TLVERTEX,
					  (LPVOID)&TLVertice[0],
					  cVertice, 
					  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  }

  DPF(( DBG_FUNC, "<--d3d    RenderBitmap" ));

  return TRUE;
}



