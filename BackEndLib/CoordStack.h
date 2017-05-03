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

#ifndef COORDSTACK_H
#define COORDSTACK_H

#include "Types.h"
#include "Coord.h"
#include "CoordSet.h"

#include <algorithm>
#include <queue>

//A class that can perform as a stack or queue, holding coordinate pairs.
class CCoordStack
{
public:
	CCoordStack()
	{
	}
	CCoordStack(const CCoordStack& rhs)
	{
		this->data = rhs.data;
	}
	CCoordStack(const UINT wX, const UINT wY)
	{
		Push(wX,wY);
	}

	CCoordStack& operator=(const CCoordStack& rhs)
	{
		this->data = rhs.data;
		return *this;
	}
	CCoordStack& operator=(const CCoordSet& rhs)
	{
		Clear();
		for (CCoordSet::const_iterator iter=rhs.begin(); iter!=rhs.end(); ++iter)
			Push(iter->wX,iter->wY);
		return *this;
	}
	CCoordStack& operator+=(const CCoordStack& rhs)
	{
		for (std::deque<ROOMCOORD>::const_iterator coord = rhs.data.begin();
				coord != rhs.data.end(); ++coord)
			Push(coord->wX, coord->wY);
		return *this;
	}

	void AddTo(CCoordSet &coordSet) const
	//Places all the coords in this object in coordSet.
	{
		for (std::deque<ROOMCOORD>::const_iterator coord = this->data.begin();
				coord != this->data.end(); ++coord)
			coordSet.insert(coord->wX, coord->wY);
	}

	bool Pop()  //w/o parameters
	{
		if (IsEmpty())
			return false;
		this->data.pop_back();
		return true;
	}
	bool Pop(UINT &wX, UINT &wY)
	{
		if (IsEmpty())
			return false;

		const ROOMCOORD& last = this->data.back();
		wX = last.wX;
		wY = last.wY;

		this->data.pop_back();
		return true;
	}
	bool PopBottom(UINT &wX, UINT &wY)	//use to make stack behave as queue
	{
		if (IsEmpty())
			return false;

		const ROOMCOORD& first = this->data.front();
		wX = first.wX;
		wY = first.wY;

		this->data.pop_front();
		return true;
	}
	inline void Push(const UINT wX, const UINT wY)
	{
		this->data.push_back(ROOMCOORD(wX,wY));
	}
	inline void PushBottom(const UINT wX, const UINT wY)
	//Add element to bottom of stack.
	{
		this->data.push_front(ROOMCOORD(wX,wY));
	}
	bool Top(UINT &wX, UINT &wY) const
	//Return value in top of stack w/o removing it.
	{
		if (IsEmpty())
			return false;

		const ROOMCOORD& last = this->data.back();
		wX = last.wX;
		wY = last.wY;

		return true;
	}
	bool GetAt(const UINT wIndex, UINT &wX, UINT &wY) const
	//Return values in indexth element w/o removing it.
	{
		//Check for invalid position
		if (wIndex >= GetSize())
			return false;

		//Set vars for return.
		const ROOMCOORD& coord = this->data[wIndex];
		wX = coord.wX;
		wY = coord.wY;

		return true;
	}
	bool SetAt(const UINT wIndex, const UINT wX, const UINT wY)
	//Sets values in indexth element.
	{
		//Check for invalid position
		if (wIndex >= GetSize())
			return false;

		//Set vars.
		ROOMCOORD& coord = this->data[wIndex];
		coord.wX = wX;
		coord.wY = wY;

		return true;
	}
	inline bool IsMember(const CCoord& coord) const {return IsMember(coord.wX, coord.wY);}
	inline bool IsMember(const UINT wX, const UINT wY) const
	//Checks whether (wX,wY) is in the structure.
	{
		return std::find(this->data.begin(), this->data.end(), ROOMCOORD(wX,wY)) != this->data.end();
	}
	bool Remove(const UINT wX, const UINT wY)
	//Removes element (wX,wY) from the list.
	{
		std::deque<ROOMCOORD>::iterator found =
			std::find(this->data.begin(), this->data.end(), ROOMCOORD(wX,wY));
		if (found == this->data.end())
			return false;

		this->data.erase(found);
		return true;
	}
	inline void Clear()
	{
		this->data.clear();
	}
	inline UINT GetSize() const
	{
		return this->data.size();
	}
	inline bool IsEmpty() const
	{
		return this->data.empty();
	}

private:
	std::deque<ROOMCOORD> data;
};

#endif //...#ifndef COORDSTACK_H
