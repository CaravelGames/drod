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

//Slayer.cpp
//Implementation of CSlayer.

#include "Slayer.h"

#include "CurrentGame.h"
#include "DbRooms.h"

#include <BackEndLib/UtilFuncs.h>

const UINT wCombatDuration = 100;     //max frequency of combat cue event

//
//Public methods.
//

//*****************************************************************************************
CSlayer::CSlayer(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame,   //(in)   If NULL (default) then class can only be
						  //used for accessing data, and not for game processing.
	const UINT type) //[default=M_SLAYER]
	: CArmedMonster(type, pSetCurrentGame,
					GROUND_AND_SHALLOW_WATER, SPD_SLAYER) //move after stalwarts, before guards
	, openingDoorAt(static_cast<UINT>(-1), static_cast<UINT>(-1))
	, state(Seeking)
	, bMovingWisp(false)
	, bMakingDefensiveMove(false)
	, wTurnCombatBegan(-(int)wCombatDuration + 10)
{
	if (pSetCurrentGame)
		this->wTurnCombatBegan += pSetCurrentGame->wSpawnCycleCount;
}

//*****************************************************************************************
CSlayer2::CSlayer2(CCurrentGame *pSetCurrentGame, const UINT type) //[default=M_SLAYER2]
	: CSlayer(pSetCurrentGame, type)
{}

//*****************************************************************************************
bool CSlayer::OnStabbed(
//Stabbing Slayer's wisp does nothing.
//
//Returns: whether monster was killed
//
//Params:
	CCueEvents &CueEvents, const UINT wX, const UINT wY, WeaponType weaponType)
{
	if (wX != this->wX || wY != this->wY)
		return false; //stabbing wisp has no effect

	return CMonster::OnStabbed(CueEvents, wX, wY, weaponType);
}

