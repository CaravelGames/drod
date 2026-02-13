#include "catch.hpp"
#include "CTestDb.h"

#include "../../DRODLib/Db.h"
#include "../../DRODLib/DbPlayers.h"
#include "../../DRODLib/EntranceData.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Date.h>
#include <BackEndLib/Internet.h>
#include <BackEndLib/Metadata.h>
#include <BackEndLib/StretchyBuffer.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
# include <unistd.h>
# include <limits.h>
#endif

CDbHold *g_pTheHold = NULL;

CDb* CTestDb::db = NULL;
CFiles* CTestDb::files = NULL;
CDbPlayer* CTestDb::player = NULL;
CDbHold* CTestDb::hold = NULL;
CDbLevel* CTestDb::level = NULL;
CDbRoom* CTestDb::room = NULL;
CCurrentGame* CTestDb::lastCurrentGame = NULL;
UINT CTestDb::globalPlayerID = UINT(-1);
UINT CTestDb::globalHoldID = UINT(-1);
UINT CTestDb::globalLevelID = UINT(-1);
UINT CTestDb::globalRoomID = UINT(-1);
const std::string *CTestDb::currentTestCaseName = NULL;

bool Setting_SaveTestLevels = false;

void CTestDb::Init(int argc, char* const argv[])
{

	CTestDb::InitializeCFiles(argv);
	CTestDb::InitializeDb();
	CTestDb::InitializePlayer();
	CTestDb::InitializeHold();
	CTestDb::InitializeLevel();
	CTestDb::InitializeRoom();
	CTestDb::InitializeEntrance();
	CTestDb::db->Commit();
}

void CTestDb::Teardown()
{
	CTestDb::db->Holds.Delete(CTestDb::globalHoldID);
}

CCurrentGame* CTestDb::GetGame(const UINT playerX, const UINT playerY, const UINT playerO){
	CCueEvents CueEvents;

	return CTestDb::GetGame(playerX, playerY, playerO, CueEvents);
}

void CTestDb::NameCurrentLevel(WCHAR* name){
	level->NameText = name;
}

void CTestDb::NameCurrentLevel(const char* name){
	level->NameText = UTF8ToUnicode(name).c_str();
}

CCurrentGame* CTestDb::GetGame(const UINT playerX, const UINT playerY, const UINT playerO, CCueEvents &CueEvents){
	if (CTestDb::lastCurrentGame != NULL){
		CTestDb::lastCurrentGame->Clear();
		delete CTestDb::lastCurrentGame;
	}
	CTestDb::lastCurrentGame = CTestDb::db->GetNewTestGame(CTestDb::globalRoomID, CueEvents, playerX, playerY, playerO, true);

	return CTestDb::lastCurrentGame;
}


void CTestDb::InitializeCFiles(char* const argv[]){
	const WCHAR wszUniqueResFile[] = {
		We('d'), We('r'), We('o'), We('d'), We('5'), We('_'), We('0'), We('.'), We('d'), We('a'), We('t'), We(0) };
	WSTRING wstrPath;
	CTestDb::GetAppPath(argv[0], wstrPath);

	std::vector<string> datFiles; //writable .dats.  [0]: + = copy, - = no copy
	std::vector<string> playerDataSubDirs;  // subdirs to create. [0]: + = copy files, - = don't copy files, anything else = don't copy & don't offset name
	playerDataSubDirs.push_back("Bitmaps");
#if defined(__linux__) || defined (__FreeBSD__) //|| defined(__APPLE__)
	playerDataSubDirs.push_back("+Homemade");
#else
	playerDataSubDirs.push_back("Homemade");
#endif
	playerDataSubDirs.push_back("Music");
	playerDataSubDirs.push_back("Sounds");
	CFiles::InitAppVars(wszUniqueResFile, datFiles, playerDataSubDirs);
	CTestDb::files = new CFiles(wstrPath.c_str(), wszDROD, wszDROD_VER, false, true, true);
	if (CFiles::bad_data_path_file) {
		throw 1;
	}
}

void CTestDb::InitializeDb()
{
	CTestDb::db = g_pTheDB = new CDb;
	CTestDb::db->Open();
}

void CTestDb::InitializePlayer()
{
	CTestDb::player = CTestDb::db->Players.GetNew();
	const WCHAR NameText[] = { We('_'), We('_'), We('T'), We('e'), We('s'), We('t'), We(0) };
	CTestDb::player->NameText = NameText;
	CTestDb::player->CNetNameText = wszEmpty;
	CTestDb::player->CNetPasswordText = wszEmpty;
	CTestDb::player->Update();
	CTestDb::globalPlayerID = CTestDb::player->dwPlayerID;
	CTestDb::db->Commit();
}

void CTestDb::InitializeHold()
{
	CTestDb::hold = CTestDb::db->Holds.GetNew();

	const WCHAR NameText[] = { We('_'), We('_'), We('T'), We('e'), We('s'), We('t'), We(0) };
	CTestDb::hold->NameText = NameText;
	CTestDb::hold->DescriptionText = WS("");
	CTestDb::hold->dwPlayerID = CTestDb::globalPlayerID;
	if (!CTestDb::hold->Update())
	{
		delete CTestDb::hold;
		throw - 1;
	}

	CTestDb::globalHoldID = CTestDb::hold->dwHoldID;
}

