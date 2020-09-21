// $Id: BloodEffect.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Contributor(s): mrimer
 *
 * ***** END LICENSE BLOCK ***** */

#include "BloodEffect.h"
#include "TileImageConstants.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CBloodEffect::CBloodEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in)   Location of blood and direction of its movement.
	const UINT wParticles,  //(in) number of particles to generate
	const UINT wParticleMinDuration,
	const UINT baseSpeed)
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 12, 12, 2, wParticles,
			wParticleMinDuration, baseSpeed)
{
	this->tileNums[0] = TI_BLOOD_1;
	this->tileNums[1] = TI_BLOOD_2;
	this->xDims[0] = this->yDims[0] = 8;
	this->xDims[1] = this->yDims[1] = 12;

	InitParticles();
}

//********************************************************************************
CBloodInWallEffect::CBloodInWallEffect(
//Particles move more slowly than for the normal effect.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in)   Location of blood and direction of its movement.
	const UINT wParticles)  //(in) number of particles to generate
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 12, 12, 2, wParticles, 12, 1)
{
	//Don't need to rotate particles.
	this->bRotatingParticles = false;

	this->tileNums[0] = TI_BLOOD_G_1;
	this->tileNums[1] = TI_BLOOD_G_2;
	this->xDims[0] = this->yDims[0] = 8;
	this->xDims[1] = this->yDims[1] = 12;
	InitParticles();
}

//********************************************************************************
bool CBloodInWallEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!MoveParticles()) return false;

	if (!pDestSurface) pDestSurface = GetDestSurface();

	Uint8 nOpacity;
	const Uint8 STEPSIZE = 255 / this->wParticleMinDuration;

	for (int nIndex=wParticleCount; nIndex--; )
	{
		PARTICLE& p = this->parrParticles[nIndex];

		//If particle is still active, plot to display.
		if (p.bActive)
		{
			//Particles slowly fade away.
			nOpacity = 255 - (this->wParticleMinDuration - p.wDurationLeft) * STEPSIZE;
			if (p.type)
				g_pTheBM->BlitTileImagePart(TI_BLOOD_G_2, ROUND(p.x),
						ROUND(p.y), 0, 0, 12, 12, pDestSurface, true, nOpacity);
			else
				g_pTheBM->BlitTileImagePart(TI_BLOOD_G_1, ROUND(p.x),
						ROUND(p.y), 0, 0, 8, 8, pDestSurface, true, nOpacity);
		}
	}

	return true;
}

//*****************************************************************************
bool CBloodInWallEffect::HitsObstacle(const CDbRoom *pRoom, const PARTICLE& /*particle*/) const
//Nothing is an obstacle to this effect.
{
	ASSERT(pRoom);
	return false;
}
