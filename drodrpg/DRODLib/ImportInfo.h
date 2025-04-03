// $Id: ImportInfo.h 8650 2008-02-23 17:29:55Z mrimer $

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
#include <vector>
using std::vector;

//#define DROD1_6EXITFORMAT   (2000000000) //not needed for RPG

//Used during DB import.
//Maps old local IDs to new local IDs and stores other needed information.
typedef std::map<UINT, UINT> PrimaryKeyMap;

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
typedef vector<RecordInfo> RecordInfoVector;
typedef std::map<UINT, RecordInfoVector> RecordMap;


class CEntranceData;
class CImportInfo {
public:
	CImportInfo();
	~CImportInfo();

	void Clear(const bool bPartialClear=false);

	PrimaryKeyMap DataIDMap;
	PrimaryKeyMap DemoIDMap;
	PrimaryKeyMap EntranceIDMap; //for copying levels
	PrimaryKeyMap HoldIDMap;
	PrimaryKeyMap LevelIDMap;
	PrimaryKeyMap PlayerIDMap;
	PrimaryKeyMap RoomIDMap;
	PrimaryKeyMap SavedGameIDMap;
	PrimaryKeyMap SpeechIDMap;
	PrimaryKeyMap HighScoreIDMap;

	bool bPreparsed;
	UINT  nData, nDemos, nHolds, nLevels, nPlayers, nRooms, nSavedGames, nSpeech, nHighScore;

	bool  bReplaceOldPlayers;
	bool  bReplaceOldHolds;     //confirm upgrading saved games in hold
	bool  bImportingSavedGames; //true when temporarily exported saved games are being re-imported
//	bool  bQuickPlayerExport;   //export only selected player data for speed

	enum ImportType
	{
		None=0,
		Data,
		Demo,
		Hold,
		Player,
		SavedGame,
		LanguageMod,
		HighScore,
	};
	ImportType  typeBeingImported;

	bool bAllowHoldUpgrade; //set to forbid upgrading an installed hold version
	bool bAllowHoldDowngrade;
	string  headerInfo;     //optional info set for export

	UINT dwPlayerImportedID, dwHoldImportedID, dwDemoImportedID, importedSavedGameID, dwDataImportedID;
	CIDSet localHoldIDs;
	std::set<WSTRING> roomStyles; //room style references encountered in import

	string   exportedDemos, exportedSavedGames,
		exportedHighScores;  //saved games temporarily exported while hold is being upgraded
	WSTRING  userMessages;     //text messages for the user's convenience

	MESSAGE_ID ImportStatus;   //result of import process

	UINT  wVersion;

	//For processing records with multiple attributes requiring multiple callbacks
	HoldVar importVar;
	HoldCharacter importChar;
//	CEntranceData *pEntrance; //single level entrance (1.6, deprecated since 2.0)
	CEntranceData *pImportEntrance; //hold entrances
};

#endif //...#ifndef IMPORTINFO_H

