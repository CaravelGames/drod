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
const char szDROD_VER[] = "1_0";
const WCHAR wszDROD_VER[] = { We('1'),We('_'),We('0'),We(0) };
const char szUserspaceFolder[] = "DRODRPG";

const UINT VERSION_NUMBER = 406; //data format version -- increment when exported data format changes
const UINT NEXT_VERSION_NUMBER = 500;

//#define DROD_VERSION_REVISION alpha.2020-11-01
#ifdef DROD_VERSION_REVISION
// (the WS macro requires c++11 or windows)
const WCHAR wszVersionReleaseNumber[] = WS("1.2.10.") WS(STRFY_EXPAND(DROD_VERSION_REVISION));
#else
const WCHAR wszVersionReleaseNumber[] = {
	We('1'),We('.'),We('2'),We('.'),We('1'),We('0'),We('.'),We('8'),We('0'),We('2'),We(0)   // 1.2.10.* -- full version number plus build number
};
#endif


//*****************************************************************************
//Schema of player input commands
namespace InputCommands
{
	const char *COMMANDNAME_ARRAY[DCMD_Count] = {
		"MoveNorthwest",
		"MoveNorth",
		"MoveNortheast",
		"MoveWest",
		"Wait",
		"MoveEast",
		"MoveSouthwest",
		"MoveSouth",
		"MoveSoutheast",
		"SwingClockwise",
		"SwingCounterclockwise",
		"Restart",
		"Undo",
		"Battle",
		"UseAccessory", //UseWeapon and UseArmor commands are piggybacked
		"Lock",
		"UseCommand"
	};

	const UINT COMMAND_MIDS[DCMD_Count] = {
		MID_MoveNorthwest, MID_MoveNorth, MID_MoveNortheast,
		MID_MoveWest, MID_Wait, MID_MoveEast,
		MID_MoveSouthwest, MID_MoveSouth, MID_MoveSoutheast,
		MID_SwingClockwise, MID_SwingCounterclockwise,
		MID_RestartRoom, MID_UndoMove, MID_BattleKey, MID_UseAccessoryKey,
		MID_LockCommand, MID_UseCommandKey
	};

	DCMD getCommandIDByVarName(const WSTRING& wtext)
	{
		const string text = UnicodeToUTF8(wtext);

		static const char COMMAND_PREFIX[] = "_CMD_";
		static const size_t prefix_len = strlen(COMMAND_PREFIX);
		if (!strncmp(text.c_str(), COMMAND_PREFIX, prefix_len))
		{
			const char *suffixText = text.c_str() + prefix_len;
			for (int i=0; i<DCMD_Count; ++i) {
				if (!strcmp(suffixText, COMMANDNAME_ARRAY[i]))
					return DCMD(i);
			}
		}

		return DCMD_NotFound;
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
		case SDLK_LEFT: return MID_KEY_LEFT;
		case SDLK_RIGHT: return MID_KEY_RIGHT;
		case SDLK_UP: return MID_KEY_UP;
		case SDLK_DOWN: return MID_KEY_DOWN;
		default: return static_cast<MESSAGE_ID>((long)MID_UNKNOWN + nKey);
		}
	}
}
