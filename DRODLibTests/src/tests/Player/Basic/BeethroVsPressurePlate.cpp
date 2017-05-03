#include "../../../test-include.hpp"

TEST_CASE("Beethro interacts correctly with pressure plates", "") {
	RoomBuilder::ClearRoom();
	RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
	RoomBuilder::AddOrbDataToTile(10, 10);

	SECTION("Standing onto pressure plate sends an event"){
		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(9, 10, N);
		Runner::ExecuteCommand(CMD_E, CueEvents);

		REQUIRE(CueEvents.HasOccurred(CID_PressurePlate));
	}

	SECTION("Stepping off a pressure plate sends an event"){
		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 10, N);
		Runner::ExecuteCommand(CMD_E, CueEvents);

		REQUIRE(CueEvents.HasOccurred(CID_PressurePlateReleased));
	}
}
