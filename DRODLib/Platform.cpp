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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Platform.h"
#include "CurrentGame.h"
#include "GameConstants.h"
#include "DbRooms.h"
#include "TileConstants.h"
#include "MonsterFactory.h"
#include "RockGolem.h"

CCoordSet CPlatform::fallTiles;

//*****************************************************************************
CPlatform::CPlatform(
//
//
//Params:
	const CCoordSet &blockSet, const CIDSet& types)
	: pCurrentGame(NULL)
	, xOffset(0), yOffset(0)
	, xDelta(0), yDelta(0)
	, types(types)
{
	this->blocks = blockSet;
	ASSERT(this->blocks.GetSize());

	PrepareEdgeSet();
}

//*****************************************************************************
void CPlatform::SetCurrentGame(CCurrentGame *pSetCurrentGame)
{
	if (!pSetCurrentGame) return;
	this->pCurrentGame = pSetCurrentGame;
	ASSERT(this->pCurrentGame->pRoom);
}

//*****************************************************************************
bool CPlatform::IsAt(const UINT wX, const UINT wY) const
{
	return this->blocks.IsMember(wX,wY);
}

//*****************************************************************************
bool CPlatform::CanMove(const UINT wO)
//Returns: whether platform can be moved in specified direction
{
	//Check direction(s) platform will move.
	PlatformDir dirIndex[2] = {DIR_COUNT, DIR_COUNT};
	switch (wO)
	{
		case N: dirIndex[0] = North; break;
		case W: dirIndex[0] = West; break;
		case E: dirIndex[0] = East; break;
		case S: dirIndex[0] = South; break;
		case NW: dirIndex[0] = North; dirIndex[1] = West; break; //don't change this order
		case SW: dirIndex[0] = West;  dirIndex[1] = South; break;
		case NE: dirIndex[0] = East;  dirIndex[1] = North; break;
		case SE: dirIndex[0] = South; dirIndex[1] = East; break;
		default: ASSERT(!"Bad platform direction"); break;
	}
	ASSERT(dirIndex[0] < DIR_COUNT);
	ASSERT(dirIndex[1] <= DIR_COUNT);

	ASSERT(this->pCurrentGame);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const int nOX = nGetOX(wO), nOY = nGetOY(wO);

	//If something is in the way on either edge, change direction and stop.
	UINT wBlockX, wBlockY;
	const bool bDiagonal = dirIndex[1] < DIR_COUNT;
	for (UINT n=bDiagonal?2:1; n--; ) //don't check second edge when moving axially
	{
		const int nAxialX = dirIndex[n] == West ? -1 : dirIndex[n] == East ? 1 : 0;
		const int nAxialY = dirIndex[n] == North ? -1 : dirIndex[n] == South ? 1 : 0;
		vector<UINT>& edge = this->edgeBlocks[dirIndex[n]];
		for (UINT wIndex=edge.size(); wIndex--; )
		{
			//Check each block for free movement in current direction.
			this->blocks.GetAt(edge[wIndex], wBlockX, wBlockY);
			if (!CanMoveTo(&room, wBlockX + nOX, wBlockY + nOY))
				return false;
			//To prevent sliding through cracks when moving diagonally,
			//we must check movement in the axial directions also.
			if (bDiagonal && !CanMoveTo(&room, wBlockX + nAxialX, wBlockY + nAxialY))
				return false;
		}
	}

	//These objects make platform too heavy to move.
	for (UINT wIndex=this->blocks.GetSize(); wIndex--; )
	{
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		if (bIsTar(room.GetTSquare(wBlockX, wBlockY)))
			return false;
		CMonster *pMonster = room.GetMonsterAtSquare(wBlockX, wBlockY);
		if (pMonster && (pMonster->IsLongMonster() || pMonster->IsPiece()))
			return false;
 	}

	//Nothing is obstructing the platform from moving.
	return true;
}

//*****************************************************************************
void CPlatform::checkForFalling(CDbRoom *pRoom, CCueEvents& CueEvents)
//Check for falling objects.
{
	if (CPlatform::fallTiles.empty())
		return;

	//There shouldn't be any tiles to process if there are no platforms in the room.
	ASSERT(!pRoom->platforms.empty());

	for (CCoordSet::const_iterator tile=CPlatform::fallTiles.begin();
			tile!=CPlatform::fallTiles.end(); ++tile)
		pRoom->CheckForFallingAt(tile->wX, tile->wY, CueEvents);
	pRoom->ConvertUnstableTar(CueEvents);
	
	CPlatform::fallTiles.clear();
}

//*****************************************************************************
void CPlatform::GetTiles(
//Adds tiles in platform to 'tiles'
//
//Params:
	CCoordSet& tiles) //(in/out)
const
{
	for (UINT wIndex=this->blocks.GetSize(); wIndex--; )
	{
		UINT wBlockX, wBlockY;
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		tiles.insert(wBlockX, wBlockY);
	}
}

//*****************************************************************************
CIDSet CPlatform::GetTypes() const
//Returns the list of tile types the Platform moves over
{
	return this->types;
}

//*****************************************************************************
void CPlatform::Move(const UINT wO)
//Move platform one square in specified direction.
//Pre-cond: CanMove(wO) was just called and returned true
{
	ASSERT(this->pCurrentGame);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const int nOX = nGetOX(wO), nOY = nGetOY(wO);

	Move(room, nOX, nOY, true);

	//Current turn offset.
	this->xDelta += nOX;
	this->yDelta += nOY;

	//Maintain total movement offset.
	this->xOffset += nOX;
	this->yOffset += nOY;
}

