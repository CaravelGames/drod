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

//FAQ: Adding a new game object:
//1.   Add T_value and its object layer in DRODLib/TileConstants.h
//2.   Add TI_value(s) for it in DROD/TileImageConstants.h
//3.   If it's a monster:
//       A. Add monster class to DRODLib project
//       B. Add to MonsterFactory.h enumeration and dynamic object creation in the .cpp
//4.   Add how feature is drawn in DROD/TileImageCalcs.*
//5.   Add tile sprites in Data/Bitmaps/*.png and TI_values in *.tim
//6.   Add game logic in DRODLib: to CurrentGame.cpp and other places it affects things
//7.   Add to editor menu in DROD/EditRoomScreen.cpp (read instructions there for how to do this)
//8.   Test out all logic paths

#ifndef TILECONSTANTS_H
#define TILECONSTANTS_H

#include "MonsterType.h"
#include "GameConstants.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Types.h>

//Tile constants--reordering existing constants may break macros.  Add new
//tile constants to end and make corresponding update to TILE_COUNT and
//TILE_LAYER.
#define T_EMPTY         0
#define T_FLOOR         1
#define T_PIT           2
#define T_STAIRS        3
#define T_WALL          4
#define T_WALL_B        5  //Broken
#define T_DOOR_C        6  //Cyan (Blue/Exit)
#define T_DOOR_M        7  //Monster (Green)
#define T_DOOR_R        8  //Red
#define T_DOOR_Y        9  //Yellow
#define T_DOOR_YO       10 //Yellow Open
#define T_TRAPDOOR      11 //trapdoor over pit
#define T_OBSTACLE      12
#define T_ARROW_N       13
#define T_ARROW_NE      14
#define T_ARROW_E       15
#define T_ARROW_SE      16
#define T_ARROW_S       17
#define T_ARROW_SW      18
#define T_ARROW_W       19
#define T_ARROW_NW      20
#define T_POTION_I      21 //Invisibility
#define T_POTION_K      22 //Mimic
#define T_SCROLL        23
#define T_ORB           24
#define T_SNK_EW        25 //deprecated -- used to represent pieces of all serpent types in v1.6
#define T_SNK_NS        26
#define T_SNK_NW        27
#define T_SNK_NE        28
#define T_SNK_SW        29
#define T_SNK_SE        30
#define T_SNKT_S        31
#define T_SNKT_W        32
#define T_SNKT_N        33
#define T_SNKT_E        34
#define T_TAR           35
#define T_CHECKPOINT    36 //deprecated; used in front end

//2.0/3.0 tiles
#define T_DOOR_B        37 //Black
#define T_POTION_SP     38 //Speed
#define T_BRIAR_SOURCE  39
#define T_BRIAR_DEAD    40
#define T_BRIAR_LIVE    41
#define T_LIGHT_CEILING 42 //ceiling light; used in front end
#define T_BOMB          43
#define T_FUSE          44
#define T_NODIAGONAL    45
#define T_TOKEN         46
#define T_TUNNEL_N      47
#define T_TUNNEL_S      48
#define T_MIRROR        49
#define T_POTION_C      50 //Clone
#define T_POTION_D      51 //Decoy
#define T_PLATFORM_W    52 //Platform (on water)
#define T_PLATFORM_P    53 //Platform (over pit)
#define T_FLOOR_M       54 //Floor mosaic
#define T_FLOOR_ROAD    55 //road texture
#define T_FLOOR_GRASS   56 //grass texture
#define T_FLOOR_DIRT    57 //dirt texture
#define T_FLOOR_ALT     58 //alternate texture
#define T_WALL_M        59 //master blocks
#define T_MUD           60 //mud (tarstuff)
#define T_STAIRS_UP     61 //upward stairs
#define T_WALL_H        62 //Hidden wall
#define T_TUNNEL_E      63
#define T_TUNNEL_W      64
#define T_FLOOR_IMAGE   65 //user-defined image
#define T_WALL2         66	//alternate solid wall
#define T_WATER         67
#define T_DOOR_GO       68 //Monster/Green Open
#define T_DOOR_CO       69 //Blue/Exit Open
#define T_DOOR_RO       70 //Red Open
#define T_DOOR_BO       71 //Black Open
#define T_TRAPDOOR2     72 //trapdoor over water
#define T_GOO           73 //oremites
#define T_LIGHT         74	//light source
#define T_HOT           75 //hot tile
#define T_GEL           76 //gel (tarstuff)
#define T_STATION       77 //relay station
#define T_PRESSPLATE    78 //pressure plate
#define T_BRIDGE        79 //bridge
#define T_BRIDGE_H      80 //bridge (horizontal)
#define T_BRIDGE_V      81 //bridge (vertical)
#define T_PIT_IMAGE     82 //user-defined image
#define T_WALL_IMAGE    83 //user-defined image
#define T_DARK_CEILING  84 //opposite of ceiling light; used in front end
#define T_WALLLIGHT     85 //light source (alternate)

//4.0 tiles
#define T_SHALLOW_WATER 86 //shallow water
#define T_HORN_SQUAD    87 //squad horn
#define T_HORN_SOLDIER  88 //soldier horn
#define T_STEP_STONE    89 //stepping stone

//5.0 tiles
#define T_BEACON        90 //seeding beacon
#define T_BEACON_OFF    91 //seeding beacon (deactivated)
#define T_POWDER_KEG    92
#define T_FLOOR_SPIKES  93
#define T_ARROW_OFF_N   94
#define T_ARROW_OFF_NE  95
#define T_ARROW_OFF_E   96
#define T_ARROW_OFF_SE  97
#define T_ARROW_OFF_S   98
#define T_ARROW_OFF_SW  99
#define T_ARROW_OFF_W   100
#define T_ARROW_OFF_NW  101
#define T_OVERHEAD_IMAGE 102 //used in front end
#define T_FLUFF         103
#define T_FLUFFVENT     104
#define T_THINICE       105
#define T_THINICE_SH    106
#define T_FIRETRAP      107
#define T_FIRETRAP_ON   108
#define T_WALL_WIN      109 //hold complete blocks

#define TILE_COUNT     (110) //Number of tile constants from above list.
static inline bool IsValidTileNo(const UINT t) {return t < TILE_COUNT;}

