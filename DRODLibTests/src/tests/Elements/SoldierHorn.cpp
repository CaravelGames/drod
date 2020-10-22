#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

#include <vector>
using namespace std;

namespace {
	void RunTestCase(const UINT wTiles, const UINT wCenterTile, const UINT wAppearance, const bool shouldWork) {
		RoomBuilder::PlotRect(wTiles, 0, 0, 37, 31);
		RoomBuilder::Plot(wCenterTile, 10, 10);
		RoomBuilder::Plot(T_HORN_SOLDIER, 10, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 9, 9);

		CCharacter* pCharacter = RoomBuilder::AddCharacter(0, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerAppearance, wAppearance);

		Runner::StartGame(9, 9);
		Runner::ExecuteCommand(SE);

		if (shouldWork) {
			AssertRoomHasMonster(M_STALWART2);
		}
		else {
			AssertRoomHasNoMonster(M_STALWART2);
		}
	}
}

TEST_CASE("Soldier Horn", "[game][elements]") {
	RoomBuilder::ClearRoom();
	
	// BEETHRO
	SECTION("Soldier will appear across floor for Beethro") {
		RunTestCase(T_FLOOR, T_FLOOR, M_BEETHRO, true);
	}
	
	SECTION("Soldier will not appear across deep water for Beethro") {
		RunTestCase(T_WATER, T_FLOOR, M_BEETHRO, false);
	}

	SECTION("Soldier will appear across shallow water for Beethro") {
		RunTestCase(T_SHALLOW_WATER, T_FLOOR, M_BEETHRO, true);
	}
	
	SECTION("Soldier will not appear across walls for Beethro") {
		RunTestCase(T_WALL, T_FLOOR, M_BEETHRO, false);
	}
	
	SECTION("Soldier will not appear across closed yellow door for Beethro") {
		RunTestCase(T_DOOR_Y, T_FLOOR, M_BEETHRO, false);
	}
	
	SECTION("Soldier will not appear across pits for Beethro") {
		RunTestCase(T_PIT, T_FLOOR, M_BEETHRO, false);
	}


	// FLYING ENTITY
	SECTION("Soldier will appear across floor for Wraithwing") {
		RunTestCase(T_FLOOR, T_FLOOR, M_WWING, true);
	}
	
	SECTION("Soldier will not appear across deep water for Wraithwing") {
		RunTestCase(T_WATER, T_FLOOR, M_WWING, false);
	}

	SECTION("Soldier will appear across shallow water for Wraithwing") {
		RunTestCase(T_SHALLOW_WATER, T_FLOOR, M_WWING, true);
	}

	SECTION("Soldier will not appear across walls for Wraithwing") {
		RunTestCase(T_WALL, T_FLOOR, M_WWING, false);
	}
	
	SECTION("Soldier will not appear across closed yellow door for Wraithwing") {
		RunTestCase(T_DOOR_Y, T_FLOOR, M_WWING, false);
	}
	
	SECTION("Soldier will not appear across pits for Wraithwing") {
		RunTestCase(T_PIT, T_FLOOR, M_WWING, false);
	}


	// SWIMMING ENTITY
	SECTION("Soldier will not appear across floor for Waterskipper") {
		RunTestCase(T_FLOOR, T_WATER, M_WATERSKIPPER, false);
	}

	SECTION("Soldier will not appear across deep water for Waterskipper") {
		RunTestCase(T_WATER, T_WATER, M_WATERSKIPPER, false);
	}

	SECTION("Soldier will appear across shallow water for Waterskipper if token on shallow water") {
		RunTestCase(T_SHALLOW_WATER, T_SHALLOW_WATER, M_WATERSKIPPER, true);
	}

	SECTION("Soldier will not appear across shallow water for Waterskipper if token on deep water") {
		RunTestCase(T_SHALLOW_WATER, T_WATER, M_WATERSKIPPER, false);
	}

	SECTION("Soldier will not appear across walls for Waterskipper") {
		RunTestCase(T_WALL, T_WATER, M_WATERSKIPPER, false);
	}

	SECTION("Soldier will not appear across closed yellow door for Waterskipper") {
		RunTestCase(T_DOOR_Y, T_WATER, M_WATERSKIPPER, false);
	}

	SECTION("Soldier will not appear across pits for Waterskipper") {
		RunTestCase(T_PIT, T_WATER, M_WATERSKIPPER, false);
	}
	
}
