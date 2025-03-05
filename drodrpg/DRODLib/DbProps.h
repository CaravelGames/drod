// $Id: DbProps.h 9290 2008-10-29 02:26:59Z mrimer $

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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Declares for DROD database.

#ifndef DBPROPS_H
#define DBPROPS_H

#include <mk4.h>

//*****************************************************************************
#ifdef INCLUDED_FROM_DBBASE_CPP
#define DEFPROP(c,n) c p_##n(#n)
#define DEFTDEF(n,v) extern const char n[] = v
#else
#define DEFPROP(c,n) extern c p_##n
#define DEFTDEF(n,v) extern const char n[]
#endif

//Props.
DEFPROP(c4_IntProp,     AnimationSpeed);
DEFPROP(c4_IntProp,     BeginTurnNo);
DEFPROP(c4_IntProp,     Bottom);
DEFPROP(c4_IntProp,     CaravelNetMedia);
DEFPROP(c4_IntProp,     Character);
DEFPROP(c4_IntProp,     CharID);
DEFPROP(c4_BytesProp,   CharNameText);
DEFPROP(c4_IntProp,     Checksum);
DEFPROP(c4_BytesProp,   ChecksumStr);
DEFPROP(c4_IntProp,     CNetNameMessageID);
DEFPROP(c4_IntProp,     CNetPasswordMessageID);
DEFPROP(c4_BytesProp,   Commands);
DEFPROP(c4_IntProp,     Created);
DEFPROP(c4_IntProp,     DataFormat);
DEFPROP(c4_IntProp,     DataID);
DEFPROP(c4_IntProp,     DataIDTiles);
DEFPROP(c4_BytesProp,   DataNameText);
DEFPROP(c4_IntProp,     Delay);
DEFPROP(c4_IntProp,     DemoID);
DEFPROP(c4_IntProp,     DescriptionMessageID);
DEFPROP(c4_IntProp,     EditingPrivileges);
DEFPROP(c4_IntProp,     EMailMessageID); //deprecated
DEFPROP(c4_IntProp,     EndHoldMessageID);
DEFPROP(c4_IntProp,     EndTurnNo);
DEFPROP(c4_IntProp,     EntranceID);
DEFPROP(c4_BytesProp,   ExtraVars);
DEFPROP(c4_IntProp,     Flags);
DEFPROP(c4_IntProp,     GID_Created);
DEFPROP(c4_IntProp,     GID_LevelIndex);
DEFPROP(c4_IntProp,     GID_NewLevelIndex);
DEFPROP(c4_IntProp,     GID_OriginalNameMessageID);
DEFPROP(c4_IntProp,     GID_PlayerID);
DEFPROP(c4_IntProp,     HighScoreID);
DEFPROP(c4_IntProp,     HoldID);
DEFPROP(c4_IntProp,     ImageStartX);
DEFPROP(c4_IntProp,     ImageStartY);
//DEFPROP(c4_IntProp,     IsFirstTurn);
DEFPROP(c4_IntProp,     IsHidden);
DEFPROP(c4_IntProp,     IsLocal);
DEFPROP(c4_IntProp,     IsMainEntrance);
DEFPROP(c4_IntProp,     IsSecret);
DEFPROP(c4_IntProp,     LanguageCode);
DEFPROP(c4_IntProp,     LastUpdated);
DEFPROP(c4_IntProp,     Left);
DEFPROP(c4_IntProp,     LevelID);
DEFPROP(c4_IntProp,     MapMarker);
DEFPROP(c4_IntProp,     MapOnly);
DEFPROP(c4_IntProp,     MapState);
DEFPROP(c4_IntProp,     MessageID);
DEFPROP(c4_BytesProp,   MessageText);
DEFPROP(c4_IntProp,     MessageTextID);
DEFPROP(c4_BytesProp,   ModName);
DEFPROP(c4_IntProp,     Mood);
DEFPROP(c4_IntProp,     Multiplier);
DEFPROP(c4_IntProp,     NameMessageID);
DEFPROP(c4_IntProp,     NextDemoID);
DEFPROP(c4_IntProp,     NumBytes);
DEFPROP(c4_IntProp,     O);
DEFPROP(c4_IntProp,     OrderIndex);
DEFPROP(c4_IntProp,     OverheadDataID);
DEFPROP(c4_IntProp,     OverheadImageStartX);
DEFPROP(c4_IntProp,     OverheadImageStartY);
DEFPROP(c4_IntProp,     PlayerID);
//DEFPROP(c4_IntProp,     ProcessSequence);
DEFPROP(c4_BytesProp,   RawData);
DEFPROP(c4_IntProp,     Right);
DEFPROP(c4_IntProp,     RoomID);
DEFPROP(c4_IntProp,     RoomCols);
DEFPROP(c4_IntProp,     RoomRows);
DEFPROP(c4_IntProp,     RoomX);
DEFPROP(c4_IntProp,     RoomY);
DEFPROP(c4_IntProp,     SavedGameID);
DEFPROP(c4_IntProp,     Score);
DEFPROP(c4_BytesProp,   ScorepointName);
DEFPROP(c4_IntProp,     ScriptID);
DEFPROP(c4_BytesProp,   Settings);
DEFPROP(c4_IntProp,     ShowDescription);
DEFPROP(c4_IntProp,     ShowSequenceNo);
DEFPROP(c4_IntProp,     SpeechID);
DEFPROP(c4_BytesProp,   Squares);
DEFPROP(c4_IntProp,     StartRoomAppearance);
DEFPROP(c4_IntProp,     StartRoomO);
DEFPROP(c4_IntProp,     StartRoomSwordOff);
DEFPROP(c4_IntProp,     StartRoomX);
DEFPROP(c4_IntProp,     StartRoomY);
DEFPROP(c4_BytesProp,   Stats);
DEFPROP(c4_IntProp,     Status);
DEFPROP(c4_BytesProp,   StyleName);
DEFPROP(c4_BytesProp,   TileLights);
DEFPROP(c4_BytesProp,   TimData);
DEFPROP(c4_IntProp,     Time);
DEFPROP(c4_IntProp,     Top);
DEFPROP(c4_IntProp,     Type);
DEFPROP(c4_IntProp,     VarID);
DEFPROP(c4_BytesProp,   VarNameText);
DEFPROP(c4_IntProp,     Version);
DEFPROP(c4_IntProp,     X);
DEFPROP(c4_IntProp,     Y);