//*****************************************************************************************
void CSlayer::Process(
//Process Slayer for movement.
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

	if (IsOnSwordsman())
	{
		if (!NPCBeethroDieCheck(this->wX, this->wY, CueEvents))
		{
			CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
			pGame->SetDyingEntity(&pGame->swordsman, this);
			CueEvents.Add(CID_MonsterKilledPlayer, this);
		}
		return;
	}

	//Determine who to attack.
	if (!GetTarget(this->wTX, this->wTY, false))
		return;

	const bool bTargetIsPlayer = this->pCurrentGame->IsPlayerAt(this->wTX, this->wTY);

	//If player just went through tunnel, regrow wisp afresh.
	//Important to protect against tactical vulnerabilities (i.e. tunnel kill).
	if (CueEvents.HasOccurred(CID_Tunnel))
	{
		this->Pieces.clear();
		if (MakeSlowTurnIfOpenTowards(this->wTX, this->wTY)) //try to guard against tunnel kill
			return;
	}

	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
		return;

	this->bMovingWisp = true;
	const bool bWispOnTarget = CheckWispIntegrity();
	this->bMovingWisp = false;

	//1. Can I kill target this turn?  (Do it.)
	int dx, dy;
	this->wDistToTarget = bTargetIsPlayer ? DistToSwordsman(false) :
		nDist(this->wX, this->wY, this->wTX, this->wTY);
	if (HasSword())
	{
		const UINT wMySX = GetWeaponX(), wMySY = GetWeaponY();
		UINT wDistMySwordToTarget = nDist(wMySX, wMySY, this->wTX, this->wTY);
		if (wDistMySwordToTarget == 0){
			if (bWeaponCutsWhenStationary(this->weaponType)){
				return this->AttackTargetUnderWeapon(this->wTX, this->wTY, CueEvents);
			}
		}

		if (wDistMySwordToTarget == 1 && (this->weaponType != WT_Dagger || wDistToTarget > 1))
		{
			//Is it possible to move and stab the target?  (Copied from CMonster::GetBestMove().)
			dx = this->wTX - wMySX;
			dy = this->wTY - wMySY;
			//Only attempt a stabbing kill if we'll still have a hook after making this move.
			const bool bFoundDir = IsOpenMove(dx, dy) && !DoesSquareRemoveWeapon(this->wX + dx, this->wY + dy);
			if (bFoundDir)
			{
				MakeStandardMove(CueEvents,dx,dy);
				this->wSwordMovement = nGetO(dx,dy);
				return;
			}
			//Is it possible to rotate and stab the target?
			const UINT wMyDistFromTarget = nDist(this->wX, this->wY, this->wTX, this->wTY);
			if (wMyDistFromTarget == 1 &&
					(GetWeaponX() == this->wTX || GetWeaponY() == this->wTY))	//one rotation is sufficient
			{
				if (MakeSlowTurnIfOpenTowards(this->wTX, this->wTY))
					return;
			}
		}
	}

	const CSwordsman& player = this->pCurrentGame->swordsman;

	
	ASSERT(this->wDistToTarget > 0);
	if (this->wDistToTarget == 1 &&
	    !(bTargetIsPlayer && !player.IsVulnerableToBodyAttack()))  // can't step on wubba player
	{
		//Step on target.
		int dxFirst, dyFirst;

		this->bMovingWisp = true;	//need to set to step on target
		const bool bFoundMove = GetDirectMovement(this->wTX, this->wTY, dxFirst, dyFirst, dx, dy);
		this->bMovingWisp = false;

		if (bFoundMove && this->wX + dx == this->wTX && this->wY + dy == this->wTY) //if this move can successfully be made
		{
			MakeStandardMove(CueEvents,dx,dy);
			this->wSwordMovement = nGetO(dx,dy);
			return;
		}
	}

	dx = sgn(this->wTX - this->wX);
	dy = sgn(this->wTY - this->wY);
	UINT desiredOrientation = nGetO(dx,dy);

	//2. Could target kill me if I'm not careful?  (Make sure I'm blocking.)
	this->wTSX = this->wTSY = 1000; //off by default
	if (bTargetIsPlayer)
	{
		this->wTO = player.wO;
		if (player.HasWeapon())
		{
			this->wTSX = player.wSwordX;
			this->wTSY = player.wSwordY;
		}
	} else {
		CMonster *pTarget = this->pCurrentGame->pRoom->GetMonsterAtSquare(this->wTX, this->wTY);
		ASSERT(pTarget);
		pTarget->GetSwordCoords(this->wTSX, this->wTSY);
		this->wTO = pTarget->wO;
	}

	const UINT wDistToTargetSword = nDist(this->wX, this->wY, this->wTSX, this->wTSY);
	if (wDistToTargetSword <= 4)
	{
		//Slayer remarks on possible close combat.
		if (this->pCurrentGame->wSpawnCycleCount > this->wTurnCombatBegan + wCombatDuration)
		{
				  CueEvents.Add(CID_SlayerCombat, this);
				  this->wTurnCombatBegan = this->pCurrentGame->wSpawnCycleCount;  //don't remark too often
		}

		//2a. Would target kill me next turn?  (Guard the kill square if so.)
		if (wDistToTargetSword < 3)
		{
			//If target can stab slayer next turn and slayer can't stab player, then go on the defensive.
			//Get target's sword position and find which square the player would need to be in
			//to hit the slayer at this orientation, and turn toward that square to guard it.
			const UINT wGuardX = this->wX + (this->wTX - this->wTSX);
			const UINT wGuardY = this->wY + (this->wTY - this->wTSY);
			const UINT wTargetDistToDangerSquare = nDist(wGuardX, wGuardY, this->wTX, this->wTY);
			if (this->pCurrentGame->pRoom->IsValidColRow(wGuardX, wGuardY) &&
					wTargetDistToDangerSquare <= 1)
			{
				dx = sgn(wGuardX - this->wX);
				dy = sgn(wGuardY - this->wY);
				desiredOrientation = nGetO(dx,dy);

				//Highest priority is survival, so don't do anything else except
				//guard if there is danger.
				if (this->wO == desiredOrientation) return;
				//If the square can't be guarded however, then retreat might be the only hope for survival.
				if (wDistToTargetSword < RotationalDistance(desiredOrientation))
					MakeDefensiveMove(CueEvents);
				else
					MakeSlowTurnIfOpen(desiredOrientation);

				return;
			}
			//else, target can't reach this square next turn because
			//1. Target can't step out of the room to get to this square, or
			//2. it's too far away.
			//So don't need to worry about guarding it -- just make best move.
			const UINT wFaceoffO = nGetO(-nGetOX(this->wTO),-nGetOY(this->wTO));
			MakeDefensiveMove(CueEvents, RotationalDistance(wFaceoffO) > 2); //don't retreat when facing the target
			return;
		}

		//Slayer should be generally facing toward the target.
		if (MakeSlowTurnIfOpen(desiredOrientation))
			return;

		//If slayer is blocking a kill, it should hold its ground.
		if (wDistToTargetSword >= 2 &&  //***2nd VARIANT: Also removing this check and rotation makes slayer easier to kill.
				this->wDistToTarget < wDistToTargetSword)   //This condition just keeps the slayer's orientation from oscillating.
		{
			if (this->wO != desiredOrientation &&
					this->wDistToTarget <= RotationalDistance(desiredOrientation) + 1)
				//Couldn't turn toward the target.  Retreating might be the only way now
				//to not become vulnerable.
			{
				MakeDefensiveMove(CueEvents);
				return;
			}
		}

		//If no move for safety presents itself, just extend the wisp, if needed.
		if (bWispOnTarget || ExtendWisp(CueEvents)) //assuming left-to-right evaluation
		{
			CueEvents.Add(CID_WispOnPlayer);
			AdvanceAlongWisp(CueEvents);
		}
		return;
	}

	//3. Not close to the player -- Face toward the player for general protection.
	const UINT desiredOrientation2 = GetOrientationFacingTarget(this->wTX, this->wTY);
	if (this->wO != desiredOrientation2 &&
			wDistToTargetSword <= RotationalDistance(desiredOrientation2) + 2)
		if (MakeSlowTurnIfOpen(desiredOrientation2))
			return;  //***5th VARIANT: Removing this return, allowing the Slayer to rotate AND move on a turn (like any monster) will make him more deadly,
						//as he can approach and engage the player more quickly.

	//4. Advance on the player.
	//Slayer extends a "wisp" from himself to locate target
	//and then moves to kill target.  If a door is closed along the wisp,
	//then the Slayer will attempt to reopen the door, and then resume advancing.
	switch (this->state)
	{
		case Seeking:
			//A wisp stretches one turn outward from the slayer in search of target.
			if (bWispOnTarget || ExtendWisp(CueEvents)) //assuming left-to-right evaluation
			{
				//The wisp has found the target, and the Slayer advances along it.
				//The wisp continues to stretch towards the target each turn using the above rules
				//with the goal of maintaining a constant connection to the target.
				CueEvents.Add(CID_WispOnPlayer);
				AdvanceAlongWisp(CueEvents);
			}
		break;

		case OpeningDoor:
			MoveToOpenDoor(CueEvents);
		break;
	}
}

