#include "CAssert.h"

#include "Runner.h"
#include "test-include.hpp"

#include <sstream>

#define LOG_INFO(file, line, context) \
	INFO( "Assert from: " << file << "::" << line << " (" << context << ")");

void Assert::Event(const char* file, int line, const CUEEVENT_ID eEventType) {
	LOG_INFO(file, line, "Event");

	CCueEvents& CueEvents = Runner::GetLastCueEvents();

	REQUIRE(CueEvents.HasOccurred(eEventType));
}

void Assert::NoEvent(const char* file, int line, const CUEEVENT_ID eEventType) {
	LOG_INFO(file, line, "NoEvent");

	CCueEvents& CueEvents = Runner::GetLastCueEvents();

	REQUIRE_FALSE(CueEvents.HasOccurred(eEventType));
}

void Assert::PlayerAt(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY) {
	LOG_INFO(file, line, "PlayerAt");

	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(game->swordsman.wX == wExpectedX);
	REQUIRE(game->swordsman.wY == wExpectedY);
}

void Assert::PlayerIsAlive(const char* file, int line) {
	LOG_INFO(file, line, "PlayerIsAlive");

	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(!game->IsPlayerDying());
}

void Assert::PlayerIsDead(const char* file, int line) {
	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(game->IsPlayerDying());
}

void Assert::RoomHasMonster(const char* file, int line, const long int wExpectedType) {
	LOG_INFO(file, line, "RoomHasMonster");

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->MonsterOfTypeExists(wExpectedType);

	REQUIRE(monster != NULL);
}

void Assert::RoomHasNoMonster(const char* file, int line, const long int wExpectedType) {
	LOG_INFO(file, line, "RoomHasNoMonster");

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->MonsterOfTypeExists(wExpectedType);

	REQUIRE(monster == NULL);
}

void Assert::Monster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const long int wExpectedType, const long int wExpectedO) {
	LOG_INFO(file, line, "Monster");

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->GetMonsterAtSquare(wExpectedX, wExpectedY);

	REQUIRE(monster != NULL);
	if (wExpectedType != -1) {
		REQUIRE(monster->wType == wExpectedType);
	}
	if (wExpectedO != -1) {
		REQUIRE(monster->wO == wExpectedO);
	}
}

void Assert::NoMonster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY) {
	LOG_INFO(file, line, "NoMonster");

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->GetMonsterAtSquare(wExpectedX, wExpectedY);

	REQUIRE(monster == NULL);
}

void Assert::Tile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType) {
	LOG_INFO(file, line, "Tile");

	REQUIRE(HasTile(wExpectedX, wExpectedY, wExpectedType));
}

void Assert::NoTile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType) {
	LOG_INFO(file, line, "NoTile");

	REQUIRE(!HasTile(wExpectedX, wExpectedY, wExpectedType));
}

void Assert::OrbState(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const OrbType expectedType) {
	LOG_INFO(file, line, "OrbState");

	CCurrentGame* game = Runner::GetCurrentGame();

	COrbData* pOrbData = game->pRoom->GetOrbAtCoords(wExpectedX, wExpectedY);
	if (!pOrbData) {
		INFO("No orb data on target tile, so we assume it's OT_NORMAL");
		REQUIRE(expectedType == OrbType::OT_NORMAL);
	}
	else
		REQUIRE(pOrbData->eType == expectedType);
}

bool Assert::HasTile(const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType) {
	CCurrentGame* game = Runner::GetCurrentGame();
	CDbRoom* room = game->pRoom;

	const UINT baseTile = bConvertFakeElement(wExpectedType);
	REQUIRE(IsValidTileNo(baseTile));

	switch (TILE_LAYER[baseTile])
	{
	case LAYER_OPAQUE:
		switch (baseTile) {
			default: return room->GetOSquare(wExpectedX, wExpectedY) == baseTile;
			case T_OVERHEAD_IMAGE: return room->overheadTiles.Exists(wExpectedX, wExpectedY);
		}
		break;
	case LAYER_TRANSPARENT:
	{
		const UINT wTTile = room->GetBottomTSquare(wExpectedX, wExpectedY);
		const UINT wTObject = room->GetTSquare(wExpectedX, wExpectedY);

		return wTTile == baseTile || wTObject == baseTile;
	}

	case LAYER_FLOOR:
		return room->GetFSquare(wExpectedX, wExpectedY) == baseTile;

	default:
		FAIL("Unknown tile layer");
		return false;
	}
}

#undef LOG_INFO