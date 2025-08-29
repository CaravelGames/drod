// $Id: DebrisEffect.cpp 9096 2008-07-09 05:45:56Z mrimer $

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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TileImageConstants.h"
#include "DrodBitmapManager.h"
#include "DebrisEffect.h"
#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CDebrisEffect::CDebrisEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in)   Location of debris and direction of its movement.
	const UINT wMinParticles,	//(in)   Pieces of debris [default=10]
	const UINT wParticleMinDuration, const UINT wParticleSpeed)
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 22, 26, 2, wMinParticles,
			wParticleMinDuration, wParticleSpeed)
{
	this->tileNums[0] = TI_DEBRIS_1;
	this->tileNums[1] = TI_DEBRIS_2;
	this->xDims[0] = 22;
	this->yDims[0] = 26;
	this->xDims[1] = 12;
	this->yDims[1] = 16;

	InitParticles();
}

//********************************************************************************
CGolemDebrisEffect::CGolemDebrisEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in)   Location of debris and direction of its movement.
	const UINT wMinParticles,	//(in)   Pieces of debris [default=10]
	const UINT wParticleMinDuration, const UINT wParticleSpeed)
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 12, 16, 2, wMinParticles,
			wParticleMinDuration, wParticleSpeed)
{
	this->tileNums[0] = TI_GOLEMDEBRIS1;
	this->tileNums[1] = TI_GOLEMDEBRIS2;
	this->xDims[0] = 12;
	this->yDims[0] = 16;
	this->xDims[1] = 8;
	this->yDims[1] = 8;

	InitParticles();
}

//********************************************************************************
CDigDebrisEffect::CDigDebrisEffect(
//Constructor.
//
//Params:
	CWidget* pSetWidget,       //(in)   Should be a room widget.
	const CMoveCoord& MoveCoord,  //(in)   Location of debris and direction of its movement.
	const UINT wMinParticles,	//(in)   Pieces of debris [default=10]
	const UINT wParticleMinDuration, const UINT wParticleSpeed)
	: CParticleExplosionEffect(pSetWidget, MoveCoord, 12, 16, 2, wMinParticles,
		wParticleMinDuration, wParticleSpeed)
{
	this->tileNums[0] = TI_GOLEMDEBRIS1;
	this->tileNums[1] = TI_GOLEMDEBRIS2;
	this->xDims[0] = 12;
	this->yDims[0] = 16;
	this->xDims[1] = 8;
	this->yDims[1] = 8;

	InitParticles();
}
