// $Id: DbSavedGames.h 9283 2008-10-29 02:09:15Z mrimer $

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

#ifndef DBSAVEDGAMES_H
#define DBSAVEDGAMES_H

#include "DbVDInterface.h"
#include "DbCommands.h"
#include "DbPackedVars.h"
#include "DbSavedGameMoves.h"
#include "PlayerStats.h"
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Date.h>
#include <BackEndLib/IDSet.h>

extern const char szSavename[];

//Save types.
enum SAVETYPE
{
	ST_Unknown = 0,
	ST_Continue=1,
	ST_Demo=2,
	ST_Autosave=3,  //	ST_LevelBegin=3,
	ST_Manual=4,    //	ST_RoomBegin=4,
	ST_Quicksave=5, //	ST_Checkpoint=5,
	ST_EndHold=6,          //saved on exiting a hold
	ST_ScoreCheckpoint=7, //   ST_SecretConquered=7,  //for uploading to the server
	ST_Progress=8,         //for uploading player's room tally progress
	ST_DemoUpload=9,       //tracks demos tested to upload for high-score submission (2.0; deprecated)
	ST_PlayerTotal=10,     //contains set of all rooms ever explored/conquered
	ST_WorldMap = 11,        //saved on entering a world map
	ST_TotalMapStates=12, //for tracking the best MapState seen for each room across all sessions
};

enum MapState
{
	Invisible = 0, //Not shown on map
	NoDetail = 1, //Shown but without detail
	Preview = 2, //Explored on a previous save, not yet visited
	Explored = 3, //The player has been to this room
};

//******************************************************************************************
class CMonster;
struct ExploredRoom
{
	ExploredRoom();
	ExploredRoom(const ExploredRoom& that);
	~ExploredRoom();

	void deleteMonsters();
	void saveMonsterList(CMonster *pThatMonsterList);

	UINT roomID;
	MapState mapState;
	bool bSave;    //include in saved game if set
	UINT mapMarker; //marker placed on this room on the map

	ScriptVars::MapIcon mapIcon; //icon for minimap
	ScriptVars::MapIconState mapIconState; //style to draw map icon

	c4_Bytes SquaresBytes;
	CMonster *pMonsterList;
	CIDSet litFuses;
	CCoordStack platformDeltas;
	c4_Bytes tileLightsBytes;

	bool IsInvisible() const { return mapState == Invisible; }
	bool IsPreview() const { return mapState == Preview; }
	bool HasDetail() const { return mapState >= Preview; }

//	vector<UINT> orbTypes; //track broken orbs/plates -- no longer used
};

//******************************************************************************************
struct SAVE_INFO {
	UINT saveID;
	UINT timestamp; //sort by save time
	UINT type;
	WSTRING name;
	bool bCanValidate; //saved game record contains data to fully validate current state
};

typedef std::multimap<UINT, SAVE_INFO> SORTED_SAVES;

typedef map<UINT, MapState> RoomMapStates;

//******************************************************************************************
class CDbSavedGames;
class CCurrentGame;
class CDbRoom;
class CCharacter;
class CDbSavedGame : public CDbBase
{
protected:
	friend class CDbSavedGames;
	friend class CDbVDInterface<CDbSavedGame>;

	CDbSavedGame(bool bClear=true);

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
	static void    ResetForImport();
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);
	virtual bool   Update();

	bool           OnWorldMap() const { return worldMapID != 0; }

	//Read/write UINT and UINT shaped types to/from buffer
	static UINT  readBpUINT(const BYTE* buffer, UINT& index);
	static void  writeBpUINT(string& buffer, UINT n);

	UINT     dwSavedGameID;
	UINT     dwRoomID;
	UINT     worldMapID; //While set, level/room/entrance data structures in play are ignored.
	UINT     dwPlayerID;
	bool     bIsHidden;

	SAVETYPE eType;
	UINT     wStartRoomX, wStartRoomY, wStartRoomO, wStartRoomAppearance;
	bool     bStartRoomSwordOff;

	void      appendToGlobalMonsterList(CMonster *pMonster);
	CCharacter* getCustomEquipment(const UINT type) const;
	ExploredRoom* getExploredRoom(const UINT roomID) const;
	CIDSet    GetExploredRooms(const bool bMapOnlyAlso=false, const bool bIncludeNoSavedAlso=true) const;
	CIDSet    GetMappedRooms() const {return GetExploredRooms(true);}
	RoomMapStates GetMappedRoomsWithState() const;
	bool      IsRoomExplored(const UINT roomID, const bool bConsiderCurrentRoom=true) const;
	void      LoadExploredRooms(const c4_View& ExploredRoomsView);
	CMonster* LoadMonster(const c4_RowRef& row);
	CMonster* LoadMonsters(c4_View &MonstersView);
	void      ReloadMonsterList(CMonster *pAlternateMonsterList=NULL);
	void      removeGlobalScriptForEquipment(const UINT type);
	void      removeGlobalScripts(const CIDSet& completedScripts);
	void      RemoveMappedRoomsNotIn(const CIDSet& exploredRoomIDs, const RoomMapStates& mappedRoomIDs,
			const CIDSet& roomPreviewIDs);
	void      SetMonsterListAtRoomStart();
	void      setMonstersCurrentGame(CCurrentGame* pCurrentGame);
	vector<ExploredRoom*> ExploredRooms;

	CIDSet      CompletedScripts, entrancesExplored;
	CDate       Created;
	CDate       LastUpdated;
	CDbCommands Commands; //command sequence + timing info for the current room
	CDbSavedGameMove moves; //command sequence previous to current room (ver. 1.1+)

	//Misc game state info
	CDbPackedVars stats; //contains current global and local hold var values
	CMonster *pMonsterList, *pMonsterListAtRoomStart; //global scripts and custom equipment
	vector<CMonster*> DeadMonsters; //deactivated global scripts and custom equipment

	typedef map<UINT, map<int, int>> ScriptArrayMap;
	ScriptArrayMap scriptArrays; //unpacked hold array var values

	WorldMapsIcons worldMapIcons;

	//Version info.
	UINT     wVersionNo;
	string   checksumStr;

