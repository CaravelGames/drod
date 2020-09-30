#include "CAssert.h"

#include "Runner.h"
#include "test-include.hpp"
#include <iostream>
#include <string>

const char* MakeLogMessage(const char* file, int line) {
	std::stringstream message;
	message << "From: ";
	message << file;
	message << "(";
	message << line;
	message << ")";

	return message.str().c_str();
}

#define CatchLog(message, file, line) \
	Catch::ScopedMessage INTERNAL_CATCH_UNIQUE_NAME(catchlog) = Catch::MessageBuilder(\
		"Info", \
		::Catch::SourceLineInfo( file, static_cast<std::size_t>( line ) ), \
		Catch::ResultWas::Info \
	) << message;

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

	CCurrentGame* game = Runner::GetCurrentGame();
	CDbRoom* room = game->pRoom;

	const UINT baseTile = bConvertFakeElement(wExpectedType);
	REQUIRE(IsValidTileNo(baseTile));
	
	switch (TILE_LAYER[baseTile])
	{
		case LAYER_OPAQUE:
			switch (baseTile) {
				default: REQUIRE(room->GetOSquare(wExpectedX, wExpectedY) == baseTile);  break;
				case T_OVERHEAD_IMAGE: REQUIRE(room->overheadTiles.Exists(wExpectedX, wExpectedY)); break;
			}
			break;
		case LAYER_TRANSPARENT:
		{
			const UINT wTTile = room->GetBottomTSquare(wExpectedX, wExpectedY);
			const UINT wTObject = room->GetTSquare(wExpectedX, wExpectedY);

			REQUIRE((wTTile == baseTile || wTObject == baseTile));
		}
		break;
	case LAYER_FLOOR:
		REQUIRE(room->GetFSquare(wExpectedX, wExpectedY) == baseTile);
		break;
	default:
		FAIL("Unknown tile layer");
		break;
	}
}