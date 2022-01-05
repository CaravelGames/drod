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

//Halph.cpp
//Implementation of CHalph.

#include "Halph.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include <queue>

//
//Public methods.
//

//*****************************************************************************************
CHalph::CHalph(
	CCurrentGame *pSetCurrentGame,
	const MovementType eMovement,   //(in)    [default=GROUND]
	const UINT type) //[default=M_HALPH]
	: CMonster(type, pSetCurrentGame, eMovement)
	, state(Entering)
	, wMoves(0)
{}

//*****************************************************************************************
CHalph2::CHalph2(CCurrentGame *pSetCurrentGame, const UINT /*type*/) //[default=M_HALPH2]
	: CHalph(pSetCurrentGame, GROUND_AND_SHALLOW_WATER, M_HALPH2)
{}

//*****************************************************************************************
void CHalph::Process(
//Process Halph for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	if (!this->bAlive) return;

	//If stunned, skip turn
	if (IsStunned())
		return;

	switch (this->state)
	{
		case Entering:
		{
			if (!(this->wMoves ||
				this->wX == 0 || this->wX == this->pCurrentGame->pRoom->wRoomCols-1 ||
				this->wY == 0 || this->wY == this->pCurrentGame->pRoom->wRoomRows-1))
			{
				SetState(Waiting);
				break;
			}

			//From room edge, take a couple of steps into the room towards Beethro.
			const CSwordsman & player = this->pCurrentGame->swordsman;
			if (!bIsBeethroDouble(player.wAppearance))
				break;
			CCoordSet dest(player.wX, player.wY);
			if (!GetPathTo(CueEvents, dest))
				break;
			if (TakeStep(CueEvents))
				if (++this->wMoves == 2)
					SetState(Waiting);
		}
		break;

		case Following:
		{
			//Follow behind the player (i.e. Halph tries to move next to Beethro).
			//Hack: Halph can form pathmaps through Beethro's sword.
			//This allows Halph to follow Beethro along single-file corridors
			//when Beethro's sword is in the way.
			CSwordsman & player = const_cast<CSwordsman&>(this->pCurrentGame->swordsman);
			if (nDist(this->wX, this->wY, player.wX, player.wY) <= 1)
				break;
			const UINT wSaveSwordX = player.wSwordX;
			player.wSwordX = (UINT)-1;

			//Use smarter-than-brain pathmap (to allow going over arrows, etc).
			//This will be calculated more quickly than a brain pathmap since player
			//will almost always be within a couple squares of Halph.
			CCoordSet dest(player.wX, player.wY);
			this->pCurrentGame->pRoom->GetSwordCoords(this->swordsInRoom); //speed optimization
			const bool bRes = FindOptimalPathTo(this->wX, this->wY, dest);
			player.wSwordX = wSaveSwordX;	//restore value
			if (bRes)
			{
				TakeStep(CueEvents);	//no verify -- player's sword might be in the way
			} else {
				int dxFirst, dyFirst, dx, dy;
				GetBeelineMovementSmart(wX, wY, dxFirst, dyFirst, dx, dy, true);
				if (dx || dy)
				{
					if (player.wX != this->wX + dx ||
							player.wY != this->wY + dy)
						Move(this->wX + dx, this->wY + dy, &CueEvents);
					SetOrientation(dxFirst, dyFirst);
				}
			}
		}
		break;

		case Waiting:
			//Halph does nothing while waiting.
		break;

		case Striking:
		{
			//Moving to strike an orb to open a door.
			CDbRoom& room = *this->pCurrentGame->pRoom;
			ASSERT(room.IsValidColRow(this->openingDoorAt.wX, this->openingDoorAt.wY));
			if (room.GetOSquare(this->openingDoorAt.wX, this->openingDoorAt.wY) != T_DOOR_Y)
			{
				//If the door has opened, then can stop going to the orb.
				SetState(PostStriking);
			} else {
				if (!ConfirmGoal(this->orbLocation, this->bPlateIsGoal))
				{
					//If we don't find an orb/plate at our goal
					//then we need to update what orbs/plates Halph wants to activate.
					CCoordSet doorSquares, orbs, plates;
					room.GetAllYellowDoorSquares(this->openingDoorAt.wX, this->openingDoorAt.wY, doorSquares);
					room.FindOrbsToOpenDoor(orbs, doorSquares);
					room.FindPlatesToOpenDoor(plates, doorSquares);

					this->orbsToHit = orbs;
					this->platesToDepress = plates;
					this->pathToDest.Clear();
				}
				CCoordSet plates;
				room.GetDepressablePlateSubset(this->platesToDepress, plates);
				if (GetPathTo(CueEvents, this->orbsToHit, &plates)) {
					VERIFY(TakeStep(CueEvents));	//toward orb
				} else {
					//Either Halph just activated an object (indicated by openingDoorAt just
					//having been reset), or can no longer reach it.
					SetState(PostStriking);
					if (room.IsValidColRow(this->openingDoorAt.wX, this->openingDoorAt.wY))
						//Halph can't reach the orb, so tries to go back to where he was.
						CueEvents.Add(CID_HalphInterrupted, this);
				}
			}
		}
		break;

		case PostStriking:
			if (GetPathTo(CueEvents, this->originSquare))
			{
				VERIFY(TakeStep(CueEvents));	//toward goal
			} else {
				//Halph can't get back to where he was, so just hangs out here.
				SetState(Waiting);
			}
		break;
	}
}