//*****************************************************************************************
void CSlayer::StopStrikingOrb(CCueEvents &CueEvents)
//If slayer is next to the orb he was trying to hit (i.e. he opened the door by striking
//the orb last turn), then he must avoid hitting the orb again this turn.
{
	//If there were no orbs to hit, then we have nothing to avoid hitting
	if (this->orbsToHit.empty()) return;

	UINT destX, destY;
	GetAdjDest(destX, destY);
	if (!this->pCurrentGame->pRoom->IsValidColRow(destX, destY)) return;

	const UINT orbStrikingOrientation = GetOrientationFacingTarget(destX, destY);
	ASSERT(orbStrikingOrientation != NO_ORIENTATION);
	if (orbStrikingOrientation != this->wO) return; //slayer didn't hit orb last turn

	//Orb was hit last turn -- must avoid hitting it again by moving.
	if (this->Pieces.size())
		AdvanceAlongWisp(CueEvents);
	else
	{
		//Try turning to avoid hitting orb again.
		if (!MakeSlowTurnIfOpen(nNextCO(orbStrikingOrientation)))
			if (!MakeSlowTurnIfOpen(nNextCCO(orbStrikingOrientation)))
			{
				//Move in opposite direction from orb.
				int dxFirst, dyFirst, dx = this->wX - destX, dy = this->wY - destY;
				GetBeelineMovementSmart(this->wX + dx, this->wY + dy, dxFirst, dyFirst, dx, dy, true);
				if (dx || dy)
				{
					if (IsSafeToStab(this->wX + dx, this->wY + dy, this->wO))
		            MakeStandardMove(CueEvents,dx,dy);
					//else orb is going to be hit again next turn, possibly re-closing door
					this->wSwordMovement = nGetO(dx,dy);
				}
			}
	}
}

//******************************************************************************************
bool CSlayer::DoesSquareContainObstacle(
//Override for Slayer brained pathmapping -- used mostly for moving wisp,
//which ignores all but decoy swords.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	if (!this->bMovingWisp) {
		//Check for monster at square.
		CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
		if (pMonster)
		{
			//Don't step-kill guards or other slayers
			if (pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2 || pMonster->wType == M_GUARD)
				return true;
		}

		return CArmedMonster::DoesSquareContainObstacle(wCol, wRow);
	}

	if (!room.IsValidColRow(wCol,wRow))
	{
		ASSERT(!"CSlayer::DoesSquareContainObstacle -- (wCol,wRow) must be valid.");
		return true;
	}

	//Check t-layer for obstacle.
	UINT wLookTileNo = room.GetTSquare(wCol, wRow);
	if (wLookTileNo != T_EMPTY && IsTileObstacle(wLookTileNo))
		return true;

	//Check f-layer for obstacle.
	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check o-layer for obstacle.
	wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//Can move into tunnel entrance if player is there.
		if (!bIsTunnel(wLookTileNo))
			return true;

		if (!this->pCurrentGame->IsPlayerAt(wCol, wRow))
			return true;
	}

	//Check for monster at square.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
	{
		//Serpents, guards, and rock giants (that would obstruct the way on death)
		//stop the wisp from advancing.
		if (pMonster->IsLongMonster() || pMonster->IsPiece() ||
				bIsRockGolemType(pMonster->wType) || pMonster->wType == M_GUARD ||
				pMonster->wType == M_CHARACTER ||
				pMonster->wType == M_FEGUNDO || pMonster->wType == M_FEGUNDOASHES)
			return true;
	}

	//Check for stationary (decoy) swords.  They are considered an obstacle.
	if (!room.Decoys.empty())
		for (int wJ=-1; wJ<=1; ++wJ) //O(9) search
			for (int wI=-1; wI<=1; ++wI)
				if ((wI || wJ) && room.IsValidColRow(wCol+wI, wRow+wJ))
				{
					CMonster *pMonster = room.GetMonsterAtSquare(wCol+wI, wRow+wJ);
					if (pMonster && pMonster->wType == M_DECOY)
					{
						CPlayerDouble *pDecoy = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
						if (pDecoy->HasSwordAt(wCol, wRow))
							return true;
					}
				}

	return false;
}

