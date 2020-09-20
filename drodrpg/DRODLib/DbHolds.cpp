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

//DbHolds.cpp
//Implementation of CDbHolds and CDbHold.

#include "DbHolds.h"

#include "Db.h"
#include "DbProps.h"
#include "DbXML.h"
#include "GameConstants.h"
#include "MonsterFactory.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Wchar.h>

//#define FULL_NPC_ID_EXPORT //uncomment to export all NPC IDs and locations in an HTML script file

UINT CDbHolds::deletingHoldID = 0;

//***************************************
bool IsCopyableSavedGame(SAVETYPE type)
{
	switch (type) {
//		case ST_RoomBegin:
//		case ST_Checkpoint:
//		case ST_LevelBegin:
		case ST_Continue:
		case ST_EndHold:
//		case ST_WorldMap:
//		case ST_HoldMastered:
			return true;
		default:
			return false;
	}
}

//For compiling the location of scripts that reference variables.
struct VARCOORDELEM {
	UINT roomID;
	CCoordIndex coords;
};
typedef vector<VARCOORDELEM> VARROOMS;
typedef map<WSTRING, VARROOMS> VARCOORDMAP;

//
//CDbHolds public methods.
//

//*****************************************************************************
void CDbHolds::Delete(
//Deletes records for a hold with the given ID.
//Also deletes all levels in the hold (and, by association, all rooms,
//saved games and demos).
//
//Params:
	const UINT dwHoldID)   //(in) ID of hold to delete.
{
	ASSERT(dwHoldID);

	BEGIN_DBREFCOUNT_CHECK;
	{
	c4_View HoldsView;
	const UINT dwHoldRowI = LookupRowByPrimaryKey(dwHoldID, V_Holds, HoldsView);
	if (dwHoldRowI == ROW_NO_MATCH) {ASSERT(!"Bad hold ID."); return;}
	
	//Mark this hold as in process of being deleted (speed optimization).
	ASSERT(!CDbHolds::deletingHoldID);
	CDbHolds::deletingHoldID = dwHoldID;

	c4_RowRef row = HoldsView[dwHoldRowI];

	//Delete all levels in hold (and their rooms, saved games and demos.)
	CDb db;
	CIDSet levelIDs = CDb::getLevelsInHold(dwHoldID);
	CIDSet::const_iterator iter;
	for (iter = levelIDs.begin(); iter != levelIDs.end(); ++iter)
		db.Levels.Delete(*iter);

	//Delete all images (and other hold-specific raw data) in the hold.
	CIDSet IDs = CDb::getDataInHold(dwHoldID);
	for (iter = IDs.begin(); iter != IDs.end(); ++iter)
		db.Data.Delete(*iter);

	//Delete speech data owned by hold character default scripts.
	c4_View CharactersView = p_Characters(row);
	const UINT wNumChars = CharactersView.GetSize();
	for (UINT wCharI=0; wCharI<wNumChars; ++wCharI)
	{
		c4_RowRef row = CharactersView[wCharI];

		CDbPackedVars ExtraVars;
		ExtraVars = p_ExtraVars(row);

		COMMAND_VECTOR commands;
		CCharacter::LoadCommands(ExtraVars, commands);
		for (UINT wIndex=commands.size(); wIndex--; )
		{
			if (commands[wIndex].pSpeech)
				g_pTheDB->Speech.Delete(commands[wIndex].pSpeech->dwSpeechID);
		}
	}

	//Delete name, description, and end hold message texts.
	const UINT dwNameMID = p_NameMessageID(row);
	ASSERTP(dwNameMID, "Bad MID for name.");
	if (dwNameMID)
		DeleteMessage(static_cast<MESSAGE_ID>(dwNameMID));
	const UINT dwDescriptionMID = p_DescriptionMessageID(row);
	ASSERTP(dwDescriptionMID, "Bad MID for description.");
	if (dwDescriptionMID)
		DeleteMessage(static_cast<MESSAGE_ID>(dwDescriptionMID));
	const UINT dwEndHoldMID = p_EndHoldMessageID(row);
	if (dwEndHoldMID)
		DeleteMessage(static_cast<MESSAGE_ID>(dwEndHoldMID));

	c4_View EntrancesView = p_Entrances(row);
	const UINT wEntrances = EntrancesView.GetSize();
	for (UINT wEntranceI=0; wEntranceI<wEntrances; ++wEntranceI)
	{
		c4_RowRef row = EntrancesView[wEntranceI];
		const UINT descriptionMID = p_DescriptionMessageID(row);
		if (descriptionMID)
			DeleteMessage(static_cast<MESSAGE_ID>(descriptionMID));
	}

	//Delete the hold.
	CDb::deleteHold(dwHoldID); //call first
	HoldsView.RemoveAt(dwHoldRowI);
	}
	END_DBREFCOUNT_CHECK;

	//After hold object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
	CDbHolds::deletingHoldID = 0;
}

//*******************************************************************************
bool CDbHolds::Exists(const UINT dwID) const
{
	return CDb::holdExists(dwID);
}

//*****************************************************************************
void CDbHolds::ExportRoomHeader(WSTRING& roomText, CDbLevel *pLevel, CDbRoom *pRoom, ENTRANCE_VECTOR& entrances) const
//Outputs room position in level and level entrance descriptions.
{
	if (!roomText.empty()) return;

	WSTRING endLine = wszPTag;
	endLine += wszCRLF;

	roomText += wszHeader;
	roomText += pLevel->NameText;
	roomText += wszColon;
	roomText += wszSpace;

	pRoom->GetLevelPositionDescription(roomText, true);
	roomText += wszEndHeader;
	roomText += endLine;

	//Entrances.
	WCHAR temp[10];
	for (UINT wIndex=0; wIndex<entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = entrances[wIndex];
		ASSERT(pEntrance);
		_itoW(pEntrance->dwEntranceID,temp,10);
		roomText += temp;
		roomText += wszColon;
		roomText += wszSpace;
		roomText += wszLeftParen;
		_itoW(pEntrance->wX,temp,10);
		roomText += temp;
		roomText += wszComma;
		_itoW(pEntrance->wY,temp,10);
		roomText += temp;
		roomText += wszRightParen;
		/*
		roomText += wszITag;
		roomText += wszSpace;
		roomText += (const WCHAR*)pEntrance->DescriptionText;
		roomText += wszEndITag;
		*/
		roomText += endLine;
	}
}

//*****************************************************************************
WSTRING CDbHolds::ExportSpeech(const UINT dwHoldID, const bool /*bCoords*/) const //[default=true]
//Returns: an HTML-formatted string containing all character speech commands,
//sorted by room and level.
{
	WSTRING allText, holdText;

	if (!dwHoldID) return holdText;

	WSTRING endLine = wszPTag;
	endLine += wszCRLF;

	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID);
	if (!pHold)
		return holdText;

	//Get speech in custom NPC default scripts.
	for (vector<HoldCharacter*>::iterator character = pHold->characters.begin();
			character != pHold->characters.end(); ++character)
	{
		HoldCharacter& c = *(*character);
		COMMAND_VECTOR commands;
		CCharacter::LoadCommands(c.ExtraVars, commands);

		WSTRING roomTextIgnored;
		ENTRANCE_VECTOR entrancesIgnored;
		WSTRING charText = GetScriptSpeechText(commands, pHold, NULL,
				roomTextIgnored, NULL, NULL, entrancesIgnored);
		if (!charText.empty())
		{
			if (holdText.empty())
			{
				holdText += wszHeader;
				holdText += wszBTag;
				holdText += g_pTheDB->GetMessageText(MID_Characters);
				holdText += wszEndBTag;
				holdText += wszEndHeader;
				holdText += endLine;
			}
			//Name custom character.
			holdText += wszBTag;
			holdText += c.charNameText;
			holdText += wszColon;
			holdText += wszEndBTag;
			holdText += endLine;

			holdText += charText;
		}
	}

	CDb db;
	CIDSet levelIDs, roomIDs, scriptIDs;
	UINT wNextFreeScriptID = 0;

	//Get levels.  Sort by local index in hold.
	db.Levels.FilterBy(dwHoldID);
	SORTED_LEVELS levels;
	CDbLevel *pLevel = db.Levels.GetFirst();
	while (pLevel)
	{
		levels.insert(pLevel);
		pLevel = db.Levels.GetNext();
	}

	for (SORTED_LEVELS::const_iterator level = levels.begin(); level != levels.end(); ++level)
	{
		ASSERT((*level)->dwLevelID);
		roomIDs = CDb::getRoomsInLevel((*level)->dwLevelID);
		for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
		{
			WSTRING roomText;
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room);
			ASSERT(pRoom);
			bool bScriptIDRepaired = false;

			//Display level entrance descriptions in this room.
			ENTRANCE_VECTOR entrancesIgnored;

			CMonster *pMonster = pRoom->pFirstMonster;
			while (pMonster)
			{
				if (pMonster->wType == M_CHARACTER)
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					WSTRING charText = GetScriptSpeechText(pCharacter->commands, pHold, pCharacter,
							roomText, *level, pRoom, entrancesIgnored);
					if (!charText.empty())
					{
						roomText += charText;
						roomText += endLine;
					}

					//Debugging: check whether characters script IDs collide.
					if (scriptIDs.contains(pCharacter->dwScriptID))
					{
						ASSERTP(wNextFreeScriptID, "ExportScripts: duplicate ScriptID");
						while (scriptIDs.contains(++wNextFreeScriptID)) ;
						pCharacter->dwScriptID = wNextFreeScriptID;
						bScriptIDRepaired = true;
					}
					VERIFY(!(scriptIDs += pCharacter->dwScriptID).empty());
				}
				pMonster = pMonster->pNext;
			}
			if (bScriptIDRepaired)
				pRoom->Update();

			delete pRoom;

			if (!roomText.empty())
			{
				holdText += roomText;
				holdText += endLine;
			}
		}
		delete *level;
	}

	if (wNextFreeScriptID)
	{
		//Some script IDs were repaired.  Update hold's iterated script ID.
		for (CIDSet::const_iterator script=scriptIDs.begin(); script != scriptIDs.end(); ++script)
			if (*script > wNextFreeScriptID) //find max ID encountered
				wNextFreeScriptID = *script;
		pHold->dwScriptID = wNextFreeScriptID;
		pHold->Update();
	}
	delete pHold;

	if (holdText.empty())
		return holdText; //nothing to return

	//HTML boiler plate.
	allText += wszHtml;
	allText += wszCRLF;
	allText += wszBody;
	allText += wszCRLF;
	allText += holdText;
	allText += wszCRLF;
	allText += wszEndBody;
	allText += wszCRLF;
	allText += wszEndHtml;
	allText += wszCRLF;

	return allText;
}

//*****************************************************************************
WSTRING CDbHolds::GetScriptSpeechText(
//Returns: text of script speech
//
//Params:
	const COMMAND_VECTOR& commands,
	CDbHold *pHold,
	CCharacter *pCharacter, //if not NULL, list char info
	WSTRING& roomText, CDbLevel *pLevel, CDbRoom *pRoom, ENTRANCE_VECTOR& entrancesIgnored)
const
{
	WSTRING charText;
	WCHAR temp[16];

	WSTRING endLine = wszPTag;
	endLine += wszCRLF;

	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		const CCharacterCommand& command = commands[wIndex];

#ifndef FULL_NPC_ID_EXPORT //export ID info for all NPCs when defined
		if (command.command != CCharacterCommand::CC_Speech)
			continue;
		if (!WCSlen((const WCHAR*)(command.pSpeech->MessageText)))
			continue;
#endif

		//Prepend character's room position.
		if (charText.empty() && pCharacter)
		{
			//Prepend level:room location, and entrance descriptions if needed.
			ExportRoomHeader(roomText, pLevel, pRoom, entrancesIgnored);

			charText += wszBTag;
			_itoW(pCharacter->dwScriptID,temp,10);
			charText += temp;
			charText += wszColon;
			charText += wszSpace;
			charText += wszLeftParen;
			_itoW(pCharacter->wX,temp,10);
			charText += temp;
			charText += wszComma;
			_itoW(pCharacter->wY,temp,10);
			charText += temp;
			charText += wszRightParen;
			charText += wszEndBTag;
			charText += wszSpace;
		}

		//Speaker + text.
		if (command.pSpeech)
		{
			static const WCHAR wszFont[] = { We('<'),We('f'),We('o'),We('n'),We('t'),We(' '), We(0) };
			static const WCHAR wszColor[] = { We('c'),We('o'),We('l'),We('o'),We('r'),We('='),We('"'),We('#'), We(0) };
			static const WCHAR wszEndColor[] = { We('"'),We('>'), We(0) };
			static const WCHAR wszEndFont[] = { We('<'),We('/'),We('f'),We('o'),We('n'),We('t'),We('>'), We(0) };
			static const WCHAR wszHTML_GT[] = { We('&'),We('g'),We('t'),We(';'), We(0) };
			static const WCHAR wszHTML_LT[] = { We('&'),We('l'),We('t'),We(';'), We(0) };

			string color;
			WSTRING wstrColor;
			UINT dwSpeakerTextID = 0;

			//Check for a custom speaker.
			UINT wChar = command.pSpeech->wCharacter;
			HoldCharacter *pCustomChar = NULL;
			if (pCharacter)
			{
				const UINT wBaseSpeakerType = getSpeakerType(MONSTERTYPE(pCharacter->GetIdentity()));
				const bool bSelf = wChar == Speaker_Self;
				if (bSelf)
					wChar = pCharacter->wLogicalIdentity; //look up possible custom char
				if (wChar >= CUSTOM_CHARACTER_FIRST && wChar != M_NONE)
				{
					pCustomChar = pHold->GetCharacter(wChar);
					if (pCustomChar)
						wChar = pCustomChar->wType;
					else if (bSelf)
						wChar = wBaseSpeakerType;
				}
				else if (bSelf)
					wChar = wBaseSpeakerType; //show base monster type for "Self" speaker
			}

			dwSpeakerTextID = getSpeakerNameText(wChar, color);
			AsciiToUnicode(color.c_str(), wstrColor);

			//Indicate which lines don't have (and need) a sound bite.
			if (!(command.pSpeech->dwDataID || command.pSpeech->dwDelay == 1))
			{
				charText += wszAsterisk;
				charText += wszSpace;
			}
			charText += wszFont;
			charText += wszColor;
			charText += wstrColor;
			charText += wszEndColor;

			charText += wszITag;
			charText += pCustomChar ? pCustomChar->charNameText.c_str() :
					g_pTheDB->GetMessageText(dwSpeakerTextID);
			charText += wszEndITag;
			charText += wszColon;
			charText += wszSpace;
			//Fix characters that won't display correctly in HTML viewers.
			WSTRING plainText = WCSReplace(
					(WSTRING)(command.pSpeech->MessageText), wszOpenAngle, wszHTML_LT);
			charText += WCSReplace(plainText, wszCloseAngle, wszHTML_GT);
			charText += wszEndFont;
			charText += endLine;
		}
	}

	return charText;
}

