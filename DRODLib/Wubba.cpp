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

#include "Wubba.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
void CWubba::Process(
//Process a wubba for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY))
		return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, DirectOnly))
		return;

	//Move to new destination square.
	//Note that it can't move onto, and thus can't kill, the player.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}

//*****************************************************************************
bool CWubba::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	const bool bMoved = this->wX != this->wPrevX || this->wY != this->wPrevY;
	const bool bOnHotTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT;

	if ((!this->bWaitedOnHotFloorLastTurn && this->pCurrentGame->swordsman.bIsHasted) || bMoved){
		this->bWaitedOnHotFloorLastTurn = bOnHotTile && !bMoved;
		return false;
	}

	if (bOnHotTile)
	{
		//Damaged, even though sword hits do not affect the wubba.
		CueEvents.Add(CID_MonsterBurned, this);
		return true;
	}
	return false;
}

//******************************************************************************************
bool CWubba::DoesSquareContainObstacle(
//Override for wubbas.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Most of the checks done in base method.
	if (CMonster::DoesSquareContainObstacle(wCol, wRow))
		return true;

	//No monsters can ever be stepped on except Fluff babies.
	ASSERT(this->pCurrentGame);
	CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
		return true;

	//Can't move onto swordsman.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//******************************************************************************************
bool CWubba::OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/, const UINT /*wY*/, WeaponType weaponType)
{
	//Firetraps kill wubbas.
	if (weaponType == WT_Firetrap)
	{
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		return true;
	}

	//Don't send cue event on stabs repeated more than one turn in a row.
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->wTurnNo >= this->wNextStabTurn)
	  CueEvents.Add(CID_WubbaStabbed, this);
	this->wNextStabTurn = this->pCurrentGame->wTurnNo + 2;

	//Stabs don't kill wubbas.
	return false;
}
