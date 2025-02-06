// $Id: RestoreScreen.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005, 2008 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "RestoreScreen.h"
#include "BrowserScreen.h"
#include "DemosScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodScreenManager.h"
#include "GameScreen.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ScrollableWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/CueEvents.h"
#include "../DRODLib/DbXML.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>

//Widget tags.
const UINT TAG_POSITION_LABEL = 1011;
const UINT TAG_ROOM_WIDGET = 1012;
const UINT TAG_STATS_LABEL = 1013;

const UINT TAG_SAVE_LBOX = 1020;
const UINT TAG_SORT_LBOX = 1021;

const UINT TAG_HOLD_EXPLORED = 1030;
const UINT TAG_HOLD_SECRETS = 1031;

const UINT TAG_RESTORE = 1091;
const UINT TAG_CANCEL = 1092;
const UINT TAG_HELP = 1093;
const UINT TAG_EXPORT = 1094;
const UINT TAG_IMPORT = 1095;
const UINT TAG_RENAME = 1096;
const UINT TAG_SCOREPOINTS = 1097;
const UINT TAG_SCOREPOINTS_LIST = 1098;

enum GameSort
{
	GS_CHRONO,
	GS_ALPHA
};

//
//Protected methods.
//

//*****************************************************************************************
CRestoreScreen::CRestoreScreen()
	: CDrodScreen(SCR_Restore)
	, dwSelectedSavedGameID(0L), dwLastGameID(0L)
//	, wConqueredRooms(0)
	, gameSort(GS_CHRONO)
	, bHoldConquered(false), bResetWidgets(true)
	, pCurrentRestoreGame(NULL)
	, pRoomWidget(NULL)
	, pScaledRoomWidget(NULL)
	, pMapWidget(NULL)
	, pSaveListBoxWidget(NULL)
	, pScorepointsDialog(NULL)
	, pScorepointsListBox(NULL)
