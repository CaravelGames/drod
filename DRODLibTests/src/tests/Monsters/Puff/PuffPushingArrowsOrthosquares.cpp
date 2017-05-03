#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Puffs pushed against force arrows stay in place", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Push puff onto force arrow"){
		RoomBuilder::Plot(T_ARROW_S, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 10, 13);

		CCurrentGame* game = Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 11));
	}

	SECTION("Push puff from force arrow"){
		RoomBuilder::Plot(T_ARROW_S, 10, 11);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 10, 13);

		CCurrentGame* game = Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 11));
	}

	SECTION("Push puff onto wall with force arrow"){
		RoomBuilder::Plot(T_ARROW_S, 10, 10);
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 10, 13);

		CCurrentGame* game = Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 11));
	}

	SECTION("Push puff onto orthosquare"){
		RoomBuilder::Plot(T_NODIAGONAL, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 11, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 12, 13);

		CCurrentGame* game = Runner::StartGame(12, 13, N);
		Runner::ExecuteCommand(CMD_NW);
		REQUIRE(game->pRoom->GetMonsterAtSquare(11, 11));
	}
	SECTION("Push puff from orthosquare"){
		RoomBuilder::Plot(T_NODIAGONAL, 11, 11);
		RoomBuilder::AddMonster(M_FLUFFBABY, 11, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 12, 13);

		CCurrentGame* game = Runner::StartGame(12, 13, N);
		Runner::ExecuteCommand(CMD_NW);
		REQUIRE(game->pRoom->GetMonsterAtSquare(11, 11));
	}

	SECTION("Push puff onto wall with orthosquare"){
		RoomBuilder::Plot(T_NODIAGONAL, 10, 10);
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 11, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(StaffToken, 12, 13);

		CCurrentGame* game = Runner::StartGame(12, 13, N);
		Runner::ExecuteCommand(CMD_NW);
		REQUIRE(game->pRoom->GetMonsterAtSquare(11, 11));
	}

	SECTION("Puff destroyed by caber should leave thin ice"){
		RoomBuilder::Plot(T_ARROW_S, 20, 10);
		RoomBuilder::Plot(T_SHALLOW_WATER, 20, 11);
		RoomBuilder::AddMonster(M_FLUFFBABY, 20, 11, NO_ORIENTATION);
		RoomBuilder::PlotToken(CaberToken, 20, 13);

		CCurrentGame* game = Runner::StartGame(20, 13, N);
		Runner::ExecuteCommand(CMD_N, 1);
		REQUIRE(game->pRoom->GetOSquare(20, 11) == T_THINICE_SH);
	}
}
