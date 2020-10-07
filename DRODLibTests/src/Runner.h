#ifndef RUNNER_H
#define RUNNER_H

#include <BackEndLib/Types.h>
#include "../../DRODLib/CueEvents.h"
#include "../../DRODLib/CurrentGame.h"
#include "../../DRODLib/DbRooms.h"
#include "../../DRODLib/TemporalClone.h"
#include "CTestDb.h"

class Runner{
public:
	static CCurrentGame* StartGame(const UINT playerX = 10, const UINT playerY = 10, const UINT playerO = N);
	static CCurrentGame* StartGame(const UINT playerX, const UINT playerY, const UINT playerO, CCueEvents &CueEvents);
	static void ExecuteCommand(const UINT command, const UINT repeats = 1);
	static void ExecuteCommand(const UINT command, CCueEvents &CueEvents);
	static void PlaceDouble(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	static void ClickClone(const UINT wX, const UINT wY);
	static void ClickClone(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	static UINT GetNewAssertsCount();

	static CTemporalClone* GetTemporalClone(UINT indexOfClone = 0);

	static void InitializeDatPath();
	static CCurrentGame* GetCurrentGame();

private:
	static char *pErrorLogPath;
	static UINT GetErrorLogSize();
	static CCurrentGame* currentGame;
	static CDbRoom* GetRoom();
	static UINT wLastErrorLogSize;
};

#endif