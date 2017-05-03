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

#include "WinStartScreen.h"
#include "GameScreen.h"
#include "DrodFontManager.h"
#include "DrodScreenManager.h"
#include "DrodSound.h"

#include "../DRODLib/Db.h"
#include "../DRODLib/DbPlayers.h"
#include "../DRODLib/NetInterface.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"

#include <FrontEndLib/TextEffect.h>

const UINT TAG_YOUWON_LABEL = 1001;
const UINT TAG_INFO_LABEL = 1002;

const UINT CX_LEFTRIGHT_MARGIN = 65;

const int X_YOUWON = CX_LEFTRIGHT_MARGIN;
const int Y_YOUWON = 0;
const UINT CY_YOUWON = 45;

const UINT CY_LABEL_SPACING = 10;
const int X_INFO = X_YOUWON;
const int Y_INFO = Y_YOUWON + CY_YOUWON + CY_LABEL_SPACING;

const SURFACECOLOR BlackBG = {0, 0, 0};

//
//Protected methods.
//

//************************************************************************************
CWinStartScreen::CWinStartScreen()
 : CDrodScreen(SCR_WinStart)
 , bCaravelNetHold(false)
//Constructor.
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

	static const UINT CX_YOUWON = CX_SCREEN - (CX_LEFTRIGHT_MARGIN * 2);
	static const UINT CX_INFO = CX_YOUWON;
	static const UINT CY_INFO = 1;

	//Add "You won" label.
	CLabelWidget *pYouWonLabel = new CLabelWidget(TAG_YOUWON_LABEL, X_YOUWON, Y_YOUWON,
			CX_YOUWON, CY_YOUWON, F_LevelName, wszEmpty);
	pYouWonLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pYouWonLabel);

	//Add info label underneath it, resizing to fit.
	CLabelWidget *pInfoLabel = new CLabelWidget(TAG_INFO_LABEL, X_INFO, Y_INFO, 
			CX_INFO, CY_INFO, F_LevelDescription, wszEmpty, true);
	AddWidget(pInfoLabel);
}

//*****************************************************************************
bool CWinStartScreen::IsMainDungeon() const
//Returns: whether this is an official hold
{
	const CDbHold::HoldStatus status = GetHoldStatus();
	const bool bOfficialHold = CDbHold::IsOfficialHold(status);

	if (status == CDbHold::TSS) {
		return IsGameFullVersion();
	} else {
		return bOfficialHold;
	}
}

//*****************************************************************************
void CWinStartScreen::OnBetweenEvents()
{
	//Erase alpha text so fade-in performs properly.
	SDL_Surface *pScreenSurface = GetDestSurface();
	for (list<CEffect *>::const_iterator effect = this->pEffects->Effects.begin();
			effect != this->pEffects->Effects.end(); ++effect)
	{
		SDL_BlitSurface(this->images[0], &((*effect)->dirtyRects[0]), pScreenSurface, &((*effect)->dirtyRects[0]));
	}

	CScreen::OnBetweenEvents();
}

