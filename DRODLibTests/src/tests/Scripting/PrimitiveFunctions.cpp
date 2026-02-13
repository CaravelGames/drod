#include "../../test-include.hpp"
#include "../../CAssert.h"

static void TestValidity(const wstring& expression, CDbHold* pHold, bool bValid = true) {
	// Convert std::wstring (wchar_t-based) to project WSTRING (WCHAR-based)
	WSTRING wexpr;
	wexpr.reserve(expression.size());
	for (wchar_t ch : expression)
		wexpr.push_back(std::char_traits<WCHAR>::to_char_type((WCHAR_t)ch));

	UINT index = 0;
	CHECK(CCharacter::IsValidExpression(wexpr.c_str(), index, pHold) == bValid);
}

static void TestParsedValue(
	wstring expression,
	int expectedValue,
	CCurrentGame* pGame,
	CCharacter* pCharacter
) {
	// Convert std::wstring (wchar_t-based) to project WSTRING (WCHAR-based)
	WSTRING wexpr;
	wexpr.reserve(expression.size());
	for (wchar_t ch : expression)
		wexpr.push_back(std::char_traits<WCHAR>::to_char_type((WCHAR_t)ch));

	UINT index = 0;
	CHECK(CCharacter::parseExpression(wexpr.c_str(), index, pGame, pCharacter) == expectedValue);
}

