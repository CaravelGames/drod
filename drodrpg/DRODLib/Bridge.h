// $Id: Bridge.h 8102 2007-08-15 14:55:40Z trick $

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

//Class for handling bridge element game logic.

/*
Poor man's guide to understanding bridges.
Bridges are grouped into components which all fall together and have common supports. Supports
are only tiles which can be conceivably removed - trapdoors, thin ice et cetera.
Bridges which have permanent supports (eg. floor) are not added to the list of components, they're
treated as regular floor.
There are 4 main operation that happen on bridges:
1. Processing - Process() runs at the end of every turn, will trigger the falling of bridges that
  are marked to fall
2. Building a tile - HandleTileBuilt(), will take a tile coordinate and regenerate any bridges that are on that
  position or next to it.
  a) Called when Build command builds an O-Layer tile.
  b) Or when thin ice is formed
3. Generating bridge components - HandleBridgeAdded(), will take a bridge coordinate and make a bridge component out of it:
  a) It's called when room is first loaded to initially populate the bridge components
  b) Is called from CBridge::HandleTileBuilt() to regenerate components
4. Handling dropped supports - Plotted(), will remove supports and mark the bridge as dropping
*/

#ifndef BRIDGE_H
#define BRIDGE_H

#include "CueEvents.h"
#include "TileMask.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/IDSet.h>
#include <vector>

class CDbRoom;
class CBridge
{
public:
	CBridge();
	void HandleBridgeAdded(const UINT wX, const UINT wY);
	void HandleTileBuilt(const UINT wX, const UINT wY, const UINT wTileNo);
	void clear();
	void Plotted(const UINT wX, const UINT wY, const UINT wTileNo);
	void process(CCueEvents &CueEvents);
	void setMembersForRoom(const CBridge& src, CDbRoom* pRoom);
	void setRoom(CDbRoom *pRoom);

private:
	void DropBridgeTiles(CCueEvents &CueEvents, const CCoordSet& tiles);
	bool DoesTileSupportBridges(const UINT wTile) const;

	bool IsBridgeDropping(const UINT wBridgeIndex) const;

	CDbRoom *pRoom;

	std::vector<CCoordSet> bridges; // Each entry is a list of bridge tiles that form a connected component. bridgeSupports must have the same length.
	std::vector<CCoordSet> bridgeSupports; // Each entry is a list of non-bridge tiles which support the bridge of the same index and prevent it from falling
	CCoordSet ignoredTiles; // Used to avoid recalculating the same bridge over and over again while calling HandleBridgeAdded repeatedly
	vector<UINT> droppingBridges; // Vector of bridge component indexes which are meant to fall

	static CTileMask bridgeMask;
};

#endif //...#ifndef BRIDGE_H

