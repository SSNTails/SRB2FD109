/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                	  Build 0.7 */
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
/*===========================================================================*/
/*  This call will clear the render surface using the pixel info built from  */
/* the surface at creation time.  The call uses Lock/Unlock to access the    */
/* surface.  The call also special cases a full clear or a dirty rectangle.  */
/*  Finally the call returns the new clear mask that reflects that the color */
/* buffer was cleared.                                                       */
/*===========================================================================*/
/* RETURN: the original mask with the bits cleared that represents the buffer*/
/* or buffers we just cleared.                                               */
/*===========================================================================*/
GLbitfield ClearBuffersD3D( GLcontext *ctx, GLbitfield mask, GLboolean all, 
			    GLint x, GLint y, GLint width, GLint height )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;

  return (mask & ~GL_COLOR_BUFFER_BIT);
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGB( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgb[][3], const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The color is solved once from the current    */
/* color components.  The alpha is ignored as Mesa is doing it in SW.        */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGBAMono( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte rgba[][4], const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The color is solved once from the current    */
/* color components.  The alpha is ignored as Mesa is doing it in SW.        */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WPixelsRGBAMono( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc isn't written for speed rather its to handle the general case. */
/* I grab each pixel from the surface and unpack the info using the PIXELINFO*/
/* structure that was generated from the OffScreen surface pixelformat.  The */
/* function will not fill in the alpha value as Mesa I have Mesa allocate its*/
/* own alpha channel when the context was created.  I did this as I didn't   */
/* feel that it was worth the effort to try and get HW to work (bus bound).  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void RSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
/*===========================================================================*/
/*  This proc isn't written for speed rather its to handle the general case. */
/* I grab each pixel from the surface and unpack the info using the PIXELINFO*/
/* structure that was generated from the OffScreen surface pixelformat.  The */
/* function will not fill in the alpha value as Mesa I have Mesa allocate its*/
/* own alpha channel when the context was created.  I did this as I didn't   */
/* feel that it was worth the effort to try and get HW to work (bus bound).  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void RPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], GLubyte rgba[][4], const GLubyte mask[] )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          	dwColor;
}