//View props.
DEFPROP(c4_ViewProp, Characters);
DEFPROP(c4_ViewProp, CompletedScripts);
DEFPROP(c4_ViewProp, Entrances);
DEFPROP(c4_ViewProp, EntrancesExplored);
DEFPROP(c4_ViewProp, Exits);
DEFPROP(c4_ViewProp, ExploredRooms);
DEFPROP(c4_ViewProp, LitFuses);
DEFPROP(c4_ViewProp, Monsters);
DEFPROP(c4_ViewProp, OrbAgents);
DEFPROP(c4_ViewProp, Orbs);
DEFPROP(c4_ViewProp, OrbTypes);
DEFPROP(c4_ViewProp, Scrolls);
DEFPROP(c4_ViewProp, Pieces);
DEFPROP(c4_ViewProp, PlatformDeltas);
DEFPROP(c4_ViewProp, Vars);

//This view has records for all ID keys.
DEFTDEF(INCREMENTEDIDS_VIEWDEF,
		"IncrementedIDs"
		"["
			"DataID:I,"
			"DemoID:I,"
			"HoldID:I,"
			"LevelID:I,"
			"MessageID:I,"
			"MessageTextID:I,"
			"PlayerID:I,"
			"RoomID:I,"
			"SavedGameID:I,"
			"SpeechID:I,"
			"HighScoreID:I"
		"]");

#define MONSTERS_VIEWPROPDEF  \
		"Monsters" \
		"[" \
			"Type:I," \
			"X:I," \
			"Y:I," \
			"O:I," \
			"ExtraVars:B," \
			"Pieces" \
			"[" \
				"Type:I," \
				"X:I," \
				"Y:I" \
			"]" \
		"]"

DEFTDEF(ROOMS_VIEWDEF,
		"Rooms"
		"["
			"RoomID:I,"
			"LevelID:I,"
			"RoomX:I,"
			"RoomY:I,"
			"RoomCols:I,"
			"RoomRows:I,"
			"IsSecret:I,"
			"DataID:I,"
			"ImageStartX:I,"
			"ImageStartY:I,"
			"OverheadDataID:I,"
			"OverheadImageStartX,"
			"OverheadImageStartY,"
			"StyleName:B,"
			"Squares:B,"
			"TileLights:B,"
			"Orbs"
			"["
				"Type:I,"
				"X:I,"
				"Y:I,"
				"OrbAgents"
				"["
					"Type:I,"
					"X:I,"
					"Y:I"
				"]"
			"],"
			MONSTERS_VIEWPROPDEF ","
			"Scrolls"
			"["
				"X:I,"
				"Y:I,"
				"MessageID:I"
			"],"
			"Exits"
			"["
				"EntranceID:I,"
				"Left:I,"
				"Right:I,"
				"Top:I,"
				"Bottom:I"
			"],"
			"ExtraVars:B"
		"]");

