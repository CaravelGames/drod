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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TILEMASK_H
#define TILEMASK_H

#include "TileConstants.h"
#include <BackEndLib/Assert.h>

//Set values for specific tiles.
class CTileMask
{
public:
	CTileMask() {
		memset(bTileset, 0, TOTAL_TILE_COUNT * sizeof(BYTE));
	}
	CTileMask(const UINT wIndex, const BYTE val=1) {
		memset(bTileset, 0, TOTAL_TILE_COUNT * sizeof(BYTE));
		set(wIndex, val);
	}
	inline bool empty() const {return size() == 0;}
	inline BYTE get(const UINT wIndex) const {
		ASSERT(wIndex < TOTAL_TILE_COUNT);
		return this->bTileset[wIndex];
	}
	inline void reset(const UINT wIndex) {
		ASSERT(wIndex < TOTAL_TILE_COUNT);
		this->bTileset[wIndex] = 0;
	}
	inline void set(const UINT wIndex, const BYTE val=1) {
		ASSERT(wIndex < TOTAL_TILE_COUNT);
		this->bTileset[wIndex] = val;
	}
	UINT size() const {
		UINT wCount = 0;
		for (UINT i=TOTAL_TILE_COUNT; i--; )
			if (bTileset[i])
				++wCount;
		return wCount;
	}

private:
	BYTE bTileset[TOTAL_TILE_COUNT];
};

#endif	//...#ifndef TILEMASK_H
