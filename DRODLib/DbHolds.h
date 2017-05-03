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

//DbHolds.h
//Declarations for CDbHolds and CDbHold.
//Classes for accessing hold data from database.

#ifndef DBHOLDS_H
#define DBHOLDS_H

#include "DbVDInterface.h"
#include "GameConstants.h"
#include "ImportInfo.h"
#include "PlayerStats.h"
#include "HoldRecords.h"
#include "EntranceData.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Date.h>

//*****************************************************************************
class CDbHolds;
class CCharacter;
class CCurrentGame;
class CDbLevel;
class CDbHold : public CDbBase
{
protected:
	friend class CDbHolds;
	friend class CDbLevel;
	friend class CCurrentGame;
	friend class CDbVDInterface<CDbHold>;
	CDbHold();

	void CopyHoldMedia(CDbHold *pNewHold, CImportInfo& info);

public:
	CDbHold(CDbHold &Src) : CDbBase() {SetMembers(Src);}
	CDbHold &operator= (const CDbHold &Src) {
		SetMembers(Src);
		return *this;
	}

	virtual ~CDbHold();

	enum HoldStatus
	{
		NoStatus=-1,
		Homemade=0,
		JtRH=1,     //Journey to Rooted Hold
		Official=2,
		Tutorial=3,
		KDD=4,      //King Dugan's Dungeon 2.0
		TCB=5,      //The City Beneath
		GatEB=6,    //Gunthro and the Epic Blunder
		TSS=7       //The Second Sky
	};

	UINT        AddCharacter(const WCHAR* pwszName);
	void        AddEntrance(CEntranceData* pEntrance, const bool bReplaceMainEntrance=true);
	UINT        AddVar(const WCHAR* pwszName);
	UINT        AddWorldMap(const WCHAR* pwszName);
	bool        ChangeAuthor(const UINT dwNewAuthorID);
	void        CopyCustomCharacterData(HoldCharacter& ch, CDbHold *pNewHold, CImportInfo& info) const;
	bool        DeleteCharacter(const UINT dwCharID);
	bool        DeleteEntrance(CEntranceData *pEntrance);
	void        DeleteEntrancesForRoom(const UINT dwRoomID);
	bool        DeleteVar(const UINT dwVarID);
	bool        DeleteWorldMap(const UINT dwWorldMapID);
	const WCHAR *  GetAuthorText() const;
	HoldCharacter* GetCharacter(const UINT dwCharID) { return const_cast<HoldCharacter*>(GetCharacterConst(dwCharID)); }
	const HoldCharacter* GetCharacterConst(const UINT dwCharID) const;
	UINT        GetCharacterID(const WCHAR* pwszName) const;
	CDate       GetCreated() const {return this->Created;}
	CIDSet      GetDeletedDataIDs() const;
	CEntranceData* GetMainEntranceForLevel(const UINT dwLevelID) const;
	UINT        GetMainEntranceIDForLevel(const UINT dwLevelID) const;
	CEntranceData* GetEntrance(const UINT dwEntranceID) const;
	CEntranceData* GetEntranceAt(const UINT dwRoomID, const UINT wX, const UINT wY) const;
	UINT        GetEntranceIndex(CEntranceData *pEntrance) const;
	WSTRING     GetLocalScriptVarNameForID(UINT varID) const;
	UINT        GetMainEntranceRoomIDForLevel(const UINT dwLevelID) const;
	UINT        GetNewScriptID();
	UINT        GetPrimaryKey() const {return this->dwHoldID;}
	UINT        getRoleForLogicalIdentity(const UINT logicalIdentity) const;
	UINT        GetScriptID() const {return this->dwScriptID;}
	CDbLevel *  GetStartingLevel() const;
	void        getStats(RoomStats& stats) const;
	char*       getVarAccessToken(const WCHAR* pName) const;
	char*       getVarAccessToken(const char* pName) const;
	UINT        GetVarID(const WCHAR* pwszName) const;
	const WCHAR* GetVarName(const UINT dwVarID) const;
	UINT        GetWorldMapID(const WCHAR* pwszName) const;
	UINT        GetWorldMapDataID(const UINT worldMapID) const;
	HoldWorldMap::DisplayType GetWorldMapDisplayType(const UINT worldMapID) const;
	WSTRING     GetWorldMapName(const UINT worldMapID) const;
	void        InsertLevel(CDbLevel *pLevel);
	static bool IsOfficialHold(HoldStatus holdstatus);
	static bool IsVarNameGoodSyntax(const WCHAR* pName);
	static bool IsVarCharValid(WCHAR wc);
	static bool IsFunctionCharValid(WCHAR wc);
	bool        DoesWorldMapExist(UINT worldMapID) const;
	bool        Load(const UINT dwHoldID, const bool bQuick=false);
	CDbHold*    MakeCopy();
	void        MarkSpeechForDeletion(CDbSpeech* pSpeech);
	void        MarkDataForDeletion(const UINT dataID);
	bool        PlayerCanEdit(const UINT playerID) const;
	void        RemoveImageID(const UINT imageID);
	void        RemoveLevel(const UINT dwLevelID, const UINT dwNewEntranceID);
	bool        RenameCharacter(const UINT dwCharID, const WSTRING& newName);
	bool        RenameVar(const UINT dwVarID, const WSTRING& newName);
	bool        RenameWorldMap(const UINT dwWorldMapID, const WSTRING& newName);
	bool        Repair();
	bool        SaveCopyOfLevels(CDbHold *pHold, CImportInfo& info);
	bool        SetDataIDForWorldMap(const UINT worldMapID, const UINT dataID);
	bool        SetDisplayTypeForWorldMap(const UINT worldMapID, HoldWorldMap::DisplayType type);
	bool        SetOrderIndexForWorldMap(const UINT worldMapID, const UINT orderIndex);

