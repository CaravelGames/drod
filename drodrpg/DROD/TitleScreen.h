// $Id: TitleScreen.h 8919 2008-04-24 03:03:55Z mrimer $

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

#ifndef TITLESCREEN_H
#define TITLESCREEN_H

#include "DrodScreen.h"
#include "VerminEffect.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/IDSet.h>
#include <FrontEndLib/MarqueeWidget.h>

//Menu selection.
enum TitleSelection
{
	MNU_NEWGAME,
	MNU_CONTINUE,
	MNU_RESTORE,
	MNU_SETTINGS,
	MNU_HELP,
	MNU_DEMO,
	MNU_QUIT,
	MNU_BUILD,
	MNU_WHO,
	MNU_WHERE,
	MNU_TUTORIAL,
	MNU_PLAYMENU,
	MNU_MAINMENU,
	MNU_BUY,
	MNU_CHAT,

	MNU_COUNT,

	MNU_UNSPECIFIED = -1
};

class CMenuWidget;
class CTitleScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CTitleScreen();

	virtual bool   SetForActivate();

private:
	void     Animate();

	bool     ConfirmNewGame();

	void     DrawRPG1Screen();
	void     DrawRPG2Screen();

	void     AnimateCaravelLogo(SDL_Surface* pDestSurface);
	void     AnimateWaves(SDL_Surface* pDestSurface, bool update);
	void     AnimateFlag(SDL_Surface* pDestSurface, bool update);

	void     RequestNews();
	UINT     GetNextDemoID();
	virtual void   Paint(bool bUpdateRect=true);
	SCREENTYPE  ProcessMenuSelection(TitleSelection wMenuPos);

	void     LoadDemos();

	virtual void   OnBetweenEvents();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDeactivate();
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseDown(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseMotion(const UINT dwTagNo, const SDL_MouseMotionEvent &MotionEvent);
	virtual void   OnSelectChange(const UINT dwTagNo);
	bool     PollForHoldList();
	bool     PollForNews();

	void     RedrawScreen(const bool bUpdate=true);
	void     ResetCNetStatus();
	void     SetMenuOptionStatus();
	void     SetNewsText();
	void     SetTitleScreenSkin();
	virtual bool   UnloadOnDeactivate() const;

	//Vermin.
	void addParticle();
	void clickParticles(const int x, const int y);
	void resetParticle(VERMIN& v);
	void updateDirection(VERMIN &v);
	void updateParticles(SDL_Surface *pDestSurface, const int nLightX, const int nLightY);
	vector<VERMIN> vermin;
	CEffectList verminEffects;
	bool bExtraCritters;
	UINT critterKills;
	bool bBackwards;

	//Demo display.
	CIDSet   ShowSequenceDemoIDs;
	CIDSet::const_iterator  currentDemo;
	bool     bReloadDemos; //demos are shown from the selected hold only
	UINT		dwNonTutorialHoldID;

	CMenuWidget *pMenu, *pPlayMenu;
	UINT    dwFirstPaint;
	bool     bSavedGameExists;

	//Internet handles.
	UINT     wNewsHandle;
	WSTRING  wstrNewsText;
	CMarqueeWidget *pMarqueeWidget;
	bool bWaitingForHoldlist;

	//Background and Graphics.
	bool IsRPG1BG() const;
	int GetMenuXPosition(const int width) const;
	CDbHold::HoldStatus hold_status;
	UINT imageNum;
	UINT backgroundIndex;
	bool bPredarken;
	bool bReloadGraphics;

	//New game
	bool bNewGamePrompt;
};

#endif //...#ifndef TITLESCREEN_H

