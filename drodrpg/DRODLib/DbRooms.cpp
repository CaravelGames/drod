// $Id: DbRooms.cpp 10216 2012-05-20 08:36:59Z skell $

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

//DbRooms.cpp
//Implementation of CDbRooms, CDbRoom, and other classes related to rooms.

#include "DbRooms.h"

#include "Combat.h"
#include "CurrentGame.h"
#include "Db.h"
#include "DbData.h"
#include "DbProps.h"
#include "Character.h"
#include "Seep.h"
#include "MonsterPiece.h"
#include "PlayerDouble.h"
#include "Serpent.h"
#include "Spider.h"
#include "Splitter.h"
#include "Stalwart.h"
#include "Swordsman.h"
#include "Zombie.h"
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

//*****************************************************************************
UINT readBpUINT(const BYTE* &pRead)
//Deserialize 1..5 bytes --> UINT
{
	ASSERT(*pRead); // should not be zero (indicating a negative number)
	UINT n = 0;
	for (;;)
	{
		n = (n << 7) + *pRead;
		if (*(pRead++) & 0x80)
			break;
	}

	return n - 0x80;
}

//*****************************************************************************
void writeBpUINT(char* &pWrite, UINT n)
//Serialize UINT --> 1..5 bytes
{
	int s = 7;
	while ((n >> s) && s < 32)
		s += 7;

	while (s)
	{
		s -= 7;
		BYTE b = BYTE((n >> s) & 0x7f);
		if (!s)
			b |= 0x80;
		*(pWrite++) = b;
	}
}

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
			CMonsterFactory mf;
			CMonster *pNew = mf.GetNewMonster((MONSTERTYPE)wMonsterType);
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

	//Include any designated floor mosaic image.
	if (pRoom->dwDataID && !bRef)
	{
		//Only save attached data if it's not a dangling reference.
		if (g_pTheDB->Data.Exists(pRoom->dwDataID))
			g_pTheDB->Data.ExportXML(pRoom->dwDataID, dbRefs, str);
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
//		str += PROPTAG(P_IsRequired);
//		str += INT32TOSTR(pRoom->bIsRequired);
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
/*
		//Add Halph/Slayer entrances to monster list.
		CCoordSet::const_iterator enters;
		for (enters=pRoom->halphEnters.begin(); enters!=pRoom->halphEnters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_HALPH, enters->wX, enters->wY, false);
		}
		for (enters=pRoom->slayerEnters.begin(); enters!=pRoom->slayerEnters.end(); ++enters)
		{
			ASSERT(pRoom->IsValidColRow(enters->wX, enters->wY));
			pRoom->AddNewMonster(M_SLAYER, enters->wX, enters->wY, false);
		}
*/

		CMonster *pMonster = pRoom->pFirstMonster;
		while (pMonster)
		{
			ASSERT(pRoom->IsValidColRow(pMonster->wX, pMonster->wY));
			pMonster->SetExtraVarsForExport();
			pMonster->ExportXML(str);
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
		if (dwRoomY == (UINT) p_RoomY(row) && dwRoomX == (UINT) p_RoomX(row))
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

	const UINT dwLevelCount = CDbBase::GetViewSize(V_Levels);
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
						if (static_cast<UINT>(pRoom->pszTSquares[wI]) == wTile)
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
									if (pRoom->pszTParams[wI] == wParam)
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
						if (static_cast<UINT>(pRoom->pszFSquares[wI]) == wTile)
							bFound = true;
						else {
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
/*
								case T_CHECKPOINT:
									if (pRoom->checkpoints.has(wX,wY))
										bFound = true;
								default: break;
*/
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
				string str = UnicodeToUTF8(wstr);
				str += "\n";
				CFiles f;
				f.AppendErrorLog(str.c_str());
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
CDbRoom::CDbRoom(const CDbRoom &Src)
	//Set pointers to NULL so Clear() won't try to delete them.
	: CDbBase()
	, pszOSquares(NULL), pszFSquares(NULL), pszTSquares(NULL), pszTParams(NULL)
	, pFirstMonster(NULL), pLastMonster(NULL)
	, pMonsterSquares(NULL)
	, pCurrentGame(NULL)
//Constructor.
{
/*
	for (int n=NumMovementTypes; n--; )
		this->pPathMap[n]=NULL;
*/
	SetMembers(Src);
}

//*****************************************************************************
CDbRoom::CDbRoom(const CDbRoom& Src, const bool copyGame)
//Set pointers to NULL so Clear() won't try to delete them.
	: CDbBase()
	, pszOSquares(NULL), pszFSquares(NULL), pszTSquares(NULL), pszTParams(NULL)
	, pFirstMonster(NULL), pLastMonster(NULL)
	, pMonsterSquares(NULL)
	, pCurrentGame(NULL)
	//Constructor.
{
	SetMembers(Src, true, copyGame);
}

//
//CDbRoom protected methods.
//

//*****************************************************************************
CDbRoom::CDbRoom()
	//Set pointers to NULL so Clear() won't try to delete them.
	: CDbBase()
	, pszOSquares(NULL), pszFSquares(NULL), pszTSquares(NULL), pszTParams(NULL)
	, pFirstMonster(NULL), pLastMonster(NULL)
	, pMonsterSquares(NULL)
	, pCurrentGame(NULL)
//Constructor.
{
/*	for (int n=0; n<NumMovementTypes; ++n)
		this->pPathMap[n]=NULL;
*/
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
CDbLevel * CDbRoom::GetLevel()
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
//	this->bIsRequired = (p_IsRequired(row) != 0);
	this->bIsSecret = (p_IsSecret(row) != 0);
	this->dwDataID = (UINT) (p_DataID(row));
	this->wImageStartX = (UINT) (p_ImageStartX(row));
	this->wImageStartY = (UINT) (p_ImageStartY(row));
	this->dwOverheadDataID = (UINT)(p_OverheadDataID(row));
	this->wOverheadImageStartX = (UINT)(p_OverheadImageStartX(row));
	this->wOverheadImageStartY = (UINT)(p_OverheadImageStartY(row));

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
bool CDbRoom::LoadMonstersLate()
//Loads monsters into a room record that was loaded with the "quick" option.
{
	ASSERT(this->bPartialLoad); //shouldn't call if all members were already loaded

	//Find record with matching room ID.
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(this->dwRoomID, V_Rooms, RoomsView);
	ASSERT(dwRoomI != ROW_NO_MATCH);

	c4_View MonstersView = p_Monsters(RoomsView[dwRoomI]);
	return LoadMonsters(MonstersView);
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
	static PrimaryKeyMap::iterator localID;
	switch (pType)
	{
		case P_RoomID:
		{
			this->dwRoomID = convertToUINT(str);
			if (!this->dwRoomID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.RoomIDMap.find(this->dwRoomID);
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
			this->dwLevelID = convertToUINT(str);
			if (!this->dwLevelID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.LevelIDMap.find(this->dwLevelID);
			if (localID == info.LevelIDMap.end())
				return MID_LevelNotFound;  //can't load a room w/o its level
			this->dwLevelID = localID->second;
			if (!this->dwLevelID)
			{
				//Records for this level are being ignored.  Don't save this room.
				bSaveRecord = false;
			}
			break;
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
		case P_StyleName:
			ASSERT(info.wVersion >= 301);
			Base64::decode(str, this->style);
			info.roomStyles.insert(this->style); //maintain set of room styles imported
			break;
//		case P_IsRequired:
//			this->bIsRequired = convertIntStrToBool(str);
//			break;
		case P_IsSecret:
			this->bIsSecret = convertIntStrToBool(str);
			break;
		case P_DataID:
			this->dwDataID = convertToUINT(str);
			if (this->dwDataID)
			{
				//Set to local ID.
				localID = info.DataIDMap.find(this->dwDataID);
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
/*
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
*/
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
					CMonsterFactory mf;
					ASSERT(!pImportMonster);
					pImportMonster = mf.GetNewMonster(M_MIMIC);
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
/*
				case P_IsFirstTurn:
					pImportMonster->bIsFirstTurn = (convertToUINT(str) != 0);
					break;
				case P_ProcessSequence:
					pImportMonster->wProcessSequence = convertToUINT(str);
					break;
*/
				case P_ExtraVars:
				{
					BYTE *data;
					Base64::decode(str,data);
					const CDbPackedVars vars = (const BYTE*)data;
					delete[] data;
					if (pImportMonster->wType == M_CHARACTER)
					{
						//Can't dynamically cast pImportMonster to a CCharacter
						CMonsterFactory mf;
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*,
								mf.GetNewMonster(M_CHARACTER));
						pCharacter->ExtraVars = vars;
						const MESSAGE_ID val = pCharacter->ImportSpeech(info);
						if (val != MID_ImportSuccessful)
							return val;
						pImportMonster->ExtraVars = pCharacter->ExtraVars;
						delete pCharacter;
					} else {
						ASSERTP(vars.Empty(), "Import error: Non character monster has ExtraVars data");
					}
					break;
				}
				case P_End:
					//Finish processing
					if (info.typeBeingImported == CImportInfo::LanguageMod)
						delete pImportMonster; //don't re-add existing room members
					else
						LinkMonster(pImportMonster);
/*
						, !DoesMonsterEnterRoomLater(
							pImportMonster->wX, pImportMonster->wY,
							pImportMonster->wType));
*/
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
/*
				//Backwards compatibility:
				//1.6 Format
            case P_LevelID:
					//Change this LevelID to the EntranceID for the level's main entrance.
					//IDs will be matched to local ones later on completion of import
					pImportExit->dwEntranceID = convertToUINT(str);
					if (pImportExit->dwEntranceID)
						pImportExit->dwEntranceID += DROD1_6EXITFORMAT;
					break;
*/

				default:
					delete pImportExit;
					pImportExit = NULL;
					return MID_FileCorrupted;
			}
			break;
/*
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
*/
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
/*
void CDbRoom::UpdatePathMapAt(const UINT wX, const UINT wY)
{
	ASSERT(IsValidColRow(wX,wY));
	for (int eMovement=0; eMovement<NumMovementTypes; ++eMovement)
		if (this->pPathMap[eMovement])
		{
			const bool bIsPathMapObstacle = DoesSquareContainPathMapObstacle(wX,
					wY, (MovementType)eMovement);
			this->pPathMap[eMovement]->SetSquare(wX, wY, bIsPathMapObstacle);
		}
}

//-****************************************************************************
void CDbRoom::RecalcStationPaths()
//Tells stations their path maps must be updated before next use.
{
	for (UINT i=this->stations.size(); i--; )
		this->stations[i]->RecalcPathmap();
}
*/

//*****************************************************************************
const RoomObject* CDbRoom::GetPushedObjectAt(const UINT wX, const UINT wY) const
{
	for (set<const RoomObject*>::const_iterator it = this->pushed_objects.begin();
		it != this->pushed_objects.end(); it++)
	{
		const RoomObject& obj = **it;
		if (obj.wX == wX && obj.wY == wY)
			return *it;
	}
	return NULL;
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
//Only call on o-, f-, and t-layer values, and not t-params or monsters, etc.
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

		//Tunnels.
		case T_TUNNEL_N: if (!bHoriz) wSquare = T_TUNNEL_S; break;
		case T_TUNNEL_S: if (!bHoriz) wSquare = T_TUNNEL_N; break;
		case T_TUNNEL_E: if (bHoriz) wSquare = T_TUNNEL_W; break;
		case T_TUNNEL_W: if (bHoriz) wSquare = T_TUNNEL_E; break;

		default:
			//Reverse stair type when turned upside down.
			if (!bHoriz && bIsStairs(wSquare))
				wSquare = (wSquare == T_STAIRS ? T_STAIRS_UP : T_STAIRS);
		break;
	}
}

//*****************************************************************************
void CDbRoom::RotateTileC(UINT &wTile) const
//When rotating the room, some objects need to be modified.
//Only call on o-, f-, and t-layer values, and not t-params or monsters, etc.
{
	switch (wTile)
	{
		//Arrows.
		case T_ARROW_N:  wTile = T_ARROW_E;  break;
		case T_ARROW_NE: wTile = T_ARROW_SE; break;
		case T_ARROW_E:  wTile = T_ARROW_S;  break;
		case T_ARROW_SE: wTile = T_ARROW_SW; break;
		case T_ARROW_S:  wTile = T_ARROW_W;  break;
		case T_ARROW_SW: wTile = T_ARROW_NW; break;
		case T_ARROW_W:  wTile = T_ARROW_N;  break;
		case T_ARROW_NW: wTile = T_ARROW_NE; break;

		//Tunnels.
		case T_TUNNEL_N: wTile = T_TUNNEL_E; break;
		case T_TUNNEL_S: wTile = T_TUNNEL_W; break;
		case T_TUNNEL_E: wTile = T_TUNNEL_S; break;
		case T_TUNNEL_W: wTile = T_TUNNEL_N; break;

		case T_BRIDGE_H: wTile = T_BRIDGE_V; break;
		case T_BRIDGE_V: wTile = T_BRIDGE_H; break;

		default:
			//Rotate stair type?
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
	for (wY=this->wRoomRows; wY--; )
	{
		//Reflect t-layer directional params first.
		for (wX=this->wRoomCols; wX--; )
			tParam[wX] = GetTParam(wX, wY);  //need to make a static copy
		for (wX=0; wX<this->wRoomCols; )
		{
			if (GetTSquare(wX, wY) != T_OBSTACLE) {++wX; continue;}
			const UINT obType = calcObstacleType(tParam[wX]);
			ASSERT(obType);
			wRefX = wX;//starting point of obstacle
			wSize = 0; //dimension of obstacle (past 1)
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
			wRefX = (this->wRoomCols-1) - wX;
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
			this->tileLights.Set(wX, wY, wSquare2);
			this->tileLights.Set(wRefX, wY, wSquare);

			wSquare = this->pressurePlateIndex.GetAt(wX, wY);
			wSquare2 = this->pressurePlateIndex.GetAt(wRefX, wY);
			this->pressurePlateIndex.Set(wX, wY, wSquare2);
			this->pressurePlateIndex.Set(wRefX, wY, wSquare);

			wSquare = GetFSquare(wX, wY);
			wSquare2 = GetFSquare(wRefX, wY);
			ReflectSquare(true, wSquare);
			ReflectSquare(true, wSquare2);
			this->pszFSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszFSquares[ARRAYINDEX(wRefX, wY)] = wSquare;

			wSquare = GetTSquare(wX, wY);
			wSquare2 = GetTSquare(wRefX, wY);
			ReflectSquare(true, wSquare);
			ReflectSquare(true, wSquare2);
			this->pszTSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszTSquares[ARRAYINDEX(wRefX, wY)] = wSquare;

			wSquare = this->coveredTSquares.GetAt(wX, wY);
			wSquare2 = this->coveredTSquares.GetAt(wRefX, wY);
			this->coveredTSquares.Set(wX, wY, wSquare2);
			this->coveredTSquares.Set(wRefX, wY, wSquare);

			wSquare = GetTParam(wX, wY);
			wSquare2 = GetTParam(wRefX, wY);
			this->pszTParams[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszTParams[ARRAYINDEX(wRefX, wY)] = wSquare;
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
	for (vector<COrbData*>::const_iterator pOrb=this->orbs.begin();
			pOrb != this->orbs.end(); ++pOrb)
	{
		COrbData& orb = *(*pOrb);
		orb.wX = this->wRoomCols-1 - orb.wX;
		//Reflect orb agents.
		for (vector<COrbAgentData*>::const_iterator agent=orb.agents.begin();
				agent != orb.agents.end(); ++agent)
		{
			(*agent)->wX = this->wRoomCols-1 - (*agent)->wX;
		}
		//Reflect pressure plate sets.
		CCoordSet coords;
		for (CCoordSet::const_iterator coord=orb.tiles.begin(); coord!=orb.tiles.end(); ++coord)
			coords.insert(this->wRoomCols-1 - coord->wX, coord->wY);
		orb.tiles = coords;
	}

	//Reflect scrolls.
	for (vector<CScrollData*>::const_iterator scroll=this->Scrolls.begin();
			scroll != this->Scrolls.end(); ++scroll)
	{
		(*scroll)->wX = this->wRoomCols-1 - (*scroll)->wX;
	}

	//Reflect exits.
	for (vector<CExitData*>::const_iterator pStair=this->Exits.begin();
			pStair != this->Exits.end(); ++pStair)
	{
		CExitData& stair = *(*pStair);
		const UINT oldwLeft = stair.wLeft;
		stair.wLeft = this->wRoomCols-1 - stair.wRight;
		stair.wRight = this->wRoomCols-1 - oldwLeft;
	}

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
	for (wX=this->wRoomCols; wX--; )
	{
		//Reflect t-layer directional params first.
		for (wY=this->wRoomRows; wY--; )
			tParam[wY] = GetTParam(wX, wY);  //need to make a static copy
		for (wY=0; wY<this->wRoomRows; )
		{
			if (GetTSquare(wX, wY) != T_OBSTACLE) {++wY; continue;}
			const UINT obType = calcObstacleType(tParam[wY]);
			ASSERT(obType);
			wRefY = wY;//starting point of obstacle
			wSize = 0; //dimension of obstacle (past 1)
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
			wRefY = (this->wRoomRows-1) - wY;
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
			this->tileLights.Set(wX, wY, wSquare2);
			this->tileLights.Set(wX, wRefY, wSquare);

			wSquare = this->pressurePlateIndex.GetAt(wX, wY);
			wSquare2 = this->pressurePlateIndex.GetAt(wX, wRefY);
			this->pressurePlateIndex.Set(wX, wY, wSquare2);
			this->pressurePlateIndex.Set(wX, wRefY, wSquare);

			wSquare = GetFSquare(wX, wY);
			wSquare2 = GetFSquare(wX, wRefY);
			ReflectSquare(false, wSquare);
			ReflectSquare(false, wSquare2);
			this->pszFSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszFSquares[ARRAYINDEX(wX, wRefY)] = wSquare;

			wSquare = GetTSquare(wX, wY);
			wSquare2 = GetTSquare(wX, wRefY);
			ReflectSquare(false, wSquare);
			ReflectSquare(false, wSquare2);
			this->pszTSquares[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszTSquares[ARRAYINDEX(wX, wRefY)] = wSquare;

			wSquare = this->coveredTSquares.GetAt(wX, wY);
			wSquare2 = this->coveredTSquares.GetAt(wX, wRefY);
			this->coveredTSquares.Set(wX, wY, wSquare2);
			this->coveredTSquares.Set(wX, wRefY, wSquare);

			wSquare = GetTParam(wX, wY);
			wSquare2 = GetTParam(wX, wRefY);
			this->pszTParams[ARRAYINDEX(wX, wY)] = wSquare2;
			this->pszTParams[ARRAYINDEX(wX, wRefY)] = wSquare;
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
	for (vector<COrbData*>::const_iterator pOrb=this->orbs.begin();
			pOrb != this->orbs.end(); ++pOrb)
	{
		COrbData& orb = *(*pOrb);
		orb.wY = (this->wRoomRows-1) - orb.wY;
		//Reflect orb agents.
		for (vector<COrbAgentData*>::const_iterator agent=orb.agents.begin();
				agent != orb.agents.end(); ++agent)
		{
			(*agent)->wY = (this->wRoomRows-1) - (*agent)->wY;
		}
		//Reflect pressure plate sets.
		CCoordSet coords;
		for (CCoordSet::const_iterator coord=orb.tiles.begin(); coord!=orb.tiles.end(); ++coord)
			coords.insert(coord->wX, (this->wRoomRows-1) - coord->wY);
		orb.tiles = coords;
	}

	//Reflect scrolls.
	for (vector<CScrollData*>::const_iterator scroll=this->Scrolls.begin();
			scroll != this->Scrolls.end(); ++scroll)
	{
		(*scroll)->wY = (this->wRoomRows-1) - (*scroll)->wY;
	}

	//Reflect exits.
	for (vector<CExitData*>::const_iterator pStair=this->Exits.begin();
			pStair != this->Exits.end(); ++pStair)
	{
		CExitData& stair = *(*pStair);
		const UINT oldwTop = stair.wTop;
		stair.wTop = (this->wRoomRows-1) - stair.wBottom;
		stair.wBottom = (this->wRoomRows-1) - oldwTop;
	}

	//ignore in-game vars
}

//*****************************************************************************
void CDbRoom::RotateClockwise()
//Rotates the room 90 degrees clockwise.
{
	//Rotate room tiles.
	const UINT wRoomArea = CalcRoomArea();
	UINT wX, wY;

	vector<UINT> oTiles(wRoomArea), coveredOTiles(wRoomArea),
		tileLights(wRoomArea), pressurePlateIndices(wRoomArea),
		fTiles(wRoomArea),
		tTiles(wRoomArea), coveredTTiles(wRoomArea);
	vector<UINT> tParams(wRoomArea);

	//Rotate obstacle boundary markers.
	CCoordSet ignore; //tiles to not process again
	UINT wOriginX, wTempX, wTempY, wXSize, wYSize;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; )
		{
			if (GetTSquare(wX, wY) != T_OBSTACLE || ignore.has(wX,wY))
			{
				++wX;
				continue;
			}

			const UINT tParam = GetTParam(wX, wY);

			//Process an obstacle when encountering its origin tile.
			if (!(tParam & (OBSTACLE_TOP|OBSTACLE_LEFT)))
			{
				++wX;
				continue;
			}

			const UINT obType = calcObstacleType(tParam);
			ASSERT(obType);
			wOriginX = wX;//origin of obstacle
			wXSize = wYSize = 1; //dimensions of obstacle
			while (++wX < this->wRoomCols) //determine width
			{
				if (GetTSquare(wX, wY) != T_OBSTACLE) break;
				const UINT tParam2 = GetTParam(wX, wY);
				if (bObstacleLeft(tParam2)) break;
				if (calcObstacleType(tParam2) != obType) break;
				++wXSize;
			}
			wTempY = wY;
			while (++wTempY < this->wRoomRows) //determine height
			{
				if (GetTSquare(wOriginX, wTempY) != T_OBSTACLE) break;
				const UINT tParam2 = GetTParam(wOriginX, wTempY);
				if (bObstacleTop(tParam2)) break;
				if (calcObstacleType(tParam2) != obType) break;
				++wYSize;
			}

			//Unmark top.  It will become the right side of the obstacle.
			for (wTempX=0; wTempX<wXSize; ++wTempX)
			{
				SetTParam(wOriginX+wTempX, wY,
						GetTParam(wOriginX+wTempX, wY) & ~OBSTACLE_TOP);
				ignore.insert(wOriginX+wTempX, wY);
			}

			//Left side becomes top.
			for (wTempY=0; wTempY<wYSize; ++wTempY)
			{
				SetTParam(wOriginX, wY+wTempY,
						(GetTParam(wOriginX, wY+wTempY) & ~OBSTACLE_LEFT) | OBSTACLE_TOP);
				ignore.insert(wOriginX, wY+wTempY);
			}

			//Bottom side becomes left.
			for (wTempX=0; wTempX<wXSize; ++wTempX)
			{
				SetTParam(wOriginX+wTempX, wY+wYSize-1,
					GetTParam(wOriginX+wTempX, wY+wYSize-1) | OBSTACLE_LEFT);
				ignore.insert(wOriginX+wTempX, wY+wYSize-1);
			}
		}

	//Rotate room arrays.
	UINT wNewIndex;
	for (wY=this->wRoomRows; wY--; )
		for (wX=this->wRoomCols; wX--; )
		{
			wNewIndex = ARRAYINDEX((this->wRoomRows-1) - wY, wX);
			oTiles[wNewIndex] = GetOSquare(wX, wY);
			RotateTileC(oTiles[wNewIndex]);
			coveredOTiles[wNewIndex] = this->coveredOSquares.GetAt(wX, wY);
			tileLights[wNewIndex] = this->tileLights.GetAt(wX, wY);
			pressurePlateIndices[wNewIndex] = this->pressurePlateIndex.GetAt(wX, wY);
			fTiles[wNewIndex] = GetFSquare(wX, wY);
			RotateTileC(fTiles[wNewIndex]);
			tTiles[wNewIndex] = GetTSquare(wX, wY);
			RotateTileC(tTiles[wNewIndex]);
			coveredTTiles[wNewIndex] = this->coveredTSquares.GetAt(wX, wY);
			tParams[wNewIndex] = GetTParam(wX, wY);
		}

	//Replace old arrays with rotated copy.
	UINT wIndex=0;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX, ++wIndex)
		{
			this->pszOSquares[wIndex] = oTiles[wIndex];
			this->coveredTSquares.Set(wX, wY, coveredOTiles[wIndex]);
			this->tileLights.Set(wX, wY, tileLights[wIndex]);
			this->pressurePlateIndex.Set(wX, wY, pressurePlateIndices[wIndex]);
			this->pszFSquares[wIndex] = fTiles[wIndex];
			this->pszTSquares[wIndex] = tTiles[wIndex];
			this->coveredTSquares.Set(wX, wY, coveredTTiles[wIndex]);
			this->pszTParams[wIndex] = tParams[wIndex];
		}

	//Rotate platforms.
	for (vector<CPlatform*>::const_iterator platformIter = this->platforms.begin();
			platformIter != this->platforms.end(); ++platformIter)
	{
		CPlatform *pPlatform = *platformIter;
		pPlatform->RotateClockwise(this);
	}

	//Rotate monsters.
	UINT wNewX;
	for (UINT i=wRoomArea; i--; )
		this->pMonsterSquares[i] = NULL; //reset monster array
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pMonster->RotateClockwise(this); //places monsters in pMonsterSquares
		pMonster = pMonster->pNext;
	}

	//Rotate orbs.
	for (vector<COrbData*>::const_iterator pOrb=this->orbs.begin();
			pOrb != this->orbs.end(); ++pOrb)
	{
		COrbData& orb = *(*pOrb);
		wNewX = (this->wRoomRows-1) - orb.wY;
		orb.wY = orb.wX;
		orb.wX = wNewX;
		//Rotate orb agents.
		for (vector<COrbAgentData*>::const_iterator pAgent=orb.agents.begin();
				pAgent != orb.agents.end(); ++pAgent)
		{
			COrbAgentData& agent = *(*pAgent);
			wNewX = (this->wRoomRows-1) - agent.wY;
			agent.wY = agent.wX;
			agent.wX = wNewX;
		}
		//Rotate pressure plate sets.
		CCoordSet coords;
		for (CCoordSet::const_iterator coord=orb.tiles.begin(); coord!=orb.tiles.end(); ++coord)
		{
			wNewX = (this->wRoomRows-1) - coord->wY;
			coords.insert(wNewX, coord->wX);
		}
		orb.tiles = coords;
	}

	//Rotate scrolls.
	for (vector<CScrollData*>::const_iterator pScroll=this->Scrolls.begin();
			pScroll != this->Scrolls.end(); ++pScroll)
	{
		CScrollData& scroll = *(*pScroll);
		wNewX = (this->wRoomRows-1) - scroll.wY;
		scroll.wY = scroll.wX;
		scroll.wX = wNewX;
	}

	//Rotate exits.
	for (vector<CExitData*>::const_iterator pStair=this->Exits.begin();
			pStair != this->Exits.end(); ++pStair)
	{
		CExitData& stair = *(*pStair);
		const UINT w = stair.wRight - stair.wLeft;
		const UINT h = stair.wBottom - stair.wTop;

		wNewX = (this->wRoomRows-1) - stair.wBottom;
		stair.wTop = stair.wLeft;
		stair.wLeft = wNewX;

		stair.wRight = stair.wLeft + w; //retain same dimensions
		stair.wBottom = stair.wTop + h;
	}

	//ignore in-game vars
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
/*
void CDbRoom::SetHalphSlayerEntrance()
//Determine whether player entered the room on a tile where one of these characters
//will enter later.  If so, retain this coordinate in the list.
{
	const bool bHalphEnters = this->halphEnters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	const bool bSlayerEnters = this->slayerEnters.has(
			this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	ASSERT(!(bHalphEnters && bSlayerEnters));

	this->halphEnters.clear();
	this->slayerEnters.clear();
	if (bHalphEnters)
		this->halphEnters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
	if (bSlayerEnters)
		this->slayerEnters.insert(this->pCurrentGame->wStartRoomX, this->pCurrentGame->wStartRoomY);
}

//-****************************************************************************
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
				wX, wY, dwPathThroughObstacleCost);
	}
	else
		this->pPathMap[eMovement]->SetTarget(wX, wY);
	for (UINT x = 0; x < this->wRoomCols; ++x)
		for (UINT y = 0; y < this->wRoomRows; ++y)
		{
			const bool obstacle = DoesSquareContainPathMapObstacle(x, y, eMovement);
			this->pPathMap[eMovement]->SetSquare(x, y, obstacle);
		}
}

//-**************************************************************************************
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
	CreatePathMap(wSX, wSY, (MovementType)0);

	//Pathmaps which already exist will be updated with wSX, wSY as target.
	if (IsBrainPresent())
		for (int n=1; n<NumMovementTypes; ++n)
			if (MonsterWithMovementTypeExists((MovementType)n))
				CreatePathMap(wSX, wSY, (MovementType)n);
}

//-**************************************************************************************
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
*/

//*****************************************************************************
bool CDbRoom::CanPlayerMoveOnThisElement(
	const UINT wAppearance, const UINT wTileNo, const bool bRaisedSrc)
const
//Returns: whether player in current role can move on this o-layer object
{
	switch (wAppearance)
	{
		case M_SEEP:
			if (!(bIsWall(wTileNo) || bIsCrumblyWall(wTileNo) || bIsDoor(wTileNo) || bIsDiggableBlock(wTileNo)))
				return false;
		break;
		case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
			if (!(bIsPit(wTileNo) || bIsWater(wTileNo) ||
					bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) ||
					bIsStairs(wTileNo) || bIsTunnel(wTileNo) || bIsPlatform(wTileNo) ||
					(bIsDoor(wTileNo) && bRaisedSrc)))
				return false;
		break;
		case M_WATERSKIPPER:
			if (!bIsWater(wTileNo) || wTileNo == T_PLATFORM_W)
				return false;
		break;
		default:
			if (!(bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) ||
					bIsStairs(wTileNo) || bIsTunnel(wTileNo) || bIsPlatform(wTileNo) ||
					(bIsDoor(wTileNo) && bRaisedSrc)))
				return false;
		break;
	}

	return true;
}

//*****************************************************************************
bool CDbRoom::CanJumpTo(
//Returns: Whether the destination room tile is free for the player to jump here
//
//Params:
	const UINT wFromX, const UINT wFromY,    //(in)   Source tile (i.e. middle tile of the two-tile jump)
	const UINT wX, const UINT wY,    //(in)   Destination tile to check.
	const bool bFromRaisedTile,      //whether jumping from a raised tile (e.g., atop a door)
	const bool bFromCrate)           //whether jumping from atop a crate
const
{
	if (!IsValidColRow(wX, wY))
		return false;

	//Look for t-square obstacle.
	const UINT wTTileNo = GetTSquare(wX, wY);
	if (!(wTTileNo == T_EMPTY ||
			wTTileNo == T_FUSE || wTTileNo == T_SCROLL || wTTileNo == T_TOKEN ||
			wTTileNo == T_KEY || bIsPowerUp(wTTileNo) || bIsEquipment(wTTileNo) ||
			wTTileNo == T_CRATE))
		return false;

	//Look for f-square obstacle.
	for (UINT i=2; i--; )
	{
		const UINT wTileNo = i==0 ? GetFSquare(wX, wY) : GetFSquare(wFromX, wFromY);
		switch (wTileNo)
		{
			case T_NODIAGONAL:
				if (wFromX != wX && wFromY != wY)
					return false;
			break;
			default:
				if (bIsArrow(wTileNo))
				{
					if (bIsArrowObstacle(wTileNo, nGetO(wX-wFromX,wY-wFromY)))
						return false;
				}
			break;
		}
	}

	//Look for o-square obstacle.
	const UINT wTileNo = GetOSquare(wX, wY);
	if (!(bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) || bIsPlatform(wTileNo) ||
			bIsTunnel(wTileNo)))
	{
		//Can jump onto a closed door...
		if (!bIsDoor(wTileNo))
			return false;

		//...from another raised tile or from a crate...
		if (!(bFromRaisedTile || bFromCrate))
			return false;

		//...but not from a crate at floor level onto a crate atop a closed door.
		if (!bFromRaisedTile && wTTileNo == T_CRATE)
			return false;
	}

	//Is there a monster in the square?
	if (GetMonsterAtSquare(wX, wY) != NULL)
		return false;

	//The player should not be in the destination square already.
	ASSERT(this->pCurrentGame);
	ASSERT(!this->pCurrentGame->IsPlayerAt(wX, wY));

	//No obstacle.
	return true;
}

//*****************************************************************************
bool CDbRoom::CanPushTo(
//Can a t-layer object be pushed here?
//
//Returns:
//True if yes, false if not.
//
//Params:
		const UINT wFromX, const UINT wFromY,    //(in)   Source square.
		const UINT wX, const UINT wY)    //(in)   Destination square to check.
const
{
	if (!IsValidColRow(wX, wY))
		return false;

	//Object can only be pushed once per turn process
	if (WasObjectPushedThisTurn(wFromX, wFromY))
		return false;

	//Look for t-square obstacle.
	UINT wTileNo = GetTSquare(wX, wY);
	if (!(wTileNo == T_EMPTY ||
			wTileNo == T_FUSE || wTileNo == T_SCROLL || wTileNo == T_TOKEN ||
			bIsMap(wTileNo) || wTileNo == T_KEY || bIsEquipment(wTileNo) || wTileNo == T_MIST))
		return false;

	//Look for f-square obstacle.
	for (UINT i=2; i--; )
	{
		wTileNo = i==0 ? GetFSquare(wX, wY) : GetFSquare(wFromX, wFromY);
		switch (wTileNo)
		{
			case T_NODIAGONAL:
				if (wFromX != wX && wFromY != wY)
					return false;
			break;
			default:
				if (bIsArrow(wTileNo))
				{
					if (bIsArrowObstacle(wTileNo, nGetO(wX-wFromX,wY-wFromY)))
						return false;
				}
			break;
		}
	}

	const bool bRaisedSrc = bIsElevatedTile(GetOSquare(wFromX, wFromY));

	//Look for o-square obstacle.
	wTileNo = GetOSquare(wX, wY);
	if (!(bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) || bIsPlatform(wTileNo) ||
			bIsPit(wTileNo) || bIsWater(wTileNo) ||
			(bIsDoor(wTileNo) && bRaisedSrc)))
		return false;

	//Is there a monster in the square?
	if (GetMonsterAtSquare(wX, wY) != NULL)
		return false;

	//Is the player in the square?
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->IsPlayerAt(wX, wY))
		return false;

	//No obstacle.
	return true;
}

//*****************************************************************************
/*
bool CDbRoom::DoesMonsterEnterRoomLater(
	const UINT wX, const UINT wY, const UINT wMonsterType) const
//Returns: whether a monster of indicating type starting at (x,y) will not be
//in the room on entrance, but will enter it later.
{
	return (wMonsterType == M_HALPH || wMonsterType == M_SLAYER) &&
		(wX==0 || wX == this->wRoomCols-1 || wY==0 || wY == this->wRoomRows-1);
}
*/

//*****************************************************************************
bool CDbRoom::DoesSquareContainPlayerObstacle(
//Does a square contain an obstacle to player movement?
//
//Params:
	const UINT wX, const UINT wY,    //(in)   Destination square to check.
	const UINT wO,          //(in)   Direction of movement onto square.
	const bool bRaisedSrc,  //whether tile coming from is raised
	const bool bCrateSrc, const bool bAllowCrateClimbing) //set when allowing crate-climbing movements [default=false, false]
//
//Returns:
//True if it does, false if not.
const
{
	ASSERT(IsValidColRow(wX, wY));

	//Look for t-square obstacle.
	const UINT wTTileNo = GetTSquare(wX, wY);
	if ( !(wTTileNo == T_EMPTY || wTTileNo == T_SCROLL || bIsPowerUp(wTTileNo) ||
			wTTileNo == T_FUSE || wTTileNo == T_TOKEN || wTTileNo == T_KEY ||
			bIsEquipment(wTTileNo) || bIsMap(wTTileNo) || bIsShovel(wTTileNo)))
	{
		if (wTTileNo == T_CRATE && bAllowCrateClimbing) {
			//movement is allowed
		} else {
			return true;
		}
	}

	if (bIsArrowObstacle(GetFSquare(wX, wY), wO))
		return true;

	//Look for o-square obstacle.
	//What is considered an obstacle depends on the player role.
	const UINT wAppearance = this->pCurrentGame->pPlayer->wAppearance;
	const UINT wOTileNo = GetOSquare(wX, wY);
	if (!CanPlayerMoveOnThisElement(wAppearance, wOTileNo, bRaisedSrc)) {
		if (bAllowCrateClimbing && bIsDoor(wOTileNo) && bCrateSrc) {
			//allowed, if not from a crate at floor level onto a crate atop a closed door.
			if (!bRaisedSrc && wTTileNo == T_CRATE)
				return true;
		} else {
			return true;
		}
	}

	//Is there a monster in the square?
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster &&
			!this->pCurrentGame->pPlayer->IsInvisible()) //invisible player can step through monsters
		return true;

	//No obstacle.
	return false;
}

//*****************************************************************************
bool CDbRoom::DoesSquarePreventDiagonal(
//Does a 4-connected space prevent diagonal movement?
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
	if (!IsValidColRow(wX + dx, wY + dy))
		return false;  //can exit room with a diagonal move
	return GetFSquare(wX + dx, wY + dy) == T_NODIAGONAL;
}

//*****************************************************************************
bool CDbRoom::DoesSquareContainTeleportationObstacle(
	// Checks if an entity of given type can teleport to this position.
	// Ignores monster layer, as player can step through monsters in certain states, e.g., when invisible
	//
	//Params:
	const UINT wX, const UINT wY, //(in)   Square to check
	const UINT wIdentity)   //(in)   Identity of the teleported entity
	//
	//Returns:
	//True if a teleportation-preventing tile is present on the target square, otherwise false
	const
{
	const UINT tTile = GetTSquare(wX, wY);
	switch (tTile) {
	case T_TAR:
		if (wIdentity != M_TARMOTHER) {
			return true;
		}
		break;
	case T_MUD:
		if (wIdentity != M_MUDMOTHER) {
			return true;
		}
		break;
	case T_GEL:
		if (wIdentity != M_GELMOTHER) {
			return true;
		}
		break;
	case T_BRIAR_DEAD:
	case T_BRIAR_LIVE:
	case T_BRIAR_SOURCE:
	case T_LIGHT:
		return true;
	}

	return false;
}

//*****************************************************************************
void CDbRoom::ExpandBriars(CCueEvents& CueEvents)
//Process one iteration of briar growth.
{
	this->briars.process(CueEvents);
}

//*****************************************************************************
void CDbRoom::ExpandMist(CCueEvents& CueEvents)
//Process one iteration of mist growth.
{
	CCoordSet ventConnectedMist;
	CCoordSet expandTo;
	UINT wX, wY;

	//Find all mist connected to vents
	for (CCoordSet::const_iterator iter = this->mistVents.begin();
		iter != this->mistVents.end(); ++iter) {
		wX = iter->wX;
		wY = iter->wY;
		UINT tTile = GetTSquare(wX, wY);

		if (CanExpandMist(wX, wY)) {
			expandTo.insert(wX, wY); //Always expand to clear vents
		} else if (tTile == T_MIST && !ventConnectedMist.has(wX, wY)) {
			//If this vent isn't already connected to another, find connected mist
			CCoordSet localMist;
			GetConnectedMistTiles(wX, wY, localMist);
			ventConnectedMist += localMist;
		}
	}

	//Find all tiles orthogonally adjecent to connected mist that allow expansion
	while (ventConnectedMist.pop_first(wX, wY)) {
		if (CanExpandMist(wX + 1, wY)) {
			expandTo.insert(wX + 1, wY);
		}
		if (CanExpandMist(wX - 1, wY)) {
			expandTo.insert(wX - 1, wY);
		}
		if (CanExpandMist(wX, wY + 1)) {
			expandTo.insert(wX, wY + 1);
		}
		if (CanExpandMist(wX, wY - 1)) {
			expandTo.insert(wX, wY - 1);
		}
	}

	while (expandTo.pop_first(wX, wY)) {
		Plot(wX, wY, T_MIST);
	}
}

//*****************************************************************************
bool CDbRoom::CanExpandMist(const UINT wX, const UINT wY) const
//Returns: If mist can expand onto this tile
{
	if (!IsValidColRow(wX, wY))
		return false;

	UINT oTile = GetOSquare(wX, wY);
	if (bIsSolidOTile(oTile) || oTile == T_HOT)
		return false;

	if (bIsArrow(GetFSquare(wX, wY)))
		return false;

	return GetTSquare(wX, wY) == T_EMPTY;
}

//*****************************************************************************
CCharacter* CDbRoom::GetCharacterWithScriptID(const UINT scriptID)
//Returns: character monster with indicated (unique) scriptID, or NULL if not found
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL;
			pMonster = pMonster->pNext)
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->dwScriptID == scriptID)
				return pCharacter; //found it
		}
	return NULL; //not found
}


//*****************************************************************************
bool CDbRoom::HasClosedDoors() const
//Returns: whether there are any closed doors in the room
{
	for (UINT i=CalcRoomArea(); i--; )
	{
		switch (this->pszOSquares[i])
		{
			//Closed door types.
			case T_DOOR_Y: case T_DOOR_G:
			case T_DOOR_C: case T_DOOR_R:
			case T_DOOR_B:
			case T_DOOR_MONEY:
				return true;
			default: break;
		}
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::HasCombatableMonsters() const
//Returns: whether there are any combatable monsters in the room
{
	for (CMonster *pMonster = this->pFirstMonster ; pMonster != NULL;
			pMonster = pMonster->pNext)
	{
		if (pMonster->IsCombatable())
			return true;
	}
	return false;
}

//*****************************************************************************
bool CDbRoom::HasGrabbableItems() const
//Returns: whether there are any instant use items the player can pick up in the room
{
	for (UINT i=CalcRoomArea(); i--; )
	{
		const char t = this->pszTSquares[i];
		//Health, power-ups, keys, map, shovels.
		if (bIsPowerUp(t) || t == T_KEY || bIsMap(t) || bIsShovel(t))
			return true;
	}

	for (CMonster* pMonster = this->pFirstMonster; pMonster != NULL;
		pMonster = pMonster->pNext)
	{
		//Monster flagged as being a collectable treasure
		if (pMonster->IsMinimapTreasure())
			return true;
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::TunnelGetExit(
//Find tunnel exit.
//
//Params:
	const UINT wStartX, const UINT wStartY,	//(in) tunnel entrance
	const int dx, const int dy,	//(in) tunnel direction
	UINT& wX, UINT& wY)	//(out) location of tunnel exit
const
{
	wX = wStartX;
	wY = wStartY;

	while (true)
	{
		wX += dx;
		wY += dy;

		//If search goes off the room edge, wrap to other side.
		if (!IsValidColRow(wX,wY))
		{
			if (wX >= this->wRoomCols)
			{
				//Wrap horizontally.
				wX = wX == this->wRoomCols ? 0 : this->wRoomCols-1;
			} else {
				//Wrap vertically.
				wY = wY == this->wRoomRows ? 0 : this->wRoomRows-1;
			}
		}

		if (wStartX == wX && wStartY == wY)
			return false;   //Found no other tunnel -- exit from same one.

		if (bIsTunnel(GetOSquare(wX,wY)))
		{
			//Found a different tunnel exit -- place player here if open.
			const UINT tTile = GetTSquare(wX,wY);
			if (IsMonsterSwordAt(wX,wY) ||
					bIsTar(tTile) || bIsBriar(tTile) ||
					GetMonsterAtSquare(wX,wY) != NULL)
			{
				//Tunnel exit is blocked -- can't use tunnel.
				wX = wStartX;
				wY = wStartY;
				return false;
			}
			return true;
		}
	}
}

//*****************************************************************************
void CDbRoom::FixUnstableTar(CCueEvents &CueEvents)
//Remove unstable tarstuff.
//Tarstuff remains under any monster situated on a tarstuff tile
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
					if (pMonster)
						continue; //allow unstable tarstuff to remain under a mother (or any monster on tar)
					DestroyTar(wX, wY, CueEvents);
					bStable = false;
				}
			}
	} while (!bStable);
}

//*****************************************************************************
/*
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

//-****************************************************************************
bool CDbRoom::IsPathmapNeeded() const
//Returns: situations in which path maps need to be generated
{
	if (this->wBrainCount > 0)
		return true;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_SLAYER)
			return true;
		pMonster = pMonster->pNext;
	}
	return false;
}

//-****************************************************************************
bool CDbRoom::IsTimerNeeded() const
//Returns: whether room objects exist that operate according to a certain time schedule
{
	ASSERT(this->pCurrentGame);
	if (!this->slayerEnters.empty() || !this->halphEnters.empty())
		return true;

	if (this->pCurrentGame->pPlayer->bIsHasted)
		return true;

	if (this->pCurrentGame->dwCutScene)
		return true;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_QROACH || bIsMother(pMonster->wType) ||
				pMonster->wType == M_SKIPPERNEST || bIsSerpent(pMonster->wType))
			return true;
		//Fegundo has a timed rebirth cycle when active.
		if ((pMonster->wType == M_FEGUNDO || pMonster->wType == M_FEGUNDOASHES) &&
				this->pCurrentGame->pPlayer->bCanGetItems)
			return true;
		pMonster = pMonster->pNext;
	}
	return false;
}
*/

//*****************************************************************************
bool CDbRoom::IsValidColRow(const UINT wX, const UINT wY) const
//Our square coords valid for this room dimensions.
{
	return wX < this->wRoomCols && wY < this->wRoomRows;
}

//*****************************************************************************
void CDbRoom::KillSeepOutsideWall(CCueEvents &CueEvents)
//Kill wall monsters that had a black gate, etc., removed from their square
//and are now outside the wall.
{
	CMonster *pNext, *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pNext = pMonster->pNext;
		if (pMonster->IsWallDwelling()) {
			pMonster->KillIfOutsideWall(CueEvents);
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
	const bool bForce)      //[default=false] always remove monster (even critical ones)
{
	ASSERT(pMonster);
	ASSERT(!pMonster->IsPiece());

	const UINT mType = pMonster->wType;

	//Don't remove killed mission-critical types from monster list.
	//It is used in the front end death animation.
	//NOTE: If this happens on room entrance, there's nothing that can
	//be done to fix it, so just pretend there was no monster in the room here.
	bool bRemoveMonster = true;//mType != M_HALPH && mType != M_CLONE;
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
		UnlinkMonster(pMonster);
	}

	switch (mType)
	{
		case M_CHARACTER:
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsMissionCritical())
			{
				//If critical NPC dies, flag it (except on room entrance).
				if (!bForce && this->pCurrentGame && this->pCurrentGame->wTurnNo > 0)
					CueEvents.Add(CID_CriticalNPCDied);
				if (!(bRemoveMonster || bForce))
				{
					pMonster->bAlive = false;
					return false; //retain dead critical character in monster list
				}
			}
			CueEvents.Add(CID_NPCKilled, pMonster);
			if (pCharacter->GetIdentity() == M_BRAIN)
			{
				if (!GetBrainsPresent())
					CueEvents.Add(CID_AllBrainsRemoved);
			}
		}
		break;
		case M_BRAIN:
			if (!GetBrainsPresent())
				CueEvents.Add(CID_AllBrainsRemoved);
		break;
		default: break;
	}
	pMonster->bAlive = false;

	//Front-end clean-up.
	Plot(CCoordSet(pMonster->wX, pMonster->wY)); //map redraw if needed
	CPlayerDouble *pDouble = dynamic_cast<CPlayerDouble*>(pMonster);
	if (pDouble)
	{
		const UINT wSX = pDouble->GetSwordX(), wSY = pDouble->GetSwordY();
		if (IsValidColRow(wSX, wSY))
			Plot(CCoordSet(wSX, wSY));
	}

	//Put monster in dead monster list.
	//The pointer will be valid until this CDbRoom is destroyed.
	this->DeadMonsters.push_back(pMonster);
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

	const bool bRemoved = KillMonster(pMonster, CueEvents, bForce);

	ASSERT(!pMonster->IsAlive());
	ASSERTP(!bRemoved || !GetMonsterAtSquare(wX, wY),
			"Monster not removed or multiple monsters on same square");

	return true;
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
	const UINT eType, //(in) monster type
	const bool bConsiderNPCIdentity) //[default=false]
