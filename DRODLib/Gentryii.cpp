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

#include "Gentryii.h"

#include "GameConstants.h"
#include "CurrentGame.h"
#include "CueEvents.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
CGentryii::CGentryii(CCurrentGame *pSetCurrentGame)
	: CMonster(M_GENTRYII, pSetCurrentGame)
	, bPinPreventsMove(false)
{
}

//*****************************************************************************************
void CGentryii::Process(
//Process for movement:
//
//The gentryii attempts to beeline its head towards the target.
//In order to do this, its body also needs to be pulled forward by the head in the
//desired direction.  Its body acts as a chain comprised of a series of links,
//pulled with approximated tension physics.
//It does not follow the rigid, static path that a serpent body advances along.
//
//A gentryii body part may be caught on an obstacle, in which case, the desired
//move is successful only if the chain can be pulled loosely before that obstacle.
//
//The workhorse of this logic is performed in IsOpenMove, which calls CanPullChain.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (IsStunned())
		return;

	//Find where to move to.
	UINT wX, wY;
	if (!GetTarget(wX,wY))
		return;

	this->tdx = nGetDX(this->wX, wX);
	this->tdy = nGetDX(this->wY, wY);
	
	//Determine move.
	this->bPinPreventsMove = false;
	this->wPinDepth = 0;

	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(wX, wY, dxFirst, dyFirst, dx, dy))
		return;

	//Take move.
	if (dx || dy) {
		MoveGentryii(this->wX + dx, this->wY + dy, CueEvents);
	} else if (this->bPinPreventsMove) {
		PullChainTaut(CueEvents);
	}

	SetHeadOrientation(dxFirst, dyFirst);
}

//*****************************************************************************
bool CGentryii::CanPushHeadOntoOTileAt(const UINT destX, const UINT destY)
{
	const UINT wTileNo = this->pCurrentGame->pRoom->GetOSquare(destX, destY);
	const bool bIsOpenMove = bIsFloor(wTileNo) ||
		bIsOpenDoor(wTileNo) ||
		bIsPlatform(wTileNo);

	if (!bIsOpenMove)
		return false;

	//Relevant logic copied from IsOpenMove

	this->piece_moves.clear();
	this->checked_moves.clear();
	this->bCheckPin = false;

	//Update wGoalX/Y as a chain obstacle since the head intends to move to this tile.
	this->wGoalX = destX;
	this->wGoalY = destY;

	this->pending_piece_dests.clear();
	return CanPullChain(destX, destY, this, this->Pieces.begin(), 1);
}

