// $Id: RotateTileEffect.cpp 8546 2008-01-22 17:20:21Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2008
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "ScaleTileEffect.h"
#include "BitmapManager.h"
#include <math.h>

//*****************************************************************************
CScaleTileEffect::CScaleTileEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const int xCenter, const int yCenter, //(in)   Location of effect's center of rotation.
	const UINT tile,        //tile to display
	const UINT duration,    //display duration
	const float fScaleRate)
	: CEffect(pSetWidget, duration, EFFECTLIB::ESCALETILE)
	, tileNo(UINT(-1))
	, xCenter(xCenter), yCenter(yCenter)
	, nOpacity(255)
	, fScaleRate(fScaleRate)
	, pScaledSurface(NULL)
{
	SetTile(tile);

	//Area of effect.
	SDL_Rect rect = {0, 0, 0, 0};
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
CScaleTileEffect::~CScaleTileEffect()
//Destructor.
{
	if (this->pScaledSurface)
		SDL_FreeSurface(this->pScaledSurface);
}

//*****************************************************************************
bool CScaleTileEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//If object is scaling with time, update image.
	if (this->fScaleRate != 0.0f)
	{
		//Determine current scale factor.
		const float fScale = this->fScaleRate * dwTimeElapsed / 1000;
		const int nWidth = int(this->srcRect.w + fScale); //!!weird math -- fix
		const int nHeight = int(this->srcRect.h + fScale);

		//Only re-render if change in scaling is sufficient so that
		//the different would be perceptible (speed optimization).
		if (abs(this->srcRect.w - nWidth) >= 1 ||
			abs(this->srcRect.h - nHeight) >= 1)
			SetScaleSize(nWidth, nHeight);
	}

	if (!this->pScaledSurface)
		return false;

	this->drawRect = MAKE_SDL_RECT(this->xCenter - this->pScaledSurface->w / 2,
		this->yCenter - this->pScaledSurface->h / 2,
		this->pScaledSurface->w, this->pScaledSurface->h);

	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0] = this->drawRect;

	return true;
}
//*****************************************************************************
void CScaleTileEffect::Draw(SDL_Surface& destSurface)
{
	SDL_Rect src = MAKE_SDL_RECT(0, 0, this->pScaledSurface->w, this->pScaledSurface->h);

	//Clip to parent rect.
	SDL_Rect ClipRect;
	this->pOwnerWidget->GetRect(ClipRect);
	SDL_SetClipRect(&destSurface, &ClipRect);
	g_pTheBM->BlitSurface(this->pScaledSurface, &src, &destSurface, &this->drawRect, this->nOpacity);
	SDL_SetClipRect(&destSurface, NULL);
}

//*****************************************************************************
void CScaleTileEffect::SetScaleSize(const int nWidth, const int nHeight)
//Rerenders the tile scale to the specified dimensions.
{
	if (this->pScaledSurface)
		SDL_FreeSurface(this->pScaledSurface);

	if (nWidth < 1 || nHeight < 1)
		this->pScaledSurface = NULL;
	else {
		//Set to transparent tile colorkey.
		ASSERT(this->pSrcSurface);
		const UINT wSurfaceIndex = g_pTheBM->GetTileSurfaceNumber(this->tileNo);
		g_pTheBM->SetSurfaceColorKey(this->tileNo, wSurfaceIndex, this->pSrcSurface);

		this->pScaledSurface = g_pTheBM->ScaleSurface(this->pSrcSurface, this->pSrcPixel,
				this->srcRect.w, this->srcRect.h, nWidth, nHeight);
	}
}

//*****************************************************************************
void CScaleTileEffect::SetTile(const UINT tileNo)
//Change the tile being rendered.
{
	if (tileNo == this->tileNo)
		return;

	ASSERT(tileNo < g_pTheBM->GetNextCustomTileNo());

	//Reset to full bounds.
	this->srcRect.x = this->srcRect.y = 0;
	this->srcRect.w = CBitmapManager::CX_TILE;
	this->srcRect.h = CBitmapManager::CY_TILE;

	//Get surface and pixel info for the tile to scale.
	this->tileNo = tileNo;
	this->pSrcSurface = g_pTheBM->GetTileSurface(this->tileNo);
	g_pTheBM->CropToOpaque(this->srcRect, this->pSrcSurface,
			g_pTheBM->GetTileSurfacePixel(this->tileNo, this->srcRect.x, this->srcRect.y));

	this->pSrcPixel = g_pTheBM->GetTileSurfacePixel(this->tileNo, this->srcRect.x, this->srcRect.y);

	//Render rotated tile.
	SetScaleSize(this->srcRect.w, this->srcRect.h);
}
