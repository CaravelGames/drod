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

//Class for handling citizen monster game logic. 

#ifndef CITIZEN_H
#define CITIZEN_H

class CCurrentGame;

#include "Monster.h"
#include "MonsterFactory.h"
#include <BackEndLib/Coord.h>
#include <vector>

class CStation;
class CCitizen : public CMonster
{
public:
	CCitizen(CCurrentGame *pSetCurrentGame = NULL,
		UINT monsterType=M_CITIZEN,
		UINT processSequence=SPD_CITIZEN);  //move after monsters, before NPCs
	IMPLEMENT_CLONE_REPLICATE(CMonster, CCitizen);

	virtual bool CheckForDamage(CCueEvents& CueEvents);
	virtual bool DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	virtual bool IsTileObstacle(const UINT wTileNo) const;
	bool GetGoal(UINT& wX, UINT& wY) const;
	CCoordSet GetVisitedStations() const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	inline int StationType() const {return this->nStationType;}

	virtual bool IsAggressive() const {return false;}
	virtual bool IsFriendly() const {return true;}
	virtual bool OnStabbed(CCueEvents &/*CueEvents*/, const UINT /*wX*/=(UINT)-1, const UINT /*wY*/=(UINT)-1,
			WeaponType weaponType=WT_Sword);

protected:
	inline bool HasSupply() const { return bHasSupply; }

	void BuildTile(CCueEvents& CueEvents);
	bool CalcPathAndAdvance(CCueEvents& CueEvents);
	bool TakeStepTowardsGoal(CCueEvents &CueEvents);

	bool bDone; //true when there is nowhere to try to go
	bool bHasSupply;   //carrying a supply

private:
	void GotoStation(CCueEvents &CueEvents);

	float l2DistanceToStationSq(const UINT wStationIndex) const;
	int sequenceOfStation(const UINT wStationIndex) const;

	int nStationType;     //which station set is being visited
	std::vector<int> visitingSequence; //station route
	int nVisitingStation; //which station in sequence is being visited
	UINT wTurnsStuck;  //# of consecutive turns moving to a station has failed
};

#endif //...#ifndef CITIZEN_H
