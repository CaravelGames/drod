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
 * Portions created by the Initial Developer are Copyright (C) 2025
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Kieran Millar
 *
 * ***** END LICENSE BLOCK ***** */

 //TotalMapStates.cpp.
 //Implementation of CTotalMapStates.

#include "TotalMapStates.h"
#include "DbSavedGames.h"
#include <BackEndLib/Types.h>

#include <map>

//*****************************************************************************
//
//Public methods.
//

//*****************************************************************************
//Load the map states from the saved game
void CTotalMapStates::Load(const UINT dwPlayerID, const UINT dwHoldID)
{
	//TODO
}

//*****************************************************************************
//Returns the stored room state for a room id
MapState CTotalMapStates::GetStoredMapStateForRoom(const UINT roomID) const
{
	MapState foundMapState = MapState::Invisible;

	std::map<UINT, MapState>::const_iterator it = this->mapStates.find(roomID);
	if (it != this->mapStates.end()) {
		foundMapState = it->second;
	}

	if (foundMapState == MapState::Explored)
		foundMapState = MapState::Preview; //Rooms explored in other play sessions should only appear as a preview at best

	return foundMapState;
}

//*****************************************************************************
//Updates the map state for a room id, adding it if not currently in the list
//The saved game is saved if the map state is larger than the current one
void CTotalMapStates::Update(
	const UINT roomID, // The room ID
	const MapState state, // The MapState to update the room to (if better)
	const bool bNoSaves) // If saving to the db should be prevented
{
	if (UpdateRoom(roomID, state) && !bNoSaves)
		Save();
}

//*****************************************************************************
//Updates the map state for many room ids, adding them if not currently in the list
//This is more efficient for updating rooms in bulk, only triggering a single update
//Used by map collectibles
void CTotalMapStates::Update(
	const CIDSet& roomIDs, // A set of room IDs
	const MapState state, // The MapState to update the rooms to (if better)
	const bool bNoSaves) // If saving to the db should be prevented
{
	if (state == MapState::Invisible)
		return;

	bool bSave = false;
	for (CIDSet::const_iterator it = roomIDs.begin(); it != roomIDs.end(); ++it)
	{
		if (UpdateRoom(*it, state))
			bSave = true;
	}
	if (bSave && !bNoSaves)
		Save();
}

//*****************************************************************************
//
//Private methods.
//

//*****************************************************************************
//Updates the state for a single room
//Returns if the room state was changed
bool CTotalMapStates::UpdateRoom(const UINT roomID, const MapState state)
{
	if (state == MapState::Invisible)
		return false; // No need to store the state if the lowest and default value, this saves time when setting many minimap icons

	MapState currentMapState = MapState::Invisible;

	std::map<UINT, MapState>::const_iterator it = this->mapStates.find(roomID);
	if (it != this->mapStates.end()) {
		currentMapState = it->second;
	}
	else {
		this->mapStates[roomID] = state;
		return true;
	}

	if (currentMapState < state) {
		this->mapStates[roomID] = state;
		return true;
	}

	return false;
}

//*****************************************************************************
//Saves the map states into the ST_TotalMapStates saved game
void CTotalMapStates::Save()
{
	// TODO
}
