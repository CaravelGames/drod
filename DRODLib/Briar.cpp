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

/*
Poor man's notes about how Briar is processed:

Vocabulary:
 - Component - a connected mass of briar. Say you have two 5x5 blocks of briar in a room, not connected to each other. Each one of them is a separate Component. When a growth causes two components to combine, they merge into one.


It appears we have a few variables:
 1. briars - stores the instances of the briar roots
 2. briarComponents - a vector of coordinate-sets containing positions of a given COMPONENT's Withered Briar tiles, but I think more accurate would be a map of ID to COMPONENT, where ID is the index in the vector. When a COMPONENT is removed from the start of the list then I guess all the other's IDs will change
 3. briarEdge - a vector of coordinate-sets containing positions of a given COMPONENT's edge Briar Growth tiles
 4. connectedBriars - a vector if coordinate-pairs, apparently only used during growth, it links COMPONENT's tiles prior to growin with the new tiles so that it can be used to merge everything together. It's emptied at the end of processing briar.

How is briar processed:
 1. If during this turn Briar was damaged in SPECIFIC WAY then RECALCULATE ALL of the components.
 2. The following operation are done for every briar root in sequence.
	a) Process briar growth - solidify a single Briar Growth (briarEdge) into a Withered Briar. Does nothing if there are no growable edges left
	b) Expand into new space - if the root's component has no briarEdges left, every briar tile in this root's component tries to grow out into 8 cardinal directions. The growing follows EXPANSIONS RULES
	c) Merge components - using the power of `connectedBriars` coordinate-pairs we can find out if any two components have grown into each other and merge them. Nothing fun happens here
 3. Explode powder kegs
 4. Convert unstable tar to tar babies
 5. And process pressure plate pressing and depressing

Damaged in SPECIFIC WAY:
 When a briar that belongs to a component (has briar root) is damaged then a flag is set that forces the briar data regeneration

RECALCULATE ALL:
 1. All briar data, except roots info, is prunned.
 2. Then for every root its component is rebuilt

EXPANSION RULES:
 1. Grow into all 8 directions
 2. But given tile can only grow diagonally if there is no put orthogonally to that diagonal
 3. Can't grow against force arrows
 4. Hot tiles prevent growth
 5. Grows into solid fluff but is consumed by it but consumption happens at the end of a single expansion cycle for a given briar root
 6. Explodes kegs but they explode after everything has finished growing
*/

#include "Briar.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include "GameConstants.h"
#include "MonsterPiece.h"
#include "RockGiant.h"
#include "TileConstants.h"
#include <BackEndLib/Assert.h>

const UINT NUM_NEIGHBORS = 8;
const int nOX[NUM_NEIGHBORS] = {0,1,0,-1, -1,-1,1,1};
const int nOY[NUM_NEIGHBORS] = {-1,0,1,0, -1,1,-1,1};

//*****************************************************************************
CBriar::CBriar(CBriars *pBriars, const UINT wComponentIndex, const UINT wX, const UINT wY)
	: wComponentIndex(wComponentIndex)
	, wX(wX), wY(wY)
	, bStuck(false), bDone(false)
	, pBriars(pBriars)
{
	ASSERT(pBriars);
}

//*****************************************************************************
bool CBriar::process()
//Solidify one edge tile of the briar's connected component.
//
//Returns: whether the briar's turn is not done
{
	if (this->bDone || this->bStuck)
		return false; //briar has finished turn or finished filling available area

	CCoordSet& edge = this->pBriars->briarEdge[this->wComponentIndex-1];
	if (edge.empty())
		return true; //no edge -- time to expand

	//Fill in the closest edge tile.
	int dx, dy;
	float fDistSq, fClosestSq = 100000000.0f;
	UINT wClosestX = 0, wClosestY = 0;
	for (CCoordSet::const_iterator tile=edge.begin(); tile!=edge.end(); ++tile)
	{
		dx = (int)tile->wX - (int)this->wX;
		dy = (int)tile->wY - (int)this->wY;
		fDistSq = float(dx*dx + dy*dy);
		if (fDistSq < fClosestSq)
		{
			fClosestSq = fDistSq;
			wClosestX = tile->wX;
			wClosestY = tile->wY;
		}
	}

	CDbRoom& room = *(this->pBriars->pRoom);
	ASSERT(room.IsValidColRow(wClosestX,wClosestY));
	ASSERT(room.GetTSquare(wClosestX,wClosestY) == T_BRIAR_LIVE);
	room.Plot(wClosestX,wClosestY,T_BRIAR_DEAD);
	edge.erase(wClosestX, wClosestY);
	this->pBriars->briarComponents[this->wComponentIndex-1].insert(wClosestX, wClosestY);

	this->bDone = true;
	return false;
}

