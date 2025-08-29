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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "PuffExplosionEffect.h"
#include "DrodBitmapManager.h"

const Uint32 EffectDuration = 750; //ms
//*****************************************************************************
CPuffExplosionEffect::CPuffExplosionEffect(
	//Constructor.
	//
	//Params:
	CWidget* pSetWidget,   //(in) Should be a room widget.
	const CCoord& origin)  //(in) Location and initial direction of movement.
	: CAnimatedTileEffect(pSetWidget, origin, EffectDuration, 0, false, EPUFFEXPLOSION)
{
}

//*****************************************************************************
bool CPuffExplosionEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	static const UINT NUM_FRAMES = 6;

	static const UINT FRAME[NUM_FRAMES] = {
		TI_FLUFF_EXP1, TI_FLUFF_EXP2, TI_FLUFF_EXP3,
		TI_FLUFF_EXP4, TI_FLUFF_EXP5, TI_FLUFF_EXP6
	};

	const UINT frame = UINT(GetElapsedFraction() * NUM_FRAMES);
	this->wTileNo = FRAME[frame < NUM_FRAMES ? frame : NUM_FRAMES - 1];

	//Fade out.
	this->nOpacity = g_pTheBM->bAlpha
		? (BYTE)(GetRemainingFraction() * 255.0)
		: 255;

	return true;
}