#include "../../test-include.hpp"
#include "../../CAssert.h"

TEST_CASE("Scripting: Push Tile command", "[game][scripting]") {
	RoomBuilder::ClearRoom();

	CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
	RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_PushTile, 10, 10, S);

	SECTION("Test Push Player") {
		CCurrentGame* pGame = Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertPlayerAt(10, 11);
		AssertPlayerIsAlive(); //Being pushed should not be fatal
	}

	SECTION("Test Push Monster") {
		CMonster* pMonster = RoomBuilder::AddMonster(M_EYE, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(15, 15);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 11);
	}

	SECTION("Test Push Object") {
		RoomBuilder::Plot(T_MIRROR, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(15, 15);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertTile(10, 11, T_MIRROR);
	}

	SECTION("Test Push Orb") {
		RoomBuilder::Plot(T_ORB, 10, 10);
		RoomBuilder::Plot(T_DOOR_YO, 12, 10);
		RoomBuilder::AddOrbDataToTile(10, 10, OrbType::OT_NORMAL);
		RoomBuilder::LinkOrb(10, 10, 12, 10, OrbAgentType::OA_CLOSE);

		CCurrentGame* pGame = Runner::StartGame(15, 15);
		Runner::ExecuteCommand(CMD_WAIT);

		// Pushing at orb will activate it
		AssertTile(12, 10, T_DOOR_Y);
	}

	SECTION("Test Push Beacon") {
		RoomBuilder::Plot(T_BEACON, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(15, 15);
		Runner::ExecuteCommand(CMD_WAIT);

		// Pushing at seeding beacon will toggle it
		AssertTile(10, 10, T_BEACON_OFF);
	}
}
