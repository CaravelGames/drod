// $Id: Mimic.cpp 9413 2010-03-27 15:48:40Z mrimer $

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
#include "Swordsman.h"
#include "Character.h"
#include "Combat.h"

//
//Public methods.
//

//*****************************************************************************************
CMimic::CMimic(
//Constructor mimic.
//
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	: CPlayerDouble(M_MIMIC, pSetCurrentGame)
{ }

//******************************************************************************************
bool CMimic::CanDropTrapdoor(const UINT wTileNo)
{
	if (!bIsFallingTile(wTileNo))
		return false;

	if (bIsThinIce(wTileNo))
		return true;

	return HasSword();
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
			case T_WATER:
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_W)
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			default:	return true;
		}
	}

	//Check t-square for obstacle.
	wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		switch (wLookTileNo)
		{
			case T_MIRROR:
			case T_CRATE:
			case T_POWDER_KEG:
			{
				const int dx = (int)wCol - (int)this->wX;
				const int dy = (int)wRow - (int)this->wY;
				if (room.CanPushTo(wCol, wRow, wCol + dx, wRow + dy))
					break; //not an obstacle
			}
			//NO BREAK
			default: return true;
		}
	}

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Can never move onto a monster.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
		return true;

	//Can't move onto player.
	if (this->pCurrentGame->pPlayer->wX == wCol &&
			this->pCurrentGame->pPlayer->wY == wRow)
		return true;
	
	//Check for player's sword at square.
	if (this->pCurrentGame->IsPlayerSwordAt(wCol, wRow))
		return true;

	//Check for monster sword at square.
	if (room.IsMonsterSwordAt(wCol, wRow, this))
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
	int dx=0, dy=0;

	switch (nLastCommand)
	{
		case CMD_C:
		{
			this->wO = nNextCO(this->wO);
			break;
		}
		case CMD_CC:
		{
			this->wO = nNextCCO(this->wO);
			break;
		}
		case CMD_NW: dx = dy = -1; break;
		case CMD_N: dy = -1; break;
		case CMD_NE: dx = 1; dy = -1; break;
		case CMD_W: dx = -1; break;
		case CMD_E: dx = 1; break;
		case CMD_SW: dx = -1; dy = 1; break;
		case CMD_S: dx = 0; dy = 1; break;
		case CMD_SE: dx = dy = 1; break;
	}

	const UINT wMovementO = nGetO(dx, dy);

//	if (this->bFrozen) //frozen by aumtlich gaze?
//		dx = dy = 0;

	//If player has engaged this mimic, it doesn't move this turn.
	if (this->pCurrentGame->IsFighting(this))
		dx = dy = 0;

	CDbRoom& room = *(this->pCurrentGame->pRoom);

	// Did the nLastCommand successfully move the player.
	// If not, then set dx/dy to zero so mimics don't move
	if (this->pCurrentGame->IsPlayerAt(this->pCurrentGame->pPlayer->wPrevX,
			this->pCurrentGame->pPlayer->wPrevY))
	{
		dx=dy=0;
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
		this->wPrevO = this->wO;

		//Player bumped into something, so mimics can "bump" into something in this
		//direction too, i.e. shatter a mirror in this direction if their sword is on it.
		this->wSwordMovement = CSwordsman::GetSwordMovement(nLastCommand, this->wO);
		if (nLastCommand != CMD_C && nLastCommand != CMD_CC && this->wSwordMovement != this->wO) //if not a turning or bumping movement
			this->wSwordMovement = NO_ORIENTATION;
		if (!HasSword() && wMovementO != NO_ORIENTATION) //they can face this direction when swordless
			this->wO = wMovementO;
	} else {
		GetBestMove(dx, dy);
		this->wSwordMovement = nGetO(dx, dy);

		//Check for obstacles in destination square.
		if (dx || dy)
		{
			ASSERT(nLastCommand != CMD_C && nLastCommand != CMD_CC && nLastCommand != CMD_WAIT);

			//Before he moves, remember important square contents.
			const UINT wOTile = room.GetOSquare(this->wX, this->wY);
			const bool bWasOnTrapdoor = bIsFallingTile(wOTile);
			const bool bWasOnPlatform = bIsPlatform(wOTile);

			//Move mimic to new destination square.
			Move(this->wX + dx, this->wY + dy, &CueEvents);

			if (!HasSword())
				this->wO = nGetO(dx,dy); //face the direction moved when w/o sword
			ASSERT(IsValidOrientation(this->wO));

			//Check for movement off of a trapdoor.
			ASSERT(dx || dy);
			if (bWasOnTrapdoor && CanDropTrapdoor(wOTile))
				room.DestroyTrapdoor(this->wX - dx, this->wY - dy, CueEvents);

			//Check for platform movement.
			if (bWasOnPlatform)
			{
				const UINT wOTile = room.GetOSquare(this->wX, this->wY);
				if (bIsPit(wOTile) || wOTile == T_WATER)
					room.MovePlatform(this->wX - dx, this->wY - dy, nGetO(dx,dy));
			}

/*
			//Check for movement onto a checkpoint.
			this->pCurrentGame->QueryCheckpoint(CueEvents, this->wX, this->wY);
*/
			//Process any and all of these item interactions.
			UINT tTile = room.GetTSquare(this->wX, this->wY);
			if (tTile==T_MIRROR || tTile==T_CRATE || tTile==T_POWDER_KEG)
			{
				room.PushObject(this->wX, this->wY, this->wX + dx, this->wY + dy, CueEvents);
				tTile = room.GetTSquare(this->wX, this->wY); //also check what was under the object
			}
			if (tTile==T_TOKEN)
				const_cast<CCurrentGame*>(this->pCurrentGame)->ActivateTokenAt(this->wX, this->wY);
		} else {
			this->wPrevX = this->wX;
			this->wPrevY = this->wY;
			this->wPrevO = this->wO;

			//Mimics can smash mirrors by bumping into them even when they don't move.
			this->wSwordMovement = CSwordsman::GetSwordMovement(nLastCommand, this->wO);
			if (nLastCommand != CMD_C && nLastCommand != CMD_CC && this->wSwordMovement != this->wO)
				this->wSwordMovement = NO_ORIENTATION;
			if (!HasSword() && wMovementO != NO_ORIENTATION) //they can face this direction when swordless
				this->wO = wMovementO;
		}
	}
	//Light any fuse stood on.
	room.LightFuse(CueEvents, this->wX, this->wY, true);
}
