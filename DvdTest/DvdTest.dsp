# Microsoft Developer Studio Project File - Name="DvdTest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=DvdTest - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DvdTest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DvdTest.mak" CFG="DvdTest - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DvdTest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Bin"
# PROP BASE Intermediate_Dir "Bin"
# PROP BASE Cmd_Line "NMAKE /f DvdTest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DvdTest.exe"
# PROP BASE Bsc_Name "DvdTest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Bin"
# PROP Intermediate_Dir "Bin"
# PROP Cmd_Line "make.bat"
# PROP Rebuild_Opt "all"
# PROP Target_File "bin/DvdTest.dol"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# Begin Target

# Name "DvdTest - Win32 Release"

!IF  "$(CFG)" == "DvdTest - Win32 Release"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\card.h
# End Source File
# Begin Source File

SOURCE=.\Drivecode.s
# End Source File
# Begin Source File

SOURCE=.\Flash.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Dvd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Dvd.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\xdump.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "GCLib"

# PROP Default_Filter ""
# Begin Group "UnUsed"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\GcLib\GC_AI.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_AI.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_AR.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_AR.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Arch.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_BBA.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_BBA.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Card.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Card.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Context.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Context.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Exception.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Exception.h
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Interrupt.c
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=..\GcLib\GC_Interrupt.h
# PROP Intermediate_Dir "Bin"
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\cache.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\cache.S
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\crt0.s
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Debug.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Debug.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_DebugFont.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_EXI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_EXI.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Font.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Interrupt.c
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Interrupt.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Memory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Memory.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_PAD.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Pad.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Processor.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Random.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_String.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_String.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Time.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Time.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Video.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_Video.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_vsprintf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GC_vsprintf.h
# End Source File
# Begin Source File

SOURCE=..\..\Dev\Source\GcLib\GCLib.h
# End Source File
# End Group
# Begin Group "Make"

# PROP Default_Filter "*.bat"
# Begin Source File

SOURCE=.\make.bat
# PROP Intermediate_Dir "Bin"
# End Source File
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Group
# End Target
# End Project
