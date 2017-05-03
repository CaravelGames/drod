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

#include "LevelStartScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "GameScreen.h"

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbLevels.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Wchar.h>

const UINT CX_LEFTRIGHT_MARGIN = 50;
const UINT CY_HOLDNAME = 70;
const UINT CY_LEVELNAME = 70;
const UINT CY_DIVIDER_SPACE = 15;
const UINT CY_AUTHOR = 40;
const UINT CY_CREATED = CY_AUTHOR;
const UINT CY_TOPTEXT = CY_HOLDNAME + CY_LEVELNAME + CY_AUTHOR + CY_CREATED + CY_DIVIDER_SPACE;
const UINT CY_DESCRIPTION = CScreen::CY_SCREEN - (CY_TOPTEXT + CY_DIVIDER_SPACE);

const int NO_CHANNEL = -1;
//
//Protected methods.
//

//************************************************************************************
CLevelStartScreen::CLevelStartScreen()
	: CDrodScreen(SCR_LevelStart)
	, pHoldNameWidget(NULL), pLevelNameWidget(NULL), pCreatedWidget(NULL)
	, pAuthorWidget(NULL), pDescriptionWidget(NULL)
	, voiceChannel(NO_CHANNEL)
//Constructor.
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

	static const int X_AUTHOR = CX_LEFTRIGHT_MARGIN;
	static const UINT CX_AUTHOR = CX_SCREEN - (CX_LEFTRIGHT_MARGIN * 2);

	static const int X_CREATED = X_AUTHOR;
	static const UINT CX_CREATED = CX_AUTHOR;

	static const int X_HOLDNAME = X_AUTHOR;
	static const UINT CX_HOLDNAME = CX_AUTHOR;

	static const int X_LEVELNAME = X_AUTHOR;
	static const UINT CX_LEVELNAME = CX_AUTHOR;

	static const int X_DESCRIPTION = X_AUTHOR;
	static const UINT CX_DESCRIPTION = CX_AUTHOR;

	this->pHoldNameWidget = new CLabelWidget(0L, X_HOLDNAME, 0, 
			CX_HOLDNAME, CY_HOLDNAME, F_LevelName, wszEmpty);
	this->pHoldNameWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pHoldNameWidget);

	this->pLevelNameWidget = new CLabelWidget(0L, X_LEVELNAME, 0, 
			CX_LEVELNAME, CY_LEVELNAME, F_LevelName, wszEmpty);
	this->pLevelNameWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pLevelNameWidget);

	this->pCreatedWidget = new CLabelWidget(0L, X_CREATED, 0, CX_CREATED,
			CY_CREATED, F_LevelInfo, wszEmpty);
	this->pCreatedWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pCreatedWidget);

	this->pAuthorWidget = new CLabelWidget(0L, X_AUTHOR, 0, CX_AUTHOR, CY_AUTHOR,
			F_LevelInfo, wszEmpty);
	this->pAuthorWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pAuthorWidget);

	this->pDescriptionWidget = new CLabelWidget(0L, X_DESCRIPTION, 0, 
			CX_DESCRIPTION, CY_DESCRIPTION, F_LevelDescription, wszEmpty);
	this->pDescriptionWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pDescriptionWidget);
}

