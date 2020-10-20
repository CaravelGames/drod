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

#include "GridEffect.h"
#include "DrodBitmapManager.h"
#include "RoomWidget.h"
#include "TileImageConstants.h"

#include "../DRODLib/DbRooms.h"

#include <BackEndLib/Assert.h>

//********************************************************************************
CGridEffect::CGridEffect(
//Params:
	CWidget *pSetWidget)    //(in)   Should be a room widget.
	: CEffect(pSetWidget, (UINT)-1, EGRID)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	//Effect covers entire room widget.
	SDL_Rect OwnerRect;
	pSetWidget->GetRect(OwnerRect);
	this->dirtyRects.push_back(OwnerRect);
}

//********************************************************************************
bool CGridEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
//Returns: true if effect should continue, or false if effect is done.
{
	ASSERT(this->dirtyRects.size() == 1);
	
	const CDbRoom* pRoom = this->pRoomWidget->GetRoom();
	if (!pRoom)
		this->dirtyRects[0].w = 0;

	else {
		SDL_Rect OwnerRect;
		this->pOwnerWidget->GetRect(OwnerRect);

		this->dirtyRects[0].w = OwnerRect.w;
	}

	return true;
}
//********************************************************************************
void CGridEffect::Draw(SDL_Surface& pDestSurface)
//Returns: true if effect should continue, or false if effect is done.
{
	static const int gridThickness = 1;

	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);

	const CDbRoom* pRoom = this->pRoomWidget->GetRoom();
	const CDbRoom &room = *pRoom;
	UINT yPixel = OwnerRect.y;
	for (UINT wY=0; wY<room.wRoomRows; ++wY, yPixel += CBitmapManager::CY_TILE) {
		UINT xPixel = OwnerRect.x;
		for (UINT wX=0; wX<room.wRoomCols; ++wX, xPixel += CBitmapManager::CX_TILE) {
			//Single pixel width edges
			g_pTheBM->BlitTileImagePart(TI_GRID_OVERLAY, xPixel, yPixel, 0, 0, CBitmapManager::CX_TILE, gridThickness, &pDestSurface, false, 128);
			g_pTheBM->BlitTileImagePart(TI_GRID_OVERLAY, xPixel, yPixel, 0, gridThickness, gridThickness, CBitmapManager::CY_TILE-gridThickness, &pDestSurface, false, 128);
		}
	}
}
