// $Id$

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
 * Portions created by the Initial Developer are Copyright (C)
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "WorldMapScreen.h"

#include "WorldMapWidget.h"
#include "GameScreen.h"

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbHolds.h"
#include "../DRODLib/SettingsKeys.h"

#include <FrontEndLib/FlashMessageEffect.h>

#include <BackEndLib/Files.h>

const UINT TAG_WORLDMAP = 1010;

//************************************************************************************
CWorldMapScreen::CWorldMapScreen()
	: CDrodScreen(SCR_WorldMap)
	, pWorldMapWidget(NULL)
{
	this->pWorldMapWidget = new CWorldMapWidget(TAG_WORLDMAP, 0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
	AddWidget(this->pWorldMapWidget);
}

//*****************************************************************************
void CWorldMapScreen::OnBetweenEvents()
{
	Animate();
}

void CWorldMapScreen::Animate()
{
	Paint();
}

//*****************************************************************************
void CWorldMapScreen::Paint(
//Overridable method to paint the screen.  
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
	                  //    surface is the screen, the screen
	                  //    will be immediately updated in
	                  //    the widget's rect.
{
	PaintChildren();
	
	this->pEffects->UpdateAndDrawEffects();

	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
bool CWorldMapScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	CCurrentGame *pCurrentGame = GetCurrentGame();
	ASSERT(pCurrentGame);

	SetMusic(pCurrentGame);

	this->pWorldMapWidget->SetCurrentGame(pCurrentGame);

	//Don't automatically return to the game screen.
	//This screen decides where to transition next.
	if (g_pTheSM->GetReturnScreenType() == SCR_Game)
		g_pTheSM->RemoveReturnScreen();

	CFlashMessageEffect *pTextPromptEffect = new CFlashMessageEffect(
		this, g_pTheDB->GetMessageText(MID_WorldMapDestinationPrompt), 250, 2000);
	pTextPromptEffect->SlowExpansion();
	this->pEffects->AddEffect(pTextPromptEffect);

	//Eat pre-existing events.
	ClearEvents();

	return true;
}

//
//Private methods.
//

CCurrentGame* CWorldMapScreen::GetCurrentGame() const
{
	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	return pGameScreen->GetCurrentGame();
}

//*****************************************************************************
void CWorldMapScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_WORLDMAP:
		{
			const WorldMapIcon *pIcon = this->pWorldMapWidget->GetClickedIcon();
			ASSERT(pIcon);
			switch (pIcon->displayFlags) {
				case ScriptFlag::WMI_DISABLED:
				case ScriptFlag::WMI_NOLABEL:
				break;
				case ScriptFlag::WMI_LOCKED:
					g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
				break;
				default:
				{
					g_pTheSound->PlaySoundEffect(SEID_SWORDS);

					const UINT entranceID = pIcon->entranceID;

					CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
							g_pTheSM->GetScreen(SCR_Game));
					ASSERT(pGameScreen);
					pGameScreen->GotoEntrance(entranceID);

					//Transition to next appropriate screen.
					const SCREENTYPE nextscreen = pGameScreen->SelectGotoScreen();
					g_pTheSM->InsertReturnScreen(nextscreen);
					Deactivate();
				}
				break;
			}
		}
	}
}

//******************************************************************************
void CWorldMapScreen::SetMusic(CCurrentGame *pCurrentGame)
{
	const WorldMapMusic music = pCurrentGame->GetWorldMapMusic(pCurrentGame->worldMapID);
	static const UINT defaultSongID = SONGID_INTRO_TSS;
	switch (music.songID) {
		case (UINT)SONGID_DEFAULT:
			g_pTheSound->CrossFadeSong(defaultSongID);
			break;
		case (UINT)SONGID_NONE:
			g_pTheSound->StopSong();
			break;
		case (UINT)SONGID_CUSTOM:
			g_pTheSound->PlayData(music.customID);
			break;
		default:
			if (music.songlist.empty()) {
				if (music.songID < UINT(SONGID_COUNT)) {
					g_pTheSound->CrossFadeSong(music.songID);
				} else {
					g_pTheSound->CrossFadeSong(defaultSongID);
				}
			} else {
				CFiles f;
				list<WSTRING> songlist;
				if (f.GetGameProfileString(INISection::Songs, music.songlist.c_str(), songlist))
				{
					g_pTheSound->CrossFadeSong(&songlist);
					f.WriteGameProfileString(INISection::Songs, music.songlist.c_str(), songlist);
				} else {
					g_pTheSound->CrossFadeSong(defaultSongID);
				}
			}
			break;
	}
}
