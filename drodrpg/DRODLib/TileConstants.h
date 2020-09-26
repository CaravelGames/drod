// $Id: TileConstants.h 9414 2010-03-27 15:49:50Z mrimer $

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

#include <BackEndLib/Types.h>
#include "../Texts/MIDs.h"

//Tile constants--reordering existing constants may break macros.  Add new
//tile constants to end and make corresponding update to TILE_COUNT and
//TILE_LAYER.
#define T_EMPTY         0
#define T_FLOOR         1
#define T_PIT           2
#define T_STAIRS        3
#define T_WALL          4
#define T_WALL_B        5  //Broken
#define T_DOOR_C        6  //Cyan (Blue)
#define T_DOOR_G        7  //Green
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
#define T_HEALTH_MED    21 //Health (medium)
#define T_HEALTH_BIG    22 //Health (large)
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
//#define T_POTION_SP     38 //Speed //now an accessory T_REUSE
#define T_BRIAR_SOURCE   39
#define T_BRIAR_DEAD    40
#define T_BRIAR_LIVE     41
#define T_LIGHT_CEILING 42 //ceiling light; used in front end
#define T_BOMB          43
#define T_FUSE          44
#define T_NODIAGONAL    45
#define T_TOKEN         46 //architectural token
#define T_TUNNEL_N      47
#define T_TUNNEL_S      48
#define T_MIRROR        49
#define T_ATK_UP        50 //Attack up
#define T_DEF_UP        51 //Defense up
#define T_PLATFORM_W    52 //Platform (on water)
#define T_PLATFORM_P    53 //Platform (over pit)
#define T_FLOOR_M       54 //Floor mosaic
#define T_FLOOR_ROAD    55 //road texture
#define T_FLOOR_GRASS   56 //grass texture
#define T_FLOOR_DIRT    57 //dirt texture
#define T_FLOOR_ALT     58 //alternate texture
#define T_DOOR_MONEY    59 //money door
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
#define T_GOO           73
#define T_LIGHT         74	//light source
#define T_HOT           75 //hot tile
#define T_GEL           76 //gel (tarstuff)
#define T_MAP           77 //level map
#define T_PRESSPLATE    78 //pressure plate
#define T_BRIDGE        79 //bridge
#define T_BRIDGE_H      80 //bridge (horizontal)
#define T_BRIDGE_V      81 //bridge (vertical)
#define T_PIT_IMAGE     82 //user-defined image
#define T_WALL_IMAGE    83 //user-defined image
#define T_DARK_CEILING  84 //opposite of ceiling light; used in front end
#define T_WALLLIGHT     85 //light source (alternate)
#define T_KEY           86 //key
#define T_SWORD         87 //sword
#define T_DOOR_MONEYO   88 //money door (open)
#define T_SHIELD        89 //shield
#define T_HEALTH_SM     90 //Health (small)
#define T_ACCESSORY     91 //accessory

#define TILE_COUNT     (92) //Number of tile constants from above list.
static inline bool IsValidTileNo(const UINT t) {return t < TILE_COUNT;}

//
//Tile-related macros (inlined functions).
// Used for combining specific tile types into types with common interaction behavior.
//

static inline bool bIsBridge(const UINT t) {return t==T_BRIDGE || t==T_BRIDGE_H || t==T_BRIDGE_V;}

static inline bool bIsCustomImageTile(const UINT t) {return t==T_FLOOR_IMAGE || t== T_PIT_IMAGE || t==T_WALL_IMAGE;}

static inline bool bIsTrapdoor(const UINT t) {return t==T_TRAPDOOR || t==T_TRAPDOOR2;}

static inline bool bIsPlainFloor(const UINT t) {return t==T_FLOOR || (t>=T_FLOOR_M && t<=T_FLOOR_ALT) || t==T_FLOOR_IMAGE;}

static inline bool bIsFloor(const UINT t) {return bIsPlainFloor(t) ||
		bIsTrapdoor(t) || bIsBridge(t) || t==T_HOT || t==T_GOO || t==T_PRESSPLATE;}

static inline bool bIsLight(const UINT t) {return t==T_LIGHT;}

static inline bool bIsWall(const UINT t) {return t==T_WALL || t==T_WALL2 || t==T_WALL_IMAGE;}

static inline bool bIsCrumblyWall(const UINT t) {return t==T_WALL_B || t==T_WALL_H;}

static inline bool bIsStairs(const UINT t) {return t==T_STAIRS || t==T_STAIRS_UP;}

static inline bool bIsPit(const UINT t) {return t==T_PIT || t==T_PIT_IMAGE;}

static inline bool bIsWater(const UINT t) {return t==T_WATER;}

