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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Zombie.cpp
//Implementation of CZombie.

#include "Aumtlich.h"
#include "Stalwart.h"
#include "CueEvents.h"
#include "CurrentGame.h"
#include "DbRooms.h"

//
//Public methods.
//

//*****************************************************************************************
void CZombie::Process(
//Process a Zombie for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
	this->lastGazedTile.wX = (UINT)-1;
	this->lastGazedTile.wY = (UINT)-1;

	//If stunned, skip turn
	if (IsStunned())
		return;

	this->wPrevX = this->wX;
	this->wPrevY = this->wY;

	//If no target, then stop here.
	const bool bTarget = GetTarget(this->wTX, this->wTY);
	if (!bTarget)
	{
		this->wTX = -1000;
		this->wTY = -1000;
		return;
	}

	if (this->bFrozen) return; //zombie is frozen -- can't move

	//Get movement offsets.
	int dxFirst, dyFirst, dx, dy;
	if (!GetDirectMovement(this->wTX, this->wTY, dxFirst, dyFirst, dx, dy, SmartOmniFlanking))
		return;

	if (dx || dy)
	{
		//If not facing the direction it will move, turn zombie that way.
		//Else move zombie (in direction it's facing) forward to new destination square.
		const UINT desiredOrientation = GetOrientationFacingTarget(this->wTX, this->wTY);
		if (desiredOrientation != this->wO)
			MakeSlowTurn(desiredOrientation);
		else
			MakeStandardMove(CueEvents, dx, dy);
	} else {
		//Can't move -- just turn toward swordsman.
		const UINT desiredOrientation = GetOrientationFacingTarget(this->wTX,this->wTY);
		MakeSlowTurn(desiredOrientation);
	}
}

//*****************************************************************************************
void CZombie::UpdateGaze(
//Each turn, after all monsters have moved:
//Check tiles along zombie's line of sight.  If player is along this line
//he is frozen for this turn.  Swords can reflect the zombie's gaze back at itself.
//
//Params:
	CCueEvents &CueEvents, CCoordIndex &SwordCoords,
	const bool bFullTurn) //whether this is occurring on a full or half-turn (i.e. when player is hasted)
{
	if (this->pCurrentGame->IsPlayerAt(this->wX, this->wY))
		return; //when killing player, don't send out a gaze

	CueEvents.Add(CID_ZombieGaze, this);

	//Follow direction of zombie's gaze.
	int oX = nGetOX(this->wO);
	int oY = nGetOY(this->wO);
	UINT wX = this->wX + oX;   //start checking in front of zombie
	UINT wY = this->wY + oY;
	CDbRoom *pRoom = this->pCurrentGame->pRoom;

	if (pRoom->DoesGentryiiPreventDiagonal(this->wX, this->wY, wX, wY))
		return;
	while (GetNextGaze(CueEvents, this, pRoom, SwordCoords,
		wX, wY, oX, oY, this->wTX, this->wTY, bFullTurn))
	{
		if (pRoom->DoesGentryiiPreventDiagonal(wX - oX, wY - oY, wX, wY))
			break;
	}
	
	this->lastGazedTile.wX = wX;
	this->lastGazedTile.wY = wY;
}

