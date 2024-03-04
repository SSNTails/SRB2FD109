# Microsoft Developer Studio Project File - Name="MESA_core" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=MESA_core - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MESA_core.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MESA_core.mak" CFG="MESA_core - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MESA_core - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MESA_core - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MESA_core - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin\VC\Release\r_d3d\Mesa"
# PROP Intermediate_Dir "..\..\..\..\objs\VC\Release\r_d3d\Mesa"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /G5 /Zp4 /MT /Ot /Og /Oi /Oy /Ob2 /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "FAST_MATH" /D "NDEBUG" /GM /TP /Gs /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "MESA_core - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin\VC\Debug\r_d3d\Mesa"
# PROP Intermediate_Dir "..\..\..\..\objs\VC\Debug\r_d3d\Mesa"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /G5 /Zp4 /MTd /Z7 /Od /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "__MSC__" /D "MESAD3D" /D "NOCRYPT" /D "FAST_MATH" /D "_DEBUG" /D "D3D_DEBUG" /GM /TP /Yd /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /DEBUGTYPE:CV /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "MESA_core - Win32 Release"
# Name "MESA_core - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accum.c
DEP_CPP_ACCUM=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ACCUM=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\alpha.c
DEP_CPP_ALPHA=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ALPHA=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\alphabuf.c
DEP_CPP_ALPHAB=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ALPHAB=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\api1.c
DEP_CPP_API1_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_API1_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\api2.c
DEP_CPP_API2_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_API2_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\apiext.c
DEP_CPP_APIEX=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_APIEX=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\asm_mmx.c
# End Source File
# Begin Source File

SOURCE=.\attrib.c
DEP_CPP_ATTRI=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ATTRI=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bitmap.c
DEP_CPP_BITMA=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_BITMA=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\blend.c
DEP_CPP_BLEND=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\asm_mmx.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_BLEND=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\clip.c
DEP_CPP_CLIP_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_CLIP_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\colortab.c
DEP_CPP_COLOR=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_COLOR=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\context.c
DEP_CPP_CONTE=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_CONTE=\
	".\asm-386.h"\
	".\mthreads.h"\
	
# End Source File
# Begin Source File

SOURCE=.\copypix.c
DEP_CPP_COPYP=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_COPYP=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\depth.c
DEP_CPP_DEPTH=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_DEPTH=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\dlist.c
DEP_CPP_DLIST=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_DLIST=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\drawpix.c
DEP_CPP_DRAWP=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_DRAWP=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\enable.c
DEP_CPP_ENABL=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ENABL=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\eval.c
DEP_CPP_EVAL_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_EVAL_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\feedback.c
DEP_CPP_FEEDB=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_FEEDB=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\fog.c
DEP_CPP_FOG_C=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_FOG_C=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\get.c
DEP_CPP_GET_C=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_GET_C=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\hash.c
DEP_CPP_HASH_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_HASH_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\image.c
DEP_CPP_IMAGE=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_IMAGE=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\light.c
DEP_CPP_LIGHT=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_LIGHT=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\lines.c
DEP_CPP_LINES=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\linetemp.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_LINES=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\logic.c
DEP_CPP_LOGIC=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_LOGIC=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\masking.c
DEP_CPP_MASKI=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_MASKI=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\matrix.c
DEP_CPP_MATRI=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_MATRI=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\misc.c
DEP_CPP_MISC_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_MISC_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\mmath.c
DEP_CPP_MMATH=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_MMATH=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\pb.c
DEP_CPP_PB_C3c=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_PB_C3c=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\pixel.c
DEP_CPP_PIXEL=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_PIXEL=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\pointers.c
DEP_CPP_POINT=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_POINT=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\points.c
DEP_CPP_POINTS=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_POINTS=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\polygon.c
DEP_CPP_POLYG=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_POLYG=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\quads.c
DEP_CPP_QUADS=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_QUADS=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\rastpos.c
DEP_CPP_RASTP=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_RASTP=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\readpix.c
DEP_CPP_READP=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_READP=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\rect.c
DEP_CPP_RECT_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_RECT_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\scissor.c
DEP_CPP_SCISS=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_SCISS=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\shade.c
DEP_CPP_SHADE=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_SHADE=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\span.c
DEP_CPP_SPAN_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_SPAN_=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\stencil.c
DEP_CPP_STENC=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_STENC=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\teximage.c
DEP_CPP_TEXIM=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_TEXIM=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\texobj.c
DEP_CPP_TEXOB=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_TEXOB=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\texstate.c
DEP_CPP_TEXST=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_TEXST=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\texture.c
DEP_CPP_TEXTU=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_TEXTU=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\triangle.c
DEP_CPP_TRIAN=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\tritemp.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_TRIAN=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\varray.c
DEP_CPP_VARRA=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_VARRA=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\vb.c
DEP_CPP_VB_C62=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_VB_C62=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\vbfill.c
DEP_CPP_VBFIL=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_VBFIL=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\vbrender.c
DEP_CPP_VBREN=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_VBREN=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\vbxform.c
DEP_CPP_VBXFO=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\asm_386.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_VBXFO=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\winpos.c
DEP_CPP_WINPO=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_WINPO=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\xform.c
DEP_CPP_XFORM=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_XFORM=\
	".\asm-386.h"\
	
# End Source File
# Begin Source File

