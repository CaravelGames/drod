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

//Phoenix.cpp
//Implementation of CFegundo.

#include "Fegundo.h"

#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CFegundo::CFegundo(CCurrentGame *pSetCurrentGame)
	: CMonster(M_FEGUNDO, pSetCurrentGame, AIR, SPD_FEGUNDO) //move after mimics, before stalwarts
{}

//*****************************************************************************
bool CFegundo::CanFindSwordsman()
//Overridable method for determining if a monster can find the player on its own.
//Currently used by movement routines to see if a fegundo is controllable
//by the player or not.
//
//Returns:
//True if monster can find the swordsman, false if not.
const
{
	ASSERT(this->pCurrentGame);
	UINT wSX, wSY;
	//Player is already known to be in the room from Process(),
	//so don't test again and ignore NPC Beethros
	this->pCurrentGame->GetSwordsman(wSX, wSY, true);

	//If player is visible, monster can see him.
	if (this->pCurrentGame->swordsman.IsVisible())
		return true;

	//Otherwise, monster can smell him if within range.
	if (CanSmellObjectAt(wSX, wSY))
		return true;

	//decoys/monster enemies will not give fegundo control to an invisible player

	return false;	//player not found
}

//*****************************************************************************
bool CFegundo::DoesSquareContainObstacle(
//Override for fegundo -- they can't step on attackable monsters or the player.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.  Must be valid.
const
{
	//Most of the checks done in base method.
	if (CMonster::DoesSquareContainObstacle(wCol, wRow))
		return true;

	//Can't move onto monsters except Fluff Babies -- even target ones.
	CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
		return true;

	//Can't step on the player.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//*****************************************************************************************
void CFegundo::Process(
//Process a Fegundo for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;

	//Don't move if player is not in the room.
	if (!this->pCurrentGame->swordsman.IsInRoom())
		return;

	//Don't move until player activates a power/control token.
	if (!this->pCurrentGame->swordsman.bCanGetItems)
		return;

	//Always move in the direction the player faces.
	const UINT wUsingO = this->pCurrentGame->swordsman.wO;
	int dx = nGetOX(wUsingO), dy = nGetOY(wUsingO);
	UINT wX = this->wX + dx;
	UINT wY = this->wY + dy;

	//Get movement offsets.
	int dxFirst, dyFirst;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy, DirectOnly))
		return;

	SetOrientation(dxFirst, dyFirst);

	//If can't move and something solid is in the way, explode.
	if (!dx && !dy && !DoesArrowPreventMovement(this->wX, this->wY, dxFirst, dyFirst) &&
			!this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dxFirst, dyFirst))
	{
		CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(
				this->wX + dxFirst, this->wY + dyFirst);
		if (!pMonster || pMonster->wType != M_FEGUNDO) //don't explode when another phoenix is in the way
			Explode(CueEvents);
		return;
	}

	MakeStandardMove(CueEvents, dx, dy);

	//Moving onto a checkpoint activates it.
	this->pCurrentGame->QueryCheckpoint(CueEvents, this->wX, this->wY);
}

//*****************************************************************************************
void CFegundo::Explode(CCueEvents &CueEvents)
//Explodes this Fegundo.
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	UINT wTileNo = room.GetFSquare(this->wX,this->wY);

	//The Fegundo burns to ashes and is re-animated 5 turns later.
	//It generates a 3x3 explosion.
	const UINT EXPLOSION_RADIUS = 1;
	CCoordSet explosion; //what tiles are affected by the explosion
	CCoordStack bombs, powder_kegs, coords;
	CCoordIndex caberCoords;
	room.GetCaberCoords(caberCoords);
	for (int y = -1; y <= 1; ++y)
		for (int x = -1; x <= 1; ++x)
		{
			const UINT wTileX = this->wX + x;
			const UINT wTileY = this->wY + y;
			
			if (!room.IsValidColRow(wTileX, wTileY))
				continue;
			if (x || y)
			{
				//Make sure Force Arrows are obeyed.
				if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo,nGetO(x,y)))
					continue;
				//Don't hit tiles under cabers
				if (caberCoords.Exists(wTileX,wTileY))
					continue;
			}
			room.ExpandExplosion(CueEvents, coords, this->wX, this->wY,
					wTileX, wTileY, bombs, powder_kegs, explosion, caberCoords, EXPLOSION_RADIUS);
		}

	//Now process the effects of the explosion.
	for (CCoordSet::const_iterator exp=explosion.begin(); exp!=explosion.end(); ++exp) {
		const bool kills_fegundo = (exp->wX != this->wX) || (exp->wY != this->wY); //explosion does not hurt self, so don't make one here
		room.ProcessExplosionSquare(CueEvents, exp->wX, exp->wY, kills_fegundo);
	}

	CueEvents.Add(CID_FegundoToAsh, this); //caller must replace this with FegundoAshes monster

	//If bombs were set off, explode them now.
	room.DoExplode(CueEvents, bombs, powder_kegs);

	room.ConvertUnstableTar(CueEvents);
}
