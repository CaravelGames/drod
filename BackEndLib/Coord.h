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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002 
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef COORD_H
#define COORD_H

#include "Types.h"
#include "AttachableObject.h"

#include <vector>

struct ROOMCOORD
{
	ROOMCOORD() : wX(0), wY(0) {}
	ROOMCOORD(const UINT wX, const UINT wY) : wX(wX), wY(wY) {}

	UINT wX, wY;
	inline bool operator==(const ROOMCOORD& rhs) const
	{
		return this->wX == rhs.wX && this->wY == rhs.wY;
	}
	inline bool operator<(const ROOMCOORD& rhs) const
	{
		if (wX < rhs.wX) return true;
		if (wX > rhs.wX) return false;
		return wY < rhs.wY;
	}
};

//Class for storing one set of coords.
class CCoord : public CAttachableObject
{
public:
	CCoord() : CAttachableObject() { }  //uninited coord
	CCoord(const UINT wX, const UINT wY) 
		: CAttachableObject(), wX(wX), wY(wY)
	{ }
	CCoord(const ROOMCOORD& coord)
		: CAttachableObject(), wX(coord.wX), wY(coord.wY)
	{ }
	operator ROOMCOORD() const {return ROOMCOORD(this->wX,this->wY);}

	bool equals(const UINT wX, const UINT wY) const
	{
		return this->wX == wX && this->wY == wY;
	}
	void set(const UINT wX, const UINT wY)
	{
		this->wX = wX;
		this->wY = wY;
	}

	static std::vector<CCoord> GetOrderedLineBetween(UINT x1, UINT y1, UINT x2, UINT y2);

	UINT wX, wY;
};

//Class for storing coords plus an orientation value.
class CMoveCoord : public CCoord
{
public:
	CMoveCoord() : CCoord() { } //uninited coord
	CMoveCoord(const UINT wX, const UINT wY, const UINT wO) 
		: CCoord(wX, wY), wO(wO)
	{ }
	UINT wO;
};

//Class for storing move coords plus an extra value
class CMoveCoordEx : public CMoveCoord
{
public:
	CMoveCoordEx(const UINT wX, const UINT wY, const UINT wO, const UINT wValue) 
		: CMoveCoord(wX, wY, wO), wValue(wValue)
	{ }
	UINT wValue;
};

//Class for storing move coords plus two extra values
class CMoveCoordEx2 : public CMoveCoordEx
{
public:
	CMoveCoordEx2(const UINT wX, const UINT wY, const UINT wO, const UINT wValue, const UINT wValue2) 
		: CMoveCoordEx(wX, wY, wO, wValue), wValue2(wValue2)
	{ }
	UINT wValue2;
};

#endif //...#ifndef COORD_H
