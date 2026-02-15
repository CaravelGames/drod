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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
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
#include "WorldMapWidget.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ScalerWidget.h>
#include <FrontEndLib/ScrollableWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/CueEvents.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>

#include <sstream>

//Widget tags.
const UINT TAG_ROOM_START = 1010;
const UINT TAG_ROOM_WIDGET = 1011;
const UINT TAG_CHOOSE_ROOM_LABEL = 1012;
const UINT TAG_CHOOSE_POS_LABEL = 1013;
const UINT TAG_MAP_SCROLLER = 1014;

const UINT TAG_LEVEL_START = 1020;

const UINT TAG_HOLD_START = 1021;
const UINT TAG_LEVEL_LBOX = 1022;
const UINT TAG_POSITION_LABEL = 1023;

const UINT TAG_LEVEL_EXPLORED = 1030;
const UINT TAG_LEVEL_SECRETS = 1031;
const UINT TAG_SAVED_GAME_STATS = 1032;
const UINT TAG_EXPLORED_LABEL = 1033;

const UINT TAG_WORLD_MAP = 1040;
const UINT TAG_SCALED_WORLD_MAP = 1041;

const UINT TAG_FURTHEST_SAVE = 1050;

const UINT TAG_RESTORE = 1091;
const UINT TAG_CANCEL = 1092;
const UINT TAG_HELP = 1093;

const UINT TAG_CHALLENGES = 1094;
const UINT TAG_CHALLENGES_LIST = 1095;

const UINT TAG_EXPORT_HOLD_SAVES = 1096;

//Reserve 2000 to (2000+room size) for checkpoint tags.
const UINT TAG_CHECKPOINT = 2000;
#define IS_CHECKPOINT_TAG(t) ((t) >= 2000 && (t) < (2000+(38*32)))

//
//Protected methods.
//

//*****************************************************************************************
CRestoreScreen::CRestoreScreen()
	: CDrodScreen(SCR_Restore)
	, dwSelectedSavedGameID(0L), dwLastGameID(0L)
	, wConqueredRooms(0)
	, bHoldConquered(false), bResetWidgets(true)
	, pCurrentRestoreGame(NULL)
	, pRoomWidget(NULL)
	, pScaledRoomWidget(NULL)
	, pMapWidget(NULL)
	, pLevelListBoxWidget(NULL)
	, pWorldMapWidget(NULL)
	, pScaledWorldMapWidget(NULL)
	, pChallengesDialog(NULL)
	, pChallengesListBox(NULL)
	, pChallengesCountLabel(NULL)
