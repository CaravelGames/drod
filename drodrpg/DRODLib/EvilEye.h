// $Id: EvilEye.h 8496 2008-01-13 01:28:02Z mrimer $

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

//EvilEye.h
//Declarations for CEvilEye.
//Class for handling evil eye monster game logic.

#ifndef EVILEYE_H
#define EVILEYE_H

class CCurrentGame;

#include "Monster.h"
#include "MonsterFactory.h"

class CEvilEye : public CMonster
{
public:
	CEvilEye(CCurrentGame *pSetCurrentGame = NULL);
	CEvilEye(const UINT type, CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CEvilEye);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual bool TurnToFacePlayerWhenFighting() const {return true;}
};

class CMadEvilEye : public CEvilEye
{
public:
	CMadEvilEye(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CMadEvilEye);
};

#endif //...#ifndef EVILEYE_H

