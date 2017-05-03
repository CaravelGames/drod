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
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "BuildUtil.h"
#include "Citizen.h"
#include "Station.h"
#include "CurrentGame.h"
#include "DbRooms.h"

const UINT STUCK_THRESHOLD = 10; //# of turns a citizen waits in a jam to get unstuck

//******************************************************************************
CCitizen::CCitizen(CCurrentGame *pSetCurrentGame,
		UINT monsterType,
		UINT processSequence)
	: CMonster(monsterType, pSetCurrentGame, GROUND_AND_SHALLOW_WATER, processSequence)
	, bDone(false)
	, bHasSupply(false)
	, nStationType(-1)
	, nVisitingStation(-1)
	, wTurnsStuck(0)
{
}

//*****************************************************************************
bool CCitizen::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	const bool bOnHotTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT;
	const bool bMoved = this->wX != this->wPrevX || this->wY != this->wPrevY;

	if ((!this->bWaitedOnHotFloorLastTurn && this->pCurrentGame->swordsman.bIsHasted) || bMoved){
		this->bWaitedOnHotFloorLastTurn = bOnHotTile && !bMoved;
		return false;
	}

	if (bOnHotTile)
	{
		//Damaged, even though sword hits do not affect citizens.
		CueEvents.Add(CID_MonsterBurned, this);
		return true;
	}
	return false;
}

//******************************************************************************************
bool CCitizen::OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/, const UINT /*wY*/, WeaponType weaponType)
{
	//Firetraps kill citizens.
	if (weaponType == WT_Firetrap)
	{
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		return true;
	}

	//Stabs don't kill citizens.
	return false;
}

//******************************************************************************************
bool CCitizen::DoesSquareContainObstacle(
//Override for citizen.  (Copied and revised code from CMonster::DoesSquareContainObstacle.)
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	ASSERT(this->pCurrentGame);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow))
		return true;

	//Don't consider moving onto same tile as a valid move.
	if (wCol == this->wX && wRow == this->wY)
		return true;

	//No monsters can ever be stepped on.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
		return true;

	//Some layer objects are obstacles.  Check for these.
	if (IsTileObstacle(room.GetOSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetTSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Can't step on any swords.
	if (this->swordsInRoom.Exists(wCol, wRow)) //this set is compiled at beginning of move
		return true;

	//Player can never be stepped on.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//*****************************************************************************
bool CCitizen::IsTileObstacle(
//Override for citizens.
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
			//Citizens don't like active firetraps
			wLookTileNo == T_FIRETRAP_ON ||
			CMonster::IsTileObstacle(wLookTileNo)
			);
}

//******************************************************************************
bool CCitizen::GetGoal(UINT& wX, UINT& wY) const
//Call to query the citizen's current (x,y) destination
//
//Returns: whether citizen has a current (x,y) goal
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (HasSupply() && !room.building.empty())
	{
		const bool bGoalIsCurrent = room.building.get(this->goal.wX, this->goal.wY) != 0;
		if (!bGoalIsCurrent)
			return false;

		//Going to build at this square.
		wX = this->goal.wX;
		wY = this->goal.wY;
		return true;
	}

	if (this->bDone)
		return false; //nowhere (else) to go

	if (this->nVisitingStation < 0)
		return false; //no station selected
	if (static_cast<UINT>(this->nVisitingStation) >= this->visitingSequence.size())
		return false; //route ended

	//Going to this station.
	const int currentStationIndex = this->visitingSequence[this->nVisitingStation];
	CStation *pStation = room.stations[currentStationIndex];
	wX = pStation->X();
	wY = pStation->Y();
	return true;
}

//******************************************************************************
void CCitizen::Process(
//Process a citizen for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
		//with codes indicating events that happened that may correspond to
		//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	//Init.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (room.stations.empty())
		this->bDone = true;

	if (HasSupply() && !room.building.empty())
	{
		//Citizen is carrying supplies and there is a build request.
		this->bDone = false;

		//Find path to closest build marker.
		if (CalcPathAndAdvance(CueEvents))
			return;
	}

	if (!this->bDone)
		GotoStation(CueEvents);
}

