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
 * Contributor(s): mrimer
 *
 * ***** END LICENSE BLOCK ***** */

#include "SparkEffect.h"
#include "TileImageConstants.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CSparkEffect::CSparkEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in)   Location of spark and direction of its movement.
	const UINT wParticles,  //(in) number of particles to generate
	const bool bContinue,   //(in) effect plays indefinitely [default=true]
	const bool bFromEdge)   //[default=false]
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 4, 4, 3, wParticles, 4, 3,
			bContinue, bFromEdge, ESPARK)
{
	this->bRotatingParticles = false;

	this->tileNums[0] = TI_SPARK_1;
	this->tileNums[1] = TI_SPARK_2;
	this->tileNums[2] = TI_SPARK_3;
	this->xDims[0] = this->yDims[0] = 4;
	this->xDims[1] = this->yDims[1] = 3;
	this->xDims[2] = this->yDims[2] = 2;
	InitParticles();
}

//********************************************************************************
bool CSparkEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (!MoveParticles(wDeltaTime))
		return false;

	return true;
}

//********************************************************************************
void CSparkEffect::Draw(SDL_Surface& destSurface)
{
	Uint8 nOpacity;
	const Uint8 STEPSIZE = 255 / this->wParticleMinDuration;
	for (int nIndex=wParticleCount; nIndex--; )
	{
		PARTICLE& p = this->parrParticles[nIndex];

		//Continuous effects: all particles are active.
		nOpacity = 255 - (this->wParticleMinDuration - p.wDurationLeft) * STEPSIZE; //alpha fade

		switch (p.type)
		{
			case 0:
				g_pTheBM->BlitTileImagePart(TI_SPARK_1, ROUND(p.x),
						ROUND(p.y), 0, 0, 4, 4, &destSurface, false, nOpacity);
				break;
			case 1:
				g_pTheBM->BlitTileImagePart(TI_SPARK_2, ROUND(p.x),
						ROUND(p.y), 0, 0, 3, 3, &destSurface, false, nOpacity);
				break;
			case 2:
				g_pTheBM->BlitTileImagePart(TI_SPARK_3, ROUND(p.x),
						ROUND(p.y), 0, 0, 2, 2, &destSurface, false, nOpacity);
				break;
		}
	}
}
