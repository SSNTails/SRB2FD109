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
/* Local function prototypes.                                                */
/*===========================================================================*/
HRESULT CALLBACK EnumDeviceHook( GUID FAR* lpGuid, LPSTR lpDesc, LPSTR lpName, 
				 LPD3DDEVICEDESC lpD3DHWDesc, LPD3DDEVICEDESC lpD3DHELDesc,  void *pVoid );
HRESULT CALLBACK EnumZBufferHook( DDPIXELFORMAT* pddpf, VOID *pVoid );
HRESULT WINAPI   EnumSurfacesHook( LPDIRECTDRAWSURFACE4 lpDDS, LPDDSURFACEDESC2 lpDDSDesc, LPVOID pVoid );
/*===========================================================================*/
/* Globals.                                                                  */
/*===========================================================================*/
USER_CTRL	g_User;
/*===========================================================================*/
/*  This call is used to do one time initialization for the Direct3D part of */
/* the OpenGL32.dll.  At the moment there isn't much going on here as we do  */
/* most of the Direct3D stuff in the resizing code.  This was only really    */
/* put in for future use.                                                    */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL InitHAL( void )
{
  DPF(( DBG_FUNC, "   HAL--> InitHAL" ));

  /* Parse the user's enviroment variables to generate a debug mask. */
  ReadDBGEnv();

  g_DBGMask = DBG_ALL_ERROR | DBG_FUNC;
  g_DBGMask = DBG_ALL;
  g_DBGMask = DBG_ALL_ERROR;

  /* Get the user define settings for the driver. */
  memset( &g_User, 0, sizeof(USER_CTRL) );

  g_User.bForceSoftware    = ( VAR_SET(MESA_FORCE_SW) )      ? TRUE : FALSE;
  g_User.bStretchtoPrimary = ( VAR_SET(MESA_STRETCH_ON) )    ? TRUE : FALSE;
  g_User.bTripleBuffer     = ( VAR_SET(MESA_TRIPLE_BUFFER) ) ? TRUE : FALSE;
  g_User.bAutoTextures     = ( VAR_SET(MESA_AUTO_TEXTURES) ) ? TRUE : FALSE;
  g_User.bMipMaps          = ( VAR_SET(MESA_MIPMAP_OFF) )    ? FALSE : TRUE;

  DPF(( DBG_FUNC, "<--HAL    InitHAL" ));

  return TRUE;
}
/*===========================================================================*/
/*  This function was only put in for completeness like the InitHAL().  At   */
/* this point it does nothing.                                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL TermHAL( void )
{
  DPF(( DBG_FUNC, "   HAL--> TermHAL" ));

  DPF(( DBG_FUNC, "<--HAL    TermHAL" ));

  return TRUE;
}
/*===========================================================================*/
/*  This function is used to init / resize the rendering surface as the two  */
/* are almost the same.  First all the DirectDraw and Direct3D objects and   */
/* interfaces are created.  Next the Coop level is set based on whether we   */
/* have a window or fullscreen.  The device enumeration routine uses the     */
/* bForceSW to determine what kind of device we are trying for.   I try and  */
/* get the most out of the hardware by first trying many different configs.  */
/*                                                                           */
/* 1) HW Device - HW ZBuffer - HW Complex FLIP                               */
/*                           - HW Offscreen BLT                              */
/* 2) SW Device - HW Complex FLIP                                            */
/*                HW Offscreen BLT                                           */
/* 3) Failed...                                                              */
/*                                                                           */
/*   NOTE:  The worst case is that all will be in SW (RGBDevice) and really  */
/*         I should forget the whole thing and fall back to a DDraw span type*/
/*         rendering but what is the point.  This way I always know I have a */
/*         D3DDevice and that makes things easier.  I do impliment the span  */
/*         rendering function for stuff that I haven't done support for such */
/*         as points and lines.                                              */
/*===========================================================================*/
/* RETURN: TRUE, FALSE                                                       */
/*===========================================================================*/
BOOL CreateD3DContext( PD3DMESACONTEXT pContext )
{
  DDSURFACEDESC2	ddsd2;
  D3DDEVICEDESC		D3DSWDevDesc;
  DWORD			dwCoopFlags;
  DDSCAPS2 	  	ddscaps;
  BOOL			bZBufferLessHSR;
  ULONG			rc;

  DPF(( DBG_FUNC, "   HAL--> CreateD3DContext" ));

  if ( pContext == NULL )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** NULL Context" ));
    return FALSE;
  }

  /*=================================*/
  /* Create all required interfaces. */
  /*=================================*/

  /* Create a instance of DDraw using the Primary display driver. */
  rc = DirectDrawCreate( NULL, &pContext->lpDD, NULL );
  if( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"***HAL*** DirectDrawCreate() <%s>", ErrorStringD3D(rc) ));
    return FALSE;
  }

  /* Get the DDraw4 interface. */
  rc = pContext->lpDD->QueryInterface( IID_IDirectDraw4, (void **)&pContext->lpDD4 );
  if( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"***HAL*** QueryInterface( IID_IDirectDraw4 ) <%s>", ErrorStringD3D(rc) ));
    return FALSE;
  }

  /* We no longer need the old DirectDraw object. */
  SAFE_RELEASE( pContext->lpDD );

  /* Get the Direct3D3 interface. */
  rc = pContext->lpDD4->QueryInterface( IID_IDirect3D3, (void **)&pContext->lpD3D3 );
  if( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"***HAL*** QueryInterface( IID_IDirect3D3 ) <%s>", ErrorStringD3D(rc) ));
    return FALSE;
  }

  /* Set the coop level to normal as I was getting COOP ALREADY SET errors. */
  pContext->lpDD4->SetCooperativeLevel( WindowFromDC(pContext->hdc), DDSCL_NORMAL );

  /* Set the Cooperative level. NOTE: we need to know if we are FS at this point.*/
  dwCoopFlags = (pContext->bWindow) ? 
                (DDSCL_NORMAL | DDSCL_FPUSETUP) : 
                (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
  //                (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX | DDSCL_FPUSETUP);
  rc = pContext->lpDD4->SetCooperativeLevel( WindowFromDC(pContext->hdc), dwCoopFlags );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"***HAL*** SetCooperativeLevel( 0x%80X ) <%s>", dwCoopFlags, ErrorStringD3D(rc) ));
    return FALSE;
  }

  /*==================================================================*/
  /* Get the best device we can and note whether its hardware or not. */
  /*==================================================================*/
  pContext->bForceSW  = g_User.bForceSoftware;
  pContext->bFlipable = FALSE;

  pContext->lpD3D3->EnumDevices( EnumDeviceHook, (void *)pContext );
  pContext->bHardware = IsEqualIID( pContext->guid, IID_IDirect3DHALDevice );
  bZBufferLessHSR = (pContext->D3DDeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR) ?
                    TRUE : FALSE;

  DPF(( DBG_CNTX_INFO, "   HAL    Creating Context: cx:%d cy:%d", pContext->dwWidth, pContext->dwHeight ));
  DPF(( DBG_CNTX_INFO, "   HAL    <%s>", (pContext->bWindow) ? "WINDOWED" : "FULL SCREEN" ));

  /*========================================================================*/
  /* HARDWARE was found.                                                    */
  /*========================================================================*/
  if ( pContext->bHardware == TRUE )
  {
    /*===================================*/
    /* HARDWARE -> Z-BUFFER.             */
    /*===================================*/
    if ( bZBufferLessHSR == FALSE )
    {
      /* Get a Z-Buffer pixelformat from the hardware. */
      memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
      ddsd2.dwSize = sizeof( DDSURFACEDESC2 );
      rc = pContext->lpD3D3->EnumZBufferFormats( pContext->guid, EnumZBufferHook, (VOID*)&ddsd2.ddpfPixelFormat );
      if ( FAILED(rc) )
      {
	DPF(( DBG_CNTX_ERROR,"***HAL*** EnumZBufferFormatsl() <%s>", ErrorStringD3D(rc) ));
	return FALSE;
      }
        
      /* Setup our request structure for the Z-buffer surface using the PF. */
      ddsd2.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
      ddsd2.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | 
                             DDSCAPS_VIDEOMEMORY;
      ddsd2.dwWidth        = pContext->dwWidth;
      ddsd2.dwHeight       = pContext->dwHeight;
      rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSZbuffer, NULL );
      if ( FAILED(rc) )
      {
	DPF(( DBG_CNTX_INFO, 
	      "   HAL    CreateSurface( DDSCAPS_ZBUFFER ) <%s>", 
	      ErrorStringD3D(rc) ));

	pContext->lpDDSZbuffer = NULL;
	pContext->lpD3DDevice  = NULL;
      }
    }

    /*=======================*/
    /* HARDWARE -> FLIPABLE  */
    /*=======================*/
    if ( pContext->bWindow == FALSE )
    {
      /* Create a TRIPLE buffered surface if the env is set MESA_TRIPLE_BUFFER. */
      INIT_DDSD2( ddsd2, DDSD_CAPS | DDSD_BACKBUFFERCOUNT );
      ddsd2.dwBackBufferCount = (g_User.bTripleBuffer) ? 2 : 1;
      ddsd2.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | 
	                        DDSCAPS_3DDEVICE | 
                                DDSCAPS_FLIP | 
                                DDSCAPS_COMPLEX | 
                                DDSCAPS_VIDEOMEMORY;
      rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSPrimary, NULL );
      if ( FAILED(rc) )
      {	
	rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSPrimary, NULL );
	if ( FAILED(rc) )
	{
	  /* Try a DOUBLE if failed. */
	  ddsd2.dwBackBufferCount = 1;

	  /* Make sure fallbacks happen correctly. */
	  pContext->lpDDSPrimary = NULL;
	  pContext->lpD3DDevice  = NULL;

	  DPF(( DBG_CNTX_INFO, 
		"   HAL    CreateSurface( PRIMARYSURFACE | 3DDEVICE | FLIP | COMPLEX | VIDEOMEMORY ) <%s>", 
		ErrorStringD3D(rc) ));
	}
      }
      else
      {
	/* Get the back buffer that was created. */
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	rc = pContext->lpDDSPrimary->GetAttachedSurface( &ddscaps, &pContext->lpDDSRender );
	if ( FAILED(rc) )
	{	
	  /* Make sure fallbacks happen correctly. */
	  SAFE_RELEASE( pContext->lpDDSPrimary );
	  pContext->lpDDSRender = NULL;
	  pContext->lpD3DDevice = NULL;

	  DPF(( DBG_CNTX_INFO, 
		"   HAL    GetAttachedSurface( BACKBUFFER ) <%s>", 
		ErrorStringD3D(rc) ));
	}	

	/*=================*/
	/* ATTACH ZBUFFER. */
	/*=================*/
	if ( pContext->lpDDSPrimary && 
	     pContext->lpDDSRender  && 
	     pContext->lpDDSZbuffer )
	{
	  /* All surface should be created so we only need to attach the Zbuffer. */
	  rc = pContext->lpDDSRender->AddAttachedSurface( pContext->lpDDSZbuffer );
	  if ( FAILED(rc) )
	  {
	    /* Make sure fallbacks happen correctly. */
	    SAFE_RELEASE( pContext->lpDDSPrimary );
	    SAFE_RELEASE( pContext->lpDDSRender );
	    pContext->lpD3DDevice  = NULL;

	    DPF(( DBG_CNTX_INFO, 
		  "   HAL    AddAttachedSurface( ZBUFFER ) <%s>", 
		  ErrorStringD3D(rc) ));
	  }
	}
	  
	/*================*/
	/* Create Device. */
	/*================*/
	if ( pContext->lpDDSPrimary && 
	     pContext->lpDDSRender  && 
	     (pContext->lpDDSZbuffer || (bZBufferLessHSR == TRUE)) )
	{
	  /* Lets restore as I don't really get this concept... */
	  DX_RESTORE( pContext->lpDDSPrimary );
	  DX_RESTORE( pContext->lpDDSRender );
	  DX_RESTORE( pContext->lpDDSZbuffer );

	  /* Create a Direct3D Device. */
	  rc = pContext->lpD3D3->CreateDevice( pContext->guid,
					       pContext->lpDDSRender,	   
					       &pContext->lpD3DDevice, 
					       NULL );
	  if ( FAILED(rc) )
	  {
	    /* Make sure fallbacks happen correctly. */
	    SAFE_RELEASE( pContext->lpDDSPrimary );
	    SAFE_RELEASE( pContext->lpDDSRender );
	    pContext->lpD3DDevice = NULL;

	    DPF(( DBG_CNTX_INFO, "   HAL    CreateDevice() <%s>", ErrorStringD3D(rc) ));
	  }
	  else
	  {
	    pContext->bFlipable = TRUE;
	  }
	}
      }
    }
    
    /*==================*/
    /* HARDWARE -> BLT  */
    /*==================*/
    if ( pContext->lpD3DDevice == NULL )
    {
      /* Won't be a flip surface at this point. */
      pContext->bFlipable = FALSE;

      /*  Release any of the possable surfaces hanging. */
      SAFE_RELEASE( pContext->lpDDSPrimary );
      SAFE_RELEASE( pContext->lpDDSRender );

      /* Create the Primary (front buffer). */
      INIT_DDSD2( ddsd2, DDSD_CAPS );
      ddsd2.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
      rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSPrimary, NULL );
      if ( FAILED(rc) )
      {
	/* Make sure fallbacks happen correctly. */
	pContext->lpDDSPrimary = NULL;
	pContext->lpD3DDevice  = NULL;

	DPF(( DBG_CNTX_INFO, 
	      "   HAL    CreateSurface( PRIMARY ) <%s>", 
	      ErrorStringD3D(rc) ));
      }
      else
      {
	/* Create the Render (back buffer). */
	INIT_DDSD2( ddsd2, DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT );
	ddsd2.dwWidth	     = pContext->dwWidth;
	ddsd2.dwHeight	     = pContext->dwHeight;
	ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | 
	                       DDSCAPS_3DDEVICE |
                               DDSCAPS_VIDEOMEMORY;
	rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSRender, NULL );
	if ( FAILED(rc) )
	{
	  /* Make sure fallbacks happen correctly. */
	  SAFE_RELEASE( pContext->lpDDSPrimary );
	  pContext->lpDDSRender = NULL;
	  pContext->lpD3DDevice = NULL;

	  DPF(( DBG_CNTX_INFO, 
		"   HAL    CreateSurface( OFFSCREENPLAIN | 3DDEVICE ) <%s>", 
		ErrorStringD3D(rc) ));
	}
      }
      
      /*=================*/
      /* ATTACH ZBUFFER. */
      /*=================*/
      if ( pContext->lpDDSPrimary && 
	   pContext->lpDDSRender  &&
	   pContext->lpDDSZbuffer )
      {
	rc = pContext->lpDDSRender->AddAttachedSurface( pContext->lpDDSZbuffer );
	if ( FAILED(rc) )
	{
	  /* Kill this setup. */
	  SAFE_RELEASE( pContext->lpDDSPrimary );
	  SAFE_RELEASE( pContext->lpDDSRender );
	  pContext->lpDDSZbuffer = NULL;
	  pContext->lpD3DDevice = NULL;

	  DPF(( DBG_CNTX_INFO, 
		"   HAL    AddAttachedSurface( ZBUFFER ) <%s>", 
		ErrorStringD3D(rc) ));
	}
      }

      /*================*/
      /* Create Device. */
      /*================*/
      if ( pContext->lpDDSPrimary && 
	   pContext->lpDDSRender  && 
	   (pContext->lpDDSZbuffer || (bZBufferLessHSR == TRUE)) )
      {
	/* Lets restore as I don't really get this concept... */
	DX_RESTORE( pContext->lpDDSPrimary );
	DX_RESTORE( pContext->lpDDSRender );
	DX_RESTORE( pContext->lpDDSZbuffer );

	/* Create a Direct3D Device. */
	rc = pContext->lpD3D3->CreateDevice( pContext->guid,
					     pContext->lpDDSRender,	   
					     &pContext->lpD3DDevice, 
					     NULL );
	if ( FAILED(rc) )
	{
	  /* Make sure fallbacks happen correctly. */
	  SAFE_RELEASE( pContext->lpDDSPrimary );
	  SAFE_RELEASE( pContext->lpDDSRender );
	  pContext->lpD3DDevice = NULL;

	  DPF(( DBG_CNTX_INFO, 
		"   HAL    CreateDevice() <%s>", 
		ErrorStringD3D(rc) ));
	}
      }
    }
  }
    
  /*===================================*/
  /* SOFTWARE -> Z-BUFFER -> BLT       */
  /*===================================*/
  if ( pContext->lpD3DDevice == NULL )
  {
    /* Make sure we clean up. */
    SAFE_RELEASE( pContext->lpDDSPrimary );
    SAFE_RELEASE( pContext->lpDDSRender );
    SAFE_RELEASE( pContext->lpDDSZbuffer );

    /* We are down to a HEL D3DDevice. */
    pContext->bForceSW = TRUE;
    pContext->lpD3D3->EnumDevices( EnumDeviceHook, (void *)pContext );
    pContext->bHardware = IsEqualIID( pContext->guid, IID_IDirect3DHALDevice );
    DPF(( DBG_CNTX_INFO, 
	  "   HAL    Using <%sWARE>", 
	  (pContext->bHardware) ? "HARD" : "SOFT" ));

    /* Create the Primary (front buffer). */
    INIT_DDSD2( ddsd2, DDSD_CAPS );
    ddsd2.ddsCaps.dwCaps  = DDSCAPS_PRIMARYSURFACE;
    rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSPrimary, NULL );
    if ( FAILED(rc) )
    {
      /* Just to be clean. */
      pContext->lpDDSPrimary = NULL;
      pContext->lpD3DDevice  = NULL;

      /* This is an error as we should be able to do this at minimum. */
      DPF(( DBG_CNTX_ERROR,
	    "***HAL*** CreateSurface( PRIMARY ) <%s>", 
	    ErrorStringD3D(rc) ));

      return FALSE;
    }

    /* Create the Render (back buffer). */
    INIT_DDSD2( ddsd2, DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT );
    ddsd2.dwWidth		  = pContext->dwWidth;
    ddsd2.dwHeight		  = pContext->dwHeight;
    ddsd2.ddsCaps.dwCaps 	  = DDSCAPS_OFFSCREENPLAIN | 
                                    DDSCAPS_3DDEVICE  | 
                                    DDSCAPS_VIDEOMEMORY;
    ddsd2.ddpfPixelFormat.dwSize  = sizeof( DDPIXELFORMAT );
    ddsd2.ddpfPixelFormat.dwFlags = (DDPF_RGB | DDPF_ALPHAPIXELS);
    rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSRender, NULL );
    if ( FAILED(rc) )
    {
      /* Try system memory I guess. */
      ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | 
	                     DDSCAPS_3DDEVICE  | 
	                     DDSCAPS_SYSTEMMEMORY;
      rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSRender, NULL );
      if ( FAILED(rc) )
      {
	/* Clean up before we try and shut down. */
	SAFE_RELEASE( pContext->lpDDSPrimary );
	pContext->lpDDSRender  = NULL;
	pContext->lpDDSZbuffer = NULL;
	pContext->lpD3DDevice  = NULL;

	/* That was our last hope. */
	DPF(( DBG_CNTX_ERROR,
	      "***HAL*** CreateSurface( OFFSCREENPLAIN | 3DDEVICE | SYSTEMMEMORY) <%s>", 
	      ErrorStringD3D(rc) ));

	return FALSE;
      }
    }

    /* Get a Z-Buffer pixelformat. */
    memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
    ddsd2.dwSize = sizeof( DDSURFACEDESC2 );
    rc = pContext->lpD3D3->EnumZBufferFormats( pContext->guid, EnumZBufferHook, (VOID*)&ddsd2.ddpfPixelFormat );
    if ( FAILED(rc) )
    {	
      /* Clean up before we try and shut down. */
      SAFE_RELEASE( pContext->lpDDSPrimary );
      SAFE_RELEASE( pContext->lpDDSRender );
      pContext->lpDDSZbuffer = NULL;
      pContext->lpD3DDevice  = NULL;

      DPF(( DBG_CNTX_ERROR,
	    "***HAL*** EnumZBufferFormatsl() <%s>", 
	    ErrorStringD3D(rc) ));

      return FALSE;
    }
        
    /* Setup our request structure for the Z-buffer surface. */
    ddsd2.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;
    ddsd2.dwWidth        = pContext->dwWidth;
    ddsd2.dwHeight       = pContext->dwHeight;
    rc = pContext->lpDD4->CreateSurface( &ddsd2, &pContext->lpDDSZbuffer, NULL );
    if ( FAILED(rc) )
    {
      /* Clean up before we try and shut down. */
      SAFE_RELEASE( pContext->lpDDSPrimary );
      SAFE_RELEASE( pContext->lpDDSRender );
      pContext->lpDDSZbuffer = NULL;
      pContext->lpD3DDevice  = NULL;

      DPF(( DBG_CNTX_ERROR,
	    "***HAL*** CreateSurface( DDSCAPS_ZBUFFER ) <%s>", 
	    ErrorStringD3D(rc) ));

      return FALSE;
    }

    rc = pContext->lpDDSRender->AddAttachedSurface( pContext->lpDDSZbuffer );
    if ( FAILED(rc) )
    {
      /* Clean up before we try and shut down. */
      SAFE_RELEASE( pContext->lpDDSPrimary );
      SAFE_RELEASE( pContext->lpDDSRender );
      pContext->lpDDSZbuffer = NULL;
      pContext->lpD3DDevice  = NULL;

      DPF(( DBG_CNTX_ERROR, 
	    "   HAL    AddAttachedSurface( ZBUFFER ) <%s>", 
	    ErrorStringD3D(rc) ));

      return FALSE;
    }

    /*================*/
    /* Create Device. */
    /*================*/
    if ( pContext->lpDDSPrimary )
    {
      DX_RESTORE( pContext->lpDDSPrimary );
      DX_RESTORE( pContext->lpDDSRender );
      DX_RESTORE( pContext->lpDDSZbuffer );

      /* Create a Direct3D Device. */
      rc = pContext->lpD3D3->CreateDevice( pContext->guid,
					 pContext->lpDDSRender,	   
					 &pContext->lpD3DDevice, 
					 NULL );
      if ( FAILED(rc) )
      {
	pContext->lpD3DDevice = NULL;

	DPF(( DBG_CNTX_ERROR, 
	      "***HAL*** CreateDevice() <%s>", 
	      ErrorStringD3D(rc) ));

	return FALSE;
      }
    }
  }

  /*=======================================*/
  /* We need a clipper if we are windowed. */
  /*=======================================*/
  if ( pContext->bWindow == TRUE )
  {
    /*  Create a clipper object so that DDraw will be able to blt windows that */
    /* have been clipped by the screen or other windows.                       */
    pContext->lpDD4->CreateClipper( 0, &pContext->lpClipper, NULL );
    pContext->lpClipper->SetHWnd( 0, WindowFromDC(pContext->hdc) );
    pContext->lpDDSPrimary->SetClipper( pContext->lpClipper );
    pContext->lpClipper->Release();
    DPF(( DBG_CNTX_INFO, "   HAL    CreateSurface( OFFSCREENPLAIN | 3DDEVICE ) Ok." ));  
  }

  /* Get a copy of what the D3DDevice supports for later use. */
  memset( &D3DSWDevDesc, 0, sizeof(D3DDEVICEDESC) );
  memset( &pContext->D3DDeviceDesc, 0, sizeof(D3DDEVICEDESC) );
  D3DSWDevDesc.dwSize = sizeof( D3DDEVICEDESC );
  pContext->D3DDeviceDesc.dwSize = sizeof( D3DDEVICEDESC );
  rc = pContext->lpD3DDevice->GetCaps( &pContext->D3DDeviceDesc, &D3DSWDevDesc );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** GetCaps() Failed." ));
    return FALSE;
  }

  /*==========================================*/
  /* Parse the pixelformat & save the values. */
  /*==========================================*/
  pContext->FillSurface = GetFillFunction( pContext->lpDDSRender );
  if ( pContext->FillSurface == NULL )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** GetFillFunction() Failed." ));
    return FALSE;
  }

  /* Init the texture manager. */
  rc = InitTMgrHAL( pContext );
  if ( rc == FALSE )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** Texture manager failed to init" ));
    return FALSE;
  }

  /* We must prime the Begin/End scene for SwapBuffers to work. */
  rc = pContext->lpD3DDevice->BeginScene();   
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"***HAL*** BeginScene <%s>", ErrorStringD3D(rc) ));
    return FALSE;
  }

  /* This check needs to be done during a scene. */
  memset( &pContext->dddi, 0, sizeof(DDDEVICEIDENTIFIER) );
  rc = pContext->lpDD4->GetDeviceIdentifier( &pContext->dddi, DDGDI_GETHOSTIDENTIFIER );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** GetDeviceIdentifier() Failed." ));
    return FALSE;
  }
  
  DPF(( DBG_FUNC, "<--HAL    CreateD3DContext" ));

  return TRUE;
}
/*===========================================================================*/
/*  This call cleans up all the Direct3D resources allocated for this D3D    */
/* context.                                                                  */
/*===========================================================================*/
/* RETURN: TRUE, FALSE                                                       */
/*===========================================================================*/
BOOL DestroyD3DContext( PD3DMESACONTEXT pContext )
{
  ULONG rc;
  LONG	nDD  = 0L,	/* Number of outstanding DDraw references. */
        nD3D = 0L;	/* Number of outstanding D3DDevice references. */

  DPF(( DBG_FUNC, "   HAL--> DestroyD3DContext" ));

  if ( pContext == NULL )
  {
    DPF(( DBG_CNTX_ERROR, "***HAL*** NULL Context" ));
    return FALSE;
  }

  /* Make sure all textures from the previous device are destroyed. */
  TermTMgrHAL( pContext );

  /*===================================*/
  /* Destroy all Direct3D stuff first. */
  /*===================================*/
  SAFE_RELEASE( pContext->lpViewport );

  /* Do a safe release of the D3DDevice and check for extra references. */
  if ( pContext->lpD3DDevice )
  {
    rc = pContext->lpD3DDevice->EndScene();   
    if ( FAILED(rc) )
    {
      DPF(( DBG_CNTX_INFO, "***HAL*** EndScene() Failed." ));
      return FALSE;
    }

    nD3D = pContext->lpD3DDevice->Release();
    if ( nD3D > 0 )
    {
      DPF(( DBG_CNTX_INFO, "***HAL*** Direct3D3 Device still has references: %d", nD3D ));
    }
    pContext->lpD3DDevice = NULL;
  }

  /* If this isn't a Flipable surface then we must clean up the render. */
  if ( pContext->bFlipable == FALSE)
    SAFE_RELEASE( pContext->lpDDSRender );
  SAFE_RELEASE( pContext->lpDDSZbuffer );
  SAFE_RELEASE( pContext->lpDDSPrimary );

  SAFE_RELEASE( pContext->lpD3D3 );

  /*===============================*/
  /* Destroy all DirectDraw stuff. */
  /*===============================*/
  if( pContext->lpDD4 )
  {
    pContext->lpDD4->SetCooperativeLevel( WindowFromDC(pContext->hdc), DDSCL_NORMAL );

    nDD = pContext->lpDD4->Release();
    if ( nDD > 0 )
    {
      DPF(( DBG_CNTX_INFO, "***HAL*** DirectDraw4 still has references: %d", nDD ));
    }
    pContext->lpD3DDevice = NULL;
  }

  DPF(( DBG_FUNC, "<--HAL    DestroyD3DContext" ));

  return TRUE;
}
/*===========================================================================*/
/*  This function will make sure a viewport is created and set for the device*/
/* in the supplied context.  If a rect is supplied then it will be used for  */
/* the viewport otherwise the current setting in the context will be used.   */
/* NOTE: that the rect is relative to the window.  So left/top must be 0,0 to*/
/* use the whole window else there is scissoring going down.                 */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void SetViewportHAL( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  D3DVIEWPORT2		vdData;
  ULONG			rc;
  POINT			pt;

  DPF(( DBG_FUNC, "   HAL--> SetViewportHAL" ));

  /* Make sure we can set a viewport. */
  if ( pContext == g_pD3DDefault )
  {
    DPF(( DBG_CNTX_WARN, "***HAL*** SetViewport( default )" ));
    return;
  }

  /* Make sure we have enough info. */
  if ( (pContext->lpDDSPrimary == NULL) || 
       (pContext->lpD3DDevice == NULL) )
  {
    DPF(( DBG_CNTX_WARN, "***HAL*** SetViewport() (NULL Pointer)" ));
    return;
  }

  /* This is just to stop redundant changes. */
  if ( (pContext->rectViewport.left   == x)       &&
       (pContext->rectViewport.top    == y)       &&
       (pContext->rectViewport.right  == (x + w)) &&
       (pContext->rectViewport.bottom == (y +h)) )
  {
    DPF(( DBG_CNTX_INFO, "   HAL    Redundant viewport" ));
    return;
  }

  DPF(( DBG_CNTX_INFO, "   HAL    Current Viewport:" ));
  DPF(( DBG_CNTX_INFO, "   HAL    x: %d y: %d",   pContext->rectViewport.left, pContext->rectViewport.top ));
  DPF(( DBG_CNTX_INFO, "   HAL    cx: %d cy: %d", (pContext->rectViewport.right  - pContext->rectViewport.left), 
	                                          (pContext->rectViewport.bottom - pContext->rectViewport.top) ));
  DPF(( DBG_CNTX_INFO, "   HAL    New Viewport:" ));
  DPF(( DBG_CNTX_INFO, "   HAL    x:  %d y:  %d", x, y ));
  DPF(( DBG_CNTX_INFO, "   HAL    cx: %d cy: %d", w, h ));

  /* Update the current viewport rect if one is supplied. */
  pContext->rectViewport.left   = x;
  pContext->rectViewport.right  = x + w;
  pContext->rectViewport.top    = y;
  pContext->rectViewport.bottom = y + h;
	 
  /* Build the request structure. */
  memset( &vdData, 0, sizeof(D3DVIEWPORT2) );
  vdData.dwSize   = sizeof(D3DVIEWPORT2);  
  vdData.dwX      = pContext->rectViewport.left;
  vdData.dwY      = pContext->rectViewport.top;
  vdData.dwWidth  = (pContext->rectViewport.right - pContext->rectViewport.left);
  vdData.dwHeight = (pContext->rectViewport.bottom - pContext->rectViewport.top);

  if ( !vdData.dwWidth || !vdData.dwHeight )
  {
    GetClientRect( WindowFromDC(pContext->hdc), &pContext->rectViewport );
    pt.x = pt.y = 0;
    ClientToScreen( WindowFromDC(pContext->hdc), &pt );
    OffsetRect( &pContext->rectClient, pt.x, pt.y);
    vdData.dwX      = pContext->rectViewport.left;
    vdData.dwY      = pContext->rectViewport.top;
    vdData.dwWidth  = (pContext->rectViewport.right - pContext->rectViewport.left);
    vdData.dwHeight = (pContext->rectViewport.bottom - pContext->rectViewport.top);
    memcpy( &pContext->rectViewport, &pContext->rectClient, sizeof(RECT) );
  }

  // The dvClipX, dvClipY, dvClipWidth, dvClipHeight, dvMinZ, 
  // and dvMaxZ members define the non-normalized post-perspective 
  // 3-D view volume which is visible to the viewer. In most cases, 
  // dvClipX is set to -1.0 and dvClipY is set to the inverse of 
  // the viewport's aspect ratio on the target surface, which can be 
  // calculated by dividing the dwHeight member by dwWidth. Similarly, 
  // the dvClipWidth member is typically 2.0 and dvClipHeight is set 
  // to twice the aspect ratio set in dwClipY. The dvMinZ and dvMaxZ 
  // are usually set to 0.0 and 1.0.
  vdData.dvClipX      = -1.0f;
  vdData.dvClipWidth  = 2.0f;
  vdData.dvClipY      = 1.0f;
  vdData.dvClipHeight = 2.0f;
  vdData.dvMaxZ       = 1.0;
  vdData.dvMinZ       = 0.0;

  /*  I'm going to destroy the viewport everytime as when we size we will */
  /* have a new D3DDevice.  As this area doesn't need to be fast...       */
  if ( pContext->lpViewport )
  {
    DPF(( DBG_CNTX_INFO, "   HAL    DeleteViewport" ));

    pContext->lpD3DDevice->DeleteViewport( pContext->lpViewport );
    rc = pContext->lpViewport->Release();
    if ( FAILED(rc) )
    {
      DPF(( DBG_CNTX_ERROR, "   HAL    Viewport->Release() <%s>", ErrorStringD3D(rc) ));
      return;
    }
    pContext->lpViewport = NULL;
  }

  rc = pContext->lpD3D3->CreateViewport( &pContext->lpViewport, NULL );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR, "   HAL    CreateViewport() <%s>", ErrorStringD3D(rc) ));
    return;
  }

  /* Update the device with the new viewport. */
  pContext->lpD3DDevice->AddViewport( pContext->lpViewport );
  pContext->lpViewport->SetViewport2( &vdData );
  pContext->lpD3DDevice->SetCurrentViewport( pContext->lpViewport );

  DPF(( DBG_FUNC, "<--HAL    SetViewportHAL" ));
}
/*===========================================================================*/
/*  This callback function will return a surface if one is found that matches*/
/* the pixelformat and the surface CAPs.                                     */
/*===========================================================================*/
/* RETURN: D3DENUMRET_CANCEL, D3DENUMRET_OK.                                 */
/*===========================================================================*/
HRESULT WINAPI EnumSurfacesHook( LPDIRECTDRAWSURFACE4 lpDDS, LPDDSURFACEDESC2 lpDDSDesc, LPVOID pVoid )
{
  DDSURFACEDESC2 *pddsd2 = (DDSURFACEDESC2 *)pVoid;

  DPF(( DBG_FUNC, "EnumSurfacesHook();" ));

  if ( (lpDDSDesc->ddpfPixelFormat.dwFlags == pddsd2->ddpfPixelFormat.dwFlags) && 
       (lpDDSDesc->ddsCaps.dwCaps == pddsd2->ddsCaps.dwCaps) )
  {
    /* Save the pixelformat now so that we know we have one. */
    memcpy( pddsd2, lpDDSDesc, sizeof(DDSURFACEDESC2) );

    return D3DENUMRET_CANCEL;
  }

  return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This is the callback proc to get a Z-Buffer.  I chose to use a ZBuffer   */
/* as low as 8bits.  Now this is almost always not enopugh but I figure if   */
/* its all we have then use it...                                            */
/*===========================================================================*/
/* RETURN: D3DENUMRET_CANCEL, D3DENUMRET_OK.                                 */
/*===========================================================================*/
HRESULT CALLBACK  EnumZBufferHook( DDPIXELFORMAT* pddpf, VOID *pVoid )
{
  DDPIXELFORMAT *pddpfChoice = (DDPIXELFORMAT *)pVoid;

  DPF(( DBG_FUNC, "   HAL--> EnumZBufferHook" ));

  /* If this is ANY type of depth-buffer, stop. */
  if( pddpf->dwFlags == DDPF_ZBUFFER )
  {
    /* Save the pixelformat now so that we know we have one. */
    memcpy( pddpfChoice, pddpf, sizeof(DDPIXELFORMAT) );

    /* I feel if the hardware supports this low then lets use it.  Could get ugly. */
    if( pddpf->dwZBufferBitDepth >= 8 )
    {
      DPF(( DBG_FUNC, "   HAL    ZBuffer matched <%dbit>.", pddpf->dwZBufferBitDepth ));
      return D3DENUMRET_CANCEL;
    }
  }

  DPF(( DBG_FUNC, "<--HAL    EnumZBufferHook" ));
 
  return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This function handles the callback for the D3DDevice enumeration.  Good  */
/* god who's idea was this? The D3D wrapper has two variables related to what*/
/* kind of device we want and have.  First we have a Bool that is set if we  */
/* have allocated a HW device.  We always look for the HW device first.  The */
/* other variable is used to force SW.  If we have run into a case that we   */
/* want to fallback to SW then we set this.                                  */
/*===========================================================================*/
/* RETURN: D3DENUMRET_CANCEL, D3DENUMRET_OK.                                 */
/*===========================================================================*/
HRESULT CALLBACK EnumDeviceHook( GUID FAR* lpGuid, LPSTR lpDesc, LPSTR lpName, 
				 LPD3DDEVICEDESC lpD3DHWDesc, LPD3DDEVICEDESC lpD3DHELDesc, void *pVoid )
{
  PD3DMESACONTEXT pContext = (PD3DMESACONTEXT)pVoid;
  LPD3DDEVICEDESC pChoice;

  DPF(( DBG_FUNC, "   HAL--> EnumDeviceHook" ));

  /* Determine if which device description is valid. */
  pChoice = (lpD3DHWDesc->dwFlags) ? lpD3DHWDesc : lpD3DHELDesc;

  /* Save some info about this device. */
  memcpy( &pContext->guid, lpGuid, sizeof(GUID) );
  memcpy( &pContext->D3DDeviceDesc, lpD3DHWDesc, sizeof(D3DDEVICEDESC) );
  strcpy( pContext->pszDeviceDesc, lpName );

  /* If we have hardware and we are looking for hardware... */
  if ( (pChoice == lpD3DHWDesc) && (pContext->bForceSW == FALSE) )
  {
    DPF(( DBG_CNTX_INFO, "   HAL    %s",    lpName  ));
    DPF(( DBG_FUNC,      "<--HAL    EnumDeviceHook" ));

    return D3DENUMRET_CANCEL;
  }

  /* We are doing to try for the reference over the RGB. */
  if ( (pContext->bForceSW == TRUE) && IsEqualIID(*lpGuid,IID_IDirect3DRefDevice) )
  {
    DPF(( DBG_CNTX_INFO, "   HAL    %s", lpDesc       ));
    DPF(( DBG_CNTX_INFO, "   HAL    SOFTWARE forced." ));
    DPF(( DBG_FUNC,      "<--HAL    EnumDeviceHook"   ));
    return D3DENUMRET_CANCEL;
  }

  memset( pContext->pszDeviceDesc, 0, sizeof(pContext->pszDeviceDesc) );
  DPF(( DBG_CNTX_INFO, "   HAL    no match." ));

  DPF(( DBG_FUNC, "<--HAL    EnumDeviceHook" ));
  return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This function will convert the DDraw error code to its macro string.  The*/
/* returned pointer is static so you need not worry about memory managemnet  */
/* but the error message gets written over from call to call...              */
/*===========================================================================*/
/* RETURN: pointer to the single static buffer that hold the error message.  */
/*===========================================================================*/
char *ErrorStringD3D( HRESULT hr )
{
  static char	errorString[128];

  switch( hr )
  {
    case DDERR_ALREADYINITIALIZED:
      strcpy( errorString, "DDERR_ALREADYINITIALIZED" );
      break;
	 
    case DDERR_CANNOTATTACHSURFACE:
      strcpy( errorString, "DDERR_CANNOTATTACHSURFACE" );
      break;
	 
    case DDERR_CANNOTDETACHSURFACE:
      strcpy( errorString, "DDERR_CANNOTDETACHSURFACE" );
      break;
	 
    case DDERR_CURRENTLYNOTAVAIL:
      strcpy( errorString, "DDERR_CURRENTLYNOTAVAIL" );
      break;
	 
    case DDERR_EXCEPTION:
      strcpy( errorString, "DDERR_EXCEPTION" );
      break;
	 
    case DDERR_GENERIC:
      strcpy( errorString, "DDERR_GENERIC" );
      break;
	 
    case DDERR_HEIGHTALIGN:
      strcpy( errorString, "DDERR_HEIGHTALIGN" );
      break;
	 
    case DDERR_INCOMPATIBLEPRIMARY:
      strcpy( errorString, "DDERR_INCOMPATIBLEPRIMARY" );
      break;
	 
    case DDERR_INVALIDCAPS:
      strcpy( errorString, "DDERR_INVALIDCAPS" );
      break;
	 
    case DDERR_INVALIDCLIPLIST:
      strcpy( errorString, "DDERR_INVALIDCLIPLIST" );
      break;
	 
    case DDERR_INVALIDMODE:
      strcpy( errorString, "DDERR_INVALIDMODE" );
      break;
	 
    case DDERR_INVALIDOBJECT:
      strcpy( errorString, "DDERR_INVALIDOBJECT" );
      break;
	 
    case DDERR_INVALIDPARAMS:
      strcpy( errorString, "DDERR_INVALIDPARAMS" );
      break;
	 
    case DDERR_INVALIDPIXELFORMAT:
      strcpy( errorString, "DDERR_INVALIDPIXELFORMAT" );
      break;
	 
    case DDERR_INVALIDRECT:
      strcpy( errorString, "DDERR_INVALIDRECT" );
      break;
	 
    case DDERR_LOCKEDSURFACES:
      strcpy( errorString, "DDERR_LOCKEDSURFACES" );
      break;
	 
    case DDERR_NO3D:
      strcpy( errorString, "DDERR_NO3D" );
      break;
	 
    case DDERR_NOALPHAHW:
      strcpy( errorString, "DDERR_NOALPHAHW" );
      break;
	 
    case DDERR_NOCLIPLIST:
      strcpy( errorString, "DDERR_NOCLIPLIST" );
      break;
	 
    case DDERR_NOCOLORCONVHW:
      strcpy( errorString, "DDERR_NOCOLORCONVHW" );
      break;
	 
    case DDERR_NOCOOPERATIVELEVELSET:
      strcpy( errorString, "DDERR_NOCOOPERATIVELEVELSET" );
      break;
	 
    case DDERR_NOCOLORKEY:
      strcpy( errorString, "DDERR_NOCOLORKEY" );
      break;
	 
    case DDERR_NOCOLORKEYHW:
      strcpy( errorString, "DDERR_NOCOLORKEYHW" );
      break;
	 
    case DDERR_NODIRECTDRAWSUPPORT:
      strcpy( errorString, "DDERR_NODIRECTDRAWSUPPORT" );
      break;
	 
    case DDERR_NOEXCLUSIVEMODE:
      strcpy( errorString, "DDERR_NOEXCLUSIVEMODE" );
      break;
	 
    case DDERR_NOFLIPHW:
      strcpy( errorString, "DDERR_NOFLIPHW" );
      break;
	 
    case DDERR_NOGDI:
      strcpy( errorString, "DDERR_NOGDI" );
      break;
	 
    case DDERR_NOMIRRORHW:
      strcpy( errorString, "DDERR_NOMIRRORHW" );
      break;
	 
    case DDERR_NOTFOUND:
      strcpy( errorString, "DDERR_NOTFOUND" );
      break;
	 
    case DDERR_NOOVERLAYHW:
      strcpy( errorString, "DDERR_NOOVERLAYHW" );
      break;
	 
    case DDERR_OVERLAPPINGRECTS:
      strcpy( errorString, "DDERR_OVERLAPPINGRECTS" );
      break;
	 
    case DDERR_NORASTEROPHW:
      strcpy( errorString, "DDERR_NORASTEROPHW" );
      break;
	 
    case DDERR_NOROTATIONHW:
      strcpy( errorString, "DDERR_NOROTATIONHW" );
      break;
	 
    case DDERR_NOSTRETCHHW:
      strcpy( errorString, "DDERR_NOSTRETCHHW" );
      break;
	 
    case DDERR_NOT4BITCOLOR:
      strcpy( errorString, "DDERR_NOT4BITCOLOR" );
      break;
	 
    case DDERR_NOT4BITCOLORINDEX:
      strcpy( errorString, "DDERR_NOT4BITCOLORINDEX" );
      break;
	 
    case DDERR_NOT8BITCOLOR:
      strcpy( errorString, "DDERR_NOT8BITCOLOR" );
      break;
	 
    case DDERR_NOTEXTUREHW:
      strcpy( errorString, "DDERR_NOTEXTUREHW" );
      break;
	 
    case DDERR_NOVSYNCHW:
      strcpy( errorString, "DDERR_NOVSYNCHW" );
      break;
	 
    case DDERR_NOZBUFFERHW:
      strcpy( errorString, "DDERR_NOZBUFFERHW" );
      break;

    case DDERR_NOZOVERLAYHW:
      strcpy( errorString, "DDERR_NOZOVERLAYHW" );
      break;
	 
    case DDERR_OUTOFCAPS:
      strcpy( errorString, "DDERR_OUTOFCAPS" );
      break;
	 
    case DDERR_OUTOFMEMORY:
      strcpy( errorString, "DDERR_OUTOFMEMORY" );
      break;
	 
    case DDERR_OUTOFVIDEOMEMORY:
      strcpy( errorString, "DDERR_OUTOFVIDEOMEMORY" );
      break;
	 
    case DDERR_OVERLAYCANTCLIP:
      strcpy( errorString, "DDERR_OVERLAYCANTCLIP" );
      break;
	 
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
      strcpy( errorString, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" );
      break;
	 
    case DDERR_PALETTEBUSY:
      strcpy( errorString, "DDERR_PALETTEBUSY" );
      break;
	 
    case DDERR_COLORKEYNOTSET:
      strcpy( errorString, "DDERR_COLORKEYNOTSET" );
      break;
	 
    case DDERR_SURFACEALREADYATTACHED:
      strcpy( errorString, "DDERR_SURFACEALREADYATTACHED" );
      break;
	 
    case DDERR_SURFACEALREADYDEPENDENT:
      strcpy( errorString, "DDERR_SURFACEALREADYDEPENDENT" );
      break;
	 
    case DDERR_SURFACEBUSY:
      strcpy( errorString, "DDERR_SURFACEBUSY" );
      break;
	 
    case DDERR_CANTLOCKSURFACE:
      strcpy( errorString, "DDERR_CANTLOCKSURFACE" );
      break;

    case DDERR_SURFACEISOBSCURED:
      strcpy( errorString, "DDERR_SURFACEISOBSCURED" );
      break;

    case DDERR_SURFACELOST:
      strcpy( errorString, "DDERR_SURFACELOST" );
      break;
	 
    case DDERR_SURFACENOTATTACHED:
      strcpy( errorString, "DDERR_SURFACENOTATTACHED" );
      break;
	 
    case DDERR_TOOBIGHEIGHT:
      strcpy( errorString, "DDERR_TOOBIGHEIGHT" );
      break;
	 
    case DDERR_TOOBIGSIZE:
      strcpy( errorString, "DDERR_TOOBIGSIZE" );
      break;
	 
    case DDERR_TOOBIGWIDTH:
      strcpy( errorString, "DDERR_TOOBIGWIDTH" );
      break;
	 
    case DDERR_UNSUPPORTED:
      strcpy( errorString, "DDERR_UNSUPPORTED" );
      break;
	 
    case DDERR_UNSUPPORTEDFORMAT:
      strcpy( errorString, "DDERR_UNSUPPORTEDFORMAT" );
      break;
	 
    case DDERR_UNSUPPORTEDMASK:
      strcpy( errorString, "DDERR_UNSUPPORTEDMASK" );
      break;
	 
    case DDERR_INVALIDSTREAM:
      strcpy( errorString, "DDERR_INVALIDSTREAM" );
      break;
	 
    case DDERR_VERTICALBLANKINPROGRESS:
      strcpy( errorString, "DDERR_VERTICALBLANKINPROGRESS" );
      break;
	 
    case DDERR_WASSTILLDRAWING:
      strcpy( errorString, "DDERR_WASSTILLDRAWING" );
      break;
	 
    case DDERR_XALIGN:
      strcpy( errorString, "DDERR_XALIGN" );
      break;
	 
    case DDERR_INVALIDDIRECTDRAWGUID:
      strcpy( errorString, "DDERR_INVALIDDIRECTDRAWGUID" );
      break;
	 
    case DDERR_DIRECTDRAWALREADYCREATED:
      strcpy( errorString, "DDERR_DIRECTDRAWALREADYCREATED" );
      break;
	 
    case DDERR_NODIRECTDRAWHW:
      strcpy( errorString, "DDERR_NODIRECTDRAWHW" );
      break;
	 
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
      strcpy( errorString, "DDERR_PRIMARYSURFACEALREADYEXISTS" );
      break;
	 
    case DDERR_NOEMULATION:
      strcpy( errorString, "DDERR_NOEMULATION" );
      break;
	 
    case DDERR_REGIONTOOSMALL:
      strcpy( errorString, "DDERR_REGIONTOOSMALL" );
      break;
	 
    case DDERR_CLIPPERISUSINGHWND:
      strcpy( errorString, "DDERR_CLIPPERISUSINGHWND" );
      break;
	 
    case DDERR_NOCLIPPERATTACHED:
      strcpy( errorString, "DDERR_NOCLIPPERATTACHED" );
      break;
	 
    case DDERR_NOHWND:
      strcpy( errorString, "DDERR_NOHWND" );
      break;
	 
    case DDERR_HWNDSUBCLASSED:
      strcpy( errorString, "DDERR_HWNDSUBCLASSED" );
      break;
	 
    case DDERR_HWNDALREADYSET:
      strcpy( errorString, "DDERR_HWNDALREADYSET" );
      break;
	 
    case DDERR_NOPALETTEATTACHED:
      strcpy( errorString, "DDERR_NOPALETTEATTACHED" );
      break;
	 
    case DDERR_NOPALETTEHW:
      strcpy( errorString, "DDERR_NOPALETTEHW" );
      break;
	 
    case DDERR_BLTFASTCANTCLIP:
      strcpy( errorString, "DDERR_BLTFASTCANTCLIP" );
      break;

    case DDERR_NOBLTHW:
      strcpy( errorString, "DDERR_NOBLTHW" );
      break;
      
    case DDERR_NODDROPSHW:
      strcpy( errorString, "DDERR_NODDROPSHW" );
      break;

    case DDERR_OVERLAYNOTVISIBLE:
      strcpy( errorString, "DDERR_OVERLAYNOTVISIBLE" );
      break;

    case DDERR_NOOVERLAYDEST:
      strcpy( errorString, "DDERR_NOOVERLAYDEST" );
      break;

    case DDERR_INVALIDPOSITION:
      strcpy( errorString, "DDERR_INVALIDPOSITION" );
      break;

    case DDERR_NOTAOVERLAYSURFACE:
      strcpy( errorString, "DDERR_NOTAOVERLAYSURFACE" );
      break;

    case DDERR_EXCLUSIVEMODEALREADYSET:
      strcpy( errorString, "DDERR_EXCLUSIVEMODEALREADYSET" );
      break;

    case DDERR_NOTFLIPPABLE:
      strcpy( errorString, "DDERR_NOTFLIPPABLE" );
      break;

    case DDERR_CANTDUPLICATE:
      strcpy( errorString, "DDERR_CANTDUPLICATE" );
      break;

    case DDERR_NOTLOCKED:
      strcpy( errorString, "DDERR_NOTLOCKED" );
      break;

    case DDERR_CANTCREATEDC:
      strcpy( errorString, "DDERR_CANTCREATEDC" );
      break;
	 
    case DDERR_NODC:
      strcpy( errorString, "DDERR_NODC" );
      break;

    case DDERR_WRONGMODE:
      strcpy( errorString, "DDERR_WRONGMODE" );
      break;

    case DDERR_IMPLICITLYCREATED:
      strcpy( errorString, "DDERR_IMPLICITLYCREATED" );
      break;

    case DDERR_NOTPALETTIZED:
      strcpy( errorString, "DDERR_NOTPALETTIZED" );
      break;

    case DDERR_UNSUPPORTEDMODE:
      strcpy( errorString, "DDERR_UNSUPPORTEDMODE" );
      break;

    case DDERR_NOMIPMAPHW:
      strcpy( errorString, "DDERR_NOMIPMAPHW" );
      break;

    case DDERR_INVALIDSURFACETYPE:
      strcpy( errorString, "DDERR_INVALIDSURFACETYPE" );
      break;

    case DDERR_NOOPTIMIZEHW:
      strcpy( errorString, "DDERR_NOOPTIMIZEHW" );
      break;

    case DDERR_NOTLOADED:
      strcpy( errorString, "DDERR_NOTLOADED" );
      break;
	 
    case DDERR_NOFOCUSWINDOW:
      strcpy( errorString, "DDERR_NOFOCUSWINDOW" );
      break;

    case DDERR_DCALREADYCREATED:
      strcpy( errorString, "DDERR_DCALREADYCREATED" );
      break;

    case DDERR_NONONLOCALVIDMEM:
      strcpy( errorString, "DDERR_NONONLOCALVIDMEM" );
      break;

    case DDERR_CANTPAGELOCK:
      strcpy( errorString, "DDERR_CANTPAGELOCK" );
      break;

    case DDERR_CANTPAGEUNLOCK:
      strcpy( errorString, "DDERR_CANTPAGEUNLOCK" );
      break;

    case DDERR_NOTPAGELOCKED:
      strcpy( errorString, "DDERR_NOTPAGELOCKED" );
      break;

    case DDERR_MOREDATA:
      strcpy( errorString, "DDERR_MOREDATA" );
      break;

    case DDERR_EXPIRED:
      strcpy( errorString, "DDERR_EXPIRED" );
      break;

    case DDERR_VIDEONOTACTIVE:
      strcpy( errorString, "DDERR_VIDEONOTACTIVE" );
      break;

    case DDERR_DEVICEDOESNTOWNSURFACE:
      strcpy( errorString, "DDERR_DEVICEDOESNTOWNSURFACE" );
      break;

    case DDERR_NOTINITIALIZED:
      strcpy( errorString, "DDERR_NOTINITIALIZED" );
      break;

    default:
      strcpy( errorString, "<unknown error code>" );
      break;
  }

  return &errorString[0];
} 


