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
 * 1997, 2000, 2001, 2002, 2005, 2012 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef GENTRYII_H
#define GENTRYII_H

#include "Monster.h"
#include "MonsterPiece.h"

#include <map>
#include <vector>

class CGentryii : public CMonster
{
public:
	CGentryii(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CGentryii);

	bool CanPushHeadOntoOTileAt(const UINT destX, const UINT destY);

	virtual bool IsLongMonster() const {return true;}
	virtual bool IsOpenMove(const int dx, const int dy) const;
	virtual bool OnStabbed(CCueEvents &/*CueEvents*/, const UINT /*wX*/=(UINT)-1, const UINT /*wY*/=(UINT)-1,
			WeaponType /*weaponType*/=WT_Sword)
		{ return false; }

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

	virtual void PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents);

private:
	bool CanPullChain(const UINT destX, const UINT destY,
		const CMonster* prevPiece, MonsterPieces::const_iterator pieceIt,
		const UINT depth) const;
	bool GetClosestOpenTileTo(
		const CMonster* prevPiece,
		const CMonsterPiece& piece,
		const CMonsterPiece* nextPiece,
		UINT& destX, UINT& destY) const;

	bool DoesSquareContainChainObstacle(const UINT wX, const UINT wY) const;
	bool IsChainObstacle(const UINT wX, const UINT wY, const int dx, const int dy,
			const CMonster* prevPiece, const UINT destX, const UINT destY) const;
	bool IsPinnedAt(const UINT wX, const UINT wY) const;

	void MoveGentryii(UINT destX, UINT destY, CCueEvents& CueEvents);
	void PullChainTaut(CCueEvents& CueEvents);
	void SetHeadOrientation(int dx, int dy);

	//Set during const move checks
	typedef std::vector<std::pair<UINT,UINT> > PIECE_MOVES;
	mutable PIECE_MOVES piece_moves;
	mutable bool bPinPreventsMove, bCheckPin;
	mutable UINT wPinDepth, wGoalX, wGoalY; //tile that head is attempting to move into, acting as a chain obstacle
	int tdx, tdy; //direction target is in -- used to set bCheckPin
	
	typedef std::multimap<std::pair<UINT,UINT>, std::pair<UINT,UINT> > CHECKED_CHAIN_MOVES;
	mutable CHECKED_CHAIN_MOVES checked_moves; //speed optimization
	mutable CCoordSet pending_piece_dests;
};

#endif //...#ifndef GENTRYII_H
