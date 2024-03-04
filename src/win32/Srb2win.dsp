# Microsoft Developer Studio Project File - Name="Srb2win" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Srb2win - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "srb2win.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "srb2win.mak" CFG="Srb2win - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Srb2win - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Srb2win - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\objs\Release"
# PROP BASE Intermediate_Dir "..\..\objs\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bin\VC\Release\Win32"
# PROP Intermediate_Dir "..\..\objs\VC\Release\Win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /W3 /GX /Zi /Ot /Og /Oi /Op /Oy /Ob1 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "USEASM" /FD /GF /Gs /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\objs\Release\Srb2win.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 dxguid.lib dinput.lib dsound.lib ddraw.lib uuid.lib winmm.lib wsock32.lib fmodvc.lib gdi32.lib advapi32.lib kernel32.lib user32.lib /nologo /subsystem:windows /pdb:"c:\srb2demo2\srb2.pdb" /debug /machine:I386 /out:"c:\srb2demo2\srb2win.exe"
# SUBTRACT LINK32 /profile /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\objs\Debug"
# PROP BASE Intermediate_Dir "..\..\objs\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\VC\Debug\Win32"
# PROP Intermediate_Dir "..\..\objs\VC\Debug\Win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GX /ZI /Op /Oy /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "USEASM" /FAcs /FR /FD /c
# SUBTRACT CPP /O<none>
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxguid.lib dinput.lib dsound.lib ddraw.lib uuid.lib winmm.lib wsock32.lib fmodvc.lib gdi32.lib advapi32.lib kernel32.lib user32.lib /nologo /subsystem:windows /pdb:"c:\srb2demo2\srb2debug.pdb" /debug /machine:I386 /out:"C:\srb2demo2\srb2debug.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Srb2win - Win32 Release"
# Name "Srb2win - Win32 Debug"
# Begin Group "Win32app"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\afxres.h
# End Source File
# Begin Source File

SOURCE=.\dx_error.c
# End Source File
# Begin Source File

SOURCE=.\dx_error.h
# End Source File
# Begin Source File

SOURCE=.\fabdxlib.c
# End Source File
# Begin Source File

SOURCE=.\fabdxlib.h
# End Source File
# Begin Source File

SOURCE=..\filesrch.c
# End Source File
# Begin Source File

SOURCE=..\filesrch.h
# End Source File
# Begin Source File

SOURCE=.\Mid2strm.c
# End Source File
# Begin Source File

SOURCE=.\Mid2strm.h
# End Source File
# Begin Source File

SOURCE=.\midstuff.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Srb2win.rc

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# ADD BASE RSC /l 0x40c /i "win32"
# ADD RSC /l 0x409 /i "win32"

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win_cd.c
# End Source File
# Begin Source File

SOURCE=.\win_dbg.c
# End Source File
# Begin Source File

SOURCE=.\win_dbg.h
# End Source File
# Begin Source File

SOURCE=.\win_dll.c
# End Source File
# Begin Source File

SOURCE=.\win_dll.h
# End Source File
# Begin Source File

SOURCE=.\win_main.c
# End Source File
# Begin Source File

SOURCE=.\win_main.h
# End Source File
# Begin Source File

SOURCE=.\win_net.c
# End Source File
# Begin Source File

SOURCE=.\win_snd.c
# End Source File
# Begin Source File

SOURCE=.\win_sys.c
# End Source File
# Begin Source File

SOURCE=.\win_vid.c
# End Source File
# End Group
# Begin Group "A_Asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\p5prof.h
# End Source File
# Begin Source File

SOURCE=..\tmap.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap.nas
InputName=tmap

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap.nas
InputName=tmap

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\tmap_mmx.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap_mmx.nas
InputName=tmap_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap_mmx.nas
InputName=tmap_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\tmap_vc.nas

!IF  "$(CFG)" == "Srb2win - Win32 Release"

# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Release\Win32
InputPath=..\tmap_vc.nas
InputName=tmap_vc

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Srb2win - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputName).nas with NASM...
IntDir=.\..\..\objs\VC\Debug\Win32
InputPath=..\tmap_vc.nas
InputName=tmap_vc

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)/$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "D_Doom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\D_clisrv.c
# End Source File
# Begin Source File

SOURCE=..\d_clisrv.h
# End Source File
# Begin Source File

SOURCE=..\D_englsh.h
# End Source File
# Begin Source File

SOURCE=..\D_event.h
# End Source File
# Begin Source File

SOURCE=..\D_main.c
# End Source File
# Begin Source File

SOURCE=..\D_main.h
# End Source File
# Begin Source File

SOURCE=..\D_net.c
# End Source File
# Begin Source File

SOURCE=..\D_net.h
# End Source File
# Begin Source File

SOURCE=..\D_netcmd.c
# End Source File
# Begin Source File

SOURCE=..\D_netcmd.h
# End Source File
# Begin Source File

