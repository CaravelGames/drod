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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "CueEvents.h"
#include "MonsterMessage.h"
#include <BackEndLib/AttachableObject.h>

using namespace std;

//
//Definitions
//

//Has player left room or will he shortly be leaving/restarting?  Another way
//of looking at: are any more commands going to be processed in the current room.
const CUEEVENT_ID CIDA_PlayerLeftRoom[15] = {
	CID_ExitRoomPending, CID_ExitRoom, CID_ExitLevelPending, CID_WinGame,
	CID_MonsterKilledPlayer, CID_ExplosionKilledPlayer, CID_BriarKilledPlayer,
	CID_HalphDied, CID_PlayerBurned, CID_NPCBeethroDied, CID_CriticalNPCDied,
	CID_PlayerImpaled, CID_PlayerFellIntoPit, CID_PlayerDrownedInWater,
	CID_PlayerEatenByOremites
};

//Did something kill the player?  This array should be checked for death instead
//of these separate events so that future causes of death will be checked without
//project-wide changes to code.
const CUEEVENT_ID CIDA_PlayerDied[11] = {
	CID_MonsterKilledPlayer, CID_ExplosionKilledPlayer, CID_BriarKilledPlayer,
	CID_HalphDied, CID_PlayerBurned, CID_NPCBeethroDied, CID_CriticalNPCDied,
	CID_PlayerImpaled, CID_PlayerFellIntoPit, CID_PlayerDrownedInWater,
	CID_PlayerEatenByOremites
};

//Did a monster die?
const CUEEVENT_ID CIDA_MonsterDied[4] = {
	CID_SnakeDiedFromTruncation, CID_MonsterDiedFromStab,
	CID_MonsterExitsRoom, CID_MonsterBurned};

//Was a monster stabbed?
const CUEEVENT_ID CIDA_MonsterStabbed[3] = {
	CID_MonsterDiedFromStab, CID_MonsterPieceStabbed, CID_TarstuffDestroyed
};

//
//Public methods.
//

//***************************************************************************************
void CCueEvents::Clear()
//Frees resources and resets members.
{
	//Delete private data and nodes to track it.
	for (UINT wCID=CUEEVENT_COUNT; wCID--; )
	{
		vector<CID_PRIVDATA_NODE>::iterator pSeek = this->CIDPrivateData[wCID].begin();
		while (pSeek != this->CIDPrivateData[wCID].end())
		{
			ASSERT(pSeek->pvPrivateData);
			if (pSeek->bIsAttached)
				delete pSeek->pvPrivateData;
			++pSeek;
		}
		//this->CIDPrivateData[wCID].clear();	//cleared in Zero below
	}

	//Zero all the members.
	Zero();
}

//***************************************************************************************
void CCueEvents::ClearEvent(const CUEEVENT_ID eCID, const bool bDeleteAttached)	//[default=true]
//Resets members for specified cue event.
{
	if (bDeleteAttached)
	{
		//Delete attached data objects.
		vector<CID_PRIVDATA_NODE>::iterator pSeek = this->CIDPrivateData[eCID].begin();
		while (pSeek != this->CIDPrivateData[eCID].end())
		{
			ASSERT(pSeek->pvPrivateData);
			if (pSeek->bIsAttached)
				delete pSeek->pvPrivateData;
			++pSeek;
		}
	}
	this->CIDPrivateData[eCID].clear();

	//Reset any iteration through this event's members.
	if (this->wNextCID == (UINT)eCID)
	{
		this->wNextPrivateDataIndex = 0;
		this->wNextCID = (UINT)-1;
	}

	//Reset flags for this event.
	if (this->barrIsCIDSet[eCID])
	{
		this->barrIsCIDSet[eCID] = false;
		--this->wEventCount;
	}
}

//***************************************************************************************
UINT CCueEvents::GetOccurrenceCount(
//Get number of times a specified cue event has occurred.
//
//Params:
	const CUEEVENT_ID eCID) //(in)   Cue event for which to count occurrences.
//
//Returns:
//The count.  Will be 0 if cue event has not occurred.
const
{
	return this->CIDPrivateData[eCID].size();
}

//***************************************************************************************
void CCueEvents::Add(
//Sets a cue event to true and associates a private data pointer with that cue event.
//
//Params:
	const CUEEVENT_ID eCID, //(in)   Cue event ID that will be set to true.
	const CAttachableObject *pvPrivateData,   //(in)   Private data to associate with cue event.  If NULL
							//    (default) then no private data will be associated.
							//    Never pass an array allocated with "new []"--encapsulate
							//    in a class or struct before passing.
	const bool bIsAttached)    //(in)   If set to true then the private data will be deleted
							//    by CCueEvents when it destructs.  If false (default)
							//    then data should get deleted elsewhere, but not by
							//    caller.  See comments about private data validity
							//    guarantee in CueEvents.h.
{
	ASSERT(IS_VALID_CID(eCID));
	ASSERT(!(pvPrivateData == NULL && bIsAttached));

	//Set CID flag.
	if (!this->barrIsCIDSet[eCID])
	{
		this->barrIsCIDSet[eCID] = true;
		++this->wEventCount;
	}

	//Add private data.
	if (pvPrivateData)
		this->CIDPrivateData[eCID].push_back(
				CID_PRIVDATA_NODE(bIsAttached, pvPrivateData));
}

