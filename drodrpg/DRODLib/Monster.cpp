// $Id: Monster.cpp 9474 2010-12-29 02:06:07Z TFMurphy $

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
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "PlayerDouble.h"
#include "Character.h"
#include "Combat.h"
#include "Swordsman.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>

#include <math.h>

#define HPStr "HP"
#define ATKStr "ATK"
#define DEFStr "DEF"
#define GOLDStr "GOLD"
#define XPStr "XP"

#define EggSpawnStr "EggSpawn"

CCoordIndex_T<USHORT> CMonster::room;
CCoordIndex CMonster::swordsInRoom;

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
	, bIsFirstTurn(false)
	, eMovement(eMovement)
	, bAlive(true)
	, HP(0), ATK(0), DEF(0), GOLD(0), XP(0)
	, bEggSpawn(false)
	, pNext(NULL), pPrevious(NULL)
	, pCurrentGame(NULL)
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
	this->pPrevious=this->pNext=NULL;
	this->wType=this->wX=this->wY=this->wO=this->wProcessSequence=0;
	this->wPrevX = this->wPrevY = this->wPrevO = 0;
	this->bIsFirstTurn=false;
	this->bAlive = true;
	this->ATK = this->DEF = this->GOLD = this->HP = this->XP = 0;
	this->bEggSpawn = false;
	this->ExtraVars.Clear();
	while (this->Pieces.size())
	{
		delete this->Pieces.back();
		this->Pieces.pop_back();
	}
}

//*****************************************************************************
void CMonster::ReflectX(CDbRoom *pRoom)
//Update monster's coords and orientation when the room is reflected horizontally
//(about the x-axis).
{
	ASSERT(pRoom);
	const UINT wNewX = (pRoom->wRoomCols-1) - this->wX;
	if (this == pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX, this->wY)])
		std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(wNewX,this->wY)]);

	for (list<CMonsterPiece*>::const_iterator piece=this->Pieces.begin();
			piece != this->Pieces.end(); ++piece)
	{
		ReflectPieceX(pRoom, *piece);
	}

	this->wX = wNewX;
	this->wO = nGetO(-nGetOX(this->wO),nGetOY(this->wO));
}

//*****************************************************************************
void CMonster::ReflectY(CDbRoom *pRoom)
//Update monster's coords and orientation when the room is reflected vertically
//(about the y-axis).
{
	ASSERT(pRoom);
	const UINT wNewY = (pRoom->wRoomRows-1) - this->wY;
	if (this == pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX, this->wY)])
		std::swap(
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,this->wY)],
			pRoom->pMonsterSquares[pRoom->ARRAYINDEX(this->wX,wNewY)]);

	for (list<CMonsterPiece*>::const_iterator piece=this->Pieces.begin();
			piece != this->Pieces.end(); ++piece)
	{
		ReflectPieceY(pRoom, *piece);
	}

	this->wY = wNewY;
	this->wO = nGetO(nGetOX(this->wO),-nGetOY(this->wO));
}

