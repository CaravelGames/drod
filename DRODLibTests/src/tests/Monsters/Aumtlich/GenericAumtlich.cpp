#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"

#include <vector>
using namespace std;

TEST_CASE("Aumtlich", "[game]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Aumtlich won't freeze player on turn 0") {
		RoomBuilder::AddMonster(M_AUMTLICH, 10, 20, N);

		CCurrentGame* game = Runner::StartGame(10, 10, N);

		REQUIRE(!game->swordsman.bFrozen);
	}

	SECTION("Aumtlich won't freeze player on turn 0 after double is placed") {
		RoomBuilder::AddMonster(M_AUMTLICH, 10, 20, N);
		RoomBuilder::Plot(T_POTION_K, 10, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 10, N);
		Runner::PlaceDouble(5, 5, CueEvents);

		REQUIRE(!game->swordsman.bFrozen);
	}
}
