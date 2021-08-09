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

#include "Station.h"
#include "DbRooms.h"
#include "CurrentGame.h"

UINT CStation::wLastTurnInit = (UINT)-1;
CCoordIndex CStation::swords;

static const UINT wNumNeighbors = 8;
static const int dxDir[wNumNeighbors] = { 0, 1, 0,-1, 1, 1,-1,-1};
static const int dyDir[wNumNeighbors] = {-1, 0, 1, 0,-1, 1, 1,-1};

//******************************************************************************
CStation::CStation(const UINT wX, const UINT wY, CDbRoom* const pRoom)
	: pRoom(pRoom), wX(wX), wY(wY), bRecalcPathmap(false)
{
	ASSERT(pRoom);
	this->wType = pRoom->GetTParam(wX,wY);
	this->pathmap.Init(pRoom->wRoomCols, pRoom->wRoomRows);
	this->distance.Init(pRoom->wRoomCols, pRoom->wRoomRows);
	RecalcPathmap();
	CStation::wLastTurnInit = (UINT)-1;
}

//******************************************************************************
CStation::CStation(const CStation& src, CDbRoom* pRoom)
	: pRoom(pRoom), wX(src.wX), wY(src.wY), wType(src.wType), bRecalcPathmap(src.bRecalcPathmap)
{
	this->pathmap = src.pathmap;
	this->distance = src.distance;
}

//******************************************************************************
UINT CStation::GetDirectionFrom(const UINT wX, const UINT wY) const
//Returns: best available direction to move from (x,y) to get closer to station
{
	ASSERT(this->pRoom->IsValidColRow(wX,wY));
	UINT wBestDir = NO_ORIENTATION, wBestScore = this->pathmap.GetAt(wX,wY),
			wCurrentDist = this->distance.GetAt(wX,wY);
	if (!wBestScore)
		wBestScore = static_cast<UINT>(-1); //moving anywhere from here would be closer
	if (!wCurrentDist)
		wCurrentDist = static_cast<UINT>(-1);

	const UINT wT = this->pRoom->GetFSquare(wX, wY);
	const UINT wCols = this->pRoom->wRoomCols, wRows = this->pRoom->wRoomRows;
	UINT wXDest, wYDest, wScore, wDist;
	for (UINT n=wNumNeighbors; n--; )
	{
		if ((wYDest = wY+dyDir[n]) >= wRows) continue;
		if ((wXDest = wX+dxDir[n]) >= wCols) continue;
		wScore = this->pathmap.GetAt(wXDest, wYDest);
		wDist = this->distance.GetAt(wXDest, wYDest);
		bool bMonsterObstacle = false;
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wXDest, wYDest);
		if (pMonster)
			bMonsterObstacle = true;
		if (this->pRoom->DoesGentryiiPreventDiagonal(wX, wY, wXDest, wYDest))
			bMonsterObstacle = true;
		if (wScore && wDist < wCurrentDist && wScore < wBestScore &&
				!bMonsterObstacle && !CStation::swords.Exists(wXDest, wYDest) &&
				!IsObstacle(wT, wXDest, wYDest, nGetO(dxDir[n], dyDir[n])) &&
				!this->pRoom->GetCurrentGame()->IsPlayerAt(wXDest, wYDest)) //can't step on player
		{
			//New best direction.
			wBestScore = wScore;
			wBestDir = nGetO(dxDir[n],dyDir[n]);
		}
	}

	return wBestDir;
}

//******************************************************************************
UINT CStation::GetDistanceFrom(const UINT wX, const UINT wY) const
//Returns: pathmap distance to this station from (x,y), or 0 if unreachable
{
	if (this->pRoom->GetTSquare(this->wX,this->wY)!=T_STATION)
		return 0; //station no longer exists

	UINT wDist = this->distance.GetAt(wX,wY);
	if (wDist)
		return wDist;

	//(x,y) is not on pathmap.  See whether an adjacent tile on the pathmap can be reached.
	const UINT wT = this->pRoom->GetFSquare(wX, wY);
	const UINT wCols = this->pRoom->wRoomCols, wRows = this->pRoom->wRoomRows;
	UINT wXDest, wYDest;
	for (UINT n=wNumNeighbors; n--; )
	{
		if ((wYDest = wY+dyDir[n]) >= wRows) continue;
		if ((wXDest = wX+dxDir[n]) >= wCols) continue;
		wDist = this->distance.GetAt(wXDest, wYDest);
		if (wDist &&
				this->pRoom->GetMonsterAtSquare(wXDest, wYDest) == NULL &&
				!CStation::swords.Exists(wXDest, wYDest) &&
				!IsObstacle(wT, wXDest, wYDest, nGetO(dxDir[n], dyDir[n])))
			return wDist;
	}

	return 0;
}

//******************************************************************************
void CStation::RecalcPathmap()
//Mark path maps as needing recalculation next time they are queried.
{
	this->bRecalcPathmap = true;
}

//******************************************************************************
bool CStation::UpdateTurn(const UINT wTurnNo)
//Marks the positions of dynamic obstacles and updates pathmap if requested.
//Must be called before any station is queried on a given turn.
//If stations are never queried, these expensive calculations never take place.
//
//Returns: whether path map was recalculated, indicating other path maps must too
{
	bool bRes = this->bRecalcPathmap;
	if (this->pRoom->GetTSquare(this->wX,this->wY)!=T_STATION)
		return bRes; //station no longer exists

	if (bRes)
		CalcPathmap();

	if (wTurnNo == CStation::wLastTurnInit)
		return bRes;
	CStation::wLastTurnInit = wTurnNo;

	//Can't step on any swords.
	ASSERT(this->pRoom);
	this->pRoom->GetSwordCoords(CStation::swords, true);

	//Don't allow stepping on player either.
	CCurrentGame *pGame = this->pRoom->GetCurrentGame();
	ASSERT(pGame);
	if (pGame->swordsman.IsInRoom())
		CStation::swords.Add(pGame->swordsman.wX, pGame->swordsman.wY);
	return bRes;
}

