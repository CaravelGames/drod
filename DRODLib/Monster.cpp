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
 * Michael Welsh Duggan (md5i), John Wm. Wicks (j_wicks), Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Monster.cpp
//Implementation of CMonster.

#include "Monster.h"
#include "MonsterPiece.h"
#include "PlayerDouble.h"
#include "Character.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>

#include <math.h>

#define NO_TARGET ((UINT)-1) //this distance to a target represents no valid option

CCoordIndex_T<UINT> CMonster::room;
CCoordIndex CMonster::searchMoves;
CCoordIndex CMonster::swordsInRoom;
bool CMonster::calculatingPathmap = false;

//Node object used in pathfinding search.
class CPathNode : public CCoord
{
public:
	CPathNode(const UINT wX, const UINT wY, const UINT wCost, const UINT wScore)
		: CCoord(wX, wY)
		, wCost(wCost), wScore(wScore)
	{ }
	UINT wCost;    //f -- distance already traveled
	UINT wScore;   //f+g -- cost + lowest possible remaining distance to goal

	//For priority queue insertion:
	//An element being inserted with equal or higher dwScore gets placed later in the queue.
	//FIXME: The current operator could have different behavior, based on implementation.
	bool operator <(const CPathNode& mc) const {return this->wScore >= mc.wScore;}
};

//Fixed version
class CPathNode2 : public CCoord
{
public:
	CPathNode2(const UINT wX, const UINT wY, const UINT wCost, const UINT wScore)
		: CCoord(wX, wY)
		, wCost(wCost), wScore(wScore)
	{ }
	UINT wCost;    //f -- distance already traveled
	UINT wScore;   //f+g -- cost + lowest possible remaining distance to goal

	//For priority queue insertion:
	bool operator <(const CPathNode2& mc) const {return mc.wScore < this->wScore;}
};

namespace Path
{
	const UINT wNumNeighbors = 8;
	const int dXs[wNumNeighbors] = { 0,  1,  0, -1,  1,  1, -1, -1};
	const int dYs[wNumNeighbors] = {-1,  0,  1,  0, -1,  1,  1, -1};
	const UINT O_MOD = 16;   //each cell in CMonster::room stores (dist * O_MOD + direction from previous square)
}

//
//CMonster methods.
//

//*****************************************************************************
CMonster::CMonster(
	const UINT wSetType, const CCurrentGame *pSetCurrentGame,
	const MovementType eMovement,
	const UINT wSetProcessSequence)
	: CEntity()
	, wType(wSetType)
	, wProcessSequence(wSetProcessSequence)
	, wKillInfo(NO_ORIENTATION)
	, bIsFirstTurn(false)
	, bUnlink(false), bProcessing(false)
	, eMovement(eMovement)
	, bAlive(true)
	, bForceWeaponAttack(false)
	, stunned(0), bNewStun(false)
	, bPushedThisTurn(false)
	, bWaitedOnHotFloorLastTurn(false)
	, pNext(NULL), pPrevious(NULL)
	, pCurrentGame(NULL)
	, bSafeToDelete(true)
{
	if (pSetCurrentGame)
		SetCurrentGame(pSetCurrentGame);
}

//*****************************************************************************
CMonster::~CMonster() 
{
	Clear();
}

//*****************************************************************************
void CMonster::Clear()
//Frees resources and zeroes members.
{  
	CEntity::Clear();

	this->pPrevious=this->pNext=NULL;
	this->wType=this->wProcessSequence=0;
	this->wKillInfo = NO_ORIENTATION;
	this->bIsFirstTurn=this->bUnlink=this->bProcessing=false;
	this->stunned = 0;
	this->bNewStun = false;
	this->bPushedThisTurn = false;
	this->ExtraVars.Clear();
	while (this->Pieces.size())
	{
		delete this->Pieces.back();
		this->Pieces.pop_back();
	}
	this->bAlive = true;
}

//*****************************************************************************
void CMonster::ReflectX(CDbRoom *pRoom)
{
	ASSERT(pRoom);
	const UINT wNewX = (pRoom->wRoomCols-1) - this->wX;
	if (this == pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX, this->wY)])
		std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(wNewX,this->wY)]);

	for (MonsterPieces::const_iterator piece=this->Pieces.begin();
			piece != this->Pieces.end(); ++piece)
	{
		ReflectPieceX(pRoom, *piece);
	}

	this->wX = wNewX;
	this->wO = nGetO(-nGetOX(this->wO),nGetOY(this->wO));
}

//*****************************************************************************
void CMonster::ReflectY(CDbRoom *pRoom)
{
	ASSERT(pRoom);
	const UINT wNewY = (pRoom->wRoomRows-1) - this->wY;
	if (this == pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX, this->wY)])
		std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,wNewY)]);

	for (MonsterPieces::const_iterator piece=this->Pieces.begin();
			piece != this->Pieces.end(); ++piece)
	{
		ReflectPieceY(pRoom, *piece);
	}

	this->wY = wNewY;
	this->wO = nGetO(nGetOX(this->wO),-nGetOY(this->wO));
}

//*****************************************************************************
void CMonster::ReflectPieceX(CDbRoom *pRoom, CMonsterPiece *pPiece)
{
	pPiece->ReflectX(pRoom);
}

//*****************************************************************************
void CMonster::ReflectPieceY(CDbRoom *pRoom, CMonsterPiece *pPiece)
{
	pPiece->ReflectY(pRoom);
}

//*****************************************************************************
void CMonster::ResetCurrentGame()
//Resets the current game pointer.  (Called only from the room editor.)
{
	this->pCurrentGame = NULL;

	for (MonsterPieces::iterator pieceIt = this->Pieces.begin();
			pieceIt != this->Pieces.end(); ++pieceIt)
	{
		(*pieceIt)->pCurrentGame = NULL;
	}
}

//*****************************************************************************
void CMonster::SetCurrentGame(
//Sets current game pointer for monster.  This is necessary for many methods of 
//the monster class to work.
//
//Params:
	const CCurrentGame *pSetCurrentGame) //(in)
{
	ASSERT(pSetCurrentGame);
	if (this->pCurrentGame == pSetCurrentGame)
		return;
	ASSERT(!this->pCurrentGame);
	this->pCurrentGame = pSetCurrentGame;

	for (MonsterPieces::iterator pieceIt = this->Pieces.begin();
			pieceIt != this->Pieces.end(); ++pieceIt)
	{
		(*pieceIt)->pCurrentGame = pSetCurrentGame;
	}
}

//*****************************************************************************
void CMonster::SetKillInfo(const UINT wKillDirection)
//Sets values for display of monster death.
{
	this->wKillInfo = IsValidOrientation(wKillDirection) ?
			wKillDirection : NO_ORIENTATION;
}

//*****************************************************************************
void CMonster::SetOrientation(
//Sets monster orientation based on direction vectors.
//
//Params:
	const int dxFirst, const int dyFirst)   //(in) Direction to face.
{
	ASSERT(abs((int)dxFirst) <= 1);
	ASSERT(abs((int)dyFirst) <= 1);
	const UINT wNewO = nGetO(dxFirst, dyFirst);
	ASSERT(IsValidOrientation(wNewO));
	if (wNewO != NO_ORIENTATION || !HasOrientation())
		this->wO = wNewO;
}

//*****************************************************************************
void CMonster::Process(
//Overridable method to process a monster for movement after swordsman moves.  If derived
//class uses this method, then the monster will do nothing.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &/*CueEvents*/) //(in) Accepts pointer to an IDList object that will be populated
							//codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//Monster does nothing and nothing happens.
}

//*****************************************************************************
//Monster is pushed in a direction, then, optionally, skips next turn
void CMonster::PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents)
{
	//Operation not implemented for large monsters
	ASSERT(!IsPiece());
	ASSERT(!IsLongMonster());
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	UINT wPrevX = this->wPrevX = this->wX;
	UINT wPrevY = this->wPrevY = this->wY;
	UINT wDestX = this->wX + dx;
	UINT wDestY = this->wY + dy;

	Move(wDestX, wDestY, &CueEvents);
	room.CheckForFallingAt(wDestX, wDestY, CueEvents);
	//FIXME: Need to discuss whether interactions with tokens/fuses/etc. are useful

	//Tar Mothers pushed off of unstable tar causes the unstable tar to be destroyed
	if (bIsMother(this->wType))
	{
		UINT wTarType = room.GetTSquare(wPrevX,wPrevY);
		if (bIsTar(wTarType) && !room.IsTarStableAt(wPrevX,wPrevY,wTarType))
			if (room.StabTar(wPrevX,wPrevY,CueEvents,true,nGetO(dx,dy)))
				room.ConvertUnstableTar(CueEvents);
	}
	if (CanDropTrapdoor(room.GetOSquare(wPrevX,wPrevY)))
		room.DestroyTrapdoor(wPrevX,wPrevY,CueEvents);
	if (bIsRockGolemType(this->wType) && !IsAlive())
	{
		room.UpdatePathMapAt(wPrevX,wPrevY);
		room.UpdatePathMapAt(wDestX,wDestY);
		room.RecalcStationPaths();
	}

	if (bStun && IsAlive())
		Stun(CueEvents, 2);

	this->bPushedThisTurn = true;
	room.pushed_monsters.insert(this);
}

//*****************************************************************************
//Cause monster to skip its next N turns
void CMonster::Stun(CCueEvents &CueEvents, UINT val) //[default=1]
{
	// With Large Monsters, only Gentryii heads can be stunned.
	if (IsPiece()) return;
	if (IsLongMonster() && wType != M_GENTRYII) return;

	this->bNewStun = true;
	if (this->stunned < val)
		this->stunned = val;

	if (val)
		CueEvents.Add(CID_Stun, new CStunTarget(this, val), true);
}

//*****************************************************************************
bool CMonster::OnStabbed(
//Overridable method to process the effects of a monster being stabbed.  If derived class
//uses this method, then a cue event of monster being killed will be returned.
//
//Params:
	CCueEvents &CueEvents,  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
	const UINT /*wX*/, const UINT /*wY*/, //(in) what square of the monster was stabbed [default = (-1,-1)]
	WeaponType /*weaponType*/) //(in) weapon type that is dealing damage
//
//Returns: whether monster was killed
{
	//Monster dies.
	CueEvents.Add(CID_MonsterDiedFromStab, this);
	return true;
}

