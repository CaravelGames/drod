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
//CMD_ESC 13--removed, but keep space for now.
#define CMD_RESTART     (14)
#define CMD_YES         (15)
#define CMD_NO          (16)
#define CMD_RESTART_FULL   (17)
#define CMD_RESTART_PARTIAL   (18)
#define CMD_UNDO      (19)
#define CMD_CLONE     (20)
#define CMD_DOUBLE    (21)
#define CMD_ANSWER    (22)
#define CMD_EXEC_COMMAND (23)
#define CMD_SETVAR    (24) //tracks vars altered through the playtest/cheat terminal popup
#define CMD_EXEC_COMMAND_TWO (25)
#define CMD_EXEC_COMMAND_THREE (26)
#define COMMAND_COUNT (27)
#define CMD_ADVANCE_CUTSCENE (COMMAND_COUNT+1) //access hook for front end call only
#define CMD_BATTLE_KEY (COMMAND_COUNT+2)       //access hook for front end processing only
#define CMD_BUMP_N (COMMAND_COUNT+3)           //bump commands for Temporal Splits only
#define CMD_BUMP_NE (COMMAND_COUNT+4)
#define CMD_BUMP_W (COMMAND_COUNT+5)
#define CMD_BUMP_E (COMMAND_COUNT+6)
#define CMD_BUMP_SW (COMMAND_COUNT+7)
#define CMD_BUMP_S (COMMAND_COUNT+8)
#define CMD_BUMP_SE (COMMAND_COUNT+9)
#define CMD_BUMP_NW (COMMAND_COUNT+10)
#define CMD_ESCAPE (COMMAND_COUNT+11) // Used only in front end for tracking wish to exit game

// Access hooks for front end options
#define CMD_EXTRA_LOCK_ROOM (COMMAND_COUNT+12)
#define CMD_EXTRA_SKIP_SPEECH (COMMAND_COUNT+13)
#define CMD_EXTRA_TOGGLE_PUZZLE_MODE (COMMAND_COUNT+14)
#define CMD_EXTRA_TOGGLE_FULL_SCREEN (COMMAND_COUNT+15)
#define CMD_EXTRA_SCREENSHOT (COMMAND_COUNT+16)
#define CMD_EXTRA_SAVE_ROOM_IMAGE (COMMAND_COUNT+17)
#define CMD_EXTRA_STATS (COMMAND_COUNT+18)
#define CMD_EXTRA_CHAT_HISTORY (COMMAND_COUNT+19)
#define CMD_EXTRA_PUZZLE_MODE_OPTIONS (COMMAND_COUNT+20)
#define CMD_EXTRA_TOGGLE_TURN_COUNT (COMMAND_COUNT+21)
#define CMD_EXTRA_QUICK_DEMO_RECORD (COMMAND_COUNT+22)
#define CMD_EXTRA_TOGGLE_DEMO_RECORD (COMMAND_COUNT+23)
#define CMD_EXTRA_WATCH_DEMOS (COMMAND_COUNT+24)
#define CMD_EXTRA_EDITOR_CUT (COMMAND_COUNT+25)
#define CMD_EXTRA_EDITOR_COPY (COMMAND_COUNT+26)
#define CMD_EXTRA_EDITOR_PASTE (COMMAND_COUNT+27)
#define CMD_EXTRA_EDITOR_UNDO (COMMAND_COUNT+28)
#define CMD_EXTRA_EDITOR_REDO (COMMAND_COUNT+29)
#define CMD_EXTRA_EDITOR_PLAYTEST_ROOM (COMMAND_COUNT+30)
#define CMD_EXTRA_EDITOR_REFLECT_X (COMMAND_COUNT+31)
#define CMD_EXTRA_EDITOR_REFLECT_Y (COMMAND_COUNT+32)
#define CMD_EXTRA_EDITOR_SET_FLOOR_IMAGE (COMMAND_COUNT+33)
#define CMD_EXTRA_EDITOR_SET_OVERHEAD_IMAGE (COMMAND_COUNT+34)
#define CMD_EXTRA_EDITOR_PREV_LEVEL (COMMAND_COUNT+35)
#define CMD_EXTRA_EDITOR_NEXT_LEVEL (COMMAND_COUNT+36)
#define CMD_EXTRA_SHOW_HELP (COMMAND_COUNT+37)
#define CMD_EXTRA_TOGGLE_HOLD_VARS (COMMAND_COUNT+38)
#define CMD_EXTRA_TOGGLE_FRAME_RATE (COMMAND_COUNT+39)
#define EXTRA_COMMAND_COUNT (COMMAND_COUNT+40)