//*****************************************************************************
bool CSlayer::IsTileObstacle(
//Override for slayers.
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
			//Slayers don't like active firetraps
			wLookTileNo == T_FIRETRAP_ON ||
			CArmedMonster::IsTileObstacle(wLookTileNo)
			);
}

//*****************************************************************************
bool CSlayer::IsOpenMove(const int dx, const int dy) const
{
	return CArmedMonster::IsOpenMove(dx,dy) &&
			IsSafeToStab(this->wX + dx, this->wY + dy, this->wO);
}

//*****************************************************************************
bool CSlayer::IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const
//For determining where wisp can be advanced using FindOptimalPathTo().
//Returns: whether moving from (wX,wY) along (dx,dy) is valid.
{
	return !(DoesSquareContainObstacle(wX + dx, wY + dy) ||
		DoesArrowPreventMovement(wX, wY, dx, dy) ||
		this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(wX, wY, dx, dy));
}

//*****************************************************************************
bool CSlayer::IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const
//Returns: true if standing at (wFromX, wFromY) with sword orientation at wSO won't stab anything
//dangerous, or a guard or Slayer.  Otherwise false.
{
	if (this->weaponType == WT_Dagger && wSO != nGetO(wFromX - this->wX, wFromY - this->wY)){
		return true;
	}
	return CArmedMonster::IsSafeToStab(wFromX, wFromY, wSO);
}

//*****************************************************************************************
void CSlayer::AdvanceAlongWisp(CCueEvents &CueEvents)
//Moves Slayer one square forward along the wisp.  If "shortcuts" can be made
//by stepping to an adjacent point further along the wisp, do that instead.
{
	if (this->Pieces.empty()) return;

	//Check from end of wisp (since this will be closest to the target)
	//for first step that can be made.
	int dxFirst, dyFirst;
	bool bWispAdjToSlayer = false;
	for (WISP wisp = this->Pieces.end(); SafePostDec(wisp, this->Pieces.begin()) != this->Pieces.begin(); )
	{
		//If wisp is on the player's sword, don't try to go there.
		//(This prevents weird movement oscillations at close quarters.)
		if ((*wisp)->wX == this->wTSX && (*wisp)->wY == this->wTSY)
			continue;

		int dx = (*wisp)->wX - this->wX;
		int dy = (*wisp)->wY - this->wY;
		if (!(dx || dy))
		{
			//Wisp has looped around back to Slayer -- remove the cycle.
			SeverWispAt(wisp, true);
			return;
		}

		//Don't consider wisp pieces that aren't adjacent to the slayer.
		const bool bWispPieceIsAdjacentSlayer = (abs(dx) <= 1) && (abs(dy) <= 1);
		if (!bWispPieceIsAdjacentSlayer) continue;

		//Don't consider wisp pieces that don't have an open move to them.
		if (!IsOpenMove(this->wX, this->wY, dx, dy)) continue;

		bWispAdjToSlayer = true;
		GetBeelineMovementSmart(this->wX + dx*2, this->wY + dy*2, dxFirst, dyFirst, dx, dy, true);
		if (dx || dy)
		{
			//Found a move along wisp that's possible to make.
			//Before advancing along wisp, make sure move won't put Slayer at risk.
			const UINT wNewDistFromPlayerSword = nDist(this->wX + dx, this->wY + dy,
					this->wTSX, this->wTSY);
			const UINT nNewOrientationFacingPlayer = nGetO(
					sgn(this->wTX - (this->wX + dx)), sgn(this->wTY - (this->wY + dy)));
			if (wNewDistFromPlayerSword <= RotationalDistance(nNewOrientationFacingPlayer)+1)
			{
				//Could be dangerous to move here (I could get stabbed soon).
				//Don't move here, but rotate to face the player.
				MakeSlowTurnIfOpen(nGetO(sgn(this->wTX - this->wX), sgn(this->wTY - this->wY)));
				return;
			}

			MakeStandardMove(CueEvents,dx,dy);
			this->wSwordMovement = nGetO(dx,dy);
			//If slayer has moved onto wisp, then remove it (and anything before).
			//Otherwise, Slayer's gotten behind and it's not time to remove a piece yet.
			if ((*wisp)->wX == this->wX && (*wisp)->wY == this->wY)
				SeverWispAt(wisp, true);
			return;
		} else {
			if (wisp != this->Pieces.begin()) continue;   //not last one
			//Only possible wisp move -- Test whether a rotation would make the move safe.
			const UINT wCurO = this->wO;
			this->wO = nNextCO(this->wO);
			if (IsSafeToStab(this->wX + dx, this->wY + dy, this->wO))
			{
				this->wSwordMovement = CSwordsman::GetSwordMovement(CMD_C, this->wO);
				return;  //turn clockwise -- will try advancing again next turn
			}
			this->wO = nNextCCO(nNextCCO(this->wO));
			if (IsSafeToStab(this->wX + dx, this->wY + dy, this->wO))
			{
				this->wSwordMovement = CSwordsman::GetSwordMovement(CMD_CC, this->wO);
				return;  //turn counter-clockwise -- will try advancing again next turn
			}
			this->wO = wCurO; //nothing works -- stuck for now
			this->wSwordMovement = NO_ORIENTATION;
		}
	}

	//If the slayer isn't next to any of the existing wisp pieces, regrow the wisp afresh.
	if (!bWispAdjToSlayer)
		this->Pieces.clear();
}

