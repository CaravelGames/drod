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
	case T_STAIRS_UP:
	case T_STAIRS:
	case T_DARK_CEILING:
	case T_WALLLIGHT:
		return false;

	//The virtual item types need to be specified for the following.
	case T_TOKEN:
	case T_KEY:
	case T_SWORD:
	case T_SHIELD:
	case T_ACCESSORY:
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

	//Crop check to valid room region
	UINT endX = px + pw;
	UINT endY = py + ph;
	if (!room.CropRegion(px, py, endX, endY))
		return;

	const bool bRealTile = IsValidTileNo(tile) || tile == T_EMPTY_F;
	const bool bVirtualTile = IsVirtualTile(tile);
	if (!bRealTile && !bVirtualTile)
		return; //unrecognized tile ID

	for (UINT y = py; y <= endY; ++y)
	{
		for (UINT x = px; x <= endX; ++x)
		{
			if (bVirtualTile) {
				BuildUtil::BuildVirtualTile(room, tile, x, y, bAllowSame, CueEvents);
			} else {
				BuildUtil::BuildRealTile(room, tile, x, y, bAllowSame, CueEvents);
			}
		}
	}
}

//*****************************************************************************
bool BuildUtil::CanBuildAt(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame)
{
	if (x >= room.wRoomCols || y >= room.wRoomRows)
		return false; //build area is completely out of room bounds -- do nothing

	bool bValid = true;
	switch (TILE_LAYER[tile])
	{
	case LAYER_OPAQUE:
	{
		//Don't build if this element is already there.
		const UINT wOTile = room.GetOSquare(x, y);
		if (wOTile == tile)
			bValid = bAllowSame;

		//Adding tiles to platforms is not currently supported by the engine.
		if (bIsPlatform(tile))
			return false;

		if (bIsPlatform(wOTile)) {
			// Removing tiles from platforms is currently not supported
			// by the saved game mechanism.
			// (In next major version, would need to upgrade saved room data
			//  to store explicit set of tiles for each platform, not platformDeltas.)
			ASSERT(tile != wOTile);
			CPlatform* pPlatform = room.GetPlatformAt(x, y);
			ASSERT(pPlatform);
			bValid = false; //pPlatform->RemoveTile(x, y);
		}
	}
	break;
	case LAYER_TRANSPARENT:
	{
		//Don't build if this element is already there.
		const UINT wTTile = room.GetTSquare(x, y);
		if (wTTile == tile)
		{
			bValid = bAllowSame;
			break;
		}

		//Obstacles may not be built -- only queried in IsTileAt.
		if (tile == T_OBSTACLE)
			bValid = false;

		//Can't build orbs on pressure plates.
		if (tile == T_ORB && room.GetOSquare(x, y) == T_PRESSPLATE)
			bValid = false;

		//Most items can be replaced.
		if (wTTile == T_EMPTY || wTTile == T_BOMB || wTTile == T_FUSE ||
			bIsPowerUp(wTTile) || bIsBriar(wTTile) || wTTile == T_MIRROR ||
			bIsEquipment(wTTile) || wTTile == T_KEY || wTTile == T_LIGHT ||
			wTTile == T_SCROLL || wTTile == T_MAP || wTTile == T_ORB ||
			wTTile == T_TOKEN || bIsTar(wTTile))
			break;
		//No other item can be built over.
		bValid = false;
	}
	break;
	case LAYER_FLOOR:
	{
		const UINT wFTile = room.GetFSquare(x, y);
		if (wFTile == tile) {
			bValid = bAllowSame;
		}
		//Can replace any item trivially on this layer.
	}
	break;
	default: ASSERT(!"Unsupported build layer");
		//no break
	case LAYER_MONSTER:
		bValid = false;
		break;
	}

	return bValid;
}

