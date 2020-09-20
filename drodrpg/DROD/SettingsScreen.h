// $Id: SettingsScreen.h 8810 2008-03-13 00:42:09Z mrimer $

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

#ifndef SETTINGSSCREEN_H
#define SETTINGSSCREEN_H

#include "DrodScreen.h"

#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/KeypressDialogWidget.h>
#include "../DRODLib/DbPlayers.h"
#include <BackEndLib/Types.h>

//Definable command constants.  Must be in same order as associated widgets and
//their tags.
enum DCMD
{
	DCMD_First = 0,
	DCMD_NW = DCMD_First,
	DCMD_N,
	DCMD_NE,
	DCMD_W,
	DCMD_Wait,
	DCMD_E,
	DCMD_SW,
	DCMD_S,
	DCMD_SE,
	DCMD_C,
	DCMD_CC,
	DCMD_Restart,
	DCMD_Undo,
	DCMD_Battle,
	DCMD_UseAccessory,
	DCMD_Lock,
	DCMD_Command,

	DCMD_Count
};

extern const char COMMANDNAME_ARRAY[DCMD_Count][24];
extern const SDLKey COMMANDKEY_ARRAY[2][DCMD_Count];	//numpad or laptop

//*****************************************************************************************
class CSettingsScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CSettingsScreen();
	virtual ~CSettingsScreen();

	virtual bool   SetForActivate();

private:
	bool     AllCommandsAreAssignedToKeys(CDbPackedVars &Settings) const;
	bool     GetCommandKeyRedefinition(const DCMD eCommand, const SDLKey CurrentKey, SDLKey &NewKey);
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void OnClick(const UINT dwTagNo);
	virtual void OnDragUp(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void OnSelectChange(const UINT dwTagNo);
	virtual void Paint(bool bUpdateRect=true);
	bool     PollForInterrupt();
	void     RestorePlayerSettings();
	void     SetUnspecifiedPlayerSettings(CDbPackedVars &Settings);
	void     SetWidgetStates();
	void     SynchOption(const UINT dwTagNo);
	void     SynchScreenSizeWidget();
	void     UpdatePlayerDataFromWidgets(CDbPlayer *pPlayer);
	void     UpdateWidgetsFromPlayerData(CDbPlayer *pPlayer);
	void     UploadScores();

	CKeypressDialogWidget * pDialogBox;
	CLabelWidget *       pCommandLabel;
	CWidget  *           pCommandLabelWidgets[DCMD_Count];
	CDbPlayer *          pCurrentPlayer;

	CTextBoxWidget * pNameWidget;
	CTextBoxWidget * pCaravelNetNameWidget;
	CTextBoxWidget * pCaravelNetPasswordWidget;
};

#endif //...#ifndef SETTINGSSCREEN_H