	//Import handling
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);

	virtual bool   Update();
	UINT VarIsSetInAnySavedGame(const char* pszName, const UINT playerID) const;
	
	CDbMessageText DescriptionText;
	CDbMessageText NameText;
	CDbMessageText EndHoldText;   //message shown player on completing hold
	CDate          LastUpdated;
	UINT          dwLevelID;  //first level
	UINT          dwHoldID;
	UINT          dwPlayerID; //author (for GUID)

	ENTRANCE_VECTOR Entrances;   //all level entrance positions in the hold
	vector<HoldVar> vars;        //all the vars used in the hold
	vector<HoldCharacter*> characters; //all custom characters used in the hold
	vector<HoldWorldMap> worldMaps;

	enum EditAccess
	{
		Anyone=0,
		OnlyYou=1,
		YouAndConquerors=2,
		YouAndMasters=3
	};
	EditAccess     editingPrivileges;  //who can edit the hold

	static HoldStatus GetOfficialHoldStatus();
	HoldStatus     status;	//type of hold
	bool           bCaravelNetMedia; //whether d/led from CaravelNet

private:
	void     Clear();
	void     ClearEntrances();
	UINT     GetLocalID(const HoldStatus eStatusMatching, const CIDSet& playerIDs, UINT& matchedPlayerID) const;
	UINT     GetNewCharacterID();
	bool     LoadCharacters(c4_View &CharsView);
	bool     LoadEntrances(c4_View& EntrancesView);
	bool     LoadVars(c4_View& VarsView);
	bool     LoadWorldMaps(c4_View& WorldMapsView);
	void     SaveCharacters(c4_View &CharsView);
	void     SaveEntrances(c4_View& EntrancesView);
	void     SaveVars(c4_View& VarsView);
	void     SaveWorldMaps(c4_View& WorldMapsView);
	bool     SetMembers(const CDbHold& Src, const bool bCopyLocalInfo=true);
	bool     UpdateExisting();
	bool     UpdateNew();

	CDate          Created;    //GUID field
	UINT          dwNewLevelIndex;  //for relative level GUIDs
	UINT          dwScriptID; //incremented ID for scripts in hold
	UINT          dwVarID;    //incremented ID for hold vars
	UINT          dwCharID;   //incremented ID for hold characters
	UINT          dwWorldMapID;

	vector<UINT> deletedTextIDs;   //message text IDs to be deleted on Update
	vector<UINT> deletedDataIDs;   //data IDs to be deleted on Update
	vector<UINT> deletedSpeechIDs; //speech IDs to be deleted on Update

	map<UINT, WSTRING> localScriptVars; //in-game optimization: IDs and names of local script vars
};

