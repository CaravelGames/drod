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
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef RESTORESCREEN_H
#define RESTORESCREEN_H

#include "DrodScreen.h"
#include "RoomWidget.h"
#include "MapWidget.h"
#include <FrontEndLib/ListBoxWidget.h>
#include "../DRODLib/CurrentGame.h"
#include <BackEndLib/Types.h>

struct CHECKPOINT
{
	CWidget *pButton;
	UINT    dwSavedGameID;
};

//typedef list<void *> CHECKPOINT_LIST; //"void *" s/b "CHECKPOINT *", but it gave me annoying warnings.
typedef list<CHECKPOINT> CHECKPOINT_LIST;

class CScalerWidget;
class CWorldMapWidget;
class CRestoreScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CRestoreScreen();
	virtual ~CRestoreScreen();

	virtual bool   SetForActivate();
	virtual bool   UnloadOnDeactivate() const {return false;}

private:
	void     AddChallengesCompletedInDemo(const UINT demoID);
	void     ClearCheckpoints();
	void     ClearState();
	void     ChooseCheckpoint(const UINT dwTagNo);
	void     ChooseFurthestSave();
	void     ChooseLevelLatest(const UINT dwLevelID);
	void     ChooseLevelSavedGame(const UINT dwSavedGameID);
	void     ChooseLevelStart(const UINT dwLevelID);
	void     ChooseRoomLatest(const UINT dwRoomX, const UINT dwRoomY);
	void     ChooseRoomSavedGame(const UINT dwSavedGameID);
	void     ChooseRoomStart(const UINT dwRoomX, const UINT dwRoomY);
	void     DisplayChallengesDialog();
	void     GetLevelStats(const UINT dwLevelID);
	void     InitChallengeQuery();
	virtual void   OnBetweenEvents();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDeactivate();
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnSelectChange(const UINT dwTagNo);
	virtual void   Paint(bool bUpdateRect=true);
	void     PopulateChallenges(CListBoxWidget* pListBoxWidget, CLabelWidget* pLabelWidget);
	void     PopulateLevelListBoxFromSavedGames();
	void     PopulateLevelListFromWorldMapSavedGames(const CDbHold& hold);
	void     RestoreGame();
	void     SetCheckpoints();
	bool     SetWidgets();
	void     SetWidgetDisplay();
	void     ShowCheckpointButtonsForSavedGame(const UINT dwSavedGameID);
	void     UpdateWidgets();
	void     UpdateStatsLabel(CCurrentGame& game);

	UINT       dwSelectedSavedGameID;
	UINT       dwLastGameID;
	UINT        wConqueredRooms;
	bool        bHoldConquered;
	bool        bResetWidgets;      //on screen activation

	CCurrentGame * pCurrentRestoreGame;
	CRoomWidget *  pRoomWidget;
	CScalerWidget *   pScaledRoomWidget;
	CMapWidget *   pMapWidget;
	CListBoxWidget *pLevelListBoxWidget;
	CWorldMapWidget *pWorldMapWidget;
	CScalerWidget *pScaledWorldMapWidget;
	CDialogWidget *pChallengesDialog;
	CListBoxWidget *pChallengesListBox;
	CLabelWidget* pChallengesCountLabel;

	CHECKPOINT_LIST Checkpoints;

	//optimization for compiling challenges and completion
	CIDSet      challengeDemoIDs, challengeScanRoomIDs;
	set<WSTRING> completedChallenges;
	CDbHolds::VARCOORDMAP challengeVarMap;
};

#endif //...#ifndef RESTORESCREEN_H
