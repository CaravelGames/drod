#include "../../test-include.hpp"
#include "../../CAssert.h"

TEST_CASE("Scripting: Set player appearance", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Clones don't press plates when role changes between two flying ones") {
		// SETUP: Room with plate and door, clone on multi-use plate;
		// script changes player role to flying then another flying
		RoomBuilder::AddMonster(M_CLONE, 10, 10);
		RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
		RoomBuilder::Plot(T_DOOR_Y, 11, 11);
		RoomBuilder::AddOrbDataToTile(10, 10, OrbType::OT_NORMAL);
		RoomBuilder::LinkOrb(10, 10, 11, 11, OrbAgentType::OA_TOGGLE);

		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerAppearance, M_WWING);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerAppearance, M_FEGUNDO);

		CCurrentGame* pGame = Runner::StartGame(20, 20, N);
		// Sanity check, door should start closed
		REQUIRE(pGame->pRoom->GetOSquare(11, 11) == T_DOOR_Y);
		Runner::ExecuteCommand(CMD_WAIT, 3);
		// THEN: Door should remain closed
		REQUIRE(pGame->pRoom->GetOSquare(11, 11) == T_DOOR_Y);
	}
}