//*****************************************************************************
void CLevelStartScreen::Paint(
//Overridable method to paint the screen.  
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
	                  //    surface is the screen, the screen
	                  //    will be immediately updated in
	                  //    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	//Vertically center all the stuff.
	UINT wIgnored, wTextH;
	this->pDescriptionWidget->GetTextWidthHeight(wIgnored, wTextH);
	const int yTop = (CY_SCREEN - (CY_TOPTEXT + wTextH + CY_DIVIDER_SPACE)) / 3;  //slightly above center
	const int Y_DIVIDER = yTop + CY_TOPTEXT;

	//Move text to correct vertical position.
	this->pHoldNameWidget->Move(this->pHoldNameWidget->GetX(),yTop);
	this->pLevelNameWidget->Move(this->pLevelNameWidget->GetX(),yTop + CY_HOLDNAME);
	this->pCreatedWidget->Move(this->pCreatedWidget->GetX(),yTop + CY_HOLDNAME + CY_LEVELNAME);
	this->pAuthorWidget->Move(this->pAuthorWidget->GetX(),Y_DIVIDER - CY_DIVIDER_SPACE - CY_AUTHOR);
	this->pDescriptionWidget->Move(this->pDescriptionWidget->GetX(),Y_DIVIDER + CY_DIVIDER_SPACE);

	//Draw dividers.
	DrawDivider(Y_DIVIDER);
	const int nLowerDividerY = Y_DIVIDER + CY_DIVIDER_SPACE + wTextH + CY_DIVIDER_SPACE;
	if (nLowerDividerY < CY_SCREEN)
		DrawDivider(nLowerDividerY);

	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
bool CLevelStartScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	//Get level that the game screen is on.
	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	CCurrentGame *pCurrentGame = pGameScreen->GetCurrentGame();
	ASSERT(pCurrentGame);
	CDbLevel *pLevel = pCurrentGame->pLevel;
	if (!pLevel) { ASSERT(!"Couldn't retrieve level."); return false; }
	CDbHold *pHold = pCurrentGame->pHold;
	if (!pHold) { ASSERT(!"Couldn't retrieve hold."); return false; }

	//Set label texts.
	this->pHoldNameWidget->SetText(pHold->NameText);
	this->pLevelNameWidget->SetText(pLevel->NameText);

	if (CDbHold::IsOfficialHold(pHold->status) || pHold->status == CDbHold::Tutorial) {
		this->pCreatedWidget->SetText(wszEmpty);
		this->pAuthorWidget->SetText(wszEmpty);
	} else {
		WSTRING wstrCreated = (WSTRING)CDbMessageText(MID_LevelCreated);
		wstrCreated += wszSpace;
		pLevel->Created.GetLocalFormattedText(DF_LONG_DATE, wstrCreated);
		this->pCreatedWidget->SetText(wstrCreated.c_str());

		WSTRING wstrAuthor = (WSTRING)CDbMessageText(MID_LevelBy);
		wstrAuthor += wszSpace;
		wstrAuthor += pLevel->GetAuthorText();
		this->pAuthorWidget->SetText(wstrAuthor.c_str());
	}

	CEntranceData *pEntrance = pCurrentGame->pEntrance;
	ASSERT(pEntrance);

	const WSTRING text = pCurrentGame->ExpandText(pEntrance->DescriptionText); //resolve var refs
	this->pDescriptionWidget->SetText(text.c_str());

	//This screen should always return to the game screen.
	if (g_pTheSM->GetReturnScreenType() != SCR_Game)
		g_pTheSM->InsertReturnScreen(SCR_Game);

	//Play any audio for this screen.
	this->voiceChannel = NO_CHANNEL;
	if (pEntrance->dwDataID) {
		CDbDatum *pSound = g_pTheDB->Data.GetByID(pEntrance->dwDataID);
		if (pSound) {
			this->voiceChannel = g_pTheSound->PlayVoice(pSound->data);
			delete pSound;

			//When music resumes, start track from the beginning, without resuming any cross-fade in progress.
			if (this->voiceChannel != NO_CHANNEL)
				g_pTheSound->StopSong();
		}
	}

	//Start music playing.
	pGameScreen->SyncMusic();

	if (this->voiceChannel != NO_CHANNEL)
		g_pTheSound->PauseMusic();

	//Eat existing events since keypresses, etc., will exit this screen immediately.
	ClearEvents();

	return true;
}

//
//Private methods.
//

//******************************************************************************
void CLevelStartScreen::CheckForVoiceDone(const bool bFinishNow)
{
	if (this->voiceChannel != NO_CHANNEL)
	{
 		if (bFinishNow || !g_pTheSound->IsSoundPlayingOnChannel(this->voiceChannel))
		{
			g_pTheSound->UnpauseMusic();
			this->voiceChannel = NO_CHANNEL;
		}
	}
}

//******************************************************************************
void CLevelStartScreen::OnBetweenEvents()
{
	CheckForVoiceDone();
}

void CLevelStartScreen::OnDeactivate()
{
	CheckForVoiceDone(true);
}

//******************************************************************************
void CLevelStartScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	CScreen::OnKeyDown(dwTagNo, Key);
	if (IsDeactivating()) return;

	//Changing screen size shouldn't advance to next screen.
	switch (Key.keysym.sym)
	{
		case SDLK_SPACE:
		case SDLK_ESCAPE:
		case SDLK_KP_ENTER:
			//These keys can exit the screen
			break;
		case SDLK_RETURN:
			if (Key.keysym.mod & KMOD_ALT)
				return; //fullscreen toggle should not exit this screen
			break;
		default:
		{
			//Bound game movement command keys can exit this screen.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			const int nCommand = pGameScreen->GetCommandForKeysym(Key.keysym.sym);
			if (nCommand >= CMD_N && nCommand <= CMD_WAIT)
				break;

			return;
		}
	}

	//Prevent premature input from skipping this screen.
	if (SDL_GetTicks() - GetTimeActivated() < 300)
		return;

	GoToScreen(SCR_Return);
}

//******************************************************************************
void CLevelStartScreen::OnMouseUp(
//Called when widget receives SDL_MOUSEBUTTONUP event.
//
//Params:
	const UINT /*dwTagNo*/,            //(in)   Widget that event applied to.
	const SDL_MouseButtonEvent &/*Button*/)   //(in)   Event.
{
	GoToScreen(SCR_Return);
}

//******************************************************************************
void CLevelStartScreen::DrawDivider(
//Draws a divider at a specified row.
//
//Params:
	const int nY)  //(in)   Row to draw at.
{
	const SURFACECOLOR Gray = GetSurfaceColor(GetDestSurface(), 102, 102, 102);

	DrawRow(CX_LEFTRIGHT_MARGIN, nY, CX_SCREEN - (CX_LEFTRIGHT_MARGIN * 2), Gray);
}
