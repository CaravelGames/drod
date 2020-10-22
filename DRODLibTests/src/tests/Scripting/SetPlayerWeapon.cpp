#include "../../test-include.hpp"
#include "../../CAssert.h"

namespace {
	void AddScript_IfSetWeapon(const WeaponType eType) {
		CCharacter* pScript = RoomBuilder::AddCharacter(2, 2);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_WaitForPlayerInput, CMD_EXEC_COMMAND);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_If);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, eType);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_TeleportPlayerTo, 20, 20);
	}
}

TEST_CASE("Scripting: Set Player Weapon", "[game]") {
	RoomBuilder::ClearRoom();
	CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);

	SECTION("If...") {
		SECTION("ON - Detects on when starting room") {
			AddScript_IfSetWeapon(WT_On);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("ON - Detects on when stepping on disarm token twice") {
			AddScript_IfSetWeapon(WT_On);
			RoomBuilder::PlotToken(WeaponDisarm, 10, 11);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_S);
			Runner::ExecuteCommand(CMD_N);
			Runner::ExecuteCommand(CMD_S);
			Runner::ExecuteCommand(CMD_S);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("ON - Detects on when reequipped with script (Off -> On)") {
			AddScript_IfSetWeapon(WT_On);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Off);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_On);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_WAIT);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("ON - Detects on when reequipped with script (Off -> Sword)") {
			AddScript_IfSetWeapon(WT_On);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Off);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Sword);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_WAIT);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("ON - Detects on when reequipped with script (Off -> Dagger)") {
			AddScript_IfSetWeapon(WT_On);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Off);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Dagger);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_WAIT);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("OFF - Detects when stepped on disarm token") {
			AddScript_IfSetWeapon(WT_Off);
			RoomBuilder::PlotToken(WeaponDisarm, 10, 11);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_S);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}

		SECTION("OFF - Detects when disarmed with script") {
			AddScript_IfSetWeapon(WT_Off);
			RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Off);

			CCurrentGame* pGame = Runner::StartGame(10, 10);
			Runner::ExecuteCommand(CMD_WAIT);
			Runner::ExecuteCommand(CMD_EXEC_COMMAND);

			AssertPlayerAt(20, 20);
		}
	}
}
