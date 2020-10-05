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

//Swordsman.cpp.
//Implementation of CSwordsman.

#include "Swordsman.h"
#include "Weapons.h"
#include "TileConstants.h"
#include <BackEndLib/Assert.h>

//
// Public methods
//

//*****************************************************************************
bool CSwordsman::CanAttackTowards(int dx, int dy) const
{
	return WeaponCanAttackTowards(GetActiveWeapon(), this->wO, this->wPrevO, dx, dy, this->wSwordMovement);
}

//*****************************************************************************
bool CSwordsman::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	if (this->wAppearance == M_CONSTRUCT)
		return true;

	if (CanLightFuses()) {
		if (bIsThinIce(oTile) && !bIsEntityFlying(this->wAppearance))
			return true;

		if (HasHeavyWeapon())
			return true;
	}

	return false;
}

//*****************************************************************************
bool CSwordsman::CanLightFuses() const
//Returns: whether the player is able to light fuses
{
	return bIsSmitemaster(this->wAppearance) || this->bCanGetItems;
}

//*****************************************************************************
bool CSwordsman::CanStepOnMonsters() const
//Returns: true if player in current role can step on (and kill) other monsters
{
	return bCanEntityStepOnMonsters(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanDaggerStep(const UINT wMonsterType, const bool bIgnoreSheath) const
//Returns: true if player is capable of killing the target monster with a "dagger step"
{
	//You can't "dagger step" without a dagger
	//Don't use HasSword() because we need to check for
	//  OTile-specific sheathing separately
	if (GetActiveWeapon() != WT_Dagger || !bEntityHasSword(this->wAppearance) ||
			this->bNoWeapon || this->bWeaponOff)
		return false;

	//Only check for Oremites/Shallow Water sheathing before movement
	if (!bIgnoreSheath && (this->bWeaponSheathed || this->bIsHiding))
		return false;
		
	//Citizens are the only entities that can be stepped on
	//  when they are invulnerable to weapons
	switch(wMonsterType)
	{
		case M_CITIZEN: case M_ARCHITECT:
			return false;
		default:
			return true;
	}
}

//*****************************************************************************
bool CSwordsman::CanWadeInShallowWater() const
//Returns: true if current player role can wade in shallow water
{
	return bIsEntitySwimming(this->wAppearance) ||
			bIsEntityFlying(this->wAppearance) ||
			this->GetWaterTraversalState() >= WTrv_CanWade;
}

//*****************************************************************************
void CSwordsman::Clear()
{
	CEntity::Clear();

	this->nLastCommand = CMD_UNSPECIFIED;
	this->wSwordX = this->wSwordY = 0;
	this->wAppearance = this->wIdentity = M_BEETHRO;
	this->bWeaponOff = false;
	this->wWaterTraversal = WTrv_AsPlayerRole;
	this->bHasTeleported = false;
	EquipWeapon(WT_Sword);

	ResetStats();
}

//*****************************************************************************
UINT CSwordsman::GetWaterTraversalState(UINT wRole) const
//Returns: the value of wWaterTraversal, with WTrv_AsPlayerRole converted as appropriate
{
	//As default, use player's identity
	if (wRole == M_NONE)
		wRole = this->wAppearance;

	//Seep, Flying and swimming roles can never wade or hide
	if (bIsEntityFlying(wRole) || bIsEntitySwimming(wRole) || wRole == M_SEEP)
		return WTrv_NoEntry;

	UINT wWT = this->wWaterTraversal;
	if (wWT == WTrv_AsPlayerRole)
	{
		if (bCanEntityHideInShallowWater(wRole))
			wWT = WTrv_CanHide;
		else if (bCanEntityWadeInShallowWater(wRole))
			wWT = WTrv_CanWade;
		else
			wWT = WTrv_NoEntry;
	}

	if (this->bCanGetItems && wWT != WTrv_CanHide)
	{
		//Humans and Goblins gain Hiding capability from Power Tokens
		//All other land-based monsters gain Wading capability
		if (bIsHuman(wRole) || wRole == M_GOBLIN || wRole == M_GOBLINKING)
			wWT = WTrv_CanHide;
		else
			wWT = WTrv_CanWade;
	}

	return wWT;
}

//*****************************************************************************
bool CSwordsman::IsAt(UINT wX, UINT wY) const
{
	return IsInRoom() && wX == this->wX && wY == this->wY;
}

//*****************************************************************************
bool CSwordsman::IsInRoom() const
//Returns: whether player is controlling a character in the game room
{
	//'none' indicates player is not controlling any character
	return this->wAppearance != M_NONE;
}

//*****************************************************************************
bool CSwordsman::IsStabbable() const
//Returns: true if player in current role is vulnerable to sword hits
{
	//Human roles can't step on monsters.
	//Exception: Slayer
	switch (this->wAppearance)
	{
		case M_WUBBA:
		case M_FLUFFBABY:
		case M_FEGUNDO:
		case M_NONE:
			return false;
		default:
			//All other (monster) roles are vulnerable.
			return true;
	}
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToWeapon(const WeaponType weaponType) const
//Returns: true if player in current role is vulnerable to sword hits
{
	const bool bIsStabbable = this->IsStabbable();

	switch (weaponType)
	{
		case WT_Firetrap:
			return this->wAppearance != M_FEGUNDO;
		case WT_FloorSpikes:
			return !bIsEntityFlying(this->wAppearance) && this->IsStabbable();
		default:
			return this->IsStabbable();
	}
}

//*****************************************************************************
bool CSwordsman::IsTarget() const
//Returns: whether player is a target to monsters
{
	if (!IsInRoom()) return false;
	if (this->wStealth == Stealth_On) return false;
	if (this->wStealth == Stealth_Off) return true;

	return bIsMonsterTarget(this->wAppearance) || //other types that monsters attack
			this->bIsTarget; //explicitly marked as a monster target
}

//*****************************************************************************
bool CSwordsman::IsWeaponAt(UINT wX, UINT wY) const
{
	return HasWeapon() && wX == this->wSwordX && wY == this->wSwordY;
}

//*****************************************************************************
bool CSwordsman::Move(
//Move player to new location
//
//Returns: whether player was moved
//
//Params:
	const UINT wSetX, const UINT wSetY) //(in) position to move to
{
	const bool bMoved = !(this->wX == wSetX && this->wY == wSetY);

	//Set new swordsman coords.
	if (this->wIdentity == M_NONE)
	{
		this->wPrevX = wSetX;
		this->wPrevY = wSetY;
	} else {
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
	}
	this->wPrevO = this->wO;
	this->wX = wSetX;
	this->wY = wSetY;

	SetSwordCoords();

	return bMoved;
}

//*****************************************************************************
void CSwordsman::ResetStats()
//Reset player stats.
{
	this->wPlacingDoubleType = 0;
	this->bIsDying = false;
	this->bIsInvisible = false;
	this->bIsHiding = false;
	this->bIsHasted = false;
	this->bFrozen = false;
	this->bNoWeapon = this->bWeaponSheathed = this->bIsTarget = this->bCanGetItems = false;
	this->wStealth = Stealth_Default;
	this->bHasTeleported = false;
}

//*****************************************************************************
void CSwordsman::RotateClockwise()
//Rotate player clockwise.
//Keeps track of previous orientation.
{
	this->wPrevX = this->wX;
	this->wPrevY = this->wY;
	this->wPrevO = this->wO;
	this->wO = nNextCO(this->wO);
	ASSERT(IsValidOrientation(this->wO));

	SetSwordCoords();
}

//*****************************************************************************
void CSwordsman::RotateCounterClockwise()
//Rotate player counter-clockwise.
//Keeps track of previous orientation.
{
	this->wPrevX = this->wX;
	this->wPrevY = this->wY;
	this->wPrevO = this->wO;
	this->wO = nNextCCO(this->wO);
	ASSERT(IsValidOrientation(this->wO));

	SetSwordCoords();
}

//*****************************************************************************
void CSwordsman::SetOrientation(
//Set player's orientation
//
//Returns: whether player was moved
//
//Params:
	const UINT wO, //(in) new orientation
	const bool updatePrevO)
{
	ASSERT(IsValidOrientation(wO));
	if (updatePrevO)
		this->wPrevO = wO;
	this->wO = wO;

	SetSwordCoords();
}

//*****************************************************************************
void CSwordsman::EquipWeapon(const UINT type)
{
	switch (type) {
		case (UINT)WT_Off:
			this->bWeaponOff = true;
		break;
		case (UINT)WT_On:
			this->bWeaponOff = false; //keeps previous weapon type
		break;
		default:
			this->bWeaponOff = false;
			SetWeaponType(type);
		break;
	}
}

//*****************************************************************************
void CSwordsman::SetWeaponType(const UINT type, const bool bPersist)
{
	if (type < WT_NumWeapons) {
		this->localRoomWeaponType = (WeaponType)type;
		if (bPersist) {
			this->weaponType = (WeaponType)type;
		}
	}
}

//*****************************************************************************
bool CSwordsman::HasHeavyWeapon() const
{
	return HasWeapon() && GetActiveWeapon() != WT_Dagger;
}

bool CSwordsman::HasMetalWeapon() const
{
	return bIsMetalWeapon(GetActiveWeapon());
}

bool CSwordsman::HasWeapon() const
{
	return bEntityHasSword(this->wAppearance) &&
			!(this->bNoWeapon || this->bWeaponSheathed || this->bWeaponOff || this->bIsHiding);
}

//*****************************************************************************
WeaponType CSwordsman::GetActiveWeapon() const
{
	return this->localRoomWeaponType;
}

//*****************************************************************************
UINT CSwordsman::GetSwordMovement(
//Returns: movement sword made to
//
//Params:
	const int nCommand, const UINT wO)     //(in)   Game command.
{
	switch (nCommand)
	{
		case CMD_C: //sword moved orthogonal to direction it's now facing
			switch (wO)
			{
				case NW: return NE;
				case N: return E;
				case NE: return SE;
				case W: return N;
				case E: return S;
				case SW: return NW;
				case S: return W;
				case SE: return SW;
			}
			break;
		case CMD_CC:
			switch (wO)
			{
				case NW: return SW;
				case N: return W;
				case NE: return NW;
				case W: return S;
				case E: return N;
				case SW: return SE;
				case S: return E;
				case SE: return NE;
			}
			break;
		case CMD_NW: case CMD_BUMP_NW: return NW;
		case CMD_N:  case CMD_BUMP_N:  return N;
		case CMD_NE: case CMD_BUMP_NE: return NE;
		case CMD_W:  case CMD_BUMP_W:  return W;
		case CMD_E:  case CMD_BUMP_E:  return E;
		case CMD_SW: case CMD_BUMP_SW: return SW;
		case CMD_S:  case CMD_BUMP_S:  return S;
		case CMD_SE: case CMD_BUMP_SE: return SE;
	}
	return NO_ORIENTATION;
}

//
// Private methods
//

//*****************************************************************************
void CSwordsman::SetSwordCoords()
//Set sword coordinates.
{
	this->wSwordX = this->wX + nGetOX(this->wO);
	this->wSwordY = this->wY + nGetOY(this->wO);
}