//Constructor.
{
	SetKeyRepeat(66);

	this->imageFilenames.push_back(string("Background"));

	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 10;
	static const UINT CY_TITLE = CY_LABEL_FONT_TITLE;

#ifdef RUSSIAN_BUILD
	static const UINT CX_RESTORE_BUTTON = 140;
	static const UINT CX_ROOM_START = 160;
	static const int Y_TITLE = 12;
#else
	static const UINT CX_RESTORE_BUTTON = 110;
	static const UINT CX_ROOM_START = 130;
	static const int Y_TITLE = Y_TITLE_LABEL_CENTER_DARK_BAR;
#endif

	static const UINT CY_RESTORE_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CX_CANCEL_BUTTON = 110;
	static const UINT CY_CANCEL_BUTTON = CY_RESTORE_BUTTON;
	static const UINT CX_HELP_BUTTON = CX_CANCEL_BUTTON;
	static const UINT CY_HELP_BUTTON = CY_RESTORE_BUTTON;
	static const UINT CX_EXPORT_SAVES_BUTTON = 120;
	static const UINT CY_EXPORT_SAVES_BUTTON = CY_RESTORE_BUTTON;
	const int X_CANCEL_BUTTON = this->w / 2;
	const int X_RESTORE_BUTTON = X_CANCEL_BUTTON - CX_RESTORE_BUTTON - CX_SPACE;
	const int X_HELP_BUTTON = X_CANCEL_BUTTON + CX_CANCEL_BUTTON + CX_SPACE;
	const int X_EXPORT_SAVES_BUTTON = X_HELP_BUTTON + CX_HELP_BUTTON + CX_SPACE;
	const int Y_RESTORE_BUTTON = this->h - CY_SPACE - CY_RESTORE_BUTTON;
	const int Y_CANCEL_BUTTON = Y_RESTORE_BUTTON;
	const int Y_HELP_BUTTON = Y_RESTORE_BUTTON;
	const int Y_EXPORT_SAVES_BUTTON = Y_RESTORE_BUTTON;

	//Mini-room widget has strict proportions and its dimensions will define
	//placement of most everything else.
	static const int Y_CHOOSE_POS_LABEL = Y_TITLE + CY_TITLE + Y_TITLE;
	static const UINT CY_CHOOSE_POS_LABEL = CY_STANDARD_BUTTON;
	static const int Y_POSITION_LABEL = Y_CHOOSE_POS_LABEL + CY_CHOOSE_POS_LABEL;
	static const UINT CY_POSITION_LABEL = 25;
	static const int Y_MINIROOM = Y_POSITION_LABEL + CY_POSITION_LABEL;
	const UINT CY_MINIROOM = this->h - Y_MINIROOM - CY_STANDARD_BUTTON - CY_SPACE * 2 - 6;
	//Width of mini-room must be proportional to regular room display.
	static const UINT CX_MINIROOM = CY_MINIROOM * CDrodBitmapManager::CX_ROOM /
			CDrodBitmapManager::CY_ROOM;
	const int X_MINIROOM = this->w - CX_SPACE - CX_MINIROOM;

	static const int X_CHOOSE_POS_LABEL = X_MINIROOM;
	static const UINT CX_CHOOSE_POS_LABEL = 250;
	const int X_ROOM_START = this->w - CX_SPACE - CX_ROOM_START - 1;
	static const int Y_ROOM_START = Y_CHOOSE_POS_LABEL;
	static const UINT CY_ROOM_START = CY_STANDARD_BUTTON;
	static const int X_POSITION_LABEL = X_MINIROOM;
	static const UINT CX_POSITION_LABEL = CX_MINIROOM;

	static const UINT CX_FURTHEST_SAVE = 140;
	static const int X_FURTHEST_SAVE = X_ROOM_START - 2*CX_SPACE - CX_FURTHEST_SAVE;
	static const int Y_FURTHEST_SAVE = Y_ROOM_START;
	static const UINT CY_FURTHEST_SAVE = CY_STANDARD_BUTTON;

	const UINT CX_MAP = this->w - CX_SPACE - CX_MINIROOM - CX_SPACE - CX_SPACE;
	const UINT CY_MAP = CY_MINIROOM * CX_MAP / CX_MINIROOM;
	static const int X_MAP = CX_SPACE;
	const int Y_MAP = Y_RESTORE_BUTTON - CY_MAP - CY_SPACE - 6;
	static const int X_CHOOSE_ROOM_LABEL = X_MAP;
	static const UINT CX_CHOOSE_ROOM_LABEL = 160;
	static const int CY_CHOOSE_ROOM_LABEL = CY_STANDARD_BUTTON;
	const int Y_CHOOSE_ROOM_LABEL = Y_MAP - CY_CHOOSE_ROOM_LABEL - 1;
	static const UINT CX_LEVEL_START = CX_ROOM_START;
	const int X_LEVEL_START = X_MAP + CX_MAP - CX_LEVEL_START - 2;
	const int Y_LEVEL_START = Y_CHOOSE_ROOM_LABEL;
	static const int CY_LEVEL_START = CY_CHOOSE_ROOM_LABEL;

	static const int X_CHOOSE_LEVEL_LABEL = CX_SPACE;
	static const int Y_CHOOSE_LEVEL_LABEL = Y_CHOOSE_POS_LABEL;
	static const UINT CX_CHOOSE_LEVEL_LABEL = 160;
	static const UINT CY_CHOOSE_LEVEL_LABEL = CY_STANDARD_BUTTON;
	static const UINT CX_GAME_START = CX_ROOM_START;
	static const UINT CY_GAME_START = CY_CHOOSE_LEVEL_LABEL;
	static const int X_GAME_START = X_MAP + CX_MAP - CX_GAME_START - 2;
	static const int Y_GAME_START = Y_CHOOSE_LEVEL_LABEL;
	static const int X_LEVEL_LBOX = X_CHOOSE_LEVEL_LABEL;
	static const int Y_LEVEL_LBOX = Y_CHOOSE_LEVEL_LABEL + CY_CHOOSE_LEVEL_LABEL + 1;
	static const UINT CX_LEVEL_LBOX = CX_MAP;
	static const UINT CY_LEVEL_LBOX = Y_CHOOSE_ROOM_LABEL - Y_CHOOSE_LEVEL_LABEL -
			CY_CHOOSE_LEVEL_LABEL - CY_SPACE - 1;

	static const int X_LEVEL_EXPLORED_LABEL = X_MAP;
	static const UINT CX_LEVEL_EXPLORED_LABEL = 80;
	static const int Y_LEVEL_EXPLORED_LABEL = Y_MAP + CY_MAP;
	const UINT CY_LEVEL_EXPLORED_LABEL = 25;
	static const int X_LEVEL_EXPLORED = X_LEVEL_EXPLORED_LABEL + CX_LEVEL_EXPLORED_LABEL;
	static const UINT CX_LEVEL_EXPLORED = 50;
	static const int Y_LEVEL_EXPLORED = Y_LEVEL_EXPLORED_LABEL;
	const UINT CY_LEVEL_EXPLORED = CY_LEVEL_EXPLORED_LABEL;

	static const int X_LEVEL_SECRET_LABEL = X_LEVEL_EXPLORED + CX_LEVEL_EXPLORED;
	static const UINT CX_LEVEL_SECRET_LABEL = X_MAP + CX_MAP - X_LEVEL_SECRET_LABEL;
	static const int Y_LEVEL_SECRET_LABEL = Y_LEVEL_EXPLORED_LABEL;
	const UINT CY_LEVEL_SECRET_LABEL = CY_LEVEL_EXPLORED_LABEL;

	static const int X_HOLD_STATS_LABEL = X_LEVEL_EXPLORED_LABEL;
	static const UINT CX_HOLD_STATS_LABEL = CX_MAP;
	static const int Y_HOLD_STATS_LABEL = Y_LEVEL_EXPLORED_LABEL + CY_LEVEL_EXPLORED;
	const UINT CY_HOLD_STATS_LABEL = 25;

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

	pButton = new CButtonWidget(TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON,
				CX_CANCEL_BUTTON, CY_CANCEL_BUTTON, g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_HELP, X_HELP_BUTTON, Y_HELP_BUTTON,
				CX_HELP_BUTTON, CY_HELP_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pButton);
	AddHotkey(SDLK_F1,TAG_HELP);

	pButton = new CButtonWidget(TAG_EXPORT_HOLD_SAVES, X_EXPORT_SAVES_BUTTON, Y_EXPORT_SAVES_BUTTON,
		CX_EXPORT_SAVES_BUTTON, CY_EXPORT_SAVES_BUTTON, g_pTheDB->GetMessageText(MID_ExportSaves));
	AddWidget(pButton);

	//Level selection area.
	AddWidget(new CLabelWidget(0L, X_CHOOSE_LEVEL_LABEL, Y_CHOOSE_LEVEL_LABEL,
				CX_CHOOSE_LEVEL_LABEL, CY_CHOOSE_LEVEL_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_ChooseLevel)));
	pButton = new CButtonWidget(TAG_HOLD_START, X_GAME_START, Y_GAME_START,
				CX_GAME_START, CY_GAME_START, g_pTheDB->GetMessageText(MID_HoldStart));
	AddWidget(pButton);

	this->pLevelListBoxWidget = new CListBoxWidget(TAG_LEVEL_LBOX,
			X_LEVEL_LBOX, Y_LEVEL_LBOX, CX_LEVEL_LBOX, CY_LEVEL_LBOX);
	AddWidget(this->pLevelListBoxWidget);
	this->pLevelListBoxWidget->SetAllowFiltering(true);

	//Room selection area.
	AddWidget(new CLabelWidget(TAG_CHOOSE_ROOM_LABEL, X_CHOOSE_ROOM_LABEL, Y_CHOOSE_ROOM_LABEL,
				CX_CHOOSE_ROOM_LABEL, CY_CHOOSE_ROOM_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_ChooseRoom)));
	pButton = new CButtonWidget(TAG_LEVEL_START, X_LEVEL_START, Y_LEVEL_START,
				CX_LEVEL_START, CY_LEVEL_START, g_pTheDB->GetMessageText(MID_LevelStart));
	AddWidget(pButton);

	CScrollableWidget *pScrollingMap = new CScrollableWidget(TAG_MAP_SCROLLER, X_MAP, Y_MAP,
			CX_MAP, CY_MAP);
	AddWidget(pScrollingMap);
	this->pMapWidget = new CMapWidget(TAG_MAP, 0, 0,
			CDrodBitmapManager::DISPLAY_COLS, CDrodBitmapManager::DISPLAY_ROWS, NULL);
	pScrollingMap->AddWidget(this->pMapWidget);

	//Level stats.
	AddWidget(new CLabelWidget(TAG_EXPLORED_LABEL, X_LEVEL_EXPLORED_LABEL, Y_LEVEL_EXPLORED_LABEL,
				CX_LEVEL_EXPLORED_LABEL, CY_LEVEL_EXPLORED_LABEL, F_Small,
				g_pTheDB->GetMessageText(MID_Complete)));
	AddWidget(new CLabelWidget(TAG_LEVEL_EXPLORED, X_LEVEL_EXPLORED, Y_LEVEL_EXPLORED,
				CX_LEVEL_EXPLORED, CY_LEVEL_EXPLORED, F_Small, wszQuestionMark));
	AddWidget(new CLabelWidget(TAG_LEVEL_SECRETS, X_LEVEL_SECRET_LABEL, Y_LEVEL_SECRET_LABEL,
				CX_LEVEL_SECRET_LABEL, CY_LEVEL_SECRET_LABEL, F_Small,
				g_pTheDB->GetMessageText(MID_SecretsFound)));
	AddWidget(new CLabelWidget(TAG_SAVED_GAME_STATS, X_HOLD_STATS_LABEL, Y_HOLD_STATS_LABEL,
				CX_HOLD_STATS_LABEL, CY_HOLD_STATS_LABEL, F_Small, wszQuestionMark));

	//Position selection area.
	AddWidget(new CLabelWidget(TAG_CHOOSE_POS_LABEL, X_CHOOSE_POS_LABEL, Y_CHOOSE_POS_LABEL,
				CX_CHOOSE_POS_LABEL, CY_CHOOSE_POS_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_ChoosePosition)));
	pButton = new CButtonWidget(TAG_FURTHEST_SAVE, X_FURTHEST_SAVE, Y_FURTHEST_SAVE,
				CX_FURTHEST_SAVE, CY_FURTHEST_SAVE, g_pTheDB->GetMessageText(MID_RestoreToFurthestSave));
	AddWidget(pButton);
	pButton = new CButtonWidget(TAG_ROOM_START, X_ROOM_START, Y_ROOM_START,
				CX_ROOM_START, CY_ROOM_START, g_pTheDB->GetMessageText(MID_RoomStart));
	AddWidget(pButton);

	AddWidget(new CLabelWidget(TAG_POSITION_LABEL, X_POSITION_LABEL, Y_POSITION_LABEL,
				CX_POSITION_LABEL, CY_POSITION_LABEL, F_Small, wszEmpty));

	this->pScaledRoomWidget = new CScalerWidget(TAG_ROOM_WIDGET, X_MINIROOM, Y_MINIROOM,
			CX_MINIROOM, CY_MINIROOM, false);
	AddWidget(this->pScaledRoomWidget);
	this->pRoomWidget = new CRoomWidget(0L, 0, 0, CDrodBitmapManager::CX_ROOM,
			CDrodBitmapManager::CY_ROOM);
	this->pScaledRoomWidget->AddScaledWidget(this->pRoomWidget);

	//World map.
	const UINT CX_WORLDMAP = CX_MINIROOM;
	const UINT CY_WORLDMAP = CX_WORLDMAP * CScreen::CY_SCREEN / CScreen::CX_SCREEN;
	this->pScaledWorldMapWidget = new CScalerWidget(TAG_SCALED_WORLD_MAP, X_MINIROOM, Y_MINIROOM,
			CX_WORLDMAP, CY_WORLDMAP, false);
	AddWidget(this->pScaledWorldMapWidget);
	this->pWorldMapWidget = new CWorldMapWidget(TAG_WORLD_MAP,
			0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
	this->pScaledWorldMapWidget->AddScaledWidget(this->pWorldMapWidget);

	//Challenges button and dialog.
	static const UINT CX_CHALLENGES_BUTTON = 120;
	const int X_CHALLENGES_BUTTON = X_MINIROOM + CX_MINIROOM - CX_CHALLENGES_BUTTON;
	const int Y_CHALLENGES_BUTTON = Y_RESTORE_BUTTON;
	const UINT CY_CHALLENGES_BUTTON = CY_RESTORE_BUTTON;

	const UINT CX_CHALLENGES_DIALOG = 700;
	const UINT CY_CHALLENGES_DIALOG = 510;

	const int CX_CHALLENGES_OKAY = 100;
	static const UINT CY_CHALLENGES_OKAY = CY_STANDARD_BUTTON;
	static const int X_CHALLENGES_OKAY = CX_CHALLENGES_DIALOG - CX_CHALLENGES_OKAY - CX_SPACE;
	static const int Y_CHALLENGES_OKAY = CY_CHALLENGES_DIALOG - CY_CHALLENGES_OKAY - CY_SPACE - 5;

	pButton = new CButtonWidget(TAG_CHALLENGES, X_CHALLENGES_BUTTON, Y_CHALLENGES_BUTTON,
				CX_CHALLENGES_BUTTON, CY_CHALLENGES_BUTTON, g_pTheDB->GetMessageText(MID_Challenges));
	AddWidget(pButton);

	this->pChallengesDialog = new CDialogWidget(0L, 0, 0, CX_CHALLENGES_DIALOG, CY_CHALLENGES_DIALOG);
	this->pChallengesDialog->Hide();
	AddWidget(this->pChallengesDialog);
	this->pChallengesDialog->Center();

	const int Y_CHALLENGES_TITLE = CY_SPACE;
	const UINT CY_CHALLENGES_TITLE = 35;
	const int X_CHALLENGES_FRAME = CX_SPACE + 5;
	const int Y_CHALLENGES_FRAME = Y_CHALLENGES_TITLE + CY_CHALLENGES_TITLE + CY_SPACE;
	const UINT CX_CHALLENGES_FRAME = CX_CHALLENGES_DIALOG - 2*X_CHALLENGES_FRAME;
	const UINT CY_CHALLENGES_FRAME = Y_CHALLENGES_OKAY - Y_CHALLENGES_FRAME - CY_SPACE;

	CLabelWidget *pLabel = new CLabelWidget(0, CX_SPACE, Y_CHALLENGES_TITLE,
			CX_CHALLENGES_DIALOG - 2*CX_SPACE, CY_CHALLENGES_TITLE,
			FONTLIB::F_Message, g_pTheDB->GetMessageText(MID_CompletedChallenges));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pChallengesDialog->AddWidget(pLabel);

	this->pChallengesCountLabel = new CLabelWidget(0, CX_SPACE, Y_CHALLENGES_OKAY,
		CX_CHALLENGES_DIALOG - 2 * CX_SPACE, CY_CHALLENGES_TITLE,
		FONTLIB::F_Message, WS("0/0"));
	this->pChallengesCountLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pChallengesDialog->AddWidget(this->pChallengesCountLabel);

	pButton = new CButtonWidget(TAG_OK,
			X_CHALLENGES_OKAY, Y_CHALLENGES_OKAY,
			CX_CHALLENGES_OKAY, CY_CHALLENGES_OKAY, g_pTheDB->GetMessageText(MID_Okay));
	this->pChallengesDialog->AddWidget(pButton);

	CFrameWidget *pFrame = new CFrameWidget(0, X_CHALLENGES_FRAME, Y_CHALLENGES_FRAME,
			CX_CHALLENGES_FRAME, CY_CHALLENGES_FRAME, NULL);
	this->pChallengesDialog->AddWidget(pFrame);

	this->pChallengesListBox = new CListBoxWidget(TAG_CHALLENGES_LIST, CX_SPACE, CY_SPACE,
			CX_CHALLENGES_FRAME - 2*CX_SPACE, CY_CHALLENGES_FRAME - 2*CY_SPACE);
	pFrame->AddWidget(this->pChallengesListBox);
}