const
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == eType)
			return pMonster;
		if (pMonster->wType == M_CHARACTER && bConsiderNPCIdentity)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->GetIdentity() == eType)
				return pMonster;
		}
	}
	return NULL;
}

//*****************************************************************************
/*
bool CDbRoom::MonsterWithMovementTypeExists(
//Returns: whether a monster with 'eMovement' type exists.
//
//Params:
	const MovementType eMovement) //(in)
const
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
		if (pMonster->eMovement == eMovement)
			return true;
	return false;
}

//-****************************************************************************
void CDbRoom::DecMonsterCount()
//Decrement room monster count.
{
	ASSERT(this->wMonsterCount != 0); //Count should never decrement past 0.
	--this->wMonsterCount;
	//Check for whether room is cleared was moved to CCurrentGame::ProcessCommand().
	//The check for whether this happened in the editor could be removed.
}
*/

//*****************************************************************************
void CDbRoom::MoveMonster(
//Moves a monster in the monster array.
//
//Params:
	CMonster* const pMonster,  //(in) Monster moving.
	const UINT wDestX, const UINT wDestY)  //(in)   Destination to move to.
{
	ASSERT(this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)]==pMonster);
	ASSERT(!this->pMonsterSquares[ARRAYINDEX(wDestX,wDestY)]);

	this->pMonsterSquares[ARRAYINDEX(pMonster->wX,pMonster->wY)] = NULL;
	this->pMonsterSquares[ARRAYINDEX(wDestX,wDestY)] = pMonster;
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
	CMonsterFactory mf(this->pCurrentGame);
	CMonster *pNew = mf.GetNewMonster(eMonsterType);

	//Set monster position.
	pNew->wX = pNew->wPrevX = wX;
	pNew->wY = pNew->wPrevY = wY;

