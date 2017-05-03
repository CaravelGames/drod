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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//This is a CDialog that has been augmented to show a list of level entrances.

#ifndef ENTRANCESELECTDIALOGWIDGET_H
#define ENTRANCESELECTDIALOGWIDGET_H

#include <FrontEndLib/DialogWidget.h>
#include "../DRODLib/DbLevels.h"
#include <BackEndLib/MessageIDs.h>
#include <set>

class CDbHold;
class CEntranceData;
class CLabelWidget;
class CListBoxWidget;
class CCurrentGame;
class CEntranceSelectDialogWidget : public CDialogWidget
{
public:
	CEntranceSelectDialogWidget(const UINT dwSetTagNo, const int nSetX=0,
			const int nSetY=0);

	enum DATATYPE
	{
		Entrances,     //level entrances in hold
		Images,        //custom hold images
		Sounds,        //custom hold sounds
		Videos,        //custom hold videos
		Speech,        //speech
		ChatHistory,   //chat log
		WorldMaps      //list of world maps in hold
	};

	enum BUTTONTYPE
	{
		OK,
		Delete,
		Other
	};

	UINT    GetSelectedItem() const;
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnSelectChange(const UINT dwTagNo);
	void     PopulateList(const DATATYPE datatype=Entrances);
	void     SelectItem(const UINT dwTagNo);
	void     SetCurrentGame(CCurrentGame *pGame);
	void     SetPrompt(const MESSAGE_ID messageID);
	void     SetSourceHold(CDbHold *pHold);

	static void     SortEntrances(CDbHold *pHold, vector<CEntranceData*> &entrances);

private:
	void     AddEntranceToList(CEntranceData *pEntrance);

	CListBoxWidget *  pListBoxWidget;
	CDbHold *pSourceHold;
	CCurrentGame *pCurrentGame;
	CLabelWidget *pPromptLabel;
};

#endif   //ENTRANCESELECTDIALOGWIDGET_H
