#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void TestPressurePlate(OrbType plateType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::ClearRoom();

		RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
		RoomBuilder::AddOrbDataToTile(10, 10, plateType);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, L"");

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

static void TestPressurePlateOutsideSWCorner(OrbType plateType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::ClearRoom();

		RoomBuilder::PlotRect(T_PRESSPLATE, 0, 0, 10, 10);
		RoomBuilder::AddOrbDataToTile(0, 0, plateType);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 2, 2, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, L"");

		CCueEvents CueEvents;
		Runner::StartGame(20, 20, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

static void TestRegularTile(UINT regularTile, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::ClearRoom();

		RoomBuilder::Plot(regularTile, 10, 10);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, L"");

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

TEST_CASE("Wait for pressure plate command works with one use plates", "[game]") {
	TestPressurePlate(OT_ONEUSE, T_PLATE_ONEUSE, true, "One-use plate");
	TestPressurePlate(OT_NORMAL, T_PLATE_ONEUSE, false, "One-use plate is not multi-use plate");
	TestPressurePlate(OT_TOGGLE, T_PLATE_ONEUSE, false, "One-use plate is not on-off plate");
}

TEST_CASE("Wait for pressure plate command works with multi use plates", "[game]") {
	TestPressurePlate(OT_ONEUSE, T_PLATE_MULTI, false, "Multi-use plate is not one-use plate");
	TestPressurePlate(OT_NORMAL, T_PLATE_MULTI, true, "Multi-use plate");
	TestPressurePlate(OT_TOGGLE, T_PLATE_MULTI, false, "Multi-use plate is not on-off plate");
}

TEST_CASE("Wait for pressure plate command works with on-off plates", "[game]") {
	TestPressurePlate(OT_ONEUSE, T_PLATE_ON_OFF, false, "On-off plate is not one-use plate");
	TestPressurePlate(OT_NORMAL, T_PLATE_ON_OFF, false, "On-off plate is not multi-use plate");
	TestPressurePlate(OT_TOGGLE, T_PLATE_ON_OFF, true, "On-off plate");
}

TEST_CASE("Wait for pressure plate command works with not-plates", "[game]") {
	const UINT tileTypes[] = { T_ORB, T_WALL, T_PIT, T_WATER, T_ARROW_N, T_ARROW_OFF_S, T_BOMB, T_FUSE, T_LIGHT };

	for (int i = 0; i < 9; i++){
		char firstName[100], secondName[100], thirdName[100];
		sprintf(firstName, "Tile #%d is not a one-use plate", tileTypes[i]);
		sprintf(secondName, "Tile #%d is not a multi plate", tileTypes[i]);
		sprintf(thirdName, "Tile #%d is not na on-of plate", tileTypes[i]);

		TestRegularTile(tileTypes[i], T_PLATE_ONEUSE, false, firstName);
		TestRegularTile(tileTypes[i], T_PLATE_MULTI, false, secondName);
		TestRegularTile(tileTypes[i], T_PLATE_ON_OFF, false, thirdName);
	}
}

TEST_CASE("Wait for pressure plate detects them even if you check non-corner tile", "[game]"){
	TestPressurePlateOutsideSWCorner(OT_ONEUSE, T_PLATE_ONEUSE, true, "One-use plate");
	TestPressurePlateOutsideSWCorner(OT_NORMAL, T_PLATE_MULTI, true, "Multi-use plate");
	TestPressurePlateOutsideSWCorner(OT_TOGGLE, T_PLATE_ON_OFF, true, "Toggle plate");
}