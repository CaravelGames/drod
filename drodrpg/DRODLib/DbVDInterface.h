// $Id: DbVDInterface.h 10108 2012-04-22 04:54:24Z mrimer $

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

#ifndef DBVDINTERFACE_H
#define DBVDINTERFACE_H

#ifdef WIN32 
#  pragma warning(disable:4786) 
#endif 

#include "DbBase.h"
#include "DbMessageText.h"
#include "DbRefs.h"
#include <BackEndLib/IDSet.h>
#include <BackEndLib/StretchyBuffer.h>

#ifdef WIN32 
#  pragma warning(disable:4786) 
#endif 

//******************************************************************************************
class CDb;

template<typename VDElement>
class CDbVDInterface : public CDbBase
{
protected:
	friend class CDb;
	CDbVDInterface(const VIEWTYPE vType, const c4_IntProp viewDefPrimaryKeyProp)
		: emptyEndRows(0)
		, bIsMembershipLoaded(false)
		, bQuickLoad(false)
		, vType(vType)
		, viewDefPrimaryKeyProp(viewDefPrimaryKeyProp)
	{
      currentRow = MembershipIDs.begin();
   }

public:
	~CDbVDInterface() {}

	virtual void      Delete(const UINT dwVDID);
	bool      Exists(const UINT dwID) const;

	//Serialize the text strings contained in this object, with accompanying GID references, as a string.
	virtual bool      ExportText(const UINT /*dwVDID*/, CDbRefs& /*dbRefs*/, CStretchyBuffer& /*str*/) {return false;}

	//Serialize the data object with specified ID as an XML-formatted string.
	virtual void      ExportXML(const UINT dwVDID, CDbRefs &dbRefs, string &str, const bool bRef=false)=0;

	static VDElement *  GetByID(const UINT dwVDID, const bool bQuick=false);
	void                GetIDs(CIDSet &IDs);
	CIDSet              GetIDs(); //alternate return method
	VDElement *         GetFirst(const bool bQuick=false);
	VDElement *         GetNext();
	virtual VDElement * GetNew();
	UINT               GetSize();

	void ResetMembership() {this->bIsMembershipLoaded = false;}

	virtual bool      Update() {return false;}

	//**************************************************************************
	//
	// The methods below should be called through the view members of
	// the global DB object.
	//

	//Calling this ensures this many empty rows are located at the end of the view.
	void EnsureEmptyRows(const UINT numRows)
	{
		if (numRows)
		{
			const int delta = numRows - this->emptyEndRows;
			if (delta > 0)
			{
				c4_View View = GetActiveView(this->vType);
				View.SetSize(View.GetSize() + delta);
				this->emptyEndRows += delta;
			}
			ASSERT(this->emptyEndRows >= numRows);
		}
	}

	//Returns: the number of non-empty rows in the view
	UINT GetViewSize(const UINT dwID=UINT(-1)) const
	{
		//By default, return the number of active rows in all views of this type.
		if (dwID == UINT(-1))
			return CDbBase::GetViewSize(this->vType) - this->emptyEndRows;

		//Otherwise, only consider the number of rows in the specific view
		//where the ID may be located.
		UINT size = GetView(vType, dwID).GetSize();
		//Which build determines which view (i.e., ID range) may have empty rows.
#ifdef DEV_BUILD
		if (dwID < START_LOCAL_ID)
			size -= this->emptyEndRows;
#else
		if (dwID >= START_LOCAL_ID)
			size -= this->emptyEndRows;
#endif
		return size;
	}

	//Either adds a row to the view and returns a reference to it,
	//or returns a reference to the first empty row at the end of the view.
	c4_RowRef GetNewRow()
	{
		c4_View View = GetActiveView(this->vType);
		const UINT size = View.GetSize();
		if (this->emptyEndRows)
			return View[size - this->emptyEndRows--];
		else {
			View.SetSize(size + 1);
			return View[size];
		}
	}

	//Removes any allocated empty rows from the end of the view.
	void RemoveEmptyRows()
	{
		if (this->emptyEndRows)
		{
			c4_View View = GetActiveView(this->vType);
			const UINT wCount = View.GetSize();
			View.SetSize(wCount - this->emptyEndRows);
			this->emptyEndRows = 0;
		}
	}

	//After a new row is added, sort it to the correct index so
	//primary keys remain monotonic (required for LookupRowByPrimaryKey to work).
	void SortLastRow()
	{
		c4_View View = GetActiveView(this->vType);
		const UINT size = View.GetSize() - this->emptyEndRows;
		if (size < 2)
			return; //nothing to sort

		c4_IntProp *pPropID = GetPrimaryKeyProp(this->vType);	//Reference to the primary key field.
		const int lastRowIndex = (int)size - 1;
		c4_Row lastRow = View[lastRowIndex]; //make temp copy of the row
		const UINT lastRowKey = UINT((*pPropID)(lastRow));

		//Swap rows until the last row's key index will be in order.
		ASSERT(lastRowIndex >= 1);
		int curIndex;
		for (curIndex = lastRowIndex - 1; curIndex >= 0; --curIndex)
		{
			c4_RowRef prevRow = View[curIndex];
			const UINT prevRowKey = UINT((*pPropID)(prevRow));
			if (prevRowKey <= lastRowKey)
				break; //done
			View.SetAt(curIndex + 1, prevRow); //push row back
		}

		//Reinsert last row if it was originally out of order.
		if (curIndex < lastRowIndex - 1) //were any swaps made?
			View.SetAt(curIndex + 1, lastRow); //yes -- swap last row to this spot
	}

