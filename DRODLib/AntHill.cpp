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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//AntHill.cpp
//Implementation of CAntHill.

#include "AntHill.h"

#include "CurrentGame.h"
#include "DbRooms.h"

#define ANT_SPAWN_SPEED (10)

//*****************************************************************************************
void CAntHill::Process(
//Process an ant hill (to release ants).
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &/*CueEvents*/) //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Change graphic when about to spawn.
	this->wO = (pCurrentGame->wSpawnCycleCount % ANT_SPAWN_SPEED == ANT_SPAWN_SPEED-1) ?
			W : NO_ORIENTATION;

	//Don't do anything if was stunned this turn
	if (this->bNewStun)
		return;

	UINT wTX, wTY;
	if (!GetTarget(wTX, wTY))
		return;

	//Shall we release some ants?
	//The criteria for releasing an ant in a square should be:
	//1. O-square is open.
	//2. Tile does not contain the player.
	//3. Tile does not contain another obstacle (monster, sword).
	//4. T-square is mostly empty.
	static const UINT infinity = 1000000;
	float fBestDistance = (float)infinity;
	UINT wBestX = 0, wBestY = 0;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (this->pCurrentGame->wSpawnCycleCount % ANT_SPAWN_SPEED == 0)
		for (int y = -1; y <= 1; ++y)
			for (int x = -1; x <= 1; ++x)
			{
				if (!x && !y) continue;
				const UINT ex = this->wX + x;
				const UINT ey = this->wY + y;
				if (!room.IsValidColRow(ex, ey)) continue;

				const UINT wOSquare = room.GetOSquare(ex, ey);
				const UINT wTSquare = room.GetTSquare(ex, ey);
				CMonster* pMonster = room.GetMonsterAtSquare(ex, ey);
				if (
					//Is o-square open?
					bIsWater(wOSquare) &&
					//Not player
					!this->pCurrentGame->IsPlayerAt(ex, ey) &&
					//Not monster or mimic or sword
					!pMonster && !DoesSquareContainObstacle(ex, ey) &&
					//And t-square is mostly empty.
					(wTSquare == T_EMPTY || wTSquare == T_FUSE || wTSquare == T_SCROLL ||
					 wTSquare == T_TOKEN)
					)
				{
					const float fDist = DistanceToTarget(wTX,wTY,ex,ey);
					if (fDist < fBestDistance)
					{
						fBestDistance = fDist;
						wBestX = ex;
						wBestY = ey;
					}
				}
			}
	if (fBestDistance < infinity)
	{
		//Place an ant.
		ASSERT(fBestDistance > 0);
		CMonster *m = room.AddNewMonster(M_WATERSKIPPER, wBestX, wBestY);
		m->bIsFirstTurn = true;
		m->SetOrientation(sgn((int)wTX - (int)wBestX), sgn((int)wTY - (int)wBestY));
	}
}
