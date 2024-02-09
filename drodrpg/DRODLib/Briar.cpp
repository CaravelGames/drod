// $Id: Briar.cpp 10108 2012-04-22 04:54:24Z mrimer $

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

#include "Briar.h"
#include "Character.h"
#include "DbRooms.h"
#include "GameConstants.h"
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "Splitter.h"
#include "TileConstants.h"
#include <BackEndLib/Assert.h>

const bool b8NeighborConnectivity =
#ifdef DIAGONAL_EXPANSION
		true;
#else
		false;
#endif

const UINT NUM_NEIGHBORS =
#ifdef DIAGONAL_EXPANSION
	8;
const int nOX[NUM_NEIGHBORS] = {0,1,0,-1, -1,-1,1,1};
const int nOY[NUM_NEIGHBORS] = {-1,0,1,0, -1,1,-1,1};
#else
	4;
const int nOX[NUM_NEIGHBORS] = {0,1,0,-1};
const int nOY[NUM_NEIGHBORS] = {-1,0,1,0};
#endif

//
//Public methods
//

//*****************************************************************************
CBriars::CBriars()
	: pRoom(NULL)
//	, bRecalc(false)
{
}

//*****************************************************************************
CBriars::~CBriars()
{
	clear();
}

//*****************************************************************************
void CBriars::clear()
//Resets data structures.
{
	this->pRoom = NULL;

	this->roots.clear();
}

//*****************************************************************************
void CBriars::setMembersForRoom(const CBriars& /*src*/, CDbRoom* pRoom)
//Deep member copy.
{
	this->pRoom = pRoom;
}

//*****************************************************************************
bool CBriars::empty() const
//Returns: whether any briar sources exist
{
	return this->roots.empty();
}

//*****************************************************************************
void CBriars::getBriarTilesConnectedToRoots(CCoordSet& connectedBriarTiles, const bool b8Neighbor)
//Outputs: a coordSet of room tiles containing briars connected to roots
{
	ASSERT(this->pRoom);
	CDbRoom& room = *this->pRoom;

	CCoordSet tiles;
	CTileMask briarTiles(T_BRIAR_SOURCE);
	briarTiles.set(T_BRIAR_LIVE);
	briarTiles.set(T_BRIAR_DEAD);
	for (CCoordSet::const_iterator briar=this->roots.begin(); briar!=this->roots.end(); ++briar)
	{
		room.GetConnectedTiles(briar->wX, briar->wY, briarTiles, b8Neighbor, tiles, &connectedBriarTiles);
		connectedBriarTiles += tiles;
	}
}

//*****************************************************************************
void CBriars::insert(const UINT wX, const UINT wY)
//Add a briar source at (x,y)
{
	this->roots.insert(wX, wY);
}

//*****************************************************************************
void CBriars::plotted(const UINT wX, const UINT wY, const UINT wTileNo)
//Room object calls this to notify briars that room geometry has changed at (x,y)
{
	if (bIsBriar(wTileNo))
		return; //plotting briar itself shouldn't affect anything

	//If a door closes here or the t-layer is modified (i.e. briar is overwritten),
	//synch data structures for any briar that used to be here.
	if (!bIsDoor(wTileNo) && TILE_LAYER[wTileNo] != 1) //t-layer plots affect briars
		return;

	const bool bRefreshBriarLife = !this->roots.empty();

	const UINT wTTile = this->pRoom->GetTSquare(wX,wY);

	//Any root at this location is removed.
	if (wTTile != T_BRIAR_SOURCE)
		removeSource(wX, wY);

	//Door closed -- remove briar from this tile.
	if (bIsDoor(wTileNo) && bIsBriar(wTTile))
		this->pRoom->Plot(wX,wY,T_EMPTY);

	//Shouldn't be a root coord retained for this location.
	ASSERT(!this->roots.has(wX,wY));

	if (bRefreshBriarLife)
		setBriarTilesLife();
}

