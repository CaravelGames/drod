#include "../../../test-include.hpp"
#include "../../../CAssert.h"

static void AddRemover() {
	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_REMOVE_TRANSPARENT);
}

static void CheckRemoval() {
	CCurrentGame* pGame = Runner::StartGame(5, 5, N);
	Runner::ExecuteCommand(CMD_WAIT, 1);

	// T-layer object should be removed
	CHECK(pGame->pRoom->GetTSquare(10, 10) == T_EMPTY);
}

static void CanRemoveTLayerObject(UINT wTile) {
	RoomBuilder::ClearRoom();
	RoomBuilder::Plot(wTile, 10, 10);

	AddRemover();
	CheckRemoval();
}

TEST_CASE("Test removal of single T-layer objects", "[game][building]") {
	SECTION("Test removing T_OBSTACLE") {
		CanRemoveTLayerObject(T_OBSTACLE);
	}
	SECTION("Test removing T_POTION_I") {
		CanRemoveTLayerObject(T_POTION_I);
	}
	SECTION("Test removing T_POTION_K") {
		CanRemoveTLayerObject(T_POTION_K);
	}
	SECTION("Test removing T_SCROLL") {
		CanRemoveTLayerObject(T_SCROLL);
	}
	SECTION("Test removing T_ORB") {
		CanRemoveTLayerObject(T_ORB);
	}
	SECTION("Test removing T_BOMB") {
		CanRemoveTLayerObject(T_BOMB);
	}
	SECTION("Test removing T_FUSE") {
		CanRemoveTLayerObject(T_FUSE);
	}
	SECTION("Test removing T_MIRROR") {
		CanRemoveTLayerObject(T_MIRROR);
	}
	SECTION("Test removing T_POTION_C") {
		CanRemoveTLayerObject(T_POTION_C);
	}
	SECTION("Test removing T_POTION_D") {
		CanRemoveTLayerObject(T_POTION_D);
	}
	SECTION("Test removing T_LIGHT") {
		CanRemoveTLayerObject(T_LIGHT);
	}
	SECTION("Test removing T_HORN_SQUAD") {
		CanRemoveTLayerObject(T_HORN_SQUAD);
	}
	SECTION("Test removing T_HORN_SOLDIER") {
		CanRemoveTLayerObject(T_HORN_SOLDIER);
	}
	SECTION("Test removing T_BEACON") {
		CanRemoveTLayerObject(T_BEACON);
	}
	SECTION("Test removing T_BEACON_OFF") {
		CanRemoveTLayerObject(T_BEACON_OFF);
	}
	SECTION("Test removing T_POWDER_KEG") {
		CanRemoveTLayerObject(T_POWDER_KEG);
	}

	SECTION("Test removing token") {
		RoomBuilder::ClearRoom();
		RoomBuilder::PlotToken(PowerTarget, 10, 10);

		AddRemover();
		CheckRemoval();
	}

	SECTION("Test removing station") {
		RoomBuilder::ClearRoom();
		RoomBuilder::PlotStation(10, 10, 1);

		AddRemover();
		CheckRemoval();
	}
}

TEST_CASE("Test removal of tarstuff T-layer objects", "[game][building][tarstuff]") {
	SECTION("Test removing entire blob") {
		RoomBuilder::ClearRoom();
		RoomBuilder::PlotRect(T_TAR, 10, 10, 11, 11);

		CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 1, 1, T_REMOVE_TRANSPARENT);

		CCurrentGame* pGame = Runner::StartGame(5, 5, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		// Tar should be removed, and no babies created
		CHECK(pGame->pRoom->GetTSquare(10, 10) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(10, 11) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(11, 10) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(11, 11) == T_EMPTY);
		AssertNoMonster(10, 10);
		AssertNoMonster(10, 11);
		AssertNoMonster(11, 10);
		AssertNoMonster(11, 11);
	}

	SECTION("Test removing piece from 2x2 blob") {
		RoomBuilder::ClearRoom();
		RoomBuilder::PlotRect(T_MUD, 10, 10, 11, 11);

		AddRemover();

		CCurrentGame* pGame = Runner::StartGame(5, 5, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		// Tar should be removed, with babies on tile that weren't built on
		CHECK(pGame->pRoom->GetTSquare(10, 10) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(10, 11) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(11, 10) == T_EMPTY);
		CHECK(pGame->pRoom->GetTSquare(11, 11) == T_EMPTY);
		AssertNoMonster(10, 10);
		AssertMonsterType(10, 11, M_MUDBABY);
		AssertMonsterType(11, 10, M_MUDBABY);
		AssertMonsterType(11, 11, M_MUDBABY);
	}
}
