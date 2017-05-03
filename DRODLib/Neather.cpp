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
 * John Wm. Wicks (j_wicks)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Neather.h"
#include "CurrentGame.h"
#include "DbRooms.h"
#include "../Texts/MIDs.h"

//
//Public methods.
//

//*****************************************************************************************
void CNeather::Process(
//Process a neather for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to an IDList object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	//If stunned, skip turn
	if (IsStunned())
		return;


	if (!CanFindSwordsman())
		return;

	int dxFirst=0, dyFirst=0, dx=0, dy=0;
	GOAL &pGoal = this->m_CurrentGoal;

	this->bStrikingOrb = false;

	if (GetGoal(pGoal,CueEvents))
	{
		if (pGoal.eType != waitTurn)
		{
			const UINT nTargetY = pGoal.eType==strikeorb ? (UINT)pGoal.nY-1 : (UINT)pGoal.nY;
			if ((UINT)pGoal.nX > this->wX)
				dx=dxFirst=1;
			else if ((UINT)pGoal.nX < this->wX)
				dx=dxFirst=-1;
			else
				dx=dxFirst=0;
			if (nTargetY > this->wY)
				dy=dyFirst=1;
			else if (nTargetY < this->wY)
				dy=dyFirst=-1;
			else
				dy=dyFirst=0;

			if (!GetGoalDirectedMovement(dx, dy))
			{
				dx=0; dy=0;
			}
		}
	}
	
	if (pGoal.eType != exitroom)
	{
		if (pGoal.eType==strikeorb)
		{
			// Make sure we're over the correct orb if there are multiple orbs
			if ((UINT)pGoal.nX==(this->wX) && (UINT)pGoal.nY==(this->wY+1))
			{
				//Is Neather in position to hit an orb?
				if (this->pCurrentGame->pRoom->GetTSquare(this->wX, this->wY+1)==T_ORB)
				{
					this->pCurrentGame->pRoom->ActivateOrb(pGoal.nX, pGoal.nY, CueEvents, OAT_Monster);
					this->bStrikingOrb = true;
					if (this->bLaughWhenOrbHit)
					{
						CueEvents.Add(CID_NeatherLaughing, this);
						this->bLaughWhenOrbHit = false;
					}
				}
			}
		}

		//Move neather to new destination square.
		MakeStandardMove(CueEvents,dx,dy);
		if (dxFirst || dyFirst) //'Neather never shown "standing still"
			SetOrientation(dxFirst, dyFirst);
	}
	else
	{
		this->bIsFirstTurn=false;
		CueEvents.Add(CID_MonsterExitsRoom, this);
		return;
	}
}

//*****************************************************************************************
bool CNeather::GetGoal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it. Calls routines specific to the current room.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	int nX, nY;
	bool bRetVal=false;
	
	//Call a GetGoalX() routine based on current room.
	nX=this->pCurrentGame->pRoom->dwRoomX;
	nY=this->pCurrentGame->pRoom->dwRoomY % 100; //allow to function in a copied version of the level
	switch (nX)
	{
		case 48:
			switch (nY)
			{
				case 49: bRetVal=GetRoom4Goal(pGoal,CueEvents); break;
				case 50: bRetVal=GetRoom5Goal(pGoal,CueEvents); break;
				case 51: bRetVal=GetRoom6Goal(pGoal,CueEvents); break;
				case 52: bRetVal=GetRoom7Goal(pGoal,CueEvents); break;
				default: break;
			}
			 break;
		case 49:
			switch (nY)
			{
				case 49: bRetVal=GetRoom3Goal(pGoal,CueEvents);  break;
				case 50: bRetVal=GetRoom12Goal(pGoal,CueEvents);  break;
				case 51: bRetVal=GetRoom11Goal(pGoal,CueEvents);  break;
				case 52: bRetVal=GetRoom8Goal(pGoal,CueEvents);  break;
				default: break;
			}
			break;
		case 50:
			switch (nY)
			{
				case 49: bRetVal=GetRoom2Goal(pGoal,CueEvents); break;
				case 50: bRetVal=GetRoom1Goal(pGoal,CueEvents); break;
				case 51: bRetVal=GetRoom10Goal(pGoal,CueEvents); break;
				case 52: bRetVal=GetRoom9Goal(pGoal,CueEvents); break;
				default: break;
			}
			break;
		default: break;
	}//...switch (nX)
	
	if (!bRetVal) {pGoal.eType=waitTurn;}
	
	return bRetVal;
}

//*****************************************************************************
bool CNeather::SetGoal(
//Sets a goal for the Neather to achieve
//
//Params:
	const int nX,           //(in) X coordinate of Goal to set
	const int nY,           //(in) Y coordinate of Goal to set
	const GOALTYPE eType,   //(in) tagGoalType of Goal to set none=0,strikeorb=1,move=2
	GOAL &pGoal,            //(out) Accepts pointer to an GOAL structure
									//that will be updated by this method
	const bool bLaugh)      //(in) whether 'Neather should laugh when orb is hit
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;

	pGoal.eType=eType;
	pGoal.nX=nX;
	pGoal.nY=nY;

	ASSERT(!bLaugh || eType == strikeorb); //laugh only when hitting orb
	this->bLaughWhenOrbHit = bLaugh;

	return (bRetVal=(pGoal.eType==eType&&pGoal.nX==nX&&pGoal.nY==nY));
}

//*****************************************************************************
bool CNeather::GetGoalDirectedMovement(
//Gets a goal directed movement for the current room
//and the state of inside it.
//
//Returns:
//true if successful, false otherwise.
//
//Params:
	int &dx,       //(out) Horizontal delta (-1, 0, or 1) for where monster
						//    can go, taking into account obstacles.
	int &dy)       //(out) Vertical delta (-1, 0, or 1) for same.
const
{
	GetBestMove(dx, dy);

	return true;
}

//*****************************************************************************
inline bool CNeather::SquareFree(
//Returns: whether the Neather, swordsman, and his sword don't occupy this square
//
//Params:
	const UINT sx, const UINT sy)
{
	return !((this->wX==sx && this->wY==sy) ||
			this->pCurrentGame->IsPlayerAt(sx, sy) ||
			(this->pCurrentGame->swordsman.wSwordX==sx &&
			this->pCurrentGame->swordsman.wSwordY==sy));
}

//*****************************************************************************
bool CNeather::OnAnswer(
//Overridable method for responding to an answer given by player to a question asked by the
//monster.
//
//Params:
	int nCommand,        //(in)   CMD_YES or CMD_NO.
	CCueEvents &CueEvents)  //(out)  Add cue events if appropriate.
//
//Returns:
//True if any cue events were added, false if not.
{
	bool bRetVal=false;
	UINT x=0, y=0;

	ASSERT(nCommand == CMD_YES || nCommand == CMD_NO);
	ASSERT(this->pCurrentGame->pRoom->dwRoomX==49 && this->pCurrentGame->pRoom->dwRoomY%100==50);

	if (nCommand == CMD_YES)
	{
		//Go to one of the empty squares if Swordsman spared Neather.
		if (this->m_CurrentState==rx_stSeven)
		{
			if (SquareFree(18,18)) {x=18; y=18;}
			else if (SquareFree(17,18)) {x=17; y=18;}
			else if (SquareFree(18,17)) {x=18; y=17;}
			else if (SquareFree(17,17)) {x=17; y=17;}
			else {ASSERTP(false, "Bad state for OnAnswer().");}
		}
		else
		{
			if (SquareFree(22,16)) {x=22; y=16;}
			else if (SquareFree(22,17)) {x=22; y=17;}
			else if (SquareFree(23,17)) {x=23; y=17;}
			else {ASSERTP(false, "Bad state for OnAnswer().");}
		}
		Move(x, y, &CueEvents);
		ASSERT(IsValidOrientation(this->wO));
	}
	else
	{
		CueEvents.Add(CID_MonsterDiedFromStab, this);
		bRetVal=true;
	}
			
	return bRetVal;
}

//******************************************************************************************
bool CNeather::OnStabbed(
//Overridable method to process the effects of a monster being stabbed.
//Neather asks Swordsman to spare its life if in the last room otherwise there has been some
//sort of coding error since the Neather should not be able to be killed except in the last
//room.
//
//Params:
	CCueEvents &CueEvents,  //(in) Accepts pointer to an IDList object that will be populated
							//codes indicating events that happened that may correspond to
							//sound or graphical effects.
	const UINT /*wX*/, const UINT /*wY*/,  //(in) unused
	WeaponType /*weaponType*/)
//
//Returns:
//True if any cue events were returned, false if not.
{
	CueEvents.Add(CID_NeatherScared, this);

	if (this->pCurrentGame->pRoom->dwRoomX==49 && this->pCurrentGame->pRoom->dwRoomY%100==50)
		AskYesNo(MID_PleaseSpareMe, CueEvents);
	else
		Say(MID_UnexpectedNeatherDeath, CueEvents);
	return true;
}