/*
	//Update room stats.
	//Only certain monsters are counted as required to remove to conquer a room.
	switch (eMonsterType)
	{
		case M_CHARACTER: //possibly required
//		case M_SLAYER:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
//		case M_CLONE: case M_DECOY: case M_MIMIC:
//		case M_CITIZEN: case M_HALPH: case M_WUBBA:
//		case M_FEGUNDO: case M_FEGUNDOASHES:
//		case M_STALWART:
			//not required
		break; 
		case M_BRAIN:
//			++this->wBrainCount;
			//NO BREAK
		default:
			++this->wMonsterCount;
		break;
	}
*/

	if(bLinkMonster)
		LinkMonster(pNew, bInRoom);

	//Return pointer to the new monster.
	return pNew;
}

//*****************************************************************************
void CDbRoom::LinkMonster(
//Sets monster's room position and adds to monster lists and array.
//
//Params:
	CMonster *pMonster,  //(in) Monster to add
	const bool bInRoom)  //is monster in room on player entrance [default=true]
{
	//Find location in list to put monster at.  List is sorted by process sequence.
	CMonster *pSeek = this->pFirstMonster, *pLastPrecedingMonster=NULL;
	while (pSeek)
	{
		if (pSeek->wProcessSequence > pMonster->wProcessSequence)
			break;
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

/*
	//Maintain special room monster lists.
	switch (pMonster->wType)
	{
		case M_DECOY:
			this->Decoys.push_back(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
		break;
		case M_STALWART:
			this->stalwarts.push_back(DYN_CAST(CPlayerDouble*, CMonster*, pMonster));
		break;
		default: break;			
	}
*/
	if (bInRoom)
		SetMonsterSquare(pMonster);
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
CMonster* CDbRoom::GetNPCBeethro() const
//Returns: pointer to first (visible) NPC Beethro in room, else NULL
{
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->GetIdentity() == M_BEETHRO && pCharacter->IsVisible())
				return pMonster;
		}
		pMonster = pMonster->pNext;
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
	ASSERT(IsValidColRow(wLeft, wTop));
	ASSERT(IsValidColRow(wRight, wBottom));

	//Check each square for a monster (or monster piece).
	CMonster **pMonsters, *pMonster;

	ASSERT(wRight < this->wRoomCols && wBottom < this->wRoomRows);
	for (UINT y=wTop; y<=wBottom; ++y)
	{
		pMonsters = &(this->pMonsterSquares[ARRAYINDEX(wLeft,y)]);
		for (UINT x=wLeft; x<=wRight; ++x)
		{
			pMonster = *pMonsters;
			if (pMonster && pMonster->IsAlive() && (!(
					pMonster->wType == M_CHARACTER)) &&
					(bConsiderPieces || !pMonster->IsPiece()))
				return true;
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
	const bool bConsiderNPCIdentity)       //[default=false]
//
//Returns:
//True if there is a monster in the rect, false otherwise.
const
{
	ASSERT(wLeft <= wRight && wTop <= wBottom);
	ASSERT(IsValidColRow(wLeft, wTop));
	ASSERT(IsValidColRow(wRight, wBottom));

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
					if (pCharacter->GetIdentity() == wType)
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
void CDbRoom::DamageMonster(CMonster* pMonster, int damageVal, CCueEvents& CueEvents)
//Damage the given monster, and then kill it if the damage was lethal.
{
	if (pMonster->Damage(CueEvents, damageVal))
	{
		//Monster HP reduced to zero.
		bool bVulnerable = true;
		switch (pMonster->wType)
		{
			case M_CHARACTER:
			{
				//Unkillable NPCs are only "defeated".
				CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				bVulnerable = pCharacter->IsVulnerable();
				if (!bVulnerable)
					pCharacter->Defeat();
			}
			break;
			default: break;
		}
		if (bVulnerable)
		{
			KillMonster(pMonster, CueEvents);
		}
	}
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
					if (pMonster->wType == M_CHARACTER)
//							|| pMonster->wType == M_HALPH
//							|| pMonster->wType == M_NEATHER)
						return true;
					//Is a sword striking the orb?
					if (pMonster->HasSwordAt(wX, wY))
						return true;
				}
			}
	return false;
}

//*****************************************************************************
/*
bool CDbRoom::IsRequired(
//Returns: Whether the room with this ID is required to complete its level.
//
//Params:
	const UINT dwRoomID)
{
	//Open rooms view.
	ASSERT(IsOpen());
	c4_View RoomsView;
	const UINT dwRoomI = LookupRowByPrimaryKey(dwRoomID, P_RoomID, RoomsView);
	if (dwRoomI == ROW_NO_MATCH)
	{
		ASSERT(!"No matching room.");
		return false;
	}

	return p_IsRequired(RoomsView[dwRoomI]) != 0;
}
*/

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

	//Check player's sword.
	if (!this->pCurrentGame->pPlayer->HasSword())
		return false;

	wSX = this->pCurrentGame->pPlayer->GetSwordX();
	wSY = this->pCurrentGame->pPlayer->GetSwordY();
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
		case 1: pszSquares = pszTSquares; break;
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
/*
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

//-****************************************************************************
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
*/

//*****************************************************************************
void CDbRoom::ActivateToken(CCueEvents &CueEvents, const UINT wX, const UINT wY)
//Toggle token -- modify room elements/behavior.
{
	ASSERT(IsValidColRow(wX,wY));

	UINT tParam = GetTParam(wX, wY);
	const RoomTokenType tType = (RoomTokenType)calcTokenType(tParam);
	const bool bOn = bTokenActive(tParam);
	switch (tType)
	{
		case SwordDisarm:
		{
			ChangeTiles(SwordDisarm);
			CueEvents.Add(CID_TokenToggled);
			this->PlotsMade.insert(wX,wY);

			this->pCurrentGame->ToggleSwordDisarm();
		}
		return;

/*
		case PowerTarget:
			//Touching this token gives the non-Beethro player Beethro-like item
			//handling abilities and makes him an attack target.
			//Allows controlling fegundos.
			if (bOn)
				return; //can't turn off
//			this->pCurrentGame->pPlayer->bCanGetItems = true;
//			this->pCurrentGame->pPlayer->bIsTarget = true;
			ChangeTiles(PowerTarget);
			CueEvents.Add(CID_TokenToggled);
			this->PlotsMade.insert(wX,wY);
		return;
*/

		case RotateArrowsCW: ChangeTiles(bOn ? RotateArrowsCCW : tType); break;
		case RotateArrowsCCW: ChangeTiles(bOn ? RotateArrowsCW : tType); break;
		case SwitchTarMud: SwitchTarstuff(T_TAR, T_MUD); break;
		case SwitchTarGel: SwitchTarstuff(T_TAR, T_GEL); break;
		case SwitchGelMud: SwitchTarstuff(T_GEL, T_MUD); break;
		case TarTranslucent:
			//Toggles tar translucency and state of all tokens of this type.
			this->bBetterVision = !bOn;
			ChangeTiles(TarTranslucent);
			CueEvents.Add(CID_TokenToggled);
			this->PlotsMade.insert(wX,wY);
		return;

//		case PersistentCitizenMovement: return; //do nothing

		default: ASSERT(!"Unexpected token type."); break;
	}

	SetTParam(wX, wY, bOn ? tParam - TOKEN_ACTIVE : tParam + TOKEN_ACTIVE);  //toggle on-off
	CueEvents.Add(CID_TokenToggled);
	this->PlotsMade.insert(wX,wY);
}

//*****************************************************************************
void CDbRoom::DoExplode(
//Blow up bombs in coords set.
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	CCoordStack& bombs,       //(in)
	CCoordStack& powder_kegs) //(in)
{
	if (bombs.IsEmpty() && powder_kegs.IsEmpty())
		return;

	static const UINT BOMB_RADIUS = 3;
	static const UINT POWDER_KEG_RADIUS = 1;

	CCoordSet explosion; //what tiles are affected following explosion

	//Each iteration explodes one bomb.
	for (;;) {
		UINT wCol, wRow;
		if (bombs.PopBottom(wCol, wRow)) { //process as queue
			DoExplodeTile(CueEvents, bombs, powder_kegs, explosion, wCol, wRow, BOMB_RADIUS);
			continue;
		}

		if (powder_kegs.PopBottom(wCol, wRow)) {
			DoExplodeTile(CueEvents, bombs, powder_kegs, explosion, wCol, wRow, POWDER_KEG_RADIUS);
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
	UINT wCol, UINT wRow,
	UINT explosion_radius,
	bool bAddCueEvent) //[default=true]
{
	//Remove exploding item.  Initiate explosion.
	const UINT tTile = GetTSquare(wCol, wRow);
	if (tTile == T_BOMB || tTile == T_POWDER_KEG)
		Plot(wCol, wRow, T_EMPTY);

	if (bAddCueEvent)
		CueEvents.Add(CID_BombExploded, new CCoord(wCol, wRow), true);

	//Keep track of a list of squares to expand explosion out from.
	//Constraints will be performed with each expansion from these squares.
	CCoordStack cs(wCol, wRow);

	//Explode outward until done.
	UINT wX, wY;
	while (cs.PopBottom(wX, wY))  //process as queue
		ExpandExplosion(CueEvents, cs, wCol, wRow, wX, wY, bombs, powder_kegs, explosion, explosion_radius);
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
				//Detonate lit keg.
				powder_kegs.Push(wX, wY);
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

						//Light each adjacent bomb and bomb fuse.
						if (this->NewFuses.insert(wAdjX, wAdjY))
							CueEvents.Add(CID_FuseBurning, new CMoveCoord(wAdjX, wAdjY,
									FuseEndAt(wAdjX, wAdjY, false)), true);
					}
				}
			}
			break;
			case T_EMPTY:
			case T_MIRROR:  //mirrors can be pushed onto burning fuses to put them out
			case T_CRATE:
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

	//Explode all explosives simultaneously.
	if (bombs.GetSize() > 0 || powder_kegs.GetSize() > 0)
		DoExplode(CueEvents, bombs, powder_kegs);

	//Ordering issues might place new fuses where these fuses just burned.
	//Ensure that they won't burn here again.
	this->NewFuses -= this->LitFuses;

	//Save newly lit pieces for burning next turn.
	this->LitFuses = this->NewFuses;
	this->NewFuses.clear();
}

//*****************************************************************************
CCoordStack CDbRoom::GetPowderKegsStillOnHotTiles() const
{
	CCoordStack powder_kegs;
	for (CCoordSet::const_iterator it = this->stationary_powder_kegs.begin();
		it != this->stationary_powder_kegs.end(); ++it)
	{
		UINT wX = it->wX;
		UINT wY = it->wY;
		if (GetOSquare(wX, wY) == T_HOT && GetTSquare(wX, wY) == T_POWDER_KEG) {
			powder_kegs.Push(wX, wY);
		}
	}
	return powder_kegs;
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
	UINT wTSquare, wTCovered;
	if (IsValidColRow(wCol-1,wRow))
	{
		wTSquare = GetTSquare(wCol-1, wRow);
		wTCovered = this->coveredTSquares.GetAt(wCol-1, wRow);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
		{
			wOrientation = E;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol+1,wRow))
	{
		wTSquare = GetTSquare(wCol+1, wRow);
		wTCovered = this->coveredTSquares.GetAt(wCol+1, wRow);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
		{
			if (wAdjFuseCount) return NO_ORIENTATION;
			wOrientation = W;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol,wRow-1))
	{
		wTSquare = GetTSquare(wCol, wRow-1);
		wTCovered = this->coveredTSquares.GetAt(wCol, wRow-1);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
		{
			if (wAdjFuseCount) return NO_ORIENTATION;
			wOrientation = S;
			++wAdjFuseCount;
		}
	}
	if (IsValidColRow(wCol,wRow+1))
	{
		wTSquare = GetTSquare(wCol, wRow+1);
		wTCovered = this->coveredTSquares.GetAt(wCol, wRow+1);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB ||
			 wTCovered == T_FUSE || wTCovered == T_BOMB)
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
void CDbRoom::LightFuse(
//Light fuse on square, if it's not already lit.
//
//Params:
	CCueEvents &CueEvents,     //(in/out)
	const UINT wCol, const UINT wRow,   //(in)
	const bool bDelayProcessing)  //(in) true if need to skip fuse processing this turn
											//[default = true]
{
	ASSERT(IsValidColRow(wCol, wRow));

	const UINT wTSquare = GetTSquare(wCol, wRow);
	if (wTSquare != T_FUSE) return;

	//Only fuse ends can be lit.
	if (FuseEndAt(wCol, wRow) == NO_ORIENTATION) return;

	if (!this->LitFuses.has(wCol, wRow))
	{
		if (bDelayProcessing)
			this->NewFuses.insert(wCol, wRow);
		else
			this->LitFuses.insert(wCol, wRow);
		CueEvents.Add(CID_FuseBurning, new CMoveCoord(wCol, wRow, NO_ORIENTATION), true);
	}
}

//*****************************************************************************
void CDbRoom::ProcessActiveFiretraps(CCueEvents& CueEvents)
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
		ActivateFiretrap(iter->wX, iter->wY, CueEvents);
	}
}

//*****************************************************************************
bool CDbRoom::IsDisarmTokenActive() const
{
	for (UINT wY=this->wRoomRows; wY--; )
		for (UINT wX=this->wRoomCols; wX--; )
		{
			if (GetTSquare(wX,wY) == T_TOKEN) {
				const UINT tParam = GetTParam(wX,wY);
				if (bTokenActive(tParam) && calcTokenType(tParam) == SwordDisarm)
					return true;
			}
		}

	return false;
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
		case SwordDisarm:
//		case PowerTarget:
			//Toggle on-off state of all tokens of this type in the room.
			for (wY=this->wRoomRows; wY--; )
				for (wX=this->wRoomCols; wX--; )
					if (GetTSquare(wX,wY) == T_TOKEN)
					{
						const UINT tParam = GetTParam(wX, wY);
						if ((RoomTokenType)calcTokenType(tParam) == tType)
						{
							const bool bOn = bTokenActive(tParam);
							SetTParam(wX, wY, bOn ? tParam - TOKEN_ACTIVE : tParam + TOKEN_ACTIVE);  //toggle on-off
							this->PlotsMade.insert(wX,wY);
						}
					}
		break;
		default: ASSERT(!"Unsupported token type"); break;
	}
}

//*****************************************************************************
void CDbRoom::CharactersCheckForCueEvents(CCueEvents &CueEvents, CMonster* pMonsterList)
//Called once all cue events on a given turn could have fired.
{
	CMonster *pMonster = pMonsterList;
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
void CDbRoom::CheckForFallingAt(const UINT wX, const UINT wY, CCueEvents& CueEvents, const bool bTrapdoorFell)
//Process any objects falling down at (x,y)
{
	UINT wOSquare = GetOSquare(wX,wY), wTSquare = GetTSquare(wX,wY);
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
		case T_MAP: case T_MAP_DETAIL:
		case T_BOMB: case T_ORB:
		case T_SCROLL:
		case T_ATK_UP: case T_ATK_UP3: case T_ATK_UP10:
		case T_DEF_UP: case T_DEF_UP3: case T_DEF_UP10:
		case T_HEALTH_HUGE: case T_HEALTH_BIG: case T_HEALTH_MED: case T_HEALTH_SM:
		case T_MIRROR:
		case T_CRATE:
		case T_POWDER_KEG:
		case T_SWORD: case T_SHIELD: case T_ACCESSORY:
		case T_KEY:
		case T_SHOVEL1: case T_SHOVEL3: case T_SHOVEL10:
			Plot(wX, wY, T_EMPTY);
			if (bIsWater(wOSquare))
				CueEvents.Add(CID_Splash, new CCoord(wX, wY), true);
			else
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(wX, wY, NO_ORIENTATION, wTSquare, 0), true);
		break;
		case T_TAR: case T_MUD: case T_GEL:
			RemoveStabbedTar(wX, wY, CueEvents);
			CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx2(wX, wY, NO_ORIENTATION, wTSquare, 0), true);
		break;
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			if (bIsPit(wOSquare)) //remains on top of water without falling
			{
				Plot(wX, wY, T_EMPTY);
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(wX, wY, NO_ORIENTATION, wTSquare, 0), true);
			}
		break;
		default: break; //nothing else falls
	}

	//Player falls?
	if (this->pCurrentGame && this->pCurrentGame->wTurnNo > 0 && this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		if (!this->pCurrentGame->pPlayer->IsFlying() && //flying player doesn't fall
				!(bIsWater(wOSquare) && this->pCurrentGame->pPlayer->CanWalkOnWater())) //swimming player won't die on water
			CueEvents.Add(bIsWater(wOSquare) ? CID_PlayerDrownedInWater : CID_PlayerFellIntoPit);
	}

	//If monster is over pit or water, then monster dies.
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (!pMonster) return;
	if (pMonster->IsFlying())
		return; //flying monsters don't fall

	if (pMonster->IsSwimming() && bIsWater(wOSquare))
		return; //water-based monsters don't die in water

	//Handle large monsters specially based on type.
	pMonster = pMonster->GetOwningMonster();
	if (LargeMonsterFalls(pMonster, wX, wY, CueEvents))
		return;

	//Non-flying monster falls and dies.
	AddFallingMonsterEvent(CueEvents, pMonster, wOSquare);

	KillMonster(pMonster, CueEvents);
//	this->pCurrentGame->TallyKill();  //counts as a kill
}

//*****************************************************************************
bool CDbRoom::AddFallingMonsterEvent(
	CCueEvents& CueEvents, CMonster* pMonster, const UINT wOSquare)
	const
{
	if (wOSquare == T_WATER) {
		CueEvents.Add(CID_Splash, new CCoord(pMonster->wX, pMonster->wY), true);
		return true;
	}

	if (bIsPit(wOSquare)) {
		UINT id = pMonster->GetResolvedIdentity();

		//Brain, nest and puff characters can have an orientation, which we don't want
		UINT wO = (id == M_BRAIN || id == M_SKIPPERNEST || id == M_FLUFFBABY) ? NO_ORIENTATION : pMonster->wO;

		//Use custom monster type for characters.
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			id = pCharacter->wLogicalIdentity;
		}

		CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(pMonster->wX, pMonster->wY,
			wO, M_OFFSET + id, pMonster->HasSword() ? pMonster->GetWeaponType() : NoSword), true);
		return true;
	}

	return false;
}

