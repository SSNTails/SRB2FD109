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
#include "Debug.h"
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
DWORD		g_DBGMask = DBG_ALL_ERROR,
                g_dwProfTime;
LARGE_INTEGER 	g_ProfStart,
		g_ProfFinish;
/*===========================================================================*/
/*  This is your basic DPF function with printf like support.  The function  */
/* also works with a global debug mask variable.  I have written support that*/
/* allows for the user's enviroment variable space to be read and set the    */
/* masks.  This is done when the dll starts and is only in the debug version.*/
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void _cdecl DebugPrint( int mask, char *pszFormat, ... )
{
  char		buffer[8192];
  va_list	args;
  FILE		*fileptr;

  /* A mask of 0 will always pass. Easy to remeber. */
  if ( (mask == 0) || (mask & g_DBGMask) ) 
  {
    va_start( args, pszFormat );

    OutputDebugString( "MesaD3D: " ); 

    vsprintf( buffer, pszFormat, args );
    strcat( buffer, "\n" );
    OutputDebugString( buffer );

    /* Break on all function entrance and exit. */
    //    if ( mask & DBG_FUNC )
    //    {
    //      DEBUG_BREAK;
    //    }

    if ( g_DBGMask & DBG_LOGFILE )
    {
      fileptr = fopen( "ogl2D3D.log", "a" );
      if ( fileptr )
      {	
	fprintf( fileptr,"%s", buffer );
	fclose( fileptr );
      }
    }

    va_end( args );
  }

  /* Shutdown on all errors. */
  if ( mask & DBG_ALL_ERROR )
  {
    //DEBUG_BREAK;

    /* Whip this baby in to try and support the API until we die... */
    MakeCurrent( g_pD3DDefault );

    /* Close the application down. */
    SendMessage( WindowFromDC(g_pD3DCurrent->hdc), WM_QUIT, 0, 0 );
  }
 
}
/*===========================================================================*/
/*  This call reads the users enviroment variables and sets any debug mask   */
/* that they have set to TRUE.  Now the value must be "TRUE".                */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void ReadDBGEnv( void )
{
  g_DBGMask = 0;

  if ( VAR_SET(DBG_FUNC) )		g_DBGMask |= DBG_FUNC;
  if ( VAR_SET(DBG_STATES) ) 		g_DBGMask |= DBG_STATES;
  if ( VAR_SET(DBG_LOGFILE) )		g_DBGMask |= DBG_LOGFILE;

  if ( VAR_SET(DBG_CNTX_INFO) )		g_DBGMask |= DBG_CNTX_INFO;
  if ( VAR_SET(DBG_CNTX_WARN) )		g_DBGMask |= DBG_CNTX_WARN;
  if ( VAR_SET(DBG_CNTX_PROFILE) )	g_DBGMask |= DBG_CNTX_PROFILE;
  if ( VAR_SET(DBG_CNTX_ERROR) )	g_DBGMask |= DBG_CNTX_ERROR;
  if ( VAR_SET(DBG_CNTX_ALL) )		g_DBGMask |= DBG_CNTX_ALL;

  if ( VAR_SET(DBG_PRIM_INFO) )		g_DBGMask |= DBG_PRIM_INFO;
  if ( VAR_SET(DBG_PRIM_WARN) )		g_DBGMask |= DBG_PRIM_WARN;
  if ( VAR_SET(DBG_PRIM_PROFILE) )	g_DBGMask |= DBG_PRIM_PROFILE;
  if ( VAR_SET(DBG_PRIM_ERROR) )	g_DBGMask |= DBG_PRIM_ERROR;
  if ( VAR_SET(DBG_PRIM_ALL) )		g_DBGMask |= DBG_PRIM_ALL;

  if ( VAR_SET(DBG_TXT_INFO) )		g_DBGMask |= DBG_TXT_INFO;
  if ( VAR_SET(DBG_TXT_WARN) )		g_DBGMask |= DBG_TXT_WARN;
  if ( VAR_SET(DBG_TXT_PROFILE) )	g_DBGMask |= DBG_TXT_PROFILE;
  if ( VAR_SET(DBG_TXT_ERROR) )		g_DBGMask |= DBG_TXT_ERROR;
  if ( VAR_SET(DBG_TXT_ALL) )		g_DBGMask |= DBG_TXT_ALL;

  if ( VAR_SET(DBG_ALL_INFO) )		g_DBGMask |= DBG_ALL_INFO;
  if ( VAR_SET(DBG_ALL_WARN) )		g_DBGMask |= DBG_ALL_WARN;
  if ( VAR_SET(DBG_ALL_PROFILE) )	g_DBGMask |= DBG_ALL_PROFILE;
  if ( VAR_SET(DBG_ALL_ERROR) )		g_DBGMask |= DBG_ALL_ERROR;
  if ( VAR_SET(DBG_ALL) )		g_DBGMask |= DBG_ALL;
}
/*===========================================================================*/
/*  This function will take a pointer to a DDSURFACEDESC2 structure & display*/
/* the parsed information using a DPF call.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void	DebugPixelFormat( char *pszSurfaceName, DDPIXELFORMAT *pddpf )
{
  char	buffer[256];

  /* Parse the flag type and write the string equivalent. */
  if ( pddpf->dwFlags & DDPF_ALPHA )
	strcat( buffer, "DDPF_ALPHA " );
  if ( pddpf->dwFlags & DDPF_ALPHAPIXELS )
	strcat( buffer, "DDPF_ALPHAPIXELS " );
  if ( pddpf->dwFlags & DDPF_ALPHAPREMULT )
	strcat( buffer, "DDPF_ALPHAPREMULT " );
  if ( pddpf->dwFlags & DDPF_BUMPLUMINANCE )
	strcat( buffer, "DDPF_BUMPLUMINANCE " );
  if ( pddpf->dwFlags & DDPF_BUMPDUDV )
	strcat( buffer, "DDPF_BUMPDUDV " );
  if ( pddpf->dwFlags & DDPF_COMPRESSED )
	strcat( buffer, "DDPF_COMPRESSED " );
  if ( pddpf->dwFlags & DDPF_FOURCC )
	strcat( buffer, "DDPF_FOURCC " );
  if ( pddpf->dwFlags & DDPF_LUMINANCE )
	strcat( buffer, "DDPF_LUMINANCE " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED1 )
	strcat( buffer, "DDPF_PALETTEINDEXED1 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED2 )
	strcat( buffer, "DDPF_PALETTEINDEXED2 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED4 )
	strcat( buffer, "DDPF_PALETTEINDEXED4 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED8 )
	strcat( buffer, "DDPF_PALETTEINDEXED8 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXEDTO8 )
	strcat( buffer, "DDPF_PALETTEINDEXEDTO8 " );
  if ( pddpf->dwFlags & DDPF_RGB )
	strcat( buffer, "DDPF_RGB  " );
  if ( pddpf->dwFlags & DDPF_RGBTOYUV )
	strcat( buffer, "DDPF_RGBTOYUV  " );
  if ( pddpf->dwFlags & DDPF_STENCILBUFFER )
	strcat( buffer, "DDPF_STENCILBUFFER  " );
  if ( pddpf->dwFlags & DDPF_YUV )
	strcat( buffer, "DDPF_YUV  " );
  if ( pddpf->dwFlags & DDPF_ZBUFFER )
	strcat( buffer, "DDPF_ZBUFFER  " );
  if ( pddpf->dwFlags & DDPF_ZPIXELS )
	strcat( buffer, "DDPF_ZPIXELS  " );

  DPF(( (DBG_TXT_INFO|DBG_CNTX_INFO),"%s", buffer ));
}





