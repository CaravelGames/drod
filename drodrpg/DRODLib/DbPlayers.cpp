// $Id: DbPlayers.cpp 10113 2012-04-22 05:40:36Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C)
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbPlayers.cpp.
//Implementation of CDbPlayers and CDbPlayer.

#include "Db.h"

#include "DbPlayers.h"
#include "DbProps.h"
#include "DbXML.h"
#include "SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/InputKey.h>
#include <BackEndLib/Ports.h>

using namespace InputCommands;

//
//CDbPlayers public methods.
//

//*****************************************************************************
void CDbPlayers::Delete(
//Deletes record for player with the given ID.
//By default, it leaves a reference to the player in the DB,
//as that player might be a hold/level author and we need to retain
//their info.
//
//Params:
	const UINT dwPlayerID, //(in)   ID of player(s) to delete.
	const bool bRetainRef)  //(in)   whether a player reference should be kept
									//       (default=true)
{
	ASSERT(dwPlayerID);

	c4_View PlayersView;
	const UINT dwPlayerI = LookupRowByPrimaryKey(dwPlayerID, V_Players, PlayersView);
	if (dwPlayerI == ROW_NO_MATCH) {ASSERT(!"Bad player ID."); return;}
	c4_RowRef row = PlayersView[dwPlayerI];

	if (!bRetainRef)
	{
		//Delete name and email message texts.
		const UINT dwNameMID = p_NameMessageID(row);
		if (!dwNameMID) {ASSERT(!"Bad MID for name."); return;}
		DeleteMessage(static_cast<MESSAGE_ID>(dwNameMID));
		const UINT dwOrigNameMID = p_GID_OriginalNameMessageID(row);
		if (!dwOrigNameMID) {ASSERT(!"Bad MID for original name."); return;}
		DeleteMessage(static_cast<MESSAGE_ID>(dwOrigNameMID));
		const UINT dwCaravelNetNameMID = p_CNetNameMessageID(row);
		if (!dwCaravelNetNameMID) {ASSERT(!"Bad MID for CaravelNet name."); return;}
		DeleteMessage(static_cast<MESSAGE_ID>(dwCaravelNetNameMID));
		const UINT dwCaravelNetPasswordMID = p_CNetPasswordMessageID(row);
		if (!dwCaravelNetPasswordMID) {ASSERT(!"Bad MID for CaravelNet password."); return;}
		DeleteMessage(static_cast<MESSAGE_ID>(dwCaravelNetPasswordMID));
	}

	//Delete player's demos first and then saved games (including hidden ones).
	CIDSet DemoIDs, DemoShowIDs, SavedGameIDs;

	CDb db;
	if (bRetainRef)
	{
		//If player ref is to be retained, make sure to retain all show demos too.
		db.Demos.FilterByShow(true);
		db.Demos.GetIDs(DemoShowIDs);
		db.Demos.FilterByShow(false);
	}
	db.Demos.FilterByPlayer(dwPlayerID);
	db.Demos.FindHiddens(true);
	db.Demos.GetIDs(DemoIDs);
	DemoIDs -= DemoShowIDs;
	CIDSet::const_iterator iter;
	for (iter = DemoIDs.begin(); iter != DemoIDs.end(); ++iter)
		db.Demos.Delete(*iter);
	db.SavedGames.FilterByPlayer(dwPlayerID);
	db.SavedGames.FindHiddens(true);
	db.SavedGames.GetIDs(SavedGameIDs);
	if (bRetainRef)
	{
		//If show demos are being retained, make sure to keep the associated saved games too.
		for (iter = DemoShowIDs.begin(); iter != DemoShowIDs.end(); ++iter)
		{
			CDbDemo *pDemo = db.Demos.GetByID(*iter);
			ASSERT(pDemo);
			if (pDemo)
			{
				SavedGameIDs -= pDemo->dwSavedGameID;
				delete pDemo;
			}
		}
	}
	for (iter = SavedGameIDs.begin(); iter != SavedGameIDs.end(); ++iter)
		db.SavedGames.Delete(*iter);

	db.HighScores.FilterByHold(0);
	db.HighScores.FilterByPlayer(dwPlayerID);
	CIDSet highScoreIDs = db.HighScores.GetIDs();
	for (iter = highScoreIDs.begin(); iter != highScoreIDs.end(); ++iter) {
		db.HighScores.Delete(*iter);
	}

	CDbBase::DirtyPlayer();
	if (!bRetainRef)
	{
		//Delete the player.
		PlayersView.RemoveAt(dwPlayerI);
	} else {
		//Hide the player record.
		p_IsLocal(row) = false;
	}

	//After player object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">" NEWLINE
#define CLOSETAG "'/>" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))
#define TIMETTOSTR(val) writeTimeT(dummy, sizeof(dummy), (val))

//*****************************************************************************
bool CDbPlayers::ExportText(const UINT dwPlayerID, CDbRefs &dbRefs, CStretchyBuffer &str)
//Simply serialize a player reference.
{
	if (dbRefs.IsSet(V_Players,dwPlayerID))
		return false;

	dbRefs.Set(V_Players,dwPlayerID);

	CDbPlayer *pPlayer = GetByID(dwPlayerID);
	ASSERT(pPlayer);
	if (!pPlayer)
		return false; //shouldn't happen -- but this is more robust

	char dummy[32];

	//GID.
	str += STARTTAG(V_Players, P_GID_OriginalNameMessage);
	string s = Base64::encode((WSTRING)pPlayer->OriginalNameText);
	str += s.c_str();
	str += PROPTAG(P_GID_Created);
	ASSERT((time_t)pPlayer->Created);	//this value should never be zero
	str += TIMETTOSTR((time_t)pPlayer->Created);
	str += PROPTAG(P_PlayerID);
	str += INT32TOSTR(pPlayer->dwPlayerID);
	str += CLOSETAG;

	delete pPlayer;

	return true;
}

//*****************************************************************************
void CDbPlayers::ExportXML(
//Returns: string containing XML text describing player with this ID
//
//Pre-condition: dwPlayerID is valid
//
//Params:
	const UINT dwPlayerID, //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)        //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_Players,dwPlayerID))
		return;

	dbRefs.Set(V_Players,dwPlayerID);

	CDbPlayer *pPlayer = GetByID(dwPlayerID);
	ASSERT(pPlayer);
	if (!pPlayer)
		return; //shouldn't happen -- but this is more robust

	//Prepare data.
	WSTRING const wNameStr = (WSTRING)pPlayer->NameText;
	WSTRING wOrigNameStr = (WSTRING)pPlayer->OriginalNameText;
	if (wOrigNameStr.empty()) //repair bad data
		wOrigNameStr = (WSTRING)pPlayer->OriginalNameText = wNameStr.c_str();
	WSTRING const wCaravelNetNameStr = (WSTRING)pPlayer->CNetNameText;
	WSTRING const wCaravelNetPasswordStr = (WSTRING)pPlayer->CNetPasswordText;
	char dummy[32];

	str += STARTTAG(V_Players, P_GID_OriginalNameMessage);
	str += Base64::encode(wOrigNameStr);
	str += PROPTAG(P_GID_Created);
	ASSERT((time_t)pPlayer->Created);	//this value should never be zero
	str += TIMETTOSTR((time_t)pPlayer->Created);
	str += PROPTAG(P_LastUpdated);
	if (bRef)
		str += "0"; //player reference -- don't overwrite anything
	else
		str += TIMETTOSTR((time_t)pPlayer->LastUpdated);
	str += PROPTAG(P_NameMessage);
	str += Base64::encode(wNameStr);
	str += PROPTAG(P_CNetName);
	if (bRef)
		str += "0"; //player reference -- don't display CaravelNet name
	else
		str += Base64::encode(wCaravelNetNameStr);
	str += PROPTAG(P_CNetPassword);
	if (bRef)
		str += "0"; //player reference -- don't display CaravelNet password
	else
		str += Base64::encode(wCaravelNetPasswordStr);
	str += PROPTAG(P_IsLocal);
	if (bRef)
		str += "0"; //player reference -- don't display
	else
		str += INT32TOSTR(pPlayer->bIsLocal);
	//Put primary key last, so all message fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_PlayerID);
	str += INT32TOSTR(pPlayer->dwPlayerID);
	if (bRef)
	{
		//Don't need any further information for a player reference.
		str += CLOSETAG;
	} else {
		CDb db;
		if (dbRefs.eSaveType == ST_Continue)
		{
			//Only the continue saved game, if any, should be exported.
			str += CLOSETAG;

			const UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
			if (dwContinueID)
				db.SavedGames.ExportXML(dwContinueID, dbRefs, str);
		} else if (dbRefs.eSaveType == ST_EndHold) {
			//Only the end hold saved game for the current hold, if any, should be exported.
			str += CLOSETAG;

			const UINT dwEndHoldID = g_pTheDB->SavedGames.FindByEndHold(g_pTheDB->GetHoldID());
			if (dwEndHoldID)
				db.SavedGames.ExportXML(dwEndHoldID, dbRefs, str);
		} else if (dbRefs.eSaveType == ST_Progress) {
			//This type is used exclusively for exporting general saved game progress.
			str += CLOSETAG;

			const UINT dwProgressID = g_pTheDB->SavedGames.FindByType(ST_Progress);
			if (dwProgressID)
				db.SavedGames.ExportXML(dwProgressID, dbRefs, str);
		} else {
			{
				UINT dwBufSize;
				BYTE *pSettings = pPlayer->Settings.GetPackedBuffer(dwBufSize);
				if (dwBufSize > 4)
				{
					str += PROPTAG(P_Settings);
					str += Base64::encode(pSettings,dwBufSize-4);   //remove null UINT
				}
				delete[] pSettings;
			}

			str += CLOSETAG;
			CDbXML::PerformCallbackf(0.01f);

			//Include hidden saved games (e.g. continue, end hold).
			db.SavedGames.FilterByPlayer(dwPlayerID);
			db.SavedGames.FindHiddens(true);
			CIDSet SavedGameIDs = db.SavedGames.GetIDs();
			const float fNumSavedGames = (float)SavedGameIDs.size();
			str.reserve(static_cast<UINT>(fNumSavedGames*500)); //large speed optimization

			CIDSet::const_iterator iter;
			UINT wCount=0;
/*
			//Export player's demos.
			if (!CDbXML::info.bQuickPlayerExport)
			{
				//Include hidden demos (e.g. highlight).
				CIDSet DemoIDs;
				db.Demos.FilterByPlayer(dwPlayerID);
				db.Demos.FindHiddens(true); //Include hidden demos (e.g. highlight).
				db.Demos.GetIDs(DemoIDs);
				const float fNumDemos = (float)DemoIDs.size();
				for (iter = DemoIDs.begin(); iter != DemoIDs.end(); ++iter)
				{
					if (str.size() > str.capacity()*0.98) //minimize string reallocs
						str.reserve(str.capacity() * 2);
					db.Demos.ExportXML(*iter, dbRefs, str);
					CDbXML::PerformCallbackf(0.01f + (++wCount/fNumDemos) * 0.39f);
				}
			}
			CDbXML::PerformCallbackf(0.4f);
*/

			//Export player's saved games (not attached to demos).
			for (iter = SavedGameIDs.begin(), wCount=0; iter != SavedGameIDs.end(); ++iter)
			{
				CDbXML::PerformCallbackf(++wCount/fNumSavedGames);
/*
				if (CDbXML::info.bQuickPlayerExport)
				{
					//Exclude checkpoint saves that take time to verify on import.
					CDbSavedGame *pSavedGame = db.SavedGames.GetByID(*iter, true);
					ASSERT(pSavedGame);
					const bool bSkip = pSavedGame->eType == ST_Checkpoint;
					delete pSavedGame;
					if (bSkip)
						continue;
				}
*/
				if (str.size() > str.capacity()*0.98)
					str.reserve(str.capacity() * 2);
				db.SavedGames.ExportXML(*iter, dbRefs, str);
			}

			CDbXML::PerformCallbackf(0.01f);
			db.HighScores.FilterByPlayer(dwPlayerID);
			CIDSet HighScoreIDs = db.HighScores.GetIDs();
			const float fNumHighScores = (float)SavedGameIDs.size();
			str.reserve(static_cast<UINT>(str.capacity() + fNumHighScores * 500)); //large speed optimization

			for (iter = HighScoreIDs.begin(), wCount = 0; iter != HighScoreIDs.end(); ++iter) {
				CDbXML::PerformCallbackf(++wCount / fNumHighScores);
				if (str.size() > str.capacity() * 0.98) {
					str.reserve(str.capacity() * 2);
				}

				db.HighScores.ExportXML(*iter, dbRefs, str);
			}
		}

		CDbXML::PerformCallbackf(1.0f);
	}

	delete pPlayer;
}
#undef STARTTAG
#undef PROPTAG
#undef ENDTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR
#undef TIMETTOSTR

//*****************************************************************************
void CDbPlayers::FilterByLocal(bool bVar)
//Filter membership so that it only contains local players (default) or all players.
{
	this->bFilterByLocal = bVar;
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
UINT CDbPlayers::FindByName(
//Find a player by name.
//
//Params:
	const WCHAR *pwczName)  //(in)   Case-sensitive name to look for.
//
//Returns:
//PlayerID if a match was found, 0L for no match.
{
	ASSERT(IsOpen());
	const UINT dwPlayerCount = g_pTheDB->Players.GetViewSize();

	//Each iteration checks a player name from one record.
	for (UINT dwPlayerI = 0; dwPlayerI < dwPlayerCount; ++dwPlayerI)
	{
		c4_RowRef row = GetRowRef(V_Players, dwPlayerI);
		CDbMessageText NameText;
		NameText.Bind(p_NameMessageID(row));
		if (!WCScmp(pwczName, (const WCHAR*)NameText))
			return (UINT)p_PlayerID(row); //Found it.
	}

	//No match.
	return 0L;
}

//*****************************************************************************
CDbPackedVars CDbPlayers::GetSettings(const UINT dwPlayerID)
{
	CDbPackedVars settings;
	if (!dwPlayerID)
		return settings;

	ASSERT(IsOpen());
	c4_View PlayersView;
	const UINT dwPlayerRowI = LookupRowByPrimaryKey(dwPlayerID, V_Players, PlayersView);
	if (dwPlayerRowI == ROW_NO_MATCH) {ASSERT(!"Bad player ID."); return settings;}

	settings = p_Settings(PlayersView[dwPlayerRowI]);
	CDbPlayer::ConvertInputSettings(settings);
	return settings;
}

//*****************************************************************************
bool CDbPlayers::Exists(const UINT playerID)
{
	if (!playerID)
		return false;

	ASSERT(IsOpen());
	c4_View PlayersView;
	const UINT dwPlayerRowI = LookupRowByPrimaryKey(playerID, V_Players, PlayersView);
	return dwPlayerRowI != ROW_NO_MATCH;
}

//*****************************************************************************
bool CDbPlayers::IsLocal(const UINT dwPlayerID)
//Returns: whether the indicated player is marked as local
{
	if (!dwPlayerID) return false;

	ASSERT(IsOpen());
	c4_View PlayersView;
	const UINT dwPlayerRowI = LookupRowByPrimaryKey(dwPlayerID, V_Players, PlayersView);
	if (dwPlayerRowI == ROW_NO_MATCH) {ASSERT(!"Bad player ID."); return false;}

	return (UINT) p_IsLocal(PlayersView[dwPlayerRowI]) != 0;
}

//
//CDbPlayers private methods.
//

//*****************************************************************************
void CDbPlayers::LoadMembership()
//Load the membership list with all player IDs.
{
	ASSERT(IsOpen());
	const UINT dwPlayerCount = GetViewSize();

	//Each iteration gets a player ID and maybe puts in membership list.
	this->MembershipIDs.clear();
	for (UINT dwPlayerI = 0; dwPlayerI < dwPlayerCount; ++dwPlayerI)
	{
		c4_RowRef row = GetRowRef(V_Players, dwPlayerI);
		const bool bIsLocal = ((UINT) p_IsLocal(row) != 0);
		if (bIsLocal || !this->bFilterByLocal)
			this->MembershipIDs += p_PlayerID(row);
	}
	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//
//CDbPlayer public methods.
//

//*****************************************************************************
bool CDbPlayer::Load(
//Loads a player from database into this object.
//
//Params:
	const UINT dwLoadPlayerID,   //(in) PlayerID of player to load.
	const bool /*bQuick*/) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {

	Clear();

	//Find record with matching player ID.
	ASSERT(IsOpen());
	c4_View PlayersView;
	const UINT dwPlayerI = LookupRowByPrimaryKey(dwLoadPlayerID, V_Players, PlayersView);
	if (dwPlayerI == ROW_NO_MATCH) throw CException("CDbPlayer::Load");
	c4_RowRef row = PlayersView[dwPlayerI];

	//Load in props from Players record.
	this->dwPlayerID = (UINT) (p_PlayerID(row));
	this->bIsLocal = ((UINT) p_IsLocal(row) != 0);
	this->NameText.Bind( (UINT) (p_NameMessageID(row)) );
	this->CNetNameText.Bind( (UINT) (p_CNetNameMessageID(row)) );
	this->CNetPasswordText.Bind( (UINT) (p_CNetPasswordMessageID(row)) );
	this->OriginalNameText.Bind( (UINT) (p_GID_OriginalNameMessageID(row)) );
	this->Created = (time_t) (p_GID_Created(row));
	this->LastUpdated = (time_t) (p_LastUpdated(row));
	this->Settings = p_Settings(row);

	}
	catch (CException&)
	{
		Clear();
		return false;
	}

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbPlayer::SetProperty(
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
		case P_PlayerID:
		{
			this->dwPlayerID = convertToUINT(str);
			if (!this->dwPlayerID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			PrimaryKeyMultiMap::const_iterator localPlayerID = info.PlayerIDMap.find(this->dwPlayerID);
			if (localPlayerID != info.PlayerIDMap.end())
			{
				if (info.bImportingSavedGames)
				{
					//record already handled
					bSaveRecord = false;
					break;
				}

				//Error - this player should not have been imported yet
				return MID_FileCorrupted;
			}

			//If the first player recorded being imported is only a reference,
			//this indicates only saved games are being imported, not a player.
			if (info.typeBeingImported == CImportInfo::Player &&
					!info.dwPlayerImportedID && !this->bIsLocal)
			{
				info.typeBeingImported = CImportInfo::SavedGame;
				info.dwPlayerImportedID = g_pTheDB->GetPlayerID();
				if (!info.dwPlayerImportedID)	//If there's no active player to
					return MID_PlayerIgnored;	//import saved games into, then halt import

				//Match player ID to local ID if possible for hold GUID matching.
				const CIDSet localPlayerIDs = GetLocalIDs();
				if (localPlayerIDs.empty()) {
					info.PlayerIDMap.insert(make_pair(this->dwPlayerID, 0));
				} else {
					for (CIDSet::const_iterator it = localPlayerIDs.begin(); it != localPlayerIDs.end(); ++it)
						info.PlayerIDMap.insert(make_pair(this->dwPlayerID, *it));
				}

				bSaveRecord = false;
				break;
			}

			//Look up player in the DB.
			const CIDSet localPlayerIDs = GetLocalIDs();
			if (!localPlayerIDs.empty())
			{
				//For DLC hold matching, support mapping of all player author profiles that match.
				for (CIDSet::const_iterator it = localPlayerIDs.begin(); it != localPlayerIDs.end(); ++it)
					info.PlayerIDMap.insert(make_pair(this->dwPlayerID, *it));

				//Which one we use here won't matter for purposes of DLC hold joining.
				const UINT dwLocalPlayerID = localPlayerIDs.getFirst();
				this->dwPlayerID = dwLocalPlayerID;

				c4_View PlayersView;
				const UINT dwPlayerI = CDbBase::LookupRowByPrimaryKey(
						dwLocalPlayerID, V_Players, PlayersView);
				if (dwPlayerI == ROW_NO_MATCH) return MID_FileCorrupted;
				
				if (this->bIsLocal)
				{
					//Local players are only imported when it's their data being
					//imported, i.e., this is not just a player reference for
					//something like a hold author or saved games.
					c4_RowRef row = PlayersView[dwPlayerI];

					//Don't need to look up or modify player in DB -- it's not being saved.
					if (info.typeBeingImported == CImportInfo::SavedGame ||
							info.typeBeingImported == CImportInfo::LanguageMod)
					{
						bSaveRecord = false;
						break;
					}

					ASSERT(info.typeBeingImported == CImportInfo::Player);
					const bool bOldPlayerIsLocal = 0 != p_IsLocal(row);
					if (bOldPlayerIsLocal)
					{
						//Don't replace a local player record unless it's older than
						//the one being imported.
						const time_t lastUpdated = (time_t)p_LastUpdated(row);
						if (lastUpdated >= this->LastUpdated)
						{
							//Don't reimport the player, but switch modes to import
							//only this player's saved games.  This is useful when,
							//if the player had saved games for non-loaded holds ignored
							//on a previous import, once the hold has been imported,
							//then the player can be reimported to retrieve the ignored
							//saved games.
							bSaveRecord = false;
							info.dwPlayerImportedID = this->dwPlayerID;   //keep track of player being imported anyway
							info.typeBeingImported = CImportInfo::SavedGame;
							return MID_ImportSuccessful;
						}

						//Newer player version is being imported -- prompt for overwrite confirmation.
						if (!info.bReplaceOldPlayers)
							return MID_OverwritePlayerPrompt;

						//Remove old player from the DB.  This new one replaces it.
						g_pTheDB->Players.Delete(dwLocalPlayerID);
					} else {
						//The existing player record is non-local -- it must be
						//updated no matter what (i.e., pretend the old one wasn't there).
						g_pTheDB->Players.Delete(dwLocalPlayerID);
					}
					info.dwPlayerImportedID = this->dwPlayerID;   //keep track of player being imported
				} else {
					//A non-local player is being imported -- don't have to update it.
					bSaveRecord = false;
				}
			} else {
				//Player not found in DB.

				//Add a new record to the DB.
				const UINT dwOldLocalID = this->dwPlayerID;
				this->dwPlayerID = 0;
				Update();
				info.PlayerIDMap.insert(make_pair(dwOldLocalID, this->dwPlayerID));

				//Keep track of player being imported: the first player record encountered.
				if (info.typeBeingImported == CImportInfo::Player && !info.dwPlayerImportedID)
					info.dwPlayerImportedID = this->dwPlayerID;
			}
			break;
		}
		case P_IsLocal:
			this->bIsLocal = convertIntStrToBool(str);
			break;
		case P_NameMessage:
		{
			Base64::decode(str,data);
			this->NameText = data.c_str();

			//repair bad data
			WSTRING wstrOrigName = WSTRING(this->OriginalNameText);
			if (wstrOrigName.empty())
				this->OriginalNameText = data.c_str();
			break;
		}
		case P_CNetName:
			Base64::decode(str, data);
			this->CNetNameText = data.c_str();
			break;
		case P_CNetPassword:
			Base64::decode(str, data);
			this->CNetPasswordText = data.c_str();
			break;
		case P_EMailMessage:
			//deprecated
		break;
		case P_GID_OriginalNameMessage:
			Base64::decode(str,data);
			this->OriginalNameText = data.c_str();
			break;
		case P_GID_Created:
			this->Created = convertToTimeT(str);

			//The exported creation timestamp should never be zero.
			if (this->Created == (time_t)0)
				return MID_FileCorrupted;
			break;
		case P_LastUpdated:
			this->LastUpdated = convertToTimeT(str);
			break;
		case P_Settings:
		{
			BYTE *data;
			Base64::decode(str,data);
			this->Settings = (const BYTE*)data;
			delete[] data;
			break;
		}
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbPlayer::Update()
//Updates database with player.
{
	g_pTheDB->Players.ResetMembership();
	if (this->bPartialLoad)
	{
		ASSERT(!"CDbPlayer: partial load update");
		return false;
	}

	CDbBase::DirtyPlayer();
	if (this->dwPlayerID == 0)
		//Update a new player.
		return UpdateNew();

	//Update existing player.
	return UpdateExisting();
}

//
//CDbPlayer protected methods.
//

CDbPlayer::CDbPlayer() : CDbBase()
{
	Clear();

	//Never support multiple language versions of these player texts in the DB.
	this->NameText.UseDefaultLanguage();
	this->CNetNameText.UseDefaultLanguage();
	this->CNetPasswordText.UseDefaultLanguage();
	this->OriginalNameText.UseDefaultLanguage();
}

//
//CDbPlayer private methods.
//

//*****************************************************************************
void CDbPlayer::Clear()
//Clears members of object.
{
	this->dwPlayerID = 0;
	this->bIsLocal = true;
	this->NameText.Clear();
	this->CNetNameText.Clear();
	this->CNetPasswordText.Clear();
	this->OriginalNameText.Clear();
	this->Created = this->LastUpdated = 0;
	this->Settings.Clear();
}

//*****************************************************************************
CIDSet CDbPlayer::GetLocalIDs()
//Compares this object's GID fields against those of the records in the DB.
//
//Returns: local ID if a record in the DB matches this object's GUID, else 0
{
	CIDSet ids;

	ASSERT(IsOpen());
	const UINT dwPlayerCount = GetViewSize(V_Players);

	//Each iteration checks a player's GIDs.
	for (UINT dwPlayerI = 0; dwPlayerI < dwPlayerCount; ++dwPlayerI)
	{
		//Check time of creation.
		c4_RowRef row = GetRowRef(V_Players, dwPlayerI);
		const time_t Created = (time_t)p_GID_Created(row);
		if (this->Created == Created)
		{
			//Check original name.
			const UINT dwOriginalNameMessageID = p_GID_OriginalNameMessageID(row);
			CDbMessageText FromDbOriginalNameText(static_cast<MESSAGE_ID>(dwOriginalNameMessageID));
			if (FromDbOriginalNameText == this->OriginalNameText)
			{
				//GUIDs match.  Return this record's local ID.
				ids += UINT(p_PlayerID(row));
			}
		}
	}

	return ids;
}

//*****************************************************************************
bool CDbPlayer::UpdateExisting()
//Update existing Players record in database.
{
	LOGCONTEXT("CDbPlayer::UpdateExisting");
	ASSERT(this->dwPlayerID != 0L);
	ASSERT(IsOpen());

	//Lookup Players record.
	c4_View PlayersView;
	const UINT dwPlayerI = CDbBase::LookupRowByPrimaryKey(this->dwPlayerID,
			V_Players, PlayersView);
	if (dwPlayerI == ROW_NO_MATCH)
	{
		ASSERT(!"Bad PlayerID.");
		return false;
	}
	c4_RowRef row = PlayersView[dwPlayerI];

	if (!CDb::FreezingTimeStamps())
		this->LastUpdated.SetToNow();

	//Get settings into a buffer that can be written to db.
	UINT dwSettingsSize;
	BYTE *pbytSettingsBytes = this->Settings.GetPackedBuffer(dwSettingsSize);
	if (!pbytSettingsBytes) return false;
	c4_Bytes SettingsBytes(pbytSettingsBytes, dwSettingsSize);

	//Update Players record.
	p_IsLocal(row) = this->bIsLocal;
	p_LastUpdated(row) = UINT((time_t)this->LastUpdated);
	p_Settings(row) = SettingsBytes;

	//Update message texts.
	const UINT dwNameID = this->NameText.UpdateText();
	UINT dwCaravelNetNameID = this->CNetNameText.UpdateText();
	UINT dwCaravelNetPasswordID = this->CNetPasswordText.UpdateText();
	ASSERT(dwNameID);
	//Provide null messages with empty strings.
	if (!dwCaravelNetNameID) {
		this->CNetNameText = wszEmpty;
		dwCaravelNetNameID = this->CNetNameText.UpdateText();
	}
	if (!dwCaravelNetPasswordID) {
		this->CNetPasswordText = wszEmpty;
		dwCaravelNetPasswordID = this->CNetPasswordText.UpdateText();
	}
	//don't have to update this->OriginalNameText

	delete[] pbytSettingsBytes;

	return true;
}

//*****************************************************************************
bool CDbPlayer::UpdateNew()
//Add new Players record to database.
{
	LOGCONTEXT("CDbPlayer::UpdateNew");
	ASSERT(this->dwPlayerID == 0L);
	ASSERT(CDbBase::IsOpen());

	//Prepare props.
	this->dwPlayerID = GetIncrementedID(p_PlayerID);
	if (this->OriginalNameText.IsEmpty())
		this->OriginalNameText = this->NameText;  //for GUID
	if ((time_t)this->Created == 0)
	{
		this->Created.SetToNow();
		this->LastUpdated.SetToNow();
	}

	//Get settings into a buffer that can be written to db.
	UINT dwSettingsSize;
	BYTE *pbytSettingsBytes = this->Settings.GetPackedBuffer(dwSettingsSize);
	if (!pbytSettingsBytes) return false;
	c4_Bytes SettingsBytes(pbytSettingsBytes, dwSettingsSize);

	//Write out message texts.
	const UINT dwNameID = this->NameText.UpdateText();
	const UINT dwOrigNameID = this->OriginalNameText.UpdateText();
	UINT dwCaravelNetNameID = this->CNetNameText.UpdateText();
	UINT dwCaravelNetPasswordID = this->CNetPasswordText.UpdateText();
	//Provide null messages with empty strings.
	if (!dwCaravelNetNameID) {
		this->CNetNameText = wszEmpty;
		dwCaravelNetNameID = this->CNetNameText.UpdateText();
	}
	if (!dwCaravelNetPasswordID) {
		this->CNetPasswordText = wszEmpty;
		dwCaravelNetPasswordID = this->CNetPasswordText.UpdateText();
	}
	ASSERT(dwNameID);
	ASSERT(dwOrigNameID);
	ASSERT(dwCaravelNetNameID);
	ASSERT(dwCaravelNetPasswordID);

	//Add record.
	c4_RowRef row = g_pTheDB->Players.GetNewRow();
	p_PlayerID(row) = this->dwPlayerID;
	p_IsLocal(row) = this->bIsLocal;
	p_NameMessageID(row) = dwNameID;
	p_CNetNameMessageID(row) = dwCaravelNetNameID;
	p_CNetPasswordMessageID(row) = dwCaravelNetPasswordID;
	p_GID_OriginalNameMessageID(row) = dwOrigNameID;
	p_GID_Created(row) = UINT((time_t)this->Created);
	p_LastUpdated(row) = UINT((time_t)this->LastUpdated);
	p_Settings(row) = SettingsBytes;

	delete[] pbytSettingsBytes;

	return true;
}

//*****************************************************************************
void CDbPlayer::ConvertInputSettings(CDbPackedVars& settings)
// 2.0 changed how key inputs are stored from UINT32 to INT64, so that modifier key flags can also
// be stored for a key
{
	string strKeyboard;
	UINT wKeyboardMode = 0;	//whether to use desktop or laptop keyboard laout
	if (CFiles::GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
	{
		wKeyboardMode = atoi(strKeyboard.c_str());
		if (wKeyboardMode > 1) wKeyboardMode = 0;	//invalid setting
	}

	for (int i = 0; i < InputCommands::DCMD_Count; ++i)
	{
		const KeyDefinition* keyDefinition = GetKeyDefinition(i);

		if (!settings.DoesVarExist(keyDefinition->settingName))
			settings.SetVar(keyDefinition->settingName, keyDefinition->GetDefaultKey(wKeyboardMode));
			continue;

		const UNPACKEDVARTYPE varType = settings.GetVarType(keyDefinition->settingName);
		if (varType == UVT_int) {
			InputKey oldKey = settings.GetVar(keyDefinition->settingName, SDLK_UNKNOWN);

			// Convert old SDL1 mappings
			const bool bInvalidSDL1mapping = oldKey >= 128 && oldKey <= 323;
			if (bInvalidSDL1mapping)
				oldKey = keyDefinition->GetDefaultKey(wKeyboardMode);

			InputKey newType = InputKey(oldKey);
			settings.SetVar(keyDefinition->settingName, newType);
		}

		ASSERT(settings.GetVarType(keyDefinition->settingName) == UVT_long_long_int);
	}
}
