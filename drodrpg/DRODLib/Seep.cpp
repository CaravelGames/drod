// $Id: Seep.cpp 9300 2008-10-29 02:33:35Z mrimer $

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

//Seep.cpp
//Implementation of CSeep.

#include "Seep.h"

//
//Public methods.
//

//*****************************************************************************************
bool CSeep::KillIfOutsideWall(CCueEvents &CueEvents)
//Kill the monster if outside wall.
//
//Returns: whether monster was killed
{
//	if (IsOnSwordsman())
//		return false; //retain for front end display death sequence

	//If ghost was on a door that opened, kill it.
	const UINT dwTileNo = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	if (bIsOpenDoor(dwTileNo) || bIsFloor(dwTileNo))
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		if (pGame->IsFighting(this))
			return false; //if terrain changes during a fight, just let the combat play out

		pGame->pRoom->KillMonster(this, CueEvents);
//		pGame->TallyKill();
		SetKillInfo(NO_ORIENTATION); //center stab effect
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		return true;
	}
	return false;
}

//*****************************************************************************************
void CSeep::Process(
//Process a seep for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (KillIfOutsideWall(CueEvents))
		return;

	FaceTarget();

	if (AttackPlayerWhenAdjacent(CueEvents))
		CueEvents.Add(CID_GoblinAttacks);
}