//*****************************************************************************
void CGentryii::MoveGentryii(UINT destX, UINT destY, CCueEvents& CueEvents)
{
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	const bool bNPCBeethroDied = NPCBeethroDieCheck(destX, destY, CueEvents);

	CCoordSet updateTiles;
	UINT wLastX1 = this->wX; UINT wLastX2 = destX;
	UINT wLastY1 = this->wY; UINT wLastY2 = destY;
	bool bLastMoved = wLastX1 != wLastX2 || wLastY1 != wLastY2;

	room.ReevalBriarNear(this->wX,this->wY,T_GENTRYII);
	Move(destX, destY, &CueEvents);

	if (!bNPCBeethroDied && IsOnSwordsman())
	{
		CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
		pGame->SetDyingEntity(&pGame->swordsman, this);
		CueEvents.Add(CID_MonsterKilledPlayer, this);
	}

	//Move pieces as determined previously.
	MonsterPieces::iterator pieceIt = this->Pieces.begin();
	for (PIECE_MOVES::const_reverse_iterator moveIt = this->piece_moves.rbegin();
			moveIt != this->piece_moves.rend(); ++moveIt, ++pieceIt)
	{
		ASSERT(pieceIt != this->Pieces.end());
		CMonsterPiece *pPiece = *pieceIt;
		const std::pair<UINT,UINT>& dest = *moveIt;
		const bool bThisMoved = pPiece->wX != dest.first || pPiece->wY != dest.second;

		if (bThisMoved || bLastMoved)
		{
			if (wLastX1 != pPiece->wX || wLastY1 != pPiece->wY)
			{
				updateTiles.insert(wLastX1,pPiece->wY);
				updateTiles.insert(pPiece->wX,wLastY1);
			}
			if (wLastX2 != dest.first || wLastY1 != dest.second)
			{
				updateTiles.insert(wLastX2,dest.second);
				updateTiles.insert(dest.first,wLastY2);
			}
		}
		wLastX1 = pPiece->wX; wLastX2 = dest.first;
		wLastY1 = pPiece->wY; wLastY2 = dest.second;
		if (bThisMoved)
		{
			updateTiles.insert(pPiece->wX,pPiece->wY);
			updateTiles.insert(dest.first,dest.second);
			pPiece->Move(dest.first, dest.second, &CueEvents);
		}
		bLastMoved = bThisMoved;
	}
	//Check whether diagonals at end after movement must be updated
	if (bLastMoved && pieceIt != this->Pieces.end())
	{
		CMonsterPiece *pPiece = *pieceIt;
		if (wLastX1 != pPiece->wX && wLastY1 != pPiece->wY)
		{
			updateTiles.insert(wLastX1,pPiece->wY);
			updateTiles.insert(pPiece->wX,wLastY1);
		}
		if (wLastX2 != pPiece->wX && wLastY2 != pPiece->wY)
		{
			updateTiles.insert(wLastX2,pPiece->wY);
			updateTiles.insert(pPiece->wX,wLastY2);
		}
	}

	this->piece_moves.clear();

	//Update Pathmap
	for (CCoordSet::const_iterator tile=updateTiles.begin(); tile!=updateTiles.end(); ++tile)
	{
		room.UpdatePathMapAt(tile->wX,tile->wY);
		room.ReevalBriarNear(tile->wX,tile->wY,T_GENTRYII);
	}
}

//*****************************************************************************
bool CGentryii::IsOpenMove(const int dx, const int dy) const
{
	ASSERT(dx || dy);

	const bool bOpenMove = CMonster::IsOpenMove(dx, dy);
	this->bCheckPin = this->tdx == dx && this->tdy == dy;
	//Only do a pin test if we're testing our unbrained direction
	if (!bOpenMove && !this->bCheckPin)
		return false;

	const UINT destX = this->wX + dx, destY = this->wY + dy;
	this->piece_moves.clear();
	this->checked_moves.clear();

	//Update wGoalX/Y as a chain obstacle since the head intends to move to this tile.
	//If we're doing a pin test, then don't move the head.
	this->wGoalX = bOpenMove ? destX : (UINT)-1;
	this->wGoalY = bOpenMove ? destY : (UINT)-1;

	this->pending_piece_dests.clear();
	const bool ret = CanPullChain(destX, destY, this, this->Pieces.begin(), 1);
	if (bOpenMove)
		return ret;
	if (ret)
	{
		//The move was a success, so the Gentryii can't be pinned, even if the head can't move.
		this->bPinPreventsMove = false;
		this->wPinDepth = 0;
	}

	return false;
}

//*****************************************************************************
bool CGentryii::CanPullChain(
	const UINT destX, const UINT destY,
	const CMonster* prevPiece,
	MonsterPieces::const_iterator pieceIt, //current piece
	const UINT depth)
