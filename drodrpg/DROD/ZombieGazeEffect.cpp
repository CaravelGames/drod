// $Id: ZombieGazeEffect.cpp 8676 2008-02-23 18:36:16Z mrimer $

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

#include "ZombieGazeEffect.h"
#include "RoomWidget.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/Zombie.h"
#include <BackEndLib/Assert.h>

Uint32 CZombieGazeEffect::dwLastDraw = 0;


//********************************************************************************
CZombieGazeEffect::CZombieGazeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const CMonster *pZombie)      //(in)   Zombie emitting gaze.
	: CEffect(pSetWidget, EGAZE)
	, pZombie(pZombie)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	ASSERT(!pRoom || pRoom->GetCurrentGame());
	this->wValidTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;

	ASSERT(pZombie);
	this->origin.wX = pZombie->wX;
	this->origin.wY = pZombie->wY;
	this->origin.wO = pZombie->wO;

	//Prepare beam effect.
	PrepareBeam();
}

//********************************************************************************
bool CZombieGazeEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	if (wTurnNow != this->wValidTurn)
		return false;
	if (!this->pZombie->bAlive)
		return false;

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Set transparency level: beam strobes.
	static Uint8 brightness = 255;
	if (g_pTheBM->bAlpha)
	{
		static bool bMakeBrighter = false;
		const Uint32 dwNow = SDL_GetTicks();
		UINT dwTimeElapsed = dwNow - CZombieGazeEffect::dwLastDraw;
		dwTimeElapsed /= 2; //half-speed
		CZombieGazeEffect::dwLastDraw = dwNow;
		if (dwTimeElapsed > 255)
			dwTimeElapsed = 255;
		const Uint8 timeElapsed = static_cast<Uint8>(dwTimeElapsed);
		if (bMakeBrighter)
		{
			if (brightness >= 255 - timeElapsed)
			{
				brightness = 255;
				bMakeBrighter = false;
			} else
				brightness += timeElapsed;
		} else {
			if (brightness <= timeElapsed)
			{
				brightness = 0;
				bMakeBrighter = true;
				return true;  //nothing to draw this frame
			}
			else
				brightness -= timeElapsed;
		}
	}

	//Clip screen surface to widget because beams will go all over the place.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(pDestSurface, &OwnerRect);

	//Draw beam.
	for (UINT wIndex=this->coords.size(); wIndex--; )
	{
		CMoveCoord& coord = this->coords[wIndex];
		g_pTheBM->BlitTileImage(coord.wO, coord.wX, coord.wY, pDestSurface, false, brightness);
	}

	//Unclip screen surface.
	SDL_SetClipRect(pDestSurface, NULL);

	//Continue effect.
	return true;
}

//*****************************************************************************
void CZombieGazeEffect::PrepareBeam()
{
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);

	//Follow direction of monster's gaze.
	int oX = nGetOX(this->origin.wO);
	int oY = nGetOY(this->origin.wO);

	//Standing on a force arrow opposing this direction will halt the gaze.
	UINT wTileNo = pRoom->GetFSquare(this->origin.wX, this->origin.wY);
	if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo, this->origin.wO))
	{
		this->endCoord.wX = this->origin.wX;
		this->endCoord.wY = this->origin.wY;
		this->endCoord.wO = NO_ORIENTATION; //don't show effect
	} else {
		//Begin gaze at next tile.
		const bool bElevatedSource = bIsElevatedTile(
				pRoom->GetOSquare(this->origin.wX, this->origin.wY));
		UINT wX = this->origin.wX + oX;   //start in front of zombie
		UINT wY = this->origin.wY + oY;

		CCueEvents Ignored;
		int destX = this->pRoomWidget->GetX() + wX * CBitmapManager::CX_TILE;
		int destY = this->pRoomWidget->GetY() + wY * CBitmapManager::CY_TILE;
		while (CMonster::GetNextGaze(Ignored, NULL, pRoom, bElevatedSource, wX, wY, oX, oY))
		{
			//Draw beam.
			UINT wTileNo = 0;
			switch (this->origin.wO)
			{
				case N: case S: wTileNo = TI_ZGAZE_NS; break;
				case E: case W: wTileNo = TI_ZGAZE_EW;	break;
				case NW: case SE: wTileNo = TI_ZGAZE_NWSE; break;
				case NE: case SW: wTileNo = TI_ZGAZE_NESW; break;
				default:	ASSERT(!"Bad gaze orientation"); break;
			}
			this->coords.push_back(CMoveCoord(destX, destY, wTileNo));
			SDL_Rect Dest = MAKE_SDL_RECT(destX, destY, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE);
			this->dirtyRects.push_back(Dest);

			destX = this->pRoomWidget->GetX() + wX * CBitmapManager::CX_TILE;
			destY = this->pRoomWidget->GetY() + wY * CBitmapManager::CY_TILE;
		}

		//Provide coordinates for contact effect.
		this->endCoord.wX = wX - oX; //just before obstructing tile
		this->endCoord.wY = wY - oY;
		this->endCoord.wO = nGetO(-nGetOX(this->origin.wO),-nGetOY(this->origin.wO));

		if (!pRoom->IsValidColRow(wX, wY))
			this->endCoord.wO = NO_ORIENTATION; //don't show effect if gaze exits room
	}
}
