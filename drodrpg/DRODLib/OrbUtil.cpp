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

#include "OrbUtil.h"

void GetOrbConnectionsForMerging(
// Finds all of the orbs that connect to a door that require merging (have more than 1 connection between the door and the orb)
	const CDbRoom& room,           // (in) Room which has the orbs data
	const CCoordSet& doorCoords,   // (in) Tiles of the door for which orb connections are checked
	vector<COrbData*>& outOrbData) // (out) Vector of the orbs that require merging
{
	for (UINT orb = 0; orb < room.orbs.size(); ++orb)
	{
		COrbData* pData = room.orbs[orb];
		UINT agentsCount = 0;
		for (UINT agent = 0; agent < pData->agents.size(); ++agent)
		{
			COrbAgentData* pAgentData = pData->agents[agent];
			if (doorCoords.has(pAgentData->wX, pAgentData->wY))
				++agentsCount;
			
			if (agentsCount > 1) {
				outOrbData.push_back(pData);
				break; // Break here so we don't add the same orb twice
			}
		}
	}
}

void MergeOrbConnections(
// Will merge connections between the given door and orb/plate so that only one connection remains
// Identical connection types are merged to the same type (Open + Open = Open) (Close + Close = Close) (Toggle + Toggle = Toggle)
// Different connection types are replaced with Toggle (Open + Close = Toggle) (Any + Toggle = Toggle)
	const CCoordSet& doorCoords, // (in) coordinates of the door which has connections merges
	COrbData* pOrbData)          // (in/out) orb data that has a connection to the door, will be modified in place
	                             // to have at most one connection to the given door
{
	bool bHasOpen = false;
	bool bHasClose = false;
	bool bHasToggle = false;

	OrbAgentType targetAction = OA_NULL;

	//1. Gather information - find if there is a connection of specific type
	for (UINT agent = 0; agent < pOrbData->agents.size(); ++agent)
	{
		COrbAgentData* pAgentData = pOrbData->agents[agent];
		if (!doorCoords.has(pAgentData->wX, pAgentData->wY))
			continue;

		if (targetAction == OA_NULL)
			targetAction = pAgentData->action;
		else if (targetAction != pAgentData->action)
			targetAction = OA_TOGGLE;
	}
	
	if (targetAction == OA_NULL) // Nothing to merge
		return;

	//2. do the actual merge
	bool bFirstAgentFound = false;
	for (UINT agent = 0; agent < pOrbData->agents.size(); ++agent)
	{
		COrbAgentData* pAgentData = pOrbData->agents[agent];
		if (!doorCoords.has(pAgentData->wX, pAgentData->wY))
			continue;

		if (bFirstAgentFound) {
			pOrbData->DeleteAgent(pAgentData);
			--agent;
		} else {
			pAgentData->action = targetAction;
			bFirstAgentFound = true;
		}
	}
}

//*****************************************************************************
void OrbUtil::MergeDoorConnectionsInArea(
// Merges orb connections at the specified area to ensure there are no conflicts
// This will check every door in the area and ensure there are no duplicate agents in the same orb/plate
// And if there are, they'll be merged. If agent action conflicts appear, they'll merge to a Toggle
	CDbRoom& room,             //(in) Room affected
	UINT wX, UINT wY,          //(in) Area start
	const UINT wWidth, const UINT wHeight) //(in) Built rectangle dimensions (use 0, 0 to signify a single tile being changed)
{
	CCoordSet mergedDoorCoords;

	UINT endX = wX + wWidth;
	UINT endY = wY + wHeight;

	if (!room.CropRegion(wX, wY, endX, endY))
		return;

	vector<COrbData*> orbData;

	for (UINT x = wX; x <= endX; ++x) {
		for (UINT y = wY; y <= endY; ++y) {
			if (mergedDoorCoords.has(x, y))
				continue;

			const UINT oTile = room.GetOSquare(x, y);
			if (!bIsDoor(oTile))
				continue;

			CCoordSet doorCoords;
			room.GetAllDoorSquares(x, y, doorCoords, oTile);
			mergedDoorCoords += doorCoords;

			orbData.clear();
			GetOrbConnectionsForMerging(room, doorCoords, orbData);

			for (UINT i = 0; i < orbData.size(); i++)
				MergeOrbConnections(doorCoords, orbData[i]);
		}
	}
}