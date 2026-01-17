#include "../../test-include.hpp"

TEST_CASE("Scripting: Smart Goto labels", "[game][scripting]") {
	RoomBuilder::ClearRoom();

	SECTION("Test Last If label") {
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_X, ScriptVars::Assign, 5);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_Y, ScriptVars::Assign, 5);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForVar, ScriptVars::P_SCRIPT_X, ScriptVars::Less, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 0, 0, 1, 1, T_BOMB);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_X, ScriptVars::Inc, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::PreviousIf);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);

		CCurrentGame* pGame = Runner::StartGame(2, 2, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// Check that a bomb is built on each tile
		REQUIRE(pGame->pRoom->GetTSquare(5, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(6, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(7, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(8, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(9, 5) == T_BOMB);
	}

	SECTION("Test Last If label with nesting") {
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_X, ScriptVars::Assign, 5);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForVar, ScriptVars::P_SCRIPT_X, ScriptVars::Less, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_Y, ScriptVars::Assign, 5);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForVar, ScriptVars::P_SCRIPT_Y, ScriptVars::Less, 10);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 0, 0, 1, 1, T_BOMB);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_Y, ScriptVars::Inc, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::PreviousIf);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_X, ScriptVars::Inc, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::PreviousIf);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);

		CCurrentGame* pGame = Runner::StartGame(2, 2, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// Check that a bomb is built on each tile
		for (int i = 5; i < 10; ++i) {
			for (int j = 5; j < 10; ++j) {
				REQUIRE(pGame->pRoom->GetTSquare(i, j) == T_BOMB);
			}
		}
	}

	SECTION("Test Next Else") {
		RoomBuilder::Plot(T_WALL, 3, 3);

		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForItem, 3, 3, 0, 0, T_WALL);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 5, 0, 0, T_BOMB);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfElse);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 6, 0, 0, T_MIRROR);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);

		CCurrentGame* pGame = Runner::StartGame(2, 2, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// Check that both items are built
		REQUIRE(pGame->pRoom->GetTSquare(5, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(5, 6) == T_MIRROR);
	}

	SECTION("Test Next Else If with condition skip") {
		RoomBuilder::Plot(T_WALL, 3, 3);

		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForItem, 3, 3, 0, 0, T_WALL);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 5, 0, 0, T_BOMB);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfElseIf);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForItem, 3, 4, 0, 0, T_WALL);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 6, 0, 0, T_MIRROR);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);

		CCurrentGame* pGame = Runner::StartGame(2, 2, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// Check that both items are built, even with Else If condition being false
		REQUIRE(pGame->pRoom->GetTSquare(5, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(5, 6) == T_MIRROR);
	}

	SECTION("Test Next Else and Else If") {
		RoomBuilder::Plot(T_WALL, 3, 3);

		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForItem, 3, 3, 0, 0, T_WALL);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 5, 0, 0, T_BOMB);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfElseIf);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForItem, 3, 4, 0, 0, T_WALL);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 6, 0, 0, T_MIRROR);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_GoTo, ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfElse);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 5, 7, 0, 0, T_POWDER_KEG);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_IfEnd);

		CCurrentGame* pGame = Runner::StartGame(2, 2, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// Check that both items are built, even with Else If condition being false
		REQUIRE(pGame->pRoom->GetTSquare(5, 5) == T_BOMB);
		REQUIRE(pGame->pRoom->GetTSquare(5, 6) == T_MIRROR);
		REQUIRE(pGame->pRoom->GetTSquare(5, 7) == T_POWDER_KEG);
	}
}
