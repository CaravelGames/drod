// $Id: DamagePreviewEffect.cpp $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "DamagePreviewEffect.h"
#include "RoomWidget.h"
#include "DrodFontManager.h"
#include "../DRODLib/Combat.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>
#include <FrontEndLib/Screen.h>

//********************************************************************************
CBonusPreviewEffect::CBonusPreviewEffect(
	//Constructor.
	//
	//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const UINT wX, const UINT wY,
	const int value)
	: CEffect(pSetWidget, (UINT)-1, EDAMAGEPREVIEW)
	, wX(wX), wY(wY)
	, pTextSurface(NULL)
	, YOFFSET(-29)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);

	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	if (pRoom) {
		CCurrentGame* pGame = pRoom->GetCurrentGame();
		ASSERT(pGame);
		this->wValidTurn = pGame->wTurnNo;

		//Prepare effect.
		PrepWidgetForValue(value);
	} else {
		this->wValidTurn = 0;
	}
}

CBonusPreviewEffect::CBonusPreviewEffect(CWidget* pSetWidget, int yOffset)
	: CEffect(pSetWidget, (UINT)-1, EDAMAGEPREVIEW)
	, pTextSurface(NULL)
	, YOFFSET(yOffset)
{ }

//********************************************************************************
CBonusPreviewEffect::~CBonusPreviewEffect()
{
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
}

//********************************************************************************
bool CBonusPreviewEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
//Updates the effect state & dirty rects
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	return wTurnNow == this->wValidTurn;
}

//********************************************************************************
void CBonusPreviewEffect::Draw(SDL_Surface& destSurface)
//Draw the effect.
{
	//Clip screen surface to widget to prevent overrun.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(&destSurface, &OwnerRect);

	int destX = this->pRoomWidget->GetX() + this->wX * CBitmapManager::CX_TILE;
	int destY = this->pRoomWidget->GetY() + this->wY * CBitmapManager::CY_TILE;

	this->dirtyRects[0].x = destX;
	this->dirtyRects[0].y = destY;

	SDL_Rect SrcRect = MAKE_SDL_RECT(0, this->YOFFSET, this->dirtyRects[0].w, this->dirtyRects[0].h);
	SDL_Rect ScreenRect = MAKE_SDL_RECT(destX, destY, this->dirtyRects[0].w, this->dirtyRects[0].h);
	SDL_BlitSurface(this->pTextSurface, &SrcRect, &destSurface, &ScreenRect);

	//Unclip screen surface.
	SDL_SetClipRect(&destSurface, NULL);
}

//*****************************************************************************
void CBonusPreviewEffect::PrepWidgetForValue(const int value)
{
	CDbRoom* pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);

	CCurrentGame* pGame = pRoom->GetCurrentGame();
	ASSERT(pGame);

	WSTRING wstr;
	if (value > 999999) {
		//Magnitude conversion
		char czValue[16];
		sprintf(czValue, value < 100000000 ? "%.3f" : "%.1f", value / 1000000.f);

		//trim trailing zeros
		int len = strlen(czValue) - 1;
		while (len && czValue[len] == '0')
			czValue[len--] = '\0';
		if (czValue[len] == '.')
			czValue[len--] = '\0';

		//append units
		strcat(czValue, "M");

		wstr += UTF8ToUnicode(czValue);
	} else {
		WCHAR temp[16];
		wstr += _itoW(value, temp, 10);
	}

	static const UINT eFontType = F_DamagePreview;
	static const UINT outlineWidth = 1;

	UINT wLineW, wLineH;
	g_pTheFM->GetTextRectHeight(eFontType, wstr.c_str(),
		CBitmapManager::CX_TILE * 2, wLineW, wLineH);

	//Render text to internal surface to avoid re-rendering each frame.
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
	this->pTextSurface = CBitmapManager::ConvertSurface(
		SDL_CreateRGBSurface(SDL_SWSURFACE, wLineW, wLineH, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
	ASSERT(this->pTextSurface);

	const Uint32 bg_color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_FillRect(this->pTextSurface, NULL, bg_color);

	//Determine color based on damage amount
	SDL_Color damage = { 255, 0, 0 };
	SDL_Color no_effect = { 220, 220, 220 };
	SDL_Color bonus = { 64, 255, 255 };
	SDL_Color color;
	if (value < 0) {
		color = damage;
	} else if (value == 0) {
		color = no_effect;
	} else {
		color = bonus;
	}

	const SDL_Color origColor = g_pTheFM->GetFontColor(eFontType);
	g_pTheFM->SetFontColor(eFontType, color);

	//Draw text (outlined, w/ anti-aliasing).
	g_pTheFM->DrawTextToRect(eFontType, wstr.c_str(),
		0, 0, wLineW, wLineH, this->pTextSurface);

	g_pTheFM->SetFontColor(eFontType, origColor);

	SDL_Rect Dest = MAKE_SDL_RECT(0, 0, wLineW, wLineH - this->YOFFSET);
	this->dirtyRects.push_back(Dest);
}

//********************************************************************************
CDamagePreviewEffect::CDamagePreviewEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const CMonster *pMonster)     //(in)   Enemy to display damage preview for.
	: CBonusPreviewEffect(pSetWidget, 3)
	, pMonster(const_cast<CMonster*>(pMonster))  //Though non-const, everything in this effect should leave monster state as-is
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	ASSERT(pMonster);

	this->wX = this->pMonster->wX;
	this->wY = this->pMonster->wY;

	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	if (pRoom) {
		CCurrentGame* pGame = pRoom->GetCurrentGame();
		ASSERT(pGame);
		this->wValidTurn = pGame->wTurnNo;

		//Prepare effect.
		PrepWidget();
	} else {
		this->wValidTurn = 0;
	}
}

