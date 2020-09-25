// $Id: TarMother.cpp 10108 2012-04-22 04:54:24Z mrimer $

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
 *
 * ***** END LICENSE BLOCK ***** */

//TarMother.cpp
//Implementation of CTarMother.

#include "TarMother.h"

//
//Public methods.
//

//*****************************************************************************
void CMother::Process(
//Process a mother for movement.
//
//Params:
	CCueEvents& /*CueEvents*/,  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
	const CUEEVENT_ID /*eGrowthEvent*/)	//(in)
{
/*
	const CDbRoom& room = *(this->pCurrentGame->pRoom);

	if (!this->bEyeSet)
	{
		//Determine whether this is a left or right eye.
		this->bLeftEye = (this->wO == NO_ORIENTATION || this->wO == W);
		this->bEyeSet = true;

		static const UINT DIST = 1;
		this->wMinX = this->wX - (this->bLeftEye ? DIST : DIST+1);
		this->wMaxX = this->wX + (this->bLeftEye ? DIST+1 : DIST);
		this->wMinY = this->wY - DIST;
		this->wMaxY = this->wY + DIST;
		if (this->wMinX >= room.wRoomCols) this->wMinX = 0;
		if (this->wMaxX >= room.wRoomCols) this->wMaxX = room.wRoomCols-1;
		if (this->wMinY >= room.wRoomRows) this->wMinY = 0;
		if (this->wMaxY >= room.wRoomRows) this->wMaxY = room.wRoomRows-1;
	}

	//Reverse eyes if either eye in pair is scared (a sword or other danger is close).
	const bool bScared = room.IsSwordWithinRect(wMinX,wMinY,wMaxX,wMaxY) ||
			room.IsMonsterInRectOfType(wMinX,wMinY,wMaxX,wMaxY,M_SERPENTG) ||
			room.IsTileInRectOfType(wMinX,wMinY,wMaxX,wMaxY,T_BRIAR_LIVE) ||
			room.IsTileInRectOfType(wMinX,wMinY,wMaxX,wMaxY,T_BRIAR_DEAD);

	if (pCurrentGame->wSpawnCycleCount % TURNS_PER_CYCLE == TURNS_PER_CYCLE-1)
		this->wO = (this->bLeftEye != bScared ? W : SW);   //closed eye
	else
		this->wO = (this->bLeftEye != bScared ? NO_ORIENTATION : S);   //open eye

	//Grow tarstuff if swordsman/decoy is sensed.
	if ((this->pCurrentGame->wSpawnCycleCount % TURNS_PER_CYCLE == 0) &&
			(this->pCurrentGame->bBrainSensesSwordsman || CanFindSwordsman() ||
			room.BrainSensesTarget()))
		CueEvents.Add(eGrowthEvent, this);
*/
}

//*****************************************************************************
void CTarMother::Process(
//Process a tar mother for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	CMother::Process(CueEvents, CID_TarGrew);
}

//*****************************************************************************
void CMudMother::Process(
//Process a mud mother for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	CMother::Process(CueEvents, CID_MudGrew);
}

//*****************************************************************************
void CGelMother::Process(
//Process a gel mother for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	CMother::Process(CueEvents, CID_GelGrew);
}

//*****************************************************************************
void CMother::ReflectX(CDbRoom *pRoom)
{
	CMonster::ReflectX(pRoom);

	//Fix orientation switch performed in ReflectX().
	this->wO = this->wO == NO_ORIENTATION ? S : NO_ORIENTATION;
}

//*****************************************************************************
void CMother::ReflectY(CDbRoom *pRoom)
{
	CMonster::ReflectY(pRoom);

	//Fix orientation switch performed in ReflectY().
	if (this->wO != NO_ORIENTATION) this->wO = S;
}

//*****************************************************************************
void CMother::RotateClockwise(CDbRoom *pRoom)
{
	CMonster::RotateClockwise(pRoom);

	//Fix orientation rotation performed in RotateClockwise().
	if (this->wO != NO_ORIENTATION) this->wO = S;
}
