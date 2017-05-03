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

#include "Fade.h"
#include "BitmapManager.h"
#include "Screen.h"  // PresentRect

#include <BackEndLib/Assert.h>

#include <memory.h>

//*****************************************************************************
void CFade::InitFade(
//Initialize vars for fade between two surfaces.
//Called upon construction.
//
//Params:
	SDL_Surface* pOldSurface,  //(in)   Surface that contains starting image and 
								//    will be destination surface that ends
								//    up faded to new surface when routine exits.
								//If set to NULL (for a fade-in), call GetDestSurface to access it
	SDL_Surface* pNewSurface)  //(in)   Image that destination surface will change to.
{
	//Set NULL pointers to temporary black screens
	//(for simple fade-in/out effects).
	if (!pOldSurface)
	{
		bOldNull = true;
		if (!pNewSurface)
		{
			bNewNull = true;
			return;  //nothing to do
		}
		pOldFadeSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, pNewSurface->w, pNewSurface->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		if (!pOldFadeSurface) return;
		SDL_FillRect(pOldFadeSurface,NULL,0);  //make black screen
	} else {
		pOldFadeSurface = pOldSurface;   //keep track of surfaces
	}
	if (!pNewSurface)
	{
		ASSERT(pOldSurface);
		bNewNull = true;
		pNewFadeSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
				SDL_SWSURFACE, pOldFadeSurface->w, pOldFadeSurface->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
		if (!pNewFadeSurface) return;
		SDL_FillRect(pNewFadeSurface,NULL,0);  //make black screen
	} else {
		pNewFadeSurface = pNewSurface;
	}

	// Lock surface for direct access to the pixels.
	if ( SDL_MUSTLOCK(pOldFadeSurface) )
		if ( SDL_LockSurface(pOldFadeSurface) < 0 )
			return;

	//The new surface shouldn't need a lock unless it is somehow a screen surface.
	ASSERT(!SDL_MUSTLOCK(pNewFadeSurface));

	//The dimensions and format of the old and new surface must match exactly.
	ASSERT(pOldFadeSurface->pitch == pNewFadeSurface->pitch);
	ASSERT(pOldFadeSurface->w == pNewFadeSurface->w);
	ASSERT(pOldFadeSurface->h == pNewFadeSurface->h);
	ASSERT(pOldFadeSurface->format->Rmask == pNewFadeSurface->format->Rmask);
	ASSERT(pOldFadeSurface->format->Rshift == pNewFadeSurface->format->Rshift);
	ASSERT(pOldFadeSurface->format->Rloss == pNewFadeSurface->format->Rloss);
	ASSERT(pOldFadeSurface->format->Gmask == pNewFadeSurface->format->Gmask);
	ASSERT(pOldFadeSurface->format->Gshift == pNewFadeSurface->format->Gshift);
	ASSERT(pOldFadeSurface->format->Gloss == pNewFadeSurface->format->Gloss);
	ASSERT(pOldFadeSurface->format->Bmask == pNewFadeSurface->format->Bmask);
	ASSERT(pOldFadeSurface->format->Bshift == pNewFadeSurface->format->Bshift);
	ASSERT(pOldFadeSurface->format->Bloss == pNewFadeSurface->format->Bloss);
	ASSERT(pOldFadeSurface->format->Rshift == pNewFadeSurface->format->Rshift);
	ASSERT(pOldFadeSurface->format->BytesPerPixel == pNewFadeSurface->format->BytesPerPixel);

	//Extract RGB pixel values from each image.
	ASSERT(pNewFadeSurface->format->BytesPerPixel==g_pTheBM->BYTES_PER_PIXEL); //32-bit color only supported

	const Uint32 size = pOldFadeSurface->pitch * pOldFadeSurface->h;
	fadeFromRGB = new Uint8[size];
	memcpy(fadeFromRGB, (Uint8 *)pOldFadeSurface->pixels, size);

	if ( SDL_MUSTLOCK(pOldFadeSurface) ) SDL_UnlockSurface(pOldFadeSurface);
}

//*****************************************************************************
void CFade::ExitFade()
//Complete fade and clean up vars.
{
	if (bOldNull && bNewNull) return;   //no fade to do

	if (fadeFromRGB && pOldFadeSurface && pNewFadeSurface)
	{
		//Show new surface entirely.
		SDL_BlitSurface(pNewFadeSurface, NULL, pOldFadeSurface, NULL);
		PresentRect(pOldFadeSurface);
	}

	//Clean up.
	delete[] fadeFromRGB;
	fadeFromRGB = NULL;

	if (bOldNull)
		SDL_FreeSurface(pOldFadeSurface);
	if (bNewNull)
		SDL_FreeSurface(pNewFadeSurface);
	pOldFadeSurface = pNewFadeSurface = NULL;
}

//*****************************************************************************
void CFade::IncrementFade(
//Show transition between two surfaces at 'ratio' between them.
//
//'pOldFadeSurface' is the surface the fade is applied to,
//'fadeFromRGB' is a copy of what the old screen looks like, used in fade calculations.
//NOTE: fadeFromRGB, pNewFadeSurface, and pOldFadeSurface must be the same size and dimensions.
//
//Params:
	float fRatio)  //(in) mixing ratio [0,1].
{
	if (bOldNull && bNewNull) return;   //no fade to do

	if (fRatio < 0.0) fRatio = 0.0;
	if (fRatio > 1.0) fRatio = 1.0;

	const Uint8 amount = (Uint8)(fRatio * 255);
	const Uint8 nOldAmt = 255 - amount;

	const Uint8 *pFrom = fadeFromRGB;
	const Uint8 *pTo = (Uint8 *)pNewFadeSurface->pixels;
	
	//Mix pixels in "from" and "to" images by 'amount'.
	Uint8 *pw = (Uint8 *)pOldFadeSurface->pixels;
	const Uint32 size = pOldFadeSurface->pitch * pOldFadeSurface->h;
	Uint8 *const pStop = pw + size;

	if ( SDL_MUSTLOCK(pOldFadeSurface) )
		if ( SDL_LockSurface(pOldFadeSurface) < 0 )
			return;

	do {
		//dividing by 256 instead of 255 provides huge optimization
		*pw = (nOldAmt * *(pFrom++) + amount * *(pTo++)) / 256;
	} while (++pw != pStop);

	if ( SDL_MUSTLOCK(pOldFadeSurface) ) SDL_UnlockSurface(pOldFadeSurface);

	PresentRect(pOldFadeSurface);
}

//*****************************************************************************
void CFade::FadeBetween(
//Fade between two surfaces of the same dimensions.
//Time effect to be independent of machine speed.
//
//Params:
	const UINT wFadeDuration)
{
	if ((bOldNull && bNewNull) || wFadeDuration == 0)
		return;   //no fade to do

	//Fade from old to new surface.  Effect takes constant time.
	UINT
		dwFirstPaint = SDL_GetTicks(),
		dwNow = dwFirstPaint;
	do
	{
		//The +50 is to allow first frame to show some change.
		IncrementFade((dwNow - dwFirstPaint + 50) / (float)wFadeDuration);
		dwNow = SDL_GetTicks();
	} while (dwNow - dwFirstPaint + 50 < wFadeDuration);  // constant-time effect
}
