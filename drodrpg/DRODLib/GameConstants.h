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

#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/UtilFuncs.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

#include <SDL.h>

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
#define COMMAND_COUNT (26)
#define CMD_ADVANCE_CUTSCENE (COMMAND_COUNT+1) //access hook for front end call only
#define CMD_BATTLE_KEY (COMMAND_COUNT+2)       //access hook for front end processing only
#define CMD_ADVANCE_COMBAT (COMMAND_COUNT+3)   //access hook for front end call only

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
		DCMD_UseAccessory,
		DCMD_Lock,
		DCMD_Command,

		DCMD_Count,
		DCMD_NotFound=DCMD_Count
	};
	extern const char *COMMANDNAME_ARRAY[DCMD_Count];
	extern const UINT COMMAND_MIDS[DCMD_Count];

	extern DCMD getCommandIDByVarName(const WSTRING& wtext);

	extern MESSAGE_ID KeyToMID(const SDL_Keycode nKey);
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
		case CMD_ANSWER: return true;
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
