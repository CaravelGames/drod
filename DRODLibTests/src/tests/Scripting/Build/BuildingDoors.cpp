#include "../../../test-include.hpp"

namespace {
	void AsserOrbConnectionsCount(const int expectedDoorConnectionsCount){
		const CDbRoom* pRoom = Runner::GetCurrentGame()->pRoom;

		int connectionsCount = 0;
		for (UINT orb = 0; orb < pRoom->orbs.size(); ++orb)
		{
			COrbData* pData = pRoom->orbs[orb];
			for (UINT agent = 0; agent < pData->agents.size(); ++agent)
			{
				COrbAgentData* pAgentData = pData->agents[agent];
				if (pAgentData->action != OA_NULL)
					connectionsCount++;
			}
		}

		REQUIRE(connectionsCount == expectedDoorConnectionsCount);
	}

	void AssertHasOrbConnection(const UINT wDoorX, const UINT wDoorY, const UINT wOrbX, const UINT wOrbY, const OrbAgentType connectionType) {
		const CDbRoom* pRoom = Runner::GetCurrentGame()->pRoom;
		
		CCoordSet doorCoords;
		pRoom->GetAllYellowDoorSquares(wDoorX, wDoorY, doorCoords);
		
		COrbData* pPlate = pRoom->GetPressurePlateAtCoords(wOrbX, wOrbY);

		for (UINT orb = 0; orb < pRoom->orbs.size(); ++orb)
		{
			COrbData* pData = pRoom->orbs[orb];

			if (
				(pData->wX != wOrbX || pData->wY != wOrbY)
				&& pData != pPlate // Orb connection is on a different tile but its part of the same plate
			)
				continue;

			bool bWasValidConnetionFound = false;
			bool bWasConnectionFound = false;
			for (UINT agent = 0; agent < pData->agents.size(); ++agent)
			{
				COrbAgentData* pAgentData = pData->agents[agent];
				if (!doorCoords.has(pAgentData->wX, pAgentData->wY))
					continue;

				REQUIRE(bWasConnectionFound == false);

				if (pAgentData->action == connectionType)
					bWasValidConnetionFound = true;

				bWasConnectionFound = true;
			}

			if (bWasConnectionFound) {
				REQUIRE(bWasValidConnetionFound == true);
				return;
			}
		}

		FAIL("Connection was not found");
	}
}

