#include "../../../catch.hpp"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

static void TestToken(RoomTokenType tokenType, UINT tileTypeToCheck, bool shouldPass, const char* title){
	SECTION(title){
		RoomBuilder::PlotToken(tokenType, 10, 10);

		CCharacter* character = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForItem, 10, 10, 0, 0, tileTypeToCheck);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

		CCueEvents CueEvents;
		Runner::StartGame(10, 25, N, CueEvents);
		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured == shouldPass);
	}
}

TEST_CASE("Wait for item: token command works", "[game]") {
	RoomBuilder::ClearRoom();

	TestToken(RotateArrowsCW, T_TOKEN_ROTATECW, true, "Rotate arrows CW");
	TestToken(RotateArrowsCCW, T_TOKEN_ROTATECCW, true, "Rotate arrows CCW");
	TestToken(SwitchTarMud, T_TOKEN_SWITCH_TARMUD, true, "Switch tar-mud");
	TestToken(SwitchTarGel, T_TOKEN_SWITCH_TARGEL, true, "Switch tar-get");
	TestToken(SwitchGelMud, T_TOKEN_SWITCH_GELMUD, true, "Switch gel-mud");
	TestToken(TarTranslucent, T_TOKEN_VISION, true, "Vision token");
	TestToken(PowerTarget, T_TOKEN_POWER, true, "Power target");
	TestToken(WeaponDisarm, T_TOKEN_DISARM, true, "Disarm");
	TestToken(PersistentCitizenMovement, T_TOKEN_PERSISTENTMOVE, true, "Persisten movement");
	TestToken(ConquerToken, T_TOKEN_CONQUER, true, "Conquer token");
	TestToken(SwordToken, T_TOKEN_WPSWORD, true, "Sword token");
	TestToken(PickaxeToken, T_TOKEN_WPPICKAXE, true, "Pickaxe token");
	TestToken(SpearToken, T_TOKEN_WPSPEAR, true, "Spear token");
	TestToken(StaffToken, T_TOKEN_WPSTAFF, true, "Staff token");
	TestToken(DaggerToken, T_TOKEN_WPDAGGER, true, "Dagger token");
	TestToken(CaberToken, T_TOKEN_WPCABER, true, "Caber token");
	TestToken(TemporalSplit, T_TOKEN_TSPLIT, true, "Temporal split (unused)");
	TestToken(TemporalSplitUsed, T_TOKEN_TSPLIT_USED, true, "Temporal split (used)");
}
