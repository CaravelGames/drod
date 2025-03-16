// $Id: DbSavedGames.cpp 10113 2012-04-22 05:40:36Z mrimer $

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
 * 1997, 2000, 2001, 2002 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * JP Burford (jpburford)
 *
 * ***** END LICENSE BLOCK ***** */

//DbSavedGames.cpp
//Implementation of CDbSavedGame and CDbSavedGames.

#include "DbSavedGames.h"

#include "Character.h"
#include "CurrentGame.h"
#include "Db.h"
#include "DbProps.h"
#include "DbXML.h"
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "NetInterface.h"
#include "Serpent.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>

const char szSavename[] = "Savename";

CMonster      *pImportERMonster = NULL;
ROOMCOORD     importPlatformDelta;
ExploredRoom  *pImportExploredRoom = NULL;
CMonsterPiece *pImportERPiece = NULL;


//*******************************************************************************
void deleteMonsterList(CMonster *pMonsterList)
//Deletes a linked list of CMonster objects.
{
	while (pMonsterList)
	{
		CMonster *pDelete = pMonsterList;
		pMonsterList = pMonsterList->pNext;
		delete pDelete;
	}
}

//*******************************************************************************
CMonster* copyMonsterList(CMonster *pThatMonsterList)
//Makes a deep copy of a monster list.
//
//Returns: the copy of the monster list
{
	CMonster *pMonsterList = NULL;
	CMonster *pLastMonster = NULL;
	for (CMonster *pMonster = pThatMonsterList; pMonster != NULL; pMonster = pMonster->pNext)
	{
		CMonster *pNew = pMonster->Clone();

		//Copy monster pieces.
		pNew->Pieces.clear();  //make new copies of pieces
		for (list<CMonsterPiece*>::iterator piece = pMonster->Pieces.begin();
				piece != pMonster->Pieces.end(); ++piece)
		{
			CMonsterPiece& oldPiece = *(*piece);
			CMonsterPiece *pNewPiece = new CMonsterPiece(pNew,
					oldPiece.wTileNo, oldPiece.wX, oldPiece.wY);
			pNew->Pieces.push_back(pNewPiece);
		}

		//Link up.
		pNew->pNext = NULL;
		pNew->pPrevious = pLastMonster;
		if (!pMonsterList)
			pMonsterList = pNew;
		else
		{
			ASSERT(pLastMonster);
			pLastMonster->pNext = pNew;
		}
		pLastMonster = pNew;
	}

	return pMonsterList;
}

//*******************************************************************************
ExploredRoom::ExploredRoom()
	: roomID(0)
	, mapState(MapState::Invisible)
	, bSave(true)
	, mapMarker(0)
	, mapIcon(ScriptVars::MapIcon::MI_None)
	, mapIconState(ScriptVars::MapIconState::MIS_Normal)
	, pMonsterList(NULL)
{
}

//*******************************************************************************
ExploredRoom::ExploredRoom(const ExploredRoom& that)
//Copy constructor.
	: SquaresBytes()
	, tileLightsBytes()
{
	this->roomID = that.roomID;
	this->mapState = that.mapState;
	this->bSave = that.bSave;
	this->mapMarker = that.mapMarker;
	this->mapIcon = that.mapIcon;
	this->mapIconState = that.mapIconState;
	this->litFuses = that.litFuses;
	this->platformDeltas = that.platformDeltas;
//	this->orbTypes = that.orbTypes;
	if (that.SquaresBytes.Size())
		this->SquaresBytes = c4_Bytes(that.SquaresBytes.Contents(), that.SquaresBytes.Size(), true);
	if (that.tileLightsBytes.Size())
		this->tileLightsBytes = c4_Bytes(that.tileLightsBytes.Contents(), that.tileLightsBytes.Size(), true);

	saveMonsterList(that.pMonsterList);
}

//*******************************************************************************
ExploredRoom::~ExploredRoom()
{
	deleteMonsters();
}

//*******************************************************************************
void ExploredRoom::deleteMonsters()
{
	deleteMonsterList(this->pMonsterList);
}

//*******************************************************************************
void ExploredRoom::saveMonsterList(CMonster *pThatMonsterList)
{
	this->pMonsterList = copyMonsterList(pThatMonsterList);
}

//
//CDbSavedGame public methods.
//

//*******************************************************************************
CDbSavedGame::CDbSavedGame(bool bClear) //[default=true]
	: CDbBase()
	, pMonsterList(NULL), pMonsterListAtRoomStart(NULL)
{
	if (bClear)
		Clear();
}

//*******************************************************************************
CDbSavedGame::~CDbSavedGame()
//Destructor.
{
	Clear();
}

//*******************************************************************************
void CDbSavedGame::appendToGlobalMonsterList(CMonster *pMonster)
//Adds the specified monster to the monster list.
{
	CMonster *pSeek = this->pMonsterList;
	if (pSeek)
	{
		//Append to list.
		while (pSeek->pNext)
			pSeek = pSeek->pNext;
		pMonster->pNext = NULL;
		pMonster->pPrevious = pSeek;
		pSeek->pNext = pMonster;
	} else {
		//First monster in list.
		pMonster->pPrevious = pMonster->pNext = NULL;
		this->pMonsterList = pMonster;
	}
}

//*******************************************************************************
CCharacter* CDbSavedGame::getCustomEquipment(const UINT type) const
//Returns: pointer to the character defining the specified piece of inventory
{
	for (CMonster *pMonster = this->pMonsterList; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->equipType == (ScriptFlag::EquipmentType)type)
				return pCharacter;
		}
	}
	return NULL;
}

//*******************************************************************************
ExploredRoom* CDbSavedGame::getExploredRoom(const UINT roomID) const
//Returns: pointer to explored room record (even if only marked on map)
{
	for (vector<ExploredRoom*>::const_iterator room = this->ExploredRooms.begin();
			room != this->ExploredRooms.end(); ++room)
	{
		if ((*room)->roomID == roomID)
			return *room;
	}
	return NULL;
}

//*******************************************************************************
//Returns: roomIDs that have been explored by player
//           with optional filters of rooms that are not saved or only marked on the map
CIDSet CDbSavedGame::GetExploredRooms(
	const bool bMapOnlyAlso, //[default=false] include rooms just marked on map
	const bool bIncludeNoSavedAlso) //[default=true] include rooms marked to not save (aka previews)
const
{
	CIDSet rooms;
	for (vector<ExploredRoom*>::const_iterator roomIt = this->ExploredRooms.begin();
			roomIt != this->ExploredRooms.end(); ++roomIt)
	{
		const ExploredRoom& room = *(*roomIt);
		if (room.IsInvisible()) {
			continue;
		}

		const bool bMapOnlyFilter = bMapOnlyAlso || room.HasDetail();
		const bool bNoSaveFilter = bIncludeNoSavedAlso || room.bSave;
		if (bMapOnlyFilter && bNoSaveFilter)
			rooms += room.roomID;
	}
	return rooms;
}

//*****************************************************************************
CIDSet CDbSavedGame::GetInvisibleRooms() const
//Returns: All "explored rooms" that should not be shown on the map
{
	CIDSet rooms;
	for (vector<ExploredRoom*>::const_iterator roomIt = this->ExploredRooms.begin();
		roomIt != this->ExploredRooms.end(); ++roomIt)
	{
		const ExploredRoom& room = *(*roomIt);
		if (room.IsInvisible()) {
			rooms += room.roomID;
		}
	}
	return rooms;
}

//*****************************************************************************
CDbRoom* CDbSavedGame::GetRoom()
//Gets room associated with a saved game.
//
//Returns:
//Pointer to loaded room which caller must delete, or NULL if no matching room
//was found.
const
{
	CDbRoom *pRoom = new CDbRoom();
	if (pRoom)
	{
		if (!pRoom->Load(this->dwRoomID))
		{
			delete pRoom;
			pRoom=NULL;
		}
	}
	return pRoom;
}

//*******************************************************************************
bool CDbSavedGame::IsRoomExplored(const UINT roomID, const bool bConsiderCurrentRoom) const
//Returns: whether room has been explored by player (not just marked on map)
{
	for (vector<ExploredRoom*>::const_iterator room = this->ExploredRooms.begin();
			room != this->ExploredRooms.end(); ++room)
	{
		if ((*room)->roomID == roomID)
			return ((*room)->HasDetail());
	}

	//room hasn't been explored before, but player is here now
	if (bConsiderCurrentRoom && roomID == this->dwRoomID)
		return true;

	return false;
}