//*****************************************************************************************
bool CSlayer::CheckWispIntegrity()
//Entire path of the wisp is checked.  If an obstacle is found that
//blocks the wisp, the wisp is truncated to that point.
//
//Returns: whether target was encountered along path
{
	bool bPieceIsAdjacent = false;	//adjacent to slayer
	UINT wPrevX = this->wX, wPrevY = this->wY;
	int dx, dy;
	for (WISP piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		CMonsterPiece *pPiece = *piece;

		//Has wisp encountered target?
		if (pPiece->wX == this->wTX && pPiece->wY == this->wTY)
		{
			++piece;
			SeverWispAt(piece);  //don't need any wisp past target
			return true;
		}

		//Is wisp now on obstacle?
		dx = pPiece->wX - wPrevX;
		dy = pPiece->wY - wPrevY;
		if (!IsOpenMove(wPrevX, wPrevY, dx, dy))
		{
			//Obstacle encountered -- sever wisp here.
			if (this->pCurrentGame->pRoom->GetOSquare(pPiece->wX, pPiece->wY) == T_DOOR_Y)
			{
				//Door closed -- find another way around, or an orb to open it.
				this->bMovingWisp = true;  //enables forming path through swords
				CCoordSet target(this->wTX, this->wTY);
				const bool bRes = FindOptimalPathTo(this->wX, this->wY, target, false);
				this->bMovingWisp = false;
				if (!bRes)  //no way around -- re-opening the door is the only way to the player
					NeedToOpenDoor(pPiece->wX, pPiece->wY);

				//!!Add Slayer cue event -- mad when door closes
			}
			SeverWispAt(piece);
			return false;
		}

		wPrevX = pPiece->wX;
		wPrevY = pPiece->wY;

		const bool bWispPieceIsAdjacentSlayer =
			(abs((int)wPrevX - (int)this->wX) <= 1) && (abs((int)wPrevY - (int)this->wY) <= 1);
		if (bWispPieceIsAdjacentSlayer)
			bPieceIsAdjacent = true;
	}

	//If wisp is nowhere next to slayer, then it must be remade afresh.
	//Otherwise time is just being wasted, since it will be cleared when slayer
	//starts trying to move.
	if (!bPieceIsAdjacent)
		this->Pieces.clear();

	return false;
}

//*****************************************************************************************
bool CSlayer::ExtendWisp(CCueEvents &CueEvents, const bool bMoveAllowed)   //[true]
//The wisp grows one square towards target using brain-directed behavior.
//Obstacles for the wisp are the same as for roaches (Ground movement).
//
//Returns: whether wisp is on player
{
	if (this->bMakingDefensiveMove) return false;

	const UINT wSavedX = this->wX, wSavedY = this->wY;
	if (this->Pieces.size())
	{
		//Temporarily change monster's position to check from the end of the wisp.
		CMonsterPiece *endOfWisp = this->Pieces.back();
		this->wX = endOfWisp->wX;
		this->wY = endOfWisp->wY;
	} //else wisp hasn't started yet -- begin at Slayer

	//If wisp has reached target, Slayer begins advancing.
	if (this->wTX == this->wX && this->wTY == this->wY)
	{
		this->wX = wSavedX;
		this->wY = wSavedY;
		return true;
	}

	//Wisp is seeking out target.
	const UINT wTempO = this->wO;
	this->wO = NO_ORIENTATION; //don't assume where slayer's sword will be along wisp
	this->bMovingWisp = true;  //enables forming path through swords
	CCoordSet target(this->wTX, this->wTY);
	const bool bRes = FindOptimalPathTo(this->wX, this->wY, target, false);
	this->bMovingWisp = false;
	this->wO = wTempO;
	if (bRes)
	{
		if (this->pathToDest.GetSize())
		{
			//Is target on part of the wisp already?
			WISP wisp = GetWispAt(this->wTX, this->wTY);
			if (wisp != this->Pieces.end())
			{
				//Yes -- truncate it at this earlier point.
				++wisp;
				SeverWispAt(wisp);
			} else {
				//Has wisp doubled back on itself?
				UINT wNextX, wNextY;
				this->pathToDest.Pop(wNextX, wNextY);
				const bool bWispHasReachedTarget = this->wTX == wNextX && this->wTY == wNextY;
				if (!bWispHasReachedTarget)
				{
					WISP wisp = GetWispAt(wNextX, wNextY);
					if (wisp != this->Pieces.end())
						SeverWispAt(wisp); //Yes -- remove the wisp cycle.
				}
				this->Pieces.push_back(new CMonsterPiece(this, 0, wNextX, wNextY));

				//Wisp has reached target -- Slayer begins advancing.
				if (bWispHasReachedTarget)
				{
					this->wX = wSavedX;
					this->wY = wSavedY;
					return true;
				}
			}
		}
		this->wX = wSavedX;
		this->wY = wSavedY;
	} else {
		//No path exists.  Just advance along wisp.
		this->wX = wSavedX;
		this->wY = wSavedY;
		if (this->Pieces.size())
			AdvanceAlongWisp(CueEvents);
		else if (bMoveAllowed)
		{
			//No wisp remains -- just beeline or defend.
			if (this->wDistToTarget <= 3)
			{
				this->bMakingDefensiveMove = true;  //to avoid infinite recursion
				MakeDefensiveMove(CueEvents);
				this->bMakingDefensiveMove = false;
			} else {
				int dxFirst, dyFirst, dx, dy;
				GetBeelineMovementSmart(this->wTX, this->wTY, dxFirst, dyFirst, dx, dy, true);
				if (dx || dy)
				{
					if (IsSafeToStab(this->wX + dx, this->wY + dy, this->wO))
					{
		            MakeStandardMove(CueEvents,dx,dy);
						this->wSwordMovement = nGetO(dx,dy);
					}
				} else {
					//Can't beeline.  If a door is adjacent, try to open it.
					UINT wX, wY;
					this->wSwordMovement = NO_ORIENTATION;
					if (IsObjectAdjacent(T_DOOR_Y, wX, wY))   //!!this only returns the first adjacent door
						NeedToOpenDoor(wX, wY);
				}
			}
		}
	}
	return false;
}

