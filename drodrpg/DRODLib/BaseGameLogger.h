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

//CBaseGameLogger.h
//Base class for logging of game actions. As well as being the base class, it
//acts a "null" implementation that doesn't log anything.

#ifndef BASEGAMELOGGER
#define BASEGAMELOGGER

#include "RoomData.h"

//*****************************************************************************
class CDbRoom;
class CMonster;
class PlayerStats;
class CBaseGameLogger {
public:
	CBaseGameLogger() = default;
	~CBaseGameLogger() = default;

	virtual void enterRoom(CDbRoom* room, const PlayerStats& ps) {}

	virtual void collectHP(const int amount) {}
	virtual void collectATK(const int amount) {}
	virtual void collectDEF(const int amount) {}
	virtual void collectShovels(const int amount) {}
	virtual void collectKey(const KeyType type) {}

	virtual void openDoorWithKey(const KeyType type, const UINT wX, const UINT wY) {}
	virtual void closeDoorWithKey(const KeyType type, const UINT wX, const UINT wY) {}
	virtual void openDoorWithMoney(const int cost, const UINT wX, const UINT wY) {}
	virtual void closeDoorWithMoney(const int cost, const UINT wX, const UINT wY) {}

	virtual void digDirt(const UINT cost, const UINT wX, const UINT wY) {}

	virtual void lightFuse(const UINT wX, const UINT wY) {}
	virtual void bombExploded(const UINT wX, const UINT wY) {}
	virtual void kegExploded(const UINT wX, const UINT wY) {}

	virtual void beginCombat(const WSTRING& monsterName, const UINT wX, const UINT wY) {}
	virtual void endCombat(const int hpDelta, const int grDelta, const int xpDelta) {}

	virtual void monsterAttack(const WSTRING& monsterName, const UINT wX, const UINT wY, const UINT damage) {}
	virtual void beamDamage(const UINT wX, const UINT wY, const UINT damage) {}
	virtual void tileDamage(const UINT tileType, const UINT wX, const UINT wY, const UINT damage) {}

	virtual void monsterKilled(const CMonster* pMonster) {}

	virtual void swapEquipment(
		const ScriptFlag::EquipmentType type,
		const WSTRING& oldEquipmentName, const WSTRING& newEquipmentName,
		const UINT wX, const UINT wY, const int atkDelta, const int defDelta) {}

	virtual void breakWallWithPickaxe(const UINT wX, const UINT wY) {}
	virtual void openDoorWithPortableOrb(const UINT tileType, const UINT wX, const UINT wY) {}
	virtual void useWarpToken(const UINT wX, const UINT wY, const UINT wDestX, const UINT wDestY) {}
	virtual void useWallWaking(const UINT wX, const UINT wY, const UINT wDestX, const UINT wDestY) {}
	virtual void useAccessory(const WSTRING& accessoryName, const UINT wX, const UINT wY) {}
	virtual void useWeapon(const WSTRING& weaponName, const UINT wX, const UINT wY) {}
	virtual void useShield(const WSTRING& shieldName, const UINT wX, const UINT wY) {}

	virtual void scriptChangeHP(const int delta, const UINT turn) {}
	virtual void scriptChangeATK(const int delta, const UINT turn) {}
	virtual void scriptChangeDEF(const int delta, const UINT turn) {}
	virtual void scriptChangeGR(const int delta, const UINT turn) {}
	virtual void scriptChangeXP(const int delta, const UINT turn) {}
	virtual void scriptChangeShovels(const int delta, const UINT turn) {}
	virtual void scriptChangeYKey(const int delta, const UINT turn) {}
	virtual void scriptChangeGKey(const int delta, const UINT turn) {}
	virtual void scriptChangeBKey(const int delta, const UINT turn) {}
	virtual void scriptChangeSKey(const int delta, const UINT turn) {}

	virtual void scoreCheckpoint(const WSTRING& scoreName, const int score) {}

	virtual void output() {}

	virtual void clear() {}
	
	virtual void writeToFile(const WSTRING& filePath) const {}
};

#endif // #ifndef BASEGAMELOGGER
