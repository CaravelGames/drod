// $Id: RoachQueen.h 8102 2007-08-15 14:55:40Z trick $

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

//RoachQueen.h
//Declarations for CRoachQueen.
//Class for handling roach queen monster game logic.

#ifndef ROACHQUEEN_H
#define ROACHQUEEN_H

#include "Monster.h"
#include "MonsterFactory.h"

class CRoachQueen : public CMonster
{
public:
	CRoachQueen(CCurrentGame *pSetCurrentGame = NULL) : CMonster(M_QROACH, pSetCurrentGame) {}
	virtual bool IsEggSpawner() const;
	IMPLEMENT_CLONE_REPLICATE(CMonster, CRoachQueen);

/*	virtual bool IsAggressive() const {return false;}
	virtual bool GetBrainDirectedMovement(int &dxFirst, int &dyFirst,
			int &dx, int &dy, const MovementIQ movementIQ=SmartDiagonalOnly) const;
*/
};

#endif //...#ifndef ROACHQUEEN_H

