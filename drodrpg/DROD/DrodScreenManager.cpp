// $Id: DrodScreenManager.cpp 8521 2008-01-17 23:12:25Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#define INCLUDED_FROM_DRODSCREENMANAGER_CPP
#include "DrodScreenManager.h"
#undef INCLUDED_FROM_DRODSCREENMANAGER_CPP

//All the screens being instantiated.
#include "BrowserScreen.h"
#include "ChatScreen.h"
#include "CreditsScreen.h"
#include "DemoScreen.h"
#include "DemosScreen.h"
#include "EditRoomScreen.h"
#include "EditSelectScreen.h"
#include "GameScreen.h"
#include "HoldSelectScreen.h"
#include "LevelStartScreen.h"
#include "ModScreen.h"
#include "NewPlayerScreen.h"
#include "RestoreScreen.h"
#include "SelectPlayerScreen.h"
#include "SellScreen.h"
#include "SettingsScreen.h"
#include "TitleScreen.h"
#include "WinStartScreen.h"
#include "WorldMapScreen.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>

//Holds the only instance of CDrodScreenManager for the app.
CDrodScreenManager *g_pTheDSM = NULL;

//
//CScreenManager public methods.
//

//*****************************************************************************
UINT CDrodScreenManager::Init()
//Inits the screen manager.
//
//Returns:
//MID_Success or an message ID for failure.
{
	LOGCONTEXT("CDrodScreenManager::Init");

	//Load mouse cursors.
	if (!LoadCursors())
		return MID_CouldNotLoadResources;

	//Success.
	return MID_Success;
}

//*****************************************************************************
CDrodScreenManager::CDrodScreenManager(
//Constructor.
//
//Params:
	SDL_Surface *pSetScreenSurface) //(in) The screen surface.
	: CScreenManager(pSetScreenSurface)
{
}

//*****************************************************************************
void CDrodScreenManager::GetScreenName(const UINT eScreen, string &strName) const
{
	switch (eScreen)
	{
		case SCR_Title:           strName = "Title"; return;      
		case SCR_Game:            strName = "Game"; return;
		case SCR_WinStart:        strName = "WinStart"; return;
		case SCR_Settings:        strName = "Settings"; return;
		case SCR_Restore:         strName = "Restore"; return;
		case SCR_Demo:            strName = "Demo"; return;
		case SCR_Demos:           strName = "Demos"; return;
		case SCR_LevelStart:      strName = "LevelStart"; return;
		case SCR_Credits:         strName = "Credits"; return;
		case SCR_NewPlayer:       strName = "NewPlayer"; return;
		case SCR_SelectPlayer:    strName = "SelectPlayer"; return;
		case SCR_EditSelect:      strName = "EditSelect"; return;
		case SCR_EditRoom:        strName = "EditRoom"; return;
		case SCR_HoldSelect:      strName = "HoldSelect"; return;
		case SCR_Browser:         strName = "Browser"; return;
		case SCR_Sell:            strName = "Sell"; return;
		case SCR_Mod:             strName = "Mod"; return;
		case SCR_Chat:            strName = "Chat"; return;
		case SCR_WorldMap:        strName = "WorldMap"; return;
		default:                  strName = "Unknown"; ASSERT(!"Bad screen type."); return;
	}
}

//*****************************************************************************
void CDrodScreenManager::InitCrossfadeDuration()
//Queries the current setting for crossfade duration and applies it.
{
	string strDuration;
	if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::CrossfadeDuration, strDuration))
	{
		static const UINT MAX_CROSSFADE = 1000; //ms
		const UINT duration = atoi(strDuration.c_str());
		this->crossfadeDuration = duration > MAX_CROSSFADE ? MAX_CROSSFADE : duration;
	}
}

//
//CDrodScreenManager protected methods.
//

//*****************************************************************************
CDrodScreenManager::~CDrodScreenManager()
//Destructor.
{
	FreeCursors();
}

//*****************************************************************************
void CDrodScreenManager::FreeCursors()
{
	if (this->pCursor)
	{
		for (UINT i=CUR_Count; i--; )
			if (this->pCursor[i])
				SDL_FreeCursor(this->pCursor[i]);
		delete[] this->pCursor;
		this->pCursor = NULL;
	}
}

//*****************************************************************************
bool CDrodScreenManager::LoadCursors()
//Loads special cursors used by app.
//
//Returns:
//True if successful, false if not.
{
	ASSERT(!this->pCursor);
	this->pCursor = new SDL_Cursor*[CUR_Count];
	if (!this->pCursor) return false;

	UINT nIndex = 0;
	this->pCursor[nIndex++] = LoadSDLCursor("Cursor");
	this->pCursor[nIndex++] = LoadSDLCursor("hourglass");
	this->pCursor[nIndex++] = LoadSDLCursor("internet");
	ASSERT(nIndex == CUR_Count);
	for (nIndex=CUR_Count; nIndex--; )
		if (!this->pCursor[nIndex])
		{
			FreeCursors();
			return false;
		}

	return true;
}

//*****************************************************************************
CScreen* CDrodScreenManager::GetNewScreen(
//Class factory for CScreen-derived classes.
//
//Params:
	const UINT eScreen)  //(in)   Type of screen to construct.
//
//Returns:
//New screen object.
{
	switch (eScreen)
	{
		case SCR_Title:
			return new CTitleScreen;

		case SCR_Game:
			return new CGameScreen;

		case SCR_WinStart:
			return new CWinStartScreen;

		case SCR_Settings:
			return new CSettingsScreen;

		case SCR_Restore:
			return new CRestoreScreen;

		case SCR_Demo:
			return new CDemoScreen;

		case SCR_Demos:
			return new CDemosScreen;

		case SCR_LevelStart:
			return new CLevelStartScreen;

		case SCR_Credits:
			return new CCreditsScreen;

		case SCR_NewPlayer:
			return new CNewPlayerScreen;

		case SCR_SelectPlayer:
			return new CSelectPlayerScreen;

		case SCR_EditSelect:
			return new CEditSelectScreen;

		case SCR_EditRoom:
			return new CEditRoomScreen;

		case SCR_HoldSelect:
			return new CHoldSelectScreen;

		case SCR_Browser:
			return new CBrowserScreen;

		case SCR_Sell:
			return new CSellScreen;

		case SCR_Mod:
			return new CModScreen;

		case SCR_Chat:
			return new CChatScreen;

		case SCR_WorldMap:
			return new CWorldMapScreen;

		default:
			ASSERT(!"Bad screen type.");
			return NULL;
	}
}
