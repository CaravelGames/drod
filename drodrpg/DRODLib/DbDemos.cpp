// $Id: DbDemos.cpp 9346 2009-03-22 14:59:29Z mrimer $

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

//DbDemos.cpp
//Implementation of CDbDemos and CDbDemo.

#include "DbDemos.h"

#include "CurrentGame.h"
#include "Db.h"
#include "DbProps.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Wchar.h>

UINT CDbDemos::wSavedShowSequenceNo = 0;

//
//CDbDemos public methods.
//

//*****************************************************************************
void CDbDemos::Delete(
//Deletes records for a demo and associated saved game.
//
//Params:
	const UINT dwDemoID)   //(in)   Demo to delete.
{
	ASSERT(dwDemoID);

	c4_View DemosView;
	const UINT dwDemoRowI = LookupRowByPrimaryKey(dwDemoID, V_Demos, DemosView);
	if (dwDemoRowI == ROW_NO_MATCH) {ASSERT(!"Bad demo ID."); return;}
	c4_RowRef row = DemosView[dwDemoRowI];

	//Get saved game ID associated with demo.
	const UINT dwSavedGameID = p_SavedGameID(row);
	if (!dwSavedGameID) {ASSERT(!"Corrupted demo record."); return;}

	//Delete description message text(s).
	const UINT dwDescriptionMID = p_DescriptionMessageID(row);
	if (!dwDescriptionMID) {ASSERT(!"Bad message ID."); return;}
	DeleteMessage((MESSAGE_ID)dwDescriptionMID);

	CDb::deleteDemo(dwDemoID);

	//Delete the saved game.
	g_pTheDB->SavedGames.Delete(dwSavedGameID);

	//Remove demo from the show sequence, if needed.
	const UINT wShowSequenceNo = p_ShowSequenceNo(row);
	if (wShowSequenceNo)
		RemoveShowSequenceNo(wShowSequenceNo);

	//Delete the demo.
	DemosView.RemoveAt(dwDemoRowI);

	//Update any record that had this one as its next demo.
	const UINT dwDemoCount = GetViewSize();
   for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		if (dwDemoID == (UINT) p_NextDemoID(row))
			p_NextDemoID(row) = 0;
	}
  
	//After demo object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
void CDbDemos::ExportXML(
//Returns: string containing XML text describing demo with this ID
//
//Pre-condition: dwDemoID is valid
//
//Params:
	const UINT dwDemoID,   //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool /*bRef*/)       //(in)
{
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define CLOSETAG "'/>\n"
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

	if (dbRefs.IsSet(V_Demos,dwDemoID))
		return;

	dbRefs.Set(V_Demos,dwDemoID);

	CDbDemo *pDemo = GetByID(dwDemoID);
	ASSERT(pDemo);
	if (!pDemo)
		return; //shouldn't happen -- but this is more robust

	if (pDemo->dwNextDemoID && dbRefs.eSaveType != (UINT)-1)
	{
		//Include next demo first (so ID is accessible on import).
		g_pTheDB->Demos.ExportXML(pDemo->dwNextDemoID, dbRefs, str);
	}

	//Include corresponding saved game with demo.
	g_pTheDB->SavedGames.ExportXML(pDemo->dwSavedGameID, dbRefs, str);

	//Prepare data.
	WSTRING const wDescStr = (WSTRING)pDemo->DescriptionText;
	char dummy[32];

	str += STARTTAG(V_Demos, P_SavedGameID);
	str += INT32TOSTR(pDemo->dwSavedGameID);
	str += PROPTAG(P_IsHidden);
	str += INT32TOSTR(pDemo->bIsHidden);
	str += PROPTAG(P_DescriptionMessage);
	str += Base64::encode(wDescStr);
	str += PROPTAG(P_ShowSequenceNo);
	str += INT32TOSTR(pDemo->wShowSequenceNo);
	str += PROPTAG(P_BeginTurnNo);
	str += INT32TOSTR(pDemo->wBeginTurnNo);
	str += PROPTAG(P_EndTurnNo);
	str += INT32TOSTR(pDemo->wEndTurnNo);
	if (pDemo->dwNextDemoID && dbRefs.eSaveType != (UINT)-1)
	{
		str += PROPTAG(P_NextDemoID);
		str += INT32TOSTR(pDemo->dwNextDemoID);
	}
	str += PROPTAG(P_Checksum);
	str += INT32TOSTR(pDemo->dwChecksum);
	str += PROPTAG(P_Flags);
	str += INT32TOSTR(pDemo->dwFlags);
	//Put primary key last, so all message fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_DemoID);
	str += INT32TOSTR(pDemo->dwDemoID);
	str += CLOSETAG;

	delete pDemo;

#undef STARTTAG
#undef PROPTAG
#undef CLOSETAG
#undef INT32TOSTR
}

//*****************************************************************************
UINT CDbDemos::FindByLatest()
//Finds the latest demo.
//
//Returns:
//DemoID of the found demo, or 0 if no match found.
{
	ASSERT(CDbBase::IsOpen());

	//For now, it is a valid to just return the last ID in the membership.
	//If it becomes possible for a demo record stored in front of a second
	//demo record to be created later, then this will no longer work.
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(bIsMembershipLoaded);

	return this->MembershipIDs.getMax();
}

//*******************************************************************************
void CDbDemos::FindHiddens(const bool bFlag)
//Changes filter so that hidden demos will also be returned (or not).
//Other filters remain the same.
{
	if (bFlag != this->bLoadHidden)
	{
      //Membership is invalid.
		this->bIsMembershipLoaded = false;
		this->bLoadHidden = bFlag;
	}
}

//*****************************************************************************
UINT CDbDemos::GetDemoID(
//Compares arguments against demo records in the DB.
//
//Returns: demo ID if a demo record in the DB matches these parameters, else 0
//
//Params:
	const UINT dwRoomID, const CDate& Created, const WSTRING& authorText)
{
	ASSERT(IsOpen());
	c4_View SavedGamesView;

	CIDSet demosInRoom = CDb::getDemosInRoom(dwRoomID);
	
	//Each iteration checks a demo's GIDs.
	for (CIDSet::const_iterator demo = demosInRoom.begin(); demo != demosInRoom.end(); ++demo)
	{
		//Check demo's paired saved game record.
		const UINT dwSavedGameID = CDb::getSavedGameOfDemo(*demo);
		if (!dwSavedGameID) continue;
		const UINT dwSavedGameI = LookupRowByPrimaryKey(dwSavedGameID,
				V_SavedGames, SavedGamesView);
		if (dwSavedGameI == ROW_NO_MATCH)
		{
			ASSERT(!"CDbDemos::GetDemoID() -- SavedGameID is foreign key to nowhere.");
			continue;
		}

		c4_RowRef row = SavedGamesView[dwSavedGameI];
		ASSERT((UINT)p_RoomID(row) == dwRoomID);

		const time_t DemoCreated = (time_t)p_Created(row);
		if (DemoCreated != Created) continue;

		//Check author.
		const UINT dwPlayerID = (UINT)p_PlayerID(row);
		ASSERT(dwPlayerID);
		CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwPlayerID, true);
		ASSERT(pPlayer);
		if (pPlayer->NameText == authorText)
		{
			//GUIDs match.  Return this demo record's local ID.
			delete pPlayer;
			return *demo;
		}
		delete pPlayer;
	}

	//No match.
	return 0L;
}