//*****************************************************************************
void CMonster::RotateClockwise(CDbRoom *pRoom)
//Update monster's coords and orientation when the room is rotated 90 degrees clockwise.
{
	ASSERT(pRoom);
	const UINT wNewX = (pRoom->wRoomRows-1) - this->wY;
	const UINT wNewY = this->wX;

	pRoom->pMonsterSquares[pRoom->ARRAYINDEX(wNewX,wNewY)] = this;

	for (list<CMonsterPiece*>::const_iterator piece=this->Pieces.begin();
			piece != this->Pieces.end(); ++piece)
	{
		RotatePieceClockwise(pRoom, *piece);
	}

	this->wX = wNewX;
	this->wY = wNewY;
	this->wO = nNextCO(nNextCO(this->wO));
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
void CMonster::RotatePieceClockwise(CDbRoom *pRoom, CMonsterPiece *pPiece)
{
	pPiece->RotateClockwise(pRoom);
}

//*****************************************************************************
void CMonster::ResetCurrentGame()
//Resets the current game pointer.  (Called only from the room editor.)
{
	this->pCurrentGame = NULL;
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

	//Assign predefined monster base stats.
	if (this->wType < MONSTER_TYPES && !this->HP) //only if HP hasn't been set yet
	{
		this->ATK = MON_ATK[this->wType];
		this->DEF = MON_DEF[this->wType];
		this->GOLD = MON_GOLD[this->wType];
		this->XP = MON_GOLD[this->wType]; //currently, default XP is the same as gold
		SetHP();
	}
}

//*****************************************************************************
void CMonster::SetHP()
//Sets the monster's HP to its base amount,
//multiplied by monsterHPmult, if applicable.
{
	this->HP = MON_HP[this->wType];

	//Apply monsterHPmult to base monster types once, when the game pointer is
	//first set.  This prevents compounding the multiplier when modifying
	//and then re-querying the monster's HP during play.
	if (this->pCurrentGame && this->HP)
	{
		ASSERT(this->pCurrentGame->pPlayer);
		const float fMult = this->pCurrentGame->GetTotalStatModifier(ScriptVars::MonsterHP);
		this->HP = UINTBounds(this->HP * fMult);
		if (!this->HP)
			this->HP = 1; //don't let a small multiplier reduce HP to zero
	}
}

//*****************************************************************************
void CMonster::SetKillInfo(const UINT wKillDirection)
//Sets values for display of monster death.
{
	this->wProcessSequence = IsValidOrientation(wKillDirection) ?
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
	if (IsValidOrientation(wNewO) && wNewO != NO_ORIENTATION || !HasOrientation())
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
bool CMonster::OnStabbed(
//Overridable method to process the effects of a monster being stabbed.  If derived class
//uses this method, then a cue event of monster being killed will be returned.
//
//Params:
	CCueEvents &CueEvents,  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
	const UINT /*wX*/, const UINT /*wY*/) //(in) what square of the monster was stabbed [default = (-1,-1)]
//
//Returns: whether monster was killed
{
	//Monster dies.
	CueEvents.Add(CID_MonsterDiedFromStab, this);
	return true;
}

//*****************************************************************************
bool CMonster::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	ASSERT(IsAlive());

	if (IsFlying()) //airborne monsters are not damaged by hot tiles
		return false;

	//Touching a hot tile causes fractional damage.
	//But standing on crate negates the effect.
	if (this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_HOT &&
		this->pCurrentGame->pRoom->GetTSquare(this->wX, this->wY) != T_CRATE)
	{
		if (DamagedByHotTiles() && IsDamageableAt(this->wX, this->wY))
		{
			const int damageVal = int(this->pCurrentGame->pPlayer->st.hotTileVal);
			if (damageVal != 0) //display effect only if damage can occur
				CueEvents.Add(CID_MonsterBurned, this);
			return Damage(CueEvents, damageVal);
		}
	}

	//Monster pieces are not harmed.
/*
	CMonsterPiece *pPiece;
	list<CMonsterPiece*>::const_iterator piece=this->Pieces.begin();
	while (piece != this->Pieces.end())
	{
		pPiece = *piece++;
		const UINT wX = pPiece->wX, wY = pPiece->wY;
		if (this->pCurrentGame->pRoom->GetOSquare(wX, wY) == T_HOT)
		{
			if (DamagedByHotTiles() && IsDamageableAt(wX, wY))
			{
				CueEvents.Add(CID_MonsterBurned, pPiece);
				return Damage(CueEvents, int(this->pCurrentGame->pPlayer->st.hotTileVal));
			}
		}
	}
*/

	return false;
}

//*****************************************************************************
bool CMonster::Damage(
//Damage monster a certain amount.
//
//Params:
	CCueEvents& CueEvents,
	int damageVal) //a negative value indicates flat-rate damage;
	               //a positive value indicates a percentage of current HP
//
//Returns: whether monster died
{
	UINT delta;
	if (damageVal < 0)
	{
		//Flat-rate damage.
		delta = -damageVal;
	} else {
		//Fractional damage (percent HP lost).
		const float fDelta = this->HP * (damageVal / 100.0f);
		if (fDelta <= 0.0f)
			return false; //no damage
		delta = UINT(ceil(fDelta)); //ceiling function
	}
	if (delta >= this->HP)
		this->HP = 0;
	else
		this->HP -= delta;

	CueEvents.Add(CID_EntityAffected,
			new CCombatEffect(this, CET_HARM, delta), true);

	return !this->HP; //monster died?
}

//*****************************************************************************
float CMonster::DistanceToTarget(
// Figures out distance to the target (usually swordsman) from a square.
//
//Params:
	const UINT wX, const UINT wY, //(in)  Target location
	const UINT x, const UINT y)   //(in)  Coords to check (i.e. monster location)
const
{
/*
   if (bUseBrainDistance && this->pCurrentGame->bBrainSensesSwordsman)
	{
		const SQUARE square = this->pCurrentGame->pRoom->pPathMap[this->eMovement]->
				GetSquare(this->wX, this->wY);
		if (square.eState == ok && square.dwTargetDist > 2 &&
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
*/

	//Calculate Euclidean distance.
	const int xd = x - wX;
	const int yd = y - wY;
	return static_cast<float>(sqrt(xd * xd + yd * yd));
}

//*****************************************************************************
UINT CMonster::DistToSwordsman(
	const bool bIncludeNonTarget) //Find swordsman even if not target [default=false]
const
//Returns: pathmap distance to player if available, else L(inf) distance to player
{
/*	CPathMap *pPathMap = this->pCurrentGame->pRoom->pPathMap[this->eMovement];
	if (bUsePathmap && pPathMap)
	{
		const UINT dwStepsToPlayer = pPathMap->GetSquare(this->wX, this->wY).dwSteps;
		if (dwStepsToPlayer < this->pCurrentGame->pRoom->CalcRoomArea())   //valid?
			return dwStepsToPlayer;
	}
*/
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
	ASSERT(wCol != this->wX || wRow != this->wY);

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

		//Monster can move into tunnel entrance if player is there.
		if (bIsTunnel(wLookTileNo))
		{
			if (this->pCurrentGame->IsPlayerAt(wCol, wRow))
				goto CheckMonster;
		}

		//No special handling was performed.  Treat it as an obstacle.
		return true;
	}

CheckMonster:
	//Can only move onto attackable monsters.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && !pMonster->IsAttackableTarget())
		return true;

	//Check for player at tile.
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
	if (
			player.IsInRoom() && //!player.IsTarget() && player.IsStabbable()) &&
			player.wX == wCol && player.wY == wRow)
		return true;
	
	//Check for player's sword at square.
	if (this->pCurrentGame->IsPlayerSwordAt(wCol, wRow))
		return true;

	//Check for monster sword at square.
	if (room.IsMonsterSwordAt(wCol, wRow, this))
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
void CMonster::ExportXML(string &str) const
//Returns: string containing XML text describing room with this ID
{
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
		for (list<CMonsterPiece*>::const_iterator iter=this->Pieces.begin();
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
}
#undef STARTVPTAG
#undef PROPTAG
#undef ENDVPTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef INT32TOSTR

//*****************************************************************************************
/*
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
		if (DoesSquareContainObstacle(wX, wY))
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
*/
//*****************************************************************************
bool CMonster::FindOptimalPathTo(
//Find the shortest path to any of the given destinations using A* search.
//
//Returns: true if path found, else false
//
//Params:
	const UINT wX, const UINT wY, //(in) starting location
	const CCoordSet &dests, //(in) possible goal destinations
	const bool bAdjIsGood)  //[default=true] whether arriving at any square
									//adjacent to a destination is a sufficient solution
{
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
	static const UINT wNumNeighbors = 8;
	static const int dXs[wNumNeighbors] = { 0,  1,  0, -1,  1,  1, -1, -1};
	static const int dYs[wNumNeighbors] = {-1,  0,  1,  0, -1,  1,  1, -1};
//	static const UINT O_MOD = 16;   //each cell in CMonster::room stores (dist * O_MOD + direction from previous square)

	CMonster::room.Add(wX,wY, 3*wNumNeighbors);  //large number ensures this square will never be visited
	CPathNode coord(wX, wY, 0, dist);
	priority_queue<CPathNode> open;
	open.push(coord);

	int dx, dy, wNewX, wNewY;
	do {
		//Expand best-valued node.
		coord = open.top();
		open.pop();

		//Move to neighbors in this order (discourage diagonal moves when possible).
		const UINT wCostSoFar = coord.wCost / wNumNeighbors;
		UINT wCostToNextTile = wCostSoFar + 2; //cost: axial=2, diagonal=3 -- this discourages diagonal moves
		for (UINT nIndex=0; nIndex<wNumNeighbors; ++nIndex)
		{
			if (nIndex == 4) //the rest of the iterations cover diagonal moves, which we give a slightly higher cost than axial moves to discourage them unless they're actually better
				++wCostToNextTile;

			wNewX = coord.wX + (dx = dXs[nIndex]);
			wNewY = coord.wY + (dy = dYs[nIndex]);
			if (!curGameRoom.IsValidColRow(wNewX, wNewY))
				continue;
			const UINT wSquareScore = CMonster::room.GetAt(wNewX,wNewY) / wNumNeighbors;
			//Only allow visiting a square if (1) it hasn't been visited, or
			//(2) this is the shortest path to the square so far.
			if (wSquareScore && wSquareScore <= wCostToNextTile)
				continue;
			//If monster can move to this square...
			if (IsOpenMove(coord.wX, coord.wY, dx, dy))
			{
				dist = getMinDistance(wNewX, wNewY, dests, this->goal);
				if (dist <= nCloseEnough) //Close enough to destination.
				{
					//Construct path to goal and stop.
					CMonster::room.Add(wNewX,wNewY, wCostToNextTile * wNumNeighbors + nIndex);  //last node in path
					while ((UINT)wNewX != wX || (UINT)wNewY != wY)  //until starting point is returned to
					{
						this->pathToDest.Push(wNewX, wNewY);
						//Reverse the step made to this square.
						const UINT wO = CMonster::room.GetAt(wNewX, wNewY) % wNumNeighbors;
						ASSERT(wO < wNumNeighbors);
						wNewX -= dXs[wO];
						wNewY -= dYs[wO];
					}
					return true;
				}
				//...Add this unvisited node to priority queue.
				open.push(CPathNode(wNewX, wNewY, wCostToNextTile * wNumNeighbors + nIndex, wCostToNextTile * wNumNeighbors + nIndex));
				//NOTE: the heuristic below makes for a faster search, but monster
				//won't be guaranteed to prefer non-diagonal movement involving roundabout paths to the goal.
				//open.push(CPathNode(wNewX, wNewY, wCostPlusOne * O_MOD + nIndex, (wCostPlusOne+dist) * O_MOD + nIndex));
				CMonster::room.Add(wNewX,wNewY, wCostToNextTile * wNumNeighbors + nIndex);  //node is now visited
			}
		}
	} while (!open.empty());

	//No path found.
	return false;
}

//*****************************************************************************
/*
bool CMonster::FindOptimalPathToClosestMonster(
//Find the shortest path to (adjacent to) any aggressive monster.
//
//Returns: true if path found, else false
//
//Params:
	const UINT wX, const UINT wY, const CIDSet& monsterTypes) //(in) starting location
{
	//Init search.
	CDbRoom& curGameRoom = *(this->pCurrentGame->pRoom);
	VERIFY(CMonster::room.Init(curGameRoom.wRoomCols, curGameRoom.wRoomRows));

	//Push starting node.
	static const UINT wNumNeighbors = 8;
	static const int dXs[wNumNeighbors] = { 0,  1,  0, -1,  1,  1, -1, -1};
	static const int dYs[wNumNeighbors] = {-1,  0,  1,  0, -1,  1,  1, -1};
	static const UINT O_MOD = 16;   //each cell in CMonster::room stores (dist * O_MOD + direction from previous square)

	CMonster::room.Add(wX,wY, O_MOD + wNumNeighbors);  //large number ensures this square will never be visited
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
		const UINT wCostPlusOne = coord.wCost/O_MOD + 1;
		//If this cost is greater than a path already found to a goal, skip this node.
		if (wGoalDistance && wCostPlusOne > wGoalDistance)
			continue;

		//Move to neighbors in this order (discourage diagonal moves when possible).
		for (UINT nIndex=0; nIndex<wNumNeighbors; ++nIndex)
		{
			wNewX = coord.wX + (dx = dXs[nIndex]);
			wNewY = coord.wY + (dy = dYs[nIndex]);
			if (!curGameRoom.IsValidColRow(wNewX, wNewY))
				continue;
			const UINT wSquareScore = CMonster::room.GetAt(wNewX,wNewY) / O_MOD;
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
					CMonster::room.Add(wNewX,wNewY, wGoalDistance * O_MOD + nIndex);  //last node in path

					//Continue searching for better path to this goal until done.
					break;
				}
			}

			//If monster can move to this square...
			if (IsOpenMove(coord.wX, coord.wY, dx, dy))
			{
				//...Add this unvisited node to priority queue.
				open.push(CPathNode(wNewX, wNewY, wCostPlusOne * O_MOD + nIndex, wCostPlusOne * O_MOD + nIndex));
				CMonster::room.Add(wNewX,wNewY, wCostPlusOne * O_MOD + nIndex);  //node is now visited
			}
		}
	} while (!open.empty());

	if (!wGoalDistance)
		return false; //No path found.

	//Construct path to goal.
	wNewX = this->goal.wX;
	wNewY = this->goal.wY;
	while ((UINT)wNewX != wX || (UINT)wNewY != wY)  //until starting point is returned to
	{
		this->pathToDest.Push(wNewX, wNewY);
		//Reverse the step made to this square.
		const UINT wO = CMonster::room.GetAt(wNewX, wNewY) % O_MOD;
		ASSERT(wO < wNumNeighbors);
		wNewX -= dXs[wO];
		wNewY -= dYs[wO];
	}
	ASSERT(this->pathToDest.GetSize() == wGoalDistance);
	return true;
}
*/

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
const CMonster* CMonster::GetOwningMonsterConst() const {
	if (IsPiece()) {
		const CMonsterPiece* pPiece = DYN_CAST(const CMonsterPiece*, const CMonster*, this);
		ASSERT(pPiece->pMonster);
		return pPiece->pMonster;
	}
	return this;
}

//*****************************************************************************
bool CMonster::IsCombatable() const
//Returns: whether player can fight this monster (if ATK stat is sufficient, of course)
{
	return IsValidMonsterType(this->wType);
}

//*****************************************************************************
bool CMonster::IsTileObstacle(
//Overridable method to determine if a tile is an obstacle for this monster.
//This method and any overrides, should not evaluate game state or anything
//else besides the tile# to determine if the tile is an obstacle.
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
				wLookTileNo==T_KEY ||
				wLookTileNo==T_MIST ||
				bIsPowerUp(wLookTileNo) ||
				bIsMap(wLookTileNo) ||
				bIsShovel(wLookTileNo) ||
				bIsEquipment(wLookTileNo) ||
				bIsAnyArrow(wLookTileNo)
			);
		case WATER:
			return !(
				wLookTileNo==T_EMPTY ||
				bIsWater(wLookTileNo) ||
				bIsAnyArrow(wLookTileNo) ||
				wLookTileNo==T_NODIAGONAL ||
				wLookTileNo==T_FUSE ||
				wLookTileNo==T_SCROLL ||
				wLookTileNo==T_KEY ||
				wLookTileNo==T_MIST ||
				bIsPowerUp(wLookTileNo) ||
				bIsMap(wLookTileNo) ||
				bIsShovel(wLookTileNo) ||
				bIsEquipment(wLookTileNo) ||
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
				wLookTileNo==T_KEY ||
				wLookTileNo==T_MIST ||
				bIsMap(wLookTileNo) ||
				bIsShovel(wLookTileNo) ||
				bIsPowerUp(wLookTileNo) ||
				bIsEquipment(wLookTileNo) ||
				(IsFlying() && (bIsPit(wLookTileNo) || bIsWater(wLookTileNo))) ||
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

//*****************************************************************************************
//Returns: whether an egg should be spawned this turn
bool CMonster::IsSpawnEggTriggered(const CCueEvents& CueEvents) const
{
	//lay an egg any time player fights a monster...
	if (!CueEvents.HasOccurred(CID_MonsterEngaged))
		return false;

	if (this->pCurrentGame->pCombat == NULL)    //...fighting someone...
		return false;

	if (this->pCurrentGame->IsFighting(this)) //...but not me...
		return false;

	const CMonster* pCombatEnemy = this->pCurrentGame->pCombat->pMonster;
	if (!this->pCurrentGame->pCombat->PlayerCanHarmMonster(pCombatEnemy)) //doesn't count if player engages a monster that is too shielded
		return false;

	//don't lay when a monster created by egg-laying is killed
	if (pCombatEnemy->bEggSpawn)
		return false;

	//type-checking for backwards compatibility
	//don't lay more eggs when eggs are killed
	const UINT spawnID = GetSpawnType(M_REGG);

	const UINT enemyType = pCombatEnemy->wType;
	if (enemyType != M_CHARACTER)
		return enemyType != spawnID;

	const CCharacter* pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pCombatEnemy);
	return pCharacter->wLogicalIdentity != spawnID;
}

//*****************************************************************************
void CMonster::SpawnEgg(CCueEvents& CueEvents)
//Attempts to generate an egg monster next to this monster, in the direction of
//the nearest target.
{
	UINT wSX, wSY;
	if (!GetTarget(wSX, wSY))
		return;	//no change -- and don't lay eggs

	CCoordSet eggs;
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	float fClosest = 99999.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			//The criteria for laying an egg in a square should be:
			//1. Square does not contain a monster (including mimic).
			//2. Square does not contain player or a sword.
			//3. T-square is mostly empty (backwards compatibility).
			//4. F-tile is open.
			//5. O-square is open floor/door (except for open yellow doors -- for backwards compatibility).
			//6. Swordsman is still sensed at the new square moved to (backwards compatibility).
			const UINT ex = this->wX + x;
			const UINT ey = this->wY + y;
			if (!room.IsValidColRow(ex, ey))
				continue;

			const UINT wOSquare = room.GetOSquare(ex, ey);
			const UINT wTSquare = room.GetTSquare(ex, ey);
			CMonster* pMonster = room.GetMonsterAtSquare(ex, ey);
			if (
				// Not current queen position
				!(ex == this->wX && ey == this->wY) &&
				// Not the player
				!this->pCurrentGame->IsPlayerAt(ex, ey) &&
				// Not monster or player double or sword
				!pMonster && !DoesSquareContainObstacle(ex, ey) &&
				//And t-square is not occupied with a blocking item.
				(wTSquare == T_EMPTY || wTSquare == T_FUSE || wTSquare == T_MIST) &&
				!bIsArrow(room.GetFSquare(ex, ey)) &&
				//And o-square is floor or open door.
				((bIsPlainFloor(wOSquare) || wOSquare == T_PRESSPLATE) ||
					bIsOpenDoor(wOSquare) || bIsMistVent(wOSquare) ||
					bIsPlatform(wOSquare) || bIsBridge(wOSquare) ||
					wOSquare == T_GOO || wOSquare == T_FIRETRAP)
				)
			{
				//Spot is open for placing an egg.
				const float fDist = DistanceToTarget(ex, ey, wSX, wSY);
				if (fDist < fClosest)
				{
					//Place one egg on the tile closest to target.
					fClosest = fDist;
					eggs.clear();
					eggs.insert(ex, ey);
				}
			}
		}
	}

	//Lay eggs and check for them being laid on pressure plates.
	const UINT spawnID = GetSpawnType(M_REGG);
	for (CCoordSet::const_iterator egg = eggs.begin(); egg != eggs.end(); ++egg)
	{
		CMonster* m = const_cast<CCurrentGame*>(this->pCurrentGame)->AddNewEntity(
			CueEvents, spawnID, egg->wX, egg->wY, S, true);
		m->bEggSpawn = true;
		UINT wType = m->GetIdentity();
		if (wType != M_REGG && !bMonsterHasDirection(wType)) {
			m->wO = NO_ORIENTATION;
		}
		CueEvents.Add(CID_EggSpawned);
	}
}