//*****************************************************************************
void CRestoreScreen::ClearState()
{
	this->Checkpoints.clear();
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = NULL;
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

		SelectFirstWidget(false);
	}

	return true;
}

//
//Private methods.
//

//******************************************************************************
void CRestoreScreen::OnBetweenEvents()
{
	if (this->pScaledWorldMapWidget->IsVisible()) {
		this->pWorldMapWidget->UpdateAnimation();
		this->pScaledWorldMapWidget->Paint();
	} else {
		this->pRoomWidget->DirtyRoom();
		this->pScaledRoomWidget->Paint();
	}

	const Uint32 MAX_TIME_TO_SPEND = 50; //ms
	const Uint32 dwStop = SDL_GetTicks() + MAX_TIME_TO_SPEND;

	//Scan some rooms for scripted challenges.
	while (!this->challengeScanRoomIDs.empty() && SDL_GetTicks() < dwStop) {
		const UINT roomID = this->challengeScanRoomIDs.getFirst();
		CDbHolds::GetScriptCommandRefsForRoom(roomID, NULL, true, this->challengeVarMap);
		this->challengeScanRoomIDs.erase(roomID);
	}

	//Scan some challenge demos.
	while (!this->challengeDemoIDs.empty() && SDL_GetTicks() < dwStop) {
		const UINT demoID = this->challengeDemoIDs.getFirst();
		AddChallengesCompletedInDemo(demoID);
		this->challengeDemoIDs.erase(demoID);
	}
}

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

		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("restore.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_HOLD_START:
		{
			//For currently selected hold.
			this->pLevelListBoxWidget->SelectItem(
					this->pCurrentRestoreGame->pHold->dwLevelID);
			GetLevelStats(this->pCurrentRestoreGame->pHold->dwLevelID);
			ChooseLevelStart(this->pCurrentRestoreGame->pHold->dwLevelID);
			Paint();
		}
		break;

		case TAG_LEVEL_START:
			ChooseLevelStart(this->pCurrentRestoreGame->pLevel->dwLevelID);
			Paint();
		break;

		case TAG_ROOM_START:
			ChooseRoomStart(this->pCurrentRestoreGame->pRoom->dwRoomX,
					this->pCurrentRestoreGame->pRoom->dwRoomY);
			Paint();
		break;

		case TAG_FURTHEST_SAVE:
			ChooseFurthestSave();
			SetWidgetDisplay();
			Paint();
		break;

		case TAG_CHALLENGES:
			DisplayChallengesDialog();
			Paint();
		break;

		case TAG_EXPORT_HOLD_SAVES:
			ExportSaves(g_pTheDB->GetPlayerID(), g_pTheDB->GetHoldID(), true);
		break;

		default:
			if (IS_CHECKPOINT_TAG(dwTagNo))
			{
				ChooseCheckpoint(dwTagNo);
				SelectWidget(TAG_RESTORE);
				this->pScaledRoomWidget->Paint();
				this->pMapWidget->RequestPaint();
			}
		break;
	}  //...switch (dwActionNo)
}

