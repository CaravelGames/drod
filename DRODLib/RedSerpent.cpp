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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "RedSerpent.h"

#include "GameConstants.h"
#include "CurrentGame.h"
#include "CueEvents.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CRedSerpent::CRedSerpent(CCurrentGame *pSetCurrentGame)
	: CSerpent(M_SERPENT, pSetCurrentGame)
{
}

//*****************************************************************************************
void CRedSerpent::Process(
//Process a red serpent for movement:
//If it can't move forward, then it shrinks by one square.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	int dxFirst, dyFirst, dx, dy;
	if (!GetSerpentMovement(wX, wY, dxFirst, dyFirst, dx, dy))
	{
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
		return;
	}

	bool bFluffPoison = false;
	const UINT wDestX = this->wX + dx;
	const UINT wDestY = this->wY + dy;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	CMonster *pMonster = room.GetMonsterAtSquare(wDestX, wDestY);
	if (pMonster && pMonster->wType == M_FLUFFBABY)
	{
		room.KillMonster(pMonster,CueEvents,false,this);
		bFluffPoison = true;
	}

	// Move according to direction chosen.
	const bool bNPCBeethroDied = NPCBeethroDieCheck(this->wX + dx, this->wY + dy, CueEvents);
	LengthenHead(dx,dy,nGetOX(wO),nGetOY(wO), CueEvents);
	if (ShortenTail(CueEvents))
	{
		ASSERT(!bFluffPoison);
		return;  //snake died
	}
	if (bFluffPoison)
	{
		room.ProcessPuffAttack(CueEvents,wDestX,wDestY);
		if (!bAlive)
			return;
	}

	if (!bNPCBeethroDied && IsOnSwordsman())
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->SetDyingEntity(&pGame->swordsman, this);
		CueEvents.Add(CID_MonsterKilledPlayer, this);
	}
}