// Hardcoded commands used by DemoScreen that cannot be remapped
#define CMD_DEMO_SEEK_010 (EXTRA_COMMAND_COUNT + 1)
#define CMD_DEMO_SEEK_020 (EXTRA_COMMAND_COUNT + 2)
#define CMD_DEMO_SEEK_030 (EXTRA_COMMAND_COUNT + 3)
#define CMD_DEMO_SEEK_040 (EXTRA_COMMAND_COUNT + 4)
#define CMD_DEMO_SEEK_050 (EXTRA_COMMAND_COUNT + 5)
#define CMD_DEMO_SEEK_060 (EXTRA_COMMAND_COUNT + 6)
#define CMD_DEMO_SEEK_070 (EXTRA_COMMAND_COUNT + 7)
#define CMD_DEMO_SEEK_080 (EXTRA_COMMAND_COUNT + 8)
#define CMD_DEMO_SEEK_090 (EXTRA_COMMAND_COUNT + 9)
#define CMD_DEMO_SEEK_100 (EXTRA_COMMAND_COUNT + 10)
#define CMD_DEMO_NEXT (EXTRA_COMMAND_COUNT + 11)
#define CMD_DEMO_PREV (EXTRA_COMMAND_COUNT + 12)
#define CMD_DEMO_SPEED_DOWN (EXTRA_COMMAND_COUNT + 13)
#define CMD_DEMO_SPEED_UP (EXTRA_COMMAND_COUNT + 14)
#define CMD_DEMO_PAUSE (EXTRA_COMMAND_COUNT + 15)
#define CMD_DEMO_UNIFORM_SPEED (EXTRA_COMMAND_COUNT + 16)
#define CMD_DEMO_GOTO (EXTRA_COMMAND_COUNT + 17)

//Sword orientation.
//TODO make an enum?
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

//Max weather values.
#define MAX_ROOM_LIGHT (6)
#define MAX_ROOM_FOG (3)
#define MAX_ROOM_SNOW (9)
#define MAX_ROOM_RAIN (19)

//******************************************************************************************
namespace InputCommands
{
	const InputKey UseDesktopKey = 0;

	class KeyDefinition {
	public:
		KeyDefinition(const UINT eCommand,
			const char *settingName,
			MID_CONSTANT commandMID,
			const InputKey defaultKeyDesktop,
			const InputKey defaultKeyNotebook = UseDesktopKey)
			: eCommand(eCommand),
			settingName(settingName),
			commandMID(commandMID),
			defaultKeyDesktop(defaultKeyDesktop),
			defaultKeyNotebook(defaultKeyNotebook == UseDesktopKey ? defaultKeyDesktop : defaultKeyNotebook)
		{}

		const InputKey GetDefaultKey(const UINT wKeyboardMode) const
		{
			return wKeyboardMode == 1 ? this->defaultKeyNotebook : this->defaultKeyDesktop;
		}

		const UINT eCommand;

		const InputKey defaultKeyDesktop;
		const InputKey defaultKeyNotebook;
		const char *settingName;
		const MID_CONSTANT commandMID;
	};
	
	enum DCMD
	{
		DCMD_NW = 0,
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
		DCMD_Command,
		DCMD_CommandTwo,
		DCMD_CommandThree,
		DCMD_CloneSwitch,
		
		// Below are keymaps which are not commands but other actions in the game
		// Toggles/actions
		DCMD_LockRoom,
		DCMD_SkipSpeech,
		DCMD_TogglePuzzleMode,
		DCMD_ToggleFullScreen,
		DCMD_Screenshot,
		DCMD_SaveRoomImage,

		// Dialogs/screens
		DCMD_Stats,
		DCMD_ChatHistory,
		DCMD_PuzzleModeOptions,
		DCMD_ToggleTurnCount,
		DCMD_ToggleHoldVars,
		DCMD_ToggleFrameRate,
		DCMD_QuickDemoRecord,
		DCMD_ToggleDemoRecord,
		DCMD_WatchDemos,
		DCMD_ShowHelp,
		
		// Editor
		DCMD_Editor_Cut,
		DCMD_Editor_Copy,
		DCMD_Editor_Paste,
		DCMD_Editor_Undo,
		DCMD_Editor_Redo,
		DCMD_Editor_PlaytestRoom,
		DCMD_Editor_ReflectX,
		DCMD_Editor_ReflectY,
		DCMD_Editor_SetFloorImage,
		DCMD_Editor_SetOverheadImage,
		DCMD_Editor_PrevLevel,
		DCMD_Editor_NextLevel,

