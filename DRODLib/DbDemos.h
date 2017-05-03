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

//DbDemos.h
//Declarations for CDbDemos and CDbDemo.
//Classes for accessing demo data from database.

#ifndef DBDEMOS_H
#define DBDEMOS_H

#include "DbBase.h"
#include "DbVDInterface.h"
#include "CueEvents.h"
#include "DbMessageText.h"
#include "ImportInfo.h"
#include <BackEndLib/IDList.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Date.h>
#include <BackEndLib/MessageIDs.h>

//Demo stat constants.
static const UINT DS_FinalChecksum       = 0;  //(UINT *) Checksum.
static const UINT DS_WasRoomConquered    = 1;  //(bool *) Was room conquered?
static const UINT DS_DidPlayerDie        = 2;  //(bool *) Did player die?
static const UINT DS_ProcessedTurnCount  = 3;  //(UINT *) Number of turns processed.
static const UINT DS_DidPlayerLeaveRoom  = 4;  //(bool *) Did player leave room?
static const UINT DS_MonsterCount        = 5;  //(UINT *) Number of monsters in room.
static const UINT DS_MonsterKills        = 6;  //(UINT *) Number of monsters killed.
static const UINT DS_DidPlayerExitLevel  = 7;  //(bool *) Did player exit level?
static const UINT DS_DidHalphDie         = 8;  //(bool *) Did Halph die?
static const UINT DS_GameTurnCount       = 9;  //(UINT *) Number of actual game turns elapsed.
static const UINT DS_RoomHasConquerToken = 10; //(bool *) Does the room have a conquer token?
static const UINT DS_ConquerTokenTurn    = 11; //(UINT *) Game turn when player touched a conquer token
                                               //(set to NO_CONQUER_TOKEN_TURN if no conquer token was touched)
static const UINT DS_ChallengeCompleted  = 12; //(vector<WSTRING> *) Name of challenge completed.

//Prototypes.
bool  GetDemoStatBool(const CIDList &DemoStats, const UINT dwDSID);
UINT  GetDemoStatUint(const CIDList &DemoStats, const UINT dwDSID);

//*****************************************************************************
class CCurrentGame;
class CDbDemos;
class CDbDemo : public CDbBase
{
protected:
	friend class CCurrentGame;
	friend class CDbDemos;
	friend class CDbVDInterface<CDbDemo>;

	CDbDemo() : CDbBase() {Clear();}

public:
	virtual ~CDbDemo() {Clear();}

	void        Clear();
	UINT			GetAuthorID() const;
	CCurrentGame * GetCurrentGame() const;
	const WCHAR *  GetAuthorText() const;
	void        GetNarrationText(WSTRING &wstrText, UINT &wMoves,
			const bool bConsiderHoldCompleted=false,
			const bool bConsiderHoldMastered=false) const;
	UINT        GetPrimaryKey() const {return this->dwDemoID;}
	bool			GetTimeElapsed(UINT &dwTimeElapsed) const;
	bool        GetTimeElapsed(WSTRING &wstrText) const;
	bool        HasChallengeSubset(const set<WSTRING>& challenges) const;

	enum DemoFlag
	{
		TestedForUpload = 0,
		Victory = 1,
		Death = 2,
		Usermade = 3,
		CompletedChallenge = 4,
		MaxFlags = 32	//32 bit fields
	};
	bool        IsFlagSet(const DemoFlag eFlag) const;
	void        SetFlag(const DemoFlag eFlag, const bool bSet=true);

	bool        Load(const UINT dwDemoID, const bool bQuick=false);
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	bool        Test(CIDList &DemoStats,
			const bool bConsiderHoldCompleted=false,
			const bool bConsiderHoldMastered=false) const;
	virtual bool   Update();

	UINT       dwDemoID;
	UINT       dwSavedGameID;
	CDbMessageText DescriptionText;
	UINT        wBeginTurnNo;
	UINT        wEndTurnNo;
	UINT        wShowSequenceNo;
	UINT       dwNextDemoID;
	UINT       dwChecksum;
	bool        bIsHidden;
	UINT       dwFlags;      //bit fields

private:
	void  GetNarrationText_English(const CIDList &DemoStats, WSTRING &wstrText) const;
	bool  UpdateNew();
	bool  UpdateExisting();
};

//*****************************************************************************
class CDb;
class CDbRoom;
class CDbDemos : public CDbVDInterface<CDbDemo>
{
protected:
	friend class CDb;
	friend class CDbRoom;
	friend class CCurrentGame;

	CDbDemos()
		: CDbVDInterface<CDbDemo>(V_Demos, p_DemoID)
		, dwFilterByHoldID(0L), dwFilterByPlayerID(0L), dwFilterByRoomID(0L), dwFilterByLevelID(0L)
		, bFilterByShow(false)
		, bLoadHidden(false)
	{}

public:
	virtual void      Delete(const UINT dwDemoID);
	virtual void      ExportXML(const UINT dwVDID, CDbRefs &dbRefs, string& str, const bool bRef=false);
	void     FilterByHold(const UINT dwSetFilterByHoldID);
	void     FilterByPlayer(const UINT dwSetFilterByPlayerID);
	void     FilterByRoom(const UINT dwSetFilterByRoomID);
	void     FilterByLevel(const UINT dwSetFilterByLevelID);
	void     FilterByShow(const UINT dwHoldID, const bool bFlag=true);
	UINT     FindByChallenges(const UINT roomID, const set<WSTRING>& challenges, const bool bZeroTurnOnly=false) const;
	UINT     FindByLatest();
	void     FindHiddens(const bool bFlag);
	static UINT GetDemoID(const UINT dwRoomID, const CDate& Created, const WSTRING& authorText);
	static UINT GetDemoIDforSavedGameID(const UINT dwSavedGameID);
	static UINT GetHoldIDofDemo(const UINT dwDemoID);
	static UINT GetLevelIDofDemo(const UINT dwDemoID);
	static UINT GetSavedGameIDofDemo(const UINT dwDemoID);
	static UINT GetDemoFlags(const UINT dwDemoID);
	UINT     GetNextSequenceNo() const;
	static bool IsFlagSet(const UINT flags, const CDbDemo::DemoFlag eFlag);

	void     RemoveShowSequenceNo(const UINT wSequenceNo);
	void     RemovePendingShowSequenceNo(const UINT wSequenceNo);
	bool     ShowSequenceNoOccupied(const UINT wSequenceNo);

private:
	virtual void      LoadMembership();
	void     LoadMembership_All();
	void     LoadMembership_ByHold();
	void     LoadMembership_ByPlayer();
	void     LoadMembership_ByRoom();
	void     LoadMembership_ByLevel();
	void     LoadMembership_ByShow();

	UINT    dwFilterByHoldID, dwFilterByPlayerID, dwFilterByRoomID, dwFilterByLevelID;
	bool     bFilterByShow;
	bool     bLoadHidden;  //whether hidden saved games should be loaded also

	static CIDSet pendingSavedShowSequenceNo;
};

#endif //...#ifndef DBDEMOS_H
