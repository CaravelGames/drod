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

//GameEvents.h

#ifndef DB_GAMEEVENTS
#define DB_GAMEEVENTS

#include <BackEndLib/Wchar.h>
#include <string>

#include "RoomData.h"

enum GameEventType {
	GE_EnterRoom,
	GE_CollectItem,
	GE_UseKeyOnDoor,
	GE_UseMoneyOnDoor,
	GE_DigDirt,
	GE_LightFuse,
	GE_ExplosiveExploded,
	GE_Combat,
	GE_MonsterAttack,
	CE_BeamDamage,
	GE_TileDamage,
	GE_MonsterKilled,
	GE_SwapEquipment,
	GE_UseEquipment,
	GE_UsePortableOrbOnDoor,
	GE_UsePickaxeOnWall,
	GE_UseWarpAccesory,
	GE_ScriptedStatChange,
	GE_ScoreCheckpoint
};

//*****************************************************************************
class CDbRoom;
class CGameEvent {
public:
	CGameEvent(GameEventType type);
	~CGameEvent() = default;

	GameEventType type() const { return eventType; }
	virtual WSTRING toText() const = 0;

private:
	GameEventType eventType;
};

//*****************************************************************************
class CEnterRoomEvent : public CGameEvent {
public:
	CEnterRoomEvent(CDbRoom* pRoom);
	~CEnterRoomEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING locationDescription;
};

//*****************************************************************************
class CStatChangeEvent : public CGameEvent {
public:
	CStatChangeEvent(GameEventType type);
	~CStatChangeEvent() = default;

	void addHP(const int amount);
	void addATK(const int amount);
	void addDEF(const int amount);
	void addShovels(const int amount);
	void addGR(const int amount);
	void addXP(const int amount);
	void addYellowKey(const int amount);
	void addGreenKey(const int amount);
	void addBlueKey(const int amount);
	void addSkeletonKey(const int amount);

	virtual WSTRING toText() const override;

protected:
	virtual bool usePlus() const = 0;

	int hp, atk, def, shovels, gr, xp; //amount of stats gained (can be negative with multiplier)
	int yellowKey, greenKey, blueKey, skeletonKey; //amount of keys gained (can be negative with script)
};

//*****************************************************************************
class CCollectedItemEvent : public CStatChangeEvent {
public:
	CCollectedItemEvent();
	~CCollectedItemEvent() = default;

	virtual WSTRING toText() const override;

protected:
	virtual bool usePlus() const { return false; }
};

//*****************************************************************************
class CUseKeyOnDoorEvent : public CGameEvent {
public:
	CUseKeyOnDoorEvent(KeyType type, UINT x, UINT y, bool opened);
	~CUseKeyOnDoorEvent() = default;

	virtual WSTRING toText() const override;

private:
	KeyType type;
	CCoord position;
	bool opened;
};

//*****************************************************************************
class CUseMoneyOnDoorEvent : public CGameEvent {
public:
	CUseMoneyOnDoorEvent(int cost, UINT x, UINT y, bool opened);
	~CUseMoneyOnDoorEvent() = default;

	virtual WSTRING toText() const override;

private:
	int cost;
	CCoord position;
	bool opened;
};

//*****************************************************************************
class CDigDirtEvent : public CGameEvent {
public:
	CDigDirtEvent(const UINT cost, const UINT wX, const UINT wY);
	~CDigDirtEvent() = default;

	virtual WSTRING toText() const override;

private:
	UINT cost;
	CCoord position;
};

//******************************************************************************
class CLightFuseEvent : public CGameEvent {
public:
	CLightFuseEvent(const UINT wX, const UINT wY);
	~CLightFuseEvent() = default;

	virtual WSTRING toText() const override;

private:
	CCoord position;
};

//******************************************************************************
class CExplosiveExplodedEvent : public CGameEvent {
public:
	CExplosiveExplodedEvent(const UINT tileType, const UINT wX, const UINT wY);
	~CExplosiveExplodedEvent() = default;

	virtual WSTRING toText() const override;

private:
	UINT tileType;
	CCoord position;
};

//******************************************************************************
class CCombatEvent : public CGameEvent {
public:
	CCombatEvent(
		const WSTRING& monsterName, const UINT wX, const UINT wY);
	~CCombatEvent() = default;