//*****************************************************************************
//Virtual tiles are plotted in special ways.
bool BuildUtil::BuildVirtualTile(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents)
{
	UINT newTile = T_EMPTY;
	switch (tile)
	{
		case TV_KEY_Y: case TV_KEY_G: case TV_KEY_B: case TV_KEY_S:
			newTile = T_KEY;
			break;
		case TV_SWORD1: case TV_SWORD2: case TV_SWORD3: case TV_SWORD4: case TV_SWORD5:
		case TV_SWORD6: case TV_SWORD7: case TV_SWORD8: case TV_SWORD9: case TV_SWORD10:
			newTile = T_SWORD;
			break;
		case TV_SHIELD1: case TV_SHIELD2: case TV_SHIELD3: case TV_SHIELD4: case TV_SHIELD5: case TV_SHIELD6:
			newTile = T_SHIELD;
			break;
		case TV_ACCESSORY1: case TV_ACCESSORY2: case TV_ACCESSORY3: case TV_ACCESSORY4:
		case TV_ACCESSORY5: case TV_ACCESSORY6: case TV_ACCESSORY7: case TV_ACCESSORY8:
		case TV_ACCESSORY9: case TV_ACCESSORY10: case TV_ACCESSORY11: case TV_ACCESSORY12:
			newTile = T_ACCESSORY;
			break;

		case TV_EXPLOSION:
		{
			const bool bBombHere = room.GetTSquare(x, y) == T_BOMB;
			room.ProcessExplosionSquare(CueEvents, x, y);
			if (bBombHere)
			{
				CCoordStack bombs(x, y);
				room.BombExplode(CueEvents, bombs);
			}
			room.ConvertUnstableTar(CueEvents);

			CueEvents.Add(CID_BombExploded, new CMoveCoord(x, y, 0), true);
		}
		return true;

		default: break;
	}

	if (newTile != T_EMPTY)
	{
		//Check for legal placement.
		const UINT wTTile = room.GetTSquare(x, y);

		//Most items can be replaced.
		//(Same logic as the t-layer check for real tiles.)
		if (wTTile == T_EMPTY || wTTile == T_BOMB || wTTile == T_FUSE ||
			bIsPowerUp(wTTile) || bIsBriar(wTTile) || wTTile == T_MIRROR ||
			bIsEquipment(wTTile) || wTTile == T_KEY || wTTile == T_LIGHT ||
			wTTile == T_SCROLL || wTTile == T_MAP || wTTile == T_ORB ||
			wTTile == T_TOKEN || bIsTar(wTTile))
		{
			if (TILE_LAYER[newTile] == LAYER_TRANSPARENT && bIsTar(wTTile) && newTile != wTTile) {
				room.RemoveStabbedTar(x, y, CueEvents);
				room.ConvertUnstableTar(CueEvents);
			}
			room.Plot(x, y, newTile);
		} else {
			return false;
		}
	}

	switch (tile) //2nd pass -- set T-layer param
	{
		case TV_KEY_Y: room.SetTParam(x, y, YellowKey); break;
		case TV_KEY_G: room.SetTParam(x, y, GreenKey); break;
		case TV_KEY_B: room.SetTParam(x, y, BlueKey); break;
		case TV_KEY_S: room.SetTParam(x, y, SkeletonKey); break;
		case TV_SWORD1: room.SetTParam(x, y, WoodenBlade); break;
		case TV_SWORD2: room.SetTParam(x, y, ShortSword); break;
		case TV_SWORD3: room.SetTParam(x, y, GoblinSword); break;
		case TV_SWORD4: room.SetTParam(x, y, LongSword); break;
		case TV_SWORD5: room.SetTParam(x, y, HookSword); break;
		case TV_SWORD6: room.SetTParam(x, y, ReallyBigSword); break;
		case TV_SWORD7: room.SetTParam(x, y, LuckySword); break;
		case TV_SWORD8: room.SetTParam(x, y, SerpentSword); break;
		case TV_SWORD9: room.SetTParam(x, y, BriarSword); break;
		case TV_SWORD10: room.SetTParam(x, y, WeaponSlot); break;
		case TV_SHIELD1: room.SetTParam(x, y, WoodenShield); break;
		case TV_SHIELD2: room.SetTParam(x, y, BronzeShield); break;
		case TV_SHIELD3: room.SetTParam(x, y, SteelShield); break;
		case TV_SHIELD4: room.SetTParam(x, y, KiteShield); break;
		case TV_SHIELD5: room.SetTParam(x, y, OremiteShield); break;
		case TV_SHIELD6: room.SetTParam(x, y, ArmorSlot); break;
		case TV_ACCESSORY1: room.SetTParam(x, y, GrapplingHook); break;
		case TV_ACCESSORY2: room.SetTParam(x, y, WaterBoots); break;
		case TV_ACCESSORY3: room.SetTParam(x, y, InvisibilityPotion); break;
		case TV_ACCESSORY4: room.SetTParam(x, y, SpeedPotion); break;
		case TV_ACCESSORY5: room.SetTParam(x, y, HandBomb); break;
		case TV_ACCESSORY6: room.SetTParam(x, y, PickAxe); break;
		case TV_ACCESSORY7: room.SetTParam(x, y, WarpToken); break;
		case TV_ACCESSORY8: room.SetTParam(x, y, PortableOrb); break;
		case TV_ACCESSORY9: room.SetTParam(x, y, LuckyGold); break;
		case TV_ACCESSORY10: room.SetTParam(x, y, WallWalking); break;
		case TV_ACCESSORY11: room.SetTParam(x, y, XPDoubler); break;
		case TV_ACCESSORY12: room.SetTParam(x, y, AccessorySlot); break;
		default: break; //nothing else to do here
	}

	CueEvents.Add(CID_ObjectBuilt, new CMoveCoord(x, y, tile), true);

	return true;
}

