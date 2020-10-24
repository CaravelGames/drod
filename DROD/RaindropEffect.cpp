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
#include <FrontEndLib/Screen.h>

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
	: CEffect(pSetWidget, (UINT)-1, ERAINDROP)
	, bHasted(bHasted)
	, pRoomWidget(NULL)
	, wDrawXSize(0)
	, wDrawYSize(0)
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

	this->wType = RAND(RAIN_TYPES);

	CalculateFrameProperties();

	//There is always one dirty rect.
	SDL_Rect rect = {
		static_cast<Sint16>(this->fX), 
		static_cast<Sint16>(this->fY), 
		int(this->wDrawXSize),
		int(this->wDrawYSize)
	};
	this->dirtyRects.push_back(rect);

	RequestRetainOnClear(); //this effect doesn't depend on room state
}


//*****************************************************************************
bool CRaindropEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (TryToCreateSplash())
		return false;

	UpdateWind();

	const float fMultiplier = float((this->bHasted ? 0.5f : 1.0f) * wDeltaTime);
	this->fX += fMultiplier * CRaindropEffect::fXDrift;	//wind blows sideways
	this->fY += fMultiplier;

	this->dirtyRects[0].x = static_cast<Sint16>(this->fX);
	this->dirtyRects[0].y = static_cast<Sint16>(this->fY);

	if (OutOfBounds())
		return false;

	return true;
}

//*****************************************************************************
bool CRaindropEffect::TryToCreateSplash()
{
	if (this->fY >= this->fGoalY) {
		//Create a splash where raindrop hits water.
		const CDbRoom* pRoom = this->pRoomWidget ? this->pRoomWidget->GetCurrentGame()->pRoom : NULL;
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

		return true;
	}
	
	return false;
}

//*****************************************************************************
void CRaindropEffect::UpdateWind() 
{
	static Uint32 lastUpdatePresentCount = 0;

	// Already updated this frame
	if (CScreen::dwPresentsCount == lastUpdatePresentCount)
		return;

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
}

void CRaindropEffect::CalculateFrameProperties()
{
	//Particle shrinks.
	static const UINT SpriteNum[RAIN_TYPES] = { TI_RAIN1, TI_RAIN2 };
	static const UINT XSpriteSize[RAIN_TYPES] = { 3, 2 };
	static const UINT YSpriteSize[RAIN_TYPES] = { 14, 10 };

	this->wDrawXSize = XSpriteSize[this->wType];
	this->wDrawYSize = YSpriteSize[this->wType];
}

//*****************************************************************************
void CRaindropEffect::Draw(SDL_Surface& pDestSurface)
{
	//Particle shrinks.
	static const UINT SpriteNum[RAIN_TYPES] = {TI_RAIN1, TI_RAIN2};
	
	const UINT wX = static_cast<UINT>(this->fX);
	const UINT wY = static_cast<UINT>(this->fY);
	const UINT wXSize = this->wDrawXSize;
	const UINT wYSize = this->wDrawYSize;

	if (wX >= (UINT)this->screenRect.x && wY >= (UINT)this->screenRect.y &&
			wX < this->screenRect.x + this->screenRect.w - wXSize &&
			wY < this->screenRect.y + this->screenRect.h - wYSize)
		g_pTheBM->BlitTileImagePart(SpriteNum[this->wType], wX, wY,
				0, 0, wXSize, wYSize, &pDestSurface, true, Uint8(255 * this->fOpacity));

}

//*****************************************************************************
inline bool CRaindropEffect::OutOfBounds() const
{
	return this->fY >= this->screenRect.y + this->screenRect.h;
}
