#include "../../test-include.hpp"
#include "../../CAssert.h"

void AssertRoachUnbrained() {
	AssertMonsterType(10, 10, M_ROACH);
}
void AssertRoachBrained() {
	AssertMonsterType(10, 9, M_ROACH);
}

TEST_CASE("Scripting: Imperative - Brain Pathmap Obstacle", "[game]") {
	RoomBuilder::ClearRoom();

	// #N# - NPC used in tests
	// #.#
	// #R# - Roach (10,10) enclosed in some walls, player is placed to the south of the roach
	// ###
	// .B. - Brain 

	RoomBuilder::PlotRect(T_WALL, 9, 8, 11, 11);
	RoomBuilder::PlotRect(T_FLOOR, 10, 8, 10, 10);
	RoomBuilder::AddMonster(M_ROACH, 10, 10, S);
	RoomBuilder::AddMonster(M_BRAIN, 10, 12, NO_ORIENTATION);

	CCharacter *pCharacter = RoomBuilder::AddVisibleCharacter(10, 8, S, M_BRAIN);
	RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::BrainPathmapObstacle);
	RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 0);

	SECTION("Roach should act unbrained"){
		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT);
		
		AssertRoachUnbrained();
	}

	SECTION("Disappear should clear the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Disappear);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertRoachBrained();
	}

	SECTION("Appear should re-add the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Disappear);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Appear);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 3);
		AssertRoachUnbrained();
		Runner::ExecuteCommand(CMD_WAIT);
		AssertRoachUnbrained();
	}

	SECTION("Appear At should re-add the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Disappear);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_AppearAt, 10, 8);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 3);
		AssertRoachUnbrained();
		Runner::ExecuteCommand(CMD_WAIT);
		AssertRoachUnbrained();
	}

	SECTION("Imperative Die should clear the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Die);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertRoachBrained();
	}

	SECTION("Imperative Die Special should clear the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::DieSpecial);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertRoachBrained();
	}

	SECTION("Imperative Not Brain Pathmap Obstacle should clear the pathmap") {
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::NotBrainPathmapObstacle);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		AssertRoachBrained();
	}

	SECTION("Killing the blocking character by stabbing should clear pathmap") {
		RoomBuilder::AddMonster(M_MIMIC, 12, 7, SW);

		CCurrentGame* pGame = Runner::StartGame(10, 20, N);
		Runner::ExecuteCommand(CMD_W);

		AssertRoachBrained();
	}
}