UINT CMonster::GetSpawnType(UINT defaultMonsterID) const
{
	return this->pCurrentGame->getSpawnID(M_REGG);
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
	const bool bSmartAxial) //(in) can monster sidestep obstacles in axial directions
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
		//
		//If the target is one tile away and the monster can't move to the
		//target square, then this may cause oscillation.  However, it may also
		//allow the monster to reach the player from another side.
		//Therefore, this line is commented out:
		//if (nDist(this->wX,this->wY,wX,wY) > 1)
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
	bool bHasSword;
	if (this->pCurrentGame->IsPlayerAt(wX, wY))
	{
		const CSwordsman& player = *this->pCurrentGame->pPlayer;
//		ASSERT(player.IsTarget());
		bHasSword = player.HasSword();
		if (bHasSword)
		{
			wSX = player.GetSwordX();
			wSY = player.GetSwordY();
		}
	} else {
		CMonster *pTarget = this->pCurrentGame->pRoom->GetMonsterAtSquare(wX, wY);
		bHasSword = pTarget ? pTarget->GetSwordCoords(wSX, wSY) : false;
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
			else if (bHasSword && abs((int)(x - wSX)) <= 1 && abs((int)(y - wSY)) <= 1)
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
/*
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
		//2. Wall monsters should not beeline when brain-directed moves don't work,
		//as this could take them away from the target, according to the search cost heuristic
		//(causing oscillations).
		if (movementIQ == DirectOnly || (this->eMovement == WALL && i==paths.size()-1))
		{
			dx = dy = 0;
			dxFirst = tdx;
			dyFirst = tdy;
			return true;
		}
	}

	return false;
}
*/

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
/*
	if (this->pCurrentGame->bBrainSensesSwordsman && BrainAffects() &&
			//Proximate decoys override brain-directed movement.
			!this->pCurrentGame->pRoom->IsMonsterOfTypeAt(M_DECOY,wX,wY,true))
	{
		if (GetBrainDirectedMovement(dxFirst, dyFirst, dx, dy, movementIQ))
			return true;
	}
	else
*/
	if (!bIncludeNonTarget && !CanFindSwordsman())
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
	}
	return true;
}

