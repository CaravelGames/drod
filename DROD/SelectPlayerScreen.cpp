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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "SelectPlayerScreen.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "GameScreen.h"
#include "SellScreen.h"

#include "../DRODLib/Db.h"
#include "../DRODLib/DbSavedGames.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>

#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

const UINT TAG_LIST_BOX = 1090;
const UINT TAG_CANCEL = 1091;
const UINT TAG_NEWPLAYER = 1092;
const UINT TAG_DELETEPLAYER = 1093;
const UINT TAG_EXPORT = 1094;
const UINT TAG_IMPORT = 1095;
const UINT TAG_EXPORTSAVES = 1096;
const UINT TAG_GET_PLAYER_FROM_CLOUD = 1097;

bool CSelectPlayerScreen::bFirst = true;

//
//Protected methods.
//

//*****************************************************************************
CSelectPlayerScreen::CSelectPlayerScreen() : CDrodScreen(SCR_SelectPlayer)
	, pPlayerBox(NULL)
	, pPlayerListBoxWidget(NULL)
	, pOKButton(NULL), pDeletePlayerButton(NULL)
	, pExportPlayerButton(NULL), pExportPlayerSavesButton(NULL)
	, pPlayerHoldLabel(NULL), pPlayerPositionLabel(NULL)
//Constructor.
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

	static const UINT CX_BOX = 500;

	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 15;

	static const int Y_PROMPT = CY_SPACE;
	static const UINT CY_PROMPT = 40;

	static const int X_LABEL = 15;
	static const UINT CY_LABEL = 30;
	static const UINT X_LISTBOX = 35;
	static const UINT Y_LISTBOX = Y_PROMPT + CY_PROMPT;
	static const UINT CX_LISTBOX = CX_BOX - X_LISTBOX * 2;
	static const UINT CY_LISTBOX = 9*22 + 4; //9 lines

	static const int Y_HOLDLABEL = Y_LISTBOX + CY_LISTBOX + CY_SPACE/2;
	static const int Y_POSITIONLABEL = Y_HOLDLABEL + 30;
	static const int CX_HOLDLABEL = CX_BOX - X_LABEL * 2;
	static const int CX_POSITIONLABEL = CX_HOLDLABEL;

	static const int Y_BUTTONS1 = Y_POSITIONLABEL + 35;
	static const int X_OKBUTTON = 50;
	static const UINT CX_OKBUTTON = 90;
	static const int X_NEWPLAYERBUTTON = X_OKBUTTON + CX_OKBUTTON + CX_SPACE;
	static const UINT CX_NEWPLAYERBUTTON = 145;
	static const int X_DELETEBUTTON = X_NEWPLAYERBUTTON + CX_NEWPLAYERBUTTON + CX_SPACE;
	static const UINT CX_DELETEBUTTON = 145;

	static const int Y_BUTTONS2 = Y_BUTTONS1 + 40;
	static const UINT CX_EXPORTBUTTON = 100;
#ifdef RUSSIAN_BUILD
	static const int X_EXPORTBUTTON = 65;
	static const int X_EXPORTSAVESBUTTON = X_EXPORTBUTTON + CX_EXPORTBUTTON + CX_SPACE;
	static const UINT CX_EXPORTSAVESBUTTON = 255;
#else
	static const int X_EXPORTBUTTON = 85;
	static const int X_EXPORTSAVESBUTTON = X_EXPORTBUTTON + CX_EXPORTBUTTON + CX_SPACE;
	static const UINT CX_EXPORTSAVESBUTTON = 215;
