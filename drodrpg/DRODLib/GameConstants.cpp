// $Id: GameConstants.cpp 9637 2011-07-18 19:28:55Z mrimer $

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
 * 1997, 2000, 2001, 2002, 2005, 2011, 2016 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "GameConstants.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <cstring>


#ifdef DROD_AUTODETECT_REVISION
#include "drod-revision.h"
#endif

const char szCompanyName[] = "Caravel Games";

//Current game version.  Update as needed.

const char szDROD[] = "drodrpg";
const WCHAR wszDROD[] = { We('d'),We('r'),We('o'),We('d'),We('r'),We('p'),We('g'),We(0) };
const char szDROD_VER[] = "2_0";
const WCHAR wszDROD_VER[] = { We('2'),We('_'),We('0'),We(0) };
const char szUserspaceFolder[] = "DRODRPG";

const UINT VERSION_NUMBER = 500; //2.0 data format version -- increment when exported data format changes
const UINT NEXT_VERSION_NUMBER = 600;

//#define DROD_VERSION_REVISION alpha.2020-11-01
#ifdef DROD_VERSION_REVISION
// (the WS macro requires c++11 or windows)
const WCHAR wszVersionReleaseNumber[] = WS("2.0.0.") WS(STRFY_EXPAND(DROD_VERSION_REVISION));
#else
const WCHAR wszVersionReleaseNumber[] = {
	We('2'),We('.'),We('0'),We('.'),We('0'),We('.'),We('7'),We('7'),We('4'),We('-'),We('d'),We('e'),We('v'),We(0)   // 2.0.0.* -- full version number plus build number
};
#endif


//*****************************************************************************
//Schema of player input commands
namespace InputCommands
{
	const std::unordered_map<DCMD, KeyDefinition*> BuildKeyDefinitions()
	{
		std::unordered_map<DCMD, KeyDefinition*> keyDefinitions;

		keyDefinitions[DCMD_NW] = new KeyDefinition(CMD_NW, "MoveNorthwest", MID_MoveNorthwest, SDLK_KP_7, SDLK_7);
		keyDefinitions[DCMD_N] = new KeyDefinition(CMD_N, "MoveNorth", MID_MoveNorth, SDLK_KP_8, SDLK_8);
		keyDefinitions[DCMD_NE] = new KeyDefinition(CMD_NE, "MoveNortheast", MID_MoveNortheast, SDLK_KP_9, SDLK_9);
		keyDefinitions[DCMD_W] = new KeyDefinition(CMD_W, "MoveWest", MID_MoveWest, SDLK_KP_4, SDLK_u);
		keyDefinitions[DCMD_Wait] = new KeyDefinition(CMD_WAIT, "Wait", MID_Wait, SDLK_KP_5, SDLK_i);
		keyDefinitions[DCMD_E] = new KeyDefinition(CMD_E, "MoveEast", MID_MoveEast, SDLK_KP_6, SDLK_o);
		keyDefinitions[DCMD_SW] = new KeyDefinition(CMD_SW, "MoveSouthwest", MID_MoveSouthwest, SDLK_KP_1, SDLK_j);
		keyDefinitions[DCMD_S] = new KeyDefinition(CMD_S, "MoveSouth", MID_MoveSouth, SDLK_KP_2, SDLK_k);
		keyDefinitions[DCMD_SE] = new KeyDefinition(CMD_SE, "MoveSoutheast", MID_MoveSoutheast, SDLK_KP_3, SDLK_l);
		keyDefinitions[DCMD_C] = new KeyDefinition(CMD_C, "SwingClockwise", MID_SwingClockwise, SDLK_w);
		keyDefinitions[DCMD_CC] = new KeyDefinition(CMD_CC, "SwingCounterclockwise", MID_SwingCounterclockwise, SDLK_q);

		keyDefinitions[DCMD_Restart] = new KeyDefinition(CMD_RESTART, "Restart", MID_RestartRoom, SDLK_r);
		keyDefinitions[DCMD_Undo] = new KeyDefinition(CMD_UNDO, "Undo", MID_UndoMove, SDLK_BACKSPACE);
		keyDefinitions[DCMD_Battle] = new KeyDefinition(CMD_BATTLE_KEY, "Battle", MID_BattleKey, SDLK_KP_PLUS);
		keyDefinitions[DCMD_UseWeapon] = new KeyDefinition(CMD_USE_WEAPON, "UseWeapon", MID_UseWeaponKey, BuildInputKey(SDLK_TAB, true, false, false));
		keyDefinitions[DCMD_UseArmor] = new KeyDefinition(CMD_USE_ARMOR, "UseArmor", MID_UseArmorKey, BuildInputKey(SDLK_TAB, false, false, true));
		keyDefinitions[DCMD_UseAccessory] = new KeyDefinition(CMD_USE_ACCESSORY, "UseAccessory", MID_UseAccessoryKey, SDLK_TAB);
		keyDefinitions[DCMD_Lock] = new KeyDefinition(CMD_LOCK, "Lock", MID_LockCommand, SDLK_KP_0);
		keyDefinitions[DCMD_Command] = new KeyDefinition(CMD_EXEC_COMMAND, "UseCommand", MID_UseCommandKey, SDLK_KP_PERIOD);
		keyDefinitions[DCMD_ShowScore] = new KeyDefinition(CMD_SCORE_KEY, "ShowScore", MID_ShowScore, SDLK_KP_MINUS);

		return keyDefinitions;
	}

