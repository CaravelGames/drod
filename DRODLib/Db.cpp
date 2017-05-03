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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Db.cpp
//Implementation of CDb.

#define INCLUDED_FROM_DB_CPP
#include "Db.h"
#undef INCLUDED_FROM_DB_CPP
#include "DbXML.h"
#include "CurrentGame.h"
#include "MonsterFactory.h"
#include "NetInterface.h"
#include "SettingsKeys.h"

#ifdef STEAMBUILD
#	include <steam_api.h>
#endif

#include <map>
using std::map;

//Holds the only instance of CDb for the app.
CDb *g_pTheDB = NULL;

//Reset hold and player.
UINT CDb::dwCurrentHoldID = 0L;
UINT CDb::dwCurrentPlayerID = 0L;
bool CDb::bFreezeTimeStamps = false;

//
//CDb public methods.
//

//*****************************************************************************
CDb::CDb()
//Constructor.
{
	this->Players.FilterByLocal();
}

//*****************************************************************************
CDb::~CDb()
//Destructor.
{
	//There shouldn't be any unused rows remaining at this point.
	ASSERT(!EmptyRowsExist());
}

//*****************************************************************************
void CDb::Commit()
//Commits changes in all the databases.
//
//Should only be called from the single global CDb pointer to ensure
//non-static data stored in the view handlers is processed correctly.
{
	RemoveEmptyRows();

	CDbBase::Commit();
}

//*****************************************************************************
void CDb::Rollback()
//Rolls back changes in all the databases.
{
	RemoveEmptyRows(); //ResetEmptyRowCount(); //unstable if metakit rollback doesn't work right

	CDbBase::Rollback();
}

//*****************************************************************************
UINT CDb::GetHoldID()
//Returns: the current hold ID.  If it is zero, try to find the first ID.
{
	if (!CDb::dwCurrentHoldID)
	{
		CDbHold *pHold = this->Holds.GetFirst(true);
		while (pHold)
		{
			const CDbHold::HoldStatus status = pHold->status;
			const UINT holdID = pHold->dwHoldID;
			delete pHold;

			//Skip tutorial holds
			if (status != CDbHold::Tutorial) {
				CDb::dwCurrentHoldID = holdID;
				pHold = NULL;
			} else {
				pHold = this->Holds.GetNext();
			}
		}
	}
	return CDb::dwCurrentHoldID;
}

//*****************************************************************************
UINT CDb::GetPlayerID()
//Returns: the current player ID.  If it is zero, try to find the first local ID.
{
	if (!CDb::dwCurrentPlayerID)
	{
		this->Players.FilterByLocal();
		CDbPlayer *pPlayer = this->Players.GetFirst(true);
		if (pPlayer)
		{
			if (g_pTheNet) g_pTheNet->ClearActiveAction();
			CDb::dwCurrentPlayerID = pPlayer->dwPlayerID;
			if (g_pTheNet) g_pTheNet->DownloadHoldList();
			delete pPlayer;
		}
	}
	return CDb::dwCurrentPlayerID;
}

//*****************************************************************************
CDbPlayer* CDb::GetCurrentPlayer()
{
	return this->Players.GetByID(GetPlayerID());
}

CDbPackedVars CDb::GetCurrentPlayerSettings()
{
	return this->Players.GetSettings(GetPlayerID());
}

//*****************************************************************************
CIDSet CDb::GetChallengeDemoIDs()
{
	CIDSet challengeDemoIDs;

	const UINT playerID = GetPlayerID();
	const UINT holdID = GetHoldID();
	if (playerID && holdID) {
		CDb db;
		db.Demos.FilterByPlayer(playerID);
		db.Demos.FilterByHold(holdID);
		db.Demos.FindHiddens(true);
		const CIDSet demoIDs = db.Demos.GetIDs();
		for (CIDSet::const_iterator it=demoIDs.begin(); it!=demoIDs.end(); ++it)
		{
			const UINT demoID = *it;
			const UINT flags = CDbDemos::GetDemoFlags(demoID);
			if (CDbDemos::IsFlagSet(flags, CDbDemo::CompletedChallenge))
				challengeDemoIDs += demoID;
		}
	}

	return challengeDemoIDs;
}

