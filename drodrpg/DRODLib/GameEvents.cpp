#include "GameEvents.h"
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
 * Portions created by the Initial Developer are Copyright (C) 2026
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */
#include "GameEvents.h"

#include "DbRooms.h"
#include "Db.h"
#include "TileConstants.h"

//Substitution strings
const WSTRING wstrPosition = WS("%position%");
const WSTRING wstrTile = WS("%tile%");
const WSTRING wstrMonster = WS("%monster%");
const WSTRING wstrEquipment = WS("%equipment%");
const WSTRING wstrStart = WS("%start%");
const WSTRING wstrDestination = WS("%destination%");
const WSTRING wstrOld = WS("%old%");
const WSTRING wstrNew = WS("%new%");

//*****************************************************************************
WSTRING intToText(const int value, const bool usePlus)
//Returns: WSTRING representation of int, with + prepended to positive value
//if usePlus is true.
{
	WSTRING wstr;
	if (value > 0 && usePlus) {
		wstr += wszPlus;
	}

	wstr += to_WSTRING(value);
	return wstr;
}

//*****************************************************************************
WSTRING coordinateToWSTRING(CCoord cordinate)
//Returns: WSTRING representation of CCoord in form (wX,wY).
{
	WSTRING positionStr = wszLeftParen;
	positionStr += to_WSTRING(cordinate.wX);
	positionStr += wszComma;
	positionStr += to_WSTRING(cordinate.wY);
	positionStr += wszRightParen;
	return positionStr;
}

//*****************************************************************************
CGameEvent::CGameEvent(GameEventType type)
	: eventType(type)
{}

//*****************************************************************************
CEnterRoomEvent::CEnterRoomEvent(CDbRoom* pRoom, const PlayerStats& ps)
	: CGameEvent(GameEventType::GE_EnterRoom), locationDescription()
	, hp (ps.HP), atk(ps.ATK), def(ps.DEF), gr(ps.GOLD), xp(ps.XP)
	, yellowKey(ps.yellowKeys), greenKey(ps.greenKeys), blueKey(ps.blueKeys)
	, skeletonKey(ps.skeletonKeys), shovels(ps.shovels)
{
	CDbLevel* pLevel = pRoom->GetCurrentGame()->pLevel;
	this->locationDescription = (const WCHAR*)pLevel->NameText;
	WSTRING position;
	pRoom->GetLevelPositionDescription(position);
	this->locationDescription += wszColon;
	this->locationDescription += wszSpace;
	this->locationDescription += position;
}

//*****************************************************************************
WSTRING CEnterRoomEvent::toText() const
{
	WSTRING wstr;
	wstr = this->locationDescription;
	wstr += wszCRLF;

	wstr += wszLeftBracket;
	wstr += intToText(this->hp, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
	wstr += wszCommaSpace;
	wstr += intToText(this->atk, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_ATKStat);
	wstr += wszCommaSpace;
	wstr += intToText(this->def, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_DEFStat);

	if (this->gr != 0) {
		wstr += wszCommaSpace;
		wstr += intToText(this->gr, false);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_GRStat);
	}
	if (this->xp != 0) {
		wstr += wszCommaSpace;
		wstr += intToText(this->xp, false);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_XPStat);
	}

	wstr += wszCommaSpace;
	wstr += intToText(this->yellowKey, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_YKEYStat);
	wstr += wszCommaSpace;
	wstr += intToText(this->greenKey, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_GKEYStat);
	wstr += wszCommaSpace;
	wstr += intToText(this->blueKey, false);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_BKEYStat);
	if (this->skeletonKey != 0) {
		wstr += wszCommaSpace;
		wstr += intToText(this->skeletonKey, false);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_SKEYStat);
	}

	if (this->shovels != 0) {
		wstr += wszCommaSpace;
		wstr += intToText(this->shovels, false);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_ShovelsStat);
	}

	wstr += wszRightBracket;

	return wstr;
}

//*****************************************************************************
CStatChangeEvent::CStatChangeEvent(GameEventType type)
	: CGameEvent(type)
	, hp(0), atk(0), def(0), gr(0), xp(0), shovels(0)
	, yellowKey(0), greenKey(0), blueKey(0), skeletonKey(0)
{}

//*****************************************************************************
void CStatChangeEvent::addHP(int amount)
{
	this->hp += amount;
}

//*****************************************************************************
void CStatChangeEvent::addATK(int amount)
{
	this->atk += amount;
}

//*****************************************************************************
void CStatChangeEvent::addDEF(int amount)
{
	this->def += amount;
}

//*****************************************************************************
void CStatChangeEvent::addShovels(int amount)
{
	this->shovels += amount;
}

