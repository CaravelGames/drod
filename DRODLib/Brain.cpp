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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Brain.cpp
//Implementation of CBrain.

#include "Brain.h"
#include "CurrentGame.h"

//*****************************************************************************
bool CBrain::CanFindSwordsman()
//Overridable method for determining if a monster can find the player on its own.
//Currently used by movement routines to see if the brain will direct monsters
//toward the player or not
//
//Returns:
//True if monster can find the player, false if not.
const
{
	if (this->bNewStun)
		return false;

	ASSERT(this->pCurrentGame);
	UINT wSX, wSY;
	const bool bSwordsmanInRoom = this->pCurrentGame->GetSwordsman(wSX, wSY);
	if (!bSwordsmanInRoom)
		return false; //don't check for decoys/monster enemies

	//If player is visible, monster can see him.
	if (this->pCurrentGame->swordsman.IsTarget())
	{
		if (this->pCurrentGame->swordsman.IsVisible())
			return true;
	} else {
		//NPC Beethro always visible and sensed.
		return true;
	}

	//Otherwise, monster can smell him if within range.
	if (CanSmellObjectAt(wSX, wSY))
		return true;

	//decoys/monster enemies will not send monsters toward an invisible player

	return false;	//player not found
}