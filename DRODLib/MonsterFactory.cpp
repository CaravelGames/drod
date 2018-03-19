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
 * 1997, 2000, 2001, 2002 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//MonsterFactory.cpp
//Implementation of CMonsterFactory.

#include "MonsterFactory.h"

//Monster child class includes.
#include "Architect.h"
#include "Aumtlich.h"
#include "BlueSerpent.h"
#include "Brain.h"
#include "Character.h"
#include "Citizen.h"
#include "Clone.h"
#include "Construct.h"
#include "Decoy.h"
#include "EvilEye.h"
#include "Fegundo.h"
#include "FegundoAshes.h"
#include "FluffBaby.h"
#include "Gentryii.h"
#include "Ghost.h"
#include "Goblin.h"
#include "GreenSerpent.h"
#include "Guard.h"
#include "Halph.h"
#include "Mimic.h"
#include "Neather.h"
#include "RedSerpent.h"
#include "Roach.h"
#include "RoachEgg.h"
#include "RoachQueen.h"
#include "RockGiant.h"
#include "Slayer.h"
#include "Spider.h"
#include "Splitter.h"
#include "Stalwart.h"
#include "TarBaby.h"
#include "TarMother.h"
#include "TemporalClone.h"
#include "Waterskipper.h"
#include "WaterskipperNest.h"
#include "WraithWing.h"
#include "Wubba.h"

#include <BackEndLib/Assert.h>

//******************************************************************************************
CMonster* CMonsterFactory::GetNewMonster(
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
		return new CWaterskipper();

		case M_SKIPPERNEST:
		return new CWaterskipperNest();

		case M_BRAIN:
		return new CBrain();

		case M_CHARACTER:
		return new CCharacter();

		case M_CITIZEN:
		return new CCitizen();

		case M_CLONE:
		return new CClone();

		case M_CONSTRUCT:
		return new CConstruct();
		
		case M_DECOY:
		return new CDecoy();
		
		case M_WUBBA:
		return new CWubba();

		case M_EYE:
		return new CEvilEye();

		case M_GELMOTHER:
		return new CGelMother();

		case M_GELBABY:
		return new CGelBaby();

		case M_SEEP:
		return new CGhost();

		case M_GOBLIN:
		return new CGoblin();

		case M_GUARD:
		return new CGuard();

		case M_HALPH:
		return new CHalph();

		case M_HALPH2:
		return new CHalph2();

		case M_MIMIC:
		return new CMimic();

		case M_MUDMOTHER:
		return new CMudMother();

		case M_MUDBABY:
		return new CMudBaby();

		case M_NEATHER:
		return new CNeather();

		case M_FEGUNDO:
		return new CPhoenix();

		case M_FEGUNDOASHES:
		return new CPhoenixAshes();

		case M_ROACH:
		return new CRoach();
		
		case M_REGG:
		return new CRoachEgg();

		case M_ROCKGOLEM:
		return new CRockGolem();

		case M_QROACH:    
		return new CRoachQueen();
		
		case M_SERPENT:
		return new CRedSerpent();

		case M_SERPENTG:
		return new CGreenSerpent();

		case M_SERPENTB:
		return new CBlueSerpent();

		case M_SLAYER:
		return new CSlayer();

		case M_SLAYER2:
		return new CSlayer2();

		case M_SPIDER:
		return new CSpider();

		case M_ROCKGIANT:
		return new CSplitter();

		case M_STALWART:
		return new CStalwart();

		case M_STALWART2:
		return new CStalwart2();

		case M_TARMOTHER:
		return new CTarMother();

		case M_TARBABY:
		return new CTarBaby();

		case M_TEMPORALCLONE:
		return new CTemporalClone();

		case M_WWING:
		return new CWraithWing();

		case M_AUMTLICH:
		return new CAumtlich();

		case M_ARCHITECT:
		return new CArchitect();

		case M_GENTRYII:
		return new CGentryii();

		case M_FLUFFBABY:
		return new CFluffBaby();

		default:
			ASSERT(!"Unexpected monster type.");
			return NULL; 
	}
}