#endif

	static const int Y_BUTTONS3 = Y_BUTTONS2 + 40;
	static const int X_IMPORTBUTTON = 15;
	static const UINT CX_IMPORTBUTTON = 100;
	static const int X_CLOUDIMPORTBUTTON = X_IMPORTBUTTON + CX_IMPORTBUTTON + CX_SPACE;
	static const UINT CX_CLOUDIMPORTBUTTON = 250;
	static const int X_CANCEL = X_CLOUDIMPORTBUTTON + CX_CLOUDIMPORTBUTTON + CX_SPACE;
	static const UINT CX_CANCEL = 100;

	static const UINT CY_BOX = Y_BUTTONS3 + CY_STANDARD_BUTTON + CY_SPACE;

	this->pPlayerBox = new CDialogWidget(0L, 0, 0, CX_BOX, CY_BOX, true);
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_PROMPT, CX_BOX, CY_PROMPT,
					F_Message, g_pTheDB->GetMessageText(MID_SelectPlayerPrompt));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pPlayerBox->AddWidget(pTitle);

	//Player position labels.
	this->pPlayerHoldLabel = new CLabelWidget(0, X_LABEL, Y_HOLDLABEL, CX_HOLDLABEL, CY_LABEL, F_Small, NULL);
	this->pPlayerHoldLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pPlayerBox->AddWidget(this->pPlayerHoldLabel);
	this->pPlayerPositionLabel = new CLabelWidget(0, X_LABEL, Y_POSITIONLABEL, CX_POSITIONLABEL, CY_LABEL, F_Small, NULL);
	this->pPlayerPositionLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pPlayerBox->AddWidget(this->pPlayerPositionLabel);

	//Buttons
	this->pOKButton = new CButtonWidget(
			TAG_OK, X_OKBUTTON, Y_BUTTONS1, CX_OKBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	this->pPlayerBox->AddWidget(this->pOKButton);
	this->pPlayerBox->AddWidget(new CButtonWidget(
			TAG_NEWPLAYER, X_NEWPLAYERBUTTON, Y_BUTTONS1, CX_NEWPLAYERBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_NewPlayer)));
	this->pDeletePlayerButton = new CButtonWidget(
			TAG_DELETEPLAYER, X_DELETEBUTTON, Y_BUTTONS1, CX_DELETEBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_DeletePlayer));
	this->pPlayerBox->AddWidget(this->pDeletePlayerButton);

	this->pExportPlayerButton = new CButtonWidget(
			TAG_EXPORT, X_EXPORTBUTTON, Y_BUTTONS2, CX_EXPORTBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Export));
	this->pPlayerBox->AddWidget(this->pExportPlayerButton);
	this->pExportPlayerSavesButton = new CButtonWidget(
			TAG_EXPORTSAVES, X_EXPORTSAVESBUTTON, Y_BUTTONS2, CX_EXPORTSAVESBUTTON,
			CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_ExportSavedProgress));
	this->pPlayerBox->AddWidget(this->pExportPlayerSavesButton);

	this->pPlayerBox->AddWidget(new CButtonWidget(TAG_IMPORT,
			X_IMPORTBUTTON, Y_BUTTONS3, CX_IMPORTBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Import)));
#ifndef STEAMBUILD
	this->pPlayerBox->AddWidget(new CButtonWidget(TAG_GET_PLAYER_FROM_CLOUD,
			X_CLOUDIMPORTBUTTON, Y_BUTTONS3, CX_CLOUDIMPORTBUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_GetPlayerFromCloud)));
#endif
	this->pPlayerBox->AddWidget(new CButtonWidget(
			TAG_CANCEL, X_CANCEL, Y_BUTTONS3, CX_CANCEL, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel)));

	//Player list.
	this->pPlayerListBoxWidget = new CListBoxWidget(TAG_LIST_BOX,
			X_LISTBOX, Y_LISTBOX, CX_LISTBOX, CY_LISTBOX, true);
	this->pPlayerBox->AddWidget(pPlayerListBoxWidget);

	AddWidget(this->pPlayerBox,true);
	this->pPlayerBox->Center();
	this->pPlayerBox->Hide();

	AddCloudDialog();
}

//*****************************************************************************
CSelectPlayerScreen::~CSelectPlayerScreen()
//Destructor.
{
}

