#include "../../test-include.hpp"

TEST_CASE("Puff player role", "[game]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_FLUFFBABY);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ActivateItemAt, 11, 11);

	SECTION("Stepping off ice tile should not drop it"){
		RoomBuilder::Plot(T_THINICE, 10, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_S);

		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);
	}

	SECTION("Stepping off thin ice tile should not drop it"){
		RoomBuilder::Plot(T_THINICE_SH, 10, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_S);

		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE_SH);
	}

	SECTION("Should be safe from spike traps"){
		RoomBuilder::Plot(T_FLOOR_SPIKES, 10, 10);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		CCueEvents CueEvents;
		Runner::ExecuteCommand(CMD_WAIT, 10);

		REQUIRE(game->GetDyingEntity() == NULL);
	}

	SECTION("Puff time clone should be safe from spike traps") {
		RoomBuilder::Plot(T_FLOOR_SPIKES, 10, 12);
		RoomBuilder::PlotToken(TemporalSplit, 10, 11);

		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_S, 2);
		Runner::ExecuteCommand(CMD_WAIT, 10);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_WAIT, 8);

		REQUIRE(game->GetDyingEntity() == NULL);
	}

	SECTION("Should be killed by fire trap"){
		RoomBuilder::Plot(T_FIRETRAP_ON, 10, 10);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		CCueEvents CueEvents;
		Runner::ExecuteCommand(CMD_WAIT, 1);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}

	SECTION("Should die when moving onto hot tile") {
		RoomBuilder::Plot(T_HOT, 10, 11);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		CCueEvents CueEvents;
		Runner::ExecuteCommand(CMD_S, 1);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}
}
