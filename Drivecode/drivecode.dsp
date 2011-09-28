# Microsoft Developer Studio Project File - Name="Gc_DVDDrivecode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Gc_DVDDrivecode - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "drivecode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "drivecode.mak" CFG="Gc_DVDDrivecode - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Gc_DVDDrivecode - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Gc_DVDDrivecode.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Gc_DVDDrivecode.exe"
# PROP BASE Bsc_Name "Gc_DVDDrivecode.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "D:\Console\GameCube\XenoGC\drivecode\make.bat"
# PROP Rebuild_Opt ""
# PROP Target_File "drvcode.bin"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# Begin Target

# Name "Gc_DVDDrivecode - Win32 Release"

!IF  "$(CFG)" == "Gc_DVDDrivecode - Win32 Release"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\mn10200.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\PPC.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\QLite.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\QliteGlobal.h
# End Source File
# End Group
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\QLiteApploader.asm.c
# End Source File
# Begin Source File

SOURCE=.\Source\QLiteDbg.asm.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\QLiteIPLDbg.asm.c
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# Begin Source File

SOURCE=".\Source\QLite Main.asm.c"
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\QLiteBreakpoints.asm.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\QLiteIPL.asm.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Source\xdump.asm.c
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# Begin Group "Make"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Group "disasm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bin\disasm.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\bin\Disasm\disasm1.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\bin\Disasm\disasm1.cmd
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\bin\Disasm\disasm2.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\bin\Disasm\disasm2.cmd
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\bin\Disasm\disasm3.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# Begin Source File

SOURCE=.\make.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Makefile
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\sim.ld
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# End Target
# End Project
