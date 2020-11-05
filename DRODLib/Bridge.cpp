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

#include "Bridge.h"
#include "TileConstants.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include "Swordsman.h"

const UINT NUM_NEIGHBORS = 4;
const int dx[NUM_NEIGHBORS] = {0,1,0,-1};
const int dy[NUM_NEIGHBORS] = {-1,0,1,0};

CTileMask CBridge::bridgeMask;

//*****************************************************************************
CBridge::CBridge()
	: pRoom(NULL)
{
	if (CBridge::bridgeMask.empty())
	{
		CBridge::bridgeMask.set(T_BRIDGE);
		CBridge::bridgeMask.set(T_BRIDGE_H);
		CBridge::bridgeMask.set(T_BRIDGE_V);
	}
}

//*****************************************************************************
void CBridge::HandleBridgeAdded(const UINT wX, const UINT wY)
// DO NOT USE OUTSIDE THIS CLASS!
// Should only be called in CDbRooms::InitRoomStats(), when called with a coordinate that points
// to an already stored bridge component will duplicate it!

// This function builds the component for a given bridge tile or does nothing if the tile
// is not a bridge

{
	ASSERT(this->pRoom);
	CCoordSet tiles, supportTiles;
	this->pRoom->GetConnected4NeighborTiles(wX, wY, this->bridgeMask, tiles, &this->ignoredTiles);
	if (tiles.empty())
		return; //no bridge tiles to process
	this->ignoredTiles += tiles; //don't process these tiles again

	//Determine whether this bridge needs to be monitored.
	CCoordSet::const_iterator tile;
	for (tile=tiles.begin(); tile!=tiles.end(); ++tile)
	{
		for (UINT i=NUM_NEIGHBORS; i--; )
		{
			const UINT wAX = tile->wX + dx[i], wAY = tile->wY + dy[i];
			if (!this->pRoom->IsValidColRow(wAX,wAY))
				continue;
			const UINT wOTile = this->pRoom->GetOSquare(wAX,wAY);
			if (!DoesTileSupportBridges(wOTile))
				continue;
			if (bIsFallingTile(wOTile))
			{
				//These tiles temporarily support bridges.
				supportTiles.insert(wAX,wAY);
				continue;
			}

			//Everything else permanent supports bridges.
			//This means these bridges will never change and need not be monitored.
			return;
		}
	}

	if (supportTiles.empty()) //this bridge is ready to drop now
		this->droppingBridges.push_back(this->bridgeSupports.size());

	this->bridges.push_back(tiles);
	this->bridgeSupports.push_back(supportTiles);
}

