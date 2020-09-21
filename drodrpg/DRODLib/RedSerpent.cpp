// $Id: RedSerpent.cpp 10108 2012-04-22 04:54:24Z mrimer $

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

#include "RedSerpent.h"

//
//Public methods.
//

//*****************************************************************************************
CRedSerpent::CRedSerpent(CCurrentGame *pSetCurrentGame)
	: CSerpent(M_SERPENT, pSetCurrentGame)
{
}

//*****************************************************************************************
/*
bool CRedSerpent::OnStabbed(
//Stabbing a red serpent in the head kills it, and in the tail shortens it by one tile.
//
//Returns: whether monster was killed
//
//Params:
	CCueEvents &CueEvents, const UINT wX, const UINT wY)
{
	if (wX == this->wX && wY == this->wY)
		return true;   //indicates serpent was killed

	if (wX == this->tailX && wY == this->tailY)
	{
		//Remove tail segment.
		if (ShortenTail(CueEvents))
			return true;   //indicates serpent was killed

		CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(wX, wY,
				this->pCurrentGame->GetSwordMovement()), true);
	}

	return false;
}
*/

//*****************************************************************************************
void CRedSerpent::Process(
//Process a red serpent for movement:
//If it can't move forward, then it shrinks by one square.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents& /*CueEvents*/)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
}

