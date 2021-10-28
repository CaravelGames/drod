#include "../../test-include.hpp"

TEST_CASE("Waterskipper player role", "[game][waterskipper][player][player role]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_WATERSKIPPER);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ActivateItemAt, 11, 11);

	SECTION("Waterskipper clone pushed onto oremites should be live") {
		RoomBuilder::Plot(T_WATER, 10, 9);
		RoomBuilder::PlotRect(T_WATER, 15, 5, 15, 20);
		RoomBuilder::AddMonster(M_CLONE, 10, 10, N);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 10, 12, N);

		CCurrentGame* game = Runner::StartGame(15, 15, N);

		Runner::ExecuteCommand(CMD_N);

		AssertMonsterType(10, 9, M_CLONE);
		REQUIRE(game->GetDyingEntity() == NULL);
	}
}
