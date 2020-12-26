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

TEST_CASE("Aumtlich gazes are done twice, second time are only meant to freeze doubles/players", "[game]") {
	//Every test cases has the same base structure
	// ..........
	// ......###.
	// ......#A#. - S-Facing Aumtlich on N-Arrow (moves first)
	// ..........
	// ...##.....
	// ...#A..X.. - A=E-Facing Aumtlich on W-Arrow (moves second), X=Cracked Orb at (10,10)
	// ...##.....
	// ..........
	RoomBuilder::ClearRoom();

	//Northern section
	RoomBuilder::PlotRect(T_WALL, 9, 6, 11, 7);
	RoomBuilder::Plot(T_FLOOR, 10, 7);
	RoomBuilder::Plot(T_ARROW_N, 10, 7);
	RoomBuilder::AddMonster(M_AUMTLICH, 10, 7, S);

	//Western section
	RoomBuilder::PlotRect(T_WALL, 6, 9, 7, 11);
	RoomBuilder::Plot(T_FLOOR, 7, 10);
	RoomBuilder::Plot(T_ARROW_W, 7, 10);
	RoomBuilder::AddMonster(M_AUMTLICH, 7, 10, E);
	
	RoomBuilder::Plot(T_ORB, 10, 10);
	RoomBuilder::AddOrbDataToTile(10, 10, OT_ONEUSE);

	SECTION("Second gaze pass will not damage cracked orbs") {
		RoomBuilder::Plot(T_ORB, 11, 10);
		RoomBuilder::Plot(T_ORB, 10, 11);
		RoomBuilder::AddOrbDataToTile(11, 10, OT_ONEUSE);
		RoomBuilder::AddOrbDataToTile(10, 11, OT_ONEUSE);

		CCurrentGame* pGame = Runner::StartGame(11, 11, SE);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertNoTile(10, 10, T_ORB);
		AssertOrbState(11, 10, OT_BROKEN);
		AssertOrbState(10, 11, OT_ONEUSE);
	}

	SECTION("Second gaze pass will freeze mimics") {
		RoomBuilder::AddMonster(M_MIMIC, 10, 11, SE);
		RoomBuilder::AddMonster(M_MIMIC, 11, 10, SE);

		CCurrentGame* pGame = Runner::StartGame(11, 11, SE);
		Runner::ExecuteCommand(CMD_WAIT);
		Runner::ExecuteCommand(CMD_SE); // Frozen mimics should not move

		AssertNoTile(10, 10, T_ORB);
		AssertMonster(11, 10);
		AssertMonster(10, 11);
	}

	SECTION("Second gaze pass will freeze player ") {
		RoomBuilder::AddMonster(M_MIMIC, 11, 10, SE);

		CCurrentGame* pGame = Runner::StartGame(10, 11, SE); // Place player on the path of the first gaze
		Runner::ExecuteCommand(CMD_WAIT);
		Runner::ExecuteCommand(CMD_SE); // Frozen player should not move

		AssertNoTile(10, 10, T_ORB);
		AssertMonster(11, 10);
		AssertPlayerAt(10, 11);
	}
}
