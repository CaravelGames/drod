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

#include "PlayerDouble.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************
CArmedMonster::CArmedMonster(
//Constructor.
//
//Params:
	const UINT wSetType,    //(in) type of derived monster
	CCurrentGame *pSetCurrentGame,   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	const MovementType eMovement,   //(in)    [default=GROUND_AND_SHALLOW_WATER]
	const UINT wSetProcessSequence)  //(in)   [default=100]
	: CMonster(wSetType, pSetCurrentGame, eMovement, wSetProcessSequence)
	, wSwordMovement(NO_ORIENTATION)
	, bWeaponSheathed(false)
	, bNoWeapon(false)
	, weaponType(WT_Sword)
	, bFrozen(false)
{ }

//*****************************************************************************
bool CArmedMonster::CanAttackWithWeaponTowards(int dx, int dy) const
{
	return WeaponCanAttackTowards(GetWeaponType(), this->wO, this->wPrevO, dx, dy, this->wSwordMovement);
}


//*****************************************************************************
void CArmedMonster::AttackTargetUnderWeapon(const UINT wTargetX, const UINT wTargetY, CCueEvents& CueEvents)
{
	//my sword is already on the target, try to stab, crush or push the target depending on the weapon

	if (this->weaponType != WT_Caber)
	{
		this->wSwordMovement = this->wO;
	}

	if (this->weaponType == WT_Dagger){
		const int dx = nGetOX(this->wO);
		const int dy = nGetOY(this->wO);

		if (IsOpenMove(dx, dy)){
			DoubleMove(CueEvents, dx, dy);
		}
	}
	
}


//*****************************************************************************
bool CArmedMonster::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	const bool bOnHotTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT;

	//Damaged by remaining stationary on a hot tile?
	const bool bMoved = this->wX != this->wPrevX || this->wY != this->wPrevY;
	if (bOnHotTile && !bMoved)
	{
		const UINT wIdentity = GetResolvedIdentity();
		//Flying and tarstuff identities are safe from hot tiles.
		if (bIsEntityFlying(wIdentity) || bIsMonsterTarstuff(wIdentity))
			return false;

		if (!this->pCurrentGame->swordsman.bIsHasted || this->bWaitedOnHotFloorLastTurn)
		{
			CCueEvents Ignored;
			if (OnStabbed(Ignored, this->wX, this->wY))
			{
				//Add special cue events here instead of inside OnStabbed.
				CueEvents.Add(CID_MonsterBurned, this);
				return true;
			}
		}
	}
	this->bWaitedOnHotFloorLastTurn = bOnHotTile && !bMoved;
	return false;
}

//*****************************************************************************
bool CArmedMonster::DoesSquareContainObstacle(
//Override for doubles.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Most of the checks done in base method.
	if (CMonster::DoesSquareContainObstacle(wCol, wRow))
		return true;

	//Can't step on the player unless we have a dagger
	return this->pCurrentGame->IsPlayerAt(wCol, wRow) && this->weaponType != WT_Dagger;
}

//*****************************************************************************
bool CArmedMonster::DoesSquareRemoveWeapon(const UINT wCol, const UINT wRow) const
//Returns: whether the tile being considered will prevent the double from having a weapon
{
	ASSERT(this->pCurrentGame->pRoom);

	const UINT wOSquare = this->pCurrentGame->pRoom->GetOSquare(wCol,wRow);
	const UINT wIdentity = GetResolvedIdentity();
	if (wOSquare == T_GOO && bIsMetalWeapon(GetWeaponType()))
		return true;

	if (wOSquare == T_SHALLOW_WATER)
	{
		switch(this->wType)
		{
			case M_CLONE:
			case M_TEMPORALCLONE:
				return this->pCurrentGame->swordsman.GetWaterTraversalState(this->GetIdentity()) == WTrv_CanHide;
			default:
				return bCanEntityHideInShallowWater(wIdentity);
		}
	}

	return false;
}

//*****************************************************************************
bool CArmedMonster::HasSword() const
//Returns: true when double has unsheathed sword
{
	return !(this->bNoWeapon || this->bWeaponSheathed);
}

//*****************************************************************************
bool CArmedMonster::IsTileObstacle(
//Override for doubles.
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
			//All the things a double can step onto.
			!bIsPotion(wLookTileNo) &&
			CMonster::IsTileObstacle(wLookTileNo)
			);
}

//*****************************************************************************
bool CArmedMonster::IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const
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

	//Never stab a bomb.
	const UINT wTSquare = room.GetTSquare(wSX,wSY);
	if (this->weaponType != WT_Staff && bIsExplodingItem(wTSquare))
		return false;

	//Don't stab a guard or Slayer.
	CMonster *pMonster = room.GetMonsterAtSquare(wSX, wSY);
	const UINT type = pMonster ? pMonster->GetIdentity() : M_NONE;
	if ((type == M_GUARD || type == M_SLAYER || type == M_SLAYER2) &&
			pMonster != static_cast<const CMonster*>(this)) //impossible to stab self -- ignore this case
		return false;
	if (bIsSmitemaster(type) || type == M_CLONE || type == M_TEMPORALCLONE || bIsStalwart(type))
		return true;  //Can stab NPCs of Beethro, Gunthro, clone or stalwart type.

	return true;
}

