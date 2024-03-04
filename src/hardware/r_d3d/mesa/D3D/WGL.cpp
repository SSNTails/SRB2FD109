/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 Makefile for DirectX 6                                 Build 0.7 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1997  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include "D3DMesa.h"
/*===========================================================================*/
/* Window managment.                                                         */
/*===========================================================================*/
extern "C" BOOL InitOpenGL( HINSTANCE hInst );
extern "C" BOOL TermOpenGL( HINSTANCE hInst );
extern "C" HGLRC wd3CreateContext( HDC hdc )
{return wglCreateContext(hdc);}
extern "C" BOOL wd3DeleteContext( HGLRC hglrc )
{return wglDeleteContext(hglrc);}
extern "C" BOOL wd3MakeCurrent( HDC hdc, HGLRC hglrc )
{return wglMakeCurrent( hdc, hglrc );}
static BOOL UnBindWindow( PD3DMESACONTEXT pContext );
static BOOL UpdateWindowSize( PD3DMESACONTEXT pContext );
static void DestroyContext( PD3DMESACONTEXT pContext );
LONG APIENTRY wglMonitorProc( HWND hwnd, UINT message, UINT wParam, LONG lParam );
/*===========================================================================*/
/* Mesa CALLBACK structures.                                                 */
/*===========================================================================*/
static void SetupDDPointers( GLcontext *ctx );
static void SetupDDPointersNULL( GLcontext *ctx );
static void SetupDDPointersHAL( GLcontext *ctx );
/*===========================================================================*/
/* Misc hooks.                                                               */
/*===========================================================================*/
static const char *RendererString( void );
/*===========================================================================*/
/* State Management Hooks.                                                   */
/*===========================================================================*/
static void SetColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
static void ClearColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
static GLboolean SetBuffer( GLcontext *ctx, GLenum mode );
/*===========================================================================*/
/* Window Management Hooks. 						     */
/*===========================================================================*/
static void GetBufferSize( GLcontext *ctx, GLuint *width, GLuint *height );
static void Flush( GLcontext *ctx );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
struct __extensions__   g_ext[] = 
{
    { (PROC)glPolygonOffsetEXT,        "glPolygonOffsetEXT"       },
    { (PROC)glBlendEquationEXT,        "glBlendEquationEXT"       },
    { (PROC)glBlendColorEXT,           "glBlendColorExt"          },
    { (PROC)glVertexPointerEXT,        "glVertexPointerEXT"       },
    { (PROC)glNormalPointerEXT,        "glNormalPointerEXT"       },
    { (PROC)glColorPointerEXT,         "glColorPointerEXT"        },
    { (PROC)glIndexPointerEXT,         "glIndexPointerEXT"        },
    { (PROC)glTexCoordPointerEXT,      "glTexCoordPointer"        },
    { (PROC)glEdgeFlagPointerEXT,      "glEdgeFlagPointerEXT"     },
    { (PROC)glGetPointervEXT,          "glGetPointervEXT"         },
    { (PROC)glArrayElementEXT,         "glArrayElementEXT"        },
    { (PROC)glDrawArraysEXT,           "glDrawArrayEXT"           },
    { (PROC)glAreTexturesResidentEXT,  "glAreTexturesResidentEXT" },
    { (PROC)glBindTextureEXT,          "glBindTextureEXT"         },
    { (PROC)glDeleteTexturesEXT,       "glDeleteTexturesEXT"      },
    { (PROC)glGenTexturesEXT,          "glGenTexturesEXT"         },
    { (PROC)glIsTextureEXT,            "glIsTextureEXT"           },
    { (PROC)glPrioritizeTexturesEXT,   "glPrioritizeTexturesEXT"  },
    { (PROC)glCopyTexSubImage3DEXT,    "glCopyTexSubImage3DEXT"   },
    { (PROC)glTexImage3DEXT,           "glTexImage3DEXT"          },
    { (PROC)glTexSubImage3DEXT,        "glTexSubImage3DEXT"       },
};

D3DMESACONTEXT	*g_pD3DCurrent,     
                *g_pD3DDefault; /* Thin support context. */
