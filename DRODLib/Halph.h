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
 * 1997, 2000, 2001, 2002, 2004, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Halph.h
//Declarations for CHalph.
//Class for handling Halph's game logic.

#ifndef HALPH_H
#define HALPH_H

#include "Monster.h"
#include "MonsterFactory.h"
#include <BackEndLib/CoordSet.h>

class CHalph : public CMonster
{
public:
	CHalph(CCurrentGame *pSetCurrentGame = NULL,
		const MovementType eMovement = GROUND, const UINT type=M_HALPH);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CHalph);

	virtual bool BrainAffects() const {return false;}
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);

	CCoord GetCurrentGoal() const;
	bool GetPathTo(CCueEvents &CueEvents, const CCoordSet &adjDests, const CCoordSet *pDirectDests=NULL);
	CCoord GetLatestRequest() const;
	virtual bool IsAggressive() const {return false;}
	virtual bool IsFriendly() const {return true;}
	bool IsOpeningDoorAt(const CCoordSet& doorSquares) const;
	void RequestOpenDoor(const UINT wDoorX, const UINT wDoorY,
			CCueEvents &CueEvents);
	void SwordsmanBumps(CCueEvents &CueEvents);
	void UseObjectToOpenDoorAt(const UINT wX, const UINT wY);

private:
	enum HalphState
	{
		Entering,
		Following,
		Waiting,
		Striking,
		PostStriking
	};

	bool  SetGoal();
	bool  ConfirmGoal(const CCoord& orbLocation, const bool bPlateIsGoal);
	void  GetAdjDest(UINT& wDestX, UINT& wDestY) const;
	void  SetState(const HalphState eState);
	bool  TakeStep(CCueEvents& CueEvents);

	HalphState state;
	UINT wMoves;
	bool bPlateIsGoal; //Whether a Plate or an Orb is the current striking goal
	CCoord orbLocation; //Exact location of the chosen orb/plate (used to determine whether goal is still valid)
	CCoord openingDoorAt;
	CCoord latestRequest; //Most recent door the player requested opened (for front end)
	CCoordSet originSquare;
	CCoordSet orbsToHit, platesToDepress;  //set of possible destination orbs/plates
};

class CHalph2 : public CHalph
{
public:
	CHalph2(CCurrentGame *pSetCurrentGame = NULL, const UINT type=M_HALPH2);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CHalph2);
};

#endif //...#ifndef HALPH_H
