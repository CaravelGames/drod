// $Id: GameConstants.h 9656 2011-08-25 16:13:27Z mrimer $

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

#ifndef GAMECONSTANTS_H
#define GAMECONSTANTS_H

#include <BackEndLib/InputKey.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/UtilFuncs.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

#include "../Texts/MIDs.h"

#include <SDL.h>
#include <unordered_map>

//Global app parameters.
extern const char szCompanyName[];

extern const char szDROD[];
extern const WCHAR wszDROD[];
extern const char szDROD_VER[];
extern const WCHAR wszDROD_VER[];
extern const char szUserspaceFolder[];

extern const UINT VERSION_NUMBER;
extern const UINT NEXT_VERSION_NUMBER;

extern const WCHAR wszVersionReleaseNumber[];

//Swordsman commands.
#define CMD_UNSPECIFIED (0)
#define CMD_N           (1)
#define CMD_NE          (2)
#define CMD_W           (3)
#define CMD_E           (5)
#define CMD_SW          (6)
#define CMD_S           (7)
#define CMD_SE          (8)
#define CMD_C           (9)
#define CMD_CC          (10)
#define CMD_NW          (11)
#define CMD_WAIT        (12)
#define CMD_LOCK        (13)
#define CMD_RESTART     (14)
#define CMD_YES         (15)
#define CMD_NO          (16)
#define CMD_USE_WEAPON  (17)
#define CMD_USE_ARMOR   (18)
#define CMD_UNDO      (19)
#define CMD_USE_ACCESSORY  (20)
#define CMD_EXEC_COMMAND (21) //request to invoke architect-defined global custom script
#define CMD_ANSWER    (22)
#define CMD_EXITROOM  (23) //used in storing game session's move sequence -- a marker between rooms
#define CMD_ENDMOVE   (24) //used in validating a game session's move sequence -- marks the end of the command sequence
#define CMD_SETVAR    (25) //tracks vars altered through the playtest/cheat terminal popup
#define CMD_WORLD_MAP (26) //used in storing game session's move sequence - world map interaction
#define COMMAND_COUNT (27)
//access hook for front end calls
#define CMD_ADVANCE_CUTSCENE (COMMAND_COUNT+1)
#define CMD_BATTLE_KEY (COMMAND_COUNT+2)
#define CMD_ADVANCE_COMBAT (COMMAND_COUNT+3)
#define CMD_SCORE_KEY (COMMAND_COUNT+4)
#define CMD_EXTRA_SAVE_GAME (COMMAND_COUNT+5)
#define CMD_EXTRA_LOAD_GAME (COMMAND_COUNT+6)
#define CMD_EXTRA_QUICK_SAVE (COMMAND_COUNT+7)
#define CMD_EXTRA_QUICK_LOAD (COMMAND_COUNT+8)
#define CMD_EXTRA_SKIP_SPEECH (COMMAND_COUNT+9)
#define CMD_EXTRA_OPEN_CHAT (COMMAND_COUNT+10)
#define CMD_EXTRA_CHAT_HISTORY (COMMAND_COUNT+11)
#define CMD_EXTRA_SCREENSHOT (COMMAND_COUNT+12)
#define CMD_EXTRA_SAVE_ROOM_IMAGE (COMMAND_COUNT+13)
#define CMD_EXTRA_SHOW_HELP (COMMAND_COUNT+14)
#define CMD_EXTRA_SETTINGS (COMMAND_COUNT+15)
#define CMD_EXTRA_TOGGLE_FULL_SCREEN (COMMAND_COUNT+16)
#define CMD_EXTRA_TOGGLE_TURN_COUNT (COMMAND_COUNT+17)
#define CMD_EXTRA_TOGGLE_HOLD_VARS (COMMAND_COUNT+18)
#define CMD_EXTRA_TOGGLE_FRAME_RATE (COMMAND_COUNT+19)
#define CMD_EXTRA_EDIT_VARS (COMMAND_COUNT+20)
#define CMD_EXTRA_LOG_VARS (COMMAND_COUNT+21)
#define CMD_EXTRA_RELOAD_STYLE (COMMAND_COUNT+22)
#define CMD_EXTRA_EDITOR_CUT (COMMAND_COUNT+23)
#define CMD_EXTRA_EDITOR_COPY (COMMAND_COUNT+24)
#define CMD_EXTRA_EDITOR_PASTE (COMMAND_COUNT+25)
#define CMD_EXTRA_EDITOR_UNDO (COMMAND_COUNT+26)
#define CMD_EXTRA_EDITOR_REDO (COMMAND_COUNT+27)
#define CMD_EXTRA_EDITOR_DELETE (COMMAND_COUNT+28)
#define CMD_EXTRA_EDITOR_PLAYTEST_ROOM (COMMAND_COUNT+29)
#define CMD_EXTRA_EDITOR_REFLECT_X (COMMAND_COUNT+30)
#define CMD_EXTRA_EDITOR_REFLECT_Y (COMMAND_COUNT+31)
#define CMD_EXTRA_EDITOR_ROTATE_CW (COMMAND_COUNT+32)
#define CMD_EXTRA_EDITOR_SET_FLOOR_IMAGE (COMMAND_COUNT+33)
#define CMD_EXTRA_EDITOR_SET_OVERHEAD_IMAGE (COMMAND_COUNT+34)
#define CMD_EXTRA_EDITOR_TOGGLE_CHARACTER_PREVIEW (COMMAND_COUNT+35)
#define CMD_EXTRA_EDITOR_PREV_LEVEL (COMMAND_COUNT+36)
#define CMD_EXTRA_EDITOR_NEXT_LEVEL (COMMAND_COUNT+37)
#define CMD_EXTRA_EDITOR_LOG_VAR_REFS (COMMAND_COUNT+38)
#define CMD_EXTRA_EDITOR_HOLD_STATS (COMMAND_COUNT+39)
#define CMD_EXTRA_EDITOR_LEVEL_STATS (COMMAND_COUNT+40)
#define CMD_EXTRA_SCRIPT_SELECT_ALL (COMMAND_COUNT+41)
#define CMD_EXTRA_SCRIPT_TO_TEXT (COMMAND_COUNT+42)
#define CMD_EXTRA_SCRIPT_FROM_TEXT (COMMAND_COUNT+43)