//*****************************************************************************
bool CDbRoom::LargeMonsterFalls(CMonster*& pMonster, const UINT wX, const UINT wY, CCueEvents& CueEvents)
//Handles special falling logic for monsters occupying more than one tile
//
//Returns: whether done processing fall logic for large monster
{
	ASSERT(pMonster);
	if (!pMonster->IsLongMonster())
		return false;

	if (pMonster->wType == M_ROCKGIANT)
	{
		//Show all pieces falling.
		UINT count = 1; //provide temporary tile numbering
		for (list<CMonsterPiece*>::iterator piece = pMonster->Pieces.begin();
			piece != pMonster->Pieces.end(); ++piece, ++count)
		{
			CMonsterPiece* pPiece = (*piece);
			CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(
				pPiece->wX, pPiece->wY,
				count * MONSTER_PIECE_OFFSET + pMonster->wO, //special encoding for monster piece
				M_OFFSET + pMonster->wType, 0), true);
		}
		return false; //more fall processing to do by caller
	}

	ASSERT(bIsSerpent(pMonster->wType));
	//Serpent dies if its head is over pit/water.
	const UINT wOTileAtHead = GetOSquare(pMonster->wX, pMonster->wY);
	if (wOTileAtHead == T_WATER)
	{
		CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(
			pMonster->wX, pMonster->wY, S), true);
		return false; //more fall processing to do by caller
	}

	const bool bFallsInPit = bIsPit(wOTileAtHead);
	if (bFallsInPit) {
		for (list<CMonsterPiece*>::iterator pieceIt = pMonster->Pieces.begin();
			pieceIt != pMonster->Pieces.end(); ++pieceIt)
		{
			const CMonsterPiece& piece = **pieceIt;
			if (bIsPit(GetOSquare(piece.wX, piece.wY)))
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(
					piece.wX, piece.wY, pMonster->wType,
					piece.wTileNo, 0), true);
		}
		return false; //more fall processing to do by caller
	}

	//Serpent head is on solid ground.
	//Break off the tail at the first point where there is no ground.
	bool bSerpentDiedFromTruncation = false;

	CSerpent* pSerpent = DYN_CAST(CSerpent*, CMonster*, pMonster);	
	pSerpent->OrderPieces(); //for following iteration to work properly

	for (list<CMonsterPiece*>::iterator piece = pMonster->Pieces.begin();
		piece != pMonster->Pieces.end(); ++piece)
	{
		const UINT wPX = (*piece)->wX, wPY = (*piece)->wY;
		const UINT wOTile = GetOSquare(wPX, wPY);
		if (!(bIsPit(wOTile) || wOTile == T_WATER))
			continue;

		//Break off this tile and everything thereafter.

		//Show falling effect for all pieces breaking off.
		while (piece != pMonster->Pieces.end())
		{
			CMonsterPiece& pPiece = *(*piece);
			if (bIsPit(GetOSquare(pPiece.wX, pPiece.wY)))
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(
					pPiece.wX, pPiece.wY, pMonster->wType,
					pPiece.wTileNo, 0), true);
			++piece;
		}

		//Shorten tail to just past this point.
		while (pSerpent->tailX != wPX || pSerpent->tailY != wPY)
		{
			if (pSerpent->ShortenTail(CueEvents))
				bSerpentDiedFromTruncation = true;
		}
		if (pSerpent->ShortenTail(CueEvents))
			bSerpentDiedFromTruncation = true;
	}
	if (bSerpentDiedFromTruncation)
	{
		//Kill serpent, but don't show it falling.
		KillMonster(pMonster, CueEvents);
		//this->pCurrentGame->TallyKill();  //counts as a kill
	}
	return true; //don't need the fall/kill processing done by caller
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
	SetSwordsSheathed();

	CMonster *pMonster, *pNextMonster;
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pNextMonster)
	{
		pNextMonster = pMonster->pNext; //cache link in case monster is unlinked below

		if (!pMonster->bIsFirstTurn && //don't process entities generated during preprocessing
				pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->Process(CMD_WAIT, CueEvents);

			if (pCharacter->bGlobal)
			{
				//Remove NPC object to global script list.
				this->UnlinkMonster(pCharacter);

				this->pCurrentGame->appendToGlobalMonsterList(pCharacter);
			}
		}
	}
}

//*****************************************************************************
bool CDbRoom::PressurePlateIsDepressedBy(const UINT item)
//Returns: whether item can depress a pressure plate
{
	switch (item)
	{
		case T_BOMB: case T_MIRROR: case T_CRATE: case T_POWDER_KEG:
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

/*
	//Tiles built are plotted to room.
	if (!this->building.empty())
	{
		for (UINT wY=0; wY<this->wRoomRows; ++wY)
			for (UINT wX=0; wX<this->wRoomCols; ++wX)
			{
				UINT wTile = this->building.get(wX,wY);
				if (wTile)
				{
					--wTile; //convert from 1-based
					if (bIsTrapdoor(wTile) && !bIsTrapdoor(GetOSquare(wX, wY)))
						IncTrapdoor(CueEvents);
					Plot(wX, wY, wTile);
					CueEvents.Add(CID_ObjectBuilt, new CMoveCoord(wX, wY, wTile), true);
				}
			}
		this->building.clear();
	}
*/

/*
	if (CueEvents.HasOccurred(CID_EvilEyeWoke))
		this->bTarWasStabbed = true;	//indicates room has entered a "dangerous" state
*/

	//These only happen on full turn increments.
	if (bFullMove)
	{
		BurnFuses(CueEvents);

/*
		//Brairs advance after any black doors were possibly opened.
		ExpandBriars(CueEvents);
*/
	} else {
		//Signal fuses burning, but don't alter them.
		BurnFuseEvents(CueEvents);
	}

	KillSeepOutsideWall(CueEvents);

	if (bFullMove ||
			//Monster died -- maybe a depressed pressure plate gets released as a result.
			CueEvents.HasOccurred(CID_MonsterDiedFromStab) ||
			CueEvents.HasOccurred(CID_NPC_Defeated))
	{
/*
		//Grow the tar flavors in response to a cue event.
		if (CueEvents.HasOccurred(CID_TarGrew) || CueEvents.HasOccurred(CID_MudGrew) || CueEvents.HasOccurred(CID_GelGrew))
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
*/

		//Check for pressure plates being released.

		//If depressed pressure plate has nothing on its surface, it resets.
		//Make sure the order in which this happens is predictable from the
		//room layout, for the rare case where it makes a difference.
		typedef std::map<UINT, COrbData*> plateOrder_t;
		plateOrder_t plateOrder;
		UINT wSwIndex = static_cast<UINT>(-1);
		if (!bIsEntityFlying(this->pCurrentGame->pPlayer->wAppearance))
		{
			wSwIndex = this->pressurePlateIndex.GetAt(
				this->pCurrentGame->pPlayer->wX,this->pCurrentGame->pPlayer->wY) - 1;
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
					if (pMonster && !pMonster->IsFlying())
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

	//Firetraps activate based on latest pressure plate state.
	if (bFullMove) {
		ProcessActiveFiretraps(CueEvents);
	}

	//Process gaze after tarstuff has grown
	//and released pressure plates have possibly changed door states.
	if (!this->pCurrentGame->InCombat())
		ProcessAumtlichGaze(CueEvents);//, bFullMove);

	//Clear first turn status of current monsters.  First turn flag indicates that
	//a monster was created and added to the monster list during processing, but is 
	//not ready to be processed yet.  Clearing it here will allow the monster to be
	//processed the next time ProcessMonsters() is called.
	ResetMonsterFirstTurnFlags();
}

//*****************************************************************************
void CDbRoom::ResetTurnFlags()
//Reset flags at the start of each "real" turn before the player moves.
{
	//Currently, this resets NPC behavioral flags.
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);

			pCharacter->bAttacked = false;
			pCharacter->bPlayerTouchedMe = false;
		}
	}
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
	CCoordStack& powder_kegs,  //(in/out) powder kegs to be exploded
	CCoordSet& explosion,      //(in/out) tiles needed to be destroyed/activated
	const UINT radius)
{
	//Constraint 1. How far explosion can go.
	if (!IsValidColRow(wX, wY)) return;

	const UINT dist = nDist(wX,wY,wBombX,wBombY); 
	const UINT direction = GetOrientation(wBombX, wBombY, wX, wY);
	UINT wTileNo = GetFSquare(wX,wY);

	if (dist > radius)
		return;

	if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo,direction))
		return; //force arrows in wrong direction stop the blast

	//Constraint 2. What can be destroyed by an explosion.
	switch (GetOSquare(wX,wY))
	{
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_PIT: case T_PIT_IMAGE: case T_PLATFORM_P:
		case T_WATER: case T_PLATFORM_W:
		case T_STAIRS:	case T_STAIRS_UP:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE:
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:
		case T_DOOR_RO: case T_DOOR_BO: case T_DOOR_MONEYO:
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_HOT: case T_GOO: case T_PRESSPLATE:
		case T_WALL_B:	case T_WALL_H:
		case T_DIRT1: case T_DIRT3: case T_DIRT5:
		case T_MISTVENT:
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
			//if we haven't considered this bomb before, then add it
			if (!explosion.has(wX, wY))
				powder_kegs.Push(wX, wY);
			break;
		case T_OBSTACLE:
			//blocks explosion, but because obstacles can be placed
			//on walls, we still need to consider this square
			explosion.insert(wX,wY);
			return;
		case T_ORB:
			//orbs stop blasts, but must still be activated
			explosion.insert(wX,wY);
			return;
		case T_TAR: case T_MUD: case T_GEL:
			//Tarstuff explosions need to have a direction of explosion,
			//so let's do that now providing we haven't done this tile yet.
			if (!explosion.has(wX,wY))
				CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY, direction, wTileNo), true);
			break;
		case T_MIST:
			if (!explosion.has(wX,wY))
				CueEvents.Add(CID_MistDestroyed, new CMoveCoordEx(wX, wY, direction, T_MIST), true);
			break;
		default:
			break;
	}

	//Add tile to explosion radius
	explosion.insert(wX,wY);

	//Constraint 3. Direction of explosion determines where explosion will
	//spread out to: (1) when coming in a direction horizontal or vertical
	//direction from the bomb, allow fan out in three directions,
	//(2) otherwise only keep moving outward in one diagonal direction.
	const int oBlastX = nGetOX(direction);
	const int oBlastY = nGetOY(direction);
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
			//Otherwise, add to list.
			cs.Push(wX + oX, wY + oY);
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
	const bool bHarmsPlayer)     //[default=true]
{
	ASSERT(IsValidColRow(wX, wY));

	//What is affected by an explosion on this tile:
	switch (GetOSquare(wX,wY))
	{
		case T_WALL_B: case T_WALL_H:
			ASSERT(bIsPlainFloor(this->coveredOSquares.GetAt(wX, wY)));
			Plot(wX,wY,this->coveredOSquares.GetAt(wX, wY));
			break;
		case T_DIRT1: case T_DIRT3: case T_DIRT5:
			Plot(wX, wY, T_FLOOR_DIRT);
			break;
		case T_THINICE:
			DestroyTrapdoor(wX, wY, CueEvents);
		break;

		//If we decided we should explode these squares, then they should
		//be dealt with, regardless of whether these tiles are impassable now.
		case T_DOOR_Y: case T_DOOR_G: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B: case T_DOOR_MONEY:
		default: break;
	}

	//Covered mist is destroyed before covering item
	if (this->coveredTSquares.GetAt(wX, wY) == T_MIST) {
		this->coveredTSquares.Remove(wX, wY);
	}

	UINT wTileNo = GetTSquare(wX,wY);
	switch (wTileNo)
	{
		case T_OBSTACLE:
		return;
		case T_FUSE:
			//Light the fuse.
			if (!this->NewFuses.has(wX, wY) && !this->LitFuses.has(wX, wY))
			{
				this->NewFuses.insert(wX, wY);  //Burn fuse next turn.
				CueEvents.Add(CID_FuseBurning, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
			}
		break;
		case T_ORB:
			//Orbs activate from the explosion
			ActivateOrb(wX, wY, CueEvents, OAT_Item);
		return;  //orb stops blast
		case T_TAR:	case T_MUD: case T_GEL:
			RemoveStabbedTar(wX,wY, CueEvents); //now remove tarstuff
		break;
		case T_BRIAR_SOURCE:
			//Briar sources are destroyed.
			Plot(wX,wY,T_EMPTY);
		break;
		case T_BRIAR_DEAD: case T_BRIAR_LIVE: //Flow tiles are destroyed.
		case T_MIRROR:  //shattered
		case T_CRATE:
		case T_KEY:
		case T_MAP: case T_MAP_DETAIL:
		case T_SHOVEL1: case T_SHOVEL3: case T_SHOVEL10:
		case T_MIST:
			Plot(wX,wY,T_EMPTY);
		break;
		default:
			if (bIsPowerUp(wTileNo))
			{
				//power up items are destroyed
				Plot(wX,wY,T_EMPTY);
			}
			//Explosion passes through everything else.
		break;
	}

	CueEvents.Add(CID_Explosion, new CCoord(wX, wY), true);

	if (!this->pCurrentGame)
		return;

	//Explosion harms player.
	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	const int explosionVal = int(player.st.explosionVal);
	if (bHarmsPlayer && this->pCurrentGame->IsPlayerAt(wX, wY))
		player.Damage(CueEvents, explosionVal, CID_ExplosionKilledPlayer);

	//Monsters are damaged.
	CMonster *pMonster = GetMonsterAtSquare(wX, wY);
	if (pMonster)
	{
		//Blow up entire serpent when its head is hit.
		if (pMonster->IsPiece())
		{
			CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
			pMonster = pPiece->pMonster;
			if (bIsSerpent(pMonster->wType))
				return; //nothing to do below //bVulnerable = false; //can't harm serpent bodies
		}

		DamageMonster(pMonster, explosionVal, CueEvents);
	}
}

//*****************************************************************************
void CDbRoom::ProcessAumtlichGaze(CCueEvents &CueEvents)
//Determine which tiles are affected by gaze beams in room.
{
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->HasRayGun())
			pMonster->UpdateGaze(CueEvents);
	}
}

//*****************************************************************************
void CDbRoom::PushObject(
//Push an object from the source to destination tiles.
//
//Params:
	const UINT wSrcX, const UINT wSrcY,
	const UINT wDestX, const UINT wDestY,
	CCueEvents& CueEvents) //(in/out)
{
	const UINT wTile = GetTSquare(wSrcX, wSrcY);
	Plot(wSrcX, wSrcY, T_EMPTY);

	if (wTile == T_POWDER_KEG) {
		this->stationary_powder_kegs.erase(wSrcX, wSrcY);

		//Check for a weapon at destination location that might explode the keg
		if (this->pCurrentGame->IsPlayerSwordAt(wDestX, wDestY) && !this->pCurrentGame->IsPlayerSwordExplosiveSafe()) {
			this->stabbed_powder_kegs.Push(wDestX, wDestY);
			return;
		}

		UINT wO = nGetO(int(wDestX) - int(wSrcX), int(wDestY) - int(wSrcY));
		for (UINT nO = 0; nO < ORIENTATION_COUNT; ++nO) {
			if (nO != NO_ORIENTATION && nO != wO)
			{
				CMonster* pMonster = GetMonsterAtSquare(wDestX - nGetOX(nO), wDestY - nGetOY(nO));
				if (pMonster && !pMonster->IsExplosiveSafe() && pMonster->HasSwordAt(wDestX, wDestY)) {
					this->stabbed_powder_kegs.Push(wDestX, wDestY);
					return;
				}
			}
		}
	}

	//Determine whether object falls down at destination square.
	switch (GetOSquare(wDestX, wDestY))
	{
		case T_PIT: case T_PIT_IMAGE:
			CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(wDestX, wDestY,
					NO_ORIENTATION, wTile, 0), true);
		return;
		case T_WATER:
			CueEvents.Add(CID_Splash, new CCoord(wDestX, wDestY), true);
		return;
		default: break;
	}

	const UINT oldTTile = GetTSquare(wDestX, wDestY);
	switch (oldTTile)
	{
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			CueEvents.Add(CID_Splash, new CCoord(wDestX, wDestY), true);
		return;
		default:
			//Keep track of item covered up
			if (TILE_LAYER[wTile] == 1 && oldTTile != T_EMPTY)
				this->coveredTSquares.Add(wDestX, wDestY, oldTTile);

			Plot(wDestX, wDestY, wTile);
			if (GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
				ActivateOrb(wDestX, wDestY, CueEvents, OAT_PressurePlate);

			//Persist the pushed object's data,
			//so it can be globally referenced for the duration of turn processing.
			RoomObject* pObj = new RoomObject(wDestX, wDestY, wTile);
			pObj->wPrevX = wSrcX;
			pObj->wPrevY = wSrcY;
			this->pushed_objects.insert(pObj);
			
			break;
	}
}

//*****************************************************************************
bool CDbRoom::CanSetSwordsman(
//Returns: whether player can be placed on this square
//(Used when checking whether room can be entered on this square.)
//
//Params:
	const UINT dwX, const UINT dwY)   //(in) square
//	const bool bRoomConquered)          //(in) whether room is conquered [default=true]
const
{
	//Listed are all things player cannot enter room on.
	const UINT wAppearance = this->pCurrentGame ?
			this->pCurrentGame->pPlayer->wAppearance : static_cast<UINT>(defaultPlayerType());
	switch (GetOSquare(dwX, dwY))
	{
		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
			if (wAppearance != M_SEEP)
				return false;
		break;
		case T_PIT: case T_PIT_IMAGE:
			if (!(wAppearance == M_WWING || wAppearance == M_FEGUNDO || wAppearance == M_FLUFFBABY))
				return false;
		break;
		case T_WATER:
			//Player can enter room on water with water-walking equipment.
			if (!this->pCurrentGame || !this->pCurrentGame->pPlayer->CanWalkOnWater())
				return false;
		break;

		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD:
		case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_STAIRS: case T_STAIRS_UP:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:
		case T_DOOR_RO: case T_DOOR_BO: case T_DOOR_MONEYO:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_PRESSPLATE: case T_THINICE:
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_GOO: case T_HOT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
		default:
			if (wAppearance == M_SEEP)
				return false;
		break;
	}

	const UINT t = GetTSquare(dwX, dwY);
	switch (t)
	{
		case T_ORB:
		case T_TAR:	case T_MUD: case T_GEL:
		case T_BOMB:
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
		case T_OBSTACLE:
			return false;
		default:
		case T_MIRROR: //is removed when player enters room on one
			break;
	}

	//Player cannot enter a room on a monster.
	CMonster *pMonster = GetMonsterAtSquare(dwX, dwY);
	if (pMonster)
		return false;

	return true;
}

//*****************************************************************************
bool CDbRoom::SomeMonsterCanSmellSwordsman() const
//Returns: whether a monster can smell the player.
{
	return IsMonsterWithin(this->pCurrentGame->pPlayer->wX,
			this->pCurrentGame->pPlayer->wY, DEFAULT_SMELL_RANGE, false);
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
//Get tile# for a square on the transparent layer.
//
//Params:
	const UINT wX, const UINT wY) //(in) 
const
{
	ASSERT(wX < this->wRoomCols);
	ASSERT(wY < this->wRoomRows);

	return (UINT) (unsigned char) (this->pszTSquares[ARRAYINDEX(wX,wY)]);
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

	return this->pszTParams[ARRAYINDEX(wX,wY)];
}

//*****************************************************************************
UINT CDbRoom::GetCoveredTSquare(const UINT wX, const UINT wY) const
{
	return this->coveredTSquares.GetAt(wX, wY);
}

//*****************************************************************************
void CDbRoom::ClampCoordsToRoom(int& nX, int& nY) const
{
	if (nX < 0)
		nX = 0;
	else if ((UINT)nX >= this->wRoomCols)
		nX = this->wRoomCols - 1;

	if (nY < 0)
		nY = 0;
	else if ((UINT)nY >= this->wRoomRows)
		nY = this->wRoomRows - 1;
}

//*****************************************************************************
UINT CDbRoom::GetOSquareWithGuessing(
//Get tile# for a square on the opaque layer.  If col/row is out-of-bounds then
//a "guess" will be made--the tile of whichever square is closest to the OOB square 
//will be used.
	 int nX, int nY) const
{
	ClampCoordsToRoom(nX, nY);
	return (UINT) (unsigned char) (this->pszOSquares[ARRAYINDEX(nX,nY)]);
}

//*****************************************************************************
UINT CDbRoom::GetTSquareWithGuessing(
//Get tile# for a square on the opaque layer.  If col/row is out-of-bounds then
//a "guess" will be made--the tile of whichever square is closest to the OOB square 
//will be used.
	 int nX, int nY) const
{
	ClampCoordsToRoom(nX, nY);
	return (UINT) (unsigned char) (this->pszTSquares[ARRAYINDEX(nX,nY)]);
}

//*****************************************************************************
CEntity* CDbRoom::GetSpeaker(const UINT wType, const bool bConsiderBaseType) //[default=false]
//Returns: pointer to character/monster of specified type
{
	if (wType == M_PLAYER)
		return this->pCurrentGame->pPlayer;
	if ((bConsiderBaseType && this->pCurrentGame->pPlayer->wAppearance == wType) ||
			(!bConsiderBaseType && this->pCurrentGame->pPlayer->wIdentity == wType))
		return this->pCurrentGame->pPlayer;

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
}

//*****************************************************************************
UINT CDbRoom::GetExitIndexAt(
//Used to differentiate between different room exits
//
//Returns: # of exit at (wX,wY), or NO_EXIT if none
//
//Params:
	const UINT wX, const UINT wY) //(in)
const
{
	for (UINT i=0; i<this->Exits.size(); ++i)
	{
		const CExitData &stairs = *(this->Exits[i]);
		if (wX >= stairs.wLeft && wX <= stairs.wRight &&
				wY >= stairs.wTop && wY <= stairs.wBottom) 
		{
			return i;
		}
	}

	return NO_EXIT;
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
		if (wX >= stairs.wLeft && wX <= stairs.wRight &&
				wY >= stairs.wTop && wY <= stairs.wBottom) 
		{
			dwEntranceID = stairs.dwEntranceID;
			return true;
		}
	}

	dwEntranceID = 0L;
	return false;
}

