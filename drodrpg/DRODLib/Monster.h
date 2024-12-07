// $Id: Monster.h 10108 2012-04-22 04:54:24Z mrimer $

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
#include "RoomData.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/AttachableObject.h>

#include <list>
using std::list;

static const UINT DEFAULT_PROCESS_SEQUENCE = 1000;

//Within this radius, a monster can sense the player when invisible.
#define DEFAULT_SMELL_RANGE (5)

//# turns between which monsters do special things
#define TURNS_PER_CYCLE (30)

//Monster movement ability types. (update as needed)
enum MovementType {
	GROUND,
	AIR,
	WALL,
	WATER,
	NumMovementTypes
};

enum MovementIQ {
	DirectOnly,          //if intended move is blocked, do nothing
	SmartDiagonalOnly,   //if intended diagonal is blocked, try vertical, then horizontal (classical 1.6 DROD movement)
	SmarterDiagonalOnly, //if intended diagonal is blocked, choose the better axial direction
	SmartOmniDirection   //can sidestep 1-square barriers in any direction
};

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
	virtual UINT  GetIdentity() const =0;
	virtual bool  HasOrientation() const {return true;}

	UINT wPrevX, wPrevY, wPrevO; //previous position
};

//******************************************************************************************
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
			const UINT wSetProcessSequence = DEFAULT_PROCESS_SEQUENCE);

public:
	virtual ~CMonster();

	virtual CMonster *Clone() const=0;
	virtual CMonster *Replicate() const=0;

	void          AskYesNo(MESSAGE_ID eMessageID, CCueEvents &CueEvents) const;
//	virtual bool  BrainAffects() const {return true;}
	virtual bool  CanAttackFirst() const {return false;}
	virtual bool  CanAttackLast() const {return false;}
	virtual bool  CanCutBriar() const {return false;}
	virtual bool  CanCutTarAnywhere() const {return false;}
	virtual bool  CanFindSwordsman() const;
	virtual bool  CanSmellObjectAt(const UINT wX, const UINT wY) const;
	virtual bool  CheckForDamage(CCueEvents& CueEvents);
	void          Clear();
//	bool          ConfirmPath();
	bool          Damage(CCueEvents& CueEvents, int damageVal);
	virtual bool  DamagedByHotTiles() const {return true;}
	virtual void  Delete() { }
	UINT          DistToSwordsman(const bool bIncludeNonTarget=false) const;
	bool          DoesArrowPreventMovement(const int dx, const int dy) const;
	bool          DoesArrowPreventMovement(const UINT wX, const UINT wY,
			const int dx, const int dy) const;
	virtual bool  DoesSquareContainObstacle(const UINT wX, const UINT wY) const;

	void          ExportXML(string &str) const;

	bool          FindOptimalPathTo(const UINT wX, const UINT wY,
			const CCoordSet &dests, const bool bAdjIsGood=true);
//	bool          FindOptimalPathToClosestMonster(const UINT wX, const UINT wY,
//			const CIDSet& monsterTypes);
	void          GetAvoidSwordMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBestMove(int &dx, int &dy) const;
	void          GetBeelineMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBeelineMovementDumb(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy) const;
	void          GetBeelineMovementSmart(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy, const bool bSmartAxial) const;
