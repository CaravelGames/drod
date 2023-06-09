// $Id: RoomScreen.cpp 9130 2008-08-05 04:21:26Z mrimer $

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

#include "RoomScreen.h"
#include "DrodFontManager.h"
#include "MapWidget.h"
#include "RoomWidget.h"
#include "SettingsScreen.h"
#include <FrontEndLib/LabelWidget.h>

#include "../DRODLib/SettingsKeys.h"

#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

#define BG_SURFACE      (0)
#define PARTS_SURFACE   (1)

static const UINT CX_LEFT_SIGN = 65;
static const UINT CX_MIDDLE_SIGN = 36;
static const UINT CX_RIGHT_SIGN = 68;
static const UINT CY_SIGN = 36;
static const int X_LEFT_SIGN_SRC = 1;
static const int X_MIDDLE_SIGN_SRC = 69;
static const int X_RIGHT_SIGN_SRC = 118;
static const int Y_SIGN_SRC = 384;
static const int X_SIGN = 163 + 44 * 3;
static const int Y_SIGN = 2;
static const UINT CX_SIGN = 1018 - X_SIGN;
static SDL_Rect LeftSignSource = { X_LEFT_SIGN_SRC, Y_SIGN_SRC, CX_LEFT_SIGN, CY_SIGN };
static SDL_Rect MiddleSignSource = { X_MIDDLE_SIGN_SRC, Y_SIGN_SRC, CX_MIDDLE_SIGN, CY_SIGN };
static SDL_Rect RightSignSource = { X_RIGHT_SIGN_SRC, Y_SIGN_SRC, CX_RIGHT_SIGN, CY_SIGN };

