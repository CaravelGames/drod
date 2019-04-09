// $Id: Weapons.h

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

#ifndef WEAPONS_H
#define WEAPONS_H

#include "GameConstants.h"

//Weapon Types
//  Do not change these constant values.
enum WeaponType
{
	WT_HotTile=-5,
	WT_Firetrap=-4,
	WT_FloorSpikes=-3,
	WT_Off=-2,
	WT_On=-1,
	WT_Sword=0,
	WT_Pickaxe=1,
	WT_Spear=2,
	WT_Staff=3,
	WT_Dagger=4,
	WT_Caber=5,
	WT_NumWeapons
};

static inline bool bIsMetalWeapon(const UINT wt)
{
	return wt != WT_Staff;
}
static inline bool bIsHeavyWeapon(const UINT wt)
{
	return wt != WT_Dagger;
}
static inline bool bWeaponCutsWhenStationary(const UINT wt)
{
	switch (wt) {
		case WT_Sword:
		case WT_Caber:
			return true;
		default:
			return false;
	}
}

extern bool WeaponCanAttackTowards(UINT wt, UINT wO, UINT wPrevO, int dx, int dy, UINT wSwordMovement);
extern RoomTokenType getTokenForWeaponType(WeaponType type);
extern WeaponType getWeaponForTokenType(RoomTokenType type);

#endif