//
//Public methods
//

//*****************************************************************************
CBriars::CBriars()
	: pRoom(NULL)
	, bRecalc(false)
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
	for (list<CBriar*>::iterator briar = this->briars.begin();
			briar != this->briars.end(); ++briar)
		delete *briar;
	this->briars.clear();

	this->briarComponents.clear();
	this->briarEdge.clear();
	this->briarIndices.Clear();
	this->connectedBriars.clear();
	this->pressurePlates.clear();
	this->bRecalc = false;
}

//*****************************************************************************
void CBriars::setMembersForRoom(const CBriars& src, CDbRoom* pRoom)
//Deep member copy.
{
	this->pRoom = pRoom;

	//Set child briar objects.
	for (list<CBriar*>::iterator briar = this->briars.begin();
			briar != this->briars.end(); ++briar)
		delete *briar;
	this->briars.clear();
	for (std::list<CBriar*>::const_iterator f = src.briars.begin();
			f != src.briars.end(); ++f)
	{
		CBriar& srcBriar = *(*f);
		CBriar *pBriar = new CBriar(this, srcBriar.wComponentIndex, srcBriar.wX, srcBriar.wY);
		pBriar->bStuck = srcBriar.bStuck;
		pBriar->bDone = srcBriar.bDone;
		this->briars.push_back(pBriar);
	}

	this->briarIndices = src.briarIndices;
	this->briarComponents = src.briarComponents;
	this->briarEdge = src.briarEdge;
	this->connectedBriars = src.connectedBriars;
	this->pressurePlates = src.pressurePlates;
	this->bRecalc = src.bRecalc;
}

//*****************************************************************************
bool CBriars::empty() const
//Returns: whether any briar roots exist
{
	return this->briars.empty();
}