DEFTDEF(SAVEDGAMES_VIEWDEF,
		"SavedGames"
		"["
			"SavedGameID:I,"
			"PlayerID:I,"
			"RoomID:I,"
			"Type:I,"
			"IsHidden:I,"
			"LastUpdated:I,"
			"StartRoomX:I,"
			"StartRoomY:I,"
			"StartRoomO:I,"
			"StartRoomAppearance:I,"
			"StartRoomSwordOff:I,"
			"ExploredRooms"
			"["
				"RoomID:I,"
				"MapState:I,"
				"MapMarker:I,"

				//room delta:
				"Squares:B,"   //room tiles now
				MONSTERS_VIEWPROPDEF "," //current monsters in the room
				"LitFuses"     //burning fuses
				"["
					"X:I"      //stores (x,y) as 1-D index
				"],"
				"PlatformDeltas"
				"["
					"X:I,"
					"Y:I"
				"],"
				"Orbs"
				"["
					"X:I"
				"],"
				"TileLights:B"
			"],"
			"CompletedScripts"
			"["
				"ScriptID:I"
			"],"
			"EntrancesExplored"
			"["
				"EntranceID:I"
			"],"
			MONSTERS_VIEWPROPDEF "," //global scripts and custom equipment
			"Created:I,"
			"Commands:B,"
			"Stats:B,"
			"Version:I,"
			"ChecksumStr:B"
		"]");

DEFTDEF(DEMOS_VIEWDEF,
		"Demos"
		"["
			"DemoID:I,"
			"SavedGameID:I,"
			"IsHidden:I,"
			"DescriptionMessageID:I,"
			"ShowSequenceNo:I,"
			"BeginTurnNo:I,"
			"EndTurnNo:I,"
			"NextDemoID:I,"
			"Checksum:I,"
			"Flags:I"
		"]");

DEFTDEF(MESSAGETEXTS_VIEWDEF,
		"MessageTexts"
		"["
			"MessageTextID:I,"
			"MessageID:I,"
			"LanguageCode:I,"
			"MessageText:B"
		"]");

DEFTDEF(LEVELS_VIEWDEF,
		"Levels"
		"["
			"LevelID:I,"
			"HoldID:I,"
			"PlayerID:I,"
			"NameMessageID:I,"
			"Created:I,"
			"LastUpdated:I,"
			"GID_LevelIndex:I,"
			"OrderIndex:I,"
			"Multiplier:I"
//			"IsRequired:I"
		"]");

DEFTDEF(HOLDS_VIEWDEF,
		"Holds"
		"["
			"HoldID:I,"
			"NameMessageID:I,"
			"DescriptionMessageID:I,"
			"LevelID:I,"
			"GID_Created:I,"
			"LastUpdated:I,"
			"GID_PlayerID:I,"
			"GID_NewLevelIndex:I,"
			"EditingPrivileges:I,"
			"EndHoldMessageID:I,"
			"Entrances"
			"["
				"EntranceID:I,"
				"DescriptionMessageID:I,"
				"RoomID:I,"
				"X:I,"
				"Y:I,"
				"O:I,"
				"IsMainEntrance:I,"
				"ShowDescription:I"
			"],"
			"ScriptID:I,"
			"Status:I,"
			"VarID:I,"
			"Vars"
			"["
				"VarID:I,"
				"VarNameText:B"
			"],"
			"CharID:I,"
			"Characters"
			"["
				"CharID:I,"
				"CharNameText:B,"
				"Type:I,"
				"DataID:I,"
				"DataIDTiles:I,"
				"AnimationSpeed:I,"
				"ExtraVars:B"
			"],"
			"CaravelNetMedia:I"
		"]");