	void setResults(const int hpDelta, const int grDelta, const int xpDelta);

	virtual WSTRING toText() const override;

private:
	CCoord position;
	WSTRING monsterName;
	int hpDelta;
	int grDelta;
	int xpDelta;
};

//*****************************************************************************
class CMonsterAttackEvent : public CGameEvent {
public:
	CMonsterAttackEvent(const WSTRING& monsterName, UINT wX, UINT wY, UINT damage);
	~CMonsterAttackEvent() = default;

	virtual WSTRING toText() const override;

private:
	CCoord position;
	WSTRING monsterName;
	UINT damage;
};

//*****************************************************************************
class CBeamDamageEvent : public CGameEvent {
public:
	CBeamDamageEvent(const UINT wX, const UINT wY, const UINT damage);
	~CBeamDamageEvent() = default;

	virtual WSTRING toText() const override;

private:
	CCoord position;
	UINT damage;
};

//*****************************************************************************
class CTileDamageEvent : public CGameEvent {
public:
	CTileDamageEvent(const UINT tileType, const UINT wX, const UINT wY, const UINT damage);
	~CTileDamageEvent() = default;

	virtual WSTRING toText() const override;

private:
	UINT tileType;
	CCoord position;
	UINT damage;
};

//*****************************************************************************
class CMonsterKilledEvent : public CGameEvent {
public:
	CMonsterKilledEvent(const WSTRING& monsterName, UINT wX, UINT wY);
	~CMonsterKilledEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING monsterName;
	CCoord position;
};

//*****************************************************************************
class CSwapEquipmentEvent : public CGameEvent {
public:
	CSwapEquipmentEvent(const ScriptFlag::EquipmentType equipType,
		const WSTRING& oldEquipmentName, const WSTRING& newEquipmentName,
		const UINT wX, const UINT wY);
	~CSwapEquipmentEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING getBaseString() const;

	ScriptFlag::EquipmentType equipType;
	WSTRING oldEquipmentName;
	WSTRING newEquipmentName;
	CCoord position;
};

//*****************************************************************************
class CUseEquipmentEvent : public CGameEvent {
public:
	CUseEquipmentEvent(
		const ScriptFlag::EquipmentType equipType,
		const WSTRING& equipmentName,
		const UINT wX, const UINT wY
	);
	~CUseEquipmentEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING getBaseString() const;

	ScriptFlag::EquipmentType equipType;
	WSTRING equipmentName;
	CCoord position;
};

//*****************************************************************************
class CUsePickaxeOnWallEvent : public CGameEvent {
public:
	CUsePickaxeOnWallEvent(const UINT wX, const UINT wY);
	~CUsePickaxeOnWallEvent() = default;

	virtual WSTRING toText() const override;

private:
	CCoord position;
};

//*****************************************************************************
class CUsePortableOrbOnDoorEvent: public CGameEvent {
public:
	CUsePortableOrbOnDoorEvent(const UINT tileType, const UINT wX, const UINT wY);
	~CUsePortableOrbOnDoorEvent() = default;

	virtual WSTRING toText() const override;

private:
	UINT tileType;
	CCoord position;
};

//*****************************************************************************
class CUseWarpAccessoryEvent : public CGameEvent {
public:
	CUseWarpAccessoryEvent(
		const AccessoryType accessoryType,
		const UINT wX, const UINT wY,
		const UINT wDestX, const UINT wDestY
	);
	~CUseWarpAccessoryEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING getBaseString() const;

	AccessoryType accessoryType;
	CCoord start;
	CCoord destination;
};

//*****************************************************************************
class CScriptedStatChangeEvent : public CStatChangeEvent {
public:
	CScriptedStatChangeEvent(UINT turn);
	~CScriptedStatChangeEvent() = default;

	UINT getTurn() { return turn; }
	virtual WSTRING toText() const override;

protected:
	virtual bool usePlus() const { return true; }

private:
	UINT turn;
};

//*****************************************************************************
class CScoreCheckpointEvent : public CGameEvent {
public:
	CScoreCheckpointEvent(const WSTRING& name, int score);
	~CScoreCheckpointEvent() = default;

	virtual WSTRING toText() const override;

private:
	WSTRING scoreCheckpointName;
	int score;
};

#endif //#ifndef DB_GAMEEVENTS
