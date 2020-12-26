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

#include "Clone.h"
#include "Swordsman.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CClone::CClone(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	: CPlayerDouble(M_CLONE, pSetCurrentGame)
	, wCreationIndex(CLONE_NO_CREATION_INDEX)
{ }

//*****************************************************************************
bool CClone::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	if (GetIdentity() == M_CONSTRUCT)
		return true;

	const bool bCanDropTrapdoor = this->pCurrentGame ?
		this->pCurrentGame->swordsman.CanLightFuses() : true;

	if (bCanDropTrapdoor) {
		if (bIsThinIce(oTile))
			return true;
		
		if (HasSword() && bIsHeavyWeapon(GetWeaponType()))
			return true;
	}

	return false;
}

//*****************************************************************************************
UINT CClone::GetIdentity() const
//Returns: what the clone looks like
{
	if (!this->pCurrentGame)
		return M_CLONE;
	const CSwordsman& player = this->pCurrentGame->swordsman;
	switch (player.wAppearance)
	{
		case M_BEETHRO: case M_NONE:
		case M_TEMPORALCLONE:
			return M_CLONE;
		case M_GUNTHRO:
			return M_GUNTHRO;
		case M_BEETHRO_IN_DISGUISE:
			return M_GUARD;
		default: return player.wAppearance;
	}
}

//*****************************************************************************
bool CClone::IsFlying() const
//Returns: whether clone is flying, based on player appearance
{
	const UINT identity = GetIdentity();
	return bIsEntityFlying(identity);
}

//*****************************************************************************
bool CClone::IsMonsterTarget() const
//Returns: whether the clone is targeted by monsters
{
	//By default, clones are targets
	if (!this->pCurrentGame)
		return true;
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.wAppearance == M_NONE)
		return true;

	//Otherwise, clones are only targets if the player is
	return player.IsTarget();
}

//*****************************************************************************
bool CClone::IsHiding() const
//Returns: whether the clone is visible
{
	//By default, clones aren't hiding/invisible
	if (!this->pCurrentGame)
		return false;

	//Clones are invisible if the player has drunk an Invisibility Potion
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.bIsInvisible)
		return true;

	//Otherwise, clones are invisible if the player can hide in water
	//and the clone is standing in shallow water
	if (IsWading() && player.GetWaterTraversalState() == WTrv_CanHide)
		return true;

	return false;
}

//*****************************************************************************************
void CClone::Process(
//Process a clone for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Light any fuse stood on.
	if (this->pCurrentGame->swordsman.CanLightFuses())
		this->pCurrentGame->pRoom->LightFuseEnd(CueEvents, this->wX, this->wY);
}

//*****************************************************************************
bool CClone::SetWeaponSheathed()
//Sets and returns whether a clone's sword is sheathed.
//Clone's sword state should be synched to the player's general sword state.
{
	if (!this->pCurrentGame)
		return this->bWeaponSheathed;

	if (CPlayerDouble::SetWeaponSheathed())
		return true;
	//If player or player's identity type is marked to not have a sword, then clones do not either.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (!bEntityHasSword(GetIdentity()) || player.bWeaponOff || player.bNoWeapon)
	{
		this->bWeaponSheathed = true;
		return true;
	}
	return false;
}

//*****************************************************************************
//Clones cannot be stunned for more than 1 turn
void CClone::Stun(CCueEvents &CueEvents, UINT /*val*/) //[default=1]
{
	CMonster::Stun(CueEvents, 1);
}


//*****************************************************************************
//Calculated the index of this clone in the room, which is used to maintain their processing sequence
//regardless of how they were switched. Index is the highest index of another clone in the room + 1,
//or last clone's index +1, whichever is higher
void CClone::CalculateCreationIndex(CDbRoom* pRoom)
{
	if (this->wCreationIndex != CLONE_NO_CREATION_INDEX) // Already calculated it once
		return;

	CMonster* pMonster = pRoom->pFirstMonster;

	UINT index = 0;
	while (pMonster) {
		if (pMonster == this)
			break;

		if (pMonster->wType == M_CLONE) {
			CClone *pClone = DYN_CAST(CClone*, CMonster*, pMonster);
			index = max(index, pClone->wCreationIndex);
		}
		else if (pMonster->GetProcessSequence() > this->GetProcessSequence())
			ASSERT(!"Calculating clone's creation index when the clone is not in the room is invalid.");

		pMonster = pMonster->pNext;
	}
	this->wCreationIndex = max(pRoom->wLastCloneIndex, index) + 1;
}