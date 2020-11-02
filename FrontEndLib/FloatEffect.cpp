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

#include "FloatEffect.h"
#include "BitmapManager.h"

//*****************************************************************************
CFloatEffect::CFloatEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in) Should be a room widget.
	const CMoveCoord &origin,  //(in) Location and initial direction of movement.
	const UINT wTileNo,        //(in) tile image to display
	const UINT wXSize, const UINT wYSize)
	: CEffect(pSetWidget, (UINT)-1, EFFECTLIB::EFLOAT)
	, origin(origin)
	, wTileNo(wTileNo)
	, wXSize(wXSize), wYSize(wYSize)
{
	ASSERT(wXSize);
	ASSERT(wYSize);

	pSetWidget->GetRect(this->screenRect);

	ResetParticle();

	//Bounding box is always of these dimensions.
	SDL_Rect rect = MAKE_SDL_RECT(this->fX, this->fY, wXSize, wYSize);
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
bool CFloatEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Update real position in real time.
	static const Uint32 dwDuration = 2000;
	if (dwTimeElapsed > dwDuration)
		ResetParticle();

	const float fMultiplier = wDeltaTime / 100.0f;
	this->fY -= fMultiplier;   //float upward


	this->nOpacity = g_pTheBM->bAlpha ?
		(BYTE)(GetRemainingFraction() * 255.0) : 
		255;

	//Update bounding box position.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = static_cast<Sint16>(this->fX);
	this->dirtyRects[0].y = static_cast<Sint16>(this->fY);

	return true;
}

//*****************************************************************************
void CFloatEffect::Draw(SDL_Surface& destSurface)
{
	if (!OutOfBounds())
		g_pTheBM->BlitTileImagePart(this->wTileNo, 
			static_cast<UINT>(this->fX), static_cast<UINT>(this->fY), 
			0, 0, this->wXSize, this->wYSize,
			&destSurface, false, this->nOpacity);
}

//*****************************************************************************
inline bool CFloatEffect::OutOfBounds() const
{
	return (this->fX < this->screenRect.x || this->fY < this->screenRect.y ||
			this->fX >= this->screenRect.x + this->screenRect.w - this->wXSize ||
			this->fY >= this->screenRect.y + this->screenRect.h - this->wYSize);
}

//*****************************************************************************
inline void CFloatEffect::ResetParticle()
//Generate randomly placed particle.
{
	const int nX = this->screenRect.x + this->origin.wX*CBitmapManager::CX_TILE;
	const int nY = this->screenRect.y + this->origin.wY*CBitmapManager::CY_TILE;
	this->fX = static_cast<float>(nX) + fRAND(CBitmapManager::CY_TILE);
	this->fY = static_cast<float>(nY);
	this->dwTimeElapsed = 0;
}
