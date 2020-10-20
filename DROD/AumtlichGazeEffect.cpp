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
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "AumtlichGazeEffect.h"
#include "RoomWidget.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/Aumtlich.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>
#include <FrontEndLib/Screen.h>

Uint8 CAumtlichGazeEffect::brightness = 255;

//********************************************************************************
CAumtlichGazeEffect::CAumtlichGazeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const CMonster *pAumtlich)      //(in)   Aumtlich emitting gaze.
	: CEffect(pSetWidget, (UINT) -1, EGAZE)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	ASSERT(!pRoom || pRoom->GetCurrentGame());
	this->wValidTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo : 0;

	ASSERT(pAumtlich);
	this->origin.wX = pAumtlich->wX;
	this->origin.wY = pAumtlich->wY;
	this->origin.wO = pAumtlich->wO;

	//Prepare beam effect.
	PrepareBeam(pAumtlich);
}

//********************************************************************************
void CAumtlichGazeEffect::SharedStateUpdate(const UINT wDeltaTime)
//Updates the state for this effect shared by all instances
{	
	static bool bMakeBrighter = false;
	static Uint32 lastUpdatePresentCount = 0;

	// Already updated this frame
	if (CScreen::dwPresentsCount == lastUpdatePresentCount)
		return;

	UINT dwTimeElapsed = wDeltaTime / 2; // Run this effect at half speed

	if (dwTimeElapsed > 255)
		dwTimeElapsed = 255;

	const Uint8 timeElapsed = static_cast<Uint8>(dwTimeElapsed);
	if (bMakeBrighter)
	{
		if (CAumtlichGazeEffect::brightness >= 255 - timeElapsed)
		{
			CAumtlichGazeEffect::brightness = 255;
			bMakeBrighter = false;
		}
		else
			CAumtlichGazeEffect::brightness += timeElapsed;
	}
	else {
		if (CAumtlichGazeEffect::brightness <= timeElapsed)
		{
			CAumtlichGazeEffect::brightness = 0;
			bMakeBrighter = true;
		}
		else
			CAumtlichGazeEffect::brightness -= timeElapsed;
	}
}

//********************************************************************************
bool CAumtlichGazeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
//Updates the effect state & dirty rects
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	if (wTurnNow != this->wValidTurn) 
		return false;

	CAumtlichGazeEffect::SharedStateUpdate(wDeltaTime);

	return true;
}

//********************************************************************************
void CAumtlichGazeEffect::Draw(SDL_Surface& pDestSurface)
//Draw the effect.
{
	if (CAumtlichGazeEffect::brightness == 0)
		return; // Nothing to draw this frame

	//Clip screen surface to widget because beams will go all over the place.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(&pDestSurface, &OwnerRect);

	//Draw beam.
	for (UINT wIndex=this->coords.size(); wIndex--; )
	{
		CMoveCoord& coord = this->coords[wIndex];
		g_pTheBM->BlitTileImage(coord.wO, coord.wX, coord.wY, &pDestSurface, false, CAumtlichGazeEffect::brightness);
	}

	//Unclip screen surface.
	SDL_SetClipRect(&pDestSurface, NULL);
}

//*****************************************************************************
void CAumtlichGazeEffect::PrepareBeam(const CMonster *pMonster)
{
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);

	CCoordIndex SwordCoords;
	pRoom->GetSwordCoords(SwordCoords);
	const CAumtlich *pAumtlich = DYN_CAST(const CAumtlich*, const CMonster*, pMonster);

	//Follow direction of aumtlich's gaze.
	int oX = nGetOX(this->origin.wO);
	int oY = nGetOY(this->origin.wO);
	UINT wX = this->origin.wX + oX;   //start in front of aumtlich
	UINT wY = this->origin.wY + oY;

	if (!pRoom->DoesGentryiiPreventDiagonal(this->origin.wX, this->origin.wY, wX, wY))
	{
		CCueEvents Ignored;
		int destX = this->pRoomWidget->GetX() + wX * CBitmapManager::CX_TILE;
		int destY = this->pRoomWidget->GetY() + wY * CBitmapManager::CY_TILE;

		UINT wTX = pAumtlich->wTX;
		UINT wTY = pAumtlich->wTY;
		while (CAumtlich::GetNextGaze(Ignored, NULL, pRoom, SwordCoords, wX, wY, oX, oY, wTX, wTY))
		{
			//Draw beam.
			UINT wTileNo = 0;
			switch (this->origin.wO)
			{
				case N: case S: wTileNo = TI_ZGAZE_NS; break;
				case E: case W: wTileNo = TI_ZGAZE_EW;	break;
				case NW: case SE: wTileNo = TI_ZGAZE_NWSE; break;
				case NE: case SW: wTileNo = TI_ZGAZE_NESW; break;
				default: ASSERT(!"Bad Aumtlich gaze orientation"); break;
			}
			this->coords.push_back(CMoveCoord(destX, destY, wTileNo));
			SDL_Rect Dest = MAKE_SDL_RECT(destX, destY, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE);
			this->dirtyRects.push_back(Dest);

			if (pRoom->DoesGentryiiPreventDiagonal(wX - oX, wY - oY, wX, wY))
				break;

			destX = this->pRoomWidget->GetX() + wX * CBitmapManager::CX_TILE;
			destY = this->pRoomWidget->GetY() + wY * CBitmapManager::CY_TILE;
		}
	}

	//Provide coordinates for contact effect.
	this->endCoord.wX = wX - oX; //just before obstructing tile
	this->endCoord.wY = wY - oY;
	this->endCoord.wO = nGetO(-nGetOX(this->origin.wO),-nGetOY(this->origin.wO));
	if (!pRoom->IsValidColRow(wX, wY))
		this->endCoord.wO = NO_ORIENTATION; //don't show effect if gaze exits room
}
