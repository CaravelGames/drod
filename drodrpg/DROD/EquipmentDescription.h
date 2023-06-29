// $Id: EquipmentDescription.h $

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

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>
#include "../DRODLib/CharacterCommand.h"

//****************************************************************************************
class CCharacter;
class EquipmentDescription {
public:
	static WSTRING GetPredefinedAccessoryAbility(UINT type);
	static WSTRING GetPredefinedShieldAbility(UINT type);
	static WSTRING GetPredefinedWeaponAbility(UINT type, const WSTRING& separator);

	static WSTRING GetEquipmentAbility(
		const CCharacter* pCharacter,
		ScriptFlag::EquipmentType equipType,
		const WSTRING& separator
	);
};
