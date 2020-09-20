// $Id: RoachEgg.cpp 10108 2012-04-22 04:54:24Z mrimer $

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
 *
 * ***** END LICENSE BLOCK ***** */

//RoachEgg.cpp
//Implementation of CRoachEgg.

#include "RoachEgg.h"

//
//Public methods.
//

//*****************************************************************************************
CRoachEgg::CRoachEgg(CCurrentGame *pSetCurrentGame) : CMonster(M_REGG, pSetCurrentGame)
{
	// Egg deposited at 8 o'clock, born at midnight (5 moves in all).
	this->wO = SW;
}

//*****************************************************************************************
void CRoachEgg::Process(
//Process a roach egg for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents& /*CueEvents*/)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	switch (this->wO)
	{
	case SW:
		this->wO = W;
		break;
	case W:
		this->wO = NW;
		break;
	case NW:
		this->wO = N;
		break;
	default:
	case N:
//Just stay an egg forever.
/*
		// Get rid of the egg and spawn a roach.
		CueEvents.Add(CID_EggHatched, this);
*/
		break;
	}
}

