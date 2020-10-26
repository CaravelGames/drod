// $Id: SnowflakeEffect.h 8019 2007-07-14 22:30:11Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005, 2011
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef RAINDROPEFFECT_H
#define RAINDROPEFFECT_H

#include "DrodEffect.h"

//*****************************************************************************
class CRoomWidget;
class CRaindropEffect : public CEffect
{
public:
	CRaindropEffect(CWidget *pSetWidget, bool bHasted);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	bool TryToEndWithSplash();
	virtual void Draw(SDL_Surface& destSurface);

private:
	bool OutOfBounds() const;
	void CalculateFrameProperties();
	static void UpdateWind();

	static float fXDrift; //delta (wind drift)
	float fX, fY;         //real position
	float fGoalY;         //stop at this position
	UINT wType;
	bool bHasted;         //whether player is hasted

	CRoomWidget *pRoomWidget;
	SDL_Rect screenRect;

	//Used for drawing only
	UINT wDrawXSize;
	UINT wDrawYSize;
};

#endif   //...#ifndef RAINDROPEFFECT_H
