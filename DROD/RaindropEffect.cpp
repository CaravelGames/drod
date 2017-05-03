// $Id: SnowflakeEffect.cpp 8139 2007-08-25 06:49:36Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005, 2011
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "RaindropEffect.h"
#include "RoomWidget.h"
#include "WadeEffect.h"
#include "TileImageConstants.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/DbRooms.h"
#include <FrontEndLib/BitmapManager.h>

//All instanced rain drops have a similar horizontal drift (wind)
float CRaindropEffect::fXDrift = 0.0;

const UINT RAIN_TYPES = 2;

//*****************************************************************************
CRaindropEffect::CRaindropEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in) parent widget
	bool bHasted)
	: CEffect(pSetWidget, ERAINDROP)
	, bHasted(bHasted)
	, pRoomWidget(NULL)
{
	if (pSetWidget->GetType() == WT_Room)
		this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	ASSERT(pSetWidget);
	pSetWidget->GetRect(this->screenRect);

	const int nX = this->screenRect.x + RAND(this->screenRect.w);
	const int nY = this->screenRect.y; // start at top
	this->fX = static_cast<float>(nX);
	this->fY = static_cast<float>(nY);
	this->fGoalY = this->fY + RAND(this->screenRect.h) + 100; //have raindrops disappear slightly below screen center, on average
	this->dwTimeOfLastMove = this->dwTimeStarted = SDL_GetTicks();

	this->wType = RAND(RAIN_TYPES);

	//There is always one dirty rect.
	SDL_Rect rect = {static_cast<Sint16>(this->fX), static_cast<Sint16>(this->fY), 0, 0};
	this->dirtyRects.push_back(rect);

	RequestRetainOnClear(); //this effect doesn't depend on room state
}

//*****************************************************************************
bool CRaindropEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();

	if (this->fY >= this->fGoalY) {
		//Create a splash where raindrop hits water.
		const CDbRoom *pRoom = this->pRoomWidget ? this->pRoomWidget->GetCurrentGame()->pRoom : NULL;
		if (pRoom) {
			const CCoord coord(
				(Sint16(this->fX) - this->screenRect.x) / CBitmapManager::CX_TILE,
				(Sint16(this->fY) - this->screenRect.y) / CBitmapManager::CY_TILE);
			if (coord.wX < pRoom->wRoomCols && coord.wY < pRoom->wRoomRows) {
				const UINT wOTile = pRoom->GetOSquare(coord.wX, coord.wY);
				if (bIsWater(wOTile)) {
					this->pRoomWidget->AddTLayerEffect(new CWadeEffect(this->pRoomWidget, coord));
				}
			}
		}
	
		return false;
	}

	//Update real position in real time.
	const Uint32 dwNow = SDL_GetTicks();
	Uint32 dwFrameTimeMS = dwNow <= this->dwTimeOfLastMove ? 1 :
			dwNow - this->dwTimeOfLastMove;
	if (dwFrameTimeMS > 500) //arbitrary threshold to identify when effects are frozen
		dwFrameTimeMS = 1;
	this->dwTimeOfLastMove = dwNow;
  
	//Downward fall movement pattern.
	const float fMultiplier = float((this->bHasted ? 0.5f : 1.0f) * dwFrameTimeMS);
	this->fY += fMultiplier;

	//Sideways wind movement.
	//Wind changes gradually.  Occasionally, velocity changes sharply.
	static const float fMaxDrift = 0.5f;
	if (RAND(20000) == 0)
		CRaindropEffect::fXDrift += fRAND_MID(0.15f);
	else
		CRaindropEffect::fXDrift += fRAND_MID(0.001f);
	if (CRaindropEffect::fXDrift < -fMaxDrift)
		CRaindropEffect::fXDrift = -fMaxDrift;
	else if (CRaindropEffect::fXDrift > fMaxDrift)
		CRaindropEffect::fXDrift = fMaxDrift;
	else
		CRaindropEffect::fXDrift *= 0.9999f;
	this->fX += fMultiplier * CRaindropEffect::fXDrift;	//wind blows sideways

	if (OutOfBounds())
		return false;

	//Particle shrinks.
	static const UINT NUM_SPRITES = 1;        //Sprites in animation
	static const UINT SpriteNum[RAIN_TYPES][NUM_SPRITES] = {
		{TI_RAIN1}, {TI_RAIN2}
	};
	static const UINT XSpriteSize[RAIN_TYPES][NUM_SPRITES] = {
		{3}, {2}
	};
	static const UINT YSpriteSize[RAIN_TYPES][NUM_SPRITES] = {
		{14}, {10}
	};
	const UINT wSpriteNo = 0;
	ASSERT(wSpriteNo < NUM_SPRITES);

	//Blit if in bounds.
	const UINT wX = static_cast<UINT>(this->fX), wY = static_cast<UINT>(this->fY);
	const UINT wXSize = XSpriteSize[this->wType][wSpriteNo];
	const UINT wYSize = YSpriteSize[this->wType][wSpriteNo];
	if (wX >= (UINT)this->screenRect.x && wY >= (UINT)this->screenRect.y &&
				wX < this->screenRect.x + this->screenRect.w - wXSize &&
				wY < this->screenRect.y + this->screenRect.h - wYSize)
		g_pTheBM->BlitTileImagePart(SpriteNum[this->wType][wSpriteNo], wX, wY,
				0, 0, wXSize, wYSize, pDestSurface, true, Uint8(255 * this->fOpacity));

	//Update bounding box position.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = static_cast<Sint16>(this->fX);
	this->dirtyRects[0].y = static_cast<Sint16>(this->fY);
	this->dirtyRects[0].w = wXSize;
	this->dirtyRects[0].h = wYSize;

	return true;
}

//*****************************************************************************
inline bool CRaindropEffect::OutOfBounds() const
{
	return this->fY >= this->screenRect.y + this->screenRect.h;
}
