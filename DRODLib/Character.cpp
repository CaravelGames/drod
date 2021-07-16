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
 *
 * ***** END LICENSE BLOCK ***** */

#include "BuildUtil.h"
#include "Character.h"
#include "CurrentGame.h"
#include "Clone.h"
#include "Db.h"
#include "DbRooms.h"
#include "EvilEye.h"
#include "Fegundo.h"
#include "Serpent.h"
#include "RockGiant.h"
#include "RockGolem.h"
#include "../Texts/MIDs.h"

#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Files.h>

const UINT NPC_DEFAULT_SWORD = UINT(-1);

const UINT MAX_ANSWERS = 9;

#define NO_LABEL (-1)

#define NO_OVERRIDE (UINT(-9999))

#define SKIP_WHITESPACE(str, index) while (iswspace(str[index])) ++index

//*****************************************************************************
inline bool isVarCharValid(WCHAR wc)
{
	return iswalnum(wc) || wc == W_t('_');
}

//*****************************************************************************
bool addWithClamp(int& val, const int operand)
//Multiplies two integers, ensuring the product doesn't overflow.
//
//Returns: false if actual result can't be given (i.e. value overflowed), otherwise true
{
	const double newVal = (double)val + operand;
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
	val += operand;
	return true;
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

void LogParseError(const WCHAR* pwStr, const char* message)
{
	CFiles f;
	string str = UnicodeToUTF8(pwStr);
	str += ": ";
	str += message;
	f.AppendErrorLog(str.c_str());
}

inline bool bCommandHasData(const UINT eCommand)
//Returns: whether this script command has a data record attached to it
{
	switch (eCommand)
	{
		case CCharacterCommand::CC_AmbientSound:
		case CCharacterCommand::CC_AmbientSoundAt:
		case CCharacterCommand::CC_PlayVideo:
		case CCharacterCommand::CC_SetMusic:
		case CCharacterCommand::CC_WorldMapMusic:
		case CCharacterCommand::CC_ImageOverlay:
		case CCharacterCommand::CC_WorldMapImage:
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
	: CPlayerDouble(M_CHARACTER, pSetCurrentGame, GROUND_AND_SHALLOW_WATER,
			SPD_CHARACTER)  //put last in process sequence so all cue events will have
							//occurred and can be detected by the time Process() is called
	, dwScriptID(0)
	, wIdentity(UINT(-1)) //none
	, wLogicalIdentity(UINT(-1))
	, pCustomChar(NULL)
	, bVisible(false)
	, eDisplayMode(CDM_Normal)
	, bScriptDone(false), bReplaced(false)
	, bWasPushed(false)
	, bPreventMoveAfterPush(false)
	, bGlobal(false)
	, bNewEntity(false)
	, bYesNoQuestion(false)
	, bPlayerTouchedMe(false)
	, wCurrentCommandIndex(0)
	, wTurnDelay(0)
	, eImperative(ScriptFlag::Vulnerable)
	, wXRel(0), wYRel(0)
	, bMovingRelative(false)
	, bSafeToPlayer(true)
	, bSwordSafeToPlayer(false)
	, bEndWhenKilled(false)
	, bNotPushable(false)
	, bPushableByBody(false)
	, bPushableByWeapon(false)
	, bStunnable(true)
	, bBrainPathmapObstacle(false), bNPCPathmapObstacle(true)
	, bWeaponOverride(false)
	, bFriendly(true)
	, movementIQ(SmartOmniDirection)
	, worldMapID(0)
	, nColor(-1)

	, bWaitingForCueEvent(false)
	, bIfBlock(false)
	, wLastSpeechLineNumber(0)

	, paramX(NO_OVERRIDE), paramY(NO_OVERRIDE), paramW(NO_OVERRIDE), paramH(NO_OVERRIDE), paramF(NO_OVERRIDE)
{
	this->goal.wX = UINT(-1); //invalid
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
	PrimaryKeyMap::const_iterator newID = info.EntranceIDMap.find(entranceID);
	if (newID != info.EntranceIDMap.end()) {
		entranceID = newID->second;
	} else {
		entranceID = 0; //end hold
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
				case CCharacterCommand::CC_ImageOverlay:
					//Make a copy of the media object in the new hold.
					CDbData::CopyObject(info, c.w, pNewHold->dwHoldID);
				break;
				case CCharacterCommand::CC_SetMusic:
				case CCharacterCommand::CC_WorldMapMusic:
					CDbData::CopyObject(info, c.y, pNewHold->dwHoldID);
				break;
				case CCharacterCommand::CC_WorldMapImage:
					CDbData::CopyObject(info, c.h, pNewHold->dwHoldID);
					SyncEntranceID(info, c.w);
				break;
				case CCharacterCommand::CC_SetNPCAppearance:
				case CCharacterCommand::CC_SetPlayerAppearance:
				case CCharacterCommand::CC_StartGlobalScript:
					SyncCustomCharacterData(c.x, pOldHold, pNewHold, info);
				break;
				case CCharacterCommand::CC_GenerateEntity:
					SyncCustomCharacterData(c.h, pOldHold, pNewHold, info);
					break;
				case CCharacterCommand::CC_WorldMapIcon:
					SyncCustomCharacterData(c.h, pOldHold, pNewHold, info);
					SyncEntranceID(info, c.w);
				break;
				case CCharacterCommand::CC_WaitForEntityType:
				case CCharacterCommand::CC_WaitForNotEntityType:
				case CCharacterCommand::CC_WaitForRemains:
					SyncCustomCharacterData(c.flags, pOldHold, pNewHold, info);
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
int CCharacter::getLocalVarInt(const WSTRING& varName) const
{
	const WSTRING wstr = getLocalVarString(varName);
	return _Wtoi(wstr.c_str());
}

WSTRING CCharacter::getLocalVarString(const WSTRING& varName) const
{
	LocalScriptMap::const_iterator it = this->localScriptVars.find(varName);
	if (it == this->localScriptVars.end())
		return WSTRING();

	return it->second;
}

//*****************************************************************************
WSTRING CCharacter::getPredefinedVar(const UINT varIndex) const
{
	WSTRING wstr;
	if (ScriptVars::IsStringVar(ScriptVars::Predefined(varIndex))) {
		wstr = getPredefinedVarString(varIndex);
	} else {
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
		case (UINT)ScriptVars::P_MONSTER_WEAPON:
			if (!this->bWeaponOverride && !bEntityHasSword(GetResolvedIdentity()))
				return WT_Off;
			if (this->bNoWeapon)
				return WT_Off;
			return this->weaponType;

		case (UINT)ScriptVars::P_MONSTER_COLOR:
			return this->nColor;

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

		case (UINT)ScriptVars::P_PLAYER_X:
		case (UINT)ScriptVars::P_PLAYER_Y:
		case (UINT)ScriptVars::P_PLAYER_O:
		case (UINT)ScriptVars::P_ROOMIMAGE_X:
		case (UINT)ScriptVars::P_ROOMIMAGE_Y:
		case (UINT)ScriptVars::P_OVERHEADIMAGE_X:
		case (UINT)ScriptVars::P_OVERHEADIMAGE_Y:
		case (UINT)ScriptVars::P_THREATCLOCK:
		case (UINT)ScriptVars::P_ROOM_X:
		case (UINT)ScriptVars::P_ROOM_Y:
		case (UINT)ScriptVars::P_PLAYERLIGHT:
		case (UINT)ScriptVars::P_PLAYERLIGHTTYPE:
		case (UINT)ScriptVars::P_RETURN_X:
		case (UINT)ScriptVars::P_RETURN_Y:
			return this->pCurrentGame->getVar(varIndex);

		default: ASSERT(!"GetVar val not supported"); return 0;
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
		case (UINT)ScriptVars::P_LEVELNAME:
			return this->pCurrentGame->getStringVar(varIndex);
		case (UINT)ScriptVars::P_MONSTER_NAME:
			return this->customName;
		default:
			ASSERT(!"getPredefinedStringVar val not supported");
			return WSTRING();
	}
}

//*****************************************************************************
void CCharacter::setPredefinedVarInt(
	const UINT varIndex, const UINT val,
	CCueEvents &CueEvents)
//Sets the value of the predefined var with this relative index to the specified value
{
	CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);

	ASSERT(varIndex >= (UINT)ScriptVars::FirstPredefinedVar);
	switch (varIndex)
	{
		case (UINT)ScriptVars::P_MONSTER_WEAPON:
			if ((int)val >= WT_Sword && val < WT_NumWeapons) {
				this->bNoWeapon = false;
				this->weaponType = WeaponType(val);
			} else if ((int)val == WT_Off) {
				this->bNoWeapon = true;
			} else if ((int)val == WT_On) {
				this->bNoWeapon = false;
			}
			else {
				return;
			}
			this->bWeaponOverride = true;
			SetWeaponSheathed();
		break;

		case (UINT)ScriptVars::P_MONSTER_COLOR:
			this->nColor = val;
		break;
		//Room position.
		case (UINT)ScriptVars::P_MONSTER_X:
		{
			//Ensure square is valid and available.
			const CDbRoom& room = *(pGame->pRoom);
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
			const CDbRoom& room = *(pGame->pRoom);
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

		case (UINT)ScriptVars::P_PLAYER_X:
			pGame->TeleportPlayer(val, pGame->swordsman.wY, CueEvents);
		break;
		case (UINT)ScriptVars::P_PLAYER_Y:
			pGame->TeleportPlayer(pGame->swordsman.wX, val, CueEvents);
		break;
		case (UINT)ScriptVars::P_PLAYER_O:
		case (UINT)ScriptVars::P_ROOMIMAGE_X:
		case (UINT)ScriptVars::P_ROOMIMAGE_Y:
		case (UINT)ScriptVars::P_OVERHEADIMAGE_X:
		case (UINT)ScriptVars::P_OVERHEADIMAGE_Y:
		case (UINT)ScriptVars::P_THREATCLOCK:
		case (UINT)ScriptVars::P_PLAYERLIGHT:
		case (UINT)ScriptVars::P_PLAYERLIGHTTYPE:
		case (UINT)ScriptVars::P_RETURN_X:
		case (UINT)ScriptVars::P_RETURN_Y:
		default:
			pGame->ProcessCommandSetVar(varIndex, val);
		break;
	}
}

//*****************************************************************************
void CCharacter::setPredefinedVarString(
	const UINT varIndex, const WSTRING val,
	CCueEvents& CueEvents)
	//Sets the value of the predefined var with this relative index to the specified value
{
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
		case M_ROCKGOLEM: case M_CONSTRUCT:
			return true;
		default: return false;
	}
}

//*****************************************************************************
void CCharacter::ReflectX(CDbRoom *pRoom)
//Update script commands to work properly when the room is reflected horizontally (about the x-axis).
{
	CMonster::ReflectX(pRoom);
	for (vector<CCharacterCommand>::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		switch (command->command)
		{
			case CCharacterCommand::CC_AppearAt:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_TeleportTo:
			case CCharacterCommand::CC_TeleportPlayerTo:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_ActivateItemAt:
			case CCharacterCommand::CC_WaitForDoorTo:
			case CCharacterCommand::CC_GameEffect:
			case CCharacterCommand::CC_AttackTile:
			case CCharacterCommand::CC_GetEntityDirection:
			case CCharacterCommand::CC_FaceTowards:
			case CCharacterCommand::CC_WaitForWeapon:
			case CCharacterCommand::CC_VarSetAt:
				command->x = (pRoom->wRoomCols-1) - command->x;
			break;
			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			case CCharacterCommand::CC_WaitForNoBuilding:
			case CCharacterCommand::CC_BuildMarker:
			case CCharacterCommand::CC_Build:
			case CCharacterCommand::CC_DestroyTrapdoor:
			case CCharacterCommand::CC_WaitForItem:
			case CCharacterCommand::CC_WaitForEntityType:
			case CCharacterCommand::CC_WaitForNotEntityType:
			case CCharacterCommand::CC_WaitForRemains:
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

			case CCharacterCommand::CC_MoveRel:
				command->x = (UINT)(-((int)command->x));
			break;
			case CCharacterCommand::CC_GenerateEntity:
			case CCharacterCommand::CC_PushTile:
				command->x = (pRoom->wRoomCols-1) - command->x;
				if (IsValidOrientation(command->w))
					command->w = nGetO(-nGetOX(command->w),nGetOY(command->w));
				else
					command->w = command->w == CMD_C ? CMD_CC : CMD_C;
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
	for (vector<CCharacterCommand>::iterator command = this->commands.begin();
			command != this->commands.end(); ++command)
	{
		switch (command->command)
		{
			case CCharacterCommand::CC_AppearAt:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_TeleportTo:
			case CCharacterCommand::CC_TeleportPlayerTo:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_ActivateItemAt:
			case CCharacterCommand::CC_WaitForDoorTo:
			case CCharacterCommand::CC_GameEffect:
			case CCharacterCommand::CC_AttackTile:
			case CCharacterCommand::CC_GetEntityDirection:
			case CCharacterCommand::CC_FaceTowards:
			case CCharacterCommand::CC_WaitForWeapon:
			case CCharacterCommand::CC_VarSetAt:
				command->y = (pRoom->wRoomRows-1) - command->y;
			break;
			case CCharacterCommand::CC_WaitForRect:
			case CCharacterCommand::CC_WaitForNotRect:
			case CCharacterCommand::CC_WaitForNoBuilding:
			case CCharacterCommand::CC_BuildMarker:
			case CCharacterCommand::CC_Build:
			case CCharacterCommand::CC_DestroyTrapdoor:
			case CCharacterCommand::CC_WaitForItem:
			case CCharacterCommand::CC_WaitForEntityType:
			case CCharacterCommand::CC_WaitForNotEntityType:
			case CCharacterCommand::CC_WaitForRemains:
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
			case CCharacterCommand::CC_PushTile:
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
CMonster* CCharacter::Replicate() const
//Make duplicate objects for the command vector.
{
	CCharacter *pCharacter = new CCharacter(*this);
	pCharacter->dwScriptID = this->dwScriptID; //must be reassigned correctly later
	for (vector<CCharacterCommand>::iterator command = pCharacter->commands.begin();
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
	for (vector<CCharacterCommand>::iterator command = this->commands.begin();
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
bool CCharacter::OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/, const UINT /*wY*/, WeaponType weaponType)
//Returns: whether character was killed
{
	const bool bIsPushableSafe = this->bPushableByWeapon
		&& weaponType != WT_Firetrap
		&& weaponType != WT_FloorSpikes
		&& weaponType != WT_HotTile;
	if (this->eImperative == ScriptFlag::Invulnerable || 
			bIsPushableSafe ||
			this->IsImmuneToWeapon(weaponType))
		return false;

	CueEvents.Add(CID_MonsterDiedFromStab, this);
	RefreshBriars();
	return true;
}

//*****************************************************************************
bool CCharacter::IsImmuneToWeapon(WeaponType type) const
//Returns: wether the character is safe from the given weapon type
{
	switch (type)
	{
		case WT_HotTile: {
			return HasBehavior(ScriptFlag::HotTileImmune);
		}
		case WT_Firetrap: {
			return HasBehavior(ScriptFlag::FiretrapImmune);
		}
		case WT_FloorSpikes: {
			return HasBehavior(ScriptFlag::FloorSpikeImmune);
		}
		case WT_Sword: {
			return HasBehavior(ScriptFlag::SwordDamageImmune);
		}
		case WT_Pickaxe: {
			return HasBehavior(ScriptFlag::PickaxeDamageImmune);
		}
		case WT_Spear: {
			return HasBehavior(ScriptFlag::SpearDamageImmune);
		}
		case WT_Staff: {
			return true;
		}
		case WT_Dagger: {
			return HasBehavior(ScriptFlag::DaggerDamageImmune);
		}
		case WT_Caber: {
			return HasBehavior(ScriptFlag::CaberDamageImmune);
		}
		default: {
			ASSERT(!"Bad weapon type");
			return true;
		}
	}
}

//*****************************************************************************
//Characters won't be stuned with imperative not stunnable
void CCharacter::Stun(CCueEvents &CueEvents, UINT val) //[default=1]
{
	if (this->bStunnable)
		CMonster::Stun(CueEvents, val);
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
			return true; //caller will parse the close paren
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
	if (pwStr[index] == W_t('_') || iswalpha(pwStr[index]) || pwStr[index] == W_t('.')) //valid first char
	{
		//Find spot where var identifier ends.
		int endIndex = index + 1;
		int spcTrail = 0;
		while (CDbHold::IsVarCharValid(pwStr[endIndex]))
		{
			if (pwStr[endIndex] == W_t(' '))
				++spcTrail;
			else
				spcTrail = 0;
			++endIndex;
		}

		const WSTRING wVarName(pwStr + index, endIndex - index - spcTrail);
		index = endIndex;

		//Is it a predefined var?
		const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wVarName);
		if (eVar != ScriptVars::P_NoVar)
			return true;

		if (ScriptVars::IsCharacterLocalVar(wVarName))
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
// term = factor {("*"|"/"|"%") factor}
//
// factor = var | number | "(" expression ")"
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
			return val; //caller will parse the closing char
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
int CCharacter::parseTerm(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC)
//Parse and evaluate a term in an expression.
//
// term = factor {("*"|"/"|"%") factor}
//
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
//Parse and evaluate a term in an expression.
{
	SKIP_WHITESPACE(pwStr, index);

	//A nested expression?
	if (pwStr[index] == W_t('('))
		return parseNestedExpression(pwStr, index, pGame, pNPC);

	//Number?
	if (iswdigit(pwStr[index]))
		return parseNumber(pwStr, index);

	//Variable identifier?
	if (pwStr[index] == W_t('_') || iswalpha(pwStr[index]) || pwStr[index] == W_t('.')) //valid first char
	{
		//Find spot where var identifier ends.
		int endIndex = index + 1;
		int spcTrail = 0;
		while (CDbHold::IsVarCharValid(pwStr[endIndex]))
		{
			if (pwStr[endIndex] == W_t(' '))
				++spcTrail;
			else
				spcTrail = 0;
			++endIndex;
		}

		const WSTRING wVarName(pwStr + index, endIndex - index - spcTrail);
		index = endIndex;

		//Is it a predefined var?
		int val = 0;
		const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wVarName);
		if (eVar != ScriptVars::P_NoVar)
		{
			if (!ScriptVars::IsStringVar(eVar)) {
				if (pNPC)
					val = int(pNPC->getPredefinedVarInt(eVar));
				else
					val = pGame->getVar(eVar);
			}
		} else if (ScriptVars::IsCharacterLocalVar(wVarName)) {
			val = pNPC ? pNPC->getLocalVarInt(wVarName) : 0;
		} else if (pGame->pHold->GetVarID(wVarName.c_str())) {
			//Is it a local hold var?
			char *varName = pGame->pHold->getVarAccessToken(wVarName.c_str());
			const UNPACKEDVARTYPE vType = pGame->stats.GetVarType(varName);
			const bool bValidInt = vType == UVT_int || vType == UVT_uint || vType == UVT_unknown;
			if (bValidInt)
				val = pGame->stats.GetVar(varName, (int)0);
		}

		//Is it a function primitive?
		ScriptVars::PrimitiveType ePrimitive = ScriptVars::parsePrimitive(wVarName);
		if (ePrimitive != ScriptVars::NoPrimitive)
			return parsePrimitive(ePrimitive, pwStr, index, pGame, pNPC);

		//else: unrecognized identifier -- just return a zero value
		return val;
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
	}
	else {
		LogParseError(pwStr, "Parse error in primitive parameter list (missing close parenthesis)");
	}

	if (params.size() == ScriptVars::getPrimitiveRequiredParameters(ePrimitive))
		return pGame->EvalPrimitive(ePrimitive, params);

	LogParseError(pwStr, "Parse error in primitive parameter list (incorrect argument count)");
	return 0;
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

	//If stunned, skip turn
	if (IsStunned())
		return;

	CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
	CDbRoom& room = *(pGame->pRoom);
	CSwordsman& player = pGame->swordsman;

	//Quick exit when done.
	if (this->wCurrentCommandIndex >= this->commands.size())
		goto Finish;

	//Sword cache must be cleared to avoid using state of the room from some other entity's
	//turn and/or being blocked by this character's own weapon
	this->swordsInRoom.Clear();

	//Only character monsters taking up a single tile are implemented.
	ASSERT(!bIsSerpentOrGentryii(GetResolvedIdentity()));

	{ //Wrap variables initialized within jump, to make g++ happy

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

				if (this->bVisible) break; //already in room
				const UINT identity = GetResolvedIdentity();
				if (identity >= CHARACTER_TYPES)
					break;	//nothing to show -- can't appear

				//Ensure square is available before appearing.
				ASSERT(room.IsValidColRow(this->wX, this->wY));
				if (room.GetMonsterAtSquare(this->wX, this->wY) != NULL ||
						pGame->IsPlayerAt(this->wX, this->wY) ||
						room.IsSwordAt(this->wX, this->wY))
					STOP_COMMAND;

				//Place character on starting square.
				this->bVisible = true;
				SetWeaponSheathed();
				room.SetMonsterSquare(this);

				//Check for stepping on pressure plate.
				if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && CanPressPressurePlates())
					room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);

				if (this->eImperative == ScriptFlag::RequiredToConquer)
				{
					++room.wMonsterCount;
					CueEvents.Add(CID_NPCTypeChange);
				}

				if (this->bBrainPathmapObstacle)
					room.UpdatePathMapAt(this->wX, this->wY);

				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;
			case CCharacterCommand::CC_AppearAt:
			{
				//Appear at square (x,y).
				bProcessNextCommand = true;

				if (this->bVisible) break; //already in room
				const UINT identity = GetResolvedIdentity();
				if (identity >= CHARACTER_TYPES)
					break;	//nothing to show -- can't appear

				//Ensure square is available before appearing.
				getCommandXY(command, px, py);
				if (!room.IsValidColRow(px,py) ||
						room.GetMonsterAtSquare(px, py) != NULL ||
						pGame->IsPlayerAt(px, py) ||
						room.IsSwordAt(px, py))
					STOP_COMMAND;

				//Place character on starting square.
				this->bVisible = true;
				this->wPrevX = this->wX = px;
				this->wPrevY = this->wY = py;
				ASSERT(!room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)]);
				room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = this;
				SetWeaponSheathed();

				//Check for stepping on pressure plate.
				if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && CanPressPressurePlates())
					room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);

				if (this->eImperative == ScriptFlag::RequiredToConquer)
				{
					++room.wMonsterCount;
					CueEvents.Add(CID_NPCTypeChange);
				}

				if (this->bBrainPathmapObstacle)
					room.UpdatePathMapAt(this->wX, this->wY);

				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;
			case CCharacterCommand::CC_Disappear:
			{
				//Remove character from room (i.e. remove from view and action,
				//but script keeps running).
				bProcessNextCommand = true;

				if (!this->bVisible) break; //not in room

				if (this->eImperative == ScriptFlag::RequiredToConquer)
				{
					room.DecMonsterCount();
					CueEvents.Add(CID_NPCTypeChange);
				}

				Disappear();

				if (this->bBrainPathmapObstacle)
					room.UpdatePathMapAt(this->wX, this->wY);

				bExecuteNoMoveCommands = true;	//allow executing commands that don't require moves immediately
			}
			break;

			case CCharacterCommand::CC_MoveTo:
			{
				//Move to square (x,y) or target set in flags.
				//If w is set, then forbid turning while moving.
				//If h is set, then take only a single step before advancing to next command.
				//However, if the NPC is not visible, then a coord change to the destination occurs

				if ((bExecuteNoMoveCommands && IsVisible()) || this->bPreventMoveAfterPush)
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
					{
						if (!(pDest = room.GetMonsterOfType(M_HALPH)))
							pDest = room.GetMonsterOfType(M_HALPH2);
					}
					else if ((pflags & ScriptFlag::MONSTER) != 0)
						pDest = room.pFirstMonster;
					else if ((pflags & ScriptFlag::NPC) != 0)
						pDest = room.GetMonsterOfType(M_CHARACTER);
					else if ((pflags & ScriptFlag::PDOUBLE) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_MIMIC)))
							if (!(pDest = room.GetMonsterOfType(M_DECOY)))
								if (!(pDest = room.GetMonsterOfType(M_CLONE)))
									pDest = room.GetMonsterOfType(M_TEMPORALCLONE);
					}
					else if ((pflags & ScriptFlag::SELF) != 0)
						break; //always at this position by definition
					else if ((pflags & ScriptFlag::SLAYER) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_SLAYER)))
							pDest = room.GetMonsterOfType(M_SLAYER2);
					}
					else if ((pflags & ScriptFlag::BEETHRO) != 0)
					{
						if (bIsSmitemaster(player.wAppearance))
							pDest = (CCoord*)&player;
						else
							pDest = room.GetNPCBeethro();
					}
					else if ((pflags & ScriptFlag::STALWART) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_STALWART)))
							pDest = room.GetMonsterOfType(M_STALWART2);
					}
					if (!pDest)
						STOP_COMMAND;

					wDestX = pDest->wX;
					wDestY = pDest->wY;
				}

				//When not visible, go to destination instantly.
				if (!IsVisible())
				{
					CCoordSet coords(this->wX, this->wY);
					coords.insert(wDestX, wDestY);

					this->wX = wDestX;
					this->wY = wDestY;

					room.Plot(coords); //update room
				}

				if (this->wX == wDestX && this->wY == wDestY)
				{
					//At destination -- done with this command.
					bProcessNextCommand = true;
					break;
				}

				int dx, dy, dxFirst, dyFirst;
				bool bPathmapping;
				const bool bAllowTurning = !pw;
				const bool bStopTurn = GetMovement(wDestX, wDestY, dx, dy, dxFirst, dyFirst, bPathmapping, bAllowTurning);

				//When pathfinding indicates to not move, 'Single Step' causes script execution to advance on the next turn.
				if (bStopTurn && !ph)
					STOP_COMMAND;
				if (bStopTurn || (!dx && !dy))
				{
					if (ph)
					{
						//If single step, then advance to next command when can't move
						if (bAllowTurning)  //allow turning to face the intended direction
						{
							if (!TurnsSlowly())
								SetOrientation(dxFirst,dyFirst);
						}
						break;
					}
					STOP_COMMAND;
				}

				//If monster type has a sword, then it must rotate slowly, and
				//it can't move on the same turn it is rotating.
				if (bAllowTurning)
					if (TurnsSlowly())
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

				//If moving toward a target entity, the NPC can't step on it
				//unless it's the player and the NPC can kill him,
				//so don't try to move if already adjacent.
				if (!(pflags && nDist(this->wX, this->wY, wDestX, wDestY) == 1 &&
						(!(pflags & ScriptFlag::PLAYER) || this->bSafeToPlayer)))
					MoveCharacter(dx, dy, bAllowTurning, CueEvents);

				if (bPathmapping) {
					this->pathToDest.Pop();  //has now made this move
				} else {
					this->goal.wX = UINT(-1); //invalidate
				}

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
				//However, if the NPC is not visible, then a coord change to the destination occurs
				if ((bExecuteNoMoveCommands && IsVisible()) || this->bPreventMoveAfterPush)
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
					if (!px && !py)
					{
						//Relative movement is zero -- nothing to do.
						bProcessNextCommand = true;
						break;
					}
					int nXDest = (int)this->wX + (int)px;
					int nYDest = (int)this->wY + (int)py;
					if (nXDest < 0)
						nXDest = 0;
					else if (nXDest >= (int)room.wRoomCols)
						nXDest = room.wRoomCols - 1;
					if (nYDest < 0)
						nYDest = 0;
					else if (nYDest >= (int)room.wRoomRows)
						nYDest = room.wRoomRows - 1;

					//When not visible, go to destination instantly.
					if (!IsVisible())
					{
						CCoordSet coords(this->wX, this->wY);
						coords.insert(nXDest, nYDest);

						this->wX = nXDest;
						this->wY = nYDest;

						room.Plot(coords); //update room
						bProcessNextCommand = true;
						break;
					}

					this->wXRel = nXDest;
					this->wYRel = nYDest;
					this->bMovingRelative = true;
				}

				//Reset from pathmapping
				this->goal.wX = UINT(-1); //invalidate

				int dx, dy, dxFirst, dyFirst;
				bool bPathmapping;
				const bool bAllowTurning = !pw;
				const bool bStopTurn = GetMovement(this->wXRel, this->wYRel, dx, dy, dxFirst, dyFirst, bPathmapping, bAllowTurning);

				//When pathfinding indicates to not move, 'Single Step' causes script execution to advance on the next turn.
				if (bStopTurn && !ph)
					STOP_COMMAND;
				if (bStopTurn || (!dx && !dy))
				{
					if (ph && IsVisible())
					{
						//If single step, then advance to next command when can't move
						if (bAllowTurning)  //allow turning to face the intended direction
						{
							if (!TurnsSlowly())
								SetOrientation(dxFirst,dyFirst);
						}
						this->bMovingRelative = false;
						break;
					}
					STOP_COMMAND;
				}

				//If monster type has a sword, then it must rotate slowly, and
				//it can't move on the same turn it is rotating.
				if (bAllowTurning)
					if (TurnsSlowly())
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

				if (bPathmapping) {
					this->pathToDest.Pop();  //has now made this move
				} else {
					this->goal.wX = UINT(-1); //invalidate
				}

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
				if ((bExecuteNoMoveCommands && this->bVisible) || this->bPreventMoveAfterPush)
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
				SetWeaponSheathed();
				if (!this->bVisible || //turning doesn't take time when not in room
						wOldO == this->wO) //already facing this way
					bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_FaceTowards:
			{
				//Turn to face indicated direction.
				if (!this->wJumpLabel){
					if ((bExecuteNoMoveCommands && this->bVisible) || this->bPreventMoveAfterPush)
					{
						goto Finish;
					}
				}

				//Face toward square (x,y) or target set by flags.
				UINT wDestX, wDestY;
				getCommandParams(command, px, py, pw, ph, pflags);
				wDestX = px;
				wDestY = py;

				if (wDestX == this->wX && wDestY == this->wY)
				{
					bProcessNextCommand = true;
					break;
				}

				if (!room.IsValidColRow(px, py))
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
					{
						if (!(pDest = room.GetMonsterOfType(M_HALPH)))
							pDest = room.GetMonsterOfType(M_HALPH2);
					}
					else if ((pflags & ScriptFlag::MONSTER) != 0)
						pDest = room.pFirstMonster;
					else if ((pflags & ScriptFlag::NPC) != 0)
						pDest = room.GetMonsterOfType(M_CHARACTER);
					else if ((pflags & ScriptFlag::PDOUBLE) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_MIMIC)))
						if (!(pDest = room.GetMonsterOfType(M_DECOY)))
						if (!(pDest = room.GetMonsterOfType(M_CLONE)))
							pDest = room.GetMonsterOfType(M_TEMPORALCLONE);
					}
					else if ((pflags & ScriptFlag::SELF) != 0)
						break; //always at this position by definition
					else if ((pflags & ScriptFlag::SLAYER) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_SLAYER)))
							pDest = room.GetMonsterOfType(M_SLAYER2);
					}
					else if ((pflags & ScriptFlag::BEETHRO) != 0)
					{
						if (bIsSmitemaster(player.wAppearance))
							pDest = (CCoord*)&player;
						else
							pDest = room.GetNPCBeethro();
					}
					else if ((pflags & ScriptFlag::STALWART) != 0)
					{
						if (!(pDest = room.GetMonsterOfType(M_STALWART)))
							pDest = room.GetMonsterOfType(M_STALWART2);
					}
					if (!pDest)
						STOP_COMMAND;

					wDestX = pDest->wX;
					wDestY = pDest->wY;
				}

				const UINT wO = this->GetOrientationFacingTarget(wDestX, wDestY);
				if (this->wJumpLabel)
				{
					if (wO != this->wO){
						STOP_COMMAND;
					}
					else 
					{
						bProcessNextCommand = true;
					}
				}
				else 
				{
					const UINT wOldO = this->wO;
					if (pw == 1)
					{
						this->MakeSlowTurn(wO);
					}
					else 
					{
						this->wO = wO;
						SetWeaponSheathed();
					}

					if (!this->bVisible || wOldO == this->wO)
						bProcessNextCommand = true;
				}
			}
			break;
			case CCharacterCommand::CC_TeleportTo:
			{
				getCommandXY(command, px, py);
				if (room.IsValidColRow(px, py) &&
						(px != this->wX || py != this->wY) &&
						(!IsVisible() || (!room.GetMonsterAtSquare(px, py) &&
						!this->pCurrentGame->IsPlayerAt(px, py))))
				{
					if (px != this->wX) this->wPrevX = this->wX;
					if (py != this->wY) this->wPrevY = this->wY;
					TeleportCharacter(px, py, CueEvents);
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_TeleportPlayerTo:
			{
				getCommandXY(command, px, py);
				if (player.IsInRoom())
				{
					pGame->TeleportPlayer(px, py, CueEvents);
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_ActivateItemAt:
			{
				getCommandXY(command, px, py);
				if (!room.IsValidColRow(px, py))
					break;

				//Activate item at (x,y).  Works only for the following items.
				const UINT lightVal = room.tileLights.GetAt(px, py);
				if (bIsWallLightValue(lightVal))
				{
					//Toggle wall light.  Doesn't expend a turn.
					room.tileLights.Add(px, py, WALL_LIGHT + ((lightVal + LIGHT_OFF) % 256));
					room.SetRoomLightingChanged();
					bProcessNextCommand = true;
				}
				//Powder Kegs take priority over other tile types
				if (room.GetTSquare(px,py) == T_POWDER_KEG)
				{
					if (pGame->wTurnNo > 0) //not on room entrance, since the player could die immediately
					{
						//Explode immediately.  Doesn't expend a turn.
						room.ExplodePowderKeg(CueEvents, px, py);
						if (IsAlive())
							bProcessNextCommand = true;
						break;
					} else {
						//Pause script until after first turn to explode.
						STOP_COMMAND;
					}
				}
				switch (room.GetBottomTSquare(px, py))
				{
					case T_LIGHT:
						//Toggle a light.  Doesn't expend a turn.
						room.ToggleLight(px, py);
						bProcessNextCommand = true;
					break;
					case T_TOKEN:
						//Activate a token.  Doesn't expend a turn.
						room.ActivateToken(CueEvents, px, py);
						bProcessNextCommand = true;
					break;

					case T_BOMB:
						if (pGame->wTurnNo > 0) //not on room entrance, since the player could die immediately
						{
							//Explode a bomb immediately.  Doesn't expend a turn.
							room.ExplodeBomb(CueEvents, px, py);
							if (IsAlive())
								bProcessNextCommand = true;
						} else {
							//Pause script until after first turn to explode the bomb.
							STOP_COMMAND;
						}
					break;
					case T_FUSE:
						//Light fuse.  Doesn't expend a turn.
						if (!bIsTLayerCoveringItem(room.GetTSquare(px, py)))
							room.LightFuseEnd(CueEvents, px, py);
						bProcessNextCommand = true;
					break;

					case T_ORB:
						//Activate orb.
						if (bExecuteNoMoveCommands) return;
						room.ActivateOrb(px, py, CueEvents, OAT_ScriptOrb);
					break;
					case T_BEACON: case T_BEACON_OFF:
						//Activate beacon.  Doesn't expend a turn.
						if (bExecuteNoMoveCommands) return;
						room.ActivateBeacon(px, py, CueEvents);

						bProcessNextCommand = true;
					break;
					case T_FIRETRAP: case T_FIRETRAP_ON:
						//Toggle firetrap.  Doesn't expend a turn.
						room.ToggleFiretrap(px, py, CueEvents);
						bProcessNextCommand = true;
					break;

					default:
						if (room.GetOSquare(px, py) == T_PRESSPLATE)
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
						} else {
							//No item to active on this tile.  Just continue script.
							bProcessNextCommand = true;
						}
					break;
				}
			}
			break;
			case CCharacterCommand::CC_AttackTile:
			{
				bProcessNextCommand = true;

				getCommandParams(command, px, py, pw, ph, pflags);
				if (!room.IsValidColRow(px, py))
					break;
				switch(pflags)
				{
					case ScriptFlag::AT_Stab:
					{
						WeaponStab stab(px, py);
						stab.pArmedMonster = this;
						pGame->StabRoomTile(stab, CueEvents);
					}
					break;
					case ScriptFlag::AT_Explode:
						switch (room.GetTSquare(px, py))
						{
							case T_BOMB:
								//Don't allow player death on turn 0.
								if (pGame->wTurnNo > 0) {
									room.ExplodeBomb(CueEvents, px, py);
								} else {
									STOP_COMMAND;
								}
							break;
							case T_POWDER_KEG:
								if (pGame->wTurnNo > 0) {
									room.ExplodePowderKeg(CueEvents, px, py);
								} else {
									STOP_COMMAND;
								}
							break;
							default:
								if (pGame->wTurnNo || !pGame->IsPlayerAt(px,py))
								{
									CCoordStack no_kegs, no_bombs;
									CCoordIndex ignoreCaber;
									ignoreCaber.Init(room.wRoomCols, room.wRoomRows);
									CCoordSet explosion;
									room.DoExplodeTile(CueEvents, no_bombs, no_kegs, explosion, ignoreCaber, px, py, 0, false);
									room.ProcessExplosionSquare(CueEvents, px, py, true, false);
									room.ConvertUnstableTar(CueEvents);
								}
							break;
						}
					break;
					case ScriptFlag::AT_Damage:
					{
						if (pGame->wTurnNo && !IsSwordSafeToPlayer()
							&& pGame->IsPlayerAt(px,py) && pGame->swordsman.IsStabbable())
						{
							pGame->SetDyingEntity(&pGame->swordsman, this);
							CueEvents.Add(CID_MonsterKilledPlayer, this);
						}
						CMonster *pMonster = room.GetMonsterAtSquare(px,py);
						if (pMonster)
						{
							pMonster = pMonster->GetOwningMonster();
							switch(pMonster->wType)
							{
								case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
								{
									CSerpent *pSerpent = dynamic_cast<CSerpent*>(pMonster);
									if (pSerpent->ShortenTail(CueEvents))
									{
										room.KillMonster(pSerpent,CueEvents,false,this);
										pGame->TallyKill();
									}
								}
								break;
								case M_ROCKGIANT:
									room.KillMonster(pMonster, CueEvents, false, this);
									CRockGiant::Shatter(CueEvents, pGame, pMonster->wX, pMonster->wY);
									pGame->TallyKill();
								break;
								case M_ROCKGOLEM: case M_CONSTRUCT:
								{
									CRockGolem *pGolem = dynamic_cast<CRockGolem*>(pMonster);
									if (pGolem->OnStabbed(CueEvents))
										pGame->TallyKill();
									else
										room.KillMonster(pGolem, CueEvents, false, this);
								}
								break;
								case M_FEGUNDO:
									pGame->FegundoToAsh(pMonster, CueEvents);
								break;
								case M_CHARACTER:
								{
									CCharacter *pCharacter = dynamic_cast<CCharacter*>(pMonster);
									if (!pCharacter->IsInvulnerable())
									{
										room.KillMonster(pCharacter, CueEvents, false, this);
										if (pCharacter->IsRequiredToConquer())
											pGame->TallyKill();
									}
								}
								break;
								default:
									room.KillMonster(pMonster, CueEvents, false, this);						
									pGame->CheckTallyKill(pMonster->wType);
									if (bIsMother(pMonster->wType))
										room.FixUnstableTar(CueEvents);
								break;
							}
						}
					}
					break;
					case ScriptFlag::AT_Kill:
					{
						if (pGame->wTurnNo && pGame->IsPlayerAt(px,py))
						{
							pGame->SetDyingEntity(&pGame->swordsman, this);
							CueEvents.Add(CID_MonsterKilledPlayer, this);
						}
						CMonster *pMonster = room.GetMonsterAtSquare(px,py);
						if (pMonster)
						{
							pMonster = pMonster->GetOwningMonster();
							room.KillMonster(pMonster, CueEvents, false, this);						
							pGame->CheckTallyKill(pMonster->wType);
							if (bIsMother(pMonster->wType))
								room.FixUnstableTar(CueEvents);
						}
					}
					break;
					default:
					break;
				}
			}
			break;
			case CCharacterCommand::CC_PushTile:
			{
				getCommandParams(command, px, py, pw, ph, pflags);
				WeaponStab push(px, py, pw, WeaponType::WT_Staff);
				pGame->ProcessScriptedPush(push, CueEvents, this);
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
					if (bExecuteNoMoveCommands || this->bPreventMoveAfterPush)
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
					if (player.IsInRoom() &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::HALPH) != 0)
				{
					if ((player.wAppearance == M_HALPH || player.wAppearance == M_HALPH2) &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_HALPH, true))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_HALPH2, true))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::MONSTER) != 0)
				{
					//excludes player doubles, friendly enemies, and NPCs
					if (!bIsHuman(player.wAppearance) &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else if (room.IsMonsterInRect(px, py,
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
					if (bIsBeethroDouble(player.wAppearance) && !bIsSmitemaster(player.wAppearance) &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_MIMIC))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_DECOY))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_CLONE))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_TEMPORALCLONE))
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
					if ((player.wAppearance == M_SLAYER || player.wAppearance == M_SLAYER2) &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_SLAYER, true))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_SLAYER2, true))
						bFound = true;
				}
				if (!bFound && (pflags & ScriptFlag::BEETHRO) != 0)
				{
					//Check for Beethro (can detect NPC Beethros).
					if (bIsSmitemaster(player.wAppearance))
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
					if (bIsStalwart(player.wAppearance) &&
							player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_STALWART, true))
						bFound = true;
					else if (room.IsMonsterInRectOfType(px, py,
							px + pw, py + ph, M_STALWART2, true))
						bFound = true;
				}

				if ((command.command == CCharacterCommand::CC_WaitForRect && !bFound) ||
					 (command.command == CCharacterCommand::CC_WaitForNotRect && bFound))
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_WaitForEntityType:
			case CCharacterCommand::CC_WaitForNotEntityType:
			{
				//Wait until a specified entity type is in rect (x,y,w,h).
				// -OR-
				//Wait until NO specified entity type is in rect (x,y,w,h).
				//
				//Note that width and height are zero-indexed.
				bool bFound = false;
				getCommandParams(command, px, py, pw, ph, pflags);
				if (!room.IsValidColRow(px, py) || !room.IsValidColRow(px+pw, py+ph))
					STOP_COMMAND;
				if (pflags == M_NONE)
					STOP_COMMAND;

				if (player.wIdentity == pflags)
				{
					if (player.wX >= px && player.wX <= px + pw &&
							player.wY >= py && player.wY <= py + ph)
						bFound = true;
				}
				if (!bFound && room.IsMonsterInRectOfType(px, py, px + pw, py + ph, pflags, true, true))
					bFound = true;

				if ((command.command == CCharacterCommand::CC_WaitForEntityType && !bFound) ||
					 (command.command == CCharacterCommand::CC_WaitForNotEntityType && bFound))
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_WaitForRemains:
			{
				//Wait until a specified dead monster type is in rect (x,y,w,h).
				bool bFound = false;
				getCommandParams(command, px, py, pw, ph, pflags);
				if (!room.IsValidColRow(px, py) || !room.IsValidColRow(px + pw, py + ph))
					STOP_COMMAND;
				if (pflags == M_NONE)
					STOP_COMMAND;

				if (pflags == M_FEGUNDO) {
					// Fegundo remains are their own monster type, so special handling is required
					bFound = room.IsMonsterInRectOfType(px, py, px + pw, py + ph, M_FEGUNDOASHES);
				} else {
					bFound = room.IsMonsterRemainsInRectOfType(px, py, px + pw, py + ph, pflags);
				}

				if (!bFound)
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
			case CCharacterCommand::CC_WaitForCleanRoom:
			{
				//Wait until room has been conquered (i.e. green doors have opened).
				if (!room.bGreenDoorsOpened)
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForCleanLevel:
			{
				//Wait until level has been conquered (i.e. blue doors have opened).
				if (!pGame->IsCurrentLevelComplete())
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForPlayerToFace:
			{
				//Wait until player faces orientation X.
				if (!player.IsInRoom())
					STOP_COMMAND;

				getCommandX(command, px);
				switch (px)
				{
					case CMD_C:
						if (player.wO != nNextCO(player.wPrevO))
							STOP_COMMAND;
						break;
					case CMD_CC:
						if (player.wO != nNextCCO(player.wPrevO))
							STOP_COMMAND;
						break;
					default:
						if (player.wO != px)
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

				const bool bPlayerMoved = player.wX != player.wPrevX ||
						player.wY != player.wPrevY;
				getCommandX(command, px);
				switch (px)
				{
					case CMD_C:
						if (player.wO != nNextCO(player.wPrevO))
							STOP_COMMAND;
						break;
					case CMD_CC:
						if (player.wO != nNextCCO(player.wPrevO))
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
			case CCharacterCommand::CC_WaitForPlayerInput:
			{
				//Waits for player to input command X
				if (!player.IsInRoom())
					STOP_COMMAND;

				getCommandX(command, px);

				//CMD_EXEC_COMMAND is translated to CMD_WAIT, so we need to kludge handling here
				if (px == CMD_WAIT && CueEvents.HasOccurred(CID_CommandKeyPressed))
					STOP_COMMAND;  //nLastCommand is a false "wait"
				if (px == CMD_EXEC_COMMAND) {
					if (!CueEvents.HasOccurred(CID_CommandKeyPressed)) //only way to detect
						STOP_COMMAND;
				} else if (px != (UINT)nLastCommand)
					STOP_COMMAND;
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

			case CCharacterCommand::CC_WaitForSomeoneToPushMe:
				if (!this->bWasPushed)
					STOP_COMMAND;
				bProcessNextCommand = true;
			break;

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
						this->jumpStack.push_back(this->wCurrentCommandIndex+1);
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
				//We set this flag to false because this character will be used for as long as the speech
				//is not finished, so we don't want this object to be deleted next turn
				this->bSafeToDelete = false;
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

				//Only wait on question if it's being asked on room entrance.
				if (pGame->ExecutingNoMoveCommands())
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
				//Set music being played to playlist 'label' (or track X, or custom track Y).
				MusicData music;
				if (!command.label.empty()) {
					music.songlistKey = command.label;
				} else {
					music.trackID = command.x;
					music.customDataID = command.y;
				}
				pGame->SetMusicStyle(music, CueEvents);
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_Imperative:
			{
				//Set NPC imperative status.
				const ScriptFlag::Imperative eNewImperative = (ScriptFlag::Imperative)command.x;
				bool bChangeImperative = true;
				switch (eNewImperative)
				{
					case ScriptFlag::Safe:
						this->bSafeToPlayer = true;
						this->bFriendly = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::SwordSafeToPlayer:
						this->bSwordSafeToPlayer = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::EndWhenKilled:
						this->bEndWhenKilled = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::Deadly:
					{
						this->bSafeToPlayer = this->bSwordSafeToPlayer = false;
						bChangeImperative = false;
						const UINT identity = GetResolvedIdentity();
						if(!(identity == M_HALPH || identity == M_HALPH2 ||	bIsStalwart(identity)))
							this->bFriendly = false;
					}
					break;
					case ScriptFlag::DieSpecial:
						bChangeImperative = !HasSpecialDeath();
						//no break
					case ScriptFlag::Die:
						if (bExecuteNoMoveCommands && bChangeImperative)
							return; //wait until first move to die

						if (this->bBrainPathmapObstacle)
							room.UpdatePathMapAt(this->wX, this->wY);

						//Stop script execution whether visible or not.
						if (bChangeImperative)
							this->wCurrentCommandIndex = this->commands.size();
						if (IsAlive())
						{
							if (!this->bVisible)
								room.KillInvisibleCharacter(this);
							else {
								//NPC dies.
								if (!bExecuteNoMoveCommands) {
									if (!room.AddFallingMonsterEvent(CueEvents, this, room.GetOSquare(this->wX, this->wY)))
										CueEvents.Add(CID_MonsterDiedFromStab, this);
								}
								if (bChangeImperative)
								{
									//Normal death behavior.
									CCueEvents Ignored;
									SetKillInfo(NO_ORIENTATION); //center stab effect
									room.KillMonster(this, Ignored, true);
									if (IsMissionCritical())
										CriticalNPCDied(CueEvents);
								}
								else {
									//Special death behavior.
									switch (GetResolvedIdentity())
									{
										case M_ROCKGOLEM: case M_CONSTRUCT: this->wO = NO_ORIENTATION; break;
										case M_FEGUNDO: TurnIntoMonster(CueEvents, true); break;
										default: break;
									}
								}
							}
						}
					break;
					case ScriptFlag::DirectBeelining:
						this->movementIQ = DirectOnly;
						bChangeImperative = false;
					break;
					case ScriptFlag::NormalBeelining:
						this->movementIQ = SmartDiagonalOnly;
						bChangeImperative = false;
					break;
					case ScriptFlag::SmartBeelining:
						this->movementIQ = SmarterDiagonalOnly;
						bChangeImperative = false;
					break;
					case ScriptFlag::FlexibleBeelining:
						this->movementIQ = SmartOmniDirection;
						bChangeImperative = false;
					break;
					case ScriptFlag::Pathfinding:
						this->movementIQ = MIQ_Pathfind;
						bChangeImperative = false;
					break;
					case ScriptFlag::PathfindingOpenOnly:
						this->movementIQ = MIQ_PathfindOpenOnly;
						bChangeImperative = false;
					break;
					case ScriptFlag::NoGhostDisplay:
						this->eDisplayMode = CDM_Normal;
						bChangeImperative = false;
					break;
					case ScriptFlag::GhostDisplay:
						this->eDisplayMode = CDM_GhostFloor;
						bChangeImperative = false;
					break;
					case ScriptFlag::GhostDisplayOverhead:
						this->eDisplayMode = CDM_GhostOverhead;
						bChangeImperative = false;
					break;
					case ScriptFlag::InvisibleInspectable:
						this->bInvisibleInspectable = true;
						bChangeImperative = false;
						break;
					case ScriptFlag::InvisibleNotInspectable:
						this->bInvisibleInspectable = false;
						bChangeImperative = false;
						break;
					case ScriptFlag::NotPushable:
						this->bNotPushable = true;
						this->bPushableByBody = false;
						this->bPushableByWeapon = false;
						bChangeImperative = false;
					break;
					case ScriptFlag::DefaultPushability:
						this->bNotPushable = false;
						this->bPushableByBody = false;
						this->bPushableByWeapon = false;
						bChangeImperative = false;
					break;
					case ScriptFlag::PushableByBody:
						this->bNotPushable = false;
						this->bPushableByBody = true;
						this->bPushableByWeapon = false;
						bChangeImperative = false;
					break;
					case ScriptFlag::PushableByWeapon:
						this->bNotPushable = false;
						this->bPushableByBody = false;
						this->bPushableByWeapon = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::PushableByBoth:
						this->bNotPushable = false;
						this->bPushableByBody = true;
						this->bPushableByWeapon = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::Stunnable:
						this->bStunnable = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::NotStunnable:
						this->bStunnable = false;
						bChangeImperative = false;
					break;
					case ScriptFlag::BrainPathmapObstacle:
						this->bBrainPathmapObstacle = true;
						room.UpdatePathMapAt(this->wX, this->wY);
						bChangeImperative = false;
					break;
					case ScriptFlag::NotBrainPathmapObstacle:
						this->bBrainPathmapObstacle = false;
						room.UpdatePathMapAt(this->wX, this->wY);
						bChangeImperative = false;
					break;
					case ScriptFlag::NPCPathmapObstacle:
						this->bNPCPathmapObstacle = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::NotNPCPathmapObstacle:
						this->bNPCPathmapObstacle = false;
						bChangeImperative = false;
					break;
					case ScriptFlag::Friendly:
						this->bFriendly = true;
						bChangeImperative = false;
					break;
					case ScriptFlag::Unfriendly:
						this->bFriendly = false;
						bChangeImperative = false;
					break;
					default: break;
				}
				if (bChangeImperative)
				{
					//Character's status has been modified -- update room state.
					const ScriptFlag::Imperative eOldImperative = this->eImperative;
					if (this->bVisible && eNewImperative != eOldImperative)
					{
						if (eOldImperative == ScriptFlag::RequiredToConquer)
						{
							if (IsAlive()) //Monster count may've already been decremented by Die/DieSpecial
								room.DecMonsterCount();
							CueEvents.Add(CID_NPCTypeChange);
						}
						else if (eNewImperative == ScriptFlag::RequiredToConquer)
						{
							++room.wMonsterCount;
							CueEvents.Add(CID_NPCTypeChange);
						}
					}

					this->eImperative = eNewImperative;
				}
				if (IsAlive())
					bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_Behavior:
			{
				// Set NPC behavior flag
				const ScriptFlag::Behavior eBehavior = (ScriptFlag::Behavior)command.x;
				const bool activate = (bool)command.y;

				if (activate) {
					behaviorFlags.insert(eBehavior);
					// some behaviors need to update other locations
					switch (eBehavior) {
						case ScriptFlag::MonsterTarget:
							// For simplicity, just add this NPC to the list of monster enemies
							if (!HasBehavior(ScriptFlag::MonsterTargetWhenPlayerIsTarget))
								room.monsterEnemies.push_back(this);
						break;
						case ScriptFlag::MonsterTargetWhenPlayerIsTarget:
							// For simplicity, just add this NPC to the list of monster enemies
							if (!HasBehavior(ScriptFlag::MonsterTarget))
								room.monsterEnemies.push_back(this);
						break;
						default: break;
					}
				} else {
					behaviorFlags.erase(eBehavior);
					// some behaviors need to update other locations
					switch (eBehavior) {
						case ScriptFlag::BriarImmune:
							// This NPC can no longer block briars, so act as if a tile was plotted.
							room.briars.plotted(this->wX, this->wY, T_EMPTY);
						break;
						case ScriptFlag::MonsterTarget:
							if (!HasBehavior(ScriptFlag::MonsterTargetWhenPlayerIsTarget))
								room.monsterEnemies.remove(this);
						break;
						case ScriptFlag::MonsterTargetWhenPlayerIsTarget:
							if (!HasBehavior(ScriptFlag::MonsterTarget))
								room.monsterEnemies.remove(this);
						break;
						default: break;
					}
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_SetMovementType:
			{
				const MovementType eNewMovementType = (MovementType)command.x;

				if (this->bIfBlock)
				{
					//When used with if, check if movement type equals X
					if (this->eMovement != eNewMovementType)
						STOP_COMMAND;
				}	else {
					//Set movement type X
					this->eMovement = eNewMovementType;
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_TurnIntoMonster:
				TurnIntoMonster(CueEvents);
			break;
			case CCharacterCommand::CC_EndScript:
				//Remove character from any future play in the current game.
				this->bScriptDone = true;
				this->wCurrentCommandIndex = this->commands.size();

				if (!this->bVisible) // Invisible ended characters should just be removed from the room
					room.KillInvisibleCharacter(this);

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
				}
				bProcessNextCommand = true;
			break;

			case CCharacterCommand::CC_StartGlobalScript:
			{
				UINT dwCharID = (UINT)command.x;
				if (!pGame->GlobalScriptsRunning.has(dwCharID))
					room.AddNewGlobalScript(dwCharID, true, CueEvents);

				bProcessNextCommand = true;
			}
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
				if (command.command == CCharacterCommand::CC_IfElseIf && this->bParseIfElseAsCondition){
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
				if (!pGame->wPlayerTurn)
					return; //don't execute on the room entrance move -- execute next turn

				//When saving room data in GotoLevelEntrance,
				//this NPC should start at the next script command
				//next time the script is processed.
				++this->wCurrentCommandIndex;

				getCommandXY(command, px, py); //NOTE: only py is considered here
				if (!CueEvents.HasOccurred(CID_ExitLevelPending)) //don't queue more than one level exit
					pGame->GotoLevelEntrance(CueEvents, command.x, py != 0);

				// revert to current command so it increments correctly for global scripts
				// or to try again if somehow it failed
				--this->wCurrentCommandIndex;
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
					CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster* ,pMonster);
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
					//Sets player to look like entity X.
					pGame->SetPlayerRole(command.x, CueEvents);
				}
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_SetNPCAppearance:
			{
				//Sets this NPC to look like entity X.
				UINT wPreviousIdentity = this->wIdentity;
				this->wIdentity = this->wLogicalIdentity = command.x;
				ResolveLogicalIdentity(pGame->pHold);
				// When the underlying identity is changed, update default behaviors
				if (!command.y && wIdentity != wPreviousIdentity) {
					SetDefaultBehaviors();
					SetDefaultMovementType();
				}
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

			case CCharacterCommand::CC_PlayerEquipsWeapon:
			{
				//(deprecated) If X is set, player is given a weapon, else it is taken away.
				player.bWeaponOff = !command.x;
				pGame->SetCloneWeaponsSheathed(); //synch clones
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_SetPlayerWeapon:
			{
				getCommandX(command, px);

				const bool bArm = px == (UINT)WT_Off || px == (UINT)WT_On;
				if (this->bIfBlock)
				{
					//As an If condition, this acts as a query that is true when
					//the player's weapon state matches the specified parameter value.
					if (bArm) {
						if (px == (UINT)WT_Off) {
							if (!player.bWeaponOff && !player.bNoWeapon)
								STOP_COMMAND;
						} else {
							if (player.bWeaponOff)
								STOP_COMMAND;
						}
					} else {
						if (px != (UINT)player.GetActiveWeapon())
							STOP_COMMAND;
					}
				} else {
					//Sets the player weapon type to X (incl. on/off -- replaces CC_PlayerEquipsWeapon)
					player.EquipWeapon(px);
					if (bArm)
						pGame->SetCloneWeaponsSheathed(); //synch clones
				}
				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_SetPlayerStealth:
			{
				const bool bOldTarget = player.IsTarget();
				player.wStealth = command.x;
				if (bOldTarget != player.IsTarget())
					pGame->pRoom->CreatePathMaps(); //update pathmaps
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_SetWaterTraversal:
			{
				if (player.wWaterTraversal != command.x)
				{
					bool wOldCanHide = player.GetWaterTraversalState() == WTrv_CanHide;
					player.wWaterTraversal = command.x;
					bool wNewCanHide = player.GetWaterTraversalState() == WTrv_CanHide;

					//Check if we need to update sword sheathing/invisibility
					if (wOldCanHide != wNewCanHide)
					{
						pGame->SetPlayerWeaponSheathedState();
						pGame->SetCloneWeaponsSheathed(); //synch clones
					}
				}

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForItem:
				//Wait for game element (flags) to exist in rect (x,y,w,h).
				if (!IsTileAt(command))
					STOP_COMMAND;
				bProcessNextCommand = true;
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
						(identity == M_NEATHER ||
						 room.GetMonsterAtSquare(px, py) != NULL ||
							pGame->IsPlayerAt(px, py)))
					STOP_COMMAND;

					//Place new entity on this tile.
					if (pw == CMD_C)
						pw = nNextCO(this->wO);
					else if (pw == CMD_CC)
						pw = nNextCCO(this->wO);

					this->GenerateEntity(identity, px, py, pw, CueEvents);

					CueEvents.Add(CID_NPCTypeChange);
				}

				bProcessNextCommand = true;
			}
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
				CDbSpeech *pSpeech = command.pSpeech;
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

				CDbSpeech *pSpeech = command.pSpeech;
				ASSERT(pSpeech);
				const WSTRING text = pGame->ExpandText((const WCHAR*)pSpeech->MessageText, this);
				CDbMessageText *pText = new CDbMessageText();
				*pText = text.c_str();
				CColorText *pColorText = new CColorText(pText, px, py, pw, ph);
				CueEvents.Add(CID_FlashingMessage, pColorText, true);

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_DisplayFilter:
				getCommandX(command, px);
				pGame->displayFilter = px;
				CueEvents.Add(CID_SetDisplayFilter);

				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_ImageOverlay:
			{
				//Display image (w) with display strategy (label).
				const WSTRING text = pGame->ExpandText(command.label.c_str(), this);
				CueEvents.Add(CID_ImageOverlay, new CImageOverlay(text, command.w, pGame->GetNextImageOverlayID()), true);

				bProcessNextCommand = true;
			}
			break;

			case CCharacterCommand::CC_DestroyTrapdoor:
			{
				getCommandRect(command, px, py, pw, ph);
				for (UINT y=py; y <= py + ph && y < room.wRoomRows; ++y)
					for (UINT x=px; x <= px + pw && x < room.wRoomCols; ++x)
						if (bIsFallingTile(room.GetOSquare(x,y)))
							room.DestroyTrapdoor(x, y, CueEvents);

				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_BuildMarker:
			{
				//Mark rect (x,y,w,h) for building game element (flags).
				BuildMarker(command);
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_Build:
			{
				//Fill rect (x,y,w,h) with building game element (flags).
				BuildTiles(command, CueEvents);
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForNoBuilding:
			{
				bool bFound = false;
				//Wait until no build markers are queued in rect (x,y,w,h).
				getCommandRect(command, px, py, pw, ph);
				for (UINT y=py; !bFound && y <= py + ph; ++y)
					for (UINT x=px; !bFound && x <= px + pw; ++x)
						if (room.building.get(x,y))
							bFound = true;
				if (bFound)
					STOP_COMMAND;
				bProcessNextCommand = true;
			}
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

			case CCharacterCommand::CC_WorldMapSelect:
				//Sets the world map that other world map commands operate on to X.
				this->worldMapID = command.x;
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_WorldMapMusic:
				//Sets the music to be played on preselected world map.
				if (!command.label.empty())
					pGame->SetWorldMapMusic(this->worldMapID, command.label);
				else
					pGame->SetWorldMapMusic(this->worldMapID, command.x, command.y);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_WorldMapIcon:
				//Places an icon on the preselected world map at (x,y), to level entrance (w), displayed as custom NPC (h), of display type (flags).
				getCommandParams(command, px, py, pw, ph, pflags);
				pGame->SetWorldMapIcon(this->worldMapID, px, py, pw, ph, 0, pflags);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_WorldMapImage:
				//Places an icon on the preselected world map at (x,y), to level entrance (w), displayed as image (h), of display type (flags).
				getCommandParams(command, px, py, pw, ph, pflags);
				pGame->SetWorldMapIcon(this->worldMapID, px, py, pw, 0, ph, pflags);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_WaitForOpenMove:
				getCommandX(command, px);

				if (!this->IsOpenMove(nGetOX(px), nGetOY(px)))
					STOP_COMMAND;
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_ChallengeCompleted:
				//Indicates scripted requirements for an architect's challenge (text) have been satisfied.
				CueEvents.Add(CID_ChallengeCompleted, new CAttachableWrapper<WSTRING>(command.label), true);
				bProcessNextCommand = true;
			break;
			case CCharacterCommand::CC_GetNaturalTarget:
			{
				UINT wX = this->wX;
				UINT wY = this->wY;
				getCommandX(command, px);
				
				switch (px){
					case(ScriptFlag::RegularMonster):
						this->GetTarget(wX, wY, true);
						break;
				}
				
				pGame->ProcessCommandSetVar(ScriptVars::P_RETURN_X, wX);
				pGame->ProcessCommandSetVar(ScriptVars::P_RETURN_Y, wY);
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_GetEntityDirection:
			{
				UINT wOrientation = NO_ORIENTATION;
				getCommandXY(command, px, py);
				

				if (pGame->swordsman.IsAt(px, py)){
					wOrientation = pGame->swordsman.wO;
				} else {
					CMonster* pMonster = pGame->pRoom->GetMonsterAtSquare(px, py);
					if (pMonster != NULL){
						wOrientation = pMonster->wO;
					}
				}

				pGame->ProcessCommandSetVar(ScriptVars::P_RETURN_X, wOrientation);
				bProcessNextCommand = true;
			}
			break;
			case CCharacterCommand::CC_WaitForWeapon:
			{
				getCommandParams(command, px, py, pw, ph, pflags);

				bool bPlayerWeapon = this->pCurrentGame->IsPlayerWeaponAt(px, py);
				bool bMonsterWeapon = room.IsMonsterSwordAt(px, py);
				bool bFound = false;

				if (!(bPlayerWeapon || bMonsterWeapon))
				{
					STOP_COMMAND;
				}

				if (!pflags) {
					bFound = true;
				}

				if (!bFound && (pflags & ScriptFlag::WEAPON_SWORD) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Sword)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Sword);
					}
				}
				if (!bFound && (pflags & ScriptFlag::WEAPON_PICKAXE) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Pickaxe)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Pickaxe);
					}
				}
				if (!bFound && (pflags & ScriptFlag::WEAPON_SPEAR) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Spear)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Spear);
					}
				}
				if (!bFound && (pflags & ScriptFlag::WEAPON_STAFF) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Staff)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Staff);
					}
				}
				if (!bFound && (pflags & ScriptFlag::WEAPON_DAGGER) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Dagger)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Dagger);
					}
				}
				if (!bFound && (pflags & ScriptFlag::WEAPON_CABER) != 0)
				{
					if (bPlayerWeapon && player.GetActiveWeapon() == WT_Caber)
					{
						bFound = true;
					}
					else if (bMonsterWeapon)
					{
						bFound = room.IsMonsterWeaponTypeAt(px, py,
							WT_Caber);
					}
				}

				if (!bFound)
					STOP_COMMAND;

				bProcessNextCommand = true;
			}
			break;

			//Deprecated commands
			case CCharacterCommand::CC_GotoIf:
			case CCharacterCommand::CC_WaitForHalph:
			case CCharacterCommand::CC_WaitForNotHalph:
			case CCharacterCommand::CC_WaitForMonster:
			case CCharacterCommand::CC_WaitForNotMonster:
			case CCharacterCommand::CC_WaitForCharacter:
			case CCharacterCommand::CC_WaitForNotCharacter:
				ASSERT(!"Deprecated script command");
			break;

			default: ASSERT(!"Bad CCharacter command"); break;
		}

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
		static const UINT VARSET_LIMIT = 10000;
		if (++wTurnCount > this->commands.size() || wVarSets > VARSET_LIMIT)
		{
			this->wCurrentCommandIndex = this->commands.size();

			WSTRING wstr = pGame->AbbrevRoomLocation();
			string str = UnicodeToUTF8(wstr);
			str += ": Character script is in an infinite loop" NEWLINE;

			CFiles f;
			f.AppendUserLog(str.c_str());
		}
	} while (bProcessNextCommand);

	} //Wrap variables initialized within jump, to make g++ happy

