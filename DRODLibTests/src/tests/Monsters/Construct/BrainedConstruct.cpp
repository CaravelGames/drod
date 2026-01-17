#include "../../../test-include.hpp"

TEST_CASE("Brained Construct", "[game][construct][brain]") {
	RoomBuilder::ClearRoom();

	SECTION("Test Construct pathing with oremites") {
		RoomBuilder::PlotRect(T_WALL, 5, 7, 8, 7);
		RoomBuilder::PlotRect(T_WALL, 5, 8, 5, 9);
		RoomBuilder::PlotRect(T_WALL, 8, 8, 8, 9);
		RoomBuilder::Plot(T_WALL, 7, 9);
		RoomBuilder::PlotRect(T_WALL, 5, 11, 8, 11);
		RoomBuilder::Plot(T_GOO, 8, 10);
		RoomBuilder::Plot(T_DOOR_Y, 5, 10);
		RoomBuilder::Plot(T_ORB, 11, 9);
		RoomBuilder::AddOrbDataToTile(11, 9);
		RoomBuilder::LinkOrb(11, 9, 5, 10, OrbAgentType::OA_OPEN);

		RoomBuilder::AddMonster(M_CONSTRUCT, 7, 8);
		RoomBuilder::AddMonster(M_BRAIN, 5, 5);

		CCurrentGame* game = Runner::StartGame(10, 10, E);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		//Should move to tile adjacent to oremites
		AssertMonsterType(7, 10, M_CONSTRUCT);

		Runner::ExecuteCommand(CMD_CC);
		Runner::ExecuteCommand(CMD_WAIT);

		//Should move along non-oremite path
		AssertMonsterType(5, 10, M_CONSTRUCT);
	}

	SECTION("Test Construct pathing with multiple oremite paths") {
		RoomBuilder::PlotRect(T_WALL, 5, 5, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 7, 7, 9, 7);
		RoomBuilder::PlotRect(T_FLOOR, 9, 7, 9, 9);
		RoomBuilder::Plot(T_GOO, 5, 8);
		RoomBuilder::Plot(T_FLOOR, 6, 8);
		RoomBuilder::Plot(T_GOO, 8, 11);
		RoomBuilder::Plot(T_FLOOR, 8, 10);

		RoomBuilder::AddMonster(M_CONSTRUCT, 8, 8);
		RoomBuilder::AddMonster(M_BRAIN, 1, 1);

		CCurrentGame* game = Runner::StartGame(4, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, 3);

		//Should move near oremites closest to player.
		AssertMonsterType(6, 8, M_CONSTRUCT);

		Runner::ExecuteCommand(CMD_SE);
		Runner::ExecuteCommand(CMD_WAIT, 4);

		//Should move near oremites closest to player.
		AssertMonsterType(8, 10, M_CONSTRUCT);
	}
}
