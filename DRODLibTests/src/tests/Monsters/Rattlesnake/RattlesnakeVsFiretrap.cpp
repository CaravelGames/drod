#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Rattlesnakes interacting with Firetraps", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	SECTION("Rattlesnake should be killed by the fire trap"){
		RoomBuilder::AddLongMonster(M_SERPENTB, 10, 10, E)
			.GrowIn(W, 3).End();
		RoomBuilder::Plot(T_FIRETRAP_ON, 11, 10);

		CCurrentGame* game = Runner::StartGame(15, 10, E);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->pFirstMonster == NULL);
	}

	SECTION("Rattlesnake should be killed by the fire trap even if its tail is damaged on the same turn"){
		RoomBuilder::AddLongMonster(M_SERPENTB, 10, 10, E)
			.GrowIn(W, 3).End();
		RoomBuilder::Plot(T_FIRETRAP_ON, 11, 10);
		RoomBuilder::Plot(T_FIRETRAP_ON, 8, 10);

		CCurrentGame* game = Runner::StartGame(15, 10, E);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->pFirstMonster == NULL);
	}
}
