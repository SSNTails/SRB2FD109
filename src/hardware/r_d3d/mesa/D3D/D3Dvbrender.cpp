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
#include "D3DMesa.h"
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
#define MESA_RGBA_D3D(c)	( (c[3]<<24) | (c[0]<<16) | (c[1]<<8) | c[2] )
/*===========================================================================*/
/* Local function prototypes.                                                */
/*===========================================================================*/
static void RenderPointsVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleFanVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleStripVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderQuadVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderQuad( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4, GLuint pv );
void RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv );
void RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );

static void OffsetPolygon( GLcontext *ctx, GLfloat a, GLfloat b, GLfloat c );
static void RenderClippedPolygon( GLcontext *ctx, GLuint n, GLuint vlist[] );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
D3DTLVERTEX	D3DTLVertices[ (VB_MAX*6) ];    // Points need 6 vertices each...
GLuint   	VList[VB_SIZE];
/*===========================================================================*/
/*  This function gets called when either the vertex buffer is full or glEnd */
/* has been called.  If the we aren't in rendering mode (FEEDBACK) then I    */
/* pass the vertex buffer back to Mesa to deal with by returning FALSE.      */
/*  If I can render the primitive types in the buffer directly then I will   */
/* return TRUE after I render the vertex buffer and reset the vertex buffer. */
/*                                                                           */
/* TODO: I don't handle the special case of when the vertex buffer is full   */
/*      and we have a primitive that bounds this buffer and the next one to  */
/*      come.  I'm not sure right now if Mesa handles this for me...         */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
GLboolean RenderVertexBuffer( GLcontext *ctx, GLboolean allDone )
{
  struct vertex_buffer	*VB = ctx->VB;
  GLuint 		index,
	                vlist[VB_SIZE];

  DPF(( DBG_FUNC, "   d3d--> RenderVertexBuffer" ));

  /* We only need to hook actual tri's that need rendering. */
  if ( ctx->RenderMode != GL_RENDER )
  {
    DPF(( DBG_PRIM_INFO, "   d3d    Passing VB back to Mesa" ));
    DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
    return FALSE;
  }

#if 0
  // TODO:  I have been told that the number supplied by the CAPS is not corret.
  //       So I have removed the code that keeps track of the flush count.

  /* Check to see if the D3DDevice needs to be flushed. */
  if ( (vbFlushCount+VB->Count) > ((PD3DMESACONTEXT)ctx->DriverCtx)->D3DDeviceDesc.dwMaxVertexCount )
  {
    //    ((PD3DMESACONTEXT)ctx->DriverCtx)->lpD3DDevice->EndScene();   
    //    ((PD3DMESACONTEXT)ctx->DriverCtx)->lpD3DDevice->BeginScene(); 
    gl_reset_vb( ctx, allDone );
    return TRUE;
  }
#endif

  // TODO: A hack to disable shadow maps...
  if ( (ctx->Color.BlendEnabled == GL_TRUE) &&
       (ctx->Color.BlendDst == GL_SRC_COLOR) &&
       !(((PD3DMESACONTEXT)ctx->DriverCtx)->D3DDeviceDesc.dpcTriCaps.dwDestBlendCaps & D3DBLEND_SRCCOLOR) )
  {
    gl_reset_vb( ctx, allDone );
    return TRUE;
  }

  /* Set all the D3D states once before we render the VB. */
  SetRenderStates( ctx );

  for( index = 0; index < VB->Count; index++ )
  {
    //    ctx->VB->Win[index][0] += 0.25f;
    ctx->VB->Win[index][1] -= 0.25f;
  }

  switch( ctx->Primitive ) 
  {
    case GL_POINTS:
      DPF(( DBG_PRIM_INFO, "   d3d    GL_POINTS( %d )", VB->Count ));
      RenderPointsVB( ctx, 0, VB->Count );
      break;

    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
      /*  Not supported functions yet so pass back that we failed to */
      /* render the vertex buffer and Mesa will have to do it.       */
      DPF(( DBG_PRIM_INFO, "   d3d    GL_LINE_?( %d )", VB->Count ));
      DPF(( DBG_PRIM_INFO, "   d3d    Passing VB back to Mesa" ));
      DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
      return FALSE;

    case GL_TRIANGLES:
      DPF(( DBG_PRIM_INFO, "   d3d    GL_TRIANGLES( %d )", VB->Count ));

      if ( VB->Count < 3 )
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }
      RenderTriangleVB( ctx, 0, VB->Count );
      break;

    case GL_TRIANGLE_STRIP:
      DPF(( DBG_PRIM_INFO, "   d3d    GL_TRIANGLE_STRIP( %d )", VB->Count ));

      if ( VB->Count < 3 )
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }
      RenderTriangleStripVB( ctx, 0, VB->Count );
      break;

    case GL_TRIANGLE_FAN:
      DPF(( DBG_PRIM_INFO, "GL_TRIANGLE_FAN( %d )", VB->Count ));

      if ( VB->Count < 3 )
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }
      RenderTriangleFanVB( ctx, 0, VB->Count );
      break;

    case GL_QUADS:
      DPF(( DBG_PRIM_INFO, "GL_QUADS( %d )", VB->Count ));

      if ( VB->Count < 4 )
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }
      RenderQuadVB( ctx, 0, VB->Count );
      break;
	  
    case GL_QUAD_STRIP:
      DPF(( DBG_PRIM_INFO, "   d3d    GL_QUAD_STRIP( %d )", VB->Count ));

      if ( VB->Count < 4 )
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }


      if ( VB->ClipOrMask ) 
      {
	for( index = 3; index < VB->Count; index += 2 ) 
	{
	  if ( VB->ClipMask[index-3] & 
	       VB->ClipMask[index-2] & 
	       VB->ClipMask[index-1] & 
	       VB->ClipMask[index] & 
	       CLIP_ALL_BITS ) 
	  {
	    /* All points clipped by common plane */
	    continue;
	  }
	  else if ( VB->ClipMask[index-3] | 
		    VB->ClipMask[index-2] | 
		    VB->ClipMask[index-1] | 
		    VB->ClipMask[index] ) 
	  {
	    vlist[0] = index - 3;
	    vlist[1] = index - 2;
	    vlist[2] = index;
	    vlist[3] = index - 1;
	    RenderClippedPolygon( ctx, 4, vlist );
	  }
	  else 
	  {
	    RenderQuad( ctx, (index-3), (index-2), index, (index-1), index );
	  }	
	}
      }
      else 
      {
	/* No clipping needed */
	for( index = 3; index < VB->Count; index += 2 ) 
	  RenderQuad( ctx, (index-3), (index-2), index, (index-1), index );
      }	
      break;
	   
    case GL_POLYGON:
      DPF(( DBG_PRIM_INFO, "   d3d    GL_POLYGON( %d )", VB->Count ));

      if ( VB->Count < 3 ) 
      {
	DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
	return FALSE;
      }

      /* All points clipped by common plane, draw nothing */
      if ( VB->ClipAndMask & CLIP_ALL_BITS ) 
	break;

      RenderTriangleFanVB( ctx, 0, VB->Count );
      break;

    default:
      /* should never get here */
      gl_problem( ctx, "Invalid mode in gl_render_vb" );
   }
 
  DPF(( DBG_PRIM_INFO, "   d3d    ResetVB" ));

  /* We return TRUE to indicate we rendered the VB. */
  gl_reset_vb( ctx, allDone );

  DPF(( DBG_FUNC, "<--d3d    RenderVertexBuffer" ));
  return TRUE;
}
/*===========================================================================*/
/*  Render a polygon that needs clipping on at least one vertex. The function*/
/* will first clip the polygon to any user clipping planes then clip to the  */
/* viewing volume.  The final polygon will be draw as single triangles that  */
/* first need minor proccessing (culling, offset, etc) before we draw the    */
/* polygon as a fan.  NOTE: the fan is draw as single triangles as its not   */
/* formed sequentaly in the VB but is in the vlist[].                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderClippedPolygon( GLcontext *ctx, GLuint n, GLuint vlist[] )
{
  struct vertex_buffer	*VB = ctx->VB;
  GLfloat 		(*win)[3] = VB->Win,
                        *proj = ctx->ProjectionMatrix,
                        ex, ey, 
                        fx, fy, c,
	                wInv; 
  GLuint		index,
	                pv,
                        facing;

  DPF(( DBG_FUNC, "   d3d--> RenderClippedPolygon" ));

  DPF(( DBG_PRIM_INFO, "   d3d    RenderClippedtPolygon( %d )", n ));

  /* Which vertex dictates the color when flat shading. */
  pv = (ctx->Primitive==GL_POLYGON) ? vlist[0] : vlist[n-1];

  /*  Clipping may introduce new vertices.  New vertices will be stored in */
  /* the vertex buffer arrays starting with location VB->Free.  After we've*/
  /* rendered the polygon, these extra vertices can be overwritten.        */
  VB->Free = VB_MAX;

  /* Clip against user clipping planes in eye coord space. */
  if ( ctx->Transform.AnyClip ) 
  {
    n = gl_userclip_polygon( ctx, n, vlist );
    if ( n < 3 )
    {
      DPF(( DBG_PRIM_INFO, "   d3d    User Clipped" ));
      return;
    }

    /* Transform vertices from eye to clip coordinates:  clip = Proj * eye */
    for( index = 0; index < n; index++ ) 
    {
      TRANSFORM_POINT( VB->Clip[vlist[index]], proj, VB->Eye[vlist[index]] );
    }
  }

  /* Clip against view volume in clip coord space */
  n = gl_viewclip_polygon( ctx, n, vlist );
  if ( n < 3 )
  {
    DPF(( DBG_PRIM_INFO, "   d3d    View Clipped" ));
    return;
  }

  /* Transform new vertices from clip to ndc to window coords.    */
  /* ndc = clip / W    window = viewport_mapping(ndc)             */
  /* Note that window Z values are scaled to the range of integer */
  /* depth buffer values.                                         */

  /* Only need to compute window coords for new vertices */
  for( index = VB_MAX; index < VB->Free; index++ ) 
  {
    if ( VB->Clip[index][3] != 0.0F ) 
    {
      wInv = 1.0F / VB->Clip[index][3];

      win[index][0] = VB->Clip[index][0] * wInv * ctx->Viewport.Sx + ctx->Viewport.Tx;
      win[index][1] = VB->Clip[index][1] * wInv * ctx->Viewport.Sy + ctx->Viewport.Ty;
      win[index][2] = VB->Clip[index][2] * wInv * ctx->Viewport.Sz + ctx->Viewport.Tz;
    }
    else 
    {
      /* Can't divide by zero, so... */
      win[index][0] = win[index][1] = win[index][2] = 0.0F;
    }
  }

  /* Draw filled polygon as a triangle fan */
  for( index = 2; index < n; index++ ) 
  {
    /* Compute orientation of triangle */
    ex = win[vlist[index-1]][0] - win[vlist[0]][0];
    ey = win[vlist[index-1]][1] - win[vlist[0]][1];
    fx = win[vlist[index]][0]   - win[vlist[0]][0];
    fy = win[vlist[index]][1]   - win[vlist[0]][1];
    c = (ex * fy) - (ey * fx);

    /* polygon is perpindicular to view plane, don't draw it */
    if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
      continue;

    /* Backface culling. */
    facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
    if ( (facing + 1) & ctx->Polygon.CullBits )  
      continue;
        
    if ( ctx->LightTwoSide ) 
    {
      if ( facing == 1 ) 
      {
	/* use back color */
	VB->Color   = VB->Bcolor;
	VB->Specular= VB->Bspec;
      }
      else 
      {
	/* use front color */
	VB->Color   = VB->Fcolor;
	VB->Specular= VB->Fspec;
      }
    }

    if ( ctx->Polygon.OffsetAny ) 
    {
      /* finish computing plane equation of polygon, compute offset */
      GLfloat fz = win[vlist[index]][2]   - win[vlist[0]][2];  
      GLfloat ez = win[vlist[index-1]][2] - win[vlist[0]][2];
      GLfloat a = (ey * fz) - (ez * fy);
      GLfloat b = (ez * fx) - (ex * fz);
      OffsetPolygon( ctx, a, b, c );
    }
    RenderOneTriangle( ctx, vlist[0], vlist[index-1], vlist[index], pv );
  }

  DPF(( DBG_FUNC, "<--d3d    RenderClippedPolygon" ));
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as triangles.  The   */
/* buffer has to be able to be rendered directly.  This means that we are    */
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleVB( GLcontext *ctx, GLuint start, GLuint end )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int			index,
                        cVertex;
  DWORD                 dwPVColor;
  GLfloat               ex, ey, 
                        fx, fy, c;
  GLuint                facing;

  DPF(( DBG_FUNC, "   d3d--> RenderTriangleVB" ));

  if ( ctx->DirectTriangles && !VB->ClipOrMask )
  {
    DPF(( DBG_PRIM_INFO, "   d3d    DirectTriangles( %d )", (end-start) ));
    for( index = start, cVertex = 0; index < end; )
    {
      dwPVColor = (VB->Color[(index+2)][3]<<24) | 
	          (VB->Color[(index+2)][0]<<16) | 
                  (VB->Color[(index+2)][1]<<8)  | 
                   VB->Color[(index+2)][2];
   
      /*=====================================*/
      /* Populate the the triangle vertices. */
      /*=====================================*/
      D3DTLVertices[cVertex].sx     = (float)VB->Win[index][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP(VB->Win[index][1]);
      D3DTLVertices[cVertex].sz     = (float)VB->Win[index][2];
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[index][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[index][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
	                              dwPVColor : MESA_RGBA_D3D( VB->Color[index] );
      index++;
   
      D3DTLVertices[cVertex].sx     = (float)VB->Win[index][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[index][1] );
      D3DTLVertices[cVertex].sz     = (float)VB->Win[index][2];
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[index][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[index][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor : MESA_RGBA_D3D( VB->Color[index] );
      index++;
      
      D3DTLVertices[cVertex].sx     = (float)VB->Win[index][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[index][1] );
      D3DTLVertices[cVertex].sz     = (float)VB->Win[index][2];
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[index][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[index][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex++].color= dwPVColor;
      index++;
    }
  }
  else
  {
#define v1     index
#define v2     (index+1)
#define v3     (index+2)

    for( index = start, cVertex = 0; index < end; index += 3 )
    {
      if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS ) 
      {
	continue;
      }
      else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] ) 
      {
	VList[0] = v1;
	VList[1] = v2;
	VList[2] = v3;
	RenderClippedPolygon( ctx, 3, VList );
	continue;
      }

      /* Compute orientation of triangle */
      ex = VB->Win[v2][0] - VB->Win[v1][0];
      ey = VB->Win[v2][1] - VB->Win[v1][1];
      fx = VB->Win[v3][0] - VB->Win[v1][0];
      fy = VB->Win[v3][1] - VB->Win[v1][1];
      c = (ex * fy) - (ey * fx);

      /* polygon is perpindicular to view plane, don't draw it */
      if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	continue;

      /* Backface culling. */
      facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
      if ( (facing + 1) & ctx->Polygon.CullBits )  
	continue;

      if ( ctx->LightTwoSide ) 
      {
	if ( facing == 1 ) 
	{
	  /* use back color */
	  VB->Color   = VB->Bcolor;
	  VB->Specular= VB->Bspec;
	}
	else 
	{
	  /* use front color */
	  VB->Color   = VB->Fcolor;
	  VB->Specular= VB->Fspec;
	}
      }	

      if ( ctx->Polygon.OffsetAny ) 
      {
	/* Finish computing plane equation of polygon, compute offset */
	GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];  
	GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	GLfloat a = (ey * fz) - (ez * fy);
	GLfloat b = (ez * fx) - (ex * fz);
	OffsetPolygon( ctx, a, b, c );
      }

      /*=====================================*/
      /* Populate the the triangle vertices. */
      /*=====================================*/

      /* Solve the prevoking vertex color as we need it for the 3rd triangle and flat shading. */
      dwPVColor = MESA_RGBA_D3D( VB->Color[v3] );
   
      D3DTLVertices[cVertex].sx     = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor : MESA_RGBA_D3D( VB->Color[v1] );
   
      D3DTLVertices[cVertex].sx     = (float)VB->Win[v2][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v2][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v2][2] + ctx->PolygonZoffset );

      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v2][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v2][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v2][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor : MESA_RGBA_D3D( VB->Color[v2] );
      
      D3DTLVertices[cVertex].sx     = (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );

      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color= dwPVColor;
    }
#undef v1
#undef v2
#undef v3
  }    

  /* Render the converted vertex buffer. */  
  if ( cVertex )
    pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					  D3DFVF_TLVERTEX,
					  (LPVOID)&D3DTLVertices[0],
					  cVertex, 
					  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );

  DPF(( DBG_FUNC, "<--d3d    RenderTriangleVB" ));
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as a triangle fan.   */
/* The buffer has to be able to be rendered directly.  This means that we are*/
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleFanVB( GLcontext *ctx, GLuint start, GLuint end )
{
  PD3DMESACONTEXT     	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer	*VB = ctx->VB;
  int                  	index,
                        cVertex;
  GLfloat               ex, ey, 
                        fx, fy, c;
  GLuint               	facing;
  DWORD                	dwPVColor;

  DPF(( DBG_FUNC, "   d3d--> RenderTriangleFanVB" ));

  /* Special case that we can blast the fan without culling, offset, etc... */
  if ( ctx->DirectTriangles && !VB->ClipOrMask && (ctx->Light.ShadeModel != GL_FLAT) )
  {
    DPF(( DBG_PRIM_INFO, "DirectTriangles( %d )", (end-start) ));

    /* Seed the the fan. */
    D3DTLVertices[0].sx    = (float)VB->Win[start][0];
    D3DTLVertices[0].sy    = (float)FLIP( VB->Win[start][1] ) ;
    D3DTLVertices[0].sz    = (float)VB->Win[start][2];
    D3DTLVertices[0].tu    = (float)( VB->TexCoord[start][0] * pContext->uScale );
    D3DTLVertices[0].tv    = (float)( VB->TexCoord[start][1] * pContext->vScale );
    D3DTLVertices[0].rhw   = (float)( 1.0 / VB->Clip[start][3] );
    D3DTLVertices[0].color = MESA_RGBA_D3D( VB->Color[start] );
   
    /* Seed the the fan. */
    D3DTLVertices[1].sx    = (float)VB->Win[(start+1)][0];
    D3DTLVertices[1].sy    = (float)FLIP( VB->Win[(start+1)][1] );
    D3DTLVertices[1].sz    = (float)VB->Win[(start+1)][2];
    D3DTLVertices[1].tu    = (float)( VB->TexCoord[(start+1)][0] * pContext->uScale );
    D3DTLVertices[1].tv    = (float)( VB->TexCoord[(start+1)][1] * pContext->vScale );
    D3DTLVertices[1].rhw   = (float)( 1.0 / VB->Clip[(start+1)][3] );
    D3DTLVertices[1].color = MESA_RGBA_D3D( VB->Color[start+1] );
   
    for( index = (start+2), cVertex = 2; index < end; index++, cVertex++ )
    {
      /*=================================*/
      /* Add the next vertex to the fan. */
      /*=================================*/
      D3DTLVertices[cVertex].sx    = (float)VB->Win[index][0];
      D3DTLVertices[cVertex].sy    = (float)FLIP( VB->Win[index][1] );
      D3DTLVertices[cVertex].sz    = (float)VB->Win[index][2];

      D3DTLVertices[cVertex].tu    = (float)( VB->TexCoord[index][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv    = (float)( VB->TexCoord[index][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw   = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex].color = MESA_RGBA_D3D( VB->Color[index] );
    }

    /* Render the converted vertex buffer. */  
    if ( cVertex )
      pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLEFAN,
					    D3DFVF_TLVERTEX,
					    (LPVOID)&D3DTLVertices[0],
					    cVertex, 
					    (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  }
  else
  {
#define v1     start
#define v2     (index-1)
#define v3     index

    for( index = (start+2), cVertex = 0; index < end; index++ )
    {
      if ( VB->ClipOrMask ) 
      {
	/* All points clipped by common plane */
	if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS ) 
	{
	  continue;
	}
	else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] ) 
	{
	  VList[0] = v1;
	  VList[1] = v2;
	  VList[2] = v3;
	  RenderClippedPolygon( ctx, 3, VList );
	  continue;
	}
      }

      /* Compute orientation of triangle */
      ex = VB->Win[v2][0] - VB->Win[v1][0];
      ey = VB->Win[v2][1] - VB->Win[v1][1];
      fx = VB->Win[v3][0] - VB->Win[v1][0];
      fy = VB->Win[v3][1] - VB->Win[v1][1];
      c = (ex * fy) - (ey * fx);
   
      /* polygon is perpindicular to view plane, don't draw it */
      if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	continue;

      /* Backface culling. */
      facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
      if ( (facing + 1) & ctx->Polygon.CullBits )  
	continue;
   
      if ( ctx->Polygon.OffsetAny ) 
      {
	/* Finish computing plane equation of polygon, compute offset */
	GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];  
	GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	GLfloat a = (ey * fz) - (ez * fy);
	GLfloat b = (ez * fx) - (ex * fz);
	OffsetPolygon( ctx, a, b, c );
      }

      /*=====================================*/
      /* Populate the the triangle vertices. */
      /*=====================================*/
      dwPVColor = MESA_RGBA_D3D( VB->Color[v3] );

      D3DTLVertices[cVertex].sx     = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ? 
                                      dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
      
      D3DTLVertices[cVertex].sx     = (float)VB->Win[v2][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v2][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v2][2] + ctx->PolygonZoffset  );
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v2][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v2][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v2][3] );
      D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ? 
                                      dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 
         
      D3DTLVertices[cVertex].sx     = (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz     = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color= dwPVColor;
    }

    /* Render the converted vertex buffer. */  
    if ( cVertex )
      pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					    D3DFVF_TLVERTEX,
					    (LPVOID)&D3DTLVertices[0],
					    cVertex, 
					    (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
#undef v1
#undef v2
#undef v3
  }

  DPF(( DBG_FUNC, "<--d3d    RenderTriangleFanVB" ));
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as a triangle strip. */
/* The buffer has to be able to be rendered directly.  This means that we are*/
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleStripVB( GLcontext *ctx, GLuint start, GLuint end )
{
  PD3DMESACONTEXT     	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                   index,
                        cVertex = 0,
	                v1, v2, v3;
  GLfloat              	ex, ey, 
                        fx, fy, c;
  GLuint               	facing;
  DWORD                	dwPVColor;

  DPF(( DBG_FUNC, "   d3d--> RenderTriangleStripVB" ));

  /* Special case that we can blast the fan without culling, offset, etc... */
  if ( ctx->DirectTriangles && !VB->ClipOrMask && (ctx->Light.ShadeModel != GL_FLAT) )
  {
    DPF(( DBG_PRIM_PROFILE, "   d3d    DirectTriangles" ));

    /* Seed the the strip. */
    D3DTLVertices[0].sx    = (float)VB->Win[start][0];
    D3DTLVertices[0].sy    = (float)FLIP( VB->Win[start][1] );
    D3DTLVertices[0].sz    = (float)VB->Win[start][2];
    D3DTLVertices[0].tu    = (float)( VB->TexCoord[start][0]  * pContext->uScale );
    D3DTLVertices[0].tv    = (float)( VB->TexCoord[start][1] * pContext->vScale );
    D3DTLVertices[0].rhw   = (float)( 1.0 / VB->Clip[start][3] );
    D3DTLVertices[0].color = MESA_RGBA_D3D( VB->Color[start] );

    /* Seed the the strip. */
    D3DTLVertices[1].sx    = (float)VB->Win[(start+1)][0];
    D3DTLVertices[1].sy    = (float)FLIP( VB->Win[(start+1)][1]);
    D3DTLVertices[1].sz    = (float)VB->Win[(start+1)][2];
    D3DTLVertices[1].tu    = (float)( VB->TexCoord[(start+1)][0] * pContext->uScale );
    D3DTLVertices[1].tv    = (float)( VB->TexCoord[(start+1)][1] * pContext->vScale );
    D3DTLVertices[1].rhw   = (float)( 1.0 / VB->Clip[(start+1)][3] );
    D3DTLVertices[1].color = MESA_RGBA_D3D( VB->Color[start+1] );
   
    for( index = (start+2), cVertex = 2; index < end; index++, cVertex++ )
    {
      /*===================================*/
      /* Add the next vertex to the strip. */
      /*===================================*/
      D3DTLVertices[cVertex].sx    = (float)VB->Win[index][0];
      D3DTLVertices[cVertex].sy    = (float)FLIP( VB->Win[index][1] );
      D3DTLVertices[cVertex].sz    = (float)VB->Win[index][2];

      D3DTLVertices[cVertex].tu    = (float)( VB->TexCoord[index][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv    = (float)( VB->TexCoord[index][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw   = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex].color = MESA_RGBA_D3D( VB->Color[index] ); 
    }

    /* Render the converted vertex buffer. */  
    if ( cVertex )
      pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,
					    D3DFVF_TLVERTEX,
					    (LPVOID)&D3DTLVertices[0],
					    cVertex, 
					    (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  }
  else
  {
    for( index = (start+2); index < end; index++ ) 
    {
      /* We need to switch order so that winding won't be a problem. */
      if ( index & 1 )
      {
	v1 = index - 1;
	v2 = index - 2;
	v3 = index - 0;
      }
      else
      {
	v1 = index - 2;
	v2 = index - 1;
	v3 = index - 0;
      }

      /* All vertices clipped by common plane */
      if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS ) 
	continue;

      /* Check if any vertices need clipping. */
      if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] ) 
      {
	VList[0] = v1;
	VList[1] = v2;
	VList[2] = v3;
	RenderClippedPolygon( ctx, 3, VList );
      }
      else 
      {
	/* Compute orientation of triangle */
	ex = VB->Win[v2][0] - VB->Win[v1][0];
	ey = VB->Win[v2][1] - VB->Win[v1][1];
	fx = VB->Win[v3][0] - VB->Win[v1][0];
	fy = VB->Win[v3][1] - VB->Win[v1][1];
	c = (ex * fy) - (ey * fx);
	
	/* Polygon is perpindicular to view plane, don't draw it */
	if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	  continue;

	/* Backface culling. */
	facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
	if ( (facing + 1) & ctx->Polygon.CullBits )  
	  continue;
	
	/* Need right color if we have two sided lighting. */
	if ( ctx->LightTwoSide ) 
	{
	  if ( facing == 1 ) 
	  {
	    /* use back color */
	    VB->Color   = VB->Bcolor;
	    VB->Specular= VB->Bspec;
	  }
	  else 
	  {
	    /* use front color */
	    VB->Color   = VB->Fcolor;
	    VB->Specular= VB->Fspec;
	  }
	}

	if ( ctx->Polygon.OffsetAny ) 
	{
	  /* Finish computing plane equation of polygon, compute offset */
	  GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];  
	  GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	  GLfloat a = (ey * fz) - (ez * fy);
	  GLfloat b = (ez * fx) - (ex * fz);
	  OffsetPolygon( ctx, a, b, c );
	}
	/*=====================================*/
	/* Populate the the triangle vertices. */
	/*=====================================*/
	
	/* Solve the prevoking vertex color as we need it for the 3rd triangle and flat shading. */
	dwPVColor = MESA_RGBA_D3D( VB->Color[v3] ); 
	
	D3DTLVertices[cVertex].sx     = (float)VB->Win[v1][0];
	D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v1][1] );
	D3DTLVertices[cVertex].sz     = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
	D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v1][0] * pContext->uScale );
	D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v1][1] * pContext->vScale );
	D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v1][3] );
	D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                        dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
   
	D3DTLVertices[cVertex].sx     = (float)VB->Win[v2][0];
	D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v2][1] );
	D3DTLVertices[cVertex].sz     = (float)( VB->Win[v2][2] + ctx->PolygonZoffset );
	D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v2][0] * pContext->uScale );
	D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v2][1] * pContext->vScale );
	D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v2][3] );
	D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                        dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 
      
	D3DTLVertices[cVertex].sx     = (float)VB->Win[v3][0];
	D3DTLVertices[cVertex].sy     = (float)FLIP( VB->Win[v3][1] );
	D3DTLVertices[cVertex].sz     = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
	D3DTLVertices[cVertex].tu     = (float)( VB->TexCoord[v3][0] * pContext->uScale );
	D3DTLVertices[cVertex].tv     = (float)( VB->TexCoord[v3][1] * pContext->vScale );
	D3DTLVertices[cVertex].rhw    = (float)( 1.0 / VB->Clip[v3][3] );
	D3DTLVertices[cVertex++].color= dwPVColor;
      }
    }

    /* Render the converted vertex buffer. */  
    if ( cVertex )
      pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					    D3DFVF_TLVERTEX,
					    (LPVOID)&D3DTLVertices[0],
					    cVertex, 
					    (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  }	

  DPF(( DBG_FUNC, "<--d3d    RenderTriangleStripVB" ));
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as Quads.  The buffer*/
/* has to be able to be rendered directly.  This means that we are filled, no*/
/* offsets, no culling and one sided rendering.  Also we must be in render   */
/* mode of cource.                                                           */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderQuadVB( GLcontext *ctx, GLuint start, GLuint end )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer	*VB = ctx->VB;
  int                  	index,
                        cVertex;
  DWORD                	dwPVColor;
  GLfloat             	ex, ey, 
                        fx, fy, c;
  GLuint		facing;  /* 0=front, 1=back */

  DPF(( DBG_FUNC, "   d3d--> RenderQuadVB" ));

