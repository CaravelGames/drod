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

#include "StandardGameLogger.h"

#include "Monster.h"

#include <BackEndLib/Files.h>

#include <cstdio>

//Help trim down long lines
using std::unique_ptr;
using std::make_unique;

//*****************************************************************************
CStandardGameLogger::~CStandardGameLogger()
{
}

//*****************************************************************************
void CStandardGameLogger::enterRoom(CDbRoom* room)
{
	this->gameEvents.push_back(std::make_unique<CEnterRoomEvent>(room));
}

//*****************************************************************************
void CStandardGameLogger::collectHP(const int amount)
{
	CCollectedItemEvent* event = getCollectedItemEvent();
	event->addHP(amount);
}

//*****************************************************************************
void CStandardGameLogger::collectATK(const int amount)
{
	CCollectedItemEvent* event = getCollectedItemEvent();
	event->addATK(amount);
}

//*****************************************************************************
void CStandardGameLogger::collectDEF(const int amount)
{
	CCollectedItemEvent* event = getCollectedItemEvent();
	event->addDEF(amount);
}

//*****************************************************************************
void CStandardGameLogger::collectShovels(const int amount)
{
	CCollectedItemEvent* event = getCollectedItemEvent();
	event->addShovels(amount);
}

//*****************************************************************************
void CStandardGameLogger::collectKey(const KeyType type)
{
	CCollectedItemEvent* event = getCollectedItemEvent();

	switch (type) {
		case YellowKey: event->addYellowKey(1); break;
		case GreenKey: event->addGreenKey(1); break;
		case BlueKey: event->addBlueKey(1); break;
		case SkeletonKey: event->addSkeletonKey(1); break;
		default: ASSERT("Invalid key type");
	}
}