DEFTDEF(PLAYERS_VIEWDEF,
		"Players"
		"["
			"PlayerID:I,"
			"IsLocal:I,"
			"NameMessageID:I,"
			"CNetNameMessageID:I,"
			"CNetPasswordMessageID:I,"
			"GID_OriginalNameMessageID:I,"
			"GID_Created:I,"
			"LastUpdated:I,"
			"Settings:B"
		"]");

DEFTDEF(DATA_VIEWDEF,
		"Data"
		"["
			"DataID:I,"
			"DataFormat:I,"
			"HoldID:I,"
			"DataNameText:B,"
			"RawData:B,"
			"TimData:B,"
			"ModName:B"
		"]");

DEFTDEF(SPEECH_VIEWDEF,
		"Speech"
		"["
			"SpeechID:I,"
			"Character:I,"
			"Mood:I,"
			"DataID:I,"
			"MessageID:I,"
			"Delay:I"
		"]");

DEFTDEF(SAVEDGAMEMOVES_VIEWDEF,
		"SavedGameMoves"
		"["
			"SavedGameID:I," //owning record
			"Version:I,"     //version of stored command data
			"NumBytes:I,"    //# of bytes in unpacked command data
			"Commands:B"     //packed movement commands
		"]");

DEFTDEF(LOCALHIGHSCORES_VIEWDEF,
		"LocalHighScores"
		"["
				"HighScoreID:I,"
				"Score:I,"
				"PlayerID:I,"
				"HoldID:I,"
				"ScorepointName:B,"
				"Stats:B"
		"]");

#undef DEFPROP
#undef DEFTDEF

//*****************************************************************************
//Used for importing data into the DB.

//ATTENTION:
//Must keep the values below synched with those above.

//All view definitions in the DB.
enum VIEWTYPE
{
	V_First=0,
	V_Data=V_First,
	V_Demos,
	V_Holds,
	V_Levels,
	V_MessageTexts,
	V_Players,
	V_Rooms,
	V_SavedGameMoves,
	V_SavedGames,
	V_Speech,
	V_LocalHighScores,
	V_Count,
	V_Invalid
};

//All viewprops in the DB, that aren't just a simple list
enum VIEWPROPTYPE
{
	VP_First=0,
	VP_Characters=VP_First,
	VP_Entrances,
	VP_Exits,
	VP_ExploredRooms,
	VP_Monsters,
	VP_OrbAgents,
	VP_Orbs,
	VP_Scrolls,
	VP_Pieces,
	VP_PlatformDeltas,
	VP_Vars,
	VP_Count,
	VP_Invalid
};

//All fields in DB views,
// and viewprops that are simple lists
enum PROPTYPE
{
	P_First=0,
	P_AnimationSpeed=P_First,
	P_BeginTurnNo,
	P_Bottom,
	P_CaravelNetMedia,
	P_Character,
	P_CharID,
	P_CharNameText,
	P_Checksum,
	P_ChecksumStr,
	P_CNetName,
	P_CNetPassword,
	P_Commands,
	P_CompletedScripts,  //list
	P_Created,
	P_DataFormat,
	P_DataID,
	P_DataIDTiles,
	P_DataNameText,
	P_DataNameTextID,  //deprecated
	P_Delay,
	P_DemoID,
	P_DescriptionMessage,   //not ID
	P_EditingPrivileges,
	P_EMailMessage, //not ID
	P_EndHoldMessage,       //not ID
	P_EndTurnNo,
	P_EntranceID,
	P_EntrancesExplored,  //list
	P_ExtraVars,
	P_Flags,
	P_GID_Created,
	P_GID_LevelIndex,
	P_GID_NewLevelIndex,
	P_GID_OriginalNameMessage, //not ID
	P_GID_PlayerID,
	P_HighScoreID,
	P_HoldID,
	P_ImageStartX,
	P_ImageStartY,
//	P_IsFirstTurn,
	P_IsHidden,
	P_IsLocal,
	P_IsMainEntrance,
	P_IsSecret,
	P_LanguageCode,
	P_LastUpdated,
	P_Left,
	P_LevelID,
	P_LitFuses,  //list
	P_MapMarker,
	P_MapOnly,
	P_MapState,
	P_Message,     //not ID
	P_MessageText,
	P_MessageTextID,
	P_ModName,
	P_Mood,
	P_Multiplier,
	P_NameMessage, //not ID
	P_NextDemoID,
	P_NumBytes,
	P_O,
	P_OrbTypes,  //list
	P_OrderIndex,
	P_OverheadDataID,
	P_OverheadImageStartX,
	P_OverheadImageStartY,
	P_PlayerID,
//	P_ProcessSequence,
	P_RawData,
	P_Right,
	P_RoomID,
	P_RoomCols,
	P_RoomRows,
	P_RoomX,
	P_RoomY,
	P_SavedGameID,
	P_Score,
	P_ScorepointName,
	P_ScriptID,
	P_Settings,
	P_ShowDescription,
	P_ShowSequenceNo,
	P_SpeechID,
	P_Squares,
	P_StartRoomAppearance,
	P_StartRoomO,
	P_StartRoomSwordOff,
	P_StartRoomX,
	P_StartRoomY,
	P_Stats,
	P_Status,
	P_StyleName,
	P_TileLights,
	P_TimData,
	P_Top,
	P_Type,
	P_VarID,
	P_VarNameText,
	P_Version,
	P_X,
	P_Y,
	P_Count,
	P_Invalid,
	P_Start, //start of viewprop
	P_End    //end of viewprop
};

