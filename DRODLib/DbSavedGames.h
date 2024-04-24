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

//DbSavedGames.h
//Declarations for CDbSavedGames and CDbSavedGame.
//Classes for accessing saved game data from database.

#ifndef DBSAVEGAMES_H
#define DBSAVEGAMES_H

#include "DbVDInterface.h"
#include "DbCommands.h"
#include "DbPackedVars.h"
#include "ImportInfo.h"
#include <BackEndLib/Date.h>
#include <BackEndLib/IDSet.h>

//Save types.
enum SAVETYPE
{
	ST_Unknown = 0,
	ST_Continue=1,
	ST_Demo=2,
	ST_LevelBegin=3,
	ST_RoomBegin=4,
	ST_Checkpoint=5,
	ST_EndHold=6,          //saved on exiting a hold
	ST_SecretConquered=7,  //saved on conquering a secret room
	ST_Progress=8,         //for uploading player's room tally progress
	ST_DemoUpload=9,       //tracks demos tested to upload for high-score submission (2.0; deprecated)
	ST_PlayerTotal=10,     //contains set of all rooms ever explored/conquered
	ST_PlayerTotalMerge=11,//for merging ST_PlayerTotal saves during import
	ST_ProgressEndHold=12, //Marks an ST_Progress save as having conquered the hold
	ST_WorldMap=13,        //saved on entering a world map
	ST_Cloud=14,           //reserved for cloud sync
	ST_HoldMastered=15     //saved on mastering a hold
};

//******************************************************************************************
class CDbSavedGames;
class CCurrentGame;
class CDbRoom;
enum PlayerBehavior;
enum PlayerBehaviorState;
class CDbSavedGame : public CDbBase
{
protected:
	friend class CDbSavedGames;
	friend class CDbVDInterface<CDbSavedGame>;

	CDbSavedGame(bool bClear=true) : CDbBase() { if (bClear) Clear(); };

public:
	CDbSavedGame(CDbSavedGame &Src) : CDbBase() {SetMembers(Src);}
	CDbSavedGame &operator= (const CDbSavedGame &Src) {
		SetMembers(Src);
		return *this;
	}

	virtual ~CDbSavedGame();

	UINT        GetPrimaryKey() const {return this->dwSavedGameID;}
	CDbRoom *   GetRoom() const;
	bool     Load(const UINT dwSavedGameID, const bool bQuick=false);

	//Import handling
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);

	virtual bool   Update();

	bool    OnWorldMap() const { return worldMapID != 0; }

	UINT    dwSavedGameID;
	UINT    dwRoomID;
	UINT    worldMapID; //While set, level/room/entrance data structures in play are ignored.
	UINT    dwPlayerID;
	bool     bIsHidden;
	
	SAVETYPE eType;
	UINT     wCheckpointX;
	UINT     wCheckpointY;
	UINT     wStartRoomX, wStartRoomY, wStartRoomO, wStartRoomAppearance;
	bool     bStartRoomSwordOff;
	UINT     wStartRoomWaterTraversal;
	UINT     wStartRoomWeaponType;
	std::map<const PlayerBehavior, PlayerBehaviorState> startRoomPlayerBehaviorOverrides;
	CIDSet      ExploredRooms;
	CIDSet      ConqueredRooms;
	CIDSet      CompletedScripts;
	CIDSet      GlobalScripts;
	CIDSet      entrancesExplored;
	CDate    Created;
	CDate    LastUpdated;
	CDbCommands Commands;

	//Overall game "scores" and misc. state
	CDbPackedVars stats; //contains game stats:
		//level tallies ("<id>[d|k|m|t]"), hold var values ("v*"),
		//and world map music ("wm<id>[c]")
	UINT    dwLevelDeaths, dwLevelKills, dwLevelMoves, dwLevelTime;  //used for active level
	UINT     wVersionNo;

	WorldMapsIcons worldMapIcons;

protected:
	void     Clear(const bool bNewGame=true);

private:
	void     SaveCompletedScripts(c4_View &CompletedScriptsView) const;
	void     SaveConqueredRooms(c4_View &ConqueredRoomsView) const;
	void     SaveEntrancesExplored(c4_View &EntrancesExploredView) const;
	void     SaveExploredRooms(c4_View &ExploredRoomsView) const;
	void     SaveGlobalScripts(c4_View &GlobalScriptsView) const;
	void     SaveWorldMapIcons(c4_View &WorldMapIconsView) const;
	void     SaveFields(c4_RowRef& row);

	void     DeserializeBehaviorOverrides();
	void     SerializeBehaviorOverrides();

	bool     SetMembers(const CDbSavedGame &Src);
	bool     UpdateExisting();
	bool     UpdateNew();
};

