#include "../../catch.hpp"
#include "../../CAssert.h"
#include "../../CTestDb.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"
#include "../../../../DRODLib/CurrentGame.h"
#include "../../../../DRODLib/Character.h"
#include "../../../../DRODLib/CharacterCommand.h"

void TestAlive() {
	Runner::StartGame(1, 1, SE);
	Runner::ExecuteCommand(CMD_WAIT);

	AssertMonsterType(10, 10, M_FEGUNDO);
}

void TestExploded() {
	Runner::StartGame(1, 1, SE);
	Runner::ExecuteCommand(CMD_WAIT);

	AssertMonsterType(10, 10, M_FEGUNDOASHES);
}

TEST_CASE("Fegundo", "[game]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotToken(RoomTokenType::PowerTarget, 1, 1);
	RoomBuilder::AddMonster(M_FEGUNDO, 10, 10, SE);

	SECTION("Fegundo flying into a wall should explode") {
		RoomBuilder::Plot(T_WALL, 11, 11);

		TestExploded();
	}

	SECTION("Fegundo flying into an opposite arrow should not explode") {
		RoomBuilder::Plot(T_ARROW_NW, 11, 11);
		TestAlive();
	}

	SECTION("Fegundo flying from an opposite arrow should not explode") {
		RoomBuilder::Plot(T_ARROW_NW, 10, 10);
		TestAlive();
	}

	SECTION("Fegundo flying diagonally onto ortho square should not explode") {
		RoomBuilder::Plot(T_NODIAGONAL, 11, 11);
		TestAlive();
	}

	SECTION("Fegundo flying diagonally from ortho square should not explode") {
		RoomBuilder::Plot(T_NODIAGONAL, 10, 10);
		TestAlive();
	}

	SECTION("Fegundo flying into gentryii chain...") {
		// ....
		// .F.. - Fegundo
		// ##C. - Gentryii Chain
		// #G.. - Gentryii surrounded by walls

		CGentryii* gentryii = DYN_CAST(CGentryii*, CMonster*, RoomBuilder::AddMonster(M_GENTRYII, 10, 12, SW));
		RoomBuilder::AddGentryiiPiece(gentryii, 11, 11);
		RoomBuilder::PlotRect(T_WALL, 9, 11, 10, 12);

		SECTION("Should explode if nothing else is there") {
			TestExploded();
		}
		SECTION("Should not explode if moving into blocking force arrow") {
			RoomBuilder::Plot(T_ARROW_NW, 11, 11);
			TestAlive();
		}
		SECTION("Should not explode if moving against blocking force arrow") {
			RoomBuilder::Plot(T_ARROW_NW, 10, 10);
			TestAlive();
		}
		SECTION("Should not explode if moving into ortho square") {
			RoomBuilder::Plot(T_NODIAGONAL, 11, 11);
			TestAlive();
		}
		SECTION("Should not explode if moving against ortho square") {
			RoomBuilder::Plot(T_NODIAGONAL, 10, 10);
			TestAlive();
		}
	}

	SECTION("Fegundo flying diagonally between gentryii chains...") {
		// .....
		// . FC. - Fegundo
		// ##C.. - Gentryii Chain
		// #G... - Gentryii surrounded by walls

		CGentryii* gentryii = DYN_CAST(CGentryii*, CMonster*, RoomBuilder::AddMonster(M_GENTRYII, 9, 12, SW));
		RoomBuilder::AddGentryiiPiece(gentryii, 10, 11);
		RoomBuilder::AddGentryiiPiece(gentryii, 11, 10);
		RoomBuilder::PlotRect(T_WALL, 8, 11, 9, 12);

		SECTION("Should explode if nothing else is there") {
			TestExploded();
		}
		SECTION("Should not explode if moving into blocking force arrow") {
			RoomBuilder::Plot(T_ARROW_NW, 11, 11);
			TestAlive();
		}
		SECTION("Should not explode if moving against blocking force arrow") {
			RoomBuilder::Plot(T_ARROW_NW, 10, 10);
			TestAlive();
		}
		SECTION("Should not explode if moving into ortho square") {
			RoomBuilder::Plot(T_NODIAGONAL, 11, 11);
			TestAlive();
		}
		SECTION("Should not explode if moving against ortho square") {
			RoomBuilder::Plot(T_NODIAGONAL, 10, 10);
			TestAlive();
		}
	}
}
