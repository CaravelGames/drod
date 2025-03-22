// $Id: RoomScreen.h 9130 2008-08-05 04:21:26Z mrimer $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef ROOMSCREEN_H
#define ROOMSCREEN_H

#include "DrodBitmapManager.h"
#include "DrodScreen.h"
#include "DrodScreenManager.h"
#include "DrodSound.h"
#include "../DRODLib/DbPackedVars.h"

//Base class for displaying a room,
//including basically everything that goes on "gamescreen.png"
//(i.e. RoomWidget, Scroll, Map, and Sign).

static const UINT TAG_ROOM = 2000;
static const UINT TAG_MAP = 2001;

static const UINT TAG_HP  = 2010;
static const UINT TAG_ATK = 2011;
static const UINT TAG_DEF = 2012;
static const UINT TAG_GOLD = 2013;
static const UINT TAG_XP = 2014;
static const UINT TAG_YKEY = 2015;
static const UINT TAG_GKEY = 2016;
static const UINT TAG_BKEY = 2017;
static const UINT TAG_SKEY = 2018;
static const UINT TAG_ITEMMULT = 2019;

static const UINT TAG_MONNAME = 2020;
static const UINT TAG_MONHP = 2021;
static const UINT TAG_MONATK = 2022;
static const UINT TAG_MONDEF = 2023;

static const UINT TAG_SHOVEL = 2024;

static const UINT TAG_SWORD = 2025;
static const UINT TAG_SHIELD = 2026;
static const UINT TAG_ACCESSORY = 2027;

class CMapWidget;
class CRoomWidget;
class CLabelWidget;
class CCurrentGame;
class CRoomScreen : public CDrodScreen
{
public:
	static void    SetMusicStyle(WSTRING style, const UINT wMood, const UINT fadeDuration=3000);
	int            GetCommandForInputKey(const InputKey& inputKey) const;

protected:
	friend class CScreenManager;

	CRoomScreen(const SCREENTYPE eSetType);
	virtual ~CRoomScreen() { }

	SDL_Rect& GetEntireSignRect() const;
	InputKey GetInputKeyForCommand(const UINT wCommand) const;
	void     HideScroll() {this->bIsScrollVisible = false;}
	void     InitKeysymToCommandMap(CDbPackedVars &PlayerSettings);
	void     PaintBackground();
	void     PaintScroll(const bool bUpdateRect=true);
	void     PaintSign();
	void     SetSignText(const WCHAR *pwczSetText);
	void     ShowScroll() {this->bIsScrollVisible = true;}

	//These are accessed by CDemoScreen.
	CMapWidget *      pMapWidget;
	CLabelWidget *    pScrollLabel;

	WSTRING           wstrSignText;
	SDL_Color         signColor;    //color of text on sign

	bool              bIsScrollVisible;

private:
	std::map<InputKey, int> InputKeyToCommandMap;
};

#endif //...#ifndef ROOMSCREEN_H

