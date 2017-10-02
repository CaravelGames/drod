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
 * 1997, 2000, 2001, 2002, 2005, 2012 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "TemporalClone.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//*****************************************************************************************
CTemporalClone::CTemporalClone(
	CCurrentGame *pSetCurrentGame)
	: CMimic(M_TEMPORALCLONE, pSetCurrentGame, SPD_TEMPORALCLONE)
	, wIdentity(M_TEMPORALCLONE)
	, bInvisible(false)
	, bIsTarget(false)
{ }

//*****************************************************************************
bool CTemporalClone::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	if (this->wIdentity == M_CONSTRUCT)
		return true;
	
	if (CanLightFuses()) {
		if (bIsThinIce(oTile))
			return true;

		if (HasSword() && bIsHeavyWeapon(GetWeaponType()))
			return true;
	}

	return false;
}

//*****************************************************************************
bool CTemporalClone::CanLightFuses() const
{
	const CSwordsman& player = this->pCurrentGame->swordsman;

	return bIsSmitemaster(this->wIdentity) || player.bCanGetItems;
}

//*****************************************************************************************
UINT CTemporalClone::GetIdentity() const
//Returns: what the temporal clone looks like
{
	return this->wIdentity;
}

//*****************************************************************************
int CTemporalClone::getNextCommand() const
{
	return this->commands.empty() ? CMD_UNSPECIFIED : this->commands.front();
}

//*****************************************************************************
void CTemporalClone::InputCommands(const vector<int>& commands)
{
	for (vector<int>::const_iterator it=commands.begin();
			it!=commands.end(); ++it) {
		this->commands.push_back(*it);
	}
}

//*****************************************************************************
bool CTemporalClone::IsAttackableTarget() const
//Returns: whether this temporal clone is of a visual type that monsters can
//attack and kill.
{
	return IsMonsterTarget();
}

//*****************************************************************************
bool CTemporalClone::IsFlying() const
//Returns: whether clone is flying, based on appearance
{
	const UINT identity = GetIdentity();
	return bIsEntityFlying(identity);
}

//*****************************************************************************
bool CTemporalClone::IsHiding() const
//Returns: whether the clone is visible
{
	if (this->bInvisible)
		return true;

	//Otherwise, temporal clones are invisible if the player can hide in water
	//and the clone is standing in shallow water
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (IsWading() && player.GetWaterTraversalState(this->wIdentity) == WTrv_CanHide)
		return true;

	return false;
}

//*****************************************************************************
bool CTemporalClone::IsMonsterTarget() const
//Returns: whether the temporal clone is targeted by monsters
{
	return this->bIsTarget; //explicitly marked as a monster target
}

//*****************************************************************************
bool CTemporalClone::IsTarget() const
//Returns: whether this temporal clone in its current state is a target for enemies
{
	return this->bIsTarget;
}

//*****************************************************************************
bool CTemporalClone::OnStabbed(
// Override for Temporal Clone, as some player roles can survive stabs.
	CCueEvents & CueEvents,
	const UINT wX,
	const UINT wY,
	WeaponType weaponType
)
{
	if ((this->wIdentity == M_WUBBA || this->wIdentity == M_FLUFFBABY)
		 && weaponType != WeaponType::WT_Firetrap) {
		// Wubbas and Puffs can only be killed by Firetrap stabs.
		return false;
	}

	if (this->wIdentity == M_FEGUNDO) {
		// Fegundoes are immune to all weapon types.
		return false;
	}

	//Monster dies.
	CueEvents.Add(CID_MonsterDiedFromStab, this);
	return true;
}

