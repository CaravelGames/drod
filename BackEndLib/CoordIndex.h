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
 * 1997, 2000, 2001, 2002 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef COORDINDEX_H
#define COORDINDEX_H

//CoordIndex.h
//Declarations for CCoordIndex.
//Class for indexing X,Y coordinates.  Should be used for quick membership
//checks.  Be careful about using to track coords over a large area because
//the index can take a lot of memory.  For a 38 x 32 room, 1216+ bytes 
//will be used.
//
//Additionally, special values can be stored at X,Y coordinates.

#include "Types.h"
#include "AttachableObject.h"
#include "Assert.h"
#include <cstring>

class CCoordSet;

template <typename T>
class CCoordIndex_T : public CAttachableObject
{
public:
	CCoordIndex_T(const UINT wCols=0, const UINT wRows=0, const T val=0);
	CCoordIndex_T(const CCoordIndex_T<T>& coordIndex);
	virtual ~CCoordIndex_T();

	CCoordIndex_T &operator= (const CCoordIndex_T<T>& Src);

	void        Add(const UINT wX, const UINT wY, const T val=1);
	void        AddAtIndex(const UINT index, const T val=1);
	void        Clear(const T val=0);
	inline bool  empty() const {return this->dwCoordCount == 0;}
	inline bool  Exists(const UINT wX, const UINT wY) const;
	inline const T* GetIndex() const {return this->pbytIndex;}
	inline UINT  GetArea() const {return this->wCols * this->wRows;}
	inline UINT  GetCols() const {return this->wCols;}
	void         GetCoordsWithValue(const T val, CCoordSet *pCoords) const;
	inline UINT  GetRows() const {return this->wRows;}
	inline UINT  GetSize() const {return this->dwCoordCount;}
	inline T     GetAt(const UINT wX, const UINT wY) const;
	inline T&    GetAt(const UINT wX, const UINT wY);
	bool        Init(const UINT wSetCols, const UINT wSetRows, const T val=0);
	void        Remove(const UINT wX, const UINT wY);
	void        RemoveAll(const T val);
	void        Replace(const T oldVal, const T newVal);
	void        Set(const UINT wX, const UINT wY, const T val);
	void        SetAtIndex(const UINT index, const T val);

private:
	UINT dwCoordCount;
	UINT  wCols, wRows;
	T *   pbytIndex;
};

typedef CCoordIndex_T<BYTE> CCoordIndex;
typedef CCoordIndex_T<UINT> CCoordIndexUint;

#include "CoordSet.h"

//
//CCoordIndex public template methods.
//

//*******************************************************************************
template<typename T>
CCoordIndex_T<T>::CCoordIndex_T(
//Constructor.
//
//Params:
	const UINT wCols, const UINT wRows, const T val) //[default = (0,0,0)]
	: CAttachableObject()
	, dwCoordCount(0L)
	, wCols(0), wRows(0)
	, pbytIndex(NULL)
{
	if (wCols && wRows)
		VERIFY(Init(wCols, wRows, val));
}

//*******************************************************************************
template<typename T>
CCoordIndex_T<T>::CCoordIndex_T(const CCoordIndex_T<T>& coordIndex)
	: CAttachableObject()
	, dwCoordCount(0L)
	, wCols(0), wRows(0)
	, pbytIndex(NULL)
{
	if (coordIndex.wCols && coordIndex.wRows)
	{
		if (Init(coordIndex.wCols, coordIndex.wRows))
		{
			//works only for primitive types
			memcpy(this->pbytIndex, coordIndex.pbytIndex,
					coordIndex.wCols * coordIndex.wRows * sizeof(T));
			this->dwCoordCount = coordIndex.dwCoordCount;
		}
	}
}

//*******************************************************************************
template<typename T>
CCoordIndex_T<T>::~CCoordIndex_T()
//Destructor.
{
	delete[] this->pbytIndex;
}

//*******************************************************************************
template<typename T>
CCoordIndex_T<T>& CCoordIndex_T<T>::operator=(const CCoordIndex_T<T>& rhs)
{
	if (!rhs.wCols || !rhs.wRows)
	{
		delete[] this->pbytIndex;
		this->pbytIndex = NULL;
		this->wCols = rhs.wCols;
		this->wRows = rhs.wRows;
		this->dwCoordCount = 0;
	} else if (Init(rhs.wCols, rhs.wRows)) {
		//works only for primitive types
		memcpy(this->pbytIndex, rhs.pbytIndex, rhs.GetArea() * sizeof(T));
		this->dwCoordCount = rhs.dwCoordCount;
	}
	return *this;
}

//*******************************************************************************
template<typename T>
bool CCoordIndex_T<T>::Init(
//Must be called before other methods to alloc the index.
//
//Params:
	const UINT wSetCols, //(in) Defines size of internal index and range of
	const UINT wSetRows, //(in) coords that other methods may accept.
	const T val)	//(in) initial value of cells [default=0]
