#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Armed monsters striking when stunned", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Decoy should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_DECOY, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}

	SECTION("Mimic should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_MIMIC, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}

	SECTION("Guard should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_GUARD, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
	SECTION("Soldier should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_STALWART2, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
	SECTION("Stalwart should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_STALWART, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
	SECTION("39th Slayer should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_SLAYER, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
	SECTION("Slayer should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_SLAYER2, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
	SECTION("Clone should stab even if they are pushed into a wall"){
		RoomBuilder::Plot(T_WALL, 10, 10);
		RoomBuilder::Plot(T_ORB, 11, 11);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 13);
		RoomBuilder::AddMonster(M_CLONE, 10, 11, E);

		CCueEvents CueEvents;
		Runner::StartGame(10, 13, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
}