//*****************************************************************************
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define STARTVPTAG(vpType,pType) "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define EXPORTTEXT(pType, messageText) messageText.ExportText(str, PropTypeStr(pType))
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">" NEWLINE
#define ENDVPTAG(vpType) "</"; str += ViewpropTypeStr(vpType); str += ">" NEWLINE
#define CLOSETAG "'/>" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))
#define TIMETTOSTR(val) writeTimeT(dummy, sizeof(dummy), (val))

//*****************************************************************************
bool CDbHolds::ExportText(const UINT dwHoldID, CDbRefs &dbRefs, CStretchyBuffer &str)
//Serialize a hold reference, its texts, and the texts of every child object.
{
	if (dbRefs.IsSet(V_Holds,dwHoldID))
		return false;

	dbRefs.Set(V_Holds,dwHoldID);

	CDbHold *pHold = GetByID(dwHoldID);
	ASSERT(pHold);
	if (!pHold)
		return false; //shouldn't happen -- but this is more robust

	char dummy[32];

	//Include corresponding GID player ref.
	g_pTheDB->Players.ExportText(pHold->dwPlayerID, dbRefs, str);

	//GID.
	str += STARTTAG(V_Holds, P_GID_Created);
	str += TIMETTOSTR((time_t)pHold->Created);
	str += PROPTAG(P_GID_PlayerID);
	str += INT32TOSTR(pHold->dwPlayerID);
	str += PROPTAG(P_LastUpdated);
	str += TIMETTOSTR((time_t)pHold->LastUpdated);  //timestamp provides hold "version" info
	str += PROPTAG(P_HoldID); //Put primary key last.
	str += INT32TOSTR(pHold->dwHoldID);
	str += CLOSESTARTTAG;

	//Texts.
	EXPORTTEXT(P_NameMessage, pHold->NameText);
	EXPORTTEXT(P_DescriptionMessage, pHold->DescriptionText);
	EXPORTTEXT(P_EndHoldMessage, pHold->EndHoldText);

	//Get levels.  Sort by local index in hold.
	CDb db;
	db.Levels.FilterBy(dwHoldID);
	SORTED_LEVELS levels;
	for (CDbLevel *pLevel = db.Levels.GetFirst(); pLevel != NULL;
			pLevel = db.Levels.GetNext())
		levels.insert(pLevel);

	//Export level entrance descriptions in order of levels.
	SORTED_LEVELS::const_iterator level;
	for (level = levels.begin(); level != levels.end(); ++level)
	{
		const UINT dwLevelID = (*level)->dwLevelID;
		ASSERT(dwLevelID);
		CIDSet roomIDs = CDb::getRoomsInLevel(dwLevelID);

		ENTRANCE_VECTOR entrances;
		for (UINT wIndex=pHold->Entrances.size(); wIndex--; )
		{
			CEntranceData *pEntrance = pHold->Entrances[wIndex];
			if (roomIDs.has(pEntrance->dwRoomID))
			{
				//GID.
				str += STARTVPTAG(VP_Entrances, P_EntranceID);
				str += INT32TOSTR(pEntrance->dwEntranceID);
				str += CLOSESTARTTAG;

				//Texts.
				EXPORTTEXT(P_DescriptionMessage, pEntrance->DescriptionText);

				str += ENDVPTAG(VP_Entrances);
			}
		}
	}

	//Export hold levels in order.
	for (level = levels.begin(); level != levels.end(); ++level)
	{
		const UINT dwLevelID = (*level)->dwLevelID;
		db.Levels.ExportText(dwLevelID, dbRefs, str);

		//Export room texts.
		CIDSet roomIDs = CDb::getRoomsInLevel(dwLevelID);
		for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
			db.Rooms.ExportText(*room, dbRefs, str);

		delete *level;
	}

	delete pHold;

	str += ENDTAG(V_Holds);

	return true;
}

//*****************************************************************************
void CDbHolds::ExportXML(
//Returns: string containing XML text describing hold with this ID
//          AND all levels having this HoldID
//          AND all show demos in this hold
//
//Params:
	const UINT dwHoldID,   //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)        //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_Holds,dwHoldID))
		return;

	dbRefs.Set(V_Holds,dwHoldID);

	CDbHold *pHold = GetByID(dwHoldID);
	ASSERT(pHold);
	if (!pHold)
		return; //shouldn't happen -- but this is more robust

	char dummy[32];

	//Include corresponding GID player ref.
	g_pTheDB->Players.ExportXML(pHold->dwPlayerID, dbRefs, str, true);

	str += STARTTAG(V_Holds, P_GID_Created);
	str += TIMETTOSTR((time_t)pHold->Created);
	str += PROPTAG(P_GID_PlayerID);
	str += INT32TOSTR(pHold->dwPlayerID);
	str += PROPTAG(P_LastUpdated);
	str += TIMETTOSTR((time_t)pHold->LastUpdated);  //timestamp provides hold "version" info
	str += PROPTAG(P_Status);
	str += INT32TOSTR(pHold->status);
	if (!bRef)
	{
		//Don't need any further information for a hold reference.

		//Prepare data.
		WSTRING const wNameStr = (WSTRING)pHold->NameText;
		WSTRING const wDescStr = (WSTRING)pHold->DescriptionText;
		WSTRING const wEndHoldStr = (WSTRING)pHold->EndHoldText;

		str += PROPTAG(P_NameMessage);
		str += Base64::encode(wNameStr);
		str += PROPTAG(P_DescriptionMessage);
		str += Base64::encode(wDescStr);
		str += PROPTAG(P_LevelID);
		str += INT32TOSTR(pHold->dwLevelID);
		str += PROPTAG(P_GID_NewLevelIndex);
		//if there is a level in the hold, the newLevelIndex must be > 0
		ASSERT(!pHold->dwLevelID || pHold->dwNewLevelIndex);
		str += INT32TOSTR(pHold->dwNewLevelIndex);
		str += PROPTAG(P_EditingPrivileges);
		str += INT32TOSTR(pHold->editingPrivileges);
		str += PROPTAG(P_EndHoldMessage);
		str += Base64::encode(wEndHoldStr);
		str += PROPTAG(P_ScriptID);
		str += INT32TOSTR(pHold->dwScriptID);
		str += PROPTAG(P_VarID);
		str += INT32TOSTR(pHold->dwVarID);
		str += PROPTAG(P_CharID);
		str += INT32TOSTR(pHold->dwCharID);
		//bCaravelNetMedia should be ignored
	}

	//Put primary key last, so all message fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_HoldID);
	str += INT32TOSTR(pHold->dwHoldID);

	if (bRef)
		str += CLOSETAG;
	else
	{
		str += CLOSESTARTTAG;

		//Entrances
		CEntranceData *pEntrance;
		UINT dwSize = pHold->Entrances.size();
		UINT dwIndex;
		c4_View RoomsView;
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			pEntrance = pHold->Entrances[dwIndex];
			const UINT dwRoomI = LookupRowByPrimaryKey(pEntrance->dwRoomID, V_Rooms, RoomsView);
			if (dwRoomI == ROW_NO_MATCH) continue; //corrupted entrance data -- don't export it
	      WSTRING const wMessage = (WSTRING)pEntrance->DescriptionText;

			str += STARTVPTAG(VP_Entrances, P_EntranceID);
			str += INT32TOSTR(pEntrance->dwEntranceID);
			str += PROPTAG(P_DescriptionMessage);
			str += Base64::encode(wMessage);
			str += PROPTAG(P_RoomID);
			str += INT32TOSTR(pEntrance->dwRoomID);
			str += PROPTAG(P_X);
			str += INT32TOSTR(pEntrance->wX);
			str += PROPTAG(P_Y);
			str += INT32TOSTR(pEntrance->wY);
			str += PROPTAG(P_O);
			str += INT32TOSTR(pEntrance->wO);
			str += PROPTAG(P_IsMainEntrance);
			str += INT32TOSTR(pEntrance->bIsMainEntrance);
			str += PROPTAG(P_ShowDescription);
			str += INT32TOSTR(pEntrance->eShowDescription);
	      str += CLOSETAG;
		}

		//Hold vars.
		dwSize = pHold->vars.size();
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			HoldVar& var = pHold->vars[dwIndex];
			str += STARTVPTAG(VP_Vars, P_VarID);
			str += INT32TOSTR(var.dwVarID);
			str += PROPTAG(P_VarNameText);
			str += Base64::encode(var.varNameText);
			str += CLOSETAG;
		}

		//Hold characters.
		dwSize = pHold->characters.size();
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			HoldCharacter& ch = *(pHold->characters[dwIndex]);
			if (ch.dwDataID_Avatar)
				g_pTheDB->Data.ExportXML(ch.dwDataID_Avatar, dbRefs, str);
			if (ch.dwDataID_Tiles)
				g_pTheDB->Data.ExportXML(ch.dwDataID_Tiles, dbRefs, str);
			COMMAND_VECTOR commands;
			CCharacter::LoadCommands(ch.ExtraVars, commands);
			str += CCharacter::ExportXMLSpeech(dbRefs, commands);

			str += STARTVPTAG(VP_Characters, P_CharID);
			str += INT32TOSTR(ch.dwCharID);
			str += PROPTAG(P_CharNameText);
			str += Base64::encode(ch.charNameText);
			str += PROPTAG(P_Type);
			str += INT32TOSTR(ch.wType);
			str += PROPTAG(P_AnimationSpeed);
			str += INT32TOSTR(ch.animationSpeed);

			UINT dwBufferSize;
			BYTE *pExtraVars = ch.ExtraVars.GetPackedBuffer(dwBufferSize);
			if (dwBufferSize > 4)   //null buffer
			{
				str += PROPTAG(P_ExtraVars);
				str += Base64::encode(pExtraVars,dwBufferSize-4);  //strip null UINT
			}
			delete[] pExtraVars;

			//Only save attached data if it's not a dangling pointer.
			if (g_pTheDB->Data.Exists(ch.dwDataID_Avatar))
			{
				str += PROPTAG(P_DataID);
				str += INT32TOSTR(ch.dwDataID_Avatar);
			}
			if (g_pTheDB->Data.Exists(ch.dwDataID_Tiles))
			{
				str += PROPTAG(P_DataIDTiles);
				str += INT32TOSTR(ch.dwDataID_Tiles);
			}
			str += CLOSETAG;
		}

		str += ENDTAG(V_Holds);

		CIDSet LevelIDs = CDb::getLevelsInHold(dwHoldID);
		CIDSet DataIDs = CDb::getDataInHold(dwHoldID);

		CIDSet::const_iterator iter;
		static const float fBasePercentDone = 0.01f;
		static const float fTotalRemainingPercent = 1.0f - fBasePercentDone;
		const float fItems = (float)(LevelIDs.size() + DataIDs.size() + 1); //+1 is for demos
		UINT wCount=0;
		CDb db;

		//Export all embedded data objects in hold.
		for (iter = DataIDs.begin(); iter != DataIDs.end(); ++iter, ++wCount)
		{
			CDbXML::PerformCallbackf(fBasePercentDone + (wCount/fItems) * fTotalRemainingPercent);
			db.Data.ExportXML(*iter, dbRefs, str);
		}
		  
		//Export all levels in hold.
		for (iter = LevelIDs.begin(); iter != LevelIDs.end(); ++iter, ++wCount)
		{
			CDbXML::PerformCallbackf(fBasePercentDone + (wCount/fItems) * fTotalRemainingPercent);
			db.Levels.ExportXML(*iter, dbRefs, str);
		}

		//Export all display demos in hold.
		CDbXML::PerformCallbackf(fBasePercentDone + (wCount/fItems) * fTotalRemainingPercent);
		db.Demos.FilterByShow();
		const CIDSet DemoIDs = db.Demos.GetIDs();
		for (iter = DemoIDs.begin(); iter != DemoIDs.end(); ++iter)
		{
			const UINT dwDemoHoldID = CDb::getHoldOfDemo(*iter);
			if (dwDemoHoldID == dwHoldID)
				db.Demos.ExportXML(*iter, dbRefs, str);
		}

		CDbXML::PerformCallbackf(1.0f);
	}

	delete pHold;
}
#undef STARTTAG
#undef STARTVPTAG
#undef EXPORTTEXT
#undef PROPTAG
#undef ENDTAG
#undef ENDVPTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR
#undef TIMETTOSTR

//*****************************************************************************
bool CDbHolds::EditableHoldExists() const
//Returns: whether there exists a hold the current player can edit.
{
	//Get holds in DB.
	CDb db;
	const CIDSet holdIDs = db.Holds.GetIDs();
	for (CIDSet::const_iterator iter = holdIDs.begin(); iter != holdIDs.end(); ++iter)
		if (g_pTheDB->Holds.PlayerCanEditHold(*iter))
			return true;

	return false;
}

//*****************************************************************************
UINT CDbHolds::GetAuthorID(const UINT dwHoldID)
//Returns: the player ID who is the author of the specified hold, or 0 if hold doesn't exist.
{
	if (!dwHoldID) return 0;

	c4_View HoldsView;
	const UINT dwHoldRowI = LookupRowByPrimaryKey(dwHoldID, V_Holds, HoldsView);
	if (dwHoldRowI == ROW_NO_MATCH) return 0;

	return (UINT) p_GID_PlayerID(HoldsView[dwHoldRowI]);
}

//*****************************************************************************
void CDbHolds::GetEntranceIDsForRoom(
//Returns a set of IDs of entrances in the specified room.
//
//Params:
	const UINT dwRoomID,   //(in)
	CIDSet& entranceIDs)    //(in/out)
const
{
	ASSERT(dwRoomID);

	CDb db;
	const UINT dwHoldID = db.Rooms.GetHoldIDForRoom(dwRoomID);
	if (!dwHoldID) return;

	CDbHold *pHold = db.Holds.GetByID(dwHoldID);
	ASSERT(pHold);
	for (UINT wIndex=pHold->Entrances.size(); wIndex--; )
	{
		CEntranceData *pEntrance = pHold->Entrances[wIndex];
		if (dwRoomID == pEntrance->dwRoomID)
			entranceIDs += pEntrance->dwEntranceID;
	}
	delete pHold;
}

//*****************************************************************************
void CDbHolds::GetEntrancesForRoom(
//OUT: returns copy of hold level entrances in this room.
//Entrance records must be deleted by caller.
//
//Params:
	const UINT dwRoomID,   //(in)
	ENTRANCE_VECTOR& entrances)    //(in/out)
