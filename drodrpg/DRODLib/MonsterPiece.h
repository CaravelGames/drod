// $Id: MonsterPiece.h 8102 2007-08-15 14:55:40Z trick $

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

#ifndef MONSTERPIECE_H
#define MONSTERPIECE_H

#include "Monster.h"

//To keep tile groups organized for monsters taking up more than 1 square.
class CMonsterPiece : public CMonster
{
public:
	CMonsterPiece(CMonster* pMonster);
	CMonsterPiece(CMonster* pMonster, const UINT wTileNo,
			const UINT wX, const UINT wY);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CMonsterPiece);   //not used

	virtual bool   IsPiece() const {return true;}
	virtual bool   OnStabbed(CCueEvents &CueEvents, const UINT wX=-1, const UINT wY=-1);

	UINT wTileNo;
	CMonster *const pMonster;  //the monster this is a piece of
};

#endif //...#ifndef MONSTERPIECE_H

