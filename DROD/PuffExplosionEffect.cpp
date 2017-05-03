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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "PuffExplosionEffect.h"
#include "DrodBitmapManager.h"

//*****************************************************************************
CPuffExplosionEffect::CPuffExplosionEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,   //(in) Should be a room widget.
	const CCoord &origin)  //(in) Location and initial direction of movement.
	: CEffect(pSetWidget, EPUFFEXPLOSION)
{
	pSetWidget->GetRect(this->screenRect);

	this->nX = this->screenRect.x + origin.wX*CBitmapManager::CX_TILE;
	this->nY = this->screenRect.y + origin.wY*CBitmapManager::CY_TILE;

	//Bounding box is always of these dimensions.
	SDL_Rect rect = MAKE_SDL_RECT(this->nX, this->nY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
bool CPuffExplosionEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!pDestSurface) pDestSurface = GetDestSurface();

	static const Uint32 dwDuration = 750; //ms
	const Uint32 dwTimeElapsed = TimeElapsed();
	if (dwTimeElapsed > dwDuration)
		return false;

	//Draw animated dissipating cloud.
	const float fPercent = dwTimeElapsed / float(dwDuration);
	static const UINT NUM_FRAMES = 6;
	const UINT frame = UINT(fPercent * NUM_FRAMES);
	static const UINT FRAME[NUM_FRAMES] = {
		TI_FLUFF_EXP1, TI_FLUFF_EXP2, TI_FLUFF_EXP3,
		TI_FLUFF_EXP4, TI_FLUFF_EXP5, TI_FLUFF_EXP6};
	const UINT wTile = FRAME[frame < NUM_FRAMES ? frame : NUM_FRAMES-1];

	//Fade out.
	const BYTE nOpacity = g_pTheBM->bAlpha ?
			(BYTE)((1.0 - dwTimeElapsed/(float)dwDuration) * 255.0) : 255;
	g_pTheBM->BlitTileImagePart(wTile, this->nX, this->nY,
			0, 0, CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE,
			pDestSurface, false, nOpacity);

	//Update bounding box position.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->nX;
	this->dirtyRects[0].y = this->nY;

	return true;
}
