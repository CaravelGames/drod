#include "../../catch.hpp"
#include "../../CAssert.h"
#include "../../CTestDb.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"
#include "../../../../DRODLib/CurrentGame.h"
#include "../../../../DRODLib/Character.h"
#include "../../../../DRODLib/CharacterCommand.h"

TEST_CASE("Player doubles", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Pushed doubles activate tokens") {
		RoomBuilder::Plot(T_ARROW_N, 1, 1);
		RoomBuilder::PlotToken(RotateArrowsCW, 10, 10);
		RoomBuilder::PlotToken(StaffToken, 9, 8);

		SECTION("Clone") {
			RoomBuilder::AddMonster(M_CLONE, 10, 9, E);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
		SECTION("Mimic") {
			RoomBuilder::AddMonster(M_MIMIC, 10, 9, E);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
		SECTION("Decoy") {
			RoomBuilder::AddMonster(M_DECOY, 10, 9, E);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
		SECTION("Clone character") {
			CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 9, E, M_CLONE);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
		SECTION("Mimic character") {
			CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 9, E, M_MIMIC);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
		SECTION("Decoy character") {
			CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 9, E, M_DECOY);
			Runner::StartGame(9, 8, E);
			Runner::ExecuteCommand(CMD_S);

			AssertTile(1, 1, T_ARROW_NE);
		}
	}
}
