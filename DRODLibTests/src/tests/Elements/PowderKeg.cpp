#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

TEST_CASE("Powder Keg", "[game][elements]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Player pushing a keg into a sword will explode the keg before anything else can move") {
		// .....M. Mimic facing anywhere
		// .B-K|.. Beethro with a stick facing East, Keg (10,10)
		// ....D.. Decoy facing North
		RoomBuilder::Plot(T_POWDER_KEG, 10, 10);
		RoomBuilder::PlotToken(StaffToken, 8, 10);
		RoomBuilder::AddMonster(M_DECOY, 11, 11, N);
		RoomBuilder::AddMonster(M_MIMIC, 12, 9, N);

		Runner::StartGame(8, 10, E);
		Runner::ExecuteCommand(E);

		AssertRoomHasNoMonster(M_MIMIC);
	}

	SECTION("Double pushing a keg into a sword will explode the keg before anything else can move") {
		// ......R. Roach queen
		// B.M-K|.. Beethro facing south | Mimic with a stick facing East | Keg (10,10)
		// .....D.. Decoy facing North
		RoomBuilder::Plot(T_POWDER_KEG, 10, 10);
		RoomBuilder::AddMonster(M_DECOY, 11, 11, N);
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 8, 10, E); // Moves first so the explosion will kill the other mimic
		RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Sword, 12, 9); // Using character here too to ensure processing order is maintained

		Runner::StartGame(6, 10, S);
		Runner::ExecuteCommand(E);

		AssertNoMonster(12, 9);
		AssertNoMonster(13, 9);
	}
}