//*****************************************************************************
UINT CDbDemos::GetDemoIDforSavedGameID(const UINT dwSavedGameID)
//Returns: demo ID matching this saved game ID, or 0 if none.
{
	//Each iteration gets a demo ID and maybe puts in membership list.
	const UINT dwDemoCount = g_pTheDB->Demos.GetViewSize();
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		//Look up saved game for this demo.
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		const UINT dwDemoSavedGameID = p_SavedGameID(row);
		if (dwSavedGameID == dwDemoSavedGameID)
			return p_DemoID(row);
	}

	//Not found.
	return 0;
}

//*****************************************************************************
UINT CDbDemos::GetHoldIDofDemo(const UINT dwDemoID)
//Get the ID of the hold a demo belongs to.
{
	return CDb::getHoldOfDemo(dwDemoID);
}

//*****************************************************************************
UINT CDbDemos::GetLevelIDofDemo(const UINT dwDemoID)
//Get the ID of the level a demo belongs to.
{
	const UINT dwSavedGameID = GetSavedGameIDofDemo(dwDemoID);
	const UINT dwRoomID = CDbSavedGames::GetRoomIDofSavedGame(dwSavedGameID);
	return CDbRooms::GetLevelIDForRoom(dwRoomID);
}

//*****************************************************************************
UINT CDbDemos::GetSavedGameIDofDemo(const UINT dwDemoID)
//Get the ID of the saved game belonging to a demo.
{
	return CDb::getSavedGameOfDemo(dwDemoID);
}

//*****************************************************************************
void CDbDemos::FilterByHold(
//Changes filter so that GetFirst() and GetNext() will return demos
//for a specified hold (and player, if specified).
//
//Params:
	const UINT dwSetFilterByHoldID) //(in)   Hold ID to filter by.  Set to 0
								//    for all holds.
{
	if (dwSetFilterByHoldID != this->dwFilterByHoldID || 
		this->dwFilterByRoomID != 0L || this->dwFilterByLevelID != 0L)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByRoomID = this->dwFilterByLevelID = 0L;
	//Don't zero filterByShow or filterByPlayer
	this->dwFilterByHoldID = dwSetFilterByHoldID;
}

//*****************************************************************************
void CDbDemos::FilterByPlayer(
//Changes filter so that GetFirst() and GetNext() will return saved games 
//for a specified player.  Other demo filters stay in effect.
//
//Params:
	const UINT dwSetFilterByPlayerID)  //(in)   Player ID to filter by.
								// Set to 0 for all demos.
								// (Other filters remain in effect.)
{
	if (dwSetFilterByPlayerID != this->dwFilterByPlayerID)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	//Don't zero other filter types.
	this->dwFilterByPlayerID = dwSetFilterByPlayerID;
}

//*****************************************************************************
void CDbDemos::FilterByRoom(
//Changes filter so that GetFirst() and GetNext() will return demos for a
//specified room.
//
//Params:
	const UINT dwSetFilterByRoomID) //(in)   Room ID to filter by.  Set to 0 for all rooms.
{
	if (dwSetFilterByRoomID != this->dwFilterByRoomID ||
			this->dwFilterByHoldID != 0L || this->dwFilterByLevelID != 0L)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = this->dwFilterByLevelID = 0L;
	this->dwFilterByRoomID = dwSetFilterByRoomID;
	//Don't zero filterByPlayer
	this->bFilterByShow = false;
}

//*****************************************************************************
void CDbDemos::FilterByLevel(
//Changes filter so that GetFirst() and GetNext() will return demos for a
//specified level.
//
//Params:
	const UINT dwSetFilterByLevelID)   //(in)   Level ID to filter by.  Set to 0 for all levels.
{
	if (dwSetFilterByLevelID != this->dwFilterByLevelID ||
			this->dwFilterByHoldID != 0L || this->dwFilterByRoomID != 0L)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = this->dwFilterByRoomID = 0L;
	this->dwFilterByLevelID = dwSetFilterByLevelID;
	//Don't zero filterByPlayer
	this->bFilterByShow = false;
}

