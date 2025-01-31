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
 * Portions created by the Initial Developer are Copyright (C) 2025
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "DbLocalHighScores.h"
#include "Db.h"

#include <BackEndLib/Base64.h>

//*****************************************************************************
CDbLocalHighScore::CDbLocalHighScore()
{
	Clear();
}

//*****************************************************************************
bool CDbLocalHighScore::Load(
//Loads a high score from database into this object.
//
//Params:
	const UINT dwLocalHighScoreID, //(in) ID of the high score to load
	const bool bQuick //(in)  [default=false] load only quick data members
)
{
	Clear();

	c4_View LocalHighScoresView;
	const UINT dwLocalHighScoreI = LookupRowByPrimaryKey(dwLocalHighScoreID,
		V_LocalHighScores, LocalHighScoresView);
	if (dwLocalHighScoreI == ROW_NO_MATCH)
		return false;
	c4_RowRef row = LocalHighScoresView[dwLocalHighScoreI];

	//Load in props from db record.
	dwHighScoreID = p_HighScoreID(row);
	dwHoldID = p_HoldID(row);
	dwPlayerID = p_PlayerID(row);
	score = p_Score(row);
	this->stats = p_Stats(row);

	c4_Bytes scorepointNameBytes = p_ScorepointName(row);
	GetWString(this->scorepointName, scorepointNameBytes);

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbLocalHighScore::SetProperty(
//Used during XML data import.
//According to pType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
	const PROPTYPE pType,   //(in) property (data member) to set
	char* const str,        //(in) string representation of value
	CImportInfo& info,      //(in/out) Import info
	bool& bSaveRecord       //(out) whether record should be saved
)
{
	static PrimaryKeyMap::iterator localID;
	switch (pType) {
		case P_HighScoreID:
		{
			this->dwHighScoreID = convertToUINT(str);
			if (!this->dwHighScoreID)
				return MID_FileCorrupted;

			//Look up local ID.
			localID = info.HighScoreIDMap.find(this->dwHoldID);
			if (localID != info.HighScoreIDMap.end())
				//Error - this high score should not have been imported already.
				return MID_FileCorrupted;

			if (bSaveRecord) {
				const UINT dwLocalHighScoreID = GetLocalID();
				if (!dwLocalHighScoreID) {
					//Import this high score into the DB.
					const UINT dwOldLocalID = this->dwHighScoreID;
					this->dwHighScoreID = 0L;
					Update();
					info.HoldIDMap[dwOldLocalID] = this->dwHighScoreID;
				} else {
					c4_View LocalHighScoresView;
					const UINT dwLocalHighScoreI = LookupRowByPrimaryKey(dwLocalHighScoreID,
						V_LocalHighScores, LocalHighScoresView);
					if (dwLocalHighScoreI == ROW_NO_MATCH) return MID_FileCorrupted;
					const int dbScore = p_Score(LocalHighScoresView[dwLocalHighScoreI]);
					if (this->score > dbScore) {
						//Overwrite existing lesser score with this one
						Update();
					}
				}
			} else {
				//This high score is being ignored
				info.HighScoreIDMap[this->dwHighScoreID] = 0;
			}
		}
		break;
		case P_HoldID:
		{
			this->dwHoldID = convertToUINT(str);
			//Set to local ID.
			localID = info.HoldIDMap.find(this->dwHoldID);
			if (localID == info.HoldIDMap.end())
				return MID_HoldNotFound;   //record should have been loaded already
			this->dwHoldID = (*localID).second;
			if (!this->dwHoldID)
			{
				//Records for this hold are being ignored. Don't save this high score.
				bSaveRecord = false;
			}
		}
		break;
		case P_PlayerID:
		{
			this->dwPlayerID = convertToUINT(str);
			//Set to local ID.
			localID = info.PlayerIDMap.find(this->dwPlayerID);
			if (localID == info.PlayerIDMap.end())
				return MID_FileCorrupted;  //record should exist now
			this->dwPlayerID = localID->second;
		}
		break;
		case P_Score:
		{
			this->score = convertToInt(str);
		}
		break;
		case P_ScorepointName:
		{
			this->scorepointName = UTF8ToUnicode(str);
		}
		break;
		case P_Stats:
		{
			BYTE* data;
			Base64::decode(str, data);
			this->stats = (const BYTE*)data;
			delete[] data;
		}
		break;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbLocalHighScore::Update()
//Update database with high score, and return if it was successful.
{
	bool bSuccess = true;

	g_pTheDB->HighScores.ResetMembership();

	if (this->bPartialLoad)
	{
		ASSERT(!"CDbLocalHighScore: partial load update");
		return false;
	}

	if (this->dwHighScoreID == 0)
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

//*****************************************************************************
void CDbLocalHighScore::Clear()
//Frees resources associated with this object and resets member vars.
{
	this->dwHighScoreID = this->dwHoldID = 
		this->dwPlayerID = this->score = 0;
	this->scorepointName.clear();
	this->stats.Clear();
}

//*****************************************************************************
UINT CDbLocalHighScore::GetLocalID() const
//Use the hold and player ID, along with the scorepoint name to find the main ID
//from DB records
//
//Returns: local ID if a record in the DB matches this object's identifiers, else 0
{
	ASSERT(IsOpen());
	ASSERT(this->dwHoldID);
	ASSERT(this->dwPlayerID);
	ASSERT(!this->scorepointName.empty());
	const UINT dwHighScoreCount = GetViewSize(V_LocalHighScores);

	//Each iteration checks the hold and player id, then the scorepoint name
	//All three need to match for the high score to be a match.
	for (UINT dwHighScoreI = 0; dwHighScoreI < dwHighScoreCount; ++dwHighScoreI)
	{
		c4_RowRef row = GetRowRef(V_LocalHighScores, dwHighScoreI);
		if (!(this->dwHoldID == p_HoldID(row) && this->dwPlayerID == p_PlayerID(row))) {
			//Not a match
			continue;
		}

		//Now get and check the scorepoint name
		WSTRING name;
		GetWString(name, p_ScorepointName(row));
		if (this->scorepointName == name) {
			return (UINT)p_HighScoreID(row);
		}
	}

	return 0;
}

//*****************************************************************************
void CDbLocalHighScore::SetMembers(const CDbLocalHighScore& src)
//For copy constructor and assignment operator.
{
	this->dwHighScoreID = src.dwHighScoreID;
	this->dwHoldID = src.dwHoldID;
	this->dwPlayerID = src.dwPlayerID;
	this->score = src.score;
	this->scorepointName = src.scorepointName;
	this->stats = src.stats;
}

//*****************************************************************************
bool CDbLocalHighScore::UpdateExisting()
//Update an existing high score record in database.
{
	LOGCONTEXT("CDbLocalHighScore::UpdateExisting");
	ASSERT(this->dwHighScoreID != 0);
	ASSERT(IsOpen());

	c4_View LocalHighScoresView;
	const UINT dwLocalHighScoreI = LookupRowByPrimaryKey(this->dwHighScoreID,
		V_LocalHighScores, LocalHighScoresView);

	if (dwLocalHighScoreI == ROW_NO_MATCH) {
		ASSERT(!"Bad local high score ID.");
		return false;
	}

	//Get stats into a buffer that can be written to db.
	UINT dwStatsSize;
	BYTE* pbytStatsBytes = this->stats.GetPackedBuffer(dwStatsSize);
	if (!pbytStatsBytes) return false;
	c4_Bytes StatsBytes(pbytStatsBytes, dwStatsSize);

	c4_RowRef row = LocalHighScoresView[dwLocalHighScoreI];

	p_HighScoreID(row) = this->dwHighScoreID;
	p_HoldID(row) = this->dwHoldID;
	p_PlayerID(row) = this->dwPlayerID;
	p_Score(row) = this->score;
	p_ScorepointName(row) = PutWString(this->scorepointName);
	p_Stats(row) = StatsBytes;

	CDbBase::DirtySave();
	return true;
}

//*****************************************************************************
bool CDbLocalHighScore::UpdateNew()
//Add new high score record to database.
{
	LOGCONTEXT("CDbLocalHighScore::UpdateNew");
	ASSERT(this->dwHighScoreID == 0);
	ASSERT(IsOpen());

	//Get stats into a buffer that can be written to db.
	UINT dwStatsSize;
	BYTE* pbytStatsBytes = this->stats.GetPackedBuffer(dwStatsSize);
	if (!pbytStatsBytes) return false;
	c4_Bytes StatsBytes(pbytStatsBytes, dwStatsSize);

	this->dwHighScoreID = GetIncrementedID(p_HighScoreID);

	c4_RowRef row = g_pTheDB->HighScores.GetNewRow();
	p_HighScoreID(row) = this->dwHighScoreID;
	p_HoldID(row) = this->dwHoldID;
	p_PlayerID(row) = this->dwPlayerID;
	p_Score(row) = this->score;
	p_ScorepointName(row) = PutWString(this->scorepointName);
	p_Stats(row) = StatsBytes;

	return true;
}

//*****************************************************************************
void CDbLocalHighScores::Delete(UINT dwLocalHighScoreID)
//Deletes records of high score with the given id
{
	ASSERT(dwLocalHighScoreID);

	c4_View LocalHighScoresView;
	const UINT dwLocalHighScoreI = LookupRowByPrimaryKey(dwLocalHighScoreID,
		V_LocalHighScores, LocalHighScoresView);

	if (dwLocalHighScoreI == ROW_NO_MATCH) {
		ASSERT(!"Bad local high score ID.");
		return;
	}

	//Delete the score
	LocalHighScoresView.RemoveAt(dwLocalHighScoreI);
	this->bIsMembershipLoaded = false;
	CDbBase::DirtySave();
}

//*****************************************************************************
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define CLOSETAG "'/>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

//*****************************************************************************
void CDbLocalHighScores::ExportXML(
//Returns: string containing XML text describing this high score
//
//Params:
	const UINT dwLocalHighScoreID, //(in)
	CDbRefs& dbRefs,               //(in/out)
	string& str,                   //(in/out)
	const bool bRef)               //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_LocalHighScores, dwLocalHighScoreID))
		return;

	dbRefs.Set(V_LocalHighScores, dwLocalHighScoreID);

	//Prepare data.
	CDbLocalHighScore* pHighScore = GetByID(dwLocalHighScoreID);
	ASSERT(pHighScore);
	if (!pHighScore)
		return; //shouldn't happen -- but this is more robust

	//Include corresponding hold and player refs.
	g_pTheDB->Players.ExportXML(pHighScore->dwPlayerID, dbRefs, str, true);
	g_pTheDB->Holds.ExportXML(pHighScore->dwHoldID, dbRefs, str, true);

	char dummy[32];

	str += STARTTAG(V_LocalHighScores, P_HoldID);
	str += INT32TOSTR(pHighScore->dwHoldID);
	str += PROPTAG(P_PlayerID);
	str += INT32TOSTR(pHighScore->dwPlayerID);
	str += PROPTAG(P_ScorepointName);
	str += Base64::encode(pHighScore->scorepointName);
	str += PROPTAG(P_Score);
	str += INT32TOSTR(pHighScore->score);

	UINT dwBufSize;
	BYTE* const pStats = pHighScore->stats.GetPackedBuffer(dwBufSize);

	if (dwBufSize > sizeof(BYTE)) {
		str += PROPTAG(P_Stats);
		str += Base64::encode(pStats, dwBufSize - sizeof(BYTE));   //strip null BYTE
	}
	delete[] pStats;

	str += PROPTAG(P_HighScoreID);
	str += INT32TOSTR(pHighScore->dwHighScoreID);
	str += CLOSETAG;

	delete pHighScore;
}

#undef STARTTAG
#undef PROPTAG
#undef CLOSETAG
#undef INT32TOSTR

//*****************************************************************************
void CDbLocalHighScores::FilterByHold(UINT dwFilterByHoldID)
{
	if (this->bIsMembershipLoaded && dwFilterByHoldID != this->dwFilterByHoldID) {
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = dwFilterByHoldID;
}

//*****************************************************************************
void CDbLocalHighScores::FilterByPlayer(UINT dwFilterByPlayerID)
{
	if (this->bIsMembershipLoaded && dwFilterByPlayerID != this->dwFilterByPlayerID) {
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByPlayerID = dwFilterByPlayerID;
}

//*****************************************************************************
UINT CDbLocalHighScores::GetIDForScorepoint(WSTRING scorepointName)
//Returns the ID of the record for the given scorepoint, or zero if the record
//couldn't be found.
//Precondition: Hold and player filters have been set.
{
	if (!(this->dwFilterByHoldID && this->dwFilterByPlayerID)) {
		ASSERT(!"CDbLocalHighScores::GetIDForScorepoint is only for filtered records");
		return 0;
	}

	if (!this->bIsMembershipLoaded) LoadMembership();

	multimap<WSTRING, UINT>::const_iterator it = scorepointNameToID.find(scorepointName);
	if (it == scorepointNameToID.end()) {
		return 0;
	}

	return it->second;
}

//*****************************************************************************
bool CDbLocalHighScores::HasScorepoint(WSTRING scorepointName)
//Returns if a scorepoint with the give name exists in the filtered set of records
{
	if (!this->bIsMembershipLoaded) LoadMembership();

	return scorepointNameToID.count(scorepointName) > 0;
}

//*****************************************************************************
void CDbLocalHighScores::LoadMembership()
//Load the membership list with all high score IDs.
{
	ASSERT(IsOpen());
	const UINT dwLocalHighScoreCount = GetViewSize();

	//Each iteration gets a high score ID and puts in membership list.
	this->MembershipIDs.clear();
	this->scorepointNameToID.clear();
	for (UINT dwHighScoreI = 0; dwHighScoreI < dwLocalHighScoreCount; ++dwHighScoreI)
	{
		c4_RowRef row = GetRowRef(V_LocalHighScores, dwHighScoreI);
		const UINT dwHoldID = p_HoldID(row);
		const UINT dwPlayerID = p_PlayerID(row);
		if ((!this->dwFilterByHoldID || dwHoldID == this->dwFilterByHoldID) &&
			(!this->dwFilterByPlayerID || dwPlayerID == this->dwFilterByPlayerID)) {
			UINT id = p_HighScoreID(row);
			this->MembershipIDs += id;

			WSTRING name;
			GetWString(name, p_ScorepointName(row));
			scorepointNameToID.insert({ name, id });
		}
	}

	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}
