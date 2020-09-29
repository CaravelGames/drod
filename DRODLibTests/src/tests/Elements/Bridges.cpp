#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

#include <vector>
using namespace std;

TEST_CASE("Bridges", "[game][elements]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Bridge should not fall on turn 0") {
		RoomBuilder::PlotRect(T_PIT, 15, 15, 20, 20);
		RoomBuilder::Plot(T_BRIDGE, 17, 17);

		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		
		AssertTile(17, 17, T_BRIDGE);
	}

	SECTION("Bridge should not fall on turn 0 after double is placed") {
		RoomBuilder::PlotRect(T_PIT, 15, 15, 20, 20);
		RoomBuilder::Plot(T_BRIDGE, 17, 17);
		RoomBuilder::Plot(T_POTION_K, 10, 10);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		Runner::PlaceDouble(5, 5, CueEvents);

		AssertTile(17, 17, T_BRIDGE);
	}
}
