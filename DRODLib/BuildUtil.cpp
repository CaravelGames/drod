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
* 1997, 2000, 2001, 2002, 2005, 2016 Caravel Software. All Rights Reserved.
*
* Contributor(s):
*
* ***** END LICENSE BLOCK ***** */

#include "BuildUtil.h"
#include "CurrentGame.h"

//*****************************************************************************
bool BuildUtil::bIsValidBuildTile(const UINT wTileNo)
{
	switch (wTileNo)
	{
	case T_OBSTACLE:
	case T_PRESSPLATE:
	case T_SNKT_E:
	case T_SNKT_N:
	case T_SNKT_S:
	case T_SNKT_W:
	case T_SNK_EW:
	case T_SNK_NE:
	case T_SNK_NS:
	case T_SNK_NW:
	case T_SNK_SE:
	case T_SNK_SW:
	case T_CHECKPOINT:
	case T_LIGHT_CEILING:
	case T_PLATFORM_W:
	case T_PLATFORM_P:
	case T_WALL_M:
	case T_STAIRS_UP:
	case T_STAIRS:
	case T_DARK_CEILING:
	case T_WALLLIGHT:
	case T_WALL_WIN:
	case T_ACTIVETOKEN:
	case T_TOKEN_TSPLIT_USED:
	case T_TOKEN:
	case T_PLATE_ONEUSE:
	case T_PLATE_MULTI:
	case T_PLATE_ON_OFF:
		return false;
	default:
		return true;
	}
}

//*****************************************************************************
void BuildUtil::BuildTilesAt(CDbRoom& room, const UINT tile, UINT px, UINT py, const UINT pw, const UINT ph, const bool bAllowSame, CCueEvents& CueEvents)
{
	if (!bIsValidBuildTile(tile))
		return;

	UINT endX = px + pw;
	UINT endY = py + ph;

	if (!room.CropRegion(px, py, endX, endY))
		return;

	const UINT baseTile = bConvertFakeElement(tile);

	for (UINT y = py; y <= endY; ++y) {
		for (UINT x = px; x <= endX; ++x)
		{
			BuildUtil::BuildAnyTile(room, baseTile, tile, x, y, bAllowSame, CueEvents);
		}
	}
}

//*****************************************************************************
bool BuildUtil::BuildTileAt(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents)
{
	if (!bIsValidBuildTile(tile))
		return false;

	if (x >= room.wRoomCols || y >= room.wRoomRows)
		return false; //build area is completely out of room bounds -- do nothing

	const UINT baseTile = bConvertFakeElement(tile);

	return BuildUtil::BuildAnyTile(room, baseTile, tile, x, y, bAllowSame, CueEvents);
}

//*****************************************************************************
bool BuildUtil::CanBuildAt(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame)
{	
	if (x >= room.wRoomCols || y >= room.wRoomRows)
		return false; //build area is completely out of room bounds -- do nothing

	bool bValid = true;
	const UINT baseTile = bConvertFakeElement(tile);
	if (IsValidTileNo(baseTile))
	{
		switch (TILE_LAYER[baseTile])
		{
			case LAYER_OPAQUE:
				//Don't build if this element is already there.
				switch (baseTile) {
					default:
						if (room.GetOSquare(x, y) == baseTile)
							bValid = bAllowSame;
					break;
					case T_OVERHEAD_IMAGE:
						if (room.overheadTiles.Exists(x,y))
							bValid = bAllowSame;
					break;
				}
			break;
			case LAYER_TRANSPARENT:
			{
				//Don't build if this element is already there.
				//FIXME?: Currently tokens must be removed before building a different token type over it
				const UINT wTTile = room.GetBottomTSquare(x,y);
				if (wTTile == baseTile)
				{
					bValid = bAllowSame;
					break;
				}

				//Cannot add/remove these
				if (baseTile == T_OBSTACLE || wTTile == T_OBSTACLE)
					bValid = false;


				if (baseTile == T_ORB){
					const UINT wOTile = room.GetOSquare(x, y);

					if (wOTile == T_PRESSPLATE)
						bValid = false;
				}

				//Fuses, bombs, potions and also briars can be replaced.
				if (wTTile == T_EMPTY || bIsCombustibleItem(wTTile) ||
						bIsTLayerCoveringItem(wTTile) ||
						bIsPotion(wTTile) || bIsBriar(wTTile))
					break;

				// Allow covering Tlayer tiles
				if (bIsTLayerCoveringItem(baseTile) && bIsTLayerCoverableItem(wTTile))
					break;

				//No other item can be built over.
				bValid = false;
			}
			break;
			case LAYER_FLOOR:
				if (room.GetFSquare(x, y) == baseTile){
					bValid = bAllowSame;
				}
			break;
			default:
				ASSERT(!"Unsupported build layer");
				bValid = false;
			break;
		}
	}
	else {
		switch (baseTile)
		{
			case T_REMOVE_OVERHEAD_IMAGE:
				if (!room.overheadTiles.Exists(x, y))
					bValid = false;
				break;
			case T_EMPTY_F:
			case T_REMOVE_FLOOR_ITEM:
				if (!room.GetFSquare(x, y))
					bValid = false;
				break;
		}
	}
	return bValid;
}