//*****************************************************************************
extern const char viewTypeStr[V_Count][16]
#ifdef INCLUDED_FROM_DBBASE_CPP
= {
	"Data", "Demos", "Holds", "Levels", "MessageTexts", "Players",
	"Rooms", "SavedGameMoves", "SavedGames", "Speech", "LocalHighScores"
}
#endif
;

extern const char viewpropTypeStr[VP_Count][15]
#ifdef INCLUDED_FROM_DBBASE_CPP
= {
	"Characters", "Entrances", "Exits", "ExploredRooms",
	"Monsters", "OrbAgents",
	"Orbs", "Scrolls", "Pieces", "PlatformDeltas", "Vars"
}
#endif
;

extern const char propTypeStr[P_Count][26]
#ifdef INCLUDED_FROM_DBBASE_CPP
= {
	"AnimationSpeed",
	"BeginTurnNo", "Bottom", "CaravelNetMedia", "Character", "CharID",
	"CharNameText", "Checksum", "ChecksumStr",
	"ForumName", "ForumPassword", //keep old names for backwards compatibility
	"Commands", "CompletedScripts",
	"Created", "DataFormat", "DataID", "DataIDTiles",
	"DataNameText",
	"DataNameTextID", "Delay", "DemoID", "DescriptionMessage",
	"EditingPrivileges", "EMailMessage", "EndHoldMessage", "EndTurnNo",
	"EntranceID", "EntrancesExplored", "ExtraVars", "Flags", "GID_Created",
	"GID_LevelIndex", "GID_NewLevelIndex", "GID_OriginalNameMessage",
	"GID_PlayerID", "HighScoreID", "HoldID", "ImageStartX", "ImageStartY",
//	"IsFirstTurn",
	"IsHidden", "IsLocal", "IsMainEntrance",
	"IsSecret",
	"LanguageCode", "LastUpdated", "Left",
	"LevelID",
	"LitFuses", "MapMarker", "MapOnly", "MapState",
	"Message", "MessageText", "MessageTextID", "ModName", "Mood", "Multiplier",
	"NameMessage", "NextDemoID", "NumBytes", "O", "OrbTypes", "OrderIndex",
	"OverheadDataID", "OverheadImageStartX", "OverheadImageStartY",
	"PlayerID",
//	"ProcessSequence",
	"RawData", "Right", "RoomID", "RoomCols",
	"RoomRows", "RoomX", "RoomY", "SavedGameID", "Score", "ScorepointName", "ScriptID",
	"Settings", "ShowDescription", "ShowSequenceNo", "SpeechID", "Squares",
	"StartRoomAppearance",
	"StartRoomO", "StartRoomSwordOff", "StartRoomX", "StartRoomY", "Stats",
	"Status", "StyleName",
	"TileLights", "TimData", "Top", "Type", "VarID", "VarNameText", "Version",
	"X", "Y"
}
#endif
;

static inline const char* ViewTypeStr(const VIEWTYPE vType)
//Returns: string corresponding to enumeration
{
	return viewTypeStr[vType];
}

static inline const char* ViewpropTypeStr(const VIEWPROPTYPE vpType)
//Returns: string corresponding to enumeration
{
	return viewpropTypeStr[vpType];
}

static inline const char* PropTypeStr(const PROPTYPE pType)
//Returns: string corresponding to enumeration
{
	return propTypeStr[pType];
}

#endif //...#ifndef DBPROPS_H