TEST_CASE("Scripting: Building doors", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Combining two doors merges their connections") {
		// ..Y..
		// .Y.Y. - Four yellow doors separated by space that has door built
		// ..Y..
		// .OOO. - Three orbs, each test defines what they do
		// ..|..
		// ..B.. - Beethro facing north

		RoomBuilder::Plot(T_DOOR_Y, 9, 10);
		RoomBuilder::Plot(T_DOOR_Y, 11, 10);
		RoomBuilder::Plot(T_DOOR_Y, 10, 9);
		RoomBuilder::Plot(T_DOOR_Y, 10, 11);
		RoomBuilder::Plot(T_ORB, 9, 12);
		RoomBuilder::Plot(T_ORB, 10, 12);
		RoomBuilder::Plot(T_ORB, 11, 12);

		RoomBuilder::AddOrbDataToTile(9, 12);
		RoomBuilder::AddOrbDataToTile(10, 12);
		RoomBuilder::AddOrbDataToTile(11, 12);

		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_DOOR_Y);

		SECTION("Open + nothing = 1 open") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_OPEN);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_OPEN);
		}

		SECTION("Close + nothing = 1 close") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_CLOSE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_CLOSE);
		}

		SECTION("Toggle + nothing = 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_TOGGLE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Open + Open = 1 open") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_OPEN);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_OPEN);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_OPEN);
		}

		SECTION("Close + Close = 1 close") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_CLOSE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_CLOSE);
		}

		SECTION("Toggle + Toggle = 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_TOGGLE);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_TOGGLE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Open + Close= 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_OPEN);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_CLOSE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Open + Toggle = 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_OPEN);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_TOGGLE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Close + Toggle = 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_TOGGLE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Open + Close + Toggle + Nothing = 1 toggle") {
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_TOGGLE);
			RoomBuilder::LinkOrb(10, 12, 10, 11, OA_OPEN);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
		}

		SECTION("Big case with different orbs") {
			// Left orb
			RoomBuilder::LinkOrb(9, 12, 11, 10, OA_OPEN);
			RoomBuilder::LinkOrb(9, 12, 10, 11, OA_OPEN);

			// Middle orb
			RoomBuilder::LinkOrb(10, 12, 9, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_TOGGLE);
			RoomBuilder::LinkOrb(10, 12, 10, 11, OA_OPEN);

			// Right orb
			RoomBuilder::LinkOrb(11, 12, 9, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(11, 12, 11, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(11, 12, 10, 9, OA_CLOSE);
			RoomBuilder::LinkOrb(11, 12, 10, 11, OA_CLOSE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(3);
			AssertHasOrbConnection(9, 10, 9, 12, OA_OPEN);
			AssertHasOrbConnection(9, 10, 10, 12, OA_TOGGLE);
			AssertHasOrbConnection(9, 10, 11, 12, OA_CLOSE);
		}

	}


	SECTION("REGRESSION (door merging): Swapped X/Y iteration when merging doors (iterated StartX/StartY to EndY/EndX)") {
		RoomBuilder::ClearRoom();

		RoomBuilder::Plot(T_DOOR_Y, 10, 3);
		RoomBuilder::Plot(T_DOOR_Y, 10, 5);
		RoomBuilder::Plot(T_ORB, 10, 10);

		RoomBuilder::AddOrbDataToTile(10, 10);

		RoomBuilder::LinkOrb(10, 10, 10, 3, OA_OPEN);
		RoomBuilder::LinkOrb(10, 10, 10, 5, OA_CLOSE);

		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Build, 10, 4, 0, 0, T_DOOR_Y);

		CCurrentGame* pGame = Runner::StartGame(10, 12, N);
		Runner::ExecuteCommand(CMD_N, 1);

		AsserOrbConnectionsCount(1);
		AssertHasOrbConnection(10, 3, 10, 10, OA_TOGGLE);
	}


	SECTION("Removing doors splits their connections evenly") {
		// ..Y..
		// .YYY. - Plus shaped yellow door that is split in the middle
		// ..Y..
		// .OOO. - Three orbs, each test defines what they do
		// ..|..
		// ..B.. - Beethro facing north

		RoomBuilder::Plot(T_DOOR_Y, 9, 10);
		RoomBuilder::Plot(T_DOOR_Y, 11, 10);
		RoomBuilder::Plot(T_DOOR_Y, 10, 10);
		RoomBuilder::Plot(T_DOOR_Y, 10, 9);
		RoomBuilder::Plot(T_DOOR_Y, 10, 11);
		RoomBuilder::Plot(T_ORB, 9, 12);
		RoomBuilder::Plot(T_ORB, 10, 12);
		RoomBuilder::Plot(T_ORB, 11, 12);

		RoomBuilder::AddOrbDataToTile(9, 12);
		RoomBuilder::AddOrbDataToTile(10, 12);
		RoomBuilder::AddOrbDataToTile(11, 12);

		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_FLOOR);

		SECTION("Orb linked on removed tile will spread the connection everywhere") {
			RoomBuilder::LinkOrb(10, 12, 10, 10, OA_OPEN);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(4);
			AssertHasOrbConnection(9, 10, 10, 12, OA_OPEN);
			AssertHasOrbConnection(11, 10, 10, 12, OA_OPEN);
			AssertHasOrbConnection(10, 9, 10, 12, OA_OPEN);
			AssertHasOrbConnection(10, 11, 10, 12, OA_OPEN);
		}

		SECTION("Orb links on isolated doors will spread everywhere") {
			RoomBuilder::LinkOrb(9, 12, 9, 10, OA_OPEN);
			RoomBuilder::LinkOrb(10, 12, 11, 10, OA_CLOSE);
			RoomBuilder::LinkOrb(11, 12, 10, 11, OA_TOGGLE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(12);
			AssertHasOrbConnection(9, 10, 9, 12, OA_OPEN);
			AssertHasOrbConnection(11, 10, 9, 12, OA_OPEN);
			AssertHasOrbConnection(10, 9, 9, 12, OA_OPEN);
			AssertHasOrbConnection(10, 11, 9, 12, OA_OPEN);
			AssertHasOrbConnection(9, 10, 10, 12, OA_CLOSE);
			AssertHasOrbConnection(11, 10, 10, 12, OA_CLOSE);
			AssertHasOrbConnection(10, 9, 10, 12, OA_CLOSE);
			AssertHasOrbConnection(10, 11, 10, 12, OA_CLOSE);
			AssertHasOrbConnection(9, 10, 11, 12, OA_TOGGLE);
			AssertHasOrbConnection(11, 10, 11, 12, OA_TOGGLE);
			AssertHasOrbConnection(10, 9, 11, 12, OA_TOGGLE);
			AssertHasOrbConnection(10, 11, 11, 12, OA_TOGGLE);
		}

		SECTION("Removed connection tile from full door") {
			RoomBuilder::PlotRect(T_DOOR_Y, 9, 9, 11, 11);
			RoomBuilder::LinkOrb(10, 12, 10, 10, OA_CLOSE);

			CCurrentGame* pGame = Runner::StartGame(10, 14, N);
			Runner::ExecuteCommand(CMD_N, 1);

			AsserOrbConnectionsCount(1);
			AssertHasOrbConnection(9, 9, 10, 12, OA_CLOSE);
		}
	}
}