//*****************************************************************************
void CBriars::process(
//Process briar connected components with roots.
//
//Params:
	CCueEvents &CueEvents)     //(in/out)
{
	if (this->roots.empty())
		return; //no briar roots in the room

	ASSERT(this->pRoom);
	CDbRoom& room = *this->pRoom;
	CCurrentGame *pCurrentGame = room.GetCurrentGame();

	//Get all briar tiles connected to a root.
	CCoordSet existingBriarTiles;
	getBriarTilesConnectedToRoots(existingBriarTiles, b8NeighborConnectivity);
	ASSERT(!existingBriarTiles.empty());

	//Examine each briar tile connected to a root for expansion to adjacent room tiles.
	CCoordSet pressurePlates;
	for (CCoordSet::const_iterator tile=existingBriarTiles.begin();
			tile!=existingBriarTiles.end(); ++tile)
	{
		const int x=tile->wX, y=tile->wY;
		bool bBlocked;
#ifdef DIAGONAL_EXPANSION
		bool bNPit = false, bSPit = false, bEPit = false, bWPit = false;
#endif

		for (UINT i=0; i<NUM_NEIGHBORS; ++i)
		{
#ifdef DIAGONAL_EXPANSION
			//Diagonal expansion across a corner surrounded by pit is not allowed.
			if (i==4 && bNPit && bWPit)
				break;
			if (i==5 && bSPit && bWPit)
				break;
			if (i==6 && bNPit && bEPit)
				break;
			if (i==7 && bSPit && bEPit)
				break;
#endif

			const UINT wX = x + nOX[i], wY = y + nOY[i];
			if (!room.IsValidColRow(wX,wY))
				continue;

			bBlocked = false;
			const UINT wTTile = room.GetTSquare(wX,wY);
			switch (wTTile)
			{
				//These items block briar.
				case T_OBSTACLE:
					bBlocked = true;
				break;
				case T_BRIAR_SOURCE: case T_BRIAR_LIVE:
					bBlocked = true;
				break;
				//Dead briars are (re)attached to this root and come alive.
				case T_BRIAR_DEAD:
				{
					CTileMask deadTileSet(T_BRIAR_DEAD);
					CCoordSet deadTiles;
					room.GetConnectedTiles(wX, wY, deadTileSet, b8NeighborConnectivity, deadTiles, NULL);
					for (CCoordSet::const_iterator tile=deadTiles.begin();
							tile!=deadTiles.end(); ++tile)
						room.Plot(tile->wX, tile->wY, T_BRIAR_LIVE);

					bBlocked = true;
				}
				break;

				//Briars break everything else.  Including orbs.
				default: break;
			}

			//Misty tiles block briar
			if (room.IsEitherTSquare(wX, wY, T_MIST)) {
				bBlocked = true;
			}

			if (bBlocked) continue;

			//Force arrows in wrong direction stop the briar
			const UINT wFTile = room.GetFSquare(wX,wY);
			if (bIsArrow(wFTile) && bIsArrowObstacle(wFTile,nGetO(nOX[i],nOY[i])))
				bBlocked = true;

			const UINT wOTile = room.GetOSquare(wX,wY);
			switch (wOTile)
			{
				//Briars can't expand into a pit.
				case T_PIT: case T_PIT_IMAGE: //case T_PLATFORM_P:
#ifdef DIAGONAL_EXPANSION
					//Keep track of which edge is next to pit.
					switch (i)
					{
						case 0: bNPit = true; break;
						case 1: bEPit = true; break;
						case 2: bSPit = true; break;
						case 3: bWPit = true; break;
						default: break;
					}
#endif
					continue;

				//Briar expands onto room tile.
				case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
				case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD: case T_FLOOR_GRASS:
				case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
				case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:
				case T_DOOR_RO: case T_DOOR_BO: case T_DOOR_MONEYO:
				case T_TRAPDOOR: case T_TRAPDOOR2: case T_PRESSPLATE:
				case T_GOO:
				case T_STAIRS: case T_STAIRS_UP:
				case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
				case T_WATER: case T_PLATFORM_W:
				case T_PLATFORM_P:
				break;

				//Can't grow onto anything else.
				case T_HOT: //hot tiles prevent new growth
				default:
					bBlocked = true;
				break;
			}
			if (bBlocked)
				continue;

			//Monsters (except snakes) are killed as briar covers them.
			CMonster *pMonster = room.GetMonsterAtSquare(wX, wY);
			if (pMonster)
			{
				pMonster = pMonster->GetOwningMonster();
				switch (pMonster->wType)
				{
					//Serpents stop the briar (acting as a retaining wall).
					case M_SERPENT:
					case M_SERPENTG:
					case M_SERPENTB:
						bBlocked = true;
					break;

					//All other monsters engulfed.
					default: break;
				}
			}
			if (bBlocked)
				continue;

			//Expand briar edge to this tile.
			CueEvents.Add(CID_BriarExpanded);
			if (pCurrentGame->IsPlayerAt(wX, wY))
				CueEvents.Add(CID_BriarKilledPlayer);

			if (bIsTar(wTTile))
			{
				room.RemoveStabbedTar(wX, wY, CueEvents);
				CueEvents.Add(CID_TarstuffDestroyed,
						new CMoveCoordEx(wX, wY, nGetO(nOX[i],nOY[i]), wTTile), true);
			}
			else if (wTTile == T_FUSE)
			{
				room.LitFuses.erase(wX, wY);
			}
			room.Plot(wX,wY,T_BRIAR_LIVE);

			//If dead tiles are adjacent to the newly-grown piece, they now become alive.
			for (UINT k=0; k<4; ++k)
			{
				const UINT wAX = wX + nOX[k], wAY = wY + nOY[k];
				if (!room.IsValidColRow(wAX,wAY))
					continue;

				//If a dead tile is here, show them as now alive.
				if (room.GetTSquare(wAX,wAY) == T_BRIAR_DEAD)
				{
					CTileMask deadTileSet(T_BRIAR_DEAD);
					CCoordSet deadTiles;
					room.GetConnectedTiles(wAX, wAY, deadTileSet, b8NeighborConnectivity, deadTiles, NULL);
					for (CCoordSet::const_iterator tile=deadTiles.begin();
							tile!=deadTiles.end(); ++tile)
						room.Plot(tile->wX, tile->wY, T_BRIAR_LIVE);
				}
			}

			//Briar kills monster once briar tile has been plotted.
			if (pMonster)
			{
				bool bVulnerable = true;

				if (pMonster->wType == M_CHARACTER)
				{
					CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					pCharacter->Defeat(); //NPC is "defeated" by briars
					bVulnerable = pCharacter->IsVulnerable();
				}

				if (bVulnerable)
				{
					//!!Won't get to see the move this monster just made since it's
					//getting removed from the monster list here.  Adding this cue event
					//will cause an effect updating the room tiles to fix a resulting problem:
					//Showing this effect may be desired, but note it's just a kludge
					//to get the monster's old square to be dirtied and refreshed.
					room.KillMonster(pMonster, CueEvents);
					CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
				}
			}

			//Briar depresses pressure plates.  Handle them all at the end of the turn.
			if (wOTile == T_PRESSPLATE)
				pressurePlates.insert(wX,wY);
		}
	}

	//Convert unstable tar to tar babies
	room.ConvertUnstableTar(CueEvents, true);

	//Process any pressure plates depressed now.
	for (CCoordSet::const_iterator plate=pressurePlates.begin();
			plate!=pressurePlates.end(); ++plate)
	{
		this->pRoom->ActivateOrb(plate->wX, plate->wY, CueEvents, OAT_PressurePlate);
	}
//	this->pressurePlates.clear();
}