//Constructor.
{
	SetKeyRepeat(66);

	this->imageFilenames.push_back(string("Background"));

	static const UINT CX_SPACE = 12;
	static const UINT CY_SPACE = 12;
	static const UINT CY_BUTTON = CY_STANDARD_BUTTON;

	static const UINT CY_TITLE = 52;
	static const UINT CY_TITLE_SPACE = 15;
	static const int Y_TITLE = CY_TITLE_SPACE;

	static const int Y_STATS_LABEL = Y_TITLE + CY_TITLE;
	static const UINT CY_STATS_LABEL = 45;

	//Mini-room widget has strict proportions and its dimensions will define 
	//placement of most everything else.
	static const int Y_POSITION_LABEL = Y_STATS_LABEL + CY_STATS_LABEL + CY_SPACE;
	static const UINT CY_POSITION_LABEL = 25;
	static const int Y_MINIROOM = Y_POSITION_LABEL + CY_POSITION_LABEL;
	const UINT CY_MINIROOM = this->h - Y_MINIROOM - CY_BUTTON - CY_SPACE * 2 - 6;
	//Width of mini-room must be proportional to regular room display.
	static const UINT CX_MINIROOM = CY_MINIROOM * CDrodBitmapManager::CX_ROOM /
			CDrodBitmapManager::CY_ROOM;
	const int X_MINIROOM = this->w - CX_SPACE - CX_MINIROOM;

	static const int X_STATS_LABEL = X_MINIROOM;
	static const UINT CX_STATS_LABEL = CX_MINIROOM;
	static const int X_POSITION_LABEL = X_MINIROOM;
	static const UINT CX_POSITION_LABEL = CX_MINIROOM;

	//Buttons.
	static const UINT CX_RESTORE_BUTTON = 90;
	static const UINT CX_RENAME_BUTTON = 90;
	static const UINT CX_CANCEL_BUTTON = 85;
	static const UINT CX_EXPORT_BUTTON = 90;
	static const UINT CX_IMPORT_BUTTON = 90;
	static const UINT CX_HELP_BUTTON = 80;
	static const UINT CX_SCOREPOINT_BUTTON = 100;
	const int X_RESTORE_BUTTON = X_MINIROOM;
	const int X_RENAME_BUTTON = X_RESTORE_BUTTON + CX_RESTORE_BUTTON + CX_SPACE/2;
	static const int X_EXPORT_BUTTON = X_RENAME_BUTTON + CX_RENAME_BUTTON + CX_SPACE/2;
	static const int X_IMPORT_BUTTON = X_EXPORT_BUTTON + CX_EXPORT_BUTTON + CX_SPACE/2;
	const int X_HELP_BUTTON = X_IMPORT_BUTTON + CX_IMPORT_BUTTON + CX_SPACE/2;
	const int X_CANCEL_BUTTON = X_HELP_BUTTON + CX_HELP_BUTTON + CX_SPACE/2;
	const int X_SCOREPOINT_BUTTON = X_RESTORE_BUTTON - CX_SCOREPOINT_BUTTON - CX_SPACE / 2;
	static const UINT CY_RESTORE_BUTTON = CY_BUTTON;
	const int Y_RESTORE_BUTTON = this->h - CY_SPACE - CY_RESTORE_BUTTON;
	const int Y_RENAME_BUTTON = Y_RESTORE_BUTTON;
	const int Y_CANCEL_BUTTON = Y_RESTORE_BUTTON;
	const int Y_EXPORT_BUTTON = Y_RESTORE_BUTTON;
	const int Y_IMPORT_BUTTON = Y_RESTORE_BUTTON;
	const int Y_HELP_BUTTON = Y_RESTORE_BUTTON;
	const int Y_SCOREPOINT_BUTTON = Y_RESTORE_BUTTON;

	//Saved games list.
	const UINT CX_MAP = this->w - CX_SPACE - CX_MINIROOM - CX_SPACE - CX_SPACE;

	static const int X_CHOOSE_SAVE_LABEL = CX_SPACE;
	static const int Y_CHOOSE_SAVE_LABEL = Y_TITLE + CY_TITLE + 22;
	static const UINT CX_CHOOSE_SAVE_LABEL = CX_MAP;
	static const UINT CY_CHOOSE_SAVE_LABEL = CY_BUTTON;
	static const int X_SAVE_LBOX = X_CHOOSE_SAVE_LABEL;
	static const int Y_SAVE_LBOX = Y_CHOOSE_SAVE_LABEL + CY_CHOOSE_SAVE_LABEL + 1;
	static const UINT CX_SAVE_LBOX = CX_MAP;
	static const UINT CY_SAVE_LBOX = 13*22 + 4; //13 items;

	static const UINT CX_SORT_LIST = 60;
	static const UINT CY_SORT_LIST = 2*22 + 9; // 53;
	static const int X_SORT_LIST = X_SAVE_LBOX + CX_SAVE_LBOX - CX_SORT_LIST;
	static const int Y_SORT_LIST = Y_TITLE + CY_TITLE;

	static const UINT CX_SORT_LABEL = 60;
	static const int X_SORT_LABEL = X_SORT_LIST - CX_SORT_LABEL;
	static const int Y_SORT_LABEL = Y_SORT_LIST;
	static const UINT CY_SORT_LABEL = CY_BUTTON;

	static const int X_MAP = CX_SPACE;
	const int Y_MAP = Y_SAVE_LBOX + CY_SAVE_LBOX + CY_SPACE;
	const UINT CY_MAP = Y_RESTORE_BUTTON - Y_MAP - CY_SPACE - 6;

	static const int X_HOLD_EXPLORED_LABEL = X_MAP;
	static const UINT CX_HOLD_EXPLORED_LABEL = 140;
	static const int Y_HOLD_EXPLORED_LABEL = Y_MAP + CY_MAP;
	const UINT CY_HOLD_EXPLORED_LABEL = this->h - Y_HOLD_EXPLORED_LABEL;
	static const int X_HOLD_EXPLORED = X_HOLD_EXPLORED_LABEL + CX_HOLD_EXPLORED_LABEL;
	static const UINT CX_HOLD_EXPLORED = 50;
	static const int Y_HOLD_EXPLORED = Y_HOLD_EXPLORED_LABEL;
	const UINT CY_HOLD_EXPLORED = CY_HOLD_EXPLORED_LABEL;

	static const int X_HOLD_SECRET_LABEL = X_HOLD_EXPLORED + CX_HOLD_EXPLORED;
	static const UINT CX_HOLD_SECRET_LABEL = X_MAP + CX_MAP - X_HOLD_SECRET_LABEL;
	static const int Y_HOLD_SECRET_LABEL = Y_HOLD_EXPLORED_LABEL;
	const UINT CY_HOLD_SECRET_LABEL = CY_HOLD_EXPLORED_LABEL;

	CButtonWidget *pButton;

	//Title.
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_TITLE,
			this->w, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_RestoreGame));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pTitle);

	//Restore, cancel and help buttons.
	pButton = new CButtonWidget(TAG_RESTORE, X_RESTORE_BUTTON, Y_RESTORE_BUTTON, 
			CX_RESTORE_BUTTON, CY_RESTORE_BUTTON, g_pTheDB->GetMessageText(MID_Restore));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_RENAME, X_RENAME_BUTTON, Y_RENAME_BUTTON,
		CX_RENAME_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Rename));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXPORT, X_EXPORT_BUTTON, Y_EXPORT_BUTTON,
			CX_EXPORT_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Export));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_IMPORT, X_IMPORT_BUTTON, Y_IMPORT_BUTTON,
			CX_IMPORT_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Import));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_HELP, X_HELP_BUTTON, Y_HELP_BUTTON, 
			CX_HELP_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pButton);
	AddHotkey(SDLK_F1,TAG_HELP);

	pButton = new CButtonWidget(TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON,
			CX_CANCEL_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pButton);

	//Saved game selection area.
	AddWidget(new CLabelWidget(0, X_CHOOSE_SAVE_LABEL, Y_CHOOSE_SAVE_LABEL, 
				CX_CHOOSE_SAVE_LABEL, CY_CHOOSE_SAVE_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_ChooseSavedGame)));
	this->pSaveListBoxWidget = new CListBoxWidget(TAG_SAVE_LBOX,
			X_SAVE_LBOX, Y_SAVE_LBOX, CX_SAVE_LBOX, CY_SAVE_LBOX,
			false, false, true);
	AddWidget(this->pSaveListBoxWidget);

	//Save game sort list box.
	AddWidget(new CLabelWidget(0L, X_SORT_LABEL, Y_SORT_LABEL, CX_SORT_LABEL,
		CY_SORT_LABEL, F_Small, g_pTheDB->GetMessageText(MID_SaveGameSort)));
	CListBoxWidget* pSortList = new CListBoxWidget(TAG_SORT_LBOX,
		X_SORT_LIST, Y_SORT_LIST, CX_SORT_LIST, CY_SORT_LIST);
	pSortList->AddItem(GS_CHRONO, g_pTheDB->GetMessageText(MID_GameSortChronological));
	pSortList->AddItem(GS_ALPHA, g_pTheDB->GetMessageText(MID_GameSortAlphabetical));
	pSortList->SelectLine(0);
	AddWidget(pSortList);

	//Level map.
	CScrollableWidget *pScrollingMap = new CScrollableWidget(0, X_MAP, Y_MAP,
			CX_MAP, CY_MAP);
	AddWidget(pScrollingMap);
	this->pMapWidget = new CMapWidget(TAG_MAP, 0, 0,
			CDrodBitmapManager::DISPLAY_COLS * MAPSIZE_MULTIPLIER,
			CDrodBitmapManager::DISPLAY_ROWS * MAPSIZE_MULTIPLIER,
			NULL,	MAPSIZE_MULTIPLIER);
	this->pMapWidget->Disable(); //don't allow selection change
	pScrollingMap->AddWidget(this->pMapWidget);

	//Game stats.
	AddWidget(new CLabelWidget(0, X_HOLD_EXPLORED_LABEL, Y_HOLD_EXPLORED_LABEL,
				CX_HOLD_EXPLORED_LABEL, CY_HOLD_EXPLORED_LABEL, F_Small,
				g_pTheDB->GetMessageText(MID_Complete)));
	AddWidget(new CLabelWidget(TAG_HOLD_EXPLORED, X_HOLD_EXPLORED, Y_HOLD_EXPLORED,
				CX_HOLD_EXPLORED, CY_HOLD_EXPLORED, F_Small, wszQuestionMark));
	AddWidget(new CLabelWidget(TAG_HOLD_SECRETS, X_HOLD_SECRET_LABEL, Y_HOLD_SECRET_LABEL,
				CX_HOLD_SECRET_LABEL, CY_HOLD_SECRET_LABEL, F_Small,
				g_pTheDB->GetMessageText(MID_SecretsFound)));

	AddWidget(new CLabelWidget(TAG_STATS_LABEL, X_STATS_LABEL, Y_STATS_LABEL,
				CX_STATS_LABEL, CY_STATS_LABEL, F_Small, wszEmpty));

	//Room location.
	AddWidget(new CLabelWidget(TAG_POSITION_LABEL, X_POSITION_LABEL, Y_POSITION_LABEL, 
				CX_POSITION_LABEL, CY_POSITION_LABEL, F_Small, wszEmpty));

	//Room display.
	this->pScaledRoomWidget = new CScalerWidget(TAG_ROOM_WIDGET, X_MINIROOM, Y_MINIROOM, 
			CX_MINIROOM, CY_MINIROOM, false);
	AddWidget(this->pScaledRoomWidget);
	this->pRoomWidget = new CRoomWidget(0L, 0, 0, CDrodBitmapManager::CX_ROOM,
			CDrodBitmapManager::CY_ROOM);
	this->pRoomWidget->SetAnimateMoves(false);
	this->pScaledRoomWidget->AddScaledWidget(this->pRoomWidget);

	//Scorepoints button and dialog
	pButton = new CButtonWidget(TAG_SCOREPOINTS, X_SCOREPOINT_BUTTON, Y_SCOREPOINT_BUTTON,
		CX_SCOREPOINT_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_Scorepoints));
	AddWidget(pButton);

	const UINT CX_SCOREPOINTS_DIALOG = 510;
	const UINT CY_SCOREPOINTS_DIALOG = 510;

	const int CX_SCOREPOINTS_OKAY = 100;
	static const UINT CY_SCOREPOINTS_OKAY = CY_STANDARD_BUTTON;
	static const int X_SCOREPOINTS_OKAY = (CX_SCOREPOINTS_DIALOG - CX_SCOREPOINTS_OKAY) / 2;
	static const int Y_SCOREPOINTS_OKAY = CY_SCOREPOINTS_DIALOG - CY_SCOREPOINTS_OKAY - CY_SPACE - 5;

	this->pScorepointsDialog = new CDialogWidget(0L, 0, 0, CX_SCOREPOINTS_DIALOG, CY_SCOREPOINTS_DIALOG);
	this->pScorepointsDialog->Hide();
	AddWidget(this->pScorepointsDialog);
	this->pScorepointsDialog->Center();

	pButton = new CButtonWidget(TAG_OK,
		X_SCOREPOINTS_OKAY, Y_SCOREPOINTS_OKAY,
		CX_SCOREPOINTS_OKAY, CY_SCOREPOINTS_OKAY, g_pTheDB->GetMessageText(MID_Okay));
	this->pScorepointsDialog->AddWidget(pButton);

	const int Y_SCOREPOINTS_TITLE = CY_SPACE;
	const UINT CY_SCOREPOINTS_TITLE = 35;
	const int X_SCOREPOINTS_FRAME = CX_SPACE + 5;
	const int Y_SCOREPOINTS_FRAME = Y_SCOREPOINTS_TITLE + CY_SCOREPOINTS_TITLE + CY_SPACE;
	const UINT CX_SCOREPOINTS_FRAME = CY_SCOREPOINTS_DIALOG - 2 * X_SCOREPOINTS_FRAME;
	const UINT CY_SCOREPOINTS_FRAME = Y_SCOREPOINTS_OKAY - Y_SCOREPOINTS_FRAME - CY_SPACE;

	CLabelWidget* pLabel = new CLabelWidget(0, CX_SPACE, Y_SCOREPOINTS_TITLE,
		CX_SCOREPOINTS_DIALOG - 2 * CX_SPACE, CY_SCOREPOINTS_TITLE,
		FONTLIB::F_Message, g_pTheDB->GetMessageText(MID_Scorepoints));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pScorepointsDialog->AddWidget(pLabel);

	CFrameWidget* pFrame = new CFrameWidget(0, X_SCOREPOINTS_FRAME, Y_SCOREPOINTS_FRAME,
		CX_SCOREPOINTS_FRAME, CY_SCOREPOINTS_FRAME, NULL);
	this->pScorepointsDialog->AddWidget(pFrame);

	this->pScorepointsListBox = new CListBoxWidget(TAG_SCOREPOINTS_LIST, CX_SPACE, CY_SPACE,
		CX_SCOREPOINTS_FRAME - 2 * CX_SPACE, CY_SCOREPOINTS_FRAME - 2 * CY_SPACE);
	pFrame->AddWidget(this->pScorepointsListBox);
}