//Sword orientation.
static const UINT NW = 0;
static const UINT N = 1;
static const UINT NE = 2;
static const UINT W = 3;
static const UINT E = 5;
static const UINT SW = 6;
static const UINT S = 7;
static const UINT SE = 8;
static const UINT NO_ORIENTATION = 4;
static const UINT ORIENTATION_COUNT = 9;

//******************************************************************************************
namespace InputCommands
{
	const InputKey UseDesktopKey = 0;

	class KeyDefinition {
	public:
		KeyDefinition(const UINT eCommand,
			const char* settingName,
			MID_CONSTANT commandMID,
			const InputKey defaultKeyDesktop,
			const InputKey defaultKeyNotebook = UseDesktopKey)
			: eCommand(eCommand),
			settingName(settingName),
			commandMID(commandMID),
			defaultKeyDesktop(defaultKeyDesktop),
			defaultKeyNotebook(defaultKeyNotebook == UseDesktopKey ? defaultKeyDesktop : defaultKeyNotebook)
		{
		}

		const InputKey GetDefaultKey(const UINT wKeyboardMode) const
		{
			return wKeyboardMode == 1 ? this->defaultKeyNotebook : this->defaultKeyDesktop;
		}

		const UINT eCommand;

		const InputKey defaultKeyDesktop;
		const InputKey defaultKeyNotebook;
		const char* settingName;
		const MID_CONSTANT commandMID;
	};

	enum DCMD
	{
		DCMD_First = 0,
		DCMD_NW = DCMD_First,
		DCMD_N,
		DCMD_NE,
		DCMD_W,
		DCMD_Wait,
		DCMD_E,
		DCMD_SW,
		DCMD_S,
		DCMD_SE,
		DCMD_C,
		DCMD_CC,
		DCMD_Restart,
		DCMD_Undo,
		DCMD_Battle,
		DCMD_UseWeapon,
		DCMD_UseArmor,
		DCMD_UseAccessory,
		DCMD_Lock,
		DCMD_Command,
		DCMD_ShowScore,

