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

#include "FiretrapEffect.h"
#include "DrodEffect.h"
#include "TileImageConstants.h"
#include <FrontEndLib/BitmapManager.h>

//********************************************************************************
CFiretrapEffect::CFiretrapEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of effect.
	const UINT duration)
	: CAnimatedTileEffect(pSetWidget,SetCoord,duration,0,false, EFIRETRAP)
{
}

//********************************************************************************
bool CFiretrapEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const UINT dwTimeElapsed = TimeElapsed();
	if (dwTimeElapsed >= this->dwDuration)
		return false; //Effect is done.

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Draw animation.
	static const UINT frames = 9;
	const float fPercent = (this->dwDuration - dwTimeElapsed) / float(this->dwDuration + 1);
	ASSERT(fPercent >= 0.0);
	ASSERT(fPercent < 1.0);

	UINT wTile;
	const UINT frame = UINT(fPercent * frames);
	switch (frame % 3) {
		case 0:
			wTile = TI_FIRETRAP_UP1; break;
		case 1:
			wTile = TI_FIRETRAP_UP2; break;
		case 2:
			wTile = TI_FIRETRAP_UP3; break;
		default:
			ASSERT(!"Unsupported firetrap frame");
			break;
	}

	//Fade out.
	const BYTE nOpacity = g_pTheBM->bAlpha ? (BYTE)(fPercent * 255.0) : 255;

	DrawTile(wTile, pDestSurface, nOpacity);

	//Continue effect.
	return true;
}