//*****************************************************************************
void CRestoreScreen::ClearState()
{
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = NULL;
}

void CRestoreScreen::InitScorepointQuery()
{
	//Prep set of rooms to begin querying incrementally for challenge data.
	this->scorepointScanRoomIDs.clear();
	this->scorepointVarMap.clear();

	const UINT holdID = g_pTheDB->GetHoldID();
	CDbHold* pHold = g_pTheDB->Holds.GetByID(holdID);
	if (pHold) {
		CDbHolds::GetScriptScorepointRefs(pHold, this->scorepointVarMap, this->scorepointScanRoomIDs);
		delete pHold;
	}

	//CDbPlayer* pPlayer = g_pTheDB->GetCurrentPlayer();
	//ASSERT(pPlayer);
	//this->completedScorepoints = pPlayer->challenges.get(holdID);
	//delete pPlayer;
}

//******************************************************************************
CRestoreScreen::~CRestoreScreen()
{
	ClearState();
}

//******************************************************************************
bool CRestoreScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	if (!this->bResetWidgets)
		this->bResetWidgets = true;	//reset next time, by default
	else
	{
		//Get widgets and current games ready.
		if (!SetWidgets())
			return false;

		SelectWidget(TAG_SAVE_LBOX);
	}

	return true;
}