		DCMD_Count,
		DCMD_NotFound=DCMD_Count,
		DCMD_ExtraKeys=DCMD_LockRoom
	};
	
	extern const std::unordered_map<DCMD, KeyDefinition*> COMMAND_MAP;

	extern DCMD getCommandIDByVarName(const WSTRING& wtext);
	extern const KeyDefinition *GetKeyDefinition(const UINT nCommand);
	extern const bool DoesCommandUseModifiers(const DCMD eCommand);

	extern MESSAGE_ID KeyToMID(const SDL_Keycode nKey);
}

//******************************************************************************************

//Types of room tokens.
enum RoomTokenType
{
	RotateArrowsCW=0,
	RotateArrowsCCW=1,
	SwitchTarMud=2,
	SwitchTarGel=3,
	SwitchGelMud=4,
	TarTranslucent=5,
	PowerTarget=6,
	WeaponDisarm=7,
	PersistentCitizenMovement=8,
	ConquerToken=9,
	SwordToken=10, //swap weapon with this type
	PickaxeToken=11,
	SpearToken=12,
	StaffToken=13,
	DaggerToken=14,
	CaberToken=15,
	TemporalSplit=16,
	RoomTokenCount,
	TemporalSplitUsed=127 //only used in play
};

//******************************************************************************************
static inline bool bIsGameCommand(const int command)
{
	return command < COMMAND_COUNT;
}

static inline bool bIsVirtualCommand(const int command)
{
	return command > COMMAND_COUNT;
}

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

static inline bool bIsSpecialCommand(const int command)
{
	switch (command)
	{
		case CMD_EXEC_COMMAND:
		case CMD_EXEC_COMMAND_TWO:
		case CMD_EXEC_COMMAND_THREE:
			return true;
		default: return false;
	}
}

//Returns: true when command requires extra data fields
static inline bool bIsComplexCommand(const int command)
{
	switch (command)
	{
		case CMD_SETVAR:
		case CMD_CLONE: case CMD_DOUBLE: case CMD_ANSWER: return true;
		default: return false;
	}
}

static inline bool bIsAnswerCommand(const int command)
{
	switch (command)
	{
		case CMD_ANSWER: case CMD_YES: case CMD_NO: return true;
		default: return false;
	}
}

static inline bool bIsBumpCommand(const int command)
{
	switch (command)
	{
		case CMD_BUMP_NW: case CMD_BUMP_N: case CMD_BUMP_NE: case CMD_BUMP_W:
		case CMD_BUMP_E: case CMD_BUMP_SW: case CMD_BUMP_S: case CMD_BUMP_SE:
			return true;
		default: return false;
	}
}
static inline int ConvertToBumpCommand(const int command)
{
	switch(command)
	{
		case CMD_NW: return CMD_BUMP_NW;
		case CMD_N:  return CMD_BUMP_N;
		case CMD_NE: return CMD_BUMP_NE;
		case CMD_W:  return CMD_BUMP_W;
		case CMD_E:  return CMD_BUMP_E;
		case CMD_SW: return CMD_BUMP_SW;
		case CMD_S:  return CMD_BUMP_S;
		case CMD_SE: return CMD_BUMP_SE;
		default: return CMD_WAIT;
	}
}

static inline bool bIsEditorCommand(const int command)
{
		return (command >= CMD_EXTRA_EDITOR_CUT && command <= CMD_EXTRA_EDITOR_NEXT_LEVEL);
}

static inline bool bIsSharedCommand(const int command)
{
	return command == CMD_EXTRA_SKIP_SPEECH || command == CMD_EXTRA_CHAT_HISTORY
		|| command == CMD_EXTRA_STATS || command == CMD_EXTRA_WATCH_DEMOS
		|| command == CMD_EXTRA_SHOW_HELP;
}

static inline bool IsValidOrientation(const UINT o) {return o<ORIENTATION_COUNT;}

//Used to determine if a point is inside a rect.
static inline bool IsInRect(const int x, const int y,
							const int left, const int top, const int right, const int bottom)
{
	return x>=left && x<=right && y>=top && y<=bottom;
}

//Gets the next orientation# for a sword moving clockwise.
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

//Gets the next orientation# for a sword moving counter-clockwise.
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

static inline int nGetDX(const UINT srcX, const UINT destX) { return sgn(int(destX-srcX)); }

static inline UINT GetOrientation(const UINT sx, const UINT sy, const UINT dx, const UINT dy) {
	return nGetO(nGetDX(sx,dx), nGetDX(sy,dy));
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
