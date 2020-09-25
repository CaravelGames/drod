#include "../../../test-include.hpp"

static void TestShouldBuildStableTarstuff(const UINT tarType){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 1, 1, tarType);

	CCurrentGame* game = Runner::StartGame(15, 15, N);

	REQUIRE(game->pRoom->GetTSquare(10, 10) == tarType);
	REQUIRE(game->pRoom->GetTSquare(11, 10) == tarType);
	REQUIRE(game->pRoom->GetTSquare(10, 11) == tarType);
	REQUIRE(game->pRoom->GetTSquare(11, 11) == tarType);
}

static void TestShouldNotBuildTarstuffOverMonsters(const UINT tarType){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 1, 1, tarType);

	RoomBuilder::AddMonster(M_ROACH, 10, 10);
	RoomBuilder::AddMonster(M_ROACH, 11, 10);
	RoomBuilder::AddMonster(M_ROACH, 10, 11);
	RoomBuilder::AddMonster(M_ROACH, 11, 11);

	CCurrentGame* game = Runner::StartGame(15, 15, N);

	REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(11, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(10, 11) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(11, 11) == T_EMPTY);
}

static void TestShouldBreakIntoBabiesWhenUnstable(const UINT tarType, const UINT babyType){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 1, 0, tarType);

	CCurrentGame* game = Runner::StartGame(15, 15, N);

	REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetTSquare(11, 10) == T_EMPTY);
	REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10));
	REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10)->wType == babyType);
	REQUIRE(game->pRoom->GetMonsterAtSquare(11, 10));
	REQUIRE(game->pRoom->GetMonsterAtSquare(11, 10)->wType == babyType);
}

static void TestShouldNotBreakMotherSupportedTarStuff(const UINT tarType, const UINT motherType) {
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 12, 12, 1, 1, tarType);

	RoomBuilder::AddMonster(motherType, 10, 10);
	RoomBuilder::Plot(tarType, 10, 10);

	CCurrentGame* game = Runner::StartGame(15, 15, N);

	REQUIRE(game->pRoom->GetTSquare(10, 10) == tarType);
}

TEST_CASE("Scripting: Build tarstuff over monsters", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Build stable Tar"){
		TestShouldBuildStableTarstuff(T_TAR);
	}
	SECTION("Build stable Mud"){
		TestShouldBuildStableTarstuff(T_MUD);
	}
	SECTION("Build stable Gel"){
		TestShouldBuildStableTarstuff(T_GEL);
	}
	SECTION("Build stable Fluff"){
		TestShouldBuildStableTarstuff(T_FLUFF);
	}

	SECTION("Make it impossible to build Tar over monsters"){
		TestShouldNotBuildTarstuffOverMonsters(T_TAR);
	}

	SECTION("Make it impossible to build Mud over monsters"){
		TestShouldNotBuildTarstuffOverMonsters(T_MUD);
	}

	SECTION("Make it impossible to build Gel over monsters"){
		TestShouldNotBuildTarstuffOverMonsters(T_GEL);
	}

	SECTION("Make it impossible to build Fluff over monsters"){
		TestShouldNotBuildTarstuffOverMonsters(T_FLUFF);
	}

	SECTION("Break Tar when unstable on build"){
		TestShouldBreakIntoBabiesWhenUnstable(T_TAR, M_TARBABY);
	}
	SECTION("Break Mud when unstable on build"){
		TestShouldBreakIntoBabiesWhenUnstable(T_MUD, M_MUDBABY);
	}
	SECTION("Break Gel when unstable on build"){
		TestShouldBreakIntoBabiesWhenUnstable(T_GEL, M_GELBABY);
	}
	SECTION("Break Fluff when unstable on build"){
		TestShouldBreakIntoBabiesWhenUnstable(T_FLUFF, M_FLUFFBABY);
	}

	SECTION("Don't break mother supported Tar on build") {
		TestShouldNotBreakMotherSupportedTarStuff(T_TAR, M_TARMOTHER);
	}
	SECTION("Don't break mother supported Mud on build") {
		TestShouldNotBreakMotherSupportedTarStuff(T_MUD, M_MUDMOTHER);
	}
	SECTION("Don't break mother supported Gel on build") {
		TestShouldNotBreakMotherSupportedTarStuff(T_GEL, M_GELMOTHER);
	}
}

static void TestShouldToggleBlackGates(const UINT tarType){
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 0);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 1, 1, T_TAR);

	RoomBuilder::Plot(T_DOOR_B, 1, 1);
	RoomBuilder::Plot(T_DOOR_BO, 2, 2);

	CCurrentGame* game = Runner::StartGame(15, 15, N);
	Runner::ExecuteCommand(CMD_WAIT);

	REQUIRE(game->pRoom->GetOSquare(1, 1) == T_DOOR_B);
	REQUIRE(game->pRoom->GetOSquare(2, 2) == T_DOOR_BO);
}

TEST_CASE("Built tarstuff should toggle black gates", "[game]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Tar toggles black gates"){
		TestShouldToggleBlackGates(T_TAR);
	}
	SECTION("Mud toggles black gates"){
		TestShouldToggleBlackGates(T_MUD);
	}
	SECTION("Gel toggles black gates"){
		TestShouldToggleBlackGates(T_GEL);
	}
}