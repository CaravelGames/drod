#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Slayers wielding different weapons", "[game]") {
	RoomBuilder::ClearRoom();
	
	
	SECTION("Slayer with a dagger won't be afraid to keep his dagger over a bomb"){
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Dagger, 11, 11, S);
		RoomBuilder::PlotRect(T_WALL, 10, 10, 12, 10);
		RoomBuilder::PlotRect(T_WALL, 12, 10, 12, 12);
		RoomBuilder::Plot(T_WALL, 11, 12);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 11, W);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11)->wO == S);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	

	
	SECTION("Slayer with a dagger won't turn while moving"){
		RoomBuilder::AddMonsterWithWeapon(M_SLAYER, WT_Staff, 10, 10, E);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(12, 8, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wO == NE);
	}
}