//*****************************************************************************
UINT CMonster::getATK() const
//Return: monster's ATK
{
	UINT val = this->ATK;
	if (!this->pCurrentGame || !val) //multiply by zero will still be zero, so return now
		return val;
	const float fMult = this->pCurrentGame->GetTotalStatModifier(ScriptVars::MonsterATK);
	val = UINTBounds(val * fMult);
	return !val ? 1 : val; //ATK always >= 1
}

//*****************************************************************************
UINT CMonster::getColor() const
//Return: monster's color
{
	return 0; //no color addition by default
}

//*****************************************************************************
UINT CMonster::getDEF() const
//Return: monster's DEF
{
	if (IsOnMistTile() && !IsMistImmune())
		return 0; //Mist tile nullifies DEF

	UINT val = this->DEF;
	if (!this->pCurrentGame || !val)
		return val;
	const float fMult = this->pCurrentGame->GetTotalStatModifier(ScriptVars::MonsterDEF);
	val = UINTBounds(val * fMult);
	return !val ? 1 : val; //DEF always >= 1
}

//*****************************************************************************
int CMonster::getGOLD() const
//Return: monster's GOLD
{
	int val = this->GOLD;
	if (!this->pCurrentGame || !val)
		return val;
	const float fMult = this->pCurrentGame->GetTotalStatModifier(ScriptVars::MonsterGR); //may be negative
	val = intBounds(val * fMult); //may be positive, zero or negative
	if (!val)
	{
		//With a small multiplier, values often end as a fraction.
		//In this case, give at least one point.
		if (fMult < 0.0f)
			return -1;
		if (fMult > 0.0f)
			return 1;
	}
	return val;
}