//*****************************************************************************
void CDbDemos::FilterByShow(const bool bFlag)	//[default=true]
//Changes filter so that GetFirst() and GetNext() will return demos in the
//show sequence.
{
	if (this->bFilterByShow != bFlag)
	{
		this->bFilterByShow = bFlag;

		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	//Don't zero filterByHold
	this->dwFilterByPlayerID = this->dwFilterByRoomID = this->dwFilterByLevelID = 0L;
}

//
//CDbDemos private methods.
//

//*****************************************************************************
void CDbDemos::LoadMembership()
//Load the membership list with all demo IDs.
//Player filtering is done internal to room filtering.
{
	ASSERT(CDbBase::IsOpen());
	this->MembershipIDs.clear();

	if (this->bFilterByShow) 
		LoadMembership_ByShow();
	else if (this->dwFilterByHoldID)
		LoadMembership_ByHold();
	else if (this->dwFilterByLevelID)
		LoadMembership_ByLevel();
	else if (this->dwFilterByRoomID)
		LoadMembership_ByRoom();
	else if (this->dwFilterByPlayerID)
		LoadMembership_ByPlayer();
	else
		LoadMembership_All();

	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//*****************************************************************************
void CDbDemos::LoadMembership_All()
//Loads membership list from all saved games,
{
	//Each iteration gets a demo ID and puts in it membership list.
	const UINT dwDemoCount = GetViewSize();
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		if (this->bLoadHidden || p_IsHidden(row) == 0)
			this->MembershipIDs += p_DemoID(row);
	}
}

//*****************************************************************************
void CDbDemos::LoadMembership_ByHold()
//Loads membership list from all saved games in a specified hold,
//and for specified player, if any.
{
	c4_View DemosView, SavedGamesView;
	CIDSet demosInHold = CDb::getDemosInHold(this->dwFilterByHoldID);

	//Each iteration processes a demo ID and maybe puts it in membership list.
	for (CIDSet::const_iterator demo = demosInHold.begin(); demo != demosInHold.end(); ++demo)
	{
		const UINT demoRowI = LookupRowByPrimaryKey(*demo, V_Demos, DemosView);
		c4_RowRef row = DemosView[demoRowI];
		if (this->bLoadHidden || p_IsHidden(row) == 0)
		{
			const UINT dwDemoID = p_DemoID(row);
			//Does player ID match filter?
			if (!this->dwFilterByPlayerID)
				this->MembershipIDs += dwDemoID;
			else
			{
				//Look up saved game for this demo.
				const UINT dwSavedGameID = p_SavedGameID(row);
				const UINT dwSavedGameI = LookupRowByPrimaryKey(dwSavedGameID,
						V_SavedGames, SavedGamesView);
				if (dwSavedGameI == ROW_NO_MATCH)
				{
					ASSERT(!"SavedGameID is foreign key to nowhere.");
					continue;
				}

				const UINT dwPlayerID = (UINT) p_PlayerID(SavedGamesView[dwSavedGameI]);
				if (dwPlayerID == this->dwFilterByPlayerID)
					this->MembershipIDs += dwDemoID;
			}
		}
	}
}

//*****************************************************************************
void CDbDemos::LoadMembership_ByLevel()
//Loads membership list from all saved games in a specified level,
//and for specified player, if any.
{
	c4_View DemosView, SavedGamesView;
	CIDSet demosInLevel = CDb::getDemosInLevel(this->dwFilterByLevelID);

	//Each iteration processes a demo ID and puts in it membership list.
	for (CIDSet::const_iterator demo = demosInLevel.begin(); demo != demosInLevel.end(); ++demo)
	{
		const UINT demoRowI = LookupRowByPrimaryKey(*demo, V_Demos, DemosView);
		c4_RowRef row = DemosView[demoRowI];
		if (this->bLoadHidden || p_IsHidden(row) == 0)
		{
			//Does player ID match filter?
			if (!this->dwFilterByPlayerID)
				this->MembershipIDs += *demo;
			else
			{
				//Look up saved game for this demo.
				const UINT dwSavedGameID = p_SavedGameID(row);
				const UINT dwSavedGameI = LookupRowByPrimaryKey(dwSavedGameID,
						V_SavedGames, SavedGamesView);
				if (dwSavedGameI == ROW_NO_MATCH)
				{
					ASSERT(!"SavedGameID is foreign key to nowhere.");
					continue;
				}

				const UINT dwPlayerID = (UINT)p_PlayerID(SavedGamesView[dwSavedGameI]);
				if (dwPlayerID == this->dwFilterByPlayerID)
					this->MembershipIDs += *demo;
			}
		}
	}
}

//*****************************************************************************
void CDbDemos::LoadMembership_ByRoom()
//Loads membership list from saved games in a specified room,
//and for specified player, if any.
{
	c4_View DemosView, SavedGamesView;
	CIDSet demosInRoom = CDb::getDemosInRoom(this->dwFilterByRoomID);

	//Each iteration processes a demo ID and maybe puts in membership list.
	for (CIDSet::const_iterator demo = demosInRoom.begin(); demo != demosInRoom.end(); ++demo)
	{
		const UINT demoRowI = LookupRowByPrimaryKey(*demo, V_Demos, DemosView);
		c4_RowRef row = DemosView[demoRowI];
		if (this->bLoadHidden || p_IsHidden(row) == 0)
		{
			//Does player ID match filter?
			if (!this->dwFilterByPlayerID)
				this->MembershipIDs += *demo;
			else
			{
				const UINT dwSavedGameID = p_SavedGameID(row);
				const UINT dwSavedGameI = LookupRowByPrimaryKey(dwSavedGameID,
						V_SavedGames, SavedGamesView);
				if (dwSavedGameI == ROW_NO_MATCH)
				{
					ASSERT(!"SavedGameID is foreign key to nowhere.");
					continue;
				}

				const UINT dwPlayerID = (UINT)p_PlayerID(SavedGamesView[dwSavedGameI]);
				if (dwPlayerID == this->dwFilterByPlayerID)
					this->MembershipIDs += *demo;
			}
		}
	}
}

//*****************************************************************************
void CDbDemos::LoadMembership_ByPlayer()
//Loads membership list from saved games for a specified player,
{
	c4_View SavedGamesView;
	const UINT dwDemoCount = GetViewSize();

	//Each iteration gets a demo ID and maybe puts in membership list.
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		if (this->bLoadHidden || p_IsHidden(row) == 0)
		{
			//Look up saved game for this demo.
			const UINT dwSavedGameID = p_SavedGameID(row);
			const UINT dwSavedGameI = LookupRowByPrimaryKey(dwSavedGameID,
					V_SavedGames, SavedGamesView);
			if (dwSavedGameI == ROW_NO_MATCH)
			{
				ASSERT(!"SavedGameID is foreign key to nowhere.");
				continue;
			}

			//Does player ID match filter?
			const UINT dwPlayerID = (UINT) p_PlayerID( SavedGamesView[dwSavedGameI] );
			if (dwPlayerID == this->dwFilterByPlayerID)
				this->MembershipIDs += p_DemoID(row);
		}
	}
}

