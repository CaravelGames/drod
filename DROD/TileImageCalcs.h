// $Id$

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TILEIMAGECALCS_H
#define TILEIMAGECALCS_H

#include "TileImageConstants.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/Weapons.h"
#include "../DRODLib/TileConstants.h"

#include <BackEndLib/Ports.h>

//Return value for functions that can't return a tile image.
static const UINT CALC_NEEDED = (UINT)(-1);

enum AnimationFrames {
	ANIMATION_FRAMES = 2, //number of frames monster animates through [0,ANIMATION_FRAMES)
	SWORD_FRAME = ANIMATION_FRAMES,
	SWORDLESS_FRAME,
	WADING_FRAME,
	SWORD_WADING_FRAME,
	WADING_FRAME2
};

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
UINT  GetSwordTile(const UINT wType, const UINT wO, const UINT weaponType=WT_Sword);
UINT  GetTileImageForBeethro(const UINT wO, const UINT wType, const UINT wAnimFrame);
UINT  GetTileImageForEntity(const UINT wType, const UINT wO, const UINT wAnimFrame);
UINT  GetTileImageForEntityOrDefault(const UINT wType, UINT wO, const UINT wAnimFrame);
UINT  GetTileImageForSerpentPiece(const UINT wType, const UINT wTileNo);
UINT  GetTileImageForRockGiantPiece(const UINT wTileIndex, const UINT wO, const UINT wAnimFrame);
UINT GetTileImageForGentryiiPiece(UINT x1, UINT y1, UINT x2, UINT y2);
UINT  GetTileImageForTileNo(const UINT wTileNo);
UINT  CalcTileImageForToken(const BYTE tParam);
UINT CalcTileImageForFloorSpikes(const CDbRoom *pRoom);
UINT  CalcTileImageForWater(const CDbRoom *pRoom, const UINT wCol, const UINT wRow, const UINT wTileNo, bool *pbAlternate = NULL);

enum WALLTYPE {WALL_NONE, WALL_EDGE, WALL_INNER};
WALLTYPE GetWallTypeAtSquare(const CDbRoom *pRoom, int nCol, int nRow);

void GetObstacleStats(const CDbRoom *pRoom, const UINT wCol, const UINT wRow,
		UINT& wObSizeIndex, UINT& xPos, UINT& yPos);
bool IsMonsterTypeAnimated(const UINT wType);

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

//Types for which wading graphics are implemented.
static inline bool bIsWadingMonsterType(const UINT mt) {
	switch (mt) {
		case M_BEETHRO:
		case M_GUNTHRO:
		case M_GUARD:
		case M_CLONE:
		case M_TEMPORALCLONE:
		case M_STALWART:
		case M_STALWART2:
		case M_MIMIC:
		case M_DECOY:
		case M_CITIZEN: case M_ARCHITECT:
		case M_HALPH: case M_HALPH2:
		case M_SLAYER: case M_SLAYER2:
		case M_GOBLIN:
		case M_ROCKGOLEM:
		case M_AUMTLICH:
		case M_CONSTRUCT:
			return true;
		default:
			return false;
	}
}

static inline bool bIsBriarTI(const UINT ti) {
	return ti == TI_BRIAR || ti == TI_BRIAR2 ||
		(ti >= TI_BRIAR_SE && ti <= TI_BRIAR_N2) ||
		(ti >= TI_BRIAREDGE_SE && ti <= TI_BRIARROOT_NW);
}

static const UINT MAX_OBSTACLE_TYPES = 64;
static const UINT MAX_OBSTACLE_SIZE = 5;
static const UINT TOTAL_OBSTACLE_TYPES = 256;

extern const UINT obstacleIndices[MAX_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE];
extern const UINT obstacleDimensions[TOTAL_OBSTACLE_TYPES][2];
extern const UINT obstacleTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE][MAX_OBSTACLE_SIZE];
extern const UINT obstacleShadowTile[TOTAL_OBSTACLE_TYPES][MAX_OBSTACLE_SIZE+1][MAX_OBSTACLE_SIZE+1];

#endif //...#ifndef TILEIMAGECALCS_H
