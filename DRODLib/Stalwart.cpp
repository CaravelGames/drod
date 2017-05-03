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

//Stalwart.cpp
//Implementation of CStalwart.

#include "Stalwart.h"

#include "CurrentGame.h"
#include "DbRooms.h"

CIDSet CStalwart::typesToAttack;

//
//Public methods.
//

//*****************************************************************************************
CStalwart::CStalwart(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame,   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	const MovementType eMovement,   //(in)    [default=GROUND]
	const UINT type) //[default=M_STALWART]
	: CPlayerDouble(type, pSetCurrentGame, eMovement, SPD_STALWART) //move after fegundo, before slayer
{
	//The first time, populate the set of monsters to attack.
	if (CStalwart::typesToAttack.empty())
	{
		//Attack non-friendly monster types.
		static const UINT NUM_TYPES = 29;
		static const UINT types[NUM_TYPES] = {
			M_ROACH, M_QROACH, M_REGG, M_GOBLIN, M_WWING,
			M_EYE, M_TARBABY, M_BRAIN, M_SPIDER, M_SERPENTG,
			M_ROCKGOLEM, M_WATERSKIPPER, M_SKIPPERNEST, M_AUMTLICH, M_SEEP,
			M_SLAYER, M_SLAYER2, M_GUARD, M_MUDBABY, M_GELBABY,
			M_ROCKGIANT, M_TARMOTHER, M_MUDMOTHER, M_GELMOTHER, M_SERPENTB,
			M_SERPENTG, M_SERPENT, M_WUBBA, M_CONSTRUCT
			//not M_GENTRYII?
		};
		for (UINT wI=NUM_TYPES; wI--; )
			CStalwart::typesToAttack += types[wI];
	}
}

//*****************************************************************************************
CStalwart2::CStalwart2(CCurrentGame *pSetCurrentGame, const UINT type) //[default=M_STALWART2]
	: CStalwart(pSetCurrentGame, GROUND_AND_SHALLOW_WATER, type)
{}