//******************************************************************************
//Returns: whether the turn was taken
bool CCitizen::CalcPathAndAdvance(CCueEvents& CueEvents)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	ASSERT(room.IsValidColRow(this->goal.wX, this->goal.wY));

	//If a valid goal is one tile away, then the citizen may build there now.
	//Otherwise, confirm a path to the goal is (still) accessible.
	const bool bGoalIsCurrent = room.building.get(this->goal.wX, this->goal.wY) != 0;
	if (!bGoalIsCurrent ||
			nDist(this->wX, this->wY, this->goal.wX, this->goal.wY) != 1)
	{
		room.GetSwordCoords(this->swordsInRoom, true); //speed optimization
		if (!bGoalIsCurrent || !ConfirmPath())
		{
			//If it's not, search for a (new) path to the goal.
			this->pathToDest.Clear();

			ASSERT(!room.building.tileSet.empty());
			if (!room.building.tileSet.has(this->wX, this->wY))
			{
				//Must find a path to a goal.
				if (!FindOptimalPathTo(this->wX, this->wY, room.building.tileSet, true))
					this->bHasSupply = false; //No path -- dump supply and go to next station
			} else {
				//A goal is located on this tile.
				this->goal.wX = this->wX;
				this->goal.wY = this->wY;

				//Builder would normally try to build on his own tile, getting stuck
				//as a result.  Therefore, take a step off it to build on it next turn.
				//First, try to step backwards, then forwards.
				//If neither work, then dump supplies and resume station route.
				int dxFirst, dyFirst, dx, dy;
				GetBeelineMovementSmart(this->wX - nGetOX(this->wO), this->wY - nGetOY(this->wO),
						dxFirst, dyFirst, dx, dy, true);
				if (!dx && !dy)
					GetBeelineMovementSmart(this->wX + nGetOX(this->wO), this->wY + nGetOY(this->wO),
							dxFirst, dyFirst, dx, dy, true);
				if (dx || dy)
				{
					SetOrientation(dx, dy);
					MakeStandardMove(CueEvents, dx, dy);
					return true;
				}

				this->bHasSupply = false;
			}
		}

		//Take a step toward the closest goal.
		if (TakeStepTowardsGoal(CueEvents))
			return true;
	}

	//Citizen has reached build marker.
	if (HasSupply())
	{
		BuildTile(CueEvents);
		return true;
	}

	//citizen has no valid path to a build marker -- go to next station
	return false;
}

void CCitizen::BuildTile(CCueEvents& CueEvents)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	ASSERT(room.IsValidColRow(this->goal.wX, this->goal.wY));
	const int dx = this->goal.wX - this->wX;
	const int dy = this->goal.wY - this->wY;

	if (dx || dy)
		SetOrientation(dx, dy); //face build spot

	//Check for vacancy.
	UINT wBuildTile = room.building.get(this->goal.wX, this->goal.wY) - 1; //1-based
	if (bIsWall(wBuildTile) || bIsCrumblyWall(wBuildTile))
	{
		//If the build tile would fill a square, the tile must be vacant now.
		CMonster *pMonster = room.GetMonsterAtSquare(this->goal.wX, this->goal.wY);
		if ((pMonster && pMonster->wType != M_FLUFFBABY) ||
				(this->pCurrentGame->swordsman.wX == this->goal.wX &&
				this->pCurrentGame->swordsman.wY == this->goal.wY))
		{
			//Build tile is occupied -- wait until vacant.
			return;
		}
	}

	if (BuildUtil::BuildTileAt(room, wBuildTile, this->goal.wX, this->goal.wY, true, CueEvents)){
		this->bHasSupply = false;
	}
}