Finish:
	if (this->bVisible && bIsBeethroDouble(GetResolvedIdentity()) && IsAlive())
	{
		//Light any fuse stood on.
		room.LightFuseEnd(CueEvents, this->wX, this->wY);
	}

	this->bPlayerTouchedMe = false; //these flags are reset at the end of each turn
	this->bWasPushed = false;

#undef STOP_COMMAND
#undef STOP_DONECOMMAND
}

//*****************************************************************************
bool CCharacter::GetMovement(
	const UINT wDestX, const UINT wDestY,
	int& dx, int& dy, int& dxFirst, int& dyFirst, bool& bPathmapping, //(out)
	const bool bAllowTurning)
	// Returns: true if the move should not be done, even if dx & dy are non-zero,
	// used by pathmapping movement rules
{
	bPathmapping = false;

	bool bStopTurn = false;
	switch (this->movementIQ)
	{
		case DirectOnly:
			GetBeelineMovementDumb(wDestX, wDestY, dxFirst, dyFirst, dx, dy);
		break;
		case SmartDiagonalOnly:
			GetBeelineMovement(wDestX, wDestY, dxFirst, dyFirst, dx, dy);
		break;
		case SmarterDiagonalOnly:
			GetBeelineMovementSmart(wDestX, wDestY, dxFirst, dyFirst,
				dx, dy, false);
		break;
		default:
		case SmartOmniDirection:
			GetBeelineMovementSmart(wDestX, wDestY, dxFirst, dyFirst,
				dx, dy, true);
		break;
		case MIQ_Pathfind:
		case MIQ_PathfindOpenOnly:
		{
			//If path is already calculated, continue following same path.
			CCurrentGame *pGame = const_cast<CCurrentGame*>(this->pCurrentGame);
			CDbRoom& room = *(pGame->pRoom);

			const bool bBeelineWhenBlocked = this->movementIQ == MIQ_Pathfind;
			const bool bGoalIsCurrent = this->goal.wX == wDestX && this->goal.wY == wDestY;
			this->goal.wX = wDestX;
			this->goal.wY = wDestY;
			room.GetSwordCoords(this->swordsInRoom, true, false, this); //optimization
			if (bGoalIsCurrent && ConfirmPathWithNextMoveOpen()) {
				bPathmapping = true;
			} else {
				//Calculate new path.
				const bool bRes = FindOptimalPath2(this->wX, this->wY, wDestX, wDestY, bBeelineWhenBlocked);
				bPathmapping = bRes && !this->pathToDest.IsEmpty();
			}

			if (bPathmapping) {
				UINT wNextX, wNextY;
				this->pathToDest.Top(wNextX, wNextY);
				dx = dxFirst = wNextX - this->wX;
				dy = dyFirst = wNextY - this->wY;
				if (!this->IsOpenMove(dx, dy))
				{
					if (bAllowTurning && !TurnsSlowly()) {
						SetOrientation(dxFirst,dyFirst);
					}
					bStopTurn = true;
				}
			} else if (bBeelineWhenBlocked) {
				GetBeelineMovementSmart(wDestX, wDestY, dxFirst, dyFirst,
						dx, dy, true);
			} else {
				bStopTurn = true;
			}
		}
		break;
	}

	return bStopTurn;
}