//*****************************************************************************
void CRestoreScreen::OnDeactivate()
{
	if (this->challengeDemoIDs.empty())
		return;

	//Complete scanning any remaining demos for completed challenges.
	for (CIDSet::const_iterator demoIt=this->challengeDemoIDs.begin();
			demoIt!=this->challengeDemoIDs.end(); ++demoIt)
	{
		AddChallengesCompletedInDemo(*demoIt);
	}
	this->challengeDemoIDs.clear();
}

//*****************************************************************************
void CRestoreScreen::OnDoubleClick(const UINT dwTagNo)
{
	switch (dwTagNo)
	{
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
		case TAG_LEVEL_LBOX:
			ChooseLevelLatest(this->pLevelListBoxWidget->GetSelectedItem());
			SetWidgetDisplay();
			Paint();
		break;

		case TAG_MAP:
		{
			UINT dwRoomX, dwRoomY;
			this->pMapWidget->RequestPaint();
			this->pMapWidget->GetSelectedRoomXY(dwRoomX, dwRoomY);
			ChooseRoomLatest(dwRoomX, dwRoomY);
			Paint();
		}
		break;
	}
}

//*****************************************************************************
bool CRestoreScreen::SetWidgets()
//Set up widgets and data used by them when user first arrives at restore
//screen.  Should only be called by SetForActivate().
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = true;

	//Update level selection list box.
	PopulateLevelListBoxFromSavedGames();

	//Delete any existing current game for this screen.
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = NULL;

	this->pChallengesListBox->Clear();

	//Load current room and level from game screen if it has a game loaded.
	CCueEvents CueEvents;
	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	const CCurrentGame *pCurrentGame = pGameScreen->GetCurrentGame(); //Delete not needed.
	if (pCurrentGame)
	{
		if (pCurrentGame->OnWorldMap()) {
			this->dwLastGameID = g_pTheDB->SavedGames.FindByHoldWorldMap(
					pCurrentGame->pHold->dwHoldID, pCurrentGame->worldMapID);
			ChooseLevelSavedGame(this->dwLastGameID);
		} else {
			this->dwLastGameID = g_pTheDB->SavedGames.FindByLevelLatest(
					pCurrentGame->pLevel->dwLevelID);

			if (!this->dwLastGameID) {bSuccess=false; goto Cleanup;}
			this->pCurrentRestoreGame = g_pTheDB->GetSavedCurrentGame(
					this->dwLastGameID, CueEvents, false,
					true); //don't save anything to DB during playback
			if (!this->pCurrentRestoreGame) {bSuccess=false; goto Cleanup;}

			ASSERT(!this->pCurrentRestoreGame->OnWorldMap());

			this->wConqueredRooms = this->pCurrentRestoreGame->ConqueredRooms.size();
			this->pCurrentRestoreGame->SetRoomStatusFromAllSavedGames();

			ChooseRoomLatest(pCurrentGame->pRoom->dwRoomX, pCurrentGame->pRoom->dwRoomY);
		}
	}
	else //I couldn't get current game from game screen.
	{
		//Load game from the continue slot.
		this->dwLastGameID = g_pTheDB->SavedGames.FindByContinue();
		this->pCurrentRestoreGame = this->dwLastGameID ? g_pTheDB->GetSavedCurrentGame(
				this->dwLastGameID, CueEvents, false,
				true) : //don't save anything to DB during playback
			NULL;
		if (!this->pCurrentRestoreGame)
		{
			//No continue slot yet, load from beginning of game.
			this->pCurrentRestoreGame = g_pTheDB->GetNewCurrentGame(g_pTheDB->GetHoldID(), CueEvents);
			if (!this->pCurrentRestoreGame) {bSuccess=false; goto Cleanup;}
			this->pCurrentRestoreGame->eType = ST_Continue;
			this->pCurrentRestoreGame->Update();
		}
		this->dwSelectedSavedGameID = this->pCurrentRestoreGame->dwSavedGameID;
		this->wConqueredRooms = this->pCurrentRestoreGame->ConqueredRooms.size();

		UpdateWidgets(CueEvents);

		if (this->pCurrentRestoreGame->OnWorldMap()) {
			ClearCheckpoints();
			this->pWorldMapWidget->SetCurrentGame(this->pCurrentRestoreGame);
		} else {
			//Update room label.
			CLabelWidget *pRoomLabel = DYN_CAST(CLabelWidget*, CWidget*,
					GetWidget(TAG_POSITION_LABEL));
			WSTRING wstrDesc;
			this->pCurrentRestoreGame->pRoom->GetLevelPositionDescription(wstrDesc);
			pRoomLabel->SetText(wstrDesc.c_str());

			//Put buttons over the room corresponding to saved games.
			SetCheckpoints();
			ShowCheckpointButtonsForSavedGame(this->dwSelectedSavedGameID);
		}
	}

	//Select level from the list box.
	if (this->pCurrentRestoreGame->OnWorldMap()) {
		this->pLevelListBoxWidget->SelectItem(
				LevelExit::ConvertWorldMapID(this->pCurrentRestoreGame->worldMapID));
	} else {
		this->pLevelListBoxWidget->SelectItem(this->pCurrentRestoreGame->pLevel->dwLevelID);
		GetLevelStats(this->pCurrentRestoreGame->pLevel->dwLevelID);
	}

	InitChallengeQuery();

	SetWidgetDisplay();

