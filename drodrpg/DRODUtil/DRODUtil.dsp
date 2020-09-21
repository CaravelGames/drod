# Microsoft Developer Studio Project File - Name="DRODUtil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=DRODUtil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DRODUtil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DRODUtil.mak" CFG="DRODUtil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DRODUtil - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "DRODUtil - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DRODUtil - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\\" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "CARAVELBUILD" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 mk4vc60s.lib gdi32.lib libjpeg.lib libpng.lib user32.lib shell32.lib fmodvc.lib sdl.lib sdlmain.lib SDL_ttf.lib libexpat.lib libcurl.lib ws2_32.lib theora_static.lib ogg_static.lib vorbis_static.lib zlib.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "DRODUtil - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\\" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "CARAVELBUILD" /FD /GZ /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mk4vc60s_d.lib gdi32.lib libjpegd.lib libpng.lib user32.lib shell32.lib fmodvc.lib sdl.lib sdlmain.lib SDL_ttf.lib libexpat.lib libcurl.lib ws2_32.lib theora_static.lib ogg_static.lib vorbis_static.lib zlibd.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DRODUtil - Win32 Release"
# Name "DRODUtil - Win32 Debug"
# Begin Group "General"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DRODUtil.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\OptionList.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionList.h
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.h
# End Source File
# Begin Source File

SOURCE=.\Util3_0.cpp
# End Source File
# Begin Source File

SOURCE=.\Util3_0.h
# End Source File
# End Group
# Begin Group "DRODIncludes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DROD\BoundingBox.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\Color.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\DrodBitmapManager.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\EditRoomWidget.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\FaceWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\GameScreenDummy.h
# End Source File
# Begin Source File

SOURCE=..\DROD\Light.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\MapWidget.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\MapWidget.h
# End Source File
# Begin Source File

SOURCE=..\DROD\ParticleExplosionEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\PendingBuildEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\PendingPlotEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\Point.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\Rectangle.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\RoomEffectList.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\RoomWidget.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\Scene.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\SnowflakeEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\SparkEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\Sphere.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\StrikeOrbEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\TileImageCalcs.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\VarMonitorEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\DROD\ZombieGazeEffect.cpp
# End Source File
# End Group
# End Target
# End Project
