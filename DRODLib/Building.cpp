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

#include "Building.h"
#include "TileConstants.h"

//*****************************************************************************
CBuilding::CBuilding()
{
}

//*****************************************************************************
void CBuilding::clear()
//Resets data members.
{
	this->tiles.Clear();
	this->tileSet.clear();
}

//*****************************************************************************
void CBuilding::init(const UINT wCols, const UINT wRows)
//Init data structures.
{
	this->tiles.Init(wCols, wRows);
}

//*****************************************************************************
UINT CBuilding::get(const UINT wX, const UINT wY) const
{
	return this->tiles.GetAt(wX,wY);
}

//*****************************************************************************
void CBuilding::plot(const UINT wX, const UINT wY, const UINT wTile)
//Set build tile at (x,y) to specified tile.
{
	//Only supported in immediate building at this time
	if (bIsFakeTokenType(wTile)) return;

	switch (wTile) {
		case T_REMOVE_BUILD_MARKER:
			remove(wX,wY);
		break;
		case T_OVERHEAD_IMAGE:
		case T_REMOVE_OVERHEAD_IMAGE:
		break; //do nothing -- only supported in immediate building
		case T_REMOVE_FLOOR_ITEM:
			this->tiles.Add(wX, wY, T_EMPTY_F + 1); //1-based
			this->tileSet.insert(wX, wY);    //goal-finding optimization
			break;
		default:
			this->tiles.Add(wX,wY,wTile+1); //1-based
			this->tileSet.insert(wX,wY);    //goal-finding optimization
		break;
	}
}

//*****************************************************************************
void CBuilding::remove(const UINT wX, const UINT wY)
//Removes tile from plot set.
{
	this->tiles.Remove(wX,wY);
	this->tileSet.erase(wX,wY);
}

//*****************************************************************************
void CBuilding::setMembers(const CBuilding& src)
//Deep member copy.
{
	this->tileSet = src.tileSet;
	this->tiles = src.tiles;
}
