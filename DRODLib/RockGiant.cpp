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

//Implementation of CRockGiant.

#include "RockGiant.h"
#include "MonsterPiece.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************
CRockGiant::CRockGiant(CCurrentGame *pSetCurrentGame)
	: CMonster(M_ROCKGIANT, pSetCurrentGame, GROUND_AND_SHALLOW_WATER)
	, wOrigX(0), wOrigY(0)
{}

//*****************************************************************************
bool CRockGiant::IsOpenMove(const int dx, const int dy) const
//Returns: whether moving from the current position along (dx,dy) is valid
{
	ASSERT(dx || dy);
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	//this->wX/Y is stored in these vars while calculating movement
	const UINT wRootX = this->wOrigX;
	const UINT wRootY = this->wOrigY;

	if (DoesArrowPreventMovement(wRootX, wRootY, dx, dy) ||
			room.DoesSquarePreventDiagonal(wRootX, wRootY, dx, dy))
		return false;
	const CMonster *pMonster = room.GetOwningMonsterOnSquare(wRootX + dx, wRootY + dy);
	if (pMonster != this) //don't need to check for obstacle where monster is already
		if (DoesSquareContainObstacle(wRootX + dx, wRootY + dy))
			return false;

	//Ensure movement is open for all other tile pieces.
	for (MonsterPieces::const_iterator piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		const CMonsterPiece& mpiece = *(*piece);
		if (DoesArrowPreventMovement(mpiece.wX, mpiece.wY, dx, dy) ||
				room.DoesSquarePreventDiagonal(mpiece.wX, mpiece.wY, dx, dy))
			return false;
		pMonster = room.GetOwningMonsterOnSquare(mpiece.wX + dx, mpiece.wY + dy);
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
				bIsPotion(wTile) ||
				wTile==T_NODIAGONAL ||
				wTile==T_SCROLL ||
				wTile==T_FUSE ||
				wTile==T_TOKEN ||
				bIsShallowWater(wTile) ||
				bIsArrow(wTile)
			);
}

//*****************************************************************************
void CRockGiant::Shatter(
//Shatter a splitter monster at (wX,wY)
	CCueEvents& CueEvents, CCurrentGame *pGame, const UINT wX, const UINT wY,
	const bool bBreakOverPit, //[default=false]
	const ROOMCOORD *pWeaponDamagePosition) //[default=NULL]
{
	ASSERT(pGame);
	CDbRoom& room = *(pGame->pRoom);

	static CCoordIndex swordCoords;
	room.GetSwordCoords(swordCoords, false, true); //resets var

	if (pWeaponDamagePosition != NULL){
		swordCoords.Add(pWeaponDamagePosition->wX, pWeaponDamagePosition->wY);
	}

	//Don't place shards where an explosion is occurring now.
	CCoordSet coords;
	const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
		CueEvents.GetFirstPrivateData(CID_Explosion));
	while (pCoord)
	{
		coords.insert(pCoord->wX, pCoord->wY);
		pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, CueEvents.GetNextPrivateData());
	}

	//Split off new monsters to free tiles on and around monster.
	static const UINT POSITIONS = 8;
	static const int dx[POSITIONS] = {0, 0, 1, 1, -1, -1, +2, +2}; //offset from top-left corner
	static const int dy[POSITIONS] = {0, 1, 0, 1, -1, +2, -1, +2};
	static const int orientation[POSITIONS] = {NW, SW, NE, SE, NW, SW, NE, SE};
	UINT wNewX, wNewY, wTile;
	for (UINT i=4; i--; )	//only break on four origin tiles //POSITIONS; i--; )
	{
		wNewX = wX + dx[i];
		wNewY = wY + dy[i];

		//Decide where shards may be placed.
		//Code checks below are taken from DoesSquareContainObstacle.
		if (!room.IsValidColRow(wNewX,wNewY)) continue;
		if (room.GetMonsterAtSquare(wNewX, wNewY) != NULL) continue;
		wTile = room.GetTSquare(wNewX, wNewY);
		if (bIsObstacle(wTile)) continue;
		if (i >= 4)
		{
			wTile = room.GetFSquare(wNewX, wNewY);
			if (bIsArrow(wTile) && bIsArrowObstacle(wTile, orientation[i]))
				continue;
			if (wTile == T_NODIAGONAL) continue;
		}
		const UINT wOTile = room.GetOSquare(wNewX, wNewY);
		if (bIsObstacle(wOTile))
		{
			if (!bIsDoor(wOTile) || i >= 4) //rock golems at source tiles may spawn on closed doors
			{
				if (!(bIsPit(wOTile) || bIsDeepWater(wOTile)) || !bBreakOverPit)
					continue;
			}
		}
		if (pGame->IsPlayerAt(wNewX, wNewY)) continue;
		if (coords.has(wNewX, wNewY)) continue;

		if (bIsShallowWater(wOTile) && swordCoords.Exists(wNewX,wNewY))
		{
			//Create a stepping stone instead
			room.Plot(wNewX,wNewY,T_STEP_STONE);
			CueEvents.Add(CID_Splash, new CCoord(wNewX,wNewY), true);
			continue;
		}

		CMonster *m = room.AddNewMonster(M_ROCKGOLEM, wNewX, wNewY);
		m->bIsFirstTurn = true;
		m->wO = orientation[i];
		if (swordCoords.Exists(wNewX, wNewY) || bIsPit(wOTile))
			m->OnStabbed(CueEvents); //turn into pile of rubble
		if (wOTile == T_PRESSPLATE)
			room.ActivateOrb(wNewX, wNewY, CueEvents, OAT_PressurePlate);
	}
}

 //****************************************************************************
