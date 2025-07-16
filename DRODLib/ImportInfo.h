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

#ifndef IMPORTINFO_H
#define IMPORTINFO_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DbProps.h"
#include "HoldRecords.h"

#include <BackEndLib/IDSet.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Types.h>

#include <map>
#include <set>
#include <vector>

#define DROD1_6EXITFORMAT   (2000000000)

//Used during import.
//Maps old local IDs to new local IDs and stores other needed information.
typedef std::map<UINT, UINT> PrimaryKeyMap;
typedef std::multimap<UINT, UINT> PrimaryKeyMultiMap;

//Used during export.
//Tracks records with owner IDs.  May be used to test for duplicate entries.
struct RecordInfo
{
	RecordInfo(const VIEWTYPE vType, const UINT id)
		: vType(vType), id(id)
		, subType(0), x(0), y(0), playerID(0)
	{}
	VIEWTYPE vType; //record type
	UINT id;        //primary key

	UINT subType;   //more specific record info
	UINT x,y,playerID;
};
typedef std::vector<RecordInfo> RecordInfoVector;
typedef std::map<UINT, RecordInfoVector> RecordMap;

class CEntranceData;
class CImportInfo {
public:
	CImportInfo();
	~CImportInfo();

	void Clear(const bool bPartialClear=false);

	bool isDemoImport() const { return typeBeingImported == CImportInfo::Demo || typeBeingImported == CImportInfo::DemosAndSavedGames; }
	bool isSavedGameImport() const { return typeBeingImported == CImportInfo::SavedGame || typeBeingImported == CImportInfo::DemosAndSavedGames; }

	PrimaryKeyMap DataIDMap;
	PrimaryKeyMap DemoIDMap;
	PrimaryKeyMap EntranceIDMap; //for copying levels
	PrimaryKeyMap HoldIDMap;
	PrimaryKeyMap LevelIDMap;
	PrimaryKeyMultiMap PlayerIDMap; //for DLC, need to support multiple instances of same player profile being present for hold joins
	PrimaryKeyMap RoomIDMap;
	PrimaryKeyMap SavedGameIDMap;
	PrimaryKeyMap SpeechIDMap;

	bool bPreparsed;
	UINT  nData, nDemos, nHolds, nLevels, nPlayers, nRooms, nSavedGames, nSpeech;

	bool  bReplaceOldPlayers;
	bool  bReplaceOldHolds;     //confirm upgrading saved games in hold
	bool  bImportingSavedGames; //true when temporarily exported saved games are being re-imported
	bool  bQuickPlayerExport;   //export only selected player data for speed

	enum ImportType
	{
		None=0,
		Data,
		Demo,
		Hold,
		Player,
		SavedGame,
		LanguageMod,
		DemosAndSavedGames
	};
	ImportType  typeBeingImported;

	bool bAllowHoldUpgrade; //set to forbid upgrading an installed hold version
	bool bAllowHoldDowngrade;
	string  headerInfo;     //optional info set for export

	UINT dwPlayerImportedID, dwHoldImportedID, dwDemoImportedID, dwDataImportedID;
	CIDSet localHoldIDs;
	std::set<WSTRING> roomStyles; //room style references encountered in import

	string   exportedDemos, exportedSavedGames;  //saved games temporarily exported while hold is being upgraded
	WSTRING  userMessages;     //text messages for the user's convenience

	MESSAGE_ID ImportStatus;   //result of import process

//Backwards compatibility:
	std::vector<CEntranceData*> LevelEntrances;   //1.6
	UINT  dwRepairHoldID;
	UINT  wVersion;
	bool  bHasAEMod;

	//For processing records with multiple attributes requiring multiple callbacks
	HoldVar importVar;
	HoldCharacter importChar;
	CEntranceData *pEntrance; //single level entrance (1.6, deprecated since 2.0)
	CEntranceData *pImportEntrance; //hold entrances

	HoldWorldMap importWorldMap;
	UINT importWorldMapID;
	WorldMapIcon importWorldMapIcon;

	std::map<UINT, string> playerChallenges;
};

#endif //...#ifndef IMPORTINFO_H
