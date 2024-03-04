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
#ifndef D3D_MESA_H
#define D3D_MESA_H
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#define	DIRECTDRAW_VERSION	0x0600
#include <ddraw.h>
#define	DIRECT3D_VERSION		0x0600
#include <d3d.h>
#include "matrix.h"
#include "context.h"
#include "types.h"
#include "vb.h"
#include "Debug.h"
#include "NULLProcs.h"
#include "D3DSurface.h"
#include "D3DTextureMgr.h"
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
#define	BOOL_UNKNOWN		-1
#define	UM_FATALSHUTDOWN	(WM_USER+42)
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
#define ALLOC(cb)            	malloc( (cb) )
#define FREE(p)              	{ free( (p) ); (p) = NULL; }
#define SAFE_RELEASE(p)		{ if(p) { (p)->Release(); (p)=NULL; } }
#define	DX_RESTORE(ps)		if ( (ps) && (ps)->IsLost() ) (ps)->Restore();
#define INIT_DDSD2(_d,_f)	memset( &_d, 0, sizeof(DDSURFACEDESC2) ); \
                                _d.dwSize  = sizeof( DDSURFACEDESC2 ); \
                                _d.dwFlags = (_f);
#define FLIP(y)			(((PD3DMESACONTEXT)ctx->DriverCtx)->dwHeight - (y))
#define VAR_SET(v)		( getenv( # v ) && !strcmp(getenv( # v ),"TRUE") )

/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
typedef struct _render_options
{
  BOOL	bForceSoftware,		// forces the HEL to be selected
        bStretchtoPrimary,	// not used currently
        bMipMaps,		// turn on/off mipmaps
        bTripleBuffer, 		// try and allocate triple buffer 
        bAutoTextures;

} USER_CTRL, *PUSER_CRTL;

struct __extensions__
{
   PROC  proc;
   char  *name;
};

typedef struct D3D_mesa_context 
{
  GLcontext     	*gl_ctx;       /* The core GL/Mesa context */
  GLvisual		*gl_visual;    /* Describes the buffers */
  GLframebuffer		*gl_buffer;    /* Depth, stencil, accum, etc buffers */

  HDC			hdc;
  HWND			hwnd;
  RECT      		rectClient,
                        rectViewport;
  DWORD			dwWidth,
                        dwHeight;
  WNDPROC       	hOldProc;

  int			bDBuffered,
                        bFlipable,
                        bForceSW,
                        bHardware,
                        bWindow;

  char			pszDeviceDesc[256];
  GUID			guid;
  LPDIRECTDRAW  	lpDD;
  LPDIRECTDRAW4        	lpDD4;
  LPDIRECT3D3          	lpD3D3;
  LPDIRECT3DDEVICE3    	lpD3DDevice;
  D3DDEVICEDESC		D3DDeviceDesc;
  DDDEVICEIDENTIFIER	dddi;
  LPDIRECTDRAWSURFACE4 	lpDDSPrimary,
                        lpDDSRender,
                        lpDDSZbuffer;
  LPDIRECT3DVIEWPORT3  	lpViewport;
  LPDIRECTDRAWCLIPPER	lpClipper;

  LPDIRECT3DTEXTURE2	lpD3DTexture2;
  PTEX_OBJ_HAL		pTexturesHAL;
  float			uScale,			/* Sclae values for square texture cards. */
                        vScale;

  DWORD			dwClearColor;	/* Current clear color. */
  UCHAR         	rClearColor,
                        gClearColor,
                        bClearColor,
                        aClearColor,
                        rColor,		/* Current rendering colors. */
                        gColor,
                        bColor,
                        aColor;
  BOOL			bForceAlpha2One;

  FILL_SURFACE_FUNC	FillSurface;

  struct D3D_mesa_context *next;

} D3DMESACONTEXT, *PD3DMESACONTEXT;
/*===========================================================================*/
/* Extern function prototypes.                                               */
/*===========================================================================*/
BOOL InitHAL( void );
BOOL TermHAL( void );
BOOL CreateD3DContext( PD3DMESACONTEXT pContext );
BOOL DestroyD3DContext( PD3DMESACONTEXT pContext );

void SetupNULLDDPointers( GLcontext *ctx );
BOOL MakeCurrent( PD3DMESACONTEXT pContext );
char *ErrorStringD3D( HRESULT hr );

void SetViewportHAL( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h );
GLbitfield ClearBuffersHAL( GLcontext *ctx, GLbitfield mask, GLboolean all, 
			    GLint x, GLint y, GLint width, GLint height );
GLbitfield ClearBuffersD3D( GLcontext *ctx, GLbitfield mask, GLboolean all, 
			    GLint x, GLint y, GLint width, GLint height );
void SwapBuffersHAL( PD3DMESACONTEXT pContext );
void SetRenderStates( GLcontext *ctx );
/*===========================================================================*/
/* Primitive Rendering Hooks.   					     */
/*===========================================================================*/
GLboolean RenderVertexBuffer( GLcontext *ctx, GLboolean allDone );
void RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv );
void RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );
GLboolean RenderBitmap( GLcontext *ctx,	GLint x, GLint y, GLsizei width, GLsizei height,
			const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap );
/*===========================================================================*/
/* Span Rendering Hooks.        					     */
/*===========================================================================*/
void WSpanRGB( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgb[][3], const GLubyte mask[] );
void WSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] );
void WSpanRGBAMono( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte mask[] );
void WPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], 
		  const GLubyte rgba[][4], const GLubyte mask[] );
void WPixelsRGBAMono( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte mask[] );
void RSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] );
void RPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], 
		  GLubyte rgba[][4], const GLubyte mask[] );
/*===========================================================================*/
/* Texture management Hooks.    					     */
/*===========================================================================*/
BOOL InitTMgrHAL( PD3DMESACONTEXT pContext );
void TermTMgrHAL( PD3DMESACONTEXT pContext );
void TextureLoadHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, 
		     GLint internalFormat, const struct gl_texture_image *image );
void TextureSubImageHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, 
			 GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, 
			 GLint internalFormat, const struct gl_texture_image *image );
void TextureBindHAL( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj );
void TextureDeleteHAL( GLcontext *ctx, struct gl_texture_object *tObj );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
extern PD3DMESACONTEXT	g_pD3DCurrent,     
                        g_pD3DDefault;     /* Thin support context. */
extern USER_CTRL        g_User;

#endif