//*****************************************************************************
void CDbDemos::LoadMembership_ByShow()
//Load the membership list with all demo IDs that are in the show sequence.
//The IDs will be ordered by values of the "ShowSequenceNo" field.
//
//Note: this filter ignores hold, room, and player filters
{
	const UINT dwDemoCount = GetViewSize();

	//This will hold unordered demo IDs.
	CIDList UnorderedIDs;

	//Each iteration gets a demo ID and maybe puts in membership list.
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		//A ShowSequenceNo value greater than zero indicates that demo is in
		//the show sequence.
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		const UINT wShowSequenceNo = p_ShowSequenceNo(row);
		if (wShowSequenceNo) 
		{
			UnorderedIDs.Add((UINT) p_DemoID(row),
					new CAttachableWrapper<UINT>(wShowSequenceNo), true);
		}
	}

	const UINT wIDCount = UnorderedIDs.GetSize();
	if (!wIDCount) {this->bIsMembershipLoaded=true; return;}

	//If assertions fire below, consider the following:  The values of the ShowSequenceNo
	//field go from 1 to n without gaps or duplicate values.  n equals the number of
	//demos in the show sequence.

	//Create an array which will hold all of the demo IDs in order.
	UINT *pdwOrderedIDs = new UINT[wIDCount];
	if (!pdwOrderedIDs) return;
	memset(pdwOrderedIDs, 0, sizeof(UINT)*wIDCount);

	//Put all the IDs into an array element indicated by the associated show sequence.
	IDNODE *pID = UnorderedIDs.Get(0);
	while (pID)
	{
		const UINT wShowSequenceNo = *(static_cast<const CAttachableWrapper<UINT> *>(pID->pvPrivate));
		if (wShowSequenceNo <= wIDCount)
			pdwOrderedIDs[wShowSequenceNo - 1] = pID->dwID;
		else
			//There are gaps between the ShowSequenceNo values, i.e "1,2,4" instead of "1,2,3,4".
			ASSERT(!"Gaps between show sequence values.");
		pID = pID->pNext;
	}

	//Put ordered IDs into membership.
	for (UINT wIDI = 0; wIDI < wIDCount; ++wIDI)
	{
		if (pdwOrderedIDs[wIDI]) this->MembershipIDs += pdwOrderedIDs[wIDI];
	}

	//If below assertion fires, there are duplicate ShowSequenceNo values, i.e. "1,2,3,3".
	ASSERT(this->MembershipIDs.size() == wIDCount);

	delete[] pdwOrderedIDs;

	this->bIsMembershipLoaded = true;
}

//
//CDbDemo public methods.
//

//*****************************************************************************
void CDbDemo::Clear()
//Frees resources and zeros members.
{
	this->DescriptionText.Clear();
	this->dwDemoID = this->dwSavedGameID = this->dwNextDemoID = this->dwChecksum = 0L;
	this->wBeginTurnNo = this->wEndTurnNo = this->wShowSequenceNo = 0;
	this->bIsHidden = false;
	this->dwFlags = 0;
}

//*****************************************************************************
bool CDbDemo::Load(
//Loads a demo from database into this object.
//
//Params:
	const UINT dwLoadDemoID,  //(in) DemoID of demo to load.
	const bool /*bQuick*/) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{  
	//Find record with matching saved game ID.
	ASSERT(CDbBase::IsOpen());
	c4_View DemosView;
	const UINT dwDemoI = LookupRowByPrimaryKey(dwLoadDemoID, V_Demos, DemosView);
	if (dwDemoI == ROW_NO_MATCH) return false;
	c4_RowRef row = DemosView[dwDemoI];

	//Load in props from Demos record.
	this->dwDemoID = (UINT) (p_DemoID(row));
	this->dwSavedGameID = (UINT) (p_SavedGameID(row));
	this->DescriptionText.Bind((UINT) p_DescriptionMessageID(row));
	this->bIsHidden = ((UINT) (p_IsHidden(row)) != 0);
	this->wShowSequenceNo = (UINT) (p_ShowSequenceNo(row));
	this->wBeginTurnNo = (UINT) (p_BeginTurnNo(row));
	this->wEndTurnNo = (UINT) (p_EndTurnNo(row));
	this->dwNextDemoID = (UINT) (p_NextDemoID(row));
	this->dwChecksum = (UINT) (p_Checksum(row));
	this->dwFlags = (UINT) (p_Flags(row));

	return true;
}

