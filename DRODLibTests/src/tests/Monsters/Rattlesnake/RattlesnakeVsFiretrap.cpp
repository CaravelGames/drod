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

	SECTION("Serpent should be killed by the fire trap"){
		CTestDb::NameCurrentLevel("Serpent Gets Killed By Fire Trap");
		CSerpent* rattlesnake = DYN_CAST(CSerpent*, CMonster*, RoomBuilder::AddMonster(M_SERPENTB, 10, 10, E));
		RoomBuilder::AddSerpentPiece(rattlesnake, 9, 10);
		RoomBuilder::AddSerpentPiece(rattlesnake, 8, 10);
		RoomBuilder::AddSerpentPiece(rattlesnake, 7, 10);
		RoomBuilder::Plot(T_FIRETRAP_ON, 11, 10);

		CCurrentGame* game = Runner::StartGame(15, 10, E);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->pFirstMonster == NULL);
	}

	SECTION("Serpent should be killed by the fire trap even if its tail is damaged on the same turn"){
		CTestDb::NameCurrentLevel("SerpentGetsKilledByFireTrapEv");
		CSerpent* rattlesnake = DYN_CAST(CSerpent*, CMonster*, RoomBuilder::AddMonster(M_SERPENTB, 10, 10, E));
		RoomBuilder::AddSerpentPiece(rattlesnake, 9, 10);
		RoomBuilder::AddSerpentPiece(rattlesnake, 8, 10);
		RoomBuilder::AddSerpentPiece(rattlesnake, 7, 10);
		RoomBuilder::Plot(T_FIRETRAP_ON, 11, 10);
		RoomBuilder::Plot(T_FIRETRAP_ON, 8, 10);

		CCurrentGame* game = Runner::StartGame(15, 10, E);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->pFirstMonster == NULL);
	}
}
