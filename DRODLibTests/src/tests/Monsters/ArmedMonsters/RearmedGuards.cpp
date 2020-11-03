#include "../../../catch.hpp"
#include "../../../CAssert.h"
#include "../../../CTestDb.h"
#include "../../../Runner.h"
#include "../../../RoomBuilder.h"
#include "../../../../../DRODLib/CurrentGame.h"
#include "../../../../../DRODLib/Character.h"
#include "../../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Guards wielding different weapons", "[game][guard][weapon]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Guard with a dagger won't bother to turn around"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, W);

		CCueEvents CueEvents;
		Runner::StartGame(12, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	
	SECTION("Guard with a dagger will body-kill player"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, W);

		CCueEvents CueEvents;
		Runner::StartGame(11, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	
	SECTION("Guard with a dagger will not body-kill if that would blow a bomb"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, W);
		RoomBuilder::Plot(T_BOMB, 12, 10);

		CCueEvents CueEvents;
		Runner::StartGame(11, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}

	SECTION("Guard with a dagger will kill-step normal monsters") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonster(M_BRAIN, 11, 10, S);

		CCueEvents CueEvents;
		CCurrentGame* game =  Runner::StartGame(13, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		CMonster* monster = game->pRoom->GetMonsterAtSquare(11, 10);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterDiedFromStab));
		REQUIRE(monster != NULL);
		REQUIRE(monster->wType == M_GUARD);
	}

	SECTION("Guard with a dagger will kill-step vulnerable characters") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		RoomBuilder::AddVisibleCharacter(11, 10, S, M_BRAIN);

		CCurrentGame* game = Runner::StartGame(13, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertEvent(CID_MonsterDiedFromStab);
		AssertMonsterType(11, 10, M_GUARD);
	}

	SECTION("Guard with a dagger will kill-step vulnerable characters (even if role that normally can't be stepped on)") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(11, 10, S, M_CITIZEN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Vulnerable);

		CCurrentGame* game = Runner::StartGame(13, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertEvent(CID_MonsterDiedFromStab);
		AssertMonsterType(11, 10, M_GUARD);
	}

	SECTION("Guard with a dagger will walk around invulnerable (naturally) characters") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		RoomBuilder::AddVisibleCharacter(11, 10, S, M_CITIZEN);

		CCurrentGame* game = Runner::StartGame(13, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertNoEvent(CID_MonsterDiedFromStab);
		AssertMonsterType(11, 10, M_CHARACTER);
		AssertMonsterType(11, 9, M_GUARD);
	}

	SECTION("Guard with a dagger will walk around invulnerable (imperative) characters") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(11, 10, S, M_BRAIN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::Invulnerable);

		CCurrentGame* game = Runner::StartGame(13, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertNoEvent(CID_MonsterDiedFromStab);
		AssertMonsterType(11, 10, M_CHARACTER);
		AssertMonsterType(11, 9, M_GUARD);
	}

	SECTION("Guard with a dagger will walk around pushable-by-weapon characters") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(11, 10, S, M_BRAIN);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Imperative, ScriptFlag::PushableByWeapon);

		CCurrentGame* game = Runner::StartGame(13, 10, S);
		Runner::ExecuteCommand(CMD_WAIT);

		AssertNoEvent(CID_MonsterDiedFromStab);
		AssertMonsterType(11, 10, M_CHARACTER);
		AssertMonsterType(11, 9, M_GUARD);
	}

	SECTION("Guard with a dagger won't kill-step other guards") {
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Sword, 11, 10, S);

		RoomBuilder::PlotRect(T_WALL, 9, 9, 14, 11);
		RoomBuilder::PlotRect(T_FLOOR, 10, 10, 13, 10);
		RoomBuilder::Plot(T_FLOOR, 10, 11);
		RoomBuilder::Plot(T_BOMB, 10, 11);
		RoomBuilder::Plot(T_FLOOR, 12, 11);
		RoomBuilder::Plot(T_BOMB, 12, 11);

		CCueEvents CueEvents;
		CCurrentGame* game = Runner::StartGame(13, 10, N);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);

		CMonster* monster = game->pRoom->GetMonsterAtSquare(11, 10);
		REQUIRE(!CueEvents.HasOccurred(CID_MonsterDiedFromStab));
		REQUIRE(monster != NULL);
		REQUIRE(monster->wType == M_GUARD);
		REQUIRE(monster->GetWeaponType() == WT_Sword);
	}

	SECTION("Guard with a caber on player will stay in place to kill the player"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Caber, 10, 10, S);

		CCueEvents CueEvents;
		Runner::StartGame(10, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	SECTION("Guard with a staff on player will move forward"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Staff, 10, 10, S);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->swordsman.wX == 10);
		REQUIRE(pGame->swordsman.wY == 12);
	}
	SECTION("Guard with a pick-axe on player will move forward"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Pickaxe, 10, 10, S);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->swordsman.wX == 10);
		REQUIRE(pGame->swordsman.wY == 12);
	}
	SECTION("Guard with a spear on player will move into player to kill"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Spear, 10, 10, S);

		CCueEvents CueEvents;
		Runner::StartGame(10, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	
	SECTION("Guard with a spear on player will move into player to kill, even if bomb is one tile further because it won't be stabbed"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Spear, 10, 10, S);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		Runner::StartGame(10, 11, E);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(CueEvents.HasOccurred(CID_MonsterKilledPlayer));
	}
	
	SECTION("Guard with a staff won't be afraid of attacking bombs"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Staff, 10, 10, S);
		RoomBuilder::Plot(T_BOMB, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 14, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11));
	}
	SECTION("Guard with a dagger will turn to face the direction they're moving"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, S);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(20, 10, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(11, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(11, 10)->wO == E);
	}
	SECTION("Guard with a dagger will turn to face the direction they tried to move"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, S);
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_WALL, 11,10, 11, 10);
		RoomBuilder::PlotRect(T_WALL, 9, 11, 11, 11);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(20, 10, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wO == E);
	}
	SECTION("Guard with a dagger will turn to face the direction they tried to move even NW"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, S);
		RoomBuilder::PlotRect(T_WALL, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_WALL, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_WALL, 11,10, 11, 10);
		RoomBuilder::PlotRect(T_WALL, 9, 11, 11, 11);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(5, 5, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10));
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 10)->wO == NW);
	}
	SECTION("Guard with a dagger will refuse to kill himself when closely surrounded by bombs"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, E);
		RoomBuilder::PlotRect(T_BOMB, 9, 9, 11, 9);
		RoomBuilder::PlotRect(T_BOMB, 9, 10, 9, 10);
		RoomBuilder::PlotRect(T_BOMB, 12,10, 12, 10);
		RoomBuilder::PlotRect(T_BOMB, 9, 11, 11, 11);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(20, 20, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(!CueEvents.HasOccurred(CID_BombExploded));
	}
	SECTION("Guard with a staff will push a powder keg"){
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Staff, 10, 10, S);
		RoomBuilder::Plot(T_POWDER_KEG, 10, 12);

		CCueEvents CueEvents;
		CCurrentGame* pGame = Runner::StartGame(10, 20, S);
		Runner::ExecuteCommand(CMD_WAIT, CueEvents);
		REQUIRE(pGame->pRoom->GetMonsterAtSquare(10, 11));
		REQUIRE(pGame->pRoom->GetTObject(10, 13));
	}

	SECTION("Guard with a dagger won't step on an invulnerable character") {
		// W.W
		// WGW - Guard with dagger facing North
		// WCW - Invulnerable character, Player placed below

		CCharacter* character = RoomBuilder::AddVisibleCharacter(10, 11, S, M_CITIZEN);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Imperative, ScriptFlag::Invulnerable);
		RoomBuilder::AddMonsterWithWeapon(M_GUARD, WT_Dagger, 10, 10, N);
		RoomBuilder::PlotRect(T_WALL, 9, 9, 9, 11);
		RoomBuilder::PlotRect(T_WALL, 11, 9, 11, 11);

		CCurrentGame* pGame = Runner::StartGame(10, 15, S);
		Runner::ExecuteCommand(CMD_WAIT);
		AssertMonsterTypeO(10, 10, M_GUARD, S);
		AssertMonsterType(10, 11, M_CHARACTER);
	}
}
