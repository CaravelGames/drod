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

#ifndef FLOATEFFECT_H
#define FLOATEFFECT_H

#include "Effect.h"
#include <BackEndLib/Coord.h>

//*****************************************************************************
class CFloatEffect : public CEffect
{
public:
	CFloatEffect(CWidget *pSetWidget, const CMoveCoord &origin, const UINT wTileNo,
			const UINT wXSize, const UINT wYSize);

	virtual bool   Draw(SDL_Surface* pDestSurface=NULL);

private:
	bool OutOfBounds() const;
	void ResetParticle();

	float fX, fY;        //real position
	CMoveCoord origin;   //starting location
	UINT wTileNo;        //image
	UINT wXSize, wYSize; //image dimensions
	SDL_Rect screenRect;
};

#endif   //...#ifndef FLOATEFFECT_H
