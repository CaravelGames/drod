#include "../../test-include.hpp"
#include "../../CAssert.h"

TEST_CASE("Scripting: CountEntityType", "[game][scripting][imperative]") {
	// SETUP:
	// ###### -- Located at (10,10)
	// #S##E# -- Stalwart & evil eye surrounded by walls
	// ######

	RoomBuilder::ClearRoom();
	RoomBuilder::PlotRect(T_WALL, 10, 10, 15, 12);
	RoomBuilder::Plot(T_FLOOR, 11, 11);
	RoomBuilder::Plot(T_FLOOR, 14, 11);
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
		RoomBuilder::AddLongMonster(M_SERPENT, 10, 10, E)
			.GrowIn(W, 3).End();
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 0, 0, 37, 31, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}

	SECTION("Count serpent's head") {
		RoomBuilder::AddLongMonster(M_SERPENT, 10, 10, E)
			.GrowIn(W, 3).End();
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 10, 10, 0, 0, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}

	SECTION("Count serpent's body") {
		RoomBuilder::AddLongMonster(M_SERPENT, 10, 10, E)
			.GrowIn(W, 3).End();
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_CountEntityType, 7, 10, 0, 0, M_SERPENT);

		CCurrentGame* pGame = Runner::StartGame(4, 4, E);
		CHECK(pGame->getVar(ScriptVars::P_RETURN_X) == 1);
	}
}