//********************************************************************************
bool CDamagePreviewEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
//Updates the effect state & dirty rects
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	if (wTurnNow != this->wValidTurn)
		return false;

	if (!this->pMonster->IsAlive())
		return false;

	return true;
}

//*****************************************************************************
void CDamagePreviewEffect::PrepWidget()
{
	ASSERT(this->pMonster);

	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);

	CCurrentGame* pGame = pRoom->GetCurrentGame();
	ASSERT(pGame);
	
	CMonster *pOrigMonster = this->pMonster;
	CMonster *pOwningMonster = pOrigMonster->GetOwningMonster();

	const CSwordsman& player = *pGame->pPlayer;
	CCombat combat(pGame, pOwningMonster, player.HasSword(),
		player.wX, player.wY,
		pOrigMonster->wX, pOrigMonster->wY);

	WSTRING wstr;
	const UINT playerHP = pGame->pPlayer->st.HP;
	const int damage = combat.GetExpectedDamage();
	if (damage == -1) {
		//player cannot harm enemy
		UTF8ToUnicode("????", wstr);
	} else {
		ASSERT(damage >= 0);

		if (combat.IsExpectedDamageApproximate())
			wstr += wszTilde;

		//Magnitude conversion
		if (damage > 999999) {
			char czDamage[16];
			sprintf(czDamage, damage < 100000000 ? "%.3f" : "%.1f", damage / 1000000.f);

			//trim trailing zeros
			int len = strlen(czDamage) - 1;
			while (len && czDamage[len] == '0')
				czDamage[len--] = '\0';
			if (czDamage[len] == '.')
				czDamage[len--] = '\0';

			//append units
			strcat(czDamage, "M");

			wstr += UTF8ToUnicode(czDamage);
		} else {
			WCHAR temp[16];
			wstr += _itoW(damage, temp, 10);
		}
	}

	static const UINT eFontType = F_DamagePreview;
	static const UINT outlineWidth = 1;
	
	UINT wLineW, wLineH;
	g_pTheFM->GetTextRectHeight(eFontType, wstr.c_str(),
		CBitmapManager::CX_TILE * 2, wLineW, wLineH);

	//Render text to internal surface to avoid re-rendering each frame.
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
	this->pTextSurface = CBitmapManager::ConvertSurface(
		SDL_CreateRGBSurface(SDL_SWSURFACE, wLineW, wLineH, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
	ASSERT(this->pTextSurface);

	const Uint32 bg_color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_FillRect(this->pTextSurface, NULL, bg_color);

	//Determine color based on damage amount
	SDL_Color no_damage = { 255, 255, 255 };
	SDL_Color neither_can_harm = { 255, 156, 255 };
	SDL_Color cannot_harm_enemy = { 255, 64, 64 };
	SDL_Color cannot_harm_player = { 172, 172, 172 };
	SDL_Color death = { 192, 0, 0 };
	SDL_Color color = { 255, 255, 0 }; //yellow = default/middle
	if (damage < 0) {
		color = combat.MonsterCanHarmPlayer(this->pMonster) ? cannot_harm_enemy : neither_can_harm;
	} else if (damage == 0) {
		color = combat.MonsterCanHarmPlayer(this->pMonster) ? no_damage : cannot_harm_player;
	} else if (UINT(damage) >= playerHP) {
		color = death;
	} else {
		const float fPercent = damage / float(playerHP);
		static const float fMidpoint = 0.2f;
		if (fPercent <= fMidpoint) {
			//Greenish hue (low damage)
			color.r = Uint8(255 * fPercent / fMidpoint); //weaken red value
		} else {
			//Reddish hue (high damage)
			const float fFactor = (fPercent - fMidpoint) / (1.0f - fMidpoint);
			color.g = Uint8(255 * (1.0f - fFactor)); //weaken green value
		}
	}

	const SDL_Color origColor = g_pTheFM->GetFontColor(eFontType);
	g_pTheFM->SetFontColor(eFontType, color);

	//Draw text (outlined, w/ anti-aliasing).
	g_pTheFM->DrawTextToRect(eFontType, wstr.c_str(),
		0, 0, wLineW, wLineH, this->pTextSurface);

	g_pTheFM->SetFontColor(eFontType, origColor);

	SDL_Rect Dest = MAKE_SDL_RECT(0, 0, wLineW, wLineH - this->YOFFSET);
	this->dirtyRects.push_back(Dest);
}
