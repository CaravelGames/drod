#include "../../../test-include.hpp"
#include "../../../CAssert.h"

static CCharacter* addPushableCharacter(UINT wX, UINT wY) {
	CCharacter* pPushableCharacter = RoomBuilder::AddVisibleCharacter(wX, wY, 0, M_CITIZEN);
	RoomBuilder::AddCommand(pPushableCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByBody);
	return pPushableCharacter;
}

TEST_CASE("Scripting: Pushing Behaviors", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();

	SECTION("Test Push Objects On") {
		RoomBuilder::Plot(T_MIRROR, 10, 10);

		// Roach cannot push object by default
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PushObjects, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(11, 10, T_MIRROR);
		AssertMonster(10, 10);
	}

	SECTION("Test Push Monsters On") {
		CCharacter* pPushableCharacter = addPushableCharacter(10, 10);

		// Roach cannot push monster by default
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PushMonsters, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertMonster(10, 10);
		AssertMonster(11, 10);
	}

	SECTION("Test Push Objects Off") {
		RoomBuilder::Plot(T_MIRROR, 10, 10);

		// Citizen character can push object by default
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PushObjects, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertMonster(9, 10);
		AssertTile(10, 10, T_MIRROR);
	}

	SECTION("Test Push Monsters Off") {
		CCharacter* pPushableCharacter = addPushableCharacter(10, 10);

		// Citizen character can push monster by default
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PushMonsters, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertMonster(9, 10);
		AssertMonster(10, 10);
	}
}