#define T_REMOVE_BUILD_MARKER (UINT(-1))
#define T_REMOVE_OVERHEAD_IMAGE (UINT(-2))
//Used to allow scripting to deal with Token types
#define T_ACTIVETOKEN (UINT(-3))
#define T_TOKEN_ROTATECW (UINT(-4))
#define T_TOKEN_ROTATECCW (UINT(-5))
#define T_TOKEN_SWITCH_TARMUD (UINT(-6))
#define T_TOKEN_SWITCH_TARGEL (UINT(-7))
#define T_TOKEN_SWITCH_GELMUD (UINT(-8))
#define T_TOKEN_VISION (UINT(-9))
#define T_TOKEN_POWER (UINT(-10))
#define T_TOKEN_DISARM (UINT(-11))
#define T_TOKEN_PERSISTENTMOVE (UINT(-12))
#define T_TOKEN_CONQUER (UINT(-13))
#define T_TOKEN_WPSWORD (UINT(-14))
#define T_TOKEN_WPPICKAXE (UINT(-15))
#define T_TOKEN_WPSPEAR (UINT(-16))
#define T_TOKEN_WPSTAFF (UINT(-17))
#define T_TOKEN_WPDAGGER (UINT(-18))
#define T_TOKEN_WPCABER (UINT(-19))
#define T_TOKEN_TSPLIT (UINT(-20))
#define T_TOKEN_TSPLIT_USED (UINT(-21))

// Reserved space for more tokens in the future to keep bIsFakeTokenType simple and have them together
#define T_TOKEN_RESERVED_SPACE (UINT(-50))
#define T_ORB_CRACKED (UINT(-51))
#define T_ORB_BROKEN (UINT(-52))
#define T_PLATE_ONEUSE (UINT(-53))
#define T_PLATE_MULTI (UINT(-54))
#define T_PLATE_ON_OFF (UINT(-55))
#define T_REMOVE_FLOOR_ITEM (UINT(-56))
#define T_ORB_NORMAL (UINT(-57))
#define LAST_FAKE_TILE_INDEX (UINT(-57))

static inline bool bIsFakeTokenType(const UINT t) { return t >= T_TOKEN_RESERVED_SPACE && t <= T_ACTIVETOKEN; }
static inline bool bIsFakeOrbType(const UINT t) { return t == T_ORB_CRACKED || t == T_ORB_BROKEN || t == T_ORB_NORMAL; }
static inline bool bIsFakePressurePlateType(const UINT t) { return t <= T_PLATE_ONEUSE && t >= T_PLATE_ON_OFF; }
static inline bool bIsFakeElementType(const UINT t) { return t <= T_ACTIVETOKEN && t >= LAST_FAKE_TILE_INDEX; }

static inline RoomTokenType ConvertFakeTokenType(const UINT t) {
	switch (t) {
		case T_TOKEN_ROTATECW:
			return RotateArrowsCW;
		case T_TOKEN_ROTATECCW:
			return RotateArrowsCCW;
		case T_TOKEN_SWITCH_TARMUD:
			return SwitchTarMud;
		case T_TOKEN_SWITCH_TARGEL:
			return SwitchTarGel;
		case T_TOKEN_SWITCH_GELMUD:
			return SwitchGelMud;
		case T_TOKEN_VISION:
			return TarTranslucent;
		case T_TOKEN_POWER:
			return PowerTarget;
		case T_TOKEN_DISARM:
			return WeaponDisarm;
		case T_TOKEN_PERSISTENTMOVE:
			return PersistentCitizenMovement;
		case T_TOKEN_CONQUER:
			return ConquerToken;
		case T_TOKEN_WPSWORD:
			return SwordToken;
		case T_TOKEN_WPPICKAXE:
			return PickaxeToken;
		case T_TOKEN_WPSPEAR:
			return SpearToken;
		case T_TOKEN_WPSTAFF:
			return StaffToken;
		case T_TOKEN_WPDAGGER:
			return DaggerToken;
		case T_TOKEN_WPCABER:
			return CaberToken;
		case T_TOKEN_TSPLIT:
			return TemporalSplit;
		case T_TOKEN_TSPLIT_USED:
			return TemporalSplitUsed;
		default:
			return RotateArrowsCW;
	}
}

//
//Tile-related macros (inlined functions).
// Used for combining specific tile types into types with common interaction behavior.
//

static inline bool bIsBridge(const UINT t) {return t==T_BRIDGE || t==T_BRIDGE_H || t==T_BRIDGE_V;}

static inline bool bIsCustomImageTile(const UINT t) {return t==T_FLOOR_IMAGE || t== T_PIT_IMAGE || t==T_WALL_IMAGE;}

static inline bool bIsEmptyTile(const UINT t) {return t==T_EMPTY;}
static inline bool bIsTrapdoor(const UINT t) {return t==T_TRAPDOOR || t==T_TRAPDOOR2;}
static inline bool bIsThinIce(const UINT t) {return t==T_THINICE || t==T_THINICE_SH;}
static inline bool bIsFallingTile(const UINT t) {return bIsTrapdoor(t) || bIsThinIce(t);}
static inline bool bIsSteppingStone (const UINT t) {return t==T_STEP_STONE;}

static inline bool bIsPlainFloor(const UINT t) {return t==T_FLOOR || (t>=T_FLOOR_M && t<=T_FLOOR_ALT) || t==T_FLOOR_IMAGE;}

static inline bool bIsFloor(const UINT t) {
	switch (t) {
		case T_HOT:
		case T_GOO:
		case T_PRESSPLATE:
		case T_FLOOR_SPIKES:
		case T_FLUFFVENT:
		case T_FIRETRAP: case T_FIRETRAP_ON:
			return true;
		default:
			return bIsPlainFloor(t) || bIsFallingTile(t) || bIsBridge(t) || bIsSteppingStone(t);
	}
}

static inline bool bIsLight(const UINT t) {return t==T_LIGHT;}
static inline bool bIsLighting(const UINT t) { return t == T_WALLLIGHT || t == T_LIGHT_CEILING || t == T_DARK_CEILING;  }

static inline bool bIsWall(const UINT t) {
	switch (t) {
		case T_WALL: case T_WALL2:
		case T_WALL_M: case T_WALL_WIN:
		case T_WALL_IMAGE:
			return true;
		default:
			return false;
	}
}

static inline bool bIsCrumblyWall(const UINT t) {return t==T_WALL_B || t==T_WALL_H;}