const
{
	ASSERT(dwRoomID);

	CDb db;
	const UINT dwHoldID = db.Rooms.GetHoldIDForRoom(dwRoomID);
	if (!dwHoldID) return;

	CDbHold *pHold = db.Holds.GetByID(dwHoldID);
	ASSERT(pHold);
	for (UINT wIndex=pHold->Entrances.size(); wIndex--; )
	{
		CEntranceData *pEntrance = pHold->Entrances[wIndex];
		if (dwRoomID == pEntrance->dwRoomID)
		{
			CEntranceData *pEntranceCopy = new CEntranceData();
			pEntranceCopy->SetMembers(*pEntrance, false); //replicates data in DB
			ASSERT(pEntranceCopy);
			entrances.push_back(pEntranceCopy);
		}
	}
	delete pHold;
}

//*****************************************************************************
UINT CDbHolds::GetLevelIDAtIndex(
//Find level at GID index in this hold
//
//Returns:
//LevelID of found level, or 0 if no match.
//
//Params:
	const UINT dwIndex, const UINT dwHoldID)  //(in) level index for hold
{
	ASSERT(IsOpen());

	CIDSet levelsInHold = CDb::getLevelsInHold(dwHoldID);

	//Scan through all levels in the hold to find a match.
	c4_View LevelsView;
	for (CIDSet::const_iterator level = levelsInHold.begin();
			level != levelsInHold.end(); ++level)
	{
		const UINT levelRowI = LookupRowByPrimaryKey(*level, V_Levels, LevelsView);
		c4_RowRef row = LevelsView[levelRowI];
		ASSERT(UINT(p_HoldID(row)) == dwHoldID);
		const UINT dwLevelIndex = (UINT) p_GID_LevelIndex(row);
		if (dwLevelIndex == dwIndex)
			return (UINT) p_LevelID(row); //Found it.
	}
	return 0; //Didn't find it.
}

//*****************************************************************************
UINT CDbHolds::GetLevelIDAtOrderIndex(
//Find level at specified order index in this hold
//
//Returns:
//LevelID of found level, or 0 if no match.
//
//Params:
	const UINT dwIndex, const UINT dwHoldID)  //(in) level index for hold
{
	ASSERT(IsOpen());

	CIDSet levelsInHold = CDb::getLevelsInHold(dwHoldID);

	//Scan through all levels in the hold to find a match.
	c4_View LevelsView;
	for (CIDSet::const_iterator level = levelsInHold.begin();
			level != levelsInHold.end(); ++level)
	{
		const UINT levelRowI = LookupRowByPrimaryKey(*level, V_Levels, LevelsView);
		c4_RowRef row = LevelsView[levelRowI];
		ASSERT(UINT(p_HoldID(row)) == dwHoldID);
		const UINT dwLevelIndex = (UINT) p_OrderIndex(row);
		if (dwLevelIndex == dwIndex)
			return (UINT) p_LevelID(row); //Found it.
	}
	return 0; //Didn't find it.
}

//*****************************************************************************
UINT CDbHolds::GetHoldID(
//Compares arguments against hold records in the DB.
//
//Returns: hold ID if a hold record in the DB matches these parameters, else 0
	const CDate& Created, CDbMessageText& HoldNameText, CDbMessageText& origAuthorText)
{
	ASSERT(IsOpen());
	const UINT dwHoldCount = g_pTheDB->Holds.GetViewSize();

	//Each iteration checks a hold's GIDs.
	for (UINT dwHoldI = 0; dwHoldI < dwHoldCount; ++dwHoldI)
	{
		//Check time of creation.
		c4_RowRef row = GetRowRef(V_Holds, dwHoldI);
		const time_t HoldCreated = (time_t)p_GID_Created(row);
		if (HoldCreated != Created) continue;

		//Check name.
		CDbMessageText NameText;
		NameText.Bind((UINT) p_NameMessageID(row));
		if (!(NameText == HoldNameText)) continue;

		//Check author.
		const UINT dwPlayerID = (UINT)p_GID_PlayerID(row);
		ASSERT(dwPlayerID);
		CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwPlayerID);
		ASSERT(pPlayer);
		if (pPlayer && pPlayer->OriginalNameText == origAuthorText)
		{
			//GUIDs match.  Return this record's local ID.
			delete pPlayer;
			return (UINT) p_HoldID(row);
		}
		delete pPlayer;
	}

	//No match.
	return 0L;
}

//*****************************************************************************
WSTRING CDbHolds::GetHoldName(const UINT holdID) const
//Returns: the name of the hold with specified ID
{
	WSTRING name;
	CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID, true);
	if (pHold)
	{
		name = (const WCHAR *)pHold->NameText;
		delete pHold;
	}
	return name;
}

//*****************************************************************************
void CDbHolds::GetRooms(const UINT dwHoldID, HoldStats& stats) const
//OUT: hold room lists
{
	ASSERT(dwHoldID);

	stats.rooms.clear();
//	stats.requiredRooms.clear();
	stats.secretRooms.clear();

	//Get all rooms in hold.
	CIDSet roomsInHold = CDb::getRoomsInHold(dwHoldID);
	for (CIDSet::const_iterator room = roomsInHold.begin(); room != roomsInHold.end(); ++room)
	{
		//Tally stats for room.
		stats.rooms += *room;
//		if (CDbRoom::IsRequired(*room))
//			stats.requiredRooms += *room;
		if (CDbRoom::IsSecret(*room))
			stats.secretRooms += *room;
	}
}

//*****************************************************************************
void CDbHolds::GetRoomsExplored(
//OUT: set of room IDs explored by player in specified hold
//
//Params:
	const UINT dwHoldID, const UINT dwPlayerID,   //(in)
	CIDSet& rooms) //(out)
{
	ASSERT(dwHoldID);
	ASSERT(dwPlayerID);

	rooms.clear();

	//Get all player's saved games in hold.
	CDb db;
	db.SavedGames.FilterByPlayer(dwPlayerID);
	db.SavedGames.FilterByHold(dwHoldID);
	db.SavedGames.FindHiddens(true);
	const CIDSet savedGameIDs = db.SavedGames.GetIDs();

	CDbSavedGame *pSavedGame;
	for (CIDSet::const_iterator iter = savedGameIDs.begin(); iter != savedGameIDs.end(); ++iter)
	{
		pSavedGame = db.SavedGames.GetByID(*iter);
		ASSERT(pSavedGame);
		if (pSavedGame->eType != ST_DemoUpload) //not a real saved game record
		{
			rooms += pSavedGame->GetExploredRooms();
			rooms += pSavedGame->dwRoomID;
		}
		delete pSavedGame;
	}

	//Total rooms explored/conquered for this hold.
	pSavedGame = db.SavedGames.GetByID(
			db.SavedGames.FindByType(ST_PlayerTotal, dwPlayerID, false));
	if (pSavedGame)
	{
		const CIDSet roomsInHold = CDb::getRoomsInHold(dwHoldID);
/*
		if (bOnlyConquered)
		{
			pSavedGame->ConqueredRooms.intersect(roomsInHold);
			rooms += pSavedGame->ConqueredRooms;
		} else {
*/
		CIDSet expRooms = pSavedGame->GetExploredRooms(true); //this save flags "map only" on all explored rooms
		expRooms.intersect(roomsInHold);
		rooms += expRooms;
//		}

		delete pSavedGame;
	}
}

//*****************************************************************************
/*
void CDbHolds::GetRoomsExplored(
//OUT: set of room IDs explored and conquered by player in specified hold
//
//Params:
	const UINT dwHoldID, const UINT dwPlayerID,   //(in)
	CIDSet& exploredRooms, CIDSet& conqueredRooms) //(out)
{
	ASSERT(dwHoldID);
	ASSERT(dwPlayerID);

	exploredRooms.clear();
	conqueredRooms.clear();

	//Get all player's saved games in hold.
	CDb db;
	CIDSet savedGameIDs;
	db.SavedGames.FilterByPlayer(dwPlayerID);
	db.SavedGames.FilterByHold(dwHoldID);
	db.SavedGames.FindHiddens(true);
	db.SavedGames.GetIDs(savedGameIDs);

	CDbSavedGame *pSavedGame;
	for (CIDSet::const_iterator iter = savedGameIDs.begin(); iter != savedGameIDs.end(); ++iter)
	{
		pSavedGame = db.SavedGames.GetByID(*iter);
		ASSERT(pSavedGame);
		if (pSavedGame->eType != ST_DemoUpload) //not a real saved game record
		{
			conqueredRooms += pSavedGame->ConqueredRooms;
			exploredRooms += pSavedGame->ExploredRooms;
		}
		delete pSavedGame;
	}

	//Total rooms explored/conquered for this hold.
	pSavedGame = db.SavedGames.GetByID(
			db.SavedGames.FindByType(ST_PlayerTotal, dwPlayerID, false));
	if (pSavedGame)
	{
		CIDSet roomsInHold = CDb::getRoomsInHold(dwHoldID);
		pSavedGame->ConqueredRooms.intersect(roomsInHold);
		conqueredRooms += pSavedGame->ConqueredRooms;

		pSavedGame->ExploredRooms.intersect(roomsInHold);
		exploredRooms += pSavedGame->ExploredRooms;

		delete pSavedGame;
	}
}
*/

//*****************************************************************************
UINT CDbHolds::GetSecretsDone(
//Returns the number of secret rooms in the specified hold that the indicated
//player has explored/conquered.
//
//Params:
	HoldStats& stats, //(out)
	const UINT dwHoldID, const UINT dwPlayerID)   //(in)
//	const bool bConqueredOnly) //[default=true]
const
{
	CIDSet roomsExplored;
	GetRooms(dwHoldID, stats);
	GetRoomsExplored(dwHoldID, dwPlayerID, roomsExplored);

	UINT wCount=0;
	for (CIDSet::const_iterator iter = stats.secretRooms.begin();
			iter != stats.secretRooms.end(); ++iter)
		if (roomsExplored.has(*iter))
			++wCount;
	return wCount;
}

//*****************************************************************************
UINT CDbHolds::GetHoldIDWithStatus(const CDbHold::HoldStatus status)
//Returns: hold ID of first hold marked with stated status
{
	ASSERT(IsOpen());
	const UINT dwHoldCount = g_pTheDB->Holds.GetViewSize();

	//Each iteration checks a hold.
	for (UINT dwHoldI = 0; dwHoldI < dwHoldCount; ++dwHoldI)
	{
		c4_RowRef row = GetRowRef(V_Holds, dwHoldI);
		if ((CDbHold::HoldStatus)(UINT)(p_Status(row)) == status)
			return (UINT)p_HoldID(row);
	}

	//Not found.
	return 0;
}

//*****************************************************************************
bool CDbHolds::IsHoldMastered(const UINT dwHoldID, const UINT playerID) const
//Returns: true if specified player has ended the hold, plus conquered all the secrets.
{
	const UINT dwEndHoldID = g_pTheDB->SavedGames.FindByEndHold(dwHoldID);
	if (!dwEndHoldID)
		return false;

	HoldStats stats;
	const UINT wSecretsConquered = GetSecretsDone(stats, dwHoldID,	playerID); //, true);
	return wSecretsConquered == stats.secretRooms.size();
}

//*****************************************************************************
void AddScriptVarRef(
//Adds the location of this variable reference to the varmap.
//
//Params:
	VARCOORDMAP& varMap, const WCHAR* varName, const UINT roomID,
	const UINT roomCols, const UINT roomRows, const UINT x, const UINT y)
{
	if (!varName)
		varName = wszEmpty; //unrecognized variable reference
	WSTRING wstrVarName(varName);

	VARCOORDMAP::iterator var = varMap.find(wstrVarName);
	if (var == varMap.end())
	{
		//First instance of var.
		varMap[wstrVarName] = VARROOMS();
		var = varMap.find(wstrVarName);
		ASSERT(var != varMap.end());
	}

	VARROOMS& rooms = var->second;
	UINT i;
	for (i=0; i<rooms.size(); ++i)
		if (rooms[i].roomID == roomID)
			break;
	if (i == rooms.size())
	{
		//First instance of var in this room.
		VARCOORDELEM elem;
		elem.roomID = roomID;
		rooms.push_back(elem);
		rooms.back().coords.Init(roomCols, roomRows);
	}

	//Tally the times this var is referenced in this script.
	VARCOORDELEM& elem = rooms[i];
	ASSERT(elem.roomID == roomID);
	elem.coords.Add(x,y, elem.coords.GetAt(x,y)+1);
}

