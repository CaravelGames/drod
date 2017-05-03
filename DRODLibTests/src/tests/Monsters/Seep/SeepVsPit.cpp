#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Seep interacting with pits", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Seep pushed out of the wall and into a pit should fall into the pit immediately"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_PIT, 10, 9);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 12);
		RoomBuilder::AddMonster(M_SEEP, 10, 10);

		CCueEvents CueEvents;
		Runner::StartGame(10, 12, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_ObjectFell));
	}
}
