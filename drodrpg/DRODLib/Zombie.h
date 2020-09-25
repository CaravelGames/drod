// $Id: Zombie.h 8656 2008-02-23 17:43:14Z mrimer $

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
	{}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CZombie);

	virtual bool  HasRayGun() const {return true;}
};

#endif //...#ifndef ZOMBIE_H