//*****************************************************************************
CCurrentGame* CDb::GetImportCurrentGame()
//Import process sometimes needs access to an empty CCurrentGame object
//to save to level start (and entrance room) position.
{
	CCurrentGame *pCCG = new CCurrentGame();
	pCCG->dwAutoSaveOptions = ASO_DEFAULT;
	return pCCG;
}

//*****************************************************************************
CCurrentGame *CDb::GetSavedCurrentGame(
//Gets a current game object from a saved game.
//
//Params:
	const UINT dwSavedGameID, //(in)   Indicates saved game to load from.
	CCueEvents &CueEvents,     //(out)  Cue events generated by swordsman's first step 
								//    into the room.
	bool bRestoreAtRoomStart,  //(in)   If true, current game will be loaded to beginning
								//    of room in saved game.  If false, (default)
								//    current game will be loaded to the exact room
								//    state specified in the saved game.
	const bool bNoSaves) //whether DB saves should be prevented [default=false]
//
//Returns:
//Pointer to loaded current game which caller must delete, or NULL saved game did not exist
//or loading failures occurred.
{
	CCurrentGame *pCCG = new CCurrentGame();
	if (pCCG)
	{
		if (!pCCG->LoadFromSavedGame(dwSavedGameID, CueEvents, bRestoreAtRoomStart, bNoSaves))
		{
			delete pCCG;
			pCCG=NULL;
		}
	}
	return pCCG;
}

//*****************************************************************************
CCurrentGame *CDb::GetNewCurrentGame(
//Gets a current game object from a hold.  The current game will be set to the
//starting settings for the hold.
//
//Params:
	const UINT dwHoldID,      //(in) Indicates hold to load from.
	CCueEvents &CueEvents,     //(out)  Cue events generated by swordsman's
								//    first step  into the room.
	const UINT dwAutoSaveOptions) //(in) game save options [default=ASO_DEFAULT]
//
//Returns:
//Pointer to loaded current game which caller must delete, or NULL if hold did
//not exist or loading failures occurred.
{
	CCurrentGame *pCCG = new CCurrentGame();
	if (pCCG)
	{
		pCCG->SetAutoSaveOptions(dwAutoSaveOptions);
		if (!pCCG->LoadFromHold(dwHoldID, CueEvents))
		{
			delete pCCG;
			pCCG=NULL;
		}
	}
	return pCCG;
}

//*****************************************************************************
CCurrentGame *CDb::GetNewTestGame(
//Gets a current game object from a room.
//The current game will be set to the starting settings for the room.
//Used for testing.
//
//Params:
	const UINT dwRoomID,      //(in) Indicates room to load from.
	CCueEvents &CueEvents,     //(out)  Cue events generated by swordsman's
								//    first step into the room.
	const UINT wX, const UINT wY, const UINT wO, //(in) Starting position
	const bool bNoSaves) //whether DB saves should be prevented [default=false]
//
//Returns:
//Pointer to loaded current game which caller must delete, or NULL if room did
//not exist or loading failures occurred.
{
	CCurrentGame *pCCG = new CCurrentGame();
	if (pCCG)
	{
		pCCG->bHoldMastered = pCCG->bHoldCompleted = true; //consider hold completed and mastered during playtesting
		if (!pCCG->LoadFromRoom(dwRoomID, CueEvents, wX, wY, wO, bNoSaves))
		{
			delete pCCG;
			pCCG=NULL;
		}
	}
	return pCCG;
}

//*****************************************************************************
UINT CDb::LookupRowByPrimaryKey(
//Looks up a row in a view by its primary key ID property.
//Assumes primary key property values are monotonically increasing.
//
//Params:
	const UINT dwID,    //(in) Primary key value to match.
	const VIEWTYPE eViewType,  //(in) View/table to scan
	c4_View &View)  //(out) specific view containing this ID