bool CCharacter::ConfirmPathWithNextMoveOpen()
{
	//Previously mapped path may go through specially marked NPCs...
	CMonster::calculatingPathmap = true;
	const bool bRes = ConfirmPath();
	CMonster::calculatingPathmap = false;

	//...as long as the step to take now is open.
	if (bRes) {
		UINT wNextX, wNextY;
		this->pathToDest.Top(wNextX, wNextY);
		if (!this->IsOpenMove(wNextX - this->wX, wNextY - this->wY))
			return false;
	}

	return bRes;
}

//*****************************************************************************
void CCharacter::BuildMarker(const CCharacterCommand& command)
//Mark the specified game element (flags) to be built in rect (x,y,w,h).
{
	UINT px, py, pw, ph, pflags;  //command parameters
	getCommandParams(command, px, py, pw, ph, pflags);
	
	if (!BuildUtil::bIsValidBuildTile(pflags))
		return;

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	UINT endX = px + pw;
	UINT endY = py + ph;

	if (!room.CropRegion(px, py, endX, endY))
		return;

	for (UINT y = py; y <= endY; ++y) {
		for (UINT x = px; x <= endX; ++x)
		{
			//Mark for building if there is no critical obstruction.
			if (BuildUtil::CanBuildAt(room, pflags, x, y, false))
				room.building.plot(x,y,pflags);
		}
	}
}