//*****************************************************************************
void CDbHolds::LogScriptVarRefs(const UINT holdID)
//Output a log of all vars used in the hold.
{
	ASSERT(IsOpen());

	CDbHold *pHold = GetByID(holdID);
	if (!pHold)
		return;

	//Order levels in hold.
	SORTED_LEVELS levels;
	CIDSet levelsInHold = CDb::getLevelsInHold(holdID);
	for (CIDSet::const_iterator levelID = levelsInHold.begin(); levelID != levelsInHold.end(); ++levelID)
	{
		CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*levelID);
		ASSERT(pLevel);
		levels.insert(pLevel);
	}

	VARCOORDMAP varMap;
	for (SORTED_LEVELS::const_iterator level = levels.begin(); level != levels.end(); ++level)
	{
		CDbLevel *pLevel = *level;
		CIDSet roomIDs = CDb::getRoomsInLevel(pLevel->dwLevelID);
		for (CIDSet::const_iterator id=roomIDs.begin(); id!=roomIDs.end(); ++id)
		{
			//Scan a room for var references.
			const UINT roomID = *id;
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(roomID);
			ASSERT(pRoom);

			for (CMonster *pMonster = pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
			{
				if (pMonster->wType != M_CHARACTER)
					continue;

				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				for (UINT i=0; i<pCharacter->commands.size(); ++i)
				{
					const CCharacterCommand& c = pCharacter->commands[i];
					switch (c.command)
					{
						case CCharacterCommand::CC_VarSet:
						case CCharacterCommand::CC_WaitForVar:
						{
							WSTRING varName;
							if (c.x >= (UINT)ScriptVars::FirstPredefinedVar)
							{
								varName = ScriptVars::getVarNameW(ScriptVars::Predefined(c.x));
							} else {
								varName = pHold->GetVarName(c.x);
							}
							AddScriptVarRef(varMap, varName.c_str(), roomID, pRoom->wRoomCols,
									pRoom->wRoomRows, pCharacter->wX, pCharacter->wY);

							if (c.command == CCharacterCommand::CC_VarSet)
							{
								switch (c.y)
								{
									case ScriptVars::Assign:
									case ScriptVars::Inc:
									case ScriptVars::Dec:
									case ScriptVars::MultiplyBy:
									case ScriptVars::DivideBy:
									case ScriptVars::Mod:
										//Search for a variable name in the operand.
										if (!c.label.empty())
										{
											//parse expression
											ScriptVars::Predefined varID = ScriptVars::parsePredefinedVar(c.label);
											if (varID != ScriptVars::P_NoVar || pHold->GetVarID(c.label.c_str()))
											{
												//Mark reference to variable.
												AddScriptVarRef(varMap, c.label.c_str(), roomID, pRoom->wRoomCols,
														pRoom->wRoomRows, pCharacter->wX, pCharacter->wY);
											}
										}
									break;
									default: break;
								}
							} else {
								switch (c.y)
								{
									case ScriptVars::Equals:
									case ScriptVars::Greater:
									case ScriptVars::Less:
										//Search for a variable name in the operand.
										if (!c.label.empty())
										{
											//parse expression
											ScriptVars::Predefined varID = ScriptVars::parsePredefinedVar(c.label);
											if (varID != ScriptVars::P_NoVar || pHold->GetVarID(c.label.c_str()))
											{
												//Mark reference to variable.
												AddScriptVarRef(varMap, c.label.c_str(), roomID, pRoom->wRoomCols,
														pRoom->wRoomRows, pCharacter->wX, pCharacter->wY);
											}
										}
									break;
									default: break;
								}
							}
						}
						break;
						default: break;
					}
				}
			}
			delete pRoom;
		}
		delete pLevel;
	}
	delete pHold;

	WSTRING text;
	for (VARCOORDMAP::const_iterator vars = varMap.begin(); vars != varMap.end(); ++vars)
	{
		//Display info for this variable.
		text += vars->first.empty() ? wszQuestionMark : vars->first.c_str();
		text += wszColon;
		text += wszCRLF;

		//Print location of all references to this variable.
		const VARROOMS& rooms = vars->second;
		for (UINT i=0; i<rooms.size(); ++i)
		{
			const VARCOORDELEM& room = rooms[i];
			const UINT roomID = room.roomID;
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(roomID, true);
			ASSERT(pRoom);
			CDbLevel *pLevel = g_pTheDB->Levels.GetByID(pRoom->dwLevelID);
			ASSERT(pLevel);

			//Level name.
			text += wszSpace;
			text += pLevel->NameText;
			text += wszColon;
			text += wszSpace;
			delete pLevel;

			//Room name.
			pRoom->GetLevelPositionDescription(text, true);
			text += wszCRLF;
			delete pRoom;

			//All positions in room where var is referenced.
			for (UINT y=0; y<room.coords.GetRows(); ++y)
				for (UINT x=0; x<room.coords.GetCols(); ++x)
				{
					const UINT num = room.coords.GetAt(x,y);
					if (num)
					{
						WCHAR temp[10];
						text += wszSpace;
						text += wszSpace;
						text += _itoW(x, temp, 10);
						text += wszComma;
						text += _itoW(y, temp, 10);
						if (num > 1)
						{
							text += wszSpace;
							text += wszLeftParen;
							text += _itoW(num, temp, 10);
							text += wszRightParen;
						}
						text += wszCRLF;
					}
				}
		}
		text += wszCRLF;
	}

	CClipboard::SetString(text);
}

//*****************************************************************************
bool CDbHolds::PlayerCanEditHold(
//Returns: whether current player can view and edit the given hold
//(True if player is the author of the hold, or has completed it, or
//hold has write privileges set for everyone.)
//
//Params:
	const UINT dwHoldID)   //(in)
const
{
	if (!dwHoldID) return false;

	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
	ASSERT(pHold);
	if (!pHold)
		return false;
	const bool bRes = pHold->PlayerCanEdit(g_pTheDB->GetPlayerID());
	delete pHold;
	return bRes;
}

//
//CDbHold protected methods.
//

//*****************************************************************************
CDbHold::CDbHold()
//Constructor.
{
	Clear();
}

//*****************************************************************************
void CDbHold::CopyHoldMedia(CDbHold *pNewHold, CImportInfo& info)
//Makes a copy of all Data records marked as owned by this hold and accessed
//in some record owned by the hold, and gives ownership of the copies to the new hold.
{
	//Copy custom character data objects.
	for (vector<HoldCharacter*>::iterator chIter=pNewHold->characters.begin();
			chIter!=pNewHold->characters.end(); ++chIter)
	{
		CopyCustomCharacterData(*(*chIter), pNewHold, info);
	}
}

//*****************************************************************************
void CDbHold::CopyCustomCharacterData(HoldCharacter& ch, CDbHold *pNewHold, CImportInfo& info) const
{
	const UINT newHoldID = pNewHold->dwHoldID;

	CDbData::CopyObject(info, ch.dwDataID_Avatar, newHoldID);
	CDbData::CopyObject(info, ch.dwDataID_Tiles, newHoldID);

	COMMAND_VECTOR commands;
	CCharacter::LoadCommands(ch.ExtraVars, commands);
	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		const CCharacterCommand& c = commands[wIndex];
		if (c.pSpeech) {
			c.pSpeech->dwSpeechID = 0; //make copy on save
			CDbData::CopyObject(info, c.pSpeech->dwDataID, newHoldID, false);
			c.pSpeech->MessageText.SaveNewInstance();
		}
	}
	CCharacter::ChangeHoldForCommands(commands, this, pNewHold, info, false);

	CCharacter::SaveCommands(ch.ExtraVars, commands);
}

//
//CDbHold public methods.
//

//*****************************************************************************
CDbHold::~CDbHold()
//Destructor.
{
	Clear();
}

//*****************************************************************************
UINT CDbHold::AddCharacter(const WCHAR* pwszName)
//Adds custom character with given name.  Name must be unique.
//
//Returns: a unique char ID, or 0 if this character name already exists.
//The caller must remember to Update() the hold if this character gets used.
{
	if (GetCharacterID(pwszName))
		return 0; //character with this name already exists -- don't add again

	const UINT dwNewCharID = GetNewCharacterID();
	const UINT dwNewScriptID = GetNewScriptID();
	this->characters.push_back(new HoldCharacter(dwNewCharID, pwszName));
	return dwNewCharID;
}

//*****************************************************************************
UINT CDbHold::AddVar(const WCHAR* pwszName)
//Adds a new unique hold variable name-id pair.
//Returns: a unique var ID, or 0 if this variable name already exists.
//The caller must remember to Update() the hold if this var gets used.
{
	if (GetVarID(pwszName))
		return 0; //Found matching name -- don't add it again.

	const UINT dwNewVarID = ++this->dwVarID;
	this->vars.push_back(HoldVar(dwNewVarID, pwszName));
	return dwNewVarID;
}

//*****************************************************************************
bool CDbHold::DeleteCharacter(const UINT dwCharID)
//Removes character with specified ID from the hold.
//
//Returns: true if deleted, else false if charID not found
{
	for (vector<HoldCharacter*>::iterator character = this->characters.begin();
			character != this->characters.end(); ++character)
	{
		HoldCharacter* pChar = *character;
		if (pChar->dwCharID == dwCharID)
		{
			//Mark speech objects owned by this character's default script for deletion.
			HoldCharacter& ch = *pChar;
			COMMAND_VECTOR commands;
			CCharacter::LoadCommands(ch.ExtraVars, commands);
			for (UINT wIndex=commands.size(); wIndex--; )
			{
				if (commands[wIndex].pSpeech)
					MarkSpeechForDeletion(commands[wIndex].pSpeech);
			}

			delete pChar;
			this->characters.erase(character);
			return true;
		}
	}
	return false;
}

//*****************************************************************************
bool CDbHold::DeleteVar(const UINT dwVarID)
//Removes hold var with specified ID from the hold.
//
//Returns: true if deleted, else false if varID not found
{
	for (vector<HoldVar>::iterator var = this->vars.begin();
			var != this->vars.end(); ++var)
		if (var->dwVarID == dwVarID)
		{
			this->vars.erase(var);
			return true;
		}
	return false;
}

//*****************************************************************************
bool CDbHold::IsVarNameGoodSyntax(const WCHAR* pName)
//Returns: whether a var name has correct syntax
{
	if (!pName)
		return false;
	if (!iswalpha(*(pName++))) //first char must be a letter
		return false;
	while (WCv(*pName))
	{
		//No punctuation except underscore and space
		if (!(iswalnum(*pName) || *pName == W_t(' ') || *pName == W_t('_')))
			return false;
		++pName;
	}
	return true;
}

//*****************************************************************************
bool CDbHold::ChangeAuthor(
//Change hold author.  Revise hold name and description to reflect this.
//
//Returns: whether author was changed
//
//Params:
	const UINT dwNewAuthorID)   //(in) new hold author-player
{
	ASSERT(dwNewAuthorID);
	if (this->dwPlayerID == dwNewAuthorID)
		return false;  //this player is already the hold author

	CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwNewAuthorID);
	ASSERT(pPlayer);
	WSTRING holdName = (WSTRING)pPlayer->NameText;
	delete pPlayer;

	static const WCHAR wszApostrophe[] = { We('\''),We('s'),We(' '),We(0) };
	holdName += wszApostrophe;
	holdName += (WSTRING)this->NameText;
	this->NameText = holdName.c_str();

	WSTRING holdDesc = (WSTRING)this->DescriptionText;
	CDbPlayer *pOrigHoldAuthor = g_pTheDB->Players.GetByID(this->dwPlayerID);
	if (pOrigHoldAuthor)
	{
		holdDesc += wszSpace;
		holdDesc += wszLeftParen;
		holdDesc += g_pTheDB->GetMessageText(MID_HoldOriginallyAuthoredBy);
		holdDesc += wszSpace;
		holdDesc += (WSTRING)pOrigHoldAuthor->NameText;
		holdDesc += wszRightParen;
		delete pOrigHoldAuthor;
	}
	this->DescriptionText = holdDesc.c_str();

	this->dwPlayerID = dwNewAuthorID;
   return Update();
}

