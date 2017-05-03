#include "../../../test-include.hpp"


static void AddPushableCharacter(UINT x, UINT y) {
	CCharacter *character = RoomBuilder::AddVisibleCharacter(x, y);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByWeapon);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForSomeoneToPushMe);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, L"");
}

TEST_CASE("Scripting command 'Wait for someone to push me'", "[scripting][push]") {
	RoomBuilder::ClearRoom();

	SECTION("Check for macro assumptions to be correct"){
		AddPushableCharacter(10, 9);
		CCurrentGame* game = Runner::StartGame(15, 15, N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
	}

	SECTION("Wait for someone to push me should execute after the character is no longer stunned"){
		CCueEvents CueEvents;
		AddPushableCharacter(10, 9);
		CCurrentGame* game = Runner::StartGame(10, 10, NE);
		Runner::ExecuteCommand(CMD_CC);
		Runner::ExecuteCommand(CMD_WAIT);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_ChallengeCompleted));
	}
}
