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
 * Richard Cookney (timeracer), Michael Welsh Duggan (md5i)
 *
 * ***** END LICENSE BLOCK ***** */

//Spider.cpp
//Implementation of CSpider.

#include "Spider.h"

#include "CurrentGame.h"
#include "DbRooms.h"

const UINT wSenseRadius = 2;	//within this distance spiders are always visible

//
//Public methods.
//

//*****************************************************************************************
void CSpider::SetVisibility()
//Returns: whether spider is visible to player
{
	this->bSpiderVisible = (DistToSwordsman(false, true) <= wSenseRadius) ||
			this->pCurrentGame->pRoom->bBetterVision;
}

//*****************************************************************************************
void CSpider::Process(
//Process a spider for movement.
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

	//Find where to move to.
	UINT wX, wY;
	bool bHaveTarget = GetTarget(wX, wY);
	if (!bHaveTarget)
		this->pCurrentGame->GetSwordsman(wX, wY, true);

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, SmartDiagonalOnly, true))
	{
		SetVisibility(); //spider invisible while stationary
		return;
	}

	//Make sure we're visible to nearby playable non-targets as well
	//(This also makes spiders constantly face non-target players.)
	if (!bHaveTarget)
	{
		SetOrientation(dxFirst, dyFirst);
		SetVisibility();
		return;
	}

	//Move spider to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
	this->bSpiderVisible = true; //spiders visible when moving
	if (!dx && !dy && DistToSwordsman(false, true) > wSenseRadius)
		SetVisibility(); //spider invisible while stationary
}
