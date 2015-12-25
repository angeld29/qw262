# Microsoft Developer Studio Project File - Name="qwcl262" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=qwcl262 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "qwcl262.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "qwcl262.mak" CFG="qwcl262 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "qwcl262 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "qwcl262 - Win32 GLRelease" (based on "Win32 (x86) Application")
!MESSAGE "qwcl262 - Win32 GLDebug" (based on "Win32 (x86) Application")
!MESSAGE "qwcl262 - Win32 GLAuthRelease" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Ox /I "include" /I "pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /J /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comctl32.lib dxguid.lib dinput.lib wsock32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib scitech\lib\win32\vc\mgllt.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "GLRelease"
# PROP BASE Intermediate_Dir "GLRelease"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "GLRelease"
# PROP Intermediate_Dir "GLRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /O2 /I "include" /I "pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /J /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comctl32.lib dxguid.lib dinput.lib glu32.lib opengl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib/zlib.lib lib/libpng.lib lib/libjpeg.lib /nologo /subsystem:windows /machine:I386 /out:"GLRelease/glqwcl262.exe"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "qwcl262___Win32_GLDebug"
# PROP BASE Intermediate_Dir "qwcl262___Win32_GLDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "GLDebug"
# PROP Intermediate_Dir "GLDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /O2 /I "include" /I "pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /J /FD /c
# ADD CPP /nologo /G6 /W3 /Gm /ZI /Od /I "include" /I "pcre" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /J /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "include" /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib dxguid.lib dinput.lib glu32.lib opengl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib/zlib.lib lib/libpng.lib lib/libjpeg.lib /nologo /subsystem:windows /machine:I386 /out:"GLRelease/glqwcl262.exe"
# ADD LINK32 comctl32.lib dxguid.lib dinput.lib glu32.lib opengl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib/zlib.lib lib/libpng.lib lib/libjpeg.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386 /out:"GLDebug/glqwcl262.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "qwcl262___Win32_GLAuthRelease"
# PROP BASE Intermediate_Dir "qwcl262___Win32_GLAuthRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "GLAuthRelease"
# PROP Intermediate_Dir "GLAuthRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /O2 /I "include" /I "pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /J /FD /c
# ADD CPP /nologo /G6 /W3 /O2 /I "include" /I "pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USE_AUTH" /YX /J /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "include" /d "NDEBUG"
# ADD RSC /l 0x409 /i "include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib dxguid.lib dinput.lib glu32.lib opengl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib/zlib.lib lib/libpng.lib lib/libjpeg.lib /nologo /subsystem:windows /machine:I386 /out:"GLRelease/glqwcl262.exe"
# ADD LINK32 comctl32.lib dxguid.lib dinput.lib glu32.lib opengl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib/zlib.lib lib/libpng.lib lib/libjpeg.lib /nologo /subsystem:windows /machine:I386 /out:"GLAuthRelease/glqwcl262.exe"

!ENDIF 

# Begin Target

# Name "qwcl262 - Win32 Release"
# Name "qwcl262 - Win32 GLRelease"
# Name "qwcl262 - Win32 GLDebug"
# Name "qwcl262 - Win32 GLAuthRelease"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "GL Source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gl_common.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_draw.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_flare.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_image.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_mesh.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_model.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_ngraph.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_refrag.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_rlight.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_rmain.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_rmisc.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_rpart.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_rsurf.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_screen.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_test.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_texture.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_vidnt.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "res" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "res" /D "GLQUAKE"
# ADD CPP /I "lib" /I "res" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "res" /D "GLQUAKE"
# ADD CPP /I "lib" /I "res" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_warp.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /D "GLQUAKE"
# ADD CPP /I "lib" /D "GLQUAKE"

!ENDIF 

