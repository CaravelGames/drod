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
 * Michael Welsh Duggan (md5i), Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Monster.h
//Declarations for CMonster.
//Abstract base class for holding monster data and performing game operations
//on it.

//SUMMARY
//
//CMonster is an abstract base class from which all monster types derive.
//A monster is just a collection of data and behavior common to most monster
//types.
//
//USAGE
//
//You can derive new monsters from CMonster.  It's necessary to add a new
//MONSTERTYPE enumeration at the top of MonsterFactory.h and add a switch
//handler in CMonsterFactory::GetNewMonster() that will construct your new class.
//Also, add an entry for the monster in TileConstants.h.
//
//In the derived monster class definition, add (1) a constructor calling CMonster,
//passing in the new monster enumeration, and
//(2) IMPLEMENT_CLONE(CMonster, <new monster type>) to make instances of your
//monster type copyable.
//
//If you followed all the above instructions without doing anything else, you
//would have a monster that just sits there (for example, see CBrain).
//
//You can make the monster do things by performing the following.
//1. Override Process().  This determines what the monster will do each turn.
//If a behavior specified in Process() is common among several monster types,
//consider refactoring that behavior into CMonster as an inheritable method.
//
//FRONT END
//
//For the front end, graphics for the new monster type will need to be
//created and specified (such as in GeneralTiles.bmp and TileImageCalcs.h).
//Other components, such as the level editor, probably also need to
//be revised accordingly to include the new monster type.
//
//

#ifndef MONSTER_H
#define MONSTER_H

#include "DbPackedVars.h"
#include "DbRefs.h"
#include "GameConstants.h"
#include "Weapons.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/AttachableObject.h>

#include <list>
using std::list;
#include <vector>
using std::vector;

//Within this radius, a monster can sense the player when invisible.
#define DEFAULT_SMELL_RANGE (5)

//# turns between which monsters do special things
#define TURNS_PER_CYCLE (30)

//Monster movement ability types. (update as needed)
//Remember to update bMovementSupportsPartialObstacles for movements supporting partial obstacles
enum MovementType {
	GROUND,
	AIR,
	WALL,
	WATER,
	GROUND_AND_SHALLOW_WATER,
	GROUND_FORCE,
	GROUND_AND_SHALLOW_WATER_FORCE,
	NumMovementTypes
};

enum MovementIQ {
	DirectOnly,           //if intended move is blocked, do nothing
	SmartDiagonalOnly,    //if intended diagonal is blocked, try vertical, then horizontal (classical 1.6 DROD movement)
	SmarterDiagonalOnly,  //if intended diagonal is blocked, choose the better axial direction
	SmartOmniDirection,   //can sidestep 1-square barriers in any direction
	SmartOmniFlanking,    // " and also flank target that is one tile away and inaccessible
	MIQ_Pathfind,         //calculate and follow optimal pathmap to destination, with beelining when no path available
	MIQ_PathfindOpenOnly  //calculate and follow optimal pathmap to destination, with no movement when blocked
};

enum SequenceSpeed {
	SPD_TEMPORALCLONE = 50,
	SPD_PDOUBLE   = 100,
	SPD_FEGUNDO   = 150,
	SPD_STALWART  = 175,
	SPD_SLAYER    = 200,
	SPD_GUARD     = 300,
	SPD_DEFAULT   = 1000,
	SPD_ARCHITECT = 1050,
	SPD_CITIZEN   = 1100,
	SPD_FLUFF     = 1500,
	SPD_CHARACTER = 9999
};

static inline bool bMovementSupportsPartialObstacles (MovementType movement)
//Returns true if the movement type supports pathmapping over partial obstacles such as
//arrows and orthosquares.  Also doesn't treat potions and horns as obstacles.
{
	return movement == GROUND_FORCE || movement == GROUND_AND_SHALLOW_WATER_FORCE;
}

//******************************************************************************************
//method used herein should be public
#define IMPLEMENT_CLONE(CBase, CDerived) virtual CBase* Clone() const \
	{ return new CDerived(*this); }
#define IMPLEMENT_CLONE_REPLICATE(CBase, CDerived) \
	IMPLEMENT_CLONE(CBase, CDerived) \
	virtual CBase* Replicate() const { return Clone(); }

class CEntity : public CMoveCoord
{
public:
	CEntity()
	: CMoveCoord(0, 0, 0)
	, wPrevX(0), wPrevY(0), wPrevO(0)
	{}

	void Clear() {
		wX = wPrevX = wY = wPrevY = wO = wPrevO = 0;
	}
	virtual bool  HasOrientation() const {return true;}

	UINT wPrevX, wPrevY; //previous position
	UINT wPrevO; //previous orientation
};

class CMonsterPiece;
typedef list<CMonsterPiece*> MonsterPieces;

