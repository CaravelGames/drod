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
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Types.h>

extern const SDL_Keycode COMMANDKEY_ARRAY[2][InputCommands::DCMD_Count];	//desktop or notebook

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
	bool     AreCNetDetailsChanged(CDbPlayer *pPlayer);
	bool     GetCommandKeyRedefinition(const InputCommands::DCMD eCommand, const SDL_Keycode CurrentKey, SDL_Keycode &NewKey);
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
	void     UploadScoreCheckpointSaves(const UINT dwPlayerID);

	CKeypressDialogWidget * pDialogBox;
	CLabelWidget *       pCommandLabel;
	CWidget  *           pCommandLabelWidgets[InputCommands::DCMD_Count];
	CDbPlayer *          pCurrentPlayer;

	CTextBoxWidget * pNameWidget;
	CTextBoxWidget * pCaravelNetNameWidget;
	CTextBoxWidget * pCaravelNetPasswordWidget;
};

#endif //...#ifndef SETTINGSSCREEN_H

