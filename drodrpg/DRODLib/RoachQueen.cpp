// $Id: RoachQueen.cpp 9299 2008-10-29 02:32:54Z mrimer $

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
 * Michael Welsh Duggan (md5i)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//RoachQueen.cpp
//Implementation of CRoachQueen.

#include "RoachQueen.h"
#include "Combat.h"
#include "Character.h"

//
//Public methods.
//

//*****************************************************************************************
//Returns: whether an egg should be spawned this turn
bool CRoachQueen::IsSpawnEggTriggered(const CCueEvents& CueEvents) const
{
	//lay an egg any time player fights a monster...
	if (!CueEvents.HasOccurred(CID_MonsterEngaged))
		return false;

	if (this->pCurrentGame->pCombat == NULL)    //...fighting someone...
		return false;

	if (this->pCurrentGame->IsFighting(this)) //...but not me...
		return false;

	const CMonster* pCombatEnemy = this->pCurrentGame->pCombat->pMonster;
	if (!this->pCurrentGame->pCombat->PlayerCanHarmMonster(pCombatEnemy)) //doesn't count if player engages a monster that is too shielded
		return false;

	//don't lay more eggs when eggs are killed
	const UINT spawnID = this->pCurrentGame->getSpawnID(M_REGG);

	const UINT enemyType = pCombatEnemy->wType;
	if (enemyType != M_CHARACTER)
		return enemyType != spawnID;

	const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pCombatEnemy);
	return pCharacter->wLogicalIdentity != spawnID;
}

//*****************************************************************************************
void CRoachQueen::Process(
//Process a roach queen for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) A cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Shall queen lay an egg?
	if (IsSpawnEggTriggered(CueEvents))
	{
		UINT wSX, wSY;
		if (!GetTarget(wSX,wSY))
			return;	//no change -- and don't lay eggs

		CCoordSet eggs;
		CDbRoom& room = *(this->pCurrentGame->pRoom);
		float fClosest = 99999.0;
		for (int y = -1; y <= 1; ++y)
		{
			for (int x = -1; x <= 1; ++x)
			{
				//The criteria for laying an egg in a square should be:
				//1. Square does not contain a monster (including mimic).
				//2. Square does not contain player or a sword.
				//3. T-square is mostly empty (backwards compatibility).
				//4. F-tile is open.
				//5. O-square is open floor/door (except for open yellow doors -- for backwards compatibility).
				//6. Swordsman is still sensed at the new square moved to (backwards compatibility).
				const UINT ex = this->wX + x;
				const UINT ey = this->wY + y;
				if (!room.IsValidColRow(ex, ey))
					continue;

				const UINT wOSquare = room.GetOSquare(ex, ey);
				const UINT wTSquare = room.GetTSquare(ex, ey);
				CMonster* pMonster = room.GetMonsterAtSquare(ex, ey);
				if (
					// Not current queen position
					!(ex == this->wX && ey == this->wY) &&
					// Not the player
					!this->pCurrentGame->IsPlayerAt(ex, ey) &&
					// Not monster or player double or sword
					!pMonster && !DoesSquareContainObstacle(ex, ey) &&
					//And t-square is not occupied with a blocking item.
					(wTSquare == T_EMPTY || wTSquare == T_FUSE) &&
					!bIsArrow(room.GetFSquare(ex, ey)) &&
					//And o-square is floor or open door.
					((bIsPlainFloor(wOSquare) || wOSquare == T_PRESSPLATE) ||
							bIsOpenDoor(wOSquare) ||
							bIsPlatform(wOSquare) || bIsBridge(wOSquare) ||
							wOSquare == T_GOO)
					)
				{
					//Spot is open for placing an egg.
					const float fDist = DistanceToTarget(ex, ey, wSX, wSY);
					if (fDist < fClosest)
					{
						//Place one egg on the tile closest to target.
						fClosest = fDist;
						eggs.clear();
						eggs.insert(ex,ey);
					}
				}
			}
		}

		//Lay eggs and check for them being laid on pressure plates.
		const UINT spawnID = this->pCurrentGame->getSpawnID(M_REGG);
		for (CCoordSet::const_iterator egg=eggs.begin(); egg!=eggs.end(); ++egg)
		{
			CMonster* m = const_cast<CCurrentGame*>(this->pCurrentGame)->AddNewEntity(
					CueEvents, spawnID, egg->wX, egg->wY, S, true);
		}
	}
}