//*****************************************************************************
bool CArmedMonster::MakeSlowTurn(const UINT wDesiredO)
//Make a 45 degree (1/8th rotation) turn toward the desired orientation.
//When facing opposite the desired direction prefer clockwise
//
//Returns: whether a turn was made
{
	ASSERT(this->wO != NO_ORIENTATION);
	ASSERT(wDesiredO != NO_ORIENTATION);

	if (this->wO == wDesiredO)
		return false;

	//Determine whether turning clockwise or counter-clockwise would
	//reach the desired orientation more quickly.
	const UINT wRotDist = RotationalDistanceCO(wDesiredO);
	bool bTurnCW = (wRotDist <= 4);
	this->wO = bTurnCW ? nNextCO(this->wO) : nNextCCO(this->wO);
	this->wSwordMovement = CSwordsman::GetSwordMovement(bTurnCW ? CMD_C : CMD_CC, this->wO);
	return true;
}

//*****************************************************************************
bool CArmedMonster::MakeSlowTurnIfOpen(const UINT wDesiredO)
//Make a slow turn toward the desired orientation if it's safe.
{
	const UINT wOldO = this->wO;
	if (!MakeSlowTurn(wDesiredO))
		return false;  //already facing desired orientation
	if (!IsSafeToStab(this->wX, this->wY, this->wO))
	{
		this->wO = wOldO; //don't turn
		this->wSwordMovement = NO_ORIENTATION;
		return false;
	}

	this->wSwordMovement = CSwordsman::GetSwordMovement(
			this->wO == nNextCO(wOldO) ? CMD_C : CMD_CC, this->wO);
	return true;
}

//*****************************************************************************
bool CArmedMonster::MakeSlowTurnIfOpenTowards(const UINT wTX, const UINT wTY)
//Make a slow turn toward (wTX, wTY) if it's safe.
{
	const UINT wOldO = this->wO;
	if (!MakeSlowTurnTowards(wTX, wTY))
		return false;  //already facing desired orientation
	if (!IsSafeToStab(this->wX, this->wY, this->wO))
	{
		this->wO = wOldO; //don't turn
		this->wSwordMovement = NO_ORIENTATION;
		return false;
	}
	return true;
}

//*****************************************************************************
bool CArmedMonster::MakeSlowTurnTowards(const UINT wTX, const UINT wTY)
//Make a 45 degree (1/8th rotation) turn toward the desired orientation.
//When facing opposite the desired direction, turn in the way that puts the
//sword closest to the target.
//
//Returns: whether a turn was made
{
	ASSERT(this->wO != NO_ORIENTATION);

	const UINT wDesiredO = GetOrientationFacingTarget(wTX, wTY);
	if (this->wO == wDesiredO) return false; //no turn needed
	ASSERT(wDesiredO != NO_ORIENTATION);

	//Determine whether turning clockwise or counter-clockwise would
	//reach the desired orientation more quickly.
	const UINT wRotDist = RotationalDistanceCO(wDesiredO);
	bool bTurnCW;
	if (wRotDist < 4)
		bTurnCW = true;
	else if (wRotDist > 4)
		bTurnCW = false;
	else {
		//Since I'm facing opposite the target, determine which way would be more
		//advantageous to turn, i.e., gets my sword closer to the target.
		bTurnCW = nDist(wTX, wTY, this->wX + nGetOX(nNextCO(this->wO)),
				this->wY + nGetOY(nNextCO(this->wO))) < nDist(wTX, wTY,
				this->wX + nGetOX(nNextCCO(this->wO)), this->wY + nGetOY(nNextCCO(this->wO)));
	}
	this->wO = bTurnCW ? nNextCO(this->wO) : nNextCCO(this->wO);
	this->wSwordMovement = CSwordsman::GetSwordMovement(bTurnCW ? CMD_C : CMD_CC, this->wO);
	return true;
}

//*****************************************************************************
void CArmedMonster::Move(
	//Moves the monster to a new square in the room.
	//
	//Params:
	const UINT wDestX, const UINT wDestY,  //(in)   Destination to move to.
	CCueEvents* pCueEvents) //[default=NULL]
{
	CMonster::Move(wDestX, wDestY, pCueEvents);

	if (!this->bAlive)
	{
		// It means the monster has died but he still needs to do his stab for consistency
		// with being pushed into a killing square
		this->bForceWeaponAttack = true;
	}
}

//*****************************************************************************
bool CArmedMonster::SetWeaponSheathed()
//Sets and returns whether double's sword gets sheathed on this tile.
{
	ASSERT(this->pCurrentGame);
	return this->bWeaponSheathed = DoesSquareRemoveWeapon(this->wX, this->wY);
}

void CArmedMonster::DoubleMove(
	CCueEvents &CueEvents,  //(out)  Add cue events if appropriate.
	const int dx, const int dy)   //(in)   Movement offset.
{
	ASSERT(dx || dy); //there should always be movement

	MakeStandardMove(CueEvents, dx, dy);
}

//*****************************************************************************
// After being pushed, an armed monster should process their sword immediately
void CArmedMonster::PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents)
{
	CMonster::PushInDirection(dx, dy, bStun, CueEvents);

	CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
	SetWeaponSheathed();
	this->wSwordMovement = nGetO(dx,dy);
	pGame->ProcessArmedMonsterWeapon(this, CueEvents);
}

//*****************************************************************************
bool CPlayerDouble::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	if (bIsThinIce(oTile))
		return true;

	if (HasSword() && bIsHeavyWeapon(GetWeaponType()))
		return true;

	return false;
}

