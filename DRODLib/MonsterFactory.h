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
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//MonsterFactory.h
//Declarations for CMonsterFactory.
//Class for creating monster objects derived from CMonster.

#ifndef MONSTERFACTORY_H
#define MONSTERFACTORY_H

#include "MonsterType.h"

#include <BackEndLib/Types.h>

//How to add new NPCs.
//DRODLib (back end):
//1. Add enumeration after CHARACTER_FIRST in MonsterType.h.
//2. Add to SPEAKER enumeration and getSpeakerType().
//3a. Add to bEntityHasSword if NPC has a sword.
//3b. Add to export scripts, if needed.
//DROD (front end):
//4. To scripting dialog (speaker and graphic list boxes).
//5. Add to CharacterTileImageArray()
//6. Add to SpeakerConstant.
//7. Add subtitle color to SpeakerColor.

class CMonster;
class CMonsterFactory
{
public:
	static CMonster* GetNewMonster(const MONSTERTYPE eType);

private:
	CMonsterFactory(); //not implemented
};

#endif //...#ifndef MONSTERFACTORY_H
