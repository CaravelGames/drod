// $Id: Clone.h 8496 2008-01-13 01:28:02Z mrimer $

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

//Clone.h
//Declarations for CClone.
//Class for handling clone monster game logic.

#ifndef CLONE_H
#define CLONE_H

#include "PlayerDouble.h"

class CClone : public CPlayerDouble
{
public:
	CClone(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CClone);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual SwordType GetWeaponType() const { return SwordType::ReallyBigSword; }
};
	
#endif //...#ifndef CLONE_H

