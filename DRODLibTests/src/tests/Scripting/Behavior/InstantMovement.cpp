#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Instant Movement Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();

	SECTION("Test instant movement") {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::InstantMovement, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// NPC should make all the moves in one turn
		AssertMonster(15, 10);
	}

	SECTION("Test instant rotation") {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, E, M_STALWART);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::InstantMovement, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_FaceDirection, CMD_C);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_FaceDirection, CMD_C);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// NPC should turn twice in one turn
		AssertMonsterO(10, 10, S);
	}
}