#include "../../test-include.hpp"

TEST_CASE("Time Token Double Potion Pushing weirdness", "[game][potion][time token][pushing]") {
	RoomBuilder::ClearRoom();

	SECTION("Pushed from Potion to Time Token") {
		// WWWW
		// M-pt - Mimic facing East with Staff, Mimic Potion, time split token
		// .B.. - Beethro that will move NE

		RoomBuilder::PlotRect(T_WALL, 10, 10, 14, 10);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 10, 11, E);
		RoomBuilder::Plot(T_POTION_K, 12, 11);
		RoomBuilder::PlotToken(RoomTokenType::TemporalSplit, 13, 11);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(11, 12, E);
		Runner::ExecuteCommand(CMD_NE, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_TemporalSplitStart));
	}
}
