#include "../../../test-include.hpp"

TEST_CASE("Pushing monsters on hot tiles", "[game]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddCharacter(1, 1, SW, M_CLONE);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerWeapon, WT_Staff);

	RoomBuilder::PlotRect(T_HOT, 0, 10, 10, 10);


	SECTION("Beethro should stay alive"){
		RoomBuilder::AddMonster(M_ROACH, 11, 10, N);
		RoomBuilder::Plot(T_POTION_SP, 12, 10);

		CCurrentGame* game = Runner::StartGame(13, 10, W);

		Runner::ExecuteCommand(CMD_W, 2);
		Runner::ExecuteCommand(CMD_WAIT, 1);
		Runner::ExecuteCommand(CMD_W, 2);
		Runner::ExecuteCommand(CMD_WAIT, 1);
		REQUIRE(game->pRoom->pFirstMonster != NULL);
	}
}
