#include "../../../test-include.hpp"


static void Setup_BuildScript(const UINT wTile){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 8, 8, 4, 4, wTile);
}

static void CanBuildOnPlayer(const UINT tileToBuild, const bool canBuild){
	RoomBuilder::ClearRoom();
	
	Setup_BuildScript(tileToBuild);

	CCurrentGame* game = Runner::StartGame(10, 10, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);

	const UINT wBaseTile = getBaseTile(tileToBuild);
	REQUIRE(game->pRoom->IsTileInRectOfType(10, 10, 10, 10, wBaseTile) == canBuild);
}


static void CanBuildOnTile(const UINT tileToBuild, const UINT underTile, const bool canBuild){
	RoomBuilder::ClearRoom();

	RoomBuilder::PlotRect(underTile, 8, 8, 12, 12);
	Setup_BuildScript(tileToBuild);

	CCurrentGame* game = Runner::StartGame(5, 10, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);

	const UINT wBaseTile = getBaseTile(tileToBuild);
	REQUIRE(game->pRoom->IsTileInRectOfType(10, 10, 10, 10, wBaseTile) == canBuild);
}

TEST_CASE("Testing can build T_FLOOR on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR, true);
}
TEST_CASE("Testing can build T_PIT on player", "[game]") {
	CanBuildOnPlayer(T_PIT, true);
}
TEST_CASE("Testing can build T_STAIRS on player", "[game]") {
	CanBuildOnPlayer(T_STAIRS, false);
}
TEST_CASE("Testing can build T_WALL on player", "[game]") {
	CanBuildOnPlayer(T_WALL, false);
}
TEST_CASE("Testing can build T_WALL_B on player", "[game]") {
	CanBuildOnPlayer(T_WALL_B, false);
}
TEST_CASE("Testing can build T_DOOR_C on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_C, true);
}
TEST_CASE("Testing can build T_DOOR_M on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_M, true);
}
TEST_CASE("Testing can build T_DOOR_R on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_R, true);
}
TEST_CASE("Testing can build T_DOOR_Y on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_Y, true);
}
TEST_CASE("Testing can build T_DOOR_YO on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_YO, true);
}
TEST_CASE("Testing can build T_TRAPDOOR on player", "[game]") {
	CanBuildOnPlayer(T_TRAPDOOR, true);
}
TEST_CASE("Testing can build T_OBSTACLE on player", "[game]") {
	CanBuildOnPlayer(T_OBSTACLE, false);
}
TEST_CASE("Testing can build T_ARROW_N on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_N, true);
}
TEST_CASE("Testing can build T_ARROW_NE on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_NE, true);
}
TEST_CASE("Testing can build T_ARROW_E on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_E, true);
}
TEST_CASE("Testing can build T_ARROW_SE on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_SE, true);
}
TEST_CASE("Testing can build T_ARROW_S on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_S, true);
}
TEST_CASE("Testing can build T_ARROW_SW on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_SW, true);
}
TEST_CASE("Testing can build T_ARROW_W on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_W, true);
}
TEST_CASE("Testing can build T_ARROW_NW on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_NW, true);
}
TEST_CASE("Testing can build T_POTION_I on player", "[game]") {
	CanBuildOnPlayer(T_POTION_I, true);
}
TEST_CASE("Testing can build T_POTION_K on player", "[game]") {
	CanBuildOnPlayer(T_POTION_K, true);
}
TEST_CASE("Testing can build T_SCROLL on player", "[game]") {
	CanBuildOnPlayer(T_SCROLL, true);
}
TEST_CASE("Testing can build T_ORB on player", "[game]") {
	CanBuildOnPlayer(T_ORB, true);
}
TEST_CASE("Testing can build T_TAR on player", "[game]") {
	CanBuildOnPlayer(T_TAR, false);
}
TEST_CASE("Testing can build T_CHECKPOINT on player", "[game]") {
	CanBuildOnPlayer(T_CHECKPOINT, false);
}
TEST_CASE("Testing can build T_DOOR_B on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_B, true);
}
TEST_CASE("Testing can build T_POTION_SP on player", "[game]") {
	CanBuildOnPlayer(T_POTION_SP, true);
}
TEST_CASE("Testing can build T_BRIAR_SOURCE on player", "[game]") {
	CanBuildOnPlayer(T_BRIAR_SOURCE, false);
}
TEST_CASE("Testing can build T_BRIAR_DEAD on player", "[game]") {
	CanBuildOnPlayer(T_BRIAR_DEAD, false);
}
TEST_CASE("Testing can build T_BRIAR_LIVE on player", "[game]") {
	CanBuildOnPlayer(T_BRIAR_LIVE, false);
}
TEST_CASE("Testing can build T_LIGHT_CEILING on player", "[game]") {
	CanBuildOnPlayer(T_LIGHT_CEILING, false);
}
TEST_CASE("Testing can build T_BOMB on player", "[game]") {
	CanBuildOnPlayer(T_BOMB, true);
}
TEST_CASE("Testing can build T_FUSE on player", "[game]") {
	CanBuildOnPlayer(T_FUSE, true);
}
TEST_CASE("Testing can build T_NODIAGONAL on player", "[game]") {
	CanBuildOnPlayer(T_NODIAGONAL, true);
}
TEST_CASE("Testing can build T_TOKEN on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN, false);
}
TEST_CASE("Testing can build T_TUNNEL_N on player", "[game]") {
	CanBuildOnPlayer(T_TUNNEL_N, true);
}
TEST_CASE("Testing can build T_TUNNEL_S on player", "[game]") {
	CanBuildOnPlayer(T_TUNNEL_S, true);
}
TEST_CASE("Testing can build T_MIRROR on player", "[game]") {
	CanBuildOnPlayer(T_MIRROR, true);
}
TEST_CASE("Testing can build T_POTION_C on player", "[game]") {
	CanBuildOnPlayer(T_POTION_C, true);
}
TEST_CASE("Testing can build T_POTION_D on player", "[game]") {
	CanBuildOnPlayer(T_POTION_D, true);
}
TEST_CASE("Testing can build T_PLATFORM_W on player", "[game]") {
	CanBuildOnPlayer(T_PLATFORM_W, false);
}
TEST_CASE("Testing can build T_PLATFORM_P on player", "[game]") {
	CanBuildOnPlayer(T_PLATFORM_P, false);
}
TEST_CASE("Testing can build T_FLOOR_M on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_M, true);
}
TEST_CASE("Testing can build T_FLOOR_ROAD on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_ROAD, true);
}
TEST_CASE("Testing can build T_FLOOR_GRASS on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_GRASS, true);
}
TEST_CASE("Testing can build T_FLOOR_DIRT on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_DIRT, true);
}
TEST_CASE("Testing can build T_FLOOR_ALT on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_ALT, true);
}
TEST_CASE("Testing can build T_WALL_M on player", "[game]") {
	CanBuildOnPlayer(T_WALL_M, false);
}
TEST_CASE("Testing can build T_MUD on player", "[game]") {
	CanBuildOnPlayer(T_MUD, false);
}
TEST_CASE("Testing can build T_STAIRS_UP on player", "[game]") {
	CanBuildOnPlayer(T_STAIRS_UP, false);
}
TEST_CASE("Testing can build T_WALL_H on player", "[game]") {
	CanBuildOnPlayer(T_WALL_H, false);
}
TEST_CASE("Testing can build T_TUNNEL_E on player", "[game]") {
	CanBuildOnPlayer(T_TUNNEL_E, true);
}
TEST_CASE("Testing can build T_TUNNEL_W on player", "[game]") {
	CanBuildOnPlayer(T_TUNNEL_W, true);
}
TEST_CASE("Testing can build T_FLOOR_IMAGE on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_IMAGE, true);
}
TEST_CASE("Testing can build T_WALL2 on player", "[game]") {
	CanBuildOnPlayer(T_WALL2, false);
}
TEST_CASE("Testing can build T_WATER on player", "[game]") {
	CanBuildOnPlayer(T_WATER, true);
}
TEST_CASE("Testing can build T_DOOR_GO on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_GO, true);
}
TEST_CASE("Testing can build T_DOOR_CO on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_CO, true);
}
TEST_CASE("Testing can build T_DOOR_RO on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_RO, true);
}
TEST_CASE("Testing can build T_DOOR_BO on player", "[game]") {
	CanBuildOnPlayer(T_DOOR_BO, true);
}
TEST_CASE("Testing can build T_TRAPDOOR2 on player", "[game]") {
	CanBuildOnPlayer(T_TRAPDOOR2, true);
}
TEST_CASE("Testing can build T_GOO on player", "[game]") {
	CanBuildOnPlayer(T_GOO, true);
}
TEST_CASE("Testing can build T_LIGHT on player", "[game]") {
	CanBuildOnPlayer(T_LIGHT, true);
}
TEST_CASE("Testing can build T_HOT on player", "[game]") {
	CanBuildOnPlayer(T_HOT, true);
}
TEST_CASE("Testing can build T_GEL on player", "[game]") {
	CanBuildOnPlayer(T_GEL, false);
}
TEST_CASE("Testing can build T_STATION on player", "[game]") {
	CanBuildOnPlayer(T_STATION, true);
}
TEST_CASE("Testing can build T_PRESSPLATE on player", "[game]") {
	CanBuildOnPlayer(T_PRESSPLATE, false);
}
TEST_CASE("Testing can build T_BRIDGE on player", "[game]") {
	CanBuildOnPlayer(T_BRIDGE, true);
}
TEST_CASE("Testing can build T_BRIDGE_H on player", "[game]") {
	CanBuildOnPlayer(T_BRIDGE_H, true);
}
TEST_CASE("Testing can build T_BRIDGE_V on player", "[game]") {
	CanBuildOnPlayer(T_BRIDGE_V, true);
}
TEST_CASE("Testing can build T_PIT_IMAGE on player", "[game]") {
	CanBuildOnPlayer(T_PIT_IMAGE, true);
}
TEST_CASE("Testing can build T_WALL_IMAGE on player", "[game]") {
	CanBuildOnPlayer(T_WALL_IMAGE, false);
}
TEST_CASE("Testing can build T_DARK_CEILING on player", "[game]") {
	CanBuildOnPlayer(T_DARK_CEILING, false);
}
TEST_CASE("Testing can build T_WALLLIGHT on player", "[game]") {
	CanBuildOnPlayer(T_WALLLIGHT, false);
}
TEST_CASE("Testing can build T_SHALLOW_WATER on player", "[game]") {
	CanBuildOnPlayer(T_SHALLOW_WATER, true);
}
TEST_CASE("Testing can build T_HORN_SQUAD on player", "[game]") {
	CanBuildOnPlayer(T_HORN_SQUAD, true);
}
TEST_CASE("Testing can build T_HORN_SOLDIER on player", "[game]") {
	CanBuildOnPlayer(T_HORN_SOLDIER, true);
}
TEST_CASE("Testing can build T_STEP_STONE on player", "[game]") {
	CanBuildOnPlayer(T_STEP_STONE, true);
}
TEST_CASE("Testing can build T_BEACON on player", "[game]") {
	CanBuildOnPlayer(T_BEACON, true);
}
TEST_CASE("Testing can build T_BEACON_OFF on player", "[game]") {
	CanBuildOnPlayer(T_BEACON_OFF, true);
}
TEST_CASE("Testing can build T_POWDER_KEG on player", "[game]") {
	CanBuildOnPlayer(T_POWDER_KEG, true);
}
TEST_CASE("Testing can build T_FLOOR_SPIKES on player", "[game]") {
	CanBuildOnPlayer(T_FLOOR_SPIKES, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_N on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_N, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_NE on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_NE, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_E on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_E, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_SE on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_SE, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_S on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_S, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_SW on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_SW, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_W on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_W, true);
}
TEST_CASE("Testing can build T_ARROW_OFF_NW on player", "[game]") {
	CanBuildOnPlayer(T_ARROW_OFF_NW, true);
}
TEST_CASE("Testing can build T_FLUFF on player", "[game]") {
	CanBuildOnPlayer(T_FLUFF, false);
}
TEST_CASE("Testing can build T_FLUFFVENT on player", "[game]") {
	CanBuildOnPlayer(T_FLUFFVENT, true);
}
TEST_CASE("Testing can build T_THINICE on player", "[game]") {
	CanBuildOnPlayer(T_THINICE, true);
}
TEST_CASE("Testing can build T_THINICE_SH on player", "[game]") {
	CanBuildOnPlayer(T_THINICE_SH, true);
}
TEST_CASE("Testing can build T_FIRETRAP on player", "[game]") {
	CanBuildOnPlayer(T_FIRETRAP, true);
}
TEST_CASE("Testing can build T_FIRETRAP_ON on player", "[game]") {
	CanBuildOnPlayer(T_FIRETRAP_ON, true);
}
TEST_CASE("Testing can build T_WALL_WIN on player", "[game]") {
	CanBuildOnPlayer(T_WALL_WIN, false);
}
TEST_CASE("Testing can build T_ACTIVETOKEN on player", "[game]") {
	CanBuildOnPlayer(T_ACTIVETOKEN, false);
}
TEST_CASE("Testing can build T_TOKEN_ROTATECW on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_ROTATECW, true);
}
TEST_CASE("Testing can build T_TOKEN_ROTATECCW on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_ROTATECCW, true);
}
TEST_CASE("Testing can build T_TOKEN_SWITCH_TARMUD on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_SWITCH_TARMUD, true);
}
TEST_CASE("Testing can build T_TOKEN_SWITCH_TARGEL on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_SWITCH_TARGEL, true);
}
TEST_CASE("Testing can build T_TOKEN_SWITCH_GELMUD on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_SWITCH_GELMUD, true);
}
TEST_CASE("Testing can build T_TOKEN_VISION on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_VISION, true);
}
TEST_CASE("Testing can build T_TOKEN_POWER on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_POWER, true);
}
TEST_CASE("Testing can build T_TOKEN_DISARM on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_DISARM, true);
}
TEST_CASE("Testing can build T_TOKEN_PERSISTENTMOVE on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_PERSISTENTMOVE, true);
}
TEST_CASE("Testing can build T_TOKEN_CONQUER on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_CONQUER, true);
}
TEST_CASE("Testing can build T_TOKEN_WPSWORD on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPSWORD, true);
}
TEST_CASE("Testing can build T_TOKEN_WPPICKAXE on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPPICKAXE, true);
}
TEST_CASE("Testing can build T_TOKEN_WPSPEAR on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPSPEAR, true);
}
TEST_CASE("Testing can build T_TOKEN_WPSTAFF on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPSTAFF, true);
}
TEST_CASE("Testing can build T_TOKEN_WPDAGGER on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPDAGGER, true);
}
TEST_CASE("Testing can build T_TOKEN_WPCABER on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_WPCABER, true);
}
TEST_CASE("Testing can build T_TOKEN_TSPLIT on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_TSPLIT, true);
}
TEST_CASE("Testing can build T_TOKEN_TSPLIT_USED on player", "[game]") {
	CanBuildOnPlayer(T_TOKEN_TSPLIT_USED, false);
}
TEST_CASE("Testing can build T_ORB_CRACKED on player", "[game]") {
	CanBuildOnPlayer(T_ORB_CRACKED, true);
}
TEST_CASE("Testing can build T_ORB_BROKEN on player", "[game]") {
	CanBuildOnPlayer(T_ORB_BROKEN, true);
}
TEST_CASE("Testing can build T_PLATE_ONEUSE on player", "[game]") {
	CanBuildOnPlayer(T_PLATE_ONEUSE, false);
}
TEST_CASE("Testing can build T_PLATE_MULTI on player", "[game]") {
	CanBuildOnPlayer(T_PLATE_MULTI, false);
}
TEST_CASE("Testing can build T_PLATE_ON_OFF on player", "[game]") {
	CanBuildOnPlayer(T_PLATE_ON_OFF, false);
}
TEST_CASE("Testing can build T_PIT on T_FLOOR", "[game]") {
	CanBuildOnTile(T_FLOOR, T_PIT, true);
}





