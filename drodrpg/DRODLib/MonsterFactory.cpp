// $Id: MonsterFactory.cpp 9118 2008-08-05 04:05:24Z mrimer $

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
 * 1997, 2000, 2001, 2002 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//MonsterFactory.cpp
//Implementation of CMonsterFactory.

#include "MonsterFactory.h"

//Monster child class includes.
#include "Ant.h"
#include "AntHill.h"
#include "BlueSerpent.h"
#include "Brain.h"
#include "Character.h"
#include "Citizen.h"
#include "Clone.h"
#include "Construct.h"
#include "Decoy.h"
#include "EvilEye.h"
#include "FluffBaby.h"
#include "Goblin.h"
#include "GreenSerpent.h"
#include "Guard.h"
#include "Halph.h"
#include "Mimic.h"
#include "Phoenix.h"
#include "PhoenixAshes.h"
#include "RedSerpent.h"
#include "Roach.h"
#include "RoachEgg.h"
#include "RoachQueen.h"
#include "RockGiant.h"
#include "Seep.h"
#include "Slayer.h"
#include "Spider.h"
#include "Splitter.h"
#include "Stalwart.h"
#include "Swordsman.h"
#include "TarBaby.h"
#include "TarMother.h"
#include "WraithWing.h"
#include "Wubba.h"
#include "Zombie.h"

#include <BackEndLib/Assert.h>

//******************************************************************************************
CMonster * CMonsterFactory::GetNewMonster(
//Class factory for classes derived from CMonster.  New monster will not be
//associated with room yet and will need its members set before it is used.
//
//Params:
	const MONSTERTYPE eType)      //(in)   One of the M_* monster enumerations.
//
//Returns:
//Pointer to a new instance of class derived from CMonster.
{
	ASSERT(eType < MONSTER_TYPES);

	//Can use a function pointer array later for better speed.

	switch (eType)
	{
		case M_WATERSKIPPER:
		return new CAnt(this->pCurrentGame);

		case M_SKIPPERNEST:
		return new CAntHill(this->pCurrentGame);

		case M_BEETHRO:
		return new CSwordsman(this->pCurrentGame);

		case M_BRAIN:
		return new CBrain(this->pCurrentGame);

		case M_CHARACTER:
		return new CCharacter(this->pCurrentGame);

		case M_CITIZEN:
		return new CCitizen(this->pCurrentGame);

		case M_CLONE:
		return new CClone(this->pCurrentGame);
		
		case M_DECOY:
		return new CDecoy(this->pCurrentGame);
		
		case M_WUBBA:
		return new CWubba(this->pCurrentGame);

		case M_EYE:
		return new CEvilEye(this->pCurrentGame);

		case M_GELMOTHER:
		return new CGelMother(this->pCurrentGame);

		case M_GELBABY:
		return new CGelBaby(this->pCurrentGame);

		case M_GOBLIN:
		return new CGoblin(this->pCurrentGame);

		case M_GOBLINKING:
		return new CGoblinKing(this->pCurrentGame);

		case M_GUARD:
		return new CGuard(this->pCurrentGame);

		case M_HALPH:
		return new CHalph(this->pCurrentGame);

		case M_MADEYE:
		return new CMadEvilEye(this->pCurrentGame);

		case M_MIMIC:
		return new CMimic(this->pCurrentGame);

		case M_MUDMOTHER:
		return new CMudMother(this->pCurrentGame);

		case M_MUDBABY:
		return new CMudBaby(this->pCurrentGame);

		case M_NEATHER:
		return new CNeather(this->pCurrentGame);

		case M_FEGUNDO:
		return new CPhoenix(this->pCurrentGame);

		case M_FEGUNDOASHES:
		return new CPhoenixAshes(this->pCurrentGame);

		case M_ROACH:     
		return new CRoach(this->pCurrentGame);
		
		case M_REGG:      
		return new CRoachEgg(this->pCurrentGame);

		case M_ROCKGOLEM:
		return new CRockGiant(this->pCurrentGame);

		case M_QROACH:    
		return new CRoachQueen(this->pCurrentGame);
		
		case M_SEEP:
		return new CSeep(this->pCurrentGame);

		case M_SERPENT:
		return new CRedSerpent(this->pCurrentGame);

		case M_SERPENTG:
		return new CGreenSerpent(this->pCurrentGame);

		case M_SERPENTB:
		return new CBlueSerpent(this->pCurrentGame);

		case M_SLAYER:
		return new CSlayer(this->pCurrentGame);

		case M_SPIDER:
		return new CSpider(this->pCurrentGame);

		case M_ROCKGIANT:
		return new CSplitter(this->pCurrentGame);

		case M_PIRATE:
		return new CStalwart(this->pCurrentGame);

		case M_TARMOTHER:
		return new CTarMother(this->pCurrentGame);

		case M_TARBABY:
		return new CTarBaby(this->pCurrentGame);

		case M_WWING:
		return new CWraithWing(this->pCurrentGame);

		case M_AUMTLICH:
		return new CZombie(this->pCurrentGame);

		case M_CONSTRUCT:
		return new CConstruct(this->pCurrentGame);

		case M_FLUFFBABY:
		return new CFluffBaby(this->pCurrentGame);

		default:
			ASSERT(!"Unexpected monster type.");
			return NULL; 
	}
}

