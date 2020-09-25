// $Id: SelectPlayerScreen.h 8484 2008-01-09 03:02:07Z mrimer $

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

#ifndef SELECTPLAYERSCREEN_H
#define SELECTPLAYERSCREEN_H

#include "DrodScreen.h"
#include "../DRODLib/DbPlayers.h"

class CListBoxWidget;
class CButtonWidget;
class CDialogWidget;
class CLabelWidget;
class CSelectPlayerScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CSelectPlayerScreen();
	virtual ~CSelectPlayerScreen();

	virtual void OnBetweenEvents();
	virtual void OnSelectChange(const UINT dwTagNo);
	virtual void Paint(bool bUpdateRect=true);

	virtual bool   SetForActivate();

private:
	UINT    AddPlayer();
	UINT    GetSelectedItem();
	void     SelectPlayer();
	void     SetPlayerID(UINT &dwPlayerID);
	void     SetPlayerHold(const UINT dwPlayerID) const;
	void     SetPlayerDesc(const UINT dwPlayerID);
	void     PopulatePlayerListBox(CListBoxWidget *pPlayerListBoxWidget) const;

	CDialogWidget *pPlayerBox;
	CListBoxWidget *pPlayerListBoxWidget;
	CButtonWidget *pOKButton, *pNewPlayerButton, *pDeletePlayerButton,
			*pExportPlayerButton, *pImportPlayerButton;
	CLabelWidget *pPlayerHoldLabel, *pPlayerPositionLabel;
	static bool bFirst;  //whether first time entered
};

#endif //...#ifndef NEWPLAYERSCREEN_H

