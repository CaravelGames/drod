#include "../../test-include.hpp"
#include "../../CAssert.h"

TEST_CASE("Scripting: CountEntityType", "[game][scripting][imperative]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotRect(T_WALL, 10, 10, 15, 12);
	RoomBuilder::PlotRect(T_FLOOR, 11, 11, 14, 11);
	RoomBuilder::Plot(T_FLOOR, 13, 12);
	RoomBuilder::AddMonster(M_STALWART, 11, 11, E);
	RoomBuilder::AddMonster(M_EYE, 14, 11, E);

	CCharacter* pScript = RoomBuilder::AddCharacter(1,1);
	RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerStealth, Stealth_On);

	SECTION("Count roaches") {
		RoomBuilder::AddMonster(M_ROACH, 15, 15, E);
		RoomBuilder::AddMonster(M_EYE, 16, 15, E);
		RoomBuilder::AddMonster(M_GOBLIN, 17, 15, E);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 0, 0, 37, 31, M_ROACH);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}

	SECTION("Do not double count serpents if area both head and body") {
		CSerpent* serpent = DYN_CAST(CSerpent*, CMonster*, RoomBuilder::AddMonster(M_SERPENT, 10, 10, E));
		RoomBuilder::AddSerpentPiece(serpent, 9, 10);
		RoomBuilder::AddSerpentPiece(serpent, 8, 10);
		RoomBuilder::AddSerpentPiece(serpent, 7, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 0, 0, 37, 31, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}

	SECTION("Count serpent's head") {
		CSerpent* serpent = DYN_CAST(CSerpent*, CMonster*, RoomBuilder::AddMonster(M_SERPENT, 10, 10, E));
		RoomBuilder::AddSerpentPiece(serpent, 9, 10);
		RoomBuilder::AddSerpentPiece(serpent, 8, 10);
		RoomBuilder::AddSerpentPiece(serpent, 7, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 10, 10, 0, 0, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}

	SECTION("Count serpent's body") {
		CSerpent* serpent = DYN_CAST(CSerpent*, CMonster*, RoomBuilder::AddMonster(M_SERPENT, 10, 10, E));
		RoomBuilder::AddSerpentPiece(serpent, 9, 10);
		RoomBuilder::AddSerpentPiece(serpent, 8, 10);
		RoomBuilder::AddSerpentPiece(serpent, 7, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 7, 10, 0, 0, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}
}
