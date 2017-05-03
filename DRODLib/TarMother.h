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

//TarMother.h
//Declarations for CTarMother.
//Class for handling tar mother monster game logic.

#ifndef TARMOTHER_H
#define TARMOTHER_H

#include "Monster.h"
#include "MonsterFactory.h"
#include "CueEvents.h"

//******************************************************************************************
class CMother : public CMonster
{
public:
	CMother(const UINT wType, CCurrentGame *pSetCurrentGame = NULL)
		: CMonster(wType, pSetCurrentGame)
		, bEyeSet(false)
	{ }

	virtual bool CheckForDamage(CCueEvents& /*CueEvents*/) {return false;}
	void Process(CCueEvents &CueEvents, const CUEEVENT_ID eGrowthEvent);

	virtual bool HasOrientation() const {return false;}
	virtual bool IsAggressive() const {return false;}
	virtual void   ReflectX(CDbRoom *pRoom);
	virtual void   ReflectY(CDbRoom *pRoom);

protected:
	bool bEyeSet, bLeftEye;
	UINT wMinX, wMaxX, wMinY, wMaxY;
};

//******************************************************************************************
class CTarMother : public CMother
{
public:
	CTarMother(CCurrentGame *pSetCurrentGame = NULL)
		: CMother(M_TARMOTHER, pSetCurrentGame)
	{ }
	IMPLEMENT_CLONE_REPLICATE(CMonster, CTarMother);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};

class CMudMother : public CMother
{
public:
	CMudMother(CCurrentGame *pSetCurrentGame = NULL)
		: CMother(M_MUDMOTHER, pSetCurrentGame)
	{ }
	IMPLEMENT_CLONE_REPLICATE(CMonster, CMudMother);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};

class CGelMother : public CMother
{
public:
	CGelMother(CCurrentGame *pSetCurrentGame = NULL)
		: CMother(M_GELMOTHER, pSetCurrentGame)
	{ }
	IMPLEMENT_CLONE_REPLICATE(CMonster, CGelMother);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};

#endif //...#ifndef TARMOTHER_H
