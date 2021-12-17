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

	SECTION("Player-construct should be vulnerable to body attack"){
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 11);
		RoomBuilder::AddMonster(M_ROACH, 10, 11, N);
		CCurrentGame* game = Runner::StartGame(10, 10, N);

		Runner::ExecuteCommand(CMD_WAIT);

		REQUIRE(game->GetDyingEntity() == &(game->swordsman));
	}

	SECTION("Construct clone pushed onto oremites should be killed"){
		RoomBuilder::Plot(T_GOO, 10, 9);
		RoomBuilder::AddMonster(M_CLONE, 10, 10, N);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 10, 12, N);

		CCurrentGame* game = Runner::StartGame(15, 15, N);

		Runner::ExecuteCommand(CMD_N);

		REQUIRE(game->GetDyingEntity() == game->pRoom->GetMonsterAtSquare(10,9));
	}
}
