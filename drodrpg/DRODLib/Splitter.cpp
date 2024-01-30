// $Id: Splitter.cpp 10108 2012-04-22 04:54:24Z mrimer $

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

//Splitter.cpp
//Implementation of CSplitter.

#include "Splitter.h"
#include "MonsterPiece.h"
#include "Swordsman.h"
#include "Combat.h"

#include <math.h>

//
//Public methods.
//

//*****************************************************************************
CSplitter::CSplitter(CCurrentGame *pSetCurrentGame)
	: CMonster(M_ROCKGIANT, pSetCurrentGame)
	, wOrigX(0), wOrigY(0)
{}

//*****************************************************************************
bool CSplitter::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	ASSERT(IsAlive());

	//Touching a hot tile causes fractional damage.
	if (this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT)
	{
		CueEvents.Add(CID_MonsterBurned, this);
		if (Damage(CueEvents, int(this->pCurrentGame->pPlayer->st.hotTileVal)))
			return true;
	}

	//All pieces are harmed.
	CMonsterPiece *pPiece;
	list<CMonsterPiece*>::const_iterator piece=this->Pieces.begin();
	while (piece != this->Pieces.end())
	{
		pPiece = *piece++;
		const UINT wX = pPiece->wX, wY = pPiece->wY;
		if (this->pCurrentGame->pRoom->GetOSquare(wX, wY) == T_HOT)
		{
			if (IsDamageableAt(wX, wY))
			{
				CueEvents.Add(CID_MonsterBurned, pPiece);
				if (DamagePiece(pPiece, CueEvents, int(this->pCurrentGame->pPlayer->st.hotTileVal)))
					return true;
			}
		}
	}

	return false;
}

//*****************************************************************************
bool CSplitter::IsOpenMove(const int dx, const int dy) const
//Returns: whether moving from the current position along (dx,dy) is valid
{
	ASSERT(dx || dy);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const CMonster *pMonster;

	//this->wX/Y is stored in these vars while calculating movement
	const UINT wRootX = this->wOrigX;
	const UINT wRootY = this->wOrigY;

	pMonster = room.GetMonsterAtSquare(wRootX + dx, wRootY + dy);
	if (pMonster && pMonster->IsPiece())
	{
		CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*, CMonster*, (CMonster*)pMonster);
		pMonster = pPiece->pMonster;
	}
	if (DoesArrowPreventMovement(wRootX, wRootY, dx, dy) ||
			room.DoesSquarePreventDiagonal(wRootX, wRootY, dx, dy))
		return false;
	if (pMonster != this) //don't need to check for obstacle where monster is already
		if (DoesSquareContainObstacle(wRootX + dx, wRootY + dy))
			return false;

	//Ensure movement is open for all other tile pieces.
	for (list<CMonsterPiece*>::const_iterator piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		const CMonsterPiece& mpiece = *(*piece);
		pMonster = room.GetMonsterAtSquare(mpiece.wX + dx, mpiece.wY + dy);
		if (pMonster)
			pMonster = pMonster->GetOwningMonsterConst();
		if (DoesArrowPreventMovement(mpiece.wX, mpiece.wY, dx, dy) ||
				room.DoesSquarePreventDiagonal(mpiece.wX, mpiece.wY, dx, dy))
			return false;
		if (pMonster != this) //don't need to check for obstacle where monster is already
			if (DoesSquareContainObstacle(mpiece.wX + dx, mpiece.wY + dy))
				return false;
	}
	return true;
}

//*****************************************************************************
inline bool bIsObstacle(const UINT wTile)
//Returns: whether splitter shard cannot be placed on this tile type
{
	//Types listed here are not placement obstacles.
	return !(wTile==T_EMPTY ||
				bIsFloor(wTile) ||
				bIsOpenDoor(wTile) ||
				bIsPlatform(wTile) ||
				bIsPowerUp(wTile) ||
				bIsMap(wTile) ||
				bIsShovel(wTile) ||
				wTile==T_NODIAGONAL ||
				wTile==T_SCROLL ||
				wTile==T_FUSE ||
				wTile==T_KEY ||
				bIsEquipment(wTile) ||
				wTile==T_TOKEN ||
				bIsAnyArrow(wTile)
			);
}

//*****************************************************************************************
void CSplitter::Process(
//Process a splitter for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents& /*CueEvents*/)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Choose monster tile closest to the target to be considered the "front", from
	//where movement is considered.  Use Euclidean distance to minimize diagonal movement.
	this->wOrigX = this->wX;
	this->wOrigY = this->wY;
	int xd, yd;
	xd = this->wX - wX;
	yd = this->wY - wY;
	UINT wDistSq, wBestDistSq = xd*xd + yd*yd;
	list<CMonsterPiece*>::const_iterator piece;
	for (piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		const CMonsterPiece& mpiece = *(*piece);
		xd = mpiece.wX - wX;
		yd = mpiece.wY - wY;
		wDistSq = xd*xd + yd*yd;
		if (wDistSq < wBestDistSq)
		{
			//Use this tile as the "front".
			wBestDistSq = wDistSq;
			this->wX = mpiece.wX;
			this->wY = mpiece.wY;
		}
	}

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	const bool bRes = GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, SmartOmniDirection);
	this->wX = this->wOrigX; //restore root coords
	this->wY = this->wOrigY;
	if (!bRes)
		return;

	SetOrientation(dxFirst, dyFirst);
}