//*****************************************************************************
CCurrentGame* CDbDemo::GetCurrentGame() 
//Get a current game that is loaded from the demo's saved game with turn set to 
//first turn in recorded demo.
//
//Returns:
//Current game or NULL if there was a compatibility error playing back commands.
//Other errors will return NULL, but also fire an assertion to indicate they should
//be debugged.
const
{
	ASSERT(this->dwSavedGameID);

	//Get the game.
	CCurrentGame *pGame;
	CCueEvents Ignored;
	{
		pGame = g_pTheDB->GetSavedCurrentGame(this->dwSavedGameID, Ignored, true,
				true); //don't save anything to DB during playback
		ASSERT(pGame);
		if (!pGame) return NULL;
	}
	
	//Demo current games should be used for playback purposes, in which case no
	//automatic game saving should occur during playback.
//	pGame->SetAutoSaveOptions(ASO_NONE);
	pGame->FreezeCommands();

	//Play commands to beginning turn# of demo.
	ASSERT(this->wBeginTurnNo < pGame->Commands.Count());
	if (this->wBeginTurnNo)
		pGame->SetTurn(this->wBeginTurnNo, Ignored);
	
	if (pGame->wTurnNo != this->wBeginTurnNo)
	{
		//Wasn't able to play back all the turns.
		delete pGame;
		return NULL; 
	}
	return pGame;
}

//*****************************************************************************
UINT CDbDemos::GetNextSequenceNo() const
//Returns: next available ShowSequenceNo
{
	//Iterate the next available number each time method is called.
	if (CDbDemos::wSavedShowSequenceNo)
		return ++CDbDemos::wSavedShowSequenceNo;

	const UINT dwDemoCount = GetViewSize();
	UINT wMaxSequenceNo = 0;

	//Each iteration gets a record's sequence number.
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		const UINT wShowSequenceNo = p_ShowSequenceNo(row);
		if (wShowSequenceNo > wMaxSequenceNo)
			wMaxSequenceNo = wShowSequenceNo;
	}

	CDbDemos::wSavedShowSequenceNo = wMaxSequenceNo + 1;
	return CDbDemos::wSavedShowSequenceNo;
}

//*****************************************************************************
void CDbDemos::RemoveShowSequenceNo(
//Removes sequence number and decrements all greater sequence numbers.
//
//Params:
	const UINT wSequenceNo) //(in) Sequence number to remove
{
	ASSERT(wSequenceNo);

	const UINT dwDemoCount = GetViewSize();

	//Each iteration checks a record's show sequence number.
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		const UINT wShowSequenceNo = p_ShowSequenceNo(row);
		if (wShowSequenceNo == wSequenceNo)
			p_ShowSequenceNo(row) = 0;
		else if (wShowSequenceNo > wSequenceNo)
			p_ShowSequenceNo(row) = wShowSequenceNo - 1;
	}
}

//*****************************************************************************
bool CDbDemos::ShowSequenceNoOccupied(
//Returns: whether sequence number is being used by some record
//
//Params:
	const UINT wSequenceNo) //(in)
{
	//Some no's might have been taken but not yet committed.
	if (CDbDemos::wSavedShowSequenceNo >= wSequenceNo)
		return true;

	const UINT dwDemoCount = GetViewSize();

	//Each iteration checks a record's show sequence number.
	for (UINT dwDemoI = 0; dwDemoI < dwDemoCount; ++dwDemoI)
	{
		c4_RowRef row = GetRowRef(V_Demos, dwDemoI);
		const UINT wShowSequenceNo = p_ShowSequenceNo(row);
		if (wShowSequenceNo == wSequenceNo)
			return true;
	}

	//No match.
	return false;
}

//*****************************************************************************
UINT CDbDemo::GetAuthorID() const
//Get the demo's player ID.
{
	c4_View SavedGamesView;
	const UINT dwSavedGamesRowI = LookupRowByPrimaryKey(this->dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGamesRowI == ROW_NO_MATCH) {ASSERT(!"Saved game row missing."); return 0;}
	return (UINT) p_PlayerID( SavedGamesView[dwSavedGamesRowI] );
}

//*****************************************************************************
const WCHAR * CDbDemo::GetAuthorText() const
//Returns author of the demo or NULL if not found.
{
	//Look up PlayerID from associated SavedGames record.
	c4_View SavedGamesView;
	const UINT dwSavedGamesRowI = LookupRowByPrimaryKey(this->dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGamesRowI == ROW_NO_MATCH) {ASSERT(!"Saved game row missing."); return NULL;}
	const UINT dwPlayerID = p_PlayerID( SavedGamesView[dwSavedGamesRowI] );

	//Look up NameMessageID from associated Players record.
	c4_View PlayersView;
	const UINT dwPlayersRowI = LookupRowByPrimaryKey(dwPlayerID, V_Players,
			PlayersView);
	if (dwPlayersRowI == ROW_NO_MATCH) {ASSERT(!"Player row missing."); return NULL;}
	const UINT dwNameMessageID = p_NameMessageID( PlayersView[dwPlayersRowI] );

	//Look up message text.
	return g_pTheDB->GetMessageText((MESSAGE_ID)dwNameMessageID);
}

//*****************************************************************************
bool CDbDemo::GetTimeElapsed(UINT &dwTimeElapsed) const
//Calculate the time duration covered by the demo
//
//Returns: whether time was calculated successfully
{
	dwTimeElapsed = 0;

	CCueEvents Ignored;
	CCurrentGame *pGame = g_pTheDB->GetSavedCurrentGame(this->dwSavedGameID, Ignored, true,
			true); //don't save anything to DB during playback
	if (!pGame)
	{
		ASSERT(!"Saved game for demo couldn't be loaded.");
		return false;
	}
	UINT wEndTurn = this->wEndTurnNo;
	if (pGame->Commands.Count() < wEndTurn + 1)
	{
		//Demo was probably truncated.
		wEndTurn = pGame->Commands.Count() - 1;
	}

	CDbCommands::const_iterator comIter = pGame->Commands.Get(this->wBeginTurnNo);
	UINT wTurnNo;
	for (wTurnNo = this->wBeginTurnNo; wTurnNo <= wEndTurn; ++wTurnNo)
	{
		if (comIter == pGame->Commands.end())
		{
			//(Should be handled by the Count() check above.)
			delete pGame;
			ASSERT(!"Not enough commands stored or the ending turn# is wrong.");
			return false;
		}
		dwTimeElapsed += comIter->msElapsedSinceLast;
		if (bIsComplexCommand(comIter->command))
			++comIter;	//skip rest of two-part commands
		++comIter;
	}
	delete pGame;
	return true;
}