SOURCE=.\zoom.c
DEP_CPP_ZOOM_=\
	".\accum.h"\
	".\all.h"\
	".\alpha.h"\
	".\alphabuf.h"\
	".\api.h"\
	".\attrib.h"\
	".\bitmap.h"\
	".\blend.h"\
	".\clip.h"\
	".\colortab.h"\
	".\CONFIG.H"\
	".\context.h"\
	".\copypix.h"\
	".\dd.h"\
	".\depth.h"\
	".\dlist.h"\
	".\drawpix.h"\
	".\enable.h"\
	".\eval.h"\
	".\feedback.h"\
	".\fixed.h"\
	".\fog.h"\
	".\get.h"\
	".\GL\gl.h"\
	".\GL\gl_mangle.h"\
	".\GL\osmesa.h"\
	".\hash.h"\
	".\image.h"\
	".\light.h"\
	".\lines.h"\
	".\logic.h"\
	".\macros.h"\
	".\masking.h"\
	".\matrix.h"\
	".\misc.h"\
	".\mmath.h"\
	".\pb.h"\
	".\pixel.h"\
	".\pointers.h"\
	".\points.h"\
	".\polygon.h"\
	".\quads.h"\
	".\rastpos.h"\
	".\readpix.h"\
	".\rect.h"\
	".\scissor.h"\
	".\shade.h"\
	".\span.h"\
	".\stencil.h"\
	".\teximage.h"\
	".\texobj.h"\
	".\texstate.h"\
	".\texture.h"\
	".\triangle.h"\
	".\types.h"\
	".\varray.h"\
	".\vb.h"\
	".\vbfill.h"\
	".\vbrender.h"\
	".\vbxform.h"\
	".\winpos.h"\
	".\xform.h"\
	".\zoom.h"\
	
NODEP_CPP_ZOOM_=\
	".\asm-386.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accum.h
# End Source File
# Begin Source File

SOURCE=.\all.h
# End Source File
# Begin Source File

SOURCE=.\alpha.h
# End Source File
# Begin Source File

SOURCE=.\alphabuf.h
# End Source File
# Begin Source File

SOURCE=.\asm_386.h
# End Source File
# Begin Source File

SOURCE=.\asm_mmx.h
# End Source File
# Begin Source File

SOURCE=.\attrib.h
# End Source File
# Begin Source File

SOURCE=.\bitmap.h
# End Source File
# Begin Source File

SOURCE=.\blend.h
# End Source File
# Begin Source File

SOURCE=.\clip.h
# End Source File
# Begin Source File

SOURCE=.\colortab.h
# End Source File
# Begin Source File

SOURCE=.\CONFIG.H
# End Source File
# Begin Source File

SOURCE=.\context.h
# End Source File
# Begin Source File

SOURCE=.\copypix.h
# End Source File
# Begin Source File

SOURCE=.\depth.h
# End Source File
# Begin Source File

SOURCE=.\dlist.h
# End Source File
# Begin Source File

SOURCE=.\drawpix.h
# End Source File
# Begin Source File

SOURCE=.\enable.h
# End Source File
# Begin Source File

SOURCE=.\eval.h
# End Source File
# Begin Source File

SOURCE=.\feedback.h
# End Source File
# Begin Source File

SOURCE=.\fixed.h
# End Source File
# Begin Source File

SOURCE=.\fog.h
# End Source File
# Begin Source File

SOURCE=.\get.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\light.h
# End Source File
# Begin Source File

SOURCE=.\lines.h
# End Source File
# Begin Source File

SOURCE=.\linetemp.h
# End Source File
# Begin Source File

SOURCE=.\logic.h
# End Source File
# Begin Source File

SOURCE=.\masking.h
# End Source File
# Begin Source File

SOURCE=.\matrix.h
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\mmath.h
# End Source File
# Begin Source File

SOURCE=.\pb.h
# End Source File
# Begin Source File

SOURCE=.\pixel.h
# End Source File
# Begin Source File

SOURCE=.\pointers.h
# End Source File
# Begin Source File

SOURCE=.\points.h
# End Source File
# Begin Source File

SOURCE=.\polygon.h
# End Source File
# Begin Source File

SOURCE=.\quads.h
# End Source File
# Begin Source File

SOURCE=.\rastpos.h
# End Source File
# Begin Source File

SOURCE=.\readpix.h
# End Source File
# Begin Source File

SOURCE=.\rect.h
# End Source File
# Begin Source File

SOURCE=.\scissor.h
# End Source File
# Begin Source File

SOURCE=.\shade.h
# End Source File
# Begin Source File

SOURCE=.\span.h
# End Source File
# Begin Source File

SOURCE=.\stencil.h
# End Source File
# Begin Source File

SOURCE=.\teximage.h
# End Source File
# Begin Source File

SOURCE=.\texobj.h
# End Source File
# Begin Source File

SOURCE=.\texstate.h
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\triangle.h
# End Source File
# Begin Source File

SOURCE=.\tritemp.h
# End Source File
# Begin Source File

SOURCE=.\varray.h
# End Source File
# Begin Source File

SOURCE=.\vb.h
# End Source File
# Begin Source File

SOURCE=.\vbfill.h
# End Source File
# Begin Source File

SOURCE=.\vbrender.h
# End Source File
# Begin Source File

SOURCE=.\vbxform.h
# End Source File
# Begin Source File

SOURCE=.\winpos.h
# End Source File
# Begin Source File

SOURCE=.\zoom.h
# End Source File
# End Group
# End Target
# End Project