static inline bool bIsStairs(const UINT t) {return t==T_STAIRS || t==T_STAIRS_UP;}

static inline bool bIsPit(const UINT t) {return t==T_PIT || t==T_PIT_IMAGE;}

static inline bool bIsDeepWater (const UINT t) {return t==T_WATER;}
static inline bool bIsShallowWater (const UINT t) {return t==T_SHALLOW_WATER;}
static inline bool bIsWater(const UINT t) {return bIsDeepWater(t) || bIsShallowWater(t);}

static inline bool bIsArrow(const UINT t) {return t>=T_ARROW_N && t<=T_ARROW_NW;}
static inline bool bIsDisabledArrow(const UINT t) {return t>=T_ARROW_OFF_N && t<=T_ARROW_OFF_NW;}
static inline bool bIsAnyForceArrow(const UINT t) { return bIsArrow(t) || bIsDisabledArrow(t); }
static inline bool bIsAnyArrow(const UINT t) {return bIsArrow(t) || bIsDisabledArrow(t);}
static inline UINT getToggledForceArrow(const UINT t) {
	switch (t) {
		case T_ARROW_NE: return T_ARROW_OFF_NE;
		case T_ARROW_E:  return T_ARROW_OFF_E;
		case T_ARROW_SE: return T_ARROW_OFF_SE;
		case T_ARROW_S:  return T_ARROW_OFF_S;
		case T_ARROW_SW: return T_ARROW_OFF_SW;
		case T_ARROW_W:  return T_ARROW_OFF_W;
		case T_ARROW_NW: return T_ARROW_OFF_NW;
		case T_ARROW_N:  return T_ARROW_OFF_N;

		case T_ARROW_OFF_NE: return T_ARROW_NE;
		case T_ARROW_OFF_E:  return T_ARROW_E;
		case T_ARROW_OFF_SE: return T_ARROW_SE;
		case T_ARROW_OFF_S:  return T_ARROW_S;
		case T_ARROW_OFF_SW: return T_ARROW_SW;
		case T_ARROW_OFF_W:  return T_ARROW_W;
		case T_ARROW_OFF_NW: return T_ARROW_NW;
		case T_ARROW_OFF_N:  return T_ARROW_N;

		default: return T_EMPTY;
	}
}

static inline bool bIsPotion(const UINT t) {
	switch (t)
	{
		case T_POTION_I: case T_POTION_K: case T_POTION_SP:
		case T_POTION_C: case T_POTION_D:
		case T_HORN_SQUAD: case T_HORN_SOLDIER:
			return true;
		default: return false;
	}
}

static inline bool bIsSerpentTile(const UINT t) {return t>=T_SNK_EW && t<=T_SNKT_E;}

static inline bool bIsBriar(const UINT t) {return t>=T_BRIAR_SOURCE && t<=T_BRIAR_LIVE;}

static inline bool bIsPlatform(const UINT t) {return t==T_PLATFORM_W || t==T_PLATFORM_P;}

static inline bool bIsDoor(const UINT t) {return (t>=T_DOOR_C && t<=T_DOOR_Y) ||	t==T_DOOR_B;}   //closed doors

static inline bool bIsOpenDoor(const UINT t) {return t==T_DOOR_YO ||	(t>=T_DOOR_GO && t<=T_DOOR_BO);}

static inline bool bIsYellowDoor(const UINT t) {return t==T_DOOR_Y || t==T_DOOR_YO;}
static inline bool bIsGreenDoor(const UINT t) {return t==T_DOOR_M || t==T_DOOR_GO;}
static inline bool bIsBlueDoor(const UINT t) {return t==T_DOOR_C || t==T_DOOR_CO;}
static inline bool bIsRedDoor(const UINT t) {return t==T_DOOR_R || t==T_DOOR_RO;}
static inline bool bIsBlackDoor(const UINT t) {return t==T_DOOR_B || t==T_DOOR_BO;}

static inline bool bIsBeacon(const UINT t) {return t==T_BEACON || t==T_BEACON_OFF;}

static inline bool bIsTar(const UINT t) {return t==T_TAR || t==T_MUD || t==T_GEL;}
static inline bool bIsTarOrFluff(const UINT t) {return t==T_TAR || t==T_MUD || t==T_GEL || t==T_FLUFF;}

static inline bool bIsTunnel(const UINT t) {return t==T_TUNNEL_N || t==T_TUNNEL_S || t==T_TUNNEL_E || t==T_TUNNEL_W;}

static inline bool bIsTLayerCoveringItem(const UINT t) { return t == T_MIRROR || t == T_POWDER_KEG; }
static inline bool bIsTLayerCoverableItem(const UINT t) {
	switch (t) {
		case T_EMPTY:
		case T_FUSE:
		case T_SCROLL:
		case T_TOKEN:
			return true;
		default: return false;
	}
}
static inline bool bIsTLayerObstacle(const UINT t) { return bIsTarOrFluff(t) || t==T_OBSTACLE || t==T_BOMB || t==T_STATION || t==T_LIGHT; }

static inline bool bIsFuseConnected(const UINT t) { return t==T_FUSE || t==T_BOMB; }
static inline bool bIsCombustibleItem(const UINT t) { return t==T_FUSE || t==T_BOMB || t==T_POWDER_KEG; }
static inline bool bIsExplodingItem(const UINT t) { return t==T_BOMB || t==T_POWDER_KEG; }

static inline bool bIsFiretrap(const UINT t) { return t==T_FIRETRAP || t==T_FIRETRAP_ON; }
static inline UINT getToggledFiretrap(const UINT t) { return t==T_FIRETRAP ? T_FIRETRAP_ON : T_FIRETRAP; }

static inline bool bIsSolidOTile(const UINT t) { return bIsWall(t) || bIsCrumblyWall(t) || bIsDoor(t); }
static inline bool bCanFluffGrowOn(const UINT t) { return t == T_EMPTY || bIsBriar(t) ||
					(bIsFloor(t) && t != T_HOT) || bIsOpenDoor(t) || bIsPlatform(t) || bIsPit(t) || bIsWater(t) || bIsDisabledArrow(t); }
static inline bool bCanPuffsMergeOn(const UINT t) { return t == T_EMPTY || bIsBriar(t) ||
					(bIsFloor(t) && t != T_HOT) || bIsOpenDoor(t) || bIsPlatform(t) || bIsPit(t) || bIsWater(t) || bIsAnyArrow(t); }