int		g_qt_ext = sizeof(g_ext) / sizeof(g_ext[0]);
/*===========================================================================*/
/*  When a process loads this DLL we will setup the linked list for context  */
/* management and create a default context that will support the API until   */
/* the user creates and binds thier own.  This THIN default context is useful*/
/* to have around.                                                           */
/*  When the process terminates we will clean up all resources here.         */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
#if 0
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD reason, LPVOID reserved )
{
  switch( reason ) 
  {
    case DLL_PROCESS_ATTACH:
      //    case DLL_THREAD_ATTACH:
      return InitOpenGL( hInst );



    case DLL_PROCESS_DETACH:
      return TermOpenGL( hInst );
  }	

  return TRUE;
}
#endif
/*===========================================================================*/
/*  This function is called when the DLL first gets loaded so I take this    */
/* time to make a thin layer of support for the OpenGL API.  First a D3DMesa */
/* context is created and saved in the global g_pD3DDefault.  Next a Mesa    */
/* visual and frame buffer is created.  This context uses the NULLDDPointers */
/* to fill the driver hooks.  Now we have a context that can be made current */
/* when we are at a unknown state and still not trap on an API call like     */
/* glViewport().                                                             */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL InitOpenGL( HINSTANCE hInst )
{
  /* Allocate and clear the default context. */
  g_pD3DDefault = (PD3DMESACONTEXT)ALLOC( sizeof(D3DMESACONTEXT) );
  if ( g_pD3DDefault == NULL )
    return FALSE;
  memset( g_pD3DDefault, 0, sizeof(D3DMESACONTEXT) );

  /*  Update the link.  We uses a circular list so that it is easy to  */
  /* add and search.  This context will also be used for head and tail.*/
  g_pD3DDefault->next = g_pD3DDefault;

  /*========================================================================*/
  /* Do all core Mesa stuff.                                                */
  /*========================================================================*/
  g_pD3DDefault->gl_visual = gl_create_visual( TRUE,
					       GL_FALSE,   /* software alpha */
					       FALSE,      /* db_flag */
					       GL_FALSE,   /* stereo */
					       16,         /* depth_bits */
					       8,          /* stencil_bits */
					       8,          /* accum_bits */
					       0,          /* index bits */
					       8,8,8,8 );  /* r, g, b, a bits */
  if ( g_pD3DDefault->gl_visual == NULL) 	
  {
    FREE( g_pD3DDefault );
    return FALSE;
  }

  /* Allocate a new Mesa context */
  g_pD3DDefault->gl_ctx = gl_create_context( g_pD3DDefault->gl_visual, NULL, g_pD3DDefault, GL_TRUE );
  if ( g_pD3DDefault->gl_ctx == NULL ) 
  {
    gl_destroy_visual( g_pD3DDefault->gl_visual );
    FREE( g_pD3DDefault );
    return FALSE;
  }

  /* Allocate a new Mesa frame buffer */
  g_pD3DDefault->gl_buffer = gl_create_framebuffer( g_pD3DDefault->gl_visual );
  if ( g_pD3DDefault->gl_buffer == NULL )
  {
    gl_destroy_visual( g_pD3DDefault->gl_visual );
    gl_destroy_context( g_pD3DDefault->gl_ctx );
    FREE( g_pD3DDefault );
    return FALSE;
  }

  /* Put this context into play. */
  MakeCurrent( g_pD3DDefault );

  /* Initialize all the DDraw/Direct3D stuff thats global to all context. */
  InitHAL();

  return TRUE;
}
/*===========================================================================*/
/*  This function will release all resources used by the DLL.  Every context */
/* will be clobbered by releaseing all driver desources and then freeing the */
/* context memory.  Most all the work is done in DestroyContext.             */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL TermOpenGL( HINSTANCE hInst )
{
  D3DMESACONTEXT *pTmp,
                 *pNext;

  /* Just incase we are still getting paint msg. */
  MakeCurrent( g_pD3DDefault );

  /* Walk the list until we get back to the default context. */
  for( pTmp = g_pD3DDefault->next; pTmp != g_pD3DDefault; pTmp = pNext )
  {
    pNext = pTmp->next;
    DestroyContext( pTmp );
  }
  DestroyContext( g_pD3DDefault );

  /* Shutdown all the DDRaw/Direct3D stuff thats global to all context. */
  TermHAL();

  return TRUE;
}
/*===========================================================================*/
/*  This function will create a new D3D context but will not create the D3D  */
/* surfaces or even an instance of D3D as the context size is not know at    */
/* this time.  The only stuff that gets done here is the internal Mesa stuff */
/* and some Win32 handles.                                                   */
/*===========================================================================*/
/* RETURN: casted pointer to the context, NULL.                              */
/*===========================================================================*/