//*****************************************************************************
void CStatChangeEvent::addGR(const int amount)
{
	this->gr += amount;
}

//*****************************************************************************
void CStatChangeEvent::addXP(const int amount)
{
	this->xp += amount;
}

//*****************************************************************************
void CStatChangeEvent::addYellowKey(int amount)
{
	this->yellowKey += amount;
}

//*****************************************************************************
void CStatChangeEvent::addGreenKey(int amount)
{
	this->greenKey += amount;
}

//*****************************************************************************
void CStatChangeEvent::addBlueKey(int amount)
{
	this->blueKey += amount;
}

//*****************************************************************************
void CStatChangeEvent::addSkeletonKey(int amount)
{
	this->skeletonKey += amount;
}

//*****************************************************************************
WSTRING CStatChangeEvent::toText() const
{
	WSTRING wstr;
	bool needSpace = false;

	if (this->hp != 0) {
		wstr += intToText(this->hp, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
		needSpace = true;
	}

	if (this->atk != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->atk, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_ATKStat);
		needSpace = true;
	}

	if (this->def != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->def, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_DEFStat);
		needSpace = true;
	}

	if (this->gr != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->gr, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_GRStat);
		needSpace = true;
	}

	if (this->xp != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->xp, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_XPStat);
		needSpace = true;
	}

	if (this->yellowKey != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->yellowKey, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_YKEYStat);
		needSpace = true;
	}

	if (this->greenKey != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->greenKey, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_GKEYStat);
		needSpace = true;
	}

	if (this->blueKey != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->blueKey, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_BKEYStat);
		needSpace = true;
	}

	if (this->skeletonKey != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->skeletonKey, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_SKEYStat);
		needSpace = true;
	}

	if (this->shovels != 0) {
		if (needSpace) {
			wstr += wszCommaSpace;
		}

		wstr += intToText(this->shovels, usePlus());
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_ShovelsStat);
		needSpace = true;
	}

	return wstr;
}

//*****************************************************************************
CCollectedItemEvent::CCollectedItemEvent()
	: CStatChangeEvent(GE_CollectItem)
{}

//*****************************************************************************
WSTRING CCollectedItemEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_Collected);
	wstr += wszSpace;
	wstr += CStatChangeEvent::toText();
	return wstr;
}

//*****************************************************************************
CUseKeyOnDoorEvent::CUseKeyOnDoorEvent(KeyType type, UINT x, UINT y, bool opened)
	: CGameEvent(GE_UseKeyOnDoor), type(type), position(x, y), opened(opened)
{}

//*****************************************************************************
WSTRING CUseKeyOnDoorEvent::toText() const
{
	WSTRING wstr = this->opened ? g_pTheDB->GetMessageText(MID_GameLog_OpenedDoor) :
		g_pTheDB->GetMessageText(MID_GameLog_ClosedDoor);

	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	wstr += wszSpace;
	wstr += wszLeftParen;
	wstr += wszHyphen;
	wstr += wszOne;
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(getKeyStatMID(this->type));
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CUseMoneyOnDoorEvent::CUseMoneyOnDoorEvent(int cost, UINT x, UINT y, bool opened)
	: CGameEvent(GE_UseMoneyOnDoor), cost(cost), position(x,y), opened(opened)
{}

//*****************************************************************************
WSTRING CUseMoneyOnDoorEvent::toText() const
{
	WSTRING wstr = this->opened ? g_pTheDB->GetMessageText(MID_GameLog_OpenedDoor) :
		g_pTheDB->GetMessageText(MID_GameLog_ClosedDoor);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	wstr += wszSpace;
	wstr += wszLeftParen;
	if (this->cost < 0) {
		wstr += wszPlus;
	}
	wstr += to_WSTRING(-this->cost);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_GRStat);
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CDigDirtEvent::CDigDirtEvent(UINT cost, UINT x, UINT y)
	: CGameEvent(GE_DigDirt), cost(cost), position(x, y)
{}

//*****************************************************************************
WSTRING CDigDirtEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_DugDirt);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	wstr += wszSpace;
	wstr += wszLeftParen;
	wstr += to_WSTRING(-cost);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_ShovelsStat);
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CLightFuseEvent::CLightFuseEvent(const UINT wX, const UINT wY)
	: CGameEvent(GE_LightFuse), position(wX, wY)
{}

//*****************************************************************************
WSTRING CLightFuseEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_LitFuse);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
CExplosiveExplodedEvent::CExplosiveExplodedEvent(
	const UINT tileType, const UINT wX, const UINT wY)
	: CGameEvent(GE_ExplosiveExploded), tileType(tileType), position(wX, wY)
{}