//*****************************************************************************
bool CMonster::CanMoveOntoTunnelAt(UINT col, UINT row) const
{
	//Monster can move into tunnel entrance to attack player.
	if (this->pCurrentGame->IsPlayerAt(col, row))
		return true;

	if (this->pCurrentGame->pRoom->GetMonsterTypeAt(col, row) == M_TEMPORALCLONE)
		return true;

	return false;
}

//*****************************************************************************
bool CMonster::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	ASSERT(this->bAlive);
	const bool bOnHotTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT;

	//Damaged by remaining stationary on a hot tile?
	const bool bMoved = this->wX != this->wPrevX || this->wY != this->wPrevY;


	if (this->Pieces.size() == 0){
		if ((!this->bWaitedOnHotFloorLastTurn && this->pCurrentGame->swordsman.bIsHasted) || bMoved){
			this->bWaitedOnHotFloorLastTurn = bOnHotTile && !bMoved;
			return false;
		}
	} else if (this->pCurrentGame->bHalfTurn || bMoved){
		// Monsters with pieces cannot be pushed and thus they don't need the elaborate check that is used above
		// to make pushing on hot tiles more logical - they are simply damaged when not moved and it is a full turn
		return false;
	}
	

	const UINT wIdentity = GetResolvedIdentity();
	//Flying and tarstuff identities are safe from hot tiles.
	if (bIsEntityFlying(wIdentity) || bIsMonsterTarstuff(wIdentity))
		return false;

	if (this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT)
	{
		CCueEvents Ignored;
		if (OnStabbed(Ignored, this->wX, this->wY))
		{
			//Add special cue events here instead of inside OnStabbed.
			CueEvents.Add(CID_MonsterBurned, this);
			return true;
		} else {
			if (Ignored.HasOccurred(CID_MonsterPieceStabbed))
				CueEvents.Add(CID_MonsterBurned, this);
		}
	}

	//Check whether pieces are harmed.
	CMonsterPiece *pPiece;
	MonsterPieces::const_iterator piece=this->Pieces.begin();
	while (piece != this->Pieces.end())
	{
		pPiece = *piece++;
		const UINT wX = pPiece->wX, wY = pPiece->wY;
		if (this->pCurrentGame->pRoom->GetOSquare(wX, wY) == T_HOT)
		{
			CCueEvents Ignored;
			if (OnStabbed(Ignored, wX, wY))
			{
				CueEvents.Add(CID_MonsterBurned, this);
				return true;
			} else {
				if (Ignored.HasOccurred(CID_MonsterPieceStabbed))
				{
					CueEvents.Add(CID_MonsterBurned,
							new CMoveCoord(wX, wY, NO_ORIENTATION), true);
					return false; //don't burn any more pieces this turn
				}
			}
		}
	}
	return false;
}

//*****************************************************************************
float CMonster::DistanceToTarget(
// Figures out distance to the target (usually swordsman) from a square.
//
//Params:
	const UINT wX, const UINT wY, //(in)  Target location
	const UINT x, const UINT y,   //(in)  Coords to check (i.e. monster location)
	const bool bUseBrainDistance)	//(in)	[default=true]
const
{
   if (bUseBrainDistance && this->pCurrentGame->bBrainSensesSwordsman)
	{
		const SQUARE square = this->pCurrentGame->pRoom->pPathMap[this->eMovement]->
				GetSquare(this->wX, this->wY);
		if (square.eBlockedDirections != DMASK_ALL && square.dwTargetDist > 2 &&
				square.dwTargetDist != static_cast<UINT>(-1))
		{
			//Brain-directed "avoid-sword movement".
			const SQUARE square2 = this->pCurrentGame->pRoom->pPathMap[this->eMovement]->
					GetSquare(x, y);
			//Discourage diagonal movements.
			const bool diagonal = ((this->wX - x) && (this->wY - y));
			return (float)(square2.dwTargetDist + (diagonal ? 0.5 : 0));
		}
	}

	//Calculate Euclidean distance.
	const int xd = x - wX;
	const int yd = y - wY;
	return sqrt(static_cast<float>(xd * xd + yd * yd));
}

//*****************************************************************************
UINT CMonster::DistToSwordsman(
	const bool bUsePathmap,       //Use pathmap [default=true]
	const bool bIncludeNonTarget) //Find swordsman even if not target [default=false]
const
//Returns: pathmap distance to player if available, else L(inf) distance to player
{
	CPathMap *pPathMap = this->pCurrentGame->pRoom->pPathMap[this->eMovement];
	if (bUsePathmap && pPathMap)
	{
		const UINT dwStepsToPlayer = pPathMap->GetSquare(this->wX, this->wY).dwSteps;
		if (dwStepsToPlayer < this->pCurrentGame->pRoom->CalcRoomArea())   //valid?
			return dwStepsToPlayer;
	}

	UINT wSX, wSY;
	this->pCurrentGame->GetSwordsman(wSX, wSY, bIncludeNonTarget);
	return nDist(this->wX, this->wY, wSX, wSY);
}

//*****************************************************************************
bool CMonster::DoesArrowPreventMovement(
//Does an arrow prevent movement of this monster to a destination square?
//The current square and destination square are both checked for arrows.
//
//Params:
	const int dx, const int dy)   //(in)   Offsets that indicate direction
											//    of movement from current square.
//
//Returns:
//True if an arrow prevents movement, false if not.
const
{
	return DoesArrowPreventMovement(this->wX, this->wY, dx, dy);
}

//*****************************************************************************
bool CMonster::DoesArrowPreventMovement(
//Does an arrow prevent movement of this monster to a destination square?
//The current square and destination square are both checked for arrows.
//
//Params:
	const UINT wX, const UINT wY, //(in)   Square to check from
	const int dx, const int dy)   //(in)   Offsets that indicate direction
											//    of movement from current square.
//
//Returns:
//True if an arrow prevents movement, false if not.
const
{
	//Check for obstacle arrow in current square.
	const int nO = nGetO(sgn(dx), sgn(dy));   //allow for dx/dy > 1
	UINT wTileNo = this->pCurrentGame->pRoom->GetFSquare(wX, wY);
	if (bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo, nO))
		return true;

	//Check for obstacle arrow in destination square.
	if (!this->pCurrentGame->pRoom->IsValidColRow(wX + dx, wY + dy))
		return false; //there is no arrow outside of room bounds
	wTileNo = this->pCurrentGame->pRoom->GetFSquare(wX + dx, wY + dy);
	return bIsArrow(wTileNo) && bIsArrowObstacle(wTileNo, nO);
}

//*****************************************************************************
bool CMonster::DoesSquareContainObstacle(
//Determines if a square contains an obstacle for this monster.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY || this->wType == M_GENTRYII);

	//Check t-square for obstacle.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow)) return true;

	//Some t-layer objects are obstacles.  Check for these.
	UINT wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
		return true;

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check o-square obstacle.
	wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.

		if (bIsTunnel(wLookTileNo))
		{
			if (CanMoveOntoTunnelAt(wCol,wRow))
				goto CheckMonster;
		}

		//No special handling was performed.  Treat it as an obstacle.
		return true;
	}

CheckMonster:
	//Can only move onto attackable monsters.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	
	if (pMonster){
		const int dx = (int)wCol - (int)this->wX;
		const int dy = (int)wRow - (int)this->wY;

		if (
			pMonster->wType != M_FLUFFBABY // Fluff babies can be stepped on regardless of anything
			&& !this->CanDaggerStep(pMonster->wType, false)
			// Even when attackable, body-attack-invulnerable targets just can't be killed by a body-attack
			&& (!pMonster->IsAttackableTarget() || !bIsVulnerableToBodyAttack(pMonster->GetIdentity()))
			&& ( // If object is pushable AND cannot be pushed
				!this->CanPushObjects()
				|| !pMonster->IsPushableByBody()
				|| !room.CanPushMonster(pMonster, wCol, wRow, wCol + dx, wRow + dy)
			)
		){

			if (!CMonster::calculatingPathmap || pMonster->IsNPCPathmapObstacle())
				return true;
		}
	}

	//Check for non-hostile or invulnerable player at square.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsInRoom() &&
			!(player.IsTarget() && player.IsVulnerableToBodyAttack()) &&
			player.wX == wCol && player.wY == wRow)
		return true;
	
	//Check for player's sword at square.
	if (this->pCurrentGame->IsPlayerWeaponAt(wCol, wRow, true))
		return true;

	//Check for monster sword at square.
	if (room.IsMonsterSwordAt(wCol, wRow, true, this))
		return true;

	//No obstacle.
	return false;
}

//*****************************************************************************
#define STARTVPTAG(vpType,pType) "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDVPTAG(vpType) "</"; str += ViewpropTypeStr(vpType); str += ">\n"
#define CLOSETAG "'/>\n"
#define CLOSESTARTTAG "'>\n"
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

//*****************************************************************************
string CMonster::ExportXML() const
//Returns: string containing XML text describing room with this ID
{
	string str;

	char dummy[32];

	str += STARTVPTAG(VP_Monsters, P_Type);
	ASSERT(IsValidMonsterType(this->wType));
	str += INT32TOSTR(this->wType);
	str += PROPTAG(P_X);
	str += INT32TOSTR(this->wX);
	str += PROPTAG(P_Y);
	str += INT32TOSTR(this->wY);
	str += PROPTAG(P_O);
	ASSERT(IsValidOrientation(this->wO));
	str += INT32TOSTR(this->wO);
	if (this->bIsFirstTurn)
	{
		str += PROPTAG(P_IsFirstTurn);
		str += INT32TOSTR(this->bIsFirstTurn);
	}
	if (this->wProcessSequence != SPD_DEFAULT)
	{
		str += PROPTAG(P_ProcessSequence);
		str += INT32TOSTR(this->wProcessSequence);
	}

	UINT dwBufferSize;
	BYTE *pExtraVars = this->ExtraVars.GetPackedBuffer(dwBufferSize);
	if (dwBufferSize > 4)   //null buffer
	{
		str += PROPTAG(P_ExtraVars);
		str += Base64::encode(pExtraVars,dwBufferSize-4);  //strip null UINT
	}
	delete[] pExtraVars;

	//Monster segments.
	if (!this->Pieces.size())
		str += CLOSETAG;
	else {
		str += CLOSESTARTTAG;
		CMonsterPiece *pPiece;
		for (MonsterPieces::const_iterator iter=this->Pieces.begin();
				iter != this->Pieces.end(); ++iter)
		{
			pPiece = *iter;
			//ASSERT(pRoom->IsValidColRow(pPiece->wX, pPiece->wY));
			str += STARTVPTAG(VP_Pieces, P_Type);
			str += INT32TOSTR(pPiece->wTileNo);
			str += PROPTAG(P_X);
			str += INT32TOSTR(pPiece->wX);
			str += PROPTAG(P_Y);
			str += INT32TOSTR(pPiece->wY);
			str += CLOSETAG;
		}
		str += ENDVPTAG(VP_Monsters);
	}

	return str;
}
#undef STARTVPTAG
#undef PROPTAG
#undef ENDVPTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR

