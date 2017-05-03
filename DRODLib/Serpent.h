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
 * Michael Welsh Duggan (md5i)
 *
 * ***** END LICENSE BLOCK ***** */

//Serpent.h
//Declarations for CSerpent.
//Class for handling serpent monster game logic.

//FUTURE CONSIDERATIONS
//
//We may want an improved serpent class, i.e.
//CSerpent2, that has the following changes:
//- Can move over scrolls.
//- Can move over arrows.
//- Should use a pathfinding routine that takes into
//  account no diagonal movement.

#ifndef SERPENT_H
#define SERPENT_H

#include "Monster.h"
#include "MonsterFactory.h"

class CSerpent : public CMonster
{
public:
	CSerpent(const MONSTERTYPE eSerpentType, CCurrentGame *pSetCurrentGame = NULL);

	void FindTail(CDbRoom *pRoom);  //1.6 compatibility
	void GetTail(UINT &wTailX, UINT &wTailY);
	UINT GetLength() const;

	//define virtual void Process(...) in derived classes
	virtual bool HasOrientation() const {return false;}
	virtual bool IsLongMonster() const {return true;}
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool GetBrainDirectedMovement(int &dxFirst, int &dyFirst,
			int &dx, int &dy, const MovementIQ movementIQ=SmartDiagonalOnly) const;
	virtual bool OnStabbed(CCueEvents &/*CueEvents*/, const UINT /*wX*/=(UINT)-1, const UINT /*wY*/=(UINT)-1,
		WeaponType /*weaponType*/=WT_Sword)
		{return false;} //Stabs don't kill serpent.

	virtual void   ReflectX(CDbRoom *pRoom);
	virtual void   ReflectY(CDbRoom *pRoom);
	virtual void   ReflectPieceX(CDbRoom *pRoom, CMonsterPiece *pPiece);
	virtual void   ReflectPieceY(CDbRoom *pRoom, CMonsterPiece *pPiece);

	void OrderPieces();
	bool ShortenTail(CCueEvents &CueEvents);

	UINT tailX, tailY, tailO;
	UINT wOldTailX, wOldTailY;

protected:
	bool GetSerpentMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	bool LengthenHead(const int dx, const int dy, const int oX, const int oY, CCueEvents &CueEvents);

	void GetNormalMovement(const UINT wX, const UINT wY, int &dx, int &dy) const;
	bool CanMoveTo(const int x, const int y) const;
};

#endif //...#ifndef SERPENT_H
