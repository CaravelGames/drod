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
 *
 * ***** END LICENSE BLOCK ***** */

//EvilEye.cpp
//Implementation of CEvilEye.

#include "EvilEye.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CEvilEye::CEvilEye(CCurrentGame *pSetCurrentGame) : CMonster(M_EYE, pSetCurrentGame)
	, bIsActive(false)
	, bHalfAwake(false)
{
}

//*****************************************************************************************
void CEvilEye::Process(
//Process an Evil Eye for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	CMoveCoord *pCoord;
	int nOX, nOY;
	if (!WakeupCheck(CueEvents, pCoord, nOX, nOY))
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy))
		return;

	//Move evil eye to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);

	//Shoot beam out of eye at new position if it's still looking at player.
	if (pCoord && pCoord->wX+nOX == this->wX && pCoord->wY+nOY == this->wY)
	{
		pCoord->wX = this->wX;
		pCoord->wY = this->wY;
	}
}

//*****************************************************************************************
bool CEvilEye::GetNextGaze(CDbRoom *pRoom, UINT& cx, UINT& cy, int& dx, int& dy, bool& bReflected)
//Updates (cx,cy) based on how evil eye's gaze travels when facing (dx,dy).
//
//Returns: true if gaze continues, false if blocked
{
	ASSERT(dx || dy);
	cx += dx;
	cy += dy;
	if (cx >= pRoom->wRoomCols || cy >= pRoom->wRoomRows)
		return false;

	switch (pRoom->GetOSquare(cx, cy))
	{
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_TUNNEL_E: case T_TUNNEL_W: case T_TUNNEL_N: case T_TUNNEL_S:
		case T_PIT:	case T_PIT_IMAGE: case T_WATER: case T_SHALLOW_WATER: case T_STEP_STONE:
		case T_HOT: case T_GOO:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_PLATFORM_P: case T_PLATFORM_W:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE: case T_THINICE_SH: case T_PRESSPLATE:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
			break;  //these do not block gaze -- examine next layer
		default:
			return false;
	}

	//Nothing on f-layer blocks gaze.

	switch (pRoom->GetTSquare(cx,cy))
	{
		case T_ORB: case T_OBSTACLE:
		case T_BEACON: case T_BEACON_OFF:
		case T_POWDER_KEG:
			return false;  //these block gaze
		case T_MIRROR:
			//Look in opposite direction.
			if (bReflected)
				return false; //closed loop
			dx = -dx;
			dy = -dy;
			cx += dx;
			cy += dy;
			bReflected = true; //only reflect one time
			return true;
		default: break;
	}

	//Monsters do not block gaze.

	return true;
}

//*****************************************************************************************
bool CEvilEye::WakeupCheck(
//Determines whether eye wakes up this turn.
//
//Returns: true if eye is awake or wakes up
//
//Params:
	CCueEvents& CueEvents,
	CMoveCoord* &pCoord, int& nOX, int& nOY) //(out)
{
	pCoord = NULL;
	if (this->bIsActive)
		return true;

	const bool bWasHalfAwake = this->bHalfAwake;

	//Check whether evil eye sees Beethro or decoy and wakes up.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	const UINT wSX = player.wX; //always use player's actual position,
	const UINT wSY = player.wY; //since he's the only Beethro who can turn invisible
	const bool bPlayerIsTarget = player.IsTarget();
	const bool bPlayerVisible = player.IsVisible();
	nOX = nGetOX(this->wO);
	nOY = nGetOY(this->wO);
	UINT cx = this->wX, cy = this->wY, wDist = 1;
	bool bReflected = false;
	bool bContinueGaze = true;
	do
	{
		bContinueGaze = GetNextGaze(this->pCurrentGame->pRoom,cx,cy,nOX,nOY,bReflected);
		if (cx >= this->pCurrentGame->pRoom->wRoomCols || cy >= this->pCurrentGame->pRoom->wRoomRows)
			break; //out of bounds -- nothing can happen here
		const bool bIsPlayer = cx == wSX && cy == wSY;
		const bool bCanSmell = wDist <= DEFAULT_SMELL_RANGE && !bReflected;
		bool bIsTarget = false;
		bool bHastedTarget = false;
		if (bIsPlayer)
		{
			bIsTarget = bPlayerIsTarget && (bPlayerVisible || bCanSmell);
			bHastedTarget = this->pCurrentGame->swordsman.bIsHasted;
		} else {
			CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(cx, cy);
			if (pMonster && pMonster->IsMonsterTarget())
			{
				if (pMonster->wType == M_CLONE || pMonster->wType == M_TEMPORALCLONE)
				{
					bIsTarget = bCanSmell || !pMonster->IsHiding();
					bHastedTarget = player.bIsHasted;
				} else {
					bIsTarget = bCanSmell || !bCanEntityHideInShallowWater(pMonster->GetResolvedIdentity())
						|| !bIsShallowWater(this->pCurrentGame->pRoom->GetOSquare(cx,cy));
				}
			}
		}

		if (bIsTarget)
		{
			//Eye spots target.
			if (bHastedTarget && !bWasHalfAwake)
			{
				//Didn't see hasted target clearly  yet -- wake up if seen again next turn.
				this->bHalfAwake = true;
				//Keep looking further in case something else can wake up the eye now.
			} else {
				//Eye wakes up.
				this->bIsActive = true;
				pCoord = new CMoveCoord(this->wX, this->wY, this->wO);
				CueEvents.Add(CID_EvilEyeWoke, pCoord, true);
				return true;
			}
		}
		++wDist;
	} while (bContinueGaze);

	ASSERT(!this->bIsActive);

	if (bWasHalfAwake)  //eye saw hasted entity last turn, but nothing this turn:
		this->bHalfAwake = false; //so eye goes fully back to sleep
	return false;
}