//******************************************************************************
void CCitizen::GotoStation(CCueEvents &CueEvents)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	vector<CStation*>& stations = room.stations;

	//Mark moving obstacles correctly on station path maps for all citizens.
	const UINT wNumStations = stations.size();
	if (stations[0]->UpdateTurn(this->pCurrentGame->wTurnNo))
	{
		for (UINT n=1; n<wNumStations; ++n)
			stations[n]->UpdateTurn(this->pCurrentGame->wTurnNo);
	}

	//If route is completed, try to find a new station to go to.
	if (this->nVisitingStation >= (int)this->visitingSequence.size())
		this->nVisitingStation = -1;

	//If ready, choose a station to move to.
	if (this->nVisitingStation == -1)
	{
		//Find closest unvisited station.
		UINT wDist, wBestDistance = (UINT)-1, wBestIndex = 0;

		UINT n=wNumStations;
		while (n--)
		{
			if (this->nStationType >= 0 && (UINT)this->nStationType != stations[n]->GetType())
				continue; //not visiting stations of this set
			if (sequenceOfStation(n) >= 0)
				continue; //station is already in route -- doesn't need to be added again
			wDist = stations[n]->GetDistanceFrom(this->wX, this->wY);
			if (!wDist) continue;
			if (wDist < wBestDistance || (wDist == wBestDistance &&
					l2DistanceToStationSq(n) < l2DistanceToStationSq(wBestIndex)))
			{
				wBestDistance = wDist;
				wBestIndex = n;
			}
		}

		if (wBestDistance == (UINT)-1)
		{
			//No new station was accessible.
			if (this->visitingSequence.empty())
				return; //search for one next turn
			this->nVisitingStation = 0; //Begin route again from the beginning.
		} else {
			//Extend route -- go visit new station.
			ASSERT(wBestIndex < wNumStations);
			this->nVisitingStation = this->visitingSequence.size();
			this->visitingSequence.push_back(wBestIndex);

			//Only ever visit stations of the set of the first station visited.
			if (!this->nVisitingStation)
				this->nStationType = stations[wBestIndex]->GetType();
		}
	}

	ASSERT(this->nVisitingStation >= 0);
	ASSERT((UINT)this->nVisitingStation < this->visitingSequence.size());
	CStation *pStation = stations[this->visitingSequence[this->nVisitingStation]];

	//Has this station been reached?
	const UINT wStationX = pStation->X(), wStationY = pStation->Y();
	const bool bStationExists = this->pCurrentGame->pRoom->GetTSquare(
			wStationX, wStationY) == T_STATION;
	const UINT wDist = bStationExists ? nDist(this->wX,this->wY,wStationX,wStationY) : 0;
	if (wDist <= 1)
	{
		//Station has been reached.
		if (wDist == 1)
			SetOrientation(wStationX-this->wX, wStationY-this->wY);
		this->bHasSupply = bStationExists;
		this->wTurnsStuck = 0;

		//Go to next station in sequence.
		++this->nVisitingStation;
		if (wNumStations == 1)
			this->bDone = true; //reached the only destination -- nowhere else to go
		return; //don't do anything else this turn
	}

	//Take a step toward station to be visited.
	const UINT wDir = pStation->GetDirectionFrom(this->wX, this->wY);
	if (wDir != NO_ORIENTATION)
	{
		const int dx = nGetOX(wDir), dy = nGetOY(wDir);
		MakeStandardMove(CueEvents,dx,dy);
		SetOrientation(dx, dy);
		this->wTurnsStuck = 0;
	} else {
		//Stuck -- face toward goal.
		SetOrientation(sgn(wStationX - this->wX), sgn(wStationY - this->wY));

		//Traffic jam inhibitor:
		//If this citizen has been stuck for min(X,ST) turns,
		//where X is the distance to the station and ST is STUCK_THRESHOLD,
		//then mark this station as visited and choose the next one.
		//This occurs except when "persistent citizen movement" is set.
		UINT wDistToStation = pStation->GetDistanceFrom(this->wX, this->wY);
		if (wDistToStation > STUCK_THRESHOLD)
			wDistToStation = STUCK_THRESHOLD;
		if (++this->wTurnsStuck > wDistToStation &&
				!this->pCurrentGame->pRoom->bPersistentCitizenMovement)
		{
			++this->nVisitingStation;
			this->wTurnsStuck = STUCK_THRESHOLD/2; //wait less if still stuck
		}
	}
}

//******************************************************************************
bool CCitizen::TakeStepTowardsGoal(CCueEvents &CueEvents)
{
	if (!this->pathToDest.GetSize())
		return false;

	UINT wNextX, wNextY;
	this->pathToDest.Top(wNextX, wNextY);
	ASSERT(this->pCurrentGame->swordsman.wX != wNextX ||
			this->pCurrentGame->swordsman.wY != wNextY); //never step on player

	const int dx = wNextX - this->wX;
	const int dy = wNextY - this->wY;
	Move(wNextX, wNextY, &CueEvents);
	this->pathToDest.Pop();  //citizen has now made this move
	SetOrientation(dx, dy);
	return true;
}

//******************************************************************************
float CCitizen::l2DistanceToStationSq(const UINT wStationIndex) const
//Returns: Euclidean distance to station, squared
{
	const vector<CStation*>& stations = this->pCurrentGame->pRoom->stations;
	ASSERT(wStationIndex < stations.size());
	CStation& station = *(stations[wStationIndex]);
	const int dx = this->wX - station.X();
	const int dy = this->wY - station.Y();
	return static_cast<float>(dx * dx + dy * dy);
}

//******************************************************************************
int CCitizen::sequenceOfStation(const UINT wStationIndex) const
//Returns: sequence index of specified station, or -1 if not in sequence.
{
	int nIndex=0;
	for (vector<int>::const_iterator index = this->visitingSequence.begin();
			index != this->visitingSequence.end(); ++index, ++nIndex)
		if (*index == (int)wStationIndex)
			return nIndex;

	//Station not found in station sequence.
	return -1;
}
