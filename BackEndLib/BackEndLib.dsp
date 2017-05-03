# Microsoft Developer Studio Project File - Name="BackEndLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=BackEndLib - Win32 Russian Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BackEndLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BackEndLib.mak" CFG="BackEndLib - Win32 Russian Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BackEndLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "BackEndLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "BackEndLib - Win32 Russian Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BackEndLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "BackEndLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "BackEndLib - Win32 Russian Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "BackEndLib___Win32_Russian_Debug"
# PROP BASE Intermediate_Dir "BackEndLib___Win32_Russian_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Russian_Debug"
# PROP Intermediate_Dir "Russian_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "UNICODE" /D "RUSSIAN_BUILD" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "BackEndLib - Win32 Release"
# Name "BackEndLib - Win32 Debug"
# Name "BackEndLib - Win32 Russian Debug"
# Begin Group "General"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Assert.cpp
# End Source File
# Begin Source File

SOURCE=.\Assert.h
# End Source File
# Begin Source File

SOURCE=.\AttachableObject.cpp
# End Source File
# Begin Source File

SOURCE=.\AttachableObject.h
# End Source File
# Begin Source File

SOURCE=.\Base64.cpp
# End Source File
# Begin Source File

SOURCE=.\Base64.h
# End Source File
# Begin Source File

SOURCE=.\Browser.cpp
# End Source File
# Begin Source File

SOURCE=.\Browser.h
# End Source File
# Begin Source File

SOURCE=.\CharTraits.h
# End Source File
# Begin Source File

SOURCE=.\Clipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\Clipboard.h
# End Source File
# Begin Source File

SOURCE=.\Date.cpp
# End Source File
# Begin Source File

SOURCE=.\Date.h
# End Source File
# Begin Source File

SOURCE=.\Dyn.cpp
# End Source File
# Begin Source File

SOURCE=.\Dyn.h
# End Source File
# Begin Source File

SOURCE=.\Files.cpp
# End Source File
# Begin Source File

SOURCE=.\Files.h
# End Source File
# Begin Source File

SOURCE=.\GameStream.cpp
# End Source File
# Begin Source File

SOURCE=.\GameStream.h
# End Source File
# Begin Source File

SOURCE=.\Heap.cpp
# End Source File
# Begin Source File

SOURCE=.\Heap.h
# End Source File
# Begin Source File

SOURCE=.\IniFile.cpp
# End Source File
# Begin Source File

SOURCE=.\IniFile.h
# End Source File
# Begin Source File

SOURCE=.\Internet.cpp
# End Source File
# Begin Source File

SOURCE=.\Internet.h
# End Source File
# Begin Source File

SOURCE=.\MessageIDs.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageIDs.h
# End Source File
# Begin Source File

SOURCE=.\Metadata.cpp
# End Source File
# Begin Source File

SOURCE=.\Metadata.hpp
# End Source File
# Begin Source File

SOURCE=.\Ports.cpp
# End Source File
# Begin Source File

SOURCE=.\Ports.h
# End Source File
# Begin Source File

SOURCE=.\PostData.cpp
# End Source File
# Begin Source File

SOURCE=.\PostData.h
# End Source File
# Begin Source File

SOURCE=.\StretchyBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\StretchyBuffer.h
# End Source File
# Begin Source File

SOURCE=.\SysTimer.cpp
# End Source File
# Begin Source File

SOURCE=.\SysTimer.h
# End Source File
# Begin Source File

SOURCE=.\Types.h
# End Source File
# Begin Source File

SOURCE=.\UtilFuncs.h
# End Source File
# Begin Source File

SOURCE=.\Wchar.cpp
# End Source File
# Begin Source File

SOURCE=.\Wchar.h
# End Source File
# End Group
# Begin Group "Data Access"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Coord.cpp
# End Source File
# Begin Source File

SOURCE=.\Coord.h
# End Source File
# Begin Source File

SOURCE=.\CoordIndex.h
# End Source File
# Begin Source File

SOURCE=.\CoordSet.h
# End Source File
# Begin Source File

SOURCE=.\CoordStack.h
# End Source File
# Begin Source File

SOURCE=.\Exception.h
# End Source File
# Begin Source File

SOURCE=.\IDList.cpp
# End Source File
# Begin Source File

SOURCE=.\IDList.h
# End Source File
# Begin Source File

SOURCE=.\IDSet.h
# End Source File
# End Group
# End Target
# End Project
