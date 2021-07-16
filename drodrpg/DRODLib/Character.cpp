// $Id: Character.cpp 10219 2012-05-21 13:18:56Z mrimer $

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

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include "Character.h"
#include "BuildUtil.h"
#include "Combat.h"
#include "Db.h"
#include "DbHolds.h"
#include "EvilEye.h"
#include "Phoenix.h"
#include "Swordsman.h"
#include "../Texts/MIDs.h"

#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Files.h>

//*****************************************************************************
#define NO_LABEL (-1)

#define NO_OVERRIDE (UINT(-9999))

const UINT MAX_ANSWERS = 9;

//Literals used to query and store values for the NPC in the packed vars object.
#define commandStr "Commands"
#define idStr "id"
#define numCommandsStr "NumCommands"
#define scriptIDstr "ScriptID"
#define startLineStr "StartLine"
#define visibleStr "visible"
#define equipTypeStr "equipType"

#define EachAttackStr "EachAttack"
#define EachDefendStr "EachDefend"
#define EachUseStr "EachUse"
#define EachVictoryStr "EachVictory"

#define CustomNameStr "Name"

#define ColorStr "Color"
#define ParamXStr "XParam"
#define ParamYStr "YParam"
#define ParamWStr "WParam"
#define ParamHStr "HParam"
#define ParamFStr "FParam"

#define MonsterHPMultStr "MonsterHPMultiplier"
#define MonsterATKMultStr "MonsterATKMultiplier"
#define MonsterDEFMultStr "MonsterDEFMultiplier"
#define MonsterGRMultStr "MonsterGRMultiplier"
#define MonsterXPMultStr "MonsterXPMultiplier"

#define ItemMultStr "ItemMultiplier"
#define ItemHPMultStr "ItemHPMultiplier"
#define ItemATKMultStr "ItemATKMultiplier"
#define ItemDEFMultStr "ItemDEFMultiplier"
#define ItemGRMultStr "ItemGRMultiplier"

#define SpawnTypeStr "SpawnType"

#define TurnDelayStr "TurnDelay"
#define XRelStr "XRel"
#define YRelStr "YRel"
#define MovingRelativeStr "MovingRel"
#define ExitRoomOStr "ExitO"
#define VulnerableStr "Vulnerable"
#define MissionCriticalStr "MissCrit"
#define SafeToPlayerStr "SafeToPlayer"
#define SwordSafeToPlayerStr "SwSafeToPlayer"
#define DefeatedStr "Defeated"
#define ShowStatChangesStr "SStatChanges"
#define GhostImageStr "GhostImage"
#define SwordStr "Sword"
#define RestartScriptOnEntranceStr "RestartOnEntrance"
#define GlobalStr "Global"
#define ExecuteScriptOnCombatStr "ESOC"

#define AttackAdjacentStr "AttackAdj"
#define AttackInFrontStr "AttackInFront"
#define AttackInFrontWhenBackIsTurnedStr "AttackInFrontWhenBackIsTurned"
#define FaceTargetStr "FaceTarget"
#define RayGunStr "RayGun"
#define RayBlockingStr "RayBlocking"
#define SurprisedFromBehindStr "SurprisedFromBehind"
#define FaceAwayFromTargetStr "FaceAwayFromTarget"
#define GoblinWeaknessStr "GoblinWeakness"
#define SerpentWeaknessStr "SerpentWeakness"
#define MetalStr "Metal"
#define LuckyGRStr "Lucky"
#define LuckyXPStr "LuckyXP"
#define BriarStr "Briar"
#define NoEnemyDefenseStr "NoEnemyDEF"
#define AttackFirstStr "AttackFirst"
#define AttackLastStr "AttackLast"
#define MovementIQStr "MoveIQ"
#define DropTrapdoorsStr "DropTrapdoors"
#define MoveIntoSwordsStr "MoveIntoSwords"
#define PushObjectsStr "PushObjects"
#define SpawnEggsStr "SpawnEggs"
#define MovementTypeStr "MovementType"

#define SKIP_WHITESPACE(str, index) while (iswspace(str[index])) ++index

//*****************************************************************************
inline bool isVarCharValid(WCHAR wc)
{
	return iswalnum(wc) || wc == W_t('_');
}

void LogParseError(const WCHAR* pwStr, const char* message)
{
	CFiles f;
	string str = UnicodeToUTF8(pwStr);
	str += ": ";
	str += message;
	f.AppendErrorLog(str.c_str());
}

//*****************************************************************************
inline bool multWithClamp(int& val, const int operand)
//Multiplies two integers, ensuring the product doesn't overflow.
//
//Returns: false if actual result can't be given (i.e. value overflowed), otherwise true
{
	const double newVal = (double)val * operand;
	if (newVal > INT_MAX)
	{
		val = INT_MAX;
		return false;
	}
	if (newVal < INT_MIN)
	{
		val = INT_MIN;
		return false;
	}
	val *= operand;
	return true;
}

//*****************************************************************************
inline bool bCommandHasData(const UINT eCommand)
//Returns: whether this script command has a data record attached to it
{
	switch (eCommand)
	{
		case CCharacterCommand::CC_AmbientSound:
		case CCharacterCommand::CC_AmbientSoundAt:
		case CCharacterCommand::CC_PlayVideo:
		case CCharacterCommand::CC_SetMusic:
			return true;
		default:
			return false;
	}
}

//
//Public methods.
//

//*****************************************************************************
CCharacter::CCharacter(
//Constructor.
//
//Params:
	CCurrentGame *pSetCurrentGame)   //(in)   If NULL (default) then
									//    class can only be used for
									//    accessing data, and not
									//    for game processing.
	: CPlayerDouble(M_CHARACTER, pSetCurrentGame,
			9999)   //put last in process sequence so all cue events will have
					  //occurred and can be detected by the time Process() is called
	, pCustomChar(NULL)
	, dwScriptID(0)
	, wIdentity(M_NONE)
	, wLogicalIdentity(M_NONE)
	, bVisible(false)
	, bScriptDone(false), bReplaced(false), bGlobal(false)
	, bYesNoQuestion(false)
	, bPlayerTouchedMe(false)
	, bAttacked(false)
	, equipType(ScriptFlag::NotEquipment)
	, movementIQ(SmartDiagonalOnly)

	, wCurrentCommandIndex(0)
	, wTurnDelay(0)
	, wLastSO(NO_ORIENTATION), wSO(NO_ORIENTATION)
	, wXRel(0), wYRel(0)
	, bMovingRelative(false)
	, wExitingRoomO(NO_ORIENTATION)

	, bVulnerable(true)
	, bMissionCritical(false)
	, bSafeToPlayer(false)
	, bSwordSafeToPlayer(false)
	, bDefeated(false)
	, bShowStatChanges(true)
	, bGhostImage(false)
	, bRestartScriptOnRoomEntrance(false)
	, bExecuteScriptOnCombat(true)

	, bAttackAdjacent(false)
	, bAttackInFront(false)
	, bAttackInFrontWhenBackIsTurned(false)
	, bFaceAwayFromTarget(false)
	, bFaceTarget(false)
	, bHasRayGun(false)
	, bHasRayBlocking(false)
	, bSurprisedFromBehind(false)
	, bGoblinWeakness(false)
	, bSerpentWeakness(false)
	, bMetal(false), bLuckyGR(false), bLuckyXP(false), bBriar(false), bNoEnemyDEF(false)
	, bAttackFirst(false), bAttackLast(false)
	, bDropTrapdoors(false), bMoveIntoSwords(false), bPushObjects(false), bSpawnEggs(false)

	, wJumpLabel(0)
	, bWaitingForCueEvent(false)
	, bIfBlock(false)
	, eachAttackLabelIndex(NO_LABEL), eachDefendLabelIndex(NO_LABEL), eachUseLabelIndex(NO_LABEL)
	, eachVictoryLabelIndex(NO_LABEL)
	, wLastSpeechLineNumber(0)

	, color(0), sword(NPC_DEFAULT_SWORD)
	, paramX(NO_OVERRIDE), paramY(NO_OVERRIDE), paramW(NO_OVERRIDE), paramH(NO_OVERRIDE), paramF(NO_OVERRIDE)
	, monsterHPmult(100), monsterATKmult(100), monsterDEFmult(100), monsterGRmult(100), monsterXPmult(100)
	, itemMult(100), itemHPmult(100), itemATKmult(100), itemDEFmult(100), itemGRmult(100)
	, wSpawnType(-1)
{
}

//*****************************************************************************
void CCharacter::ChangeHold(
//Call this when a character is being moved from an old hold to a new hold.
	const CDbHold* pSrcHold, //may be NULL.  This indicates character is copied within the same hold.
	CDbHold* pDestHold,
	CImportInfo& info, //(in/out) media copy info
	const bool bGetNewScriptID) //[default=true]
{
	ASSERT(pDestHold);

	if (bGetNewScriptID)
		this->dwScriptID = pDestHold->GetNewScriptID();

	ChangeHoldForCommands(this->commands, pSrcHold, pDestHold, info, true);

	SyncCustomCharacterData(pSrcHold, pDestHold, info);
}

//*****************************************************************************
void CCharacter::SyncCustomCharacterData(
	const CDbHold* pSrcHold,
	CDbHold* pDestHold,
	CImportInfo& info)
{
	SyncCustomCharacterData(this->wLogicalIdentity, pSrcHold, pDestHold, info);
}

//Returns: whether something was changed
void CCharacter::SyncCustomCharacterData(
	UINT& wLogicalIdentity,
	const CDbHold* pSrcHold,
	CDbHold* pDestHold,
	CImportInfo& info)
{
	if (pSrcHold)
	{
		const HoldCharacter *pCustomChar = pSrcHold->GetCharacterConst(wLogicalIdentity);
		if (pCustomChar)
		{
			//Match on character name.
			UINT charID = pDestHold->GetCharacterID(pCustomChar->charNameText.c_str());
			if (!charID)
			{
				//Copy custom character data to new hold.
				charID = pDestHold->AddCharacter(pCustomChar->charNameText.c_str());
				ASSERT(charID);
				HoldCharacter *pDestCustomChar = pDestHold->GetCharacter(charID);
				ASSERT(pDestCustomChar);
				pDestCustomChar->animationSpeed = pCustomChar->animationSpeed;
				pDestCustomChar->wType = pCustomChar->wType;
				pDestCustomChar->dwDataID_Avatar = pCustomChar->dwDataID_Avatar;
				pDestCustomChar->dwDataID_Tiles = pCustomChar->dwDataID_Tiles;
				pDestCustomChar->ExtraVars = pCustomChar->ExtraVars;

				pSrcHold->CopyCustomCharacterData(*pDestCustomChar, pDestHold, info);
			}
			wLogicalIdentity = charID;
		}
	}
}

void SyncEntranceID(CImportInfo& info, UINT& entranceID)
{
	if (entranceID && entranceID != (UINT)EXIT_PRIOR_LOCATION) {
		PrimaryKeyMap::const_iterator newID = info.EntranceIDMap.find(entranceID);
		if (newID != info.EntranceIDMap.end()) {
			entranceID = newID->second;
		} else {
			entranceID = 0; //end hold
		}
	}
}

void CCharacter::ChangeHoldForCommands(
   COMMAND_VECTOR& commands,
   const CDbHold* pOldHold, CDbHold* pNewHold,
   CImportInfo& info,
   bool bUpdateSpeech)
{
	const bool bDifferentHold = pOldHold && pOldHold->dwHoldID != pNewHold->dwHoldID;

	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		CCharacterCommand& c = commands[wIndex];
		if (bDifferentHold)
		{
			//Merge script vars and IDs from a different source hold.
			switch (c.command)
			{
				case CCharacterCommand::CC_WaitForVar:
				case CCharacterCommand::CC_VarSet:
				{
					//Update var refs.
					if (c.x >= (UINT)ScriptVars::FirstPredefinedVar)
						break; //predefined var IDs remain the same

					const WCHAR *pVarName = pOldHold->GetVarName(c.x);
					UINT uVarID = pNewHold->GetVarID(pVarName);
					if (!uVarID && pVarName)
					{
						//A var with this (valid) name doesn't exist in the
						//destination hold -- add one.
						uVarID = pNewHold->AddVar(pVarName);
					}
					//Update the var ID to match the ID of the var with this
					//name in the destination hold.
					c.x = uVarID;
				}
				break;
				case CCharacterCommand::CC_AmbientSound:
				case CCharacterCommand::CC_AmbientSoundAt:
				case CCharacterCommand::CC_PlayVideo:
					//Make a copy of the media object in the new hold.
					CDbData::CopyObject(info, c.w, pNewHold->dwHoldID);
				break;
				case CCharacterCommand::CC_SetMusic:
					CDbData::CopyObject(info, c.w, pNewHold->dwHoldID);
				break;
				case CCharacterCommand::CC_GenerateEntity:
					SyncCustomCharacterData(c.h, pOldHold, pNewHold, info);
					break;

				case CCharacterCommand::CC_LevelEntrance:
					SyncEntranceID(info, c.x);
				break;

				case CCharacterCommand::CC_Speech:
					if (c.pSpeech)
						SyncCustomCharacterData(c.pSpeech->wCharacter, pOldHold, pNewHold, info);
				break;

				default: break;
			}
		}

		//Point all data objects to the destination hold.
		if (c.pSpeech && bUpdateSpeech)
		{
			CDbDatum *pSound = (CDbDatum*)c.pSpeech->GetSound();
			if (pSound)
			{
				pSound->dwHoldID = pNewHold->dwHoldID;
				pSound->Update();
			}
		}
	}
}

//*****************************************************************************
WSTRING CCharacter::getPredefinedVar(const UINT varIndex) const
{
	WSTRING wstr;
	if (ScriptVars::IsStringVar(ScriptVars::Predefined(varIndex))) {
		wstr = getPredefinedVarString(varIndex);
	}
	else {
		WCHAR wIntText[20];
		const UINT val = getPredefinedVarInt(varIndex);
		wstr = _itoW(int(val), wIntText, 10);
	}
	return wstr;
}

//*****************************************************************************
UINT CCharacter::getPredefinedVarInt(const UINT varIndex) const
//Returns: the value of the predefined var with this relative index
{
	ASSERT(this->pCurrentGame);
	ASSERT(varIndex >= (UINT)ScriptVars::FirstPredefinedVar);
	switch (varIndex)
	{
		case (UINT)ScriptVars::P_MONSTER_HP:
			return this->HP;
		case (UINT)ScriptVars::P_MONSTER_ATK:
			return this->ATK;
		case (UINT)ScriptVars::P_MONSTER_DEF:
			return this->DEF;
		case (UINT)ScriptVars::P_MONSTER_GOLD:
			return this->GOLD;
		case (UINT)ScriptVars::P_MONSTER_XP:
			return this->XP;
		case (UINT)ScriptVars::P_MONSTER_COLOR:
			return this->color;
		case (UINT)ScriptVars::P_MONSTER_SWORD:
			return this->sword;

		//Room position.
		case (UINT)ScriptVars::P_MONSTER_X:
			return this->wX;
		case (UINT)ScriptVars::P_MONSTER_Y:
			return this->wY;
		case (UINT)ScriptVars::P_MONSTER_O:
			return this->wO;

		//Script parameter overrides.
		case (UINT)ScriptVars::P_SCRIPT_X:
			return this->paramX;
		case (UINT)ScriptVars::P_SCRIPT_Y:
			return this->paramY;
		case (UINT)ScriptVars::P_SCRIPT_W:
			return this->paramW;
		case (UINT)ScriptVars::P_SCRIPT_H:
			return this->paramH;
		case (UINT)ScriptVars::P_SCRIPT_F:
			return this->paramF;

		//Local statistic modifiers
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_HP_MULT:
			return this->monsterHPmult;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_ATK_MULT:
			return this->monsterATKmult;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_DEF_MULT:
			return this->monsterDEFmult;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_GOLD_MULT:
			return this->monsterGRmult;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_XP_MULT:
			return this->monsterXPmult;

		case (UINT)ScriptVars::P_SCRIPT_ITEM_MULT:
			return this->itemMult;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_HP_MULT:
			return this->itemHPmult;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_ATK_MULT:
			return this->itemATKmult;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_DEF_MULT:
			return this->itemDEFmult;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_GR_MULT:
			return this->itemGRmult;

		//Spawn type
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_SPAWN:
			return this->wSpawnType;

		//Hidden global values
		case (UINT)ScriptVars::P_TOTALTIME:
			return 0;

		default:
			return this->pCurrentGame->getVar(ScriptVars::Predefined(varIndex));
	}
}

//*****************************************************************************
WSTRING CCharacter::getPredefinedVarString(const UINT varIndex) const
//Returns: the value of the predefined var with this relative index
{
	ASSERT(this->pCurrentGame);
	ASSERT(varIndex >= (UINT)ScriptVars::FirstPredefinedVar);
	switch (varIndex)
	{
		case (UINT)ScriptVars::P_MONSTER_NAME:
			return this->customName;
		default:
			ASSERT(!"getPredefinedStringVar val not supported");
			return WSTRING();
	}
}

