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
 * Portions created by the Initial Developer are Copyright (C) 2004 Caravel
 * Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//IDSet.h
//Declarations for CIDSet.
//Class for managing a set of ID values.

#ifndef IDSET_H
#define IDSET_H

#include "Types.h"
#include "IDList.h"

#include <set>
#include <vector>

class CIDSet
{
public:
	typedef std::set<UINT> IDSet;
	typedef IDSet::iterator iterator;
	typedef IDSet::const_iterator const_iterator;
	typedef IDSet::const_reverse_iterator const_reverse_iterator;

	CIDSet() {}
	CIDSet(const CIDSet& that) {this->ids = that.ids;}
	CIDSet(const std::vector<UINT>& idVector) {
		const size_t size = idVector.size();
		for (UINT i=0; i<size; ++i)
			this->ids.insert(idVector[i]);
	}
	CIDSet(const UINT dwID) {this->ids.insert(dwID);}

	inline bool  empty() const {return this->ids.empty();}
	inline bool  has(const UINT dwID) const {return this->ids.count(dwID) != 0;}
	inline UINT  size() const {return this->ids.size();}

	inline void  clear() {this->ids.clear();}
	inline int   erase(const UINT dwID) {return this->ids.erase(dwID);}
	inline iterator erase(const iterator iter) {
#if defined _MSC_VER
		return this->ids.erase(iter);
#else
		iterator newiter=iter;
		++newiter;
		this->ids.erase(iter);
		return newiter;
#endif
	}

	bool operator == (const CIDSet &Src) const {return this->ids == Src.ids;}
	bool operator != (const CIDSet &Src) const {return this->ids != Src.ids;}
	CIDSet& operator = (const CIDSet &Src) {
		clear();
		return operator+=(Src);
	}
	CIDSet& operator += (const UINT dwID) {
		this->ids.insert(dwID);
		return *this;
	}
	CIDSet& operator += (const CIDSet &Src) {
		for (const_iterator iter = Src.begin(); iter != Src.end(); ++iter)
			this->ids.insert(*iter);
		return *this;
	}
	CIDSet& operator += (const CIDList &Src) {
		IDNODE *pNode = Src.Get(0);
		while (pNode)
		{
			this->ids.insert(pNode->dwID);
			pNode = pNode->pNext;
		}
		return *this;
	}
	CIDSet& operator -= (const UINT dwID) {
		this->ids.erase(dwID);
		return *this;
	}
	CIDSet& operator -= (const CIDSet &Src) {
		for (const_iterator iter = Src.begin(); iter != Src.end(); ++iter)
			this->ids.erase(*iter);
		return *this;
	}

	//Removes IDs from this set that aren't members of Filter.
	void intersect(const CIDSet &Filter)
	{
		std::set<UINT> filteredIDs;
		for (const_iterator iter = Filter.begin(); iter != Filter.end(); ++iter)
			if (has(*iter))
				filteredIDs.insert(*iter);
		this->ids = filteredIDs;
	}

	//Does this set contain all IDs that a second set has.
	bool contains(const CIDSet &against) const
	{
		for (const_iterator iter = against.begin(); iter != against.end(); ++iter)
			if (!has(*iter))
				return false;
		return true;
	}
	//Does this set contain any IDs that a second set has.
	bool containsAny(const CIDSet &against) const
	{
		for (const_iterator iter = against.begin(); iter != against.end(); ++iter)
			if (has(*iter))
				return true;
		return false;
	}

	inline iterator begin() {return this->ids.begin();}
	inline const_iterator begin() const {return this->ids.begin();}
	inline iterator end() {return this->ids.end();}
	inline const_iterator end() const {return this->ids.end();}
	inline const_reverse_iterator rbegin() const {return this->ids.rbegin();}
	inline const_reverse_iterator rend() const {return this->ids.rend();}

	inline UINT getFirst() const {return size() ? *begin() : 0;}
	inline UINT getLast() const {return size() ? *rbegin() : 0;}
	inline UINT getMax() const {return size() ? *(this->ids.rbegin()) : 0;}

	// Split the given set into a number of sets that are no larger than the given amount
	static std::vector<CIDSet> divide(CIDSet& set, size_t amount) {
		if (set.size() <= amount) {
			return { set };
		}

		std::vector<CIDSet> result;
		result.push_back(CIDSet());
		size_t n = 0;
		size_t count = 0;
		for (IDSet::const_iterator it = set.begin(); it != set.end(); ++it) {
			result[n] += *it;
			++count;
			if (count >= amount) {
				count = 0;
				++n;
				result.push_back(CIDSet());
			}
		}

		return result;
	}

private:
	std::set<UINT> ids;
};

#endif //...#ifndef IDSET_H
