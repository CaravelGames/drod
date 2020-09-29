#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../CAssert.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Slayers wielding daggers", "[game][slayer][weapon][dagger]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Slayer with a dagger won't be afraid to keep his dagger over a bomb"){
		// WWW - W=wall,
		// PSW - P=Beethro, S=Slayer
		// BWW - B=Bomb
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Dagger, 11, 11, S);
		RoomBuilder::PlotRect(T_WALL, 10, 10, 12, 10);
		RoomBuilder::PlotRect(T_WALL, 12, 10, 12, 12);
		RoomBuilder::Plot(T_WALL, 11, 12);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 11, W);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		AssertMonsterTypeO(10, 11, M_SLAYER, S);
		AssertPlayerIsDead();
	}
	
	SECTION("Slayer with a dagger won't turn while moving"){
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Staff, 10, 10, E);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(12, 8, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		AssertMonsterTypeO(10, 10, M_SLAYER, NE);
	}

	SECTION("Slayer with a dagger will kill-step normal monsters") {
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, S);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(12, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		AssertMonsterType(11, 10, M_SLAYER);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
	}

	SECTION("Slayer with a dagger won't kill-step guards") {
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Sword, 11, 10, S);

		RoomBuilder::PlotRect(T_WALL, 9, 9, 14, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 13, 10);
		RoomBuilder::Plot(T_FLOOR, 10, 11);
		RoomBuilder::Plot(T_BOMB, 10, 11);
		RoomBuilder::Plot(T_FLOOR, 12, 11);
		RoomBuilder::Plot(T_BOMB, 12, 11);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(12, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		CMonster* monster = game->pRoom->GetMonsterAtSquare(11, 10);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterDiedFromStab));
		REQUIRE(monster != NULL);
		REQUIRE(monster->wType == M_GUARD);
		REQUIRE(monster->GetWeaponType() == WT_Sword);
	}
}
