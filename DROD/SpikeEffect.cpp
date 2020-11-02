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

#include "SpikeEffect.h"
#include "DrodEffect.h"
#include "TileImageConstants.h"
#include <FrontEndLib/BitmapManager.h>

//********************************************************************************
CSpikeEffect::CSpikeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of effect.
	const UINT duration)
	: CAnimatedTileEffect(pSetWidget,SetCoord,duration,0,true, ESPIKES)
{
}

//********************************************************************************
bool CSpikeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	static const UINT frames = 6;

	const UINT frame = UINT(GetElapsedFraction() * frames);
	switch (frame) {
	case 0: case 5:
		this->wTileNo = TI_FLOOR_SPIKES_UP1; break;
	case 1: case 4:
		this->wTileNo = TI_FLOOR_SPIKES_UP2; break;
	case 2: case 3:
		this->wTileNo = TI_FLOOR_SPIKES_UP3; break;
	default:
		ASSERT(!"Unsupported spikes frame");
		break;
	}

	return true;
}