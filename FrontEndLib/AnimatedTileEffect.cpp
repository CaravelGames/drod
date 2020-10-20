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

#include "AnimatedTileEffect.h"
#include "BitmapManager.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CAnimatedTileEffect::CAnimatedTileEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of animated tile.
	const UINT dwDuration,		//(in)	Effect duration.
	const UINT wTileNo,		//(in)   Tile to draw.
	const bool bUseLightLevel, //(in) whether to draw at light level specified in bitmap manager
	const UINT eType) //(in)  Type of effect [default=EFFECTLIB::EGENERIC]
	: CEffect(pSetWidget, dwDuration, eType)
	, wTileNo(wTileNo)
	, bUseLightLevel(bUseLightLevel)
	, nOpacity(255)
{
	ASSERT(pSetWidget->GetType() == WT_Room);

	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->wX = OwnerRect.x + (SetCoord.wX * CBitmapManager::CX_TILE);
	this->wY = OwnerRect.y + (SetCoord.wY * CBitmapManager::CY_TILE);

	//Area of effect.
	SDL_Rect rect = MAKE_SDL_RECT(this->wX, this->wY,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);
}

//********************************************************************************
bool CAnimatedTileEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	return true;
}

//********************************************************************************
void CAnimatedTileEffect::Draw(SDL_Surface& pDestSurface)
{
	DrawTile(this->wTileNo, pDestSurface, this->nOpacity);
}

//********************************************************************************
void CAnimatedTileEffect::DrawTile(
//Blit a tile at my location.
//
//Params:
	const UINT wTileImageNo,   //(in) Tile to draw.
	SDL_Surface& pDestSurface, //(in)
	const Uint8 nOpacity)      //(in) Level of opacity (default = 255)
{
	g_pTheBM->BlitTileImage(wTileImageNo, this->wX, this->wY, &pDestSurface,
			this->bUseLightLevel, nOpacity);
}

//********************************************************************************
void CAnimatedTileEffect::ShadeTile(
//Add shading to my location.
	const SURFACECOLOR &Color, //(in) 
	SDL_Surface& pDestSurface) //(in)
{
	g_pTheBM->ShadeTile(this->wX, this->wY, Color, &pDestSurface);
}
