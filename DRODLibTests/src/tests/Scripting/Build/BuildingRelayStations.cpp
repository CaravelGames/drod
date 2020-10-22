#include "../../../test-include.hpp"
#include "../../../CAssert.h"
#include "DRODLib/Citizen.h"

TEST_CASE("Scripting: Build Relay Station", "[game][scripting][engineer][citizen]") {
	RoomBuilder::ClearRoom();

	CCharacter* pScript = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait);
	RoomBuilder::AddMonster(M_CITIZEN, 5, 5);

	SECTION("Build station with command") {
		RoomBuilder::AddCommand(
			pScript, CCharacterCommand::CC_Build, 5, 7, 0, 0, T_STATION);

		CCurrentGame* pGame = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 5);

		CDbRoom* pRoom = pGame->pRoom;
		CCitizen* pCitizen = dynamic_cast<CCitizen*>(
			pGame->pRoom->GetMonsterAtSquare(5, 6));

		AssertTile(5,7, T_STATION);
		CHECK(pRoom->GetTParam(5, 7) == 0);
		REQUIRE(pCitizen);
		CHECK(pCitizen->StationType() == 0);
	}

	SECTION("Build station with engineer") {
		RoomBuilder::AddMonster(M_ARCHITECT, 5, 8);
		RoomBuilder::AddCommand(
			pScript, CCharacterCommand::CC_BuildMarker, 5, 7, 0, 0, T_STATION);

		CCurrentGame* pGame = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 3);
		
		CDbRoom* pRoom = pGame->pRoom;
		CCitizen* pCitizen = dynamic_cast<CCitizen*>(
			pRoom->GetMonsterAtSquare(5, 6));

		AssertTile(5, 7, T_STATION);
		CHECK(pRoom->GetTParam(5, 7) == 0);
		REQUIRE(pCitizen);
		CHECK(pCitizen->StationType() == 0);
	}

	SECTION("Build station with citizen") {
		RoomBuilder::PlotRect(T_WALL, 4, 8, 6, 11);
		RoomBuilder::PlotRect(T_FLOOR, 5, 8, 5, 10);
		RoomBuilder::Plot(T_ARROW_N, 5, 8);
		RoomBuilder::PlotStation(5, 10, 3);

		RoomBuilder::AddMonster(M_CITIZEN, 5, 9);
		RoomBuilder::AddCommand(
			pScript, CCharacterCommand::CC_BuildMarker, 5, 7, 0, 0, T_STATION);

		CCurrentGame* pGame = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 5);

		CDbRoom* pRoom = pGame->pRoom;
		CCitizen* pCitizen = dynamic_cast<CCitizen*>(
			pRoom->GetMonsterAtSquare(5, 6));

		AssertTile(5, 7, T_STATION);
		CHECK(pRoom->GetTParam(5, 7) == 3);
		REQUIRE(pCitizen);
		CHECK(pCitizen->StationType() == 3);
	}
}