//
//Returns:
//Row index in the outputted view or ROW_NO_MATCH.
{
	//Determine prop for primary key based on table.
	c4_IntProp *pPropID;	//Reference to the primary key field.

	//Determine the rows available for search based on the count maintained
	//in the respective view interface of the global DB object.
	UINT dwRowCount;
	switch (eViewType)
	{
		case V_Data: pPropID = &p_DataID; dwRowCount = g_pTheDB->Data.GetViewSize(dwID); break;
		case V_Demos: pPropID = &p_DemoID; dwRowCount = g_pTheDB->Demos.GetViewSize(dwID); break;
		case V_Holds: pPropID = &p_HoldID; dwRowCount = g_pTheDB->Holds.GetViewSize(dwID); break;
		case V_Levels: pPropID = &p_LevelID; dwRowCount = g_pTheDB->Levels.GetViewSize(dwID); break;
		case V_MessageTexts: pPropID = &p_MessageTextID; dwRowCount = GetView(eViewType, dwID).GetSize(); break;
		case V_Players: pPropID = &p_PlayerID; dwRowCount = g_pTheDB->Players.GetViewSize(dwID); break;
		case V_Rooms: pPropID = &p_RoomID; dwRowCount = g_pTheDB->Rooms.GetViewSize(dwID); break;
		case V_SavedGames: pPropID = &p_SavedGameID; dwRowCount = g_pTheDB->SavedGames.GetViewSize(dwID); break;
		case V_Speech: pPropID = &p_SpeechID; dwRowCount = g_pTheDB->Speech.GetViewSize(dwID); break;
		default:
			ASSERT(!"CDb::LookupRowByPrimaryKey: Unexpected property type.");
			return ROW_NO_MATCH;
	}

	return CDbBase::LookupRowByPrimaryKey(dwID, eViewType, pPropID, dwRowCount, View);
}

//*****************************************************************************
void CDb::ResetMembership()
//Reset all table memberships.
{
	this->Data.ResetMembership();
	this->Demos.ResetMembership();
	this->Holds.ResetMembership();
	this->Levels.ResetMembership();
	this->Players.ResetMembership();
	this->Rooms.ResetMembership();
	this->SavedGames.ResetMembership();
	this->Speech.ResetMembership();
}

//*****************************************************************************
void CDb::SetHoldID(const UINT dwNewHoldID)
//Set active hold ID.
{
	dwCurrentHoldID = dwNewHoldID;
	this->SavedGames.FilterByHold(dwNewHoldID);
}

//*****************************************************************************
void CDb::SetPlayerID(const UINT dwNewPlayerID, const bool bCaravelLogin) //[default=true]
//Set active player and filter saved games for them.
{
	if (CDb::dwCurrentPlayerID == dwNewPlayerID)
		return; //nothing to change

	//Resolve any transactions in progress before changing the player.
	if (g_pTheNet && bCaravelLogin)
		g_pTheNet->ClearActiveAction();

	CDb::dwCurrentPlayerID = dwNewPlayerID;

	//Get hold list according to settings for new player.
	if (g_pTheNet && bCaravelLogin) {
		CDbPlayer* p = CDb::Players.GetByID(dwNewPlayerID);
		if (p) {
			const UINT dwLastNotice = p->Settings.GetVar(Settings::LastNotice, 0);
			delete p;

			g_pTheNet->SetLastNotice(dwLastNotice);
			g_pTheNet->DownloadHoldList();
		}
	}

	this->SavedGames.FilterByPlayer(dwNewPlayerID);
}

//*****************************************************************************
//Acceleration structure -- indexed for fast hierarchical ID set lookup.
struct HoldOwnership {
	CIDSet levelIDs, dataIDs;
};
typedef map<UINT,HoldOwnership> holdMap;

typedef CIDSet LevelOwnership;
typedef map<UINT,LevelOwnership> levelMap;

struct RoomOwnership {
	CIDSet demoIDs, savedGameIDs;
};
typedef map<UINT,RoomOwnership> roomMap;

typedef map<UINT,UINT> idMap;

holdMap holdIndex; //hold -> levels + data
levelMap levelIndex; //level -> rooms
roomMap roomIndex; //room -> saved games + demos
idMap demoIndex; //demo -> saved game
idMap demosHoldIndex; //demo -> hold

