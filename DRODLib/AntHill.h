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

//AntHill.h
//Declarations for CAntHill.
//Class for handling Ant Hill monster game logic.

#ifndef ANTHILL_H
#define ANTHILL_H

class CCurrentGame;

#include "Monster.h"
#include "MonsterFactory.h"

class CAntHill : public CMonster
{
public:
	CAntHill(CCurrentGame *pSetCurrentGame = NULL) : 
			CMonster(M_SKIPPERNEST, pSetCurrentGame, WATER) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CAntHill);

	virtual bool HasOrientation() const {return false;}
	virtual bool IsAggressive() const {return false;}
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};

#endif //...#ifndef ANTHILL_H