//
//Private methods.
//

//******************************************************************************
void CRestoreScreen::OnClick(
//Called when widget receives a click event.
//
//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_ESCAPE:
		case TAG_CANCEL:
			GoToScreen(SCR_Return);
		break;

		case TAG_RESTORE:
			RestoreGame();
		break;

		case TAG_RENAME:
			RenameSaveGame(this->pSaveListBoxWidget->GetSelectedItem());
		break;

		case TAG_EXPORT:
		{
			CIDSet savedGameIDs = this->pSaveListBoxWidget->GetSelectedItems();
			const bool bResult = SaveGamesToDisk(savedGameIDs);
			if (bResult)
				ShowOkMessage(MID_SavedGamesSaved);
		}
		break;

		case TAG_IMPORT:
		{
			CIDSet importedSavedGameIDs;
			set<WSTRING> importedStyles;
			Import(EXT_SAVE, importedSavedGameIDs, importedStyles, true);
			if (CDbXML::WasImportSuccessful() && !importedSavedGameIDs.empty())
			{
				//Synch widgets.
				GetSaves();
				PopulateListBoxFromSavedGames();

				//Select imported saves.
				if (SetWidgets())
				{
					SelectFirstWidget();
					const UINT listedID = this->pSaveListBoxWidget->HasAnyKey(importedSavedGameIDs);

					this->pSaveListBoxWidget->SelectItems(importedSavedGameIDs);
					ChooseSavedGame(listedID);

					Paint();
				}
				ShowOkMessage(MID_ImportSuccessful);
			}
			ASSERT(importedStyles.empty());
		}
		break;

		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("restore.html");
			GoToScreen(SCR_Browser);

			this->bResetWidgets = false;	//keep current room active on return
		break;

		case TAG_SCOREPOINTS:
			DisplayScorepointsDialog();
			Paint();
		break;

		default:
		break;
	}
}

