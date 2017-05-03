// $Id: CheckpointEffect.cpp 8546 2008-01-22 17:20:21Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "FadeTileEffect.h"

//*****************************************************************************
CFadeTileEffect::CFadeTileEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord, //(in)   Location of checkpoint.
	const UINT tile,        //tile to display
	const UINT duration)    //[default=2000ms]
	: CAnimatedTileEffect(pSetWidget,SetCoord,duration,tile,false, EFFECTLIB::EFADETILE)
{
}

//*****************************************************************************
bool CFadeTileEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	const Uint32 dwElapsed = TimeElapsed();
	if (dwElapsed >= this->dwDuration)
		return false; //Effect is done.

	if (!pDestSurface) pDestSurface = GetDestSurface();

	//Draw lit up checkpoint.
	const Uint32 dwFullOpacityTime = this->dwDuration/4;
	const Uint32 dwFadeTime = this->dwDuration - dwFullOpacityTime;
	const float fMultiplier = 255.0f / (float)dwFadeTime;
	const Uint8 opacity = dwElapsed > dwFullOpacityTime ?
			static_cast<Uint8>((dwFadeTime-(dwElapsed-dwFullOpacityTime)) * fMultiplier) :
			255;
	DrawTile(this->wTileNo, pDestSurface, opacity);

	//Continue effect.
	return true;
}