static inline bool bIsArrow(const UINT t) {return t>=T_ARROW_N && t<=T_ARROW_NW;}

static inline bool bIsHealth(const UINT t) {
	switch (t)
	{
		case T_HEALTH_BIG: case T_HEALTH_MED: case T_HEALTH_SM:
			return true;
		default: return false;
	}
}

static inline bool bIsPowerUp(const UINT t) {
	switch (t)
	{
		case T_MAP:
		case T_ATK_UP: case T_DEF_UP:
		case T_HEALTH_BIG: case T_HEALTH_MED: case T_HEALTH_SM:
			return true;
		default: return false;
	}
}

static inline bool bIsSerpentTile(const UINT t) {return t>=T_SNK_EW && t<=T_SNKT_E;}

static inline bool bIsBriar(const UINT t) {return t>=T_BRIAR_SOURCE && t<=T_BRIAR_LIVE;}

static inline bool bIsPlatform(const UINT t) {return t==T_PLATFORM_W || t==T_PLATFORM_P;}

static inline bool bIsDoor(const UINT t) {return (t>=T_DOOR_C && t<=T_DOOR_Y) ||	t==T_DOOR_B || t==T_DOOR_MONEY;}   //closed doors

static inline bool bIsOpenDoor(const UINT t) {return t==T_DOOR_YO ||	(t>=T_DOOR_GO && t<=T_DOOR_BO) || t==T_DOOR_MONEYO;}

static inline bool bIsYellowDoor(const UINT t) {return t==T_DOOR_Y || t==T_DOOR_YO;}
static inline bool bIsGreenDoor(const UINT t) {return t==T_DOOR_G || t==T_DOOR_GO;}
static inline bool bIsBlueDoor(const UINT t) {return t==T_DOOR_C || t==T_DOOR_CO;}
static inline bool bIsRedDoor(const UINT t) {return t==T_DOOR_R || t==T_DOOR_RO;}
static inline bool bIsBlackDoor(const UINT t) {return t==T_DOOR_B || t==T_DOOR_BO;}

static inline bool bIsTar(const UINT t) {return t==T_TAR || t==T_MUD || t==T_GEL;}

static inline bool bIsTunnel(const UINT t) {return t==T_TUNNEL_N || t==T_TUNNEL_S || t==T_TUNNEL_E || t==T_TUNNEL_W;}

static inline bool bIsElevatedTile(const UINT t) {
	return bIsWall(t) || bIsCrumblyWall(t) || bIsDoor(t);
}

static inline bool bIsEquipment(const UINT t) {return t==T_SWORD || t==T_SHIELD || t==T_ACCESSORY;}

//Obstacle parameter bit format: <Top edge>:1 <Left edge>:1 <64 possible obstacle types>:6
#define OBSTACLE_TOP (0x80)
#define OBSTACLE_LEFT (0x40)
static inline bool bObstacleTop(const UINT obType) {return (obType & OBSTACLE_TOP) != 0;}
static inline bool bObstacleLeft(const UINT obType) {return (obType & OBSTACLE_LEFT) != 0;}
static inline UINT calcObstacleType(const UINT obType) {return obType & (OBSTACLE_LEFT-1);}

//Token parameter bit format: <on/off>:1 <types>:128
#define TOKEN_ACTIVE (0x80)     //on/off
static inline bool bTokenActive(const UINT val) {return (val & TOKEN_ACTIVE) != 0;}
static inline UINT calcTokenType(const UINT val) {return val & (TOKEN_ACTIVE-1);}

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
static inline UINT calcLightRadius(const UINT val) {return (val & ((MAX_LIGHT_DISTANCE-1)*NUM_LIGHT_TYPES))/NUM_LIGHT_TYPES;}
static inline UINT calcLightType(const UINT val) {return val & (NUM_LIGHT_TYPES-1);}

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
#define T_PIRATE     (M_PIRATE+M_OFFSET)
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
#define T_MADEYE     (M_MADEYE+M_OFFSET)
#define T_GOBLINKING (M_GOBLINKING+M_OFFSET)
#define MONSTER_COUNT      (38)  //Number of monsters in above list.

#define TOTAL_TILE_COUNT      (TILE_COUNT+MONSTER_COUNT) //Number of all tile constants.
static inline bool IsMonsterTileNo(const UINT t) {return t>=TILE_COUNT && t<TOTAL_TILE_COUNT;}