//*****************************************************************************
void CPlatform::Move(CDbRoom& room, const int nOX, const int nOY, const bool bPlotToRoom)
{
	//Move all pieces in direction of movement.
	//1. Remove all blocks from current position.
	UINT wBlockX, wBlockY;
	UINT wIndex;
	for (wIndex=this->blocks.GetSize(); wIndex--; )
	{
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		if (bPlotToRoom)
			room.Plot(wBlockX, wBlockY, room.coveredOSquares.GetAt(wBlockX, wBlockY));
		CPlatform::fallTiles.insert(wBlockX, wBlockY);
 	}

	//2. Place blocks in new position.
	const UINT wTile = this->types.has(T_WATER) ? T_PLATFORM_W : T_PLATFORM_P;
	for (wIndex=this->blocks.GetSize(); wIndex--; )
	{
		//Move block.
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		wBlockX += nOX;
		wBlockY += nOY;
		this->blocks.SetAt(wIndex, wBlockX, wBlockY);
		if (bPlotToRoom)
			room.Plot(wBlockX, wBlockY, wTile);
		CPlatform::fallTiles.erase(wBlockX, wBlockY);
	}
}

//*****************************************************************************
void CPlatform::RemoveTile(const UINT wX, const UINT wY)
//Remove indicated tile from this platform.
{
	if (!IsAt(wX,wY))
		return;
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	CTileMask tileMask(this->types.has(T_WATER) ? T_PLATFORM_W : T_PLATFORM_P);

	CCoordSet oldPlatform;
	this->blocks.AddTo(oldPlatform);
	vector<CCoordSet> regions;
	room.GetConnectedRegionsAround(wX, wY, tileMask, regions, NULL, &oldPlatform);
	if (regions.size() <= 1)
	{
		this->blocks.Remove(wX,wY);
		PrepareEdgeSet();
	}
	else
	{
		vector<CCoordSet>::const_iterator reg = regions.begin();
		this->blocks = *reg;
		PrepareEdgeSet();

		for (++reg; reg != regions.end(); ++reg)
		{
			CPlatform* pNewPlat = new CPlatform(*reg, this->types);
			pNewPlat->SetCurrentGame(this->pCurrentGame);
			room.platforms.push_back(pNewPlat);
			pNewPlat->PrepareEdgeSet();
		}
	}
}

//*****************************************************************************
void CPlatform::ReflectX(CDbRoom *pRoom)
//Reflect platform through X-axis
{
	UINT wBlockX, wBlockY;
	UINT wIndex;
	for (wIndex=this->blocks.GetSize(); wIndex--; )
	{
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		wBlockX = (pRoom->wRoomCols-1) - wBlockX;
		this->blocks.SetAt(wIndex, wBlockX, wBlockY);
	}
}

//*****************************************************************************
void CPlatform::ReflectY(CDbRoom *pRoom)
//Reflect platform through Y-axis
{
	UINT wBlockX, wBlockY;
	UINT wIndex;
	for (wIndex=this->blocks.GetSize(); wIndex--; )
	{
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		wBlockY = (pRoom->wRoomRows-1) - wBlockY;
		this->blocks.SetAt(wIndex, wBlockX, wBlockY);
	}
}

//
// Private methods
//

//*****************************************************************************
bool CPlatform::CanMoveTo(CDbRoom* pRoom, const UINT wX, const UINT wY) const
//Returns: whether platform can move onto this tile
{
	if (!pRoom->IsValidColRow(wX, wY))
		return false;

	//Platforms can always move onto itself
	if (IsAt(wX, wY))
		return true;

	//Platform can only move onto its specified tile type.
	if (!this->types.has(pRoom->GetOSquare(wX, wY)))
		return false;

	//Obstacles block platforms.
	switch (pRoom->GetTSquare(wX, wY))
	{
		case T_OBSTACLE: case T_LIGHT:
			return false;
		default: break;
	}

	//Water monsters will block platforms.
	CMonster *pMonster = pRoom->GetMonsterAtSquare(wX, wY);
	if (pMonster && pMonster->IsSwimming() && !CPlatform::fallTiles.has(wX, wY))
		return false;

	return true;
}

//*****************************************************************************
void CPlatform::PrepareEdgeSet()
//Speed optimization for collision detection during movement:
//Generate four sets - one for each direction - of tiles that don't have
//another platform tile adjacent in that direction.
{
	UINT wBlockX, wBlockY;
	for (UINT wIndex=this->blocks.GetSize(); wIndex--; )
	{
		this->blocks.GetAt(wIndex, wBlockX, wBlockY);
		if (!this->blocks.IsMember(wBlockX, wBlockY-1))
			this->edgeBlocks[North].push_back(wIndex);
		if (!this->blocks.IsMember(wBlockX, wBlockY+1))
			this->edgeBlocks[South].push_back(wIndex);
		if (!this->blocks.IsMember(wBlockX+1, wBlockY))
			this->edgeBlocks[East].push_back(wIndex);
		if (!this->blocks.IsMember(wBlockX-1, wBlockY))
			this->edgeBlocks[West].push_back(wIndex);
	}
}
