#include "../../test-include.hpp"
#include "../../CAssert.h"

namespace {
	void AddStabbingCharacter(UINT wX, UINT wY) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(wX, wY, N, M_MIMIC);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, -1);
	}
}

TEST_CASE("Scripting: Imperative - Vulnerable & Invulnerable", "[game]") {
	RoomBuilder::ClearRoom();
	CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1, NO_ORIENTATION);
	RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, StealthType::Stealth_Off);

	SECTION("Wubba") {
		SECTION("Wubba starts sword invulnerable by default") {
			RoomBuilder::AddVisibleCharacter(10, 10, NO_ORIENTATION, M_WUBBA);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertMonster(10, 10);
		}

		SECTION("Wubba set to imperative Vulnerable should be sword vulnerable") {
			pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, NO_ORIENTATION, M_WUBBA);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Vulnerable);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertNoMonster(10, 10);
		}

		SECTION("Wubba set to imperative Vulnerable then Invulnerable should be sword invulnerable") {
			pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, NO_ORIENTATION, M_WUBBA);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Vulnerable);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Invulnerable);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertMonster(10, 10);
		}
	}

	SECTION("Roach") {
		SECTION("Roach starts sword vulnerable by default") {
			RoomBuilder::AddVisibleCharacter(10, 10, N, M_ROACH);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertNoMonster(10, 10);
		}

		SECTION("Roach set to imperative Invulnerable should be sword invulnerable") {
			pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, N, M_ROACH);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Invulnerable);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertMonster(10, 10);
		}

		SECTION("Roach set to imperative Invulnerable then Vulnerable should be sword vulnerable") {
			pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, N, M_ROACH);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Invulnerable);
			RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Vulnerable);
			AddStabbingCharacter(10, 12);

			Runner::StartGame(30, 30);
			Runner::ExecuteCommand(CMD_WAIT);

			AssertNoMonster(10, 10);
		}
	}
}