Cleanup:
	return bSuccess;
}

//*****************************************************************************
void CRestoreScreen::InitChallengeQuery()
{
	//Prep set of rooms to begin querying incrementally for challenge data.
	this->challengeScanRoomIDs.clear();
	this->challengeVarMap.clear();

	const UINT holdID = g_pTheDB->GetHoldID();
	CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID);
	if (pHold) {
		CDbHolds::GetScriptChallengeRefs(pHold, this->challengeVarMap, this->challengeScanRoomIDs);
		delete pHold;
	}

	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	this->completedChallenges = pPlayer->challenges.get(holdID);
	delete pPlayer;

	if (this->completedChallenges.empty()) {
		//5.0 beta fix:
		//Prep set of demos to begin querying incrementally for completed challenges.
		this->challengeDemoIDs = g_pTheDB->GetChallengeDemoIDs();
	}
}

//*****************************************************************************
void CRestoreScreen::SetWidgetDisplay()
{
	//Show if checkpoints are present
	CWidget *pWidget = GetWidget(TAG_CHOOSE_POS_LABEL);
	pWidget->Show(GetWidget(TAG_CHECKPOINT) != NULL);

	//Whether widgets related to the level and room are displayed.
	const bool room_level_display =
		this->pCurrentRestoreGame && !this->pCurrentRestoreGame->OnWorldMap();

	static const UINT tags[] = {
		TAG_CHOOSE_ROOM_LABEL,
		TAG_EXPLORED_LABEL,
		TAG_LEVEL_START,
		TAG_MAP_SCROLLER,
		TAG_LEVEL_EXPLORED,
		TAG_LEVEL_SECRETS,
		TAG_ROOM_START,
		TAG_POSITION_LABEL,
		TAG_ROOM_WIDGET,
		0
	};

	for (UINT i=0; tags[i]; ++i) {
		pWidget = GetWidget(tags[i]);
		pWidget->Show(room_level_display);
	}

	this->pScaledWorldMapWidget->Show(!room_level_display);
}

//*****************************************************************************
void CRestoreScreen::ShowCheckpointButtonsForSavedGame(
//Hides checkpoint button corresponding to a saved game and shows all the rest.
//
//Params:
	const UINT dwSavedGameID) //(in)
{
	for (CHECKPOINT_LIST::const_iterator iSeek = this->Checkpoints.begin();
			iSeek != this->Checkpoints.end(); ++iSeek)
	{
		const CHECKPOINT& checkpoint = *iSeek;
		checkpoint.pButton->Show(checkpoint.dwSavedGameID != dwSavedGameID);
	}
}

//*****************************************************************************
void CRestoreScreen::ClearCheckpoints()
{
	for (CHECKPOINT_LIST::const_iterator iSeek = this->Checkpoints.begin();
			iSeek != this->Checkpoints.end(); ++iSeek)
		this->pScaledRoomWidget->RemoveWidget(iSeek->pButton);
	this->Checkpoints.clear();
}

void CRestoreScreen::SetCheckpoints()
//Adds and removes widgets so that a checkpoint button exists over each
//checkpoint in the room widget with a corresponding saved game.
{
	ClearCheckpoints();

	CDb db;
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);
	db.SavedGames.FilterByRoom(this->pCurrentRestoreGame->pRoom->dwRoomID);
	db.SavedGames.FilterByPlayer(dwCurrentPlayerID);

	//Each iteration looks at one saved game in the room.
	UINT dwTagNo = TAG_CHECKPOINT;
	CDbSavedGame *pSavedGame = db.SavedGames.GetFirst();
	while (pSavedGame)
	{
		//Is it a checkpoint saved game?
		if (pSavedGame->eType == ST_Checkpoint)
		{
			//Yes--add a new button for it.
			CButtonWidget *pCheckpointButton;
			SDL_Rect SquareRect;
			SDL_Rect ScaledRoomRect;
			this->pScaledRoomWidget->GetRect(ScaledRoomRect);

			this->pRoomWidget->GetSquareRect(pSavedGame->wCheckpointX,
					pSavedGame->wCheckpointY, SquareRect);
			SquareRect.x = this->pScaledRoomWidget->GetScaledX(SquareRect.x) -
					ScaledRoomRect.x - 5;
			SquareRect.y = this->pScaledRoomWidget->GetScaledY(SquareRect.y) -
					ScaledRoomRect.y - 5;
			SquareRect.w = this->pScaledRoomWidget->GetScaledW(SquareRect.w) + 15;
			SquareRect.h = this->pScaledRoomWidget->GetScaledH(SquareRect.h) + 15;

			static const WCHAR wszX[] = {We('x'),We(0)};
			pCheckpointButton = new CButtonWidget(dwTagNo, SquareRect.x, SquareRect.y,
					SquareRect.w, SquareRect.h, wszX);
			this->pScaledRoomWidget->AddWidget(pCheckpointButton, true);
			++dwTagNo;

			//Add button to checkpoint button list.
			if (!pCheckpointButton)
			{
				ASSERT(!"No checkpoint button.");
			}
			else
			{
				CHECKPOINT sNew = { pCheckpointButton, pSavedGame->dwSavedGameID };
				this->Checkpoints.push_back(sNew);
			}
		}
		delete pSavedGame;
		pSavedGame = db.SavedGames.GetNext();
	}
}

