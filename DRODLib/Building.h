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

//Class for handling tile building game logic.

#ifndef BUILDING_H
#define BUILDING_H

#include "CueEvents.h"
#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/Types.h>

class CDbRoom;
class CBuilding
{
public:
	CBuilding();

	void clear();
	inline bool empty() const {return this->tiles.empty();}
	UINT get(const UINT wX, const UINT wY) const;
	void init(const UINT wCols, const UINT wRows);
	void plot(const UINT wX, const UINT wY, const UINT wTile);
	void remove(const UINT wX, const UINT wY);
	void setMembers(const CBuilding& src);

	CCoordSet tileSet;
private:
	CCoordIndexUint tiles;
};

#endif //...#ifndef BUILDING_H