//*****************************************************************************
void CBriars::expand(
//Expands the specified briar connected component.
//
//Params:
	CCueEvents &CueEvents, //(in/out)
	const UINT wIndex,     //(in)
	CCoordSet &killedPuffs,//(in/out) dead Fluff Puffs that are consumed attempting to stop an expansion
	CCoordStack &powder_kegs) //(in/out) powder kegs to detonate
{
	ASSERT(wIndex);
	ASSERT(wIndex <= this->briarComponents.size());
	CCoordSet& edge = this->briarEdge[wIndex-1];
	if (!edge.empty())
		return; //not ready to expand until all edge tiles have been processed

	CCurrentGame *pCurrentGame = this->pRoom->GetCurrentGame();
	ASSERT(pCurrentGame);

	//Expand each tile of the briar, if possible, according to the rules below.
	CCoordSet& briar = this->briarComponents[wIndex-1];
	CCoordSet *pBriar = &(this->briarComponents[wIndex-1]);
	CCoordSet addedEdge, addedBriar, oldAddedBriar; //joining to stagnant briar tiles
	do {
		for (CCoordSet::const_iterator tile=pBriar->begin(); tile!=pBriar->end(); ++tile)
		{
			//Expand from one tile to adjacent tiles.
			bool bAdjacentPit = false, bBlocked;
			const UINT wSrcTile = this->pRoom->GetTSquare(tile->wX,tile->wY);
			const UINT wSrcFTile = this->pRoom->GetFSquare(tile->wX,tile->wY);
			for (UINT i=0; i<NUM_NEIGHBORS; ++i)
			{
				if (i==4 && bAdjacentPit)
					break; //when a pit is axially adjacent, don't expand diagonally
				const UINT wX = tile->wX + nOX[i], wY = tile->wY + nOY[i];
				if (!this->pRoom->IsValidColRow(wX,wY))
					continue;

				bBlocked = false;
				const UINT wTTile = this->pRoom->GetTSquare(wX,wY);
				switch (wTTile)
				{
					//These items block briar.
					case T_OBSTACLE:
						bBlocked = true;
					break;

					//Solid fluff damages briar.
					case T_FLUFF:
						if (i >= 4)
							continue; //don't harm diagonally adjacent fluff

						//Briar burned by fluff
						bAdjacentPit = true;
						if (wSrcTile == T_BRIAR_DEAD) //this tile becomes an edge again
						{
							this->pRoom->Plot(tile->wX,tile->wY,T_BRIAR_LIVE);
							edge.insert(tile->wX,tile->wY);
						}
					continue;

					case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
					{
						const UINT wAdjIndex = this->briarIndices.GetAt(wX,wY);
						if (!wAdjIndex)
						{
							//Stagnant briar that had been cut off from other briar.
							//Add it to this one now.
							if (wTTile == T_BRIAR_LIVE)
								addedEdge.insert(wX,wY);
							else
								addedBriar.insert(wX,wY);
							this->briarIndices.Add(wX,wY, wIndex);
							this->pRoom->Plot(CCoordSet(wX,wY)); //front end--update room tiles
						} else if (wAdjIndex != wIndex)
							this->connectedBriars.push_back(make_pair(CCoord(wX,wY),
								CCoord(tile->wX,tile->wY))); //this briar will join the other one
						bBlocked = true;
					}
					break;

					case T_LIGHT:
						this->pRoom->SetTParam(wX, wY, 0);
						CueEvents.Add(CID_LightToggled); // This will force static lighting regeneration
						break;

					//Briar passes over and covers everything else.  Including orbs.
					default: break;
				}

				//Force arrows in wrong direction stop expansion.
				const UINT destFTile = this->pRoom->GetFSquare(wX,wY);
				if (bIsArrow(wSrcFTile) && bIsArrowObstacle(wSrcFTile,nGetO(nOX[i],nOY[i]))) //source
					bBlocked = true;
				else if (bIsArrow(destFTile) && bIsArrowObstacle(destFTile,nGetO(nOX[i],nOY[i]))) //destination
					bBlocked = true;

				const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
				switch (wOTile)
				{
					//Briar grows into hole.
					case T_PIT: case T_PIT_IMAGE:
						if (i >= 4)
							continue; //don't expand into diagonally adjacent hole

						//Briars into hole.
						bAdjacentPit = true;
						if (wSrcTile == T_BRIAR_DEAD) //this tile becomes an edge again
						{
							this->pRoom->Plot(tile->wX,tile->wY,T_BRIAR_LIVE);
							edge.insert(tile->wX,tile->wY);
						}
					continue;

					//Briar expands onto room tile.
					case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
					case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD: case T_FLOOR_GRASS:
					case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
					case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
					case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE: case T_THINICE_SH:
					case T_PRESSPLATE: case T_GOO:
					case T_STAIRS: case T_STAIRS_UP:
					case T_WATER: case T_SHALLOW_WATER: case T_PLATFORM_W: case T_PLATFORM_P:
					case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
					case T_FLOOR_SPIKES: case T_FLUFFVENT:
					case T_FIRETRAP: case T_FIRETRAP_ON:
					case T_STEP_STONE:
					break;

					//Can't grow onto anything else.
					case T_HOT: //hot tiles prevent new growth
					default:
						bBlocked = true;
					break;
				}
				if (bBlocked)
					continue;

				//Non-air monsters are killed as briar covers them.
				CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX, wY);
				if (pMonster)
				{
					pMonster = pMonster->GetOwningMonster();
					switch (pMonster->wType)
					{
						//Serpents stop the briar (acting as a retaining wall).
						case M_SERPENT:
						case M_SERPENTG:
						case M_SERPENTB:
						case M_GENTRYII:
							bBlocked = true;
						break;

						//Puffs temporarily stop briar, but die as a result.
						case M_FLUFFBABY:
							//Flag Puff to be killed after expansion
							killedPuffs.insert(wX, wY);
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
				{
					pCurrentGame->SetDyingEntity(&pCurrentGame->swordsman);
					CueEvents.Add(CID_BriarKilledPlayer);
				}

				switch (wTTile) {
					case T_MIRROR:
						CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
					break;
					case T_POWDER_KEG:
						powder_kegs.Push(wX,wY);
					break;
					default:
						if (bIsTar(wTTile))
						{
							this->pRoom->RemoveStabbedTar(wX, wY, CueEvents);
							CueEvents.Add(CID_TarstuffDestroyed,
									new CMoveCoordEx(wX, wY, nGetO(nOX[i],nOY[i]), wTTile), true);
						}
					break;
				}

				this->pRoom->Plot(wX,wY,T_BRIAR_LIVE);
				edge.insert(wX,wY);
				ASSERT(!this->briarIndices.Exists(wX,wY));
				this->briarIndices.Add(wX,wY, wIndex);

				this->pRoom->RemoveFuse(wX,wY);

				//Briar kills monster once briar tile has been plotted.
				if (pMonster)
				{
					this->pRoom->GetCurrentGame()->CheckTallyKill(pMonster);
					this->pRoom->KillMonster(pMonster, CueEvents);
					//!!Won't get to see the move this monster just made since it's
					//getting removed from the monster list here.  Adding this cue event
					//will cause an effect updating the room tiles to fix a resulting problem:
					//Showing this effect may be desired, but note it's just a kludge
					//to get the monster's old square to be dirtied and refreshed.
					CueEvents.Add(CID_MonsterDiedFromStab, pMonster);

					if (pMonster->wType == M_ROCKGIANT)
						CRockGiant::Shatter(CueEvents, this->pRoom->GetCurrentGame(),
								pMonster->wX, pMonster->wY);
				}

				//Briar depresses pressure plates.  Handle them all at the end of the turn.
				if (wOTile == T_PRESSPLATE)
					this->pressurePlates.insert(wX,wY);
			}
		}

		//If there's stagnant briar, add it to this briar and expand from these stagnant tiles
		edge += addedEdge;
		addedEdge.clear();
		briar += addedBriar;
		oldAddedBriar = addedBriar;
		addedBriar.clear();
		pBriar = &oldAddedBriar;
	} while (!pBriar->empty());

	if (!killedPuffs.empty())
	{
		for (CCoordSet::const_iterator puff=killedPuffs.begin(); puff!=killedPuffs.end(); ++puff)
		{
			CMonster *pMonster = this->pRoom->GetMonsterAtSquare(puff->wX,puff->wY);
			if (pMonster)
			{
				ASSERT(pMonster->wType == M_FLUFFBABY);
				this->pRoom->KillMonster(pMonster, CueEvents);
				this->pRoom->ProcessPuffAttack(CueEvents,puff->wX,puff->wY);
			}
		}
	}
}

//*****************************************************************************
const CCoordSet& CBriars::getEdgeTilesFor(const UINT wX, const UINT wY) const
//Returns: set of edge tiles connected to component at (x,y)
{
	const UINT index = getIndexAt(wX, wY);
	if (!index) {
		static const CCoordSet empty;
		return empty;
	}
	ASSERT(this->briarEdge.size() >= index);
	return this->briarEdge[index-1];
}

//*****************************************************************************
UINT CBriars::getIndexAt(const UINT wX, const UINT wY) const
//Returns: index of briar at (x,y)
{
	if (!this->briarIndices.GetSize())
		return 0; //no briar roots in room
	return this->briarIndices.GetAt(wX, wY);
}

//*****************************************************************************
UINT CBriars::getNumSourcesWithIndex(const UINT index) const
//Returns: the number of (connected) roots having this index
{
	UINT count = 0;
	for (list<CBriar*>::const_iterator briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		if ((*briar)->wComponentIndex == index)
			++count;
	return count;
}

//*****************************************************************************
void CBriars::initLiveTiles()
{
	ASSERT(this->pRoom);

	CTileMask briarMask;
	briarMask.set(T_BRIAR_SOURCE);
	briarMask.set(T_BRIAR_LIVE);
	briarMask.set(T_BRIAR_DEAD);

	CCoordSet ignoredTiles;
	for (list<CBriar*>::const_iterator it = this->briars.begin(); it != this->briars.end(); ++it)
	{
		CBriar& briar = **it;
	
		CCoordSet tiles;
		this->pRoom->GetConnected8NeighborTilesWithoutAxialPit(briar.wX, briar.wY, briarMask, tiles, &ignoredTiles);

		if (tiles.empty())
			continue;

		ignoredTiles += tiles; //don't process these tiles again

		CCoordSet edges, dead;
		CCoordSet::const_iterator tile;
		for (tile=tiles.begin(); tile!=tiles.end(); ++tile)
		{
			const UINT tTile = this->pRoom->GetTSquare(tile->wX, tile->wY);
			if (tTile == T_BRIAR_LIVE || tTile == T_BRIAR_DEAD) {
				this->briarIndices.Add(tile->wX, tile->wY, briar.wComponentIndex);
				if (tTile == T_BRIAR_LIVE)
					edges.insert(tile->wX, tile->wY);
				else
					dead.insert(tile->wX, tile->wY);
			}
		}

		ASSERT(briar.wComponentIndex > 0);
		this->briarEdge[briar.wComponentIndex-1] = edges;
		this->briarComponents[briar.wComponentIndex-1] += dead;
	}
}

//*****************************************************************************
void CBriars::insert(const UINT wX, const UINT wY)
//Add a briar root at (x,y)
{
	ASSERT(!this->briarIndices.Exists(wX,wY)); //this root should not have been processed yet

	//Each briar root starts with its own separate connected component of briar tiles.
	this->briarComponents.push_back(CCoordSet(wX, wY));
	this->briarEdge.push_back(CCoordSet());
	const UINT wIndex = this->briarComponents.size();
	this->briarIndices.Add(wX, wY, wIndex);

	CBriar *pBriar = new CBriar(this, wIndex, wX, wY);
	this->briars.push_back(pBriar);

	//If another briar root is adjacent, join its connected component to this root.
	bool bAdjacentPit = false;
	for (UINT i=0; i<NUM_NEIGHBORS; ++i)
	{
		if (i==4 && bAdjacentPit)
			break; //when a pit is axially adjacent, don't join to diagonally adjacent roots
		const UINT wAdjX = wX + nOX[i], wAdjY = wY + nOY[i];
		if (!this->pRoom->IsValidColRow(wAdjX,wAdjY))
			continue;

		const UINT wOTile = this->pRoom->GetOSquare(wAdjX,wAdjY);
		switch (wOTile)
		{
			//Holes prevent roots from connecting diagonally.
			case T_PIT: case T_PIT_IMAGE: case T_PLATFORM_P:
				bAdjacentPit = true;
			break;
		}

		const UINT wTTile = this->pRoom->GetTSquare(wAdjX,wAdjY);
		if (wTTile != T_BRIAR_SOURCE)
			continue;

		const UINT wAdjIndex = this->briarIndices.GetAt(wAdjX,wAdjY);
		if (!wAdjIndex)
			continue; //this root hasn't been processed yet -- ignore it for now
		if (wAdjIndex == wIndex)
			continue; //this root has already been joined to this one

		//Join the adjacent briar root(s) to this one.
		this->briarIndices.Replace(wAdjIndex, wIndex);
		for (list<CBriar*>::iterator briar = this->briars.begin(); briar != this->briars.end(); ++briar)
			if ((*briar)->wComponentIndex == wAdjIndex)
				(*briar)->wComponentIndex = wIndex;
		//Combine connected components.  Edges are still empty and can be ignored.
		this->briarComponents[wIndex-1] += this->briarComponents[wAdjIndex-1];
		this->briarComponents[wAdjIndex-1].clear();
	}
}

//*****************************************************************************
void CBriars::plotted(const UINT wX, const UINT wY, const UINT wTileNo)
//Room object calls this to notify briars that room geometry has changed at (x,y)
{
	if (bIsBriar(wTileNo))
		return; //plotting briar itself shouldn't affect anything

	//If briars previously stopped, this allows them to try to advance again.
	list<CBriar*>::iterator briar;
	for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		(*briar)->bStuck = false;

	//If a door closes here or the t-layer is modified (i.e. briar is overwritten),
	//synch data structures for any briar that used to be here.
	if (!bIsDoor(wTileNo) && TILE_LAYER[wTileNo] != 1) //t-layer plots affect briars
		return;

	vector<CCoordSet>::iterator tiles;
	for (tiles = this->briarComponents.begin();
			tiles != this->briarComponents.end(); ++tiles)
		if (tiles->has(wX,wY))
		{
			this->bRecalc = true; //when non-edge briar is cut, connected components must be recalced

			tiles->erase(wX,wY);
			this->briarIndices.Remove(wX,wY);
		}
	for (tiles = this->briarEdge.begin();
			tiles != this->briarEdge.end(); ++tiles)
		if (tiles->has(wX,wY))
		{
			this->bRecalc = true; //in some specific situations, two roots may have been connected only by an edge tile 

			tiles->erase(wX,wY);
			this->briarIndices.Remove(wX,wY);
		}

	//Door closed -- remove briar (that is unconnected from any roots) from this tile.
	if (bIsDoor(wTileNo) && bIsBriar(this->pRoom->GetTSquare(wX,wY)))
		this->pRoom->Plot(wX,wY,T_EMPTY);
}

//*****************************************************************************
void CBriars::process(
//Process each briar root.
//
//Params:
	CCueEvents &CueEvents)     //(in/out)
{
	if (this->briars.empty())
		return;

	ASSERT(this->pRoom);

	//Preprocessing.
	list<CBriar*>::iterator briar;
	for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		(*briar)->bDone = false; //briar has not filled in a tile yet

	if (this->bRecalc) //need to recompute connected components
	{
		this->bRecalc = false;

		//Determine which briar tiles are still connected to briar roots.
		static CTileMask briarMask;
		if (briarMask.empty())
		{
			briarMask.set(T_BRIAR_SOURCE);
			briarMask.set(T_BRIAR_DEAD);
			briarMask.set(T_BRIAR_LIVE);
		}

		//Keep old connected component info for reference while restructuring below.
		CCoordIndex_T<USHORT> oldBriarIndices = this->briarIndices;
		std::vector<CCoordSet> oldBriarComponents = this->briarComponents;
		std::vector<CCoordSet> oldBriarEdges = this->briarEdge;

		this->briarComponents.clear();
		this->briarEdge.clear();
		this->briarIndices.Clear();

		//Recalculate each root's connected component.
		CCoordSet preBriarComponent;
		for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		{
			CBriar *pBriar = *briar;
			CBriar& briarObj = *pBriar;

			//Is this root a part of a connected briar component just recalculated?
			if (this->briarIndices.Exists(briarObj.wX, briarObj.wY))
			{
				//This briar root's connected component has already been
				//calculated and is found joined to at least one other root.
				briarObj.wComponentIndex = this->briarIndices.GetAt(briarObj.wX, briarObj.wY);
				continue; //Nothing more needs to be done for it.
			}

			//Calculate this briar root's connected component.
			const USHORT oldIndex = oldBriarIndices.GetAt(briarObj.wX, briarObj.wY);
			CCoordSet oldBriarCoords = oldBriarComponents[oldIndex - 1];
			oldBriarCoords += oldBriarEdges[oldIndex - 1];

			briarObj.wComponentIndex = this->briarComponents.size()+1;
			this->pRoom->GetConnected8NeighborTiles(briarObj.wX, briarObj.wY, briarMask, preBriarComponent, NULL, &oldBriarCoords);

			//Sort tiles into filled and edge tiles.
			CCoordSet edges, briarComponent;
			
			for (CCoordSet::const_iterator tile=preBriarComponent.begin(); tile!=preBriarComponent.end(); ++tile)
			{
				//Only tiles that were part of this root's component before
				//recalculation should be included.  That is, no adjacent pieces
				//not belonging to this component already should be attached now.
				if (oldBriarIndices.GetAt(tile->wX, tile->wY) == oldIndex)
				{
					//Readd this tile to the connected component.
					this->briarIndices.Add(tile->wX, tile->wY, briarObj.wComponentIndex);
					briarComponent.insert(tile->wX, tile->wY);
					if (this->pRoom->GetTSquare(tile->wX, tile->wY) == T_BRIAR_LIVE)
						edges.insert(tile->wX, tile->wY);
				}
			}

			//This is the current state of this briar root's connected component.
			this->briarEdge.push_back(edges);

			//Before we subtract edges from briarComponent, we need to remove
			//all previous components from the coord set.
			if (oldIndex)
				edges -= oldBriarComponents[oldIndex-1];
			briarComponent -= edges;

			this->briarComponents.push_back(briarComponent);
		}
	}

	//Process all briar roots together in three synchronous steps.
	CCoordStack powder_kegs;
	do {
		//1. Try to mark a tile.
		bool bDone = true;
		for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
			if ((*briar)->process())
				bDone = false;
		if (bDone)
			break; //no briars need to expand at this point

		//2. Expand each briar root.
		for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		{
			CBriar *pBriar = *briar;
			//Is the briar ready to expand into new tiles?
			if (!pBriar->bDone && !pBriar->bStuck)
			{
				CCoordSet killedPuffs;
				expand(CueEvents, pBriar->wComponentIndex, killedPuffs, powder_kegs);

				CCoordSet& edge = this->briarEdge[pBriar->wComponentIndex-1];
				if (!killedPuffs.empty())
					pBriar->bDone = true;
				else if (edge.empty())
					pBriar->bStuck = true; //nowhere to expand to, until we try to connect in Step 3.
			}
		}

		//3. Join connected briar components.
		//If briar root came into contact with briar from another root, combine them.
		for (vector<CoordPair>::const_iterator connectedBriarPair = this->connectedBriars.begin();
				connectedBriarPair != this->connectedBriars.end(); ++connectedBriarPair)
		{
			const CoordPair& coordPair = *connectedBriarPair;

			//Check whether briars have been joined yet.
			const UINT wIndex1 = this->briarIndices.GetAt(coordPair.first.wX,coordPair.first.wY);
			const UINT wIndex2 = this->briarIndices.GetAt(coordPair.second.wX,coordPair.second.wY);
			if (wIndex1 == wIndex2)
				continue; //already joined

			//Combine briars.
			this->pRoom->Plot(this->briarComponents[wIndex2-1]); //front end--update room tiles
			this->pRoom->Plot(this->briarEdge[wIndex2-1]);

			this->briarComponents[wIndex1-1] += this->briarComponents[wIndex2-1];
			this->briarComponents[wIndex2-1].clear();
			this->briarEdge[wIndex1-1] += this->briarEdge[wIndex2-1];
			this->briarEdge[wIndex2-1].clear();
			this->briarIndices.Replace(wIndex2, wIndex1);
			for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
			{
				CBriar& component = *(*briar);
				if (component.wComponentIndex == wIndex2)
				{
					component.wComponentIndex = wIndex1;
					component.bStuck = false; //no longer stuck
				} else if (component.wComponentIndex == wIndex1)
					component.bStuck = false; //no longer stuck
			}
		}
		this->connectedBriars.clear();

		//4. Any briar root that is still stuck is now done processing.
		for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
		{
			CBriar *pBriar = *briar;
			if (pBriar->bStuck)
				pBriar->bDone = true;
		}
	} while (true); //repeat until all briars are done

	if (!powder_kegs.IsEmpty()) {
		CCoordStack no_bombs;
		pRoom->DoExplode(CueEvents, no_bombs, powder_kegs);
	}

	//Convert unstable tar to tar babies
	this->pRoom->ConvertUnstableTar(CueEvents, true);

	//Process any pressure plates depressed now.
	for (CCoordSet::const_iterator plate=this->pressurePlates.begin();
			plate!=this->pressurePlates.end(); ++plate)
	{
		this->pRoom->ActivateOrb(plate->wX, plate->wY, CueEvents, OAT_PressurePlate);
	}
	this->pressurePlates.clear();
}

//*****************************************************************************
void CBriars::removeSource(const UINT wX, const UINT wY)
//Removes briar root at (x,y).
{
	list<CBriar*>::iterator briar;
	for (briar = this->briars.begin(); briar != this->briars.end(); ++briar)
	{
		CBriar *pBriar = *briar;
		if (pBriar->wX == wX && pBriar->wY == wY)
		{
			delete pBriar;
			this->briars.erase(briar);
			this->briarIndices.Remove(wX,wY);
			return;
		}
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

	this->briarIndices.Init(pRoom->wRoomCols, pRoom->wRoomRows);
}
