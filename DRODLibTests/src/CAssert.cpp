#include "CAssert.h"

#include "Runner.h"
#include "test-include.hpp"

#include <sstream>

const char* MakeLogMessage(const char* file, int line) {
	std::stringstream message;
	message << "From: ";
	message << file;
	message << "(";
	message << line;
	message << ")";

	return message.str().c_str();
}

void Assert::Event(const char* file, int line, const CUEEVENT_ID eEventType) {
	INFO(MakeLogMessage(file, line));

	CCueEvents& CueEvents = Runner::GetLastCueEvents();

	REQUIRE(CueEvents.HasOccurred(eEventType));
}

void Assert::NoEvent(const char* file, int line, const CUEEVENT_ID eEventType) {
	INFO(MakeLogMessage(file, line));

	CCueEvents& CueEvents = Runner::GetLastCueEvents();

	REQUIRE(!CueEvents.HasOccurred(eEventType));
}

void Assert::PlayerAt(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY) {
	INFO(MakeLogMessage(file, line));

	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(game->swordsman.wX == wExpectedX);
	REQUIRE(game->swordsman.wY == wExpectedY);
}

void Assert::PlayerIsAlive(const char* file, int line) {
	INFO(MakeLogMessage(file, line));

	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(!game->IsPlayerDying());
}

void Assert::PlayerIsDead(const char* file, int line) {
	CCurrentGame* game = Runner::GetCurrentGame();

	REQUIRE(game->IsPlayerDying());
}

void Assert::RoomHasMonster(const char* file, int line, const long int wExpectedType) {
	INFO(MakeLogMessage(file, line));

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->MonsterOfTypeExists(wExpectedType);

	REQUIRE(monster != NULL);
}

void Assert::RoomHasNoMonster(const char* file, int line, const long int wExpectedType) {
	INFO(MakeLogMessage(file, line));

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->MonsterOfTypeExists(wExpectedType);

	REQUIRE(monster == NULL);
}

void Assert::Monster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const long int wExpectedType, const long int wExpectedO) {
	INFO(MakeLogMessage(file, line));

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
	INFO(MakeLogMessage(file, line));

	CCurrentGame* game = Runner::GetCurrentGame();
	CMonster* monster = game->pRoom->GetMonsterAtSquare(wExpectedX, wExpectedY);

	REQUIRE(monster == NULL);
}

void Assert::Tile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType) {
	INFO(MakeLogMessage(file, line));

	REQUIRE(HasTile(wExpectedX, wExpectedY, wExpectedType));
}

void Assert::NoTile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType) {
	INFO(MakeLogMessage(file, line));

	REQUIRE(!HasTile(wExpectedX, wExpectedY, wExpectedType));
}

void Assert::OrbState(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const OrbType expectedType) {
	INFO(MakeLogMessage(file, line));

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