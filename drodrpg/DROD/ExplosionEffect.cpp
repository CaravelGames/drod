// $Id: ExplosionEffect.cpp 8102 2007-08-15 14:55:40Z trick $

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

#include "ExplosionEffect.h"
#include "TileImageConstants.h"
#include <FrontEndLib/BitmapManager.h>

//********************************************************************************
CExplosionEffect::CExplosionEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of explosion.
	const UINT duration) //[default=500]
	: CAnimatedTileEffect(pSetWidget,SetCoord,duration,0,false)
{
}

//********************************************************************************
bool CExplosionEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	//Draw shrinking explosion.
	const float elapsedFraction = GetRemainingFraction();

	if (elapsedFraction < 0.30)
		this->wTileNo = TI_EXPLOSION_4;
	else if (elapsedFraction < 0.55)
		this->wTileNo = TI_EXPLOSION_3;
	else if (elapsedFraction < 0.80)
		this->wTileNo = TI_EXPLOSION_2;
	else
		this->wTileNo = TI_EXPLOSION_1;

	return true;
}