//*****************************************************************************
bool BuildUtil::BuildRealTile(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents)
{
	//Build if there is no critical obstruction.
	const bool bValid = CanBuildAt(room, tile, x, y, bAllowSame);
	if (!bValid)
		return false;

	const CCurrentGame* pCurrentGame = room.GetCurrentGame();
	if (bIsWall(tile) || bIsCrumblyWall(tile) || bIsBriar(tile))
	{
		//If the build tile would fill a square, the tile must be vacant now.
		CMonster* pMonster = room.GetMonsterAtSquare(x, y);
		if (pMonster || pCurrentGame->IsPlayerAt(x, y))
		{
			//Build tile is occupied and invalid.
			return false;
		}
	}

	const UINT wOldOTile = room.GetOSquare(x, y);
	if (tile == wOldOTile && !bAllowSame)
		return false;

	//Handle special bookkeeping.
	const UINT wLayer = TILE_LAYER[tile];
	if (wLayer == LAYER_OPAQUE)
	{
		const bool bReplacingPit = bIsPit(wOldOTile) && !bIsPit(tile);
		if ((bIsCrumblyWall(tile) && bIsWall(wOldOTile)) || //if a crumbly wall replaces a normal wall...
			bReplacingPit)                              //...or when replacing a pit with non-pit...
		{
			room.Plot(x, y, T_FLOOR);                        //...plot floor first to update the covered o-layer
		}
		//Update room's trapdoor stats when a trapdoor is removed this special way.
		else if (!bIsTrapdoor(tile) && bIsTrapdoor(wOldOTile))
			room.DecTrapdoor(CueEvents);
		else if (bIsTrapdoor(tile) && !bIsTrapdoor(wOldOTile)) {
			room.IncTrapdoor(CueEvents);
		}
		else if (!bIsPlatform(tile) && bIsPlatform(wOldOTile)) {
			ASSERT(tile != wOldOTile);
			CPlatform* pPlatform = room.GetPlatformAt(x, y);
			ASSERT(pPlatform);
			pPlatform->RemoveTile(x, y);
		}
		//Update orb data if a pressure plate or door is removed.
		else if (wOldOTile == T_PRESSPLATE) {
			room.RemovePressurePlateTile(x, y);
		}
		else if (bIsDoor(wOldOTile)) {
			room.RemoveDoorTile(x, y, wOldOTile);
		}

		//When water or doors are plotted (or overwritted), redraw edges.
		//WARNING: Where plots are needed is front-end implementation dependent.
		if (bIsWater(tile) || bIsWater(wOldOTile) || bIsDoor(wOldOTile) || bIsDoor(tile))
		{
			CCoordSet plots;
			for (UINT nx = x - 1; nx != x + 2; ++nx)
				if (nx < room.wRoomCols)
					for (UINT ny = y - 1; ny != y + 2; ++ny)
						if (ny < room.wRoomRows)
							plots.insert(nx, ny);
			room.Plot(plots);
		}
		//When pit is added/removed, redraw this tile's pit edge.
		//WARNING: Where plots are needed is front-end implementation dependent.
		if ((bIsFloor(wOldOTile) || bIsWall(wOldOTile) || bIsCrumblyWall(wOldOTile)) &&
			(bIsPlatform(tile) || bIsBridge(tile) || bIsTrapdoor(tile)))
		{
			CCoordSet plots;
			for (UINT ny = y + 1; ny < room.wRoomRows; ++ny) //all the way down
				plots.insert(x, ny);
			room.Plot(plots);
		}
	}
	else if (wLayer == LAYER_TRANSPARENT) {
		const UINT oldTTile = room.GetTSquare(x, y);
		if (bIsTar(oldTTile) && tile != oldTTile) {
			room.StabTar(x, y, CueEvents, true, NO_ORIENTATION);
		}
		else if (tile == T_BRIAR_SOURCE && tile != oldTTile) {
			room.briars.insert(x, y);
			//room.briars.forceRecalc();
		}
		else if (oldTTile == T_BRIAR_SOURCE && tile != oldTTile) {
			room.briars.removeSource(x, y);
			//room.briars.forceRecalc();
		}
		else if (oldTTile == T_BRIAR_LIVE && tile == T_BRIAR_DEAD) {
			//this case isn't handled in briars.plotted() via room plot below
			//room.briars.forceRecalc();
		}

		//Update room tarstuff state
		if (bIsTar(tile)) {
			if (pCurrentGame->IsPlayerAt(x, y)) {
				return false;
			}

			if (pCurrentGame->pRoom->GetMonsterAtSquare(x, y) != NULL) {
				return false;
			}

			//room.bTarWasBuilt = true;

			if (!bIsTar(room.GetTSquare(x, y))) {
				if (room.wTarLeft == 0) {
					room.ToggleBlackGates(CueEvents);
				}
				++room.wTarLeft;
			}
		}
	}

	room.Plot(x, y, tile);

	//When placing a hole, things might fall.
	if ((bIsPit(tile) || bIsWater(tile) || wLayer == LAYER_TRANSPARENT) &&
		pCurrentGame->wTurnNo > 0) //don't allow player falling on room entrance
	{
		room.CheckForFallingAt(x, y, CueEvents);
		room.ConvertUnstableTar(CueEvents);
	}

	//When o-layer changes, refresh bridge supports.
	if (wLayer == LAYER_OPAQUE)
		room.bridges.built(x, y, tile);

	CueEvents.Add(CID_ObjectBuilt, new CMoveCoord(x, y, tile), true);

	return true;
}
