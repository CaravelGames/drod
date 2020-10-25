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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Effect.h"
#include "Screen.h"
#include <BackEndLib/Assert.h>

//*****************************************************************************
CEffect::CEffect(CWidget *pSetOwnerWidget, const UINT dwDuration, const UINT eType)
	: pOwnerWidget(pSetOwnerWidget)
	, dwTimeOfLastMove(CScreen::dwLastRenderTicks)
	, eEffectType(eType)
	, bRequestRetainOnClear(false)
	, fOpacity(1.0)
	, dwTimeElapsed(0)
	, dwDuration(dwDuration)
//Constructor.
{
	ASSERT(pSetOwnerWidget);
}

//*****************************************************************************
bool CEffect::Update(
	const UINT wDeltaTime)     //(in) Time between this and last draw, can be 0 to just draw the effect as-is without any state update
{
	this->dwTimeElapsed += wDeltaTime;

	if (this->dwTimeElapsed >= this->dwDuration)
		return false;

	return this->Update(wDeltaTime, this->dwTimeElapsed);
}

//*****************************************************************************
void CEffect::Draw(
	SDL_Surface* pDestSurface) //(in) Surface on which to draw the effect, defaults to owner widget's destination surface
//Return: the time elapsed since effect started.
{
	if (!pDestSurface)
		pDestSurface = GetDestSurface();
	
	ASSERT(pDestSurface);
	if (pDestSurface)
		Draw(*pDestSurface);
}

//*****************************************************************************
float CEffect::GetElapsedFraction() const
//Return: the fraction of the duration that has already elapsed
{
	const float fraction = this->dwDuration > 0
		? float(this->dwTimeElapsed) / float(this->dwDuration)
		: 1.0f;

	if (fraction < 0)
		return 0.0f;
	
	if (fraction > 1)
		return 1.0f;

	return fraction;
}

//*****************************************************************************
float CEffect::GetRemainingFraction() const
//Return: the fraction of the duration that is left
{
	return 1 - GetElapsedFraction();
}