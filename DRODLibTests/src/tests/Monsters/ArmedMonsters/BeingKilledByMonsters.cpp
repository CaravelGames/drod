#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void TestIsKilled(const UINT defender, const UINT attacker, const bool requireKilling, const char* title){
	SECTION(title){
		RoomBuilder::AddMonster(defender, 10, 10, N);
		RoomBuilder::AddMonster(attacker, 10, 11, N);

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		bool defenderWasKilled = CueEvents.HasOccurred(CID_MonsterDiedFromStab);
		REQUIRE(requireKilling == defenderWasKilled);
	}
}

TEST_CASE("Armed friends attacked by regular monsters", "[game]") {
	RoomBuilder::ClearRoom();

	TestIsKilled(M_STALWART, M_ROACH, true, "Stalwart is attacked by roach");
	TestIsKilled(M_STALWART, M_GOBLIN, true, "Stalwart is attacked by goblin");
	TestIsKilled(M_STALWART, M_ROCKGOLEM, true, "Stalwart is attacked by rock golem");
	TestIsKilled(M_STALWART, M_SPIDER, true, "Stalwart is attacked by spider");
	TestIsKilled(M_STALWART, M_WUBBA, false, "Stalwart is not attacked by wubba");
}
