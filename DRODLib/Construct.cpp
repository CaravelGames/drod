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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Construct.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//*****************************************************************************
bool CConstruct::CanDropTrapdoor(const UINT oTile) const {
	return bIsFallingTile(oTile);
}

//*****************************************************************************
bool CConstruct::CanPushObjects() const {
	return true;
}

//*****************************************************************************
bool CConstruct::IsTileObstacle(
//Override for constructs.
//
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.  Note each tile# will always be
						//    found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	return (wLookTileNo == T_GOO || CMonster::IsTileObstacle(wLookTileNo));
}

//******************************************************************************************
bool CConstruct::DoesSquareContainObstacle(
//Override for constructs.  Parts copied from CMonster::DoesSquareContainObstacle.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow)) return true;

	//Check o-square for obstacle.
	UINT wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.

		if (!(bIsTunnel(wLookTileNo) && CanMoveOntoTunnelAt(wCol,wRow)))
			//No special handling was performed.  Treat it as an obstacle.
			return true;
	}

	//Check t-square for obstacle.
	wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		if (bIsTLayerCoveringItem(wLookTileNo))
		{
			//item is not an obstacle if it can be pushed away
			const int dx = (int)wCol - (int)this->wX;
			const int dy = (int)wRow - (int)this->wY;
			if (!room.CanPushTo(wCol, wRow, wCol + dx, wRow + dy))
				return true;
		} else {
			return true;
		}
	}

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Can only move onto attackable monsters.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && !pMonster->IsAttackableTarget() && pMonster->wType != M_FLUFFBABY){
		const int dx = (int)wCol - (int)this->wX;
		const int dy = (int)wRow - (int)this->wY;
		if (!pMonster->IsPushableByBody() || !room.CanPushMonster(pMonster, wCol, wRow, wCol + dx, wRow + dy)){
			return true;
		}
	}

	//Check for non-hostile or invulnerable player at square.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom() &&
			!(player.IsTarget() && player.IsVulnerableToBodyAttack()) &&
			player.wX == wCol && player.wY == wRow)
		return true;
	
	//Check for player's sword at square.
	if (this->pCurrentGame->IsPlayerWeaponAt(wCol, wRow, true))
		return true;

	//Check for monster sword at square.
	if (room.IsMonsterSwordAt(wCol, wRow, true, this))
		return true;

	//No obstacle.
	return false;
}

//*****************************************************************************************
bool CConstruct::KillIfOnOremites(CCueEvents &CueEvents)
//Kill the monster if on oremites.
{
	if (bOremiteDamage)
		return false; //already damaged by oremites on this turn
	if (IsOnSwordsman())
		return false; //retain for front end display death sequence

	//If construct is currently standing on oremites, kill it.
	const UINT dwTileNo = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	if (dwTileNo == T_GOO)
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		this->bOremiteDamage = true;
		if (IsAlive())
		{
			if (CRockGolem::OnStabbed(CueEvents))
			{
				this->wTurnDisabled = pGame->wTurnNo;
				pGame->TallyKill();
			}
		} else {
			pGame->pRoom->KillMonster(this, CueEvents);
			SetKillInfo(NO_ORIENTATION); //center stab effect
			CueEvents.Add(CID_MonsterDiedFromStab, this);
			pGame->pRoom->UpdatePathMapAt(this->wX, this->wY);
			pGame->pRoom->RecalcStationPaths();
		}
		return true;
	}
	return false;
}

//*****************************************************************************************
void CConstruct::Process(
//Process movement turn.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)
{
	if (KillIfOnOremites(CueEvents))
		return;

	//Don't do anything if was stunned this turn
	if (this->bNewStun)
		return;

	if (this->bBroken) {
		//can't move, but can reactivate itself periodically
		if ((this->pCurrentGame->wSpawnCycleCount % TURNS_PER_CYCLE == 0) &&
				(this->pCurrentGame->wTurnNo > this->wTurnDisabled)) //only after the same turn as disabled
		{
			//Cannot reactivate on hot tile.
			const bool can_reactivate = !this->pCurrentGame->pRoom->bGreenDoorsOpened &&
					this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) != T_HOT;
			if (can_reactivate) {
				this->bBroken = false;
				SetOrientation(0, 1); //S
				++this->pCurrentGame->pRoom->wMonsterCount;
				CueEvents.Add(CID_ConstructReanimated);
				this->pCurrentGame->pRoom->UpdatePathMapAt(this->wX, this->wY);
				this->pCurrentGame->pRoom->RecalcStationPaths();
			}
		}
		return;
	}

	//If stunned, skip turn
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY))
		return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, SmartOmniFlanking))
		return;
 
	//Before movement, remember important square contents.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wOTile = room.GetOSquare(this->wX, this->wY);
	const bool bWasOnTrapdoor = bIsFallingTile(wOTile);

	//Move to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);

	if (dx || dy)
	{
		//Drop trapdoors when moving
		if (bWasOnTrapdoor)
			room.DestroyTrapdoor(this->wX - dx, this->wY - dy, CueEvents);
		//Push movable objects
		UINT tTile = room.GetTSquare(this->wX, this->wY);
		if (bIsTLayerCoveringItem(tTile))
			room.PushTLayerObject(this->wX, this->wY, this->wX + dx, this->wY + dy, CueEvents);
	}
}

//*****************************************************************************************
bool CConstruct::OnStabbed(
//When stabbed, disabled as rock golem, but also tracks turn of disablement.
//
//Params:
	CCueEvents &CueEvents,
	const UINT /*wX*/, const UINT /*wY*/, //(in) unused
	WeaponType /*weaponType*/)
{
	if (CRockGolem::OnStabbed(CueEvents)) {
		this->wTurnDisabled = this->pCurrentGame->wTurnNo;
		return true;
	}

	return false;
}