//Obstacle parameter bit format: <Top edge>:1 <Left edge>:1 <64 possible obstacle types>:6
#define OBSTACLE_TOP (0x80)
#define OBSTACLE_LEFT (0x40)
static inline bool bObstacleTop(const BYTE obType) {return (obType & OBSTACLE_TOP) != 0;}
static inline bool bObstacleLeft(const BYTE obType) {return (obType & OBSTACLE_LEFT) != 0;}
static inline BYTE calcObstacleType(const BYTE obType) {return obType & (OBSTACLE_LEFT-1);}


//Token parameter bit format: <on/off>:1 <types>:128
#define TOKEN_ACTIVE (0x80)     //on/off
static inline bool bTokenActive(const BYTE val) {return (val & TOKEN_ACTIVE) != 0;}
static inline BYTE calcTokenType(const BYTE val) {return val & (TOKEN_ACTIVE-1);}

//Light parameter bit format: <wall light>:1 <on/off>:1 <radius>:3 <type>:4
#define WALL_LIGHT   (0x100)    //wall light offset
#define LIGHT_OFF    (0x80)     //on/off
#define NUM_LIGHT_TYPES	(0x10)  //16 types
#define MAX_LIGHT_DISTANCE (8)  //up to 8 radius
#define NUM_DARK_TYPES	(0x20)  //32 types
static inline bool bIsLightTileValue(const UINT val) {return val > 0 && val <= LIGHT_OFF;} //offset by 1
static inline bool bIsDarkTileValue(const UINT val) {return val > LIGHT_OFF && val < WALL_LIGHT;} //<128 types
static inline bool bIsWallLightValue(const UINT val) {return (val & WALL_LIGHT) != 0;}
static inline bool bLightOff(const UINT val) {return (val & LIGHT_OFF) != 0;}
static inline BYTE calcLightRadius(const UINT val) {return (val & ((MAX_LIGHT_DISTANCE-1)*NUM_LIGHT_TYPES))/NUM_LIGHT_TYPES;}
static inline BYTE calcLightType(const UINT val) {return val & (NUM_LIGHT_TYPES-1);}


static inline bool bIsValidBuildTile(const UINT wTileNo)
{
	switch (wTileNo)
	{
	case T_OBSTACLE:
	case T_PRESSPLATE:
	case T_SNKT_E:
	case T_SNKT_N:
	case T_SNKT_S:
	case T_SNKT_W:
	case T_SNK_EW:
	case T_SNK_NE:
	case T_SNK_NS:
	case T_SNK_NW:
	case T_SNK_SE:
	case T_SNK_SW:
	case T_CHECKPOINT:
	case T_LIGHT_CEILING:
	case T_PLATFORM_W:
	case T_PLATFORM_P:
	case T_WALL_M:
	case T_STAIRS_UP:
	case T_STAIRS:
	case T_DARK_CEILING:
	case T_WALLLIGHT:
	case T_WALL_WIN:
	case T_ACTIVETOKEN:
	case T_TOKEN_TSPLIT_USED:
	case T_TOKEN:
	case T_PLATE_ONEUSE:
	case T_PLATE_MULTI:
	case T_PLATE_ON_OFF:
		return false;
	default:
		return true;
	}
}

static inline UINT getBaseTile(const UINT wVirtualTile)
{
	switch (wVirtualTile)
	{
	case T_ACTIVETOKEN:
	case T_TOKEN_ROTATECW:
	case T_TOKEN_ROTATECCW:
	case T_TOKEN_SWITCH_TARMUD:
	case T_TOKEN_SWITCH_TARGEL:
	case T_TOKEN_SWITCH_GELMUD:
	case T_TOKEN_VISION:
	case T_TOKEN_POWER:
	case T_TOKEN_DISARM:
	case T_TOKEN_PERSISTENTMOVE:
	case T_TOKEN_CONQUER:
	case T_TOKEN_WPSWORD:
	case T_TOKEN_WPPICKAXE:
	case T_TOKEN_WPSPEAR:
	case T_TOKEN_WPSTAFF:
	case T_TOKEN_WPDAGGER:
	case T_TOKEN_WPCABER:
	case T_TOKEN_TSPLIT:
	case T_TOKEN_TSPLIT_USED:
	case T_TOKEN_RESERVED_SPACE:
		return T_TOKEN;
	case T_ORB_CRACKED:
	case T_ORB_BROKEN:
		return T_ORB;
	case T_PLATE_ONEUSE:
	case T_PLATE_MULTI:
	case T_PLATE_ON_OFF:
		return T_PRESSPLATE;
	default:
		return wVirtualTile;
	}
}

