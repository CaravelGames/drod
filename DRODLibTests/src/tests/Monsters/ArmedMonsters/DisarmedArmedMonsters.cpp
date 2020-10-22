#include "../../../catch.hpp"
#include "../../../CAssert.h"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

namespace {
	void TestHasNoWeapon(const UINT wMonsterType) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, E, wMonsterType);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_VarSet, ScriptVars::P_MONSTER_WEAPON, ScriptVars::Assign, WT_Off);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_TurnIntoMonster);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, NO_ORIENTATION);

		Runner::StartGame(15, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonsterType(11, 10, M_BRAIN); // Brain must stay alive
	}
}

TEST_CASE("Disarmed armed monsters", "[game][armed-monster][disarm]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Can create disarmed mimic") {
		TestHasNoWeapon(M_MIMIC);
	}
	SECTION("Can create disarmed decoy") {
		TestHasNoWeapon(M_DECOY);
	}
	SECTION("Can create disarmed stalwart") {
		TestHasNoWeapon(M_STALWART);
	}
	SECTION("Can create disarmed soldier") {
		TestHasNoWeapon(M_STALWART2);
	}
	SECTION("Can create disarmed guard") {
		TestHasNoWeapon(M_GUARD);
	}
	SECTION("Can create disarmed slayer") {
		TestHasNoWeapon(M_SLAYER);
	}
	SECTION("Can create disarmed slayer2") {
		TestHasNoWeapon(M_SLAYER2);
	}
}
