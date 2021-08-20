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

//Guard.cpp
//Implementation of CGuard.

#include "Guard.h"
#include "Character.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CGuard::CGuard(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	: CArmedMonster(M_GUARD, pSetCurrentGame,
					GROUND_AND_SHALLOW_WATER, SPD_GUARD) //move after slayer, before monsters
{ }

//*****************************************************************************
bool CGuard::DoesSquareContainObstacle(
//Override for guards -- they can't step on attackable monsters.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Most of the checks done in base method.
	if (CMonster::DoesSquareContainObstacle(wCol, wRow))
		return true;

	//Can't move onto monsters -- even target ones.
	CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wCol, wRow);
	if (pMonster){
		if (!CanDaggerStep(pMonster->wType))
			return true;
		if (pMonster->wType == M_GUARD || pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2 ||
			  pMonster->wType == M_FLUFFBABY)
			return true;
		if (pMonster->wType == M_CHARACTER) {
			CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsInvulnerable()
					|| pCharacter->IsPushableByWeaponAttack()
					|| !this->CanDaggerStep(pCharacter->wType)) // wType instead of GetIdentity() to make it consistent with Player behavior
				return true;
		}

	}

	//Can't step on the player.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow) && this->weaponType != WT_Dagger;
}

//*****************************************************************************
bool CGuard::IsTileObstacle(
//Override for guards.
//
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.  Note each tile# will always be
						//    found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	return (
			//Guards don't like active firetraps
			wLookTileNo == T_FIRETRAP_ON ||
			CArmedMonster::IsTileObstacle(wLookTileNo)
			);
}

//*****************************************************************************
bool CGuard::IsOpenMove(const int dx, const int dy) const
{
	if (!CArmedMonster::IsOpenMove(dx,dy))
		return false;

	UINT wSO = this->wO;
	if (!HasSword() || this->weaponType == WT_Dagger)
		wSO = nGetO(dx,dy); //will end up facing this way after moving

	return IsSafeToStab(this->wX + dx, this->wY + dy, wSO);
}

//*****************************************************************************************
void CGuard::Process(
//Process guard for movement.
//NOTE: Code copied and dumbed-down from slayer behavior.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	this->wSwordMovement = NO_ORIENTATION;

	//Don't move if another monster is already killing player.
	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied)) return;

	UINT wX, wY;
	int dx, dy;
	if (!GetTarget(wX,wY)) 
		return;

	//0. We move straight for the target if equipped with a dagger
	if (this->HasSword() && this->weaponType == WT_Dagger){
		return this->ProcessDaggerMove(wX, wY, CueEvents);
	}

	//1. Can I kill target this turn?  (Do it.)
	const UINT wMySX = GetWeaponX(), wMySY = GetWeaponY();
	const UINT wDistMySwordFromSwordsman = nDist(wMySX, wMySY, wX, wY);
	if (wDistMySwordFromSwordsman == 0){
		return this->AttackTargetUnderWeapon(wX, wY, CueEvents);
	}
	const UINT wMyDistFromSwordsman = nDist(this->wX, this->wY, wX, wY);

	//If I think that I can kill my target during this turn, take the
	//move immediately.  I'm not smart enough to concern myself
	//with whether I have my sword sheathed or not this close to the target.
	if (wDistMySwordFromSwordsman == 1)	
	{
		//See if directly moving to square will work.  (Copied from CMonster::GetBestMove().)
		dx = wX - wMySX;
		dy = wY - wMySY;
		//Use the old IsOpenMove behavior
		const bool bFoundDir = CArmedMonster::IsOpenMove(dx,dy) &&
				IsSafeToStab(this->wX + dx, this->wY + dy, this->wO);
		if (bFoundDir)
		{
			DoubleMove(CueEvents,dx,dy);
			this->wSwordMovement = nGetO(dx,dy);
			if (!HasSword())
				this->wPrevO = this->wO = this->wSwordMovement; //automatically face this direction
			return;
		}
		//See if rotating sword will work.
		if (wMyDistFromSwordsman == 1)
		{
			if (MakeSlowTurnIfOpenTowards(wX,wY))
				return;
		}
	}
	//If I haven't found a killing move (successful or not), and I'm next to a target
	//but without a sword, "run" into it (i.e. point towards it).
	if (!HasSword() && wMyDistFromSwordsman == 1)
	{
		dx = sgn(wX - this->wX);
		dy = sgn(wY - this->wY);
		SetOrientation(dx,dy);
		this->wSwordMovement = nGetO(dx,dy);
		return;
	}

	//2. Not close to the target -- advance using intelligent path finding.
	//Use brained motion to chase the player or a Beethro NPC,
	//but unbrained motion to chase a Stalwart.
	int dxFirst, dyFirst;
	UINT wSX, wSY;
	const bool bSwordsmanInRoom = this->pCurrentGame->GetSwordsman(wSX, wSY);
	if (!bSwordsmanInRoom || wSX != wX || wSY != wY ||
		!GetBrainDirectedMovement(dxFirst, dyFirst, dx, dy, SmartOmniDirection))
		GetBeelineMovementSmart(wX, wY, dxFirst, dyFirst, dx, dy, true);    

	if (dx || dy)
	{
		//If not facing the direction it will move, turn guard that way.
		//Else move guard (in direction it's facing) forward to new destination square.
		if (!HasSword() || !MakeSlowTurnIfOpenTowards(wX, wY))
		{
			//2b. Planning on moving forward.  Make sure I won't stab another guard.
			UINT wSO = this->wO;
			if (!HasSword())
				wSO = nGetO(dx,dy); //will end up facing this way after moving

			if (IsSafeToStab(this->wX + dx, this->wY + dy, wSO))
			{
				DoubleMove(CueEvents, dx, dy);
				this->wSwordMovement = nGetO(dx,dy);
				if (!HasSword())
					this->wPrevO = this->wO = this->wSwordMovement; //automatically face this direction
			}
		}
	} else {
		//Can't move -- just turn toward swordsman.
		MakeSlowTurnIfOpenTowards(wX,wY);
	}
}