class CCueEvents;
class CCurrentGame;
class CMonsterFactory;
class CDbRoom;
class CMonster : public CEntity
{
protected:
	friend class CMonsterFactory;
	CMonster(const UINT wSetType, const CCurrentGame *pSetCurrentGame = NULL,
			const MovementType eMovement = GROUND,
			const UINT wSetProcessSequence = SPD_DEFAULT);

public:
	virtual ~CMonster();

	virtual CMonster *Clone() const=0;
	virtual CMonster *Replicate() const=0;

	void          AskYesNo(MESSAGE_ID eMessageID, CCueEvents &CueEvents) const;
	virtual bool  BrainAffects() const {return true;}
	bool          CanDaggerStep(const UINT wMonsterType, const bool bIgnoreSheath = false) const;
	virtual bool  CanDropTrapdoor(const UINT /*oTile*/) const {return false;}
	virtual bool  CanFindSwordsman() const;
	virtual bool  CanHideInShallowWater() const {return false;}
	virtual bool  CanPushObjects() const { return false; }
	virtual bool  CanPressPressurePlates() const { return !this->IsFlying(); }
	virtual bool  CanSmellObjectAt(const UINT wX, const UINT wY) const;
	virtual bool  CanWadeInShallowWater() const { return this->eMovement == GROUND_AND_SHALLOW_WATER; }
	virtual bool  CheckForDamage(CCueEvents& CueEvents);
	void          Clear();
	bool          ConfirmPath();
	virtual void  Delete() { }
	UINT          DistToSwordsman(const bool bUsePathmap=true, const bool bIncludeNonTarget=false) const;
	bool          DoesArrowPreventMovement(const int dx, const int dy) const;
	bool          DoesArrowPreventMovement(const UINT wX, const UINT wY,
			const int dx, const int dy) const;
	virtual bool  DoesSquareContainObstacle(const UINT wX, const UINT wY) const;

	string        ExportXML() const;

	bool          FindOptimalPathTo(const UINT wX, const UINT wY,
			const CCoordSet &dests, const bool bAdjIsGood=true, const bool bPathThroughObstacles=false);
	bool          FindOptimalPath2(const UINT wStartX, const UINT wStartY,
			const UINT wGoalX, const UINT wGoalY, const bool bPathThroughObstacles=false);
	bool          FindOptimalPathToClosestMonster(const UINT wX, const UINT wY,
			const CIDSet& monsterTypes);

	void          GetAvoidSwordMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBestMove(int &dx, int &dy) const;
	void          GetBeelineMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBeelineMovementDumb(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBeelineMovementSmart(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy, const bool bSmartAxial,
			const bool bSmartFlanking=false) const;
	virtual bool  GetBrainDirectedMovement(int &dxFirst, int &dyFirst,
			int &dx, int &dy, const MovementIQ movementIQ=SmartDiagonalOnly) const;
	bool          GetDirectMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy,
			const MovementIQ movementIQ=SmartDiagonalOnly,
			const bool bIncludeNonTarget=false) const;
	static void GetCommandDXY(int nCommand, int &dx, int &dy);
	virtual UINT  GetIdentity() const {return this->wType;}
	const CMonster* GetOwningMonsterConst() const;
	inline CMonster* GetOwningMonster() { return const_cast<CMonster*>(GetOwningMonsterConst()); }
	CCoordSet     GetMatchingEndTiles(const vector<CCoord>& coords) const;
	virtual UINT  GetProcessSequence() const;
	virtual UINT  GetResolvedIdentity() const {return GetIdentity();}
	UINT          GetOrientationFacingTarget(const UINT wX, const UINT wY) const;
	bool          GetSwordCoords(UINT& wX, UINT& wY) const;
	virtual WeaponType GetWeaponType() const { return WT_Sword; }
	bool          GetTarget(UINT &wX, UINT &wY, const bool bConsiderDecoys=true);

	virtual bool  HasSword() const { return false; }
	bool          HasSwordAt(const UINT wX, const UINT wY) const;
	virtual bool  IsAggressive() const {return true;}
	virtual bool  IsAlive() const {return this->bAlive;}
	virtual bool  IsAttackableTarget() const {return false;}
	virtual bool  IsBrainPathmapObstacle() const {return false;}
	virtual bool  IsFlying() const {return this->eMovement == AIR;}
	virtual bool  IsFriendly() const {return false;}
	virtual bool  IsLongMonster() const {return false;}
	virtual bool  IsMonsterTarget() const;
	bool          IsNextToSwordsman() const;
	virtual bool  IsNPCPathmapObstacle() const {return true;}
	bool          IsConquerable() const;
	bool          IsObjectAdjacent(const UINT wObject, UINT& wX, UINT& wY) const;
	bool          IsOnSwordsman() const;
	virtual bool  IsOpenMove(const int dx, const int dy) const;
	virtual bool  IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const;
	virtual bool  IsPiece() const {return false;}
	virtual bool  IsPushableByBody() const { return false; }
	virtual bool  IsPushableByWeaponAttack() const { return false; }
	virtual bool  IsSwimming() const {return this->eMovement == WATER;}
	virtual bool  IsTarget() const { return false; }
	virtual bool  IsTileObstacle(const UINT wTileNo) const;
	virtual bool  IsHiding() const {return false;}
	virtual bool  IsVisible() const {return true;}
	virtual bool  IsWading() const;
	bool          IsStunned() const { return this->stunned != 0; }

