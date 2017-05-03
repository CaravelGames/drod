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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Class for calculating brain-directed monster movement for a room.

//FUTURE CONSIDERATIONS
//
//Force arrows are currently considered obstacles to brain-directed movement.
//
//We need to leave the inaccurate pathfinding in for compatibility, as Serpents
//are unable to move over force arrows, and some rooms depend on this logic,
//but it is entirely possible to later have a "Roach II", "Queen Roach II",
//"Serpent II", etc. that take advantage of better pathfinding.
//The old monsters would still find their old paths in the old rooms.

#ifndef PATHMAP_H
#define PATHMAP_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>

#include <queue>
#include <string>
#include <vector>

#define DMASK_NONE 0x00
#define DMASK_N    0x01
#define DMASK_NE   0x02
#define DMASK_E    0x04
#define DMASK_SE   0x08
#define DMASK_S    0x10
#define DMASK_SW   0x20
#define DMASK_W    0x40
#define DMASK_NW   0x80
#define DMASK_ALL  0xff

//Path map square that contains only information needed for determining paths.
enum DIRECTION {nw, n, ne, w, none, e, sw, s, se, DIR_COUNT};

static inline DIRECTION operator--(DIRECTION &d) { return d=static_cast<DIRECTION>(d-1);}
static inline DIRECTION operator--(DIRECTION &d, int) { DIRECTION const t=d; --d; return t; }
static inline DIRECTION operator++(DIRECTION &d) { return d=static_cast<DIRECTION>(d+1);}
static inline DIRECTION operator++(DIRECTION &d, int) { DIRECTION const t=d; ++d; return t; }

struct SQUARE
{
	//Directions that are blocked by obstacles or partial obstacles to/from this square
	//(assuming all surrounding squares are open).
	UINT eBlockedDirections;

	UINT dwTargetDist, dwSteps;
};

//Used for sorting points in QSortPoint().
class SORTPOINT
{
public:
	SORTPOINT() { }
	SORTPOINT(const UINT wX, const UINT wY, const UINT dwScore=0, const UINT wMoves=0)
		: wX(wX), wY(wY), wMoves(wMoves), dwScore(dwScore) { }
	UINT wX, wY, wMoves;
	UINT dwScore;
	bool operator <(const SORTPOINT& rhs) const { return this->dwScore >= rhs.dwScore; }
};
typedef std::vector<SORTPOINT> SORTPOINTS;

class CCoord;
class CPathMap
{
public:
	CPathMap(const UINT wCols, const UINT wRows,
			const UINT xTarget=(UINT)-1, const UINT yTarget=(UINT)-1,
			const UINT dwPathThroughObstacleCost=(UINT)-1,
			const bool bSupportPartialObstacles=false);
	CPathMap(const CPathMap &Src) {SetMembers(Src);}
	CPathMap &operator= (const CPathMap &Src) {
		SetMembers(Src);
		return *this;
	}

	void CalcPaths();
	void GetDebugOutput(bool bShowDirection, bool bShowState, bool bShowDistance,
		std::string &strOutput) const;
	void GetEntrances(SORTPOINTS& sortPoints);
	void GetRecPaths(const UINT wX, const UINT wY, SORTPOINTS& sortPoints);
	const SQUARE& GetSquare(const UINT wX, const UINT wY);
	void Reset();
	void SetMembers(const CPathMap& Src);
	void SetSquare(const UINT wX, const UINT wY, UINT eBlockedDirections);
	void SetTarget(const UINT xTarget, const UINT yTarget);

	UINT wCols;
	UINT wRows;
	std::vector<SQUARE> squares;

private:
	inline UINT       GetSquareIndex(const UINT x, const UINT y) const;
	static void       StableSortPoints(SORTPOINTS& sortPoints);

	UINT xTarget, yTarget;
	std::priority_queue<SORTPOINT> recalcSquares;

	//closest entrance squares to target
	std::priority_queue<SORTPOINT> entrySquares;

	//This value is set for use when no obstacle-free path to the target exists,
	//but a mostly-valid path needs to be calculated anyway.
	UINT dwPathThroughObstacleCost;

	//Support force arrows, orthosquares, etc. If false (default), these are
	//treated as full obstacles.
	bool bSupportPartialObstacles;
};

#endif //...#ifndef PATHMAP_H
