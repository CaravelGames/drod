// $Id: NewPlayerScreen.h 8102 2007-08-15 14:55:40Z trick $

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

#ifndef NEWPLAYERSCREEN_H
#define NEWPLAYERSCREEN_H

#include "DrodScreen.h"
#include "../DRODLib/DbPlayers.h"

class CListBoxWidget;
class CButtonWidget;
class CDialogWidget;
class CLabelWidget;
class CNewPlayerScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CNewPlayerScreen();
	virtual ~CNewPlayerScreen();

	virtual void OnBetweenEvents();
	virtual bool OnQuit();
	virtual void Paint(bool bUpdateRect=true);

	virtual bool   SetForActivate();

private:
	UINT    AddPlayer();
	UINT		Benchmark() const;
	void     SetPlayerHold(const UINT dwPlayerID) const;

	CDialogWidget *pPlayerBox;
	CButtonWidget *pOKButton, *pImportPlayerButton;
	CTextBoxWidget *pNameWidget;
};

#endif //...#ifndef NEWPLAYERSCREEN_H

