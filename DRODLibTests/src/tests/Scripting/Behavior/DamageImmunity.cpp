#include "../../../test-include.hpp"
#include "../../../CAssert.h"

static void TestUnsafePushPrevented(const UINT identity, const UINT tileType) {
	string name =
		"Character with identity #" +
		std::to_string(identity) +
		" should not be pushed to tile #" +
		std::to_string(tileType);

	SECTION(name) {
		RoomBuilder::Plot(tileType, 15, 11);
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(15, 10, S, identity);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::FatalPushImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Staff);

		CCurrentGame* pGame = Runner::StartGame(15, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(15, 10);
	}
}

static void TestSafePushAllowed(const UINT identity, const UINT tileType) {
	string name =
		"Character with identity #" +
		std::to_string(identity) +
		" should be pushed to tile #" +
		std::to_string(tileType);

	SECTION(name) {
		RoomBuilder::Plot(tileType, 15, 11);
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(15, 10, S, identity);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::FatalPushImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Staff);

		CCurrentGame* pGame = Runner::StartGame(15, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(15, 11);
	}
}

TEST_CASE("Scripting: Damage Immunity Behaviors", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10);

	SECTION("Test Sword Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::SwordDamageImmune, 1);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(10, 10);
	}

	SECTION("Test Pickaxe Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PickaxeDamageImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Pickaxe);

		CCurrentGame* pGame = Runner::StartGame(10, 9, SE);
		Runner::ExecuteCommand(CMD_C);

		AssertMonster(10, 10);
	}

	SECTION("Test Spear Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::SpearDamageImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Spear);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(10, 10);
	}

	SECTION("Test Dagger Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::DaggerDamageImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Dagger);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(10, 10);

		// Also check that step-kill is blocked
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(10, 10);
	}

	SECTION("Test Caber Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::CaberDamageImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetPlayerWeapon, WeaponType::WT_Caber);
		RoomBuilder::Plot(T_WALL, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_S);

		AssertMonster(10, 10);
	}

	SECTION("Test Floorspike Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::FloorSpikeImmune, 1);
		RoomBuilder::Plot(T_FLOOR_SPIKES, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_WAIT, 10);

		AssertMonster(10, 10);
	}

	SECTION("Test Firetrap Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::FiretrapImmune, 1);
		RoomBuilder::Plot(T_FIRETRAP_ON, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 10);
	}

	SECTION("Test Hot Tile Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::HotTileImmune, 1);
		RoomBuilder::Plot(T_HOT, 10, 10);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 10);
	}

	SECTION("Test Explosion Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::ExplosionImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_AttackTile, ScriptFlag::AT_Explode);

		CCurrentGame* pGame = Runner::StartGame(10, 8, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 10);
	}

	SECTION("Test Briar Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::BriarImmune, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 2);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::BriarImmune, 0);

		RoomBuilder::PlotRect(T_WALL, 8, 9, 12, 11);
		RoomBuilder::PlotRect(T_FLOOR, 9, 10, 11, 10);
		RoomBuilder::Plot(T_BRIAR_SOURCE, 9, 10);

		CCurrentGame* pGame = Runner::StartGame(1, 1, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonster(10, 10);

		// Check that turning off immunity causes briar to grow
		Runner::ExecuteCommand(CMD_WAIT, 5);

		AssertNoMonster(10, 10);
		AssertTile(10, 10, T_BRIAR_DEAD);
		AssertTile(11, 10, T_BRIAR_DEAD);
	}

	SECTION("Test Adder Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::AdderImmune, 1);
		RoomBuilder::Plot(T_WALL, 9, 10);
		RoomBuilder::Plot(T_WALL, 11, 10);
		RoomBuilder::Plot(T_WALL, 9, 9);
		RoomBuilder::Plot(T_WALL, 11, 9);

		CMonster* pMonster = RoomBuilder::AddMonster(M_SERPENTG, 10, 9, S);
		CSerpent* pAdder = DYN_CAST(CSerpent*, CMonster*, pMonster);
		RoomBuilder::AddSerpentPiece(pAdder, 10, 8);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertMonsterType(10, 10, M_CHARACTER);
	}

	SECTION("Test Puff Immunity") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::PuffImmune, 1);
		RoomBuilder::AddMonster(M_FLUFFBABY, 10, 9);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 5);

		AssertMonster(10, 10);
	}

	// Test that FatalPushImmune behavior stops monsters being pushed into pits/water if it kills them
	TestUnsafePushPrevented(M_CITIZEN, T_PIT);
	TestUnsafePushPrevented(M_CITIZEN, T_WATER);
	TestUnsafePushPrevented(M_GUARD, T_PIT);
	TestUnsafePushPrevented(M_GUARD, T_WATER);
	TestUnsafePushPrevented(M_GELBABY, T_PIT);
	TestUnsafePushPrevented(M_GELBABY, T_WATER);
	TestUnsafePushPrevented(M_WATERSKIPPER, T_PIT);
	TestUnsafePushPrevented(M_SKIPPERNEST, T_PIT);

	// Test that FatalPushImmune behavior doesn't stop safe pit/water pushes
	TestSafePushAllowed(M_CITIZEN, T_SHALLOW_WATER);
	TestSafePushAllowed(M_GELBABY, T_SHALLOW_WATER);
	TestSafePushAllowed(M_CONSTRUCT, T_SHALLOW_WATER);
	TestSafePushAllowed(M_GUARD, T_SHALLOW_WATER);
	TestSafePushAllowed(M_WATERSKIPPER, T_WATER);
	TestSafePushAllowed(M_WATERSKIPPER, T_SHALLOW_WATER);
	TestSafePushAllowed(M_SKIPPERNEST, T_WATER);
	TestSafePushAllowed(M_SKIPPERNEST, T_SHALLOW_WATER);
	TestSafePushAllowed(M_FEGUNDO, T_PIT);
	TestSafePushAllowed(M_FEGUNDO, T_WATER);
	TestSafePushAllowed(M_FEGUNDO, T_SHALLOW_WATER);
	TestSafePushAllowed(M_WWING, T_PIT);
	TestSafePushAllowed(M_WWING, T_WATER);
	TestSafePushAllowed(M_WWING, T_SHALLOW_WATER);
}
