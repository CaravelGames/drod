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

#include "Pan.h"
#include "BitmapManager.h"
#include "Screen.h"  // PresentRect

#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

//*****************************************************************************
void CPan::InitPan(
//Initialize vars for pan between two surfaces.
//Called upon construction.
//
//Params:
	SDL_Surface* pOldSurface,  //(in)   Surface containing starting image
										//    (the surface being drawn to)
	SDL_Surface* pNewSurface)  //(in)   Surface containing ending image
								//A NULL pointer signifies to pan to a black surface.
{
	ASSERT(pOldSurface);
	this->pDisplaySurface = pOldSurface;   //this surface displays the effect

	//Save the surface we're panning from.
	this->pFromSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, pOldSurface->w, pOldSurface->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	SDL_BlitSurface(pOldSurface, NULL, this->pFromSurface, NULL);

	if (!pNewSurface)
	{
		this->bNewNull = true;
		this->pToSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, this->pFromSurface->w, this->pFromSurface->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		SDL_FillRect(this->pToSurface,NULL,0);
	} else {
		this->pToSurface = pNewSurface;
	}

	//The dimensions and format of the old and new surface must match exactly.
	ASSERT(this->pFromSurface->pitch == this->pToSurface->pitch);
	ASSERT(this->pFromSurface->w == this->pToSurface->w);
	ASSERT(this->pFromSurface->h == this->pToSurface->h);
	ASSERT(this->pFromSurface->format->BytesPerPixel ==
			this->pToSurface->format->BytesPerPixel);
}

//*****************************************************************************
void CPan::ExitPan()
//Complete pan and clean up vars.
{

	//Clean up.
	SDL_FreeSurface(this->pFromSurface);
	if (this->bNewNull)
		SDL_FreeSurface(this->pToSurface);
	this->pFromSurface = this->pToSurface = this->pDisplaySurface = NULL;
}

//*****************************************************************************
void CPan::IncrementPan(
//Show transition between two surfaces at 'ratio' between them.
//
//'pFromSurface' is the surface you want to apply the pan to,
//'fadeFrom' is a copy of what the old surface looks like, and
//'fadeTo' is a copy of what the new surface looks like
//
//NOTE: fadeFrom, fadeTo, and pSurface must be the same size and dimensions.
//
//Params:
	const float fRatio)  //(in) panning distance covered [0.0,1.0).
{
	const bool bVertical = this->panDirection == PanNorth || this->panDirection == PanSouth;
	const int nSize = (bVertical ?
			this->pFromSurface->h : this->pFromSurface->w);
	const float fBoundedRatio = fRatio > 1.0f ? 1.0f : fRatio;
	const int nAmount = (int)(fBoundedRatio * nSize);

	//Show pan from "from" to "to" image by 'amount'.
	SDL_Rect fromSrc, fromDest, toSrc, toDest;
	switch (this->panDirection)
	{
		//North and South use the full width of the surface.
		//The height of each piece changes based on nAmount.
		case PanNorth:
			fromSrc.x = toSrc.x = fromDest.x = toDest.x = 0;
			fromSrc.w = toSrc.w = fromDest.w = toDest.w = this->pDisplaySurface->w;

			fromSrc.y = toDest.y = 0;
			fromDest.y = toSrc.h = toDest.h = nAmount;
			toSrc.y = fromSrc.h = fromDest.h = this->pFromSurface->h - nAmount;
		break;
		case PanSouth:
			fromSrc.x = toSrc.x = fromDest.x = toDest.x = 0;
			fromSrc.w = toSrc.w = fromDest.w = toDest.w = this->pDisplaySurface->w;

			fromDest.y = toSrc.y = 0;
			fromSrc.y = toSrc.h = toDest.h = nAmount;
			toDest.y = fromSrc.h = fromDest.h = this->pFromSurface->h - nAmount;
		break;

		//East and West use the full height of the surface.
		//The width of each piece changes based on nAmount.
		case PanEast:
			fromSrc.y = toSrc.y = fromDest.y = toDest.y = 0;
			fromSrc.h = toSrc.h = fromDest.h = toDest.h = this->pDisplaySurface->h;

			fromDest.x = toSrc.x = 0;
			fromSrc.x = toSrc.w = toDest.w = nAmount;
			toDest.x = fromSrc.w = fromDest.w = this->pFromSurface->w - nAmount;
		break;
		case PanWest:
			fromSrc.y = toSrc.y = fromDest.y = toDest.y = 0;
			fromSrc.h = toSrc.h = fromDest.h = toDest.h = this->pDisplaySurface->h;

			fromSrc.x = toDest.x = 0;
			fromDest.x = toSrc.w = toDest.w = nAmount;
			toSrc.x = fromSrc.w = fromDest.w = this->pFromSurface->w - nAmount;
		break;

		default: break;
	}

	SDL_BlitSurface(this->pFromSurface, &fromSrc, this->pDisplaySurface, &fromDest);
	SDL_BlitSurface(this->pToSurface, &toSrc, this->pDisplaySurface, &toDest);
	PresentRect(this->pDisplaySurface, 0, 0, 0, 0);
}

//*****************************************************************************
void CPan::PanBetween(
//Pan between two surfaces of the same dimensions.
//Time effect to be independent of machine speed.
//
//Params:
	const int unsigned wPanDuration) //(in)
{
	if (wPanDuration == 0) return;   //nothing to do

	//Allow first frame to show some change.
	const UINT dwFirstFrameIncrement = wPanDuration / 50;

	//Pan from old to new surface.  Effect takes constant time.
	UINT
		dwFirstPaint = SDL_GetTicks(),
		dwNow = dwFirstPaint;
	do
	{
		//The +50 is to allow first frame to show some change.
		IncrementPan((dwNow - dwFirstPaint + dwFirstFrameIncrement)
				/ (float)wPanDuration);
		dwNow = SDL_GetTicks();
	} while (dwNow - dwFirstPaint + dwFirstFrameIncrement < wPanDuration);
}
