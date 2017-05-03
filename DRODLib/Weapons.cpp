// $Id: Weapons.cpp

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

#include "Weapons.h"

#include <BackEndLib/Assert.h>

bool WeaponCanAttackTowards(UINT wt, UINT wO, UINT wPrevO, int dx, int dy, UINT wSwordMovement)
{
	switch (wt) {
		case WT_Sword:
			return true;
		case WT_Dagger:
			if (wO != wSwordMovement)
				return false; //only attack when entering movement commands
			return true;
		case WT_Pickaxe:
			if (wO != wPrevO) //rotation attacks
				return true;
			if (!dx && !dy)
				return false; //must have some movement to attack
			switch (wO) { //forward moves do not attack
				case N: return dy >= 0;
				case S: return dy <= 0;
				case E: return dx <= 0;
				case W: return dx >= 0;
				case NW: return dy > 0 || dx > 0;
				case SE: return dy < 0 || dx < 0;
				case NE: return dy > 0 || dx < 0;
				case SW: return dy < 0 || dx > 0;
				default: return false;
			}
		case WT_Spear:
			if (wO != wPrevO) //rotation doesn't attack
				return false;
			if (wO == wSwordMovement) //stab forward works
				return true;
			if (!dx && !dy)
				return false; //must have some movement to attack
			switch (wO) { //attack by forward moves only
				case N: return dy < 0;
				case S: return dy > 0;
				case E: return dx > 0;
				case W: return dx < 0;
				case NW: return dy <= 0 && dx <= 0;
				case SE: return dy >= 0 && dx >= 0;
				case NE: return dy <= 0 && dx >= 0;
				case SW: return dy >= 0 && dx <= 0;
				default: return false;
			}
		case WT_Staff: return false; //never kills directly
		case WT_Caber:
			if (wO != wPrevO) //rotation attacks
				return false;
			if (wO == wSwordMovement) //stab forwards should be dealt with as a push first
				return false;
			return !dx && !dy; //holding weapon on something performs attack
		default:
			ASSERT(!"Invalid weapon type");
			return false;
	}
}

RoomTokenType getTokenForWeaponType(WeaponType type)
{
	switch (type) {
		default: ASSERT(!"Invalid weapon type"); //no break
		case WT_Sword: return SwordToken;
		case WT_Pickaxe: return PickaxeToken;
		case WT_Spear: return SpearToken;
		case WT_Staff: return StaffToken;
		case WT_Dagger: return DaggerToken;
		case WT_Caber: return CaberToken;
	}
}

WeaponType getWeaponForTokenType(RoomTokenType type)
{
	switch (type) {
		default: ASSERT(!"Invalid weapon token type"); //no break
		case SwordToken: return WT_Sword;
		case PickaxeToken: return WT_Pickaxe;
		case SpearToken: return WT_Spear;
		case StaffToken: return WT_Staff;
		case DaggerToken: return WT_Dagger;
		case CaberToken: return WT_Caber;
	}
}