//******************************************************************************************
class CDb;
class CDbRoom;
class CDbHolds : public CDbVDInterface<CDbHold>
{
protected:
	friend class CCurrentGame;
	friend class CDb;
	CDbHolds()
		: CDbVDInterface<CDbHold>(V_Holds, p_HoldID)
	{}

public:
	//For compiling the location of scripts that reference variables.
	typedef map<UINT, CCoordIndex> VARROOMS;
	struct VAR_LOCATIONS {
		VARROOMS rooms;
		set<WSTRING> characterNames;
	};
	typedef map<WSTRING, VAR_LOCATIONS> VARCOORDMAP;

	virtual void      Delete(const UINT dwHoldID);
	virtual bool      Exists(const UINT dwID) const;
	void					ExportRoomHeader(WSTRING& roomText, CDbLevel *pLevel,
			CDbRoom *pRoom, ENTRANCE_VECTOR& entrances) const;
	WSTRING           ExportSpeech(const UINT dwHoldID, const bool bCoords=true) const;
	virtual bool      ExportText(const UINT dwHoldID, CDbRefs &dbRefs, CStretchyBuffer &str);
	virtual void      ExportXML(const UINT dwHoldID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	bool        EditableHoldExists() const;
	static UINT      GetAuthorID(const UINT dwHoldID);
	void              GetEntranceIDsForRoom(const UINT dwRoomID, CIDSet& entranceIDs) const;
	void              GetEntrancesForRoom(const UINT dwRoomID, ENTRANCE_VECTOR& entrances) const;
	static UINT      GetLevelIDAtIndex(const UINT dwIndex, const UINT dwHoldID);
	static UINT      GetLevelIDAtOrderIndex(const UINT dwIndex, const UINT dwHoldID);
	static UINT      GetHoldID(const CDate& Created,
			CDbMessageText& HoldNameText, CDbMessageText& origAuthorText);
	static UINT GetHoldIDWithStatus(const CDbHold::HoldStatus status);
	static WSTRING GetHoldName(const UINT holdID);
	static CDbHold::HoldStatus GetNewestInstalledOfficialHoldStatus();
	void        GetRooms(const UINT dwHoldID, HoldStats& stats) const;
	static void GetRoomsExplored(const UINT dwHoldID, const UINT dwPlayerID,
			CIDSet& rooms, const bool bOnlyConquered=false);
	static void GetRoomsExplored(const UINT dwHoldID, const UINT dwPlayerID,
			CIDSet& exploredRooms, CIDSet& conqueredRooms);
	static void   GetScriptChallengeRefs(const CDbHold *pHold, VARCOORDMAP& varMap, CIDSet& roomIDs);
	static CIDSet GetScriptCommandRefs(const CDbHold *pHold, const bool bChallenges, VARCOORDMAP& varMap);
	static void GetScriptCommandRefsForRoom(const UINT roomID,
			const CDbHold *pHold, const bool bChallenges, VARCOORDMAP& varMap);
	WSTRING     GetScriptSpeechText(const COMMAND_VECTOR& commands,
			CDbHold *pHold, CCharacter *pCharacter, WSTRING& roomText,
			CDbLevel *pLevel, CDbRoom *pRoom, ENTRANCE_VECTOR& entrancesIgnored) const;
	UINT        GetSecretsDone(HoldStats& stats, const UINT dwHoldID,
			const UINT dwPlayerID, const bool bConqueredOnly=true) const;
	static CDbHold::HoldStatus GetStatus(const UINT dwHoldID);
	static bool IsHoldCompleted(const UINT dwHoldID, const UINT playerID);
	static bool IsHoldMastered(const UINT dwHoldID, const UINT playerID);
	bool        ScanForNewHoldMastery(const UINT dwHoldID, UINT playerID=0, const UINT ignoreMasterySaves=false) const;

	void        LogScriptVarRefs(const UINT holdID, const bool bChallenges);
	bool        PlayerCanEditHold(const UINT dwHoldID) const;

	static UINT deletingHoldID; //ID of hold in process of being deleted

private:
	static void AddScriptVarRef(VARCOORDMAP& varMap, const WCHAR* varName,
		const CDbRoom *pRoom, const CCharacter *pCharacter, const WSTRING& characterName);
	static void CheckForVarRefs(const CCharacterCommand& c, const bool bChallenges, VARCOORDMAP& varMap,
		const CDbHold *pHold, const CDbRoom *pRoom, const CCharacter *pCharacter,
		const WSTRING& characterName);
	static WSTRING GetTextForVarMap(const VARCOORDMAP& varMap);
};

#endif //...#ifndef DBHOLDS_H
