// $Id: MovementOrderHintEffect.h $

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
 * Portions created by the Initial Developer are Copyright (C) 2025
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "MovementOrderHintEffect.h"
#include "RoomWidget.h"
#include "DrodFontManager.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>
#include <FrontEndLib/Screen.h>

static map<int, SDL_Surface*> s_surfaceCache;

//********************************************************************************
CMovementOrderHintEffect::CMovementOrderHintEffect(
	CWidget* pSetWidget, const CMonster* pMonster, int moveOrder)
	: CEffect(pSetWidget, (UINT)-1, EMOVEORDERHINT)
	, pMonster(const_cast<CMonster*>(pMonster))  //Though non-const, everything in this effect should leave monster state as-is
	, wMoveOrder(moveOrder)
	, pTextSurface(NULL)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	ASSERT(pMonster);

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
CMovementOrderHintEffect::~CMovementOrderHintEffect()
{
	//Don't free pTextSurface here, as that will invalidate the cache entry.
}

//********************************************************************************
void CMovementOrderHintEffect::ClearSurfaceCache()
//Free all surfaces in s_surfaceCache, then clear it.
{
	for (map<int, SDL_Surface*>::iterator iter = s_surfaceCache.begin();
		iter != s_surfaceCache.end(); ++iter)
	{
		SDL_FreeSurface(iter->second);
	}
	s_surfaceCache.clear();
}

//********************************************************************************
bool CMovementOrderHintEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	ASSERT(this->pRoomWidget->GetRoom());
	ASSERT(this->pRoomWidget->GetRoom()->GetCurrentGame());

	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	if (wTurnNow != this->wValidTurn)
		return false;

	if (!this->pMonster->IsAlive())
		return false;

	return true;
}

//********************************************************************************
void CMovementOrderHintEffect::Draw(SDL_Surface& destSurface)
{
	//Clip screen surface to widget to prevent overrun.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(&destSurface, &OwnerRect);

	ASSERT(this->pMonster);

	CCueEvents Ignored;
	int destX = this->pRoomWidget->GetX() + this->pMonster->wX * CBitmapManager::CX_TILE;
	int destY = this->pRoomWidget->GetY() + this->pMonster->wY * CBitmapManager::CY_TILE;

	this->dirtyRects[0].x = destX;
	this->dirtyRects[0].y = destY;

	SDL_Rect SrcRect = MAKE_SDL_RECT(0, 0, this->dirtyRects[0].w, this->dirtyRects[0].h);
	SDL_Rect ScreenRect = MAKE_SDL_RECT(destX, destY, this->dirtyRects[0].w, this->dirtyRects[0].h);
	SDL_BlitSurface(this->pTextSurface, &SrcRect, &destSurface, &ScreenRect);

	//Unclip screen surface.
	SDL_SetClipRect(&destSurface, NULL);
}

//********************************************************************************
void CMovementOrderHintEffect::PrepWidget()
{
	this->pTextSurface = GetSurfaceForOrder(wMoveOrder);
	SDL_Rect Dest = MAKE_SDL_RECT(0, 0, this->pTextSurface->w, this->pTextSurface->h);
	this->dirtyRects.push_back(Dest);
}

//********************************************************************************
SDL_Surface* CMovementOrderHintEffect::GetSurfaceForOrder(int order)
//Get a surface with the rendered text for the given movement order position.
//Since rendering text is relatively expensive, surfaces are cached to reduce the
//amount of renders required.
{
	//At some point, these hints aren't useful
	if (order > 1000)
		order = 1000;

	//Try getting from the cache first
	map<int, SDL_Surface*>::iterator finder = s_surfaceCache.find(order);
	if (finder != s_surfaceCache.end()) {
		return finder->second;
	}

	WSTRING wstr; 
	if (order == 1000) {
		wstr = L">999";
	} else {
		wstr = std::to_wstring(order);
	}
	static const UINT eFontType = F_MovementOrderHint;
	UINT wLineW, wLineH;
	g_pTheFM->GetTextRectHeight(eFontType, wstr.c_str(),
		CBitmapManager::CX_TILE * 2, wLineW, wLineH);

	//Render text to a cached surface to avoid having to rerender each turn.
	SDL_Surface* pSurface = CBitmapManager::ConvertSurface(
		SDL_CreateRGBSurface(SDL_SWSURFACE, wLineW, wLineH, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
	ASSERT(pSurface);

	const Uint32 bg_color = SDL_MapRGBA(pSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_FillRect(pSurface, NULL, bg_color);

	//Draw text (outlined, w/ anti-aliasing).
	g_pTheFM->DrawTextToRect(eFontType, wstr.c_str(),
		0, 0, wLineW, wLineH, pSurface);

	s_surfaceCache[order] = pSurface;
	return pSurface;
}