//*******************************************************************************
bool CDbSavedGame::Load(
//Loads a saved game from database into this object.
//
//Params:
	const UINT dwLoadSavedGameID,   //(in) ID of saved game to load.
	const bool bQuick) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {
	
	Clear();

	//Find record with matching saved game ID.
	ASSERT(IsOpen());
	c4_View SavedGamesView;
	const UINT dwSavedGameI = LookupRowByPrimaryKey(dwLoadSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGameI == ROW_NO_MATCH) throw CException("CDbSavedGame::Load");
	c4_RowRef row = SavedGamesView[dwSavedGameI];

	//Load in props from SavedGames record.
	this->dwSavedGameID = (UINT) (p_SavedGameID(row));
	ASSERT(this->dwSavedGameID == dwLoadSavedGameID);
	this->dwPlayerID = (UINT) (p_PlayerID(row));
	this->dwRoomID = (UINT) (p_RoomID(row));
	this->eType = (SAVETYPE) (int) p_Type(row);
	this->bIsHidden = ( p_IsHidden(row)!=0 );

	if (!this->dwRoomID && this->eType != ST_DemoUpload && //Placeholder record.
			this->eType != ST_PlayerTotal)
		throw CException("CDbSavedGame::Load");

	this->bPartialLoad = bQuick;
	if (!bQuick)
	{
		c4_View ExploredRoomsView, CompletedScriptsView, EntrancesExploredView;
		c4_Bytes checksumStrBytes = (c4_Bytes)p_ChecksumStr(row);
		UINT dwScriptI, dwScriptCount, entranceI, entrancesExploredCount;

		this->wStartRoomX = (UINT) p_StartRoomX(row);
		this->wStartRoomY = (UINT) p_StartRoomY(row);
		this->wStartRoomO = (UINT) p_StartRoomO(row);
		this->wStartRoomAppearance = (UINT) p_StartRoomAppearance(row);
		this->bStartRoomSwordOff = p_StartRoomSwordOff(row) != 0;
		this->Created = (time_t) p_Created(row);
		this->LastUpdated = (time_t) p_LastUpdated(row);
		this->Commands = p_Commands(row);
		this->moves.Load(dwLoadSavedGameID);

		this->stats = p_Stats(row);
		this->wVersionNo = p_Version(row);

		LoadExploredRooms(p_ExploredRooms(row));

		//Populate completed scripts list.
		CompletedScriptsView = p_CompletedScripts(row);
		dwScriptCount = CompletedScriptsView.GetSize();
		for (dwScriptI = 0; dwScriptI < dwScriptCount; ++dwScriptI)
			this->CompletedScripts += (UINT) p_ScriptID(CompletedScriptsView[dwScriptI]);

		//Populate entrances explored list.
		EntrancesExploredView = p_EntrancesExplored(row);
		entrancesExploredCount = EntrancesExploredView.GetSize();
		for (entranceI = 0; entranceI < entrancesExploredCount; ++entranceI)
			this->entrancesExplored += (UINT) p_EntranceID(EntrancesExploredView[entranceI]);

		//Load monsters for this room
		c4_View MonstersView = p_Monsters(row);
		this->pMonsterListAtRoomStart = LoadMonsters(MonstersView);
		this->pMonsterList = copyMonsterList(this->pMonsterListAtRoomStart);

		this->checksumStr.assign(
				(const char*)checksumStrBytes.Contents(), checksumStrBytes.Size());
		if (!g_pTheNet->VerifyChecksum(this))
			this->checksumStr.resize(0);
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
void CDbSavedGame::LoadExploredRooms(const c4_View& ExploredRoomsView)
//Retrieve set of explored room records from the view.
{
	const UINT wCount = ExploredRoomsView.GetSize();
	for (UINT wI=0; wI < wCount; ++wI)
	{
		c4_RowRef row = ExploredRoomsView[wI];

		ExploredRoom *pRoom = new ExploredRoom();
		pRoom->roomID = p_RoomID(row);
		pRoom->mapState = (MapState)(int)(p_MapState(row));
		pRoom->mapMarker = p_MapMarker(row);
		pRoom->mapIcon = (ScriptVars::MapIcon)(int)p_MapIcon(row);
		pRoom->mapIconState = (ScriptVars::MapIconState)(int)p_MapIconState(row);
		c4_Bytes SquaresBytes = p_Squares(row);
		pRoom->SquaresBytes = c4_Bytes(SquaresBytes.Contents(), SquaresBytes.Size(), true);
		pRoom->pMonsterList = NULL;
		c4_View MonstersView = p_Monsters(row);
		c4_View LitFusesView = p_LitFuses(row);
		c4_View PlatformDeltasView = p_PlatformDeltas(row);
		c4_View OrbTypesView = p_OrbTypes(row);
		c4_Bytes TileLightsBytes = p_TileLights(row);
		if (TileLightsBytes.Size())
			pRoom->tileLightsBytes = c4_Bytes(TileLightsBytes.Contents(), TileLightsBytes.Size(), true);
		else
			pRoom->tileLightsBytes = c4_Bytes();

		UINT wCount = 0;

		for (wCount=0; wCount<UINT(LitFusesView.GetSize()); ++wCount)
			pRoom->litFuses += UINT(p_X(LitFusesView[wCount]));
		for (wCount=0; wCount<UINT(PlatformDeltasView.GetSize()); ++wCount)
		{
			c4_RowRef row = PlatformDeltasView[wCount];
			pRoom->platformDeltas.Push(p_X(row), p_Y(row));
		}
/*
		for (wCount=0; wCount<UINT(OrbTypesView.GetSize()); ++wCount)
			pRoom->orbTypes.push_back(UINT(p_X(OrbTypesView[wCount])));
*/

		pRoom->pMonsterList = LoadMonsters(MonstersView);

		this->ExploredRooms.push_back(pRoom);
	}
}

//*****************************************************************************
CMonster* CDbSavedGame::LoadMonster(const c4_RowRef& row)
//Returns: pointer to monster object
{
	const UINT wMonsterType = p_Type(row);
	const UINT wX = p_X(row), wY = p_Y(row);

	const MONSTERTYPE eMonsterType = (const MONSTERTYPE)wMonsterType;
	CMonsterFactory mf(NULL);
	CMonster *pNew = mf.GetNewMonster(eMonsterType);
	if (!pNew) throw CException("CDbSavedGame::LoadMonster: Alloc failed");

	//Set monster position.
	pNew->wX = pNew->wPrevX = wX;
	pNew->wY = pNew->wPrevY = wY;
	pNew->wO = pNew->wPrevO = p_O(row);

	pNew->ExtraVars = p_ExtraVars(row);
	pNew->SetMembers(pNew->ExtraVars);

	//Pieces.
	c4_View PiecesView = p_Pieces(row);
	const UINT wNumPieces = PiecesView.GetSize();
	ASSERT(pNew->IsLongMonster() || wNumPieces == 0);
	for (UINT wPieceI=0; wPieceI < wNumPieces; ++wPieceI)
	{
		c4_RowRef pieceRow = PiecesView[wPieceI];
		const UINT wX = p_X(pieceRow);
		const UINT wY = p_Y(pieceRow);
		CMonsterPiece *pMPiece = new CMonsterPiece(pNew, p_Type(pieceRow), wX, wY);
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

	return pNew;
}

//*****************************************************************************
CMonster* CDbSavedGame::LoadMonsters(
//Loads monsters from database into member vars of object.
//
//Params:
	c4_View &MonstersView)     //(in) Open view containing 0 or more monsters.
//
//Returns:
//Loaded monster list.
{
	CMonster *pMonsterList = NULL, *pLastMonster = NULL;
	const UINT wMonsterCount = MonstersView.GetSize();
	UINT wCount;
	for (wCount=0; wCount<wMonsterCount; ++wCount)
	{
		c4_RowRef row = MonstersView[wCount];
		CMonster *pNew = LoadMonster(row);
		if (!pNew)
			continue;

		//Add to monster list.
		if (!pMonsterList)
		{
			pMonsterList = pNew;
		} else {
			pLastMonster->pNext = pNew;
		}
		pNew->pNext = NULL;
		pLastMonster = pNew;
	}

	return pMonsterList;
}

//*****************************************************************************
void CDbSavedGame::ReloadMonsterList(CMonster *pAlternateMonsterList) //[default=NULL]
//Reloads the monster list from the monster list at room start.
{
	deleteMonsterList(this->pMonsterList);
	this->pMonsterList = copyMonsterList(pAlternateMonsterList ?
			pAlternateMonsterList : this->pMonsterListAtRoomStart);
}

//*****************************************************************************
void CDbSavedGame::removeGlobalScriptForEquipment(const UINT type)
//Searches for the first instance of a custom NPC representing this equipment type
//in the monster list.  If found, it is deleted.
{
	for (CMonster *pMonster = this->pMonsterList; pMonster; pMonster = pMonster->pNext)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if ((UINT)pCharacter->equipType == type)
			{
				//Remove monster.
				if (pMonster->pPrevious)
					pMonster->pPrevious->pNext = pMonster->pNext;
				if (pMonster->pNext)
					pMonster->pNext->pPrevious = pMonster->pPrevious;
				if (pMonster == this->pMonsterList)
					this->pMonsterList = pMonster->pNext;

				this->DeadMonsters.push_back(pMonster);
				break;
			}
		}
	}
}

//*****************************************************************************
void CDbSavedGame::removeGlobalScripts(const CIDSet& completedScripts)
//Removes any running global character scripts that have been marked as completed.
{
	vector<CMonster*> deletedMonsters;
	for (CIDSet::const_iterator scriptIter = completedScripts.begin();
			scriptIter != completedScripts.end(); ++scriptIter)
	{
		//Search for scriptID in global monsters.
		const UINT scriptID = *scriptIter;
		for (CMonster *pMonster = this->pMonsterList; pMonster; pMonster = pMonster->pNext)
		{
			if (pMonster->wType == M_CHARACTER)
			{
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
				if (pCharacter->dwScriptID == scriptID)
				{
					//Remove monster.
					deletedMonsters.push_back(pMonster);
					if (pMonster->pPrevious)
						pMonster->pPrevious->pNext = pMonster->pNext;
					if (pMonster->pNext)
						pMonster->pNext->pPrevious = pMonster->pPrevious;
					if (pMonster == this->pMonsterList)
						this->pMonsterList = pMonster->pNext;
					break;
				}
			}
		}
	}

	//Delete removed monster records.
	for (vector<CMonster*>::const_iterator monIter=deletedMonsters.begin();
			monIter!=deletedMonsters.end(); ++monIter)
		delete *monIter;
}

//*****************************************************************************
void CDbSavedGame::RemoveMappedRoomsNotIn(
//Remove any room objects in this->ExploredRooms that are not contained in mappedRoomIDs,
//which is a superset of exploredRoomIDs.
//Any rooms that were previously explored, but now are only mapped,
//are reverted to "bMapOnly" status.
//Explored rooms are also reverted to non-saved room previews where applicable.
	const CIDSet& exploredRoomIDs, const CIDSet& mappedRoomIDs,
	const CIDSet& roomPreviewIDs) //rooms to display on the map, but not to write to a saved game
{
	ASSERT(mappedRoomIDs.contains(exploredRoomIDs)); //superset

	vector<ExploredRoom*> retainedRooms;
	for (vector<ExploredRoom*>::const_iterator roomIter = this->ExploredRooms.begin();
			roomIter != this->ExploredRooms.end(); ++roomIter)
	{
		ExploredRoom* pRoom = *roomIter;
		const UINT roomID = pRoom->roomID;
		if (!mappedRoomIDs.has(roomID))
		{
			//This room was added to the map since entering the current room -- take it back off
			if (pRoom->bSave && !roomPreviewIDs.has(roomID)) {
				delete pRoom; //room shouldn't be included in any list
			} else {
				//Persist map display of non-saved rooms
				pRoom->bSave = false;
				retainedRooms.push_back(pRoom);
			}
			continue;
		}

		if (!exploredRoomIDs.has(roomID) && pRoom->HasDetail())
		{
			//Explored room is reverted to "mapped only" room.
			delete pRoom;
			pRoom = new ExploredRoom();
			pRoom->roomID = roomID;
			pRoom->mapState = MapState::NoDetail;
		}

		retainedRooms.push_back(pRoom);
	}
	this->ExploredRooms = retainedRooms;
}

//*****************************************************************************
void CDbSavedGame::ResetForImport()
//Resets static vars used in import process.
{
	delete pImportExploredRoom; pImportExploredRoom = NULL;
	delete pImportERMonster; pImportERMonster = NULL;
	delete pImportERPiece; pImportERPiece = NULL;
}

//*****************************************************************************
void CDbSavedGame::SetMonsterListAtRoomStart()
//Makes a copy of the current monster list.
//This should be called whenever the player enters a new room.
//The saved list will be reverted to whenever the player must restart the room.
{
	deleteMonsterList(this->pMonsterListAtRoomStart);
	this->pMonsterListAtRoomStart = copyMonsterList(this->pMonsterList);
}

//*****************************************************************************
void CDbSavedGame::setMonstersCurrentGame(CCurrentGame* pCurrentGame)
//Sets the current game for the monsters in the monster list.
{
	for (CMonster *pMonster = this->pMonsterList; pMonster != NULL; pMonster = pMonster->pNext)
		pMonster->SetCurrentGame(pCurrentGame);
}

//*****************************************************************************
MESSAGE_ID CDbSavedGame::SetProperty(
//Used during XML data import.
//According to pType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
	const PROPTYPE pType,   //(in) property (data member) to set
	char* const str,        //(in) string representation of value
	CImportInfo &info,      //(in/out) Import data
	bool &bSaveRecord)      //(out) whether record should be saved
{
	static PrimaryKeyMap::iterator localID;
	switch (pType)
	{
		//By the time this field is parsed, all other relevant fields should
		//have been parsed already.
		case P_SavedGameID:
		{
			this->dwSavedGameID = convertToUINT(str);
			if (!this->dwSavedGameID)
				return MID_FileCorrupted;  //corrupt data

			//Look up local ID.
			localID = info.SavedGameIDMap.find(this->dwSavedGameID);
			if (localID != info.SavedGameIDMap.end())
				//Error - this saved game should not have been imported yet
				return MID_FileCorrupted;

			//Don't import saved game records of these types that match
			//identical or newer versions of these saves already in the DB.
			if (this->eType == ST_Autosave || this->eType == ST_Manual ||
					this->eType == ST_Continue || this->eType == ST_Quicksave ||
					this->eType == ST_EndHold)
			{
				const UINT dwHoldID = CDbRooms::GetHoldIDForRoom(this->dwRoomID);
				if (!dwHoldID)
					bSaveRecord = false;
				else
				{
					//Match save records by type, hold, player and name (for auto and manual saves only).
					UINT existingSavedGameID = 0;
					switch (this->eType)
					{
						case ST_Manual:
							//For manual saves, avoid importing this record if it
							//has the same basic info and timestamps as an existing save.
							if (g_pTheDB->SavedGames.FindIdenticalSave(*this) != 0)
								bSaveRecord = false;
						break;
						case ST_Autosave:
						{
							WSTRING saveName = this->stats.GetVar(szSavename, wszEmpty);
							existingSavedGameID = g_pTheDB->SavedGames.FindByName(
									this->eType, dwHoldID, this->dwPlayerID, &saveName);
						}
						break;
						default:
							existingSavedGameID = g_pTheDB->SavedGames.FindByName(
									this->eType, dwHoldID, this->dwPlayerID);
						break;
					}

					//If there is a match, retain only the newest record.
					if (existingSavedGameID)
					{
						CDbSavedGame *pExistingSave = g_pTheDB->SavedGames.GetByID(existingSavedGameID);
						//For predefined save types/points, replace the existing record with
						//the newer record being imported.
						const bool bNewer = ((time_t)this->LastUpdated > (time_t)pExistingSave->LastUpdated);
						delete pExistingSave;
						if (bNewer)
						{
							const UINT dwOldLocalID = this->dwSavedGameID;
							this->dwSavedGameID = existingSavedGameID; //will overwrite existing save
							info.SavedGameIDMap[dwOldLocalID] = this->dwSavedGameID;
							info.importedSavedGameID = this->dwSavedGameID;  //keep track of which saved game was imported
							break; //don't save a fresh record below, but resave later once everything is parsed
						} else {
							bSaveRecord = false; //skip this older version
							//!!inform player of older versions of saved games not being imported?
						}
					}
				}
			}

			if (bSaveRecord)
			{
				//Add a new record to the DB.
				const UINT dwOldLocalID = this->dwSavedGameID;
				this->dwSavedGameID = 0;
				Update();
				info.SavedGameIDMap[dwOldLocalID] = this->dwSavedGameID;
				info.importedSavedGameID = this->dwSavedGameID;  //keep track of which saved game was imported
			} else {
				//This saved game is being ignored.
				//(It might belong to a non-existant hold/level/room.)
				info.SavedGameIDMap[this->dwSavedGameID] = 0;   //skip records with refs to this saved game ID
			}
		}
		break;
		case P_PlayerID:
			//This ID field is the first received.
			this->dwPlayerID = convertToUINT(str);
			if (!this->dwPlayerID)
				return MID_FileCorrupted;  //corrupt data (saved game must have player)

			//Look up local ID.
			localID = info.PlayerIDMap.find(this->dwPlayerID);
			if (localID == info.PlayerIDMap.end())
				return MID_FileCorrupted;  //record should exist now
			this->dwPlayerID = localID->second;

			//When importing saved games only, and not during a hold upgrade,
			//they become owned by the current player.
			//The above id mapping is still performed as a perfunctory integrity check.
			if (info.typeBeingImported == CImportInfo::SavedGame &&
					!info.bImportingSavedGames) //don't transfer ownership during hold upgrade
			{
				this->dwPlayerID = g_pTheDB->GetPlayerID();
				if (!this->dwPlayerID)
				{
					//There is no current player to merge this saved game into.
					//Quit import.
					bSaveRecord = false;
					return MID_SavesIgnored;
				}
			}
			break;
		case P_RoomID:
			this->dwRoomID = convertToUINT(str);

			//Look up local ID.
			localID = info.RoomIDMap.find(this->dwRoomID);
			//If room ID doesn't exist, then the saved game probably exists for a removed room.
			this->dwRoomID = localID != info.RoomIDMap.end() ? localID->second : 0;
			if (!this->dwRoomID)
			{
				//Records for this room are being ignored.  Don't save this game.
				bSaveRecord = false;
			}
			break;
		case P_Type:
			this->eType = static_cast<SAVETYPE>(convertToInt(str));
			if (!this->dwRoomID)
			{
				//2.0.9: A zero room ID is valid for some saved game types.
				//For other types, a zero ID indicates records for this room are
				//being ignored -- don't save this game.
				if (this->eType != ST_PlayerTotal)
					bSaveRecord = false;
			}
			if (info.typeBeingImported == CImportInfo::SavedGame || info.typeBeingImported == CImportInfo::Player)
			{
				//When importing a record claiming the hold is conquered,
				//only keep it if the room it's in has a hold exit.
				if (this->eType == ST_EndHold)
				{
					CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(this->dwRoomID);
					if (!pRoom)
						bSaveRecord = false;	//invalid save
					else
					{
						UINT wIndex;
						for (wIndex=0; wIndex<pRoom->Exits.size(); ++wIndex)
							if (!pRoom->Exits[wIndex]->dwEntranceID) break;	//end hold stairs found
						if (wIndex == pRoom->Exits.size()) //no end hold stairs found
							bSaveRecord = false;
						delete pRoom;
					}
				}

				if (info.typeBeingImported == CImportInfo::SavedGame)
				{
					//When importing only saved games, certain types of saved game records
					//get ignored.
					switch (this->eType)
					{
						case ST_Unknown:
//						case ST_Continue:
							bSaveRecord = false;
						break;
						default: break;	//all other types get conditionally saved
					}
				}
			}
			break;
/*
		case P_CheckpointX:
			this->wCheckpointX = convertToUINT(str);
			break;
		case P_CheckpointY:
			this->wCheckpointY = convertToUINT(str);
			break;
*/
		case P_IsHidden:
			this->bIsHidden = convertIntStrToBool(str);
			break;
		case P_LastUpdated:
			this->LastUpdated = convertToTimeT(str);
			break;
		case P_StartRoomX:
			this->wStartRoomX = convertToUINT(str);
			break;
		case P_StartRoomY:
			this->wStartRoomY = convertToUINT(str);
			break;
		case P_StartRoomO:
			this->wStartRoomO = convertToUINT(str);
			break;
		case P_StartRoomAppearance:
			this->wStartRoomAppearance = convertToUINT(str);
			break;
		case P_StartRoomSwordOff:
			this->bStartRoomSwordOff = convertIntStrToBool(str);
			break;
		case P_CompletedScripts:
		{
			//Parse list of script IDs.
			char *token = strtok((char*)str, " ");
			while (token)
			{
				//IDs don't need to be converted.
				const UINT dwScriptID = convertToUINT(token);
				if (!dwScriptID)
					return MID_FileCorrupted;  //corrupt file
				this->CompletedScripts += dwScriptID;
				token = strtok(NULL, " ");
			}
		}
		break;
		case P_EntrancesExplored:
		{
			//Parse list of hold entrance IDs.
			char *token = strtok((char*)str, " ");
			while (token)
			{
				//IDs don't need to be converted.
				const UINT entranceID = convertToUINT(token);
				this->entrancesExplored += entranceID;
				token = strtok(NULL, " ");
			}
		}
		break;
		case P_Created:
			this->Created = convertToTimeT(str);
			break;
		case P_Commands:
		{
			BYTE *data;
			Base64::decode(str,data);
			this->Commands = (const BYTE*)data;
			delete[] data;
			break;
		}
		case P_Stats:
		{
			BYTE *data;
			Base64::decode(str,data);
			this->stats = (const BYTE*)data;
			delete[] data;

			//Update any values interpreted as IDs.
			PlayerStats ps;
			ps.Unpack(this->stats);
			if (ps.priorRoomID)
			{
				//Look up local room ID.
				localID = info.RoomIDMap.find(ps.priorRoomID);

				//If room ID doesn't exist, then the imported ID probably exists
				//for a removed room -- reset it.
				ps.priorRoomID = localID != info.RoomIDMap.end() ? localID->second : 0;

				ps.Pack(this->stats);
			}

			break;
		}
		case P_Version:
			this->wVersionNo = convertToUINT(str);
			break;
		case P_ChecksumStr:
			this->checksumStr = str;
			break;

		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbSavedGame::SetProperty(
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
	static PrimaryKeyMap::iterator localID;
	switch (vpType)
	{
		case VP_ExploredRooms:
		{
			ASSERT(pType == P_Start || pImportExploredRoom);
			ASSERT(pType == P_Start || (!pImportERMonster && !pImportERPiece));
			//In version 1.0, the tags for the map marker and map only properties were transposed.
			//We must account for this error when importing older saves
			PROPTYPE propertyType = pType;
			if (info.wVersion < 500)
			{
				if (propertyType == P_MapMarker)
				{
					propertyType = P_MapOnly;
				}
				else if (propertyType == P_MapOnly)
				{
					propertyType = P_MapMarker;
				}
			}
			switch (propertyType)
			{
				case P_Start:
					ASSERT(!pImportExploredRoom);
					pImportExploredRoom = new ExploredRoom();
					break;
				case P_RoomID:
					pImportExploredRoom->roomID = convertToUINT(str);

					//Look up local ID.
					localID = info.RoomIDMap.find(pImportExploredRoom->roomID);
					//If room ID doesn't exist, then this record probably exists for a removed room.
					pImportExploredRoom->roomID = localID != info.RoomIDMap.end() ? localID->second : 0;
					break;
				case P_MapIcon:
				{
					pImportExploredRoom->mapIcon = (ScriptVars::MapIcon)convertToInt(str);
				}
				break;
				case P_MapIconState:
				{
					pImportExploredRoom->mapIconState = (ScriptVars::MapIconState)convertToInt(str);
				}
				break;
				case P_MapOnly:
				{
					//Update from 1.0
					bool bMapOnly = convertIntStrToBool(str);
					pImportExploredRoom->mapState = bMapOnly ? MapState::NoDetail : MapState::Explored;
				}
					break;
				case P_MapState:
					pImportExploredRoom->mapState = (MapState)convertToInt(str);
					break;
				case P_MapMarker:
					pImportExploredRoom->mapMarker = convertToUINT(str);
					break;
				case P_Squares:
				{
					BYTE *data;
					const UINT size = Base64::decode(str,data);
					pImportExploredRoom->SquaresBytes = c4_Bytes((const BYTE*)data, size, true);
					delete[] data;
				}
				break;
				case P_LitFuses:
				{
					//Parse list of coords.
					char *token = strtok((char*)str, " ");
					do
					{
						pImportExploredRoom->litFuses += convertToUINT(token);
						token = strtok(NULL, " ");
					} while (token);
				}
				break;
				case P_OrbTypes:
/*
				{
					//Parse list of orb types.
					char *token = strtok((char*)str, " ");
					do
					{
						pImportExploredRoom->orbTypes.push_back(convertToUINT(token));
						token = strtok(NULL, " ");
					} while (token);
				}
*/
				break;
				case P_TileLights:
				{
					BYTE* data;
					const UINT size = Base64::decode(str, data);
					pImportExploredRoom->tileLightsBytes = c4_Bytes((const BYTE*)data, size, true);
					delete[] data;
				}
				break;
				case P_End:
					//Finish processing
					if (pImportExploredRoom->roomID) //only save records for valid rooms
						this->ExploredRooms.push_back(pImportExploredRoom);
					else
						delete pImportExploredRoom;
					pImportExploredRoom = NULL;
					break;
				default:
					delete pImportExploredRoom;
					pImportExploredRoom = NULL;
					return MID_FileCorrupted;
			}
			break;
		}
		case VP_Monsters:
			ASSERT(pType == P_Start || pImportERMonster);
			switch (pType)
			{
				case P_Start:
				{
					CMonsterFactory mf;
					ASSERT(!pImportERMonster);
					pImportERMonster = mf.GetNewMonster(M_MIMIC);
					break;
				}
				case P_Type:
					pImportERMonster->wType = convertToUINT(str);
					break;
				case P_X:
					pImportERMonster->wX = convertToUINT(str);
					break;
				case P_Y:
					pImportERMonster->wY = convertToUINT(str);
					break;
				case P_O:
					pImportERMonster->wO = convertToUINT(str);
					if (pImportERMonster->wO >= ORIENTATION_COUNT)
					{
						delete pImportERMonster; pImportERMonster = NULL;
						return MID_FileCorrupted;
					}
					break;
				case P_ExtraVars:
				{
					BYTE *data;
					Base64::decode(str,data);
					pImportERMonster->ExtraVars = (const BYTE*)data;
					delete[] data;
/*
//Monsters in saved games are saved without script info, so they don't need speech import.
//If this step is performed, having an empty script would cause their attributes,
//saved in ExtraVars, to be cleared inside ImportSpeech.
					if (pImportERMonster->wType == M_CHARACTER)
					{
						//Can't dynamically cast pImportMonster to a CCharacter
						CMonsterFactory mf;
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*,
								mf.GetNewMonster(M_CHARACTER));
						pCharacter->ExtraVars = pImportERMonster->ExtraVars;
						const MESSAGE_ID val = pCharacter->ImportSpeech(info);
						if (val != MID_ImportSuccessful)
							return val;
						pImportERMonster->ExtraVars = pCharacter->ExtraVars;
						delete pCharacter;
					}
*/
					break;
				}
				case P_End:
				//Finish processing
				{
					ASSERT(info.typeBeingImported != CImportInfo::LanguageMod);
					if (pImportExploredRoom)
					{
						//Append monster to the explored room being parsed.
						if (!pImportExploredRoom->pMonsterList)
							pImportExploredRoom->pMonsterList = pImportERMonster;
						else
						{
							CMonster *pMonster = pImportExploredRoom->pMonsterList;
							while (pMonster->pNext)
								pMonster = pMonster->pNext;
							pMonster->pNext = pImportERMonster;
						}
					} else {
						//Append monster to the saved game's global monster list.
						if (!this->pMonsterListAtRoomStart)
							this->pMonsterListAtRoomStart = pImportERMonster;
						else
						{
							CMonster *pMonster = this->pMonsterListAtRoomStart;
							while (pMonster->pNext)
								pMonster = pMonster->pNext;
							pMonster->pNext = pImportERMonster;
						}
					}
					pImportERMonster = NULL;
				}
				break;
				default:
					delete pImportERMonster; pImportERMonster = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_Pieces:
			ASSERT(pImportERMonster);
			ASSERT(pType == P_Start || pImportERPiece);
			switch (pType)
			{
				case P_Start:
					ASSERT(!pImportERPiece);
					pImportERPiece = new CMonsterPiece(pImportERMonster);
					pImportERPiece->wType = pImportERMonster->wType;
					break;
				case P_Type:
					pImportERPiece->wTileNo = convertToUINT(str);
					break;
				case P_X:
					pImportERPiece->wX = convertToUINT(str);
					break;
				case P_Y:
					pImportERPiece->wY = convertToUINT(str);
					break;
				case P_End:
					//Finish processing
					pImportERMonster->Pieces.push_back(pImportERPiece);
					pImportERPiece = NULL;
					break;
				default:
					delete pImportERMonster; pImportERMonster = NULL;
					delete pImportERPiece;	 pImportERPiece = NULL;
					return MID_FileCorrupted;
			}
			break;
		case VP_PlatformDeltas:
			switch (pType)
			{
				case P_Start:
					ASSERT(pImportExploredRoom);
					break;
				case P_X:
					importPlatformDelta.wX = convertToUINT(str);
					break;
				case P_Y:
					importPlatformDelta.wY = convertToUINT(str);
					break;
				case P_End:
					//Finish processing
					pImportExploredRoom->platformDeltas.Push(importPlatformDelta.wX, importPlatformDelta.wY);
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
bool CDbSavedGame::Update()
//Updates database with saved game.
{
	g_pTheDB->SavedGames.ResetMembership();

	if (this->dwSavedGameID == 0)
		//Insert a new saved game.
		return UpdateNew();

	//Update existing saved game.
	return UpdateExisting();
}

//
//CDbSavedGame protected methods.
//

//*******************************************************************************
void CDbSavedGame::Clear(
//Frees resources associated with object and zeroes members.
//
//Params:
	const bool bNewGame)  //(in)   whether new game is starting [default=true]
{
	this->dwRoomID=this->dwSavedGameID=0L;
	this->bIsHidden=false;
	this->wStartRoomX=this->wStartRoomY=this->wStartRoomO=0;
	this->wStartRoomAppearance = defaultPlayerType(); //use default value
	this->bStartRoomSwordOff = false;
	this->scriptArrays.clear();
	this->eType = ST_Unknown;

	this->dwPlayerID = 0L;

	this->Commands.Clear();
	ClearDeadMonsters();

	if (bNewGame)
	{
		this->CompletedScripts.clear();
		this->entrancesExplored.clear();
		this->stats.Clear();
		this->moves.clear();

		DeleteExploredRooms();
		deleteMonsterList(this->pMonsterList);
		deleteMonsterList(this->pMonsterListAtRoomStart);
		this->pMonsterList = this->pMonsterListAtRoomStart = NULL;

		this->wVersionNo = 0;
		this->checksumStr.resize(0);
	}
}

//*****************************************************************************
void CDbSavedGame::ClearDeadMonsters()
//Frees memory and resets members for dead monster list.
{
	for (vector<CMonster*>::const_iterator iMonster = this->DeadMonsters.begin();
		iMonster != this->DeadMonsters.end(); ++iMonster)
	{
		CMonster *pDelete = *iMonster;
		ASSERT(pDelete);
		delete pDelete;
	}
	this->DeadMonsters.clear();
}

//*****************************************************************************
UINT CDbSavedGame::readBpUINT(const BYTE* buffer, UINT& index)
//Deserialize 1..5 bytes --> UINT
{
	const BYTE* buffer2 = buffer + (index++);
	ASSERT(*buffer2); // should not be zero (indicating a negative number)
	UINT n = 0;
	for (;; index++)
	{
		n = (n << 7) + *buffer2;
		if (*buffer2++ & 0x80)
			break;
	}

	return n - 0x80;
}

//*****************************************************************************
void CDbSavedGame::writeBpUINT(string& buffer, UINT n)
//Serialize UINT --> 1..5 bytes
{
	int s = 7;
	while (s < 32 && (n >> s))
		s += 7;

	while (s)
	{
		s -= 7;
		BYTE b = BYTE((n >> s) & 0x7f);
		if (!s)
			b |= 0x80;
		buffer.append(1, b);
	}
}

//
//CDbSavedGame private methods.
//

//*****************************************************************************
void CDbSavedGame::DeleteExploredRooms()
{
	for (vector<ExploredRoom*>::const_iterator room = this->ExploredRooms.begin();
			room != this->ExploredRooms.end(); ++room)
		delete *room;
	this->ExploredRooms.clear();
}

//*****************************************************************************
void CDbSavedGame::SaveCompletedScripts(
//Saves a set of script IDs for scripts that have been marked completed.
//
//Params:
	c4_View &CompletedScriptsView)
const
{
	CompletedScriptsView.SetSize(this->CompletedScripts.size());
	UINT wCount = 0;
	CIDSet::const_iterator iter;
	for (iter = this->CompletedScripts.begin(); iter != this->CompletedScripts.end(); ++iter)
	{
		ASSERT(*iter);
		p_ScriptID(CompletedScriptsView[wCount++]) = *iter;
	}
}

//*****************************************************************************
void CDbSavedGame::SaveEntrancesExplored(
//Saves a set of entrance IDs for hold entrances the player has entered.
//
//Params:
	c4_View &EntrancesExploredView)
const
{
	EntrancesExploredView.SetSize(this->entrancesExplored.size());
	UINT wCount = 0;
	CIDSet::const_iterator iter;
	for (iter = this->entrancesExplored.begin(); iter != this->entrancesExplored.end(); ++iter)
	{
		ASSERT(*iter);
		p_EntranceID(EntrancesExploredView[wCount++]) = *iter;
	}
}

//*****************************************************************************
void CDbSavedGame::SaveExploredRooms(
//Saves explored rooms from member vars of object into database.
//
//Params:
	c4_View &ExploredRoomsView)      //(in) Open view to fill.
const
{
	UINT wI, wRoomCount=0;
	for (wI=0; wI < this->ExploredRooms.size(); ++wI)
	{
		const ExploredRoom &room = *(this->ExploredRooms[wI]);
		if (room.bSave)
			++wRoomCount;
	}

	ExploredRoomsView.SetSize(wRoomCount);  //speed optimization

	for (wI=0, wRoomCount=0; wI < this->ExploredRooms.size(); ++wI)
	{
		const ExploredRoom &room = *(this->ExploredRooms[wI]);
		if (!room.bSave)
			continue;

		c4_View MonstersView, LitFusesView, PlatformDeltasView, OrbTypesView;
		UINT wCount = 0;

		LitFusesView.SetSize(room.litFuses.size());
		for (CIDSet::const_iterator iter = room.litFuses.begin();
				iter != room.litFuses.end(); ++iter)
			p_X(LitFusesView[wCount++]) = *iter;

		PlatformDeltasView.SetSize(room.platformDeltas.GetSize());
		for (wCount=0; wCount<room.platformDeltas.GetSize(); ++wCount)
		{
			UINT wX, wY;
			room.platformDeltas.GetAt(wCount, wX, wY);
			c4_RowRef row = PlatformDeltasView[wCount];
			p_X(row) = wX;
			p_Y(row) = wY;
		}

/*
		OrbTypesView.SetSize(room.orbTypes.size());
		for (vector<UINT>::const_iterator orb = room.orbTypes.begin();
				orb != room.orbTypes.end(); ++orb)
			p_X(OrbTypesView[wCount++]) = *orb;
*/

		SaveMonsters(MonstersView, room.pMonsterList);

		c4_RowRef row = ExploredRoomsView[wRoomCount++];
		p_RoomID(row) = room.roomID;
		p_MapState(row) = room.mapState;
		p_MapMarker(row) = room.mapMarker;
		p_MapIcon(row) = room.mapIcon;
		p_MapIconState(row) = room.mapIconState;
		p_Squares(row) = room.SquaresBytes;
		p_Monsters(row) = MonstersView;
		p_LitFuses(row) = LitFusesView;
		p_PlatformDeltas(row) = PlatformDeltasView;
		p_OrbTypes(row) = OrbTypesView;
		p_TileLights(row) = room.tileLightsBytes;
	}
}

//*****************************************************************************
void CDbSavedGame::SaveMonsters(
//Saves monsters from member vars of object into database.
//
//Params:
	c4_View &MonstersView, //(in) Open view to fill.
	CMonster *pMonsterList)
const
{
	//Speed optimization.
	UINT wCount = 0, wNumMonsters = 0;
	CMonster *pMonster = pMonsterList;
	while (pMonster)
	{
		++wNumMonsters;
		pMonster = pMonster->pNext;
	}
	MonstersView.SetSize(wNumMonsters);

	pMonster = pMonsterList;
	while (pMonster)
	{
		pMonster->Save(MonstersView[wCount++],
				false); //don't save NPC scripts here -- those are in the original room data
		pMonster = pMonster->pNext;
	}

	ASSERT(wCount == wNumMonsters); //these should match
}

//*****************************************************************************
void CDbSavedGame::SerializeScriptArrays()
//Converts script arrays into byte buffers that can be stored in CDbPackedVars
//Note: deserialization is done in CCurrentGame, as it requires hold information
{
	if (this->scriptArrays.empty()) {
		return;
	}

	for (ScriptArrayMap::const_iterator it = this->scriptArrays.cbegin();
		it != this->scriptArrays.cend(); ++it) {
		const map<int, int> arrayMap = it->second;
		string buffer;

		UINT size = 0;
		for (map<int, int>::const_iterator arrayIt = arrayMap.cbegin();
			arrayIt != arrayMap.cend(); ++arrayIt) {
			if (arrayIt->second == 0) {
				continue; //save space by skipping zero-value entries
			}

			writeBpUINT(buffer, (UINT)arrayIt->first);
			writeBpUINT(buffer, (UINT)arrayIt->second);
			++size;
		}

		string sizeBuffer;
		writeBpUINT(sizeBuffer, size);

		string varName("v");
		varName += std::to_string(it->first);
		this->stats.SetVar(varName.c_str(), (sizeBuffer + buffer).c_str());
	}
}

//*****************************************************************************
bool CDbSavedGame::UpdateNew()
//Add new SavedGames record to database.
{
	LOGCONTEXT("CDbSavedGame::UpdateNew");
	ASSERT(this->dwSavedGameID == 0);
	ASSERT(IsOpen());
	
	//Update checksum.
	this->checksumStr = g_pTheNet->GetChecksum(this);

	c4_View ExploredRoomsView;
	SaveExploredRooms(ExploredRoomsView);

	//Get subviews ready.
	c4_View CompletedScriptsView, EntrancesExploredView;
	SaveCompletedScripts(CompletedScriptsView);
	SaveEntrancesExplored(EntrancesExploredView);

	c4_View MonstersView;
	SaveMonsters(MonstersView, this->pMonsterListAtRoomStart);

	//Get stats into a buffer that can be written to db.
	UINT dwStatsSize;
	BYTE *pbytStatsBytes = this->stats.GetPackedBuffer(dwStatsSize);
	if (!pbytStatsBytes) return false;
	c4_Bytes StatsBytes(pbytStatsBytes, dwStatsSize);

	//Write SavedGames record.
	if (CDb::FreezingTimeStamps())
	{
		//Only assign values if they have never been set.
		if ((time_t)this->Created == 0)
			this->Created.SetToNow();
		if ((time_t)this->LastUpdated == 0)
			this->LastUpdated.SetToNow();
	} else {
		this->Created.SetToNow();
		this->LastUpdated.SetToNow();
	}
	this->dwSavedGameID = GetIncrementedID(p_SavedGameID);
	UINT dwCommandsSize;
	BYTE *pbytCommands = this->Commands.GetPackedBuffer(dwCommandsSize);
	c4_Bytes CommandsBytes(pbytCommands, dwCommandsSize);
	c4_RowRef row = g_pTheDB->SavedGames.GetNewRow();
	p_SavedGameID(row) = this->dwSavedGameID;
	p_PlayerID(row) = this->dwPlayerID;
	p_RoomID(row) = this->dwRoomID;
	p_Type(row) = this->eType;
	p_IsHidden(row) = this->bIsHidden;
	p_LastUpdated(row) = UINT(this->LastUpdated);
	p_StartRoomX(row) = this->wStartRoomX;
	p_StartRoomY(row) = this->wStartRoomY;
	p_StartRoomO(row) = this->wStartRoomO;
	p_StartRoomAppearance(row) = this->wStartRoomAppearance;
	p_StartRoomSwordOff(row) = this->bStartRoomSwordOff;
	p_ExploredRooms(row) = ExploredRoomsView;
	p_CompletedScripts(row) = CompletedScriptsView;
	p_EntrancesExplored(row) = EntrancesExploredView;
	p_Monsters(row) = MonstersView;
	p_Created(row) = UINT(this->Created);
	p_Commands(row) = CommandsBytes;
	p_Stats(row) = StatsBytes;
	p_Version(row) = this->wVersionNo;
	p_ChecksumStr(row) = c4_Bytes(this->checksumStr.c_str(), this->checksumStr.length());

	delete[] pbytCommands;
	delete[] pbytStatsBytes;

	if (this->eType != ST_Progress && this->eType != ST_PlayerTotal)
	{
		this->moves.savedGameID = this->dwSavedGameID;
		VERIFY(this->moves.Update());
	}

	CDb::addSavedGameToRoom(this->dwSavedGameID, this->dwRoomID);
	return true;
}

//*******************************************************************************
bool CDbSavedGame::UpdateExisting()
//Update an existing SavedGames record in database.
{
	LOGCONTEXT("CDbSavedGame::UpdateExisting");
	ASSERT(this->dwSavedGameID != 0);

	//Lookup SavedGames record.
	ASSERT(IsOpen());
	c4_View SavedGamesView;
	const UINT dwSavedGameI = LookupRowByPrimaryKey(this->dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGameI == ROW_NO_MATCH)
	{
		ASSERT(!"Bad SavedGameID.");
		return false;
	}

	//Update checksum.
	ASSERT(g_pTheNet);
	this->checksumStr = g_pTheNet->GetChecksum(this);

	c4_View ExploredRoomsView;
	SaveExploredRooms(ExploredRoomsView);
	
	//Get subviews ready.
	c4_View CompletedScriptsView, EntrancesExploredView;
	SaveCompletedScripts(CompletedScriptsView);
	SaveEntrancesExplored(EntrancesExploredView);

	c4_View MonstersView;
	SaveMonsters(MonstersView, this->pMonsterListAtRoomStart);

	SerializeScriptArrays();

	//Get stats into a buffer that can be written to db.
	UINT dwStatsSize;
	BYTE *pbytStatsBytes = this->stats.GetPackedBuffer(dwStatsSize);
	if (!pbytStatsBytes) return false;
	c4_Bytes StatsBytes(pbytStatsBytes, dwStatsSize);

	//Update SavedGames record.
	if (!CDb::FreezingTimeStamps())
		this->LastUpdated.SetToNow();
	UINT dwCommandsSize;
	BYTE *pbytCommands = this->Commands.GetPackedBuffer(dwCommandsSize);
	c4_Bytes CommandsBytes(pbytCommands, dwCommandsSize);
	
	c4_RowRef row = SavedGamesView[dwSavedGameI];
	CDb::moveSavedGame(this->dwSavedGameID, UINT(p_RoomID(row)), this->dwRoomID);

	p_SavedGameID(row) = this->dwSavedGameID;
	p_PlayerID(row) = this->dwPlayerID;
	p_RoomID(row) = this->dwRoomID;
	p_Type(row) = this->eType;
	p_IsHidden(row) = this->bIsHidden;
	p_LastUpdated(row) = UINT(this->LastUpdated);
	p_StartRoomX(row) = this->wStartRoomX;
	p_StartRoomY(row) = this->wStartRoomY;
	p_StartRoomO(row) = this->wStartRoomO;
	p_StartRoomAppearance(row) = this->wStartRoomAppearance;
	p_StartRoomSwordOff(row) = this->bStartRoomSwordOff;
	p_ExploredRooms(row) = ExploredRoomsView;
	p_CompletedScripts(row) = CompletedScriptsView;
	p_EntrancesExplored(row) = EntrancesExploredView;
	p_Monsters(row) = MonstersView;
	p_Created(row) = UINT(this->Created);
	p_Commands(row) = CommandsBytes;
	p_Stats(row) = StatsBytes;
	p_Version(row) = this->wVersionNo;
	p_ChecksumStr(row) = c4_Bytes(this->checksumStr.c_str(), this->checksumStr.length());

	delete[] pbytCommands;
	delete[] pbytStatsBytes;

	if (this->eType != ST_Progress && this->eType != ST_PlayerTotal)
	{
		this->moves.savedGameID = this->dwSavedGameID;
		VERIFY(this->moves.Update());
	}

	return true;
}

//*****************************************************************************
bool CDbSavedGame::SetMembers(
//For copy constructor and assignment operator.
//
//Params:
	const CDbSavedGame &Src)
{
	//primitive types
	this->dwSavedGameID = Src.dwSavedGameID;
	this->dwRoomID = Src.dwRoomID;
	this->dwPlayerID = Src.dwPlayerID;
	this->bIsHidden = Src.bIsHidden;
	this->eType = Src.eType;
	this->wStartRoomX = Src.wStartRoomX;
	this->wStartRoomY = Src.wStartRoomY;
	this->wStartRoomO = Src.wStartRoomO;
	this->wStartRoomAppearance = Src.wStartRoomAppearance;
	this->bStartRoomSwordOff = Src.bStartRoomSwordOff;

	//object members
	DeleteExploredRooms();
	this->ExploredRooms = GetCopyOfExploredRooms(Src.ExploredRooms);

	//Monster data
	deleteMonsterList(this->pMonsterList);
	deleteMonsterList(this->pMonsterListAtRoomStart);
	this->pMonsterList = copyMonsterList(Src.pMonsterList);
	this->pMonsterListAtRoomStart = copyMonsterList(Src.pMonsterListAtRoomStart);

	this->CompletedScripts = Src.CompletedScripts;
	this->entrancesExplored = Src.entrancesExplored;
	this->Created = Src.Created;
	this->LastUpdated = Src.LastUpdated;
	this->Commands = Src.Commands;
	this->moves = Src.moves;

	this->stats = Src.stats;
	this->scriptArrays = Src.scriptArrays;
	this->wVersionNo = Src.wVersionNo;
	this->checksumStr = Src.checksumStr;

	return true;
}

//*******************************************************************************
//Returns: a vector of pointers to a deep copy of explored room objects
vector<ExploredRoom*> CDbSavedGame::GetCopyOfExploredRooms(const vector<ExploredRoom*>& rooms)
{
	vector<ExploredRoom*> duplicate;
	for (vector<ExploredRoom*>::const_iterator room = rooms.begin();
		room != rooms.end(); ++room)
		duplicate.push_back(new ExploredRoom(*(*room)));
	return duplicate;
}

//Reassigns pointers in source object.
//'rooms' is cleared to preclude caller destroying transferred objects
void CDbSavedGame::ReplaceExploredRooms(vector<ExploredRoom*>& rooms) //(in/out) objects are reassigned
{
	DeleteExploredRooms();
	this->ExploredRooms = rooms;
	rooms.clear();
}

//
//CDbSavedGames public methods.
//

//*******************************************************************************
void CDbSavedGames::AddRoomsToPlayerTally(
//Adds these rooms to the player's room tally.
	const UINT dwPlayerID, const CIDSet& ExploredRooms)
{
	const UINT dwID = FindByType(ST_PlayerTotal, dwPlayerID);
	CDbSavedGame *pPlayerProgress = GetByID(dwID);
	if (!pPlayerProgress)
	{
		//Make new save record to store player's room tallies.
		pPlayerProgress = GetNew();
		pPlayerProgress->dwPlayerID = dwPlayerID;
		pPlayerProgress->eType = ST_PlayerTotal;
		pPlayerProgress->bIsHidden = true;
	}

	//Add ExploredRooms set to tally of all rooms explored.
	bool bUpdate=false;
	for (CIDSet::const_iterator iter = ExploredRooms.begin(); iter != ExploredRooms.end(); ++iter)
	{
		const UINT roomID = *iter;
		ASSERT(roomID);
		vector<ExploredRoom*>::const_iterator room;
		for (room = pPlayerProgress->ExploredRooms.begin();
				room != pPlayerProgress->ExploredRooms.end(); ++room)
		{
			const ExploredRoom *pRoom = *room;
			if (pRoom->roomID == roomID)
				break; //don't need to add this one
		}

		//Room not found -- add a record of it.
		if (room == pPlayerProgress->ExploredRooms.end())
		{
			ExploredRoom *pExpRoom = new ExploredRoom();
			pExpRoom->roomID = roomID;
			pExpRoom->mapState = MapState::NoDetail;
			pPlayerProgress->ExploredRooms.push_back(pExpRoom);
			bUpdate = true;
		}
	}

	if (bUpdate)
		pPlayerProgress->Update();

	delete pPlayerProgress;		
}

//*******************************************************************************
bool CDbSavedGames::CleanupPlayerTallies()
//Removes rooms which do not exist from all "PlayerTotal" saved games.
//These might be left over from a deleted hold.
{
	bool bAnyChanged = false;
	CDb db;
	CIDSet allRooms = db.Rooms.GetIDs();

	const UINT dwSavedGamesCount = GetViewSize();
	for (UINT dwSavedGamesI=0; dwSavedGamesI<dwSavedGamesCount; ++dwSavedGamesI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
		if (SAVETYPE(int(p_Type(row))) != ST_PlayerTotal)
			continue;

		CDbSavedGame *pPlayerProgress = GetByID(UINT(p_SavedGameID(row)));
		ASSERT(pPlayerProgress);
		if (!pPlayerProgress) continue;
		bool bChanged = false;

		vector<ExploredRoom*>::iterator iter = pPlayerProgress->ExploredRooms.begin();
		while (iter != pPlayerProgress->ExploredRooms.end())
		{
			ExploredRoom *pExpRoom = *iter;
			if (!allRooms.has(pExpRoom->roomID))
			{
				bChanged = true;
				delete pExpRoom;
				iter = pPlayerProgress->ExploredRooms.erase(iter);
			}
			else
				++iter;
		}

		if (bChanged)
		{
			pPlayerProgress->Update();
			bAnyChanged = true;
		}
		delete pPlayerProgress;
	}
	return bAnyChanged;
}

//*******************************************************************************
void CDbSavedGames::Delete(
//Deletes a saved games record.
//NOTE: Don't call this directly to delete saved games belonging to demos.
//Instead call CDbDemos::Delete() to delete the demo and associated saved game.
//
//Params:
	const UINT dwSavedGameID) //(in)   Saved game to delete.
{
	ASSERT(dwSavedGameID);

	c4_View SavedGamesView;
	const UINT dwSavedGameRowI = LookupRowByPrimaryKey(dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGameRowI==ROW_NO_MATCH) {ASSERT(!"Bad dwSavedGameID."); return;}

	CDb::deleteSavedGame(dwSavedGameID); //call first
	SavedGamesView.RemoveAt(dwSavedGameRowI);

	//Delete the child moves object.
	//This will work for pre-version 1.1 data.
	g_pTheDB->SavedGameMoves.Delete(dwSavedGameID);

	//After object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*******************************************************************************
void CDbSavedGames::DeleteForRoom(
//Deletes all saved games records (and demos) for the specified room.
//
//Params:
	const UINT dwRoomID)   //(in)  Room to delete data from.
{
	//Delete demos and their saved games...
	CDb db;
	CIDSet ids = CDb::getDemosInRoom(dwRoomID);
	CIDSet::const_iterator iter;
	for (iter = ids.begin(); iter != ids.end(); ++iter)
		db.Demos.Delete(*iter);

	//...and then saved games w/o demos.
	ids = CDb::getSavedGamesInRoom(dwRoomID);
	for (iter = ids.begin(); iter != ids.end(); ++iter)
		db.SavedGames.Delete(*iter);
}

//*****************************************************************************
void CDbSavedGames::ExportXML(
//Returns: string containing XML text describing saved game with this ID
//
//Pre-condition: dwSavedGameID is valid
//
//Params:
	const UINT dwSavedGameID, //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)    //(in)
{
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define STARTVPTAG(vpType,pType) "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">\n"
#define ENDVPTAG(vpType) "</"; str += ViewpropTypeStr(vpType); str += ">\n"
#define CLOSETAG "'/>\n"
#define CLOSESTARTTAG "'>\n"
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))
#define TIMETTOSTR(val) writeTimeT(dummy, sizeof(dummy), (val))

	if (dbRefs.IsSet(V_SavedGames,dwSavedGameID))
		return;

	dbRefs.Set(V_SavedGames,dwSavedGameID);

	CDbSavedGame *pSavedGame = GetByID(dwSavedGameID);
	if (!pSavedGame)
		return; //placeholder record -- not needed

	//Don't export saved games attached to demos that aren't being exported,
	//or "dummy" records tracking which demos have been uploaded.
	if ((pSavedGame->eType == ST_Demo && dbRefs.vTypeBeingExported == V_SavedGames) ||
			pSavedGame->eType == ST_DemoUpload)
	{
		delete pSavedGame;
		return;
	}

	//Include corresponding player and room refs.
	g_pTheDB->Players.ExportXML(pSavedGame->dwPlayerID, dbRefs, str, true);
	g_pTheDB->Rooms.ExportXML(pSavedGame->dwRoomID, dbRefs, str, true);

/*
//Don't save an NPC script's speech and data records into the saved game itself.
//Instead, NPC scripts are loaded from the original room data whenever an
//explored room is reentered.
#ifndef EXPORTNOSPEECH
	//For all explored rooms in saved game, export refs to speech data
	//so ID keys will be linked up properly when importing the saved game.
	if (!bRef)
	{
		for (UINT i=0; i<pSavedGame->ExploredRooms.size(); ++i)
		{
			for (CMonster *pMonster = pSavedGame->ExploredRooms[i]->pMonsterList;
					pMonster != NULL; pMonster = pMonster->pNext)
				if (M_CHARACTER == pMonster->wType)
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					str += CCharacter::ExportXMLSpeech(dbRefs, pCharacter->commands, true);
				}
		}
	}
#endif
*/

	//First include refs for rooms in explored list.
	vector<ExploredRoom*>::const_iterator iter;
	for (iter = pSavedGame->ExploredRooms.begin();
			iter != pSavedGame->ExploredRooms.end(); ++iter)
		g_pTheDB->Rooms.ExportXML((*iter)->roomID, dbRefs, str, true);
/*
	for (iter = pSavedGame->ConqueredRooms.begin();
			iter != pSavedGame->ConqueredRooms.end(); ++iter)
		g_pTheDB->Rooms.ExportXML(*iter, dbRefs, str, true);
*/

	//Prepare data.
	char dummy[32];

	str += STARTTAG(V_SavedGames, P_PlayerID);
	str += INT32TOSTR(pSavedGame->dwPlayerID);
	str += PROPTAG(P_RoomID);
	str += INT32TOSTR(pSavedGame->dwRoomID);
	str += PROPTAG(P_Type);
	str += INT32TOSTR(pSavedGame->eType);
/*
	str += PROPTAG(P_CheckpointX);
	str += INT32TOSTR(pSavedGame->wCheckpointX);
	str += PROPTAG(P_CheckpointY);
	str += INT32TOSTR(pSavedGame->wCheckpointY);
*/
	str += PROPTAG(P_IsHidden);
	str += INT32TOSTR(pSavedGame->bIsHidden);
	str += PROPTAG(P_LastUpdated);
	str += TIMETTOSTR((time_t)pSavedGame->LastUpdated);
	str += PROPTAG(P_StartRoomX);
	str += INT32TOSTR(pSavedGame->wStartRoomX);
	str += PROPTAG(P_StartRoomY);
	str += INT32TOSTR(pSavedGame->wStartRoomY);
	str += PROPTAG(P_StartRoomO);
	str += INT32TOSTR(pSavedGame->wStartRoomO);
	str += PROPTAG(P_StartRoomAppearance);
	str += INT32TOSTR(pSavedGame->wStartRoomAppearance);
	str += PROPTAG(P_StartRoomSwordOff);
	str += INT32TOSTR((bool)pSavedGame->bStartRoomSwordOff);

	if (!bRef)
	{
		//Process completed scripts.  Refs are not needed.
		if (!pSavedGame->CompletedScripts.empty())
		{
			str += PROPTAG(P_CompletedScripts);
			for (CIDSet::const_iterator iter = pSavedGame->CompletedScripts.begin();
				iter != pSavedGame->CompletedScripts.end(); ++iter)
			{
				str += INT32TOSTR(*iter);
				str += " ";
			}
		}

		//Process explored hold entrances.  Refs are not needed.
		if (!pSavedGame->entrancesExplored.empty())
		{
			str += PROPTAG(P_EntrancesExplored);
			for (CIDSet::const_iterator iter = pSavedGame->entrancesExplored.begin();
				iter != pSavedGame->entrancesExplored.end(); ++iter)
			{
				str += INT32TOSTR(*iter);
				str += " ";
			}
		}

		str += PROPTAG(P_Created);
		str += TIMETTOSTR((time_t)pSavedGame->Created);
		str += PROPTAG(P_Commands);
		UINT dwBufSize;
		BYTE *const pCommands = pSavedGame->Commands.GetPackedBuffer(dwBufSize);
		if (dwBufSize > sizeof(BYTE))
			str += Base64::encode(pCommands, dwBufSize-sizeof(BYTE));   //strip null BYTE
		delete[] pCommands;

		BYTE *pStats = pSavedGame->stats.GetPackedBuffer(dwBufSize);
		if (dwBufSize > 4)
		{
			str += PROPTAG(P_Stats);
			str += Base64::encode(pStats,dwBufSize-4);   //remove null UINT
		}
		delete[] pStats;
	}
	str += PROPTAG(P_Version);
	str += INT32TOSTR(pSavedGame->wVersionNo);
	str += PROPTAG(P_ChecksumStr);
	str += pSavedGame->checksumStr;

	//Put primary key last, so all relevant fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_SavedGameID);
	str += INT32TOSTR(pSavedGame->dwSavedGameID);

	if (bRef)
		str += CLOSETAG;
	else
	{
		str += CLOSESTARTTAG;

		//Export explored room data.
		for (UINT i=0; i<pSavedGame->ExploredRooms.size(); ++i)
		{
			ExploredRoom& r = *(pSavedGame->ExploredRooms[i]);

			str += STARTVPTAG(VP_ExploredRooms, P_RoomID);
			str += INT32TOSTR(r.roomID);
			str += PROPTAG(P_MapIcon);
			str += INT32TOSTR(r.mapIcon);
			str += PROPTAG(P_MapIconState);
			str += INT32TOSTR(r.mapIconState);
			str += PROPTAG(P_MapState);
			str += INT32TOSTR(r.mapState);
			str += PROPTAG(P_MapMarker);
			str += INT32TOSTR(r.mapMarker);
			str += PROPTAG(P_Squares);
			str += Base64::encode(r.SquaresBytes.Contents(), r.SquaresBytes.Size());
			str += PROPTAG(P_TileLights);
			str += Base64::encode(r.tileLightsBytes.Contents(), r.tileLightsBytes.Size());
			str += CLOSESTARTTAG;

			for (CMonster *pMonster = r.pMonsterList; pMonster != NULL; pMonster = pMonster->pNext)
				pMonster->ExportXML(str);

			if (!r.litFuses.empty())
			{
				str += PROPTAG(P_LitFuses);
				for (CIDSet::const_iterator iter = r.litFuses.begin();
					iter != r.litFuses.end(); ++iter)
				{
					str += INT32TOSTR(*iter);
					str += " ";
				}
				str += CLOSETAG;
			}

			if (r.platformDeltas.GetSize())
			{
				for (UINT i=0; i<r.platformDeltas.GetSize(); ++i)
				{
					UINT wX, wY;
					r.platformDeltas.GetAt(i, wX, wY);
					str += STARTVPTAG(VP_PlatformDeltas, P_X);
					str += INT32TOSTR(wX);
					str += PROPTAG(P_Y);
					str += INT32TOSTR(wY);
					str += CLOSETAG;
				}
			}
			
/*
			if (!r.orbTypes.empty())
			{
				str += PROPTAG(P_OrbTypes);
				for (vector<UINT>::const_iterator iter = r.orbTypes.begin();
					iter != r.orbTypes.end(); ++iter)
				{
					str += INT32TOSTR(*iter);
					str += " ";
				}
				str += CLOSETAG;
			}
*/
			str += ENDVPTAG(VP_ExploredRooms);
		}

		//Global monster records.
		CMonster *pMonster = pSavedGame->pMonsterListAtRoomStart;
		while (pMonster)
		{
			//CDbSavedGame::LoadMonster sets ExtraVars; don't call SetExtraVarsForExport here
			pMonster->ExportXML(str);
			pMonster = pMonster->pNext;
		}

		str += ENDTAG(V_SavedGames);

		//Finally, export attached move sequence data.
		g_pTheDB->SavedGameMoves.ExportXML(dwSavedGameID, dbRefs, str, false);
	}

	delete pSavedGame;

#undef STARTTAG
#undef STARTVPTAG
#undef PROPTAG
#undef ENDTAG
#undef ENDVPTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR
#undef TIMETTOSTR
}

//*********************************************************************************
void CDbSavedGames::FilterByHold(
//Changes filter so that GetFirst() and GetNext() will return saved games 
//for a specified hold (and player, if specified).
//
//Params:
	const UINT dwSetFilterByHoldID) //(in)   Hold ID to filter by.  Set to 0
								//    for all saved games.
{
	if (this->bIsMembershipLoaded && (dwSetFilterByHoldID != this->dwFilterByHoldID || 
		this->dwFilterByLevelID != 0 || this->dwFilterByRoomID != 0) )
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByLevelID = this->dwFilterByRoomID = 0;
	this->dwFilterByHoldID = dwSetFilterByHoldID;
}

//*********************************************************************************
void CDbSavedGames::FilterByLevel(
//Changes filter so that GetFirst() and GetNext() will return saved games 
//for a specified level (and player, if specified). 
//
//Params:
	const UINT dwSetFilterByLevelID)   //(in)   Level ID to filter by.  Set to
								//    0 for all saved games.
{
	if (this->bIsMembershipLoaded && (dwSetFilterByLevelID != this->dwFilterByLevelID || 
		this->dwFilterByHoldID != 0 || this->dwFilterByRoomID != 0) )
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = this->dwFilterByRoomID = 0;
	this->dwFilterByLevelID = dwSetFilterByLevelID;
}

//*********************************************************************************
void CDbSavedGames::FilterByPlayer(
//Changes filter so that GetFirst() and GetNext() will return saved games 
//for a specified player.  Other saved game filters stay in effect.
//
//Params:
	const UINT dwSetFilterByPlayerID)  //(in)   Player ID to filter by.
								// Set to 0 for all saved games.
								// (Other filters remain in effect.)
{
	if (this->bIsMembershipLoaded && (dwSetFilterByPlayerID != this->dwFilterByPlayerID))
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	//Don't zero other filter types.
	this->dwFilterByPlayerID = dwSetFilterByPlayerID;
}

//*********************************************************************************
void CDbSavedGames::FilterByRoom(
//Changes filter so that GetFirst() and GetNext() will return saved games
//for a specified room (and player, if specified).
//
//Params:
	const UINT dwSetFilterByRoomID) //(in)   Room ID to filter by.  Set to 0
								//    for all saved games.
{
	if (this->bIsMembershipLoaded && (dwSetFilterByRoomID != this->dwFilterByRoomID || 
		this->dwFilterByHoldID != 0 || this->dwFilterByLevelID != 0) )
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = this->dwFilterByLevelID = 0;
	this->dwFilterByRoomID = dwSetFilterByRoomID;
}

//*******************************************************************************
void CDbSavedGames::FindHiddens(const bool bFlag)
//Changes filter so that hidden saved games will also be returned (or not).
//Other filters remain the same.
{
	if (bFlag != this->bLoadHidden)
	{
      //Membership is invalid.
		this->bIsMembershipLoaded = false;
		this->bLoadHidden = bFlag;
	}
}

//*******************************************************************************
UINT CDbSavedGames::FindByContinue(const UINT type) //[default=ST_Continue]
//Finds ID of saved game that was saved to continue slot
//for the current player and hold.
//
//Returns:
//SavedGameID of the found continue saved game, or 0 if none were found.
{
	ASSERT(IsOpen());
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);

	return FindByContinue(dwCurrentPlayerID, g_pTheDB->GetHoldID(), type);
}

//*******************************************************************************
UINT CDbSavedGames::FindByContinue(const UINT playerID, const UINT holdID, const UINT type)
//Finds ID of saved game that was saved to continue slot
//for the current player and hold.
//
//Returns:
//SavedGameID of the found continue saved game, or 0 if none were found.
{
	ASSERT(IsOpen());
	ASSERT(playerID);
	if (!playerID)
		return 0;

	CIDSet savedGamesInHold = CDb::getSavedGamesInHold(holdID);

	//Each iteration looks at one saved game record for a match.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInHold.begin();
			savedGame != savedGamesInHold.end(); ++savedGame)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		ASSERT(dwSavedGameI != ROW_NO_MATCH);
		if (dwSavedGameI != ROW_NO_MATCH)
		{
			c4_RowRef row = SavedGamesView[dwSavedGameI];

			if ((UINT)SAVETYPE(int(p_Type(row))) == type)
				if (UINT(p_PlayerID(row)) == playerID)
					return UINT(p_SavedGameID(row)); //Found it.
		}
	}

	//Didn't find one.
	return 0;
}

//*******************************************************************************
UINT CDbSavedGames::SaveNewContinue(const UINT dwPlayerID, const UINT type) //[default=ST_Continue]
//Make new continue saved game slot for this player.
//
//Returns: new saved game ID
{
	CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetNew();
	pSavedGame->dwPlayerID = dwPlayerID;
	pSavedGame->dwRoomID = 0;
	pSavedGame->eType = SAVETYPE(type);
	pSavedGame->bIsHidden = true;
	pSavedGame->wStartRoomX = 0;
	pSavedGame->wStartRoomY = 0;
	pSavedGame->wVersionNo = VERSION_NUMBER;
	pSavedGame->Update();
	const UINT dwSavedGameID = pSavedGame->dwSavedGameID;
	delete pSavedGame;

	return dwSavedGameID;
}

//*******************************************************************************
UINT CDbSavedGames::FindByContinueLatest(
//Finds the latest continue saved game ID for the given player.
//
//Returns:
//SavedGameID of the found saved game, or 0 if no match found.
//
//Params:
	const UINT dwLookupPlayerID)
{
	ASSERT(IsOpen());
	ASSERT(dwLookupPlayerID);
	const UINT dwSavedGamesCount = GetViewSize();

	//Each iteration looks at one saved game record.
	UINT dwLatestSavedGameIndex = ROW_NO_MATCH;
	UINT dwLatestTime = 0L;
	for (UINT dwSavedGamesI = 0; dwSavedGamesI < dwSavedGamesCount; ++dwSavedGamesI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
		if (SAVETYPE(int(p_Type(row))) != ST_Continue)
			continue;
		if (UINT(p_PlayerID(row)) != dwLookupPlayerID)
			continue;
		if (UINT(p_RoomID(row)) == 0)
			continue;
		if (UINT(p_LastUpdated(row)) > dwLatestTime)
		{
			//This continue saved game is the most recent one found so far.
			dwLatestSavedGameIndex = dwSavedGamesI;
			dwLatestTime = (UINT) p_LastUpdated(row);
		}
	}

	//No continue slot found for player.
	if (dwLatestSavedGameIndex == ROW_NO_MATCH)
		return 0L;

	//Found player's most recent continue slot.
	const UINT dwSavedGameID = p_SavedGameID( GetRowRef(V_SavedGames, dwLatestSavedGameIndex) );
	return dwSavedGameID;
}

//*****************************************************************************
UINT CDbSavedGames::FindByEndHold(
//Finds the end hold saved game for the current player for the given hold.
//
//
//Params:
	const UINT dwQueryHoldID) //(in)  Hold to look in.
//Returns:
//ID of the found saved game, or 0 if no match found.
{
	if (!dwQueryHoldID) return 0;
	ASSERT(IsOpen());

	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);

	CIDSet savedGamesInHold = CDb::getSavedGamesInHold(dwQueryHoldID);

	//Each iteration looks at one saved game record for a match.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInHold.begin();
			savedGame != savedGamesInHold.end(); ++savedGame)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		c4_RowRef row = SavedGamesView[dwSavedGameI];

		if (SAVETYPE(int(p_Type(row))) != ST_EndHold)
			continue;
		if (UINT(p_PlayerID(row)) == dwCurrentPlayerID)
			return UINT(p_SavedGameID(row)); //Found it.
	}

	//No end hold slot found for player in this hold.
	return 0;
}

//*****************************************************************************
UINT CDbSavedGames::FindByLevelLatest(
//Finds the latest saved game on a specified level.
//
//Params:
	const UINT dwFindLevelID) //(in)   Level to look for.
//
//Returns:
//SavedGameID of the found saved game, or 0 if no match found.
{
	ASSERT(dwFindLevelID);
	ASSERT(IsOpen());

	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);

	//Ensure level exists.
	if (!g_pTheDB->Levels.Exists(dwFindLevelID))
	{
		ASSERT(!"FindByLevelLatest: Couldn't load level.");
		return 0L;
	}

	//Get IDs of saved games for the level.
	CDb db;
	CIDSet SavedGameIDs;
	db.SavedGames.FilterByLevel(dwFindLevelID);
	db.SavedGames.FilterByPlayer(dwCurrentPlayerID);
	db.SavedGames.GetIDs(SavedGameIDs);
	if (SavedGameIDs.empty())
		return 0L; //No saved games on this level.

	//Find the saved game with latest date.  Each iteration looks at the date of one
	//saved game.
	c4_View SavedGamesView;
	UINT dwLatestSavedGameID = 0, dwLatestTime = 0;
	for (CIDSet::const_iterator iter = SavedGameIDs.begin(); iter != SavedGameIDs.end(); ++iter)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*iter, V_SavedGames, SavedGamesView);
		if (dwSavedGameI == ROW_NO_MATCH)
		{
			ASSERT(!"Corrupted DB data.");
			continue;
		}
		c4_RowRef row = SavedGamesView[dwSavedGameI];
		if (UINT(p_PlayerID(row)) != dwCurrentPlayerID)
			continue;
		if (UINT(p_LastUpdated(row)) <= dwLatestTime)
			continue;
		SAVETYPE eSaveType = (SAVETYPE) (int) p_Type(row);
		if (eSaveType != ST_Demo && eSaveType != ST_Continue)
		{
			dwLatestSavedGameID = (UINT) p_SavedGameID(row);
			dwLatestTime = (UINT) p_LastUpdated(row);
		}
	}
	ASSERT(dwLatestSavedGameID);

	//Return the latest saved game.
	return dwLatestSavedGameID;
}

