// $Id: Mimic.h 8651 2008-02-23 17:30:18Z mrimer $

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

//Mimic.h
//Declarations for CMimic.
//Class for handling mimic monster game logic.

#ifndef MIMIC_H
#define MIMIC_H

#include "PlayerDouble.h"

class CMimic : public CPlayerDouble
{
public:
	CMimic(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CMimic);

	virtual bool CanDropTrapdoor(const UINT wTileNo);
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual SwordType GetWeaponType() const { return SwordType::LongSword; }
};
	
#endif //...#ifndef MIMIC_H

