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
#include "D3DMesa.h"
/*===========================================================================*/
/* Local only functions.                                                     */
/*===========================================================================*/
/*===========================================================================*/
/* Globals.                                                                  */
/*===========================================================================*/
long	int	x1 = 0,
                x2 = 0,
                x3 = 0,
                t1 = 0,
                t2 = 0,
                t3 = 0;
/*===========================================================================*/
/*  This function takes a surface pointer and returns a pointer to a function*/
/* that can be used to fill the surface when OGL needs to.  The function will*/
/* find a fast path function if one exists but will fall back to the generic */
/* fill function that should work for all pixelformats.                      */
/*===========================================================================*/
/* RETURN: function pointer, NULL.                                           */
/*===========================================================================*/
FILL_SURFACE_FUNC GetFillFunction( LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  FILL_SURFACE_FUNC	func;
  HRESULT		rc;      

  DPF(( DBG_FUNC, "   SUF--> GetFillFunction" ));

#ifdef D3D_DEBUG
  /* Lock the surface so we can get some attributes. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);
  rc = lpDDS->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***SUF*** Lock() <%s>.", ErrorStringD3D(rc) ));
    return NULL;
  }

  /* Get a copy of the pixelformat. */
  rc = lpDDS->GetPixelFormat( &ddsd2.ddpfPixelFormat );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***SUF*** GetPixelFormat() <%s>.", ErrorStringD3D(rc) ));
    return NULL;
  }
#else
  /* Lock the surface so we can get some attributes. */
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);
  lpDDS->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );
  lpDDS->GetPixelFormat( &ddsd2.ddpfPixelFormat );
#endif

  /* Use this as the default. */
  func = FillSurfaceGeneric;

  /* Search for a fast fill function. */
  switch( ddsd2.ddpfPixelFormat.dwFlags )
  {
    case (DDPF_RGB | DDPF_ALPHAPIXELS):
      switch( ddsd2.ddpfPixelFormat.dwRGBBitCount )
      {	
	case 32:
	  if ( (ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF000000) &&
	       (ddsd2.ddpfPixelFormat.dwRBitMask == 0x00FF0000) &&
	       (ddsd2.ddpfPixelFormat.dwGBitMask == 0x0000FF00) &&
	       (ddsd2.ddpfPixelFormat.dwBBitMask == 0x000000FF) )
	    func = FillSurface8888;
	  break;

	case 16:
	  if ( (ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask == 0xF000) &&
	       (ddsd2.ddpfPixelFormat.dwRBitMask == 0x0F00) &&
	       (ddsd2.ddpfPixelFormat.dwGBitMask == 0x00F0) &&
	       (ddsd2.ddpfPixelFormat.dwBBitMask == 0x000F) )
	    func = FillSurface4444;
	  else if ( (ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask == 0x8000) &&
	       (ddsd2.ddpfPixelFormat.dwRBitMask == 0x7C00) &&
	       (ddsd2.ddpfPixelFormat.dwGBitMask == 0x03E0) &&
	       (ddsd2.ddpfPixelFormat.dwBBitMask == 0x001F) )
	    func = FillSurface1555;
	  break;
      }
      break;

    case DDPF_RGB:
      switch( ddsd2.ddpfPixelFormat.dwRGBBitCount )
      {	
	case 24:
	  if ( (ddsd2.ddpfPixelFormat.dwRBitMask == 0xFF0000) &&
	       (ddsd2.ddpfPixelFormat.dwGBitMask == 0x00FF00) &&
	       (ddsd2.ddpfPixelFormat.dwBBitMask == 0x0000FF) )
	    func = FillSurface888;
	  break;

	case 16:
	  if ( (ddsd2.ddpfPixelFormat.dwRBitMask == 0xF800) &&
	       (ddsd2.ddpfPixelFormat.dwGBitMask == 0x07E0) &&
	       (ddsd2.ddpfPixelFormat.dwBBitMask == 0x001F) )
	    func = FillSurface565;
	  break;
      }
      break;
  }

#ifdef D3D_DEBUG
  /* Release the surface back to DDraw. */
  rc = lpDDS->Unlock( NULL );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) ));
    return NULL;
  }
