# Microsoft Developer Studio Project File - Name="DRODLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=DRODLib - Win32 Russian Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DRODLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DRODLib.mak" CFG="DRODLib - Win32 Russian Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DRODLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "DRODLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "DRODLib - Win32 Russian Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DRODLib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "NDEBUG" /D "UNICODE" /D "WIN32" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "DRODLib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "DRODLib - Win32 Russian Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DRODLib___Win32_Russian_Debug"
# PROP BASE Intermediate_Dir "DRODLib___Win32_Russian_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Russian_Debug"
# PROP Intermediate_Dir "Russian_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT BASE CPP /Fr /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_MBCS" /D "_LIB" /D "RUSSIAN_BUILD" /FD /GZ /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
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

# Name "DRODLib - Win32 Release"
# Name "DRODLib - Win32 Debug"
# Name "DRODLib - Win32 Russian Debug"
# Begin Group "General"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BuildUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildUtil.h
# End Source File
# Begin Source File

SOURCE=.\CueEvents.cpp
# End Source File
# Begin Source File

SOURCE=.\CueEvents.h
# End Source File
# Begin Source File

SOURCE=.\CurrentGame.cpp
# End Source File
# Begin Source File

SOURCE=.\CurrentGame.h
# End Source File
# Begin Source File

SOURCE=.\EntranceData.cpp
# End Source File
# Begin Source File

SOURCE=.\EntranceData.h
# End Source File
# Begin Source File

SOURCE=.\GameConstants.cpp
# End Source File
# Begin Source File

SOURCE=.\GameConstants.h
# End Source File
# Begin Source File

SOURCE=.\NetInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\NetInterface.h
# End Source File
# Begin Source File

SOURCE=.\PlayerStats.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerStats.h
# End Source File
# Begin Source File

SOURCE=.\SettingsKeys.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsKeys.h
# End Source File
# Begin Source File

SOURCE=.\Swordsman.cpp
# End Source File
# Begin Source File

SOURCE=.\Swordsman.h
# End Source File
# Begin Source File

SOURCE=.\TileConstants.h
# End Source File
# Begin Source File

SOURCE=.\TileMask.h
# End Source File
# End Group

SOURCE=.\Weapons.cpp
# End Source File
# Begin Source File

SOURCE=.\Weapons.h
# End Source File
# Begin Source File
# Begin Group "Data Access"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Db.cpp
# End Source File
# Begin Source File

SOURCE=.\Db.h
# End Source File
# Begin Source File

SOURCE=.\DbBase.cpp
# End Source File
# Begin Source File

SOURCE=.\DbBase.h
# End Source File
# Begin Source File

SOURCE=.\DbCommands.cpp
# End Source File
# Begin Source File

SOURCE=.\DbCommands.h
# End Source File
# Begin Source File

SOURCE=.\DbData.cpp
# End Source File
# Begin Source File

SOURCE=.\DbData.h
# End Source File
# Begin Source File

SOURCE=.\DbDemos.cpp
# End Source File
# Begin Source File

SOURCE=.\DbDemos.h
# End Source File
# Begin Source File

SOURCE=.\DbHolds.cpp
# End Source File
# Begin Source File

SOURCE=.\DbHolds.h
# End Source File
# Begin Source File

SOURCE=.\DbLevels.cpp
# End Source File
# Begin Source File

SOURCE=.\DbLevels.h
# End Source File
# Begin Source File

SOURCE=.\DbMessageText.cpp
# End Source File
# Begin Source File

SOURCE=.\DbMessageText.h
# End Source File
# Begin Source File

SOURCE=.\DbPackedVars.cpp
# End Source File
# Begin Source File

SOURCE=.\DbPackedVars.h
# End Source File
# Begin Source File

SOURCE=.\DbPlayers.cpp
# End Source File
# Begin Source File

SOURCE=.\DbPlayers.h
# End Source File
# Begin Source File

SOURCE=.\DbProps.h
# End Source File
# Begin Source File

SOURCE=.\DbRefs.cpp
# End Source File
# Begin Source File

SOURCE=.\DbRefs.h
# End Source File
# Begin Source File

SOURCE=.\DbRooms.cpp
# End Source File
# Begin Source File