//*****************************************************************************
void CStandardGameLogger::openDoorWithKey(
	const KeyType type, const UINT wX, const UINT wY)
{
	unique_ptr<CUseKeyOnDoorEvent> event =
		make_unique<CUseKeyOnDoorEvent>(type, wX, wY, true);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::closeDoorWithKey(
	const KeyType type, const UINT wX, const UINT wY)
{
	unique_ptr<CUseKeyOnDoorEvent> event =
		make_unique<CUseKeyOnDoorEvent>(type, wX, wY, false);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::openDoorWithMoney(
	const int cost, const UINT wX, const UINT wY)
{
	unique_ptr<CUseMoneyOnDoorEvent> event =
		make_unique<CUseMoneyOnDoorEvent>(cost, wX, wY, true);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::closeDoorWithMoney(
	const int cost, const UINT wX, const UINT wY)
{
	unique_ptr<CUseMoneyOnDoorEvent> event =
		make_unique<CUseMoneyOnDoorEvent>(cost, wX, wY, false);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::digDirt(
	const UINT cost, const UINT wX, const UINT wY)
{
	unique_ptr<CDigDirtEvent> event =
		make_unique<CDigDirtEvent>(cost, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::lightFuse(const UINT wX, const UINT wY)
{
	unique_ptr<CLightFuseEvent> event = make_unique<CLightFuseEvent>(wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::bombExploded(const UINT wX, const UINT wY)
{
	unique_ptr<CExplosiveExplodedEvent> event =
		make_unique<CExplosiveExplodedEvent>(T_BOMB, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::kegExploded(const UINT wX, const UINT wY)
{
	unique_ptr<CExplosiveExplodedEvent> event =
		make_unique<CExplosiveExplodedEvent>(T_POWDER_KEG, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::beginCombat(
	const WSTRING& monsterName, const UINT wX, const UINT wY)
{
	unique_ptr<CCombatEvent> event = make_unique<CCombatEvent>(monsterName, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::endCombat(const int hpDelta, const int grDelta, const int xpDelta)
{
	CCombatEvent* combatEvent = nullptr;
	for (std::vector<unique_ptr<CGameEvent>>::reverse_iterator it = this->gameEvents.rbegin();
		it != this->gameEvents.rend(); ++it) {
		if (it->get()->type() == GE_Combat) {
			combatEvent = DYN_CAST(CCombatEvent*, CGameEvent*, it->get());
			break;
		}
	}

	if (!combatEvent) {
		return;
	}

	combatEvent->setResults(hpDelta, grDelta, xpDelta);
}

//*****************************************************************************
void CStandardGameLogger::monsterAttack(
	const WSTRING& monsterName, const UINT wX, const UINT wY, const UINT damage)
{
	unique_ptr<CMonsterAttackEvent> event =
		make_unique<CMonsterAttackEvent>(monsterName, wX, wY, damage);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::beamDamage(const UINT wX, const UINT wY, const UINT damage)
{
	unique_ptr<CBeamDamageEvent> event = 
		make_unique<CBeamDamageEvent>(wX, wY, damage);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::tileDamage(const UINT tileType, const UINT wX, const UINT wY, const UINT damage)
{
	unique_ptr<CTileDamageEvent> event =
		make_unique<CTileDamageEvent>(tileType, wX, wY, damage);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::monsterKilled(const CMonster* pMonster)
{
	unique_ptr<CMonsterKilledEvent> event =
		make_unique<CMonsterKilledEvent>(pMonster->GetName(), pMonster->wX, pMonster->wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::swapEquipment(
	const ScriptFlag::EquipmentType type,
	const WSTRING& oldEquipmentName, const WSTRING& newEquipmentName,
	const UINT wX, const UINT wY
)
{
	unique_ptr<CSwapEquipmentEvent> event = make_unique<CSwapEquipmentEvent>(
		type, oldEquipmentName, newEquipmentName, wX, wY
	);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::breakWallWithPickaxe(const UINT wX, const UINT wY)
{
	unique_ptr<CUsePickaxeOnWallEvent> event =
		make_unique<CUsePickaxeOnWallEvent>(wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::openDoorWithPortableOrb(const UINT tileType, const UINT wX, const UINT wY)
{
	unique_ptr<CUsePortableOrbOnDoorEvent> event =
		make_unique<CUsePortableOrbOnDoorEvent>(tileType, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::useWarpToken(
	const UINT wX, const UINT wY,
	const UINT wDestX, const UINT wDestY
)
{
	unique_ptr<CUseWarpAccessoryEvent> event =
		make_unique<CUseWarpAccessoryEvent>(AccessoryType::WarpToken, wX, wY, wDestX, wDestY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::useWallWaking(
	const UINT wX, const UINT wY,
	const UINT wDestX, const UINT wDestY
)
{
	unique_ptr<CUseWarpAccessoryEvent> event =
		make_unique<CUseWarpAccessoryEvent>(AccessoryType::WallWalking, wX, wY, wDestX, wDestY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::useAccessory(
	const WSTRING& accessoryName,
	const UINT wX, const UINT wY
)
{
	unique_ptr<CUseEquipmentEvent> event =
		make_unique<CUseEquipmentEvent>(ScriptFlag::Accessory, accessoryName, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::useWeapon(
	const WSTRING& weaponName,
	const UINT wX, const UINT wY
)
{
	unique_ptr<CUseEquipmentEvent> event =
		make_unique<CUseEquipmentEvent>(ScriptFlag::Accessory, weaponName, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::useShield(
	const WSTRING& shieldName,
	const UINT wX, const UINT wY
)
{
	unique_ptr<CUseEquipmentEvent> event =
		make_unique<CUseEquipmentEvent>(ScriptFlag::Accessory, shieldName, wX, wY);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeHP(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addHP(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeATK(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addATK(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeDEF(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addDEF(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeGR(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addGR(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeXP(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addXP(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeShovels(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addShovels(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeYKey(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addYellowKey(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeGKey(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addGreenKey(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeBKey(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addBlueKey(delta);
}

//*****************************************************************************
void CStandardGameLogger::scriptChangeSKey(const int delta, const UINT turn)
{
	CScriptedStatChangeEvent* event = getScriptedStatChangeEvent(turn);
	event->addSkeletonKey(delta);
}

//*****************************************************************************
void CStandardGameLogger::scoreCheckpoint(const WSTRING& scoreName, const int score)
{
	unique_ptr<CScoreCheckpointEvent> event =
		make_unique<CScoreCheckpointEvent>(scoreName, score);
	this->gameEvents.push_back(std::move(event));
}

//*****************************************************************************
void CStandardGameLogger::output()
//Write out the logged game actions as text to internal buffer
{
	if (this->gameEvents.size() < 2) {
		return;
	}

	for (auto& event : this->gameEvents) {
		std::string str = UnicodeToUTF8(event->toText());
		str += NEWLINE;
		this->outputBuffer += str.c_str();
	}
}

//*****************************************************************************
void CStandardGameLogger::clear()
{
	this->gameEvents.clear();
}

//*****************************************************************************
void CStandardGameLogger::writeToFile(const WSTRING& filePath) const
{
	CFiles::WriteBufferToFile(filePath.c_str(), this->outputBuffer);
}

//*****************************************************************************
CCollectedItemEvent* CStandardGameLogger::getCollectedItemEvent()
//Returns: A pointer to the CCollectedItem event that is at the end of gameEvents
//If the last event isn't that kind of event, create one first
{
	if (this->gameEvents.empty() || this->gameEvents.back()->type() != GE_CollectItem) {
		this->gameEvents.push_back(make_unique<CCollectedItemEvent>());
	}

	CGameEvent* lastEvent = this->gameEvents.back().get();
	CCollectedItemEvent* lastCollectionEvent =
		DYN_CAST(CCollectedItemEvent*, CGameEvent*, lastEvent);

	return lastCollectionEvent;
}

//*****************************************************************************
CScriptedStatChangeEvent* CStandardGameLogger::getScriptedStatChangeEvent(const UINT turn)
//Returns: A pointer to the CScriptedStatChangeEvent event that is at the end of gameEvents
//If the last event isn't that kind of event, create one first. Additionally,
//we will create a new event if we're no longer on the same turn as the last event.
{
	if (this->gameEvents.empty() || this->gameEvents.back()->type() != GE_ScriptedStatChange) {
		this->gameEvents.push_back(make_unique<CScriptedStatChangeEvent>(turn));
	}

	CGameEvent* lastEvent = this->gameEvents.back().get();
	CScriptedStatChangeEvent* lastScriptedStatChangeEvent =
		DYN_CAST(CScriptedStatChangeEvent*, CGameEvent*, lastEvent);
	if (lastScriptedStatChangeEvent->getTurn() == turn) {
		return lastScriptedStatChangeEvent;
	}

	this->gameEvents.push_back(make_unique<CScriptedStatChangeEvent>(turn));
	lastEvent = this->gameEvents.back().get();
	lastScriptedStatChangeEvent =
		DYN_CAST(CScriptedStatChangeEvent*, CGameEvent*, lastEvent);

	return lastScriptedStatChangeEvent;
}
