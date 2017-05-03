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
 * Michael Welsh Duggan (md5i)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//RoachQueen.cpp
//Implementation of CRoachQueen.

#include "RoachQueen.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
void CRoachQueen::Process(
//Process a roach queen for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Don't do anything if was stunned this turn
	if (this->bNewStun)
		return;

	// Save previous location
	const UINT oX = this->wX;
	const UINT oY = this->wY;

	//Only move if not stunned
	if (!IsStunned())
	{
		//Get movement offsets.
		int dxFirst, dyFirst, dx, dy;
		if (this->pCurrentGame->bBrainSensesSwordsman)
			GetBrainDirectedMovement(dxFirst, dyFirst, dx, dy);
		else
		{
			//Roach queens are scared of the player and run away from him.
			//Find where to run away from.
			UINT wSX, wSY;
			if (!GetTarget(wSX,wSY))
				return;	//no change -- and don't lay eggs

			// Run away!
			dx = dxFirst = -sgn(wSX - this->wX);
			dy = dyFirst = -sgn(wSY - this->wY);
			GetBestMove(dx, dy);
		}

		//Move roach queen to new destination square.
		MakeStandardMove(CueEvents,dx,dy);
		SetOrientation(dxFirst, dyFirst);
	}

	//Shall we lay some eggs?
	//The criteria for laying an egg in a square should be:
	//1. Square does not contain a monster (including mimic).
	//2. Square does not contain a swordsman or sword.
	//3. Square does not contain a mimic sword.
	//4. T-square is mostly empty (backwards compatibility).
	//5. O-square is open floor/door (except for open yellow doors -- for backwards compatibility).
	//6. Swordsman is still sensed at the new square moved to (backwards compatibility).
	if (this->pCurrentGame->wSpawnCycleCount % TURNS_PER_CYCLE == 0 &&
		(this->pCurrentGame->bBrainSensesSwordsman || CanFindSwordsman()))
	{
		CCoordSet eggs;
		CDbRoom& room = *(this->pCurrentGame->pRoom);
		for (int y = -1; y <= 1; ++y)
		{
			for (int x = -1; x <= 1; ++x)
			{
				const UINT ex = this->wX + x;
				const UINT ey = this->wY + y;
				if (room.IsValidColRow(ex, ey))
				{
					const UINT wOSquare = room.GetOSquare(ex, ey);
					const UINT wTSquare = room.GetTSquare(ex, ey);
					CMonster* pMonster = room.GetMonsterAtSquare(ex, ey);
					if (
						// Not current queen position
						!(ex == this->wX && ey == this->wY) &&
						// Not old queen position (compat)
						!(ex == oX && ey == oY) &&
						// Not the player
						!this->pCurrentGame->IsPlayerAt(ex, ey) &&
						// Not monster or player double or sword
						!pMonster && !DoesSquareContainObstacle(ex, ey) &&
						//And t-square is mostly empty (compat).
						(wTSquare == T_EMPTY || wTSquare == T_FUSE) &&
						!bIsArrow(room.GetFSquare(ex, ey)) &&
						//And o-square is floor and not open yellow door (compat).
						((bIsPlainFloor(wOSquare) || wOSquare == T_PRESSPLATE) ||
								(bIsOpenDoor(wOSquare) && wOSquare != T_DOOR_YO) ||
								bIsPlatform(wOSquare) || bIsBridge(wOSquare) ||
								wOSquare == T_GOO || wOSquare == T_FLUFFVENT)
						)
					{
						// Place an egg
						CMonster *m = room.AddNewMonster(M_REGG, ex,ey);
						m->bIsFirstTurn = true;
						eggs.insert(ex,ey);

						CueEvents.Add(CID_EggSpawned);
					}
				}
			}
		}

		//Once eggs have been laid, check for them being laid on pressure plates.
		for (CCoordSet::const_iterator egg=eggs.begin(); egg!=eggs.end(); ++egg)
		{
			if (room.GetOSquare(egg->wX, egg->wY) == T_PRESSPLATE)
				room.ActivateOrb(egg->wX, egg->wY, CueEvents, OAT_PressurePlate);
		}
	}
}

//******************************************************************************************
bool CRoachQueen::GetBrainDirectedMovement(
// Special-cased for roach queens.
//
//Params:
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy,    //(out) Vertical delta (-1, 0, or 1) for same.
	const MovementIQ /*movementIQ*/)   //(in) [default = SmartDiagonalOnly]
const
{
	//Roach queen tries to run opposite the direction brain suggests to go.
	//ATTN: This heuristic is scheduled for change in 3.0.
	SORTPOINTS paths;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wTSquare = room.GetTSquare(this->wX, this->wY);
	switch (wTSquare)
	{
		case T_POTION_I: case T_POTION_K:
		case T_SCROLL:
			break;	//1.6 compatibility: queen beelines when on t-layer objects
		default:
			if (bIsArrow(room.GetFSquare(this->wX, this->wY)))
				break; //1.6 compatibility: queen beelines when on arrows
			ASSERT(room.pPathMap[this->eMovement]);
			room.pPathMap[this->eMovement]->GetRecPaths(this->wX, this->wY, paths);
			break;
	}

	if (paths.empty())
	{
		//Just beeline away from player if no brained option is available.
		//With a brain, even invisible swordsman is sensed.
		UINT wSX, wSY;
		this->pCurrentGame->GetSwordsman(wSX, wSY);
		dx = dxFirst = -sgn(wSX - this->wX);
		dy = dyFirst = -sgn(wSY - this->wY);
	} else {
		dx = this->wX - paths[0].wX;	//run away using first suggestion
		dy = this->wY - paths[0].wY;
		ASSERT(abs(dx) <= 1);
		ASSERT(abs(dy) <= 1);

		//Try to move diagonally toward middle of the room on non-primary axis.
		if (dx == 0)
			dx = (wX < (room.wRoomCols / 2)) ? 1 : -1;
		else if (dy == 0)
			dy = (wY < (room.wRoomRows / 2)) ? 1 : -1;

		dxFirst = dx;
		dyFirst = dy;
	}
	GetBestMove(dx, dy);
	return true;
}
