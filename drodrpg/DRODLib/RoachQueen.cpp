// $Id: RoachQueen.cpp 9299 2008-10-29 02:32:54Z mrimer $

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

//RoachQueen.cpp
//Implementation of CRoachQueen.

#include "RoachQueen.h"
#include "Combat.h"
#include "Character.h"

//
//Public methods.
//

//*****************************************************************************************
void CRoachQueen::Process(
//Process a roach queen for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Shall queen lay an egg?
	if (IsSpawnEggTriggered(CueEvents))
	{
		SpawnEgg(CueEvents);
	}
}
