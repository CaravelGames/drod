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

#ifndef DEMOSCREEN_H
#define DEMOSCREEN_H

#include "../DRODLib/DbDemos.h"
#include <BackEndLib/Types.h>

#include "GameScreen.h"

//***************************************************************************************
class CDemoScreen : public CGameScreen
{
public:
	bool     LoadDemoGame(const UINT dwDemoID);
	void     SetReplayOptions(bool bChangeSpeed);

protected:
	friend class CDrodScreenManager;

	CDemoScreen();
	virtual ~CDemoScreen();

	virtual bool   SetForActivate();

private:
	virtual void   OnBetweenEvents();
	virtual void   OnDeactivate();
	virtual void   OnKeyDown(const UINT dwTagNo,
			const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseUp(const UINT dwTagNo,
			const SDL_MouseButtonEvent &Button);

	CDbCommands::const_iterator  currentCommandIter;
	UINT       dwNextCommandTime;
	CDbDemo *      pDemo;

	bool           bBeforeFirstTurn;
	bool           bCanChangeSpeed;
	bool           bPaused;
	bool           bPauseNextMove;
	UINT           dwSavedMoveDuration;
	static float   fMoveRateMultiplier;
	bool           bUniformTurnSpeed;
};

#endif //...#ifndef DEMOSCREEN_H
