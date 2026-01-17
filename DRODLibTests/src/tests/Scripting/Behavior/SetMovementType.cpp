#include "../../../test-include.hpp"
#include "../../../CAssert.h"

static void TestDefaultMovementAllowed(const UINT identity, const UINT tileType) {
	string name =
		"Character with identity #" +
		std::to_string(identity) +
		" should be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, identity);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 2);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 12);
	}
}

static void TestDefaultMovementBlocked(const UINT identity, const UINT tileType) {
	string name =
		"Character with identity #" +
		std::to_string(identity) +
		" should not be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, identity);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 2);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 10);
	}
}

static void TestMovementTypeAllowed(const MovementType moveType, const UINT tileType) {
	string name =
		"Character with move type #" +
		std::to_string(moveType) +
		" should be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetMovementType, moveType);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 2);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 12);
	}
}

static void TestMovementTypeBlocked(const MovementType moveType, const UINT tileType) {
	string name =
		"Character with move type #" +
		std::to_string(moveType) +
		" should not be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetMovementType, moveType);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 2);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 10);
	}
}

static void TestRestrictedMovementTypeAllowed(const MovementType moveType, const UINT tileType) {
	string name =
		"Character with restricted move type #" +
		std::to_string(moveType) +
		" should be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::RestrictedMovement, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetMovementType, moveType);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 1);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 11);
	}
}

static void TestRestrictedMovementTypeBlocked(const MovementType moveType, const UINT tileType) {
	string name =
		"Character with restricted move type #" +
		std::to_string(moveType) +
		" should not be able to cross tile #" +
		std::to_string(tileType);

	SECTION(name) {
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, S, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DirectBeelining);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::RestrictedMovement, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_SetMovementType, moveType);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 0, 1);

		RoomBuilder::Plot(tileType, 10, 11);

		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertMonster(10, 10);
	}
}

