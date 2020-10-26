// $Id: MovingTileEffect.h $

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

#ifndef MOVINGTILEEFFECT_H
#define MOVINGTILEEFFECT_H

#include "AnimatedTileEffect.h"

//*****************************************************************************
class CMovingTileEffect : public CAnimatedTileEffect
{
public:
	CMovingTileEffect(CWidget *pSetWidget, const UINT wTileNo, const CCoord &StartCoord,
		const CCoord &DestCoord, const float fSpeed, const bool bUseLightLevel=false,
		const UINT eType=EFFECTLIB::EGENERIC);

	void MoveTo(const UINT wX, const UINT wY, const float fSpeed=0.0,
			const bool bEndEffectAtDestination=false);

	enum Behavior
	{
		UniformSpeed,
		Accelerating
	};
	Behavior behavior;

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

	bool UpdateLocation(const UINT wDeltaTime, const Uint32 dwTimeElapsed);

	//Move effect to a different location.
	UINT     wMoveToX, wMoveToY;
	float    fX, fY, fSpeed;
	bool     bEndEffectAtDestination;

	SDL_Rect drawRect;
	bool     bDrawEffect;
};

#endif //...#ifndef MOVINGTILEEFFECT_H