//*****************************************************************************
void CSelectPlayerScreen::Paint(
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
bool CSelectPlayerScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	const bool bFirstTime = CSelectPlayerScreen::bFirst;

	ShowCursor();
	SetCursor();

	//Find out whether any local player records exist.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	if (dwPlayerID)
	{
		//Some players exist.  Select one.
		if (bFirstTime)
		{
			SetPlayerHold(dwPlayerID);

			//Automatically login to previous player selected if option is set.
			string str;
			bool bAutoLogin = false;
			if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::AutoLogin, str))
				if (atoi(str.c_str()) > 0)
					bAutoLogin = true;

			if (!bAutoLogin)
				SelectPlayer();

			CSelectPlayerScreen::bFirst = false;
			return false;  //Don't have to enter screen.
		}
		return true;
	}

	//No players exist -- add one (show player selection dialog).
	SelectPlayer();
	CSelectPlayerScreen::bFirst = false;

	//Can leave this screen now.
	return false;
}

//*****************************************************************************
void CSelectPlayerScreen::OnBetweenEvents()
//Bring up player selection dialog right away.
//As soon as it closes, return from this screen.
{
	SelectPlayer();

	if (!IsDeactivating())
		GoToScreen(SCR_Return);
}

//*****************************************************************************
void CSelectPlayerScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_LIST_BOX:
		{
			//Update the description label.
			const UINT dwPlayerID = GetSelectedItem();
			SetPlayerDesc(dwPlayerID);
		}
		break;

		default: break;
	}
}

//
// Private methods
//

//*****************************************************************************
UINT CSelectPlayerScreen::AddPlayer()
//Add a local player to the DB.
//
//Returns: ID of new player added, else 0L if none
{
	LOGCONTEXT("CSelectPlayerScreen::AddPlayer");
	WSTRING wstrPlayerName;
	const UINT dwTagNo = ShowTextInputMessage(MID_NewPlayerPrompt, wstrPlayerName);

	if (dwTagNo == TAG_OK)
	{
		CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();

		//Add player to DB.
		CDbPlayer *pPlayer = g_pTheDB->Players.GetNew();
		pPlayer->NameText = wstrPlayerName.c_str();
		if (pCurrentPlayer)
		{
			//By default, give new player same settings as current player.
			pPlayer->Settings = pCurrentPlayer->Settings;
			delete pCurrentPlayer;
		}
		pPlayer->Update();
		const UINT dwPlayerID = pPlayer->dwPlayerID;
		delete pPlayer;
		g_pTheDB->Commit();

		return dwPlayerID;
	}

	return 0L;
}

//*****************************************************************************
UINT CSelectPlayerScreen::GetSelectedItem()
//Returns: tag of item selected in the list box widget.
{
	return this->pPlayerListBoxWidget->GetSelectedItem();
}

//*****************************************************************************
void CSelectPlayerScreen::SelectPlayer()
//Prompt the user to select a player profile.
{
	UINT dwPlayerID;
	const UINT dwCurrentPlayerID = dwPlayerID = g_pTheDB->GetPlayerID();

	this->pPlayerBox->Show();
	do {
		SetPlayerID(dwPlayerID);
		if (!dwPlayerID)
			if (CScreen::OnQuit()) //no sell screen
				return;
	} while (!dwPlayerID);
	this->pPlayerBox->Hide();

	if (dwPlayerID != dwCurrentPlayerID)
	{
		//Disable any previous player's game.
		CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_Game));
		ASSERT(pGameScreen);
		pGameScreen->UnloadGame();

		//Set player, and find latest hold player had saved in.
		g_pTheDB->SetPlayerID(dwPlayerID);
		SetPlayerHold(dwPlayerID);
	}
}