void CCharacter::BuildTiles(const CCharacterCommand& command, CCueEvents& CueEvents)
//Build the specified game element (flags) in rect (x,y,w,h).
{
	UINT px, py, pw, ph, pflags;  //command parameters
	getCommandParams(command, px, py, pw, ph, pflags);

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	BuildUtil::BuildTilesAt(room, pflags, px, py, pw, ph, false, CueEvents);
}

//*****************************************************************************
bool CCharacter::CanPushObjects() const {
	return HasBehavior(ScriptFlag::PushObjects);
}

//*****************************************************************************
bool CCharacter::CanPushMonsters() const
{
	return HasBehavior(ScriptFlag::PushMonsters);
}

//*****************************************************************************
bool CCharacter::CanPushOntoOTileAt(UINT wX, UINT wY) const
{
	UINT wTileNo = this->pCurrentGame->pRoom->GetOSquare(wX, wY);

	if (bIsFloor(wTileNo) || bIsOpenDoor(wTileNo) || bIsPlatform(wTileNo))
		return true;

	// If the character is immune to "fatal pushes", only allow it to be pushed
	// to tiles that won't cause it to fall
	bool bOnlySafe = HasBehavior(ScriptFlag::FatalPushImmune);

	if (!bOnlySafe) {
		// We don't care that these tiles might be deadly
		return bIsPit(wTileNo) || bIsWater(wTileNo);
	}

	if (bIsPit(wTileNo) && (IsFlying()))
		return true;

	if (bIsDeepWater(wTileNo) && (IsSwimming() || IsFlying()))
		return true;

	if (bIsShallowWater(wTileNo) &&
		(CanWadeInShallowWater() || IsSwimming() || IsFlying()))
		return true;

	return false;
}

