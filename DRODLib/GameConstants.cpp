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
 * 1997, 2000, 2001, 2002, 2005, 2011 Caravel Software. All Rights Reserved.
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

const char szDROD[] = "drod"; //allows import/export of DROD-compatible XML
const WCHAR wszDROD[] = { We('d'),We('r'),We('o'),We('d'),We(0) };
const char szDROD_VER[] = "5_0";
const WCHAR wszDROD_VER[] = { We('5'),We('_'),We('0'),We(0) };
const char szUserspaceFolder[] = "DROD 5";

const UINT VERSION_NUMBER = 509; //5.2 data format version -- increment whenever export format changes
const UINT NEXT_VERSION_NUMBER = 600;

//#define DROD_VERSION_REVISION alpha-2020-11-02
#ifdef DROD_VERSION_REVISION
// (the WS macro requires c++11 or windows)
const WCHAR wszVersionReleaseNumber[] = WS("5.2.0.") WS(STRFY_EXPAND(DROD_VERSION_REVISION));
#else
const WCHAR wszVersionReleaseNumber[] = {
	We('5'),We('.'),We('2'),We('.'),We('0'),We('.'),We('1'),We('0'),We('0'),We('5'),We(0)   // 5.2.0.*
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
		keyDefinitions[DCMD_Battle] = new KeyDefinition(CMD_BATTLE_KEY, "Battle", MID_BattleKey, SDLK_KP_PLUS, SDLK_0);
		keyDefinitions[DCMD_Command] = new KeyDefinition(CMD_EXEC_COMMAND, "UseCommand", MID_UseCommandKey, SDLK_KP_PERIOD);
		keyDefinitions[DCMD_CommandTwo] = new KeyDefinition(CMD_EXEC_COMMAND_TWO, "UseCommandTwo", MID_UseCommandKeyTwo, SDLK_KP_MULTIPLY);
		keyDefinitions[DCMD_CommandThree] = new KeyDefinition(CMD_EXEC_COMMAND_THREE, "UseCommandThree", MID_UseCommandKeyThree, SDLK_KP_MINUS);
		keyDefinitions[DCMD_CloneSwitch] = new KeyDefinition(CMD_CLONE, "CloneSwitch", MID_CloneSwitch, SDLK_TAB);

		keyDefinitions[DCMD_LockRoom] = new KeyDefinition(CMD_EXTRA_LOCK_ROOM, "Key_LockRoom", MID_Command_LockRoom, BuildInputKey(0, true, false, false));
		keyDefinitions[DCMD_SkipSpeech] = new KeyDefinition(CMD_EXTRA_SKIP_SPEECH, "Key_SkipSpeech", MID_Command_SkipSpeech, SDLK_SPACE);
		keyDefinitions[DCMD_TogglePuzzleMode] = new KeyDefinition(CMD_EXTRA_TOGGLE_PUZZLE_MODE, "Key_TogglePuzzleMode", MID_Command_TogglePuzzleMode, SDLK_F3);
		keyDefinitions[DCMD_ToggleFullScreen] = new KeyDefinition(CMD_EXTRA_TOGGLE_FULL_SCREEN, "Key_ToggleFullScreen", MID_Command_ToggleFullScreen, SDLK_F10);
		keyDefinitions[DCMD_Screenshot] = new KeyDefinition(CMD_EXTRA_SCREENSHOT, "Key_Screenshot", MID_Command_Screenshot, SDLK_F11);
		keyDefinitions[DCMD_SaveRoomImage] = new KeyDefinition(CMD_EXTRA_SAVE_ROOM_IMAGE, "Key_SaveRoomImage", MID_Command_SaveRoomImage, BuildInputKey(SDLK_F11, false, false, true));
		keyDefinitions[DCMD_Stats] = new KeyDefinition(CMD_EXTRA_STATS, "Key_Stats", MID_Command_Stats, SDLK_RETURN);
		keyDefinitions[DCMD_ChatHistory] = new KeyDefinition(CMD_EXTRA_CHAT_HISTORY, "Key_ChatHistory", MID_Command_ChatHistory, BuildInputKey(SDLK_RETURN, false, false, true));
		keyDefinitions[DCMD_PuzzleModeOptions] = new KeyDefinition(CMD_EXTRA_PUZZLE_MODE_OPTIONS, "Key_PuzzleModeOptions", MID_Command_PuzzleModeOptions, BuildInputKey(SDLK_F3, false, true, false));
		keyDefinitions[DCMD_ToggleMovementOrderHint] = new KeyDefinition(CMD_EXTRA_TOGGLE_MOVE_ORDER_HINT, "Key_ToggleMovementOrderHint", MID_Command_ToggleMovementOrderHint, BuildInputKey(SDLK_F3, false, false, true));

		keyDefinitions[DCMD_ToggleTurnCount] = new KeyDefinition(CMD_EXTRA_TOGGLE_TURN_COUNT, "Key_ToggleTurnCount", MID_Command_ToggleTurnCount, SDLK_F7);
		keyDefinitions[DCMD_ToggleHoldVars] = new KeyDefinition(CMD_EXTRA_TOGGLE_HOLD_VARS, "Key_ToggleHoldVars", MID_Command_ToggleHoldVars, BuildInputKey(SDLK_F7, false, false, true));
		keyDefinitions[DCMD_ToggleFrameRate] = new KeyDefinition(CMD_EXTRA_TOGGLE_FRAME_RATE, "Key_ToggleFrameRate", MID_Command_ToggleFrameRate, BuildInputKey(SDLK_F7, false, true, false));
		keyDefinitions[DCMD_QuickDemoRecord] = new KeyDefinition(CMD_EXTRA_QUICK_DEMO_RECORD, "Key_QuickDemoRecord", MID_Command_QuickDemoRecord, SDLK_F4);
		keyDefinitions[DCMD_ToggleDemoRecord] = new KeyDefinition(CMD_EXTRA_TOGGLE_DEMO_RECORD, "Key_ToggleDemoRecord", MID_Command_ToggleDemoRecord, SDLK_F5);
		keyDefinitions[DCMD_WatchDemos] = new KeyDefinition(CMD_EXTRA_WATCH_DEMOS, "Key_WatchDemos", MID_Command_WatchDemos, SDLK_F6);
		keyDefinitions[DCMD_ShowHelp] = new KeyDefinition(CMD_EXTRA_SHOW_HELP, "Key_ShowHelp", MID_Command_ShowHelp, SDLK_F1);
		keyDefinitions[DCMD_Settings] = new KeyDefinition(CMD_EXTRA_SETTINGS, "Key_Settings", MID_Command_OpenSettings, SDLK_F2);
		keyDefinitions[DCMD_ReloadStyle] = new KeyDefinition(CMD_EXTRA_RELOAD_STYLE, "Key_ReloadStyle", MID_Command_ReloadStyle, BuildInputKey(SDLK_F3, true, false, false));
		keyDefinitions[DCMD_LogVars] = new KeyDefinition(CMD_EXTRA_LOG_VARS, "Key_LogVars", MID_Command_LogVars, BuildInputKey(SDLK_F7, true, false, false));

		keyDefinitions[DCMD_Editor_Cut] = new KeyDefinition(CMD_EXTRA_EDITOR_CUT, "Key_Editor_Cut", MID_Command_Editor_Cut, BuildInputKey(SDLK_x, false, false, true));
		keyDefinitions[DCMD_Editor_Copy] = new KeyDefinition(CMD_EXTRA_EDITOR_COPY, "Key_Editor_Copy", MID_Command_Editor_Copy, BuildInputKey(SDLK_c, false, false, true));
		keyDefinitions[DCMD_Editor_Paste] = new KeyDefinition(CMD_EXTRA_EDITOR_PASTE, "Key_Editor_Paste", MID_Command_Editor_Paste, BuildInputKey(SDLK_v, false, false, true));
		keyDefinitions[DCMD_Editor_Undo] = new KeyDefinition(CMD_EXTRA_EDITOR_UNDO, "Key_Editor_Undo", MID_Command_Editor_Undo, BuildInputKey(SDLK_z, false, false, true));
		keyDefinitions[DCMD_Editor_Redo] = new KeyDefinition(CMD_EXTRA_EDITOR_REDO, "Key_Editor_Redo", MID_Command_Editor_Redo, BuildInputKey(SDLK_y, false, false, true));
		keyDefinitions[DCMD_Editor_Delete] = new KeyDefinition(CMD_EXTRA_EDITOR_DELETE, "Key_Editor_Delete", MID_Command_Editor_Delete, SDLK_DELETE);
		keyDefinitions[DCMD_Editor_PlaytestRoom] = new KeyDefinition(CMD_EXTRA_EDITOR_PLAYTEST_ROOM, "Key_Editor_PlaytestRoom", MID_Command_Editor_PlaytestRoom, SDLK_F5);
		keyDefinitions[DCMD_Editor_ReflectX] = new KeyDefinition(CMD_EXTRA_EDITOR_REFLECT_X, "Key_Editor_ReflectX", MID_Command_Editor_ReflectX, SDLK_F7);
		keyDefinitions[DCMD_Editor_ReflectY] = new KeyDefinition(CMD_EXTRA_EDITOR_REFLECT_Y, "Key_Editor_ReflectY", MID_Command_Editor_ReflectY, SDLK_F8);
		keyDefinitions[DCMD_Editor_SetFloorImage] = new KeyDefinition(CMD_EXTRA_EDITOR_SET_FLOOR_IMAGE, "Key_Editor_SetFloorImage", MID_Command_Editor_SetFloorImage, SDLK_F9);
		keyDefinitions[DCMD_Editor_SetOverheadImage] = new KeyDefinition(CMD_EXTRA_EDITOR_SET_OVERHEAD_IMAGE, "Key_Editor_SetOverheadImage", MID_Command_Editor_SetOverheadImage, BuildInputKey(SDLK_F9, true, false, false));
		keyDefinitions[DCMD_Editor_PrevLevel] = new KeyDefinition(CMD_EXTRA_EDITOR_PREV_LEVEL, "Key_Editor_PrevLevel", MID_Command_Editor_PrevLevel, SDLK_PAGEUP);
		keyDefinitions[DCMD_Editor_NextLevel] = new KeyDefinition(CMD_EXTRA_EDITOR_NEXT_LEVEL, "Key_Editor_NextLevel", MID_Command_Editor_NextLevel, SDLK_PAGEDOWN);

		keyDefinitions[DCMD_Editor_LogVarRefs] = new KeyDefinition(CMD_EXTRA_EDITOR_LOG_VAR_REFS, "Key_Editor_LogVarRefs", MID_Command_Editor_LogVarRefs, SDLK_F2);
		keyDefinitions[DCMD_Editor_LogChallengeRefs] = new KeyDefinition(CMD_EXTRA_EDITOR_LOG_CHALLENGE_REFS, "Key_Editor_LogChallengeRefs", MID_Command_Editor_LogChallengeRefs, BuildInputKey(SDLK_F2, false, true, false));

		keyDefinitions[DCMD_Editor_HoldStats] = new KeyDefinition(CMD_EXTRA_EDITOR_HOLD_STATS, "Key_Editor_HoldStats", MID_Command_Editor_HoldStats, SDLK_F3);
		keyDefinitions[DCMD_Editor_LevelStats] = new KeyDefinition(CMD_EXTRA_EDITOR_LEVEL_STATS, "Key_Editor_LevelStats", MID_Command_Editor_LevelStats, SDLK_F4);

		keyDefinitions[DCMD_Script_SelectAll] = new KeyDefinition(CMD_EXTRA_SCRIPT_SELECT_ALL, "Key_Script_SelectAll", MID_Command_Script_SelectAll, BuildInputKey(SDLK_a, false, false, true));
		keyDefinitions[DCMD_Script_ToText] = new KeyDefinition(CMD_EXTRA_SCRIPT_TO_TEXT, "Key_Script_ToText", MID_Command_Script_ToText, BuildInputKey(SDLK_b, false, false, true));
		keyDefinitions[DCMD_Script_FromText] = new KeyDefinition(CMD_EXTRA_SCRIPT_FROM_TEXT, "Key_Script_FromText", MID_Command_Script_FromText, BuildInputKey(SDLK_b, true, false, true));
		keyDefinitions[DCMD_Script_AddCommand] = new KeyDefinition(CMD_EXTRA_SCRIPT_ADD_COMMAND, "Key_Script_AddCommand", MID_Command_Script_AddCommand, SDLK_INSERT);
		keyDefinitions[DCMD_Script_CharacterOptions] = new KeyDefinition(CMD_EXTRA_SCRIPT_CHAR_OPTIONS, "Key_Script_CharacterOptions", MID_Command_Script_CharacterOptions, SDLK_c);

		return keyDefinitions;
	}

	const std::unordered_map<DCMD, KeyDefinition *> COMMAND_MAP = BuildKeyDefinitions();

	const KeyDefinition *GetKeyDefinition(const UINT nCommand) {
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
		return (eCommand >= DCMD_NW && eCommand <= DCMD_CC) || (eCommand == DCMD_Restart);
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
				const KeyDefinition *keyDefinition = COMMAND_MAP.at(eCommand);

				if (!strcmp(suffixText, keyDefinition->settingName))
					return DCMD(i);
			}
		}

		return DCMD_NotFound;
	}

	//************************************************************************************
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
			default: return static_cast<MESSAGE_ID>((long) MID_UNKNOWN + nKey);
		}
	}
}
