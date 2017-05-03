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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//IDList.cpp
//Implementation of CIDList.

#include "Types.h"
#include "Assert.h"
#include "IDList.h"

//************************************************************************************
CIDList::CIDList()
	: CAttachableObject()
	, pFirstID(NULL), pLastID(NULL)
	, dwSize(0)
{}

//************************************************************************************
CIDList::CIDList(const CIDList &Src)
	: CAttachableObject()
	, pFirstID(NULL), pLastID(NULL)
	, dwSize(0)
//Copy constructor.  
//
//Note: Any attached private data will be unattached in copy.  In other words, the 
//responsibility for deleting attached private data will not transfer from source 
//CIDList to this CIDList.  Caller should probably not reference private data in this 
//CIDList if source CIDList goes out of scope.
{
	IDNODE *pSeek = Src.pFirstID;
	while (pSeek != NULL)
	{
		Add(pSeek->dwID, pSeek->pvPrivate);
		pSeek = pSeek->pNext;
	}
}

//************************************************************************************
CIDList &CIDList::operator= (const CIDList &Src)
//Assignment operator.  Will clear this list and copy from source list.
//
//Note: Any attached private data will be unattached in copy.  In other words, the 
//responsibility for deleting attached private data will not transfer from source 
//CIDList to this CIDList.  Caller should probably not reference private data in this 
//CIDList if source CIDList goes out of scope.
{
	Clear();

	IDNODE *pSeek = Src.pFirstID;
	while (pSeek != NULL)
	{
		Add(pSeek->dwID, pSeek->pvPrivate);
		pSeek = pSeek->pNext;
	}
	return *this;
}

//************************************************************************************
void CIDList::operator += (const CIDList &Src)
//Append source list to this one.
//
//Note: Any attached private data will be unattached in copy.  In other words, the 
//responsibility for deleting attached private data will not transfer from source 
//CIDList to this CIDList.  Caller should probably not reference private data in this 
//CIDList if source CIDList goes out of scope.
{
	IDNODE *pSeek = Src.pFirstID;
	while (pSeek != NULL)
	{
		Add(pSeek->dwID, pSeek->pvPrivate);
		pSeek = pSeek->pNext;
	}
}

//************************************************************************************
void CIDList::operator -= (const CIDList &Src)
//Remove any IDs in this list that match IDs in source list.
{
	IDNODE *pSeek = this->pFirstID, *pDelete, *pPrev = NULL;
	while (pSeek != NULL)
	{
		if (Src.IsIDInList(pSeek->dwID)) 
		{
			//Remove this node.
			pDelete = pSeek;
			pSeek = pSeek->pNext;
			delete pDelete;

			if (pPrev) 
				pPrev->pNext = pSeek;
			else
				this->pFirstID = pSeek;
			--this->dwSize;
		}
		else  
		{
			//Keep this node.
			pPrev = pSeek;
			pSeek = pSeek->pNext;
		}
	}
}

//************************************************************************************
void CIDList::Add(
//Add an ID to the list.
//
//Params:
	const UINT dwID,    //(in)   ID to add.
	CAttachableObject *pvPrivate,//(in) Private data to associate with ID.  If NULL
							//    (default) then no private data will be associated.
							//    Never pass an array allocated with "new []"--encapsulate
							//    in a class or struct before passing.
	const bool bIsAttached) //(in)   If set to true then the private data will be deleted
							//    by CIDList when it destructs.  If false (default) 
							//    then data should get deleted elsewhere, but not by
							//    caller.
{
	if (IsIDInList(dwID))
		return; //Don't add a duplicate.

	//Create new ID node.
	IDNODE *pNew = new IDNODE;
	pNew->dwID = dwID;
	pNew->pvPrivate = pvPrivate;
	pNew->bIsAttached = bIsAttached;
	pNew->pNext = NULL;

	//Add new ID node to list.
	if (this->pLastID)
	{
		this->pLastID->pNext = pNew;
		this->pLastID = pNew;
	}
	else
	{
		this->pFirstID = this->pLastID = pNew;
	}
	++this->dwSize;
}