//*****************************************************************************
bool BuildUtil::BuildAnyTile(CDbRoom& room, const UINT baseTile, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents)
{
	switch (tile)
	{
		case T_REMOVE_BUILD_MARKER:
			if (room.building.get(x, y)){
				room.building.remove(x, y);
				return true;
			}
		return false;
		case T_OVERHEAD_IMAGE:
			if (!room.overheadTiles.Exists(x, y)){
				room.overheadTiles.Add(x, y);
				room.Plot(CCoordSet(x, y));
				return true;
			}
		return false;
		case T_REMOVE_OVERHEAD_IMAGE:
			if (room.overheadTiles.Exists(x, y)){
				room.overheadTiles.Remove(x, y);
				return true;
			}
		return false;
		case T_ORB:
			if (BuildNormalTile(room, T_ORB, T_ORB, x, y, true, CueEvents)){
				COrbData *pOrb = room.GetOrbAtCoords(x, y);
				if (pOrb == NULL && room.GetTSquare(x, y) == T_ORB)
				{
					pOrb = room.AddOrbToSquare(x, y);
				}
				pOrb->eType = OT_NORMAL;
				room.ForceTileRedraw(x, y, false);
				return true;
			}
		return false;
		case T_ORB_CRACKED:
			if (BuildNormalTile(room, T_ORB, T_ORB, x, y, true, CueEvents)){
				COrbData *pOrb = room.GetOrbAtCoords(x, y);
				if (pOrb == NULL && room.GetTSquare(x, y) == T_ORB)
				{
					pOrb = room.AddOrbToSquare(x, y);
				}
				pOrb->eType = OT_ONEUSE;
				room.ForceTileRedraw(x, y, false);
				return true;
			}
		return false;
		case T_ORB_BROKEN:
			if (BuildNormalTile(room, T_ORB, T_ORB, x, y, true, CueEvents)){
				COrbData *pOrb = room.GetOrbAtCoords(x, y);
				if (pOrb == NULL && room.GetTSquare(x, y) == T_ORB)
				{
					pOrb = room.AddOrbToSquare(x, y);
				}
				pOrb->eType = OT_BROKEN;
				room.ForceTileRedraw(x, y, false);
				return true;
			}
		return false;
		default:
			return BuildNormalTile(room, baseTile, tile, x, y, bAllowSame, CueEvents);
	}
}