//*****************************************************************************
UINT CMonster::getHP() const
//Return: monster's HP
{
	return this->HP;

	//Do not reapply the monsterHPmult each time this value is queried
	//during combat, because the return value gets used to reassign this->HP,
	//which would cause exponential compounding.
}

//*****************************************************************************
UINT CMonster::getSword() const
//Return: monster's custom weapon type
{
	return NoSword; //no predefined monster has a custom weapon
}

//*****************************************************************************
int CMonster::getXP() const
//Return: XP for defeating monster
{
	int val = this->XP;
	if (!this->pCurrentGame || !val)
		return val;
	const float fMult = this->pCurrentGame->GetTotalStatModifier(ScriptVars::MonsterXP); //may be negative
	val = intBounds(val * fMult); //may be positive, zero or negative
	if (!val)
	{
		//With a small multiplier, values often end as a fraction.
		//In this case, give at least one point.
		if (fMult < 0.0f)
			return -1;
		if (fMult > 0.0f)
			return 1;
	}
	return val;
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
		const CPlayerDouble *pDouble = DYN_CAST(const CPlayerDouble*, const CMonster*, this);
		if (pDouble->HasSword())
		{
			wX = pDouble->GetSwordX();
			wY = pDouble->GetSwordY();
			return true;
		}
	} else if (this->wType == M_CHARACTER)
	{
		const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, this);
		if (pCharacter->IsVisible() && pCharacter->HasSword())
		{
			//If NPC type has a sword by default, or NPC has been set to carry
			//a pre-defined or custom sword, then return its coords.
			const UINT sword = pCharacter->getSword();
			const bool bDefaultSword = sword == NPC_DEFAULT_SWORD &&
				 bEntityHasSword(pCharacter->GetIdentity());
			if (bDefaultSword ||
				 CCurrentGame::getPredefinedWeaponPower(sword) > 0 ||
				 (bIsCustomEquipment(sword) && this->pCurrentGame && this->pCurrentGame->CustomNPCExists(sword)))
			{
				wX = pCharacter->GetSwordX();
				wY = pCharacter->GetSwordY();
				return true;
			}
		}
	}
	return false;
}

