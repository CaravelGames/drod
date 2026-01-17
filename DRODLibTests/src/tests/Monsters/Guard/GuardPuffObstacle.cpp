#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Guard interaction with puffs", "[game][guard][puff]") {
	RoomBuilder::ClearRoom();

	SECTION("Guard paths around puff") {
		RoomBuilder::AddMonster(M_GUARD, 10, 10, S);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::Plot(T_WALL, 11, 11);

		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Guard should move around puff
		AssertMonsterType(9, 11, M_GUARD);
	}

	SECTION("Dagger Guard paths around puff") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, S);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::Plot(T_WALL, 11, 11);


		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Guard should move around puff
		AssertMonsterType(9, 11, M_GUARD);
	}

	SECTION("Pathmap ignores puff") {
		RoomBuilder::AddMonster(M_GUARD, 10, 10, S);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 12);

		RoomBuilder::PlotRect(T_WALL, 9, 9, 9, 12);
		RoomBuilder::PlotRect(T_WALL, 11, 9, 11, 12);

		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Guard should step south
		AssertMonsterType(10, 11, M_GUARD);
	}
}