//Virtual tiles: special values used to identify building custom items that don't appear in the above list
#define TV_KEY_Y       ((UINT)-1)
#define TV_KEY_G       ((UINT)-2)
#define TV_KEY_B       ((UINT)-3)
#define TV_KEY_S       ((UINT)-4)
#define TV_EXPLOSION   ((UINT)-5)
#define TV_SWORD1      ((UINT)-6)
#define TV_SWORD2      ((UINT)-7)
#define TV_SWORD3      ((UINT)-8)
#define TV_SWORD4      ((UINT)-9)
#define TV_SWORD5      ((UINT)-10)
#define TV_SWORD6      ((UINT)-11)
#define TV_SWORD7      ((UINT)-12)
#define TV_SWORD8      ((UINT)-13)
#define TV_SWORD9      ((UINT)-14)
#define TV_SWORD10     ((UINT)-15)
#define TV_SHIELD1     ((UINT)-16)
#define TV_SHIELD2     ((UINT)-17)
#define TV_SHIELD3     ((UINT)-18)
#define TV_SHIELD4     ((UINT)-19)
#define TV_SHIELD5     ((UINT)-20)
#define TV_SHIELD6     ((UINT)-21)
#define TV_ACCESSORY1  ((UINT)-22)
#define TV_ACCESSORY2  ((UINT)-23)
#define TV_ACCESSORY3  ((UINT)-24)
#define TV_ACCESSORY4  ((UINT)-25)
#define TV_ACCESSORY5  ((UINT)-26)
#define TV_ACCESSORY6  ((UINT)-27)
#define TV_ACCESSORY7  ((UINT)-28)
#define TV_ACCESSORY8  ((UINT)-29)
#define TV_ACCESSORY9  ((UINT)-30)
#define TV_ACCESSORY10 ((UINT)-31)
#define TV_ACCESSORY11 ((UINT)-32)
#define TV_ACCESSORY12 ((UINT)-33)
static inline bool IsVirtualTile(const UINT t) {return t>=(UINT)TV_ACCESSORY12;}

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
	LAYER_OPAQUE, //T_DOOR_G        7
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
	LAYER_TRANSPARENT, //T_HEALTH_MED    21
	LAYER_TRANSPARENT, //T_HEALTH_BIG    22
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
	LAYER_TRANSPARENT, //T_BRIAR_SOURCE   39
	LAYER_TRANSPARENT, //T_BRIAR_DEAD    40
	LAYER_TRANSPARENT, //T_BRIAR_LIVE     41
	LAYER_FLOOR, //T_LIGHT_CEILING 42 //front end only -- show on f-layer menu
	LAYER_TRANSPARENT, //T_BOMB          43
	LAYER_TRANSPARENT, //T_FUSE          44
	LAYER_FLOOR, //T_NODIAGONAL    45 //was in layer 1 before v3.0
	LAYER_TRANSPARENT, //T_TOKEN         46
	LAYER_OPAQUE, //T_TUNNEL_N      47
	LAYER_OPAQUE, //T_TUNNEL_S      48
	LAYER_TRANSPARENT, //T_MIRROR        49
	LAYER_TRANSPARENT, //T_ATK_UP        50
	LAYER_TRANSPARENT, //T_DEF_UP        51
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
	LAYER_TRANSPARENT, //T_LIGHT			74
	LAYER_OPAQUE, //T_HOT           75
	LAYER_TRANSPARENT, //T_GEL           76
	LAYER_TRANSPARENT, //T_MAP           77
	LAYER_OPAQUE, //T_PRESSPLATE    78
	LAYER_OPAQUE, //T_BRIDGE        79
	LAYER_OPAQUE, //T_BRIDGE_H      80
	LAYER_OPAQUE, //T_BRIDGE_V      81
	LAYER_OPAQUE, //T_PIT_IMAGE     82
	LAYER_OPAQUE, //T_WALL_IMAGE    83
	LAYER_FLOOR, //T_DARK_CEILING  84 //front end only -- show on f-layer menu
	LAYER_FLOOR, //T_WALLLIGHT     85 //front end only -- show on f-layer menu
	LAYER_TRANSPARENT, //T_KEY           86
	LAYER_TRANSPARENT, //T_SWORD         87
	LAYER_OPAQUE, //T_DOOR_MONEY    88
	LAYER_TRANSPARENT, //T_SHIELD        89
	LAYER_TRANSPARENT, //T_HEALTH_SM     90
	LAYER_TRANSPARENT, //T_ACCESSORY     91

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
	LAYER_MONSTER, //M_PIRATE        +23
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
	LAYER_MONSTER, //M_MADEYE        +36
	LAYER_MONSTER, //M_GOBLINKING    +37

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
	MID_GreenDoor,    //T_DOOR_G        7
	MID_RedDoor,      //T_DOOR_R        8
	MID_YellowDoor,   //T_DOOR_Y        9
	MID_OpenYellowDoor,//T_DOOR_YO       10
	MID_Trapdoor,     //T_TRAPDOOR      11
	MID_Obstacle,     //T_OBSTACLE      12
	MID_ForceArrow_N, //T_ARROW_N       13
	MID_ForceArrow_NE,//T_ARROW_NE      14
	MID_ForceArrow_E, //T_ARROW_E       15
	MID_ForceArrow_SE,//T_ARROW_SE      16
	MID_ForceArrow_S, //T_ARROW_S       17
	MID_ForceArrow_SW,//T_ARROW_SW      18
	MID_ForceArrow_W, //T_ARROW_W       19
	MID_ForceArrow_NW,//T_ARROW_NW      20
	MID_MediumHealth, //T_HEALTH_MED    21
	MID_LargeHealth,  //T_HEALTH_BIG    22
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
	MID_FlowInner,    //T_BRIAR_DEAD    40
	MID_FlowInner,    //T_BRIAR_LIVE    41
	MID_LightCeiling, //T_LIGHT_CEILING 42
	MID_Bomb,         //T_BOMB          43
	MID_Fuse,         //T_FUSE          44
	MID_Ortho,        //T_NODIAGONAL    45
	MID_Token,        //T_TOKEN         46
	MID_Tunnel_N,     //T_TUNNEL_N      47
	MID_Tunnel_S,     //T_TUNNEL_S      48
	MID_Mirror,       //T_MIRROR        49
	MID_AttackUp,     //T_ATK_UP        50
	MID_DefenseUp,    //T_DEF_UP        51
	MID_PlatformWater,//T_PLATFORM_W    52
	MID_PlatformPit,  //T_PLATFORM_P    53
	MID_FloorMosaic,  //T_FLOOR_M       54
	MID_FloorRoad,    //T_FLOOR_ROAD    55
	MID_FloorGrass,   //T_FLOOR_GRASS   56
	MID_FloorDirt,    //T_FLOOR_DIRT    57
	MID_FloorAlt,     //T_FLOOR_ALT     58
	MID_MoneyDoor,    //T_DOOR_MONEY    59
	MID_Mud,          //T_MUD           60
	MID_StairsUp,     //T_STAIRS_UP     61
	MID_SecretWall,   //T_WALL_H        62
	MID_Tunnel_E,     //T_TUNNEL_E      63
	MID_Tunnel_W,     //T_TUNNEL_W      64
	MID_FloorImage,   //T_FLOOR_IMAGE   65
	MID_Wall2,        //T_WALL2         66
	MID_Water,			//T_WATER         67
	MID_OpenGreenDoor,//T_DOOR_GO       68
	MID_OpenBlueDoor, //T_DOOR_CO       69
	MID_OpenRedDoor,  //T_DOOR_RO       70
	MID_OpenBlackDoor,//T_DOOR_BO       71
	MID_Trapdoor2,    //T_TRAPDOOR2     72
	MID_Goo,			   //T_GOO           73
	MID_Light,        //T_LIGHT         74
	MID_Hot,          //T_HOT           75
	MID_Gel,	         //T_GEL           76
	MID_Station,      //T_MAP           77
	MID_PressurePlate,//T_PRESSPLATE    78
	MID_Bridge,       //T_BRIDGE        79
	MID_Bridge_H,     //T_BRIDGE_H      80
	MID_Bridge_V,     //T_BRIDGE_V      81
	MID_PitImage,     //T_PIT_IMAGE     82
	MID_WallImage,    //T_WALL_IMAGE    83
	MID_DarkCeiling,  //T_DARK_CEILING  84
	MID_WallLight,    //T_WALLLIGHT     85
	MID_YellowKey,    //T_KEY           86
	MID_Sword1,       //T_SWORD         87
	MID_OpenMoneyDoor,//T_DOOR_MONEYO   88
	MID_Shield1,      //T_SHIELD        89
	MID_SmallHealth,  //T_HEALTH_SM     90
	MID_Accessory1,   //T_ACCESSORY     91

	MID_Roach,        //M_ROACH         +0
	MID_RoachQueen,   //M_QROACH        +1
	MID_RoachEgg,     //M_REGG          +2
	MID_Goblin,       //M_GOBLIN        +3
	MID_NeatherNoApostrophe, //M_NEATHER       +4
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
	MID_Pirate,       //M_PIRATE        +23
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
	MID_MadEye,       //M_MADEYE        +36
	MID_GoblinKing,   //M_GOBLINKING    +37

	MID_Swordsman,    //T_SWORDSMAN     TOTAL+0
	0,                //T_NOMONSTER     TOTAL+1
	0                 //T_EMPTY_F       TOTAL+2
};

#endif //...#ifndef TILECONSTANTS_H