//*****************************************************************************
void CRestoreScreen::OnDoubleClick(const UINT dwTagNo)
{
	switch (dwTagNo)
	{
		case TAG_SAVE_LBOX:
			if (this->pSaveListBoxWidget->ClickedSelection())
				RestoreGame();
		break;
		case TAG_ROOM_WIDGET:
			RestoreGame();
		break;
	}
}

//*****************************************************************************
void CRestoreScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	CScreen::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
/*
		case SDLK_F6:
		{
			if (!this->pCurrentRestoreGame) break;
			CScreen *pScreen = g_pTheSM->GetScreen(SCR_Demos);
			if (!pScreen)
			{
				ShowOkMessage(MID_CouldNotLoadResources);
				break;
			}
			CDemosScreen *pDemosScreen = DYN_CAST(CDemosScreen*, CScreen*, pScreen);
			ASSERT(pDemosScreen);

			pDemosScreen->ShowRoom(this->pCurrentRestoreGame->pRoom->dwRoomID);
			GoToScreen(SCR_Demos);

			this->bResetWidgets = false;	//keep current room active on return
		}
		break;
*/
		case SDLK_DELETE:
		{
			//Delete selected saved games.
			CIDSet savedGameIDs = this->pSaveListBoxWidget->GetSelectedItems();
			if (!savedGameIDs.empty())
			{
				if (ShowYesNoMessage(MID_DeleteSavedGamesPrompt) == TAG_YES)
				{
					for (CIDSet::const_iterator savedGame = savedGameIDs.begin();
							savedGame != savedGameIDs.end(); ++savedGame)
						g_pTheDB->SavedGames.Delete(*savedGame);

					//Synch widgets.
					if (SetWidgets())
					{
						SelectFirstWidget();
						Paint();
					}
				}
			}
		}
		break;

		default: break;
	}
}

//*****************************************************************************
void CRestoreScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_SAVE_LBOX:
			ChooseSavedGame(this->pSaveListBoxWidget->GetSelectedItem());
			Paint();
		break;
		case TAG_SORT_LBOX:
		{
			CListBoxWidget* pSortList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_SORT_LBOX));
			this->gameSort = pSortList->GetSelectedItem();

			PopulateListBoxFromSavedGames();
			if (this->dwSelectedSavedGameID)
				this->pSaveListBoxWidget->SelectItem(this->dwSelectedSavedGameID);
			Paint();
		}
		break;
		default: break;
	}
}

//*****************************************************************************
void CRestoreScreen::ChooseSavedGame(
//Loads the selected save game and updates display.
//
//Params:
	const UINT dwSavedGameID) //(in) Saved game to use.
{
	ASSERT(dwSavedGameID);

	//Load the saved game.
	CCueEvents Ignored;
	CCurrentGame *pNewGame = g_pTheDB->GetSavedCurrentGame(dwSavedGameID, Ignored, false,
			true); //don't save anything to DB here
	if (pNewGame)
	{
		//Switch current game over to new one from saved game.
		delete this->pCurrentRestoreGame;
		this->pCurrentRestoreGame = pNewGame;
		this->dwSelectedSavedGameID = dwSavedGameID;
	}

	ShowSave();
}

void CRestoreScreen::DisplayScorepointsDialog()
{
	if (this->pScorepointsListBox->IsEmpty()) {
		SetCursor(CUR_Wait);
		PopulateScorepoints(this->pScorepointsListBox);
		SetCursor();
	}

	this->pScorepointsDialog->Display();
}

