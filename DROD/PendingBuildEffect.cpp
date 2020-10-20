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

#include "PendingBuildEffect.h"
#include "RoomWidget.h"
#include <BackEndLib/Assert.h>

const BYTE OPACITY_INCREMENT = 2;
const BYTE MIN_OPACITY = OPACITY_INCREMENT;
const BYTE MAX_OPACITY = 128;

unsigned char CPendingBuildEffect::nOpacity = MIN_OPACITY;
bool CPendingBuildEffect::bRising = true;

//*****************************************************************************
CPendingBuildEffect::CPendingBuildEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,        //(in)   Should be a room widget.
	const UINT wTileImageNo,    //(in)   Tile to display.
	const UINT wX, const UINT wY,
	const bool bModOpacity)     //(in) whether this instance changes the static opacity
	: CEffect(pSetWidget, (UINT)-1, EPENDINGBUILD)
	, wX(wX), wY(wY)
	, wTileImageNo(wTileImageNo)
	, bModOpacity(bModOpacity)
{
	ASSERT(pSetWidget->GetType() == WT_Room);

	this->pOwnerWidget->GetRect(this->OwnerRect);
	this->pRoomWidget = DYN_CAST(CRoomWidget *, CWidget *, this->pOwnerWidget);
	ASSERT(this->pRoomWidget);

	SDL_Rect dest = MAKE_SDL_RECT(this->OwnerRect.x + wX * CBitmapManager::CX_TILE,
			this->OwnerRect.y + wY * CBitmapManager::CY_TILE,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(dest);
}

//*****************************************************************************
bool CPendingBuildEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (this->bModOpacity)
	{
		//Change level of transparency.
		if (this->bRising)
		{
			this->nOpacity += OPACITY_INCREMENT;
			if (this->nOpacity > MAX_OPACITY)
				this->bRising = false;
		}
		else {
			this->nOpacity -= OPACITY_INCREMENT;
			if (this->nOpacity < MIN_OPACITY)
				this->bRising = true;
		}
	}

	return true;
}
//*****************************************************************************
void CPendingBuildEffect::Draw(SDL_Surface& pDestSurface)
{
	ASSERT(this->dirtyRects.size() == 1);
	g_pTheBM->BlitTileImage(this->wTileImageNo,
			this->dirtyRects[0].x, this->dirtyRects[0].y,
			&pDestSurface, false, this->nOpacity);
}
