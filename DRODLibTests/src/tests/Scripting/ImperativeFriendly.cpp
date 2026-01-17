#include "../../test-include.hpp"
#include "../../CAssert.h"

TEST_CASE("Scripting: Imperative Friendly/Unfriendly", "[game][scripting][imperative]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotRect(T_WALL, 10, 10, 15, 12);
	RoomBuilder::PlotRect(T_FLOOR, 11, 11, 14, 11);
	RoomBuilder::Plot(T_FLOOR, 13, 12);
	RoomBuilder::AddMonster(M_STALWART, 11, 11, E);
	RoomBuilder::AddMonster(M_EYE, 14, 11, E);

	CCharacter* pScript = RoomBuilder::AddCharacter(1,1);
	RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);

	SECTION("Test marking non-deadly NPCs as unfriendly") {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(5, 5, 0, M_CITIZEN1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Unfriendly);
		pCharacter = RoomBuilder::AddVisibleCharacter(13, 11, 0, M_CITIZEN1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Unfriendly);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		Runner::ExecuteCommand(CMD_S);

		// Killing unfriendly NPC should remove player stealth
		CHECK(pGame->swordsman.wStealth == 0);
		
		// Stalwart should move towards target at (14,11) and stab unfriendly NPC
		AssertMonster(12, 11);
		AssertNoMonster(13, 11);
	}

	SECTION("Test marking deadly NPCs as friendly") {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(5, 5, 0, M_CITIZEN1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Deadly);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Friendly);
		pCharacter = RoomBuilder::AddVisibleCharacter(13, 11, 0, M_CITIZEN1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Deadly);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Friendly);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		Runner::ExecuteCommand(CMD_S);

		// Killing friendly NPC should not remove player stealth
		CHECK(pGame->swordsman.wStealth == 1);

		// Stalwart should not move towards target at (14,11), since that would stab a friendly NPC
		AssertMonster(11, 11);
		AssertMonster(13, 11);
	}
}
