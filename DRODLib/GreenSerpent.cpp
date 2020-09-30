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

#include "GreenSerpent.h"
#include "Character.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CGreenSerpent::CGreenSerpent(CCurrentGame *pSetCurrentGame)
	: CSerpent(M_SERPENTG, pSetCurrentGame)
{
}

//*****************************************************************************************
void CGreenSerpent::Process(
//Process a green serpent for movement:
//Don't grow or shrink.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If serpent is too short (i.e. died from truncation), don't do anything here.
	if (this->tailX == this->wX && this->tailY == this->wY) return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	CDbRoom& room = *(this->pCurrentGame->pRoom);

	int dxFirst, dyFirst, dx, dy;
	if (!GetSerpentMovement(wX, wY, dxFirst, dyFirst, dx, dy))
	{
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
		return;
	}

	//Check for eating things.
	bool bAte = false;
	bool bAteFluff = false, bAtePuff = false;
	if (dx || dy)
	{
		const UINT wNewX = this->wX + dx, wNewY = this->wY + dy;
		const UINT wTile = room.GetTSquare(wNewX, wNewY);
		if (bIsTar(wTile))
		{
			room.RemoveStabbedTar(wNewX, wNewY, CueEvents);
			room.ConvertUnstableTar(CueEvents, true);
			CueEvents.Add(CID_TarstuffDestroyed, new CMoveCoordEx(wNewX, wNewY, nGetO(dx,dy), wTile), true);
			bAte = true;
		}
		if (wTile == T_FLUFF)
		{
			room.RemoveStabbedTar(wNewX, wNewY, CueEvents);
			room.ConvertUnstableTar(CueEvents, true);
			CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wNewX, wNewY, nGetO(dx,dy), T_FLUFF), true);
			bAteFluff = true;
		}
		CMonster *pMonster = room.GetMonsterAtSquare(wNewX, wNewY);
		if (pMonster)
		{
			//Eat monster.
			ASSERT(!pMonster->IsLongMonster());
			ASSERT(!pMonster->IsPiece());
			//Critical heros cause KillMonster to return false.
			//If this occurs, we have to remove the monster manually so that the
			//adder can still move onto the tile.
			if (pMonster->wType == M_FLUFFBABY)
			{
				room.KillMonster(pMonster,CueEvents,false,this);
				bAtePuff = true;
			} else {
				CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
				bool bMonsterRemoved = false;
				if (pMonster->bAlive) {
					room.GetCurrentGame()->CheckTallyKill(pMonster);
					bMonsterRemoved = room.KillMonster(pMonster, CueEvents, false, this);
				}
				if (!bMonsterRemoved)
				{
					//We need to allow the serpent to move onto this tile though,
					//so remove the dead critical hero off the tile.
					room.RemoveMonsterFromTileArray(pMonster);
				}
				bAte = !bAteFluff;
			}
		}
	} else {
		//Can't move -- shrink if a serpent's body is in the way, unless an arrow prevents this.
		ProcessWhenStuck(CueEvents, room);
		return;
	}

	// Move according to direction chosen.
	const bool bNPCBeethroDied = NPCBeethroDieCheck(this->wX + dx, this->wY + dy, CueEvents);
	if (!LengthenHead(dx,dy,nGetOX(wO),nGetOY(wO), CueEvents))
		return;	//can't move

	if (!bAte)  //grows one tile longer when eating
		if (ShortenTail(CueEvents))	//to keep length the same
		{
			ASSERT(!"CGreenSerpent::Process: incorrectly died from shortening");
			return;  //snake died (shouldn't happen for this kind of serpent)
		}

	if (bAtePuff)  //eating fluff is not nutritious
	{
		this->pCurrentGame->pRoom->ProcessPuffAttack(CueEvents,this->wX,this->wY);
		if (!bAlive)
			return;
	} else if (bAteFluff) {
		if (ShortenTail(CueEvents))
		{
			CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
			pGame->pRoom->KillMonster(this, CueEvents);
			pGame->TallyKill();
			return;
		}
	}
	
	if (!bNPCBeethroDied && IsOnSwordsman())
	{
		if (!NPCBeethroDieCheck(this->wX, this->wY, CueEvents))
		{
			CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
			pGame->SetDyingEntity(&pGame->swordsman, this);
			CueEvents.Add(CID_MonsterKilledPlayer, this);
		}
	}

	//ATTN: If head moves onto a sword, this is not considered a stab.
}

