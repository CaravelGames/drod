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

//Db.h
//Declarations for CDb.
//Top-level class for accessing DROD data from database.

#ifndef DB_H
#define DB_H

#include "DemoRecInfo.h"
#include "DbSavedGames.h"
#include "DbDemos.h"
#include "DbHolds.h"
#include "DbData.h"
#include "DbLevels.h"
#include "DbRooms.h"
#include "DbPlayers.h"
#include "DbSpeech.h"

//******************************************************************************************
class CCurrentGame;
class CDb : public CDbBase
{
public:
	CDb();
	~CDb();

	virtual void  Commit();
	virtual void  Rollback();

	//Use these methods to access data in the context of a game.  The CCurrentGame
	//object will simplify many operations and prevent errors.
	CCurrentGame * GetImportCurrentGame();
	CCurrentGame * GetSavedCurrentGame(const UINT dwSavedGameID, CCueEvents &CueEvents,
			bool bRestoreAtRoomStart = false, const bool bNoSaves=false);
	CCurrentGame * GetNewCurrentGame(const UINT dwHoldID, CCueEvents &CueEvents,
			const UINT dwAutoSaveOptions=ASO_DEFAULT);
	CCurrentGame * GetNewTestGame(const UINT dwRoomID, CCueEvents &CueEvents,
			const UINT wX, const UINT wY, const UINT wO, const bool bNoSaves=false);

	//Get the current player record with all of his settings.
	CDbPlayer *    GetCurrentPlayer();
	CDbPackedVars  GetCurrentPlayerSettings();

	UINT       GetHoldID();
	UINT       GetPlayerID();

	CIDSet     GetChallengeDemoIDs();

	static UINT LookupRowByPrimaryKey(const UINT dwID,	const VIEWTYPE vType, c4_View &View);

	void        ResetMembership();
	void        SetHoldID(const UINT dwNewHoldID);
	void        SetPlayerID(const UINT dwNewPlayerID, const bool bCaravelLogin=true);

	//Call Update() in the derived data members below instead.
	virtual bool   Update() {return false;}

	static bool    FreezingTimeStamps() {return bFreezeTimeStamps;}
	static void    FreezeTimeStamps(const bool bFlag) {bFreezeTimeStamps = bFlag;}

	static void    SubmitSteamAchievement(const WSTRING& holdName, const string& achievement);

	//Use these members to access data directly.  Requires more knowledge of
	//the database.
	CDbDemos       Demos;
	CDbHolds       Holds;
	CDbData        Data;       //generic embedded data
	CDbLevels      Levels;
	CDbPlayers     Players;
	CDbRooms       Rooms;
	CDbSavedGames  SavedGames;
	CDbSpeeches    Speech;

	//Accelerated lookups.
	static void   addDataToHold(const UINT dataID, const UINT holdID);
	static void   addDemo(const UINT demoID, const UINT savedGameID);
	static void   addHold(const UINT holdID);
	static void   addLevelToHold(const UINT levelID, const UINT holdID);
	static void   addRoomToLevel(const UINT roomID, const UINT levelID);
	static void   addSavedGameToRoom(const UINT savedGameID, const UINT roomID);
	static void   deleteData(const UINT dataID);
	static void   deleteDemo(const UINT demoID);
	static void   deleteHold(const UINT holdID);
	static void   deleteLevel(const UINT levelID);
	static void   deleteRoom(const UINT roomID);
	static void   deleteSavedGame(const UINT savedGameID);
	static CIDSet getDataInHold(const UINT holdID);
	static CIDSet getDemosInHold(const UINT holdID);
	static CIDSet getDemosInLevel(const UINT levelID);
	static CIDSet getDemosInRoom(const UINT roomID);
	static UINT   getHoldOfDemo(const UINT demoID);
	static CIDSet getLevelsInHold(const UINT holdID);
	static CIDSet getRoomsInHold(const UINT holdID);
	static CIDSet getRoomsInLevel(const UINT levelID);
	static UINT   getSavedGameOfDemo(const UINT demoID);
	static CIDSet getSavedGamesInHold(const UINT holdID);
	static CIDSet getSavedGamesInLevel(const UINT levelID);
	static CIDSet getSavedGamesInRoom(const UINT roomID);
	static bool   holdExists(const UINT holdID);
	static bool   levelExists(const UINT levelID);
	static void   moveData(const UINT dataID, const UINT fromHoldID, const UINT toHoldID);
	static void   moveRoom(const UINT roomID, const UINT fromLevelID, const UINT toLevelID);
	static void   moveSavedGame(const UINT savedGameID, const UINT fromRoomID, const UINT toRoomID);

private:
	bool         EmptyRowsExist() const;
	void         RemoveEmptyRows();
	void         ResetEmptyRowCount();

	virtual void resetIndex();
	virtual void buildIndex();

	static UINT      dwCurrentHoldID, dwCurrentPlayerID;
	static bool       bFreezeTimeStamps;
};

//Define global pointer to the one and only CDb object.
#  ifndef INCLUDED_FROM_DB_CPP
extern CDb * g_pTheDB;
#  endif

#endif //...#ifndef DB_H
