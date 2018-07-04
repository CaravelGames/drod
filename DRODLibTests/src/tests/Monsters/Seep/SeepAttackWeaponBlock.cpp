#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"

TEST_CASE("Seep interaction with weapons on targets", "[game][player][seep]") {
	SECTION("Staff should block Seep attack") {
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 8, 10, E);

		CCueEvents CueEvents;
		Runner::StartGame(19, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}

	SECTION("Spear should block Seep attack") {
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Spear, 8, 10, E);

		CCueEvents CueEvents;
		Runner::StartGame(19, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}

	SECTION("Pickaxe should block Seep attack") {
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Pickaxe, 8, 10, E);

		CCueEvents CueEvents;
		Runner::StartGame(19, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}

	SECTION("Dagger should not block Seep attack") {
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Dagger, 8, 10, E);

		CCueEvents CueEvents;
		Runner::StartGame(19, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
}