//*****************************************************************************************
void CGreenSerpent::ProcessWhenStuck(CCueEvents& CueEvents, CDbRoom& room)
{
	int dxNext = nGetOX(this->wO);
	int dyNext = nGetOY(this->wO);
	const bool bVert = !dxNext;
	bool bDead = false, bShortened = false;
	CMonster *pMonster = room.GetMonsterAtSquare(this->wX+dxNext,this->wY+dyNext);
	if (pMonster && pMonster->wType == M_SERPENTG &&
		!DoesArrowPreventMovement(this->wX, this->wY, dxNext, dyNext))
	{
		bShortened = true;
		bDead = ShortenTail(CueEvents);
	}
	if (bVert) {
		dxNext = 1; dyNext = 0;
	} else {
		dyNext = 1; dxNext = 0;
	}
	pMonster = room.GetMonsterAtSquare(this->wX+dxNext,this->wY+dyNext);
	if (!bShortened && pMonster && pMonster->wType == M_SERPENTG &&
		!DoesArrowPreventMovement(this->wX, this->wY, dxNext, dyNext))
	{
		bShortened = true;
		bDead = ShortenTail(CueEvents);
	}
	if (bVert)
		dxNext = -1;
	else
		dyNext = -1;
	pMonster = room.GetMonsterAtSquare(this->wX+dxNext,this->wY+dyNext);
	if (!bShortened && pMonster && pMonster->wType == M_SERPENTG &&
		!DoesArrowPreventMovement(this->wX, this->wY, dxNext, dyNext))
	{
		bShortened = true;
		bDead = ShortenTail(CueEvents);
	}

	//Serpent shortened itself to death.
	if (bDead)
	{
		CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
		pGame->pRoom->KillMonster(this, CueEvents);
		pGame->TallyKill();
	}
}

//*****************************************************************************************
bool CGreenSerpent::OnStabbed(
//Stabbing a green serpent in the head shortens it by one tile (at the tail end).
//
//Returns: whether monster was killed
//
//Params:
	CCueEvents &CueEvents, const UINT wX, const UINT wY, WeaponType /*weaponType*/)
{
	if (wX == this->wX && wY == this->wY)
	{
		//Remove tail segment.
		if (ShortenTail(CueEvents))
			return true;   //indicates serpent was killed

		CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(wX, wY,
				this->pCurrentGame->GetSwordMovement()), true);
	}
	return false;
}

//******************************************************************************************
bool CGreenSerpent::DoesSquareContainObstacle(
//Determines if a square contains an obstacle for a green serpent.
//Unlike other monsters, the serpent CAN move onto a sword square.
//Green serpents may eat tarstuff and other small monsters.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	//Check t-square for obstacle.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	UINT wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
		return true;

	//Green serpents can eat invulnerable (unstabbable) tarstuff.
	if (bIsTar(wLookTileNo) && room.IsTarVulnerableToStab(wCol,wRow))
		return true;

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check o-square obstacle.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		if (!bIsTunnel(wLookTileNo))
			return true;

		//Only move onto tunnel if Beethro is there.
		if (!this->pCurrentGame->IsPlayerAt(wCol, wRow) || !player.IsTarget())
			return true;
	}

	//Check for monster at square.
	//Large monsters and invincible NPCs can't be eaten.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
	{
		if (pMonster->IsLongMonster() || pMonster->IsPiece())
			return true;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsInvulnerable())
				return true;
		}
	}

	//No obstacle.
	return false;
}

//*****************************************************************************************
bool CGreenSerpent::IsTileObstacle(
// Override the normal serpent IsTileObstacle for green serpents.
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
		bIsAnyArrow(wLookTileNo) ||
		bIsPlatform(wLookTileNo) ||
		bIsTarOrFluff(wLookTileNo)   //green serpents can eat tarstuff
	));
}