//*****************************************************************************
void CWinStartScreen::Paint(
//Overridable method to paint the screen.  
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
bool CWinStartScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	PlayWinGameSong();

	//Return to the title screen for escape.
	g_pTheSM->ClearReturnScreens();
	g_pTheSM->InsertReturnScreen(SCR_Title);

	this->pEffects->Clear();

	const UINT dwHoldID = g_pTheDB->GetHoldID();

	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
	ASSERT(pHold);

	//Mark when tutorial is completed.
	if (pHold && pHold->status == CDbHold::Tutorial)
	{
		CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
		ASSERT(pPlayer);
		pPlayer->Settings.SetVar(Settings::TutorialFinished, UINT(1));
		pPlayer->Update();
		delete pPlayer;
	}

	//Store whether this is a CaravelNet hold.
	this->bCaravelNetHold = g_pTheNet->IsLocalHold(dwHoldID) && !g_pTheNet->IsBetaHold(dwHoldID);

	//Prepare "You Won" text.
	WSTRING wstrYouWon = g_pTheDB->GetMessageText(MID_YouConquered);
	wstrYouWon += wszSpace;
	wstrYouWon += pHold->NameText;
	wstrYouWon += wszExclamation;

	//Show customized end hold message, if defined.
	WSTRING wstrTmp = (WSTRING)pHold->EndHoldText;
	if (!wstrTmp.length())
	{
		//Show canned end message.
		wstrTmp = g_pTheDB->GetMessageText(MID_WinHomemadeDungeon1);
	
		CDbPlayer *pAuthor = g_pTheDB->Players.GetByID(pHold->dwPlayerID);
		ASSERT(pAuthor);
		wstrTmp += pAuthor->NameText;
		delete pAuthor;

		if (pHold->editingPrivileges == CDbHold::YouAndConquerors)
		{
			wstrTmp += g_pTheDB->GetMessageText(MID_WinHomemadeDungeonCanEdit1);
			wstrTmp += wszSpace;
			wstrTmp += pHold->NameText;
			wstrTmp += wszSpace;
			wstrTmp += g_pTheDB->GetMessageText(MID_WinHomemadeDungeonCanEdit2);
		} else {
			wstrTmp += wszQuote;
			wstrTmp += wszPeriod;
		}
	} else {
		//Interpolate hold vars in ending text.
		const UINT dwEndHoldSavedGameID = g_pTheDB->SavedGames.FindByEndHold(dwHoldID);
		CCueEvents Ignored;
		CCurrentGame *pEndHoldGame = g_pTheDB->GetSavedCurrentGame(dwEndHoldSavedGameID,
				Ignored, false, true);
		if (pEndHoldGame)
		{
			wstrTmp = pEndHoldGame->ExpandText(wstrTmp.c_str());
			delete pEndHoldGame;
		}
	}

	//Compile set of secret rooms found by player.
	HoldStats stats;
	const UINT wSecretsConquered = g_pTheDB->Holds.GetSecretsDone(
			stats, pHold->dwHoldID, g_pTheDB->GetPlayerID(), true);

	//Mark hold mastered if all secret room have been completed.
	if (wSecretsConquered >= stats.secretRooms.size() && !g_pTheDB->SavedGames.FindByHoldMastered(dwHoldID))
	{
		const UINT dwEndHoldSavedGameID = g_pTheDB->SavedGames.FindByEndHold(dwHoldID);
		CCueEvents Ignored;
		CCurrentGame *pEndHoldGame = g_pTheDB->GetSavedCurrentGame(dwEndHoldSavedGameID, Ignored);
		if (pEndHoldGame)
		{
			pEndHoldGame->SaveToHoldMastered();
			delete pEndHoldGame;
		}
	}

	CTextEffect *pSecretsText = NULL;
	if (!stats.secretRooms.empty())
	{
		//Only show percent of secrets found if there were any secret rooms at all.
		const UINT wPercent = stats.secretRooms.size() ?
				(wSecretsConquered*100)/stats.secretRooms.size() : 100;
		WCHAR temp[16];
		_itoW(wPercent, temp, 10);
		WSTRING wStr = g_pTheDB->GetMessageText(MID_SecretsConquered);
		wStr += wszSpace;
		wStr += temp;
		wStr += wszPercent;

		pSecretsText = new CTextEffect(this, wStr.c_str(), F_FlashMessage, 7000, 0, false, true);
		if (pSecretsText)
			this->pEffects->AddEffect(pSecretsText);
	}
	const UINT wSecretTextH = pSecretsText ? pSecretsText->dirtyRects[0].h + 20 : 0;

	//Now that I know combined height of the labels, center them vertically.
	CLabelWidget *pYouWonLabel = DYN_CAST(CLabelWidget *, CWidget*, GetWidget(TAG_YOUWON_LABEL));
	ASSERT(pYouWonLabel);
	CLabelWidget *pInfoLabel = DYN_CAST(CLabelWidget *, CWidget*, GetWidget(TAG_INFO_LABEL));
	ASSERT(pInfoLabel);
	pYouWonLabel->SetText(wstrYouWon.c_str(), true);
	pInfoLabel->SetText(wstrTmp.c_str(), true);
	const int cyLabels = pYouWonLabel->GetH() + CY_LABEL_SPACING + pInfoLabel->GetH() + wSecretTextH;
	const int yCenterOffset = (CY_SCREEN - cyLabels) / 2;
	pYouWonLabel->Move(X_YOUWON, Y_YOUWON + yCenterOffset +
			wSecretTextH/2); //slightly above center
	pInfoLabel->Move(X_INFO, pYouWonLabel->GetY() + pYouWonLabel->GetH() + CY_LABEL_SPACING);
	if (pSecretsText)
	{
		pSecretsText->Move(pSecretsText->X(), pYouWonLabel->GetY() - pSecretsText->dirtyRects[0].h - 20);
		if (pSecretsText->Y() < 0)
			pSecretsText->Move(pSecretsText->X(), 0);
	}

	delete pHold;

	ClearEvents(); //don't let an extra keypress during transition cause quick exit

	return true;
}

