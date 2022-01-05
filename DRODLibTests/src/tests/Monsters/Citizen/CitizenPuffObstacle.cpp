#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Citizen interaction with puffs", "[game][citizen][puff]") {
	RoomBuilder::ClearRoom();

	SECTION("Puff hides build marker from Citizen") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 11);

		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::AddMonster(M_CITIZEN, 10, 13);
		RoomBuilder::PlotStation(10, 14, 0);

		CCharacter* pScript = RoomBuilder::AddCharacter(0, 0);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_BuildMarker, 10, 10, 0, 0, T_BOMB);

		Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		//Citizen should not have moved
		AssertMonsterType(10, 13, M_CITIZEN);
	}

	SECTION("Puff does not hide station from Citizen") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 11);

		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::AddMonster(M_CITIZEN, 10, 13);

		RoomBuilder::PlotStation(10, 10, 1);
		RoomBuilder::PlotStation(10, 14, 1);

		Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		//Citizen should have made a move towards the station
		AssertMonsterType(10, 12, M_CITIZEN);

		Runner::ExecuteCommand(CMD_WAIT);

		//Citizen should get stuck on puff
		AssertMonsterType(10, 12, M_CITIZEN);
		AssertMonsterType(10, 11, M_FLUFFBABY);
	}
}
