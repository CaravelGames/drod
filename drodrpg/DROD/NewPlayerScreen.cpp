// $Id: NewPlayerScreen.cpp 9758 2011-10-28 13:37:06Z mrimer $

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
#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/TextBoxWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

#ifdef DEV_BUILD //comment out to embed media immediately
#	define EMBED_STYLES
#endif

const UINT TAG_CANCEL = 1091L;
const UINT TAG_IMPORT = 1092L;
const UINT TAG_NAME = 1093L;

//
//Protected methods.
//

//*****************************************************************************
CNewPlayerScreen::CNewPlayerScreen() : CDrodScreen(SCR_NewPlayer)
	, pPlayerBox(NULL)
	, pOKButton(NULL), pImportPlayerButton(NULL)
	, pNameWidget(NULL)
//Constructor.
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

	static const UINT CX_DIALOG = 450;
	static const UINT CY_DIALOG = 310;

	static const int X_LABEL = 30;
	static const int Y_LABEL = 12;
	static const UINT CX_LABEL = CX_DIALOG - X_LABEL*2;
	static const UINT CY_LABEL = 190;

	static const int Y_BUTTONS = CY_DIALOG - 55;
	static const int X_OK_BUTTON = 25;
	static const UINT CX_OKAY_BUTTON = 90;
	static const int X_CANCEL_BUTTON = X_OK_BUTTON + CX_OKAY_BUTTON + 25;
	static const UINT CX_CANCEL_BUTTON = 100;
	static const UINT CX_IMPORT_BUTTON = 100;	
	static const int X_IMPORT_BUTTON = CX_DIALOG - CX_IMPORT_BUTTON - 25;

	static const int X_TEXTBOX = X_OK_BUTTON;
	static const UINT CY_TEXTBOX = CY_STANDARD_TBOX;
	static const int Y_TEXTBOX = Y_BUTTONS - CY_TEXTBOX - 15;
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
	this->pOKButton = new CButtonWidget(
			TAG_OK, X_OK_BUTTON, Y_BUTTONS, CX_OKAY_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	this->pPlayerBox->AddWidget(this->pOKButton);
	this->pPlayerBox->AddWidget(new CButtonWidget(
			TAG_CANCEL, X_CANCEL_BUTTON, Y_BUTTONS, CX_CANCEL_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel)));
	this->pImportPlayerButton = new CButtonWidget(
			TAG_IMPORT, X_IMPORT_BUTTON, Y_BUTTONS, CX_IMPORT_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Import));
	this->pPlayerBox->AddWidget(this->pImportPlayerButton);

	AddWidget(this->pPlayerBox,true);
	this->pPlayerBox->Center();
	this->pPlayerBox->Hide();

	this->pPlayerBox->SetEnterText(TAG_NAME);
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
#if defined(BETA) && defined(CARAVELBUILD) && defined(EMBED_STYLES)
	if (!dwPlayerID)
		ImportHoldMedia(NULL);
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
		}
	} while (!IsDeactivating());

	//Perform graphics benchmark test to determine what effects can be enabled.
	if (GetDestScreenType() != SCR_None)
	{
		const UINT wFPS = Benchmark();
		if (wFPS < 20)
		{
			CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(g_pTheDB->GetPlayerID());
			ASSERT(pPlayer);

			g_pTheBM->eyeCandy = 0;
			pPlayer->Settings.SetVar("EyeCandy", false);

			if (wFPS < 10)
			{
				g_pTheBM->bAlpha = false;
				pPlayer->Settings.SetVar("Alpha", false);
			}

			pPlayer->Update();
			delete pPlayer;
		}

		//Warn user if required screen resolution is not supported.
		UINT wX, wY;
		GetHighestScreenRes(wX,wY);
		if (wX < (UINT)CScreen::CX_SCREEN || wY < (UINT)CScreen::CY_SCREEN)
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
//Returns: ID of new player added, else 0L if none
{
	LOGCONTEXT("CNewPlayerScreen::AddPlayer");

	//Prompt for keyboard configuration.
	const UINT dwAnswer = ShowYesNoMessage(MID_KeyboardConfigurationPrompt, MID_Desktop, MID_Laptop);
   if (dwAnswer == TAG_YES || dwAnswer == TAG_NO)
	{
		CFiles f;
		f.WriteGameProfileString("Localization", "Keyboard", dwAnswer == TAG_YES ? "0" : "1");
	}

	//Add player to DB.
	CDbPlayer *pPlayer = g_pTheDB->Players.GetNew();
	pPlayer->NameText = this->pNameWidget->GetText();
	pPlayer->CNetNameText = wszEmpty;
	pPlayer->CNetPasswordText = wszEmpty;
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
	SDL_SetAlpha(pSrcSurface, SDL_SRCALPHA, 1);	//non-optimized alpha value
	do {
		//Do a dummy redraw of the entire screen.
		SDL_BlitSurface(pSrcSurface, NULL, pDestSurface, NULL);
		SDL_UpdateRect(pDestSurface, 0, 0, 0, 0);
		++wFrames;
	} while (SDL_GetTicks() < dwEnd);
	SDL_FreeSurface(pSrcSurface);
	return wFrames;
}

