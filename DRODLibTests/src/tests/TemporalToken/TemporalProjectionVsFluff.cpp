#include "../../test-include.hpp"

TEST_CASE("Temporal projection vs Fluff", "[game]") {
	RoomBuilder::ClearRoom();

	RoomBuilder::AddMonster(M_FLUFFBABY, 10, 8, NO_ORIENTATION);
	RoomBuilder::PlotToken(RoomTokenType::TemporalSplit, 10, 10);

	SECTION("Construct role should not die"){
		CCharacter* character = RoomBuilder::AddCharacter(0, 0);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_CONSTRUCT);

		CCurrentGame* game = Runner::StartGame(10, 11, N);

		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_S);
		Runner::ExecuteCommand(CMD_WAIT);

		REQUIRE(game->GetKillingEntity() == NULL);
	}

	SECTION("Construct role should not alert other monsters"){
		CCharacter* character = RoomBuilder::AddCharacter(0, 0);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_CONSTRUCT);

		CCurrentGame* game = Runner::StartGame(10, 11, N);

		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_S);
		Runner::ExecuteCommand(CMD_WAIT);

		
		REQUIRE(Runner::GetTemporalClone(0));
		REQUIRE(Runner::GetTemporalClone(0)->bIsTarget == false);
	}
}
