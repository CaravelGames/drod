#include "../../../test-include.hpp"

#include <vector>
using namespace std;

static void AddPushableCharacter(UINT x, UINT y){
	CCharacter *character = RoomBuilder::AddVisibleCharacter(x, y);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByBody);
}

static void TestCharacterCanBePushedByCharacterWithIdentity(const UINT identity) {
	char name[100];
	sprintf(name, "Character ID #%d should be able to push the character", identity);

	SECTION(name){
		AddPushableCharacter(10, 9);
		CCharacter* character = RoomBuilder::AddVisibleCharacter(10, 8, S, identity);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_VarSet, ScriptVars::P_MONSTER_WEAPON, ScriptVars::Assign, WT_Off);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_MoveRel, 0, 1);
		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10)->wType == M_CHARACTER);
	}
}


TEST_CASE("Invulnerable character with 'Imperative: Pushable by body'", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	SECTION("Check for macro assumptions to be correct"){
		AddPushableCharacter(10, 9);
		CCurrentGame* game = Runner::StartGame(15, 15, N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
	}

	SECTION("Construct should be able to push the character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_CONSTRUCT, 10, 8, S);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10)->wType == M_CHARACTER);
	}

	SECTION("Roach character should not be able to push the character"){
		AddPushableCharacter(10, 9);
		CCharacter* character = RoomBuilder::AddVisibleCharacter(10, 8, S, M_ROACH);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_MoveRel, 0, 1);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 10) == NULL);
	}

	SECTION("Pushing construct should step on tile occupied by pushed object"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_CONSTRUCT, 10, 8, S);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8) == NULL);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CONSTRUCT);
	}

	SECTION("Construct push should fail if blocked by force arrow on target tile"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_CONSTRUCT, 10, 8, S);
		RoomBuilder::Plot(T_ARROW_N, 10, 10);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
	}

	SECTION("Construct push  should fail if blocked by force arrow on source tile"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_CONSTRUCT, 10, 8, S);
		RoomBuilder::Plot(T_ARROW_N, 10, 9);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
	}

	SECTION("Construct push should fail if object is on oremites"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_CONSTRUCT, 10, 8, S);
		RoomBuilder::Plot(T_GOO, 10, 9);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_WAIT);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 9)->wType == M_CHARACTER);
	}

	SECTION("Mimic should push character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::AddMonster(M_MIMIC, 10, 10, S);

		CCurrentGame* game = Runner::StartGame(10, 15, N);
		Runner::ExecuteCommand(CMD_N);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8)->wType == M_CHARACTER);
	}

	SECTION("Temporal clone should push character"){
		AddPushableCharacter(10, 9);
		RoomBuilder::PlotToken(TemporalSplit, 10, 10);

		CCurrentGame* game = Runner::StartGame(10, 11, S);
		Runner::ExecuteCommand(CMD_N, 2);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_E, 1);
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8));
		REQUIRE(game->pRoom->GetMonsterAtSquare(10, 8)->wType == M_CHARACTER);
	}

	TestCharacterCanBePushedByCharacterWithIdentity(M_DECOY);
	TestCharacterCanBePushedByCharacterWithIdentity(M_MIMIC);
	TestCharacterCanBePushedByCharacterWithIdentity(M_BEETHRO);
	TestCharacterCanBePushedByCharacterWithIdentity(M_BEETHRO_IN_DISGUISE);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CLONE);
	TestCharacterCanBePushedByCharacterWithIdentity(M_SLAYER);
	TestCharacterCanBePushedByCharacterWithIdentity(M_SLAYER2);
	TestCharacterCanBePushedByCharacterWithIdentity(M_GUARD);
	TestCharacterCanBePushedByCharacterWithIdentity(M_STALWART);
	TestCharacterCanBePushedByCharacterWithIdentity(M_STALWART2);
	TestCharacterCanBePushedByCharacterWithIdentity(M_GUNTHRO);
	TestCharacterCanBePushedByCharacterWithIdentity(M_TEMPORALCLONE);
	TestCharacterCanBePushedByCharacterWithIdentity(M_HALPH);
	TestCharacterCanBePushedByCharacterWithIdentity(M_HALPH2);
	TestCharacterCanBePushedByCharacterWithIdentity(M_NEATHER);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CITIZEN);
	TestCharacterCanBePushedByCharacterWithIdentity(M_ARCHITECT);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CITIZEN1);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CITIZEN2);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CITIZEN3);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CITIZEN4);
	TestCharacterCanBePushedByCharacterWithIdentity(M_INSTRUCTOR);
	TestCharacterCanBePushedByCharacterWithIdentity(M_NEGOTIATOR);
	TestCharacterCanBePushedByCharacterWithIdentity(M_MUDCOORDINATOR);
	TestCharacterCanBePushedByCharacterWithIdentity(M_TARTECHNICIAN);
	TestCharacterCanBePushedByCharacterWithIdentity(M_CONSTRUCT);
}