//*****************************************************************************
CRoomScreen::CRoomScreen(
//Base constructor.
//
//Params:
	const SCREENTYPE eSetType)
	: CDrodScreen(eSetType)
	, pMapWidget(NULL)
	, pScrollLabel(NULL)
	, bIsScrollVisible(false)
{
	this->imageFilenames.push_back(string("GameScreen"));
	this->imageFilenames.push_back(string("GameScreenParts"));

	this->signColor = Black;

	static const int X_MAP = 15;
	static const int Y_MAP = 578;
	static const UINT CX_MAP = 245; //130;
	static const UINT CY_MAP = 138;
	static const int X_SCROLL_LABEL = 27;
	static const int Y_SCROLL_LABEL = 218;
	static const UINT CX_SCROLL_LABEL = 215;
	static const UINT CY_SCROLL_LABEL = 310;

	//Add widgets.
	this->pMapWidget = new CMapWidget(TAG_MAP, X_MAP, Y_MAP, CX_MAP, CY_MAP, NULL, MAPSIZE_MULTIPLIER);
	if (!this->pMapWidget) throw CException("CRoomScreen: Couldn't allocate resources");
	this->pMapWidget->Disable();
	AddWidget(this->pMapWidget);

	this->pScrollLabel = new CLabelWidget(0L, X_SCROLL_LABEL, Y_SCROLL_LABEL, 
			CX_SCROLL_LABEL, CY_SCROLL_LABEL, F_Scroll, wszEmpty);
	AddWidget(this->pScrollLabel);
	this->pScrollLabel->Hide();

	//The following widgets only show up during play.
	const UINT screenType = GetScreenType();
	if (screenType != SCR_Game && screenType != SCR_Demo)
		return;

	//Player stats.
	static const int X_HP = 60;
	static const int Y_HP = 205;
	static const UINT CX_HP = 145 - X_HP;
	static const UINT CY_HP = 29;

	static const int X_ATK = X_HP;
	static const int Y_ATK = Y_HP + CY_HP;
	static const UINT CX_ATK = CX_HP;
	static const UINT CY_ATK = CY_HP;

	static const int X_DEF = X_ATK;
	static const int Y_DEF = Y_ATK + CY_ATK;
	static const UINT CX_DEF = CX_ATK;
	static const UINT CY_DEF = CY_ATK;

	static const int X_GOLD = X_DEF;
	static const int Y_GOLD = Y_DEF + CY_DEF;
	static const UINT CX_GOLD = CX_DEF;
	static const UINT CY_GOLD = CY_DEF;

	static const int X_XP = X_GOLD;
	static const int Y_XP = Y_GOLD + CY_GOLD;
	static const UINT CX_XP = CX_GOLD;
	static const UINT CY_XP = CY_GOLD;

	//Player keys.
	static const int X_YKEY = 22;
	static const int Y_YKEY = 370;
	static const UINT CX_YKEY = 30;
	static const UINT CY_YKEY = CY_XP;

	static const int X_GKEY = X_YKEY + CX_YKEY;
	static const int Y_GKEY = Y_YKEY;
	static const UINT CX_GKEY = CX_YKEY;
	static const UINT CY_GKEY = CY_YKEY;

	static const int X_BKEY = X_GKEY + CX_GKEY;
	static const int Y_BKEY = Y_GKEY;
	static const UINT CX_BKEY = CX_GKEY;
	static const UINT CY_BKEY = CY_GKEY;

	static const int X_SKEY = X_BKEY + CX_BKEY;
	static const int Y_SKEY = Y_BKEY;
	static const UINT CX_SKEY = CX_BKEY;
	static const UINT CY_SKEY = CY_BKEY;

	static const int X_SHOVEL = X_SKEY + CX_SKEY;
	static const int Y_SHOVEL = Y_SKEY;
	static const UINT CX_SHOVEL = 45;
	static const UINT CY_SHOVEL = CY_BKEY;

	//Item multiplier display.
	static const int X_ITEMMULT = 135;
	static const int Y_ITEMMULT = 355;
	static const UINT CX_ITEMMULT = 265 - X_ITEMMULT;
	static const UINT CY_ITEMMULT = 40;

	//Player equipment.
	static const int X_SWORD = 182;
	static const int Y_SWORD = 200;
	static const UINT CX_SWORD = 260 - X_SWORD;
	static const UINT CY_SWORD = 260 - Y_SWORD;

	static const int X_SHIELD = X_SWORD;
	static const int Y_SHIELD = Y_SWORD + 50;
	static const UINT CX_SHIELD = CX_SWORD;
	static const UINT CY_SHIELD = CY_SWORD;

	static const int X_ACCESSORY = X_SHIELD;
	static const int Y_ACCESSORY = Y_SHIELD + 50;
	static const UINT CX_ACCESSORY = CX_SHIELD;
	static const UINT CY_ACCESSORY = CY_SHIELD;

	CLabelWidget *pLabelWidget;
	AddWidget(new CLabelWidget(TAG_HP, X_HP, Y_HP, CX_HP, CY_HP, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_ATK, X_ATK, Y_ATK, CX_ATK, CY_ATK, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_DEF, X_DEF, Y_DEF, CX_DEF, CY_DEF, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_GOLD, X_GOLD, Y_GOLD, CX_GOLD, CY_GOLD, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_XP, X_XP, Y_XP, CX_XP, CY_XP, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_YKEY, X_YKEY, Y_YKEY, CX_YKEY, CY_YKEY, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_GKEY, X_GKEY, Y_GKEY, CX_GKEY, CY_GKEY, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_BKEY, X_BKEY, Y_BKEY, CX_BKEY, CY_BKEY, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_SKEY, X_SKEY, Y_SKEY, CX_SKEY, CY_SKEY, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_SHOVEL, X_SHOVEL, Y_SHOVEL, CX_SHOVEL, CY_SHOVEL, F_FrameCaption, wszEmpty));

	pLabelWidget = new CLabelWidget(
			TAG_ITEMMULT, X_ITEMMULT, Y_ITEMMULT, CX_ITEMMULT, CY_ITEMMULT, F_ItemMultiplier, wszEmpty, false, 0, WT_Label, true);
	pLabelWidget->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pLabelWidget);

	pLabelWidget = new CLabelWidget(
			TAG_SWORD, X_SWORD, Y_SWORD, CX_SWORD, CY_SWORD, F_InvSlotText, wszEmpty);
	pLabelWidget->SetClickable(true);
	AddWidget(pLabelWidget);
	pLabelWidget = new CLabelWidget(
			TAG_SHIELD, X_SHIELD, Y_SHIELD, CX_SHIELD, CY_SHIELD, F_InvSlotText, wszEmpty);
	pLabelWidget->SetClickable(true);
	AddWidget(pLabelWidget);
	pLabelWidget = new CLabelWidget(
			TAG_ACCESSORY, X_ACCESSORY, Y_ACCESSORY, CX_ACCESSORY, CY_ACCESSORY, F_InvSlotText, wszEmpty);
	pLabelWidget->SetClickable(true);
	AddWidget(pLabelWidget);

	//Monster stats.
	static const int X_MONNAME = 22;
	static const int Y_MONNAME = 425;
	static const UINT CX_MONNAME = 160 - X_MONNAME;
	static const UINT CY_MONNAME = 25;

	static const int X_MONHP = X_HP;
	static const int Y_MONHP = 452;
	static const UINT CX_MONHP = 160 - X_MONHP;
	static const UINT CY_MONHP = CY_HP;

	static const int X_MONATK = X_MONHP;
	static const int Y_MONATK = Y_MONHP + CY_MONHP;
	static const UINT CX_MONATK = CX_MONHP;
	static const UINT CY_MONATK = CY_MONHP;

	static const int X_MONDEF = X_MONATK;
	static const int Y_MONDEF = Y_MONATK + CY_MONATK;
	static const UINT CX_MONDEF = CX_MONATK;
	static const UINT CY_MONDEF = CY_MONATK;

	AddWidget(new CLabelWidget(TAG_MONNAME, X_MONNAME, Y_MONNAME, CX_MONNAME, CY_MONNAME, F_Small, wszEmpty));
	AddWidget(new CLabelWidget(TAG_MONHP, X_MONHP, Y_MONHP, CX_MONHP, CY_MONHP, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_MONATK, X_MONATK, Y_MONATK, CX_MONATK, CY_MONATK, F_FrameCaption, wszEmpty));
	AddWidget(new CLabelWidget(TAG_MONDEF, X_MONDEF, Y_MONDEF, CX_MONDEF, CY_MONDEF, F_FrameCaption, wszEmpty));
}