HGLRC APIENTRY wglCreateContext( HDC hdc )
{
  D3DMESACONTEXT *pNewContext;

  /* ALLOC and clear the new context. */
  pNewContext = (PD3DMESACONTEXT)ALLOC( sizeof(D3DMESACONTEXT) );
  if ( pNewContext == NULL )
  {
    SetLastError( 0 );
    return (HGLRC)NULL;
  }
  memset( pNewContext, 0, sizeof(D3DMESACONTEXT) );

  /*========================================================================*/
  /* Do all core Mesa stuff.                                                */
  /*========================================================================*/
  pNewContext->gl_visual = gl_create_visual( TRUE,
					     GL_TRUE,   /* software alpha */
					     TRUE,       /* db_flag */
					     GL_FALSE,   /* stereo */
					     16,         /* depth_bits */
					     8,          /* stencil_bits */
					     8,          /* accum_bits */
					     0,          /* index bits */
					     8,8,8,8 );  /* r, g, b, a bits */
  if ( pNewContext->gl_visual == NULL) 
  {
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /* Allocate a new Mesa context */
  pNewContext->gl_ctx = gl_create_context( pNewContext->gl_visual, NULL, pNewContext, GL_TRUE );
  if ( pNewContext->gl_ctx == NULL ) 
  {
    gl_destroy_visual( pNewContext->gl_visual );
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /* Allocate a new Mesa frame buffer */
  pNewContext->gl_buffer = gl_create_framebuffer( pNewContext->gl_visual );
  if ( pNewContext->gl_buffer == NULL )
  {
    gl_destroy_visual( pNewContext->gl_visual );
    gl_destroy_context( pNewContext->gl_ctx );
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /*========================================================================*/
  /* Do all the driver stuff.                                               */
  /*========================================================================*/
  pNewContext->hdc       	= hdc;
  pNewContext->bHardware 	= BOOL_UNKNOWN;
  pNewContext->bWindow   	= BOOL_UNKNOWN;
  pNewContext->bForceAlpha2One 	= FALSE;

  /* Add to circular list. */
  pNewContext->next   = g_pD3DDefault->next;
  g_pD3DDefault->next = pNewContext; 

  return (HGLRC)pNewContext;
}
/*===========================================================================*/
/*  This is a wrapper function that is supported by my own internal function.*/
/* that takes my own D3D Mesa context structure.  This so I can reuse the    */
/* function (no need for speed).                                             */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL APIENTRY wglDeleteContext( HGLRC hglrc )
{
  DestroyContext( (D3DMESACONTEXT *)hglrc );

  return TRUE;
}
/*===========================================================================*/
/*  This function is an internal function that will clean up all the Mesa    */
/* context bound to this D3D context.  Also any D3D stuff that this context  */
/* uses will be unloaded.                                                    */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static void DestroyContext( PD3DMESACONTEXT pContext )
{
  PD3DMESACONTEXT pTmp;

  /* Walk the list until we find the context before this one. */
  for( pTmp = g_pD3DDefault; pTmp && (pTmp->next != pContext); pTmp = pTmp->next )
    if ( pTmp == pTmp->next )
      break;

  /* If we never found it it must already be deleted. */
  if ( pTmp->next != pContext )
    return;

  /* Make sure we are not using this context. */
  UnBindWindow( pContext );

  /* Put our thin layer in if this context was current. */
  if ( pContext == g_pD3DCurrent )
    MakeCurrent( g_pD3DDefault );

  /* Free the Mesa stuff. */
  if ( pContext->gl_visual ) 
  {
    gl_destroy_visual( pContext->gl_visual );
    pContext->gl_visual = NULL;
  }
  if ( pContext->gl_buffer ) 
  {
    gl_destroy_framebuffer( pContext->gl_buffer );
    pContext->gl_buffer = NULL;
  }
  if ( pContext->gl_ctx )    
  {
    gl_destroy_context( pContext->gl_ctx );
    pContext->gl_ctx = NULL;
  }	

  /* Now dump the D3D. */
  DestroyD3DContext( pContext );

  /* Update the previous context's link. */
  pTmp->next = pContext->next;

  /* Gonzo. */
  FREE( pContext );
}
/*===========================================================================*/
/*  This is a wrapper function that is supported by MakeCurrent.             */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY  wglMakeCurrent( HDC hdc, HGLRC hglrc )
{
  /*=================================================*/
  /* Make for a fast redundant use of this function. */
  /*=================================================*/
  if ( g_pD3DCurrent == (PD3DMESACONTEXT)hglrc )
    return TRUE;

  return MakeCurrent( (PD3DMESACONTEXT)hglrc );
}
/*===========================================================================*/
/*  MakeCurrent will unbind whatever context is current (if any) & then bind */
/* the supplied context.  A context that is bound has it's window proc hooked*/
/* with the wglMonitorProc and the context pointer is saved in g_pD3DCurrent.*/
/* Once the context is bound we update the Mesa-3.0 hooks (SetDDPointers) and*/
/* the viewport (Mesa-.30 and DX6).                                          */
/*===========================================================================*/
/* RETURN: TRUE                                                              */
/*===========================================================================*/
BOOL MakeCurrent( PD3DMESACONTEXT pContext )
{
  D3DMESACONTEXT *pNext;

  /*====================================================================*/
  /* This is a special case that is a request to have no context bound. */
  /*====================================================================*/
  if ( pContext == NULL )
  {
    /* Walk the whole list. We start and end at the Default context. */
    for( pNext = g_pD3DDefault->next; pNext != g_pD3DDefault; pNext = pNext->next )
      UnBindWindow( pNext );

    /* Put our support layer in place. */
    return MakeCurrent( g_pD3DDefault );
  }

  /*=============================*/
  /* Unbind the current context. */
  /*=============================*/
  UnBindWindow( g_pD3DCurrent );

  /* Make sure our D3D context is still valid. */
  UpdateWindowSize( pContext );

  /* Let Mesa-3.0 know we have a new context. */
  SetupDDPointers( pContext->gl_ctx );
  gl_make_current( pContext->gl_ctx, pContext->gl_buffer );
  gl_DepthRange( pContext->gl_ctx, pContext->gl_ctx->Viewport.Near, pContext->gl_ctx->Viewport.Far );

  /* Make sure we have a viewport. */
  if ( (pContext->gl_ctx->Viewport.Width == 0) || (pContext->gl_ctx->Viewport.Height == 0) )
  {
    gl_Viewport( pContext->gl_ctx, 0, 0, pContext->dwWidth, pContext->dwHeight );
  }

  /*==================================*/
  /* Hook this context to the window. */
  /*==================================*/
  if ( pContext != g_pD3DDefault )
  {
    pContext->hOldProc = (WNDPROC)GetWindowLong( WindowFromDC(pContext->hdc), GWL_WNDPROC );
    SetWindowLong( WindowFromDC(pContext->hdc), GWL_WNDPROC, (LONG)wglMonitorProc );
  }

  /* Declare it current internally. */
  g_pD3DCurrent = pContext;

  return TRUE;
}
/*===========================================================================*/
/*  This function will pull the supplied context away from Win32.  Basicly it*/
/* will remove the hook from the window Proc.  Callers should beware that    */
/* the Default context should be made current after this function is called. */
/* I didn't put the code to make the default current as I felt it was easier */
/* to read the code this way.                                                */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static BOOL UnBindWindow( PD3DMESACONTEXT pContext )
{
  /* Return FALSE so this can be used with a linked list and a loop. */
  if ( pContext == NULL )
    return FALSE;

  /* We only unbind a context if it isn't our default. */
  if ( pContext != g_pD3DDefault )
  {
    /* Remove the window hook. */
    SetWindowLong( WindowFromDC(pContext->hdc), GWL_WNDPROC, (LONG)pContext->hOldProc );
    pContext->hOldProc = NULL;
  }

  return TRUE;
}
/*===========================================================================*/
/*  This function will only return the current window size.                  */   
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void GetBufferSize( GLcontext *ctx, GLuint *width, GLuint *height )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* Fall through for the default because that is one of the uses for it. */
  if ( pContext == g_pD3DDefault )
  {
    *width  = 0;
    *height = 0;
  }
  else
  {
    *width  = pContext->dwWidth;
    *height = pContext->dwHeight;
  }
}
/*===========================================================================*/
/*  This function will first validate the supplied context before going on   */
/* to the window management.  Next we find the size of the window that the   */
/* context is bound to and determine if it is full screen or not.            */
/*  Next we compare the findings with the current values in the context.  If */
/* we find that the window attribs have changed at all then we will destroy  */
/* the D3D context and create a new one with these attribs.                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static BOOL UpdateWindowSize( PD3DMESACONTEXT pContext )
{
  static BOOL bLocked = FALSE;
  POINT	pt;
  RECT  rectClient;
  DWORD	dwWidth,
        dwHeight;
  BOOL	bWindow = FALSE;
  int	rc;

  /* Validate the context. */
  if ( (pContext == NULL)                    ||
       (WindowFromDC(pContext->hdc) == NULL) || 
       (pContext == g_pD3DDefault)           ||
       (bLocked == TRUE) )
    return FALSE;

  /*  Make this single threaded as we have problems with DirectX */
  /* causing a resize before I'm ready.                          */
  bLocked = TRUE;

  /* Get the current window dimensions. */
  GetClientRect( WindowFromDC(pContext->hdc), &rectClient );
  pt.x = pt.y = 0;
  ClientToScreen( WindowFromDC(pContext->hdc), &pt );
  OffsetRect( &rectClient, pt.x, pt.y);

  /* Compare the window to the screen to see if its a window or not. */
  if ( (rectClient.left > 0) || 
       (rectClient.top > 0)  ||
       (rectClient.right > GetSystemMetrics(SM_CXSCREEN)) || 
       (rectClient.bottom > GetSystemMetrics(SM_CYSCREEN)) )
    bWindow = TRUE;

  /* Calculate it once as we use these values more then once. */
  dwWidth  = rectClient.right - rectClient.left;
  dwHeight = rectClient.bottom - rectClient.top;

  /* Has the window changed and is it valid? */
  if ( ((dwWidth > 1) && (dwHeight > 1)) &&
       ((pContext->bWindow != bWindow) ||
        (pContext->dwWidth != dwWidth) ||
        (pContext->dwHeight != dwHeight)) )
  {
    /* Destroy the old D3D context. */
    DestroyD3DContext( pContext );

    /* Save then new attribs for the window. */
    memcpy( &pContext->rectClient, &rectClient, sizeof(RECT) );
    pContext->dwWidth  = dwWidth;
    pContext->dwHeight = dwHeight;
    pContext->bWindow = bWindow;

    /* Create all the D3D surfaces and device. */
    rc = CreateD3DContext( pContext );
    if ( rc == FALSE )
    {
      DPF(( DBG_CNTX_ERROR, "***HAL*** CreateD3DContext()" ));
      
      bLocked = FALSE;
      return FALSE;
    }
  }

  /* We are done no so break the lock. */
  bLocked = FALSE;
  return TRUE;
}
/*===========================================================================*
/*  This function will Blt the render buffer to the PRIMARY surface. I repeat*/
/* this code for the other SwapBuffer like functions and the flush (didn't   */
/* want the function calling overhead).  Thsi could have been a macro...     */
/*                                                                           */
/* TODO: there are some problems with viewport/scissoring.                   */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY wglSwapBuffers( HDC hdc )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( g_pD3DCurrent == g_pD3DDefault )
    return FALSE;

  SwapBuffersHAL( g_pD3DCurrent );

  return TRUE;
}
/*===========================================================================*/
/*  Same as wglSwapBuffers.                                                  */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
#if 0
BOOL APIENTRY SwapBuffers( HDC hdc )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( g_pD3DCurrent == g_pD3DDefault )
    return FALSE;

  SwapBuffersHAL( g_pD3DCurrent );

  return TRUE;
}
#endif
/*===========================================================================*/
/*  This should be ok as none of the SwapBuffers will cause a redundant Blt  */
/* as none of my Swap functions will call flush.  This should also allow     */
/* sinlge buffered applications to work (not really worried though).  Some   */
/* applications may flush then swap but then this is there fault IMHO.       */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void Flush( GLcontext *ctx )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( g_pD3DCurrent == g_pD3DDefault )
    return;

  //  SwapBuffersHAL( g_pD3DCurrent );
}
/*===========================================================================*/
/*  For now this function will ignore the supplied PF. If I'm going to allow */
/* the user to choice the mode and device at startup I'm going to have to do */
/* something different.                                                      */
/*                                                                           */
/* TODO: use the linked list of modes to build a pixel format to be returned */
/*      to the caller.                                                       */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY wglChoosePixelFormat( HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
  return 1;
}
/*===========================================================================*/
/*  See wglChoosePixelFormat.                                                */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
#if 0
int APIENTRY ChoosePixelFormat( HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
  return wglChoosePixelFormat(hdc,ppfd);
}
#endif
/*===========================================================================*/
/*  This function (for now) returns a static PF everytime.  This is just to  */
/* allow things to continue.                                                 */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY wglDescribePixelFormat( HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd )
{
  static PIXELFORMATDESCRIPTOR  pfd = 
  {
    sizeof(PIXELFORMATDESCRIPTOR),   /* size */
    1,                               /* version */
    PFD_SUPPORT_OPENGL |
    PFD_DRAW_TO_WINDOW |
    PFD_DOUBLEBUFFER,                /* support double-buffering */
    PFD_TYPE_RGBA,                   /* color type */
    16,                              /* prefered color depth */
    0, 0, 0, 0, 0, 0,                /* color bits (ignored) */
    0,                               /* no alpha buffer */
    0,                               /* alpha bits (ignored) */
    0,                               /* no accumulation buffer */
    0, 0, 0, 0,                      /* accum bits (ignored) */
    16,                              /* depth buffer */
    0,                               /* no stencil buffer */
    0,                               /* no auxiliary buffers */
    PFD_MAIN_PLANE,                  /* main layer */
    0,                               /* reserved */
    0, 0, 0,                         /* no layer, visible, damage masks */
  };

  /* Return an error if we have no return buffer. */
  if ( ppfd == NULL )
    return 0;

  /* Copy the pixelformat to the supplied buffer. */
  memcpy( ppfd, &pfd, (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) ? nBytes : sizeof(PIXELFORMATDESCRIPTOR) );

  return 1;
}
/*===========================================================================*/
/*  See wglDescribePixelFormat.                                              */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY DescribePixelFormat( HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd )
{
  return wglDescribePixelFormat(hdc,iPixelFormat,nBytes,ppfd);
}
/*===========================================================================*/
/*  This function will always return 1 for now.  Just to allow for support.  */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY wglGetPixelFormat( HDC hdc )
{
  return 1;
}
/*===========================================================================*/
/*  See wglGetPixelFormat.                                                   */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY GetPixelFormat( HDC hdc )
{
  return wglGetPixelFormat(hdc);
}
/*===========================================================================*/
/*  This will aways work for now.                                            */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL APIENTRY wglSetPixelFormat( HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
  return TRUE;
}
/*===========================================================================*/
/*  See wglSetPixelFormat.                                                   */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
#if 0
BOOL APIENTRY SetPixelFormat( HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
  return wglSetPixelFormat(hdc,iPixelFormat,ppfd);
}
#endif
/*===========================================================================*/
/*  Simple getter function that uses a cast.                                 */
/*===========================================================================*/
/* RETURN: casted pointer to the context, NULL.                              */
/*===========================================================================*/
HGLRC APIENTRY wglGetCurrentContext( VOID )
{
  return (g_pD3DCurrent) ? (HGLRC)g_pD3DCurrent : (HGLRC)NULL;
}
/*===========================================================================*/
/* No support.                                                               */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/* No support.                                                               */
/*===========================================================================*/
/* RETURN: NULL.                                                             */
/*===========================================================================*/
HGLRC APIENTRY wglCreateLayerContext( HDC hdc,int iLayerPlane )
{
  SetLastError( 0 );
  return (HGLRC)NULL;
}
/*===========================================================================*/
/*  Simple getter function.                                                  */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
HDC APIENTRY wglGetCurrentDC( VOID )
{
  return (g_pD3DCurrent) ? g_pD3DCurrent->hdc : (HDC)NULL;
}
/*===========================================================================*/
/*  Simply call that searches the supported extensions for a match & returns */
/* the pointer to the function that lends support.                           */
/*===========================================================================*/
/* RETURN: pointer to API call, NULL.                                        */
/*===========================================================================*/
PROC APIENTRY wglGetProcAddress( LPCSTR lpszProc )
{
  int   index;

  for( index = 0; index < g_qt_ext; index++ )
    if( !strcmp(lpszProc,g_ext[index].name) )
      return g_ext[index].proc;
  
  SetLastError( 0 );
  return NULL;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglShareLists( HGLRC hglrc1, HGLRC hglrc2 )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglUseFontBitmaps( HDC fontDevice, DWORD firstChar, DWORD numChars, DWORD listBase )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglUseFontBitmapsW( HDC hdc,DWORD first,DWORD count,DWORD listBase )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglUseFontOutlinesA( HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglUseFontOutlinesW( HDC hdc,DWORD first,DWORD count, DWORD listBase,FLOAT deviation, FLOAT extrusion,int format, LPGLYPHMETRICSFLOAT lpgmf )
{
  SetLastError( 0 );
  return FALSE ;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY wglSwapLayerBuffers( HDC hdc, UINT fuPlanes )
{
  SetLastError( 0 );
  return FALSE;
}
/*===========================================================================*/
/*  This function gets hooked into the window that has been bound.  We can   */
/* track the window size and position.  Also the we clean  up the currrent   */
/*  context when the window is close/destroyed.                              */
/*                                                                           */
/* TODO: there might be something wrong here as some games (Heretic II) don't*/
/*      track the window quit right.                                         */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
LONG APIENTRY wglMonitorProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
  WNDPROC	hOldProc = g_pD3DCurrent->hOldProc;
  POINT		pt;
  RECT		rectClient;

  switch( message ) 
  {
    case WM_MOVE:
      /* This should always be true I'm thinking. */
      if ( WindowFromDC(g_pD3DCurrent->hdc) == hwnd )
      {
	/* Get the current window dimensions. */
	GetClientRect( hwnd, &rectClient );
	pt.x = 0;
	pt.y = 0;
	ClientToScreen( hwnd, &pt );
	OffsetRect( &rectClient, pt.x, pt.y);

	/* Save then new attribs for the window as it has moved. */
	memcpy( &g_pD3DCurrent->rectClient, &rectClient, sizeof(RECT) );
      }
      break;

    case WM_DISPLAYCHANGE:
      UpdateWindowSize( g_pD3DCurrent );
      MakeCurrent( g_pD3DCurrent );
      break;

    case WM_SIZE:
      UpdateWindowSize( g_pD3DCurrent );
      MakeCurrent( g_pD3DCurrent );
      break;

    case WM_CLOSE:
    case WM_DESTROY:
      /* Support the API until we die... */
      if ( g_pD3DCurrent != g_pD3DDefault )
      {
	DestroyContext( g_pD3DCurrent );
	MakeCurrent( g_pD3DDefault );
      }
      break;
  }

  /* Make sure we have a backup window proc... */
  return ( hOldProc ) ? 
         (hOldProc)(hwnd,message,wParam,lParam) : 
         DefWindowProc(hwnd,message,wParam,lParam);
}

/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/**********************************************************************/

/*===========================================================================*/
/*  Not reacting to this as I'm only supporting drawing to the back buffer   */
/* right now.                                                                */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
static GLboolean SetBuffer( GLcontext *ctx, GLenum mode )
{
  return TRUE;
}
/*===========================================================================*/
/*  This build a 32bit color value using the four color components.  It saves*/
/* having to do it per clear.                                                */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void ClearColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  pContext->dwClearColor = (a << 24) | (r << 16) | (g <<  8) | b;
  pContext->rClearColor = r;
  pContext->gClearColor = g;
  pContext->bClearColor = b;
  pContext->aClearColor = a;
}
/*===========================================================================*/
/*  This function could be better I guess but I decided just to grab the four*/
/* components and store then seperately.  Makes it easier to use IMHO.       */
/* (is there an echo in here?)                                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  pContext->aColor = a;
  pContext->bColor = b;
  pContext->gColor = g;
  pContext->rColor = r;
}
/*===========================================================================*/
/*  This fuction will build a render string based on what type of context is */
/* current.                                                                  */
/*===========================================================================*/
/* RETURN: pointer to the string.                                            */
/*===========================================================================*/
static const char *RendererString( void )
{
  static char pszRender[256];

  sprintf( pszRender, "altD3D %s", 
	   (g_pD3DCurrent == g_pD3DDefault) ? "(NULL)" : g_pD3DCurrent->dddi.szDescription );

  return (const char *)pszRender;
}
/*===========================================================================*/
/*  This function will choose which set of pointers Mesa will use based on   */
/* whether we hard using hardware or software.  I have added another set of  */
/* pointers that will do nothing but stop the API from crashing.             */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetupDDPointers( GLcontext *ctx )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* If don't have a D3DDevice then use the NULL proc. */
  ctx->Driver.UpdateState = (pContext->lpD3DDevice) ? SetupDDPointersHAL : SetupDDPointersNULL;

  /* Make sure that Mesa has all the pointers filled in. */
  (*ctx->Driver.UpdateState)( ctx );
}
/*===========================================================================*/
/*  This function will populate all the Mesa driver hooks. This version of   */
/* hooks will do nothing but support the API when we don't have a valid      */
/* context bound.  This is mostly for applications that don't behave right   */
/* and also to help exit as clean as possable when we have a FatalError.     */
/*===========================================================================*/
/* RETURN: pointer to the specific function.                                 */
/*===========================================================================*/
static void SetupDDPointersNULL( GLcontext *ctx )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* State management hooks. */
  ctx->Driver.Color                = NULLSetColor;
  ctx->Driver.ClearColor           = NULLClearColor;
  ctx->Driver.Clear                = NULLClearBuffers;
  ctx->Driver.SetBuffer            = NULLSetBuffer;

  /* Window management hooks. */
  ctx->Driver.GetBufferSize        = NULLGetBufferSize;

  /* Primitive rendering hooks. */
  ctx->Driver.TriangleFunc         = NULL;
  ctx->Driver.RenderVB             = NULL;

  /* Pixel/span writing functions: */
  ctx->Driver.WriteRGBASpan        = NULLWrSpRGBA;
  ctx->Driver.WriteRGBSpan         = NULLWrSpRGB;
  ctx->Driver.WriteMonoRGBASpan    = NULLWrSpRGBAMono;
  ctx->Driver.WriteRGBAPixels      = NULLWrPiRGBA;
  ctx->Driver.WriteMonoRGBAPixels  = NULLWrPiRGBAMono;

  /* Pixel/span reading functions: */
  ctx->Driver.ReadRGBASpan         = NULLReSpRGBA;
  ctx->Driver.ReadRGBAPixels       = NULLRePiRGBA;
  
  /* Misc. hooks. */
  ctx->Driver.Flush                = NULL;
  ctx->Driver.Finish               = NULL;
  ctx->Driver.RendererString 	   = RendererString;
}
/*===========================================================================*/
/*  This function will populate all the Mesa driver hooks. There are two of  */
/* these functions.  One if we have hardware support and one is there is only*/
/* software.  These functions will be called by Mesa and by the wgl.c when we*/
/* have resized (or created) the buffers.  The thing is that if a window gets*/
/* resized we may loose hardware support or gain it...                       */
/*===========================================================================*/
/* RETURN: pointer to the specific function.                                 */
/*===========================================================================*/
static void SetupDDPointersHAL( GLcontext *ctx )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;

  DPF(( DBG_FUNC, "   d3d--> SetRenderStatesHW" ));

  /* State management hooks. */
  ctx->Driver.Color                = SetColor;
  ctx->Driver.ClearColor           = ClearColor;
  ctx->Driver.Clear                = ClearBuffersHAL;
  ctx->Driver.SetBuffer            = SetBuffer;

  /* Window management hooks. */
  ctx->Driver.GetBufferSize        = GetBufferSize;
  ctx->Driver.Viewport             = SetViewportHAL;

  /* Primitive rendering hooks. */
  ctx->Driver.TriangleFunc         = RenderOneTriangle;
  ctx->Driver.LineFunc 		   = RenderOneLine;
  ctx->Driver.RenderVB             = RenderVertexBuffer;
  ctx->Driver.Bitmap		   = RenderBitmap;

  /* Pixel/span writing functions: */
  ctx->Driver.WriteRGBASpan        = WSpanRGBA;
  ctx->Driver.WriteRGBSpan         = WSpanRGB;
  ctx->Driver.WriteMonoRGBASpan    = WSpanRGBAMono;
  ctx->Driver.WriteRGBAPixels      = WPixelsRGBA;
  ctx->Driver.WriteMonoRGBAPixels  = WPixelsRGBAMono;

  /* Pixel/span reading functions: */
  ctx->Driver.ReadRGBASpan         = RSpanRGBA;
  ctx->Driver.ReadRGBAPixels       = RPixelsRGBA;

  /* Texture management hooks. */
  ctx->Driver.BindTexture          = TextureBindHAL;
  ctx->Driver.TexImage             = TextureLoadHAL;
  ctx->Driver.TexSubImage          = TextureSubImageHAL;
  ctx->Driver.DeleteTexture	   = TextureDeleteHAL;

  /* Misc. hooks. */
  ctx->Driver.Flush                = Flush;
  ctx->Driver.Finish               = Flush;
  ctx->Driver.RendererString 	   = RendererString;
}

