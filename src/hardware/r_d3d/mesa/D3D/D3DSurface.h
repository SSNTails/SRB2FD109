/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                       Build 0.7 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#ifndef D3D_SURFACE_H
#define D3D_SURFACE_H
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#define	DIRECTDRAW_VERSION	0x0600
#include <ddraw.h>
#define	DIRECT3D_VERSION		0x0600
#include <d3d.h>
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
#define FORCE_ALPHA_ONE      	0x0001
#define FORCE_ALPHA_ZERO       	0x0002
#define FORCE_ALPHA_INTENSITY	0x0004
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
typedef void (*FILL_SURFACE_FUNC)( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
/*===========================================================================*/
/* Extern function prototypes.                                               */
/*===========================================================================*/
FILL_SURFACE_FUNC GetFillFunction( LPDIRECTDRAWSURFACE4 lpDDS );

void FillSurfaceGeneric( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
void FillSurface565( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
void FillSurface4444( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
void FillSurface1555( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
void FillSurface888( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
void FillSurface8888( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/*  See RETURN :)                                                            */
/*===========================================================================*/
/* RETURN: number of contiguous zeros starting from the right.               */
/*===========================================================================*/
inline int CountTrailingZeros( DWORD dwMask )
{
  DWORD Mask;

  if ( dwMask == 0 ) 
    return 32;

  /* Can't take credit for this one! */
  Mask = dwMask & -(int)dwMask;
  return ((Mask & 0xFFFF0000)!=0) << 4
         | ((Mask & 0xFF00FF00)!=0) << 3
         | ((Mask & 0xF0F0F0F0)!=0) << 2
         | ((Mask & 0xCCCCCCCC)!=0) << 1
         | ((Mask & 0xAAAAAAAA)!=0);
}

#endif