//*****************************************************************************
void CRoomScreen::SetMusicStyle(
//Changes the music to match style.  If music is already matching style, nothing
//will happen.
//
//Params:
	WSTRING style, const UINT wMood, const UINT fadeDuration) //[default=3000ms]
{
	if (g_pTheDBM->IsStyleFrozen())
		return; //don't change music if style is not to change

	ASSERT(g_pTheDBM);
	g_pTheDBM->ConvertStyle(style);

	WSTRING wMoodText;
	switch (wMood)
	{
//		case SONG_AMBIENT: case SONG_ATTACK:
		case SONG_PUZZLE: case SONG_EXIT:
			UTF8ToUnicode(moodText[wMood], wMoodText);
		break;
		default: ASSERT(!"Invalid style mood"); break;
	}
	style += wMoodText;

	//Fade to next song in list and update play order.
	CFiles f;
	list<WSTRING> songlist;
	if (f.GetGameProfileString(INISection::Songs, style.c_str(), songlist))
	{
		ASSERT(g_pTheSound);
		g_pTheSound->CrossFadeSong(&songlist, fadeDuration);
		f.WriteGameProfileString(INISection::Songs, style.c_str(), songlist);
	}
}

//*****************************************************************************
void CRoomScreen::SetSignText(
//Set text that appears on sign.
//
//Params:
	const WCHAR *pwczSetText)  //(in)   New text.  NULL will make the sign 
								//    disappear on next paint.
{
	this->wstrSignText = pwczSetText ? pwczSetText : wszEmpty;
}

//*****************************************************************************
void CRoomScreen::PaintBackground()
//Paint background.
{
	ASSERT(this->images[BG_SURFACE]);
	SDL_BlitSurface(this->images[BG_SURFACE], NULL, GetDestSurface(), NULL);
}

