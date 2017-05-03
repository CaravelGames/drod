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

//DbRefs.cpp
//Implementation of CDbRefs.

#include "DbRefs.h"
#include <BackEndLib/Assert.h>

//
//CDbRefs public methods.
//

//*****************************************************************************
CDbRefs::CDbRefs(const VIEWTYPE vType, const CIDSet& ids, const UINT eSaveType)
	: vTypeBeingExported(vType)
	, exportingIDs(ids)
	, eSaveType(eSaveType)
{
}

//*****************************************************************************
bool CDbRefs::IsSet(
//Returns: whether view row key has been marked
	const VIEWTYPE vType,   //(in) DB view
	const UINT dwID)    //(in) view row key
const
{  
	switch (vType)
	{
		case V_Data:
			return this->Data.has(dwID);
		case V_Demos:
			return this->Demos.has(dwID);
		case V_Holds:
			return this->Holds.has(dwID);
		case V_Levels:
			return this->Levels.has(dwID);
		case V_Players:
			return this->Players.has(dwID);
		case V_Rooms:
			return this->Rooms.has(dwID);
		case V_SavedGames:
			return this->SavedGames.has(dwID);
		case V_Speech:
			return this->Speech.has(dwID);
		default:
			ASSERT(!"CDbRefs::IsSet() Unexpected view type.");
			return false;
	}
}

//*****************************************************************************
void CDbRefs::Set(
//Pre-cond: index is valid for view
//
//Set index when record at that row is exported/imported
//(or has a reference imported/exported).
	const VIEWTYPE vType,   //(in) DB view
	const UINT dwID)    //(in) view row index
{  
	switch (vType)
	{
		case V_Data:
			this->Data += dwID;
			break;
		case V_Demos:
			this->Demos += dwID;
			break;
		case V_Holds:
			this->Holds += dwID;
			break;
		case V_Levels:
			this->Levels += dwID;
			break;
		case V_Players:
			this->Players += dwID;
			break;
		case V_Rooms:
			this->Rooms += dwID;
			break;
		case V_SavedGames:
			this->SavedGames += dwID;
			break;
		case V_Speech:
			this->Speech += dwID;
			break;
		default:
			ASSERT(!"CDbRefs::Set() Unexpected view type.");
			break;
	}
}