//*****************************************************************************************
bool CZombie::GetNextGaze(
//Updates (cx,cy) based on how aumtlich gaze travels when facing (dx,dy).
//
//Returns: true if gaze continues, false if blocked
//
//Params:
	CCueEvents &CueEvents,	//(in/out)
	CZombie *pAumtlich,		//non-NULL if aumtlich and game state is affected
	CDbRoom *pRoom,			//active room
	CCoordIndex &SwordCoords,	//where swords are located
	UINT& cx, UINT& cy, int& dx, int& dy,	//(cx,cy) + (dx,dy)
	UINT wTX, UINT wTY,     //Aumtlich Target
	const bool bFullTurn)   //[default=true] whether this is occurring on a full or half-turn (i.e. when player is hasted)
{
	ASSERT(pRoom);
	const CCurrentGame *pCurrentGame = pRoom->GetCurrentGame();
	ASSERT(pCurrentGame);
	if (!pRoom->IsValidColRow(cx, cy))
		return false;

	const UINT wOSquare = pRoom->GetOSquare(cx,cy);
	const UINT wTSquare = pRoom->GetTSquare(cx,cy);
	switch (wOSquare)
	{
		//Gaze can go over these objects.
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_PIT: case T_PIT_IMAGE: case T_WATER: case T_SHALLOW_WATER:
		case T_PLATFORM_P: case T_PLATFORM_W: case T_STEP_STONE:
		case T_HOT: case T_GOO:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
		case T_TUNNEL_E: case T_TUNNEL_W: case T_TUNNEL_N: case T_TUNNEL_S:
		case T_TRAPDOOR: case T_TRAPDOOR2: case T_THINICE: case T_THINICE_SH: case T_PRESSPLATE:
		case T_FLOOR_SPIKES: case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
		break;
		default:
			//All other objects stop gaze.
		return false;
	}

	//Gaze affects certain room objects.
	switch (wTSquare)
	{
		case T_TAR: case T_MUD: case T_GEL:
		case T_BOMB:
		case T_OBSTACLE:
		case T_POWDER_KEG:
			//These objects stop gaze.
			return false;
		case T_MIRROR:
			//Aumtlich looks into mirror and freezes itself.
			if (pAumtlich)
				pAumtlich->bFrozen = true;
			return false;
		case T_ORB:
			//Gaze damages cracked orbs.
			if (pAumtlich && bFullTurn && (pAumtlich->lastGazedTile.wX != cx || pAumtlich->lastGazedTile.wY != cy)) //on full turns only
			{
				COrbData *pOrb = pRoom->GetOrbAtCoords(cx, cy);
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

	//Certain monster types may be frozen.
	CSwordsman& player = ((CCurrentGame*)pCurrentGame)->swordsman;
	//Aumtlich beams cannot hit entities hiding in Shallow Water unless
	//the entity is their current target.
	bool bWaterCover = bIsShallowWater(wOSquare) && (cx != wTX || cy != wTY);
	CMonster *pMonster = pRoom->GetMonsterAtSquare(cx, cy);
	if (pMonster)
	{
		//If this square is possible water cover, then check
		//that the monster is hiding here.
		bool bHideinWater = false;
		if (bWaterCover)
		{
			if (pMonster->wType == M_CLONE || pMonster->wType == M_TEMPORALCLONE)
				bHideinWater = player.GetWaterTraversalState(pMonster->GetIdentity()) == WTrv_CanHide;
			else
				bHideinWater = bCanEntityHideInShallowWater(pMonster->GetResolvedIdentity());
		}
		if (!bHideinWater)
		{
			if (pAumtlich)
				if (bIsStalwart(pMonster->wType) || pMonster->wType == M_MIMIC || pMonster->wType == M_TEMPORALCLONE)
				{
					CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
					if (pDouble->IsTarget() && (!pDouble->HasSword() || pDouble->wO != nGetReverseO(nGetO(dx,dy))))
					{
						pDouble->bFrozen = true;
						CueEvents.Add(CID_PlayerFrozen, pMonster);
					}
				}
			return false;   //Monster blocks gaze
		}
	}

	//Is player frozen?
	if (pCurrentGame->IsPlayerAt(cx, cy))
	{
		if (!bWaterCover || player.GetWaterTraversalState() != WTrv_CanHide)
		{
			if (pAumtlich)
			{
				//Only types targeted by aumtlich are affected by gaze.
				if (player.IsTarget() && !(player.HasWeapon() && player.wO == nGetReverseO(nGetO(dx,dy))))
				{
					CueEvents.Add(CID_PlayerFrozen, &player);
					player.bFrozen = true;
				}
			}

			//Blocks gaze.
			return false;
		}
	}

	//Does a weapon block the gaze?
	bool bBlocked = false;
	if (SwordCoords.Exists(cx,cy))
	{
		bool bReflect = true;  //Whether a standard reflection takes place
		bool bPickaxe = false; //Whether a pickaxe reflection takes place
		bool bDagger  = false; //Whether a dagger block takes place
		bool bCaber   = false; //Whether a caber block takes place
		const UINT wO = nGetO(dx,dy);
		//There may be multiple swords at this location
		//Get all swords surrounding the tile

		const CSwordsman& player = pCurrentGame->swordsman;
		if (pCurrentGame->IsPlayerWeaponAt(cx, cy))
		{
			const UINT playerWeapon = player.GetActiveWeapon();
			const DeflectType eType = DeflectAngle(wO, player.wO, playerWeapon);
			if (eType != GazePassesThrough) {
				if (playerWeapon == WT_Dagger)
					bDagger = true;
				else {
					bBlocked = true;
					if (eType == GazeBlocked)
					{
						bReflect = false;
						if (playerWeapon == WT_Caber)
							bCaber = true;
					} else if (playerWeapon == WT_Pickaxe)
						bPickaxe = true;
				}
			}
		}

		for (UINT nO=0; nO<ORIENTATION_COUNT; ++nO) //O(9) search
			if (nO != NO_ORIENTATION)
			{
				CMonster *pMonster = pRoom->GetMonsterAtSquare(cx-nGetOX(nO), cy-nGetOY(nO));
				if (pMonster && pMonster->HasSwordAt(cx, cy))
				{
					const UINT weaponType = pMonster->GetWeaponType();
					const DeflectType eType = DeflectAngle(wO, nO, weaponType);
					if (eType != GazePassesThrough) {
						if (weaponType == WT_Dagger)
							bDagger = true;
						else {
							bBlocked = true;
							if (eType == GazeBlocked)
							{
								bReflect = false;
								if (weaponType == WT_Caber)
									bCaber = true;
							} else if (weaponType == WT_Pickaxe)
								bPickaxe = true;
						}
					}
				}
			}

		if (bBlocked) {
			//If reflected, freeze aumtlich
			if (pAumtlich && (bReflect || bPickaxe))
				pAumtlich->bFrozen = true;

			//Delay blocking if a Pickaxe or Caber is not performing the block/reflection
			if (bPickaxe || bCaber)
				return false;
		}
		//Daggers only contribute to blocking if nothing else reflects the gaze
		if (bDagger)
			bBlocked = true;
	}

	//Gaze lights fuses.
	if (pAumtlich && wTSquare == T_FUSE)
		pRoom->LightFuseEnd(CueEvents, cx, cy);

	//If a weapon blocked the gaze from continuing, stop the beam now.
	if (bBlocked)
		return false;

	//Gaze continues.
	cx += dx;
	cy += dy;
	return true;
}

//*****************************************************************************************
CZombie::DeflectType CZombie::DeflectAngle(
//When zombie gaze encounters a surface, determine whether the gaze would
//reflect or be blocked by the specific weapon type
//
//Params:
	const UINT wInAngle,        //(in)
	const UINT wDeflectSurface, //(in)
	const UINT weaponType)      //(in)
{
	ASSERT(wInAngle != NO_ORIENTATION);
	ASSERT(wDeflectSurface != NO_ORIENTATION);
	ASSERT(wInAngle != wDeflectSurface);
	
	UINT wO;

	switch(weaponType)
	{
		case WT_Sword:
			wO = nNextCO(nNextCO(wDeflectSurface));
			if (wInAngle == wO || wInAngle == nGetReverseO(wO))
				return GazeReturned;
		break;
		case WT_Pickaxe:
			wO = nGetReverseO(wDeflectSurface);
			if (wInAngle == wO || wInAngle == nNextCO(wO) || wInAngle == nNextCCO(wO))
				return GazeReturned;
		break;
		//These have no reflective surface
		case WT_Spear:
		case WT_Staff:
		case WT_Caber:
		break;

		//Dagger only blocks direct gaze
		case WT_Dagger:
			wO = nGetReverseO(wDeflectSurface);
			if (wInAngle != wO)
				return GazePassesThrough;
		break;
	}

	return GazeBlocked;
}