//*****************************************************************************************
bool CHalph::GetPathTo(
//A path to the destination is rechecked each turn in case something got
//in the way since the previous turn.  If the path is no longer open,
//then try to find another path.
//
//Returns: whether an open step toward the destination square exists
//
//Params:
	CCueEvents &CueEvents,  //(out) when Halph hits orb
	const CCoordSet &adjDests, //(in) set of tiles to move adjacent to
	const CCoordSet *pDirectDests)  //(in) set of tiles that must be stepped on directly [default=NULL]
{
	//Confirm path to goal is still open.
	this->pCurrentGame->pRoom->GetSwordCoords(this->swordsInRoom); //speed optimization
	if (!ConfirmPath())
	{
		//If it's not, search for a new path to the goal.
		bool bOrbPathFound = FindOptimalPathTo(this->wX, this->wY, adjDests, true);
		bool bPathFound = bOrbPathFound;
		if (pDirectDests && !pDirectDests->empty())
		{
			CCoordStack orbPath = this->pathToDest;
			bool bPlatePathFound = FindOptimalPathTo(this->wX, this->wY, *pDirectDests, false);
			bPathFound |= bPlatePathFound;

			//Choose shorter path.  With a tie, paths to orbs take preference.
			if (!bPlatePathFound || (bOrbPathFound && this->pathToDest.GetSize() >= orbPath.GetSize()))
				this->pathToDest = orbPath;
		}
		if (!bPathFound)
			return false;  //no path to any goal is available
		this->orbsToHit = adjDests;
		if (pDirectDests)
			this->platesToDepress = *pDirectDests;
		else
			this->platesToDepress.clear();
		//If this is a striking path, then update our goal so we can keep an eye on the
		//existence of the orb/plate at the end of it
		if (this->state == Striking)
			VERIFY(SetGoal());
	}

	//Take first step toward goal.
	if (this->pathToDest.GetSize())
		return true;

	switch (this->state)
	{
		case Striking:
		{
			//If on a destination pressure plate, this movement is done.
			if (this->platesToDepress.has(this->wX, this->wY))
			{
				this->openingDoorAt.wX = this->openingDoorAt.wY = static_cast<UINT>(-1);
				return false;
			}
			//Adjacent to a destination orb?  If so, select and hit it.
			if (this->orbsToHit.empty())
			{
				SetState(Waiting);
				return false;
			}
			UINT destX, destY;
			GetAdjDest(destX, destY);
			ASSERT(this->pCurrentGame->pRoom->IsValidColRow(destX, destY));
			this->pCurrentGame->pRoom->ActivateOrb(destX, destY, CueEvents, OAT_Monster);
			SetOrientation(destX - this->wX, destY - this->wY);
			this->openingDoorAt.wX = this->openingDoorAt.wY = static_cast<UINT>(-1);
			return false;
		}

		case PostStriking:
		{
			//Halph is adjacent to or on the square he was at before going to activate an object.
			//Try to move back to it.
			SetState(Waiting);
			ASSERT(!this->originSquare.empty());
			CCoordSet::const_iterator origin = this->originSquare.begin();
			const UINT wOX = origin->wX, wOY = origin->wY;  //shorthand
			const int dx = wOX - this->wX;
			const int dy = wOY - this->wY;
			if (dx || dy)  //identity check
				if (IsOpenMove(dx, dy))
				{
					this->pathToDest.Push(wOX, wOY);
					return true;
				}
			return false;
		}

		case Entering: return false;

		default: ASSERT(!"Bad state in CHalph::GetPathTo()"); return false;
	}
}