/*
	virtual bool  GetBrainDirectedMovement(int &dxFirst, int &dyFirst,
			int &dx, int &dy, const MovementIQ movementIQ=SmartDiagonalOnly) const;
*/
	bool          GetDirectMovement(const UINT wX, const UINT wY,
			int &dxFirst, int &dyFirst, int &dx, int &dy,
			const MovementIQ movementIQ=SmartDiagonalOnly,
			const bool bIncludeNonTarget=false) const;
	virtual UINT  GetIdentity() const {return this->wType;}
	virtual UINT  GetLogicalIdentity() const {return this->wType;}
	static  bool  GetNextGaze(CCueEvents &CueEvents, CMonster *pCaster, CDbRoom *pRoom,
			const bool bElevatedSource, UINT& cx, UINT& cy, int& dx, int& dy);
	UINT          GetOrientationFacingTarget(const UINT wX, const UINT wY) const;
	const CMonster* GetOwningMonsterConst() const;
	inline CMonster* GetOwningMonster() { return const_cast<CMonster*>(GetOwningMonsterConst()); }
	CCoordStack*  GetPathToGoal() {return &this->pathToDest;}
	virtual UINT  GetResolvedIdentity() const {return GetIdentity();}
	virtual UINT  GetSpawnType(UINT defaultMonsterID) const;
	bool          GetSwordCoords(UINT& wX, UINT& wY) const;
	virtual SwordType GetWeaponType() const { return SwordType::NoSword; }
	bool          GetTarget(UINT &wX, UINT &wY);

	virtual UINT  getATK() const;
	virtual UINT  getColor() const;
	virtual UINT  getDEF() const;
	virtual int   getGOLD() const; //may be negative
	virtual UINT  getHP() const;
	virtual UINT  getSword() const;
	virtual int   getXP() const; //may be negative

	virtual bool  HasCustomWeakness() const {return false;}
	virtual bool  HasGoblinWeakness() const {return false;}
	virtual bool  HasNoEnemyDefense() const {return false;}
	virtual bool  HasRayGun() const {return false;}
	virtual bool  HasRayBlocking() const {return false;}
	virtual bool  HasSerpentWeakness() const {return false;}
	virtual bool  HasSword() const {return false;}
	bool          HasSwordAt(const UINT wX, const UINT wY) const;
	virtual bool  IsAggressive() const {return true;}
	virtual bool  IsAlive() const {return this->bAlive;}
	virtual bool  IsAttackableTarget() const {return false;}
	virtual bool  IsCombatable() const;
	virtual bool  IsDamageableAt(const UINT /*wX*/, const UINT /*wY*/) const {return true;}
	virtual bool  IsExplosiveSafe() const { return false; }
	virtual bool  IsFlying() const {return this->eMovement == AIR;}
	virtual bool  IsFriendly() const {return false;}
	virtual bool  IsLongMonster() const {return false;}
	bool          IsNextToSwordsman() const;
	bool          IsObjectAdjacent(const UINT wObject, UINT& wX, UINT& wY) const;
	virtual bool  IsOnMistTile() const;
	bool          IsOnSwordsman() const;
	virtual bool  IsOpenMove(const int dx, const int dy) const;
	virtual bool  IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const;
	virtual bool  IsPiece() const {return false;}
	bool          IsSpawnEggTriggered(const CCueEvents& CueEvents) const;
	virtual bool  IsSwimming() const {return this->eMovement == WATER;}
	virtual bool  IsTileObstacle(const UINT wTileNo) const;
	virtual bool  IsMinimapTreasure() const { return false; }
	virtual bool  IsVisible() const {return true;}
	virtual bool  IsWallAndMirrorSafe() const { return false; }
	bool          MakeSlowTurn(const UINT wDesiredO);
	void          MakeStandardMove(CCueEvents &CueEvents,
			const int dx, const int dy);
	bool          MakeThisMove(int &dx, int &dy) const;
	virtual void  Move(const UINT wX, const UINT wY, CCueEvents* pCueEvents=NULL);
	virtual bool  OnAnswer(int nCommand, CCueEvents &CueEvents);
	virtual bool  OnStabbed(CCueEvents &CueEvents, const UINT wX=(UINT)-1, const UINT wY=(UINT)-1);
	virtual void  Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual bool  ProcessAfterAttack(CCueEvents& /*CueEvents*/) {return true;} //each time I attack the player, do this
	virtual bool  ProcessAfterDefend(CCueEvents& /*CueEvents*/) {return true;} //each time the player attacks me, do this
	virtual bool  ProcessAfterUse(CCueEvents& /*CueEvents*/) {return true;} //each time I am used as an item, do this
	virtual void  ReflectX(CDbRoom *pRoom);
	virtual void  ReflectY(CDbRoom *pRoom);
	virtual void  ReflectPieceX(CDbRoom *pRoom, CMonsterPiece *pPiece);
	virtual void  ReflectPieceY(CDbRoom *pRoom, CMonsterPiece *pPiece);
	void          ResetCurrentGame();
	void          ResetMyPathToGoal() {this->pathToDest.Clear();}
	virtual void  RotateClockwise(CDbRoom *pRoom);
	virtual void  RotatePieceClockwise(CDbRoom *pRoom, CMonsterPiece *pPiece);
	UINT          RotationalDistance(const UINT wTargetO) const;
	virtual void  Save(const c4_RowRef &MonsterRowRef, const bool bSaveScript=true);
	void          Say(MESSAGE_ID eMessageID, CCueEvents &CueEvents) const;
	virtual void  SetCurrentGame(const CCurrentGame *pSetCurrentGame);
	virtual void  SetExtraVarsForExport() { } //monsters typically do not write any extra vars
	void          SetHP();
	void          SetKillInfo(const UINT wKillDirection);
	virtual void  SetMembers(const CDbPackedVars& vars);
	void          SetOrientation(const int dxFirst, const int dyFirst);
	void          SpawnEgg(CCueEvents& CueEvents);
	virtual bool  TurnToFacePlayerWhenFighting() const {return false;}
	void          UpdateGaze(CCueEvents &CueEvents);

	UINT          wType;          //monster type
	UINT          wProcessSequence;  //priority in movement sequence
	bool          bIsFirstTurn;
	MovementType  eMovement;   //movement capability
	bool          bAlive;  //whether monster is alive/active

	UINT HP, ATK, DEF, GOLD, XP; //combat stats

	bool          bEggSpawn; //was generated by an egg-laying monster

	CDbPackedVars ExtraVars; //used only for saving stats during play and for storing CCharacter properties and scripting
	MonsterPieces Pieces;  //when monster fills more than one square

	CMonster *    pNext; //should be updated by caller when copying a monster
	CMonster *    pPrevious;

