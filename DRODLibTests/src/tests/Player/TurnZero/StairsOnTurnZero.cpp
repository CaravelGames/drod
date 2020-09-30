#include "../../../test-include.hpp"

TEST_CASE("Stairs on Turn Zero", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Beethro should not walk the stairs on turn zero"){
		RoomBuilder::Plot(T_STAIRS, 10, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 10, W, CueEvents);

		REQUIRE(!CueEvents.HasOccurred(CID_WinGame));
		REQUIRE(!CueEvents.HasOccurred(CID_ExitLevelPending));
	}
}
