# Microsoft Developer Studio Project File - Name="qwsv262" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=qwsv262 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "qwsv262.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "qwsv262.mak" CFG="qwsv262 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "qwsv262 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "qwsv262 - Win32 PR2Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=C:\Program Files\Codeplay\vectorcl.exe
RSC=rc.exe

!IF  "$(CFG)" == "qwsv262 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "qwsv262___Win32_Release"
# PROP BASE Intermediate_Dir "qwsv262___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "svRelease"
# PROP Intermediate_Dir "svRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /Ox /I "include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "SERVERONLY" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib wsock32.lib winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "qwsv262 - Win32 PR2Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "qwsv262___Win32_PR2Release"
# PROP BASE Intermediate_Dir "qwsv262___Win32_PR2Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "PR2Release"
# PROP Intermediate_Dir "PR2Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /GX /Ox /I "include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "SERVERONLY" /D "USE_PR2" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /Ox /I "include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "SERVERONLY" /D "USE_PR2" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib wsock32.lib winmm.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib wsock32.lib winmm.lib /nologo /subsystem:console /machine:I386

!ENDIF 

# Begin Target

# Name "qwsv262 - Win32 Release"
# Name "qwsv262 - Win32 PR2Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\cmd.c
# End Source File
# Begin Source File

SOURCE=.\src\common.c
# SUBTRACT CPP /X
# End Source File
# Begin Source File

SOURCE=.\src\crc.c
# End Source File
# Begin Source File

SOURCE=.\src\cvar.c
# End Source File
# Begin Source File

SOURCE=.\src\hash.c
# End Source File
# Begin Source File

SOURCE=.\src\mathlib.c
# End Source File
# Begin Source File

SOURCE=.\src\md4.c
# End Source File
# Begin Source File

SOURCE=.\src\net_chan.c
# End Source File
# Begin Source File

SOURCE=.\src\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\src\pmove.c
# End Source File
# Begin Source File

SOURCE=.\src\pmovetst.c
# End Source File
# Begin Source File

SOURCE=.\src\pr2_cmds.c
# End Source File
# Begin Source File

SOURCE=.\src\pr2_edict.c
# End Source File
# Begin Source File

SOURCE=.\src\pr2_exec.c
# End Source File
# Begin Source File

SOURCE=.\src\pr2_vm.c
# End Source File
# Begin Source File

SOURCE=.\src\pr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\src\pr_edict.c
# End Source File
# Begin Source File

SOURCE=.\src\pr_exec.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_ccmds.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_demo.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_ents.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_init.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_io.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_io_select.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_model.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_move.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_nchan.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_phys.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_send.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_sys_win.c
# End Source File
# Begin Source File

SOURCE=.\src\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\src\world.c
# End Source File
# Begin Source File

SOURCE=.\src\zone.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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

SOURCE=.\include\cmd.h
# End Source File
# Begin Source File

SOURCE=.\include\common.h
# End Source File
# Begin Source File

SOURCE=.\include\crc.h
# End Source File
# Begin Source File

SOURCE=.\include\cvar.h
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

SOURCE=.\include\wad.h
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
# End Group
# Begin Source File

SOURCE=.\include\client.h
# End Source File
# End Target
# End Project
