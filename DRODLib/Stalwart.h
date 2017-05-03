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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Stalwart.h
//Declarations for CStalwart.
//Class for handling Stalwart monster game logic.

#ifndef STARWART_H
#define STARWART_H

#include "PlayerDouble.h"
#include "MonsterFactory.h"
#include <BackEndLib/IDSet.h>

class CStalwart : public CPlayerDouble
{
public:
	CStalwart(CCurrentGame *pSetCurrentGame = NULL,
		const MovementType eMovement = GROUND, const UINT type=M_STALWART);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CStalwart);

	virtual bool CanDropTrapdoor(const UINT /*oTile*/) const {return false;}
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool IsAttackableTarget() const {return true;}
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	virtual bool IsOpenMove(const int dx, const int dy) const;
	virtual bool IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const;
	virtual bool IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const;
	virtual bool IsTarget() const { return true; }
	bool GetGoal(UINT& wX, UINT& wY); 
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	void ProcessDaggerMove(const int dx, const int dy, CCueEvents& CueEvents);

protected:
	static CIDSet typesToAttack;
};

class CStalwart2 : public CStalwart
{
public:
	CStalwart2(CCurrentGame *pSetCurrentGame = NULL, const UINT type=M_STALWART2);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CStalwart2);
};

#endif //...#ifndef STARWART_H
