// $Id: PendingPlotEffect.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "PendingPlotEffect.h"
#include "EditRoomWidget.h"
#include <BackEndLib/Assert.h>

const BYTE MIN_OPACITY = 48;
const BYTE MAX_OPACITY = 208;

unsigned char CPendingPlotEffect::nOpacity = MIN_OPACITY;
bool CPendingPlotEffect::bRising = true;

const SURFACECOLOR Red = {255, 0, 0};

//*****************************************************************************
CPendingPlotEffect::CPendingPlotEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const UINT wObjectNo,         //(in)   Object to place.
	const UINT* pwTileImageNo,    //(in)   Tile(s) to display.
	const UINT wXSize, const UINT wYSize,  //(in) Dimensions of object being displayed
											//[default = 1x1]
	const UINT wO)                //(in)   Orientation of object [default = NO_OREINTATION]
	: CEffect(pSetWidget, (UINT)-1, EPENDINGPLOT)
	, pwTileImageNo(pwTileImageNo)
	, wObjectNo(wObjectNo)
	, wXSize(wXSize), wYSize(wYSize)
	, wO(wO)
	, wDrawStartX(0), wDrawStartY(0)
	, wDrawEndX(0), wDrawEndY(0)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	ASSERT(this->wXSize && this->wYSize);
#ifdef _DEBUG
	for (UINT wIndex=wXSize*wYSize; wIndex--; )
		ASSERT(pwTileImageNo[wIndex] < TI_COUNT); //valid tiles
#endif

	this->pOwnerWidget->GetRect(this->OwnerRect);
	this->pRoomWidget = DYN_CAST(CEditRoomWidget *, CWidget *, this->pOwnerWidget);
	ASSERT(this->pRoomWidget);
}

//*****************************************************************************
bool CPendingPlotEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	this->dirtyRects.clear();

	//Change level of transparency.
	if (this->bRising)
	{
		this->nOpacity += 4;
		if (this->nOpacity > MAX_OPACITY)
			this->bRising = false;
	}
	else {
		this->nOpacity -= 4;
		if (this->nOpacity < MIN_OPACITY)
			this->bRising = true;
	}

	this->wDrawStartX = this->pRoomWidget->wMidX;
	this->wDrawStartY = this->pRoomWidget->wMidY;
	this->wDrawEndX = this->pRoomWidget->wEndX;
	this->wDrawEndY = this->pRoomWidget->wEndY;

	//Show at least one full object of specified type.
	if (this->wDrawEndX - pRoomWidget->wMidX + 1 < this->wXSize)
		this->wDrawEndX = pRoomWidget->wMidX + this->wXSize - 1;
	if (this->wDrawEndY - pRoomWidget->wMidY + 1 < this->wYSize)
		this->wDrawEndY = pRoomWidget->wMidY + this->wYSize - 1;

	UINT wDrawWidth = this->wDrawEndX - this->wDrawStartX;
	UINT wDrawHeight = this->wDrawEndY - this->wDrawStartY;


	switch (this->wObjectNo)
	{
	case T_SWORDSMAN:
		//Room entrance has special calculation
		this->wDrawStartX = this->pRoomWidget->wEndX;
		this->wDrawStartY = this->pRoomWidget->wEndY;
		this->wDrawEndX = this->wDrawStartX;
		this->wDrawEndY = this->wDrawStartY;

		wDrawWidth = this->wDrawEndX - this->wDrawStartX;
		wDrawHeight = this->wDrawEndY - this->wDrawStartY;
		break;
	case T_ROCKGIANT: //always show only whole large monsters
		if (wDrawWidth % 2 != 1) {
			++this->wDrawEndX;
			++wDrawWidth;
		}
		if (wDrawHeight % 2 != 1) {
			++this->wDrawEndY;
			++wDrawHeight;
		}
		break;
	}

	this->dirtyRects.push_back(MAKE_SDL_RECT(
		this->OwnerRect.x + this->wDrawStartX * CBitmapManager::CX_TILE,
		this->OwnerRect.y + this->wDrawStartY * CBitmapManager::CY_TILE,
		(wDrawWidth + 1) * CBitmapManager::CX_TILE,
		(wDrawHeight + 1) * CBitmapManager::CY_TILE
	));

	return true;
}

//*****************************************************************************
void CPendingPlotEffect::Draw(SDL_Surface& destSurface)
{
	switch (this->wObjectNo)
	{
	case T_SWORDSMAN:
		PlotSwordsman(this->wDrawStartX, this->wDrawStartY, destSurface);
		break;
	default:
	{
		UINT wX, wY, wTileNo;
		for (wY = this->wDrawStartY; wY <= this->wDrawEndY; ++wY)
			for (wX = this->wDrawStartX; wX <= this->wDrawEndX; ++wX)
			{
				wTileNo = this->pwTileImageNo[
					((wY - this->wDrawStartY) % this->wYSize) * this->wXSize +
						(wX - this->wDrawStartX) % this->wXSize];
				PlotTile(wX, wY, this->wObjectNo, wTileNo, destSurface);
			}
	}
	break;
	}
}

//*****************************************************************************
void CPendingPlotEffect::PlotSwordsman(
//Draw swordsman
//
//Params:
	const UINT wSwordsmanX, const UINT wSwordsmanY, SDL_Surface& destSurface)
{
	//Unarmed default player type (stalwart).
	static const UINT SMAN_TI[9] = {
		TI_STALWART_UNW, TI_STALWART_UN, TI_STALWART_UNE,
		TI_STALWART_UW,  TI_TEMPTY,      TI_STALWART_UE,
		TI_STALWART_USW, TI_STALWART_US, TI_STALWART_USE};
	PlotTile(wSwordsmanX, wSwordsmanY, T_SWORDSMAN, SMAN_TI[this->wO], destSurface);
}

//*****************************************************************************
void CPendingPlotEffect::PlotTile(
	const UINT wX, const UINT wY, const UINT wObjectNo, const UINT wTileNo,
	SDL_Surface& destSurface) //(in)
{
	SDL_Rect dest = MAKE_SDL_RECT(this->OwnerRect.x + wX * CBitmapManager::CX_TILE,
			this->OwnerRect.y + wY * CBitmapManager::CY_TILE,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	if (pRoomWidget->IsSafePlacement(wObjectNo,wX,wY,NO_ORIENTATION,wObjectNo==T_SWORDSMAN))
	{
		g_pTheBM->BlitTileImage(wTileNo, dest.x, dest.y, &destSurface, false, this->nOpacity);
		this->dirtyRects.push_back(dest);
	} else if (wX * CBitmapManager::CX_TILE < (UINT)this->OwnerRect.w && wY *
				CBitmapManager::CY_TILE < (UINT)this->OwnerRect.h)
	{
		g_pTheBM->ShadeTile(dest.x, dest.y, Red, &destSurface);
		this->dirtyRects.push_back(dest);
	}
}
