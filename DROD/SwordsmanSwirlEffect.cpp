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

#include "SwordsmanSwirlEffect.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/CurrentGame.h"
#include <BackEndLib/Assert.h>

//********************************************************************************
CSwordsmanSwirlEffect::CSwordsmanSwirlEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,                //(in) Should be a room widget.
	const CCurrentGame *pCurrentGame)   //(in) to track the swordsman's position
	: CSwirlEffect(pSetWidget, ESWIRL)
	, pCurrentGame(pCurrentGame)
	, wOldX((UINT)-1), wOldY((UINT)-1)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
}


//********************************************************************************
bool CSwordsmanSwirlEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (!this->pCurrentGame->swordsman.IsInRoom())
		return false; //Player is not in the room.  Don't show effect.

	//Swirl gradually fades out.
	this->nOpacity = g_pTheBM->bAlpha ? 255 - ((dwTimeElapsed * 255) / dwDuration) : 255;

	//Center effect on player's position.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);

	const CMoveCoord* pCoord = &this->pCurrentGame->swordsman;

	const UINT wSX = pCoord->wX;
	const UINT wSY = pCoord->wY;
	const UINT wXNewCenter = OwnerRect.x + (wSX * CBitmapManager::CX_TILE) +
		CBitmapManager::CX_TILE / 4;
	const UINT wYNewCenter = OwnerRect.y + (wSY * CBitmapManager::CY_TILE) +
		CBitmapManager::CY_TILE / 4;
	//As swordsman moves, have effect pursue him.
	UINT wXCenter, wYCenter;
	if (wOldX == static_cast<UINT>(-1))
	{
		//Start at this position.
		wXCenter = this->wOldX = wXNewCenter;
		wYCenter = this->wOldY = wYNewCenter;
	}
	else {
		this->wOldX = wXCenter = (this->wOldX + static_cast<int>(wXNewCenter - this->wOldX) / 4);
		this->wOldY = wYCenter = (this->wOldY + static_cast<int>(wYNewCenter - this->wOldY) / 4);
	}

	//Draw swirl.
	CalculateSwirl(wXCenter, wYCenter);

	return true;
}
