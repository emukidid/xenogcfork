# Microsoft Developer Studio Project File - Name="XenoFlash" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=XenoFlash - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XenoFlash.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XenoFlash.mak" CFG="XenoFlash - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XenoFlash - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "XenoFlash - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "XenoFlash - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f XenoFlash.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XenoFlash.exe"
# PROP BASE Bsc_Name "XenoFlash.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "makeme.bat"
# PROP Rebuild_Opt " clean"
# PROP Target_File "XenoFlash.dol"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "XenoFlash - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Bin"
# PROP BASE Intermediate_Dir "Bin"
# PROP BASE Cmd_Line "NMAKE /f XenoFlash.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XenoFlash.exe"
# PROP BASE Bsc_Name "XenoFlash.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Bin"
# PROP Intermediate_Dir "Bin"
# PROP Cmd_Line "makeme.bat"
# PROP Rebuild_Opt "clean"
# PROP Target_File "xenoflash.dol"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "XenoFlash - Win32 Release"
# Name "XenoFlash - Win32 Debug"

!IF  "$(CFG)" == "XenoFlash - Win32 Release"

!ELSEIF  "$(CFG)" == "XenoFlash - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\flashloader.s
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\xdump.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Make"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\first.lds
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Makefile
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\makeme.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# Begin Group "GCLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_AI.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_AR.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_BBA.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Card.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Context.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Debug.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Dvd.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Exception.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_EXI.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Interrupt.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Memory.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_PAD.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Random.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_String.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Time.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Video.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_vsprintf.cpp
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# End Target
# End Project
