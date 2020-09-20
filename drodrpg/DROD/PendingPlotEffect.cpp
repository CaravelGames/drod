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
	: CEffect(pSetWidget, EPENDINGPLOT)
	, pwTileImageNo(pwTileImageNo)
	, wObjectNo(wObjectNo)
	, wXSize(wXSize), wYSize(wYSize)
	, wO(wO)
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
bool CPendingPlotEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!pDestSurface) pDestSurface = GetDestSurface();
	this->dirtyRects.clear();

	//Change level of transparency.
	if (this->bRising)
	{
		this->nOpacity += 4;
		if (this->nOpacity > MAX_OPACITY)
			this->bRising = false;
	} else {
		this->nOpacity -= 4;
		if (this->nOpacity < MIN_OPACITY)
			this->bRising = true;
	}

	//Draw transparent tiles in region specified in the room widget.
	UINT wStartX = this->pRoomWidget->wMidX;
	UINT wStartY = this->pRoomWidget->wMidY;
	UINT wEndX = this->pRoomWidget->wEndX;
	UINT wEndY = this->pRoomWidget->wEndY;
	//Show at least one full object of specified type.
	if (wEndX - pRoomWidget->wMidX + 1 < this->wXSize)
		wEndX = pRoomWidget->wMidX + this->wXSize - 1;
	if (wEndY - pRoomWidget->wMidY + 1 < this->wYSize)
		wEndY = pRoomWidget->wMidY + this->wYSize - 1;

	//Stairs and tunnels have a special appearance.
	switch (this->wObjectNo)
	{
		case T_STAIRS:
		case T_STAIRS_UP:
			PlotStaircase(wStartX,wStartY, wEndX,wEndY, this->wObjectNo, pDestSurface);
		break;
		case T_SWORDSMAN:
		{
			//Objects w/ swords must calc the coords specially.
			wStartX = this->pRoomWidget->wEndX;
			wStartY = this->pRoomWidget->wEndY;
			wEndX = wStartX + nGetOX(this->wO);
			wEndY = wStartY + nGetOY(this->wO);
			PlotSwordsman(wStartX,wStartY, wEndX,wEndY, pDestSurface);
		}
		break;
		case T_ROCKGIANT: //always show only whole large monsters
			if (((wEndX - wStartX + 1) % 2) != 0)
				++wEndX;
			if (((wEndY - wStartY + 1) % 2) != 0)
				++wEndY;
		//NO BREAK
		default:
		{
			UINT wX, wY, wTileNo;
			for (wY=wStartY; wY<=wEndY; ++wY)
				for (wX=wStartX; wX<=wEndX; ++wX)
				{
					wTileNo = this->pwTileImageNo[
							((wY - wStartY) % this->wYSize) * this->wXSize +
							(wX - wStartX) % this->wXSize];
					PlotTile(wX, wY, this->wObjectNo, wTileNo, pDestSurface);
				}
		}
		break;
	}

	//Continue effect.
	return true;
}

//*****************************************************************************
void CPendingPlotEffect::PlotStaircase(
//Staircase is drawn with a wall around the left, bottom and right sides.
//(And nothing on top.)
//
//Params:
	const UINT wStartX, const UINT wStartY, const UINT wEndX, const UINT wEndY,
	const UINT wStairType,     //(in)
	SDL_Surface* pDestSurface) //(in)
{
	ASSERT(bIsStairs(wStairType));
	const UINT wBaseY = wStairType == T_STAIRS ? wEndY : wStartY;
	const UINT wEntranceY = wStairType == T_STAIRS ? wStartY : wEndY;
	UINT wX, wY, wObjectNo, wTileNo;
	for (wY=wStartY; wY<=wEndY; ++wY)
		for (wX=wStartX; wX<=wEndX; ++wX)
		{
			//Determine what part of staircase is being plotted at this square.
			if (wX == wStartX || wX == wEndX || wY == wBaseY)
				wObjectNo = T_WALL;
			else if (wY == wEntranceY)
				continue;   //stair entrance -- nothing will be plotted here
			else
				wObjectNo = wStairType;

			wTileNo = (bIsWall(wObjectNo) ? TI_WALL : TI_STAIRS);
			PlotTile(wX, wY, wObjectNo, wTileNo, pDestSurface);
		}
}

//*****************************************************************************
void CPendingPlotEffect::PlotSwordsman(
//Draw swordsman and sword.
//
//Params:
	const UINT wStartX, const UINT wStartY, const UINT /*wEndX*/, const UINT /*wEndY*/,
	SDL_Surface* pDestSurface)
{
	//Unarmed default player type (stalwart).
	static const UINT SMAN_TI[9] = {
		TI_STALWART_UNW, TI_STALWART_UN, TI_STALWART_UNE,
		TI_STALWART_UW,  TI_TEMPTY,      TI_STALWART_UE,
		TI_STALWART_USW, TI_STALWART_US, TI_STALWART_USE};
	PlotTile(wStartX, wStartY, T_SWORDSMAN, SMAN_TI[this->wO], pDestSurface);
/*
	static const UINT SWORD_TI[9] = {
		TI_SWORD_YNW, TI_SWORD_YN, TI_SWORD_YNE,
		TI_SWORD_YW, TI_TEMPTY, TI_SWORD_YE,
		TI_SWORD_YSW, TI_SWORD_YS, TI_SWORD_YSE};
	PlotTile(wEndX, wEndY, T_EMPTY, SWORD_TI[this->wO], pDestSurface);
*/
}

//*****************************************************************************
void CPendingPlotEffect::PlotTile(
	const UINT wX, const UINT wY, const UINT wObjectNo, const UINT wTileNo,
	SDL_Surface* pDestSurface) //(in)
{
	ASSERT(pDestSurface);

	SDL_Rect dest = MAKE_SDL_RECT(this->OwnerRect.x + wX * CBitmapManager::CX_TILE,
			this->OwnerRect.y + wY * CBitmapManager::CY_TILE,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	if (pRoomWidget->IsSafePlacement(wObjectNo,wX,wY,NO_ORIENTATION,wObjectNo==T_SWORDSMAN))
	{
		g_pTheBM->BlitTileImage(wTileNo, dest.x, dest.y, pDestSurface, false, this->nOpacity);
		this->dirtyRects.push_back(dest);
	} else if (wX * CBitmapManager::CX_TILE < this->OwnerRect.w && wY *
				CBitmapManager::CY_TILE < this->OwnerRect.h)
	{
		g_pTheBM->ShadeTile(dest.x, dest.y, Red, pDestSurface);
		this->dirtyRects.push_back(dest);
	}
}
