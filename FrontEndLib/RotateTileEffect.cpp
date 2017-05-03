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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "RotateTileEffect.h"
#include "BitmapManager.h"
#include <math.h>

#define MIN_ROTATION_INCREMENT (1.0) //minimum degrees since last rendering to rerender

//*****************************************************************************
CRotateTileEffect::CRotateTileEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const int xCenter, const int yCenter, //(in)   Location of effect's center of rotation.
	const UINT tile,        //tile to display
	const float fInitialAngleDeg, //initial rotation angle (degrees)
	const float fRotationRateDeg,    //rate of rotation (degrees)
	const UINT duration)    //display duration
	: CEffect(pSetWidget, EFFECTLIB::EROTATETILE)
	, dwDuration(duration)
	, tileNo(UINT(-1))
	, xCenter(xCenter), yCenter(yCenter)
	, nOpacity(255)
	, pRotatedSurface(NULL)
	, fInitialAngle(fInitialAngleDeg), fLastAngle(fInitialAngleDeg)
	, fRotationRate(fRotationRateDeg)
{
	SetTile(tile);

	//Area of effect.
	SDL_Rect rect = {0, 0, 0, 0};
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
CRotateTileEffect::~CRotateTileEffect()
//Destructor.
{
	if (this->pRotatedSurface)
		SDL_FreeSurface(this->pRotatedSurface);
}

//*****************************************************************************
bool CRotateTileEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const Uint32 dwElapsed = TimeElapsed();
	if (dwElapsed >= this->dwDuration)
		return false; //Effect is done.

	//If object is rotating with time, update image.
	if (this->fRotationRate != 0.0f)
	{
		//Determine current angle of rotation.
		const float fAngle = this->fInitialAngle + (this->fRotationRate * dwElapsed / 1000);

		//Only re-render if change in rotation is sufficient so that
		//the different would be perceivable (speed optimization).
		if (fabs(this->fLastAngle - fAngle) >= MIN_ROTATION_INCREMENT)
			SetRotationAngle(fAngle);
	}

	if (!this->pRotatedSurface)
		return false;

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Draw rotated surface.
	SDL_Rect src = MAKE_SDL_RECT(0, 0, this->pRotatedSurface->w, this->pRotatedSurface->h);
	SDL_Rect dest = MAKE_SDL_RECT(this->xCenter - this->pRotatedSurface->w/2,
			this->yCenter - this->pRotatedSurface->h/2,
			this->pRotatedSurface->w, this->pRotatedSurface->h);

	//Clip to parent rect.
	SDL_Rect ClipRect;
	this->pOwnerWidget->GetRect(ClipRect);
	SDL_SetClipRect(pDestSurface, &ClipRect);
	g_pTheBM->BlitSurface(this->pRotatedSurface, &src, pDestSurface, &dest, nOpacity);
	SDL_SetClipRect(pDestSurface, NULL);

	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0] = dest;

	//Continue effect.
	return true;
}

//*****************************************************************************
void CRotateTileEffect::SetRotationAngle(const float fAngle)
//Rerenders the tile rotated to the specified angle.
{
	if (this->pRotatedSurface)
		SDL_FreeSurface(this->pRotatedSurface);

	//Set to transparent tile colorkey.
	ASSERT(this->pSrcSurface);
	const UINT wSurfaceIndex = g_pTheBM->GetTileSurfaceNumber(this->tileNo);
	g_pTheBM->SetSurfaceColorKey(this->tileNo, wSurfaceIndex, this->pSrcSurface);

	this->pRotatedSurface = g_pTheBM->RotateSurface(this->pSrcSurface, this->pSrcPixel,
			this->srcRect.w, this->srcRect.h, fAngle);
	this->fLastAngle = fAngle;
}

//*****************************************************************************
void CRotateTileEffect::SetTile(const UINT tileNo)
//Change the tile being rendered.
{
	if (tileNo == this->tileNo)
		return;

	ASSERT(tileNo < g_pTheBM->GetNextCustomTileNo());

	//Reset to full bounds.
	this->srcRect.x = this->srcRect.y = 0;
	this->srcRect.w = CBitmapManager::CX_TILE;
	this->srcRect.h = CBitmapManager::CY_TILE;

	//Get surface and pixel info for the tile to rotate.
	this->tileNo = tileNo;
	this->pSrcSurface = g_pTheBM->GetTileSurface(this->tileNo);
	g_pTheBM->CropToOpaque(this->srcRect, this->pSrcSurface,
			g_pTheBM->GetTileSurfacePixel(this->tileNo, this->srcRect.x, this->srcRect.y));

	this->pSrcPixel = g_pTheBM->GetTileSurfacePixel(this->tileNo, this->srcRect.x, this->srcRect.y);

	//Render rotated tile.
	SetRotationAngle(this->fInitialAngle);
}
