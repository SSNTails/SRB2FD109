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
static PTEX_OBJ_HAL CreateTextureOBJ( PD3DMESACONTEXT pContext );
static void DestroyTextureOBJ( PD3DMESACONTEXT pContext, PTEX_OBJ_HAL pTexHAL );
static BOOL FreeTextureMemory( PD3DMESACONTEXT pContext, PTEX_OBJ_HAL pTexHAL );
static void UpdateTextureCache( PD3DMESACONTEXT	pContext, PTEX_OBJ_HAL pTexHAL );
HRESULT CALLBACK EnumPFHook( LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext );
/*===========================================================================*/
/*  This function simply initializes the linked list of texture objects.  We */
/* need to keep track off all the textures so they can be freed when we have */
/* to destroy the context.  This can happen on a resize as we get a new D3D  */
/* device and the current textures must be reloaded.                         */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL InitTMgrHAL( PD3DMESACONTEXT pContext )
{
  DPF(( DBG_FUNC, "   TXT--> InitTMgrHAL" ));

  pContext->pTexturesHAL = NULL;

  DPF(( DBG_FUNC, "<--TXT    InitTMgrHAL" ));

  return TRUE;
}
/*===========================================================================*/
/*  This function will walk the linked list and release and free all the     */
/* texture objects found.  Remeber textures will only work with the device   */
/* that was used to create them.  So this function must be called when we get*/
/* a new D3D device.                                                         */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TermTMgrHAL( PD3DMESACONTEXT pContext )
{
  DPF(( DBG_FUNC, "   TXT--> TermTMgrHAL" ));

  /* Walk the linked list destroying all texture objects left. */
  while( pContext->pTexturesHAL )
    DestroyTextureOBJ( pContext, pContext->pTexturesHAL );

  DPF(( DBG_FUNC, "<--TXT    TermTMgrHAL" ));
}
/*===========================================================================*/
/*  This function will ALLOC a texture object structure and add it to the    */
/* linked list of texture objects for the supplied context.                  */
/*===========================================================================*/
/* RETURN: pointer to the texture object, NULL.                              */
/*===========================================================================*/
static PTEX_OBJ_HAL CreateTextureOBJ( PD3DMESACONTEXT pContext )
{
  PTEX_OBJ_HAL	pTexHAL;

  DPF(( DBG_FUNC, "   TXT--> CreateTextureOBJ" ));

  /* Allocate a new texture object for the HAL. */
  pTexHAL = (PTEX_OBJ_HAL)ALLOC( sizeof(TEX_OBJ_HAL) );
  if ( pTexHAL == NULL )
  {
    DPF(( DBG_TXT_ERROR, "***TXT*** ALLOC TEX_OBJ_HAL." ));
    return NULL;
  }
  memset( pTexHAL, 0, sizeof(TEX_OBJ_HAL) ); 

  /* Add the new object to the linked list. */
  if ( pContext->pTexturesHAL )
    pContext->pTexturesHAL->prev = pTexHAL;

  pTexHAL->next = pContext->pTexturesHAL;

  /* Set to top just like a cache. */
  pContext->pTexturesHAL = pTexHAL; 

  DPF(( DBG_FUNC, "<--TXT    CreateTextureOBJ" ));

  return pTexHAL;
}
/*===========================================================================*/
/*  Just a simple function that puts the supplied texture object to the top  */
/* of the stack.  This creates a cache effect as when I want more texture    */
/* memory I start freeing from the bottom.  If auto texture management is on */
/* then this won't matter either way.                                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void UpdateTextureCache( PD3DMESACONTEXT	pContext, PTEX_OBJ_HAL pTexHAL )
{
  /* This object is already at the front. */
  if ( (pContext->pTexturesHAL == pTexHAL) ||
       (pTexHAL == NULL) )
    return;

  /*===============================================================*/
  /* Make some caching happen by pulling this object to the front. */ 
  /*===============================================================*/

  /* Pull the object out of the list. */
  if ( pTexHAL->prev )
    (pTexHAL->prev)->next = pTexHAL->next;

  if ( pTexHAL->next )
    (pTexHAL->next)->prev = pTexHAL->prev;

  pTexHAL->prev = NULL;
  pTexHAL->next = NULL;

  /* Put the object at the front of the list. */
  if ( pContext->pTexturesHAL )
    pContext->pTexturesHAL->prev = pTexHAL;

  pTexHAL->next = pContext->pTexturesHAL;

  pContext->pTexturesHAL = pTexHAL;
}
/*===========================================================================*/
/*  This function will pull the texture object out of the linked list of     */
/* objects for this context. Next it will free all the DX resources and the  */
/* memory for the structure itself.                                          */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void DestroyTextureOBJ( PD3DMESACONTEXT pContext, PTEX_OBJ_HAL pTexHAL )
{
  int		index;
  DDSCAPS2	ddsCaps2; 

  DPF(( DBG_FUNC, "   TXT--> DestroyTextureOBJ" ));

  /* I like to have functions that can be called with NULL. */
  if ( pTexHAL == NULL )
    return;

  /*=====================================*/
  /* Update any or all texture surfaces. */
  /*=====================================*/
  for( index = 0; pTexHAL->lpDDS[index]; index++ ) 
  {
    /* Look for an attached texture. */
    ddsCaps2.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    pTexHAL->lpDDS[index]->GetAttachedSurface( &ddsCaps2, &pTexHAL->lpDDS[(index+1)] ); 

    SAFE_RELEASE( pTexHAL->lpDDS[index] );
  }

  /* Now free up all the DX stuff. */
  SAFE_RELEASE( pTexHAL->lpD3DTexture2 );

  /* Pull this texture out of the list. */
  if ( pTexHAL == pContext->pTexturesHAL )
    pContext->pTexturesHAL = NULL;

  if ( pTexHAL->prev )
    (pTexHAL->prev)->next = pTexHAL->next;

  if ( pTexHAL->next )
    (pTexHAL->next)->prev = pTexHAL->prev;

  FREE( pTexHAL );

  DPF(( DBG_FUNC, "<--TXT    DestroyTextureOBJ" ));
}
/*===========================================================================*/
/*  Currently I am using the auto texture management for DirectX.  I switched*/
/* to this from my own as I felt that some DirectX drivers might be able to  */
/* do some extra work if given the chance.  Also it solves the problem of    */
/* where the textures are going to be stored (VIDMEM,AGP,SYS).               */
/*  Unfortunatly this has not worked out well as when I want to update only  */
/* a subimage of the texture it forces the DirectX texture manager to refresh*/
/* the whole texture.                                                        */
/*  So when I went back to my own method which was faster I had problems     */
/* creating mipmaps on some cards.  Since this project is free time bound I  */
/* use the auto texture manager when the user set mipmaps on and my own when */
/* mipmaps are disabled.                                                     */
/*                                                                           */
/*  Ok.  This function works by first checking to see if we have a texture   */
/* object for the texture we are loading.  If we don't have a object then I  */
/* create one and all the mipmaps.  So now the next time through the object  */
/* will have the texture surfaces created unless this texture has changed    */
/* size in which case we will destory our object and create another.         */
/*  The texture object will create parse the surface pixel format and choose */
/* a function that will be used to fill the texture.  There are many diferent*/
/* functions writen for the most comomon pixelformats.  Also a scale value   */
/* will be set for the u,v coords if the device requires square textures.    */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TextureLoadHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, 
		     GLint internalFormat, const struct gl_texture_image *image )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  PTEX_OBJ_HAL		pTexHAL  = (PTEX_OBJ_HAL)tObj->DriverData;
  DDSURFACEDESC2	ddsd2; 
  BOOL			bNewSurface;
  DDSCAPS2		ddsCaps2; 
  ULONG			rc;
  int			index;

  DPF(( DBG_FUNC, "   TXT--> TextureLoadHAL" ));

  /*=======================================================*/
  /* Check too see if we have a compatable texture object. */
  /*=======================================================*/
  if ( (pTexHAL                 == NULL) ||
       (pTexHAL->lpDDS[level]   == NULL) ||
       (pTexHAL->ddsd2.dwWidth  != tObj->Image[0]->Width) ||       
       (pTexHAL->ddsd2.dwHeight != tObj->Image[0]->Height) )
  {
    /* Destroy the current texture object. */
    DestroyTextureOBJ( pContext, pTexHAL );

    /* Allocate a new texture object for the HAL. */
    pTexHAL= CreateTextureOBJ( pContext );
    if ( pTexHAL == NULL )
    {
      DPF(( DBG_TXT_ERROR, "***TXT***CreateTextureOBJ()" ));
      return;
    }
    tObj->DriverData = pTexHAL;

    INIT_DDSD2( pTexHAL->ddsd2, DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT );
    pTexHAL->ddsd2.ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );

    pTexHAL->ddsd2.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
    if ( g_User.bAutoTextures )
    {
      pTexHAL->ddsd2.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    }
    else
    {
      pTexHAL->ddsd2.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    }

    if ( g_User.bMipMaps )
      pTexHAL->ddsd2.ddsCaps.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;

    /*  Devices that need square textures will be handled by allocating a */
    /* larger surface and later the u/v coordinates will be changed.      */
    if ( pContext->D3DDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
      pTexHAL->ddsd2.dwWidth  = max( tObj->Image[0]->Width, tObj->Image[0]->Height );
      pTexHAL->ddsd2.dwHeight = pTexHAL->ddsd2.dwWidth;
      pContext->uScale = (float)tObj->Image[0]->Width / (float)pTexHAL->ddsd2.dwWidth;
      pContext->vScale = (float)tObj->Image[0]->Height / (float)pTexHAL->ddsd2.dwHeight;
    }
    else
    {
      pTexHAL->ddsd2.dwWidth  = tObj->Image[0]->Width; 
      pTexHAL->ddsd2.dwHeight = tObj->Image[0]->Height;  
      pContext->uScale = 1.0;
      pContext->vScale = 1.0;
    }

    /*================================================================*/
    /* Translate the the Mesa/OpenGL pixel channels to the D3D flags. */
    /*================================================================*/
    switch( tObj->Image[0]->Format )
    {
      case GL_ALPHA:
	pTexHAL->ddsd2.ddpfPixelFormat.dwFlags = DDPF_ALPHA; 
	DPF(( DBG_TXT_WARN, "   TXT   GL_ALPHA n/s" ));
	return;
	 
      case GL_INTENSITY:
      case GL_LUMINANCE:
	DPF(( DBG_TXT_WARN, "   TXT   GL_INTENSITY/GL_LUMINANCE n/s" ));
	pTexHAL->ddsd2.ddpfPixelFormat.dwFlags = DDPF_LUMINANCE; 
	return;
	 
      case GL_LUMINANCE_ALPHA:
	DPF(( DBG_TXT_WARN, "   TXT   GL_LUMINANCE_ALPHA n/s" ));
	pTexHAL->ddsd2.ddpfPixelFormat.dwFlags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS; 
	return;
	 
      case GL_RGB:
	DPF(( DBG_TXT_INFO, "   TXT   Texture -> GL_RGB" ));
	pTexHAL->ddsd2.ddpfPixelFormat.dwFlags = DDPF_RGB; 
	break;

      case GL_RGBA:
	DPF(( DBG_TXT_INFO, "   TXT   Texture -> GL_RGBA" ));
	pTexHAL->ddsd2.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS; 
	break;
    }

    /* Look for a matching texture pixelformat. */
    rc = pContext->lpD3DDevice->EnumTextureFormats( EnumPFHook, &pTexHAL->ddsd2.ddpfPixelFormat );
    if ( FAILED(rc) )
    {
      DPF(( DBG_TXT_ERROR,"***TXT*** EnumerTextureFormats() <%s>", ErrorStringD3D(rc) ));
      return;
    }

    /*============================*/
    /* Try and create the mipmap. */
    /*============================*/
    do
    {
      /* Try to create the texture surface. */
      rc = pContext->lpDD4->CreateSurface( &pTexHAL->ddsd2, &pTexHAL->lpDDS[0], NULL );
      if ( !FAILED(rc) )
	break;

      DPF(( DBG_TXT_INFO, "   TXT    Free Texture Memory" ));

      /* DestroyTexture will return TRUE if a surface was freed. */
    } while( FreeTextureMemory(pContext,NULL) );

    /* Did we create a valid texture surface? */
    if ( FAILED(rc) )
    {
      DPF(( DBG_TXT_WARN, "Failed to load texture" ));
      pContext->lpD3DDevice->SetTexture( 0, NULL );
      return;
    }

    /* Find a function to fill this texture surface. */
    pTexHAL->FillSurface = GetFillFunction( pTexHAL->lpDDS[0] );
    if ( pTexHAL->FillSurface == NULL )
    {
      DPF(( DBG_TXT_ERROR, "***TXT*** GetFillFunction() )" ));

      /* Destroy the current texture object. */
      DestroyTextureOBJ( pContext, pTexHAL );
      tObj->DriverData = NULL;
      return;
    }

    /* When we are filling the texture all levels are invalid now. */
    bNewSurface = TRUE;
  }
  else
  {
    /*  Make a cache effect happen by pulling this texture surface */
    /* to the top of the linked list.                              */
    UpdateTextureCache( pContext, pTexHAL );
  }

  /*=====================================*/
  /* Update any or all texture surfaces. */
  /*=====================================*/
  for( index = 0; pTexHAL->lpDDS[index]; index++ ) 
  {
    /* Update if the surface is new or its the level being loaded. */
    if ( ((bNewSurface == TRUE) || (index == level)) && tObj->Image[index] )
    {
      /* Use the fill function in the pixel information structure to fill it. */
      pTexHAL->FillSurface( (GLubyte *)tObj->Image[index]->Data, 	
			    tObj->Image[index]->Width,
			    0, 0,
			    tObj->Image[index]->Width,
			    tObj->Image[index]->Height, 			
			    pTexHAL->lpDDS[index] );
    }

    /* Look for an attached texture. */
    ddsCaps2.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    pTexHAL->lpDDS[index]->GetAttachedSurface( &ddsCaps2, &pTexHAL->lpDDS[(index+1)] ); 
  }

  /* Get the Texture interface that is used to render with. */
  rc = pTexHAL->lpDDS[0]->QueryInterface( IID_IDirect3DTexture2, (void **)&pTexHAL->lpD3DTexture2 ); 
  if( FAILED(rc) )
  {
    DPF(( DBG_TXT_WARN, "   TXT  QueryInterface() <%s>", ErrorStringD3D(rc) ));

    /* Destroy the current texture object. */
    DestroyTextureOBJ( pContext, pTexHAL );
    tObj->DriverData = NULL;
    return;
  }

  /* Make this the current texture. */
  rc = pContext->lpD3DDevice->SetTexture( 0, pTexHAL->lpD3DTexture2 );
  if ( FAILED(rc) )
  {
    DPF(( DBG_TXT_WARN, "   TXT  SetTexture() <%s>", ErrorStringD3D(rc) ));

    /* Destroy the current texture object. */
    DestroyTextureOBJ( pContext, pTexHAL );
    tObj->DriverData = NULL;
    return;
  }

  /* Make this the current texture. */
  pContext->lpD3DTexture2 = pTexHAL->lpD3DTexture2;

  DPF(( DBG_FUNC, "<--TXT    TextureLoadHAL" ));
}
/*===========================================================================*/
/*  This function counts on the fact that there is a already a valid texture */
/* object created for this texture and I think this is always the case.  The */
/* subimage for the texture is updated using the fill surface function.      */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TextureSubImageHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, 
			 GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, 
			 GLint internalFormat, const struct gl_texture_image *image )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  PTEX_OBJ_HAL		pTexHAL  = (PTEX_OBJ_HAL)tObj->DriverData;

  DPF(( DBG_FUNC, "   TXT--> TextureSubImageHAL" ));

  /* Update if the surface for the level being loaded. */
  if ( tObj->Image[level] )
  {
    /* Use the fill function in the pixel information structure to fill it. */
    pTexHAL->FillSurface( (GLubyte *)tObj->Image[level]->Data, 	
			  tObj->Image[level]->Width,
			  xoffset, 
			  yoffset,
			  xoffset + width,
			  yoffset + height,
			  pTexHAL->lpDDS[level] );
  }

  DPF(( DBG_FUNC, "<--TXT    TextureSubImageHAL" ));
}
/*===========================================================================*/
/*  This function simply extracts the texture object for the HAL from the    */
/* driver specific field.  If we have a texture object then we set it.  If   */
/* not we simply fall through.                                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TextureBindHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj )
{
  PD3DMESACONTEXT 	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  PTEX_OBJ_HAL		pTexHAL  = (PTEX_OBJ_HAL)tObj->DriverData;
  ULONG			rc;

  DPF(( DBG_FUNC, "   TXT--> TextureBindHAL" ));

  if ( pTexHAL && pContext && pContext->lpD3DDevice )
  {
    /* Make this the current texture. */
    pContext->lpD3DTexture2 = pTexHAL->lpD3DTexture2;

    /*  Devices that need square textures will be handled by allocating a */
    /* larger surface and later the u/v coordinates will be changed.      */
    if ( pContext->D3DDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
      pTexHAL->ddsd2.dwWidth  = max( tObj->Image[0]->Width, tObj->Image[0]->Height );
      pTexHAL->ddsd2.dwHeight = pTexHAL->ddsd2.dwWidth;
      pContext->uScale = (float)tObj->Image[0]->Width / (float)pTexHAL->ddsd2.dwWidth;
      pContext->vScale = (float)tObj->Image[0]->Height / (float)pTexHAL->ddsd2.dwHeight;
    }
    else
    {
      pTexHAL->ddsd2.dwWidth  = tObj->Image[0]->Width; 
      pTexHAL->ddsd2.dwHeight = tObj->Image[0]->Height;  
      pContext->uScale = 1.0;
      pContext->vScale = 1.0;
    }
  }	

  UpdateTextureCache( pContext, pTexHAL );

  DPF(( DBG_FUNC, "<--TXT    TextureBindHAL" ));
}
/*===========================================================================*/
/*  This function checks for our texture object.  If one is found then I will*/
/* release the surface and the interface.  Also the memory for the object    */
/* itself will be freed.                                                     */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TextureDeleteHAL( GLcontext *ctx, struct gl_texture_object *tObj )
{
  PTEX_OBJ_HAL	pTexHAL  = (PTEX_OBJ_HAL)tObj->DriverData;

  DPF(( DBG_FUNC, "   TXT--> TextureDeleteHAL" ));

  /* Destroy the current texture object. */
  DestroyTextureOBJ( (PD3DMESACONTEXT)ctx->DriverCtx, pTexHAL );

  tObj->DriverData = NULL;

  DPF(( DBG_FUNC, "<--TXT    TextureDeleteHAL" ));
}
/*===========================================================================*/
/*  If this function gets a texture object struc then we will try and free   */
/* it.  If we get a NULL then we will search from the bottom up and free one */
/* VMEM surface.  I can only free when the surface isn't locked and of course*/
/* there must be a VMEM surface.  We never free SMEM surfaces as that isn't  */
/* the point.                                                                */
/* TODO: should have a pointer to the bottom of the stack really.            */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static BOOL FreeTextureMemory( PD3DMESACONTEXT pContext, PTEX_OBJ_HAL pTexHAL )
{
  PTEX_OBJ_HAL	pCurrent;
  DDSCAPS2	ddsCaps2; 
  int		index;

  /* Just to be safe. */
  if ( !pTexHAL || !pContext->pTexturesHAL )
    return FALSE;

  /* Free the last texture in the list. */
  if ( pTexHAL == NULL )
  {
    DPF(( DBG_TXT_INFO, "Free Last texture in cache" ));

    /* Find the last texture object. */
    for( pCurrent = pContext->pTexturesHAL; pCurrent->next; pCurrent = pCurrent->next );

    /* Didn't find anything. */
    if ( pCurrent->next == NULL )
      return FALSE;
  }
  else
  {
    return FALSE;
  }

  /* Free the texture interface. */
  DestroyTextureOBJ( pContext, pCurrent );
  
  return TRUE;
}
/*===========================================================================*/
/*  This function is the callback function that gets called when we are doing*/
/* an enumeration of the texture formats supported by this device. The choice*/
/* is made by checking to see if we have a match with the supplied D3D pixel-*/
/* format.  So the enumeration has to pass a desired D3D PF as the user var. */
/*===========================================================================*/
/* RETURN: D3DENUMRET_OK, D3DENUMRET_CANCEL.                                 */
/*===========================================================================*/
HRESULT CALLBACK EnumPFHook( LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext )
{
  LPDDPIXELFORMAT	lpDDPixFmtRequest = (LPDDPIXELFORMAT)lpContext;

  DPF(( DBG_FUNC, "   TXT--> EnumPFHook" ));

  if ( lpDDPixFmt->dwFlags == lpDDPixFmtRequest->dwFlags )
  {
    /* Save this format no matter what as its a match of sorts. */
    memcpy( lpDDPixFmtRequest, lpDDPixFmt, sizeof(DDPIXELFORMAT) );

    /* Are we looking for an alpha channel? If so look for something greater then 4bits*/
    if ( (lpDDPixFmtRequest->dwFlags & DDPF_ALPHAPIXELS) &&
	 ((lpDDPixFmt->dwRGBAlphaBitMask >> CountTrailingZeros(lpDDPixFmt->dwRGBAlphaBitMask)) <= 4 ) )
    {
      DPF(( DBG_TXT_INFO,"   TXT    Matched with small ALPHA." ));
      DPF(( DBG_FUNC, "<--TXT    EnumPFHook" ));
      return D3DENUMRET_OK;
    }

    /* Save this format as its a good match. */
    DPF(( DBG_TXT_INFO,"   TXT    Matched." ));
    DPF(( DBG_FUNC, "<--TXT    EnumPFHook" ));

    /* We are happy at this point so lets leave. */
    return D3DENUMRET_CANCEL;
  }
  
  DPF(( DBG_TXT_INFO,"   TXT    No match." ));
  DPF(( DBG_FUNC, "<--TXT    EnumPFHook" ));
  return D3DENUMRET_OK;
}
