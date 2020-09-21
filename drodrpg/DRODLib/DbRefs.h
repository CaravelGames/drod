// $Id: DbRefs.h 8102 2007-08-15 14:55:40Z trick $

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

//DBRefs.h

//Used during DB export.
//Marks which DB records (or abbreviated references) have been exported.

#ifndef DBREFS_H
#define DBREFS_H

#include "DbBase.h"
#include "DbProps.h"

#include <BackEndLib/IDSet.h>

class CDbRefs : public CDbBase
{
public:
	CDbRefs(const VIEWTYPE vType, const CIDSet& ids, const UINT eSaveType=0);

	bool IsSet(const VIEWTYPE vType, const UINT dwID) const;

	void Set(const VIEWTYPE vType, const UINT dwID);

	virtual bool   Update() {return false;}

	VIEWTYPE vTypeBeingExported;
	CIDSet exportingIDs;
	
	//Specialized exports.
	UINT eSaveType;	//for player export -- only certain type of record

private:
	CIDSet Data;        //never a ref
	CIDSet Demos;       //never a ref
	CIDSet Holds;       //maybe ref
	CIDSet Levels;      //maybe ref
	CIDSet Players;     //maybe ref
	CIDSet Rooms;       //maybe ref
	CIDSet SavedGames;  //maybe ref
	CIDSet Speech;      //never a ref
};

#endif //...#ifndef DBREFS_H

