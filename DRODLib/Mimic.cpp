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

#include "Mimic.h"
#include "TemporalClone.h"
#include "Swordsman.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include "Character.h"

//
//Public methods.
//

//*****************************************************************************************
CMimic::CMimic(
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	: CPlayerDouble(M_MIMIC, pSetCurrentGame)
{ }

CMimic::CMimic(const UINT wSetType, CCurrentGame *pSetCurrentGame, UINT processingSequence)
	: CPlayerDouble(wSetType, pSetCurrentGame, GROUND_AND_SHALLOW_WATER, processingSequence)
{ }

//*****************************************************************************************
//Can this Mimic step onto (and thus kill) the player?
bool CMimic::CanStepAttackPlayer(const CSwordsman& player, const bool bStepAttack) const
{
	if (!((HasSword() && GetWeaponType() == WT_Dagger) || bStepAttack))
		return false;

	//Check if player is invulnerable to "dagger stepping".
	switch (player.wAppearance)
	{
		case M_WUBBA: case M_FEGUNDO:
		case M_ROCKGOLEM: case M_CONSTRUCT:
			return false;
		case M_CITIZEN: case M_ARCHITECT:
			if (!bStepAttack)
				return false;
		default:
			return true;
	}
}

//******************************************************************************************
bool CMimic::DoesSquareContainObstacle(
//Override for mimics.  Parts copied from CMonster::DoesSquareContainObstacle.
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

	const UINT wRole = this->pCurrentGame->pHold->getRoleForLogicalIdentity(GetIdentity());

	//Check o-square for obstacle.
	UINT wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		switch (wLookTileNo)
		{
			//If standing on a platform, check whether it can move.
			case T_PIT: case T_PIT_IMAGE:
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_P)
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			case T_WATER: /*case T_SHALLOW_WATER:*/
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_W)
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			default:
				if (bIsTunnel(wLookTileNo) && CanMoveOntoTunnelAt(wCol,wRow))
					break;
			return true;
		}
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

	//Can only move onto monsters when "dagger stepping".
	const bool bStepAttack = bCanEntityStepOnMonsters(wRole);
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
	{
		//Check if can "dagger step"
		if ((HasSword() && GetWeaponType() == WT_Dagger) || bStepAttack)
		{
			if (pMonster->IsPiece() || pMonster->IsLongMonster())
				return true;
			switch(pMonster->wType)
			{
				case M_WUBBA: case M_FEGUNDO:
				case M_ROCKGOLEM: case M_CONSTRUCT:
					return true;
				case M_CITIZEN: case M_ARCHITECT:
					if (!bStepAttack)
						return true;
					break;
				case M_CHARACTER:
				{
					const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
					if (pCharacter->IsInvulnerable())
						return true;
				}
				default: break;
			}
		}
		else if (!pMonster->IsPushableByBody() || !this->CanPushObjects()){
			return true;
		} else {
			const int dx = (int)wCol - (int)this->wX;
			const int dy = (int)wRow - (int)this->wY;
			if (!room.CanPushMonster(pMonster, wCol, wRow, wCol + dx, wRow + dy)){
				return true;
			}
		}
	}

	//Can only move onto the player when "dagger stepping".
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom() && player.wX == wCol && player.wY == wRow)
	{
		if (!CanStepAttackPlayer(player, bStepAttack))
			return true;
	}
	
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
void CMimic::Process(
//Process a mimic for movement.
//
//Params:
	const int nLastCommand, //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	//Only move if not stunned
	if (!IsStunned())
	{
		int dx, dy;
		InitiateMimicMovement(nLastCommand, dx, dy);

		const UINT wMovementO = nGetO(dx, dy);

		if (this->bFrozen) //frozen by aumtlich gaze?
			dx = dy = 0;

		// Did the nLastCommand successfully move the swordsman.
		// If not, then set dx/dy to zero so mimics don't move
		if (this->pCurrentGame->IsPlayerAt(this->pCurrentGame->swordsman.wPrevX,
				this->pCurrentGame->swordsman.wPrevY))
		{
			dx=dy=0;
			this->wPrevX = this->wX;
			this->wPrevY = this->wY;

			if ((!HasSword() || GetWeaponType() == WT_Dagger) && wMovementO != NO_ORIENTATION) //they can face this direction when swordless
				this->wO = wMovementO;

			//Player bumped into something, so mimics can "bump" into something in this
			//direction too, i.e. shatter a mirror in this direction if their sword is on it.
			this->wSwordMovement = CSwordsman::GetSwordMovement(nLastCommand, this->wO);
			if (nLastCommand != CMD_C && nLastCommand != CMD_CC && this->wSwordMovement != this->wO) //if not a turning or bumping movement
				this->wSwordMovement = NO_ORIENTATION;
		} else {
			GetBestMove(dx, dy);
			ApplyMimicMove(dx, dy, nLastCommand, wMovementO, CueEvents);
		}
	}

	//Light any fuse stood on.
	room.LightFuseEnd(CueEvents, this->wX, this->wY);
}

