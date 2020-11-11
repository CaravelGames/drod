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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WIN32 //Many things will not compile w/o WIN32 API.  Fix them if you are porting.
#  include <windows.h> //Should be first include.
#  pragma warning(disable:4786)
#endif

#include "Util3_0.h"
#include "GameScreenDummy.h"	//to avoid a bunch of other includes from DROD
#include "../DROD/EditRoomWidget.h"
#include "../DROD/MapWidget.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbProps.h"
#include "../DRODLib/DbMessageText.h"
#include "../DRODLib/GameConstants.h"
#include "../Texts/MIDs.h"
#include <FrontEndLib/Screen.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Date.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/GameStream.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <unistd.h> //unlink
#include <dirent.h> //opendir, readdir, closedir
#endif

const UINT MAXLEN_NAMETAG = 200;

const UINT uFirstNonSimpleMessageID = 5000;

//**************************************************************************************
bool CUtil3_0::PrintDelete(
//Deletes DROD data.
//
//Params:
	const COptionList &/*Options*/)  //(in) Reserved for future use.
//
//Returns:
//True if successful, false if not.
const
{
	//Get paths to data storage files.
	WSTRING wstrMasterDatFilepath;
	GetMasterFilepath(wstrMasterDatFilepath);

	//Make sure at least one of the files exists.
	if (!CFiles::DoesFileExist(wstrMasterDatFilepath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Couldn't find data to delete at %S." NEWLINE, this->strPath.c_str());
#else
		const string path = UnicodeToUTF8(strPath);
		printf("FAILED--Couldn't find data to delete at %s." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Delete official dat.
	bool bDeleteSuccess = !CFiles::DoesFileExist(wstrMasterDatFilepath.c_str()) ||
		DeleteDat(wstrMasterDatFilepath.c_str());
#ifdef __linux__
	if (!bDeleteSuccess)
	{
		const string strHoldFilepath = UnicodeToUTF8(wstrMasterDatFilepath);
		printf("Couldn't delete %s.\n", strHoldFilepath.c_str());
	}
#else
	if (!bDeleteSuccess) printf("Couldn't delete %S." NEWLINE,wstrMasterDatFilepath.c_str());
#endif

	if (!bDeleteSuccess)
	{
		printf("FAILED--Couldn't delete all of the files.  Possibly an access problem.  "
				"Make sure DROD isn't running." NEWLINE);
		return false;
	}

	//Success.
	return true;
}

//**************************************************************************************
bool CUtil3_0::PrintCreate(
//Create storage files for 3.0 data.
//
//Params:
	const COptionList &/*Options*/)  //(in) Reserved for future use.
//
//Returns:
//True if successful, false if not.
const
{
	//Check for valid dest path.
	if (!IsPathValid(this->strPath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Destination path (%S) is not a valid path." NEWLINE, this->strPath.c_str());
#else
		const string path = UnicodeToUTF8(strPath);
		printf("FAILED--Destination path (%s) is not a valid path." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Get paths to data storage files.
	WSTRING wstrMasterDatFilepath;
	GetMasterFilepath(wstrMasterDatFilepath);

	//If any of the data storage files already exit, then exit.
	if (DoesFileExist(wstrMasterDatFilepath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--data already exists at %S." NEWLINE, this->strPath.c_str());
#else
		const string path = UnicodeToUTF8(strPath);
		printf("FAILED--data already exists at %s." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Create the database(s).
	if (!CDbBase::CreateDatabase(wstrMasterDatFilepath))
		return false;

	//Success.
	return true;
}

//**************************************************************************************
bool CUtil3_0::PrintImport(
//Imports DROD data from a specified location/version.
//
//Params:
	const COptionList &Options,   //(in)   Options affecting the import.--IGNORED
	const WCHAR* pszSrcPath,      //(in)   Path to find DROD data for import.
	VERSION eSrcVersion)        //(in)  Version of data to locate.--IGNORED
//
//Returns:
//True if successful, false if not.
const
{
  //Get options.
  static const WCHAR wt[] = {{'t'},{0}};
  bool bImportMIDs = Options.GetSize()==0 || Options.Exists(wt);

	//Make sure version path is valid.
	if (!IsPathValid(this->strPath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Destination path (%S) is not a valid path." NEWLINE, this->strPath.c_str());
#else
		const string path = UnicodeToUTF8(strPath);
		printf("FAILED--Destination path (%s) is not a valid path." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Make sure source path is valid.
	if (!IsPathValid(pszSrcPath))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Source path (%S) is not a valid path." NEWLINE, pszSrcPath);
#else
		const string path = UnicodeToUTF8(pszSrcPath);
		printf("FAILED--Source path (%s) is not a valid path." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Check for presence of destination dat files.
	WSTRING wstrMasterDatFilepath;
	GetMasterFilepath(wstrMasterDatFilepath);
	if (!DoesFileExist(wstrMasterDatFilepath.c_str()))
	{
#ifdef HAS_UNICODE
		printf("FAILED--Storage files not found at %S." NEWLINE, this->strPath.c_str());
#else
		const string path = UnicodeToUTF8(strPath);
		printf("FAILED--Storage files not found at %s." NEWLINE, path.c_str());
#endif
		return false;
	}

	//Populate hold database with messages needed and maybe basic text.
	{
		//Open database.
		const string filename = UnicodeToUTF8(wstrMasterDatFilepath.c_str());
		printf("Opening %s...", filename.c_str());
		c4_Storage HoldStorage(filename.c_str(), true);
		printf("done." NEWLINE);

		if (bImportMIDs)
		{
			//Ensure all incremented IDs have been created at this time.
			c4_View HoldIncrementedIDs = HoldStorage.View("IncrementedIDs");
			if (HoldIncrementedIDs.GetSize() < 1)
			{
				HoldIncrementedIDs.Add(
					p_DataID[        0 ] +
					p_DemoID[        0 ] +
					p_HoldID[        0 ] +
					p_LevelID[       0 ] +
					p_MessageID[     0 ] +
					p_MessageTextID[ 0 ] +
					p_PlayerID[      0 ] +
					p_RoomID[        0 ] +
					p_SavedGameID[   0 ] +
					p_SpeechID[      0 ]);
				//HoldStorage.Commit(); //performed below
			}

			printf("Importing basic messages...");
			if (!ImportBasicMessages(pszSrcPath, HoldStorage))
			{
				printf("FAILED." NEWLINE);
				return false;
			}
			printf("done." NEWLINE);
			HoldStorage.Commit();
		}
	}

	//Success.
	return true;
}

//**************************************************************************************
bool CUtil3_0::PrintRoom(const COptionList &Options, UINT dwRoomID) const
//Currently, all this does for a room(s) is save a snapshot image to disk.
{
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
		return false;

	CDb db;
	if (!db.IsOpen())
	{
		if (db.Open(this->strPath.c_str()) != MID_Success)
			return false;
	}

	{
	//Set up global DROD vars needed to perform room drawing operations.
	g_pTheDB = &db;

	ASSERT(!g_pTheBM);
	g_pTheBM = g_pTheDBM = new CDrodBitmapManager();
	g_pTheBM->BITS_PER_PIXEL = 24;
	g_pTheBM->BYTES_PER_PIXEL = g_pTheBM->BITS_PER_PIXEL / 8;
	MESSAGE_ID ret = (MESSAGE_ID)g_pTheDBM->Init();
	if (ret) goto Failure;

	//Get all rooms to be processed.
	static const WCHAR wH[] = {{'h'},{0}};
	OPTIONNODE *pOpNode = Options.Get(wH);
	const UINT dwHoldID = pOpNode ? _Wtoi(pOpNode->szAttributes) : 0;
	HoldStats roomStats;
	if (dwHoldID)
		db.Holds.GetRooms(dwHoldID, roomStats);	//get all rooms in hold
	else
		roomStats.rooms += dwRoomID;	//process only one room

	//Prepare room widget.
	static const UINT CX_TILE = 22, CY_TILE = 22;
	static const UINT ROOM_COLS = 38, ROOM_ROWS = 32;	//assume all rooms are same size
	static CEditRoomWidget *pRoomWidget = NULL;
	if (!pRoomWidget) //only create one instance for all calls
		pRoomWidget = new CEditRoomWidget(0, 0, 0, ROOM_COLS * CX_TILE,
				ROOM_ROWS * CY_TILE);
	ASSERT(pRoomWidget);

	SDL_Surface *pRoomSurface = SDL_CreateRGBSurface(
			SDL_SWSURFACE, pRoomWidget->GetW(), pRoomWidget->GetH(),
			g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0);
	if (!pRoomSurface) goto Failure;

	//To save only the room.
	SDL_Rect screenRect = {0, 0, (int)pRoomWidget->GetW(), (int)pRoomWidget->GetH()};
	SDL_Rect roomRect = {0, 0, (int)pRoomWidget->GetW(), (int)pRoomWidget->GetH()};
	pRoomWidget->SetAnimateMoves(false);
	vector<CMoveCoord*> LevelEntrances;

	//Render each room and save to disk.
	for (CIDSet::const_iterator room = roomStats.rooms.begin(); room != roomStats.rooms.end(); ++room)
	{
		CDbRoom *pRoom = db.Rooms.GetByID(*room);
		if (!pRoom) continue;	//no room with this ID
		ASSERT(pRoom->wRoomCols == ROOM_COLS);
		ASSERT(pRoom->wRoomRows == ROOM_ROWS);
		pRoomWidget->LoadFromRoom(pRoom, NULL, &LevelEntrances);
		pRoomWidget->RenderRoom();
		pRoomWidget->DrawMonsters(pRoom->pFirstMonster, pRoomWidget->pRoomSnapshotSurface, false);
		SDL_BlitSurface(pRoomWidget->pRoomSnapshotSurface, &screenRect, pRoomSurface, &roomRect);

		static const WCHAR wszRoom[] = {{'r'},{'o'},{'o'},{'m'},{0}};
		WCHAR temp[10];
		WSTRING fileName = wszRoom;
		fileName += _itoW(*room, temp, 10);
		CScreen::SaveSnapshot(pRoomSurface, fileName);
		delete pRoom;
		printf(".");
	}

	//Uninit room widget.
	//	delete pRoomWidget;	//can't delete -- but causes memory leak
	pRoomWidget->ResetRoom();
	SDL_FreeSurface(pRoomSurface);

	//Uninit all the global DROD stuff.
	g_pTheDB = NULL;

	delete g_pTheDBM;
	g_pTheDBM = NULL;
	SDL_Quit();

	return true;
	}

Failure:
	delete g_pTheDBM;
	g_pTheDBM = NULL;
	return false;
}

//**************************************************************************************
bool CUtil3_0::PrintLevel(const COptionList &Options, UINT dwLevelID) const
//Currently, all this does for a level is save a snapshot image of the level minimap to disk.
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return false;

	CDb db;
	if (!db.IsOpen())
	{
		if (db.Open(this->strPath.c_str()) != MID_Success)
			return false;
	}

	CDbLevel *pLevel = NULL;
	{
	//Set up global DROD vars needed to perform room drawing operations.
	g_pTheDB = &db;

	//Load level.
	pLevel = db.Levels.GetByID(dwLevelID);
	if (!pLevel) return false;	//no level with this ID

	ASSERT(!g_pTheBM);
	g_pTheBM = g_pTheDBM = new CDrodBitmapManager();
	g_pTheBM->BITS_PER_PIXEL = 24;
	g_pTheBM->BYTES_PER_PIXEL = g_pTheBM->BITS_PER_PIXEL / 8;
	MESSAGE_ID ret = (MESSAGE_ID)g_pTheDBM->Init();
	if (ret) goto Failure;

	//Get level map widget.
	static const UINT ROOM_COLS = 38, ROOM_ROWS = 32; //assume all rooms are same size
	static CMapWidget *pMapWidget = NULL;
	if (!pMapWidget) //only create one instance for all calls
		pMapWidget = new CMapWidget(0, 0, 0, ROOM_COLS, ROOM_ROWS, NULL);
	ASSERT(pMapWidget);
	if (!pMapWidget->LoadFromLevel(pLevel))
		goto Failure;
	SDL_Surface *pMapSurface = pMapWidget->GetMapSurface();
	if (!pMapSurface)
		goto Failure;

	//To save the map without the border.
	SDL_Surface *pMapSurfaceSmall = SDL_CreateRGBSurface(SDL_SWSURFACE,
			pMapSurface->w - ROOM_COLS*2 - 1, pMapSurface->h - ROOM_ROWS*2 - 1, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0);
	if (!pMapSurfaceSmall) goto Failure;
	SDL_Rect mapRect = {ROOM_COLS, ROOM_ROWS, pMapSurfaceSmall->w, pMapSurfaceSmall->h};
	SDL_BlitSurface(pMapSurface, &mapRect, pMapSurfaceSmall, NULL);

	static const WCHAR wszLevel[] = {{'l'},{'e'},{'v'},{'e'},{'l'},{0}};
	WCHAR temp[10];
	WSTRING fileName = wszLevel;
	fileName += _itoW(dwLevelID, temp, 10);
	CScreen::SaveSnapshot(pMapSurfaceSmall, fileName);

	//Uninit map widget.
	SDL_FreeSurface(pMapSurfaceSmall);
	SDL_FreeSurface(pMapSurface);
	//delete pMapWidget;	//can't delete -- but causes memory leak
	pMapWidget->ClearState();
	delete pLevel;

	//Uninit all the global DROD stuff.
	g_pTheDB = NULL;

	delete g_pTheDBM;
	g_pTheDBM = NULL;
	SDL_Quit();

	return true;
	}

Failure:
	delete g_pTheDBM;
	g_pTheDBM = NULL;
	delete pLevel;
	return false;
}

//**************************************************************************************
bool CUtil3_0::PrintTest(const COptionList &Options, UINT dwDemoID) const
//Tests a demo or all demos for conquering or integrity.
{
	CDb db;
	if (!db.IsOpen())
	{
		if (db.Open(this->strPath.c_str()) != MID_Success) return false;
	}

	static const WCHAR wC[] = {{'c'},{0}};
	static const WCHAR wM[] = {{'m'},{0}};
	static const WCHAR wS[] = {{'s'},{0}};
	const bool bTestConquer = Options.GetSize()==0 || Options.Exists(wC);
	const bool bTestMonsters = Options.GetSize()==0 || Options.Exists(wM);
	const bool bTestChecksum = Options.GetSize()==0 || Options.Exists(wS);
	OPTIONNODE *pOpNode = Options.Get(wS);
	const UINT dwChecksum = pOpNode ? _Wtoi(pOpNode->szAttributes) : 0;

	CIDList DemoStats;
	bool bRes = true;
	CDbDemo *pDemo;
	if (dwDemoID)
	{
		//Test one demo.
		pDemo = db.Demos.GetByID(dwDemoID);
		if (!pDemo) return false;	//no demo with this ID
		bRes &= pDemo->Test(DemoStats);
		if (bTestConquer) bRes &= GetDemoStatBool(DemoStats,DS_WasRoomConquered);
		if (bTestMonsters) bRes &= (GetDemoStatUint(DemoStats,DS_MonsterCount) == 0);
		if (bTestChecksum) bRes &= (GetDemoStatUint(DemoStats,DS_FinalChecksum) == dwChecksum);
		delete pDemo;
	} else {
		//Test all demos.
		pDemo = db.Demos.GetFirst();
		while (pDemo)
		{
			bRes &= pDemo->Test(DemoStats);
			if (bTestConquer) bRes &= GetDemoStatBool(DemoStats,DS_WasRoomConquered);
			if (bTestMonsters) bRes &= (GetDemoStatUint(DemoStats,DS_MonsterCount) == 0);
			if (bTestChecksum) bRes &= (GetDemoStatUint(DemoStats,DS_FinalChecksum) == dwChecksum);
			delete pDemo;
			if (!bRes) return false;	//a demo failed
			pDemo = db.Demos.GetNext();
		}
	}
	return bRes;
}

//
//Private methods.
//

//**************************************************************************************
void CUtil3_0::GetMasterFilepath(
//Concat filepath to the master .dat filename.
//
//Params:
	WSTRING &wstrFilepath)  //(out)  Filepath.
const
{
	static const WCHAR wszMasterDat[] =
		{{'d'},{'r'},{'o'},{'d'},{'5'},{'_'},{'0'},{'.'},{'d'},{'a'},{'t'},{0}};
	wstrFilepath += this->strPath.c_str();
	wstrFilepath += wszSlash;
	wstrFilepath += wszMasterDat;
}

//**************************************************************************************
bool CUtil3_0::DeleteDat(
//Delete a dat file.
//
//Params:
	const WCHAR *pwszFilepath) //(in)   Dat to delete.
{
	const string filepath = UnicodeToUTF8(WSTRING(pwszFilepath));
#ifdef WIN32
	OFSTRUCT ofs;
	memset(&ofs, 0, sizeof(ofs));
	return (OpenFile(filepath.c_str(), &ofs, OF_DELETE)!=HFILE_ERROR);
#else
	return !unlink(filepath.c_str());
#endif
}

//****************************************************************************************************
bool CUtil3_0::ImportBasicMessages(
//Import basic messages (those that correspond to MID_* constants) from .uni files.  Any
//existing texts will be deleted.  A header file containing MID_* constants that match message
//records will be generated at destination path.
//
//Params:
	 const WCHAR *pwzSrcPath,    //(in)  Path to .uni files.  All files matching *.uni will be imported.
	 c4_Storage &TextStorage)    //(in)  database containing a text messages viewdef
//
//Returns:
//True if successful, false if not.
const
{
	 //Get map of already-assigned MIDs from existing MIDS.h.
	 UINT dwLastMessageID = 0;
	 WSTRING wstrHeaderFilepath;
	 ASSIGNEDMIDS AssignedMIDs;
	 {
		  static const WCHAR wszMIDS_H[] = {{'M'},{'I'},{'D'},{'s'},{'.'},{'h'},{0}};
		  wstrHeaderFilepath = pwzSrcPath;
		  wstrHeaderFilepath += wszSlash;
		  wstrHeaderFilepath += wszMIDS_H;
		  GetAssignedMIDs(wstrHeaderFilepath.c_str(), AssignedMIDs, dwLastMessageID);
	 }

	 //Concat first part of MIDS.h.
	 string strMIDs =
		  "//MIDs.h" NEWLINE
		  "//Generated by DRODUtil";
	 strMIDs += NEWLINE
		  "" NEWLINE
		  "#ifndef _MIDS_H_" NEWLINE
		  "#define _MIDS_H_" NEWLINE
		  "" NEWLINE
		  "enum MID_CONSTANT {" NEWLINE
		  "  //Range of MIDs between 0 and 99 are reserved for IDs that do not correspond to" NEWLINE
		  "  //messages stored in the database." NEWLINE
		  "  MID_Success = 0," NEWLINE
		  "  MID_DatMissing = 1," NEWLINE
		  "  MID_DatNoAccess = 2," NEWLINE
		  "  MID_DatCorrupted_NoBackup = 3," NEWLINE
		  "  MID_DatCorrupted_Restored = 4," NEWLINE
		  "  MID_CouldNotOpenDB = 5," NEWLINE
		  "  MID_MemPerformanceWarning = 6," NEWLINE
		  "  MID_MemLowWarning = 7," NEWLINE
		  "  MID_MemLowExitNeeded = 8," NEWLINE
		  "  MID_AppConfigError = 9," NEWLINE
		  "  MID_DRODIsAlreadyRunning = 10," NEWLINE
		  "  MID_DataPathDotTextFileIsInvalid = 11," NEWLINE
		  "  MID_LastUnstored = 99," NEWLINE;
	 if (dwLastMessageID < 99) dwLastMessageID = 99;

	 //Delete any existing basic messages from the database.  (basic means "MIDs < uFirstNonSimpleMessageID").
	 {
		  c4_View MessageTextsView = TextStorage.View("MessageTexts");
		  long lMessageTextCount = MessageTextsView.GetSize();
		  while (lMessageTextCount--)
		  {
				if (p_MessageID( MessageTextsView[lMessageTextCount] ) < int(uFirstNonSimpleMessageID))
					 MessageTextsView.RemoveAt(lMessageTextCount);
		  }
	 }

	 //Ensure Incremented IDs for text exist.
	c4_View IncrementedIDs = TextStorage.View("IncrementedIDs");
	if (IncrementedIDs.GetSize() < 1)
		IncrementedIDs.Add(
			p_MessageTextID[            0 ] +
			p_MessageID[                uFirstNonSimpleMessageID ]);
	else {
		c4_RowRef row = IncrementedIDs.GetAt(0);
		p_MessageID(row) = uFirstNonSimpleMessageID;
	}

	WSTRING wstrSearchPath = pwzSrcPath;
#ifdef WIN32
	 //Concat search path.
	 const WCHAR wszUNI[] = {{'*'},{'.'},{'u'},{'n'},{'i'},{0}};
	 wstrSearchPath += wszSlash;
	 wstrSearchPath += wszUNI;

	 //Import every *.UNI file found at the source path.
	 WIN32_FIND_DATA FindData;
	 HANDLE hFindUNI = FindFirstFile(wstrSearchPath.c_str(), &FindData);
	 if (hFindUNI != INVALID_HANDLE_VALUE)
	 {
		 do
		 {
			 WSTRING wstrUNIFilepath = pwzSrcPath;
			 wstrUNIFilepath += wszSlash;
			 wstrUNIFilepath += FindData.cFileName;
			 const string szFilename = UnicodeToUTF8(FindData.cFileName);
#else
	const string szSearchPath = UnicodeToUTF8(wstrSearchPath);
	DIR *pdFindUNI = opendir(szSearchPath.c_str());
	if (pdFindUNI)
	{
		struct dirent *pDir;
		while ((pDir = readdir(pdFindUNI)))
		{
			const size_t len = strlen(pDir->d_name);
			if (len < 4 || strcasecmp(pDir->d_name + len - 4, ".uni"))
				continue;

			WSTRING wstrUNIFilepath = pwzSrcPath;
			wstrUNIFilepath += wszSlash;
			WSTRING wstrFilename;
			UTF8ToUnicode(pDir->d_name, wstrFilename);
			wstrUNIFilepath += wstrFilename;
#endif

			strMIDs +=
			"" NEWLINE
			"  //Messages from ";
#ifdef WIN32
			strMIDs += szFilename;
#else
			strMIDs += pDir->d_name;
#endif
			strMIDs += ":" NEWLINE;

			if (!ImportUNI(wstrUNIFilepath.c_str(), TextStorage, AssignedMIDs, dwLastMessageID, strMIDs))
			{
				TextStorage.Rollback();
				return false;
			}
#ifdef WIN32
		} while (FindNextFile(hFindUNI, &FindData));

		FindClose(hFindUNI);
#else
		}

		closedir(pdFindUNI);
#endif
	}

	//Finish MIDs concatenation.
	strMIDs +=
		  "" NEWLINE
		  "  MID_END_UNUSED" NEWLINE
		  "};" NEWLINE
		  "" NEWLINE
		  "#endif //...#ifndef _MIDS_H_" NEWLINE;

	//Write MIDs.h.
	{
		CGameStream HeaderStream(wstrHeaderFilepath.c_str(), "wb");
		if (!HeaderStream.Write(strMIDs.c_str(), strMIDs.size()))
		{
			ASSERT(!"Failed to write header.");
			TextStorage.Rollback();
			return false;
		}
	}

	//Commit database writes.
	TextStorage.Commit();
	return true;
}

//****************************************************************************************************
bool CUtil3_0::ImportUNI(
//Imports contents of one UNI file into the text dat.
//
//Params:
  const WCHAR *pwzFilepath,         //(in)      Full path to UNI file.
  c4_Storage &TextStorage,          //(in)      text.dat
  const ASSIGNEDMIDS &AssignedMIDs, //(in)      Already assigned MIDs.
  UINT &dwLastMessageID,           //(in/out)  Accepts last message ID used.  Returned with new value
												//          of last message ID used.
  string &strMIDs)                  //(in/out)  C++ MID_* enumerations appended to this.
//
//Returns:
//True if successful, false if not.
const
{
  /* Example Format 1.

  [MID_NoText]
  [English]

  [MID_Yes]
  [English]
  &Yes

  [French]
  &Oui

  [MID_PressAnyKeyToContinue]
  [English]
  Press any key to continue.

  [French]
  Appuyez n'importe quelle touche pour continuer.


  Example Format 2--no whitespace.

  [MID_NoText][English][MID_Yes][English]&Yes[French]&Oui[MID_PressAnyKeyToContinue][Engli
  sh]Press any key to continue.[French]Appuyez n'importe quelle touche pour continuer.
  */

# define IS_WHITESPACE(c) \
	 ((WCHAR)(c)==' ' || (WCHAR)(c)==9 || (WCHAR)(c)=='\r' || (WCHAR)(c)=='\n' || (WCHAR)(c)==0)

  //Read the whole .UNI file into a buffer.
  CStretchyBuffer Source;
  if (!CFiles::ReadFileIntoBuffer(pwzFilepath, Source)) return false;

  const WCHAR wszNameTagStart[] = {{'M'},{'I'},{'D'},{'_'},{0}};
  char szNameTag[MAXLEN_NAMETAG + 1];

  const WCHAR *pSeek = (const WCHAR *)((const BYTE *)Source);
  const WCHAR *pStop = (const WCHAR *)((const BYTE *) pSeek + Source.Size());
  const WCHAR *pTagStart;
  UINT wTagLen;
  while (pSeek <= pStop)
  {
	 //Looking for a name tag.
	 while (true)
	 {
		while (*pSeek != '[' && pSeek < pStop) ++pSeek;
		if (pSeek + 5 >= pStop) return true;
		if (WCSncmp(pSeek + 1, wszNameTagStart, (sizeof(wszNameTagStart)-1)/2)!=0)
		  continue; //Some other tag--keep looking.
		else
		  break;
	 }

	 //
	 //Found a name tag.
	 //

	 //Find end of name tag.
	 ASSERT(*pSeek == '['); //pSeek is at '[' in "[MID_*]".
	 pTagStart = pSeek;
	 while (*pSeek != ']' && pSeek < pStop) ++pSeek;
	 if (pSeek >= pStop) {ASSERTP(false, "Parse name tag error."); return false;}
	 wTagLen = pSeek - pTagStart + 1;
	 if (wTagLen - 2 > MAXLEN_NAMETAG) {ASSERTP(false, "Parse name tag error.(2)"); return false;}
	 pSeek = pTagStart + 1;
	 char *pszWriteNameTag = szNameTag;
	 while (*pSeek != ']')
		*(pszWriteNameTag++) = (char) pWCv(pSeek++);
	 *pszWriteNameTag = 0;

	 //Use previously-assigned MID or a new one?
	 UINT dwUseMessageID;
	 {
		  ASSIGNEDMIDS::const_iterator iMID;
		  string strNameTag = szNameTag;
		  iMID = AssignedMIDs.find(strNameTag);
		  if (iMID == AssignedMIDs.end())
				dwUseMessageID = ++dwLastMessageID; //Use new MID.
		  else    //Use existing MID.
		  {
				dwUseMessageID = iMID->second;
		  }
	 }

	 //Copy name to MIDs concatenation.
	 {
		  //In the format "  tagname = tagvalue,".
		  static char szNumBuf[20];
		  strMIDs += "  ";
		  strMIDs += szNameTag;
		  strMIDs += " = ";
		  strMIDs += _itoa(dwUseMessageID, szNumBuf, 10);
		  strMIDs += ",";
		  strMIDs += "" NEWLINE;
	 }

	 //Look for language tags with associated text to store.
	 pSeek = pTagStart + wTagLen;
	 while (true)
	 {
		//Find next language tag.
		while (*pSeek != '[' && pSeek < pStop) ++pSeek;
		if (pSeek >= pStop) return true;
		pTagStart = pSeek;
		while (*pSeek != ']' && pSeek < pStop) ++pSeek;
		if (pSeek + 5 >= pStop) {ASSERTP(false, "Find next language tag error."); return false;}
		UINT wTagLen = pSeek - pTagStart + 1;
		if (WCSncmp(pTagStart + 1, wszNameTagStart, (sizeof(wszNameTagStart)-1)/2)==0)
		  {pSeek = pTagStart; break;} //New name tag found.

		//Which language?
		WSTRING wstr(pTagStart + 1, wTagLen - 2);
		const Language::LANGUAGE eLanguage = Language::Get(wstr.c_str());
		if (eLanguage == Language::Unknown)
		{
		  ASSERTP(false, "Unexpected language."); return false;
		}

		//Find start of text.
		pSeek = pTagStart + wTagLen;
		while (pSeek < pStop && IS_WHITESPACE(*pSeek)) ++pSeek;
		if (pSeek + 1 >= pStop) return true;
		if (*pSeek == '[' && pSeek[1] != '[') //An unescaped left bracket was found before any text.
		{
		  //This is an empty string.
		  AddMessageText(TextStorage, dwUseMessageID, eLanguage, wszEmpty);
		  continue;
		}
		pTagStart = pSeek;

		//Find end of text by finding first unescaped left bracket and then backing up
		//past any whitespace.
		while (true)
		{
		  while (pSeek < pStop && *pSeek != '[') ++pSeek;
		  if (pSeek == pStop) break;
		  if (pSeek + 1 < pStop && pSeek[1] == '[') //Escaped left bracket.
		  {
			 pSeek += 2;
			 continue;
		  }
		  else //Unescaped left bracket.
			 break;
		}
		--pSeek;
		while (pSeek > pTagStart && IS_WHITESPACE(*pSeek)) --pSeek;
		if (pSeek < pTagStart) {ASSERTP(false, "Find end of text error."); return false;}
		++pSeek;
		wTagLen = pSeek - pTagStart;
		if (wTagLen > 10000) {ASSERTP(false, "Find end of text error.(2)"); return false;} //10000 = unreasonably lengthy text.

		//Copy text into a buffer.
		WCHAR *pwzText = new WCHAR[wTagLen + 1];
		WCHAR *pwzWrite = pwzText;
		const WCHAR *pRead = pTagStart, *pStopRead = pRead + wTagLen;
		if (!pwzText) {ASSERTP(false, "Alloc failed."); return false;}
		while (pRead < pStopRead)
		{
		  if (*pRead == '[') //Escaped left bracket.
		  {
			 ASSERT(pRead[1] == '[' && pRead + 1 < pStopRead);
			 pWCv(pwzWrite++) = '[';
			 pRead += 2;
		  }
		  else if (*pRead == ']') //Escaped right bracker.
		  {
			 ASSERT(pRead[1] == ']' && pRead + 1 < pStopRead);
			 pWCv(pwzWrite++) = ']';
			 pRead += 2;
		  }
		  else
			 *(pwzWrite++) = *(pRead++);
		}
		pWCv(pwzWrite) = 0;

		//Add MessageTexts record to database.
		AddMessageText(TextStorage, dwUseMessageID, eLanguage, pwzText);
		delete [] pwzText;
	 } //...for each language tag.
  } //...for each name tag.

# undef IS_WHITESPACE

  return true;
}

//****************************************************************************************************
void CUtil3_0::AddMessageText(
//Add MessageTexts record to database.
//
//Params:
	 c4_Storage &TextStorage,  //(in)  text.dat
	 const UINT dwMessageID,        //(in)  These params supply field values for record.
	 const Language::LANGUAGE eLanguage,  //(in)
	 const WCHAR *pwszText)    //(in)
{
	//Get next MessageTextID.
	c4_View IncrementedIDsView = TextStorage.View("IncrementedIDs");
	UINT dwMessageTextID = p_MessageTextID(IncrementedIDsView[0]);
	p_MessageTextID(IncrementedIDsView[0]) = ++dwMessageTextID;

	//Add record.
	c4_Bytes TextBytes(pwszText, (WCSlen(pwszText) + 1)*sizeof(WCHAR));
	c4_View MessageTextsView = TextStorage.View("MessageTexts");
	MessageTextsView.Add(
		p_MessageTextID[ dwMessageTextID ] +
		p_MessageID[ dwMessageID ] +
		p_LanguageCode[ (int)eLanguage ] +
		p_MessageText[ TextBytes ] );
}

//****************************************************************************************************
void CUtil3_0::GetAssignedMIDs(
//Populate map of MID names-to-values from existing MIDs.h file.
//
//Params:
	 const WCHAR *pwzMIDFilepath,        //(in)  Path to mids.h.
	 ASSIGNEDMIDS &AssignedMIDs,         //(out) Receives name/value elements.
	 UINT &dwLastMessageID)             //(out) Receives largest MID value found.
const
{
	 dwLastMessageID = 0;

	 //Read file into buffer.
	 std::set<UINT> usedMids;
	 CStretchyBuffer buf;
	 CFiles files;
	 if (!files.ReadFileIntoBuffer(pwzMIDFilepath, buf)) {
		 ASSERT(!"Read file failed."); return;
	}

	 //Skip to first "{" which indicates start of enum values.
	 const BYTE *pszSeek = (const BYTE *)buf;
	 while (*pszSeek && *pszSeek != '{') ++pszSeek;
	 if (!*pszSeek) return;

	 //Each iteration reads one name/value pair and adds to assigned mids param.
	 const UINT MAXLEN_VALUE = 15;
	 char szName[MAXLEN_NAMETAG + 1], szValue[MAXLEN_VALUE + 1], *pszWrite;
	 while (true)
	 {
		  //Find next "MID_".  If I don't find it then return.
		  while (*pszSeek)
		  {
				if (pszSeek[0] == 'M' && pszSeek[1] == 'I' && pszSeek[2] == 'D' && pszSeek[3] == '_')
					 break;
				++pszSeek;
		  }
		  if (!*pszSeek) return;

		  //Found MID name.  Copy until space found.
		  ASSERT(*pszSeek == 'M');
		  pszWrite = szName;
		  while (*pszSeek && *pszSeek != ' ')
		  {
				if (pszWrite - szName > static_cast<int>(MAXLEN_NAMETAG)) {ASSERT(!"Copy MID name error."); return;}
				*(pszWrite++) = *(pszSeek++);
		  }
		  if (!*pszSeek) return;
		  *pszWrite = 0;

		  //Check for end of enum values.  Last value, "MID_Count", will not have a
		  //space/equal after it.
		  if (pszSeek[0] != ' ' || pszSeek[1] != '=' || pszSeek[2] != ' ') return;
		  pszSeek += 3;

		  //Copy value until space found.
		  pszWrite = szValue;
		  while (*pszSeek >= '0' && *pszSeek <= '9')
		  {
				if (pszWrite - szValue > static_cast<int>(MAXLEN_VALUE)) {ASSERT(!"Copy value error."); return;}
				*(pszWrite++) = *(pszSeek++);
		  }
		  if (!*pszSeek) return;
		  *pszWrite = 0;

		  //Add element.
		  string strName = szName;
		  UINT dwValue = convertToUINT(szValue);
		  if (usedMids.find(dwValue) != usedMids.end()) {
			  printf("Duplicate MID value found in key '%s' (value %u)", strName.c_str(), dwValue);
			  throw false;
		  }
		  usedMids.insert(dwValue);
		  AssignedMIDs[strName] = dwValue;

		  //If this is the largest MID value, remember it.
		  if (dwValue > dwLastMessageID) dwLastMessageID = dwValue;

	 } //...keep looping forever.  Exit condition inside of loop.
}