//*****************************************************************************
UINT CDbSavedGames::FindByName(const UINT eSavetype, const WSTRING& name)
//Finds ID of a saved game that was saved to a slot with the given name and type,
//for the current player and hold.
//
//Returns:
//SavedGameID of the found saved game, or 0 if none were found.
{
	return FindByName(eSavetype, g_pTheDB->GetHoldID(), g_pTheDB->GetPlayerID(), &name);
}

//*****************************************************************************
UINT CDbSavedGames::FindByName(
//Finds ID of a saved game that was saved to a slot with the given type,
//for the indicated player, hold, and name, if requested.
//
//Params:
	const UINT eSavetype,
	const UINT holdID, const UINT playerID,
	const WSTRING *pName) //if NULL, then ignore this field [default=NULL]
//
//Returns:
//SavedGameID of the found saved game, or 0 if none were found.
{
	ASSERT(IsOpen());

	if (!holdID || !playerID)
		return 0; //not valid

	CIDSet savedGamesInHold = CDb::getSavedGamesInHold(holdID);

	//Each iteration looks at one saved game record for a match.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInHold.begin();
			savedGame != savedGamesInHold.end(); ++savedGame)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*savedGame, V_SavedGames,
				SavedGamesView);
		ASSERT(dwSavedGameI != ROW_NO_MATCH);
		if (dwSavedGameI != ROW_NO_MATCH)
		{
			c4_RowRef row = SavedGamesView[dwSavedGameI];

			if ((UINT)SAVETYPE(int(p_Type(row))) == eSavetype)
				if (UINT(p_PlayerID(row)) == playerID)
				{
					//Compare names.
					CDbPackedVars stats;
					stats = p_Stats(row);
					WSTRING saveName = stats.GetVar(szSavename, wszEmpty);
					if (!pName || !saveName.compare(*pName))
						return UINT(p_SavedGameID(row)); //Found it.
				}
		}
	}

	//Didn't find one.
	return 0;
}