const
{
	if (pieceIt == this->Pieces.end())
		return true;

	const CMonsterPiece *pPiece = *pieceIt;
	const CMonsterPiece& piece = *pPiece;

	const std::pair<UINT,UINT> pos = make_pair(piece.wX, piece.wY);
	pair<CHECKED_CHAIN_MOVES::const_iterator, CHECKED_CHAIN_MOVES::const_iterator> range =
		this->checked_moves.equal_range(pos);
	for (CHECKED_CHAIN_MOVES::const_iterator it=range.first; it!=range.second; ++it)
	{
		const std::pair<UINT,UINT>& dest = it->second;
		if (dest.first == destX && dest.second == destY)
			return false; //already checked this possibility -- it didn't work
	}

	//Must either be adjacent to leading piece's destination,
	//or be able to move adjacent to it.
	const int piece_dx = destX - piece.wX;
	const int piece_dy = destY - piece.wY;
	const bool adjacent = (abs(piece_dx) <= 1) && (abs(piece_dy) <= 1);
	if (adjacent)
	{
		//Make sure that this doesn't pull a connecting link against a Force Arrow's direction
		const UINT wFTileNo = this->pCurrentGame->pRoom->GetFSquare(piece.wX,piece.wY);
		const int link_dx = destX - prevPiece->wX;
		const int link_dy = destY - prevPiece->wY;
		if (!(bIsArrow(wFTileNo) && bIsArrowObstacle(wFTileNo,nGetO(link_dx,link_dy))))
			return true; //found slack -- chain pull stops here
	}

	if (IsPinnedAt(piece.wX, piece.wY)) {
		//cannot pull closer, so this move doesn't work
		if (this->bCheckPin && this->wPinDepth < depth)
		{
			this->bPinPreventsMove = true;
			this->wPinDepth = depth;
		}
		return false;
	}

	MonsterPieces::const_iterator nextPiece = pieceIt;
	++nextPiece;

	this->pending_piece_dests.insert(destX, destY);

	//If we got this far, then the current piece cannot stay where it is
	UINT newX = destX, newY = destY;
	if (!GetClosestOpenTileTo(prevPiece, piece,
			(nextPiece != this->Pieces.end()) ? *nextPiece : NULL,
			newX, newY))
	{
		this->checked_moves.insert(make_pair(pos, make_pair(destX, destY)));
		this->pending_piece_dests.erase(destX, destY);
		if (this->bCheckPin && this->wPinDepth < depth)
		{
			this->bPinPreventsMove = true;
			this->wPinDepth = depth;
		}
		return false; //no valid move
	}

	if (CanPullChain(newX, newY, pPiece, nextPiece, depth+1)) //recursive call
	{
		this->piece_moves.push_back(make_pair(newX, newY));
		return true;
	}

	//Something blocked the best move -- try a suboptimal one.
	if (piece_dx && piece_dy) {
		const UINT alternateNewX = piece.wX;
		const bool alternateX_is_adjacent = (abs((int)(destX - alternateNewX)) <= 1) && (abs((int)(destY - newY)) <= 1);
		if (alternateX_is_adjacent &&
				!IsChainObstacle(piece.wX, piece.wY, int(alternateNewX - piece.wX), int(newY - piece.wY), prevPiece, destX, destY) &&
				CanPullChain(alternateNewX, newY, pPiece, nextPiece, depth+1)) //recursive call
		{
			this->piece_moves.push_back(make_pair(alternateNewX, newY));
			return true;
		}

		const UINT alternateNewY = piece.wY;
		const bool alternateY_is_adjacent = (abs((int)(destX - newX)) <= 1) && (abs((int)(destY - alternateNewY)) <= 1);
		if (alternateY_is_adjacent &&
				!IsChainObstacle(piece.wX, piece.wY, int(newX - piece.wX), int(alternateNewY - piece.wY), prevPiece, destX, destY) &&
				CanPullChain(newX, alternateNewY, pPiece, nextPiece, depth+1)) //recursive call
		{
			this->piece_moves.push_back(make_pair(newX, alternateNewY));
			return true;
		}
	}

	this->checked_moves.insert(make_pair(pos, make_pair(destX, destY)));
	this->pending_piece_dests.erase(destX, destY);
	return false;
}

//*****************************************************************************
bool CGentryii::IsPinnedAt(const UINT wX, const UINT wY) const
{
	const CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT oTile = room.GetOSquare(wX, wY);
	if (bIsDoor(oTile))
		return true;
	if (bIsCrumblyWall(oTile))
		return true;
	if (bIsWall(oTile))
		return true;

	return false;
}

//*****************************************************************************
bool CGentryii::GetClosestOpenTileTo(
//Searches for the closest valid tile that 'piece' can be moved to,
//assuming that the leading monster piece has moved to destX/Y
//
//Return: true when destX/Y has been set to the closest tile this piece can move to,
//        otherwise false
	const CMonster* prevPiece,
	const CMonsterPiece& piece,
	const CMonsterPiece* nextPiece,
	UINT& destX, UINT& destY) //in: proposed new location of leading piece
	                          //out: proposed new location of this piece