//*****************************************************************************************
bool CHalph::SetGoal()
//Sets bPlateIsGoal and orbLocation based on what we want to depress at our chosen destination.
//Halph only cares what his destination will be; when he finally gets to that destination,
//he will either be standing on a pressure plate from the possible choices, or he will be next
//to one of the orbs he'd identified.  Find out which of these he will choose to strike 'by
//default' and remember these so we can easily check whether this goal gets broken.
//
//Returns: whether a valid orb/plate is at our goal
{
	CDbRoom& room = *this->pCurrentGame->pRoom;
	UINT wDestX, wDestY;
	ASSERT(this->state == Striking);

	if (this->pathToDest.GetSize())
	{
		//Get the last coordinate
		pathToDest.GetAt(0,wDestX,wDestY);
	} else {
		//We may already be at our goal
		wDestX = this->wX;
		wDestY = this->wY;
	}

	if (!room.IsValidColRow(wDestX,wDestY))
		return false;

	//Plates take priority, so check them first
	if (this->platesToDepress.has(wDestX,wDestY))
	{
		//If we don't find a pressure plate here, then our goal has changed
		if (!ConfirmGoal(CCoord(wDestX,wDestY), true))
			return false;

		this->bPlateIsGoal = true;
		this->orbLocation.set(wDestX,wDestY);
		return true;
	}

	//Find first goal orb that is adjacent.
	UINT wOrbX, wOrbY;
	bool bFoundOrb = false;
	for (CCoordSet::const_iterator orb=this->orbsToHit.begin(); orb!=this->orbsToHit.end(); ++orb)
	{
		if (nDist(wDestX, wDestY, orb->wX, orb->wY) <= 1)
		{
			wOrbX = orb->wX;
			wOrbY = orb->wY;
			bFoundOrb = true;
			break;
		}
	}
	//No goal found
	if (!bFoundOrb)
		return false;

	//If we don't find an orb here, then our goal has changed
	if (!ConfirmGoal(CCoord(wOrbX,wOrbY), false))
		return false;

	this->bPlateIsGoal = false;
	this->orbLocation.set(wOrbX,wOrbY);
	return true;
}

