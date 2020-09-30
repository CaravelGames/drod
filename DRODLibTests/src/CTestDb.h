#ifndef CTestDb_H
#define CTestDb_H

#include "../../DRODLib/CurrentGame.h"
#include "../../DRODLib/Db.h"
#include "../../DRODLib/DbHolds.h"
#include "../../DRODLib/DbPlayers.h"
#include "../../DRODLib/DbLevels.h"
#include <BackEndLib\Wchar.h>
#include <BackEndLib\Files.h>
#include <BackEndLib\Types.h>

#define DISPLAY_ROWS     32
#define DISPLAY_COLS     38

class Runner;
class CTestDb{
	friend class Runner;
	friend class RoomBuilder;

public:
	static void Init(int argc, char* const argv[]);
	static void Teardown();
	static void NameCurrentLevel(WCHAR* name);

protected:
	static CCurrentGame* GetGame(const UINT playerX, const UINT playerY, const UINT playerO);
	static CCurrentGame* GetGame(const UINT playerX, const UINT playerY, const UINT playerO, CCueEvents &CueEvents);

private:
	static void InitializeCFiles(char* const argv[]);
	static void InitializeDb();
	static void InitializePlayer();
	static void InitializeHold();
	static void InitializeLevel();
	static void InitializeRoom();
	static void InitializeEntrance();
	static void RegenerateRoom();
	static void GetAppPath(const char *pszArg0, WSTRING &wstrAppPath);
	
	static CDb* db;
	static CFiles* files;
	static CDbPlayer* player;
	static CDbHold* hold;
	static CDbLevel* level;
	static CDbRoom* room;

	static CCurrentGame* lastCurrentGame;

	static UINT globalPlayerID;
	static UINT globalHoldID;
	static UINT globalLevelID;
	static UINT globalRoomID;
};

#endif