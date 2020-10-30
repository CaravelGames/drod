#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Activate Tokens Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotToken(RoomTokenType::RotateArrowsCW, 11, 10);
	RoomBuilder::PlotToken(RoomTokenType::SwitchTarMud, 12, 10);
	RoomBuilder::Plot(T_ARROW_N, 8, 8);
	RoomBuilder::PlotRect(T_TAR, 5, 5, 6, 6);

	SECTION("Test Activate Tokens On") {
		// By default, non-human characters do not activate plates
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::ActivateTokens, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 2, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Check token effects occured
		AssertTile(8, 8, T_ARROW_NE);
		AssertTile(5, 5, T_MUD);
	}

	SECTION("Test Activate Tokens Off") {
		// By default, human characters do activate tokens
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::ActivateTokens, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 2, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Check token effects did not occur
		AssertTile(8, 8, T_ARROW_N);
		AssertTile(5, 5, T_TAR);
	}
}
