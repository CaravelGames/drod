#include "../../../test-include.hpp"

TEST_CASE("Scripting: Build with different types of orbs", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);

	RoomBuilder::Plot(T_DOOR_Y, 5, 5);
	RoomBuilder::Plot(T_ORB, 10, 10);
	RoomBuilder::Plot(T_ORB, 11, 10);
	RoomBuilder::Plot(T_ORB, 12, 10);

	RoomBuilder::AddOrbDataToTile(10, 10, OT_NORMAL);
	RoomBuilder::AddOrbDataToTile(11, 10, OT_ONEUSE);
	RoomBuilder::AddOrbDataToTile(12, 10, OT_BROKEN);

	RoomBuilder::LinkOrb(10, 10, 5, 5, OA_TOGGLE);
	RoomBuilder::LinkOrb(11, 10, 5, 5, OA_TOGGLE);
	RoomBuilder::LinkOrb(12, 10, 5, 5, OA_TOGGLE);

	SECTION("Building cracked orb over normal one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_ORB_CRACKED);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(10, 10);
		REQUIRE(game->pRoom->GetTSquare(10, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_ONEUSE);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}
	SECTION("Building broken orb over normal one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_ORB_BROKEN);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(10, 10);
		REQUIRE(game->pRoom->GetTSquare(10, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_BROKEN);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}



	SECTION("Building normal orb over cracked one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 11, 10, 0, 0, T_ORB);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(11, 10);
		REQUIRE(game->pRoom->GetTSquare(11, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_NORMAL);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}
	SECTION("Building broken orb over cracked one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 11, 10, 0, 0, T_ORB_BROKEN);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(11, 10);
		REQUIRE(game->pRoom->GetTSquare(11, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_BROKEN);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}


	SECTION("Building normal orb over broken one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 12, 10, 0, 0, T_ORB);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(12, 10);
		REQUIRE(game->pRoom->GetTSquare(12, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_NORMAL);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}
	SECTION("Building cracked orb over broken one"){
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 12, 10, 0, 0, T_ORB_CRACKED);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		COrbData *pOrbData = game->pRoom->GetOrbAtCoords(12, 10);
		REQUIRE(game->pRoom->GetTSquare(12, 10) == T_ORB);
		REQUIRE(pOrbData);
		REQUIRE(pOrbData->eType == OT_ONEUSE);
		REQUIRE(pOrbData->GetAgentAt(5, 5));
		REQUIRE(pOrbData->GetAgentAt(5, 5)->action == OA_TOGGLE);
	}
}
