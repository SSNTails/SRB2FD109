# Microsoft Developer Studio Project File - Name="r_d3d" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=r_d3d - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "r_d3d.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "r_d3d.mak" CFG="r_d3d - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "r_d3d - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "r_d3d - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "r_d3d - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "r_d3d___Win32_Release"
# PROP BASE Intermediate_Dir "r_d3d___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bin\VC\Release\r_d3d"
# PROP Intermediate_Dir "..\..\..\objs\VC\Release\r_d3d"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "R_OPENGL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /Zi /O2 /I "mesa" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x80c /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\..\..\bin\VC\Release\r_d3d\Mesa\D3D\D3D.lib ..\..\..\bin\VC\Release\r_d3d\Mesa\MESA_core.lib ddraw.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib glu32.lib dxguid.lib /nologo /dll /pdb:"..\..\..\bin\VC\Release\r_d3d.pdb" /machine:I386 /out:"..\..\..\bin\VC\Release\r_d3d.dll"
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "r_d3d - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\..\objs\Debug"
# PROP BASE Intermediate_Dir "..\..\..\objs\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bin\VC\Debug\r_d3d"
# PROP Intermediate_Dir "..\..\..\objs\VC\Debug\r_d3d"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "R_OPENGL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "mesa" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "_MBCS" /D "_USRDLL" /D "R_OPENGL_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x80c /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\bin\VC\Debug\r_d3d\Mesa\D3D\D3D.lib ..\..\..\bin\VC\Debug\r_d3d\Mesa\MESA_core.lib ddraw.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib glu32.lib dxguid.lib /nologo /dll /pdb:"..\..\..\bin\VC\Debug\r_d3d.pdb" /debug /machine:I386 /out:"..\..\..\bin\VC\Debug\r_d3d.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "r_d3d - Win32 Release"
# Name "r_d3d - Win32 Debug"
# Begin Source File

SOURCE=.\d3d_win.c
# End Source File
# Begin Source File

SOURCE=.\r_d3d.c
# End Source File
# Begin Source File

SOURCE=.\r_d3d.h
# End Source File
# End Target
# End Project