//******************************************************************************************
class CDb;
class CDbRoom;
class CDbLevel;
class CDbHold;
class CDbSavedGames : public CDbVDInterface<CDbSavedGame>
{
protected:
	friend class CDb;
	friend class CDbRoom;
	friend class CDbLevel;
	friend class CDbHold;
	CDbSavedGames()
		: CDbVDInterface<CDbSavedGame>(V_SavedGames, p_SavedGameID)
		, dwFilterByHoldID(0L), dwFilterByLevelID(0L), dwFilterByPlayerID(0L)
		, dwFilterByRoomID(0L)
		, bLoadHidden(false)
	{ }

public:
	void        AddRoomsToPlayerTally(const UINT dwPlayerID,
			const CIDSet& ConqueredRooms, const CIDSet& ExploredRooms);
	bool        CleanupPlayerTallies();
	virtual void      Delete(const UINT dwSavedGameID);
	void        DeleteForRoom(const UINT dwRoomID);
	void        VerifyForRoom(const UINT dwRoomID);
	virtual void    ExportXML(const UINT dwSavedGameID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void        FilterByHold(const UINT dwSetFilterByHoldID);
	void        FilterByLevel(const UINT dwSetFilterByLevelID);
	void        FilterByPlayer(const UINT dwSetFilterByPlayerID);
	void        FilterByRoom(const UINT dwSetFilterByRoomID);
	void        FindHiddens(const bool bFlag);

	UINT       FindByCheckpoint(const UINT dwRoomID, const UINT wCol,
			const UINT wRow);
	UINT			FindByConqueredRoom(const UINT dwQueryRoomID);
	UINT       FindByContinue(const UINT holdID=0);
	UINT       FindByContinueLatest(const UINT dwLookupPlayerID);
	UINT       FindByEndHold(const UINT dwQueryHoldID, const UINT playerID=0);
	UINT       FindByHoldLatest(const UINT dwHoldID);
	UINT       FindByHoldMastered(const UINT dwQueryHoldID, const UINT playerID=0);
	UINT       FindByLevelBegin(const UINT dwLevelID);
	UINT       FindByLevelLatest(const UINT dwLevelID);
	UINT       FindByRoomBegin(const UINT dwRoomID);
	UINT       FindByRoomLatest(const UINT dwRoomID);
	UINT       FindByType(const SAVETYPE eType, const UINT dwPlayerID=0, const bool bBackwardsSearch=true);
	UINT       FindByHoldWorldMap(const UINT holdID, const UINT worldMapID);

	static UINT GetHoldIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetPlayerIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetRoomIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetSavedGameID(const UINT dwRoomID, const CDate& Created, const UINT dwPlayerID);

	static CIDSet GetConqueredRooms(const UINT savedGameID);
	static CIDSet GetExploredRooms(const UINT savedGameID);
	static UINT GetLastUpdated(const UINT savedGameID);
	static SAVETYPE GetType(const UINT savedGameID);
	static UINT GetWorldMapID(const UINT savedGameID);

	static bool IsDuplicateRecord(RecordMap& exportInfo, CDbSavedGame *pSavedGame);

	void       MergePlayerTotals(UINT dwPlayerID);
	UINT       SaveNewContinue(const UINT dwPlayerID);
	void       UpdatePlayerTallies(const CImportInfo& info);

private:
	virtual void      LoadMembership();
	void     LoadMembership_All();
	void     LoadMembership_ByHold(const UINT dwByHoldID);
	void     LoadMembership_ByLevel(const UINT dwByLevelID);
	void     LoadMembership_ByPlayer(const UINT dwByPlayerID);
	void     LoadMembership_ByRoom(const UINT dwByRoomID);

	UINT    dwFilterByHoldID;
	UINT    dwFilterByLevelID;
	UINT    dwFilterByPlayerID;
	UINT    dwFilterByRoomID;
	bool     bLoadHidden;  //whether hidden saved games should be loaded also
};

#endif //...#ifndef DBSAVEDGAMES_H