//*****************************************************************************
bool CRestoreScreen::SetWidgets()
//Set up widgets and data used by them when user first arrives at restore
//screen.  Should only be called by SetForActivate().
//
//Returns:
//True if successful, false if not.
{
	//Update level selection list box.
	GetSaves();
	PopulateListBoxFromSavedGames();

	//Delete any existing current game for this screen.
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = NULL;

	this->pScorepointsListBox->Clear();

	//Load current room and level from game screen if it has a game loaded.
	CCueEvents Ignored;
	this->dwLastGameID = this->pSaveListBoxWidget->GetKeyAtLine(0);
	this->dwSelectedSavedGameID = this->dwLastGameID;
	if (this->dwLastGameID)
	{
		this->pCurrentRestoreGame = g_pTheDB->GetSavedCurrentGame(
				this->dwLastGameID, Ignored, false,
				true); //don't save anything to DB during playback
	}
	if (!this->pCurrentRestoreGame)
	{
		//No saved games exist.
		//Create a new continue save at the start of the hold so something shows up.
		this->pCurrentRestoreGame = g_pTheDB->GetNewCurrentGame(g_pTheDB->GetHoldID(), Ignored);
		if (!this->pCurrentRestoreGame)
			return false;
		this->pCurrentRestoreGame->SaveToContinue();
		this->dwSelectedSavedGameID = this->pCurrentRestoreGame->dwSavedGameID;

		GetSaves();
		PopulateListBoxFromSavedGames();
	}

	//The latest saved game is being displayed.
	this->pSaveListBoxWidget->SelectLine(0);

	ShowSave();
	InitScorepointQuery();

	return true;
}

//*****************************************************************************
void CRestoreScreen::ShowSave()
//Show information pertaining to this saved game.
{
	UpdateWidgets();

	//Update level and room location label.
	WSTRING wstrDesc;
	if (this->pCurrentRestoreGame)
	{
		wstrDesc += this->pCurrentRestoreGame->pLevel->NameText;
		wstrDesc += wszColon;
		wstrDesc += wszSpace;
		this->pCurrentRestoreGame->pRoom->GetLevelPositionDescription(wstrDesc);
	}
	CLabelWidget *pRoomLabel = DYN_CAST(CLabelWidget*, CWidget*,
			GetWidget(TAG_POSITION_LABEL));
	pRoomLabel->SetText(wstrDesc.c_str());

	ShowSaveStats();

	//Can restore if there is an active saved game.
	CWidget *pWidget = GetWidget(TAG_RESTORE);
	pWidget->Enable(this->pCurrentRestoreGame != NULL);

	//Can restore if there is an active saved game.
	pWidget = GetWidget(TAG_EXPORT);
	pWidget->Enable(this->pSaveListBoxWidget->GetSelectedItem() != 0);
}

//*****************************************************************************
void CRestoreScreen::ShowSaveStats()
//Show player's level completion stats.
//More information is displayed after hold is completed.
{
	CLabelWidget *pStatsLabel =
			DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_STATS_LABEL));
	if (!this->pCurrentRestoreGame)
	{
		pStatsLabel->SetText(wszEmpty);
		pStatsLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_HOLD_EXPLORED));
		pStatsLabel->SetText(wszEmpty);
		pStatsLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_HOLD_SECRETS));
		pStatsLabel->SetText(wszEmpty);
		return;
	}

	//Show modified stats correctly.
	PlayerStats st = this->pCurrentRestoreGame->pPlayer->st; //make temp copy
	st.ATK = this->pCurrentRestoreGame->getPlayerATK();
	st.DEF = this->pCurrentRestoreGame->getPlayerDEF();
	st.totalTime += this->pCurrentRestoreGame->Commands.GetTimeElapsed();
	WSTRING wstr = getStatsText(st, this->pCurrentRestoreGame);
	pStatsLabel->SetText(wstr.c_str());

	//Get percentage of rooms explored in hold.
	CIDSet playerRoomsExplored;
	HoldStats roomsInHold;
	g_pTheDB->Holds.GetRooms(g_pTheDB->GetHoldID(), roomsInHold);
	playerRoomsExplored = this->pCurrentRestoreGame->GetExploredRooms();
	playerRoomsExplored += this->pCurrentRestoreGame->dwRoomID; //consider current room explored

	//Display percent of rooms in level explored.
	WCHAR num[16];
	pStatsLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_HOLD_EXPLORED));
	wstr = _itoW(playerRoomsExplored.size(), num, 10);

	if (this->bHoldConquered)
	{
		//When hold has been finished, reveal much of the hold has been explored in this game.
		wstr += wszSpace;
		wstr += wszLeftParen;
		wstr += _itoW(static_cast<int>(playerRoomsExplored.size() * 100.0 /
				(float)roomsInHold.rooms.size()), num, 10);
		wstr += wszPercent;
		wstr += wszRightParen;
	}
	pStatsLabel->SetText(wstr.c_str());

	//Display how many secret rooms have been found.
	pStatsLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_HOLD_SECRETS));
	UINT secretRoomsFound=0;
	for (CIDSet::const_iterator room = playerRoomsExplored.begin();
			room != playerRoomsExplored.end(); ++room)
	{
		if (roomsInHold.secretRooms.has(*room))
			++secretRoomsFound;
	}
	if (!secretRoomsFound)
	{
		pStatsLabel->SetText(wszEmpty);
	} else {
		wstr = g_pTheDB->GetMessageText(MID_SecretsFound);
		wstr += wszSpace;

		wstr += _itoW(secretRoomsFound, num, 10);
		if (this->bHoldConquered)
		{
			//When hold is finished, reveal how many secret rooms are in this hold.
			wstr += wszForwardSlash;
			wstr += _itoW(roomsInHold.secretRooms.size(), num, 10);
		}
		pStatsLabel->SetText(wstr.c_str());
	}
}