		// Below are keymaps which are not commands but other actions in the game
		//Save and load
		DCMD_SaveGame,
		DCMD_LoadGame,
		DCMD_QuickSave,
		DCMD_QuickLoad,

		//Dialogs and screens
		DCMD_SkipSpeech,
		DCMD_OpenChat,
		DCMD_ChatHistory,
		DCMD_Screenshot,
		DCMD_SaveRoomImage,
		DCMD_ShowHelp,
		DCMD_Settings,
		DCMD_ToggleFullScreen,

		//Technical keys
		DCMD_ToggleTurnCount,
		DCMD_ToggleHoldVars,
		DCMD_ToggleFrameRate,
		DCMD_EditVars,
		DCMD_LogVars,
		DCMD_ReloadStyle,

		//Editor
		DCMD_Editor_Cut,
		DCMD_Editor_Copy,
		DCMD_Editor_Paste,
		DCMD_Editor_Undo,
		DCMD_Editor_Redo,
		DCMD_Editor_Delete,
		DCMD_Editor_PlaytestRoom,
		DCMD_Editor_ReflectX,
		DCMD_Editor_ReflectY,
		DCMD_Editor_RotateCW,
		DCMD_Editor_SetFloorImage,
		DCMD_Editor_SetOverheadImage,
		DCMD_Editor_ToggleCharacterPreview,
		DCMD_Editor_PrevLevel,
		DCMD_Editor_NextLevel,
		DCMD_Editor_LogVarRefs,

		DCMD_Editor_HoldStats,
		DCMD_Editor_LevelStats,

		//Script editor
		DCMD_Script_SelectAll,
		DCMD_Script_ToText,
		DCMD_Script_FromText,

		DCMD_Count,
		DCMD_NotFound=DCMD_Count,
		DCMD_ExtraKeys = DCMD_SaveGame
	};

	extern const std::unordered_map<DCMD, KeyDefinition*> COMMAND_MAP;

	extern DCMD getCommandIDByVarName(const WSTRING& wtext);
	extern const KeyDefinition* GetKeyDefinition(const UINT nCommand);
	extern const bool DoesCommandUseModifiers(const DCMD eCommand);

	extern MESSAGE_ID KeyToMID(const SDL_Keycode nKey);
}

//Returns true if the command is a real game command
static inline bool bIsGameCommand(const int command)
{
	return command < COMMAND_COUNT;
}

//Returns true if the command is a front-end hook
static inline bool bIsVirtualCommand(const int command)
{
	return command > COMMAND_COUNT;
}

static inline bool bIsGameScreenCommand(const int command)
{
	return bIsGameCommand(command) ||
		(command >= CMD_ADVANCE_CUTSCENE && command <= CMD_EXTRA_RELOAD_STYLE);
}

static inline bool bIsEditorCommand(const int command)
{
	return bIsGameCommand(command) ||
		(command >= CMD_EXTRA_EDITOR_CUT && command <= CMD_EXTRA_EDITOR_LOG_VAR_REFS) ||
		command == CMD_EXTRA_SKIP_SPEECH || command == CMD_EXTRA_CHAT_HISTORY ||
		command == CMD_EXTRA_OPEN_CHAT || command == CMD_EXTRA_RELOAD_STYLE ||
		command == CMD_EXTRA_SHOW_HELP || command == CMD_EXTRA_EDIT_VARS ||
		command >= CMD_EXTRA_SCRIPT_SELECT_ALL;
}

static inline bool bIsEditSelectCommand(const int command)
{
	return (command >= CMD_EXTRA_EDITOR_CUT && command <= CMD_EXTRA_EDITOR_ROTATE_CW) ||
		command == CMD_EXTRA_SHOW_HELP || command == CMD_EXTRA_EDITOR_LOG_VAR_REFS ||
		command == CMD_EXTRA_EDITOR_LEVEL_STATS || command == CMD_EXTRA_EDITOR_HOLD_STATS;
}