protected:
	//Behavior patterns.
	bool          AttackPlayerWhenAdjacent(CCueEvents& CueEvents);
	bool          AttackPlayerInFrontWhenBackIsTurned(CCueEvents &CueEvents);
	bool          AttackPlayerWhenInFront(CCueEvents &CueEvents);
	void          FaceAwayFromTarget();
	void          FaceTarget();
	bool          IsTileAboveMe(const UINT wTX, const UINT wTY) const;

	int           getMinDistance(const UINT wX, const UINT wY, const CCoordSet &dests,
			ROOMCOORD& closestDest) const;

	float         DistanceToTarget(const UINT wX, const UINT wY, const UINT x, const UINT y) const;
//			const bool bUseBrainDistance=true) const;
	UINT          RotationalDistanceCO(const UINT wTargetO) const;

	const CCurrentGame * pCurrentGame;
	CCoordStack pathToDest; //sequence of squares that lead to preferred goal coord
	ROOMCOORD goal; //goal coord

	static CCoordIndex_T<USHORT> room; //for breadth-first search
	static CCoordIndex swordsInRoom;  //speed optimization for pathmapping
};

//*****************************************************************************
//A child class that may be derived from to have a monster that simply
//faces the player on its turn.
class CMonsterFacesTarget : public CMonster
{
public:
	CMonsterFacesTarget(const UINT wSetType, const CCurrentGame *pSetCurrentGame = NULL,
			const MovementType eMovement = GROUND,
			const UINT wSetProcessSequence = DEFAULT_PROCESS_SEQUENCE);

	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};


#endif //...#ifndef MONSTER_H