//
//Returns:
//True if index was allocated, false if not.
{
	ASSERT(wSetCols > 0);
	ASSERT(wSetRows > 0);

	//500/1000 = unreasonably large and probably an error.
	ASSERT(wSetCols < 1000);
	ASSERT(wSetRows < 500);

	//Check whether object is already inited to these dimensions.
	if (this->wCols == wSetCols && this->wRows == wSetRows)
	{
		Clear(val);
		return true;
	}

	delete[] this->pbytIndex;

	//Create new index storage.
	this->pbytIndex = new T[wSetCols * wSetRows];
	if (this->pbytIndex)
	{
		this->wCols = wSetCols;
		this->wRows = wSetRows;
		this->dwCoordCount = 1; //force Clear to reset memory buffer
		Clear(val);
		return true;
	}

	this->dwCoordCount = 0;

	//Make assertions fire in other methods that have been incorrectly called.
	this->wCols = this->wRows = 0;

	return false;
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::Add(
//Adds the coord (x,y) to the index with the specified value.
//
//Params:
	const UINT wX, const UINT wY,
	const T val)               //(in) default=1
{
	ASSERT(wX < this->wCols && wY < this->wRows);
	AddAtIndex(wY * this->wCols + wX, val);
}

template<typename T>
void CCoordIndex_T<T>::AddAtIndex(
	const UINT index, const T val) //[default=1]
{
	ASSERT(val != 0); //zero is considered the reset value
	ASSERT(this->pbytIndex);
	if (this->pbytIndex[index] == 0) {
		++this->dwCoordCount;
		ASSERT(this->dwCoordCount <= GetArea());
	}
	this->pbytIndex[index] = val;
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::Clear(const T val)	//[default=0]
//Removes all coords from the index.
{
	if (this->pbytIndex) {
		if (val || !empty()) //if already empty, don't need to reset memory to 0 again
			memset(this->pbytIndex, val, GetArea() * sizeof(T));
	}
	this->dwCoordCount = val ? GetArea() : 0;
}

//*******************************************************************************
template<typename T>
bool CCoordIndex_T<T>::Exists(
//Are coords currently in the index?
//
//Params:
	const UINT wX, const UINT wY)
//
//Returns:
//True if they are, false if not.
const
{
	return GetAt(wX,wY) != 0;
}

//*******************************************************************************
template<typename T>
T CCoordIndex_T<T>::GetAt(
//
//
//Params:
	const UINT wX, const UINT wY)
//
//Returns:
//Value stored at (wX,wY)
const
{
	ASSERT(wX < this->wCols && wY < this->wRows);
	ASSERT(this->pbytIndex);
	return this->pbytIndex[wY * this->wCols + wX];
}

//*******************************************************************************
template<typename T>
T& CCoordIndex_T<T>::GetAt(
//
//
//Params:
	const UINT wX, const UINT wY)
//
//Returns:
//Value stored at (wX,wY)
{
	ASSERT(wX < this->wCols && wY < this->wRows);
	ASSERT(this->pbytIndex);
	return this->pbytIndex[wY * this->wCols + wX];
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::GetCoordsWithValue(const T val, CCoordSet* pCoords) const
//Appends coords that have this value to 'coords'.
{
	ASSERT(pCoords);
	const UINT area = GetArea();
	for (UINT dwSquareI = 0; dwSquareI < area; ++dwSquareI)
	{
		if (this->pbytIndex[dwSquareI] == val)
			pCoords->insert(dwSquareI % this->wCols, dwSquareI / this->wCols);
	}
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::Remove(
//Removes a coordinate from the index.
//
//Params:
	const UINT wX, const UINT wY)
{
	ASSERT(wX < this->wCols && wY < this->wRows);
	const UINT dwSquareI = wY * this->wCols + wX;
	ASSERT(this->pbytIndex);
	if (this->pbytIndex[dwSquareI])
	{
		--this->dwCoordCount;
		ASSERT(this->dwCoordCount < GetArea());
		this->pbytIndex[dwSquareI] = 0;
	}
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::RemoveAll(
//Removes the specified value from all positions in the index (reset positions to zero).
//
//Params:
	const T val)
{
	ASSERT(val != 0); //wasteful when called with zero

	const UINT area = GetArea();
	ASSERT(this->dwCoordCount <= area);
	for (UINT dwSquareI = 0; dwSquareI<area; ++dwSquareI)
	{
		if (this->pbytIndex[dwSquareI] == val)
		{
			this->pbytIndex[dwSquareI] = 0;
			--this->dwCoordCount;
		}
	}
	ASSERT(this->dwCoordCount <= area);
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::Replace(const T oldVal, const T newVal)
//Removes all locations marked with the old value and sets them to the new value.
{
	ASSERT(oldVal != newVal); //wasteful when called with same value

	const int inc = oldVal == 0 ? 1 : newVal == 0 ? -1 : 0;

	const UINT area = GetArea();
	for (UINT dwSquareI = 0; dwSquareI < area; ++dwSquareI)
	{
		if (this->pbytIndex[dwSquareI] == oldVal)
		{
			this->pbytIndex[dwSquareI] = newVal;
			this->dwCoordCount += inc;
		}
	}
}

//*******************************************************************************
template<typename T>
void CCoordIndex_T<T>::Set(
//Sets the value at (x,y).
//
//Params:
	const UINT wX, const UINT wY,
	const T val)
{
	ASSERT(wX < this->wCols && wY < this->wRows);
	SetAtIndex(wY * this->wCols + wX, val);
}

template<typename T>
void CCoordIndex_T<T>::SetAtIndex(
	const UINT index,
	const T val)
{
	ASSERT(this->pbytIndex);
	if (this->pbytIndex[index])
		--this->dwCoordCount;
	this->pbytIndex[index] = val;
	if (val)
	{
		++this->dwCoordCount;
		ASSERT(this->dwCoordCount <= GetArea());
	}
}

#endif //...#ifndef COORDINDEX_H
