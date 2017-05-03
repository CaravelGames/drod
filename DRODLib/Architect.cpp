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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2012
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "Architect.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//******************************************************************************
CArchitect::CArchitect(CCurrentGame *pSetCurrentGame)
	: CCitizen(pSetCurrentGame, M_ARCHITECT, SPD_ARCHITECT) //move after monsters, before citizens
{
}

//******************************************************************************
void CArchitect::Process(
//Architect takes its turn.
//
//Params:
	const int /*nLastCommand*/,   //(in) Player command on this turn
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
		//with codes indicating events that happened that may correspond to
		//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	const CDbRoom& room = *(this->pCurrentGame->pRoom);
	this->bDone = room.building.empty();

	if (!this->bDone) {
		this->bHasSupply = true; //always attempting to build
		CalcPathAndAdvance(CueEvents);
	}
}
