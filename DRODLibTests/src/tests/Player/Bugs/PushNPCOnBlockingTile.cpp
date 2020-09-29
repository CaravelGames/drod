#include "../../../test-include.hpp"

static void AddPushableCharacter(UINT x, UINT y) {
	CCharacter *character = RoomBuilder::AddVisibleCharacter(x, y);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByBody);
}

TEST_CASE("Player should push NPC on blocking tile", "[game][scripting][player][player moves][push][bug]") {
	RoomBuilder::ClearRoom();

	SECTION("Player should not move") {
		// Surround player with various obstacles, place pushable characters on top of them and then make player move onto each one
		// and making sure the thin ice under player is not dropped

		//WBL.
		//S*TT
		//.OTT
		//....
		RoomBuilder::Plot(T_THINICE, 10, 10); 
		RoomBuilder::Plot(T_WALL, 9, 9);              // NW of Player
		RoomBuilder::Plot(T_BOMB, 10, 9);             // N of Player
		RoomBuilder::Plot(T_LIGHT, 11, 9);            // NE of player
		RoomBuilder::Plot(T_STATION, 9, 10);          // E of player
		RoomBuilder::Plot(T_OBSTACLE, 10, 11);        // S of player
		RoomBuilder::PlotRect(T_TAR, 11, 10, 12, 11); // SE of player

		AddPushableCharacter(9, 9);   // NW of player 
		AddPushableCharacter(10, 9);  // N of player
		AddPushableCharacter(11, 9);  // NE of player
		AddPushableCharacter(9, 10);  // E of player
		AddPushableCharacter(10, 11); // S of player
		AddPushableCharacter(11, 11); // SE of player

		CCurrentGame* game = Runner::StartGame(10, 10, SW);

		Runner::ExecuteCommand(CMD_NW); 
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);

		Runner::ExecuteCommand(CMD_N);  
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);

		Runner::ExecuteCommand(CMD_NE);
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);

		Runner::ExecuteCommand(CMD_E);
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);

		Runner::ExecuteCommand(CMD_S);
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);

		Runner::ExecuteCommand(CMD_SE);
		REQUIRE(game->pRoom->GetOSquare(10, 10) == T_THINICE);
	}
}