//Add to monster values so they don't overlap o- and t-layer values on a tile.
//NOTE: Make sure list matches that in MonsterFactory.h.
static const UINT M_OFFSET = TILE_COUNT;
#define T_ROACH      (M_ROACH+M_OFFSET)
#define T_QROACH     (M_QROACH+M_OFFSET)
#define T_REGG       (M_REGG+M_OFFSET)
#define T_GOBLIN     (M_GOBLIN+M_OFFSET)
#define T_NEATHER    (M_NEATHER+M_OFFSET)
#define T_WWING      (M_WWING+M_OFFSET)
#define T_EYE        (M_EYE+M_OFFSET)
#define T_SERPENT    (M_SERPENT+M_OFFSET)
#define T_TARMOTHER  (M_TARMOTHER+M_OFFSET)
#define T_TARBABY    (M_TARBABY+M_OFFSET)
#define T_BRAIN      (M_BRAIN+M_OFFSET)
#define T_MIMIC      (M_MIMIC+M_OFFSET)
#define T_SPIDER     (M_SPIDER+M_OFFSET)
#define T_SERPENTG   (M_SERPENTG+M_OFFSET)
#define T_SERPENTB   (M_SERPENTB+M_OFFSET)
#define T_ROCKGOLEM  (M_ROCKGOLEM+M_OFFSET)
#define T_WATERSKIPPER (M_WATERSKIPPER+M_OFFSET)
#define T_SKIPPERNEST  (M_SKIPPERNEST+M_OFFSET)
#define T_AUMTLICH   (M_AUMTLICH+M_OFFSET)
#define T_CLONE      (M_CLONE+M_OFFSET)
#define T_DECOY      (M_DECOY+M_OFFSET)
#define T_WUBBA      (M_WUBBA+M_OFFSET)
#define T_SEEP       (M_SEEP+M_OFFSET)
#define T_STALWART   (M_STALWART+M_OFFSET)
#define T_HALPH      (M_HALPH+M_OFFSET)
#define T_SLAYER     (M_SLAYER+M_OFFSET)
#define T_FEGUNDO    (M_FEGUNDO+M_OFFSET)
#define T_FEGUNDOASHES (M_FEGUNDOASHES+M_OFFSET)
#define T_GUARD      (M_GUARD+M_OFFSET)
#define T_CHARACTER  (M_CHARACTER+M_OFFSET)
#define T_MUDMOTHER  (M_MUDMOTHER+M_OFFSET)
#define T_MUDBABY    (M_MUDBABY+M_OFFSET)
#define T_GELMOTHER  (M_GELMOTHER+M_OFFSET)
#define T_GELBABY    (M_GELBABY+M_OFFSET)
#define T_CITIZEN    (M_CITIZEN+M_OFFSET)
#define T_ROCKGIANT  (M_ROCKGIANT+M_OFFSET)
#define T_HALPH2     (M_HALPH2+M_OFFSET)
#define T_SLAYER2    (M_SLAYER2+M_OFFSET)
#define T_STALWART2  (M_STALWART2+M_OFFSET)
#define T_ARCHITECT  (M_ARCHITECT+M_OFFSET)
#define T_CONSTRUCT  (M_CONSTRUCT+M_OFFSET)
#define T_GENTRYII   (M_GENTRYII+M_OFFSET)
#define T_TEMPORALCLONE (M_TEMPORALCLONE+M_OFFSET)
#define T_FLUFFBABY  (M_FLUFFBABY+M_OFFSET)
#define MONSTER_COUNT      (44)  //Number of monsters in above list.

#define TOTAL_TILE_COUNT      (TILE_COUNT+MONSTER_COUNT) //Number of all tile constants.
static inline bool IsMonsterTileNo(const UINT t) {return t>=TILE_COUNT && t<TOTAL_TILE_COUNT;}

//Virtual tiles: enumerate after TOTAL_TILE_COUNT
#define T_SWORDSMAN           (TOTAL_TILE_COUNT + 0)  //for placing the level entrance
#define T_NOMONSTER           (TOTAL_TILE_COUNT + 1)  //for erasing monsters only
#define T_EMPTY_F             (TOTAL_TILE_COUNT + 2)  //for erasing f-layer objects only

#define TOTAL_EDIT_TILE_COUNT (TOTAL_TILE_COUNT + 3)

enum TILELAYERS {
	LAYER_OPAQUE = 0,
	LAYER_TRANSPARENT = 1,
	LAYER_MONSTER = 2,
	LAYER_FLOOR = 3
};

