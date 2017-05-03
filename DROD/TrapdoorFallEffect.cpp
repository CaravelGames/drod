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

#include "TrapdoorFallEffect.h"
#include "RoomWidget.h"

//********************************************************************************
CTrapdoorFallEffect::CTrapdoorFallEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in) Should be a room widget.
	const CCoord &SetCoord,    //(in) Location of falling trapdoor.
	const vector<UINT>& tiles, //(in) Tiles to show falling down (layered on top of one another)
	const UINT tileFallTime)  //(in) time to fall down one tile [default=130ms]
	: CEffect(pSetWidget)
	, pSurface(NULL)
	, wCol(SetCoord.wX)
	, wRow(SetCoord.wY)
	, tileFallTime(tileFallTime)
{
	//It will save some time to do this just at construction.
	this->pRoom = DYN_CAST(CRoomWidget*, CWidget*, this->pOwnerWidget)->GetCurrentGame()->pRoom;
	ASSERT(this->pRoom->IsValidColRow(this->wCol, this->wRow));

	//Calc coords of falling animation rect.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->xTrapdoor = OwnerRect.x + (SetCoord.wX * CBitmapManager::CX_TILE);
	this->yTrapdoor = OwnerRect.y + (SetCoord.wY * CBitmapManager::CY_TILE);

	this->pSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE,
			g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));

	//Set surface transparency if needed.
	bool bTransparent = true;
	vector<UINT>::const_iterator tile;
	for (tile = tiles.begin(); tile != tiles.end(); ++tile)
		if (g_pTheBM->GetTileType(*tile) == TIT_Opaque)
		{
			bTransparent = false;
			break;
		}
	if (bTransparent)
	{
		static const Uint32 TranspColor = SDL_MapRGB(this->pSurface->format,
				192, 192, 192);
		SDL_FillRect(this->pSurface, NULL, TranspColor);
		SetColorKey(this->pSurface, SDL_TRUE, TranspColor);
	}

	for (tile = tiles.begin(); tile != tiles.end(); ++tile)
		g_pTheBM->BlitTileImage(*tile, 0, 0, this->pSurface, true);

	//Specify area of effect.
	SDL_Rect rect = MAKE_SDL_RECT(this->xTrapdoor, this->yTrapdoor, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);
}

//********************************************************************************
CTrapdoorFallEffect::~CTrapdoorFallEffect()
{
	if (this->pSurface)
		SDL_FreeSurface(this->pSurface);
}

//********************************************************************************
bool CTrapdoorFallEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	ASSERT(this->dirtyRects.size() == 1);
	if (!this->pSurface) return false;

	if (!pDestSurface) pDestSurface = GetDestSurface();
	const UINT dwTimeElapsed = TimeElapsed();
	const float fyOffset = dwTimeElapsed / float(this->tileFallTime);   //ms per tile falling

	//Determine whether object has fallen to bottom of pit.
	const UINT wPitHeight = (UINT)g_pTheDBM->pTextures[PITSIDE_MOSAIC]->h; //in pixels
	const int yOffset = static_cast<int>(fyOffset * CBitmapManager::CY_TILE);
	if (yOffset >= (int)wPitHeight)
		return false;

	const UINT wCurY = this->wRow + UINT(fyOffset);
	if (wCurY >= this->pRoom->wRoomRows)
		return false;  //trapdoor fell off south end of room

	//Determine whether to display object still falling.
	const UINT wOSquare = this->pRoom->GetOSquare(this->wCol, wCurY);

	//Object fell behind something solid and will never reappear.
	if (!(bIsPit(wOSquare) || wOSquare==T_TRAPDOOR || wOSquare==T_PLATFORM_P || bIsBridge(wOSquare)))
		return false;

	//Clip object if it is occluded.
	const bool bClipTop = !bIsPit(wOSquare);
	const bool bClipBottom = (wCurY+1 >= this->pRoom->wRoomRows) ||
			!bIsPit(this->pRoom->GetOSquare(this->wCol, wCurY+1));
	if (bClipTop && bClipBottom)
	{
		//trapdoor is completely occluded this frame
		static const SDL_Rect emptyRect = {0,0,0,0};
		this->dirtyRects[0] = emptyRect;
		return true;
	}

	//Object fades as it falls.
	const BYTE nOpacity = g_pTheBM->bAlpha ? (BYTE)((1.0 - yOffset / float(wPitHeight)) * 255.0) : 255;

	//Calculate clipping needed as it falls behind objects.
	const int yPos = this->yTrapdoor + yOffset;
	SDL_Rect clipRect = MAKE_SDL_RECT(this->xTrapdoor, yPos,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	if (bClipTop || bClipBottom)
	{
		if (bClipTop)
		{
			const UINT yClipOffset = CBitmapManager::CY_TILE - yOffset % CBitmapManager::CY_TILE;
			clipRect.y += yClipOffset;
			clipRect.h -= yClipOffset;
		} else {
			ASSERT(bClipBottom);
			clipRect.h -= yOffset % CBitmapManager::CY_TILE;
		}
	}

	if (nOpacity < 255)
		EnableSurfaceBlending(this->pSurface, nOpacity);
	SDL_Rect src = MAKE_SDL_RECT(0, clipRect.y - yPos, CBitmapManager::CX_TILE, clipRect.h);
	SDL_BlitSurface(this->pSurface, &src, pDestSurface, &clipRect);

	this->dirtyRects[0] = clipRect;

	//Object is still falling.  Continue effect.
	return true;
}
