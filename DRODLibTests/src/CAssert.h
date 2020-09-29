#ifndef CAssert_H
#define CAssert_H

#include "../../DRODLib/CurrentGame.h"
#include "../../DRODLib/Db.h"
#include "../../DRODLib/DbHolds.h"
#include "../../DRODLib/DbPlayers.h"
#include "../../DRODLib/DbLevels.h"
#include <BackEndLib\Wchar.h>
#include <BackEndLib\Files.h>
#include <BackEndLib\Types.h>

#define DISPLAY_ROWS     32
#define DISPLAY_COLS     38

class Assert{
	friend class Runner;
	friend class RoomBuilder;

public:
	static void PlayerAt(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY);
	static void PlayerIsAlive(const char* file, int line);
	static void PlayerIsDead(const char* file, int line);
	static void Monster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY, const long int wExpectedType = -1, const long int wExpectedO = -1);
	static void NoMonster(const char* file, int line, const UINT wExpectedX, const UINT wExpectedY);
};

#define AssertPlayerAt(wExpectedX, wExpectedY) Assert::PlayerAt(__FILE__, __LINE__, wExpectedX, wExpectedY);
#define AssertPlayerIsAlive() Assert::PlayerIsAlive(__FILE__, __LINE__);
#define AssertPlayerIsDead() Assert::PlayerIsDead(__FILE__, __LINE__);
#define AssertMonster(wExpectedX, wExpectedY) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY);
#define AssertMonsterType(wExpectedX, wExpectedY, wExpectedType) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType);
#define AssertMonsterO(wExpectedX, wExpectedY, wExpectedO) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, -1, wExpectedO);
#define AssertMonsterTypeO(wExpectedX, wExpectedY, wExpectedType, wExpectedO) Assert::Monster(__FILE__, __LINE__, wExpectedX, wExpectedY, wExpectedType, wExpectedO);
#define AssertNoMonster(wExpectedX, wExpectedY) Assert::NoMonster(__FILE__, __LINE__, wExpectedX, wExpectedY);

#endif