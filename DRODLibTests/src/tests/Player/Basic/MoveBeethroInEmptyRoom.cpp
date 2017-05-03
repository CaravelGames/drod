#include "../../../test-include.hpp"

TEST_CASE("Beethro changes his position on move command", "[game][player moves][beethro]") {
	RoomBuilder::ClearRoom();
	CCurrentGame* game = Runner::StartGame(10, 10, N);

	REQUIRE(game != NULL);

	SECTION("Command north-east works"){
		Runner::ExecuteCommand(CMD_NE);
		REQUIRE(game->swordsman.wX == 11);
		REQUIRE(game->swordsman.wY == 9);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command north works"){
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 9);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command north-west works"){
		Runner::ExecuteCommand(CMD_NW);
		REQUIRE(game->swordsman.wX == 9);
		REQUIRE(game->swordsman.wY == 9);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command east works"){
		Runner::ExecuteCommand(CMD_E);
		REQUIRE(game->swordsman.wX == 11);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command wait works"){
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command west works"){
		Runner::ExecuteCommand(CMD_W);
		REQUIRE(game->swordsman.wX == 9);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command south-east works"){
		Runner::ExecuteCommand(CMD_SE);
		REQUIRE(game->swordsman.wX == 11);
		REQUIRE(game->swordsman.wY == 11);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command south works"){
		Runner::ExecuteCommand(CMD_S);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 11);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command south-west works"){
		Runner::ExecuteCommand(CMD_SW);
		REQUIRE(game->swordsman.wX == 9);
		REQUIRE(game->swordsman.wY == 11);
		REQUIRE(game->swordsman.wO == N);
	}
	SECTION("Command CW works"){
		Runner::ExecuteCommand(CMD_C);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->swordsman.wO == NE);
	}
	SECTION("Command CCW works"){
		Runner::ExecuteCommand(CMD_CC);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->swordsman.wO == NW);
	}
}
