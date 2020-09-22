// $Id: PlayerDouble.h 8566 2008-01-31 16:14:12Z mrimer $

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

class CPlayerDouble : public CMonster
{
public:
	CPlayerDouble(const UINT wSetType, CCurrentGame *pSetCurrentGame = NULL,
			const UINT wSetProcessSequence = 100);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CPlayerDouble);

	//define virtual void Process(...) in derived classes
	virtual bool   DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	bool           DoesSquareRemoveSword(const UINT wCol, const UINT wRow) const;
	virtual bool   HasSword() const;
	virtual bool   IsSafeToStab(const UINT wX, const UINT wY) const;
	virtual bool   IsSwordDisabled() const;
	bool        MakeSlowTurnIfOpen(const UINT wDesiredO);
	bool        MakeSlowTurnIfOpenTowards(const UINT wTX, const UINT wTY);
	bool        MakeSlowTurnTowards(const UINT wTX, const UINT wTY);

	UINT        GetSwordX() const {return this->wX + nGetOX(this->wO);}
	UINT        GetSwordY() const {return this->wY + nGetOY(this->wO);}

	virtual bool SetSwordSheathed();

	UINT wSwordMovement; //which way sword moved this turn
	bool bSwordSheathed;
	bool bNoSword;       //for current room only

//	bool     bFrozen; //prevent moves if frozen
};
	
#endif //...#ifndef PLAYERDOUBLE_H

