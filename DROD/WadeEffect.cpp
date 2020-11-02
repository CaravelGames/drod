// $Id: WadeEffect.cpp 8019 2007-07-14 22:30:11Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2011
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "WadeEffect.h"
#include "TileImageConstants.h"

const UINT EffectDuration = 250;

//********************************************************************************
CWadeEffect::CWadeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord)
	: CAnimatedTileEffect(pSetWidget,SetCoord,EffectDuration,TI_WADE1,true)
{
}

//********************************************************************************
bool CWadeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	const float fPercent = dwTimeElapsed / float(dwDuration);
	static const UINT NUM_FRAMES = 3;
	static const UINT FRAME[NUM_FRAMES] = {TI_WADE1, TI_WADE2, TI_WADE3};
	const UINT frame = UINT(GetElapsedFraction() * NUM_FRAMES);
	ASSERT(frame < NUM_FRAMES);

	this->wTileNo = FRAME[frame];

	//Effect fades as it progresses.
	this->nOpacity = static_cast<Uint8>(GetRemainingFraction() * 255);

	return true;
}