//*****************************************************************************
void CBridge::HandleTileBuilt(const UINT wX, const UINT wY, const UINT wTileNo)
//New tile was built at (x,y) and any adjacent bridge components will need to be reevaluated.
// This function will do the following:
// 1. Calculate which bridges were affected by this tile being built
// 2. Undo dropping of any affected bridges
// 3. Rebuild their components
{
	//Wasteful to call for non o-layer tiles.
	ASSERT(this->pRoom->GetOSquare(wX,wY) == wTileNo);

	this->ignoredTiles.clear(); //reset to recalc bridges here

	//1. Compile set of bridge tiles adjacent to built tile.
	CCoordSet adjacentBridges;
	if (bIsBridge(wTileNo))
		adjacentBridges.insert(wX, wY);

	for (UINT i=NUM_NEIGHBORS; i--; )
	{
		const UINT wAX = wX + dx[i], wAY = wY + dy[i];
		if (this->pRoom->IsValidColRow(wAX,wAY))
			if (bIsBridge(this->pRoom->GetOSquare(wAX,wAY)))
				adjacentBridges.insert(wAX, wAY);
	}

	//2. Determine which active bridges are affected - also undo dropping of affected bridges
	UINT wBridgeIndex;
	CCoordSet recalcBridgeTiles(adjacentBridges);
	CIDSet eraseBridgeIndexSet;
	for (wBridgeIndex=this->bridges.size(); wBridgeIndex--; ) //perform in reverse order
	{
		//If this bridge contains or is adjacent to the altered tile,
		//then it must be reevaluated for stability.
		CCoordSet& bridgeTiles = this->bridges[wBridgeIndex];
		bool bRecalculate = false;
		if (bridgeTiles.has(wX, wY))
			bRecalculate = true;
		else
			for (CCoordSet::const_iterator tile=adjacentBridges.begin(); tile!=adjacentBridges.end(); ++tile)
			{
				if (bridgeTiles.has(*tile))
				{
					//Redo this bridge.
					bRecalculate = true;
					break;
				}
			}

		if (bRecalculate) {
			// If an affected bridge was dropping undo the dropping so it can be potentially resupported again
			for (UINT wBridgeI = this->droppingBridges.size(); wBridgeI--; )
				if (this->droppingBridges[wBridgeI] == wBridgeIndex)
				{
					this->droppingBridges.erase(this->droppingBridges.begin() + wBridgeI);
					break;
				}

			recalcBridgeTiles += bridgeTiles;
			eraseBridgeIndexSet += wBridgeIndex;
		}
	}

	//3. Erase bridges
	//Bridge indices must be removed in reverse order to be valid.
	for (CIDSet::const_reverse_iterator id = eraseBridgeIndexSet.rbegin();
			id != eraseBridgeIndexSet.rend(); ++id)
	{
		const UINT wBridgeI = *id;
		this->bridges.erase(this->bridges.begin() + wBridgeI);
		this->bridgeSupports.erase(this->bridgeSupports.begin() + wBridgeI);

		//Since we're deleting indices, we should repair lists that reference those indices
		for (UINT wDropI=this->droppingBridges.size(); wDropI--; )
		{
			ASSERT(this->droppingBridges[wDropI] != wBridgeI);
			if (this->droppingBridges[wDropI] > wBridgeI)
				this->droppingBridges[wDropI]--;
		}
	}
	
	//4. Recompute these bridge tiles.
	CCoordSet::const_iterator tile;
	for (tile=recalcBridgeTiles.begin(); tile!=recalcBridgeTiles.end(); ++tile)
		HandleBridgeAdded(tile->wX, tile->wY);
}

//*****************************************************************************
void CBridge::Clear()
//Reset data members.
{
	this->pRoom = NULL;
	this->bridges.clear();
	this->bridgeSupports.clear();
	this->ignoredTiles.clear();
	this->droppingBridges.clear();
}

//*****************************************************************************
void CBridge::Plotted(const UINT wX, const UINT wY, const UINT wTileNo)
// Non-bridge tile has changed, remove it from supports where necessary
// and mark a bridge as falling if it has no supports left.
{
	if (DoesTileSupportBridges(wTileNo))
		return; //this tile doesn't affect any bridge supports
	if (bIsBridge(wTileNo))
		return; //rely on CBridge::HandleTileBuilt function to determine falling

	//Check each bridge's set of supporting tiles for membership.
	vector<CCoordSet>::iterator tiles;
	UINT wIndex=0;
	for (tiles = this->bridgeSupports.begin();
			tiles != this->bridgeSupports.end(); ++tiles, ++wIndex)
	{
		//Since a support is no longer there, remove it from sets that contain it.
		if (tiles->erase(wX,wY))
		{
			//Tile was removed from the set of supporting tiles.
			if (tiles->empty())
				this->droppingBridges.push_back(wIndex);
		}
	}
}

//*****************************************************************************
void CBridge::Process(CCueEvents &CueEvents)
//Process any relevant changes that have occurred to bridges.
//
//Returns: whether any bridges are falling
{
	if (this->droppingBridges.empty())
		return; //no bridges are dropping

	// Do not process bridges on turn 0, even after double placement
	CCurrentGame* pCurrentGame = this->pRoom->GetCurrentGame();
	if (pCurrentGame->wPlayerTurn == 0)
		return;

	//Drop any bridges marked without support now.
	CIDSet indices;
	for (vector<UINT>::const_iterator dropIndex = this->droppingBridges.begin();
			dropIndex != this->droppingBridges.end(); ++dropIndex)
	{
		//This bridge has been marked has having no support.
		indices += *dropIndex;
	}
	for (CIDSet::const_reverse_iterator index=indices.rbegin();
			index != indices.rend(); ++index)
	{
		const UINT wIndex = *index;
		ASSERT(this->bridgeSupports[wIndex].empty());
		DropBridgeTiles(CueEvents, this->bridges[wIndex]);

		//Remove data structures for dropped bridge.
		this->bridges.erase(this->bridges.begin() + wIndex);
		this->bridgeSupports.erase(this->bridgeSupports.begin() + wIndex);
	}
	this->droppingBridges.clear();
}

