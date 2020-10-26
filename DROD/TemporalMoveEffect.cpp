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
const Uint32 TOTAL_DURATION = START_DELAY + DISPLAY_DURATION + END_DELAY;

//********************************************************************************
CTemporalMoveEffect::CTemporalMoveEffect(
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CMoveCoord &SetCoord, //(in)   tile location
	const UINT wTI,
	const bool isBump,
	const UINT type)
	: CAnimatedTileEffect(pSetWidget, CCoord(SetCoord.wX,SetCoord.wY),
		TOTAL_DURATION, wTI, true, type)
	, startDelay(START_DELAY)
	, endDelay(END_DELAY)
	, isBump(isBump)
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
bool CTemporalMoveEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	const CDbRoom* pRoom = this->pRoomWidget->GetRoom();
	if (!pRoom) 
		return false;

	const CCurrentGame* pGame = pRoom->GetCurrentGame();
	if (!pGame || pGame->wTurnNo != this->wValidTurn)
		return false;

	const UINT animationElapsed = dwTimeElapsed - this->startDelay;
	if (animationElapsed >= DISPLAY_DURATION) {
		//Effect persists for a while, to prevent new one from being created right away.
		this->dirtyRects[0].w = this->dirtyRects[0].h = 0;
		this->nOpacity = 0;

		const Uint32 totalDuration = this->startDelay + this->dwDuration + this->endDelay;
		return dwTimeElapsed < totalDuration;
	}

	const float elapsedFraction = animationElapsed / float(DISPLAY_DURATION);
	const float transparencyPercent = elapsedFraction > 0.5f
		? (elapsedFraction - 0.5f) * 2
		: 0;
	const float positionPercent = (this->isBump && elapsedFraction > 0.5)
		? 1 - elapsedFraction // Bump commands should look differently
		: elapsedFraction;

	this->nOpacity = static_cast<Uint8>(255 * (1.0f - transparencyPercent));

	this->wDrawX = this->startX + int(deltaX * positionPercent);
	this->wDrawY = this->startY + int(deltaY * positionPercent);

	this->blitRect = MAKE_SDL_RECT(this->wDrawX, this->wDrawY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);

	SDL_Rect WidgetRect = MAKE_SDL_RECT(0, 0, 0, 0);
	this->pRoomWidget->GetRect(WidgetRect);

	if (!CWidget::ClipRectToRect(this->blitRect, WidgetRect)) {
		this->nOpacity = 0;
		return true;
	}

	this->dirtyRects[0] = this->blitRect;

	return true;
}

//********************************************************************************
void CTemporalMoveEffect::Draw(SDL_Surface& destSurface)
{
	if (this->nOpacity > 0)
		g_pTheBM->BlitTileImagePart(
			this->wTileNo,
			this->blitRect.x, this->blitRect.y,
			this->blitRect.x - this->wDrawX, this->blitRect.y - this->wDrawY,
			this->blitRect.w, this->blitRect.h,
			&destSurface, this->bUseLightLevel, this->nOpacity);
}
