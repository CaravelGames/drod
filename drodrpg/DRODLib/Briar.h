// $Id: Briar.h 8452 2008-01-03 19:27:30Z mrimer $

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

#ifndef BRIAR_H
#define BRIAR_H

#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include "CueEvents.h"
#include <list>
#include <utility>
#include <vector>

class CDbRoom;

//*****************************************************************************
class CBriars
{
public:
	CBriars();
	~CBriars();

	void clear();
	bool empty() const;
	
	void getBriarTilesConnectedToRoots(CCoordSet& existingBriarTiles, const bool b8Neighbor);

	void insert(const UINT wX, const UINT wY);
	void plotted(const UINT wX, const UINT wY, const UINT wTileNo);
	void process(CCueEvents &CueEvents);
	void removeSource(const UINT wX, const UINT wY);
	void setBriarTilesLife();
	void setMembersForRoom(const CBriars& src, CDbRoom* pRoom);
	void setRoom(CDbRoom* pRoom);

private:
	CDbRoom   *pRoom;
	CCoordSet roots;             //briar roots in the room
};

#endif //...#ifndef BRIAR_H

