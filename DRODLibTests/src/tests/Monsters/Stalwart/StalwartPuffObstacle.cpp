#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Stalwart interaction with puffs", "[game][stalwart][puff]") {
	RoomBuilder::ClearRoom();
	CDbRoom* pRoom;

	SECTION("Puff hides monster from Stalwart") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 11);

		RoomBuilder::AddMonster(M_BRAIN, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::AddMonster(M_STALWART, 10, 15, N);

		Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT);

		//Stalwart should not move, and should not have a path to a monster
		pRoom = Runner::GetCurrentGame()->GetRoom();
		AssertMonsterType(10, 15, M_STALWART);
		CHECK(!pRoom->GetMonsterAtSquare(10,15)->ConfirmPath());
	}
}
