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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Ghost.cpp
//Implementation of CSeep.

#include "Seep.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
bool CSeep::KillIfOutsideWall(CCueEvents &CueEvents)
//Kill the monster if outside wall.
{
	if (IsOnSwordsman())
		return false; //retain for front end display death sequence

	//If ghost was on a door that opened, kill it.
	const UINT dwTileNo = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	if (!bIsWall(dwTileNo) && !bIsCrumblyWall(dwTileNo) && !bIsDoor(dwTileNo))
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->pRoom->KillMonster(this, CueEvents);
		pGame->TallyKill();
		SetKillInfo(NO_ORIENTATION); //center stab effect
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		return true;
	}
	return false;
}

//*****************************************************************************************
void CSeep::Process(
//Process a Ghost for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (KillIfOutsideWall(CueEvents))
		return;

	//If stunned, skip turn
	if (IsStunned())
		return;

	//If another monster has already got the player, don't move this one there too.
	if (CueEvents.HasOccurred(CID_MonsterKilledPlayer))
		return;

	//Decide where to move to.
	UINT wSX, wSY;
	if (!GetTarget(wSX,wSY))
		return;

	//If next to the target (not a decoy) then jump out and kill him.
	//Note this will kill the ghost too.
	if (abs(static_cast<int>(this->wX - wSX)) <= 1 && abs(static_cast<int>(this->wY - wSY)) <= 1)
	{
		bool bKamikaze = this->pCurrentGame->IsPlayerAt(wSX, wSY);
		if (!bKamikaze)
		{
			CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wSX, wSY);
			if (pMonster && pMonster->IsAttackableTarget())
				bKamikaze = true;
		}

		if (bKamikaze)
		{
			//Move onto target if possible.
			const int dx = wSX - this->wX;
			const int dy = wSY - this->wY;
			if (!(DoesArrowPreventMovement(dx, dy) ||
					this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy)))
			{
				MakeStandardMove(CueEvents, dx, dy);
				SetOrientation(dx, dy);
				return;
			}
		}
	}

	// Get directed movement toward swordsman
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wSX, wSY, dxFirst, dyFirst, dx, dy, SmarterDiagonalOnly))
		return;

	//Move ghost to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}
