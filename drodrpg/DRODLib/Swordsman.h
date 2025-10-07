// $Id: Swordsman.h 9302 2008-10-29 02:36:04Z mrimer $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//CSwordsman.h
//Declarations for CSwordsman.h.
//
//GENERAL
//
//Class for accessing and manipulating player game state.

#ifndef SWORDSMAN_H
#define SWORDSMAN_H

//#include "GameConstants.h"
//#include "MonsterTypes.h"
#include "PlayerDouble.h"
#include "PlayerStats.h"
//#include <BackEndLib/Coord.h>

class CCurrentGame;
class CSwordsman : public CPlayerDouble
{
public:
	CSwordsman(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CSwordsman);

	virtual UINT GetIdentity() const;
//	virtual bool BrainAffects() const {return false;}
	virtual bool IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const;
	virtual bool IsSwordDisabled() const;
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

	UINT CalcDamage(int damageVal) const;
	bool CanDropTrapdoor(UINT wTileNo) const;
	bool CanJump(const int dx, const int dy) const;
	bool CanWalkOnWater() const;
	void Clear(const bool bNewGame);
	UINT Damage(CCueEvents& CueEvents, int damageVal, CUEEVENT_ID deathCID);
	void DecHealth(CCueEvents& CueEvents, const UINT delta, CUEEVENT_ID deathCID);
	void EndHaste() {this->bHasted = false;}
	int  getGoldMultiplier() const;
	int  getXPMultiplier() const;
	virtual bool IsFlying() const;
	bool IsHasted() const {return this->bHasted;}
	bool IsInRoom() const;
	bool IsInvisible() const {return this->bInvisible;}
	bool IsWeaponAt(UINT wX, UINT wY) const;
	bool Move(const UINT wSetX, const UINT wSetY);
	void ResetRoomStats();
	void RotateClockwise();
	void RotateCounterClockwise();
	void SetOrientation(const UINT wO);
	static UINT GetSwordMovement(const int nCommand, const UINT wO);

	//Sword position
	virtual bool HasSword() const;
	bool     bSwordOff;      //weapon disabled indefinitely
	bool     bShieldOff, bAccessoryOff, bCommandOff; //disabled indefinitely if set
/*
	UINT     wSwordMovement;

	//Double placing
	UINT     wPlacingDoubleType;
	UINT     wDoubleCursorX;
	UINT     wDoubleCursorY;

	//Misc. effects
	bool     bIsDying;   //whether player was killed
	bool     bFrozen; //prevent moves if frozen
	bool     bCanGetItems; //whether non-Beethro role can pick up items
*/
	bool     bHasTeleported;
	UINT     wAppearance, wIdentity; //visual appearance, actual logical role enumeration

	//RPG stats
	PlayerStats st;

	//Accessory modifiers
	bool     bHasted;   //whether player's speed is doubled
	bool     bInvisible;  //whether player is invisible

	//Misc.
	bool bIntraRoomPath; //when pathfinding within a room
	bool bPathToStairs;  //when pathfinding onto stairs
};

#endif   //...#ifndef SWORDSMAN_H