//*****************************************************************************
void CRoomScreen::PaintScroll(const bool bUpdateRect) //[default=true]
//Paint the scroll.
{
	static const int X_SCROLL = 5;
	static const int Y_SCROLL = 185;
	static const UINT CX_SCROLL = 270;
	static const UINT CY_SCROLL = 380;
	static const int X_SRC_SCROLL = 2;
	static const int Y_SRC_SCROLL = 2;
	static SDL_Rect ScreenRect = {X_SCROLL, Y_SCROLL, CX_SCROLL, CY_SCROLL};
	static SDL_Rect ScrollRect = {X_SRC_SCROLL, Y_SRC_SCROLL, CX_SCROLL, CY_SCROLL};

	ASSERT(this->images[BG_SURFACE]);
	ASSERT(this->images[PARTS_SURFACE]);

	SDL_Surface *pDestSurface = GetDestSurface();

	if (this->bIsScrollVisible)
	{
		ASSERT(this->images[PARTS_SURFACE]);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &ScrollRect, 
				pDestSurface, &ScreenRect);
		this->pScrollLabel->Show();
		this->pScrollLabel->Paint();
	}
	else
	{
		ASSERT(this->images[BG_SURFACE]);
		SDL_BlitSurface(this->images[BG_SURFACE], &ScreenRect, 
				pDestSurface, &ScreenRect);
		this->pScrollLabel->Hide();
	}

	if (bUpdateRect)
		UpdateRect(ScreenRect);
}

//*****************************************************************************
SDL_Rect& CRoomScreen::GetEntireSignRect() const
{
	static SDL_Rect EntireSign = { X_SIGN, Y_SIGN, CX_SIGN, CY_SIGN };

	return EntireSign;
}

//*****************************************************************************
void CRoomScreen::PaintSign()
//Paint the sign.
{
	SDL_Rect& EntireSign = GetEntireSignRect();

	ASSERT(this->images[BG_SURFACE]);
	ASSERT(this->images[PARTS_SURFACE]);

	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit background over the entire possible area the sign could cover.
	SDL_BlitSurface(this->images[BG_SURFACE], &EntireSign, pDestSurface, &EntireSign);

	//Is there text to display?
	if (this->wstrSignText.size()) //Yes.
	{
		UINT wMiddleCount, wTextWidth, wTextHeight;

		//Figure out how wide it will be.
		g_pTheFM->GetTextWidthHeight(F_Sign, this->wstrSignText.c_str(), wTextWidth, wTextHeight);
		ASSERT(wTextWidth > 0);
			
		//Figure how many middle sign parts will be needed to display the text.
		wMiddleCount = (wTextWidth / CX_MIDDLE_SIGN);
		if (wTextWidth % CX_MIDDLE_SIGN != 0) ++wMiddleCount; //Round up.

		//If the text is too large to fit, then exit without displaying it.
		UINT wSignWidth = CX_LEFT_SIGN + (wMiddleCount * CX_MIDDLE_SIGN) +
				CX_RIGHT_SIGN;
		if (wSignWidth > CX_SIGN)
		{
			//Sign width too large -- truncate sign text to fit.
			wSignWidth = CX_SIGN;
			wMiddleCount = (CX_SIGN - CX_LEFT_SIGN - CX_RIGHT_SIGN) / CX_MIDDLE_SIGN;
		}

		//Blit left part of sign.
		SDL_Rect Dest = MAKE_SDL_RECT(X_SIGN + ((CX_SIGN - wSignWidth) / 2), Y_SIGN, 
				CX_LEFT_SIGN, CY_SIGN);
		Uint32 TransparentColor = SDL_MapRGB(this->images[PARTS_SURFACE]->format, 226, 0, 0);
		SetColorKey(this->images[PARTS_SURFACE], SDL_TRUE, TransparentColor);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &LeftSignSource, pDestSurface, &Dest);
		SetColorKey(this->images[PARTS_SURFACE], 0, TransparentColor);

		//Blit middle parts of sign.
		Dest.x += CX_LEFT_SIGN;
		Dest.w = CX_MIDDLE_SIGN;
		for (UINT wI = 0; wI < wMiddleCount; ++wI)
		{
			SDL_BlitSurface(this->images[PARTS_SURFACE], &MiddleSignSource, pDestSurface, &Dest);
			Dest.x += CX_MIDDLE_SIGN;
		}

		//Blit right part of sign.
		Dest.w = CX_RIGHT_SIGN;
		SetColorKey(this->images[PARTS_SURFACE], SDL_TRUE, TransparentColor);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &RightSignSource, pDestSurface, &Dest);

		//Draw text on sign.
		int xText = X_SIGN + (int(CX_SIGN - wTextWidth) / 2);
		if (xText < X_SIGN + (int)CX_LEFT_SIGN/2)
			xText = X_SIGN + CX_LEFT_SIGN/2;
		int yText = Y_SIGN + ((CY_SIGN - wTextHeight) / 2);
		if (yText < Y_SIGN)
			yText = Y_SIGN;
		yText -= 3;     //kludge -- this font text is normally drawn too low

		//Set color.
		const SDL_Color origColor = g_pTheFM->GetFontColor(F_Sign);
		g_pTheFM->SetFontColor(F_Sign, this->signColor);

		g_pTheFM->DrawTextXY(F_Sign, this->wstrSignText.c_str(), pDestSurface, xText, yText,
				CX_SIGN - CX_LEFT_SIGN);

		g_pTheFM->SetFontColor(F_Sign, origColor);
		SetColorKey(this->images[PARTS_SURFACE], 0, TransparentColor);
	}

	UpdateRect(EntireSign);
}

