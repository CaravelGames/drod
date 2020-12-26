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
 * Portions created by the Initial Developer are Copyright (C) 2004
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Types.h"
#include "Coord.h"
#include "CoordIndex.h"

#ifndef COORDSET_H
#define COORDSET_H

#include <set>

class CCoordSet : public CAttachableObject
{
public:
	typedef std::set<ROOMCOORD> CoordSet;
	typedef CoordSet::const_iterator const_iterator;

	CCoordSet() {}
	CCoordSet(const UINT wX, const UINT wY) {insert(wX,wY);}

	inline void clear() {this->data.clear();}

	inline bool insert(const UINT wX, const UINT wY)
	{
		return insert(ROOMCOORD(wX,wY));
	}
	inline bool insert(ROOMCOORD const &cc)
	{
		return this->data.insert(cc).second;
	}
	inline void insert(CCoordSet::const_iterator begin, CCoordSet::const_iterator end)
	{
		this->data.insert(begin, end);
	}

	inline bool empty() const {return this->data.empty();}
	inline UINT size() const {return this->data.size();}

	inline bool erase(const UINT wX, const UINT wY)
	{
		return erase(ROOMCOORD(wX,wY));
	}
	inline bool erase(ROOMCOORD const &cc)
	{
		return this->data.erase(cc) != 0;
	}

	inline bool first(UINT &wX, UINT &wY) const
	{
		if (empty())
			return false;
		CoordSet::const_iterator iter = this->data.begin();
		wX = iter->wX; wY = iter->wY;
		return true;
	}
	bool pop_first(UINT &wX, UINT &wY)
	{
		if (empty())
			return false;
		CoordSet::iterator iter = this->data.begin();
		wX = iter->wX; wY = iter->wY;
		this->data.erase(iter);
		return true;
	}

	inline bool has(const UINT wX, const UINT wY) const
	{
		return has(ROOMCOORD(wX, wY));
	}
	inline bool has(ROOMCOORD const &cc) const
	{
		return this->data.count(cc) != 0;
	}

	inline const_iterator begin() const {return this->data.begin();}
	inline const_iterator end() const {return this->data.end();}

	void AddTo(CCoordIndex &coordIndex) const
	//Places all the coords in this object in coordIndex.
	{
		for (const_iterator trav = this->data.begin();
				trav != this->data.end(); ++trav)
		{
			//ASSUME: (wX,wY) are within the bounds of coordIndex
			coordIndex.Add(trav->wX, trav->wY);
		}
	}
	CCoordSet& operator=(const CCoordIndex& that)
	{
		clear();
		return operator+=(that);		
	}
	CCoordSet& operator+=(const CCoord& coord) {
		insert(coord);
		return *this;
	}
	CCoordSet& operator+=(const CCoordIndex& coordIndex)
	//Adds all instances of coords in 'coordIndex' to this.
	{
		if (coordIndex.GetSize()) {
			const BYTE *index = coordIndex.GetIndex();
			const UINT rows = coordIndex.GetRows();
			const UINT cols = coordIndex.GetCols();
			for (UINT wY=0; wY<rows; ++wY)
				for (UINT wX=0; wX<cols; ++wX, ++index)
					if (*index)
						insert(wX,wY);
		}
		return *this;
	}

	CCoordSet& operator+=(const CCoordSet& that)
	//Adds all instances of coords in 'that' to this.
	{
		for (const_iterator iter = that.data.begin();
				iter != that.data.end(); ++iter)
			insert(*iter);
		return *this;
	}
	CCoordSet& operator-=(const CCoordSet& that)
	//Removes all instances of coords in 'that' from this.
	{
		for (const_iterator iter = that.data.begin();
				iter != that.data.end(); ++iter)
			erase(*iter);
		return *this;
	}

private:
	CoordSet data;
};

#endif //...#ifndef COORDSET_H