SOURCE=..\D_netfil.c
# End Source File
# Begin Source File

SOURCE=..\d_netfil.h
# End Source File
# Begin Source File

SOURCE=..\D_player.h
# End Source File
# Begin Source File

SOURCE=..\D_think.h
# End Source File
# Begin Source File

SOURCE=..\D_ticcmd.h
# End Source File
# Begin Source File

SOURCE=..\Dehacked.c
# End Source File
# Begin Source File

SOURCE=..\Dehacked.h
# End Source File
# Begin Source File

SOURCE=..\Doomdata.h
# End Source File
# Begin Source File

SOURCE=..\Doomdef.h
# End Source File
# Begin Source File

SOURCE=..\Doomstat.h
# End Source File
# Begin Source File

SOURCE=..\Doomtype.h
# End Source File
# Begin Source File

SOURCE=..\Dstrings.c
# End Source File
# Begin Source File

SOURCE=..\Dstrings.h
# End Source File
# Begin Source File

SOURCE=..\Z_zone.c
# End Source File
# Begin Source File

SOURCE=..\Z_zone.h
# End Source File
# End Group
# Begin Group "F_Frame"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\F_finale.c
# End Source File
# Begin Source File

SOURCE=..\F_finale.h
# End Source File
# Begin Source File

SOURCE=..\F_wipe.c
# End Source File
# End Group
# Begin Group "G_Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\G_game.c
# End Source File
# Begin Source File

SOURCE=..\G_game.h
# End Source File
# Begin Source File

SOURCE=..\G_input.c
# End Source File
# Begin Source File

SOURCE=..\G_input.h
# End Source File
# Begin Source File

SOURCE=..\g_state.h
# End Source File
# End Group
# Begin Group "H_Hud"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Am_map.c
# End Source File
# Begin Source File

SOURCE=..\Am_map.h
# End Source File
# Begin Source File

SOURCE=..\Command.c
# End Source File
# Begin Source File

SOURCE=..\Command.h
# End Source File
# Begin Source File

SOURCE=..\Console.c
# End Source File
# Begin Source File

SOURCE=..\Console.h
# End Source File
# Begin Source File

SOURCE=..\Hu_stuff.c
# End Source File
# Begin Source File

SOURCE=..\Hu_stuff.h
# End Source File
# Begin Source File

SOURCE=..\St_stuff.c
# End Source File
# Begin Source File

SOURCE=..\St_stuff.h
# End Source File
# Begin Source File

SOURCE=..\y_inter.c
# End Source File
# Begin Source File

SOURCE=..\y_inter.h
# End Source File
# End Group
# Begin Group "Hw_Hardware"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\hardware\hw3dsdrv.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw3sound.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw3sound.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_bsp.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_cache.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_data.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_defs.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_dll.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_draw.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_drv.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_glide.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_glob.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_light.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_light.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_main.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_main.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_md2.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_md2.h
# End Source File
# Begin Source File

SOURCE=..\hardware\hw_trick.c
# End Source File
# Begin Source File

SOURCE=..\hardware\hws_data.h
# End Source File
# End Group
# Begin Group "I_Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\byteptr.h
# End Source File
# Begin Source File

SOURCE=..\i_joy.h
# End Source File
# Begin Source File

SOURCE=..\I_net.h
# End Source File
# Begin Source File

SOURCE=..\I_sound.h
# End Source File
# Begin Source File

SOURCE=..\I_system.h
# End Source File
# Begin Source File

SOURCE=..\i_tcp.c
# End Source File
# Begin Source File

SOURCE=..\i_tcp.h
# End Source File
# Begin Source File

SOURCE=..\I_video.h
# End Source File
# Begin Source File

SOURCE=..\Keys.h
# End Source File
# Begin Source File

SOURCE=..\mserv.c
# End Source File
# Begin Source File

SOURCE=..\mserv.h
# End Source File
# End Group
# Begin Group "M_Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\M_argv.c
# End Source File
# Begin Source File

SOURCE=..\M_argv.h
# End Source File
# Begin Source File

SOURCE=..\M_bbox.c
# End Source File
# Begin Source File

SOURCE=..\M_bbox.h
# End Source File
# Begin Source File

SOURCE=..\M_cheat.c
# End Source File
# Begin Source File

SOURCE=..\M_cheat.h
# End Source File
# Begin Source File

SOURCE=..\M_fixed.c
# End Source File
# Begin Source File

SOURCE=..\M_fixed.h
# End Source File
# Begin Source File

SOURCE=..\M_menu.c
# End Source File
# Begin Source File

SOURCE=..\M_menu.h
# End Source File
# Begin Source File

SOURCE=..\M_misc.c
# End Source File
# Begin Source File

SOURCE=..\M_misc.h
# End Source File
# Begin Source File

SOURCE=..\M_random.c
# End Source File
# Begin Source File

SOURCE=..\M_random.h
# End Source File
# Begin Source File