//*****************************************************************************************
void CSplitter::ReflectX(CDbRoom *pRoom)
{
	CMonster::ReflectX(pRoom);

	//Restore correct relative piece positions.
	--this->wX;
	ASSERT(this->Pieces.size() == 3);
	list<CMonsterPiece*>::iterator piece = this->Pieces.begin();
	++(*(piece++))->wX;
	--(*(piece++))->wX;
	++(*piece)->wX;
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY)]);
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY+1)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY+1)]);
}

//*****************************************************************************************
void CSplitter::ReflectY(CDbRoom *pRoom)
{
	CMonster::ReflectY(pRoom);

	//Restore correct relative piece positions.
	--this->wY;
	ASSERT(this->Pieces.size() == 3);
	list<CMonsterPiece*>::iterator piece = this->Pieces.begin();
	--(*(piece++))->wY;
	++(*(piece++))->wY;
	++(*piece)->wY;
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY+1)]);
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY+1)]);
}

//*****************************************************************************************
void CSplitter::RotateClockwise(CDbRoom *pRoom)
{
	CMonster::RotateClockwise(pRoom);

	//Restore correct relative piece positions.
	--this->wX;
	ASSERT(this->Pieces.size() == 3);
	list<CMonsterPiece*>::iterator piece = this->Pieces.begin();
	--(*(piece++))->wY;
	++(*(piece++))->wY;
	++(*piece)->wX;
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY)]);
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY+1)]);
	std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY+1)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX+1,this->wY+1)]);
}

//
// Private methods.
//

//*****************************************************************************
bool CSplitter::DamagePiece(CMonster* pPiece, CCueEvents &CueEvents, int damageVal)
//Damage monster piece a certain amount.
//
{
	ASSERT(pPiece);

	//A copy of CMonster::Damage, with the CueEvent altered.
	UINT delta;
	if (damageVal < 0)
	{
		//Flat-rate damage.
		delta = -damageVal;
	} else {
		//Fractional damage (percent HP lost).
		const float fDelta = this->HP * (damageVal / 100.0f);
		if (fDelta <= 0.0f)
			return false; //no damage
		delta = UINT(ceil(fDelta)); //ceiling function
	}
	if (delta >= this->HP)
		this->HP = 0;
	else
		this->HP -= delta;

	CueEvents.Add(CID_EntityAffected,
			new CCombatEffect(pPiece, CET_HARM, delta), true);

	return !this->HP; //monster died?
}

//*****************************************************************************************
void CSplitter::MovePiece(
//Called instead of CMonster::Move to handle moving pieces of this large monster
//one at a time.
//
//Params:
	CMonster* /*pMonster*/,  //piece being moved
	const int /*dx*/, const int /*dy*/,  //movement offset
	const UINT /*wSX*/, const UINT /*wSY*/, //swordsman position (speed optimization)
	CCueEvents& /*CueEvents*/) //(in/out)
{
/*
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wDestX = pMonster->wX + dx, wDestY = pMonster->wY + dy;

	//If a monster target is at the destination square, it will die.
	//Remove it from the room's monster array to avoid a pointer overwrite assertion.
	CMonster *pDestMonster = room.GetMonsterAtSquare(wDestX,wDestY);
	if (pDestMonster)
	{
		//Remove it from the room's monster array to avoid a pointer overwrite assertion.
		room.pMonsterSquares[room.ARRAYINDEX(wDestX, wDestY)] = NULL;
	}

	//Place monster piece on destination tile.
	ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(wDestX,wDestY)]);
	room.pMonsterSquares[room.ARRAYINDEX(wDestX,wDestY)] = pMonster;

	//Set new coords.
	pMonster->wX = wDestX;
	pMonster->wY = wDestY;

	//Check for stepping on pressure plate.
	if (room.GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
		room.ActivateOrb(wDestX, wDestY, CueEvents, OAT_PressurePlate);

	//Did monster step on swordsman?
	if (wDestX == wSX && wDestY == wSY)
	{
		if (wDestX == this->pCurrentGame->swordsman.wX &&
			 wDestY == this->pCurrentGame->swordsman.wY)
			CueEvents.Add(CID_MonsterKilledPlayer, pMonster);
		else
			CueEvents.Add(CID_NPCBeethroDied, pMonster);
	}
*/
}
