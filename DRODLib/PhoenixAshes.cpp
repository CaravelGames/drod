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

//PhoenixAshes.cpp
//Implementation of CPhoenixAshes.

#include "PhoenixAshes.h"

#include "CueEvents.h"
#include "GameConstants.h"

//
//Public methods.
//

CPhoenixAshes::CPhoenixAshes(CCurrentGame *pSetCurrentGame)
	: CMonster(M_FEGUNDOASHES, pSetCurrentGame, GROUND)
{
	this->wO = S;  //begin respawn timer
}

//*****************************************************************************************
void CPhoenixAshes::Process(
//Process a phoenix for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Use orientation as timer for rebirth into a fegundo.
	switch (this->wO)
	{
	case S: 
		this->wO = SW;
		break;
	case SW:
		this->wO = W;
		break;
	case W:
		this->wO = NW;
		break;
	case NW:
		this->wO = N;
		break;
	case N:
		// Get rid of the ashes and spawn a new fegundo.
		CueEvents.Add(CID_AshToFegundo, this);
		break;
	default:
		ASSERT(!"Bad orientation.");
		break;
	}
}