//Layer associated with each tile--0 is opaque layer, 1 is transparent layer,
//    2 is monster layer, and 3 is the floor layer.
static const UINT TILE_LAYER[TOTAL_EDIT_TILE_COUNT] =
{
	LAYER_TRANSPARENT, //T_EMPTY         0
	LAYER_OPAQUE, //T_FLOOR         1
	LAYER_OPAQUE, //T_PIT           2
	LAYER_OPAQUE, //T_STAIRS        3
	LAYER_OPAQUE, //T_WALL          4
	LAYER_OPAQUE, //T_WALL_B        5
	LAYER_OPAQUE, //T_DOOR_C        6
	LAYER_OPAQUE, //T_DOOR_M        7
	LAYER_OPAQUE, //T_DOOR_R        8
	LAYER_OPAQUE, //T_DOOR_Y        9
	LAYER_OPAQUE, //T_DOOR_YO       10
	LAYER_OPAQUE, //T_TRAPDOOR      11
	LAYER_TRANSPARENT, //T_OBSTACLE      12 //was in layer 0 before v2.0
	LAYER_FLOOR, //T_ARROW_N       13 //arrows were in layer 1 before v3.0
	LAYER_FLOOR, //T_ARROW_NE      14
	LAYER_FLOOR, //T_ARROW_E       15
	LAYER_FLOOR, //T_ARROW_SE      16
	LAYER_FLOOR, //T_ARROW_S       17
	LAYER_FLOOR, //T_ARROW_SW      18
	LAYER_FLOOR, //T_ARROW_W       19
	LAYER_FLOOR, //T_ARROW_NW      20
	LAYER_TRANSPARENT, //T_POTION_I      21
	LAYER_TRANSPARENT, //T_POTION_K      22
	LAYER_TRANSPARENT, //T_SCROLL        23
	LAYER_TRANSPARENT, //T_ORB           24
	LAYER_MONSTER, //T_SNK_EW        25
	LAYER_MONSTER, //T_SNK_NS        26
	LAYER_MONSTER, //T_SNK_NW        27
	LAYER_MONSTER, //T_SNK_NE        28
	LAYER_MONSTER, //T_SNK_SW        29
	LAYER_MONSTER, //T_SNK_SE        30
	LAYER_MONSTER, //T_SNKT_S        31
	LAYER_MONSTER, //T_SNKT_W        32
	LAYER_MONSTER, //T_SNKT_N        33
	LAYER_MONSTER, //T_SNKT_E        34
	LAYER_TRANSPARENT, //T_TAR           35
	LAYER_OPAQUE, //T_CHECKPOINT    36

	LAYER_OPAQUE, //T_DOOR_B        37
	LAYER_TRANSPARENT, //T_POTION_SP     38
	LAYER_TRANSPARENT, //T_BRIAR_SOURCE  39
	LAYER_TRANSPARENT, //T_BRIAR_DEAD    40
	LAYER_TRANSPARENT, //T_BRIAR_LIVE    41
	LAYER_FLOOR, //T_LIGHT_CEILING 42 //front end only -- show on f-layer menu
	LAYER_TRANSPARENT, //T_BOMB          43
	LAYER_TRANSPARENT, //T_FUSE          44
	LAYER_FLOOR, //T_NODIAGONAL    45 //was in layer 1 before v3.0
	LAYER_TRANSPARENT, //T_TOKEN         46
	LAYER_OPAQUE, //T_TUNNEL_N      47
	LAYER_OPAQUE, //T_TUNNEL_S      48
	LAYER_TRANSPARENT, //T_MIRROR        49
	LAYER_TRANSPARENT, //T_POTION_C      50
	LAYER_TRANSPARENT, //T_POTION_D      51
	LAYER_OPAQUE, //T_PLATFORM_W    52
	LAYER_OPAQUE, //T_PLATFORM_P    53
	LAYER_OPAQUE, //T_FLOOR_M       54
	LAYER_OPAQUE, //T_FLOOR_ROAD    55
	LAYER_OPAQUE, //T_FLOOR_GRASS   56
	LAYER_OPAQUE, //T_FLOOR_DIRT    57
	LAYER_OPAQUE, //T_FLOOR_ALT     58
	LAYER_OPAQUE, //T_WALL_M        59
	LAYER_TRANSPARENT, //T_MUD           60
	LAYER_OPAQUE, //T_STAIRS_UP     61
	LAYER_OPAQUE, //T_WALL_H        62
	LAYER_OPAQUE, //T_TUNNEL_E      63
	LAYER_OPAQUE, //T_TUNNEL_W      64
	LAYER_OPAQUE, //T_FLOOR_IMAGE   65
	LAYER_OPAQUE,	//T_WALL2         66
	LAYER_OPAQUE,	//T_WATER         67
	LAYER_OPAQUE, //T_DOOR_GO       68
	LAYER_OPAQUE, //T_DOOR_CO       69
	LAYER_OPAQUE, //T_DOOR_RO       70
	LAYER_OPAQUE, //T_DOOR_BO       71
	LAYER_OPAQUE, //T_TRAPDOOR2     72
	LAYER_OPAQUE, //T_GOO           73
	LAYER_TRANSPARENT, //T_LIGHT         74
	LAYER_OPAQUE, //T_HOT           75
	LAYER_TRANSPARENT, //T_GEL           76
	LAYER_TRANSPARENT, //T_STATION       77
	LAYER_OPAQUE, //T_PRESSPLATE    78
	LAYER_OPAQUE, //T_BRIDGE        79
	LAYER_OPAQUE, //T_BRIDGE_H      80
	LAYER_OPAQUE, //T_BRIDGE_V      81
	LAYER_OPAQUE, //T_PIT_IMAGE     82
	LAYER_OPAQUE, //T_WALL_IMAGE    83
	LAYER_FLOOR, //T_DARK_CEILING  84 //front end only -- show on f-layer menu
	LAYER_FLOOR, //T_WALLLIGHT     85 //front end only -- show on f-layer menu
	LAYER_OPAQUE, //T_SHALLOW_WATER 86
	LAYER_TRANSPARENT, //T_HORN_SQUAD    87
	LAYER_TRANSPARENT, //T_HORN_SOLIDER  88
	LAYER_OPAQUE, //T_STEP_STONE    89
	LAYER_TRANSPARENT, //T_BEACON        90
	LAYER_TRANSPARENT, //T_BEACON_OFF    91
	LAYER_TRANSPARENT, //T_POWDER_KEG    92
	LAYER_OPAQUE, //T_FLOOR_SPIKES  93
	LAYER_FLOOR, //T_ARROW_OFF_N   94
	LAYER_FLOOR, //T_ARROW_OFF_NE  95
	LAYER_FLOOR, //T_ARROW_OFF_E   96
	LAYER_FLOOR, //T_ARROW_OFF_SE  97
	LAYER_FLOOR, //T_ARROW_OFF_S   98
	LAYER_FLOOR, //T_ARROW_OFF_SW  99
	LAYER_FLOOR, //T_ARROW_OFF_W   100
	LAYER_FLOOR, //T_ARROW_OFF_NW  101
	LAYER_OPAQUE, //T_OVERHEAD_IMAGE 102
	LAYER_TRANSPARENT, //T_FLUFF         103
	LAYER_OPAQUE, //T_FLUFFVENT     104
	LAYER_OPAQUE, //T_THINICE       105
	LAYER_OPAQUE, //T_THINICE_SH    106
	LAYER_OPAQUE, //T_FIRETRAP      107
	LAYER_OPAQUE, //T_FIRETRAP_ON   108
	LAYER_OPAQUE, //T_WALL_WIN      109

	LAYER_MONSTER, //M_ROACH         +0
	LAYER_MONSTER, //M_QROACH        +1
	LAYER_MONSTER, //M_REGG          +2
	LAYER_MONSTER, //M_GOBLIN        +3
	LAYER_MONSTER, //M_NEATHER       +4
	LAYER_MONSTER, //M_WWING         +5
	LAYER_MONSTER, //M_EYE           +6
	LAYER_MONSTER, //M_SERPENT       +7
	LAYER_MONSTER, //M_TARMOTHER     +8
	LAYER_MONSTER, //M_TARBABY       +9
	LAYER_MONSTER, //M_BRAIN         +10
	LAYER_MONSTER, //M_MIMIC         +11
	LAYER_MONSTER, //M_SPIDER        +12
	LAYER_MONSTER, //M_SERPENTG      +13
	LAYER_MONSTER, //M_SERPENTB      +14
	LAYER_MONSTER, //M_ROCKGOLEM     +15
	LAYER_MONSTER, //M_WATERSKIPPER  +16
	LAYER_MONSTER, //M_SKIPPERNEST   +17
	LAYER_MONSTER, //M_AUMTLICH      +18
	LAYER_MONSTER, //M_CLONE         +19
	LAYER_MONSTER, //M_DECOY         +20
	LAYER_MONSTER, //M_WUBBA         +21
	LAYER_MONSTER, //M_SEEP          +22
	LAYER_MONSTER, //M_STALWART      +23
	LAYER_MONSTER, //M_HALPH         +24
	LAYER_MONSTER, //M_SLAYER        +25
	LAYER_MONSTER, //M_FEGUNDO       +26
	LAYER_MONSTER, //M_FEGUNDOASHES  +27
	LAYER_MONSTER, //M_GUARD         +28
	LAYER_MONSTER, //M_CHARACTER     +29
	LAYER_MONSTER, //M_MUDMOTHER     +30
	LAYER_MONSTER, //M_MUDBABY       +31
	LAYER_MONSTER, //M_GELMOTHER     +32
	LAYER_MONSTER, //M_GELBABY       +33
	LAYER_MONSTER, //M_CITIZEN       +34
	LAYER_MONSTER, //M_ROCKGIANT     +35
	LAYER_MONSTER, //M_HALPH2        +36
	LAYER_MONSTER, //M_SLAYER2       +37
	LAYER_MONSTER, //M_STALWART2     +38
	LAYER_MONSTER, //M_ARCHITECT     +39
	LAYER_MONSTER, //M_CONSTRUCT     +40
	LAYER_MONSTER, //M_GENTRYII      +41
	LAYER_MONSTER, //M_TEMPORALCLONE +42
	LAYER_MONSTER, //M_FLUFFBABY     +43

	LAYER_MONSTER, //T_SWORDSMAN     TOTAL+0
	LAYER_MONSTER, //T_NOMONSTER     TOTAL+1
	LAYER_FLOOR  //T_EMPTY_F       TOTAL+2
};