//*****************************************************************************
bool CCharacter::CanBeNPCBeethro() const
// Returns if this character can be chosen as the "NPC Beethro"
// When the player is not a valid monster target, monsters may select the first
// character in the monster list with a Beethro or Gunthro appearance to be a target.
// To be selected, the character must be visible, have a smitemaster identity,
// and have the CanBeNPCBeethro behavior flag. By default, all smitemaster appearances
// have this flag, but the user may remove it.
{
	return IsVisible() && bIsSmitemaster(this->wIdentity) && HasBehavior(ScriptFlag::CanBeNPCBeethro);
}

//*****************************************************************************
bool CCharacter::CanDropTrapdoor(const UINT oTile) const
{
	if (!bIsFallingTile(oTile))
		return false;

	if (HasBehavior(ScriptFlag::DropTrapdoors))
		return true;
	
	if (HasBehavior(ScriptFlag::DropTrapdoorsArmed)) {
		if (bIsThinIce(oTile))
			return true;

		if (HasSword() && bIsHeavyWeapon(GetWeaponType()))
			return true;
	}

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
	if (!this->bVisible)
		return false;  //NPC not in room
	//invincibility checked in OnStabbed

	return CMonster::CheckForDamage(CueEvents);
}

//*****************************************************************************
void CCharacter::CriticalNPCDied(CCueEvents& CueEvents)
{
	const UINT identity = GetIdentity();
	if (identity == M_HALPH || identity == M_HALPH2)
		CueEvents.Add(CID_HalphDied, this);
	else
		CueEvents.Add(CID_CriticalNPCDied);

	const_cast<CCurrentGame*>(this->pCurrentGame)->SetDyingEntity(this);
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
				if (!wSkipToIfEnd && wNestingDepth == 0){
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
//  If the NPC is set to M_CLONE, then it will match the current player role.
//  Call this method instead of GetIdentity to fully resolve what an NPC should look like.
{
	UINT wIdentity = GetIdentity();
	if (wIdentity == M_CLONE || wIdentity == M_TEMPORALCLONE)
	{
		//Resolve player appearance.
		CClone clone(const_cast<CCurrentGame*>(this->pCurrentGame));
		return clone.GetIdentity();
	}

	return wIdentity;
}

//*****************************************************************************
bool CCharacter::IsAttackableTarget() const
//Returns: whether this character is of a type and state that monsters can
//attack and kill.
{
	if (!this->bVisible)
		return false;

	if (IsInvulnerable())
		return false;

	return HasBehavior(ScriptFlag::MonsterAttackable);
}

//*****************************************************************************
bool CCharacter::IsBrainPathmapObstacle() const
{
	return this->bBrainPathmapObstacle;
}

//*****************************************************************************
bool CCharacter::IsFriendly() const
//Returns: whether character is friendly to the player
{
	return this->bFriendly;
}

//*****************************************************************************
bool CCharacter::IsMonsterTarget() const
//Returns: whether the character is a valid target for monsters
{
	if (HasBehavior(ScriptFlag::MonsterTarget)) {
		return true;
	}

	const UINT identity = GetIdentity();
	//Only a target if the player is
	if (HasBehavior(ScriptFlag::MonsterTargetWhenPlayerIsTarget))
	{
		if (!this->pCurrentGame)
			return true;
		const CSwordsman& player = this->pCurrentGame->swordsman;
		if (player.wAppearance == M_NONE)
			return true;
		return player.IsTarget();
	}

	return false;
}

//*****************************************************************************
bool CCharacter::IsNPCPathmapObstacle() const
{
	return this->bNPCPathmapObstacle;
}

//*****************************************************************************
bool CCharacter::IsPushableByBody() const
//Returns: whether character is pushable by body
{
	return this->bPushableByBody;
}

//*****************************************************************************
bool CCharacter::IsPushableByWeaponAttack() const
//Returns: whether character is pushable by body
{
	return this->bPushableByWeapon;
}

//*****************************************************************************
bool CCharacter::IsTileAt(const CCharacterCommand& command) const
//Returns: whether the specified game element (flags) is in rect (x,y,w,h).
{
	UINT px, py, pw, ph, pflags;  //command parameters
	getCommandParams(command, px, py, pw, ph, pflags);

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (px >= room.wRoomCols)
		return false;
	if (py >= room.wRoomRows)
		return false;

	const bool bFakeElement = bIsFakeElementType(pflags);
	const bool bLightingElement = bIsLighting(pflags);
	const UINT tile = bFakeElement ? bConvertFakeElement(pflags) : pflags;

	const bool bRealTile = IsValidTileNo(tile);
	if (bRealTile)
	{
		UINT endX = px + pw;
		UINT endY = py + ph;
		if (endX >= room.wRoomCols)
			endX = room.wRoomCols-1;
		if (endY >= room.wRoomRows)
			endY = room.wRoomRows-1;

		for (UINT y=py; y <= endY; ++y)
		{
			for (UINT x=px; x <= endX; ++x)
			{
				if (!room.IsValidColRow(x,y))
					continue;
				if (bLightingElement){
					const UINT lightingValue = this->pCurrentGame->pRoom->tileLights.GetAt(x, y);

					if (pflags == T_WALLLIGHT){
						if (bIsWallLightValue(lightingValue)) return true;
					} else if (pflags == T_LIGHT_CEILING){
						if (bIsLightTileValue(lightingValue)) return true;
					} else if (pflags == T_DARK_CEILING){
						if (bIsDarkTileValue(lightingValue)) return true;
					}
					
					continue;
				}

				switch (TILE_LAYER[tile])
				{
					case 0:  //o-layer
						if (bFakeElement){
							if (tile == T_PRESSPLATE)
							{
								const COrbData* pOrbData = this->pCurrentGame->pRoom->GetPressurePlateAtCoords(x, y);

								if (pOrbData != NULL && bPlateTypeMatchesFakeTile(pflags, pOrbData->eType)) return true;
							}
						}
						else {
							if (room.GetOSquare(x, y) == tile)
								return true;
							if (tile == T_OVERHEAD_IMAGE && room.overheadTiles.Exists(x, y))
								return true;
						}
					break;
					case 1:  //t-layer
						if (room.GetTSquare(x,y) == tile)
						{
							if (bFakeElement)
							{
								switch(tile)
								{
									case T_TOKEN: {
										const BYTE tParam = room.GetTParam(x,y);
										RoomTokenType tType = (RoomTokenType)calcTokenType(tParam);

										if (pflags == T_ACTIVETOKEN)
										{
											if (bTokenActive(tParam))
												return true;
										} else {
											//Special handling for tokens that have active equivalents of other base types
											if (bTokenActive(tParam)) {
												switch (tType) {
													case RotateArrowsCW: tType = RotateArrowsCCW; break;
													case RotateArrowsCCW: tType = RotateArrowsCW; break;
													default: break;
												}
											}
											if (tType == ConvertFakeTokenType(pflags))
												return true;
										}
									}
									break;
									case T_ORB: {
										COrbData* pOrbData = room.GetOrbAtCoords(x, y);
										if (pOrbData){
											return  ((pOrbData->eType == OT_NORMAL && pflags == T_ORB_NORMAL)
												|| (pOrbData->eType == OT_ONEUSE && pflags == T_ORB_CRACKED)
												|| (pOrbData->eType == OT_BROKEN && pflags == T_ORB_BROKEN));
										} else {
											return pflags == T_ORB_NORMAL; // Regular orbs with no agents have no entry in CDbRoom::orbs collection
										}
									}
									break;
								}
								
							} else
								return true;
						}
					break;
					case 3:  //f-layer
						if (room.GetFSquare(x,y) == tile)
							return true;
					break;
					default: break;
				}
			}
		}
	}

	return false;
}

//*****************************************************************************
bool CCharacter::DoesVarSatisfy(const CCharacterCommand& command, CCurrentGame *pGame)
{
	const UINT varIndex = command.x;

	//Get variable.
	CDbPackedVars& stats = pGame->stats;
	char holdVarName[11] = "v";
	UNPACKEDVARTYPE vType = UVT_int;

	const bool bPredefinedVar = varIndex >= UINT(ScriptVars::FirstPredefinedVar);
	bool bValidInt = true;
	WSTRING localVarName;
	bool bLocalVar = false;
	if (!bPredefinedVar)
	{
		if (pGame->pHold) {
			localVarName = pGame->pHold->GetLocalScriptVarNameForID(varIndex);
			bLocalVar = !localVarName.empty();
		}
		if (!bLocalVar)
		{
			//Get local hold var.
			char varID[10];
			_itoa(varIndex, varID, 10);
			strcat(holdVarName, varID);

			//Enforce basic type checking.
			vType = stats.GetVarType(holdVarName);
			bValidInt = vType == UVT_int || vType == UVT_uint || vType == UVT_unknown;
		}
	}

	int operand = int(command.w); //expect an integer value by default
	if (!operand && !command.label.empty() && command.y != ScriptVars::EqualsText)
	{
		//Operand is not just an integer, but a text expression.
		UINT index=0;
		operand = parseExpression(command.label.c_str(), index, pGame, this);
	}

	int x=0;
	const bool bNumber = bValidInt && command.y != ScriptVars::EqualsText;
	if (bNumber) {
		if (bPredefinedVar) {
			x = int(getPredefinedVarInt(varIndex));
		} else if (bLocalVar) {
			x = getLocalVarInt(localVarName);
		} else {
			x = stats.GetVar(holdVarName, (int)0);
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
			if (bPredefinedVar) {
				if (ScriptVars::IsStringVar(ScriptVars::Predefined(varIndex)))
					wStr = getPredefinedVarString(varIndex);
			} else if (bLocalVar) {
				wStr = getLocalVarString(localVarName);
			} else {
				if (vType == UVT_wchar_string)
					wStr = stats.GetVar(holdVarName, wszEmpty);
			}
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
void CCharacter::SetVariable(const CCharacterCommand& command, CCurrentGame* pGame, CCueEvents& CueEvents)
{
	const UINT varIndex = command.x;

	//Get variable.
	CDbPackedVars& stats = pGame->stats;
	char varName[11] = "v";

	const bool bPredefinedVar = varIndex >= UINT(ScriptVars::FirstPredefinedVar);
	int predefinedVarVal = 0;
	WSTRING predefinedVarValStr;
	bool bValidInt = true;
	WSTRING localVarName;
	bool bLocalVar = false;

	if (bPredefinedVar) {
		if (ScriptVars::IsStringVar(ScriptVars::Predefined(varIndex))) {
			predefinedVarValStr = getPredefinedVarString(varIndex);
		} else {
			predefinedVarVal = int(getPredefinedVarInt(varIndex));
		}
	} else {
		if (pGame->pHold) {
			localVarName = pGame->pHold->GetLocalScriptVarNameForID(varIndex);
			bLocalVar = !localVarName.empty();
		}
		if (!bLocalVar)
		{
			//Get local hold var.
			char varID[10];
			_itoa(varIndex, varID, 10);
			strcat(varName, varID);

			//Enforce basic type checking.
			const UNPACKEDVARTYPE vType = stats.GetVarType(varName);
			bValidInt = vType == UVT_int || vType == UVT_uint || vType == UVT_unknown;
		}
	}

	const bool bSetNumber = !(command.y == ScriptVars::AssignText ||
			command.y == ScriptVars::AppendText);

	int operand = int(command.w); //expect an integer by default
	if (!operand && !command.label.empty() && bSetNumber)
	{
		//Operand is not just an integer, but a text expression.
		UINT index=0;
		operand = parseExpression(command.label.c_str(), index, pGame, this);
	}

	int x=0;

	switch (command.y)
	{
		case ScriptVars::Assign:
			x = operand;
		break;
		case ScriptVars::Inc:
			if (bValidInt)
				x = bPredefinedVar ? predefinedVarVal : bLocalVar ? getLocalVarInt(localVarName) : stats.GetVar(varName, (int)0);
			addWithClamp(x, operand);
		break;
		case ScriptVars::Dec:
			if (bValidInt)
				x = bPredefinedVar ? predefinedVarVal : bLocalVar ? getLocalVarInt(localVarName) : stats.GetVar(varName, (int)0);
			addWithClamp(x, -operand);
		break;
		case ScriptVars::MultiplyBy:
			if (bValidInt)
				x = bPredefinedVar ? predefinedVarVal : bLocalVar ? getLocalVarInt(localVarName) : stats.GetVar(varName, (int)0);
			multWithClamp(x, operand);
		break;
		case ScriptVars::DivideBy:
			if (bValidInt)
				x = bPredefinedVar ? predefinedVarVal : bLocalVar ? getLocalVarInt(localVarName) : stats.GetVar(varName, (int)0);
			if (operand)
				x /= operand;
		break;
		case ScriptVars::Mod:
			if (bValidInt)
				x = bPredefinedVar ? predefinedVarVal : bLocalVar ? getLocalVarInt(localVarName) : stats.GetVar(varName, (int)0);
			if (operand)
				x = x % operand;
		break;

		case ScriptVars::AssignText:
		{
			const WSTRING text = pGame->ExpandText(command.label.c_str(), this);
			if (bPredefinedVar)
				setPredefinedVarString(varIndex, text, CueEvents);
			else if (bLocalVar)
				SetLocalVar(localVarName, text);
			else
				stats.SetVar(varName, text.c_str());
		}
		break;
		case ScriptVars::AppendText:
		{
			WSTRING text = bLocalVar ? getLocalVarString(localVarName) : stats.GetVar(varName, wszEmpty);
			text += pGame->ExpandText(command.label.c_str(), this);
			if (bPredefinedVar)
				setPredefinedVarString(varIndex, text, CueEvents);
			else if (bLocalVar)
				SetLocalVar(localVarName, text);
			else
				stats.SetVar(varName, text.c_str());
		}
		break;
		default: break;
	}
	if (bSetNumber)
	{
		if (bPredefinedVar) {
			setPredefinedVarInt(varIndex, x, CueEvents);
		} else if (bLocalVar) {
			WCHAR wIntText[20];
			_itoW(int(x), wIntText, 10);
			SetLocalVar(localVarName, wIntText);
		} else {
			stats.SetVar(varName, x);
		}
	}
}

//*****************************************************************************
void CCharacter::SetLocalVar(const WSTRING& varName, const WSTRING& val)
{
	this->localScriptVars[varName] = val;
}

//*****************************************************************************
bool CCharacter::HasSword() const
//Returns: true when double has unsheathed sword
{
	if (!this->bWeaponOverride && !bEntityHasSword(GetResolvedIdentity()))
		return false;

	return CArmedMonster::HasSword();
}


//*****************************************************************************
bool CCharacter::TurnsSlowly() const
{
	return HasSword() && GetWeaponType() != WT_Dagger;
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
			if (nCommand != CMD_YES)
			{
				FailedIfCondition(); //skip if block
				// If we are entering else if, make sure we set proper variables for it to be handled correclty
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
					this->bIfBlock = false;
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
UINT getDataID(const CCharacterCommand& command)
{
	if (!bCommandHasData(command.command))
		return 0;

	if (command.IsMusicCommand())
		return command.y; //SetMusic uses y param to store the music dataID

	if (command.command == CCharacterCommand::CC_WorldMapImage)
		return command.h;

	return command.w;
}

//*****************************************************************************
void setDataID(CCharacterCommand& command, const UINT dwDataID)
{
	if (!bCommandHasData(command.command))
		return;

	if (command.IsMusicCommand()) {
		command.y = dwDataID;
	} else if (command.command == CCharacterCommand::CC_WorldMapImage) {
		command.h = dwDataID;
	} else {
		command.w = dwDataID;
	}
}

//*****************************************************************************
string CCharacter::ExportXMLSpeech(
//Returns: string containing XML text describing character with this ID
//
//Params:
	CDbRefs &dbRefs,        //(in/out)
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
		const UINT dataID = getDataID(command);
		if (dataID && g_pTheDB->Data.Exists(dataID))
			g_pTheDB->Data.ExportXML(dataID, dbRefs, str, bRef);
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
	CImportInfo &info,     //(in/out) Import data
	const bool bHoldChar)  //(in) Whether character is a Hold Character Default Script
{
	UINT dwSpeechID, eCommand;
	const UINT wNumCommands = this->ExtraVars.GetVar(numCommandsStr, 0);
	PrimaryKeyMap::iterator localID;

	BYTE *commandBuffer = (BYTE*)this->ExtraVars.GetVar(commandStr, (const void*)(NULL));
	BYTE *oldCommandBuffer = (BYTE*)this->ExtraVars.GetVar(commandStr_3_0_2_2, (const void*)(NULL));
	if (commandBuffer || oldCommandBuffer)
	{
		// Set member vars at the same time
		SetBaseMembersFromExtraVars();

		this->dwScriptID = this->ExtraVars.GetVar(scriptIDstr, 0);

		this->commands.reserve(wNumCommands);

		UINT index = 0;

		if (commandBuffer)
		{
			//Current script data format.
			const UINT bufferSize = this->ExtraVars.GetVarValueSize(commandStr);

			while (index < bufferSize)
			{
				CCharacterCommand command;

				eCommand = readBpUINT(commandBuffer, index);
				command.command = CCharacterCommand::CharCommand(eCommand);
				command.x = readBpUINT(commandBuffer, index);
				command.y = readBpUINT(commandBuffer, index);
				command.w = readBpUINT(commandBuffer, index);
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
					UINT dwDataID = getDataID(command);
					if (dwDataID)
					{
						//If data is not present, simply reset this field.
						localID = info.DataIDMap.find(dwDataID);
						dwDataID = localID != info.DataIDMap.end() ? localID->second : 0;

						setDataID(command, dwDataID);
					}
				}

				if (dwSpeechID)
				{
					localID = info.SpeechIDMap.find(dwSpeechID);
					if (localID == info.SpeechIDMap.end())
						return MID_FileCorrupted;  //record should have been loaded already
					dwSpeechID = localID->second;
					command.pSpeech = g_pTheDB->Speech.GetByID(dwSpeechID);
					ASSERT(command.pSpeech);
				}

				Upgrade2_0CommandTo3_0(command, this->commands);
			}
			ASSERT(index == bufferSize);
		} else if (oldCommandBuffer) {
			//3.0.2 rev2 script data format.
			const UINT bufferSize = this->ExtraVars.GetVarValueSize(commandStr_3_0_2_2);

			while (index < bufferSize)
			{
				CCharacterCommand command;

				eCommand = oldCommandBuffer[index++];
				command.command = CCharacterCommand::CharCommand(eCommand);
				command.x = readUINT(oldCommandBuffer, index);
				command.y = readUINT(oldCommandBuffer, index);
				command.w = readUINT(oldCommandBuffer, index); // set this later
				command.h = readUINT(oldCommandBuffer, index);
				command.flags = readUINT(oldCommandBuffer, index);
				dwSpeechID = readUINT(oldCommandBuffer, index); // set this later

				const UINT labelSize = readUINT(oldCommandBuffer, index);
				string str;
				str.resize(labelSize);
				for (UINT i=0; i<labelSize; ++i)
					str[i] = oldCommandBuffer[index++];
				Base64::decode(str, command.label);

				if (bCommandHasData(eCommand))
				{
					UINT dwDataID = command.w;
					if (dwDataID)
					{
						//If data is not present, simply reset this field.
						localID = info.DataIDMap.find(dwDataID);
						dwDataID = localID != info.DataIDMap.end() ? localID->second : 0;
						command.w = dwDataID;
					}
				}

				if (dwSpeechID)
				{
					localID = info.SpeechIDMap.find(dwSpeechID);
					if (localID == info.SpeechIDMap.end())
						return MID_FileCorrupted;  //record should have been loaded already
					dwSpeechID = localID->second;
					command.pSpeech = g_pTheDB->Speech.GetByID(dwSpeechID);
					ASSERT(command.pSpeech);
				}

				Upgrade2_0CommandTo3_0(command, this->commands);
			}
			ASSERT(index == bufferSize);
		}
		ASSERT(this->commands.size() == wNumCommands);
	} else {
		//Pre-3.0.2 rev2 script data format.
		char varName[20], num[10];
		for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
		{
			_itoa(wIndex, num, 10);
			strcpy(varName, num);
			strcat(varName, "c");
			eCommand = this->ExtraVars.GetVar(varName, (UINT)CCharacterCommand::CC_Count);
			if (bCommandHasData(eCommand))
			{
				strcpy(varName, num);
				strcat(varName, "w");
				UINT dwDataID = this->ExtraVars.GetVar(varName, 0);
				if (dwDataID)
				{
					//If data is not present, simply reset this field.
					localID = info.DataIDMap.find(dwDataID);
					dwDataID = localID != info.DataIDMap.end() ? localID->second : 0;
					this->ExtraVars.SetVar(varName, dwDataID);
				}
			}

			strcpy(varName, num);
			strcat(varName, "s");
			dwSpeechID = this->ExtraVars.GetVar(varName, 0);
			if (dwSpeechID)
			{
				localID = info.SpeechIDMap.find(dwSpeechID);
				if (localID == info.SpeechIDMap.end())
					return MID_FileCorrupted;  //record should have been loaded already
				dwSpeechID = localID->second;
				this->ExtraVars.SetVar(varName, dwSpeechID);
			}
		}

		//Upgrade deprecated 2.0 functionality to 3.0 replacements.
		SetMembersFromExtraVars();
	}

	SetExtraVarsFromMembers(bHoldChar);

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
	//Routine is not written to check the square on which this monster is
	//standing.
	ASSERT(wCol != this->wX || wRow != this->wY);

	CDbRoom& room = *(this->pCurrentGame->pRoom);
	if (!room.IsValidColRow(wCol,wRow)) return true;

	//Check o-square for obstacle.
	UINT wLookTileNo = room.GetOSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		switch (wLookTileNo)
		{
			//If standing on a platform, check whether it can move.
			case T_PIT: case T_PIT_IMAGE:
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_P 
						&& HasBehavior(ScriptFlag::MovePlatforms))
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					if (room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			case T_WATER: /*case T_SHALLOW_WATER:*/
				if (room.GetOSquare(this->wX, this->wY) == T_PLATFORM_W
						&& HasBehavior(ScriptFlag::MovePlatforms))
				{
					const int nFirstO = nGetO((int)wCol - (int)this->wX, (int)wRow - (int)this->wY);
					// @FIXME - nDist is a temporary fix to prevent hard crashes 
					if (nDist(wCol, wRow, this->wX, this->wY) == 1 && room.CanMovePlatform(this->wX, this->wY, nFirstO))
						break;
				}
			return true;
			default:	return true;
		}
	}

	//Check t-square for obstacle.
	wLookTileNo = room.GetTSquare(wCol, wRow);
	if (IsTileObstacle(wLookTileNo))
	{
		//There is something at the destination that is normally an obstacle,
		//but some of them are handled specially.  Check for special handling first.
		if (bIsTLayerCoveringItem(wLookTileNo) && CanPushObjects())
		{
			//item is not an obstacle when it can be pushed away
			const int dx = (int)wCol - (int)this->wX;
			const int dy = (int)wRow - (int)this->wY;
			if (!room.CanPushTo(wCol, wRow, wCol + dx, wRow + dy))
				return true;
		} else {
			return true;
		}
	}

	if (IsTileObstacle(room.GetFSquare(wCol, wRow)))
		return true;

	//Check for monster at square.
	CMonster *pMonster = room.GetMonsterAtSquare(wCol, wRow);
	if (pMonster && pMonster->wType != M_FLUFFBABY) {
		if (!CMonster::calculatingPathmap || pMonster->IsNPCPathmapObstacle()){
			const int dx = (int)wCol - (int)this->wX;
			const int dy = (int)wRow - (int)this->wY;

			if (pMonster->wType != M_FLUFFBABY && 
				(!pMonster->IsAttackableTarget() || !CanDaggerStep(pMonster)) &&
				(!this->CanPushMonsters() || !pMonster->IsPushableByBody() || !room.CanPushMonster(pMonster, wCol, wRow, wCol + dx, wRow + dy))){
				return true;
			}
		}
	}

	//Can't move onto player if set to "safe".
	if (this->bSafeToPlayer && this->pCurrentGame->IsPlayerAt(wCol, wRow))
		return true;

	//Can't step on any swords.
	if (!this->swordsInRoom.empty()) {
		if (this->swordsInRoom.Exists(wCol, wRow)) //this set is compiled at beginning of move
			return true;
	} else {
		//Check for player's sword at square.
		if (this->pCurrentGame->IsPlayerWeaponAt(wCol, wRow, true))
			return true;

		//Check for monster sword at square.
		if (room.IsMonsterSwordAt(wCol, wRow, true, this))
			return true;
	}

	//No obstacle.
	return false;
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
bool CCharacter::IsOpenMove(const int dx, const int dy) const
//Returns: whether move is possible, and player is not in the way
{
	return CMonster::IsOpenMove(dx,dy) &&
		(!this->bSafeToPlayer || !this->pCurrentGame->IsPlayerAt(this->wX+dx, this->wY+dy));
}

//*****************************************************************************
bool CCharacter::IsPlayerAllyTarget() const
// Returns: whether Stalwart/Soldiers should treat this character as a target
{
	if (!IsVisible())
		return false;

	return HasBehavior(ScriptFlag::AllyTarget);
}

//*****************************************************************************
bool CCharacter::IsPuffTarget() const
// Returns: whether Puff monsters should treat this character as a target
{
	return HasBehavior(ScriptFlag::PuffTarget);
}

//*****************************************************************************

void CCharacter::GenerateEntity(const UINT identity, const UINT wX, const UINT wY, const UINT wO, CCueEvents& CueEvents)
{
	CCurrentGame* pGame = (CCurrentGame*)this->pCurrentGame; //non-const
	UINT wBaseIdentity = bEntityBaseIdentity(identity);

	CMonster* pMonster = pGame->AddNewEntity(CueEvents, wBaseIdentity, wX, wY, wO);

	if (pMonster)
		switch (identity) {
			case M_EYE_ACTIVE:
				CEvilEye* pEvilEye = DYN_CAST(CEvilEye*, CMonster*, pMonster);
				pEvilEye->SetActive(true);
				break;
		}
	
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
	// With the Restricted Movement behavior, characters cannot perform normal
	// movement if their movement types prevent it.
	if (HasBehavior(ScriptFlag::RestrictedMovement)) {
		return CMonster::IsTileObstacle(wTileNo);
	}

	// All the things a character can step onto
	bool bIsObstacle = !(
		wTileNo == T_EMPTY ||
		bIsFloor(wTileNo) ||
		bIsOpenDoor(wTileNo) ||
		bIsAnyArrow(wTileNo) ||
		bIsPlatform(wTileNo) ||
		wTileNo == T_NODIAGONAL ||
		wTileNo == T_SCROLL ||
		wTileNo == T_FUSE ||
		wTileNo == T_TOKEN ||
		wTileNo == T_SHALLOW_WATER ||
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

		//These types can also move over pits.
		case MovementType::AIR:
			return bIsObstacle && !bIsWater(wTileNo) && !bIsPit(wTileNo);

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
void CCharacter::LoadCommands(CDbPackedVars& ExtraVars, COMMAND_VECTOR& commands)
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
		//Packed var name used for command serialization in version 3.0.2 rev2.
		BYTE *oldCommandBuffer = (BYTE*)ExtraVars.GetVar(commandStr_3_0_2_2, (const void*)(NULL));
		if (oldCommandBuffer)
		{
			const UINT bufferSize = ExtraVars.GetVarValueSize(commandStr_3_0_2_2);
			DeserializeCommands_3_0_2_2(oldCommandBuffer, bufferSize, commands);
			ASSERT(commands.size() == wNumCommands);
		} else {
			//Pre-3.0.2 rev2 script data encapsulation format.

			//Construct each command.
			char num[10];
			char varName[20];
			UINT eCommand;
			UINT dwSpeechID;
			for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
			{
				CCharacterCommand command;

				//Vars loaded here must match the naming convention used in CCharacter::Save().
				_itoa(wIndex, num, 10);
				strcpy(varName, num);
				strcat(varName, "c");
				eCommand = ExtraVars.GetVar(varName, (UINT)CCharacterCommand::CC_Count);
				ASSERT(eCommand < CCharacterCommand::CC_Count);
				command.command = (CCharacterCommand::CharCommand)eCommand;

				//Query in alphabetical order for speed.
				strcpy(varName, num);
				strcat(varName, "f");
				command.flags = ExtraVars.GetVar(varName, 0);
				strcpy(varName, num);
				strcat(varName, "h");
				command.h = ExtraVars.GetVar(varName, 0);
				strcpy(varName, num);
				strcat(varName, "l");
				command.label = ExtraVars.GetVar(varName, wszEmpty);
				strcpy(varName, num);
				strcat(varName, "s");
				dwSpeechID = ExtraVars.GetVar(varName, 0);
				strcpy(varName, num);
				strcat(varName, "w");
				command.w = ExtraVars.GetVar(varName, 0);
				strcpy(varName, num);
				strcat(varName, "x");
				command.x = ExtraVars.GetVar(varName, 0);
				strcpy(varName, num);
				strcat(varName, "y");
				command.y = ExtraVars.GetVar(varName, 0);
				if (dwSpeechID)
				{
					command.pSpeech = g_pTheDB->Speech.GetByID(dwSpeechID);
					ASSERT(command.pSpeech);
				}

				Upgrade2_0CommandTo3_0(command, commands);
			}
		}
	}
}

//*****************************************************************************
void CCharacter::LoadCommands(CDbPackedVars& ExtraVars, COMMANDPTR_VECTOR& commands)
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
		ASSERT(!"Shouldn't be encountered in command pointers");
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
			if (this->pCustomChar){
				this->wIdentity = this->pCustomChar->wType;

				if (this->commands.empty()){
					this->wProcessSequence = this->pCustomChar->ExtraVars.GetVar(ParamProcessSequenceStr, this->wProcessSequence);	
				}
			} else
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
void CCharacter::SetCurrentGame(
//Sets current game pointer for monster.
//This is necessary for many methods of the monster class to work.
//
//Params:
	const CCurrentGame *pSetCurrentGame) //(in)
{
	CMonster::SetCurrentGame(pSetCurrentGame);

	//Check for a custom character.
	ResolveLogicalIdentity(this->pCurrentGame ? this->pCurrentGame->pHold : NULL);

	//Certain character types have special default traits.
	//Assign these when room play is first starting.
	if (pSetCurrentGame->wTurnNo == 0)
	{
		switch (GetIdentity())
		{
			case M_CITIZEN: case M_ARCHITECT:
			case M_WUBBA:
				SetImperative(ScriptFlag::Invulnerable);
			break;
			case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: case M_GUNTHRO:
			case M_CLONE:
			case M_TEMPORALCLONE:
			case M_HALPH: case M_HALPH2:
				SetImperative(ScriptFlag::MissionCritical);
				bFriendly = true;
			break;
			default: break;
		}

		SetDefaultBehaviors();
		SetDefaultMovementType();
	}

	//If this NPC is a custom character with no script,
	//then use the default script for this custom character type.
	if (this->pCustomChar && this->commands.empty())
		LoadCommands(this->pCustomChar->ExtraVars, this->commands);

	//Global scripts started without commands should be flagged as done
	//and removed on room exit
	if (this->bGlobal && this->commands.empty())
		this->bScriptDone = true;
}

//*****************************************************************************
// Certain character types have default behaviors.
void CCharacter::SetDefaultBehaviors()
{
	const UINT wResolvedIdentity = GetResolvedIdentity();
	behaviorFlags.clear();

	if (wResolvedIdentity == M_CONSTRUCT) {
		// Character construct can drop trapdoors and push pushable monsters by default
		// But it can't push objects by default
		behaviorFlags.insert(ScriptFlag::DropTrapdoors);
		behaviorFlags.insert(ScriptFlag::PushMonsters);
	}

	if (bIsMonsterTarstuff(wResolvedIdentity))
	{
		// Tarstuff monsters are immune to hot tiles by default
		behaviorFlags.insert(ScriptFlag::HotTileImmune);
	}

	if (bIsStalwart(wResolvedIdentity)) {
		bFriendly = true;
	}

	if (bIsBeethroDouble(wResolvedIdentity)) {
		behaviorFlags.insert(ScriptFlag::DropTrapdoorsArmed);
	}

	if (bIsHuman(wResolvedIdentity)) {
		behaviorFlags.insert(ScriptFlag::ActivateTokens);
		behaviorFlags.insert(ScriptFlag::PushObjects);
		behaviorFlags.insert(ScriptFlag::PushMonsters);
		behaviorFlags.insert(ScriptFlag::MovePlatforms);
	}

	if (!this->IsFlying() && this->GetIdentity() != M_SEEP) {
		behaviorFlags.insert(ScriptFlag::ActivatePlates);
	}

	if (bIsSmitemaster(wResolvedIdentity) || bIsStalwart(wResolvedIdentity)) {
		//These types can be attacked and killed by default.
		behaviorFlags.insert(ScriptFlag::MonsterAttackable);
	}

	if (bIsSmitemaster(wIdentity)) {
		// By default, all Beethro and Gunthro NPCs can be the NPC Beethro
		behaviorFlags.insert(ScriptFlag::CanBeNPCBeethro);
	}

	if (bCanFluffTrack(wResolvedIdentity)) {
		behaviorFlags.insert(ScriptFlag::PuffTarget);
	}
	if (!bCanFluffKill(wResolvedIdentity)) {
		behaviorFlags.insert(ScriptFlag::PuffImmune);
	}
}

//*****************************************************************************
void CCharacter::SetDefaultMovementType()
//Sets the character's eMovement to the appropriate type for its identity
{
	switch (GetResolvedIdentity()) {
		//These types can move through wall.
		case M_SEEP:
			eMovement = MovementType::WALL;
		break;

		//These types can also move over pits.
		case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
			eMovement = MovementType::AIR;
		break;

		case M_WATERSKIPPER:
		case M_SKIPPERNEST:
			eMovement = MovementType::WATER;
		break;

		default:
			eMovement = MovementType::GROUND_AND_SHALLOW_WATER;
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
	while (s < 32 && (n >> s))
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
		ASSERT(command.pSpeech); //This was commented out in RPG: may still need to be commented out in F&M
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
		Upgrade2_0CommandTo3_0(command, commands);
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
		//Upgrade2_0CommandTo3_0(command, commands);
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

//********************* 3.0.2 rev2 serialization *****************************/

//*****************************************************************************
UINT CCharacter::readUINT(const BYTE* buffer, UINT& index)
//Deserialize 4-bytes --> UINT
{
	ASSERT(sizeof(UINT) == 4);
	const BYTE *buffer2 = buffer + index;
	UINT n = 0;
	for (UINT i=4; i--; )
	{
		n <<= 8;
		n += UINT(buffer2[i]);
	}

	index += sizeof(UINT);

	return n;
}

//*****************************************************************************
void CCharacter::writeUINT(string& buffer, UINT n)
//Serialize UINT --> 4-bytes
{
	ASSERT(sizeof(UINT) == 4);
	for (UINT i=5; --i; n >>= 8)
		buffer.append(1, BYTE(n & 0xff));
}

//*****************************************************************************
void CCharacter::DeserializeCommands_3_0_2_2(BYTE* buffer, const UINT bufferSize, COMMAND_VECTOR& commands)
//Extracts commands serialized in 'buffer' into 'commands'.
//This data format was used briefly for game version 3.0.2 rev2.
{
	UINT index=0;
	while (index < bufferSize)
	{
		CCharacterCommand command;

		command.command = CCharacterCommand::CharCommand(buffer[index++]);
		command.x = readUINT(buffer, index);
		command.y = readUINT(buffer, index);
		command.w = readUINT(buffer, index);
		command.h = readUINT(buffer, index);
		command.flags = readUINT(buffer, index);
		const UINT speechID = readUINT(buffer, index);
		if (speechID)
		{
			command.pSpeech = g_pTheDB->Speech.GetByID(speechID);
			ASSERT(command.pSpeech);
		}

		const UINT labelSize = readUINT(buffer, index);
		string str;
		str.resize(labelSize);
		for (UINT i=0; i<labelSize; ++i)
			str[i] = buffer[index++];
		Base64::decode(str, command.label);

		Upgrade2_0CommandTo3_0(command, commands);
	}
	ASSERT(index == bufferSize);
}

//*****************************************************************************
/*
void CCharacter::SerializeCommands_3_0_2_2(string& buffer) const
//Returns: allocated byte array containing 'commands' in serialized form
{
	const UINT wNumCommands = this->commands.size();

	buffer.resize(0);
	buffer.reserve(wNumCommands * 10 * sizeof(UINT)); //heuristic

	//Pack each command in sequence.
	for (UINT wIndex=0; wIndex<wNumCommands; ++wIndex)
	{
		const CCharacterCommand& command = this->commands[wIndex];
		ASSERT(command.command < CCharacterCommand::CC_Count);

		buffer.append(1, BYTE(command.command));
		writeUINT(buffer, command.x);
		writeUINT(buffer, command.y);
		writeUINT(buffer, command.w);
		writeUINT(buffer, command.h);
		writeUINT(buffer, command.flags);
		writeUINT(buffer, command.pSpeech ? command.pSpeech->dwSpeechID : 0);

		const string label = Base64::encode(command.label);
		writeUINT(buffer, label.size());
		buffer += label;
	}
}
*/

//*****************************************************************************
void CCharacter::SetExtraVarsFromMembers(const bool bHoldChar)
//Packs command script.
{
	SetExtraVarsFromMembersWithoutScript(bHoldChar);

	//Save command sequence (script) in packed vars member.
	SaveCommands(this->ExtraVars, this->commands);
}

//*****************************************************************************
void CCharacter::SetExtraVarsFromMembersWithoutScript(const bool bHoldChar)
//Packs NPC state info.
{
	//Only save info currently in NPC object's data structures.
	this->ExtraVars.Clear();

	if (!bHoldChar)
	{
		this->ExtraVars.SetVar(idStr, this->wLogicalIdentity);
		this->ExtraVars.SetVar(visibleStr, this->bVisible);
	}

	//Stats.
	if (this->paramX != NO_OVERRIDE)
		this->ExtraVars.SetVar(ParamXStr, this->paramX);
	if (this->paramY != NO_OVERRIDE)
		this->ExtraVars.SetVar(ParamYStr, this->paramY);
	if (this->paramW != NO_OVERRIDE)
		this->ExtraVars.SetVar(ParamWStr, this->paramW);
	if (this->paramH != NO_OVERRIDE)
		this->ExtraVars.SetVar(ParamHStr, this->paramH);
	if (this->paramF != NO_OVERRIDE)
		this->ExtraVars.SetVar(ParamFStr, this->paramF);
	this->ExtraVars.SetVar(ParamProcessSequenceStr, this->wProcessSequence);

	//ASSERT(this->dwScriptID);
	this->ExtraVars.SetVar(scriptIDstr, this->dwScriptID);
}

//*****************************************************************************
void CCharacter::SetBaseMembersFromExtraVars()
//Use default values if these packed vars do not exist.
//Otherwise, override them.
{
	this->wLogicalIdentity = this->ExtraVars.GetVar(idStr, 0);

	this->wIdentity = this->wLogicalIdentity; //by default, these are the same

	this->bVisible = this->ExtraVars.GetVar(visibleStr, false);
	if (!this->bVisible)
		this->bWeaponSheathed = true;

	//Stats.
	this->paramX = this->ExtraVars.GetVar(ParamXStr, this->paramX);
	this->paramY = this->ExtraVars.GetVar(ParamYStr, this->paramY);
	this->paramW = this->ExtraVars.GetVar(ParamWStr, this->paramW);
	this->paramH = this->ExtraVars.GetVar(ParamHStr, this->paramH);
	this->paramF = this->ExtraVars.GetVar(ParamFStr, this->paramF);

	this->wProcessSequence = this->ExtraVars.GetVar(ParamProcessSequenceStr, this->wProcessSequence);
}

//*****************************************************************************
void CCharacter::SetMembersFromExtraVars()
//Reads vars from ExtraVars to reconstruct the character's ID and command sequence.
{
	SetBaseMembersFromExtraVars();
	
	LoadCommands(this->ExtraVars, this->commands);

	this->dwScriptID = this->ExtraVars.GetVar(scriptIDstr, UINT(0));
}

//*****************************************************************************
void CCharacter::Upgrade2_0CommandTo3_0(CCharacterCommand& command, COMMAND_VECTOR& commands)
//Upgrading of deprecated 2.0 commands to 3.0 replacements.
{
	switch (command.command)
	{
		case CCharacterCommand::CC_GotoIf:
			command.command = CCharacterCommand::CC_If;
			//rest of upgrade gets handled after next command is processed
		break;

		case CCharacterCommand::CC_WaitForMonster:
			command.command = CCharacterCommand::CC_WaitForRect;
			command.flags = ScriptFlag::MONSTER;
		break;
		case CCharacterCommand::CC_WaitForNotMonster:
			command.command = CCharacterCommand::CC_WaitForNotRect;
			command.flags = ScriptFlag::MONSTER;
		break;

		case CCharacterCommand::CC_WaitForHalph:
			command.command = CCharacterCommand::CC_WaitForRect;
			command.flags = ScriptFlag::HALPH;
		break;
		case CCharacterCommand::CC_WaitForNotHalph:
			command.command = CCharacterCommand::CC_WaitForNotRect;
			command.flags = ScriptFlag::HALPH;
		break;

		case CCharacterCommand::CC_WaitForCharacter:
			command.command = CCharacterCommand::CC_WaitForRect;
			command.flags = ScriptFlag::NPC;
		break;
		case CCharacterCommand::CC_WaitForNotCharacter:
			command.command = CCharacterCommand::CC_WaitForNotRect;
			command.flags = ScriptFlag::NPC;
		break;

		case CCharacterCommand::CC_SetMusic:
			//Update 2.0 style song refs to the current style naming system.
			if ((int)command.x >= 5 && (int)command.x <= 16)
			{
				string songName;
				switch (command.x)
				{
					case 5: songName = "FoundationExit"; break;
					case 6: songName = "Deep SpacesExit"; break;
					case 7: songName = "IceworksExit"; break;
					case 8: songName = "FoundationAmbient"; break;
					case 9: songName = "FoundationAttack"; break;
					case 10: songName = "FoundationPuzzle"; break;
					case 11: songName = "Deep SpacesAmbient"; break;
					case 12: songName = "Deep SpacesAttack"; break;
					case 13: songName = "Deep SpacesPuzzle"; break;
					case 14: songName = "IceworksAmbient"; break;
					case 15: songName = "IceworksAttack"; break;
					case 16: songName = "IceworksPuzzle"; break;
					default: break; //don't change others
				}
				if (!songName.empty()) {
					UTF8ToUnicode(songName.c_str(), command.label);
					command.x = 0;
				}
			}
		break;

		case CCharacterCommand::CC_Question:
		case CCharacterCommand::CC_AnswerOption:
			//Update 2.0-format questions and answers to use speech objects
			//to reference message texts instead of a single label text
			//to facilitate localization.
			if (!command.pSpeech)
			{
				command.pSpeech = g_pTheDB->Speech.GetNew();
				command.pSpeech->MessageText = command.label.c_str();
				command.pSpeech->Update();
				command.label.resize(0);
			}
		break;

		default: break; //all other commands are current
	}

	commands.push_back(command);

	//Complete upgrading a half-changed GotoIf command.
	if (commands.size() >= 2)
	{
		CCharacterCommand& oldCommand=commands[commands.size()-2];
		if (oldCommand.command == CCharacterCommand::CC_If && oldCommand.x)
		{
			CCharacterCommand& condition=commands.back();
			if (condition.command == CCharacterCommand::CC_Label)
			{
				//Special case: If ... goto Label won't work in 3.0 syntax.
				//Fortunately, it's a no-op, so removing the If ... goto
				//doesn't change the script semantic.
				commands.erase(commands.begin() + (commands.size()-2));
			} else {
				CCharacterCommand c;
				c.command = CCharacterCommand::CC_GoTo;
				c.x = oldCommand.x;
				oldCommand.x = 0;
				commands.push_back(c);
				CCharacterCommand cEnd;
				cEnd.command = CCharacterCommand::CC_IfEnd;
				commands.push_back(cEnd);
			}
		}
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
	//Pack vars.
	if (bSaveScript)
		SetExtraVarsFromMembers();
	else
		SetExtraVarsFromMembersWithoutScript();

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

//*****************************************************************************
void CCharacter::Delete()
//Deletes speech records (exclusively) owned by character from DB.
{
	SetMembersFromExtraVars();
	for (UINT wIndex=this->commands.size(); wIndex--; )
	{
		if (this->commands[wIndex].pSpeech)
			g_pTheDB->Speech.Delete(this->commands[wIndex].pSpeech->dwSpeechID);
	}
}

//*****************************************************************************
bool CCharacter::SetWeaponSheathed()
//Sets and returns whether NPC's sword is sheathed.
//Currently, this is based on whether double is standing on goo.
{
	if (CPlayerDouble::SetWeaponSheathed())
		return true;
	//If player is marked to not have a sword, then NPC Beethro does not either.
	if (bIsSmitemaster(GetResolvedIdentity()) && this->pCurrentGame->swordsman.bWeaponOff)
	{
		this->bWeaponSheathed = true;
		return true;
	}
	return false;
}

//
// Private methods
//

//*****************************************************************************
void CCharacter::Disappear()
//Removes the NPC from the room, but not the monster list.
{
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);
	CDbRoom& room = *(this->pCurrentGame->pRoom);

	this->bVisible = false;
	this->bWeaponSheathed = true;
	RefreshBriars();

	ASSERT(room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] == this);
	room.pMonsterSquares[room.ARRAYINDEX(this->wX,this->wY)] = NULL;
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
			break;
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
			break;
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
bool CCharacter::JumpToCommandWithLabel(const WCHAR *pText)
//Returns: true if label existed and jump succeeded, else false
{
	if (pText)
		for (UINT wIndex=this->commands.size(); wIndex--; )
		{
			const CCharacterCommand& command = this->commands[wIndex];
			if (command.command == CCharacterCommand::CC_Label &&
					!WCScmp(pText, command.label.c_str()))
			{
				this->wCurrentCommandIndex = wIndex;
				return true;
			}
		}
	return false;
}

//*****************************************************************************
bool CCharacter::JumpToCommandWithLabel(const UINT num)
{
	WCHAR temp[12];
	_itoW(num, temp, 10);
	return JumpToCommandWithLabel(temp);
}

//*****************************************************************************
// Characters can be pushed and not be stunned, so we want to make sure that
// a moved character won't execute any move action during its turn
void CCharacter::PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents)
{
	if (this->bNotPushable){
		return;
	}

	this->bWasPushed = true;
	if (this->bStunnable)
		this->bPreventMoveAfterPush = true;

	const UINT wOldX = this->wX, wOldY = this->wY;

	CMonster::PushInDirection(dx, dy, bStun, CueEvents);
	SetWeaponSheathed();
	RefreshBriars();


	if (this->bBrainPathmapObstacle) {
		CDbRoom& room = *(this->pCurrentGame->pRoom);

		room.UpdatePathMapAt(wOldX, wOldY);
		room.UpdatePathMapAt(this->wX, this->wY);
	}

	if (HasSword())
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		SetWeaponSheathed();
		this->wSwordMovement = nGetO(dx,dy);
		pGame->ProcessArmedMonsterWeapon(this, CueEvents);
	}
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
	//Before he moves, remember important square contents.
	CDbRoom& room = *(this->pCurrentGame->pRoom);
	const UINT wOTile = room.GetOSquare(this->wX, this->wY);
	const bool bWasOnPlatform = bIsPlatform(wOTile);

	Move(this->wX + dx, this->wY + dy, &CueEvents);
	this->wSwordMovement = nGetO(dx,dy);
	if (bFaceDirection){	//allow turning?
		SetOrientation(dx, dy);	//character faces the direction it actually moves
		this->wPrevO = this->wO;
	}
	
	if (CanDropTrapdoor(wOTile))
		room.DestroyTrapdoor(this->wX - dx, this->wY - dy, CueEvents);


	//Process any and all of these item interactions.
	if (bWasOnPlatform && HasBehavior(ScriptFlag::MovePlatforms))
	{
		const UINT wOTile = room.GetOSquare(this->wX, this->wY);
		if (bIsPit(wOTile) || bIsDeepWater(wOTile))
			room.MovePlatform(this->wX - dx, this->wY - dy, nGetO(dx,dy));
	}

	UINT tTile = room.GetTSquare(this->wX, this->wY);

	if (bIsTLayerCoveringItem(tTile) && HasBehavior(ScriptFlag::PushObjects))
	{
		room.PushTLayerObject(this->wX, this->wY, this->wX + dx, this->wY + dy, CueEvents);
		tTile = room.GetTSquare(this->wX, this->wY); //also check what was under the item
	}

	if (HasBehavior(ScriptFlag::ActivateTokens)) {
		if (tTile == T_TOKEN)
			room.ActivateToken(CueEvents, this->wX, this->wY, this);
	}

	SetWeaponSheathed();
	RefreshBriars();

	//If player was stepped on, kill him.
	if (!this->bSafeToPlayer && this->pCurrentGame->IsPlayerAt(this->wX, this->wY))
	{
		CCurrentGame *pGame = (CCurrentGame*)this->pCurrentGame; //non-const
		pGame->SetDyingEntity(&pGame->swordsman, this);
		CueEvents.Add(CID_MonsterKilledPlayer, this);
	}

	if (this->bBrainPathmapObstacle) {
		room.UpdatePathMapAt(this->wX, this->wY);
		room.UpdatePathMapAt(this->wX - dx, this->wY - dy);
	}
}

//*****************************************************************************
void CCharacter::RefreshBriars()
// Refresh briars if the NPC can block them
// Do so by acting as if a new tile has been plotted at the character's position
{
	if (HasBehavior(ScriptFlag::BriarImmune)) {
		CDbRoom& room = *(this->pCurrentGame->pRoom);
		room.briars.plotted(this->wX, this->wY, T_EMPTY);
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

	const UINT wOldX = this->wX;
	const UINT wOldY = this->wY;

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
		SetWeaponSheathed();

		//Check for stepping on pressure plate.
		if (room.GetOSquare(this->wX, this->wY) == T_PRESSPLATE && CanPressPressurePlates())
			room.ActivateOrb(this->wX, this->wY, CueEvents, OAT_PressurePlate);
	}

	room.Plot(coords); //update room

	if (this->bBrainPathmapObstacle) {
		room.UpdatePathMapAt(this->wX, this->wY);
		room.UpdatePathMapAt(wOldX, wOldY);
	}
}

//*****************************************************************************
void CCharacter::TurnIntoMonster(
//Replace with normal monster of specified type.
//
//Params:
	CCueEvents& CueEvents, //(in/out)
	const bool bSpecial)  //special transformation behavior [default=false]
{
	bool bActiveEye = false;
	UINT identity = GetIdentity();
	if (!IsValidMonsterType(identity))
	{
		if (identity != M_EYE_ACTIVE)
		{
			//This is a pseudo-monster type.  Just stop executing script.
			this->wCurrentCommandIndex = this->commands.size();

			//Special option: when the player is not in the room,
			//replace this character with the player at this location.
			if (this->pCurrentGame->swordsman.wIdentity == M_NONE &&
					identity >= CHARACTER_FIRST) //non-monster only
			{
				if (IsVisible())
					Disappear();
				CCurrentGame *pCurrentGame = const_cast<CCurrentGame*>(this->pCurrentGame);
				CSwordsman& player = pCurrentGame->swordsman;
				player.Move(this->wX, this->wY);
				player.Move(this->wX, this->wY); //removes prev coords
				player.SetOrientation(this->wO);
				pCurrentGame->SetPlayerWeaponSheathedState();
				pCurrentGame->SetPlayerRole(this->wLogicalIdentity, CueEvents);
			}

			return;
		}

		//Turn active evil eye into evil eye.
		identity = M_EYE;
		bActiveEye = true;
	}

	CCueEvents Ignored;
	this->pCurrentGame->pRoom->KillMonster(this, Ignored, true);

	if (!this->bVisible)
		return; //not in room -- don't change to real monster type

	CMonster *pMonster = this->pCurrentGame->pRoom->AddNewMonster(identity, this->wX, this->wY);
	pMonster->bIsFirstTurn = true;
	switch (pMonster->wType)	//fix orientation
	{
		case M_SKIPPERNEST:
		case M_BRAIN: pMonster->wO = NO_ORIENTATION; break;
		case M_FEGUNDO:
			pMonster->wO = this->wO;
			if (bSpecial)
			{
				CFegundo *pFegundo = DYN_CAST(CFegundo*, CMonster*, pMonster);
				pFegundo->Explode(CueEvents);
			}
		break;
		case M_ROCKGOLEM:
			// Rock Golems are special case, when you turn them into monster as per rules they should turn
			// into rock pile if their orientation is none (eg by calling Die Special) so we just simualate
			// stabbing them (to not duplicate functionality). Unfortunately rock golem changing shallow water
			// into stepping stones is not handled in their code so we duplicate it here.
			if (this->wO == NO_ORIENTATION){
				CCueEvents Ignored;
				pMonster->OnStabbed(Ignored);

				if (this->pCurrentGame->pRoom->GetOSquare(this->wX, this->wY) == T_SHALLOW_WATER){
					this->pCurrentGame->pRoom->KillMonster(pMonster, CueEvents, true);
					this->pCurrentGame->pRoom->Plot(this->wX, this->wY, T_STEP_STONE);
					CueEvents.Add(CID_Splash, new CCoord(this->wX, this->wY), true);
				}
			} else {
				pMonster->wO = this->wO; break;
			}
		break;
		case M_EYE:
			if (bActiveEye)
			{
				CEvilEye *pEye = DYN_CAST(CEvilEye*, CMonster*, pMonster);
				pEye->SetActive();
			}
		//NO BREAK
		default: pMonster->wO = this->wO; break;
	}

	if (bEntityHasSword(pMonster->wType))
	{
		CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
		pArmedMonster->weaponType = GetWeaponType();
		pArmedMonster->bNoWeapon = this->bNoWeapon;
		pArmedMonster->SetWeaponSheathed();
	}

	this->bReplaced = true;
	CueEvents.Add(CID_NPCTypeChange);
}