//*****************************************************************************
void CDb::addDataToHold(const UINT dataID, const UINT holdID)
//Adds dataID to hold's data set.
{
	CDbBase::DirtyData();

	if (!holdID)
		return; //no hold to attach this data to

	holdMap::iterator holdIter = holdIndex.find(holdID);
	if (holdIter == holdIndex.end())
	{
		ASSERT(!"Data exists in DB with dangling hold ID");
	} else {
		//Add the datum to its parent hold's level ID set.
		holdIter->second.dataIDs += dataID;
	}
}

//*****************************************************************************
void CDb::addDemo(const UINT demoID, const UINT savedGameID)
//Adds demo to index.
{
	CDbBase::DirtySave();
	ASSERT(!demoIndex.count(demoID));
	ASSERT(!demosHoldIndex.count(demoID));
	ASSERT(savedGameID); //each demo must be attached to a saved game
	if (!savedGameID)
		return; //robust to bad data

	//Link demo to its saved game, room and hold.
	demoIndex[demoID] = savedGameID;

	const UINT demosRoomID = CDbSavedGames::GetRoomIDofSavedGame(savedGameID);
	ASSERT(demosRoomID);
	const UINT demosHoldID = CDbRooms::GetHoldIDForRoom(demosRoomID);
	ASSERT(demosHoldID);
	roomMap::iterator room = roomIndex.find(demosRoomID);
	ASSERT(room != roomIndex.end());
	room->second.demoIDs += demoID;

	demosHoldIndex[demoID] = demosHoldID;
}

//*****************************************************************************
void CDb::addHold(const UINT holdID)
//Adds hold to index.
{
	CDbBase::DirtyHold();
	ASSERT(!holdIndex.count(holdID));

	HoldOwnership IDs;
	holdIndex[holdID] = IDs;
}

//*****************************************************************************
void CDb::addLevelToHold(const UINT levelID, const UINT holdID)
//Adds level to index.
{
	CDbBase::DirtyHold();
	ASSERT(!levelIndex.count(levelID));

	LevelOwnership IDs;
	levelIndex[levelID] = IDs;
	//Add level to hold.
	holdMap::iterator hold = holdIndex.find(holdID);
	ASSERT(hold != holdIndex.end());
	hold->second.levelIDs += levelID;
}

//*****************************************************************************
void CDb::addRoomToLevel(const UINT roomID, const UINT levelID)
//Adds room to index.
{
	CDbBase::DirtyHold();
	ASSERT(!roomIndex.count(roomID));

	RoomOwnership roomIDs;
	roomIndex[roomID] = roomIDs;
	//Add room to level.
	levelMap::iterator level = levelIndex.find(levelID);
	ASSERT(level != levelIndex.end());
	level->second += roomID;
}

//*****************************************************************************
void CDb::addSavedGameToRoom(const UINT savedGameID, const UINT roomID)
//Adds saved game to index.
{
	CDbBase::DirtySave();
	if (roomID) //some special saved game records are not associated with a room
	{
		roomMap::iterator room = roomIndex.find(roomID);
		ASSERT(room != roomIndex.end());
		room->second.savedGameIDs += savedGameID;
	}
}

//*****************************************************************************
void CDb::deleteData(const UINT dataID)
//Remove data ID from any hold that owns it.
{
	CDbBase::DirtyData();
	for (holdMap::iterator hold = holdIndex.begin(); hold != holdIndex.end(); ++hold)
		hold->second.dataIDs -= dataID;
}

//*****************************************************************************
void CDb::deleteDemo(const UINT demoID)
//Remove demo from hold and room.
{
	CDbBase::DirtySave();

	//Find demo's room to remove demoID from room index.
	const UINT savedGameID = getSavedGameOfDemo(demoID);
	const UINT roomID = CDbSavedGames::GetRoomIDofSavedGame(savedGameID);
	roomMap::iterator room = roomIndex.find(roomID);
	if (room != roomIndex.end())
		room->second.demoIDs -= demoID;

	//Now remove demo mappings.
	demoIndex.erase(demoID);
	demosHoldIndex.erase(demoID);
}