//*****************************************************************************
bool CDbDemo::GetTimeElapsed(WSTRING &wstrText) const
//Append to string argument the time duration covered by the demo,
//in hours, minutes and seconds.
//
//Returns: whether time was calculated successfully
{
	UINT dwTimeElapsed;
	if (!GetTimeElapsed(dwTimeElapsed)) return false;

	wstrText += CDate::FormatTime(dwTimeElapsed / 1000);   //convert: ms --> s

	return true;
}

//*****************************************************************************
bool CDbDemo::IsFlagSet(const DemoFlag eFlag) const
//Returns: whether bit field flag is set
{
	ASSERT(eFlag < MaxFlags);
	const UINT dwFlagVal = 1 << eFlag;
	return (this->dwFlags & dwFlagVal) == dwFlagVal;
}

//*****************************************************************************
void CDbDemo::SetFlag(const DemoFlag eFlag, const bool bSet) //[default=true]
{
	ASSERT(eFlag < MaxFlags);
	const UINT dwFlagVal = 1 << eFlag;
	if (bSet)
	{
		this->dwFlags |= dwFlagVal;   //set bit
	} else {
		this->dwFlags &= ~dwFlagVal;  //inverse mask to reset bit
	}
}

//*****************************************************************************
void CDbDemo::GetNarrationText(
//Gets a narration of what happened in the demo, and move count.
//
//Params:
	WSTRING &wstrText, UINT &wMoves)   //(out)  Narration text; move count
const
{
	CIDList DemoStats;
	const bool bDemoCorrect = Test(DemoStats);

	GetNarrationText_English(DemoStats, wstrText);

	if (!bDemoCorrect)
	{
		//Demo was corrupted or couldn't be played through because of version
		//changes.  Tack on this explanation to the description.
		wstrText += wszLeftParen;
		wstrText += g_pTheDB->GetMessageText(MID_DemoBroken);
		wstrText += wszRightParen;
	}

	wMoves = GetDemoStatUint(DemoStats, DS_GameTurnCount);
}

