#include "../../../test-include.hpp"
#include "../../../CAssert.h"
#include <DRODLib/Slayer.h>

TEST_CASE("Slayer interaction with puffs", "[game][Slayer][puff]") {
	RoomBuilder::ClearRoom();

	SECTION("Slayer does not step on puff to kill player") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 11);

		RoomBuilder::AddMonster(M_SLAYER, 10, 10, S);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);

		CCueEvents CueEvents;
		Runner::StartGame(10, 12, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		//Puff should prevent Slayer moving south and killing player
		CHECK(!CueEvents.HasOccurred(CID_MonsterKilledPlayer));
		AssertMonsterType(10, 10, M_SLAYER);
		AssertMonsterType(10, 11, M_FLUFFBABY);
	}

	SECTION("Puff blocks Slayer pathing") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 12);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 12);

		RoomBuilder::AddMonster(M_SLAYER, 10, 10, S);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 12);

		CCueEvents CueEvents;
		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		//As the path is blocked, the slayer should move south
		AssertMonsterType(10, 11, M_SLAYER);
	}
}
