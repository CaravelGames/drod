#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Move Platforms Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotRect(T_PIT, 8, 8, 12, 12);
	RoomBuilder::Plot(T_PLATFORM_P, 10, 10);

	SECTION("Test Move Platforms On") {
		// By default, non-human characters do not move platforms
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MovePlatforms, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(11, 10);
		AssertTile(11, 10, T_PLATFORM_P);
	}

	SECTION("Test Move Platforms Off") {
		// By default, human characters move platforms
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MovePlatforms, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 10);
		AssertTile(10, 10, T_PLATFORM_P);
	}
}