#else
  lpDDS->Unlock( NULL );
#endif

  DPF(( DBG_FUNC, "<--SUF    GetFillFunction" ));

  return func;
}
/*===========================================================================*/
/*  This function attempts to try and fill all pixelformat types.  First I   */
/* get the pixel format from the surface.  Using the PF I get a scale value  */
/* for each color component that will change a 8bit value to whatever this PF*/
/* uses.  Next I compute (I like that word) the shift position for the color */
/* component.  Now armed with this information I can fill a generic surface. */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurfaceGeneric( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			dwCol,
                        dwIntensity,
                        dwColor;
  GLubyte 		*pDest,
                        *pSrcRow,
                        *pDestRow;
  float			rScale,
                        gScale,
                        bScale,
                        aScale;
  DWORD			rShift,
                        gShift,
                        bShift,
                        aShift,
                        cbSize;
  ULONG			rc;

  PROFILE_START;

  /*=======================*/
  /* Lock the D3D surface. */
  /*=======================*/
#ifdef D3D_DEBUG
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					

  /* Get the destination pointer. */
  pDest = (GLubyte *)ddsd2.lpSurface;

  /*================================================*/
  /* Solve the pixel format sclae and shift values. */
  /*================================================*/
	 
  /* Get a copy of the pixelformat. */
  rc = lpDDS->GetPixelFormat( &ddsd2.ddpfPixelFormat );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***SUF*** GetPixelFormat() <%s>.", ErrorStringD3D(rc) ));
    return;
  }
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );	

  /* Get the destination pointer. */
  pDest = (GLubyte *)ddsd2.lpSurface;

  /* Get a copy of the pixelformat. */
  lpDDS->GetPixelFormat( &ddsd2.ddpfPixelFormat );