//*****************************************************************************
void CRestoreScreen::UpdateWidgets()
//Update the map and room widgets to reflect the current game.
{
	//Update the map and room widgets with new current game.
	if (IsCursorVisible())
		SetCursor(CUR_Wait);
	VERIFY(this->pRoomWidget->LoadFromCurrentGame(this->pCurrentRestoreGame));
	CCueEvents Ignored;
	this->pRoomWidget->DisplayPersistingImageOverlays(Ignored);

	VERIFY(this->pMapWidget->LoadFromCurrentGame(this->pCurrentRestoreGame));
	if (IsCursorVisible())
		SetCursor();
}

//*****************************************************************************
void CRestoreScreen::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
void CRestoreScreen::GetSaves()
{
	this->saves.clear();

	//Active hold.
	const UINT holdID = g_pTheDB->GetHoldID();
	if (!holdID) { ASSERT(!"Failed to retrieve hold."); return; } //Probably corrupted DB.

	//Get set of all player's saved games in this hold.
	CDb db;
	db.SavedGames.FilterByHold(holdID);
	db.SavedGames.FilterByPlayer(g_pTheDB->GetPlayerID());
	db.SavedGames.FindHiddens(true);
	const CIDSet savedGameIDs = db.SavedGames.GetIDs();

	this->saves = db.SavedGames.GetSaveInfo(savedGameIDs);

	//Also maintain flag tracking whether hold has been conquered.
	this->bHoldConquered = (g_pTheDB->SavedGames.FindByEndHold(holdID) != 0);
}

//*****************************************************************************
//Return: an ordering of saved games, according to 'gameSort' strategy
SORTED_SAVES CRestoreScreen::GetSortedSaves() const
{
	map<UINT, UINT> idSort; //id -> position
	if (this->gameSort == GS_ALPHA) {
		//Build map of sorted save names to ids.
		multimap<WSTRING, UINT, WSTRINGicmp> saveNames;
		for (vector<SAVE_INFO>::const_iterator it = this->saves.begin(); it != this->saves.end(); ++it) {
			saveNames.insert(std::make_pair(it->name, it->saveID));
		}

		//Provide the position of each save ID.
		UINT count = 0;
		for (multimap<WSTRING, UINT>::const_iterator it2 = saveNames.begin(); it2 != saveNames.end(); ++it2) {
			idSort[it2->second] = count++;
		}
	}

	SORTED_SAVES sortedSaves;
	for (vector<SAVE_INFO>::const_iterator it = this->saves.begin(); it != this->saves.end(); ++it) {
		const SAVE_INFO& save = *it;

		UINT sortVal;
		switch (this->gameSort) {
			case GS_ALPHA:
				sortVal = idSort[save.saveID];
				break;
			case GS_CHRONO:
			default:
				sortVal = UINT(-1) - save.timestamp; //newest first
				break;
		}

		sortedSaves.insert(std::make_pair(sortVal, save));
	}
	return sortedSaves;
}

