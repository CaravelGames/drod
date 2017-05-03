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

static void DoSteppingTest(const UINT type, const char* title){
	SECTION(title){
		RoomBuilder::Plot(T_ORB, 10, 10);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 11, NO_ORIENTATION);
		RoomBuilder::AddMonster(M_BRAIN, 10, 6, NO_ORIENTATION);
		RoomBuilder::AddMonster(type, 10, 12, N);

		CCueEvents CueEvents;
		Runner::StartGame(10, 5, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_OrbActivatedByDouble));
	}
}

TEST_CASE("Aumtlich won't instakill cracked orbs", "[game]") {
	RoomBuilder::ClearRoom();
	
	RoomBuilder::Plot(T_ORB, 10, 10);
	RoomBuilder::AddOrbDataToTile(10, 10, OT_ONEUSE);
	RoomBuilder::AddMonster(M_AUMTLICH, 5, 10, SE);

	CCurrentGame *pGame = Runner::StartGame(20, 10, N);
	Runner::ExecuteCommand(CMD_WAIT);

	COrbData* pOrbData = pGame->pRoom->GetOrbAtCoords(10, 10);
	REQUIRE(pOrbData->eType == OT_BROKEN);
	REQUIRE(pGame->pRoom->GetTSquare(10, 10) == T_ORB);
}
