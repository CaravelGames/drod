#include "../../../test-include.hpp"

// This
TEST_CASE("Tarstuff gates toggle bug", "[game]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerAppearance, M_FLUFFBABY);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ActivateItemAt, 11, 11);

	SECTION("Newly grown tar being destroyed by powder keg triggered by spikes should NOT result in any tarstuff gates being toggled") {
		// .B::: // Black gate closed with fuse on it, : = Walls
		// PksM: // Player, then keg, then spike trap (with force arrow E), then tar mother without tar underneath
		// .b::: // Black gate open, no fuse because it'd be removed by initial toggle anyway
		
		RoomBuilder::PlotRect(T_WALL, 9, 9, 3, 3);
		RoomBuilder::Plot(T_FLOOR_SPIKES, 9, 10);
		RoomBuilder::Plot(T_ARROW_E, 9, 10);
		RoomBuilder::Plot(T_FLOOR, 10, 10);
		RoomBuilder::AddMonster(M_TARMOTHER, 10, 10, NO_ORIENTATION);
		RoomBuilder::Plot(T_POWDER_KEG, 8, 10);
		RoomBuilder::Plot(T_DOOR_B, 8, 9);
		RoomBuilder::Plot(T_DOOR_BO, 8, 11);
		RoomBuilder::Plot(T_FUSE, 8, 9);

		CCurrentGame* game = Runner::StartGame(7, 10, S);
		Runner::ExecuteCommand(CMD_WAIT, 25);

		Runner::ExecuteCommand(CMD_E);
		Runner::ExecuteCommand(CMD_W);

		Runner::ExecuteCommand(CMD_WAIT, 3);

		// Ensure tarstuff gates are in their correct state, opposite from what was plotted because they're toggled on turn 0
		REQUIRE(game->pRoom->GetOSquare(8, 9) == T_DOOR_BO);
		REQUIRE(game->pRoom->GetOSquare(8, 11) == T_DOOR_B);

		// Ensure no door toggling happened at all
		REQUIRE(game->pRoom->GetTSquare(8, 9) == T_FUSE);
	}
}