const
{
	static const int step_dx[8] = {0,  -1, 1, 0, -1,  1, -1, 1};
	static const int step_dy[8] = {-1,  0, 0, 1, -1, -1,  1, 1};

	ASSERT(prevPiece);

	static const int NO_BEST_MOVE = -1;
	int best_move = NO_BEST_MOVE;
	UINT bestManhattan = UINT(-1);
    UINT bestSteps = UINT(-1);

	//If this routine is called, then we have already checked and discarded
	//the option of remaining stationary.  Don't consider it.
	
	for (UINT k=0; k<8; ++k) {
		const int dx = step_dx[k];
		const int dy = step_dy[k];
		const int i = piece.wX + dx;
		const int j = piece.wY + dy;

		const int dest_dx = destX - i;
		const int dest_dy = destY - j;
		const bool adjacent = (abs(dest_dx) <= 1) && (abs(dest_dy) <= 1);
		if (!adjacent)
			continue;

		//Move to location with smallest distance both to the leading and trailing pieces.
		UINT manhattanDist = nL1Dist(i, j, destX, destY);
		UINT steps = nDist(i, j, destX, destY);
		if (nextPiece) {
			manhattanDist += nL1Dist(i, j, nextPiece->wX, nextPiece->wY);
			steps += nDist(i, j, nextPiece->wX, nextPiece->wY);
		} else {
			manhattanDist += nL1Dist(i, j, piece.wX, piece.wY);
			steps += nDist(i, j, piece.wX, piece.wY);
		}

		const bool better_move = manhattanDist < bestManhattan ||
			(manhattanDist == bestManhattan &&
				(steps < bestSteps ||
				(steps == bestSteps && (UINT)i == prevPiece->wX && (UINT)j == prevPiece->wY)));
		if (better_move && !IsChainObstacle(piece.wX, piece.wY, dx, dy, prevPiece, destX, destY))
		{
			//No obstacle -- select this as the best move
			bestManhattan = manhattanDist;
			bestSteps = steps;
			best_move = k;
		}
	}

	//Found a valid move?
	if (best_move == NO_BEST_MOVE)
		return false;

	destX = piece.wX + step_dx[best_move];
	destY = piece.wY + step_dy[best_move];
	return true;
}

//*****************************************************************************
bool CGentryii::DoesSquareContainChainObstacle(
//Determines if a square contains an obstacle for this chain link.
//Parts copied from CMonster::DoesSquareContainObstacle.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
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
		return true;

	//Chain can only sweep through fluff babies.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY)
		return true;

	//Check for player at square.
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if (player.IsAt(wCol,wRow))
		return true;
	
	//No obstacle.
	return false;
}

//*****************************************************************************
bool CGentryii::IsChainObstacle(
	const UINT wX, const UINT wY,
	const int dx, const int dy,
	const CMonster* prevPiece,
	const UINT prvDestX, const UINT prvDestY)
