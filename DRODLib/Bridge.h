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

//Class for handling bridge element game logic.

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
	void addBridge(const UINT wX, const UINT wY);
	void built(const UINT wX, const UINT wY, const UINT wTileNo);
	void clear();
	void plotted(const UINT wX, const UINT wY, const UINT wTileNo);
	void process(CCueEvents &CueEvents);
	void setMembersForRoom(const CBridge& src, CDbRoom* pRoom);
	void setRoom(CDbRoom *pRoom);

private:
	void drop(CCueEvents &CueEvents, const CCoordSet& tiles);
	bool supports(const UINT wTile) const;

	CDbRoom *pRoom;

	std::vector<CCoordSet> bridges; //connected components
	std::vector<CCoordSet> bridgeSupports; //tile sets
	CCoordSet ignoredTiles;
	vector<UINT> droppingBridges;

	static CTileMask bridgeMask;
};

#endif //...#ifndef BRIDGE_H
