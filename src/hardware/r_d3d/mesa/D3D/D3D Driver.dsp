# Microsoft Developer Studio Project File - Name="D3D Driver" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=D3D Driver - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "D3D Driver.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "D3D Driver.mak" CFG="D3D Driver - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "D3D Driver - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "D3D Driver - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "D3D Driver - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\..\bin\VC\Release\r_d3d\Mesa\D3D"
# PROP Intermediate_Dir "..\..\..\..\..\objs\VC\Release\r_d3d\Mesa\D3D"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /G5 /Zp4 /MT /Ot /Og /Oi /Oy /Ob2 /I ".." /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "NDEBUG" /Gs /GM /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\bin\VC\Release\r_d3d\Mesa\D3D\D3D.lib"

!ELSEIF  "$(CFG)" == "D3D Driver - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\..\bin\VC\Debug\r_d3d\Mesa\D3D"
# PROP Intermediate_Dir "..\..\..\..\..\objs\VC\Debug\r_d3d\Mesa\D3D"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /G5 /Zp4 /MTd /Z7 /Od /I ".." /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "_DEBUG" /D "D3D_DEBUG" /GM /Yd /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\bin\VC\Debug\r_d3d\Mesa\D3D\D3D.lib"

!ENDIF 

# Begin Target

# Name "D3D Driver - Win32 Release"
# Name "D3D Driver - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\d3dBitmaps.cpp
DEP_CPP_D3DBI=\
	"..\clip.h"\
	"..\context.h"\
	"..\image.h"\
	"..\light.h"\
	"..\lines.h"\
	"..\macros.h"\
	"..\matrix.h"\
	"..\pb.h"\
	"..\points.h"\
	"..\vb.h"\
	"..\vbrender.h"\
	"..\xform.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\D3DInit.cpp
DEP_CPP_D3DIN=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\D3DRaster.cpp
DEP_CPP_D3DRA=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\D3DSurface.cpp
DEP_CPP_D3DSU=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\D3DTextureMgr.cpp
DEP_CPP_D3DTE=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\D3Dvbrender.cpp
DEP_CPP_D3DVB=\
	"..\clip.h"\
	"..\context.h"\
	"..\light.h"\
	"..\lines.h"\
	"..\macros.h"\
	"..\matrix.h"\
	"..\pb.h"\
	"..\points.h"\
	"..\vb.h"\
	"..\vbrender.h"\
	"..\xform.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\DDrawPROCS.cpp
DEP_CPP_DDRAW=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
DEP_CPP_DEBUG=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\NullProcs.cpp
DEP_CPP_NULLP=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\NULLProcs.h"\
	
# End Source File
# Begin Source File

SOURCE=.\WGL.cpp
DEP_CPP_WGL_C=\
	"..\context.h"\
	"..\matrix.h"\
	"..\vb.h"\
	".\D3DMesa.h"\
	".\D3DSurface.h"\
	".\D3DTextureMgr.h"\
	".\Debug.h"\
	".\NULLProcs.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\D3DMesa.h
# End Source File
# Begin Source File

SOURCE=.\D3DSurface.h
# End Source File
# Begin Source File

SOURCE=.\D3DTextureMgr.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\NULLProcs.h
# End Source File
# End Group
# End Target
# End Project
