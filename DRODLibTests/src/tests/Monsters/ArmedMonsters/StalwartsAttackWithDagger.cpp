#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Armed friends attack monsters", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Stalwart with dagger targets a roach"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonster(M_ROACH, 10, 9, N);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 25, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		CMonster* monster = game->pRoom->GetMonsterAtSquare(10, 9);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
		REQUIRE(monster != NULL);
		REQUIRE(monster->wType == M_STALWART);
	}

	SECTION("Soldier with dagger targets a roach"){
		RoomBuilder::AddMonsterWithWeapon(M_STALWART2, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonster(M_ROACH, 10, 9, N);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(10, 25, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		CMonster* monster = game->pRoom->GetMonsterAtSquare(10, 9);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
		REQUIRE(monster != NULL);
		REQUIRE(monster->wType == M_STALWART2);
	}
}