//*****************************************************************************
bool CDbHold::DeleteEntrance(CEntranceData *pEntrance)   //(in/out) deleted on return
//Delete this entrance.
//NOTE: Main entrances should only be deleted through calls by DeleteEntrancesForRoom().
{
	ASSERT(pEntrance);
	UINT wIndex;
	for (wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
		if (pEntrance == this->Entrances[wIndex])
			break;
	//If entrance was not found, then it wasn't part of this hold.
	if (wIndex == this->Entrances.size())
		return false;

	this->Entrances[wIndex] = this->Entrances[this->Entrances.size()-1];
	this->Entrances.pop_back();
	const bool bMainEntrance = pEntrance->bIsMainEntrance;
	const UINT dwMessageID = pEntrance->DescriptionText.GetMessageID();
	if (dwMessageID)
		this->deletedTextIDs.push_back(dwMessageID);
	delete pEntrance;
	return bMainEntrance;
}

//*****************************************************************************
void CDbHold::DeleteEntrancesForRoom(
//Scan all entrance records for those belonging to specified room.
//If a deleted record is the main entrance for a level, mark another entrance
//belonging to a room in the level as the new main one, if one exists.
//
//Params:
	const UINT dwRoomID)   //(in)
{
	bool bRemovedMainEntrance = false;
	UINT wIndex;
	for (wIndex=this->Entrances.size(); wIndex--; ) //do backwards
	{
		CEntranceData *pEntrance = this->Entrances[wIndex];
		if (dwRoomID == pEntrance->dwRoomID)
		{
			//Delete this entrance.
			bRemovedMainEntrance |= DeleteEntrance(pEntrance);
		}
	}
	if (!bRemovedMainEntrance)
	{
		Update();
		return;
	}

	//Level's main entrance was removed.  Designate a new one, if possible.
	//Compile room IDs in level.
	const UINT dwLevelID = CDbRooms::GetLevelIDForRoom(dwRoomID);
	CIDSet roomIDs = CDb::getRoomsInLevel(dwLevelID);

	//Rescan entrances -- if any of them are for one of the level's rooms,
	//then set it as new main level entrance.
	for (wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
	{
		CEntranceData &entrance = *(this->Entrances[wIndex]);
		if (roomIDs.has(entrance.dwRoomID))
		{
			ASSERT(!entrance.bIsMainEntrance);
			entrance.bIsMainEntrance = true;
			break;
		}
	}
	Update();
}

//*****************************************************************************
const HoldCharacter* CDbHold::GetCharacterConst(const UINT dwCharID) const
//Returns: pointer to character record with given ID, else NULL if doesn't exist
{
	//No custom character has these character type IDs.
	if (dwCharID < CUSTOM_CHARACTER_FIRST || dwCharID == M_NONE)
		return NULL;

	for (vector<HoldCharacter*>::const_iterator character = this->characters.begin();
			character != this->characters.end(); ++character)
	{
		const HoldCharacter *c = *character;
		if (c->dwCharID == dwCharID)
			return c;
	}

	return NULL;  //not found
}

//*****************************************************************************
UINT CDbHold::GetCharacterID(const WCHAR* pwszName) const
//Returns: character ID for first record found with given name, if exists, else 0
{
	for (vector<HoldCharacter*>::const_iterator character = this->characters.begin();
			character != this->characters.end(); ++character)
	{
		const HoldCharacter *c = *character;
		if (!WCSicmp(c->charNameText.c_str(), pwszName)) //case insensitive
			return c->dwCharID;
	}

	return 0;  //name not found
}

//*****************************************************************************
const WCHAR* CDbHold::GetAuthorText() const
//Returns author of the hold or NULL if not found.
{
	//Look up NameMessageID from associated Players record.
	c4_View PlayersView;
	const UINT dwPlayersRowI = LookupRowByPrimaryKey(this->dwPlayerID,
			V_Players, PlayersView);
	if (dwPlayersRowI == ROW_NO_MATCH) {ASSERT(!"Bad player row."); return NULL;}
	const UINT dwNameMessageID = p_NameMessageID( PlayersView[dwPlayersRowI] );

	//Look up message text.
	return g_pTheDB->GetMessageText(static_cast<MESSAGE_ID>(dwNameMessageID));
}

//*****************************************************************************
CIDSet CDbHold::GetDeletedDataIDs() const
//Returns: the set of deleted dataIDs
{
	CIDSet ids;
	for (vector<UINT>::const_iterator iter = this->deletedDataIDs.begin();
			iter != this->deletedDataIDs.end(); ++iter)
		ids += *iter;
	return ids;
}

//*****************************************************************************
UINT CDbHold::GetNewScriptID()
//Returns: the next unique hold script ID.  The caller must remember to Update()
//the hold if this ID gets used.
{
	return ++this->dwScriptID;
}

//*****************************************************************************
UINT CDbHold::GetNewCharacterID()
//Returns: the next unique hold character ID.  The caller must remember to Update()
//the hold if this ID gets used.
{
	//All these IDs must be >= CUSTOM_CHARACTER_FIRST to differentiate them
	//from the stock characters.
	if (!this->dwCharID)
		return this->dwCharID = CUSTOM_CHARACTER_FIRST;
	ASSERT(this->dwCharID >= CUSTOM_CHARACTER_FIRST && this->dwCharID+1 != M_NONE);

	return ++this->dwCharID;
}

//*****************************************************************************
bool CDbHold::PlayerCanEdit(const UINT playerID) const
//Returns: whether specified player may edit this hold.
{
	//Hold author may always edit.
	const UINT dwAuthorID = this->dwPlayerID;
	if (playerID == dwAuthorID)
		return true;

	const CDbHold::EditAccess editAccess = this->editingPrivileges;
	switch (editAccess)
	{
		case CDbHold::Anyone: return true;
		case CDbHold::OnlyYou: return false;
		case CDbHold::YouAndConquerors:
		{
			//Those that finish the hold have access.
			const UINT dwEndHoldID = g_pTheDB->SavedGames.FindByEndHold(dwHoldID);
			return dwEndHoldID != 0;
		}
		case CDbHold::YouAndMasters:
			//Those that finish the hold with 100% secrets found have access.
			return g_pTheDB->Holds.IsHoldMastered(dwHoldID, playerID);
		default: ASSERT(!"Unexpected editAccess."); return false;
	}
}

//*****************************************************************************
void CDbHold::RemoveImageID(const UINT imageID)
//Removes references to this image ID from the hold record,
//replacing them with a 0 value.
{
	for (vector<HoldCharacter*>::iterator character = this->characters.begin();
			character != this->characters.end(); ++character)
	{
		HoldCharacter& c = *(*character);
		if (c.dwDataID_Avatar == imageID)
			c.dwDataID_Avatar = 0;
		if (c.dwDataID_Tiles == imageID)
			c.dwDataID_Tiles = 0;
	}
}

//*****************************************************************************
void CDbHold::RemoveLevel(
//Removes references to rooms in this level's ID from its hold's entrance list.
//Specifically, this replaces 'dwLevelID' in all levels' room Exits lists
//that have it to 'newLevelID'.
//NOTE: RemoveLevel() should generally be called following a call to CDbLevel::Delete().
//WARNING: Any affected rooms currently in memory will need to be reloaded.
//
//Params:
	const UINT dwLevelID, const UINT dwNewEntranceID)   //(in)
{
	ASSERT(dwLevelID);
	ASSERT(IsOpen());
	CDbBase::DirtyHold();

	//If this level is the first level in the hold, update hold's first level ID.
	if (dwLevelID == this->dwLevelID)
	{
		if (dwNewEntranceID)
		{
			CEntranceData *pEntrance = GetEntrance(dwNewEntranceID);
			ASSERT(pEntrance);
			const UINT dwLevelID = CDbRooms::GetLevelIDForRoom(pEntrance->dwRoomID);
			this->dwLevelID = dwLevelID;
		} else {
			//Get ID of first remaining level, else 0.
			CIDSet LevelIDs = CDb::getLevelsInHold(this->dwHoldID);
			this->dwLevelID = LevelIDs.getFirst();
		}
	}

	//Update Exits sub-records in any rooms in hold leading to entrances in this level.
	CIDSet roomsInHold = CDb::getRoomsInHold(this->dwHoldID);
	CIDSet roomsInLevel = CDb::getRoomsInLevel(dwLevelID);
	c4_View ExitsView;

	for (CIDSet::const_iterator room = roomsInHold.begin();
			room != roomsInHold.end(); ++room)
	{
		c4_View RoomsView;
		const UINT roomRowI = LookupRowByPrimaryKey(*room, V_Rooms, RoomsView);
		c4_RowRef row = RoomsView[roomRowI];

		ExitsView = p_Exits(row);
		const UINT wExitCount = ExitsView.GetSize();
		for (UINT wExitI = 0; wExitI < wExitCount; ++wExitI)
		{
			//If level exit leads to room in level being removed, update the
			//exit's EntranceID to the newEntranceID.
			c4_RowRef exitRow = ExitsView[wExitI];
			const UINT dwEntranceID = (UINT) p_EntranceID(exitRow);
			if (dwEntranceID)
			{
				CEntranceData *pEntrance = GetEntrance(dwEntranceID);
				if (!pEntrance)
					p_EntranceID(exitRow) = 0; //bad entrance -- remove reference
				else if (roomsInLevel.has(pEntrance->dwRoomID))
					p_EntranceID(exitRow) = dwNewEntranceID;
			}
		}
	}

	//Remove Entrance records for any rooms in this level.
	for (UINT wEntranceI = this->Entrances.size(); wEntranceI--; ) //do backwards
	{
		CEntranceData *pEntrance = this->Entrances[wEntranceI];
		if (roomsInLevel.has(pEntrance->dwRoomID))
		{
			delete pEntrance;
			this->Entrances[wEntranceI] = this->Entrances[this->Entrances.size()-1];
			this->Entrances.pop_back();
		}
	}
}

//*****************************************************************************
bool CDbHold::Load(
//Loads a hold from database into this object.
//
//Params:
	const UINT dwLoadHoldID,  //(in) HoldID of hold to load.
	const bool bQuick) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {

	Clear();

	//Open holds view.
	ASSERT(IsOpen());
	c4_View HoldsView;

	//Find record with matching hold ID.
	UINT dwEndHoldMessageID;
	const UINT dwHoldI = LookupRowByPrimaryKey(dwLoadHoldID, V_Holds, HoldsView);
	if (dwHoldI == ROW_NO_MATCH) throw CException("CDbHold::Load");
	c4_RowRef row = HoldsView[dwHoldI];

	//Load in props from Holds record.
	this->dwHoldID = (UINT) p_HoldID(row);
	this->NameText.Bind((UINT) p_NameMessageID(row));
	this->DescriptionText.Bind((UINT) p_DescriptionMessageID(row));
	this->dwLevelID = (UINT) p_LevelID(row);
	this->Created = (time_t) (p_GID_Created(row));
	this->LastUpdated = (time_t) (p_LastUpdated(row));
	this->dwPlayerID = (UINT) p_GID_PlayerID(row);
	this->dwNewLevelIndex = (UINT) (p_GID_NewLevelIndex(row));
	this->editingPrivileges = (CDbHold::EditAccess)(UINT)(p_EditingPrivileges(row));
	dwEndHoldMessageID = (UINT) p_EndHoldMessageID(row);
	if (dwEndHoldMessageID)
		this->EndHoldText.Bind(dwEndHoldMessageID);
	this->dwScriptID = (UINT) p_ScriptID(row);
	this->status = (CDbHold::HoldStatus)(UINT)(p_Status(row));
	this->dwVarID = (UINT) p_VarID(row);
	this->dwCharID = (UINT) p_CharID(row);

	this->bPartialLoad = bQuick;
	if (!bQuick)
	{
		c4_View EntrancesView = p_Entrances(row);
		if (!LoadEntrances(EntrancesView)) throw CException("CDbHold::Load");
		c4_View VarsView = p_Vars(row);
		if (!LoadVars(VarsView)) throw CException("CDbHold::Load");
		c4_View CharactersView = p_Characters(row);
		if (!LoadCharacters(CharactersView)) throw CException("CDbHold::Load");
	}

	this->bCaravelNetMedia = (UINT)p_CaravelNetMedia(row) != 0;

	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CDbHold::LoadCharacters(
//Loads hold's characters from database into member vars of object.
//
//Returns: True if successful, false if not.
//
//Params:
	c4_View &CharactersView)    //(in) Open view containing 0 or more characters
{
	const UINT wSize = CharactersView.GetSize();
	for (UINT wCharI=0; wCharI<wSize; ++wCharI)
	{
		c4_RowRef row = CharactersView[wCharI];

		c4_Bytes CharNameTextBytes = p_CharNameText(row);
		WSTRING name;
		GetWString(name, CharNameTextBytes);
		CDbPackedVars ExtraVars;
		ExtraVars = p_ExtraVars(row);

		this->characters.push_back(new HoldCharacter(
				(UINT)(p_CharID(row)),
				name.c_str(),
				(UINT)(p_Type(row)),
				(UINT)(p_DataID(row)),
				(UINT)(p_DataIDTiles(row)),
				(UINT)(p_AnimationSpeed(row)),
				ExtraVars
		));
	}

	return true;
}

//*****************************************************************************
bool CDbHold::LoadEntrances(
//Loads hold's level entrances from database into member vars of object.
//
//Params:
	c4_View &EntrancesView)    //(in) Open view containing 0 or more entrances.
//
//Returns: True if successful, false if not.
{
	try {

	UINT wEntranceI;
	const UINT wSize = EntrancesView.GetSize();
	for (wEntranceI=0; wEntranceI<wSize; ++wEntranceI)
	{
		c4_RowRef row = EntrancesView[wEntranceI];
		CEntranceData *pEntrance = new CEntranceData(
				(UINT) (p_EntranceID(row)),
				(UINT) (p_DescriptionMessageID(row)),
				(UINT) (p_RoomID(row)),
				p_X(row),
				p_Y(row),
				p_O(row),
				p_IsMainEntrance(row) != 0,
				static_cast<CEntranceData::DescriptionDisplay>((int) (p_ShowDescription(row)))
		);
		if (!pEntrance) throw CException("CDbHold::LoadEntrances");
		this->Entrances.push_back(pEntrance);
	}

	}
	catch (CException&)
	{
		ClearEntrances();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CDbHold::LoadVars(
//Loads hold's vars from database into member vars of object.
//
//Returns: True if successful, false if not.
//
//Params:
	c4_View &VarsView)    //(in) Open view containing 0 or more vars
{
	UINT wVarI;
	const UINT wSize = VarsView.GetSize();
	for (wVarI=0; wVarI<wSize; ++wVarI)
	{
		c4_RowRef row = VarsView[wVarI];
		c4_Bytes VarNameTextBytes = p_VarNameText(row);
		WSTRING name;
		GetWString(name, VarNameTextBytes);
		this->vars.push_back(HoldVar(
				(UINT)(p_VarID(row)),
				name.c_str()
		));
	}

	return true;
}

//*****************************************************************************
void CDbHold::AddEntrance(
//Adds this entrance to the hold's entrance list.
//
//Params:
	CEntranceData* pEntrance,        //(in) entrance to add
	const bool bReplaceMainEntrance) //(in) [default = true]
{
	ASSERT(pEntrance);
	ASSERT(pEntrance->dwRoomID);

	//If the entrance being added is a main entrance, it replaces
	//(or, optionally, is replaced by) any existing main entrance for the same level.
	if (pEntrance->bIsMainEntrance)
	{
		const UINT dwLevelID = CDbRooms::GetLevelIDForRoom(pEntrance->dwRoomID);
		CEntranceData *pMainEntrance = GetMainEntranceForLevel(dwLevelID);
		if (pMainEntrance)
		{
			if (bReplaceMainEntrance)
				pMainEntrance->bIsMainEntrance = false;
			else
				pEntrance->bIsMainEntrance = false;
		}
	}

	//Set the EntranceID for this entrance uniquely.
	UINT dwMaxEntranceID = 0;
	for (ENTRANCE_VECTOR::const_iterator entrance = this->Entrances.begin();
		entrance != this->Entrances.end(); ++entrance)
	{
		if ((*entrance)->dwEntranceID > dwMaxEntranceID)
			dwMaxEntranceID = (*entrance)->dwEntranceID;
	}
	pEntrance->dwEntranceID = dwMaxEntranceID + 1;

	this->Entrances.push_back(pEntrance);
}

//*****************************************************************************
CEntranceData* CDbHold::GetMainEntranceForLevel(const UINT dwLevelID) const
//Returns: pointer to main entrance object in specified level, NULL if none
{
	if (!dwLevelID) return NULL;

	for (UINT wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->Entrances[wIndex];
		if (pEntrance->bIsMainEntrance)
		{
			const UINT dwRoomID = pEntrance->dwRoomID;
			if (dwLevelID == g_pTheDB->Rooms.GetLevelIDForRoom(dwRoomID))
				return pEntrance;
		}
	}
	return NULL;
}

//*****************************************************************************
UINT CDbHold::GetMainEntranceIDForLevel(const UINT dwLevelID) const
//Returns: entrance ID of main entrance in specified level, 0 if none
{
	for (UINT wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->Entrances[wIndex];
		if (pEntrance->bIsMainEntrance)
		{
			const UINT dwRoomID = pEntrance->dwRoomID;
			if (dwLevelID == g_pTheDB->Rooms.GetLevelIDForRoom(dwRoomID))
				return pEntrance->dwEntranceID;
		}
	}
	return 0;
}

//*****************************************************************************
CEntranceData* CDbHold::GetEntrance(const UINT dwEntranceID) const
//Returns: pointer to entrance object with specified ID
{
	for (UINT wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->Entrances[wIndex];
		if (dwEntranceID == pEntrance->dwEntranceID)
			return pEntrance;
	}
	return NULL;
}

//*****************************************************************************
CEntranceData* CDbHold::GetEntranceAt(
//Returns: pointer to entrance object with specified Room ID and coords
//
//Params:
	const UINT dwRoomID, const UINT wX, const UINT wY)   //(in)
const
{
	for (UINT wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->Entrances[wIndex];
		if (dwRoomID == pEntrance->dwRoomID && wX == pEntrance->wX && wY == pEntrance->wY)
			return pEntrance;
	}
	return NULL;
}

//*****************************************************************************
UINT CDbHold::GetMainEntranceRoomIDForLevel(const UINT dwLevelID) const
//Returns: main entrance's room ID in specified level
{
	CEntranceData *pEntrance = GetMainEntranceForLevel(dwLevelID);
	if (!pEntrance)
		return 0;
	return pEntrance->dwRoomID;
}

//*****************************************************************************
UINT CDbHold::GetEntranceIndex(CEntranceData *pEntrance) const
//Returns: index of this entrance record, or -1 if not in this hold.
{
	ASSERT(pEntrance);
	UINT wIndex;
	for (wIndex=0; wIndex<this->Entrances.size(); ++wIndex)
		if (pEntrance == this->Entrances[wIndex])
			return wIndex;

	//Entrance isn't one of the ones in this hold.
	return (UINT)-1;
}

//*****************************************************************************
CDbLevel* CDbHold::GetStartingLevel()
//Gets starting level for this hold.
//
//Returns:
//Pointer to new loaded level object which caller must delete, or NULL if level
//was not defined or could not be loaded.
const
{
	if (this->dwLevelID==0)
		return NULL;

	CDbLevel *pLevel = new CDbLevel();
	if (pLevel)
	{
		if (!pLevel->Load(this->dwLevelID))
		{
			delete pLevel;
			pLevel = NULL;
		}
	}
	return pLevel;
}

//*****************************************************************************
void CDbHold::getStats(RoomStats& stats) const
//Tallies the value of stat-affecting items in the hold.
{
	CIDSet levelIDs = CDb::getLevelsInHold(this->dwHoldID);
	for (CIDSet::const_iterator level = levelIDs.begin(); level != levelIDs.end(); ++level)
	{
		const CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*level);
		ASSERT(pLevel);

		pLevel->getStats(stats);

		delete pLevel;
	}
}

//*****************************************************************************
char* CDbHold::getVarAccessToken(const WCHAR* pName) const
//Returns: pointer to the text string used to access the variable
//with this name in the hold's packed vars
{
	static char varName[12];
	memset(varName, 0, 12 * sizeof(char)); //reset string
	varName[0] = 'v';
	const UINT dwVarID = GetVarID(pName);

	char varID[10];
	_itoa(dwVarID, varID, 10);
	strcat(varName, varID);

	return varName;
}

//*****************************************************************************
UINT CDbHold::GetVarID(const WCHAR* pwszName) const
//Returns: ID of var if name is found, else 0.
{
	if (!pwszName)
		return 0;
	for (vector<HoldVar>::const_iterator var = this->vars.begin();
			var != this->vars.end(); ++var)
		if (!var->varNameText.compare(pwszName))
			return var->dwVarID;
	return 0;
}

//*****************************************************************************
const WCHAR* CDbHold::GetVarName(const UINT dwVarID) const
//Returns: var ID's name string, or NULL if ID is not found.
{
	for (vector<HoldVar>::const_iterator var = this->vars.begin();
			var != this->vars.end(); ++var)
		if (var->dwVarID == dwVarID)
			return var->varNameText.c_str();
	return NULL;
}

//*****************************************************************************
bool CDbHold::RenameCharacter(const UINT dwCharID, const WSTRING& newName)
//Renames character with specified ID.
//Returns: whether character with this ID exists and newName is unique
{
	for (vector<HoldCharacter*>::iterator character = this->characters.begin();
			character != this->characters.end(); ++character)
	{
		HoldCharacter& c = *(*character);
		if (c.dwCharID == dwCharID)
		{
			//Found this character.  Ensure new name is unique.
			for (vector<HoldCharacter*>::iterator character2 = this->characters.begin();
					character2 != this->characters.end(); ++character2)
				if (character2 != character && //other characters only
						!(*character2)->charNameText.compare(newName))
					return false;

			//New name is unique.  Assign it.
			c.charNameText = newName;
			return true;
		}
	}

	return false; //ID not found
}

//*****************************************************************************
bool CDbHold::RenameVar(const UINT dwVarID, const WSTRING& newName)
//Renames variable with specified ID.
//Returns: whether var with this ID exists and newName is unique
{
	for (vector<HoldVar>::iterator var = this->vars.begin();
			var != this->vars.end(); ++var)
		if (var->dwVarID == dwVarID)
		{
			//Found this var.  Ensure new name is unique.
			for (vector<HoldVar>::iterator var2 = this->vars.begin();
					var2 != this->vars.end(); ++var2)
				if (var2 != var && !var2->varNameText.compare(newName)) //other vars only
					return false;

			//New name is unique.  Assign it.
			var->varNameText = newName;
			return true;
		}
	return false; //ID not found
}

//*****************************************************************************
void CDbHold::InsertLevel(
//Inserts the supplied level into this hold.
//
//If dwLevelSupplantedID was the ID of the hold's first level,
//the new level becomes the hold's first level.
//
//Params:
	CDbLevel *pLevel) //(in) Level being inserted
{
	ASSERT(IsOpen());
	ASSERT(pLevel->dwHoldID);
	ASSERT(pLevel->dwHoldID == this->dwHoldID);

	//Give level a (GID) value in the hold.
	ASSERT(pLevel->dwLevelIndex == 0);  //level is not currently a member of a hold
	pLevel->dwLevelIndex = pLevel->dwOrderIndex = ++this->dwNewLevelIndex;
	ASSERT(pLevel->dwLevelIndex);

	//If hold doesn't have a first level, set hold's first level ID.
	if (!this->dwLevelID)
	{
		ASSERT(pLevel->dwLevelID);   //otherwise, the hold will not work
		this->dwLevelID = pLevel->dwLevelID;
	}

	//Save hold's modified dwNewLevelIndex, etc.
	Update();
	pLevel->Update();
}

//*****************************************************************************
CDbHold* CDbHold::MakeCopy()
//Creates a copy of the entire hold, saving it to the DB.
//Also copies relevant saved games, demos, and media objects.
//
//Returns: pointer to new hold
{
	CDbHold *pNewHold = g_pTheDB->Holds.GetNew();
	if (!pNewHold) return NULL;

	pNewHold->SetMembers(*this, false);         //must make new message texts
	pNewHold->Created = 0;     //update timestamps
	pNewHold->Update();

	//Entrances get reinserted in SaveCopyOfLevels
	pNewHold->ClearEntrances();

	//Copy all levels in hold.
	CImportInfo info;
	VERIFY(SaveCopyOfLevels(pNewHold, info));

	//Copy embedded media objects.
	CopyHoldMedia(pNewHold, info);

	//Update all room IDs in new hold's entrance list.
	for (UINT wIndex=0; wIndex<pNewHold->Entrances.size(); ++wIndex)
	{
		//1. Get old room coords.
		CDbRoom *pOldEntranceRoom = g_pTheDB->Rooms.GetByID(
				pNewHold->Entrances[wIndex]->dwRoomID, true);
		ASSERT(pOldEntranceRoom);

		//2. Get old level GID.
		CDbLevel *pOldLevel = g_pTheDB->Levels.GetByID(pOldEntranceRoom->dwLevelID);
		ASSERT(pOldLevel);

		//3. Get new level ID from old level GID (indices match when hold is copied).
		const UINT dwNewLevelID = g_pTheDB->Holds.GetLevelIDAtIndex(
				pOldLevel->dwLevelIndex, pNewHold->dwHoldID);
		delete pOldLevel;

		//4. Get room ID with analogous coords of old entrance room.
		CDbLevel *pNewLevel = g_pTheDB->Levels.GetByID(dwNewLevelID);
		const UINT dwNewEntranceRoomID = pNewLevel->GetRoomIDAtCoords(
				pOldEntranceRoom->dwRoomX,
				(dwNewLevelID * 100) + (pOldEntranceRoom->dwRoomY % 100));
		delete pNewLevel;
		delete pOldEntranceRoom;

		//5. Set entrance to point to new room ID.
		pNewHold->Entrances[wIndex]->dwRoomID = dwNewEntranceRoomID;
	}
	pNewHold->Update();

	//Make copy of show demos.
	CDb db;
	db.Demos.FilterByShow();
	CIDSet showSequenceDemoIDs, showDemoIDs = db.Demos.GetIDs();
	for (CIDSet::const_iterator demo = showDemoIDs.begin(); demo != showDemoIDs.end(); ++demo)
	{
		const UINT dwDemoHoldID = CDb::getHoldOfDemo(*demo);
		if (dwDemoHoldID == this->dwHoldID)
			showSequenceDemoIDs += *demo;
	}
	for (CIDSet::const_iterator id=showSequenceDemoIDs.begin();
			id!=showSequenceDemoIDs.end(); ++id)
	{
		CDbDemo *pDemo = db.Demos.GetByID(*id);
		if (!pDemo) continue;
		CDbSavedGame *pSavedGame = db.SavedGames.GetByID(pDemo->dwSavedGameID);
		if (!pSavedGame) {delete pDemo; continue;}

		pDemo->dwDemoID = 0; //make copy
		pSavedGame->dwSavedGameID = 0; //make copy

		//Hook demo's saved game into room in new hold.
		{
			//1. Get old room coords.
			CDbRoom *pOldRoom = g_pTheDB->Rooms.GetByID(pSavedGame->dwRoomID, true);
			ASSERT(pOldRoom);

			//2. Get old level GID.
			CDbLevel *pOldLevel = g_pTheDB->Levels.GetByID(pOldRoom->dwLevelID);
			ASSERT(pOldLevel);

			//3. Get new level ID from old level GID (indices match when hold is copied).
			const UINT dwNewLevelID = g_pTheDB->Holds.GetLevelIDAtIndex(
					pOldLevel->dwLevelIndex, pNewHold->dwHoldID);
			delete pOldLevel;

			//4. Get room ID with analogous coords of old room.
			CDbLevel *pNewLevel = g_pTheDB->Levels.GetByID(dwNewLevelID);
			const UINT dwNewRoomID = pNewLevel->GetRoomIDAtCoords(
					pOldRoom->dwRoomX, (dwNewLevelID * 100) + (pOldRoom->dwRoomY % 100));
			delete pNewLevel;
			delete pOldRoom;

			//5. Set saved game to point to new room ID.
			pSavedGame->dwRoomID = dwNewRoomID;
		}

		pSavedGame->Update();
		pDemo->dwSavedGameID = pSavedGame->dwSavedGameID;
		pDemo->wShowSequenceNo = db.Demos.GetNextSequenceNo();
		pDemo->Update();
		delete pSavedGame;
		delete pDemo;
	}

	//Copy all saved games in hold.
	const CIDSet savedGamesInHold = CDb::getSavedGamesInHold(this->dwHoldID);
	for (CIDSet::const_iterator setIter = savedGamesInHold.begin();
			setIter != savedGamesInHold.end(); ++setIter)
	{
		CDbSavedGame *pSavedGame = db.SavedGames.GetByID(*setIter);
		ASSERT(pSavedGame);
		if (IsCopyableSavedGame(pSavedGame->eType))
		{
			pSavedGame->dwSavedGameID = 0;   //add to DB as a new saved game

			//Convert old room IDs to new room IDs.
			if (pSavedGame->eType != ST_EndHold)
			{
				PrimaryKeyMap::const_iterator localID = info.RoomIDMap.find(pSavedGame->dwRoomID);
				ASSERT(localID != info.RoomIDMap.end());
				if (localID != info.RoomIDMap.end()) {
					ASSERT(localID->second != 0);
					pSavedGame->dwRoomID = localID->second;
				} else {
					//fallback
					{ //if (pSavedGame->eType != ST_EndHold && pSavedGame->eType != ST_HoldMastered) {
						//bad record
						delete pSavedGame;
						continue;
					}
/*
					if (pSavedGame->ExploredRooms.empty()) {
						delete pSavedGame;
						continue;
					}

					//keep record linked to this hold
					pSavedGame->dwRoomID = pSavedGame->ExploredRooms.getFirst();
*/
				}
			}

/*
			CIDSet rooms;
			CIDSet::const_iterator room;
			for (room = pSavedGame->ConqueredRooms.begin();
					room != pSavedGame->ConqueredRooms.end();  ++room)
			{
				PrimaryKeyMap::const_iterator localID = info.RoomIDMap.find(*room);
				ASSERT(localID != info.RoomIDMap.end());
				if (localID != info.RoomIDMap.end()) { //robustness guard
					ASSERT(localID->second != 0);
					rooms += localID->second;
				}
			}
			pSavedGame->ConqueredRooms = rooms;

			rooms.clear();
*/
			
			for (vector<ExploredRoom*>::const_iterator room = pSavedGame->ExploredRooms.begin();
					room != pSavedGame->ExploredRooms.end();  ++room)
			{
				ExploredRoom* pExploredRoom = *room;
				PrimaryKeyMap::const_iterator localID = info.RoomIDMap.find(pExploredRoom->roomID);
				ASSERT(localID != info.RoomIDMap.end());
				if (localID != info.RoomIDMap.end()) { //robustness guard
					ASSERT(localID->second != 0);
					pExploredRoom->roomID = localID->second;
				}
			}

			pSavedGame->Update();
		}
		delete pSavedGame;
	}

	return pNewHold;
}

//*****************************************************************************
void CDbHold::MarkDataForDeletion(const UINT dataID)
//Keep track of data IDs so that data object is deleted if Update is called.
{
	if (dataID)
		this->deletedDataIDs.push_back(dataID);
}

//*****************************************************************************
void CDbHold::MarkSpeechForDeletion(
//Keep track of speech IDs so that speech object is deleted if Update is called.
//
//Params:
	CDbSpeech* pSpeech)           //(in)
{
	ASSERT(pSpeech);
	if (pSpeech->dwSpeechID)
		this->deletedSpeechIDs.push_back(pSpeech->dwSpeechID);
}

//*****************************************************************************
bool CDbHold::Repair()
//Repair corrupted data, if necessary (might be needed for data from older versions).
//
//Returns: whether bad data were corrected
{
	bool bRepaired = false;
	CIDSet levelIDs, roomIDs, scriptIDs;
	UINT wNextFreeScriptID = 0;

	//Repair hold: first level.
	levelIDs = CDb::getLevelsInHold(this->dwHoldID);
	ASSERT(!levelIDs.has(0)); //no level may have a 0 id
	if (!levelIDs.has(this->dwLevelID))
	{
		//Entrance level ID is bogus -- correct it.
		//Find the level first in the level ordering.
		UINT lowestLevelIndex = UINT(-1);
		for (CIDSet::const_iterator level = levelIDs.begin();
				level != levelIDs.end(); ++level)
		{
			CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*level);
			ASSERT(pLevel);
			if (pLevel->dwOrderIndex < lowestLevelIndex)
			{
				this->dwLevelID = *level;
				lowestLevelIndex = pLevel->dwOrderIndex;
			}

			delete pLevel;
		}
		bRepaired |= (lowestLevelIndex < UINT(-1));
	}

	//Repair rooms: unique NPC IDs.
	roomIDs = CDb::getRoomsInHold(this->dwHoldID);
	for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
	{
		WSTRING roomText;
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room);
		ASSERT(pRoom);
		bool bScriptIDRepaired = false;

		for (CMonster *pMonster = pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		{
			if (pMonster->wType == M_CHARACTER)
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);

				//Check whether characters script IDs collide.
				if (scriptIDs.contains(pCharacter->dwScriptID))
				{
					pCharacter->dwScriptID = wNextFreeScriptID = GetNewScriptID();
					bScriptIDRepaired = true;
				}
				VERIFY(!(scriptIDs += pCharacter->dwScriptID).empty());
			}
		}

		if (bScriptIDRepaired)
			pRoom->Update();

		delete pRoom;
	}

	if (wNextFreeScriptID)
	{
		//Some script IDs were repaired.  Update hold's iterated script ID.
		Update();

		bRepaired = true;
	}

	return bRepaired;
}

//*****************************************************************************
bool CDbHold::SaveCopyOfLevels(
//Make copies of all levels in a hold in the DB and assigns them to a new hold.
//
//Post-Cond: pHold's Update method is called
//
//Returns: true if operation succeeded, else false
//
//Params:
	CDbHold *pNewHold,  //(in) hold new levels belong to
	CImportInfo& info) //(in/out)
{
	ASSERT(pNewHold);
	UINT dwEntranceLevelID = 0L;

	vector<CDbLevel*> newLevels;
	const CIDSet levelsInHold = CDb::getLevelsInHold(this->dwHoldID);
	for (CIDSet::const_iterator level = levelsInHold.begin(); level != levelsInHold.end(); ++level)
	{
		CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*level);
		ASSERT(pLevel);
		const bool bEntranceLevel = pLevel->dwLevelID == this->dwLevelID;
		CDbLevel *pNewLevel = pLevel->CopyIntoHold(pNewHold, info);
		pNewLevel->dwHoldID = pNewHold->dwHoldID;
		pNewLevel->Update();
		newLevels.push_back(pNewLevel);
		delete pLevel;

		if (bEntranceLevel)
		{
			//Get hold's new entrance level ID.
			ASSERT(dwEntranceLevelID == 0); //there should only be one
			dwEntranceLevelID = pNewLevel->dwLevelID;
		}
	}

	//Rekey the exits once entrance ID mapping for all levels has been established.
	for (vector<CDbLevel*>::const_iterator it=newLevels.begin(); it!=newLevels.end(); ++it)
	{
		CDbLevel *pNewLevel = *it;
		pNewLevel->RekeyExitIDs(this, pNewHold, info);
		delete pNewLevel;
	}

	//Set new hold's entrance level ID.
	pNewHold->dwLevelID = dwEntranceLevelID;
	pNewHold->Update();
	return true;
}