//***************************************************************************************
const CAttachableObject* CCueEvents::GetFirstPrivateData(
//Returns: first private data pointer associated with a cue event, and sets object state so
//that a call to GetNextPrivateData() will return the second private data, if available.
//
//Params:
	const CUEEVENT_ID eCID) //(in)   Which event for which to retrieve private data.
//
//Returns:
//First private data pointer or NULL if there is no private data associated with cue event.
{
	vector<CID_PRIVDATA_NODE>::const_iterator pFirst = this->CIDPrivateData[eCID].begin();
	if (pFirst != this->CIDPrivateData[eCID].end())
	{
		ASSERT(this->barrIsCIDSet[eCID]); //If I found private data then the event should be set.
		ASSERT(pFirst->pvPrivateData);

		this->wNextCID = eCID;
		this->wNextPrivateDataIndex = 1;
		return pFirst->pvPrivateData;
	}

	this->wNextPrivateDataIndex = 0;
	this->wNextCID = (UINT)-1;
	return NULL;
}

//***************************************************************************************
const CAttachableObject* CCueEvents::GetNextPrivateData()
//Gets next private data pointer after call made to GetFirstPrivateData().  May be called
//multiple times to retrieve a succession of private data pointers.
//
//Returns:
//Private data pointer or NULL if there are no more.
{
	//Check whether there are any more private data left.
	if (this->wNextCID == (UINT)-1) return NULL;
	if (this->wNextPrivateDataIndex >= this->CIDPrivateData[this->wNextCID].size())
		return NULL;

	CID_PRIVDATA_NODE *pCurrent = &(this->CIDPrivateData[this->wNextCID]
			[this->wNextPrivateDataIndex++]);
	ASSERT(pCurrent->pvPrivateData);
	return pCurrent->pvPrivateData;
}

//***************************************************************************************
bool CCueEvents::HasAnyOccurred(
//Checks to see if at least one cue event from a list has been set.
//
//Params:
	const UINT wCIDArrayCount,       //# of elements in array.
	const CUEEVENT_ID *peCIDArray)   //IDs to check for.
//
//Returns:
//True if any of the IDs were found, false if not.
const
{
	//Each iteration checks for presence of one CID from
	for (UINT wCIDI = 0; wCIDI < wCIDArrayCount; ++wCIDI)
	{
		ASSERT(IS_VALID_CID(peCIDArray[wCIDI]));
		if (this->barrIsCIDSet[peCIDArray[wCIDI]])
			//Found one from the list--any remaining IDs are ignored.
			return true;
	}

	//Didn't find any.
	return false;
}

//***************************************************************************************
bool CCueEvents::HasOccurredWith(
//Has an event occurred with a matching private data pointer?
//
//Params:
	const CUEEVENT_ID eCID,             //(in)   Check for this CID.
	const CAttachableObject *pvPrivateData)   //(in)   Check for this private data.
//
//Returns:
//True if a match was found, false if not.
const
{
	ASSERT(IS_VALID_CID(eCID));
	ASSERT(pvPrivateData);

	vector<CID_PRIVDATA_NODE>::const_iterator pSeek = this->CIDPrivateData[eCID].begin();
	const vector<CID_PRIVDATA_NODE>::const_iterator pSeekEnd = this->CIDPrivateData[eCID].end();

	//Each iteration checks on private data for a matching pointer.
	while (pSeek != pSeekEnd)
	{
		ASSERT(this->barrIsCIDSet[eCID]);
		if (pSeek->pvPrivateData == pvPrivateData)
			return true; //Found it.
		++pSeek;
	}

	//Didn't find it.
	return false;
}

//***************************************************************************************
bool CCueEvents::Remove(
//Remove event that occurred with a matching private data pointer
//
//Params:
	const CUEEVENT_ID eCID,                 //(in) Check for this CID.
	const CAttachableObject *pvPrivateData) //(in) Check for this private data.
//
//Returns:
//True if a match was found, false if not.
{
	ASSERT(IS_VALID_CID(eCID));
	ASSERT(pvPrivateData);

	vector<CID_PRIVDATA_NODE>::iterator pSeek = this->CIDPrivateData[eCID].begin();
	const vector<CID_PRIVDATA_NODE>::const_iterator pSeekEnd = this->CIDPrivateData[eCID].end();

	//Each iteration checks on private data for a matching pointer.
	while (pSeek != pSeekEnd)
	{
		ASSERT(this->barrIsCIDSet[eCID]);
		if (pSeek->pvPrivateData == pvPrivateData)
		{
			if (pSeek->bIsAttached)
				delete pSeek->pvPrivateData;
			this->CIDPrivateData[eCID].erase(pSeek);
			return true; //Found it.
		}
		++pSeek;
	}

	//Didn't find it.
	return false;
}

//
//Private methods.
//

//***************************************************************************************
void CCueEvents::Zero()
//Zero all the members.
{
	ASSERT(sizeof(this->barrIsCIDSet) ==
			sizeof(this->barrIsCIDSet[0]) * CUEEVENT_COUNT);
	ASSERT(sizeof(this->CIDPrivateData) ==
			sizeof(this->CIDPrivateData[0]) * CUEEVENT_COUNT);

	this->wNextPrivateDataIndex = 0;
	this->wNextCID = (UINT)-1;
	memset(this->barrIsCIDSet, 0, sizeof(this->barrIsCIDSet));
	for (UINT wCID=CUEEVENT_COUNT; wCID--; )
		this->CIDPrivateData[wCID].clear();
	this->wEventCount = 0;
}
