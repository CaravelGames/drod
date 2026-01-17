#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: NPC Beethro Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_BEETHRO);

	SECTION("Test Beethro character is not monster target when player is target") {
		RoomBuilder::AddMonster(M_ROACH, 9, 10);

		CCurrentGame* pGame = Runner::StartGame(7, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should move towards player
		AssertMonster(8, 10);
	}

	SECTION("Test Beethro character is monster target when player is not target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);

		CCurrentGame* pGame = Runner::StartGame(7, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should move towards character
		AssertMonster(9, 10);
	}

	SECTION("Test Gunthro character is monster target when player is not target") {
		pCharacter->wIdentity = M_GUNTHRO;
		pCharacter->wLogicalIdentity = M_GUNTHRO;
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);

		CCurrentGame* pGame = Runner::StartGame(7, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should move towards character
		AssertMonster(9, 10);
	}

	SECTION("Test Beethro character is not monster target when player is target and hiding") {
		RoomBuilder::AddMonster(M_ROACH, 8, 10);
		RoomBuilder::Plot(T_SHALLOW_WATER, 20, 20);

		CCurrentGame* pGame = Runner::StartGame(20, 20, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should not move
		AssertMonster(8, 10);
	}

	SECTION("Test that first Beethro character is always target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);
		RoomBuilder::AddVisibleCharacter(7, 10, 0, M_BEETHRO);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should move towards first character
		AssertMonster(9, 10);
	}

	SECTION("Test that NPC Beethro behavior can be disabled") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::CanBeNPCBeethro, 0);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);

		CCurrentGame* pGame = Runner::StartGame(7, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monster should not move
		AssertMonster(8, 10);
	}
}