//*****************************************************************************
WSTRING CExplosiveExplodedEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_TileExploded);
	wstr = WCSReplace(wstr, wstrTile, g_pTheDB->GetMessageText(TILE_MID[this->tileType]));
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
CCombatEvent::CCombatEvent(const WSTRING& monsterName, UINT wX, UINT wY)
	: CGameEvent(GE_Combat), monsterName(monsterName), position(wX, wY)
	, hpDelta(0), grDelta(0), xpDelta(0)
{}

//*****************************************************************************
void CCombatEvent::setResults(const int hpDelta, const int grDelta, const int xpDelta)
{
	this->hpDelta = hpDelta;
	this->grDelta = grDelta;
	this->xpDelta = xpDelta;
}

//*****************************************************************************
WSTRING CCombatEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_FoughtMonster);
	wstr = WCSReplace(wstr, wstrMonster, this->monsterName);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	if (this->hpDelta != 0 || this->grDelta != 0 || this->xpDelta != 0) {
		bool needSpace = false;
		wstr += wszSpace;
		wstr += wszLeftParen;
		if (this->hpDelta != 0) {
			wstr += intToText(this->hpDelta, true);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
			needSpace = true;
		}

		if (this->grDelta != 0) {
			if (needSpace) {
				wstr += wszCommaSpace;
			}
			wstr += intToText(this->grDelta, true);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_GRStat);
			needSpace = true;
		}

		if (this->xpDelta != 0) {
			if (needSpace) {
				wstr += wszCommaSpace;
			}
			wstr += intToText(this->xpDelta, true);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_XPStat);
		}

		wstr += wszRightParen;
	}

	return wstr;
}

//*****************************************************************************
CMonsterAttackEvent::CMonsterAttackEvent(
	const WSTRING& monsterName, UINT wX, UINT wY, UINT damage)
	: CGameEvent(GE_MonsterAttack)
	, monsterName(monsterName), position(wX, wY), damage(damage)
{}

//*****************************************************************************
WSTRING CMonsterAttackEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_AttackByMonster);
	wstr = WCSReplace(wstr, wstrMonster, this->monsterName);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	wstr += wszSpace;
	wstr += wszLeftParen;
	wstr += to_WSTRING(-damage);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CBeamDamageEvent::CBeamDamageEvent(
	const UINT wX, const UINT wY, const UINT damage)
	: CGameEvent(CE_BeamDamage), position(wX, wY), damage(damage)
{}

//*****************************************************************************
WSTRING CBeamDamageEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_HitByBeam);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	wstr += wszSpace;
	wstr += wszLeftParen;
	wstr += to_WSTRING(-damage);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CTileDamageEvent::CTileDamageEvent(
	const UINT tileType, const UINT wX, const UINT wY, const UINT damage)
	: CGameEvent(GE_TileDamage), tileType(tileType)
	, position(wX, wY), damage(damage)
{}

//*****************************************************************************
WSTRING CTileDamageEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_HurtByTile);
	wstr = WCSReplace(wstr, wstrTile, g_pTheDB->GetMessageText(TILE_MID[this->tileType]));
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));

	wstr += wszSpace;
	wstr += wszLeftParen;
	wstr += to_WSTRING(-damage);
	wstr += wszSpace;
	wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
	wstr += wszRightParen;

	return wstr;
}

//*****************************************************************************
CMonsterKilledEvent::CMonsterKilledEvent(
	const WSTRING& monsterName, UINT wX, UINT wY
)
	: CGameEvent(GE_MonsterKilled), monsterName(monsterName), position(wX, wY)
{}

//*****************************************************************************
WSTRING CMonsterKilledEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_MonsterKilled);
	wstr = WCSReplace(wstr, wstrMonster, this->monsterName);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
CSwapEquipmentEvent::CSwapEquipmentEvent(const ScriptFlag::EquipmentType equipType,
	const WSTRING& oldEquipmentName, const WSTRING& newEquipmentName,
	const UINT wX, const UINT wY
)
	: CGameEvent(GE_SwapEquipment) , equipType(equipType)
	, oldEquipmentName(oldEquipmentName), newEquipmentName(newEquipmentName)
	, position(wX, wY)
{}

