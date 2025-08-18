#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void TestWallLight(UINT lightType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::AddWallLight(10, 10, lightType);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, L"");

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

static void TestCeilingLight(UINT lightType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::AddCeilingLight(10, 10, lightType);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

static void TestCeilingDarkness(UINT lightType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::AddCeilingDarkness(10, 10, lightType);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

TEST_CASE("Wait for item light command works", "[game]") {
	RoomBuilder::ClearRoom();

	for (int i = 0; i < NUM_LIGHT_TYPES; i++){
		char name[100];
		sprintf(name, "Test for wall light, type #%d", i);
		TestWallLight(i, T_WALLLIGHT, true, name);
	}

	for (int i = 0; i < NUM_LIGHT_TYPES; i++){
		char name[100];
		sprintf(name, "Test for ceiling light, type #%d", i);
		TestCeilingLight(i, T_LIGHT_CEILING, true, name);
	}

	for (int i = 0; i < NUM_DARK_TYPES; i++){
		char name[100];
		sprintf(name, "Test for ceiling darkness, type #%d", i);
		TestCeilingDarkness(i, T_DARK_CEILING, true, name);
	}
}