//*****************************************************************************
void CRestoreScreen::ChooseCheckpoint(
//Chooses saved game from a checkpoint.
//
//Params:
	const UINT dwTagNo) //(in)   Tag# of pressed checkpoint button.
{
	//Find checkpoint info.
	const CHECKPOINT *pCheckpoint = NULL;
	CHECKPOINT_LIST::const_iterator iSeek;
	for (iSeek = this->Checkpoints.begin(); iSeek != this->Checkpoints.end(); ++iSeek)
	{
		pCheckpoint = &*iSeek;
		if (pCheckpoint->pButton->GetTagNo() == dwTagNo) break; //Found it.
	}
	if (iSeek == this->Checkpoints.end()) return; //No match.

	ChooseRoomSavedGame(pCheckpoint->dwSavedGameID);
}

//*****************************************************************************
void CRestoreScreen::ChooseFurthestSave()
//Chooses the saved game with the most progress in the current hold and updates display.
{
	const UINT holdID = this->pCurrentRestoreGame->pHold->dwHoldID;
	const UINT savedGameID = g_pTheDB->SavedGames.FindByHoldLatest(holdID);
	if (!savedGameID)
		return;

	const UINT roomID = g_pTheDB->SavedGames.GetRoomIDofSavedGame(savedGameID);
	const UINT levelID = g_pTheDB->Rooms.GetLevelIDForRoom(roomID);
	if (!levelID)
		return;

	const UINT worldMapID = g_pTheDB->SavedGames.GetWorldMapID(savedGameID);
	const bool isWorldMap = LevelExit::IsWorldMapID(levelID) || worldMapID != 0;
	if (!isWorldMap)
		GetLevelStats(levelID);

	this->pLevelListBoxWidget->SelectItem(levelID);
	ChooseLevelSavedGame(savedGameID);
}

