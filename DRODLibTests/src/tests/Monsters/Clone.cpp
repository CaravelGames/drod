#include "../../catch.hpp"
#include "../../CAssert.h"
#include "../../CTestDb.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"
#include "../../../../DRODLib/CurrentGame.h"
#include "../../../../DRODLib/Character.h"
#include "../../../../DRODLib/CharacterCommand.h"

namespace {

	void _AssertProcessingSequence(const char* file, int line, const UINT expectedIndex, const UINT wX, const UINT wY, const UINT wO, const UINT wType) {
		INFO("Assert processing sequence from: " << file << ":" << line);

		stringstream message;

		message << "Monster at (";
		message << wX;
		message << ",";
		message << wY;
		message << ") Orientation=";
		message << wO;
		message << " Type=";
		message << wType;

		INFO(message.str());

		CCurrentGame* pCurrentGame = Runner::GetCurrentGame();
		CDbRoom* pRoom = pCurrentGame->pRoom;

		CMonster* pMonster = pRoom->pFirstMonster;
		UINT index = 0;
		while (pMonster)
		{
			if (pMonster->wX == wX && pMonster->wY == wY) {
				REQUIRE(index == expectedIndex);
				REQUIRE(wO == pMonster->wO);
				REQUIRE(pRoom->GetMonsterAtSquare(pMonster->wX, pMonster->wY) == pMonster);
				REQUIRE(pMonster->wType == wType);
				return;
			}

			pMonster = pMonster->pNext;
			++index;
		}

		FAIL("Failed to find monster");
	}
	void _AssertProcessingSequence_Clone(const char* file, int line, const UINT expectedCloneIndex, const UINT wO) {
		CCurrentGame* pCurrentGame = Runner::GetCurrentGame();
		CDbRoom* pRoom = pCurrentGame->pRoom;

		CMonster* pMonster = pRoom->pFirstMonster;
		UINT index = 0;
		const UINT wExpectedX = wO == S ? 10 : 20;
		const UINT wExpectedY = wO == N ? 10 : 20;

		UINT expectedIndex = 0;
		switch (expectedCloneIndex) {
			case 0: expectedIndex = 1; break;
			case 1: expectedIndex = 3; break;
			case 2: expectedIndex = 6; break;
		}

		_AssertProcessingSequence(file, line, expectedIndex, wExpectedX, wExpectedY, wO, M_CLONE);
	}
}

#define AssertProcessingSequence_Common() \
	_AssertProcessingSequence(__FILE__, __LINE__, 0, 0, 0, S,  M_CHARACTER); \
	_AssertProcessingSequence(__FILE__, __LINE__, 1, 1, 5, N,  M_CLONE); \
	_AssertProcessingSequence(__FILE__, __LINE__, 2, 2, 0, S,  M_CHARACTER); \
	_AssertProcessingSequence(__FILE__, __LINE__, 3, 3, 5, SE, M_CLONE); \
	_AssertProcessingSequence(__FILE__, __LINE__, 4, 4, 0, S,  M_DECOY); \
	_AssertProcessingSequence(__FILE__, __LINE__, 5, 5, 0, S,  M_MIMIC); \
	_AssertProcessingSequence(__FILE__, __LINE__, 6, 6, 5, SW, M_CLONE); \
	_AssertProcessingSequence(__FILE__, __LINE__, 7, 7, 0, S,  M_CHARACTER);

TEST_CASE("Clone", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Switching clones preserves movement order - supports different monsters on same processing sequence") {
		CCharacter* pCharacter;

		/* INDEX=0 */ pCharacter = RoomBuilder::AddVisibleCharacter(0, 0, S, M_CLONE);
		              pCharacter->wProcessSequence = 1;

		/* INDEX=1 */ RoomBuilder::AddMonster(M_CLONE, 1, 5, N);
		/* INDEX=2 */ pCharacter = RoomBuilder::AddVisibleCharacter(2, 0, S, M_CLONE);
		              pCharacter->wProcessSequence = SPD_PDOUBLE;
		/* INDEX=3 */ RoomBuilder::AddMonster(M_CLONE, 3, 5, SE);
		/* INDEX=4 */ RoomBuilder::AddMonster(M_DECOY, 4, 0, S);
		/* INDEX=5 */ RoomBuilder::AddMonster(M_MIMIC, 5, 0, S);
		/* INDEX=6 */ RoomBuilder::AddMonster(M_CLONE, 6, 5, SW);

		/* INDEX=7 */ RoomBuilder::AddVisibleCharacter(7, 0, S, M_CLONE);

		Runner::StartGame(8, 5, N);
		AssertProcessingSequence_Common();

		// Full cycle of tabbing
		Runner::ExecuteCommand(CMD_CLONE, 4);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		// Click on each clone and back
		Runner::ClickClone(1, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		Runner::ClickClone(3, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		Runner::ClickClone(6, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		// Click various combinations
		Runner::ClickClone(6, 5);
		Runner::ClickClone(3, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		Runner::ClickClone(1, 5);
		Runner::ClickClone(6, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();

		Runner::ClickClone(1, 5);
		Runner::ClickClone(3, 5);
		Runner::ClickClone(6, 5);
		Runner::ClickClone(1, 5);
		Runner::ClickClone(8, 5);
		AssertPlayerAt(8, 5);
		AssertProcessingSequence_Common();
	}
}
