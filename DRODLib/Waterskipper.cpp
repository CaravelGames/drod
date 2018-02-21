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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Waterskipper.cpp
//Implementation of CWaterskipper.

#include "Waterskipper.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include <math.h>

static const UINT MAX_TURNS_STUCK = 1;

//
//Public methods.
//

//*****************************************************************************************
CWaterskipper::CWaterskipper(CCurrentGame *pSetCurrentGame)
	: CMonster(M_WATERSKIPPER, pSetCurrentGame, WATER)
{}

//*****************************************************************************************
void CWaterskipper::Process(
//Process a waterskipper for movement.
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
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

	//If next to the target then jump out and kill it.
	//The waterskipper is not killed, but is largely immobile out of water.
	if (abs(static_cast<int>(this->wX - wSX)) <= 1 && abs(static_cast<int>(this->wY - wSY)) <= 1)
	{
		bool bAttack = this->pCurrentGame->IsPlayerAt(wSX, wSY);
		if (!bAttack)
		{
			CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wSX, wSY);
			if (pMonster && pMonster->IsAttackableTarget())
				bAttack = true;
		}

		if (bAttack)
		{
			//Move onto target if possible.
			const int dx = wSX - this->wX;
			const int dy = wSY - this->wY;
			const UINT wOSquare = this->pCurrentGame->pRoom->GetOSquare(wSX,wSY);
			if (!(bIsWall(wOSquare) || bIsCrumblyWall(wOSquare) || bIsDoor(wOSquare) ||
					DoesArrowPreventMovement(dx, dy) ||
					this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy)))
			{
				MakeStandardMove(CueEvents, dx, dy);
				SetOrientation(dx, dy);
				return;
			}
		}
	}

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wSX, wSY, dxFirst, dyFirst, dx, dy))
		return;

	//Move roach to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}
