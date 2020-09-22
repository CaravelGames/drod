// $Id: Goblin.cpp 8665 2008-02-23 17:58:37Z mrimer $

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

//Goblin.cpp
//Implementation of CGoblin.

#include "Goblin.h"

//
//Public methods.
//

//*****************************************************************************************
void CGoblin::Process(
//Process a goblin for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a CueEvent object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	FaceTarget();

	if (AttackPlayerInFrontWhenBackIsTurned(CueEvents))
		CueEvents.Add(CID_GoblinAttacks);
}