//************************************************************************************
void CIDList::Remove(
//Removes an ID from list, deleting associated private data if attached.
//
//Params:
	const UINT dwID)
{
	//Find the node.
	IDNODE *pSeek = this->pFirstID, *pBeforeSeek = NULL;
	while (pSeek)
	{
		if (pSeek->dwID == dwID) break;
		pBeforeSeek = pSeek;
		pSeek = pSeek->pNext;
	}
	if (!pSeek) {ASSERT(!"ID to remove not found."); return;} //No match.

	//Remove the node from the list.
	RemoveNode(pSeek, pBeforeSeek);
}

//************************************************************************************
void CIDList::RemoveNode(
//Removes the specified node from list, deleting associated private data if attached.
//
//Params:
	IDNODE* pSeek, IDNODE* pBeforeSeek)
{
	//Remove the node from the list.
	if (pBeforeSeek) 
		pBeforeSeek->pNext = pSeek->pNext;
	else
	{
		ASSERT(this->pFirstID == pSeek);
		this->pFirstID = pSeek->pNext;
	}
	if (pSeek == this->pLastID)
		this->pLastID = pBeforeSeek;
	--this->dwSize;

	//Delete the node and maybe its private data.
	if (pSeek->bIsAttached)
		delete pSeek->pvPrivate;
	delete pSeek;
}

//************************************************************************************
void CIDList::Clear()
//Frees resources associated with object and zeroes members.
{
	IDNODE *pDelete, *pSeek=this->pFirstID;

	while (pSeek)
	{
		pDelete = pSeek;
		pSeek = pSeek->pNext;
		if (pDelete->bIsAttached)
			delete pDelete->pvPrivate;
		delete pDelete;
	}
	this->pFirstID = this->pLastID = NULL;

	this->dwSize = 0L;
}

//************************************************************************************
bool CIDList::IsIDInList(UINT dwID) const
//Checks for an ID in the list.
{
	IDNODE *pSeek = this->pFirstID;

	while (pSeek)
	{
		if (pSeek->dwID == dwID) return true; //Found it.
		pSeek = pSeek->pNext;
	}
	return false; //Didn't find it.
}

//************************************************************************************
bool CIDList::AreIDsInList(
//Checks for an occurrence of more than one ID in a list.
//
//Params:
	UINT wIDArrayCount,        //# of elements in array.
	const UINT *pdwIDArray)   //IDs to check for.
//
//Returns:
//True if any of the IDs were found, false if not.
const
{
	IDNODE *pSeek = this->pFirstID;

	UINT wCompareI = 0;
	while (pSeek)
	{
		for (wCompareI = 0; wCompareI < wIDArrayCount; ++wCompareI)
		{
			if (pSeek->dwID == pdwIDArray[wCompareI]) return true; //Found it.
		}
		pSeek = pSeek->pNext;
	}
	return false;
}

//************************************************************************************
UINT CIDList::GetID(UINT dwIndex) const
//Gets an ID from list by index.
{
	IDNODE *pSeek = this->pFirstID;

	ASSERT(dwIndex < dwSize);

	UINT dwCurrentIndex = 0;
	while (pSeek)
	{
		if (dwCurrentIndex == dwIndex) return pSeek->dwID; //Found it.
		pSeek = pSeek->pNext;
		++dwCurrentIndex;
	}
	return 0; //Didn't find it.
}

//************************************************************************************
IDNODE *CIDList::Get(UINT dwIndex) const
//Gets an ID node from list by index.
{
	IDNODE *pSeek = this->pFirstID;

	if (dwIndex >= dwSize) return NULL;

	UINT dwCurrentIndex = 0;
	while (pSeek)
	{
		if (dwCurrentIndex == dwIndex) return pSeek; //Found it.
		pSeek = pSeek->pNext;
		++dwCurrentIndex;
	}
	return NULL; //Didn't find it.
}

//************************************************************************************
IDNODE *CIDList::GetByID(UINT dwID) const
//Gets an ID node from list by its ID.
{
	IDNODE *pSeek = this->pFirstID;
	while (pSeek)
	{
		if (dwID == pSeek->dwID) return pSeek; //Found it.
		pSeek = pSeek->pNext;
	}
	return NULL; //Didn't find it.
}
