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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002 
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//TarBaby.cpp
//Implementation of CTarBaby.

#include "TarBaby.h"

//
//Public methods.
//

//*****************************************************************************************
void CTarstuffBaby::Process(
//Process a tarbaby for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy))
		return;
 
	//Move tarbaby to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}

//*****************************************************************************************
void CGelBaby::Process(
//Process a gelbaby for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, DirectOnly))	//dumber
		return;
 
	//Move tarbaby to new destination square.
	MakeStandardMove(CueEvents,dx,dy);
	SetOrientation(dxFirst, dyFirst);
}
