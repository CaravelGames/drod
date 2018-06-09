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

#include "RockGolem.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
void CRockGolem::Process(
//Process movement turn.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a CCueEvents object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (this->bBroken) return; //can't move

	//If stunned, skip turn
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, DirectOnly))
		return;
 
	//Move to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}

//*****************************************************************************
bool CRockGolem::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	const bool bMoved = this->wX != this->wPrevX || this->wY != this->wPrevY;
	const bool bOnHotTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT;

	if ((!this->bWaitedOnHotFloorLastTurn && this->pCurrentGame->swordsman.bIsHasted) || bMoved){
		this->bWaitedOnHotFloorLastTurn = bOnHotTile && !bMoved;
		return false;
	}

	CCueEvents Ignored;
	if (bOnHotTile)
		if (OnStabbed(Ignored, this->wX, this->wY))
		{
			CueEvents.Add(CID_MonsterBurned, this);
			const_cast<CCurrentGame*>(this->pCurrentGame)->TallyKill();
			return false; //don't remove the monster from the room
		}

	return false;
}

//*****************************************************************************************
bool CRockGolem::OnStabbed(
//When stabbed, rock giant is disabled but doesn't disappear.
//
//Params:
	CCueEvents &CueEvents,  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
	const UINT /*wX*/, const UINT /*wY*/, //(in) unused
	WeaponType /*weaponType*/)
{
	if (this->bBroken)
		return false;  //no further effect

	//The monster becomes inactive, but should not be removed from the room.
	CueEvents.Add(CID_MonsterDiedFromStab, this);

	//When killed over shallow water, rock giants become a stepping stone
	//Delay dealing with it until ProcessWeaponHit
	if (!(this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_SHALLOW_WATER))
	{
		this->bBroken = true;
		this->wO = NO_ORIENTATION; //show disabled graphic
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;

		this->pCurrentGame->pRoom->DecMonsterCount();

		//Since rock giant becomes an obstacle, pathmaps might need updating.
		this->pCurrentGame->pRoom->UpdatePathMapAt(this->wX, this->wY);
		this->pCurrentGame->pRoom->RecalcStationPaths();
	}

	return true;
}