const
{
	if (DoesArrowPreventMovement(wX, wY, dx, dy))
		return true;

	const int destX = wX + dx;
	const int destY = wY + dy;
	//Chain can't move to where the head is going
	if ((UINT)destX == this->wGoalX && (UINT)destY == this->wGoalY)
		return true;

	const CDbRoom& room = *(this->pCurrentGame->pRoom);
	const CMonster *pMonster = room.GetMonsterAtSquare(destX, destY);
	if (pMonster == prevPiece) //don't need to check for obstacle where leading piece is currently located
		return false; //TODO -- do we need at least some obstacle checks here?

	if (DoesSquareContainChainObstacle(destX, destY))
		return true;

	//The Gentryii is not permitted to pull
	//a pair of diagonal chain links (C) across a single obstacle tile (#), i.e., moving from C to c:
	// .C.
	// C#c
	// .c.
	const int chain_dx = prvDestX - prevPiece->wX;
	const int chain_dy = prvDestY - prevPiece->wY;
	const int piece_dx = prvDestX - wX;
	const int piece_dy = prvDestY - wY;
	if (chain_dx && chain_dy && !(piece_dx && piece_dy)) //diagonal link with prevPiece moving diagonally to axial location
	{
		UINT chain_nx = wX, chain_ny = wY;
		if (piece_dx)
		{
			chain_nx += sgn(piece_dx);
		} else {
			chain_ny += sgn(piece_dy);
		}

		if (chain_nx == this->wGoalX && chain_ny == this->wGoalY)
			return true;
		if (DoesSquareContainChainObstacle(chain_nx, chain_ny))
			return true;
		if (this->pCurrentGame->IsPlayerAt(chain_nx, chain_ny))
			return true;

		const UINT wTileNo = room.GetFSquare(chain_nx, chain_ny);
		if (bIsArrow(wTileNo)) {
			const int nO = nGetO(sgn(chain_dx), sgn(chain_dy));
			return bIsArrowObstacle(wTileNo, nO);
		}
	}

	if (this->pending_piece_dests.has(destX, destY))
		return true; //TOFIX: conditional failures?

	return false;
}