//*****************************************************************************
bool CCharacter::setPredefinedVarInt(const UINT varIndex, const UINT val, CCueEvents& CueEvents)
//Sets the value of the predefined var with this relative index to the specified value
//Returns: false if command cannot be allowed to execute (e.g., killing player on turn 0), otherwise true
{
	ASSERT(varIndex >= (UINT)ScriptVars::FirstPredefinedVar);
	switch (varIndex)
	{
		case (UINT)ScriptVars::P_MONSTER_HP:
			if ((int)val > 0) //guard against negative HP (only allow up to max int)
				this->HP = val;
			else
				this->HP = 0;

			//When HP is set to a positive value, the defeated flag is reset so the NPC-monster can fight again.
			if (this->HP > 0)
				this->bDefeated = false;
		break;
		case (UINT)ScriptVars::P_MONSTER_ATK:
			this->ATK = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_DEF:
			this->DEF = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_GOLD:
			this->GOLD = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_XP:
			this->XP = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_COLOR:
			this->color = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_SWORD:
			this->sword = val;
		break;

		//Room position.
		case (UINT)ScriptVars::P_PLAYER_X:
			const_cast<CCurrentGame*>(this->pCurrentGame)->TeleportPlayer(val, this->pCurrentGame->pPlayer->wY, CueEvents);
		break;
		case (UINT)ScriptVars::P_PLAYER_Y:
			const_cast<CCurrentGame*>(this->pCurrentGame)->TeleportPlayer(this->pCurrentGame->pPlayer->wX, val, CueEvents);
		break;
		case (UINT)ScriptVars::P_PLAYER_O:
			if (IsValidOrientation(val) && val != NO_ORIENTATION)
				this->pCurrentGame->pPlayer->wO = val;
		break;
		case (UINT)ScriptVars::P_MONSTER_X:
		{
			//Ensure square is valid and available.
			const CDbRoom& room = *(this->pCurrentGame->pRoom);
			if (room.IsValidColRow(val, this->wY) &&
					(!IsVisible() || (!room.GetMonsterAtSquare(val, this->wY) &&
					!this->pCurrentGame->IsPlayerAt(val, this->wY))))
			{
				this->wPrevX = this->wX;
				TeleportCharacter(val, this->wY, CueEvents);
			}
		}
		break;
		case (UINT)ScriptVars::P_MONSTER_Y:
		{
			//Ensure square is valid and available.
			const CDbRoom& room = *(this->pCurrentGame->pRoom);
			if (room.IsValidColRow(this->wX, val) &&
					(!IsVisible() || (!room.GetMonsterAtSquare(this->wX, val) &&
					!this->pCurrentGame->IsPlayerAt(this->wX, val))))
			{
				this->wPrevY = this->wY;
				TeleportCharacter(this->wX, val, CueEvents);
			}
		}
		break;
		case (UINT)ScriptVars::P_MONSTER_O:
			if (IsValidOrientation(val) && val != NO_ORIENTATION)
				this->wO = val;
		break;

		//Script parameter overrides.
		case (UINT)ScriptVars::P_SCRIPT_X:
			this->paramX = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_Y:
			this->paramY = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_W:
			this->paramW = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_H:
			this->paramH = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_F:
			this->paramF = val;
		break;

		//Local statistic modifiers
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_HP_MULT:
			this->monsterHPmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_ATK_MULT:
			this->monsterATKmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_DEF_MULT:
			this->monsterDEFmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_GOLD_MULT:
			this->monsterGRmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_XP_MULT:
			this->monsterXPmult = val;
		break;

		case (UINT)ScriptVars::P_SCRIPT_ITEM_MULT:
			this->itemMult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_HP_MULT:
			this->itemHPmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_ATK_MULT:
			this->itemATKmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_DEF_MULT:
			this->itemDEFmult = val;
		break;
		case (UINT)ScriptVars::P_SCRIPT_ITEM_GR_MULT:
			this->itemGRmult = val;
		break;

		//Spawn type
		case (UINT)ScriptVars::P_SCRIPT_MONSTER_SPAWN:
			this->wSpawnType = val;
		break;

		//Combat enemy stats.
		case (UINT)ScriptVars::P_ENEMY_HP:
		case (UINT)ScriptVars::P_ENEMY_ATK:
		case (UINT)ScriptVars::P_ENEMY_DEF:
		case (UINT)ScriptVars::P_ENEMY_GOLD:
		case (UINT)ScriptVars::P_ENEMY_XP:
		{
			//Do nothing if there is no current combat enemy.
			if (!this->pCurrentGame->InCombat())
				break;
			CCombat *pCombat = this->pCurrentGame->pCombat;
			ASSERT(pCombat);
			CMonster *pMonster = pCombat->pMonster;
			ASSERT(pMonster);
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_ENEMY_HP: pMonster->HP = (int)val > 0 ? val : 0; break;
				case (UINT)ScriptVars::P_ENEMY_ATK: pMonster->ATK = (int)val > 0 ? val : 0; break;
				case (UINT)ScriptVars::P_ENEMY_DEF: pMonster->DEF = (int)val > 0 ? val : 0; break;
				case (UINT)ScriptVars::P_ENEMY_GOLD: pMonster->GOLD = val; break;
				case (UINT)ScriptVars::P_ENEMY_XP: pMonster->XP = val; break;
			}
		}
		break;

		//Player equipment values.
		case (UINT)ScriptVars::P_WEAPON_ATK:
		case (UINT)ScriptVars::P_WEAPON_DEF:
		case (UINT)ScriptVars::P_WEAPON_GR:
		case (UINT)ScriptVars::P_ARMOR_ATK:
		case (UINT)ScriptVars::P_ARMOR_DEF:
		case (UINT)ScriptVars::P_ARMOR_GR:
		case (UINT)ScriptVars::P_ACCESSORY_ATK:
		case (UINT)ScriptVars::P_ACCESSORY_DEF:
		case (UINT)ScriptVars::P_ACCESSORY_GR:
		{
			UINT type = 0;
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_WEAPON_ATK: case (UINT)ScriptVars::P_WEAPON_DEF:
				case (UINT)ScriptVars::P_WEAPON_GR: type = ScriptFlag::Weapon; break;
				case (UINT)ScriptVars::P_ARMOR_ATK:	case (UINT)ScriptVars::P_ARMOR_DEF:
				case (UINT)ScriptVars::P_ARMOR_GR: type = ScriptFlag::Armor; break;
				case (UINT)ScriptVars::P_ACCESSORY_ATK: case (UINT)ScriptVars::P_ACCESSORY_DEF:
				case (UINT)ScriptVars::P_ACCESSORY_GR: type = ScriptFlag::Accessory; break;
			}
			CCharacter* pCharacter = this->pCurrentGame->getCustomEquipment(type);
			if (!pCharacter)
				break; //only custom equipment may have its values modified
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_WEAPON_ATK: case (UINT)ScriptVars::P_ARMOR_ATK:
				case (UINT)ScriptVars::P_ACCESSORY_ATK: pCharacter->ATK = val; break;
				case (UINT)ScriptVars::P_WEAPON_DEF: case (UINT)ScriptVars::P_ARMOR_DEF:
				case (UINT)ScriptVars::P_ACCESSORY_DEF: pCharacter->DEF = val; break;
				case (UINT)ScriptVars::P_WEAPON_GR: case (UINT)ScriptVars::P_ARMOR_GR:
				case (UINT)ScriptVars::P_ACCESSORY_GR: pCharacter->GOLD = val; break;
			}
		}
		break;

		//Stat modifications that may display the change as a special effect.
		default:
		{
			CSwordsman &p = *(const_cast<CCurrentGame*>(this->pCurrentGame)->pPlayer);
			PlayerStats& st = p.st;

			//Bounds checks
			UINT newVal = val;
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_YKEY:
				case (UINT)ScriptVars::P_GKEY:
				case (UINT)ScriptVars::P_BKEY:
				case (UINT)ScriptVars::P_SKEY:
					if (int(val) < 0)
						newVal = 0;
				break;
				default: break;
			}

			if (this->bShowStatChanges)
			{
				const UINT oldVal = st.getVar(ScriptVars::Predefined(varIndex));
				int delta = int(newVal) - int(oldVal);
				CombatEffectType type = CET_NODAMAGE;
				switch (varIndex)
				{
					case (UINT)ScriptVars::P_HP:
						if (int(newVal) <= 0 && this->pCurrentGame->wTurnNo == 0) //forbid killing player on turn 0 (avoids respawn loop)
							return false;
						type = delta < 0 ? CET_HARM : CET_HEAL;
						if (delta < 0)
							delta = -delta;
					break;
					case (UINT)ScriptVars::P_ATK:
						type = CET_ATK;
					break;
					case (UINT)ScriptVars::P_DEF:
						type = CET_DEF;
					break;
					case (UINT)ScriptVars::P_GOLD:
						type = CET_GOLD;
					break;
					case (UINT)ScriptVars::P_XP:
						type = CET_XP;
					break;
					case (UINT)ScriptVars::P_YKEY:
						type = CET_YKEY;
					break;
					case (UINT)ScriptVars::P_GKEY:
						type = CET_GKEY;
					break;
					case (UINT)ScriptVars::P_BKEY:
						type = CET_BKEY;
					break;
					case (UINT)ScriptVars::P_SKEY:
						type = CET_SKEY;
					break;
					case (UINT)ScriptVars::P_MONSTER_ATK_MULT:
					case (UINT)ScriptVars::P_MONSTER_DEF_MULT:
					case (UINT)ScriptVars::P_MONSTER_HP_MULT:
					case (UINT)ScriptVars::P_MONSTER_GOLD_MULT:
					case (UINT)ScriptVars::P_MONSTER_XP_MULT:
					case (UINT)ScriptVars::P_ITEM_MULT:
					case (UINT)ScriptVars::P_ITEM_HP_MULT:
					case (UINT)ScriptVars::P_ITEM_ATK_MULT:
					case (UINT)ScriptVars::P_ITEM_DEF_MULT:
					case (UINT)ScriptVars::P_ITEM_GR_MULT:
					case (UINT)ScriptVars::P_HOTTILE:
					case (UINT)ScriptVars::P_EXPLOSION:
					case (UINT)ScriptVars::P_MUD_SPAWN:
					case (UINT)ScriptVars::P_TAR_SPAWN:
					case (UINT)ScriptVars::P_GEL_SPAWN:
					case (UINT)ScriptVars::P_QUEEN_SPAWN:
						//display nothing
					break;

					case (UINT)ScriptVars::P_SWORD:
						if (!const_cast<CCurrentGame*>(this->pCurrentGame)->IsEquipmentValid(newVal, ScriptFlag::Weapon))
							return true;
						break;

					case (UINT)ScriptVars::P_SHIELD:
						if (!const_cast<CCurrentGame*>(this->pCurrentGame)->IsEquipmentValid(newVal, ScriptFlag::Armor))
							return true;
						break;

					case (UINT)ScriptVars::P_ACCESSORY:
						if (!const_cast<CCurrentGame*>(this->pCurrentGame)->IsEquipmentValid(newVal, ScriptFlag::Accessory))
							return true;
						break;

					case (UINT)ScriptVars::P_SPEED:
					case (UINT)ScriptVars::P_TOTALMOVES:
					case (UINT)ScriptVars::P_TOTALTIME:
						//cannot alter
					break;

					default:
						ASSERT(!"Unsupported var display");
					break;
				}
				if (type != CET_NODAMAGE && delta != 0)
				{
					//If player is in room, show stat changes where player is.
					//Otherwise, show them where this NPC is.
					CEntity *pEntity;
					if (p.IsInRoom())
						pEntity = &p;
					else pEntity = this;

					CueEvents.Add(CID_EntityAffected, new CCombatEffect(pEntity, type, delta), true);
				}
			}

			//Check for special things that need to happen as a result of altering
			//these vars.
			CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
			switch (varIndex)
			{
				case (UINT)ScriptVars::P_HP:
					if (int(newVal) <= 0)
						CueEvents.Add(CID_MonsterKilledPlayer); //player died
					st.setVar(ScriptVars::Predefined(varIndex), newVal);
				break;

				default:
					pGame->ProcessCommandSetVar(CueEvents, varIndex, newVal);
				break;
			}
		}
		break;
	}
	return true;
}

//*****************************************************************************
void CCharacter::setPredefinedVarString(
	const UINT varIndex, const WSTRING val,
	CCueEvents& CueEvents)
	//Sets the value of the predefined var with this relative index to the specified value
{
//	CCurrentGame* pGame = const_cast<CCurrentGame*>(this->pCurrentGame);

	ASSERT(varIndex >= (UINT)ScriptVars::FirstPredefinedVar);
	switch (varIndex)
	{
	case (UINT)ScriptVars::P_MONSTER_NAME:
		this->customName = val;
		break;
	}
}

//*****************************************************************************
bool CCharacter::HasSpecialDeath() const
//Returns: whether NPC's monster type has special death behavior
{
	switch (GetResolvedIdentity())
	{
		case M_FEGUNDO:
		case M_ROCKGOLEM:
			return true;
		default: return false;
	}
}

//*****************************************************************************
void CCharacter::ReflectX(CDbRoom *pRoom)
//Update script commands to work properly when the room is reflected horizontally (about the x-axis).
{
	CMonster::ReflectX(pRoom);
	for (COMMAND_VECTOR::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		switch (command->command)
		{
			case CCharacterCommand::CC_ActivateItemAt:
			case CCharacterCommand::CC_AppearAt:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_WaitForDoorTo:
			case CCharacterCommand::CC_GameEffect:
			case CCharacterCommand::CC_SetMonsterVar:
			case CCharacterCommand::CC_VarSetAt:
				command->x = (pRoom->wRoomCols-1) - command->x;
			break;
			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			case CCharacterCommand::CC_BuildTile:
			case CCharacterCommand::CC_WaitForItem:
				command->x = (pRoom->wRoomCols-1) - command->x - command->w;
			break;

			case CCharacterCommand::CC_FaceDirection:
			case CCharacterCommand::CC_WaitForPlayerToFace:
			case CCharacterCommand::CC_WaitForPlayerToMove:
			case CCharacterCommand::CC_WaitForOpenMove:
				if (IsValidOrientation(command->x))
					command->x = nGetO(-nGetOX(command->x),nGetOY(command->x));
				else
					command->x = command->x == CMD_C ? CMD_CC : CMD_C;
			break;

			case CCharacterCommand::CC_GenerateEntity:
				command->x = (pRoom->wRoomCols-1) - command->x;
				if (IsValidOrientation(command->w))
					command->w = nGetO(-nGetOX(command->w),nGetOY(command->w));
				else
					command->w = command->w == CMD_C ? CMD_CC : CMD_C;
			break;

			case CCharacterCommand::CC_MoveRel:
				command->x = (UINT)(-((int)command->x));
			break;
			default:	break;
		}
	}
}

//*****************************************************************************
void CCharacter::ReflectY(CDbRoom *pRoom)
//Update script commands to work properly when the room is reflected vertically (about the y-axis).
{
	CMonster::ReflectY(pRoom);
	for (COMMAND_VECTOR::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		switch (command->command)
		{
			case CCharacterCommand::CC_ActivateItemAt:
			case CCharacterCommand::CC_AppearAt:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_WaitForDoorTo:
			case CCharacterCommand::CC_GameEffect:
			case CCharacterCommand::CC_SetMonsterVar:
			case CCharacterCommand::CC_VarSetAt:
				command->y = (pRoom->wRoomRows-1) - command->y;
			break;
			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			case CCharacterCommand::CC_BuildTile:
			case CCharacterCommand::CC_WaitForItem:
				command->y = (pRoom->wRoomRows-1) - command->y - command->h;
			break;

			case CCharacterCommand::CC_FaceDirection:
			case CCharacterCommand::CC_WaitForPlayerToFace:
			case CCharacterCommand::CC_WaitForPlayerToMove:
			case CCharacterCommand::CC_WaitForOpenMove:
				if (IsValidOrientation(command->x))
					command->x = nGetO(nGetOX(command->x),-nGetOY(command->x));
				else
					command->x = command->x == CMD_C ? CMD_CC : CMD_C;
			break;

			case CCharacterCommand::CC_GenerateEntity:
				command->y = (pRoom->wRoomRows-1) - command->y;
				if (IsValidOrientation(command->w))
					command->w = nGetO(nGetOX(command->w),-nGetOY(command->w));
				else
					command->w = command->w == CMD_C ? CMD_CC : CMD_C;
			break;

			case CCharacterCommand::CC_MoveRel:
				command->y = (UINT)(-((int)command->y));
			break;
			default:	break;
		}
	}
}

//*****************************************************************************
void CCharacter::RotateClockwise(CDbRoom *pRoom)
//Update script commands to work properly when the room is rotated 90 degrees clockwise.
{
	CMonster::RotateClockwise(pRoom);
	int wNewX, wNewW;
	for (COMMAND_VECTOR::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		switch (command->command)
		{
			case CCharacterCommand::CC_AppearAt:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_ActivateItemAt:
			case CCharacterCommand::CC_WaitForDoorTo:
			case CCharacterCommand::CC_GameEffect:
			case CCharacterCommand::CC_SetMonsterVar:
				wNewX = (pRoom->wRoomRows-1) - command->y;
				command->y = command->x;
				command->x = wNewX;
			break;
			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			case CCharacterCommand::CC_BuildTile:
			case CCharacterCommand::CC_WaitForItem:
				//SW corner of rectangle will become the new NW corner after rotation.
				wNewX = (pRoom->wRoomRows-1) - (command->y + command->h);
				command->y = command->x;
				command->x = wNewX;
				//Width and height are switched.
				wNewW = command->h;
				command->h = command->w;
				command->w = wNewW;
			break;

			case CCharacterCommand::CC_FaceDirection:
			case CCharacterCommand::CC_WaitForPlayerToFace:
			case CCharacterCommand::CC_WaitForPlayerToMove:
			case CCharacterCommand::CC_WaitForOpenMove:
				if (IsValidOrientation(command->x))
					command->x = nNextCO(nNextCO(command->x));
			break;

			case CCharacterCommand::CC_GenerateEntity:
				wNewX = (pRoom->wRoomRows-1) - command->y;
				command->y = command->x;
				command->x = wNewX;
				if (IsValidOrientation(command->w))
					command->w = nNextCO(nNextCO(command->w));
			break;

			case CCharacterCommand::CC_MoveRel:
				wNewX = -(int)command->y;
				command->y = command->x;
				command->x = wNewX;
			break;
			default:	break;
		}
	}
}

//*****************************************************************************
CMonster* CCharacter::Replicate() const
//Make duplicate objects for the command vector.
{
	CCharacter *pCharacter = new CCharacter(*this);
	pCharacter->dwScriptID = this->dwScriptID; //must be reassigned correctly later
	for (COMMAND_VECTOR::iterator command = pCharacter->commands.begin();
			command != pCharacter->commands.end(); ++command)
	{
		if (command->pSpeech)
		{
			//Duplicate speech/sound data records for saving to DB.
			CDbSpeech *pSpeech = new CDbSpeech(*(command->pSpeech), true);
			ASSERT(pSpeech);
			const CDbDatum *pSound = command->pSpeech->GetSound();
			if (pSound)
			{
				CDbDatum *pNewData = new CDbDatum(*pSound, true);
				ASSERT(pNewData);
				pSpeech->SetSound(pNewData);
			}
			delete command->pSpeech;
			command->pSpeech = pSpeech;
		}
	}
	return pCharacter;
}

//*****************************************************************************
bool CCharacter::ResetLevelExits()
//Returns: whether any level exit commands were reset
{
	bool bUpdated = false;
	for (COMMAND_VECTOR::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		if (command->command == CCharacterCommand::CC_LevelEntrance)
		{
			command->x = 0;
			command->y = 0;
			bUpdated = true;
		}
	}
	return bUpdated;
}

//*****************************************************************************
void CCharacter::Defeat()
//Call this when the NPC is defeated (i.e. killed by anything except falling).
{
	this->bDefeated = true;
	this->HP = 0; //indicates defeat
	ASSERT(!IsCombatable()); //shouldn't be able to fight NPC again after defeat
}

//*****************************************************************************
bool CCharacter::OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/, const UINT /*wY*/)
//This is called when the NPC is defeated in combat.
//
//Returns: whether character was killed
{
	Defeat();

	if (!IsVulnerable()) //if not vulnerable, NPC doesn't die on defeat
		return false;

	CueEvents.Add(CID_MonsterDiedFromStab, this);
	return true;
}

//*****************************************************************************
bool CCharacter::IsValidExpression(
//Parses an expression to determine its syntactical validity.
//See ::parseExpression for supported grammar
//
//Returns: whether the expression is valid
//
//Params:
	const WCHAR *pwStr, UINT& index, CDbHold *pHold,
	const char closingChar) //[default=0] if set, indicates that the specified character (e.g., close paren) can mark the end of this (nested) expression
{
	ASSERT(pwStr);
	ASSERT(pHold);
	SKIP_WHITESPACE(pwStr, index);

	if (pwStr[index] == W_t('+') || pwStr[index] == W_t('-'))
		++index;

	if (!IsValidTerm(pwStr, index, pHold))
		return false;

	SKIP_WHITESPACE(pwStr, index);
	while (pwStr[index]!=0)
	{
		//Parse another term.
		if (pwStr[index] == W_t('+') || pwStr[index] == W_t('-'))
			++index;
		else if (closingChar && pwStr[index] == W_t(closingChar)) //closing nested expression
			return true; //caller will parse the closing character
		else
			return false; //invalid symbol between terms

		if (!IsValidTerm(pwStr, index, pHold))
			return false;
	}

	return true;
}

//*****************************************************************************
bool CCharacter::IsValidTerm(const WCHAR *pwStr, UINT& index, CDbHold *pHold)
//Returns: whether a term in an expression is valid
{
	if (!IsValidFactor(pwStr, index, pHold))
		return false;

	while (pwStr[index]!=0)
	{
		//Parse another term.
		SKIP_WHITESPACE(pwStr, index);

		if (pwStr[index] == W_t('*') || pwStr[index] == W_t('/') || pwStr[index] == W_t('%'))
			++index;
		else
			//no more factors in this term
			return true;

		if (!IsValidFactor(pwStr, index, pHold))
			return false;
	}

	return true;
}

//*****************************************************************************
//
// factor = var | number | "(" expression ")" | primitive "(" comma-separated expressions as input parameters ")"
//
bool CCharacter::IsValidFactor(const WCHAR *pwStr, UINT& index, CDbHold *pHold)
//Returns: whether a factor in an expression is valid
{
	SKIP_WHITESPACE(pwStr, index);

	//A nested expression?
	if (pwStr[index] == W_t('('))
	{
		++index;
		if (!IsValidExpression(pwStr, index, pHold, ')')) //recursive call
			return false;
		if (pwStr[index] != W_t(')')) //should be parsing the close parenthesis at this point
			return false; //missing close parenthesis

		++index;
		return true;
	}

	//Number?
	if (iswdigit(pwStr[index]))
	{
		//Parse past digits.
		++index;
		while (iswdigit(pwStr[index]))
			++index;

		if (iswalpha(pwStr[index])) //i.e. of form <digits><alphas>
			return false; //invalid var name

		return true;
	}

	//Variable identifier?
	if (pwStr[index] == W_t('_') || iswalpha(pwStr[index])) //valid first char
	{
		//Find spot where var identifier ends.
		int endIndex = index + 1;
		while (isVarCharValid(pwStr[endIndex]))
			++endIndex;

		const WSTRING wVarName(pwStr + index, endIndex - index);

		//Continue parsing after identifier
		index = endIndex;

		//Is it a predefined var?
		const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wVarName);
		if (eVar != ScriptVars::P_NoVar)
			return true;

		//Is it a hold var?
		if (pHold->GetVarID(wVarName.c_str()))
			return true;

		//Is it a function primitive?
		ScriptVars::PrimitiveType ePrimitive = ScriptVars::parsePrimitive(wVarName);
		if (ePrimitive != ScriptVars::NoPrimitive)
			return IsValidPrimitiveParameters(ePrimitive, pwStr, index, pHold);

		//Unrecognized identifier.
		return false;
	}

	//Invalid identifier
	return false;
}

//*****************************************************************************
bool CCharacter::IsValidPrimitiveParameters(
	ScriptVars::PrimitiveType ePrimitive,
	const WCHAR* pwStr, UINT& index, CDbHold* pHold)
{
	SKIP_WHITESPACE(pwStr, index);

	//Parse arguments surrounded by parens
	if (pwStr[index] != W_t('('))
		return false;
	++index;

	SKIP_WHITESPACE(pwStr, index);

	UINT numArgs = 0;
	if (pwStr[index] != W_t(')'))
	{
		UINT lookaheadIndex = index;
		while (IsValidExpression(pwStr, lookaheadIndex, pHold, ',')) //recursive call
		{
			index = lookaheadIndex;
			if (pwStr[index] != W_t(','))
				return false;

			++index;
			++numArgs;
			lookaheadIndex = index;
		}

		//no more commas; parse final argument
		if (!IsValidExpression(pwStr, index, pHold, ')')) //recursive call
			return false;
		++numArgs;
	}

	if (pwStr[index] != W_t(')')) //should be parsing the close parenthesis at this point
		return false; //missing close parenthesis
	++index;

	//Confirm correct parameter count for this primitive
	if (numArgs != ScriptVars::getPrimitiveRequiredParameters(ePrimitive))
		return false;

	return true;
}

//*****************************************************************************
int CCharacter::parseExpression(
//Parse and evaluate a simple nested expression for the grammar
//
// expression = ["+"|"-"] term {("+"|"-") term}
//
//Params:
	const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC, //[default=NULL]
	const char closingChar) //[default=0] if set, indicates that the specified character (e.g., close paren) can mark the end of this (nested) expression
{
	ASSERT(pwStr);
	ASSERT(pGame);
	SKIP_WHITESPACE(pwStr, index);

	bool bAdd = true; //otherwise subtract
	if (pwStr[index] == W_t('+'))
		++index;
	else if (pwStr[index] == W_t('-'))
	{
		bAdd = false;
		++index;
	}

	const int term = parseTerm(pwStr, index, pGame, pNPC);
	int val = (bAdd ? term : -term);

	SKIP_WHITESPACE(pwStr, index);
	while (pwStr[index]!=0)
	{
		//Parse another term.
		if (pwStr[index] == W_t('+'))
		{
			bAdd = true;
			++index;
		}
		else if (pwStr[index] == W_t('-'))
		{
			bAdd = false;
			++index;
		}
		else if (closingChar && pwStr[index] == W_t(closingChar)) //closing nested expression
			return val; //caller will parse the closing character
		else
		{
			//parse error -- return the current value
			LogParseError(pwStr + index, "Parse error (bad symbol)");
			return val;
		}

		const int term = parseTerm(pwStr, index, pGame, pNPC);
		val += (bAdd ? term : -term);
	}

	return val;
}