//*****************************************************************************
bool CMonster::GetTarget(
//Monster chooses player as target
//
//Returns: whether monster is attracted to anything
//
//Params:
	UINT &wX, UINT &wY)  //(out) Where to move to
{
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
	if (player.IsInvisible() || !player.IsInRoom())
		return false; //invisible players aren't faced

	wX = player.wX;
	wY = player.wY;
	return true;
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
			if (i && j && pRoom->IsValidColRow(wX, wY))
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
bool CMonster::IsOnMistTile() const
//Returns: whether the monster's tile has mist
{
	return this->pCurrentGame->pRoom->GetTSquare(this->wX, this->wY) == T_MIST;
}

//*****************************************************************************
bool CMonster::IsOnSwordsman()
//Is the monster in the same square as Beethro?
const
{
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
//	if (player.IsTarget())
	{
		//Player is Beethro or an equivalent target.
		if (this->wX == player.wX && this->wY == player.wY)
			return true;
	}
/*
	//Check whether an NPC is posing as the swordsman.
	CMonster *pNPCBeethro = this->pCurrentGame->pRoom->GetNPCBeethro();
	if (pNPCBeethro)
	{
		if (this->wX == pNPCBeethro->wX && this->wY == pNPCBeethro->wY)
			return true;
	}
*/
	return false;
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
		return false; //SensesTarget(); //no Beethro in room, but an alternate target might be sensed

	//Currently, player is always target and visible.
	return true;
/*
	//If swordsman is visible, monster can see him.
	if (this->pCurrentGame->pPlayer->IsTarget())
	{
//		if (this->pCurrentGame->pPlayer->bIsVisible)
			return true;
	} else {
		//NPC Beethro always visible and sensed.
		return true;
	}

	return false;
	//Otherwise, monster can smell him if within range.
	if (CanSmellObjectAt(wSX, wSY))
		return true;

	//Otherwise, monster might sense any alternate monster targets.
	return SensesTarget();
*/
}

//*****************************************************************************
bool CMonster::CanSmellObjectAt(
//Returns: whether (wX,wY) is within smelling range.
//
//Params:
	const UINT wX, const UINT wY) //(in) Coordinates of object.
const
{
	 return nDist(wX, wY, this->wX, this->wY) <= DEFAULT_SMELL_RANGE;
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
bool CMonster::KillIfOutsideWall(CCueEvents& CueEvents)
//Kill the monster if it's not on a wall or door.
//
//Returns: If it died
{
	//If ghost was on a door that opened, kill it.
	const UINT dwTileNo = this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY);
	if (bIsOpenDoor(dwTileNo) || bIsFloor(dwTileNo))
	{
		CCurrentGame* pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		if (pGame->IsFighting(this))
			return false; //if terrain changes during a fight, just let the combat play out

		pGame->pRoom->KillMonster(this, CueEvents);
		SetKillInfo(NO_ORIENTATION); //center stab effect
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		return true;
	}
	return false;
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

/*
	//Check whether an NPC Beethro is going to be killed by this move.
	//Speed optimization (this avoids calling the slower IsOnSwordsman).
	CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(this->wX + dx, this->wY + dy);
	if (pMonster && (pMonster->GetIdentity() == M_BEETHRO ||
			pMonster->GetIdentity() == M_BEETHRO_IN_DISGUISE))
	{
		CueEvents.Add(CID_NPCBeethroDied, this);
		pMonster->bAlive = false;
	}
*/

	Move(this->wX + dx, this->wY + dy, &CueEvents);

	//If on the player then kill him.
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
//	if (player.IsTarget())
	{
		//Player is Beethro or an equivalent target.
		if (this->wX == player.wX && this->wY == player.wY)
			CueEvents.Add(CID_MonsterKilledPlayer, this);
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
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	CMonster *pMonster = room.GetMonsterAtSquare(wDestX,wDestY);
	ASSERT(!pMonster);
/*
	//If a stalwart is at the destination square, it dies.
	//If an NPC is playing the role of Beethro at the destination square, it also dies (i.e. game ends).
	if (pMonster)
	{
		ASSERT(pCueEvents);
		ASSERT(pMonster->wType == M_CHARACTER || pMonster->wType == M_STALWART);
		switch (pMonster->wType)
		{
			case M_CHARACTER:
				ASSERT(pMonster->GetIdentity() == M_BEETHRO ||
						pMonster->GetIdentity() == M_BEETHRO_IN_DISGUISE ||
						pMonster->GetIdentity() == M_STALWART);
				if (pMonster->GetIdentity() == M_STALWART)
				{
					pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
					room.KillMonster(pMonster, *pCueEvents); //will return false if it's a critical NPC
					pMonster->SetKillInfo(nGetO(wDestX-this->wX, wDestY-this->wY));
				}
			break;
			case M_STALWART:
				pCueEvents->Add(CID_MonsterDiedFromStab, pMonster);
				VERIFY(room.KillMonster(pMonster, *pCueEvents));
				pMonster->SetKillInfo(nGetO(wDestX-this->wX, wDestY-this->wY));
			break;
		}

		//Remove it from the room's monster array to avoid a pointer overwrite assertion.
		room.pMonsterSquares[room.ARRAYINDEX(wDestX, wDestY)] = NULL;
	}
*/

	//Update monster array.
	room.MoveMonster(this,wDestX,wDestY);

	//Set new coords.
	this->wX = wDestX;
	this->wY = wDestY;

	//Check for stepping on pressure plate.
	if (pCueEvents && !IsFlying() &&
			room.GetOSquare(wDestX, wDestY) == T_PRESSPLATE)
		room.ActivateOrb(wDestX, wDestY, *pCueEvents, OAT_PressurePlate);
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
	//Don't clear this->ExtraVars here, as CCharacter::Save, which is called above this, won't work properly because its compiled data will be wiped.

	//Pack monster stats.
	if (this->HP)
		this->ExtraVars.SetVar(HPStr, this->HP);
	if (this->ATK)
		this->ExtraVars.SetVar(ATKStr, this->ATK);
	if (this->DEF)
		this->ExtraVars.SetVar(DEFStr, this->DEF);
	if (this->GOLD)
		this->ExtraVars.SetVar(GOLDStr, this->GOLD);
	if (this->XP)
		this->ExtraVars.SetVar(XPStr, this->XP);

	if (this->bEggSpawn)
		this->ExtraVars.SetVar(EggSpawnStr, this->bEggSpawn);

	UINT dwExtraVarsSize;
	BYTE *pbytExtraVarsBytes = this->ExtraVars.GetPackedBuffer(dwExtraVarsSize);
	ASSERT(pbytExtraVarsBytes);
	c4_Bytes ExtraVarsBytes(pbytExtraVarsBytes, dwExtraVarsSize);

	c4_View PiecesView;
	const UINT wNumPieces = this->Pieces.size();
	if (wNumPieces)
	{
		PiecesView.SetSize(wNumPieces);
		list<CMonsterPiece*>::const_iterator piece = this->Pieces.begin();
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
//	p_IsFirstTurn(MonsterRowRef) = this->bIsFirstTurn;
//	p_ProcessSequence(MonsterRowRef) = this->wProcessSequence;
	p_ExtraVars(MonsterRowRef) = ExtraVarsBytes;
	p_Pieces(MonsterRowRef) = PiecesView;

	delete[] pbytExtraVarsBytes;
}

//*****************************************************************************
void CMonster::SetMembers(const CDbPackedVars& vars)
//Reads vars from ExtraVars to reconstruct the monster's stats.
{
	this->HP = vars.GetVar(HPStr, this->HP);
	this->ATK = vars.GetVar(ATKStr, this->ATK);
	this->DEF = vars.GetVar(DEFStr, this->DEF);
	this->GOLD = vars.GetVar(GOLDStr, this->GOLD);
	this->XP = vars.GetVar(XPStr, this->XP);
	this->bEggSpawn = vars.GetVar(EggSpawnStr, this->bEggSpawn);
}

//
//Behavior patterns.
//

//*****************************************************************************
bool CMonster::AttackPlayerWhenAdjacent(CCueEvents& CueEvents)
//If adjacent to the player in a direction without movement forbidden, then attack.
//Returns: true if an attack is executed
{
	//Find a target (i.e. the player).
	//If the player is invisible, he can't be seen.
	UINT wTX, wTY;
	if (!GetTarget(wTX,wTY))
		return false;

	//Attack if player is the target.
	if (!this->pCurrentGame->IsPlayerAt(wTX,wTY))
		return false;

	//Attack if adjacent.
	if (abs(static_cast<int>(this->wX - wTX)) > 1 || abs(static_cast<int>(this->wY - wTY)) > 1)
		return false;

	//Attack if movement in this direction is not forbidden.
	const int dx = wTX - this->wX;
	const int dy = wTY - this->wY;
	if (DoesArrowPreventMovement(dx, dy) ||
			this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy))
		return false;

	//Cannot attack something above me.
	if (IsTileAboveMe(wTX, wTY))
		return false;

	//Attack if the player has not already engaged the monster in combat.
	if (this->pCurrentGame->IsFighting(this))
		return false;

	//Hit the player once.
	CCombat combat((CCurrentGame*)this->pCurrentGame, this, false);
	combat.MonsterAttacksPlayerOnce(CueEvents);
	return true;
}

//*****************************************************************************
bool CMonster::AttackPlayerInFrontWhenBackIsTurned(CCueEvents &CueEvents)
//Attack the player when in front of the monster and the player's back is turned.
//Returns: true if an attack is executed
{
	//Find a target (i.e. the player).
	//If the player is invisible, he can't be seen.
	UINT wTX, wTY;
	if (!GetTarget(wTX,wTY))
		return false;

	//Attack if player is the target.
	if (!this->pCurrentGame->IsPlayerAt(wTX,wTY))
		return false;

	//Attack if the player is in front of me.
	const bool bPlayerSeen =
			wTX == (this->wX + nGetOX(this->wO)) &&
			wTY == (this->wY + nGetOY(this->wO));
	if (!bPlayerSeen)
		return false;

	//Attack if the player is not facing me.
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
	const int dx = wTX - this->wX;
	const int dy = wTY - this->wY;
	const bool bPlayerIsFacingMe =
		//player is pointing in my direction
		((dx && nGetOX(player.wO) == -dx) || (dy && nGetOY(player.wO) == -dy)) &&
		//and not diagonally to the side
		!(dx && dy && (nGetOX(player.wO) == dx || nGetOY(player.wO) == dy));
	if (bPlayerIsFacingMe)
		return false;

	//Attack if movement in this direction is not forbidden.
	if (DoesArrowPreventMovement(dx, dy) ||
		this->pCurrentGame->pRoom->DoesSquarePreventDiagonal(this->wX, this->wY, dx, dy))
		return false;

	//Cannot attack something above me.
	if (IsTileAboveMe(wTX, wTY))
		return false;

	//Attack if the player has not already engaged this monster in combat.
	if (this->pCurrentGame->IsFighting(this))
		return false;

	//Hit the player once.
	CCombat combat((CCurrentGame*)this->pCurrentGame, this, false);
	combat.MonsterAttacksPlayerOnce(CueEvents);
	return true;
}

//*****************************************************************************
bool CMonster::AttackPlayerWhenInFront(CCueEvents &CueEvents)
//Attack the player when the player stands directly in front of the monster.
//Returns: true if an attack is executed
{
	//Find target.  If player is invisible, player can't be seen.
	UINT wTX, wTY;
	if (!GetTarget(wTX,wTY))
		return false;

	//Attack if player is the target.
	if (!this->pCurrentGame->IsPlayerAt(wTX,wTY))
		return false;

	const bool bPlayerSeen = !HasOrientation() || //targets are always in front of monsters w/o orientation
			(wTX == (this->wX + nGetOX(this->wO)) &&
			wTY == (this->wY + nGetOY(this->wO)));
	if (!bPlayerSeen)
		return false;

	//Cannot attack something above me.
	if (IsTileAboveMe(wTX, wTY))
		return false;

	//Attack if the player has not already engaged this monster in combat.
	if (this->pCurrentGame->IsFighting(this))
		return false;

	//Hit the player once.
	CCombat combat((CCurrentGame*)this->pCurrentGame, this, false);
	combat.MonsterAttacksPlayerOnce(CueEvents);
	return true;
}

//*****************************************************************************
void CMonster::FaceAwayFromTarget()
//Make the monster face away from the player.
{
	if (!HasOrientation())
		return; //monster can't turn

	//Find a target (i.e. the player).
	//If the player is invisible, he can't be seen.
	UINT wTX, wTY;
	if (!GetTarget(wTX,wTY))
		return;

	//Face away from the target.
	const int dx = wTX - this->wX;
	const int dy = wTY - this->wY;
	SetOrientation(sgn(-dx), sgn(-dy));
}

//*****************************************************************************
void CMonster::FaceTarget()
//Make the monster face the player.
{
	if (!HasOrientation())
		return; //monster can't turn

	//Find a target (i.e. the player).
	//If the player is invisible, he can't be seen.
	UINT wTX, wTY;
	if (!GetTarget(wTX,wTY))
		return;

	//Face the target.
	const int dx = wTX - this->wX;
	const int dy = wTY - this->wY;
	SetOrientation(sgn(dx), sgn(dy));
}

//*****************************************************************************
bool CMonster::IsTileAboveMe(const UINT wTX, const UINT wTY) const
//Returns: whether the target tile is a position higher than my current tile
{
	if (IsFlying())
		return false; //flying monsters are always high up

	const UINT wTargetOTile = this->pCurrentGame->pRoom->GetOSquare(wTX,wTY);
	if (!bIsElevatedTile(wTargetOTile))
		return false;

	//If the target tile is elevated, then it is above me if my tile is not elevated.
	const UINT wMyOTile = this->pCurrentGame->pRoom->GetOSquare(this->wX,this->wY);
	return !bIsElevatedTile(wMyOTile);
}

//*****************************************************************************************
bool CMonster::GetNextGaze(
//Processes the gaze for this tile.  If the player is along this line,
//he is damaged.  Certain items and inventory can block the gaze.
//
//OUT: (cx,cy) is updated based on how the gaze travels when facing (dx,dy).
//
//Returns: true if gaze continues, false if blocked
//
//Params:
	CCueEvents &CueEvents,	//(in/out)
	CMonster *pCaster,		//non-NULL (i.e. pointer to casting monster) if game state is affected
	CDbRoom *pRoom,			//active room
	const bool bElevatedSource, //whether gaze comes from an elevated source
	UINT& cx, UINT& cy, int& dx, int& dy)	//(cx,cy) + (dx,dy)
{
	ASSERT(pRoom);
	const CCurrentGame *pCurrentGame = pRoom->GetCurrentGame();
	ASSERT(pCurrentGame);
	if (!pRoom->IsValidColRow(cx, cy))
		return false;

	const UINT oTile = pRoom->GetOSquare(cx,cy);
	switch (oTile)
	{
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_PIT: case T_PIT_IMAGE: case T_WATER:
		case T_PLATFORM_P: case T_PLATFORM_W:
		case T_HOT: case T_GOO:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_DOOR_MONEYO:
		case T_TUNNEL_E: case T_TUNNEL_W: case T_TUNNEL_N: case T_TUNNEL_S:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_PRESSPLATE: case T_THINICE:
		case T_MISTVENT: case T_FIRETRAP: case T_FIRETRAP_ON:
			//Gaze can go over these objects.
		break;
		default:
			//Door tiles may be traversed if the source is also elevated.
			if (bIsDoor(oTile))
			{
				if (!bElevatedSource)
					return false;
				break;
			}
			//All other objects stop gaze.
		return false;
	}

	//Opposing force arrows will block gaze.
	const int nO = nGetO(sgn(dx), sgn(dy));
	UINT fTile = pRoom->GetFSquare(cx, cy);
	if (bIsArrow(fTile) && bIsArrowObstacle(fTile, nO))
		return false;

	//Effect on t-layer items.
	switch (pRoom->GetTSquare(cx,cy))
	{
		case T_TAR: case T_MUD: case T_GEL:
		case T_BOMB:
		case T_OBSTACLE:
		case T_MIRROR:
		case T_CRATE:
		case T_POWDER_KEG:
			//These objects stop gaze.
			return false;
		case T_FUSE:
			//Gaze lights fuses.
			if (pCaster)
				pRoom->LightFuse(CueEvents, cx, cy,	false); //Light it right away next turn.
			break;
		case T_ORB:
			//Gaze damages cracked orbs.
			if (pCaster)
			{
				COrbData* pOrb = pRoom->GetOrbAtCoords(cx, cy);
				if (!pOrb)
					pOrb = pRoom->AddOrbToSquare(cx, cy); //add record to track orb state if not present
				if (pOrb->eType == OT_BROKEN)
				{
					pRoom->Plot(cx, cy, T_EMPTY); //broken orb is destroyed
					CueEvents.Add(CID_CrumblyWallDestroyed, new CMoveCoord(cx, cy, NO_ORIENTATION), true);
					break; //gaze continues
				}
				else if (pOrb->eType == OT_ONEUSE)
				{
					pOrb->eType = OT_BROKEN;
					CueEvents.Add(CID_OrbDamaged, pOrb);
					pRoom->Plot(CCoordSet(cx, cy));
					//gaze is blocked
				}
			}
		return false;
		default: break;
	}

	//Only player's largest sword will block the gaze.
	if (pCurrentGame->IsPlayerSwordAt(cx,cy) && pCurrentGame->equipmentBlocksGaze(ScriptFlag::Weapon))
		return false;

	CMonster *pMonster = pRoom->GetMonsterAtSquare(cx, cy);
	if (pMonster)
		return false;   //Monsters block gaze

	//Is swordsman frozen?
	if (pCurrentGame->IsPlayerAt(cx, cy))
	{
		if (pCaster)
		{
			CSwordsman& player = *((CCurrentGame*)pCurrentGame)->pPlayer;
			ASSERT(!pCurrentGame->InCombat()); //shouldn't process gazes during combat
/*
			if (
//				player.IsTarget() &&
				//Don't repeatedly damage player while engaged in combat.
				//Damage will occur only once when combat is completed.
				(!pCurrentGame->IsFighting(pCaster)))
*/
			{
				const int nPlayerO = player.wO;
				const int dot = dx * nGetOX(nPlayerO) + dy * nGetOY(nPlayerO);

				if (!(pCurrentGame->equipmentBlocksGaze(ScriptFlag::Armor) && dot < 0) &&
					 !pCurrentGame->equipmentBlocksGaze(ScriptFlag::Accessory))
				{
					//Cut player's health by beam value (halved when hasted).
					UINT beamVal = player.st.beamVal;
					if (!beamVal)
						return false; // no damage, blocks gaze

					UINT delta = 0;
					if (beamVal == 50) {
						// Backwards compatibility - CalcDamage rounds up, old way rounds down
						PlayerStats& ps = player.st;
						delta = ps.HP / (player.IsHasted() ? 4 : 2);
					} else {
						delta = player.CalcDamage(beamVal);
					}

					if (!delta)
						delta = 1;
					player.DecHealth(CueEvents, delta, CID_MonsterKilledPlayer);

					CueEvents.Add(CID_PlayerFrozen, &player);
				}
			}
		}

		//Blocks gaze.
		return false;
	}

	//Gaze continues.
	cx += dx;
	cy += dy;
	return true;
}

//*****************************************************************************************
void CMonster::UpdateGaze(
//Check tiles along the monster's line of sight.
//Effects of the gaze are processed for each tile in GetNextGaze().
//
//Params:
	CCueEvents &CueEvents)
{
	//Follow direction of the monster's gaze (i.e. orientation).
	int oX = nGetOX(this->wO);
	int oY = nGetOY(this->wO);

	//Standing on a force arrow opposing this direction will halt the gaze.
	if (!DoesArrowPreventMovement(this->wX, this->wY, oX, oY))
	{
		CueEvents.Add(CID_ZombieGaze, this);

		const bool bElevatedSource = bIsElevatedTile(
				this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY));
		UINT wX = this->wX + oX;   //begin processing from the tile in front of me
		UINT wY = this->wY + oY;
		while (GetNextGaze(CueEvents, this, this->pCurrentGame->pRoom, bElevatedSource, wX, wY, oX, oY))
			; //continue until blocked
	}
}


//*****************************************************************************
CMonsterFacesTarget::CMonsterFacesTarget(
	const UINT wSetType, const CCurrentGame *pSetCurrentGame,
	const MovementType eMovement,
	const UINT wSetProcessSequence)
	: CMonster(wSetType, pSetCurrentGame, eMovement, wSetProcessSequence)
{ }

//*****************************************************************************
void CMonsterFacesTarget::Process(
//Overridable method to process a monster for movement after player moves.
//If derived class uses this method, then the monster will face the player.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &/*CueEvents*/) //(in) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may
							//correspond to sound or graphical effects.
{
	FaceTarget();
}
