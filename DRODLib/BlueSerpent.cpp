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

#include "BlueSerpent.h"

#include "CueEvents.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CBlueSerpent::CBlueSerpent(CCurrentGame *pSetCurrentGame)
	: CSerpent(M_SERPENTB, pSetCurrentGame)
{
}

//*****************************************************************************************
void CBlueSerpent::Process(
//Process a blue serpent for movement:
//Don't grow or shrink.  Serpent only shrinks when stabbed in the tail.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (this->tailX == this->wX && this->tailY == this->wY)
		return;  //snake died from stabbing

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	int dxFirst, dyFirst, dx, dy;
	if (!GetSerpentMovement(wX, wY, dxFirst, dyFirst, dx, dy))
	{
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
		return;
	}

	bool bFluffPoison = false;
	const UINT wDestX = this->wX + dx;
	const UINT wDestY = this->wY + dy;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	CMonster *pMonster = room.GetMonsterAtSquare(wDestX, wDestY);
	if (pMonster && pMonster->wType == M_FLUFFBABY)
	{
		room.KillMonster(pMonster,CueEvents,false,this);
		bFluffPoison = true;
	}

	// Move according to direction chosen.
	const bool bNPCBeethroDied = NPCBeethroDieCheck(wDestX, wDestY, CueEvents);
	if (!LengthenHead(dx,dy,nGetOX(wO),nGetOY(wO), CueEvents))
		return;	//can't move

	if (ShortenTail(CueEvents))	//to keep length the same
	{
		ASSERT(!"CBlueSerpent::Process: incorrectly died from shortening");
		return;  //snake died (shouldn't happen for this kind of serpent)
	}
	if (bFluffPoison)
	{
		room.ProcessPuffAttack(CueEvents,wDestX,wDestY);
		if (!bAlive)
			return;
	}

	if (IsOnSwordsman())
	{
		if (!bNPCBeethroDied)
		{
			CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
			pGame->SetDyingEntity(&pGame->swordsman, this);
			CueEvents.Add(CID_MonsterKilledPlayer, this);
		}
	}
	else
	{
		//If tail moves onto a sword, consider this a stab too.
		//(Only check for this if I didn't just eat the player.)
		CCoordIndex SwordCoords;
		this->pCurrentGame->pRoom->GetSwordCoords(SwordCoords, false, true);
		if (SwordCoords.Exists(this->tailX,this->tailY))
			if (OnStabbed(CueEvents, this->tailX, this->tailY))
			{
				CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
				//Don't need to know whose sword killed the Blue Serpent, since KillMonster
				//only uses KillingEntity for Halph and Critical NPC deaths
				pGame->pRoom->KillMonster(this, CueEvents);
				pGame->TallyKill();
			}
	}
}

//*****************************************************************************************
bool CBlueSerpent::OnStabbed(
//Stabbing a blue serpent in the tail shortens it by one tile.
//
//Returns: whether monster was killed
//
//Params:
	CCueEvents &CueEvents, const UINT wX, const UINT wY, WeaponType /*weaponType*/)
{
	if (wX == this->tailX && wY == this->tailY)
	{
		//Remove tail segment.
		if (ShortenTail(CueEvents))
			return true;   //indicates blue serpent was killed

		CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(wX, wY,
				this->pCurrentGame->GetSwordMovement()), true);
	}
	return false;
}

//*****************************************************************************************
bool CBlueSerpent::IsTileObstacle(
// Override the normal serpent IsTileObstacle for blue serpents.
// 
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.  Note each tile# will
						//    always be found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	return (!(
		bIsFloor(wLookTileNo) ||
		bIsOpenDoor(wLookTileNo) ||
		wLookTileNo == T_EMPTY ||
		wLookTileNo == T_NODIAGONAL ||
		wLookTileNo == T_SCROLL ||
		wLookTileNo == T_TOKEN ||
		wLookTileNo == T_FUSE ||
		bIsShallowWater(wLookTileNo) ||
		bIsPlatform(wLookTileNo) ||
		bIsAnyArrow(wLookTileNo)
	));
}
