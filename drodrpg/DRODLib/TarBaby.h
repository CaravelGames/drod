// $Id: TarBaby.h 9070 2008-07-09 05:24:25Z mrimer $

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

//TarBaby.h
//Declarations for CTarBaby.
//Class for handling tar baby monster game logic.

#ifndef TARBABY_H
#define TARBABY_H

#include "Monster.h"
#include "MonsterFactory.h"

class CTarBaby : public CMonsterFacesTarget
{
public:
	CTarBaby(CCurrentGame *pSetCurrentGame = NULL) : CMonsterFacesTarget(M_TARBABY, pSetCurrentGame) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CTarBaby);

	virtual bool DamagedByHotTiles() const {return false;}
};

class CMudBaby : public CMonsterFacesTarget
{
public:
	CMudBaby(CCurrentGame *pSetCurrentGame = NULL) : CMonsterFacesTarget(M_MUDBABY, pSetCurrentGame) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CMudBaby);

	virtual bool DamagedByHotTiles() const {return false;}
};

class CGelBaby : public CMonsterFacesTarget
{
public:
	CGelBaby(CCurrentGame *pSetCurrentGame = NULL) : CMonsterFacesTarget(M_GELBABY, pSetCurrentGame) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CGelBaby);

	virtual bool DamagedByHotTiles() const {return false;}
};

#endif //...#ifndef TARBABY_H