//*****************************************************************************
void CRestoreScreen::ChooseRoomLatest(
//Chooses latest saved game for room in the current level and updates display.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of chosen room.
{
	//Find the room.
	const UINT dwRoomID = g_pTheDB->Rooms.FindIDAtCoords(
			this->pCurrentRestoreGame->pLevel->dwLevelID, dwRoomX, dwRoomY);
	if (dwRoomID)
	{
		//Find the saved game.
		const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByRoomLatest(dwRoomID);
		if (dwSavedGameID)
			ChooseRoomSavedGame(dwSavedGameID);
	}
}

//*****************************************************************************
void CRestoreScreen::ChooseRoomStart(
//Chooses room start saved game for room in the current level and updates display.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of chosen room.
{
	//Find the room.
	const UINT dwRoomID = g_pTheDB->Rooms.FindIDAtCoords(
			this->pCurrentRestoreGame->pLevel->dwLevelID, dwRoomX, dwRoomY);
	if (dwRoomID)
	{
		//Find the saved game.
		const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByRoomBegin(dwRoomID);
		if (!dwSavedGameID) return;

		ChooseRoomSavedGame(dwSavedGameID);
	}
}

//*****************************************************************************
void CRestoreScreen::ChooseRoomSavedGame(
//Chooses a room in the current level and updates display.
//
//Params:
	const UINT dwSavedGameID) //(in) Saved game to use.
{
	ASSERT(dwSavedGameID);

	SetCursor(CUR_Wait);

	//Load the saved game.
	CCueEvents CueEvents;
	CCurrentGame *pNewGame = g_pTheDB->GetSavedCurrentGame(dwSavedGameID, CueEvents, false,
			true); //don't save anything to DB here
	if (pNewGame)
	{
		//Switch current game over to new one from saved game.
		this->dwSelectedSavedGameID = dwSavedGameID;

		delete this->pCurrentRestoreGame;
		this->pCurrentRestoreGame = pNewGame;

		UpdateWidgets(CueEvents);

		this->pMapWidget->SelectRoom(this->pCurrentRestoreGame->pRoom->dwRoomX,
				this->pCurrentRestoreGame->pRoom->dwRoomY);

		//Update room label.
		WSTRING wstrDesc;
		this->pCurrentRestoreGame->pRoom->GetLevelPositionDescription(wstrDesc);
		CLabelWidget *pRoomLabel = DYN_CAST(CLabelWidget*, CWidget*,
				GetWidget(TAG_POSITION_LABEL));
		pRoomLabel->SetText(wstrDesc.c_str());

		//Put buttons over the room corresponding to saved games.
		SetCheckpoints();
		ShowCheckpointButtonsForSavedGame(dwSavedGameID);
	}

	SetCursor();
}

//*****************************************************************************
void CRestoreScreen::ChooseLevelStart(
//Choose a level start saved game and updates display.
//
//Params:
	const UINT dwLevelID)  //(in) Level to load saved game for.
{
	ASSERT(dwLevelID);

	//Find the level start saved game.
	const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByLevelBegin(dwLevelID);
	if (!dwSavedGameID) return;

	ChooseLevelSavedGame(dwSavedGameID);
}

//*****************************************************************************
void CRestoreScreen::ChooseLevelLatest(
//Choose latest saved game in a level and updates display.
//
//Params:
	const UINT dwLevelID)  //(in) Level to load saved game for.
{
	ASSERT(dwLevelID);

	//Don't update when selecting the same level.
	if (!this->pCurrentRestoreGame->OnWorldMap() &&
			dwLevelID == this->pCurrentRestoreGame->pLevel->dwLevelID)
		return;

	UINT savedGameID;
	const bool isWorldMap = LevelExit::IsWorldMapID(dwLevelID);
	if (isWorldMap) {
		const UINT worldMapID = LevelExit::ConvertWorldMapID(dwLevelID);
		savedGameID = g_pTheDB->SavedGames.FindByHoldWorldMap(
			this->pCurrentRestoreGame->pHold->dwHoldID, worldMapID);
	} else {
		//Find the latest saved game on the level.
		savedGameID = g_pTheDB->SavedGames.FindByLevelLatest(dwLevelID);
	}

	if (savedGameID) {
		if (!isWorldMap)
			GetLevelStats(dwLevelID);

		ChooseLevelSavedGame(savedGameID);
	}
}

//*****************************************************************************
void CRestoreScreen::ChooseLevelSavedGame(
//Chooses a level and updates display.
//
//Params:
	const UINT dwSavedGameID) //(in) Saved game to load.
{
	ASSERT(dwSavedGameID);

	//Load the saved game.
	CCueEvents CueEvents;
	CCurrentGame *pNewGame = g_pTheDB->GetSavedCurrentGame(dwSavedGameID, CueEvents, false,
			true); //don't save anything to DB here
	if (pNewGame)
	{
		//Switch current game over to new one from saved game.
		this->dwSelectedSavedGameID = dwSavedGameID;

		delete this->pCurrentRestoreGame;
		this->pCurrentRestoreGame = pNewGame;

		UpdateWidgets(CueEvents);

		if (pNewGame->OnWorldMap()) {
			ClearCheckpoints();
			this->pWorldMapWidget->SetCurrentGame(pNewGame);
		} else {
			//Update room label.
			CLabelWidget *pRoomLabel = DYN_CAST(CLabelWidget*, CWidget*,
					GetWidget(TAG_POSITION_LABEL));
			WSTRING wstrDesc;
			this->pCurrentRestoreGame->pRoom->GetLevelPositionDescription(wstrDesc);
			pRoomLabel->SetText(wstrDesc.c_str());

			//Put buttons over the room corresponding to saved games.
			SetCheckpoints();
			ShowCheckpointButtonsForSavedGame(dwSavedGameID);
		}
	}
}

//*****************************************************************************
void CRestoreScreen::DisplayChallengesDialog()
{
	if (this->pChallengesListBox->IsEmpty()) {
		SetCursor(CUR_Wait);
		PopulateChallenges(this->pChallengesListBox, this->pChallengesCountLabel);
		SetCursor();
	}

	this->pChallengesDialog->Display();
}

//*****************************************************************************
void CRestoreScreen::AddChallengesCompletedInDemo(const UINT demoID)
{
	CDbDemo *pDemo = CDbDemos::GetByID(demoID);
	if (pDemo) {
		CIDList demoStats;
		pDemo->Test(demoStats);
		IDNODE *pNode = demoStats.GetByID(DS_ChallengeCompleted);
		if (pNode) {
			const CAttachableWrapper<set<WSTRING> > *pStrings =
				static_cast<CAttachableWrapper<set<WSTRING> > *>(pNode->pvPrivate);
			ASSERT(pStrings);
			if (!pStrings->data.empty()) {
				this->completedChallenges.insert(pStrings->data.begin(), pStrings->data.end());

				//Add to player profile so challenges don't need to be requeried.
				CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
				ASSERT(pPlayer);
				if (pPlayer->challenges.add(g_pTheDB->GetHoldID(), pStrings->data))
					pPlayer->Update();
				delete pPlayer;
			}
		}
		delete pDemo;
	}
}

//*****************************************************************************
void CRestoreScreen::PopulateChallenges(
	CListBoxWidget* pListBoxWidget, CLabelWidget* pLabelWidget)
{
	//Complete scanning any remaining rooms for challenges.
	for (CIDSet::const_iterator roomIt=this->challengeScanRoomIDs.begin();
			roomIt!=this->challengeScanRoomIDs.end(); ++roomIt)
	{
		CDbHolds::GetScriptCommandRefsForRoom(*roomIt, NULL, true, this->challengeVarMap);
	}
	this->challengeScanRoomIDs.clear();

	//Complete scanning any remaining demos for completed challenges.
	if (!this->challengeDemoIDs.empty()) {
		UINT count=0;
		for (CIDSet::const_iterator demoIt=this->challengeDemoIDs.begin();
				demoIt!=this->challengeDemoIDs.end(); ++demoIt, ++count)
		{
			Callbackf(count/float(this->challengeDemoIDs.size()));
			AddChallengesCompletedInDemo(*demoIt);
		}
		this->challengeDemoIDs.clear();
		PublicHideProgressWidget();
		Paint();
	}

	CIDSet exploredRoomIDs;
	if (this->challengeVarMap.empty()) {
		pListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(MID_None), true);
		pLabelWidget->Hide();
	} else {
		CDb db;
		const UINT savedGameID = db.SavedGames.FindByType(ST_PlayerTotal, g_pTheDB->GetPlayerID(), false);
		exploredRoomIDs = db.SavedGames.GetExploredRooms(savedGameID); //roomIDs for all holds
		pLabelWidget->Show();
	}

#ifndef ENABLE_CHEATS
	// Check hold authorship
	// Not required if cheats are active
	CDbPlayer* pPlayer = g_pTheDB->GetCurrentPlayer();
	CDbHold* pHold = this->pCurrentRestoreGame->pHold;
	bool bIsAuthor = (pPlayer->dwPlayerID == pHold->dwPlayerID);
	delete pPlayer;
#endif
	for (CDbHolds::VARCOORDMAP::const_iterator vars = this->challengeVarMap.begin();
			vars != this->challengeVarMap.end(); ++vars)
	{
		//Display info for this variable.
		WSTRING challengeName = vars->first;
		const bool challengeEarned = this->completedChallenges.count(challengeName) != 0;

		//Print location of first reference to this challenge.
		const CDbHolds::VAR_LOCATIONS& locations = vars->second;
		const CDbHolds::VARROOMS& rooms = locations.rooms;
		if (!rooms.empty()) {
			const UINT roomID = rooms.begin()->first;
#ifndef ENABLE_CHEATS
			if (exploredRoomIDs.has(roomID) || bIsAuthor)
#endif
			{
				ASSERT(roomID);
				CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(roomID, true);
				ASSERT(pRoom);

				challengeName += wszSpace;
				challengeName += wszHyphen;
				challengeName += wszSpace;
				challengeName += CDbLevels::GetLevelName(pRoom->dwLevelID);
				challengeName += wszColon;
				challengeName += wszSpace;

				pRoom->GetLevelPositionDescription(challengeName, true);
				delete pRoom;
			}
		}

		pListBoxWidget->AddItem(0, challengeName.c_str(), !challengeEarned);
	}

	pListBoxWidget->SelectLine(0);

	// Count of challenges
	WSTRING countString = wszLeftParen;
	countString += to_WSTRING(this->completedChallenges.size());
	countString += wszForwardSlash;
	countString += to_WSTRING(this->challengeVarMap.size());
	countString += wszRightParen;
	pLabelWidget->SetText(countString.c_str());
}

//*****************************************************************************
void CRestoreScreen::GetLevelStats(const UINT dwLevelID)
//Show player's level completion stats.
//More information is displayed after hold is completed.
{
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	CIDSet roomsInLevel, playerRoomsExploredInLevel;
	g_pTheDB->Levels.GetRoomsExplored(dwLevelID, dwPlayerID,
			roomsInLevel, playerRoomsExploredInLevel);

	//Display percent of rooms in level explored.
	WCHAR num[10];
	WSTRING wstr;
	CLabelWidget *pStatsLabel =
			DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_LEVEL_EXPLORED));
	ASSERT(pStatsLabel);
	if (!this->bHoldConquered)
	{
		_itoW(playerRoomsExploredInLevel.size(), num, 10);
		wstr = num;
	} else {
		_itoW(static_cast<int>(playerRoomsExploredInLevel.size() * 100.0 /
				(float)roomsInLevel.size()), num, 10);
		wstr = num;
		wstr += wszPercent;
	}
	pStatsLabel->SetText(wstr.c_str());

	//Display number of these rooms explored or conquered.
	pStatsLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_LEVEL_SECRETS));
	ASSERT(pStatsLabel);
	wstr = g_pTheDB->GetMessageText(
			this->bHoldConquered ? MID_SecretsConquered : MID_SecretsFound);
	wstr += wszSpace;

	UINT dwSecretRooms, dwSecretRoomsDone;
	if (this->bHoldConquered)
	{
		//Tally only conquered secret rooms once hold is finished.
		g_pTheDB->Levels.GetRoomsExplored(dwLevelID, dwPlayerID,
				roomsInLevel, playerRoomsExploredInLevel, true);
	}
	g_pTheDB->Levels.GetSecretRoomsInSet(roomsInLevel, playerRoomsExploredInLevel,
			dwSecretRooms, dwSecretRoomsDone);
	_itoW(dwSecretRoomsDone, num, 10);
	wstr += num;
	if (this->bHoldConquered)
	{
		//When hold is finished, reveal how many secret rooms are in this level.
		wstr += wszForwardSlash;
		_itoW(dwSecretRooms, num, 10);
		wstr += num;
	}
	pStatsLabel->SetText(wstr.c_str());
}