//*****************************************************************************
void CDb::deleteHold(const UINT holdID)
//Remove hold from index.
{
	CDbBase::DirtyHold();
	holdMap::iterator holdIter = holdIndex.find(holdID);
	if (holdIter != holdIndex.end())
		holdIndex.erase(holdIter);
}

//*****************************************************************************
void CDb::deleteLevel(const UINT levelID)
//Remove level from index.
{
	CDbBase::DirtyHold();
	levelMap::iterator levelIter = levelIndex.find(levelID);
	if (levelIter != levelIndex.end())
	{
		levelIndex.erase(levelIter);
		//Delete level from hold that owns it.
		const UINT holdID = CDbLevels::GetHoldIDForLevel(levelID);
		if (holdID)
		{
			holdMap::iterator hold = holdIndex.find(holdID);
			ASSERT(hold != holdIndex.end());
			hold->second.levelIDs -= levelID;
		}
	}
}

//*****************************************************************************
void CDb::deleteRoom(const UINT roomID)
//Remove room from index.
{
	CDbBase::DirtyHold();
	roomMap::iterator roomIter = roomIndex.find(roomID);
	if (roomIter != roomIndex.end())
	{
		roomIndex.erase(roomIter);
		//Delete room from level that owns it.
		const UINT levelID = CDbRooms::GetLevelIDForRoom(roomID);
		if (levelID)
		{
			levelMap::iterator level = levelIndex.find(levelID);
			ASSERT(level != levelIndex.end());
			level->second -= roomID;
		}
	}
}

//*****************************************************************************
void CDb::deleteSavedGame(const UINT savedGameID)
//Remove saved game from room.
{
	CDbBase::DirtySave();

	//Find saved game's room to remove savedGameID from room index.
	const UINT roomID = CDbSavedGames::GetRoomIDofSavedGame(savedGameID);
	roomMap::iterator room = roomIndex.find(roomID);
	if (room != roomIndex.end())
		room->second.savedGameIDs -= savedGameID;
}

//*****************************************************************************
CIDSet CDb::getDataInHold(const UINT holdID)
//Returns: set of dataIDs belonging to this hold, or empty set if hold doesn't exist
{
	holdMap::iterator holdIter = holdIndex.find(holdID);
	if (holdIter == holdIndex.end())
		return CIDSet(); //no entry
	return holdIter->second.dataIDs;
}

//*****************************************************************************
CIDSet CDb::getDemosInHold(const UINT holdID)
//Returns: set of demos in hold
{
	CIDSet demosInHold, levelsInHold = CDb::getLevelsInHold(holdID);
	for (CIDSet::const_iterator level = levelsInHold.begin();
			level != levelsInHold.end(); ++level)
		demosInHold += CDb::getDemosInLevel(*level);
	return demosInHold;
}

//*****************************************************************************
CIDSet CDb::getDemosInLevel(const UINT levelID)
//Returns: set of demos in level
{
	CIDSet demosInLevel, roomsInLevel = CDb::getRoomsInLevel(levelID);
	for (CIDSet::const_iterator room = roomsInLevel.begin();
			room != roomsInLevel.end(); ++room)
		demosInLevel += CDb::getDemosInRoom(*room);
	return demosInLevel;
}

//*****************************************************************************
CIDSet CDb::getDemosInRoom(const UINT roomID)
//Returns: set of demos in room
{
	roomMap::iterator roomIter = roomIndex.find(roomID);
	if (roomIter == roomIndex.end())
		return CIDSet(); //no entry
	return roomIter->second.demoIDs;
}

//*****************************************************************************
UINT CDb::getHoldOfDemo(const UINT demoID)
//Returns: holdID of the hold that this demo is in
{
	idMap::iterator demoIter = demosHoldIndex.find(demoID);
	return demoIter != demosHoldIndex.end() ? demoIter->second : 0;
}

//*****************************************************************************
CIDSet CDb::getLevelsInHold(const UINT holdID)
//Returns: set of levelIDs belonging to this hold, or empty set if hold doesn't exist
{
	holdMap::iterator holdIter = holdIndex.find(holdID);
	if (holdIter == holdIndex.end())
		return CIDSet(); //no entry
	return holdIter->second.levelIDs;
}