//*****************************************************************************
WSTRING CSwapEquipmentEvent::toText() const
{
	WSTRING wstr = getBaseString();
	wstr = WCSReplace(wstr, wstrOld, this->oldEquipmentName);
	wstr = WCSReplace(wstr, wstrNew, this->newEquipmentName);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
WSTRING CSwapEquipmentEvent::getBaseString() const
{
	switch (this->equipType) {
		case ScriptFlag::Weapon: return g_pTheDB->GetMessageText(MID_GameLog_SwapWeapon);
		case ScriptFlag::Armor: return g_pTheDB->GetMessageText(MID_GameLog_SwapShield);
		case ScriptFlag::Accessory: return g_pTheDB->GetMessageText(MID_GameLog_SwapAccessory);
		default: return WS("");
	}
}

//*****************************************************************************
CUseEquipmentEvent::CUseEquipmentEvent(
	const ScriptFlag::EquipmentType equipType,
	const WSTRING& equipmentName,
	const UINT wX, const UINT wY
)
	: CGameEvent(GE_UseEquipment), equipType(equipType)
	, position(wX, wY), equipmentName(equipmentName)
{}

//*****************************************************************************
WSTRING CUseEquipmentEvent::toText() const
{
	WSTRING wstr = getBaseString();
	wstr = WCSReplace(wstr, wstrEquipment, this->equipmentName);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
WSTRING CUseEquipmentEvent::getBaseString() const
{
	switch (this->equipType) {
	case ScriptFlag::Weapon: return g_pTheDB->GetMessageText(MID_GameLog_UseWeapon);
	case ScriptFlag::Armor: return g_pTheDB->GetMessageText(MID_GameLog_UseShield);
	case ScriptFlag::Accessory:
	default: return g_pTheDB->GetMessageText(MID_GameLog_UseEquipment);
	}
}

//*****************************************************************************
CUsePickaxeOnWallEvent::CUsePickaxeOnWallEvent(const UINT wX, const UINT wY)
	: CGameEvent(GE_UsePickaxeOnWall), position(wX, wY)
{}

//*****************************************************************************
WSTRING CUsePickaxeOnWallEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_UsePickaxe);
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(position));
	return wstr;
}

//*****************************************************************************
CUsePortableOrbOnDoorEvent::CUsePortableOrbOnDoorEvent(
	const UINT tileType, const UINT wX, const UINT wY)
	: CGameEvent(GE_UsePortableOrbOnDoor), tileType(tileType), position(wX, wY)
{}

//*****************************************************************************
WSTRING CUsePortableOrbOnDoorEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_UsePortableOrb);
	wstr = WCSReplace(wstr, wstrTile, g_pTheDB->GetMessageText(TILE_MID[this->tileType]));
	wstr = WCSReplace(wstr, wstrPosition, coordinateToWSTRING(this->position));
	return wstr;
}

//*****************************************************************************
CUseWarpAccessoryEvent::CUseWarpAccessoryEvent(
	const AccessoryType accessoryType,
	const UINT wX, const UINT wY,
	const UINT wDestX, const UINT wDestY
)
	:CGameEvent(GE_UseWarpAccesory), accessoryType(accessoryType)
	, start(wX, wY), destination(wDestX, wDestY)
{}

//*****************************************************************************
WSTRING CUseWarpAccessoryEvent::toText() const
{
	WSTRING wstr = getBaseString();
	wstr = WCSReplace(wstr, wstrStart, coordinateToWSTRING(this->start));
	wstr = WCSReplace(wstr, wstrDestination, coordinateToWSTRING(this->destination));
	return wstr;
}

//***** ************************************************************************
WSTRING CUseWarpAccessoryEvent::getBaseString() const
{
	switch (accessoryType) {
		case AccessoryType::WarpToken: return g_pTheDB->GetMessageText(MID_GameLog_UseWarpToken);
		case AccessoryType::WallWalking: return g_pTheDB->GetMessageText(MID_GameLog_UseWallWalking);
		default: return g_pTheDB->GetMessageText(MID_GameLog_WarpWithAccessory);
	}
}


//*****************************************************************************
CScriptedStatChangeEvent::CScriptedStatChangeEvent(UINT turn)
	: CStatChangeEvent(GE_ScriptedStatChange), turn(turn)
{}

//*****************************************************************************
WSTRING CScriptedStatChangeEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_ScriptChangedStats);
	wstr += wszSpace;
	wstr += CStatChangeEvent::toText();
	return wstr;
}

//*****************************************************************************
CScoreCheckpointEvent::CScoreCheckpointEvent(const WSTRING& name, int score)
	: CGameEvent(GE_ScoreCheckpoint), scoreCheckpointName(name), score(score)
{}

//*****************************************************************************
WSTRING CScoreCheckpointEvent::toText() const
{
	WSTRING wstr = g_pTheDB->GetMessageText(MID_GameLog_ScoreCheckpoint);
	wstr += wszSpace;
	wstr += this->scoreCheckpointName;
	wstr += wszSpace;
	wstr += wszHyphen;
	wstr += wszSpace;
	wstr += to_WSTRING(this->score);
	return wstr;
}
