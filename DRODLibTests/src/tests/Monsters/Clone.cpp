#include "../../catch.hpp"
#include "../../CAssert.h"
#include "../../CTestDb.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"
#include "../../../../DRODLib/CurrentGame.h"
#include "../../../../DRODLib/Character.h"
#include "../../../../DRODLib/CharacterCommand.h"

namespace {
	void _AssertProcessingSequence(const char* file, int line, const UINT expectedIndex, const UINT wO) {
		CCurrentGame* pCurrentGame = Runner::GetCurrentGame();
		CDbRoom* pRoom = pCurrentGame->pRoom;

		CMonster* pMonster = pRoom->pFirstMonster;
		UINT index = 0;
		const UINT wExpectedX = wO == S ? 10 : 20;
		const UINT wExpectedY = wO == N ? 10 : 20;
		while (pMonster)
		{
			if (pMonster->wO == wO) {
				REQUIRE(index == expectedIndex);
				REQUIRE(wExpectedX == pMonster->wX);
				REQUIRE(wExpectedY == pMonster->wY);
				REQUIRE(pRoom->GetMonsterAtSquare(pMonster->wX, pMonster->wY) == pMonster);
				REQUIRE(pMonster->wType == M_CLONE);
				return;
			}

			pMonster = pMonster->pNext;
			++index;
		}

		INFO(MakeLogMessage(file, line));
		FAIL("Failed to find the clone");
	}
}

#define AssertProcessingSequence(expectedIndex, wO) _AssertProcessingSequence(__FILE__, __LINE__, expectedIndex, wO)

TEST_CASE("Clone", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Switching clones preserves movement order") {
		RoomBuilder::AddMonster(M_CLONE, 20, 10, N);
		RoomBuilder::AddMonster(M_CLONE, 20, 20, E);
		RoomBuilder::AddMonster(M_CLONE, 10, 20, S);

		Runner::StartGame(10, 10, W);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		// Full cycle of tabbing
		Runner::ExecuteCommand(CMD_CLONE, 4);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		// Click on each clone and back
		Runner::ClickClone(20, 10);
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		Runner::ClickClone(20, 20);	
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		Runner::ClickClone(10, 20);
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		// Click various combinations
		Runner::ClickClone(10, 20);
		Runner::ClickClone(20, 20);
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		Runner::ClickClone(20, 10);
		Runner::ClickClone(10, 20);
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);

		Runner::ClickClone(20, 10);
		Runner::ClickClone(20, 20);
		Runner::ClickClone(10, 20);
		Runner::ClickClone(20, 10);
		Runner::ClickClone(10, 10);
		AssertPlayerAt(10, 10);
		AssertProcessingSequence(0, N);
		AssertProcessingSequence(1, E);
		AssertProcessingSequence(2, S);
	}
}
