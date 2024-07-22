// $Id: MonsterFactory.h 9294 2008-10-29 02:29:15Z mrimer $

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

//MonsterFactory.h
//Declarations for CMonsterFactory.
//Class for creating monster objects derived from CMonster.

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#ifndef MONSTERFACTORY_H
#define MONSTERFACTORY_H

#include <BackEndLib/Types.h>

//How to add new NPCs.
//BackEnd:
//1. Add enumeration after CHARACTER_FIRST below.
//2. Add to SPEAKER enumeration and getSpeakerType().
//3a. Add to bEntityHasSword if NPC has a sword.
//3b. Add to export scripts, if needed.
//FrontEnd:
//4. To scripting dialog (speaker and graphic list boxes).
//5. Add to CharacterTileImageArray()
//6. Add to SpeakerConstant.
//7. Add subtitle color to SpeakerColor.
#include "MonsterTypes.h"

//for automatic (convenient) inclusion into all monster types
#include "CueEvents.h"
#include "CurrentGame.h"
#include "DbRooms.h"	// circular dependency, needs MONSTERTYPE
#include "GameConstants.h"
#include "TileConstants.h"

#define MONSTER_PIECE_OFFSET (100) //for keeping track of special monster piece tiles

static inline bool IsValidMonsterType(const UINT mt) {return (mt<MONSTER_TYPES);}

static inline bool bIsMother(const UINT mt) {
	switch (mt)
	{
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER: return true;
		default: return false;
	}
}

static inline bool bIsBeethroDouble(const UINT mt) {
	switch (mt)
	{
//		case M_MIMIC: case M_CLONE: case M_DECOY: //not semantically similar in RPG
		case M_BEETHRO:
			return true;
		//Don't include M_BEETHRO_IN_DISGUISE because that type is functionally
		//different from Beethro.
		default:
			return false;
	}
}

//Any sworded monster type.
static inline bool bEntityHasSword(const UINT mt) {
	switch (mt)
	{
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_SLAYER:	case M_GUARD: case M_PIRATE: case M_STALWART:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
			return true;
		default:
			return false;
	}
}

//All human types.
static inline bool bIsHuman(const UINT mt) {
	switch (mt)
	{
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_SLAYER:	case M_GUARD: case M_PIRATE: case M_STALWART:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
		case M_HALPH: case M_NEATHER:
		case M_CITIZEN: case M_CITIZEN1: case M_CITIZEN2: case M_CITIZEN3: case M_CITIZEN4:
		case M_INSTRUCTOR: case M_NEGOTIATOR:
		case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
		case M_ARCHIVIST: case M_ARCHITECT: case M_PATRON:
			return true;
		default:
			return false;
	}
}

static inline bool bIsEntityFlying(const UINT mt) {
	switch (mt)
	{
	case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
			return true;
		default:
			return false;
	}
}

static inline bool bIsGoblin(const UINT mt) {return mt == M_GOBLIN || mt == M_GOBLINKING;}

static inline bool bIsSerpent(const UINT mt) {return mt==M_SERPENT || mt==M_SERPENTG || mt==M_SERPENTB;}

static inline bool bIsLargeMonster(const UINT mt) { return bIsSerpent(mt) || mt == M_ROCKGIANT; }

static inline bool bMonsterHasDirection(const UINT mt) {
	switch (mt)
	{
		case M_BRAIN:
		case M_FEGUNDOASHES:
		case M_REGG:
		case M_SKIPPERNEST:
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
		case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
		case M_WUBBA: case M_FLUFFBABY:
			return false;
		default:
			return true;
	}
}

class CMonster;
class CMonsterFactory
{
public:
	CMonsterFactory(CCurrentGame *pSetCurrentGame = NULL) : pCurrentGame(pSetCurrentGame) {}
	~CMonsterFactory() {}
	CMonster *GetNewMonster(const MONSTERTYPE eType);

	CCurrentGame *pCurrentGame;
};

#endif //...#ifndef MONSTERFACTORY_H