//*****************************************************************************************
WISP CSlayer::GetWispAt(const UINT wX, const UINT wY)
//Returns: whether a piece of wisp is at (wX, wY)
{
	for (WISP piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		const CMonsterPiece *pPiece = *piece;
		if (wX == pPiece->wX && wY == pPiece->wY)
			return piece;
	}
	return this->Pieces.end();
}

//*****************************************************************************************
void CSlayer::SeverWispAt(WISP piece, const bool bKeepBack)   //[default=false]
//Removes all elements of the wisp list after (or before) 'piece' (inclusive).
{
	if (piece == this->Pieces.end()) return;  //nothing to do

	MonsterPieces truncatedWisp;
	if (bKeepBack)
		truncatedWisp.splice(truncatedWisp.end(), this->Pieces, this->Pieces.begin(), ++piece);
	else
		truncatedWisp.splice(truncatedWisp.end(), this->Pieces, piece, this->Pieces.end());
	while (truncatedWisp.size())
	{
		delete truncatedWisp.back();
		truncatedWisp.pop_back();
	}
}

//*****************************************************************************************
void CSlayer::MakeDefensiveMove(
//When player threatens an imminent kill at close quarters,
//choose the best possible available move (with potential for both attack and defense).
//
//Params:
	CCueEvents &CueEvents,	//(in/out)
	const bool bCanRetreat)	//[default=true] whether retreat is allowed if it's the best move
									//i.e. when false, only opportunistic attacks are made
{
//Determine how directly player and Slayer are facing each other.
#define GET_DIRECTIONS { \
	/* When facing opposite the player's orientation, they would each need one more move to stab each other */ \
	if (RotationalDistance(this->wTO) == 4) {++fSafetyDist; ++fAttackDist;} \
\
	dx = sgn(this->wTX - this->wX); \
	dy = sgn(this->wTY - this->wY); \
	wRotDistToP = RotationalDistance(nGetO(dx,dy)); \
	this->wO = this->wTO; \
	wRotDistToS = RotationalDistance(nGetO(-dx,-dy)); \
	this->wO = wOrigO; \
\
   /* Being rotated (more than one notch) away from one's opponent counts against you */ \
	if (wRotDistToS) --wRotDistToS; \
	if (wRotDistToP) --wRotDistToP; \
	fSafetyDist += wRotDistToS/2.0f; \
	fAttackDist += wRotDistToP/2.0f; \
}

//Measure the worth of the proposed state.
#define CALC_SCORE { \
	GET_DIRECTIONS \
	fScore = fSafetyDist*2 - fAttackDist; \
	if ((fScore > fBestScore) || (fScore == fBestScore && \
			((fSafetyDist <= fAttackDist || fBestSafetyDist <= fBestAttackDist) ? (fSafetyDist > fBestSafetyDist) : (fAttackDist < fBestAttackDist)))) { \
			fBestScore = fScore; fBestSafetyDist = fSafetyDist; fBestAttackDist = fAttackDist; nBestCommand = nCommand; \
}	}

