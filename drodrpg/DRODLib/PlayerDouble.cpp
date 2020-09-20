// $Id: PlayerDouble.cpp 9297 2008-10-29 02:31:31Z mrimer $

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
#include "Swordsman.h"

//
//Public methods.
//

//*****************************************************************************
CPlayerDouble::CPlayerDouble(
//Constructor.
//
//Params:
	const UINT wSetType,    //(in) type of derived monster
	CCurrentGame *pSetCurrentGame,   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	const UINT wSetProcessSequence)  //(in)   [default=100]
	: CMonster(wSetType, pSetCurrentGame, GROUND, wSetProcessSequence)
	, wSwordMovement(NO_ORIENTATION)
	, bSwordSheathed(false)
	, bNoSword(false)
//	, bFrozen(false)
{ }

//*****************************************************************************
bool CPlayerDouble::DoesSquareContainObstacle(
//Override for doubles.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Most of the checks done in base method.
	if (CMonster::DoesSquareContainObstacle(wCol, wRow))
		return true;

	//Can't step on the player.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//*****************************************************************************
bool CPlayerDouble::DoesSquareRemoveSword(const UINT wCol, const UINT wRow) const
//Returns: whether the tile being considered will prevent the double from having a sword
{
	ASSERT(this->pCurrentGame);
	return this->pCurrentGame->DoesTileDisableMetal(wCol,wRow);
}

//*****************************************************************************
bool CPlayerDouble::HasSword() const
//Returns: true when double has unsheathed sword
{
	return !this->bSwordSheathed;
}

//*****************************************************************************
bool CPlayerDouble::IsSwordDisabled() const
//Returns: whether this sword-wielding entity is unable to use their sword
{
	return this->bSwordSheathed; //on oremites
}

//*****************************************************************************
bool CPlayerDouble::IsSafeToStab(const UINT wX, const UINT wY) const
//Returns: true if my sword at (wX, wY) won't stab anything dangerous.
{
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wX,wY))
		return true;

	//Never stab a bomb.
	const UINT wTSquare = room.GetTSquare(wX,wY);
	if (wTSquare == T_BOMB)
		return false;

/*
	//Don't stab a guard or Slayer.
	CMonster *pMonster = room.GetMonsterAtSquare(wX, wY);
	if (pMonster &&
//			(pMonster->wType == M_GUARD || pMonster->wType == M_SLAYER || pMonster->wType == M_CHARACTER) &&
			pMonster != static_cast<const CMonster*>(this)) //impossible to stab self -- ignore this case
		return false;
*/

	return true;
}

//*****************************************************************************
bool CPlayerDouble::MakeSlowTurnIfOpen(const UINT wDesiredO)
//Make a slow turn toward the desired orientation if it's safe.
{
	const UINT wOldO = this->wO;
	if (!MakeSlowTurn(wDesiredO)) return false;  //already facing desired orientation
	if (!IsSafeToStab(GetSwordX(), GetSwordY()))
	{
		this->wO = wOldO; //don't turn
		this->wSwordMovement = NO_ORIENTATION;
		return false;
	} else {
		this->wSwordMovement = CSwordsman::GetSwordMovement(
				this->wO == nNextCO(wOldO) ? CMD_C : CMD_CC, this->wO);
	}
	return true;
}

//*****************************************************************************
bool CPlayerDouble::MakeSlowTurnIfOpenTowards(const UINT wTX, const UINT wTY)
//Make a slow turn toward (wTX, wTY) if it's safe.
{
	const UINT wOldO = this->wO;
	if (!MakeSlowTurnTowards(wTX, wTY)) return false;  //already facing desired orientation
	if (!IsSafeToStab(GetSwordX(), GetSwordY()))
	{
		this->wO = wOldO; //don't turn
		this->wSwordMovement = NO_ORIENTATION;
		return false;
	}
	return true;
}

//*****************************************************************************
bool CPlayerDouble::MakeSlowTurnTowards(const UINT wTX, const UINT wTY)
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
bool CPlayerDouble::SetSwordSheathed()
//Sets and returns whether double's sword gets sheathed on this tile.
{
	ASSERT(this->pCurrentGame);
	return this->bSwordSheathed = DoesSquareRemoveSword(this->wX, this->wY);
}

