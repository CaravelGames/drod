#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void DoPushingTest(const UINT type, const char* title){
	SECTION(title){
		RoomBuilder::Plot(T_ORB, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11, NO_ORIENTATION);
		RoomBuilder::AddMonster(type, 10, 12, N);
		RoomBuilder::PlotToken(RoomTokenType::StaffToken, 10, 14);

		CCueEvents CueEvents;
		Runner::StartGame(10, 14, N);
		Runner::ExecuteCommand(CMD_N, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
}

TEST_CASE("Armed monsters entering puff tile", "[game]") {
	RoomBuilder::ClearRoom();

	DoPushingTest(M_GUARD, "Guard pushed into puff should stab the tile beyond it");
	DoPushingTest(M_SLAYER, "39th Slayer pushed into puff should stab the tile beyond it");
	DoPushingTest(M_SLAYER2, "Slayer pushed into puff should stab the tile beyond it");
	DoPushingTest(M_STALWART, "Stalwart pushed into puff should stab the tile beyond it");
	DoPushingTest(M_STALWART2, "Soldier pushed into puff should stab the tile beyond it");
	DoPushingTest(M_MIMIC, "Mimic pushed into puff should stab the tile beyond it");
	DoPushingTest(M_DECOY, "Decoy pushed into puff should stab the tile beyond it");
}
