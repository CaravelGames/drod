#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

#include <vector>
using namespace std;

namespace {
	void AssertGreenGatesToggled() {
		AssertTile(1, 1, T_DOOR_GO);
		AssertTile(2, 1, T_DOOR_M);
	}
	void AssertGreenGatesNotToggled() {
		AssertTile(1, 1, T_DOOR_M);
		AssertTile(2, 1, T_DOOR_GO);
	}
}

TEST_CASE("Green Gate", "[game][elements]") {
	RoomBuilder::ClearRoom();

	RoomBuilder::Plot(T_DOOR_M, 1, 1);
	RoomBuilder::Plot(T_DOOR_GO, 2, 1);
	
	SECTION("Should toggle on turn 0 when cleared") {
		Runner::StartGame(10, 10);
		AssertGreenGatesToggled();
	}

	SECTION("Should not auto toggle on turn 1 when not cleared (has monster)") {
		RoomBuilder::AddMonster(M_ROACH, 20, 20);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_WAIT);
		AssertGreenGatesNotToggled();
	}

	SECTION("Should toggle when last monster is killed and there are no conquer tokens") {
		RoomBuilder::AddMonster(M_ROACH, 9, 9);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesToggled();
	}

	SECTION("Should not toggle when last monster is killed but there is a conquer token") {
		RoomBuilder::AddMonster(M_ROACH, 9, 9);
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 20, 20);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesNotToggled();
	}

	SECTION("Should not auto toggle on turn 1 when not cleared (has conquer token)") {
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 20, 20);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_WAIT);
		AssertGreenGatesNotToggled();
	}

	SECTION("Should toggle when conquer token is stepped on and there are no monsters") {
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 9, 10);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesToggled();
	}

	SECTION("Should not toggle when conquer token is stepped on and there is a conquer token") {
		RoomBuilder::AddMonster(M_ROACH, 20, 20);
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 9, 10);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesNotToggled();
	}

	SECTION("Should toggle when conquer token is stepped on and then last monster is killed") {
		RoomBuilder::AddMonster(M_ROACH, 9, 8);
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 9, 10);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesNotToggled();
		Runner::ExecuteCommand(CMD_N);
		AssertGreenGatesToggled();
	}

	SECTION("Should toggle when last monster is killed and then conquer token is stepped") {
		RoomBuilder::AddMonster(M_ROACH, 9, 9);
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 9, 9);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesNotToggled();
		Runner::ExecuteCommand(CMD_N);
		AssertGreenGatesToggled();
	}

	SECTION("Should toggle when last monster is killed and conquer token is stepped on the same move") {
		RoomBuilder::AddMonster(M_ROACH, 9, 9);
		RoomBuilder::PlotToken(RoomTokenType::ConquerToken, 9, 10);
		Runner::StartGame(10, 10);
		Runner::ExecuteCommand(CMD_W);
		AssertGreenGatesToggled();
	}
}