//
//Private methods.
//

//******************************************************************************
void CWinStartScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	CScreen::OnKeyDown(dwTagNo, Key);
	this->pEffects->Clear();
	if (IsDeactivating())
	{
		const bool bIsMainDungeon = IsMainDungeon();
		SetDestScreenType(bIsMainDungeon ? GetCreditsScreen() : SCR_Return);
		return;
	}

	RateHoldOrNextScreen();
}

//******************************************************************************
void CWinStartScreen::OnMouseUp(
//Called when widget receives SDL_MOUSEBUTTONDOWN event.
//Don't use mouse down to leave the screen.
//
//Params:
	const UINT dwTagNo,          //(in)   Widget that event applied to.
	const SDL_MouseButtonEvent &Button) //(in)   Event.
{
	CScreen::OnMouseUp(dwTagNo, Button);

	this->pEffects->Clear();

	RateHoldOrNextScreen();
}

//******************************************************************************
void CWinStartScreen::PlayWinGameSong()
{
	SONGID songid;
	switch (GetHoldStatus()) {
		case CDbHold::GatEB: songid = SONGID_WINGAME_GATEB; break;
		case CDbHold::JtRH: songid = SONGID_WINGAME_JTRH; break;
		case CDbHold::KDD: songid = SONGID_WINGAME_KDD; break;
		case CDbHold::TCB: songid = SONGID_WINGAME_TCB; break;
		case CDbHold::TSS: songid = SONGID_WINGAME_TSS; break;
		default: songid = SONGID_WINGAME_TCB; break;
	}

	g_pTheSound->PlaySong(songid);
}

//******************************************************************************
void CWinStartScreen::RateHoldOrNextScreen()
{
	const bool bIsMainDungeon = IsMainDungeon();
	if (!bIsMainDungeon)
		if (UserWillRateHold())
			return;

	GoToScreen(bIsMainDungeon ? GetCreditsScreen() : SCR_Return);
}

SCREENTYPE CWinStartScreen::GetCreditsScreen() const
{
	switch (GetHoldStatus()) {
		case CDbHold::GatEB: return SCR_CreditsGatEB;
		case CDbHold::JtRH: return SCR_CreditsJtRH;
		case CDbHold::KDD: return SCR_CreditsKDD;
		case CDbHold::TCB: return SCR_CreditsTCB;
		case CDbHold::TSS: return SCR_CreditsTSS;
		default: return SCR_Return;
	}
}

//******************************************************************************
bool CWinStartScreen::UserWillRateHold()
//Returns: whether user is prompted to rate hold and agrees to
{
	if (this->bCaravelNetHold)
	{
		//If this is a CaravelNet hold that the user has not rated,
		//prompt them to rate it now.
		const UINT dwHoldID = g_pTheDB->GetHoldID();
		vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
		const int index = g_pTheNet->getMediaIndexWithLocalHoldID(dwHoldID);
		ASSERT(index >= 0);
		const CNetMedia& holdData = *(cNetMedia[index]);
		double fRating = atof(holdData.myDifficulty.c_str());
		if (!fRating)
			fRating = atof(holdData.myRating.c_str());
		if (!fRating) //no personal rating is recorded
			if (ShowYesNoMessage(MID_RateHoldPrompt) == TAG_YES)
			{
				GoToScreen(SCR_HoldSelect);
				return true;
			}
	}
	return false;
}