	UINT        emptyEndRows; //empty view rows appended to the table for later use

protected:
	virtual void      LoadMembership();

	bool        bIsMembershipLoaded;
	CIDSet     MembershipIDs;
	CIDSet::const_iterator    currentRow;
	bool        bQuickLoad;

	VIEWTYPE    vType;
	c4_IntProp  viewDefPrimaryKeyProp;
};

//
// Implementation of template methods
//

//*****************************************************************************
template<typename VDElement>
void CDbVDInterface<VDElement>::Delete(
//Deletes element with this Primary Key ID.
//
//Params:
//NOTE: Unused param names commented out to suppress warning
	const UINT /*dwVDID*/) //(in)
{
}

//*****************************************************************************
template<typename VDElement>
bool CDbVDInterface<VDElement>::Exists(const UINT dwID) const
//Returns: whether row in DB exists
{
	c4_View view;
	return LookupRowByPrimaryKey(dwID, this->vType, view) != ROW_NO_MATCH;
}

//*****************************************************************************
template<typename VDElement>
VDElement* CDbVDInterface<VDElement>::GetByID(
//Get an element by its Primary Key ID.
//
//Params:
	const UINT dwVDID,  //(in)
	const bool bQuick)   //(in) load only certain data members [default=false]
//
//Returns:
//Pointer to loaded element which caller must delete, or NULL if no matching element 
//was found.
{
	VDElement *pVDElement = new VDElement();
	if (pVDElement)
	{
		if (!pVDElement->Load(dwVDID, bQuick))
		{
			delete pVDElement;
			pVDElement=NULL;
		}
	}
	return pVDElement;
}

//*****************************************************************************
template<typename VDElement>
void CDbVDInterface<VDElement>::GetIDs(
//Gets IDs in membership.
//
//Params:
	CIDSet &IDs)   //(out) Receives copy of object's membership list.
{
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(this->bIsMembershipLoaded);

	IDs = this->MembershipIDs;
}

//*****************************************************************************
template<typename VDElement>
CIDSet CDbVDInterface<VDElement>::GetIDs()
{
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(this->bIsMembershipLoaded);

	return this->MembershipIDs;
}

//*****************************************************************************
template<typename VDElement> UINT CDbVDInterface<VDElement>::GetSize()
//Returns count of elements in membership.
{
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(this->bIsMembershipLoaded);

	return this->MembershipIDs.size();
}

//*****************************************************************************
template<typename VDElement>
VDElement* CDbVDInterface<VDElement>::GetFirst(
//Gets first element.  A subsequent call to GetNext() will retrieve the second element.
//
//Params:
	const bool bQuick)   //(in) load only certain simple data members of element [default=false]
//
//Returns:
//Pointer to loaded element which caller must delete, or NULL if no matching element
//was found.
{
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(bIsMembershipLoaded);
	this->bQuickLoad = bQuick;

	//Set current row to first.
	this->currentRow = this->MembershipIDs.begin();

	//If there are no rows, then just return NULL.
	if (this->currentRow == this->MembershipIDs.end()) return NULL;

	//Load element.
	VDElement *pVDElement = GetByID(*this->currentRow, this->bQuickLoad);

	//Advance row to next.
	++this->currentRow;

	return pVDElement;
}

//*****************************************************************************
template<typename VDElement>
VDElement* CDbVDInterface<VDElement>::GetNext()
//Gets next element.
//
//Returns:
//Pointer to loaded element which caller must delete, or NULL if no matching element
//was found.
{
	if (!this->bIsMembershipLoaded) LoadMembership();
	ASSERT(bIsMembershipLoaded);

	//If at end of rows, just return NULL.
	if (this->currentRow == this->MembershipIDs.end()) return NULL;

	//Load element.
	VDElement *pVDElement = GetByID(*currentRow, this->bQuickLoad);

	//Advance row to next.
	++currentRow;

	return pVDElement;
}

//*****************************************************************************
template<typename VDElement>
VDElement* CDbVDInterface<VDElement>::GetNew()
//Get a new object that will be added to database when it is updated.
//
//Returns:
//Pointer to new element
{
	//After element object is updated, membership changes, so reset the flag.
	this->bIsMembershipLoaded = false;

	//Return new object.
	return new VDElement;   
}

//
//CDbVDInterface protected methods.
//

//*****************************************************************************
template<typename VDElement>
void CDbVDInterface<VDElement>::LoadMembership()
//Load the membership list with all viewdef IDs.
{
	ASSERT(CDbBase::IsOpen());
	const UINT dwCount = GetViewSize();
	this->MembershipIDs.clear();

	//Each iteration gets a new ID and puts in membership list.
	for (UINT dwViewDefI = 0; dwViewDefI < dwCount; ++dwViewDefI)
	{
		c4_RowRef row = GetRowRef(this->vType, dwViewDefI);
		this->MembershipIDs += this->viewDefPrimaryKeyProp(row);
	}
	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

#endif //...#ifndef DBVDINTERFACE_H

