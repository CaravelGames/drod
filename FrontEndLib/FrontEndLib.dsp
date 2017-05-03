# Microsoft Developer Studio Project File - Name="FrontEndLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=FrontEndLib - Win32 Russian Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FrontEndLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FrontEndLib.mak" CFG="FrontEndLib - Win32 Russian Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FrontEndLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "FrontEndLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "FrontEndLib - Win32 Russian Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FrontEndLib - Win32 Release"

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

!ELSEIF  "$(CFG)" == "FrontEndLib - Win32 Debug"

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

!ELSEIF  "$(CFG)" == "FrontEndLib - Win32 Russian Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "FrontEndLib___Win32_Russian_Debug"
# PROP BASE Intermediate_Dir "FrontEndLib___Win32_Russian_Debug"
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

# Name "FrontEndLib - Win32 Release"
# Name "FrontEndLib - Win32 Debug"
# Name "FrontEndLib - Win32 Russian Debug"
# Begin Group "General"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BitmapManager.cpp
# End Source File
# Begin Source File

SOURCE=.\BitmapManager.h
# End Source File
# Begin Source File

SOURCE=.\Colors.cpp
# End Source File
# Begin Source File

SOURCE=.\Colors.h
# End Source File
# Begin Source File

SOURCE=.\Fade.cpp
# End Source File
# Begin Source File

SOURCE=.\Fade.h
# End Source File
# Begin Source File

SOURCE=.\FontManager.cpp
# End Source File
# Begin Source File

SOURCE=.\FontManager.h
# End Source File
# Begin Source File

SOURCE=.\Outline.cpp
# End Source File
# Begin Source File

SOURCE=.\Outline.h
# End Source File
# Begin Source File

SOURCE=.\Pan.cpp
# End Source File
# Begin Source File

SOURCE=.\Pan.h
# End Source File
# Begin Source File

SOURCE=.\Sound.cpp
# End Source File
# Begin Source File

SOURCE=.\Sound.h
# End Source File
# End Group
# Begin Group "Widgets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ButtonWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ButtonWidget.h
# End Source File
# Begin Source File

SOURCE=.\DialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\EventHandlerWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\EventHandlerWidget.h
# End Source File
# Begin Source File

SOURCE=.\FileDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\FileDialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\FocusWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\FocusWidget.h
# End Source File
# Begin Source File

SOURCE=.\FrameWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\FrameWidget.h
# End Source File
# Begin Source File

SOURCE=.\HTMLWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLWidget.h
# End Source File
# Begin Source File

SOURCE=.\HyperLinkWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\HyperLinkWidget.h
# End Source File
# Begin Source File

SOURCE=.\ImageWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ImageWidget.h
# End Source File
# Begin Source File

SOURCE=.\Inset.cpp
# End Source File
# Begin Source File

SOURCE=.\Inset.h
# End Source File
# Begin Source File

SOURCE=.\KeypressDialogWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\KeypressDialogWidget.h
# End Source File
# Begin Source File

SOURCE=.\LabelWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\LabelWidget.h
# End Source File
# Begin Source File

SOURCE=.\ListBoxWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ListBoxWidget.h
# End Source File
# Begin Source File

SOURCE=.\MarqueeWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\MarqueeWidget.h
# End Source File
# Begin Source File

SOURCE=.\MenuWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuWidget.h
# End Source File
# Begin Source File

SOURCE=.\ObjectMenuWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectMenuWidget.h
# End Source File
# Begin Source File

SOURCE=.\OptionButtonWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionButtonWidget.h
# End Source File
# Begin Source File

SOURCE=.\ProgressBarWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressBarWidget.h
# End Source File
# Begin Source File

SOURCE=.\ScalerWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ScalerWidget.h
# End Source File
# Begin Source File

SOURCE=.\ScrollableWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollableWidget.h
# End Source File
# Begin Source File

SOURCE=.\ScrollingTextWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollingTextWidget.h
# End Source File
# Begin Source File

SOURCE=.\SliderWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\SliderWidget.h
# End Source File
# Begin Source File

SOURCE=.\TabbedMenuWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\TabbedMenuWidget.h
# End Source File
# Begin Source File

SOURCE=.\TextBox2DWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\TextBox2DWidget.h
# End Source File
# Begin Source File

SOURCE=.\TextBoxWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\TextBoxWidget.h
# End Source File
# Begin Source File

SOURCE=.\TilesWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\TilesWidget.h
# End Source File
# Begin Source File

SOURCE=.\Widget.cpp
# End Source File
# Begin Source File

SOURCE=.\Widget.h
# End Source File
# End Group
# Begin Group "Screens"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Screen.cpp
# End Source File
# Begin Source File

SOURCE=.\Screen.h
# End Source File
# Begin Source File

SOURCE=.\ScreenManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ScreenManager.h
# End Source File
# End Group
# Begin Group "Effects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AnimatedTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimatedTileEffect.h
# End Source File
# Begin Source File

SOURCE=.\Bolt.cpp
# End Source File
# Begin Source File

SOURCE=.\Bolt.h
# End Source File
# Begin Source File

SOURCE=.\BumpObstacleEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\BumpObstacleEffect.h
# End Source File
# Begin Source File

SOURCE=.\Effect.cpp
# End Source File
# Begin Source File

SOURCE=.\Effect.h
# End Source File
# Begin Source File

SOURCE=.\EffectList.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectList.h
# End Source File
# Begin Source File

SOURCE=.\ExpandingTextEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpandingTextEffect.h
# End Source File
# Begin Source File

SOURCE=.\FadeTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FadeTileEffect.h
# End Source File
# Begin Source File

SOURCE=.\FlashMessageEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FlashMessageEffect.h
# End Source File
# Begin Source File

SOURCE=.\FlashShadeEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FlashShadeEffect.h
# End Source File
# Begin Source File

SOURCE=.\FloatEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FloatEffect.h
# End Source File
# Begin Source File

SOURCE=.\FloatTextEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FloatTextEffect.h
# End Source File
# Begin Source File

SOURCE=.\FrameRateEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\FrameRateEffect.h
# End Source File
# Begin Source File

SOURCE=.\MovingTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\MovingTileEffect.h
# End Source File
# Begin Source File

SOURCE=.\RotateTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\RotateTileEffect.h
# End Source File
# Begin Source File

SOURCE=.\ScaleTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ScaleTileEffect.h
# End Source File
# Begin Source File

SOURCE=.\ShadeEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ShadeEffect.h
# End Source File
# Begin Source File

SOURCE=.\SubtitleEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\SubtitleEffect.h
# End Source File
# Begin Source File

SOURCE=.\TextEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEffect.h
# End Source File
# Begin Source File

SOURCE=.\ToolTipEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolTipEffect.h
# End Source File
# Begin Source File

SOURCE=.\TransTileEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\TransTileEffect.h
# End Source File
# End Group
# Begin Group "ImageSupport"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\JpegHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\JpegHandler.h
# End Source File
# Begin Source File

SOURCE=.\PNGHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\PNGHandler.h
# End Source File
# Begin Source File

SOURCE=.\TheoraPlayer.cpp
# End Source File
# Begin Source File

SOURCE=.\TheoraPlayer.h
# End Source File
# End Group
# End Target
# End Project