protected:
	void     Clear(const bool bNewGame=true);
	void     ClearDeadMonsters();
	vector<ExploredRoom*> GetCopyOfExploredRooms(const vector<ExploredRoom*>& rooms);
	void     ReplaceExploredRooms(vector<ExploredRoom*>& rooms);

private:
	void     DeleteExploredRooms();
	void     SaveCompletedScripts(c4_View &CompletedScriptsView) const;
	void     SaveEntrancesExplored(c4_View &EntrancesExploredView) const;
	void     SaveExploredRooms(c4_View &ExploredRoomsView) const;
	void     SaveMonsters(c4_View &MonstersView, CMonster *pMonsterList) const;
	void     SaveWorldMapIcons(c4_View& WorldMapIconsView) const;
	void     SerializeScriptArrays();
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
	void        AddRoomsToPlayerTally(const UINT dwPlayerID, const CIDSet& ExploredRooms);
	bool        CleanupPlayerTalliesAndMapStates();
	virtual void      Delete(const UINT dwSavedGameID);
	void        DeleteForRoom(const UINT dwRoomID);
//	void        VerifyForRoom(const UINT dwRoomID);
	virtual void      ExportXML(const UINT dwVDID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void        FilterByHold(const UINT dwSetFilterByHoldID);
	void        FilterByLevel(const UINT dwSetFilterByLevelID);
	void        FilterByPlayer(const UINT dwSetFilterByPlayerID);
	void        FilterByRoom(const UINT dwSetFilterByRoomID);

	UINT        FindByContinue(const UINT type=ST_Continue);
	UINT        FindByContinue(const UINT playerID, const UINT holdID, const UINT type);
	UINT        FindByContinueLatest(const UINT dwLookupPlayerID);
	UINT        FindByEndHold(const UINT dwQueryHoldID);
	UINT        FindByLevelLatest(const UINT dwLevelID);
	UINT        FindByName(const UINT eSavetype, const WSTRING& name);
	UINT        FindByName(const UINT eSavetype, const UINT holdID, const UINT playerID, const WSTRING *pName=NULL);
	UINT        FindByRoomLatest(const UINT dwRoomID);
	UINT        FindByType(const SAVETYPE eType, const UINT dwPlayerID=0, const bool bBackwardsSearch=true);
	UINT        FindByHoldWorldMap(const UINT holdID, const UINT worldMapID);
	void        FindHiddens(const bool bFlag);
	UINT        FindIdenticalSave(const CDbSavedGame& that);

	static CIDSet GetExploredRooms(const UINT savedGameID);
	static UINT GetHoldIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetPlayerIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetRoomIDofSavedGame(const UINT dwSavedGameID);
	static UINT GetSavedGameID(const UINT dwRoomID, const CDate& Created, const UINT dwPlayerID);
	static vector<SAVE_INFO> GetSaveInfo(const CIDSet& savedGameIDs);
	static SAVETYPE GetType(const UINT savedGameID);
	static UINT GetWorldMapID(const UINT savedGameID);

	static bool RenameSavedGame(const UINT savedGameID, const WSTRING& name);
	UINT       SaveNewContinue(const UINT dwPlayerID, const UINT type=ST_Continue);
	void       UpdatePlayerTalliesAndMapStates(const CImportInfo& info);

	RoomMapStates LoadMapStates(const UINT dwPlayerID, const CIDSet& rooms);
	void          UpdateTotalMapStates(const UINT dwPlayerID, const RoomMapStates& RoomMapStates);
	void          UpdateTotalMapStatesWithAllSavedGameRooms(const UINT dwPlayerID);
	static bool   IsMoreDetailedMapState(const MapState first, const MapState second);

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
