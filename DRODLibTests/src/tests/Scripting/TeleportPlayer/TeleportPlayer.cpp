#include "../../../test-include.hpp"

static void Setup_CreateTeleportScript(){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 10, 10);
}

static void Setup_CreateTeleportScriptWithStaff(){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Staff);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 10, 10);
}

static void Setup_CreateMultipleTeleportScript(){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 10, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 11, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 12, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 13, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 14, 10);
}

static void Setup_CreateMultipleTeleportScriptTurn0(){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 10, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 11, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 12, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 13, 10);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TeleportPlayerTo, 14, 10);
}

static void CanTeleportOnto(const UINT tileToCheck, const bool canTeleport){
	char name[100];
	sprintf(name, "Player can teleport onto tile #%d", tileToCheck);

	SECTION(name){
		Setup_CreateTeleportScript();

		RoomBuilder::PlotRect(tileToCheck, 9, 9, 11, 11);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		if (canTeleport){
			REQUIRE(game->swordsman.wX == 10);
			REQUIRE(game->swordsman.wY == 10);
		}
		else {
			REQUIRE(game->swordsman.wX == 5);
			REQUIRE(game->swordsman.wY == 10);
		}
	}
}

TEST_CASE("Teleporting player", "") {
	RoomBuilder::ClearRoom();

	
	SECTION("Teleporting player shouldn't push monsters on the target tile"){
		Setup_CreateTeleportScriptWithStaff();

		RoomBuilder::Plot(T_ARROW_E, 10, 10);
		RoomBuilder::AddMonster(M_ROACH, 11, 10, 0);

		CCurrentGame* game = Runner::StartGame(5, 10, E);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 10);
		REQUIRE(game->pRoom->GetMonsterAtSquare(11, 10));
	}

	SECTION("Pit trapdoor shouldn't drop when teleporting off them"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_TRAPDOOR, 5, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_TRAPDOOR);
	}
	SECTION("Water trapdoor shouldn't drop when teleporting off them"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_TRAPDOOR2, 5, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_TRAPDOOR2);
	}
	SECTION("Thin ice shouldn't drop when teleporting off them"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_THINICE, 5, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_THINICE);
	}
	SECTION("Thin ice over shallow water shouldn't drop when teleporting off them"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_THINICE_SH, 5, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_THINICE_SH);
	}
	SECTION("Platform over pitwon't be moved when teleported off it"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_PLATFORM_P, 5, 10);
		RoomBuilder::Plot(T_PIT, 6, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_PLATFORM_P);
	}
	SECTION("Platform over water won't be moved when teleported off it"){
		Setup_CreateTeleportScript();

		RoomBuilder::Plot(T_PLATFORM_W, 5, 10);
		RoomBuilder::Plot(T_PIT, 6, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);
		REQUIRE(game->pRoom->GetOSquare(5, 10) == T_PLATFORM_W);
	}
	SECTION("All tokens teleported over to on turn 0 will be activated"){
		Setup_CreateMultipleTeleportScriptTurn0();

		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 10, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 11, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 12, 10);
		RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 13, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(5, 10, N, CueEvents);
		REQUIRE(bTokenActive(game->pRoom->GetTParam(10, 10)));
		REQUIRE(bTokenActive(game->pRoom->GetTParam(11, 10)));
		REQUIRE(bTokenActive(game->pRoom->GetTParam(12, 10)));
		REQUIRE(bTokenActive(game->pRoom->GetTParam(13, 10)));
	}
	SECTION("Teleporting onto multiple placing potions should use only the first one"){
		Setup_CreateMultipleTeleportScriptTurn0();

		RoomBuilder::Plot(T_POTION_K, 10, 10);
		RoomBuilder::Plot(T_POTION_K, 11, 10);
		RoomBuilder::Plot(T_POTION_K, 12, 10);
		RoomBuilder::Plot(T_POTION_K, 13, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(5, 10, N, CueEvents);
		REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
		REQUIRE(game->pRoom->GetTSquare(11, 10) == T_POTION_K);
		REQUIRE(game->pRoom->GetTSquare(12, 10) == T_POTION_K);
		REQUIRE(game->pRoom->GetTSquare(13, 10) == T_POTION_K);
		REQUIRE(game->swordsman.wPlacingDoubleType == M_MIMIC);
		REQUIRE(game->swordsman.wDoubleCursorX == 10);
		REQUIRE(game->swordsman.wDoubleCursorY == 10);
	}
	SECTION("Teleporting onto non-blocking potions when in double placing mode should still drink them"){
		Setup_CreateMultipleTeleportScriptTurn0();

		RoomBuilder::Plot(T_POTION_K, 10, 10);
		RoomBuilder::Plot(T_POTION_SP, 11, 10);
		RoomBuilder::Plot(T_POTION_I, 12, 10);
		RoomBuilder::Plot(T_HORN_SOLDIER, 13, 10);
		RoomBuilder::Plot(T_HORN_SQUAD, 14, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(5, 10, N, CueEvents);
		REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
		REQUIRE(game->pRoom->GetTSquare(11, 10) == T_EMPTY);
		REQUIRE(game->pRoom->GetTSquare(12, 10) == T_EMPTY);
		REQUIRE(game->pRoom->GetTSquare(13, 10) == T_EMPTY);
		REQUIRE(game->pRoom->GetTSquare(14, 10) == T_EMPTY);
	}
	SECTION("Teleporting onto water should kill the player and prevent further teleports"){
		Setup_CreateMultipleTeleportScript();

		RoomBuilder::Plot(T_PIT, 10, 10);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(5, 10, N, CueEvents);
		Runner::ExecuteCommand(CMD_WAIT, 1);
		REQUIRE(game->IsPlayerDying() == true);
		REQUIRE(game->swordsman.wX == 10);
		REQUIRE(game->swordsman.wY == 10);
	}
	SECTION("Teleporting onto monster should fail"){
		Setup_CreateTeleportScript();

		RoomBuilder::AddMonster(M_ROACH, 10, 10);
		RoomBuilder::Plot(T_ARROW_E, 10, 10);

		CCurrentGame* game = Runner::StartGame(5, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);
		REQUIRE(game->IsPlayerDying() == false);
		REQUIRE(game->swordsman.wX == 5);
		REQUIRE(game->swordsman.wY == 10);
	}

	CanTeleportOnto(T_EMPTY, true);
	CanTeleportOnto(T_FLOOR, true);
	CanTeleportOnto(T_PIT, true);
	CanTeleportOnto(T_STAIRS, true);
	CanTeleportOnto(T_WALL, true);
	CanTeleportOnto(T_WALL_B, true);
	CanTeleportOnto(T_DOOR_C, true);
	CanTeleportOnto(T_DOOR_M, true);
	CanTeleportOnto(T_DOOR_R, true);
	CanTeleportOnto(T_DOOR_Y, true);
	CanTeleportOnto(T_DOOR_YO, true);
	CanTeleportOnto(T_TRAPDOOR, true);
	CanTeleportOnto(T_OBSTACLE, true);
	CanTeleportOnto(T_ARROW_N, true);
	CanTeleportOnto(T_ARROW_NE, true);
	CanTeleportOnto(T_ARROW_E, true);
	CanTeleportOnto(T_ARROW_SE, true);
	CanTeleportOnto(T_ARROW_S, true);
	CanTeleportOnto(T_ARROW_SW, true);
	CanTeleportOnto(T_ARROW_W, true);
	CanTeleportOnto(T_ARROW_NW, true);
	CanTeleportOnto(T_POTION_I, true);
	CanTeleportOnto(T_POTION_K, true);
	CanTeleportOnto(T_SCROLL, true);
	CanTeleportOnto(T_ORB, true);
	CanTeleportOnto(T_TAR, false);
	CanTeleportOnto(T_DOOR_B, true);
	CanTeleportOnto(T_POTION_SP, true);
	CanTeleportOnto(T_BRIAR_SOURCE, false);
	CanTeleportOnto(T_BRIAR_DEAD, false);
	CanTeleportOnto(T_BRIAR_LIVE, false);
	CanTeleportOnto(T_BOMB, true);
	CanTeleportOnto(T_FUSE, true);
	CanTeleportOnto(T_NODIAGONAL, true);
	CanTeleportOnto(T_TUNNEL_N, true);
	CanTeleportOnto(T_TUNNEL_S, true);
	CanTeleportOnto(T_MIRROR, true);
	CanTeleportOnto(T_POTION_C, true);
	CanTeleportOnto(T_POTION_D, true);
	CanTeleportOnto(T_PLATFORM_W, true);
	CanTeleportOnto(T_PLATFORM_P, true);
	CanTeleportOnto(T_FLOOR_M, true);
	CanTeleportOnto(T_FLOOR_ROAD, true);
	CanTeleportOnto(T_FLOOR_GRASS, true);
	CanTeleportOnto(T_FLOOR_DIRT, true);
	CanTeleportOnto(T_FLOOR_ALT, true);
	CanTeleportOnto(T_WALL_M, false);
	CanTeleportOnto(T_MUD, false);
	CanTeleportOnto(T_STAIRS_UP, true);
	CanTeleportOnto(T_WALL_H, true);
	CanTeleportOnto(T_TUNNEL_E, true);
	CanTeleportOnto(T_TUNNEL_W, true);
	CanTeleportOnto(T_FLOOR_IMAGE, true);
	CanTeleportOnto(T_WALL2, true);
	CanTeleportOnto(T_WATER, true);
	CanTeleportOnto(T_DOOR_GO, true);
	CanTeleportOnto(T_DOOR_CO, true);
	CanTeleportOnto(T_DOOR_RO, true);
	CanTeleportOnto(T_DOOR_BO, true);
	CanTeleportOnto(T_TRAPDOOR2, true);
	CanTeleportOnto(T_GOO, true);
	CanTeleportOnto(T_LIGHT, false);
	CanTeleportOnto(T_HOT, true);
	CanTeleportOnto(T_GEL, false);
	CanTeleportOnto(T_STATION, true);
	CanTeleportOnto(T_PRESSPLATE, true);
	CanTeleportOnto(T_BRIDGE, true);
	CanTeleportOnto(T_BRIDGE_H, true);
	CanTeleportOnto(T_BRIDGE_V, true);
	CanTeleportOnto(T_PIT_IMAGE, true);
	CanTeleportOnto(T_WALL_IMAGE, true);
	CanTeleportOnto(T_SHALLOW_WATER, true);
	CanTeleportOnto(T_HORN_SQUAD, true);
	CanTeleportOnto(T_HORN_SOLDIER, true);
	CanTeleportOnto(T_STEP_STONE, true);
	CanTeleportOnto(T_BEACON, true);
	CanTeleportOnto(T_BEACON_OFF, true);
	CanTeleportOnto(T_POWDER_KEG, true);
	CanTeleportOnto(T_FLOOR_SPIKES, true);
	CanTeleportOnto(T_ARROW_OFF_N, true);
	CanTeleportOnto(T_ARROW_OFF_NE, true);
	CanTeleportOnto(T_ARROW_OFF_E, true);
	CanTeleportOnto(T_ARROW_OFF_SE, true);
	CanTeleportOnto(T_ARROW_OFF_S, true);
	CanTeleportOnto(T_ARROW_OFF_SW, true);
	CanTeleportOnto(T_ARROW_OFF_W, true);
	CanTeleportOnto(T_ARROW_OFF_NW, true);
	CanTeleportOnto(T_OVERHEAD_IMAGE, true);
	CanTeleportOnto(T_FLUFF, false);
	CanTeleportOnto(T_FLUFFVENT, true);
	CanTeleportOnto(T_THINICE, true);
	CanTeleportOnto(T_THINICE_SH, true);
	CanTeleportOnto(T_FIRETRAP, true);
	CanTeleportOnto(T_FIRETRAP_ON, true);
	CanTeleportOnto(T_WALL_WIN, false);
}
