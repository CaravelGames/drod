#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Pushing Puffs and Fluff onto Tarstuff Mothers", "[game][fluff][puff][tarstuff]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddCharacter(1, 1, SW, M_CLONE);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerWeapon, WT_Staff);

	SECTION("Push Puff onto stable mother") {
		// Puffs can kill mothers when pused, but not the unlying tarstuff
		// Therefore, if the underlying tarstuff is stable, it will not be removed

		RoomBuilder::AddMonster(M_FLUFFBABY, 12, 10);
		RoomBuilder::AddMonster(M_TARMOTHER, 13, 10);
		RoomBuilder::PlotRect(T_TAR, 13, 10, 14, 11);

		CCurrentGame* pGame = Runner::StartGame(10, 10, E);

		// Player steps east, pushing puff on to tar mother
		Runner::ExecuteCommand(CMD_E);

		// Mother should be removed, but not the tar
		REQUIRE(!pGame->pRoom->GetMonsterAtSquare(13, 10));
		REQUIRE(pGame->pRoom->GetTSquare(13, 10) == T_TAR);
	}

	SECTION("Push Puff onto unstable mother") {
		// Tarstuff mothers can support otherwise unstable tarstuff
		// When killed by a puff push, that tarstuff should be removed

		RoomBuilder::AddMonster(M_FLUFFBABY, 12, 10);
		RoomBuilder::AddMonster(M_TARMOTHER, 13, 10);
		RoomBuilder::Plot(T_TAR, 13, 10);

		CCurrentGame* pGame = Runner::StartGame(10, 10, E);

		// Player steps east, pushing puff on to tar mother
		Runner::ExecuteCommand(CMD_E);

		// Mother and tar should be removed
		REQUIRE(!pGame->pRoom->GetMonsterAtSquare(13, 10));
		REQUIRE(pGame->pRoom->GetTSquare(13, 10) == T_EMPTY);
	}

	SECTION("Push Fluff onto stable mother") {
		// Fluff can kill mothers when pushed, but not the unlying tarstuff
		// Therefore, if the underlying tarstuff is stable, it will not be removed

		RoomBuilder::AddMonster(M_TARMOTHER, 10, 8);
		RoomBuilder::PlotRect(T_TAR, 10, 8, 11, 9);
		RoomBuilder::PlotRect(T_FLUFF, 8, 8, 9, 9);

		CCurrentGame* pGame = Runner::StartGame(10, 10, W);

		// Player rotates clockwise, pushing fluff onto tar mother
		Runner::ExecuteCommand(CMD_C);

		// Mother should be removed, but not the tar
		REQUIRE(!pGame->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(pGame->pRoom->GetTSquare(10, 8) == T_TAR);
	}

	SECTION("Push Fluff onto unstable mother") {
		// Tarstuff mothers can support otherwise unstable tarstuff
		// When killed by a fluff push, that tarstuff should be removed

		RoomBuilder::AddMonster(M_TARMOTHER, 10, 8);
		RoomBuilder::Plot(T_TAR, 10, 8);
		RoomBuilder::PlotRect(T_FLUFF, 8, 8, 9, 9);

		CCurrentGame* pGame = Runner::StartGame(10, 10, W);

		// Player rotates clockwise, pushing fluff onto tar mother
		Runner::ExecuteCommand(CMD_C);

		// Mother and tar should be removed
		REQUIRE(!pGame->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(pGame->pRoom->GetTSquare(10, 8) == T_EMPTY);
	}
}
