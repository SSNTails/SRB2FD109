#############################################################################
#
# $Id: makefile,v 1.75 2004/04/20 00:34:26 andy $
#     GNU Make makefile for SRB2
#
# Copyright (C) 1998-2000 by DooM Legacy Team.
# Copyright (C) 2003-2004 by Sonic Team Jr.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
#     -DPC_DOS    -> use DOS specific code (eg:textmode stuff)...
#     -DLINUX     -> use for the GNU/Linux specific
#     -D_WINDOWS  -> use for the Win32/DirectX specific
#     -DSDL       -> use for the SDL interface
#
# Sets:
#     Compile the DGJPP/DOS version with 'make WATTCP=1'
#     Compile the DirectX/Mingw version with 'make MINGW=1'
#     Compile the SDL/Mingw version with 'make MINGW=1 SDL=1'
#     Compile the SDL/Linux version with 'make LINUX=1'
#     Compile the SDL/Cygwin version with 'make CYGWIN32=1'
#     Comile the SDL/other version try with 'make SDL=1'
#
# 'Targets':
#     clean
#       Remove all object files
#     cleandep
#       Remove depend.dep
#     dll
#       compile primary HW render DLL/SO
#     all_dll
#       compile all HW render and 3D sound DLLs for the set
#     opengl_dll
#       Pure Mingw only, compile OpenGL HW render DLL
#     minigl_dll
#       Pure Mingw only, compile MiniGL HW render DLL
#     ds3d_dll
#       Pure Mingw only, compile DirectX DirectSound HW sound DLL
#     fmod_dll
#       Pure Mingw only, compile FMOD HW sound DLL
#     openal_dll
#       Pure Mingw only, compile OpenAL HW sound DLL
#     fmod_so
#       Non-Mingw, compile FMOD HW sound SO
#     openal_so
#       Non-Mingw, compile OpenAL HW sound SO
#
#
# Addon:
#     Compile with GCC 2.97 version or lower, add 'GCC29=1'
#     Compile a profile version, add 'PROFILEMODE=1'
#     Compile a debug version, add 'DEBUGMODE=1'
#     Compile with extra warnings, add 'WARNINGMODE=1'
#     Compile without NASM's tmap.nas, add 'NOASM=1'
#     Compile without 3D hardware support, add 'NOHW=1'
#     Compile without 3D sound support, add 'NOHS=1'
#     Compile with GDBstubs, add 'RDB=1'
#
# Addon for SDL:
#     Compile without SDL_Mixer, add 'NOMIXER=1'
#     Compile without BSD API, add 'NONET=1'
#     Compile without IPX/SPX, add 'NOIPX=1'
#     Compile Mingw/SDL with S_DS3S, add 'DS3D=1'
#     Comple with S_FMOD3D, add 'FMOD=1' (WIP)
#     Comple with S_OPENAL, add 'OPENAL=1' (WIP)
#     To link with the whole SDL_Image lib to load Icons, add 'SDL_IMAGE=1' but it isn't not realy needed
#     To link with SDLMain to hide console or make on a console-less binary, add 'SDLMAIN=1'
#
#
#############################################################################

include makefile.cfg


ifdef DJGPPDOS
include djgppdos/makefile.cfg
endif

ifdef LINUX
include sdl/makeNIX.cfg
endif #ifdef LINUX

ifdef MINGW
include win32/makefile.cfg
endif #ifdef MINGW

ifdef DC
include sdl/SRB2DC/makefile.cfg
endif #ifdef DC

ifdef XBOX
include sdl/SRB2XBOX/makefile.cfg
endif #ifdef XBOX

ifdef WINCE
include sdl/SRB2CE/makefile.cfg
endif #ifef WINCE

ifdef CYGWIN32
include sdl/makeCYG.cfg
endif #ifdef CYGWIN32

ifdef SDL
include sdl/makefile.cfg
endif #ifdef SDL

ifndef NONX86
ifndef GCC29
	M5=-march=pentium
	M4=-march=i486
