#include "../../test-include.hpp"

TEST_CASE("Scripting: Go to level entrance", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Should go to level entrance on turn 1"){
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_LevelEntrance);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		Runner::ExecuteCommand(CMD_WAIT);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		
		REQUIRE(CueEvents.HasOccurred(CID_WinGame));
	}

	SECTION("Should not go to level entrance on turn 0") {
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_LevelEntrance);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 10, N, CueEvents);

		REQUIRE(!CueEvents.HasOccurred(CID_WinGame));
	}

	SECTION("Should not go to level entrance on turn 0 after double placement") {
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_LevelEntrance);
		RoomBuilder::Plot(T_POTION_K, 10, 10);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		Runner::PlaceDouble(15, 15, CueEvents);

		REQUIRE(!CueEvents.HasOccurred(CID_WinGame));
	}

	SECTION("Should go to level entrance after turn 0, execution on turn 0 should just suppress the script for a turn") {
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_LevelEntrance);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		REQUIRE(CueEvents.HasOccurred(CID_WinGame));
	}
}