#endif

  /* Get the size of the pixel in bytes. */
  cbSize = ddsd2.ddpfPixelFormat.dwRGBBitCount >> 3;

  /* Generate pixel info for the RGB components. */
  if ( ddsd2.ddpfPixelFormat.dwFlags & DDPF_RGB )
  {
    /* Solve the red stuff. */
    rShift = CountTrailingZeros( ddsd2.ddpfPixelFormat.dwRBitMask );
    rScale = (1.0f / 255.0f) * (float)(ddsd2.ddpfPixelFormat.dwRBitMask >> rShift);

    /* Solve the green thingy's. */
    gShift = CountTrailingZeros( ddsd2.ddpfPixelFormat.dwGBitMask );
    gScale = (1.0f / 255.0f) * (float)(ddsd2.ddpfPixelFormat.dwGBitMask >> gShift);

    /* Solve the blues. */
    bShift = CountTrailingZeros( ddsd2.ddpfPixelFormat.dwBBitMask );
    bScale = (1.0f / 255.0f) * (float)(ddsd2.ddpfPixelFormat.dwBBitMask >> bShift);
  }

  /* Generate pixel info for the alpha if there is one. */
  if ( ddsd2.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS )
  {
    /* Solve the alpha shift. */
    aShift = CountTrailingZeros( ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask );

    /* Special case a 1bit alpha. */
    if ( (ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask >> aShift) == 1 )
      aScale = -1.0;
    else
      aScale = (float)0.00392156 * (float)(ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask >> aShift);
  }	

  /* Fill the texture surface based on the pixelformat flags. */
  switch( ddsd2.ddpfPixelFormat.dwFlags )
  {
    case (DDPF_RGB | DDPF_ALPHAPIXELS):
      srcPitch *= 4;
      pSrc     += ((y * srcPitch)     + (x * 4));
      pDest    += ((y * ddsd2.lPitch) + (x * cbSize));
    
      for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
      {
	for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
	{
	  dwColor =  ( ((DWORD)(*(pSrc  ) * rScale)) << rShift );
	  dwColor |= ( ((DWORD)(*(pSrc+1) * gScale)) << gShift );
	  dwColor |= ( ((DWORD)(*(pSrc+2) * bScale)) << bShift );

	  /* This is a 1bit ALPHA check. */
	  if ( aScale == -1.0 )
	    dwColor |= ( (*(pSrc+3) & 0x80) ? (1 << aShift) : 0 );
	  else
	    dwColor |= ( ((DWORD)(*(pSrc+3) * aScale)) << aShift );

	  memcpy( pDest, &dwColor, cbSize );
	  pSrc  += 4;
	  pDest += cbSize;
	}
      }
      break;

    case DDPF_RGB:
      srcPitch *= 3;
      pSrc     += ((y * cx * 3)       + (x * 3));
      pDest    += ((y * ddsd2.lPitch) + (x * cbSize));

      for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
      {
	for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
	{
	  dwColor =  ( ((DWORD)(*(pSrc  ) * rScale)) << rShift );
	  dwColor |= ( ((DWORD)(*(pSrc+1) * gScale)) << gShift );
	  dwColor |= ( ((DWORD)(*(pSrc+2) * bScale)) << bShift );

	  memcpy( pDest, &dwColor, cbSize );
	  pSrc  += 3;
	  pDest += cbSize;
	}
      }
      break;

    case (DDPF_LUMINANCE | DDPF_ALPHAPIXELS):
      srcPitch *= 2;
      pSrc     += ((y * cx * 2)       + (x * 2));
      pDest    += ((y * ddsd2.lPitch) + (x * cbSize));

      for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
      {
	for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
	{
	  dwColor =  ( ((DWORD)(*(pSrc  ) * rScale)) << rShift );

	  /* This is a 1bit ALPHA check. */
	  if ( aScale == -1.0 )
	    dwColor |= ( (*(pSrc+3) & 0x80) ? (1 << aShift) : 0 );
	  else
	    dwColor |= ( ((DWORD)(*(pSrc+3) * aScale)) << aShift );
	  
	  memcpy( pDest, &dwColor, cbSize );
	  pSrc  += 2;
	  pDest += cbSize;
	}
      }
      break;

    case DDPF_LUMINANCE:
      pSrc  += ((y * cx)           +  x);
      pDest += ((y * ddsd2.lPitch) + (x * cbSize));

      for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
      {
	for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
	{
	  dwColor =  ( ((DWORD)(*(pSrc  ) * rScale)) << rShift );

	  memcpy( pDest, &dwColor, cbSize );
	  pSrc++;
	  pDest += cbSize;
	}
      }
      break;

    case DDPF_ALPHAPIXELS:
      pSrc  += ((y * cx)           +  x);
      pDest += ((y * ddsd2.lPitch) + (x * cbSize));

      for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
      {
	for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
	{
	  /* This is a 1bit ALPHA check. */
	  if ( aScale == -1.0 )
	    dwColor = ( (*(pSrc+3) & 0x80) ? (1 << aShift) : 0 );
	  else
	    dwColor = ( ((DWORD)(*(pSrc+3) * aScale)) << aShift );

	  memcpy( pDest, &dwColor, cbSize );
	  pSrc++;
	  pDest += cbSize;
	}
      }
      break;
  }

  /* We are done so unlock the D3D surface. */
#ifdef D3D_DEBUG
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

  PROFILE_STOP;
}
/*===========================================================================*/
/*  This fast path function is used to fill 565 surfaces only.  Note that the*/
/* pitch is really the width and needs to be updated based on the number of  */
/* components the surface has.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurface565( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			destPitch,
                        dwCol;
  GLubyte 		*pSrcRow;
  WORD			*pDest,
                        *pDestRow;
  ULONG			rc;

  PROFILE_START;

#define SRC_CBSIZE	3	/* Source is Mesa RGB. */

  /* Lock the D3D surface. */
#ifdef D3D_DEBUG
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
#endif

  /* Convert the destination pitch to be in terms of WORDs. */
  destPitch = ddsd2.lPitch >> 1;

  /* FInd the starting destination pointer which is in WORDs also. */
  pDest = ((WORD *)ddsd2.lpSurface) + (y * destPitch) + x;

  /* Get the source pitch in terms of BYTES. */
  srcPitch *= SRC_CBSIZE;

  /* Find the starting source pointer in terms of BYTES. */
  pSrc += ((y * srcPitch) + (x * SRC_CBSIZE));

  /* Fill the surface one row at a time using pitch to reset pDestRow/pSrcRow. */
  for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += destPitch, pSrcRow += srcPitch )
  {
    /* Pointer are incremented easily as they are in terms of their pixel size. */
    for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++, pDest++ )
    {
      *pDest =  ( ((WORD)(*pSrc++) & 0xF8) << 8);
      *pDest |= ( ((WORD)(*pSrc++) & 0xFC) << 3);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF8) >> 3);
    }
  }

  /* We are done so unlock the D3D surface. */