TEST_CASE("Testing can build T_TAR on T_MUD", "[game]") {
	CanBuildOnTile(T_TAR, T_MUD, false);
}
TEST_CASE("Testing can build T_TAR on T_GEL", "[game]") {
	CanBuildOnTile(T_TAR, T_GEL, false);
}
TEST_CASE("Testing can build T_MUD on T_TAR", "[game]") {
	CanBuildOnTile(T_MUD, T_TAR, false);
}
TEST_CASE("Testing can build T_MUD on T_GEL", "[game]") {
	CanBuildOnTile(T_MUD, T_GEL, false);
}
TEST_CASE("Testing can build T_GEL on T_TAR", "[game]") {
	CanBuildOnTile(T_GEL, T_TAR, false);
}
TEST_CASE("Testing can build T_GEL on T_MUD", "[game]") {
	CanBuildOnTile(T_GEL, T_MUD, false);
}
TEST_CASE("Testing can build T_TAR on T_ORB", "[game]") {
	CanBuildOnTile(T_TAR, T_ORB, false);
}
TEST_CASE("Testing can build T_MUD on T_ORB", "[game]") {
	CanBuildOnTile(T_MUD, T_ORB, false);
}
TEST_CASE("Testing can build T_GEL on T_ORB", "[game]") {
	CanBuildOnTile(T_GEL, T_ORB, false);
}
TEST_CASE("No errors or asserts when building outside room edge", "[game]"){
	RoomBuilder::ClearRoom();
	
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 40, -1, 4, 4, T_WALL);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, -1, 40, 4, 4, T_WALL);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, -1, 5, 4, 4, T_WALL);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 5, -1, 4, 4, T_WALL);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, -1, -1, 4, 4, T_WALL);

	Runner::StartGame(5, 10, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);
	
	REQUIRE(Runner::GetNewAssertsCount() == 0);
}

TEST_CASE("Building crops top-left edge", "[game]"){
	RoomBuilder::ClearRoom();
	
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, -1, -1, 1, 1, T_WALL);

	CCurrentGame* game = Runner::StartGame(5, 10, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);
	
	REQUIRE(game->pRoom->GetOSquare(0, 0) == T_WALL);
	REQUIRE(game->pRoom->GetOSquare(1, 0) == T_FLOOR);
	REQUIRE(game->pRoom->GetOSquare(0, 1) == T_FLOOR);
	REQUIRE(game->pRoom->GetOSquare(1, 1) == T_FLOOR);
}

TEST_CASE("Disallow building orbs on pressure plates", "[game]"){
	RoomBuilder::ClearRoom();
	
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_ORB);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 11, 10, 0, 0, T_ORB_BROKEN);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 12, 10, 0, 0, T_ORB_CRACKED);
	RoomBuilder::PlotRect(T_PRESSPLATE, 10, 10, 12, 10);

	CCurrentGame* game = Runner::StartGame(5, 10, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);
	
	REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(11, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(12, 10) == T_EMPTY);
}