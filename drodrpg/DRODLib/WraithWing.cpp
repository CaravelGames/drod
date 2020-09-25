// $Id: WraithWing.cpp 10108 2012-04-22 04:54:24Z mrimer $

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
 * Michael Welsh Duggan (md5i)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//WraithWing.cpp
//Implementation of CWraithWing.

#include "WraithWing.h"

//
//Public methods.
//

//*****************************************************************************************
void CWraithWing::Process(
//Process a wraith-wing for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents& /*CueEvents*/)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY))
		return;
	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy))
		return;
	//Face target.
	SetOrientation(dxFirst, dyFirst);

/*
	int dxFirst, dyFirst, dx, dy;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	const int distance = nDist(this->wX, this->wY, wX, wY);

	// Get directed movement toward swordsman
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy))
		return;

	//Wraith-wings are cowards and only attack in numbers.
	if (distance <= 5)
	{
		// Let's look for another wraith-wing, ready to pounce.
		bool runaway = true;
		for (CMonster *pSeek = this->pCurrentGame->pRoom->pFirstMonster;
			  pSeek != NULL; pSeek = pSeek->pNext)
		{
			if (pSeek->wType != M_WWING ||
				 (pSeek->wX == this->wX && pSeek->wY == this->wY))
				continue;
			const int dist2 = nDist(pSeek->wX, pSeek->wY, wX, wY);
			//  We want a wraith-wing who is within 2 of the same
			//  distance we are from the swordsman, but who is at least
			//  3 away from us.  (Based on old code.)
			if (abs (dist2 - distance) <= 2 &&
				 nDist(pSeek->wX, pSeek->wY, this->wX, this->wY) >= 3)
			{
				// Another wraith-wing is able to collaborate
				runaway = false;
				break;
			}
		}
		if (runaway)
			if (distance == 5) 
				dx = dy = 0;   // Don't get involved
			else
			{
				// Flee!
				dxFirst = dx = -dxFirst;
				dyFirst = dy = -dyFirst;
				GetBestMove(dx, dy);
			}
	}

	//Move wraith-wing to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
*/
}

