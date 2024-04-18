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
#include "Character.h"
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
bool CSwordsman::CanBlowSquadHorn() const
{
	bool active;
	if (HasBehavior(PB_UseSquadHorn, active)) {
		return active;
	}

	return CanGetItems();
}

//*****************************************************************************
bool CSwordsman::CanBlowSoldierHorn() const
{
	bool active;
	if (HasBehavior(PB_UseSoldierHorn, active)) {
		return active;
	}

	return bIsMonsterTarget(this->wAppearance) || this->bCanGetItems;
}

//*****************************************************************************
bool CSwordsman::CanBumpActivateOrb() const
{
	bool active;
	if (HasBehavior(PB_BumpActivateOrb, active)) {
		return active;
	}

	return (bIsHuman(this->wAppearance) && !bEntityHasSword(this->wAppearance)) ||
		this->bCanGetItems;
}

//*****************************************************************************
bool CSwordsman::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	bool active;
	if (HasBehavior(PB_DropTrapdoors, active)) {
		return active;
	} else if (HasBehavior(PB_DropTrapdoorsArmed, active)) {
		return active && (bIsThinIce(oTile) || HasHeavyWeapon());
	}

	if (this->wAppearance == M_CONSTRUCT)
		return true;

	if (CanGetItems()) {
		if (bIsThinIce(oTile) && !bIsEntityFlying(this->wAppearance))
			return true;

		if (HasHeavyWeapon())
			return true;
	}

	return false;
}

