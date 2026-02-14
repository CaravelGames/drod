#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Activate Pressure Plate Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::Plot(T_DOOR_Y, 10, 12);
	RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
	RoomBuilder::AddOrbDataToTile(10, 10, OrbType::OT_ONEUSE);
	RoomBuilder::LinkOrb(10, 10, 10, 12, OrbAgentType::OA_TOGGLE);

	SECTION("Test Activate Plates On") {
		// By default, flying characters do not activate plates
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_FEGUNDO);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::ActivatePlates, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 12, T_DOOR_YO);
	}

	SECTION("Test Activate Plates Off") {
		// By default, roach characters do activate plates
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_ROACH);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::ActivatePlates, 0);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 12, T_DOOR_Y);
	}

	SECTION("Test Wraithwing-characters don't activate plates by default") {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(9, 10, 0, M_WWING);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 12, T_DOOR_Y);
	}
}