	virtual bool  MakeSlowTurn(const UINT wDesiredO);
	void          MakeStandardMove(CCueEvents &CueEvents,
			const int dx, const int dy);
	bool          MakeThisMove(int &dx, int &dy) const;
	virtual void  Move(const UINT wX, const UINT wY, CCueEvents* pCueEvents=NULL);

	virtual void  MyClosestTile(const UINT wX, const UINT wY, UINT &wMyCX, UINT &wMyCY) const;
	bool          NPCBeethroDieCheck(const UINT wX, const UINT wY, CCueEvents& CueEvents);
	virtual bool  OnAnswer(int nCommand, CCueEvents &CueEvents);
	virtual bool  OnStabbed(CCueEvents &CueEvents, const UINT wX=(UINT)-1, const UINT wY=(UINT)-1,
			WeaponType weaponType=WT_Sword);
	virtual void  Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual void  PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents);
	virtual void  ReflectX(CDbRoom *pRoom);
	virtual void  ReflectY(CDbRoom *pRoom);
	virtual void  ReflectPieceX(CDbRoom *pRoom, CMonsterPiece *pPiece);
	virtual void  ReflectPieceY(CDbRoom *pRoom, CMonsterPiece *pPiece);
	void          ResetCurrentGame();
	UINT          RotationalDistance(const UINT wTargetO) const;
	virtual void  Save(const c4_RowRef &MonsterRowRef, const bool bSaveScript=true);
	void          Say(MESSAGE_ID eMessageID, CCueEvents &CueEvents) const;
	bool          SensesTarget() const;
	virtual void  SetCurrentGame(const CCurrentGame *pSetCurrentGame);
	virtual void  SetExtraVarsFromMembers() { }
	void          SetKillInfo(const UINT wKillDirection);
	virtual void  SetMembersFromExtraVars() { }
	void          SetOrientation(const int dxFirst, const int dyFirst);
	virtual void  Stun(CCueEvents &CueEvents, UINT val=1);
	virtual bool  TakesTurn() const { return IsAlive(); }

	UINT          wType;          //monster type
	UINT          wProcessSequence;  //priority in movement sequence
	UINT          wKillInfo;      //direction monster is killed in
	bool          bIsFirstTurn;
	bool          bUnlink;     //Does the monster have a deferred unlink to process?
	bool          bProcessing; //Is the monster currently taking its turn?
	MovementType  eMovement;   //movement capability
	bool          bAlive;  //whether monster is alive/active
	bool          bForceWeaponAttack;  //whether the monster should attack even if dead
	UINT          stunned; //whether monster is stunned and skips turn
	bool          bNewStun; //whether monster stun was inflicted this turn
	bool          bPushedThisTurn; //whether monster was pushed this turn
	bool          bWaitedOnHotFloorLastTurn; //for hasteable playerdoubles

	CDbPackedVars ExtraVars;
	MonsterPieces Pieces;  //when monster fills more than one square

	CMonster *    pNext; //should be updated by caller when copying a monster
	CMonster *    pPrevious;

protected:
	virtual bool  CanMoveOntoTunnelAt(UINT col, UINT row) const;

	int           getMinDistance(const UINT wX, const UINT wY, const CCoordSet &dests,
			ROOMCOORD& closestDest) const;

	float         DistanceToTarget(const UINT wX, const UINT wY, const UINT x, const UINT y,
			const bool bUseBrainDistance=true) const;
	UINT          RotationalDistanceCO(const UINT wTargetO) const;

	const CCurrentGame * pCurrentGame;
	CCoordStack pathToDest; //sequence of squares that lead to preferred goal coord
	ROOMCOORD goal; //goal coord

	static CCoordIndex_T<UINT> room; //for breadth-first search
	static CCoordIndex searchMoves;  //moves made during search
	static CCoordIndex swordsInRoom;  //speed optimization for pathmapping
	static bool calculatingPathmap;

private:
	void          PushPathFromGoal(UINT endX, UINT endY, UINT startX, UINT startY);
};

#endif //...#ifndef MONSTER_H