//*****************************************************************************
void CSelectPlayerScreen::SetPlayerDesc(const UINT dwPlayerID)
//Sets the labels to display the indicated player's
//current location in the game, based on the latest continue slot.
{
	static const WCHAR wszSignSep[] = { We(':'),We(' '),We(0) };

	if (dwPlayerID)
	{
		const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByContinueLatest(dwPlayerID);
		CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetByID(dwSavedGameID);
		if (pSavedGame)
		{
			ASSERT(pSavedGame->dwPlayerID == dwPlayerID);
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(pSavedGame->dwRoomID, true);
			delete pSavedGame;
			if (pRoom)
			{
				WSTRING wstrRoomPosDesc;
				pRoom->GetLevelPositionDescription(wstrRoomPosDesc);
				CDbLevel *pLevel = g_pTheDB->Levels.GetByID(pRoom->dwLevelID);
				delete pRoom;
				if (pLevel)
				{
					WSTRING wstrLevelPosDesc = (const WCHAR *)pLevel->NameText;
					wstrLevelPosDesc += wszSignSep;
					wstrLevelPosDesc += wstrRoomPosDesc;
					this->pPlayerPositionLabel->SetText(wstrLevelPosDesc.c_str());

					CDbHold *pHold = g_pTheDB->Holds.GetByID(pLevel->dwHoldID, true);
					delete pLevel;
					if (pHold)
					{
						this->pPlayerHoldLabel->SetText(pHold->NameText);
						delete pHold;
						Paint();
						return;
					}
				}
			}
		}
	}

	//If any of the above steps failed, blank the fields.
	this->pPlayerHoldLabel->SetText(NULL);
	this->pPlayerPositionLabel->SetText(NULL);
	Paint();
}

//*****************************************************************************
void CSelectPlayerScreen::SetPlayerHold(const UINT dwPlayerID) const
//Set active hold to the last one player was playing.
{
	const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByContinueLatest(dwPlayerID);
	const UINT dwHoldID = g_pTheDB->SavedGames.GetHoldIDofSavedGame(dwSavedGameID);
	if (!dwHoldID)
		return;
	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
	if (pHold)
	{
		if (pHold->status != CDbHold::Tutorial)
			g_pTheDB->SetHoldID(dwHoldID);
		delete pHold;
	}
}