else
	M5=-mpentium
	M4=-m486
endif
endif

ifndef NOASM
ifndef NONX86
	OBJS:= $(OBJDIR)/tmap.o $(OBJDIR)/tmap_mmx.o $(OBJDIR)/vid_copy.o $(OBJS)
	OPTS:= $(OPTS) -DUSEASM
endif
endif

ifdef DEBUGMODE

	# build with debugging information
	WINDRESFLAGS = -D_DEBUG
	CFLAGS = -g -O0 -fno-strict-aliasing $(OPTS) -fno-exceptions -ffloat-store
	ifndef DC
		CFLAGS += $(M5) -DPARANOIA $(WINDRESFLAGS)
	endif
else


	# build a normal optimised version
	WINDRESFLAGS = -DNDEBUG
	CFLAGS = $(OPTS) $(M5) -Os -fno-strict-aliasing $(WINDRESFLAGS) -ffloat-store
		# -fgcse -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop -ffunction-cse \
		# -fexpensive-optimizations -fforce-mem -fstrength-reduce -fthread-jumps  \
		# -fregmove -fschedule-insns -fschedule-insns2 -fmove-all-movables \
		# -fcaller-saves -finline-functions -mieee-fp -mfp-ret-in-387 -m80387 -mhard-float -finline
	LIBS := -s $(LIBS)
endif

ifdef PROFILEMODE
   # build with profiling information
	CFLAGS := -pg $(CFLAGS)
else
	CFLAGS := -fomit-frame-pointer $(CFLAGS)
endif

# not too sophisticated dependency
OBJS := $(i_main_o) \
		$(OBJDIR)/d_main.o \
		$(OBJDIR)/d_clisrv.o \
		$(OBJDIR)/d_net.o    \
		$(OBJDIR)/d_netfil.o \
		$(OBJDIR)/dehacked.o \
		$(OBJDIR)/dstrings.o \
		$(OBJDIR)/z_zone.o   \
		$(OBJDIR)/f_finale.o \
		$(OBJDIR)/f_wipe.o   \
		$(OBJDIR)/g_game.o   \
		$(OBJDIR)/g_input.o  \
		$(OBJDIR)/am_map.o   \
		$(OBJDIR)/command.o  \
		$(OBJDIR)/console.o  \
		$(OBJDIR)/hu_stuff.o \
		$(OBJDIR)/y_inter.o  \
		$(OBJDIR)/st_stuff.o \
		$(OBJDIR)/m_argv.o   \
		$(OBJDIR)/m_bbox.o   \
		$(OBJDIR)/m_cheat.o  \
		$(OBJDIR)/m_fixed.o  \
		$(OBJDIR)/m_menu.o   \
		$(OBJDIR)/m_misc.o   \
		$(OBJDIR)/m_random.o \
		$(OBJDIR)/info.o     \
		$(OBJDIR)/p_ceilng.o \
		$(OBJDIR)/p_enemy.o  \
		$(OBJDIR)/p_fab.o    \
		$(OBJDIR)/p_floor.o  \
		$(OBJDIR)/p_inter.o  \
		$(OBJDIR)/p_lights.o \
		$(OBJDIR)/p_map.o    \
		$(OBJDIR)/p_maputl.o \
		$(OBJDIR)/p_mobj.o   \
		$(OBJDIR)/p_saveg.o  \
		$(OBJDIR)/p_setup.o  \
		$(OBJDIR)/p_sight.o  \
		$(OBJDIR)/p_spec.o   \
		$(OBJDIR)/p_telept.o \
		$(OBJDIR)/p_tick.o   \
		$(OBJDIR)/p_user.o   \
		$(OBJDIR)/tables.o   \
		$(OBJDIR)/r_bsp.o    \
		$(OBJDIR)/r_data.o   \
		$(OBJDIR)/r_draw.o   \
		$(OBJDIR)/r_main.o   \
		$(OBJDIR)/r_plane.o  \
		$(OBJDIR)/r_segs.o   \
		$(OBJDIR)/r_sky.o    \
		$(OBJDIR)/r_splats.o \
		$(OBJDIR)/r_things.o \
		$(OBJDIR)/screen.o   \
		$(OBJDIR)/v_video.o  \
		$(OBJDIR)/s_sound.o  \
		$(OBJDIR)/sounds.o   \
		$(OBJDIR)/md5.o      \
		$(OBJDIR)/w_wad.o    \
		$(OBJDIR)/filesrch.o \
		$(OBJDIR)/mserv.o    \
		$(OBJDIR)/i_tcp.o    \
		$(i_cdmus_o)    \
		$(i_net_o)      \
		$(i_system_o)   \
		$(i_sound_o)    \
		$(OBJS)