//*****************************************************************************
void CBriars::removeSource(const UINT wX, const UINT wY)
//Removes briar source at (x,y).
{
	this->roots.erase(wX,wY);
}

//*****************************************************************************
void CBriars::setBriarTilesLife()
//Set each briar tile in the room to live or dead state.
//It is alive when part of a connected component containing a root, otherwise dead.
{
	ASSERT(this->pRoom);
	CDbRoom& room = *this->pRoom;

	CCoordSet connectedBriarTiles;
	getBriarTilesConnectedToRoots(connectedBriarTiles, b8NeighborConnectivity);

	for (UINT y=room.wRoomRows; y--; )
		for (UINT x=room.wRoomCols; x--; )
			switch (room.GetTSquare(x,y))
			{
				case T_BRIAR_LIVE:
					if (!connectedBriarTiles.has(x,y))
						room.Plot(x, y, T_BRIAR_DEAD);
				break;
				case T_BRIAR_DEAD:
					if (connectedBriarTiles.has(x,y))
						room.Plot(x, y, T_BRIAR_LIVE);
				break;
				default: break;
			}
}

//*****************************************************************************
void CBriars::setRoom(CDbRoom* pRoom)
//Sets room objects in which briars are located.
{
	ASSERT(pRoom);
	if (pRoom == this->pRoom)
		return; //don't reinit

	ASSERT(empty()); //must call before adding any briars to room
	this->pRoom = pRoom;

//	this->briarIndices.Init(pRoom->wRoomCols, pRoom->wRoomRows);
}
