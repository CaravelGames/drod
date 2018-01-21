#include "../../test-include.hpp"

TEST_CASE("Fegundo player role", "[game]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_FEGUNDO);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ActivateItemAt, 11, 11);

	SECTION("Stepping off ice tile"){
		RoomBuilder::Plot(T_THINICE, 10, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_S);

		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);
	}

	SECTION("Stepping off thin ice tile"){
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

	SECTION("Should not be killed by fire trap"){
		RoomBuilder::Plot(T_FIRETRAP_ON, 10, 10);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		CCueEvents CueEvents;
		Runner::ExecuteCommand(CMD_WAIT, 1);

		REQUIRE(game->GetDyingEntity() == NULL);
	}

	SECTION("Fegundo time clone should not be killed by fire trap") {
		RoomBuilder::Plot(T_FIRETRAP_ON, 10, 12);

		CTemporalClone* time_clone = DYN_CAST(
			CTemporalClone*, CMonster*, RoomBuilder::AddMonster(M_TEMPORALCLONE, 10, 10)
		);
		time_clone->wIdentity = M_FEGUNDO;
		std::vector<int> time_moves = { CMD_S, CMD_S, CMD_WAIT, CMD_WAIT };
		time_clone->InputCommands(time_moves);

		CCurrentGame* game = Runner::StartGame(5, 5, N);
		time_clone->eMovement = AIR;

		CCueEvents CueEvents;
		Runner::ExecuteCommand(CMD_WAIT, 2);

		REQUIRE(game->GetDyingEntity() == NULL);
	}
}
