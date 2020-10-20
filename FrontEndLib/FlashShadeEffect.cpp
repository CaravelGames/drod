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

#include "FlashShadeEffect.h"

//********************************************************************************
CFlashShadeEffect::CFlashShadeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of checkpoint.
	const SURFACECOLOR &Color, //(in)   Color to shade with.
	const Uint8 flashSpeed)    //(in)   [default=1]
	: CShadeEffect(pSetWidget,SetCoord,Color)
	, bMakeBrighter(true)
	, brightness(0)
	, flashSpeed(flashSpeed)
{
	this->eEffectType = EFFECTLIB::EFLASHSHADE;
}

//********************************************************************************
bool CFlashShadeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Determine brightness of shade.
	if (this->bMakeBrighter)
	{
		if (this->brightness >= 255 - this->flashSpeed)
			this->bMakeBrighter = false;
		else
			this->brightness += this->flashSpeed;
	}
	else {
		if (this->brightness <= this->flashSpeed)
			this->bMakeBrighter = true;
		else
			this->brightness -= this->flashSpeed;
	}

	return true;
}

//********************************************************************************
void CFlashShadeEffect::Draw(SDL_Surface& pDestSurface)
{
	const float fMult = static_cast<float>(this->brightness) / 255.0f;
	SURFACECOLOR flashColor;
	flashColor.byt1 = static_cast<Uint8>(fMult * static_cast<float>(this->Color.byt1));
	flashColor.byt2 = static_cast<Uint8>(fMult * static_cast<float>(this->Color.byt2));
	flashColor.byt3 = static_cast<Uint8>(fMult * static_cast<float>(this->Color.byt3));

	//Add flashing shading to tile.
	ShadeTile(flashColor, pDestSurface);
}