//*****************************************************************************
MESSAGE_ID CDbDemo::SetProperty(
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
	static PrimaryKeyMap::iterator localID;
	static WSTRING data;

	switch (pType)
	{
		case P_DemoID:
			this->dwDemoID = convertToUINT(str);
			if (!this->dwDemoID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.DemoIDMap.find(this->dwDemoID);
			if (localID != info.DemoIDMap.end())
				//Error - this demo should not have been imported already.
				return MID_FileCorrupted;
			if (!this->dwSavedGameID)
				bSaveRecord = false; //don't save demos without an existing saved game

			if (bSaveRecord)
			{
				//Add a new demo (ignore old local ID).
				const UINT dwOldLocalID = this->dwDemoID;
				this->dwDemoID = 0;
				Update();
				info.DemoIDMap[dwOldLocalID] = this->dwDemoID;
				info.dwDemoImportedID = this->dwDemoID;  //keep track of which demo was imported
						//(If more than one, returning the ID of any of them is acceptable.)
			} else {
				//This demo is being ignored.
				//(It belongs to a non-existant hold/level/room/saved game.)
				info.DemoIDMap[this->dwDemoID] = 0;   //skip records with refs to this demo ID
				if (info.typeBeingImported == CImportInfo::Demo && !info.bImportingSavedGames)
					return MID_DemoIgnored; //notify user some demos were ignored
			}
			break;
		case P_SavedGameID:
			//This ID field is received first.
			this->dwSavedGameID = convertToUINT(str);
			if (!this->dwSavedGameID)
				return MID_FileCorrupted;

			//Look up local ID.
			localID = info.SavedGameIDMap.find(this->dwSavedGameID);
			if (localID == info.SavedGameIDMap.end())
				return MID_FileCorrupted;  //should have been loaded already
			this->dwSavedGameID = localID->second;
			if (!this->dwSavedGameID)
			{
				//Records for this saved game are being ignored.  Don't save this demo.
				c4_View DemosView;
				const UINT dwDemoRowI = LookupRowByPrimaryKey(this->dwDemoID,
						V_Demos, DemosView);
				if (dwDemoRowI != ROW_NO_MATCH)
				{
					//This is a pre-1.6 build 34/35 demo with an older XML export format.
					//It was saved to the DB before confirming whether it belonged
					//to a saved game that was actually being imported.
					//This one belongs to a saved game for a room not in the current
					//data files and must be taken back out of the DB.
					DemosView.RemoveAt(dwDemoRowI);
				}
				bSaveRecord = false;
			}
			break;
		case P_IsHidden:
			this->bIsHidden = convertIntStrToBool(str);
			break;
		case P_DescriptionMessage:
			Base64::decode(str,data);
			this->DescriptionText = data.c_str();
			break;
		case P_ShowSequenceNo:
			this->wShowSequenceNo = convertToUINT(str);
			//When importing a demo, allow viewing by any player.
			if (info.typeBeingImported == CImportInfo::Demo)
			{
				this->bIsHidden = false;
				this->wShowSequenceNo = 0;
			}
			if (this->wShowSequenceNo &&
					bSaveRecord)  //don't grab a sequence number unless it will be used
			{
				//Find next available show sequence index.
				this->wShowSequenceNo = 1;
				if (g_pTheDB->Demos.ShowSequenceNoOccupied(this->wShowSequenceNo))
					this->wShowSequenceNo = g_pTheDB->Demos.GetNextSequenceNo();
			}
			break;
		case P_BeginTurnNo:
			this->wBeginTurnNo = convertToUINT(str);
			break;
		case P_EndTurnNo:
			this->wEndTurnNo = convertToUINT(str);
			break;
		case P_NextDemoID:
			this->dwNextDemoID = convertToUINT(str);
			if (!this->dwNextDemoID) break;  //no next demo

			//Look up local ID.
			localID = info.DemoIDMap.find(this->dwNextDemoID);
			if (localID == info.DemoIDMap.end())
				return MID_FileCorrupted;  //next demo should have been loaded already
			this->dwNextDemoID = localID->second;
			break;
		case P_Checksum:
			this->dwChecksum = convertToUINT(str);
			break;
		case P_Flags:
			this->dwFlags = convertToUINT(str);
			break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbDemo::Test( 
//Starts a temporary current game and tests this demo against it, running 
//all the commands without UI interaction. Statistics are collected along the way so that 
//the results of the demo can be evaluated. 
// 
//Params:
	CIDList &DemoStats) //(out) Returned containing statistics about things that happened. 
// 
//Returns: 
//True if demo was able to be completely processed, false if not. 
const
{
	bool bSuccess = true;

	ASSERT(CDbBase::IsOpen());
	ASSERT(this->dwSavedGameID);

	//Load current game from saved game with option to restore from beginning
	//of room without playing commands.
	CCueEvents CueEvents;
	CCurrentGame *pGame = g_pTheDB->GetSavedCurrentGame(this->dwSavedGameID, CueEvents, true,
			true); //don't save anything to DB during playback
	if (!pGame)
	{
		ASSERT(!"Saved game for demo couldn't be loaded.");
		return false;
	}
	if (!pGame->bIsGameActive)
	{
		//Left the room on turn zero, e.g. stairs at the entrance.  Old versions of DROD
		//sometimes recorded demos in this situation, but they're always invalid.
		delete pGame;
		return false;
	}
	UINT wEndTurn = this->wEndTurnNo;
	if (pGame->Commands.Count() < this->wEndTurnNo + 1)
	{
		//This could happen if saved game for demo was invalidated and truncated.
		//Perform demo test as best as possible (i.e. while play sequence is valid).
		if (!pGame->Commands.Count())
		{
			delete pGame;
			return false;
		}
		wEndTurn = pGame->Commands.Count() - 1;
	}

	//While processing the command list, I don't want to take any actions that
	//will modify the command list.
	pGame->FreezeCommands();

	//Play from turn 0 to ending turn of demo.
	bool bDidPlayerDie = false;
	bool bDidPlayerLeaveRoom = false, bDidPlayerExitLevel = false;
	CDbCommands::const_iterator comIter = pGame->Commands.GetFirst();
	UINT wTurnNo, wX=(UINT)-1, wY=(UINT)-1;
	for (wTurnNo = 0; wTurnNo <= wEndTurn; ++wTurnNo)
	{
		if (comIter == pGame->Commands.end())
		{
			//(Should be handled by the Count() check above.)
			ASSERT(!"Not enough commands stored or the ending turn# is wrong.");
			bSuccess = false;
			break;
		}
		const int nCommand = static_cast<int>(comIter->command);
		if (bIsComplexCommand(nCommand))	//handle two-part commands here
			pGame->Commands.GetData(wX,wY);
		pGame->ProcessCommand(nCommand, CueEvents, wX, wY);

		//Collect statistics.
		ASSERT(!bDidPlayerDie);
		bDidPlayerDie = CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied);
	
		//Check for player leaving room or game inactive.
		bDidPlayerLeaveRoom = CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom),
				CIDA_PlayerLeftRoom);
		if (bDidPlayerLeaveRoom || !pGame->bIsGameActive)
		{
			if (wTurnNo < wEndTurn)
				bSuccess = false;

			if (CueEvents.HasOccurred(CID_ExitLevelPending) || CueEvents.HasOccurred(CID_WinGame))
				bDidPlayerExitLevel = true;
			break;
		}

		comIter = pGame->Commands.GetNext();
	}

	//Fix turn# so it indicates last command processed.
	if (wTurnNo > wEndTurn) wTurnNo = wEndTurn;

	//Populate demostats return param.
	UINT dwDemoChecksum = pGame->GetChecksum();
	UINT wProcessedTurnCount = (wTurnNo >= this->wBeginTurnNo) ? 
			wTurnNo - this->wBeginTurnNo + 1 : 0;
	DemoStats.Add(DS_FinalChecksum, new CAttachableWrapper<UINT>(dwDemoChecksum), true);
//	DemoStats.Add(DS_WasRoomConquered, new CAttachableWrapper<bool>(bWasRoomConquered), true);
	DemoStats.Add(DS_DidPlayerDie, new CAttachableWrapper<bool>(bDidPlayerDie), true);
	DemoStats.Add(DS_ProcessedTurnCount, new CAttachableWrapper<UINT>(wProcessedTurnCount), true);
	DemoStats.Add(DS_DidPlayerLeaveRoom, new CAttachableWrapper<bool>(bDidPlayerLeaveRoom), true);
//	DemoStats.Add(DS_MonsterCount, new CAttachableWrapper<UINT>(pGame->pRoom->wMonsterCount), true);
//	DemoStats.Add(DS_MonsterKills, new CAttachableWrapper<UINT>(pGame->wMonsterKills), true);
	DemoStats.Add(DS_DidPlayerExitLevel, new CAttachableWrapper<bool>(bDidPlayerExitLevel), true);
	DemoStats.Add(DS_GameTurnCount, new CAttachableWrapper<UINT>(pGame->wPlayerTurn), true);

	//Compare checksum if it was specified.
	if (this->dwChecksum)
	{
		if (dwDemoChecksum != this->dwChecksum &&
				!bDidPlayerExitLevel)   //causes occasional checksum problems
			//Probably the demo no longer produces the same results as it did when 
			//it was recorded due to changes in DRODLib.  But maybe data corruption
			//or other problems are to blame.
			bSuccess = false;
	}

	pGame->UnfreezeCommands();
	delete pGame;
	return bSuccess;
}

