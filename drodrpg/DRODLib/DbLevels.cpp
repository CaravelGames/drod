// $Id: DbLevels.cpp 10113 2012-04-22 05:40:36Z mrimer $

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

//DbLevels.cpp
//Implementation of CDbLevels and CDbLevel.

#include "DbLevels.h"

#include "Db.h"
#include "DbProps.h"
#include "DbXML.h"
#include "MonsterFactory.h"
#include "Character.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>

//*****************************************************************************
bool sortLevels::operator() (const CDbLevel* pLevel1, const CDbLevel* pLevel2) const
//Compare positions of two levels in their hold.
{
	const int i1 = (int)(pLevel1->dwOrderIndex);
	const int i2 = (int)(pLevel2->dwOrderIndex);
	return i1 < i2;
}

//
//CDbLevels public methods.
//

//*****************************************************************************
void CDbLevels::Delete(
//Deletes records for a level with the given ID.
//Also deletes all rooms in the level (and their scrolls, saved games and demos).
//NOTE: Does not remove the level (i.e. references to its ID) from its hold,
// since this might need to be handled in a specific way.
// RemoveFromHold() should be called following a call to Delete().
//
//Params:
	const UINT dwLevelID)  //(in)   ID of level to delete.
{
	ASSERT(dwLevelID);

	//Get index in DB.
	c4_View LevelsView;
	const UINT dwLevelRowI = LookupRowByPrimaryKey(dwLevelID, V_Levels, LevelsView);
	if (dwLevelRowI == ROW_NO_MATCH) {ASSERT(!"Bad level ID."); return;}

	//Delete all rooms in level (and their associated data).
	CDb db;
	CIDSet RoomIDs = CDb::getRoomsInLevel(dwLevelID);
	for (CIDSet::const_iterator iter = RoomIDs.begin(); iter != RoomIDs.end(); ++iter)
		db.Rooms.Delete(*iter);

	//Delete name and description message texts.
	const UINT dwNameMID = p_NameMessageID( LevelsView[dwLevelRowI] );
	if (!dwNameMID) {ASSERT(!"Bad MID for name"); return;}
	DeleteMessage(static_cast<MESSAGE_ID>(dwNameMID));

	//Delete the level.
	CDb::deleteLevel(dwLevelID); //call first
	LevelsView.RemoveAt(dwLevelRowI);

	//After level object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*******************************************************************************
bool CDbLevels::Exists(const UINT dwID) const
{
	return CDb::levelExists(dwID);
}

//*****************************************************************************
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">" NEWLINE
#define CLOSETAG "'/>" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))
#define TIMETTOSTR(val) writeTimeT(dummy, sizeof(dummy), (val))
#define EXPORTTEXT(pType, messageText) messageText.ExportText(str, PropTypeStr(pType))

//*****************************************************************************
bool CDbLevels::ExportText(const UINT dwLevelID, CDbRefs &dbRefs, CStretchyBuffer &str)
//Simply serialize a level reference.
{
	if (dbRefs.IsSet(V_Levels,dwLevelID))
		return false;

	dbRefs.Set(V_Levels,dwLevelID);

	CDbLevel *pLevel = GetByID(dwLevelID);
	ASSERT(pLevel);
	if (!pLevel)
		return false; //shouldn't happen -- but this is more robust

	char dummy[32];

	//GID.
	str += STARTTAG(V_Levels, P_HoldID);
	str += INT32TOSTR(pLevel->dwHoldID);
	str += PROPTAG(P_GID_LevelIndex);
	str += INT32TOSTR(pLevel->dwLevelIndex);
	str += PROPTAG(P_LevelID);
	str += INT32TOSTR(pLevel->dwLevelID);
	str += CLOSESTARTTAG;

	EXPORTTEXT(P_NameMessage, pLevel->NameText);

	delete pLevel;

	str += ENDTAG(V_Levels);

	return true;
}

//*****************************************************************************
void CDbLevels::ExportXML(
//Returns: string containing XML text describing level with this ID
//          AND all rooms having this LevelID
//
//Pre-condition: dwLevelID is valid
//
//Params:
	const UINT dwLevelID,  //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)        //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_Levels,dwLevelID))
		return;

	dbRefs.Set(V_Levels,dwLevelID);

	//Prepare data.
	CDbLevel *pLevel = GetByID(dwLevelID);
	ASSERT(pLevel);
	if (!pLevel)
		return; //shouldn't happen -- but this is more robust

	char dummy[32];

	//Include corresponding hold ref.
	g_pTheDB->Holds.ExportXML(pLevel->dwHoldID, dbRefs, str, true);
	if (!bRef)
	{
		//Include corresponding player ref.
		g_pTheDB->Players.ExportXML(pLevel->dwPlayerID, dbRefs, str, true);
	}

	str += STARTTAG(V_Levels, P_HoldID);
	str += INT32TOSTR(pLevel->dwHoldID);
	str += PROPTAG(P_GID_LevelIndex);
	str += INT32TOSTR(pLevel->dwLevelIndex);
	if (!bRef)
	{
		//Prepare data.
		WSTRING const wNameStr = (WSTRING)pLevel->NameText;

		str += PROPTAG(P_OrderIndex);
		str += INT32TOSTR(pLevel->dwOrderIndex);
		str += PROPTAG(P_PlayerID);
		str += INT32TOSTR(pLevel->dwPlayerID);
		str += PROPTAG(P_NameMessage);
		str += Base64::encode(wNameStr);
		str += PROPTAG(P_Created);
		str += TIMETTOSTR((time_t)pLevel->Created);
		str += PROPTAG(P_LastUpdated);
		str += TIMETTOSTR((time_t)pLevel->LastUpdated);
		//str += PROPTAG(P_IsRequired);
		//str += INT32TOSTR(pLevel->bIsRequired);
		str += PROPTAG(P_Multiplier);
		str += INT32TOSTR(pLevel->dwMultiplier);
	}
	//Put primary key last, so all message fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_LevelID);
	str += INT32TOSTR(pLevel->dwLevelID);
	str += CLOSETAG;
	if (!bRef)
	{
		//Export all rooms in level.
		CDb db;
		CIDSet RoomIDs = CDb::getRoomsInLevel(dwLevelID);
		for (CIDSet::const_iterator iter = RoomIDs.begin(); iter != RoomIDs.end(); ++iter)
			db.Rooms.ExportXML(*iter, dbRefs, str);
	}

	delete pLevel;
}
#undef STARTTAG
#undef PROPTAG
#undef ENDTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR
#undef TIMETTOSTR
#undef EXPORTTEXT

//*****************************************************************************
CDbLevel* CDbLevels::GetNew()
//Get a new level object that will be added to database when it is updated.
//
//Returns:
//Pointer to new level.
{
	//After level object is updated, membership changes, so reset the flag.
	this->bIsMembershipLoaded = false;

	//Create new level object.
	CDbLevel *pLevel = new CDbLevel;

	//Put level in specified hold.
	pLevel->dwHoldID = this->dwFilterByHoldID;

	return pLevel;
}

//*****************************************************************************
void CDbLevels::FilterBy(
//Changes filter so that GetFirst() and GetNext() will return levels for a specified hold.
//
//Params:
	const UINT dwSetFilterByHoldID) //(in)   Hold ID to filter by.  Set to 0 for all levels.
{
	if (dwSetFilterByHoldID != this->dwFilterByHoldID && this->bIsMembershipLoaded)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = dwSetFilterByHoldID;
}

//*****************************************************************************
UINT CDbLevels::GetHoldIDForLevel(const UINT dwLevelID)
{
	if (!dwLevelID) return 0;
	c4_View LevelsView;
	const UINT dwLevelRowI = LookupRowByPrimaryKey(dwLevelID, V_Levels, LevelsView);
	if (dwLevelRowI == ROW_NO_MATCH) {ASSERT(!"Bad level ID."); return 0;}
	return (UINT)(p_HoldID(LevelsView[dwLevelRowI]));
}

//*****************************************************************************
WSTRING CDbLevels::GetLevelName(const UINT levelID)
{
	WSTRING name;

	c4_View LevelsView;
	const UINT dwLevelI = LookupRowByPrimaryKey(levelID, V_Levels, LevelsView);
	if (dwLevelI != ROW_NO_MATCH) {
		c4_RowRef row = LevelsView[dwLevelI];

		CDbMessageText NameText((UINT)p_NameMessageID(row));
		name = (const WCHAR*)NameText;
	}

	return name;
}

//*****************************************************************************
void CDbLevels::GetRoomsExplored(
//OUT: Compiled lists of room IDs, one being the all the rooms in the level,
//and the second the subset of these rooms having been explored
//or conquered (as indicated) by the specified player.
//
//Params:
	const UINT dwLevelID, const UINT dwPlayerID,  //(in)
	CIDSet& roomsInLevel,                          //(out)
	CIDSet& playerRoomsExploredInLevel)            //(out)
//	const bool bConquered)                         //only conquered rooms [default=false]
{
	ASSERT(dwLevelID);
	ASSERT(dwPlayerID);
	CDb db;

	//Get all rooms in level.
	roomsInLevel = CDb::getRoomsInLevel(dwLevelID);

	//Get all player's saved games in level.
	db.SavedGames.FilterByLevel(dwLevelID);
	db.SavedGames.FilterByPlayer(dwPlayerID);
	db.SavedGames.FindHiddens(true);
	CIDSet playerSavedGamesInLevel = db.SavedGames.GetIDs();

	//Total rooms explored.
	const UINT progressSaveID = db.SavedGames.FindByType(ST_PlayerTotal, dwPlayerID, false);
	if (progressSaveID)
		playerSavedGamesInLevel += progressSaveID;

	//Tally rooms explored/conquered by player.
	playerRoomsExploredInLevel.clear();
	CDbSavedGame *pSavedGame;
	for (CIDSet::const_iterator iter = playerSavedGamesInLevel.begin();
			iter != playerSavedGamesInLevel.end(); ++iter)
	{
		pSavedGame = db.SavedGames.GetByID(*iter);
		ASSERT(pSavedGame);
/*
		if (bConquered)
		{
			pSavedGame->ConqueredRooms.intersect(roomsInLevel);
			playerRoomsExploredInLevel += pSavedGame->ConqueredRooms;
		} else {
*/
		CIDSet explRooms = pSavedGame->GetExploredRooms();
		explRooms.intersect(roomsInLevel);
		playerRoomsExploredInLevel += explRooms;

		//This room might not be marked as explored, but the player is there now.
		if (pSavedGame->dwRoomID) //ignore the dummy roomID in the playerTotal saved game
			playerRoomsExploredInLevel += pSavedGame->dwRoomID;
//		}
		delete pSavedGame;
	}
	ASSERT(playerRoomsExploredInLevel.size() <= roomsInLevel.size());
}

//*****************************************************************************
void CDbLevels::GetSecretRoomsInSet(
//OUT: the number of secret rooms in the given level,
//and the number in the subset of these rooms in the explored list
//
//Params:
	CIDSet& roomsInLevel,       //(in)
	CIDSet& roomIDs,            //(in)
	UINT& dwSecretRooms, UINT& dwPlayerSecretRooms)   //(out)
{
	dwSecretRooms = dwPlayerSecretRooms = 0;

	for (CIDSet::const_iterator iter = roomsInLevel.begin(); iter != roomsInLevel.end(); ++iter)
	{
		if (CDbRoom::IsSecret(*iter))
		{
			++dwSecretRooms;
			if (roomIDs.has(*iter))
				++dwPlayerSecretRooms;
		}
	}
}

//*****************************************************************************
UINT CDbLevels::GetStartingRoomCoords(
//Gets coordinates of starting room for level.
//Edited levels might not have a starting room, so return (0,0) in this case.
//
//Returns: ID of starting room
//
//Params:
	const UINT dwLevelID,  //(in)
	UINT &dwRoomX, UINT &dwRoomY)  //(out)  The starting room coords.
{
	ASSERT(IsOpen());
	CDbHold *pHold = g_pTheDB->Holds.GetByID(GetHoldIDForLevel(dwLevelID));
	ASSERT(pHold);
	if (!pHold) return 0;
	const UINT dwRoomID = pHold->GetMainEntranceRoomIDForLevel(dwLevelID);
	delete pHold;
	c4_View RoomsView;
	const UINT dwRoomI = CDbBase::LookupRowByPrimaryKey(dwRoomID, V_Rooms,
			RoomsView);
	if (dwRoomI == ROW_NO_MATCH) {dwRoomX = 0L; dwRoomY = 0L; return 0;}
	dwRoomX = p_RoomX( RoomsView[dwRoomI] );
	dwRoomY = p_RoomY( RoomsView[dwRoomI] );
	return dwRoomID;
}

//*****************************************************************************
void CDbLevels::UpdateExitIDs(
//When a level is copied from an old hold to a new hold, then the stairs'
//destination entrance IDs must be updated.  In this case, the
//only reliable thing to do is reset all the IDs.
//
//Params:
	const UINT dwLevelID) //(in)
{
	CDb db;
	db.Rooms.FilterBy(dwLevelID);
	CDbRoom *pRoom = db.Rooms.GetFirst();
	while (pRoom)
	{
		pRoom->ResetExitIDs();
		delete pRoom;
		pRoom = db.Rooms.GetNext();
	}
}

//
//CDbLevels private methods.
//

//*****************************************************************************
void CDbLevels::LoadMembership()
//Load the membership list with all level IDs.
{
	ASSERT(IsOpen());
	const UINT dwLevelCount = GetViewSize();

	//Each iteration gets a level ID and puts in membership list.
	this->MembershipIDs.clear();
	for (UINT dwLevelI = 0; dwLevelI < dwLevelCount; ++dwLevelI)
	{
		c4_RowRef row = GetRowRef(V_Levels, dwLevelI);
		const UINT dwHoldID = p_HoldID(row);
		if (!this->dwFilterByHoldID || dwHoldID == this->dwFilterByHoldID)
			this->MembershipIDs += p_LevelID(row);
	}
	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//
//CDbLevel protected methods.
//

//*****************************************************************************
CDbLevel::CDbLevel()
//Constructor.
{  
	Clear();
}

//
//CDbLevel public methods.
//

//*****************************************************************************
CDbLevel::~CDbLevel()
//Destructor.
{
	Clear();
}

//*****************************************************************************
const WCHAR * CDbLevel::GetAuthorText() const
//Returns author of the level or NULL if not found.
{
	//Look up NameMessageID from associated Players record.
	c4_View PlayersView;
	const UINT dwPlayersRowI = LookupRowByPrimaryKey(this->dwPlayerID,
			V_Players, PlayersView);
	if (dwPlayersRowI == ROW_NO_MATCH) {ASSERT(!"Bad player row."); return NULL;}
	const UINT dwNameMessageID = p_NameMessageID( PlayersView[dwPlayersRowI] );

	//Look up message text.
	return g_pTheDB->GetMessageText(static_cast<MESSAGE_ID>(dwNameMessageID));
}

//*****************************************************************************
bool CDbLevel::Load(
//Loads a level from database into this object.
//
//Params:
	const UINT dwLoadLevelID, //(in) LevelID of level to load.
	const bool /*bQuick*/) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {

	Clear();

	//Find record with matching level ID.
	ASSERT(IsOpen());
	c4_View LevelsView;
	const UINT dwLevelI = LookupRowByPrimaryKey(dwLoadLevelID, V_Levels, LevelsView);
	if (dwLevelI == ROW_NO_MATCH) throw CException("CDbLevel::Load");
	c4_RowRef row = LevelsView[dwLevelI];

	//Load in props from Levels record.
	this->dwLevelID = (UINT) (p_LevelID(row));
	this->dwHoldID = (UINT) (p_HoldID(row));
	this->dwPlayerID = (UINT) (p_PlayerID(row));
	this->Created = (time_t) (p_Created(row));
	this->LastUpdated = (time_t) (p_LastUpdated(row));
	this->NameText.Bind((UINT) p_NameMessageID(row));
	this->dwLevelIndex = (UINT) (p_GID_LevelIndex(row));
	this->dwOrderIndex = (UINT) (p_OrderIndex(row));
	//this->bIsRequired = (p_IsRequired(row) != 0);
	this->dwMultiplier = (UINT) (p_Multiplier(row));
	//1.0.0.4: backwards compatibility
	if (!this->dwMultiplier)
		this->dwMultiplier = 1;

	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
CDbHold * CDbLevel::GetHold()
//Get hold associated with this level.
//
//Returns:
//Pointer to loaded hold which caller must delete, or NULL if no matching hold 
//was found.
const
{
	CDbHold *pHold = new CDbHold();
	if (pHold)
	{
		if (!pHold->Load(this->dwHoldID))
		{
			delete pHold;
			pHold=NULL;
		}
	}
	return pHold;
}

//*****************************************************************************
/*
void CDbLevel::GetRequiredRooms(
//Gets list of rooms in level required to conquer in order to complete level.
//
//Params:
	CIDSet& requiredRooms)  //(out)
{
	requiredRooms.clear();

	CDb db;
	CIDSet roomsInLevel = CDb::getRoomsInLevel(this->dwLevelID);
	for (CIDSet::const_iterator iter = roomsInLevel.begin(); iter != roomsInLevel.end(); ++iter)
		if (CDbRoom::IsRequired(*iter))
			requiredRooms += *iter;
}
*/

//*****************************************************************************
CDbRoom * CDbLevel::GetStartingRoom()
//Gets starting room associated with this level.
//
//Returns:
//Pointer to a new loaded room object which caller must delete or NULL if could 
//not load room.
{
	if (!GetStartingRoomID()) return NULL;

	CDbRoom *pRoom = new CDbRoom();
	if (pRoom)
	{
		if (pRoom->Load(this->dwStartingRoomID))
		{
			//Remember starting room coords.
			if (!this->bGotStartingRoomCoords)
			{
				this->dwStartingRoomX = pRoom->dwRoomX;
				this->dwStartingRoomY = pRoom->dwRoomY;
				this->bGotStartingRoomCoords = true;
			}
		}
		else
		{
			delete pRoom;
			pRoom = NULL;
		}
	}
	return pRoom;
}

//*****************************************************************************
void CDbLevel::GetStartingRoomCoords(
//Gets coordinates of starting room for level.
//Edited levels might not have a starting room, so return (0,0) in this case.
//
//Params:
	UINT &dwRoomX, UINT &dwRoomY)  //(out) The starting room coords.
{
	//Do I already have starting room coordinates.
	if (!this->bGotStartingRoomCoords)
	{
		//No--Look up the coords.
		this->dwStartingRoomID = CDbLevels::GetStartingRoomCoords(this->dwLevelID,
				this->dwStartingRoomX, this->dwStartingRoomY);
		this->bGotStartingRoomCoords = (this->dwStartingRoomID != 0);
	}

	//Return coords.
	dwRoomX = this->dwStartingRoomX;
	dwRoomY = this->dwStartingRoomY;
}

//*****************************************************************************
UINT CDbLevel::GetStartingRoomID()
//Find main entrance for this level and get room ID
{
	if (this->dwStartingRoomID)
		return this->dwStartingRoomID;

	CDbHold *pHold = g_pTheDB->Holds.GetByID(this->dwHoldID);
	ASSERT(pHold);
	if (!pHold) return 0;

	this->dwStartingRoomID = pHold->GetMainEntranceRoomIDForLevel(this->dwLevelID);
	delete pHold;
	return this->dwStartingRoomID;
}

//*****************************************************************************
UINT CDbLevel::getItemAmount(const UINT item) const
//Returns: the usage amount of this item on this level
{
	switch (item)
	{
		case T_HEALTH_HUGE:
		{
			static const UINT healFactor = 1000;
			return healFactor * this->dwMultiplier;
		}
		case T_HEALTH_BIG:
		{
			static const UINT healFactor = 200;
			return healFactor * this->dwMultiplier;
		}
		case T_HEALTH_MED:
		{
			static const UINT healFactor = 50;
			return healFactor * this->dwMultiplier;
		}
		case T_HEALTH_SM:
		{
			static const UINT healFactor = 15;
			return healFactor * this->dwMultiplier;
		}

		case T_ATK_UP:
		{
			static const UINT atkFactor = 1;
			return atkFactor * this->dwMultiplier;
		}
		case T_ATK_UP3:
		{
			static const UINT atkFactor = 3;
			return atkFactor * this->dwMultiplier;
		}
		case T_ATK_UP10:
		{
			static const UINT atkFactor = 10;
			return atkFactor * this->dwMultiplier;
		}

		case T_DEF_UP:
		{
			static const UINT defFactor = 1;
			return defFactor * this->dwMultiplier;
		}
		case T_DEF_UP3:
		{
			static const UINT defFactor = 3;
			return defFactor * this->dwMultiplier;
		}
		case T_DEF_UP10:
		{
			static const UINT defFactor = 10;
			return defFactor * this->dwMultiplier;
		}

		case T_DOOR_MONEY: case T_DOOR_MONEYO:
		{
			static const UINT costFactor = 10;
			return costFactor * this->dwMultiplier;
		}
		default: return 0;
	}
}

//*****************************************************************************
CDbRoom * CDbLevel::GetRoomAtCoords(
//Gets a room at specified coords in this level.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in)   Coords of room to get.
//
//Returns:
//Pointer to a new loaded room object which caller must delete or NULL if could 
//not load room.
{
	CDbRoom *pRoom = NULL;

	//Find matching room ID.
	const UINT dwLoadRoomID = GetRoomIDAtCoords(dwRoomX, dwRoomY);
	if (!dwLoadRoomID) return NULL;

	//Load room.
	pRoom = new CDbRoom();
	if (pRoom)
	{
		if (!pRoom->Load(dwLoadRoomID))
		{
			delete pRoom;
			pRoom = NULL;
		}
		
		//Remember starting room coords if this is the starting room.
		GetStartingRoomID();
		if (dwLoadRoomID == this->dwStartingRoomID && !this->bGotStartingRoomCoords)
		{
			this->dwStartingRoomX = pRoom->dwRoomX;
			this->dwStartingRoomY = pRoom->dwRoomY;
			this->bGotStartingRoomCoords = true;
		}
	}
	return pRoom;
}

//*****************************************************************************
void CDbLevel::getStats(RoomStats& stats) const
//Tallies the value of stat-affecting items in the level.
{
	++stats.levels;

	CCurrentGame *pTempGame = g_pTheDB->GetDummyCurrentGame();

	const CIDSet roomIDs = CDb::getRoomsInLevel(this->dwLevelID);
	for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room);
		ASSERT(pRoom);
		pRoom->SetCurrentGameForMonsters(pTempGame);

		pRoom->getStats(stats, this);

		delete pRoom;
	}

	delete pTempGame;
}

//*****************************************************************************
CDbLevel* CDbLevel::CopyIntoHold(
//Creates a copy of the entire level and adds it to the specified hold,
//saving it to the DB.
//
//Post-Cond: destination hold's Update method has been called
//
//Returns: pointer to new level object
//
//Params:
	CDbHold *pNewHold,  //(in) hold new level belongs to
	CImportInfo& info) //(in/out)	
{
	ASSERT(pNewHold);

	CDbLevel *pNewLevel = g_pTheDB->Levels.GetNew();
	if (!pNewLevel) return NULL;

	const bool bDifferentHold = pNewHold->dwHoldID != this->dwHoldID;

	pNewLevel->SetMembers(*this, false);        //must make new message texts
	pNewLevel->Created = 0;    //update timestamps
	pNewLevel->dwHoldID = pNewHold->dwHoldID;
	pNewLevel->Update();

	//Make a copy of all the rooms in level.
	const UINT dwEntranceRoomID = SaveCopyOfRooms(pNewLevel->dwLevelID, info, pNewHold->dwHoldID);

	//Set new level's entrance room ID.
	ASSERT(dwEntranceRoomID);
	pNewLevel->dwStartingRoomID = dwEntranceRoomID;
	pNewLevel->Update();

	CDbHold *pOldHold = GetHold();
	ASSERT(pOldHold);

	//Copy the level entrances.
	//Add level entrances in destination hold's entrance list.
	for (UINT wIndex=0; wIndex<pOldHold->Entrances.size(); ++wIndex)
	{
		//Only add entrances located in copied level.
		const CDbRoom *pOldEntranceRoom = g_pTheDB->Rooms.GetByID(
				pOldHold->Entrances[wIndex]->dwRoomID, true);
		ASSERT(pOldEntranceRoom);
		if (pOldEntranceRoom->dwLevelID == this->dwLevelID)
		{
			//Add this entrance to duplicate level's hold.
			ASSERT(pOldHold->Entrances[wIndex]);
			const CEntranceData *pSrcEntrance = pOldHold->Entrances[wIndex];
			CEntranceData *pEntrance = new CEntranceData(*pSrcEntrance);
			ASSERT(pEntrance);

			//Remove binding to old message text ID.
			pEntrance->DescriptionText.Clear();
			pEntrance->DescriptionText = pOldHold->Entrances[wIndex]->DescriptionText;

			//Update entrance's room ID to point to duplicated level's room.
			const UINT dwNewEntranceRoomID = pNewLevel->GetRoomIDAtCoords(
					pOldEntranceRoom->dwRoomX,
					(pNewLevel->dwLevelID * 100) + (pOldEntranceRoom->dwRoomY % 100));
			pEntrance->dwRoomID = dwNewEntranceRoomID;

			pNewHold->AddEntrance(pEntrance);

			//Destination entrance IDs must be rekeyed by the caller using this mapping info (typically by calling RekeyExitIDs).
			ASSERT(!info.EntranceIDMap.count(pSrcEntrance->dwEntranceID));
			info.EntranceIDMap[pSrcEntrance->dwEntranceID] = pEntrance->dwEntranceID;
		}
		delete pOldEntranceRoom;
	}

	delete pOldHold;
	pNewHold->Update();

	return pNewLevel;
}

//*****************************************************************************
void CDbLevel::RekeyExitIDs(
//Pre-condition: this is a copy of a level from a different hold, created by CopyIntoHold().
//
//Update level entrance and character script IDs in all copied rooms.
	const CDbHold* pOldHold, CDbHold *pNewHold,
	CImportInfo& info) //already populated with entranceID mappings
{
	const bool bDifferentHold = pNewHold->dwHoldID != pOldHold->dwHoldID;

	//Special official hold considerations.
	const bool bOfficialHoldTransfer = pOldHold->status == CDbHold::Main && (pOldHold->status == pNewHold->status);

	CDb db;
	const CIDSet roomIDs = CDb::getRoomsInLevel(this->dwLevelID);
	for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
	{
		CDbRoom *pRoom = db.Rooms.GetByID(*room);
		ASSERT(pRoom);
		bool bIDsUpdated = false;

		if (bDifferentHold) {
			//Update stair destination entrance IDs.
			for (UINT wIndex = 0; wIndex < pRoom->Exits.size(); ++wIndex)
			{
				//Only need to change non-zero IDs.
				UINT& entranceID = pRoom->Exits[wIndex]->dwEntranceID;
				if (entranceID && entranceID != (UINT)EXIT_PRIOR_LOCATION) {
					//For entrances with an entranceID mapping provided in 'info', apply the remapping.
					//For the remaining entrances, when copying the level to a different hold,
					//the only reliable thing to do is reset all the destination IDs.
					PrimaryKeyMap::const_iterator newID = info.EntranceIDMap.find(entranceID);
					if (newID != info.EntranceIDMap.end()) {
						entranceID = newID->second;
					} else {
						entranceID = 0; //end hold
					}

					bIDsUpdated = true;
				}
			}
		}

		for (CMonster *pMonster = pRoom->pFirstMonster; pMonster!=NULL; pMonster = pMonster->pNext)
		{
			if (pMonster->wType == M_CHARACTER)
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				pCharacter->ChangeHold(pOldHold, pNewHold, info, !bOfficialHoldTransfer);
				if (bOfficialHoldTransfer)
				{
					//When copying levels to an alternate version of an official hold,
					//i.e. from the full hold to the demo-version hold,
					//scriptIDs must be preserved so saved games transferred
					//from the demo to full game hold retain correct behavior.
					if (pCharacter->dwScriptID > pNewHold->GetScriptID())
						pNewHold->dwScriptID = pCharacter->dwScriptID;
				} else {
					bIDsUpdated = true;
				}
			}			
		}

		if (bIDsUpdated)
			pRoom->Update();

		delete pRoom;
	}

	pNewHold->Update();
}

//*****************************************************************************
MESSAGE_ID CDbLevel::SetProperty(
//Used during XML import of language modules.
	const PROPTYPE pType,
	const char** atts,
	CImportInfo &/*info*/)
{
	switch (pType)
	{
		case P_NameMessage:
			this->NameText.ImportText(atts);
		break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbLevel::SetProperty(
//Used during XML data import.                      
//According to pType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
	const PROPTYPE pType,   //(in) property (data member) to set
	char* const str,        //(in) string representation of value
	CImportInfo &info,      //(in/out) Import info
	bool &bSaveRecord)      //(out) whether record should be saved
{
//	static CEntranceData *pEntrance = NULL;

	static PrimaryKeyMap::iterator localID;
	switch (pType)
	{
		case P_LevelID:
		{
			this->dwLevelID = convertToUINT(str);
			if (!this->dwLevelID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.LevelIDMap.find(this->dwLevelID);
			if (localID != info.LevelIDMap.end())
				//Error - this level should not have been imported already.
				return MID_FileCorrupted;

			//Look up level in the DB.
			UINT dwLocalLevelID;

			//If this level being imported has the same local ID
			//as another level that was just imported, this level's LevelIndex
			//is corrupted and should be updated.
			//NOTE: This code will probably have to be revised if ever multiple
			//holds can be imported simultaneously with levels pointing to each other.
			while (true)
			{
				dwLocalLevelID = GetLocalID();
				if (!dwLocalLevelID)
					break;

				//Finding the local level ID in the map means a level w/ this GUID
				//was just imported, implying one of them is corrupted.
				if (info.typeBeingImported != CImportInfo::Hold)
				{
					//No actual levels are being imported, so don't modify the level GID.
					//Imported data belonging to this level (e.g. saved games) will
					//be matched to the colliding local level ID.
					bSaveRecord = false;
					break;
				} else {
					//Repair the level data.
					for (localID = info.LevelIDMap.begin();
							localID != info.LevelIDMap.end(); ++localID)
						if (localID->second == dwLocalLevelID)
							break;
					if (localID == info.LevelIDMap.end())
						break;   //didn't find another level being imported w/ same GUID -- good
					//Increment this level's index and check again for an unused value.
					++this->dwLevelIndex;
				}
			}


			if (dwLocalLevelID)
			{
				//Level found in DB, but was not just imported.
				info.LevelIDMap[this->dwLevelID] = dwLocalLevelID;
				this->dwLevelID = dwLocalLevelID;
				bSaveRecord = false;
			} else {
				//Don't save new level records to DB unless a hold is being imported.
				if (info.typeBeingImported != CImportInfo::Hold || info.bImportingSavedGames)
					bSaveRecord = false;

				if (bSaveRecord)
				{
					//Add a new record to the DB.
					const UINT dwOldLocalID = this->dwLevelID;
					this->dwLevelID = 0;
					Update();
					info.LevelIDMap[dwOldLocalID] = this->dwLevelID;
				} else {
					//This level is being ignored.
					//(It's probably a GUID reference to a non-existant hold.)
					info.LevelIDMap[this->dwLevelID] = 0;   //skip records with refs to this level ID
				}
			}
			break;
		}
		case P_HoldID:
		{
			this->dwHoldID = convertToUINT(str);
			if (!this->dwHoldID)
				return MID_FileCorrupted;  //corrupt data (can't load a level w/o its hold)

			//Set to local ID.
			localID = info.HoldIDMap.find(this->dwHoldID);
			if (localID == info.HoldIDMap.end())
				return MID_HoldNotFound;   //record should have been loaded already
			this->dwHoldID = (*localID).second;
			if (!this->dwHoldID)
			{
				//Records for this hold are being ignored.  Don't save this level.
				bSaveRecord = false;
			}
			break;
		}
		case P_PlayerID:
		{
			this->dwPlayerID = convertToUINT(str);
			if (!this->dwPlayerID)
				return MID_FileCorrupted;  //corrupt file (must have an author)

			//Set to local ID.
			localID = info.PlayerIDMap.find(this->dwPlayerID);
			if (localID == info.PlayerIDMap.end())
				return MID_FileCorrupted;  //record should have been loaded already
			this->dwPlayerID = (*localID).second;
			break;
		}
		case P_NameMessage:
		{
			WSTRING data;
			Base64::decode(str,data);
			this->NameText = data.c_str();
			break;
		}
		case P_Created:
			this->Created = convertToTimeT(str);
			break;
		case P_LastUpdated:
			this->LastUpdated = convertToTimeT(str);
			break;
		case P_GID_LevelIndex:
			this->dwLevelIndex = convertToUINT(str);
			if (!this->dwLevelIndex)
				return MID_FileCorrupted;  //corrupt file
			break;
		case P_OrderIndex:
			this->dwOrderIndex = convertToUINT(str);
			if (!this->dwOrderIndex)
				return MID_FileCorrupted;  //corrupt file
			break;
//		case P_IsRequired:
//			this->bIsRequired = convertIntStrToBool(str);
//			break;
		case P_Multiplier:
			this->dwMultiplier = convertToUINT(str);
			if (!this->dwMultiplier)  //backwards compatibility fix for v1.0.0.4
				return this->dwMultiplier = this->dwOrderIndex;
			break;

/*
		//Backwards compatibility:
		//1.6 Format
		//Move fields to record in hold's Entrances table.
		case P_DescriptionMessage:
		{
			ASSERT(!pEntrance);
			pEntrance = new CEntranceData();
			pEntrance->bIsMainEntrance = true;  //all 1.6 entrances are unique to a level
			pEntrance->bShowDescription = true; //all 1.6 entrance texts are shown
			WSTRING data;
			Base64::decode(str,data);
			pEntrance->DescriptionText = data.c_str();
			break;
		}
		case P_RoomID:
			pEntrance->dwRoomID = convertToUINT(str);
			//Room hasn't been read in yet and local ID must be set later
			break;
		case P_X:
			pEntrance->wX = convertToUINT(str);
			break;
		case P_Y:
			pEntrance->wY = convertToUINT(str);
			break;
		case P_O:
			pEntrance->wO = convertToUINT(str);
			//Done gathering entrance data.
			info.LevelEntrances.push_back(pEntrance);
			pEntrance = NULL;
			break;
*/

		default:
//			delete pEntrance;
//			pEntrance = NULL;
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbLevel::Update()
//Updates database with level.
//
//Returns: true if successful, else false.
{
	bool bSuccess=true;

	g_pTheDB->Levels.ResetMembership();

	if (this->bPartialLoad)
	{
		ASSERT(!"CDbLevel: partial load update");
		return false;
	}
	
	if (this->dwLevelID == 0)
	{
		//Insert a new level.
		bSuccess = UpdateNew();
	}
	else
	{
		//Update existing level.
		bSuccess = UpdateExisting();
	}

	return bSuccess;
}

//
//CDbLevel private methods.
//

//*****************************************************************************
UINT CDbLevel::GetLocalID() const
//Compares this object's GID fields against those of the records in the DB.
//ASSUME: dwHoldID has already been set to the local record ID
//
//Returns: local ID if a record in the DB matches this object's GUID, else 0
{
	ASSERT(IsOpen());

	//Get levels in the indicated hold.
	CIDSet levelsInHold = CDb::getLevelsInHold(this->dwHoldID);

	//Each iteration checks one of the hold's level GIDs against this level.
	c4_View LevelsView;
	for (CIDSet::const_iterator level = levelsInHold.begin();
			level != levelsInHold.end(); ++level)
	{
		const UINT levelRowI = LookupRowByPrimaryKey(*level, V_Levels, LevelsView);
		c4_RowRef row = LevelsView[levelRowI];

		//Check level index.
		const UINT dwLevelIndex = (UINT)p_GID_LevelIndex(row);
		if (this->dwLevelIndex == dwLevelIndex)
		{
			//GID matches.  Return this record's local ID.
			return (UINT) p_LevelID(row);
		}
	}

	//No match.
	return 0L;
}

//*****************************************************************************
bool CDbLevel::UpdateNew()
//Add new Levels record to database.
{
	LOGCONTEXT("CDbLevel::UpdateNew");
	ASSERT(this->dwLevelID == 0);
	ASSERT(IsOpen());

	//Prepare props.
	this->dwLevelID = GetIncrementedID(p_LevelID);
	if ((time_t)this->Created == 0)
	{
		this->Created.SetToNow();
		this->LastUpdated.SetToNow();
	}
	//ASSERT(this->dwLevelIndex); //can't assume this: it will be 0 if it hasn't
	//been inserted into a hold yet, since only a level with a real dwLevelID
	//can be inserted into a hold.

	//Write out message texts.
	const UINT dwNameID = this->NameText.UpdateText();
	ASSERT(dwNameID);

	//Write Level record.
	c4_RowRef row = g_pTheDB->Levels.GetNewRow();
	p_LevelID(row) = this->dwLevelID;
	p_HoldID(row) = this->dwHoldID;
	p_PlayerID(row) = this->dwPlayerID;
	p_NameMessageID(row) = dwNameID;
	p_Created(row) = UINT(this->Created);
	p_LastUpdated(row) = UINT(this->LastUpdated);
	p_GID_LevelIndex(row) = this->dwLevelIndex;
	p_OrderIndex(row) = this->dwOrderIndex;
//	p_IsRequired(row) = this->bIsRequired;
	p_Multiplier(row) = this->dwMultiplier;

	CDb::addLevelToHold(this->dwLevelID, this->dwHoldID);
	return true;
}

//*****************************************************************************
bool CDbLevel::UpdateExisting()
//Update an existing Levels record in database.
{
	LOGCONTEXT("CDbLevel::UpdateExisting");
	ASSERT(this->dwLevelID != 0);
	ASSERT(IsOpen());

	//Lookup Levels record.
	c4_View LevelsView;
	const UINT dwLevelI = LookupRowByPrimaryKey(this->dwLevelID, V_Levels,
			LevelsView);
	if (dwLevelI == ROW_NO_MATCH)
	{
		ASSERT(!"Bad LevelID.");
		return false;
	}
	c4_RowRef row = LevelsView[dwLevelI];

	ASSERT(this->dwLevelIndex);

	//Update Levels record.
	if (!CDb::FreezingTimeStamps())
		this->LastUpdated.SetToNow();
	p_LevelID(row) = this->dwLevelID;
	p_HoldID(row) = this->dwHoldID;
	p_PlayerID(row) = this->dwPlayerID;
	p_Created(row) = UINT(this->Created);
	p_LastUpdated(row) = UINT(this->LastUpdated);
	p_GID_LevelIndex(row) = this->dwLevelIndex;
	p_OrderIndex(row) = this->dwOrderIndex;
//	p_IsRequired(row) = this->bIsRequired;
	p_Multiplier(row) = this->dwMultiplier;

	//Write out message texts.
	const UINT dwNameID = this->NameText.UpdateText();
	ASSERT(dwNameID);

	CDbBase::DirtyHold();
	return true;
}

//*****************************************************************************
UINT CDbLevel::GetRoomIDAtCoords(
//Finds room at specified coords in this level.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of room to find.
//
//Returns:
//RoomID of found room, or 0 if no match.
const
{
	return g_pTheDB->Rooms.FindIDAtCoords(this->dwLevelID, dwRoomX, dwRoomY);
}

//*****************************************************************************
void CDbLevel::Clear()
//Frees resources associated with this object and resets member vars.
{  
	this->dwLevelID = this->dwHoldID = this->dwPlayerID =
			this->dwStartingRoomX = this->dwStartingRoomY = this->dwStartingRoomID = 0;
	this->Created = this->LastUpdated = 0L;
	this->dwLevelIndex = this->dwOrderIndex = 0;
	this->bGotStartingRoomCoords = false;
//	this->bIsRequired = true;
	this->dwMultiplier = 0;

	this->NameText.Clear();
}

//*****************************************************************************
UINT CDbLevel::SaveCopyOfRooms(
//Make copies of all rooms in level in the DB.
//
//Returns: ID of new entrance room
//
//Params:
	const UINT dwNewLevelID,  //(in) level new rooms belong to
	CImportInfo& info,
	const UINT newHoldID)
{
	UINT dwEntranceRoomID = 0L;
	GetStartingRoomID();

	CDb db;
	db.Rooms.FilterBy(this->dwLevelID);
	for (CDbRoom *pRoom = db.Rooms.GetFirst(); pRoom != NULL; pRoom = db.Rooms.GetNext())
	{
		const bool bEntranceRoom = pRoom->dwRoomID == this->dwStartingRoomID;
		CDbRoom *pRoomCopy = pRoom->MakeCopy(info, newHoldID); //must make new message texts + data
		pRoomCopy->dwLevelID = dwNewLevelID;
		//keep room (x,y) coords synched with local levelID
		pRoomCopy->dwRoomY = (dwNewLevelID * 100) + (pRoomCopy->dwRoomY % 100);
		pRoomCopy->dwRoomID = 0;  //so this room gets added to DB as a new room
		pRoomCopy->Update();
		if (bEntranceRoom)
		{
			//Get level's new entrance room ID.
			ASSERT(dwEntranceRoomID == 0);  //there should only be one
			dwEntranceRoomID = pRoomCopy->dwRoomID;
		}

		//Keep track of room ID conversions for possible use.
		ASSERT(!info.RoomIDMap.count(pRoom->dwRoomID));
		info.RoomIDMap[pRoom->dwRoomID] = pRoomCopy->dwRoomID;

		delete pRoom;
		delete pRoomCopy;
	}

	ASSERT(dwEntranceRoomID);  //there should always be an entrance room
	return dwEntranceRoomID;
}

//*****************************************************************************
bool CDbLevel::SetMembers(
//For copy constructor and assignment operator.
//
//Params:
	const CDbLevel &Src,
	const bool bCopyLocalInfo) //[default=true]
{
	//Retain prior IDs, if requested.
	if (!bCopyLocalInfo)
		Clear();
	else {
		this->dwLevelID = Src.dwLevelID;
		this->dwHoldID = Src.dwHoldID;
		this->bGotStartingRoomCoords = Src.bGotStartingRoomCoords;
		this->dwStartingRoomX = Src.dwStartingRoomX;
		this->dwStartingRoomY = Src.dwStartingRoomY;
		this->dwStartingRoomID = Src.dwStartingRoomID;

		//Don't make a duplicate copy of the texts in DB.
		const UINT dwMessageID = Src.NameText.GetMessageID();
		if (dwMessageID) this->NameText.Bind(dwMessageID);
	}
	this->dwPlayerID = Src.dwPlayerID;
	this->dwLevelIndex = Src.dwLevelIndex;
	this->dwOrderIndex = Src.dwOrderIndex;
//	this->bIsRequired = Src.bIsRequired;
	this->dwMultiplier = Src.dwMultiplier;

	//Make a copy of the text string.
	this->NameText = Src.NameText;

	this->Created = Src.Created;
	this->LastUpdated = Src.LastUpdated;

	return true;
}