//*****************************************************************************
void CBridge::SetMembersForRoom(const CBridge& src, CDbRoom* pRoom)
//Deep member copy.
{
	this->pRoom = pRoom;

	this->bridges = src.bridges;
	this->bridgeSupports = src.bridgeSupports;
	this->ignoredTiles = src.ignoredTiles;
	this->droppingBridges = src.droppingBridges;
}

//*****************************************************************************
void CBridge::SetRoom(CDbRoom *pRoom)
//Initializes for use in a room.
{
	Clear();
	this->pRoom = pRoom;
}

//
// Private members.
//

//*****************************************************************************
void CBridge::DropBridgeTiles(CCueEvents &CueEvents, const CCoordSet& tiles)
//Drop the specified bridge.
{
	ASSERT(this->pRoom);

	//Decide which tile to replace bridge with.
	//The default is water, unless pit is adjacent (overrides).
	UINT wTileToPlot = T_WATER;
	CCoordSet::const_iterator tile;
	for (tile=tiles.begin(); tile!=tiles.end(); ++tile)
	{
		for (UINT i=NUM_NEIGHBORS; i--; )
		{
			const UINT wX = tile->wX + dx[i], wY = tile->wY + dy[i];
			if (!this->pRoom->IsValidColRow(wX,wY))
				continue;
			const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
			if (bIsPit(wOTile))
			{
				wTileToPlot = wOTile;
				break;
			}
			if (wOTile == T_PLATFORM_P)
			{
				wTileToPlot = this->pRoom->coveredOSquares.GetAt(wX,wY);
				break;
			}
		}
		if (bIsPit(wTileToPlot))
			break; //don't need to search further
	}

	//Player dies if on falling bridge.
	CCurrentGame *pCurrentGame = this->pRoom->GetCurrentGame();
	CSwordsman& player = pCurrentGame->swordsman;
	if (player.IsInRoom() && tiles.has(player.wX, player.wY))
	{
		if (!bIsEntityFlying(player.wAppearance))
		{
			pCurrentGame->SetDyingEntity(&player);
			CueEvents.Add(CID_BriarKilledPlayer);   //!!CID_PlayerFellInPit
		}
	}

	//Replace bridge.
	for (tile=tiles.begin(); tile!=tiles.end(); ++tile)
	{
		const UINT wX = tile->wX, wY = tile->wY;
		const UINT wOTile = this->pRoom->GetOSquare(wX, wY);
		if (bIsPit(wTileToPlot))
			CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(wX, wY, NO_ORIENTATION, wOTile, 0), true);
		else {
			ASSERT(bIsWater(wTileToPlot));
			CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
		}
		this->pRoom->Plot(wX, wY, wTileToPlot);
		this->pRoom->coveredOSquares.Add(wX, wY, wTileToPlot);
	}
	//Once entire bridge is gone, check for falling objects.
	for (tile=tiles.begin(); tile!=tiles.end(); ++tile)
		this->pRoom->CheckForFallingAt(tile->wX, tile->wY, CueEvents);
	this->pRoom->ConvertUnstableTar(CueEvents);
}

//*****************************************************************************
bool CBridge::DoesTileSupportBridges(const UINT wTile) const
//Returns: whether this tile supports a bridge
{
	//These tiles do not support a bridge.
	return !(bIsBridge(wTile) || bIsPit(wTile) || bIsWater(wTile) ||
			bIsSteppingStone(wTile) || bIsPlatform(wTile));
}

//*****************************************************************************
bool CBridge::IsBridgeDropping(const UINT wBridgeIndex) const
// Return: true if the given bridge index is falling
{
	for (UINT wBridgeI = this->droppingBridges.size(); wBridgeI--; )
		if (this->droppingBridges[wBridgeI] == wBridgeIndex)
			return true;

	return false;
}