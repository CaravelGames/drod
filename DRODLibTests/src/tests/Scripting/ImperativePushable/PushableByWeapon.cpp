#include "../../../test-include.hpp"

static void AddPushableCharacter(UINT x, UINT y) {
	CCharacter *character = RoomBuilder::AddVisibleCharacter(x, y); 
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByWeapon); 
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerWeapon, WT_Dagger); 
}

TEST_CASE("Invulnerable character with 'Imperative: Pushable by weapon'", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	SECTION("Check for macro assumptions to be correct"){
		AddPushableCharacter(10, 9);
		CCurrentGame* game = Runner::StartGame(15, 15, N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
	}

	SECTION("Should bump when moving into the and the character should stay in place"){
		AddPushableCharacter(10, 9);
		CCurrentGame* game = Runner::StartGame(10, 10, N);
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8) == NULL);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->swordsman.wY == 10);
	}

	SECTION("Guard with a dagger should not be able to push the character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 7, 9, 9);
		RoomBuilder::PlotRect(T_WALL, 11, 7, 11, 9);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 8, S);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8)->wType == M_GUARD);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
	}

	SECTION("Stalwart with a dagger should not be able to push the character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 7, 9, 9);
		RoomBuilder::PlotRect(T_WALL, 11, 7, 11, 9);
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 8, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 10, NO_ORIENTATION);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 7));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 7)->wType == M_STALWART);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10)->wType == M_BRAIN);
	}

	SECTION("Soldier with a dagger should not be able to push the character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 7, 9, 9);
		RoomBuilder::PlotRect(T_WALL, 11, 7, 11, 9);
		RoomBuilder::AddMonsterWithWeapon(M_STALWART2, WT_Dagger, 10, 8, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 10, NO_ORIENTATION);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 7));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 7)->wType == M_STALWART2);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10)->wType == M_BRAIN);
	}

	SECTION("Guard with a dagger should not be able to kill the character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 7, S);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8)->wType == M_GUARD);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
	}

	SECTION("Character should be killed by spike trap"){
		AddPushableCharacter(10, 9);
		RoomBuilder::Plot(T_FLOOR_SPIKES, 10, 9);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 11);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9) == NULL);
	}

	SECTION("Character should be killed by fire trap"){
		AddPushableCharacter(10, 9);
		RoomBuilder::Plot(T_FIRETRAP_ON, 10, 9);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9) == NULL);
	}
	
	SECTION("Character should be killed by explosion"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_DECOY, 10, 7, S);
		RoomBuilder::Plot(T_BOMB, 10, 8);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9) == NULL);
	}
}