#define  v1 (index)
#define  v2 (index+1)
#define  v3 (index+2)
#define  v4 (index+3)

  if ( ctx->DirectTriangles && !VB->ClipOrMask )
  {
    DPF(( DBG_PRIM_PROFILE, "   d3d    DirectTriangles" ));

    for( cVertex = 0, index = start; index < end; index += 4 )
    {
      if ( ctx->Light.ShadeModel == GL_FLAT )
	dwPVColor = MESA_RGBA_D3D( VB->Color[v4] ); 
   
      /*=====================================*/
      /* Populate the the triangle vertices. */
      /*=====================================*/
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v1][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v2][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v2][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v2][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v2][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v2][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v2][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 
      
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v3][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v1][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v3][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v4][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v4][1] );
      D3DTLVertices[cVertex].sz      = (float)VB->Win[v4][2];
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v4][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v4][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v4][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v4] ); 
    }
  }
  else
  {
    for( cVertex = 0, index = start; index < end; index += 4 )
    {
      if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3]  & VB->ClipMask[v4] & CLIP_ALL_BITS ) 
      {
	continue;
      }	
      else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] | VB->ClipMask[v4] ) 
      {
	VList[0] = v1;
	VList[1] = v2;
	VList[2] = v3;
	VList[3] = v4;
	RenderClippedPolygon( ctx, 4, VList );
	continue;
      }

      /* Compute orientation of triangle */
      ex = VB->Win[v2][0] - VB->Win[v1][0];
      ey = VB->Win[v2][1] - VB->Win[v1][1];
      fx = VB->Win[v3][0] - VB->Win[v1][0];
      fy = VB->Win[v3][1] - VB->Win[v1][1];
      c = (ex * fy) - (ey * fx);

      /* polygon is perpindicular to view plane, don't draw it */
      if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	continue;
      
      /* Backface culling. */
      facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
      if ( (facing + 1) & ctx->Polygon.CullBits ) 
	continue;

      if ( ctx->LightTwoSide ) 
      {
	if ( facing == 1 ) 
	{
	  /* use back color */
	  VB->Color   = VB->Bcolor;
	  VB->Specular= VB->Bspec;
	}
	else 
	{
	  /* use front color */
	  VB->Color   = VB->Fcolor;
	  VB->Specular= VB->Fspec;
	}	
      }	
      
      if ( ctx->Polygon.OffsetAny ) 
      {
	/* Finish computing plane equation of polygon, compute offset */
	GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];  
	GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	GLfloat a = (ey * fz) - (ez * fy);
	GLfloat b = (ez * fx) - (ex * fz);
	OffsetPolygon( ctx, a, b, c );
      }

      if ( ctx->Light.ShadeModel == GL_FLAT )
	dwPVColor = MESA_RGBA_D3D( VB->Color[v4] ); 
      
      /*=====================================*/
      /* Populate the the triangle vertices. */
      /*=====================================*/
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v2][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v2][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v2][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v2][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v2][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v2][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 
      
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 
      
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v1][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v1][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 
   
      D3DTLVertices[cVertex].sx      =  (float)VB->Win[v3][0];
      D3DTLVertices[cVertex].sy      =  (float)FLIP( VB->Win[v3][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 
   
      D3DTLVertices[cVertex].sx      = (float)VB->Win[v4][0];
      D3DTLVertices[cVertex].sy      = (float)FLIP( VB->Win[v4][1] );
      D3DTLVertices[cVertex].sz      = (float)( VB->Win[v4][2] + ctx->PolygonZoffset );
      D3DTLVertices[cVertex].tu      = (float)( VB->TexCoord[v4][0] * pContext->uScale );
      D3DTLVertices[cVertex].tv      = (float)( VB->TexCoord[v4][1] * pContext->vScale );
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[v4][3] );
      D3DTLVertices[cVertex++].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                                       dwPVColor : MESA_RGBA_D3D( VB->Color[v4] ); 
    }
  }	

