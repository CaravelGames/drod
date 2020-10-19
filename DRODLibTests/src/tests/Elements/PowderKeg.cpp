#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

TEST_CASE("Powder Keg", "[game][elements]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Player pushing a keg into a sword will explode the keg before anything else can move") {
		// .....M. Mimic facing anywhere
		// .B-K|.. Beethro facing East, Keg (10,10)
		// ....D.. Decoy facing North
		RoomBuilder::PlotToken(StaffToken, 8, 10);
		RoomBuilder::AddMonster(M_DECOY, 11, 11, N);
		RoomBuilder::AddMonster(M_DECOY, 12, 9, N);

		Runner::StartGame(8, 10, E);
		Runner::ExecuteCommand(E);

		AssertRoomHasNoMonster(M_MIMIC);
	}	
}
