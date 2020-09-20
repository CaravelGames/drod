// $Id: SteamEffect.cpp 10126 2012-04-24 05:40:08Z mrimer $

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

#include "SteamEffect.h"
#include "DrodBitmapManager.h"

//*****************************************************************************
CSteamEffect::CSteamEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,   //(in) Should be a room widget.
	const CCoord &origin)  //(in) Location and initial direction of movement.
	: CEffect(pSetWidget, ESTEAM)
{
	pSetWidget->GetRect(this->screenRect);

	this->nX = this->screenRect.x + origin.wX*CBitmapManager::CX_TILE;
	this->nY = this->screenRect.y + origin.wY*CBitmapManager::CY_TILE;

	//Bounding box is always of these dimensions.
	SDL_Rect rect = MAKE_SDL_RECT(this->nX, this->nY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
bool CSteamEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!pDestSurface) pDestSurface = GetDestSurface();

	static const Uint32 dwDuration = 1000; //ms
	const Uint32 dwTimeElapsed = TimeElapsed();
	if (dwTimeElapsed > dwDuration)
		return false;

	const Uint32 dwNow = SDL_GetTicks();
	this->dwTimeOfLastMove = dwNow;

	//Float upward half a tile.
	const float fPercent = dwTimeElapsed / float(dwDuration);
	const int yPos = this->nY - int(fPercent * float(CDrodBitmapManager::CY_TILE/2));

	//Ensure effect stays within parent widget's bounds.
	const int yClip = yPos < this->screenRect.y ? this->screenRect.y - yPos : 0;

	//Draw animated dissipating cloud.
	ASSERT(fPercent <= 1.0);
	static const UINT NUM_FRAMES = 8;
	const UINT frame = UINT(fPercent * NUM_FRAMES);
	static const UINT FRAME[NUM_FRAMES] = {TI_SMOKE1, TI_SMOKE2, TI_SMOKE3,
		TI_SMOKE4, TI_SMOKE5, TI_SMOKE6, TI_SMOKE7, TI_SMOKE8};
	UINT wTile = FRAME[frame < NUM_FRAMES ? frame : NUM_FRAMES-1];

	//Fade out.
	const BYTE nOpacity = g_pTheBM->bAlpha ?
			(BYTE)((1.0 - dwTimeElapsed/(float)dwDuration) * 255.0) : 255;
	g_pTheBM->BlitTileImagePart(wTile, this->nX, yPos + yClip,
			0, yClip, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE - yClip,
			pDestSurface, false, nOpacity);

	//Update bounding box position.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->nX;
	this->dirtyRects[0].y = yPos;

	return true;
}