//*****************************************************************************
SDL_Keycode CRoomScreen::GetKeysymForCommand(const UINT wCommand) const
//Returns: keysym currently set for indicated command
{
	for (std::map<SDL_Keycode,int>::const_iterator it = KeysymToCommandMap.begin(); it != KeysymToCommandMap.end(); ++it)
		if (it->second == (int)wCommand)
			return it->first;

	ASSERT(!"Command not assigned");
	return SDLK_UNKNOWN;
}

//*****************************************************************************
void CRoomScreen::InitKeysymToCommandMap(
//Set the keysym-to-command map with values from player settings that will determine
//which commands correspond to which keys.
//
//Params:
	CDbPackedVars &PlayerSettings)   //(in)   Player settings to load from.
{
	//Clear the map.
	this->KeysymToCommandMap.clear();

	//Check whether default is for keyboard or laptop.
	CFiles Files;
	string strKeyboard;
	UINT wKeyboard = 0;	//default to numpad
	if (Files.GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
	{
		wKeyboard = atoi(strKeyboard.c_str());
		if (wKeyboard > 1) wKeyboard = 0;	//invalid setting
	}

	//Get key command values from current player settings.
	static const int commands[InputCommands::DCMD_Count] = {
		CMD_NW, CMD_N, CMD_NE,
		CMD_W, CMD_WAIT, CMD_E,
		CMD_SW, CMD_S, CMD_SE,
		CMD_C, CMD_CC, CMD_RESTART, CMD_UNDO,
		CMD_BATTLE_KEY, CMD_USE_ACCESSORY,
		CMD_LOCK, CMD_EXEC_COMMAND, CMD_SCORE_KEY
	};

	for (UINT wIndex = 0; wIndex < InputCommands::DCMD_Count; ++wIndex) {
		const int nKey = PlayerSettings.GetVar(InputCommands::COMMANDNAME_ARRAY[wIndex],
				COMMANDKEY_ARRAY[wKeyboard][wIndex]);
		const bool bInvalidSDL1mapping = nKey >= 128 && nKey <= 323;
		this->KeysymToCommandMap[bInvalidSDL1mapping ? COMMANDKEY_ARRAY[wKeyboard][wIndex] : nKey] = commands[wIndex];
	}
}

//*****************************************************************************
int CRoomScreen::GetCommandForKeysym(const SDL_Keycode& sym) const
{
	std::map<SDL_Keycode,int>::const_iterator it = this->KeysymToCommandMap.find(sym);
	if (it != this->KeysymToCommandMap.end())
			return it->second;

	return CMD_UNSPECIFIED;
}