//*****************************************************************************
void CRestoreScreen::UpdateWidgets(CCueEvents& CueEvents)
//Update the map and room widgets to reflect the current game.
//Update the map widget to show rooms which are not explored in the saved game.
{
	UpdateStatsLabel(*this->pCurrentRestoreGame);

	if (this->pCurrentRestoreGame->OnWorldMap()) {
		this->pRoomWidget->UnloadCurrentGame();
		this->pMapWidget->ClearMap();
	} else {
		VERIFY(this->pRoomWidget->LoadFromCurrentGame(this->pCurrentRestoreGame));

		// Always clear events before new ones are added to avoid persistence
		// when changing rooms/checkpoints
		this->pRoomWidget->ClearEffects();

		// Load the image overlays and freeze them so they don't disappear nor animate.
		this->pRoomWidget->DisplayPersistingImageOverlays(CueEvents);
		ProcessImageEvents(CueEvents, this->pRoomWidget, this->pCurrentRestoreGame);
		this->pRoomWidget->SetEffectsFrozen(true);

		VERIFY(this->pMapWidget->LoadFromCurrentGame(this->pCurrentRestoreGame, false));

		//Set conquered/explored status of rooms from all saved games in the level.
		const CIDSet CurrentExploredRooms = this->pCurrentRestoreGame->ExploredRooms;
		this->pCurrentRestoreGame->SetRoomStatusFromAllSavedGames();

		CIDSet DarkenedRooms = this->pCurrentRestoreGame->ExploredRooms;
		DarkenedRooms -= CurrentExploredRooms;
		this->pMapWidget->SetDarkenedRooms(DarkenedRooms);
	}
}

void CRestoreScreen::UpdateStatsLabel(CCurrentGame& game)
{
	CLabelWidget *pStatsLabel =	DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_SAVED_GAME_STATS));
	ASSERT(pStatsLabel);

	std::ostringstream str;
	str << " " << game.ConqueredRooms.size() << " / " << game.ExploredRooms.size();
	WSTRING wstr = g_pTheDB->GetMessageText(MID_TotalRoomStats), wtemp;
	UTF8ToUnicode(str.str().c_str(), wtemp);
	wstr += wtemp;

	pStatsLabel->SetText(wstr.c_str());
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
void CRestoreScreen::PopulateLevelListBoxFromSavedGames()
//Put levels into list box that have at least one saved game associated with
//them.  Determine whether this hold has been completed.
{
	BEGIN_DBREFCOUNT_CHECK;

	this->pLevelListBoxWidget->Clear();

	{
		//Get the hold.
		const UINT holdID = g_pTheDB->GetHoldID();
		CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID);
		if (!pHold) {ASSERT(!"Failed to retrieve hold."); return;} //Probably corrupted DB.

		//Check for first level.
		const CIDSet levelsInHold = CDb::getLevelsInHold(holdID);
		if (levelsInHold.empty())
		{
			delete pHold; //Hold doesn't have any levels.
			return;
		}

		PopulateLevelListFromWorldMapSavedGames(*pHold);

		//Add level IDs containing level start saved games in the current hold.
		SORTED_LEVELS levels;
		for (CIDSet::const_iterator levelID = levelsInHold.begin(); levelID != levelsInHold.end(); ++levelID)
		{
			CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*levelID);
			ASSERT(pLevel);
			if (g_pTheDB->SavedGames.FindByLevelBegin(pLevel->dwLevelID))
				levels.insert(pLevel);
			else
				delete pLevel;
		}

		//Display levels in sorted order.
		for (SORTED_LEVELS::const_iterator level = levels.begin(); level != levels.end(); ++level)
		{
			this->pLevelListBoxWidget->AddItem((*level)->dwLevelID, (*level)->NameText);
			delete *level;
		}

		this->bHoldConquered = (g_pTheDB->SavedGames.FindByEndHold(pHold->dwHoldID) != 0);

		delete pHold;
	}

	END_DBREFCOUNT_CHECK;
}

void CRestoreScreen::PopulateLevelListFromWorldMapSavedGames(const CDbHold& hold)
{
	if (!hold.worldMaps.empty()) {
		CIDSet worldMapIDs;
		CDb db;
		db.SavedGames.FilterByPlayer(g_pTheDB->GetPlayerID());
		db.SavedGames.FilterByHold(hold.dwHoldID);
		const CIDSet savedGameIDs = db.SavedGames.GetIDs();
		for (CIDSet::const_iterator iter = savedGameIDs.begin(); iter != savedGameIDs.end(); ++iter)
		{
			const SAVETYPE type = db.SavedGames.GetType(*iter);
			if (type == ST_WorldMap)
			{
				const UINT worldMapID = db.SavedGames.GetWorldMapID(*iter);
				if (worldMapID)
					worldMapIDs += worldMapID;
			}
		}

		for (vector<HoldWorldMap>::const_iterator map = hold.worldMaps.begin();
				map != hold.worldMaps.end(); ++map) {
			if (worldMapIDs.has(map->worldMapID)) {
				const UINT uniqueWorldMapID = LevelExit::ConvertWorldMapID(map->worldMapID);
				this->pLevelListBoxWidget->AddItem(uniqueWorldMapID, map->nameText.c_str());
				this->pLevelListBoxWidget->SetItemColor(uniqueWorldMapID, DarkGreen);
			}
		}
	}
}

//*****************************************************************************
void CRestoreScreen::RestoreGame()
{
	if (!this->dwSelectedSavedGameID) return;

	if (this->dwLastGameID)
	{
		//If the level being restored to is the same as the one
		//for the current game or continue slot, prompt the player
		//if a saved game with fewer conquered rooms is being selected.
		CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetByID(
				this->dwLastGameID);
		if (pSavedGame)
		{
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(pSavedGame->dwRoomID, true);
			delete pSavedGame;
			ASSERT(pRoom);
			if (pRoom->dwLevelID == this->pCurrentRestoreGame->pRoom->dwLevelID)
			{
				if (this->wConqueredRooms >
						this->pCurrentRestoreGame->ConqueredRooms.size())
				{
					if (ShowYesNoMessage(MID_UnconqueredRooms) != TAG_YES)
					{
						delete pRoom;
						return;
					}
				}
			}
			delete pRoom;
		} //else: the "last" saved game could be for an empty continue slot,
		//in which case it shouldn't be compared against.
	}
	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	pGameScreen->LoadSavedGame(this->dwSelectedSavedGameID);
	GoToScreen(pGameScreen->SelectGotoScreen());
}