//******************************************************************************
void CStation::UpdateType()
// Updates the type of the station to its tile's T parameter
{
	this->wType = pRoom->GetTParam(wX, wY);
}

//
//Private methods
//

//******************************************************************************
void CStation::CalcPathmap()
//Generates the pathmap.
{
	this->distance.Clear(); //real pathmap distance
	this->pathmap.Clear();  //monotonic relative distance metric -- for breaking ties in real distance

	CCoordStack evalCoords;
	if (this->pRoom->GetTSquare(this->wX,this->wY)==T_STATION)
		evalCoords.Push(this->wX,this->wY); //only rebuild pathmap if station still exists
	UINT wCount = 0; 

	const UINT wCols = this->pRoom->wRoomCols, wRows = this->pRoom->wRoomRows;
	int dx, dy, wNewX, wNewY;
	UINT wThisX, wThisY;
	while (evalCoords.PopBottom(wThisX,wThisY)) //perform as a queue for performance
	{
		const UINT wT = this->pRoom->GetFSquare(wThisX, wThisY);
		const UINT wDist = this->distance.GetAt(wThisX, wThisY);

		//Check every adjacent square for movement to this square.
		for (UINT nIndex=0; nIndex<wNumNeighbors; ++nIndex)
		{
			wNewX = wThisX + (dx = dxDir[nIndex]);
			wNewY = wThisY + (dy = dyDir[nIndex]);

			if ((UINT)wNewX >= wCols || (UINT)wNewY >= wRows)
				continue;  //out of bounds

			if (this->pathmap.Exists(wNewX, wNewY))
				continue; //already visited -- don't reevaluate

			//Check for obstacle, coming from the new square to this square.
			if (!IsObstacle(wT, wNewX, wNewY, nGetO(-dx, -dy)))
			{
				//Mark tile and branch out from there.
				evalCoords.Push(wNewX, wNewY);
				this->distance.Add(wNewX, wNewY, wDist+1);
				//Relative distance is measured by the order tiles are traversed.
				this->pathmap.Add(wNewX, wNewY, ++wCount);
			}
		}
	}

	this->bRecalcPathmap = false;
}

//******************************************************************************
bool CStation::IsObstacle(
//Returns: whether source tile can be exited and destination one entered
//
//Params:
	const UINT wDestF,  //F-layer tile at destination
	const UINT wX, const UINT wY, //source tile
	const UINT wOrientation) //direction of approach
const
{
	//Check whether it is impossible to reach destination from this direction.
	switch (wDestF)
	{
		case T_NODIAGONAL:
			if (wOrientation == NW || wOrientation == SW ||
					wOrientation == NE || wOrientation == SE)
				return true;
		break;

		case T_ARROW_N: case T_ARROW_NE: case T_ARROW_E: case T_ARROW_SE:
		case T_ARROW_S: case T_ARROW_SW: case T_ARROW_W: case T_ARROW_NW:
			if (bIsArrowObstacle(wDestF, wOrientation))
				return true;
		break;
		default: break;
	}

	//These are tiles it is impossible to come from.
	switch (this->pRoom->GetOSquare(wX,wY))
	{
		case T_PIT: case T_PIT_IMAGE: case T_STAIRS: case T_STAIRS_UP:
		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
		case T_WALL_M: case T_WALL_WIN:
		case T_WALL_B: case T_WALL_H:
		case T_DOOR_C: case T_DOOR_M: case T_DOOR_R: case T_DOOR_Y: case T_DOOR_B:
		case T_FIRETRAP_ON: case T_WATER: /*case T_SHALLOW_WATER:*/
			return true;
		default: break;
	}
	const UINT wF = this->pRoom->GetFSquare(wX,wY);
	switch (wF)
	{
		//Can the source tile be left from this direction?
		case T_ARROW_N: case T_ARROW_NE: case T_ARROW_E: case T_ARROW_SE:
		case T_ARROW_S: case T_ARROW_SW: case T_ARROW_W: case T_ARROW_NW:
			if (bIsArrowObstacle(wF, wOrientation))
				return true;
		break;

		case T_NODIAGONAL:
			if (wOrientation == NW || wOrientation == SW ||
					wOrientation == NE || wOrientation == SE)
				return true;
		break;
		default: break;
	}
	switch (this->pRoom->GetTSquare(wX,wY))
	{
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
		case T_OBSTACLE: case T_ORB: case T_BOMB:
		case T_TAR: case T_MUD: case T_GEL: case T_FLUFF:
		case T_LIGHT: case T_MIRROR:
		case T_POWDER_KEG:
		case T_STATION: case T_BEACON: case T_BEACON_OFF:
			return true;
		default: break;
	}

	//Broken rock golems are an obstacle.
	//However, serpent body tiles aren't being considered an obstacle, like for
	//brain pathmapping, which included them as obstacles solely due to code
	//limitations in v1.5.
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
	if (pMonster && bIsRockGolemType(pMonster->wType) && !pMonster->IsAlive())
		return true;

	return false; //step can be taken
}