//*****************************************************************************
bool CMonster::ConfirmPath()
//Returns: true if a recorded path to goal exists and is still valid, else false
{
	const UINT wSize = this->pathToDest.GetSize();
	if (!wSize) return false;

	//Check each square along the path.
	bool bPathOpen = true;
	UINT wPrevX = this->wX, wPrevY = this->wY;
	UINT wX, wY;
	int dx, dy;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	for (UINT wIndex=wSize; wIndex--; ) //from top of stack down to bottom
	{
		this->pathToDest.GetAt(wIndex, wX, wY);

		// Backwards compatibility hack: if a path resides on monster's current position,
		// regenerate the path. Previously, it would run DoesSquareContainObstacle() at the monster's
		// position, which would assert, then return that an obstacle was found.

		const bool bIsCurrentMonsterTile = wX == this->wX && wY == this->wY;

		if (bIsCurrentMonsterTile || DoesSquareContainObstacle(wX, wY))
		{
			bPathOpen = false;
			break;
		}
		dx = wX - wPrevX;
		dy = wY - wPrevY;
		if ((abs(dx) > 1 || abs(dy) > 1) || //invalid step
				DoesArrowPreventMovement(wPrevX, wPrevY, dx, dy) ||
				room.DoesSquarePreventDiagonal(wPrevX, wPrevY, dx, dy))
		{
			bPathOpen = false;
			break;
		}
		wPrevX = wX;
		wPrevY = wY;
	}
	return bPathOpen;
}

//*****************************************************************************
bool CMonster::FindOptimalPathTo(
//Find the shortest path to any of the given destinations using A* search.
//
//Returns: true if path found, else false
//
//Params:
	const UINT wX, const UINT wY, //(in) starting location
	const CCoordSet &dests, //(in) possible goal destinations
	const bool bAdjIsGood,  //[default=true] whether arriving at any square
									//adjacent to a destination is a sufficient solution
	const bool bPathThroughObstacles) //[default=false] if set,
	   //then when path is completely blocked, find the path with fewest obstacles to the goal
{
	this->pathToDest.Clear();

	if (dests.empty())
		return false; //no destination specified

	//Find closest destination.
	const int nCloseEnough = bAdjIsGood ? 1 : 0;
	int dist = getMinDistance(wX, wY, dests, this->goal);
	if (dist <= nCloseEnough)
		return true;   //Next to the destination -- no search needed.

	//Init search.
	CDbRoom& curGameRoom = *(this->pCurrentGame->pRoom);
	VERIFY(CMonster::room.Init(curGameRoom.wRoomCols, curGameRoom.wRoomRows));

	//Push starting node.
	const UINT dwCostThroughObstacle = CMonster::room.GetArea() * Path::O_MOD;
	CMonster::room.Add(wX, wY, Path::O_MOD + Path::wNumNeighbors);  //small number ensures this square will never be visited
	CPathNode coord(wX, wY, 0, dist);
	priority_queue<CPathNode> open;
	open.push(coord);

	UINT bestScore = 0;
	int dx, dy, wNewX, wNewY;
	do {
		//Expand best-valued node.
		coord = open.top();
		open.pop();

		//If a questionable path has been found,
		//continue searching until no better path might be found.
		if (bestScore && bestScore < coord.wCost)
			return true; //nothing better -- return found path

		//Move to neighbors in this order (discourage diagonal moves when possible).
		const UINT wCostPlusOne = coord.wCost/Path::O_MOD + 1;
		for (UINT nIndex=0; nIndex<Path::wNumNeighbors; ++nIndex)
		{
			wNewX = coord.wX + (dx = Path::dXs[nIndex]);
			wNewY = coord.wY + (dy = Path::dYs[nIndex]);
			if (!curGameRoom.IsValidColRow(wNewX, wNewY)) continue;
			const UINT wSquareScore = CMonster::room.GetAt(wNewX,wNewY) / Path::O_MOD;
			//Only allow visiting a square if (1) it hasn't been visited, or
			//(2) this is the shortest path to the square so far.
			if (wSquareScore && wSquareScore <= wCostPlusOne) continue;
			//If monster can move to this square...
			const bool bOpenMove = IsOpenMove(coord.wX, coord.wY, dx, dy);
			if (bOpenMove || bPathThroughObstacles)
			{
				UINT newScore = wCostPlusOne * Path::O_MOD + nIndex;
				if (!bOpenMove) {
					newScore += dwCostThroughObstacle;
					if (wSquareScore && wSquareScore <= newScore-nIndex)
						continue;
				}

				CMonster::room.Add(wNewX, wNewY, newScore); //node is now visited

				dist = getMinDistance(wNewX, wNewY, dests, this->goal);
				if (dist <= nCloseEnough) //Close enough to destination.
				{
					//Construct path to goal.
					this->pathToDest.Clear();
					PushPathFromGoal(UINT(wNewX), UINT(wNewY), wX, wY);
					if (bOpenMove)
						return true; //guaranteed best path
					//otherwise, allow search to continue until no better path might be found
					bestScore = newScore;
				} else {
					//...Add this unvisited node to priority queue.
					open.push(CPathNode(wNewX, wNewY, newScore, newScore));
					//NOTE: the heuristic below makes for a faster search, but monster
					//won't be guaranteed to prefer non-diagonal movement involving roundabout paths to the goal.
					//open.push(CPathNode(wNewX, wNewY, newScore, newScore + dist*Path::O_MOD));
				}
			}
		}
	} while (!open.empty());

	//No path found.
	return false;
}

//*****************************************************************************
//Fixed version of A* pathfinding that prefers non-diagonal movements
//and applies a proper operator< for priority queuing
bool CMonster::FindOptimalPath2(
	const UINT wStartX, const UINT wStartY,
	const UINT wGoalX, const UINT wGoalY,
	const bool bPathThroughObstacles)
{
	this->pathToDest.Clear();

	if (wStartX == wGoalX && wStartY == wGoalY)
		return true; //At the destination -- no search needed.

	//Init search.
	this->goal.wX = wGoalX;
	this->goal.wY = wGoalY;
	CDbRoom& curGameRoom = *(this->pCurrentGame->pRoom);
	VERIFY(CMonster::room.Init(curGameRoom.wRoomCols, curGameRoom.wRoomRows));
	VERIFY(CMonster::searchMoves.Init(curGameRoom.wRoomCols, curGameRoom.wRoomRows));

	//larger than the sum of move indexes for a path of maximum possible length
	const UINT STEP_INC = CMonster::room.GetArea() * Path::O_MOD;

	const UINT dwCostThroughObstacle = STEP_INC * CMonster::room.GetArea();
	ASSERT(dwCostThroughObstacle < UINT(-1) / max(curGameRoom.wRoomCols, curGameRoom.wRoomRows)); //avoid potential overflow

	//Push starting node.
	CMonster::room.Add(wStartX, wStartY, 1);  //small number ensures this square will never be visited
	CMonster::searchMoves.Add(wStartX, wStartY, 1+this->wO);
	int dist = nDist(wStartX, wStartY, wGoalX, wGoalY);
	CPathNode2 coord(wStartX, wStartY, 0, dist*STEP_INC);
	priority_queue<CPathNode2> open;
	open.push(coord);

	UINT bestScore = 0;
	int dx, dy, wNewX, wNewY;
	do {
		//Expand best-valued node.
		coord = open.top();
		open.pop();

		//If a questionable path has been found,
		//continue searching until no better path might be found.
		if (bestScore && bestScore < coord.wCost)
			return true; //nothing better -- return found path

		//Move to neighbors in this order (discourage diagonal moves when possible).
		const UINT wNextStepCost = coord.wCost + STEP_INC;
		for (UINT nIndex=0; nIndex<Path::wNumNeighbors; ++nIndex)
		{
			wNewX = coord.wX + (dx = Path::dXs[nIndex]);
			wNewY = coord.wY + (dy = Path::dYs[nIndex]);
			if (!curGameRoom.IsValidColRow(wNewX, wNewY)) continue;
			const UINT wSquareScore = CMonster::room.GetAt(wNewX, wNewY);
			UINT newScore = wNextStepCost +
					nIndex*2; //break ties on movement direction
			const bool bChangedDirection = (1+nIndex != CMonster::searchMoves.GetAt(coord.wX, coord.wY));
			if (bChangedDirection)
				++newScore; //...and whether turning was required to make this step

			//Allow visiting a square if (1) it hasn't been visited, or
			//(2) this is the shortest path to the square so far.
			if (wSquareScore && wSquareScore <= newScore) continue;

			//If monster can move to this square...

			//pathmapping through NPCs flagged to pathmap through is only considered on later moves in the path
			CMonster::calculatingPathmap = coord.wX != wStartX || coord.wY != wStartY;
			const bool bOpenMove = IsOpenMove(coord.wX, coord.wY, dx, dy) || (UINT(wNewX) == wGoalX && UINT(wNewY) == wGoalY);
			CMonster::calculatingPathmap = false;

			if (bOpenMove || bPathThroughObstacles)
			{
				if (!bOpenMove) {
					newScore += dwCostThroughObstacle;
					if (wSquareScore && wSquareScore <= newScore)
						continue;
				}

				CMonster::room.Add(wNewX, wNewY, newScore); //node is now visited
				CMonster::searchMoves.Add(wNewX, wNewY, 1+nIndex); //direction moved to get here

				dist = nDist(wNewX, wNewY, wGoalX, wGoalY);
				if (!dist)
				{
					//Construct path to goal.
					this->pathToDest.Clear();
					while ((UINT)wNewX != wStartX || (UINT)wNewY != wStartY)  //until starting point is returned to
					{
						this->pathToDest.Push(wNewX, wNewY);
						//Reverse the step made to this square.
						const UINT wO = CMonster::searchMoves.GetAt(wNewX, wNewY) - 1;
						ASSERT(wO < Path::wNumNeighbors);
						wNewX -= Path::dXs[wO];
						wNewY -= Path::dYs[wO];
					}
					if (bOpenMove)
						return true; //guaranteed best path
					//otherwise, allow search to continue until no better path might be found
					bestScore = newScore;
				} else {
					//...Add this unvisited node to priority queue.
					open.push(CPathNode2(wNewX, wNewY, newScore, newScore + dist*STEP_INC));
				}
			}
		}
	} while (!open.empty());

	//No path found.
	return false;
}