ifdef DJGPPDOS
all:	 pre-build $(BIN)/$(EXENAME)
endif

ifdef MINGW
ifndef SDL
all:	 pre-build $(BIN)/$(EXENAME) dll
endif
endif

ifdef SDL
all:	 pre-build $(BIN)/$(EXENAME)
endif

cleandep:
	$(REMOVE) $(OBJDIR)/depend.dep

pre-build:
	$(REMOVE) $(OBJDIR)/d_main.o
	$(REMOVE) $(OBJDIR)/d_netcmd.o

clean:
	$(REMOVE) *~ *.flc
	$(REMOVE) $(OBJDIR)/*.o

ifdef MINGW
	$(REMOVE) $(OBJDIR)/*.res
endif

ifdef CYGWIN32
	$(REMOVE) $(OBJDIR)/*.res
endif

#make a big srb2.s that is the disasm of the exe (dos only ?)
asm:
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -g $(OBJDIR)/i_main.o \
	-o $(OBJDIR)/tmp.exe $(LIBS)
	objdump  -d $(OBJDIR)/tmp.exe --no-show-raw-insn > srb2.s
	$(REMOVE) $(OBJDIR)/tmp.exe

# executable

$(BIN)/$(EXENAME): $(OBJS) $(OBJDIR)/d_netcmd.o
	@mkdir -p $(BIN)
	@echo Linking $(EXENAME)...
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(OBJDIR)/d_netcmd.o \
	-o $(BIN)/$(EXENAME) $(LIBS)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

ifndef SDL
#dll
dll : opengl_dll
ifdef MINGW
all_dll: opengl_dll minigl_dll ds3d_dll fmod_dll openal_dll
opengl_dll : $(OBJDIR)/r_opengl.o $(OBJDIR)/ogl_win.o hardware/r_mingw.def
	@echo Linking R_OpenGL.dll...
	$(DLLWRAP) --def hardware/r_mingw.def $(OBJDIR)/r_opengl.o $(OBJDIR)/ogl_win.o -lopengl32 -lglu32 -lgdi32 -mwindows $(M5) $(CFLAGS) -o $(BIN)/r_opengl.dll

minigl_dll: $(OBJDIR)/r_minigl.o hardware/r_mingw.def
	@echo Linking R_MiniGL.dll...
	$(DLLWRAP) --def hardware/r_mingw.def $(OBJDIR)/r_minigl.o -lopengl32 -lglu32 -lgdi32 -mwindows $(M5) $(CFLAGS) -o $(BIN)/r_minigl.dll

ds3d_dll: $(OBJDIR)/s_ds3d.o hardware/s_mingw.def
	@echo Linking S_DS3d.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_ds3d.o -ldsound -luuid -mwindows $(M5) $(CFLAGS) -o $(BIN)/s_ds3d.dll

fmod_dll: $(OBJDIR)/s_fmod.o hardware/s_mingw.def
	@echo Linking S_FMOD.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_fmod.o -lfmod -mwindows $(M5) $(CFLAGS) -o $(BIN)/s_fmod.dll

openal_dll: $(OBJDIR)/s_openal.o hardware/s_mingw.def
	@echo Linking S_OpenAL.dll...
	$(DLLWRAP) --def hardware/s_mingw.def $(OBJDIR)/s_openal.o -lopenal32 -mwindows $(M5) $(CFLAGS) -o $(BIN)/s_openal.dll
else
all_dll: fmod_so openal_so

fmod_so: $(OBJDIR)/s_fmod.o
	@echo Linking S_FMOD.so...
	$(CC) $(CFLAGS) -Os -o $(BIN)/s_fmod.so -shared -nostartfiles $(OBJDIR)/s_fmod.o -L. -lfmod -lm

openal_so: $(OBJDIR)/s_openal.o
	@echo Linking S_OpenAL.so...
	$(CC) $(CFLAGS) -Os -o $(BIN)/s_openal.so -shared -nostartfiles $(OBJDIR)/s_openal.o -lopenal -lm

endif

ifndef MINGW


$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o -DHW3SOUND -DLINUX -shared -nostartfiles -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o -DHW3SOUND -DLINUX -shared -nostartfiles -c hardware/s_openal/s_openal.c

endif
else
ifdef SDL
ifdef MINGW
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
else
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -I/usr/X11R6/include -c $< -o $@
endif
endif

endif

ifdef MINGW
ifndef SDL
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -DHWRENDER -mwindows -c $< -o $@

$(OBJDIR)/ogl_win.o: hardware/r_opengl/ogl_win.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/ogl_win.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/r_opengl/ogl_win.c

$(OBJDIR)/r_minigl.o: hardware/r_minigl/r_minigl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/r_minigl.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/r_minigl/r_minigl.c

$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -fno-strict-aliasing -o $(OBJDIR)/s_ds3d.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_ds3d/s_ds3d.c

$(OBJDIR)/s_fmod.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_openal/s_openal.c

endif
endif

ifdef SDL
$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

ifdef FILTERS
$(OBJDIR)/%.o: $(INTERFACE)/filter/%.c
	@echo $< needs deps
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/filters.o: $(INTERFACE)/filter/filters.c $(INTERFACE)/filter/filters.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/hq2x.o: $(INTERFACE)/filter/hq2x.c $(INTERFACE)/filter/hq2x.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/lq2x.o: $(INTERFACE)/filter/lq2x.c $(INTERFACE)/filter/lq2x.c \
 $(INTERFACE)/filter/filters.h $(INTERFACE)/filter/interp.h \
 $(INTERFACE)/filter/hq2x.h $(INTERFACE)/filter/lq2x.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif
endif

#dependecy made by gcc itself !
$(OBJS):
-include $(OBJDIR)/depend.dep

$(OBJDIR)/depend.dep:
	@echo "Creating dependency file, depend.dep"
	$(CC) $(CFLAGS) -MM *.c > $(OBJDIR)/depend.ped
	$(CC) $(CFLAGS) -MM $(INTERFACE)/*.c >> $(OBJDIR)/depend.ped
ifndef NOHW
	$(CC) $(CFLAGS) -MM hardware/*.c >> $(OBJDIR)/depend.ped
endif
	@sed -e 's,\(.*\)\.o: ,$(subst /,\/,$(OBJDIR))\/&,g' < $(OBJDIR)/depend.ped > $(OBJDIR)/depend.dep
	$(REMOVE) $(OBJDIR)/depend.ped
	@echo "Created dependency file, depend.dep"

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(INTERFACE)/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: hardware/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.nas
	$(NASM) -o $@ -f $(NASMFORMAT) $<

$(OBJDIR)/vid_copy.o: vid_copy.s asm_defs.inc
	$(CC) $(OPTS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/%.o: %.s
	$(CC) $(OPTS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/SRB2.res: win32/Srb2win.rc win32/afxres.h win32/resource.h
	$(WINDRES) -i $< -O rc $(WINDRESFLAGS) --include-dir=win32 -o $@ -O coff

ifdef DC
$(OBJDIR)/romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d ../data -v

$(OBJDIR)/romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

$(OBJDIR)/dchelp.o: $(INTERFACE)/SRB2DC/dchelp.c
		$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif

#############################################################
#
#############################################################