//*****************************************************************************
void CDbRoom::FindOrbsToOpenDoor(CCoordSet& orbs, CCoordSet& doorSquares) const
{
	COrbData *pOrb;
	COrbAgentData *pAgent;
	UINT wOrbIndex, wAgentIndex;
	for (wOrbIndex=this->orbs.size(); wOrbIndex--; )
	{
		pOrb = this->orbs[wOrbIndex];
		for (wAgentIndex=pOrb->agents.size(); wAgentIndex--; )
		{
			pAgent = pOrb->agents[wAgentIndex];
			if (pAgent->action != OA_CLOSE)
			{
				//This agent opens some door.
				//Check each door square for this orb agent acting on it.
				if (doorSquares.has(pAgent->wX, pAgent->wY))
				{
					//This orb agent opens the door.  Remember this orb.
					orbs.insert(pOrb->wX, pOrb->wY);
					break;   //continue to next orb
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
		CExitData *pStairs = this->Exits[i];
		if (wX >= pStairs->wLeft && wX <= pStairs->wRight &&
				wY >= pStairs->wTop && wY <= pStairs->wBottom)
		{
			//Modify existing exit's value.
			pStairs->dwEntranceID = dwEntranceID;

			//Ensure boundaries include those passed in.
			if (wX < pStairs->wLeft)
				pStairs->wLeft = wX;
			if (wX2 > pStairs->wRight)
				pStairs->wRight = wX2;
			if (wY < pStairs->wTop)
				pStairs->wTop = wY;
			if (wY2 > pStairs->wBottom)
				pStairs->wBottom = wY2;

			return;
		}
	}

	//Add new exit for the specified rectangular room region.
	ASSERT(wX2 < this->wRoomCols);
	ASSERT(wY2 < this->wRoomRows);

	CExitData *pNewExit = new CExitData(dwEntranceID, wX, wX2, wY, wY2);
	this->Exits.push_back(pNewExit);
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
	//calculate how far from entrance
	int dX, dY;
	GetPositionInLevel(dX, dY);

	//Call language-specific version of method.
	switch (Language::GetLanguage())
	{
		case Language::English:
		case Language::French:
		case Language::Russian:
			GetLevelPositionDescription_English(wstrDescription, dX, dY, bAbbreviate);
		break;
		default:
			//Not supported -- just use English grammar.
			GetLevelPositionDescription_English(wstrDescription, dX, dY, bAbbreviate);
		break;
	}
}

//*****************************************************************************
void CDbRoom::GetPositionInLevel(int& dx, int& dy) const
{
	UINT dwRoomX, dwRoomY; //level starting room coords
	CDbLevels::GetStartingRoomCoords(this->dwLevelID, dwRoomX, dwRoomY);

	dx = this->dwRoomX - dwRoomX;   //offset from starting room
	dy = this->dwRoomY - dwRoomY;
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
float CDbRoom::GetStatModifierFromCharacters(ScriptVars::StatModifiers statType) const
// Calculates the overall stat modifier provided by NPCs in the room
{
	float fMult = 1.0f;

	for (CMonster* pMonster = this->pFirstMonster; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER) {
			CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			fMult *= pCharacter->GetStatModifier(statType);
		}
	}

	return fMult;
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
			const UINT tTile = GetTSquare(wX,wY);
			CMonster *pMonster = GetMonsterAtSquare(wX,wY);

			if (pMonster)
			{
				stats.GOLD += pMonster->getGOLD();
				stats.XP += pMonster->getXP();
			}

			switch (oTile)
			{
				case T_DOOR_MONEY:
					stats.moneyDoorCost += pLevel->getItemAmount(oTile);
				break;
				case T_DOOR_MONEYO:
					stats.openMoneyDoorCost += pLevel->getItemAmount(oTile);
				break;
				case T_DOOR_Y:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.yellowDoors;
					}
				break;
				case T_DOOR_YO:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openYellowDoors;
					}
				break;
				case T_DOOR_G:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.greenDoors;
					}
				break;
				case T_DOOR_GO:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openGreenDoors;
					}
				break;
				case T_DOOR_R:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.redDoors;
					}
				break;
				case T_DOOR_RO:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openRedDoors;
					}
				break;
				case T_DOOR_C:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.blueDoors;
					}
				break;
				case T_DOOR_CO:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openBlueDoors;
					}
				break;
				case T_DOOR_B:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.blackDoors;
					}
				break;
				case T_DOOR_BO:
					GetConnectedTiles(wX,wY,CTileMask(oTile),false,
							tiles, &ignoreOTiles);
					if (!tiles.empty())
					{
						ignoreOTiles += tiles;
						++stats.openBlackDoors;
					}
				break;
				
				case T_DIRT1:
				case T_DIRT3:
				case T_DIRT5:
					stats.dirtBlockCost += pLevel->getItemAmount(oTile);
				break;
			}

			switch (tTile)
			{
				case T_KEY:
				{
					const UINT tParam = GetTParam(wX,wY);
					switch (tParam)
					{
						case YellowKey: ++stats.yellowKeys; break;
						case GreenKey: ++stats.greenKeys; break;
						case BlueKey: ++stats.blueKeys; break;
						case SkeletonKey: ++stats.skeletonKeys; break;
						default: break;
					}
				}
				break;
				case T_ATK_UP: case T_ATK_UP3: case T_ATK_UP10:
					stats.ATK += pLevel->getItemAmount(tTile);
				break;
				case T_DEF_UP: case T_DEF_UP3: case T_DEF_UP10:
					stats.DEF += pLevel->getItemAmount(tTile);
				break;
				case T_HEALTH_HUGE: case T_HEALTH_BIG: case T_HEALTH_MED: case T_HEALTH_SM:
					stats.HP += pLevel->getItemAmount(tTile);
				break;
				case T_SHOVEL1: case T_SHOVEL3: case T_SHOVEL10:
					stats.shovels += pLevel->getItemAmount(tTile);
				break;

				case T_SWORD:
				{
					//Display most powerful sword available.
					const UINT tParam = GetTParam(wX,wY);
					const UINT power = CCurrentGame::getPredefinedWeaponPower(tParam);
					const UINT oldPower = CCurrentGame::getPredefinedWeaponPower(stats.sword);
					if ((power > oldPower || (power == oldPower && tParam > stats.sword)) && tParam != WeaponSlot)
						stats.sword = tParam;
				}
				break;
				case T_SHIELD:
				{
					//Display most powerful shield available.
					const UINT tParam = GetTParam(wX,wY);
					const UINT power = CCurrentGame::getPredefinedShieldPower(tParam);
					const UINT oldPower = CCurrentGame::getPredefinedShieldPower(stats.shield);
					if ((power > oldPower || (power == oldPower && tParam > stats.shield)) && tParam != ArmorSlot)
						stats.shield = tParam;
				}
				break;
				case T_ACCESSORY:
				{
					//Display any accessory available, as they are not ordered by power.
					const UINT tParam = GetTParam(wX,wY);
					ASSERT(tParam < AccessoryCount);
					if (tParam != AccessorySlot)
						stats.accessory = tParam;
				}
				break;
			}
		}
}

//
//CDbRoom private methods.
//

//*****************************************************************************
CMonster* CDbRoom::FindLongMonster(
//From the current square, searches along the length of a long monster
//until a pointer to the monster object is found.  (Recursive call.)
//
//Returns: pointer to the monster, if it was found, else NULL
//
//Params:
	const UINT wX, const UINT wY, //(in) Search from this position.
	const UINT wFromDirection) //(in) Where last search move was from
										//(default = 10 (an invalid orientation) at start
const
{
	if (!IsValidColRow(wX,wY))
		return NULL;

	CMonster *pMonster = GetMonsterAtSquare(wX,wY);
	if (pMonster)
		return pMonster;  //found the head

	const UINT tile = GetTSquare(wX, wY);
	ASSERT(bIsSerpentTile(tile)); //only long monster
	switch (tile)
	{
		//Check in one direction.
		case T_SNK_EW:
		case T_SNK_NW:
		case T_SNK_SW:
		case T_SNKT_W:
			if (wFromDirection != E) pMonster = FindLongMonster(wX+1,wY,W);
			break;

		case T_SNK_NS:
		case T_SNK_SE:
		case T_SNKT_S:
			if (wFromDirection != N) pMonster = FindLongMonster(wX,wY-1,S);
			break;

		case T_SNK_NE:
		case T_SNKT_N:
			if (wFromDirection != S) pMonster = FindLongMonster(wX,wY+1,N);
			break;

		case T_SNKT_E:
			if (wFromDirection != W) pMonster = FindLongMonster(wX-1,wY,E);
			break;
		default: ASSERT(!"Bad serpent tile."); return NULL;
	}

	if (!pMonster)
		switch (tile)
		{
			//Check in other direction.
			case T_SNK_EW:
			case T_SNK_SE:
			case T_SNK_NE:
				if (wFromDirection != W) pMonster = FindLongMonster(wX-1,wY,E);
				break;

			case T_SNK_NW:
			case T_SNK_NS:
				if (wFromDirection != S) pMonster = FindLongMonster(wX,wY+1,N);
				break;

			case T_SNK_SW:
				if (wFromDirection != N) pMonster = FindLongMonster(wX,wY-1,S);
				break;
		}

	//Return monster, or NULL if a pointer to it wasn't found along this way.
	return pMonster;
}

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
				wstrDescription += temp[0];
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
				wstrDescription += temp[0];
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
void CDbRoom::OpenDoor(
//Opens a door.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coords of any square of the door to open.
{
	const UINT oTile = GetOSquare(wX, wY);
	switch (oTile)
	{
		case T_DOOR_Y: FloodPlot(wX, wY, T_DOOR_YO, false); break;
		case T_DOOR_G: FloodPlot(wX, wY, T_DOOR_GO, false); break;
		case T_DOOR_C: FloodPlot(wX, wY, T_DOOR_CO, false); break;
		case T_DOOR_R: FloodPlot(wX, wY, T_DOOR_RO, false); break;
		case T_DOOR_B: FloodPlot(wX, wY, T_DOOR_BO, false); break;
		case T_DOOR_MONEY: Plot(wX, wY, T_DOOR_MONEYO); break; //only open one tile
		default: break;
	}
}

//*****************************************************************************
void CDbRoom::CloseDoor(
//Closes a door.
//
//Params:
	const UINT wX, const UINT wY, //(in) Coords of any square of the door to close.
	CCueEvents& CueEvents) //(in/out)
{
	const UINT oTile = GetOSquare(wX, wY);
	if (!bIsOpenDoor(oTile))
		return;

	CTileMask doorMask(oTile);
	CCoordSet squares;

	if (oTile == T_DOOR_MONEYO)
	{
		//Only alter this tile.
		squares.insert(wX, wY);
	} else {
		//Gather list of open door squares.
		GetConnectedTiles(wX, wY, doorMask, false, squares);
	}

	UINT plotTile;
	switch (oTile)
	{
		case T_DOOR_YO: plotTile = T_DOOR_Y; break;
		case T_DOOR_GO: plotTile = T_DOOR_G; break;
		case T_DOOR_CO: plotTile = T_DOOR_C; break;
		case T_DOOR_RO: plotTile = T_DOOR_R; break;
		case T_DOOR_BO: plotTile = T_DOOR_B; break;
		case T_DOOR_MONEYO: plotTile = T_DOOR_MONEY; break;
		default: break;
	}

	//Plot these squares.
	UINT wSX, wSY;
	for (CCoordSet::const_iterator iter=squares.begin(); iter!=squares.end(); ++iter)
	{
		wSX = iter->wX; wSY = iter->wY;
		Plot(wSX, wSY, plotTile);
		//Closing a door will cut fuses.
		switch (GetTSquare(wSX, wSY))
		{
		case T_FUSE:
			Plot(wSX, wSY, T_EMPTY);
			this->LitFuses.erase(wSX, wSY);
		break;
		}

		//Closing a door destroys mist
		DestroyMist(wSX, wSY, CueEvents);
	}
}

//*****************************************************************************
void CDbRoom::ToggleDoor(
//Toggles a door either open or shut.
//
//Params:
	const UINT wX, const UINT wY, //(in) Coords of any square of the door to toggle.
	CCueEvents& CueEvents) //(in/out)
{
	if (bIsOpenDoor(GetOSquare(wX, wY)))
		CloseDoor(wX, wY, CueEvents);
	else
		OpenDoor(wX, wY);
}

//*****************************************************************************
void CDbRoom::ActivateFiretrap(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	CueEvents.Add(CID_Firetrap, new CCoord(wX, wY), true);

	//Deal with mist separately as it may be hiding under another tile
	//Process this first because a firetrap can turn itself off after exploding a keg or bomb that hits an orb
	//This is easier than making the logic within account for this
	DestroyMist(wX, wY, CueEvents);

	//Tile interactions
	UINT tTile = this->GetTSquare(wX, wY);

	switch (tTile) {
		case T_ORB:
			ActivateOrb(wX, wY, CueEvents, OAT_Item);
		break;
		case T_BOMB:
			ExplodeBomb(CueEvents, wX, wY);
		break;
		case T_POWDER_KEG:
			ExplodePowderKeg(CueEvents, wX, wY);
		break;
		case T_FUSE:
			//Light the fuse.
			if (!this->NewFuses.has(wX, wY) && !this->LitFuses.has(wX, wY))
			{
				this->LitFuses.insert(wX, wY);  //Burn fuse next turn.
				CueEvents.Add(CID_FuseBurning, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
			}
		break;
		case T_TAR: case T_MUD: case T_GEL:
			StabTar(wX, wY, CueEvents, true);
		break;
		case T_CRATE:
			Plot(wX, wY, T_EMPTY);
			CueEvents.Add(CID_CrateDestroyed, new CMoveCoord(wX, wY, NO_ORIENTATION), true);
		break;
	}

	//Damage
	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	bool bPlayerHarmed = this->pCurrentGame->IsPlayerDamagedByFiretrap();
	int damageVal = (int)player.st.firetrapVal;

	if (!damageVal) {
		return;
	}

	if (bPlayerHarmed && player.wX == wX && player.wY == wY) {
		//Burn player
		UINT delta = player.CalcDamage(damageVal);
		player.DecHealth(CueEvents, delta, CID_ExplosionKilledPlayer);
		CueEvents.Add(CID_FiretrapHit, new CCoord(wX, wY));
	}

	CMonster* pMonster = this->GetMonsterAtSquare(wX, wY);
	if (pMonster && pMonster->DamagedByFiretraps()) {
		//Burn monster
		if (pMonster->IsPiece())
		{
			CMonsterPiece* pPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
			pMonster = pPiece->pMonster;
			if (bIsSerpent(pMonster->wType)) {
				//Only burn heads to avoid behaviour influenced by activation order
				return;
			}
		}

		DamageMonster(pMonster, damageVal, CueEvents);
		CueEvents.Add(CID_FiretrapHit, new CCoord(wX, wY));
	}
}

void CDbRoom::DisableFiretrap(const UINT wX, const UINT wY)
{
	const UINT oTile = GetOSquare(wX, wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP_ON)
		Plot(wX, wY, T_FIRETRAP);
}

void CDbRoom::EnableFiretrap(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	const UINT oTile = GetOSquare(wX, wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP) {
		Plot(wX, wY, T_FIRETRAP_ON);
		CueEvents.Add(CID_FiretrapActivated, new CCoord(wX, wY), true);
	}
}

void CDbRoom::ToggleFiretrap(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	const UINT oTile = GetOSquare(wX, wY);
	ASSERT(bIsFiretrap(oTile));
	if (oTile == T_FIRETRAP)
		CueEvents.Add(CID_FiretrapActivated, new CCoord(wX, wY), true);
	Plot(wX, wY, getToggledFiretrap(oTile));
}

//*****************************************************************************
void CDbRoom::DisableForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX, wY);
	ASSERT(bIsAnyArrow(fTile));
	if (bIsArrow(fTile))
		Plot(wX, wY, getToggledForceArrow(fTile));
}

void CDbRoom::EnableForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX, wY);
	ASSERT(bIsAnyArrow(fTile));
	if (bIsDisabledArrow(fTile))
		Plot(wX, wY, getToggledForceArrow(fTile));
}

void CDbRoom::ToggleForceArrow(const UINT wX, const UINT wY)
{
	const UINT fTile = GetFSquare(wX, wY);
	ASSERT(bIsAnyArrow(fTile));
	Plot(wX, wY, getToggledForceArrow(fTile));
}

//*****************************************************************************
void CDbRoom::ToggleLight(
//Toggles a light on or off.
//
//Params:
	const UINT wX, const UINT wY, //(in) Coords of a light.
	CCueEvents& CueEvents)
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	SetTParam(wX,wY,(tParam + LIGHT_OFF) % 256);
	CueEvents.Add(CID_LightToggled);
	this->disabledLights.insert(wX,wY);
}

void CDbRoom::TurnOffLight(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	if (tParam < LIGHT_OFF)
	{
		SetTParam(wX,wY,tParam + LIGHT_OFF);
		CueEvents.Add(CID_LightToggled);
	}
}

void CDbRoom::TurnOnLight(const UINT wX, const UINT wY, CCueEvents& CueEvents)
{
	ASSERT(bIsLight(GetTSquare(wX,wY)));
	const UINT tParam = GetTParam(wX,wY);
	if (tParam >= LIGHT_OFF)
	{
		SetTParam(wX,wY,tParam - LIGHT_OFF);
		CueEvents.Add(CID_LightToggled);
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
	this->bBetterVision = false;
//	this->bPersistentCitizenMovement = false;
	this->mapMarker = 0;
//	this->bIsRequired = false;
	this->bIsSecret = false;
	this->style.resize(0);

	delete[] this->pszOSquares;
	this->pszOSquares = NULL;

	delete[] this->pszFSquares;
	this->pszFSquares = NULL;

	delete[] this->pszTSquares;
	this->pszTSquares = NULL;

	delete[] this->pszTParams;
	this->pszTParams = NULL;

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
/*
	for (wIndex=this->stations.size(); wIndex--; )
		delete this->stations[wIndex];
	this->stations.clear();

	this->halphEnters.clear();
	this->slayerEnters.clear();

	this->checkpoints.clear();
*/

	this->ExtraVars.Clear();
	this->weather.clear();
	this->bridges.clear();
//	this->building.clear();
	this->mistVents.clear();

//	this->bCheckForHoldMastery = false;
	this->bTarWasStabbed = false;
//	this->bGreenDoorsOpened = false;
//	this->pLastClone = NULL;

/*
	DeletePathMaps();

	this->wMonsterCount = this->wBrainCount = 0;
*/
	ClearMonsters();
	ClearDeadMonsters();
	ClearPlatforms();
	this->deletedScrollIDs.clear();
	this->deletedSpeechIDs.clear();
	this->deletedDataIDs.clear();

	ClearPushInfo();
	ClearStateVarsUsedDuringTurn();

/*
	//These should have been cleared by the calls above.
	ASSERT(this->Decoys.empty());
	ASSERT(this->stalwarts.empty());
*/
}

//*****************************************************************************
void CDbRoom::ClearPushInfo()
{
	for (set<const RoomObject*>::const_iterator it = this->pushed_objects.begin();
		it != this->pushed_objects.end(); it++)
	{
		delete *it;
	}
	this->pushed_objects.clear();
}

//*****************************************************************************
void CDbRoom::ClearStateVarsUsedDuringTurn()
{
	this->stationary_powder_kegs.clear();
	this->stabbed_powder_kegs.Clear();
}

//*****************************************************************************
void CDbRoom::UnlinkMonster(CMonster* pMonster)
//Removes the specified monster from the monster list.
{
	//Remove monster's tiles from the room's monster array.
	UINT tileNo = ARRAYINDEX(pMonster->wX,pMonster->wY);
	if (pMonster == this->pMonsterSquares[tileNo])
		this->pMonsterSquares[tileNo] = NULL;

	//All pieces are removed.
	for (list<CMonsterPiece*>::const_iterator piece=pMonster->Pieces.begin();
			piece != pMonster->Pieces.end(); ++piece)
	{
		CMonsterPiece *pPiece = *piece;
		tileNo = ARRAYINDEX(pPiece->wX,pPiece->wY);
		if (pPiece == this->pMonsterSquares[tileNo])
			this->pMonsterSquares[tileNo] = NULL;
	}

	if (pMonster->pPrevious)
		pMonster->pPrevious->pNext = pMonster->pNext;
	if (pMonster->pNext)
		pMonster->pNext->pPrevious = pMonster->pPrevious;
	if (pMonster == this->pLastMonster)
		this->pLastMonster = pMonster->pPrevious;
	if (pMonster == this->pFirstMonster)
		this->pFirstMonster = pMonster->pNext;

	//Retain pMonster's pNext and pPrevious pointers intact, so that
	//monster traversal in ProcessMonsters() isn't disrupted if many
	//monsters die (e.g. by a bomb explosion) at the same time.
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
//Unpacks squares from database into a format that the game will use.
//Must support backward compatibility!
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

	const BYTE *pRead = pSrc, *pStopReading = pRead + dwSrcSize;

	//1. Check SquaresBytes data version.
	const BYTE version = *(pRead++);
	// The version for PackSquares() was 5 when development of DROD RPG
	// started, so there is no RPG data in existence with version < 5.
	ASSERT(version >= 5);
	if (version < 5 || version > 8)
		return false; //DROD RPG supports up to room data format version 8.

	char *pWriteO = this->pszOSquares, *pWriteF = this->pszFSquares, *pWriteT = this->pszTSquares;  //shorthand
	UINT *pWriteTP = this->pszTParams;
	BYTE numTiles;
	char tileNo;
	UINT param=0;

	//2. Read o-layer.
	const char *pStopOWriting = pWriteO + dwSquareCount;
	while (pWriteO < pStopOWriting)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;
 
		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		ASSERT(pWriteO + numTiles <= pStopOWriting);
		tileNo = *(pRead++);

		if (tileNo == T_PRESSPLATE_BROKEN_VIRTUAL) {
			//When loading data for a saved game, extract alternative orb type from t-param value
			tileNo = T_PRESSPLATE; //don't keep value around; was only used to load pressure plate state

			const UINT index = pWriteO - this->pszOSquares;
			COrbData* pOrb = GetOrbAtCoords(ROOMINDEX_TO_X(index), ROOMINDEX_TO_Y(index));
			if (pOrb) {
				pOrb->eType = OT_BROKEN;
			}
		}

		ASSERT (tileNo < TILE_COUNT); //all tile types should be recognized
		while (numTiles--)
			*(pWriteO++) = tileNo;
	}
	//All squares in layer should have been read without excess.
	if (pWriteO != pStopOWriting) return false;

	//3. Read f-layer.
	const char *pStopFWriting = pWriteF + dwSquareCount;
	while (pWriteF < pStopFWriting)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		ASSERT(pWriteF + numTiles <= pStopFWriting);
		tileNo = *(pRead++);
		ASSERT (tileNo < TILE_COUNT); //all tile types should be recognized
		while (numTiles--)
			*(pWriteF++) = tileNo;
	}
	//All squares in layer should have been read without excess.
	if (pWriteF != pStopFWriting) return false;

	//4 and 5. Read t-layer and tile parameters.
	const char *pStopTWriting = pWriteT + dwSquareCount;
	while (pWriteT < pStopTWriting)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		numTiles = *(pRead++);
		ASSERT(numTiles); //having a run-length of zero is wasteful
		ASSERT(pWriteT + numTiles <= pStopTWriting);
		tileNo = *(pRead++);
		ASSERT (tileNo < TILE_COUNT); //all tile types should be recognized
		if (version >= 6)
			param = readBpUINT(pRead);
		else
			param = *(pRead++); //unpack a BYTE into a UINT

		if (tileNo == T_ORB && param != 0) {
			//When loading data for a saved game, extract alternative orb type from t-param value
			const UINT index = pWriteT - this->pszTSquares;
			COrbData* pOrb = GetOrbAtCoords(ROOMINDEX_TO_X(index), ROOMINDEX_TO_Y(index));
			if (pOrb) {
				pOrb->eType = OrbType(param);
				param = 0; //don't keep value around; was only used to load orb state
			}
		}

		while (numTiles--)
		{
			*(pWriteT++) = tileNo;
			*(pWriteTP++) = param;
		}
	}
	//All squares in layer should have been read without excess.
	if (pWriteT != pStopTWriting) return false;

	//6. Read coveredTSquares.
	if (version >= 7)
	{
		//Don't read past end of buffer.
		if (pRead >= pStopReading) return false;

		//Since coveredTSquares is usually sparse, the format here
		//lists non-zero entries rather than runs of identical entries.
		UINT numCoveredTiles = readBpUINT(pRead);
		while (numCoveredTiles--)
		{
			if (pRead >= pStopReading) return false;

			UINT dwCoord = readBpUINT(pRead);
			tileNo = *(pRead++);
			ASSERT(dwCoord < dwSquareCount);
			ASSERT(tileNo < TILE_COUNT);
			this->coveredTSquares.Set(dwCoord % this->wRoomCols, dwCoord / this->wRoomCols, tileNo);
		}
	}

	//7. Read overhead layer (version 8+)
	if (version >= 8)
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

   //Source buffer should contain data for exactly the number of squares in 
	//the room.
	if (pRead != pStopReading) return false;

	return true;
}

