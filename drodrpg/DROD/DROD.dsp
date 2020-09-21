# Microsoft Developer Studio Project File - Name="drod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DROD - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DROD.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DROD.mak" CFG="DROD - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "drod - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "drod - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "drod - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "UNICODE" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 mk4vc60s.lib libjpeg.lib libpng.lib libcurl.lib user32.lib shell32.lib fmodvc.lib sdl.lib SDLmain.lib SDL_ttf.lib libexpat.lib ws2_32.lib theora_static.lib ogg_static.lib vorbis_static.lib zlib.lib /nologo /version:3.0 /subsystem:windows /profile /machine:I386 /nodefaultlib:"libc.lib"

!ELSEIF  "$(CFG)" == "drod - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "USE_LOGCONTEXT" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "UNICODE" /D "ENABLE_CHEATS" /D "CARAVELBUILD" /FD /GZ /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mk4vc60s_d.lib gdi32.lib libjpegd.lib libpng.lib libcurl.lib user32.lib shell32.lib fmodvc.lib sdl.lib SDLmain.lib SDL_ttf.lib libexpat.lib ws2_32.lib theora_static.lib ogg_static_d.lib vorbis_static_d.lib zlibd.lib /nologo /version:3.0 /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept
# SUBTRACT LINK32 /profile /pdb:none /map /nodefaultlib

!ENDIF 

# Begin Target

# Name "drod - Win32 Release"
# Name "drod - Win32 Debug"
# Begin Group "General"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Chat.cpp
# End Source File
# Begin Source File

SOURCE=.\Chat.h
# End Source File
# Begin Source File

SOURCE=.\DrodBitmapManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodBitmapManager.h
# End Source File
# Begin Source File

SOURCE=.\DrodFontManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodFontManager.h
# End Source File
# Begin Source File

SOURCE=.\DrodScreenManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodScreenManager.h
# End Source File
# Begin Source File

SOURCE=.\DrodSound.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodSound.h
# End Source File
# Begin Source File

SOURCE=.\DrodWidget.h
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\TileImageCalcs.cpp
# End Source File
# Begin Source File

SOURCE=.\TileImageCalcs.h
# End Source File
# Begin Source File

SOURCE=.\TileImageConstants.h
# End Source File
# End Group
# Begin Group "Screens"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BrowserScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserScreen.h
# End Source File
# Begin Source File

SOURCE=.\ChatScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\ChatScreen.h
# End Source File
# Begin Source File

SOURCE=.\CreditsScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\CreditsScreen.h
# End Source File
# Begin Source File

SOURCE=.\DemoScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\DemoScreen.h
# End Source File
# Begin Source File

SOURCE=.\DemosScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\DemosScreen.h
# End Source File
# Begin Source File

SOURCE=.\DrodScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodScreen.h
# End Source File
# Begin Source File

SOURCE=.\EditRoomScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\EditRoomScreen.h
# End Source File
# Begin Source File

SOURCE=.\EditSelectScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\EditSelectScreen.h
# End Source File
# Begin Source File

SOURCE=.\GameScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\GameScreen.h
# End Source File
# Begin Source File

SOURCE=.\HoldSelectScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\HoldSelectScreen.h
# End Source File
# Begin Source File

SOURCE=.\LevelStartScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\LevelStartScreen.h
# End Source File
# Begin Source File

SOURCE=.\ModScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\ModScreen.h
# End Source File
# Begin Source File

SOURCE=.\NewPlayerScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\NewPlayerScreen.h
# End Source File
# Begin Source File

SOURCE=.\RestoreScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\RestoreScreen.h
# End Source File
# Begin Source File

SOURCE=.\RoomScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\RoomScreen.h
# End Source File
# Begin Source File

SOURCE=.\SelectPlayerScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectPlayerScreen.h
# End Source File
# Begin Source File

SOURCE=.\SellScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\SellScreen.h
# End Source File
# Begin Source File

SOURCE=.\SettingsScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsScreen.h
# End Source File
# Begin Source File