//*****************************************************************************
void CRestoreScreen::PopulateListBoxFromSavedGames()
//Put saved games for this player into list box.
//Determine whether this hold has been completed.
{
	static const SDL_Color Purple = {96, 0, 96, 0};

	this->pSaveListBoxWidget->Clear();

	//Display saved games in sorted order.
	const SORTED_SAVES sortedSaves = GetSortedSaves();
	for (SORTED_SAVES::const_iterator saveIter = sortedSaves.begin(); saveIter != sortedSaves.end(); ++saveIter)
	{
		const SAVE_INFO& save = saveIter->second;
		WSTRING saveText;
		if (!save.bCanValidate) //notify pre-1.1 players that their saved games can't be validated
		{
			saveText += wszQuestionMark;
			saveText += wszSpace;
		}
		saveText += save.name;
		saveText += wszSpace;
		const SDL_Color *pColor = NULL;
		bool bDisplay = true;
		switch (save.type)
		{
			case ST_Autosave:
			{
				pColor = &Maroon;
				saveText += wszLeftParen;
				saveText += g_pTheDB->GetMessageText(MID_Autosave);
				saveText += wszRightParen;
				saveText += wszSpace;
			}
			break;
			case ST_Continue:
			{
				pColor = &Purple;
				saveText += wszLeftParen;
				saveText += g_pTheDB->GetMessageText(MID_ContinueSave);
				saveText += wszRightParen;
				saveText += wszSpace;
			}
			break;
			case ST_Manual:
				//no special info
			break;
			case ST_Quicksave:
			{
				pColor = &Purple;
				saveText += wszLeftParen;
				saveText += g_pTheDB->GetMessageText(MID_Quicksave);
				saveText += wszRightParen;
				saveText += wszSpace;
			}
			break;
			case ST_ScoreCheckpoint:
			default:
				bDisplay = false; //don't display
			break;
		}
		if (!bDisplay)
			continue;
		CDate date((time_t)save.timestamp);
		date.GetLocalFormattedText(DF_SHORT_DATE | DF_SHORT_TIME, saveText);

		const UINT lineNo = this->pSaveListBoxWidget->AddItem(save.saveID, saveText.c_str());
		if (pColor)
			this->pSaveListBoxWidget->SetItemColorAtLine(lineNo, *pColor);
	}
}

void CRestoreScreen::PopulateScorepoints(CListBoxWidget* pListBoxWidget)
{
	//Complete scanning any remaining rooms for scorepoints.
	for (CIDSet::const_iterator roomIt = this->scorepointScanRoomIDs.begin();
		roomIt != this->scorepointScanRoomIDs.end(); ++roomIt)
	{
		CDbHolds::GetScriptCommandRefsForRoom(*roomIt, NULL, true, this->scorepointVarMap);
	}
	this->scorepointScanRoomIDs.clear();

	if (this->scorepointVarMap.empty()) {
		pListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(MID_None), true);
	}

	CDb db;
	pListBoxWidget->SelectLine(0);

	CDbPlayer* pPlayer = g_pTheDB->GetCurrentPlayer();
	CDbHold* pHold = this->pCurrentRestoreGame->pHold;
	db.HighScores.FilterByPlayer(pPlayer->dwPlayerID);
	db.HighScores.FilterByHold(pHold->dwHoldID);

#ifndef ENABLE_CHEATS
	// Check hold authorship
	// Not required if cheats are active
	bool bIsAuthor = (pPlayer->dwPlayerID == pHold->dwPlayerID);
	delete pPlayer;
#endif

	for (CDbHolds::VARCOORDMAP::const_iterator vars = this->scorepointVarMap.begin();
		vars != this->scorepointVarMap.end(); ++vars)
	{
		//Display info for this variable.
		WSTRING scorepointName = vars->first;
		bool hasScore = (db.HighScores.HasScorepoint(scorepointName));
#ifndef ENABLE_CHEATS
		if (!(hasScore || bIsAuthor)) {
			pListBoxWidget->AddItem(0, L"???", true);
		}	else
#endif
		{
			if (hasScore) {
				UINT scoreID = db.HighScores.GetIDForScorepoint(scorepointName);
				ASSERT(scoreID);
				CDbLocalHighScore* pHighScore = db.HighScores.GetByID(scoreID);
				scorepointName += wszSpace;
				scorepointName += wszHyphen;
				scorepointName += wszSpace;
				scorepointName += std::to_wstring(pHighScore->score);
				delete pHighScore;
			}

			pListBoxWidget->AddItem(0, scorepointName.c_str(), !hasScore);
		}
	}
}

//*****************************************************************************
void CRestoreScreen::RenameSaveGame(const UINT saveID)
//User renames currently selected level.
{
	if (!saveID) return;

	WSTRING wstr = wszQuestionMark;
	for (vector<SAVE_INFO>::const_iterator it = this->saves.begin(); it != this->saves.end(); ++it)
	{
		if (it->saveID == saveID) {
			wstr = it->name;
			break;
		}
	}

	const UINT dwAnswerTagNo = ShowTextInputMessage(MID_DescribeSave, wstr);
	if (dwAnswerTagNo == TAG_OK)
	{
		if (CDbSavedGames::RenameSavedGame(saveID, wstr)) {
			GetSaves();
			PopulateListBoxFromSavedGames();
			this->pSaveListBoxWidget->SelectItem(this->dwSelectedSavedGameID);
			Paint();
		}
	}
}

//*****************************************************************************
void CRestoreScreen::RestoreGame()
{
	if (!this->dwSelectedSavedGameID)
		return;

	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	pGameScreen->LoadSavedGame(this->dwSelectedSavedGameID);
	if (pGameScreen->ShouldShowLevelStart())
		GoToScreen(SCR_LevelStart);
	else
		GoToScreen(SCR_Game);
}
