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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SNOWFLAKEEFFECT_H
#define SNOWFLAKEEFFECT_H

#include "DrodEffect.h"

//*****************************************************************************
class CSnowflakeEffect : public CEffect
{
public:
	CSnowflakeEffect(CWidget *pSetWidget);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

	void UpdateWind();

private:
	void UpdateFrame(float elapsedFraction);
	bool OutOfBounds() const;

	float fX, fY;         //real position
	static float fXDrift; //delta
	UINT wType;
	SDL_Rect screenRect;

	Uint8 nOpacity;
	UINT wTileNo;
	UINT wDrawSize;
};

#endif   //...#ifndef SNOWFLAKEEFFECT_H
