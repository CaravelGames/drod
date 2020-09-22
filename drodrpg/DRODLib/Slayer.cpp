// $Id: Slayer.cpp 10232 2012-05-23 06:03:47Z mrimer $

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

//Slayer.cpp
//Implementation of CSlayer.

#include "Slayer.h"
#include "Swordsman.h"

//
//Public methods.
//

//*****************************************************************************************
CSlayer::CSlayer(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not for game processing.
	: CPlayerDouble(M_SLAYER, pSetCurrentGame)
{
}

//*****************************************************************************************
void CSlayer::Process(
//Process Slayer for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents& /*CueEvents*/)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	UINT wX, wY;
	if (!GetTarget(wX,wY))
		return;

	if (nDist(this->wX, this->wY, wX, wY) < 2)
	{
		FaceTarget();
	} else {
		//Face opposite the way the player is facing.
		const CSwordsman& player = *this->pCurrentGame->pPlayer;
		SetOrientation(-nGetOX(player.wO),-nGetOY(player.wO));
	}
}
