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

//StandardGameLogger.h

#ifndef STANDARDGAMELOGGER
#define STANDARDGAMELOGGER

#include "BaseGameLogger.h"
#include "GameEvents.h"

#include <BackEndLib/StretchyBuffer.h>

#include <memory>
#include <vector>

//*****************************************************************************
class CStandardGameLogger : public CBaseGameLogger {
public:
	CStandardGameLogger() = default;
	~CStandardGameLogger();

	virtual void enterRoom(CDbRoom* room, const PlayerStats& ps) override;

	virtual void collectHP(const int amount) override;
	virtual void collectATK(const int amount) override;
	virtual void collectDEF(const int amount) override;
	virtual void collectShovels(const int amount) override;
	virtual void collectKey(const KeyType type) override;

	virtual void openDoorWithKey(const KeyType type, const UINT wX, const UINT wY) override;
	virtual void closeDoorWithKey(const KeyType type, const UINT wX, const UINT wY) override;
	virtual void openDoorWithMoney(const int cost, const UINT wX, const UINT wY) override;
	virtual void closeDoorWithMoney(const int cost, const UINT wX, const UINT wY) override;

	virtual void digDirt(const UINT cost, const UINT wX, const UINT wY) override;

	virtual void lightFuse(const UINT wX, const UINT wY) override;
	virtual void bombExploded(const UINT wX, const UINT wY) override;
	virtual void kegExploded(const UINT wX, const UINT wY) override;

	virtual void beginCombat(const WSTRING& monsterName, const UINT wX, const UINT wY) override;
	virtual void endCombat(const int hpDelta, const int grDelta, const int xpDelta) override;

	virtual void monsterAttack(const WSTRING& monsterName, const UINT wX, const UINT wY, const UINT damage) override;
	virtual void beamDamage(const UINT wX, const UINT wY, const UINT damage) override;
	virtual void tileDamage(const UINT tileType, const UINT wX, const UINT wY, const UINT damage) override;

	virtual void monsterKilled(const CMonster* pMonster) override;

	virtual void swapEquipment(
		const ScriptFlag::EquipmentType type,
		const WSTRING& oldEquipmentName, const WSTRING& newEquipmentName,
		const UINT wX, const UINT wY, const int atkDelta, const int defDelta) override;

	virtual void breakWallWithPickaxe(const UINT wX, const UINT wY) override;
	virtual void openDoorWithPortableOrb(const UINT tileType, const UINT wX, const UINT wY) override;
	virtual void useWarpToken(const UINT wX, const UINT wY, const UINT wDestX, const UINT wDestY) override;
	virtual void useWallWaking(const UINT wX, const UINT wY, const UINT wDestX, const UINT wDestY) override;

	virtual void useAccessory(const WSTRING& accessoryName, const UINT wX, const UINT wY) override;
	virtual void useWeapon(const WSTRING& weaponName, const UINT wX, const UINT wY) override;
	virtual void useShield(const WSTRING& shieldName, const UINT wX, const UINT wY) override;

	virtual void scriptChangeHP(const int delta, const UINT turn) override;
	virtual void scriptChangeATK(const int delta, const UINT turn) override;
	virtual void scriptChangeDEF(const int delta, const UINT turn) override;
	virtual void scriptChangeGR(const int delta, const UINT turn) override;
	virtual void scriptChangeXP(const int delta, const UINT turn) override;
	virtual void scriptChangeShovels(const int delta, const UINT turn) override;
	virtual void scriptChangeYKey(const int delta, const UINT turn) override;
	virtual void scriptChangeGKey(const int delta, const UINT turn) override;
	virtual void scriptChangeBKey(const int delta, const UINT turn) override;
	virtual void scriptChangeSKey(const int delta, const UINT turn) override;

	virtual void scoreCheckpoint(const WSTRING& scoreName, const int score) override;

	virtual void output() override;

	virtual void clear() override;

	virtual void writeToFile(const WSTRING& filePath) const override;

protected:
	CCollectedItemEvent* getCollectedItemEvent();
	CScriptedStatChangeEvent* getScriptedStatChangeEvent(const UINT turn);

	std::vector<std::unique_ptr<CGameEvent>> gameEvents;
	CStretchyBuffer outputBuffer;

private:
	//Disallowing copying of logger objects
	CStandardGameLogger(const CStandardGameLogger&) = delete;
	CStandardGameLogger& operator= (const CStandardGameLogger&) = delete;
};

#endif // #ifndef STANDARDGAMELOGGER
