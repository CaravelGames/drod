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
 * Contributor(s): Mike Rimer (mrimer), Matt Schikore (schik)
 *
 * ***** END LICENSE BLOCK ***** */

#include "NewPlayerScreen.h"
#include "GameScreen.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "SettingsScreen.h" //for COMMANDKEY_ARRAY

#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/TextBoxWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Metadata.h>
#include <BackEndLib/Wchar.h>

const UINT TAG_CANCEL = 1091;
const UINT TAG_IMPORT = 1092;
const UINT TAG_NAME = 1093;
const UINT TAG_GET_PLAYER_FROM_CLOUD = 1094;

//
//Protected methods.
//

//*****************************************************************************
CNewPlayerScreen::CNewPlayerScreen() : CDrodScreen(SCR_NewPlayer)
	, pPlayerBox(NULL)
	, pNameWidget(NULL)
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

#ifdef RUSSIAN_BUILD
	static const UINT CX_IMPORT_BUTTON = 180;
	static const UINT CX_DIALOG = 470;
#else
	static const UINT CX_IMPORT_BUTTON = 100;	
	static const UINT CX_DIALOG = 450;
#endif
#ifdef STEAMBUILD
	static const UINT CY_DIALOG = 302;
#else
	static const UINT CY_DIALOG = 360;
#endif

	static const UINT CX_SPACE = 25;
	static const UINT CY_SPACE = 15;

	static const int X_LABEL = 30;
	static const int Y_LABEL = 12;
	static const UINT CX_LABEL = CX_DIALOG - X_LABEL*2;
	static const UINT CY_LABEL = 190;

	static const int X_OK_BUTTON = CX_SPACE;
	static const UINT CX_OKAY_BUTTON = 90;
	static const int X_CANCEL_BUTTON = X_OK_BUTTON + CX_OKAY_BUTTON + CX_SPACE;
	static const UINT CX_CANCEL_BUTTON = 100;
	static const int X_IMPORT_BUTTON = CX_DIALOG - CX_IMPORT_BUTTON - CX_SPACE;

#ifdef STEAMBUILD
	static const int Y_BUTTONS = CY_DIALOG - 47;
#else
	static const int Y_BUTTONS = CY_DIALOG - 105;

	static const int Y_BUTTONS2 = Y_BUTTONS + CY_STANDARD_BUTTON + CY_SPACE;
	static const UINT CX_CLOUDIMPORTBUTTON = 250;
	static const int X_CLOUDIMPORTBUTTON = CX_DIALOG - CX_CLOUDIMPORTBUTTON - CX_SPACE;
#endif

	static const int X_TEXTBOX = X_OK_BUTTON;
	static const UINT CY_TEXTBOX = CY_STANDARD_TBOX;
	static const int Y_TEXTBOX = Y_BUTTONS - CY_TEXTBOX - CY_SPACE;
	static const UINT CX_TEXTBOX = CX_DIALOG - 2*X_TEXTBOX;

	this->pPlayerBox = new CDialogWidget(0L, 0, 0, CX_DIALOG, CY_DIALOG);
	this->pPlayerBox->AddWidget(
			new CLabelWidget(0L, X_LABEL, Y_LABEL, CX_LABEL, CY_LABEL,
					F_Message, g_pTheDB->GetMessageText(MID_NewPlayerDialogPrompt)), 1);

	//Name entry field (gets focus).
	this->pNameWidget = new CTextBoxWidget(TAG_NAME, X_TEXTBOX, Y_TEXTBOX,
			CX_TEXTBOX, CY_TEXTBOX, CY_STANDARD_TBOX);
	this->pPlayerBox->AddWidget(this->pNameWidget);

	//Buttons
	this->pPlayerBox->AddWidget(new CButtonWidget(TAG_OK,
			X_OK_BUTTON, Y_BUTTONS, CX_OKAY_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay)));
	this->pPlayerBox->AddWidget(new CButtonWidget(
			TAG_CANCEL, X_CANCEL_BUTTON, Y_BUTTONS, CX_CANCEL_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel)));
	this->pPlayerBox->AddWidget(new CButtonWidget(TAG_IMPORT,
			X_IMPORT_BUTTON, Y_BUTTONS, CX_IMPORT_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Import)));
#ifndef STEAMBUILD
	this->pPlayerBox->AddWidget(new CButtonWidget(TAG_GET_PLAYER_FROM_CLOUD,
			X_CLOUDIMPORTBUTTON, Y_BUTTONS2, CX_CLOUDIMPORTBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_GetPlayerFromCloud)));
#endif

	AddWidget(this->pPlayerBox,true);
	this->pPlayerBox->Center();
	this->pPlayerBox->Hide();

	this->pPlayerBox->SetEnterText(TAG_NAME);

	AddCloudDialog();
}

//*****************************************************************************
CNewPlayerScreen::~CNewPlayerScreen()
//Destructor.
{
}

//*****************************************************************************
void CNewPlayerScreen::Paint(
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

//*****************************************************************************
bool CNewPlayerScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	ShowCursor();
	SetCursor();

	//Need to enter this screen if no players exist yet.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
#ifdef DEV_BUILD
	if (!dwPlayerID) {
		if (Metadata::GetInt(MetaKey::EMBEDMEDIA) == 1) {
			ImportMedia();

			//force quit game
			GoToScreen(SCR_None);
			return false;
		}

		if (ImportQueuedFiles()) {
			GoToScreen(SCR_None);
			return false;
		}
	}
#endif

	return dwPlayerID == 0;
}

