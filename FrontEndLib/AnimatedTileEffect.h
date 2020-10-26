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

#ifndef ANIMATEDTILEEFFECT_H
#define ANIMATEDTILEEFFECT_H

#include "Effect.h"
#include <BackEndLib/Coord.h>

//****************************************************************************************
class CAnimatedTileEffect : public CEffect
{
public:
	CAnimatedTileEffect(CWidget *pSetWidget, const CCoord &SetCoord,
		const UINT dwDuration, const UINT wTileNo, const bool bUseLightLevel,
		const UINT eType=EFFECTLIB::EGENERIC);

	UINT GetX() const {return this->wX;}
	UINT GetY() const {return this->wY;}

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);
	void DrawTile(const UINT wTileImageNo, SDL_Surface& destSurface, const Uint8 nOpacity=255);
	void ShadeTile(const SURFACECOLOR &Color, SDL_Surface& destSurface);

	UINT	 wTileNo;
	bool     bUseLightLevel;

	Uint8    nOpacity;
	UINT     wX, wY;
};

#endif //...#ifndef ANIMATEDTILEEFFECT_H