#ifdef D3D_DEBUG
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

#undef SRC_CBSIZE

  PROFILE_STOP;
}
/*===========================================================================*/
/*  This fast path function is used to fill 4444 surfaces only. Note that the*/
/* pitch is really the width and needs to be updated based on the number of  */
/* components the surface has.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurface4444( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			destPitch,
                        dwCol;
  GLubyte 		*pSrcRow;
  WORD			*pDest,
                        *pDestRow;
  ULONG			rc;

  PROFILE_START;

#define SRC_CBSIZE	4	/* Source is Mesa RBA. */

#ifdef D3D_DEBUG
  /* Lock the D3D surface. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
#endif

  /* Solve the destination pitch in terms of WORDs. */
  destPitch = ddsd2.lPitch >> 1;

  /* Find the starting destination pointer in WORDs. */
  pDest = ((WORD *)ddsd2.lpSurface) + (y * destPitch) + x;

  /* The source pitch is in terms of BYTEs. */
  srcPitch *= SRC_CBSIZE;

  /* Get the starting source pointer. */
  pSrc += ((y * srcPitch) + (x * SRC_CBSIZE));

  /* Fill the surface one row at a time using pitch to reset pDestRow/pSrcRow. */
  for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += destPitch, pSrcRow += srcPitch )
  {
    for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++, pDest++ )
    {
      /* Pointer are incremented easily as they are in terms of their pixel size. */
      *pDest =  ( ((WORD)(*pSrc++) & 0xF0) << 4);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF0) >> 0);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF0) >> 4);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF0) << 8);
    }
  }

#ifdef D3D_DEBUG
  /* We are done so unlock the D3D surface. */
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

#undef SRC_CBSIZE

  PROFILE_STOP;
}
/*===========================================================================*/
/*  This fast path function is used to fill 1555 surfaces only. Note that the*/
/* pitch is really the width and needs to be updated based on the number of  */
/* components the surface has.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurface1555( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			destPitch,
                        dwCol;
  GLubyte 		*pSrcRow;
  WORD			*pDest,
                        *pDestRow;
  ULONG			rc;

  PROFILE_START;

#define SRC_CBSIZE	4	/* Source is Mesa RBA. */

#ifdef D3D_DEBUG
  /* Lock the D3D surface. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
#endif

  /* Solve the destination pitch in terms of WORDs. */
  destPitch = ddsd2.lPitch >> 1;

  /* Find the starting destination pointer in WORDs. */
  pDest = ((WORD *)ddsd2.lpSurface) + (y * destPitch) + x;

  /* The source pitch is in terms of BYTEs. */
  srcPitch *= SRC_CBSIZE;

  /* Get the starting source pointer. */
  pSrc += ((y * srcPitch) + (x * SRC_CBSIZE));

  /* Fill the surface one row at a time using pitch to reset pDestRow/pSrcRow. */
  for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += destPitch, pSrcRow += srcPitch )
  {
    for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++, pDest++ )
    {
      /* Pointer are incremented easily as they are in terms of their pixel size. */
      *pDest =  ( ((WORD)(*pSrc++) & 0xF8) << 7);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF8) << 2);
      *pDest |= ( ((WORD)(*pSrc++) & 0xF8) >> 3);
      *pDest |= ( ((WORD)(*pSrc++) & 0x80) << 8);
    }
  }

#ifdef D3D_DEBUG
  /* We are done so unlock the D3D surface. */
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