//*****************************************************************************
bool CDbDemo::Update()
//Updates database with demo.
{
	g_pTheDB->Demos.ResetMembership();
	g_pTheDB->Demos.ResetSavedShowSequenceNo();

	if (this->bPartialLoad)
	{
		ASSERT(false); //don't try to update partially-loaded records
		return false;
	}

	if (this->dwDemoID == 0)
	{
		//Insert a new demo.
		return UpdateNew();
	}

	//Update existing demo.
	return UpdateExisting();
}

//
//CDbDemo private methods.
//

//*****************************************************************************
bool CDbDemo::UpdateNew()
//Insert a new Demos record.
{
	LOGCONTEXT("CDbDemo::UpdateNew");
	ASSERT(this->dwDemoID == 0);
	ASSERT(IsOpen());

	this->dwDemoID = GetIncrementedID(p_DemoID);

	//Save text.
	const UINT dwDescriptionMID = this->DescriptionText.UpdateText();
	//ASSERT(dwDescriptionMID);   //will be 0 on import of 1.6-beta info

	//Add record.
	c4_RowRef row = g_pTheDB->Demos.GetNewRow();
	p_DemoID(row) = this->dwDemoID;
	p_SavedGameID(row) = this->dwSavedGameID;
	p_DescriptionMessageID(row) = dwDescriptionMID;
	p_IsHidden(row) = this->bIsHidden;
	p_ShowSequenceNo(row) = this->wShowSequenceNo;
	p_BeginTurnNo(row) = this->wBeginTurnNo;
	p_EndTurnNo(row) = this->wEndTurnNo;
	p_NextDemoID(row) = this->dwNextDemoID;
	p_Flags(row) = this->dwFlags;
	p_Checksum(row) = this->dwChecksum;

	CDb::addDemo(this->dwDemoID, this->dwSavedGameID);
	return true;
}

//*****************************************************************************
bool CDbDemo::UpdateExisting()
//Update an existing Demos record.
{
	LOGCONTEXT("CDbDemo::UpdateExisting");
	ASSERT(this->dwDemoID != 0);
	ASSERT(CDbBase::IsOpen());

	//Lookup demos record.
	c4_View DemosView;
	const UINT dwDemoI = LookupRowByPrimaryKey(this->dwDemoID, V_Demos, DemosView);
	if (dwDemoI == ROW_NO_MATCH)
	{
		ASSERT(!"DemoID is bad.");
		return false;
	}
	c4_RowRef row = DemosView[dwDemoI];

	//Save text.
	const UINT dwDescriptionMID = this->DescriptionText.UpdateText();
	ASSERT(dwDescriptionMID);

	//Update record.
	p_DemoID(row) = this->dwDemoID;
	p_SavedGameID(row) = this->dwSavedGameID;
	p_IsHidden(row) = this->bIsHidden;
	p_DescriptionMessageID(row) = dwDescriptionMID;
	p_ShowSequenceNo(row) = this->wShowSequenceNo;
	p_BeginTurnNo(row) = this->wBeginTurnNo;
	p_EndTurnNo(row) = this->wEndTurnNo;
	p_NextDemoID(row) = this->dwNextDemoID;
	p_Checksum(row) = this->dwChecksum;
	p_Flags(row) = this->dwFlags;

	CDbBase::DirtySave();
	return true;
}

//*****************************************************************************
void CDbDemo::GetNarrationText_English(
//Gets narration in English.
//
//Params:
	const CIDList &DemoStats,  //(in)   Statistics to use for narration.
	WSTRING &wstrText)         //(out) Narration text.
const
{
	//Get relevant demo statistics.
	const bool bDidPlayerDie =       GetDemoStatBool(DemoStats, DS_DidPlayerDie);
	const UINT wProcessedTurnCount = GetDemoStatUint(DemoStats, DS_GameTurnCount);
	const bool bDidPlayerLeaveRoom = GetDemoStatBool(DemoStats, DS_DidPlayerLeaveRoom);
//	const UINT wMonsterCount =       GetDemoStatUint(DemoStats, DS_MonsterCount);
	const bool bDidPlayerExitLevel = GetDemoStatBool(DemoStats, DS_DidPlayerExitLevel);

	//Player dies.
	if (bDidPlayerDie)
	{
		if (wProcessedTurnCount < 50)
			wstrText += g_pTheDB->GetMessageText(MID_DemoDescKilled1);
		wstrText += wszSpace;
		wstrText += wszSpace;
		return;
	}

	//Player left the room.
	if (bDidPlayerLeaveRoom)
	{
		if (bDidPlayerExitLevel)
			wstrText += g_pTheDB->GetMessageText(MID_DemoDescExits2);
		else
			wstrText += g_pTheDB->GetMessageText(MID_DemoDescLeaves2);
		wstrText += wszSpace;
		wstrText += wszSpace;
		return;
	}
	
	//Player didn't leave the room or conquer it.
	if (wProcessedTurnCount < 200)
		wstrText += g_pTheDB->GetMessageText(MID_DemoDescLoiters1);
	else
		wstrText += g_pTheDB->GetMessageText(MID_DemoDescLoiters2);
	wstrText += wszSpace;
	wstrText += wszSpace;
}

//Helper functions.

//*****************************************************************************
bool GetDemoStatBool(const CIDList &DemoStats, const UINT dwDSID)
{
	IDNODE *pNode = DemoStats.GetByID(dwDSID);
	if (!pNode) return false;

	const CAttachableWrapper<bool> *pBool = static_cast<CAttachableWrapper<bool> *>
			(pNode->pvPrivate);

	return *pBool;
}

//*****************************************************************************
UINT GetDemoStatUint(const CIDList &DemoStats, const UINT dwDSID)
{
	IDNODE *pNode = DemoStats.GetByID(dwDSID);
	if (!pNode) return 0;

	const CAttachableWrapper<UINT> *pUint = static_cast<CAttachableWrapper<UINT> *>
			(pNode->pvPrivate);

	return *pUint;
}