//*****************************************************************************
bool CDbRoom::AllocTileLayers()
{
	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);

	if (!this->overheadTiles.Init(this->wRoomCols, this->wRoomRows))
		return false;

	if (!this->pszOSquares)
		this->pszOSquares = new char[dwSquareCount];
	if (!this->pszOSquares) return false;
	if (!this->pszFSquares)
		this->pszFSquares = new char[dwSquareCount];
	if (!this->pszFSquares) { delete[] this->pszOSquares; return false; }
	if (!this->pszTSquares)
		this->pszTSquares = new char[dwSquareCount];
	if (!this->pszTSquares) { delete[] this->pszOSquares; delete[] this->pszFSquares; return false; }
	if (!this->pszTParams)
		this->pszTParams = new UINT[dwSquareCount];
	if (!this->pszTParams) { delete[] this->pszOSquares; delete[] this->pszOSquares; delete[] this->pszTSquares; return false; }
	if (!this->pMonsterSquares)
		this->pMonsterSquares = new CMonster * [dwSquareCount];
	if (!this->pMonsterSquares) { delete[] this->pszOSquares; delete[] this->pszOSquares; delete[] this->pszTSquares; delete[] this->pszTParams; return false; }
	memset(this->pMonsterSquares, 0, dwSquareCount * sizeof(CMonster*));
	this->coveredTSquares.Init(this->wRoomCols, this->wRoomRows, T_EMPTY);

	return true;
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
CMonster* CDbRoom::LoadMonster(const c4_RowRef& row)
//Returns: pointer to monster object
{
	//Note: To test a monster which is not in any of the available rooms,
	//it is useful to change the line below to replace the monster type
	//with the one you'd like to see.
	const UINT wMonsterType = p_Type(row);
	const UINT wX = p_X(row), wY = p_Y(row);

	CMonster *pNew = AddNewMonster((MONSTERTYPE)wMonsterType, wX, wY, true, false);
	if (!pNew) throw CException("CDbRoom::LoadMonster: Alloc failed");
//	pNew->bIsFirstTurn = (p_IsFirstTurn(row) == 1);
	pNew->wO = pNew->wPrevO = p_O(row);
	const CDbPackedVars vars = p_ExtraVars(row);
	pNew->SetMembers(vars);

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
		ASSERT(wNumPieces);
		UINT wTempX, wTempY;
		pSerpent->GetTail(wTempX, wTempY);
	}

	LinkMonster(pNew);

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
	try {

	UINT wMonsterI;
	const UINT wMonsterCount = MonstersView.GetSize();
	for (wMonsterI = 0; wMonsterI < wMonsterCount; ++wMonsterI)
	{
		c4_RowRef row = MonstersView[wMonsterI];
		CMonster *pNew = LoadMonster(row);
		if (!pNew) continue;

		if (pNew->wType == M_CHARACTER)
		{
			//Character doesn't appear unless script indicates.
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
			if (!pCharacter->IsVisible())
				this->pMonsterSquares[ARRAYINDEX(pNew->wX,pNew->wY)] = NULL;
		}
	}

	}
	catch (CException&)
	{
		ClearMonsters();
		return false;
	}
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
void CDbRoom::ClearMonsters()
//Puts room in a monster-free state.  Note that when a room is conquered,
//monsters that don't influence the conquering criterion should be retained.
{
	//Traverse backwards, with order of retained monsters the same as before.
	CMonster *pDelete, *pCharacters = NULL, *pSeek = this->pLastMonster;
	this->pLastMonster = NULL;

	while (pSeek)
	{
		pDelete = pSeek;
		pSeek = pSeek->pPrevious;
		if (this->pMonsterSquares)
		{
			this->pMonsterSquares[ARRAYINDEX(pDelete->wX, pDelete->wY)] = NULL;
			for (list<CMonsterPiece*>::iterator piece = pDelete->Pieces.begin();
					piece != pDelete->Pieces.end(); ++piece)
				this->pMonsterSquares[ARRAYINDEX((*piece)->wX, (*piece)->wY)] = NULL;
		}
		delete pDelete;
	}
	this->pFirstMonster = pCharacters;  //usually NULL
/*
	this->wMonsterCount = 0;
	this->wBrainCount = 0;
*/
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
void CDbRoom::ClearPlatforms()
//Frees memory and resets members for 'platforms'.
{
	for (UINT wIndex=this->platforms.size(); wIndex--; )
		delete this->platforms[wIndex];
	this->platforms.clear();
	CPlatform::clearFallTiles();
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
/*
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

//-****************************************************************************
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
			GetConnectedTiles(wX, wY, doorMask, true, squares8);

			//Remove door tiles affected by agent's 4-neighbor connected component.
			GetConnectedTiles(wX, wY, doorMask, false, squares4);
			squares8 -= squares4;

			//Set an agent to any other 4-connected piece.
			while (squares8.pop_first(wX,wY))
			{
				pOrb->AddAgent(wX, wY, pAgent->action);
				GetConnectedTiles(wX, wY, doorMask, false, squares4);
				squares8 -= squares4;
			}
		}
	}
}
*/

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
	GetConnectedTiles(pPlate->wX, pPlate->wY, mask, false, pPlate->tiles);
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
	GetConnectedTiles(wX, wY, mask, false, platform);
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
		if (wX >= stairs.wLeft && wX <= stairs.wRight &&
				wY >= stairs.wTop && wY <= stairs.wBottom) 
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

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		pMonster->SetCurrentGame(pSetCurrentGame);
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
void CDbRoom::SetHoldForMonsters(CDbHold* pHold)
//A back door call to set custom NPC identity for its hold when not in play
{
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->ResolveLogicalIdentity(pHold);
		}
		pMonster = pMonster->pNext;
	}
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
		if (y >= this->wRoomRows)
			continue;
		const int dy = y-ty+1;
		const UINT yIndex = y * this->wRoomCols;
		for (x=tx-1; x!=endX; ++x)
		{
			if (x >= this->wRoomCols)
				continue;
			const int dx = x-tx+1;
			if (dx == 1 && dy == 1)
				continue;
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
//Determines whether new tar/mud could be placed at this spot (as opposed to a tar baby)
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
//Compiles set of all contiguous (connected component) squares containing tar
//tiles of indicated type, starting from (wX,wY).  If there is no tile of
//indicated type at the starting coords, outputs an empty set.
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
		cstack.Push((wX), (wY));} while (0)

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
CMonster* CDbRoom::GetMotherConnectedToTarTile(const UINT wX, const UINT wY) const
//Calculates whether tarstuff at (x,y) is part of a tarstuff component
//connected to a mother.
//
//Any monster on the tarstuff counts as a mother for this 
//
//Returns: if the specified tile is connected to a mother by tarstuff,
// return a pointer to the mother, otherwise NULL
{
	if (!IsValidColRow(wX,wY))
		return NULL;

	UINT wTarType = GetTSquare(wX,wY);
	if (!bIsTar(wTarType))
		return NULL;

	CCoordSet contiguousTar;
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (!pMonster->bAlive)
			continue;

		//Calculate connected components contiguous to existing mothers.  Skip redundancies.

		//If a Mother doesn't have tarstuff under it, then consider it not connected to any tarstuff.
		if (GetTSquare(pMonster->wX, pMonster->wY) != wTarType)
			continue;

		if (!contiguousTar.has(pMonster->wX, pMonster->wY))
		{
			CCoordSet tiles;
			GetTarConnectedComponent(pMonster->wX, pMonster->wY, tiles);
			if (tiles.has(wX,wY))
				return pMonster; //found a tile connected to a mother by tarstuff

			contiguousTar += tiles; //don't repeat these tiles
		}
	}

	return NULL;
}

//*****************************************************************************
bool CDbRoom::StabTar(
//Update game for results of stabbing a square containing tar.
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
	
	if (removeTarNow)
	{
		//Remove tar (effect of possibly simultaneous hits).
		//In this case, checking for vulnerability of this spot should have been
		//performed previously.
		const UINT wTSquare = GetTSquare(wX, wY);
		if (bIsTar(wTSquare))
		{
			RemoveStabbedTar(wX, wY, CueEvents);
			ConvertUnstableTar(CueEvents);
			CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wX, wY,	wStabO, wTSquare), true);
			this->bTarWasStabbed = true;
			return true;
		}
		return false;
	}

	//If the tar is vulnerable, flag to remove it.
	return IsTarVulnerableToStab(wX, wY);
}

//*****************************************************************************
UINT CDbRoom::GetBrainsPresent() const
//Returns: the number of brain monsters in the room (including NPCs)
{
	UINT wCount=0;
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_BRAIN)
			++wCount;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->GetIdentity() == M_BRAIN && pCharacter->IsVisible())
				++wCount;
		}
	}
	return wCount;
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
							((UINT)this->pszTSquares[ARRAYINDEX(x,y)] == wTarType);

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
		return wTSquare == T_GEL;

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
//Removes tarstuff found in a square in response to the tar being stabbed.  Adjacent
//tarstuff squares may change into tar babies as a result.  Does not check that stab
//is directed at a vulnerable tar square.
//
//Any mother on an otherwise unstable tarstuff tile maintains the tarstuff underneath it.
//
//Any monster on tarstuff counts as a mother for this check.
//
//Params:
	const UINT wX, const UINT wY, //(in)  Square containing tar to remove.
	CCueEvents &CueEvents)        //(out) May receive cue events.
{
	const UINT wTarType = GetTSquare(wX, wY);
	ASSERT(bIsTar(wTarType));

	//Easy part--remove the tar.
	DestroyTar(wX, wY, CueEvents);

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
					this->NewBabies.IsMember(i, j))
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
			if (!pMonster) //any monster counts as a mother for this check
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
//Transforms marked unstable tarstuff in babyOrder into new babies.
//
//Params:
	CCueEvents &CueEvents,        //(out) May receive cue events.
	const bool bDelayBabyMoves)   //(in)  Whether babies can move immediately when processed [default=false]
{
	const UINT wSX = this->pCurrentGame ? this->pCurrentGame->pPlayer->wX : (UINT)-1;
	const UINT wSY = this->pCurrentGame ? this->pCurrentGame->pPlayer->wY : (UINT)-1;
	CCoordIndex swordCoords;
	GetSwordCoords(swordCoords);

	UINT wX, wY;
	while (this->NewBabies.PopBottom(wX,wY)) //process as queue
	{
		const UINT wTarType = GetTSquare(wX,wY);
		if (bIsTar(wTarType))
		{
			//Get rid of the unstable tar
			DestroyTar(wX, wY, CueEvents);

			CMonster *pMonster = GetMonsterAtSquare(wX,wY);

			//Don't spawn tar babies under living monsters
			if (this->pCurrentGame && !pMonster)
			{
				//Spawn a tarbaby.
				const UINT wOSquare = GetOSquare(wX,wY);

				//Don't create babies on swords, stairs, pits or the player.
				if (!(swordCoords.Exists(wX,wY) || bIsStairs(wOSquare) || bIsPit(wOSquare) ||
						bIsWater(wOSquare) || (wX == wSX && wY == wSY)))
				{
					//Spawn a tarbaby or variant.
					UINT monsterID;
					CMonster *m = NULL;
					const UINT wO = nGetO(sgn(wSX - wX), sgn(wSY - wY));
					switch (wTarType)
					{
						case T_TAR:
							monsterID = this->pCurrentGame->getSpawnID(M_TARBABY);
							m = this->pCurrentGame->AddNewEntity(CueEvents, monsterID, wX, wY, wO, true);
							if (m)
								CueEvents.Add(CID_TarBabyFormed, m);
							break;
						case T_MUD:
							monsterID = this->pCurrentGame->getSpawnID(M_MUDBABY);
							m = this->pCurrentGame->AddNewEntity(CueEvents, monsterID, wX, wY, wO, true);
							if (m)
								CueEvents.Add(CID_MudBabyFormed, m);
							break;
						case T_GEL:
							monsterID = this->pCurrentGame->getSpawnID(M_GELBABY);
							m = this->pCurrentGame->AddNewEntity(CueEvents, monsterID, wX, wY, wO, true);
							if (m)
								CueEvents.Add(CID_GelBabyFormed, m);
							break;
						default: ASSERT(!"Bad tar type"); continue;
					}
				}
			}
		}
	}
	
	this->NewBabies.Clear();
}

//*****************************************************************************
void CDbRoom::SwitchTarstuff(const UINT wType1, const UINT wType2)
//Switch these two tarstuff types in the room, including monster types.
{
	CMonsterFactory mf(this->pCurrentGame);

	const bool bTar = wType1 == T_TAR || wType2 == T_TAR;
	const bool bMud = wType1 == T_MUD || wType2 == T_MUD;
	const bool bGel = wType1 == T_GEL || wType2 == T_GEL;

	const int tarSpawnId = this->pCurrentGame->getSpawnID(M_TARBABY);
	const int mudSpawnId = this->pCurrentGame->getSpawnID(M_MUDBABY);
	const int gelSpawnId = this->pCurrentGame->getSpawnID(M_GELBABY);
	const int tarSwapId = this->pCurrentGame->getSpawnID(M_TARMOTHER);
	const int mudSwapId = this->pCurrentGame->getSpawnID(M_MUDMOTHER);
	const int gelSwapId = this->pCurrentGame->getSpawnID(M_GELMOTHER);

	//Don't try to swap from or to multi-tile monsters
	if ((bTar && bIsLargeMonster(tarSpawnId)) ||
		(bMud && bIsLargeMonster(mudSpawnId)) ||
		(bGel && bIsLargeMonster(gelSpawnId))) {
		return; 
	}

	UINT wX, wY;
	CCueEvents Ignored;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			const UINT wTTile = GetTSquare(wX,wY);
			if (wTTile == wType1)
				Plot(wX,wY, wType2);
			else if (wTTile == wType2)
				Plot(wX,wY, wType1);

			//Swap monsters. Multi-tile monsters can't be swapped
			CMonster *pMonster = GetMonsterAtSquare(wX,wY);
			if (pMonster && !pMonster->IsLongMonster())
			{
				int mType = pMonster->GetLogicalIdentity();
				int nType = -1;

				if (bTar && mType == tarSpawnId) {
					nType = bMud ? mudSpawnId : gelSpawnId;
				}
				else if (bMud && mType == mudSpawnId) {
					nType = bTar ? tarSpawnId : gelSpawnId;
				}
				else if (bGel && mType == gelSpawnId) {
					nType = bMud ? mudSpawnId : tarSpawnId;
				}
				else if (bTar && mType == tarSwapId) {
					nType = bMud ? mudSwapId : gelSwapId;
				}
				else if (bMud && mType == mudSwapId) {
					nType = bTar ? tarSwapId : gelSwapId;
				}
				else if (bGel && mType == gelSwapId) {
					nType = bMud ? mudSwapId : tarSwapId;
				}

				if (nType != -1 && !bIsLargeMonster(nType) &&
					(IsValidMonsterType(nType) || this->pCurrentGame->pHold->GetCharacter(nType)))
				{
					//Create new monster and insert in list where old monster was.
					CMonster* pNew;
					if (IsValidMonsterType(nType)) {
						pNew = mf.GetNewMonster((const MONSTERTYPE)nType);
					}
					else {
						//hold character
						pNew = mf.GetNewMonster(M_CHARACTER);
						//Set up NPC info.
						CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
						pCharacter->wLogicalIdentity = nType;
						pCharacter->SetCurrentGame(this->pCurrentGame); //will assign the default script for custom NPCs
						pCharacter->dwScriptID = this->pCurrentGame->getNewScriptID();
						pCharacter->bVisible = true;
					}

					pNew->wX = pNew->wPrevX = wX;
					pNew->wY = pNew->wPrevY = wY;
					pNew->wO = pNew->wPrevO = pMonster->wO;
//					++this->wMonsterCount;

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

					pMonster->pNext = pMonster->pPrevious = NULL;
					KillMonster(pMonster, Ignored);

					SetMonsterSquare(pNew);
				}
			}
		}
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
	CMonster *pIgnore) //[default=NULL] optional monster to ignore in tally
const
{
	UINT wSX, wSY;
	DoubleSwordCoords.Init(this->wRoomCols, this->wRoomRows);
	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster != pIgnore && pMonster->GetSwordCoords(wSX, wSY))
			if (IsValidColRow(wSX, wSY))
				DoubleSwordCoords.Add(wSX, wSY, 1+pMonster->wO); //avoid 0 values
		pMonster = pMonster->pNext;
	}
}

//*****************************************************************************
bool CDbRoom::SwordfightCheck() const
//Returns: true if two or more swords occupy the same square and at least one
//of them just moved to this square, else false.
{
	ASSERT(this->pCurrentGame);

	CCoordSet
		swordMoved,	//sword just moved to this square
		swords;	//where swords are located

	UINT wSX, wSY;
	CSwordsman& player = *this->pCurrentGame->pPlayer;
	if (player.HasSword())
	{
		swords.insert(player.GetSwordX(), player.GetSwordY());

		//Check whether player is moving sword.
		if (player.wSwordMovement != NO_ORIENTATION)
			swordMoved.insert(player.GetSwordX(), player.GetSwordY());
	}

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->GetSwordCoords(wSX, wSY))
		{
			CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
			if (pDouble->wSwordMovement != NO_ORIENTATION)
				swordMoved.insert(wSX, wSY);
			if (swords.has(wSX, wSY) && swordMoved.has(wSX, wSY))
				return true;
			swords.insert(wSX, wSY);
		}
		pMonster = pMonster->pNext;
	}
	return false;
}

//*****************************************************************************
void CDbRoom::GetSwordCoords(
//Gets a coord index containing coords of all swords (player & doubles).
//If swords are out of the room boundaries, don't add them.
//
//Params:
	CCoordIndex &SwordCoords,  //(out) Uninitialized.
	CMonster *pIgnore) //[default=NULL] optional monster to ignore in tally
const
{
	GetDoubleSwordCoords(SwordCoords, pIgnore);
	if (this->pCurrentGame && this->pCurrentGame->pPlayer->HasSword() && IsValidColRow(
			this->pCurrentGame->pPlayer->GetSwordX(), this->pCurrentGame->pPlayer->GetSwordY()))
		SwordCoords.Add(this->pCurrentGame->pPlayer->GetSwordX(),
				this->pCurrentGame->pPlayer->GetSwordY(), 1+this->pCurrentGame->pPlayer->wO); //avoid 0 values
}