//*****************************************************************************
bool CMonster::FindOptimalPathToClosestMonster(
//Find the shortest path to (adjacent to) any aggressive monster.
//
//Returns: true if path found, else false
//
//Params:
	const UINT wX, const UINT wY, const CIDSet& monsterTypes) //(in) starting location
{
	//Init search.
	this->pathToDest.Clear();
	CDbRoom& curGameRoom = *(this->pCurrentGame->pRoom);
	VERIFY(CMonster::room.Init(curGameRoom.wRoomCols, curGameRoom.wRoomRows));

	//Push starting node.
	CMonster::room.Add(wX,wY, Path::O_MOD + Path::wNumNeighbors);  //large number ensures this square will never be visited
	CPathNode coord(wX, wY, 0, 1);
	priority_queue<CPathNode> open;
	open.push(coord);

	UINT wGoalDistance = 0;
	int dx, dy, wNewX, wNewY;
	do {
		//Expand best-valued node.
		coord = open.top();
		open.pop();

		//The cost to this node's neighbors is this.
		const UINT wCostPlusOne = coord.wCost/Path::O_MOD + 1;
		//If this cost is greater than a path already found to a goal, skip this node.
		if (wGoalDistance && wCostPlusOne > wGoalDistance)
			continue;

		//Move to neighbors in this order (discourage diagonal moves when possible).
		for (UINT nIndex=0; nIndex<Path::wNumNeighbors; ++nIndex)
		{
			wNewX = coord.wX + (dx = Path::dXs[nIndex]);
			wNewY = coord.wY + (dy = Path::dYs[nIndex]);
			if (!curGameRoom.IsValidColRow(wNewX, wNewY))
				continue;
			const UINT wSquareScore = CMonster::room.GetAt(wNewX,wNewY) / Path::O_MOD;
			//Only allow visiting a square if (1) it hasn't been visited, or
			//(2) this is the shortest path to the square so far.
			if (wSquareScore && wSquareScore <= wCostPlusOne)
				continue;

			//Is an active target monster at this square?
			if (!wGoalDistance)
			{
				CMonster *pMonster = curGameRoom.GetMonsterAtSquare(wNewX, wNewY);
				if (pMonster && monsterTypes.has(pMonster->wType) && pMonster->IsAlive())
				{
					//Goal found.
					wGoalDistance = wCostPlusOne;
					this->goal.wX = wNewX;
					this->goal.wY = wNewY;
					CMonster::room.Add(wNewX, wNewY, wGoalDistance * Path::O_MOD + nIndex);  //last node in path

					//Continue searching for better path to this goal until done.
					break;
				}
			}

			//If monster can move to this square...
			if (IsOpenMove(coord.wX, coord.wY, dx, dy))
			{
				//...Add this unvisited node to priority queue.
				const UINT newScore = wCostPlusOne * Path::O_MOD + nIndex;
				open.push(CPathNode(wNewX, wNewY, newScore, newScore));
				CMonster::room.Add(wNewX, wNewY, newScore);  //node is now visited
			}
		}
	} while (!open.empty());

	if (!wGoalDistance)
		return false; //No path found.

	//Construct path to goal.
	PushPathFromGoal(this->goal.wX, this->goal.wY, wX, wY);
	ASSERT(this->pathToDest.GetSize() == wGoalDistance);
	return true;
}

void CMonster::PushPathFromGoal(UINT endX, UINT endY, UINT startX, UINT startY)
{
	while (endX != startX || endY != startY)  //until starting point is returned to
	{
		this->pathToDest.Push(endX, endY);
		//Reverse the step made to this square.
		const UINT wO = CMonster::room.GetAt(endX, endY) % Path::O_MOD;
		ASSERT(wO < Path::wNumNeighbors);
		endX -= Path::dXs[wO];
		endY -= Path::dYs[wO];
	}
}

//*****************************************************************************
int CMonster::getMinDistance(
//Returns: minimum distance from (wX,wY) to any of the coords in 'dests'
//Params:
	const UINT wX, const UINT wY, //(in) starting location
	const CCoordSet &dests, //(in) possible goal destinations
	ROOMCOORD& closestDest)  //(out) closest goal destination
const
{
	ASSERT(!dests.empty());
	int nSize, nMin = 9999;
	for (CCoordSet::const_iterator dest=dests.begin(); dest!=dests.end(); ++dest)
	{
		nSize = nDist(wX, wY, dest->wX, dest->wY);
		if (nSize < nMin)
		{
			//Closest destination found.
			nMin = nSize;
			closestDest.wX = dest->wX;
			closestDest.wY = dest->wY;
		}
	}
	return nMin;
}

//*****************************************************************************
void CMonster::GetCommandDXY(int nCommand, int &dx, int &dy)
{
	dx=dy=0;

	switch (nCommand)
	{
		case CMD_NW: case CMD_BUMP_NW: dx = dy = -1; break;
		case CMD_N:  case CMD_BUMP_N:  dy = -1; break;
		case CMD_NE: case CMD_BUMP_NE: dx = 1; dy = -1; break;
		case CMD_W:  case CMD_BUMP_W:  dx = -1; break;
		case CMD_E:  case CMD_BUMP_E:  dx = 1; break;
		case CMD_SW: case CMD_BUMP_SW: dx = -1; dy = 1; break;
		case CMD_S:  case CMD_BUMP_S:  dx = 0; dy = 1; break;
		case CMD_SE: case CMD_BUMP_SE: dx = dy = 1; break;
		default: break;
	}
}

//*****************************************************************************
const CMonster* CMonster::GetOwningMonsterConst() const {
	if (IsPiece()) {
		const CMonsterPiece *pPiece = DYN_CAST(const CMonsterPiece*, const CMonster*, this);
		ASSERT(pPiece->pMonster);
		return pPiece->pMonster;
	}
	return this;
}

//*****************************************************************************
bool CMonster::IsConquerable() const
{
	switch (wType) {
		case M_CHARACTER:
		case M_HALPH: case M_HALPH2:
		case M_SLAYER: case M_SLAYER2:
		case M_CITIZEN: case M_ARCHITECT:
		case M_WUBBA: case M_FLUFFBABY:
		case M_FEGUNDO: case M_FEGUNDOASHES:
		case M_STALWART: case M_STALWART2:
		case M_GENTRYII:
		case M_BEETHRO_IN_DISGUISE:
			return false;
		default:
			return !bIsBeethroDouble(wType);
	}
}

//*****************************************************************************
bool CMonster::IsTileObstacle(
//Overridable method to determine if a tile is an obstacle for this monster.
//This method and any overrides, should not evaluate game state or anything
//else besides the tile# to determine if the tile is an obstacle.  If a tile
//is sometimes an obstacle, but not always, IsTileObstacle() should return true,
//and the caller can use extra context to figure it out.  An example of this would
//be arrows, which can be an obstacle to a monster depending on direction of 
//movement.
//
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.  Note each tile# will always be
						//    found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	switch (this->eMovement)
	{
		case WALL:
			//obstacles for WALL movement type (any walls and doors, except blue)
			return
			!(wLookTileNo==T_EMPTY ||
				bIsWall(wLookTileNo) ||
				bIsCrumblyWall(wLookTileNo) ||
				bIsDoor(wLookTileNo) ||
				wLookTileNo==T_NODIAGONAL ||
				wLookTileNo==T_SCROLL ||
				wLookTileNo==T_FUSE ||
				wLookTileNo==T_TOKEN ||
				bIsAnyArrow(wLookTileNo)
			);
		case WATER:
			return !(
				wLookTileNo==T_EMPTY ||
				bIsWater(wLookTileNo) ||
				bIsAnyArrow(wLookTileNo) ||
				wLookTileNo==T_NODIAGONAL ||
				wLookTileNo==T_FUSE ||
				wLookTileNo==T_TOKEN
			);
		default:
			//All the things a monster can step onto.
			return !(
				wLookTileNo==T_EMPTY ||
				bIsFloor(wLookTileNo) ||
				bIsOpenDoor(wLookTileNo) ||
				bIsAnyArrow(wLookTileNo) ||
				bIsPlatform(wLookTileNo) ||
				wLookTileNo==T_NODIAGONAL ||
				wLookTileNo==T_SCROLL ||
				wLookTileNo==T_FUSE ||
				wLookTileNo==T_TOKEN ||
				(IsFlying() && (bIsPit(wLookTileNo) || bIsWater(wLookTileNo))) ||
				(CanWadeInShallowWater() && bIsShallowWater(wLookTileNo)) ||
				(this->wType == M_CHARACTER && //NPCs can walk on stairs and tunnels
						(bIsStairs(wLookTileNo) || bIsTunnel(wLookTileNo)))
			);
	}
}

//*****************************************************************************
bool CMonster::IsOpenMove(const int dx, const int dy) const
//Returns: whether moving from the current position along (dx,dy) is valid
{
	return !(DoesSquareContainObstacle(this->wX + dx, this->wY + dy) ||
				DoesArrowPreventMovement(this->wX, this->wY, dx, dy) ||
				this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy));
}

//*****************************************************************************
bool CMonster::IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const
//More general than IsOpenMove(dx,dy).
//Returns: whether moving from (wX,wY) along (dx,dy) is valid.
{
	return !(DoesSquareContainObstacle(wX + dx, wY + dy) ||
		DoesArrowPreventMovement(wX, wY, dx, dy) ||
		this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(wX, wY, dx, dy));
}

