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

#ifndef PLAYERDOUBLE_H
#define PLAYERDOUBLE_H

#include "Monster.h"
#include "MonsterFactory.h"
#include "GameConstants.h"

class CArmedMonster : public CMonster
{
public:
	CArmedMonster(const UINT wSetType, CCurrentGame *pSetCurrentGame = NULL,
			const MovementType eMovement = GROUND_AND_SHALLOW_WATER,
			const UINT wSetProcessSequence = SPD_PDOUBLE);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CArmedMonster);

	//define virtual void Process(...) in derived classes
	void           AttackTargetUnderWeapon(const UINT wTargetX, const UINT wTargetY, CCueEvents& CueEvents);
	bool           CanAttackWithWeaponTowards(int dx, int dy) const;
	virtual bool   CanPushObjects() const { return this->GetResolvedIdentity() == M_CONSTRUCT || bIsHuman(this->GetResolvedIdentity()); }
	virtual bool   CheckForDamage(CCueEvents& CueEvents);
	bool           DoesSquareRemoveWeapon(const UINT wCol, const UINT wRow) const;
	virtual bool   DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual WeaponType GetWeaponType() const { return this->weaponType; }
	virtual bool   HasSword() const;
	virtual bool   IsAggressive() const {return false;}
	virtual bool   IsTileObstacle(const UINT wTileNo) const;
	virtual bool   IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const;
	virtual bool  MakeSlowTurn(const UINT wDesiredO);
	bool        MakeSlowTurnIfOpen(const UINT wDesiredO);
	bool        MakeSlowTurnIfOpenTowards(const UINT wTX, const UINT wTY);
	bool        MakeSlowTurnTowards(const UINT wTX, const UINT wTY);
	virtual void   PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents);
	virtual void  Move(const UINT wX, const UINT wY, CCueEvents* pCueEvents = NULL);

	UINT        GetWeaponX() const {return this->wX + nGetOX(this->wO);}
	UINT        GetWeaponY() const {return this->wY + nGetOY(this->wO);}

	virtual bool SetWeaponSheathed();

	UINT wSwordMovement; //which way sword moved this turn
	bool bWeaponSheathed;
	bool bNoWeapon;
	WeaponType weaponType;

	bool     bFrozen; //prevent moves if frozen

protected:
	void DoubleMove(CCueEvents &CueEvents, const int dx, const int dy);
};

class CPlayerDouble : public CArmedMonster
{
public:
	CPlayerDouble(const UINT wSetType, CCurrentGame *pSetCurrentGame = NULL,
			const MovementType eMovement = GROUND_AND_SHALLOW_WATER,
			const UINT wSetProcessSequence = SPD_PDOUBLE)
		: CArmedMonster(wSetType, pSetCurrentGame, eMovement, wSetProcessSequence)
	{ }
	IMPLEMENT_CLONE_REPLICATE(CMonster, CPlayerDouble);

	virtual bool CanDropTrapdoor(const UINT oTile) const;
	virtual bool BrainAffects() const {return false;}
	virtual bool IsAggressive() const {return false;}
	virtual bool IsFriendly() const {return true;}

	virtual void PushInDirection(int dx, int dy, bool bStun, CCueEvents& CueEvents);
};

#endif //...#ifndef PLAYERDOUBLE_H