//When calculating worth of movement, use L-inf norm since that's how moves are made.
#define COMPARE_SCORE { \
	fSafetyDist = (float)nDist(this->wTSX,this->wTSY,this->wX,this->wY); \
	fAttackDist = (float)nDist(GetWeaponX(), GetWeaponY(), this->wTX, this->wTY); \
	CALC_SCORE \
}

	const UINT wOrigX = this->wX, wOrigY = this->wY, wOrigO = this->wO;

	//Determine which command maximizes the equation:
	// dist(player's sword, me) - dist(my sword, player)
	//and give more weight to defense than attack, since player will move next.
	float fSafetyDist, fAttackDist, fBestAttackDist = 9999, fBestSafetyDist = 0;
	float fScore, fBestScore = -1000;
	int nCommand = NO_ORIENTATION, nBestCommand = -1;
	int dx, dy;
	UINT wRotDistToP, wRotDistToS;

	//Remaining stationary gets priority if nothing is better.
	COMPARE_SCORE;

	//Rotations.
	nCommand = CMD_C;
	this->wO = nNextCO(this->wO);
	if (IsSafeToStab(this->wX, this->wY, this->wO))
		COMPARE_SCORE;

	nCommand = CMD_CC;
	this->wO = nNextCCO(wOrigO);
	if (IsSafeToStab(this->wX, this->wY, this->wO))
		COMPARE_SCORE;
	this->wO = wOrigO;

	//Movement.
	static const int dXs[ORIENTATION_COUNT] = { 0,  1,  0, -1,  0,  1,  1, -1, -1};
	static const int dYs[ORIENTATION_COUNT] = {-1,  0,  1,  0,  0, -1,  1,  1, -1};
	for (nCommand = 0; nCommand < (int)ORIENTATION_COUNT; ++nCommand)
	{
		if (nCommand == (int)NO_ORIENTATION) continue;
		if (!IsOpenMove(dXs[nCommand], dYs[nCommand])) continue;

		this->wX += dXs[nCommand];
		this->wY += dYs[nCommand];

		if (bCanRetreat)
		{	//must have {}'s here
			COMPARE_SCORE;
		} else {
			//If only opportunistic attack is allowed, ensure this move
			//wouldn't be backing up.
			const UINT wNewProposedDistFromTarget = nDist(this->wX, this->wY,
					this->wTX, this->wTY);
			if (wNewProposedDistFromTarget <= this->wDistToTarget)
				COMPARE_SCORE;
		}
		this->wX = wOrigX;
		this->wY = wOrigY;
	}

	//Execute best move.
	if (nBestCommand < (int)ORIENTATION_COUNT)
	{
		if (nBestCommand == (int)NO_ORIENTATION || nBestCommand == -1)
		{
			if (ExtendWisp(CueEvents, bCanRetreat))
				CueEvents.Add(CID_WispOnPlayer);
		} else {
			const int dx = dXs[nBestCommand], dy = dYs[nBestCommand];
			if (this->Pieces.size())
			{
				//Prepend to the wisp where the Slayer is moving from to maintain continuity.
				WISP wisp = GetWispAt(this->wX,this->wY);
				if (wisp == this->Pieces.end())	//no wisp there yet
					this->Pieces.insert(this->Pieces.begin(),
							new CMonsterPiece(this, 0, this->wX, this->wY));

				//Cut front of wisp at any point the Slayer steps onto it.
				wisp = GetWispAt(this->wX + dx,this->wY + dy);
				SeverWispAt(wisp, true);
			}

			MakeStandardMove(CueEvents,dx,dy);
			this->wSwordMovement = nGetO(dx,dy);
		}
	} else {
		const bool bTurnCW = nBestCommand == (int)ORIENTATION_COUNT;
		this->wO = bTurnCW ? nNextCO(this->wO) : nNextCCO(this->wO);
		this->wSwordMovement = CSwordsman::GetSwordMovement(bTurnCW ? CMD_C : CMD_CC, this->wO);
	}

#undef CALC_SCORE
#undef COMPARE_SCORE
#undef GET_DIRECTIONS
}

//******************************************************************************************
void CSlayer::GetAdjDest(
//Returns the first coord in list adjacent to current position, or (-1,-1) if none.
//
//Params:
	UINT& wDestX, UINT& wDestY)   //(out) First coord in list adjacent to current position
const
{
	ASSERT(!this->orbsToHit.empty());
	for (CCoordSet::const_iterator orb=this->orbsToHit.begin(); orb!=this->orbsToHit.end(); ++orb)
	{
		if (nDist(this->wX, this->wY, orb->wX, orb->wY) <= 1)
		{
			wDestX = orb->wX;
			wDestY = orb->wY;
			return;
		}
	}
	wDestX = wDestY = static_cast<UINT>(-1);
}