//*****************************************************************************
//Parse and evaluate a term in an expression.
//
// term = factor {("*"|"/"|"%") factor}
//
int CCharacter::parseTerm(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC)
{
	int val = parseFactor(pwStr, index, pGame, pNPC);

	while (pwStr[index]!=0)
	{
		//Parse another term.
		SKIP_WHITESPACE(pwStr, index);

		bool bMultiply;
		bool bMod = false;
		if (pwStr[index] == W_t('*'))
		{
			bMultiply = true;
			++index;
		}
		else if (pwStr[index] == W_t('/'))
		{
			bMultiply = false;
			++index;
		}
		else if (pwStr[index] == W_t('%'))
		{
			bMod = true;
			++index;
		} else {
			//no more factors in this term -- return result
			return val;
		}

		const int factor = parseFactor(pwStr, index, pGame, pNPC);
		if (bMod)
		{
			if (factor) //no mod by zero
				val = val % factor;
		}
		else if (bMultiply)
			val *= factor;
		else {
			if (factor) //no divide by zero
				val /= factor;
		}
	}

	return val;
}

//*****************************************************************************
int CCharacter::parseNestedExpression(const WCHAR* pwStr, UINT& index, CCurrentGame* pGame, CCharacter* pNPC)
//Parse and evaluate a nested expression.
{
	ASSERT(pwStr[index] == W_t('('));
	++index; //pass left paren

	int val = parseExpression(pwStr, index, pGame, pNPC, ')'); //recursive call
	SKIP_WHITESPACE(pwStr, index);
	if (pwStr[index] == W_t(')'))
		++index;
	else
	{
		//parse error -- return the current value
		LogParseError(pwStr, "Parse error (missing close parenthesis)");
	}
	return val;
}

//*****************************************************************************
int CCharacter::parseNumber(const WCHAR* pwStr, UINT& index)
{
	ASSERT(iswdigit(pwStr[index]));

	const int val = _Wtoi(pwStr + index);

	//Parse past digits.
	++index;
	while (iswdigit(pwStr[index]))
		++index;

	if (iswalpha(pwStr[index])) //i.e. of form <digits><alphas>
	{
		//Invalid var name -- skip to end of it and return zero value.
		while (isVarCharValid(pwStr[index]))
			++index;

		LogParseError(pwStr, "Parse error (invalid var name)");

		return 0;
	}

	return val;
}

//*****************************************************************************
int CCharacter::parseFactor(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC)
//Parse and evaluate a factor in an expression.
{
	SKIP_WHITESPACE(pwStr, index);

	//A nested expression?
	if (pwStr[index] == W_t('('))
		return parseNestedExpression(pwStr, index, pGame, pNPC);

	//Number?
	if (iswdigit(pwStr[index]))
		return parseNumber(pwStr, index);

	//Variable identifier?
	if (pwStr[index] == W_t('_') || iswalpha(pwStr[index])) //valid first char
	{
		//Find spot where var identifier ends.
		int endIndex = index + 1;
		while (isVarCharValid(pwStr[endIndex]))
			++endIndex;

		const WSTRING wVarName(pwStr + index, endIndex - index);

		//Continue parsing after identifier
		index = endIndex;

		//Is it a predefined var?
		const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wVarName);
		if (eVar != ScriptVars::P_NoVar)
		{
			if (!ScriptVars::IsStringVar(eVar)) {
				if (pNPC)
					return int(pNPC->getPredefinedVarInt(eVar));

				return pGame->getVar(eVar);
			}
		}

		//Is it a local hold var?
		if (pGame->pHold->GetVarID(wVarName.c_str())) {
			char* varName = pGame->pHold->getVarAccessToken(wVarName.c_str());
			const UNPACKEDVARTYPE vType = pGame->stats.GetVarType(varName);
			if (vType == UVT_int || vType == UVT_unknown) //valid int type
			{
				return pGame->stats.GetVar(varName, (int)0);
			}
		}

		//Is it a function primitive?
		ScriptVars::PrimitiveType ePrimitive = ScriptVars::parsePrimitive(wVarName);
		if (ePrimitive != ScriptVars::NoPrimitive)
			return parsePrimitive(ePrimitive, pwStr, index, pGame, pNPC);

		//else: unrecognized identifier -- just return a zero value below
	}

	//Invalid identifier
	LogParseError(pwStr, "Parse error(invalid var name)");

	return 0;
}

//*****************************************************************************
//Parses and evaluates a primitive function call in the context of the game object.
//
//Returns: calculated value from call to primitive
int CCharacter::parsePrimitive(
	ScriptVars::PrimitiveType ePrimitive,
	const WCHAR* pwStr, UINT& index, CCurrentGame* pGame, CCharacter* pNPC)
{
	SKIP_WHITESPACE(pwStr, index);

	ASSERT(pwStr[index] == W_t('('));
	++index; //pass left paren

	SKIP_WHITESPACE(pwStr, index);

	vector<int> params;

	if (pwStr[index] != W_t(')')) {
		UINT lookaheadIndex = index;
		while (IsValidExpression(pwStr, lookaheadIndex, pGame->pHold, ',')) //recursive call
		{
			params.push_back(parseExpression(pwStr, index, pGame, pNPC, ',')); //recursive call
			ASSERT(pwStr[index] == W_t(','));
			++index;
			lookaheadIndex = index;
		}

		//no more commas; parse final argument
		params.push_back(parseExpression(pwStr, index, pGame, pNPC, ')')); //recursive call
	}

	if (pwStr[index] == W_t(')')) {
		++index;
	} else {
		LogParseError(pwStr, "Parse error in primitive parameter list (missing close parenthesis)");
	}

	if (params.size() == ScriptVars::getPrimitiveRequiredParameters(ePrimitive))
		return pGame->EvalPrimitive(ePrimitive, params);

	LogParseError(pwStr, "Parse error in primitive parameter list (incorrect argument count)");
	return 0;
}

//***************************************************************************************
bool CCharacter::HasUnansweredQuestion(CCueEvents &CueEvents) const
{
	for (const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_MonsterSpoke);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonsterMessage *pMessage = DYN_CAST(const CMonsterMessage*, const CAttachableObject*, pObj);
		if (pMessage->pSender == this)
			return true;
	}

	return false;
}

