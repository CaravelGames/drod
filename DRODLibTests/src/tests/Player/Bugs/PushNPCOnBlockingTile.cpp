#include "../../../test-include.hpp"

static void AddPushableCharacter(UINT x, UINT y) {
	CCharacter *character = RoomBuilder::AddVisibleCharacter(x, y);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByBody);
}

TEST_CASE("Player should push NPC on blocking tile", "[game][scripting][player][player moves][push][bug]") {
	SECTION("Player should not move") {
		//WBL.
		//S*TT
		//.OTT
		//....
		RoomBuilder::Plot(T_THINICE, 10, 10);
		RoomBuilder::Plot(T_WALL, 9, 9);
		RoomBuilder::Plot(T_BOMB, 10, 9);
		RoomBuilder::Plot(T_LIGHT, 11, 9);
		RoomBuilder::Plot(T_STATION, 9, 10);
		RoomBuilder::Plot(T_OBSTACLE, 10, 11);
		RoomBuilder::PlotRect(T_TAR, 11, 10, 12, 11);

		AddPushableCharacter(9, 9);
		AddPushableCharacter(10, 9);
		AddPushableCharacter(11, 9);
		AddPushableCharacter(9, 10);
		AddPushableCharacter(10, 11);
		AddPushableCharacter(9, 9);
		AddPushableCharacter(11, 11);

		CCurrentGame* game = Runner::StartGame(10, 10, S);

		Runner::ExecuteCommand(CMD_NW);
		Runner::ExecuteCommand(CMD_N);
		Runner::ExecuteCommand(CMD_NE);
		Runner::ExecuteCommand(CMD_E);
		Runner::ExecuteCommand(CMD_S);
		Runner::ExecuteCommand(CMD_SW);

		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE_SH);
	}
}