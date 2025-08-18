#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Monster Targeting Behaviors", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CITIZEN1);

	SECTION("Test monster is attracted to character target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MonsterTarget, 1);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);
	
		CCurrentGame* pGame = Runner::StartGame(1, 1);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(9, 10);

		// Wait another turn and check character is not killed
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonsterType(9, 10, M_ROACH);
		AssertMonsterType(10, 10, M_CHARACTER);
	}

	SECTION("Test monster will kill vulnerable character target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MonsterTarget, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MonsterAttackable, 1);
		RoomBuilder::AddMonster(M_ROACH, 9, 10);

		CCurrentGame* pGame = Runner::StartGame(1, 1);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonsterType(10, 10, M_ROACH);
	}

	SECTION("Test Eye will see non-target character with eye-waking behavior") {
		// For simplicity, we will make this character move before monsters
		pCharacter->wProcessSequence = 100;
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::WakesEyes, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 1);
		RoomBuilder::AddMonster(M_EYE, 5, 11, E);

		CCurrentGame* pGame = Runner::StartGame(1, 11);
		Runner::ExecuteCommand(CMD_WAIT);

		// Eye should wake up and make move
		CHECK(pGame->pRoom->GetMonsterOfType(M_EYE)->IsAggressive());
		AssertMonster(4, 11);
	}

	SECTION("Test monster is not attracted to conditional character target until stealth is broken") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MonsterTargetWhenPlayerIsTarget, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);
		RoomBuilder::AddMonster(M_ROACH, 1, 3);
		RoomBuilder::AddMonster(M_ROACH, 8, 10);

		CCurrentGame* pGame = Runner::StartGame(1, 1, S);
		Runner::ExecuteCommand(CMD_WAIT);

		// Monsters should not move
		AssertMonster(1, 3);
		AssertMonster(8, 10);

		// Kill a monster to break stealth
		Runner::ExecuteCommand(CMD_S);

		AssertMonsterType(9, 10, M_ROACH);
	}

	SECTION("Test monster kills attackable non-target on way to target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::MonsterAttackable, 1);
		RoomBuilder::AddMonster(M_ROACH, 9, 10);

		CCurrentGame* pGame = Runner::StartGame(12, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonsterType(10, 10, M_ROACH);
	}

	SECTION("Test Stalwart kills unfriendly target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Unfriendly);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::AllyTarget, 1);
		RoomBuilder::AddMonster(M_STALWART, 8, 10, E);

		CCurrentGame* pGame = Runner::StartGame(1, 1);
		Runner::ExecuteCommand(CMD_WAIT);

		// Stalwart should move and kill target
		AssertMonsterType(9, 10, M_STALWART);
		AssertNoMonster(10, 10);
	}

	SECTION("Test Stalwart moves to but does not kill friendly target") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::AllyTarget, 1);
		RoomBuilder::AddMonster(M_STALWART, 8, 9, SE);

		CCurrentGame* pGame = Runner::StartGame(1, 1);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Stalwart should move but not kill target
		AssertMonsterType(9, 10, M_STALWART);
		AssertMonsterType(10, 10, M_CHARACTER);
	}
}
