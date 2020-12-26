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
* 1997, 2000, 2001, 2002, 2005, 2016, 2020 Caravel Software. All Rights Reserved.
*
* Contributor(s):
*
* ***** END LICENSE BLOCK ***** */

#ifndef BUILD_UTIL_H
#define BUILD_UTIL_H

#include "CueEvents.h"
#include "DbRooms.h"

class BuildUtil;

//*****************************************************************************
class BuildUtil
{
public:
	static bool bIsValidBuildTile(const UINT wTileNo);

	static bool BuildTilesAt(CDbRoom& room, const UINT tile, UINT x, UINT y, const UINT w, const UINT h, const bool bAllowSame, CCueEvents& CueEvents);
	static bool BuildTileAt(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents);
	static bool CanBuildAt(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame);

private:
	static bool BuildVirtualTile(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents);
	static bool BuildRealTile(CDbRoom& room, const UINT tile, const UINT x, const UINT y, const bool bAllowSame, CCueEvents& CueEvents);
};

#endif //...#ifndef BUILD_UTIL_H