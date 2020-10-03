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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), John Wm. Wicks (j_wicks),
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//DbRooms.cpp
//Implementation of CDbRooms, CDbRoom, and other classes related to rooms.

#include "DbRooms.h"

#include "Aumtlich.h"
#include "Construct.h"
#include "CurrentGame.h"
#include "Db.h"
#include "DbData.h"
#include "DbProps.h"
#include "Character.h"
#include "Gentryii.h"
#include "MonsterPiece.h"
#include "PlayerDouble.h"
#include "RockGiant.h"
#include "Seep.h"
#include "Serpent.h"
#include "Spider.h"
#include "Stalwart.h"
#include "TemporalClone.h"
#include "Platform.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

//Import utility vars.
CScrollData   *pImportScroll = NULL;
CMonster      *pImportMonster = NULL;
COrbAgentData *pImportOrbAgent = NULL;
COrbData      *pImportOrb = NULL;
CMonsterPiece *pImportPiece = NULL;
CExitData     *pImportExit = NULL;
ROOMCOORD      importCheckpoint;

#define WEATHER_OUTSIDE "outside"
#define WEATHER_LIGHTNING "lightning"
#define WEATHER_CLOUDS "clouds"
#define WEATHER_SUN "sunshine"
#define WEATHER_LIGHTFADE "lf"
#define WEATHER_FOG "fog"
#define WEATHER_LIGHT "light"
#define WEATHER_SNOW "snow"
#define WEATHER_SKY "sky"
#define WEATHER_RAIN "rain"

//
//CDbRooms public methods.
//

//*****************************************************************************
void CDbRooms::Delete(
//Deletes records for a room with the given ID.
//Deletes scroll messages, demos and saved games associated with this room.
//
//Params:
	const UINT dwRoomID)   //(in)   ID of room(s) to delete.
{
	ASSERT(dwRoomID);

	c4_View RoomsView;
	const UINT dwRoomRowI = LookupRowByPrimaryKey(dwRoomID, V_Rooms, RoomsView);
	if (dwRoomRowI == ROW_NO_MATCH) {ASSERT(!"Bad room ID."); return;}

	c4_RowRef row = RoomsView[dwRoomRowI];

	//Delete all scroll messages in room.
	c4_View ScrollsView = p_Scrolls(row);
	for (UINT wScrollI = ScrollsView.GetSize(); wScrollI--; )
	{
		const UINT dwMessageID = p_MessageID(ScrollsView[wScrollI]);
		DeleteMessage(static_cast<MESSAGE_ID>(dwMessageID));
	}

	//Delete all character speech data.
	c4_View MonstersView = p_Monsters(row);
	const UINT wMonsterCount = MonstersView.GetSize();
	for (UINT wMonsterI = 0; wMonsterI < wMonsterCount; ++wMonsterI)
	{
		const UINT wMonsterType = p_Type(MonstersView[wMonsterI]);
		if (wMonsterType == M_CHARACTER)
		{
			CMonster *pNew = CMonsterFactory::GetNewMonster((MONSTERTYPE)wMonsterType);
			ASSERT(pNew);
			pNew->ExtraVars = p_ExtraVars(MonstersView[wMonsterI]);
			pNew->Delete();
			delete pNew;
		}
	}

	//Delete all demos in room (and their associated saved game).
	CDb db;
	CIDSet DemoIDs = CDb::getDemosInRoom(dwRoomID);
	CIDSet::const_iterator iter;
	for (iter = DemoIDs.begin(); iter != DemoIDs.end(); ++iter)
		db.Demos.Delete(*iter);

	//Delete all (remaining) saved games in room.
	CIDSet SavedGameIDs = CDb::getSavedGamesInRoom(dwRoomID);
	for (iter = SavedGameIDs.begin(); iter != SavedGameIDs.end(); ++iter)
		db.SavedGames.Delete(*iter);

	//Remove dangling level entrance records for this room,
	//unless this hold is in process of being deleted, in which case the hold
	//entrances will be deleted anyway and slow hold updates may be avoided.
	const UINT dwLevelID = p_LevelID(row);
	const UINT dwHoldID = db.Levels.GetHoldIDForLevel(dwLevelID);
	if (dwHoldID && dwHoldID != CDbHolds::deletingHoldID)
	{
		CDbHold *pHold = db.Holds.GetByID(dwHoldID);
		ASSERT(pHold);
		pHold->DeleteEntrancesForRoom(dwRoomID);  //performs Update
		delete pHold;
	}

	//Delete the room.
	CDb::deleteRoom(dwRoomID); //call first
	RoomsView.RemoveAt(dwRoomRowI);

	//After room object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
#define STARTVPTAG(str, vpType,pType) str += "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(str, pType) str += "' "; str += PropTypeStr(pType); str += "='"
#define ENDVPTAG(str, vpType) str += "</"; str += ViewpropTypeStr(vpType); str += ">" NEWLINE
#define EXPORTTEXT(str, pType, messageText) messageText.ExportText(str, PropTypeStr(pType))

#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">" NEWLINE
#define CLOSETAG "'/>" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

//*****************************************************************************
bool CDbRooms::ExportText(const UINT dwRoomID, CDbRefs &dbRefs, CStretchyBuffer &str)
//Serialize all texts in the room.
{
	if (dbRefs.IsSet(V_Rooms,dwRoomID))
		return false;

	dbRefs.Set(V_Rooms,dwRoomID);

	CDbRoom *pRoom = GetByID(dwRoomID);
	ASSERT(pRoom);
	if (!pRoom)
		return false; //shouldn't happen -- but this is more robust

	char dummy[32];

	//Wasteful to export room unless there is some text inside it.
	CStretchyBuffer strLocal;

	//Export scroll texts.
	UINT wSize = pRoom->Scrolls.size(), wIndex;
	for (wIndex=0; wIndex<wSize; ++wIndex)
	{
		CScrollData *pScroll = pRoom->Scrolls[wIndex];

		ASSERT(pRoom->IsValidColRow(pScroll->wX, pScroll->wY));
		STARTVPTAG(strLocal, VP_Scrolls, P_X);
		strLocal += INT32TOSTR(pScroll->wX);
		PROPTAG(strLocal, P_Y);
		strLocal += INT32TOSTR(pScroll->wY);
		strLocal += CLOSESTARTTAG;

		EXPORTTEXT(strLocal, P_Message, pScroll->ScrollText);

		ENDVPTAG(strLocal, VP_Scrolls);
	}

	//Character monster texts.
	CMonster *pMonster = pRoom->pFirstMonster;
	while (pMonster)
	{
		ASSERT(pRoom->IsValidColRow(pMonster->wX, pMonster->wY));
		if (M_CHARACTER == pMonster->wType)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->ExportText(dbRefs, strLocal);
		}
		pMonster = pMonster->pNext;
	}

	//Are there any texts in the room?
	if (!strLocal.empty())
	{
		//GID.
		str += STARTTAG(V_Rooms, P_LevelID);
		str += INT32TOSTR(pRoom->dwLevelID);
		PROPTAG(str, P_RoomX);
		str += INT32TOSTR(pRoom->dwRoomX);
		PROPTAG(str, P_RoomY);
		str += INT32TOSTR(pRoom->dwRoomY);
		PROPTAG(str, P_RoomID);
		str += INT32TOSTR(pRoom->dwRoomID);
		str += CLOSESTARTTAG;

		str += strLocal;

		str += ENDTAG(V_Rooms);
	}

	delete pRoom;

	return true;
}

//*****************************************************************************
#undef STARTVPTAG
#undef PROPTAG
#undef ENDVPTAG
#undef EXPORTTEXT

#define STARTVPTAG(vpType,pType) "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDVPTAG(vpType) "</"; str += ViewpropTypeStr(vpType); str += ">\n"

//*****************************************************************************
void CDbRooms::ExportNetRoom(
//Returns: structure that CaravelNet can use to identify a room
//
//Pre-condition: dwRoomID is valid
//
//Params:
	const UINT dwRoomID,	//(in)
	CNetRoom &room)			//(out)
{
	CDbRoom* pRoom = GetByID(dwRoomID);
	if (pRoom) {
		CDbLevel* pLevel = pRoom->GetLevel();
		if (pLevel) {
			CDbHold* pHold = pLevel->GetHold();
			if (pHold) {
				room.bValid = true;
				room.gidCreated = pHold->GetCreated();
				room.lastUpdated = pHold->LastUpdated;
				room.wGidLevelIndex = pLevel->dwLevelIndex;
				pRoom->GetPositionInLevel(room.dx, room.dy);
				delete pHold;
			}
			delete pLevel;
		}
		delete pRoom;
	}
}

//*****************************************************************************
void CDbRooms::ExportXML(
//Returns: string containing XML text describing room with this ID
//
//Pre-condition: dwRoomID is valid
//
//Params:
	const UINT dwRoomID,   //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)        //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_Rooms,dwRoomID))
		return;

	dbRefs.Set(V_Rooms,dwRoomID);

	//Prepare data.
	CDbRoom *pRoom = GetByID(dwRoomID, bRef);
	if (!pRoom)
		return; //might have saved games for old (non-existant) rooms

	char dummy[32];

	//Include corresponding level ref.
	g_pTheDB->Levels.ExportXML(pRoom->dwLevelID, dbRefs, str, true);

	//Include any designated floor and overhead images.
	if (pRoom->dwDataID && !bRef)
	{
		//Only save attached data if it's not a dangling reference.
		if (g_pTheDB->Data.Exists(pRoom->dwDataID))
			g_pTheDB->Data.ExportXML(pRoom->dwDataID, dbRefs, str);
	}
	if (pRoom->dwOverheadDataID && !bRef)
	{
		//Only save attached data if it's not a dangling reference.
		if (g_pTheDB->Data.Exists(pRoom->dwOverheadDataID))
			g_pTheDB->Data.ExportXML(pRoom->dwOverheadDataID, dbRefs, str);
	}

	if (str.size() > str.capacity()*0.98) //minimize string reallocs
		str.reserve(str.capacity() * 2);

	//Export character script speech records first.
#ifndef EXPORTNOSPEECH
	if (!bRef)
	{
		CMonster *pMonster = pRoom->pFirstMonster;
		while (pMonster)
		{
			if (M_CHARACTER == pMonster->wType)
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				str += CCharacter::ExportXMLSpeech(dbRefs, pCharacter->commands);
			}
			pMonster = pMonster->pNext;
		}
	}
#endif

	str += STARTTAG(V_Rooms, P_LevelID);
	str += INT32TOSTR(pRoom->dwLevelID);
	str += PROPTAG(P_RoomX);
	str += INT32TOSTR(pRoom->dwRoomX);
	str += PROPTAG(P_RoomY);
	str += INT32TOSTR(pRoom->dwRoomY);
	str += PROPTAG(P_RoomID);
	str += INT32TOSTR(pRoom->dwRoomID);
	if (bRef)
	{
		//Don't need any further information for a room reference.
		str += CLOSETAG;
	} else {
		str += PROPTAG(P_RoomCols);
		str += INT32TOSTR(pRoom->wRoomCols);
		str += PROPTAG(P_RoomRows);
		str += INT32TOSTR(pRoom->wRoomRows);
		str += PROPTAG(P_StyleName);
		str += Base64::encode(pRoom->style);
		str += PROPTAG(P_IsRequired);
		str += INT32TOSTR(pRoom->bIsRequired);
		str += PROPTAG(P_IsSecret);
		str += INT32TOSTR(pRoom->bIsSecret);
		if (pRoom->dwDataID)
		{
			//Only save attached data if it's not a dangling pointer.
			if (g_pTheDB->Data.Exists(pRoom->dwDataID))
			{
				str += PROPTAG(P_DataID);
				str += INT32TOSTR(pRoom->dwDataID);
				str += PROPTAG(P_ImageStartX);
				str += INT32TOSTR(pRoom->wImageStartX);
				str += PROPTAG(P_ImageStartY);
				str += INT32TOSTR(pRoom->wImageStartY);
			}
		}
		if (pRoom->dwOverheadDataID)
		{
			//Only save attached data if it's not a dangling pointer.
			if (g_pTheDB->Data.Exists(pRoom->dwOverheadDataID))
			{
				str += PROPTAG(P_OverheadDataID);
				str += INT32TOSTR(pRoom->dwOverheadDataID);
				str += PROPTAG(P_OverheadImageStartX);
				str += INT32TOSTR(pRoom->wOverheadImageStartX);
				str += PROPTAG(P_OverheadImageStartY);
				str += INT32TOSTR(pRoom->wOverheadImageStartY);
			}
		}

		//Process squares data.
		str += PROPTAG(P_Squares);
		UINT dwSize;
		{
			c4_Bytes *c4Squares = pRoom->PackSquares();
			const BYTE *pSquares = c4Squares->Contents();
			dwSize = c4Squares->Size();

			str += Base64::encode(pSquares,dwSize);

			delete c4Squares;
		}

		//Process tile lights data.
		str += PROPTAG(P_TileLights);
		{
			c4_Bytes *c4Tiles = pRoom->PackTileLights();
			const BYTE *pTiles = c4Tiles->Contents();
			dwSize = c4Tiles->Size();

			str += Base64::encode(pTiles,dwSize);

			delete c4Tiles;
		}

		//Extra vars (e.g. weather).
		UINT dwBufferSize;
		BYTE *pExtraVars = pRoom->ExtraVars.GetPackedBuffer(dwBufferSize);
		if (dwBufferSize > 4)   //null buffer
		{
			str += PROPTAG(P_ExtraVars);
			str += Base64::encode(pExtraVars,dwBufferSize-4);  //strip null UINT
		}
		delete[] pExtraVars;

		str += CLOSESTARTTAG;

		//Process room data lists.
		UINT dwIndex, dwAgentI, dwNumAgents;

		//Orbs
		COrbData *pOrb;
		COrbAgentData *pOrbAgent;
		dwSize = pRoom->orbs.size();
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			pOrb = pRoom->orbs[dwIndex];
			ASSERT(pRoom->IsValidColRow(pOrb->wX, pOrb->wY));
			str += STARTVPTAG(VP_Orbs, P_Type);
			str += INT32TOSTR(pOrb->eType);
			str += PROPTAG(P_X);
			str += INT32TOSTR(pOrb->wX);
			str += PROPTAG(P_Y);
			str += INT32TOSTR(pOrb->wY);
			str += CLOSESTARTTAG;

			dwNumAgents = pOrb->agents.size();
			for (dwAgentI=0; dwAgentI<dwNumAgents; ++dwAgentI)
			{
				pOrbAgent = pOrb->agents[dwAgentI];
				ASSERT(bIsValidOrbAgentType(pOrbAgent->action));
				ASSERT(pRoom->IsValidColRow(pOrbAgent->wX, pOrbAgent->wY));
				str += STARTVPTAG(VP_OrbAgents, P_Type);
				str += INT32TOSTR(pOrbAgent->action);
				str += PROPTAG(P_X);
				str += INT32TOSTR(pOrbAgent->wX);
				str += PROPTAG(P_Y);
				str += INT32TOSTR(pOrbAgent->wY);
				str += CLOSETAG;
			}

			str += ENDVPTAG(VP_Orbs);
		}

		//Monsters
		//Add Halph/Slayer entrances to monster list.
		CCoordSet::const_iterator enters;
		for (enters=pRoom->halphEnters.begin(); enters!=pRoom->halphEnters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_HALPH, enters->wX, enters->wY, false);
		}
		for (enters=pRoom->halph2Enters.begin(); enters!=pRoom->halph2Enters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_HALPH2, enters->wX, enters->wY, false);
		}
		for (enters=pRoom->slayerEnters.begin(); enters!=pRoom->slayerEnters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_SLAYER, enters->wX, enters->wY, false);
		}
		for (enters=pRoom->slayer2Enters.begin(); enters!=pRoom->slayer2Enters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_SLAYER2, enters->wX, enters->wY, false);
		}

		CMonster *pMonster = pRoom->pFirstMonster;
		while (pMonster)
		{
			ASSERT(pRoom->IsValidColRow(pMonster->wX, pMonster->wY));
			str += pMonster->ExportXML();
			pMonster = pMonster->pNext;
		}

		//Scrolls
		CScrollData *pScroll;
		dwSize = pRoom->Scrolls.size();
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			pScroll = pRoom->Scrolls[dwIndex];
			WSTRING const wMessage = (WSTRING)pScroll->ScrollText;

			ASSERT(pRoom->IsValidColRow(pScroll->wX, pScroll->wY));
			str += STARTVPTAG(VP_Scrolls, P_X);
			str += INT32TOSTR(pScroll->wX);
			str += PROPTAG(P_Y);
			str += INT32TOSTR(pScroll->wY);
			str += PROPTAG(P_Message);
			str += Base64::encode(wMessage);
			str += CLOSETAG;
		}

		//Exits.
		CExitData *pExit;
		CDbLevel *pLevel;
		UINT dwHoldID = 0;
		dwSize = pRoom->Exits.size();
		if (dwSize)
		{
			pLevel = g_pTheDB->Levels.GetByID(pRoom->dwLevelID, true);
			ASSERT(pLevel);
			dwHoldID = pLevel->dwHoldID;
			delete pLevel;
		}
		for (dwIndex=0; dwIndex<dwSize; ++dwIndex)
		{
			pExit = pRoom->Exits[dwIndex];

			ASSERT(pRoom->IsValidColRow(pExit->wLeft, pExit->wTop));
			ASSERT(pRoom->IsValidColRow(pExit->wRight, pExit->wBottom));
			str += STARTVPTAG(VP_Exits, P_EntranceID);
			str += INT32TOSTR(pExit->dwEntranceID);
			str += PROPTAG(P_Left);
			str += INT32TOSTR(pExit->wLeft);
			str += PROPTAG(P_Right);
			str += INT32TOSTR(pExit->wRight);
			str += PROPTAG(P_Top);
			str += INT32TOSTR(pExit->wTop);
			str += PROPTAG(P_Bottom);
			str += INT32TOSTR(pExit->wBottom);
			str += CLOSETAG;
		}

		//Checkpoints.
		for (CCoordSet::const_iterator checkpoint = pRoom->checkpoints.begin();
				checkpoint != pRoom->checkpoints.end(); ++checkpoint)
		{
			ASSERT(pRoom->IsValidColRow(checkpoint->wX, checkpoint->wY));
			str += STARTVPTAG(VP_Checkpoints, P_X);
			str += INT32TOSTR(checkpoint->wX);
			str += PROPTAG(P_Y);
			str += INT32TOSTR(checkpoint->wY);
			str += CLOSETAG;
		}

		str += ENDTAG(V_Rooms);
	}

	delete pRoom;
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

//*****************************************************************************
UINT CDbRooms::GetAuthorID(const UINT dwRoomID)
//Returns: the player ID who is the author of the hold this room is in.
{
	const UINT dwHoldID = g_pTheDB->Rooms.GetHoldIDForRoom(dwRoomID);
	if (!dwHoldID) return 0;
	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID);
	if (pHold)
	{
		const UINT dwAuthorID = pHold->dwPlayerID;
		delete pHold;
		return dwAuthorID;
	}
	return 0;
}

//*****************************************************************************
CDbRoom* CDbRooms::GetByCoords(
//Get a room by its coordinates.
//
//Params:
	const UINT dwLevelID,           //(in)   Level containing room.
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of room to get.
//
//Returns:
//Pointer to loaded room which caller must delete, or NULL if no matching room
//was found.
{
	const UINT dwRoomID = FindIDAtCoords(dwLevelID, dwRoomX, dwRoomY);
	if (!dwRoomID)
		return NULL;
	return GetByID(dwRoomID);
}

//*****************************************************************************
UINT CDbRooms::GetHoldIDForRoom(const UINT dwRoomID)
//Returns: Quick lookup of room's hold ID.
{
	const UINT dwLevelID = GetLevelIDForRoom(dwRoomID);
	if (!dwLevelID) return 0;	//not found

	return CDbLevels::GetHoldIDForLevel(dwLevelID);
}

//*****************************************************************************
UINT CDbRooms::GetLevelIDForRoom(const UINT dwRoomID)
//Returns: Quick lookup of room's level ID.
{
	if (!dwRoomID) return 0;

	//Find record with matching room ID.
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(dwRoomID, V_Rooms, RoomsView);
	if (dwRoomI == ROW_NO_MATCH)
		return 0;

	return UINT(p_LevelID(RoomsView[dwRoomI]));
}

//*****************************************************************************
UINT CDbRooms::FindIDAtCoords(
//Finds a room at specified coordinates.
//
//Params:
	const UINT dwLevelID,           //(in)   Level containing room.
	const UINT dwRoomX, const UINT dwRoomY) //(in)   Coords of room to find.
//
//Returns:
//RoomID of matching room, or 0 if not matching room was found.
{
	ASSERT(IsOpen());	//Ensure rooms view is open.

	CIDSet roomsInLevel = CDb::getRoomsInLevel(dwLevelID);
	c4_View RoomsView;

	//Scan through rooms in level to find matching room.
	for (CIDSet::const_iterator room = roomsInLevel.begin(); room != roomsInLevel.end(); ++room)
	{
		const UINT roomRowI = LookupRowByPrimaryKey(*room, V_Rooms, RoomsView);
		c4_RowRef row = RoomsView[roomRowI];
		if (dwRoomX == (UINT) p_RoomX(row) && dwRoomY == (UINT) p_RoomY(row))
			return p_RoomID(row); //Found it.
	}

	//No match.
	return 0;
}

//*****************************************************************************
CDbRoom* CDbRooms::GetNew()
//Get a new room object that will be added to database when it is updated.
//
//Returns:
//Pointer to new room.
{
	//After room object is updated, membership changes, so reset the flag.
	this->bIsMembershipLoaded = false;

	//Return new room object.
	CDbRoom *pRoom = new CDbRoom;
	pRoom->dwLevelID = this->dwFilterByLevelID;  //set to current level
	return pRoom;
}

//*****************************************************************************
void CDbRooms::FilterBy(
//Changes filter so that GetFirst() and GetNext() will return rooms for a
//specified level.
//
//Params:
	const UINT dwSetFilterByLevelID)   //(in)   Level ID to filter by.
													//Set to 0 for all rooms.
{
	if (dwSetFilterByLevelID != this->dwFilterByLevelID && this->bIsMembershipLoaded)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByLevelID = dwSetFilterByLevelID;
}

//*****************************************************************************
void CDbRooms::LogRoomsWithItem(const UINT wTile, const UINT wParam)
//Output a log of all rooms that contain the mentioned item.
{
	const UINT wLayer = TILE_LAYER[wTile];

	ASSERT(IsOpen());
	CDb db;

	const UINT dwLevelCount = g_pTheDB->Levels.GetViewSize();
	for (UINT dwLevelI = 0; dwLevelI < dwLevelCount; ++dwLevelI)
	{
		c4_RowRef row = GetRowRef(V_Levels, dwLevelI);
		CDbLevel *pLevel = db.Levels.GetByID(p_LevelID(row));
		ASSERT(pLevel);

		CIDSet roomIDs = CDb::getRoomsInLevel(pLevel->dwLevelID);
		for (CIDSet::const_iterator id=roomIDs.begin(); id!=roomIDs.end(); ++id)
		{
			CDbRoom *pRoom = GetByID(*id);
			ASSERT(pRoom);
			const UINT wArea = pRoom->CalcRoomArea();

			bool bFound = false;
			for (UINT wI=0; wI<wArea && !bFound; ++wI)
			{
				//Check this room tile for item.
				switch (wLayer)
				{
					case 0:
						if (static_cast<UINT>(pRoom->pszOSquares[wI]) == wTile)
						{
							const UINT wX = wI % pRoom->wRoomCols;
							const UINT wY = wI / pRoom->wRoomCols;
							switch (wTile)
							{
								case T_PRESSPLATE:
								{
									COrbData *pOrb = pRoom->GetOrbAtCoords(wX,wY);
									if (pOrb && static_cast<UINT>(pOrb->eType) == wParam)
										bFound = true;
								}
								break;
								default:
									bFound = true;
								break;
							}
						}
					break;
					case 1:
						if (pRoom->GetTSquare(wI) == wTile)
						{
							const UINT wX = wI % pRoom->wRoomCols;
							const UINT wY = wI / pRoom->wRoomCols;
							switch (wTile)
							{
								case T_ORB:
								{
									COrbData *pOrb = pRoom->GetOrbAtCoords(wX,wY);
									if (pOrb && static_cast<UINT>(pOrb->eType) == wParam)
										bFound = true;
								}
								break;
								default:
									if (pRoom->GetTParam(wI) == wParam)
										bFound = true;
								break;
							}
						}
					break;
					case 2:
					{
						CMonster *pMonster = pRoom->pMonsterSquares[wI];
						if (pMonster && pMonster->wType + M_OFFSET == wTile)
							bFound = true;
					}
					break;
					case 3:
						if (static_cast<UINT>(pRoom->pszFSquares[wI]) == wTile) {
							bFound = true;
						} else {
							const UINT wX = wI % pRoom->wRoomCols;
							const UINT wY = wI / pRoom->wRoomCols;
							const UINT wLightParam = pRoom->tileLights.GetAt(wX,wY);
							switch (wTile)
							{
								case T_LIGHT_CEILING:
									if ((wParam && wLightParam == wParam) || (!wParam && bIsLightTileValue(wLightParam)))
										bFound = true;
								break;
								case T_DARK_CEILING:
									if ((wParam && wLightParam == wParam) || (!wParam && bIsDarkTileValue(wLightParam)))
										bFound = true;
								break;
								case T_WALLLIGHT:
									if ((wParam && wLightParam == wParam) || (!wParam && bIsWallLightValue(wLightParam)))
										bFound = true;
								break;
								case T_CHECKPOINT:
									if (pRoom->checkpoints.has(wX,wY))
										bFound = true;
								default: break;
							}
						}
					break;

					default: ASSERT(!"Unsupported layer"); delete pRoom; return;
				}
			}
			if (bFound)
			{
				WSTRING wstr;
				wstr += (const WCHAR*)pLevel->NameText;
				wstr += wszColon;
				pRoom->GetLevelPositionDescription(wstr, true);

				string str = UnicodeToAscii(wstr);
				str += "\n";
				CFiles f;
				f.AppendUserLog(str.c_str());
			}
			delete pRoom;
		}
		delete pLevel;
	}
}

//
//CDbRooms private methods.
//

//*****************************************************************************
void CDbRooms::LoadMembership()
//Load the membership list with all room IDs.
{
	ASSERT(IsOpen());
	const UINT dwRoomCount = GetViewSize();

	//Each iteration gets a room ID and puts in membership list.
	this->MembershipIDs.clear();
	for (UINT dwRoomI = 0; dwRoomI < dwRoomCount; ++dwRoomI)
	{
		c4_RowRef row = GetRowRef(V_Rooms, dwRoomI);
		if (!this->dwFilterByLevelID || UINT(p_LevelID(row)) == this->dwFilterByLevelID)
			this->MembershipIDs += p_RoomID(row);
	}
	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//*****************************************************************************
CDbRoom::CDbRoom(const CDbRoom &Src, 
	const bool bCopyCurrentGame, //[default=true]
	const bool bCopyForEditor //[default=false]
)//Set pointers to NULL so Clear() won't try to delete them.
	: CDbBase()
	, pszOSquares(NULL), pszFSquares(NULL)
	, pFirstMonster(NULL), pLastMonster(NULL)
	, tLayer(NULL)
	, pMonsterSquares(NULL)
	, pCurrentGame(NULL)
//Constructor.
{
	for (int n=NumMovementTypes; n--; )
		this->pPathMap[n]=NULL;
	SetMembers(Src, true, bCopyCurrentGame, bCopyForEditor);
}

//
//CDbRoom protected methods.
//

//*****************************************************************************
CDbRoom::CDbRoom()
	//Set pointers to NULL so Clear() won't try to delete them.
	: CDbBase()
	, pszOSquares(NULL), pszFSquares(NULL)
	, pFirstMonster(NULL), pLastMonster(NULL)
	, tLayer(NULL)
	, pMonsterSquares(NULL)
	, pCurrentGame(NULL)
//Constructor.
{
	for (int n=0; n<NumMovementTypes; ++n)
		this->pPathMap[n]=NULL;
	Clear();
}

//*****************************************************************************
void CDbRoom::ClearPlotHistory()
//Resets flag stating that a plot(s) were made to the room.
{
	this->PlotsMade.clear();
	this->geometryChanges.clear();
	this->disabledLights.clear();
}

//
//CDbRoom public methods.
//

//*****************************************************************************
CDbRoom::~CDbRoom()
//Destructor.
{
	Clear();
}

//*****************************************************************************
bool CDbRoom::CanEndHoldHere() const
//Returns: whether it is possible for the player to end the hold from this room.
{
	//Look for end hold stairs or a script that ends the hold.
	UINT wIndex;
	for (wIndex=0; wIndex<this->Exits.size(); ++wIndex)
	{
		if (!this->Exits[wIndex]->dwEntranceID)
			return true;	//end hold stairs found
	}

	//No end hold stairs found if we reached here --
	// Look for a script that can end the hold.
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			for (wIndex=0; wIndex<pCharacter->commands.size(); ++wIndex)
			{
				CCharacterCommand& c = pCharacter->commands[wIndex];
				if (c.command == CCharacterCommand::CC_LevelEntrance &&
						c.x == END_HOLD_LEVEL_ID)
					return true; //it might be possible to end the hold here

				//Note that a dangling level exit ID will also technically end the
				//hold, but we're not going to worry about that now because it's a minor issue
				//and usually only found while a hold is in development.
			}
		}
		pMonster = pMonster->pNext;
	}

	return false;
}

//*****************************************************************************
CDbLevel* CDbRoom::GetLevel()
//Get level associated with this room.
//
//Returns:
//Pointer to loaded level which caller must delete, or NULL if no matching level
//was found.
const
{
	CDbLevel *pLevel = new CDbLevel();
	if (pLevel)
	{
		if (!pLevel->Load(this->dwLevelID))
		{
			delete pLevel;
			pLevel=NULL;
		}
	}
	return pLevel;
}

//*****************************************************************************
bool CDbRoom::Load(
//Loads a room from database into this object.
//
//Params:
	const UINT dwLoadRoomID,  //(in) RoomID of room to load.
	const bool bQuick) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {

	Clear();

	//Find record with matching room ID.
	ASSERT(IsOpen());
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(dwLoadRoomID, V_Rooms, RoomsView);
	if (dwRoomI == ROW_NO_MATCH) throw CException("CDbRoom::Load");
	c4_RowRef row = RoomsView[dwRoomI];

	//Load in props from Rooms record.
	this->dwRoomID = (UINT) (p_RoomID(row));
	ASSERT(this->dwRoomID == dwLoadRoomID);
	this->dwLevelID = (UINT) (p_LevelID(row));
	this->dwRoomX = (UINT) (p_RoomX(row));
	this->dwRoomY = (UINT) (p_RoomY(row));
	this->wRoomCols = (UINT) (p_RoomCols(row));
	this->wRoomRows = (UINT) (p_RoomRows(row));
	this->bIsRequired = (p_IsRequired(row) != 0);
	this->bIsSecret = (p_IsSecret(row) != 0);
	this->dwDataID = (UINT) (p_DataID(row));
	this->wImageStartX = (UINT) (p_ImageStartX(row));
	this->wImageStartY = (UINT) (p_ImageStartY(row));
	this->dwOverheadDataID = (UINT) (p_OverheadDataID(row));
	this->wOverheadImageStartX = (UINT) (p_OverheadImageStartX(row));
	this->wOverheadImageStartY = (UINT) (p_OverheadImageStartY(row));

	this->bPartialLoad = bQuick;
	if (!bQuick)
	{
		c4_Bytes StyleNameBytes = p_StyleName(row);
		GetWString(this->style, StyleNameBytes);

		c4_Bytes SquaresBytes = p_Squares(row);
		if (!UnpackSquares(SquaresBytes.Contents(), SquaresBytes.Size()))
			throw CException("CDbRoom::Load");
		InitRoomStats();

		c4_Bytes tileLightBytes = p_TileLights(row);
		if (!UnpackTileLights(tileLightBytes.Contents(), tileLightBytes.Size()))
			throw CException("CDbRoom::Load");

		//Load orbs for this room.
		c4_View OrbsView = p_Orbs(row);
		if (!LoadOrbs(OrbsView)) throw CException("CDbRoom::Load");

		//Load monsters for this room
		c4_View MonstersView = p_Monsters(row);
		if (!LoadMonsters(MonstersView)) throw CException("CDbRoom::Load");

		//Load scrolls for this room
		c4_View ScrollsView = p_Scrolls(row);
		if (!LoadScrolls(ScrollsView)) throw CException("CDbRoom::Load");

		//Load exits for this room.
		c4_View ExitsView = p_Exits(row);
		if (!LoadExits(ExitsView)) throw CException("CDbRoom::Load");

		//Load checkpoints for this room.
		c4_View CheckpointsView = p_Checkpoints(row);
		if (!LoadCheckpoints(CheckpointsView)) throw CException("CDbRoom::Load");

		this->ExtraVars = p_ExtraVars(row);
		SetMembersFromExtraVars();
	}

	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CDbRoom::LoadTiles()
//Loads only tile data (speed optimization).
{
	ASSERT(this->bPartialLoad); //shouldn't call if all members were already loaded

	//Find record with matching room ID.
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(this->dwRoomID, V_Rooms, RoomsView);
	ASSERT(dwRoomI != ROW_NO_MATCH);

	c4_Bytes SquaresBytes = p_Squares(RoomsView[dwRoomI]);
	return UnpackSquares(SquaresBytes.Contents(), SquaresBytes.Size());
}

//*****************************************************************************
UINT CDbRoom::GetImportCharacterSpeechID()
//Returns: pointer to monster character currently being imported, otherwise NULL
{
	if (!pImportMonster)
		return 0;

	CMonster *pMonster = GetMonsterAtSquare(pImportMonster->wX, pImportMonster->wY);
	if (pMonster && pMonster->wType == M_CHARACTER)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		return pCharacter->GetNextSpeechID();
	}
	return 0;
}

//*****************************************************************************
void CDbRoom::ResetForImport()
//Resets static vars used in import process.
{
	delete pImportScroll; pImportScroll = NULL;
	delete pImportMonster; pImportMonster = NULL;
	delete pImportOrbAgent; pImportOrbAgent = NULL;
	delete pImportOrb; pImportOrb = NULL;
	delete pImportPiece; pImportPiece = NULL;
	delete pImportExit; pImportExit = NULL;
}

//*****************************************************************************
MESSAGE_ID CDbRoom::SetProperty(
//Used during XML import of language modules.
	const PROPTYPE pType,
	const char** atts,
	CImportInfo &/*info*/)
{
	switch (pType)
	{
		case P_Message:
			if (pImportScroll)
			{
				CScrollData *pScroll = GetScrollAtSquare(pImportScroll->wX, pImportScroll->wY);
				if (pScroll)
					pScroll->ScrollText.ImportText(atts);
			}
		break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbRoom::SetProperty(
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
	switch (pType)
	{
		case P_RoomID:
		{
			this->dwRoomID = convertToUINT(str);
			if (!this->dwRoomID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			PrimaryKeyMap::const_iterator localID = info.RoomIDMap.find(this->dwRoomID);
			if (localID != info.RoomIDMap.end())
				//Error - this room should not have been imported yet
				return MID_FileCorrupted;

			//Look up room in the DB.
			const UINT dwLocalRoomID = GetLocalID();
			if (dwLocalRoomID)
			{
				//Room found in DB.
				info.RoomIDMap[this->dwRoomID] = dwLocalRoomID;
				this->dwRoomID = dwLocalRoomID;
				bSaveRecord = false;

				if (info.typeBeingImported == CImportInfo::LanguageMod)
				{
					//A language module is being imported.
					//Load the real room object into this record.
					CDbRoom *pLocalRoom = g_pTheDB->Rooms.GetByID(dwLocalRoomID);
					ASSERT(pLocalRoom);
					SetMembers(*pLocalRoom, true);
					delete pLocalRoom;
				}
			} else {
				//Don't save room unless a hold is being imported right now.
				if (info.typeBeingImported != CImportInfo::Hold ||
						info.bImportingSavedGames)
					bSaveRecord = false;

				if (bSaveRecord)
				{
					//Add a new record to the DB.
					const UINT dwOldLocalID = this->dwRoomID;
					this->dwRoomID = 0;
					Update();
					info.RoomIDMap[dwOldLocalID] = this->dwRoomID;
				} else {
					//This room is being ignored.
					//(It's probably a GUID reference to a non-existant hold/level.)
					info.RoomIDMap[this->dwRoomID] = 0;   //skip records with refs to this room ID
				}
			}
			break;
		}
		case P_LevelID:
		{
			this->dwLevelID = convertToUINT(str);
			if (!this->dwLevelID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			PrimaryKeyMap::const_iterator localID = info.LevelIDMap.find(this->dwLevelID);
			if (localID == info.LevelIDMap.end())
				return MID_LevelNotFound;  //can't load a room w/o its level
			this->dwLevelID = localID->second;
			if (!this->dwLevelID)
			{
				//Records for this level are being ignored.  Don't save this room.
				bSaveRecord = false;
			}
			break;
		}
		case P_RoomX:
			this->dwRoomX = convertToUINT(str);
			break;
		case P_RoomY:
			this->dwRoomY = convertToUINT(str);
			if (bSaveRecord || info.typeBeingImported == CImportInfo::LanguageMod)
			{
				//Update room's "level membership" to reflect local level ID.
				ASSERT(this->dwLevelID);
				this->dwRoomY = (this->dwLevelID * 100) + (this->dwRoomY % 100);
			}
			break;
		case P_RoomCols:
			this->wRoomCols = convertToUINT(str);
			break;
		case P_RoomRows:
			this->wRoomRows = convertToUINT(str);
			break;
		case P_Style:  //deprecated since 3.0
		{
			ASSERT(info.wVersion < 301);
			UINT wStyle = convertToUINT(str);
			if (wStyle > 9) wStyle = 1;

			if (info.wVersion < 200 && info.bHasAEMod) {
				//When AE mod is installed, upgrade to those style names
				static const char ae_styleNames[9][10] = {
					"Cavern", "Cliffs", "Slime",
					"Voids AE", "Ice", "Lava",
					"Palace", "Catacombs", "Sanctum"
				};
				AsciiToUnicode(ae_styleNames[wStyle-1], this->style);
			} else {
				//Convert old style number to current style name.
				static const UINT wCurrentNumberOfStyles = 12;
				static const char styleNames[wCurrentNumberOfStyles][12] = {
					"Foundation","Deep Spaces","Iceworks",
					"Aboveground", "City", "Fortress",
					"Beach", "Forest", "Swamp",
					"Badlands", "Caldera", "Greenhouse"
				};

				//Styles from 1.6 and 2.0 are converted differently.
				static const UINT styleMapping[2][9] = {{1, 2, 4, 9, 3, 11, 5, 12, 6},{1, 2, 3, 9, 8, 11, 5, 12, 6}};
				const UINT wGameIndex = info.wVersion >= 200 ? 1 : 0;

				const UINT newStyle = styleMapping[wGameIndex][wStyle-1] - 1;
				ASSERT(newStyle < wCurrentNumberOfStyles);

				AsciiToUnicode(styleNames[newStyle], this->style);
			}
		}
		break;
		case P_StyleName:
			ASSERT(info.wVersion >= 301);
			Base64::decode(str, this->style);
			info.roomStyles.insert(this->style); //maintain set of room styles imported
			break;
		case P_IsRequired:
			this->bIsRequired = convertIntStrToBool(str);
			break;
		case P_IsSecret:
			this->bIsSecret = convertIntStrToBool(str);
			break;
		case P_DataID:
			this->dwDataID = convertToUINT(str);
			if (this->dwDataID)
			{
				//Set to local ID.
				PrimaryKeyMap::const_iterator localID = info.DataIDMap.find(this->dwDataID);
				if (localID == info.DataIDMap.end())
					return MID_FileCorrupted;  //record should have been loaded already
				this->dwDataID = localID->second;
			}
			break;
		case P_ImageStartX:
			this->wImageStartX = convertToUINT(str);
			break;
		case P_ImageStartY:
			this->wImageStartY = convertToUINT(str);
			break;
		case P_OverheadDataID:
			this->dwOverheadDataID = convertToUINT(str);
			if (this->dwOverheadDataID)
			{
				//Set to local ID.
				PrimaryKeyMap::const_iterator localID = info.DataIDMap.find(this->dwOverheadDataID);
				if (localID == info.DataIDMap.end())
					return MID_FileCorrupted;  //record should have been loaded already
				this->dwOverheadDataID = localID->second;
			}
			break;
		case P_OverheadImageStartX:
			this->wOverheadImageStartX = convertToUINT(str);
			break;
		case P_OverheadImageStartY:
			this->wOverheadImageStartY = convertToUINT(str);
			break;
		case P_Squares:
		{
			//Must be called following calls for P_RoomCols and P_RoomRows.
			ASSERT(CalcRoomArea() > 0);
			BYTE *data;
			const UINT size = Base64::decode(str,data);
			const bool bRes = UnpackSquares((const BYTE*)data, size);
			delete[] data;
			if (!bRes) return MID_FileCorrupted;
			InitRoomStats();
			break;
		}
		case P_TileLights:
		{
			//Must be called following calls for P_RoomCols and P_RoomRows.
			ASSERT(CalcRoomArea() > 0);
			BYTE *data;
			const UINT size = Base64::decode(str,data);
			const bool bRes = UnpackTileLights((const BYTE*)data, size);
			delete[] data;
			if (!bRes) return MID_FileCorrupted;
			break;
		}
		case P_ExtraVars:
		{
			BYTE *data;
			Base64::decode(str,data);
			this->ExtraVars = (const BYTE*)data;
			delete[] data;
			SetMembersFromExtraVars();
			break;
		}
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbRoom::SetProperty(
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
	CImportInfo &info)   //(in/out) Import data
{
	switch (vpType)
	{
		case VP_OrbAgents:
			ASSERT(pImportOrb);
			ASSERT(pType == P_Start || pImportOrbAgent);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportOrbAgent);
					pImportOrbAgent = new COrbAgentData;
					break;
				case P_Type:
					pImportOrbAgent->action = static_cast<OrbAgentType>(convertToInt(str));
					if (!bIsValidOrbAgentType(pImportOrbAgent->action))
					{
						delete pImportOrb;
						pImportOrb = NULL;
						delete pImportOrbAgent;
						pImportOrbAgent = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_X:
					pImportOrbAgent->wX = convertToUINT(str);
					if (pImportOrbAgent->wX >= this->wRoomCols)
					{
						delete pImportOrb;
						pImportOrb = NULL;
						delete pImportOrbAgent;
						pImportOrbAgent = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Y:
					pImportOrbAgent->wY = convertToUINT(str);
					if (pImportOrbAgent->wY >= this->wRoomRows)
					{
						delete pImportOrb;
						pImportOrb = NULL;
						delete pImportOrbAgent;
						pImportOrbAgent = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_End:
					//Finish processing
					pImportOrb->AddAgent(pImportOrbAgent);
					pImportOrbAgent = NULL;
					break;
				default:
					delete pImportOrb;
					pImportOrb = NULL;
					delete pImportOrbAgent;
					pImportOrbAgent = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Orbs:
		  ASSERT(pType == P_Start || pImportOrb);
			ASSERT(pType == P_Start || !pImportOrbAgent);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportOrb);
					pImportOrb = new COrbData;
					break;
				case P_Type:
					pImportOrb->eType = static_cast<OrbType>(convertToInt(str));
					if (!bIsValidOrbType(pImportOrb->eType))
					{
						delete pImportOrb;
						pImportOrb = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_X:
					pImportOrb->wX = convertToUINT(str);
					if (pImportOrb->wX >= this->wRoomCols)
					{
						delete pImportOrb;
						pImportOrb = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Y:
					pImportOrb->wY = convertToUINT(str);
					if (pImportOrb->wY >= this->wRoomRows)
					{
						delete pImportOrb;
						pImportOrb = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_End:
					//Finish processing
					if (info.wVersion < 200)
					{
						//1.5 orb bug fix: KDD has at least one room with two orb records on the same square.
						//If found, then delete the second one and retain the first.
						//This will produce behavior consistent with 1.5/1.6.
						COrbData *pOrb = GetOrbAtCoords(pImportOrb->wX, pImportOrb->wY);
						if (pOrb)
						{
							delete pImportOrb;
							pImportOrb = NULL;
							break;
						}
					}
					if (!AddOrb(pImportOrb))
						delete pImportOrb; //else: don't delete
					pImportOrb = NULL;
					break;
				default:
					delete pImportOrb;
					pImportOrb = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Monsters:
			ASSERT(pType == P_Start || pImportMonster);
			switch (pType)
			{
				case P_Start:
				{
					ASSERT(!pImportMonster);
					pImportMonster = CMonsterFactory::GetNewMonster(M_MIMIC);
					break;
				}
				case P_Type:
					pImportMonster->wType = convertToUINT(str);
					break;
				case P_X:
					pImportMonster->wX = convertToUINT(str);
					if (pImportMonster->wX >= this->wRoomCols)
					{
						delete pImportMonster;
						pImportMonster = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Y:
					pImportMonster->wY = convertToUINT(str);
					if (pImportMonster->wY >= this->wRoomRows)
					{
						delete pImportMonster;
						pImportMonster = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_O:
					pImportMonster->wO = convertToUINT(str);
					if (pImportMonster->wO >= ORIENTATION_COUNT)
					{
						delete pImportMonster;
						pImportMonster = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_IsFirstTurn:
					pImportMonster->bIsFirstTurn = convertIntStrToBool(str);
					break;
				case P_ProcessSequence:
					pImportMonster->wProcessSequence = convertToUINT(str);
					break;
				case P_ExtraVars:
				{
					BYTE *data;
					Base64::decode(str,data);
					pImportMonster->ExtraVars.UseOldFormat(info.wVersion < 201);
					pImportMonster->ExtraVars = (const BYTE*)data;
					delete[] data;
					if (pImportMonster->wType == M_CHARACTER)
					{
						//Can't dynamically cast pImportMonster to a CCharacter
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*,
								CMonsterFactory::GetNewMonster(M_CHARACTER));
						pCharacter->ExtraVars = pImportMonster->ExtraVars;
						pCharacter->dwScriptID = 1; //avoids assertion
						const MESSAGE_ID val = pCharacter->ImportSpeech(info);
						if (val != MID_ImportSuccessful) return val;
						pImportMonster->ExtraVars = pCharacter->ExtraVars;
						delete pCharacter;
					}
					break;
				}
				case P_End:
					//Finish processing
					if (info.typeBeingImported == CImportInfo::LanguageMod)
						delete pImportMonster; //don't re-add existing room members
					else
						LinkMonster(pImportMonster, !DoesMonsterEnterRoomLater(
							pImportMonster->wX, pImportMonster->wY,
							pImportMonster->wType));
					pImportMonster = NULL;
					break;
				default:
					delete pImportMonster;
					pImportMonster = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Pieces:
			ASSERT(pImportMonster);
			ASSERT(pType == P_Start || pImportPiece);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportPiece);
					pImportPiece = new CMonsterPiece(pImportMonster);
					pImportPiece->wType = pImportMonster->wType;
					break;
				case P_Type:
					pImportPiece->wTileNo = convertToUINT(str);
					break;
				case P_X:
					pImportPiece->wX = convertToUINT(str);
					if (pImportPiece->wX >= this->wRoomCols)
					{
						delete pImportMonster;
						pImportMonster = NULL;
						delete pImportPiece;
						pImportPiece = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Y:
					pImportPiece->wY = convertToUINT(str);
					if (pImportPiece->wY >= this->wRoomRows)
					{
						delete pImportMonster;
						pImportMonster = NULL;
						delete pImportPiece;
						pImportPiece = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_End:
					//Finish processing
					ASSERT(!this->pMonsterSquares[ARRAYINDEX(pImportPiece->wX,
							pImportPiece->wY)]);
					this->pMonsterSquares[ARRAYINDEX(pImportPiece->wX,
							pImportPiece->wY)] = pImportPiece;
					pImportMonster->Pieces.push_back(pImportPiece);
					pImportPiece = NULL;
					break;
				default:
					delete pImportMonster;
					pImportMonster = NULL;
					delete pImportPiece;
					pImportPiece = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Scrolls:
			ASSERT(pType == P_Start || pImportScroll);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportScroll);
					pImportScroll = new CScrollData;
					break;
				case P_X:
					pImportScroll->wX = convertToUINT(str);
					if (pImportScroll->wX >= this->wRoomCols)
					{
						delete pImportScroll;
						pImportScroll = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Y:
					pImportScroll->wY = convertToUINT(str);
					if (pImportScroll->wY >= this->wRoomRows)
					{
						delete pImportScroll;
						pImportScroll = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Message:
				{
					WSTRING data;
					Base64::decode(str,data);
					pImportScroll->ScrollText = data.c_str();
					break;
				}
				case P_End:
					//Finish processing
					if (info.typeBeingImported == CImportInfo::LanguageMod)
						delete pImportScroll; //don't re-add existing room members
					else if (!AddScroll(pImportScroll))
						delete pImportScroll;  //else: don't delete pImportScroll
					pImportScroll = NULL;
					break;
				default:
					delete pImportScroll;
					pImportScroll = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Exits:
			ASSERT(pType == P_Start || pImportExit);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportExit);
					pImportExit = new CExitData;
					break;
				case P_EntranceID:
					pImportExit->dwEntranceID = convertToUINT(str);
					break;
				case P_Left:
					pImportExit->wLeft = convertToUINT(str);
					if (pImportExit->wLeft >= this->wRoomCols)
					{
						delete pImportExit;
						pImportExit = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Right:
					pImportExit->wRight = convertToUINT(str);
					if (pImportExit->wRight >= this->wRoomCols)
					{
						delete pImportExit;
						pImportExit = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Top:
					pImportExit->wTop = convertToUINT(str);
					if (pImportExit->wTop >= this->wRoomRows)
					{
						delete pImportExit;
						pImportExit = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_Bottom:
					pImportExit->wBottom = convertToUINT(str);
					if (pImportExit->wBottom >= this->wRoomRows)
					{
						delete pImportExit;
						pImportExit = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_End:
					//Finish processing
					if (!AddExit(pImportExit))
						delete pImportExit;  //else: don't delete pImportExit
					pImportExit = NULL;
					break;
				//Backwards compatibility:
				//1.6 Format
			case P_LevelID:
					//Change this LevelID to the EntranceID for the level's main entrance.
					//IDs will be matched to local ones later on completion of import
					pImportExit->dwEntranceID = convertToUINT(str);
					if (pImportExit->dwEntranceID)
						pImportExit->dwEntranceID += DROD1_6EXITFORMAT;
					break;

				default:
					delete pImportExit;
					pImportExit = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Checkpoints:
			switch (pType)
			{
				case P_Start:
					break;
				case P_X:
					importCheckpoint.wX = convertToUINT(str);
					if (importCheckpoint.wX >= this->wRoomCols)
						return MID_FileCorrupted;
					break;
				case P_Y:
					importCheckpoint.wY = convertToUINT(str);
					if (importCheckpoint.wY >= this->wRoomRows)
						return MID_FileCorrupted;
					break;
				case P_End:
					//Finish processing
					this->checkpoints.insert(importCheckpoint);
					break;
				default:
					return MID_FileCorrupted;
			}
			break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbRoom::Update()
//Updates database with room.
//
//Returns: true if successful, else false.
{
	if (this->bPartialLoad)
	{
		ASSERT(!"CDbRoom::Update -- partial load"); //don't try to update partially-loaded records
		return false;
	}

	g_pTheDB->Rooms.ResetMembership();
	if (this->dwRoomID == 0)
	{
		//Insert a new room.
		return UpdateNew();
	}

	//Update existing room.
	return UpdateExisting();
}

//*****************************************************************************
void CDbRoom::UpdatePathMapAt(const UINT wX, const UINT wY)
{
	ASSERT(IsValidColRow(wX,wY));
	for (int eMovement=0; eMovement<NumMovementTypes; ++eMovement)
		if (this->pPathMap[eMovement])
		{
			this->pPathMap[eMovement]->SetSquare(wX, wY, GetSquarePathMapObstacles(
					wX, wY, (MovementType)eMovement));
		}
}

//*****************************************************************************
void CDbRoom::RecalcStationPaths()
//Tells stations their path maps must be updated before next use.
{
	for (UINT i=this->stations.size(); i--; )
		this->stations[i]->RecalcPathmap();
}

//*****************************************************************************
void CDbRoom::ReevalBriarNear(
//Alert briar structures that room geometry has changed at (x,y).
//
//Params:
	const UINT wX, const UINT wY, //(in) Coords for square plotted
	const UINT wTileNo)           //(in) New tile
{
	this->briars.plotted(wX,wY, wTileNo);
}

//*****************************************************************************
void CDbRoom::ReflectSquare(const bool bHoriz, UINT &wSquare) const
//When reflecting the room, some objects need to be modified.
//Only call on o- and t-layer values, not t-params or monsters, etc.
{
	switch (wSquare)
	{
		//Arrows.
		case T_ARROW_N: if (!bHoriz) wSquare = T_ARROW_S; break;
		case T_ARROW_NE: wSquare = (bHoriz ? T_ARROW_NW : T_ARROW_SE); break;
		case T_ARROW_E: if (bHoriz) wSquare = T_ARROW_W; break;
		case T_ARROW_SE: wSquare = (bHoriz ? T_ARROW_SW : T_ARROW_NE); break;
		case T_ARROW_S: if (!bHoriz) wSquare = T_ARROW_N; break;
		case T_ARROW_SW: wSquare = (bHoriz ? T_ARROW_SE : T_ARROW_NW); break;
		case T_ARROW_W: if (bHoriz) wSquare = T_ARROW_E; break;
		case T_ARROW_NW: wSquare = (bHoriz ? T_ARROW_NE : T_ARROW_SW); break;

		case T_TUNNEL_N: if (!bHoriz) wSquare = T_TUNNEL_S; break;
		case T_TUNNEL_S: if (!bHoriz) wSquare = T_TUNNEL_N; break;
		case T_TUNNEL_E: if (bHoriz) wSquare = T_TUNNEL_W; break;
		case T_TUNNEL_W: if (bHoriz) wSquare = T_TUNNEL_E; break;

		case T_ARROW_OFF_N: if (!bHoriz) wSquare = T_ARROW_OFF_S; break;
		case T_ARROW_OFF_NE: wSquare = (bHoriz ? T_ARROW_OFF_NW : T_ARROW_OFF_SE); break;
		case T_ARROW_OFF_E: if (bHoriz) wSquare = T_ARROW_OFF_W; break;
		case T_ARROW_OFF_SE: wSquare = (bHoriz ? T_ARROW_OFF_SW : T_ARROW_OFF_NE); break;
		case T_ARROW_OFF_S: if (!bHoriz) wSquare = T_ARROW_OFF_N; break;
		case T_ARROW_OFF_SW: wSquare = (bHoriz ? T_ARROW_OFF_SE : T_ARROW_OFF_NW); break;
		case T_ARROW_OFF_W: if (bHoriz) wSquare = T_ARROW_OFF_E; break;
		case T_ARROW_OFF_NW: wSquare = (bHoriz ? T_ARROW_OFF_NE : T_ARROW_OFF_SW); break;

		default:
			//Reverse stair type when turned upside down.
			if (!bHoriz && bIsStairs(wSquare))
				wSquare = (wSquare == T_STAIRS ? T_STAIRS_UP : T_STAIRS);
		break;
	}
}

//*****************************************************************************
void CDbRoom::ReflectX()
//Reflects everything in the room in the X direction.
{
	//Reflect room tiles.
	UINT wX, wRefX, wY, wSize;
	UINT wSquare, wSquare2;
	vector<UINT> tParam(this->wRoomCols);
	const UINT reflectionOffset = this->wRoomCols-1;
	for (wY=this->wRoomRows; wY--; )
	{
		//Reflect t-layer directional params first.
		for (wX=this->wRoomCols; wX--; )
			tParam[wX] = GetTParam(wX, wY);  //need to make a static copy

		for (wX=0; wX<this->wRoomCols; )
		{
			if (GetTSquare(wX, wY) != T_OBSTACLE) {++wX; continue;}
			const BYTE obType = calcObstacleType(tParam[wX]);
			ASSERT(obType);
			wRefX = wX;//starting point of obstacle
			wSize = 0; //dimension of obstacle
			while (++wX < this->wRoomCols)
			{
				if (GetTSquare(wX, wY) != T_OBSTACLE) break;
				if (bObstacleLeft(tParam[wX])) break;
				if (calcObstacleType(tParam[wX]) != obType) break;
				++wSize;
			}
			if (wSize)
			{
				//Reverse obstacle boundaries.
				SetTParam(wRefX, wY, tParam[wRefX] & ~OBSTACLE_LEFT);  //remove Left marker
				SetTParam(wRefX+wSize, wY, tParam[wRefX+wSize] | OBSTACLE_LEFT);   //add Left marker
			}
		}

		for (wX=this->wRoomCols/2; wX--; )
		{
			wRefX = reflectionOffset - wX;
			wSquare = GetOSquare(wX, wY);
			wSquare2 = GetOSquare(wRefX, wY);
			ReflectSquare(true, wSquare);
			ReflectSquare(true, wSquare2);
			this->pszOSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszOSquares[ARRAYINDEX(wRefX, wY)] = wSquare;

			wSquare = this->coveredOSquares.GetAt(wX, wY);
			wSquare2 = this->coveredOSquares.GetAt(wRefX, wY);
			this->coveredOSquares.Add(wX, wY, wSquare2);
			this->coveredOSquares.Add(wRefX, wY, wSquare);

			wSquare = this->tileLights.GetAt(wX, wY);
			wSquare2 = this->tileLights.GetAt(wRefX, wY);
			if (wSquare2)
				this->tileLights.Add(wX, wY, wSquare2);
			else this->tileLights.Remove(wX, wY);
			if (wSquare)
				this->tileLights.Add(wRefX, wY, wSquare);
			else this->tileLights.Remove(wRefX, wY);

			wSquare = this->pressurePlateIndex.GetAt(wX, wY);
			wSquare2 = this->pressurePlateIndex.GetAt(wRefX, wY);
			if (wSquare2)
				this->pressurePlateIndex.Add(wX, wY, wSquare2);
			else this->pressurePlateIndex.Remove(wX, wY);
			if (wSquare)
				this->pressurePlateIndex.Add(wRefX, wY, wSquare);
			else this->pressurePlateIndex.Remove(wRefX, wY);

			wSquare = GetFSquare(wX, wY);
			wSquare2 = GetFSquare(wRefX, wY);
			ReflectSquare(true, wSquare);
			ReflectSquare(true, wSquare2);
			this->pszFSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszFSquares[ARRAYINDEX(wRefX, wY)] = wSquare;

			swapTLayer(wX, wY, wRefX, wY);
		}
	}

	//Reflect platforms.
	for (vector<CPlatform*>::const_iterator platformIter = this->platforms.begin();
			platformIter != this->platforms.end(); ++platformIter)
	{
		CPlatform *pPlatform = *platformIter;
		pPlatform->ReflectX(this);
	}

	//Reflect monsters.
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pMonster->ReflectX(this);
		pMonster = pMonster->pNext;
	}

	//Reflect orbs.
	for (vector<COrbData*>::const_iterator orb=this->orbs.begin();
			orb != this->orbs.end(); ++orb)
	{
		COrbData *pOrb = *orb;
		pOrb->wX = reflectionOffset - pOrb->wX;
		//Reflect orb agents.
		for (vector<COrbAgentData*>::const_iterator agent=pOrb->agents.begin();
				agent != pOrb->agents.end(); ++agent)
		{
			(*agent)->wX = reflectionOffset - (*agent)->wX;
		}
		//Reflect pressure plate sets.
		CCoordSet coords;
		for (CCoordSet::const_iterator coord=pOrb->tiles.begin(); coord!=pOrb->tiles.end(); ++coord)
			coords.insert(reflectionOffset - coord->wX, coord->wY);
		pOrb->tiles = coords;
	}

	//Reflect scrolls.
	for (vector<CScrollData*>::const_iterator scroll=this->Scrolls.begin();
			scroll != this->Scrolls.end(); ++scroll)
	{
		(*scroll)->wX = reflectionOffset - (*scroll)->wX;
	}

	//Reflect exits.
	for (vector<CExitData*>::const_iterator stair=this->Exits.begin();
			stair != this->Exits.end(); ++stair)
	{
		CExitData* pStair = *stair;
		const UINT oldwLeft = pStair->wLeft;
		pStair->wLeft = reflectionOffset - pStair->wRight;
		pStair->wRight = reflectionOffset - oldwLeft;
	}

	//Reflect checkpoints.
	CCoordSet::const_iterator coord;
	CCoordSet coords;
	for (coord=this->checkpoints.begin(); coord!=this->checkpoints.end(); ++coord)
		coords.insert(reflectionOffset - coord->wX, coord->wY);
	this->checkpoints = coords;
	coords.clear();

	//Reflect Halph/Slayer.
	for (coord=this->halphEnters.begin(); coord!=this->halphEnters.end(); ++coord)
		coords.insert(reflectionOffset - coord->wX, coord->wY);
	this->halphEnters = coords;
	coords.clear();
	for (coord=this->halph2Enters.begin(); coord!=this->halph2Enters.end(); ++coord)
		coords.insert(reflectionOffset - coord->wX, coord->wY);
	this->halph2Enters = coords;
	coords.clear();

	for (coord=this->slayerEnters.begin(); coord!=this->slayerEnters.end(); ++coord)
		coords.insert(reflectionOffset - coord->wX, coord->wY);
	this->slayerEnters = coords;
	coords.clear();
	for (coord=this->slayer2Enters.begin(); coord!=this->slayer2Enters.end(); ++coord)
		coords.insert(reflectionOffset - coord->wX, coord->wY);
	this->slayer2Enters = coords;

	//ignore in-game vars
}

//*****************************************************************************
void CDbRoom::ReflectY()
//Reflects everything in the room in the Y direction.
{
	//Reflect room tiles.
	UINT wX, wY, wRefY, wSize;
	UINT wSquare, wSquare2;
	vector<UINT> tParam(this->wRoomRows);
	const UINT reflectionOffset = this->wRoomRows-1;
	for (wX=this->wRoomCols; wX--; )
	{
		//Reflect t-layer directional params first.
		for (wY=this->wRoomRows; wY--; )
			tParam[wY] = GetTParam(wX, wY);  //need to make a static copy

		for (wY=0; wY<this->wRoomRows; )
		{
			if (GetTSquare(wX, wY) != T_OBSTACLE) {++wY; continue;}
			const BYTE obType = calcObstacleType(tParam[wY]);
			ASSERT(obType);
			wRefY = wY;//starting point of obstacle
			wSize = 0; //dimension of obstacle
			while (++wY < this->wRoomRows)
			{
				if (GetTSquare(wX, wY) != T_OBSTACLE) break;
				if (bObstacleTop(tParam[wY])) break;
				if (calcObstacleType(tParam[wY]) != obType) break;
				++wSize;
			}
			if (wSize)
			{
				//Reverse obstacle boundaries.
				SetTParam(wX, wRefY, tParam[wRefY] & ~OBSTACLE_TOP);  //remove Top marker
				SetTParam(wX, wRefY+wSize, tParam[wRefY+wSize] | OBSTACLE_TOP);   //add Top marker
			}
		}

		for (wY=this->wRoomRows/2; wY--; )
		{
			wRefY = reflectionOffset - wY;
			wSquare = GetOSquare(wX, wY);
			wSquare2 = GetOSquare(wX, wRefY);
			ReflectSquare(false, wSquare);
			ReflectSquare(false, wSquare2);
			this->pszOSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszOSquares[ARRAYINDEX(wX, wRefY)] = wSquare;

			wSquare = this->coveredOSquares.GetAt(wX, wY);
			wSquare2 = this->coveredOSquares.GetAt(wX, wRefY);
			this->coveredOSquares.Add(wX, wY, wSquare2);
			this->coveredOSquares.Add(wX, wRefY, wSquare);

			wSquare = this->tileLights.GetAt(wX, wY);
			wSquare2 = this->tileLights.GetAt(wX, wRefY);
			if (wSquare2)
				this->tileLights.Add(wX, wY, wSquare2);
			else this->tileLights.Remove(wX, wY);
			if (wSquare)
				this->tileLights.Add(wX, wRefY, wSquare);
			else this->tileLights.Remove(wX, wRefY);

			wSquare = this->pressurePlateIndex.GetAt(wX, wY);
			wSquare2 = this->pressurePlateIndex.GetAt(wX, wRefY);
			if (wSquare2)
				this->pressurePlateIndex.Add(wX, wY, wSquare2);
			else this->pressurePlateIndex.Remove(wX, wY);
			if (wSquare)
				this->pressurePlateIndex.Add(wX, wRefY, wSquare);
			else this->pressurePlateIndex.Remove(wX, wRefY);

			wSquare = GetFSquare(wX, wY);
			wSquare2 = GetFSquare(wX, wRefY);
			ReflectSquare(false, wSquare);
			ReflectSquare(false, wSquare2);
			this->pszFSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszFSquares[ARRAYINDEX(wX, wRefY)] = wSquare;

			swapTLayer(wX, wY, wX, wRefY);
		}
	}

	//Reflect platforms.
	for (vector<CPlatform*>::const_iterator platformIter = this->platforms.begin();
			platformIter != this->platforms.end(); ++platformIter)
	{
		CPlatform *pPlatform = *platformIter;
		pPlatform->ReflectY(this);
	}

	//Reflect monsters.
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pMonster->ReflectY(this);
		pMonster = pMonster->pNext;
	}

	//Reflect orbs.
	for (vector<COrbData*>::const_iterator orb=this->orbs.begin();
			orb != this->orbs.end(); ++orb)
	{
		COrbData *pOrb = *orb;
		pOrb->wY = reflectionOffset - pOrb->wY;
		//Reflect orb agents.
		for (vector<COrbAgentData*>::const_iterator agent=pOrb->agents.begin();
				agent != pOrb->agents.end(); ++agent)
		{
			(*agent)->wY = reflectionOffset - (*agent)->wY;
		}
		//Reflect pressure plate sets.
		CCoordSet coords;
		for (CCoordSet::const_iterator coord=pOrb->tiles.begin(); coord!=pOrb->tiles.end(); ++coord)
			coords.insert(coord->wX, reflectionOffset - coord->wY);
		pOrb->tiles = coords;
	}

	//Reflect scrolls.
	for (vector<CScrollData*>::const_iterator scroll=this->Scrolls.begin();
			scroll != this->Scrolls.end(); ++scroll)
	{
		(*scroll)->wY = reflectionOffset - (*scroll)->wY;
	}

	//Reflect exits.
	for (vector<CExitData*>::const_iterator stair=this->Exits.begin();
			stair != this->Exits.end(); ++stair)
	{
		CExitData* pStair = *stair;
		const UINT oldwTop = pStair->wTop;
		pStair->wTop = reflectionOffset - pStair->wBottom;
		pStair->wBottom = reflectionOffset - oldwTop;
	}

	//Reflect checkpoints.
	CCoordSet::const_iterator coord;
	CCoordSet coords;
	for (coord=this->checkpoints.begin(); coord!=this->checkpoints.end(); ++coord)
		coords.insert(coord->wX, reflectionOffset - coord->wY);
	this->checkpoints = coords;
	coords.clear();

	//Reflect Halph/Slayer.
	for (coord=this->halphEnters.begin(); coord!=this->halphEnters.end(); ++coord)
		coords.insert(coord->wX, reflectionOffset - coord->wY);
	this->halphEnters = coords;
	coords.clear();
	for (coord=this->halph2Enters.begin(); coord!=this->halph2Enters.end(); ++coord)
		coords.insert(coord->wX, reflectionOffset - coord->wY);
	this->halph2Enters = coords;
	coords.clear();

	for (coord=this->slayerEnters.begin(); coord!=this->slayerEnters.end(); ++coord)
		coords.insert(coord->wX, reflectionOffset - coord->wY);
	this->slayerEnters = coords;
	coords.clear();
	for (coord=this->slayer2Enters.begin(); coord!=this->slayer2Enters.end(); ++coord)
		coords.insert(coord->wX, reflectionOffset - coord->wY);
	this->slayer2Enters = coords;

	//ignore in-game vars
}

void CDbRoom::swapTLayer(const UINT x1, const UINT y1, const UINT x2, const UINT y2)
{
	RoomObject* &obj1 = this->tLayer[ARRAYINDEX(x1,y1)];
	RoomObject* &obj2 = this->tLayer[ARRAYINDEX(x2,y2)];

	std::swap(obj1, obj2);

	const bool horiz = x1 != x2;

	if (obj1) {
		obj1->set_pos(x1, y1);
		ReflectSquare(horiz, obj1->tile);
	}

	if (obj2) {
		obj2->set_pos(x2, y2);
		ReflectSquare(horiz, obj2->tile);
	}
}

//*****************************************************************************
void CDbRoom::Reload()
//Reloads a currently loaded room.
{
	ASSERT(this->dwRoomID);
	VERIFY(Load(this->dwRoomID));
}

//*****************************************************************************
void CDbRoom::ResetExitIDs()
//Resets level entrance IDs for all exits in the room.
//
//Whenever a room is moved to a different hold, it is that code's responsibility
//to call this method to not retain possibly incorrect level entrance IDs.
{
	bool bChanged = false;
	for (UINT wIndex = 0; wIndex < this->Exits.size(); ++wIndex)
	{
		//Only need to change non-zero IDs.
		if (this->Exits[wIndex]->dwEntranceID)
		{
			bChanged = true;
			this->Exits[wIndex]->dwEntranceID = 0;
		}
	}
	if (bChanged)
		Update();
}

//*****************************************************************************
void CDbRoom::ResetMonsterFirstTurnFlags()
//Sets first turn flag to false for all monsters.  When the first turn flag is
//set to true, the monster is not processed in CCurrentGame::ProcessMonsters().
{
	for (CMonster *pSeek = this->pFirstMonster; pSeek != NULL; pSeek=pSeek->pNext)
		pSeek->bIsFirstTurn = false;
}

//*****************************************************************************
bool CDbRoom::AddNewGlobalScript(const UINT dwCharID, CCueEvents &CueEvents)
//Create a new global script character at (0,0)
//
//Returns: True if successful, false if not.
{
	ASSERT(this->pCurrentGame);

	HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacter(dwCharID);
	if (!pCustomChar)
		return false;

	ASSERT(pCustomChar->dwScriptID);
	if (!pCustomChar->dwScriptID)
		return false;

	//If an instance of this global script was running previously in the room, then don't create a new one
	if (GetCharacterWithScriptID(pCustomChar->dwScriptID))
		return false;

	CMonster *pNew = CMonsterFactory::GetNewMonster(M_CHARACTER);
	ASSERT(pNew);
	pNew->wX = pNew->wY = pNew->wO = 0;
	pNew->bIsFirstTurn = true;

	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
	pCharacter->wLogicalIdentity = dwCharID;
	pCharacter->SetCurrentGame(this->pCurrentGame);
	pCharacter->bGlobal = true;
	pCharacter->dwScriptID = pCustomChar->dwScriptID;
	pCharacter->wProcessSequence = pCustomChar->ExtraVars.GetVar(ParamProcessSequenceStr, SPD_CHARACTER);

	LinkMonster(pNew, false);

	//Make sure that the current list of Global Scripts includes this ID
	this->pCurrentGame->GlobalScriptsRunning += dwCharID;

	const bool bExec = this->pCurrentGame->ExecutingNoMoveCommands();
	this->pCurrentGame->SetExecuteNoMoveCommands();
	pCharacter->Process(CMD_WAIT, CueEvents);
	this->pCurrentGame->SetExecuteNoMoveCommands(bExec);

	return true;
}

//*****************************************************************************
void CDbRoom::AddRunningGlobalScripts(CCueEvents &CueEvents)
//Start up all currently running global scripts on room start.
{
	if (!this->pCurrentGame)
		return; //don't change anything

	CIDSet BrokenGlobalScripts;

	for (CIDSet::const_iterator c = this->pCurrentGame->GlobalScriptsRunning.begin();
			c != this->pCurrentGame->GlobalScriptsRunning.end(); ++c)
		if (!AddNewGlobalScript(*c, CueEvents))
			BrokenGlobalScripts += *c;

	this->pCurrentGame->GlobalScriptsRunning -= BrokenGlobalScripts;
}

//*****************************************************************************
void CDbRoom::SetHalphSlayerEntrance()
//Determine whether player entered the room on a tile where one of these characters
//will enter later.  If so, retain this coordinate in the list.
{
	if (!this->pCurrentGame)
		return; //don't change anything

	const bool bHalphEnters = this->halphEnters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	const bool bHalph2Enters = this->halph2Enters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	const bool bSlayerEnters = this->slayerEnters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	const bool bSlayer2Enters = this->slayer2Enters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	ASSERT(!bHalphEnters || !bHalph2Enters || !bSlayerEnters || !bSlayer2Enters); //this check could be tightened

	this->halphEnters.clear();
	this->halph2Enters.clear();
	this->slayerEnters.clear();
	this->slayer2Enters.clear();

	if (bHalphEnters)
		this->halphEnters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	if (bHalph2Enters)
		this->halph2Enters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	if (bSlayerEnters)
		this->slayerEnters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	if (bSlayer2Enters)
		this->slayer2Enters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
}

//*****************************************************************************
void CDbRoom::CreatePathMap(
//Creates a PathMap for the current room.  If a PathMap has already
//been created, it will reset the PathMap.
//
//Params:
	const UINT wX, const UINT wY, // (in) Position of target (swordsman)
	const MovementType eMovement) // (in) Type of movement path reflects
{
	if (!this->pPathMap[eMovement])
	{
		UINT dwPathThroughObstacleCost = 0;
		if (eMovement == WALL || eMovement == WATER) //allow for non-wall/water
			dwPathThroughObstacleCost = 1000;         //squares to be in the path
		this->pPathMap[eMovement] = new CPathMap(this->wRoomCols, this->wRoomRows,
				wX, wY, dwPathThroughObstacleCost, bMovementSupportsPartialObstacles(eMovement));
	}
	else
		this->pPathMap[eMovement]->SetTarget(wX, wY);
	for (UINT x = 0; x < this->wRoomCols; ++x)
		for (UINT y = 0; y < this->wRoomRows; ++y)
			this->pPathMap[eMovement]->SetSquare(x, y, GetSquarePathMapObstacles(x, y, eMovement));
}

//***************************************************************************************
void CDbRoom::CreatePathMaps()
//Create PathMap for each movement ability type.
{
	if (!IsPathmapNeeded())
		return;

	ASSERT(this->pCurrentGame);

	//Always create Ground pathmap (as there's always GROUND monsters
	//in the room, e.g. brains, slayer, guards)
	UINT wSX, wSY;
	this->pCurrentGame->GetSwordsman(wSX, wSY);
	CreatePathMap(wSX, wSY, GROUND);
	if (MonsterWithMovementTypeExists(GROUND_AND_SHALLOW_WATER))
		CreatePathMap(wSX, wSY, GROUND_AND_SHALLOW_WATER);
	//!! same as GROUND if no shallow water exists..

	//Generate other path maps as needed.
	//Pathmaps which already exist will be updated with wSX, wSY as target.
	if (IsBrainPresent())
		for (int n=1; n<NumMovementTypes; ++n)
			if (MonsterWithMovementTypeExists((MovementType)n))
				CreatePathMap(wSX, wSY, (MovementType)n);
}

//***************************************************************************************
void CDbRoom::SetPathMapsTarget(
//Set PathMap target for each movement ability type.
//
//Params:
	const UINT wX, const UINT wY) //(in) Target for each pathmaps
{
	for (int n=0; n<NumMovementTypes; ++n)
		if (this->pPathMap[n])
			this->pPathMap[n]->SetTarget(wX, wY);
}

//*****************************************************************************
bool CDbRoom::CanPlayerMoveOnThisElement(const UINT wAppearance, const UINT wTileNo) const
//Returns: whether player in current role can move on this o-layer object
{
	switch (wAppearance)
	{
		case M_SEEP:
			if (!(bIsWall(wTileNo) || bIsCrumblyWall(wTileNo) || bIsDoor(wTileNo)))
				return false;
		break;
		case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
			if (!(bIsPit(wTileNo) || bIsWater(wTileNo) ||
					bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) ||
					bIsStairs(wTileNo) || bIsTunnel(wTileNo) || bIsPlatform(wTileNo) ||
					(wTileNo == T_WALL_M && this->pCurrentGame && this->pCurrentGame->bHoldMastered) ||
					(wTileNo == T_WALL_WIN && this->pCurrentGame && this->pCurrentGame->bHoldCompleted)
					))
				return false;
		break;
		case M_WATERSKIPPER:
			if (!bIsWater(wTileNo) || wTileNo == T_PLATFORM_W)
				return false;
		break;
		default:
			if (!(bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) ||
					bIsStairs(wTileNo) || bIsTunnel(wTileNo) || bIsPlatform(wTileNo) ||
					(bIsShallowWater(wTileNo) &&
					this->pCurrentGame->swordsman.GetWaterTraversalState() >= WTrv_CanWade) ||
					(wTileNo == T_WALL_M && this->pCurrentGame && this->pCurrentGame->bHoldMastered) ||
					(wTileNo == T_WALL_WIN && this->pCurrentGame && this->pCurrentGame->bHoldCompleted)
					))
				return false;
		break;
	}
	return true;
}

//*****************************************************************************
bool CDbRoom::CanPushOntoFTile(
	const UINT wFromX, const UINT wFromY,
	const UINT wToX, const UINT wToY)
const
{
	for (UINT i=2; i--; )
	{
		const UINT wTileNo = i==0 ? GetFSquare(wToX, wToY) : GetFSquare(wFromX, wFromY);
		switch (wTileNo)
		{
			case T_NODIAGONAL:
				if (wFromX != wToX && wFromY != wToY) //diagonal movement
					return false;
			break;
			default:
				if (bIsArrow(wTileNo))
				{
					const UINT movement = nGetO(wToX-wFromX,wToY-wFromY);
					if (bIsArrowObstacle(wTileNo, movement))
						return false;
				}
			break;
		}
	}
	return true;
}

bool CDbRoom::CanPushOntoOTile(const UINT wX, const UINT wY) const
{
	const UINT wTileNo = GetOSquare(wX, wY);
	return bIsFloor(wTileNo) ||
		bIsOpenDoor(wTileNo) ||
		bIsPlatform(wTileNo) ||
		bIsPit(wTileNo) ||
		bIsWater(wTileNo);
}

bool CDbRoom::CanPushOntoTTile(const UINT wX, const UINT wY) const
{
	return bIsTLayerCoverableItem(GetTSquare(wX, wY));
}

//*****************************************************************************
bool CDbRoom::CanPushTo(
//Returns: whether a t-layer object can be pushed here
//
//Params:
	const UINT wFromX, const UINT wFromY,
	const UINT wToX, const UINT wToY)
const
{
	if (!IsValidColRow(wToX, wToY))
		return false;

	if (!CanPushOntoTTile(wToX, wToY))
		return false;

	if (!CanPushOntoFTile(wFromX, wFromY, wToX, wToY))
		return false;

	if (!CanPushOntoOTile(wToX, wToY))
		return false;

	CMonster *pMonster = GetMonsterAtSquare(wToX, wToY);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
		return false;

	//Is the player in the square?
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->IsPlayerAt(wToX, wToY))
		return false;

	//Is there a caber in the square?
	const UINT wO = GetOrientation(wFromX,wFromY,wToX,wToY);
	for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO) {
		if (nO != NO_ORIENTATION && nO != wO)
		{
			CMonster *pMonster = GetMonsterAtSquare(wToX-nGetOX(nO), wToY-nGetOY(nO));
			if (pMonster && pMonster->HasSwordAt(wToX, wToY) && pMonster->GetWeaponType() == WT_Caber)
				return false;
		}
	}
	if (this->pCurrentGame->IsPlayerWeaponAt(wToX, wToY) && !this->pCurrentGame->IsPlayerAt(wFromX,wFromY)
			&& this->pCurrentGame->swordsman.GetActiveWeapon() == WT_Caber)
		return false;

	//Does a Gentryii link block the push?
	if (DoesGentryiiPreventDiagonal(wFromX, wFromY, wToX, wToY))
		return false;

	//No obstacle.
	return true;
}

//*****************************************************************************
bool CDbRoom::CanPushMonster(
	const CMonster* pMonster,
	const UINT wFromX, const UINT wFromY,
	const UINT wToX, const UINT wToY)
const
{
	ASSERT(IsValidColRow(wToX, wToY));

	if (pMonster->IsPiece())
		return false;

	const UINT type = pMonster->wType;
	if (pMonster->IsLongMonster() && type != M_GENTRYII)
		return false;

	if (type == M_TEMPORALCLONE){
		const UINT tile = GetOSquare(wFromX, wFromY);
		if (bIsTunnel(tile) && this->pCurrentGame->PlayerEnteredTunnel(tile, nGetO(wToX - wFromX, wToY - wFromY), pMonster->GetIdentity())){
			return true;
		}
	}

	CMonster *pDestMonster = GetMonsterAtSquare(wToX, wToY);
	if (pDestMonster && pDestMonster->wType != M_FLUFFBABY)
		return false;

	//Tarstuff mothers can only be pushed onto their native tarstuff.
	const UINT tarType = getTarTypeForMother(type);
	if (tarType) {
		if (GetTSquare(wToX, wToY) != tarType)
			return false;
	} else {
		if (!CanPushOntoTTile(wToX, wToY))
			return false;
	}

	if (!CanPushOntoFTile(wFromX, wFromY, wToX, wToY))
		return false;

	//Seep can be pushed within walls and doors, as well as onto open areas.
	switch (type)
	{
		case M_SEEP:
		{
			const UINT tile = GetOSquare(wToX, wToY);
			if (!(CanPushOntoOTile(wToX, wToY) ||
					bIsDoor(tile) || bIsWall(tile) || bIsCrumblyWall(tile)))
				return false;
		}
		break;
		case M_GENTRYII:
		{
			CGentryii *pGentryii = const_cast<CGentryii*>(DYN_CAST(const CGentryii*, const CMonster*, pMonster));
			if (!pGentryii->CanPushHeadOntoOTileAt(wToX, wToY))
				return false;
		}
		break;
		case M_FLUFFBABY:
		{
			const UINT tile = GetOSquare(wToX, wToY);
			//Pushing onto Hot Tiles should fail, causing a "Puff Attack" instead
			if (!(CanPushOntoOTile(wToX, wToY) && tile != T_HOT))
				return false;
		}
		default:
			if (!CanPushOntoOTile(wToX, wToY))
				return false;
		break;
	}

	//Is the player in the square?
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->IsPlayerAt(wToX, wToY))
		return false;

	//Is there a caber in the square?
	const UINT wO = GetOrientation(wFromX,wFromY,wToX,wToY);
	for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO) {
		if (nO != NO_ORIENTATION && nO != wO)
		{
			CMonster *pMonster = GetMonsterAtSquare(wToX-nGetOX(nO), wToY-nGetOY(nO));
			if (pMonster && pMonster->HasSwordAt(wToX, wToY) && pMonster->GetWeaponType() == WT_Caber)
				return false;
		}
	}
	if (this->pCurrentGame->IsPlayerWeaponAt(wToX, wToY) && !this->pCurrentGame->IsPlayerAt(wFromX,wFromY)
			&& this->pCurrentGame->swordsman.GetActiveWeapon() == WT_Caber)
		return false;

	//Does a Gentryii link block the push?
	if (DoesGentryiiPreventDiagonal(wFromX, wFromY, wToX, wToY))
		return false;

	return true;
}

//*****************************************************************************
bool CDbRoom::DoesGentryiiPreventDiagonal(
	const UINT x1, const UINT y1, //diagonal source tile
	const UINT x2, const UINT y2) //destination tile
const
{
	const int dx = int(x1) - int(x2);
	if (abs(dx) != 1)
		return false;
	const int dy = int(y1) - int(y2);
	if (abs(dy) != 1)
		return false;

	const CMonster *pMonster1 = GetMonsterAtSquare(x1, y2);
	if (!pMonster1)
		return false;
	if (pMonster1->wType != M_GENTRYII)
		return false;

	const CMonster *pMonster2 = GetMonsterAtSquare(x2, y1);
	if (!pMonster2)
		return false;
	if (pMonster2->wType != M_GENTRYII)
		return false;

	//Establish whether the two adjacent diagonal tiles belong to the same gentryii.
	const CMonster *pHead = pMonster1->GetOwningMonsterConst();
	if (pHead != pMonster2->GetOwningMonsterConst())
		return false;

	//Determine whether the two tiles are directly connected.
	ASSERT(!pHead->Pieces.empty());
	if (pMonster1 == pHead && pMonster2 == pHead->Pieces.front())
		return true;
	if (pMonster2 == pHead && pMonster1 == pHead->Pieces.front())
		return true;

	//Still need to find first of these pieces in the chain and
	//determine whether the other piece follows immediately after.
	for (MonsterPieces::const_iterator it = pHead->Pieces.begin();
		it != pHead->Pieces.end(); ++it)
	{
		if (*it == pMonster1) {
			MonsterPieces::const_iterator next = it;
			++next;
			return next != pHead->Pieces.end() && *next == pMonster2;
		} else if (*it == pMonster2) {
			MonsterPieces::const_iterator next = it;
			++next;
			return next != pHead->Pieces.end() && *next == pMonster1;
		}
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::DoesMonsterEnterRoomLater(
	const UINT wX, const UINT wY, const UINT wMonsterType) const
//Returns: whether a monster of indicating type starting at (x,y) will not be
//in the room on entrance, but will enter it later.
{
	return (wMonsterType == M_HALPH || wMonsterType == M_HALPH2 ||
				wMonsterType == M_SLAYER || wMonsterType == M_SLAYER2) &&
		(wX==0 || wX == this->wRoomCols-1 || wY==0 || wY == this->wRoomRows-1);
}

//*****************************************************************************
bool CDbRoom::DoesSquareContainPlayerObstacle(
//Does a square contain an obstacle to player movement?
//
//Params:
		const UINT wX, const UINT wY,    //(in)   Destination square to check.
		const UINT wO,          //(in)   Direction of movement onto square.
		bool& bMonsterObstacle) //(out) If a monster obstructs path
//
//Returns:
//True if it does, false if not.
const
{
	ASSERT(IsValidColRow(wX, wY));
	bool bObstacle = false;
	bMonsterObstacle = false;

	//Look for t-square obstacle.
	UINT wTileNo = GetTSquare(wX, wY);
	const UINT wAppearance = this->pCurrentGame->swordsman.wAppearance;
	switch (wAppearance)
	{
		case M_WWING: case M_ROACH: case M_QROACH: case M_EYE: case M_EYE_ACTIVE:
		case M_SPIDER:	case M_GOBLIN:
		case M_ROCKGOLEM: case M_CONSTRUCT:
		case M_TARBABY: case M_MUDBABY: case M_GELBABY:
		case M_AUMTLICH: case M_WUBBA: case M_FEGUNDO:
			if (!CanPushOntoTTile(wX, wY))
			{
				if (!(bIsPotion(wTileNo)))
					bObstacle = true;

				//Older monster types cannot normally step on potions or scrolls.
				if (!this->pCurrentGame->swordsman.bCanGetItems)
					bObstacle = true;
			}
		break;
		default:
			//These items are not obstacles to humanoid types.
			if (!(CanPushOntoTTile(wX, wY) || bIsPotion(wTileNo)))
				bObstacle = true;
		break;
	}

	if (bIsArrowObstacle(GetFSquare(wX, wY), wO))
		bObstacle = true;

	//Look for o-square obstacle.
	//What is considered an obstacle depends on the player role.
	wTileNo = GetOSquare(wX, wY);
	if (!CanPlayerMoveOnThisElement(wAppearance, wTileNo))
		bObstacle = true;

	//Is there a monster here?
	const CMonster *pMonster = GetOwningMonsterOnSquare(wX, wY);
	if (pMonster)
	{
		if (pMonster->wType == M_FLUFFBABY)
		{
			if (this->pCurrentGame->swordsman.wAppearance == M_FLUFFBABY)
				bObstacle = bMonsterObstacle = true; //Fluff babies block Fluff babies
		} else {
			if (!(this->pCurrentGame->swordsman.CanStepOnMonsters() ||
					this->pCurrentGame->swordsman.CanDaggerStep(pMonster)))
				bObstacle = bMonsterObstacle = true; //some roles can't step on monsters

			if (pMonster->IsLongMonster())
				bObstacle = bMonsterObstacle = true; //can't ever step on a large monster

			//These monsters can't be stepped on
			UINT monsterType = pMonster->wType;
			if (monsterType == M_TEMPORALCLONE || monsterType == M_CLONE) {
				const CPlayerDouble *pDouble = DYN_CAST(const CPlayerDouble*, const CMonster*, pMonster);
				monsterType = pDouble->GetIdentity();
			}
			switch (monsterType)
			{
				case M_WUBBA:
				case M_ROCKGOLEM: case M_CONSTRUCT:
				case M_FEGUNDO:
					bObstacle = bMonsterObstacle = true;
					break;
				case M_CHARACTER:
				{
					const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
					if (pCharacter->IsInvulnerable() || pCharacter->IsPushableByBody() || pCharacter->IsPushableByWeaponAttack())
						bObstacle = bMonsterObstacle = true;
				}
				break;
				default: break;
			}
		}
	}

	//Is there a double's sword in the square?
	if (IsMonsterSwordAt(wX, wY, true))
		bObstacle = true;

	return bObstacle;
}

//*****************************************************************************
bool CDbRoom::DoesSquareContainDoublePlacementObstacle(
//Does a square contain an obstacle to player-double placement?
//
//Params:
	const UINT wX, const UINT wY,    //(in)   Destination square to check.
	const UINT wDoubleType)          //(in)   Type of Double to place [Default=M_MIMIC]
//
//Returns:
//True if it does, false if not.
const
{
	ASSERT(IsValidColRow(wX, wY));

	//Is there a monster in the square?
	if (GetMonsterAtSquare(wX, wY) != NULL)
		return true;

	//Look for t-square obstacle.
	UINT wTileNo = GetTSquare(wX, wY);
	if (!(wTileNo == T_EMPTY || wTileNo == T_FUSE || wTileNo == T_TOKEN))
		return true;

	if (bIsArrow(GetFSquare(wX, wY)))
		return true;

	//Look for o-square obstacle.
	//Can't be placed on trapdoors or open yellow doors (backwards compatibility).
	//Mimics/Decoys can always be placed on Shallow Water
	//Clones can only be placed on Shallow Water if the player can move over Shallow Water
	wTileNo = GetOSquare(wX, wY);
	if (bIsTrapdoor(wTileNo) || bIsThinIce(wTileNo) || !(bIsFloor(wTileNo) || bIsPlatform(wTileNo) ||
			(bIsShallowWater(wTileNo) &&
			((wDoubleType != M_CLONE && wDoubleType != M_TEMPORALCLONE) || this->pCurrentGame->swordsman.CanWadeInShallowWater())) ||
			(bIsOpenDoor(wTileNo) && wTileNo != T_DOOR_YO)))
		return true;

	if (this->pCurrentGame->IsPlayerAt(wX, wY))
		return true;

	if (this->pCurrentGame->IsPlayerWeaponAt(wX, wY))
		return true;

	if (IsMonsterSwordAt(wX, wY))
		return true;

	//No obstacle.
	return false;
}

//*****************************************************************************
bool CDbRoom::DoesOrthoSquarePreventDiagonal(
//Does an ortho-square prevent diagonal movement between the two tiles?
//The current square and destination square are both checked for these.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Square to check
	const int dx, const int dy)   //(in)   Offsets that indicate direction
											//    of movement from current square.
//
//Returns:
//True if a 4-connected square prevents diagonal movement, false if not.
const
{
	if (!dx || !dy)
		return false;  //not a diagonal move

	//Check for 4-connected square in current and destination squares.
	if (GetFSquare(wX, wY) == T_NODIAGONAL)
		return true;

	const UINT x2 = wX + dx, y2 = wY + dy;
	if (!IsValidColRow(x2, y2))
		return false;  //can exit room with a diagonal move
	if (GetFSquare(x2, y2) == T_NODIAGONAL)
		return true;

	return false;
}


//*****************************************************************************
bool CDbRoom::DoesSquarePreventDiagonal(
//Does anything on source/target square prevent moving in the specified diagonal?
	//
	//Params:
	const UINT wX, const UINT wY, //(in)   Square to check
	const int dx, const int dy)   //(in)   Directional offset
//
//Returns:
//True if an ortho square or gentryii chain prevents diagonal movement
const
{
	return DoesOrthoSquarePreventDiagonal(wX, wY, dx, dy)
		|| DoesGentryiiPreventDiagonal(wX, wY, wX + dx, wY + dy);
}

//*****************************************************************************
bool CDbRoom::DoesSquareContainTeleportationObstacle(
	// Checks if an entity of given type can teleport to this position.
	// Monster layer is not considered here! (because teleporting on monsters is
	// allowed if the teleported entity can body kill them)
	//
	//Params:
	const UINT wX, const UINT wY, //(in)   Square to check
	const UINT wIdentity)   //(in)   Identity of the teleported entity
	//
	//Returns:
	//True if a teleportation-preventing tile is present on the target square
	const
{
	const UINT oTile = GetOSquare(wX, wY);
	switch (oTile){
		case T_WALL_M:
		case T_WALL_WIN:
			return true;
	}

	const UINT tTile = GetTSquare(wX, wY);
	switch (tTile){
		case T_TAR:
			if (wIdentity != M_TARMOTHER){
				return true;
			}
		break;
		case T_MUD:
			if (wIdentity != M_MUDMOTHER){
				return true;
			}
		break;
		case T_GEL:
			if (wIdentity != M_GELMOTHER){
				return true;
			}
		break;
		case T_FLUFF:
		case T_BRIAR_DEAD:
		case T_BRIAR_LIVE:
		case T_BRIAR_SOURCE:
		case T_LIGHT:
			return true;
	}

	return false;
}
//*****************************************************************************
CMonster* CDbRoom::FindNextClone()
//Returns: pointer to next monster of type M_CLONE, else NULL if none
{
	ASSERT(!this->pLastClone || this->pLastClone->IsAlive());

	CMonster *pMonster = this->pLastClone; //keep track of last one returned
	if (!pMonster)
		pMonster = this->pFirstMonster;
	else {
		pMonster = pMonster->pNext; //start looking after this monster
		if (!pMonster)
			pMonster = this->pFirstMonster;
	}

	while (pMonster)
	{
		if (pMonster->wType == M_CLONE)
			break; //found next clone

		pMonster = pMonster->pNext;
		if (pMonster == this->pLastClone)
			break; //only one clone is in the monster list -- return it again
		if (!pMonster)
			pMonster = this->pFirstMonster;
	}

	return pMonster;
}

//*****************************************************************************
CCharacter* CDbRoom::GetCharacterWithScriptID(const UINT scriptID)
//Returns: character monster with indicated (unique) scriptID, or NULL if not found
{
	CMonster *pMonster;
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->dwScriptID == scriptID)
				return pCharacter; //found it
		}

	//Check dead monster list for character.
	for (list<CMonster*>::const_iterator iter = this->DeadMonsters.begin();
			iter != this->DeadMonsters.end(); ++iter)
	{
		pMonster = *iter;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->dwScriptID == scriptID)
				return pCharacter; //found it
		}
	}

	return NULL; //not found
}

//*****************************************************************************
bool CDbRoom::IsPathmapNeeded() const
//Returns: situations in which path maps need to be generated
{
	if (wBrainCount > 0 || !this->slayerEnters.empty())
		return true;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2 || pMonster->wType == M_GUARD)
			return true;
		pMonster = pMonster->pNext;
	}
	return false;
}

//*****************************************************************************
bool CDbRoom::IsTimerNeeded() const
//Returns: whether room objects exist that operate according to a certain time schedule
{
	ASSERT(this->pCurrentGame);
	if (!this->slayerEnters.empty() || !this->slayer2Enters.empty() ||
			!this->halphEnters.empty() || !this->halph2Enters.empty())
		return true;

	if (this->pCurrentGame->swordsman.bIsHasted)
		return true;

	if (this->pCurrentGame->dwCutScene)
		return true;

	if (!this->floorSpikes.empty())
		return true;
	if (!this->fluffVents.empty())
		return true;

	if (this->pCurrentGame->GetTemporalSplit().queuing())
		return true;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_QROACH || bIsMother(pMonster->wType) ||
				pMonster->wType == M_SKIPPERNEST || pMonster->wType == M_FLUFFBABY || bIsSerpent(pMonster->wType) ||
				pMonster->wType == M_TEMPORALCLONE)
			return true;
		//Constructs are only timed while the room has not yet been conquered.
		if (pMonster->wType == M_CONSTRUCT && !this->pCurrentGame->pRoom->bGreenDoorsOpened)
			return true;
		//Fegundo has a timed rebirth cycle when active.
		if ((pMonster->wType == M_FEGUNDO || pMonster->wType == M_FEGUNDOASHES) &&
				this->pCurrentGame->swordsman.bCanGetItems)
			return true;
		pMonster = pMonster->pNext;
	}
	return false;
}

//*****************************************************************************
bool CDbRoom::IsValidColRow(const UINT wX, const UINT wY) const
//Our square coords valid for this room dimensions.
{
	return wX < this->wRoomCols && wY < this->wRoomRows;
}

//*****************************************************************************
void CDbRoom::KillFluffOnHazard(CCueEvents &CueEvents)
//Destroy fluff and puffs in walls and on hot tiles upon room entry
//DestroyFluff checks should be used instead during normal turns
{
	for (UINT wY=wRoomRows; wY--; )
		for (UINT wX=wRoomCols; wX--; )
			if (GetTSquare(wX, wY) == T_FLUFF)
			{
				const UINT wOSquare = GetOSquare(wX, wY);
				if (bIsSolidOTile(wOSquare) || wOSquare == T_HOT)
				{
					RemoveStabbedTar(wX, wY, CueEvents, true);
					CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, T_FLUFF), true);
				}
			}

	ConvertUnstableTar(CueEvents);

	CMonster *pNext, *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pNext = pMonster->pNext;
		if (pMonster->wType == M_FLUFFBABY)
		{
			const UINT wOSquare = GetOSquare(pMonster->wX, pMonster->wY);
			if (bIsSolidOTile(wOSquare) || wOSquare == T_HOT)
			{
				KillMonster(pMonster,CueEvents);
				ProcessPuffAttack(CueEvents, pMonster->wX, pMonster->wY);
			}
		}
		pMonster = pNext;
	}
}

//*****************************************************************************
void CDbRoom::KillMonstersOnHazard(CCueEvents &CueEvents)
//Kill seep that are now outside walls, and constructs that are on oremites
{
	CMonster *pNext, *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pNext = pMonster->pNext;
		switch (pMonster->wType)
		{
			case M_SEEP:
			{
				CSeep *pWallMonster = DYN_CAST(CSeep*, CMonster*, pMonster);
				pWallMonster->KillIfOutsideWall(CueEvents);
			}
			break;
			case M_CONSTRUCT:
			{
				CConstruct *pConstruct = DYN_CAST(CConstruct*, CMonster*, pMonster);
				pConstruct->KillIfOnOremites(CueEvents);
			}
			break;
			case M_TEMPORALCLONE:
			{
				CTemporalClone *pTemporalClone = DYN_CAST(CTemporalClone*, CMonster*, pMonster);
				pTemporalClone->KillIfOnDeadlyTile(CueEvents);
			}
			break;
		}
		pMonster = pNext;
	}
}

//*****************************************************************************
bool CDbRoom::KillMonster(
//Kills a monster.
//
//Returns: whether monster was removed from array and list
//
//Params:
	CMonster *pMonster,     //(in)   Monster already in list of monsters for this room.
	CCueEvents &CueEvents,  //(out)  Adds cue events as appropriate.
	const bool bForce,      //[default=false] always remove monster (even Halph)
	const CEntity* pKillingEntity) //(in) Optional killing Entity who caused death [default=NULL]
{
	ASSERT(pMonster);
	ASSERT(!pMonster->IsPiece());

	const UINT mType = pMonster->wType;

	//Don't remove killed mission-critical types from monster list.
	//It is used in the front end death animation.
	//NOTE: If this happens on room entrance, there's nothing that can
	//be done to fix it, so just pretend there was no monster in the room here.
	bool bRemoveMonster = mType != M_HALPH && mType != M_HALPH2 && mType != M_CLONE && mType != M_TEMPORALCLONE;
	if (bRemoveMonster && mType == M_CHARACTER)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		bRemoveMonster = !pCharacter->IsMissionCritical() ||
				(this->pCurrentGame && this->pCurrentGame->wTurnNo == 0);
	}

	if (bRemoveMonster || bForce)
	{
		//Remove monster from the array and list and fix up links.
		ASSERT(pMonster->bAlive);
		RemoveMonsterFromTileArray(pMonster);
		if (pMonster->bProcessing)
			pMonster->bUnlink = true; //Defer unlinking until the monster's turn is over
		else
			UnlinkMonster(pMonster);

		//Retain pMonster's pNext and pPrevious pointers intact, so that
		//monster traversal in ProcessMonsters() isn't disrupted if many
		//monsters die (e.g. by a bomb explosion) at the same time.

		//Removing this monster may affect brain pathmaps.
		UpdatePathMapAt(pMonster->wX,pMonster->wY);
	}

	switch (mType)
	{
		case M_CHARACTER:
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsMissionCritical()) {
				//If critical NPC dies, flag it (except on room entrance).
				if (!bForce && this->pCurrentGame && this->pCurrentGame->wTurnNo > 0)
				{
					pCharacter->CriticalNPCDied(CueEvents);
				}
				if (!(bRemoveMonster || bForce))
				{
					pMonster->bAlive = false;
					return false; //retain dead critical character in monster list
				}
			} else if (pCharacter->IsRequiredToConquer()) {
				DecMonsterCount();
			}
			//else: don't decrement monster count.

			//Does character script complete when monster is killed?
			if (pCharacter->EndsWhenKilled())
				this->pCurrentGame->ScriptCompleted(pCharacter);

			CueEvents.Add(CID_NPCKilled, pMonster);
		}
		break;

		case M_CITIZEN: case M_ARCHITECT:
		case M_MIMIC:
		case M_WUBBA: case M_FLUFFBABY:
		case M_FEGUNDO: case M_FEGUNDOASHES:
		case M_SLAYER: case M_SLAYER2:
			//Don't decrement monster count.
		break;

		case M_TEMPORALCLONE:
			//RemoveMonsterEnemy(pMonster); //not needed on death
			//no break
		case M_CLONE:
			if (this->pCurrentGame)
				this->pCurrentGame->SetDyingEntity(pMonster, pKillingEntity);
			CueEvents.Add(CID_CriticalNPCDied);
			pMonster->bAlive = false;
		return false; //Leave in the monster list.

		case M_DECOY:
			RemoveDecoy(pMonster);
		break;

		case M_STALWART:
		case M_STALWART2:
			RemoveMonsterEnemy(pMonster);
		break;

		case M_HALPH:
		case M_HALPH2:
			//NOTE: If Halph is killed game ends, so removing path maps when he dies is a moot point.
			if (this->pCurrentGame)
				this->pCurrentGame->SetDyingEntity(pMonster, pKillingEntity);
			CueEvents.Add(CID_HalphDied, pMonster);
			pMonster->bAlive = false;
		return false; //Leave Halph in the monster list.

		case M_BRAIN:
			DecMonsterCount();
			//Decrement brain count.
			ASSERT(this->wBrainCount != 0); //Count should never decrement past 0.
			if (--wBrainCount == 0)
				CueEvents.Add(CID_AllBrainsRemoved);
			//Pathmaps will still be needed until end of turn, so don't remove them
		break;

		case M_GENTRYII:
			RemoveLongMonsterPieces(pMonster);
		break;
		case M_CONSTRUCT:
			//prevent construct from taking the current turn if monsters are already processing
			pMonster->bNewStun = true;

			if (pMonster->IsAlive())
				DecMonsterCount();
		break;
		default:
			if (pMonster->IsAlive())   //rock monster is considered "dead" when broken
				DecMonsterCount();      //and shouldn't decrement the count again here

			//Remove left-over long monster pieces.
			if (pMonster->IsLongMonster())
				RemoveLongMonsterPieces(pMonster);
		break;
	}
	pMonster->bAlive = false;

	//Put monster in dead monster list.
	//The pointer will be valid until this CDbRoom is destroyed.
	this->DeadMonsters.push_back(pMonster);
	CueEvents.Add(CID_MonsterExistenceCeased, pMonster);
	return bRemoveMonster || bForce;
}

//*****************************************************************************
bool CDbRoom::KillMonsterAtSquare(
//Kills a monster in a specified square.
//Supports monsters occupying multiple squares.
//
//Params:
	const UINT wX, const UINT wY, //(in) Indicates square containing monster to kill.
	CCueEvents &CueEvents,  //(out) Adds cue events as appropriate.
	const bool bForce)      //[default=false]
//
//Returns:
//True if a monster was found in square, false if not.
{
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (!pMonster) return false;
	pMonster = pMonster->GetOwningMonster();

	//Currently, KillMonsterAtSquare is not used by anything that
	//would require pKillingEntity to be set
	const bool bRemoved = KillMonster(pMonster, CueEvents, bForce);

	ASSERT(!pMonster->IsAlive());
	ASSERTP(!bRemoved || GetMonsterAtSquare(wX, wY)==0,
			"Monster not removed or multiple monsters on same square");

	return true;
}

//*****************************************************************************
void CDbRoom::RemoveMonsterFromTileArray(CMonster* pMonster)
{
	if (pMonster == this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)])
		this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)] = NULL;
}

//*****************************************************************************
void CDbRoom::RemoveMonsterDuringPlayWithoutEffect(CMonster* pMonster)
{
	switch (pMonster->wType) {
		case M_STALWART: case M_STALWART2:
		case M_TEMPORALCLONE:
			RemoveMonsterEnemy(pMonster);
		break;
		default: break;
	}

	RemoveMonsterFromTileArray(pMonster);

	if (pMonster->bProcessing)
		pMonster->bUnlink = true;
	else
		UnlinkMonster(pMonster);

	pMonster->bAlive = false;

	this->DeadMonsters.push_back(pMonster);

	UpdatePathMapAt(pMonster->wX, pMonster->wY);
}

//*****************************************************************************
bool CDbRoom::MonsterHeadIsAt(
//Returns: whether a monster's head is on a square.
// That is, if a monster occupies multiple squares, then its head is here.
// If the monster covers only one square, should always return true.
//
//Params:
	const UINT wX, const UINT wY) //(in) Position
const
{
	CMonster *pMonster = GetMonsterAtSquare(wX,wY);
	if (!pMonster)
		return false;  //no monster there
	return (pMonster->wX == wX && pMonster->wY == wY);
}

//*****************************************************************************
CMonster* CDbRoom::MonsterOfTypeExists(
//Returns: pointer to first monster of given type, if exists, else NULL
//
//Params:
	const UINT eType) //(in) monster type
const
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		if (pMonster->wType == eType)
			return pMonster;
	return NULL;
}

//*****************************************************************************
bool CDbRoom::MonsterWithMovementTypeExists(
//Returns: whether a monster with 'eMovement' type exists.
//
//Params:
	const MovementType eMovement) //(in)
const
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->eMovement == eMovement)
			return true;
		//Tarbabies can be spawned and potentially be switched with tarstuff switches
		if (eMovement == GROUND_AND_SHALLOW_WATER && bIsMonsterTarstuff(pMonster->wType))
			return true;
	}
	return false;
}

//*****************************************************************************
void CDbRoom::DecMonsterCount()
//Decrement room monster count.
{
	ASSERT(this->wMonsterCount != 0); //Count should never decrement past 0.
	--this->wMonsterCount;
	//Check for whether room is cleared was moved to CCurrentGame::ProcessCommand().
	//The check for whether this happened in the editor could be removed.
}

//*****************************************************************************
void CDbRoom::MoveMonster(
//Moves a monster in the monster array.
//
//Params:
	const CMonster* pMonster,  //(in) Monster moving.
	const UINT wDestX, const UINT wDestY)  //(in)   Destination to move to.
{
	ASSERT(this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)]==pMonster);
	ASSERT(!this->pMonsterSquares[ARRAYINDEX(wDestX,wDestY)]);

	this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)] = NULL;
	this->pMonsterSquares[ARRAYINDEX(wDestX,wDestY)] = const_cast<CMonster*>(pMonster);
}

//*****************************************************************************
CMonster* CDbRoom::AddNewMonster(
//Creates a new monster and returns a pointer to it.
//Also sets its room position and adds to monster list and array.
//
//Params:
	const UINT wMonsterType,      //(in)   One of M_* constants indicating
							//    type of monster to create.
	const UINT wX, const UINT wY, //(in) position of monster
	const bool bInRoom, //monster is in room on player entrance [default=true]
	const bool bLinkMonster) // whether to link the monster [default=true]
//
//Returns:
//Pointer to new monster object.
{
	//Set up a new monster.
	const MONSTERTYPE eMonsterType = (const MONSTERTYPE)wMonsterType;
	CMonster *pNew = CMonsterFactory::GetNewMonster(eMonsterType);
	if (this->pCurrentGame)
		pNew->SetCurrentGame(this->pCurrentGame);

	//Set monster position.
	pNew->wX = pNew->wPrevX = wX;
	pNew->wY = pNew->wPrevY = wY;

	//Update room stats.
	if (eMonsterType == M_BRAIN)
		++this->wBrainCount;

	//Only certain monsters are counted as required to remove to conquer a room.
	if (pNew->IsConquerable())
		++this->wMonsterCount;

	if (bLinkMonster)
		LinkMonster(pNew, bInRoom, eMonsterType == M_TEMPORALCLONE);

	//Return pointer to the new monster.
	return pNew;
}

//*****************************************************************************
void CDbRoom::LinkMonster(
//Sets monster's room position and adds to monster lists and array.
//
//Params:
	CMonster *pMonster,  //(in) Monster to add
	const bool bInRoom,  //is monster in room on a monster tile at this point [default=true]
	const bool bReverseRule) //should the entity be linked first in a tie-break? [default=false]
{
	//Find location in list to put monster at.  List is sorted by process sequence.
	CMonster *pSeek = this->pFirstMonster, *pLastPrecedingMonster=NULL;
	const UINT wProcessSequence = pMonster->GetProcessSequence();

	while (pSeek)
	{
		const UINT wNextMonsterSequence = pSeek->GetProcessSequence();
		if (wNextMonsterSequence > wProcessSequence)
			break;
		if (bReverseRule && wNextMonsterSequence == wProcessSequence)
			break; //new temporal clones go before others, not after
		pLastPrecedingMonster = pSeek;
		pSeek = pSeek->pNext;
	}

	//Add monster to list.
	if (pLastPrecedingMonster) //New monster goes at middle or end of list.
	{
		pMonster->pNext = pLastPrecedingMonster->pNext;
		pMonster->pPrevious = pLastPrecedingMonster;
		pLastPrecedingMonster->pNext = pMonster;
		if (pMonster->pNext) //Adding at middle of list.
		{
			pMonster->pNext->pPrevious = pMonster;
		}
		else //Adding at end of list.
		{
			this->pLastMonster = pMonster;
		}
	}
	else  //New monster goes at beginning of list.
	{
		pMonster->pPrevious = NULL;
		if (this->pFirstMonster) //The list has nodes.
		{
			pMonster->pNext = this->pFirstMonster;
			this->pFirstMonster->pPrevious = pMonster;
			this->pFirstMonster = pMonster;
		}
		else  //Empty list.
		{
			this->pFirstMonster = this->pLastMonster = pMonster;
			pMonster->pNext = NULL;
		}
	}

	//Maintain special room monster lists.
	switch (pMonster->wType)
	{
		case M_DECOY:
			this->Decoys.push_back(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
		break;
		case M_STALWART: case M_STALWART2:
			this->monsterEnemies.push_back(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
		break;
		case M_TEMPORALCLONE:
			//HACK: If we're linking this dynamically, then a new Temporal Clone goes at the front.
			//  This places it both before Stalwarts and other Temporal Clones in 'target order'.
			//Otherwise, we're linking sequentially, so the new Temporal Clone should go at the end,
			//  which should still be before other TClones and Stalwarts.
			if (bReverseRule)
				this->monsterEnemies.push_front(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
			else
				this->monsterEnemies.push_back(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
		break;
		default: break;
	}

	if (bInRoom)
		SetMonsterSquare(pMonster);
}

//*****************************************************************************
void CDbRoom::UnlinkMonster(
//Sets monster's room position and adds to monster lists and array.
//
//Params:
	CMonster *pMonster)  //(in) Monster to remove
{
	if (pMonster->pPrevious) pMonster->pPrevious->pNext = pMonster->pNext;
	if (pMonster->pNext) pMonster->pNext->pPrevious = pMonster->pPrevious;
	if (pMonster == this->pLastMonster) this->pLastMonster = pMonster->pPrevious;
	if (pMonster == this->pFirstMonster) this->pFirstMonster = pMonster->pNext;
}

//*****************************************************************************
CMonster* CDbRoom::GetMonsterAtSquare(
//Gets a monster located within the room at specified coordinates.
//Supports monsters covering multiple squares.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coords to look in.
//
//Returns:
//Pointer to monster at (wX,wY) or NULL if none.
const
{
	//Quickly look up pointer to a monster in a specified square.
	if (!IsValidColRow(wX,wY))
		return NULL;
	ASSERT(this->pMonsterSquares);
	return this->pMonsterSquares[ARRAYINDEX(wX,wY)];
}

//*****************************************************************************
const CMonster* CDbRoom::GetOwningMonsterOnSquare(const UINT wX, const UINT wY) const
{
	const CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster)
		pMonster = pMonster->GetOwningMonsterConst();
	return pMonster;
}

//*****************************************************************************
CMonster* CDbRoom::GetMonsterOfType(const UINT wType) const
//Returns: first monster of specified type in monster list, or NULL if none
{
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType != M_CHARACTER)
		{
			if (pMonster->wType == wType)
				return pMonster;
		} else {
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsVisible()) //Character must be in room to count.
			{
				if (wType == M_CHARACTER || pCharacter->GetIdentity() == wType)
					return pMonster;
			}
		}
		pMonster = pMonster->pNext;
	}
	return NULL;
}

//*****************************************************************************
UINT CDbRoom::GetMonsterTypeAt(
//Returns: monster type at (x,y), or M_NONE if none
//
//Params:
	const UINT wX, const UINT wY,
	const bool bConsiderNPCIdentity, //[default=false]
	const bool bOnlyLiveMonsters)	//[default=true]
const
{
	if (!IsValidColRow(wX,wY))
		return M_NONE;

	CMonster *pMonster = this->pMonsterSquares[ARRAYINDEX(wX,wY)];
	if (!pMonster)
		return M_NONE;

	if (bOnlyLiveMonsters && !pMonster->IsAlive())
		return M_NONE;

	if (pMonster->wType == M_CHARACTER && bConsiderNPCIdentity)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		return pCharacter->GetIdentity();
	}

	return pMonster->wType;
}

//*****************************************************************************
bool CDbRoom::GetNearestEntranceTo(const UINT wX, const UINT wY, const MovementType eMovement, UINT &wEX, UINT &wEY)
{
	//As pathmap targets are updated, we should make sure that only the new maps for Horn Blowing are used
	//Ideally, GetNearestEntranceTo should use its own pathmap storage rather than the global one leading
	//  to the player, but for now, this works.
	ASSERT(eMovement == GROUND_FORCE || eMovement == GROUND_AND_SHALLOW_WATER_FORCE);

	if (!this->pPathMap[eMovement])
		CreatePathMap(wX, wY, eMovement);
	else
		this->pPathMap[eMovement]->SetTarget(wX, wY);

	SORTPOINTS sp;
	this->pPathMap[eMovement]->GetEntrances(sp);
	for (SORTPOINTS::const_iterator it = sp.begin(); it != sp.end(); ++it)
	{
		const SORTPOINT& p = *it;
		if (GetMonsterAtSquare(p.wX, p.wY) != NULL || IsMonsterSwordAt(p.wX, p.wY)
				|| this->pCurrentGame->IsPlayerWeaponAt(p.wX, p.wY))
			continue;
		wEX = p.wX;
		wEY = p.wY;
		return true;
	}

	return false;
}

//*****************************************************************************
CMonster* CDbRoom::GetNPCBeethro(bool bDeadOnly) const
//Returns: pointer to first (visible) NPC Beethro (or Gunthro) in room, else NULL
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (!bDeadOnly || !pMonster->IsAlive())
		{
			switch (pMonster->wType)
			{
				case M_CHARACTER:
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					ASSERT(pCharacter);
					if (pCharacter && bIsSmitemaster(pCharacter->GetIdentity()) && pCharacter->IsVisible())
						return pMonster;
				}
				break;
				case M_TEMPORALCLONE:
				{
					const UINT wIdentity = pMonster->GetIdentity();
					if (bIsSmitemaster(wIdentity) || wIdentity == M_BEETHRO_IN_DISGUISE)
						return pMonster;
				}
				break;
				default: break;
			}
		}		
	}
	return NULL;
}

//*****************************************************************************
void CDbRoom::SetMonsterSquare(CMonster *pMonster)
//Set monster array pointer.
{
	ASSERT(!this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)]);
	this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)] = pMonster;
}

//*****************************************************************************
bool CDbRoom::IsMonsterOfTypeAt(
//Returns: whether monster of indicated type is at (x,y).
//         The given identity of character NPCs may also be considered.
//
//Params:
	const UINT eType,
	const UINT wX, const UINT wY, //(in) Coords to look in.
	const bool bConsiderNPCIdentity,	//[default=false]
	const bool bOnlyLiveMonsters)	//[default=true]
const
{
	if (!IsValidColRow(wX,wY)) return false;
	CMonster *pMonster = this->pMonsterSquares[ARRAYINDEX(wX,wY)];
	if (!pMonster) return false;
	if (bOnlyLiveMonsters && !pMonster->IsAlive()) return false;
	if (pMonster->wType == eType) return true;

	if (pMonster->wType == M_CHARACTER && bConsiderNPCIdentity)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		return pCharacter->GetIdentity() == eType;
	}
	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterInRect(
//Determines if any live monster is in a rectangular area of the room.
//Considers any part of a monster covering multiple squares.
//
//NOTE: Excludes player copies, Halph and characters from search.
//
//Params:
	const UINT wLeft, const UINT wTop,     //(in)   Rect to find monsters in.
	const UINT wRight, const UINT wBottom, //     Boundaries are inclusive.
	const bool bConsiderPieces)	//include monster pieces [default=true]
//
//Returns:
//True if there is a monster in the rect, false otherwise.
const
{
	ASSERT(wLeft <= wRight && wTop <= wBottom);
	ASSERT(wRight < this->wRoomCols && wBottom < this->wRoomRows);

	for (UINT y=wTop; y<=wBottom; ++y)
	{
		CMonster **pMonsters = &(this->pMonsterSquares[ARRAYINDEX(wLeft,y)]);
		for (UINT x=wLeft; x<=wRight; ++x)
		{
			CMonster *pMonster = *pMonsters;
			if (pMonster && pMonster->IsAlive() && (bConsiderPieces || !pMonster->IsPiece())) {
				if (!(bIsBeethroDouble(pMonster->wType) ||
					pMonster->wType == M_HALPH || pMonster->wType == M_HALPH2 ||
					pMonster->wType == M_STALWART || pMonster->wType == M_STALWART2 ||
					pMonster->wType == M_CHARACTER))
				{
					return true;
				}
			}
			++pMonsters;
		}
	}

	//No monster in rect.
	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterInRectOfType(
//Determines if any live monster of specified type is in a rectangular area of the room.
//Considers any part of a monster covering multiple squares.
//
//Params:
	const UINT wLeft, const UINT wTop,     //(in)   Rect to find monsters in.
	const UINT wRight, const UINT wBottom, //     Boundaries are inclusive.
	const UINT wType,                      //(in) monster type
	const bool bConsiderNPCIdentity,       //[default=false]
	const bool bUseNPCLogicalIdentity)     //[default=false]
//
//Returns:
//True if there is a monster in the rect, false otherwise.
const
{
	ASSERT(wLeft <= wRight && wTop <= wBottom);

	//Check each square for a monster (or monster piece).
	CMonster **pMonsters;

	ASSERT(wRight < this->wRoomCols && wBottom < this->wRoomRows);
	for (UINT y=wTop; y<=wBottom; ++y)
	{
		pMonsters = &(this->pMonsterSquares[ARRAYINDEX(wLeft,y)]);
		for (UINT x=wLeft; x<=wRight; ++x)
		{
			if (*pMonsters && (*pMonsters)->IsAlive())
			{
				if ((*pMonsters)->wType == wType)
					return true;
				if ((*pMonsters)->wType == M_CHARACTER && bConsiderNPCIdentity)
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, *pMonsters);
					if (bUseNPCLogicalIdentity)
					{
						if (pCharacter->wLogicalIdentity == wType)
							return true;
					} else {
						if (pCharacter->GetIdentity() == wType)
							return true;
					}
				}
			}
			++pMonsters;
		}
	}

	//No monster in rect.
	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterRemainsInRectOfType(
//Determines if any dead monster of specified type is in a rectangular area of the room.
//
//Params:
	const UINT wLeft, const UINT wTop,     //(in)   Rect to find monsters in.
	const UINT wRight, const UINT wBottom, //     Boundaries are inclusive.
	const UINT wType)					   //(in) monster type
//
//Returns:
//True if there is a monster in the rect, false otherwise.
const
{
	ASSERT(wLeft <= wRight && wTop <= wBottom);

	//Check each square for a monster (or monster piece).
	CMonster** pMonsters;

	ASSERT(wRight < this->wRoomCols&& wBottom < this->wRoomRows);
	for (UINT y = wTop; y <= wBottom; ++y)
	{
		pMonsters = &(this->pMonsterSquares[ARRAYINDEX(wLeft, y)]);
		for (UINT x = wLeft; x <= wRight; ++x)
		{
			if (*pMonsters && !(*pMonsters)->IsAlive())
			{
				if ((*pMonsters)->wType == wType)
					return true;
			}
			++pMonsters;
		}
	}

	//No monster in rect.
	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterNextTo(
//Returns: true if monster of specified type is adjacent to (not on) specified square
//
//Params:
	const UINT wX, const UINT wY, //(in) location
	const UINT wType) //(in) monster type
const
{
	int wI, wJ;
	for (wJ=-1; wJ<=1; ++wJ)
		for (wI=-1; wI<=1; ++wI)
			if ((wI || wJ) && IsValidColRow(wX+wI, wY+wJ))
			{
				CMonster *pMonster = GetMonsterAtSquare(wX+wI, wY+wJ);
				if (pMonster && pMonster->wType == wType)
					return true;
			}
	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterWithin(
//Returns: Whether a monster is within 'wSquares' of (wX,wY)?
//
//Params:
	const UINT wX, const UINT wY,    //(in)   Square to check from.
	const UINT wSquares, //(in)   Radius to check around center square (inclusive).
	const bool bConsiderPieces)	//includes monster segments [default=true]
const
{
	//bounds checking
	const UINT wLeft = (wX > wSquares ? wX - wSquares : 0);
	const UINT wTop = (wY > wSquares ? wY - wSquares : 0);
	const UINT wRight = (wX < this->wRoomCols-wSquares ?
			wX + wSquares : this->wRoomCols-1);
	const UINT wBottom = (wY < this->wRoomRows-wSquares ?
			wY + wSquares : this->wRoomRows-1);

	return IsMonsterInRect(wLeft,wTop,wRight,wBottom,bConsiderPieces);
}

//*****************************************************************************
bool CDbRoom::IsOrbBeingStruck(const UINT wX, const UINT wY) const
//Returns: whether a creature is striking the orb at (wX,wY).
{
	ASSERT(IsValidColRow(wX,wY));
	if (GetTSquare(wX,wY) != T_ORB) return false;

	//Doesn't check for player striking the orb.

	CMonster *pMonster;
	int wI, wJ;
	for (wJ=-1; wJ<=1; ++wJ)
		for (wI=-1; wI<=1; ++wI)
			if ((wI || wJ) && IsValidColRow(wX+wI, wY+wJ))
			{
				pMonster = GetMonsterAtSquare(wX+wI, wY+wJ);
				if (pMonster)
				{
					//Is a character or Neather/Halph adjacent to the orb?
					if (pMonster->wType == M_CHARACTER || pMonster->wType == M_NEATHER ||
							pMonster->wType == M_HALPH || pMonster->wType == M_HALPH2)
						return true;
					//Is a sword striking the orb?
					if (pMonster->HasSwordAt(wX, wY))
						return true;
				}
			}
	return false;
}

//*****************************************************************************
bool CDbRoom::IsRequired(
//Returns: Whether the room with this ID is required to complete its level.
//
//Params:
	const UINT dwRoomID)
{
	//Open rooms view.
	ASSERT(IsOpen());
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(dwRoomID, V_Rooms, RoomsView);
	if (dwRoomI == ROW_NO_MATCH)
	{
		ASSERT(!"No matching room.");
		return false;
	}

	return p_IsRequired(RoomsView[dwRoomI]) != 0;
}

//*****************************************************************************
bool CDbRoom::IsSecret(
//Returns: Whether the room with this ID is marked as secret.
//
//Params:
	const UINT dwRoomID)
{
	//Open rooms view.
	ASSERT(IsOpen());
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(dwRoomID, V_Rooms, RoomsView);
	if (dwRoomI == ROW_NO_MATCH)
	{
		ASSERT(!"No matching room.");
		return false;
	}

	return p_IsSecret(RoomsView[dwRoomI]) != 0;
}

//*****************************************************************************
bool CDbRoom::IsSwordAt(
//Returns: Whether a sword is at (wX,wY)
//
//Params:
	const UINT wX, const UINT wY)
const
{
	return IsSwordWithinRect(wX,wY,wX,wY);
}

//*****************************************************************************
bool CDbRoom::IsSwordWithinRect(
//Returns: Whether a sword is within this rectangular region
//
//Params:
	const UINT wMinX, const UINT wMinY, const UINT wMaxX, const UINT wMaxY)
const
{
	//Check double/NPC swords.
	UINT wSX, wSY;
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->GetSwordCoords(wSX, wSY))
			if (wMinX <= wSX && wSX <= wMaxX && wMinY <= wSY && wSY <= wMaxY)
				return true;
		pMonster = pMonster->pNext;
	}

	//Check player's weapon.
	if (!this->pCurrentGame->swordsman.HasWeapon())
		return false;

	wSX = this->pCurrentGame->swordsman.wSwordX;
	wSY = this->pCurrentGame->swordsman.wSwordY;
	return wMinX <= wSX && wSX <= wMaxX && wMinY <= wSY && wSY <= wMaxY;
}

//*****************************************************************************
bool CDbRoom::IsTileInRectOfType(
//Determines if a tile of the specified type is in a rectangular area of the room.
//
//Params:
	const UINT wLeft, const UINT wTop,     //(in)   Rect in which to find tiles.
	const UINT wRight, const UINT wBottom, //     Boundaries are inclusive.
	const UINT wType)                      //(in) tile type
//
//Returns:
//True if there is a tile of the specified type in the rect, false otherwise.
const
{
	ASSERT(wLeft <= wRight && wTop <= wBottom);
	ASSERT(wRight < this->wRoomCols && wBottom < this->wRoomRows);
	ASSERT(wType < TILE_COUNT);

	//Check each square for this tile.
	char *pszSquare, *pszSquares;
	switch (TILE_LAYER[wType])
	{
		case 0: pszSquares = pszOSquares; break;
		case 1:
		{
			for (UINT y=wTop; y<=wBottom; ++y)
			{
				RoomObject **pObj = this->tLayer + ARRAYINDEX(wLeft,y);
				for (UINT x=wLeft; x<=wRight; ++x, ++pObj)
				{
					RoomObject *tObj = *pObj;
					if (tObj && tObj->tile == wType)
						return true;
				}
			}
		}
		return false;
		case 3: pszSquares = pszFSquares; break;
		default: ASSERT(!"Invalid layer"); return false;
	}
	for (UINT y=wTop; y<=wBottom; ++y)
	{
		pszSquare = pszSquares + ARRAYINDEX(wLeft,y);
		for (UINT x=wLeft; x<=wRight; ++x)
		{
			if (*(pszSquare++) == static_cast<char>(wType))
				return true;
		}
	}

	//No tile of this type in rect.
	return false;
}

//*****************************************************************************
bool CDbRoom::BrainSensesSwordsman() const
//Returns: whether any brain can sense Beethro.
//
//NOTE: Don't have monsters call this directly, as it's processor intensive and
//redundant.  Query CCurrentGame.bBrainSensesSwordsman instead for multiple
//checks on the same turn.
{
	if (!IsBrainPresent())
		return false;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_BRAIN && pMonster->CanFindSwordsman())
			return true;
		pMonster = pMonster->pNext;
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::BrainSensesTarget() const
//Returns: whether a brain senses a non-Beethro monster target
{
	if (!IsBrainPresent()) return false;

	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL;
			pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_BRAIN)
			if (pMonster->SensesTarget())
				return true;
	}

	return false;	//none found
}

//*****************************************************************************
void CDbRoom::ActivateToken(
	CCueEvents &CueEvents,
	const UINT wX, const UINT wY,
	CPlayerDouble* pDouble) //if set, indicates a double is activating this token
//Toggle token -- modify room elements/behavior.
{
	ASSERT(IsValidColRow(wX,wY));
	if (GetTSquare(wX, wY) != T_TOKEN)
		return;

	BYTE tParam = GetTParam(wX, wY);
	const RoomTokenType tType = (RoomTokenType)calcTokenType(tParam);

	if (pDouble && !DoubleCanActivateToken(tType))
		return;

	const bool bOn = bTokenActive(tParam);
	switch (tType)
	{
		case WeaponDisarm:
		{
			this->pCurrentGame->swordsman.bNoWeapon = !this->pCurrentGame->swordsman.bNoWeapon;
			ChangeTiles(WeaponDisarm);
			CueEvents.Add(CID_TokenToggled);
			this->PlotsMade.insert(wX,wY);

			for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL;
					pMonster = pMonster->pNext)
				switch (pMonster->wType)
				{
					case M_DECOY: case M_CLONE: case M_MIMIC:
					case M_TEMPORALCLONE:
					{
						CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
						pDouble->bNoWeapon = this->pCurrentGame->swordsman.bNoWeapon;
					}
					break;
					default: break;
				}
		}
		return;

		case PowerTarget:
			//Touching this token gives the non-Beethro player Beethro-like item
			//handling abilities and makes him an attack target.
			//Allows controlling fegundos.
			if (bOn)
				return; //can't turn off
			this->pCurrentGame->swordsman.bCanGetItems = true;
			this->pCurrentGame->SetPlayerAsTarget();
			//Set all temporal clones as targets
			for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
			{
				switch (pMonster->wType)
				{
					case M_TEMPORALCLONE:
					{
						CTemporalClone *pClone = DYN_CAST(CTemporalClone*, CMonster*, pMonster);
						pClone->bIsTarget = true;
						pClone->SetMovementType();
					}
					break;
					default: break;
				}
			}

			ChangeTiles(PowerTarget);
			CueEvents.Add(CID_TokenToggled);
		return;

		case TarTranslucent:
			//Toggles tar translucency and state of all tokens of this type.
			this->bBetterVision = !bOn;
			ChangeTiles(TarTranslucent);
			CueEvents.Add(CID_TokenToggled);
		return;

		case ConquerToken:
			//Touching a Conquer token for the first time.
			if (bOn)
				return; //can't turn off
			ASSERT(this->pCurrentGame->conquerTokenTurn == NO_CONQUER_TOKEN_TURN);
			this->pCurrentGame->conquerTokenTurn = this->pCurrentGame->wPlayerTurn;
			ChangeTiles(ConquerToken);
			CueEvents.Add(CID_TokenToggled);
		return;

		case RotateArrowsCW: ChangeTiles(bOn ? RotateArrowsCCW : tType); break;
		case RotateArrowsCCW: ChangeTiles(bOn ? RotateArrowsCW : tType); break;
		case SwitchTarMud: SwitchTarstuff(T_TAR, T_MUD, CueEvents); break;
		case SwitchTarGel: SwitchTarstuff(T_TAR, T_GEL, CueEvents); break;
		case SwitchGelMud: SwitchTarstuff(T_GEL, T_MUD, CueEvents); break;

		case SwordToken:
		case PickaxeToken:
		case SpearToken:
		case StaffToken:
		case DaggerToken:
		case CaberToken:
		{
			WeaponType& wieldedWeapon = (pDouble ? pDouble->weaponType : this->pCurrentGame->swordsman.localRoomWeaponType);
			const RoomTokenType swapTokenType = getTokenForWeaponType(WeaponType(wieldedWeapon));
			if (swapTokenType != tType) {
				wieldedWeapon = getWeaponForTokenType(tType);

				SetTParam(wX, wY, swapTokenType);
				CueEvents.Add(CID_TokenToggled);
				this->PlotsMade.insert(wX,wY);
				if (pDouble) {
					pDouble->SetWeaponSheathed();
				} else {
					this->pCurrentGame->SetPlayerWeaponSheathedState();
				}
			}
		}
		return;

		case TemporalSplit:
			//Touching a temporal split point for the first time.
			if (!bOn && this->pCurrentGame->swordsman.wX == wX &&
					this->pCurrentGame->swordsman.wY == wY && 
					this->pCurrentGame->swordsman.wPlacingDoubleType == 0 &&
					this->pCurrentGame->StartTemporalSplit()) {
				SetTParam(wX, wY, tParam + TOKEN_ACTIVE);  //toggle on-off
				CueEvents.Add(CID_TemporalSplitStart);
				this->PlotsMade.insert(wX,wY);
			}
		return;

		case PersistentCitizenMovement:
		case TemporalSplitUsed:
			return; //do nothing

		default: ASSERT(!"Unexpected token type."); break;
	}

	SetTParam(wX, wY, tParam + TOKEN_ACTIVE);  //toggle on-off
	CueEvents.Add(CID_TokenToggled);
	this->PlotsMade.insert(wX,wY);
}

//*****************************************************************************
void CDbRoom::DisableToken(CCueEvents& CueEvents, UINT x, UINT y)
{
	if (GetTSquare(x,y) == T_TOKEN) {
		const BYTE tParam = GetTParam(x,y);
		if (bTokenActive(tParam)) {
			SetTParam(x, y, tParam + TOKEN_ACTIVE);  //toggle on-off
			CueEvents.Add(CID_TokenToggled);
			this->PlotsMade.insert(x,y);
		}
	}
}

//*****************************************************************************
bool CDbRoom::DoubleCanActivateToken(RoomTokenType type) const
{
	switch (type) {
		case TemporalSplit:
			return false;
		default: return true;
	}
}

//*****************************************************************************
void CDbRoom::DoExplode(
//Blow up bombs in coords set.
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	CCoordStack& bombs,   //(in) empty on exit
	CCoordStack& powder_kegs) //(in) empty on exit
{
	if (bombs.IsEmpty() && powder_kegs.IsEmpty())
		return;

	static const UINT BOMB_RADIUS = 3;
	static const UINT POWDER_KEG_RADIUS = 1;

	CCoordSet explosion; //what tiles are affected following explosion
	CCoordIndex caberCoords;
	GetCaberCoords(caberCoords);

	//Each iteration explodes one bomb.
	for (;;) {
		UINT wCol, wRow;
		if (bombs.PopBottom(wCol, wRow)) { //process as queue
			DoExplodeTile(CueEvents, bombs, powder_kegs, explosion, caberCoords, wCol, wRow, BOMB_RADIUS);
			continue;
		}

		if (powder_kegs.PopBottom(wCol, wRow)) {
			DoExplodeTile(CueEvents, bombs, powder_kegs, explosion, caberCoords, wCol, wRow, POWDER_KEG_RADIUS);
			continue;
		}

		break; //none left -- done creating explosions
	}

	//Now process the explosion's effects.
	for (CCoordSet::const_iterator exp=explosion.begin(); exp!=explosion.end(); ++exp)
		ProcessExplosionSquare(CueEvents, exp->wX, exp->wY);

	ConvertUnstableTar(CueEvents);
}

void CDbRoom::DoExplodeTile(
	CCueEvents& CueEvents,
	CCoordStack& bombs,
	CCoordStack& powder_kegs,
	CCoordSet& explosion,
	const CCoordIndex& caberCoords,
	UINT wCol, UINT wRow,
	UINT explosion_radius,
	bool bAddCueEvent) //[default=true]
{
	//Remove exploding item.  Initiate explosion.
	const UINT tTile = GetTSquare(wCol,wRow);
	if (tTile == T_BOMB || tTile == T_POWDER_KEG)
		Plot(wCol,wRow,T_EMPTY);

	if (bAddCueEvent)
		CueEvents.Add(CID_BombExploded, new CCoord(wCol, wRow), true);

	//Keep track of a list of squares to expand explosion out from.
	//Constraints will be performed with each expansion from these squares.
	CCoordStack cs(wCol, wRow);

	//Explode outward until done.
	UINT wX, wY;
	while (cs.PopBottom(wX, wY))  //process as queue
		ExpandExplosion(CueEvents, cs, wCol, wRow, wX, wY, bombs, powder_kegs, explosion, caberCoords, explosion_radius);
}

//*****************************************************************************
void CDbRoom::ExplodeStabbedPowderKegs(CCueEvents& CueEvents)
{
	if (!this->stabbed_powder_kegs.IsEmpty())
	{
		CCoordStack no_bombs;
		DoExplode(CueEvents, no_bombs, this->stabbed_powder_kegs);

		this->stabbed_powder_kegs.Clear();
	}
}

//*****************************************************************************
CCoordStack CDbRoom::GetPowderKegsStillOnHotTiles() const
{
	CCoordStack powder_kegs;

	for (set<const RoomObject*>::const_iterator it=this->stationary_powder_kegs.begin();
		it!=this->stationary_powder_kegs.end(); ++it)
	{
		const RoomObject& keg = **it;
		if (GetOSquare(keg.wX,keg.wY) == T_HOT && GetTSquare(keg.wX, keg.wY) == T_POWDER_KEG)
			powder_kegs.Push(keg.wX,keg.wY);
	}

	return powder_kegs;
}

//*****************************************************************************
void CDbRoom::BurnFuses(
//Burn all lit fuses one square each turn.
//Detonate all lit bombs.
//
//Params:
	CCueEvents &CueEvents)     //(in/out)
{
	CCoordStack bombs, powder_kegs;

	//Burn each lit fuse piece.
	UINT wX, wY;
	for (CCoordSet::const_iterator fuse=this->LitFuses.begin();
			fuse!=this->LitFuses.end(); ++fuse)
	{
		wX = fuse->wX;
		wY = fuse->wY;
		switch (GetTSquare(wX,wY))
		{
			case T_BOMB:
				//Detonate lit bomb.
				bombs.Push(wX,wY);
				Plot(wX,wY,T_EMPTY);
			break;
			case T_POWDER_KEG:
				powder_kegs.Push(wX,wY);
				Plot(wX,wY,T_EMPTY);
				//No break -- fallthrough to Fuse burning
			case T_FUSE:
			{
				//Start adjacent fuse pieces burning.
				Plot(wX,wY,T_EMPTY);
				static const int wOX[4] = {0,1,0,-1};
				static const int wOY[4] = {-1,0,1,0};
				for (int i=0; i<4; ++i)
				{
					const UINT wAdjX = wX + wOX[i], wAdjY = wY + wOY[i];
					// skip it if it's past the edge of the room
					if (wAdjX >= wRoomCols || wAdjY >= wRoomRows)
						continue;
					const UINT wAdjTileNo = GetTSquare(wAdjX, wAdjY);
					const bool canFuseBurnOnto = bIsFuseConnected(wAdjTileNo) ||
						(wAdjTileNo == T_POWDER_KEG && GetCoveredTSquare(wAdjX, wAdjY) == T_FUSE);
					if (canFuseBurnOnto)
					{
						//Don't allow burning when blocked by force arrows.
						const UINT wSrcFTile = GetFSquare(wX, wY),
							wDestFTile = GetFSquare(wAdjX, wAdjY);
						if (bIsArrow(wSrcFTile) && bIsArrowObstacle(wSrcFTile, nGetO(wOX[i],wOY[i])))
							continue;
						if (bIsArrow(wDestFTile) && bIsArrowObstacle(wDestFTile, nGetO(wOX[i],wOY[i])))
							continue;

						//Light each adjacent combustible item.
						if (this->NewFuses.insert(wAdjX, wAdjY))
							CueEvents.Add(CID_FuseBurning, new CMoveCoord(wAdjX, wAdjY,
									FuseEndAt(wAdjX, wAdjY, false)), true);
					}
				}
			}
			break;
			case T_EMPTY:
			case T_MIRROR:  //mirrors can be pushed onto burning fuses to put them out
			case T_BRIAR_LIVE: //can grow onto a burning fuse
				//do nothing
			break;
			default:
				ASSERT(!"Bad fuse burning item");
			break;
		}
	}

	//Include powder kegs that explode from hot tiles here,
	//in order to avoid introducing another ordering dependency.
	const CCoordStack kegs_on_hot_tiles = GetPowderKegsStillOnHotTiles();
	powder_kegs += kegs_on_hot_tiles;

	//Detonate all explosives simultaneously.
	DoExplode(CueEvents, bombs, powder_kegs);

	//Ordering issues might place new fuses where these fuses just burned.
	//Ensure that they won't burn here again.
	this->NewFuses -= this->LitFuses;
		this->LitFuses.clear();
}

//*****************************************************************************
void CDbRoom::BurnFuseEvents(CCueEvents &CueEvents)
//Signal cue events for which fuses are burning, but don't advance fuses.
{
	//Combine newly lit pieces with pieces already burning for burning next turn.
	this->LitFuses += this->NewFuses;
	this->NewFuses.clear();

	for (CCoordSet::const_iterator fuse=this->LitFuses.begin();
			fuse!=this->LitFuses.end(); ++fuse)
	{
		const UINT wX = fuse->wX;
		const UINT wY = fuse->wY;
		const UINT wTileNo = GetTSquare(wX,wY);
		ASSERT(bIsCombustibleItem(wTileNo) || wTileNo==T_EMPTY);
		if (bIsCombustibleItem(wTileNo))
		{
			//Fuse is burning at this tile.
			CueEvents.Add(CID_FuseBurning, new CMoveCoord(wX, wY,
					FuseEndAt(wX, wY, false)), true);
		}
	}
}

//*****************************************************************************
UINT CDbRoom::FuseEndAt(
//Returns: orientation of fuse piece at indicated square.
//If fuse is end piece, then returns direction to its end, else returns NO_ORIENTATION.
//
//Params:
	const UINT wCol, const UINT wRow,   //(in)
	const bool bLighting)					//[default=true]
const
{
	ASSERT(IsValidColRow(wCol, wRow));

	UINT wAdjFuseCount=0;
	UINT wOrientation = NO_ORIENTATION;
	UINT wTSquare;
	if (IsValidColRow(wCol-1,wRow))
	{
		wTSquare = GetTSquare(wCol-1, wRow);
		if (bIsFuseConnected(wTSquare) ||
			bIsFuseConnected(GetCoveredTSquare(wCol-1, wRow)))
		{
			wOrientation = E;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol+1,wRow))
	{
		wTSquare = GetTSquare(wCol+1, wRow);
		if (bIsFuseConnected(wTSquare) ||
			bIsFuseConnected(GetCoveredTSquare(wCol+1, wRow)))
		{
			if (wAdjFuseCount) return NO_ORIENTATION;
			wOrientation = W;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol,wRow-1))
	{
		wTSquare = GetTSquare(wCol, wRow-1);
		if (bIsFuseConnected(wTSquare) ||
			bIsFuseConnected(GetCoveredTSquare(wCol, wRow-1)))
		{
			if (wAdjFuseCount) return NO_ORIENTATION;
			wOrientation = S;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol,wRow+1))
	{
		wTSquare = GetTSquare(wCol, wRow+1);
		if (bIsFuseConnected(wTSquare) ||
			bIsFuseConnected(GetCoveredTSquare(wCol, wRow+1)))
		{
			if (wAdjFuseCount) return NO_ORIENTATION;
			wOrientation = N;
			++wAdjFuseCount;
		}
	}

	ASSERT(wAdjFuseCount <= 1);
	if (!wAdjFuseCount && bLighting) return N;	//can light isolated nubs
	return wOrientation;
}

//*****************************************************************************
void CDbRoom::LightFuseEnd(
//Light a fuse end on this tile.
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	const UINT wCol, const UINT wRow)   //(in)
{
	ASSERT(IsValidColRow(wCol, wRow));

	const UINT wTSquare = GetTSquare(wCol, wRow);
	if (wTSquare != T_FUSE) return;

	//Only fuse ends can be lit.
	if (FuseEndAt(wCol, wRow) == NO_ORIENTATION) return;

	LightFuse(CueEvents, wCol, wRow);
}

//Light fuse here if it's not already lit.
void CDbRoom::LightFuse(
	CCueEvents &CueEvents,
	const UINT wCol, const UINT wRow)
{
	if (!this->LitFuses.has(wCol, wRow))
	{
		if (this->NewFuses.insert(wCol, wRow))
			CueEvents.Add(CID_FuseBurning, new CMoveCoord(wCol, wRow, NO_ORIENTATION), true);
	}
}

void CDbRoom::RemoveFuse(const UINT wCol, const UINT wRow)
{
	this->LitFuses.erase(wCol, wRow);
}

//*****************************************************************************
FloorSpikesType CDbRoom::GetFloorSpikesState() const
{
	static const UINT SPIKE_CYCLE_SPEED = 10;

	if (!this->pCurrentGame)
		return FloorSpikesDown;

	const UINT turn = this->pCurrentGame->wSpawnCycleCount % SPIKE_CYCLE_SPEED;
	switch (turn) {
		case 0:
			if (!this->pCurrentGame->wSpawnCycleCount)
				return FloorSpikesDown; //down on turn 0
			if (!this->pCurrentGame->bHalfTurn)
				return FloorSpikesUp;
			return FloorSpikesDown;
		case SPIKE_CYCLE_SPEED-1: return FloorSpikesPoised;
		default: return FloorSpikesDown;
	}
}

void CDbRoom::ProcessSpikes(CCueEvents &CueEvents, bool bNoStab)
{
	if (this->floorSpikes.empty())
		return;

	switch (GetFloorSpikesState())
	{
		case FloorSpikesDown: break;
		case FloorSpikesPoised:
			if (!this->pCurrentGame->bHalfTurn) //drawn this way for entire turn, but cue event only fires once
			{
				CueEvents.Add(CID_SpikesPoised);

				//Redraw spikes in new position
				for (CCoordSet::const_iterator iter=this->floorSpikes.tiles.begin();
						iter != this->floorSpikes.tiles.end(); ++iter) {
					this->PlotsMade.insert(iter->wX, iter->wY);
				}
			}
		break;
		case FloorSpikesUp:
		{
			if (bNoStab)
				break; //don't stab either on half-turns, nor multiple times when ProcessTurn is called more than once on a single turn

			//Spikes are up -- damage things on them
			ASSERT(this->pCurrentGame);

			for (CCoordSet::const_iterator iter=this->floorSpikes.tiles.begin();
					iter != this->floorSpikes.tiles.end(); ++iter)
			{
				this->PlotsMade.insert(iter->wX, iter->wY); //redraw

				CueEvents.Add(CID_SpikesUp, new CCoord(iter->wX, iter->wY), true);

				const WeaponStab stab(iter->wX, iter->wY, NO_ORIENTATION, WT_FloorSpikes);
				this->pCurrentGame->StabRoomTile(stab, CueEvents);
			}
		}
		break;
	}
}

//*****************************************************************************
void CDbRoom::ProcessPuffAttack(
//Causes a Puff attack at (wX,wY).
//
//Params:
	CCueEvents &CueEvents,        //(in/out)
	const UINT wX, const UINT wY) //(in) square to attack
{
	ASSERT(IsValidColRow(wX, wY));

	const UINT wOTile = GetOSquare(wX,wY);
	switch(wOTile)
	{
		case T_WATER:
			Plot(wX, wY, T_THINICE);
			this->bridges.built(wX,wY,T_THINICE);
		break;
		case T_SHALLOW_WATER:
			Plot(wX, wY, T_THINICE_SH);
			this->bridges.built(wX,wY,T_THINICE_SH);
		break;
	}

	const UINT wTTile = GetTSquare(wX,wY);
	switch(wTTile)
	{
		//Puffs destroy briar
		case T_BRIAR_SOURCE:
			this->briars.removeSource(wX,wY);
		// fall-through
		case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			Plot(wX, wY, T_EMPTY);
		break;
		//Puff explosions extinguish fuses
		case T_MIRROR: case T_POWDER_KEG: case T_FUSE:
			RemoveFuse(wX, wY);
		break;
	}

	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster && !pMonster->IsPiece())
	{
		switch(pMonster->wType)
		{
			case M_CHARACTER:
			{
				const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
				ASSERT(pCharacter);
				if (!pCharacter->IsPuffImmune() && !pCharacter->IsInvulnerable())
				{
					KillMonster(pMonster, CueEvents);
					this->pCurrentGame->CheckTallyKill(pMonster);
				}
			}
			break;
			case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
			{
				CSerpent *pSerpent = dynamic_cast<CSerpent*>(pMonster);
				ASSERT(pSerpent);
				if (pSerpent->ShortenTail(CueEvents))
				{
					KillMonster(pSerpent,CueEvents);
					this->pCurrentGame->TallyKill();
				}
			}				
			break;
			default:
				if (bCanFluffKill(pMonster->GetResolvedIdentity()))
				{
					KillMonster(pMonster, CueEvents);
					this->pCurrentGame->CheckTallyKill(pMonster);
				}
			break;
		}
	}

	if (this->pCurrentGame && this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		const CSwordsman& player = this->pCurrentGame->swordsman;
		if (bCanFluffKill(player.wAppearance))
		{
			CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
			pGame->SetDyingEntity(&player);
			CueEvents.Add(CID_BriarKilledPlayer); //FIXME-Fluff: New CueEvent for being killed by a Puff?
		}
	}

	CueEvents.Add(CID_FluffPuffDestroyed, new CCoord(wX, wY), true);
}

void CDbRoom::ProcessFluffVents(CCueEvents &CueEvents)
//Actviate all Fluff Vents in the room, spawning Puffs and growing Fluff as appropriate
{
	if (this->fluffVents.empty())
		return;
	if (!this->pCurrentGame)
		return;
	if (this->pCurrentGame->wSpawnCycleCount % TURNS_PER_CYCLE || this->pCurrentGame->bHalfTurn)
		return;

	//Player's position.
	UINT wSManX = UINT(-1), wSManY = UINT(-1);
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom())
	{
		wSManX = player.wX;
		wSManY = player.wY;
	}

	CCoordSet newGrowth, newPuffs;
	CalcFluffGrowth(player, wSManX, wSManY, newGrowth, newPuffs, CueEvents);

	CCoordStack possible;
	if (!newGrowth.empty())
	{
		//Calculate currently growing Fluff
		CCoordSet contiguousFluff;
		for (CCoordSet::const_iterator vent=newGrowth.begin(); vent!=newGrowth.end(); ++vent)
		{
			if (!contiguousFluff.has(vent->wX, vent->wY))
			{
				CCoordSet tiles;
				GetTarConnectedComponent(vent->wX, vent->wY, tiles);
				contiguousFluff += tiles;
			}
		}

		//Calculate where fluff might grow.
		vector<tartype> added_tar(CalcRoomArea());
		UINT x, y;
		for (x = 0; x < this->wRoomCols; ++x)
			for (y = 0; y < this->wRoomRows; ++y)
			{
				//Determine where old fluff is.
				const UINT wTTile = GetTSquare(x, y);
				CMonster *pMonster = GetMonsterAtSquare(x, y);
				const UINT pos = ARRAYINDEX(x,y);  //shorthand
				if (wTTile == T_FLUFF)
				{
					added_tar[pos] = oldtar;
					continue;
				}
				added_tar[pos] = notar;
				//Determine whether new tarstuff can go here.
				if (bCanFluffGrowOn(GetOSquare(x, y)) &&
					bCanFluffGrowOn(GetFSquare(x, y)) &&
					bCanFluffGrowOn(wTTile) &&
					!(x == wSManX && y == wSManY) && !pMonster)
				{
					//Check orthogonal squares only
					for (UINT o = 1; o < ORIENTATION_COUNT; o += 2)
					{
						const int nx = x + nGetOX(o);
						const int ny = y + nGetOY(o);

						if (!IsValidColRow(nx, ny)) continue;
						if (GetTSquare(nx, ny) == T_FLUFF && contiguousFluff.has(nx,ny))
						{
							//Eligible fluff is adjacent to this square, so fluff might grow here.
							added_tar[pos] = newtar;
							possible.Push(x, y);
							break;
						}
					}
				}
			}

		//Calculate whether fluff or puffs are placed where fluff grows.
		UINT wCount=0;
		while (possible.Pop(x, y))
		{
			ASSERT(added_tar[ARRAYINDEX(x,y)] == newtar); //something is growing here
			if (NewTarWouldBeStable(added_tar,x,y))
			{
				//Fluff is growing here.  Destroy briar that might be in this square.
				const UINT wTTile = GetTSquare(x,y);
				if (wTTile == T_BRIAR_SOURCE)
					this->briars.removeSource(x,y);

				Plot(x, y, T_FLUFF);
				newPuffs.erase(x,y);
				contiguousFluff.insert(x,y);
				wCount=0;
			} else {
				//This tile might turn into a baby.
				possible.PushBottom(x,y);
				++wCount;
			}
			ASSERT(wCount <= possible.GetSize());
			if (wCount == possible.GetSize())
				break; //everything left in 'possible' at this point is unstable fluff
		}
	}

	UINT x, y;
	while(possible.Pop(x, y))
		newPuffs.insert(x,y);

	//CCoordSets are in T-B/L-R order.  Tarstuff monster order is in B-T/R-L order,
	//so we must reverse the order before spawning
	if (!newPuffs.empty())
	{
		CCoordStack spawnOrder;
		for (CCoordSet::const_iterator vent=newPuffs.begin(); vent!=newPuffs.end(); ++vent)
			spawnOrder.Push(vent->wX,vent->wY);

		while(spawnOrder.Pop(x, y))
		{
			const UINT wTTile = GetTSquare(x, y);
			//Destroy Briar here instead of spawning a Puff
			if (bIsBriar(wTTile))
			{
				ProcessPuffAttack(CueEvents, x, y);
				continue;
			}

			CMonster *m;
			m = AddNewMonster(M_FLUFFBABY,x,y);
			//FIXME-Fluff: Need CID_FluffBabyFormed
			m->bIsFirstTurn = true;
		}
	}
}

void CDbRoom::CalcFluffGrowth(
	const CSwordsman& player,
	const UINT wSManX, const UINT wSManY,
	CCoordSet& newGrowth, CCoordSet& newPuffs, CCueEvents &CueEvents)
{
	for (CCoordSet::const_iterator iter=this->fluffVents.begin();
			iter != this->fluffVents.end(); ++iter)
	{
		const UINT wX = iter->wX, wY = iter->wY;

		ASSERT(GetOSquare(wX,wY) == T_FLUFFVENT);
		const UINT wTSquare = GetTSquare(wX,wY);
		
		//Fluff on top of a vent will grow
		if (wTSquare == T_FLUFF)
		{
			newGrowth.insert(wX,wY);
			continue;
		}
		//Mark briar for destruction after growth
		if (bIsBriar(wTSquare))
		{
			newPuffs.insert(wX,wY);
			continue;
		}
		//All other TSquare items block the vent
		if (wTSquare != T_EMPTY)
			continue;

		//Check for monster/player obstacles
		CMonster *pMonster = GetMonsterAtSquare(wX,wY);
		ASSERT(this->pCurrentGame);
		bool bBlocked = false;
		if (pMonster)
		{
			if (pMonster->wType != M_FLUFFBABY)
			{
				ProcessPuffAttack(CueEvents,wX,wY);
				continue;
			}
			bBlocked = true;
		}
		if (wSManX == wX && wSManY == wY)
		{
			if (player.wAppearance != M_FLUFFBABY)
			{
				ProcessPuffAttack(CueEvents,wX,wY);
				continue;
			}
			bBlocked = true;
		}

		if (!bBlocked)
			newPuffs.insert(wX,wY);
	}
}

void CDbRoom::SolidifyFluff(CCueEvents &CueEvents)
//Turn puffs back into fluff, and create thin ice over water
{
	//Player's position.
	UINT wSManX = UINT(-1), wSManY = UINT(-1);
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom())
	{
		wSManX = player.wX;
		wSManY = player.wY;
	}

	for (CMonster *pFluff = pFirstMonster; pFluff != NULL; pFluff = pFluff->pNext)
	{
		if (pFluff->wType != M_FLUFFBABY)
			continue;

		const UINT wX = pFluff->wX;
		const UINT wY = pFluff->wY;
		if (!bCanPuffsMergeOn(GetOSquare(wX,wY)) ||
			!bCanPuffsMergeOn(GetFSquare(wX,wY)) ||
			!bCanPuffsMergeOn(GetTSquare(wX,wY)) ||
			(wX == wSManX && wY == wSManY))
			continue;

		bool tar[3][3] = {{false}};
		UINT x, y;

		//mark where fluff is
		for (x=wX-1; x!=wX+2; x++)
			if (x < wRoomCols)
				for (y=wY-1; y!=wY+2; y++)
					if (y < wRoomRows)
					{
						if (x == wX && y == wY)
							continue;

						bool bFluff = GetTSquare(x,y) == T_FLUFF;
						if (!bFluff)
						{
							if (x == wSManX && wY == wSManY)
								continue;

							CMonster *pMonster = GetMonsterAtSquare(x,y);
							if (pMonster && pMonster->wType == M_FLUFFBABY &&
								bCanPuffsMergeOn(GetOSquare(x,y)) &&
								bCanPuffsMergeOn(GetFSquare(x,y)) &&
								bCanPuffsMergeOn(GetTSquare(x,y)))
								bFluff = true;
						}
						tar[x-wX+1][y-wY+1] = bFluff;
					}

		if ((tar[0][0] && tar[0][1] && tar[1][0]) ||
			(tar[0][2] && tar[0][1] && tar[1][2]) ||
			(tar[2][0] && tar[2][1] && tar[1][0]) ||
			(tar[2][2] && tar[2][1] && tar[1][2]))
		{
			KillMonster(pFluff,CueEvents);
			Plot(wX,wY,T_FLUFF);
			CueEvents.Add(CID_PuffMergedIntoFluff, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
		}
	}

	//FIXME-Fluff: Thin Ice generation goes here.
}

//*****************************************************************************
void CDbRoom::ProcessActiveFiretraps(CCueEvents &CueEvents)
//Actviate all Fluff Vents in the room, spawning Puffs and growing Fluff as appropriate
{
	if (this->activeFiretraps.empty())
		return;
	if (!this->pCurrentGame)
		return;
	if (this->pCurrentGame->bHalfTurn)
		return;

	bool bFluffBurned = false;
	CCoordSet activeFiretraps;
	activeFiretraps.insert(this->activeFiretraps.begin(), this->activeFiretraps.end());

	for (CCoordSet::const_iterator iter = activeFiretraps.begin();
			iter != activeFiretraps.end(); ++iter)
	{
		ASSERT(bIsFiretrap(GetOSquare(iter->wX, iter->wY)));

		//Any Fluff we encounter will be destroyed within ActivateFiretrap,
		//but we should delay conversion of unstable Fluff until after all firetraps
		if (GetTSquare(iter->wX, iter->wY) == T_FLUFF)
			bFluffBurned = true;

		ActivateFiretrap(iter->wX, iter->wY, CueEvents);
	}

	if (bFluffBurned)
		ConvertUnstableTar(CueEvents);
}

//*****************************************************************************
void CDbRoom::ChangeTiles(const RoomTokenType tType)
//Traverses room squares to change each arrow only once.
{
	UINT wX, wY;
	switch (tType)
	{
		case RotateArrowsCW:
			//Rotate force arrows CW.
			for (wY=this->wRoomRows; wY--; )
				for (wX=this->wRoomCols; wX--; )
					switch (GetFSquare(wX,wY))
					{
						case T_ARROW_N: Plot(wX, wY, T_ARROW_NE); break;
						case T_ARROW_S: Plot(wX, wY, T_ARROW_SW); break;
						case T_ARROW_W: Plot(wX, wY, T_ARROW_NW); break;
						case T_ARROW_E: Plot(wX, wY, T_ARROW_SE); break;
						case T_ARROW_NW: Plot(wX, wY, T_ARROW_N); break;
						case T_ARROW_SW: Plot(wX, wY, T_ARROW_W); break;
						case T_ARROW_NE: Plot(wX, wY, T_ARROW_E); break;
						case T_ARROW_SE: Plot(wX, wY, T_ARROW_S); break;
						case T_ARROW_OFF_N: Plot(wX, wY, T_ARROW_OFF_NE); break;
						case T_ARROW_OFF_S: Plot(wX, wY, T_ARROW_OFF_SW); break;
						case T_ARROW_OFF_W: Plot(wX, wY, T_ARROW_OFF_NW); break;
						case T_ARROW_OFF_E: Plot(wX, wY, T_ARROW_OFF_SE); break;
						case T_ARROW_OFF_NW: Plot(wX, wY, T_ARROW_OFF_N); break;
						case T_ARROW_OFF_SW: Plot(wX, wY, T_ARROW_OFF_W); break;
						case T_ARROW_OFF_NE: Plot(wX, wY, T_ARROW_OFF_E); break;
						case T_ARROW_OFF_SE: Plot(wX, wY, T_ARROW_OFF_S); break;
						default: break;
					}
		break;
		case RotateArrowsCCW:
			//Rotate force arrows CCW.
			for (wY=this->wRoomRows; wY--; )
				for (wX=this->wRoomCols; wX--; )
					switch (GetFSquare(wX,wY))
					{
						case T_ARROW_N: Plot(wX, wY, T_ARROW_NW); break;
						case T_ARROW_S: Plot(wX, wY, T_ARROW_SE); break;
						case T_ARROW_W: Plot(wX, wY, T_ARROW_SW); break;
						case T_ARROW_E: Plot(wX, wY, T_ARROW_NE); break;
						case T_ARROW_NW: Plot(wX, wY, T_ARROW_W); break;
						case T_ARROW_SW: Plot(wX, wY, T_ARROW_S); break;
						case T_ARROW_NE: Plot(wX, wY, T_ARROW_N); break;
						case T_ARROW_SE: Plot(wX, wY, T_ARROW_E); break;
						case T_ARROW_OFF_N: Plot(wX, wY, T_ARROW_OFF_NW); break;
						case T_ARROW_OFF_S: Plot(wX, wY, T_ARROW_OFF_SE); break;
						case T_ARROW_OFF_W: Plot(wX, wY, T_ARROW_OFF_SW); break;
						case T_ARROW_OFF_E: Plot(wX, wY, T_ARROW_OFF_NE); break;
						case T_ARROW_OFF_NW: Plot(wX, wY, T_ARROW_OFF_W); break;
						case T_ARROW_OFF_SW: Plot(wX, wY, T_ARROW_OFF_S); break;
						case T_ARROW_OFF_NE: Plot(wX, wY, T_ARROW_OFF_N); break;
						case T_ARROW_OFF_SE: Plot(wX, wY, T_ARROW_OFF_E); break;
						default: break;
					}
		break;
		case TarTranslucent:
			//Rerender all the tarstuff.
			for (wY=this->wRoomRows; wY--; )
				for (wX=this->wRoomCols; wX--; )
					if (bIsTar(GetTSquare(wX,wY)))
						this->PlotsMade.insert(wX,wY);
			//NO BREAK
		case WeaponDisarm:
		case PowerTarget:
		case ConquerToken:
			//Toggle on-off state of all tokens of this type in the room.
			for (wY=this->wRoomRows; wY--; )
				for (wX=this->wRoomCols; wX--; )
					if (IsAnyTLayerObject(wX,wY, T_TOKEN))
					{
						const BYTE tParam = GetTParam(wX, wY);
						if ((RoomTokenType)calcTokenType(tParam) == tType)
						{
							SetTParam(wX, wY, tParam + TOKEN_ACTIVE);  //toggle on-off
							this->PlotsMade.insert(wX,wY);
						}
					}
		break;
		default: ASSERT(!"Unsupported token type"); break;
	}
}

//*****************************************************************************
void CDbRoom::CharactersCheckForCueEvents(CCueEvents &CueEvents)
//Called once all cue events on a given turn could have fired.
{
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->CheckForCueEvent(CueEvents);
		}
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
void CDbRoom::CheckForFallingAt(const UINT wX, const UINT wY, CCueEvents& CueEvents, bool bTrapdoorFell)
//Process any objects falling down at (x,y)
{
	UINT wOSquare = GetOSquare(wX,wY), wTSquare = GetTSquare(wX,wY);
	bool bStepStoneCreated = false;
	if (!bIsPit(wOSquare) && !bIsWater(wOSquare))
		return;  //no falling occurs here

	//These objects fall into pit/water.
	switch (wTSquare)
	{
		case T_FUSE:
		//Fuses break only when a trapdoor falls, not when a platform moves, a bridge falls, or a Citizen builds pit or water.
		if (!bTrapdoorFell)
			break;
		//else fall through...
		case T_BOMB: case T_ORB:
		case T_BEACON: case T_BEACON_OFF:
			Plot(wX, wY, T_EMPTY);
			if (bIsWater(wOSquare))
				CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
			else
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTSquare), true);
		break;
		//These types survive in shallow water
		case T_STATION: case T_SCROLL:
		case T_POTION_K: case T_POTION_I: case T_POTION_D: case T_POTION_C: case T_POTION_SP:
		case T_HORN_SQUAD: case T_HORN_SOLDIER:
			if (!bIsShallowWater(wOSquare))
			{
				Plot(wX, wY, T_EMPTY);
				if (bIsDeepWater(wOSquare))
					CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
				else
					CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTSquare), true);
			}
		break;
		case T_MIRROR:
		case T_POWDER_KEG:
			Plot(wX, wY, T_EMPTY);
			if (bIsWater(wOSquare))
			{
				if (bIsShallowWater(wOSquare))
				{
					Plot(wX, wY, T_STEP_STONE);
					bStepStoneCreated = true;
				}
				CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
			} else
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTSquare), true);
			break;
		case T_TAR: case T_MUD: case T_GEL:
			//Gel Babies may survive a fall into shallow water
			RemoveStabbedTar(wX, wY, CueEvents, false);
			CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTSquare), true);
		break;
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			if (bIsPit(wOSquare)) //remains on top of water without falling
			{
				Plot(wX, wY, T_EMPTY);
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTSquare), true);
			}
		break;
		default: break; //nothing else falls
	}
	if (bIsTLayerCoveringItem(wTSquare))
	{
		//What was under the object?
		wTSquare = GetTSquare(wX, wY);
		if (wTSquare == T_SCROLL || bIsPotion(wTSquare) || (wTSquare == T_FUSE && bTrapdoorFell))
			//A CID_ObjectFell was already created for the item, and the front end will animate that.  Just delete the second object.
			Plot(wX, wY, T_EMPTY);
	}
	//If a stepping stone was created, then no monsters that could possibly be standing on the mirror should fall
	if (bStepStoneCreated)
		return;

	//Player falls?
	if (this->pCurrentGame && this->pCurrentGame->wTurnNo > 0 && this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		const UINT wPlayerRole = this->pCurrentGame->swordsman.wAppearance;
		if (!bIsEntityFlying(wPlayerRole) &&
				!(bIsWater(wOSquare) && bIsEntitySwimming(wPlayerRole)) &&				
				!(bIsShallowWater(wOSquare) &&
				this->pCurrentGame->swordsman.GetWaterTraversalState() >= WTrv_CanWade))
		{
			this->pCurrentGame->SetDyingEntity(&this->pCurrentGame->swordsman);
			CueEvents.Add(bIsDeepWater(wOSquare) ? CID_PlayerDrownedInWater : CID_PlayerFellIntoPit);
		}
	}

	//If monster is over pit or water, then monster dies.
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (!pMonster)
		return;
	if (pMonster->wType == M_SEEP)
		return; //seep don't fall, and should die naturally on their turn
	if (pMonster->IsFlying())
		return; //flying monsters don't fall
	if (pMonster->IsSwimming() && bIsWater(wOSquare))
		return; //water-based monsters don't die in water

	if (bIsShallowWater(wOSquare))
	{
		if (bIsRockGolemType(pMonster->wType) && !pMonster->IsAlive())
		{
			Plot(wX, wY, T_STEP_STONE);
			CueEvents.Add(CID_Splash, new CCoord(pMonster->wX, pMonster->wY), true);
			KillMonster(pMonster, CueEvents);
			if (this->pCurrentGame)
				this->pCurrentGame->TallyKill();
			return;
		}
		if (pMonster->CanWadeInShallowWater())
			return; //monsters that can wade in shallow water don't die
	}

	//Handle large monsters specially based on type.
	pMonster = pMonster->GetOwningMonster();
	if (LargeMonsterFalls(pMonster, wX, wY, CueEvents))
		return;

	//Non-flying monster falls and dies.
	AddFallingMonsterEvent(CueEvents, pMonster, wOSquare);

	KillMonster(pMonster, CueEvents);
	if (this->pCurrentGame)
		this->pCurrentGame->CheckTallyKill(pMonster);
}

//*****************************************************************************
bool CDbRoom::AddFallingMonsterEvent(
	CCueEvents& CueEvents, CMonster* pMonster, const UINT wOSquare)
const
{
	if (bIsWater(wOSquare)) {
		CueEvents.Add(CID_Splash, new CCoord(pMonster->wX, pMonster->wY), true);
		return true;
	}
	if (bIsPit(wOSquare)) {
		UINT id = pMonster->GetResolvedIdentity();

		//Flag special monster appearance.
		if (pMonster->wType == M_EYE && pMonster->IsAggressive())
			id = M_EYE_ACTIVE;

		//Brain and nest characters can have an orientation, which we don't want
		UINT wO = (id == M_BRAIN || id == M_SKIPPERNEST) ? NO_ORIENTATION : pMonster->wO;

		//Use custom monster type for characters.
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			id = pCharacter->wLogicalIdentity;
		}

		CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(pMonster->wX, pMonster->wY,
				wO, M_OFFSET + id), true);
		return true;
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::LargeMonsterFalls(CMonster* &pMonster, const UINT wX, const UINT wY, CCueEvents& CueEvents)
//Handles special falling logic for monsters occupying more than one tile
//
//Returns: whether done processing fall logic for large monster
{
	ASSERT(pMonster);
	if (!pMonster->IsLongMonster())
		return false;

	if (pMonster->wType == M_ROCKGIANT)
	{
		//Splitters can wade in shallow water
		UINT wOSquare = GetOSquare(wX,wY);
		if (bIsShallowWater(wOSquare))
			return true; //nothing else happens during fall

		//Splitter breaks into pieces.
		KillMonster(pMonster, CueEvents);
		this->pCurrentGame->TallyKill();
		CRockGiant::Shatter(CueEvents, this->pCurrentGame, pMonster->wX,
				pMonster->wY, true); //allow forming monster shards over pit/water

		pMonster = GetMonsterAtSquare(wX, wY); //get resultant piece
		ASSERT(pMonster);
		return false; //more fall processing to do by caller
	}

	ASSERT(bIsSerpentOrGentryii(pMonster->wType));

	//Serpent/gentryii dies if its head is over deep water or pit.
	const UINT wOTileAtHead = GetOSquare(pMonster->wX, pMonster->wY);
	if (bIsDeepWater(wOTileAtHead))
	{
		if (bIsSerpent(pMonster->wType))
			CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(
					pMonster->wX, pMonster->wY, S), true);
		return false; //more fall processing to do by caller
	}

	const bool bFallsInPit = bIsPit(wOTileAtHead);
	if (bFallsInPit) {
		//All of monster falls into pit -- display each piece falling into pit.
		if (pMonster->wType == M_GENTRYII) {
			GentryiiFallsInPit(pMonster->wX, pMonster->wY,
					pMonster->Pieces.begin(), pMonster->Pieces.end(), CueEvents);
		} else {
			for (MonsterPieces::iterator pieceIt = pMonster->Pieces.begin();
					pieceIt != pMonster->Pieces.end(); ++pieceIt)
			{
				const CMonsterPiece& piece = **pieceIt;
				if (bIsPit(GetOSquare(piece.wX, piece.wY))) {
					CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(
							piece.wX, piece.wY, pMonster->wType,
							piece.wTileNo), true);
				}
			}
		}
		return false; //more fall processing to do by caller
	}

	//Head is on solid ground.
	//Break off the tail at the first point where there is no ground.
	bool bDiedFromTruncation = false;

	CSerpent *pSerpent = dynamic_cast<CSerpent*>(pMonster);
	if (pSerpent)
		pSerpent->OrderPieces(); //necessary for the following iteration to work properly

	UINT wPrevX = pMonster->wX, wPrevY = pMonster->wY;
	for (MonsterPieces::iterator pieceIt = pMonster->Pieces.begin();
			pieceIt != pMonster->Pieces.end(); ++pieceIt)
	{
		const CMonsterPiece& piece = **pieceIt;
		const UINT wPX = piece.wX, wPY = piece.wY;
		const UINT wOTile = GetOSquare(wPX, wPY);
		if (!bIsPit(wOTile) && !bIsDeepWater(wOTile)) {
			wPrevX = piece.wX;
			wPrevY = piece.wY;
			continue;
		}

		//Break off this tile and everything thereafter.
		if (pSerpent) {
			//Show falling effect for all pieces breaking off.
			for (MonsterPieces::const_iterator breakoff_piece = pieceIt;
				breakoff_piece != pMonster->Pieces.end(); ++breakoff_piece)
			{
				const CMonsterPiece& piece = **breakoff_piece;
				if (bIsPit(GetOSquare(piece.wX, piece.wY)))
					CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(
							piece.wX, piece.wY, pMonster->wType,
							piece.wTileNo), true);
			}

			//Shorten tail to just past this point.
			while (pSerpent->tailX != wPX || pSerpent->tailY != wPY)
			{
				if (pSerpent->ShortenTail(CueEvents))
					bDiedFromTruncation = true;
			}
			if (pSerpent->ShortenTail(CueEvents))
				bDiedFromTruncation = true;
		} else {
			ASSERT(pMonster->wType == M_GENTRYII);

			//Show falling effect for all pieces breaking off.
			const UINT pieces_removed = GentryiiFallsInPit(
				wPrevX, wPrevY, pieceIt, pMonster->Pieces.end(), CueEvents);

			for (UINT i=0; i<pieces_removed; ++i)
			{
				CMonsterPiece* pPiece = pMonster->Pieces.back();
				Plot(pPiece->wX, pPiece->wY, T_NOMONSTER);
			}
		}

		//no more pieces left to check at this point
		break;
	}
	if (bDiedFromTruncation)
	{
		//Kill serpent, but don't show it falling.
		KillMonster(pMonster, CueEvents);
		this->pCurrentGame->TallyKill();
	}
	return true; //don't need the fall/kill processing done by caller
}

//*****************************************************************************
UINT CDbRoom::GentryiiFallsInPit(
	UINT wPrevX, UINT wPrevY, //location of piece before pieceIt
	MonsterPieces::iterator pieceIt, //consider falling from this item in the pieces list
	MonsterPieces::const_iterator pieces_end,
	CCueEvents& CueEvents)
{
	UINT pieces_removed = 0;
	for ( ;	pieceIt != pieces_end; ++pieceIt)
	{
		const CMonsterPiece& piece = **pieceIt;
		if (bIsPit(GetOSquare(piece.wX, piece.wY))) {
			//the offset indicates this is a body piece instead of the head so the front-end can render it correctly
			UINT wO = nGetO(int(piece.wX - wPrevX), int(piece.wY - wPrevY)) + ORIENTATION_COUNT;

			CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(
					piece.wX, piece.wY, wO, T_GENTRYII), true);

			MonsterPieces::const_iterator nextPiece = pieceIt;
			if (++nextPiece != pieces_end) {
				wO = nGetO(int(piece.wX - (*nextPiece)->wX), int(piece.wY - (*nextPiece)->wY)) + ORIENTATION_COUNT;
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(
						piece.wX, piece.wY, wO, T_GENTRYII), true);
			}
		}

		wPrevX = piece.wX;
		wPrevY = piece.wY;

		++pieces_removed;
	}

	return pieces_removed;
}

//*****************************************************************************
bool CDbRoom::CanMovePlatform(const UINT wX, const UINT wY, const UINT wO)
//Returns: whether platform at (x,y) can move in specified direction.
{
	CPlatform *pPlatform = GetPlatformAt(wX,wY);
	ASSERTP(pPlatform, "No platform here.");
	if (!pPlatform)
		return false;
	return pPlatform->CanMove(wO);
}

//*****************************************************************************
void CDbRoom::MovePlatform(const UINT wX, const UINT wY, const UINT wO)
//Move platform at (x,y) in specified direction.
{
	CPlatform *pPlatform = GetPlatformAt(wX,wY);
	ASSERTP(pPlatform, "No platform here.");
	if (pPlatform)
		pPlatform->Move(wO);
}

//*****************************************************************************
void CDbRoom::PreprocessMonsters(CCueEvents& CueEvents)
//Called on player room entrance to process character commands that don't
//require a turn to execute (e.g. visibility, gotos, speech).
{
	CMonster *pMonster;
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->SetWeaponSheathed();
			pCharacter->Process(CMD_WAIT, CueEvents);
			this->pCurrentGame->PostProcessCharacter(pCharacter, CueEvents);
		}
		else if (pMonster->wType == M_SPIDER)
		{
			//Spiders far from player always start invisible.
			CSpider *pSpider = DYN_CAST(CSpider*, CMonster*, pMonster);
			pSpider->SetVisibility();
		}
	}

	//Clone sword state can be affected by what role the player is set to, so
	//perform a second pass for sword-carrying entities once characters have processed.
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (bEntityHasSword(pMonster->wType))
		{
			CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
			pArmedMonster->SetWeaponSheathed();
		}
	}
}

//*****************************************************************************
bool CDbRoom::PressurePlateIsDepressedBy(const UINT item)
//Returns: whether item can depress a pressure plate
{
	switch (item)
	{
		case T_BOMB: case T_MIRROR:
		case T_POWDER_KEG:
		case T_TAR: case T_MUD: case T_GEL:
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			return true;
		default: return false;
	}
}

//*****************************************************************************
void CDbRoom::ProcessTurn(CCueEvents &CueEvents, const bool bFullMove)
//A prioritized list of general room changes that are checked each game turn.
{
	//Bridges fall before anything else happens.
	this->bridges.process(CueEvents);

	if (CueEvents.HasOccurred(CID_EvilEyeWoke))
		this->bTarWasStabbed = true;	//indicates room has entered a "dangerous" state

	//These only happen on full turn increments.
	if (bFullMove)
	{
		BurnFuses(CueEvents);
		//Briar advances after any black doors were possibly opened.
		this->briars.process(CueEvents);
	} else {
		//Signal fuses burning, but don't alter them.
		BurnFuseEvents(CueEvents);
	}

	if (!bFullMove) {
		ProcessSpikes(CueEvents, true);
	} else {
		//Grow the tar flavors in response to a cue event.
		const bool bTarstuffGrowthSignalled = CueEvents.HasOccurred(CID_TarGrew) || CueEvents.HasOccurred(CID_MudGrew) || CueEvents.HasOccurred(CID_GelGrew);
		const bool bRoomHadNoTar = this->wTarLeft == 0;
		if (bTarstuffGrowthSignalled)
		{
			//Get coord index with all swords for quick evaluation.
			CCoordIndex babies(this->wRoomCols, this->wRoomRows), SwordCoords;
			GetSwordCoords(SwordCoords);

			//Tar takes precedence over mud, and mud over gel,
			//but all grow simultaneously if possible,
			//i.e. stable mud takes precedence over spawned tar babies, etc.
			if (CueEvents.HasOccurred(CID_TarGrew))
				GrowTar(CueEvents, babies, SwordCoords, T_TAR);
			if (CueEvents.HasOccurred(CID_MudGrew))
				GrowTar(CueEvents, babies, SwordCoords, T_MUD);
			if (CueEvents.HasOccurred(CID_GelGrew))
				GrowTar(CueEvents, babies, SwordCoords, T_GEL);
		}
		ProcessFluffVents(CueEvents);
		SolidifyFluff(CueEvents);

		ProcessSpikes(CueEvents);

		if (bTarstuffGrowthSignalled)
		{
			//If there was no tar in the room, and now it somehow grew (e.g. from a mother that wasn't on tar),
			//toggle black gates again.
			if (bRoomHadNoTar && this->wTarLeft)
			{
				ToggleBlackGates(CueEvents);
				ConvertUnstableTar(CueEvents);
			}
		}

		ResetUnloadedPressurePlates(CueEvents);

		//Firetraps activate based on latest pressure plate state.
		ProcessActiveFiretraps(CueEvents);
	}

	this->PostProcessTurn(CueEvents, bFullMove);
}

//*****************************************************************************
void CDbRoom::PostProcessTurn(CCueEvents &CueEvents, const bool bFullMove)
// Process some things which need to happen after all room-state-changing things are finished 
{
	// This flag is used in a situation where tar mother in a room with 0 tar grows but its tar is
	// then destroyed by, for example, spike-induced keg explosion, which normally would cause
	// tar gates to be toggled ONCE
	this->bTarstuffGateTogglePending = false;

	//Seep die only after pressure plates may have triggered
	KillMonstersOnHazard(CueEvents);

	//Process gaze after tarstuff has grown
	//and released pressure plates have possibly changed door states.
	ProcessAumtlichGaze(CueEvents, bFullMove);

	//Clear first turn status of current monsters.  First turn flag indicates that
	//a monster was created and added to the monster list during processing, but is
	//not ready to be processed yet.  Clearing it here will allow the monster to be
	//processed the next time ProcessMonsters() is called.
	ResetMonsterFirstTurnFlags();
}
//*****************************************************************************
void CDbRoom::ResetUnloadedPressurePlates(CCueEvents &CueEvents)
{
	//If depressed pressure plate has nothing on its surface, it resets.
	//Make sure the order in which this happens is predictable from the
	//room layout, for the rare case where it makes a difference.
	typedef std::map<UINT, COrbData*> plateOrder_t;
	plateOrder_t plateOrder;
	UINT wSwIndex = static_cast<UINT>(-1);
	if (!bIsEntityFlying(this->pCurrentGame->swordsman.wAppearance))
	{
		wSwIndex = this->pressurePlateIndex.GetAt(
			this->pCurrentGame->swordsman.wX,this->pCurrentGame->swordsman.wY) - 1;
	}
	for (UINT wIndex=this->orbs.size(); wIndex--; )
	{
		if (wIndex == wSwIndex)
			continue; //player is on this pressure plate -- no more checks needed
		COrbData *pOrb = this->orbs[wIndex];
		if (pOrb->bActive)
		{
			bool bEmpty = true;
			//wLowestInd will track the first tile in
			//left-to-right-then-top-to-bottom order (like spawn order)
			//CCoordSet::first() does T-to-B-then-L-to-R, less expected.
			UINT wLowestInd = CalcRoomArea();
			for (CCoordSet::const_iterator tile=pOrb->tiles.begin();
					bEmpty && tile!=pOrb->tiles.end(); ++tile)
			{
				//These objects can depress a pressure plate.
				CMonster *pMonster = GetMonsterAtSquare(tile->wX,tile->wY);
				if (pMonster && !(pMonster->IsFlying() || pMonster->wType == M_SEEP))
					bEmpty = false;
				else if (PressurePlateIsDepressedBy(GetTSquare(tile->wX,tile->wY)))
					bEmpty = false;
				else
				{
					const UINT wInd = ARRAYINDEX(tile->wX,tile->wY);
					if (wInd < wLowestInd)
						wLowestInd = wInd;
				}
			}
			if (bEmpty) //pressure plate is released
				plateOrder.insert(plateOrder_t::value_type(wLowestInd, pOrb));
		}
	}

	for (plateOrder_t::const_iterator plate = plateOrder.begin();
			plate != plateOrder.end(); ++plate)
		ActivateOrb(plate->second->wX, plate->second->wY, CueEvents, OAT_PressurePlateUp);
}

//*****************************************************************************
void CDbRoom::ExpandExplosion(
//Determines how an explosion can expand based on the normal constraints:
// 1. How far an explosion can go.
// 2. What can be destroyed by an explosion.
//If constraints are met, explosion continues at square (wX,wY), according to
// 3. Direction of explosion.
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	CCoordStack& cs,           //(in/out) valid explosion squares added here
	const UINT wBombX, const UINT wBombY,  //(in) source of explosion
	const UINT wX, const UINT wY, //(in) square to place explosion
	CCoordStack& bombs,        //(in/out) bombs to be exploded
	CCoordStack& powder_kegs,  //(in/out) bombs to be exploded
	CCoordSet& explosion,      //(in/out) tiles needed to be destroyed/activated
	const CCoordIndex& caberCoords,   //(in) list of tiles containing cabers and their directions
	const UINT radius)
{
	if (!IsValidColRow(wX, wY))
		return;

	//Constraint 1. How far explosion can go.
	const UINT dist = nDist(wX,wY,wBombX,wBombY);
	if (dist > radius)
		return;

	const UINT direction = GetOrientation(wBombX, wBombY, wX, wY);
	UINT wTileNo = GetFSquare(wX,wY);

	if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo,direction))
		return; //force arrows in wrong direction stop the blast

	//Constraint 2. What can be destroyed by an explosion.
	switch (GetOSquare(wX,wY))
	{
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_PIT: case T_PIT_IMAGE: case T_PLATFORM_P:
		case T_WATER: case T_SHALLOW_WATER: case T_STEP_STONE: case T_PLATFORM_W:
		case T_STAIRS:	case T_STAIRS_UP:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE: case T_THINICE_SH:
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_HOT: case T_GOO: case T_PRESSPLATE:
		case T_WALL_B: case T_WALL_H:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
			//Bomb blast goes over/through these things
			break;
		default: return;  //everything else stops bomb blast
	}

	wTileNo = GetTSquare(wX,wY);
	switch (wTileNo)
	{
		case T_BOMB:
			//if we haven't considered this bomb before, then add it
			if (!explosion.has(wX,wY))
				bombs.Push(wX,wY);
			break;
		case T_POWDER_KEG:
			//if we haven't considered this one before, add it
			if (!explosion.has(wX,wY))
				powder_kegs.Push(wX,wY);
			break;
		case T_OBSTACLE:
			//blocks explosion, but because obstacles can be placed
			//on walls, we still need to consider this square
			explosion.insert(wX,wY);
			return;
		case T_ORB: case T_BEACON: case T_BEACON_OFF:
			//orbs/beacons stop blasts, but must still be activated
			explosion.insert(wX,wY);
			return;
		case T_TAR: case T_MUD: case T_GEL:
			//Tarstuff explosions need to have a direction of explosion,
			//so let's do that now providing we haven't done this tile yet.
			if (!explosion.has(wX,wY))
				CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, direction, wTileNo), true);
			break;
		case T_FLUFF:
			if (!explosion.has(wX,wY))
				CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wX, wY, direction, wTileNo), true);
			break;
		default:
			break;
	}

	//Add tile to explosion area-of-effect
	explosion.insert(wX,wY);

	//Constraint 3. Direction of explosion determines where explosion will
	//spread out to: (1) when coming in a direction horizontal or vertical
	//direction from the bomb, allow fan out in three directions,
	//(2) otherwise only keep moving outward in one diagonal direction.
	const int oBlastX = nGetOX(direction);
	const int oBlastY = nGetOY(direction);
	static const UINT dirmask[] = {DMASK_NW, DMASK_N, DMASK_NE, DMASK_W, 0, DMASK_E, DMASK_SW, DMASK_S, DMASK_SE};
	wTileNo = GetFSquare(wX,wY);
	for (UINT o = 0; o < ORIENTATION_COUNT; ++o)
	{
		if (o == NO_ORIENTATION) continue;
		const int oX = nGetOX(o);
		const int oY = nGetOY(o);
		if ((oBlastX == 0 || (oBlastX * oX) > 0) &&
			 (oBlastY == 0 || (oBlastY * oY) > 0))
		{
			//Valid direction.  Check if arrow blocks it.
			if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo, o))
				continue;

			//Check if caber on this tile blocks it.
			const UINT wCaberO = caberCoords.GetAt(wX,wY);
			if (wCaberO && (dirmask[nGetO(oX,oY)] & wCaberO))
				continue;

			const UINT wNewX = wX + oX;
			const UINT wNewY = wY + oY;

			if (!IsValidColRow(wNewX,wNewY))
				continue;
			//Check if caber on new tile blocks it.
			if (caberCoords.Exists(wNewX,wNewY))
				continue;

			//Otherwise, add to list.
			cs.Push(wNewX, wNewY);
		}
	}
}

//*****************************************************************************
void CDbRoom::ProcessExplosionSquare(
//Causes explosion effects at (wX,wY).
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	const UINT wX, const UINT wY, //(in) square to place explosion
	const bool bKillsFegundo,     //[default=true]
	const bool bAddCueEvent) //[default=true]
{
	ASSERT(IsValidColRow(wX, wY));

	//What is affected by an explosion on this tile:
	switch (GetOSquare(wX,wY))
	{
		case T_WALL_B: case T_WALL_H:
			ASSERT(bIsPlainFloor(this->coveredOSquares.GetAt(wX, wY)));
			Plot(wX,wY,this->coveredOSquares.GetAt(wX, wY));
			break;

		case T_THINICE: case T_THINICE_SH:
			DestroyTrapdoor(wX,wY,CueEvents);
		break;

		//If we decided we should explode these squares, then they should
		//be dealt with, regardless of whether these tiles are impassable now.
		case T_DOOR_Y: case T_DOOR_M: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B:
		default: break;
	}

	UINT wTileNo = GetTSquare(wX,wY);
	// Destroy covering item first, and then deal with anything that was under it.
	if (bIsTLayerCoveringItem(wTileNo))
	{
		Plot(wX,wY,T_EMPTY);
		wTileNo = GetTSquare(wX,wY);
	}
	switch (wTileNo)
	{
		case T_OBSTACLE:
			// We allowed the explosion to process underneath an
			// obstacle and possibly break a crumbly wall, but
			// don't add the CueEvent to trigger the animation.
		return;
		case T_FUSE:
			LightFuse(CueEvents, wX, wY);
		break;
		case T_ORB:
			//Orbs activate from the explosion
			ActivateOrb(wX, wY, CueEvents, OAT_Item);
		break;
		case T_BEACON: case T_BEACON_OFF:
			//Beacons activate from the explosion
			ActivateBeacon(wX, wY, CueEvents);
		break;
		case T_TAR:	case T_MUD: case T_GEL: case T_FLUFF:
						// Remove tarstuff.  A CID_TarstuffDestroyed was added
						// in ExpandExplosion(), since that method can add a
						// direction for the animation.
			RemoveStabbedTar(wX,wY, CueEvents);
		break;
		case T_BRIAR_SOURCE:
			//Briar roots are destroyed.
			this->briars.removeSource(wX,wY);
			Plot(wX,wY,T_EMPTY);
		break;
		case T_BOMB: //Bombs explode
		case T_BRIAR_DEAD: case T_BRIAR_LIVE: //Briar tiles are destroyed.
		case T_STATION: //destroyed
			Plot(wX,wY,T_EMPTY);
		break;
		default:
			if (bIsPotion(wTileNo))
			{
				//potions are destroyed
				Plot(wX,wY,T_EMPTY);
			}
			//Explosion passes through everything else.
		break;
	}

	if (bAddCueEvent)
		CueEvents.Add(CID_Explosion, new CCoord(wX, wY), true);

	//Monsters (and swordsman) are killed on the way.
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster && (bKillsFegundo || pMonster->wType != M_FEGUNDO))
	{
		ScriptFlag::Imperative eImperative = ScriptFlag::Vulnerable;
		bool bRebirthFegundo = false;
		bool bShatterRockGiant = false;
		//Blow up entire serpent when its head is hit.
		if (pMonster->IsPiece())
		{
			CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
			pMonster = pPiece->pMonster;
			if (bIsSerpentOrGentryii(pMonster->wType))
				eImperative = ScriptFlag::Invulnerable; //can't harm serpent bodies
		}
		switch (pMonster->wType)
		{
			case M_CHARACTER:
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				if (pCharacter->IsInvulnerable() || pCharacter->IsExplosionImmune()) {
					eImperative = ScriptFlag::Invulnerable;
				} else if (pCharacter->IsBriarImmune()) {
					// If a briar blocking NPC is killed, act as if a tile has been plotted.
					this->briars.plotted(wX, wY, T_EMPTY);
				}
			}
			break;
			case M_FEGUNDOASHES:
				//Don't destroy ashes on the same turn they are created.
				if (pMonster->bIsFirstTurn)
					eImperative = ScriptFlag::Invulnerable;
			break;
			case M_FEGUNDO:
			{
				//Prevent FegundoToAsh from being called
				if (CueEvents.Remove(CID_FegundoToAsh,pMonster) &&
						CueEvents.GetOccurrenceCount(CID_FegundoToAsh) == 0)
					CueEvents.ClearEvent(CID_FegundoToAsh);					
				bRebirthFegundo = true;
			}
			break;
			case M_ROCKGIANT:
				bShatterRockGiant = true;
			break;
			case M_GENTRYII:
				eImperative = ScriptFlag::Invulnerable;
			break;
			default: break;
		}
		if (eImperative != ScriptFlag::Invulnerable)
		{
			UINT wNewX = pMonster->wX, wNewY = pMonster->wY;

			KillMonster(pMonster, CueEvents);
			if (this->pCurrentGame)
				this->pCurrentGame->CheckTallyKill(pMonster);

			if (bRebirthFegundo)
			{
				const UINT wOTile = GetOSquare(wNewX, wNewY);
				switch(wOTile)
				{
					case T_PIT: case T_PIT_IMAGE:
						CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wNewX, wNewY,
							S, M_OFFSET + M_FEGUNDOASHES), true);
					break;
					case T_WATER: case T_SHALLOW_WATER:
						CueEvents.Add(CID_Splash, new CCoord(wNewX,wNewY), true);
					break;
					default:
						CMonster *pAshes = AddNewMonster(M_FEGUNDOASHES, wNewX, wNewY);
						pAshes->bIsFirstTurn = true;
						if (wOTile == T_PRESSPLATE)
							ActivateOrb(wNewX, wNewY, CueEvents, OAT_PressurePlate);
				}
			}
			else if (bShatterRockGiant && this->pCurrentGame)
				CRockGiant::Shatter(CueEvents, this->pCurrentGame, wNewX, wNewY);
		}
	}
	if (this->pCurrentGame && this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		this->pCurrentGame->SetDyingEntity(&this->pCurrentGame->swordsman);
		CueEvents.Add(CID_ExplosionKilledPlayer);
	}
}

//*****************************************************************************
void CDbRoom::ProcessAumtlichGaze(CCueEvents &CueEvents, const bool bFullMove)
//Determine which squares are affected by aumtlich in room.
//Do freeze player/aumtlich check.
{
	// Do not process on turn 0, even after double placement
	if (this->pCurrentGame->wPlayerTurn == 0)
		return;

	//Remove all previous gaze effects.
	this->pCurrentGame->swordsman.bFrozen = false;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		switch (pMonster->wType)
		{
			case M_MIMIC:
			case M_STALWART: case M_STALWART2:
			case M_TEMPORALCLONE:
			{
				CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
				pDouble->bFrozen = false;
			}
			break;
			case M_AUMTLICH:
			{
				CAumtlich *pAumtlich = DYN_CAST(CAumtlich*, CMonster*, pMonster);
				pAumtlich->bFrozen = false;
			}
			break;
			default: break;
		}
		pMonster = pMonster->pNext;
	}

	CCoordIndex *pSwordCoords = NULL;
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_AUMTLICH)
		{
			//Get sword coords once for quick evaluation.
			if (!pSwordCoords)
			{
				pSwordCoords = new CCoordIndex();
			   GetSwordCoords(*pSwordCoords);
			}
			//Update aumtlich's line of sight.
			CAumtlich *pAumtlich = DYN_CAST(CAumtlich*, CMonster*, pMonster);
			pAumtlich->UpdateGaze(CueEvents, *pSwordCoords, bFullMove);
		}
	}
	delete pSwordCoords;
}

//*****************************************************************************
void CDbRoom::PushTLayerObject(
//Push an object from the source to destination tiles.
//
//Params:
	const UINT wSrcX, const UINT wSrcY,
	const UINT wDestX, const UINT wDestY,
	CCueEvents& CueEvents) //(in/out)
{
	const int dx = int(wDestX) - int(wSrcX);
	const int dy = int(wDestY) - int(wSrcY);
	ASSERT(abs(dx) <= 1 && abs(dy) <= 1);

	const UINT wTile = GetTSquare(wSrcX, wSrcY);

	//Destroy Puffs at destination square
	CMonster *pMonster = GetMonsterAtSquare(wDestX, wDestY);
	if (pMonster && pMonster->wType == M_FLUFFBABY)
	{
		KillMonster(pMonster,CueEvents);
		ProcessPuffAttack(CueEvents, wDestX, wDestY);
	}

	//Check for a weapon at destination location that might break the mirror
	switch (wTile) {
	case T_MIRROR:
	{
		UINT wO = nGetO(dx,dy);
		for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO) {
			if (nO != NO_ORIENTATION && nO != wO)
			{
				CMonster *pMonster = GetMonsterAtSquare(wDestX-nGetOX(nO), wDestY-nGetOY(nO));
				if (pMonster && pMonster->HasSwordAt(wDestX, wDestY))
				{
					bool bShatter = false;
					const UINT wSO = nGetReverseO(nO);
					switch (pMonster->GetWeaponType())
					{
						case WT_Sword:
						case WT_Dagger:
							if (wSO == wO)
								bShatter = true;
						break;
						case WT_Pickaxe:
							if (wSO != wO && nNextCO(wSO) != wO && nNextCCO(wSO) != wO)
								bShatter = true;
						break;
						case WT_Spear:
							if (wSO == wO || nNextCO(wSO) == wO || nNextCCO(wSO) == wO)
								bShatter = true;
						break;
						case WT_Staff:
							bShatter = false;
						break;
						case WT_Caber:
							bShatter = true;
						break;

						default: break;
					}
					if (bShatter)
					{
						Plot(wSrcX, wSrcY, RoomObject::emptyTile());
						CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wSrcX, wSrcY, wO), true);
						return;
					}
				}
			}
		}

		//Is the swordsman's weapon in the square?
		CSwordsman& player = this->pCurrentGame->swordsman;
		if (this->pCurrentGame->IsPlayerWeaponAt(wDestX, wDestY))
		{
			bool bShatter = false;
			const UINT wSO = nGetReverseO(player.wO);
			switch (player.GetActiveWeapon())
			{
				case WT_Sword:
				case WT_Dagger:
					if (wSO == wO)
						bShatter = true;
				break;
				case WT_Pickaxe:
					if (wSO != wO && nNextCO(wSO) != wO && nNextCCO(wSO) != wO && player.wO != wO)
						bShatter = true;
				break;
				case WT_Spear:
					if (wSO == wO || nNextCO(wSO) == wO || nNextCCO(wSO) == wO)
						bShatter = true;
				break;
				case WT_Staff:
					bShatter = false;
				break;
				case WT_Caber:
					bShatter = true;
				break;

				default: break;
			}
			if (bShatter)
			{
				Plot(wSrcX, wSrcY, RoomObject::emptyTile());
				CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wSrcX, wSrcY, wO), true);
				return;
			}
		}
	}
	break;
	case T_POWDER_KEG:
	{
		//If powder keg is pushed into a weapon, explode
		UINT wO = nGetO(dx,dy);
		for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO)
			if (nO != NO_ORIENTATION && nO != wO)
			{
				bool bExplode = false;
				CMonster *pMonster = GetMonsterAtSquare(wDestX-nGetOX(nO), wDestY-nGetOY(nO));
				if (pMonster && pMonster->HasSwordAt(wDestX, wDestY)) {
					const UINT wSO = nGetReverseO(nO);
					switch (pMonster->GetWeaponType())
					{
						case WT_Sword:
						case WT_Caber:
							bExplode = true;
						break;
						case WT_Dagger:
							if (wSO == wO)
								bExplode = true;
						break;
						case WT_Pickaxe:
							if (wSO != wO && nNextCO(wSO) != wO && nNextCCO(wSO) != wO)
								bExplode = true;
						break;
						case WT_Spear:
							if (wSO == wO || nNextCO(wSO) == wO || nNextCCO(wSO) == wO)
								bExplode = true;
						break;
						case WT_Staff:
							bExplode = false;
						break;

						default: break;
					}
					if (bExplode)
					{
						Plot(wSrcX, wSrcY, RoomObject::emptyTile());
						this->stabbed_powder_kegs.Push(wDestX, wDestY);
						return;
					}
				}
			}

		if (this->pCurrentGame->IsPlayerWeaponAt(wDestX, wDestY)) {
			CSwordsman& player = this->pCurrentGame->swordsman;
			bool bExplode = false;
			const UINT wSO = nGetReverseO(player.wO);
			switch (player.GetActiveWeapon())
			{
				case WT_Sword:
				case WT_Caber:
					bExplode = true;
				break;
				case WT_Dagger:
					if (wSO == wO)
						bExplode = true;
				break;
				case WT_Pickaxe:
					if (wSO != wO && nNextCO(wSO) != wO && nNextCCO(wSO) != wO && player.wO != wO)
						bExplode = true;
				break;
				case WT_Spear:
					if (wSO == wO || nNextCO(wSO) == wO || nNextCCO(wSO) == wO)
						bExplode = true;
				break;
				case WT_Staff:
					bExplode = false;
				break;

				default: break;
			}
			if (bExplode)
			{
				Plot(wSrcX, wSrcY, RoomObject::emptyTile());
				this->stabbed_powder_kegs.Push(wDestX, wDestY);
				return;
			}
		}
	}
	break;
	}

	//Determine whether object falls down at destination square.
	switch (GetOSquare(wDestX, wDestY))
	{
		case T_PIT: case T_PIT_IMAGE:
			Plot(wSrcX, wSrcY, RoomObject::emptyTile());
			CueEvents.Add(CID_ObjectFell, new CMoveCoordEx(wDestX, wDestY,
					NO_ORIENTATION, wTile), true);
		return;
		case T_WATER:
			Plot(wSrcX, wSrcY, RoomObject::emptyTile());
			CueEvents.Add(CID_Splash, new CCoord(wDestX, wDestY), true);
		return;
		case T_SHALLOW_WATER:
			//Covering items fall to create stepping stones
			Plot(wSrcX, wSrcY, RoomObject::emptyTile());
			if (bIsTLayerCoveringItem(wTile))
				Plot(wDestX, wDestY, T_STEP_STONE);
			CueEvents.Add(CID_Splash, new CCoord(wDestX, wDestY), true);
		return;
		default: break;
	}

	//Persist the pushed object's data,
	//so it can be globally referenced for the duration of turn processing.
	const UINT index = ARRAYINDEX(wSrcX, wSrcY);
	RoomObject *coveringObj = this->tLayer[index];
	ASSERT(coveringObj);
	RoomObject *uncoveredObj = coveringObj->uncover();
	if (uncoveredObj)
		this->tLayerObjects.push_back(uncoveredObj);
	this->tLayer[index] = uncoveredObj;

	coveringObj->move(wDestX, wDestY);

	const UINT destIndex = ARRAYINDEX(wDestX, wDestY);
	RoomObject *destObj = this->tLayer[destIndex];
	if (coveringObj->cover(destObj))
		RemoveTLayerObject(destObj);

	this->tLayer[destIndex] = coveringObj;
	this->pushed_objects.insert(coveringObj);

	if (wTile == T_POWDER_KEG)
		this->stationary_powder_kegs.erase(coveringObj);

	//Handle room state bookkeeping usually handled in ::Plot
	RecalcStationPaths();
	this->PlotsMade.insert(wSrcX,wSrcY);
	this->PlotsMade.insert(wDestX,wDestY);

	UpdatePathMapAt(wSrcX, wSrcY);
	UpdatePathMapAt(wDestX, wDestY);
	ReevalBriarNear(wSrcX, wSrcY, GetTSquare(wSrcX, wSrcY));
	ReevalBriarNear(wDestX, wDestY, wTile);

	if (GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
		ActivateOrb(wDestX, wDestY, CueEvents, OAT_PressurePlate);
}

//*****************************************************************************
bool CDbRoom::CanSetSwordsman(
//Returns: whether swordsman can be placed on this square
//(Used when checking whether room can be entered on this square.)
//
//Params:
	const UINT dwX, const UINT dwY,   //(in) square
	const bool bRoomConquered)          //(in) whether room is conquered [default=true]
const
{
	//Listed are all things swordsman cannot be standing on.
	//(Monsters (except serpents) and broken walls are removed when player enters room onto them,
	//so they are allowed.)
	const UINT wAppearance = this->pCurrentGame ?
			this->pCurrentGame->swordsman.wAppearance : static_cast<UINT>(M_BEETHRO);
	bool bCanEnter = true;
	switch (GetOSquare(dwX, dwY))
	{
		case T_WALL_M:
			bCanEnter = this->pCurrentGame && this->pCurrentGame->bHoldMastered;
			break;
		case T_WALL_WIN:
			bCanEnter = this->pCurrentGame && this->pCurrentGame->bHoldCompleted;
			break;
		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
			bCanEnter = wAppearance == M_SEEP;
			break;
		case T_PIT: case T_PIT_IMAGE:
			bCanEnter = bIsEntityFlying(wAppearance);
			break;
		case T_WATER:
			bCanEnter = bIsEntityFlying(wAppearance) || bIsEntitySwimming(wAppearance);
			break;
		case T_SHALLOW_WATER:
			bCanEnter = bIsEntityFlying(wAppearance) || bIsEntitySwimming(wAppearance) ||
				this->pCurrentGame->swordsman.GetWaterTraversalState() >= WTrv_CanWade;
			break;
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD:
		case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_STAIRS: case T_STAIRS_UP:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_STEP_STONE: case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE: case T_THINICE_SH:
		case T_PRESSPLATE: case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_GOO: case T_HOT:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
			bCanEnter = wAppearance != M_SEEP;
			break;
	}
	if (!bCanEnter)
		return false;

	const UINT t = GetTSquare(dwX, dwY);
	switch (t)
	{
		case T_ORB:
		case T_TAR:	case T_MUD: case T_GEL:
		case T_BOMB:
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
		case T_OBSTACLE:
		case T_BEACON: case T_BEACON_OFF:
			return false;
		default:
		case T_MIRROR: //is removed when player enters room on one
		case T_POWDER_KEG:
			break;
	}

	CMonster *pMonster = GetMonsterAtSquare(dwX, dwY);
	if (pMonster && bIsSerpent(pMonster->wType))
		return bRoomConquered;  //serpent will be gone if room has been cleared

	return true;
}

//*****************************************************************************
bool CDbRoom::SomeMonsterCanSmellSwordsman() const
//Returns: whether a monster can smell the player.
{
	return IsMonsterWithin(this->pCurrentGame->swordsman.wX,
			this->pCurrentGame->swordsman.wY, DEFAULT_SMELL_RANGE, false);
}

//*****************************************************************************
void CDbRoom::GetDepressablePlateSubset(
//Outputs subset of tiles that belong to pressure plates that may be depressed
//(i.e. they are not currently depressed already).
	const CCoordSet& plates, //(in)
	CCoordSet& depressablePlates) //(out) appended subset of 'plates' that may be depressed
const
{
	for (CCoordSet::const_iterator tile = plates.begin(); tile != plates.end(); ++tile)
	{
		COrbData *pPlate = GetPressurePlateAtCoords(tile->wX, tile->wY);
		if (pPlate && !pPlate->bActive && pPlate->eType != OT_BROKEN)
			depressablePlates.insert(tile->wX, tile->wY);
	}
}

//*****************************************************************************
COrbData* CDbRoom::GetOrbAtCoords(
//Gets an orb located within room at specified coords.
//
//Params:
	const UINT wX, const UINT wY) //(in) Square to look for orb at.
//
//Returns:
//Pointer to found orb or NULL if no match.
const
{
	ASSERT(IsValidColRow(wX, wY));
	for (UINT wOrbI=this->orbs.size(); wOrbI--; )
	{
		if (wX == this->orbs[wOrbI]->wX && wY == this->orbs[wOrbI]->wY)
			return this->orbs[wOrbI];
	}
	return NULL;
}

//*****************************************************************************
COrbData* CDbRoom::GetPressurePlateAtCoords(
//Gets an orb located within room at specified coords.
//
//Params:
	const UINT wX, const UINT wY) //(in) Tile to look at
//
//Returns:
//Pointer to found plate or NULL if no match.
const
{
	const UINT wIndex = this->pressurePlateIndex.GetAt(wX,wY);
	if (!wIndex)
		return NULL; //no pressure plate definition here

	ASSERT(wIndex <= this->orbs.size());
	return this->orbs[wIndex-1]; //base-1
}

//*****************************************************************************
CScrollData* CDbRoom::GetScrollAtSquare(const UINT wX, const UINT wY) const
//Returns: Pointer to scroll or NULL if no match.
{
	for (UINT wScrollI=this->Scrolls.size(); wScrollI--; )
	{
		if (wX == this->Scrolls[wScrollI]->wX && wY == this->Scrolls[wScrollI]->wY)
			return this->Scrolls[wScrollI];
	}
	return NULL;
}

//*****************************************************************************
const WCHAR* CDbRoom::GetScrollTextAtSquare(
//Gets scroll text of a scroll located within room at specified coords.
//
//Params:
	const UINT wX, const UINT wY) //(in) Square to look for scroll at.
//
//Returns:
//Pointer to text of scroll or NULL if no match.
const
{
	ASSERT(IsValidColRow(wX, wY));
	for (UINT wScrollI=this->Scrolls.size(); wScrollI--; )
	{
		if (wX == this->Scrolls[wScrollI]->wX && wY == this->Scrolls[wScrollI]->wY)
			return (const WCHAR*) this->Scrolls[wScrollI]->ScrollText;
	}
	return NULL;
}

//*****************************************************************************
void CDbRoom::MoveScroll(
//Moves scroll at specified coords to new coords.
//
//Params:
	const UINT wX, const UINT wY, //(in) Square to look for scroll at.
	const UINT wNewX, const UINT wNewY) //(in) Square to look for scroll at.
{
	ASSERT(IsValidColRow(wX, wY));
	for (UINT wScrollI=this->Scrolls.size(); wScrollI--; )
	{
		if (wX == this->Scrolls[wScrollI]->wX && wY == this->Scrolls[wScrollI]->wY)
		{
			this->Scrolls[wScrollI]->wX = wNewX;
			this->Scrolls[wScrollI]->wY = wNewY;
			return;
		}
	}
}

//*****************************************************************************
UINT CDbRoom::GetOSquare(
//Get tile# for a square on the opaque layer.
//
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return (UINT) (unsigned char) (this->pszOSquares[ARRAYINDEX(wX,wY)]);
}

//*****************************************************************************
UINT CDbRoom::GetFSquare(const UINT wX, const UINT wY) const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return (UINT) (unsigned char) (this->pszFSquares[ARRAYINDEX(wX,wY)]);
}

//*****************************************************************************
UINT CDbRoom::GetTSquare(
//Get object# for a tile on the transparent layer.
//
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return GetTSquare(ARRAYINDEX(wX, wY));
}

UINT CDbRoom::GetTSquare(const UINT index) const
{
	RoomObject *tObj = this->tLayer[index];
	if (!tObj)
		return RoomObject::emptyTile();

	return tObj->tile;
}

//*****************************************************************************
UINT CDbRoom::GetBottomTSquare(
//Get a transparent layer tile,
//preferring the bottom element if one hides another.
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	RoomObject *tObj = this->tLayer[ARRAYINDEX(wX,wY)];
	if (!tObj)
		return RoomObject::emptyTile();

	return tObj->getBottomTile();
}

//*****************************************************************************
UINT CDbRoom::GetCoveredTSquare(
//Get any covered item under the transparent layer tile.
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return GetCoveredTSquare(ARRAYINDEX(wX,wY));
}

UINT CDbRoom::GetCoveredTSquare(const UINT index) const
{
	RoomObject *tObj = this->tLayer[index];
	if (!tObj)
		return RoomObject::emptyTile();

	return tObj->coveredTile;
}

//*****************************************************************************
bool CDbRoom::IsAnyTLayerObject(const UINT wX, const UINT wY, const UINT tile) const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	RoomObject *tObj = this->tLayer[ARRAYINDEX(wX,wY)];
	if (!tObj) {
		return tile == RoomObject::emptyTile();
	}

	if (tObj->tile == tile)
		return true;
	if (tObj->coveredTile == tile)
		return true;

	return false;
}

//*****************************************************************************
UINT CDbRoom::GetTParam(
//Get parameter for a square on the transparent layer.
//
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return GetTParam(ARRAYINDEX(wX,wY));
}

UINT CDbRoom::GetTParam(const UINT index) const
{
	RoomObject *tObj = this->tLayer[index];
	if (!tObj)
		return RoomObject::noParam();

	return tObj->param;
}

//*****************************************************************************
void CDbRoom::RemoveCoveredTLayerItem(const UINT wSX, const UINT wSY)
{
	RoomObject *tObj = this->tLayer[ARRAYINDEX(wSX,wSY)];
	if (tObj)
		tObj->remove_covered();
}

//*****************************************************************************
UINT CDbRoom::GetOSquareWithGuessing(
//Get tile# for a square on the opaque layer.  If col/row is out-of-bounds then
//a "guess" will be made--the tile of whichever square is closest to the OOB square
//will be used.
	 const int nX, const int nY) const
{
	 int nUseX = nX, nUseY = nY;
	 if (nUseX < 0)
		  nUseX = 0;
	 else if ((UINT)nUseX >= this->wRoomCols)
		  nUseX = this->wRoomCols - 1;
	 if (nUseY < 0)
		  nUseY = 0;
	 else if ((UINT)nUseY >= this->wRoomRows)
		  nUseY = this->wRoomRows - 1;
	 return (UINT) (unsigned char) (this->pszOSquares[ARRAYINDEX(nUseX,nUseY)]);
}

//*****************************************************************************
UINT CDbRoom::GetTSquareWithGuessing(
//Get tile# for a square on the opaque layer.  If col/row is out-of-bounds then
//a "guess" will be made--the tile of whichever square is closest to the OOB square
//will be used.
	 const int nX, const int nY) const
{
	 int nUseX = nX, nUseY = nY;
	 if (nUseX < 0)
		  nUseX = 0;
	 else if ((UINT)nUseX >= this->wRoomCols)
		  nUseX = this->wRoomCols - 1;
	 if (nUseY < 0)
		  nUseY = 0;
	 else if ((UINT)nUseY >= this->wRoomRows)
		  nUseY = this->wRoomRows - 1;

	 return GetTSquare(nUseX,nUseY);
}

//*****************************************************************************
CEntity* CDbRoom::GetSpeaker(const UINT wType, const bool bConsiderBaseType) //[default=false]
//Returns: pointer to character/monster of specified type
{
	if (wType == M_PLAYER)
		return &(this->pCurrentGame->swordsman);
	if ((bConsiderBaseType && this->pCurrentGame->swordsman.wAppearance == wType) ||
			(!bConsiderBaseType && this->pCurrentGame->swordsman.wIdentity == wType))
		return &(this->pCurrentGame->swordsman);

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType != M_CHARACTER)
		{
			if (wType == pMonster->GetIdentity())
				return pMonster;
		} else {
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->bVisible && wType == pCharacter->wLogicalIdentity)
				return pMonster;
		}
		pMonster = pMonster->pNext;
	}

	return NULL;
}

//*****************************************************************************
void CDbRoom::SetCurrentGame(
//Sets the current game pointer for anything associated with this room.
//
//Params:
	CCurrentGame *const pSetCurrentGame)
{
	ASSERT(pSetCurrentGame);
	this->pCurrentGame = pSetCurrentGame;
	SetCurrentGameForMonsters(pSetCurrentGame);
	for (UINT wIndex=this->platforms.size(); wIndex--; )
		this->platforms[wIndex]->SetCurrentGame(pSetCurrentGame);

	if (this->bCheckForHoldCompletion)
	{
		if (this->pCurrentGame && !this->pCurrentGame->bHoldCompleted)
		{
			const UINT holdID = this->pCurrentGame->pHold ?
					this->pCurrentGame->pHold->dwHoldID :
					g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
			this->pCurrentGame->bHoldCompleted = g_pTheDB->Holds.IsHoldCompleted(
					holdID, g_pTheDB->GetPlayerID());
		}
		this->bCheckForHoldCompletion = false;
	}

	if (this->bCheckForHoldMastery)
	{
		if (this->pCurrentGame && !this->pCurrentGame->bHoldMastered)
		{
			const UINT holdID = this->pCurrentGame->pHold ?
					this->pCurrentGame->pHold->dwHoldID :
					g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
			this->pCurrentGame->bHoldMastered = g_pTheDB->Holds.IsHoldMastered(
					holdID, g_pTheDB->GetPlayerID());
		}
		this->bCheckForHoldMastery = false;
	}
}

//*****************************************************************************
UINT CDbRoom::GetExitIndexAt(
//Used to differentiate between different room exits
//
//Returns: # of exit at (wX,wY), or -1 if none
//
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	for (UINT i=0; i<this->Exits.size(); ++i)
	{
		const CExitData &stairs = *(this->Exits[i]);
		if (IsInRect(wX, wY,
				stairs.wLeft, stairs.wTop, stairs.wRight, stairs.wBottom))
		{
			return i;
		}
	}

	return static_cast<UINT>(-1);
}

//*****************************************************************************
bool CDbRoom::GetExitEntranceIDAt(
//Returns: true if a level exit at (wX,wY) was found
//
//Params:
	const UINT wX, const UINT wY, //(in)
	UINT &dwEntranceID) //(out) entranceID for the exit at (wX,wY), else 0L if none.
const
{
	for (UINT i=0; i<this->Exits.size(); ++i)
	{
		const CExitData &stairs = *(this->Exits[i]);
		if (IsInRect(wX, wY,
				stairs.wLeft, stairs.wTop, stairs.wRight, stairs.wBottom))
		{
			dwEntranceID = stairs.dwEntranceID;
			return true;
		}
	}

	dwEntranceID = 0L;
	return false;
}

//*****************************************************************************
void CDbRoom::FindOrbsToOpenDoor(CCoordSet& orbs, const CCoordSet& doorSquares) const
//Returns: a set of orb and pressure plate coords that will open any of the indicated door tiles
{
	UINT wOrbIndex, wAgentIndex;
	for (wOrbIndex=this->orbs.size(); wOrbIndex--; )
	{
		COrbData& orb = *(this->orbs[wOrbIndex]);
		if (GetTSquare(orb.wX, orb.wY) != T_ORB)
			continue; //no orb is at this location

		if (orb.eType == OT_BROKEN)
			continue; //orb doesn't work

		for (wAgentIndex=orb.agents.size(); wAgentIndex--; )
		{
			COrbAgentData& agent = *(orb.agents[wAgentIndex]);
			if (agent.action != OA_CLOSE)
			{
				//This agent opens some door.
				//Check each door square for this orb agent acting on it.
				if (doorSquares.has(agent.wX, agent.wY))
				{
					//This agent opens the door.  Remember this orb.
					orbs.insert(orb.wX, orb.wY); //orb
					break;   //continue to next orb
				}
			}
		}
	}
}

//*****************************************************************************
void CDbRoom::FindPlatesToOpenDoor(CCoordSet& plateTiles, const CCoordSet& doorSquares) const
//Returns: a set of orb and pressure plate coords that will open any of the indicated door tiles
{
	UINT wOrbIndex, wAgentIndex;
	for (wOrbIndex=this->orbs.size(); wOrbIndex--; )
	{
		COrbData& plate = *(this->orbs[wOrbIndex]);
		if (GetOSquare(plate.wX, plate.wY) != T_PRESSPLATE)
			continue; //no plate is at this location

		if (plate.eType == OT_BROKEN)
			continue; //plate doesn't work

		for (wAgentIndex=plate.agents.size(); wAgentIndex--; )
		{
			COrbAgentData& agent = *(plate.agents[wAgentIndex]);
			if (agent.action != OA_CLOSE)
			{
				//This agent opens some door.
				//Check each door square for this agent acting on it.
				if (doorSquares.has(agent.wX, agent.wY))
				{
					//This agent opens the door.
					//Get all tiles for this pressure plate.
					const UINT wPlateIndex = wOrbIndex + 1;
					ASSERT(wPlateIndex == this->pressurePlateIndex.GetAt(plate.wX, plate.wY));
					this->pressurePlateIndex.GetCoordsWithValue(wPlateIndex, &plateTiles);
					break;   //continue to next plate
				}
			}
		}
	}
}

//*****************************************************************************
void CDbRoom::SetExit(
//Sets the exit at (wX,wY) to dwEntranceID.
//ASSUME: there's only one exit at that coordinate.
//
//Params:
	const UINT dwEntranceID,
	const UINT wX, const UINT wY, const UINT wX2, const UINT wY2)
{
	ASSERT(bIsStairs(GetOSquare(wX,wY)));

	UINT i;
	for (i=0; i<this->Exits.size(); ++i)
	{
		CExitData& stairs = *(this->Exits[i]);
		if (IsInRect(wX, wY,
				stairs.wLeft, stairs.wTop, stairs.wRight, stairs.wBottom))
		{
			//Modify existing exit's value.
			stairs.dwEntranceID = dwEntranceID;

			//Ensure boundaries include those passed in.
			if (wX < stairs.wLeft)
				stairs.wLeft = wX;
			if (wX2 > stairs.wRight)
				stairs.wRight = wX2;
			if (wY < stairs.wTop)
				stairs.wTop = wY;
			if (wY2 > stairs.wBottom)
				stairs.wBottom = wY2;

			return;
		}
	}

	//Add new exit for the specified rectangular room region.
	ASSERT(wX2 < this->wRoomCols);
	ASSERT(wY2 < this->wRoomRows);

	CExitData *pNewExit = new CExitData(dwEntranceID, wX, wX2, wY, wY2);
	this->Exits.push_back(pNewExit);
}

void CDbRoom::GetPositionInLevel(int& dx, int& dy) const
{
	UINT dwRoomX, dwRoomY; //level starting room coords
	CDbLevels::GetStartingRoomCoords(this->dwLevelID, dwRoomX, dwRoomY);

	dx = this->dwRoomX - dwRoomX;   //offset from starting room
	dy = this->dwRoomY - dwRoomY;
}

//*****************************************************************************
void CDbRoom::GetLevelPositionDescription(
//Gets text description of this room's position within level.
//
//Params:
	WSTRING &wstrDescription,  //(in/out)  Accepts empty or non-empty value.
								//       Returns with description appended.
	const bool bAbbreviate)    //(in) whether to return an abbreviated
										//desc. text (default=false)
{
	int dX, dY;
	GetPositionInLevel(dX, dY);

	//Call language-specific version of method.
	switch (Language::GetLanguage())
	{
		case Language::English:
		case Language::French:
			GetLevelPositionDescription_English(wstrDescription, dX, dY, bAbbreviate);
		break;
		case Language::Russian:
			GetLevelPositionDescription_Russian(wstrDescription, dX, dY, bAbbreviate);
		break;
		default:
			//Not supported -- just use English grammar.
			GetLevelPositionDescription_English(wstrDescription, dX, dY, bAbbreviate);
		break;
	}
}

//*****************************************************************************
CPlatform* CDbRoom::GetPlatformAt(
//Returns: platform with a piece at (wX,wY), or NULL if none.
//
//Params:
	const UINT wX, const UINT wY)
const
{
	for (UINT wIndex=this->platforms.size(); wIndex--; )
		if (this->platforms[wIndex]->IsAt(wX,wY))
			return this->platforms[wIndex];
	return NULL;
}

//*****************************************************************************
void CDbRoom::getStats(RoomStats& stats, const CDbLevel *pLevel) const
//Tallies the value of stat-affecting items in the room.
//
//This ignores any script commands that may modify monster or player stats.
{
	CCoordSet ignoreOTiles, tiles;

	++stats.rooms;
	if (this->bIsSecret)
		++stats.secrets;

	ASSERT(pLevel);
	UINT wX, wY;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			const UINT oTile = GetOSquare(wX,wY);

/*
			CMonster *pMonster = GetMonsterAtSquare(wX,wY);

			if (pMonster)
			{
				++stats.monsters;
			}
*/

			switch (oTile)
			{
				case T_DOOR_Y:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.yellowDoors;
					}
				break;
				case T_DOOR_YO:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openYellowDoors;
					}
				break;
				case T_DOOR_M:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.greenDoors;
					}
				break;
				case T_DOOR_GO:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openGreenDoors;
					}
				break;
				case T_DOOR_R:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.redDoors;
					}
				break;
				case T_DOOR_RO:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openRedDoors;
					}
				break;
				case T_DOOR_C:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.blueDoors;
					}
				break;
				case T_DOOR_CO:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openBlueDoors;
					}
				break;
				case T_DOOR_B:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.blackDoors;
					}
				break;
				case T_DOOR_BO:
					GetConnected4NeighborTiles(wX,wY,CTileMask(oTile),
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openBlackDoors;
					}
				break;
			}
		}
}

//
//CDbRoom private methods.
//

//*****************************************************************************
UINT CDbRoom::GetLocalID() const
//Compares this object's GID fields against those of the records in the DB.
//ASSUME: dwLevelID has already been set to the local record ID
//
//Returns: local ID if a record in the DB matches this object's GUID, else 0
{
	ASSERT(IsOpen());

	CIDSet roomsInLevel = CDb::getRoomsInLevel(this->dwLevelID);
	c4_View RoomsView;

	//Each iteration checks a room's GID.
	for (CIDSet::const_iterator room = roomsInLevel.begin();
			room != roomsInLevel.end(); ++room)
	{
		const UINT roomRowI = LookupRowByPrimaryKey(*room, V_Rooms, RoomsView);
		c4_RowRef row = RoomsView[roomRowI];
		ASSERT((UINT)p_LevelID(row) == this->dwLevelID);

		//Check room coords.
		const UINT dwRoomX = (UINT)p_RoomX(row);
		const UINT dwRoomY = (UINT)p_RoomY(row);
		if (this->dwRoomX == dwRoomX && this->dwRoomY == dwRoomY)
		{
			//GUIDs match.  Return this record's local ID.
			return (UINT) p_RoomID(row);
		}
	}

	//No match.
	return 0L;
}

//*****************************************************************************
void CDbRoom::GetNumber_English(
//Writes a number in English.  Should only be called by
//GetLevelPositionDescription_English().
//
//Params:
	const UINT num,  //(in)   decimal number
	WCHAR *str)       //(out)  English description of number
{
	static const MESSAGE_ID numberText[19] = {MID_Once, MID_Twice, MID_Thrice,
			MID_Quarce, MID_Quince, MID_Sence, MID_Septence, MID_Octence,
			MID_Novence, MID_Tonce, MID_Elevonce, MID_Twolce, MID_Thorce,
			MID_Quartonce, MID_Quintonce, MID_Sextonce, MID_Septonce,
			MID_Octonce, MID_Noventonce};

	ASSERT(num);

	if (num < 20)
		WCScpy(str, GetMessageText(numberText[num-1]));
	else
	{
		_itoW(num, str, 10);
		WCScat(str, wszSpace);
		WCScat(str, GetMessageText(MID_Times));
	}
}

//*****************************************************************************
void CDbRoom::GetLevelPositionDescription_English(
//Gets English text description of player's room position within level.
//Should only be called by GetLevelPositionDescription().
//
//Params:
	WSTRING &wstrDescription,  //(in/out)  Accepts empty or non-empty value.
										//       Returns with description appended.
	const int dX, const int dY,//(in) Offset from entrance of level.
	const bool bAbbreviate)    //(in) whether to return an abbreviated
										//desc. text (default=false)
{
	if (dX == 0 && dY == 0)
	{
		wstrDescription += GetMessageText(MID_TheEntrance);
	} else {
		WCHAR temp[40]; //Hold "once", "twice", "25 times" etc.
		if (dY)
		{
			if (bAbbreviate)
			{
				_itoW(abs(dY), temp, 10);
				wstrDescription += temp;
				if (dY > 0) WCScpy(temp, GetMessageText(MID_South));
					else WCScpy(temp, GetMessageText(MID_North));
#ifdef RUSSIAN_BUILD
				ASSERT(WCSlen(temp) >= 4);
				wstrDescription += temp[3]; //e.g. "HA CEBEP" --> "C"
#else
				wstrDescription += temp[0];
#endif
			} else {
				static const WCHAR wszComma[] = { We(','),We(' '),We(0) };

				GetNumber_English(abs(dY),temp);
				wstrDescription += temp;
				wstrDescription += wszSpace;
				if (dY > 0) wstrDescription += GetMessageText(MID_South);
					else wstrDescription += GetMessageText(MID_North);
				if (dX) wstrDescription += wszComma;
			}
		}
		if (dX)
		{
			if (bAbbreviate)
			{
				_itoW(abs(dX), temp, 10);
				wstrDescription += temp;
				if (dX > 0) WCScpy(temp, GetMessageText(MID_East));
					else WCScpy(temp, GetMessageText(MID_West));
#ifdef RUSSIAN_BUILD
				ASSERT(WCSlen(temp) >= 4);
				wstrDescription += temp[3]; //e.g. "HA CEBEP" --> "C"
#else
				wstrDescription += temp[0];
#endif
			} else {
				GetNumber_English(abs(dX),temp);
				wstrDescription += temp;
				wstrDescription += wszSpace;
				if (dX > 0) wstrDescription += GetMessageText(MID_East);
					else wstrDescription += GetMessageText(MID_West);
			}
		}
	}
}

//*****************************************************************************
void CDbRoom::GetLevelPositionDescription_Russian(
//Gets English text description of player's room position within level.
//Should only be called by GetLevelPositionDescription().
//
//Params:
	WSTRING &wstrDescription,  //(in/out)  Accepts empty or non-empty value.
										//       Returns with description appended.
	const int dX, const int dY,//(in) Offset from entrance of level.
	const bool bAbbreviate)    //(in) whether to return an abbreviated
										//desc. text (default=false)
{
	if (dX == 0 && dY == 0)
	{
		wstrDescription += GetMessageText(MID_TheEntrance);
	} else {
		WCHAR temp[40]; //Hold "once", "twice", "25 times" etc.
		if (dY)
		{
			if (bAbbreviate)
			{
				_itoW(abs(dY), temp, 10);
				wstrDescription += temp;
				if (dY > 0) WCScpy(temp, GetMessageText(MID_South));
					else WCScpy(temp, GetMessageText(MID_North));
				ASSERT(WCSlen(temp) >= 4);
				wstrDescription += temp[3]; //e.g. "HA CEBEP" --> "C"
			} else {
				static const WCHAR wszComma[] = { We(','),We(' '),We(0) };

				GetNumber_English(abs(dY),temp);
				wstrDescription += temp;
				wstrDescription += wszSpace;
				if (dY > 0) wstrDescription += GetMessageText(MID_South);
					else wstrDescription += GetMessageText(MID_North);
				if (dX) wstrDescription += wszComma;
			}
		}
		if (dX)
		{
			if (bAbbreviate)
			{
				_itoW(abs(dX), temp, 10);
				wstrDescription += temp;
				if (dX > 0) WCScpy(temp, GetMessageText(MID_East));
					else WCScpy(temp, GetMessageText(MID_West));
				ASSERT(WCSlen(temp) >= 4);
				wstrDescription += temp[3]; //e.g. "HA CEBEP" --> "C"
			} else {
				GetNumber_English(abs(dX),temp);
				wstrDescription += temp;
				wstrDescription += wszSpace;
				if (dX > 0) wstrDescription += GetMessageText(MID_East);
					else wstrDescription += GetMessageText(MID_West);
			}
		}
	}
}

//*****************************************************************************
void CDbRoom::DeletePathMaps()
//Deletes all existing PathMaps.
{
	for (int n=0; n<NumMovementTypes; ++n)
	{
		delete pPathMap[n];
		pPathMap[n] = NULL;
	}
}

//*****************************************************************************
UINT CDbRoom::GetSquarePathMapObstacles(
//Does a square contain an obstacle for the pathmap.  The CPathMap class can
//use different obstacle rules; this routine just defines the obstacle rules
//for the CDbRoom's pathmap member.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Square to evaluate.
	const MovementType eMovement) //(in)  Type of movement ability to consider
//
//Returns:
//Non-zero if a square blocks movement in at least one direction.  In this case,
//the returned value will be a bitmask of blocked directions to/from the square
//if the movement type supports partial obstacles, otherwise (or if all directions
//are blocked) it'll be DMASK_ALL.
const
{
	//O-square obstacle?
	const UINT wOSquare = GetOSquare(wX, wY);
	
	//Active firetraps are always pathmap obstacles
	if (wOSquare == T_FIRETRAP_ON)
		return DMASK_ALL;

	switch (eMovement)
	{
		case GROUND:
		case GROUND_FORCE:
			if (!(bIsFloor(wOSquare) || bIsOpenDoor(wOSquare) || bIsPlatform(wOSquare)))
				return DMASK_ALL;
			break;
		case AIR:
			if (!(bIsFloor(wOSquare) || bIsOpenDoor(wOSquare) || bIsPlatform(wOSquare)))
				switch (wOSquare)
				{
					case T_PIT: case T_PIT_IMAGE:
					case T_WATER: case T_SHALLOW_WATER:
						break;  //can fly over these
					default:
						return DMASK_ALL;   //all others are automatically obstacles
				}
			break;
		case WALL:
			if (!(bIsWall(wOSquare) || bIsCrumblyWall(wOSquare) || bIsDoor(wOSquare)))
				return DMASK_ALL;
			break;
		case WATER:
			if (!bIsWater(wOSquare))
				return DMASK_ALL;
			break;
		case GROUND_AND_SHALLOW_WATER:
		case GROUND_AND_SHALLOW_WATER_FORCE:
			if (!(bIsFloor(wOSquare) || bIsOpenDoor(wOSquare) || bIsPlatform(wOSquare) ||
					bIsShallowWater(wOSquare)))
				return DMASK_ALL;
			break;
		default: ASSERT(!"Unsupported pathmap movement type"); break;
	}

	//Serpent body tiles (backwards compatibility) and inactive monsters are
	//considered pathmap obstacles.
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster) {
		if (!pMonster->IsAlive())
			return DMASK_ALL;
		if (bIsSerpentOrGentryii(pMonster->wType) && pMonster->IsPiece())
			return DMASK_ALL;
		if (pMonster->IsBrainPathmapObstacle())
			return DMASK_ALL;
	}

	UINT wBlockDir = DMASK_NONE;
	if (DoesGentryiiPreventDiagonal(wX,wY,wX-1,wY-1))
		wBlockDir |= DMASK_NW;
	if (DoesGentryiiPreventDiagonal(wX,wY,wX-1,wY+1))
		wBlockDir |= DMASK_SW;
	if (DoesGentryiiPreventDiagonal(wX,wY,wX+1,wY-1))
		wBlockDir |= DMASK_NE;
	if (DoesGentryiiPreventDiagonal(wX,wY,wX+1,wY+1))
		wBlockDir |= DMASK_SE;

	if (bMovementSupportsPartialObstacles(eMovement))
	{
		//T-square obstacle?
		const UINT wTSquare = GetTSquare(wX, wY);
		if (!(wTSquare == T_EMPTY || wTSquare == T_FUSE || wTSquare == T_TOKEN || bIsPotion(wTSquare) || wTSquare == T_SCROLL))
			return DMASK_ALL;

		//F-square obstacle?
		switch (GetFSquare(wX, wY))
		{
			case T_NODIAGONAL: return DMASK_NW | DMASK_NE | DMASK_SW | DMASK_SE;

			case T_ARROW_N:  return wBlockDir | DMASK_SE | DMASK_S | DMASK_SW;
			case T_ARROW_NE: return wBlockDir | DMASK_S | DMASK_SW | DMASK_W;
			case T_ARROW_E:  return wBlockDir | DMASK_SW | DMASK_W | DMASK_NW;
			case T_ARROW_SE: return wBlockDir | DMASK_W | DMASK_NW | DMASK_N;
			case T_ARROW_S:  return wBlockDir | DMASK_NW | DMASK_N | DMASK_NE;
			case T_ARROW_SW: return wBlockDir | DMASK_N | DMASK_NE | DMASK_E;
			case T_ARROW_W:  return wBlockDir | DMASK_NE | DMASK_E | DMASK_SE;
			case T_ARROW_NW: return wBlockDir | DMASK_E | DMASK_SE | DMASK_S;

			default: break;
		}
	}
	else
	{
		//T-square obstacle?
		const UINT wTSquare = GetTSquare(wX, wY);
		if (!(wTSquare == T_EMPTY || wTSquare == T_FUSE || wTSquare == T_TOKEN))
			return DMASK_ALL;

		//F-square obstacle?
		if (bIsArrow(GetFSquare(wX, wY)))
			return DMASK_ALL;
	}

	return wBlockDir;
}

//*****************************************************************************
void CDbRoom::OpenYellowDoor(
//Opens a yellow door.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coords of any square of the door to open.
{
	if (GetOSquare(wX, wY)==T_DOOR_Y)
		FloodPlot(wX, wY, T_DOOR_YO, false);
}

//*****************************************************************************
void CDbRoom::CloseYellowDoor(
//Closes a yellow door.
//
//Params:
	const UINT wX, const UINT wY, //(in) Coords of any square of the door to close.
	CCueEvents &CueEvents)
{
	if (GetOSquare(wX, wY)!=T_DOOR_YO) return;

	//Gather list of door squares.
	CTileMask doorMask(T_DOOR_YO);
	CCoordSet squares;
	GetConnected4NeighborTiles(wX, wY, doorMask, squares);

	//Plot these squares.
	UINT wSX, wSY;
	for (CCoordSet::const_iterator iter=squares.begin(); iter!=squares.end(); ++iter)
	{
		wSX = iter->wX; wSY = iter->wY;
		Plot(wSX, wSY, T_DOOR_Y);
		//Closing a door will cut fuses.
		switch (GetTSquare(wSX, wSY))
		{
		case T_FUSE:
			Plot(wSX, wSY, T_EMPTY);
			RemoveFuse(wSX, wSY);
			break;
		case T_MIRROR:
		case T_POWDER_KEG:
			if (GetCoveredTSquare(wSX,wSY) == T_FUSE)
			{
				RemoveCoveredTLayerItem(wSX,wSY);
				RemoveFuse(wSX, wSY);
			}
			break;
		default: break;
		}

		//Closing a door destroys Fluff
		DestroyFluff(wSX,wSY,CueEvents);
	}
	ConvertUnstableTar(CueEvents);
}

//*****************************************************************************
bool CDbRoom::ToggleGreenDoors(CCueEvents &CueEvents)
//Returns: whether any green doors were toggled
{
	ASSERT(!this->bGreenDoorsOpened);
	this->bGreenDoorsOpened = true;
	return ToggleTiles(T_DOOR_M, T_DOOR_GO, CueEvents); //green door
}

//*****************************************************************************
void CDbRoom::ToggleYellowDoor(
//Toggles a yellow door either open or shut.
//
//Params:
	const UINT wX, const UINT wY,  //(in) Coords of any square of the door to toggle.
	CCueEvents &CueEvents)
{
	if (GetOSquare(wX, wY) == T_DOOR_Y)
		OpenYellowDoor(wX, wY);
	else
		CloseYellowDoor(wX, wY, CueEvents);
}

//*****************************************************************************
void CDbRoom::ActivateFiretrap(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	CueEvents.Add(CID_Firetrap, new CCoord(wX, wY), true);

	const WeaponStab stab(wX, wY, NO_ORIENTATION, WT_Firetrap);
	this->pCurrentGame->StabRoomTile(stab, CueEvents);
}

void CDbRoom::DisableFiretrap(const UINT wX, const UINT wY)
{
	const UINT oTile = GetOSquare(wX,wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP_ON)
		Plot(wX,wY,T_FIRETRAP);
}

void CDbRoom::EnableFiretrap(const UINT wX, const UINT wY, CCueEvents &CueEvents)
{
	const UINT oTile = GetOSquare(wX,wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP) {
		Plot(wX,wY,T_FIRETRAP_ON);
		CueEvents.Add(CID_FiretrapActivated, new CCoord(wX, wY), true);
	}
}

void CDbRoom::ToggleFiretrap(const UINT wX, const UINT wY, CCueEvents &CueEvents)
{
	const UINT oTile = GetOSquare(wX,wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP)
		CueEvents.Add(CID_FiretrapActivated, new CCoord(wX, wY), true);
	Plot(wX,wY,getToggledFiretrap(oTile));
}

//*****************************************************************************
void CDbRoom::DisableForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX,wY);
	ASSERT(bIsAnyArrow(fTile));
	if (bIsArrow(fTile))
		Plot(wX,wY,getToggledForceArrow(fTile));
}

void CDbRoom::EnableForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX,wY);
	ASSERT(bIsAnyArrow(fTile));
	if (bIsDisabledArrow(fTile))
		Plot(wX,wY,getToggledForceArrow(fTile));
}

void CDbRoom::ToggleForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX,wY);
	ASSERT(bIsAnyArrow(fTile));
	Plot(wX,wY,getToggledForceArrow(fTile));
}

//*****************************************************************************
void CDbRoom::ToggleLight(
//Toggles a light on or off.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coords of a light.
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	SetTParam(wX,wY,(tParam + LIGHT_OFF) % 256);
	this->disabledLights.insert(wX,wY);
	SetRoomLightingChanged();
}

void CDbRoom::TurnOffLight(const UINT wX, const UINT wY)
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	if (tParam < LIGHT_OFF)
	{
		SetTParam(wX,wY,tParam + LIGHT_OFF);
		SetRoomLightingChanged();
	}
}

void CDbRoom::TurnOnLight(const UINT wX, const UINT wY)
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	if (tParam >= LIGHT_OFF)
	{
		SetTParam(wX,wY,tParam - LIGHT_OFF);
		SetRoomLightingChanged();
	}
}

//*****************************************************************************
void CDbRoom::Clear()
//Frees resources associated with this object and resets member vars.
{
	UINT wIndex;

	this->bPartialLoad = false;

	ClearPlotHistory();

	this->dwRoomID = this->dwLevelID =
		this->dwDataID = this->dwOverheadDataID =
		this->dwRoomX = this->dwRoomY = 0;
	this->wRoomCols = this->wRoomRows =
		this->wImageStartX = this->wImageStartY =
		this->wOverheadImageStartX = this->wOverheadImageStartY =
		this->wTrapDoorsLeft = this->wTarLeft = 0;
	this->bTarWasBuilt = false;
	this->bTarstuffGateTogglePending = false;
	this->bBetterVision = false;
	this->bPersistentCitizenMovement = this->bHasConquerToken = this->bHasActiveBeacon = false;
	this->bIsRequired = false;
	this->bIsSecret = false;
	this->style.resize(0);

	delete[] this->pszOSquares;
	this->pszOSquares = NULL;

	delete[] this->pszFSquares;
	this->pszFSquares = NULL;

	ClearTLayer();
	delete[] this->tLayer;
	this->tLayer = NULL;

	this->overheadTiles.Clear();

	delete[] this->pMonsterSquares;
	this->pMonsterSquares = NULL;

	for (wIndex=this->orbs.size(); wIndex--; )
		delete this->orbs[wIndex];
	this->orbs.clear();

	for (wIndex=this->Scrolls.size(); wIndex--; )
		delete this->Scrolls[wIndex];
	this->Scrolls.clear();

	for (wIndex=this->Exits.size(); wIndex--; )
		delete this->Exits[wIndex];
	this->Exits.clear();

	this->tileLights.Clear();

	this->LitFuses.clear();
	this->NewFuses.clear();
	this->NewBabies.Clear();
	this->briars.clear();
	for (wIndex=this->stations.size(); wIndex--; )
		delete this->stations[wIndex];
	this->stations.clear();

	this->halphEnters.clear();
	this->halph2Enters.clear();
	this->slayerEnters.clear();
	this->slayer2Enters.clear();
	this->checkpoints.clear();
	this->Beacons.clear();

	this->ExtraVars.Clear();
	this->weather.clear();
	this->bridges.clear();
	this->building.clear();
	this->floorSpikes.clear();
	this->fluffVents.clear();
	this->activeFiretraps.clear();

	this->bCheckForHoldCompletion = this->bCheckForHoldMastery = false;
	this->bTarWasStabbed = this->bGreenDoorsOpened = false;
	this->pLastClone = NULL;

	DeletePathMaps();

	//this->wMonsterCount = this->wBrainCount = 0;
	ClearMonsters();
	ClearDeadMonsters();
	ClearDeadRoomObjects();
	ClearPlatforms();

	this->deletedScrollIDs.clear();
	this->deletedSpeechIDs.clear();
	this->deletedDataIDs.clear();

	ClearStateVarsUsedDuringTurn();

	//These should have been cleared by the calls above.
	ASSERT(this->Decoys.empty());
	ASSERT(this->monsterEnemies.empty());
}

//*****************************************************************************
void CDbRoom::ClearPushStates()
{
	this->pushed_objects.clear();
	this->pushed_monsters.clear();
	this->pushed_player = false;
}

//*****************************************************************************
void CDbRoom::ClearStateVarsUsedDuringTurn()
{
	this->stationary_powder_kegs.clear();
	this->monsters_stabbed_by_spikes_this_turn.clear();
	this->stabbed_powder_kegs.Clear();

	this->room_lighting_changed = false;

	ClearPushStates();
}

//*****************************************************************************
bool CDbRoom::UnpackTileLights(
//Unpacks tile lights from database (version 3.0) into a format that the game will use.
//This routine will fail if the data is incompatible with the game engine.
//
//Params:
	const BYTE *pSrc, //(in) Buffer of tile data from database.
	const UINT dwSrcSize)  //(in) Size of buffer.
//
//Returns:
//True if successful, false if not.
{
	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);

	//Alloc tiles for this room.
	this->tileLights.Init(this->wRoomCols, this->wRoomRows);

	if (!pSrc || !dwSrcSize)
		return true; //an empty light data record is okay

	const BYTE *pRead = pSrc, *pStopReading = pRead + dwSrcSize;

	//1. Version number.
	const BYTE version = *(pRead++);
	if (version < 4)
		return false; //DROD 3.0+ supports light data format version 4+.

	//Run length decoding info.
	BYTE numTiles;
	UINT tileNo;

	//2. Read light tiles.
	const bool bShortValues = version >= 5;
	UINT wWriteIndex = 0;
	while (wWriteIndex < dwSquareCount)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		tileNo = *(pRead++);
		if (bShortValues) //unpack 2-byte value
		{
			tileNo *= 256;
			tileNo += *(pRead++);
		}
		if (!tileNo)
			wWriteIndex += numTiles;
		else
			while (numTiles--)
			{
				this->tileLights.Add(wWriteIndex % this->wRoomCols,
						wWriteIndex / this->wRoomCols, tileNo); //deduce (x,y) coord
				++wWriteIndex;
			}
	}
	//All tiles should have been read without excess.
	if (wWriteIndex != dwSquareCount)
		return false;

   //Source buffer should contain data for exactly the number of squares in
	//the room.
	if (pRead != pStopReading)
		return false;

	return true;
}

//*****************************************************************************
bool CDbRoom::UnpackSquares(
//Unpacks squares from database (version 1.6 - 5.0) into a format that the game will use.
//This routine will fail if the data is incompatible with the game engine.
//
//Params:
	const BYTE *pSrc, //(in) Buffer of tile data from database.
	const UINT dwSrcSize)  //(in) Size of buffer.
//
//Returns:
//True if successful, false if not.
{
	if (!pSrc || !dwSrcSize) return false;

	if (!AllocTileLayers())
		return false;

	const UINT dwSquareCount = CalcRoomArea();

	memset(this->pMonsterSquares, 0, dwSquareCount * sizeof(CMonster*));
	memset(this->tLayer, 0, dwSquareCount * sizeof(RoomObject*));

	const BYTE *pRead = pSrc, *pStopReading = pRead + dwSrcSize;

	//1. Number of layers.
	const BYTE version = *(pRead++);
	if (version < 3)
	{
		//These layers were added in version 3 or after.
		memset(this->pszFSquares, T_EMPTY, dwSquareCount * sizeof(char));

		return UnpackSquares1_6(pSrc, dwSrcSize);
	}
	if (version > 6)
		return false; //DROD supports up to room data format version 5.

	BYTE numTiles;
	char tileNo;

	//2. Read o-layer.
	char *pWriteO = this->pszOSquares;  //shorthand
	const char *pStopOWriting = pWriteO + dwSquareCount;
	while (pWriteO < pStopOWriting)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		ASSERT(pWriteO + numTiles <= pStopOWriting);
		tileNo = *(pRead++);
		ASSERT(tileNo < TILE_COUNT); //all tile types should be recognized
		while (numTiles--)
			*(pWriteO++) = tileNo;
	}
	//All squares in layer should have been read without excess.
	if (pWriteO != pStopOWriting) return false;

	//3. Read f-layer (version 5+).
	if (version < 5) {
		memset(this->pszFSquares, T_EMPTY, dwSquareCount * sizeof(char));
	} else {
		char *pWriteF = this->pszFSquares; //shorthand
		const char *pStopFWriting = pWriteF + dwSquareCount;
		while (pWriteF < pStopFWriting)
		{
			//Don't read past end of buffer.
			if (pRead >= pStopReading) return false;

			numTiles = *(pRead++);
			ASSERT(numTiles); //having a run-length of zero is wasteful
			ASSERT(pWriteF + numTiles <= pStopFWriting);
			tileNo = *(pRead++);
			ASSERT(tileNo < TILE_COUNT); //all tile types should be recognized
			while (numTiles--)
				*(pWriteF++) = tileNo;
		}
		//All squares in layer should have been read without excess.
		if (pWriteF != pStopFWriting) return false;
	}

	//4 and 5. Read t-layer and tile parameters.
	RoomObject **pWriteT = this->tLayer;
	BYTE param=0;
	RoomObject **const pStopTWriting = pWriteT + dwSquareCount;
	while (pWriteT < pStopTWriting)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		ASSERT(pWriteT + numTiles <= pStopTWriting);
		tileNo = *(pRead++);
		ASSERT(tileNo < TILE_COUNT); //all tile types should be recognized
		if (version >= 4)
			param = *(pRead++);
		const bool needObject = ((UINT)tileNo != RoomObject::emptyTile()) || (param != RoomObject::noParam());
		if (needObject) {
			while (numTiles--)
			{
				const UINT index = pWriteT - this->tLayer;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				RoomObject *tObj = AddTLayerObject(wX, wY, tileNo);
				if (bIsTLayerCoveringItem(tileNo)) {
					tObj->coveredTile = param;
				} else {
					tObj->param = param;
				}
				*(pWriteT++) = tObj;
			}
		} else {
			pWriteT += numTiles;
		}
	}
	//All squares in layer should have been read without excess.
	if (pWriteT != pStopTWriting)
		return false;

	//6. Read overhead layer (version 6+).
	if (version >= 6)
	{
		BYTE val = *(pRead++);
		if (val != 0) //indicates there is some data
		{
			UINT index = 0;
			while (index < dwSquareCount)
			{
				//Don't read past end of buffer.
				if (pRead >= pStopReading)
					return false;

				numTiles = *(pRead++);
				ASSERT(numTiles); //having a run-length of zero is wasteful
				ASSERT(index + numTiles <= dwSquareCount);
				val = *(pRead++);
				ASSERT(val < 2); //only on/off
				while (numTiles--) {
					this->overheadTiles.SetAtIndex(index++, val);
				}
			}
			//All tiles in layer should have been read without excess.
			if (index != dwSquareCount)
				return false;
		}
	}

	//Monster records read in LoadMonsters().

	//Source buffer should contain data for exactly the number of squares in the room.
	if (pRead != pStopReading)
		return false;

	if (version < 5)
	{
		//Upgrade version 4 data to version 5:
		//Move the items listed below from the t-layer to the new f-layer.
		pWriteT = this->tLayer;
		for (UINT wIndex=0; wIndex<dwSquareCount; ++wIndex, ++pWriteT)
		{
			RoomObject *tObj = *pWriteT;
			const UINT tTile = tObj ? tObj->tile : RoomObject::emptyTile();
			switch (tTile)
			{
				case T_ARROW_N: case T_ARROW_S: case T_ARROW_W: case T_ARROW_E:
				case T_ARROW_NW: case T_ARROW_SW: case T_ARROW_NE: case T_ARROW_SE:
				case T_NODIAGONAL:
				{
					RemoveTLayerObject(tObj);
					*pWriteT = NULL;
					this->pszFSquares[wIndex] = tTile;
				}
				break;
			}
		}
	}

	return true;
}

//*****************************************************************************
bool CDbRoom::AllocTileLayers()
{
	if (!this->overheadTiles.Init(this->wRoomCols, this->wRoomRows))
		return false;

	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);

	ASSERT(!this->pszOSquares);
	this->pszOSquares = new char[dwSquareCount];
	if (!this->pszOSquares) return false;

	ASSERT(!this->pszFSquares);
	this->pszFSquares = new char[dwSquareCount];
	if (!this->pszFSquares) {delete[] this->pszOSquares; return false;}

	ASSERT(!this->tLayer);
	this->tLayer = new RoomObject*[dwSquareCount];
	if (!this->tLayer) {delete[] this->pszOSquares; delete[] this->pszFSquares; return false;}

	ASSERT(!this->pMonsterSquares);
	this->pMonsterSquares = new CMonster*[dwSquareCount];
	if (!this->pMonsterSquares) {delete[] this->pszOSquares; delete[] this->pszOSquares; delete[] this->tLayer; return false;}

	return true;
}

//*****************************************************************************
bool CDbRoom::UnpackSquares1_6(
//Unpacks squares from database (version 1.6 or earlier only)
//into a format that the game will use.  The data
//format supports a larger tileset and number of layers than the current DROD
//game engine.  This routine will fail if the data is incompatible with the
//game engine.
//
//Params:
	const BYTE *pSrc, //(in) Buffer of square data from database.
	const UINT dwSrcSize)  //(in) Size of buffer.
//
//Returns:
//True if successful, false if not.
{
	const UINT dwSquareCount = CalcRoomArea();

	const BYTE *pRead = pSrc, *pStopReading = pRead + dwSrcSize;
	char *pWriteO = pszOSquares;
	char *pStopOWriting = pWriteO + dwSquareCount;
	RoomObject **pWriteT = this->tLayer;

	USHORT wLayerCount, wOpaqueTileNo, wTransparentTileNo;
	while (pRead < pStopReading)
	{
		//Don't write past end of buffer.
		if (pWriteO >= pStopOWriting) return false;

		//Get number of layers stored in this square.
		wLayerCount = *(pRead++);
		if (pRead >= pStopReading) return false;  //There should be square data next.
		if (wLayerCount < 1 || wLayerCount > 2) return false; //DROD only supports 2 layers right now.

		//Read opaque square.
		memcpy( (void*)&wOpaqueTileNo, pRead, sizeof(USHORT));
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wOpaqueTileNo);
#endif
		if (wOpaqueTileNo > 255) return false; //DROD only supports 256 tiles right now.

		*(pWriteO++) = (char)wOpaqueTileNo;
		pRead += sizeof(USHORT);

		//Read transparent square.
		if (wLayerCount == 1)
		{
			wTransparentTileNo = RoomObject::emptyTile();
		} else {
			memcpy( (void*)&wTransparentTileNo, pRead, sizeof(USHORT));
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
			LittleToBig(&wTransparentTileNo);
#endif
			if (wTransparentTileNo > 255) return false; //DROD only supports 256 tiles right now.
			pRead += sizeof(USHORT);
		}

		const UINT index = pWriteT - this->tLayer;
		if (bIsArrow(wTransparentTileNo)) //f-layer conversion
		{
			pszFSquares[index] = (char)wTransparentTileNo;
		} else {
			if (wTransparentTileNo != RoomObject::emptyTile()) {
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				RoomObject *tObj = AddTLayerObject(wX, wY, UINT(wTransparentTileNo));
				*pWriteT = tObj;
			}
		}
		++pWriteT;
	}

	//Source buffer should contain data for exactly the number of tiles in the room.
	return pWriteO == pStopOWriting;
}

//*****************************************************************************
bool CDbRoom::LoadOrbs(
//Loads orbs from database into member vars of object.
//
//Params:
	c4_View &OrbsView)      //(in) Open view containing 0 or more orbs.
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = true;

	ASSERT(this->orbs.size() == 0);

	this->pressurePlateIndex.Init(this->wRoomCols, this->wRoomRows);

	const UINT wOrbCount = OrbsView.GetSize();
	if (!wOrbCount) return bSuccess;

	UINT wOrbI, wOrbAgentI;
	for (wOrbI=0; wOrbI < wOrbCount; ++wOrbI)
	{
		c4_RowRef row = OrbsView[wOrbI];
		COrbData *pOrb = AddOrbToSquare(p_X(row), p_Y(row));
		if (!pOrb) {bSuccess=false; goto Cleanup;}
		pOrb->eType = (OrbType)(UINT)p_Type(row);

		c4_View OrbAgentsView = p_OrbAgents(row);
		const UINT wNumAgents = OrbAgentsView.GetSize();
		for (wOrbAgentI=0; wOrbAgentI < wNumAgents; ++wOrbAgentI)
		{
			c4_RowRef agentRow = OrbAgentsView[wOrbAgentI];
			COrbAgentData *pAgent = new COrbAgentData(
					p_X(agentRow), p_Y(agentRow),
				  (OrbAgentType)(UINT)(p_Type(agentRow)));
			if (!pAgent) {bSuccess=false; goto Cleanup;}
			pOrb->agents.push_back(pAgent);
		}
	}

Cleanup:
	if (!bSuccess)
	{
		for (wOrbI=this->orbs.size(); wOrbI--; )
			delete this->orbs[wOrbI];
		this->orbs.clear();
	}
	return bSuccess;
}

//*****************************************************************************
CMonster* CDbRoom::LoadMonster(const c4_RowRef& row,
	CDbHold* &pHold) //optimization, for custom character insertion order
//Returns: pointer to monster object, or NULL if not needed
{
	//Note: To test a monster which is not in any of the available rooms,
	//it is useful to change the line below to replace the monster type
	//with the one you'd like to see.
	const UINT wMonsterType = p_Type(row);
	const UINT wX = p_X(row), wY = p_Y(row);

	//Halph/Slayer on room edge aren't put in to the room yet.
	if (DoesMonsterEnterRoomLater(wX, wY, wMonsterType))
	{
		CCoordSet& entrances = wMonsterType == M_HALPH ? this->halphEnters :
			wMonsterType == M_HALPH2 ? this->halph2Enters :
			wMonsterType == M_SLAYER ? this->slayerEnters : this->slayer2Enters;
		entrances.insert(wX, wY);
		return NULL;
	}

	CMonster *pNew = AddNewMonster((MONSTERTYPE)wMonsterType, wX, wY, true, false);
	if (!pNew)
		throw CException("CDbRoom::LoadMonster: Alloc failed");

	pNew->bIsFirstTurn = (p_IsFirstTurn(row) == 1);
	pNew->ExtraVars = p_ExtraVars(row);
	pNew->wO = pNew->wPrevO = p_O(row);
	pNew->SetMembersFromExtraVars();

	if (pNew->wType == M_CHARACTER){
		// For custom characters we need to initialize their processing sequence before linking
		// We need to set current game beforehand too, for the check to pass
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);

		if (!pHold) {
			const UINT dwHoldID = g_pTheDB->Levels.GetHoldIDForLevel(this->dwLevelID);
			if (dwHoldID)
				pHold = g_pTheDB->Holds.GetByID(dwHoldID);
		}

		pCharacter->ResolveLogicalIdentity(pHold);
	}

	pNew->ResetCurrentGame(); //need to add monster pieces below before setting current game

	c4_View PiecesView = p_Pieces(row);
	const UINT wNumPieces = PiecesView.GetSize();
	ASSERT(pNew->IsLongMonster() || wNumPieces == 0);
	for (UINT wPieceI=0; wPieceI < wNumPieces; ++wPieceI)
	{
		c4_RowRef pieceRow = PiecesView[wPieceI];
		const UINT wX = p_X(pieceRow);
		const UINT wY = p_Y(pieceRow);
		CMonsterPiece *pMPiece = new CMonsterPiece(pNew, p_Type(pieceRow), wX, wY);
		ASSERT(!this->pMonsterSquares[ARRAYINDEX(wX,wY)]);
		this->pMonsterSquares[ARRAYINDEX(wX,wY)] = pMPiece;
		pNew->Pieces.push_back(pMPiece);
	}

	if (bIsSerpent(pNew->wType))
	{
		//Link serpent pieces to the main monster object.
		CSerpent *pSerpent = DYN_CAST(CSerpent*, CMonster*, pNew);
		if (wNumPieces == 0) {
			pSerpent->FindTail(this);   //(1.6 serpent data compatibility)
		} else {
			UINT xIgnored, yIgnored;
			pSerpent->GetTail(xIgnored, yIgnored);
		}
	}

	if (this->pCurrentGame)
		pNew->SetCurrentGame(this->pCurrentGame);

	return pNew;
}

//*****************************************************************************
bool CDbRoom::LoadMonsters(
//Loads monsters from database into member vars of object.
//
//Params:
	c4_View &MonstersView)     //(in) Open view containing 0 or more monsters.
//
//Returns:
//True if successful, false if not.
{
	CDbHold *pHold = NULL; //optimization
	vector<CMonster*> monsters;

	try {
		UINT wMonsterI;
		const UINT wMonsterCount = MonstersView.GetSize();
		monsters.reserve(wMonsterCount);
		for (wMonsterI = 0; wMonsterI < wMonsterCount; ++wMonsterI)
		{
			c4_RowRef row = MonstersView[wMonsterI];
			CMonster *pNew = LoadMonster(row, pHold);
			if (pNew)
				monsters.push_back(pNew);
		}
	}
	catch (CException&)
	{
		for (vector<CMonster*>::const_iterator it=monsters.begin(); it!=monsters.end(); ++it)
		{
			CMonster *pMonster = *it;
			RemoveMonsterFromLayer(pMonster);
			delete pMonster;
		}

		delete pHold;

		return false;
	}

	for (vector<CMonster*>::const_iterator it=monsters.begin(); it!=monsters.end(); ++it)
	{
		CMonster *pMonster = *it;

		bool bInRoom = pMonster->IsVisible();

		LinkMonster(pMonster, bInRoom);
	}

	delete pHold;

	return true;
}

//*****************************************************************************
void CDbRoom::PlaceCharacters(CDbHold *pHold) //[default=NULL]
//As invisible NPCs are not placed on the monster layer in play (they are
//removed from the monster tile array in ::LoadMonsters), this method
//is used as a back door for the room editor to place invisible character
//monsters on their squares in the monster layer for easier editing access.
//
//If hold is specified, then also resolve the NPC's identity.
{
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pHold)
				pCharacter->ResolveLogicalIdentity(pHold);
			if (!pCharacter->IsVisible())
				SetMonsterSquare(pMonster);
		}
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
void CDbRoom::ClearMonsters(
//Puts room in a monster-free state.  Note that when a room is conquered,
//monsters that don't influence the conquering criterion should be retained.
//
//Params:
	const bool bRetainNonConquerableMonsters) //(in) if true, don't delete non-influential monsters
											//[default=false]
{
	//Traverse backwards, with order of retained monsters the same as before.
	CMonster *pDelete, *pCharacters = NULL, *pSeek = this->pLastMonster;
	this->pLastMonster = NULL;

	while (pSeek)
	{
		pDelete = pSeek;
		pSeek = pSeek->pPrevious;
		//Retain monsters that don't influence the conquering criterion.
		if (bRetainNonConquerableMonsters && !pDelete->IsConquerable())
		{
			pDelete->pNext = pCharacters;
			pDelete->pPrevious = NULL;
			if (pCharacters) pCharacters->pPrevious = pDelete;
			pCharacters = pDelete;
			if (!this->pLastMonster) this->pLastMonster = pDelete;
		} else {
			RemoveMonsterFromLayer(pDelete);
			switch (pDelete->wType)
			{
				case M_STALWART: case M_STALWART2:
				case M_TEMPORALCLONE:
					RemoveMonsterEnemy(pDelete);
				break;
				case M_DECOY:
					RemoveDecoy(pDelete);
				break;
				default: break;
			}
			delete pDelete;
		}
	}
	this->pFirstMonster = pCharacters;  //usually NULL
	this->wMonsterCount = this->wBrainCount = 0;

	//Don't delete Halph/Slayer entrance positions.
}

//*****************************************************************************
void CDbRoom::RemoveMonsterFromLayer(CMonster *pMonster)
{
	if (this->pMonsterSquares)
	{
		this->pMonsterSquares[ARRAYINDEX(pMonster->wX, pMonster->wY)] = NULL;

		MonsterPieces::const_iterator end = pMonster->Pieces.end();
		for (MonsterPieces::const_iterator piece = pMonster->Pieces.begin(); piece != end; ++piece)
			this->pMonsterSquares[ARRAYINDEX((*piece)->wX, (*piece)->wY)] = NULL;
	}
}

//*****************************************************************************
void CDbRoom::ClearDeadMonsters()
//Frees memory and resets members for dead monster list.
{
	for (list<CMonster *>::const_iterator iSeek = this->DeadMonsters.begin();
		iSeek != this->DeadMonsters.end(); ++iSeek)
	{
		CMonster *pDelete = *iSeek;
		ASSERT(pDelete);
		delete pDelete;
	}
	this->DeadMonsters.clear();
}

//*****************************************************************************
void CDbRoom::ClearDeadRoomObjects()
//Frees memory and resets members for dead room objects.
{
	for (list<RoomObject*>::const_iterator it = this->DeadRoomObjects.begin();
		it != this->DeadRoomObjects.end(); ++it)
	{
		RoomObject *pDelete = *it;
		ASSERT(pDelete);
		delete pDelete;
	}
	this->DeadRoomObjects.clear();
}

//*****************************************************************************
void CDbRoom::ClearPlatforms()
//Frees memory and resets members for 'platforms'.
{
	for (UINT wIndex=this->platforms.size(); wIndex--; )
		delete this->platforms[wIndex];
	this->platforms.clear();
	CPlatform::clearFallTiles();
}

//*****************************************************************************
void CDbRoom::ClearTLayer()
{
	for (list<RoomObject*>::const_iterator iSeek = this->tLayerObjects.begin();
		iSeek != this->tLayerObjects.end(); ++iSeek)
	{
		RoomObject *pDelete = *iSeek;
		ASSERT(pDelete);
		delete pDelete;
	}
	this->tLayerObjects.clear();

	//!!! FIXME: this->tLayer is sometimes NULL here
	if (this->tLayer)
		memset(this->tLayer, 0, CalcRoomArea() * sizeof(RoomObject*));
}

//*****************************************************************************
bool CDbRoom::LoadScrolls(
//Loads scrolls from database into member vars of object.
//
//Params:
	c4_View &ScrollsView)      //(in) Open view containing 0 or more scrolls.
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess=true;

	UINT wScrollI;
	CScrollData *pScroll;
	const UINT wScrollCount = ScrollsView.GetSize();
	if (wScrollCount > 0)
	{
		this->Scrolls.resize(wScrollCount);
		for (wScrollI = 0; wScrollI < wScrollCount; ++wScrollI)
		{
			c4_RowRef row = ScrollsView[wScrollI];
			this->Scrolls[wScrollI] = pScroll = new CScrollData;
			if (!pScroll) {bSuccess=false; goto Cleanup;}
			pScroll->wX = p_X(row);
			pScroll->wY = p_Y(row);
			pScroll->ScrollText.Bind((UINT) (p_MessageID(row)) );
		}
	}

Cleanup:
	if (!bSuccess)
	{
		//Free all scrolls that were allocated.
		while (wScrollI)
			delete this->Scrolls[--wScrollI];
		this->Scrolls.clear();
	}
	return bSuccess;
}

//*****************************************************************************
bool CDbRoom::LoadCheckpoints(
//Loads checkpoints from database into member vars of object.
//
//Params:
	c4_View &CheckpointsView)     //(in) Open view containing 0 or more scrolls.
//
//Returns:
//True if successful, false if not.
{
	for (UINT wCheckpointI = CheckpointsView.GetSize(); wCheckpointI--; )
	{
		c4_RowRef row = CheckpointsView[wCheckpointI];
		this->checkpoints.insert(p_X(row), p_Y(row));
	}

	return true;
}

//*****************************************************************************
void CDbRoom::AddDiagonalDoorAssociations()
//Rooms from version 1.6 consider yellow doors to be connected diagonally (8-neighbor).
//Version 2.0 and later use 4-neighbor connections for orb effects.
//This method preserves the diagonal connectivity of yellow doors intended in 1.6 holds.
{
	CCoordSet squares8, squares4;
	CTileMask doorMask(T_DOOR_YO);
	doorMask.set(T_DOOR_Y);
	UINT wX, wY;

	for (UINT wOrbIndex=this->orbs.size(); wOrbIndex--; )
	{
		COrbData *pOrb = this->orbs[wOrbIndex];
		ASSERT(pOrb);
		for (UINT wAgentIndex=pOrb->agents.size(); wAgentIndex--; ) //must go backwards
		{
			COrbAgentData *pAgent = pOrb->agents[wAgentIndex];
			ASSERT(pAgent);
			wX = pAgent->wX;
			wY = pAgent->wY;
			ASSERT(IsValidColRow(wX, wY));

			//Get all door tiles to be affected.
			GetConnected8NeighborTiles(wX, wY, doorMask, squares8);

			//Remove door tiles affected by agent's 4-neighbor connected component.
			GetConnected4NeighborTiles(wX, wY, doorMask, squares4);
			squares8 -= squares4;

			//Set an agent to any other 4-connected piece.
			while (squares8.pop_first(wX,wY))
			{
				pOrb->AddAgent(wX, wY, pAgent->action);
				GetConnected4NeighborTiles(wX, wY, doorMask, squares4);
				squares8 -= squares4;
			}
		}
	}
}

//*****************************************************************************
bool CDbRoom::AddOrb(
//Adds orb to room object.
//
//Params:
	COrbData *pOrb)   //(in) orb object
{
	ASSERT(pOrb);
	if (GetTSquare(pOrb->wX,pOrb->wY) != T_ORB &&
			GetOSquare(pOrb->wX,pOrb->wY) != T_PRESSPLATE)
		return false;   //no orb actually here

	this->orbs.push_back(pOrb);
	return true;
}

//*****************************************************************************
COrbData* CDbRoom::AddOrbToSquare(
//Adds orb at coords in room object.
//
//Returns: pointer to new orb data
//
//Params:
	const UINT wX, const UINT wY) //(in) Square with orb
{
	ASSERT(IsValidColRow(wX,wY));
	const UINT wOTile = GetOSquare(wX,wY);
	ASSERT(GetTSquare(wX,wY) == T_ORB || wOTile == T_PRESSPLATE);

	//Add new orb to list.
	COrbData *pNewOrb = new COrbData(wX, wY);
	this->orbs.push_back(pNewOrb);

	//Pressure plate tile compilation and indexing.
	if (wOTile == T_PRESSPLATE)
		VERIFY(AddPressurePlateTiles(pNewOrb));

	return pNewOrb;
}

//*****************************************************************************
bool CDbRoom::AddPressurePlateTiles(COrbData* pPlate)
//Generate and mark the set of tiles the pressure plate covers.
{
	//Find plate's position in list.
	ASSERT(pPlate);
	UINT wOrbIndex;
	for (wOrbIndex=this->orbs.size(); wOrbIndex--; )
		if (pPlate == this->orbs[wOrbIndex])
			break; //found index of pressure plate
	if (wOrbIndex >= this->orbs.size())
		return false; //not found -- do nothing

	//Remove old set of tiles marked for this pressure plate.
	this->pressurePlateIndex.RemoveAll(wOrbIndex+1);

	//Mark current set of tiles for this pressure plate.
	CTileMask mask(T_PRESSPLATE);
	GetConnected4NeighborTiles(pPlate->wX, pPlate->wY, mask, pPlate->tiles);
	for (CCoordSet::const_iterator tile=pPlate->tiles.begin();
			tile!=pPlate->tiles.end(); ++tile)
		this->pressurePlateIndex.Add(tile->wX, tile->wY, wOrbIndex+1); //base-1
	return true;
}

//*****************************************************************************
bool CDbRoom::AddScroll(
//Adds scroll to room object.
//
//Params:
	CScrollData *pScroll)   //(in) scroll object
{
	ASSERT(pScroll);
	if (GetTSquare(pScroll->wX,pScroll->wY) != T_SCROLL)
		return false;  //no scroll actually here

	this->Scrolls.push_back(pScroll);
	return true;
}

//*****************************************************************************
bool CDbRoom::AddExit(
//Adds an Exit object to the room
//
//Params:
	CExitData *pExit)
{
	ASSERT(pExit);
	if (!bIsStairs(GetOSquare(pExit->wLeft,pExit->wTop)))
		return false;  //no exit actually here

	this->Exits.push_back(pExit);
	return true;
}

//*****************************************************************************
void CDbRoom::AddPlatformPiece(
//Add connected component platform including coord (wX,wY) to room platforms,
//and mark off this area of the room so it won't be checked for addition again.
//
//Params:
	 const UINT wX, const UINT wY,   //(in) Square with platform
	 CCoordIndex &plots)             //(in/out) Known squares with platform
{
	CCoordSet platform;
	const UINT wOSquare = GetOSquare(wX,wY);
	CTileMask mask(wOSquare);
	GetConnected4NeighborTiles(wX, wY, mask, platform);
	const bool bPit = wOSquare == T_PLATFORM_P;
	CIDSet types(bPit ? T_PIT : T_WATER);
	if (bPit)
		types += T_PIT_IMAGE;
	this->platforms.push_back(new CPlatform(platform, types));
	platform.AddTo(plots);
}

//*****************************************************************************
void CDbRoom::DeleteExitAtSquare(
//Removes all exit data at coord from room object.
//
//Params:
	 const UINT wX, const UINT wY)   //(in) Square with stairs
{
	for (UINT i=this->Exits.size(); i--; )
	{
		const CExitData &stairs = *(this->Exits[i]);
		if (IsInRect(wX, wY,
				stairs.wLeft, stairs.wTop, stairs.wRight, stairs.wBottom))
		{
			//Remove this data from list.
			delete this->Exits[i];
			this->Exits[i] = this->Exits[this->Exits.size() - 1];
			this->Exits.pop_back();
		}
	}
}

//*****************************************************************************
void CDbRoom::DeleteOrbAtSquare(
//Removes orb data at coord from room object.
//
//Params:
	const UINT wX, const UINT wY) //(in) Square with orb
{
	ASSERT(GetTSquare(wX,wY) == T_ORB || GetOSquare(wX,wY) == T_PRESSPLATE);

	bool bFound = false;
	UINT wOrbI;
	for (wOrbI=this->orbs.size(); wOrbI--; )
	{
		if (wX == this->orbs[wOrbI]->wX && wY == this->orbs[wOrbI]->wY)
		{
			//Found it.  Remove orb object (move last one forward).
			bFound = true;
			delete this->orbs[wOrbI];
			this->orbs[wOrbI] = this->orbs[this->orbs.size() - 1];
			this->orbs.pop_back();
		}
	}

	//Rebuild pressure plate index.
	if (!bFound || !this->pressurePlateIndex.GetSize())
		return; //nothing to rebuild
	this->pressurePlateIndex.Clear();
	for (wOrbI=this->orbs.size(); wOrbI--; )
	{
		COrbData *pOrb = this->orbs[wOrbI];
		for (CCoordSet::const_iterator tile=pOrb->tiles.begin();
				tile!=pOrb->tiles.end(); ++tile)
			this->pressurePlateIndex.Add(tile->wX, tile->wY, wOrbI+1); //base-1
	}
}

//*****************************************************************************
void CDbRoom::DeleteScrollTextAtSquare(
//Removes scroll text at coord from room object.
//
//Params:
	const UINT wX, const UINT wY) //(in) Square with scroll
{
	ASSERT(GetTSquare(wX,wY) == T_SCROLL);

	//Search for scroll in room data.
	UINT wScrollI;
	for (wScrollI=this->Scrolls.size(); wScrollI--; )
	{
		if (this->Scrolls[wScrollI]->wX == wX && this->Scrolls[wScrollI]->wY == wY)
		{
			//Found it.  Put scroll text ID in list of message texts to remove from DB on Update.
			const UINT dwMessageID = this->Scrolls[wScrollI]->ScrollText.GetMessageID();
			if (dwMessageID)
				this->deletedScrollIDs.push_back(dwMessageID);
			//Remove from scrolls array (copy last item pointer over it and shrink count).
			delete this->Scrolls[wScrollI];
			this->Scrolls[wScrollI] = this->Scrolls[this->Scrolls.size()-1];
			this->Scrolls.pop_back();
		}
	}
}

//*****************************************************************************
bool CDbRoom::LoadExits(
//Loads exits from database into member vars of object.
//
//Params:
	c4_View &ExitsView)  //(in) Open view containing 0 or more exits.
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess=true;

	const UINT wSize = ExitsView.GetSize();
	UINT wExitI;
	for (wExitI=0; wExitI<wSize; ++wExitI)
	{
		c4_RowRef row = ExitsView[wExitI];
		CExitData *pExit = new CExitData(p_EntranceID(row),
				p_Left(row), p_Right(row),
				p_Top(row), p_Bottom(row));
		if (!pExit) {bSuccess = false; goto Cleanup;}
		this->Exits.push_back(pExit);
	}

Cleanup:
	if (!bSuccess)
	{
		for (wExitI=this->Exits.size(); wExitI--; )
			delete this->Exits[wExitI];
		this->Exits.clear();
	}
	return bSuccess;
}

//*****************************************************************************
void CDbRoom::SetCurrentGameForMonsters(
//Sets the current game pointer for all monsters in the room.  The monsters
//can't be used for current game operations until this is done.
//
//Params:
	const CCurrentGame *pSetCurrentGame)
{
	ASSERT(pSetCurrentGame);

	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		pMonster->SetCurrentGame(pSetCurrentGame);
}

//*****************************************************************************
void CDbRoom::SetRoomEntryState(
//Prepares room state for game play.
//
//Params:
	CCueEvents& CueEvents, //(in/out)
	const bool bWasLevelComplete,
	const bool bIsCurrentLevelComplete,
	const bool bWasRoomConquered,
	UINT& wMonsterCountAtStart) //(in/out)
{
	//See if room is already conquered.
	if (bWasRoomConquered)
	{
		ToggleGreenDoors(CueEvents);
		ClearMonsters(true);
		ChangeTiles(ConquerToken);
		DeactivateBeacons();
		wMonsterCountAtStart = 0;
		bHasActiveBeacon = false;
	}
	//Not conquered, but if no monsters/conquer tokens/beacons in it, then add to conquered list immediately.
	else if (!wMonsterCountAtStart && !this->bHasConquerToken && !this->bHasActiveBeacon)
	{
		CueEvents.Add(CID_ConquerRoom);
		ToggleGreenDoors(CueEvents);
		ClearMonsters(true); //remove monsters that only show in unconquered rooms
	} else {
	   //Clear first turn status on new room monsters.
		ResetMonsterFirstTurnFlags();
	}

	AddRunningGlobalScripts(CueEvents);
	SetHalphSlayerEntrance();
	RemoveFinishedCharacters();

	//Clear data objects previously stored in room.  They were kept in
	//memory to keep private data pointers valid.
	ClearDeadMonsters();
	ClearDeadRoomObjects();

	KillMonstersOnHazard(CueEvents); //remove before any doors change
	KillFluffOnHazard(CueEvents);

	//Remove blue doors if level is complete.
	if (bIsCurrentLevelComplete)
	{
		if (!bWasLevelComplete)
			CueEvents.Add(CID_CompleteLevel);
		ToggleTiles(T_DOOR_C, T_DOOR_CO, CueEvents); //blue/exit door
		ConvertUnstableTar(CueEvents);
	}

	//Remove red(black) doors if no trapdoors(tarstuff) in the room.
	//This would arise from a level editing error.
	if (this->wTrapDoorsLeft == 0)
	{
		ToggleTiles(T_DOOR_R, T_DOOR_RO, CueEvents); //red door
		ConvertUnstableTar(CueEvents);
	}

	//Remove black doors if no tarstuff is in the room.
	if (!this->wTarLeft)
	{
		ToggleBlackGates(CueEvents);
		ConvertUnstableTar(CueEvents);
	}
	else if (!wMonsterCountAtStart)
	{
		//Remove unstable tar that might have occurred underneath dead mothers.
		FixUnstableTar(CueEvents);
	}

	//Mark which pressure plates are depressed on entrance.
	SetPressurePlatesState();
}

//*****************************************************************************
void CDbRoom::SetScrollTextAtSquare(
//Updates scroll text at coord in room object.
//If there is no scroll text there, add it.
//
//Params:
	const UINT wX, const UINT wY, //(in) Square with scroll
	const WCHAR* pwczScrollText)        //(in) Text to assign
{
	ASSERT(GetTSquare(wX,wY) == T_SCROLL);

	//Search for scroll in room data.
	UINT wScrollI;
	for (wScrollI=this->Scrolls.size(); wScrollI--; )
	{
		if (this->Scrolls[wScrollI]->wX == wX &&
				this->Scrolls[wScrollI]->wY == wY)
		{
			//Update text of existing scroll.
			this->Scrolls[wScrollI]->ScrollText = pwczScrollText;
			return;
		}
	}

	//Add new scroll.
	CScrollData *pNewScroll = new CScrollData;
	pNewScroll->ScrollText = pwczScrollText;
	pNewScroll->wX = wX;
	pNewScroll->wY = wY;
	this->Scrolls.push_back(pNewScroll);
}

//*****************************************************************************
bool CDbRoom::NewGelWouldBeStable(
//Determines whether new gel could be placed at this spot (as opposed to a gel baby)
//according to rule that a minimum of a 2x2 square of tarstuff can exist.
//It must be adjacent to gel connected to a gel mother to be stable.
//
//Params:
	const vector<tartype> &addedGel, //(in) where gel is located in room
	const UINT tx, const UINT ty,    //(in) square where gel is growing
	const CCoordSet& contiguousGel)  //(in) currently only used to represent the set
									 //of gel tiles connected to gel mothers
//
//Returns:
//True if gel should go here, false if gel baby.
{
	bool gel[3][3] = {{false}};
	bool adjGel[2][2] = {{false}};

	//mark where tar is
	const UINT endX = tx+2, endY = ty+2;
	UINT x, y;
	for (y=ty-1; y!=endY; ++y)
	{
		if (y >= wRoomRows) continue;
		const int dy = y-ty+1;
		const UINT yIndex = y * wRoomCols;
		for (x=tx-1; x!=endX; ++x)
		{
			if (x >= wRoomCols) continue;
			const int dx = x-tx+1;
			if (dx == 1 && dy == 1) continue;
			gel[dx][dy] = addedGel[yIndex + x] != notar;

			//Check for any gel connected to a gel mother.
			if (contiguousGel.has(x,y))
			{
				if (dx <= 1)
				{
					if (dy <= 1)
						adjGel[0][0] = true;
					if (dy >= 1)
						adjGel[0][1] = true;
				}
				if (dx >= 1)
				{
					if (dy <= 1)
						adjGel[1][0] = true;
					if (dy >= 1)
						adjGel[1][1] = true;
				}
			}
		}
	}

	return
		(gel[0][0] && gel[0][1] && gel[1][0] && adjGel[0][0]) ||  //upper-left corner
		(gel[0][2] && gel[0][1] && gel[1][2] && adjGel[0][1]) ||  //lower-left corner
		(gel[2][0] && gel[2][1] && gel[1][0] && adjGel[1][0]) ||  //upper-right corner
		(gel[2][2] && gel[2][1] && gel[1][2] && adjGel[1][1]);   //lower-right corner
}

//*****************************************************************************
bool CDbRoom::NewTarWouldBeStable(
//Determines whether new tar/mud/fluff could be placed at this spot (as opposed to a tar baby)
//according to rule that a minimum of a 2x2 square of tarstuff can exist.
//
//Params:
	const vector<tartype> &addedTar, //(in) where tar is located in room
	const UINT tx, const UINT ty)    //(in) square where tar is growing
//
//Returns:
//True if tar should go here, false if tar baby.
{
	bool tar[3][3] = {{false}};

	//mark where tar is
	const UINT endX = tx+2, endY = ty+2;
	UINT x, y;
	for (y=ty-1; y!=endY; ++y)
		if (y < wRoomRows)
		{
			const int dy = y-ty+1;
			const UINT yIndex = y * wRoomCols;
			for (x=tx-1; x!=endX; ++x)
				if (x < wRoomCols)
					tar[x-tx+1][dy] = addedTar[yIndex + x] != notar;
		}

	return
		(tar[0][0] && tar[0][1] && tar[1][0]) ||  //upper-left corner
		(tar[0][2] && tar[0][1] && tar[1][2]) ||  //lower-left corner
		(tar[2][0] && tar[2][1] && tar[1][0]) ||  //upper-right corner
		(tar[2][2] && tar[2][1] && tar[1][2]);    //lower-right corner
}

//*****************************************************************************
void CDbRoom::GetTarConnectedComponent(
//Compiles set of all connected tiles containing tarstuff
//of the same type as at (wX,wY).
//
//Note that this search is necessarily more complicated than a naive flood fill
//because adjacent tar pieces might not be semantically connected.
//
//Params:
	const UINT wFirstX, const UINT wFirstY, //(in) square to check from
	CCoordSet& tiles,         //(out) set of contiguous tiles
	const bool bAddAdjOnly) //only add the 8 adjacent tiles, then stop [default=false]
const
{
	tiles.clear();

	const UINT wTar = GetTSquare(wFirstX,wFirstY);
	if (!bIsTarOrFluff(wTar))
	{
		tiles.insert(wFirstX,wFirstY);
		return;
	}

	CCoordStack cstack(wFirstX,wFirstY);
	UINT wX, wY;
	bool bAddTop, bAddBottom, bAddLeft, bAddRight;

	while (cstack.PopBottom(wX,wY)) //perform as a queue for performance
	{
		if (!tiles.insert(wX, wY))
			continue; //don't need to reprocess this tile if it was already handled

#define PushTileIfOfType(wX, wY) do {\
	ASSERT(IsValidColRow((wX), (wY)));\
	if (!tiles.has((wX), (wY)))\
		cstack.Push ((wX), (wY));} while(0)

		//For each of the four diagonals adjacent to this tar tile:
		//  check whether it and the two axial pieces are present.
		//  If so, add the corner, and a flag for the axial pieces.
		//  Add piece for each set flags.
		const bool
		   bTop = wY > 0 && wTar == GetTSquare(wX,wY-1),
		bBottom = wY < this->wRoomRows-1 && wTar == GetTSquare(wX,wY+1),
		  bLeft = wX > 0 && wTar == GetTSquare(wX-1,wY),
		 bRight = wX < this->wRoomCols-1 && wTar == GetTSquare(wX+1,wY);

		bAddTop = bAddBottom = bAddLeft = bAddRight = false;
	  if (bTop && bLeft && wTar == GetTSquare(wX-1,wY-1))
		{
			PushTileIfOfType(wX-1,wY-1);
			bAddTop = bAddLeft = true;
		}
	  if (bTop && bRight && wTar == GetTSquare(wX+1,wY-1))
		{
			PushTileIfOfType(wX+1,wY-1);
			bAddTop = bAddRight = true;
		}
	  if (bBottom && bLeft && wTar == GetTSquare(wX-1,wY+1))
		{
			PushTileIfOfType(wX-1,wY+1);
			bAddBottom = bAddLeft = true;
		}
	  if (bBottom && bRight && wTar == GetTSquare(wX+1,wY+1))
		{
			PushTileIfOfType(wX+1,wY+1);
			bAddBottom = bAddRight = true;
		}

		if (bAddTop)
			PushTileIfOfType(wX,wY-1);
		if (bAddBottom)
			PushTileIfOfType(wX,wY+1);
		if (bAddLeft)
			PushTileIfOfType(wX-1,wY);
		if (bAddRight)
			PushTileIfOfType(wX+1,wY);

#undef PushTileIfOfType

		if (bAddAdjOnly)
		{
			//Add tiles in stack from first pass and stop.
			while (cstack.PopBottom(wX,wY))
				tiles.insert(wX, wY);
			break;
		}
	}
}

//*****************************************************************************
void CDbRoom::GrowTar(
//Grows the tarstuff and creates tar babies.
//
//Params:
	CCueEvents &CueEvents,  //(out)  May receive some new cue events.
	CCoordIndex &babies,    //(in/out)  Where another tar flavor formed babies this turn
	CCoordIndex &SwordCoords,  //(in)  Where swords are
	const UINT wTarType)    //(in)   Tar tile type
{
	//This operation requires room to be attached to a current game.
	ASSERT(this->pCurrentGame);

	//Player's position.
	UINT wSManX = UINT(-1), wSManY = UINT(-1);
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom())
	{
		wSManX = player.wX;
		wSManY = player.wY;
	}

	UINT wMotherType = M_TARMOTHER;
	CUEEVENT_ID cid = CID_TarGrew;
	switch (wTarType)
	{
		case T_TAR: wMotherType = M_TARMOTHER; cid = CID_TarGrew; break;
		case T_MUD: wMotherType = M_MUDMOTHER; cid = CID_MudGrew; break;
		case T_GEL: wMotherType = M_GELMOTHER; cid = CID_GelGrew; break;
		default: ASSERT(!"Bad tar type"); break;
	}

	//Check whether any mothers of this type are still alive.  No growing if not.
	if (!IsGrowingTarstuffMotherAlive(CueEvents, cid))
		return;

	bool bTarstuffGrew = false;
	const CMonster *pMonster;

	//Only grow gel contiguously from existing mothers.  Skip redundancies.
	CCoordSet contiguousGel;
	const bool bGel = wTarType == T_GEL;
	if (bGel)
	{
		CCoordSet newGel;
		pMonster = DYN_CAST(const CMonster*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(cid));
		ASSERT(pMonster);
		while (pMonster)
		{
			if (pMonster->bAlive)
			{
				//If a gel Mother doesn't have gel under them, then flag the square to be revisited later.
				if (GetTSquare(pMonster->wX, pMonster->wY) != wTarType)
					newGel.insert(pMonster->wX, pMonster->wY);
				else if (!contiguousGel.has(pMonster->wX, pMonster->wY))
				{
					CCoordSet tiles;
					GetTarConnectedComponent(pMonster->wX, pMonster->wY, tiles);
					contiguousGel += tiles;
				}
			}
			pMonster = DYN_CAST(const CMonster*, const CAttachableObject*,
					CueEvents.GetNextPrivateData());
		}
		//Revisit aware gel Mothers that are not on gel to add them to growth and place gel under them.
		for (CCoordSet::const_iterator tile=newGel.begin(); tile!=newGel.end(); ++tile)
		{
			Plot(tile->wX, tile->wY, wTarType);
			++this->wTarLeft;
			ASSERT(GetTSquare(tile->wX, tile->wY) == wTarType);
			contiguousGel.insert(tile->wX, tile->wY);
			bTarstuffGrew = true;
		}
	} else {
		//Ensure tarstuff is under each mother now, aware of the player or not.
		for (pMonster = this->pFirstMonster; pMonster; pMonster = pMonster->pNext)
		{
			if (pMonster->wType == wMotherType && GetTSquare(pMonster->wX, pMonster->wY) != wTarType)
			{
				if (this->wTarLeft == 0)
					this->bTarstuffGateTogglePending = true;

				Plot(pMonster->wX, pMonster->wY, wTarType);
				++this->wTarLeft;
				ASSERT(GetTSquare(pMonster->wX, pMonster->wY) == wTarType);
				bTarstuffGrew = true;
			}
		}
	}

	//Calculate where tarstuff might grow.
	vector<tartype> added_tar(CalcRoomArea());
	CCoordStack possible;
	UINT x, y;
	for (x = 0; x < this->wRoomCols; ++x)
		for (y = 0; y < this->wRoomRows; ++y)
		{
			//Determine where old tarstuff is.
			UINT wTile = GetTSquare(x, y);
			const CMonster *pMonster = GetMonsterAtSquare(x, y);
			const UINT pos = ARRAYINDEX(x,y);  //shorthand
			if (wTile == wTarType)
			{
				added_tar[pos] = oldtar;
				continue;
			}
			added_tar[pos] = notar;
			//Determine whether new tarstuff can go here.
			wTile = GetOSquare(x, y);
			UINT wFTile = GetFSquare(x, y);
			if ((bIsFloor(wTile) || bIsOpenDoor(wTile) || bIsPlatform(wTile)) &&
				(wFTile == T_EMPTY || bIsDisabledArrow(wFTile)) && 
				GetTSquare(x, y) == T_EMPTY &&
				!(x == wSManX && y == wSManY) &&
					(!pMonster || pMonster->wType == wMotherType ||
					pMonster->wType == M_FLUFFBABY || babies.Exists(x,y)))
			{
				for (UINT o = 0; o < ORIENTATION_COUNT; ++o)
				{
					if (o == NO_ORIENTATION) continue;
					const int nx = x + nGetOX(o);
					const int ny = y + nGetOY(o);

					if (!IsValidColRow(nx, ny)) continue;
					if (GetTSquare(nx, ny) == wTarType &&
							(!bGel || contiguousGel.has(nx,ny))) //gel must be contiguous to some mother
					{
						//Eligible tarstuff is adjacent to this square, so tar might grow here.
						added_tar[pos] = newtar;
						possible.Push(x, y);
						break;
					}
				}
			}
		}

	//Calculate whether tar or tar babies are placed where tarstuff grows.
	UINT wCount=0;
	while (possible.Pop(x, y))
	{
		ASSERT(added_tar[ARRAYINDEX(x,y)] == newtar); //something is growing here
		if ((!bGel && NewTarWouldBeStable(added_tar,x,y)) ||
			  (bGel && NewGelWouldBeStable(added_tar,x,y,contiguousGel)))
		{
			//Tar is growing here.

			//If baby of this or another tar-flavor was just spawned here, replace it with this flavor of tar.
			if (babies.Exists(x,y))
			{
				if (babies.GetAt(x,y) == T_FLUFF)
					ActivateOrb(x,y,CueEvents,OAT_PressurePlate); //can depress pressure plate
				else
					KillMonsterAtSquare(x,y, CueEvents);
				babies.Remove(x,y);
			} else {
				CMonster *pFluff = GetMonsterAtSquare(x,y);
				if (pFluff && pFluff->wType == M_FLUFFBABY)
				{
					KillMonster(pFluff,CueEvents);
					ProcessPuffAttack(CueEvents,x,y);
				}
				ActivateOrb(x,y,CueEvents,OAT_PressurePlate); //can depress pressure plate
			}

			Plot(x, y, wTarType);
			++this->wTarLeft;
			if (bGel)
			{
				contiguousGel.insert(x,y); //this gel is likewise connected to a gel mother
				wCount=0;  //Need to re-iterate through the entire remaining queue
						   //to ensure no remaining new tiles might also be stable.
			}
			bTarstuffGrew = true;
		} else {
			//If we got this far, then any monster occupying this tile must also block a new baby
			const CMonster *pMonsterObstacle = GetMonsterAtSquare(x,y);
			if (!pMonsterObstacle || pMonsterObstacle->wType == M_FLUFFBABY)
			{
				//This tile might turn into a baby.
				possible.PushBottom(x,y);
				++wCount;
			}
		}
		ASSERT(wCount <= possible.GetSize());
		if (wCount == possible.GetSize())
			break; //everything left in 'possible' at this point is unstable tarstuff/babies
	}

	//These tiles are free to place a baby.
	while (possible.Pop(x, y))
	{
		//tar baby can't grow under sword or new baby of different tar flavor
		if (SwordCoords.Exists(x,y) || babies.Exists(x,y))
			continue;

		CMonster *pFluff = GetMonsterAtSquare(x,y);
		if (pFluff && pFluff->wType == M_FLUFFBABY)
		{
			//Kill the Puff instead of spawning a baby
			KillMonster(pFluff,CueEvents);
			ProcessPuffAttack(CueEvents,x,y);
			//Mark babies CCoordIndex so that no other babies spawn here
			babies.Add(x,y,T_FLUFF);
			continue;
		}

		CMonster *m;
		switch (wTarType)
		{
			case T_TAR:
				m = AddNewMonster(M_TARBABY,x,y);
				CueEvents.Add(CID_TarBabyFormed, m);
			break;
			case T_MUD:
				m = AddNewMonster(M_MUDBABY,x,y);
				CueEvents.Add(CID_MudBabyFormed, m);
			break;
			case T_GEL:
				m = AddNewMonster(M_GELBABY,x,y);
				CueEvents.Add(CID_GelBabyFormed, m);
			break;
			default: ASSERT(!"Bad tar type"); continue;
		}
		m->bIsFirstTurn = false;
		babies.Add(x,y,wTarType);
		ActivateOrb(x,y,CueEvents,OAT_PressurePlate); //can depress pressure plate
		bTarstuffGrew = true;
	}

	if (bTarstuffGrew)
		CueEvents.Add(CID_TarstuffGrew, new CAttachableWrapper<UINT>(wTarType), true);
}

//*****************************************************************************
bool CDbRoom::IsGrowingTarstuffMotherAlive(CCueEvents &CueEvents, CUEEVENT_ID cid)
{
	if (!CueEvents.HasOccurred(cid))
		return false;

	const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*,
			CueEvents.GetFirstPrivateData(cid));
	while (pMonster)
	{
		if (pMonster->bAlive)
			return true;

		pMonster = DYN_CAST(const CMonster*, const CAttachableObject*,
				CueEvents.GetNextPrivateData());
	}
	return false;
}

//*****************************************************************************
bool CDbRoom::StabTar(
//Update game for results of stabbing a square containing tar.
//Do not use for removing Fluff.
//
//Params:
	const UINT wX, const UINT wY, //(in)      Tar square coords.
	CCueEvents &CueEvents,  //(in/out)  If tar is destroyed, a cue event will be added.
	const bool removeTarNow,   //(in) If true, automatically stab tar.
														//    If false, only determine whether effect of stabbing tar would destroy tar or not
	const UINT wStabO)		//(in) direction of stab [default=NO_ORIENTATION]
//
//Returns:
//True if tar will be removed, else false.
{
	//Operation should only be performed when room is attached to a current game.
	ASSERT(this->pCurrentGame);

	if (removeTarNow) {
		//Remove tar (effect of possibly simultaneous hits).
		//In this case, checking for vulnerability of this spot should have been
		//performed previously.
		const UINT wTSquare = GetTSquare(wX, wY);
		if (bIsTar(wTSquare))
		{
			RemoveStabbedTar(wX, wY, CueEvents);
			CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, wStabO, wTSquare), true);
			this->bTarWasStabbed = true;
			return true;
		}
		return false;
	}

	//If the tar is vulnerable, flag to remove it.
	return IsTarVulnerableToStab(wX, wY);
}

//*****************************************************************************
void CDbRoom::FixUnstableTar(CCueEvents &CueEvents)
//Remove unstable tarstuff.
{
	bool bStable;
	UINT wX, wY, wTTile;
	do {
		bStable = true;
		for (wY=0; wY<this->wRoomRows; ++wY)
			for (wX=0; wX<this->wRoomCols; ++wX)
			{
				wTTile = GetTSquare(wX,wY);
				if (bIsTar(wTTile) && !IsTarStableAt(wX,wY,wTTile))
				{
					CMonster *pMonster = GetMonsterAtSquare(wX,wY);
					if (pMonster && bIsMother(pMonster->wType))
						continue; //allow unstable tarstuff to remain under a mother
					DestroyTar(wX, wY, CueEvents);
					bStable = false;
				}
			}
	} while (!bStable);
}


//*****************************************************************************
void CDbRoom::BreakUnstableTar(CCueEvents &CueEvents)
//Breaks unstable tarstuff into tarbabies.
{
	bool bShallowPathmapNeeded = false;
	const UINT wSX = this->pCurrentGame ? this->pCurrentGame->swordsman.wX : (UINT)-1;
	const UINT wSY = this->pCurrentGame ? this->pCurrentGame->swordsman.wY : (UINT)-1;
	CCoordIndex swordCoords;
	GetSwordCoords(swordCoords);

	bool bStable;
	UINT wX, wY;
	do {
		bStable = true;
		for (wY = 0; wY<this->wRoomRows; ++wY)
		for (wX = 0; wX<this->wRoomCols; ++wX)
		{
			const UINT wTarType = GetTSquare(wX, wY);
			const bool bFluff = wTarType == T_FLUFF;
			if (bIsTarOrFluff(wTarType) && !IsTarStableAt(wX, wY, wTarType))
			{
				//Get rid of the unstable tar
				if (bFluff){
					RemoveStabbedTar(wX, wY, CueEvents, true);
					CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, T_FLUFF), true);

				} else {
					CMonster* pMonster = GetMonsterAtSquare(wX, wY);
					if (pMonster && bIsMother(pMonster->wType))
						continue; //allow unstable tarstuff to remain under a mother
					CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTarType), true);
					DestroyTar(wX, wY, CueEvents);
				}

				bStable = false;
				CMonster *pMonster = GetMonsterAtSquare(wX, wY);

				//Don't spawn tar babies under living monsters
				if (!pMonster)
				{
					//Spawn a tarbaby.
					const UINT wOSquare = GetOSquare(wX, wY);

					if (bFluff)
					{
						if (!(bIsStairs(wOSquare) || (wX == wSX && wY == wSY)))
						{
							//Spawn a puff
							CMonster *m = NULL;
							m = AddNewMonster(M_FLUFFBABY, wX, wY);
							//FIXME-Fluff: Need CID_FluffBabyFormed
							if (this->pCurrentGame)
								m->SetCurrentGame(this->pCurrentGame);
							//Fluff Babies never move on the same turn they're converted
							m->bIsFirstTurn = true;
							m->SetOrientation(sgn(wSX - wX), sgn(wSY - wY));
						}
					}
					//Don't create babies on swords, stairs, pits, water or the player.
					else if (!(swordCoords.Exists(wX, wY) || bIsStairs(wOSquare) || bIsPit(wOSquare) ||
						bIsDeepWater(wOSquare) || (bIsShallowWater(wOSquare) && wTarType != T_GEL) ||
						(wX == wSX && wY == wSY)))
					{
						//Active spike traps prevent babies being formed, but unlike other tiles, we should
						//cause an additional graphical explosion effect since the spike is only visible for a brief time
						if (wOSquare == T_FLOOR_SPIKES && GetFloorSpikesState() == FloorSpikesUp)
							CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTarType), true);
						else
						{
							bShallowPathmapNeeded = true; //Tarstuff Babies may require Shallow Water pathmap
							//Spawn a tarbaby.
							CMonster *m = NULL;
							switch (wTarType)
							{
							case T_TAR:
								m = AddNewMonster(M_TARBABY, wX, wY);
								CueEvents.Add(CID_TarBabyFormed, m);
								break;
							case T_MUD:
								m = AddNewMonster(M_MUDBABY, wX, wY);
								CueEvents.Add(CID_MudBabyFormed, m);
								break;
							case T_GEL:
								m = AddNewMonster(M_GELBABY, wX, wY);
								CueEvents.Add(CID_GelBabyFormed, m);
								break;
							case T_FLUFF:
								m = AddNewMonster(M_FLUFFBABY, wX, wY);
								break;
							default: ASSERT(!"Bad tar type"); continue;
							}
							if (this->pCurrentGame)
								m->SetCurrentGame(this->pCurrentGame);
							m->bIsFirstTurn = true;
							m->SetOrientation(sgn(wSX - wX), sgn(wSY - wY));
						}
					}
				}
			}
		}
	} while (!bStable);

	if (bShallowPathmapNeeded && this->pCurrentGame && !this->pPathMap[GROUND_AND_SHALLOW_WATER])
		CreatePathMaps();

}
//*****************************************************************************
bool CDbRoom::IsTarStableAt(
//Determines whether tar placed at this square would not turn into a tar baby,
//according to the rule that a minimum of a 2x2 square of tar can exist.
//
//Params:
	const UINT wX, const UINT wY, //(in) square being considered
	const UINT wTarType)          //(in) tar tile type
//
//Returns:
//True if tar would be stable here, false if it should be a tar baby.
const
{
	bool tar[3][3] = {{false}};   //center position is square being considered
	UINT x, y;

	//mark where tar is
	for (x=wX-1; x!=wX+2; x++)
		if (x < wRoomCols)
			for (y=wY-1; y!=wY+2; y++)
				if (y < wRoomRows)
					tar[x-wX+1][y-wY+1] =
							(GetTSquare(x,y) == wTarType);

	return (
		(tar[0][0] && tar[0][1] && tar[1][0]) ||  //upper-left corner
		(tar[0][2] && tar[0][1] && tar[1][2]) ||  //lower-left corner
		(tar[2][0] && tar[2][1] && tar[1][0]) ||  //upper-right corner
		(tar[2][2] && tar[2][1] && tar[1][2]));   //lower-right corner
}

//*****************************************************************************
bool CDbRoom::IsTarVulnerableToStab(
//Checks that a square of tar is vulnerable to a sword stab, either from the
//swordsman or a mimic.
//
//Params:
	const UINT wX, const UINT wY) //(in) Square to check.
//
//Returns:
//True if square is vulnerable, false if not.
const
{
	const UINT wTSquare = GetTSquare(wX, wY);
	ASSERT(bIsTar(wTSquare));

	static const int orthogonal[] = {N, E, S, W};
	// Tar can be destroyed if it has a flat edge.  Tar has a flat edge if:
	// a) there is a non-tar space orthogonally adjacent to the tar, and
	// b) the 6 squares on the other side of the empty space are all tar.
	// For example:
	//                        (T = tar)
	//  TTT  TT.  .n.  .TT    (x = stabbed tar)
	//  TxT  Txn  TxT  nxT    (n = non-tar)
	//  .n.  TT.  TTT  .TT    (. = any)
	//
	// Mud can be destroyed on a curved edge.  Mud has a curved edge if:
	// a) same as (a) above
	// b) the opposite of (b) above
	//
	// Gel is destroyed only on tiles connected on all four sides.

	//
	//Checking condition A.
	//

	//Adjacent connected tiles.
	CCoordSet tiles;
	GetTarConnectedComponent(wX, wY, tiles, true);

	//For each orthogonal neighbor of the stabbed square, check for a square
	//which does not contain tar.
	UINT empty = NO_ORIENTATION;
	UINT i;
	for (i = 0; i < 4; ++i)
	{
		const UINT tx = wX + nGetOX(orthogonal[i]);
		const UINT ty = wY + nGetOY(orthogonal[i]);
		if (!IsValidColRow(tx, ty) || GetTSquare(tx, ty) != wTSquare ||
				!tiles.has(tx, ty)) //must be connected to this tarstuff's connected component
		{
			empty = orthogonal[i];
			break;
		}
	}

	//If no non-tar square is orthogonally adjacent to stab, the stab does not
	//destroy tar.  Only enclosed connected gel squares can be destroyed.
	if (empty == NO_ORIENTATION)
	{
		if (wTSquare != T_GEL)
			return false;

		//To be an inside corner, at least one of the diagonal tiles must be
		//empty, and two of the diagonal tiles must be opposite each other
		const bool bGelNW = tiles.has(wX-1,wY-1) && GetTSquare(wX-1, wY-1) == T_GEL;
		const bool bGelNE = tiles.has(wX+1,wY-1) && GetTSquare(wX+1, wY-1) == T_GEL;
		const bool bGelSW = tiles.has(wX-1,wY+1) && GetTSquare(wX-1, wY+1) == T_GEL;
		const bool bGelSE = tiles.has(wX+1,wY+1) && GetTSquare(wX+1, wY+1) == T_GEL;

		//If all full, then the gel is completely enclosed and not cuttable
		if (bGelNW && bGelNE && bGelSW && bGelSE)
			return false;

		if ((bGelNW && bGelSE) || (bGelSW && bGelNE))
			return true;

		ASSERT(!"Encountered isolated Gel tile connected to stabbed inside corner");
		return false;
	}

	//
	//Checking condition B.
	//

	//For each of the six squares on the other side of the empty space, check
	//that it contains tar.
	const int dx = nGetOX(empty);
	const int dy = nGetOY(empty);
	for (i = 0; i <= 1; ++i)
		for (int j = -1; j <= 1; ++j)
		{
			const UINT tx = wX + dy * j - dx * i;
			const UINT ty = wY + dx * j - dy * i;
			if (!IsValidColRow(tx, ty) || GetTSquare(tx, ty) != wTSquare)
				return wTSquare == T_MUD;
		}

	return wTSquare == T_TAR;
}

//*****************************************************************************
void CDbRoom::RemoveStabbedTar(
//Removes tar found in a square in response to the tar being stabbed.  Adjacent
//tar squares may change into tar babies as a result.  Does not check that stab
//is directed at a vulnerable tar square.
//
//Params:
	const UINT wX, const UINT wY, //(in)  Square containing tar to remove.
	CCueEvents &CueEvents,        //(out) May receive cue events.
	const bool bKillBaby)         //(in) [default=true] Whether baby on stabbed square should be killed
{
	const UINT wTarType = GetTSquare(wX, wY);
	ASSERT(bIsTarOrFluff(wTarType));

	//Easy part--remove the tar.
	if (bKillBaby)
	{
		DestroyTar(wX, wY, CueEvents);
	} else {
		//If we don't kill the baby, then instead mark this square for baby conversion
		//Note that the baby may still not end up being spawned (tar over water for example)
		if (!NewBabies.IsMember(wX,wY))
			NewBabies.Push(wX,wY);
	}

	//Tar baby formation rules: If a square contains tar, but does not
	//have three adjacent neighbors at n/nw/w, n/ne/e, s/sw/s, or s/se/e
	//tar is removed from that square (T_TAR -> T_EMPTY) and a tar baby
	//monster is created in that square (if no sword is there).

recompute:
	//For each of the eight squares adjacent to the stabbed square, check
	//for a tar baby formation in that square.
	for (UINT j = wY - 1; j != wY + 2; ++j)
	{
		for (UINT i = wX - 1; i != wX + 2; ++i)
		{
			if (!IsValidColRow(i,j) ||
					//Skip over the square that got stabbed or...
					(i == wX && j == wY) ||

					//...a square that doesn't contain tar or...
					(GetTSquare(i, j) != wTarType) ||

					//...tar that's marked for baby conversion...
					NewBabies.IsMember(i, j))
				continue;

			//Get the orthogonal squares once for speed.
			const bool bIsNorthTar = (j > 0) ? (GetTSquare(i, j - 1)==wTarType) && !NewBabies.IsMember(i,j-1) : false;
			const bool bIsSouthTar = (j < this->wRoomRows - 1) ? (GetTSquare(i, j + 1)==wTarType) && !NewBabies.IsMember(i,j+1) : false;
			const bool bIsWestTar = (i > 0) ? (GetTSquare(i - 1, j)==wTarType) && !NewBabies.IsMember(i-1,j) : false;
			const bool bIsEastTar = (i < this->wRoomCols - 1) ? (GetTSquare(i + 1, j)==wTarType) && !NewBabies.IsMember(i+1,j) : false;

			//Check the four corners.
			if (
					//Check northwest corner.
					(i > 0 && j > 0 &&
					GetTSquare(i - 1, j - 1)==wTarType &&
					!NewBabies.IsMember(i-1,j-1) &&
					bIsNorthTar && bIsWestTar) ||

					//Check northeast corner.
					(i < this->wRoomCols - 1 && j > 0 &&
					GetTSquare(i + 1, j - 1)==wTarType &&
					!NewBabies.IsMember(i+1,j-1) &&
					bIsNorthTar && bIsEastTar) ||

					//Check southwest corner.
					(i > 0 && j < this->wRoomRows - 1 &&
					GetTSquare(i - 1, j + 1)==wTarType &&
					!NewBabies.IsMember(i-1,j+1) &&
					bIsSouthTar && bIsWestTar) ||

					//Check southeast corner.
					(i < this->wRoomCols - 1 && j < this->wRoomRows - 1 &&
					GetTSquare(i + 1, j + 1)==wTarType &&
					!NewBabies.IsMember(i+1,j+1) &&
					bIsSouthTar && bIsEastTar)
				)
				//Stable tar--skip to next square.
				continue;

			//If there is not a tarstuff mother here, tarstuff breaks.
			CMonster *pMonster = GetMonsterAtSquare(i,j);
			if (!pMonster || wTarType == T_FLUFF || !bIsMother(pMonster->wType))
			{
				//Mark tar for baby conversion
				if (!NewBabies.IsMember(i, j))
					NewBabies.Push(i, j);

				//This tar breaking may invalidate a previous decision,
				//so recompute them.
				goto recompute;
			}
		} //...for i
	} //...for j
}

//*****************************************************************************
void CDbRoom::ConvertUnstableTar(
//Transforms marked unstable tar in babyOrder into new babies.
//
//Params:
	CCueEvents &CueEvents,        //(out) May receive cue events.
	const bool bDelayBabyMoves)   //(in)  Whether babies can move immediately when processed [default=false]
{
	bool bShallowPathmapNeeded = false;
	const UINT wSX = this->pCurrentGame ? this->pCurrentGame->swordsman.wX : (UINT)-1;
	const UINT wSY = this->pCurrentGame ? this->pCurrentGame->swordsman.wY : (UINT)-1;
	CCoordIndex swordCoords;
	GetSwordCoords(swordCoords);

	UINT wX, wY;
	while (NewBabies.PopBottom(wX,wY)) //process as queue
	{
		const UINT wTarType = GetTSquare(wX,wY);
		const bool bFluff = wTarType == T_FLUFF;
		if (bIsTarOrFluff(wTarType))
		{
			//Get rid of the unstable tar
			DestroyTar(wX, wY, CueEvents);

			CMonster *pMonster = GetMonsterAtSquare(wX,wY);

			//Don't spawn tar babies under living monsters
			if (!pMonster)
			{
				//Spawn a tarbaby.
				const UINT wOSquare = GetOSquare(wX,wY);

				if (bFluff)
				{
					if (!(bIsStairs(wOSquare) || (wX == wSX && wY == wSY)))
					{
						//Spawn a puff
						CMonster *m = NULL;
						m = AddNewMonster(M_FLUFFBABY,wX,wY);
						//FIXME-Fluff: Need CID_FluffBabyFormed
						if (this->pCurrentGame)
							m->SetCurrentGame(this->pCurrentGame);
						//Fluff Babies never move on the same turn they're converted
						m->bIsFirstTurn = true;
						m->SetOrientation(sgn(wSX - wX), sgn(wSY - wY));
					}
				}
				//Don't create babies on swords, stairs, pits, water or the player.
				else if (!(swordCoords.Exists(wX,wY) || bIsStairs(wOSquare) || bIsPit(wOSquare) ||
						bIsDeepWater(wOSquare) || (bIsShallowWater(wOSquare) && wTarType != T_GEL) ||
						(wX == wSX && wY == wSY)))
				{
					//Active spike traps prevent babies being formed, but unlike other tiles, we should
					//cause an additional graphical explosion effect since the spike is only visible for a brief time
					if (wOSquare == T_FLOOR_SPIKES && GetFloorSpikesState() == FloorSpikesUp)
						CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, wTarType), true);
					else
					{
						bShallowPathmapNeeded = true; //Tarstuff Babies may require Shallow Water pathmap
						//Spawn a tarbaby.
						CMonster *m = NULL;
						switch (wTarType)
						{
							case T_TAR:
								m = AddNewMonster(M_TARBABY,wX,wY);
								CueEvents.Add(CID_TarBabyFormed, m);
								break;
							case T_MUD:
								m = AddNewMonster(M_MUDBABY,wX,wY);
								CueEvents.Add(CID_MudBabyFormed, m);
								break;
							case T_GEL:
								m = AddNewMonster(M_GELBABY,wX,wY);
								CueEvents.Add(CID_GelBabyFormed, m);
								break;
							default: ASSERT(!"Bad tar type"); continue;
						}
						if (this->pCurrentGame)
							m->SetCurrentGame(this->pCurrentGame);
						m->bIsFirstTurn = bDelayBabyMoves;
						m->SetOrientation(sgn(wSX - wX), sgn(wSY - wY));
					}
				}
			}
		}
	}

	if (bShallowPathmapNeeded && this->pCurrentGame && !this->pPathMap[GROUND_AND_SHALLOW_WATER])
		CreatePathMaps();

	NewBabies.Clear();
}

//*****************************************************************************
int CDbRoom::SwapTarstuffRoles(const UINT type, const bool bTar, const bool bMud, const bool bGel)
//Returns: monster type of swapped tarstuff monster role, or -1 if type isn't a tarstuff monster
{
	int nType = -1;
	switch (type)
	{
		case M_TARBABY:
			if (bTar) nType = bMud ? M_MUDBABY : M_GELBABY;
		break;
		case M_MUDBABY:
			if (bMud) nType = bTar ? M_TARBABY : M_GELBABY;
		break;
		case M_GELBABY:
			if (bGel) nType = bMud ? M_MUDBABY : M_TARBABY;
		break;
		case M_TARMOTHER:
			if (bTar) nType = bMud ? M_MUDMOTHER : M_GELMOTHER;
		break;
		case M_MUDMOTHER:
			if (bMud) nType = bTar ? M_TARMOTHER : M_GELMOTHER;
		break;
		case M_GELMOTHER:
			if (bGel) nType = bMud ? M_MUDMOTHER : M_TARMOTHER;
		break;
		default: break;
	}
	return nType;
}

//*****************************************************************************
void CDbRoom::SwitchTarstuff(const UINT wType1, const UINT wType2, CCueEvents& CueEvents)
//Switch these two tarstuff types in the room, including monster types.
{
	const bool bTar = wType1 == T_TAR || wType2 == T_TAR;
	const bool bMud = wType1 == T_MUD || wType2 == T_MUD;
	const bool bGel = wType1 == T_GEL || wType2 == T_GEL;

	//Keep tarstuff type growth events synched.
	const CUEEVENT_ID eGrowth1 = bTar ? CID_TarGrew : CID_MudGrew;
	const CUEEVENT_ID eGrowth2 = bGel ? CID_GelGrew : CID_MudGrew;

	UINT wX, wY;
	CCueEvents Ignored;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			//Switch tarstuff.
			const UINT wTTile = GetTSquare(wX,wY);
			if (wTTile == wType1)
				Plot(wX,wY, wType2);
			else if (wTTile == wType2)
				Plot(wX,wY, wType1);

			//Swap monster types.
			CMonster *pMonster = GetMonsterAtSquare(wX,wY);
			if (pMonster)
			{

				if (pMonster->wType == M_TEMPORALCLONE) {
					CTemporalClone* pTemporalClone = DYN_CAST(CTemporalClone*, CMonster*, pMonster);
					const int nType = SwapTarstuffRoles(pTemporalClone->wAppearance, bTar, bMud, bGel);
					if (nType != -1)
					{
						//Switched Gel Babies can't swim.  Kill them instead.
						if (pTemporalClone->wAppearance == M_GELBABY && bIsShallowWater(GetOSquare(wX, wY)))
						{
							CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
							KillMonster(pMonster, CueEvents);
						}

						pTemporalClone->wIdentity = pTemporalClone->wAppearance = nType;

					}

					continue;
				}

				const int nType = SwapTarstuffRoles(pMonster->wType, bTar, bMud, bGel);
				if (nType != -1)
				{
					//Switched Gel Babies can't swim.  Kill them instead.
					if (pMonster->wType == M_GELBABY && bIsShallowWater(GetOSquare(wX,wY)))
					{
						CueEvents.Add(CID_Splash, new CCoord(wX,wY), true);
						KillMonster(pMonster, CueEvents);
						this->pCurrentGame->TallyKill();
					} else {
						//Create new monster and insert in list where old monster was.
						CMonster *pNew = CMonsterFactory::GetNewMonster((const MONSTERTYPE)nType);
						pNew->SetCurrentGame(this->pCurrentGame);
						pNew->wX = pNew->wPrevX = wX;
						pNew->wY = pNew->wPrevY = wY;
						pNew->wO = pNew->wPrevO = pMonster->wO;
						pNew->bNewStun = pMonster->bNewStun;
						pNew->bPushedThisTurn = pMonster->bPushedThisTurn;
						pNew->stunned = pMonster->stunned;
						++this->wMonsterCount;

						pNew->pNext = pMonster->pNext;
						pNew->pPrevious = pMonster->pPrevious;
						if (pNew->pNext)
							pNew->pNext->pPrevious = pNew;
						if (pNew->pPrevious)
							pNew->pPrevious->pNext = pNew;
						if (pMonster == this->pLastMonster)
							this->pLastMonster = pNew;
						if (pMonster == this->pFirstMonster)
							this->pFirstMonster = pNew;

						//Need to re-fire a growth command from this new mother object.
						if (bIsMother(pMonster->wType))
						{
							if (CueEvents.Remove(eGrowth1,pMonster))
								CueEvents.Add(eGrowth2,pNew);
							if (CueEvents.Remove(eGrowth2,pMonster))
								CueEvents.Add(eGrowth1,pNew);
						}

						pMonster->pNext = pMonster->pPrevious = NULL;
						KillMonster(pMonster, Ignored);

						SetMonsterSquare(pNew);
					}
				}
			}
		}

	//Remove growth event types completely that are no longer occurring.
	if (CueEvents.HasOccurred(eGrowth1) && CueEvents.GetOccurrenceCount(eGrowth1) == 0)
		CueEvents.ClearEvent(eGrowth1);
	if (CueEvents.HasOccurred(eGrowth2) && CueEvents.GetOccurrenceCount(eGrowth2) == 0)
		CueEvents.ClearEvent(eGrowth2);

	//If player is in a tar role, switch player role.
	const int nSwappedPlayerType = SwapTarstuffRoles(this->pCurrentGame->swordsman.wAppearance, bTar, bMud, bGel);
	if (nSwappedPlayerType >= 0)
		this->pCurrentGame->SetPlayerRole(nSwappedPlayerType, CueEvents);
}

//*****************************************************************************
void CDbRoom::GetDoubleSwordCoords(
//Gets a coord index containing coords of all doubles' swords.
//If swords are out of the room boundaries, don't add them.
//
//For efficiency, use this method when you need to check several squares for a
//double's sword.  Use DoesSquareContainDoubleSword() when you have only need to
//check one or two.
//
//Params:
	CCoordIndex &DoubleSwordCoords,  //(out) Uninitialized.
	const bool bIgnoreDagger,  //[default=false] whether to include daggers as obstacles
	const bool bIgnoreNonCuts, //[default=false] whether to include weapons that don't cut when stationary
	CMonster *pIgnore) //[default=NULL] optional monster to ignore in tally
const
{
	UINT wSX, wSY;
	DoubleSwordCoords.Init(this->wRoomCols, this->wRoomRows);
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster != pIgnore && pMonster->GetSwordCoords(wSX, wSY))
			if (!(bIgnoreDagger && pMonster->GetWeaponType() == WT_Dagger) &&
				!(bIgnoreNonCuts && !bWeaponCutsWhenStationary(pMonster->GetWeaponType())) &&
					IsValidColRow(wSX, wSY))
				DoubleSwordCoords.Add(wSX, wSY, 1+pMonster->wO); //avoid 0 values
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
inline bool weaponMakesSwordSound(WeaponType wt) {
	switch (wt) {
		case WT_Sword:
		case WT_Pickaxe:
		case WT_Dagger:
			return true;
		default:
			return false;
	}
}

WeaponType CDbRoom::SwordfightCheck() const
//Returns: true if two or more swords occupy the same square and at least one
//of them just moved to this square, else false.
{
	ASSERT(this->pCurrentGame);

	CCoordSet weaponMoved; //weapon just moved to this square
	map<UINT, WeaponType> weaponTypeAtTile;

	UINT wSX, wSY;
	CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.HasWeapon())
	{
		weaponTypeAtTile.insert(make_pair(ARRAYINDEX(player.wSwordX, player.wSwordY), player.GetActiveWeapon()));

		//Check whether player is moving sword.
		if (player.wSwordMovement != NO_ORIENTATION)
			weaponMoved.insert(player.wSwordX, player.wSwordY);
	}

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->GetSwordCoords(wSX, wSY))
		{
			const UINT tileIndex = ARRAYINDEX(wSX, wSY);
			const WeaponType wt = pMonster->GetWeaponType();

			CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
			if (pArmedMonster->wSwordMovement != NO_ORIENTATION)
				weaponMoved.insert(wSX, wSY);
			if (weaponTypeAtTile.count(tileIndex) && weaponMoved.has(wSX, wSY)) {
				//Weapons clash.  Determine sound effect.
				if (weaponMakesSwordSound(wt)) {
					map<UINT, WeaponType>::const_iterator it = weaponTypeAtTile.find(tileIndex);
					ASSERT(it != weaponTypeAtTile.end());
					if (weaponMakesSwordSound(it->second))
						return WT_Sword;
				}
				return WT_Staff;
			}
			weaponTypeAtTile.insert(make_pair(tileIndex, wt));
		}
		pMonster = pMonster->pNext;
	}
	return WT_Off;
}

//*****************************************************************************
void CDbRoom::GetCaberCoords(
//Gets a coord index containing coords of all cabers in the room
//If swords are out of the room boundaries, don't add them.
//
//Each index is a DMASK that indicates in which direction
//the caber is being held from.
//
//Params:
	CCoordIndex &CaberCoords)  //(out) Uninitialized.
const
{
	UINT wSX, wSY;
	CaberCoords.Init(this->wRoomCols, this->wRoomRows);
	static const UINT rdirmask[] = {DMASK_SE, DMASK_S, DMASK_SW, DMASK_E, 0, DMASK_W, DMASK_NE, DMASK_N, DMASK_NW};

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->GetWeaponType() == WT_Caber && pMonster->GetSwordCoords(wSX,wSY) && IsValidColRow(wSX,wSY))
		{
			ASSERT(pMonster->wO != NO_ORIENTATION && pMonster->wO < ORIENTATION_COUNT);
			const UINT wMask = rdirmask[pMonster->wO];
			CaberCoords.Add(wSX, wSY, wMask | CaberCoords.GetAt(wSX, wSY));
		}
		pMonster = pMonster->pNext;
	}
	if (this->pCurrentGame && this->pCurrentGame->swordsman.GetActiveWeapon() == WT_Caber && this->pCurrentGame->swordsman.HasWeapon() &&
			IsValidColRow(this->pCurrentGame->swordsman.wSwordX, this->pCurrentGame->swordsman.wSwordY))
	{
		ASSERT(this->pCurrentGame->swordsman.wO != NO_ORIENTATION && this->pCurrentGame->swordsman.wO < ORIENTATION_COUNT);
		const UINT wMask = rdirmask[this->pCurrentGame->swordsman.wO];
		wSX = this->pCurrentGame->swordsman.wSwordX;
		wSY = this->pCurrentGame->swordsman.wSwordY;
		CaberCoords.Add(wSX, wSY, wMask | CaberCoords.GetAt(wSX, wSY));
	}
}

//*****************************************************************************
void CDbRoom::GetSwordCoords(
//Gets a coord index containing coords of all swords (player & doubles).
//If swords are out of the room boundaries, don't add them.
//
//Params:
	CCoordIndex &SwordCoords,  //(out) Uninitialized.
	const bool bIgnoreDagger,  //[default=false] whether to include daggers as obstacles
	const bool bIgnoreNonCuts, //[default=false] whether to include weapons that don't cut when stationary
	CMonster *pIgnore) //[default=NULL] optional monster to ignore in tally
const
{
	GetDoubleSwordCoords(SwordCoords, bIgnoreDagger, bIgnoreNonCuts, pIgnore);
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (this->pCurrentGame && player.HasWeapon() && 
		!(bIgnoreDagger && player.GetActiveWeapon() == WT_Dagger) &&
		!(bIgnoreNonCuts && !bWeaponCutsWhenStationary(player.GetActiveWeapon())) &&
		IsValidColRow(player.wSwordX, player.wSwordY))
	{
		SwordCoords.Add(player.wSwordX, player.wSwordY, 1+player.wO); //avoid 0 values
	}
}

//*****************************************************************************
bool CDbRoom::IsMonsterSwordAt(
//Determines if a square contains a monster/double's sword.
//
//For efficiency, use GetDoubleSwordCoords() when you need to check several
//squares for a double's sword.  Use this method when you only need to check
//one or two.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Square to check.
	const bool bIgnoreDagger, //[default=false]
	const CMonster *pIgnore) //[default=NULL] optional monster to ignore in search
//
//Returns:
//True if it does, false if not.
const
{
	//For a monster to have a sword in this square, the monster must be adjacent.
	for (int nJ=-1; nJ<=1; ++nJ) //O(9) search
		for (int nI=-1; nI<=1; ++nI)
			if (nI || nJ)
			{
				CMonster *pMonster = GetMonsterAtSquare(wX+nI, wY+nJ);
				//Monsters can walk into their own sword square.
				if (pMonster && pMonster != pIgnore && pMonster->HasSwordAt(wX, wY)) {
					if (bIgnoreDagger && pMonster->GetWeaponType() == WT_Dagger)
						continue;
					return true;
				}
			}

	return false;
}

//*****************************************************************************
bool CDbRoom::IsMonsterWeaponTypeAt(
	//Determines if a square contains a monster's weapon, and it is of the
	//given type.
	//
	//
	//Params:
	const UINT wX, const UINT wY, //(in)   Square to check.
	const WeaponType wt, //Type of weapon
	const CMonster *pIgnore) //[default=NULL] optional monster to ignore in search
							 //
							 //Returns:
							 //True if it does, false if not.
	const
{
	//For a monster to have a sword in this square, the monster must be adjacent.
	for (int nJ = -1; nJ <= 1; ++nJ) //O(9) search
		for (int nI = -1; nI <= 1; ++nI)
			if (nI || nJ)
			{
				CMonster *pMonster = GetMonsterAtSquare(wX + nI, wY + nJ);
				//Monsters can walk into their own sword square.
				if (pMonster && pMonster != pIgnore && pMonster->HasSwordAt(wX, wY)) {
					if (pMonster->GetWeaponType() == wt)
						return true;
				}
			}

	return false;
}

//*****************************************************************************
bool bIsArrowObstacle(
//Determines whether a specified arrow tile would be an obstacle when
//approaching it from a specified angle, and whether the tile could
//be left if currently standing on it
//
//Accepts:
  const UINT nArrowTile,
  const UINT nO) //Direction of approach
//
//Returns:
//true/false.
{
	switch (nArrowTile)
	{
	case T_ARROW_N:
		return (nO==S || nO==SW || nO==SE);
	case T_ARROW_S:
		return (nO==N || nO==NW || nO==NE);
	case T_ARROW_W:
		return (nO==E || nO==SE || nO==NE);
	case T_ARROW_E:
		return (nO==W || nO==SW || nO==NW);
	case T_ARROW_NW:
		return (nO==S || nO==E || nO==SE);
	case T_ARROW_SW:
		return (nO==N || nO==E || nO==NE);
	case T_ARROW_NE:
		return (nO==S || nO==W || nO==SW);
	case T_ARROW_SE:
		return (nO==N || nO==W || nO==NW);
	default:
		return false;
	}
}

//*****************************************************************************
void CDbRoom::ActivateBeacon(
//Toggles the state of a specific seeding beacon
//
//Accepts:
	const UINT wX, const UINT wY,    //(in) Orb location.
	CCueEvents &CueEvents)     //(in/out) Appends cue events to list.
{
	UINT wOldTile = GetTSquare(wX,wY);
	UINT wNewTile = T_BEACON;
	ASSERT(bIsBeacon(wOldTile));
	if (bIsBeacon(wOldTile))
	{
		if (wOldTile == T_BEACON) {
			wNewTile = T_BEACON_OFF;
			CueEvents.Add(CID_SeedingBeaconDeactivated);
		} else {
			CueEvents.Add(CID_SeedingBeaconActivated);
		}
		Plot(wX, wY, wNewTile);

		SetRoomLightingChanged();
	}
}

//*****************************************************************************
bool CDbRoom::IsBeaconActive() const
//Returns: whether a beacon is still active in the room
{
	for (CCoordSet::const_iterator beacon=this->Beacons.begin();
			beacon!=this->Beacons.end(); ++beacon)
	{
		const UINT wX = beacon->wX;
		const UINT wY = beacon->wY;
		const UINT wTileNo = GetTSquare(wX,wY);
		ASSERT(bIsBeacon(wTileNo));
		if (wTileNo==T_BEACON)
			return true;
	}
	return false;
}

//*****************************************************************************
void CDbRoom::DeactivateBeacons()
//Turns off all beacons in the room
{
	for (CCoordSet::const_iterator beacon=this->Beacons.begin();
			beacon!=this->Beacons.end(); ++beacon)
	{
		const UINT wX = beacon->wX;
		const UINT wY = beacon->wY;
		const UINT wTileNo = GetTSquare(wX,wY);
		ASSERT(bIsBeacon(wTileNo));
		if (wTileNo==T_BEACON)
			Plot(wX, wY, T_BEACON_OFF);
	}
}

//*****************************************************************************
void CDbRoom::ActivateOrb(
//Activates an orb by releasing any associated agents into the current room
//to perform tasks.
//
//Accepts:
	const UINT wX, const UINT wY,    //(in) Orb location.
	CCueEvents &CueEvents,     //(in/out) Appends cue events to list.
	const OrbActivationType eActivationType,     //(in) what activated the orb
	const bool bBreakOrb)      //(in) whether cracked/broken orbs will break
{
	//Get the orb and its agent instructions.
	COrbData *pOrb;
	const bool bPressurePlate =
			eActivationType == OAT_PressurePlate ||
			eActivationType == OAT_PressurePlateUp ||
			eActivationType == OAT_ScriptPlate;
	if (!bPressurePlate)
	{
		//Orb.
		pOrb = GetOrbAtCoords(wX,wY);
	} else {
		//Pressure plate.
		pOrb = GetPressurePlateAtCoords(wX,wY);
		if (!pOrb)
			return;
		switch (pOrb->eType)
		{
			case OT_BROKEN: return; //doesn't do anything
			case OT_NORMAL:
			case OT_ONEUSE:
			case OT_TOGGLE:
				if (eActivationType == OAT_PressurePlate)
				{
					if (pOrb->bActive)
						return;
					pOrb->bActive = true;
				} else if (eActivationType == OAT_ScriptPlate) {
					//When activating a plate via script, it doesn't matter whether
					//the plate is currently up or down, and plate activation also
					//doesn't change whether the plate moves up or down.
				} else {
					ASSERT(eActivationType == OAT_PressurePlateUp); //only other type
					ASSERT(pOrb->bActive); //should only be called on an actual release
					pOrb->bActive = false;
				}
			break;
			case OT_COUNT: break;
		}
		//Ensure these tiles are redrawn as plate changes state.
		for (CCoordSet::const_iterator tile=pOrb->tiles.begin();
				tile!=pOrb->tiles.end(); ++tile)
			this->PlotsMade.insert(tile->wX,tile->wY);
	}

	if (pOrb && pOrb->eType == OT_BROKEN)
	{
		ASSERT(!bPressurePlate);
		if (bBreakOrb)
		{
			//Broken orb shatters when activated
			Plot(wX, wY, T_EMPTY);
			CueEvents.Add(CID_CrumblyWallDestroyed, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
		} else {
			CueEvents.Add(CID_OrbDamaged, pOrb);
		}
		return;
	}

	//Add cue event even if there's no info for this orb in the DB.
	const bool bAttached = pOrb == NULL;
	if (!pOrb)
		pOrb = new COrbData(wX,wY);
	switch (eActivationType)
	{
		case OAT_Player:
			CueEvents.Add(CID_OrbActivatedByPlayer, pOrb, bAttached);
			break;
		case OAT_Monster:
		case OAT_ScriptOrb:
			CueEvents.Add(CID_OrbActivatedByDouble, pOrb, bAttached);
			break;
		case OAT_Item:
			CueEvents.Add(CID_OrbActivated, pOrb, bAttached);
			break;
		case OAT_PressurePlate:
		case OAT_ScriptPlate:
			CueEvents.Add(CID_PressurePlate, pOrb, bAttached);
			break;
		case OAT_PressurePlateUp:
			//Only the toggle type activates doors on panel release.
			if (pOrb->eType != OT_TOGGLE)
			{
				CueEvents.Add(CID_PressurePlateReleased, NULL);
				return;
			}
			CueEvents.Add(CID_PressurePlateReleased, pOrb, bAttached);
			break;
		default:
			ASSERT(!"Unrecognized orb activation type");
			break;
	}

	if (bAttached) return;  //No orb information -- nothing to do.

	//For each agent in orb...
	const UINT wNumAgents = pOrb->agents.size();
	for (UINT wIndex=0; wIndex<wNumAgents; ++wIndex)
	{
		COrbAgentData *pAgent = pOrb->agents[wIndex];
		ASSERT(pAgent);
		const UINT oTile = GetOSquare(pAgent->wX, pAgent->wY);
		const UINT wTTile = GetTSquare(pAgent->wX, pAgent->wY);
		const UINT fTile = GetFSquare(pAgent->wX, pAgent->wY);
		switch (pAgent->action)
		{
			case OA_TOGGLE:
				if (bIsLight(wTTile))
					ToggleLight(pAgent->wX, pAgent->wY);
				else if (bIsYellowDoor(oTile))
					ToggleYellowDoor(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsFiretrap(oTile))
					ToggleFiretrap(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsAnyArrow(fTile))
					ToggleForceArrow(pAgent->wX, pAgent->wY);
			break;

			case OA_OPEN:
				if (bIsLight(wTTile))
					TurnOnLight(pAgent->wX, pAgent->wY);
				else if (bIsYellowDoor(oTile))
					OpenYellowDoor(pAgent->wX, pAgent->wY);
				else if (bIsFiretrap(oTile))
					EnableFiretrap(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsAnyArrow(fTile))
					DisableForceArrow(pAgent->wX, pAgent->wY);
			break;

			case OA_CLOSE:
				if (bIsLight(wTTile))
					TurnOffLight(pAgent->wX, pAgent->wY);
				else if (bIsYellowDoor(oTile))
					CloseYellowDoor(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsFiretrap(oTile))
					DisableFiretrap(pAgent->wX, pAgent->wY);
				else if (bIsAnyArrow(fTile))
					EnableForceArrow(pAgent->wX, pAgent->wY);
			break;

			default:
				ASSERT(!"Bad orb agent.");
			break;
		}
	}

	if ((bBreakOrb || bPressurePlate) && pOrb->eType == OT_ONEUSE) //one-use type...
	{
		pOrb->eType = OT_BROKEN;   //...no longer works now
		if (!bPressurePlate)
			CueEvents.Add(CID_OrbDamaged, pOrb);
		this->PlotsMade.insert(wX,wY);
	}
}

//*****************************************************************************
void CDbRoom::ForceTileRedraw(
// Forces a redraw of a tile at given coordinates
	const UINT wX, const UINT wY, const bool bGeometryChanges)
{
	if (bGeometryChanges)
		this->geometryChanges.insert(wX, wY);

	this->PlotsMade.insert(wX, wY);
}

//*****************************************************************************
void CDbRoom::FloodPlot(
//Flood fills all squares from a starting square with a new tile.  Only adjacent
//squares from the starting square with the same tile will be flooded.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Starting square coords.
	const UINT wTileNo,     //(in)   Tile to flood with.
	const bool b8Neighbor)  //(in)   Diagonal flood if true, 4-neighbor if false
{
	ASSERT(IsValidColRow(wX, wY));
	ASSERT(IsValidTileNo(wTileNo));

	//Flood squares of tile type at (wX,wY).
	const UINT wLayer = TILE_LAYER[wTileNo];
	CTileMask mask(wLayer == 0 ? GetOSquare(wX, wY) : GetTSquare(wX,wY));

	//Gather list of squares to flood fill.
	CCoordSet squares;
	GetConnectedTiles(wX, wY, mask, b8Neighbor ? Connect_8 : Connect_4, squares);

	//Plot these squares.
	for (CCoordSet::const_iterator iter = squares.begin(); iter != squares.end(); ++iter)
		Plot(iter->wX, iter->wY, wTileNo);
}

//*****************************************************************************
void CDbRoom::DecTrapdoor(CCueEvents &CueEvents)
//Remove trapdoor during play.
{
	ASSERT(this->wTrapDoorsLeft); //This function should never be called with 0 Trap Doors
	--this->wTrapDoorsLeft;
	if (this->wTrapDoorsLeft == 0)
	{
		CueEvents.Add(CID_AllTrapdoorsRemoved);
		if (ToggleTiles(T_DOOR_R,T_DOOR_RO,CueEvents)) //red doors
		{
			CueEvents.Add(CID_RedGatesToggled);
			ConvertUnstableTar(CueEvents);
		}
	}
}

//*****************************************************************************
void CDbRoom::IncTrapdoor(CCueEvents& CueEvents)
//Updates room state when a new trapdoor is added to the room during play.
{
	if (!this->wTrapDoorsLeft)
	{
		if (ToggleTiles(T_DOOR_R,T_DOOR_RO,CueEvents)) //red doors
		{
			CueEvents.Add(CID_RedGatesToggled);
			ConvertUnstableTar(CueEvents);
		}
	}
	++this->wTrapDoorsLeft;
}

//*****************************************************************************
void CDbRoom::DestroyCrumblyWall(
//Plots a floor to a crumbly wall square.
//
//Params:
	const UINT wX, const UINT wY, //(in) Crumbly wall square.
	CCueEvents &CueEvents,        //(in/out)
	const UINT wStabO)				//(in) direction hit from [default=NO_ORIENTATION]
{
	ASSERT(bIsCrumblyWall(GetOSquare(wX,wY)));
	ASSERT(bIsPlainFloor(this->coveredOSquares.GetAt(wX, wY)));
	Plot(wX, wY, this->coveredOSquares.GetAt(wX, wY));
	CueEvents.Add(CID_CrumblyWallDestroyed, new CMoveCoord(wX, wY,	wStabO), true);
}

//*****************************************************************************
void CDbRoom::DestroyFluff(
//Destroy fluff and fluff puffs due to a built wall/door.
//
//Params:
	const UINT wX, const UINT wY, //(in)
	CCueEvents &CueEvents)     //(in/out)
{
	const UINT wOSquare = GetOSquare(wX, wY);
	ASSERT(bIsSolidOTile(wOSquare) || wOSquare == T_HOT);

	if (GetTSquare(wX,wY) == T_FLUFF)
	{
		RemoveStabbedTar(wX, wY, CueEvents, true);
		CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, T_FLUFF), true);
	}
	
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster && pMonster->wType == M_FLUFFBABY)
	{
		KillMonster(pMonster,CueEvents);
		ProcessPuffAttack(CueEvents, wX, wY);
	}
}

//*****************************************************************************
void CDbRoom::DestroyTar(
//Remove tar/fluff at square.
//
//Params:
	const UINT wX, const UINT wY, //(in)
	CCueEvents &CueEvents)     //(in/out)
{
	UINT wTarType = GetTSquare(wX,wY);
	ASSERT(bIsTarOrFluff(wTarType));
	Plot(wX, wY, T_EMPTY);
	if (wTarType != T_FLUFF)
	{
		ASSERT(this->wTarLeft); //This function should never be called with 0 Tar squares
		--this->wTarLeft;
		if (!this->wTarLeft && !this->bTarstuffGateTogglePending)
		{
			CueEvents.Add(CID_AllTarRemoved);
			ToggleBlackGates(CueEvents);
			ConvertUnstableTar(CueEvents);
		}
	}
}

//******************************************************************************
void CDbRoom::DestroyTrapdoor(
//Plots a pit or water to a trapdoor square.
//Check for objects on the tile to fall.
//Updates red doors as needed.
//
//Params:
	const UINT wX, const UINT wY,    //(in)
	CCueEvents &CueEvents)     //(in/out)
{
	const UINT oldOTile = GetOSquare(wX,wY);
	const UINT newOTile = this->coveredOSquares.GetAt(wX, wY);
	ASSERT(bIsFallingTile(oldOTile));
	ASSERT(bIsPit(newOTile) || bIsWater(newOTile));
	Plot(wX, wY, newOTile);
	CheckForFallingAt(wX, wY, CueEvents, true);
	ConvertUnstableTar(CueEvents);
	if (bIsTrapdoor(oldOTile))
	{
		CueEvents.Add(CID_TrapDoorRemoved, new CCoord(wX, wY), true);
		DecTrapdoor(CueEvents);
	} else {
		ASSERT(bIsThinIce(oldOTile));
		CueEvents.Add(CID_ThinIceMelted, new CCoord(wX, wY), true);
	}
}

//*****************************************************************************
void CDbRoom::RemoveFinishedCharacters()
//Retain each character monster only if its script hasn't yet been run to
//completion in the current game.
{
	ASSERT(this->pCurrentGame);
	CMonster *pMonster = this->pFirstMonster, *pNext;
	CCueEvents Ignored;
	while (pMonster)
	{
		pNext = pMonster->pNext;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (this->pCurrentGame->CompletedScripts.has(pCharacter->dwScriptID))
				KillMonster(pMonster, Ignored);
		}
		pMonster = pNext;
	}
}

//*****************************************************************************
void CDbRoom::RemoveDecoy(CMonster *pMonster)
//Remove decoy/monster from room decoy list.
{
	list<CPlayerDouble*>::const_iterator iSeek;
	for (iSeek = this->Decoys.begin(); iSeek != this->Decoys.end(); ++iSeek)
	{
		if (*iSeek == pMonster)
		{
			//Found it.
			this->Decoys.remove(*iSeek);
			return;
		}
	}
	ASSERT(iSeek != this->Decoys.end());   //indicates decoy wasn't stored in list (bad)
}

//*****************************************************************************
bool CDbRoom::RemoveLongMonsterPieces(
//Removes all the pieces of a long monster (besides the head).
//
//Returns: whether monster was successfully removed
//
//Params:
	CMonster *pMonster)  //(in) Long monster
{
	ASSERT(pMonster->IsLongMonster());

	const bool bIsGentryii = pMonster->wType == M_GENTRYII;
	UINT wLastX = pMonster->wX, wLastY = pMonster->wY;
	CCoordSet updateTiles;
	while (pMonster->Pieces.size() > 0)
	{
		CMonsterPiece *pMPiece = *(pMonster->Pieces.begin());
		if (bIsGentryii)
		{
			if (wLastX != pMPiece->wX && wLastY != pMPiece->wY)
			{
				updateTiles.insert(wLastX,pMPiece->wY);
				updateTiles.insert(pMPiece->wX,wLastY);
			}
			wLastX = pMPiece->wX;
			wLastY = pMPiece->wY;
		}
		Plot(pMPiece->wX, pMPiece->wY, T_NOMONSTER);
	}
	if (bIsGentryii)
		for (CCoordSet::const_iterator tile=updateTiles.begin(); tile!=updateTiles.end(); ++tile)
			UpdatePathMapAt(tile->wX,tile->wY);

	return true;
}

//*****************************************************************************
void CDbRoom::RemoveMonsterEnemy(CMonster *pMonster)
//Remove monster enemy from the room list.
{
	list<CPlayerDouble*>::const_iterator iSeek;
	for (iSeek = this->monsterEnemies.begin(); iSeek != this->monsterEnemies.end(); ++iSeek)
	{
		if (*iSeek == pMonster)
		{
			//Found it.
			this->monsterEnemies.remove(*iSeek);
			return;
		}
	}
	ASSERT(iSeek != this->monsterEnemies.end());   //indicates monster wasn't stored in list (bad)
}

//*****************************************************************************
bool CDbRoom::RemoveTiles(const UINT wOldTile)
//Removes any occurrence of wOldTile in the room with the underlying o-layer tile.
//
//Returns:
//True if any tiles were found
{
	ASSERT(TILE_LAYER[wOldTile] == 0);  //o-layer only implemented currently
	bool bChangedTiles=false;
	for (UINT wY=wRoomRows; wY--; )
		for (UINT wX=wRoomCols; wX--; )
			if (GetOSquare(wX,wY) == wOldTile)
			{
				Plot(wX, wY, this->coveredOSquares.GetAt(wX, wY));
				bChangedTiles=true;
			}
	return bChangedTiles;
}

//*****************************************************************************
void CDbRoom::ToggleBlackGates(CCueEvents& CueEvents)
//When there is no tarstuff in room, call this to toggle closed and open black gates.
//
//NOTES:
//For full 2.0 compatibility, black doors should never re-close once opened.
//However, this only affects a few 2.0 user-made rooms, and shouldn't break
//any of them, so we've changed this semantic slightly for 3.0 for better effect.
{
	const bool bToggled = ToggleTiles(T_DOOR_B,T_DOOR_BO,CueEvents);
	if (bToggled)
		CueEvents.Add(CID_BlackGatesToggled);
}

//*****************************************************************************
bool CDbRoom::ToggleTiles(const UINT wOldTile, const UINT wNewTile, CCueEvents& CueEvents)
//Exchanges any occurrence of wOldTile in the room with wNewTile.
//
//Returns:
//True if any tiles were found
{
	ASSERT(TILE_LAYER[wOldTile] == 0);  //o-layer only implemented currently
	ASSERT(TILE_LAYER[wNewTile] == 0);

	bool bChangedTiles=false, bThisTileChanged=false;
	const bool bSolidOldTile = bIsSolidOTile(wOldTile) || wOldTile == T_HOT;
	const bool bSolidNewTile = bIsSolidOTile(wNewTile) || wNewTile == T_HOT;
	UINT wTile;
	for (UINT wY=wRoomRows; wY--; )
		for (UINT wX=wRoomCols; wX--; )
		{
			bool bDestroyFluff = false;
			wTile = GetOSquare(wX,wY);
			if (wTile == wOldTile)
			{
				Plot(wX, wY, wNewTile);
				bThisTileChanged=bChangedTiles=true;
				bDestroyFluff = !bSolidOldTile && bSolidNewTile;
			} else if (wTile == wNewTile)	{
				Plot(wX, wY, wOldTile);
				bThisTileChanged=bChangedTiles=true;
				bDestroyFluff = bSolidOldTile && !bSolidNewTile;
			}

			//If a door has closed, it cuts fuses.
			if (bThisTileChanged)
			{
				bThisTileChanged = false;
				wTile = GetOSquare(wX, wY);
				if (bIsDoor(wTile))
				{
					switch (GetTSquare(wX, wY))
					{
						case T_FUSE:
							Plot(wX, wY, T_EMPTY);
							RemoveFuse(wX, wY);
						break;
						case T_MIRROR:
						case T_POWDER_KEG:
							if (GetCoveredTSquare(wX,wY) == T_FUSE)
							{
								RemoveCoveredTLayerItem(wX,wY);
								RemoveFuse(wX, wY);
							}
						break;
						default: break;
					}
				}
				if (bDestroyFluff)
					DestroyFluff(wX, wY, CueEvents);
			}
		}

	ConvertUnstableTar(CueEvents);

	return bChangedTiles;
}

//*****************************************************************************
void CDbRoom::Plot(
//Plots a tile value to a square in the room.  All game operations that involve
//changing tiles should use Plot() as opposed to directly modifying square
//data.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Coords for square to plot.
	const UINT wTileNo,     //(in)   New tile.  The tile# will also determine
						//    which layer the tile is plotted to.
	CMonster *pMonster,  //(in) default=NULL
	bool bUnderObject)   //(in) default=false
{
	ASSERT(IsValidTileNo(wTileNo) || wTileNo == T_NOMONSTER || wTileNo == T_EMPTY_F || wTileNo == T_REMOVE_FLOOR_ITEM);
	ASSERT(IsValidColRow(wX, wY));

	const UINT wSquareIndex = ARRAYINDEX(wX,wY);
	switch (TILE_LAYER[wTileNo])
	{
		case 0: //Opaque layer.
			this->pszOSquares[wSquareIndex] = static_cast<unsigned char>(wTileNo);

			RecalcStationPaths();

			this->PlotsMade.insert(wX,wY);
			this->geometryChanges.insert(wX,wY);  //always assume changes to o-layer affect room geometry for easier maintenance

			ReplaceOLayerTile(wX, wY, wTileNo);
		break;

		case 3: //Floor layer.
			this->pszFSquares[wSquareIndex] = static_cast<unsigned char>(
					wTileNo == T_EMPTY_F || wTileNo == T_REMOVE_FLOOR_ITEM ? T_EMPTY : wTileNo);
			RecalcStationPaths();
			this->PlotsMade.insert(wX,wY);
		break;

		case 1: //Transparent layer.
		{
			//For front end -- mark when objects that are part of room geometry change.
			const UINT oldTile = GetTSquare(wSquareIndex);
			const bool bGeometryChanging =
					(oldTile == T_ORB || oldTile == T_BOMB) &&
					!(wTileNo == T_ORB || wTileNo == T_BOMB);
			if (bGeometryChanging)
				this->geometryChanges.insert(wX,wY);

			ReplaceTLayerItem(wX, wY, wTileNo, bUnderObject);

			RecalcStationPaths();
			this->PlotsMade.insert(wX,wY);
		}
		break;

		case 2: //Monster layer.
			PlotMonster(wX, wY, wTileNo, pMonster);
		break;

		default: ASSERT(!"Invalid layer"); break;
	}

	UpdatePathMapAt(wX, wY);
	ReevalBriarNear(wX,wY,wTileNo);
}

//*****************************************************************************
void CDbRoom::Plot(const CCoordSet& plots, const bool bChangesRoomGeometry) //[default=false]
//Marks a set of tiles as plotted.
{
	this->PlotsMade += plots;
	if (bChangesRoomGeometry)
		this->geometryChanges += plots;
}

//*****************************************************************************
//Hooks to tell the front-end to refresh the display of custom image tiles
void CDbRoom::MarkPlotImageTiles()
{
	CCoordSet coords;
	UINT wX, wY;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			if (bIsCustomImageTile(this->GetOSquare(wX, wY)))
				coords.insert(wX, wY);
		}

	Plot(coords);
}

void CDbRoom::MarkPlotOverheadTiles()
{
	CCoordSet coords;
	UINT wX, wY;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			if (this->overheadTiles.Exists(wX, wY))
				coords.insert(wX, wY);
		}

	Plot(coords);
}

//*****************************************************************************
void CDbRoom::PlotMonster(UINT wX, UINT wY, UINT wTileNo, CMonster *pMonster)
{
	//Remove old monster/monster piece.
	CMonster *pOldMonster = GetMonsterAtSquare(wX, wY);
	if (pOldMonster)
	{
		if (pOldMonster->IsPiece())
		{
			//Remove piece from owning monster.
			CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pOldMonster);
			pMPiece->pMonster->Pieces.remove(pMPiece);
		}
		delete pOldMonster;
	}

	//Update whether monster now occupies square.
	const bool bPlacingMonsterPiece = pMonster && (bIsSerpentTile(wTileNo) || wTileNo == T_GENTRYII);
	if (bPlacingMonsterPiece)
	{
		ASSERT(pMonster->IsLongMonster());
		CMonsterPiece *pMPiece = new CMonsterPiece(pMonster, wTileNo, wX, wY);
		this->pMonsterSquares[ARRAYINDEX(wX,wY)] = pMPiece;
		pMonster->Pieces.push_back(pMPiece);
	} else {
		this->pMonsterSquares[ARRAYINDEX(wX,wY)] = wTileNo == T_NOMONSTER ? NULL : pMonster;
		if (wTileNo == T_NOMONSTER)
			this->PlotsMade.insert(wX,wY);
	}
}

//*****************************************************************************
void CDbRoom::ReplaceOLayerTile(
	const UINT wX, const UINT wY,
	const UINT wTileNo)
{
	this->bridges.plotted(wX,wY,wTileNo);

	//Maintain sets of uncommon tiles that have a periodic effect
	if (wTileNo == T_FLOOR_SPIKES) {
		this->floorSpikes.insert(wX,wY);
	} else {
		this->floorSpikes.erase(wX,wY);
	}
	if (wTileNo == T_FLUFFVENT) {
		this->fluffVents.insert(wX,wY);
	} else {
		this->fluffVents.erase(wX,wY);
	}
	if (wTileNo == T_FIRETRAP_ON) {
		this->activeFiretraps.insert(wX,wY);
	} else {
		this->activeFiretraps.erase(wX,wY);
	}

	//Specific plot types require in-game stat updates.
	switch (wTileNo)
	{
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD:
		case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
		case T_FLOOR_IMAGE:
		case T_HOT:
		case T_WATER: case T_SHALLOW_WATER: case T_STEP_STONE: case T_PIT: case T_PIT_IMAGE:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
			this->coveredOSquares.Add(wX,wY,wTileNo);
		break;
		case T_GOO:
		case T_WALL_B: case T_WALL_H:
			//If goo is placed on non-floor, then floor gets placed under it.
			if (!bIsPlainFloor(this->coveredOSquares.GetAt(wX,wY)))
			{
				this->coveredOSquares.Add(wX,wY,T_FLOOR);
				//If another floor type is adjacent to this tile, use that floor type.
				//This includes floor under an adjacent object of this type,
				//which overrides dissimilar object floor.
				bool bFloorFound = false, bThisItemFound = false;
				for (int nJ=-1; nJ<=1; ++nJ)
					for (int nI=-1; nI<=1; ++nI)
					{
						if (!IsValidColRow(wX+nI, wY+nJ))
							continue;
						const UINT wOTile = GetOSquare(wX+nI, wY+nJ);
						if (bIsPlainFloor(wOTile) && !bFloorFound)
						{
							this->coveredOSquares.Add(wX,wY, GetOSquare(wX+nI, wY+nJ));
							bFloorFound = true;
						}
						if (wOTile == wTileNo && !bThisItemFound)
						{
							this->coveredOSquares.Add(wX,wY, this->coveredOSquares.GetAt(wX+nI, wY+nJ));
							bThisItemFound = bFloorFound = true; //overrides dissimilar object floor
						}
					}
			}
		break;
		case T_TRAPDOOR:
			this->coveredOSquares.Add(wX,wY,T_PIT); //best guess
		break;
		case T_TRAPDOOR2: case T_THINICE:
			this->coveredOSquares.Add(wX,wY,T_WATER); //deep water only
		break;
		case T_THINICE_SH:
			this->coveredOSquares.Add(wX,wY,T_SHALLOW_WATER); //shallow water only
		break;
		default: break;
	}
}

//*****************************************************************************
void CDbRoom::ReplaceTLayerItem(const UINT wX, const UINT wY, const UINT wTileNo, const bool bUnderObject)
{
	const UINT tileIndex = ARRAYINDEX(wX,wY);
	RoomObject *tObj = this->tLayer[tileIndex];

	const UINT oldTile = tObj ? tObj->tile : RoomObject::emptyTile();

	if (oldTile == T_BRIAR_SOURCE)
		this->briars.removeSource(wX, wY);

//!!I think this logic can be cleaned up thanks to the new RoomObject data structures -- see updated pushable object logic for how to refactor
	bool bReplacedCoveringItem = (bIsTLayerCoveringItem(oldTile) && !bIsEmptyTile(wTileNo));
	if (bIsEmptyTile(wTileNo) && !bIsEmptyTile(GetCoveredTSquare(wX,wY)))
	{
		ASSERT(tObj);
		tObj->remove_top();
	} else if (bUnderObject && bIsTLayerCoveringItem(oldTile) &&
		(wTileNo == T_SCROLL || wTileNo == T_FUSE || wTileNo == T_TOKEN))
	{
		bReplacedCoveringItem = false;
		ASSERT(tObj);
		tObj->place_under(wTileNo);
	} else {
		tObj = SetTLayer(wX,wY,wTileNo);
	}

	//If a mirror/keg was removed, also remove anything that was underneath
	if (bReplacedCoveringItem) {
		ASSERT(tObj);
		tObj->remove_covered();
	}

	const bool bSetToBeacon = bIsBeacon(wTileNo);
	if (bIsBeacon(oldTile) != bSetToBeacon)
	{
		if (bSetToBeacon) {
			this->Beacons.insert(wX,wY);
		} else {
			this->Beacons.erase(wX,wY);
		}

		SetRoomLightingChanged();
	}

	if (tObj && !this->pCurrentGame)
		tObj->param = RoomObject::noParam(); //reset outside of play
}

//*****************************************************************************
void CDbRoom::ResetPressurePlatesState()
//Reset plates' depressed state.
{
	for (UINT i=this->orbs.size(); i--; )
		this->orbs[i]->bActive = false;
}

//*****************************************************************************
void CDbRoom::SetPressurePlatesState()
//Mark which pressure plates are currently depressed.
{
	//Player.
	if (this->pCurrentGame && this->pCurrentGame->swordsman.IsInRoom() &&
			!bIsEntityFlying(this->pCurrentGame->swordsman.wAppearance))
	{
		COrbData *pPlate = GetPressurePlateAtCoords(
				this->pCurrentGame->swordsman.wX, this->pCurrentGame->swordsman.wY);
		if (pPlate)
			pPlate->bActive = true;
	}

	//Monsters.
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		switch (pMonster->wType)
		{
			case M_CHARACTER:
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				if (!pCharacter->IsVisible())
					continue;
			}
			break;
		}
		if (!(pMonster->IsFlying() || pMonster->wType == M_SEEP))
		{
			COrbData *pPlate = GetPressurePlateAtCoords(pMonster->wX, pMonster->wY);
			if (pPlate)
				pPlate->bActive = true;
		}

		//Monster pieces.
		for (MonsterPieces::iterator piece = pMonster->Pieces.begin();
				piece != pMonster->Pieces.end(); ++piece)
		{
			CMonsterPiece *pPiece = *piece;
			COrbData *pPlate = GetPressurePlateAtCoords(pPiece->wX, pPiece->wY);
			if (pPlate)
				pPlate->bActive = true;
		}
	}

	//Room items.
	for (UINT wY=this->wRoomRows; wY--; )
		for (UINT wX=this->wRoomCols; wX--; )
			if (PressurePlateIsDepressedBy(GetTSquare(wX, wY)))
			{
				COrbData *pPlate = GetPressurePlateAtCoords(wX, wY);
				if (pPlate)
					pPlate->bActive = true;
			}
}

RoomObject* CDbRoom::SetTLayer(const UINT wX, const UINT wY, const UINT tile)
{
	ASSERT(IsValidColRow(wX, wY));

	const bool bSetToEmpty = tile == RoomObject::emptyTile();
	const UINT index = ARRAYINDEX(wX,wY);
	RoomObject *tObj = this->tLayer[index];
	if (tObj) {
		if (bSetToEmpty) {
			if (tObj->param == RoomObject::noParam()) {
//!!consider uncovering coveredItem, if present, to facilitate cleanup of ::ReplaceTLayerItem logic?
				RemoveTLayerObject(tObj);
				this->tLayer[index] = NULL;
			} else {
				tObj->tile = tile;
			}
		} else {
			*tObj = tile;
		}
	} else if (!bSetToEmpty) {
		tObj = this->tLayer[index] = AddTLayerObject(wX, wY, tile);
	}

	return tObj;
}

//*****************************************************************************
RoomObject* CDbRoom::AddTLayerObject(const UINT wX, const UINT wY, const UINT tile)
{
	RoomObject *tObj = new RoomObject(wX, wY, tile);
	this->tLayerObjects.push_back(tObj);
	return tObj;
}

//*****************************************************************************
void CDbRoom::RemoveTLayerObject(RoomObject* obj)
{
	ASSERT(obj);
	this->tLayerObjects.remove(obj);
	this->DeadRoomObjects.push_back(obj);
}

//*****************************************************************************
void CDbRoom::SetTParam(const UINT wX, const UINT wY, const BYTE value)
{
	ASSERT(IsValidColRow(wX, wY));

	//currently the t-layer param is only used to customize obstacles, lights, stations and tokens
	const UINT wTTile = GetBottomTSquare(wX,wY);
	ASSERT(wTTile == T_OBSTACLE ||
			bIsLight(wTTile) ||
			wTTile == T_TOKEN ||
			wTTile == T_STATION);

	RoomObject *tObj = this->tLayer[ARRAYINDEX(wX,wY)];
	ASSERT(tObj);
	tObj->param = value;
}

//*****************************************************************************
void CDbRoom::SetCoveredTLayerObject(const UINT wX, const UINT wY, const UINT tile)
{
	ASSERT(IsValidColRow(wX, wY));

	RoomObject *tObj = this->tLayer[ARRAYINDEX(wX,wY)];
	ASSERT(tObj);
	ASSERT(bIsTLayerCoveringItem(tObj->tile));

	tObj->coveredTile = tile;
}

//*****************************************************************************
int CDbRoom::DangerLevel() const
//Returns: int (how dangerous elements in the room make it)
{
	ASSERT(this->pCurrentGame);

	int nDanger = 0;

	if (!this->slayerEnters.empty()) ++nDanger;
	if (!this->slayer2Enters.empty()) ++nDanger;
	if (!this->halphEnters.empty()) ++nDanger;
	if (!this->halph2Enters.empty()) ++nDanger;
	if (this->bTarWasStabbed) ++nDanger;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		switch (pMonster->wType)
		{
		case M_BRAIN: case M_HALPH: case M_HALPH2:
		case M_ROACH: case M_REGG: case M_WWING: case M_EYE:
		case M_SPIDER:	case M_WUBBA: case M_FLUFFBABY: case M_SEEP:
		case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
		case M_TARBABY: case M_MUDBABY: case M_GELBABY:
		case M_WATERSKIPPER: 
		case M_AUMTLICH: case M_FEGUNDO: case M_FEGUNDOASHES:
		case M_ARCHITECT:
		case M_GENTRYII:
			++nDanger; break;

		case M_SKIPPERNEST: case M_GOBLIN: case M_GUARD: case M_ROCKGIANT: nDanger += 5; break;
		case M_QROACH: nDanger += 10; break;
		case M_NEATHER: case M_SLAYER: case M_SLAYER2: nDanger += 15; break;
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER: nDanger += 30; break;

		case M_ROCKGOLEM:
			if (pMonster->IsAggressive())
				++nDanger;
			break;
		case M_CONSTRUCT:
			if (pMonster->IsAggressive() || !this->pCurrentGame->IsCurrentRoomPendingExit())
				nDanger += 3;
			break;

		case M_CITIZEN:
			if (!this->stations.empty())
				++nDanger;
			break;

		case M_CHARACTER:
		{
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			if (pCharacter->IsRequiredToConquer() && pCharacter->IsVisible())
				++nDanger;
		}
		break;
		default:
			break; //NPCs and other monsters by default do not provide any urgency
		}
		pMonster = pMonster->pNext;
	}

	return nDanger;
}

//*****************************************************************************
bool CDbRoom::IsDoorOpen(
//Determines if a door is open or not.
//
//Accepts:
	const int nCol, const int nRow) //Coords to a door.
//
//Returns:
//true if it is, false if it isn't.
{
	ASSERT(IsValidColRow((UINT)nCol, (UINT)nRow));
	const UINT unTile=GetOSquare((UINT)nCol,(UINT)nRow);
	switch (unTile)
	{
		case T_DOOR_YO: return true;
		case T_DOOR_Y: return false;
		default: ASSERT(!"Bad door tile."); return false; //checked a tile that wasn't a door
	}
}

//*****************************************************************************
c4_Bytes* CDbRoom::PackSquares() const
//Saves room squares from member vars of object into database (version 2.0).
//
//Returns: pointer to record to be saved into database (must be deleted by caller).
{
	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);
	char *pSquares = new char[dwSquareCount*7 + 1];  //max size possible
	char *pWrite = pSquares;

	//1. Version of data format.
	*(pWrite++) = 6;

	//Run-length encoding info.
	char lastSquare, square = T_EMPTY;
	BYTE count=0, lastParam, param = 0;
	UINT dwSquareI;

	//2. Write opaque squares.
	ASSERT(this->pszOSquares);
	lastSquare = this->pszOSquares[0];
	for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		square = this->pszOSquares[dwSquareI];
		ASSERT(square != T_EMPTY);

		if (square == lastSquare && count < 255) //a char can store max run length of 255
			++count;
		else
		{
			//Write out old run info.  Start new run.
			ASSERT(count > 0);
			*(pWrite++) = (char)count;
			*(pWrite++) = lastSquare;

			lastSquare = square;
			count = 1;
		}
	}
	ASSERT(count > 0);
	*(pWrite++) = (char)count;
	*(pWrite++) = square;

	//3. Write floor-layer squares.
	ASSERT(this->pszFSquares);
	lastSquare = this->pszFSquares[0];
	count=0;
	for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		square = this->pszFSquares[dwSquareI];

		if (square == lastSquare && count < 255) //a char can store max run length of 255
			++count;
		else
		{
			//Write out old run info.  Start new run.
			ASSERT(count > 0);
			*(pWrite++) = (char)count;
			*(pWrite++) = lastSquare;

			lastSquare = square;
			count = 1;
		}
	}
	ASSERT(count > 0);
	*(pWrite++) = (char)count;
	*(pWrite++) = square;

	//4 and 5. Write transparent squares + parameter values.
	ASSERT(this->tLayer);
	lastSquare = GetTSquare(0);
	lastParam = GetTParam(0);
	count=0;
	for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		square = GetTSquare(dwSquareI);
		param = (bIsTLayerCoveringItem(square) ? GetCoveredTSquare(dwSquareI) : GetTParam(dwSquareI));

		if (square == lastSquare && param == lastParam &&
				count < 255) //a char can store max run length of 255
		{
			++count;
		} else {
			//Write out old run info.  Start new run.
			ASSERT(count > 0);
			*(pWrite++) = (char)count;
			*(pWrite++) = lastSquare;
			*(pWrite++) = lastParam;

			lastSquare = square;
			lastParam = param;
			count = 1;
		}
	}
	ASSERT(count > 0);
	*(pWrite++) = (char)count;
	*(pWrite++) = square;
	*(pWrite++) = param;

	//6. Write overhead layer, if necessary.
	if (this->overheadTiles.empty()) {
		*(pWrite++) = (char)0;
	} else {
		*(pWrite++) = (char)1; //indicates we have some data for this layer

		const BYTE *pData = this->overheadTiles.GetIndex();
		BYTE val, lastVal = *pData;
		count=0;
		for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI, ++pData)
		{
			val = *pData;

			if (*pData == lastVal && count < 255) {
				++count;
			} else {
				//Write out run info.  Start new run.
				ASSERT(count > 0);
				*(pWrite++) = (char)count;
				*(pWrite++) = (char)lastVal;

				lastVal = val;
				count = 1;
			}
		}
		ASSERT(count > 0);
		*(pWrite++) = (char)count;
		*(pWrite++) = (char)val;
	}

	const UINT dwSquaresLen = (UINT) (pWrite - pSquares);
	return new c4_Bytes(pSquares, dwSquaresLen);
}

//*****************************************************************************
c4_Bytes* CDbRoom::PackTileLights() const
//Saves room squares from member vars of object into database (version 3.0).
//
//Returns: pointer to record to be saved into database (must be deleted by caller).
{
	if (!this->tileLights.GetCols() || !this->tileLights.GetRows())
		return new c4_Bytes; //okay to have an empty object

	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);
	char *pTiles = new char[3*dwSquareCount + 1];  //max size possible
	char *pWrite = pTiles;

	//1. Current version of lights data format.
	*(pWrite++) = 5;

	//Run-length encoding info.
	UINT lastValue, val = T_EMPTY;
	BYTE count=0;

	//2. Write tile lights.
	const short *lights = this->tileLights.GetIndex();
	lastValue = lights[0];
	for (UINT dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		val = lights[dwSquareI];

		if (val == lastValue && count < 255) //a char can store max run length of 255
			++count;
		else
		{
			//Write out old run info.  Start new run.
			ASSERT(count > 0);
			*(pWrite++) = char(count);
			*(pWrite++) = char(lastValue/256); //2-byte value
			*(pWrite++) = char(lastValue%256);

			lastValue = val;
			count = 1;
		}
	}
	ASSERT(count > 0);
	*(pWrite++) = char(count);
	*(pWrite++) = char(val/256); //2-byte value
	*(pWrite++) = char(val%256);

	const UINT dwTilesLen = (UINT) (pWrite - pTiles);
	return new c4_Bytes(pTiles, dwTilesLen);
}

//*****************************************************************************
void CDbRoom::SaveOrbs(
//Saves orbs from member vars of object into database.
//
//Params:
	c4_View &OrbsView)      //(in) Open view to fill.
const
{
	UINT wOrbI, wOrbAgentI, wOrbCount=0;

	//Don't save orbs without any agents or special type.
	for (wOrbI=this->orbs.size(); wOrbI--; )
	{
		const COrbData& orb = *(this->orbs[wOrbI]);
		if (orb.agents.size() || orb.eType != OT_NORMAL || GetOSquare(orb.wX, orb.wY) == T_PRESSPLATE)
			++wOrbCount;
	}
	OrbsView.SetSize(wOrbCount);  //speed optimization
	wOrbCount = 0;

	for (wOrbI=0; wOrbI < this->orbs.size(); ++wOrbI)
	{
		COrbData& orb = *(this->orbs[wOrbI]);
		const UINT wNumAgents = orb.agents.size();
		if (wNumAgents || orb.eType != OT_NORMAL || GetOSquare(orb.wX, orb.wY) == T_PRESSPLATE)
		{
			//Save orb.
			c4_View OrbAgentsView;
			OrbAgentsView.SetSize(wNumAgents);
			for (wOrbAgentI=0; wOrbAgentI < wNumAgents; ++wOrbAgentI)
			{
				c4_RowRef agentRow = OrbAgentsView[wOrbAgentI];
				COrbAgentData& agent = *(orb.agents[wOrbAgentI]);
				p_Type(agentRow) = agent.action;
				p_X(agentRow) = agent.wX;
				p_Y(agentRow) = agent.wY;
			}
			c4_RowRef row = OrbsView[wOrbCount++];
			p_Type(row) = orb.eType;
			p_X(row) = orb.wX;
			p_Y(row) = orb.wY;
			p_OrbAgents(row) = OrbAgentsView;
		}
	}
}

//*****************************************************************************
void CDbRoom::SaveMonsters(
//Saves monsters from member vars of object into database.
//
//Params:
	c4_View &MonstersView)     //(in) Open view to fill.
{
	//Speed optimization.
	UINT wCount = 0, wNumMonsters = this->halphEnters.size() + this->halph2Enters.size() +
			this->slayerEnters.size() + this->slayer2Enters.size();
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		++wNumMonsters;
		pMonster = pMonster->pNext;
	}
	MonstersView.SetSize(wNumMonsters);

	pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pMonster->Save(MonstersView[wCount++]);
		pMonster = pMonster->pNext;
	}

	//Add Halph/Slayer entrances.
	CCoordSet::const_iterator enters;
	for (enters=this->halphEnters.begin(); enters!=this->halphEnters.end(); ++enters)
	{
		pMonster = CMonsterFactory::GetNewMonster(M_HALPH);
		pMonster->wX = enters->wX;
		pMonster->wY = enters->wY;
		pMonster->Save(MonstersView[wCount++]);
		delete pMonster;
	}
	for (enters=this->halph2Enters.begin(); enters!=this->halph2Enters.end(); ++enters)
	{
		pMonster = CMonsterFactory::GetNewMonster(M_HALPH2);
		pMonster->wX = enters->wX;
		pMonster->wY = enters->wY;
		pMonster->Save(MonstersView[wCount++]);
		delete pMonster;
	}
	for (enters=this->slayerEnters.begin(); enters!=this->slayerEnters.end(); ++enters)
	{
		pMonster = CMonsterFactory::GetNewMonster(M_SLAYER);
		pMonster->wX = enters->wX;
		pMonster->wY = enters->wY;
		pMonster->Save(MonstersView[wCount++]);
		delete pMonster;
	}
	for (enters=this->slayer2Enters.begin(); enters!=this->slayer2Enters.end(); ++enters)
	{
		pMonster = CMonsterFactory::GetNewMonster(M_SLAYER2);
		pMonster->wX = enters->wX;
		pMonster->wY = enters->wY;
		pMonster->Save(MonstersView[wCount++]);
		delete pMonster;
	}

	ASSERT(wCount == wNumMonsters); //these should match

	//Now remove any speeches/data marked for deletion.
	for (UINT wSpeechI=this->deletedSpeechIDs.size(); wSpeechI--; )
		g_pTheDB->Speech.Delete(this->deletedSpeechIDs[wSpeechI]);
	this->deletedSpeechIDs.clear();
	for (UINT wDataI=this->deletedDataIDs.size(); wDataI--; )
		g_pTheDB->Data.Delete(this->deletedDataIDs[wDataI]);
	this->deletedDataIDs.clear();
}

//*****************************************************************************
void CDbRoom::SaveScrolls(
//Saves scrolls from member vars of object into database.
//
//Params:
	c4_View &ScrollsView)      //(in) Open view to fill.
{
	ScrollsView.SetSize(this->Scrolls.size()); //speed optimization
	UINT wScrollI;
	for (wScrollI=0; wScrollI < this->Scrolls.size(); ++wScrollI)
	{
		c4_RowRef row = ScrollsView[wScrollI];
		CScrollData& scroll = *(this->Scrolls[wScrollI]);
		p_X(row) = scroll.wX;
		p_Y(row) = scroll.wY;
		p_MessageID(row) = scroll.ScrollText.UpdateText();
	}

	//Now remove any scroll message texts marked for deletion.
	for (wScrollI=this->deletedScrollIDs.size(); wScrollI--; )
		DeleteMessage(this->deletedScrollIDs[wScrollI]);
	this->deletedScrollIDs.clear();
}

//*****************************************************************************
void CDbRoom::SaveExits(
//Saves exits from member vars of object into database.
//
//Params:
	c4_View &ExitsView)     //(in) Open view to fill.
const
{
	ExitsView.SetSize(this->Exits.size()); //speed optimization
	for (UINT wExitI=0; wExitI < this->Exits.size(); ++wExitI)
	{
		c4_RowRef row = ExitsView[wExitI];
		CExitData& exitData = *(this->Exits[wExitI]);
		p_EntranceID(row) = exitData.dwEntranceID;
		p_Left(row) = exitData.wLeft;
		p_Right(row) = exitData.wRight;
		p_Top(row) = exitData.wTop;
		p_Bottom(row) = exitData.wBottom;
	}
}

//*****************************************************************************
void CDbRoom::SaveCheckpoints(
//Saves checkpoints from member vars of object into database.
//
//Params:
	c4_View &CheckpointsView)     //(in) Open view to fill.
const
{
	CheckpointsView.SetSize(this->checkpoints.size()); //speed optimization
	UINT wCount = 0;
	for (CCoordSet::const_iterator checkpoint = this->checkpoints.begin();
			checkpoint != this->checkpoints.end(); ++checkpoint)
	{
		c4_RowRef row = CheckpointsView[wCount++];
		p_X(row) = checkpoint->wX;
		p_Y(row) = checkpoint->wY;
	}
}

//*****************************************************************************
bool CDbRoom::UpdateExisting()
//Update an existing Rooms record in database.
{
	LOGCONTEXT("CDbRoom::UpdateExisting");
	ASSERT(this->dwRoomID != 0);
	ASSERT(IsOpen());

	//Lookup Rooms record.
	c4_View RoomsView;
	const UINT roomRowI = LookupRowByPrimaryKey(this->dwRoomID, V_Rooms, RoomsView);
	if (roomRowI == ROW_NO_MATCH)
	{
		ASSERT(!"Bad RoomID.");
		return false;
	}

	//Ensure the room is indexed under the level it is currently owned by.
	c4_RowRef row = RoomsView[roomRowI];
	CDb::moveRoom(this->dwRoomID, UINT(p_LevelID(row)), this->dwLevelID);

	UpdateFields(row);

	return true;
}

//*****************************************************************************
bool CDbRoom::UpdateNew()
//Add new Rooms record to database.
//Note: this doesn't add the room to its corresponding level.
{
	LOGCONTEXT("CDbRoom::UpdateNew");
	ASSERT(this->dwRoomID == 0);
	ASSERT(IsOpen());

	this->dwRoomID = GetIncrementedID(p_RoomID);

	c4_RowRef row = g_pTheDB->Rooms.GetNewRow();
	UpdateFields(row);

	CDb::addRoomToLevel(this->dwRoomID, this->dwLevelID);
	return true;
}

//*****************************************************************************
void CDbRoom::UpdateFields(c4_RowRef& row)
{
	//During import, this can be called while square data is still uninited.
	c4_Bytes *pSquaresBytes = CalcRoomArea() ? PackSquares() : new c4_Bytes;
	c4_Bytes *pLightsBytes = CalcRoomArea() ? PackTileLights() : new c4_Bytes;

	c4_View OrbsView;
	SaveOrbs(OrbsView);

	c4_View MonstersView;
	SaveMonsters(MonstersView);

	c4_View ScrollsView;
	SaveScrolls(ScrollsView);

	c4_View ExitsView;
	SaveExits(ExitsView);

	c4_View CheckpointsView;
	SaveCheckpoints(CheckpointsView);

	SetExtraVarsFromMembers();
	UINT dwExtraSize;
	BYTE *pbytExtraBytes = this->ExtraVars.GetPackedBuffer(dwExtraSize);
	ASSERT(pbytExtraBytes);
	c4_Bytes ExtraBytes(pbytExtraBytes, dwExtraSize);

	p_RoomID(row) = this->dwRoomID;
	p_LevelID(row) = this->dwLevelID;
	p_RoomX(row) = this->dwRoomX;
	p_RoomY(row) = this->dwRoomY;
	p_RoomCols(row) = this->wRoomCols;
	p_RoomRows(row) = this->wRoomRows;
	p_IsRequired(row) = this->bIsRequired;
	p_IsSecret(row) = this->bIsSecret;
	p_DataID(row) = this->dwDataID;
	p_ImageStartX(row) = this->wImageStartX;
	p_ImageStartY(row) = this->wImageStartY;
	p_OverheadDataID(row) = this->dwOverheadDataID;
	p_OverheadImageStartX(row) = this->wOverheadImageStartX;
	p_OverheadImageStartY(row) = this->wOverheadImageStartY;
	p_StyleName(row) = PutWString(this->style);
	p_Squares(row) = *pSquaresBytes;
	p_TileLights(row) = *pLightsBytes;
	p_Orbs(row) = OrbsView;
	p_Monsters(row) = MonstersView;
	p_Scrolls(row) = ScrollsView;
	p_Exits(row) = ExitsView;
	p_Checkpoints(row) = CheckpointsView;
	p_ExtraVars(row) = ExtraBytes;

	delete pSquaresBytes;
	delete pLightsBytes;
	delete[] pbytExtraBytes;
}

//*****************************************************************************
CDbRoom* CDbRoom::MakeCopy(CImportInfo& info, const UINT newHoldID, const bool bCopyForEditor) const
//Replicates room data for new record entry in DB.
{
	CDbRoom *pCopy = g_pTheDB->Rooms.GetNew();
	if (!pCopy) return NULL;
	pCopy->SetMembers(*this, false, true, bCopyForEditor);

	//Copy custom room media, if needed.
	if (newHoldID) {
		CDbData::CopyObject(info, pCopy->dwDataID, newHoldID);
		CDbData::CopyObject(info, pCopy->dwOverheadDataID, newHoldID);
	}

	return pCopy;
}

//*****************************************************************************
bool CDbRoom::SetMembers(
//For copy constructor and assignment operator.
//
//Params:
	const CDbRoom &Src,        //(in)
	const bool bCopyLocalInfo, //[default=true]
	const bool bCopyCurrentGame, //[default=true]
	const bool bCopyForEditor) //[default=false]
{
	try {

	UINT wIndex;

	Clear();

	//primitive types

	//Retain prior IDs, if requested.
	if (bCopyLocalInfo)
	{
		this->dwRoomID = Src.dwRoomID;
		this->dwLevelID = Src.dwLevelID;
	}
	this->dwRoomX = Src.dwRoomX;
	this->dwRoomY = Src.dwRoomY;
	this->wRoomCols = Src.wRoomCols;
	this->wRoomRows = Src.wRoomRows;
	this->style = Src.style;
	this->bIsRequired = Src.bIsRequired;
	this->bIsSecret = Src.bIsSecret;
	this->dwDataID = Src.dwDataID;
	this->wImageStartX = Src.wImageStartX;
	this->wImageStartY = Src.wImageStartY;
	this->dwOverheadDataID = Src.dwOverheadDataID;
	this->wOverheadImageStartX = Src.wOverheadImageStartX;
	this->wOverheadImageStartY = Src.wOverheadImageStartY;
	this->wMonsterCount = Src.wMonsterCount;
	this->wBrainCount = Src.wBrainCount;
	this->wTarLeft = Src.wTarLeft;
	this->wTrapDoorsLeft = Src.wTrapDoorsLeft;
	this->bBetterVision = Src.bBetterVision;
	this->bPersistentCitizenMovement = Src.bPersistentCitizenMovement;
	this->bHasConquerToken = Src.bHasConquerToken;
	this->bHasActiveBeacon = Src.bHasActiveBeacon;
	this->bTarWasBuilt = Src.bTarWasBuilt;
	this->bTarstuffGateTogglePending = Src.bTarstuffGateTogglePending;

	//Room tile layers
	if (!AllocTileLayers())
		throw CException("CDbRoom::SetMembers alloc failed");

	const UINT dwSquareCount = CalcRoomArea();
	memcpy(this->pszOSquares, Src.pszOSquares, dwSquareCount * sizeof(char));
	memcpy(this->pszFSquares, Src.pszFSquares, dwSquareCount * sizeof(char));
	CopyTLayer(Src.tLayerObjects);
	this->overheadTiles = Src.overheadTiles;

	this->tileLights = Src.tileLights;

	//Special room data
	for (wIndex=0; wIndex<Src.orbs.size(); ++wIndex)  //must retain order
		this->orbs.push_back(new COrbData(*(Src.orbs[wIndex])));
	for (wIndex=Src.Scrolls.size(); wIndex--; )
	{
		CScrollData *pScroll = new CScrollData();
		pScroll->wX = Src.Scrolls[wIndex]->wX;
		pScroll->wY = Src.Scrolls[wIndex]->wY;
		if (bCopyLocalInfo)
		{
			//Don't make a duplicate copy of the scroll text in DB.
			const UINT dwMessageID = Src.Scrolls[wIndex]->ScrollText.GetMessageID();
			if (dwMessageID)
				pScroll->ScrollText.Bind(dwMessageID);
		}
		//Make a copy of the text string.
		pScroll->ScrollText = Src.Scrolls[wIndex]->ScrollText;

		this->Scrolls.push_back(pScroll);
	}
	for (wIndex=0; wIndex<Src.Exits.size(); ++wIndex)
	{
		CExitData *pExit = new CExitData(*Src.Exits[wIndex]);
		if (!pExit) throw CException("CDbRoom::SetMembers alloc failed");
		this->Exits.push_back(pExit);
	}
	this->checkpoints = Src.checkpoints;

	this->ExtraVars = Src.ExtraVars;
	this->pressurePlateIndex = Src.pressurePlateIndex;
	this->weather = Src.weather;

	if (bCopyCurrentGame) {
		this->pCurrentGame = Src.pCurrentGame;
	}

	//Monster data
	this->pFirstMonster = this->pLastMonster = NULL;

	memset(this->pMonsterSquares, 0, dwSquareCount * sizeof(CMonster*));
	CMonster *pMonster, *pTrav;
	for (pTrav = Src.pFirstMonster; pTrav != NULL; pTrav = pTrav->pNext)
	{
		pMonster = bCopyLocalInfo ? pTrav->Clone() : pTrav->Replicate();

		bool bIsInRoom = bCopyForEditor || pMonster->IsVisible();

		//Add monsters in the same order they appear in the source list.
		const UINT wProcessSequence = pMonster->wProcessSequence;
		pMonster->wProcessSequence = UINT(-1); //place at end

		LinkMonster(pMonster, bIsInRoom);

		//restore value
		pMonster->wProcessSequence = wProcessSequence;

		//Copy monster pieces.
		pMonster->Pieces.clear();  //make new copies of pieces
		for (MonsterPieces::iterator piece = pTrav->Pieces.begin();
				piece != pTrav->Pieces.end(); ++piece)
		{
			CMonsterPiece *pOldPiece = *piece;
			CMonsterPiece *pNewPiece = new CMonsterPiece(pMonster, pOldPiece->wTileNo, pOldPiece->wX, pOldPiece->wY);
			pMonster->Pieces.push_back(pNewPiece);
			if (Src.pMonsterSquares[ARRAYINDEX(pOldPiece->wX,pOldPiece->wY)] == pOldPiece)
			{
				ASSERT(!this->pMonsterSquares[ARRAYINDEX(pOldPiece->wX,pOldPiece->wY)]);
				this->pMonsterSquares[ARRAYINDEX(pOldPiece->wX,pOldPiece->wY)] = pNewPiece;
			}
		}
		pMonster->ResetCurrentGame();
	}

	this->halphEnters = Src.halphEnters;
	this->halph2Enters = Src.halph2Enters;
	this->slayerEnters = Src.slayerEnters;
	this->slayer2Enters = Src.slayer2Enters;

	if (bCopyLocalInfo)
	{
		this->deletedScrollIDs = Src.deletedScrollIDs;
		this->deletedSpeechIDs = Src.deletedSpeechIDs;
		this->deletedDataIDs = Src.deletedDataIDs;

		//In-game state information.
		this->PlotsMade = Src.PlotsMade;
		//	this->geometryChanges = Src.geometryChanges; //temporary front-end only info not needed
		//	this->disabledLights = Src.disabledLights;
		if (Src.pLastClone)
		{
			this->pLastClone = this->pMonsterSquares[ARRAYINDEX(Src.pLastClone->wX,Src.pLastClone->wY)];
			ASSERT(this->pLastClone);
		}
		for (wIndex=NumMovementTypes; wIndex--; )
			if (Src.pPathMap[wIndex])
				this->pPathMap[wIndex] = new CPathMap(*(Src.pPathMap[wIndex]));
		this->LitFuses = Src.LitFuses;
		this->NewFuses = Src.NewFuses;
		this->NewBabies = Src.NewBabies;
		this->briars.setMembersForRoom(Src.briars, this);
		for (wIndex=0; wIndex<Src.platforms.size(); ++wIndex)
		{
			CPlatform *pPlatform = new CPlatform(*(Src.platforms[wIndex]));
			this->platforms.push_back(pPlatform);
		}
		for (wIndex=0; wIndex<Src.stations.size(); ++wIndex)
		{
			CStation *pStation = new CStation(*(Src.stations[wIndex]), this);
			this->stations.push_back(pStation);
		}
		this->coveredOSquares = Src.coveredOSquares;
		this->bTarWasStabbed = Src.bTarWasStabbed;
		this->bGreenDoorsOpened = Src.bGreenDoorsOpened;
		this->bridges.setMembersForRoom(Src.bridges, this);
		this->building.setMembers(Src.building);
		this->floorSpikes = Src.floorSpikes;
		this->fluffVents = Src.fluffVents;
		this->activeFiretraps = Src.activeFiretraps;
		this->Beacons = Src.Beacons;
		this->bCheckForHoldCompletion = Src.bCheckForHoldCompletion;
		this->bCheckForHoldMastery = Src.bCheckForHoldMastery;

		//not needed, as these are only used for processing the current turn.  It also won't work when pointers are invalidated.
		//ClearStateVarsUsedDuringTurn();
		this->room_lighting_changed = Src.room_lighting_changed;

		for (list<CMonster*>::const_iterator m = Src.DeadMonsters.begin();
				m != Src.DeadMonsters.end(); ++m)
		{
			pTrav = *m;
			pMonster = bCopyLocalInfo ? pTrav->Clone() : pTrav->Replicate();
			//Copy monster pieces.
			pMonster->Pieces.clear();  //make new copies of pieces
			for (MonsterPieces::iterator piece = pTrav->Pieces.begin();
					piece != pTrav->Pieces.end(); ++piece)
			{
				CMonsterPiece *pOldPiece = *piece;
				CMonsterPiece *pNewPiece = new CMonsterPiece(pMonster, pOldPiece->wTileNo, pOldPiece->wX, pOldPiece->wY);
				pMonster->Pieces.push_back(pNewPiece);
			}
			this->DeadMonsters.push_back(pMonster);
		}

		//this->DeadRoomObjects -- don't think we need to create a copy of these, as it is reset each turn
	}

	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
void CDbRoom::CopyTLayer(const list<RoomObject*>& src)
{
	memset(this->tLayer, 0, CalcRoomArea() * sizeof(RoomObject*));

	for (list<RoomObject*>::const_iterator it=src.begin(); it!=src.end(); ++it) {
		const RoomObject *pObj = *it;
		if (pObj) {
			const RoomObject& obj = *pObj;
			RoomObject *copy = new RoomObject(obj);
			this->tLayerObjects.push_back(copy);
			this->tLayer[ARRAYINDEX(obj.wX, obj.wY)] = copy;
		}
	}
}

//*****************************************************************************
void CDbRoom::SetExtraVarsFromMembers()
//Pack extra vars.
{
	//Weather vars.
	CDbPackedVars& v = this->ExtraVars;
	Weather& w = this->weather;
	v.SetVar(WEATHER_OUTSIDE, w.bOutside);
	v.SetVar(WEATHER_LIGHTNING, w.bLightning);
	v.SetVar(WEATHER_CLOUDS, w.bClouds);
	v.SetVar(WEATHER_SUN, w.bSunshine);
	v.SetVar(WEATHER_LIGHTFADE, w.bSkipLightfade);
	v.SetVar(WEATHER_FOG, w.wFog);
	v.SetVar(WEATHER_LIGHT, w.wLight);
	v.SetVar(WEATHER_SNOW, w.wSnow);
	v.SetVar(WEATHER_SKY, w.sky.c_str());
	v.SetVar(WEATHER_RAIN, w.rain);
}

//*****************************************************************************
void CDbRoom::SetMembersFromExtraVars()
//Unpack extra vars.
{
	//Weather vars.
	CDbPackedVars& v = this->ExtraVars;
	Weather& w = this->weather;
	w.bOutside = v.GetVar(WEATHER_OUTSIDE, false);
	w.bLightning = v.GetVar(WEATHER_LIGHTNING, false);
	w.bClouds = v.GetVar(WEATHER_CLOUDS, false);
	w.bSunshine = v.GetVar(WEATHER_SUN, false);
	w.bSkipLightfade = v.GetVar(WEATHER_LIGHTFADE, false);
	w.wFog = v.GetVar(WEATHER_FOG, UINT(0));
	w.wLight = v.GetVar(WEATHER_LIGHT, UINT(0));
	w.wSnow = v.GetVar(WEATHER_SNOW, UINT(0));
	w.sky = v.GetVar(WEATHER_SKY, wszEmpty);
	w.rain = v.GetVar(WEATHER_RAIN, UINT(0));
}

//*****************************************************************************
inline UINT getMaxTileType(
//Returns: tile type with most representation, or the first one if none of any
//
//Params:
	UINT* const wTileCounts, UINT* const tileTypes, UINT numTileTypes, const UINT wMajorityType)
{
	ASSERT(numTileTypes > 0);
	UINT wNum, wMax=0, wMaxType=wMajorityType;
	while (numTileTypes--)
	{
		wNum = wTileCounts[tileTypes[numTileTypes]];
		if (wNum > wMax)
		{
			wMax = wNum;
			wMaxType = tileTypes[numTileTypes];
		}
	}
	return wMaxType;
}

void CDbRoom::InitCoveredTiles()
//Initialize a room-sized grid of o-layer tile values, representing what tiles
//are underneath existing tiles that can be removed.  In other words, if o-layer
//tiles are removed, this grid contains the tiles that will replace them.
{
	//Heuristically determine implicit o-layer floor tiles.
	//These will appear under: Broken walls + Doors (included to propagate styles through them).
	static const UINT numFloors = 6, numPits = 2;
	static const UINT floors[numFloors] = {T_FLOOR, T_FLOOR_M, T_FLOOR_ROAD,
			T_FLOOR_GRASS, T_FLOOR_DIRT, T_FLOOR_ALT};   //not T_FLOOR_IMAGE
	static const UINT pits[numPits] = {T_PIT, T_PIT_IMAGE};
	static CTileMask floorMask, pitMask;
	if (floorMask.empty())
	{
		floorMask.set(T_CHECKPOINT);  //keep for 1.6 import
		floorMask.set(T_WALL_B);
		floorMask.set(T_WALL_H);
		floorMask.set(T_WALL_M);
		floorMask.set(T_WALL_WIN);
		floorMask.set(T_DOOR_C);
		floorMask.set(T_DOOR_M);
		floorMask.set(T_DOOR_R);
		floorMask.set(T_DOOR_Y);
		floorMask.set(T_DOOR_B);
		floorMask.set(T_DOOR_YO);
		floorMask.set(T_DOOR_GO);
		floorMask.set(T_DOOR_CO);
		floorMask.set(T_DOOR_RO);
		floorMask.set(T_DOOR_BO);
		floorMask.set(T_TUNNEL_N);
		floorMask.set(T_TUNNEL_S);
		floorMask.set(T_TUNNEL_E);
		floorMask.set(T_TUNNEL_W);
		floorMask.set(T_GOO);
		floorMask.set(T_FLOOR_IMAGE);

		pitMask.set(T_TRAPDOOR);
		pitMask.set(T_PLATFORM_P);
	}

	this->coveredOSquares.Init(this->wRoomCols, this->wRoomRows);

	UINT wNumTiles[TILE_COUNT] = {0};
	UINT wX, wY, wOSquare;

	//Mark all tiles that we know the value of.  Count how many exist of each type.
	for (wY=this->wRoomRows; wY--; )
		for (wX=this->wRoomCols; wX--; )
		{
			wOSquare = GetOSquare(wX, wY);
			switch (wOSquare)
			{
				case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
				case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD:
				case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
				case T_WALL: case T_WALL2: case T_WALL_IMAGE:
				case T_PIT: case T_PIT_IMAGE:
				case T_STAIRS:	case T_STAIRS_UP:
				case T_HOT:
				case T_PRESSPLATE:
				case T_WATER: case T_SHALLOW_WATER: case T_STEP_STONE:
				case T_FLOOR_SPIKES:
				case T_FLUFFVENT:
				case T_FIRETRAP: case T_FIRETRAP_ON:
					this->coveredOSquares.Add(wX,wY,wOSquare);
					++wNumTiles[wOSquare];
				break;
				case T_TRAPDOOR2:
				case T_PLATFORM_W:
				case T_THINICE:
					//Water trapdoors and rafts are over deep water.
					this->coveredOSquares.Add(wX,wY,T_WATER);
					++wNumTiles[wOSquare];
				break;
				case T_THINICE_SH:
					//Thin Ice can be over shallow water as well.
					this->coveredOSquares.Add(wX,wY,T_SHALLOW_WATER);
					++wNumTiles[wOSquare];
				break;
				default: break;   //Other types can change.  Consider them below.
			}
		}

	MarkTilesForStyle(floorMask, (UINT*)floors, numFloors,
			getMaxTileType(wNumTiles, (UINT*)floors, numFloors, floors[0]));
	MarkTilesForStyle(pitMask, (UINT*)pits, numPits,
			getMaxTileType(wNumTiles, (UINT*)pits, numPits, pits[0]));
}

//*****************************************************************************
void CDbRoom::InitStateForThisTurn()
{
	ClearStateVarsUsedDuringTurn();

	//Any lit fuses are no longer "new", and eligible to spread next time
	//we reach BurnFuses().
	this->LitFuses += this->NewFuses;
	this->NewFuses.clear();

	for (list<RoomObject*>::const_iterator it=this->tLayerObjects.begin();
		it!=this->tLayerObjects.end(); ++it)
	{
		const RoomObject* obj = *it;
		if (obj->tile == T_POWDER_KEG)
			this->stationary_powder_kegs.insert(obj);
	}
}

//*****************************************************************************
void CDbRoom::InitRoomStats()
//Initialize variables whose values are implicitly determined by the contents
//of squares in the room.
{
	InitCoveredTiles();

	this->wTrapDoorsLeft = 0;
	this->wTarLeft = 0;
	this->bBetterVision = false;
	this->bPersistentCitizenMovement = this->bHasConquerToken = this->bHasActiveBeacon = false;
	this->bCheckForHoldCompletion = false;
	this->bCheckForHoldMastery = false;
	this->briars.clear();
	this->briars.setRoom(this); //call after clear()
	this->bridges.setRoom(this);
	this->building.init(this->wRoomCols, this->wRoomRows);
	ClearPlatforms();
	this->floorSpikes.clear();
	this->fluffVents.clear();
	this->activeFiretraps.clear();

	char *pszSeek, *pszStop;
	pszStop = this->pszOSquares + CalcRoomArea() * sizeof(char);
	CCoordIndex plots(this->wRoomCols, this->wRoomRows);
	CCoordIndex obstacles(this->wRoomCols, this->wRoomRows);
	for (pszSeek = this->pszOSquares; pszSeek != pszStop; ++pszSeek)
	{
		switch (*pszSeek)
		{
			//Number of trapdoors left.
			case T_TRAPDOOR: case T_TRAPDOOR2:
				++this->wTrapDoorsLeft;
			break;

			case T_WALL_M:
				this->bCheckForHoldMastery = true;
			break;

			case T_WALL_WIN:
				this->bCheckForHoldCompletion = true;
			break;

			//Checkpoints are moved from the o-layer in version 1.6 to a meta-layer in 2.0
			case T_CHECKPOINT:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				Plot(wX, wY, this->coveredOSquares.GetAt(wX, wY));
				this->checkpoints.insert(wX,wY);
			}
			break;
			//Obstacles are moved from the o-layer in version 1.6 to the t-layer in 2.0
			case T_OBSTACLE:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				if (GetTSquare(wX, wY) != T_EMPTY)
				{
					//keep obstacle-like object if t-layer is already occupied
					*pszSeek = T_WALL;
				} else {
					//move obstacle to t-layer
					*pszSeek = T_FLOOR;
					Plot(wX, wY, T_OBSTACLE);
					obstacles.Add(wX,wY);
				}
			}
			break;
			//Platforms.
			case T_PLATFORM_W: case T_PLATFORM_P:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				if (!plots.Exists(wX,wY))
					AddPlatformPiece(wX, wY, plots);
			}
			break;
			//Bridges.
			case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->bridges.addBridge(wX, wY);
			}
			break;

			case T_FLOOR_SPIKES:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->floorSpikes.insert(wX, wY);
			}
			break;

			case T_FLUFFVENT:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->fluffVents.insert(wX, wY);
			}
			break;
			case T_FIRETRAP_ON:
			{
				const UINT index = pszSeek - this->pszOSquares;
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->activeFiretraps.insert(wX, wY);
			}
			break;
		}
	}

	for (UINT index = 0; index < CalcRoomArea(); ++index)
	{
		switch (GetTSquare(index))
		{
			//Number of tar left.
			case T_TAR: case T_MUD: case T_GEL:
				++this->wTarLeft;
			break;
			//Briar roots.
			case T_BRIAR_SOURCE:
			{
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->briars.insert(wX,wY);
			}
			break;
			case T_STATION:
			{
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->stations.push_back(new CStation(wX, wY, this));
			}
			break;
			//Beacons.
			case T_BEACON: case T_BEACON_OFF:
			{
				const UINT wX = index % this->wRoomCols;
				const UINT wY = index / this->wRoomCols;
				this->Beacons.insert(wX, wY);
				if (GetTSquare(index) == T_BEACON)
					this->bHasActiveBeacon = true;
			}
			break;
			case T_TOKEN:
			{
				const UINT tParam = GetTParam(index);
				switch (tParam)
				{
					case PersistentCitizenMovement:
						this->bPersistentCitizenMovement = true;
					break;
					case ConquerToken:
						this->bHasConquerToken = true;
					break;
					default: break;
				}
			}
			break;
		}
	}

	ObstacleFill(obstacles);
	this->briars.initLiveTiles();
}

//*****************************************************************************
void CDbRoom::MarkTilesForStyle(
//Compile connected groups of tiles of indicated type(s).  For each group,
//decided what tile style should replace them if and when they are removed.
//
//Params:
	const CTileMask &mask,  //(in) tiles to consider
	UINT* const tileTypes, const UINT numTileTypes, const UINT wMajorityStyle)   //(in)
{
	UINT wX, wY, wOSquare;
	//Scan all room tiles.  Mark any unmarked tiles.
	for (wY=this->wRoomRows; wY--; )
		for (wX=this->wRoomCols; wX--; )
			if (!this->coveredOSquares.Exists(wX, wY))
			{
				//Add this square to the grid.
				wOSquare = GetOSquare(wX, wY);
				ASSERT(wOSquare != 0);  //values of zero aren't compatible with representation of CCoordIndex::Exists().
				//1. Compile set of elements needing to be marked.
				if (mask.get(wOSquare))
				{
					MarkTilesFromSquare(wX, wY, mask, tileTypes, numTileTypes, wMajorityStyle, false);
					MarkTilesFromSquare(wX, wY, mask, tileTypes, numTileTypes, wMajorityStyle, true);
				}
			}
}

//*****************************************************************************
void CDbRoom::MarkTilesFromSquare(
	const UINT wX, const UINT wY, //(in) tile to start from
	const CTileMask &mask,  //(in) tiles to consider
	UINT* const tileTypes, const UINT numTileTypes, const UINT wMajorityStyle,   //(in)
	const bool b8Neighbor)  //(in) search constraint
{
	ASSERT(wMajorityStyle < TILE_COUNT);

	CCoordSet squares;
	CCoordSet::const_iterator iter;
	UINT wVoteTiles[TILE_COUNT] = {0};
	UINT wSX, wSY, wOSquare;
	int nI, nJ;

	GetConnectedTiles(wX, wY, mask, b8Neighbor ? Connect_8 : Connect_4, squares);
	//1. For each connected component section of these tiles
	//tally number of its 8-neighbor perimeter style tiles and vote.
	for (iter = squares.begin(); iter != squares.end(); ++iter)
	{
		wSX = iter->wX; wSY = iter->wY;
		for (nJ=-1; nJ<=1; ++nJ)
			for (nI=-1; nI<=1; ++nI)
				if (IsValidColRow(wSX+nI, wSY+nJ))
					++wVoteTiles[GetOSquare(wSX+nI, wSY+nJ)];
	}
	//2b. Mark all these tiles with the majority floor style.
	wOSquare = getMaxTileType(wVoteTiles, tileTypes, numTileTypes, wMajorityStyle);

	//Mark unmarked tiles according to majority vote.
	for (iter = squares.begin(); iter != squares.end(); ++iter)
	{
		wSX = iter->wX; wSY = iter->wY;
		if (!this->coveredOSquares.Exists(wSX,wSY))
			this->coveredOSquares.Add(wSX,wSY,wOSquare);
	}
}

//*****************************************************************************
void CDbRoom::GetConnectedTiles(
//Compiles set of all contiguous (connected component) squares containing tiles
//of indicated type, starting from (wX,wY).
//If there is no tile of indicated type at the starting coords, outputs an empty set.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	const CTileMask &tileMask,    //(in) which type of tiles to gather
	const TileConnectionStrategy eConnect, //(in) connection strategy
	CCoordSet& squares,         //(out) set of contiguous tiles
	const CCoordSet* pIgnoreSquares, //(in) optional set of squares to ignore [default=NULL]
	const CCoordSet* pRegionMask)    //(in) optional region to limit the search to [default=NULL]
const
{
	squares.clear();

	if (!IsValidColRow(wX,wY))
		return;

	if (pIgnoreSquares && pIgnoreSquares->has(wX, wY)) return;
	if (pRegionMask && !pRegionMask->has(wX, wY)) return;

	if (!tileMask.get(GetOSquare(wX, wY)) && !tileMask.get(GetTSquare(wX, wY)))
		return;

#define PushTileIfOfType(x,y) {\
	ASSERT(IsValidColRow((x), (y)));\
	if ( (tileMask.get(GetOSquare((x), (y))) || tileMask.get(GetTSquare((x), (y))))\
			&& !squares.has((x), (y)) && (!pIgnoreSquares || !pIgnoreSquares->has((x), (y)))\
			&& (!pRegionMask || pRegionMask->has((x), (y))) )\
		evalCoords.Push((x), (y));}

#define CheckForPitAt(x,y) {\
	switch (GetOSquare((x),(y))) {\
		case T_PIT: case T_PIT_IMAGE: case T_PLATFORM_P: bAdjacentPit = true; break; } }

	const bool b8Neighbor = eConnect == Connect_8;
	const UINT rightEdge = this->wRoomRows - 1;
	const UINT lowerEdge = this->wRoomCols - 1;

	UINT wEvalX, wEvalY;
	CCoordStack evalCoords(wX,wY);
	while (evalCoords.PopBottom(wEvalX,wEvalY)) //perform as a queue for performance
	{
		// If the tile is of this type, add it to the squares set and
		// push surrounding tiles onto the evaluation stack.
		if (!squares.insert(wEvalX, wEvalY))
			continue; //don't need to reprocess this tile if it was already handled

		const bool bGTLeft = wEvalX > 0;
		const bool bGTTop = wEvalY > 0;
		const bool bLTBottom = wEvalY < rightEdge;
		const bool bLTRight = wEvalX < lowerEdge;

		//Add adjacent coords to Eval stack.
		if (eConnect == Connect_8_WithoutAxialPit) {
			bool bAdjacentPit = false;

			if (bGTLeft) {
				CheckForPitAt(wEvalX - 1, wEvalY);
				PushTileIfOfType(wEvalX - 1, wEvalY);
			}
			if (bGTTop) {
				CheckForPitAt(wEvalX, wEvalY - 1);
				PushTileIfOfType(wEvalX, wEvalY - 1);
			}
			if (bLTRight) {
				CheckForPitAt(wEvalX + 1, wEvalY);
				PushTileIfOfType(wEvalX + 1, wEvalY);
			}
			if (bLTBottom) {
				CheckForPitAt(wEvalX, wEvalY + 1);
				PushTileIfOfType(wEvalX, wEvalY + 1);
			}

			if (!bAdjacentPit) {
				if (bGTLeft) {
					if (bGTTop) PushTileIfOfType(wEvalX - 1, wEvalY - 1);
					if (bLTBottom) PushTileIfOfType(wEvalX - 1, wEvalY + 1);
				}
				if (bLTRight) {
					if (bGTTop) PushTileIfOfType(wEvalX + 1, wEvalY - 1);
					if (bLTBottom) PushTileIfOfType(wEvalX + 1, wEvalY + 1);
				}
			}
		} else {
			if (bGTLeft)
			{
				PushTileIfOfType(wEvalX - 1, wEvalY);
				if (b8Neighbor)
				{
					if (bGTTop) PushTileIfOfType(wEvalX - 1, wEvalY - 1);
					if (bLTBottom) PushTileIfOfType(wEvalX - 1, wEvalY + 1);
				}
			}
			if (bGTTop) PushTileIfOfType(wEvalX, wEvalY - 1);
			if (bLTRight)
			{
				PushTileIfOfType(wEvalX + 1, wEvalY);
				if (b8Neighbor)
				{
					if (bGTTop) PushTileIfOfType(wEvalX + 1, wEvalY - 1);
					if (bLTBottom) PushTileIfOfType(wEvalX + 1, wEvalY + 1);
				}
			}
			if (bLTBottom) PushTileIfOfType(wEvalX, wEvalY + 1);
		}
	}
#undef PushTileIfOfType
#undef CheckForPitAt
}

//*****************************************************************************
void CDbRoom::GetConnectedRegionsAround(
//Compiles a list of unique connected tile groups surrounding a square.
//The given square is not considered to connect them together.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	const CTileMask &tileMask,    //(in) which type of tiles to gather
	vector<CCoordSet>& regions,   //(out) sets of contiguous tiles
	const CCoordSet* pIgnoreSquares,	//(in) optional set of squares to ignore [default=NULL]
	const CCoordSet* pRegionMask)    //(in) optional region to limit the search to [default=NULL]
const
{
	regions.clear();
	regions.reserve(4);
	UINT numRegions = 0;
	bool haveE=false, haveS=false, haveW=false;

	CCoordSet ignore;
	if (pIgnoreSquares)
		ignore = *pIgnoreSquares;
	ignore.insert(wX,wY);

	{
		regions.resize(++numRegions);
		CCoordSet& currRegion = regions.back();
		GetConnected4NeighborTiles(wX, wY-1, tileMask, currRegion, &ignore, pRegionMask);
		if (currRegion.has(wX+1,wY))
			haveE = true;
		if (currRegion.has(wX,wY+1))
			haveS = true;
		if (currRegion.has(wX-1,wY))
			haveW = true;
		if (currRegion.empty())
			--numRegions;
	}

	if (!haveE)
	{
		regions.resize(++numRegions);
		CCoordSet& currRegion = regions.back();
		GetConnected4NeighborTiles(wX+1, wY, tileMask, currRegion, &ignore, pRegionMask);
		if (currRegion.has(wX,wY+1))
			haveS = true;
		if (currRegion.has(wX-1,wY))
			haveW = true;
		if (currRegion.empty())
			--numRegions;
	}

	if (!haveS)
	{
		regions.resize(++numRegions);
		CCoordSet& currRegion = regions.back();
		GetConnected4NeighborTiles(wX, wY+1, tileMask, currRegion, &ignore, pRegionMask);
		if (currRegion.has(wX-1,wY))
			haveW = true;
		if (currRegion.empty())
			--numRegions;
	}

	if (!haveW)
	{
		regions.resize(++numRegions);
		CCoordSet& currRegion = regions.back();
		GetConnected4NeighborTiles(wX-1, wY, tileMask, currRegion, &ignore, pRegionMask);
		if (currRegion.empty())
			--numRegions;
	}

	regions.resize(numRegions);
}

//*****************************************************************************
void CDbRoom::GetAllYellowDoorSquares(
//Compiles set of all squares for yellow door, starting at (wX,wY).
//If there is no door there, returns an empty set.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	CCoordSet& squares,         //(out) set of contiguous door squares
	const CCoordSet* pIgnoreSquares)	 //(in) optional set to ignore [default=NULL]
const
{
	CTileMask doorMask(T_DOOR_Y);
	doorMask.set(T_DOOR_YO);
	GetConnected4NeighborTiles(wX, wY, doorMask, squares, pIgnoreSquares);
}

//*****************************************************************************
void CDbRoom::MarkSpeechForDeletion(
//Keep track of speech's ID so that speech object is deleted if Update is called.
//
//Params:
	CDbSpeech* pSpeech)           //(in)
{
	ASSERT(pSpeech);
	if (pSpeech->dwSpeechID)
		this->deletedSpeechIDs.push_back(pSpeech->dwSpeechID);
}

//*****************************************************************************
void CDbRoom::MarkDataForDeletion(const CDbDatum* pDatum)
//Keep track of data IDs so that data object is deleted if Update is called.
{
	if (pDatum && pDatum->dwDataID)
		this->deletedDataIDs.push_back(pDatum->dwDataID);
}

//*****************************************************************************
void CDbRoom::ObstacleFill(CCoordIndex& obstacles)
//Fill marked obstacle tiles with the largest obstacles
//of their type that will fit.  Top-down, left-to right.
{
	if (!obstacles.GetSize()) return;   //no obstacles marked

	UINT wX, wY;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			BYTE obType = obstacles.GetAt(wX,wY);
			if (obType)
			{
				const BYTE obType = 1 + RAND(2);  //rock obstacle types
				ASSERT(GetTSquare(wX,wY) == T_OBSTACLE);
				obstacles.Remove(wX,wY);
				SetTParam(wX, wY, OBSTACLE_TOP | OBSTACLE_LEFT | obType);

				//Check for 2x2 obstacle.
				if (IsValidColRow(wX+1, wY+1) &&	//handles all three cases
						obstacles.GetAt(wX, wY+1) && obstacles.GetAt(wX+1, wY) &&
						obstacles.GetAt(wX+1, wY+1))
				{
					SetTParam(wX, wY+1, OBSTACLE_LEFT | obType);
					SetTParam(wX+1, wY, OBSTACLE_TOP | obType);
					SetTParam(wX+1, wY+1, obType);
					obstacles.Remove(wX,wY+1);
					obstacles.Remove(wX+1,wY);
					obstacles.Remove(wX+1,wY+1);
				}

				if (!obstacles.GetSize()) return;   //quick return once everything's been handled
			}
		}
}

//*****************************************************************************
bool CDbRoom::RemovePressurePlateTile(
//Updates orb data for removal of a pressure plate tile, possibly
//splitting the pressure plate into multiple pressure plates.
//May be called either before or after changing the actual tile.
//
//Returns: true if the plate was just one tile and no longer exists
//Params:
	const UINT wX, const UINT wY) //(in) square to check from
{
	USHORT wPlateI = this->pressurePlateIndex.GetAt(wX,wY);
	if (!wPlateI || wPlateI > this->orbs.size())
	{
		ASSERT(!"Pressure plate not found");
		return false;
	}
	this->pressurePlateIndex.Remove(wX, wY);

	CTileMask tileMask( T_PRESSPLATE );
	vector<CCoordSet> regions;
	GetConnectedRegionsAround(wX, wY, tileMask, regions);
	COrbData* pPlate = this->orbs[wPlateI-1]; //base-1
	if (regions.size() <= 1)
	{
		//Still entirely connected (or completely gone) - just shrink it.
		pPlate->tiles.erase(wX,wY);
		if (regions.empty())
			return true;
		//Make sure the plate's main coordinates are still on it.
		if (pPlate->wX==wX && pPlate->wY==wY)
		{
			const CCoordSet& reg = regions.front();
			reg.first(pPlate->wX, pPlate->wY);
		}
	}
	else
	{
		//Reuse the old COrbData and index for the first connected region.
		COrbData& firstPlate = *pPlate;
		vector<CCoordSet>::const_iterator reg = regions.begin();
		firstPlate.tiles = *reg;
		reg->first(firstPlate.wX, firstPlate.wY);

		for (++reg; reg != regions.end(); ++reg)
		{
			//Copy agents, eType, and bActive:
			pPlate = new COrbData(firstPlate);
			pPlate->tiles = *reg;
			reg->first(pPlate->wX, pPlate->wY);
			this->orbs.push_back(pPlate);
			wPlateI = static_cast<USHORT>(this->orbs.size()); //base-1
			for (CCoordSet::const_iterator sq = reg->begin();
				 sq != reg->end(); ++sq)
				this->pressurePlateIndex.Add(sq->wX, sq->wY, wPlateI);
		}
	}
	return false;
}

//*****************************************************************************
void CDbRoom::RemoveYellowDoorTile(
//Updates orb data for removal of a yellow door tile.
//May be called either before or after changing the actual tile.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	const UINT wTile)  //(in) removed tile type (open or closed)
{
	ASSERT(bIsYellowDoor(wTile));

	vector<CCoordSet> regions;
	GetConnectedRegionsAround(wX, wY, CTileMask(wTile), regions);
	bool bDoorGone = regions.empty(); //1-square door removed

	CCoordSet oldDoor(wX,wY);
	vector<CCoordSet>::const_iterator reg;
	for (reg = regions.begin(); reg != regions.end(); ++reg)
		oldDoor += *reg;

	//Must examine all orb data even if the door is still in one piece,
	//in case something references the specific square removed.
	for (vector<COrbData*>::const_iterator o=orbs.begin(); o!=orbs.end(); ++o)
	{
		vector<COrbAgentData*>& agents = (*o)->agents;
		for (vector<COrbAgentData*>::const_iterator agentIter=agents.begin();
			 agentIter!=agents.end(); ++agentIter)
		{
			COrbAgentData& agent = **agentIter;
			if (oldDoor.has(agent.wX, agent.wY))
			{
				if (bDoorGone)
				{
					(*o)->DeleteAgent(*agentIter);
					break;
				}
				reg = regions.begin();
				reg->first(agent.wX, agent.wY);
				for (++reg; reg != regions.end(); ++reg)
				{
					COrbAgentData* pNewAgent = new COrbAgentData;
					reg->first(pNewAgent->wX, pNewAgent->wY);
					pNewAgent->action = agent.action;
					agents.push_back(pNewAgent);
				}
				break; //have fixed this orb/plate, and shouldn't ref same door again anyway
			}
		}
	}
}

//*****************************************************************************
bool CDbRoom::CropRegion(UINT& x1, UINT &y1, UINT &x2, UINT &y2) const
//Crops rectangular region from (x1,y1) to (x2,y2) inclusive to the room area.
//Returns: whether a non-empty cropped region is output
{
	if (int(x1) >= int(this->wRoomCols))
		return false;
	if (int(y1) >= int(this->wRoomRows))
		return false;

	if (int(x2) < 0)
		return false;
	if (int(y2) < 0)
		return false;

	if (int(x1) < 0)
		x1 = 0;
	if (int(y1) < 0)
		y1 = 0;

	if (x2 >= this->wRoomCols)
		x2 = this->wRoomCols-1;
	if (y2 >= this->wRoomRows)
		y2 = this->wRoomRows-1;

	return true;
}