//*****************************************************************************
void CCharacter::Process(
//Process a character for movement.
//
//Params:
	const int nLastCommand,   //(in) Last player command.
	CCueEvents &CueEvents)  //(in/out) Accepts pointer to a cues object that will be populated
							//with codes indicating events that happened that may correspond to
							//sound or graphical effects.
{
//If executing a command normally, calling this will end the character's turn.
//If the command was being evaluated as an If conditional, continue processing the next command.
#define STOP_COMMAND {if (!this->wJumpLabel) goto Finish; this->wJumpLabel=0; bProcessNextCommand=true; break;}

//Call this one instead if evaluating the condition took a turn and no more commands should be executed now.
#define STOP_DONECOMMAND {if (!this->wJumpLabel) goto Finish; this->wJumpLabel=0; break;}

	//Only character monsters taking up a single tile are implemented.
	ASSERT(!bIsSerpent(GetResolvedIdentity()));

	//Skip script execution if no commands to play.
	if (this->wCurrentCommandIndex >= this->commands.size())
		goto Finish;

	{ //scoping
	CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
	if (HasUnansweredQuestion(CueEvents) || pGame->HasUnansweredQuestion(this)) //don't execute further commands until current question is answered
		goto Finish;

	//If disabled, don't execute any script commands during combat.
	if (!this->bExecuteScriptOnCombat && pGame->InCombat() &&
			!this->bGlobal) //...this doesn't affect global scripts
	{
		pGame->bContinueCutScene = true; //cut scenes can keep going
		goto Finish; //just process persistent monster-like behaviors
	}

	CDbRoom& room = *(pGame->pRoom);
	CSwordsman& player = *pGame->pPlayer;

	//Keep track of swordsman's orientation on previous and current turn.
	this->wLastSO = this->wSO;
	this->wSO = player.wO;
	if (this->wLastSO == NO_ORIENTATION) this->wLastSO = this->wSO;   //init before first turn

	//On player room entrance, as a preprocessing step, execute commands that
	//don't require a turn to execute (e.g., visibility, gotos, etc.).
	bool bExecuteNoMoveCommands = pGame->ExecutingNoMoveCommands();

	this->bWaitingForCueEvent = this->bIfBlock = false;
	this->wSwordMovement = NO_ORIENTATION;
	this->wJumpLabel = 0;

	//Simple infinite loop checking.
	UINT wTurnCount = 0, wVarSets = 0;

	UINT px, py, pw, ph, pflags;  //command parameters

	bool bProcessNextCommand;     //true if next command should be initiated this turn
	do {
		if (this->wTurnDelay)
		{
			//Skip turn.
			--this->wTurnDelay;
			pGame->bContinueCutScene = true;
			goto Finish;
		}

		//Stop if no more commands to play.
		if (this->wCurrentCommandIndex >= this->commands.size())
			goto Finish;

		bProcessNextCommand = false;

		//While script commands are still executing, any cut scene playing may continue.
		pGame->bContinueCutScene = true;

		CCharacterCommand& command = this->commands[this->wCurrentCommandIndex];
		switch (command.command)
		{
			case CCharacterCommand::CC_Appear:
			{
				//Appear at current square.
				bProcessNextCommand = true;

				if (this->bVisible)
					break; //already in room
				if (this->bGlobal)
					break; //can't appear when running as a global script
				if (this->equipType != ScriptFlag::NotEquipment)
					break; //can't appear as inventory equipment

				const UINT identity = GetResolvedIdentity();
				if (identity >= CHARACTER_TYPES)
					break;	//nothing to show -- can't appear

				//Ensure square is available before appearing.
				ASSERT(room.IsValidColRow(this->wX, this->wY));
				if (room.GetMonsterAtSquare(this->wX, this->wY) != NULL ||
						pGame->IsPlayerAt(this->wX, this->wY))
					STOP_COMMAND;

				//Place character on starting square.
				this->bVisible = true;
				SetSwordSheathed();
				ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)]);
				room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = this;

				//Check for stepping on pressure plate.
				if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && !IsFlying())
					room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);

				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;
			case CCharacterCommand::CC_AppearAt:
			{
				//Appear at square (x,y).
				bProcessNextCommand = true;

				if (this->bVisible)
					break; //already in room
				if (this->bGlobal)
					break; //can't appear when running as a global script
				if (this->equipType != ScriptFlag::NotEquipment)
					break; //can't appear as inventory equipment

				const UINT identity = GetResolvedIdentity();
				if (identity >= CHARACTER_TYPES)
					break;	//nothing to show -- can't appear

				//Ensure square is available before appearing.
				getCommandXY(command, px, py);
				if (!room.IsValidColRow(px,py) ||
						room.GetMonsterAtSquare(px, py) != NULL ||
						pGame->IsPlayerAt(px, py))
					STOP_COMMAND;

				//Place character on starting square.
				this->bVisible = true;
				this->wPrevX = this->wX = px;
				this->wPrevY = this->wY = py;
				ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)]);
				room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = this;
				SetSwordSheathed();

				//Check for stepping on pressure plate.
				if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && !IsFlying())
					room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);

				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;
			case CCharacterCommand::CC_Disappear:
			{
				//Remove character from room (i.e. remove from view and action,
				//but script keeps running).
				bProcessNextCommand = true;

				if (!this->bVisible)
					break; //not in room

				Disappear();
				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;

			case CCharacterCommand::CC_MoveTo:
			{
				//Move to square (x,y) or target set in flags.
				//If w is set, then forbid turning while moving.
				//If h is set, then take only a single step before advancing to next command.
				//However, if the NPC is not visible, then a coord change to the destination occurs instantly.
				if (bExecuteNoMoveCommands && IsVisible())
				{
					//If the move is used as an If condition, back up to the If
					//so the check is performed for real next turn.
					if (this->wJumpLabel)
						--this->wCurrentCommandIndex;
					goto Finish;
				}

				//Move toward square (x,y) or target set by flags.
				UINT wDestX, wDestY;
				getCommandParams(command, px, py, pw, ph, pflags);
				wDestX = px;
				wDestY = py;
				if (!room.IsValidColRow(px,py))
				{
					bProcessNextCommand = true;
					break;
				}
				if (pflags)
				{
					CCoord *pDest = NULL;
					if ((pflags & ScriptFlag::PLAYER) != 0)
						pDest = (CCoord*)&player;
					else if ((pflags & ScriptFlag::HALPH) != 0)
						pDest = room.GetMonsterOfType(M_HALPH);
					else if ((pflags & ScriptFlag::MONSTER) != 0)
						pDest = room.pFirstMonster;
					else if ((pflags & ScriptFlag::NPC) != 0)
						pDest = room.GetMonsterOfType(M_CHARACTER);
					else if ((pflags & ScriptFlag::PDOUBLE) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_MIMIC)))
							if (!(pDest = room.GetMonsterOfType(M_DECOY)))
								pDest = room.GetMonsterOfType(M_CLONE);
					}
					else if ((pflags & ScriptFlag::SELF) != 0)
						break; //always at this position by definition
					else if ((pflags & ScriptFlag::SLAYER) != 0)
						pDest = room.GetMonsterOfType(M_SLAYER);
					else if ((pflags & ScriptFlag::BEETHRO) != 0)
					{
						if (player.wAppearance == M_BEETHRO)
							pDest = (CCoord*)&player;
						else
							pDest = room.GetMonsterOfType(M_BEETHRO);
					}
					if (!pDest)
						STOP_COMMAND;

					wDestX = pDest->wX;
					wDestY = pDest->wY;
				}

				//When not visible, go to destination instantly.
				if (!IsVisible())
				{
					this->wX = wDestX;
					this->wY = wDestY;

					CCoordSet coords(this->wX, this->wY);
					coords.insert(wDestX, wDestY);
					room.Plot(coords); //update room
				}

				if (this->wX == wDestX && this->wY == wDestY)
				{
					//At destination -- done with this command.
					bProcessNextCommand = true;
					break;
				}
				int dxFirst, dyFirst, dx, dy;
				GetDirectMovement(wDestX, wDestY, dxFirst, dyFirst, dx, dy,
						this->movementIQ, true);
				if (!dx && !dy)
				{
					if (ph)
					{
						//If single step, then advance to next command when can't move
						if (!pw)  //allow turning to face the intended direction
						{
							if (!bEntityHasSword(GetResolvedIdentity())) //only if w/o a sword
								SetOrientation(dxFirst,dyFirst);
						}
						break;
					}
					STOP_COMMAND;
				}

				//If monster type has a sword, then it must rotate slowly, and
				//it can't move on the same turn it is rotating.
				const bool bAllowTurning = !pw;
				if (bAllowTurning)
					if (bEntityHasSword(GetResolvedIdentity()) && HasSword())
					{
						const UINT wOldO = this->wO;
						if (MakeSlowTurn(nGetO(dx, dy))) //toward desired direction
						{
							this->wSwordMovement = CSwordsman::GetSwordMovement(
									this->wO == nNextCO(wOldO) ? CMD_C : CMD_CC, this->wO);
							if (ph)  //single step?
								break;
							STOP_DONECOMMAND;
						}
					}

				MoveCharacter(dx, dy, bAllowTurning, CueEvents);

				//Repeat command until arrived at destination.
				if (ph)  //single step?
					break;
				if (this->wX != px || this->wY != py)
					STOP_DONECOMMAND;
			}
			break;
			case CCharacterCommand::CC_MoveRel:
			{
				//Move (x,y) relative to current position. If w is set, then forbid turning while moving.
				//If h is set, then take only a single step before advancing to next command.
				//However, if the NPC is not visible, then a coord change to the destination occurs instantly.
				if (bExecuteNoMoveCommands && IsVisible())
				{
					//If the move is used as an If condition, back up to the If
					//so the check is performed for real next turn.
					if (this->wJumpLabel)
						--this->wCurrentCommandIndex;
					goto Finish;
				}

				//Move relative (x,y).
				getCommandRect(command, px, py, pw, ph);
				if (!this->bMovingRelative)
				{
					//Get relative vector first time this very command is invoked.
					this->wExitingRoomO = NO_ORIENTATION; //none by default
					if (!px && !py)
					{
						//Relative movement is zero -- nothing to do.
						bProcessNextCommand = true;
						break;
					}
					int nXDest = (int)this->wX + (int)px;
					int nYDest = (int)this->wY + (int)py;
					if (nXDest < 0)
					{
						this->wExitingRoomO = W;
						nXDest = 0;
					}
					else if (nXDest >= (int)room.wRoomCols)
					{
						this->wExitingRoomO = E;
						nXDest = room.wRoomCols - 1;
					}
					if (nYDest < 0)
					{
						nYDest = 0;
						this->wExitingRoomO = N;
					}
					else if (nYDest >= (int)room.wRoomRows)
					{
						nYDest = room.wRoomRows - 1;
						this->wExitingRoomO = S;
					}

					//When not visible, go to destination instantly.
					if (!IsVisible() && this->wExitingRoomO == NO_ORIENTATION)
					{
						this->wX = nXDest;
						this->wY = nYDest;

						CCoordSet coords(this->wX, this->wY);
						coords.insert(nXDest, nYDest);
						room.Plot(coords); //update room
						break;
					}

					this->wXRel = nXDest;
					this->wYRel = nYDest;
					this->bMovingRelative = true;
				}
				int dxFirst, dyFirst, dx, dy;
				GetDirectMovement(this->wXRel, this->wYRel, dxFirst, dyFirst, dx, dy,
						this->movementIQ, true);
				if (!dx && !dy)
				{
					if (ph && IsVisible())
					{
						//If single step, then advance to next command when can't move
						if (!pw)  //allow turning to face the intended direction
						{
							if (!bEntityHasSword(GetResolvedIdentity())) //only if w/o a sword
								SetOrientation(dxFirst,dyFirst);
						}
						this->bMovingRelative = false;
						break;
					}

					//If NPC is to cause a room exit (only when player role is NONE)
					//then the zero movement delta chosen above indicates the NPC
					//may be ready to exit the room.
					if (this->wExitingRoomO != NO_ORIENTATION &&
							//NPC can only force a room exit when player is not in room
							player.wIdentity == M_NONE)
					{
						//Exit if already at room edge.
						bool bExit = false;
						switch (this->wExitingRoomO)
						{
							case N:
								bExit = this->wY == 0;
							break;
							case W:
								bExit = this->wX == 0;
							break;
							case S:
								bExit = this->wY == room.wRoomRows-1;
							break;
							case E:
								bExit = this->wX == room.wRoomCols-1;
							break;
							default: ASSERT(!"Invalid exit."); break;
						}
						if (bExit)
						{
							//Set this command to finished, then load new room.
							this->bMovingRelative = false;
							ASSERT(this->wCurrentCommandIndex < this->commands.size());
							++this->wCurrentCommandIndex;

							if (pGame->LoadNewRoomForExit(this->wExitingRoomO, CueEvents))
								return;

							//If the current room couldn't be exited in this direction,
							//then current room play may continue as if this
							//command completed this turn anyway.
						}
					}

					STOP_COMMAND;
				}

				//If monster type has a sword, then it must rotate slowly, and
				//it can't move on the same turn it is rotating.
				const bool bAllowTurning = !pw;
				if (bAllowTurning)
					if (bEntityHasSword(GetResolvedIdentity()) && HasSword())
					{
						const UINT wOldO = this->wO;
						if (MakeSlowTurn(nGetO(dx, dy))) //toward desired direction
						{
							this->wSwordMovement = CSwordsman::GetSwordMovement(
									this->wO == nNextCO(wOldO) ? CMD_C : CMD_CC, this->wO);
							if (ph)  //single step?
							{
								this->bMovingRelative = false;
								break;
							}
							STOP_DONECOMMAND;
						}
					}

				MoveCharacter(dx, dy, bAllowTurning, CueEvents);

				//Repeat command until arrived at destination.
				if (ph)  //single step?
				{
					this->bMovingRelative = false;
					break;
				}
				if (this->wX != this->wXRel || this->wY != this->wYRel)
					STOP_DONECOMMAND;

				//Arrived.
				this->bMovingRelative = false;
			}
			break;
			case CCharacterCommand::CC_FaceDirection:
			{
				//Turn to face indicated direction.
				if (bExecuteNoMoveCommands && this->bVisible)
				{
					//If the move is used as an If condition, back up to the If
					//so the check is performed for real next turn.
					if (this->wJumpLabel)
						--this->wCurrentCommandIndex;
					goto Finish;
				}

				const UINT wOldO = this->wO;
				getCommandX(command, px);
				switch (px)
				{
					case CMD_C: this->wO = nNextCO(this->wO);
						this->wSwordMovement = CSwordsman::GetSwordMovement(CMD_C, this->wO);
					break;
					case CMD_CC: this->wO = nNextCCO(this->wO);
						this->wSwordMovement = CSwordsman::GetSwordMovement(CMD_CC, this->wO);
					break;
					default:
						if (!IsValidOrientation(px) && px != NO_ORIENTATION)
							break; //not valid -- do nothing
						this->wO = px;
					break;
				}
				SetSwordSheathed();
				if (!this->bVisible || //turning doesn't take time when not in room
						wOldO == this->wO) //already facing this way
					bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_ActivateItemAt:
			{
				//Activate item at (x,y).  Works only for the following items.
				getCommandXY(command, px, py);
				if (!room.IsValidColRow(px, py))
					break;

				const UINT lightVal = room.tileLights.GetAt(px, py);
				if (bIsWallLightValue(lightVal))
				{
					//Toggle wall light.  Doesn't expend a turn.
					room.tileLights.Add(px, py, WALL_LIGHT + ((lightVal + LIGHT_OFF) % 256));
					CueEvents.Add(CID_LightToggled);
					bProcessNextCommand = true;
				}
				switch (room.GetTSquare(px, py))
				{
					case T_LIGHT:
						//Toggle a light.  Doesn't expend a turn.
						room.ToggleLight(px, py, CueEvents);
						bProcessNextCommand = true;
					break;
					case T_TOKEN:
						//Activate a token.  Doesn't expend a turn.
						pGame->ActivateTokenAt(px, py);
						bProcessNextCommand = true;
					break;

					case T_BOMB:
						if (pGame->wTurnNo > 0) //not on room entrance, since the player could die immediately
						{
							//Explode a bomb immediately.  Doesn't expend a turn.
							CCoordStack bomb(px, py);
							room.BombExplode(CueEvents, bomb);
							if (IsAlive())
								bProcessNextCommand = true;
						} else {
							//Pause script until after first turn to explode the bomb.
							STOP_COMMAND;
						}
					break;
					case T_FUSE:
						//Light fuse.  Doesn't expend a turn.
						room.LightFuse(CueEvents, px, py,
								//don't cause waiting another turn when executed on room entrance
								pGame->wTurnNo > 0);
						bProcessNextCommand = true;
					break;

					case T_ORB:
						//Activate orb.
						if (bExecuteNoMoveCommands) return;
						room.ActivateOrb(px, py, CueEvents, OAT_ScriptOrb);
						bProcessNextCommand = true;
					break;

					default:
					{
						const UINT oTile = room.GetOSquare(px, py);
						switch (oTile)
						{
							case T_PRESSPLATE:
							{
								//Activate pressure plate.
								if (bExecuteNoMoveCommands) return;

								//Determine tile location of pressure plate data object.
								UINT wX = px, wY = py;
								COrbData *pData = room.GetPressurePlateAtCoords(wX, wY);
								if (pData)
								{
									//Activate the pressure plate from this tile.
									wX = pData->wX;
									wY = pData->wY;
								}
								room.ActivateOrb(wX, wY, CueEvents, OAT_ScriptPlate);
								bProcessNextCommand = true;
							}
							break;
							case T_DOOR_Y: case T_DOOR_B: case T_DOOR_G: case T_DOOR_R: case T_DOOR_C: case T_DOOR_MONEY:
							case T_DOOR_YO: case T_DOOR_BO: case T_DOOR_GO: case T_DOOR_RO: case T_DOOR_CO: case T_DOOR_MONEYO:
								//Toggle this door.
								room.ToggleDoor(px, py);
								bProcessNextCommand = true;
							break;
							default:
								//No item to active on this tile.  Just continue script.
								bProcessNextCommand = true;
							break;
						}
					}
					break;
				}
			}
			break;
			case CCharacterCommand::CC_AddRoomToMap:
			{
				//Add room at (x,y) to player's mapped rooms.
				const UINT roomID = pGame->pLevel->GetRoomIDAtCoords(
						command.x, pGame->pLevel->dwLevelID*100 + command.y);
				if (roomID)
				{
					const bool bMarkExplored = command.w != 0;
					pGame->AddRoomToMap(roomID, bMarkExplored);
					CueEvents.Add(CID_LevelMap, new CAttachableWrapper<UINT>(
							bMarkExplored ? T_MAP_DETAIL : T_MAP));
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_Autosave:
			{
				//Autosave with identifier 'label'.
				if (pGame->Autosave(command.label))
					CueEvents.Add(CID_Autosave);
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_Wait:
			{
				//Set to wait for X turns.
				getCommandX(command, px);
				if (px)
				{
					//If a move can't be made right now, then don't register
					//the delay until next turn to avoid an additional turn delay
					//in loops after it completes.
					if (bExecuteNoMoveCommands && !this->bGlobal)
						STOP_COMMAND;

					this->wTurnDelay = px;
					--this->wTurnDelay; //This turn counts as one of them.
				}
				//else: when a 0 wait is specified, stop executing commands until the next turn
			}
			break;
			case CCharacterCommand::CC_WaitForCueEvent:
			{
				//Wait for cue event X to fire.
				const CUEEVENT_ID cid = static_cast<CUEEVENT_ID>(command.x);
				if (!CueEvents.HasOccurred(cid))
				{
					//If NPC is waiting, try to catch event at end of game turn in CheckForCueEvent().
					//!!NOTE: This won't work for conditional "If <late cue event>".
					if (!this->wJumpLabel)
						this->bWaitingForCueEvent = true;
					STOP_COMMAND;
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForDefeat:
			{
				//Wait until the NPC-monster is defeated in combat.
				if (!this->bDefeated)
					STOP_COMMAND;

				this->bDefeated = false; //reset this flag to allow another defeat if desired
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			{
				//Wait until a specified entity is in rect (x,y,w,h).
				// -OR-
				//Wait until NONE of the specified entities are in rect (x,y,w,h).
				//
				//Note that width and height are zero-indexed.
				bool bFound = false;
				getCommandParams(command, px, py, pw, ph, pflags);
				if (!room.IsValidColRow(px, py) || !room.IsValidColRow(px+pw, py+ph))
					STOP_COMMAND;

				if (!bFound && (!pflags || (pflags & ScriptFlag::PLAYER) != 0))
				{
					//Check for player by default if no flags are selected.
					if (
						player.IsInRoom() &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::HALPH) != 0)
				{
					if (player.wAppearance == M_HALPH &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else
					if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_HALPH, true))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::MONSTER) != 0)
				{
					//excludes NPCs
					if (room.IsMonsterInRect(px, py,
							px + pw, py + ph))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::NPC) != 0)
				{
					//visible characters only
					if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_CHARACTER))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::PDOUBLE) != 0)
				{
					//All player double types
					if (bIsBeethroDouble(player.wAppearance) && player.wAppearance != M_BEETHRO &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else
					if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_MIMIC))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_DECOY))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_CLONE))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::SELF) != 0)
				{
					if (this->wX >= px && this->wX <= px + pw &&
							this->wY >= py && this->wY <= py + ph)
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::SLAYER) != 0)
				{
					if (player.wAppearance == M_SLAYER &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else
					if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_SLAYER, true))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::BEETHRO) != 0)
				{
					//Check for Beethro (can detect NPC Beethros).
					if (player.wAppearance == M_BEETHRO)
					{
						if (player.wX >= px && player.wX <= px + pw &&
								player.wY >= py && player.wY <= py + ph)
							bFound = true;
					}
					CMonster *pNPCBeethro = pGame->pRoom->GetNPCBeethro();
					if (pNPCBeethro)
					{
						const UINT wSX = pNPCBeethro->wX;
						const UINT wSY = pNPCBeethro->wY;
						if (wSX >= px && wSX <= px + pw &&
								wSY >= py && wSY <= py + ph)
							bFound = true;
					}
				}
				if (!bFound && (pflags & ScriptFlag::STALWART) != 0)
				{
					if (player.wAppearance == M_STALWART &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else
					if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_STALWART, true))
						bFound = true;
				}

				if ((command.command == CCharacterCommand::CC_WaitForRect && !bFound) ||
					 (command.command == CCharacterCommand::CC_WaitForNotRect && bFound))
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_WaitForDoorTo:
			{
				//Wait for door at (x,y) to (w=close/open).
				getCommandXY(command, px, py);
				const UINT wTile = room.GetOSquare(px, py);
				if (command.w==(UINT)OA_CLOSE && !bIsDoor(wTile))
					STOP_COMMAND;  //door hasn't closed yet
				if (command.w==(UINT)OA_OPEN && bIsDoor(wTile))
					STOP_COMMAND;  //door hasn't opened yet
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForTurn:
			{
				//Wait until play reaches turn X.
				getCommandX(command, px);
				if (pGame->wSpawnCycleCount < px)
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForPlayerToFace:
			{
				//Wait until player faces orientation X.
				if (!player.IsInRoom())
					STOP_COMMAND;
				ASSERT(this->wLastSO != NO_ORIENTATION);
				ASSERT(this->wSO != NO_ORIENTATION);
				getCommandX(command, px);
				switch (px)
				{
					case CMD_C:
						if (this->wSO != nNextCO(this->wLastSO))
							STOP_COMMAND;
						break;
					case CMD_CC:
						if (this->wSO != nNextCCO(this->wLastSO))
							STOP_COMMAND;
						break;
					default:
						if (this->wSO != px)
							STOP_COMMAND;
					break;
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForPlayerToMove:
			{
				//Wait until player moves in direction X.
				if (!player.IsInRoom())
					STOP_COMMAND;
				ASSERT(this->wLastSO != NO_ORIENTATION);
				ASSERT(this->wSO != NO_ORIENTATION);
				const bool bPlayerMoved = player.wX != player.wPrevX ||
						player.wY != player.wPrevY;
				getCommandX(command, px);
				switch (px)
				{
					case CMD_C:
						if (this->wSO != nNextCO(this->wLastSO))
							STOP_COMMAND;
						break;
					case CMD_CC:
						if (this->wSO != nNextCCO(this->wLastSO))
							STOP_COMMAND;
						break;
					default:
						if (!bPlayerMoved || CSwordsman::GetSwordMovement( //conversion routine
								nLastCommand, NO_ORIENTATION) != px)
							STOP_COMMAND;
					break;
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForPlayerToTouchMe:
			{
				//Wait until player bumps into me (on this turn).
				if (player.wX == this->wX && player.wY == this->wY)
					this->bPlayerTouchedMe = true; //standing on an invisible NPC counts

				if (!this->bPlayerTouchedMe)
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForOpenMove:
			{
				//Wait until NPC may take a step or turn toward orientation X.
				if (IsVisible())
				{
					bool bOpen = true;
					UINT wSX, wSY;
					getCommandX(command, px);
					switch (px)
					{
						case N: case S: case E: case W:
						case NW: case SW: case NE: case SE:
						{
							//Can I move in this direction?
							int dxFirst, dyFirst, dx, dy;
							UINT wMove = px;
							UINT wDestX = this->wX + nGetOX(wMove);
							UINT wDestY = this->wY + nGetOY(wMove);
							GetBeelineMovementDumb(wDestX, wDestY, dxFirst, dyFirst, dx, dy);
							if (!dx && !dy)
								bOpen = false;
							else
							{
								//Can I move here without stabbing something that would kill me?
								const UINT wOldX = this->wX, wOldY = this->wY;
								this->wX = wDestX; //temp change to perform these queries
								this->wY = wDestY;
								if (GetSwordCoords(wSX, wSY) && !DoesSquareRemoveSword(wDestX, wDestY))
									if (!IsSafeToStab(wSX, wSY))
										bOpen = false;
								this->wX = wOldX;
								this->wY = wOldY;
							}
						}
						break;
						case CMD_C:
						case CMD_CC:
						{
							const UINT wOldO = this->wO;
							this->wO = (px == CMD_C) ? nNextCO(this->wO) : nNextCCO(this->wO);
							if (GetSwordCoords(wSX, wSY))
							{
								if (!IsSafeToStab(wSX, wSY))
									bOpen = false;
							}
							this->wO = wOldO;
						}
						break;
						default: break;
					}
					if (!bOpen)
						STOP_COMMAND;
				}
				bProcessNextCommand = true;
			}

			case CCharacterCommand::CC_Label:
				//A comment or destination marker for a GoTo command.
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_GoSub:
			case CCharacterCommand::CC_GoTo:
			{
				//Continue executing script commands from marked jump point.
				//Will continue script if jump point is invalid.
				const int wNextIndex = GetIndexOfCommandWithLabel(command.x);
				this->bParseIfElseAsCondition = false;
				if (wNextIndex != NO_LABEL) {
					if (command.command == CCharacterCommand::CC_GoSub)
						this->jumpStack.push_back(this->wCurrentCommandIndex + 1);
					this->wCurrentCommandIndex = wNextIndex;
				} else {
					++this->wCurrentCommandIndex; //invalid jump -- just play next command
				}
				bProcessNextCommand = true;
			}
			continue;   //don't increment wCurrentCommandIndex again
			case CCharacterCommand::CC_Return:
				if (this->jumpStack.empty()) {
					this->wCurrentCommandIndex = this->commands.size(); //done with script execution
				} else {
					this->wCurrentCommandIndex = this->jumpStack.back();
					this->jumpStack.pop_back();
				}
				bProcessNextCommand = true;
			continue;   //don't increment wCurrentCommandIndex again

			case CCharacterCommand::CC_Speech:
			{
				//Deliver speech dialog.
				if (!command.pSpeech)
					break; //robustness check
				CFiredCharacterCommand *pSpeech = new CFiredCharacterCommand(this, &command,
					pGame->wTurnNo, this->dwScriptID, this->wCurrentCommandIndex);
				pSpeech->text = pGame->ExpandText(
						(const WCHAR*)command.pSpeech->MessageText, this);

				CueEvents.Add(CID_Speech, pSpeech);	//don't attach object to event
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_FlushSpeech:
			{
				//Remove queued speech commands (front end: either display immediately or discard).
				CFiredCharacterCommand *pFlushSpeech = new CFiredCharacterCommand(this, &command,
					pGame->wTurnNo, this->dwScriptID, this->wCurrentCommandIndex);
				pFlushSpeech->bFlush = true;
				pFlushSpeech->bPlaySound = command.x != 0;
				CueEvents.Add(CID_Speech, pFlushSpeech);	//don't attach -- it should be deleted by handler
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_AnswerOption:
			{
				//Text answer option (speech text) for a Question command that jumps to Label (x).
				if (this->answerOptions.size() < MAX_ANSWERS)
					this->answerOptions += this->wCurrentCommandIndex;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_Question:
			{
				//Ask a yes/no or multiple choice question (speech text).

				//Wait on questions if they're being asked when the player can't make moves.
				if (pGame->ExecutingNoMoveCommands() || pGame->InCombat())
				{
					//If the question follows an If condition, back up to the If
					//so the check is performed for real next turn.
					if (this->wJumpLabel)
						--this->wCurrentCommandIndex;
					return;
				}

				//Ignore set of answers if question is an If condition (i.e. follows an If command)
				this->bYesNoQuestion = this->answerOptions.empty() || this->bIfBlock;
				CDbSpeech *pSpeech = command.pSpeech;
				ASSERT(pSpeech);
				const WCHAR *pText = (const WCHAR*)pSpeech->MessageText;
				WSTRING wstr = pGame->ExpandText(pText, this);
				CMonsterMessage *pMessage = new CMonsterMessage(
						this->bYesNoQuestion ? MMT_YESNO : MMT_MENU, wstr.c_str(), this);
				CueEvents.Add(CID_MonsterSpoke, pMessage, true);

				//Stop processing until answer to question is received, then resume next turn.
				++this->wCurrentCommandIndex;
				goto Finish;
			}
			break;

			case CCharacterCommand::CC_SetMusic:
			{
				//Set music being played to X (custom dataID W/label).
				pGame->music.clear();

				if (!command.label.empty())
					pGame->music.songMood = command.label;
				else
				{
					pGame->music.musicEnum = command.x; //enumeration
					pGame->music.musicID = command.w;   //dataID
					pGame->music.bPlayMusicID = true;
				}
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_Imperative:
			{
				//Flag NPC imperative.
				bProcessNextCommand = true;

				const ScriptFlag::Imperative eNewImperative = (ScriptFlag::Imperative)command.x;
				switch (eNewImperative)
				{
					case ScriptFlag::Killable:
						this->bVulnerable = true;
					break;
					case ScriptFlag::Unkillable:
						this->bVulnerable = false;
					break;
					case ScriptFlag::MissionCritical:
						this->bMissionCritical = true;
					break;
					case ScriptFlag::NotMissionCritical:
						this->bMissionCritical = false;
					break;
					case ScriptFlag::Die:
						//Stop script execution whether visible or not.
						bProcessNextCommand = false;
						if (pGame->wPlayerTurn == 0)
							return; //wait until first move to display death

						this->wCurrentCommandIndex = this->commands.size();
						if (this->bVisible)
						{
							//NPC dies.
							CueEvents.Add(CID_MonsterDiedFromStab, this);

							CCueEvents Ignored;
							SetKillInfo(NO_ORIENTATION); //center stab effect
							room.KillMonster(this, Ignored, true);
						}
					break;
					case ScriptFlag::Safe:
						this->bSafeToPlayer = true;
					break;
					case ScriptFlag::Deadly:
						this->bSafeToPlayer = this->bSwordSafeToPlayer = false;
					break;
					case ScriptFlag::SwordSafeToPlayer:
						this->bSwordSafeToPlayer = true;
					break;

					//Whether changes to player stats are displayed on screen.
					case ScriptFlag::HideStatChanges:
						this->bShowStatChanges = false;
					break;
					case ScriptFlag::ShowStatChanges:
						this->bShowStatChanges = true;
					break;

					//Whether a not-visible NPC (i.e. not occupying a room tile) is displayed on screen.
					case ScriptFlag::NoGhostDisplay:
						this->bGhostImage = false;
					break;
					case ScriptFlag::GhostDisplay:
						this->bGhostImage = true;
					break;

					//Whether script should be restarted each time room is (re)entered.
					case ScriptFlag::NoRestartScriptOnRoomEntrance:
						this->bRestartScriptOnRoomEntrance = false;
					break;
					case ScriptFlag::RestartScriptOnRoomEntrance:
						this->bRestartScriptOnRoomEntrance = true;
					break;

					//Script enters the hold's global script list.
					case ScriptFlag::MakeGlobal:
						this->bGlobal = true;
						this->bVisible = false; //no longer as an entity in the room
					break;

					//Whether to execute script commands on the turn when combat is initiated.
					case ScriptFlag::RunOnCombat:
						this->bExecuteScriptOnCombat = true;
					break;
					case ScriptFlag::PauseOnCombat:
						this->bExecuteScriptOnCombat = false;
					break;

					default:
						ASSERT(!"Unknown imperative");
					break;
				}
			}
			break;

			case CCharacterCommand::CC_Behavior:
			{
				//Activate behavior pattern X.
				bProcessNextCommand = true;

				const ScriptFlag::Behavior eNewBehavior = (ScriptFlag::Behavior)command.x;
				switch (eNewBehavior)
				{
					case ScriptFlag::Standard:
						this->bAttackAdjacent =
						this->bAttackInFront =
						this->bAttackInFrontWhenBackIsTurned =
						this->bFaceAwayFromTarget =
						this->bFaceTarget =
						this->bHasRayGun = this->bHasRayBlocking =
						this->bSurprisedFromBehind =
						this->bGoblinWeakness = this->bSerpentWeakness =
						this->bMetal = this->bLuckyGR = this->bLuckyXP = this->bBriar = this->bNoEnemyDEF =
						this->bAttackFirst = this->bAttackLast =
						this->bDropTrapdoors = this->bMoveIntoSwords = this->bPushObjects = this->bSpawnEggs =
							false;
						this->movementIQ = SmartDiagonalOnly;
					break;
					case ScriptFlag::FaceAwayFromTarget:
						this->bFaceAwayFromTarget = true;
						this->bFaceTarget = false;
					break;
					case ScriptFlag::FaceTarget:
						this->bFaceTarget = true;
						this->bFaceAwayFromTarget = false;
					break;
					case ScriptFlag::AttackInFrontWithBackTurned:
						this->bAttackInFrontWhenBackIsTurned = true;
					break;
					case ScriptFlag::AttackInFront:
						this->bAttackInFront = true;
					break;
					case ScriptFlag::AttackAdjacent:
						this->bAttackAdjacent = true;
					break;
					case ScriptFlag::BeamAttack:
						this->bHasRayGun = true;
					break;
					case ScriptFlag::BeamBlock:
						this->bHasRayBlocking = true;
						break;
					case ScriptFlag::SurprisedFromBehind:
						this->bSurprisedFromBehind = true;
					break;
					case ScriptFlag::GoblinWeakness:
						this->bGoblinWeakness = true;
					break;
					case ScriptFlag::SerpentWeakness:
						this->bSerpentWeakness = true;
					break;
					case ScriptFlag::Metal:
						this->bMetal = true;
					break;
					case ScriptFlag::LuckyGR:
						this->bLuckyGR = true;
					break;
					case ScriptFlag::LuckyXP:
						this->bLuckyXP = true;
					break;
					case ScriptFlag::Briar:
						this->bBriar = true;
					break;
					case ScriptFlag::NoEnemyDefense:
						this->bNoEnemyDEF = true;
					break;
					case ScriptFlag::AttackFirst:
						this->bAttackFirst = true;
						this->bAttackLast = false;
					break;
					case ScriptFlag::AttackLast:
						this->bAttackLast = true;
						this->bAttackFirst = false;
					break;

					case ScriptFlag::DirectMovement:
						this->movementIQ = DirectOnly;
					break;
					case ScriptFlag::NormalMovement:
						this->movementIQ = SmartDiagonalOnly;
					break;
					case ScriptFlag::SmarterMovement:
						this->movementIQ = SmarterDiagonalOnly;
					break;
					case ScriptFlag::OmniMovement:
						this->movementIQ = SmartOmniDirection;
					break;

					case ScriptFlag::DropTrapdoors:
						this->bDropTrapdoors = true;
					break;
					case ScriptFlag::MoveIntoSwords:
						this->bMoveIntoSwords = true;
					break;
					case ScriptFlag::PushObjects:
						this->bPushObjects = true;
					break;
					case ScriptFlag::SpawnEggs:
						this->bSpawnEggs = true;
					break;

					default:
						ASSERT(!"Unknown behavior");
					break;
				}
			}
			break;

			case CCharacterCommand::CC_SetMovementType:
			{
				//Set movement type X
				bProcessNextCommand = true;

				const MovementType eNewMovementType = (MovementType)command.x;
				this->eMovement = eNewMovementType;
			}
			break;

			case CCharacterCommand::CC_EachAttack:
				//Goto label X each time I attack (i.e. hit the player).
				this->eachAttackLabelIndex = GetIndexOfCommandWithLabel(command.x);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_EachDefend:
				//Goto label X each time I defend (i.e. get hit).
				this->eachDefendLabelIndex = GetIndexOfCommandWithLabel(command.x);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_EachUse:
				//Goto label X each time I use this item (for custom inventory items).
				this->eachUseLabelIndex = GetIndexOfCommandWithLabel(command.x);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_EachVictory:
				//Goto label X each time combat victory occurs.
				this->eachVictoryLabelIndex = GetIndexOfCommandWithLabel(command.x);
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_Equipment:
			{
				//Give/take player equipment type X, defined by hold custom charID Y.
				bProcessNextCommand = true;

				ScriptFlag::TransactionType trans = (ScriptFlag::TransactionType)command.w;
				switch (trans)
				{
					default:
					case ScriptFlag::Trade:
					{
						//Swap this item into player's inventory.
						//Previous item is dropped.
						const UINT equipmentID = command.y;
						switch (command.x)
						{
							case ScriptFlag::Weapon: pGame->TradeWeapon(CueEvents, equipmentID, this->bShowStatChanges); break;
							case ScriptFlag::Armor: pGame->TradeArmor(CueEvents, equipmentID, this->bShowStatChanges); break;
							case ScriptFlag::Accessory: pGame->TradeAccessory(CueEvents, equipmentID, this->bShowStatChanges); break;
							case ScriptFlag::Command: pGame->changingInventory(CueEvents, command.x, equipmentID); break;
							default: break;
						}
					}
					break;
					case ScriptFlag::Destroy:
						pGame->DestroyInventory(CueEvents, command.x, this->bShowStatChanges);
					break;
					case ScriptFlag::Sell:
						pGame->SellInventory(CueEvents, command.x, this->bShowStatChanges);
					break;
					case ScriptFlag::Disable:
						pGame->DisableInventory(CueEvents, command.x, true);
					break;
					case ScriptFlag::Enable:
						pGame->EnableInventory(CueEvents, command.x);
					break;
					case ScriptFlag::QueryStatus:
						if (!pGame->QueryInventoryStatus(command.x))
							STOP_COMMAND;
					break;
				}
			}
			break;

			case CCharacterCommand::CC_GenerateEntity:
			{
				//Generates a new entity of type h in the room at (x,y) with orientation w.

				//Ensure square is available before appearing.
				//Custom NPCs may appear where another entity is in the room,
				//but they will not be allowed to appear in the room until the
				//tile is vacant.
				getCommandRect(command, px, py, pw, ph);
				const UINT identity = ph;
				if (room.IsValidColRow(px,py)) {
					if (identity < CUSTOM_CHARACTER_FIRST &&
						(room.GetMonsterAtSquare(px, py) != NULL ||
							pGame->IsPlayerAt(px, py)))
					STOP_COMMAND;

					//Place new entity on this tile.
					if (pw == CMD_C)
						pw = nNextCO(this->wO);
					else if (pw == CMD_CC)
						pw = nNextCCO(this->wO);

					pGame->AddNewEntity(CueEvents, identity, px, py, pw); 
				}

				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_SetMonsterVar:
				//Sets monster at (x,y) attribute(w) to value (h).
				//No effects accompany this command.
				//It's intended to operate invisibly.
				{
					getCommandRect(command, px, py, pw, ph);

					CMonster* pMonster = room.GetMonsterAtSquare(px, py);
					if (pMonster) {
						int nValue = int(ph); //expect an integer value by default
						if (!nValue && !command.label.empty())
						{
							//Operand is not just an integer, but a text expression.
							UINT index = 0;
							nValue = parseExpression(command.label.c_str(), index, pGame, this);
						}
						const UINT value = UINT(nValue > 0 ? nValue : 0); //support only non-negative values

						switch (pw) {
							case ScriptFlag::HP: pMonster->HP = max(1, value); break; //cannot kill monster via this command
							case ScriptFlag::ATK: pMonster->ATK = value; break;
							case ScriptFlag::DEF: pMonster->DEF = value; break;
							case ScriptFlag::GOLD: pMonster->GOLD = value; break;
							case ScriptFlag::XP: pMonster->XP = value; break;
							default: break; //do nothing
						}
					}
				}
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_GameEffect:
			{
				//Cues the front end to generate a graphic+sound effect (w,h,flags) at (x,y).
				getCommandParams(command, px, py, pw, ph, pflags);
				if (room.IsValidColRow(px,py)) {
					UINT wO = pw;
					if (!IsValidOrientation(wO))
						wO = NO_ORIENTATION;
					VisualEffectInfo *pEffect = new VisualEffectInfo(px, py, wO, ph, pflags, this->wX, this->wY);
					CueEvents.Add(CID_VisualEffect, pEffect, true);
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_RoomLocationText:
			{
				//Sets the room location text.
				CDbSpeech* pSpeech = command.pSpeech;
				ASSERT(pSpeech);
				pGame->customRoomLocationText = pGame->ExpandText((const WCHAR*)pSpeech->MessageText, this);
				CueEvents.Add(CID_RoomLocationTextUpdate);

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_FlashingText:
			{
				//Flashes a large message onscreen.  Use hex code RRGGBB color (x,y,w) when h is set.
				getCommandRect(command, px, py, pw, ph);

				CDbSpeech* pSpeech = command.pSpeech;
				ASSERT(pSpeech);
				const WSTRING text = pGame->ExpandText((const WCHAR*)pSpeech->MessageText, this);
				CDbMessageText* pText = new CDbMessageText();
				*pText = text.c_str();
				CColorText* pColorText = new CColorText(pText, px, py, pw, ph);
				CueEvents.Add(CID_FlashingMessage, pColorText, true);

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_ScoreCheckpoint:
			{
				//Defines a scoring point with identifier 'label'.
				const bool bNotFrozen = !this->pCurrentGame->Commands.IsFrozen();
				if (bNotFrozen ||  //when playing back commands, don't do this stuff
						this->pCurrentGame->IsValidatingPlayback()) //unless we're validating
				{
					CDbMessageText *pScoreIDText = new CDbMessageText();
					*pScoreIDText = command.label.c_str();
					CueEvents.Add(CID_ScoreCheckpoint, pScoreIDText, true);
					if (bNotFrozen)
						const_cast<CCurrentGame*>(this->pCurrentGame)->WriteScoreCheckpointSave(command.label);
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_TurnIntoMonster:
				//Replace with normal monster of specified type.
				TurnIntoMonster(CueEvents);
			break;
			case CCharacterCommand::CC_EndScript:
				//Remove character from any future play in the current game.
				this->bScriptDone = true;
				this->wCurrentCommandIndex = this->commands.size();
			goto Finish;
			case CCharacterCommand::CC_EndScriptOnExit:
				//Remove character from any future play in the current game once the room is exited.
				this->bScriptDone = true;
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_ReplaceWithDefault:
				//Replace the script with the character's default script if possible.
				//Nothing will happen for non-custom characters.
				if (this->pCustomChar) {
					this->commands.clear();
					this->wCurrentCommandIndex = 0;
					wTurnCount = 0;
					++wVarSets; //Count as setting a variable for loop avoidence
					LoadCommands(this->pCustomChar->ExtraVars, this->commands);
				}	else {
					// Index does not automatically increment after this command is executed
					++this->wCurrentCommandIndex;
				}
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_If:
				//Begin a conditional block if the next command is satisfied.
				//If it is not satisfied, the code block will be skipped.
				++this->wCurrentCommandIndex;
				this->wJumpLabel = this->wCurrentCommandIndex + 1;
				this->bIfBlock = true;
				this->bParseIfElseAsCondition = false;
				bProcessNextCommand = true;
			continue;   //perform the jump check below next iteration
			case CCharacterCommand::CC_IfElse:
			case CCharacterCommand::CC_IfElseIf:
			{
				//Marks the beginning of a code block executed when an CC_If condition was not satisfied.
				//Note that reaching this command indicates an If (true) block has successfully
				//completed, thus the following Else code block should be skipped.
				//
				//If an Else command is encountered in the code (w/o any previous If)
				//then it will effective function as an If-false and skip the next block.
				this->bIfBlock = true; //this will skip the subsequent code block
				bProcessNextCommand = true;

				// Are we supposed to parse this ElseIf as a condition 
				if (command.command == CCharacterCommand::CC_IfElseIf && this->bParseIfElseAsCondition) {
					if (bExecuteNoMoveCommands)
					{
						goto Finish;
					}

					++this->wCurrentCommandIndex;
					this->wJumpLabel = this->wCurrentCommandIndex + 1;
					continue;
				}
			}
			break;
			case CCharacterCommand::CC_IfEnd:
				//Ends a conditional If or IfElse block.
				bProcessNextCommand = true;
				this->bParseIfElseAsCondition = false;
			break;

			case CCharacterCommand::CC_LevelEntrance:
				//Takes player to level entrance X.  If Y is set, skip level entrance display.
				if (!pGame->wTurnNo)
					return; //don't execute on the room entrance move -- execute next turn

				//When saving room data in GotoLevelEntrance,
				//this NPC should start at the next script command
				//next time the script is processed.
				++this->wCurrentCommandIndex;

				getCommandXY(command, px, py); //NOTE: only py is considered here
				if (!CueEvents.HasOccurred(CID_ExitLevelPending)) //don't queue more than one level exit
					pGame->GotoLevelEntrance(CueEvents, command.x, py != 0);

				--this->wCurrentCommandIndex; //revert to current command so it increments correctly for global scripts
			break;

			case CCharacterCommand::CC_VarSet:
			{
				//Sets var X (operation Y) W, e.g. X += 5
				SetVariable(command, pGame, CueEvents);

				//When a var is set, this might get it out of an otherwise infinite loop.
				++wVarSets;
				wTurnCount = 0;

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForVar:
			{
				//Wait until var X (comparison Y) W, e.g. X >= 5
				if (!DoesVarSatisfy(command, pGame))
					STOP_COMMAND;

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_VarSetAt:
			{
				//Remotely set local variable w (with operation h) of NPC at (x,y)
				UINT wX, wY;
				bool success = false;
				getCommandXY(command, wX, wY);

				CMonster* pMonster = room.GetMonsterAtSquare(wX, wY);
				if (pMonster) {
					CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					if (pCharacter) {
						// Transfer var set information to a tempory command.
						CCharacterCommand setCommand;
						setCommand.x = command.w;
						setCommand.y = command.h;
						setCommand.w = command.flags;
						setCommand.label = command.label;
						pCharacter->SetVariable(setCommand, pGame, CueEvents);
						success = true;

						//When a var is set, this might get it out of an otherwise infinite loop.
						++wVarSets;
						wTurnCount = 0;
					}
				}

				if (this->bIfBlock && !success) {
					//As an if condition, query if the command was able to invoke
					//remote variable set.
					STOP_COMMAND;
				}

				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_SetPlayerAppearance:
			{
				if (this->bIfBlock)
				{
					//As an If condition, this acts as a query that is true when
					//the player is in this role.
					if (player.wIdentity != command.x)
						STOP_COMMAND;
				} else {
					//Sets player's identity to entity X.
					pGame->SetPlayerRole(command.x);
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_SetNPCAppearance:
			{
				//Sets this NPC to look like entity X.
				this->wIdentity = this->wLogicalIdentity = command.x;
				ResolveLogicalIdentity(pGame->pHold);
				SetDefaultMovementType();
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_CutScene:
				//Begin cut scene (if X is set), else ends cut scene.
				getCommandX(command, px);
				pGame->dwCutScene = px;
				bProcessNextCommand = true;
				//If ending a cut scene, then wait until next turn to make further moves.
				if (!px)
					bExecuteNoMoveCommands = true;
			break;

			case CCharacterCommand::CC_SetPlayerSword:
			{
				//If X is set, player is given a sword, else it is taken away.
				//(deprecated by Equipment)
				player.bSwordOff = !command.x;
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_BuildTile:
			{
				//Build game element (flags) in rect (x,y,w,h).
				if (!BuildTiles(command, CueEvents))
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForItem:
				//Wait for game element (flags) to exist in rect (x,y,w,h).
				if (!IsTileAt(command, CueEvents))
					STOP_COMMAND;
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_AmbientSound:
				//Play sound with DataID w (0 stops ambient sounds).
				//If h is set, loop indefinitely.
				CueEvents.Add(CID_AmbientSound, new CMoveCoordEx(UINT(-1), UINT(-1), command.w, command.h), true);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_AmbientSoundAt:
				//Play sound with DataID=w (0 stops ambient sounds) at (x,y).
				//If h is set, loop indefinitely.
				getCommandXY(command, px, py);
				CueEvents.Add(CID_AmbientSound, new CMoveCoordEx(px, py, command.w, command.h), true);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_PlayVideo:
				//Play video at (x,y) with DataID=w.
				getCommandXY(command, px, py);
				CueEvents.Add(CID_PlayVideo, new CMoveCoord(px, py, command.w), true);
				bProcessNextCommand = true;
			break;

			//Deprecated commands
			case CCharacterCommand::CC_GotoIf:
			case CCharacterCommand::CC_WaitForCleanRoom:
			case CCharacterCommand::CC_WaitForHalph:
			case CCharacterCommand::CC_WaitForNotHalph:
			case CCharacterCommand::CC_WaitForMonster:
			case CCharacterCommand::CC_WaitForNotMonster:
			case CCharacterCommand::CC_WaitForCharacter:
			case CCharacterCommand::CC_WaitForNotCharacter:
			case CCharacterCommand::CC_WaitForNoBuilding:
				ASSERT(!"Deprecated script command");
			break;

			default: ASSERT(!"Bad CCharacter command"); break;
		}

		if (this->wCurrentCommandIndex < this->commands.size() && command.command != CCharacterCommand::CC_ReplaceWithDefault)
			++this->wCurrentCommandIndex;

		//If MoveRel command was used as an If condition, then reset the relative
		//movement destination for the next relative movement command.
		if (this->bIfBlock)
			this->bMovingRelative = false;

		//Go to jump point if this command executed successfully.
		if (this->wJumpLabel)
		{
			const int wNextIndex = this->bIfBlock ? this->wJumpLabel :
					GetIndexOfCommandWithLabel(this->wJumpLabel);
			if (wNextIndex != NO_LABEL)
				this->wCurrentCommandIndex = wNextIndex;
			this->wJumpLabel = 0;
			this->bIfBlock = false;
		}
		else if (this->bIfBlock) //arriving here indicates If condition failed
			FailedIfCondition();

		//Stop script if more than a certain number of commands have played through
		//on this turn, indicating a probable infinite loop.
		static const UINT VARSET_LIMIT = 1000;
		if (++wTurnCount > this->commands.size() || wVarSets > VARSET_LIMIT)
		{
			this->wCurrentCommandIndex = this->commands.size();
			string str = "Character script is in an infinite loop.  ID = ";
			char temp[16];
			str += _itoa(this->dwScriptID, temp, 10);
			str += NEWLINE;
			CFiles f;
			f.AppendUserLog(str.c_str());
		}
	} while (bProcessNextCommand);
	}

Finish:
	if (this->bVisible)
	{
/*
		if (bIsBeethroDouble(GetResolvedIdentity()))
		{
			//Light any fuse stood on.
 			room.LightFuse(CueEvents, this->wX, this->wY, true);
		}
*/
		//Behavior patterns.
		if (this->bFaceTarget)
			FaceTarget();
		if (this->bFaceAwayFromTarget)
			FaceAwayFromTarget();

		if (this->bSpawnEggs && this->IsSpawnEggTriggered(CueEvents))
			SpawnEgg(CueEvents);

		if (this->bAttackAdjacent && !this->bAttacked)
		{
			this->bAttacked = true; //setting this prevents these methods from being called endlessly when EachAttack is set
			this->bAttacked = AttackPlayerWhenAdjacent(CueEvents);
		}
		if (this->bAttackInFront && !this->bAttacked)
		{
			this->bAttacked = true;
			this->bAttacked = AttackPlayerWhenInFront(CueEvents);
		}
		if (this->bAttackInFrontWhenBackIsTurned && !this->bAttacked)
		{
			this->bAttacked = true;
			this->bAttacked = AttackPlayerInFrontWhenBackIsTurned(CueEvents);
		}
	}

#undef STOP_COMMAND
#undef STOP_DONECOMMAND
}

//*****************************************************************************
void CCharacter::SetVariable(const CCharacterCommand& command, CCurrentGame* pGame, CCueEvents& CueEvents)
{
	const UINT varIndex = command.x;

	//Get variable.
	CDbPackedVars& stats = pGame->stats;
	char varID[10], varName[11] = "v";
	UNPACKEDVARTYPE vType = UVT_int;

	const bool bPredefinedVar = varIndex >= UINT(ScriptVars::FirstPredefinedVar);
	int predefinedVarVal = 0;
	WSTRING predefinedVarValStr;
	bool bValidInt = true;
	if (bPredefinedVar) {
		if (ScriptVars::IsStringVar(ScriptVars::Predefined(varIndex))) {
			predefinedVarValStr = getPredefinedVarString(varIndex);
		} else {
			predefinedVarVal = int(getPredefinedVarInt(varIndex));
		}
	} else {
		//Get local hold var.
		_itoa(varIndex, varID, 10);
		strcat(varName, varID);

		//Enforce basic type checking.
		vType = stats.GetVarType(varName);
		bValidInt = vType == UVT_int || vType == UVT_unknown;
	}

	const bool bSetNumber = !(command.y == ScriptVars::AssignText ||
		command.y == ScriptVars::AppendText);

	int operand = int(command.w); //expect an integer value by default
	if (!operand && !command.label.empty() && bSetNumber)
	{
		//Operand is not just an integer, but a text expression.
		UINT index = 0;
		operand = parseExpression(command.label.c_str(), index, pGame, this);
	}

	int x = 0;

	switch (command.y)
	{
	case ScriptVars::Assign:
		x = operand;
		break;
	case ScriptVars::Inc:
		if (bValidInt)
			x = bPredefinedVar ? predefinedVarVal : stats.GetVar(varName, (int)0);
		addWithClamp(x, operand);
		break;
	case ScriptVars::Dec:
		if (bValidInt)
			x = bPredefinedVar ? predefinedVarVal : stats.GetVar(varName, (int)0);
		addWithClamp(x, -operand);
		break;
	case ScriptVars::MultiplyBy:
		if (bValidInt)
			x = bPredefinedVar ? predefinedVarVal : stats.GetVar(varName, (int)0);
		multWithClamp(x, operand);
		break;
	case ScriptVars::DivideBy:
		if (bValidInt)
			x = bPredefinedVar ? predefinedVarVal : stats.GetVar(varName, (int)0);
		if (operand)
			x /= operand;
		break;
	case ScriptVars::Mod:
		if (bValidInt)
			x = bPredefinedVar ? predefinedVarVal : stats.GetVar(varName, (int)0);
		if (operand)
			x = x % operand;
		break;

	case ScriptVars::AssignText:
	{
		const WSTRING text = pGame->ExpandText(command.label.c_str());
		if (bPredefinedVar)
			setPredefinedVarString(varIndex, text, CueEvents);
		else
			stats.SetVar(varName, text.c_str());
	}
	break;
	case ScriptVars::AppendText:
	{
		WSTRING text = stats.GetVar(varName, wszEmpty);
		text += pGame->ExpandText(command.label.c_str());
		if (bPredefinedVar)
			setPredefinedVarString(varIndex, text, CueEvents);
		else
			stats.SetVar(varName, text.c_str());
	}
	break;
	default: break;
	}
	if (bSetNumber)
	{
		if (bPredefinedVar) {
			setPredefinedVarInt(command.x, x, CueEvents);
		} else {
			stats.SetVar(varName, x);
		}
	}
}

//*****************************************************************************
bool CCharacter::BuildTiles(const CCharacterCommand& command, CCueEvents &CueEvents)
//Build the specified game element (flags) in rect (x,y,w,h).
{
	UINT px, py, pw, ph, pflags;  //command parameters
	getCommandParams(command, px, py, pw, ph, pflags);

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	return BuildUtil::BuildTilesAt(room, pflags, px, py, pw, ph, false, CueEvents);
}

//*****************************************************************************
bool CCharacter::IsTileAt(const CCharacterCommand& command, CCueEvents &CueEvents) const
//Returns: whether the specified game element (flags) is in rect (x,y,w,h).
{
	UINT px, py, pw, ph, pflags;  //command parameters
	getCommandParams(command, px, py, pw, ph, pflags);

	CDbRoom& room = *(this->pCurrentGame->pRoom);

	//Crop check to valid room region
	UINT endX = px + pw;
	UINT endY = py + ph;
	if (!room.CropRegion(px, py, endX, endY))
		return false;

	const UINT tile = pflags;

	if (tile == TV_EXPLOSION)
	{
		//TODO doesn't currently work before processing room turn
		if (CueEvents.HasOccurred(CID_Explosion))
		{
			for (const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_Explosion);
				pObj != NULL; pObj = CueEvents.GetNextPrivateData())
			{
				const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
				if (px <= pCoord->wX && pCoord->wX <= endX &&
						py <= pCoord->wY && pCoord->wY <= endY)
					return true;
			}
		}
		return false;
	}

	const bool bRealTile = IsValidTileNo(tile);
	for (UINT y=py; y <= endY; ++y)
	{
		for (UINT x=px; x <= endX; ++x)
		{
			if (bRealTile)
			{
				switch (TILE_LAYER[tile])
				{
					case 0:  //o-layer
						if (room.GetOSquare(x,y) == tile)
							return true;
					break;
					case 1:  //t-layer
						if (room.GetTSquare(x,y) == tile)
							return true;
					break;
					case 3:  //f-layer
						if (room.GetFSquare(x,y) == tile)
							return true;
					break;
					default: break;
				}
			}
			else if (IsVirtualTile(tile))
			{
				//Virtual tiles must be queried by t-tile and param.
				UINT realTile=T_EMPTY;
				switch (tile)
				{
					case TV_KEY_Y: case TV_KEY_G: case TV_KEY_B: case TV_KEY_S:
						realTile = T_KEY;
					break;
					case TV_EXPLOSION: break; //handled above
					case TV_SWORD1: case TV_SWORD2: case TV_SWORD3: case TV_SWORD4: case TV_SWORD5:
					case TV_SWORD6: case TV_SWORD7: case TV_SWORD8: case TV_SWORD9: case TV_SWORD10:
						realTile = T_SWORD;
					break;
					case TV_SHIELD1: case TV_SHIELD2: case TV_SHIELD3: case TV_SHIELD4: case TV_SHIELD5:
					case TV_SHIELD6:
						realTile = T_SHIELD;
					break;
					case TV_ACCESSORY1: case TV_ACCESSORY2: case TV_ACCESSORY3: case TV_ACCESSORY4:
					case TV_ACCESSORY5: case TV_ACCESSORY6: case TV_ACCESSORY7: case TV_ACCESSORY8:
					case TV_ACCESSORY9: case TV_ACCESSORY10: case TV_ACCESSORY11: case TV_ACCESSORY12:
						realTile = T_ACCESSORY;
					break;
					default: break;
				}
				if (realTile != room.GetTSquare(x,y))
					continue;

				UINT param = UINT(-1);
				switch (tile) //2nd pass -- query T-layer param
				{
					case TV_KEY_Y: param = YellowKey; break;
					case TV_KEY_G: param = GreenKey; break;
					case TV_KEY_B: param = BlueKey; break;
					case TV_KEY_S: param = SkeletonKey; break;
					case TV_SWORD1: param = WoodenBlade; break;
					case TV_SWORD2: param = ShortSword; break;
					case TV_SWORD3: param = GoblinSword; break;
					case TV_SWORD4: param = LongSword; break;
					case TV_SWORD5: param = HookSword; break;
					case TV_SWORD6: param = ReallyBigSword; break;
					case TV_SWORD7: param = LuckySword; break;
					case TV_SWORD8: param = SerpentSword; break;
					case TV_SWORD9: param = BriarSword; break;
					case TV_SWORD10: param = WeaponSlot; break;
					case TV_SHIELD1: param = WoodenShield; break;
					case TV_SHIELD2: param = BronzeShield; break;
					case TV_SHIELD3: param = SteelShield; break;
					case TV_SHIELD4: param = KiteShield; break;
					case TV_SHIELD5: param = OremiteShield; break;
					case TV_SHIELD6: param = ArmorSlot; break;
					case TV_ACCESSORY1: param = GrapplingHook; break;
					case TV_ACCESSORY2: param = WaterBoots; break;
					case TV_ACCESSORY3: param = InvisibilityPotion; break;
					case TV_ACCESSORY4: param = SpeedPotion; break;
					case TV_ACCESSORY5: param = HandBomb; break;
					case TV_ACCESSORY6: param = PickAxe; break;
					case TV_ACCESSORY7: param = WarpToken; break;
					case TV_ACCESSORY8: param = PortableOrb; break;
					case TV_ACCESSORY9: param = LuckyGold; break;
					case TV_ACCESSORY10: param = WallWalking; break;
					case TV_ACCESSORY11: param = XPDoubler; break;
					case TV_ACCESSORY12: param = AccessorySlot; break;
					default: break; //nothing else to do here
				}
				if (param == room.GetTParam(x,y))
					return true;
			}
		}
	}

	return false;
}

//******************************************************************************************
bool CCharacter::DoesVarSatisfy(const CCharacterCommand& command, CCurrentGame* pGame)
{
	const UINT varIndex = command.x;

	//Get variable.
	CDbPackedVars& stats = pGame->stats;
	char varID[10], varName[11] = "v";
	UNPACKEDVARTYPE vType = UVT_int;

	const bool bPredefinedVar = varIndex >= UINT(ScriptVars::FirstPredefinedVar);
	bool bValidInt = true;
	if (!bPredefinedVar)
	{
		//Get local hold var.
		_itoa(varIndex, varID, 10);
		strcat(varName, varID);

		//Enforce basic type checking.
		vType = stats.GetVarType(varName);
		bValidInt = vType == UVT_int || vType == UVT_unknown;
	}

	int operand = int(command.w); //expect an integer value by default
	if (!operand && !command.label.empty() && command.y != ScriptVars::EqualsText)
	{
		//Operand is not just an integer, but a text expression.
		UINT index = 0;
		operand = parseExpression(command.label.c_str(), index, pGame, this);
	}

	int x = 0;
	const bool bNumber = bValidInt && command.y != ScriptVars::EqualsText;
	if (bNumber) {
		if (bPredefinedVar) {
			x = int(getPredefinedVarInt(varIndex));
		} else {
			x = stats.GetVar(varName, (int)0);
		}
	}

	switch (command.y)
	{
		case ScriptVars::Equals: return x == operand;
		case ScriptVars::Greater: return x > operand;
		case ScriptVars::GreaterThanOrEqual: return x >= operand;
		case ScriptVars::Less: return x < operand;
		case ScriptVars::LessThanOrEqual: return x <= operand;
		case ScriptVars::Inequal: return x != operand;
		case ScriptVars::EqualsText:
		{
			WSTRING wStr;
			if (vType == UVT_wchar_string)
				wStr = stats.GetVar(varName, wszEmpty);
			const WSTRING operand = pGame->ExpandText(command.label.c_str(), this);
			return wStr == operand;
		}
		break;
		default: break;
	}
	ASSERT(!"Unrecognized var operator");
	return false;
}

//*****************************************************************************
void CCharacter::CheckForCueEvent(CCueEvents &CueEvents) //(in)
//Called once all cue events have been gathered.
//If the current command is waiting for a cue event, satisfying this
//will continue to the next command on the following turn.
{
	if (!this->bWaitingForCueEvent)
		return;
	ASSERT(this->wCurrentCommandIndex < this->commands.size());
	CCharacterCommand& command = this->commands[this->wCurrentCommandIndex];
	ASSERT(command.command == CCharacterCommand::CC_WaitForCueEvent);

	//Wait for cue event X to fire.
	const CUEEVENT_ID cid = static_cast<CUEEVENT_ID>(command.x);
	if (CueEvents.HasOccurred(cid))
	{
		++this->wCurrentCommandIndex;
		this->bWaitingForCueEvent = false; //no longer waiting for the event
	}
}

//*****************************************************************************
bool CCharacter::CheckForDamage(CCueEvents& CueEvents)
//Returns: whether monster was killed by room-inflicted damage
{
	if (!IsVisible())
		return false;  //NPC not in room

	bool bRes = CMonster::CheckForDamage(CueEvents);
	if (bRes && !this->bVulnerable)
	{
		//Unkillable NPC is defeated.
		Defeat();
		return false;
	}

	//Check for Metal on oremites.
	if (!IsFlying()) //airborne monsters are not damaged by oremites
	{
		//Damage to metal monster on oremites is the same as on hot tiles.
		if (IsMetal() && this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_GOO)
		{
			if (IsDamageableAt(this->wX, this->wY))
			{
				CueEvents.Add(CID_MonsterBurned, this);
				return Damage(CueEvents, int(this->pCurrentGame->pPlayer->st.hotTileVal));
			}
		}
	}

	return bRes;
}

//*****************************************************************************
bool CCharacter::ProcessAfterAttack(CCueEvents &CueEvents)
//Each time I attack, go to the specified label and execute a script turn.
//This routine may be executed either by an NPC attacking the player
//(this may happen outside of combat), or by the player's custom weapon.
//
//Returns: whether combat should continue
{
	if (this->eachAttackLabelIndex != NO_LABEL)
	{
		const bool bCombat = this->pCurrentGame->pCombat != NULL;
/*
		if (bCombat)
		{
			//Update my HP based on this attack round.
			const int newHP = this->pCurrentGame->pCombat->monHP;
			this->pCurrentGame->pCombat->pMonster->HP = newHP > 0 ? newHP : 0; //avoid negative values
		}
*/

		//Allow these script commands to run, even if normal script execution
		//is disabled during combat.
		const bool bExecuteScriptOnCombat_temp = this->bExecuteScriptOnCombat;
		this->bExecuteScriptOnCombat = true;

		//Goto this label and process special commands.
		this->wCurrentCommandIndex = this->eachAttackLabelIndex+1; //start past label
		const CCoord oldPos(*this);
		Process(CMD_ADVANCE_COMBAT, CueEvents);

		this->bExecuteScriptOnCombat = bExecuteScriptOnCombat_temp; //revert

		//Synch combat stats with any changes due to script execution.
		if (bCombat)
			this->pCurrentGame->pCombat->InitMonsterStats(false);

		//If monster is no longer visible or has moved, then interrupt combat.
		return IsVisible() && oldPos.wX == this->wX && oldPos.wY == this->wY;
	}

	return true;
}

//*****************************************************************************
void CCharacter::ProcessAfterDefeat(CCueEvents &CueEvents)
//When the NPC is defeated, but not killed, this is invoked to execute commands
//dealing only with the NPC's defeat immediately.
{
	if (this->wCurrentCommandIndex >= this->commands.size())
		return; //nothing to execute
	CCharacterCommand& command = this->commands[this->wCurrentCommandIndex];
	if (command.command == CCharacterCommand::CC_WaitForDefeat)
		Process(CMD_ADVANCE_COMBAT, CueEvents);
}

//*****************************************************************************
bool CCharacter::ProcessAfterDefend(CCueEvents &CueEvents)
//Each time I defend, go to the specified label and execute a script turn.
//This may happen either each time the player hits me once during combat,
//or also when functioning as the player's custom armor when a monster hits the player.
//
//Returns: whether combat should continue (not considering whether the monster is dead)
{
	if (this->eachDefendLabelIndex != NO_LABEL)
	{
		const bool bCombat = this->pCurrentGame->pCombat != NULL;
/*
		if (bCombat)
		{
			//Update my HP based on this attack round.
			const int newHP = this->pCurrentGame->pCombat->monHP;
			this->pCurrentGame->pCombat->pMonster->HP = newHP > 0 ? newHP : 0; //avoid negative values
		}
*/

		//Allow these script commands to run, even if normal script execution
		//is disabled during combat.
		const bool bExecuteScriptOnCombat_temp = this->bExecuteScriptOnCombat;
		this->bExecuteScriptOnCombat = true;

		//Goto this label and process special commands.
		this->wCurrentCommandIndex = this->eachDefendLabelIndex+1; //start past label
		const CCoord oldPos(*this);
		Process(CMD_ADVANCE_COMBAT, CueEvents);

		this->bExecuteScriptOnCombat = bExecuteScriptOnCombat_temp; //revert

		//Synch combat stats with any changes due to script execution.
		if (bCombat)
			this->pCurrentGame->pCombat->InitMonsterStats(false);

		//If monster is no longer visible or has moved, then interrupt combat.
		return IsVisible() && oldPos.wX == this->wX && oldPos.wY == this->wY;
	}

	return true;
}

//*****************************************************************************
bool CCharacter::ProcessAfterUse(CCueEvents &CueEvents)
//Each time I am used (i.e. the player activates me) as an inventory item,
//go to the specified label and execute a script turn.
//
//Returns: true
{
	if (this->eachUseLabelIndex != NO_LABEL)
	{
		//Goto this label and process commands.
		this->wCurrentCommandIndex = this->eachUseLabelIndex+1; //start past label
		Process(CMD_WAIT, CueEvents);
	}

	return true;
}

//*****************************************************************************
void CCharacter::ProcessAfterVictory(CCueEvents& CueEvents)
//Go to the specified label and execute a script turn.
{
	if (this->eachVictoryLabelIndex != NO_LABEL)
	{
		//Goto this label and process commands.
		this->wCurrentCommandIndex = this->eachVictoryLabelIndex + 1; //start past label
		Process(CMD_WAIT, CueEvents);
	}
}

//*****************************************************************************
void CCharacter::FailedIfCondition()
//An if condition failed.  Move command execution pointer past the if block. 
{
	ASSERT(this->bIfBlock);

	this->bIfBlock = false;
	this->bParseIfElseAsCondition = false;

	//Scan until the end of the If block is encountered.
	//This could be indicated by either an IfElse or IfEnd command.
	UINT wNestingDepth = 0;
	const bool wSkipToIfEnd = this->wCurrentCommandIndex > 0 ? this->commands[this->wCurrentCommandIndex - 1].command == CCharacterCommand::CC_IfElseIf : false;
	bool bScanning = true;
	do
	{
		if (this->wCurrentCommandIndex >= this->commands.size())
			return; //block continued to the end of the script

		CCharacterCommand& command = this->commands[this->wCurrentCommandIndex];
		switch (command.command)
		{
			case CCharacterCommand::CC_If:
				++wNestingDepth;  //entering a nested If block
			break;
			case CCharacterCommand::CC_IfElse:
				if (!wSkipToIfEnd && wNestingDepth == 0)
					bScanning = false;  //found the If command's matching Else block
			break;
			case CCharacterCommand::CC_IfElseIf:
				if (!wSkipToIfEnd && wNestingDepth == 0) {
					bScanning = false;
					this->bIfBlock = true;
					this->wJumpLabel = this->wCurrentCommandIndex + 2;
				}
				break;
			case CCharacterCommand::CC_IfEnd:
				if (wNestingDepth-- == 0)	//exiting an If block
					bScanning = false;  //found the end of the If block (no Else was found in between)
			break;
			default: break;
		}
		++this->wCurrentCommandIndex;
	} while (bScanning);
}

//*****************************************************************************
UINT CCharacter::GetResolvedIdentity() const
//Returns: what identity the NPC should take.
//  Call this method instead of GetIdentity to fully resolve what an NPC should look like.
{
	UINT wIdentity = GetIdentity();
	return wIdentity;
}

UINT CCharacter::GetSpawnType(UINT defaultMonsterID) const
{
	if (this->wSpawnType >= 0) {
		return this->wSpawnType;
	}

	return CMonster::GetSpawnType(defaultMonsterID);
}

//*****************************************************************************
float CCharacter::GetStatModifier(ScriptVars::StatModifiers statType) const
//Returns: the NPC's local modifier value for the given type
{
	// When calculating, treat values that can be negative as signed int
	switch (statType) {
		case ScriptVars::StatModifiers::MonsterHP:
			return this->monsterHPmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterATK:
			return this->monsterATKmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterDEF:
			return this->monsterDEFmult / 100.0f;
		case ScriptVars::StatModifiers::MonsterGR:
			return int(this->monsterGRmult) / 100.0f;
		case ScriptVars::StatModifiers::MonsterXP:
			return int(this->monsterXPmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemAll:
			return int(this->itemMult) / 100.0f;
		case ScriptVars::StatModifiers::ItemHP:
			return int(this->itemHPmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemATK:
			return int(this->itemATKmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemDEF:
			return int(this->itemDEFmult) / 100.0f;
		case ScriptVars::StatModifiers::ItemGR:
			return int(this->itemGRmult) / 100.0f;
		default:
			return 1.0f;
	}

	return 1.0f;
}

//*****************************************************************************
bool CCharacter::IsDamageableAt(const UINT /*wX*/, const UINT /*wY*/) const
//Returns: whether player may attack this character
{
	return IsCombatable();
}

//*****************************************************************************
bool CCharacter::IsFriendly() const
//Returns: whether character is friendly to the player
{
//	const UINT identity = GetResolvedIdentity();
	return //identity == M_HALPH || identity == M_STALWART ||
			this->bSafeToPlayer;
}

//*****************************************************************************
bool CCharacter::OnAnswer(
//Overridable method for responding to an answer given by player to a question
//asked by the monster.
//
//Params:
	int nCommand,        //(in)   CMD_YES or CMD_NO, or line # of selected AnswerOption.
	CCueEvents &/*CueEvents*/)  //(out)  Add cue events if appropriate.
//
//Returns:
//True if any cue events were added, false if not.
{
	if (this->bYesNoQuestion)
	{
		//Primitive yes/no answer given.
		if (this->bIfBlock)
		{
			if (nCommand != CMD_YES) {
				FailedIfCondition(); //skip if block
				// If we are entering else if, make sure we set proper variables for it to be handled correctly
				if (this->wCurrentCommandIndex > 0 ? this->commands[this->wCurrentCommandIndex - 1].command == CCharacterCommand::CC_IfElseIf : false)
				{
					--this->wCurrentCommandIndex;
					this->bIfBlock = false;
					this->wJumpLabel = 0;
					this->bParseIfElseAsCondition = true;
				}
			}
			else
			{
				//Enter if block.
				if (this->wJumpLabel)
				{
					const int wNextIndex = this->bIfBlock ? this->wJumpLabel :
							GetIndexOfCommandWithLabel(this->wJumpLabel);
					if (wNextIndex != NO_LABEL)
						this->wCurrentCommandIndex = wNextIndex;
					this->wJumpLabel = 0;
				}
			}
		}
		//else the question was not asked as a conditional
	} else {
		//Answer selected from a set.
		ASSERT(!this->bIfBlock);
		ASSERT(nCommand >= 0);
		ASSERT((UINT)nCommand < this->commands.size());
		const int wNextIndex = GetIndexOfCommandWithLabel(nCommand);
		if (wNextIndex != NO_LABEL)
			this->wCurrentCommandIndex = wNextIndex;

		this->answerOptions.clear(); //reset answer set for next question
	}

	return false;
}

//*****************************************************************************
#define STARTVPTAG(vpType,pType) "<"; str += ViewpropTypeStr(vpType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDVPTAG(vpType) "</"; str += ViewpropTypeStr(vpType); str += ">" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

//*****************************************************************************
void CCharacter::ExportText(CDbRefs &dbRefs, CStretchyBuffer& str)
//No texts to export here, but owned speech texts are exported.
{
	bool bContainsSpeechText = false;
	
	//Export speech texts.
	char dummy[32];
	const UINT wNumCommands = this->commands.size();
	for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
	{
		CCharacterCommand& command = this->commands[wIndex];
		if (command.pSpeech)
		{
			if (!bContainsSpeechText)
			{
				//Output character ID first time relevant data are encountered.
				bContainsSpeechText = true;

				str += STARTVPTAG(VP_Monsters, P_X);
				str += INT32TOSTR(this->wX);
				str += PROPTAG(P_Y);
				str += INT32TOSTR(this->wY);
				str += CLOSESTARTTAG;
			}

			const UINT dwSpeechID = command.pSpeech->dwSpeechID;
			g_pTheDB->Speech.ExportText(dwSpeechID, dbRefs, str);
		}
	}

	if (bContainsSpeechText)
	{
		str += ENDVPTAG(VP_Monsters);
	}
}
#undef STARTVPTAG
#undef PROPTAG
#undef ENDVPTAG
#undef CLOSESTARTTAG
#undef INT32TOSTR

//*****************************************************************************
string CCharacter::ExportXMLSpeech(
//Returns: string containing XML text describing character with this ID
//
//Params:
	CDbRefs &dbRefs,       //(in/out)
	const COMMAND_VECTOR& commands, //(in)
	const bool bRef) //Only export GUID references [default=false]
{
	string str;

	const UINT wNumCommands = commands.size();
	for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
	{
		const CCharacterCommand& command = commands[wIndex];

		//Export character-owned speech records.
		if (command.pSpeech && command.pSpeech->dwSpeechID)
			g_pTheDB->Speech.ExportXML(command.pSpeech->dwSpeechID, dbRefs, str, bRef);

		//Export character-owned sound effects.
		if (bCommandHasData(command.command) && command.w && g_pTheDB->Data.Exists(command.w))
			g_pTheDB->Data.ExportXML(command.w, dbRefs, str, bRef);
	}

	return str;
}

//*****************************************************************************
UINT CCharacter::GetNextSpeechID()
//Returns: speechID of next speech record referenced in character script,
//or 0 if there are no more
{
	while (this->wLastSpeechLineNumber < this->commands.size())
	{
		CCharacterCommand& command = this->commands[this->wLastSpeechLineNumber++];
		if (command.pSpeech)
			return command.pSpeech->dwSpeechID;
	}

	return 0; //no more
}

//*****************************************************************************
MESSAGE_ID CCharacter::ImportSpeech(
//Updates speech and data IDs in script commands.
//
//Params:
	CImportInfo &info)   //(in/out) Import data
{
	// Set member vars to use in repopulating ExtraVars from scratch below
	setBaseMembers(this->ExtraVars);

	this->dwScriptID = this->ExtraVars.GetVar(scriptIDstr, UINT(0));

	UINT dwSpeechID, dwDataID, eCommand;
	const UINT wNumCommands = this->ExtraVars.GetVar(numCommandsStr, UINT(0));
	PrimaryKeyMap::iterator localID;

	BYTE *commandBuffer = (BYTE*)this->ExtraVars.GetVar(commandStr, (const void*)(NULL));
	if (commandBuffer)
	{
		this->commands.reserve(wNumCommands);

		this->wCurrentCommandIndex = this->ExtraVars.GetVar(startLineStr, UINT(0));
		ASSERT(this->wCurrentCommandIndex <= wNumCommands);

		//Current script data format.
		const UINT bufferSize = this->ExtraVars.GetVarValueSize(commandStr);

		UINT index = 0;
		while (index < bufferSize)
		{
			CCharacterCommand command;

			eCommand = readBpUINT(commandBuffer, index);
			command.command = CCharacterCommand::CharCommand(eCommand);
			command.x = readBpUINT(commandBuffer, index);
			command.y = readBpUINT(commandBuffer, index);
			dwDataID = readBpUINT(commandBuffer, index); // set this later
			command.h = readBpUINT(commandBuffer, index);
			command.flags = readBpUINT(commandBuffer, index);
			dwSpeechID = readBpUINT(commandBuffer, index); // set this later

			const UINT labelSize = readBpUINT(commandBuffer, index);
			if (labelSize)
			{
				const UINT wchars = labelSize/sizeof(WCHAR);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
				WCHAR *pwzBuf = new WCHAR[wchars+1];
				memcpy((void*)pwzBuf, (const void*)(commandBuffer + index), labelSize);
				LittleToBig(pwzBuf, wchars);
				command.label.assign(pwzBuf, wchars);
				delete[] pwzBuf;
#else
				command.label.assign((const WCHAR*)(commandBuffer + index), wchars);
#endif
				index += labelSize;
			}

			if (bCommandHasData(eCommand))
			{
				if (dwDataID)
				{
					//If data is not present, simply reset this field.
					localID = info.DataIDMap.find(dwDataID);
					dwDataID = localID != info.DataIDMap.end() ? localID->second : 0;
				}
			}
			command.w = dwDataID;

			if (dwSpeechID)
			{
				localID = info.SpeechIDMap.find(dwSpeechID);
				if (localID == info.SpeechIDMap.end())
				{
					//looks like a dangling pointer, but don't fail import because of it
					dwSpeechID = 0;
				} else {
					dwSpeechID = localID->second;
					command.pSpeech = g_pTheDB->Speech.GetByID(dwSpeechID);
					ASSERT(command.pSpeech);
				}
			}

			this->commands.push_back(command);
		}
		ASSERT(index == bufferSize);
		ASSERT(this->commands.size() == wNumCommands);
	}

	//Reset ExtraVars to repopulate from scratch with re-mapped script commands
	SetExtraVarsFromMembersWithoutScript(this->ExtraVars);

	//Save command sequence (script) into packed vars member.
	SaveCommands(this->ExtraVars, this->commands);

	return MID_ImportSuccessful;
}

//******************************************************************************************
bool CCharacter::DoesSquareContainObstacle(
//Override for characters.  Parts copied from CMimic::DoesSquareContainObstacle.
//
//Params:
	const UINT wCol, const UINT wRow) //(in) Coords of square to evaluate.
const
{
	//Can't move onto player if "safe" flag is set.
	if (this->bSafeToPlayer && this->pCurrentGame->IsPlayerAt(wCol, wRow))
		return true;
	
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow))
		return true;

	//Routine is not written to check the square on which this monster is 
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	UINT wLookTileNo;

	//Check o-layer for obstacle.
	wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		switch (wLookTileNo)
		{
			//If standing on a platform, check whether it can move.
			case T_PIT: case T_PIT_IMAGE:
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_P && this->bPushObjects)
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			case T_WATER:
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_W && this->bPushObjects)
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			default:	return true;
		}
	}

	//Check t-layer for obstacle.
	wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		switch (wLookTileNo)
		{
			case T_MIRROR:
			case T_CRATE:
				if (this->bPushObjects)
				{
					const int dx = (int)wCol - (int)this->wX;
					const int dy = (int)wRow - (int)this->wY;
					if (room.CanPushTo(wCol, wRow, wCol + dx, wRow + dy))
						break; //not an obstacle
				}
			//NO BREAK
			default: return true;
		}
	}

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check for monster at square.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster)
		return true;

	if (!this->bMoveIntoSwords)
	{
		//Check for player's sword at square.
		if (this->pCurrentGame->IsPlayerSwordAt(wCol, wRow))
			return true;

		//Check for monster sword at square.
		if (room.IsMonsterSwordAt(wCol, wRow, this))
			return true;
	}

	//No obstacle.
	return false;
}

//*****************************************************************************
UINT CCharacter::getATK() const
//Return: monster's ATK
//
//NOTE: we allow returning a negative value for equipment modifiers
{
	if (this->equipType == ScriptFlag::NotEquipment)
		return CPlayerDouble::getATK(); //the usual calculation

	//Equipment: monster ATK/DEF multipliers do not affect equipment stats
	return this->ATK;
}

//*****************************************************************************
UINT CCharacter::getColor() const
//Return: monster's color
{
	if (this->color)
		return this->color;

	return CPlayerDouble::getColor();
}

//*****************************************************************************
UINT CCharacter::getDEF() const
//Return: monster's DEF
//
//NOTE: we allow returning a negative value for equipment modifiers
{
	if (this->equipType == ScriptFlag::NotEquipment)
		return CPlayerDouble::getDEF(); //the usual calculation

	//Equipment: monster ATK/DEF multipliers do not affect equipment stats
	return this->DEF;
}

//*****************************************************************************
UINT CCharacter::getSword() const
//Return: monster's custom weapon type
{
	if (this->sword)
		return this->sword;

	return CPlayerDouble::getSword();
}

//*****************************************************************************
void CCharacter::getCommandParams(
//Outputs: the parameter values for this command
	const CCharacterCommand& command,
	UINT& x, UINT& y, UINT& w, UINT& h, UINT& f)
const
{
	x = (this->paramX == NO_OVERRIDE ? command.x : this->paramX);
	y = (this->paramY == NO_OVERRIDE ? command.y : this->paramY);
	w = (this->paramW == NO_OVERRIDE ? command.w : this->paramW);
	h = (this->paramH == NO_OVERRIDE ? command.h : this->paramH);
	f = (this->paramF == NO_OVERRIDE ? command.flags : this->paramF);
}

//*****************************************************************************
void CCharacter::getCommandRect(
//Outputs: the parameter values for this command rect
	const CCharacterCommand& command,
	UINT& x, UINT& y, UINT& w, UINT& h)
const
{
	x = (this->paramX == NO_OVERRIDE ? command.x : this->paramX);
	y = (this->paramY == NO_OVERRIDE ? command.y : this->paramY);
	w = (this->paramW == NO_OVERRIDE ? command.w : this->paramW);
	h = (this->paramH == NO_OVERRIDE ? command.h : this->paramH);
}

//*****************************************************************************
void CCharacter::getCommandX(
//Outputs: the X value for this command
	const CCharacterCommand& command, UINT& x)
const
{
	x = (this->paramX == NO_OVERRIDE ? command.x : this->paramX);
}

//*****************************************************************************
void CCharacter::getCommandXY(
//Outputs: the XY values for this command
	const CCharacterCommand& command, UINT& x, UINT& y)
const
{
	x = (this->paramX == NO_OVERRIDE ? command.x : this->paramX);
	y = (this->paramY == NO_OVERRIDE ? command.y : this->paramY);
}

//*****************************************************************************
bool CCharacter::IsCombatable() const
//Returns: whether the player may fight this monster
{
	if (!IsVisible())
		return false; //can't fight NPC if it has left the room (during combat, say)

	if (IsSafeToPlayer())
		return false; //can't fight safe (friendly) NPCs

	//If this NPC has HP, then the player may fight it.
	//It is not necessary to have non-zero ATK, DEF, or GOLD in this case.
	//
	//Any "defeated" NPC should have 0 HP.
	//To make a defeated NPC combatable again, its HP stat must be set again.
	ASSERT(!this->bDefeated || !getHP());
	return getHP() > 0;
}

//*****************************************************************************
bool CCharacter::IsTileObstacle(
//Override for NPCs.
//
//Params:
	const UINT wTileNo) //(in)   Tile to evaluate.  Note each tile# will always be
						//    found on the same layer of squares.
//
//Returns:
//True if tile is an obstacle, false if not.
const
{
	//All the things characters can step on
	bool bIsObstacle = !(wTileNo == T_EMPTY ||
		bIsFloor(wTileNo) ||
		bIsOpenDoor(wTileNo) ||
		bIsArrow(wTileNo) ||
		bIsPlatform(wTileNo) ||
		wTileNo == T_NODIAGONAL ||
		wTileNo == T_SCROLL ||
		wTileNo == T_FUSE ||
		wTileNo == T_TOKEN ||
		wTileNo == T_KEY ||
		bIsPowerUp(wTileNo) ||
		bIsEquipment(wTileNo) ||
		bIsStairs(wTileNo) ||
		bIsTunnel(wTileNo)
	);

	switch (eMovement)
	{
		//These types can move through wall.
		//NOTE: For greater scripting flexibility, these types will also be allowed
		//to perform normal movement.
		case MovementType::WALL:
		{
			return bIsObstacle &&
				!(bIsWall(wTileNo) || bIsCrumblyWall(wTileNo) || bIsDoor(wTileNo));
			//i.e. tile is considered an obstacle only when it blocks both movement types
		}

		//Flying types may also move over pits.
		case MovementType::AIR:
			return bIsObstacle &&
					!bIsWater(wTileNo) && !bIsPit(wTileNo);

		case MovementType::WATER:
			return bIsObstacle && !bIsWater(wTileNo);

		default:	return CMonster::IsTileObstacle(wTileNo);
	}
}

//*****************************************************************************
const CCharacterCommand* CCharacter::GetCommandWithLabel(const UINT label) const
//Returns: pointer to command with specified label, or NULL if none
{
	ASSERT(label);
	for (UINT wIndex=this->commands.size(); wIndex--; )
	{
		const CCharacterCommand& command = this->commands[wIndex];
		if (command.command == CCharacterCommand::CC_Label &&
				label == command.x)
			return &command;
	}
	return NULL;
}

//*****************************************************************************
void CCharacter::LoadCommands(const CDbPackedVars& ExtraVars, COMMAND_VECTOR& commands)
{
	commands.clear();
	const UINT wNumCommands = ExtraVars.GetVar(numCommandsStr, UINT(0));
	if (!wNumCommands)
		return;

	commands.reserve(wNumCommands);

	BYTE *commandBuffer = (BYTE*)ExtraVars.GetVar(commandStr, (const void*)(NULL));
	if (commandBuffer)
	{
		const UINT bufferSize = ExtraVars.GetVarValueSize(commandStr);
		DeserializeCommands(commandBuffer, bufferSize, commands);
		ASSERT(commands.size() == wNumCommands);
	} else {
		//Pre-3.1 script data encapsulation format:
		ASSERT(!"Should be obsolete and not encountered anywhere");
	}
}

//*****************************************************************************
void CCharacter::LoadCommands(const CDbPackedVars& ExtraVars, COMMANDPTR_VECTOR& commands)
//Overloaded method with vector of pointers to commands.
{
	//Delete commands pointed to.
	COMMANDPTR_VECTOR::iterator command;
	for (command = commands.begin(); command != commands.end(); ++command)
		delete *command;
	commands.clear();

	const UINT wNumCommands = ExtraVars.GetVar(numCommandsStr, UINT(0));
	if (!wNumCommands)
		return;

	commands.reserve(wNumCommands);

	BYTE *commandBuffer = (BYTE*)ExtraVars.GetVar(commandStr, (const void*)(NULL));
	if (commandBuffer)
	{
		const UINT bufferSize = ExtraVars.GetVarValueSize(commandStr);
		DeserializeCommands(commandBuffer, bufferSize, commands);
		ASSERT(commands.size() == wNumCommands);
	} else {
		//Pre-3.1 script data encapsulation format:
		ASSERT(!"Should be obsolete and not encountered anywhere");
	}
}

//*****************************************************************************
void CCharacter::ResolveLogicalIdentity(CDbHold *pHold)
//Determines semantic identity from logical identity ID.
{
	if (this->wLogicalIdentity >= CHARACTER_TYPES)
	{
		if (this->wLogicalIdentity >= CUSTOM_CHARACTER_FIRST && this->wLogicalIdentity != M_NONE && pHold)
		{
			//Keep reference to custom info.
			this->pCustomChar = pHold->GetCharacter(this->wLogicalIdentity);

			//Show character with designated identity.
			if (this->pCustomChar)
			{
				this->wIdentity = this->pCustomChar->wType;
			}
			else
				//When character has a dangling reference to a custom character definition
				//then default the character's appearance to a citizen.
				//(This could happen if the character was copied to a different hold.)
				this->wIdentity = M_CITIZEN1;
		}

		//Override visibility for non-graphic IDs.
		if (this->wIdentity >= CHARACTER_TYPES && IsVisible())
		{
			if (this->pCurrentGame)
				Disappear();
			else
				this->bVisible = false;
		}
	}
}

//*****************************************************************************
void CCharacter::RestartScript()
//Restart the executing script from the first command.
//Reset the vars that involve currently executing script commands.
{
	//These vars are all reset to their values on object construction.
	this->wCurrentCommandIndex = 0;

	this->wTurnDelay = 0;
	this->wXRel = this->wYRel = 0;
	this->bMovingRelative = false;
	this->wExitingRoomO = NO_ORIENTATION;

	this->eachAttackLabelIndex = this->eachDefendLabelIndex = this->eachUseLabelIndex = NO_LABEL;
	this->eachVictoryLabelIndex = NO_LABEL;

	this->bVulnerable = true;
	this->bMissionCritical = false;
	this->bSafeToPlayer = false;
	this->bSwordSafeToPlayer = false;
	this->bDefeated = false;
	this->bShowStatChanges = true;
	this->bGhostImage = false;
	this->bRestartScriptOnRoomEntrance = false;
//	this->bGlobal = false; //this flag doesn't get reset -- once a script's in the global list, it stays there
	this->bExecuteScriptOnCombat = true;

	this->bAttackAdjacent =
	this->bAttackInFront =
	this->bAttackInFrontWhenBackIsTurned =
	this->bFaceTarget =
	this->bHasRayGun =
	this->bHasRayBlocking =
	this->bSurprisedFromBehind = false;
	this->movementIQ = SmartDiagonalOnly;

	this->HP = 0;
	this->ATK = 0;
	this->DEF = 0;
	this->GOLD = 0;
	this->XP = 0;
	this->color = 0;
	this->sword = NPC_DEFAULT_SWORD;

	//These values are reloaded as they were on setup.
	this->wLogicalIdentity = this->ExtraVars.GetVar(idStr, this->wLogicalIdentity);
	this->bVisible = this->ExtraVars.GetVar(visibleStr, this->bVisible);
}

//*****************************************************************************
void CCharacter::SetCurrentGame(
//Sets current game pointer for monster.
//This is necessary for many methods of the monster class to work.
//
//Params:
	const CCurrentGame *pSetCurrentGame) //(in)
{
	ASSERT(pSetCurrentGame);
	if (this->pCurrentGame != pSetCurrentGame)
	{
		ASSERT(!this->pCurrentGame);
		this->pCurrentGame = pSetCurrentGame;
	}

	//CCharacter should not have its base stats set by a call to CMonster::SetCurrentGame,
	//as these values all default to zero.

	//Check for a custom character.
	ResolveLogicalIdentity(this->pCurrentGame ? this->pCurrentGame->pHold : NULL);

	//Set the movement type
	if(bIsFirstTurn)
		SetDefaultMovementType();

	//If this NPC is a custom character with no script,
	//then use the default script for this custom character type.
	if (this->pCustomChar && this->commands.empty())
		LoadCommands(this->pCustomChar->ExtraVars, this->commands);
}

//*****************************************************************************
void CCharacter::SetDefaultMovementType()
//Sets the character's eMovement to the appropriate type for its identity
{
	switch (GetResolvedIdentity())
	{
		//These types can move through wall.
		case M_SEEP:
			eMovement = MovementType::WALL;
		break;

		//Flying types may also move over pits.
		case M_WWING: case M_FEGUNDO:
			eMovement = MovementType::AIR;
		break;

		case M_WATERSKIPPER:
		case M_SKIPPERNEST:
			eMovement = MovementType::WATER;
		break;

		default:
			eMovement = MovementType::GROUND;
		break;
	}
}

//*****************************************************************************
void CCharacter::SaveCommands(CDbPackedVars& ExtraVars, const COMMAND_VECTOR& commands)
{
	//First, save speech records owned by script commands to DB.
	SaveSpeech(commands);

	const UINT wNumCommands = commands.size();
	if (wNumCommands)
		ExtraVars.SetVar(numCommandsStr, wNumCommands);

	//Serialize commands into a single buffer.
	string buffer;
	SerializeCommands(buffer, commands);
	if (!buffer.empty())
		ExtraVars.SetVar(commandStr, (void*)(buffer.c_str()), buffer.size(), UVT_byte_buffer);
}

//*****************************************************************************
void CCharacter::SaveCommands(CDbPackedVars& ExtraVars, const COMMANDPTR_VECTOR& commands)
{
	//First, save speech records owned by script commands to DB.
	SaveSpeech(commands);

	const UINT wNumCommands = commands.size();
	if (wNumCommands)
		ExtraVars.SetVar(numCommandsStr, wNumCommands);

	//Serialize commands into a single buffer.
	string buffer;
	SerializeCommands(buffer, commands);
	if (!buffer.empty())
		ExtraVars.SetVar(commandStr, (void*)(buffer.c_str()), buffer.size(), UVT_byte_buffer);
}

//********************* Current serialization ********************************/

//*****************************************************************************
UINT CCharacter::readBpUINT(const BYTE* buffer, UINT& index)
//Deserialize 1..5 bytes --> UINT
{
	const BYTE *buffer2 = buffer + (index++);
	ASSERT(*buffer2); // should not be zero (indicating a negative number)
	UINT n = 0;
	for (;;index++)
	{
		n = (n << 7) + *buffer2;
		if (*buffer2++ & 0x80)
			break;
	}

	return n - 0x80;
}

//*****************************************************************************
void CCharacter::writeBpUINT(string& buffer, UINT n)
//Serialize UINT --> 1..5 bytes
{
	int s = 7;
	while ((n >> s) && s < 32)
		s += 7;

	while (s)
	{
		s -= 7;
		BYTE b = BYTE((n >> s) & 0x7f);
		if (!s)
			b |= 0x80;
		buffer.append(1, b);
	}
}

//*****************************************************************************
void CCharacter::DeserializeCommand(BYTE* buffer, UINT& index, CCharacterCommand& command)
//Extracts commands serialized in 'buffer' into a command.
{
	command.command = CCharacterCommand::CharCommand(readBpUINT(buffer, index));
	command.x = readBpUINT(buffer, index);
	command.y = readBpUINT(buffer, index);
	command.w = readBpUINT(buffer, index);
	command.h = readBpUINT(buffer, index);
	command.flags = readBpUINT(buffer, index);
	const UINT speechID = readBpUINT(buffer, index);
	if (speechID)
	{
		command.pSpeech = g_pTheDB->Speech.GetByID(speechID);
		//ASSERT(command.pSpeech); //commented out -- script ID recorded in saved game may still exist in the current version of the room, or it may not
	}

	const UINT labelSize = readBpUINT(buffer, index);
	if (labelSize)
	{
		const UINT wchars = labelSize/sizeof(WCHAR);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		WCHAR *pwzBuf = new WCHAR[wchars+1];
		memcpy((void*)pwzBuf, (const void*)(buffer + index), labelSize);
		LittleToBig(pwzBuf, wchars);
		command.label.assign(pwzBuf, wchars);
		delete[] pwzBuf;
#else
		command.label.assign((const WCHAR*)(buffer + index), wchars);
#endif
		index += labelSize;
	}

//		Upgrade2_0CommandTo3_0(command);
}

//*****************************************************************************
void CCharacter::DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMAND_VECTOR& commands)
//Extracts commands serialized in 'buffer' into 'commands'.
{
	UINT index=0;
	while (index < bufferSize)
	{
		CCharacterCommand command;
		DeserializeCommand(buffer, index, command);
		commands.push_back(command);
	}
	ASSERT(index == bufferSize);
}

//*****************************************************************************
void CCharacter::DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMANDPTR_VECTOR& commands)
//Overloaded method with a vector of pointers to commands.
{
	UINT index=0;
	while (index < bufferSize)
	{
		CCharacterCommand *pCommand = new CCharacterCommand();
		DeserializeCommand(buffer, index, *pCommand);
		commands.push_back(pCommand);
	}
	ASSERT(index == bufferSize);
}

//*****************************************************************************
void CCharacter::SerializeCommand(string& buffer, const CCharacterCommand& command)
//Outputs text containing the given command in serialized form.
{
	ASSERT(command.command < CCharacterCommand::CC_Count);

	writeBpUINT(buffer, command.command);
	writeBpUINT(buffer, command.x);
	writeBpUINT(buffer, command.y);
	writeBpUINT(buffer, command.w);
	writeBpUINT(buffer, command.h);
	writeBpUINT(buffer, command.flags);
	writeBpUINT(buffer, command.pSpeech ? command.pSpeech->dwSpeechID : 0);

	const UINT length = command.label.size() * sizeof(WCHAR);
	writeBpUINT(buffer, length);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	WCHAR *pBytes = new WCHAR[command.label.size()];
	memcpy(pBytes, command.label.c_str(), length);
	LittleToBig(pBytes, command.label.size());
	const string label(reinterpret_cast<const char*>(pBytes), length);
	buffer += label;
	delete[] pBytes;
#else
	const string label(reinterpret_cast<const char*>(command.label.c_str()), length);
	buffer += label;
#endif
}

//*****************************************************************************
void CCharacter::SerializeCommands(string& buffer, const COMMAND_VECTOR& commands)
//Returns: allocated byte array containing 'commands' in serialized form
{
	const UINT wNumCommands = commands.size();

	buffer.resize(0);
	buffer.reserve(wNumCommands * 10 * sizeof(UINT)); //heuristic

	//Pack each command in sequence.
	for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
	{
		const CCharacterCommand& command = commands[wIndex];
		SerializeCommand(buffer, command);
	}
}

//*****************************************************************************
void CCharacter::SerializeCommands(string& buffer, const COMMANDPTR_VECTOR& commands)
//Overloaded method with a vector of pointers to commands.
{
	const UINT wNumCommands = commands.size();

	buffer.resize(0);
	buffer.reserve(wNumCommands * 10 * sizeof(UINT)); //heuristic

	//Pack each command in sequence.
	for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
	{
		const CCharacterCommand *pCommand = commands[wIndex];
		SerializeCommand(buffer, *pCommand);
	}
}

//*****************************************************************************
void CCharacter::setBaseMembers(const CDbPackedVars& vars)
//Use default values if these packed vars do not exist.
//Otherwise, override them.
{
	this->wLogicalIdentity = vars.GetVar(idStr, this->wLogicalIdentity);

	this->wIdentity = this->wLogicalIdentity; //by default, these are the same

	this->bVisible = vars.GetVar(visibleStr, this->bVisible);
	if (!this->bVisible)
		this->bSwordSheathed = true;

	this->equipType = (ScriptFlag::EquipmentType)vars.GetVar(equipTypeStr, (int)this->equipType);

	//Various properties.
	this->wTurnDelay = vars.GetVar(TurnDelayStr, this->wTurnDelay);
	this->wXRel = vars.GetVar(XRelStr, this->wXRel);
	this->wYRel = vars.GetVar(YRelStr, this->wYRel);
	this->bMovingRelative = vars.GetVar(MovingRelativeStr, this->bMovingRelative);
	this->wExitingRoomO = vars.GetVar(ExitRoomOStr, this->wExitingRoomO);

	this->eachAttackLabelIndex = vars.GetVar(EachAttackStr, this->eachAttackLabelIndex);
	this->eachDefendLabelIndex = vars.GetVar(EachDefendStr, this->eachDefendLabelIndex);
	this->eachUseLabelIndex = vars.GetVar(EachUseStr, this->eachUseLabelIndex);
	this->eachVictoryLabelIndex = vars.GetVar(EachVictoryStr, this->eachVictoryLabelIndex);

	this->customName = vars.GetVar(CustomNameStr, this->customName.c_str());

	//Imperatives.
	this->bVulnerable = vars.GetVar(VulnerableStr, this->bVulnerable);
	this->bMissionCritical = vars.GetVar(MissionCriticalStr, this->bMissionCritical);
	this->bSafeToPlayer = vars.GetVar(SafeToPlayerStr, this->bSafeToPlayer);
	this->bSwordSafeToPlayer = vars.GetVar(SwordSafeToPlayerStr, this->bSwordSafeToPlayer);
	this->bDefeated = vars.GetVar(DefeatedStr, this->bDefeated);
	this->bShowStatChanges = vars.GetVar(ShowStatChangesStr, this->bShowStatChanges);
	this->bGhostImage = vars.GetVar(GhostImageStr, this->bGhostImage);
	this->bRestartScriptOnRoomEntrance = vars.GetVar(RestartScriptOnEntranceStr, this->bRestartScriptOnRoomEntrance);
	this->bGlobal = vars.GetVar(GlobalStr, this->bGlobal);
	this->bExecuteScriptOnCombat = vars.GetVar(ExecuteScriptOnCombatStr, this->bExecuteScriptOnCombat);

	//Behaviors.
	this->bAttackAdjacent = vars.GetVar(AttackAdjacentStr, this->bAttackAdjacent);
	this->bAttackInFront = vars.GetVar(AttackInFrontStr, this->bAttackInFront);
	this->bAttackInFrontWhenBackIsTurned = vars.GetVar(AttackInFrontWhenBackIsTurnedStr, this->bAttackInFrontWhenBackIsTurned);
	this->bFaceAwayFromTarget = vars.GetVar(FaceAwayFromTargetStr, this->bFaceAwayFromTarget);
	this->bFaceTarget = vars.GetVar(FaceTargetStr, this->bFaceTarget);
	this->bHasRayGun = vars.GetVar(RayGunStr, this->bHasRayGun);
	this->bHasRayBlocking = vars.GetVar(RayBlockingStr, this->bHasRayBlocking);
	this->bSurprisedFromBehind = vars.GetVar(SurprisedFromBehindStr, this->bSurprisedFromBehind);
	this->bGoblinWeakness = vars.GetVar(GoblinWeaknessStr, this->bGoblinWeakness);
	this->bSerpentWeakness = vars.GetVar(SerpentWeaknessStr, this->bSerpentWeakness);
	this->bMetal = vars.GetVar(MetalStr, this->bMetal);
	this->bLuckyGR = vars.GetVar(LuckyGRStr, this->bLuckyGR);
	this->bLuckyXP = vars.GetVar(LuckyXPStr, this->bLuckyXP);
	this->bBriar = vars.GetVar(BriarStr, this->bBriar);
	this->bNoEnemyDEF = vars.GetVar(NoEnemyDefenseStr, this->bNoEnemyDEF);
	this->bAttackFirst = vars.GetVar(AttackFirstStr, this->bAttackFirst);
	this->bAttackLast = vars.GetVar(AttackLastStr, this->bAttackLast);
	this->movementIQ = (MovementIQ)vars.GetVar(MovementIQStr, this->movementIQ);
	this->bDropTrapdoors = vars.GetVar(DropTrapdoorsStr, this->bDropTrapdoors);
	this->bMoveIntoSwords = vars.GetVar(MoveIntoSwordsStr, this->bMoveIntoSwords);
	this->bPushObjects = vars.GetVar(PushObjectsStr, this->bPushObjects);
	this->bSpawnEggs = vars.GetVar(SpawnEggsStr, this->bSpawnEggs);
	this->eMovement = (MovementType)vars.GetVar(MovementTypeStr, this->eMovement);

	//Stats.
	this->color = vars.GetVar(ColorStr, this->color);
	this->sword = vars.GetVar(SwordStr, this->sword);
	this->paramX = vars.GetVar(ParamXStr, this->paramX);
	this->paramY = vars.GetVar(ParamYStr, this->paramY);
	this->paramW = vars.GetVar(ParamWStr, this->paramW);
	this->paramH = vars.GetVar(ParamHStr, this->paramH);
	this->paramF = vars.GetVar(ParamFStr, this->paramF);

	//Modifiers
	this->monsterHPmult = vars.GetVar(MonsterHPMultStr, this->monsterHPmult);
	this->monsterATKmult = vars.GetVar(MonsterATKMultStr, this->monsterATKmult);
	this->monsterDEFmult = vars.GetVar(MonsterDEFMultStr, this->monsterDEFmult);
	this->monsterGRmult = vars.GetVar(MonsterGRMultStr, this->monsterGRmult);
	this->monsterXPmult = vars.GetVar(MonsterXPMultStr, this->monsterXPmult);

	this->itemMult = vars.GetVar(ItemMultStr, this->itemMult);
	this->itemHPmult = vars.GetVar(ItemHPMultStr, this->itemHPmult);
	this->itemATKmult = vars.GetVar(ItemATKMultStr, this->itemATKmult);
	this->itemDEFmult = vars.GetVar(ItemDEFMultStr, this->itemDEFmult);
	this->itemGRmult = vars.GetVar(ItemGRMultStr, this->itemGRmult);

	//Spawn type
	this->wSpawnType = vars.GetVar(SpawnTypeStr, this->wSpawnType);
}

//*****************************************************************************
void CCharacter::SetExtraVarsFromMembersWithoutScript(CDbPackedVars& vars) //(out)
const
//Packs NPC state info.
{
	//Only save info currently in NPC object's data structures.
	vars.Clear();

	vars.SetVar(idStr, this->wLogicalIdentity);
	vars.SetVar(visibleStr, this->bVisible);
	vars.SetVar(equipTypeStr, (int)this->equipType);

	//Various properties.
	//Only set if not the default value to save space.
	if (this->wTurnDelay)
		vars.SetVar(TurnDelayStr, this->wTurnDelay);
	if (this->wXRel)
		vars.SetVar(XRelStr, this->wXRel);
	if (this->wYRel)
		vars.SetVar(YRelStr, this->wYRel);
	if (this->bMovingRelative)
		vars.SetVar(MovingRelativeStr, this->bMovingRelative);
	if (this->wExitingRoomO != NO_ORIENTATION)
		vars.SetVar(ExitRoomOStr, this->wExitingRoomO);

	if (this->eachAttackLabelIndex != NO_LABEL)
		vars.SetVar(EachAttackStr, this->eachAttackLabelIndex);
	if (this->eachDefendLabelIndex != NO_LABEL)
		vars.SetVar(EachDefendStr, this->eachDefendLabelIndex);
	if (this->eachUseLabelIndex != NO_LABEL)
		vars.SetVar(EachUseStr, this->eachUseLabelIndex);
	if (this->eachVictoryLabelIndex != NO_LABEL)
		vars.SetVar(EachVictoryStr, this->eachVictoryLabelIndex);

	if (!this->customName.empty())
		vars.SetVar(CustomNameStr, this->customName.c_str());

	//Imperatives.
	if (!this->bVulnerable)
		vars.SetVar(VulnerableStr, this->bVulnerable);
	if (this->bMissionCritical)
		vars.SetVar(MissionCriticalStr, this->bMissionCritical);
	if (this->bSafeToPlayer)
		vars.SetVar(SafeToPlayerStr, this->bSafeToPlayer);
	if (this->bSwordSafeToPlayer)
		vars.SetVar(SwordSafeToPlayerStr, this->bSwordSafeToPlayer);
	if (this->bDefeated)
		vars.SetVar(DefeatedStr, this->bDefeated);
	if (!this->bShowStatChanges)
		vars.SetVar(ShowStatChangesStr, this->bShowStatChanges);
	if (this->bGhostImage)
		vars.SetVar(GhostImageStr, this->bGhostImage);
	if (this->bRestartScriptOnRoomEntrance)
		vars.SetVar(RestartScriptOnEntranceStr, this->bRestartScriptOnRoomEntrance);
	if (this->bGlobal)
		vars.SetVar(GlobalStr, this->bGlobal);
	if (this->bExecuteScriptOnCombat)
		vars.SetVar(ExecuteScriptOnCombatStr, this->bExecuteScriptOnCombat);

	//Behaviors.
	if (this->bAttackAdjacent)
		vars.SetVar(AttackAdjacentStr, this->bAttackAdjacent);
	if (this->bAttackInFront)
		vars.SetVar(AttackInFrontStr, this->bAttackInFront);
	if (this->bAttackInFrontWhenBackIsTurned)
		vars.SetVar(AttackInFrontWhenBackIsTurnedStr, this->bAttackInFrontWhenBackIsTurned);
	if (this->bFaceAwayFromTarget)
		vars.SetVar(FaceAwayFromTargetStr, this->bFaceAwayFromTarget);
	if (this->bFaceTarget)
		vars.SetVar(FaceTargetStr, this->bFaceTarget);
	if (this->bHasRayGun)
		vars.SetVar(RayGunStr, this->bHasRayGun);
	if (this->bHasRayBlocking)
		vars.SetVar(RayBlockingStr, this->bHasRayBlocking);
	if (this->bSurprisedFromBehind)
		vars.SetVar(SurprisedFromBehindStr, this->bSurprisedFromBehind);
	if (this->bGoblinWeakness)
		vars.SetVar(GoblinWeaknessStr, this->bGoblinWeakness);
	if (this->bSerpentWeakness)
		vars.SetVar(SerpentWeaknessStr, this->bSerpentWeakness);
	if (this->bMetal)
		vars.SetVar(MetalStr, this->bMetal);
	if (this->bLuckyGR)
		vars.SetVar(LuckyGRStr, this->bLuckyGR);
	if (this->bLuckyXP)
		vars.SetVar(LuckyXPStr, this->bLuckyXP);
	if (this->bBriar)
		vars.SetVar(BriarStr, this->bBriar);
	if (this->bNoEnemyDEF)
		vars.SetVar(NoEnemyDefenseStr, this->bNoEnemyDEF);
	if (this->bAttackFirst)
		vars.SetVar(AttackFirstStr, this->bAttackFirst);
	if (this->bAttackLast)
		vars.SetVar(AttackLastStr, this->bAttackLast);
	if (this->movementIQ)
		vars.SetVar(MovementIQStr, this->movementIQ);
	if (this->bDropTrapdoors)
		vars.SetVar(DropTrapdoorsStr, this->bDropTrapdoors);
	if (this->bMoveIntoSwords)
		vars.SetVar(MoveIntoSwordsStr, this->bMoveIntoSwords);
	if (this->bPushObjects)
		vars.SetVar(PushObjectsStr, this->bPushObjects);
	if (this->bSpawnEggs)
		vars.SetVar(SpawnEggsStr, this->bSpawnEggs);
	if (this->eMovement)
		vars.SetVar(MovementTypeStr, this->eMovement);

	//Stats.
	if (this->color)
		vars.SetVar(ColorStr, this->color);
	if (this->sword != NPC_DEFAULT_SWORD)
		vars.SetVar(SwordStr, this->sword);
	if (this->paramX != NO_OVERRIDE)
		vars.SetVar(ParamXStr, this->paramX);
	if (this->paramY != NO_OVERRIDE)
		vars.SetVar(ParamYStr, this->paramY);
	if (this->paramW != NO_OVERRIDE)
		vars.SetVar(ParamWStr, this->paramW);
	if (this->paramH != NO_OVERRIDE)
		vars.SetVar(ParamHStr, this->paramH);
	if (this->paramF != NO_OVERRIDE)
		vars.SetVar(ParamFStr, this->paramF);

	// Modifiers
	if (this->monsterHPmult != 100)
		vars.SetVar(MonsterHPMultStr, this->monsterHPmult);
	if (this->monsterATKmult != 100)
		vars.SetVar(MonsterATKMultStr, this->monsterATKmult);
	if (this->monsterDEFmult != 100)
		vars.SetVar(MonsterDEFMultStr, this->monsterDEFmult);
	if (this->monsterGRmult != 100)
		vars.SetVar(MonsterGRMultStr, this->monsterGRmult);
	if (this->monsterXPmult != 100)
		vars.SetVar(MonsterXPMultStr, this->monsterXPmult);

	if (this->itemMult != 100)
		vars.SetVar(ItemMultStr, this->itemMult);
	if (this->itemHPmult != 100)
		vars.SetVar(ItemHPMultStr, this->itemHPmult);
	if (this->itemATKmult != 100)
		vars.SetVar(ItemATKMultStr, this->itemATKmult);
	if (this->itemDEFmult != 100)
		vars.SetVar(ItemDEFMultStr, this->itemDEFmult);
	if (this->itemGRmult != 100)
		vars.SetVar(ItemGRMultStr, this->itemGRmult);

	// Spawn type
	if (this->wSpawnType != -1)
		vars.SetVar(SpawnTypeStr, this->wSpawnType);

	vars.SetVar(scriptIDstr, this->dwScriptID);

	vars.SetVar(startLineStr, this->wCurrentCommandIndex);
}

//*****************************************************************************
void CCharacter::SetMembers(const CDbPackedVars& vars)
//Reads vars from ExtraVars to reconstruct the character's ID and command sequence.
{
	setBaseMembers(vars);

	LoadCommands(vars, this->commands);

	this->dwScriptID = vars.GetVar(scriptIDstr, UINT(0));

	this->wCurrentCommandIndex = vars.GetVar(startLineStr, UINT(0));
//	ASSERT(this->wCurrentCommandIndex <= this->commands.size()); //not valid when scripts are not loaded

	CMonster::SetMembers(vars);
}

//*****************************************************************************
void CCharacter::Delete()
//Deletes speech records (exclusively) owned by character from DB.
{
	SetMembers(this->ExtraVars);
	for (UINT wIndex=this->commands.size(); wIndex--; )
	{
		if (this->commands[wIndex].pSpeech)
			g_pTheDB->Speech.Delete(this->commands[wIndex].pSpeech->dwSpeechID);
	}
}

//*****************************************************************************
void CCharacter::PackExtraVars(
	const bool bSaveScript) //whether to save the NPC script in packed vars
{
	SetExtraVarsFromMembersWithoutScript(this->ExtraVars);
	if (bSaveScript) {
		SaveCommands(this->ExtraVars, this->commands);
	}
}

//*****************************************************************************
void CCharacter::Save(
//Places monster object member vars into database view.
//
//Params:
	const c4_RowRef &MonsterRowRef,     //(in/out) Open view to fill.
	const bool bSaveScript) //whether to save the NPC script in packed vars [default=true]
{
	PackExtraVars(bSaveScript);

	CMonster::Save(MonsterRowRef);
}

//*****************************************************************************
void CCharacter::SaveSpeech(const COMMAND_VECTOR& commands)
//Save speech objects owned by script commands to the DB.
{
	//Save out any new speech records to DB.
	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		const CCharacterCommand& command = commands[wIndex];
		if (command.pSpeech)
			command.pSpeech->Update();
	}
}

//*****************************************************************************
void CCharacter::SaveSpeech(const COMMANDPTR_VECTOR& commands)
//Save speech objects owned by script commands to the DB.
{
	//Save out any new speech records to DB.
	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		const CCharacterCommand *pCommand = commands[wIndex];
		ASSERT(pCommand);
		if (pCommand->pSpeech)
			pCommand->pSpeech->Update();
	}
}

//
// Private methods
//

//*****************************************************************************
void CCharacter::Disappear()
//Removes the NPC from the room, but not the monster list.
{
	ASSERT(this->bVisible);
	this->bVisible = false;
	this->bSwordSheathed = true;

	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->pRoom)
	{
		CDbRoom& room = *(this->pCurrentGame->pRoom);
		ASSERT(room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] == this);
		room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = NULL;
	}
}

//*****************************************************************************
int CCharacter::GetIndexOfCommandWithLabel(const int label) const
//Returns: index of command with specified label, or NO_LABEL if none
{
	if (label > 0) {
		for (UINT wIndex = this->commands.size(); wIndex--; )
		{
			const CCharacterCommand& command = this->commands[wIndex];
			if (command.command == CCharacterCommand::CC_Label &&
				label == command.x)
				return wIndex;
		}
	} else if (label < 0) {
		const ScriptFlag::GotoSmartType eGotoType = (ScriptFlag::GotoSmartType) label;
		switch (eGotoType) {
			case ScriptFlag::GotoSmartType::PreviousIf:
				return GetIndexOfPreviousIf(true);
			case ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition:
			{
				int wIndex = GetIndexOfNextElse(false);
				if (wIndex != NO_LABEL) {
					if (this->commands[wIndex].command == CCharacterCommand::CC_IfElseIf)
						return wIndex + 2;

					return wIndex + 1;
				}
			}
			break;
			default:
				return NO_LABEL;
		}
	}
	return NO_LABEL;
}

//*****************************************************************************
//Return: Index of the if or else if command at the beginning of the block or
// NO_LABEL if not in a block
int CCharacter::GetIndexOfPreviousIf(const bool bIgnoreElseIf) const
{
	UINT wCommandIndex = this->wCurrentCommandIndex;
	UINT wNestingDepth = 0;

	while (wCommandIndex > 0) {
		--wCommandIndex;

		CCharacterCommand command = this->commands[wCommandIndex];
		switch (command.command) {
		case CCharacterCommand::CC_If:
			if (wNestingDepth-- == 0)
				return wCommandIndex; // Found start of if block
			break;
		case CCharacterCommand::CC_IfElseIf:
			if (wNestingDepth == 0 && !bIgnoreElseIf)
				return wCommandIndex; // Found start of else-if block
			break;
		case CCharacterCommand::CC_IfEnd:
			wNestingDepth++; // entering a nested if-block
			break;
		}
	}

	return NO_LABEL;
}

//*****************************************************************************
// Return: Index of the else or else if command at the end of the block or
// NO_LABEL if not in a block
int CCharacter::GetIndexOfNextElse(const bool bIgnoreElseIf) const
{
	UINT wCommandIndex = this->wCurrentCommandIndex;
	UINT wNestingDepth = 0;

	while (wCommandIndex < this->commands.size()) {
		CCharacterCommand command = this->commands[wCommandIndex];
		switch (command.command) {
		case CCharacterCommand::CC_If:
			wNestingDepth++; // entering a nested if-block
			break;
		case CCharacterCommand::CC_IfElse:
			if (wNestingDepth == 0)
				return wCommandIndex; // Found start of else block
			break;
		case CCharacterCommand::CC_IfElseIf:
			if (wNestingDepth == 0 && !bIgnoreElseIf)
				return wCommandIndex; // Found start of else-if block
			break;
		case CCharacterCommand::CC_IfEnd:
			if (wNestingDepth > 0)
				wNestingDepth--; // exiting a nested if-block
			break;
		}

		++wCommandIndex;
	}

	return NO_LABEL;
}

//*****************************************************************************
void CCharacter::MoveCharacter(
//Handle an open move and incidental consequences.
//
//Params:
	const int dx, const int dy, //movement delta
	const bool bFaceDirection,
	CCueEvents& CueEvents)      //(in/out)
{
	//If player was bumped into, attempt to initiate combat.
	if (this->pCurrentGame->IsPlayerAt(this->wX + dx, this->wY + dy))
	{
		if (IsCombatable())
			((CCurrentGame*)this->pCurrentGame)->MonsterInitiatesCombat(CueEvents, this);
		return;
	}

	//Before he moves, remember important square contents.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wOTile = room.GetOSquare(this->wX, this->wY);
	const bool bWasOnTrapdoor = bIsTrapdoor(wOTile);
	const bool bWasOnPlatform = bIsPlatform(wOTile);

	Move(this->wX + dx, this->wY + dy, &CueEvents);
	this->wSwordMovement = nGetO(dx,dy);
	if (bFaceDirection)	//allow turning?
		SetOrientation(dx, dy);	//character faces the direction it actually moves
	SetSwordSheathed();

	//Special actions.
	if (bWasOnTrapdoor && this->bDropTrapdoors)
		room.DestroyTrapdoor(this->wX - dx, this->wY - dy, CueEvents);

	if (this->bPushObjects)
	{
		if (bWasOnPlatform)
		{
			const UINT wOTile = room.GetOSquare(this->wX, this->wY);
			if (bIsPit(wOTile) || wOTile == T_WATER)
				room.MovePlatform(this->wX - dx, this->wY - dy, nGetO(dx,dy));
		}

		//Process any and all of these item interactions.
		UINT tTile = room.GetTSquare(this->wX, this->wY);
		if (tTile==T_MIRROR || tTile==T_CRATE)
		{
			room.PushObject(this->wX, this->wY, this->wX + dx, this->wY + dy, CueEvents);
			tTile = room.GetTSquare(this->wX, this->wY); //also check what was under the object
		}
		if (tTile==T_TOKEN)
			room.ActivateToken(CueEvents, this->wX, this->wY);
	}
}

//*****************************************************************************
void CCharacter::TeleportCharacter(
	const UINT wDestX, const UINT wDestY, //(in) Destination square
	CCueEvents& CueEvents)      //(in/out)
//Moves a Character from one square to an entirely different one.
//Disappear/AppearAt logic is used for this, since it is not a standard move.
//Target square must be already clear to use: check before using this routine
{
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	if (this->bVisible)
	{
		ASSERT(room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] == this);
		room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = NULL;
	}

	CCoordSet coords(this->wX, this->wY);
	coords.insert(wDestX, wDestY);
	//Since this method is called for setting either X or Y alone,
	//the caller must handle setting of wPrevX/Y as applicable
	this->wX = wDestX;
	this->wY = wDestY;

	if (this->bVisible)
	{
		ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)]);
		room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = this;
		SetSwordSheathed();

		//Check for stepping on pressure plate.
		if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && !IsFlying())
			room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);
	}

	room.Plot(coords); //update room
}

//*****************************************************************************
void CCharacter::TurnIntoMonster(
//Replace with normal monster of specified type.
//
//Params:
	CCueEvents& CueEvents, //(in/out)
	const bool /*bSpecial*/)  //special transformation behavior [default=false]
{
	UINT identity = this->wLogicalIdentity;
	if (!IsValidMonsterType(identity))
	{
		//This is a pseudo-monster type.  Just stop executing script.
		this->wCurrentCommandIndex = this->commands.size();

		//Special option: when the player is not in the room,
		//replace this character with the player at this location.
		if (this->pCurrentGame->pPlayer->wIdentity == M_NONE &&
				identity >= CHARACTER_FIRST) //non-monster only
		{
			Disappear();
			CCurrentGame *pCurrentGame = (CCurrentGame*)this->pCurrentGame;
			CSwordsman& player = *(pCurrentGame->pPlayer);
			player.Move(this->wX, this->wY);
			player.Move(this->wX, this->wY); //removes prev coords
			player.SetOrientation(this->wO);
			pCurrentGame->SetPlayerRole(identity);
		}
		return;
	}

	CCueEvents Ignored;
	this->pCurrentGame->pRoom->KillMonster(this, Ignored, true);

	if (!this->bVisible)
		return; //not in room -- don't change to real monster type

	const_cast<CCurrentGame*>(this->pCurrentGame)->AddNewEntity(CueEvents,
			identity, this->wX, this->wY, this->wO);

	this->bReplaced = true;
//	CueEvents.Add(CID_NPCTypeChange);
}
