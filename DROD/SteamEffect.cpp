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

#include "SteamEffect.h"
#include "DrodBitmapManager.h"

const UINT SteamEffectDuration = 1000;

//*****************************************************************************
CSteamEffect::CSteamEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,   //(in) Should be a room widget.
	const CCoord &origin)  //(in) Location and initial direction of movement.
	: CEffect(pSetWidget, SteamEffectDuration, ESTEAM)
{
	pSetWidget->GetRect(this->screenRect);

	this->wDrawX = this->screenRect.x + origin.wX * CBitmapManager::CX_TILE;
	this->nInitialDrawY = this->screenRect.y + origin.wY * CBitmapManager::CY_TILE;

	//Bounding box is always of these dimensions.
	SDL_Rect rect = MAKE_SDL_RECT(this->wDrawX, this->nInitialDrawY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
bool CSteamEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	this->nDrawOpacity = g_pTheBM->bAlpha 
		? (Uint8)(GetRemainingFraction() * 255.0) 
		: 255;

	this->wDrawY = this->nInitialDrawY - int(GetElapsedFraction() * float(CDrodBitmapManager::CY_TILE / 2));
	this->wDrawClipY = this->wDrawY < UINT(this->screenRect.y) ? this->screenRect.y - this->wDrawY : 0;
	this->wDrawY += this->wDrawClipY;

	UpdateFrame();

	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].y = this->wDrawY;

	return true;
}

//*****************************************************************************
void CSteamEffect::Draw(SDL_Surface& destSurface)
{
	//Float upward half a tile.
	const float fPercent = dwTimeElapsed / float(dwDuration);

	//Ensure effect stays within parent widget's bounds.

	//Fade out.
	g_pTheBM->BlitTileImagePart(this->wDrawTile, 
			this->wDrawX, this->wDrawY,
			0, this->wDrawClipY,
			CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE - this->wDrawClipY,
			&destSurface, false, this->nDrawOpacity);
}

void CSteamEffect::UpdateFrame()
{
	static const UINT NUM_FRAMES = 8;
	const UINT frame = UINT(GetElapsedFraction() * NUM_FRAMES);
	static const UINT FRAME[NUM_FRAMES] = { 
		TI_SMOKE1, TI_SMOKE2, TI_SMOKE3, TI_SMOKE4,
		TI_SMOKE5, TI_SMOKE6, TI_SMOKE7, TI_SMOKE8 };

	this->wDrawTile = FRAME[frame < NUM_FRAMES ? frame : NUM_FRAMES - 1];
}