//***************************************************************************************
void CSlayer::MoveToOpenDoor(CCueEvents &CueEvents)     //(in/out)
//Move to strike an orb to open a door along the way to the player.
//If the door has opened, then I can stop going to the orb.
{
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	ASSERT(room.IsValidColRow(this->openingDoorAt.wX, this->openingDoorAt.wY));
	if (room.GetOSquare(this->openingDoorAt.wX, this->openingDoorAt.wY) != T_DOOR_Y)
	{
		StopStrikingOrb(CueEvents);
		StopOpeningDoor();
		return;
	}

	//Confirm path to goal is still open.
	if (!ConfirmPath())
	{
		//If it's not, search for a new path to the goal.
		bool bOrbPathFound = FindOptimalPathTo(this->wX, this->wY, this->orbsToHit, true);
		bool bPathFound = bOrbPathFound;
		CCoordSet plates;
		room.GetDepressablePlateSubset(this->platesToDepress, plates);
		if (!plates.empty())
		{
			CCoordStack orbPath = this->pathToDest;
			bPathFound |= FindOptimalPathTo(this->wX, this->wY, plates, false);

			//Choose shorter path.  With a tie, paths to orbs take preference.
			if (bOrbPathFound && this->pathToDest.GetSize() >= orbPath.GetSize())
				this->pathToDest = orbPath;
		}
		if (!bPathFound)
		{
			//No path is available -- go back to basic searching for a target.
			StopOpeningDoor();
			return;
		}
	}

	if (this->pathToDest.GetSize())
	{
		//As Slayer supposedly gets further from the player,
		//prepend backwards steps to wisp so Slayer doesn't have to rebuild
		//the wisp once the door is re-opened.
		this->Pieces.insert(this->Pieces.begin(),
				new CMonsterPiece(this, 0, this->wX, this->wY));

		//Calculate direction for another step toward orb.
		UINT wNextX, wNextY;
		this->pathToDest.Pop(wNextX, wNextY);  //Slayer is now making this move
		this->wSwordMovement = nGetO(wNextX - this->wX, wNextY - this->wY);
		Move(wNextX, wNextY, &CueEvents);
	} else {
		//If we think we're next to our destination but there are no orbs to hit,
		//then we must be on a plate.  Start chasing the player again.
		if (this->orbsToHit.empty())
		{
			StopOpeningDoor();
			return;
		}

		//Adjacent to one of destination orbs -- select and rotate to hit it w/ sword.
		UINT destX, destY;
		GetAdjDest(destX, destY);
		ASSERT(room.IsValidColRow(destX, destY));
		const UINT orbStrikingOrientation = GetOrientationFacingTarget(destX, destY);
		ASSERT(orbStrikingOrientation != NO_ORIENTATION);
		if (!MakeSlowTurnIfOpen(orbStrikingOrientation))
		{
			//Slayer can't turn to strike the orb.  Start searching for player again.
			StopStrikingOrb(CueEvents);
			StopOpeningDoor();
		}
	}
}

//***************************************************************************************
void CSlayer::NeedToOpenDoor(
//Search for orbs that the Slayer can hit to open the door at (wX, wY).
//If any are accessible, start the Slayer moving toward the closest one.
//If Slayer is already going to open this door, do nothing.
//
//Params:
	const UINT wX, const UINT wY)  //(in)
{
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	

	CCoordSet doorSquares;
	room.GetAllYellowDoorSquares(wX, wY, doorSquares);

	//Is Slayer already going to strike this orb?  If so, then don't need to do anything here.
	if (doorSquares.has(this->openingDoorAt))
		return;

	//Compile list of all orbs that can open this door.
	CCoordSet orbs, plates;
	room.FindOrbsToOpenDoor(orbs, doorSquares);
	room.FindPlatesToOpenDoor(plates, doorSquares);

	//Slayer will to try to reach any of the orbs that will open the door.
	if (FindOptimalPathTo(this->wX, this->wY, orbs))
	{
		//Slayer has a path to an orb for this door -- go strike it.
		this->orbsToHit = orbs;
		this->platesToDepress = plates; //remember these in case orbs get blocked
		this->openingDoorAt.wX = wX;
		this->openingDoorAt.wY = wY;
		this->state = OpeningDoor;
	}

	//If pressure plates that could open the door exist, check these also.
	//If a valid path is found.
	if (!plates.empty())
	{
		const CCoordStack orbPath = this->pathToDest; //retain this path
		CCoordSet depressablePlates;
		room.GetDepressablePlateSubset(plates, depressablePlates);
		
		if (!FindOptimalPathTo(this->wX, this->wY, depressablePlates,
				false)) //must step on plate, not be adjacent to it
		{
			this->pathToDest = orbPath;
		} else {
			this->orbsToHit = orbs; //remember these in case plates get blocked
			this->platesToDepress = plates;
			this->openingDoorAt.wX = wX;
			this->openingDoorAt.wY = wY;
			this->state = OpeningDoor;

			//Choose shorter path.  With a tie, paths to orbs take preference.
			if (this->pathToDest.GetSize() >= orbPath.GetSize())
				this->pathToDest = orbPath;
		}
	}

	//if (neither path was found)
		//Slayer can't reach any orbs to open this door.
}

//***************************************************************************************
void CSlayer::StopOpeningDoor()
//Revert to original state.
{
	this->state = Seeking;
	this->orbsToHit.clear();
	this->openingDoorAt.wX = this->openingDoorAt.wY = static_cast<UINT>(-1);
}
