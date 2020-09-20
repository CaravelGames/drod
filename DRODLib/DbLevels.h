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

//DbLevels.h
//Declarations for CDbLevels and CDbLevel.
//Classes for accessing level data from database.

#ifndef DBLEVELS_H
#define DBLEVELS_H

#include "DbVDInterface.h"
#include "DbRooms.h"
#include "ImportInfo.h"
#include "PlayerStats.h"

//******************************************************************************************
class CDbLevels;
class CDbHold;
class CCurrentGame;
class CDbLevel : public CDbBase
{
protected:
	friend class CDbLevels;
	friend class CDbHold;
	friend class CDbRoom;
	friend class CCurrentGame;
	friend class CDbVDInterface<CDbLevel>;
	CDbLevel();

public:
	CDbLevel(CDbLevel &Src) : CDbBase() {SetMembers(Src);}
	CDbLevel &operator= (const CDbLevel &Src) {
		SetMembers(Src);
		return *this;
	}

	virtual ~CDbLevel();

	UINT       GetRoomIDAtCoords(const UINT dwRoomX, const UINT dwRoomY) const;
	const WCHAR *  GetAuthorText() const;
	CDbHold *      GetHold() const;
	UINT        GetPrimaryKey() const {return this->dwLevelID;}
	CDbRoom *      GetRoomAtCoords(const UINT dwRoomX, const UINT dwRoomY);
	void        GetRequiredRooms(CIDSet& requiredRooms);
	CDbRoom *      GetStartingRoom();
	void        GetStartingRoomCoords(UINT &dwRoomX, UINT &dwRoomY);
	UINT       GetStartingRoomID();
	void       getStats(RoomStats& stats) const;
	void       SetNewStartingRoomID(UINT roomID) { UncacheStartingRoomCoords(); dwStartingRoomID = roomID; }
	void       UncacheStartingRoomCoords() { dwStartingRoomX = dwStartingRoomY = dwStartingRoomID = 0; bGotStartingRoomCoords = false; }

	bool        Load(const UINT dwLevelID, const bool bQuick=false);
	CDbLevel*   CopyIntoHold(CDbHold *pNewHold, CImportInfo& info);
	void       RekeyExitIDs(const CDbHold* pOldHold, CDbHold *pNewHold, CImportInfo& info);
	UINT       SaveCopyOfRooms(const UINT dwNewLevelID, CImportInfo& info, const UINT newHoldID);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual bool   Update();

	UINT       dwLevelID;
	UINT       dwHoldID;
	UINT       dwPlayerID;
	CDbMessageText NameText;
	CDate      Created;
	CDate      LastUpdated;

	//Quick lookup of main starting room info
	bool       bGotStartingRoomCoords;
	UINT       dwStartingRoomX;
	UINT       dwStartingRoomY;
	void       SetStartingRoomID(const UINT dwRoomID) {this->dwStartingRoomID = dwRoomID;}

	UINT       dwLevelIndex;  //GUID w/in scope of owner hold
	UINT       dwOrderIndex;  //relative order of level in hold

	bool       bIsRequired;   //if true, "Exit Level" is shown on level completion

private:
	void     Clear();
	UINT    GetLocalID() const;
	bool     SetMembers(const CDbLevel &Src, const bool bCopyLocalInfo=true);
	bool     UpdateExisting();
	bool     UpdateNew();

	UINT       dwStartingRoomID;
};

//*****************************************************************************
class CDbLevels : public CDbVDInterface<CDbLevel>
{
protected:
	friend class CCurrentGame;
	friend class CDbHold;
	friend class CDb;
	CDbLevels()
		: CDbVDInterface<CDbLevel>(V_Levels, p_LevelID),
		  dwFilterByHoldID(0)
	{}

public:
	virtual void      Delete(const UINT dwLevelID);
	virtual bool      Exists(const UINT dwID) const;
	virtual bool   ExportText(const UINT dwLevelID, CDbRefs &dbRefs, CStretchyBuffer &str);
	virtual void   ExportXML(const UINT dwLevelID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void     FilterBy(const UINT dwSetFilterByHoldID);
	static UINT   GetHoldIDForLevel(const UINT dwLevelID);
	static WSTRING GetLevelName(const UINT levelID);
	virtual CDbLevel *   GetNew();
	static void GetRequiredRooms(const UINT levelID, CIDSet& requiredRooms);
	static void GetSecretRoomsInSet(CIDSet& roomsInLevel,
			CIDSet& roomIDs,
			UINT& dwSecretRooms, UINT& dwPlayerSecretRooms);
	static void GetRoomsExplored(const UINT dwLevelID, const UINT dwPlayerID,
			CIDSet& roomsInLevel, CIDSet& playerRoomsExploredInLevel, const bool bConquered=false);
	static UINT GetStartingRoomCoords(const UINT dwLevelID, UINT &dwRoomX, UINT &dwRoomY);

private:
	virtual void      LoadMembership();

	UINT    dwFilterByHoldID;
};

struct sortLevels {
	bool operator() (const CDbLevel* pLevel1, const CDbLevel* pLevel2) const;
};
typedef std::set<CDbLevel*, sortLevels> SORTED_LEVELS;

#endif //...#ifndef DBLEVELS_H
