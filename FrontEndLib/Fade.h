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

#ifndef FADE_H
#define FADE_H

#include <BackEndLib/Types.h>

#include <SDL.h>

//Fade between two SDL_Surfaces.
class CFade
{
public:
	CFade(SDL_Surface* pOldSurface, SDL_Surface* pNewSurface)
		: pOldFadeSurface(NULL), pNewFadeSurface(NULL)
		, fadeFromRGB(NULL)
		, bOldNull(false), bNewNull(false)
			{InitFade(pOldSurface,pNewSurface);}

	~CFade() {ExitFade();}

	//Performs a step of the fade.
	void IncrementFade(float fRatio);

	//Performs entire fade.
	void FadeBetween(const UINT wFadeDuration=400); //How long to fade, in milliseconds

	SDL_Surface* GetDestSurface() const {return pOldFadeSurface;}

private:
	SDL_Surface *pOldFadeSurface, *pNewFadeSurface;
	Uint8 *fadeFromRGB;  //surface pixels
	bool bOldNull, bNewNull;         //whether a surface is NULL

	void InitFade(SDL_Surface* pOldSurface, SDL_Surface* pNewSurface);
	void ExitFade();
};

#endif //...#ifndef FADE_H