static const UINT TILE_MID[TOTAL_EDIT_TILE_COUNT] =
{
	0,                //T_EMPTY         0
	MID_Floor,        //T_FLOOR         1
	MID_Pit,          //T_PIT           2
	MID_Stairs,       //T_STAIRS        3
	MID_Wall,         //T_WALL          4
	MID_BrokenWall,   //T_WALL_B        5
	MID_BlueDoor,     //T_DOOR_C        6
	MID_GreenDoor,    //T_DOOR_M        7
	MID_RedDoor,      //T_DOOR_R        8
	MID_YellowDoor,   //T_DOOR_Y        9
	MID_OpenYellowDoor,//T_DOOR_YO       10
	MID_Trapdoor,     //T_TRAPDOOR      11
	MID_Obstacle,     //T_OBSTACLE      12
	MID_ForceArrow,   //T_ARROW_N       13
	MID_ForceArrow,   //T_ARROW_NE      14
	MID_ForceArrow,   //T_ARROW_E       15
	MID_ForceArrow,   //T_ARROW_SE      16
	MID_ForceArrow,   //T_ARROW_S       17
	MID_ForceArrow,   //T_ARROW_SW      18
	MID_ForceArrow,   //T_ARROW_W       19
	MID_ForceArrow,   //T_ARROW_NW      20
	MID_InvisPotion,  //T_POTION_I      21
	MID_MimicPotion,  //T_POTION_K      22
	MID_Scroll,       //T_SCROLL        23
	MID_Orb,          //T_ORB           24
	MID_Serpent,      //T_SNK_EW        25
	MID_Serpent,      //T_SNK_NS        26
	MID_Serpent,      //T_SNK_NW        27
	MID_Serpent,      //T_SNK_NE        28
	MID_Serpent,      //T_SNK_SW        29
	MID_Serpent,      //T_SNK_SE        30
	MID_Serpent,      //T_SNKT_S        31
	MID_Serpent,      //T_SNKT_W        32
	MID_Serpent,      //T_SNKT_N        33
	MID_Serpent,      //T_SNKT_E        34
	MID_Tar,          //T_TAR           35
	MID_Checkpoint,   //T_CHECKPOINT    36
	MID_BlackDoor,    //T_DOOR_B        37
	MID_SpeedPotion,  //T_POTION_SP     38
	MID_FlowSource,   //T_BRIAR_SOURCE  39
	MID_BriarWithered,//T_BRIAR_DEAD    40
	MID_FlowInner,    //T_BRIAR_LIVE    41
	MID_LightCeiling, //T_LIGHT_CEILING 42
	MID_Bomb,         //T_BOMB          43
	MID_Fuse,         //T_FUSE          44
	MID_Ortho,        //T_NODIAGONAL    45
	MID_Token,        //T_TOKEN         46
	MID_Tunnel_N,     //T_TUNNEL_N      47
	MID_Tunnel_S,     //T_TUNNEL_S      48
	MID_Mirror,       //T_MIRROR        49
	MID_ClonePotion,  //T_POTION_C      50
	MID_DecoyPotion,  //T_POTION_D      51
	MID_PlatformWater,//T_PLATFORM_W    52
	MID_PlatformPit,  //T_PLATFORM_P    53
	MID_FloorMosaic,  //T_FLOOR_M       54
	MID_FloorRoad,    //T_FLOOR_ROAD    55
	MID_FloorGrass,   //T_FLOOR_GRASS   56
	MID_FloorDirt,    //T_FLOOR_DIRT    57
	MID_FloorAlt,     //T_FLOOR_ALT     58
	MID_MasterWall,   //T_WALL_M        59
	MID_Mud,          //T_MUD           60
	MID_StairsUp,     //T_STAIRS_UP     61
	MID_SecretWall,   //T_WALL_H        62
	MID_Tunnel_E,     //T_TUNNEL_E      63
	MID_Tunnel_W,     //T_TUNNEL_W      64
	MID_FloorImage,   //T_FLOOR_IMAGE   65
	MID_Wall2,        //T_WALL2         66
	MID_Water,        //T_WATER         67
	MID_OpenGreenDoor,//T_DOOR_GO       68
	MID_OpenBlueDoor, //T_DOOR_CO       69
	MID_OpenRedDoor,  //T_DOOR_RO       70
	MID_OpenBlackDoor,//T_DOOR_BO       71
	MID_Trapdoor2,    //T_TRAPDOOR2     72
	MID_Goo,          //T_GOO           73
	MID_Light,        //T_LIGHT         74
	MID_Hot,          //T_HOT           75
	MID_Gel,          //T_GEL           76
	MID_Station,      //T_STATION       77
	MID_PressurePlate,//T_PRESSPLATE    78
	MID_Bridge,       //T_BRIDGE        79
	MID_Bridge_H,     //T_BRIDGE_H      80
	MID_Bridge_V,     //T_BRIDGE_V      81
	MID_PitImage,     //T_PIT_IMAGE     82
	MID_WallImage,    //T_WALL_IMAGE    83
	MID_DarkCeiling,  //T_DARK_CEILING  84
	MID_WallLight,    //T_WALLLIGHT     85
	MID_ShallowWater, //T_SHALLOW_WATER 86
	MID_SquadHorn,    //T_HORN_SQUAD    87
	MID_SoldierHorn,  //T_HORN_SOLIDER  88
	MID_StepStone,    //T_STEP_STONE    89
	MID_Beacon,       //T_BEACON        90
	MID_BeaconOff,    //T_BEACON_OFF    91
	MID_PowderKeg,    //T_POWDER_KEG    92
	MID_FloorSpikes,  //T_FLOOR_SPIKES  93
	MID_ForceArrowDisabled, //T_ARROW_OFF_N   94
	MID_ForceArrowDisabled, //T_ARROW_OFF_NE  95
	MID_ForceArrowDisabled, //T_ARROW_OFF_E   96
	MID_ForceArrowDisabled, //T_ARROW_OFF_SE  97
	MID_ForceArrowDisabled, //T_ARROW_OFF_S   98
	MID_ForceArrowDisabled, //T_ARROW_OFF_SW  99
	MID_ForceArrowDisabled, //T_ARROW_OFF_W   100
	MID_ForceArrowDisabled, //T_ARROW_OFF_NW  101
	MID_OverheadImage, //T_OVERHEAD_IMAGE 102
	MID_Fluff,         //T_FLUFF        103
	MID_FluffVent,     //T_FLUFFVENT    104
	MID_ThinIce,       //T_THINICE      105
	MID_ThinIce2,      //T_THINICE_SH   106
	MID_Firetrap,      //T_FIRETRAP     107
	MID_FiretrapOn,    //T_FIRETRAP_ON  108
	MID_HoldCompleteWall, //T_WALL_WIN   109

	MID_Roach,        //M_ROACH         +0
	MID_RoachQueen,   //M_QROACH        +1
	MID_RoachEgg,     //M_REGG          +2
	MID_Goblin,       //M_GOBLIN        +3
	0,                //M_NEATHER       +4
	MID_Wraithwing,   //M_WWING         +5
	MID_EvilEye,      //M_EYE           +6
	MID_Serpent,      //M_SERPENT       +7
	MID_TarMother,    //M_TARMOTHER     +8
	MID_TarBaby,      //M_TARBABY       +9
	MID_Brain,        //M_BRAIN         +10
	MID_Mimic,        //M_MIMIC         +11
	MID_Spider,       //M_SPIDER        +12
	MID_GreenSerpent, //M_SERPENTG      +13
	MID_BlueSerpent,  //M_SERPENTB      +14
	MID_StoneGolem,   //M_ROCKGOLEM     +15
	MID_Ant,          //M_WATERSKIPPER  +16
	MID_AntHill,      //M_SKIPPERNEST   +17
	MID_Zombie,       //M_AUMTLICH      +18
	MID_Clone,        //M_CLONE         +19
	MID_Decoy,        //M_DECOY         +20
	MID_Wubba,        //M_WUBBA         +21
	MID_Ghost,        //M_SEEP          +22
	MID_Stalwart,     //M_STALWART      +23
	MID_Halph,        //M_HALPH         +24
	MID_Slayer,       //M_SLAYER        +25
	MID_Phoenix,      //M_FEGUNDO       +26
	MID_PhoenixAshes, //M_FEGUNDOASHES  +27
	MID_Guard,        //M_GUARD         +28
	MID_Character,    //M_CHARACTER     +29
	MID_MudMother,    //M_MUDMOTHER     +30
	MID_MudBaby,      //M_MUDBABY       +31
	MID_GelMother,    //M_GELMOTHER     +32
	MID_GelBaby,      //M_GELBABY       +33
	MID_Citizen,      //M_CITIZEN       +34
	MID_Splitter,     //M_ROCKGIANT     +35
	MID_Halph2,       //M_HALPH2        +36
	MID_Slayer2,      //M_SLAYER2       +37
	MID_Stalwart2,    //M_STALWART2     +38
	MID_Architect,    //M_ARCHITECT     +39
	MID_Construct,    //M_CONSTRUCT     +40
	MID_Gentryii,     //M_GENTRYII      +41
	MID_TemporalClone, //M_TEMPORALCLONE +42
	MID_FluffBaby,    //M_FLUFFBABY     +43
	
	MID_Swordsman,    //T_SWORDSMAN     TOTAL+0
	0,                //T_NOMONSTER     TOTAL+1
	0                 //T_EMPTY_F       TOTAL+2
};

