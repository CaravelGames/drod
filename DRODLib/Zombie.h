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

//Zombie.h
//Declarations for CZombie.
//Class for handling zombie monster game logic.

#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "Monster.h"
#include "MonsterFactory.h"

class CZombie : public CMonster
{
public:
	CZombie(CCurrentGame *pSetCurrentGame = NULL)
		: CMonster(M_AUMTLICH, pSetCurrentGame)
		, wTX(-1000), wTY(-1000)
		, bFrozen(false) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CZombie);
	
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

	static  bool GetNextGaze(CCueEvents &CueEvents, CZombie *pZombie, CDbRoom *pRoom,
			CCoordIndex &SwordCoords, UINT& cx, UINT& cy, int& dx, int& dy,
			UINT wTX, UINT wTY, const bool bFullTurn=true);
	void UpdateGaze(CCueEvents &CueEvents, CCoordIndex &SwordCoords, const bool bFullTurn);

	UINT wTX, wTY; //target this turn
	bool bFrozen;  //zombie can freeze itself with gaze

protected:
	enum DeflectType {
		GazeBlocked,
		GazeReturned,
		GazePassesThrough
	};
	static DeflectType DeflectAngle(const UINT wInAngle, const UINT wDeflectSurface, const UINT weaponType);
	ROOMCOORD lastGazedTile;
};

#endif //...#ifndef ZOMBIE_H
