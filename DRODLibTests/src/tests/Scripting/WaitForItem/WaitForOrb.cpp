#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void TestOrb(OrbType orbType, UINT tileTypeToCheck, bool shouldPass){
	RoomBuilder::ClearRoom();

	RoomBuilder::Plot(T_ORB, 10, 10);
	RoomBuilder::AddOrbDataToTile(10, 10, orbType);

	CCharacter* character = RoomBuilder::AddCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

	CCueEvents CueEvents;
	Runner::StartGame(10, 25, N, CueEvents);
	bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
	REQUIRE(eventOccured == shouldPass);
}

TEST_CASE("Wait for any orb detects normal orb", "[game]") {
	TestOrb(OT_NORMAL, T_ORB, true);
}
TEST_CASE("Wait for any orb detects cracked orb", "[game]") {
	TestOrb(OT_ONEUSE, T_ORB, true);
}
TEST_CASE("Wait for any orb detects broken orb", "[game]") {
	TestOrb(OT_BROKEN, T_ORB, true);
}

TEST_CASE("Wait for normal orb detects normal orb", "[game]") {
	TestOrb(OT_NORMAL, T_ORB_NORMAL, true);
}
TEST_CASE("Wait for normal any orb does not detect cracked orb", "[game]") {
	TestOrb(OT_ONEUSE, T_ORB_NORMAL, false);
}
TEST_CASE("Wait for normal orb does not detect broken orb", "[game]") {
	TestOrb(OT_BROKEN, T_ORB_NORMAL, false);
}

TEST_CASE("Wait for cracked orb does not detect normal orb", "[game]") {
	TestOrb(OT_NORMAL, T_ORB_CRACKED, false);
}
TEST_CASE("Wait for cracked any orb detects cracked orb", "[game]") {
	TestOrb(OT_ONEUSE, T_ORB_CRACKED, true);
}
TEST_CASE("Wait for cracked orb does not detect broken orb", "[game]") {
	TestOrb(OT_BROKEN, T_ORB_CRACKED, false);
}

TEST_CASE("Wait for broken orb does not detect normal orb", "[game]") {
	TestOrb(OT_NORMAL, T_ORB_BROKEN, false);
}
TEST_CASE("Wait for broken any orb does not detect cracked orb", "[game]") {
	TestOrb(OT_ONEUSE, T_ORB_BROKEN, false);
}
TEST_CASE("Wait for broken orb detects broken orb", "[game]") {
	TestOrb(OT_BROKEN, T_ORB_BROKEN, true);
}