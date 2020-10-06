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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TemporalMoveEffect.h"
#include "RoomWidget.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/Colors.h>

const Uint32 START_DELAY = 500;
const Uint32 DISPLAY_DURATION = 750;
const Uint32 END_DELAY = 500;

//********************************************************************************
CTemporalMoveEffect::CTemporalMoveEffect(
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CMoveCoord &SetCoord, //(in)   tile location
	const UINT wTI,
	const UINT type)
	: CAnimatedTileEffect(pSetWidget, CCoord(SetCoord.wX,SetCoord.wY),
			DISPLAY_DURATION, wTI, true, type)
	, startDelay(START_DELAY)
	, endDelay(END_DELAY)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	ASSERT(pRoom->GetCurrentGame());
	this->wValidTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;

	this->startX = this->wX;
	this->startY = this->wY;
	this->deltaX = nGetOX(SetCoord.wO) * CBitmapManager::CX_TILE;
	this->deltaY = nGetOY(SetCoord.wO) * CBitmapManager::CY_TILE;
}

//********************************************************************************
bool CTemporalMoveEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	if (!pRoom) return false;
	const CCurrentGame *pGame = pRoom->GetCurrentGame();
	if (!pGame || pGame->wTurnNo != this->wValidTurn)
		return false;

	const Uint32 dwElapsed = TimeElapsed();
	if (dwElapsed < this->startDelay || dwElapsed >= this->startDelay + this->dwDuration) {
		//Effect persists for a while, to prevent new one from being created right away.
		this->dirtyRects[0].w = this->dirtyRects[0].h = 0;

		const Uint32 totalDuration = this->startDelay + this->dwDuration + this->endDelay;
		return dwElapsed < totalDuration;
	}

	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	const float percent = (dwElapsed - this->startDelay) / float(this->dwDuration);
	const Uint8 opacity = static_cast<Uint8>(255 * (1.0f-percent));

	const UINT wThisX = this->startX + int(deltaX * percent);
	const UINT wThisY = this->startY + int(deltaY * percent);
	
	SDL_Rect BlitRect = MAKE_SDL_RECT(wThisX, wThisY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	SDL_Rect WidgetRect = MAKE_SDL_RECT(0, 0, 0, 0);
	this->pRoomWidget->GetRect(WidgetRect);
	if (!CWidget::ClipRectToRect(BlitRect, WidgetRect))
		return true;

	g_pTheBM->BlitTileImagePart(
		this->wTileNo, 
		BlitRect.x, BlitRect.y,
		BlitRect.x - wThisX, BlitRect.y - wThisY, BlitRect.w, BlitRect.h,
		pDestSurface, this->bUseLightLevel, opacity);

	SDL_Rect clipRect = MAKE_SDL_RECT(BlitRect.x, BlitRect.y,
		BlitRect.w, BlitRect.h);
	this->dirtyRects[0] = clipRect;

	//Continue effect.
	return true;
}
