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

//Slayer.h
//Declarations for CSlayer.
//Class for handling Slayer's game logic.

#ifndef SLAYER_H
#define SLAYER_H

#include "PlayerDouble.h"
#include "MonsterPiece.h"

typedef MonsterPieces::iterator WISP;
typedef MonsterPieces::const_iterator const_WISP;

class CSlayer : public CArmedMonster
{
public:
	CSlayer(CCurrentGame *pSetCurrentGame = NULL, const UINT type=M_SLAYER);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CSlayer);
	
	virtual bool BrainAffects() const {return false;}
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	virtual bool IsOpenMove(const int dx, const int dy) const;
	virtual bool IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const;
	virtual bool IsSafeToStab(const UINT wFromX, const UINT wFromY, const UINT wSO) const;
	virtual bool OnStabbed(CCueEvents &CueEvents, const UINT wX=(UINT)-1, const UINT wY=(UINT)-1,
			WeaponType weaponType=WT_Sword);
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

private:
   virtual bool IsAggressive() const {return true;}

	void AdvanceAlongWisp(CCueEvents &CueEvents);
	bool CheckWispIntegrity();
	bool ExtendWisp(CCueEvents &CueEvents, const bool bMoveAllowed=true);
	WISP GetWispAt(const UINT wX, const UINT wY);
	const_WISP GetWispAt(const UINT wX, const UINT wY) const {
		return const_cast<CSlayer*>(this)->GetWispAt(wX, wY);
	}
	void SeverWispAt(WISP piece, const bool bKeepBack=false);
	
	void MakeDefensiveMove(CCueEvents &CueEvents, const bool bCanRetreat=true);

	//for opening doors
	void GetAdjDest(UINT& wDestX, UINT& wDestY) const;
	void NeedToOpenDoor(const UINT wDoorX, const UINT wDoorY);
	void MoveToOpenDoor(CCueEvents &CueEvents);
	void StopOpeningDoor();
	void StopStrikingOrb(CCueEvents &CueEvents);
	CCoord openingDoorAt; //door being opened
	CCoordSet orbsToHit, platesToDepress;  //set of possible destination orbs/plates

	enum SlayerState
	{
		Seeking,
		OpeningDoor
	};
	SlayerState state;
	bool        bMovingWisp;
	bool        bMakingDefensiveMove;	//for logic bookkeeping
	UINT        wTX, wTY, wTO; //target this turn
	UINT        wTSX, wTSY; //target's sword this turn
	UINT        wDistToTarget; //distance to target this turn

	//For cue events
	UINT wTurnCombatBegan;
};

class CSlayer2 : public CSlayer
{
public:
	CSlayer2(CCurrentGame *pSetCurrentGame = NULL, const UINT type=M_SLAYER2);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CSlayer2);
};

#endif //...#ifndef SLAYER_H
