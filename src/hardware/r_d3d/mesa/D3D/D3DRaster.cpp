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
/*  This function is a direct Mesa hook.  The mask passed in is the OGL mask */
/* that the API gets directly.  I use this to choose which buffers to clear  */
/* and I also clear the bits that represent the buffers I have cleared.      */
/*===========================================================================*/
/* RETURN: the modified mask.                                                */
/*===========================================================================*/
GLbitfield ClearBuffersHAL( GLcontext *ctx, GLbitfield mask, GLboolean all, 
			    GLint x, GLint y, GLint width, GLint height )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  DWORD          	dwFlags = 0;
  D3DRECT	 	d3dRect;
  HRESULT		rc;

  DPF(( DBG_FUNC, "   d3d--> ClearBuffersHAL" ));

  /* Make sure we have enough info. */
  if ( (pContext ==  NULL) || (pContext->lpViewport == NULL) )
    return mask;

  /* Parse the flags that request which buffer at to be cleared. */
  if ( mask & GL_COLOR_BUFFER_BIT )
  {
    dwFlags |= D3DCLEAR_TARGET;
    mask &= ~GL_COLOR_BUFFER_BIT;

    DPF(( DBG_CNTX_INFO, "   d3d    GL_COLOR_BUFFER_BIT <0x%X>", pContext->dwClearColor ));
  }
  if ( mask & GL_DEPTH_BUFFER_BIT )
  {
    if ( !(pContext->D3DDeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR) )
      dwFlags |= D3DCLEAR_ZBUFFER;
    mask &= ~GL_DEPTH_BUFFER_BIT;

    DPF(( DBG_CNTX_INFO, "   d3d    GL_DEPTH_BUFFER_BIT <%f>", ctx->Depth.Clear ));
  }

  /* We are done if we can't support the requested clear flag. */
  if ( dwFlags == 0 )
    return mask;

  if ( all )
  {
    /* I assume my viewport is valid. */
    d3dRect.lX1 = pContext->rectViewport.left;
    d3dRect.lY1 = pContext->rectViewport.top;
    d3dRect.lX2 = pContext->rectViewport.right;
    d3dRect.lY2 = pContext->rectViewport.bottom;
  }
  else
  {
    d3dRect.lX1 = pContext->rectClient.left + x;
    d3dRect.lY1 = pContext->rectClient.top  + y;
    d3dRect.lX2 = d3dRect.lX1 + width;
    d3dRect.lY2 = d3dRect.lY1 + height;
  }

  DPF(( DBG_CNTX_INFO, "   d3d    Rectangle (%d,%d) (%d,%d)", 
	d3dRect.lX1, d3dRect.lY1, d3dRect.lX2, d3dRect.lY2 ));

  /* Clear the context using the HAL. */
  rc = pContext->lpViewport->Clear2( 1, 
				     &d3dRect, 
				     dwFlags, 
				     pContext->dwClearColor, 
				     ctx->Depth.Clear, 
				     0 );
  if ( FAILED(rc) )
  {
    DPF(( DBG_CNTX_ERROR,"Clear2 <%s>", ErrorStringD3D(rc) ));
  }

  DPF(( DBG_FUNC, "<--d3d    ClearBuffersHAL" ));

  return mask;
}
/*===========================================================================*/
/*  This call will handle the swapping of the buffers.  Now I didn't bother  */
/* to support single buffered so this will be used for glFlush() as its all  */
/* the same.  So first we do an EndScene as we are always considered to be in*/
/* a BeginScene because when we leave we do a BeginScene.  Now note that when*/
/* the context is created in the first place we do a BeginScene also just to */
/* get things going.  The call will use either Flip/blt based on the type of */
/* surface was created for rendering.                                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void SwapBuffersHAL( PD3DMESACONTEXT pContext )
{
  long	x;
#ifdef D3D_DEBUG
  HRESULT	rc;      

  DPF(( DBG_ALL_PROFILE, "=================SWAP===================" ));
  DPF(( DBG_FUNC, "   d3d--> SwapBuffersHAL" ));

  /* Make sure we have enough info. */
  if ( (pContext == NULL) || (pContext->lpD3DDevice == NULL) )
    return;

  /* Make sure we have enough info. */
  if ( pContext->lpDDSPrimary != NULL )
  {
    rc = pContext->lpD3DDevice->EndScene();   
    if ( FAILED(rc) )
    {
      DPF(( DBG_CNTX_ERROR,"***d3d***EndScene() <%s>", ErrorStringD3D(rc) ));
    }

    if ( pContext->bFlipable )
    {
      DPF(( DBG_CNTX_PROFILE, "   d3d    Swap->FLIP" ));
      rc = pContext->lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
    }
    else
    {
      DPF(( DBG_CNTX_PROFILE, "   d3d    Swap->Blt" ));
      rc = pContext->lpDDSPrimary->Blt( &pContext->rectClient, pContext->lpDDSRender, NULL, DDBLT_WAIT, NULL );
    }
    if ( FAILED(rc) )
    {
      DPF(( DBG_CNTX_ERROR,"***d3d*** Blt/Flip (RENDER/PRIMARY) <%s>", ErrorStringD3D(rc) ));
    }

    rc = pContext->lpD3DDevice->BeginScene(); 
    if ( FAILED(rc) )
    {
      DPF(( DBG_CNTX_ERROR,"***d3d*** BeginScene() <%s>", ErrorStringD3D(rc) ));
    }
  }

  DPF(( DBG_FUNC, "<--d3d    SwapBuffersHAL" ));
  DPF(( DBG_ALL_PROFILE, "=================SWAP===================" ));

#else
  pContext->lpD3DDevice->EndScene();   

  if ( pContext->bFlipable )
    //    pContext->lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
    pContext->lpDDSPrimary->Flip( NULL, NULL );
  else
    pContext->lpDDSPrimary->Blt( &pContext->rectClient, pContext->lpDDSRender, NULL, DDBLT_WAIT, NULL );

  pContext->lpD3DDevice->BeginScene(); 

#endif
}