TEST_CASE("Scripting: Primitive Functions", "[game][scripting][functions]") {
	RoomBuilder::ClearRoom();

	SECTION("Mathematical Functions") {
		CCurrentGame* pGame = Runner::StartGame(10, 10);
		CDbHold* pHold = pGame->pHold;
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 11);

		// Absolute value
		wstring absExpression(L"_abs(-5)");
		wstring absExpression2(L"_abs(7)");
		wstring absBad(L"_abs(-6");

		TestValidity(absExpression, pHold);
		TestValidity(absExpression2, pHold);
		TestValidity(absBad, pHold, false);

		TestParsedValue(absExpression, 5, pGame, pCharacter);
		TestParsedValue(absExpression2, 7, pGame, pCharacter);

		// Minimum and maximum
		wstring minExpression(L"_min(2,6)");
		wstring maxExpression(L"_max(-7, -3)");
		wstring nestedMin(L"_min(_min(-1, 3), _min(4, 22))");
		wstring nestedMax(L"_max(_max(-1, 3), _max(4, 22))");
		wstring nestedMinMax(L"_min(_max(-1, 3), _max(4, 22))");
		wstring nestedMaxMin(L"_max(_min(-1, 3), _min(4, 22))");

		TestValidity(minExpression, pHold);
		TestValidity(maxExpression, pHold);
		TestValidity(nestedMin, pHold);
		TestValidity(nestedMax, pHold);
		TestValidity(nestedMinMax, pHold);
		TestValidity(nestedMaxMin, pHold);

		TestParsedValue(minExpression, 2, pGame, pCharacter);
		TestParsedValue(maxExpression, -3, pGame, pCharacter);
		TestParsedValue(nestedMin, -1, pGame, pCharacter);
		TestParsedValue(nestedMax, 22, pGame, pCharacter);
		TestParsedValue(nestedMinMax, 3, pGame, pCharacter);
		TestParsedValue(nestedMaxMin, 4, pGame, pCharacter);

		// Distances
		wstring distLInf(L"_dist0(5,5,7,12)");
		wstring distL1(L"_dist1(2,2,4,5)");
		wstring distL2(L"_dist2(1,2,4,6)");

		TestValidity(distLInf, pHold);
		TestValidity(distL1, pHold);
		TestValidity(distL2, pHold);

		TestParsedValue(distLInf, 7, pGame, pCharacter);
		TestParsedValue(distL1, 5, pGame, pCharacter);
		TestParsedValue(distL2, 5, pGame, pCharacter);
	}

	SECTION("DROD Math Functions") {
		CCurrentGame* pGame = Runner::StartGame(10, 10);
		CDbHold* pHold = pGame->pHold;
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 11);

		// Orientations
		TestValidity(L"_ox(3)", pHold);
		TestValidity(L"_oy(7)", pHold);

		for (int wO = 0; wO <= 8; wO++) {
			if (wO == NO_ORIENTATION) {
				continue;
			}

			TestParsedValue(
				wstring(L"_rotateCW(") + to_wstring(wO) + wstring(L")"),
				nNextCO(wO),
				pGame,
				pCharacter
			);
			TestParsedValue(
				wstring(L"_rotateCCW(") + to_wstring(wO) + wstring(L")"),
				nNextCCO(wO),
				pGame,
				pCharacter
			);
			TestParsedValue(
				wstring(L"_ox(") + to_wstring(wO) + wstring(L")"),
				nGetOX(wO),
				pGame,
				pCharacter
			);
			TestParsedValue(
				wstring(L"_oy(") + to_wstring(wO) + wstring(L")"),
				nGetOY(wO),
				pGame,
				pCharacter
			);
		}

		wstring orientExpression(L"_orient(1,1)");
		wstring orientExpression2(L"_orient(-5,7)");
		wstring badOX(L"_ox(11)");
		wstring badOY(L"_oy(-6)");

		TestValidity(badOX, pHold);
		TestValidity(badOY, pHold);
		TestValidity(orientExpression, pHold);
		TestValidity(orientExpression2, pHold);

		TestParsedValue(orientExpression, SE, pGame, pCharacter);
		TestParsedValue(orientExpression2, SW, pGame, pCharacter);
		TestParsedValue(badOX, 11, pGame, pCharacter);
		TestParsedValue(badOY, -6, pGame, pCharacter);

		wstring noOrientCW(L"_rotateCW(4)");
		wstring noOrientCCW(L"_rotateCCW(4)");
		TestParsedValue(noOrientCW, NO_ORIENTATION, pGame, pCharacter);
		TestParsedValue(noOrientCCW, NO_ORIENTATION, pGame, pCharacter);

		// Facing
		wstring facingExpression(L"_facing(2,2)");
		wstring facingExpression2(L"_facing(0,-3)");
		wstring facingExpression3(L"_facing(1,5)");
		wstring facingExpression4(L"_facing(0,0)");

		TestValidity(facingExpression, pHold);
		TestValidity(facingExpression2, pHold);
		TestValidity(facingExpression3, pHold);
		TestValidity(facingExpression4, pHold);

		TestParsedValue(facingExpression, SE, pGame, pCharacter);
		TestParsedValue(facingExpression2, N, pGame, pCharacter);
		TestParsedValue(facingExpression3, S, pGame, pCharacter);
		TestParsedValue(facingExpression4, NO_ORIENTATION, pGame, pCharacter);

		// Rotation distance
		wstring rotationDist(L"_rotateDist(2,6)");
		wstring rotationDist2(L"_rotateDist(1,3)");
		wstring rotationDist3(L"_rotateDist(7,5)");
		wstring rotationDistSame(L"_rotateDist(1,1)");
		wstring rotationDistNo(L"_rotateDist(4,7)");
		wstring rotationDistBad(L"_rotateDist(12,1)");

		TestValidity(rotationDist, pHold);
		TestValidity(rotationDist2, pHold);
		TestValidity(rotationDist3, pHold);
		TestValidity(rotationDistSame, pHold);
		TestValidity(rotationDistNo, pHold);
		TestValidity(rotationDistBad, pHold);

		TestParsedValue(rotationDist, 4, pGame, pCharacter);
		TestParsedValue(rotationDist2, 2, pGame, pCharacter);
		TestParsedValue(rotationDist3, 2, pGame, pCharacter);
		TestParsedValue(rotationDistSame, 0, pGame, pCharacter);
		TestParsedValue(rotationDistNo, 0, pGame, pCharacter);
		TestParsedValue(rotationDistBad, 0, pGame, pCharacter);
	}

	SECTION("Room Information Functions") {
		// Room setup
		RoomBuilder::Plot(T_ARROW_N, 5, 15);
		RoomBuilder::Plot(T_ARROW_S, 6, 15);
		RoomBuilder::Plot(T_ARROW_NW, 7, 15);

		RoomBuilder::Plot(T_FIRETRAP, 5, 16);
		RoomBuilder::Plot(T_NODIAGONAL, 5, 16);
		RoomBuilder::Plot(T_BOMB, 5, 16);

		RoomBuilder::AddMonster(M_ROACH, 5, 17);
		RoomBuilder::AddMonster(M_GELMOTHER, 6, 17);
		CMonster* pSnake = RoomBuilder::AddMonster(M_SERPENT, 7, 17);
		CSerpent* pSerpent = DYN_CAST(CSerpent*, CMonster*, pSnake);
		RoomBuilder::AddSerpentPiece(pSerpent, 8, 17);

		CCurrentGame* pGame = Runner::StartGame(10, 10);
		CDbHold* pHold = pGame->pHold;
		CCharacter* pCharacter = RoomBuilder::AddVisibleCharacter(10, 11);

		// Arrow direction
		wstring arrowDir(L"_ArrowDir(5,15)");
		wstring arrowDir2(L"_ArrowDir(6,15)");
		wstring arrowDir3(L"_ArrowDir(7,15)");
		wstring arrowDirNone(L"_ArrowDir(8,15)");

		TestValidity(arrowDir, pHold);
		TestValidity(arrowDir2, pHold);
		TestValidity(arrowDir3, pHold);
		TestValidity(arrowDirNone, pHold);

		TestParsedValue(arrowDir, N, pGame, pCharacter);
		TestParsedValue(arrowDir2, S, pGame, pCharacter);
		TestParsedValue(arrowDir3, NW, pGame, pCharacter);
		TestParsedValue(arrowDirNone, NO_ORIENTATION, pGame, pCharacter);

		// Room Tile
		wstring roomTileO(L"_RoomTile(5,16,0)");
		wstring roomTileF(L"_RoomTile(5,16,1)");
		wstring roomTileT(L"_RoomTile(5,16,2)");
		wstring roomTileInvalidLayer(L"_RoomTile(5,16,3)");

		TestValidity(roomTileO, pHold);
		TestValidity(roomTileF, pHold);
		TestValidity(roomTileT, pHold);
		TestValidity(roomTileInvalidLayer, pHold);

		TestParsedValue(roomTileO, T_FIRETRAP, pGame, pCharacter);
		TestParsedValue(roomTileF, T_NODIAGONAL, pGame, pCharacter);
		TestParsedValue(roomTileT, T_BOMB, pGame, pCharacter);
		TestParsedValue(roomTileInvalidLayer, 0, pGame, pCharacter);

		// Monster Type
		wstring monsterType(L"_MonsterType(5,17)");
		wstring monsterType2(L"_MonsterType(6,17)");
		wstring monsterType3(L"_MonsterType(7,17)");
		wstring monsterTypeBig(L"_MonsterType(8,17)"); // snake piece

		TestValidity(monsterType, pHold);
		TestValidity(monsterType2, pHold);
		TestValidity(monsterType3, pHold);
		TestValidity(monsterTypeBig, pHold);

		TestParsedValue(monsterType, M_ROACH, pGame, pCharacter);
		TestParsedValue(monsterType2, M_GELMOTHER, pGame, pCharacter);
		TestParsedValue(monsterType3, M_SERPENT, pGame, pCharacter);
		TestParsedValue(monsterTypeBig, M_SERPENT, pGame, pCharacter);
	}
}