//Returns: whether the command is a movement in a compass direction
static inline bool bIsMovementCommand(const int command)
{
	switch (command)
	{
		case CMD_N: case CMD_NE: case CMD_W: case CMD_E:
		case CMD_SW: case CMD_S: case CMD_SE: case CMD_NW:
			return true;
		default:
			return false;
	}
}

//Returns: whether the command activates an inventory item
static inline bool bIsInventoryCommand(const int command)
{
	switch (command)
	{
		case CMD_USE_WEAPON: case CMD_USE_ARMOR: case CMD_USE_ACCESSORY:
		case CMD_EXEC_COMMAND:
			return true;
		default:
			return false;
	}
}

//Returns: the command matching movement in the specified direction
static inline UINT orientationToCommand(const UINT o)
{
	switch (o)
	{
		case N: return CMD_N;
		case NE: return CMD_NE;
		case E: return CMD_E;
		case SE: return CMD_SE;
		case S: return CMD_S;
		case SW: return CMD_SW;
		case W: return CMD_W;
		case NW: return CMD_NW;
		case NO_ORIENTATION: return CMD_WAIT;
		default: return CMD_UNSPECIFIED;
	}
}

//Returns: true when command requires extra data fields
static inline bool bIsComplexCommand(const int command)
{
	switch (command)
	{
		case CMD_SETVAR:
		case CMD_ANSWER:
		case CMD_WORLD_MAP: return true;
		default: return false;
	}
}

//Returns: true when this command is for answering a question
static inline bool bIsQuestionResponseCommand(const int command)
{
	switch (command)
	{
		case CMD_YES:
		case CMD_NO:
		case CMD_ANSWER: return true;
		default: return false;
	}
}

static inline bool IsValidOrientation(const UINT o) {return o<ORIENTATION_COUNT;}

//Used to determine if a point is inside a rect.
static inline bool IsInRect(const int x, const int y,
							const int left, const int top, const int right, const int bottom)
{
	return x>=left && x<=right && y>=top && y<=bottom;
}

//Gets the next orientation# for rotating clockwise.
static inline UINT nNextCO(const UINT o) {
	switch (o)
	{
		case NW: case N: return o+1;
		case NE: case E: return o+3;
		case S: case SE: return o-1;
		case W: case SW: return o-3;
		default: return NO_ORIENTATION;
	}
}

//Gets the next orientation# for rotating counter-clockwise.
static inline UINT nNextCCO(const UINT o) {
	switch (o)
	{
		case NW: case W: return o+3;
		case E: case SE: return o-3;
		case SW: case S: return o+1;
		case N: case NE: return o-1;
		default: return NO_ORIENTATION;
	}
}

//Get the orientation# from the relative sword coordinates.  Where ox and oy range from -1 to 1.
static inline UINT nGetO(const int ox, const int oy) {return ((oy + 1) * 3) + (ox + 1);}

//Get relative horizontal sword position from orientation#.
static inline int nGetOX(const UINT o) {return (o % 3) - 1;}

//Get relative vertical sword position from orientation#.
static inline int nGetOY(const UINT o) {return (o / 3) - 1;}

static inline UINT GetOrientation(const UINT sx, const UINT sy, const UINT dx, const UINT dy) {
	return nGetO(sgn(int(dx-sx)),sgn(int(dy-sy)));
}

//Get the orientation that is in the reverse direction from orientation#.
static inline UINT nGetReverseO(const UINT o) {return nGetO(-nGetOX(o),-nGetOY(o));}

static inline UINT umax(const UINT x, const UINT y) {return x>y?x:y;}

//Returns L-infinity distance between two points.
static inline UINT nDist(const UINT x1, const UINT y1, const UINT x2, const UINT y2) {
	return umax((UINT)abs((int)(x1-x2)), (UINT)abs((int)(y1-y2)));
}

//Returns L-1 (Manhattan) distance between two points.
static inline UINT nL1Dist(const UINT x1, const UINT y1, const UINT x2, const UINT y2) {
	return abs((int)(x1-x2)) + abs((int)(y1-y2));
}

#endif //...#ifndef GAMECONSTANTS_H
