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
 * 1997, 2000, 2001, 2002, 2005, 2012 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CONSTRUCT_H
#define CONSTRUCT_H

#include "Monster.h"
#include "MonsterFactory.h"
#include "RockGiant.h"

class CConstruct : public CRockGolem
{
public:
	CConstruct(CCurrentGame *pSetCurrentGame = NULL)
		: CRockGolem(pSetCurrentGame, M_CONSTRUCT)
		, wTurnDisabled(0), bOremiteDamage(false)
	{ }
	IMPLEMENT_CLONE_REPLICATE(CMonster, CConstruct);
	
	virtual bool CanDropTrapdoor(const UINT oTile) const;
	virtual bool CanPushObjects() const;
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	bool         KillIfOnOremites(CCueEvents &CueEvents);
	void         ResetOremiteDamage() {this->bOremiteDamage = false;}
	virtual bool OnStabbed(CCueEvents &CueEvents, const UINT wX=(UINT)-1, const UINT wY=(UINT)-1, WeaponType weaponType=WT_Sword);
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual bool TakesTurn() const { return true; } //even when disabled

private:
	UINT wTurnDisabled;
	bool bOremiteDamage;
};

#endif //...#ifndef CONSTRUCT_H