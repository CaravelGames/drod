#include "../../test-include.hpp"

TEST_CASE("Construct player role", "[game][construct][player][player role]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_CONSTRUCT);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ActivateItemAt, 11, 11);

	SECTION("Player-construct stepping onto oremites should be killed"){
		RoomBuilder::Plot(T_GOO, 10, 9);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_N);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}

	SECTION("Player-construct pushed onto oremites should be killed"){
		RoomBuilder::Plot(T_GOO, 10, 9);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Staff, 10, 12, N);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_WAIT);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}

	SECTION("Time clone construct stepping onto oremites should be killed") {
		RoomBuilder::Plot(T_GOO, 10, 9);

		CTemporalClone* time_clone = DYN_CAST(
			CTemporalClone*, CMonster*, RoomBuilder::AddMonster(M_TEMPORALCLONE, 10, 10)
		);
		time_clone->wIdentity = M_CONSTRUCT;
		std::vector<int> time_moves = { CMD_N, CMD_WAIT };
		time_clone->InputCommands(time_moves);

		CCurrentGame* game = Runner::StartGame(5, 5, N);
		time_clone->eMovement = GROUND_AND_SHALLOW_WATER;

		Runner::ExecuteCommand(CMD_WAIT);

		INFO("(" << time_clone->wX << ", " << time_clone->wY << ")");

		REQUIRE(game->GetDyingEntity() == time_clone);
	}

	SECTION("Player-construct should be vulnerable to body attack"){
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		RoomBuilder::AddMonster(M_ROACH, 10, 11, N);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_WAIT);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}
}