//*****************************************************************************
void CGentryii::PullChainTaut(CCueEvents& CueEvents)
{
	MonsterPieces::iterator pieceIt = this->Pieces.begin();
	if (pieceIt == this->Pieces.end())
		return;

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	this->piece_moves.clear();
	CMonster *pPrevPiece = this;
	UINT prevX = pPrevPiece->wX, prevY = pPrevPiece->wY;
	UINT depth = 1;
	CCoordSet movedPieces, updateTiles;
	for (;;)
	{
		CMonsterPiece *pPiece = *pieceIt;
		const CMonsterPiece& piece = *pPiece;

		if (this->wPinDepth == depth)
			break;

		MonsterPieces::iterator nextPieceIt = pieceIt;
		++nextPieceIt;
		if (nextPieceIt == this->Pieces.end())
			break; //nothing past this point to pull taut against

		const int dx = prevX - piece.wX;
		const int dy = prevY - piece.wY;
		if (!dx || !dy) {
			//Remain stationary.
			prevX = piece.wX;
			prevY = piece.wY;
		} else {
			//Attempt to pull this chain link taut, i.e.,
			//reduce the Manhattan distance to the leading piece to one.
			const CMonsterPiece& nextPiece = **nextPieceIt;

			//Choose the axial direction that is better to pull taut.
			//It's the one with the lowest number of steps to reach.
			const UINT d1 = nDist(prevX, piece.wY, nextPiece.wX, nextPiece.wY);
			const UINT d2 = nDist(piece.wX, prevY, nextPiece.wX, nextPiece.wY);

			if (d1 < d2) {
				ASSERT(d1 <= 1);

				//Set prevX/Y to where this link should go.
				const int linkX = int(prevX - piece.wX);
				if (!d1 || movedPieces.has(prevX, piece.wY) || DoesSquareContainChainObstacle(prevX, piece.wY) ||
					DoesArrowPreventMovement(piece.wX, piece.wY, linkX, 0) ||
					bIsArrowObstacle(room.GetFSquare(pPrevPiece->wX, pPrevPiece->wY), nGetO(linkX, 0)) ||
					bIsArrowObstacle(room.GetFSquare(nextPiece.wX, nextPiece.wY), nGetO(linkX, 0)))
					prevX = piece.wX;
				prevY = piece.wY;
			} else if (d2 < d1) {
				ASSERT(d2 <= 1);

				const int linkY = int(prevY - piece.wY);
				prevX = piece.wX;
				if (!d2 || movedPieces.has(piece.wX, prevY) || DoesSquareContainChainObstacle(piece.wX, prevY) ||
					DoesArrowPreventMovement(piece.wX, piece.wY, 0, linkY) ||
					bIsArrowObstacle(room.GetFSquare(pPrevPiece->wX, pPrevPiece->wY), nGetO(0, linkY)) ||
					bIsArrowObstacle(room.GetFSquare(nextPiece.wX, nextPiece.wY), nGetO(0, linkY)))
					prevY = piece.wY;
			} else {
				//Remain stationary.
				prevX = piece.wX;
				prevY = piece.wY;
			}
		}

		this->piece_moves.push_back(make_pair(prevX, prevY));
		movedPieces.insert(prevX,prevY);

		pPrevPiece = pPiece;
		pieceIt = nextPieceIt;
		++depth;
	}

	//Move pieces as determined previously.
	bool bMovedAnyPieces = false;
	UINT wLastX1 = this->wX; UINT wLastX2 = this->wX;
	UINT wLastY1 = this->wY; UINT wLastY2 = this->wY;
	bool bLastMoved = false;
	pieceIt = this->Pieces.begin();
	for (PIECE_MOVES::const_iterator moveIt = this->piece_moves.begin();
			moveIt != this->piece_moves.end(); ++moveIt, ++pieceIt)
	{
		ASSERT(pieceIt != this->Pieces.end());
		CMonsterPiece *pPiece = *pieceIt;
		const std::pair<UINT,UINT>& dest = *moveIt;
		const bool bThisMoved = pPiece->wX != dest.first || pPiece->wY != dest.second;
		if (bThisMoved || bLastMoved)
		{
			if (wLastX1 != pPiece->wX && wLastY1 != pPiece->wY)
			{
				updateTiles.insert(wLastX1,pPiece->wY);
				updateTiles.insert(pPiece->wX,wLastY1);
			}
			if (wLastX2 != dest.first && wLastY2 != dest.second)
			{
				updateTiles.insert(wLastX2,dest.second);
				updateTiles.insert(dest.first,wLastY2);
			}
		}
		wLastX1 = pPiece->wX; wLastX2 = dest.first;
		wLastY1 = pPiece->wY; wLastY2 = dest.second;
		if (bThisMoved)
		{
			updateTiles.insert(pPiece->wX,pPiece->wY);
			updateTiles.insert(dest.first,dest.second);
			pPiece->Move(dest.first, dest.second, &CueEvents);
			bMovedAnyPieces = true;
		}
		bLastMoved = bThisMoved;
	}
	//Check whether diagonals at end after movement must be updated
	if (bLastMoved && pieceIt != this->Pieces.end())
	{
		CMonsterPiece *pPiece = *pieceIt;
		if (wLastX1 != pPiece->wX && wLastY1 != pPiece->wY)
		{
			updateTiles.insert(wLastX1,pPiece->wY);
			updateTiles.insert(pPiece->wX,wLastY1);
		}
		if (wLastX2 != pPiece->wX && wLastY1 != pPiece->wY)
		{
			updateTiles.insert(wLastX2,pPiece->wY);
			updateTiles.insert(pPiece->wX,wLastY2);
		}
	}

	if (bMovedAnyPieces)
		CueEvents.Add(CID_GentryiiPulledTaut);

	this->piece_moves.clear();

	//Update Pathmap
	for (CCoordSet::const_iterator tile=updateTiles.begin(); tile!=updateTiles.end(); ++tile)
	{
		room.UpdatePathMapAt(tile->wX,tile->wY);
		room.ReevalBriarNear(tile->wX,tile->wY,T_GENTRYII);
	}
}

//*****************************************************************************
void CGentryii::PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents)
{
	this->wPrevX = this->wX;
	this->wPrevY = this->wY;

	const UINT wDestX = this->wX + dx;
	const UINT wDestY = this->wY + dy;

	MoveGentryii(wDestX, wDestY, CueEvents);
	SetHeadOrientation(dx, dy);

	this->pCurrentGame->pRoom->CheckForFallingAt(wDestX, wDestY, CueEvents);

	if (bStun && IsAlive())
		Stun(CueEvents, 2);
}

//*****************************************************************************
void CGentryii::SetHeadOrientation(int dx, int dy)
{
	if (this->Pieces.empty()) {
		SetOrientation(dx, dy);
	} else {
		//Orientation is away from first chain link
		const CMonsterPiece* first_piece = this->Pieces.front();
		SetOrientation(this->wX - first_piece->wX, this->wY - first_piece->wY);
	}
}
