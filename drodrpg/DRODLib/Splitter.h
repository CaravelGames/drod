// $Id: Splitter.h 9301 2008-10-29 02:34:31Z mrimer $

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

//Splitter.h
//Declarations for CSplitter.
//Class for handling splitter monster game logic.

#ifndef SPLITTER_H
#define SPLITTER_H

#include "Monster.h"
#include "MonsterFactory.h"

class CSplitter : public CMonster
{
public:
	CSplitter(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CSplitter);

	virtual bool CheckForDamage(CCueEvents& CueEvents);

	virtual bool IsLongMonster() const {return true;}
	virtual bool IsOnMistTile() const override;
	virtual bool IsOpenMove(const int dx, const int dy) const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

	virtual void ReflectX(CDbRoom *pRoom);
	virtual void ReflectY(CDbRoom *pRoom);
	virtual void RotateClockwise(CDbRoom *pRoom);

private:
	bool DamagePiece(CMonster* pPiece, CCueEvents &CueEvents, int damageVal);
	void MovePiece(CMonster *pMonster, const int dx, const int dy,
			const UINT wSX, const UINT wSY, CCueEvents &CueEvents);

	UINT wOrigX, wOrigY; //to facilitate movement calculation
};

#endif //...#ifndef SPLITTER_H

