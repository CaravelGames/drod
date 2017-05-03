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

#include "MonsterPiece.h"
#include "TileConstants.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*****************************************************************************************
CMonsterPiece::CMonsterPiece(CMonster* pMonster)
//This constructor is only used for importing.  Unset values must be filled in.
	: CMonster(pMonster->wType)
	, wTileNo(static_cast<UINT>(-1))
	, pMonster(pMonster)
{
	ASSERT(pMonster);
	this->wX = this->wY = static_cast<UINT>(-1);
}

CMonsterPiece::CMonsterPiece(CMonster* pMonster, const UINT wTileNo,
		const UINT wX, const UINT wY)
	: CMonster(pMonster->wType)
	, wTileNo(wTileNo)
	, pMonster(pMonster)
{
	ASSERT(pMonster);
	ASSERT(wTileNo < TOTAL_TILE_COUNT);
	this->wX = wX;
	this->wY = wY;
}

//*****************************************************************************************
bool CMonsterPiece::OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/, const UINT /*wY*/, WeaponType weaponType)
{
	return this->pMonster->OnStabbed(CueEvents, this->wX, this->wY, weaponType);
}
