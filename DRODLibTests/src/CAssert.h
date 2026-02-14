#ifndef CAssert_H
#define CAssert_H

#include "../../DRODLib/CurrentGame.h"
#include "../../DRODLib/Db.h"
#include "../../DRODLib/DbHolds.h"
#include "../../DRODLib/DbPlayers.h"
#include "../../DRODLib/DbLevels.h"
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Types.h>

#define DISPLAY_ROWS     32
#define DISPLAY_COLS     38

class Assert{
	friend class Runner;
	friend class RoomBuilder;

public:
	static void Event(const char* file, int line, const CUEEVENT_ID eEventType);
	static void NoEvent(const char* file, int line, const CUEEVENT_ID eEventType);
	static void PlayerAt(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY);
	static void PlayerIsAlive(const char* file, int line);
	static void PlayerIsDead(const char* file, int line);
	static void Tile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType);
	static void NoTile(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType);
	static void RoomHasMonster(const char* file, int line, const long int wExpectedType = -1);
	static void RoomHasNoMonster(const char* file, int line, const long int wExpectedType = -1);
	static void Monster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const long int wExpectedType = -1, const long int wExpectedO = -1);
	static void NoMonster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY);
	static void OrbState(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const OrbType expectedType);

private :
	static bool HasTile(const UINT wExpectedX, const UINT wExpectedY, const UINT wExpectedType);
};

#define AssertEvent(eEventType) Assert::Event(__FILE__, __LINE__, eEventType);
#define AssertNoEvent(eEventType) Assert::NoEvent(__FILE__, __LINE__, eEventType);
#define AssertPlayerAt(wExpectedX, wExpectedY) Assert::PlayerAt(__FILE__, __LINE__, wExpectedX, wExpectedY);
#define AssertPlayerIsAlive() Assert::PlayerIsAlive(__FILE__, __LINE__);
#define AssertPlayerIsDead() Assert::PlayerIsDead(__FILE__, __LINE__);
#define AssertMonster(wExpectedX, wExpectedY) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY);
#define AssertMonsterType(wExpectedX, wExpectedY, wExpectedType) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType);
#define AssertMonsterO(wExpectedX, wExpectedY, wExpectedO) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, -1, wExpectedO);
#define AssertMonsterTypeO(wExpectedX, wExpectedY, wExpectedType, wExpectedO) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType, wExpectedO);
#define AssertNoMonster(wExpectedX, wExpectedY) Assert::NoMonster(__FILE__, __LINE__, wExpectedX, wExpectedY);
#define AssertRoomHasMonster(wExpectedType) Assert::RoomHasMonster(__FILE__, __LINE__, wExpectedType);
#define AssertRoomHasNoMonster(wExpectedType) Assert::RoomHasNoMonster(__FILE__, __LINE__, wExpectedType);
#define AssertTile(wExpectedX, wExpectedY, wExpectedType) Assert::Tile(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType);
#define AssertNoTile(wExpectedX, wExpectedY, wExpectedType) Assert::NoTile(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType);
#define AssertOrbState(wExpectedX, wExpectedY, expectedType) Assert::OrbState(__FILE__, __LINE__, wExpectedX, wExpectedY, expectedType);


#endif