//*****************************************************************************************
void CMimic::InitiateMimicMovement(int nCommand, int& dx, int& dy)
{
	GetCommandDXY(nCommand, dx, dy);

	switch (nCommand)
	{
		case CMD_C:
			this->wO = nNextCO(this->wO);
			break;
		case CMD_CC:
			this->wO = nNextCCO(this->wO);
			break;
		default: break;
	}
}

//*****************************************************************************************
void CMimic::ApplyMimicMove(int dx, int dy, int nCommand, const UINT wMovementO,
	CCueEvents &CueEvents, //(in/out)
	bool bEnteredTunnel)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	this->wSwordMovement = nGetO(dx, dy);
	const bool bBumpCommand = bIsBumpCommand(nCommand);
	const UINT wRole = this->pCurrentGame->pHold->getRoleForLogicalIdentity(GetIdentity());

	//Check for obstacles in destination square.
	if (!bBumpCommand && (dx || dy))
	{
		ASSERT(nCommand != CMD_C && nCommand != CMD_CC && nCommand != CMD_WAIT
				&& !bIsSpecialCommand(nCommand));

		//Before he moves, remember important square contents.
		const UINT wOTile = room.GetOSquare(this->wX, this->wY);
		const bool bWasOnPlatform = bIsPlatform(wOTile);

		//Move mimic to new destination square.
		if (bEnteredTunnel) {
			UINT destX, destY;
			if (!this->pCurrentGame->TunnelGetExit(this->wX, this->wY, dx, dy, destX, destY, this)) {
				ASSERT(!"Should not set entered tunnel without valid tunnel exit");
				this->wPrevX = this->wX;
				this->wPrevY = this->wY;
				return;
			}
			Move(destX, destY, &CueEvents);
		} else {
			const UINT wDestX = this->wX + dx;
			const UINT wDestY = this->wY + dy;
			//Process "dagger stepping" first if appropriate
			if ((HasSword() && GetWeaponType() == WT_Dagger) || bCanEntityStepOnMonsters(wRole))
			{
				CMonster *pMonster = room.GetMonsterAtSquare(wDestX,wDestY);
				if (pMonster && pMonster->GetIdentity() != M_FLUFFBABY)
				{
					ASSERT(!pMonster->IsPiece() && !pMonster->IsLongMonster());
					CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
					room.KillMonster(pMonster, CueEvents, false, this); //will return false if it's a critical NPC
					pMonster->SetKillInfo(nGetO(dx,dy));
					room.RemoveMonsterFromTileArray(pMonster);
					if (this->wType == M_TEMPORALCLONE && pMonster->GetIdentity() != M_FLUFFBABY && !pMonster->IsFriendly())
					{
						CTemporalClone *pClone = DYN_CAST(CTemporalClone*, CMimic*, this);
						pClone->bIsTarget = true;
					}
				}
				const CSwordsman& player = this->pCurrentGame->swordsman;
				if (player.IsInRoom() && player.wX == wDestX && player.wY == wDestY)
				{
					CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
					pGame->SetDyingEntity(&player, this);
					CueEvents.Add(CID_MonsterKilledPlayer, this);
				}
			}
			Move(wDestX,wDestY,&CueEvents);
		}

		//face the direction moved when w/o sword, or with dagger
		if (!HasSword() || GetWeaponType() == WT_Dagger)
			this->wPrevO = this->wO = nGetO(dx,dy);
		ASSERT(IsValidOrientation(this->wO));

		if (!IsFlying())
		{
			//Check for movement off of a trapdoor.
			ASSERT(dx || dy);
			if (CanDropTrapdoor(wOTile))
				room.DestroyTrapdoor(this->wX - dx, this->wY - dy, CueEvents);

			//Check for platform movement.
			if (bWasOnPlatform)
			{
				const UINT wOTile = room.GetOSquare(this->wX, this->wY);
				if (bIsPit(wOTile) || bIsDeepWater(wOTile))
					room.MovePlatform(this->wX - dx, this->wY - dy, nGetO(dx,dy));
			}

			//If another monster was pushed, the destination tile may have fallen
			if (this->bPushedOtherMonster)
				room.CheckForFallingAt(this->wX, this->wY, CueEvents);
		}

		//Check for movement onto a checkpoint.
		this->pCurrentGame->QueryCheckpoint(CueEvents, this->wX, this->wY);

		//Process any and all of these item interactions.
		UINT tTile = room.GetTSquare(this->wX, this->wY);
		if (bIsTLayerCoveringItem(tTile))
		{
			room.PushTLayerObject(this->wX, this->wY, this->wX + dx, this->wY + dy, CueEvents);
			tTile = room.GetTSquare(this->wX, this->wY); //also check what was under the item
		}
		room.ActivateToken(CueEvents, this->wX, this->wY, this);
	} else {
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;

		if (bBumpCommand && this->wType == M_TEMPORALCLONE)
		{
			const UINT wDestX = this->wX + dx;
			const UINT wDestY = this->wY + dy;
			
			if (room.IsValidColRow(wDestX, wDestY))
			{
				//Find out if we can affect the next square by figuring out exactly what stopped our movement
				const UINT wOTile = room.GetOSquare(this->wX, this->wY);
				const UINT wNewOTile = room.GetOSquare(wDestX, wDestY);
				const UINT wNewTTile = room.GetTSquare(wDestX, wDestY);

				bool bBumpOrb = true;

				if (this->bFrozen
						|| DoesArrowPreventMovement(this->wX, this->wY, dx, dy)
						|| room.DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy)
						|| (CanEnterTunnel() && room.IsTunnelTraversableInDirection(this->wX, this->wY, dx, dy)))
					bBumpOrb = false;
				if (bBumpOrb)
				{
					switch(wNewOTile)
					{
						case T_PIT: case T_PIT_IMAGE:
							//Flying roles do not consider pit an obstacle
							if (!bIsEntityFlying(wRole))
							//If standing on a platform, check whether it can move.
								if (wOTile != T_PLATFORM_P || !room.CanMovePlatform(this->wX, this->wY, nGetO(dx, dy)))
									bBumpOrb = false;
						break;
						case T_SHALLOW_WATER:
							//Flying/Swimming/Wading roles do not consider shallow water an obstacle
							if (!(bIsEntityFlying(wRole) || bIsEntitySwimming(wRole) ||
									this->pCurrentGame->swordsman.GetWaterTraversalState(wRole) >= WTrv_CanWade))
								bBumpOrb = false;
						break;
						case T_WATER:
							//Flying/Swimming roles do not consider water an obstacle
							if (!(bIsEntityFlying(wRole) || bIsEntitySwimming(wRole)))
								if (wOTile != T_PLATFORM_W || !room.CanMovePlatform(this->wX, this->wY, nGetO(dx, dy)))
									bBumpOrb = false;
						break;
						case T_DOOR_Y:
							//Temporal Clones cannot currently direct Halph
							bBumpOrb = false;
						break;
						case T_WALL_M:
							if (!this->pCurrentGame->bHoldMastered)
								bBumpOrb = false;
						break;
						case T_WALL_WIN:
							if (!this->pCurrentGame->bHoldCompleted)
								bBumpOrb = false;
						break;
						default:
						break;
					}
				}

				if (bBumpOrb)
				{
					switch(wNewTTile)
					{
						case T_ORB:
						case T_BEACON: case T_BEACON_OFF:
							if ((bIsHuman(wRole) && !bEntityHasSword(wRole)) ||
								this->pCurrentGame->swordsman.bCanGetItems) //power token allows this too
							{
								if (wNewTTile == T_ORB)
								{
									room.ActivateOrb(wDestX, wDestY, CueEvents, OAT_Monster);
								} else {
									room.ActivateBeacon(wDestX, wDestY, CueEvents);
								}
							}
							break;

						default: break;
					}

					CMonster *pMonster = room.GetMonsterAtSquare(wDestX, wDestY);
					if (pMonster != NULL && pMonster->wType == M_CHARACTER){
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
						pCharacter->bPlayerTouchedMe = true;
					}
				}
			}
		}

		//face the initial movement direction when w/o sword, or with dagger
		if ((!HasSword() || GetWeaponType() == WT_Dagger) && wMovementO != NO_ORIENTATION)
			this->wO = wMovementO;

		//Mimics can smash mirrors by bumping into them even when they don't move.
		this->wSwordMovement = CSwordsman::GetSwordMovement(nCommand, this->wO);
		if (nCommand != CMD_C && nCommand != CMD_CC && this->wSwordMovement != this->wO)
			this->wSwordMovement = NO_ORIENTATION;
	}
}
