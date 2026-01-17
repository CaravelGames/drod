#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Drop Trapdoor Behaviors", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();
	RoomBuilder::Plot(T_TRAPDOOR, 10, 10);
	RoomBuilder::Plot(T_THINICE, 10, 11);

	RoomBuilder::Plot(T_TRAPDOOR, 12, 10);
	RoomBuilder::Plot(T_THINICE, 12, 11);

	SECTION("Test Drop Trapdoors On") {
		// Neither of these characters can drop trapdoors or melt ice by default
		CCharacter* pUnarmed = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoors, 1);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCharacter* pArmed = RoomBuilder::AddVisibleCharacter(12, 10, S, M_GUARD);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoors, 1);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// All objects should have dropped
		AssertTile(10, 10, T_PIT);
		AssertTile(10, 11, T_WATER);
		AssertTile(12, 10, T_PIT);
		AssertTile(12, 11, T_WATER);
	}

	SECTION("Test Drop Trapdoors when Armed On") {
		// Neither of these characters can drop trapdoors or melt ice by default
		CCharacter* pUnarmed = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CITIZEN);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoorsArmed, 1);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCharacter* pArmed = RoomBuilder::AddVisibleCharacter(12, 10, S, M_GUARD);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoorsArmed, 1);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Unarmed character should have melted ice only
		AssertTile(10, 10, T_TRAPDOOR);
		AssertTile(10, 11, T_WATER);
		// Armed character should have dropped trapdoor and melted ice
		AssertTile(12, 10, T_PIT);
		AssertTile(12, 11, T_WATER);
	}

	SECTION("Test Drop Trapdoors Off") {
		// Construct can drop trapdoors by default
		CCharacter* pUnarmed = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_CONSTRUCT);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoors, 0);
		RoomBuilder::AddCommand(pUnarmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// No objects should have dropped
		AssertTile(10, 10, T_TRAPDOOR);
		AssertTile(10, 11, T_THINICE);
	}

	SECTION("Test Drop Trapdoors when Armed Off") {
		// Mimic can drop trapdoors by default
		CCharacter* pArmed = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_MIMIC);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_Behavior, ScriptFlag::DropTrapdoorsArmed, 0);
		RoomBuilder::AddCommand(pArmed, CCharacterCommand::CC_MoveRel, 0, 2);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// No objects should have dropped
		AssertTile(10, 10, T_TRAPDOOR);
		AssertTile(10, 11, T_THINICE);
	}
}