static inline UINT getBuildMarkerTileMID(const UINT wTile){
	switch(wTile){
		case T_ORB_NORMAL: return MID_OrbWaitNormal;
		case T_ORB_CRACKED: return MID_OrbWaitCracked;
		case T_ORB_BROKEN: return MID_OrbWaitBroken;
		case T_EMPTY: return MID_RemoveItem;
		case T_REMOVE_FLOOR_ITEM: return MID_RemoveFloorLayer;
		case T_EMPTY_F: return MID_RemoveFloorLayer;
		case T_REMOVE_OVERHEAD_IMAGE: return MID_RemoveOverheadImage;
		default: return TILE_MID[wTile];
	}

}

static inline UINT getTarTypeForMother(const UINT mt)
{
	switch (mt) {
		case M_TARMOTHER: return T_TAR;
		case M_MUDMOTHER: return T_MUD;
		case M_GELMOTHER: return T_GEL;
	}
	return 0;
}

static inline UINT getMIDForMonster(const UINT mt)
{
	switch (mt) {
		case M_NEGOTIATOR: return MID_Negotiator;
		case M_CITIZEN1: return MID_Citizen1;
		case M_CITIZEN2: return MID_Citizen2;
		case M_CITIZEN3: return MID_Citizen3;
		case M_CITIZEN4: return MID_Citizen4;
		case M_GOBLINKING: return MID_GoblinKing;
		case M_INSTRUCTOR: return MID_Instructor;
		case M_MUDCOORDINATOR: return MID_MudCoordinator;
		case M_TARTECHNICIAN: return MID_TarTechnician;
		case M_EYE_ACTIVE: return MID_EvilEyeActive;
		case M_BEETHRO: return MID_Beethro;
		case M_BEETHRO_IN_DISGUISE: return MID_BeethroInDisguise;
		case M_GUNTHRO: return MID_Gunthro;
		case M_NONE: return MID_None;
		default: return (mt < MONSTER_TYPES) ? TILE_MID[mt + M_OFFSET] : 0;
	}
}

static inline UINT bConvertFakeElement(const UINT t) {
	if (bIsFakeTokenType(t)){
		return T_TOKEN;
	}
	if (bIsFakeOrbType(t)){
		return T_ORB;
	}
	if (bIsFakePressurePlateType(t)){
		return T_PRESSPLATE;
	}
	if (t == T_REMOVE_FLOOR_ITEM){
		return T_EMPTY_F;
	}
	return t;
}
#endif //...#ifndef TILECONSTANTS_H