//*****************************************************************************
UINT CDbSavedGames::FindIdenticalSave(const CDbSavedGame& that)
//Searches for a saved game record in the DB matching the general creation
//info of the record passed in.
//
//Returns: SavedGameID of the found matching saved game, or 0 if no match found.
{
	const UINT holdID = CDbRooms::GetHoldIDForRoom(that.dwRoomID);
	if (!holdID || !that.dwPlayerID)
		return 0; //not valid

	//Hold must match.
	CIDSet savedGamesInHold = CDb::getSavedGamesInHold(holdID);

	//Each iteration looks at one saved game record for a match.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInHold.begin();
			savedGame != savedGamesInHold.end(); ++savedGame)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*savedGame, V_SavedGames,
				SavedGamesView);
		ASSERT(dwSavedGameI != ROW_NO_MATCH);
		if (dwSavedGameI != ROW_NO_MATCH)
		{
			c4_RowRef row = SavedGamesView[dwSavedGameI];

			//Save type, playerID, creation and last modification times must match.
			//The name field is ignored for speed, but may be considered
			//if a stronger match is desired.
			if (SAVETYPE(int(p_Type(row))) == that.eType)
				if (UINT(p_PlayerID(row)) == that.dwPlayerID)
					if ((time_t)(p_Created(row)) == (time_t)that.Created)
						if ((time_t)(p_LastUpdated(row)) == (time_t)that.LastUpdated)
							return UINT(p_SavedGameID(row)); //Found a match.
		}
	}

	//Didn't find one.
	return 0;
}