#undef   v4
#undef   v3
#undef   v2
#undef   v1

  /* Render the converted vertex buffer. */  
  if ( cVertex )
    pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					  D3DFVF_TLVERTEX,
					  (LPVOID)&D3DTLVertices[0],
					  cVertex, 
					  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );

  DPF(( DBG_FUNC, "<--d3d    RenderQuadVB" ));
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static void RenderQuad( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4, GLuint pv )
{
  PD3DMESACONTEXT     	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  DWORD                	dwPVColor;
  GLfloat              	ex, ey, 
                        fx, fy, c;
  GLuint               	facing;  /* 0=front, 1=back */
  static D3DTLVERTEX   	TLVertices[6];

  DPF(( DBG_FUNC, "   d3d--> RenderQuad" ));

  /* Compute orientation of triangle */
  ex = VB->Win[v2][0] - VB->Win[v1][0];
  ey = VB->Win[v2][1] - VB->Win[v1][1];
  fx = VB->Win[v3][0] - VB->Win[v1][0];
  fy = VB->Win[v3][1] - VB->Win[v1][1];
  c = (ex * fy) - (ey * fx);

  /* polygon is perpindicular to view plane, don't draw it */
  if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
    return;

  /* Backface culling. */
  facing = (c < 0.0F) ^ (ctx->Polygon.FrontFace == GL_CW);
  if ( (facing + 1) & ctx->Polygon.CullBits )  
    return;

  if ( ctx->LightTwoSide ) 
  {
    if ( facing == 1 ) 
    {
      /* use back color */
      VB->Color   = VB->Bcolor;
      VB->Specular= VB->Bspec;
    }
    else 
    {
      /* use front color */
      VB->Color   = VB->Fcolor;
      VB->Specular= VB->Fspec;
    }
  }

  if ( ctx->Polygon.OffsetAny ) 
  {
    /* Finish computing plane equation of polygon, compute offset */
    GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];  
    GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
    GLfloat a = (ey * fz) - (ez * fy);
    GLfloat b = (ez * fx) - (ex * fz);
    OffsetPolygon( ctx, a, b, c );
  }
  
  if ( ctx->Light.ShadeModel == GL_FLAT )
    dwPVColor = MESA_RGBA_D3D( VB->Color[pv] );

  /*=====================================*/
  /* Populate the the triangle vertices. */
  /*=====================================*/
  TLVertices[0].sx      = (float)VB->Win[v1][0];
  TLVertices[0].sy      = (float)FLIP( VB->Win[v1][1] );
  TLVertices[0].sz      = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
  TLVertices[0].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
  TLVertices[0].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
  TLVertices[0].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
  TLVertices[0].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v1] );

  TLVertices[1].sx      = (float)VB->Win[v2][0];
  TLVertices[1].sy      = (float)FLIP( VB->Win[v2][1] );
  TLVertices[1].sz      = (float)( VB->Win[v2][2] + ctx->PolygonZoffset );
  TLVertices[1].tu      = (float)( VB->TexCoord[v2][0] * pContext->uScale );
  TLVertices[1].tv      = (float)( VB->TexCoord[v2][1] * pContext->vScale );
  TLVertices[1].rhw     = (float)( 1.0 / VB->Clip[v2][3] );
  TLVertices[1].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 
   
  TLVertices[2].sx      = (float)VB->Win[v3][0];
  TLVertices[2].sy      = (float)FLIP( VB->Win[v3][1] );
  TLVertices[2].sz      = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
  TLVertices[2].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
  TLVertices[2].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
  TLVertices[2].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
  TLVertices[2].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 

  TLVertices[5].sx      = (float)VB->Win[v1][0];
  TLVertices[5].sy      = (float)FLIP( VB->Win[v1][1] );
  TLVertices[5].sz      = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
  TLVertices[5].tu      = (float)( VB->TexCoord[v1][0] * pContext->uScale );
  TLVertices[5].tv      = (float)( VB->TexCoord[v1][1] * pContext->vScale );
  TLVertices[5].rhw     = (float)( 1.0 / VB->Clip[v1][3] );
  TLVertices[5].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 

  TLVertices[3].sx      = (float)VB->Win[v3][0];
  TLVertices[3].sy      = (float)FLIP( VB->Win[v3][1] );
  TLVertices[3].sz      = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
  TLVertices[3].tu      = (float)( VB->TexCoord[v3][0] * pContext->uScale );
  TLVertices[3].tv      = (float)( VB->TexCoord[v3][1] * pContext->vScale );
  TLVertices[3].rhw     = (float)( 1.0 / VB->Clip[v3][3] );
  TLVertices[3].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 

  TLVertices[4].sx      = (float)VB->Win[v4][0];
  TLVertices[4].sy      = (float)FLIP( VB->Win[v4][1] );
  TLVertices[4].sz      = (float)( VB->Win[v4][2] + ctx->PolygonZoffset );
  TLVertices[4].tu      = (float)( VB->TexCoord[v4][0] * pContext->uScale );
  TLVertices[4].tv      = (float)( VB->TexCoord[v4][1] * pContext->vScale );
  TLVertices[4].rhw     = (float)( 1.0 / VB->Clip[v4][3] );
  TLVertices[4].color   = (ctx->Light.ShadeModel == GL_FLAT) ? 
                          dwPVColor : MESA_RGBA_D3D( VB->Color[v4] ); 

  /* Draw the two triangles. */  
  pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					D3DFVF_TLVERTEX,
					(LPVOID)&TLVertices[0],
					6, 
					(D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );

  DPF(( DBG_FUNC, "<--d3d    RenderQuad" ));
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void	RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  DWORD                	dwPVColor;
  static D3DTLVERTEX   	TLVertices[3];

  DPF(( DBG_FUNC, "   d3d--> RenderOneTriangle" ));

  /*=====================================*/
  /* Populate the the triangle vertices. */
  /*=====================================*/
  if ( ctx->Light.ShadeModel == GL_FLAT )
    dwPVColor = (VB->Color[pv][3]<<24) | (VB->Color[pv][0]<<16) | (VB->Color[pv][1]<<8) | VB->Color[pv][2];

  TLVertices[0].sx    = (float)VB->Win[v1][0];
  TLVertices[0].sy    = (float)FLIP( VB->Win[v1][1] );
  TLVertices[0].sz    = (float)( VB->Win[v1][2] + ctx->PolygonZoffset );
  TLVertices[0].tu    = (float)( VB->TexCoord[v1][0] * pContext->uScale );
  TLVertices[0].tv    = (float)( VB->TexCoord[v1][1] * pContext->vScale );
  TLVertices[0].rhw   = (float)( 1.0 / VB->Clip[v1][3] );
  TLVertices[0].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                        dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 

  TLVertices[1].sx    = (float)VB->Win[v2][0];
  TLVertices[1].sy    = (float)FLIP( VB->Win[v2][1] );
  TLVertices[1].sz    = (float)( VB->Win[v2][2] + ctx->PolygonZoffset );
  TLVertices[1].tu    = (float)( VB->TexCoord[v2][0] * pContext->uScale );
  TLVertices[1].tv    = (float)( VB->TexCoord[v2][1] * pContext->vScale );
  TLVertices[1].rhw   = (float)( 1.0 / VB->Clip[v2][3] );
  TLVertices[1].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                        dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 

  TLVertices[2].sx    = (float)VB->Win[v3][0];
  TLVertices[2].sy    = (float)FLIP( VB->Win[v3][1] );
  TLVertices[2].sz    = (float)( VB->Win[v3][2] + ctx->PolygonZoffset );
  TLVertices[2].tu    = (float)( VB->TexCoord[v3][0] * pContext->uScale );
  TLVertices[2].tv    = (float)( VB->TexCoord[v3][1] * pContext->vScale );
  TLVertices[2].rhw   = (float)( 1.0 / VB->Clip[v3][3] );
  TLVertices[2].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                        dwPVColor : MESA_RGBA_D3D( VB->Color[v3] ); 

  /* Draw the triangle. */  
  pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					D3DFVF_TLVERTEX,
					(LPVOID)&TLVertices[0],
					3, 
					(D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );

  DPF(( DBG_FUNC, "<--d3d    RenderOneTriangle" ));
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
  PD3DMESACONTEXT     	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  DWORD                	dwPVColor;
  static D3DTLVERTEX	TLVertices[2];

  DPF(( DBG_FUNC, "   d3d--> RenderOneLine" ));

  if ( VB->MonoColor ) 
    dwPVColor = (pContext->aColor<<24) | (pContext->rColor<<16) | (pContext->gColor<<8) | pContext->bColor;
  else 	
    dwPVColor = MESA_RGBA_D3D( VB->Color[pv] );

  TLVertices[0].sx    = (float)VB->Win[v1][0];
  TLVertices[0].sy    = (float)FLIP( VB->Win[v1][1] );
  TLVertices[0].sz    = (float)( VB->Win[v1][2] + ctx->LineZoffset );
  TLVertices[0].tu    = (float)( VB->TexCoord[v1][0] * pContext->uScale );
  TLVertices[0].tv    = (float)( VB->TexCoord[v1][1] * pContext->vScale );
  TLVertices[0].rhw   = (float)( 1.0 / VB->Clip[v1][3] );
  TLVertices[0].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                        dwPVColor : MESA_RGBA_D3D( VB->Color[v1] ); 

  TLVertices[1].sx    = (float)VB->Win[v2][0];
  TLVertices[1].sy    = (float)FLIP( VB->Win[v2][1] );
  TLVertices[1].sz    = (float)( VB->Win[v2][2] + ctx->LineZoffset );
  TLVertices[1].tu    = (float)( VB->TexCoord[v2][0] * pContext->uScale );
  TLVertices[1].tv    = (float)( VB->TexCoord[v2][1] * pContext->vScale );
  TLVertices[1].rhw   = (float)( 1.0 / VB->Clip[v2][3] );
  TLVertices[1].color = (ctx->Light.ShadeModel == GL_FLAT) ? 
                        dwPVColor : MESA_RGBA_D3D( VB->Color[v2] ); 

  /* Draw line from (x0,y0) to (x1,y1) with current pixel color/index */
  pContext->lpD3DDevice->DrawPrimitive( D3DPT_LINELIST,
					D3DFVF_TLVERTEX,
					(LPVOID)&TLVertices[0],
					2, 
					(D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );

  DPF(( DBG_FUNC, "<--d3d    RenderOneLine" ));
}
/*===========================================================================*/
/*  This function was written to convert points into triangles.  I did this  */
/* as all card accelerate triangles and most drivers do this anyway.  In hind*/
/* thought this might be a bad idea as some cards do better.                 */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderPointsVB( GLcontext *ctx, GLuint start, GLuint end )
{
  PD3DMESACONTEXT	pContext = (PD3DMESACONTEXT)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  struct pixel_buffer 	*PB = ctx->PB;
  GLuint 		index;
  GLfloat               radius, z,
                        xmin, ymin, 
                        xmax, ymax;
  GLint			cVertex;
  DWORD			dwPVColor;
  
  DPF(( DBG_FUNC, "   d3d--> RenderPointsVB" ));

  radius = CLAMP( ctx->Point.Size, MIN_POINT_SIZE, MAX_POINT_SIZE ) * 0.5F;

  for( index = start, cVertex = 0; index <= end; index++ ) 
  {
    if ( VB->ClipMask[index] == 0 ) 
    {
      xmin = (float)( VB->Win[index][0] - radius );
      xmax = (float)( VB->Win[index][0] + radius );
      ymin = (float)( pContext->dwHeight - VB->Win[index][1] - radius );
      ymax = (float)( pContext->dwHeight - VB->Win[index][1] + radius );
      z    = (float)( (VB->Win[index][2] + ctx->PointZoffset) );

      dwPVColor = (VB->Color[index][3]<<24) | 
	          (VB->Color[index][0]<<16) | 
	          (VB->Color[index][1]<<8) | 
                  VB->Color[index][2];

      D3DTLVertices[cVertex].sx	     = xmin;
      D3DTLVertices[cVertex].sy      = ymax;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex++].color = dwPVColor;

      D3DTLVertices[cVertex].sx      = xmin;
      D3DTLVertices[cVertex].sy      = ymin;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( 1.0 / VB->Clip[index][3] );
      D3DTLVertices[cVertex++].color = dwPVColor;
   
      D3DTLVertices[cVertex].sx      = xmax;
      D3DTLVertices[cVertex].sy      = ymin;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( (1.0 / VB->Clip[index][3]) );
      D3DTLVertices[cVertex++].color = dwPVColor;
   
      D3DTLVertices[cVertex].sx      = xmax;
      D3DTLVertices[cVertex].sy      = ymin;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( (1.0 / VB->Clip[index][3]) );
      D3DTLVertices[cVertex++].color = dwPVColor;

      D3DTLVertices[cVertex].sx	  = xmax;
      D3DTLVertices[cVertex].sy      = ymax;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( (1.0 / VB->Clip[index][3]) );
      D3DTLVertices[cVertex++].color = dwPVColor;

      D3DTLVertices[cVertex].sx	  = xmin;
      D3DTLVertices[cVertex].sy      = ymax;
      D3DTLVertices[cVertex].sz      = z;
      D3DTLVertices[cVertex].tu      = 0.0;
      D3DTLVertices[cVertex].tv      = 0.0;
      D3DTLVertices[cVertex].rhw     = (float)( (1.0 / VB->Clip[index][3]) );
      D3DTLVertices[cVertex++].color = dwPVColor;
    }
  }

  /* Render the converted vertex buffer. */  
  if ( cVertex )
  {
    pContext->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
					  D3DFVF_TLVERTEX,
					  (LPVOID)&D3DTLVertices[0],
					  cVertex, 
					  (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  }

  DPF(( DBG_FUNC, "<--d3d    RenderPointsVB" ));
}
/*===========================================================================*/
/* Compute Z offsets for a polygon with plane defined by (A,B,C,D)           */
/* D is not needed. TODO: Currently we are calculating this but not using it.*/
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void OffsetPolygon( GLcontext *ctx, GLfloat a, GLfloat b, GLfloat c )
{
  GLfloat ac, 
          bc, 
          m,
          offset;

  DPF(( DBG_FUNC, "   d3d--> OffsetPolygon" ));

  if ( (c < 0.001F) && (c > - 0.001F) ) 
  {
    /* Prevents underflow problems. */
    offset = 0.0F;
  }
  else 
  {
    ac = a / c;
    bc = b / c;
    if ( ac < 0.0F )  
	 ac = -ac;
    if ( bc<0.0F )  
	 bc = -bc;
    m = MAX2( ac, bc );     /* m = sqrt( ac*ac + bc*bc ); */

    offset = (m * ctx->Polygon.OffsetFactor + ctx->Polygon.OffsetUnits);
  }

  ctx->PointZoffset   = ctx->Polygon.OffsetPoint ? offset : 0.0F;
  ctx->LineZoffset    = ctx->Polygon.OffsetLine  ? offset : 0.0F;
  ctx->PolygonZoffset = ctx->Polygon.OffsetFill  ? offset : 0.0F;

  DPF(( DBG_PRIM_INFO, "   d3d    OffsetPolygon: %f", offset ));

  DPF(( DBG_FUNC, "<--d3d    OffsetPolygon" ));
}
/*===========================================================================*/
/*                                                                           */
/*                                                                           */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void SetRenderStates( GLcontext *ctx )
{
  PD3DMESACONTEXT	pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD			dwFunc;
  ULONG			rc;


  /*===============================================*/
  /* Check too see if there are new RASTER states. */
  /*===============================================*/
  pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE,      D3DCULL_NONE );
  pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_DITHERENABLE,  (ctx->Color.DitherFlag) ? TRUE : FALSE );
  pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE , (ctx->Depth.Mask == GL_TRUE) ? TRUE : FALSE );

  /*===========================================*/
  /* Only set TEXTURE STATES if texture is on. */
  /*===========================================*/
  if ( ctx->Texture.Enabled && ctx->Texture.Set[ctx->Texture.CurrentSet].Current2D )
  {
    /* We keep track of the last texture handle. */
    rc = pContext->lpD3DDevice->SetTexture( 0, pContext->lpD3DTexture2 );
    if ( FAILED(rc) )
    {
      DPF(( DBG_TXT_WARN, "   TXT  Failed SetTexture() <%s>", ErrorStringD3D(rc) ));
      pContext->lpD3DDevice->SetTexture( 0, NULL );
    }

    /* Make sure we reset this. */
    pContext->bForceAlpha2One = FALSE;

    /*======================*/
    /* TEXTURE ENVIRONMENT. */
    /*======================*/
    switch( ctx->Texture.Set[ctx->Texture.CurrentSet].EnvMode )
    {
      case GL_MODULATE:
	if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current2D->Image[0]->Format == GL_RGBA )
	{
	  dwFunc = D3DTBLEND_MODULATEALPHA;
	}
	else
	{
	  dwFunc = D3DTBLEND_MODULATE;
	  pContext->bForceAlpha2One = TRUE;
	}
	break;

      case GL_BLEND:
	// TODO: figure out a texture stage...
	dwFunc = D3DTBLEND_DECALALPHA;
	break;
  
      case GL_REPLACE:
	dwFunc = D3DTBLEND_DECAL;
	break;

      case GL_DECAL:
	if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current2D->Image[0]->Format == GL_RGBA )
	  dwFunc = D3DTBLEND_DECALALPHA;
	else
	  dwFunc = D3DTBLEND_DECAL;
	break;
      
      default:
	dwFunc = D3DTBLEND_MODULATE;
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_TEXTUREMAPBLEND, dwFunc );

    /*=============*/
    /* MAG FILTER. */
    /*=============*/
    switch( ctx->Texture.Set[ctx->Texture.CurrentSet].Current2D->MagFilter )
    {
      case GL_NEAREST:
	dwFunc = D3DFILTER_NEAREST;
	break;
      case GL_LINEAR:
	dwFunc = D3DFILTER_LINEAR;
	break;
      case GL_NEAREST_MIPMAP_NEAREST:
	dwFunc = D3DFILTER_MIPNEAREST;
	break;
      case GL_LINEAR_MIPMAP_NEAREST:
	dwFunc = D3DFILTER_LINEARMIPNEAREST;
	break;
      case GL_NEAREST_MIPMAP_LINEAR:
	dwFunc = D3DFILTER_MIPLINEAR;
	break;
      case GL_LINEAR_MIPMAP_LINEAR:
	dwFunc = D3DFILTER_LINEARMIPLINEAR;
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_TEXTUREMAG, dwFunc );

    /*=============*/
    /* MIN FILTER. */
    /*=============*/
    switch( ctx->Texture.Set[ctx->Texture.CurrentSet].Current2D->MinFilter )
    {
      case GL_NEAREST:
	dwFunc = D3DFILTER_NEAREST;
	break;
      case GL_LINEAR:
	dwFunc = D3DFILTER_LINEAR;
	break;
      case GL_NEAREST_MIPMAP_NEAREST:
	dwFunc = D3DFILTER_MIPNEAREST;
	break;
      case GL_LINEAR_MIPMAP_NEAREST:
	dwFunc = D3DFILTER_LINEARMIPNEAREST;
	break;
      case GL_NEAREST_MIPMAP_LINEAR:
	dwFunc = D3DFILTER_MIPLINEAR;
	break;
      case GL_LINEAR_MIPMAP_LINEAR:
	dwFunc = D3DFILTER_LINEARMIPLINEAR;
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_TEXTUREMIN, dwFunc  );
  }
  else
  {
    /* This is how we shut textures off to my knowledge. */
    rc = pContext->lpD3DDevice->SetTexture( 0, NULL );
    if ( FAILED(rc) )
    {
      DPF(( DBG_TXT_WARN, "   TXT  Failed SetTexture() <%s>", ErrorStringD3D(rc) ));
      pContext->lpD3DDevice->SetTexture( 0, NULL );
    }
  }
  
  // TODO: no concept of front & back.
  switch( ctx->Polygon.FrontMode )
  {
    case GL_POINT:
      pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_POINT );
      break;
    case GL_LINE:
      pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME );
      break;
    case GL_FILL:
      pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID );
      break;
  }

  /*==========================*/
  /* Set the Z-Buffer states. */
  /*==========================*/
  if ( ctx->Depth.Test == GL_TRUE )
  {
    switch( ctx->Depth.Func )
    {  
      case GL_NEVER:    
	dwFunc = D3DCMP_NEVER;        
	break;
      case GL_LESS:     
	dwFunc = D3DCMP_LESS;         
	break;
      case GL_GEQUAL:   
	dwFunc = D3DCMP_GREATEREQUAL; 
	break;
      case GL_LEQUAL:   
	dwFunc = D3DCMP_LESSEQUAL;    
	break;
      case GL_GREATER:  
	dwFunc = D3DCMP_GREATER;      
	break;
      case GL_NOTEQUAL: 
	dwFunc = D3DCMP_NOTEQUAL;     
	break;
      case GL_EQUAL:    
	dwFunc = D3DCMP_EQUAL;        
	break;
      case GL_ALWAYS:   
	dwFunc = D3DCMP_ALWAYS;       
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZFUNC, dwFunc );
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, TRUE );
  }	
  else
  {
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, FALSE );
  }

  /*============================*/
  /* Set the Alpha test states. */
  /*============================*/
  if ( ctx->Color.AlphaEnabled == GL_TRUE )
  {
    switch( ctx->Color.AlphaFunc )
    {  
      case GL_NEVER:    
	dwFunc = D3DCMP_NEVER;        
	break;
      case GL_LESS:     
	dwFunc = D3DCMP_LESS;         
	break;
      case GL_GEQUAL:   
	dwFunc = D3DCMP_GREATEREQUAL; 
	break;
      case GL_LEQUAL:   
	dwFunc = D3DCMP_LESSEQUAL;    
	break;
      case GL_GREATER:  
	dwFunc = D3DCMP_GREATER;      
	break;
      case GL_NOTEQUAL: 
	dwFunc = D3DCMP_NOTEQUAL;     
	break;
      case GL_EQUAL:    
	dwFunc = D3DCMP_EQUAL;        
	break;
      case GL_ALWAYS:   
	dwFunc = D3DCMP_ALWAYS;       
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHAFUNC , dwFunc );
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, TRUE );
  }
  else
  {
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, FALSE );
  }

  /*=============================*/
  /* Set the Alpha blend states. */
  /*=============================*/
  if ( ctx->Color.BlendEnabled == GL_TRUE )
  {
    switch( ctx->Color.BlendSrc ) 
    {
      case GL_ZERO:	    			
	dwFunc = D3DBLEND_ZERO;
	break;
      case GL_ONE:                     
	dwFunc = D3DBLEND_ONE;
	break;
      case GL_DST_COLOR:               
	dwFunc = D3DBLEND_DESTCOLOR;
	break;
      case GL_ONE_MINUS_DST_COLOR:     
	dwFunc = D3DBLEND_INVDESTCOLOR;
	break;
      case GL_SRC_ALPHA:               
	dwFunc = D3DBLEND_SRCALPHA;
	break;
      case GL_ONE_MINUS_SRC_ALPHA:     
	dwFunc = D3DBLEND_INVSRCALPHA;
	break;
      case GL_DST_ALPHA:               
	dwFunc = D3DBLEND_DESTALPHA;
	break;
      case GL_ONE_MINUS_DST_ALPHA:     
	dwFunc = D3DBLEND_INVDESTALPHA;
	break;
      case GL_SRC_ALPHA_SATURATE:      
	dwFunc = D3DBLEND_SRCALPHASAT;
	break;
      case GL_CONSTANT_COLOR:          
	dwFunc = D3DBLEND_SRCCOLOR;
	break;
      case GL_ONE_MINUS_CONSTANT_COLOR:
	dwFunc = D3DBLEND_INVSRCCOLOR;
	break;
      case GL_CONSTANT_ALPHA:          
	dwFunc = D3DBLEND_BOTHSRCALPHA;
	break;
      case GL_ONE_MINUS_CONSTANT_ALPHA:
	dwFunc = D3DBLEND_BOTHINVSRCALPHA;
	break;
    }
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, dwFunc );
    
    switch( ctx->Color.BlendDst ) 
    {
      case GL_ZERO:	    			
	dwFunc = D3DBLEND_ZERO;
	break;
      case GL_ONE:                     
	dwFunc = D3DBLEND_ONE;
	break;
      case GL_SRC_COLOR:
	dwFunc = D3DBLEND_SRCCOLOR;
	break;
      case GL_ONE_MINUS_SRC_COLOR:
	dwFunc = D3DBLEND_INVSRCCOLOR;
	break;
      case GL_SRC_ALPHA:               
	dwFunc = D3DBLEND_SRCALPHA;
	break;
      case GL_ONE_MINUS_SRC_ALPHA:     
	dwFunc = D3DBLEND_INVSRCALPHA;
	break;
      case GL_DST_ALPHA:               
	dwFunc = D3DBLEND_DESTALPHA;
	break;
      case GL_ONE_MINUS_DST_ALPHA:     
	dwFunc = D3DBLEND_INVDESTALPHA;
	break;
      case GL_CONSTANT_COLOR:          
	dwFunc = D3DBLEND_SRCCOLOR;
	break;
      case GL_ONE_MINUS_CONSTANT_COLOR:
	dwFunc = D3DBLEND_INVSRCCOLOR;
	break;
      case GL_CONSTANT_ALPHA:          
	dwFunc = D3DBLEND_BOTHSRCALPHA;
	break;
      case GL_ONE_MINUS_CONSTANT_ALPHA:
	dwFunc = D3DBLEND_BOTHINVSRCALPHA;
	break;
    }

    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, dwFunc );
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
  }
  else
  {
    pContext->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
  }

  DPF(( DBG_FUNC, "<--d3d    SetRenderStatesHW" ));
}