# End Source File
# End Group
# Begin Group "SOFT Source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cl_model.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "scitech/include"
# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "scitech/include"
# ADD CPP /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_edge.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_fill.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_init.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_modech.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_part.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_polyse.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_scan.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_sky.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_sprite.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_surf.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_vars.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_zpoint.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\draw.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "dxsdk/sdk/inc" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "scitech/include"
# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "scitech/include"
# ADD CPP /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_aclip.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_alias.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_bsp.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_draw.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_edge.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_efrag.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_light.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_main.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_misc.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_part.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_screen.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_sky.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_sprite.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_surf.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_vars.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /I "lib" /I "scitech/include"
# ADD CPP /I "lib" /I "scitech/include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\vid_win.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include" /I "res"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Client Source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cd_win.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_cam.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_demo.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_ents.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_input.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_main.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_mhooks.c
# End Source File
# Begin Source File

SOURCE=.\src\cl_parse.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_pred.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_sys_win.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include" /I "res"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /I "res" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /I "res" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /I "res" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /I "res" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /I "res" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cl_tent.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# End Group
# Begin Group "ASM sources"

# PROP Default_Filter "s"
# Begin Source File

SOURCE=.\src\d_draw.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_draw16.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_parta.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_polysa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_scana.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_spr8.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\d_varsa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\math.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# Begin Custom Build
OutDir=.\GLRelease
InputPath=.\src\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=.\src\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# Begin Custom Build
OutDir=.\GLAuthRelease
InputPath=.\src\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_aclipa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_aliasa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_drawa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_edgea.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\r_varsa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\snd_mixa.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# Begin Custom Build
OutDir=.\GLRelease
InputPath=.\src\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=.\src\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# Begin Custom Build
OutDir=.\GLAuthRelease
InputPath=.\src\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\surf16.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\surf8.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sys_wina.s

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\src\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I include > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# Begin Custom Build
OutDir=.\GLRelease
InputPath=.\src\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=.\src\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# Begin Custom Build
OutDir=.\GLAuthRelease
InputPath=.\src\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /DGLQUAKE /I include> $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm.exe < $(OUTDIR)\$(InputName).spp          >$(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "f_queries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\auth.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\fchecks.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\fmod.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\modules.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\cmd.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\common.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\console.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\crc.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cvar.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\hash.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\in_win.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\keys.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\locfiles.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mathlib.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\md4.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\menu.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\net_chan.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\net_wins.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pcre\pcre.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\pmove.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\pmovetst.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sbar.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\screen.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\skin.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\snd_dma.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\snd_mem.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\snd_mix.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\snd_win.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pcre\study.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\teamplay.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\triggers.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\util.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\view.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\wad.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\winquake.rc
# End Source File
# Begin Source File

SOURCE=.\src\zone.c

!IF  "$(CFG)" == "qwcl262 - Win32 Release"

# ADD CPP /I "scitech/include"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLRelease"

# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLDebug"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ELSEIF  "$(CFG)" == "qwcl262 - Win32 GLAuthRelease"

# ADD BASE CPP /I "lib" /I "scitech/include" /D "GLQUAKE"
# ADD CPP /I "lib" /I "scitech/include" /D "GLQUAKE"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "GL Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\gl_flare.h
# End Source File
# Begin Source File

SOURCE=.\include\gl_image.h
# End Source File
# Begin Source File

SOURCE=.\include\gl_model.h
# End Source File
# Begin Source File

SOURCE=.\include\gl_mtex.h
# End Source File
# Begin Source File

SOURCE=.\include\gl_texture.h
# End Source File
# Begin Source File

SOURCE=.\include\gl_warp_sin.h
# End Source File
# Begin Source File

SOURCE=.\include\glquake.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\include\adivtab.h
# End Source File
# Begin Source File

SOURCE=.\include\anorm_dots.h
# End Source File
# Begin Source File

SOURCE=.\include\anorms.h
# End Source File
# Begin Source File

SOURCE=.\include\asm_draw.h
# End Source File
# Begin Source File

SOURCE=.\include\asm_i386.h
# End Source File
# Begin Source File

SOURCE=.\include\auth.h
# End Source File
# Begin Source File

SOURCE=.\include\block16.h
# End Source File
# Begin Source File

SOURCE=.\include\block8.h
# End Source File
# Begin Source File

SOURCE=.\include\bothdefs.h
# End Source File
# Begin Source File

SOURCE=.\include\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\include\cdaudio.h
# End Source File
# Begin Source File

SOURCE=.\include\cl_sys.h
# End Source File
# Begin Source File

SOURCE=.\include\client.h
# End Source File
# Begin Source File

SOURCE=.\include\cmd.h
# End Source File
# Begin Source File

SOURCE=.\include\common.h
# End Source File
# Begin Source File

SOURCE=.\include\console.h
# End Source File
# Begin Source File

SOURCE=.\include\crc.h
# End Source File
# Begin Source File

SOURCE=.\include\cvar.h
# End Source File
# Begin Source File

SOURCE=.\include\d_iface.h
# End Source File
# Begin Source File

SOURCE=.\include\d_ifacea.h
# End Source File
# Begin Source File

SOURCE=.\include\d_local.h
# End Source File
# Begin Source File

SOURCE=.\include\draw.h
# End Source File
# Begin Source File

SOURCE=.\include\fchecks.h
# End Source File
# Begin Source File

SOURCE=.\include\fmod.h
# End Source File
# Begin Source File

SOURCE=.\include\hash.h
# End Source File
# Begin Source File

SOURCE=.\include\input.h
# End Source File
# Begin Source File

SOURCE=.\include\io.h
# End Source File
# Begin Source File

SOURCE=.\include\keys.h
# End Source File
# Begin Source File

SOURCE=.\include\locfiles.h
# End Source File
# Begin Source File

SOURCE=.\include\mathlib.h
# End Source File
# Begin Source File

SOURCE=.\include\menu.h
# End Source File
# Begin Source File

SOURCE=.\include\model.h
# End Source File
# Begin Source File

SOURCE=.\include\modelgen.h
# End Source File
# Begin Source File

SOURCE=.\include\modules.h
# End Source File
# Begin Source File

SOURCE=.\include\net.h
# End Source File
# Begin Source File

SOURCE=.\include\pmove.h
# End Source File
# Begin Source File

SOURCE=.\include\pr_comp.h
# End Source File
# Begin Source File

SOURCE=.\include\progdefs.h
# End Source File
# Begin Source File

SOURCE=.\include\progs.h
# End Source File
# Begin Source File

SOURCE=.\include\protocol.h
# End Source File
# Begin Source File

SOURCE=.\include\quakeasm.h
# End Source File
# Begin Source File

SOURCE=.\include\quakedef.h
# End Source File
# Begin Source File

SOURCE=.\include\qwsvdef.h
# End Source File
# Begin Source File

SOURCE=.\include\r_local.h
# End Source File
# Begin Source File

SOURCE=.\include\r_shared.h
# End Source File
# Begin Source File

SOURCE=.\include\render.h
# End Source File
# Begin Source File

SOURCE=.\include\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\sbar.h
# End Source File
# Begin Source File

SOURCE=.\include\screen.h
# End Source File
# Begin Source File

SOURCE=.\include\server.h
# End Source File
# Begin Source File

SOURCE=.\include\setup.h.in
# End Source File
# Begin Source File

SOURCE=.\include\sound.h
# End Source File
# Begin Source File

SOURCE=.\include\spritegn.h
# End Source File
# Begin Source File

SOURCE=.\include\sv_sys.h
# End Source File
# Begin Source File

SOURCE=.\include\teamplay.h
# End Source File
# Begin Source File

SOURCE=.\include\triggers.h
# End Source File
# Begin Source File

SOURCE=.\include\util.h
# End Source File
# Begin Source File

SOURCE=.\include\vid.h
# End Source File
# Begin Source File

SOURCE=.\include\view.h
# End Source File
# Begin Source File

SOURCE=.\include\wad.h
# End Source File
# Begin Source File

SOURCE=.\include\winquake.h
# End Source File
# Begin Source File

SOURCE=.\include\world.h
# End Source File
# Begin Source File

SOURCE=.\include\zone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\qwcl2.ico
# End Source File
# End Group
# End Target
# End Project