//*****************************************************************************
UINT CDbSavedGames::FindByRoomLatest(
//Finds the latest saved game in a specified room.
//
//Params:
	const UINT dwFindRoomID) //(in) Room to look for.
//
//Returns: SavedGameID of the found saved game, or 0 if no match found.
{
	ASSERT(dwFindRoomID);
	ASSERT(IsOpen());

	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);

	//Get IDs of saved games for the room.
	CIDSet savedGamesInRoom = CDb::getSavedGamesInRoom(dwFindRoomID);

	//Find the saved game with latest date.
	//Each iteration looks at the date of one saved game.
	c4_View SavedGamesView;
	UINT dwLatestSavedGameID = 0;
	UINT dwLatestTime = 0;
	for (CIDSet::const_iterator iter = savedGamesInRoom.begin(); iter != savedGamesInRoom.end(); ++iter)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*iter, V_SavedGames, SavedGamesView);
		ASSERT(dwSavedGameI != ROW_NO_MATCH);
		c4_RowRef row = SavedGamesView[dwSavedGameI];

		if (p_IsHidden(row) != 0)
			continue;
		if (UINT(p_PlayerID(row)) != dwCurrentPlayerID)
			continue;
		if (UINT(p_LastUpdated(row)) <= dwLatestTime)
			continue;
		const SAVETYPE eSaveType = (SAVETYPE) (int) p_Type(row);
		if (eSaveType != ST_Demo && eSaveType != ST_Continue)
		{
			dwLatestSavedGameID = (UINT) p_SavedGameID(row);
			dwLatestTime = (UINT) p_LastUpdated(row);
		}
	}
	ASSERT(dwLatestSavedGameID);

	//Return the latest saved game.
	return dwLatestSavedGameID;
}