//*****************************************************************************
void CMonster::GetBestMove(
//Given a direction, chooses best possible movement for monster taking
//obstacles into account.
//
//Params:
	int &dx,    //(in/out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy)    //(in/out) Vertical delta (-1, 0, or 1) for same.
const
{
	//Check for out of bounds movement.
	if (this->wX + dx >= this->pCurrentGame->pRoom->wRoomCols ||
		 (int)this->wX + dx < 0) dx = 0;
	if (this->wY + dy >= this->pCurrentGame->pRoom->wRoomRows ||
		 (int)this->wY + dy < 0) dy = 0;
	if (!dx && !dy)
	  return;

	//If a diagonal move is required, see if directly moving to the square will work.
	if (dx && dy)
	{
		if (IsOpenMove(dx,dy))
			return;  //Can take the direct move.
	}

	//See if moving in just the vertical direction will work.
	if (dy)
	{
		if (IsOpenMove(0, dy))
		{
			dx = 0;
			return;
		}
	}

	//See if moving in just the horizontal direction will work.
	if (dx)
	{
		if (IsOpenMove(dx, 0))
		{
			dy = 0;
			return;
		}
	}
	   
	//No open direction has been found -- set movement deltas to zero.
	dx=dy=0;
}

//*****************************************************************************
void CMonster::GetBeelineMovementSmart(
//Given a destination, chooses best possible beeline movement for monster taking
//obstacles into account.
//
//Improvement over GetBeelineMovement's GetBestMove():
//If a diagonal move can't be made, then the uniaxial movement that brings
//the monster closer to the target will be tried and taken first.
//The analog is also performed for axial movement.
//
//Params:
	const UINT wX, const UINT wY, //(in) target destination (usually swordsman)
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy,    //(out) Vertical delta (-1, 0, or 1) for same.
	const bool bSmartAxial, //(in) can monster sidestep obstacles in axial directions
	const bool bSmartFlanking) // if 'bSmartAxial' is true, then also try to flank target that is one tile
	                           // away and inaccessible [default=false]
const
{
	dx = dxFirst = sgn(wX - this->wX);
	dy = dyFirst = sgn(wY - this->wY);

	//Check for out of bounds movement.
	if (this->wX + dx >= this->pCurrentGame->pRoom->wRoomCols ||
		 (int)this->wX + dx < 0) dx = 0;
	if (this->wY + dy >= this->pCurrentGame->pRoom->wRoomRows ||
		 (int)this->wY + dy < 0) dy = 0;
	if (!dx && !dy)
		return;

	//If a diagonal move is required, see if directly moving to the square will work.
	const bool bDiagonal = dx && dy;
	bool bTriedHorizontal = false;
	if (bDiagonal)
	{
		if (IsOpenMove(dx, dy))
			return;  //Can take the direct move.

		//A diagonal move is desired but can't be made.  Determine whether moving in
		//the vertical or horizontal direction will bring the monster closer to the target.
		//Try it first, and choose it if legal.
		if (nDist(this->wX + dx, this->wY, wX, wY) < nDist(this->wX, this->wY + dy, wX, wY))
		{
			//Moving horizontally would get the monster closer to the target.
			//Try it first.
			if (IsOpenMove(dx, 0))
			{
				dy = 0;
				return;
			}
			//Moving horizontally didn't work -- don't try it again below.
			bTriedHorizontal = true;
		}
	}

	//See if moving in just the vertical direction will work.
	if (dy)
	{
		if (IsOpenMove(0, dy))
		{
			dx = 0;
			return;
		}
	}

	//See if moving in just the horizontal direction will work.
	if (dx && !bTriedHorizontal)
	{
		if (IsOpenMove(dx, 0))
		{
			dy = 0;
			return;
		}
	}

	if (!bDiagonal && bSmartAxial)
	{
		//If moving in the desired axial direction doesn't work, then try
		//the analog of the "best possible movement" for diagonal beelining, i.e.
		//Try taking a diagonal step toward the target instead of a straight step
		//(analogous to trying a straight step when a diagonal is desired).
		if ((nDist(this->wX,this->wY,wX,wY) > 1) ||
		//If the target is one tile away and the monster can't move to the
		//target square, then this may cause oscillation.  However, it may also
		//allow the monster to reach the player from another side (e.g.
		//when a force arrow doesn't allow access from one side, but does from another),
		//so allow this movement to be considered if requested.
				bSmartFlanking)
		{
			if (dx)
			{
				ASSERT(!dy);
				dy = -1;
				if (IsOpenMove(dx, dy))
					return;
				dy = 1;
				if (IsOpenMove(dx, dy))
					return;
			} else {
				ASSERT(dy);
				dx = -1;
				if (IsOpenMove(dx, dy))
					return;
				dx = 1;
				if (IsOpenMove(dx, dy))
					return;
			}
		}
	}

	//No open direction has been found -- set movement deltas to zero.
	dx=dy=0;
}

//*****************************************************************************
bool CMonster::MakeThisMove(
//Given a direction, chooses this movement for monster if no obstacle is
//in the way.  Otherwise makes no move.
//
//Returns: whether a (non-stationary) move can be made
//
//Params:
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy)    //(out) Vertical delta (-1, 0, or 1) for same.
const
{
	//Check for out of bounds movement.
	if (this->wX + dx >= this->pCurrentGame->pRoom->wRoomCols ||
		 (int)this->wX + dx < 0) dx = 0;
	if (this->wY + dy >= this->pCurrentGame->pRoom->wRoomRows ||
		 (int)this->wY + dy < 0) dy = 0;
	if (dx == 0 && dy == 0)
	  return false;

	//See if directly moving to square will work.
	if (IsOpenMove(dx, dy))
	{
		dx=dy=0;
		return false;
	}
	return true;
}

//*****************************************************************************
void CMonster::GetAvoidSwordMovement(
//Calculates movement of monsters who try to avoid the target's sword.
//
//Params:
	const UINT wX, const UINT wY, //(in) target (usually player) 
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy)    //(out) Vertical delta (-1, 0, or 1) for same.
const
{
	dx = dy = 0;
	dxFirst = nGetOX(this->wO);
	dyFirst = nGetOY(this->wO);

	//Get target's sword location.
	UINT wSX, wSY;
	bool bHasWeapon;
	if (this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		const CSwordsman& player = this->pCurrentGame->swordsman;
		ASSERT(player.IsTarget());
		bHasWeapon = player.HasWeapon();
		if (bHasWeapon)
		{
			wSX = player.wSwordX;
			wSY = player.wSwordY;
		}
	} else {
		CMonster *pTarget = this->pCurrentGame->pRoom->GetMonsterAtSquare(wX, wY);
		bHasWeapon = pTarget ? pTarget->GetSwordCoords(wSX, wSY) : false;
	}

	//Choose move yielding highest score.
	UINT dwBestScore = 1;
	for (UINT dir = 0; dir < ORIENTATION_COUNT; ++dir)
	{
		const int ndx = nGetOX(dir);
		const int ndy = nGetOY(dir);
		const int x = this->wX + ndx;
		const int y = this->wY + ndy;
		UINT score = 0L; //assume: it can't move here
		if (dir == NO_ORIENTATION || IsOpenMove(ndx, ndy))
		{
			if ((UINT)x == wX && (UINT)y == wY)
				score = 90000L;   //Kill player -- high score
			else if (bHasWeapon && abs((int)(x - wSX)) <= 1 && abs((int)(y - wSY)) <= 1)
				score = 5000L; //Near sword -- low score
			else
			{
				//Closer is better
				const float fDist = DistanceToTarget(wX, wY, x, y);
				score = fDist > 900.0f ? 0 : 90000L - (UINT)(100.0f * fDist);
				ASSERT(score <= 90000L);
			}
		}
		if (score > dwBestScore)
		{
			dwBestScore = score;
			dx = ndx;
			dy = ndy;
		}
	}
	if (dx || dy)
	{
		dxFirst = dx;
		dyFirst = dy;
	}
}

//*****************************************************************************
void CMonster::GetBeelineMovement(
//Gets offsets for a beeline movement of this monster to a target (wX,wY),
//taking obstacles into account.
//
//Params:
	const UINT wX, const UINT wY, //(in) target destination (usually swordsman)
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy)    //(out) Vertical delta (-1, 0, or 1) for same.
const
{
	dx = dxFirst = sgn(wX - this->wX);
	dy = dyFirst = sgn(wY - this->wY);

	GetBestMove(dx, dy);
}

//*****************************************************************************
void CMonster::GetBeelineMovementDumb(
//Given a direction, move for monster if direction is not blocked.
//
//Params:
	const UINT wX, const UINT wY, //(in) target destination (usually swordsman)
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy)    //(out) Vertical delta (-1, 0, or 1) for same.
const
{
	dx = dxFirst = sgn(wX - this->wX);
	dy = dyFirst = sgn(wY - this->wY);

	//Check for out of bounds movement.
	if (this->wX + dx >= this->pCurrentGame->pRoom->wRoomCols ||
			this->wY + dy >= this->pCurrentGame->pRoom->wRoomRows ||
			int(this->wX) + dx < 0 || int(this->wY) + dy < 0)
	{
		//Don't allow sliding along room edge with dumb movement.
		dx = dy = 0;
		return;
	}

	//See if directly moving to the square will work.
	if (dx || dy)
	{
		if (!IsOpenMove(dx, dy))
			//Way is blocked -- set movement deltas to zero.
			dx=dy=0;
	}
	//...else can make move
}

//*****************************************************************************
bool CMonster::GetBrainDirectedMovement(
//Gets offsets for a brain-directed movement of this monster to the swordsman,
//taking obstacles into account.
//
//Brain-directed movement is:
// Follow shortest path to swordsman, even if he is invisible
//
//Returns: whether any valid movement exists
//
//Params:
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy,    //(out) Vertical delta (-1, 0, or 1) for same.
	const MovementIQ movementIQ)   //(in) [default = SmartDiagonalOnly]
const
{
	SORTPOINTS paths;
	ASSERT(this->pCurrentGame->pRoom->pPathMap[this->eMovement]);
	this->pCurrentGame->pRoom->pPathMap[this->eMovement]->
			GetRecPaths(this->wX, this->wY, paths);
	for (UINT i = 0; i < paths.size(); ++i)
	{
		if (paths[i].wX == this->wX && paths[i].wY == this->wY)
			break;   //no advantageous brain-directed path found -- beeline

		const int tdx = (int)paths[i].wX - (int)this->wX;
		const int tdy = (int)paths[i].wY - (int)this->wY;
		ASSERT(abs(tdx) <= 1);
		ASSERT(abs(tdy) <= 1);
		if (IsOpenMove(tdx, tdy))
		{
			dx = dxFirst = tdx;
			dy = dyFirst = tdy;
			return true;
		}

		//1. Stupider monsters only move when brain-directed if the best path option is open.
		//2. Stupider monsters should not get stuck on gentryii-chains-diagonals because those are brain-aware obstacles
		//@TODO: If this additional call ever proves to be a bottleneck we should consider refactoring it out 
		//3. Wall monsters should not beeline when brain-directed moves don't work,
		//as this could take them away from the target, according to the search cost heuristic
		//(causing oscillations).
		if ((
				movementIQ == DirectOnly 
				&& !this->pCurrentGame->pRoom->DoesGentryiiPreventDiagonal(this->wX, this->wY, (int)paths[i].wX, (int)paths[i].wY)
			) || (this->eMovement == WALL && i==paths.size()-1))
		{
			dx = dy = 0;
			dxFirst = tdx;
			dyFirst = tdy;
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool CMonster::GetDirectMovement(
//Gets offsets for standard monster movement to the swordsman, taking
//obstacles into account.
//
//Returns: true if some movement was planned, else false
//
//Params:
	const UINT wX, const UINT wY, //(in) target destination (usually swordsman)
	int &dxFirst,  //(out) Horizontal delta for where the monster would
					//    go if there weren't obstacles.
	int &dyFirst,  //(out) Vertical delta for same.
	int &dx,    //(out) Horizontal delta (-1, 0, or 1) for where monster
					//    can go, taking into account obstacles.
	int &dy,    //(out) Vertical delta (-1, 0, or 1) for same.
	const MovementIQ movementIQ,   //(in) [default = SmartDiagonalOnly]
	const bool bIncludeNonTarget)  //(in) [default = false]
const
{
	if (this->pCurrentGame->bBrainSensesSwordsman && BrainAffects() &&
			//Proximate decoys override brain-directed movement.
			!this->pCurrentGame->pRoom->IsMonsterOfTypeAt(M_DECOY,wX,wY,true))
	{
		if (GetBrainDirectedMovement(dxFirst, dyFirst, dx, dy, movementIQ))
			return true;
	}
	else if (!bIncludeNonTarget && !CanFindSwordsman())
		return false;

	switch (movementIQ)
	{
		case DirectOnly:
			GetBeelineMovementDumb(wX, wY, dxFirst, dyFirst, dx, dy);
			break;
		case SmartDiagonalOnly:
			GetBeelineMovement(wX, wY, dxFirst, dyFirst, dx, dy);
			break;
		case SmarterDiagonalOnly:
			GetBeelineMovementSmart(wX, wY, dxFirst, dyFirst, dx, dy, false);
			break;
		case SmartOmniDirection:
			GetBeelineMovementSmart(wX, wY, dxFirst, dyFirst, dx, dy, true);
			break;
		case SmartOmniFlanking:
			GetBeelineMovementSmart(wX, wY, dxFirst, dyFirst, dx, dy, true, true);
			break;
		case MIQ_Pathfind:
		case MIQ_PathfindOpenOnly:
			ASSERT(!"Unhandled case: Called GetDirectMovement with MIQ_Pathfind*");
			break;
	}
	return true;
}

//*****************************************************************************
CCoordSet CMonster::GetMatchingEndTiles(const vector<CCoord>& coords) const
{
	CCoordSet matching_coords;
	MonsterPieces::const_reverse_iterator pieces = this->Pieces.rbegin();
	vector<CCoord>::const_iterator it=coords.begin();
	for ( ; it!=coords.end() && pieces != this->Pieces.rend(); ++it, ++pieces)
	{
		const CMonsterPiece& piece = **pieces;
		if (it->wX != piece.wX || it->wY != piece.wY)
			break;

		matching_coords.insert(it->wX, it->wY);
	}

	if (pieces == this->Pieces.rend() && it!=coords.end()) {
		if (it->wX == this->wX && it->wY == this->wY)
			matching_coords.insert(it->wX, it->wY);
	}

	return matching_coords;
}

//*****************************************************************************
UINT CMonster::GetProcessSequence() const
//Returns: calculated processing sequence used with CDbRoom::LinkMonster
{
	return this->wProcessSequence;
}

//*****************************************************************************
UINT CMonster::GetOrientationFacingTarget(const UINT wX, const UINT wY) const
//Returns: orientation most directly facing (wX, wY)
{
	//Calc vector to (wX, wY)
	int dx = wX - this->wX, dy = wY - this->wY;
	//If one of the four compass directions is more direct than a diagonal,
	//snap to it.
	const int absDx = abs(dx), absDy = abs(dy);
	if (absDx > 2 * absDy)
		dy = 0;
	else if (absDy > 2 * absDx)
		dx = 0;
	return nGetO(sgn(dx),sgn(dy));
}

//*****************************************************************************
bool CMonster::GetSwordCoords(UINT& wX, UINT& wY) const
//Returns: true if monster has a sword unsheathed, in which case (wX,wY) is set,
//otherwise false
{
	if (bEntityHasSword(this->wType))
	{
		const CArmedMonster *pArmedMonster = DYN_CAST(const CArmedMonster*, const CMonster*, this);
		if (pArmedMonster->HasSword())
		{
			wX = pArmedMonster->GetWeaponX();
			wY = pArmedMonster->GetWeaponY();
			return true;
		}
	} else if (this->wType == M_CHARACTER)
	{
		const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, this);
		if (pCharacter->IsVisible() && pCharacter->HasSword())
		{
			wX = pCharacter->GetWeaponX();
			wY = pCharacter->GetWeaponY();
			return true;
		}
	}
	return false;
}

//*****************************************************************************
bool CMonster::GetTarget(
//Monster chooses target based on these rules:
//1. If Beethro or a stalwart is smelled, move toward the closest one.
//2. If decoy is smelled, move toward closest one.
//3. If swordsman or stalwart is otherwise sensed, move toward closest one.
//4. Else, don't choose a target.
//
//Returns: whether monster is attracted to anything
//
//Params:
	UINT &wX, UINT &wY,  //(out) Where to move to
	const bool bConsiderDecoys) //[default=true]
{
	const CSwordsman& player = this->pCurrentGame->swordsman;
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	UINT wDistance, wClosestMonsterEnemyDistance = NO_TARGET;
	UINT wL1Dist, wClosestL1Dist = NO_TARGET;
	UINT wMyCX, wMyCY;

	// Rule 1.  Is player or NPC Beethro an apparent monster target?
	UINT wSX = player.wX, wSY = player.wY;
	bool bMainTarget = false;
	if (player.IsTarget() &&
			(player.IsVisible() || CanSmellObjectAt(player.wX, player.wY) ||
			this->pCurrentGame->bBrainSensesSwordsman))
	{
		//Player is Beethro or a comparable target that is somehow sensed by this monster,
		//either directly or through a brain.
		bMainTarget = true;
	} else {
		CMonster *pNPCBeethro = room.GetNPCBeethro();
		
		if (pNPCBeethro && pNPCBeethro->IsTarget() && (!pNPCBeethro->IsHiding() ||
				CanSmellObjectAt(pNPCBeethro->wX, pNPCBeethro->wY)))
		{
			//A visible NPC Beethro is sensed by this monster.
			bMainTarget = true;
			wSX = pNPCBeethro->wX;
			wSY = pNPCBeethro->wY;
		}
	}
	if (bMainTarget)
	{
		MyClosestTile(wSX, wSY, wMyCX, wMyCY);
		wClosestMonsterEnemyDistance = nDist(wSX, wSY, wMyCX, wMyCY);
		wClosestL1Dist = nL1Dist(wSX, wSY, wMyCX, wMyCY);
		wX = wSX; //pending target
		wY = wSY;
	}

	//1b. Is a monster enemy close by?
	list<CPlayerDouble*>::const_iterator iSeek;
	for (iSeek = room.monsterEnemies.begin(); iSeek != room.monsterEnemies.end(); ++iSeek)
	{
		const CPlayerDouble& monsterEnemy = *(*iSeek);
		if (!monsterEnemy.IsTarget())
			continue;
		if (monsterEnemy.IsHiding() && !CanSmellObjectAt(monsterEnemy.wX, monsterEnemy.wY))
			continue; //invisible -- ignore
		if (!monsterEnemy.IsMonsterTarget())
			continue; //not a valid target -- ignore

		MyClosestTile(monsterEnemy.wX, monsterEnemy.wY, wMyCX, wMyCY);
		wDistance = nDist(monsterEnemy.wX, monsterEnemy.wY, wMyCX, wMyCY);
		if (wDistance > wClosestMonsterEnemyDistance)
			continue; //further away -- ignore

		//Break ties using L1 distance.
		wL1Dist = nL1Dist(monsterEnemy.wX, monsterEnemy.wY, wMyCX, wMyCY);
		if (wDistance == wClosestMonsterEnemyDistance && wL1Dist > wClosestL1Dist)
			continue;

		//Attracted to closest stalwart.
		wClosestMonsterEnemyDistance = wDistance;
		wClosestL1Dist = wL1Dist;
		wX = monsterEnemy.wX;
		wY = monsterEnemy.wY;
	}
	if (wClosestMonsterEnemyDistance <= DEFAULT_SMELL_RANGE)
		//This entity takes precedence over decoys even if decoys are in smell range.
		return true; 

	// No primary targets are in smelling range.
	// Rule 2.  Is a decoy in smelling range?
	if (bConsiderDecoys)
	{
		UINT nClosestDecoyDistance = NO_TARGET, wClosestDecoyL1Dist = NO_TARGET;
		if (this->pCurrentGame->swordsman.wAppearance == M_DECOY)
		{
			//If player is role-playing a decoy in smell range, consider him first.
			MyClosestTile(player.wX, player.wY, wMyCX, wMyCY);
			wDistance = nDist(player.wX, player.wY, wMyCX, wMyCY);
			if (wDistance <= DEFAULT_SMELL_RANGE)
			{
				nClosestDecoyDistance = wDistance;
				wClosestDecoyL1Dist = nL1Dist(player.wX, player.wY, wMyCX, wMyCY);
				wX = player.wX; //pending target
				wY = player.wY;
			}
		}

		//Is a decoy close by?
		for (iSeek = room.Decoys.begin(); iSeek != room.Decoys.end(); ++iSeek)
		{
			const CPlayerDouble& decoy = *(*iSeek);
			MyClosestTile(decoy.wX, decoy.wY, wMyCX, wMyCY);
			wDistance = nDist(decoy.wX, decoy.wY, wMyCX, wMyCY);
			if (wDistance > DEFAULT_SMELL_RANGE)
				continue; //decoys not in smell range are ignored

			if (wDistance > nClosestDecoyDistance)
				continue; //further away -- ignore

			//Break ties using L1 distance.
			wL1Dist = nL1Dist(decoy.wX, decoy.wY, wMyCX, wMyCY);
			if (wDistance == nClosestDecoyDistance && wL1Dist > wClosestDecoyL1Dist)
				continue;

			//Attracted to closest decoy.
			nClosestDecoyDistance = wDistance;
			wClosestDecoyL1Dist = wL1Dist;
			wX = decoy.wX;
			wY = decoy.wY;
		}
		if (nClosestDecoyDistance <= DEFAULT_SMELL_RANGE)
			return true; //This decoy takes precedence over player and monster enemies further away.
	}

	// Rule 3. No target is in smelling range.
	//Check for monster enemies/Beethro/player target type further away.
	//Closest one takes precedence.
	if (bMainTarget)
	{
		const bool bSensesTarget =
			!player.IsTarget() || //this implies the player is not role-playing Beethro, but an NPC Beethro is visible
			player.IsVisible() ||  //otherwise the player is Beethro and visible
			this->pCurrentGame->bBrainSensesSwordsman;   //else brain smells proximate invisible player Beethro
		if (bSensesTarget)
		{
			//Can move toward non-smelled, but somehow sensed, target.
			MyClosestTile(wSX, wSY, wMyCX, wMyCY);
			wDistance = nDist(wSX, wSY, wMyCX, wMyCY);
			wL1Dist = nL1Dist(wSX, wSY, wMyCX, wMyCY);
			if (wDistance < wClosestMonsterEnemyDistance ||
				(wDistance == wClosestMonsterEnemyDistance && wL1Dist < wClosestL1Dist))
			{
				//Closer than any other influencing targets.
				wX = wSX;
				wY = wSY;
				return true;
			}
		}
	}

	if (wClosestMonsterEnemyDistance != NO_TARGET)
		return true; //a monster enemy was chosen as a pending target above -- it is the target

	// Rule 4. No influencing decoys, no monster enemies, and invisible player can't be smelled.
	return false;
}

//*****************************************************************************
bool CMonster::HasSwordAt(const UINT wX, const UINT wY) const
//Returns: whether this monster has a sword at (x,y)
{
	UINT wSX, wSY;
	if (GetSwordCoords(wSX, wSY))
		return wX == wSX && wY == wSY;
	return false;
}

//*****************************************************************************
bool CMonster::IsMonsterTarget() const
//Returns: Is the monster a valid target for other monsters to attack?
{
	return bIsMonsterTarget(this->wType);
}

//*****************************************************************************
bool CMonster::IsNextToSwordsman()
//Returns: Is the monster adjacent to, or in the same square as the swordsman?
const
{
	UINT wSX, wSY;
	if (!this->pCurrentGame->GetSwordsman(wSX, wSY))
		return false;
	return abs(static_cast<int>(this->wX - wSX)) <= 1 &&
			abs(static_cast<int>(this->wY - wSY)) <= 1;
}

//*****************************************************************************
bool CMonster::IsObjectAdjacent(
//Returns: whether object is adjacent to monster.
//
//Params:
	const UINT wObject,
	UINT& wX, UINT& wY)  //(out) location of first adjacent object found
const
{
	ASSERT(wObject < TOTAL_TILE_COUNT);
	ASSERT(this->pCurrentGame);
	CDbRoom *pRoom = this->pCurrentGame->pRoom;
	ASSERT(pRoom);

	const UINT wLayer = TILE_LAYER[wObject];
	ASSERT(wLayer <= 2);

	int i,j;
	wY = this->wY - 1;
	for (j=-1; j!=2; ++j, ++wY)
	{
		wX = this->wX - 1;
		for (i=-1; i!=2; ++i, ++wX)
		{
			if ((i || j) && pRoom->IsValidColRow(wX, wY))
			{
				switch (wLayer)
				{
					case 0: if (pRoom->GetOSquare(wX, wY) == wObject) return true;
						break;
					case 1: if (pRoom->GetTSquare(wX, wY) == wObject) return true;
						break;
					case 2:
					{
						CMonster *pMonster = pRoom->GetMonsterAtSquare(wX, wY);
						if (pMonster && pMonster->wType+M_OFFSET == wObject) return true;
					}
					break;
				}
			}
		}
	}
	return false;
}

//*****************************************************************************
bool CMonster::IsOnSwordsman()
//Is the monster in the same square as Beethro?
const
{
	const CSwordsman& player = this->pCurrentGame->swordsman;
	//Player is Beethro or an equivalent target.
	if (this->wX == player.wX && this->wY == player.wY && player.IsInRoom())
		return true;

	//Check whether an NPC is posing as the swordsman.
	CMonster *pNPCBeethro = this->pCurrentGame->pRoom->GetNPCBeethro();
	if (pNPCBeethro)
	{
		if (this->wX == pNPCBeethro->wX && this->wY == pNPCBeethro->wY)
			return true;
	}

	return false;
}

//*****************************************************************************
bool CMonster::IsWading() const
{
	if (!this->pCurrentGame)
		return false;
	if (IsFlying())
		return false;
	const UINT oTile = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	return bIsWater(oTile);
}

//*****************************************************************************
void CMonster::MyClosestTile(
//This routine outputs the location of the part of this monster
//closest to the given coordinates, that is movable/controllable.
//
//Params:
	const UINT /*wX*/, const UINT /*wY*/, // (in) target co-ordinates
	UINT &wMyCX, UINT &wMyCY)	// (out) coordinates of my closest movable piece
const
{
	//Output the monster's coords.
	//This applies to serpents as well, because their head is the only part
	//of them that is responsible for targeting/smelling enemies and moving.
	wMyCX = this->wX;
	wMyCY = this->wY;
}

//*****************************************************************************
bool CMonster::CanFindSwordsman()
//Overridable method for determining if a monster can find the swordsman on
//its own.  
//Currently used by movement routines to see if monster will attempt to move
//towards the swordsman or not (w/o help of a brain).
//
//Sensing proximate decoys also influence the monster.
//
//Returns:
//True if monster can find the swordsman, false if not.
const
{
	ASSERT(this->pCurrentGame);
	UINT wSX, wSY;
	const bool bSwordsmanInRoom = this->pCurrentGame->GetSwordsman(wSX, wSY);
	if (!bSwordsmanInRoom)
		return SensesTarget(); //no Beethro in room, but an alternate target might be sensed

	//If swordsman is visible, monster can see him.
	if (this->pCurrentGame->swordsman.IsTarget())
	{
		if (this->pCurrentGame->swordsman.IsVisible())
			return true;
	} else {
		//NPC Beethro always visible and sensed.
		return true;
	}

	//Otherwise, monster can smell him if within range.
	if (CanSmellObjectAt(wSX, wSY))
		return true;

	//Otherwise, monster might sense any alternate monster targets.
	return SensesTarget();
}

//*****************************************************************************
bool CMonster::CanSmellObjectAt(
//Returns: whether (wX,wY) is within smelling range.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coordinates of object.
const
{
	UINT wMyCX, wMyCY;
	MyClosestTile(wX, wY, wMyCX, wMyCY); //get closest part of me that might smell this tile
	return nDist(wX, wY, wMyCX, wMyCY) <= DEFAULT_SMELL_RANGE;
}

//*****************************************************************************
bool CMonster::OnAnswer(
//Overridable method for responding to an answer given by player to a question asked by the
//monster.
//
//Params:
	int nCommand,        //(in)   CMD_YES or CMD_NO.
	CCueEvents &/*CueEvents*/) //(out)  Add cue events if appropriate.
//
//Returns:
//True if any cue events were added, false if not.
{
	ASSERT(nCommand == CMD_YES || nCommand == CMD_NO);

	return false;
}

//*****************************************************************************
void CMonster::Say(
//Create a cue event that makes the monster "say" something.
//
//Params:
	MESSAGE_ID eMessageID,     //(in)   Message to say.  One of the MID_* constants.
	CCueEvents &CueEvents)    //(out)   One new CID_MonsterSpoke cue event will be added.
const
{
	ASSERT(eMessageID > (MESSAGE_ID)0);

	//Create the monster message.
	CMonsterMessage *pNewMessage = 
			new CMonsterMessage(MMT_OK, eMessageID, const_cast<CMonster *>(this));
	
	//Add cue event with attached monster message.
	CueEvents.Add(CID_MonsterSpoke, pNewMessage, true);
}

//*****************************************************************************
void CMonster::AskYesNo(
//Create a cue event that makes the monster "ask" something.
//
//Params:
	MESSAGE_ID eMessageID,    //(in) Message for question to ask.  One of the MID_* constants.
	CCueEvents &CueEvents)    //(out)   One new CID_MonsterSpoke cue event will be added.
const
{
	ASSERT(eMessageID > (MESSAGE_ID)0);

	//Create the monster message.
	CMonsterMessage *pNewMessage = 
		new CMonsterMessage(MMT_YESNO, eMessageID, const_cast<CMonster *>(this));
		
	//Add cue event with attached monster message.
	CueEvents.Add(CID_MonsterSpoke, pNewMessage, true);
}

//*****************************************************************************
bool CMonster::MakeSlowTurn(const UINT wDesiredO)
//Make a 45 degree (1/8th rotation) turn toward the desired orientation.
//
//Returns: whether a turn was made
{
	if (this->wO == wDesiredO) return false; //no turn needed
	ASSERT(wDesiredO != NO_ORIENTATION);
	ASSERT(this->wO != NO_ORIENTATION);

	//Determine whether turning clockwise or counter-clockwise would
	//reach the desired orientation more quickly.
	this->wO = RotationalDistanceCO(wDesiredO) <= 4 ? nNextCO(this->wO) : nNextCCO(this->wO);
	return true;
}

//*****************************************************************************
void CMonster::MakeStandardMove(
//Move monster and check whether it killed the swordsman.
//
//Params:
	CCueEvents &CueEvents,  //(out)  Add cue events if appropriate.
	const int dx, const int dy)   //(in)   Movement offset.
{
	if (!dx && !dy)
		return;

	//Check whether an NPC Beethro is going to be killed by this move.
	//Speed optimization (this avoids calling the slower IsOnSwordsman).
	NPCBeethroDieCheck(this->wX + dx, this->wY + dy, CueEvents);

	Move(this->wX + dx, this->wY + dy, &CueEvents);

	//If on the player then kill him.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsTarget())
	{
		//Player is Beethro or an equivalent target.
		if (this->wX == player.wX && this->wY == player.wY)
		{
			CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
			pGame->SetDyingEntity(&player, this);
			CueEvents.Add(CID_MonsterKilledPlayer, this);
		}
	}
}

//*****************************************************************************
void CMonster::Move(
//Moves the monster to a new square in the room.
//
//Params:
	const UINT wDestX, const UINT wDestY,  //(in)   Destination to move to.
	CCueEvents* pCueEvents) //[default=NULL]
{
	//If a stalwart is at the destination square, it dies.
	//If an NPC is playing the role of Beethro at the destination square, it also dies (i.e. game ends).
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	CMonster *pMonster = room.GetMonsterAtSquare(wDestX,wDestY);
	bool bFluffPoison = false;
	if (pMonster)
	{
		ASSERT(pCueEvents);

		if (pMonster->IsPushableByBody() && this->CanPushObjects()){
			pMonster->PushInDirection(sgn(wDestX - this->wX), sgn(wDestY - this->wY), false, *pCueEvents);
		}
		else 
		{
			switch (pMonster->wType)
			{
			case M_CHARACTER:
			{
				const bool bCanStrike = bIsStalwart(pMonster->GetIdentity()) ||
					this->CanDaggerStep(pMonster->wType, false);
				ASSERT(bIsSmitemaster(pMonster->GetIdentity()) ||
					pMonster->GetIdentity() == M_BEETHRO_IN_DISGUISE ||
					bCanStrike);
				if (bCanStrike)
				{
					pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
					room.KillMonster(pMonster, *pCueEvents, false, this); //will return false if it's a critical NPC
					pMonster->SetKillInfo(GetOrientation(this->wX, this->wY, wDestX, wDestY));
				}
			}
				break;
			case M_STALWART: case M_STALWART2:
				pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
				VERIFY(room.KillMonster(pMonster, *pCueEvents, false, this));
				pMonster->SetKillInfo(GetOrientation(this->wX, this->wY, wDestX, wDestY));
				break;
			case M_TEMPORALCLONE:
				pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
				room.KillMonster(pMonster, *pCueEvents, false, this); //will return false
				pMonster->SetKillInfo(GetOrientation(this->wX, this->wY, wDestX, wDestY));
				break;
			case M_FLUFFBABY:
				room.KillMonster(pMonster, *pCueEvents, false, this);
				bFluffPoison = true;
				break;
			default:
				ASSERT(this->CanDaggerStep(pMonster->wType, false));
				ASSERT(bIsStalwart(this->wType) || this->wType == M_GUARD || this->wType == M_SLAYER || this->wType == M_SLAYER2);

				pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
				VERIFY(room.KillMonster(pMonster, *pCueEvents, false, this));
				pMonster->SetKillInfo(GetOrientation(this->wX, this->wY, wDestX, wDestY));
				break;
			}

			//Remove it from the room's monster array to avoid a pointer overwrite assertion.
			room.RemoveMonsterFromTileArray(pMonster);
		}
	}

	if (pCueEvents && IsWading())
		pCueEvents->Add(CID_Wade, new CCoord(this->wX, this->wY));

	//Update monster array.
	room.MoveMonster(this,wDestX,wDestY);

	//Set new coords.
	this->wX = wDestX;
	this->wY = wDestY;

	//If a Fluff puff was stepped on, then an attack occurs here.
	if (bFluffPoison)
		room.ProcessPuffAttack(*pCueEvents,wDestX,wDestY);
	//Check for stepping on pressure plate.
	if (pCueEvents && CanPressPressurePlates() && room.GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
		room.ActivateOrb(wDestX, wDestY, *pCueEvents, OAT_PressurePlate);
}

//*****************************************************************************
bool CMonster::NPCBeethroDieCheck(const UINT wX, const UINT wY, CCueEvents& CueEvents)
//Returns: whether monster moving to this square kills an NPC Beethro
{
	CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(wX, wY);
	if (!pMonster)
		return false;
	const UINT wIdentity = pMonster->GetIdentity();
	if (bIsSmitemaster(wIdentity) || wIdentity == M_BEETHRO_IN_DISGUISE)
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->SetDyingEntity(pMonster, this);
		CueEvents.Add(CID_NPCBeethroDied, this);
		pMonster->bAlive = false;
		return true;
	}
	return false;
}

//*****************************************************************************
UINT CMonster::RotationalDistance(const UINT wTargetO) const
//Returns: How many rotational units this is from the target orientation.
{
	const UINT wRotDist = RotationalDistanceCO(wTargetO);
	return wRotDist <= 4 ? wRotDist : 8 - wRotDist;
}

//*****************************************************************************
UINT CMonster::RotationalDistanceCO(const UINT wTargetO) const
//Returns: How many rotational units in clockwise direction
//this is to the target orientation.
{
	ASSERT(IsValidOrientation(wTargetO));
	if (this->wO == NO_ORIENTATION || wTargetO == NO_ORIENTATION)
		return 0;

	UINT wO = this->wO, wTurns = 0;
	while (wO != wTargetO)
	{
		wO = nNextCO(wO);
		++wTurns;
		ASSERT(wTurns < 8);
	}
	return wTurns;
}

//*****************************************************************************
void CMonster::Save(
//Places monster object member vars into database view.
//
//Params:
	const c4_RowRef &MonsterRowRef,     //(in/out) Open row ref to fill.
	const bool /*bSaveScript*/) //currently for NPCs only [default=true]
{
	//Prepare monster data.
	UINT dwSettingsSize;
	BYTE *pbytSettingsBytes = this->ExtraVars.GetPackedBuffer(dwSettingsSize);
	ASSERT(pbytSettingsBytes);
	c4_Bytes SettingsBytes(pbytSettingsBytes, dwSettingsSize);

	c4_View PiecesView;
	const UINT wNumPieces = this->Pieces.size();
	if (wNumPieces)
	{
		PiecesView.SetSize(wNumPieces);
		MonsterPieces::const_iterator piece = this->Pieces.begin();
		for (UINT wPieceI=0; wPieceI < wNumPieces; ++wPieceI)
		{
			c4_RowRef pieceRow = PiecesView[wPieceI];
			CMonsterPiece *pMPiece = *(piece++);
			p_Type(pieceRow) = pMPiece->wTileNo;
			p_X(pieceRow) = pMPiece->wX;
			p_Y(pieceRow) = pMPiece->wY;
		}
	}

	p_Type(MonsterRowRef) = this->wType;
	p_X(MonsterRowRef) = this->wX;
	p_Y(MonsterRowRef) = this->wY;
	p_O(MonsterRowRef) = this->wO;
	p_IsFirstTurn(MonsterRowRef) = this->bIsFirstTurn;
	p_ProcessSequence(MonsterRowRef) = this->wProcessSequence;
	p_ExtraVars(MonsterRowRef) = SettingsBytes;
	p_Pieces(MonsterRowRef) = PiecesView;

	delete[] pbytSettingsBytes;
}

//*****************************************************************************
bool CMonster::SensesTarget() const
//Returns: true if a non-Beethro monster target is influencing the monster
{
	//Stunned monsters can't sense anything
	if (this->IsStunned())
		return false;

	//Is the player posing as a monster target type?
	if (bIsMonsterTarget(this->pCurrentGame->swordsman.wAppearance))
	{
		if (this->pCurrentGame->swordsman.IsTarget() && CanSmellObjectAt(this->pCurrentGame->swordsman.wX, this->pCurrentGame->swordsman.wY))
			return true;
	}

	//Is a monster enemy in the room?
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	list<CPlayerDouble*>::const_iterator iSeek;
	for (iSeek = room.monsterEnemies.begin(); iSeek != room.monsterEnemies.end(); ++iSeek)
	{
		const CPlayerDouble& monsterEnemy = *(*iSeek);
		if (!monsterEnemy.IsTarget())
			continue;
		if (monsterEnemy.IsHiding() && !CanSmellObjectAt(monsterEnemy.wX, monsterEnemy.wY))
			continue; //invisible -- ignore
		if (!monsterEnemy.IsMonsterTarget())
			continue; //not a valid target -- ignore

		return true;
	}

	//Is a decoy within range of influence?
	for (iSeek = room.Decoys.begin(); iSeek != room.Decoys.end(); ++iSeek)
	{
		if (CanSmellObjectAt((*iSeek)->wX, (*iSeek)->wY))
			return true;
	}

	return false;	//none found
}

//*****************************************************************************
bool CMonster::CanDaggerStep(const UINT wMonsterType, const bool bIgnoreSheath) const
//Returns: true if monster is capable of killing the target monster with a "dagger step"
{
	//You can't "dagger step" without a dagger
	//Don't use HasSword() because we need to check for
	//  OTile-specific sheathing separately
	if (this->GetWeaponType() != WT_Dagger || !bEntityHasSword(this->GetIdentity()))
		return false;

	//Only check for Oremites/Shallow Water sheathing before movement
	if (!bIgnoreSheath && !this->HasSword())
		return false;

	// These monsters can't be dagger stepped because they are either invulnerable or leave a dead body
	switch (wMonsterType)
	{
	case M_CITIZEN: case M_ARCHITECT: case M_ROCKGOLEM:
	case M_CONSTRUCT: case M_WUBBA:
		return false;
	default:
		return true;
	}
}