SOURCE=..\M_swap.h
# End Source File
# End Group
# Begin Group "P_Play"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Info.c
# End Source File
# Begin Source File

SOURCE=..\Info.h
# End Source File
# Begin Source File

SOURCE=..\P_ceilng.c
# End Source File
# Begin Source File

SOURCE=..\P_enemy.c
# End Source File
# Begin Source File

SOURCE=..\P_fab.c
# End Source File
# Begin Source File

SOURCE=..\P_floor.c
# End Source File
# Begin Source File

SOURCE=..\P_inter.c
# End Source File
# Begin Source File

SOURCE=..\P_lights.c
# End Source File
# Begin Source File

SOURCE=..\P_local.h
# End Source File
# Begin Source File

SOURCE=..\P_map.c
# End Source File
# Begin Source File

SOURCE=..\P_maputl.c
# End Source File
# Begin Source File

SOURCE=..\p_maputl.h
# End Source File
# Begin Source File

SOURCE=..\P_mobj.c
# End Source File
# Begin Source File

SOURCE=..\P_mobj.h
# End Source File
# Begin Source File

SOURCE=..\P_pspr.h
# End Source File
# Begin Source File

SOURCE=..\P_saveg.c
# End Source File
# Begin Source File

SOURCE=..\P_saveg.h
# End Source File
# Begin Source File

SOURCE=..\P_setup.c
# End Source File
# Begin Source File

SOURCE=..\P_setup.h
# End Source File
# Begin Source File

SOURCE=..\P_sight.c
# End Source File
# Begin Source File

SOURCE=..\P_spec.c
# End Source File
# Begin Source File

SOURCE=..\P_spec.h
# End Source File
# Begin Source File

SOURCE=..\P_telept.c
# End Source File
# Begin Source File

SOURCE=..\P_tick.c
# End Source File
# Begin Source File

SOURCE=..\P_tick.h
# End Source File
# Begin Source File

SOURCE=..\P_user.c
# End Source File
# Begin Source File

SOURCE=..\Tables.c
# End Source File
# Begin Source File

SOURCE=..\Tables.h
# End Source File
# End Group
# Begin Group "R_Rend"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\R_bsp.c
# End Source File
# Begin Source File

SOURCE=..\R_bsp.h
# End Source File
# Begin Source File

SOURCE=..\R_data.c
# End Source File
# Begin Source File

SOURCE=..\R_data.h
# End Source File
# Begin Source File

SOURCE=..\R_defs.h
# End Source File
# Begin Source File

SOURCE=..\R_draw.c
# End Source File
# Begin Source File

SOURCE=..\R_draw.h
# End Source File
# Begin Source File

SOURCE=..\R_draw16.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\R_draw8.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\R_local.h
# End Source File
# Begin Source File

SOURCE=..\R_main.c
# End Source File
# Begin Source File

SOURCE=..\R_main.h
# End Source File
# Begin Source File

SOURCE=..\R_plane.c
# End Source File
# Begin Source File

SOURCE=..\R_plane.h
# End Source File
# Begin Source File

SOURCE=..\R_segs.c
# End Source File
# Begin Source File

SOURCE=..\R_segs.h
# End Source File
# Begin Source File

SOURCE=..\R_sky.c
# End Source File
# Begin Source File

SOURCE=..\R_sky.h
# End Source File
# Begin Source File

SOURCE=..\r_splats.c
# End Source File
# Begin Source File

SOURCE=..\r_splats.h
# End Source File
# Begin Source File

SOURCE=..\R_state.h
# End Source File
# Begin Source File

SOURCE=..\R_things.c
# End Source File
# Begin Source File

SOURCE=..\R_things.h
# End Source File
# Begin Source File

SOURCE=..\Screen.c
# End Source File
# Begin Source File

SOURCE=..\Screen.h
# End Source File
# Begin Source File

SOURCE=..\V_video.c
# End Source File
# Begin Source File

SOURCE=..\V_video.h
# End Source File
# End Group
# Begin Group "S_Sounds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\S_sound.c
# End Source File
# Begin Source File

SOURCE=..\S_sound.h
# End Source File
# Begin Source File

SOURCE=..\Sounds.c
# End Source File
# Begin Source File

SOURCE=..\Sounds.h
# End Source File
# End Group
# Begin Group "W_Wad"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\md5.c
# End Source File
# Begin Source File

SOURCE=..\md5.h
# End Source File
# Begin Source File

SOURCE=..\W_wad.c
# End Source File
# Begin Source File

SOURCE=..\W_wad.h
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\Copying
# End Source File
# Begin Source File

SOURCE=..\..\doc\Faq.txt
# End Source File
# Begin Source File

SOURCE=..\..\readme.txt
# End Source File
# Begin Source File

SOURCE=..\..\doc\Source.txt
# End Source File
# End Group
# Begin Source File

SOURCE=.\Srb2win.ico
# End Source File
# End Target
# End Project