TEST_CASE("Scripting: Set movement type", "[game][scripting][movement]") {
	RoomBuilder::ClearRoom();
	//Include trapdoor, tar and brain to prevent door state change
	RoomBuilder::Plot(T_TRAPDOOR, 1, 1);
	RoomBuilder::PlotRect(T_TAR, 2, 2, 3, 3);
	RoomBuilder::AddMonster(M_BRAIN, 1, 2);

	//Test that identities with each default move type interact correctly
	//Ground and Shallow charatcer
	TestDefaultMovementAllowed(M_ROACH, T_FLOOR);
	TestDefaultMovementAllowed(M_ROACH, T_SHALLOW_WATER);
	TestDefaultMovementBlocked(M_ROACH, T_WALL);
	TestDefaultMovementBlocked(M_ROACH, T_PIT);
	TestDefaultMovementBlocked(M_ROACH, T_WATER);
	//Water charater
	TestDefaultMovementAllowed(M_WATERSKIPPER, T_FLOOR);
	TestDefaultMovementAllowed(M_WATERSKIPPER, T_WATER);
	TestDefaultMovementAllowed(M_WATERSKIPPER, T_SHALLOW_WATER);
	TestDefaultMovementBlocked(M_WATERSKIPPER, T_WALL);
	TestDefaultMovementBlocked(M_WATERSKIPPER, T_PIT);
	//Wall character
	TestDefaultMovementAllowed(M_SEEP, T_FLOOR);
	TestDefaultMovementAllowed(M_SEEP, T_WALL);
	TestDefaultMovementAllowed(M_SEEP, T_DOOR_C);
	TestDefaultMovementAllowed(M_SEEP, T_DOOR_M);
	TestDefaultMovementAllowed(M_SEEP, T_DOOR_R);
	TestDefaultMovementAllowed(M_SEEP, T_DOOR_Y);
	TestDefaultMovementAllowed(M_SEEP, T_DOOR_B);
	TestDefaultMovementAllowed(M_SEEP, T_SHALLOW_WATER);
	TestDefaultMovementBlocked(M_SEEP, T_PIT);
	TestDefaultMovementBlocked(M_SEEP, T_WATER);
	//Air character
	TestDefaultMovementAllowed(M_FEGUNDO, T_FLOOR);
	TestDefaultMovementAllowed(M_FEGUNDO, T_PIT);
	TestDefaultMovementAllowed(M_FEGUNDO, T_WATER);
	TestDefaultMovementAllowed(M_FEGUNDO, T_SHALLOW_WATER);
	TestDefaultMovementBlocked(M_FEGUNDO, T_WALL);

	//Test that setting movement type has correct interactions
	//Ground
	TestMovementTypeAllowed(MovementType::GROUND, T_FLOOR);
	TestMovementTypeBlocked(MovementType::GROUND, T_PIT);
	TestMovementTypeBlocked(MovementType::GROUND, T_WALL);
	TestMovementTypeBlocked(MovementType::GROUND, T_WATER);
	TestMovementTypeBlocked(MovementType::GROUND, T_SHALLOW_WATER);
	//Ground and Shallow
	TestMovementTypeAllowed(MovementType::GROUND_AND_SHALLOW_WATER, T_FLOOR);
	TestMovementTypeAllowed(MovementType::GROUND_AND_SHALLOW_WATER, T_SHALLOW_WATER);
	TestMovementTypeBlocked(MovementType::GROUND_AND_SHALLOW_WATER, T_PIT);
	TestMovementTypeBlocked(MovementType::GROUND_AND_SHALLOW_WATER, T_WALL);
	TestMovementTypeBlocked(MovementType::GROUND_AND_SHALLOW_WATER, T_WATER);
	//Water
	TestMovementTypeAllowed(MovementType::WATER, T_FLOOR);
	TestMovementTypeAllowed(MovementType::WATER, T_WATER);
	TestMovementTypeAllowed(MovementType::WATER, T_SHALLOW_WATER);
	TestMovementTypeBlocked(MovementType::WATER, T_PIT);
	TestMovementTypeBlocked(MovementType::WATER, T_WALL);
	//Wall
	TestMovementTypeAllowed(MovementType::WALL, T_FLOOR);
	TestMovementTypeAllowed(MovementType::WALL, T_DOOR_C);
	TestMovementTypeAllowed(MovementType::WALL, T_DOOR_M);
	TestMovementTypeAllowed(MovementType::WALL, T_DOOR_R);
	TestMovementTypeAllowed(MovementType::WALL, T_DOOR_Y);
	TestMovementTypeAllowed(MovementType::WALL, T_DOOR_B);
	TestMovementTypeAllowed(MovementType::WALL, T_SHALLOW_WATER);
	TestMovementTypeBlocked(MovementType::WALL, T_PIT);
	TestMovementTypeBlocked(MovementType::WALL, T_WATER);
	//Air
	TestMovementTypeAllowed(MovementType::AIR, T_FLOOR);
	TestMovementTypeAllowed(MovementType::AIR, T_PIT);
	TestMovementTypeAllowed(MovementType::AIR, T_WATER);
	TestMovementTypeAllowed(MovementType::AIR, T_SHALLOW_WATER);
	TestMovementTypeBlocked(MovementType::AIR, T_WALL);

	//Test that restricted movement type has correct interactions
	//Water
	TestRestrictedMovementTypeAllowed(MovementType::WATER, T_WATER);
	TestRestrictedMovementTypeAllowed(MovementType::WATER, T_SHALLOW_WATER);
	TestRestrictedMovementTypeBlocked(MovementType::WATER, T_FLOOR);
	TestRestrictedMovementTypeBlocked(MovementType::WATER, T_PIT);
	TestRestrictedMovementTypeBlocked(MovementType::WATER, T_WALL);
	//Wall
	TestRestrictedMovementTypeAllowed(MovementType::WALL, T_DOOR_C);
	TestRestrictedMovementTypeAllowed(MovementType::WALL, T_DOOR_M);
	TestRestrictedMovementTypeAllowed(MovementType::WALL, T_DOOR_R);
	TestRestrictedMovementTypeAllowed(MovementType::WALL, T_DOOR_Y);
	TestRestrictedMovementTypeAllowed(MovementType::WALL, T_DOOR_B);
	TestRestrictedMovementTypeBlocked(MovementType::WALL, T_FLOOR);
	TestRestrictedMovementTypeBlocked(MovementType::WALL, T_PIT);
	TestRestrictedMovementTypeBlocked(MovementType::WALL, T_WATER);
	TestRestrictedMovementTypeBlocked(MovementType::WALL, T_SHALLOW_WATER);
}