//*****************************************************************************************
bool CHalph::ConfirmGoal(const CCoord& location, const bool bPlateIsGoal)
//Checks whether our chosen orb or plate still exists.
//Since plates and orbs cannot change what they're associated with mid-room, all
//we need to check is that the orb/plate is present at the co-ordinates and that
//it isn't broken.
//
//Returns: whether the orb or plate we were going to use at
//   the end of our path is still available
{
	CDbRoom& room = *this->pCurrentGame->pRoom;
	ASSERT(this->state == Striking);
	
	if (!room.IsValidColRow(location.wX,location.wY))
		return false;

	if (bPlateIsGoal)
	{
		if (room.GetOSquare(location.wX,location.wY) != T_PRESSPLATE)
			return false;
		COrbData *pOrb = room.GetPressurePlateAtCoords(location.wX,location.wY);
		//If the plate is broken, then our goal has changed
		if (!pOrb || pOrb->eType == OT_BROKEN)
			return false;
	} else {
		if (room.GetTSquare(location.wX,location.wY) != T_ORB)
			return false;

		COrbData *pOrb = room.GetOrbAtCoords(location.wX,location.wY);
		//If the orb is broken, then our goal has changed
		if (!pOrb || pOrb->eType == OT_BROKEN)
			return false;
	}
	return true;
}

//*****************************************************************************************
bool CHalph::IsOpeningDoorAt(const CCoordSet& doorSquares) const
//Returns: whether Halph is going to open a door square that is part of
//the given set of coords.
{
	if (this->state != Striking)
		return false;
	return doorSquares.has(this->openingDoorAt);
}

//*****************************************************************************************
void CHalph::SetState(const HalphState eState)
{
	this->state = eState;
	this->wMoves = 0;
	if (eState != Striking)
		this->pathToDest.Clear();
	//Don't reset this->originSquare
}

//*****************************************************************************************
void CHalph::UseObjectToOpenDoorAt(const UINT wX, const UINT wY)
//Tell Halph to use one of the orbs/plates found in FindOptimalPathTo().
{
	this->openingDoorAt.wX = wX;
	this->openingDoorAt.wY = wY;
	SetState(Striking);

	//Go back to this square when done.
	this->originSquare.clear();
	this->originSquare.insert(this->wX, this->wY);
}

//*****************************************************************************************
void CHalph::SwordsmanBumps(CCueEvents &CueEvents)
//Player bumps into Halph, signaling him to either follow or wait.
{
	switch (this->state)
	{
		case Entering:
		case Waiting:
		case Striking:
		case PostStriking:
			SetState(Following);
			CueEvents.Add(CID_HalphFollowing, this);
		break;

		case Following:
			SetState(Waiting);
			CueEvents.Add(CID_HalphWaiting, this);
		break;
	}
}

//******************************************************************************************
bool CHalph::TakeStep(CCueEvents& CueEvents)
//Take another step toward goal along pre-calculated path.
{
	ASSERT(this->pathToDest.GetSize());

	UINT wNextX, wNextY;
	if (!this->pathToDest.Top(wNextX, wNextY))
		return false;
	const CSwordsman& player = this->pCurrentGame->swordsman;
	if ((player.wX == wNextX && player.wY == wNextY) ||
			this->pCurrentGame->IsPlayerWeaponAt(wNextX, wNextY))
		return false;  //never step on player or player's sword

	const int dx = wNextX - this->wX;
	const int dy = wNextY - this->wY;
	Move(wNextX, wNextY, &CueEvents);
	this->pathToDest.Pop();  //Halph has now made this move
	SetOrientation(dx, dy);
	return true;
}

//******************************************************************************************
bool CHalph::DoesSquareContainObstacle(
//Override for Halph.  (Copied and revised code from CMonster::DoesSquareContainObstacle.)
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	ASSERT(this->pCurrentGame);
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow))
		return true;

	//No monsters can ever be stepped on.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
		return true;

	//Some layer objects are obstacles.  Check for these.
	if (IsTileObstacle(room.GetOSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetTSquare(wCol, wRow)))
		return true;
	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Can't step on any swords.
	if (this->swordsInRoom.Exists(wCol, wRow)) //this set is compiled at beginning of move
		return true;

	//Player can never be stepped on.
	return this->pCurrentGame->IsPlayerAt(wCol, wRow);
}