//*****************************************************************************
UINT CDbSavedGames::FindByType(
//Finds the first saved game of the specified type in the DB.
//
//Params:
	const SAVETYPE eType, //(in)  Saved game type to look up.
	const UINT dwPlayerID,	//(in) [default=0]
	const bool bBackwardsSearch) //[default=true]
//Returns:
//ID of the found saved game, or 0 if no match found.
{
	ASSERT(IsOpen());

	const UINT dwSavedGamesCount = GetViewSize();

	if (bBackwardsSearch)
	{
		//Each iteration looks at one saved game record.
		//Search is done from back to front, as the record we're looking for, as a
		//rule of thumb, will be very recent.
		for (UINT dwSavedGamesI=dwSavedGamesCount; dwSavedGamesI--; )
		{
			c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
			if (SAVETYPE(int(p_Type(row))) != eType)
				continue;

			//Optional player ID filter.
			if (dwPlayerID && dwPlayerID != UINT(p_PlayerID(row)))
				continue;

			return UINT(p_SavedGameID(row)); //Found it.
		}
	} else {
		for (UINT dwSavedGamesI=0; dwSavedGamesI<dwSavedGamesCount; ++dwSavedGamesI)
		{
			c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
			if (SAVETYPE(int(p_Type(row))) != eType)
				continue;

			//Optional player ID filter.
			if (dwPlayerID && dwPlayerID != UINT(p_PlayerID(row)))
				continue;

			return UINT(p_SavedGameID(row)); //Found it.
		}
	}

	//No saved game of this type found.
	return 0;
}

