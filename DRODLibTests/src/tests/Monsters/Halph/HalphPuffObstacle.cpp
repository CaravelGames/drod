#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Halph interaction with puffs", "[game][Halph][puff]") {
	RoomBuilder::ClearRoom();

	SECTION("Puff hides orb from Halph") {
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 10, 11);

		RoomBuilder::Plot(T_ORB, 10, 10);
		RoomBuilder::Plot(T_DOOR_Y, 10, 15);
		RoomBuilder::AddOrbDataToTile(10, 10, OrbType::OT_NORMAL);
		RoomBuilder::LinkOrb(10, 10, 10, 15, OrbAgentType::OA_OPEN);

		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11);
		RoomBuilder::AddMonster(M_HALPH, 10, 13);
		
		CCueEvents CueEvents;
		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_S, CueEvents);

		//Halph should not move
		AssertMonsterType(10, 13, M_HALPH);
		CHECK(CueEvents.HasOccurred(CID_HalphCantOpen));
	}

	SECTION("Puff hides pressure plate from Halph") {
		RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
		RoomBuilder::Plot(T_DOOR_Y, 10, 15);
		RoomBuilder::AddOrbDataToTile(10, 10, OrbType::OT_NORMAL);
		RoomBuilder::LinkOrb(10, 10, 10, 15, OrbAgentType::OA_OPEN);

		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 10);
		RoomBuilder::AddMonster(M_HALPH, 10, 13);

		CCueEvents CueEvents;
		Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_S, CueEvents);

		//Halph should not move
		AssertMonsterType(10, 13, M_HALPH);
		CHECK(CueEvents.HasOccurred(CID_HalphCantOpen));
	}
}