//*****************************************************************************
bool CHalph::IsTileObstacle(
//Override for Halph.
//
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.  Note each tile# will always be
						//    found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	return (
			//Halph doesn't like active firetraps
			wLookTileNo == T_FIRETRAP_ON ||
			CMonster::IsTileObstacle(wLookTileNo)
			);
}

//******************************************************************************************
void CHalph::GetAdjDest(
//Returns the first coord in list adjacent to current position, or (-1,-1) if none.
//
//Params:
	UINT& wDestX, UINT& wDestY)   //(out) First coord in list adjacent to current position
const
{
	ASSERT(!this->orbsToHit.empty());
	for (CCoordSet::const_iterator orb=this->orbsToHit.begin(); orb!=this->orbsToHit.end(); ++orb)
	{
		if (nDist(this->wX, this->wY, orb->wX, orb->wY) <= 1)
		{
			wDestX = orb->wX;
			wDestY = orb->wY;
			return;
		}
	}
	wDestX = wDestY = static_cast<UINT>(-1);
}

//***************************************************************************************
void CHalph::RequestOpenDoor(
//Search for orbs that Halph can hit to open the door at (wX, wY).
//Start Halph moving toward the closest orb of these, if any.
//If Halph is already going to open this door, do nothing.
//
//Params:
	const UINT wX, const UINT wY,  //(in)
	CCueEvents &CueEvents)         //(in/out)
{
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	CCoordSet doorSquares;
	room.GetAllYellowDoorSquares(wX, wY, doorSquares);

	//Is Halph already going to activate this object?  If so, then shouldn't do anything here.
	if (IsOpeningDoorAt(doorSquares))
	{
		CueEvents.Add(CID_HalphHurryUp, this); //Halph responds to player's impatience
		return;
	}

	//Compile list of all orbs that can open this door.
	CCoordSet orbs, plates;
	room.FindOrbsToOpenDoor(orbs, doorSquares);
	room.FindPlatesToOpenDoor(plates, doorSquares);

	//Tell Halph to try to hit any of these orbs or step on these plates.
	room.GetSwordCoords(this->swordsInRoom); //speed optimization
	bool bPlatePathFound = false;
	bool bOrbPathFound = FindOptimalPathTo(this->wX, this->wY, orbs);
	if (bOrbPathFound)
	{
		//Halph has a path to an orb for this door.  He will go strike it.
		this->orbsToHit = orbs;
		this->platesToDepress = plates; //remember these in case orbs get blocked
		UseObjectToOpenDoorAt(wX, wY);
		CueEvents.Add(CID_HalphStriking, this);
	}

	//If pressure plates that could open the door exist, check these also.
	//If a valid path is found.
	if (!plates.empty())
	{
		const CCoordStack orbPath = this->pathToDest; //retain this path
		CCoordSet depressablePlates;
		room.GetDepressablePlateSubset(plates, depressablePlates);
		
		bPlatePathFound = FindOptimalPathTo(this->wX, this->wY, depressablePlates,
				false); //must step on plate, not be adjacent to it
		if (!bPlatePathFound) {
			this->pathToDest = orbPath;
		} else {
			this->orbsToHit = orbs; //remember these in case plates get blocked
			this->platesToDepress = plates;
			UseObjectToOpenDoorAt(wX, wY);
			CueEvents.Add(CID_HalphStriking, this);

			//Choose shorter path.  With a tie, paths to orbs take preference.
			if (bOrbPathFound && this->pathToDest.GetSize() >= orbPath.GetSize())
				this->pathToDest = orbPath;
		}
	}

	if (!bOrbPathFound && !bPlatePathFound)
	{
		//Halph can't reach any objects to open this door.
		CueEvents.Add(CID_HalphCantOpen, this);
	} else {
		//Set our new goal so we can make sure our chosen orb/plate still exists each turn
		VERIFY(SetGoal());
	}
}