SOURCE=.\DbRooms.h
# End Source File
# Begin Source File

SOURCE=.\DbSavedGames.cpp
# End Source File
# Begin Source File

SOURCE=.\DbSavedGames.h
# End Source File
# Begin Source File

SOURCE=.\DbSpeech.cpp
# End Source File
# Begin Source File

SOURCE=.\DbSpeech.h
# End Source File
# Begin Source File

SOURCE=.\DbVDInterface.h
# End Source File
# Begin Source File

SOURCE=.\DbXML.cpp
# End Source File
# Begin Source File

SOURCE=.\DbXML.h
# End Source File
# Begin Source File

SOURCE=.\DemoRecInfo.h
# End Source File
# Begin Source File

SOURCE=.\EntranceData.h
# End Source File
# Begin Source File

SOURCE=.\HoldRecords.h
# End Source File
# Begin Source File

SOURCE=.\ImportInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ImportInfo.h
# End Source File
# End Group
# Begin Group "Monsters"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Ant.cpp
# End Source File
# Begin Source File

SOURCE=.\Ant.h
# End Source File
# Begin Source File

SOURCE=.\AntHill.cpp
# End Source File
# Begin Source File

SOURCE=.\AntHill.h
# End Source File
# Begin Source File

SOURCE=.\Architect.cpp
# End Source File
# Begin Source File

SOURCE=.\Architect.h
# End Source File
# Begin Source File

SOURCE=.\BlueSerpent.cpp
# End Source File
# Begin Source File

SOURCE=.\BlueSerpent.h
# End Source File
# Begin Source File

SOURCE=.\Brain.cpp
# End Source File
# Begin Source File

SOURCE=.\Brain.h
# End Source File
# Begin Source File

SOURCE=.\Character.cpp
# End Source File
# Begin Source File

SOURCE=.\Character.h
# End Source File
# Begin Source File

SOURCE=.\CharacterCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\CharacterCommand.h
# End Source File
# Begin Source File

SOURCE=.\Citizen.cpp
# End Source File
# Begin Source File

SOURCE=.\Citizen.h
# End Source File
# Begin Source File

SOURCE=.\Clone.cpp
# End Source File
# Begin Source File

SOURCE=.\Clone.h
# End Source File
# Begin Source File

SOURCE=.\Construct.cpp
# End Source File
# Begin Source File

SOURCE=.\Construct.h
# End Source File
# Begin Source File

SOURCE=.\Decoy.cpp
# End Source File
# Begin Source File

SOURCE=.\Decoy.h
# End Source File
# Begin Source File

SOURCE=.\EvilEye.cpp
# End Source File
# Begin Source File

SOURCE=.\EvilEye.h
# End Source File
# Begin Source File

SOURCE=.\FluffBaby.cpp
# End Source File
# Begin Source File

SOURCE=.\FluffBaby.h
# End Source File
# Begin Source File

SOURCE=.\Gentryii.cpp
# End Source File
# Begin Source File

SOURCE=.\Gentryii.h
# End Source File
# Begin Source File

SOURCE=.\Ghost.cpp
# End Source File
# Begin Source File

SOURCE=.\Ghost.h
# End Source File
# Begin Source File

SOURCE=.\Goblin.cpp
# End Source File
# Begin Source File

SOURCE=.\Goblin.h
# End Source File
# Begin Source File

SOURCE=.\GreenSerpent.cpp
# End Source File
# Begin Source File

SOURCE=.\GreenSerpent.h
# End Source File
# Begin Source File

SOURCE=.\Guard.cpp
# End Source File
# Begin Source File

SOURCE=.\Guard.h
# End Source File
# Begin Source File

SOURCE=.\Halph.cpp
# End Source File
# Begin Source File

SOURCE=.\Halph.h
# End Source File
# Begin Source File

SOURCE=.\Mimic.cpp
# End Source File
# Begin Source File

SOURCE=.\Mimic.h
# End Source File
# Begin Source File

SOURCE=.\Monster.cpp
# End Source File
# Begin Source File

SOURCE=.\Monster.h
# End Source File
# Begin Source File

SOURCE=.\MonsterFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\MonsterFactory.h
# End Source File
# Begin Source File

SOURCE=.\MonsterType.h
# End Source File
# Begin Source File