//*****************************************************************************
bool CDbRoom::IsMonsterSwordAt(
//Determines if a square contains a double's sword.
//
//For efficiency, use GetDoubleSwordCoords() when you need to check several 
//squares for a double's sword.  Use this method when you only need to check 
//one or two.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Square to check.
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
				if (pMonster && pMonster != pIgnore && pMonster->HasSwordAt(wX, wY))
					return true;
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
void CDbRoom::ActivateOrb(
//Activates an orb by releasing any associated agents into the current room
//to perform tasks.
//
//Accepts:
	const UINT wX, const UINT wY,    //(in) Orb location.
	CCueEvents &CueEvents,     //(in/out) Appends cue events to list.
	const OrbActivationType eActivationType)     //(in) what activated the orb
{
	const bool bBreakOrb = true; //are there any use cases to set to false?

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
				if (eActivationType == OAT_PressurePlate || eActivationType == OAT_ScriptPlate)
				{
					if (pOrb->bActive)
						return;
					pOrb->bActive = true;
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

	if (bAttached)
		return;  //No orb information -- nothing to do.

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
					ToggleLight(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsDoor(oTile) || bIsOpenDoor(oTile))
					ToggleDoor(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsFiretrap(oTile))
					ToggleFiretrap(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsAnyArrow(fTile))
					ToggleForceArrow(pAgent->wX, pAgent->wY);
			break;

			case OA_OPEN:
				if (bIsLight(wTTile))
					TurnOnLight(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsDoor(oTile) || bIsOpenDoor(oTile))
					OpenDoor(pAgent->wX, pAgent->wY);
				else if (bIsFiretrap(oTile))
					EnableFiretrap(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsAnyArrow(fTile))
					DisableForceArrow(pAgent->wX, pAgent->wY);
			break;

			case OA_CLOSE:
				if (bIsLight(wTTile))
					TurnOffLight(pAgent->wX, pAgent->wY, CueEvents);
				else if (bIsDoor(oTile) || bIsOpenDoor(oTile))
					CloseDoor(pAgent->wX, pAgent->wY, CueEvents);
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
		this->PlotsMade.insert(wX, wY);
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
	GetConnectedTiles(wX, wY, mask, b8Neighbor, squares);

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
		if (ToggleTiles(T_DOOR_R,T_DOOR_RO)) //red doors
			CueEvents.Add(CID_RedGatesToggled);
	}
}

//*****************************************************************************
void CDbRoom::IncTrapdoor(CCueEvents& CueEvents)
//Updates room state when a new trapdoor is added to the room during play.
{
	if (!this->wTrapDoorsLeft)
	{
		if (ToggleTiles(T_DOOR_R,T_DOOR_RO)) //red doors
			CueEvents.Add(CID_RedGatesToggled);
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
void CDbRoom::Dig(const UINT wX, const UINT wY, const UINT wO, CCueEvents& CueEvents)
{
	const UINT oTile = GetOSquare(wX, wY);
	ASSERT(bIsDiggableBlock(oTile));
	CueEvents.Add(CID_Dig, new CMoveCoordEx(wX, wY, wO, oTile), true);
	Plot(wX, wY, T_FLOOR_DIRT);
}

//*****************************************************************************
void CDbRoom::DestroyTar(
//Remove tar at square.
//
//Params:
	const UINT wX, const UINT wY, //(in)
	CCueEvents &CueEvents)     //(in/out)
{
	ASSERT(bIsTar(GetTSquare(wX,wY)));
	Plot(wX, wY, T_EMPTY);
	ASSERT(this->wTarLeft); //This function should never be called with 0 Tar squares
	--this->wTarLeft;
	if (!this->wTarLeft)
	{
		CueEvents.Add(CID_AllTarRemoved);
		ToggleBlackGates(CueEvents);
	}
}

//******************************************************************************
void CDbRoom::DestroyMist(
//Destroy mist due to a built wall/door.
//
//Params:
	const UINT wX, const UINT wY, //(in)
	CCueEvents& CueEvents)     //(in/out)
{
	const UINT wOSquare = GetOSquare(wX, wY);
	ASSERT(bIsSolidOTile(wOSquare) || wOSquare == T_HOT || wOSquare == T_FIRETRAP_ON);

	if (GetTSquare(wX, wY) == T_MIST)
	{
		Plot(wX, wY, T_EMPTY);
		CueEvents.Add(CID_MistDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, T_MIST), true);
	} else if (GetCoveredTSquare(wX, wY) == T_MIST) {
		this->coveredTSquares.Remove(wX, wY);
		CueEvents.Add(CID_MistDestroyed, new CMoveCoordEx(wX, wY, NO_ORIENTATION, T_MIST), true);
	}
}

//******************************************************************************
void CDbRoom::DestroyTrapdoor(
//Plots a pit to a trapdoor square.
//If there is a scroll or power up item there also, remove it.
//Updates red doors as needed.
//
//Params:
	const UINT wX, const UINT wY,    //(in)
	CCueEvents &CueEvents)     //(in/out)
{
	const UINT oldOTile = GetOSquare(wX, wY);
	const UINT newOTile = this->coveredOSquares.GetAt(wX, wY);
	ASSERT(bIsFallingTile(GetOSquare(wX,wY)));
	ASSERT(bIsPit(newOTile) || bIsWater(newOTile));
	Plot(wX, wY, newOTile);

	CheckForFallingAt(wX, wY, CueEvents, true);
	ConvertUnstableTar(CueEvents);

	if (bIsTrapdoor(oldOTile)) {
		CueEvents.Add(CID_TrapDoorRemoved, new CCoord(wX, wY), true);
		DecTrapdoor(CueEvents);
	} else {
		ASSERT(bIsThinIce(oldOTile));
		CueEvents.Add(CID_ThinIceMelted, new CCoord(wX, wY), true);
	}
}

//*****************************************************************************
/*
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
*/

//*****************************************************************************
/*
bool CDbRoom::RemoveLongMonsterPieces(
//Removes all the pieces of a long monster (besides the head).
//
//Returns: whether monster was successfully removed
//
//Params:
	CMonster *pMonster)  //(in) Long monster
{
	ASSERT(pMonster->IsLongMonster());

	while (pMonster->Pieces.size() > 0)
	{
		CMonsterPiece *pMPiece = *(pMonster->Pieces.begin());
		Plot(pMPiece->wX, pMPiece->wY, T_NOMONSTER);
	}
	return true;
}
*/

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
{
	const bool bToggled = ToggleTiles(T_DOOR_B,T_DOOR_BO);
	if (bToggled)
		CueEvents.Add(CID_BlackGatesToggled);
}

//*****************************************************************************
bool CDbRoom::ToggleTiles(const UINT wOldTile, const UINT wNewTile)
//Exchanges any occurrence of wOldTile in the room with wNewTile.
//
//Returns:
//True if any tiles were found
{
	ASSERT(TILE_LAYER[wOldTile] == 0);  //o-layer only implemented currently
	ASSERT(TILE_LAYER[wNewTile] == 0);

	bool bChangedTiles=false, bThisTileChanged=false;
	UINT wTile;
	for (UINT wY=wRoomRows; wY--; )
		for (UINT wX=wRoomCols; wX--; )
		{
			wTile = GetOSquare(wX,wY);
			if (wTile == wOldTile)
			{
				Plot(wX, wY, wNewTile);
				bThisTileChanged=bChangedTiles=true;
			} else if (wTile == wNewTile)	{
				Plot(wX, wY, wOldTile);
				bThisTileChanged=bChangedTiles=true;
			}

			//If a door has closed, it cuts fuses and removes mist.
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
							this->LitFuses.erase(wX, wY);
						break;
						case T_MIST:
							Plot(wX, wY, T_EMPTY);
						break;
						default: break;
					}
				}
			}
		}
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
	CMonster *pMonster)  //(in) default=NULL
{
	ASSERT(IsValidTileNo(wTileNo) || wTileNo == T_NOMONSTER || wTileNo == T_EMPTY_F);
	ASSERT(IsValidColRow(wX, wY));

	const UINT wSquareIndex = ARRAYINDEX(wX,wY);
	switch (TILE_LAYER[wTileNo])
	{
		case 0: //Opaque layer.
		{
			this->pszOSquares[wSquareIndex] = static_cast<unsigned char>(wTileNo);
//			RecalcStationPaths();
			this->bridges.Plotted(wX,wY,wTileNo);
			this->PlotsMade.insert(wX,wY);
			this->geometryChanges.insert(wX,wY);  //always assume changes to o-layer affect room geometry for easier maintenance

			if (wTileNo == T_MISTVENT) {
				this->mistVents.insert(wX, wY);
			} else {
				this->mistVents.erase(wX, wY);
			}

			if (wTileNo == T_FIRETRAP_ON) {
				this->activeFiretraps.insert(wX, wY);
			}
			else {
				this->activeFiretraps.erase(wX, wY);
			}

			//Specific plot types require in-game stat updates.
			switch (wTileNo)
			{
				case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD:
				case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
				case T_FLOOR_IMAGE:
				case T_HOT:
				case T_WATER: case T_PIT: case T_PIT_IMAGE:
				case T_MISTVENT:
				case T_FIRETRAP: case T_FIRETRAP_ON:
				case T_DIRT1: case T_DIRT3: case T_DIRT5:
					this->coveredOSquares.Add(wX,wY,wTileNo);
				break;
				case T_GOO:
					//If goo is placed on non-floor, then floor gets placed under it.
					if (!bIsPlainFloor(this->coveredOSquares.GetAt(wX,wY)))
					{
						this->coveredOSquares.Add(wX,wY,T_FLOOR);
						//If another floor type is adjacent to this tile, use that floor type.
						//This includes floor under adjacent goo, which overrides non-goo floor.
						bool bFloorFound = false, bGooFound = false;
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
								if (wOTile == T_GOO && !bGooFound)
								{
									this->coveredOSquares.Add(wX,wY, this->coveredOSquares.GetAt(wX+nI, wY+nJ));
									bGooFound = bFloorFound = true; //overrides non-goo floor
								}
							}
					}
				break;
				case T_TRAPDOOR:
					this->coveredOSquares.Add(wX,wY,T_PIT); //best guess
				break;
				case T_TRAPDOOR2:
				case T_THINICE:
					this->coveredOSquares.Add(wX,wY,T_WATER);
				break;
				default: break;
			}
		}
		break;

		case 3: //Floor layer.
			this->pszFSquares[wSquareIndex] = static_cast<char>(
					wTileNo == T_EMPTY_F ? T_EMPTY : wTileNo);
//			RecalcStationPaths();
			this->PlotsMade.insert(wX,wY);
		break;

		case 1: //Transparent layer.
		{
			//For front end -- mark when objects that are part of room geometry change.
			const UINT oldTile = this->pszTSquares[wSquareIndex];
			const bool bGeometryChanging = (oldTile == T_ORB || oldTile == T_BOMB) &&
					!(wTileNo == T_ORB || wTileNo == T_BOMB);
			if (bGeometryChanging)
				this->geometryChanges.insert(wX,wY);

			if (wTileNo == T_EMPTY && this->coveredTSquares.GetAt(wX,wY) != T_EMPTY)
			{
				this->pszTSquares[wSquareIndex] = static_cast<char>(
						this->coveredTSquares.GetAt(wX,wY));
				ASSERT(T_EMPTY == 0);
				this->coveredTSquares.Remove(wX,wY); //set to T_EMPTY
			} else
				this->pszTSquares[wSquareIndex] = static_cast<char>(wTileNo);
			if (!this->pCurrentGame)
				this->pszTParams[wSquareIndex] = 0; //only reset outside of play
//			RecalcStationPaths();
			this->PlotsMade.insert(wX,wY);
		}
		break;

		case 2: //Monster layer.
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
			const bool bPlacingMonsterPiece = pMonster && bIsSerpentTile(wTileNo);
			if (bPlacingMonsterPiece)
			{
				ASSERT(pMonster->IsLongMonster());
				CMonsterPiece *pMPiece = new CMonsterPiece(pMonster, wTileNo, wX, wY);
				this->pMonsterSquares[wSquareIndex] = pMPiece;
				pMonster->Pieces.push_back(pMPiece);
			} else {
				this->pMonsterSquares[wSquareIndex] = wTileNo == T_NOMONSTER ? NULL : pMonster;
				if (wTileNo == T_NOMONSTER)
					this->PlotsMade.insert(wX,wY);
			}
		}
		break;

		default: ASSERT(!"Invalid layer"); break;
	}

