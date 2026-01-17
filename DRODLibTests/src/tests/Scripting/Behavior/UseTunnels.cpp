#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Use Tunnels Behavior", "[game][scripting][behavior]") {
	RoomBuilder::ClearRoom();

	SECTION("NPC with Use Tunnels behavior goes through a tunnel") {
		RoomBuilder::Plot(T_TUNNEL_E, 10, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);

		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_SPIDER);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::UseTunnels, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// NPC should travel to the far tunnel
		AssertMonster(20, 10);
	}

	SECTION("NPC does not travel through tunnel in wrong direction") {
		RoomBuilder::Plot(T_TUNNEL_S, 10, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);

		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_SPIDER);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::UseTunnels, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// NPC should not travel to the far tunnel
		AssertMonster(11, 10);
		AssertNoMonster(20, 10);
	}

	SECTION("NPC travels through unpaired tunnel correctly") {
		RoomBuilder::Plot(T_TUNNEL_E, 10, 10);

		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_SPIDER);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::UseTunnels, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_MoveRel, 1, 0, 0, 1);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);

		// NPC should 'travel' to the same tunnel
		AssertMonster(10, 10);
	}

	SECTION("NPC should see traversable tunnel as open move") {
		RoomBuilder::Plot(T_TUNNEL_E, 10, 10);
		RoomBuilder::Plot(T_WALL, 11, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);

		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_SPIDER);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::UseTunnels, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_WaitForOpenMove, E);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(15, 15, N, CueEvents);
		Runner::ExecuteCommand(CMD_WAIT);

		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(eventOccured);
	}

	SECTION("NPC should not see untraversable tunnel as open move") {
		RoomBuilder::Plot(T_TUNNEL_S, 10, 10);
		RoomBuilder::Plot(T_WALL, 11, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);

		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 10, 0, M_SPIDER);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Behavior, ScriptFlag::UseTunnels, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_WaitForOpenMove, E);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS(""));

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(15, 15, N, CueEvents);
		Runner::ExecuteCommand(CMD_WAIT);

		bool eventOccured = CueEvents.HasOccurred(CID_ChallengeCompleted);
		REQUIRE(!eventOccured);
	}
}