//*****************************************************************************
bool BuildUtil::BuildNormalTile(CDbRoom& room, const UINT baseTile, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents)
{
	const CCurrentGame* pCurrentGame = room.GetCurrentGame();

	//Make sure there's no critical obstruction
	bool bValid = CanBuildAt(room, tile, x, y, bAllowSame);

	if (bIsWall(baseTile) || bIsCrumblyWall(baseTile) || bIsBriar(baseTile))
	{
		//If the build tile would fill a square, the tile must be vacant now.
		CMonster *pMonster = room.GetMonsterAtSquare(x, y);
		// Allow building walls on top of Gentryii chain
		const bool bIsGentryiiChainBuildable = pMonster
			&& pMonster->wType == M_GENTRYII
			&& pMonster->IsPiece()
			&& (bIsWall(baseTile) || bIsCrumblyWall(baseTile));

		const bool bMonsterBlocksBuild = pMonster && !bIsGentryiiChainBuildable;
		if (bMonsterBlocksBuild || (pCurrentGame->swordsman.wX == x &&
			pCurrentGame->swordsman.wY == y))
		{
			//Build tile is occupied and invalid.
			bValid = false;
		}
	}

	if (!bValid)
		return false;

	const UINT wOldOTile = room.GetOSquare(x, y);
	if (baseTile == wOldOTile && !bAllowSame)
		return false;

	const UINT wLayer = TILE_LAYER[baseTile];
	if (wLayer == LAYER_OPAQUE)
	{
		//Update room's trapdoor stats when a trapdoor is removed this special way.
		if (!bIsTrapdoor(baseTile) && bIsTrapdoor(wOldOTile))
			room.DecTrapdoor(CueEvents);
		else if (bIsTrapdoor(baseTile) && !bIsTrapdoor(wOldOTile))
			room.IncTrapdoor(CueEvents);

		//Update platform component.
		if (bIsPlatform(wOldOTile))
		{
			ASSERT(baseTile != wOldOTile);
			CPlatform *pPlatform = room.GetPlatformAt(x, y);
			ASSERT(pPlatform);
			pPlatform->RemoveTile(x, y);
		}
		//Update orb data if a pressure plate or yellow door is removed.
		else if (wOldOTile == T_PRESSPLATE) {
			room.RemovePressurePlateTile(x, y);
		}
		else if (bIsYellowDoor(wOldOTile) && !bIsYellowDoor(baseTile)) {
			room.RemoveYellowDoorTile(x, y, wOldOTile);
		}
	}
	else if (wLayer == LAYER_TRANSPARENT)
	{
		const UINT wOldTTile = room.GetTSquare(x, y);
		if (bIsTar(wOldTTile) && baseTile != wOldTTile)
		{
			if (room.StabTar(x, y, CueEvents, true, NO_ORIENTATION))
				room.ConvertUnstableTar(CueEvents);
		} else if (wOldTTile == T_FLUFF && baseTile != wOldTTile) {
			room.DestroyFluff(x, y, CueEvents);
		} else if (baseTile == T_BRIAR_SOURCE && baseTile != wOldTTile) {
			room.briars.insert(x, y);
			room.briars.forceRecalc();
		} else if (wOldTTile == T_BRIAR_SOURCE && baseTile != wOldTTile) {
			room.briars.removeSource(x, y);
			room.briars.forceRecalc();
		} else if (wOldTTile == T_BRIAR_LIVE && baseTile == T_BRIAR_DEAD) {
			//this case isn't handled in briars.plotted() via room plot below
			room.briars.forceRecalc();
		} else if (baseTile == T_STATION) {
			room.stations.push_back(new CStation(x, y, &room));
		}
	}

	//When water, ice or doors are plotted (or overwritten), redraw edges.
	//WARNING: Where plots are needed is front-end implementation dependent.
	if (
		bIsWater(baseTile)
		|| bIsWater(wOldOTile)
		|| bIsThinIce(baseTile)
		|| bIsThinIce(wOldOTile)
		|| bIsDoor(wOldOTile)
		|| bIsDoor(baseTile))
	{
		CCoordSet plots;
		for (int nJ = -1; nJ <= 1; ++nJ){
			for (int nI = -1; nI <= 1; ++nI){
				if ((nI || nJ) && room.IsValidColRow(x + nI, y + nJ)){
					plots.insert(x + nI, y + nJ);
				}
			}
		}
		room.Plot(plots, true);
	}

	//When pit is added/removed, redraw this tile's pit edge.
	if (bIsPit(baseTile) || bIsPit(wOldOTile))
	{
		CCoordSet plots;
		UINT wY = y + 1;
		//Down.
		while (wY < room.wRoomRows &&
			bIsPit(room.GetOSquare(x, wY)))
		{
			plots.insert(x, wY);
			++wY;
		}

		//Left.
		UINT wX = x - 1;
		wY = y;
		while (wX < room.wRoomCols && bIsPit(room.GetOSquare(wX, wY)))
		{
			plots.insert(wX, wY);
			--wX;
		}
		//Right.
		wX = x + 1;
		while (wX < room.wRoomCols && bIsPit(room.GetOSquare(wX, wY)))
		{
			plots.insert(wX, wY);
			++wX;
		}

		//Down a row, to left and right.
		if (++wY < room.wRoomRows)
		{
			wX = x - 1;
			while (wX < room.wRoomCols && bIsPit(room.GetOSquare(wX, wY)))
			{
				plots.insert(wX, wY);
				--wX;
			}
			wX = x + 1;
			while (wX < room.wRoomCols && bIsPit(room.GetOSquare(wX, wY)))
			{
				plots.insert(wX, wY);
				++wX;
			}
		}

		room.Plot(plots, true);
	}

	if (bIsTarOrFluff(baseTile)){
		if (pCurrentGame->swordsman.wX == x &&
			pCurrentGame->swordsman.wY == y){
			return false;
		}

		if (pCurrentGame->pRoom->GetMonsterAtSquare(x, y) != NULL){
			return false;
		}

		room.bTarWasBuilt = true;

		if (bIsTar(baseTile) && !bIsTar(room.GetTSquare(x, y))){
			if (room.wTarLeft == 0){
				room.ToggleBlackGates(CueEvents);
			}
			++room.wTarLeft;
		}
	}

	room.Plot(x, y, baseTile, NULL, true);

	if (bIsSolidOTile(baseTile) || baseTile == T_HOT)
	{
		room.DestroyFluff(x, y, CueEvents);
		room.ConvertUnstableTar(CueEvents);
	}

	//Tokens need to have their correct type set
	if (baseTile == T_TOKEN)
	{
		BYTE tTokenType = (BYTE)ConvertFakeTokenType(tile);
		switch (tile)
		{
		case T_TOKEN_DISARM:
			if (pCurrentGame->swordsman.bNoWeapon) 
				tTokenType += TOKEN_ACTIVE;
			break;
		case T_TOKEN_POWER:
			if (pCurrentGame->swordsman.bCanGetItems) 
				tTokenType += TOKEN_ACTIVE;
			break;
		case T_TOKEN_CONQUER:
			if (pCurrentGame->conquerTokenTurn != NO_CONQUER_TOKEN_TURN) 
				tTokenType += TOKEN_ACTIVE;
			break;
		case T_TOKEN_VISION:
			if (room.bBetterVision) 
				tTokenType += TOKEN_ACTIVE;
			break;
		default: break;
		}
		room.SetTParam(x, y, tTokenType);
	}

	if (bIsSolidOTile(baseTile) || baseTile == T_HOT)
	{
		room.DestroyFluff(x, y, CueEvents);
		room.ConvertUnstableTar(CueEvents);
	}

	//When placing a hole, things might fall - but not on Turn 0.
	if ((bIsPit(baseTile) || bIsWater(baseTile) || wLayer == LAYER_TRANSPARENT) &&
		pCurrentGame->wTurnNo > 0)
	{
		room.CheckForFallingAt(x, y, CueEvents);
		room.ConvertUnstableTar(CueEvents);
	}

	if (wOldOTile == T_PRESSPLATE && room.PressurePlateIsDepressedBy(baseTile)){
		room.ActivateOrb(x, y, CueEvents, OAT_PressurePlate);
	}

	CueEvents.Add(CID_ObjectBuilt, new CAttachableWrapper<UINT>(baseTile), true);
	
	//When o-layer changes, refresh bridge supports.
	if (wLayer == LAYER_OPAQUE) {
		const UINT newTile = (bIsShallowWater(baseTile) && bIsSteppingStone(room.GetOSquare(x, y))) ? T_STEP_STONE : baseTile;
		room.bridges.built(x, y, newTile);
	}

	if (room.building.get(x, y))
		room.building.remove(x, y);

	return true;
}