//*****************************************************************************
void CSelectPlayerScreen::SetPlayerID(
//If this is the first time this screen is entered (on startup)
//and there is only one local player in the DB, then automatically select them.
//
//Otherwise, displays a dialog box with a list box of all players in the DB.
//
//Hitting OK will set the parameter to the selected player's ID and exit.
//Hitting New Player will prompt for a new player name and add them to the DB.
//Hitting Delete Player will delete the selected player upon confirmation.
//
//Params:
	UINT &dwPlayerID)   //(in/out)  ID of selected player on OK.
{
	//Get the local players in the DB.
	PopulatePlayerListBox(this->pPlayerListBoxWidget);

	//Automatically select only player on startup.
	if (CSelectPlayerScreen::bFirst &&
			this->pPlayerListBoxWidget->GetItemCount() == 1)
		return;

	//Select current choice.
	this->pPlayerListBoxWidget->SelectItem(dwPlayerID);
	SetPlayerDesc(dwPlayerID);

	UINT dwTagNo;
	do
	{
		const bool bEnable = pPlayerListBoxWidget->GetItemCount() > 0;
		this->pOKButton->Enable(bEnable);
		this->pDeletePlayerButton->Enable(bEnable);
		this->pExportPlayerButton->Enable(bEnable);
		this->pExportPlayerSavesButton->Enable(bEnable);

		dwTagNo = this->pPlayerBox->Display();
		switch (dwTagNo)
		{
		case TAG_OK:
			//Get selected value.
			dwPlayerID = this->pPlayerListBoxWidget->GetSelectedItem();
			EnablePlayerSettings(dwPlayerID);
			break;
		case TAG_NEWPLAYER:
		{
			//Add new player and select it.
			const UINT dwNewPlayerID = AddPlayer();
			if (dwNewPlayerID)
			{
				dwPlayerID = dwNewPlayerID;
				PopulatePlayerListBox(this->pPlayerListBoxWidget);
				this->pPlayerListBoxWidget->SelectItem(dwPlayerID);
				SetPlayerDesc(dwPlayerID);
			}
			break;
		}
		case TAG_DELETEPLAYER:
			//Delete player on confirmation.
			if (ShowYesNoMessage(MID_DeletePlayerPrompt) == TAG_YES)
			{
				SetCursor(CUR_Wait);
				{
					WSTRING wstr = g_pTheDB->GetMessageText(MID_Deleting);
					wstr += wszSpace;
					wstr += this->pPlayerListBoxWidget->GetSelectedItemText();
					CScreen::ShowStatusMessage(wstr.c_str());
				}

				g_pTheDB->Players.Delete(this->pPlayerListBoxWidget->GetSelectedItem());
				HideStatusMessage();

				PopulatePlayerListBox(this->pPlayerListBoxWidget);
				this->pPlayerListBoxWidget->SelectLine(0);
				dwPlayerID = this->pPlayerListBoxWidget->GetSelectedItem();
				//If no players remain, then reset active player immediately.
				if (!dwPlayerID)
				{
					g_pTheDB->SetPlayerID(0);
					SetPlayerDesc(0);
				}
				SetCursor();
			}
			break;

		case TAG_EXPORT:
		{
			const UINT dwPlayerID = this->pPlayerListBoxWidget->GetSelectedItem();
			if (!dwPlayerID) break;
			CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwPlayerID);
			if (!pPlayer) break;

			//Quick player export if requested.
			bool bQuickExport = false;
			string str;
			if (CFiles::GetGameProfileString(INISection::Customizing, "QuickPlayerExport", str))
				bQuickExport = atoi(str.c_str()) != 0;
			if (bQuickExport && ShowYesNoMessage(MID_ExportPlayerQuickPrompt) == TAG_YES)
				CDbXML::info.bQuickPlayerExport = true;

			//Default filename is player name.
			WSTRING wstrExportFile = (WSTRING)pPlayer->NameText;
			CDrodScreen::callbackContext = wstrExportFile;
			if (ExportSelectFile(MID_SavePlayerPath, wstrExportFile, EXT_PLAYER))
			{
				//Write the player file.
				SetCursor(CUR_Wait);
				Callback(MID_Exporting);
				CDbXML::SetCallback(this);
				const bool bResult = CDbXML::ExportXML(V_Players, dwPlayerID, wstrExportFile.c_str());
				ExportCleanup();
				ShowOkMessage(bResult ? MID_PlayerFileSaved : MID_PlayerFileNotSaved);
			}
			CDrodScreen::callbackContext.resize(0);
			delete pPlayer;
		}
		break;

		case TAG_EXPORTSAVES:
		{
			const UINT dwPlayerID = this->pPlayerListBoxWidget->GetSelectedItem();
			if (!dwPlayerID) break;

			//Compile IDs of this player's saved games.
			//The export will exclude hidden saved game records (e.g. those attached to demos, room tallies, etc.)
			//but will include records for conquering secret rooms and ending holds.
			CDb db;
			db.SavedGames.FilterByPlayer(dwPlayerID);
			db.SavedGames.FindHiddens(true);
			CIDSet savedGameIDs, allSavedGameIDs = db.SavedGames.GetIDs();
			for (CIDSet::const_iterator id=allSavedGameIDs.begin();
					id!=allSavedGameIDs.end(); ++id)
			{
				CDbSavedGame *pSavedGame = db.SavedGames.GetByID(*id, true);
				if (!pSavedGame->bIsHidden ||
						pSavedGame->eType == ST_SecretConquered ||
						pSavedGame->eType == ST_EndHold ||
						pSavedGame->eType == ST_HoldMastered)
					savedGameIDs += *id;
				delete pSavedGame;
			}
			if (savedGameIDs.empty()) break;

			CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwPlayerID);
			if (!pPlayer) break;

			//Quick player export if requested.
			bool bQuickExport = false;
			string str;
			if (CFiles::GetGameProfileString(INISection::Customizing, "QuickPlayerExport", str))
				bQuickExport = atoi(str.c_str()) != 0;
			if (bQuickExport && ShowYesNoMessage(MID_ExportPlayerQuickPrompt) == TAG_YES)
				CDbXML::info.bQuickPlayerExport = true;

			//Default filename is player name.
			WSTRING wstrExportFile = (WSTRING)pPlayer->NameText;
			CDrodScreen::callbackContext = wstrExportFile;
			wstrExportFile += wszSpace;
			wstrExportFile += g_pTheDB->GetMessageText(MID_Saves);
			if (ExportSelectFile(MID_SavePlayerPath, wstrExportFile, EXT_PLAYER))
			{
				//Write the player saves file.
				SetCursor(CUR_Wait);
				Callback(MID_Exporting);
				CDbXML::SetCallback(this);

				const bool bResult = CDbXML::ExportXML(V_SavedGames, savedGameIDs,
					wstrExportFile.c_str());
				ExportCleanup();
				ShowOkMessage(bResult ? MID_SavedGamesSaved : MID_PlayerFileNotSaved);
			}
			CDrodScreen::callbackContext.resize(0);
			delete pPlayer;
		}
		break;

		case TAG_IMPORT:
		{
			//Ensure highlight room ID list is compiled.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			if (!pGameScreen) break;

			//When importing player data, if it's only saved games for merging,
			//make sure saved games are merged into the currently highlighted
			//player, even if it hasn't been selected yet.
			const UINT dwTempPlayerID = g_pTheDB->GetPlayerID();
			g_pTheDB->SetPlayerID(this->pPlayerListBoxWidget->GetSelectedItem());

			//Import a player data file.
			CIDSet importedPlayerIDs;
			set<WSTRING> importedStyles;
			const MESSAGE_ID res = Import(EXT_PLAYER, importedPlayerIDs, importedStyles);
			if (res && CDbXML::WasImportSuccessful())
			{
				if (importedPlayerIDs.empty()) //imported saved games only
					ShowOkMessage(MID_SavedGamesOnlyImported);
				else
				{
					//Select the imported player.
					dwPlayerID = importedPlayerIDs.getFirst();
					//Update in case a player was added.
					PopulatePlayerListBox(this->pPlayerListBoxWidget);
					this->pPlayerListBoxWidget->SelectItem(dwPlayerID);
					SetPlayerDesc(dwPlayerID);

					//If the active player was just reimported, unload their game in
					//progress so any imported continue save is used to play next time.
					if (dwTempPlayerID == dwPlayerID)
					{
						CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
								g_pTheSM->GetScreen(SCR_Game));
						ASSERT(pGameScreen);
						pGameScreen->UnloadGame();
					}
				}
				ASSERT(importedStyles.empty());
			}

			g_pTheDB->SetPlayerID(dwTempPlayerID);
		}
		break;

		case TAG_GET_PLAYER_FROM_CLOUD:
			dwPlayerID = ShowGetCloudPlayerDialog();
			if (dwPlayerID)
			{
				//Update in case a player was added.
				PopulatePlayerListBox(this->pPlayerListBoxWidget);
				this->pPlayerListBoxWidget->SelectItem(dwPlayerID);
				SetPlayerDesc(dwPlayerID);
			}
		break;

		case TAG_CANCEL:
		case TAG_ESCAPE:
		case TAG_QUIT:
			return;
		}
	} while (dwTagNo != TAG_OK || !dwPlayerID);
}

//*****************************************************************************
void CSelectPlayerScreen::PopulatePlayerListBox(
//Puts levels of current hold into list box.
//
//Params:
	CListBoxWidget *pPlayerListBoxWidget)  //(in/out)
const
{
	BEGIN_DBREFCOUNT_CHECK;
	pPlayerListBoxWidget->Clear();

	//Get holds in DB.
	{
		CDb db;
		db.Players.FilterByLocal();
		CDbPlayer *pPlayer = db.Players.GetFirst(true);
		while (pPlayer)
		{
			pPlayerListBoxWidget->AddItem(pPlayer->dwPlayerID, pPlayer->NameText);
			delete pPlayer;
			pPlayer = db.Players.GetNext();
		}
	}
	END_DBREFCOUNT_CHECK;
}