	DCMD getCommandIDByVarName(const WSTRING& wtext)
	{
		const string text = UnicodeToUTF8(wtext);

		static const char COMMAND_PREFIX[] = "_CMD_";
		static const size_t prefix_len = strlen(COMMAND_PREFIX);
		if (!strncmp(text.c_str(), COMMAND_PREFIX, prefix_len))
		{
			const char *suffixText = text.c_str() + prefix_len;
			for (int i = 0; i < DCMD_Count; ++i) {
				const DCMD eCommand = DCMD(i);
				const KeyDefinition* keyDefinition = COMMAND_MAP.at(eCommand);

				if (!strcmp(suffixText, keyDefinition->settingName))
					return DCMD(i);
			}
		}

		return DCMD_NotFound;
	}

	const std::unordered_map<DCMD, KeyDefinition*> COMMAND_MAP = BuildKeyDefinitions();

	const KeyDefinition* GetKeyDefinition(const UINT nCommand) {
		ASSERT(nCommand < DCMD_Count);
		const DCMD eCommand = DCMD(nCommand);
		ASSERT(COMMAND_MAP.count(eCommand) == 1);
		return COMMAND_MAP.at(eCommand);
	}

	const bool DoesCommandUseModifiers(const DCMD eCommand)
	// These commands by default support Ctrl (and other) modifiers, and as such they can't
	// be mapped to keys with modifiers nor other commands may map to modifer-versions
	// of these commands
	{
		return (eCommand >= DCMD_NW && eCommand <= DCMD_SE);
	}

	MESSAGE_ID KeyToMID(const SDL_Keycode nKey)
	{
		switch (nKey) {
		case SDLK_KP_DIVIDE: return MID_KEY_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return MID_KEY_KP_MULTIPLY;
		case SDLK_KP_MINUS: return MID_KEY_KP_MINUS;
		case SDLK_KP_PLUS: return MID_KEY_KP_PLUS;
		case SDLK_KP_ENTER: return MID_KEY_KP_ENTER;
		case SDLK_KP_1: return MID_KEY_KP1;
		case SDLK_KP_2: return MID_KEY_KP2;
		case SDLK_KP_3: return MID_KEY_KP3;
		case SDLK_KP_4: return MID_KEY_KP4;
		case SDLK_KP_5: return MID_KEY_KP5;
		case SDLK_KP_6: return MID_KEY_KP6;
		case SDLK_KP_7: return MID_KEY_KP7;
		case SDLK_KP_8: return MID_KEY_KP8;
		case SDLK_KP_9: return MID_KEY_KP9;
		case SDLK_KP_0: return MID_KEY_KP0;
		case SDLK_KP_PERIOD: return MID_KEY_KP_PERIOD;
		case SDLK_F1: return MID_KEY_F1;
		case SDLK_F2: return MID_KEY_F2;
		case SDLK_F3: return MID_KEY_F3;
		case SDLK_F4: return MID_KEY_F4;
		case SDLK_F5: return MID_KEY_F5;
		case SDLK_F6: return MID_KEY_F6;
		case SDLK_F7: return MID_KEY_F7;
		case SDLK_F8: return MID_KEY_F8;
		case SDLK_F9: return MID_KEY_F9;
		case SDLK_F10: return MID_KEY_F10;
		case SDLK_F11: return MID_KEY_F11;
		case SDLK_F12: return MID_KEY_F12;
		case SDLK_HOME: return MID_KEY_HOME;
		case SDLK_END: return MID_KEY_END;
		case SDLK_PAGEUP: return MID_KEY_PAGEUP;
		case SDLK_PAGEDOWN: return MID_KEY_PAGEDOWN;
		case SDLK_SPACE: return MID_KEY_SPACE;
		case SDLK_RETURN: return MID_KEY_RETURN;
		case SDLK_CAPSLOCK: return MID_KEY_CAPSLOCK;
		case SDLK_NUMLOCKCLEAR: return MID_KEY_NUMLOCK;
		case SDLK_INSERT: return MID_KEY_INSERT;
		case SDLK_DELETE: return MID_KEY_DELETE;
		case SDLK_PRINTSCREEN: return MID_KEY_PRINT;
		case SDLK_UP: return MID_KEY_UP;
		case SDLK_DOWN: return MID_KEY_DOWN;
		case SDLK_LEFT: return MID_KEY_LEFT;
		case SDLK_RIGHT: return MID_KEY_RIGHT;
		case SDLK_APP1: return MID_UNKNOWN;
		case SDLK_APP2: return MID_UNKNOWN;
		case SDLK_APPLICATION: return MID_UNKNOWN;
		default: return static_cast<MESSAGE_ID>((long)MID_UNKNOWN + nKey);
		}
	}
}
