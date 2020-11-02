// $Id: SwordsmanSwirlEffect.cpp 8975 2008-05-01 22:47:03Z mrimer $

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

#include "SwordsmanSwirlEffect.h"
#include "DrodBitmapManager.h"
#include "TileImageConstants.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/Swordsman.h"
#include <BackEndLib/Assert.h>

#include <math.h>

#define NUM_SWIRL_OBJECTS     (10)        //Sprites in animation
#define SPRITE_SIZE           (6)         //largest image, in pixels
#define MAX_RADIUS            (4.0)       //radius of swirl at initial position
#define NUM_REVOLUTIONS       (2)         //# revolutions around player
#define MS_PER_REVOLUTION     (1000)      //Duration of one revolution (now 1 sec)
#define MS_PER_OBJECT         (60)        //delay before showing each next object in swirl

const double twoPi = 3.1415926536 * 2.0;

const UINT wSwirlImageNo[NUM_SWIRL_OBJECTS] = {
		TI_SWIRLDOT_1,
		TI_SWIRLDOT_1,
		TI_SWIRLDOT_2,
		TI_SWIRLDOT_2,
		TI_SWIRLDOT_3,
		TI_SWIRLDOT_3,
		TI_SWIRLDOT_4,
		TI_SWIRLDOT_4,
		TI_SWIRLDOT_5,
		TI_SWIRLDOT_5};

#define NOT_DEFINED UINT(-1)
const Uint32 EffectDuration = NUM_REVOLUTIONS * MS_PER_REVOLUTION + NUM_SWIRL_OBJECTS * MS_PER_OBJECT;


//********************************************************************************
CSwordsmanSwirlEffect::CSwordsmanSwirlEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,                //(in) Should be a room widget.
	const CCurrentGame *pCurrentGame)   //(in) to track the swordsman's position
	: CEffect(pSetWidget, EffectDuration, ESWIRL)
	, pCurrentGame(pCurrentGame)
	, wOldX(NOT_DEFINED), wOldY(NOT_DEFINED)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	ASSERT(pCurrentGame);
	SDL_Rect rect = {0,0,0,0};
	this->dirtyRects.push_back(rect);   
}

//********************************************************************************
CSwordsmanSwirlEffect::CSwordsmanSwirlEffect(
//Constructor.
//
//Params:
	CWidget* pSetWidget, //(in) Should be a room widget.
	CCoord coord)        //(in) static position
	: CEffect(pSetWidget, EffectDuration, ESWIRL)
	, pCurrentGame(NULL)
	, wOldX(NOT_DEFINED), wOldY(NOT_DEFINED)
	, staticCenter(coord)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	SDL_Rect rect = {0,0,0,0};
	this->dirtyRects.push_back(rect);
}

//********************************************************************************
bool CSwordsmanSwirlEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Swirl gradually fades out.
	this->nOpacity = g_pTheBM->bAlpha ? 255 - ((dwTimeElapsed * 255) / dwDuration) : 255;

	this->drawSwirls.clear();

	//Center effect on player's position.
	UINT wSX, wSY;
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	if (this->pCurrentGame)
	{
		if (!this->pCurrentGame->pPlayer->IsInRoom())
			return false; //Player is not in the room.  Don't show effect.

		
		wSX = this->pCurrentGame->pPlayer->wX;
		wSY = this->pCurrentGame->pPlayer->wY;
	}
	else {
		//Remain at this position.
		wSX = this->staticCenter.wX;
		wSY = this->staticCenter.wY;
	}

	const UINT wXNewCenter = OwnerRect.x + (wSX * CBitmapManager::CX_TILE) +
			CBitmapManager::CX_TILE/4;
	const UINT wYNewCenter = OwnerRect.y + (wSY * CBitmapManager::CY_TILE) +
			CBitmapManager::CY_TILE/4;
	//As swordsman moves, have effect pursue him.
	UINT wXCenter, wYCenter;
	if (wOldX == NOT_DEFINED)
	{
		//Start at this position.
		wXCenter = this->wOldX = wXNewCenter;
		wYCenter = this->wOldY = wYNewCenter;
	} else {
		this->wOldX = wXCenter = (this->wOldX + static_cast<int>(wXNewCenter - this->wOldX) / 4);
		this->wOldY = wYCenter = (this->wOldY + static_cast<int>(wYNewCenter - this->wOldY) / 4);
	}

	//Reset area of effect.
	UINT xMax=0, yMax=0; //bottom edge of bounding box
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = OwnerRect.x + OwnerRect.w;
	this->dirtyRects[0].y = OwnerRect.y + OwnerRect.h;

	//Draw swirl.
	double radius, theta;
	UINT wX, wY, wSize;
	const UINT rMaxPosition=NUM_REVOLUTIONS*MS_PER_REVOLUTION;
	
	const double thetaFactor = twoPi/(MS_PER_REVOLUTION+1.0);
	const double radiusFactor = MAX_RADIUS/(rMaxPosition+1.0);
	for (UINT nIndex=0; nIndex<NUM_SWIRL_OBJECTS; ++nIndex)
	{
		static const UINT SIZES[NUM_SWIRL_OBJECTS] = {18, 18, 16, 16, 12, 12, 8, 8, 6, 6};
		const UINT rPosition = dwTimeElapsed-nIndex*MS_PER_OBJECT;
		if (rPosition > rMaxPosition) continue;   //objects appear gradually (excludes "negative" values)
		wSize = SIZES[nIndex];  //dimensions of this dot
		radius = (rMaxPosition-rPosition)*radiusFactor;
		theta = rPosition * thetaFactor;
		wX = static_cast<UINT>(wXCenter + sin(theta) * CBitmapManager::CX_TILE*radius);
		wY = static_cast<UINT>(wYCenter + cos(theta) * CBitmapManager::CY_TILE*radius);

		if (static_cast<int>(wX) >= OwnerRect.x && static_cast<int>(wY) >= OwnerRect.y &&
				wX < OwnerRect.x + OwnerRect.w - wSize &&
				wY < OwnerRect.y + OwnerRect.h - wSize)
		{
			this->drawSwirls.push_back(CMoveCoordEx(wX, wY, wSize, nIndex));

			//Update bounding box of area of effect.
			if (static_cast<int>(wX) < this->dirtyRects[0].x)
				this->dirtyRects[0].x = wX;
			if (static_cast<int>(wY) < this->dirtyRects[0].y)
				this->dirtyRects[0].y = wY;
			if (wX + wSize > xMax)  //Technically should also have a -1,
				xMax = wX + wSize;   //but this simpler and consistent with
			if (wY + wSize > yMax)  //the calculation below.
				yMax = wY + wSize;
		}
	}
	this->dirtyRects[0].w = xMax - this->dirtyRects[0].x;
	this->dirtyRects[0].h = yMax - this->dirtyRects[0].y;

	return true;
}
//********************************************************************************
void CSwordsmanSwirlEffect::Draw(SDL_Surface& destSurface)
{
	for (UINT i = 0; i < this->drawSwirls.size(); ++i) {
		CMoveCoordEx swirl = this->drawSwirls.at(i);

		g_pTheBM->BlitTileImagePart(wSwirlImageNo[swirl.wValue], swirl.wX, swirl.wY,
			0, 0, swirl.wO, swirl.wO, &destSurface, false, this->nOpacity);
	}
}
