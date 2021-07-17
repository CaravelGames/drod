// $Id: DbBase.cpp 10203 2012-05-19 21:25:31Z skell $

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
 * 1997, 2000, 2001, 2002, 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbBase.cpp
//Implementation of CDbBase.

//Define DbProps in this object
#define INCLUDED_FROM_DBBASE_CPP

#include "DbBase.h"
#include "Db.h"
#include "DbProps.h"
#include "GameConstants.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Ports.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

#include <fstream>

#ifndef PATCH
//Uncomment this to build a patch executable.
#	define PATCH
#endif

//One for each data file.
bool CDbBase::bDirtyData = false;
bool CDbBase::bDirtyHold = false;
bool CDbBase::bDirtyPlayer = false;
bool CDbBase::bDirtySave = false;
bool CDbBase::bDirtyText = false;

//Module-scope vars.
set<CDbBase*> m_dbRefs;
c4_Storage *m_pMainStorage = NULL;
c4_Storage *m_pDataStorage = NULL;
c4_Storage *m_pHoldStorage = NULL;
c4_Storage *m_pPlayerStorage = NULL;
c4_Storage *m_pSaveStorage = NULL;
c4_Storage *m_pSaveMoveStorage = NULL; //auxiliary to pSaveStorage
c4_Storage *m_pTextStorage = NULL;