//*****************************************************************************************
void CGuard::ProcessDaggerMove(const UINT wTargetX, const UINT wTargetY, CCueEvents& CueEvents)
{
	int dx, dy;
	const UINT wDistFromSwordsman = nDist(this->wX, this->wY, wTargetX, wTargetY);
	if (wDistFromSwordsman == 1){
		int dx = sgn(wTargetX - this->wX);
		int dy = sgn(wTargetY - this->wY);
		UINT wNewOrientation = nGetO(dx, dy);
		if (CArmedMonster::IsOpenMove(dx, dy) && IsSafeToStab(this->wX + dx, this->wY + dy, wNewOrientation))
		{
			DoubleMove(CueEvents, dx, dy);
			SetOrientation(dx, dy);
		}
	}
	else
	{
		UINT wSX, wSY;
		int dxFirst, dyFirst;
		const bool bSwordsmanInRoom = this->pCurrentGame->GetSwordsman(wSX, wSY);
		const bool bIsTargetNotPlayer = !bSwordsmanInRoom || wSX != wTargetX || wSY != wTargetY;
		if (bIsTargetNotPlayer || !GetBrainDirectedMovement(dxFirst, dyFirst, dx, dy, SmartOmniDirection))
		{
			GetBeelineMovementSmart(wTargetX, wTargetY, dxFirst, dyFirst, dx, dy, true);
		}

		UINT wNewOrientation = nGetO(dx, dy);
		if ((dx || dy) && IsSafeToStab(this->wX + dx, this->wY + dy, wNewOrientation))
		{
			DoubleMove(CueEvents, dx, dy);
			this->wSwordMovement = wNewOrientation;
			this->wO = this->wSwordMovement;
		}
		else
		{
			wNewOrientation = nGetO(dxFirst, dyFirst);
			if (this->wO != wNewOrientation && IsSafeToStab(this->wX, this->wY, wNewOrientation) && !CMonster::IsOpenMove(dxFirst, dyFirst)){
				this->wO = wNewOrientation;
				CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
				pGame->ProcessWeaponHit(this->wX + dxFirst, this->wY + dyFirst, CueEvents, this);
			}
		}
	}
}