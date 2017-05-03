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

#ifndef PAN_H
#define PAN_H

#include <SDL.h>

enum PanDirection
{
	PanNorth=0,
	PanSouth,
	PanEast,
	PanWest,
	PanCount
};

//Pan between two SDL_Surfaces.
class CPan
{
public:
	CPan(SDL_Surface* pOldSurface, SDL_Surface* pNewSurface,
			const PanDirection panDirection)
		: bNewNull(false)
		, panDirection(panDirection)
			{InitPan(pOldSurface, pNewSurface);}

	~CPan() {ExitPan();}

	//Performs a step of the pan effect.
	void IncrementPan(const float fRatio);

	//Performs entire pan effect.
	void PanBetween(const int unsigned panDuration=400);  //How long to pan, in milliseconds.

private:
	SDL_Surface *pFromSurface, *pToSurface, *pDisplaySurface;
	bool bNewNull;       //whether dest. surface is NULL
	PanDirection panDirection; //panning direction

	void InitPan(SDL_Surface* pOldSurface, SDL_Surface* pNewSurface);
	void ExitPan(void);
};

#endif //...#ifndef PAN_H
