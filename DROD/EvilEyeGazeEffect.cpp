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

#include "EvilEyeGazeEffect.h"
#include "RoomWidget.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/EvilEye.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CEvilEyeGazeEffect::CEvilEyeGazeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,          //(in)   Should be a room widget.
	const UINT wX, const UINT wY, const UINT wO,    //(in)   Eye's location.
	const Uint32 dwDuration) //effect duration (0 = indefinite)
	: CEffect(pSetWidget, dwDuration, dwDuration ? EGENERIC : EEVILEYEGAZE)
	, wX(wX)
	, wY(wY)
	, dwDuration(dwDuration)
	, opacity(255)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	this->dx = nGetOX(wO);
	this->dy = nGetOY(wO);
	switch (wO)
	{
		case N: case S: this->wTileNo = TI_EYEGAZE_NS; break;
		case E: case W: this->wTileNo = TI_EYEGAZE_EW; break;
		case NW: case SE: this->wTileNo = TI_EYEGAZE_NWSE; break;
		case NE: case SW: this->wTileNo = TI_EYEGAZE_NESW; break;
		default: ASSERT(!"Bad evil eye gaze orientation"); break;
	}
}

//********************************************************************************
bool CEvilEyeGazeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Set transparency level: beam strobes.
	if (this->dwDuration && dwTimeElapsed > this->dwDuration)
		return false;

	UpdateOpacity(dwTimeElapsed);

	//Tiles affected by beam last rendering must be erased.
	DirtyOldBeamTiles();
	DrawNewBeam();

	return true;
}

void CEvilEyeGazeEffect::DirtyOldBeamTiles()
{
	static SDL_Rect Dest = MAKE_SDL_RECT(0, 0, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE);
	this->dirtyRects.clear();
	for (CCoordSet::const_iterator coord = this->gazeTiles.begin();
		coord != this->gazeTiles.end(); ++coord)
	{
		Dest.x = this->pRoomWidget->GetX() + coord->wX * CBitmapManager::CX_TILE;
		Dest.y = this->pRoomWidget->GetY() + coord->wY * CBitmapManager::CY_TILE;
		this->dirtyRects.push_back(Dest);
	}
}

//********************************************************************************
void CEvilEyeGazeEffect::UpdateOpacity(const Uint32& dwTimeElapsed)
{
	this->opacity = 255;
	if (g_pTheBM->bAlpha)
	{
		if (this->dwDuration)
		{
			const float fMultiplier = 255.0f / (float)this->dwDuration;
			this->opacity = (Uint8)((this->dwDuration - dwTimeElapsed) * fMultiplier);
		}
		else
			this->opacity = 128;
	}
}

//********************************************************************************
void CEvilEyeGazeEffect::DrawNewBeam()
{
	static SDL_Rect Dest = MAKE_SDL_RECT(0, 0, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE);
	UINT cx = this->wX, cy = this->wY;
	int nDx = this->dx, nDy = this->dy;
	bool bReflected = false;
	CCoordSet newGazeTiles;
	while (CEvilEye::GetNextGaze(this->pRoomWidget->GetRoom(), cx, cy, nDx, nDy, bReflected))
	{
		const int destX = this->pRoomWidget->GetX() + cx * CBitmapManager::CX_TILE;
		const int destY = this->pRoomWidget->GetY() + cy * CBitmapManager::CY_TILE;
		if (!this->gazeTiles.has(cx, cy))
		{
			Dest.x = destX;
			Dest.y = destY;
			this->dirtyRects.push_back(Dest);
		}
		newGazeTiles.insert(cx, cy);
	}

	this->gazeTiles = newGazeTiles;
}

//********************************************************************************
void CEvilEyeGazeEffect::Draw(SDL_Surface& pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	//Draw beam.
	for (CCoordSet::const_iterator coord = this->gazeTiles.begin();
		coord != this->gazeTiles.end(); ++coord)
	{
		const int destX = this->pRoomWidget->GetX() + coord->wX * CBitmapManager::CX_TILE;
		const int destY = this->pRoomWidget->GetY() + coord->wY * CBitmapManager::CY_TILE;
		g_pTheBM->BlitTileImage(this->wTileNo, destX, destY, &pDestSurface, false, this->opacity);
	}
}
