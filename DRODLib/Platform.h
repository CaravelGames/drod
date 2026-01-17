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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <BackEndLib/CoordStack.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/IDSet.h>
#include "CueEvents.h"

class CCurrentGame;
class CDbRoom;
class CPlatform
{
public:
	CPlatform(const CCoordSet &blockSet, const CIDSet& types);
	void SetCurrentGame(CCurrentGame *pSetCurrentGame);

	static void checkForFalling(CDbRoom *pRoom, CCueEvents& CueEvents);
	static void clearFallTiles() {CPlatform::fallTiles.clear();}
	static bool fallTilesPending() {return !CPlatform::fallTiles.empty();}

	void AddTile(const UINT wX, const UINT wY);
	bool CanMove(const UINT wO);
	void GetTiles(CCoordSet& tiles) const;
	CIDSet GetTypes() const;
	bool IsAt(const UINT wX, const UINT wY) const;
	void Merge(CPlatform* other);
	void Move(const UINT wO);
	void Move(CDbRoom& room, const int nOX, const int nOY, const bool bPlotToRoom);
	void RemoveTile(const UINT wX, const UINT wY);
	void ReflectX(CDbRoom *pRoom);
	void ReflectY(CDbRoom *pRoom);

	CCurrentGame *pCurrentGame;

	int xOffset, yOffset; //offset from original location
	int xDelta, yDelta;   //offset for this turn

private:
	enum PlatformDir {North, South, East, West, DIR_COUNT};

	bool CanMoveTo(CDbRoom* pRoom, const UINT wX, const UINT wY) const;
	void PrepareEdgeSet();

	CCoordStack blocks;  //tile coords that compose the platform
	vector<UINT> edgeBlocks[DIR_COUNT];   //indices of leading edge blocks
	CIDSet types; //located on what tile type(s)

	static CCoordSet fallTiles; //tiles platforms moved off of since last falling check
};

#endif //...#ifndef PLATFORM_H