void CTestDb::InitializeLevel()
{
	WSTRING wstrName;

	CTestDb::level = g_pTheDB->Levels.GetNew();

	//Set members that correspond to database fields.
	//Note: pLevel->dwHoldID was already set to match its containing hold
	//in the call to CDbLevels::GetNew().
	CTestDb::level->dwPlayerID = CTestDb::globalPlayerID;
	const WCHAR NameText[] = { We('_'), We('_'), We('T'), We('e'), We('s'), We('t'), We(0) };
	CTestDb::level->NameText = NameText;

	//Save the new level.
	CTestDb::level->dwHoldID = CTestDb::globalHoldID;
	if (!CTestDb::level->Update())
	{
		delete CTestDb::level;
		throw 2;
	}
	//Insert level into hold.
	CTestDb::hold->InsertLevel(CTestDb::level);

	//Add to level list box.
	CTestDb::globalLevelID = CTestDb::level->dwLevelID;
}

void CTestDb::InitializeRoom()
{
	//Get new room.
	CTestDb::room = g_pTheDB->Rooms.GetNew();

	//Set members that correspond to database fields.
	CTestDb::room->dwLevelID = CTestDb::globalLevelID;
	CTestDb::room->dwRoomX = 25;
	CTestDb::room->dwRoomY = 25;
	CTestDb::room->wRoomCols = DISPLAY_COLS;
	CTestDb::room->wRoomRows = DISPLAY_ROWS;
	CTestDb::room->style = WS("Badlands");
	CTestDb::room->bIsRequired = true;
	CTestDb::room->bIsSecret = false;

	if (!CTestDb::room->AllocTileLayers()) {
		delete CTestDb::room;
		throw 3;
	}

	const UINT dwSquareCount = CTestDb::room->CalcRoomArea();
	memset(CTestDb::room->pszOSquares, T_FLOOR, dwSquareCount * sizeof(char));
	memset(CTestDb::room->pszFSquares, T_EMPTY, dwSquareCount * sizeof(char));
	CTestDb::room->ClearTLayer();

	CTestDb::room->coveredOSquares.Init(DISPLAY_COLS, DISPLAY_ROWS);
	CTestDb::room->tileLights.Init(DISPLAY_COLS, DISPLAY_ROWS);

	if (!CTestDb::room->Update())
	{
		delete CTestDb::room;
		throw 4;
	}
	CTestDb::globalRoomID = CTestDb::room->dwRoomID;
	delete CTestDb::room;
	CTestDb::room = CTestDb::db->Rooms.GetByID(CTestDb::globalRoomID);
}


void CTestDb::InitializeEntrance()
{
	//And new level entrance to hold's entrance list.
	CEntranceData *pEntrance = new CEntranceData(0, 0, globalRoomID,
		DISPLAY_COLS / 2, DISPLAY_ROWS / 2,
		SE, true, CEntranceData::DD_Always, 0);
	pEntrance->DescriptionText = WS("");
	hold->AddEntrance(pEntrance);
	hold->Update();
}

void CTestDb::RegenerateRoom(){
	if (Setting_SaveTestLevels){
		WSTRING buffer;
		UTF8ToUnicode(CTestDb::currentTestCaseName->c_str(), buffer);
		level->NameText = buffer.c_str();
		level->NameText.Update();

		room->Update();
		level->Update();
		hold->Update();
		db->Commit();

		InitializeLevel();
	}
	else {
		g_pTheDB->Rooms.Delete(CTestDb::globalRoomID);
	}

	InitializeRoom();
	InitializeEntrance();
	CTestDb::db->Commit();
}

void CTestDb::GetAppPath(
	const char *pszArg0,    //(in)  First command-line argument which will be used
	//      to find application path if it is the best
	//      available way.
	WSTRING &wstrAppPath)   //(out) App path.
{
	//
	//Try to use an O/S-specific means of getting the application path.
	//

#if defined(__linux__)
	char exepath[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exepath, (ssize_t)sizeof(exepath) - 1);
	if (len > 0)
	{
		if (len >= (ssize_t)sizeof(exepath))
		{
			printf("Unable to start - executable's path is longer than the max allowed path. Move this somewhere else please.\n");
			throw 1;
		}
		exepath[len] = '\0';
		UTF8ToUnicode(exepath, wstrAppPath);
		return;
	}
#elif defined(WIN32)
	WCHAR wszPathBuffer[MAX_PATH + 1];
	if (GetModuleFileName(NULL, wszPathBuffer, MAX_PATH))
	{
		wstrAppPath = wszPathBuffer;
		return;
	}
	else //On older versions of Windows, Unicode functions fail.
	{
		char szPathBuffer[MAX_PATH + 1];
		if (GetModuleFileNameA(NULL, szPathBuffer, MAX_PATH))
		{
			UTF8ToUnicode(szPathBuffer, wstrAppPath);
			return;
		}
	}
#elif defined(__APPLE__) || defined(__FreeBSD__)
	char fullPathBuffer[PATH_MAX];
	realpath(pszArg0, fullPathBuffer);
	UTF8ToUnicode(fullPathBuffer, wstrAppPath);
	return;
#endif

	//Fallback solution--use the command-line argument.
	UTF8ToUnicode(pszArg0, wstrAppPath);
}