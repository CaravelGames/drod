// $Id: Swordsman.cpp 10108 2012-04-22 04:54:24Z mrimer $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Swordsman.cpp.
//Implementation of CSwordsman,

#include "Swordsman.h"
#include "Combat.h"
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "Character.h"
#include <BackEndLib/Assert.h>
#include <math.h>

//
// Public methods
//

//*****************************************************************************
CSwordsman::CSwordsman(CCurrentGame *pSetCurrentGame)
	: CPlayerDouble(defaultPlayerType(), pSetCurrentGame, 1)
{
	Clear(false);
}

//*****************************************************************************
UINT CSwordsman::CalcDamage(int damageVal) const
//Calculate damage to player from a game element.  If hasted, damage is halved.
//
//Returns: calculated damage
{
	UINT delta;
	if (damageVal < 0)
	{
		//Flat-rate damage.
		delta = -damageVal;
		if (IsHasted())
			delta /= 2;
	} else {
		//Fractional damage (percent HP lost).
		float fDamageFactor = damageVal / 100.0f;
		ASSERT(fDamageFactor >= 0.0f);
		float fDelta = this->st.HP * fDamageFactor;
		if (IsHasted())
			fDelta /= 2.0f;
		delta = UINT(ceil(fDelta)); //ceiling function
	}
	return delta;
}

//*****************************************************************************
bool CSwordsman::IsFlying() const
//Returns: whether the player type allows flying
{
	return this->wAppearance == M_WWING || this->wAppearance == M_FEGUNDO;
}

//*****************************************************************************
bool CSwordsman::CanJump(const int dx, const int dy) const
//Returns: whether player has the equipment to jump a tile in (dx,dy) direction
{
	if (this->st.accessory != GrapplingHook) //must have grappling hook equipped
		return false;
	return (nGetOX(this->wO) == dx && nGetOY(this->wO) == dy);
}

//*****************************************************************************
bool CSwordsman::CanWalkOnWater() const
//Returns: whether player has the ability to walk on water
{
	//These monster base types may move over water
	if (bIsEntityFlying(this->wAppearance) ||
			this->wAppearance == M_WATERSKIPPER)
		return true;

	return this->st.accessory == WaterBoots; //must have grappling hook equipped
}

//*****************************************************************************
void CSwordsman::Clear(const bool bNewGame)
{
	this->wX = this->wY = this->wO = this->wPrevX = this->wPrevY = this->wPrevO = 0;
	this->wAppearance = defaultPlayerType();
	this->bSwordOff = this->bShieldOff = this->bAccessoryOff = this->bCommandOff = false;
	this->bHasTeleported = false;
	ResetRoomStats();
	if (bNewGame)
		this->st.clear();

	this->bIntraRoomPath = this->bPathToStairs = false;
}

//*****************************************************************************
UINT CSwordsman::Damage(CCueEvents& CueEvents, int damageVal, CUEEVENT_ID deathCID)
//Damage player from a game element.  If hasted, damage is halved.
{
	const UINT delta = CalcDamage(damageVal);

	if (delta)
		DecHealth(CueEvents, delta, deathCID);
	return delta;
}

//*****************************************************************************
void CSwordsman::DecHealth(CCueEvents& CueEvents, const UINT delta, CUEEVENT_ID deathCID)
{
	ASSERT(delta);
	if (delta > this->st.HP)
		this->st.HP = 0;
	else
		this->st.HP -= delta;

	CueEvents.Add(CID_EntityAffected,
			new CCombatEffect(this, CET_HARM, delta), true);

	if (!this->st.HP) //player died
		CueEvents.Add(deathCID);
}

//*****************************************************************************
UINT CSwordsman::GetIdentity() const
//Returns: logical identity
{
	return this->wIdentity;
}

//*****************************************************************************
int CSwordsman::getGoldMultiplier() const
//Returns: gold multiplier
{
	int multiplier = 1; //default

	if (this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Weapon))
		multiplier *= 2;
	if (this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Armor))
		multiplier *= 2;
	if (this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Accessory))
		multiplier *= 2;

	return multiplier;
}

//*****************************************************************************
int CSwordsman::getXPMultiplier() const
//Returns: XP multiplier
{
	int multiplier = 1; //default

	if (this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Weapon))
		multiplier *= 2;
	if (this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Armor))
		multiplier *= 2;
	if (this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Accessory))
		multiplier *= 2;

	return multiplier;
}

//*****************************************************************************
bool CSwordsman::IsInRoom() const
//Returns: whether player is controlling a character in the game room
{
	//'none' indicates player is not controlling any character
	return this->wAppearance != M_NONE;
}