//*****************************************************************************
CIDSet CDb::getRoomsInHold(const UINT holdID)
//Returns: set of roomIDs belonging to this hold, or empty set if level doesn't exist
{
	CIDSet roomsInHold;
	const CIDSet levelsInHold = CDb::getLevelsInHold(holdID);
	for (CIDSet::const_iterator iter = levelsInHold.begin(); iter != levelsInHold.end(); ++iter)
		roomsInHold += CDb::getRoomsInLevel(*iter);
	return roomsInHold;
}

//*****************************************************************************
CIDSet CDb::getRoomsInLevel(const UINT levelID)
//Returns: set of roomIDs belonging to this level, or empty set if level doesn't exist
{
	levelMap::iterator levelIter = levelIndex.find(levelID);
	if (levelIter == levelIndex.end())
		return CIDSet(); //no entry
	return levelIter->second;
}

//*****************************************************************************
UINT CDb::getSavedGameOfDemo(const UINT demoID)
//Returns: savedGameID that this demo is tied to
{
	idMap::iterator demoIter = demoIndex.find(demoID);
	return demoIter != demoIndex.end() ? demoIter->second : 0;
}

//*****************************************************************************
CIDSet CDb::getSavedGamesInHold(const UINT holdID)
{
	CIDSet savedGamesInHold, levelsInHold = CDb::getLevelsInHold(holdID);
	for (CIDSet::const_iterator level = levelsInHold.begin();
			level != levelsInHold.end(); ++level)
		savedGamesInHold += CDb::getSavedGamesInLevel(*level);
	return savedGamesInHold;
}

//*****************************************************************************
CIDSet CDb::getSavedGamesInLevel(const UINT levelID)
{
	CIDSet savedGamesInLevel, roomsInLevel = CDb::getRoomsInLevel(levelID);
	for (CIDSet::const_iterator room = roomsInLevel.begin();
			room != roomsInLevel.end(); ++room)
		savedGamesInLevel += getSavedGamesInRoom(*room);
	return savedGamesInLevel;
}

//*****************************************************************************
CIDSet CDb::getSavedGamesInRoom(const UINT roomID)
//Returns: set of saved games in room
{
	roomMap::iterator roomIter = roomIndex.find(roomID);
	if (roomIter == roomIndex.end())
		return CIDSet(); //no entry
	return roomIter->second.savedGameIDs;
}

//*****************************************************************************
bool CDb::holdExists(const UINT holdID)
{
	return holdIndex.count(holdID) != 0;
}

//*****************************************************************************
bool CDb::levelExists(const UINT levelID)
{
	return levelIndex.count(levelID) != 0;
}

//*****************************************************************************
void CDb::moveData(const UINT dataID, const UINT fromHoldID, const UINT toHoldID)
//Updates data-hold indexing when data object might have changed holds.
{
	CDbBase::DirtyData();
	if (fromHoldID == toHoldID)
		return; //data object is in the same hold as before

	//Remove data index from previous hold.
	holdMap::iterator hold;
	if (fromHoldID)
	{
		hold = holdIndex.find(fromHoldID);
		ASSERT(hold != holdIndex.end());
		hold->second.dataIDs -= dataID;
	}

	//Add data index to current hold.
	if (toHoldID)
	{
		hold = holdIndex.find(toHoldID);
		ASSERT(hold != holdIndex.end());
		hold->second.dataIDs += dataID;
	}
}

//*****************************************************************************
void CDb::moveRoom(const UINT roomID, const UINT fromLevelID, const UINT toLevelID)
//Updates room-level indexing when room might have changed levels.
{
	CDbBase::DirtyHold();
	if (fromLevelID == toLevelID)
		return; //room is in the same level as before

	//Remove room index from previous level.
	levelMap::iterator level = levelIndex.find(fromLevelID);
	ASSERT(level != levelIndex.end());
	level->second -= roomID;

	//Add room index to current level.
	level = levelIndex.find(toLevelID);
	ASSERT(level != levelIndex.end());
	level->second += roomID;
}