SOURCE=.\MonsterMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\MonsterMessage.h
# End Source File
# Begin Source File

SOURCE=.\MonsterPiece.cpp
# End Source File
# Begin Source File

SOURCE=.\MonsterPiece.h
# End Source File
# Begin Source File

SOURCE=.\Neather.cpp
# End Source File
# Begin Source File

SOURCE=.\Neather.h
# End Source File
# Begin Source File

SOURCE=.\Phoenix.cpp
# End Source File
# Begin Source File

SOURCE=.\Phoenix.h
# End Source File
# Begin Source File

SOURCE=.\PhoenixAshes.cpp
# End Source File
# Begin Source File

SOURCE=.\PhoenixAshes.h
# End Source File
# Begin Source File

SOURCE=.\PlayerDouble.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerDouble.h
# End Source File
# Begin Source File

SOURCE=.\RedSerpent.cpp
# End Source File
# Begin Source File

SOURCE=.\RedSerpent.h
# End Source File
# Begin Source File

SOURCE=.\Roach.cpp
# End Source File
# Begin Source File

SOURCE=.\Roach.h
# End Source File
# Begin Source File

SOURCE=.\RoachEgg.cpp
# End Source File
# Begin Source File

SOURCE=.\RoachEgg.h
# End Source File
# Begin Source File

SOURCE=.\RoachQueen.cpp
# End Source File
# Begin Source File

SOURCE=.\RoachQueen.h
# End Source File
# Begin Source File

SOURCE=.\RockGiant.cpp
# End Source File
# Begin Source File

SOURCE=.\RockGiant.h
# End Source File
# Begin Source File

SOURCE=.\Serpent.cpp
# End Source File
# Begin Source File

SOURCE=.\Serpent.h
# End Source File
# Begin Source File

SOURCE=.\Slayer.cpp
# End Source File
# Begin Source File

SOURCE=.\Slayer.h
# End Source File
# Begin Source File

SOURCE=.\Spider.cpp
# End Source File
# Begin Source File

SOURCE=.\Spider.h
# End Source File
# Begin Source File

SOURCE=.\Splitter.cpp
# End Source File
# Begin Source File

SOURCE=.\Splitter.h
# End Source File
# Begin Source File

SOURCE=.\Stalwart.cpp
# End Source File
# Begin Source File

SOURCE=.\Stalwart.h
# End Source File
# Begin Source File

SOURCE=.\TarBaby.cpp
# End Source File
# Begin Source File

SOURCE=.\TarBaby.h
# End Source File
# Begin Source File

SOURCE=.\TarMother.cpp
# End Source File
# Begin Source File

SOURCE=.\TarMother.h
# End Source File
# Begin Source File

SOURCE=.\TemporalClone.cpp
# End Source File
# Begin Source File

SOURCE=.\TemporalClone.h
# End Source File
# Begin Source File

SOURCE=.\WraithWing.cpp
# End Source File
# Begin Source File

SOURCE=.\Wraithwing.h
# End Source File
# Begin Source File

SOURCE=.\Wubba.cpp
# End Source File
# Begin Source File

SOURCE=.\Wubba.h
# End Source File
# Begin Source File

SOURCE=.\Zombie.cpp
# End Source File
# Begin Source File

SOURCE=.\Zombie.h
# End Source File
# End Group
# Begin Group "Items"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Briar.cpp
# End Source File
# Begin Source File

SOURCE=.\Briar.h
# End Source File
# Begin Source File

SOURCE=.\Bridge.cpp
# End Source File
# Begin Source File

SOURCE=.\Bridge.h
# End Source File
# Begin Source File

SOURCE=.\Building.cpp
# End Source File
# Begin Source File

SOURCE=.\Building.h
# End Source File
# Begin Source File

SOURCE=.\PathMap.cpp
# End Source File
# Begin Source File

SOURCE=.\PathMap.h
# End Source File
# Begin Source File

SOURCE=.\Platform.cpp
# End Source File
# Begin Source File

SOURCE=.\Platform.h
# End Source File
# Begin Source File

SOURCE=.\Station.cpp
# End Source File
# Begin Source File

SOURCE=.\Station.h
# End Source File
# End Group
# End Target
# End Project
