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

#include "IceMeltEffect.h"
#include "TileImageConstants.h"
#include <FrontEndLib/BitmapManager.h>

#define THIN_ICE_OPACITY       (128) //Opacity of Thin Ice

//********************************************************************************
CIceMeltEffect::CIceMeltEffect(
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of explosion.
	const UINT duration) //[default=500]
	: CAnimatedTileEffect(pSetWidget,SetCoord,duration,0,true)
{
}

//********************************************************************************
bool CIceMeltEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT dwTimeElapsed = TimeElapsed();
	if (dwTimeElapsed >= this->dwDuration)
		return false; //Effect is done.

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Draw shrinking ice.
	const float fPercent = (this->dwDuration - dwTimeElapsed) / (float)this->dwDuration;
	ASSERT(fPercent >= 0.0);
	ASSERT(fPercent <= 1.0);
	UINT wTile = TI_THINICE;
	if (fPercent < 0.20)
		wTile = TI_ICEMELT_4;
	else if (fPercent < 0.40)
		wTile = TI_ICEMELT_3;
	else if (fPercent < 0.60)
		wTile = TI_ICEMELT_2;
	else if (fPercent < 0.80)
		wTile = TI_ICEMELT_1;

	DrawTile(wTile, pDestSurface, THIN_ICE_OPACITY);

	//Continue effect.
	return true;
}
