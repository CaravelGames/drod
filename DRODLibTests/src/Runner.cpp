#include <sys/stat.h>
#include <fstream>
#include "Runner.h"
#include "RoomBuilder.h"
#include "CTestDb.h"
#include "catch.hpp"

CCurrentGame* Runner::currentGame = NULL;
CCueEvents Runner::lastCueEvents;
UINT Runner::wLastErrorLogSize;
char* Runner::pErrorLogPath;

CCurrentGame* Runner::StartGame(const UINT playerX, const UINT playerY, const UINT playerO){
	Runner::wLastErrorLogSize = GetErrorLogSize();
	RoomBuilder::SaveRoom();
	Runner::currentGame = CTestDb::GetGame(playerX, playerY, playerO);

	return Runner::currentGame;
}

CCurrentGame* Runner::StartGame(const UINT playerX, const UINT playerY, const UINT playerO, CCueEvents &CueEvents){
	RoomBuilder::SaveRoom();
	Runner::currentGame = CTestDb::GetGame(playerX, playerY, playerO, CueEvents);

	return Runner::currentGame;
}

CCurrentGame* Runner::GetCurrentGame() {
	if (Runner::currentGame == NULL) {
		throw "Tried to get game when there is no game started";
	}

	return Runner::currentGame;
}

CCueEvents& Runner::GetLastCueEvents() {
	return Runner::lastCueEvents;
}

void Runner::ExecuteCommand(const UINT command, const UINT repeats){
	CCueEvents CueEvents;

	for (UINT i = 0; i < repeats; ++i){
		CueEvents.Clear();
		Runner::currentGame->ProcessCommand(command, CueEvents);
	}
	Runner::lastCueEvents.SetMembers(CueEvents);
}

void Runner::ExecuteCommand(const UINT command, CCueEvents &CueEvents){
	Runner::currentGame->ProcessCommand(command, CueEvents);
	Runner::lastCueEvents.SetMembers(CueEvents);
}

void Runner::PlaceDouble(const UINT wX, const UINT wY, CCueEvents& CueEvents) {
	REQUIRE(Runner::currentGame->swordsman.wPlacingDoubleType);
	Runner::currentGame->ProcessCommand(CMD_DOUBLE, CueEvents, wX, wY);
	Runner::lastCueEvents.SetMembers(CueEvents);
}

void Runner::ClickClone(const UINT wX, const UINT wY) {
	CCueEvents CueEvents;
	ClickClone(wX, wY, CueEvents);
}

void Runner::ClickClone(const UINT wX, const UINT wY, CCueEvents& CueEvents) {
	REQUIRE(!Runner::currentGame->swordsman.wPlacingDoubleType);
	REQUIRE(Runner::currentGame->pRoom->GetMonsterTypeAt(wX, wY) == M_CLONE);
	Runner::currentGame->ProcessCommand(CMD_CLONE, CueEvents, wX, wY);
}

UINT Runner::GetErrorLogSize(){
	struct stat st;
	if (stat(pErrorLogPath, &st) == 0)
		return st.st_size;
	else
		return 0;
}

UINT Runner::GetNewAssertsCount(){
	return GetErrorLogSize() - Runner::wLastErrorLogSize;
}

CTemporalClone* Runner::GetTemporalClone(UINT indexOfClone){
	for (CMonster *pMonster = GetRoom()->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext){
		if (pMonster->wType == M_TEMPORALCLONE){
			if (indexOfClone == 0){
				return DYN_CAST(CTemporalClone*, CMonster*, pMonster);
			}
			else
			{
				--indexOfClone;
			}
		}
	}

	return NULL;
}

void Runner::InitializeDatPath(){
	WSTRING wstrExtension;
	AsciiToUnicode(".err", wstrExtension);

	WSTRING wstrDatPathTxt = CFiles::GetDatPath();
	wstrDatPathTxt += wszSlash;
	wstrDatPathTxt += CFiles::wGameName;
	wstrDatPathTxt += wstrExtension;

	pErrorLogPath = new char[wstrDatPathTxt.length() * 2];
	UnicodeToUTF8(wstrDatPathTxt.c_str(), pErrorLogPath);
}

CDbRoom* Runner::GetRoom(){
	return currentGame->pRoom;
}