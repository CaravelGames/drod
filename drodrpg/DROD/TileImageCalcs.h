// $Id: TileImageCalcs.h 8948 2008-04-28 13:07:13Z mrimer $

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TILEIMAGECALCS_H
#define TILEIMAGECALCS_H

#include "TileImageConstants.h"
#include "../DRODLib/MonsterFactory.h"

#include <BackEndLib/Ports.h>

//Return value for functions that can't return a tile image.
static const UINT CALC_NEEDED = (UINT)(-1);

const UINT ANIMATION_FRAMES = 2; //number of frames monster animates through [0,ANIMATION_FRAMES)
const UINT SWORD_FRAME = ANIMATION_FRAMES;
const UINT SWORDLESS_FRAME = SWORD_FRAME+1;

class CDbRoom;
class CMonster;
struct EDGES;

enum EDGETYPE {EDGE_NONE, EDGE_FLOOR, EDGE_WALL, EDGE_COUNT};
EDGETYPE CalcEdge(const UINT wTile1No, const UINT wTile2No, const UINT side);

void  CalcStairPosition(const CDbRoom *pRoom, const UINT wCol, const UINT wRow,
								UINT& wStairsCol, UINT& wStairsRow);
void  CalcStairUpPosition(const CDbRoom *pRoom, const UINT wCol, const UINT wRow,
								UINT& wStairsCol, UINT& wStairsRow);
void  CalcTileCoordForPit(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, EDGES* edges);
UINT  CalcTileImageFor(const CDbRoom *pRoom, const UINT wTileNo, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForOSquare(const CDbRoom *pRoom, UINT wCol, UINT wRow);
UINT  CalcTileImageForTSquare(const CDbRoom *pRoom, UINT wCol, UINT wRow);
UINT  CalcTileImageForCoveredTSquare(const CDbRoom *pRoom, UINT wCol, UINT wRow);
UINT  CalcTileImagesForWallShadow(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
bool  CastsWallShadow(const UINT t);
UINT  GetSwordlessEntityTile(const UINT wType, const UINT wO);
UINT  GetPredefinedSwordTile(const UINT wType, const UINT wO, const UINT sword=NoSword);
UINT  GetTileImageForEntity(const UINT wType, const UINT wO, const UINT wAnimFrame);
UINT  GetTileImageForSerpentPiece(const UINT wType, const UINT wTileNo);
UINT  GetTileImageForRockGiantPiece(const UINT wTileNo, const UINT wO, const UINT wFrame);
UINT  GetTileImageForTileNo(const UINT wTileNo);
UINT  GetTileImageForMapIcon(const ScriptVars::MapIcon mapIcon);
UINT  CalcTileImageForKey(const BYTE tParam);
UINT  CalcTileImageForSword(const BYTE tParam);
UINT  CalcTileImageForShield(const BYTE tParam);
UINT  CalcTileImageForAccessory(const BYTE tParam);
UINT  CalcTileImageForStairs(const CDbRoom *pRoom, const UINT wCol, const UINT wRow);
UINT  CalcTileImageForToken(const BYTE tParam);
UINT  CalcTileImageForWater(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, const UINT wTileNo);
UINT  GetTileImageForEntityOrDefault(const UINT wType, UINT wO, const UINT wAnimFrame);

enum WALLTYPE {WALL_NONE, WALL_EDGE, WALL_INNER};
WALLTYPE GetWallTypeAtSquare(const CDbRoom *pRoom, int nCol, int nRow);

void GetObstacleStats(const CDbRoom *pRoom, const UINT wCol, const UINT wRow,
		UINT& wObSizeIndex, UINT& xPos, UINT& yPos);

BYTE GetMistCorners(const CDbRoom* pRoom, const UINT wCol, const UINT wRow);

//Monster tiles.
//Determined by orientation and animation frame.
static const UINT DONT_USE = (UINT)-1;
extern const UINT MonsterTileImageArray[MONSTER_TYPES][ORIENTATION_COUNT];
extern const UINT AnimatedMonsterTileImageArray[MONSTER_TYPES][ORIENTATION_COUNT];
extern const UINT CharacterTileImageArray[CHARACTER_TYPES-CHARACTER_FIRST][ORIENTATION_COUNT];

static inline bool bIsMosaic(const UINT t) {return bIsPlainFloor(t) || bIsPit(t) || t==T_WALL_IMAGE;}

//Whether a shadow can fall on a room square of this type.
static inline bool bShowsShadow(const UINT o) {
	return bIsFloor(o) || bIsOpenDoor(o) || bIsTunnel(o) || bIsWater(o) || bIsPlatform(o) || bIsPit(o);
}
static inline bool bObstacleCastsShadowFrom(const UINT from, const UINT to) {
	switch (from)
	{
		//Obstacles in pits cast a shadow on the pit.
		case T_PIT: case T_PIT_IMAGE: return bIsPit(to);

		default:
			//From nowhere else will an obstacle cast a shadow on a pit.
			if (bIsPit(to)) return false;

			//Otherwise, obstacle will cast a shadow when at or above the point of shadow.
			return bShowsShadow(to) || bIsStairs(to) || !bShowsShadow(from);
	}
}

static const UINT MAX_OBSTACLE_TYPES = 64;
static const UINT MAX_OBSTACLE_SIZE = 5;
static const UINT TOTAL_OBSTACLE_TYPES = 256;

extern const UINT obstacleIndices[MAX_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE];
extern const UINT obstacleDimensions[TOTAL_OBSTACLE_TYPES][2];
extern const UINT obstacleTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE][MAX_OBSTACLE_SIZE];
extern const UINT obstacleShadowTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE+1][MAX_OBSTACLE_SIZE+1];

#endif //...#ifndef TILEIMAGECALCS_H
