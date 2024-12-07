// $Id: EquipmentDescription.cpp $

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
 * Portions created by the Initial Developer are Copyright (C) 2007, 2023
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "EquipmentDescription.h"

#include "../DRODLib/Character.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/RoomData.h"

 //*****************************************************************************
WSTRING EquipmentDescription::GetPredefinedAccessoryAbility(UINT type)
//Returns: string with text of the accessory's ability, or empty string if none
{
	switch (type) {
	case AccessoryType::LuckyGold: return g_pTheDB->GetMessageText(MID_BehaviorLuckyGR);
	case AccessoryType::XPDoubler: return g_pTheDB->GetMessageText(MID_DoubleXP);
	default: return WSTRING();
	}
}

//*****************************************************************************
WSTRING EquipmentDescription::GetPredefinedShieldAbility(UINT type)
//Returns: string with text of the shield's ability, or empty string if none
{
	WSTRING wstr;

	if (!(type == ShieldType::WoodenShield ||
		type == ShieldType::OremiteShield ||
		type == ShieldType::ArmorSlot)) {
		wstr += g_pTheDB->GetMessageText(MID_BehaviorMetal);
	}

	return wstr;
}

//*****************************************************************************
WSTRING EquipmentDescription::GetPredefinedWeaponAbility(
	UINT type, const WSTRING& separator)
//Returns: string with text of the sword's ability, or empty string if none
{
	WSTRING wstr;

	if (type == SwordType::WoodenBlade || type == SwordType::WeaponSlot) {
		//These swords have no abilties
		return wstr;
	}

	//All swords with abilites are also metal
	wstr += g_pTheDB->GetMessageText(MID_BehaviorMetal);

	switch (type) {
		case SwordType::GoblinSword: {
			wstr += separator;
			wstr += g_pTheDB->GetMessageText(MID_BehaviorGoblinWeakness);
		}
		break;
		case SwordType::ReallyBigSword: {
			wstr += separator;
			wstr += g_pTheDB->GetMessageText(MID_BehaviorBeamBlock);
		}
		break;
		case SwordType::LuckySword: {
			wstr += separator;
			wstr += g_pTheDB->GetMessageText(MID_BehaviorLuckyGR);
		}
		break;
		case SwordType::SerpentSword: {
			wstr += separator;
			wstr += g_pTheDB->GetMessageText(MID_BehaviorSerpentWeakness);
		}
		break;
		case SwordType::BriarSword: {
			wstr += separator;
			wstr += g_pTheDB->GetMessageText(MID_BehaviorBriarCut);
		}
		break;
		default: break;
	}

	return wstr;
}

//*****************************************************************************
WSTRING EquipmentDescription::GetEquipmentAbility(
	const CCharacter* pCharacter,
	ScriptFlag::EquipmentType equipType,
	const WSTRING& separator)
//Returns: string with text of custom equipment ability, or empty string if none
{
	ASSERT(CCharacterCommand::IsRealEquipmentType(equipType));

	WSTRING text;
	bool needSeparator = false;

	if (pCharacter->IsMetal())
	{
		text += g_pTheDB->GetMessageText(MID_BehaviorMetal);
		needSeparator = true;
	}
	if (pCharacter->HasGoblinWeakness())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorGoblinWeakness);
		needSeparator = true;
	}
	if (pCharacter->HasSerpentWeakness())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorSerpentWeakness);
		needSeparator = true;
	}
	if (pCharacter->HasCustomWeakness())
	{
		if (needSeparator)
			text += separator;
		text += WCSReplace(
			g_pTheDB->GetMessageText(MID_StrongAgainstAspect),
			wszStringToken,
			pCharacter->GetCustomWeakness()
		);
		needSeparator = true;
	}
	if (pCharacter->HasRayBlocking())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorBeamBlock);
		needSeparator = true;
	}
	if (pCharacter->CanCutBriar())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorBriarCut);
		needSeparator = true;
	}
	if (pCharacter->CanCutTarAnywhere())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_CutTarAnywhere);
		needSeparator = true;
	}
	if (pCharacter->IsLuckyGR())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorLuckyGR);
		needSeparator = true;
	}
	if (pCharacter->IsLuckyXP())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_DoubleXP);
		needSeparator = true;
	}
	if (pCharacter->CanAttackFirst())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_AttackFirst);
		needSeparator = true;
	}
	if (pCharacter->CanAttackLast())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_AttackLast);
		needSeparator = true;
	}
	if (pCharacter->RemovesSword())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_RemovesSword);
		needSeparator = true;
	}
	if (pCharacter->TurnToFacePlayerWhenFighting())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_BehaviorSurprisedBehind);
		needSeparator = true;
	}
	if (pCharacter->HasNoEnemyDefense())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_NoEnemyDefense);
		needSeparator = true;
	}
	if (pCharacter->IsExplosiveSafe())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_ExplosiveSafe);
		needSeparator = true;
	}
	if (pCharacter->IsWallAndMirrorSafe())
	{
		if (needSeparator)
			text += separator;
		text += g_pTheDB->GetMessageText(MID_WallMirrorSafe);
		needSeparator = true;
	}
	if (pCharacter->HasCustomDescription()) {
		vector<WSTRING> descriptions = pCharacter->GetCustomDescriptions();

		for (size_t i = 0; i < descriptions.size(); ++i) {
			const WSTRING& description = descriptions[i];
			if (description.empty()) continue;

			if (needSeparator)
				text += separator;

			text += description;
			needSeparator = true;
		}
	}

	return text;
}