void CRockGiant::MyClosestTile(
//This routine outputs the location of a movable part of this monster
//closest to the given coordinates.
//
//Params:
	const UINT wX, const UINT wY, // (in) target coordinates
	UINT &wMyCX, UINT &wMyCY)	// (out) coordinates of closest piece of rock giant
const
{
	wMyCX = this->wOrigX; //pick the NW piece by default
	wMyCY = this->wOrigY;

	if (wX > wMyCX)
		++wMyCX; //if the target coordinate is more easterly, then pick eastern pieces
	if (wY > wMyCY)
		++wMyCY; //if the target coordinate is more southerly, then pick southern pieces
}

//*****************************************************************************************
void CRockGiant::Process(
//Process a splitter for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	this->wOrigX = this->wX;
	this->wOrigY = this->wY;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY)) return;

	//Choose monster tile closest to the target to be considered the "front", from
	//where movement is considered.  Use Euclidean distance to minimize diagonal movement.
	int xd, yd;
	xd = this->wX - wX;
	yd = this->wY - wY;
	UINT wDistSq, wBestDistSq = xd*xd + yd*yd;
	MonsterPieces::const_iterator piece;
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
	const bool bRes = GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, SmartOmniFlanking);
	this->wX = this->wOrigX; //restore root coords
	this->wY = this->wOrigY;
	if (!bRes)
		return;

	SetOrientation(dxFirst, dyFirst);
	if (!dx && !dy)
		return;

	//Remove off old tiles.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = NULL;
	for (piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
	{
		const CMonsterPiece& mpiece = *(*piece);
		room.pMonsterSquares[room.ARRAYINDEX(mpiece.wX,mpiece.wY)] = NULL;
	}

	//Move onto destination tiles.
	MovePiece(this, dx, dy, CueEvents);
	for (piece = this->Pieces.begin(); piece != this->Pieces.end(); ++piece)
		MovePiece(*piece, dx, dy, CueEvents);
}

//*****************************************************************************************
void CRockGiant::ReflectX(CDbRoom *pRoom)
{
	CMonster::ReflectX(pRoom);

	//Restore correct relative piece positions.
	--this->wX;
	ASSERT(this->Pieces.size() == 3);
	MonsterPieces::iterator piece = this->Pieces.begin();
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
void CRockGiant::ReflectY(CDbRoom *pRoom)
{
	CMonster::ReflectY(pRoom);

	//Restore correct relative piece positions.
	--this->wY;
	ASSERT(this->Pieces.size() == 3);
	MonsterPieces::iterator piece = this->Pieces.begin();
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
void CRockGiant::MovePiece(
//Called instead of CMonster::Move to handle moving pieces of this large monster
//one at a time.
//
//Params:
	CMonster *pMonster,  //piece being moved
	const int dx, const int dy,  //movement offset
	CCueEvents &CueEvents) //(in/out)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wDestX = pMonster->wX + dx, wDestY = pMonster->wY + dy;
	
	//Keep track of the parent Giant entity to set kill info
	CMonster *pGiant = pMonster;
	pGiant = pGiant->GetOwningMonster();

	//If a monster target is at the destination square, it will die.
	//Remove it from the room's monster array to avoid a pointer overwrite assertion.
	bool bFluffPoison = false;
	CMonster *pDestMonster = room.GetMonsterAtSquare(wDestX,wDestY);
	if (pDestMonster)
	{
		if (pDestMonster->wType == M_FLUFFBABY)
		{
			room.KillMonster(pDestMonster,CueEvents,false,pGiant);
			bFluffPoison = true;
		} else {
			ASSERT(pDestMonster->IsAttackableTarget());
			if (bIsSmitemaster(pDestMonster->GetResolvedIdentity()))
			{
				CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
				pGame->SetDyingEntity(pDestMonster, pGiant);
				CueEvents.Add(CID_NPCBeethroDied, this);
				pDestMonster->bAlive = false;
			}
			else
			{
				CueEvents.Add(CID_MonsterDiedFromStab, pDestMonster);
				VERIFY(room.KillMonster(pDestMonster, CueEvents, false, pGiant));
				//store info about kill in the monster orientation
				//(can't do this if monster is still being shown in room)
				pDestMonster->wO = nGetO(wDestX-this->wX, wDestY-this->wY);
				if (!IsValidOrientation(pDestMonster->wO))
					pDestMonster->wO = NO_ORIENTATION;
			}
		}

		//Remove it from the room's monster array to avoid a pointer overwrite assertion.
		room.pMonsterSquares[room.ARRAYINDEX(wDestX, wDestY)] = NULL;
	}

	//Place monster piece on destination tile.
	ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(wDestX,wDestY)]);
	room.pMonsterSquares[room.ARRAYINDEX(wDestX,wDestY)] = pMonster;

	//Set new coords.
	pMonster->wX = wDestX;
	pMonster->wY = wDestY;

	//Check for Fluff puff dying.
	if (bFluffPoison)
		room.ProcessPuffAttack(CueEvents,wDestX,wDestY);
	//Check for stepping on pressure plate.
	if (room.GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
		room.ActivateOrb(wDestX, wDestY, CueEvents, OAT_PressurePlate);

	//Did monster step on swordsman?
	if (this->pCurrentGame->swordsman.IsInRoom() &&
		 wDestX == this->pCurrentGame->swordsman.wX &&
		 wDestY == this->pCurrentGame->swordsman.wY)
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->SetDyingEntity(&pGame->swordsman, pGiant);
		CueEvents.Add(CID_MonsterKilledPlayer, this);
	}
}