//*****************************************************************************
void CDb::moveSavedGame(const UINT savedGameID, const UINT fromRoomID, const UINT toRoomID)
//Updates savedgame-room indexing when room might have changed levels.
{
	CDbBase::DirtySave();
	if (fromRoomID == toRoomID)
		return; //saved game is in the same room as before

	//Remove savedgame index from previous room.
	if (fromRoomID) //some special saved game records are not associated with a room
	{
		roomMap::iterator room = roomIndex.find(fromRoomID);
		ASSERT(room != roomIndex.end());
		room->second.savedGameIDs -= savedGameID;
	}

	//Add savedgame index to current room.
	if (toRoomID)
	{
		roomMap::iterator room = roomIndex.find(toRoomID);
		ASSERT(room != roomIndex.end());
		room->second.savedGameIDs += savedGameID;
	}
}

//*****************************************************************************
void CDb::resetIndex()
//Resets database ID hierarchy.
{
	holdIndex.clear();
	levelIndex.clear();
	roomIndex.clear();
	demoIndex.clear();
	demosHoldIndex.clear();

	CDbBase::resetIndex();
}

//*****************************************************************************
void CDb::buildIndex()
//Constructs map of database ID hierarchy.
//Currently, this includes holds, levels, rooms, and saved games + demos.
{
	ASSERT(holdIndex.empty()); //This method should only be called once.

	//Build hold index.
	//Scan rows of holds DB table directly for speed.
	const UINT holdCount = GetViewSize(V_Holds);
	for (UINT holdI = 0; holdI < holdCount; ++holdI)
	{
		c4_RowRef row = GetRowRef(V_Holds, holdI);
		addHold(UINT(p_HoldID(row)));
	}

	//Build data index.
	const UINT dataCount = GetViewSize(V_Data);
	for (UINT dataI = 0; dataI < dataCount; ++dataI)
	{
		c4_RowRef row = GetRowRef(V_Data, dataI);
		const UINT dataID = UINT(p_DataID(row));

		//Look up this level's hold in the hold map.
		const UINT datumsHoldID = UINT(p_HoldID(row));
		if (datumsHoldID) //ignore data not belonging to any hold
			addDataToHold(dataID, datumsHoldID);
	}

	//Build level index.
	const UINT levelCount = GetViewSize(V_Levels);
	for (UINT levelI = 0; levelI < levelCount; ++levelI)
	{
		c4_RowRef row = GetRowRef(V_Levels, levelI);
		const UINT levelID = UINT(p_LevelID(row));

		//Add level entry.
		LevelOwnership IDs;
		levelIndex[levelID] = IDs;

		//Look up this level's hold in the hold map.
		const UINT levelsHoldID = UINT(p_HoldID(row));
		holdMap::iterator holdIter = holdIndex.find(levelsHoldID);
		if (holdIter == holdIndex.end())
		{
			ASSERT(!"Level exists in DB without a hold");
		} else {
			//Add the level to its parent hold's level ID set.
			HoldOwnership& holdIndex = holdIter->second;
			holdIndex.levelIDs += levelID;
		}
	}

	//Build room index.
	const UINT roomCount = GetViewSize(V_Rooms);
	for (UINT roomI = 0; roomI < roomCount; ++roomI)
	{
		c4_RowRef row = GetRowRef(V_Rooms, roomI);
		const UINT roomID = UINT(p_RoomID(row));

		//Add room entry.
		RoomOwnership IDs;
		roomIndex[roomID] = IDs;

		//Look up this room's level in the level map.
		const UINT roomsLevelID = UINT(p_LevelID(row));
		levelMap::iterator levelIter = levelIndex.find(roomsLevelID);
		if (levelIter == levelIndex.end())
		{
			ASSERT(!"Room exists in DB without a level");
		} else {
			//Add the room to its parent level's room ID set.
			levelIter->second += roomID;
		}
	}

	//Build demo index.
	idMap savedGameDemoIndex; //for building set of demos in a room below
	const UINT demoCount = GetViewSize(V_Demos);
	for (UINT demoI = 0; demoI < demoCount; ++demoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, demoI);
		const UINT demosSavedGameID = UINT(p_SavedGameID(row));
		const UINT savedGamesDemoID = UINT(p_DemoID(row));
		demoIndex[savedGamesDemoID] = demosSavedGameID;
		savedGameDemoIndex[demosSavedGameID] = savedGamesDemoID;
	}

	//Build saved game index.
	idMap::iterator iter;
	const UINT savedGameCount = GetViewSize(V_SavedGames);
	for (UINT sgI = 0; sgI < savedGameCount; ++sgI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, sgI);
		const UINT savedGamesRoomID = UINT(p_RoomID(row));
		roomMap::iterator roomIter = roomIndex.find(savedGamesRoomID);
		if (roomIter != roomIndex.end()) //some special saved game types aren't associated with a room
		{
			//Add the saved game to its parent room's saved game ID set.
			RoomOwnership& roomIndex = roomIter->second;
			const UINT savedGameID = UINT(p_SavedGameID(row));
			roomIndex.savedGameIDs += savedGameID;
			//If a demo owns this saved game, track this.
			iter = savedGameDemoIndex.find(savedGameID);
			if (iter != savedGameDemoIndex.end())
			{
				const UINT demoID = iter->second;
				roomIndex.demoIDs += demoID;
				//Track which hold this demo is in.
				const UINT demosHoldID = CDbRooms::GetHoldIDForRoom(savedGamesRoomID);
				demosHoldIndex[demoID] = demosHoldID;
			}
		}
	}

	CDbBase::buildIndex();
}