//*****************************************************************************
void CNewPlayerScreen::OnBetweenEvents()
//Bring up player selection dialog right away.
//As soon as it closes, return from this screen.
{
	UINT dwTagNo;
	do {
		dwTagNo = this->pPlayerBox->Display();
		switch (dwTagNo)
		{
			case TAG_OK:
				//Create a user with the selected name
				AddPlayer();
				Deactivate();
			break;

			case TAG_CANCEL:
			case TAG_ESCAPE:
				this->bQuitPrompt = true;
				if (ShowYesNoMessage(MID_ReallyQuit) != TAG_NO)
					GoToScreen(SCR_None);
				this->bQuitPrompt = false;
			break;

			case TAG_IMPORT:
			{
				//Ensure highlight room ID list is compiled.
				CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
						g_pTheSM->GetScreen(SCR_Game));
				if (!pGameScreen) break;

				//Import a player data file.
				CIDSet importedPlayerIDs;
				set<WSTRING> importedStyles;
				Import(EXT_PLAYER, importedPlayerIDs, importedStyles);
				if (CDbXML::WasImportSuccessful() && !importedPlayerIDs.empty())
				{
					EnablePlayerSettings(importedPlayerIDs.getFirst());
					Deactivate();
				}
				ASSERT(importedStyles.empty());
			}
			break;

			case TAG_GET_PLAYER_FROM_CLOUD:
			{
				const UINT playerID = ShowGetCloudPlayerDialog();
				if (playerID)
					Deactivate();
			}
			break;
		}
	} while (!IsDeactivating());

	//Perform graphics benchmark test to determine what effects can be enabled.
	if (GetDestScreenType() != SCR_None)
	{
		const UINT wFPS = Benchmark();

		{
			CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(g_pTheDB->GetPlayerID());
			ASSERT(pPlayer);

			const UINT speed_ticks = wFPS / 20;
			const UINT max_speed = Metadata::GetInt(MetaKey::MAX_EYE_CANDY);
			g_pTheBM->eyeCandy = BYTE(speed_ticks > max_speed ? max_speed : speed_ticks);
			pPlayer->Settings.SetVar(Settings::EyeCandy, g_pTheBM->eyeCandy);

			if (wFPS < 10)
			{
				g_pTheBM->bAlpha = false;
				pPlayer->Settings.SetVar(Settings::Alpha, g_pTheBM->bAlpha);
			}

			pPlayer->Update();
			delete pPlayer;
		}

		//Warn user if required screen resolution is not supported.
		if (GetDisplayForDesktopResOfAtLeast(CScreen::CX_SCREEN, CScreen::CY_SCREEN) == -1)
			ShowOkMessage(MID_LowResWarning);
	}
}

//*****************************************************************************
bool CNewPlayerScreen::OnQuit()
//Called when SDL_QUIT event is received.
{
	//Quit immediately when quit is received on this screen.
	GoToScreen(SCR_None);
	return true;
}

//
// Private methods
//

//*****************************************************************************
UINT CNewPlayerScreen::AddPlayer()
//Add a local player to the DB.
//
//Returns: ID of new player added
{
	LOGCONTEXT("CNewPlayerScreen::AddPlayer");

	//Prompt for keyboard configuration.
	UINT wKeyboard = 0;
	const UINT dwAnswer = ShowYesNoMessage(MID_KeyboardConfigurationPrompt, MID_Desktop, MID_Laptop);
	if (dwAnswer == TAG_YES || dwAnswer == TAG_NO)
	{
		CFiles f;
		f.WriteGameProfileString(INISection::Localization, INIKey::Keyboard, dwAnswer == TAG_YES ? "0" : "1");
		if (dwAnswer == TAG_NO)
			wKeyboard = 1;
	}

	//Add player to DB.
	CDbPlayer *pPlayer = g_pTheDB->Players.GetNew();
	pPlayer->NameText = this->pNameWidget->GetText();
	pPlayer->CNetNameText = wszEmpty;
	pPlayer->CNetPasswordText = wszEmpty;
	pPlayer->Settings.SetVar(Settings::PlaySessions, UINT(1));

	for (UINT wIndex = 0; wIndex<InputCommands::DCMD_Count; ++wIndex)
	{
		const SDL_Keycode nKey = COMMANDKEY_ARRAY[wKeyboard][wIndex];
		pPlayer->Settings.SetVar(InputCommands::COMMANDNAME_ARRAY[wIndex], nKey);
	}

	pPlayer->Update();
	const UINT dwPlayerID = pPlayer->dwPlayerID;
	delete pPlayer;
	g_pTheDB->Commit();

	return dwPlayerID;
}

//*****************************************************************************
void CNewPlayerScreen::SetPlayerHold(const UINT dwPlayerID) const
//Set active hold to the last one player was playing.
{
	const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByContinueLatest(dwPlayerID);
	const UINT dwHoldID = g_pTheDB->SavedGames.GetHoldIDofSavedGame(dwSavedGameID);
	if (dwHoldID)
		g_pTheDB->SetHoldID(dwHoldID);
}

//*****************************************************************************
UINT CNewPlayerScreen::Benchmark() const
//Returns: number of full-screen alpha frames that can be drawn in a second
{
	SDL_Surface *pDestSurface = GetDestSurface();

	UINT wFrames = 0;
	const Uint32 dwStart = SDL_GetTicks();
	const Uint32 dwEnd = dwStart + 1000; //1s
	SDL_Surface *pSrcSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, pDestSurface->w, pDestSurface->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	EnableSurfaceBlending(pSrcSurface, 1);	//non-optimized alpha value
	do {
		//Do a dummy redraw of the entire screen.
		SDL_BlitSurface(pSrcSurface, NULL, pDestSurface, NULL);
		PresentRect(pDestSurface);
		++wFrames;
	} while (SDL_GetTicks() < dwEnd);
	SDL_FreeSurface(pSrcSurface);
	return wFrames;
}
