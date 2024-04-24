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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002 
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//FluffBaby.cpp
//Implementation of CFluffBaby.

#include "FluffBaby.h"
#include "Swordsman.h"
#include "Character.h"
#include "CurrentGame.h"
#include "DbRooms.h"

#define NO_TARGET ((UINT)-1) //this distance to a target represents no valid option

//
//Public methods.
//

//*****************************************************************************************
bool CFluffBaby::KillIfInWall(CCueEvents &CueEvents)
//Kill the monster if in a solid object or on hot tiles.
{
	bool bDie = false;

	//If ghost was on a door that opened, kill it.
	const UINT wTileNo = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	if (bIsWall(wTileNo) || bIsCrumblyWall(wTileNo) || bIsDoor(wTileNo) || wTileNo == T_HOT)
		bDie = true;
	
	if (bDie)
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->pRoom->KillMonster(this, CueEvents);
		pGame->pRoom->ProcessPuffAttack(CueEvents, this->wX, this->wY);
		return true;
	}
	return false;
}

//******************************************************************************
bool CFluffBaby::GetGoal(UINT& wX, UINT& wY) const
//Call to query whether the puff has a new target
//
//Returns: whether puff has a current goal at (wX,wY)
{
	const CSwordsman& player = this->pCurrentGame->swordsman;
	const CDbRoom& room = *(this->pCurrentGame->pRoom);

	UINT wDistance, wClosestDistance = NO_TARGET;
	UINT wL1Dist, wClosestL1Dist = NO_TARGET;

	//Check player location first as a baseline
	UINT wSX = player.wX, wSY = player.wY;
	if (player.IsInRoom() && player.CanFluffTrack() &&
			(player.IsVisible() || CanSmellObjectAt(player.wX, player.wY)))
	{
		//Player is Beethro or a comparable target that is somehow sensed by this monster,
		//either directly or through a brain.
		wClosestDistance = nDist(wSX, wSY, this->wX, this->wY);
		wClosestL1Dist = nL1Dist(wSX, wSY, this->wX, this->wY);
		wX = wSX; //pending target
		wY = wSY;
	}

	for (const CMonster *pMonster = room.pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->CanFluffTrack())
		{
			if (pMonster->IsHiding() && !CanSmellObjectAt(pMonster->wX, pMonster->wY))
			{
				continue;
			}

			wDistance = nDist(pMonster->wX, pMonster->wY, this->wX, this->wY);
			if (wDistance > wClosestDistance)
				continue; //further away -- ignore

			//Break ties using L1 distance.
			wL1Dist = nL1Dist(pMonster->wX, pMonster->wY, this->wX, this->wY);
			if (wDistance == wClosestDistance && wL1Dist > wClosestL1Dist)
				continue;

			//Attracted to closest monster.
			wClosestDistance = wDistance;
			wClosestL1Dist = wL1Dist;
			wX = pMonster->wX;
			wY = pMonster->wY;
		}
	}

	if (wClosestDistance != NO_TARGET)
		return true; //a target has been chosen

	// No target found
	return false;
}

//*****************************************************************************
bool CFluffBaby::CanPushOntoOTile(UINT wTile) const
//Override for Fluff puffs
{
	//Pushing onto Hot Tiles should fail, causing a "Puff Attack" instead
	if (wTile == T_HOT) {
		return false;
	}

	return CMonster::CanPushOntoOTile(wTile);
}

//*****************************************************************************
bool CFluffBaby::DoesSquareContainObstacle(
//Override for Fluff puffs.  Parts copied from CMonster::DoesSquareContainObstacle.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	//Check t-square for obstacle.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow)) return true;

	//Some t-layer objects are obstacles.  Check for these.
	UINT wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo) && !bIsBriar(wLookTileNo))
		return true;

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check o-square obstacle.
	wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.

		if (bIsTunnel(wLookTileNo))
		{
			if (CanMoveOntoTunnelAt(wCol,wRow))
				goto CheckMonster;
		}

		//No special handling was performed.  Treat it as an obstacle.
		return true;
	}

CheckMonster:
	//Cannot move onto Fluff Puffs
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
	{
		const UINT wType = pMonster->GetResolvedIdentity();
		if (bIsSerpentOrGentryii(wType) && pMonster->IsPiece())
			return true;
		if (wType == M_FLUFFBABY)
			return true;
	}

	//Cannot move onto Fluff Puff-role player
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom() && player.wAppearance == M_FLUFFBABY &&
			player.wX == wCol && player.wY == wRow)
		return true;
	
	//No obstacle.
	return false;
}

