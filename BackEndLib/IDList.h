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

//IDList.h
//Declarations for CIDList.
//Class for managing a list of ID values.

#ifndef IDLIST_H
#define IDLIST_H

#include "Types.h"
#include "AttachableObject.h"

//Used to maintain a set of IDs with generic attached data.
//If attached data is not required, only a set of IDs, then use the
//simpler and more efficient CIDSet instead.

struct IDNODE
{
	UINT dwID;
	CAttachableObject *pvPrivate;
	bool bIsAttached;
	IDNODE *pNext;
};

class CIDList : public CAttachableObject
{
public:
	CIDList();
	virtual ~CIDList() {Clear();}

	CIDList(const CIDList &Src);
	CIDList &operator= (const CIDList &Src);
	void operator += (const CIDList &Src);
	void operator -= (const CIDList &Src);
	
	void  Add(const UINT dwID, CAttachableObject *pvPrivate = NULL, const bool bIsAttached=false);
	bool  AreIDsInList(const UINT wIDArrayCount, const UINT *pdwIDArray) const;
	void  Clear();
	IDNODE *Get(const UINT dwIndex) const;
	IDNODE *GetByID(const UINT dwID) const;
	UINT GetID(const UINT dwIndex) const;
	UINT GetSize() const {return this->dwSize;}
	bool  IsIDInList(const UINT dwID) const;
	void  Remove(const UINT dwID);

protected:
	void  RemoveNode(IDNODE* pSeek, IDNODE* pBeforeSeek);

	IDNODE *pFirstID;
	IDNODE *pLastID;
	UINT dwSize;
};

#endif //...#ifndef IDLIST_H