//******************************************************************************************
bool CStalwart::DoesSquareContainObstacle(
//Override for stalwart.  (Copied and revised code from CMonster::DoesSquareContainObstacle.)
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	ASSERT(this->pCurrentGame);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow))
		return true;

	//No monsters can ever be stepped on unless you have a dagger
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY){
		if (!CStalwart::typesToAttack.has(pMonster->wType))
			return true;
		else if (!this->CanDaggerStep(pMonster->wType))
			return true;
		if (pMonster->wType == M_STALWART || pMonster->wType == M_STALWART2)
			return true;
	}
	
	//Some layer objects are obstacles.  Check for these.
	if (IsTileObstacle(room.GetOSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetTSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Can't step on any swords.
	if (this->swordsInRoom.Exists(wCol, wRow)) //this set is compiled at beginning of move
		return true;

	//Player can never be stepped on.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//*****************************************************************************
bool CStalwart::IsTileObstacle(
//Override for stalwarts.
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
			//Stalwarts don't like active firetraps
			wLookTileNo == T_FIRETRAP_ON ||
			CArmedMonster::IsTileObstacle(wLookTileNo)
			);
}

//*****************************************************************************
bool CStalwart::IsOpenMove(const int dx, const int dy) const
//Move is only "available" if there is no risk to stabbing the destination's sword square.
{
	if (!CPlayerDouble::IsOpenMove(dx,dy))
		return false;

	UINT wSO = this->wO;
	if (!HasSword() || this->weaponType == WT_Dagger)
		wSO = nGetO(dx,dy); //will end up facing this way after moving

	return IsSafeToStab(this->wX + dx, this->wY + dy, wSO);
}

//*****************************************************************************
bool CStalwart::IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const
//More general than IsOpenMove(dx,dy).
//Move is only "available" if there is no risk to stabbing the destination's sword square.
{
	if (!CPlayerDouble::IsOpenMove(wX, wY, dx, dy))
		return false;

	UINT wSO = this->wO;
	if (!HasSword() || this->weaponType == WT_Dagger)
		wSO = nGetO(dx,dy); //will end up facing this way after moving

	return IsSafeToStab(wX + dx, wY + dy, wSO);
}

//*****************************************************************************
bool CStalwart::IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const
//Returns: true if standing at (wFromX, wFromY) with sword orientation at wSO won't stab anything
//dangerous, or a guard or Slayer.  Otherwise false.
 {
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	ASSERT(room.IsValidColRow(wFromX,wFromY)); //we shouldn't be calling IsSafeToStab
							//when we're considering standing on a tile that isn't valid.

	//Determine where the sword would be.
	const UINT wSX = wFromX + nGetOX(wSO);
	const UINT wSY = wFromY + nGetOY(wSO);

	if (!room.IsValidColRow(wSX,wSY))
		return true; //sword would be out-of-bounds and wouldn't hit anything

 	//If sword is put away on this tile, no stabbing would occur, so this position is always safe.
	if (DoesSquareRemoveWeapon(wFromX,wFromY))
		return true;

	//Don't stab the player.
	if (this->pCurrentGame->IsPlayerAt(wSX, wSY))
		return false;

	//Never stab a bomb.
	const UINT wTSquare = room.GetTSquare(wSX,wSY);
	if (this->weaponType != WT_Staff && bIsExplodingItem(wTSquare))
		return false;

	//Don't stab a friendly monster.
	CMonster *pMonster = room.GetMonsterAtSquare(wSX, wSY);
	if (pMonster && pMonster->IsAlive() && pMonster->IsFriendly() &&
			pMonster != static_cast<const CMonster*>(this)) //impossible to stab self -- ignore this case
		return false;

	return true;
}

//******************************************************************************
bool CStalwart::GetGoal(UINT& wX, UINT& wY)
//Call to query whether the stalwart has a new target
//
//Returns: whether stalwart has a current goal at (wX,wY)
{
	//Check whether there are any monsters to kill.
	const CMonster *pMonster = this->pCurrentGame->pRoom->pFirstMonster;
	while (pMonster)
	{
		if (CStalwart::typesToAttack.has(pMonster->GetIdentity()))
			break;  //There is a monster to kill.
		pMonster = pMonster->pNext;
	}
	if (!pMonster)
		return false; //no monsters to kill -- do nothing

	//Each turn, find the optimal path to the closest monster.
	//Optimization: get all sword coords once for pathmap search.
	this->pCurrentGame->pRoom->GetSwordCoords(this->swordsInRoom, true, false, this);
	this->pathToDest.Clear();
	if (!FindOptimalPathToClosestMonster(this->wX, this->wY, CStalwart::typesToAttack))
		return false;  //no path is available

	wX = this->goal.wX;
	wY = this->goal.wY;
	return true;
}

//*****************************************************************************************
void CStalwart::Process(
//Process this stalwart for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	this->wSwordMovement = NO_ORIENTATION;
	if (this->bFrozen)
	{
		//If frozen, then can't move.  Just rotate to attempt to unfreeze.
		this->wO = nNextCO(this->wO);
		this->wSwordMovement = CSwordsman::GetSwordMovement(CMD_C, this->wO);
		return;
	}

	//Check whether we have a goal
	UINT wGoalX, wGoalY;
	if (!GetGoal(wGoalX, wGoalY))
		return;

	const UINT wDist = this->pathToDest.GetSize();
	ASSERT(wDist); //there must be a current path and valid goal at this point
	ASSERT(this->pCurrentGame->pRoom->IsValidColRow(wGoalX, wGoalY));

	//Consider next step along path to goal.
	UINT wNextX, wNextY;
	if (!this->pathToDest.Top(wNextX, wNextY)) return;

	const int dx = wNextX - this->wX;
	const int dy = wNextY - this->wY;
	ASSERT(dx || dy); //there should always be movement
	
	//0. We move straight for the target if equipped with a dagger
	if (this->HasSword() && this->weaponType == WT_Dagger){
		return this->ProcessDaggerMove(dx, dy, CueEvents);
	}

	//1a. Kill target this turn, if possible.
	UINT wSwordDistToTarget = nDist(GetWeaponX(), GetWeaponY(), wGoalX, wGoalY);
	if (!wSwordDistToTarget)
	{
		//sword is on (probably invulnerable) target already
		return this->AttackTargetUnderWeapon(wGoalX, wGoalY, CueEvents);
	}

	//Check for killing moves first.  Ignore whether I have my sword sheathed or not -
	//this close to the target, all I'm interested in is killing.
	if (wSwordDistToTarget == 1)
	{
		//Is it possible to rotate and stab the target? (Copied from CSlayer::Process().)
		const int dsx = wGoalX - GetWeaponX();
		const int dsy = wGoalY - GetWeaponY();
		const UINT wMyDistFromTarget = nDist(this->wX, this->wY, wGoalX, wGoalY);
		if (wMyDistFromTarget == 1 &&
				(!dsx || !dsy)) //one rotation is sufficient to hit target
		{
			if (MakeSlowTurnIfOpenTowards(wGoalX, wGoalY))
				return;
		}

		//Is it possible to move and stab the target?  Don't use the smart version
		//of IsOpenMove - just concentrate on the direction we're facing
		const bool bFoundDir = CPlayerDouble::IsOpenMove(dsx,dsy) &&
			IsSafeToStab(this->wX + dsx, this->wY + dsy, this->wO);
		if (bFoundDir)
		{
			DoubleMove(CueEvents,dsx,dsy);
			this->wSwordMovement = nGetO(dsx,dsy);
			if (!HasSword() || this->weaponType == WT_Dagger)
				this->wO = this->wSwordMovement; //automatically face this direction
			return;
		}
	}

	//1b. If goal monster is close, turn towards it for both offense and defense.
	if (wDist == 1)
	{
		//If I don't have a sword or I have a dagger, I can immediately turn to face the desired direction.
		if (!HasSword() || this->weaponType == WT_Dagger)
		{
			SetOrientation(dx,dy);
			this->wPrevO = this->wO;
			this->wSwordMovement = nGetO(dx,dy);
		} else {
			//Otherwise, I must turn slowly.
			MakeSlowTurnIfOpenTowards(wNextX, wNextY);
		}
		return;
	}

	//2. If facing in monster's general direction when far away, don't need to turn.
	//   Otherwise, turn towards the monster.
	const UINT wDesiredO = nGetO(dx, dy);
	if (!HasSword() ||
			(wDist > 2 && RotationalDistance(wDesiredO) <= 1) ||
			!MakeSlowTurnIfOpenTowards(wGoalX, wGoalY))
	{
		//3. Planning on moving towards the goal.  Make sure I won't stab something with adverse affect.
		UINT wSO = this->wO;
		if (!HasSword())
			wSO = wDesiredO; //will end up facing this way after moving

		if (IsSafeToStab(this->wX + dx, this->wY + dy, wSO))
		{
			DoubleMove(CueEvents, dx, dy);
			this->wSwordMovement = nGetO(dx,dy);
			if (!HasSword() || this->weaponType == WT_Dagger)
				this->wPrevO = this->wO = this->wSwordMovement; //automatically face this direction

			this->pathToDest.Pop();  //stalwart has now made this move
		}
	}
}


//*****************************************************************************************
void CStalwart::ProcessDaggerMove(const int dx, const int dy, CCueEvents& CueEvents)
{
	UINT wNewOrientation = nGetO(dx, dy);

	if (IsSafeToStab(this->wX + dx, this->wY + dy, wNewOrientation)){
		if (CArmedMonster::IsOpenMove(dx, dy)){
			DoubleMove(CueEvents, dx, dy);
			this->wSwordMovement = wNewOrientation;
			this->wO = this->wSwordMovement;
		} else {
			if (IsSafeToStab(this->wX, this->wY, wNewOrientation) && !CMonster::IsOpenMove(dx, dy)){
				this->wO = wNewOrientation;
				CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
				pGame->ProcessWeaponHit(this->wX + dx, this->wY + dy, CueEvents, this);
			}
		}

		SetOrientation(dx, dy);
	}
}