//*****************************************************************************
bool CFluffBaby::IsOpenMove(const int dx, const int dy) const
//Returns: whether moving from the current position along (dx,dy) is valid
{
	const UINT wDestX = this->wX + dx;
	const UINT wDestY = this->wY + dy;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (DoesSquareContainObstacle(wDestX, wDestY) ||
				DoesArrowPreventMovement(this->wX, this->wY, dx, dy) ||
				room.DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy))
		return false;

	//Check for Pickaxes/Cabers blocking Puff movement
	const UINT wO = nGetReverseO(nGetO(dx,dy));
	for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO) {
		if (nO != NO_ORIENTATION)
		{
			CMonster *pMonster = room.GetMonsterAtSquare(wDestX-nGetOX(nO), wDestY-nGetOY(nO));
			if (pMonster && pMonster->HasSwordAt(wDestX, wDestY))
			{
				switch(pMonster->GetWeaponType())
				{
					case WT_Pickaxe:
						if (wO == nO || wO == nNextCO(nO) || wO == nNextCCO(nO))
							return false;
					break;
					case WT_Caber:
						return false;
					break;

					default: break;
				}
			}
		}
	}
	if (this->pCurrentGame->IsPlayerWeaponAt(wDestX, wDestY))
	{
		switch(this->pCurrentGame->swordsman.GetActiveWeapon())
		{
			case WT_Pickaxe:
			{
				const UINT nO = this->pCurrentGame->swordsman.wO;
				if (wO == nO || wO == nNextCO(nO) || wO == nNextCCO(nO))
					return false;
			}
			break;
			case WT_Caber:
				return false;
			break;

			default: break;
		}
	}

	//No obstacles
	return true;
}

//*****************************************************************************************
void CFluffBaby::MovePuff(
//Move a Fluff Puff in the indicated direction.
//
//Params:
//Params:
	CCueEvents &CueEvents,  //(out)  Add cue events if appropriate.
	const int dx, const int dy)   //(in)   Movement offset.
{
	if (!dx && !dy)
		return;

	const UINT wDestX = this->wX + dx;
	const UINT wDestY = this->wY + dy;

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wOTile = room.GetOSquare(wDestX,wDestY);
	const UINT wTTile = room.GetTSquare(wDestX,wDestY);
	CMonster *pMonster = room.GetMonsterAtSquare(wDestX,wDestY);
	if (wOTile == T_HOT || bIsBriar(wTTile) || pMonster || this->pCurrentGame->IsPlayerAt(wDestX,wDestY))
	{
		room.KillMonster(this,CueEvents,false,this);
		room.ProcessPuffAttack(CueEvents,wDestX,wDestY);
	} else {
		room.MoveMonster(this,wDestX,wDestY);
		this->wX = wDestX;
		this->wY = wDestY;
	}
}

//*****************************************************************************************
void CFluffBaby::Process(
//Process a Fluff Puff for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned()) {
		CheckForRoomDamageWhenStationary(CueEvents);
		return;
	}

	//Only move every 5th turn
	if (this->pCurrentGame->wSpawnCycleCount % 5 != 0) {
		CheckForRoomDamageWhenStationary(CueEvents);
		return;
	}

	//Find where to move to.
	UINT wX, wY;
	if (!GetGoal(wX,wY)) {
		CheckForRoomDamageWhenStationary(CueEvents);
		return;
	}

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, SmartDiagonalOnly, true)) {
		CheckForRoomDamageWhenStationary(CueEvents);
		return;
	}
 
	//Move Puff to new destination square.
	MovePuff(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}

//******************************************************************************************
bool CFluffBaby::OnStabbed(CCueEvents &/*CueEvents*/, const UINT /*wX*/, const UINT /*wY*/, WeaponType /*weaponType*/)
{
	//Stabs don't kill Fluff puffs.  Firetrap damage is dealt with in CCurrentGame::StabMonsterAt
	return false;
}

//******************************************************************************************
bool CFluffBaby::CheckForRoomDamageWhenStationary(CCueEvents &CueEvents)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	const UINT wOTile = room.GetOSquare(this->wX,this->wY);
	const UINT wTTile = room.GetTSquare(this->wX,this->wY);
	if (wOTile == T_HOT || bIsBriar(wTTile)) {
		room.KillMonster(this,CueEvents,false,this);
		room.ProcessPuffAttack(CueEvents,this->wX,this->wY);
		return true;
	}
	return false;
}