//	UpdatePathMapAt(wX, wY);
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
	if (this->pCurrentGame && this->pCurrentGame->pPlayer->IsInRoom())
	{
		COrbData *pPlate = GetPressurePlateAtCoords(
				this->pCurrentGame->pPlayer->wX, this->pCurrentGame->pPlayer->wY);
		if (pPlate)
			pPlate->bActive = true;
	}

	//Monsters.
	for (CMonster *pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->IsFlying())
			continue;

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
		COrbData *pPlate = GetPressurePlateAtCoords(pMonster->wX, pMonster->wY);
		if (pPlate)
			pPlate->bActive = true;

		//Monster pieces.
		for (list<CMonsterPiece*>::iterator piece = pMonster->Pieces.begin();
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

//*****************************************************************************
void CDbRoom::SetSwordsSheathed()
{
	CMonster *pMonster;
	for (pMonster = this->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (bEntityHasSword(pMonster->wType) || pMonster->wType == M_CHARACTER)
		{
			CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
			pDouble->SetSwordSheathed();
		}
	}
}

//*****************************************************************************
void CDbRoom::SetTParam(const UINT wX, const UINT wY, const UINT value)
{
	ASSERT(IsValidColRow(wX, wY));
	//currently only used for obstacles, lights, stations and tokens
	ASSERT(GetTSquare(wX,wY) == T_OBSTACLE ||
			bIsLight(GetTSquare(wX,wY)) ||
			GetTSquare(wX,wY) == T_TOKEN ||
			GetTSquare(wX,wY) == T_KEY ||
			bIsEquipment(GetTSquare(wX,wY)));
	this->pszTParams[ARRAYINDEX(wX,wY)] = value;
}

//*****************************************************************************
int CDbRoom::DangerLevel() const
//Returns: int (how dangerous elements in the room make it)
{
	int nDanger = 0;

/*
	if (!this->slayerEnters.empty()) ++nDanger;
	if (!this->halphEnters.empty()) ++nDanger;
*/
	if (this->bTarWasStabbed) ++nDanger;

	CMonster *pMonster = this->pFirstMonster;
	while (pMonster)
	{
		switch (pMonster->wType)
		{
		case M_BRAIN: case M_HALPH:
		case M_ROACH: case M_REGG: case M_WWING: case M_EYE: case M_MADEYE:
		case M_SPIDER:	case M_WUBBA: case M_SEEP:
		case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
		case M_TARBABY: case M_MUDBABY: case M_GELBABY:
		case M_WATERSKIPPER: case M_SKIPPERNEST:
		case M_AUMTLICH: case M_FEGUNDO: case M_FEGUNDOASHES:
		case M_FLUFFBABY:
			++nDanger; break;

		case M_GOBLIN: case M_GUARD: case M_ROCKGIANT: nDanger += 5; break;
		case M_QROACH: nDanger += 10; break;
		case M_NEATHER: case M_SLAYER: nDanger += 15; break;
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER: nDanger += 30; break;

		case M_ROCKGOLEM:
//			if (pMonster->IsAggressive())
				++nDanger;
			break;
		case M_CONSTRUCT:
//			if (pMonster->IsAggressive() || !this->pCurrentGame->IsCurrentRoomPendingExit())
				nDanger += 3;
			break;

		case M_CITIZEN:
//			if (!this->stations.empty())
				++nDanger;
			break;

		case M_CHARACTER:
			if (pMonster->getHP() > 0) //Only characters with HP can be fought, which we should assume are enemies
				++nDanger;
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
bool CDbRoom::IsEitherTSquare(const UINT wX, const UINT wY, const UINT wTile) const
//Returns: If the t-layer or covered t-layer square at a position is the given
//tile type
{
	return this->pszTSquares[ARRAYINDEX(wX, wY)] == wTile ||
		this->coveredTSquares.GetAt(wX, wY) == wTile;
}

//*****************************************************************************
//Saves room squares from member vars of object into database (version 2.0+).
//
//Returns: pointer to record to be saved into database (must be deleted by caller).
c4_Bytes* CDbRoom::PackSquares(
	const bool bSaveGameData //if set [default=false], populate with augmented data about room state for saved game record
) const
{
	const UINT dwSquareCount = CalcRoomArea();
	ASSERT(dwSquareCount);
	char *pSquares = new char[dwSquareCount*18 + 6];  //max size possible
	char *pWrite = pSquares;

	//1. Version of data format.
	*(pWrite++) = 8;

	//Run-length encoding info.
	char lastSquare, square = T_EMPTY;
	BYTE count=0;
	UINT lastParam, param = 0;
	UINT dwSquareI;

	//2. Write opaque squares.
	ASSERT(this->pszOSquares);
	lastSquare = this->pszOSquares[0];
	for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		square = this->pszOSquares[dwSquareI];
		ASSERT(square != T_EMPTY);

		if (bSaveGameData) {
			//Mark current type of pressure plate as special tile type.
			if (square == T_PRESSPLATE) {
				const COrbData* pOrb = GetOrbAtCoords(ROOMINDEX_TO_X(dwSquareI), ROOMINDEX_TO_Y(dwSquareI));
				if (pOrb && pOrb->eType == OT_BROKEN)
					square = T_PRESSPLATE_BROKEN_VIRTUAL;
			}
		}

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
	ASSERT(this->pszTSquares);
	lastSquare = this->pszTSquares[0];
	lastParam = this->pszTParams[0];
	count=0;
	for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
	{
		square = this->pszTSquares[dwSquareI];
		param = this->pszTParams[dwSquareI];

		if (bSaveGameData) {
			//Mark current type of orb in t-param value.
			if (square == T_ORB) {
				COrbData* pOrb = GetOrbAtCoords(ROOMINDEX_TO_X(dwSquareI), ROOMINDEX_TO_Y(dwSquareI));
				if (pOrb) {
					param = pOrb->eType;
				}
			}
		}

		if (square == lastSquare && param == lastParam &&
				count < 255) //a char can store max run length of 255
			++count;
		else
		{
			//Write out old run info.  Start new run.
			ASSERT(count > 0);
			*(pWrite++) = (char)count;
			*(pWrite++) = lastSquare;
			writeBpUINT(pWrite, lastParam); //v6

			lastSquare = square;
			lastParam = param;
			count = 1;
		}
	}
	ASSERT(count > 0);
	*(pWrite++) = (char)count;
	*(pWrite++) = square;
	writeBpUINT(pWrite, param); //v6

	//Added in v7: write coveredTSquares.
	//Since coveredTSquares is usually sparse, the format here
	//lists non-zero entries rather than runs of identical entries.
	ASSERT(this->coveredTSquares.empty() || this->coveredTSquares.GetArea() == dwSquareCount);
	writeBpUINT(pWrite, this->coveredTSquares.GetSize());
	if (!this->coveredTSquares.empty())
	{
		const BYTE *pRawCovered = this->coveredTSquares.GetIndex();
		for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI)
		{
			square = pRawCovered[dwSquareI];
			if (square != T_EMPTY)
			{
				writeBpUINT(pWrite, dwSquareI);
				*(pWrite++) = square;
			}
		}
	}

	//6. Write overhead layer, if necessary.
	if (this->overheadTiles.empty()) {
		*(pWrite++) = (char)0;
	}
	else {
		*(pWrite++) = (char)1; //indicates we have some data for this layer

		const BYTE* pData = this->overheadTiles.GetIndex();
		BYTE val, lastVal = *pData;
		count = 0;
		for (dwSquareI = 0; dwSquareI < dwSquareCount; ++dwSquareI, ++pData)
		{
			val = *pData;

			if (*pData == lastVal && count < 255) {
				++count;
			}
			else {
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
	UINT wCount = 0, wNumMonsters = 0;
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
/*
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
*/

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

	//Prepare props.
	c4_Bytes *pSquaresBytes = PackSquares();
	c4_Bytes *pLightsBytes = PackTileLights();

	c4_View OrbsView;
	SaveOrbs(OrbsView);

	c4_View MonstersView;
	SaveMonsters(MonstersView);

	c4_View ScrollsView;
	SaveScrolls(ScrollsView);

	c4_View ExitsView;
	SaveExits(ExitsView);

	SetExtraVarsFromMembers();
	UINT dwExtraSize;
	BYTE *pbytExtraBytes = this->ExtraVars.GetPackedBuffer(dwExtraSize);
	ASSERT(pbytExtraBytes);
	c4_Bytes ExtraBytes(pbytExtraBytes, dwExtraSize);

	//Update Rooms record.
	p_RoomID(row) = this->dwRoomID;
	p_LevelID(row) = this->dwLevelID;
	p_RoomX(row) = this->dwRoomX;
	p_RoomY(row) = this->dwRoomY;
	p_RoomCols(row) = this->wRoomCols;
	p_RoomRows(row) = this->wRoomRows;
//	p_IsRequired(row) = this->bIsRequired;
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
	p_ExtraVars(row) = ExtraBytes;

	delete pSquaresBytes;
	delete pLightsBytes;
	delete[] pbytExtraBytes;

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

	//Prepare props.
	this->dwRoomID = GetIncrementedID(p_RoomID);

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

	SetExtraVarsFromMembers();
	UINT dwExtraSize;
	BYTE *pbytExtraBytes = this->ExtraVars.GetPackedBuffer(dwExtraSize);
	ASSERT(pbytExtraBytes);
	c4_Bytes ExtraBytes(pbytExtraBytes, dwExtraSize);

	//Write Rooms record.
	c4_RowRef row = g_pTheDB->Rooms.GetNewRow();
	p_RoomID(row) = this->dwRoomID;
	p_LevelID(row) = this->dwLevelID;
	p_RoomX(row) = this->dwRoomX;
	p_RoomY(row) = this->dwRoomY;
	p_RoomCols(row) = this->wRoomCols;
	p_RoomRows(row) = this->wRoomRows;
//	p_IsRequired(row) = this->bIsRequired;
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
	p_ExtraVars(row) = ExtraBytes;

	delete pSquaresBytes;
	delete pLightsBytes;
	delete[] pbytExtraBytes;

	CDb::addRoomToLevel(this->dwRoomID, this->dwLevelID);
	return true;
}

//*****************************************************************************
CDbRoom* CDbRoom::MakeCopy(CImportInfo& info, const UINT newHoldID) const
//Replicates room data for new record entry in DB.
{
	CDbRoom *pCopy = g_pTheDB->Rooms.GetNew();
	if (!pCopy) return NULL;
	pCopy->SetMembers(*this, false);

	//Copy custom room media, if needed.
	if (newHoldID) {
		CDbData::CopyObject(info, pCopy->dwDataID, newHoldID);
	}

	return pCopy;
}

//*****************************************************************************
bool CDbRoom::SetMembers(
//For copy constructor and assignment operator.
//
//Params:
	const CDbRoom &Src,        //(in)
	const bool bCopyLocalInfo, //(in) default = true
	const bool bCopyGame)      //(in) default = true
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
//	this->bIsRequired = Src.bIsRequired;
	this->bIsSecret = Src.bIsSecret;
	this->dwDataID = Src.dwDataID;
	this->wImageStartX = Src.wImageStartX;
	this->wImageStartY = Src.wImageStartY;
	this->dwOverheadDataID = Src.dwOverheadDataID;
	this->wOverheadImageStartX = Src.wOverheadImageStartX;
	this->wOverheadImageStartY = Src.wOverheadImageStartY;
//	this->wMonsterCount = Src.wMonsterCount;
//	this->wBrainCount = Src.wBrainCount;
	this->wTarLeft = Src.wTarLeft;
	this->wTrapDoorsLeft = Src.wTrapDoorsLeft;
	this->bBetterVision = Src.bBetterVision;
//	this->bPersistentCitizenMovement = Src.bPersistentCitizenMovement;
	this->PlotsMade = Src.PlotsMade;

	//Room squares
	const UINT dwSquareCount = CalcRoomArea();
	this->pszOSquares = new char[dwSquareCount];
	if (!this->pszOSquares) throw CException("CDbRoom::SetMembers alloc failed");
	memcpy(this->pszOSquares, Src.pszOSquares, dwSquareCount * sizeof(char));
	this->pszFSquares = new char[dwSquareCount];
	if (!this->pszFSquares) throw CException("CDbRoom::SetMembers alloc failed");
	memcpy(this->pszFSquares, Src.pszFSquares, dwSquareCount * sizeof(char));
	this->pszTSquares = new char[dwSquareCount];
	if (!this->pszTSquares) throw CException("CDbRoom::SetMembers alloc failed");
	memcpy(this->pszTSquares, Src.pszTSquares, dwSquareCount * sizeof(char));
	this->pszTParams = new UINT[dwSquareCount];
	if (!this->pszTParams) throw CException("CDbRoom::SetMembers alloc failed");
	memcpy(this->pszTParams, Src.pszTParams, dwSquareCount * sizeof(UINT));
	this->tileLights = Src.tileLights;

	this->coveredTSquares = Src.coveredTSquares;

	this->overheadTiles = Src.overheadTiles;

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
/*
	this->checkpoints = Src.checkpoints;
*/

	this->ExtraVars = Src.ExtraVars;
	this->pressurePlateIndex = Src.pressurePlateIndex;
	this->weather = Src.weather;

	if (bCopyGame) {
		this->pCurrentGame = Src.pCurrentGame;
	}

	//Monster data
	this->pFirstMonster = this->pLastMonster = NULL;
	this->pMonsterSquares = new CMonster*[dwSquareCount];
	if (!this->pMonsterSquares) throw CException("CDbRoom::SetMembers alloc failed");
	memset(this->pMonsterSquares, 0, dwSquareCount * sizeof(CMonster*));
	CMonster *pMonster, *pTrav;
	for (pTrav = Src.pFirstMonster; pTrav != NULL; pTrav = pTrav->pNext)
	{
		pMonster = bCopyLocalInfo ? pTrav->Clone() : pTrav->Replicate();

		//Add monsters in the same order they appear in the source list.
		const UINT wProcessSequence = pMonster->wProcessSequence;
		pMonster->wProcessSequence = UINT(-1); //place at end

		//When not in play, always add monsters to the monster layer.
		//When making a room copy during play, only place monsters in the monster
		//layer when they are visible.
		LinkMonster(pMonster, !this->pCurrentGame || pMonster->IsVisible());
		pMonster->wProcessSequence = wProcessSequence; //restore value

		//Copy monster pieces.
		pMonster->Pieces.clear();  //make new copies of pieces
		for (list<CMonsterPiece*>::iterator piece = pTrav->Pieces.begin();
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

/*
	this->halphEnters = Src.halphEnters;
	this->slayerEnters = Src.slayerEnters;
*/
	if (bCopyLocalInfo)
	{
		this->deletedScrollIDs = Src.deletedScrollIDs;
		this->deletedSpeechIDs = Src.deletedSpeechIDs;
		this->deletedDataIDs = Src.deletedDataIDs;

		//In-game state information.
		this->PlotsMade = Src.PlotsMade;
		//	this->geometryChanges = Src.geometryChanges; //temporary front-end only info not needed
		//	this->disabledLights = Src.disabledLights;
/*		if (Src.pLastClone)
		{
			this->pLastClone = this->pMonsterSquares[ARRAYINDEX(Src.pLastClone->wX,Src.pLastClone->wY)];
			ASSERT(this->pLastClone);
		}

		for (wIndex=NumMovementTypes; wIndex--; )
			if (Src.pPathMap[wIndex])
				this->pPathMap[wIndex] = new CPathMap(*(Src.pPathMap[wIndex]));
*/
		this->LitFuses = Src.LitFuses;
		this->NewFuses = Src.NewFuses;
		this->NewBabies = Src.NewBabies;
		this->briars.setMembersForRoom(Src.briars, this);
		for (wIndex=0; wIndex<Src.platforms.size(); ++wIndex)
		{
			CPlatform *pPlatform = new CPlatform(*(Src.platforms[wIndex]));
			this->platforms.push_back(pPlatform);
		}
/*
		for (wIndex=0; wIndex<Src.stations.size(); ++wIndex)
		{
			CStation *pStation = new CStation(*(Src.stations[wIndex]), this);
			this->stations.push_back(pStation);
		}
*/
		this->mapMarker = Src.mapMarker;

		this->coveredOSquares = Src.coveredOSquares;
		this->pressurePlateIndex = Src.pressurePlateIndex;
		this->bTarWasStabbed = Src.bTarWasStabbed;
//		this->bGreenDoorsOpened = Src.bGreenDoorsOpened;
		this->weather = Src.weather;
		this->bridges.setMembersForRoom(Src.bridges, this);
//		this->building.setMembers(Src.building);
//		this->bCheckForHoldMastery = Src.bCheckForHoldMastery;
		this->mistVents = Src.mistVents;
		this->activeFiretraps = Src.activeFiretraps;

		for (list<CMonster*>::const_iterator m = Src.DeadMonsters.begin();
				m != Src.DeadMonsters.end(); ++m)
		{
			pTrav = *m;
			pMonster = bCopyLocalInfo ? pTrav->Clone() : pTrav->Replicate();
			//Copy monster pieces.
			pMonster->Pieces.clear();  //make new copies of pieces
			for (list<CMonsterPiece*>::iterator piece = pTrav->Pieces.begin();
					piece != pTrav->Pieces.end(); ++piece)
			{
				CMonsterPiece *pOldPiece = *piece;
				CMonsterPiece *pNewPiece = new CMonsterPiece(pMonster, pOldPiece->wTileNo, pOldPiece->wX, pOldPiece->wY);
				pMonster->Pieces.push_back(pNewPiece);
			}
			this->DeadMonsters.push_back(pMonster);
		}
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
void CDbRoom::SetMembersFromExploredRoomData(ExploredRoom *pExpRoom)
{
	if (!pExpRoom)
		return; //room hasn't been visited before, so nothing needs updating

	this->mapMarker = pExpRoom->mapMarker;

	//If the following happens, it means that a room marked on the map has been entered,
	//but not yet exited.
	if (!pExpRoom->SquaresBytes.Size())
		return;

	//Load room state.
	UnpackSquares(pExpRoom->SquaresBytes.Contents(), pExpRoom->SquaresBytes.Size());

	if (pExpRoom->tileLightsBytes.Size())
		UnpackTileLights(pExpRoom->tileLightsBytes.Contents(), pExpRoom->tileLightsBytes.Size());

	InitRoomStats(true); //don't alter platform connected components

	//Load special room info.
	for (CIDSet::const_iterator fuse = pExpRoom->litFuses.begin();
			fuse != pExpRoom->litFuses.end(); ++fuse)
		this->LitFuses.insert((*fuse) % this->wRoomCols, (*fuse) / this->wRoomCols);

	UINT count;
	const UINT minSize = min(pExpRoom->platformDeltas.GetSize(), (UINT)this->platforms.size());
	for (count=0; count<minSize; ++count)
	{
		UINT wX, wY;
		pExpRoom->platformDeltas.GetAt(count, wX, wY);
		CPlatform& platform = *(this->platforms[count]);
		platform.Move(*this, int(wX), int(wY), false);
		platform.xOffset = int(wX);
		platform.yOffset = int(wY);
	}

/*
	for (count=0; count<pExpRoom->orbTypes.size(); ++count)
		this->orbs[count]->eType = OrbType(pExpRoom->orbTypes[count]);
*/

	SetMonstersFromExploredRoomData(pExpRoom, true);
}

//*****************************************************************************
void CDbRoom::SetMonstersFromExploredRoomData(
//Adds monsters objects to the room from data saved in the explored room record.
//
//Params:
	ExploredRoom* pExpRoom,
	const bool bLoadNPCScripts) //whether NPC scripts should be loaded from the
	                            //original room data
{
	//Add NPC script info to that of saved NPC state info.
	vector<CDbPackedVars> extraVars;
	CMonster *pMonster;
	if (bLoadNPCScripts)
	{
		for (pMonster = pExpRoom->pMonsterList; pMonster != NULL; pMonster = pMonster->pNext)
		{
			if (pMonster->wType == M_CHARACTER)
			{
				//Find original NPC containing the script for this saved character.
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				CCharacter *pOrigNPC = GetCharacterWithScriptID(pCharacter->dwScriptID);
				if (pOrigNPC)
				{
					//Add the script data to the NPC's state, saved in the explored room data.
					CDbPackedVars vars = pCharacter->ExtraVars; //don't alter the explored room data (needed?)
					CCharacter::SaveCommands(vars, pOrigNPC->commands);
					extraVars.push_back(vars);
				} else {
					//No script found for this NPC in the base room data.
					//It could be a generated entity with a default script or the saved
					//game could be out of synch with the current hold version.
					extraVars.push_back(pCharacter->ExtraVars);
				}
			}
		}
	}

	//Remove room's original monster setup and replace it with monsters in the
	//saved room data.
	ClearMonsters();
	UINT characterIndex=0;
	for (pMonster = pExpRoom->pMonsterList; pMonster != NULL; pMonster = pMonster->pNext)
	{
		//NPC setup.
		bool bInRoom = true;
		const bool bCharacter = pMonster->wType == M_CHARACTER;
		CCharacter* pCharacter = NULL;
		if (bCharacter)
		{
			pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			//Invisible characters are not in room.
			if (!pCharacter->IsVisible())
				bInRoom = false;
		}

		CMonster *pNew = AddNewMonster(pMonster->wType, pMonster->wX, pMonster->wY, bInRoom);
		pNew->wO = pMonster->wO;
		pNew->ATK = pMonster->ATK;
		pNew->DEF = pMonster->DEF;
		pNew->GOLD = pMonster->GOLD;
		pNew->HP = pMonster->HP;
		pNew->XP = pMonster->XP;
		pNew->bEggSpawn = pMonster->bEggSpawn;
		if (bCharacter) {
			if (bLoadNPCScripts) {
				pNew->SetMembers(extraVars[characterIndex++]); //get from data prepared above
			} else {
				pNew->SetMembers(pCharacter->ExtraVars); //don't need script data, just pre-existing stats
			}
		}

		ASSERT(pNew->IsLongMonster() || pMonster->Pieces.empty());
		for (list<CMonsterPiece*>::const_iterator p = pMonster->Pieces.begin();
				p != pMonster->Pieces.end(); ++p)
		{
			CMonsterPiece& oldPiece = *(*p);
			CMonsterPiece *pMPiece = new CMonsterPiece(pNew, oldPiece.wTileNo, oldPiece.wX, oldPiece.wY);
			ASSERT(!this->pMonsterSquares[ARRAYINDEX(oldPiece.wX,oldPiece.wY)]);
			this->pMonsterSquares[ARRAYINDEX(oldPiece.wX,oldPiece.wY)] = pMPiece;
			pNew->Pieces.push_back(pMPiece);
		}

		if (bIsSerpent(pNew->wType))
		{
			CSerpent *pSerpent = DYN_CAST(CSerpent*, CMonster*, pMonster);
			CSerpent *pNewSerpent = DYN_CAST(CSerpent*, CMonster*, pNew);
			//Link serpent pieces to the main monster object.
			ASSERT(pMonster->Pieces.size());
			UINT wTempX, wTempY;
			pNewSerpent->GetTail(wTempX, wTempY);
			//Update base hp
			pNewSerpent->BaseHP = pSerpent->BaseHP;
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
//		floorMask.set(T_CHECKPOINT);  //keep for 1.6 import
		floorMask.set(T_WALL_B);
		floorMask.set(T_WALL_H);
		floorMask.set(T_DOOR_MONEY);
		floorMask.set(T_DOOR_C);
		floorMask.set(T_DOOR_G);
		floorMask.set(T_DOOR_R);
		floorMask.set(T_DOOR_Y);
		floorMask.set(T_DOOR_B);
		floorMask.set(T_DOOR_YO);
		floorMask.set(T_DOOR_GO);
		floorMask.set(T_DOOR_CO);
		floorMask.set(T_DOOR_RO);
		floorMask.set(T_DOOR_BO);
		floorMask.set(T_DOOR_MONEYO);
		floorMask.set(T_TUNNEL_N);
		floorMask.set(T_TUNNEL_S);
		floorMask.set(T_TUNNEL_E);
		floorMask.set(T_TUNNEL_W);
		floorMask.set(T_GOO);
		floorMask.set(T_FLOOR_IMAGE);
		floorMask.set(T_PRESSPLATE);
		floorMask.set(T_MISTVENT);
		floorMask.set(T_FIRETRAP);
		floorMask.set(T_FIRETRAP_ON);
		floorMask.set(T_DIRT1);
		floorMask.set(T_DIRT3);
		floorMask.set(T_DIRT5);

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
				case T_WATER:
				case T_MISTVENT:
				case T_FIRETRAP: case T_FIRETRAP_ON:
				case T_DIRT1: case T_DIRT3: case T_DIRT5:
					this->coveredOSquares.Add(wX,wY,wOSquare);
					++wNumTiles[wOSquare];
				break;
				case T_TRAPDOOR2:
				case T_PLATFORM_W:
				case T_THINICE:
					//Water trapdoors and rafts are over water.
					this->coveredOSquares.Add(wX,wY,T_WATER);
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
void CDbRoom::InitRoomStats(const bool bSkipPlatformInit) //[false]
//Initialize variables whose values are implicitly determined by the contents 
//of squares in the room.
{
	InitCoveredTiles();

	this->wTrapDoorsLeft = 0;
	this->wTarLeft = 0;
	this->bBetterVision = false;
//	this->bPersistentCitizenMovement = false;
//	this->bCheckForHoldMastery = false;
	this->briars.clear();
	this->briars.setRoom(this); //call after Clear
	this->bridges.setRoom(this);
//	this->building.init(this->wRoomCols, this->wRoomRows);
	this->mistVents.clear();
	this->activeFiretraps.clear();
	if (!bSkipPlatformInit)
		ClearPlatforms();

	char *pszSeek, *pszStop;
	pszStop = this->pszOSquares + CalcRoomArea() * sizeof(char);
	CCoordIndex plots(this->wRoomCols, this->wRoomRows);
	CCoordIndex obstacles(this->wRoomCols, this->wRoomRows);
	for (pszSeek = this->pszOSquares; pszSeek != pszStop; ++pszSeek)
		switch (*pszSeek)
		{
			//Number of trapdoors left.
			case T_TRAPDOOR: case T_TRAPDOOR2:
				++this->wTrapDoorsLeft;
			break;

/*			case T_WALL_M:
				this->bCheckForHoldMastery = true;
			break;
*/
			//Obstacles are moved from the o-layer in version 1.6 to the t-layer in 2.0
			case T_OBSTACLE:
			{
				const UINT wX = (pszSeek - this->pszOSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszOSquares) / this->wRoomCols;
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
				if (!bSkipPlatformInit)
				{
					const UINT wX = (pszSeek - this->pszOSquares) % this->wRoomCols;
					const UINT wY = (pszSeek - this->pszOSquares) / this->wRoomCols;
					if (!plots.Exists(wX,wY))
						AddPlatformPiece(wX, wY, plots);
				}
			break;
			//Bridges.
			case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
			{
				const UINT wX = (pszSeek - this->pszOSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszOSquares) / this->wRoomCols;
				this->bridges.HandleBridgeAdded(wX, wY);
			}
			break;
			//Vents.
			case T_MISTVENT:
			{
				const UINT wX = (pszSeek - this->pszOSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszOSquares) / this->wRoomCols;
				this->mistVents.insert(wX, wY);
			}
			break;
			case T_FIRETRAP_ON:
			{
				const UINT wX = (pszSeek - this->pszOSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszOSquares) / this->wRoomCols;
				this->activeFiretraps.insert(wX, wY);
			}
			break;
		}

	pszStop = this->pszTSquares + CalcRoomArea() * sizeof(char);
	for (pszSeek = this->pszTSquares; pszSeek != pszStop; ++pszSeek)
		switch (*pszSeek)
		{
			//Number of tar left.
			case T_TAR: case T_MUD: case T_GEL:
				++this->wTarLeft;
			break;
			//Flow sources.
			case T_BRIAR_SOURCE:
			{
				const UINT wX = (pszSeek - this->pszTSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszTSquares) / this->wRoomCols;
				this->briars.insert(wX,wY);
			}
			break;
/*
			case T_STATION:
			{
				const UINT wX = (pszSeek - this->pszTSquares) % this->wRoomCols;
				const UINT wY = (pszSeek - this->pszTSquares) / this->wRoomCols;
				this->stations.push_back(new CStation(wX, wY, this));
			}
			break;
*/
			case T_TOKEN:
			{
				const UINT tParam = this->pszTParams[pszSeek - this->pszTSquares];
				if (bTokenActive(tParam)) {
					const UINT tokenType = calcTokenType(tParam);
					switch (tokenType)
					{
						case TarTranslucent:
							this->bBetterVision = true;
							break;
						case SwordDisarm: //handled in SetMembersAfterRoomLoad
						default: break;
					}
				}
			}
			break;
			case T_SWORD:
				//Backwards-compatibility: Shields originally were indexed immediately after swords in the same enumeration.
				//This upgrades shield data to make it a separate tile and its own enumeration.
				if (this->pszTParams[pszSeek - this->pszTSquares] >= SwordCount &&
						!bIsCustomEquipment(this->pszTParams[pszSeek - this->pszTSquares]))
				{
					*pszSeek = T_SHIELD;
					this->pszTParams[pszSeek - this->pszTSquares] -= ShieldSwordOffset;
				}
			break;

			case T_SHIELD:
				//Backwards-compatibility: Accessories originally were indexed immediately after shields in the same enumeration.
				//This upgrades accessory data to make it a separate tile and its own enumeration.
				if (this->pszTParams[pszSeek - this->pszTSquares] >= ShieldCount &&
						!bIsCustomEquipment(this->pszTParams[pszSeek - this->pszTSquares]))
				{
					*pszSeek = T_ACCESSORY;
					this->pszTParams[pszSeek - this->pszTSquares] -= AccessoryShieldOffset;
				}
			break;

			default: break;
		}
	ObstacleFill(obstacles);
}

//*****************************************************************************
void CDbRoom::InitStateForThisTurn()
{
	ClearStateVarsUsedDuringTurn();

	char* pszSeek, * pszStop;
	pszStop = this->pszTSquares + CalcRoomArea() * sizeof(char);
	for (pszSeek = this->pszTSquares; pszSeek != pszStop; ++pszSeek) {
		if (*pszSeek == T_POWDER_KEG) {
			const UINT wX = (pszSeek - this->pszTSquares) % this->wRoomCols;
			const UINT wY = (pszSeek - this->pszTSquares) / this->wRoomCols;
			this->stationary_powder_kegs.insert(wX, wY);
		}
	}
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

	GetConnectedTiles(wX, wY, mask, b8Neighbor, squares);
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
	const CTileMask &tileMask,          //(in) which type of tiles to gather
	const bool b8Neighbor,        //(in) perform 8-neighbor (diagonal) search if true, 4-neighbor if false
	CCoordSet& squares,         //(out) set of contiguous tiles
	const CCoordSet* pIgnoreSquares,	//(in) optional set of squares to ignore [default=NULL]
	const CCoordSet* pRegionMask)    //(in) optional region to limit the search to [default=NULL]
const
{
	squares.clear();

	if (!IsValidColRow(wX,wY))
		return;

	if (pIgnoreSquares && pIgnoreSquares->has(wX, wY))
		return;
	if (pRegionMask && !pRegionMask->has(wX, wY))
		return;

	if (!tileMask.get(GetOSquare(wX, wY)) && !tileMask.get(GetTSquare(wX, wY)))
		return;

	CCoordStack evalCoords(wX,wY);

	UINT wEvalX, wEvalY;
	while (evalCoords.PopBottom(wEvalX,wEvalY)) //perform as a queue for performance
	{
		// If the tile is of this type, add it to the squares set and
		// push surrounding tiles onto the evaluation stack.
		if (!squares.insert(wEvalX, wEvalY))
			continue; //don't need to reprocess this tile if it was already handled

#define PushTileIfOfType(wX, wY) {\
	ASSERT(IsValidColRow((wX), (wY)));\
	if ( (tileMask.get(GetOSquare((wX), (wY))) || tileMask.get(GetTSquare((wX), (wY))))\
			&& !squares.has((wX), (wY)) && (!pIgnoreSquares || !pIgnoreSquares->has((wX), (wY)))\
			&& (!pRegionMask || pRegionMask->has((wX), (wY))) )\
		evalCoords.Push((wX), (wY));}

		//Add adjacent coords to Eval stack.
		const bool bGTTop = wEvalY > 0;
		const bool bLTBottom = wEvalY < this->wRoomRows - 1;
		if (wEvalX > 0)
		{
			PushTileIfOfType(wEvalX - 1, wEvalY);
			if (b8Neighbor)
			{
	         if (bGTTop) PushTileIfOfType(wEvalX - 1, wEvalY - 1);
		      if (bLTBottom) PushTileIfOfType(wEvalX - 1, wEvalY + 1);
			}
		}
		if (bGTTop) PushTileIfOfType(wEvalX, wEvalY - 1);
		if (wEvalX < this->wRoomCols - 1)
		{
			PushTileIfOfType(wEvalX + 1, wEvalY);
			if (b8Neighbor)
			{
	         if (bGTTop) PushTileIfOfType(wEvalX + 1, wEvalY - 1);
		      if (bLTBottom) PushTileIfOfType(wEvalX + 1, wEvalY + 1);
			}
		}
		if (bLTBottom) PushTileIfOfType(wEvalX, wEvalY + 1);
#undef PushTileIfOfType
	}
}

//*****************************************************************************
void CDbRoom::GetConnectedMistTiles(
//Compiles set of all orthogonally connected mist squares, starting from (wX, wY)
//Unlike other connection functions, this allows connections via covered T-layer
	const UINT wX, const UINT wY, //(in) square to check from
	CCoordSet& mistSquares)       //(out) set of contiguous tiles
const
{
	mistSquares.clear();
	CCoordSet toCheck;

	if (!IsValidColRow(wX, wY))
		return;

	CCoordStack evalCoords(wX, wY);

#define PushTileIfMist(x, y) {\
	if (IsValidColRow(x, y) && IsEitherTSquare(x, y, T_MIST) && !mistSquares.has(x, y))\
		evalCoords.Push((x), (y));}

	UINT wEvalX, wEvalY;
	while (evalCoords.PopBottom(wEvalX, wEvalY)) //perform as a queue for performance
	{
		if (!mistSquares.insert(wEvalX, wEvalY))
			continue; //don't need to reprocess this tile if it was already handled

		PushTileIfMist(wEvalX + 1, wEvalY);
		PushTileIfMist(wEvalX - 1, wEvalY);
		PushTileIfMist(wEvalX, wEvalY + 1);
		PushTileIfMist(wEvalX, wEvalY - 1);
	}

#undef PushTileIfMist
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
		GetConnectedTiles(wX, wY-1, tileMask, false, currRegion, &ignore, pRegionMask);
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
		GetConnectedTiles(wX+1, wY, tileMask, false, currRegion, &ignore, pRegionMask);
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
		GetConnectedTiles(wX, wY+1, tileMask, false, currRegion, &ignore, pRegionMask);
		if (currRegion.has(wX-1,wY))
			haveW = true;
		if (currRegion.empty())
			--numRegions;
	}

	if (!haveW)
	{
		regions.resize(++numRegions);
		CCoordSet& currRegion = regions.back();
		GetConnectedTiles(wX-1, wY, tileMask, false, currRegion, &ignore, pRegionMask);
		if (currRegion.empty())
			--numRegions;
	}

	regions.resize(numRegions);
}

//*****************************************************************************
void CDbRoom::GetAllDoorSquares(
//Compiles set of all squares for a door type, starting at (wX,wY).
//If there is no door there, returns an empty set.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	CCoordSet& squares,         //(out) set of contiguous door squares
	const UINT tile,            //door color type
	const CCoordSet* pIgnoreSquares)	 //(in) optional set to ignore [default=NULL]
const
{
	CTileMask doorMask;
	switch (tile)
	{
		case T_DOOR_Y: case T_DOOR_YO:
			doorMask.set(T_DOOR_Y);
			doorMask.set(T_DOOR_YO);
		break;
		case T_DOOR_G: case T_DOOR_GO:
			doorMask.set(T_DOOR_G);
			doorMask.set(T_DOOR_GO);
		break;
		case T_DOOR_C: case T_DOOR_CO:
			doorMask.set(T_DOOR_C);
			doorMask.set(T_DOOR_CO);
		break;
		case T_DOOR_R: case T_DOOR_RO:
			doorMask.set(T_DOOR_R);
			doorMask.set(T_DOOR_RO);
		break;
		case T_DOOR_B: case T_DOOR_BO:
			doorMask.set(T_DOOR_B);
			doorMask.set(T_DOOR_BO);
		break;
		case T_DOOR_MONEY: case T_DOOR_MONEYO:
			//Add only this tile, not connected pieces.
			if (pIgnoreSquares && pIgnoreSquares->has(wX, wY))
				break;
			squares.insert(wX, wY);
		return;
	}
	GetConnectedTiles(wX, wY, doorMask, false, squares, pIgnoreSquares);
}

//*****************************************************************************
void CDbRoom::MarkDataForDeletion(const CDbDatum* pDatum)
//Keep track of data IDs so that data object is deleted if Update is called.
{
	if (pDatum && pDatum->dwDataID)
		this->deletedDataIDs.push_back(pDatum->dwDataID);
}

//*****************************************************************************
void CDbRoom::MarkSpeechForDeletion(
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
void CDbRoom::ObstacleFill(CCoordIndex& obstacles)
//Fill marked obstacle tiles with the largest obstacles
//of their type that will fit.  Top-down, left-to right.
{
	if (!obstacles.GetSize()) return;   //no obstacles marked

	UINT wX, wY;
	for (wY=0; wY<this->wRoomRows; ++wY)
		for (wX=0; wX<this->wRoomCols; ++wX)
		{
			UINT obType = obstacles.GetAt(wX,wY);
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

				if (!obstacles.GetSize())
					return;   //quick return once everything's been handled
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
void CDbRoom::RemoveDoorTile(
//Updates orb data for removal of a door tile.
//May be called either before or after changing the actual tile.
//
//Params:
	const UINT wX, const UINT wY, //(in) square to check from
	const UINT wTile)  //(in) removed tile type (open or closed)
{
	vector<CCoordSet> regions;
	switch (wTile)
	{
		case T_DOOR_MONEY: case T_DOOR_MONEYO:
			//Don't consider connected components.
		break;
		default:
			GetConnectedRegionsAround(wX, wY, CTileMask(wTile), regions);
		break;
	}
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