//*****************************************************************************
void CTemporalClone::Process(const int /*nLastCommand*/, CCueEvents &CueEvents)
{
	//Disappears when command list is exhausted.
	if (this->commands.empty()) {
		CueEvents.Add(CID_TemporalSplitEnd, new CMoveCoord(this->wX, this->wY, NO_ORIENTATION), true);

		CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
		pGame->pRoom->RemoveMonsterDuringPlayWithoutEffect(this);
		return;
	}

	CDbRoom& room = *(this->pCurrentGame->pRoom);

	bool applying_command = false;

	int dx = 0, dy = 0;
	int command = CMD_WAIT;

	if (!IsStunned()) {
		applying_command = true;
		command = this->commands.front();

		InitiateMimicMovement(command, dx, dy);
	}

	//Check whether taking a step is valid.
	bool bEnteredTunnel = false;
	if (!bIsBumpCommand(command) && (dx || dy)) {
		if (this->bFrozen)
		{
			//Frozen clones can't playback full movements
			command = ConvertToBumpCommand(command);
			applying_command = false;
		} else {
			bEnteredTunnel = this->pCurrentGame->PlayerEnteredTunnel(
					this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY), nGetO(dx, dy), this->wIdentity) &&
				!DoesArrowPreventMovement(this->wX, this->wY, dx, dy);
			if (bEnteredTunnel) {
				//If tunnel cannot be exited, then don't expend a turn.
				UINT destX, destY;
				if (!this->pCurrentGame->TunnelGetExit(this->wX, this->wY, dx, dy, destX, destY, this)) {
					command = ConvertToBumpCommand(command);
					bEnteredTunnel = false;
					//Apply the command anyway:
					//The player can program a command moving into a tunnel without an exit,
					//so let the temporal clone consume the command.
				}
				CueEvents.Add(CID_Tunnel);
			} else {
				if (!IsOpenMove(dx,dy)) {
					command = ConvertToBumpCommand(command);
					applying_command = false;
				}
			}
		}
	}

	ApplyMimicMove(dx, dy, command, nGetO(dx,dy), CueEvents, bEnteredTunnel);

	//Light any fuse stood on.
	if (CanLightFuses())
		room.LightFuseEnd(CueEvents, this->wX, this->wY);

	//Advance to next command when this one was successfully executed.
	if (applying_command)
		this->commands.pop_front();
}

//*****************************************************************************
void CTemporalClone::PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents)
//Monster is pushed in a direction, then, optionally, skips next turn
{
	const CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wPushO = nGetO(dx, dy);
	const bool bEnteredTunnel = this->pCurrentGame->PlayerEnteredTunnel(room.GetOSquare(this->wX, this->wY), wPushO, this->wIdentity)
				&& !DoesArrowPreventMovement(this->wX, this->wY, dx, dy);
	if (bEnteredTunnel)
	{
		UINT destX, destY;
		CueEvents.Add(CID_Tunnel);
		if (this->pCurrentGame->TunnelGetExit(this->wX, this->wY, dx, dy, destX, destY, this)) {
			Move(destX, destY, &CueEvents);
			
			CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
			SetWeaponSheathed();
			this->wSwordMovement = nGetO(dx,dy);
			pGame->ProcessArmedMonsterWeapon(this, CueEvents);

			if (bStun && IsAlive())
				Stun(CueEvents, 2);
		} else {
			if (bStun && IsAlive())
				Stun(CueEvents, 1);
		}
	} else {
		CMimic::PushInDirection(dx, dy, bStun, CueEvents);
	}
}

//*****************************************************************************
bool CTemporalClone::SetWeaponSheathed()
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
void CTemporalClone::SetMovementType()
//Sets the movement type depending on the identity
{
	switch (wIdentity){
	case(M_SEEP) :
		eMovement = WALL;
		break;
	case(M_WATERSKIPPER) :
		eMovement = WATER;
		break;
	case(M_FEGUNDO) :
	case(M_WWING) :
	case(M_FLUFFBABY):
		eMovement = AIR;
		break;
	default:
		eMovement = this->pCurrentGame->swordsman.CanWadeInShallowWater() ? GROUND_AND_SHALLOW_WATER : GROUND;
		break;
	}
}

//*****************************************************************************
//Clones cannot be stunned for more than 1 turn
void CTemporalClone::Stun(CCueEvents &CueEvents, UINT /*val*/) //[default=1]
{
	CMonster::Stun(CueEvents, 1);
}
