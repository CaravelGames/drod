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

#include "TransTileEffect.h"
#include "BitmapManager.h"
#include "Screen.h"

#include <BackEndLib/Assert.h>

//********************************************************************************
CTransTileEffect::CTransTileEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of checkpoint.
	const UINT wTileImageNo,   //(in)   Tile to display.
	const bool bUseLightLevel)
	: CAnimatedTileEffect(pSetWidget,SetCoord,(UINT)-1,wTileImageNo,bUseLightLevel,EFFECTLIB::ETRANSTILE)
{
}

//********************************************************************************
bool CTransTileEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	this->nOpacity = UpdateStaticState();

	return true;
}

//********************************************************************************
Uint8 CTransTileEffect::UpdateStaticState()
{
	static const unsigned char MIN_OPACITY = 32;
	static const unsigned char MAX_OPACITY = 192;
	static unsigned char nOpacity = MIN_OPACITY;
	static bool bRising = true;
	static Uint32 lastUpdatePresentCount = 0;

	// Already updated this frame
	if (CScreen::dwPresentsCount == lastUpdatePresentCount)
		return nOpacity;

	lastUpdatePresentCount = CScreen::dwPresentsCount;

	//Value is modified by only one of the instances.
	if (bRising)
	{
		nOpacity += 3;
		if (nOpacity > MAX_OPACITY)
			bRising = false;
	}
	else {
		nOpacity -= 3;
		if (nOpacity < MIN_OPACITY)
			bRising = true;
	}

	return nOpacity;
}