//*****************************************************************************
void CDb::SubmitSteamAchievement(const WSTRING& holdName, const string& achievement)
{
#ifdef STEAMBUILD
	if (SteamUserStats())
	{
		string achievementName = "ACH_";
		achievementName += UnicodeToUTF8(filterFirstLettersAndNumbers(holdName));
		achievementName += "_";
		achievementName += achievement;

		SteamUserStats()->SetAchievement(achievementName.c_str());
		SteamUserStats()->StoreStats();
	}
#endif
}

//
// Private methods
//

//*****************************************************************************
bool CDb::EmptyRowsExist() const
{
	if (this->Data.emptyEndRows) return true;
	if (this->Holds.emptyEndRows) return true;
	if (this->Demos.emptyEndRows) return true;
	if (this->Levels.emptyEndRows) return true;
	if (this->Players.emptyEndRows) return true;
	if (this->Rooms.emptyEndRows) return true;
	if (this->SavedGames.emptyEndRows) return true;
	if (this->Speech.emptyEndRows) return true;

	return false;
}

//*****************************************************************************
void CDb::ResetEmptyRowCount()
//When rolling back the DB, empty row counts should be reset.
{
	this->Data.emptyEndRows = 0;
	this->Demos.emptyEndRows = 0;
	this->Holds.emptyEndRows = 0;
	this->Levels.emptyEndRows = 0;
	this->Players.emptyEndRows = 0;
	this->Rooms.emptyEndRows = 0;
	this->SavedGames.emptyEndRows = 0;
	this->Speech.emptyEndRows = 0;
}

//*****************************************************************************
void CDb::RemoveEmptyRows()
//Removes any unused, empty rows added previously to the end of each view.
{
	if (this->Data.emptyEndRows)
	{
		this->Data.RemoveEmptyRows();
		DirtyData();
	}
	if (this->Demos.emptyEndRows || this->SavedGames.emptyEndRows)
	{
		this->Demos.RemoveEmptyRows();
		this->SavedGames.RemoveEmptyRows();
		DirtySave();
	}
	if (this->Holds.emptyEndRows || this->Levels.emptyEndRows ||
			this->Rooms.emptyEndRows || this->Speech.emptyEndRows)
	{
		this->Holds.RemoveEmptyRows();
		this->Levels.RemoveEmptyRows();
		this->Rooms.RemoveEmptyRows();
		this->Speech.RemoveEmptyRows();
		DirtyHold();
	}
	if (this->Players.emptyEndRows)
	{
		this->Players.RemoveEmptyRows();
		DirtyPlayer();
	}
	//no extra rows need removing from text DB
}
