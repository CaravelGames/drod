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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2012
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "StunEffect.h"
#include "DrodEffect.h"
#include "TileImageConstants.h"
#include "RoomWidget.h"
#include "../DRODLib/CurrentGame.h"

//********************************************************************************
CStunEffect::CStunEffect(
	CWidget *pSetWidget,            //(in)   Should be a room widget.
	const UINT stunX,
	const UINT stunY,
	const UINT stunDuration)    //(in)   stun duration
	: CAnimatedTileEffect(pSetWidget,CCoord(stunX, stunY),
			(UINT)-1,TI_STUN1,false,ESTUN)
	, fading(false)
{
	ASSERT(pSetWidget);
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	CDbRoom *pRoom = this->pRoomWidget->GetRoom();
	ASSERT(pRoom);
	ASSERT(!pRoom || pRoom->GetCurrentGame());

	UINT turnDuration = stunDuration;
	ASSERT(turnDuration > 0);
	if (turnDuration == 1) {
		//effect lasts for turn that just passed only -- show as temporary effect that expires next turn
		this->dwDuration = 1000;
		this->fading = true;
		++turnDuration;
	}

	this->wExpiresOnTurn = pRoom ? pRoom->GetCurrentGame()->wTurnNo + (turnDuration-1) : 0;
}

//********************************************************************************
bool CStunEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	const UINT wTurnNow = this->pRoomWidget->GetRoom()->GetCurrentGame()->wTurnNo;
	if (wTurnNow >= this->wExpiresOnTurn)
		return false;

	if (this->fading)
		this->nOpacity = static_cast<Uint8>(255.0 * GetRemainingFraction());

	static const Uint32 framerate = 125; //ms

	static const UINT NUM_FRAMES = 4;
	static const UINT FRAME[NUM_FRAMES] = { TI_STUN1, TI_STUN2, TI_STUN3, TI_STUN4 };
	const UINT frame = (dwTimeElapsed / framerate) % NUM_FRAMES;
	this->wTileNo = FRAME[frame];

	ASSERT(frame < NUM_FRAMES);

	return true;
}