#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Stunned roach queen", "[game]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Won't lay eggs when player is stealthed and the queen is about to leave stun"){
		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, 1);
		RoomBuilder::PlotToken(StaffToken, 10, 10);
		RoomBuilder::AddMonster(M_QROACH, 9, 9);
		
		CCueEvents CueEvents;
		Runner::StartGame(10, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 28);
		Runner::ExecuteCommand(CMD_CC);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_EggSpawned));
	}
	
	SECTION("Won't lay eggs when player and temporal clone is stealthed and the queen is about to leave stun"){
		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerStealth, 1);
		RoomBuilder::PlotToken(StaffToken, 10, 10);
		RoomBuilder::PlotToken(TemporalSplit, 11, 11);
		RoomBuilder::AddMonster(M_QROACH, 9, 9);
		
		CCueEvents CueEvents;
		Runner::StartGame(12, 12, N);
		Runner::ExecuteCommand(CMD_NW);
		Runner::ExecuteCommand(CMD_SE);
		Runner::ExecuteCommand(CMD_WAIT, 30);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_NW);
		Runner::ExecuteCommand(CMD_WAIT, 26);
		Runner::ExecuteCommand(CMD_CC);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_EggSpawned));
	}
}