SOURCE=.\TitleScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\TitleScreen.h
# End Source File
# Begin Source File

SOURCE=.\WinStartScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\WinStartScreen.h
# End Source File
# End Group
# Begin Group "Effects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BloodEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\BloodEffect.h
# End Source File
# Begin Source File

SOURCE=.\DebrisEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\DebrisEffect.h
# End Source File
# Begin Source File

SOURCE=.\DrodEffect.h
# End Source File
# Begin Source File

SOURCE=.\ExplosionEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ExplosionEffect.h
# End Source File
# Begin Source File

SOURCE=.\ParticleExplosionEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ParticleExplosionEffect.h
# End Source File
# Begin Source File

SOURCE=.\PendingBuildEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\PendingBuildEffect.h
# End Source File
# Begin Source File

SOURCE=.\PendingPlotEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\PendingPlotEffect.h
# End Source File
# Begin Source File

SOURCE=.\RoomEffectList.cpp
# End Source File
# Begin Source File

SOURCE=.\RoomEffectList.h
# End Source File
# Begin Source File

SOURCE=.\SnowflakeEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SnowflakeEffect.h
# End Source File
# Begin Source File

SOURCE=.\SparkEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SparkEffect.h
# End Source File
# Begin Source File

SOURCE=.\SplashEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SplashEffect.h
# End Source File
# Begin Source File

SOURCE=.\SteamEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SteamEffect.h
# End Source File
# Begin Source File

SOURCE=.\StrikeOrbEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\StrikeOrbEffect.h
# End Source File
# Begin Source File

SOURCE=.\SwordsmanSwirlEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SwordsmanSwirlEffect.h
# End Source File
# Begin Source File

SOURCE=.\SwordSwingEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SwordSwingEffect.h
# End Source File
# Begin Source File

SOURCE=.\TarStabEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\TarStabEffect.h
# End Source File
# Begin Source File

SOURCE=.\TrapdoorFallEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\TrapdoorFallEffect.h
# End Source File
# Begin Source File

SOURCE=.\VarMonitorEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\VarMonitorEffect.h
# End Source File
# Begin Source File

SOURCE=.\VerminEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\VerminEffect.h
# End Source File
# Begin Source File

SOURCE=.\ZombieGazeEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ZombieGazeEffect.h
# End Source File
# End Group
# Begin Group "Widgets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CharacterDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\CharacterDialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\DrodFileDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\DrodFileDialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\EditRoomWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\EditRoomWidget.h
# End Source File
# Begin Source File

SOURCE=.\EntranceSelectDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\EntranceSelectDialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\FaceWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\FaceWidget.h
# End Source File
# Begin Source File

SOURCE=.\MapWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\MapWidget.h
# End Source File
# Begin Source File

SOURCE=.\RoomWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\RoomWidget.h
# End Source File
# Begin Source File

SOURCE=.\WeatherDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\WeatherDialogWidget.h
# End Source File
# End Group
# Begin Group "Model"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BoundingBox.cpp
# End Source File
# Begin Source File

SOURCE=.\BoundingBox.h
# End Source File
# Begin Source File

SOURCE=.\Color.cpp
# End Source File
# Begin Source File

SOURCE=.\Color.h
# End Source File
# Begin Source File

SOURCE=.\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\Point.cpp
# End Source File
# Begin Source File

SOURCE=.\Point.h
# End Source File
# Begin Source File

SOURCE=.\Rectangle.cpp
# End Source File
# Begin Source File

SOURCE=.\Rectangle.h
# End Source File
# Begin Source File

SOURCE=.\Scene.cpp
# End Source File
# Begin Source File

SOURCE=.\Scene.h
# End Source File
# Begin Source File

SOURCE=.\SceneObject.h
# End Source File
# Begin Source File

SOURCE=.\Sphere.cpp
# End Source File
# Begin Source File

SOURCE=.\Sphere.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Res\DROD.ico
# End Source File
# Begin Source File

SOURCE=.\DROD.rc
# End Source File
# End Group
# End Target
# End Project