#undef SRC_CBSIZE

  PROFILE_STOP;
}
/*===========================================================================*/
/*  This fast path function is used to fill 888 surfaces only. Note that the */
/* pitch is really the width and needs to be updated based on the number of  */
/* components the surface has.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurface888( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			dwCol;
  GLubyte 		*pSrcRow,
        		*pDest,
                        *pDestRow;
  ULONG			rc;

  PROFILE_START;
#define SRC_CBSIZE	3

  /* Lock the D3D surface. */
#ifdef D3D_DEBUG
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
#endif

  /*  Both the destination pitch and pointer are in BYTEs already so we */
  /* only need to find the starting pointer.                            */
  pDest = ((GLubyte *)ddsd2.lpSurface) + (y * ddsd2.lPitch) + (x * SRC_CBSIZE);

  /* Solve the source pitch in terms of BYTEs. */
  srcPitch *= SRC_CBSIZE;
  
  /* Get the starting pointer in terms of BYTEs. */
  pSrc += ((y * srcPitch) + (x * SRC_CBSIZE));

  /* Fill the surface one row at a time using pitch to reset pDestRow/pSrcRow. */
  for( pDestRow = pDest, pSrcRow = pSrc; cy > 0; cy--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
  {
    /* Pointer are incremented easily as they are in terms of their pixel size. */
    for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < cx; dwCol++ )
    {
      *pDest++ = *pSrc++;
      *pDest++ = *pSrc++;
      *pDest++ = *pSrc++;
    }
  }

  /* We are done so unlock the D3D surface. */
#ifdef D3D_DEBUG
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

#undef SRC_CBSIZE
  PROFILE_STOP;
}
/*===========================================================================*/
/*  This fast path function is used to fill 8888 surfaces only. Note that the*/
/* pitch is really the width and needs to be updated based on the number of  */
/* components the surface has.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void FillSurface8888( GLubyte *pSrc, int srcPitch, int x, int y, int cx, int cy, LPDIRECTDRAWSURFACE4 lpDDS )
{
  DDSURFACEDESC2	ddsd2;
  DWORD			dwCol,
                        destPitch,
                        *pSource,
                   	*pSrcRow,
        		*pDest,
                        *pDestRow;
  ULONG			rc;

  PROFILE_START;

#define SRC_CBSIZE	4

  /* Lock the D3D surface. */
#ifdef D3D_DEBUG
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );			
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  rc = lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Lock() <%s>", ErrorStringD3D(rc) )); 
    return;
  }					
#else
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);				
  lpDDS->Lock( NULL, &ddsd2, (DDLOCK_WAIT | DDLOCK_WRITEONLY), NULL );	
#endif

  /* Get the destination pitch in terms of DWORDs. */
  destPitch = ddsd2.lPitch >> 2;

  /* Find the starting pointer that is a DWORD also. */
  pDest = ((DWORD *)ddsd2.lpSurface) + (y * destPitch) + x;

  /* The source pitch is ok as we are in DWORDs so find the starting pointer. */
  pSource = ((DWORD *)pSrc) + (y * srcPitch) + x;

  /* Fill the surface one row at a time using pitch to reset pDestRow/pSrcRow. */
  for( pDestRow = pDest, pSrcRow = pSource; cy > 0; cy--, pDestRow += destPitch, pSrcRow += srcPitch )
  {
    /* Pointer are incremented easily as they are in terms of their pixel size. */
    for( dwCol = 0, pDest = pDestRow, pSource = pSrcRow; dwCol < cx; dwCol++, pDest++, pSource++ )
    {
      *pDest =  ((*pSource & 0xFF000000) >> 0);
      *pDest |= ((*pSource & 0x00FF0000) >> 16);
      *pDest |= ((*pSource & 0x0000FF00) << 0);
      *pDest |= ((*pSource & 0x000000FF) << 16);
    }
  }

  /* We are done so unlock the D3D surface. */
#ifdef D3D_DEBUG
  rc = lpDDS->Unlock( NULL );				
  if ( FAILED(rc) )							
  {									
    DPF(( DBG_TXT_ERROR,"***SUF*** Unlock() <%s>.", ErrorStringD3D(rc) )); 	
    return;
  }
#else
  lpDDS->Unlock( NULL );				
#endif

#undef SRC_CBSIZE
  PROFILE_STOP;
}
