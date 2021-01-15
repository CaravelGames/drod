// $Id: RestoreScreen.h 8482 2008-01-09 03:00:36Z mrimer $

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
#include <FrontEndLib/ScalerWidget.h>
#include "../DRODLib/CurrentGame.h"
#include <BackEndLib/Types.h>

class CRestoreScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CRestoreScreen();
	virtual ~CRestoreScreen();

	virtual bool   SetForActivate();
	virtual bool   UnloadOnDeactivate() const {return false;}

private:
	void     ChooseSavedGame(const UINT dwSavedGameID);
	void     ClearState();
	void     DisplayScorepointsDialog();
	void     InitScorepointQuery();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnSelectChange(const UINT dwTagNo);
	virtual void   Paint(bool bUpdateRect=true);
	void     PopulateListBoxFromSavedGames();
	void     PopulateScorepoints(CListBoxWidget* pListBoxWidget);
	void     RenameSaveGame(const UINT saveID);
	void     RestoreGame();
	bool     SetWidgets();
	void     ShowSave();
	void     ShowSaveStats();
	void     UpdateWidgets();

	void     GetSaves();
	SORTED_SAVES GetSortedSaves() const;

	vector<SAVE_INFO> saves;

	UINT       dwSelectedSavedGameID;
	UINT       dwLastGameID;
//	UINT       wConqueredRooms;
	UINT       gameSort;
	bool       bHoldConquered;
	bool       bResetWidgets;      //on screen activation

	CCurrentGame   *pCurrentRestoreGame;
	CRoomWidget    *pRoomWidget;
	CScalerWidget  *pScaledRoomWidget;
	CMapWidget     *pMapWidget;
	CListBoxWidget *pSaveListBoxWidget;
	CDialogWidget* pScorepointsDialog;
	CListBoxWidget* pScorepointsListBox;

	//optimization for compiling challenges and completion
	CIDSet scorepointScanRoomIDs;
	CDbHolds::VARCOORDMAP scorepointVarMap;
};

#endif //...#ifndef RESTORESCREEN_H

