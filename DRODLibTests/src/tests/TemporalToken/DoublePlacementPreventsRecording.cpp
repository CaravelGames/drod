#include "../../test-include.hpp"

TEST_CASE("Time Token Double Potion Pushing weirdness", "[game][potion][time token][pushing]") {
	RoomBuilder::ClearRoom();

	SECTION("Pushed from Potion to Time Token") {
		RoomBuilder::PlotRect(T_WALL, 0, 8, 20, 8);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 9, 9, E);
		RoomBuilder::PlotToken(RoomTokenType::TemporalSplit, 11, 9);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 10, E);
		Runner::ExecuteCommand(CMD_NE, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_TemporalSplitStart));
	}
}