//*****************************************************************************
bool CSwordsman::IsOpenMove(const UINT wX, const UINT wY, const int dx, const int dy) const
//More general than IsOpenMove(dx,dy).
//
//NOTE: Currently used only for quick backtracking and intra-room pathmapping.
//
//Returns: whether moving from (wX,wY) along (dx,dy) is valid.
{
	const CDbRoom& room = *(this->pCurrentGame->pRoom);

	//Do arrows prevent movement?
	if (DoesArrowPreventMovement(wX, wY, dx, dy) ||
			room.DoesSquarePreventDiagonal(wX, wY, dx, dy))
		return false;

	//Some objects are obstacles.
	//Also, we're looking for a clear path, so don't allow picking up any items.
	const UINT wNewX = wX + dx, wNewY = wY + dy;
	const UINT oTile = room.GetOSquare(wNewX, wNewY);
	if (IsTileObstacle(oTile))
	{
		if (!(bIsDoor(oTile) && bIsDoor(room.GetOSquare(wX, wY))) && //may walk along door tops
			 !(bIsWater(oTile) && CanWalkOnWater()) && //can cross water tiles
			 !(bIsPit(oTile) && bIsEntityFlying(wAppearance))) //can fly over pit
			return false;
	}

	if (IsTileObstacle(room.GetFSquare(wNewX, wNewY)))
		return false;

	const UINT tTile = room.GetTSquare(wNewX, wNewY);
	switch (tTile)
	{
		case T_FUSE:
			//Allow stepping on fuse if it won't light it.
			if (room.FuseEndAt(wNewX, wNewY) != NO_ORIENTATION)
				return false;
			break;
		default:
			if (IsTileObstacle(tTile))
				return false;
			break;
	}

	//Monsters and monster swords at square can incur damage.
	//Cannot form a path through these when quick backtracking.
	CMonster *pMonster = room.GetMonsterAtSquare(wNewX, wNewY);

	if (pMonster && pMonster->IsPiece())
		return false; //can't travel through monster pieces

	if (this->bIntraRoomPath)
	{
		if (pMonster && !pMonster->IsCombatable())
			return false;
	} else {
		if (pMonster)
			return false;
		if (room.IsMonsterSwordAt(wNewX, wNewY, this))
			return false;
	}

	//Player's sword is not allowed to hit anything affected by sword hits.
	const bool bDoesTileDisableMetal = this->pCurrentGame->DoesTileDisableMetal(wNewX, wNewY);
	const bool swordless = (this->st.sword == NoSword || this->bSwordOff || this->pCurrentGame->IsPlayerSwordRemoved());
	if (!swordless &&
			(!this->pCurrentGame->IsSwordMetal(this->st.sword) || !bDoesTileDisableMetal))
	{
		//A sword is being wielded and it is not disabled.
		const UINT wSX = wNewX + nGetOX(this->wO), wSY = wNewY + nGetOY(this->wO);
		if (room.IsValidColRow(wSX, wSY))
		{
			//If player has a sword, even if he might a disarm token in effect
			//for part of the path, we still will find only paths
			//that wouldn't allow sword hits along those points for simplicity.
			//Otherwise, the logic to figure the player's sword state at each tile
			//along the path would be much more complex.
			//
			//There is also the issue of how goo will change the player's orientation
			//along the path, but I'm not ready to implement logic to keep track
			//of this during pathmapping yet.
/*	//Players would get confused if a seemingly clear path can't be taken
	//because their sword would inadvertently hit a crumbly/secret wall.
	//Therefore, we'll allow hitting these with the sword on quick exits.
			switch (room.GetOSquare(wSX, wSY))
			{
				case T_WALL_B:
				case T_WALL_H:
					if (!this->bIntraRoomPath)
						return false;
					break;
				default: break;
			}
*/
			switch (room.GetTSquare(wSX, wSY))
			{
				case T_ORB:
				case T_BOMB:
					return false;
				case T_MIRROR:
					if (!this->bIntraRoomPath)
						return false;
					break;
				case T_TAR: case T_MUD: case T_GEL:
					if (room.IsTarVulnerableToStab(wSX, wSY))
						return false;
				break;
				case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
					if (this->pCurrentGame->CanPlayerCutBriars())
						return false;
				break;
				default: break;
			}
			if (!this->bIntraRoomPath)
				if (room.GetMonsterAtSquare(wSX, wSY))
					return false;
		}
	}

	return true;
}

//*****************************************************************************
bool CSwordsman::IsSwordDisabled() const
//Returns: whether player is unable to use their sword
{
	return
		this->bSwordOff || //script has disabled sword
		this->bNoSword || //disarm token in effect
		CPlayerDouble::IsSwordDisabled();
}