//*****************************************************************************
UINT CDbSavedGames::GetHoldIDofSavedGame(
//Get the ID of the hold a saved game belongs to.
//
//Params:
	const UINT dwSavedGameID) //(in)
{
	return CDbRooms::GetHoldIDForRoom(GetRoomIDofSavedGame(dwSavedGameID));
}

//*****************************************************************************
UINT CDbSavedGames::GetPlayerIDofSavedGame(
//Get the ID of the player a saved game belongs to.
//
//Params:
	const UINT dwSavedGameID)   //(in)
{
	if (!dwSavedGameID)
		return 0;

	//Look up PlayerID from associated SavedGames record.
	c4_View SavedGamesView;
	const UINT dwSavedGamesRowI = LookupRowByPrimaryKey(dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGamesRowI == ROW_NO_MATCH) {ASSERT(!"Saved game row missing."); return 0;}
	return UINT(p_PlayerID(SavedGamesView[dwSavedGamesRowI]));
}

//*****************************************************************************
UINT CDbSavedGames::GetRoomIDofSavedGame(const UINT dwSavedGameID)
//Get the ID of the room a saved game belongs to.
{
	if (!dwSavedGameID) return 0;
	c4_View SavedGamesView;
	const UINT dwSavedGamesRowI = LookupRowByPrimaryKey(dwSavedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGamesRowI == ROW_NO_MATCH) {ASSERT(!"Saved game row missing."); return 0;}
	return UINT(p_RoomID(SavedGamesView[dwSavedGamesRowI]));
}

