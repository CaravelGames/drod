// $Id: DrodScreenManager.h 8367 2007-10-19 14:19:47Z mrimer $

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//DrodScreenManager.h
//Declarations for CDrodScreenManager.
//CScreenManager loads and unloads DROD screens, with knowledge of how DROD uses the
//screens.  It directs execution to an input loop in an appropriate CScreen class.

#ifndef DRODSCREENMANAGER_H
#define DRODSCREENMANAGER_H

#include <FrontEndLib/ScreenManager.h>

//All of these screen types should be handled in GetNewScreen() and GetScreenName().
enum SCREENTYPE
{
	SCR_None = SCREENLIB::SCR_None,     //Not an actual screen--indicates no screen or application exit.
	SCR_Return = SCREENLIB::SCR_Return, //Not an actual screen--indicates the screen previously visited.
	SCR_Title,
	SCR_Game,
	SCR_WinStart,
	SCR_Settings,
	SCR_Restore,
	SCR_Demo,
	SCR_Demos,
	SCR_LevelStart,
	SCR_Credits,
	SCR_NewPlayer,
	SCR_SelectPlayer,
	SCR_EditSelect,
	SCR_EditRoom,
	SCR_HoldSelect,
	SCR_Browser,
	SCR_Sell,
	SCR_Mod,
	SCR_Chat,
	SCR_WorldMap
};

//Cursor icons.
enum CURSORTYPE {
	CUR_Select = SCREENLIB::CUR_Select,
	CUR_Wait = SCREENLIB::CUR_Wait,
	CUR_Internet = SCREENLIB::CUR_Internet,
	CUR_Count
};

//*****************************************************************************
class CScreen;
class CDrodScreenManager : public CScreenManager
{
public:
	CDrodScreenManager(SDL_Surface *pSetScreenSurface);
	virtual ~CDrodScreenManager();

	virtual void      GetScreenName(const UINT eScreen, string &strName) const;
	virtual UINT      Init();
	void              InitCrossfadeDuration();
	
protected:
	virtual void FreeCursors();
	virtual CScreen* GetNewScreen(const UINT eScreen);
	virtual bool LoadCursors();
};

//Define global pointer to the one and only CDrodScreenManager object.
#ifndef INCLUDED_FROM_DRODSCREENMANAGER_CPP
	extern CDrodScreenManager *g_pTheDSM;
#endif

#endif //...#ifndef DRODSCREENMANAGER_H