//*****************************************************************************
bool CSwordsman::IsTileObstacle(
//Overridable method to determine if a tile is an obstacle for the player.
//
//NOTE: This method is called for intelligent player pathmapping to run
//quickly through clean rooms.  This means no items should be picked up on the
//way, hence are considered obstacles to the pathmap.
//
//Params:
	const UINT wLookTileNo) //(in)   Tile to evaluate.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	//All the things the player can step onto during a room quick exit path.
	if (
		wLookTileNo==T_EMPTY ||
		bIsPlainFloor(wLookTileNo) ||
		bIsBridge(wLookTileNo) ||
		wLookTileNo==T_GOO ||
		bIsOpenDoor(wLookTileNo) ||
		bIsArrow(wLookTileNo) ||
		bIsPlatform(wLookTileNo) ||
		wLookTileNo==T_NODIAGONAL ||
		wLookTileNo==T_SCROLL ||
		wLookTileNo==T_TOKEN ||
		wLookTileNo==T_PRESSPLATE ||
		(bIsTrapdoor(wLookTileNo) && !HasSword()) //won't drop trapdoors
	)
		return false;

	//Items the player can step on while executing a smart intra-room path.
	if (this->bIntraRoomPath)
	{
		if (bIsTrapdoor(wLookTileNo))
			return false;
		if (bIsPowerUp(wLookTileNo) || bIsEquipment(wLookTileNo) ||
				bIsMap(wLookTileNo) || bIsShovel(wLookTileNo) ||
				wLookTileNo == T_KEY || wLookTileNo == T_MIRROR || wLookTileNo == T_CRATE)
			return false;

		//Accessory-specific paths.
		if (bIsWater(wLookTileNo) && this->st.accessory == WaterBoots)
			return false;
	}

	//When path-mapping to stairs, allow walking through other stair tiles
	//because the tile chosen by the user might not be adjacent to open tiles.
	if (this->bPathToStairs)
	{
		if (bIsStairs(wLookTileNo))
			return false;
	}

	return true;
}

//*****************************************************************************
bool CSwordsman::Move(
//Move player to new location
//
//Returns: whether player was moved
//
//Params:
	const UINT wSetX, const UINT wSetY) //(in) position to move to
{
	const bool bMoved = !(this->wX == wSetX && this->wY == wSetY);

	//Set new player coords.
	if (this->wIdentity == M_NONE)
	{
		this->wPrevX = wSetX;
		this->wPrevY = wSetY;
	}
	else {
		this->wPrevX = this->wX;
		this->wPrevY = this->wY;
	}
	this->wPrevO = this->wO;
	this->wX = wSetX;
	this->wY = wSetY;

//	SetSwordCoords();

	return bMoved;
}

//*****************************************************************************************
void CSwordsman::Process(
//Process the player for movement.
//
//Params:
	const int /*nLastCommand*/,   //(in) Last swordsman command.
	CCueEvents& /*CueEvents*/)  //(in/out) Accepts a CCueEvents object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
}

//*****************************************************************************
void CSwordsman::ResetRoomStats()
//Reset player stats limited to the current room.
{
/*	this->wPlacingDoubleType = 0;
	this->bFrozen = false;
	this->bIsTarget = this->bCanGetItems = false;
*/
	this->bAlive = true;
	this->bNoSword = this->bSwordSheathed = false;
	this->bHasTeleported = false;

	this->bInvisible = this->bHasted = false; //only lasts for the current room
}

//*****************************************************************************
void CSwordsman::RotateClockwise()
//Rotate player clockwise.
//Keeps track of previous orientation.
{
	this->wPrevX = this->wX;
	this->wPrevY = this->wY;
	this->wPrevO = this->wO;
	this->wO = nNextCO(this->wO);
	ASSERT(IsValidOrientation(this->wO));

//	SetSwordCoords();
}

//*****************************************************************************
void CSwordsman::RotateCounterClockwise()
//Rotate player counter-clockwise.
//Keeps track of previous orientation.
{
	this->wPrevX = this->wX;
	this->wPrevY = this->wY;
	this->wPrevO = this->wO;
	this->wO = nNextCCO(this->wO);
	ASSERT(IsValidOrientation(this->wO));

//	SetSwordCoords();
}

//*****************************************************************************
void CSwordsman::SetOrientation(
//Move player to new location
//
//Returns: whether player was moved
//
//Params:
	const UINT wO) //(in) new orientation
{
	ASSERT(IsValidOrientation(wO));
	this->wO = this->wPrevO = wO;

//	SetSwordCoords();
}

//*****************************************************************************
bool CSwordsman::HasSword() const
//Whether player is carrying a sword in front.
{
	if (this->st.sword == NoSword)
		return false;

	if (this->pCurrentGame->IsPlayerSwordRemoved())
		return false;

	return !IsSwordDisabled();
}

//*****************************************************************************
UINT CSwordsman::GetSwordMovement(
//Returns: movement sword made to
//
//Params:
	const int nCommand, const UINT wO)     //(in)   Game command.
{
	switch (nCommand)
	{
		case CMD_C: //sword moved orthogonal to direction it's now facing
			switch (wO)
			{
				case NW: return NE;
				case N: return E;
				case NE: return SE;
				case W: return N;
				case E: return S;
				case SW: return NW;
				case S: return W;
				case SE: return SW;
			}
			break;
		case CMD_CC:
			switch (wO)
			{
				case NW: return SW;
				case N: return W;
				case NE: return NW;
				case W: return S;
				case E: return N;
				case SW: return SE;
				case S: return E;
				case SE: return NE;
			}
			break;
		case CMD_NW: return NW;
		case CMD_N: return N;
		case CMD_NE: return NE;
		case CMD_W: return W;
		case CMD_E: return E;
		case CMD_SW: return SW;
		case CMD_S: return S;
		case CMD_SE: return SE;
	}
	return NO_ORIENTATION;
}
