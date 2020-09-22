// $Id: Ant.cpp 8914 2008-04-24 02:58:16Z mrimer $

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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Ant.cpp
//Implementation of CAnt.

#include "Ant.h"

//
//Public methods.
//

//*****************************************************************************************
CAnt::CAnt(CCurrentGame *pSetCurrentGame)
	: CMonster(M_WATERSKIPPER, pSetCurrentGame, WATER)
{}

//*****************************************************************************************
void CAnt::Process(
//Process an ant for movement.
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	FaceTarget();

	if (AttackPlayerWhenAdjacent(CueEvents))
		CueEvents.Add(CID_GoblinAttacks);
}