//*****************************************************************************************
bool CNeather::GetRoom1Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &/*CueEvents*/) //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;

	//Run from the swordsman.
	if (this->wY > 10)
		bRetVal = SetGoal(13, 10, ::move, pGoal);
	else if(this->wX==16 && this->wY==0) // move the neather off the board
		bRetVal = SetGoal(-1, -1, exitroom, pGoal);
	else
		bRetVal = SetGoal(16, 0, ::move, pGoal);

	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom2Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	bool bDoorOpen;
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = r2_stMovingSWPassage;

	//Slam the first door shut in the swordsman's face.
	if (this->pCurrentGame->pRoom->IsDoorOpen(25, 28)) {
		return SetGoal(27, 29, strikeorb, pGoal,
				IsInRect(swX, swY, 19, 28, 24, 28));   //laugh if player is in corridor
	}

	//If it's already closed then next move east to the corner of the passage.
	if (IsInRect(this->wX, this->wY, 27, 28, 34, 28)) {return SetGoal(34, 27, ::move, pGoal);}

	//If Neather has gotten past SW corner of passage, head north to the orb and wait.
	if (this->m_CurrentState==r2_stMovingSWPassage && (this->wY>11 || (this->wY==11 && this->wX!=31)) )
		{return SetGoal(31, 11, ::move, pGoal);}

	//If the door to the trap area is still open and the swordsman has just stepped inside then
	//hit the orb to spring the trap.
	bDoorOpen=this->pCurrentGame->pRoom->IsDoorOpen(21, 21);
	if (bDoorOpen && IsInRect(this->pCurrentGame->swordsman.wX,
			this->pCurrentGame->swordsman.wY, 19, 20, 23, 20))
	{ //Spring the trap.
		this->m_CurrentState = r2_stSpringingTrap;
	}
	if (!bDoorOpen && this->m_CurrentState==r2_stSpringingTrap)
		{this->m_CurrentState=rx_stWaiting;}  //Wait for swordsman to die in the trap.
	if (this->m_CurrentState==r2_stSpringingTrap) {
		return SetGoal(31, 12, strikeorb, pGoal, true);
	}

	//If the swordsman steps onto the entrance square of the NW passageway then
	//it is time for the Neather to flee.
	if (this->pCurrentGame->swordsman.wX==6 && this->pCurrentGame->swordsman.wY==11)
	{
		CueEvents.Add(CID_NeatherFrustrated, this);
		this->m_CurrentState=rx_stFleeing;
	}

	//If flee flag is set, negotiate Neather's path through escape tunnel.
	if (this->m_CurrentState==rx_stFleeing)
	{
		//E corridor to N corridor.
		if (IsInRect(this->wX, this->wY, 31, 3, 34, 11))
			{bRetVal=SetGoal(34, 2, ::move, pGoal);}
		else if (this->wX==0&&this->wY==13)
			{bRetVal=SetGoal(-1, -1, exitroom, pGoal);}
		//W corridor to exit.
		else if (this->wX<4)
			{bRetVal=SetGoal(0, 13, ::move, pGoal);}
		//N corridor to W corridor;
		else
			{bRetVal=SetGoal(3, 2, ::move, pGoal);}
		return bRetVal;
	}
			
	//Wait for one of the above events to happen.
	bRetVal=SetGoal(this->wX, this->wY, waitTurn, pGoal);  
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom3Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	bool bRoachesAreInLock, bSouthDoorIsOpen;
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	
	//Get Neather state.
	if (this->m_CurrentState==0)
		this->m_CurrentState = rx_stGuardingFirstDoors;

	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stGuardingFirstDoors: //Keep swordsman inside the first three doors.
				if (IsInRect(swX, swY, 26, 11, 33, 13) 
					&& this->pCurrentGame->pRoom->IsDoorOpen(30, 10) )  //Close North door.
						{return SetGoal(29, 2, strikeorb, pGoal);}
				else if (IsInRect(swX, swY, 26, 14, 29, 18)
					&& this->pCurrentGame->pRoom->IsDoorOpen(25, 16) )  //Close West door.
						{return SetGoal(30, 2, strikeorb, pGoal);}
				else if (IsInRect(swX, swY, 30, 15, 36, 18)
					&& this->pCurrentGame->pRoom->IsDoorOpen(33, 19) )  //Close South door.
						{return SetGoal(28, 2, strikeorb, pGoal);}
				else
					//Check for swordsman moving out of three door area.
					if (IsInRect(swX, swY, 28, 9, 32, 10) || //Escape through North door.
							IsInRect(swX, swY, 24, 14, 25, 18) || //West door.
							IsInRect(swX, swY, 31, 19, 35, 20) )
					{
						CueEvents.Add(CID_NeatherFrustrated, this);
						this->m_CurrentState=r3_stClosingNWPassage;
						break;
					}
				//If swordsman has not left three door area.  Move to center position
				// for quick access to all 3 orbs.
				return SetGoal(29, 1, ::move, pGoal);
				break;
			
			case r3_stClosingNWPassage:
				//Swordsman has escaped 3 door area.  Neather tries to head him off by
				//shutting off the northwest passage.
				//
				//Or swordsman has already gone through the passage and tried to go
				//back to 3 door area after Neather reopened it.
				if (this->pCurrentGame->pRoom->IsDoorOpen(22, 6)) //Close it.
					{return SetGoal(16, 2, strikeorb, pGoal);}
				else
					{this->m_CurrentState=r3_stMovingNWControlArea; break;}
			
			case r3_stMovingNWControlArea:
				//Northwest passage has been closed whether the swordsman made it
				//through or not. Head to the northwest control area.
				if (this->wX==3 && this->wY==4) {this->m_CurrentState=r3_stReleasingRoaches; break;}
				return SetGoal(3, 4, ::move, pGoal);
			
			case r3_stReleasingRoaches:
			{
				//Got to NW control area.  Release roaches from Queen room
				//when they get to the roach lock.  
					
				//If no monsters are in the Queen room or lock then go to flee step.
				if (!this->pCurrentGame->pRoom->IsMonsterInRect(4, 10, 12, 17))
				{
					CueEvents.Add(CID_NeatherFrustrated, this);
					this->m_CurrentState=rx_stFleeing;
					break;
				}

				//Check for a good time to release roaches from the 3 door area.
				//then Open door to 3 door area.
				const bool bSCloseTo3DoorArea = IsInRect(swX, swY, 19, 5, 21, 30);
				bSouthDoorIsOpen=this->pCurrentGame->pRoom->IsDoorOpen(8, 17);
				if (!this->pCurrentGame->pRoom->IsDoorOpen(22,6)
					&& !bSCloseTo3DoorArea && !bSouthDoorIsOpen)
						{this->m_CurrentState=r3_stOpening3DoorAreaToRoach; break;}

				//Close door to 3 door area so swordsman won't get in.
				if (this->pCurrentGame->pRoom->IsDoorOpen(22, 6)
					&& bSCloseTo3DoorArea)
						{this->m_CurrentState=r3_stClosingNWPassage; break;} 

				//Keep roaches flowing through the lock.
				bRoachesAreInLock=this->pCurrentGame->pRoom->IsMonsterInRect(4, 15, 12, 17);
				//Close north door, open south door.
				if (bRoachesAreInLock && !bSouthDoorIsOpen)
					{return SetGoal(4, 5, strikeorb, pGoal);}
				//Close south door, open north door.
				if (!bRoachesAreInLock && bSouthDoorIsOpen)
					{return SetGoal(2, 5, strikeorb, pGoal);}
				else //Stay between the two orbs.
					{return SetGoal(3, 4, ::move, pGoal);} 
			}

			case r3_stOpening3DoorAreaToRoach: //Let roaches through from the 3 door area.
				if (this->pCurrentGame->pRoom->IsDoorOpen(22, 6)) //Go back to NW control area.
					{this->m_CurrentState=r3_stReleasingRoaches; break;}
				else                                //Open door.
					{return SetGoal(16, 2, strikeorb, pGoal);}

			case rx_stFleeing: //Flee!
				if (this->wX ==0 && this->wY==2)
					return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(0, 2, ::move, pGoal);

			default:
				ASSERTP(false, "Bad state in room 3."); return false;
		}
	}

	ASSERTP(false, "Bad flow in room 3.");
	pGoal.eType=waitTurn;

	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom4Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	
	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
		
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stGuardingFirstDoors: //Swordsman in rooms A or B after first entrance.
				//If North exit to room B is open and Swordsman in Room A close the door.
				if (IsInRect(swX, swY, 22, 24, 28, 30) && this->pCurrentGame->pRoom->IsDoorOpen(25, 25))
					{return SetGoal(31, 5, strikeorb, pGoal,true);}
				if (IsInRect(swX, swY, 31, 17, 34, 21))
					{this->m_CurrentState=r4_stGuardingRoomC; break;}//Swordsman entered room C.
				else //Swordsman not in room C yet.
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(32, 23))//Open south entrance of room C.
						{return SetGoal(30, 5, strikeorb, pGoal);}
				}
			return true;
			
			case r4_stGuardingRoomC:  //Swordsman in room C.
				if (this->pCurrentGame->pRoom->IsDoorOpen(32, 23))//Close south entrance of room C.
					{return SetGoal(30, 5, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 29, 13, 36, 15))
					{this->m_CurrentState=r4_stGuardingRoomD; break;}//Swordsman just got to room D.
				else //Swordsman not in room D yet.
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(32, 16))//Open south entrance of room D.
						{return SetGoal(29, 5, strikeorb, pGoal);}
				}
			return true;
			
			case r4_stGuardingRoomD:  //Swordsman in room D.
				if (this->pCurrentGame->pRoom->IsDoorOpen(32, 16))//Close south entrance of room D.
					{return SetGoal(29, 5, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 22, 12, 27, 17))
					{this->m_CurrentState=r4_stGuardingRoomE; break;}//Swordsman just got to room E.
				else
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(28, 14))
						{return SetGoal(28, 5, strikeorb, pGoal);}
				}                 
			return true;

			case r4_stGuardingRoomE: //Swordsman in room E and beyond.
				if (IsInRect(swX, swY, 14, 24, 21, 27))
				{  //Time to get out of the room.
					CueEvents.Add(CID_NeatherScared, this);
					this->m_CurrentState=rx_stFleeing;
					break;
				}

				//Check for doors that Swordsman is too close too.
				if ((IsInRect(swX, swY, 20, 13, 23, 17) && this->pCurrentGame->pRoom->IsDoorOpen(19, 15)) ||
					(!IsInRect(swX, swY, 20, 13, 23, 17) && !this->pCurrentGame->pRoom->IsDoorOpen(19, 15))) //West door of room E.
					{return SetGoal(25, 5, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 22, 18, 25, 24) && this->pCurrentGame->pRoom->IsDoorOpen(21, 21)) ||
					(!IsInRect(swX, swY, 22, 18, 25, 24) && !this->pCurrentGame->pRoom->IsDoorOpen(21, 21))) //West door of room F.
					{return SetGoal(24, 5, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 26, 7, 31, 11) && this->pCurrentGame->pRoom->IsDoorOpen(32, 9)) ||
					(!IsInRect(swX, swY, 26, 7, 31, 11) && !this->pCurrentGame->pRoom->IsDoorOpen(32, 9))) //West door of room G.
					{return SetGoal(26, 5, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 29, 13, 36, 15) && this->pCurrentGame->pRoom->IsDoorOpen(34, 12)) ||
					(!IsInRect(swX, swY, 29, 13, 36, 15) && !this->pCurrentGame->pRoom->IsDoorOpen(34, 12))) //South door of room G.
					{return SetGoal(27, 5, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 13, 3, 19, 7) && this->pCurrentGame->pRoom->IsDoorOpen(16, 4)) ||
					(!IsInRect(swX, swY, 13, 3, 19, 7) && !this->pCurrentGame->pRoom->IsDoorOpen(16, 4))) //North door of room H.
					{return SetGoal(22, 5, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 13, 9, 21, 13) && this->pCurrentGame->pRoom->IsDoorOpen(16, 12)) ||
					(!IsInRect(swX, swY, 13, 9, 21, 13) && !this->pCurrentGame->pRoom->IsDoorOpen(16, 12))) //South door of room H.
					{return SetGoal(23, 5, strikeorb, pGoal);}
			return true;

			case rx_stFleeing:   //Flee to fork.
				if (IsInRect(this->wX, this->wY, 1, 1, 1, 5)) {return SetGoal(2, 6, ::move, pGoal);}
				if (this->wX==2 && this->wY==6) {this->m_CurrentState=r4_GuardingNWFork; break;} //Got there.
				else
					return SetGoal(1, 1, ::move, pGoal); // Move to NW Corner
			return true;

			case r4_GuardingNWFork: //At the fork, wait until the swordsman steps into one of the two passages.
				if (IsInRect(swX, swY, 1, 11, 1, 13)) //Swordsman is in west passage.
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(1, 10)) //Close west passage.
							{return SetGoal(2, 7, strikeorb, pGoal);}
						else
							{this->m_CurrentState=r4_stGuardingOrbPit; break;}//Swordsman is trapped.  Okay to proceed.
					}
				if (IsInRect(swX, swY, 3, 11, 3, 13)) //Swordsman is in east passage.
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(3, 10)) //Close East passage.
							{return SetGoal(3, 7, strikeorb, pGoal);}
						else
							{this->m_CurrentState=r4_stGuardingOrbPit; break;} //Swordsman is trapped.  Okay to proceed.
					}
				
				//Otherwise Neather just waits for Swordsman.
			return true;
			
			case r4_stGuardingOrbPit: //Move to North entrance of orb pit.
				if (IsInRect(this->wX, this->wY, 7, 3, 7, 6) && !(this->pCurrentGame->pRoom->IsDoorOpen(17, 28))) 
					{return SetGoal(7, 7, strikeorb, pGoal);}  //Got there, now strike the orb.
				if (this->pCurrentGame->pRoom->IsDoorOpen(17, 28)) {this->m_CurrentState=r4_stLeavingOrbPit; break;} //Struck the orb.
				else
					return SetGoal(7, 3, ::move, pGoal);
			
			case r4_stLeavingOrbPit: //Leave the orb pit.
				if (this->wX==3 && this->wY==3) {this->m_CurrentState=r4_stReturningToNWFork; break;} //Got there.
				else
					return SetGoal(3, 3, ::move, pGoal);
			
			case r4_stReturningToNWFork: //Go back to the fork.
				if (this->wX==1 && this->wY==8) {this->m_CurrentState=r4_stFindingOpenPassage; break;} //Got there.
				else
					return SetGoal(1, 8, ::move, pGoal);
			
			case r4_stFindingOpenPassage: //Go through whichever passage is open.
				if (IsInRect(swX, swY, 1, 11, 1, 13))  //Swordsman is in west passage.
				{
					if (this->wX==3 && this->wY==15) {this->m_CurrentState=rx_stMovingSWCorner; break;}  //Through the passage.
					else
						return SetGoal(3, 15, ::move, pGoal);
				}
				
				//Swordsman is in east passage.
				if (this->wX==1 && this->wY==15) {this->m_CurrentState=rx_stMovingSWCorner; break;}  //Through the passage.
				else
					return SetGoal(1, 15, ::move, pGoal); 
			
			case rx_stMovingSWCorner: //Go to the southwestmost corner.
				if (this->wX==1 && this->wY==30) {this->m_CurrentState=rx_stMovingSECorner; break;} //Got there.
				else
					return SetGoal(1, 30, ::move, pGoal);
			
			case rx_stMovingSECorner: //Go to the next southeast corner.   
				if (this->wX==13 && this->wY==30) {this->m_CurrentState=rx_stMovingNNECorner; break;} //Got there.
				else
					return SetGoal(13, 30, ::move, pGoal);
			
			case rx_stMovingNNECorner: //Go to the n-w-e point.
				if (this->wX==13 && this->wY==27) {this->m_CurrentState=r4_stOpeningDoorsForSwordsman; break;} //Got there.
				else
					return SetGoal(13, 27, ::move, pGoal);
			
			case r4_stOpeningDoorsForSwordsman: //Strike orb to release S.
				if (this->pCurrentGame->pRoom->IsDoorOpen(1, 14) && this->pCurrentGame->pRoom->IsDoorOpen(3, 14)) 
					{this->m_CurrentState=rx_stGuardingLastEntrance; break;}  //Struck it.
				else
					return SetGoal(11, 28, strikeorb, pGoal);
			return true;
			
			case rx_stGuardingLastEntrance: //Go to last room entrance.
				if (this->wX==14 && this->wY==27) {this->m_CurrentState=rx_stClosingLastEntrance; break;} //Got there.
				else
					return SetGoal(14, 27, ::move, pGoal);
			
			case rx_stClosingLastEntrance: //Strike the orb that closes the room exit door.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(17, 28)) {this->m_CurrentState=rx_stLeavingRoom; break;} //Struck it.
				else
					return SetGoal(20, 30, strikeorb, pGoal);

			case rx_stLeavingRoom: //Leave.
				if(this->wX==18 && this->wY==31) return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(18, 31, ::move, pGoal);
			
			default:
				ASSERTP(false, "Bad state in room 4."); return false;
		}
	}
	ASSERTP(false, "Bad flow in room 4.");
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom5Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx: //Start
				// Reached first orb in central trap.
				if (this->wX==18 && this->wY==13)
					{this->m_CurrentState=r5_SettingCentralTrapStep1; break;}
				else
					return SetGoal(18, 13, ::move, pGoal);
			
			case r5_SettingCentralTrapStep1: //Set up the central trap.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(22, 14))
					{this->m_CurrentState=r5_SettingCentralTrapStep2; break;}
				else
					return SetGoal(20, 17, strikeorb, pGoal);
			
			case r5_SettingCentralTrapStep2: //Set up the central trap.
				if (this->wX==17 && this->wY==16)
					{this->m_CurrentState=r5_SettingCentralTrapStep3; break;}
				else
					return SetGoal(17, 16, ::move, pGoal);
			
			case r5_SettingCentralTrapStep3: //Set up the central trap.
				if (this->wX==12 && this->wY==15)
					{this->m_CurrentState=r5_SettingCentralTrapStep4; break;}
				else
					return SetGoal(12, 15, ::move, pGoal);

			case r5_SettingCentralTrapStep4: //Set up the central trap.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(13, 14))
					{this->m_CurrentState=r5_SpringingTrap; break;}
				else
					return SetGoal(11, 17, strikeorb, pGoal);
			
			case r5_SpringingTrap: //Done setting up the central trap.  Get ready to spring it.
				if (!(this->wX==7 && this->wY==16))
					{return SetGoal(7, 16, ::move, pGoal);} //Move over the orb to hit.
				if (IsInRect(swX, swY, 14, 11, 21, 18))
					{
						if (!this->pCurrentGame->pRoom->IsDoorOpen(17, 6))
							{return SetGoal(7, 17, strikeorb, pGoal, true);}
						else
							{this->m_CurrentState=r5_stGuardingRoomA; break;} //Trap sprung.
					}
				//Wait for Swordsman to enter central trap.
				return true;               
			
			case r5_stGuardingRoomA:  //Move above the orb that controls the east door of room A.
				if (this->wX==11 && this->wY==16) {this->m_CurrentState=r5_stGuardingSouthPassage; break;}
				else
					return SetGoal(11, 16, ::move, pGoal);
			
			case r5_stGuardingSouthPassage: //Wait for swordsman to enter the south passage below the central room.
				if (IsInRect(swX, swY, 16, 21, 19, 21)) {this->m_CurrentState=r5_stSpringingSouthPassageTrap; break;}
				//Wait in room A, but close the east door if S opens it.
				if (this->pCurrentGame->pRoom->IsDoorOpen(13, 14) && swX==14)
					{return SetGoal(11, 17, strikeorb, pGoal, true);}
				return true;
			
			case r5_stSpringingSouthPassageTrap: //Spring the south passage trap.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(13, 22)) 
					{return SetGoal(8, 17, strikeorb, pGoal, true);}
				else if (!(this->wX==11 && this->wY==16)) //Get back to the orb that controls east room A exit.
					{return SetGoal(11, 16, ::move, pGoal);}
				else
					{this->m_CurrentState=r5_stGuardingEastDoorRoomA; break;}
			
			case r5_stGuardingEastDoorRoomA: //Keep hitting the east door to room A to keep it closed.
				//swordsman steps into the south passage flee trigger. Decide to escape room A.
				if (IsInRect(swX, swY, 9, 18, 12, 24)) {
					CueEvents.Add(CID_NeatherScared, this);
					this->m_CurrentState=r5_stEscapingRoomA;
					break;
				}

				//If the swordsman is near, close the door.
				if (IsInRect(swX, swY, 14, 14, 14, 15) && this->pCurrentGame->pRoom->IsDoorOpen(13, 14))
					{return SetGoal(11, 17, strikeorb, pGoal, true);}
				return true;
				
			case r5_stEscapingRoomA:  //Get ready to flee from Room A.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(8, 19))//Release some trouble for Swordsman.
					{return SetGoal(10, 17, strikeorb, pGoal, true);}
				else if (!this->pCurrentGame->pRoom->IsDoorOpen(13, 14))//Open the east door to room A for an escape.
					{return SetGoal(11, 17, strikeorb, pGoal);}
				else if (!(this->wX==12 && this->wY==15)) //Move up a little.
					{return SetGoal(12, 15, ::move, pGoal);}
				else
					{this->m_CurrentState=r5_RetreatingEastSideCentralRoom; break;}
			
			case r5_RetreatingEastSideCentralRoom: //Retreating to east side of central room.
				 //Slow Swordsman down some.
				if (IsInRect(swX, swY, 8, 13, 12, 17) && this->pCurrentGame->pRoom->IsDoorOpen(13, 15) && this->wX<16)
					{return SetGoal(15, 17, strikeorb, pGoal);}
				else if (this->wX==20 && this->wY==16)
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(22, 14))
							{this->m_CurrentState=r5_stExitingCentralRoom; break;} //Door must be open before continuing.
						else
							return SetGoal(20, 17, strikeorb, pGoal);
					}
				else
					return SetGoal(20, 16, ::move, pGoal);
			
			case r5_stExitingCentralRoom:  //Exit central room and leave through the nw trapdoors.
				if (this->wX==33 && this->wY==5) {
					this->m_CurrentState=r5_stEscapingViaTrapDoor;
					break;
				}
				else
					return SetGoal(33, 5, ::move, pGoal);

			case r5_stEscapingViaTrapDoor: //Open the door.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(32, 15))
					{return SetGoal(34, 11, strikeorb, pGoal);}
				else if (!(this->wX==33 && this->wY==8))
					{return SetGoal(33, 8, ::move, pGoal);}
				else
					{this->m_CurrentState=r5_stFindingHideOut; break;}
				
			case r5_stFindingHideOut:  //Run to the closet.
				if (this->wX==32 && this->wY==18) {this->m_CurrentState=r5_stHidingInCloset; break;}
				else
					return SetGoal(32, 18, ::move, pGoal);
			
			case r5_stHidingInCloset: //In the closet.  Shut the door and wait.
				if (this->pCurrentGame->pRoom->IsDoorOpen(32, 17))
					{return SetGoal(32, 19, strikeorb, pGoal);}
				else if (swX==3 && swY==12) {this->m_CurrentState=r5_EscapingHideOut; break;}
				return true;
			
			case r5_EscapingHideOut: //Swordsman is far enough away to make a break from the closet.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(32, 17))
					{return SetGoal(32, 19, strikeorb, pGoal, true);}
				if (!(this->wX==31 && this->wY==16))
					{return SetGoal(31, 16, ::move, pGoal);}

				this->m_CurrentState=r5_stEscapingFinalDoor;
				break;
			
			case r5_stEscapingFinalDoor: //Open the final door to escape.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(17, 30))
					{return SetGoal(25, 26, strikeorb, pGoal);}
				else if (!(this->wX==21 && this->wY==23))
					{return SetGoal(21, 23, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stFleeing; break;}
			
			case rx_stFleeing: //Exit the room.
				if(this->wX==20&& this->wY==31)
					return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(20, 31, ::move, pGoal);

			default:
				ASSERTP(false, "Bad state in room 5."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 5.");
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom6Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx: //Get away from Swordsman at the entrance.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(28, 3)) {this->m_CurrentState=r6_stMovingControlArea; break;}
				else
					return SetGoal(30, 4, strikeorb, pGoal);
			
			case r6_stMovingControlArea: //Move down to control center.
				if (this->wX==36 && this->wY==23) {this->m_CurrentState=r6_stEnteringControlCenter; break;}
				else
					return SetGoal(36, 23, ::move, pGoal);
			
			case r6_stEnteringControlCenter: //Move to position inside control center.
				if (this->wX==35 && this->wY==27) {this->m_CurrentState=r6_GuardingControlCenter; break;}
				else 
					return SetGoal(35, 27, ::move, pGoal);
			
			case r6_GuardingControlCenter:  //Manning control center.
				//If the tar mother dead, take off.
				if (!this->pCurrentGame->pRoom->IsMonsterInRect(17, 13, 18, 13))
				{
					CueEvents.Add(CID_NeatherFrustrated, this);
					this->m_CurrentState=r6_PreparingEscapeRoute;
					break;
				}
										
				//Close any open doors that Swordsman is too close to.
				if (IsInRect(swX, swY, 16, 10, 26, 10) && this->pCurrentGame->pRoom->IsDoorOpen(21, 11))
					{return SetGoal(35, 28, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 27, 11, 27, 16) && this->pCurrentGame->pRoom->IsDoorOpen(26, 16))
					{return SetGoal(34, 28, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 12, 21, 17, 21) && this->pCurrentGame->pRoom->IsDoorOpen(18, 21))
					{return SetGoal(32, 28, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 6, 13, 12, 19) && this->pCurrentGame->pRoom->IsDoorOpen(9, 16))
					{return SetGoal(33, 28, strikeorb, pGoal);}
				if (IsInRect(swX, swY, 6, 25, 16, 25) && this->pCurrentGame->pRoom->IsDoorOpen(11, 26))
					{return SetGoal(31, 28, strikeorb, pGoal);}
				if ((IsInRect(swX, swY, 18, 25, 24, 25) || IsInRect(swX, swY, 24, 20, 25, 24))
				 && this->pCurrentGame->pRoom->IsDoorOpen(23, 26))
					{return SetGoal(30, 28, strikeorb, pGoal);}
				
				//Open whatever doors Swordsman isn't close to.
				if (!IsInRect(swX, swY, 16, 10, 26, 10) && !this->pCurrentGame->pRoom->IsDoorOpen(21, 11))
					{return SetGoal(35, 28, strikeorb, pGoal);}
				if (!IsInRect(swX, swY, 27, 11, 27, 16) && !this->pCurrentGame->pRoom->IsDoorOpen(26, 16))
					{return SetGoal(34, 28, strikeorb, pGoal);}
				if (!IsInRect(swX, swY, 12, 21, 17, 21) && !this->pCurrentGame->pRoom->IsDoorOpen(18, 21))
					{return SetGoal(32, 28, strikeorb, pGoal);}
				if (!IsInRect(swX, swY, 6, 13, 12, 19) && !this->pCurrentGame->pRoom->IsDoorOpen(9, 16))
					{return SetGoal(33, 28, strikeorb, pGoal);}
				if (!IsInRect(swX, swY, 6, 25, 16, 25) && !this->pCurrentGame->pRoom->IsDoorOpen(11, 26))
					{return SetGoal(31, 28, strikeorb, pGoal);}
				if (!(IsInRect(swX, swY, 18, 25, 24, 25) || IsInRect(swX, swY, 24, 20, 25, 24))
				 && !this->pCurrentGame->pRoom->IsDoorOpen(23, 26))
					{return SetGoal(30, 28, strikeorb, pGoal);}
				//Just wait.
				return true;
			
			case r6_PreparingEscapeRoute:  //Close south doors so it is safe to flee.
				if (this->pCurrentGame->pRoom->IsDoorOpen(23, 26))
					{return SetGoal(30, 28, strikeorb, pGoal);}
				if (this->pCurrentGame->pRoom->IsDoorOpen(11, 26))
					{return SetGoal(31, 28, strikeorb, pGoal);}

				this->m_CurrentState=r6_stExitingControlCenter;
				break;
			
			case r6_stExitingControlCenter: //Exit the control center through north passage.
				if (this->wX==30 && this->wY==24) {this->m_CurrentState=r6_stMovingNorthPassage; break;}
				else
					return SetGoal(30, 24, ::move, pGoal);
			
			case r6_stMovingNorthPassage: //Keep running.
				if (this->wX==27 && this->wY==22) {this->m_CurrentState=rx_stFleeing; break;}
				else
					return SetGoal(27, 22, ::move, pGoal);
			
			case rx_stFleeing: //Hit orb to open final door and leave the room.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(4, 28))
					{return SetGoal(7, 28, strikeorb, pGoal);}
				else if (this->wX==3 && this->wY==31)
					return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(3, 31, ::move, pGoal);

			default:
				ASSERTP(false, "Bad state in room 6."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 6.");
	return bRetVal;
}

//*****************************************************************************************
inline void CNeather::CanPlayerEscape(
	UINT const wPathCode,   //(in)
	CDbRoom* pRoom,         //(in)
	CCueEvents &CueEvents)  //(in/out)
{
	if (!wPathCode)   //no paths left
		if (pRoom->IsDoorOpen(3,29) &&
			 pRoom->IsDoorOpen(11,29) &&
			 pRoom->IsDoorOpen(19,29) &&
			 pRoom->IsDoorOpen(27,29) &&
			 pRoom->IsDoorOpen(35,29)) //player can exit
			CueEvents.Add(CID_NeatherFrustrated, this);
}

//*****************************************************************************************
bool CNeather::GetRoom7Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	enum DoorMasks {
		DoorMask1=1<<0,   // 1
		DoorMask2=1<<1,   // 2
		DoorMask3=1<<2,   // 4
		DoorMask4=1<<3,   // 8
		DoorMask5=1<<4,   // 16
		DoorMask6=1<<5,   // 32
		DoorMask7=1<<6,   // 64
		DoorMask8=1<<7 // 128
	};
	
	bool bRetVal=false;
	
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	UINT const wPathCode=
		this->pCurrentGame->pRoom->IsDoorOpen(14, 5)  << 0 |
		this->pCurrentGame->pRoom->IsDoorOpen(14, 7)  << 1 |
		this->pCurrentGame->pRoom->IsDoorOpen(14, 9)  << 2 |
		this->pCurrentGame->pRoom->IsDoorOpen(18, 10) << 3 |
		this->pCurrentGame->pRoom->IsDoorOpen(20, 10) << 4 |
		this->pCurrentGame->pRoom->IsDoorOpen(22, 9)  << 5 |
		this->pCurrentGame->pRoom->IsDoorOpen(22, 7)  << 6 |
		this->pCurrentGame->pRoom->IsDoorOpen(22, 5)  << 7 ;
	ASSERT(wPathCode<256);

	//Get Neather state.
	if (this->m_CurrentState==0)
	{
		this->m_CurrentState = rx_stx;
	}
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx:  //Swordsman has first entered room.
				this->m_CurrentState=r7_stGuardingLock3; break;
				return true;
			
			case r7_stGuardingLock3: //Neather is in third lock.  Lock doors 1, 3, and 4 are closed.
				if (this->wX==18 && this->wY==19) {this->m_CurrentState=r7_stGuardingLock4; break;}  //Neather is in fourth lock.
				if (this->pCurrentGame->pRoom->IsDoorOpen(18, 19))
					{return SetGoal(18, 19, ::move, pGoal);}  //Walk through lock door 3.
				return true;
			
			case r7_stGuardingLock4: //Neather is in fourth lock.  Hit the orb and split.
				if (!(this->wX==22 && this->wY==23))
				{  //Door's shut and still in lock 4.
					if (!this->pCurrentGame->pRoom->IsDoorOpen(18, 21) && IsInRect(this->wX, this->wY, 17, 19, 19, 20))
						{return SetGoal(18, 20, strikeorb, pGoal);}
					else
						{return SetGoal(22, 23, ::move, pGoal);}
				}
				else
					{this->m_CurrentState=r7_stMovingLock1; break;}

			case r7_stMovingLock1: //Walk around to the first lock.
				if (!(this->wX==36 && this->wY==8))
					{return SetGoal(36, 8, ::move, pGoal);}
				else
					{this->m_CurrentState=r7_stCheckingPathStatus; break;}
			
			case r7_stCheckingPathStatus: //Keep walking to first lock
				// or if there are no paths left exit the room.
				if (!wPathCode) {this->m_CurrentState=rx_stN; break;}
				else if (!(this->wX==21 && this->wY==1))
					{return SetGoal(20, 1, ::move, pGoal);}
				else
					{this->m_CurrentState=r7_stGuardingLock1; break;}
			
			case r7_stGuardingLock1: //Wait outside the first lock if the swordsman is not in lock 3.
				if (!wPathCode) //Make sure there are paths left.
					{this->m_CurrentState=rx_stN; break;}  //Exit the room.
				else if (IsInRect(swX, swY, 17, 18, 19, 18)) 
					{this->m_CurrentState=r7_stMovingLock2; break;}  //Okay to enter lock 1.
				else
					return true;  //Wait for S to move out of lock 1 and shut the door between lock 1 and 2.
			
			case r7_stMovingLock2: //Enter first lock and hit the orb to open lock door 3. Then wait for swordsman to open lock door 1.
				if (!(this->wX==19 && this->wY==4))
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(18, 19))  //Open lock door 3 so swordsman will enter and open lock door 1.
						{return SetGoal(18, 3, strikeorb, pGoal);}
					else
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(18, 4))  //It's open, so enter lock 2.
							{return SetGoal(19, 4, ::move, pGoal);}
						else
							return true;  //Nothing to do but wait for the swordsman to open lock door 1.
					}
				}
				else
					{this->m_CurrentState=r7_stGuardingLock2; break;}  //Neather is now in lock 2.
			
			case r7_stGuardingLock2: //Open lock 2 door after S exits lock 4 and enter the selection chamber.
				if (!(this->wX==19 && this->wY==9))
				{
					if (this->pCurrentGame->pRoom->IsDoorOpen(18, 8))  //Lock 2 door is open.
						{return SetGoal(19, 9, ::move, pGoal);}
					else
					{    //Lock 2 door is not open.
						if (IsInRect(swX, swY, 17, 16, 19, 21))  //Swordsman isn't ready, just move to lock door 2 orb.
						{
							if (this->wX==18 && this->wY==6) return true;
							else
								{return SetGoal(18, 6, ::move, pGoal);}
						}
						else          //Hit the lock door 2 orb.
							{return SetGoal(18, 7, strikeorb, pGoal);}  //Open lock 2 door.
					}
				}
				else
					{this->m_CurrentState=r7_stSearchingPaths; break;}
				
			case r7_stSearchingPaths: //Determine which of 9 paths to take.
					
				if (wPathCode & DoorMask6) {
					if (wPathCode & DoorMask8)
						this->m_CurrentState=rx_sth; //Door to path 8.
					else
						this->m_CurrentState=rx_stf; //Door to path 6.
				}  else if (wPathCode & DoorMask7)
					this->m_CurrentState=rx_stg; //Door to path 7.
				else if (wPathCode & DoorMask1)
					this->m_CurrentState=rx_sta; //Door to path 1.
				else if (wPathCode & DoorMask3)
					this->m_CurrentState=rx_stc; //Door to path 3.
				else if (wPathCode & DoorMask5)
					this->m_CurrentState=rx_ste; //Door to path 5.
				else if (wPathCode & DoorMask2)
					this->m_CurrentState=rx_stb; //Door to path 2.
				else if (wPathCode & DoorMask4)
					this->m_CurrentState=rx_std; //Door to path 4.
				else if (wPathCode & DoorMask8)
					this->m_CurrentState=rx_sth; //Door to path 8.
				else
					ASSERTP(false, "Neather should not be in selection chamber if there are no paths left.");
				return true;

			case r7_stMovingLock3:  //Move into lock 3.  Used by all of the paths.
				if (!(this->wX==18 && this->wY==18))
					{return SetGoal(18, 18, ::move, pGoal);}
				else
					{this->m_CurrentState=r7_stGuardingLock3; break;}  //Loop cycles.
			
			case r7_stMovingPath1Step1:  //Path 1.
				if (!(this->wX==16 && this->wY==9))
					{return SetGoal(16, 9, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stM; break;}
			case rx_stM:
				if (!(this->wX==15 && this->wY==6))
					{return SetGoal(15, 6, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stj; break;}
			case rx_stj:
				if (!(this->wX==6 && this->wY==5))
					{return SetGoal(6, 5, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stk; break;}
			case rx_stk:
				if (!(this->wX==5 && this->wY==6))
					{return SetGoal(5, 6, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stl; return SetGoal(5, 7, strikeorb, pGoal);}
			case rx_stl:               
				{this->m_CurrentState=rx_stm; return SetGoal(6, 7, ::move, pGoal);}
			case rx_stm:
				{this->m_CurrentState=r7_stMovingLock3; return SetGoal(5, 8, ::move, pGoal);}
			
			case rx_stb: //Path 2.
				if (!(this->wX==16 && this->wY==9))
					{return SetGoal(16, 9, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stn; break;}
			case rx_stn:
				if (!(this->wX==9 && this->wY==7))
					{return SetGoal(9, 7, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_sto; break;}
			case rx_sto:
				{this->m_CurrentState=rx_stp; return SetGoal(8, 8, ::move, pGoal);}
			case rx_stp:
				{this->m_CurrentState=rx_stq; return SetGoal(8, 9, strikeorb, pGoal);}
			case rx_stq:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(7, 9, ::move, pGoal);
			}
			
			case rx_stc: //Path 3.
				if (!(this->wX==14 && this->wY==9))
					{return SetGoal(14, 9, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_str; break;}
			case rx_str:
				{this->m_CurrentState=rx_sts; return SetGoal(13, 10, ::move, pGoal);}
			case rx_sts:
				{this->m_CurrentState=rx_stu; return SetGoal(13, 11, strikeorb, pGoal);}
			case rx_stu:
				if (!(this->wX==10 && this->wY==12))
					{return SetGoal(10, 12, ::move, pGoal);}
				else
				{
					CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
					this->m_CurrentState=rx_stAsterix; break;
				}
					
			case rx_std: //Path 4.
				if (!(this->wX==18 && this->wY==11))
					{return SetGoal(18, 11, ::move, pGoal);}
				else                             
					{this->m_CurrentState=rx_stv; break;}
			case rx_stv:
				{this->m_CurrentState=rx_stw; return SetGoal(18, 12, strikeorb, pGoal);}
			case rx_stw:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(17, 12, ::move, pGoal);
			}
			
			case rx_ste: //Path 5.
				if (!(this->wX==21 && this->wY==11))
					{return SetGoal(21, 11, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stX; break;}
			case rx_stX:
				{this->m_CurrentState=rx_sty; return SetGoal(21, 12, strikeorb, pGoal);}
			case rx_sty:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(22, 12, ::move, pGoal);
			}

			case rx_stf: //Path 6.
				if (!(this->wX==23 && this->wY==9))
					{return SetGoal(23, 9, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stD; break;}
			case rx_stD:
				{this->m_CurrentState=rx_stE; return SetGoal(24, 10, ::move, pGoal);}
			case rx_stE:
				{this->m_CurrentState=rx_stF; return SetGoal(24, 11, strikeorb, pGoal);}
			case rx_stF:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(25, 11, ::move, pGoal);
			}
			
			case rx_stg: //Path 7.
				{this->m_CurrentState=rx_stz; return SetGoal(20, 9, ::move, pGoal);}
			case rx_stz:
				if (!(this->wX==26 && this->wY==7))
					{return SetGoal(26, 7, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stA; break;}
			case rx_stA:
				if (!(this->wX==27 && this->wY==9))
					{return SetGoal(27, 9, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stB; break;}
			case rx_stB:
				{this->m_CurrentState=rx_stC; return SetGoal(27, 10, strikeorb, pGoal);}
			case rx_stC:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(28, 10, ::move, pGoal);
			}
								
			case rx_sth: //Path 8.
				{this->m_CurrentState=rx_stG; return SetGoal(20, 9, ::move, pGoal);}
			case rx_stG:
				if (!(this->wX==21 && this->wY==6))
					{return SetGoal(21, 6, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stH; break;}
			case rx_stH:
				if (!(this->wX==27 && this->wY==5))
					{return SetGoal(27, 5, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stI; break;}
			case rx_stI:
				if (!(this->wX==30 && this->wY==11))
					{return SetGoal(30, 11, ::move, pGoal);}
				else
					{this->m_CurrentState=rx_stJ; break;}
			case rx_stJ:
				{this->m_CurrentState=rx_stK; return SetGoal(30, 12, strikeorb, pGoal);}      
			case rx_stK:
				{this->m_CurrentState=rx_stL; return SetGoal(29, 12, ::move, pGoal);}  
			case rx_stL:
			{
				CanPlayerEscape(wPathCode,this->pCurrentGame->pRoom,CueEvents);
				this->m_CurrentState=rx_stAsterix; return SetGoal(30, 13, ::move, pGoal);
			}

			case rx_stN: //Exit the room.
				if (this->wX==33 && this->wY==2) {this->m_CurrentState=rx_stO; break;}
				else
					{return SetGoal(33, 2, ::move, pGoal);}
			case rx_stO:
				if (this->wX==35 && this->wY==2) {this->m_CurrentState=rx_stP; break;}
				else
					{this->m_CurrentState=rx_stP; return SetGoal(35, 2, ::move, pGoal);}
			case rx_stP:
				if (!this->pCurrentGame->pRoom->IsDoorOpen(37, 2))
					{return SetGoal(36, 5, strikeorb, pGoal);}
				else
				{
					if (this->pCurrentGame->pRoom->IsDoorOpen(34, 2))
						{return SetGoal(35, 5, strikeorb, pGoal);}
					else if (this->wX==37 && this->wY==2)
						{return SetGoal(-1, -1, exitroom, pGoal);}
					else
						{return SetGoal(37, 2, ::move, pGoal);} //Move off screen.
				}

			default:
				ASSERTP(false, "Bad state in room 7."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 7.");
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom8Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	bool bInTrap1, bInTrap2, bInTrap3, bInTrap4, bInTrap5;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx: //N is manning the controls to trap S.
				//If there are no more monsters then set stage to flee.
				if (!this->pCurrentGame->pRoom->IsMonsterInRect(1, 4, 33, 30))
				{
					CueEvents.Add(CID_NeatherFrustrated, this);
					this->m_CurrentState=rx_stf;
					break;
				}
				
				//Check for S in trigger areas where traps are sprung.  Don't close a door that will cause a
				//serpent to be dead-ended.
				bInTrap1=bInTrap2=bInTrap3=bInTrap4=bInTrap5=false;
				if (IsInRect(swX, swY, 7, 25, 16, 29))
				{
					bInTrap1=true;
					if (this->pCurrentGame->pRoom->IsDoorOpen(7, 28) && !this->pCurrentGame->pRoom->IsMonsterInRect(5, 28, 11, 28))
						{return SetGoal(15, 2, strikeorb, pGoal);}
				   if (this->pCurrentGame->pRoom->IsDoorOpen(16, 25) && !this->pCurrentGame->pRoom->IsMonsterInRect(16, 23, 16, 27)
				    && !this->pCurrentGame->pRoom->IsMonsterInRect(13, 28, 16, 28))  
				      {return SetGoal(17, 2, strikeorb, pGoal);}
				}
				if (IsInRect(swX, swY, 2, 7, 7, 11))
				{
					bInTrap2=true;
					if (this->pCurrentGame->pRoom->IsDoorOpen(3, 11) && !this->pCurrentGame->pRoom->IsMonsterInRect(3, 9, 3, 13)) 
						{return SetGoal(14, 2, strikeorb, pGoal);}
				}
				if (IsInRect(swX, swY, 16, 5, 23, 12))
				{
					bInTrap3=true;
					if (this->pCurrentGame->pRoom->IsDoorOpen(16, 8) && !this->pCurrentGame->pRoom->IsMonsterInRect(14, 8, 18, 8)) 
						{return SetGoal(16, 2, strikeorb, pGoal);}
					if (this->pCurrentGame->pRoom->IsDoorOpen(24, 8) && !this->pCurrentGame->pRoom->IsMonsterInRect(22, 8, 26, 8))
						{return SetGoal(19, 2, strikeorb, pGoal);}
				}
				if (IsInRect(swX, swY, 19, 25, 25, 28))
				{
					bInTrap4=true;
					if (this->pCurrentGame->pRoom->IsDoorOpen(22, 25) && !this->pCurrentGame->pRoom->IsMonsterInRect(22, 23, 22, 28)) 
						{return SetGoal(18, 2, strikeorb, pGoal);}
				}
				//This is an L-shaped trap where all exits are cut off.  N will close the area off entirely if a
				//serpent is inside, but leave one door open if a serpent is not yet inside.
				if (IsInRect(swX, swY, 25, 7, 32, 13) || IsInRect(swX, swY, 27, 14, 31, 16))
				{
					bInTrap5=true;
					//Serpents inside trap.
					if (this->pCurrentGame->pRoom->IsMonsterInRect(25, 7, 32, 13) || this->pCurrentGame->pRoom->IsMonsterInRect(27, 14, 31, 16))
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(24, 8) && !this->pCurrentGame->pRoom->IsMonsterInRect(22, 8, 26, 8))
							{return SetGoal(19, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(27, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(27, 13, 27, 18)) 
							{return SetGoal(20, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(31, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(31, 14, 31, 18)) 
							{return SetGoal(21, 2, strikeorb, pGoal);}
					}
					//Find nearby serpents to determine door to leave open.
					else if (this->pCurrentGame->pRoom->IsMonsterInRect(29, 17, 32, 24)) //Serpent(s) outside door 3.
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(24, 8) && !this->pCurrentGame->pRoom->IsMonsterInRect(22, 8, 26, 8))
							{return SetGoal(19, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(27, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(27, 13, 27, 18)) 
							{return SetGoal(20, 2, strikeorb, pGoal);}
						if (!this->pCurrentGame->pRoom->IsDoorOpen(31, 16)) {return SetGoal(21, 2, strikeorb, pGoal);}
					}
					else if (this->pCurrentGame->pRoom->IsMonsterInRect(13, 5, 23, 18)) //Serpent(s) outside door 1.
					{
						if (!this->pCurrentGame->pRoom->IsDoorOpen(24, 8)) {return SetGoal(19, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(27, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(27, 13, 27, 18))  
							{return SetGoal(20, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(31, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(31, 14, 31, 18)) 
							{return SetGoal(21, 2, strikeorb, pGoal);}
					}
					else //Just leave door 2 open.
					{
						if (this->pCurrentGame->pRoom->IsDoorOpen(24, 8) && !this->pCurrentGame->pRoom->IsMonsterInRect(22, 8, 26, 8))
							{return SetGoal(19, 2, strikeorb, pGoal);}
						if (!this->pCurrentGame->pRoom->IsDoorOpen(27, 16)) {return SetGoal(20, 2, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(31, 16) && !this->pCurrentGame->pRoom->IsMonsterInRect(31, 14, 31, 18))  
							{return SetGoal(21, 2, strikeorb, pGoal);}
					}
				} //...if (IsInRect(swX, swY, 24, 7, 32, 13) || IsInRect(swX, swY, 27, 14, 31, 16))
										
				//Execution will reach this point if all actions within trap trigger areas have been handled.  The lower
				//priority task left is to open as many doors as prudence allows so serpents can move freely.
				if (!bInTrap1)
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(7, 28)) {return SetGoal(15, 2, strikeorb, pGoal);}
					if (!this->pCurrentGame->pRoom->IsDoorOpen(16, 25))   {return SetGoal(17, 2, strikeorb, pGoal);}
				}
				if (!bInTrap2)
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(3, 11)) {return SetGoal(14, 2, strikeorb, pGoal);}
				}
				if (!bInTrap3)
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(16, 8)) {return SetGoal(16, 2, strikeorb, pGoal);}
				}
				if (!bInTrap4)
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(22, 25)) {return SetGoal(18, 2, strikeorb, pGoal);}
				}
				if (!bInTrap5)
				{
					if (!this->pCurrentGame->pRoom->IsDoorOpen(27, 16)) {return SetGoal(20, 2, strikeorb, pGoal);}
					if (!this->pCurrentGame->pRoom->IsDoorOpen(31, 16)) {return SetGoal(21, 2, strikeorb, pGoal);}
				}
				if (!(bInTrap3 || bInTrap5)) //One door is shared by both traps.
				{
			   if (!this->pCurrentGame->pRoom->IsDoorOpen(24, 8)) {return SetGoal(19, 2, strikeorb, pGoal);}
				}
				
				//Move to home position for quick access.
				if (!(this->wX==17 && this->wY==2)) {return SetGoal(17, 2, ::move, pGoal);}
			
				//Otherwise just wait.
				return true;
			
			case rx_stf:
				//N flees.
				if (!(this->wX==35 && this->wY==1)) {return SetGoal(35, 1, ::move, pGoal);}
				else
					this->m_CurrentState=rx_stOne; break;
			case rx_stOne:
				//Move around NW bend.
				if (!(this->wX==36 && this->wY==7)) {return SetGoal(36, 7, ::move, pGoal);}
				else
					this->m_CurrentState=rx_stTwo; break;
			case rx_stTwo:
				//Open the escape door.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(35, 11)) {return SetGoal(35, 9, strikeorb, pGoal);}
				else
					this->m_CurrentState=rx_stThree; break;
			case rx_stThree:
				//Leave the screen.
				if (this->wX==37 && this->wY==12)
					return SetGoal(-1, -1, exitroom, pGoal);
				else  
					return SetGoal(37, 12, ::move, pGoal);

			default:
				ASSERTP(false, "Bad state in room 8."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 8.");
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom9Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;

	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	bool bIsTrap1Loaded, bIsTrap2Loaded, bIsTrap3Loaded, bIsTrap4Loaded, bIsTrap5Loaded,
		 bIsTrap6Loaded, bIsTrap7Loaded, bIsTrap8Loaded, bIsTrap9Loaded, bAreAllTrapsLoaded,
		 bIsTrap1FrontOpen, bIsTrap1BackOpen, bIsTrap2FrontOpen, bIsTrap2BackOpen,
		 bIsTrap3FrontOpen, bIsTrap3BackOpen, bIsTrap4FrontOpen, bIsTrap4BackOpen,
		 bIsTrap5FrontOpen, bIsTrap5BackOpen, bIsTrap6FrontOpen, bIsTrap6BackOpen,
		 bIsTrap7FrontOpen, bIsTrap7BackOpen, bIsTrap8FrontOpen, bIsTrap8BackOpen,
		 bIsTrap9FrontOpen, bIsTrap9BackOpen,
		 bIsTrap1S, bIsTrap2S, bIsTrap3S, bIsTrap4S, bIsTrap5S, bIsTrap6S, bIsTrap7S,
		 bIsTrap8S, bIsTrap9S;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		//Determine what traps are loaded.
		bIsTrap1Loaded=(this->pCurrentGame->pRoom->IsMonsterInRect(1, 24, 1, 30) || this->pCurrentGame->pRoom->IsMonsterInRect(2, 30, 22, 30));
		bIsTrap1S=(IsInRect(swX, swY, 1, 24, 1, 30) || IsInRect(swX, swY, 2, 30, 22, 30));
		bIsTrap1FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(1, 23);
		bIsTrap1BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(23, 30);
		bIsTrap2Loaded=(this->pCurrentGame->pRoom->IsMonsterInRect(10, 1, 10, 5) || this->pCurrentGame->pRoom->IsMonsterInRect(11, 5, 19, 5));
		bIsTrap2S=(IsInRect(swX, swY, 10, 1, 10, 5) || IsInRect(swX, swY, 11, 5, 19, 5));
		bIsTrap2FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(9, 1);
		bIsTrap2BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(20, 5);
		bIsTrap3Loaded=(this->pCurrentGame->pRoom->IsMonsterInRect(13, 23, 21, 23) || this->pCurrentGame->pRoom->IsMonsterInRect(21, 21, 21, 22));
		bIsTrap3S=(IsInRect(swX, swY, 13, 23, 21, 23) || IsInRect(swX, swY, 21, 21, 21, 22));
		bIsTrap3FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(12, 23);
		bIsTrap3BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(21, 20);
		bIsTrap4Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(16, 11, 19, 17);
		bIsTrap4S=IsInRect(swX, swY, 16, 11, 19, 17);
		bIsTrap4FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(15, 11);
		bIsTrap4BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(20, 11);
		bIsTrap5Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(15, 28, 25, 28);
		bIsTrap5S=IsInRect(swX, swY, 15, 28, 25, 28);
		bIsTrap5FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(15, 27);
		bIsTrap5BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(27, 29);
		bIsTrap6Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(23, 11, 26, 16);
		bIsTrap6S=IsInRect(swX, swY, 23, 11, 26, 16);
		bIsTrap6FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(27, 11);
		bIsTrap6BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(22, 11);
		bIsTrap7Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(23, 18, 26, 22);
		bIsTrap7S=IsInRect(swX, swY, 23, 18, 26, 22);
		bIsTrap7FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(27, 18);
		bIsTrap7BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(22, 18);
		bIsTrap8Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(31, 8, 32, 15);
		bIsTrap8S=IsInRect(swX, swY, 31, 8, 32, 15);
		bIsTrap8FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(30, 15);
		bIsTrap8BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(32, 7);
		bIsTrap9Loaded=this->pCurrentGame->pRoom->IsMonsterInRect(31, 23, 33, 28);
		bIsTrap9S=IsInRect(swX, swY, 31, 23, 33, 28);
		bIsTrap9FrontOpen=this->pCurrentGame->pRoom->IsDoorOpen(30, 28);
		bIsTrap9BackOpen=this->pCurrentGame->pRoom->IsDoorOpen(34, 28);
		bAreAllTrapsLoaded=bIsTrap1Loaded && bIsTrap2Loaded && bIsTrap3Loaded &&
							bIsTrap4Loaded && bIsTrap5Loaded && bIsTrap6Loaded &&
							bIsTrap7Loaded && bIsTrap8Loaded && bIsTrap9Loaded;
		switch (this->m_CurrentState)
		{
			case rx_stx:
				//See if S made it inside.
				if (IsInRect(swX, swY, 23, 29, 36, 30) || IsInRect(swX, swY, 35, 5, 36, 28)
				 || IsInRect(swX, swY, 20, 5, 34, 6) ||   IsInRect(swX, swY, 20, 7, 22, 20))
					{this->m_CurrentState=rx_stp; break;}                 
				
				//First priority--if S is in initial chamber, keep him there while traps are loaded.
				if (!bAreAllTrapsLoaded)
				{
					if (IsInRect(swX, swY, 1, 1, 6, 11) || IsInRect(swX, swY, 7, 10, 9, 12))
					{//S is on north side of initial chamber, so keep north exit closed and south open.
						if (!this->pCurrentGame->pRoom->IsDoorOpen(10, 14)) {return SetGoal(19, 3, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(10, 12)) {return SetGoal(18, 3, strikeorb, pGoal);}
					}
					else if (IsInRect(swX, swY, 1, 17, 4, 20) || IsInRect(swX, swY, 5, 17, 7, 26) || IsInRect(swX, swY, 8, 14, 9, 18))
					{//S is on south side of initial chamber, so keep south exit closed and north open.
						if (!this->pCurrentGame->pRoom->IsDoorOpen(10, 12)) {return SetGoal(18, 3, strikeorb, pGoal);}
						if (this->pCurrentGame->pRoom->IsDoorOpen(10, 14)) {return SetGoal(19, 3, strikeorb, pGoal);}                              
					}
				}
				else //All traps are loaded--make sure both initial chamber doors are open.  It is signal to S that N
				{     //will allow him to exit initial chamber.
					if (!this->pCurrentGame->pRoom->IsDoorOpen(10, 12)) {return SetGoal(18, 3, strikeorb, pGoal);}
					if (!this->pCurrentGame->pRoom->IsDoorOpen(10, 14)) {return SetGoal(19, 3, strikeorb, pGoal);}
				}
				
				//2nd priority--don't let S into the nesting area.
				if (bIsTrap1BackOpen && bIsTrap1S) {return SetGoal(29, 3, strikeorb, pGoal);}
				if (bIsTrap2BackOpen && bIsTrap2S) {return SetGoal(23, 3, strikeorb, pGoal);}
				if (bIsTrap3BackOpen && bIsTrap3S) {return SetGoal(25, 3, strikeorb, pGoal);}
				if (bIsTrap4BackOpen && bIsTrap4S) {return SetGoal(24, 3, strikeorb, pGoal);}
				if (bIsTrap5BackOpen && bIsTrap5S) {return SetGoal(30, 3, strikeorb, pGoal);}
				if (bIsTrap6BackOpen && bIsTrap6S) {return SetGoal(27, 3, strikeorb, pGoal);}
				if (bIsTrap7BackOpen && bIsTrap7S) {return SetGoal(28, 3, strikeorb, pGoal);}
				if (bIsTrap8BackOpen && bIsTrap8S) {return SetGoal(35, 3, strikeorb, pGoal);}
				if (bIsTrap9BackOpen && bIsTrap9S) {return SetGoal(36, 3, strikeorb, pGoal);}
				
				//3rd priority--spring traps if S is in trigger areas.
				if (IsInRect(swX, swY, 10, 13, 16, 19) && bIsTrap2Loaded && bIsTrap5Loaded)
				{
					if (!bIsTrap2FrontOpen) {return SetGoal(17, 3, strikeorb, pGoal);}
					if (!bIsTrap5FrontOpen) {return SetGoal(22, 3, strikeorb, pGoal);}
					return true;  //Wait until S leaves rect or one of the traps is unloaded.
				}                 
				if (IsInRect(swX, swY, 9, 18, 18, 26) && bIsTrap1Loaded && bIsTrap4Loaded && bIsTrap9Loaded)
				{
					if (!bIsTrap1FrontOpen) {return SetGoal(16, 3, strikeorb, pGoal);}
					if (!bIsTrap4FrontOpen) {return SetGoal(21, 3, strikeorb, pGoal);}
					if (!bIsTrap9FrontOpen) {return SetGoal(34, 3, strikeorb, pGoal);}
					return true;  //Wait until S leaves rect or one of the traps is unloaded.
				}
				if (IsInRect(swX, swY, 23, 17, 34, 28) && bIsTrap6Loaded && bIsTrap3Loaded)
				{
					if (!bIsTrap3FrontOpen) {return SetGoal(20, 3, strikeorb, pGoal);}
					if (!bIsTrap6FrontOpen) {return SetGoal(31, 3, strikeorb, pGoal);}
					return true;  //Wait until S leaves rect or one of the traps is unloaded.
				}                                                     
				if (IsInRect(swX, swY, 23, 7, 30, 10) && bIsTrap7Loaded && bIsTrap8Loaded)
				{
					if (!bIsTrap7FrontOpen) {return SetGoal(32, 3, strikeorb, pGoal);}
					if (!bIsTrap8FrontOpen) {return SetGoal(33, 3, strikeorb, pGoal);}
					return true;  //Wait until S leaves rect or one of the traps is unloaded.
				}                    
				
				//4th priority--load traps.
				if (!bIsTrap1Loaded || bIsTrap1FrontOpen || bIsTrap1BackOpen)
				{
					if (bIsTrap1Loaded)
					{
						if (bIsTrap1BackOpen) {return SetGoal(29, 3, strikeorb, pGoal);}
						if (bIsTrap1FrontOpen) {return SetGoal(16, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap1FrontOpen) {return SetGoal(16, 3, strikeorb, pGoal);}
						if (!(bIsTrap1BackOpen || bIsTrap1S)) {return SetGoal(29, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap2Loaded || bIsTrap2FrontOpen || bIsTrap2BackOpen)
				{
					if (bIsTrap2Loaded)
					{
						if (bIsTrap2BackOpen) {return SetGoal(23, 3, strikeorb, pGoal);}
						if (bIsTrap2FrontOpen) {return SetGoal(17, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap2FrontOpen) {return SetGoal(17, 3, strikeorb, pGoal);}
						if (!(bIsTrap2BackOpen || bIsTrap2S)) {return SetGoal(23, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap3Loaded || bIsTrap3FrontOpen || bIsTrap3BackOpen)
				{
					if (bIsTrap3Loaded)
					{
						if (bIsTrap3BackOpen) {return SetGoal(25, 3, strikeorb, pGoal);}
						if (bIsTrap3FrontOpen) {return SetGoal(20, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap3FrontOpen) {return SetGoal(20, 3, strikeorb, pGoal);}
						if (!(bIsTrap3BackOpen || bIsTrap3S)) {return SetGoal(25, 3, strikeorb, pGoal);}
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap4Loaded || bIsTrap4FrontOpen || bIsTrap4BackOpen)
				{
					if (bIsTrap4Loaded)
					{
						if (bIsTrap4BackOpen) {return SetGoal(24, 3, strikeorb, pGoal);}
						if (bIsTrap4FrontOpen) {return SetGoal(21, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap4FrontOpen) {return SetGoal(21, 3, strikeorb, pGoal);}
						if (!(bIsTrap4BackOpen || bIsTrap4S)) {return SetGoal(24, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap5Loaded || bIsTrap5FrontOpen || bIsTrap5BackOpen)
				{
					if (bIsTrap5Loaded)
					{
						if (bIsTrap5BackOpen) {return SetGoal(30, 3, strikeorb, pGoal);}
						if (bIsTrap5FrontOpen) {return SetGoal(22, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap5FrontOpen) {return SetGoal(22, 3, strikeorb, pGoal);}
						if (!(bIsTrap5BackOpen || bIsTrap5S)) {return SetGoal(30, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap6Loaded || bIsTrap6FrontOpen || bIsTrap6BackOpen)
				{
					if (bIsTrap6Loaded)
					{
						if (bIsTrap6BackOpen) {return SetGoal(27, 3, strikeorb, pGoal);}
						if (bIsTrap6FrontOpen) {return SetGoal(31, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap6FrontOpen) {return SetGoal(31, 3, strikeorb, pGoal);}
						if (!(bIsTrap6BackOpen || bIsTrap6S)) {return SetGoal(27, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap7Loaded || bIsTrap7FrontOpen || bIsTrap7BackOpen)
				{
					if (bIsTrap7Loaded)
					{
						if (bIsTrap7BackOpen) {return SetGoal(28, 3, strikeorb, pGoal);}
						if (bIsTrap7FrontOpen) {return SetGoal(32, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap7FrontOpen) {return SetGoal(32, 3, strikeorb, pGoal);}
						if (!(bIsTrap7BackOpen || bIsTrap7S)) {return SetGoal(28, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap8Loaded || bIsTrap8FrontOpen || bIsTrap8BackOpen)
				{
					if (bIsTrap8Loaded)
					{
						if (bIsTrap8BackOpen) {return SetGoal(35, 3, strikeorb, pGoal);}
						if (bIsTrap8FrontOpen) {return SetGoal(33, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap8FrontOpen) {return SetGoal(33, 3, strikeorb, pGoal);}
						if (!(bIsTrap8BackOpen || bIsTrap8S)) {return SetGoal(35, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				if (!bIsTrap9Loaded || bIsTrap9FrontOpen || bIsTrap9BackOpen)
				{
					if (bIsTrap9Loaded)
					{
						if (bIsTrap9BackOpen) {return SetGoal(36, 3, strikeorb, pGoal);}
						if (bIsTrap9FrontOpen) {return SetGoal(34, 3, strikeorb, pGoal);}                            
					}
					else
					{
						if (!bIsTrap9FrontOpen) {return SetGoal(34, 3, strikeorb, pGoal);}
						if (!(bIsTrap9BackOpen || bIsTrap9S)) {return SetGoal(36, 3, strikeorb, pGoal);}                            
					}
					return true;  //Wait for trap to load.
				}
				
				//Otherwise do nothing.
				return true;
			
			case rx_stp: //N gets paranoid because S is inside.   
				//N flees when S gets too close.
				if (IsInRect(swX, swY, 20, 4, 36, 6)) {
					CueEvents.Add(CID_NeatherScared, this);
					this->m_CurrentState=rx_stf;
					break;
				}
				
				//Open all the doors so roaches can get to S.
				if (!bIsTrap9BackOpen) {return SetGoal(36, 3, strikeorb, pGoal);}
				if (!bIsTrap8BackOpen) {return SetGoal(35, 3, strikeorb, pGoal);}
				if (!bIsTrap9FrontOpen) {return SetGoal(34, 3, strikeorb, pGoal);}                  
				if (!bIsTrap8FrontOpen) {return SetGoal(33, 3, strikeorb, pGoal);}                  
				if (!bIsTrap7FrontOpen) {return SetGoal(32, 3, strikeorb, pGoal);}
				if (!bIsTrap6FrontOpen) {return SetGoal(31, 3, strikeorb, pGoal);}
				if (!bIsTrap5BackOpen) {return SetGoal(30, 3, strikeorb, pGoal);}
				if (!bIsTrap1BackOpen) {return SetGoal(29, 3, strikeorb, pGoal);}
				if (!bIsTrap7BackOpen) {return SetGoal(28, 3, strikeorb, pGoal);}
				if (!bIsTrap6BackOpen) {return SetGoal(27, 3, strikeorb, pGoal);}
				if (!bIsTrap3BackOpen) {return SetGoal(25, 3, strikeorb, pGoal);}
				if (!bIsTrap4BackOpen) {return SetGoal(24, 3, strikeorb, pGoal);}
				if (!bIsTrap5FrontOpen) {return SetGoal(22, 3, strikeorb, pGoal);}                  
				if (!bIsTrap2BackOpen) {return SetGoal(23, 3, strikeorb, pGoal);}
				if (!bIsTrap4FrontOpen) {return SetGoal(21, 3, strikeorb, pGoal);}                  
				if (!bIsTrap3FrontOpen) {return SetGoal(20, 3, strikeorb, pGoal);}                  
				if (!bIsTrap2FrontOpen) {return SetGoal(17, 3, strikeorb, pGoal);}                  
				if (!bIsTrap1FrontOpen) {return SetGoal(16, 3, strikeorb, pGoal);}

				//All doors are open so flee.
				CueEvents.Add(CID_NeatherLaughing, this);
				this->m_CurrentState=rx_stf; break;

			case rx_stf: //Flee from room.
				if (this->wX==15 && this->wY==2) {this->m_CurrentState=rx_stOne; break;}
				else 
					return SetGoal(15, 2, ::move, pGoal);
			case rx_stOne: //Move off the screen.
				if (this->wX==14 && this->wY==0)
					return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(14, 0, ::move, pGoal);

			default:
				ASSERTP(false, "Bad state in room 9."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 9.");
	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom10Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	
	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;
	int x=0, y=0;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;
			
	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx: //Neather is at entrance of the loop-d-loop trap. Swordsman hasn't entered.
				//Swordsman walked into loop-d-loop corridor.
				if (IsInRect(swX, swY, 12, 27, 14, 27)) {
					CueEvents.Add(CID_NeatherScared, this);
					this->m_CurrentState=rx_stOne;
					break;
				}
				
				//Get N in decision position.
				if (!(this->wX==13 && this->wY==21))
					{return SetGoal(13, 21, ::move, pGoal);}
				return true; //Otherwise wait.
													  
			//Stages for the loop-d-loop trap.
			case rx_stOne:
			//Swordsman inside the loop-d-loop trap heading towards the loop.
			//Maintain a steady distance from the S.
				  //Swordsman got cold feet and went back to the entrance.
				if (IsInRect(swX, swY, 11, 28, 15, 28)) {this->m_CurrentState=rx_stx; break;}
				
				//If Swordsman is north enough, Neather heads into the west entrance to the loop.
				if (swY==17) {this->m_CurrentState=rx_stFour; break;} //Go west.
																
				//The position N should be at is dependant on S's Y coord.
				switch (swY)
				{
					case 27: x=14; y=20; break;
					case 26: x=15; y=19; break;
					case 25: x=16; y=19; break;
					case 24: x=17; y=19; break;
					case 23: x=18; y=18; break;
					case 22: x=19; y=17; break;
					case 21: x=19; y=16; break;
					case 20: x=19; y=15; break;
					case 19: x=19; y=14; break;
					case 18: x=19; y=13; break;                        
					default: ASSERTP(false, "Bad swY"); //Huh?
				}
				return SetGoal(x, y, ::move, pGoal);
				
			//Swordsman is following N into loop. Neather has entered to west entrance.
			//Neather wants to get S to follow enough that he can break south
			//and lock Swordsman into the trap.
			case rx_stFour:  
										
				if (swY==18) {this->m_CurrentState=rx_stOne; break;} //Swordsman backed out of the loop.
				if (swX==19 && swY==7) { //Break south.
					CueEvents.Add(CID_NeatherLaughing, this);
					this->m_CurrentState=rx_stFive;
					break;
				}

				//Keep distance from Swordsman while leading him to the break south point.
				//Swordsman may move into either passage.
				if (swX<19) //Swordsman is on west side.
				{
					switch (swY)
					{
						case 17: x=18; y=12; break;
						case 16: x=17; y=11; break;
						case 15: x=16; y=10; break;
						case 14: x=16; y=9; break;
						case 13: x=16; y=8; break;
						case 12: x=17; y=7; break;
						case 11: x=18; y=7; break;
						case 10: x=19; y=7; break;
						case 9: x=20; y=7; break;
						case 8: x=21; y=7; break;
						case 7:
							if (swX==16 || swX==17) {x=22; y=8; break;}
							else if (swX==18) {x=22; y=9; break;}
							else {ASSERTP(false, "Bad swX."); break;}
						default: ASSERTP(false, "Bad swY(2).");   break;
					}
				}
				else //Swordsman is on east side. Maybe he thinks he'll head off Neather, but he won't.
				{
					switch (swY)
					{
						case 17: x=18; y=12; break;
						case 16: x=17; y=11; break;
						case 15: x=16; y=10; break;
						case 14: x=16; y=9; break;
						case 13: x=16; y=8; break;
						case 12: x=17; y=7; break;
						case 11: x=18; y=7; break;
						case 10: x=17; y=7; break;
						case 9: x=16; y=7; break;
						case 8: x=16; y=8; break;
						case 7:
							if (swX==22 || swX==21) {x=16; y=9; break;}
							else if (swX==20) {x=16; y=10; break;}
							else {ASSERTP(false,"Bad swX(3)"); break;}
						default: ASSERTP(false,"Bad swY(3)"); break;
					}
				}
				return SetGoal(x, y, ::move, pGoal);
			
			//The chump fell for it! Neather breaks south to lock
			//Swordsman into the loop-d-loop chamber.
			case rx_stFive: 
				if (this->wX==18 && this->wY==18) {this->m_CurrentState=rx_stSix; break;}
				else
					return SetGoal(18, 18, ::move, pGoal);
			case rx_stSix: //Around the first bend.
				if (this->wX==13 && this->wY==27) {this->m_CurrentState=rx_stSeven; break;}
				else
					return SetGoal(13, 27, ::move, pGoal);
			case rx_stSeven: //Near the loop-d-loop door orb.
				if (!this->pCurrentGame->pRoom->IsDoorOpen(13, 27)) {this->m_CurrentState=rx_stEight; break;}
				else
					return SetGoal(15, 29, strikeorb, pGoal);

			case rx_stEight: //Determine what to do now that Swordsman is trapped.
				if (!(bIsPit(this->pCurrentGame->pRoom->GetOSquare(26, 2)))) {this->m_CurrentState=rx_sta; break;} //Leave room through north exit.
				if (!(bIsPit(this->pCurrentGame->pRoom->GetOSquare(3, 6)))) {this->m_CurrentState=rx_stA; break;} //Leave room through west exit.
				
				//Neather and Swordsman are both trapped.  Neather goes to chamber of despair.
				CueEvents.Add(CID_NeatherFrustrated, this);
				this->m_CurrentState=rx_stm;
				break;
				
			//North exit. Neather starts at position above orb.               
			case rx_sta:
				if (this->wX==25 && this->wY==3) {this->m_CurrentState=rx_ste; break;}
				else
					return SetGoal(25, 3, ::move, pGoal);
			case rx_ste:
				if (this->wX==27 && this->wY==2) {this->m_CurrentState=rx_stf; break;}
				else
					return SetGoal(27, 2, ::move, pGoal);
			case rx_stf:
				if (this->pCurrentGame->pRoom->IsDoorOpen(24, 1)) {this->m_CurrentState=rx_stNine; break;}
				else
					return SetGoal(28, 4, strikeorb, pGoal);
			case rx_stNine:
				//Neather wins the room!
				if (this->wX==25 && this->wY==0)
					return SetGoal(-1, -1, exitroom, pGoal);
				else
					return SetGoal(25, 0, ::move, pGoal);
			
			//West exit. Neather starts at position above orb.
			case rx_stA:
				if (this->wX==11 && this->wY==28) {this->m_CurrentState=rx_stF; break;}
				else
					return SetGoal(11, 28, ::move, pGoal);
			case rx_stF:
				if (this->wX==5 && this->wY==7) {this->m_CurrentState=rx_stB; break;}
				else
					return SetGoal(5, 7, ::move, pGoal);
			case rx_stB:
				if (this->wX==3 && this->wY==6) {this->m_CurrentState=rx_stC; break;}
				else
					return SetGoal(3, 6, ::move, pGoal);
			case rx_stC:
				if (this->pCurrentGame->pRoom->IsDoorOpen(1, 3)) {this->m_CurrentState=rx_stD; break;}
				else
					return SetGoal(2, 9, strikeorb, pGoal);
			case rx_stD:
				if (this->wX==5 && this->wY==4) {this->m_CurrentState=rx_stE; break;}
				else
					return SetGoal(5, 4, ::move, pGoal);
			case rx_stE:
				//Swordsman wins the room!
				if (this->wX==0 && this->wY==3)
					return SetGoal(-1, -1, exitroom, pGoal);
				else  
					return SetGoal(0, 3, ::move, pGoal); 
			
			//Neather goes to chamber of despair to wait for eternity.
			case rx_stm:
				if (!this->pCurrentGame->pRoom->IsDoorOpen(30, 17)) {this->m_CurrentState=rx_stn; break;}
				else
					return SetGoal(30, 19, strikeorb, pGoal);
			//March around the chamber of despair.
			case rx_stn: this->m_CurrentState=rx_sto; return SetGoal(31, 18, ::move, pGoal);
			case rx_sto: this->m_CurrentState=rx_stp; return SetGoal(31, 19, ::move, pGoal);
			case rx_stp: this->m_CurrentState=rx_stq; return SetGoal(31, 20, ::move, pGoal);
			case rx_stq: this->m_CurrentState=rx_str; return SetGoal(30, 20, ::move, pGoal);
			case rx_str: this->m_CurrentState=rx_sts; return SetGoal(29, 20, ::move, pGoal);
			case rx_sts: this->m_CurrentState=rx_stt; return SetGoal(29, 19, ::move, pGoal);
			case rx_stt: this->m_CurrentState=rx_stu; return SetGoal(29, 18, ::move, pGoal);
			case rx_stu: this->m_CurrentState=rx_stn; return SetGoal(30, 18, ::move, pGoal);

			default:
				ASSERTP(false, "Bad room 10 state."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 10.");

	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom11Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &CueEvents)  //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;
	
	//Get Neather state.
	if (this->m_CurrentState==0)
	{
		CueEvents.Add(CID_NeatherScared, this);
		this->m_CurrentState = rx_stx;
	}
	
	if (this->wX==17 && this->wY==0)
		bRetVal = SetGoal(-1, -1, exitroom, pGoal);     
	else
		bRetVal = SetGoal(17, 0, ::move, pGoal);

	return bRetVal;
}

//*****************************************************************************************
bool CNeather::GetRoom12Goal(
//Gets a goal for the Neather to achieve based on the current room
//and the state of things inside it.
//
//Params:
	GOAL &pGoal,      //(in) Accepts pointer to an GOAL structure
							//that will be populated by this method
	CCueEvents &/*CueEvents*/) //(in/out)
//
//Returns:
//true if successful, false otherwise.
{
	bool bRetVal=false;

	const UINT swX=this->pCurrentGame->swordsman.wX, swY=this->pCurrentGame->swordsman.wY;

	//Get Neather state.
	if (this->m_CurrentState==0) this->m_CurrentState = rx_stx;

	SetGoal(this->wX, this->wY, waitTurn, pGoal);
	while (true)  //Function exits with return.
	{
		switch (this->m_CurrentState)
		{
			case rx_stx:  //Swordsman just entered room.
				if (this->pCurrentGame->pRoom->IsDoorOpen(18, 7)) {this->m_CurrentState=rx_stZero; break;}
				else //Open door to central area and enter it.
					return SetGoal(6, 24, strikeorb, pGoal);
			case rx_stZero:
				if (this->wX==6 && this->wY==21) {this->m_CurrentState=rx_stOne; break;}
			   else
			      return SetGoal(6, 21, ::move, pGoal);
			case rx_stOne:
				if (this->wX==1 && this->wY==2) {this->m_CurrentState=rx_stTwo; break;}
			   else
			      return SetGoal(1, 2, ::move, pGoal);
			case rx_stTwo:
				if (this->wX==16 && this->wY==1) {this->m_CurrentState=rx_stThree; break;}
			   else
			      return SetGoal(16, 1, ::move, pGoal);
			case rx_stThree:
				if (this->wX==17 && this->wY==16) {this->m_CurrentState=rx_stFour; break;}
				else
					return SetGoal(17, 16, ::move, pGoal);

			//Neather is in central area.  Close door to central area whether Swordsman is inside or not.
			case rx_stFour:
				if (!this->pCurrentGame->pRoom->IsDoorOpen(18, 7)) {this->m_CurrentState=rx_stFive; break;}
				else
					return SetGoal(15, 18, strikeorb, pGoal);

			case rx_stFive: //Get into ready position.
				if (this->wX==18 && this->wY==16) {this->m_CurrentState=rx_stSix; break;}
				//Swordsman is approaching Neather.
				if (IsInRect(swX, swY, 17, 14, 20, 14)) {this->m_CurrentState=rx_stSeven; break;}
				return SetGoal(18, 16, ::move, pGoal);

			case rx_stSix: //In ready position.
				//Swordsman is approaching Neather.
				if (IsInRect(swX, swY, 17, 14, 20, 14)) {this->m_CurrentState=rx_stSeven; break;}
				//Swordsman is heading toward southeast orb. 
				if (IsInRect(swX, swY, 22, 11, 26, 16)) {this->m_CurrentState=rx_stEight; break;} 
			   return true;

			case rx_stSeven: //Move away from Swordsman to corner.
				if (swY<14) {this->m_CurrentState=rx_stFive; break;}
				if (swY<17) return SetGoal(18, 18, ::move, pGoal);   //Move into corner.
				return true;   //Wait wherever it is.

			case rx_stEight: //Get ready to kill Swordsman when he strikes the southeast orb.
				if (this->wX==19 && this->wY==15) {this->m_CurrentState=rx_stNine; break;}
				else
					return SetGoal(19, 15, ::move, pGoal);
			case rx_stNine:
				if (this->wX==23 && this->wY==17) {this->m_CurrentState=rx_sta; break;}
				else
					return SetGoal(23, 17, ::move, pGoal);
			case rx_sta: //Wait for Swordsman to strike southeast orb.
			  //Swordsman followed Neather into orb passage.
			  //Swordsman cannot win room from this point on.
			  //When west door is open, it means Swordsman hit the orb.
			  if (this->pCurrentGame->pRoom->IsDoorOpen(12, 11) && !this->pCurrentGame->pRoom->IsDoorOpen(26, 14))  
			   {return SetGoal(23, 18, strikeorb, pGoal,true);}
			  return true;  //Otherwise wait.

			default:
				ASSERTP(false, "Bad room 12 state."); return false;
		}
	}
	
	ASSERTP(false, "Bad flow in room 12.");
	return bRetVal;
}
