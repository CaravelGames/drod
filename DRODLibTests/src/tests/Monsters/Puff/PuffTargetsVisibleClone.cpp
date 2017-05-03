#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Puff targets clone", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Puff should target clone even if player is hiding in shallow water"){
		RoomBuilder::Plot(T_SHALLOW_WATER, 5, 5);
		RoomBuilder::AddMonster(M_FLUFFBABY, 15, 15, NO_ORIENTATION);
		RoomBuilder::AddMonster(M_CLONE, 25, 25, S);

		CCurrentGame* game = Runner::StartGame(5, 5, E);
		Runner::ExecuteCommand(CMD_WAIT, 5);
		REQUIRE(game->pRoom->GetMonsterAtSquare(16, 16));
	}

}