const WCHAR pwszDrod[] = { We('d'),We('r'),We('o'),We('d'),We(0) };
const WCHAR pwszDotDat[] = { We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszData[] = { We('d'),We('a'),We('t'),We('a'),We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszHold[] = { We('h'),We('o'),We('l'),We('d'),We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszPlayer[] = { We('p'),We('l'),We('a'),We('y'),We('e'),We('r'),We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszSave[] = { We('s'),We('a'),We('v'),We('e'),We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszSaveMoves[] = { We('s'),We('a'),We('v'),We('e'),We('m'),We('o'),We('v'),We('e'),We('s'),We('.'),We('d'),We('a'),We('t'),We(0) };
const WCHAR pwszText[] = { We('t'),We('e'),We('x'),We('t'),We('.'),We('d'),We('a'),We('t'),We(0) };

//Accelerated lookup index.
typedef map<UINT,CIDSet> messageIDsMap;
messageIDsMap messageIndex; //message -> global rows in DB
CIDSet messageIDsMarkedForDeletion;

//Used for checking the reference count at application exit.
UINT GetDbRefCount() {return m_dbRefs.size();}

//
// Utility methods.
//

void GetWString(WSTRING& wstr, const c4_Bytes& Bytes)
//Assigns Unicode-16 text contained in 'Bytes' to 'wstr'.  Performs correct Endian conversion.
{
	if (!Bytes.Size())
	{
		wstr.resize(0);
		return;
	}

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	const UINT bytesSize = Bytes.Size();
	const UINT wchars = bytesSize/sizeof(WCHAR);
	WCHAR *pwzBuf = new WCHAR[wchars];
	memcpy((void*)pwzBuf, (const void*)Bytes.Contents(), bytesSize);
	LittleToBig(pwzBuf, wchars-1);
	wstr.assign(pwzBuf, wchars-1);
	delete[] pwzBuf;
#else
	wstr.assign((WCHAR*)Bytes.Contents(), Bytes.Size()/sizeof(WCHAR) - 1);
#endif
}

c4_Bytes PutWString(const WSTRING& wstr)
//Assigns Unicode-16 text to 'Bytes'.  Performs correct Endian conversion.
{
	const UINT length = wstr.size();
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	WCHAR *pBytes = new WCHAR[length+1];
	const UINT bytesSize = (length+1)*sizeof(WCHAR);
	memcpy(pBytes, wstr.c_str(), bytesSize);
	LittleToBig(pBytes, length);
	c4_Bytes Bytes(pBytes, bytesSize, 1); //allocate a copy of the temp string
	delete[] pBytes;
	return Bytes;
#else
	return c4_Bytes(wstr.c_str(), (length+1)*sizeof(WCHAR));
#endif
}

//
//Public methods.
//

//*****************************************************************************
CDbBase::CDbBase()
	: bPartialLoad(false)
	, pwczLastMessageText(NULL)
//Constructor.
{
	ASSERT(m_dbRefs.count(this) == 0);
	m_dbRefs.insert(this);
}

//*****************************************************************************
CDbBase::~CDbBase()
//Destructor.
{
	ASSERT(GetDbRefCount() != 0);
	ASSERT(m_dbRefs.count(this) != 0);
	m_dbRefs.erase(this);
	if (GetDbRefCount() == 0)
	{
		Close();
		ASSERT(!IsOpen());
	}

	//Release current char buffer.
	delete[] this->pwczLastMessageText;
}

//*****************************************************************************
c4_IntProp* CDbBase::GetPrimaryKeyProp(
//Returns: the primary key ID property for the indicated view type, or NULL if invalid view type.
//
//Params:
	const VIEWTYPE eViewType)  //(in) View/table
{
	//Determine prop for primary key based on table.
	switch (eViewType)
	{
		case V_Data: return &p_DataID;
		case V_Demos: return &p_DemoID;
		case V_Holds: return &p_HoldID;
		case V_Levels: return &p_LevelID;
		case V_MessageTexts: return &p_MessageTextID;
		case V_Players: return &p_PlayerID;
		case V_Rooms: return &p_RoomID;
		case V_SavedGames: return &p_SavedGameID;
		case V_SavedGameMoves: return &p_SavedGameID;
		case V_Speech: return &p_SpeechID;
		default:
			ASSERT(!"CDbBase::GetPrimaryKeyProp: Unexpected property type.");
		return NULL;
	}
}

//*****************************************************************************
UINT CDbBase::GetIncrementedID(
//Gets the next incremented ID value for a property and increments the value
//in the appropriate database.
//
//Params:
	const c4_IntProp &propID)     //(in) One of the properties stored in IncrementedIDs table.
//
//Returns:
//The next ID which should not be in use.
{
	ASSERT(IsOpen());

	c4_Storage *pStorage;
#ifdef DEV_BUILD
	pStorage = m_pMainStorage;
	CDbBase::DirtyData();
#else
	const int nPropID = propID.GetId();

	//Each type of incremented ID.
	static const int nDataID = p_DataID.GetId();
	static const int nDemoID = p_DemoID.GetId();
	static const int nHoldID = p_HoldID.GetId();
	static const int nLevelID = p_LevelID.GetId();
	static const int nMessageID = p_MessageID.GetId();
	static const int nMessageTextID = p_MessageTextID.GetId();
	static const int nPlayerID = p_PlayerID.GetId();
	static const int nRoomID = p_RoomID.GetId();
	static const int nSavedGameID = p_SavedGameID.GetId();
	static const int nSpeechID = p_SpeechID.GetId();

	if (nPropID == nDataID)
	{
		pStorage = m_pDataStorage;
		CDbBase::DirtyData();
	} else if (nPropID == nHoldID || nPropID == nLevelID || nPropID == nRoomID || nPropID == nSpeechID) {
		pStorage = m_pHoldStorage;
		CDbBase::DirtyHold();
	} else if (nPropID == nPlayerID) {
		pStorage = m_pPlayerStorage;
		CDbBase::DirtyPlayer();
	} else if (nPropID == nDemoID || nPropID == nSavedGameID) {
		pStorage = m_pSaveStorage;
		CDbBase::DirtySave();
	} else if (nPropID == nMessageID || nPropID == nMessageTextID) {
		pStorage = m_pTextStorage;
		CDbBase::DirtyText();
	} else {
		ASSERT(!"Unsupported ID");
		return 0;
	}
#endif

	c4_View IncrementedIDsView = pStorage->View("IncrementedIDs");
	c4_RowRef row = IncrementedIDsView.GetAt(0);
	UINT dwID = propID(row);

	//Set ID value in table to this ID that will now be considered in use.
	propID(row) = ++dwID;

	return dwID;
}

//*****************************************************************************
c4_RowRef CDbBase::GetRowRef(const VIEWTYPE vType, UINT dwGlobalIndex)
//Returns: a reference to a view row
//
//Hint: Call this method when iterating across all row indices.
//However, after a call to LookupRowByPrimaryKey(..., view),
//calling this is incorrect.  Instead, invoke view[returnedIndex] to retrieve
//the locally-indexed row from the proper view.
{
	const char* viewName = ViewTypeStr(vType);

	UINT localIndex = dwGlobalIndex;
	c4_View View = m_pMainStorage->View(viewName);
	const UINT size = View.GetSize(); //!!this count is too large if DEV_BUILD and emptyEndRows > 0,
	                                    //but this is only a problem if other .dat files are non-empty
	                                    //which should be avoided when building the main .dat file.
	if (localIndex < size)
		return View.GetAt(localIndex);
	localIndex -= size; //index not in this view -- use relative index in subsequent views

	//Look in other DB.
	c4_View localContentView = GetPlayerDataView(vType, viewName);
	return localContentView.GetAt(localIndex);
}

//*****************************************************************************
c4_ViewRef CDbBase::GetActiveView(const VIEWTYPE vType)
//Returns: the view of indicated type that may be written to
{
	const char* viewName = ViewTypeStr(vType);
#ifdef DEV_BUILD
	return m_pMainStorage->View(viewName);
#else
	return GetPlayerDataView(vType, viewName);
#endif
}

//*****************************************************************************
c4_ViewRef CDbBase::GetView(const VIEWTYPE vType, const UINT dwID)
//Returns: view reference from one of the databases.
{
	const char* viewName = ViewTypeStr(vType);
	if (dwID < START_LOCAL_ID)
		return m_pMainStorage->View(viewName); //pre-packaged database

	//The player's local content database.
	return GetPlayerDataView(vType, viewName);
}

c4_ViewRef CDbBase::GetPlayerDataView(const VIEWTYPE vType, const char* viewName)
{
#ifdef DEV_BUILD
	static c4_Storage noView; //not using any player view under this mode
	return noView.View(viewName);
#else
	switch (vType)
	{
		case V_Data:
			return m_pDataStorage->View(viewName);
		case V_Holds:
		case V_Levels:
		case V_Rooms:
		case V_Speech:
			return m_pHoldStorage->View(viewName);
		case V_Players:
			return m_pPlayerStorage->View(viewName);
		case V_Demos:
		case V_SavedGames:
			return m_pSaveStorage->View(viewName);
		case V_SavedGameMoves:
			return m_pSaveMoveStorage->View(viewName);
		case V_MessageTexts:
			return m_pTextStorage->View(viewName);
		default:
			ASSERT(!"Non-supported DB table");
			return m_pMainStorage->View(viewName); //gotta return something
	}
#endif
}

//*****************************************************************************
UINT CDbBase::GetViewSize(const VIEWTYPE vType)
//Returns: the number of rows in all DB views of the specified type.
{
	const char* viewName = ViewTypeStr(vType);

	c4_View view = m_pMainStorage->View(viewName);
	c4_View localContentView = GetPlayerDataView(vType, viewName);
	return view.GetSize() + localContentView.GetSize();
}

//*****************************************************************************
UINT CDbBase::globalRowToLocalRow(
//Translates a global row index to its local row index.
	const UINT globalRowIndex, const VIEWTYPE vType)
{
	const UINT baseIndexRowCount = GetView(vType,0).GetSize();
	if (globalRowIndex < baseIndexRowCount)
		return globalRowIndex; //in base view, global index is equivalent to local index
	return globalRowIndex - baseIndexRowCount; //subtract number of rows in first view
}

//*****************************************************************************
UINT CDbBase::LookupRowByPrimaryKey(const UINT dwID, const VIEWTYPE vType, c4_View &View)
//This overloaded method ensures all calls are routed through the global DB object
//that maintains special view info.
{
	return g_pTheDB->LookupRowByPrimaryKey(dwID, vType, View);
}

//*****************************************************************************
UINT CDbBase::LookupRowByPrimaryKey(
//Looks up a row by its primary key ID property.
//The view is deduced from ePropType.
//Assumes primary key property is in sequential order.
//
//Params:
	const UINT dwID,      //(in) Primary key value to match.
	const VIEWTYPE vType, //(in) View to examine
	const c4_IntProp* pPropID, //(in) Property that contains primary key.
	const UINT rowCount,  //(in) number of rows to scan
	                      //(may exclude empty rows allocated at the end of the view)
	c4_View &View)        //(out) View to look for row within.
//
//Returns:
//Row index, local to the outputted view, or ROW_NO_MATCH.
{
	//Get specific view where primary key field is located.
	View = GetView(vType, dwID);

	//Binary search for ID.
	ASSERT(rowCount < ROW_NO_MATCH);
	if (rowCount == 0)
		return ROW_NO_MATCH; //No rows to search.

	UINT dwFirstRowI = 0;
	UINT dwLastRowI = rowCount - 1;
	while (dwFirstRowI <= dwLastRowI) //Each iteration is one test at a new row position.
	{
		const UINT dwRowI = dwFirstRowI + (dwLastRowI - dwFirstRowI + 1) / 2;
		const UINT dwThisID = UINT((*pPropID)(View[dwRowI]));
		if (dwThisID == dwID)
			return dwRowI;
		if (dwThisID < dwID)
		{
			dwFirstRowI = dwRowI + 1;
		} else {
			dwLastRowI = dwRowI - 1;
			if (dwLastRowI == UINT(-1))
				return ROW_NO_MATCH; //Prevent looping err.
		}
	}

	return ROW_NO_MATCH;
}

//*****************************************************************************
bool CDbBase::IsOpen()
{
	if (!m_pMainStorage)
		return false;

#ifndef DEV_BUILD
	return m_pDataStorage != NULL && m_pHoldStorage != NULL && m_pPlayerStorage != NULL &&
			m_pSaveStorage != NULL && m_pSaveMoveStorage != NULL && m_pTextStorage != NULL;
#endif

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbBase::Open(
//Opens database files.
//
//Params:
	const WCHAR *pwszResFilepath) //(in)   Path to database resource files.  If NULL (default),
							//    then the application's resource path will be used.  Writable
							//    database files always use the application's data path.
//
//Returns:
//MID_Success or another message ID for failure.
{
	LOGCONTEXT("CDbBase::Open");
	MESSAGE_ID dwRet = MID_Success;

	try
	{
		//Close databases if already open.
		Close();
		ASSERT(!m_pMainStorage);
		ASSERT(!m_pDataStorage);
		ASSERT(!m_pHoldStorage);
		ASSERT(!m_pPlayerStorage);
		ASSERT(!m_pSaveStorage);
		ASSERT(!m_pSaveMoveStorage);
		ASSERT(!m_pTextStorage);

		//Concatenate paths to stock and player databases.
		CFiles Files;
		WSTRING wstrPath, wstrMainDatPath, wstrDataDatPath, wstrHoldDatPath,
				wstrPlayerDatPath, wstrSaveDatPath, wstrSaveMoveDatPath, wstrTextDatPath;
		wstrMainDatPath = (pwszResFilepath) ? pwszResFilepath : Files.GetResPath();
		wstrMainDatPath += wszSlash;
		wstrMainDatPath += CFiles::wGameName;
		wstrMainDatPath += wszDROD_VER;
		wstrMainDatPath += pwszDotDat;

		wstrPath = Files.GetDatPath();
		wstrPath += wszSlash;
		wstrDataDatPath = wstrPath + pwszData;
		wstrHoldDatPath = wstrPath + pwszHold;
		wstrPlayerDatPath = wstrPath + pwszPlayer;
		wstrSaveDatPath = wstrPath + pwszSave;
		wstrSaveMoveDatPath = wstrPath + pwszSaveMoves;
		wstrTextDatPath = wstrPath + pwszText;

		//Make sure the resource files exist.
		if (!CFiles::DoesFileExist(wstrMainDatPath.c_str()))
			return MID_DatMissing;

		//Create new, fresh data files if they don't already exist.
		if (!CFiles::DoesFileExist(wstrDataDatPath.c_str())
				&& !CreateDatabase(wstrDataDatPath.c_str(), START_LOCAL_ID))
			return MID_DatNoAccess;
		if (!CFiles::DoesFileExist(wstrHoldDatPath.c_str()))
		{
			if (!CreateDatabase(wstrHoldDatPath.c_str(), START_LOCAL_ID))
				return MID_DatNoAccess;
		}
		if (!CFiles::DoesFileExist(wstrPlayerDatPath.c_str()))
		{
			if (!CreateDatabase(wstrPlayerDatPath.c_str(), START_LOCAL_ID))
				return MID_DatNoAccess;
		}
		if (!CFiles::DoesFileExist(wstrSaveDatPath.c_str())
				&& !CreateDatabase(wstrSaveDatPath.c_str(), START_LOCAL_ID))
			return MID_DatNoAccess;
		if (!CFiles::DoesFileExist(wstrSaveMoveDatPath.c_str())
				&& !CreateDatabase(wstrSaveMoveDatPath.c_str(), START_LOCAL_ID))
			return MID_DatNoAccess;
		if (!CFiles::DoesFileExist(wstrTextDatPath.c_str())
				&& !CreateDatabase(wstrTextDatPath.c_str(), START_LOCAL_ID))
			return MID_DatNoAccess;

		//Verify read and write access.
#if defined(WIN32) && defined(DEV_BUILD)
		if (!CFiles::HasReadWriteAccess(wstrMainDatPath.c_str())
				&& !CFiles::MakeFileWritable(wstrMainDatPath.c_str()))
			return MID_DatNoAccess;
#endif
#if defined(WIN32) || !defined(DEV_BUILD)
		if (
			(!CFiles::HasReadWriteAccess(wstrDataDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrDataDatPath.c_str())) ||
			(!CFiles::HasReadWriteAccess(wstrHoldDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrHoldDatPath.c_str())) ||
			(!CFiles::HasReadWriteAccess(wstrPlayerDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrPlayerDatPath.c_str())) ||
			(!CFiles::HasReadWriteAccess(wstrSaveDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrSaveDatPath.c_str())) ||
			(!CFiles::HasReadWriteAccess(wstrSaveMoveDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrSaveMoveDatPath.c_str())) ||
			(!CFiles::HasReadWriteAccess(wstrTextDatPath.c_str())
			&& !CFiles::MakeFileWritable(wstrTextDatPath.c_str()))
		)
			return MID_DatNoAccess;
#endif

		//Open the databases.
		//Metakit has performance issues when working with Unicode filenames.
		//So, we will ignore the WCHAR filenames and open the DB the fast and memory efficient way.
		string filename = UnicodeToUTF8(wstrMainDatPath);
		int writeFlag =
#ifdef DEV_BUILD
		1; //save explicitly to this file only (when not creating another pack)
#else
		//Static dats MUST be opened read-only, otherwise things break horribly (i.e., implementation doesn't support this)
		0;  //0 = read-only
#endif
		m_pMainStorage = new c4_Storage(filename.c_str(), writeFlag);
		if (!m_pMainStorage)
			throw MID_CouldNotOpenDB;

#ifndef DEV_BUILD
		UnicodeToUTF8(wstrDataDatPath, filename);
		m_pDataStorage = new c4_Storage(filename.c_str(), 1);
		UnicodeToUTF8(wstrHoldDatPath, filename);
		m_pHoldStorage = new c4_Storage(filename.c_str(), 1);
		UnicodeToUTF8(wstrPlayerDatPath, filename);
		m_pPlayerStorage = new c4_Storage(filename.c_str(), 1);
		UnicodeToUTF8(wstrSaveDatPath, filename);
		m_pSaveStorage = new c4_Storage(filename.c_str(), 1);
		UnicodeToUTF8(wstrSaveMoveDatPath, filename);
		m_pSaveMoveStorage = new c4_Storage(filename.c_str(), 1);
		UnicodeToUTF8(wstrTextDatPath, filename);
		m_pTextStorage = new c4_Storage(filename.c_str(), 1);
		if (!m_pDataStorage || !m_pHoldStorage ||
				!m_pPlayerStorage || !m_pSaveStorage || !m_pSaveMoveStorage || !m_pTextStorage)
			throw MID_CouldNotOpenDB;
#endif

		buildIndex();
	}
	catch (MESSAGE_ID dwSetRetMessageID)
	{
		  dwRet = dwSetRetMessageID;
		  ResetStorage();
	}
	Undirty();
	return dwRet;
}

//*****************************************************************************
void CDbBase::Close(const bool bCommit) //[default=true]
//Closes database file.
{
	LOGCONTEXT("CDbBase::Close");
	if (IsOpen())
	{
		//Commit before closing.
		if (bCommit)
			Commit();

		ResetStorage();

		resetIndex();
	}
	Undirty();
}

//*****************************************************************************
void CDbBase::ResetStorage()
{
	//Close main database.
	delete m_pMainStorage; m_pMainStorage = NULL;

	//Close player databases.
	delete m_pDataStorage; m_pDataStorage = NULL;
	delete m_pHoldStorage; m_pHoldStorage = NULL;
	delete m_pPlayerStorage; m_pPlayerStorage = NULL;
	delete m_pSaveStorage; m_pSaveStorage = NULL;
	delete m_pSaveMoveStorage; m_pSaveMoveStorage = NULL;
	delete m_pTextStorage; m_pTextStorage = NULL;
}

//**************************************************************************************
bool CDbBase::CreateDatabase(const WSTRING& wstrFilepath, int initIncrementedIDs)
//Creates a new, blank database file with support for all record types.
//Returns: true if operation succeeded, else false
{
	const string filepath = UnicodeToUTF8(wstrFilepath);

	c4_Storage newStorage(filepath.c_str(), true); //create file on disk
	if (!CFiles::DoesFileExist(wstrFilepath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Was not able to create %S." NEWLINE, wstrFilepath.c_str());
#else
		const string path = UnicodeToUTF8(wstrFilepath);
		printf("FAILED--Was not able to create %s." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Add all views to database file.
	c4_View IncrementedIDs = newStorage.GetAs(INCREMENTEDIDS_VIEWDEF);
	c4_View Data = newStorage.GetAs(DATA_VIEWDEF);
	c4_View Demos = newStorage.GetAs(DEMOS_VIEWDEF);
	c4_View Holds = newStorage.GetAs(HOLDS_VIEWDEF);
	c4_View Levels = newStorage.GetAs(LEVELS_VIEWDEF);
	c4_View MessageTexts = newStorage.GetAs(MESSAGETEXTS_VIEWDEF);
	c4_View Players = newStorage.GetAs(PLAYERS_VIEWDEF);
	c4_View Rooms = newStorage.GetAs(ROOMS_VIEWDEF);
	c4_View SavedGames = newStorage.GetAs(SAVEDGAMES_VIEWDEF);
	c4_View SavedGameMoves = newStorage.GetAs(SAVEDGAMEMOVES_VIEWDEF);
	c4_View Speech = newStorage.GetAs(SPEECH_VIEWDEF);

	if (initIncrementedIDs >= 0)
	{
		IncrementedIDs.Add(
			p_DataID[        initIncrementedIDs ] +
			p_DemoID[        initIncrementedIDs ] +
			p_HoldID[        initIncrementedIDs ] +
			p_LevelID[       initIncrementedIDs ] +
			p_MessageID[     initIncrementedIDs ] +
			p_MessageTextID[ initIncrementedIDs ] +
			p_PlayerID[      initIncrementedIDs ] +
			p_RoomID[        initIncrementedIDs ] +
			p_SavedGameID[   initIncrementedIDs ] +
			p_SpeechID[      initIncrementedIDs ]);
	}

	newStorage.Commit();

	return true;
}

//*****************************************************************************
const WCHAR* CDbBase::GetMessageText(
//Get message text from database.  If the current language is set to something other than
//English an attempt to retrieve that text will be made before returning English text.
//
//Returns:
//Pointer to class buffer with message stored in it.
//Buffer is cleared after each call to GetMessageText.
//
//Params:
	const MESSAGE_ID  eMessageID,   //(in) Message ID identifying message to retrieve.
	UINT* pdwLen)        //(in) If not NULL (default), will be set to length of the message.
{
#ifdef PATCH
	//Hard-code message texts that won't be in the pre-packaged .dat file when patching an older version.
	//Add new message texts here as they are added in minor version bumps.
	string strText;
	switch (eMessageID)
	{
		//TODO 1.3: remove all of these when building barebones and release build dat files
		case MID_DoubleXP: strText = "Double REP"; break;
		case MID_DisableMouseMovement: strText = "Disable mouse movement"; break;
		case MID_EquipQueryStatus: strText = "Status"; break;
		case MID_RunOnCombat: strText = "Run on combat engagement"; break;
		case MID_PauseOnCombat: strText = "Pause on combat engagement"; break;
		case MID_OpenMoneyDoor: strText = "Greckle gate (open)"; break;
		case MID_NPCCustomTileSizeWarning: strText = "WARNING: Image is not a multiple of the room tile size and may not display correctly."; break;
		case MID_KnockOpenedDoor: strText = "Knock opened door"; break;
		case MID_DoorLocked: strText = "Player locked door"; break;
		case MID_ATKForFasterCombatWin: strText = "ATK for faster win"; break;
		case MID_NoFocusPlaysMusic: strText = "No focus plays music"; break;
		case MID_MoneyDoorOperated: strText = "Money door opened"; break;
		case MID_MoneyDoorLocked: strText = "Money door locked"; break;
		case MID_CaravelServerError: strText = "There was an error contacting the Caravel server."; break;
		case MID_BehaviorBeamBlock: strText = "Beam blocker"; break;
		case MID_VarLessThanOrEqual: strText = "<="; break;
		case MID_VarGreaterThanOrEqual: strText = ">="; break;
		case MID_VarInequal: strText = "!="; break;
		case MID_VarMyMonsterHPMult: strText = "_MyMonsterHPMult"; break;
		case MID_VarMyMonsterATKMult: strText = "_MyMonsterATKMult"; break;
		case MID_VarMyMonsterDEFMult: strText = "_MyMonsterDEFMult"; break;
		case MID_VarMyMonsterGRMult: strText = "_MyMonsterGRMult"; break;
		case MID_VarMyMonsterXPMult: strText = "_MyMonsterREPMult"; break;
		case MID_VarMyItemMult: strText = "_MyItemMult"; break;
		case MID_VarMyItemHPMult: strText = "_MyItemHPMult"; break;
		case MID_VarMyItemATKMult: strText = "_MyItemATKMult"; break;
		case MID_VarMyItemDEFMult: strText = "_MyItemDEFMult"; break;
		case MID_VarMyItemGRMult: strText = "_MyItemGRMult"; break;
		case MID_IfElseIf: strText = "Else If"; break;
		case MID_ReturnCommand: strText = "Return"; break;
		case MID_GoSub: strText = "GoSub"; break;
		case MID_PreviousIf: strText = "<Previous If>"; break;
		case MID_NextElseOrElseIfSkip: strText = "<Next Else or Else If (Skip Condition)>"; break;
		case MID_NPCDefeated: strText = "NPC defeated"; break;
		case MID_QuickPathNotAvailable: strText = "No known path is available."; break;
		case MID_Rain: strText = "Rain"; break;
		case MID_LevelMapDetail: strText = "Level Map (detailed)"; break;
		case MID_DrankPotion: strText = "Player drank speed/invisibility potion"; break;
		case MID_ReceivedATK: strText = "Received ATK"; break;
		case MID_ReceivedDEF: strText = "Received DEF"; break;
		case MID_ReceivedHP: strText = "Received HP"; break;
		case MID_HugeHealth: strText = "Huge health"; break;
		case MID_DefenseUp: strText = "Small Shield gem"; break;
		case MID_DefenseUp3: strText = "Medium Shield gem"; break;
		case MID_DefenseUp10: strText = "Large Shield gem"; break;
		case MID_AttackUp: strText = "Small Power gem"; break;
		case MID_AttackUp3: strText = "Medium Power gem"; break;
		case MID_AttackUp10: strText = "Large Power gem"; break;
		case MID_EachVictory: strText = "Each victory"; break;
		case MID_VarMudSpawn: strText = "_MudSpawn"; break;
		case MID_VarTarSpawn: strText = "_TarSpawn"; break;
		case MID_VarGelSpawn: strText = "_GelSpawn"; break;
		case MID_VarQueenSpawn: strText = "_QueenSpawn"; break;
		case MID_Crate: strText = "Crate"; break;
		case MID_RoomLocationText: strText = "Room location text"; break;
		case MID_FlashingMessage: strText = "Flashing message"; break;
		case MID_ScriptColor: strText = "Color(hex: rrggbb)"; break;
		case MID_SetMonsterVar: strText = "Set monster var"; break;
		case MID_To: strText = "to"; break;
		case MID_ValueOrExpression: strText = "Value or expression"; break;
		case MID_VarMonsterName: strText = "_MyName"; break;
		case MID_SaveGameSort: strText = "Sort by"; break;
		case MID_GameSortChronological: strText = "Time"; break;
		case MID_GameSortAlphabetical: strText = "Name"; break;
		case MID_SetMovementType: strText = "Set movement type"; break;
		case MID_Ground: strText = "Ground"; break;
		case MID_Air: strText = "Air"; break;
		case MID_SpawnEggs: strText = "Spawn Eggs"; break;
		case MID_VarMySpawn: strText = "_MySpawn"; break;
		case MID_Scorepoints: strText = "Scorepoints"; break;
		case MID_ListboxFilter: strText = "Filter"; break;
		case MID_ReplaceWithDefault: strText = "Replace with Default Script"; break;
		case MID_VarSetAt: strText = "Set var at"; break;
		default: break;
	}
	if (!strText.empty() && (Language::GetLanguage() == Language::English))
	{
		static WSTRING wstrText;
		UTF8ToUnicode(strText.c_str(), wstrText);
		if (pdwLen) *pdwLen = wstrText.length();
		return wstrText.c_str();
	}
#endif

	ASSERT(IsOpen());

	//Find message text.
	const UINT dwFoundRowI = FindMessageText(eMessageID);
	if (dwFoundRowI == ROW_NO_MATCH)
	{
		//Set last message to an empty string and return
		if (pdwLen) *pdwLen=0;
		return SetLastMessageText(wszEmpty, 0);
	}

	//Set last message to retrieved message and return.
	c4_Bytes MessageTextBytes = p_MessageText(GetRowRef(V_MessageTexts, dwFoundRowI));
	return GetMessageText(MessageTextBytes, pdwLen);
}

//*****************************************************************************
const WCHAR* CDbBase::GetMessageText(
//Params:
	const c4_Bytes& MessageTextBytes,
	UINT* pdwLen)       //(in) If not NULL (default), will be set to length of text.
//Returns:
//Pointer to class buffer with message stored in it.  Buffer is cleared after each call
//to GetMessageText.
{
	UINT dwMessageTextLen = (MessageTextBytes.Size() - 1) / 2;
	if (pdwLen) *pdwLen=dwMessageTextLen;

	//Which buffer to use?  For speed try to use smaller buffer to avoid allocs.
	static const UINT MAXLEN_SMALL_BUF = 500, MAXLEN_LARGE_BUF = 10000; //10k is unexpectedly large.
	static WCHAR wzSmallBuf[MAXLEN_SMALL_BUF + 1];
	WCHAR *pwzLargeBuf = NULL, *pwzUseBuf;
	if (dwMessageTextLen > MAXLEN_SMALL_BUF)
	{
		if (dwMessageTextLen < MAXLEN_LARGE_BUF)
		{
			 pwzUseBuf = pwzLargeBuf = new WCHAR[dwMessageTextLen + 1];
			 if (!pwzUseBuf)
			 {
				  ASSERT(!"Low memory condition.");
				  return NULL;
			 }
		}
		else //This is probably corrupted data, but I will try to show the first part of it.
		{
			 dwMessageTextLen = *pdwLen = MAXLEN_SMALL_BUF;
			 pwzUseBuf = wzSmallBuf;
		}
	}
	else
		pwzUseBuf = wzSmallBuf;

	memcpy( (void*)pwzUseBuf, (const void*)MessageTextBytes.Contents(), dwMessageTextLen * sizeof(WCHAR));
	WCv(pwzUseBuf[dwMessageTextLen]) = 0;

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
    LittleToBig(pwzUseBuf, dwMessageTextLen);
#endif
	WCHAR *pwzRet = SetLastMessageText(pwzUseBuf, dwMessageTextLen);
	delete[] pwzLargeBuf;
	return pwzRet;
}

//*****************************************************************************
WCHAR * CDbBase::GetAllocMessageText(
//Gets message text from database and copies to a return buffer allocated in this routine.
//If the current language is set to something other than English an attempt to retrieve
//that text will be made before returning English text.
//
//Params:
	MESSAGE_ID eMessageID,   //(in) Message ID identifying message to retrieve.
	UINT* pdwLen)        //(in) If not NULL (default), will be set to length of
								//message.
//
//Returns:
//Pointer to new buffer with message stored in it that caller must delete.
const
{
	ASSERT(IsOpen());

	//Find message text.
	UINT dwFoundRowI = FindMessageText(eMessageID);

	WCHAR *pwczRet = NULL;
	if (dwFoundRowI == ROW_NO_MATCH)
	{
		//Set new buffer to empty string and return
		if (pdwLen) *pdwLen=0;
		pwczRet = new WCHAR[1];
		WCv(pwczRet[0]) = '\0';
	}
	else
	{
		//Copy retrieved message to new buffer and return.
		c4_Bytes MessageTextBytes = p_MessageText(GetRowRef(V_MessageTexts, dwFoundRowI));
		UINT dwMessageTextLen = (MessageTextBytes.Size() - 1) / 2;
		if (pdwLen) *pdwLen=dwMessageTextLen;
		pwczRet = new WCHAR[dwMessageTextLen+1];
		if (!pwczRet) return NULL;
		WCScpy(pwczRet, (const WCHAR *) (MessageTextBytes.Contents()));
	}
	return pwczRet;
}

//*****************************************************************************
MESSAGE_ID CDbBase::AddMessageText(
//Writes a message to database.
//
//Params:
	const WCHAR *pwczMessage)     //(in) Message to write in Unicode.
{
	ASSERT(IsOpen());
	const MESSAGE_ID eMessageID = static_cast<MESSAGE_ID>(GetIncrementedID(p_MessageID));
	AddMessageText(static_cast<UINT>(eMessageID), pwczMessage);
	return eMessageID;
}

//*****************************************************************************
MESSAGE_ID CDbBase::AddMessageText(
//Writes a message to database.
//Call this method explicitly with an existing messageID and different language
//code to add texts for a new language.
//
//Params:
	const UINT eMessageID,    //(in) MessageID to use
	const WCHAR *pwczMessage)  //(in) Message to write in Unicode.
//
//Returns:
//MessageID of new MessageTexts record.
{
	LOGCONTEXT("CDbBase::AddMessageText");
	ASSERT(IsOpen());

	const UINT dwMessageLen = WCSlen(pwczMessage);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(sizeof(WCHAR)==2);
	char *pBytes = new char[(dwMessageLen + 1) * sizeof(WCHAR)];
	memcpy(pBytes, pwczMessage, (dwMessageLen + 1) * sizeof(WCHAR));
	LittleToBig(reinterpret_cast<WCHAR*>(pBytes), dwMessageLen);
#else
	const unsigned char *pBytes = reinterpret_cast<const unsigned char*>(pwczMessage);
#endif
	c4_Bytes MessageBytes(pBytes, (dwMessageLen + 1)*sizeof(WCHAR));

	//Add message record.
	c4_Row newRow;
	const UINT dwMessageTextID = GetIncrementedID(p_MessageTextID); //calls DirtyText
	p_MessageTextID(newRow) = dwMessageTextID;
	p_MessageID(newRow) = static_cast<UINT>(eMessageID);
	p_LanguageCode(newRow) = static_cast<UINT>(Language::GetLanguage());
	p_MessageText(newRow) = MessageBytes;

	c4_View MessageTextsView = GetView(V_MessageTexts, dwMessageTextID);
	MessageTextsView.Add(newRow);

	//Determine global row index where message text is added.
	c4_View view;
	UINT rowIndex = LookupRowByPrimaryKey(dwMessageTextID, V_MessageTexts, view);
	ASSERT(rowIndex != ROW_NO_MATCH);
	//If ID is in the first view, the local ID is equivalent to the global ID.
	//Otherwise, add the size of the first view to get the ID's global index.
	if (dwMessageTextID >= START_LOCAL_ID)
		rowIndex += GetView(V_MessageTexts,0).GetSize();

	addMessage(eMessageID, rowIndex);

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	delete[] pBytes;
#endif

	return eMessageID;
}

//*****************************************************************************
MESSAGE_ID CDbBase::ChangeMessageText(
//Changes text of a message in database.  If message text exists, but not in the current language
//a separate message text for the current message is added.
//
//Returns:
//MessageID of the MessageTexts record.
//
//Params:
	const MESSAGE_ID eMessageID,  //(in)   MessageID of message to change.  Current
														//    language will be used to look up text.
	const WCHAR* pwczMessage)     //(in)   Message to write in Unicode.
{
	ASSERT(eMessageID > (MESSAGE_ID)0);
	ASSERT(IsOpen());

	//Find the message text record.
	const UINT dwRowI = FindMessageText(eMessageID);
	if (dwRowI==ROW_NO_MATCH)	//missing/corrupted ID, but have to be robust to it
		return AddMessageText(pwczMessage);

	//If message text record is not for the current language then add a new message text
	//for the current language.  The old message text will remain the same.
	const Language::LANGUAGE eLanguage = Language::GetLanguage();
	c4_RowRef row = GetRowRef(V_MessageTexts, dwRowI);
	if ((UINT)p_LanguageCode(row) != eLanguage)
		return AddMessageText(eMessageID, pwczMessage);

	//Because writes are expensive, check for an existing value that already
	//matches new value.
	const UINT dwMessageLen = WCSlen(pwczMessage);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(sizeof(WCHAR)==2);
	char *pBytes = new char[(dwMessageLen + 1) * sizeof(WCHAR)];
	memcpy(pBytes, pwczMessage, (dwMessageLen + 1) * sizeof(WCHAR));
	LittleToBig(reinterpret_cast<WCHAR*>(pBytes), dwMessageLen);
#else
	const unsigned char *pBytes = reinterpret_cast<const unsigned char*>(pwczMessage);
#endif
	{
		c4_Bytes OriginalMessageBytes = p_MessageText(row);
		if (dwMessageLen == (OriginalMessageBytes.Size()/sizeof(WCHAR) - 1) &&
				!memcmp(pBytes, (const WCHAR *) (OriginalMessageBytes.Contents()), dwMessageLen * sizeof(WCHAR)))
		{
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
			delete[] pBytes;
#endif
			return eMessageID; //New message is same as current, so nothing to do.
		}
	}

	//Update message record.
	c4_Bytes MessageBytes(pBytes, (dwMessageLen + 1)*sizeof(WCHAR));
	p_MessageText(row) = MessageBytes;
	CDbBase::DirtyText();

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	delete[] pBytes;
#endif
	return eMessageID;
}

//*****************************************************************************
void CDbBase::DeleteMessage(
//Deletes all message texts (different languages) for one message ID.
//
//Params:
	const MESSAGE_ID eMessageID)  //(in)   MessageID of message text(s) to delete.
{
	//Mark this messageID for deletion, but wait to delete it until a DB commit
	//in order to reduce time rebuilding the row lookup index.
	ASSERT(!messageIDsMarkedForDeletion.has(eMessageID));
	messageIDsMarkedForDeletion += eMessageID;
	DirtyText();
}

//*****************************************************************************
void CDbBase::DeleteMarkedMessages()
//Deletes all messages marked for deletion and resynchs the row lookup index.
{
	if (messageIDsMarkedForDeletion.empty())
		return; //nothing to do

	ASSERT(CDbBase::bDirtyText);

	//Assume: all message texts being deleted are from the same view
	//(i.e. the same .dat file).
	c4_View MessageTextsView = GetView(V_MessageTexts,
#ifdef DEV_BUILD
		0
#else
		START_LOCAL_ID
#endif
	);

	//Compile set of all messageText rows to be deleted.
	CIDSet messageRows;
	for (CIDSet::const_iterator messageID = messageIDsMarkedForDeletion.begin();
			messageID != messageIDsMarkedForDeletion.end(); ++messageID)
		messageRows += getMessageRows(*messageID);

	//Each iteration deletes one messageText row.
	//Delete in descending order to keep lower row indices correct.
	for (CIDSet::const_reverse_iterator messageRow = messageRows.rbegin();
			messageRow != messageRows.rend(); ++messageRow)
	{
		ASSERT(*messageRow != ROW_NO_MATCH);
		ASSERT(messageIDsMarkedForDeletion.has(
				UINT(p_MessageID(GetRowRef(V_MessageTexts, *messageRow)))));

		const UINT localRowIndex = globalRowToLocalRow(*messageRow, V_MessageTexts);
		MessageTextsView.RemoveAt(localRowIndex);
	}

	//Resynch row index lookup.
	deleteMessages(messageIDsMarkedForDeletion, messageRows);
	messageIDsMarkedForDeletion.clear();
}

//*****************************************************************************
void CDbBase::ExportTexts(const WCHAR *pFilename)
//Exports all message texts in base DB to file.
{
	CStretchyBuffer buf;
	c4_View MessageTextsView = GetView(V_MessageTexts, 0);
	ExportTexts(MessageTextsView, buf);
	if (!buf.empty())
		CFiles::WriteBufferToFile(pFilename, buf);
}

//*****************************************************************************
void CDbBase::ExportTexts(c4_View& MessageTextsView, CStretchyBuffer& buf)
//Exports all message texts of current language in view to 'buf'.
{
	char temp[10];
	WCHAR wzBuf[10000];
	BYTE* pbOutStr = NULL;

	const UINT language = Language::GetLanguage();
	UINT messageTextCount = MessageTextsView.GetSize();
	for (UINT i=0; i<messageTextCount; ++i)
	{
		c4_RowRef row = MessageTextsView[i];
		if ((UINT)p_LanguageCode(row) == language)
		{
			//Skip hold texts in the DB.
			const UINT messageID = p_MessageID(row);
			if (messageID >= 5000) //first non-stock messageTextID
				continue;
			c4_Bytes MessageTextBytes = p_MessageText(row);

			//ID reference header.
			buf += "[[";
			buf += _itoa(messageID, temp, 10);
			buf += "]]" NEWLINE;

			//Message text.
			const UINT messageTextLen = (MessageTextBytes.Size() - 1) / 2;
			memcpy( (void*)wzBuf, (const void*)MessageTextBytes.Contents(), messageTextLen * sizeof(WCHAR));
			WCv(wzBuf[messageTextLen]) = 0;
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
			LittleToBig(wzBuf, messageTextLen);
#endif
			const UINT wSize = to_utf8(wzBuf, pbOutStr);
			if (wSize)
				buf.Append(pbOutStr, wSize);
			delete[] pbOutStr;
			pbOutStr = NULL;
			buf += NEWLINE;
		}
	}
}

//*****************************************************************************
bool CDbBase::ImportTexts(const WCHAR *pFilename)
//Sets all message texts in base DB from info contained in file.
{
	CStretchyBuffer buf;
	CFiles::ReadFileIntoBuffer(pFilename, buf, true);
	c4_View MessageTextsView = GetView(V_MessageTexts, 0);
	return ImportTexts(MessageTextsView, buf);
}

//*****************************************************************************
bool CDbBase::ImportTexts(c4_View& /*MessageTextsView*/, CStretchyBuffer& buf)
//Sets all message texts of current language in view from info contained in 'buf'.
{
	WSTRING wstr;
	UTF8ToUnicode((char*)(BYTE*)buf, buf.Size(), wstr);
	const UINT wstrSize = wstr.size();

	static const WCHAR IDMARKER[] = { We('['), We('['), We(0) };
	static const WCHAR ENDMARKER[] = { We(']'), We(']'), We(0) };
	UINT messageID;
	WSTRING::size_type index, endTextIndex, nextIndex = wstr.find(IDMARKER, 0); //start past first marker
	while (nextIndex != WSTRING::npos)
	{
		index = nextIndex + 2; //skip past marker

		//Parse message ID.
		WSTRING num = wstr.substr(index, 12); //copy enough chars to get the whole number
		messageID = _Wtoi(num.c_str());
		index = wstr.find(ENDMARKER, index); //skip past end marker
		if (index == WSTRING::npos)
			break;
		index += 2;

		//Skip whitespace.
		while (index < wstrSize && iswspace(wstr[index]))
			++index;

		//Find next marker.
		nextIndex = wstr.find(IDMARKER, index);
		endTextIndex = (nextIndex == WSTRING::npos ? wstrSize - 1 : nextIndex - 1);

		//Find end of message text before this marker(i.e., ignore trailing whitespace).
		while (iswspace(wstr[endTextIndex]))
			--endTextIndex;

		if (endTextIndex >= index)
		{
			const WSTRING text = wstr.substr(index, endTextIndex-index+1);
			if (messageID)
				ChangeMessageText(messageID, text.c_str());
		}
	}

	//Save to disk.
	m_pMainStorage->Commit();
	return true;
}

//*****************************************************************************
MESSAGE_ID CDbBase::SetProperty(
	const PROPTYPE /*pType*/,
	const char** /*atts*/,
	CImportInfo &/*info*/)
{
	return MID_NoText;
}

//*****************************************************************************
MESSAGE_ID CDbBase::SetProperty(
//NOTE: Unused parameter names commented out to suppress warnings
	const PROPTYPE /*pType*/,
	char* const /*str*/,
	CImportInfo &/*Maps*/,
	bool &/*bSaveRecord*/)
{
	return MID_NoText;
}

//*****************************************************************************
MESSAGE_ID CDbBase::SetProperty(
//NOTE: Unused parameter names commented out to suppress warnings
	const VIEWPROPTYPE /*vpType*/,
	const PROPTYPE /*pType*/,
	char* const /*str*/,
	CImportInfo &/*Maps*/)
{
	return MID_NoText;
}

//
//Protected methods.
//

//*****************************************************************************
UINT CDbBase::FindMessageText(
//Finds a message text row that matches a message ID and current language.
//
//Params:
	const MESSAGE_ID eMessageID,      //(in) Message ID to look for.
	const bool bReturnOtherLanguages) //[default=true]
//
//Returns:
//Global index of the row in MessageTextsView to use.
//ROW_NO_MATCH if no matching message ID was found.
const
{
	const Language::LANGUAGE eLanguage = Language::GetLanguage();
	UINT dwEnglishRowI = ROW_NO_MATCH, dwFoundRowI = ROW_NO_MATCH;

	//Get rows in which this messageID is located.
	CIDSet messageRows = getMessageRows(eMessageID);

	//Each iteration scans one of these rows for a language match.
	for (CIDSet::const_iterator messageRow = messageRows.begin();
			messageRow != messageRows.end(); ++messageRow)
	{
		c4_RowRef row = GetRowRef(V_MessageTexts, *messageRow);
		ASSERT(eMessageID == UINT(p_MessageID(row)));

		const Language::LANGUAGE eSeekLanguageCode = (Language::LANGUAGE)(p_LanguageCode(row));
		if (eSeekLanguageCode == eLanguage)
			return *messageRow; //Found message+language match.

		//Found right message, but wrong language.
		if (bReturnOtherLanguages)
		{
			dwFoundRowI = *messageRow;
			if (eSeekLanguageCode == Language::English)
				dwEnglishRowI = dwFoundRowI;
		}
	}

	//No message/language match.  If found an message/English match then return that.
	if (dwEnglishRowI != ROW_NO_MATCH)
		return dwEnglishRowI;

	//Otherwise return a match found in any language.
	return dwFoundRowI;
}

//*****************************************************************************
CCoordSet CDbBase::GetMessageTextIDs(
//Gets all message text rows that match a message ID.
//
//Params:
	const MESSAGE_ID eMessageID)      //(in) Message ID to look for.
//
//Returns:
//Set of rows+languages in MessageTextsView matching this messageID,
//or empty set if no matching messageIDs were found.
const
{
	CCoordSet ids;

	//Get rows in which this messageID is located.
	CIDSet messageRows = getMessageRows(eMessageID);

	//Each iteration scans one of these rows for a language match.
	for (CIDSet::const_iterator messageRow = messageRows.begin();
			messageRow != messageRows.end(); ++messageRow)
	{
		c4_RowRef row = GetRowRef(V_MessageTexts, *messageRow);
		ASSERT(eMessageID == UINT(p_MessageID(row)));

		ids.insert(*messageRow, int(p_LanguageCode(row)));
	}

	return ids;
}

//
// Private methods.
//

//*****************************************************************************
bool CDbBase::IsDirty()
//Returns: whether updated data in the DB need to be committed
{
	return CDbBase::bDirtyData || CDbBase::bDirtyHold || CDbBase::bDirtyPlayer ||
			CDbBase::bDirtySave || CDbBase::bDirtyText;
}

//*****************************************************************************
void CDbBase::Commit()
//Commits changes in all the databases.
{
	LOGCONTEXT("CDbBase::Commit");
	ASSERT(IsOpen());
	if (IsDirty())
	{
		DeleteMarkedMessages();
#ifdef DEV_BUILD
		ASSERT(m_pMainStorage);
		m_pMainStorage->Commit();
#else
		if (CDbBase::bDirtyData)
			m_pDataStorage->Commit();
		if (CDbBase::bDirtyHold)
			m_pHoldStorage->Commit();
		if (CDbBase::bDirtyPlayer)
			m_pPlayerStorage->Commit();
		if (CDbBase::bDirtySave)
		{
			m_pSaveStorage->Commit();
			m_pSaveMoveStorage->Commit();
		}
		if (CDbBase::bDirtyText)
			m_pTextStorage->Commit();
#endif
		Undirty();
	}
}

//*****************************************************************************
void CDbBase::Rollback()
//Rolls back changes in all the databases.
{
	LOGCONTEXT("CDbBase::Rollback");
	ASSERT(IsOpen());
	if (IsDirty())
	{
		messageIDsMarkedForDeletion.clear();
#ifdef DEV_BUILD
		m_pMainStorage->Rollback();
#else
		m_pDataStorage->Rollback();
		m_pHoldStorage->Rollback();
		m_pPlayerStorage->Rollback();
		m_pSaveStorage->Rollback();
		m_pSaveMoveStorage->Rollback();
		m_pTextStorage->Rollback();
#endif
		Undirty();

		//Rebuild acceleration index.
		resetIndex();
		buildIndex();
	}
}

//*****************************************************************************
void CDbBase::Undirty()
//Resets all DB dirty bits.
{
	CDbBase::bDirtyData = CDbBase::bDirtyHold = CDbBase::bDirtyPlayer =
			CDbBase::bDirtySave = CDbBase::bDirtyText = false;
}

//*****************************************************************************
WCHAR* CDbBase::SetLastMessageText(
//Sets class char buffer to contain specified text.
//
//Params:
	const WCHAR *pwczNewMessageText, //(in) New text to copy to char buffer.
	const UINT dwNewMessageTextLen)       //(in) Chars in message text.
//
//Returns:
//Point to char buffer.
{
	ASSERT(dwNewMessageTextLen == WCSlen(pwczNewMessageText));

	//Release current char buffer.
	delete[] this->pwczLastMessageText;
	this->pwczLastMessageText = NULL;

	//Allocate new char buffer.
	this->pwczLastMessageText = new WCHAR[dwNewMessageTextLen + 1];
	if (!this->pwczLastMessageText) return NULL;

	//Copy new text to char buffer.
	WCScpy(this->pwczLastMessageText, pwczNewMessageText);

	//Return char buffer pointer.
	return this->pwczLastMessageText;
}

//*****************************************************************************
void CDbBase::addMessage(const UINT messageID, const UINT messageRow)
//Adds a messageID to the index.
//A messageID may have more than one row (one per language variant).
{
	DirtyText();
	messageIDsMap::iterator messages = messageIndex.find(messageID);
	if (messages == messageIndex.end())
		messageIndex[messageID] = CIDSet(messageRow);
	else
		messages->second += messageRow;
}

//*****************************************************************************
void CDbBase::deleteMessages(const CIDSet& messageIDs, const CIDSet& rowIDs)
//Deletes all rows for each messageID from the index.
{
	ASSERT(!messageIDs.empty());
	ASSERT(!rowIDs.empty());

	//Remove entries for deleted messageIDs.
	for (CIDSet::const_iterator messageID = messageIDs.begin();
			messageID != messageIDs.end(); ++messageID)
	{
		messageIDsMap::iterator message = messageIndex.find(*messageID);
		if (message != messageIndex.end())
			messageIndex.erase(message);
		else
			ASSERT(!"Missing messageID");
	}

	//Resynch row values in lookup index.
	//This can be done without requerying the DB because the number of rows by
	//which each messageID has been offset by deletions of earlier rows
	//may be tallied.

	//Each iteration processes one messageID.
	for (messageIDsMap::iterator message = messageIndex.begin();
			message != messageIndex.end(); ++message)
	{
		//Correct row index for each messageTextID belonging to this messageID.
		CIDSet& ids = message->second;
		CIDSet::iterator messageTextRow = ids.begin();
		while (messageTextRow != ids.end())
		{
			//Tally how many rows before this row have been deleted.
			const UINT oldRow = *messageTextRow;
			UINT rowOffset = 0;
			for (CIDSet::const_iterator rowIDIter = rowIDs.begin();
					rowIDIter != rowIDs.end(); ++rowIDIter, ++rowOffset)
			{
				if (*rowIDIter > oldRow)
					break; //no more rows before the old row have been deleted
			}

			if (rowOffset)
			{
				//We can replace a value in this set with a lower value this way.
				messageTextRow = ids.erase(messageTextRow); //increments iterator
				ASSERT(oldRow >= rowOffset);
				ASSERT(!ids.has(oldRow - rowOffset)); //this row index shouldn't already be present in the set
				ids += (oldRow - rowOffset);
			} else {
				++messageTextRow; //this messageText's row index has not changed
			}
		}
	}
}

//*****************************************************************************
CIDSet CDbBase::getMessageRows(const UINT messageID)
//Returns: global rows in messageTexts views where this messageID is stored
//There will be a row for each language variant of this messageID.
{
	messageIDsMap::iterator messages = messageIndex.find(messageID);
	if (messages == messageIndex.end())
		return CIDSet(); //no entry
	return CIDSet(messages->second);
}

//*****************************************************************************
void CDbBase::resetIndex()
{
	messageIndex.clear();
}

//*****************************************************************************
void CDbBase::buildIndex()
{
	//Build message row index.
	const UINT messageCount = GetViewSize(V_MessageTexts);
	for (UINT messageI = 0; messageI < messageCount; ++messageI)
	{
		c4_RowRef row = GetRowRef(V_MessageTexts, messageI);
		addMessage(UINT(p_MessageID(row)), messageI);
	}
}