//*****************************************************************************
bool CSwordsman::CanFluffTrack() const
{
	bool active;
	if (HasBehavior(PB_PuffTarget, active)) {
		return active;
	}

	return bCanFluffTrack(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanGetItems() const
//Returns: whether the player is able to collect in-room items
{
	return bIsSmitemaster(this->wAppearance) || this->bCanGetItems;
}

//*****************************************************************************
bool CSwordsman::CanLightFuses() const
//Returns: whether the player is able to light fuses
{
	bool active;
	if (HasBehavior(PB_LightFuses, active)) {
		return active;
	}

	return CanGetItems();
}

//*****************************************************************************
bool CSwordsman::CanStepOnMonsters() const
//Returns: true if player in current role can step on (and kill) other monsters
{
	bool active;
	if (HasBehavior(PB_StepKill, active)) {
		return active;
	}

	return bCanEntityStepOnMonsters(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanDaggerStep(const CMonster* pMonster, const bool bIgnoreSheath) const
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
	switch(pMonster->wType)
	{
		case M_CITIZEN: case M_ARCHITECT:
			return false;
		case M_CLONE: case M_TEMPORALCLONE: {
			const CPlayerDouble* pDouble = DYN_CAST(const CPlayerDouble*, const CMonster*, pMonster);
			if (pDouble) {
				return pDouble->IsVulnerableToWeapon(WT_Dagger);
			}
		}
		case M_CHARACTER: {
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			if (!pCharacter || !pCharacter->IsVisible()) {
				return true; // You can always step on something that isn't there
			} 
			return !pCharacter->IsImmuneToWeapon(WT_Dagger);
		}
		default:
			return true;
	}
}

//*****************************************************************************
bool CSwordsman::CanDrinkClonePotion() const
{
	bool active;
	if (HasBehavior(PB_UseClonePotion, active)) {
		return active;
	}

	return CanGetItems();
}

//*****************************************************************************
bool CSwordsman::CanDrinkDecoyPotion() const
{
	bool active;
	if (HasBehavior(PB_UseDecoyPotion, active)) {
		return active;
	}

	return bIsSmitemaster(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanDrinkInvisibilityPotion() const
{
	bool active;
	if (HasBehavior(PB_UseInvisibilityPotion, active)) {
		return active;
	}

	return CanGetItems();
}

//*****************************************************************************
bool CSwordsman::CanDrinkMimicPotion() const
{
	bool active;
	if (HasBehavior(PB_UseMimicPotion, active)) {
		return active;
	}

	return bIsSmitemaster(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanDrinkSpeedPotion() const
{
	bool active;
	if (HasBehavior(PB_UseSpeedPotion, active)) {
		return active;
	}

	return CanGetItems();
}

//*****************************************************************************
bool CSwordsman::CanDrinkPotionType(const UINT wTile) const
{
	switch (wTile) {
		case T_POTION_K: return CanDrinkMimicPotion();
		case T_POTION_I: return CanDrinkInvisibilityPotion();
		case T_POTION_D: return CanDrinkDecoyPotion();
		case T_POTION_SP: return CanDrinkSpeedPotion();
		case T_POTION_C: return CanDrinkClonePotion();
		default: return false;
	}
}

//*****************************************************************************
bool CSwordsman::CanEnterTunnel() const
{
	bool active;
	if (HasBehavior(PB_UseTunnels, active)) {
		return active;
	}

	return bIsMonsterTarget(this->wAppearance) || this->bCanGetItems;
}

//*****************************************************************************
bool CSwordsman::CanHaveWeapon() const
//Returns: Does the player's appearance or behaviors allow them to have a weapon
{
	bool active;
	if (HasBehavior(PB_HasWeapon, active)) {
		return active;
	}

	return bEntityHasSword(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::CanPushOntoOTile(const UINT wTile) const
{
	if (bIsFloor(wTile) || bIsOpenDoor(wTile) || bIsPlatform(wTile))
		return true;

	bool bSafeOnly;
	HasBehavior(PB_FatalPushImmune, bSafeOnly);

	if (!bSafeOnly) {
		// We don't care that these tiles might be deadly
		return bIsPit(wTile) || bIsWater(wTile);
	}

	bool bFlying = bIsEntityFlying(this->wAppearance);

	if (bIsPit(wTile) && bFlying)
		return true;

	bool bIsSwimming = bIsEntitySwimming(this->wAppearance);
	if (bIsDeepWater(wTile) && (bIsSwimming || bFlying))
		return true;

	if (bIsShallowWater(wTile) && (CanWadeInShallowWater() || bIsSwimming || bFlying))
		return true;

	return false;
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
bool CSwordsman::FacesMovementDirection() const
{
	bool active;
	if (HasBehavior(PB_FaceMovementDirection, active)) {
		return active;
	}

	return !HasWeapon() || GetActiveWeapon() == WT_Dagger;;
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
	this->behaviorOverrides.clear();
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
MovementType CSwordsman::GetMovementType() const
//Returns: the movement type that best applies to player's current configuration of appearance and water traversal
{
	switch (this->wAppearance)
	{
		case M_SEEP:
			return WALL;
		case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
			return AIR;
		case M_WATERSKIPPER:
			return WATER;
		default:
			return this->GetWaterTraversalState(this->wAppearance) == WTrv_NoEntry
				? GROUND
				: GROUND_AND_SHALLOW_WATER;
	}
}

//*****************************************************************************
bool CSwordsman::HasBehavior(
//Returns: If the given behavior is overridden. Output argument describes if it
//appplies to the player.
	const PlayerBehavior& behavior, //(in) behavior to check
	bool& active //(out) does the behavior apply
) const
{
	if (this->behaviorOverrides.count(behavior) == 0) {
		active = false;
		return false;
	}

	PlayerBehaviorState state = this->behaviorOverrides.at(behavior);
	switch (state) {
		case PBS_On: {
			active = true;
			return true;
		}
		case PBS_Off: {
			active = false;
			return true;
		}
		case PBS_Powered: {
			active = bCanGetItems;
			return true;
		}
		case PBS_Unpowered: {
			active = !bCanGetItems;
			return true;
		}
		default: {
			active = false;
			return false;
		}
	}
}

//*****************************************************************************
bool CSwordsman::HasDamageImmunity(const WeaponType& weaponType, bool& active) const
{
	PlayerBehavior behavior;
	switch (weaponType) {
		case WT_Sword: behavior = PB_SwordDamageImmune; break;
		case WT_Pickaxe: behavior = PB_PickaxeDamageImmune; break;
		case WT_Spear: behavior = PB_SpearDamageImmune; break;
		case WT_Dagger: behavior = PB_DaggerDamageImmune; break;
		case WT_Caber: behavior = PB_CaberDamageImmune; break;
		case WT_FloorSpikes: behavior = PB_FloorSpikeImmune; break;
		case WT_Firetrap: behavior = PB_FiretrapImmune; break;
		case WT_HotTile: behavior = PB_HotTileImmune; break;
		default: behavior = PB_SwordDamageImmune; break;
	}

	return HasBehavior(behavior, active);
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
bool CSwordsman::IsVulnerableToAdder() const
{
	bool active;
	HasBehavior(PB_AdderImmune, active);
	return !active; //No immunity = edible
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToExplosion() const
{
	bool active;
	HasBehavior(PB_ExplosionImmune, active);
	return !active; //No immunity = it hurts
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToFluff() const
{
	bool active;
	if (HasBehavior(PB_PuffImmune, active)) {
		return !active; //No immunity = it hurts
	}

	return bCanFluffKill(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToHeat() const
{
	bool active;
	if (HasBehavior(PB_HotTileImmune, active)) {
		return !active; //No immunity = it hurts
	}

	return bIsEntityTypeVulnerableToHeat(this->wAppearance);
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToWeapon(const WeaponType weaponType) const
//Returns: true if player in current role is vulnerable to sword hits
{
	bool active;
	if (HasDamageImmunity(weaponType, active)) {
		return !active; //No immunity = it hurts
	}

	const bool bIsStabbable = this->IsStabbable();

	switch (weaponType)
	{
		case WT_HotTile:
			return bIsEntityTypeVulnerableToHeat(this->wAppearance);
		case WT_Firetrap:
			return this->wAppearance != M_FEGUNDO;
		case WT_FloorSpikes:
			return !bIsEntityFlying(this->wAppearance) && bIsStabbable;
		default:
			return bIsStabbable;
	}
}

//*****************************************************************************
bool CSwordsman::IsVulnerableToBodyAttack() const
//Return whether the player can be stepped on by monsters
{
	bool active;
	if (HasBehavior(PB_BodyAttackImmune, active)) {
		return !active; //No immunity = can be attacked
	}

	return bIsVulnerableToBodyAttack(this->wAppearance);
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
void CSwordsman::SetBehavior(const PlayerBehavior behavior, const PlayerBehaviorState state)
{
	if (state == PBS_Default) {
		this->behaviorOverrides.erase(behavior);
	} else {
		this->behaviorOverrides[behavior] = state;
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
	return CanHaveWeapon() &&
			!(this->bNoWeapon || this->bWeaponSheathed || this->bWeaponOff || this->bIsHiding);
}

bool CSwordsman::HasWeaponType(WeaponType type) const
{
	switch (type) {
		case WT_Off:
			return (this->bNoWeapon || this->bWeaponOff);
		case WT_On:
			return !this->bWeaponOff;
		default:
			return (type == this->localRoomWeaponType);
	}
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