//*****************************************************************************
UINT CDbSavedGames::GetSavedGameID(
//Compares arguments against saved game records in the DB.
//
//Returns: saved game ID if a saved game record in the DB matches these parameters, else 0
	const UINT dwRoomID, const CDate& Created, const UINT dwPlayerID)
{
	ASSERT(IsOpen());

	//Get all saved games in the specified room.
	CIDSet savedGamesInRoom = CDb::getSavedGamesInRoom(dwRoomID);

	//Each iteration checks a demo's GIDs.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInRoom.begin();
			savedGame != savedGamesInRoom.end(); ++savedGame)
	{
		const UINT dwSavedGameI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		c4_RowRef row = SavedGamesView[dwSavedGameI];

		const time_t DemoCreated = (time_t)p_Created(row);
		if (DemoCreated != Created)
			continue;

		//Check author.
		if (UINT(p_PlayerID(row)) == dwPlayerID)
		{
			//Signatures match.  Return this record's ID.
			return (UINT)p_SavedGameID(row);
		}
	}

	//No match.
	return 0;
}

//*******************************************************************************
UINT CDbSavedGames::GetScore(const PlayerStats& st)
//Return: score for these player stats
{
	UINT dwScore = 0;
	dwScore += CalculateStatScore(st.HP, st.scoreHP);
	dwScore += CalculateStatScore(st.ATK, st.scoreATK);
	dwScore += CalculateStatScore(st.DEF, st.scoreDEF);
	dwScore += CalculateStatScore(st.yellowKeys, st.scoreYellowKeys);
	dwScore += CalculateStatScore(st.greenKeys, st.scoreGreenKeys);
	dwScore += CalculateStatScore(st.blueKeys, st.scoreBlueKeys);
	dwScore += CalculateStatScore(st.skeletonKeys, st.scoreSkeletonKeys);
	dwScore += CalculateStatScore(st.GOLD, st.scoreGOLD);
	dwScore += CalculateStatScore(st.XP, st.scoreXP);
	dwScore += CalculateStatScore(st.shovels, st.scoreShovels);

	return dwScore;
}

//*******************************************************************************
UINT CDbSavedGames::CalculateStatScore(const int stat, const int scoreMultiplier)
//Return: score for a particular stat
{
	const int maxAllowedScore = 100000000;
	if (scoreMultiplier > 0)
	{
		if (stat > maxAllowedScore / scoreMultiplier) return maxAllowedScore;
		if (stat < -maxAllowedScore / scoreMultiplier) return -maxAllowedScore;
		return stat * scoreMultiplier;
	}
	if (scoreMultiplier < 0)
	{
		return min(maxAllowedScore, max(-maxAllowedScore, stat / abs(scoreMultiplier)));
	}
	return 0;
}

//*****************************************************************************
CIDSet CDbSavedGames::GetExploredRooms(const UINT savedGameID)
//Returns: set of explored rooms for saved game with specified ID
{
	CIDSet ids;
	c4_View SavedGamesView;
	const UINT dwSavedGameI = LookupRowByPrimaryKey(savedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGameI == ROW_NO_MATCH)
		return ids;

	c4_View RoomsView = p_ExploredRooms(SavedGamesView[dwSavedGameI]);
	const UINT roomCount = RoomsView.GetSize();
	for (UINT dwRoomI = 0; dwRoomI < roomCount; ++dwRoomI)
		ids += UINT(p_RoomID(RoomsView[dwRoomI]));

	return ids;
}

//*****************************************************************************
vector<SAVE_INFO> CDbSavedGames::GetSaveInfo(const CIDSet& savedGameIDs)
//Returns: a vector of saved game info structs
{
	vector<SAVE_INFO> saves;
	c4_View SavedGamesView;
	CDbPackedVars stats;

	for (CIDSet::const_iterator iter = savedGameIDs.begin(); iter != savedGameIDs.end(); ++iter)
	{
		const UINT savedGameID = *iter;
		const UINT dwSavedGameI = LookupRowByPrimaryKey(savedGameID,
				V_SavedGames, SavedGamesView);
		if (dwSavedGameI == ROW_NO_MATCH)
			continue;
		c4_RowRef row = SavedGamesView[dwSavedGameI];

		SAVE_INFO save;
		save.saveID = savedGameID;
		save.type = UINT(p_Type(row));
		save.timestamp = (UINT)(time_t)(p_LastUpdated(row));
		stats = p_Stats(row);
		save.name = stats.GetVar(szSavename, wszEmpty);

		//The saved game can be validated if (1) it is accompanied a full move sequence record (version 1.1+),
		//or (2) this is the first room of a play session.
		const c4_View& ExploredRoomsView = p_ExploredRooms(row);
		save.bCanValidate = ExploredRoomsView.GetSize() == 0;
		if (!save.bCanValidate)
			save.bCanValidate = g_pTheDB->SavedGameMoves.Exists(savedGameID);

		saves.push_back(save);
	}

	return saves;
}

//*****************************************************************************
bool CDbSavedGames::RenameSavedGame(const UINT savedGameID, const WSTRING& name)
{
	c4_View SavedGamesView;
	const UINT dwSavedGameI = LookupRowByPrimaryKey(savedGameID,
		V_SavedGames, SavedGamesView);
	if (dwSavedGameI == ROW_NO_MATCH)
		return false;
	c4_RowRef row = SavedGamesView[dwSavedGameI];

	CDbPackedVars stats;
	stats = p_Stats(row);
	stats.SetVar(szSavename, name.c_str());

	//Get stats into a buffer that can be written to db.
	UINT dwStatsSize;
	BYTE* pbytStatsBytes = stats.GetPackedBuffer(dwStatsSize);
	if (!pbytStatsBytes)
		return false;
	c4_Bytes StatsBytes(pbytStatsBytes, dwStatsSize);
	p_Stats(row) = StatsBytes;

	return true;
}

//*****************************************************************************
SAVETYPE CDbSavedGames::GetType(const UINT savedGameID)
//Returns: type of saved game for saved game with specified ID
{
	c4_View SavedGamesView;
	const UINT dwSavedGameI = LookupRowByPrimaryKey(savedGameID,
			V_SavedGames, SavedGamesView);
	if (dwSavedGameI == ROW_NO_MATCH)
		return ST_Unknown;

	return SAVETYPE(int(p_Type(SavedGamesView[dwSavedGameI])));
}

//******************************************************************************
void CDbSavedGames::UpdatePlayerTallies(
//Modify all ST_PlayerTotals saves when replacing a hold to use the new room IDs.
//
//Params:
	const CImportInfo& info)
{
	const UINT dwSavedGamesCount = GetViewSize();
	for (UINT dwSavedGamesI=0; dwSavedGamesI<dwSavedGamesCount; ++dwSavedGamesI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
		if (SAVETYPE(int(p_Type(row))) != ST_PlayerTotal)
			continue;

		CDbSavedGame *pPlayerProgress = GetByID(UINT(p_SavedGameID(row)));
		ASSERT(pPlayerProgress);
		if (!pPlayerProgress)
			continue;
		bool bChanged = false;

		//Transform old room IDs to new room IDs.
		for (PrimaryKeyMap::const_iterator room_iter=info.RoomIDMap.begin();
		     room_iter!=info.RoomIDMap.end(); ++room_iter)
		{
			for (vector<ExploredRoom*>::iterator iter = pPlayerProgress->ExploredRooms.begin();
					iter != pPlayerProgress->ExploredRooms.end(); ++iter)
			{
				ExploredRoom *pExpRoom = *iter;
				if (pExpRoom->roomID == room_iter->first)
				{
					pExpRoom->roomID = room_iter->second;
					bChanged = true;
					break;
				}
			}
		}

		if (bChanged)
			pPlayerProgress->Update();
		delete pPlayerProgress;
	}
}

//
//CDbSavedGames private methods.
//

//*******************************************************************************
void CDbSavedGames::LoadMembership()
//Load the membership list with all saved game IDs.
//Player filtering is done internal to hold, level, and room filtering.
{
	ASSERT(IsOpen());
	this->MembershipIDs.clear();

	//Call function load membership with appropriate filtering.
	if (this->dwFilterByHoldID)
		LoadMembership_ByHold(this->dwFilterByHoldID);
	else if (this->dwFilterByLevelID)
		LoadMembership_ByLevel(this->dwFilterByLevelID);
	else if (this->dwFilterByRoomID)
		LoadMembership_ByRoom(this->dwFilterByRoomID);
	else if (this->dwFilterByPlayerID)
		LoadMembership_ByPlayer(this->dwFilterByPlayerID);
	else
		LoadMembership_All();

	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//*******************************************************************************
void CDbSavedGames::LoadMembership_All()
//Loads membership list from all saved games.
{
	const UINT dwSavedGamesCount = GetViewSize();

	//Each iteration gets a saved game ID and puts in membership list.
	for (UINT dwSavedGamesI = 0L; dwSavedGamesI < dwSavedGamesCount; ++dwSavedGamesI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
		if (this->bLoadHidden || p_IsHidden(row) == 0)
			this->MembershipIDs += p_SavedGameID(row);
	}
}

//*******************************************************************************
void CDbSavedGames::LoadMembership_ByRoom(const UINT dwByRoomID)
//Loads membership list from saved games in a specified room,
//and for specified player, if any.
{
	c4_View SavedGamesView;
	CIDSet savedGamesInRoom = CDb::getSavedGamesInRoom(dwByRoomID);

	//Each iteration processes a saved game ID and maybe puts it in membership list.
	for (CIDSet::const_iterator savedGame = savedGamesInRoom.begin();
			savedGame != savedGamesInRoom.end(); ++savedGame)
	{
		const UINT savedGameRowI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		c4_RowRef row = SavedGamesView[savedGameRowI];

		if (this->bLoadHidden || p_IsHidden(row) == 0)
		{
			//Does player ID match filter?
			if (!this->dwFilterByPlayerID)
				this->MembershipIDs += p_SavedGameID(row);
			else
			{
				const UINT dwPlayerID = (UINT) p_PlayerID(row);
				if (dwPlayerID == this->dwFilterByPlayerID)
					this->MembershipIDs += p_SavedGameID(row);
			}
		}
	}
}

//*******************************************************************************
void CDbSavedGames::LoadMembership_ByPlayer(const UINT dwByPlayerID)
//Loads membership list from saved games for a specified player.
{
	const UINT dwSavedGamesCount = GetViewSize();

	//Each iteration gets a saved game ID and puts in membership list.
	for (UINT dwSavedGamesI = 0L; dwSavedGamesI < dwSavedGamesCount; ++dwSavedGamesI)
	{
		c4_RowRef row = GetRowRef(V_SavedGames, dwSavedGamesI);
		if (UINT(p_PlayerID(row)) == dwByPlayerID && 
				(this->bLoadHidden || p_IsHidden(row) == 0))
			this->MembershipIDs += p_SavedGameID(row);
	}
}

//*******************************************************************************
void CDbSavedGames::LoadMembership_ByLevel(const UINT dwByLevelID)
//Loads membership list from saved games in a specified level,
//and for specified player, if any.
{
	if (!g_pTheDB->Levels.Exists(dwByLevelID))
	{
		ASSERT(!"Failed to retrieve level.");
		return;
	}

	//Compile IDs of all saved games in the specified level.
	c4_View SavedGamesView;
	CIDSet savedGamesInLevel = CDb::getSavedGamesInLevel(dwByLevelID);

	//Each iteration processes a saved game ID and maybe puts it in membership list.
	for (CIDSet::const_iterator savedGame = savedGamesInLevel.begin();
			savedGame != savedGamesInLevel.end(); ++savedGame)
	{
		const UINT savedGameRowI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		ASSERT(savedGameRowI != ROW_NO_MATCH);
		c4_RowRef row = SavedGamesView[savedGameRowI];
		if (this->bLoadHidden || p_IsHidden(row) == 0)
			if (!this->dwFilterByPlayerID || UINT(p_PlayerID(row)) == this->dwFilterByPlayerID)
				this->MembershipIDs += p_SavedGameID(row);
	}
}

//*******************************************************************************
void CDbSavedGames::LoadMembership_ByHold(const UINT dwByHoldID)
//Loads membership list from saved games in a specified level,
//and for specified player, if any.
{
	CIDSet savedGamesInHold = CDb::getSavedGamesInHold(dwByHoldID);

	//Each iteration processes a saved game ID and maybe puts it in membership list.
	c4_View SavedGamesView;
	for (CIDSet::const_iterator savedGame = savedGamesInHold.begin();
			savedGame != savedGamesInHold.end(); ++savedGame)
	{
		const UINT savedGameRowI = LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		ASSERT(savedGameRowI != ROW_NO_MATCH);
		c4_RowRef row = SavedGamesView[savedGameRowI];
		if (this->bLoadHidden || p_IsHidden(row) == 0)
			if (!this->dwFilterByPlayerID || UINT(p_PlayerID(row)) == this->dwFilterByPlayerID)
				this->MembershipIDs += p_SavedGameID(row);
	}
}
