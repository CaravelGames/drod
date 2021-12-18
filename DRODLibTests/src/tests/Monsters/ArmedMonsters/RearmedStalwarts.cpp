#include "../../../catch.hpp"
#include "../../../CAssert.h"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Stalwarts wielding different weapons", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Stalwart with a dagger won't bother to turn around"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, W);
		RoomBuilder::AddMonster(M_BRAIN, 12, 10, NO_ORIENTATION);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}
	
	SECTION("Stalwart with a dagger will body-kill"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, W);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, NO_ORIENTATION);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}
	
	SECTION("Stalwart with a dagger will not body-kill if that would blow a bomb"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, W);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, NO_ORIENTATION);
		RoomBuilder::Plot(T_BOMB, 12, 10);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}
	
	SECTION("Stalwart with a caber on monster will stay in place to kill the monster"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Caber, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 11, NO_ORIENTATION);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}

	SECTION("Stalwart with a staff on monster will move forward"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Staff, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 11, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wType == M_STALWART);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 12));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 12)->wType == M_BRAIN);
	}
	
	SECTION("Stalwart with a pick-axe on monster will move forward"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Pickaxe, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 11, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wType == M_STALWART);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 12));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 12)->wType == M_BRAIN);
	}
	
	SECTION("Stalwart with a spear on monster will move into the monster to kill"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Spear, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 11, NO_ORIENTATION);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}
	
	SECTION("Stalwart with a spear on monster will move into the monster to kill, even if bomb is one tile further because it won't be stabbed"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Spear, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 11, NO_ORIENTATION);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		Runner::StartGame(30, 30, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}
	
	SECTION("Stalwart with a staff won't be afraid of attacking bombs"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Staff, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 14, NO_ORIENTATION);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11));
	}
	
	SECTION("Stalwart with a dagger will turn to face the direction he's moving"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 20, 10, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(11, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(11, 10)->wO == E);
	}
	
	SECTION("Stalwart with a dagger will turn to face the direction they tried to move"){
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_WALL, 11,10, 11, 10);
		RoomBuilder::PlotRect(T_WALL, 9, 11, 11, 11);
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wO == E);
	}
	
	SECTION("Stalwart with a dagger will turn to face the direction they tried to move even NW"){
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_WALL, 11,10, 11, 10);
		RoomBuilder::PlotRect(T_WALL, 9, 11, 11, 11);
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 9, 9, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wO == NW);
	}
	
	SECTION("Stalwart with a dagger will refuse to kill himself when closely surrounded by bombs"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, E);
		RoomBuilder::PlotRect(T_BOMB, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_BOMB, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_BOMB, 12,10, 12, 10);
		RoomBuilder::PlotRect(T_BOMB, 9, 11, 11, 11);
		RoomBuilder::AddMonster(M_BRAIN, 11, 11, NO_ORIENTATION);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(20, 20, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_BombExploded));
	}
	
	SECTION("Stalwart with a dagger bump-kill not blocked by bomb beyond target") {
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, W);
		RoomBuilder::AddMonster(M_WATERSKIPPER, 11, 10, E);

		RoomBuilder::Plot(T_WATER, 11, 10);
		RoomBuilder::Plot(T_BOMB, 12, 10);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(20, 20, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		AssertNoMonster(11, 10);
		AssertMonsterTypeO(10, 10, M_STALWART, E);
	}

	SECTION("Stalwart with a staff will push a powder keg"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Staff, 10, 10, S);
		RoomBuilder::AddMonster(M_BRAIN, 10, 20, NO_ORIENTATION);
		RoomBuilder::Plot(T_POWDER_KEG, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(30, 30, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11));
		REQUIRE(pGame->pRoom->GetTObject(10, 13));
	}
}