//*****************************************************************************
bool CDbHold::SetMembers(
//For copy constructor and assignment operator.
//
//Params:
	const CDbHold &Src,
	const bool bCopyLocalInfo) //(in) default = true
{
	//Retain prior IDs, if requested.
	if (!bCopyLocalInfo)
	{
		Clear();
	} else {
		ClearEntrances();

		this->dwHoldID = Src.dwHoldID;
		this->dwLevelID = Src.dwLevelID;

		this->bCaravelNetMedia = Src.bCaravelNetMedia;

		this->deletedTextIDs = Src.deletedTextIDs;
		this->deletedDataIDs = Src.deletedDataIDs;
		this->deletedSpeechIDs = Src.deletedSpeechIDs;

		//Don't make a duplicate copy of the texts in DB.
		UINT dwMessageID = Src.NameText.GetMessageID();
		if (dwMessageID) this->NameText.Bind(dwMessageID);
		dwMessageID = Src.DescriptionText.GetMessageID();
		if (dwMessageID) this->DescriptionText.Bind(dwMessageID);
		dwMessageID = Src.EndHoldText.GetMessageID();
		if (dwMessageID) this->EndHoldText.Bind(dwMessageID);
	}
	this->dwPlayerID = Src.dwPlayerID;
	this->editingPrivileges = Src.editingPrivileges;
	this->dwNewLevelIndex = Src.dwNewLevelIndex;

	//Make a copy of the texts.
	this->NameText = Src.NameText;
	this->DescriptionText = Src.DescriptionText;
	this->EndHoldText = Src.EndHoldText;

	this->Created = Src.Created;
	this->LastUpdated = Src.LastUpdated;
	this->status = Src.status;
	this->dwScriptID = Src.dwScriptID;
	this->dwVarID = Src.dwVarID;
	this->dwCharID = Src.dwCharID;

	UINT wIndex;
	for (wIndex=0; wIndex<Src.Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = new CEntranceData();
		if (!pEntrance) {Clear(); return false;}
		pEntrance->SetMembers(*Src.Entrances[wIndex], bCopyLocalInfo);
		this->Entrances.push_back(pEntrance);
	}

	this->vars = Src.vars;

	for (vector<HoldCharacter*>::const_iterator chIter = Src.characters.begin();
			chIter != Src.characters.end(); ++chIter)
	{
		this->characters.push_back(new HoldCharacter(*(*chIter), !bCopyLocalInfo));
	}

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbHold::SetProperty(
//Used during XML import of language modules.
	const PROPTYPE pType,
	const char** atts,
	CImportInfo &info)
{
	switch (pType)
	{
		case P_NameMessage:
			ASSERT(!info.pImportEntrance);
			this->NameText.ImportText(atts);
		break;
		case P_DescriptionMessage:
			if (info.pImportEntrance)
			{
				//Time to get the text for this entrance.
				CEntranceData *pEntrance = GetEntrance(info.pImportEntrance->dwEntranceID);
				if (pEntrance)
					pEntrance->DescriptionText.ImportText(atts);
				break;
			}

			this->DescriptionText.ImportText(atts);
		break;
		case P_EndHoldMessage:
			ASSERT(!info.pImportEntrance);
			this->EndHoldText.ImportText(atts);
		break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbHold::SetProperty(
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
	switch (pType)
	{
		case P_HoldID:
		{
			this->dwHoldID = convertToUINT(str);
			if (!this->dwHoldID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.HoldIDMap.find(this->dwHoldID);
			if (localID != info.HoldIDMap.end())
				//Error - this hold should not have been imported already.
				return MID_FileCorrupted;

			//Look up hold in the DB.
			const HoldStatus eMatchHoldsWithStatus =
					(info.typeBeingImported == CImportInfo::Player ||
							info.typeBeingImported == CImportInfo::SavedGame)
					? Main : NoStatus;	//match player imports for demo and full main hold
			const UINT dwLocalHoldID = GetLocalID(eMatchHoldsWithStatus);
			if (!dwLocalHoldID)
			{
				//Hold not found -- add a new record to the DB.
				if (this->NameText.IsEmpty() ||
						info.typeBeingImported == CImportInfo::LanguageMod)
				{
					//This means this record is a GUID reference for a hold not
					//present.  Ignore importing records related to this hold.
					info.HoldIDMap[this->dwHoldID] = 0;   //skip records with refs to this hold ID
					bSaveRecord = false;
				} else {
					//Import this hold into the DB.
					const UINT dwOldLocalID = this->dwHoldID;
					this->dwHoldID = 0L;
					Update();
					info.HoldIDMap[dwOldLocalID] = this->dwHoldID;
					info.dwHoldImportedID = this->dwHoldID;  //keep track of which hold was imported
				}
			} else {
				//Hold found in DB.
				//Compare hold versions.
				c4_View HoldsView;
				const UINT dwHoldI = LookupRowByPrimaryKey(dwLocalHoldID, V_Holds, HoldsView);
				if (dwHoldI == ROW_NO_MATCH) return MID_FileCorrupted;
				const time_t lastUpdated = (time_t)p_LastUpdated(HoldsView[dwHoldI]);
				switch (info.typeBeingImported)
				{
				case CImportInfo::Hold:
					if (this->NameText.IsEmpty())
					{
						//This is just a hold reference.  Don't need to check its version.
						info.HoldIDMap[this->dwHoldID] = dwLocalHoldID;
						this->dwHoldID = dwLocalHoldID;
						bSaveRecord = false;
					} else {
						//This hold is being imported.
						if ((lastUpdated >= this->LastUpdated && !info.bAllowHoldDowngrade) ||
								!info.bAllowHoldUpgrade)
						{
		               //Don't import this hold since it's an older version of an existing one
							//or hold upgrading has been disabled.
							bSaveRecord = false;
							info.HoldIDMap[this->dwHoldID] = dwLocalHoldID;
							this->dwHoldID = dwLocalHoldID;
							if (!info.bAllowHoldUpgrade || lastUpdated == this->LastUpdated)
								return MID_HoldIdenticalIgnored;	//don't need to replace hold if versions are identical

							CDbXML::ExportSavedGames(this->dwHoldID);	//store current hold progress
							return MID_DowngradeHoldPrompt;
						}

						const UINT dwOldLocalID = this->dwHoldID;

						if (!info.bReplaceOldHolds)
						{
							//Saved games must be reimported to new version of hold.
							const bool bPlayersAffected = CDbXML::ExportSavedGames(dwLocalHoldID);

							//Prompt the player that incompatible saved games for the older hold version will be deleted.
							if (bPlayersAffected)
							{
								bSaveRecord = false;
								return MID_OverwriteHoldPrompt;
							}
							//Otherwise there are no saved games for this hold, so confirmation can be skipped.
						}

						//Remove old hold from the DB.  This new one replaces it.
						g_pTheDB->Holds.Delete(dwLocalHoldID);
						this->dwHoldID = 0;
						Update();
						info.HoldIDMap[dwOldLocalID] = this->dwHoldID;
						info.dwHoldImportedID = this->dwHoldID;  //keep track of which hold was imported
					}
				break;

				case CImportInfo::Player:
					//A player with records for this hold is being imported.
					bSaveRecord = false;
					info.HoldIDMap[this->dwHoldID] = dwLocalHoldID;
					this->dwHoldID = dwLocalHoldID;

					if (lastUpdated != this->LastUpdated &&
							info.ImportStatus == MID_ImportSuccessful)   //player record is actually being imported, because this status code would
								//have been changed to MID_DowngradeHoldPrompt or MID_OverwriteHoldPrompt if the date is different when importing a hold.
					{
						info.localHoldIDs += dwLocalHoldID;

						//If not the official hold, inform the player that hold versions don't match,
						//and saved games from a different hold version might not be valid here.
						if (this->status != Main)
							return MID_PlayerSavesIgnored;
					}
				break;

				case CImportInfo::Demo:
				case CImportInfo::SavedGame:
					//A saved game/demo in this hold is being imported.
					//No hold version checking is done.
					bSaveRecord = false;
					info.HoldIDMap[this->dwHoldID] = dwLocalHoldID;
					this->dwHoldID = dwLocalHoldID;

					//Also, if a local player is being re-imported, its saved games must be reverified.
					if (info.bImportingSavedGames || info.dwPlayerImportedID)
						info.localHoldIDs += dwLocalHoldID;
				break;

				case CImportInfo::LanguageMod:
				{
					//A language module is being imported.
					//Hold versions must match.
					if (lastUpdated != this->LastUpdated)
					{
						//Hold versions are not the same.  Language module import probably won't work properly.
						info.HoldIDMap[this->dwHoldID] = 0; //skip records with refs to this hold ID
						bSaveRecord = false;
						return MID_HoldNotIdenticalIgnored;
					}

					info.HoldIDMap[this->dwHoldID] = dwLocalHoldID;
					info.dwHoldImportedID = dwLocalHoldID;  //keep track of which hold is modified

					//Load local hold data into this record.
					CDbHold *pLocalHold = g_pTheDB->Holds.GetByID(dwLocalHoldID);
					ASSERT(pLocalHold);
					SetMembers(*pLocalHold, true);
					delete pLocalHold;

					//This record won't be directly modified.
					bSaveRecord = false;
				}
				break;

				default: break;
				}  //switch
			}
			break;
		}
		case P_NameMessage:
		{
			WSTRING data;
			Base64::decode(str,data);
			this->NameText = data.c_str();
			break;
		}
		case P_DescriptionMessage:
		{
			WSTRING data;
			Base64::decode(str,data);
			this->DescriptionText = data.c_str();
			break;
		}
		case P_LevelID:
			this->dwLevelID = convertToUINT(str);
			//Level hasn't been read in yet and local ID must be set later
			break;
		case P_GID_Created:
			this->Created = convertToTimeT(str);
			break;
		case P_LastUpdated:
			this->LastUpdated = convertToTimeT(str);
			break;
		case P_GID_PlayerID:
			this->dwPlayerID = convertToUINT(str);
			if (!this->dwPlayerID)
				return MID_FileCorrupted;  //corrupt file (must have an author)

			//Set to local ID.
			localID = info.PlayerIDMap.find(this->dwPlayerID);
			if (localID == info.PlayerIDMap.end())
				return MID_FileCorrupted;  //record should have been loaded already
			this->dwPlayerID = (*localID).second;
			break;
		case P_GID_NewLevelIndex:
			this->dwNewLevelIndex = convertToUINT(str);
			if (this->dwLevelID && !this->dwNewLevelIndex)
				return MID_FileCorrupted;  //bad data -- can't have a level when dwNewLevelIndex == 0
			break;
		case P_EditingPrivileges:
			this->editingPrivileges = static_cast<EditAccess>(convertToInt(str));
			break;
		case P_EndHoldMessage:
		{
			WSTRING data;
			Base64::decode(str,data);
			this->EndHoldText = data.c_str();
			break;
		}
		case P_ScriptID:
			this->dwScriptID = convertToUINT(str);
			break;
		case P_Status:
			this->status = static_cast<HoldStatus>(convertToInt(str));
			break;
		case P_VarID:
			this->dwVarID = convertToUINT(str);
			break;
		case P_CharID:
			this->dwCharID = convertToUINT(str);
			break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbHold::SetProperty(
//Used during XML data import.
//According to vpType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
//NOTE: Unused parameter name commented out to suppress warning
	const VIEWPROPTYPE vpType, //(in) sub-view to modify
	const PROPTYPE pType,   //(in) property (data member) in sub-view to set
	char* const str,        //(in) string representation of value
	CImportInfo &info)  //(in/out) Import data
{
	switch (vpType)
	{
		case VP_Entrances:
			if (pType != P_Start)
				ASSERT(info.pImportEntrance);
			switch (pType)
			{
				case P_Start:
					ASSERT(!info.pImportEntrance);
					info.pImportEntrance = new CEntranceData();
					break;
				case P_EntranceID:
					info.pImportEntrance->dwEntranceID = convertToUINT(str);
					break;
				case P_DescriptionMessage:
				{
					WSTRING data;
					Base64::decode(str,data);
					info.pImportEntrance->DescriptionText = data.c_str();
					break;
				}
				case P_RoomID:
					info.pImportEntrance->dwRoomID = convertToUINT(str);
					//Room hasn't been read in yet and local ID must be set later
					break;
				case P_X:
					info.pImportEntrance->wX = convertToUINT(str);
					break;
				case P_Y:
					info.pImportEntrance->wY = convertToUINT(str);
					break;
				case P_O:
					info.pImportEntrance->wO = convertToUINT(str);
					break;
				case P_IsMainEntrance:
					info.pImportEntrance->bIsMainEntrance = convertIntStrToBool(str);
					break;
				case P_ShowDescription:
					info.pImportEntrance->eShowDescription = static_cast<CEntranceData::DescriptionDisplay>(convertToInt(str));
					break;
				case P_End:
					//Finish processing
					if (info.typeBeingImported == CImportInfo::LanguageMod)
						delete info.pImportEntrance; //don't re-add existing hold members
					else
						this->Entrances.push_back(info.pImportEntrance);
					info.pImportEntrance = NULL;
					break;
				default:
					delete info.pImportEntrance;
					info.pImportEntrance = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Vars:
			switch (pType)
			{
				case P_Start:
					//Start with fresh record.
					ASSERT(!info.importVar.dwVarID);
					ASSERT(info.importVar.varNameText.empty());
					ASSERT(info.typeBeingImported != CImportInfo::LanguageMod);
					break;
				case P_VarID:
					info.importVar.dwVarID = convertToUINT(str);
					break;
				case P_VarNameText:
				{
					WSTRING text;
					Base64::decode(str,text);
					info.importVar.varNameText = text.c_str();
					break;
				}
				case P_End:
					//Finish processing
					this->vars.push_back(info.importVar);
					info.importVar.dwVarID = 0;
					info.importVar.varNameText.resize(0);
					break;
				default:
					info.importVar.dwVarID = 0;
					info.importVar.varNameText.resize(0);
					return MID_FileCorrupted;
			}
			break;
		case VP_Characters:
			switch (pType)
			{
				case P_Start:
					//Start with fresh record.
					ASSERT(!info.importChar.dwCharID);
					ASSERT(info.importChar.charNameText.empty());
					ASSERT(info.typeBeingImported != CImportInfo::LanguageMod);
					break;
				case P_CharID:
					info.importChar.dwCharID = convertToUINT(str);
					break;
				case P_CharNameText:
				{
					WSTRING text;
					Base64::decode(str,text);
					info.importChar.charNameText = text.c_str();
					break;
				}
				case P_Type:
					info.importChar.wType = convertToUINT(str);
					break;
				case P_DataID:
					info.importChar.dwDataID_Avatar = convertToUINT(str);
					if (info.importChar.dwDataID_Avatar)
					{
						//Set to local ID.
						PrimaryKeyMap::iterator localID = info.DataIDMap.find(info.importChar.dwDataID_Avatar);
						if (localID == info.DataIDMap.end())
							return MID_FileCorrupted;  //record should have been loaded already
						info.importChar.dwDataID_Avatar = localID->second;
					}
					break;
				case P_DataIDTiles:
					info.importChar.dwDataID_Tiles = convertToUINT(str);
					if (info.importChar.dwDataID_Tiles)
					{
						//Set to local ID.
						PrimaryKeyMap::iterator localID = info.DataIDMap.find(info.importChar.dwDataID_Tiles);
						if (localID == info.DataIDMap.end())
							return MID_FileCorrupted;  //record should have been loaded already
						info.importChar.dwDataID_Tiles = localID->second;
					}
					break;
				case P_AnimationSpeed:
					info.importChar.animationSpeed = convertToUINT(str);
					break;
				case P_ExtraVars:
				{
					BYTE *data;
					Base64::decode(str,data);

					//Process linked script through CCharacter interface.
					CMonsterFactory mf;
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*,
							mf.GetNewMonster(M_CHARACTER));
					pCharacter->ExtraVars = (const BYTE*)data;
					delete[] data;
					const MESSAGE_ID val = pCharacter->ImportSpeech(info);
					if (val != MID_ImportSuccessful)
						return val;

					info.importChar.ExtraVars = pCharacter->ExtraVars;
					delete pCharacter;

					break;
				}
				case P_End:
					//Finish processing
					this->characters.push_back(new HoldCharacter(info.importChar));
					info.importChar.clear();
					break;
				default:
					info.importChar.clear();
					return MID_FileCorrupted;
			}
			break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbHold::Update()
//Updates database with hold.
//
//Returns: true if successful, else false.
{
	if (this->bPartialLoad)
	{
		ASSERT(!"CDbHold: partial load update");
		return false;
	}

	bool bSuccess=true;

	g_pTheDB->Holds.ResetMembership();
	if (this->dwHoldID == 0)
	{
		//Insert a new hold.
		bSuccess = UpdateNew();
	} else {
		//Update existing hold.
		bSuccess = UpdateExisting();
	}

	return bSuccess;
}

//
//CDbHold private methods.
//

//*****************************************************************************
UINT CDbHold::GetLocalID(
//Compares this object's GID fields against those of the records in the DB.
//ASSUME: dwPlayerID has already been set to the local record ID
//
//Returns: local ID if a record in the DB matches this object's GUID, else 0
//
//Params:
	const HoldStatus eStatusMatching)  //(in) consider holds w/ this status as identical
	                                   //     [default = NoStatus]
const
{
	ASSERT(IsOpen());
	const UINT dwHoldCount = GetViewSize(V_Holds);

	//Each iteration checks a hold's GIDs.
	for (UINT dwHoldI = 0; dwHoldI < dwHoldCount; ++dwHoldI)
	{
		//Check time of creation.
		c4_RowRef row = GetRowRef(V_Holds, dwHoldI);
		const time_t Created = (time_t)p_GID_Created(row);
		if (this->Created == Created)
		{
			//Check author.
			const UINT dwPlayerID = (UINT)p_GID_PlayerID(row);
			if (this->dwPlayerID == dwPlayerID)
			{
				//GUIDs match.  Return this record's local ID.
				return (UINT) p_HoldID(row);
			}
		}

		//Check for matching unique hold status.
		if (this->status == eStatusMatching)
		{
			const HoldStatus status = (HoldStatus)(UINT)p_Status(row);
			if (status == eStatusMatching)
				//Both are considered as the same hold.  Match them.
				return (UINT) p_HoldID(row);
		}
	}

	//No match.
	return 0;
}

//*****************************************************************************
void CDbHold::SaveCharacters(
//Saves hold character from member vars of object into database.
//
//Params:
	c4_View &CharsView)    //(in) Open view to fill.
{
	//Ensure hold objects don't point to data objects being removed.
	UINT wDataI;
	for (wDataI=this->deletedDataIDs.size(); wDataI--; )
		RemoveImageID(this->deletedDataIDs[wDataI]);

	const UINT wSize = this->characters.size();
	CharsView.SetSize(wSize); //speed optimization
	for (UINT wCharI=0; wCharI < wSize; ++wCharI)
	{
		c4_RowRef row = CharsView[wCharI];
		HoldCharacter& ch = *(this->characters[wCharI]);

		//If the character's default script has been edited
		//(indicated by an allocated working copy of the command list)
		//then update the data records owned by the script.
		if (ch.pCommands)
		{
			//Repack the script, saving any owned data objects.
			CCharacter::SaveCommands(ch.ExtraVars, *ch.pCommands);

			//Now the working copy of the default script can be cleared.
			ch.deleteWorkingCommands();
		}

		//Prepare complex data.
		UINT dwVarsSize;
		BYTE *pbytVarsBytes = ch.ExtraVars.GetPackedBuffer(dwVarsSize);
		ASSERT(pbytVarsBytes);
		c4_Bytes VarsBytes(pbytVarsBytes, dwVarsSize);

		p_CharID(row) = ch.dwCharID;
		p_CharNameText(row) = PutWString(ch.charNameText);
		p_Type(row) = ch.wType;
		p_DataID(row) = ch.dwDataID_Avatar;
		p_DataIDTiles(row) = ch.dwDataID_Tiles;
		p_AnimationSpeed(row) = ch.animationSpeed;
		p_ExtraVars(row) = VarsBytes;

		delete[] pbytVarsBytes;
	}

	//Now remove any speeches/data marked for deletion.
	for (UINT wSpeechI=this->deletedSpeechIDs.size(); wSpeechI--; )
		g_pTheDB->Speech.Delete(this->deletedSpeechIDs[wSpeechI]);
	this->deletedSpeechIDs.clear();
	for (wDataI=this->deletedDataIDs.size(); wDataI--; )
		g_pTheDB->Data.Delete(this->deletedDataIDs[wDataI]);
	this->deletedDataIDs.clear();
}

//*****************************************************************************
void CDbHold::SaveEntrances(
//Saves hold entrances from member vars of object into database.
//
//Params:
	c4_View &EntrancesView)    //(in) Open view to fill.
{
	UINT wEntranceI;
	const UINT wSize = this->Entrances.size();
	EntrancesView.SetSize(wSize); //speed optimization
	for (wEntranceI=0; wEntranceI < wSize; ++wEntranceI)
	{
		c4_RowRef row = EntrancesView[wEntranceI];
		CEntranceData& entrance = *(this->Entrances[wEntranceI]);
		p_EntranceID(row) = entrance.dwEntranceID;
		p_DescriptionMessageID(row) = entrance.DescriptionText.UpdateText();
		p_RoomID(row) = entrance.dwRoomID;
		p_X(row) = entrance.wX;
		p_Y(row) = entrance.wY;
		p_O(row) = entrance.wO;
		p_IsMainEntrance(row) = entrance.bIsMainEntrance;
		p_ShowDescription(row) = entrance.eShowDescription;
	}

	//Now remove any message texts marked for deletion.
	for (wEntranceI=this->deletedTextIDs.size(); wEntranceI--; )
		DeleteMessage(this->deletedTextIDs[wEntranceI]);
	this->deletedTextIDs.clear();
}

//*****************************************************************************
void CDbHold::SaveVars(
//Saves hold vars from member vars of object into database.
//
//Params:
	c4_View &VarsView)    //(in) Open view to fill.
{
	UINT wVarI;
	const UINT wSize = this->vars.size();
	VarsView.SetSize(wSize); //speed optimization
	for (wVarI=0; wVarI < wSize; ++wVarI)
	{
		c4_RowRef row = VarsView[wVarI];
		HoldVar& var = this->vars[wVarI];

		p_VarID(row) = var.dwVarID;
		p_VarNameText(row) = PutWString(var.varNameText);
	}
}

//*****************************************************************************
bool CDbHold::UpdateNew()
//Add new Holds record to database.
{
	LOGCONTEXT("CDbHold::UpdateNew");
	ASSERT(this->dwHoldID == 0);
	ASSERT(IsOpen());

	//Prepare props.
	this->dwHoldID = GetIncrementedID(p_HoldID);
	if ((time_t)this->Created == 0)
	{
		this->Created.SetToNow();
		this->LastUpdated.SetToNow();
	}
	c4_View CharactersView, EntrancesView, VarsView;
	SaveCharacters(CharactersView);
	SaveEntrances(EntrancesView);
	SaveVars(VarsView);

	//Write out message texts.
	const UINT dwNameID = this->NameText.UpdateText();
	const UINT dwDescID = this->DescriptionText.UpdateText();
	const UINT dwEndHoldID = this->EndHoldText.UpdateText();
	ASSERT(dwNameID);
	ASSERT(dwDescID);
	//ASSERT(dwEndHoldID);  //might be 0

	//Write new Hold record.
	c4_RowRef row = g_pTheDB->Holds.GetNewRow();
	p_HoldID(row) = this->dwHoldID;
	p_NameMessageID(row) = dwNameID;
	p_DescriptionMessageID(row) = dwDescID;
	p_LevelID(row) = this->dwLevelID;
	p_GID_Created(row) = UINT(this->Created);
	p_LastUpdated(row) = UINT(this->LastUpdated);
	p_GID_PlayerID(row) = this->dwPlayerID;
	p_GID_NewLevelIndex(row) = 0;
	p_EditingPrivileges(row) = this->editingPrivileges;
	p_EndHoldMessageID(row) = dwEndHoldID;
	p_Entrances(row) = EntrancesView;
	p_ScriptID(row) = this->dwScriptID;
	p_Status(row) = this->status;
	p_VarID(row) = this->dwVarID;
	p_Vars(row) = VarsView;
	p_CharID(row) = this->dwCharID;
	p_Characters(row) = CharactersView;
	p_CaravelNetMedia(row) = this->bCaravelNetMedia;

	CDb::addHold(this->dwHoldID);
	return true;
}

//*****************************************************************************
bool CDbHold::UpdateExisting()
//Update an existing Holds record in database.
{
	LOGCONTEXT("CDbHold::UpdateExisting");
	ASSERT(this->dwHoldID != 0);
	ASSERT(IsOpen());

	//Lookup Holds record.
	c4_View HoldsView;
	const UINT dwHoldI = LookupRowByPrimaryKey(this->dwHoldID, V_Holds, HoldsView);
	if (dwHoldI == ROW_NO_MATCH)
	{
		ASSERT(!"Bad holdID.");
		return false;
	}
	c4_RowRef row = HoldsView[dwHoldI];

	//Prepare props.
	if (!CDb::FreezingTimeStamps())
		this->LastUpdated.SetToNow();
	const UINT dwNameID = this->NameText.UpdateText();
	const UINT dwDescID = this->DescriptionText.UpdateText();
	const UINT dwEndHoldID = this->EndHoldText.UpdateText();
	ASSERT(dwNameID);
	ASSERT(dwDescID);
	//ASSERT(dwEndHoldID);  //might be 0
	c4_View CharactersView, EntrancesView, VarsView;
	SaveCharacters(CharactersView);
	SaveEntrances(EntrancesView);
	SaveVars(VarsView);

	//Update Holds record.
	p_HoldID(row) = this->dwHoldID;
	p_LevelID(row) = this->dwLevelID;
	if (this->status != CDbHold::Main) //CaravelNet keeps version of published official holds the same
		p_LastUpdated(row) = UINT(this->LastUpdated);
	p_GID_PlayerID(row) = this->dwPlayerID;
	p_GID_NewLevelIndex(row) = this->dwNewLevelIndex;
	p_EditingPrivileges(row) = this->editingPrivileges;
	p_EndHoldMessageID(row) = dwEndHoldID;
	p_Entrances(row) = EntrancesView;
	p_ScriptID(row) = this->dwScriptID;
	p_Status(row) = this->status;
	p_VarID(row) = this->dwVarID;
	p_Vars(row) = VarsView;
	p_CharID(row) = this->dwCharID;
	p_Characters(row) = CharactersView;
	p_CaravelNetMedia(row) = this->bCaravelNetMedia;

	CDbBase::DirtyHold();
	return true;
}

//*****************************************************************************
void CDbHold::Clear()
//Frees resources associated with this object and resets member vars.
{
	this->bPartialLoad = false;

	this->dwLevelID = this->dwHoldID = this->dwPlayerID = 0;

	this->NameText.Clear();
	this->DescriptionText.Clear();
	this->EndHoldText.Clear();
	this->Created = this->LastUpdated = 0;

	this->dwNewLevelIndex = this->dwScriptID = this->dwVarID = this->dwCharID = 0;
	this->editingPrivileges = Anyone;
	this->status = Homemade;
	this->bCaravelNetMedia = false;

	ClearEntrances();
	this->vars.clear();

	for (vector<HoldCharacter*>::iterator chIt=this->characters.begin();
			chIt!=this->characters.end(); ++chIt)
		delete *chIt;
	this->characters.clear();

	this->deletedTextIDs.clear();
	this->deletedSpeechIDs.clear();
	this->deletedDataIDs.clear();
}

//*****************************************************************************
void CDbHold::ClearEntrances()
//Frees resources associated with this object's level entrances.
{
	for (UINT wIndex=this->Entrances.size(); wIndex--; )
		delete this->Entrances[wIndex];
	this->Entrances.clear();
}

