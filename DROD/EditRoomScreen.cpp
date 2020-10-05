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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//How to add a new item to the room editor:
//1. Add menu display tile(s) to MenuDisplayTiles.
//2. Add to SinglePlacement, wItemX and wItemY tables.
//3. Add to one of the ordered layer entry lists.
//4. Add to DisplaySelection.
//5. Add placement handling in PlotObjects.
//6. Define placement rules in CEditRoomWidget::IsSafePlacement.
//7. If the new item is customizeable,
//     new supporting UI must be added in one or more places.
//     A suggestion for doing this would be to examine another customizeable
//     object that most resembles the new one and roughly mirror the steps
//     the are handled throughout this class for working with that object.

#include "EditRoomScreen.h"
#include "EditSelectScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "BrowserScreen.h"
#include "DemosScreen.h"
#include "GameScreen.h"
#include "EntranceSelectDialogWidget.h"
#include "MapWidget.h"
#include "CharacterDialogWidget.h"
#include "CharacterOptionsWidget.h"
#include "TileImageCalcs.h"

#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/ObjectMenuWidget.h>
#include <FrontEndLib/TabbedMenuWidget.h>
#include <FrontEndLib/TextBox2DWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/TheoraPlayer.h>

#include "../DRODLib/TileConstants.h"
#include "../DRODLib/Character.h"
#include "../DRODLib/MonsterPiece.h"
#include "../DRODLib/Serpent.h"
#include "../DRODLib/PlayerDouble.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/IDList.h>

const UINT TAG_MENU = 1010;
const UINT TAG_OMENU = 1011;
const UINT TAG_FMENU = 1012;
const UINT TAG_TMENU = 1013;
const UINT TAG_MMENU = 1014;
const UINT TAB_ITEMTEXT = 1015;

//Pop-up menus.
const UINT TAG_OBS_MENU = 1020;
const UINT TAG_LIGHT_MENU = 1021;
const UINT TAG_DARK_MENU = 1022;
const UINT TAG_FLOOR_MENU = 1023;
const UINT TAG_TOKEN_MENU = 1024;
const UINT TAG_POTION_MENU = 1025;

const UINT TAG_CHATBOX = 1030;
const UINT TAG_CHATENABLE = 1035;
const UINT TAG_CHATWHISPERSONLY = 1036;
const UINT TAG_CHATINPUT = 1037;
const UINT TAG_CHATUSERS = 1038;

const UINT TAG_UNDO = 1040;
const UINT TAG_REDO = 1041;
const UINT TAG_SHOWERRORS = 1042;

const UINT TAG_HELP = 1050;
const UINT TAG_ESC = 1051;

//Level entrance dialog.
const UINT TAG_LEVELENTRANCETEXTBOX = 1060;
const UINT TAG_MAINENTRANCE = 1061;
const UINT TAG_SHOWDESCRIPTION = 1062;
const UINT TAG_SHOWDESCRIPTION_ONCE = 1063;
const UINT TAG_LEVELENTRANCEAUDIO = 1064;
const UINT TAG_LEVELENTRANCEAUDIONAME = 1065;

#define NO_ENTRANCE (static_cast<UINT>(-1))

const SURFACECOLOR PaleYellow = {255, 255, 128};

//Menu background color.
#define BG_COLOR 255,250,205
#ifdef GAME_RENDERING_OFFSET
const Uint32 MenuBGColor = 0xff << 8 | 0xfa << 16 | 0xcd << 24; //pale yellow
#else
const Uint32 MenuBGColor = 0xff << 16 | 0xfa << 8 | 0xcd; //pale yellow
#endif

//Game objects.
const UINT MenuDisplayTiles[TOTAL_EDIT_TILE_COUNT][4] =
{
	{TI_FLOOR},                                        //T_EMPTY
	{TI_FLOOR},                                        //T_FLOOR
	{TI_PIT_M},                                        //T_PIT
	{TI_STAIRS_2,TI_STAIRS_3},                         //T_STAIRS
	{TI_WALL_N,TI_WALL_N,TI_WALL_S,TI_WALL_S},         //T_WALL
	{TI_WALL_BN,TI_WALL_BN,TI_WALL_BS,TI_WALL_BS},     //T_WALL_B
	{TI_DOOR_C},                                       //T_DOOR_C
	{TI_DOOR_M},                                       //T_DOOR_M
	{TI_DOOR_R},                                       //T_DOOR_R
	{TI_DOOR_Y},                                       //T_DOOR_Y
	{TI_DOOR_YO},                                      //T_DOOR_YO
	{TI_TRAPDOOR},                                     //T_TRAPDOOR
	{TI_OB_1_1},                                       //T_OBSTACLE
	{TI_ARROW_N},                                      //T_ARROW_N
	{TI_ARROW_NE},                                     //T_ARROW_NE
	{TI_ARROW_E},                                      //T_ARROW_E
	{TI_ARROW_SE},                                     //T_ARROW_SE
	{TI_ARROW_S},                                      //T_ARROW_S
	{TI_ARROW_SW},                                     //T_ARROW_SW
	{TI_ARROW_W},                                      //T_ARROW_W
	{TI_ARROW_NW},                                     //T_ARROW_NW
	{TI_POTION_I},                                     //T_POTION_I
	{TI_POTION_K},                                     //T_POTION_K
	{TI_SCROLL},                                       //T_SCROLL
	{TI_ORB_D},                                        //T_ORB
	{DONT_USE},                                        //T_SNK* (deprecated)
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{DONT_USE},                                        //
	{TI_TAR_NW,TI_TAR_NE,TI_TAR_SW,TI_TAR_SE},         //T_TAR
	{TI_CHECKPOINT},                                   //T_CHECKPOINT
	{TI_DOOR_B},                                       //T_DOOR_B
	{TI_POTION_SP},                                    //T_POTION_SP
	{TI_BRIARROOT},                                    //T_BRIAR_SOURCE
	{TI_BRIAR},                                        //T_BRIAR_DEAD
	{TI_BRIAREDGE},                                    //T_BRIAR_LIVE
	{TI_LIGHT_CEILING},                                //T_LIGHT_CEILING
	{TI_BOMB},                                         //T_BOMB
	{TI_FUSE_NSWE},                                    //T_FUSE
	{TI_NODIAGONAL},                                   //T_NODIAGONAL
	{TI_TOKEN_CW},                                     //T_TOKEN
	{TI_TUNNEL_N},                                     //T_TUNNEL_N
	{TI_TUNNEL_S},                                     //T_TUNNEL_S
	{TI_MIRROR},                                       //T_MIRROR
	{TI_POTION_C},                                     //T_POTION_C
	{TI_POTION_D},                                     //T_POTION_D
	{TI_PLATFORM_W},                                   //T_PLATFORM_W
	{TI_PLATFORM_P},                                   //T_PLATFORM_P
	{TI_FLOOR_M},                                      //T_FLOOR_M
	{TI_ROAD},                                         //T_FLOOR_ROAD
	{TI_GRASS},                                        //T_FLOOR_GRASS
	{TI_DIRT},                                         //T_FLOOR_DIRT
	{TI_ALT},                                          //T_FLOOR_ALT
	{TI_WALL_MASTER},                                  //T_WALL_M
	{TI_MUD_NW,TI_MUD_NE,TI_MUD_SW,TI_MUD_SE},         //T_MUD
	{TI_STAIRSUP_1,TI_STAIRSUP_1},                     //T_STAIRS_UP
	{TI_WALL_HN,TI_WALL_HN,TI_WALL_HS,TI_WALL_HS},     //T_WALL_H
	{TI_TUNNEL_E},                                     //T_TUNNEL_E
	{TI_TUNNEL_W},                                     //T_TUNNEL_W
	{TI_FLOOR_IMAGE},                                  //T_FLOOR_IMAGE
	{TI_WALL,TI_WALL,TI_WALL,TI_WALL},                 //T_WALL2
	{TI_WATER_TOP},                                    //T_WATER
	{TI_DOOR_GO},                                      //T_DOOR_GO
	{TI_DOOR_CO},                                      //T_DOOR_CO
	{TI_DOOR_RO},                                      //T_DOOR_RO
	{TI_DOOR_BO},                                      //T_DOOR_BO
	{TI_TRAPDOOR2},                                    //T_TRAPDOOR2
	{TI_GOO_NSWE},                                     //T_GOO
	{TI_LIGHT},                                        //T_LIGHT
	{TI_HOT},                                          //T_HOT
	{TI_GEL_NW,TI_GEL_NE,TI_GEL_SW,TI_GEL_SE},         //T_GEL
	{TI_STATION},                                      //T_STATION
	{TI_PP},                                           //T_PRESSPLATE
	{TI_BRIDGE},                                       //T_BRIDGE
	{TI_BRIDGE_H},                                     //T_BRIDGE_H
	{TI_BRIDGE_V},                                     //T_BRIDGE_V
	{TI_PIT_IMAGE},                                    //T_PIT_IMAGE
	{TI_WALL_IMAGE},                                   //T_WALL_IMAGE
	{TI_DARK_CEILING},                                 //T_DARK_CEILING
	{TI_WALLLIGHT},                                    //T_WALLLIGHT
	{TI_SHALLOW_TOP},                                  //T_SHALLOW_WATER
	{TI_HORN_SQUAD},                                   //T_HORN_SQUAD
	{TI_HORN_SOLDIER},                                 //T_HORN_SOLDIER
	{TI_STEP_STONE},                                   //T_STEP_STONE
	{TI_BEACON},                                       //T_BEACON
	{TI_BEACON_OFF},                                   //T_BEACON_OFF
	{TI_POWDER_KEG},                                   //T_POWDER_KEG
	{TI_FLOOR_SPIKES_DOWN},                            //T_FLOOR_SPIKES
	{TI_ARROW_OFF_N},                                  //T_ARROW_OFF_N
	{TI_ARROW_OFF_NE},                                 //T_ARROW_OFF_NE
	{TI_ARROW_OFF_E},                                  //T_ARROW_OFF_E
	{TI_ARROW_OFF_SE},                                 //T_ARROW_OFF_SE
	{TI_ARROW_OFF_S},                                  //T_ARROW_OFF_S
	{TI_ARROW_OFF_SW},                                 //T_ARROW_OFF_SW
	{TI_ARROW_OFF_W},                                  //T_ARROW_OFF_W
	{TI_ARROW_OFF_NW},                                 //T_ARROW_OFF_NW
	{TI_OVERHEAD_IMAGE},                               //T_OVERHEAD_IMAGE
	{TI_FLUFF_NW, TI_FLUFF_NE, TI_FLUFF_SW, TI_FLUFF_SE}, //T_FLUFF
	{TI_FLUFFVENT},                                    //T_FLUFFVENT
	{TI_THINICE_NSWE},                                 //T_THINICE
	{TI_THINICE},                                      //T_THINICE_SH
	{TI_FIRETRAP},                                     //T_FIRETRAP
	{TI_FIRETRAP_ON},                                  //T_FIRETRAP_ON
	{TI_WALL_WIN},                                     //T_WALL_WIN

	//monsters
	{TI_ROACH_SE},
	{TI_QROACH_SE},
	{DONT_USE},
	{TI_GOBLIN_SE},
	{DONT_USE},
	{TI_WW_SE},
	{TI_EYE_SE},
	{TI_SNKT_W,TI_SNK_EW,TI_SNK_E},
	{TI_TAREYE_WO,TI_TAREYE_EO},
	{TI_TARBABY_SE},
	{TI_BRAIN},
	{TI_MIMIC_SE},
	{TI_SPIDER_SE},
	{TI_SNKT_G_W,TI_SNK_G_EW,TI_SNK_G_E},
	{TI_SNKT_B_W,TI_SNK_B_EW,TI_SNK_B_E},
	{TI_ROCK_SE},
	{TI_WATERSKIPPER_SE},
	{TI_SKIPPERNEST},
	{TI_AUMTLICH_SE},
	{TI_SMAN_YSE},
	{TI_SMAN_IYSE},
	{TI_WUBBA},
	{TI_SEEP_SE},
	{TI_STALWART_SE},
	{TI_HALPH_SE},
	{TI_SLAYER_SE},
	{TI_FEGUNDO_SE},
	{DONT_USE},
	{TI_GUARD_SE},
	{TI_NEGOTIATOR_SE},
	{TI_MUDEYE_WO,TI_MUDEYE_EO},
	{TI_MUDBABY_SE},
	{TI_GELEYE_WO,TI_GELEYE_EO},
	{TI_GELBABY_SE},
	{TI_CITIZEN_SE},
	{TI_ROCKGIANT_SE,TI_ROCKGIANT_SE1,TI_ROCKGIANT_SE2,TI_ROCKGIANT_SE3},
	{TI_HALPH_SE},
	{TI_SLAYER_SE},
	{TI_SOLDIER_SE},
	{TI_ARCHITECT_SE},
	{TI_CONSTRUCT_ASE},
	{TI_GENTRYII_LINK_E, TI_GENTRYII_HEAD_E},
	{TI_SMAN_YSE},
	{TI_FLUFFBABY},

	{TI_SMAN_YE, TI_SWORD_YE},
	{DONT_USE},
	{DONT_USE}
};

//Specifies whether the given object is only allowed to be placed one at a
//time.
const bool SinglePlacement[TOTAL_EDIT_TILE_COUNT] =
{
	0, //T_EMPTY         0
	0, //T_FLOOR         1
	0, //T_PIT           2
	0, //T_STAIRS        3
	0, //T_WALL          4
	0, //T_WALL_B        5
	0, //T_DOOR_C        6
	0, //T_DOOR_M        7
	0, //T_DOOR_R        8
	0, //T_DOOR_Y        9
	0, //T_DOOR_YO       10
	0, //T_TRAPDOOR      11
	0, //T_OBSTACLE      12
	0, //T_ARROW_N       13
	0, //T_ARROW_NE      14
	0, //T_ARROW_E       15
	0, //T_ARROW_SE      16
	0, //T_ARROW_S       17
	0, //T_ARROW_SW      18
	0, //T_ARROW_W       19
	0, //T_ARROW_NW      20
	0, //T_POTION_I      21
	0, //T_POTION_K      22
	1, //T_SCROLL        23
	0, //T_ORB           24
	0, //T_SNK_EW        25
	0, //T_SNK_NS        26
	0, //T_SNK_NW        27
	0, //T_SNK_NE        28
	0, //T_SNK_SW        29
	0, //T_SNK_SE        30
	1, //T_SNKT_S        31
	1, //T_SNKT_W        32
	1, //T_SNKT_N        33
	1, //T_SNKT_E        34
	0, //T_TAR           35
	1, //T_CHECKPOINT    36
	0, //T_DOOR_B        37
	0, //T_POTION_SP     38
	0, //T_BRIAR_SOURCE  39
	0, //T_BRIAR_DEAD    40
	0, //T_BRIAR_LIVE    41
	0, //T_LIGHT_CEILING 42
	0, //T_BOMB          43
	0, //T_FUSE          44
	0, //T_NODIAGONAL    45
	0, //T_TOKEN         46
	1, //T_TUNNEL_N      47
	1, //T_TUNNEL_S      48
	0, //T_MIRROR        49
	0, //T_POTION_C      50
	0, //T_POTION_D      51
	0, //T_PLATFORM_W    52
	0, //T_PLATFORM_P    53
	0, //T_FLOOR_M       54
	0, //T_FLOOR_ROAD    55
	0, //T_FLOOR_GRASS   56
	0, //T_FLOOR_DIRT    57
	0, //T_FLOOR_ALT     58
	0, //T_WALL_M        59
	0, //T_MUD           60
	0, //T_STAIRS_UP     61
	0, //T_WALL_H        62
	1, //T_TUNNEL_E      63
	1, //T_TUNNEL_W      64
	0, //T_FLOOR_IMAGE   65
	0, //T_WALL2         66
	0, //T_WATER         67
	0, //T_DOOR_GO       68
	0, //T_DOOR_CO       69
	0, //T_DOOR_RO       70
	0, //T_DOOR_BO       71
	0, //T_TRAPDOOR2     72
	0, //T_GOO           73
	0, //T_LIGHT         74
	0, //T_HOT           75
	0, //T_GEL           76
	0, //T_STATION       77
	0, //T_PRESSPLATE    78
	0, //T_BRIDGE        79
	0, //T_BRIDGE_H      80
	0, //T_BRIDGE_V      81
	0, //T_PIT_IMAGE     82
	0, //T_WALL_IMAGE    83
	0, //T_DARK_CEILING  84
	0, //T_WALLLIGHT     85
	0, //T_SHALLOW_WATER 86
	0, //T_HORN_SQUAD    87
	0, //T_HORN_SOLDIER  88
	0, //T_STEP_STONE    89
	0, //T_BEACON        90
	0, //T_BEACON_OFF    91
	0, //T_POWDER_KEG    92
	0, //T_FLOOR_SPIKES  93
	0, //T_ARROW_OFF_N   94
	0, //T_ARROW_OFF_NE  95
	0, //T_ARROW_OFF_E   96
	0, //T_ARROW_OFF_SE  97
	0, //T_ARROW_OFF_S   98
	0, //T_ARROW_OFF_SW  99
	0, //T_ARROW_OFF_W   100
	0, //T_ARROW_OFF_NW  101
	0, //T_OVERHEAD_IMAGE 102
	0, //T_FLUFF         103
	0, //T_FLUFFVENT     104
	0, //T_THINICE       105
	0, //T_THINICE_SH    106
	0, //T_FIRETRAP      107
	0, //T_FIRETRAP_ON   108
	0, //T_WALL_WIN      109
	
	0, //T_ROACH         +0
	0, //T_QROACH        +1
	0, //T_REGG          +2
	0, //T_GOBLIN        +3
	1, //T_NEATHER       +4
	0, //T_WWING         +5
	0, //T_EYE           +6
	1, //T_SERPENT       +7
	1, //T_TARMOTHER     +8
	0, //T_TARBABY       +9
	0, //T_BRAIN         +10
	0, //T_MIMIC         +11
	0, //T_SPIDER        +12
	1, //T_SERPENTG      +13
	1, //T_SERPENTB      +14
	0, //T_ROCKGOLEM     +15
	0, //T_WATERSKIPPER  +16
	0, //T_SKIPPERNEST   +17
	0, //T_AUMTLICH      +18
	0, //T_CLONE         +19
	0, //T_DECOY         +20
	0, //T_WUBBA         +21
	0, //T_SEEP          +22
	0, //T_STALWART      +23
	0, //T_HALPH         +24
	0, //T_SLAYER        +25
	0, //T_FEGUNDO       +26
	0, //T_FEGUNDOASHES  +27
	0, //T_GUARD         +28
	1, //T_CHARACTER     +29
	1, //T_MUDMOTHER     +30
	0, //T_MUDBABY       +31
	1, //T_GELMOTHER     +32
	0, //T_GELBABY       +33
	0, //T_CITIZEN       +34
	0, //T_ROCKGIANT     +35
	0, //T_HALPH2        +36
	0, //T_SLAYER2       +37
	0, //T_STALWART2     +38
	0, //T_ARCHITECT     +39
	0, //T_CONSTRUCT     +40
	1, //T_GENTRYII      +41
	0, //T_TEMPORALCLONE +42
	0, //T_FLUFFBABY     +43

	1, //T_SWORDSMAN     TOTAL+0
	0, //T_NOMONSTER     TOTAL+1
	0  //T_EMPTY_F       TOTAL+2
};

const UINT wItemX[TOTAL_EDIT_TILE_COUNT] = {
	1, 1, 1, 1, 2, 2, //6
	1, 1, 1, 1, 1, //doors
	1, 1,  //2
	1, 1, 1, 1, 1, 1, 1, 1,  //8 arrows
	1, 1, 1, 1,  //4
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10 snake pieces (unused)
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20
	1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1,  //13
	1, 1, 1, 1, //doors
	1, 1, 1, 1, 2, 1, 1,  //7
	1, 1, 1, //bridges
	1, 1, 1, 1, //image/lights
	1, 1, 1, 1, //shallow water, horns, stones
	1, 1, 1, 1, //seeding beacons, powder keg, floor spikes
	1, 1, 1, 1, 1, 1, 1, 1,  //8 disabled arrows
	1, 2, 1, 1, 1, 1, 1, 1,  //overhead image, fluff, firetrap
	1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //M+25
	1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, //M+19
	2, 1, 1 //special
};

const UINT wItemY[TOTAL_EDIT_TILE_COUNT] = {
	1, 1, 1, 2, 2, 2,  //6
	1, 1, 1, 1, 1,  //doors
	1, 1,  //2
	1, 1, 1, 1, 1, 1, 1, 1,  //8 arrows
	1, 1, 1, 1,  // 4
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10 snake pieces (unused)
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20
	1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 2, 1,  //13
	1, 1, 1, 1,  //doors
	1, 1, 1, 1, 2, 1, 1,  //7
	1, 1, 1, //bridges
	1, 1, 1, 1, //image/lights
	1, 1, 1, 1, //shallow water, horns, stones
	1, 1, 1, 1, //seeding beacons, powder keg, floor spikes
	1, 1, 1, 1, 1, 1, 1, 1,  //8 disabled arrows
	1, 2, 1, 1, 1, 1, 1, 1, //overhead image, fluff, firetrap
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //M+25
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, //M+19
	1, 1, 1 //special
};

const UINT ObstacleDisplayTiles[MAX_OBSTACLE_TYPES] =
{
	0, TI_OB_1_1, TI_OB_2_1, TI_OB_3_1, TI_OB_4_1, TI_OB_5_1,
	TI_OB_6_1, TI_OB_7_1, TI_OB_8_1, TI_OB_9_1, TI_OB_10_1, TI_OB_11_1, TI_OB_12_1,
	TI_OB_13_1, TI_OB_14_1, TI_OB_15_1, TI_OB_16_1,
	TI_OB_17_1, TI_OB_18_1, TI_OB_19_1, TI_OB_20_1,	TI_OB_21_1, TI_OB_22_1,
	TI_OB_23_1, TI_OB_24_1, TI_OB_25_1, TI_OB_26_1, TI_OB_27_1, TI_OB_28_1,
	TI_OB_29_1, TI_OB_30_1, TI_OB_31_1, TI_OB_32_1
};

const UINT TokenDisplayTiles[RoomTokenCount] =
{
	TI_TOKEN_CW, TI_TOKEN_CCW, TI_TOKEN_TARMUD, TI_TOKEN_TARGEL, TI_TOKEN_GELMUD,
	TI_TOKEN_TAROFF, TI_TOKEN_PWR_ON, TI_TOKEN_WEAPON_OFF, TI_TOKEN_CITIZEN, TI_TOKEN_CONQUER_ON,
	TI_TOKEN_SWORD, TI_TOKEN_PICKAXE, TI_TOKEN_SPEAR, TI_TOKEN_STAFF, TI_TOKEN_DAGGER, TI_TOKEN_CABER,
	TI_TEMPORAL_SPLIT_ON
};

const UINT NEIGHBORS = 4;
const int dx[NEIGHBORS] = {-1, 0, 1, 0};
const int dy[NEIGHBORS] = {0, -1, 0, 1};

#define O_LAYER_TAB (0)
#define F_LAYER_TAB (1)
#define T_LAYER_TAB (2)
#define MONSTER_TAB (3)

const UINT numOLayerGroupedEntries = 21;
const UINT oLayerGroupedEntries[numOLayerGroupedEntries] = {
	T_WALL, T_WALL_B, T_STAIRS,
	T_DOOR_C, T_DOOR_M, T_DOOR_R,
	T_DOOR_B, T_DOOR_Y, T_PIT,
	T_WATER, T_THINICE, T_WALL_M, T_TUNNEL_E,
	T_PRESSPLATE, T_GOO, T_HOT, T_FLUFFVENT,
	T_FLOOR_SPIKES, T_BRIDGE, T_FLOOR, T_FLOOR_IMAGE
};

const UINT numOLayerFullEntries = 42;
const UINT oLayerFullEntries[numOLayerFullEntries] = {
	T_WALL, T_WALL_B, T_STAIRS,
	T_DOOR_C, T_DOOR_M, T_DOOR_R, T_DOOR_B,
	T_DOOR_CO, T_DOOR_GO, T_DOOR_RO, T_DOOR_BO,
	T_DOOR_Y, T_DOOR_YO, T_WALL_M, T_PRESSPLATE,
	T_TRAPDOOR, T_PIT, T_PLATFORM_P, T_TUNNEL_E,
	T_TRAPDOOR2, T_WATER, T_PLATFORM_W, T_THINICE,
	T_BRIDGE, T_BRIDGE_H, T_BRIDGE_V, T_GOO,
	T_FLOOR, T_FLOOR_M, T_FLOOR_ROAD, T_HOT,
	T_FLOOR_GRASS, T_FLOOR_DIRT, T_FLOOR_ALT, T_FLUFFVENT,
	T_FLOOR_SPIKES, T_FIRETRAP, T_FIRETRAP_ON,
	T_FLOOR_IMAGE, T_WALL_IMAGE, T_PIT_IMAGE, T_OVERHEAD_IMAGE
};

const UINT numFLayerEntries = 8;
const UINT fLayerEntries[numFLayerEntries] = {
	T_ARROW_NE, T_ARROW_OFF_NE, T_NODIAGONAL, T_CHECKPOINT,
	T_WALLLIGHT, T_LIGHT_CEILING, T_DARK_CEILING, T_SWORDSMAN
};

const UINT numTLayerGroupedEntries = 17;
const UINT tLayerGroupedEntries[numTLayerGroupedEntries] = {
	T_ORB, T_POTION_K, T_SCROLL, T_BOMB,
	T_FUSE, T_BRIAR_SOURCE, T_BRIAR_LIVE, T_BRIAR_DEAD,
	T_OBSTACLE, T_MIRROR, T_TOKEN, T_STATION,
	T_LIGHT, T_BEACON, T_POWDER_KEG, T_TAR,
	T_FLUFF
};

const UINT numTLayerFullEntries = 26;
const UINT tLayerFullEntries[numTLayerFullEntries] = {
	T_ORB, T_POTION_K, T_POTION_I, T_POTION_D,
	T_POTION_C, T_POTION_SP, T_HORN_SQUAD, T_HORN_SOLDIER,
	T_SCROLL, T_BOMB, T_FUSE, T_OBSTACLE,
	T_BRIAR_SOURCE, T_BRIAR_LIVE, T_BRIAR_DEAD, T_MIRROR,
	T_TOKEN, T_STATION, T_LIGHT, T_POWDER_KEG,
	T_BEACON, T_BEACON_OFF,
	T_TAR, T_MUD, T_GEL,
	T_FLUFF
};

const UINT numMLayerEntries = 36;
const UINT mLayerEntries[numMLayerEntries] = {
	T_ROACH, T_QROACH, T_EYE, T_WWING,
	T_SPIDER, T_GOBLIN, T_BRAIN, T_ROCKGOLEM,
	T_WUBBA, T_TARBABY, T_MUDBABY, T_GELBABY,
	T_SERPENTB, T_TARMOTHER,
	T_SEEP,	T_GUARD, T_SLAYER, T_SLAYER2,
	T_HALPH, T_HALPH2, T_WATERSKIPPER, T_SKIPPERNEST,
	T_AUMTLICH, T_FEGUNDO, T_FLUFFBABY, T_CLONE,
	T_MIMIC, T_DECOY, T_STALWART, T_STALWART2,
	T_ROCKGIANT, T_CITIZEN, T_ARCHITECT,
	T_GENTRYII, T_CONSTRUCT, T_CHARACTER
};

//Ordering of obstacle types on pop-up menu.
const UINT numObstacles = 32;
const UINT mObstacleEntries[numObstacles] = {
	 1,  2,  3,  4, 28,  5,  6, 25,
	 9, 10,  7,  8, 19, 20, 17, 26,
	18, 24, 21, 29, 30, 22, 23, 11,
	12, 31, 13, 14, 15, 16, 27, 32
};

const UINT wLightMult = 2, wDarkMult = 25;

const int X_ROOM = 163;
const int Y_ROOM = 40;
const UINT CX_SPACE = 12;
const UINT CY_SPACE = 12;

//*****************************************************************************
#define AddObjectToMenu(o) pObjectMenu->AddObject((o), wItemX[o], wItemY[o], MenuDisplayTiles[o])

//*****************************************************************************
bool bIsLightingTile(const UINT wT)
//Returns: true if this tile is a light or dark tile type
{
	switch (wT)
	{
		case T_WALLLIGHT: case T_LIGHT_CEILING: case T_DARK_CEILING:
			return true;
		default:
			return bIsLight(wT);
	}
}

//*****************************************************************************
UINT GetItemMenuWidth(UINT numTiles)
{
	return 8 + //edge
		numTiles * CBitmapManager::CX_TILE + //tiles
		(numTiles-1) * 10; //buffer
}
UINT GetItemMenuHeight(UINT numTiles)
{
	return 8 + //edge
		numTiles * CBitmapManager::CY_TILE + //tiles
		(numTiles-1) * 10; //buffer
}

//
//Public methods.
//

//*****************************************************************************
CEditRoomScreen::~CEditRoomScreen()
{
	ResetMembers();
}

//*****************************************************************************
bool CEditRoomScreen::SetRoom(
//Instantiates level and room members by room ID.
//These instances are deleted during call to Unload() upon destruction.
//
//Returns: whether room was successfully loaded
//
//Params:
	const UINT dwRoomID,   //(in) ID of room to load.
	const bool bQuick)  //if set, don't load room data fully
{
	delete this->pRoom;
	this->bRoomDirty = false;
	this->pRoom = dwRoomID ? g_pTheDB->Rooms.GetByID(dwRoomID, bQuick) : NULL;
	if (!this->pRoom)
		return false;
	if (!bQuick)
	{
		this->pRoom->PlaceCharacters(this->pHold);
		this->pRoomWidget->LoadFromRoom(this->pRoom, this->pHold, &this->LevelEntrances);
	}

	delete this->pLevel;
	this->pLevel = this->pRoom->GetLevel();
	ASSERT(this->pLevel);
	this->pMapWidget->LoadFromLevel(this->pLevel);
	this->pMapWidget->SelectRoom(this->pRoom->dwRoomX, this->pRoom->dwRoomY);
	delete this->pHold;
	this->pHold = this->pLevel->GetHold();
	ASSERT(this->pHold);

	if (!bQuick)
		GetLevelEntrancesInRoom();

	//Reset.
	ResetAdjacentRooms();
	ClearList(this->redoList);
	ClearList(this->undoList);
	SetButtons(false);

	SetSignTextToCurrentRoom();

	return true;
}

//
//Protected methods.
//

//*****************************************************************************
CEditRoomScreen::CEditRoomScreen()
	: CRoomScreen(SCR_EditRoom)
	, pHold(NULL), pLevel(NULL), pRoom(NULL)
	, pRoomWidget(NULL), pTabbedMenu(NULL)
	, pCharacterDialog(NULL)
	, pEntranceBox(NULL), pLevelEntranceDialog(NULL)

	, wSelectedObject(static_cast<UINT>(-1))
	, wSelectedObjectSave(static_cast<UINT>(-1))

	, eState(ES_PLACING)
	, pLongMonster(NULL)
	, pOrb(NULL)
	, bShowErrors(true)
	, bAutoSave(true)
	, bRoomDirty(false)
	, nUndoSize(0)
	, bPaintItemText(false)
	, bGroupMenuItems(false)

	, wTestX(static_cast<UINT>(-1)), wTestY(static_cast<UINT>(-1))
	, wTestO(static_cast<UINT>(-1))
	, wO(SE)
	, wSelectedObType(1) //first valid obstacle type
	, wSelectedLightType(0)
	, wSelectedDarkType(NUM_DARK_TYPES-1)
	, wSelOrbType(OT_NORMAL), wSelPlateType(OT_NORMAL)
	, wSelTokenType(RotateArrowsCW)
	, wSelWaterType(T_WATER)
	, wLastFloorSelected(T_FLOOR)
	, wLastEntranceSelected(static_cast<UINT>(-1))
	, bSelectingImageStart(false)

	, pCopyRoom(NULL)
	, wCopyX1(static_cast<UINT>(-1)), wCopyY1(static_cast<UINT>(-1))
	, wCopyX2(static_cast<UINT>(-1)), wCopyY2(static_cast<UINT>(-1))
	, bAreaJustCopied(false), bCutAndPaste(false)

	, dwSavePlayerID(0), dwTestPlayerID(0)
{
	this->pAdjRoom[0] = this->pAdjRoom[1] = this->pAdjRoom[2] = this->pAdjRoom[3] = NULL;

	static const int X_OBJECTMENU = 6;
	static const int Y_OBJECTMENU = 4;
	static const UINT CX_OBJECTMENU = 150;
	static const UINT CY_OBJECTMENU = 490;
	static const int X_INNERMENU = 6;
	static const int Y_INNERMENU = 33;
	static const UINT CX_INNERMENU = CX_OBJECTMENU - X_INNERMENU;
	static const UINT CY_ITEMTEXT = 52;
	static const UINT CY_INNERMENU = CY_OBJECTMENU - Y_INNERMENU - 5 - CY_ITEMTEXT;
	static const UINT CX_MENUSPACE = 10;
	static const UINT CY_MENUSPACE = CX_MENUSPACE;

	static const int X_OBSMENU = X_OBJECTMENU + CX_OBJECTMENU;
	static const int Y_OBSMENU = Y_ROOM + 30;
	static const UINT CX_OBSMENU = GetItemMenuWidth(8);
	static const UINT CY_OBSMENU = GetItemMenuHeight(4);

	static const int X_LIGHTMENU = X_OBSMENU;
	static const int Y_LIGHTMENU = Y_OBSMENU;
	static const UINT CX_LIGHTMENU = GetItemMenuWidth(4);
	static const UINT CY_LIGHTMENU = GetItemMenuHeight(4);

	static const UINT CX_DARKMENU = GetItemMenuWidth(8);
	static const UINT CY_DARKMENU = GetItemMenuHeight(4);

	static const int X_FLOORMENU = X_OBSMENU;
	static const int Y_FLOORMENU = Y_OBSMENU + 82;
	static const UINT CX_FLOORMENU = GetItemMenuWidth(3);
	static const UINT CY_FLOORMENU = GetItemMenuHeight(2);

	static const int X_TOKENMENU = X_OBSMENU;
	static const int Y_TOKENMENU = Y_OBSMENU + 35;
	static const UINT CX_TOKENMENU = GetItemMenuWidth(6);
	static const UINT CY_TOKENMENU = GetItemMenuHeight(3);

	static const int X_POTIONMENU = X_OBSMENU;
	static const int Y_POTIONMENU = Y_OBSMENU;
	static const UINT CX_POTIONMENU = GetItemMenuWidth(4);
	static const UINT CY_POTIONMENU = GetItemMenuHeight(2);

	static const UINT CX_ITEMTEXT = CX_INNERMENU - 2;
	static const int X_ITEMTEXT = 6;
	static const int Y_ITEMTEXT = Y_INNERMENU + CY_INNERMENU;

	static const int X_UNDO = CX_SPACE;
	static const int Y_UNDO = Y_OBJECTMENU + CY_OBJECTMENU + CY_SPACE;
	static const UINT CX_UNDO = 62;
	static const UINT CY_UNDO = CY_STANDARD_BUTTON;
	static const int X_REDO = X_UNDO + CX_UNDO + CX_SPACE;
	static const int Y_REDO = Y_UNDO;
	static const UINT CX_REDO = CX_UNDO;
	static const int X_SHOWERRORS = CX_SPACE;

#ifdef RUSSIAN_BUILD
	static const int Y_SHOWERRORS = Y_UNDO + CY_UNDO + 2;

	static const int X_ESC = 15;
	static const int Y_ESC = 718;
	static const UINT CX_ESC = 150;
	static const UINT CY_ESC = 30;

	static const int X_HELP = 30;
	static const int Y_HELP = 740;
	static const UINT CX_HELP = CX_ESC;
	static const UINT CY_HELP = 30;
#else
	static const int Y_SHOWERRORS = Y_UNDO + CY_STANDARD_BUTTON;

	static const int X_ESC = 6;
	static const int Y_ESC = 726;
	static const UINT CX_ESC = 86;
	static const UINT CY_ESC = 32;

	static const int X_HELP = 96;
	static const int Y_HELP = Y_ESC;
	static const UINT CX_HELP = 64;
	static const UINT CY_HELP = 32;
#endif

	this->pMapWidget->Enable();

	//Add widgets.
	this->pRoomWidget = new CEditRoomWidget(TAG_ROOM, X_ROOM, Y_ROOM,
			CDrodBitmapManager::CX_ROOM, CDrodBitmapManager::CY_ROOM);
	if (!this->pRoomWidget) throw CException("CEditRoomScreen: Couldn't allocate resources");
	AddWidget(this->pRoomWidget);

	//Object menu.
	this->pTabbedMenu = new CTabbedMenuWidget(TAG_MENU, X_OBJECTMENU,
			Y_OBJECTMENU, CX_OBJECTMENU, CY_OBJECTMENU, 4,
			static_cast<UINT>(CBitmapManager::CY_TILE * 1.4), BG_COLOR);
	if (!this->pTabbedMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	this->pTabbedMenu->SetTabTile(O_LAYER_TAB, TI_WALL);
	this->pTabbedMenu->SetTabTile(F_LAYER_TAB, TI_CHECKPOINT);
	this->pTabbedMenu->SetTabTile(T_LAYER_TAB, TI_ORB_D);
	this->pTabbedMenu->SetTabTile(MONSTER_TAB, TI_ROACH_SE);
	AddWidget(this->pTabbedMenu);

	//Add object menus to tabbed menu
	CObjectMenuWidget *pObjectMenu;
	pObjectMenu = new CObjectMenuWidget(TAG_OMENU, X_INNERMENU, Y_INNERMENU,
			CX_INNERMENU, CY_INNERMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	this->pTabbedMenu->AddWidgetToTab(pObjectMenu,O_LAYER_TAB);

	pObjectMenu = new CObjectMenuWidget(TAG_FMENU, X_INNERMENU, Y_INNERMENU,
			CX_INNERMENU, CY_INNERMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	this->pTabbedMenu->AddWidgetToTab(pObjectMenu,F_LAYER_TAB);

	pObjectMenu = new CObjectMenuWidget(TAG_TMENU, X_INNERMENU, Y_INNERMENU,
			CX_INNERMENU, CY_INNERMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	this->pTabbedMenu->AddWidgetToTab(pObjectMenu,T_LAYER_TAB);

	pObjectMenu = new CObjectMenuWidget(TAG_MMENU, X_INNERMENU, Y_INNERMENU,
			CX_INNERMENU, CY_INNERMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	this->pTabbedMenu->AddWidgetToTab(pObjectMenu,MONSTER_TAB);

	//Obstacle scenery pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_OBS_MENU, X_OBSMENU, Y_OBSMENU,
			CX_OBSMENU, CY_OBSMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	for (UINT ob=0; ob<numObstacles; ++ob)
		pObjectMenu->AddObject(mObstacleEntries[ob], 1, 1, ObstacleDisplayTiles + mObstacleEntries[ob]);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);
	
	//Light palette pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_LIGHT_MENU, X_LIGHTMENU, Y_LIGHTMENU,
			CX_LIGHTMENU, CY_LIGHTMENU, CX_MENUSPACE, CY_MENUSPACE, 0); //black BG
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	UINT light;
	for (light=0; light<NUM_LIGHT_TYPES; ++light)
		pObjectMenu->AddObject(light, 1, 1, MenuDisplayTiles[T_LIGHT_CEILING],
				wLightMult*lightMap[0][light], wLightMult*lightMap[1][light], wLightMult*lightMap[2][light]);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);

	//Dark palette pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_DARK_MENU, X_LIGHTMENU, Y_LIGHTMENU,
			CX_DARKMENU, CY_DARKMENU, CX_MENUSPACE, CY_MENUSPACE, 0); //black BG
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	for (light=0; light<NUM_DARK_TYPES; ++light)
		pObjectMenu->AddObject(light, 1, 1, MenuDisplayTiles[T_DARK_CEILING],
				wDarkMult*darkMap[light], wDarkMult*darkMap[light], wDarkMult*darkMap[light]);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);

	//Floor texture pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_FLOOR_MENU, X_FLOORMENU, Y_FLOORMENU,
			CX_FLOORMENU, CY_FLOORMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	pObjectMenu->AddObject(T_FLOOR, 1, 1, MenuDisplayTiles[T_FLOOR]);
	pObjectMenu->AddObject(T_FLOOR_M, 1, 1, MenuDisplayTiles[T_FLOOR_M]);
	pObjectMenu->AddObject(T_FLOOR_ROAD, 1, 1, MenuDisplayTiles[T_FLOOR_ROAD]);
	pObjectMenu->AddObject(T_FLOOR_GRASS, 1, 1, MenuDisplayTiles[T_FLOOR_GRASS]);
	pObjectMenu->AddObject(T_FLOOR_DIRT, 1, 1, MenuDisplayTiles[T_FLOOR_DIRT]);
	pObjectMenu->AddObject(T_FLOOR_ALT, 1, 1, MenuDisplayTiles[T_FLOOR_ALT]);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);

	//Token texture pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_TOKEN_MENU, X_TOKENMENU, Y_TOKENMENU,
			CX_TOKENMENU, CY_TOKENMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	for (UINT token=0; token<RoomTokenCount; ++token)
		pObjectMenu->AddObject(token, 1, 1, TokenDisplayTiles + token);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);

	//Potion pop-up menu.
	pObjectMenu = new CObjectMenuWidget(TAG_POTION_MENU, X_POTIONMENU, Y_POTIONMENU,
			CX_POTIONMENU, CY_POTIONMENU, CX_MENUSPACE, CY_MENUSPACE, MenuBGColor);
	if (!pObjectMenu) throw CException("CEditRoomScreen: Couldn't allocate resources");
	pObjectMenu->DrawBackground(true);
	AddObjectToMenu(T_POTION_K);
	AddObjectToMenu(T_POTION_I);
	AddObjectToMenu(T_POTION_D);
	AddObjectToMenu(T_POTION_C);
	AddObjectToMenu(T_POTION_SP);
	AddObjectToMenu(T_HORN_SQUAD);
	AddObjectToMenu(T_HORN_SOLDIER);
	pObjectMenu->Hide();
	AddWidget(pObjectMenu);

	CLabelWidget *pItemLabel = new CLabelWidget(TAB_ITEMTEXT,
			X_ITEMTEXT, Y_ITEMTEXT, CX_ITEMTEXT, CY_ITEMTEXT, F_Small, wszEmpty);
	this->pTabbedMenu->AddWidget(pItemLabel);

	//Undo/redo buttons.
	CButtonWidget *pButton = new CButtonWidget(TAG_UNDO, X_UNDO, Y_UNDO,
			CX_UNDO, CY_UNDO, g_pTheDB->GetMessageText(MID_Undo));
	AddWidget(pButton);
	pButton = new CButtonWidget(TAG_REDO, X_REDO, Y_REDO,
			CX_REDO, CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_Redo));
	AddWidget(pButton);

	//Option settings.
	AddWidget(new COptionButtonWidget(TAG_SHOWERRORS, X_SHOWERRORS, Y_SHOWERRORS,
			CX_OBJECTMENU, CY_STANDARD_OPTIONBUTTON,
			g_pTheDB->GetMessageText(MID_ShowErrors), this->bShowErrors, true));

	CLabelWidget *pLabel = new CLabelWidget(TAG_ESC, X_ESC, Y_ESC, CX_ESC, CY_ESC, F_ButtonWhite,
				g_pTheDB->GetMessageText(MID_EscBack));
	pLabel->SetClickable(true);
	AddWidget(pLabel);

	pLabel = new CLabelWidget(TAG_HELP, X_HELP, Y_HELP, CX_HELP, CY_HELP, F_ButtonWhite,
				g_pTheDB->GetMessageText(MID_F1Help));
	pLabel->SetClickable(true);
	AddWidget(pLabel);

	//Level list dialog box.
	this->pEntranceBox = new CEntranceSelectDialogWidget(0L);
	AddWidget(this->pEntranceBox);
	this->pEntranceBox->Move(
		X_ROOM + (CDrodBitmapManager::CX_ROOM - this->pEntranceBox->GetW()) / 2,
		Y_ROOM + (CDrodBitmapManager::CY_ROOM - this->pEntranceBox->GetH()) / 2);   //center over room widget
	this->pEntranceBox->Hide();

	//Character monster customization box.
	this->pCharacterDialog = new CCharacterDialogWidget(0L);
   AddWidget(this->pCharacterDialog);
	this->pCharacterDialog->Move(
		X_ROOM + (CDrodBitmapManager::CX_ROOM - this->pCharacterDialog->GetW()) / 2,
		Y_ROOM + (CDrodBitmapManager::CY_ROOM - this->pCharacterDialog->GetH()) / 2);   //center over room widget
	this->pCharacterDialog->Hide();

	AddChatDialog();
	AddLevelEntranceDialog();

	PopulateItemMenus();
}

//*****************************************************************************
void CEditRoomScreen::AddChatDialog()
//Show dialog box for CaravelNet chat.
{
	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 12;

	static const UINT CX_DIALOG = 600;
	static const UINT CY_DIALOG = 415;

	static const int Y_HEADER = 15;
	static const int X_HEADER = 20;
	static const UINT CX_HEADER = CX_DIALOG - 2*X_HEADER;
	static const UINT CY_HEADER = 30;

	static const int X_CHATOPTION = X_HEADER;
	static const int Y_CHATOPTION = Y_HEADER + CY_HEADER + CY_STANDARD_OPTIONBUTTON + CY_SPACE;
	static const UINT CY_CHATOPTION = CY_STANDARD_OPTIONBUTTON;
	static const UINT CX_CHATOPTION = 280;
	static const int X_CHATWHISPEROPTION = X_CHATOPTION;
	static const int Y_CHATWHISPEROPTION = Y_CHATOPTION + CY_CHATOPTION + CY_SPACE;
	static const UINT CX_CHATWHISPEROPTION = CX_CHATOPTION + 5;

	static const int Y_OKAY_BUTTON = CY_DIALOG - CY_STANDARD_BUTTON - CY_SPACE*3/2;
	static const int Y_CHAT = Y_OKAY_BUTTON - CY_STANDARD_TBOX - CY_SPACE;

	static const UINT CY_USERLISTLABEL = 27;
	static const int X_USERLIST = X_CHATOPTION + CX_CHATOPTION + CX_SPACE;
	static const int Y_USERLISTLABEL = Y_CHATOPTION - CY_USERLISTLABEL;
	static const int Y_USERLIST = Y_USERLISTLABEL + CY_USERLISTLABEL;
	static const UINT CX_USERLIST = CX_DIALOG - X_USERLIST - X_HEADER;
	static const UINT CY_USERLIST = 10*22 + 4;

	static const UINT CX_OK_BUTTON = 80;

	CDialogWidget *pStatsBox = new CDialogWidget(TAG_CHATBOX, 0, 0, CX_DIALOG, CY_DIALOG);

	CLabelWidget *pLabel = new CLabelWidget(0L, X_HEADER, Y_HEADER, CX_HEADER,
			CY_HEADER, F_Header, g_pTheDB->GetMessageText(MID_ChatTitle));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pStatsBox->AddWidget(pLabel);

	//Chat.
	pStatsBox->AddWidget(new COptionButtonWidget(TAG_CHATENABLE, X_CHATOPTION, Y_CHATOPTION,
			CX_CHATOPTION, CY_CHATOPTION, g_pTheDB->GetMessageText(MID_ChatEnableOnGameScreen)));
	pStatsBox->AddWidget(new COptionButtonWidget(TAG_CHATWHISPERSONLY, X_CHATWHISPEROPTION, Y_CHATWHISPEROPTION,
			CX_CHATWHISPEROPTION, CY_CHATOPTION, g_pTheDB->GetMessageText(MID_ReceiveWhispersOnly)));
	pStatsBox->AddWidget(new CTextBoxWidget(TAG_CHATINPUT, X_CHATOPTION, Y_CHAT,
			CX_DIALOG - X_CHATOPTION*2, CY_STANDARD_TBOX));
	pStatsBox->AddWidget(new CLabelWidget(0, X_USERLIST, Y_USERLISTLABEL,
			CX_USERLIST, CY_USERLISTLABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_PlayersChatting)));
	pStatsBox->AddWidget(new CListBoxWidget(TAG_CHATUSERS, X_USERLIST, Y_USERLIST,
			CX_USERLIST, CY_USERLIST, false, false, true));

	//Buttons.
	CButtonWidget *pButton = new CButtonWidget(TAG_OK,
			(CX_DIALOG - CX_OK_BUTTON) / 2, Y_OKAY_BUTTON,
			CX_OK_BUTTON, CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	pStatsBox->AddWidget(pButton);

	this->pRoomWidget->AddWidget(pStatsBox,true);
	pStatsBox->Center();
	pStatsBox->Hide();
}

//*****************************************************************************
void CEditRoomScreen::AddLevelEntranceDialog()
//Dialog box for defining level entrances.
{
	static const UINT CX_ENTRANCE_DIALOG = 650;
	static const UINT CY_ENTRANCE_DIALOG = 592;

	static const int FRAME_BUFFER = 5;
	static const int X_TEXT = CX_SPACE;
	static const int Y_TEXT = CY_SPACE;
	static const UINT CX_TEXT = CX_ENTRANCE_DIALOG - (CX_SPACE * 2);
	static const UINT CY_TEXT = 35;
	static const int Y_TEXTBOX = Y_TEXT + CY_TEXT + FRAME_BUFFER * 2;
	static const UINT CX_TEXTBOX = CX_ENTRANCE_DIALOG - X_TEXT - CX_SPACE;
	static const UINT CY_TEXTBOX = CY_STANDARD_TBOX2D;
	static const int Y_SHOWDESCRIPTION = Y_TEXTBOX + CY_TEXTBOX;
	static const int Y_SHOWDESCRIPTION_ONCE = Y_SHOWDESCRIPTION + CY_STANDARD_OPTIONBUTTON;
	static const int Y_MAINENTRANCE = Y_SHOWDESCRIPTION_ONCE + CY_STANDARD_OPTIONBUTTON;
	static const UINT CY_MAINENTRANCE = CY_STANDARD_OPTIONBUTTON;

	static const int X_ENTRANCEAUDIO = X_TEXT;
	static const int Y_ENTRANCEAUDIO = Y_MAINENTRANCE + CY_MAINENTRANCE + CY_SPACE;
	static const UINT CX_ENTRANCEAUDIO = 130;
	static const UINT CY_ENTRANCEAUDIO = CY_STANDARD_BUTTON;

	static const int X_SOUNDNAMELABEL = X_ENTRANCEAUDIO + CX_ENTRANCEAUDIO + CX_SPACE;
	static const int Y_SOUNDNAMELABEL = Y_ENTRANCEAUDIO;
	static const UINT CX_SOUNDNAMELABEL = CX_ENTRANCE_DIALOG - X_SOUNDNAMELABEL - CX_SPACE;
	static const UINT CY_SOUNDNAMELABEL = CY_TEXT;

	static const UINT CX_BUTTON = 80;
	static const UINT CY_BUTTON = CY_STANDARD_BUTTON;
	static const int X_OK_ENTRANCE = CX_ENTRANCE_DIALOG/2 - CX_SPACE - CX_BUTTON;
	static const int X_CANCEL = X_OK_ENTRANCE + CX_BUTTON + CX_SPACE;
	static const int Y_BUTTONS = CY_ENTRANCE_DIALOG - CY_BUTTON - CY_SPACE;

	this->pLevelEntranceDialog = new CDialogWidget(0L, 0, 0, CX_ENTRANCE_DIALOG,
			CY_ENTRANCE_DIALOG);
	AddWidget(this->pLevelEntranceDialog);
	this->pLevelEntranceDialog->Move(
		X_ROOM + (CDrodBitmapManager::CX_ROOM - this->pLevelEntranceDialog->GetW()) / 2,
		Y_ROOM + (CDrodBitmapManager::CY_ROOM - this->pLevelEntranceDialog->GetH()) / 2);   //center over room widget
	this->pLevelEntranceDialog->Hide();

	CLabelWidget *pLabel = new CLabelWidget(0, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT, FONTLIB::F_Message, g_pTheDB->GetMessageText(MID_DescribeLevel));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pLevelEntranceDialog->AddWidget(pLabel);

	CFrameWidget *pFrame = new CFrameWidget(0, X_TEXT - FRAME_BUFFER, Y_TEXT - FRAME_BUFFER,
			CX_TEXT + FRAME_BUFFER*2, CY_TEXT + FRAME_BUFFER*2, NULL);
	this->pLevelEntranceDialog->AddWidget(pFrame);

	CTextBox2DWidget *pTextBox2D = new CTextBox2DWidget(TAG_LEVELENTRANCETEXTBOX,
			X_TEXT, Y_TEXTBOX, CX_TEXTBOX, CY_TEXTBOX, 1350);
	this->pLevelEntranceDialog->AddWidget(pTextBox2D);
	this->pLevelEntranceDialog->SetEnterText(TAG_LEVELENTRANCETEXTBOX);

	COptionButtonWidget *pOpButton = new COptionButtonWidget(TAG_SHOWDESCRIPTION,
			X_TEXT, Y_SHOWDESCRIPTION, CX_ENTRANCE_DIALOG, CY_STANDARD_OPTIONBUTTON,
			g_pTheDB->GetMessageText(MID_ShowEntranceDesc));
	pLevelEntranceDialog->AddWidget(pOpButton);
	pOpButton = new COptionButtonWidget(TAG_SHOWDESCRIPTION_ONCE,
			X_TEXT, Y_SHOWDESCRIPTION_ONCE, CX_ENTRANCE_DIALOG, CY_STANDARD_OPTIONBUTTON,
			g_pTheDB->GetMessageText(MID_ShowEntranceDescOnce));
	pLevelEntranceDialog->AddWidget(pOpButton);
	pOpButton = new COptionButtonWidget(TAG_MAINENTRANCE,
			X_TEXT, Y_MAINENTRANCE, CX_ENTRANCE_DIALOG, CY_MAINENTRANCE,
			g_pTheDB->GetMessageText(MID_MakeEntranceMain));
	pLevelEntranceDialog->AddWidget(pOpButton);

	CButtonWidget *pButton = new CButtonWidget(TAG_LEVELENTRANCEAUDIO,
		X_ENTRANCEAUDIO, Y_ENTRANCEAUDIO, CX_ENTRANCEAUDIO, CY_ENTRANCEAUDIO,
		g_pTheDB->GetMessageText(MID_AddSound));
	pLevelEntranceDialog->AddWidget(pButton);
	pLevelEntranceDialog->AddWidget(new CLabelWidget(TAG_LEVELENTRANCEAUDIONAME, X_SOUNDNAMELABEL,
			Y_SOUNDNAMELABEL, CX_SOUNDNAMELABEL, CY_SOUNDNAMELABEL, F_Small, wszEmpty));

	this->pLevelEntranceDialog->AddWidget(new CButtonWidget(TAG_OK, X_OK_ENTRANCE, Y_BUTTONS,
			CX_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_OkayNoHotkey)));
	this->pLevelEntranceDialog->AddWidget(new CButtonWidget(TAG_CANCEL_, X_CANCEL, Y_BUTTONS,
			CX_BUTTON, CY_BUTTON, g_pTheDB->GetMessageText(MID_CancelNoHotkey)));
}

//*****************************************************************************
void CEditRoomScreen::ApplyINISettings()
//(Re)query the INI for current values and apply them.
{
	CDrodScreen::ApplyINISettings();

	ForceFullStyleReload();
}

//*****************************************************************************
void CEditRoomScreen::ForceFullStyleReload()
//Force full style reload and rerender room with current settings.
{
	g_pTheDBM->LoadTileImagesForStyle(this->pRoom->style, true);
	this->pRoomWidget->LoadRoomImages();
	Paint();
}

//*****************************************************************************
void CEditRoomScreen::ResetAdjacentRooms()
{
	//Reset adjacent rooms used for error checking.
	delete this->pAdjRoom[0];
	delete this->pAdjRoom[1];
	delete this->pAdjRoom[2];
	delete this->pAdjRoom[3];

	this->pAdjRoom[0] = this->pAdjRoom[1] = this->pAdjRoom[2] = this->pAdjRoom[3] = NULL;
}

//*****************************************************************************
void CEditRoomScreen::ResetMembers()
//Reset state on screen exit.
{
	delete this->pHold;
	this->pHold = NULL;

	delete this->pLevel;
	this->pLevel = NULL;

	delete this->pRoom;
	this->pRoom = NULL;

	ClearList(this->undoList);
	ClearList(this->redoList);
	GetLevelEntrancesInRoom(); //will clear list
	ResetAdjacentRooms();

	delete this->pCopyRoom;
	this->pCopyRoom = NULL;
}

//*****************************************************************************
bool CEditRoomScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	SetCursor(CUR_Wait);

	ASSERT(this->pRoom);

	//Ensure everything's current and fresh.
	this->pRoom->Reload();
	CRoomScreen::SetMusicStyle(this->pRoom->style, SONG_EDITOR); //do after reload
	g_pTheDBM->LoadTileImagesForStyle(this->pRoom->style);
	if (!this->pRoomWidget->LoadFromRoom(this->pRoom, this->pHold, &this->LevelEntrances))
		return false; //couldn't load image resources for room
	this->pRoom->PlaceCharacters(this->pHold);
	GetLevelEntrancesInRoom();
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
	this->bRoomDirty = false;
	SetLightLevel();

	//If returning from play-testing, delete the temp player.
	if (this->dwTestPlayerID)
	{
		//Retain user settings possibly changed during play-testing.
		CDbPlayer *pCurPlayer = g_pTheDB->Players.GetByID(this->dwTestPlayerID);
		ASSERT(pCurPlayer);
		CDbPackedVars& settings = pCurPlayer->Settings;
		const bool bMoveCounter = settings.GetVar(Settings::MoveCounter, false);
		const bool bPuzzleMode = settings.GetVar(Settings::PuzzleMode, false);
		const bool bShowChats = settings.GetVar(Settings::EnableChatInGame, false);
		const bool bReceiveWhispersOnly = settings.GetVar(Settings::ReceiveWhispersOnlyInGame, false);
		const bool bAutoUndoOnDeath = settings.GetVar(Settings::AutoUndoOnDeath, false);
		const UINT dwLastNotice = settings.GetVar(Settings::LastNotice, 0);

		delete pCurPlayer;

		ASSERT(this->dwSavePlayerID);
		g_pTheDB->Players.Delete(this->dwTestPlayerID, false);
		g_pTheDB->SetPlayerID(this->dwSavePlayerID, false); //saved player doesn't need to relogin to CaravelNet
		this->dwSavePlayerID = this->dwTestPlayerID = 0;

		pCurPlayer = g_pTheDB->GetCurrentPlayer();
		ASSERT(pCurPlayer);
		CDbPackedVars& baseSettings = pCurPlayer->Settings;
		const bool bChangedSettings =
			bMoveCounter != baseSettings.GetVar(Settings::MoveCounter, false) ||
			bPuzzleMode != baseSettings.GetVar(Settings::PuzzleMode, false) ||
			bShowChats != baseSettings.GetVar(Settings::EnableChatInGame, false) ||
			bReceiveWhispersOnly != baseSettings.GetVar(Settings::ReceiveWhispersOnlyInGame, false) ||
			bAutoUndoOnDeath != baseSettings.GetVar(Settings::AutoUndoOnDeath, false) ||
			dwLastNotice != baseSettings.GetVar(Settings::LastNotice, 0u);

		if (bChangedSettings)
		{
			baseSettings.SetVar(Settings::MoveCounter, bMoveCounter);
			baseSettings.SetVar(Settings::PuzzleMode, bPuzzleMode);
			baseSettings.SetVar(Settings::EnableChatInGame, bShowChats);
			baseSettings.SetVar(Settings::ReceiveWhispersOnlyInGame, bReceiveWhispersOnly);
			baseSettings.SetVar(Settings::AutoUndoOnDeath, bAutoUndoOnDeath);
			baseSettings.SetVar(Settings::LastNotice, dwLastNotice);
			pCurPlayer->Update();
		}
		delete pCurPlayer;
	} else {
		this->bAreaJustCopied = this->bCutAndPaste = false;
		this->wLastEntranceSelected = NO_ENTRANCE;
		this->obstacles.Init(this->pRoom->wRoomCols, this->pRoom->wRoomRows);
	}
	this->nUndoSize = this->undoList.size();

	//Init the keysym-to-command map and load player editor settings.
	ApplyPlayerSettings();

	SelectFirstWidget(false);

	SetCursor();
	return true;
}

//
//Private methods.
//

//*****************************************************************************
void CEditRoomScreen::AddPlotEffect(
//Returns: tile #'s to display specified object
//
//Params:
	const UINT wObjectNo)
{
	//Determine size of object to show.
	UINT wCX, wCY;
	switch (wObjectNo)
	{
		case T_TARMOTHER:
		case T_MUDMOTHER:
		case T_GELMOTHER:
			wCX = 2;  wCY = 1;
			break;

		case T_ROCKGIANT:
			wCX = 2;  wCY = 2;
			break;

		default:
			wCX = wCY = 1;
			break;
	}

	this->pRoomWidget->AddPendingPlotEffect(wObjectNo,
			DisplaySelection(wObjectNo), wCX, wCY, SinglePlacement[wObjectNo], this->wO);
}

//*****************************************************************************
void CEditRoomScreen::ClearList(
//Clears sequence of base pointers, deleting all objects pointed to.
//
//Params:
	UndoList& List)
{
	for (UndoList::iterator vec=List.begin(); vec!=List.end(); ++vec)
		for (vector<CDbBase*>::iterator iter = vec->begin(); iter != vec->end(); ++iter)
			delete *iter;
	List.clear();
}

//*****************************************************************************
void CEditRoomScreen::ClickRoom()
//Handle click event in room area.
{
	switch (this->eState)
	{
		case ES_LONGMONSTER:
		{
			//Place a long monster segment if there are no obstacles in the way.
			const PlotType plot = PlotLongMonsterSegment();
			switch (plot)
			{
				case PLOT_NOTHING:
					//An illegal placement was attempted -- try again.
					break;
				case PLOT_HEAD:
				{
					//Only a head was plotted, or the tail was moved over the head.
					const UINT monster_type = this->wSelectedObject;
					if (monster_type != T_GENTRYII) {
						//Serpents need a body -- remove the monster head.
						CCueEvents Ignored;
						this->pRoom->KillMonsterAtSquare(
								this->pRoomWidget->monsterSegment.wHeadX,
								this->pRoomWidget->monsterSegment.wHeadY,
								Ignored, true);
					}

					VERIFY(SetState(ES_PLACING));
					Paint(); //erase shaded square and redraw errors
				}
					break;
				case PLOT_DONE:
					//Done placing monster.
					VERIFY(SetState(ES_PLACING));
					Paint(); //redraw errors
					break;
				case PLOT_SEGMENT:
					//Another segment was placed -- continue.
					break;
				case PLOT_ERROR:
					ASSERT(!"Plot error.");
					break;
			}
		}
		break;

		case ES_DOOR:
		{
			const UINT wX = this->pRoomWidget->wEndX,
					wY = this->pRoomWidget->wEndY;
			const UINT wDoorX = this->pOrb->wX,
				  wDoorY = this->pOrb->wY;
			const UINT wOSquare = this->pRoom->GetOSquare(wX, wY);
			const UINT wTSquare = this->pRoom->GetTSquare(wX, wY);
			if (bIsYellowDoor(wOSquare))
			{
				//Start editing the orb agents associated with this door.
				//ATTN: this->pOrb will be a temporary record that keeps track of which
				//orbs/plates affect the door.
				SetOrbAgentsForDoor(wX, wY);
			}
			else if (wTSquare == T_ORB || wOSquare == T_PRESSPLATE)
			{
				//Change the door's association with the orb/pressure plate clicked on.
				Changing();
				COrbAgentData *pAgent = NULL;
				COrbData *pEditOrb;
				if (wOSquare == T_PRESSPLATE)
					pEditOrb = this->pRoom->GetPressurePlateAtCoords(wX,wY);
				else
					pEditOrb = this->pRoom->GetOrbAtCoords(wX,wY);
				if (!pEditOrb)
				{
					//Add an orb record for this orb.
					pEditOrb = this->pRoom->AddOrbToSquare(wX,wY);
					ASSERT(pEditOrb);
					pEditOrb->eType = (OrbType)(wOSquare == T_PRESSPLATE ?
							this->wSelPlateType : this->wSelOrbType);
				} else {
					// See whether this orb has any agents for this tile.
					pAgent = FindOrbAgentFor(wDoorX, wDoorY, pEditOrb);
				}
				if (!pAgent)
				{
					//No -- give the orb an agent to this door.
					pEditOrb->AddAgent(wDoorX,wDoorY, OA_TOGGLE);
					this->pOrb->AddAgent(pEditOrb->wX, pEditOrb->wY, OA_TOGGLE);
					this->pRoomWidget->AddOrbAgentToolTip(pEditOrb->wX, pEditOrb->wY, OA_TOGGLE);
				} else {
					//Yes -- change the orb's effect on this door.
					COrbAgentData* pDoorAgent; //update visual display of what effects door
					if (wOSquare == T_PRESSPLATE)
						pDoorAgent = this->pOrb->GetAgentAt(pEditOrb->wX, pEditOrb->wY);
					else
						pDoorAgent = this->pOrb->GetAgentAt(wX, wY);
					ASSERT(pDoorAgent);
					++pAgent->action;
					if (!bIsValidOrbAgentType(pAgent->action))
					{
						pEditOrb->DeleteAgent(pAgent);
						this->pOrb->DeleteAgent(pDoorAgent);
						this->pRoomWidget->AddOrbAgentToolTip(wX, wY, OA_NULL);
					} else {
						++pDoorAgent->action;
						this->pRoomWidget->AddOrbAgentToolTip(wX, wY, pAgent->action);
					}
				}
			} else {
				//If neither is here, leave orb editing state.
				this->pRoomWidget->ResetPlot();
				this->pRoomWidget->wOX = static_cast<UINT>(-1);
				delete this->pOrb;
				this->pOrb = NULL;
				VERIFY(SetState(ES_PLACING));
				this->pRoomWidget->ClearEffects();
				Paint(); //refresh room
			}
		}
		break;

		case ES_ORB:
		{
			ASSERT(this->pOrb);  //some orb should always be selected
			const UINT wX = this->pRoomWidget->wEndX, wY = this->pRoomWidget->wEndY;
			const UINT wOTileNo = this->pRoom->GetOSquare(wX,wY);
			const UINT wTSquare = this->pRoom->GetTSquare(wX,wY);
			const UINT fTile = this->pRoom->GetFSquare(wX,wY);
			if (wTSquare == T_ORB || (wOTileNo == T_PRESSPLATE && !bIsAnyArrow(fTile)))
			{
				//Start editing this orb/pressure plate.
				COrbData *pNewOrb;
				if (wOTileNo == T_PRESSPLATE)
					pNewOrb = this->pRoom->GetPressurePlateAtCoords(wX,wY);
				else
					pNewOrb = this->pRoom->GetOrbAtCoords(wX,wY);
				if (pNewOrb)
					this->pOrb = pNewOrb;
				else
				{
					//Add an orb record to the room.
					this->pOrb = this->pRoom->AddOrbToSquare(wX,wY);
					this->pOrb->eType = (OrbType)(wOTileNo == T_PRESSPLATE ?
							this->wSelPlateType : this->wSelOrbType);
				}
				this->pRoomWidget->RemoveLastLayerEffectsOfType(ETOOLTIP);
				this->pRoomWidget->AddOrbAgentsEffect(this->pOrb);
			} else if (bIsYellowDoor(wOTileNo) || bIsLight(wTSquare) || bIsAnyArrow(fTile) || bIsFiretrap(wOTileNo)) {
				//A door or light exists here -- add, modify or delete its orb agent.
				Changing();
				//Is this door already affected by the orb?
				COrbAgentData *pAgent = FindOrbAgentFor(wX,wY,this->pOrb);
				if (!pAgent)
				{
					//No -- add new agent for this door.
					this->pOrb->AddAgent(wX,wY,OA_TOGGLE);
					this->pRoomWidget->AddOrbAgentToolTip(wX, wY,
							OA_TOGGLE);
				} else {
					//Yes -- change the agent's effect on the door.
					++pAgent->action;
					if (!bIsValidOrbAgentType(pAgent->action))
					{
						this->pOrb->DeleteAgent(pAgent);
						this->pRoomWidget->AddOrbAgentToolTip(wX, wY, OA_NULL);
					}
					else
						this->pRoomWidget->AddOrbAgentToolTip(wX, wY, pAgent->action);
				}

				this->pRoomWidget->AddOrbAgentsEffect(this->pOrb);
			} else {
				//If none of these are here, leave orb editing state.
				this->pRoomWidget->ResetPlot();
				this->pRoomWidget->wOX = static_cast<UINT>(-1);
				VERIFY(SetState(ES_PLACING));
				this->pRoomWidget->ClearEffects(true);
				Paint(); //refresh room
			}
		}
		break;

		case ES_PASTING:
			if (RightMouseButton())
				SetState(ES_PLACING);
			else
			{
				//If Ctrl-C was hit to copy a room region while the mouse
				//was down, then let it come up w/o doing anything.
				if (this->bAreaJustCopied)
				{
					this->bAreaJustCopied = false;
					break;
				}

				//Duplicate marked room region.
				ASSERT(this->wCopyX1 != (UINT)-1);
				g_pTheSound->PlaySoundEffect(SEID_MIMIC);
				PasteRegion(this->pRoomWidget->wEndX, this->pRoomWidget->wEndY);
			}
		break;

		case ES_PLACING:
			if (RightMouseButton())
				EraseObjects();
			else
				PlotObjects();
		break;

		case ES_TESTROOM:
		{
			//Start playtest session at indicated room coords.
			const UINT wX = this->pRoomWidget->wEndX, wY = this->pRoomWidget->wEndY;

			//Streamline: orient start location to level entrance at this location, if any.
			UINT wStartO = this->wO;
			for (UINT wIndex=0; wIndex<this->pRoomWidget->pLevelEntrances->size(); ++wIndex)
			{
				const CMoveCoord& entrance = *((*this->pRoomWidget->pLevelEntrances)[wIndex]);
				if (wX == entrance.wX && wY == entrance.wY)
				{
					wStartO = entrance.wO;
					break;
				}
			}
				
			if (!this->pRoomWidget->IsSafePlacement(T_SWORDSMAN, wX, wY, wStartO, true))
				return;

			if (!SaveRoom())  //update latest changes
			{
				//Reload the room in its unmodified state.
				this->bRoomDirty = false;  //Don't ask again to save room.
				LoadRoomAtCoords(this->pRoom->dwRoomX, this->pRoom->dwRoomY);
				//Confirm starting place is still valid.
				if (!this->pRoomWidget->IsSafePlacement(T_SWORDSMAN, wX, wY, wStartO))
					return;
			}

			CGameScreen *pGameScreen = DYN_CAST(CGameScreen *, CScreen *,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			VERIFY(SetState(ES_PLACING));

			CDbPlayer *pCurPlayer = g_pTheDB->GetCurrentPlayer();
			ASSERT(pCurPlayer);

			//Create temporary player for play-testing.
			CDbPlayer *pPlayer = g_pTheDB->Players.GetNew();
			pPlayer->bIsLocal = false;
			pPlayer->NameText = wszEmpty;
			pPlayer->CNetNameText = wszEmpty;
			pPlayer->CNetPasswordText = wszEmpty;
			pPlayer->Settings = pCurPlayer->Settings;
			CDbPackedVars& settings = pPlayer->Settings;
			settings.SetVar(Settings::ShowCheckpoints, true); //always show checkpoints
			settings.SetVar(Settings::ConnectToInternet, false);
			settings.SetVar(Settings::MoveCounter,
				pCurPlayer->Settings.GetVar(Settings::MoveCounter, false));
			settings.SetVar(Settings::PuzzleMode,
				pCurPlayer->Settings.GetVar(Settings::PuzzleMode, false));
			settings.SetVar(Settings::AutoUndoOnDeath,
				pCurPlayer->Settings.GetVar(Settings::AutoUndoOnDeath, false));
			settings.SetVar(Settings::LastNotice,
				pCurPlayer->Settings.GetVar(Settings::LastNotice, 0));
			pPlayer->Update();
			this->dwTestPlayerID = pPlayer->dwPlayerID;
			this->dwSavePlayerID = g_pTheDB->GetPlayerID();
			g_pTheDB->SetPlayerID(this->dwTestPlayerID, false); //don't log player out of CaravelNet
			delete pPlayer;
			delete pCurPlayer;

			if (pGameScreen->TestRoom(this->pRoom->dwRoomID, wX, wY, wStartO))
			{
				//Make this the active hold and go to game screen.
				g_pTheDB->SetHoldID(this->pHold->dwHoldID);
				GoToScreen(SCR_Game);
			} else {
				//Delete temp player if starting play-testing failed.
				g_pTheDB->SetPlayerID(this->dwSavePlayerID, false); //don't need to relogin to CaravelNet
				g_pTheDB->Players.Delete(this->dwTestPlayerID, false);
				this->dwSavePlayerID = this->dwTestPlayerID = 0;
			}
		}
		break;

		//Fill in coordinate values for a script command being added,
		//and continue editing the character script.
		case ES_GETSQUARE:
		{
			if (this->bSelectingImageStart)
			{
				VERIFY(SetState(ES_PLACING));   //resets this->bSelectingImageStart
				if (GetSelectedObject() == T_OVERHEAD_IMAGE) {
					this->pRoom->wOverheadImageStartX = this->pRoomWidget->wEndX;
					this->pRoom->wOverheadImageStartY = this->pRoomWidget->wEndY;
				} else {
					this->pRoom->wImageStartX = this->pRoomWidget->wEndX;
					this->pRoom->wImageStartY = this->pRoomWidget->wEndY;
				}
				this->pRoomWidget->Paint();
			} else {
				this->pCharacterDialog->FinishCommand(this->pRoomWidget->wEndX,
						this->pRoomWidget->wEndY);
				VERIFY(SetState(ES_PLACING));
				this->pCharacterDialog->Display();
			}
			Paint();
		}
		break;
		case ES_GETRECT:
		{
			this->pCharacterDialog->FinishCommand(this->pRoomWidget->wStartX,
					this->pRoomWidget->wStartY,
					this->pRoomWidget->wEndX - this->pRoomWidget->wStartX,
					this->pRoomWidget->wEndY - this->pRoomWidget->wStartY);
			VERIFY(SetState(ES_PLACING));
			this->pCharacterDialog->Display();
			Paint();
		}
		break;

		default: ASSERT(false); break;
	}
}

//*****************************************************************************
bool CEditRoomScreen::DeleteLevelEntrance(
//Deletes the level entrance at (wX, wY).  The entrance cannot be the level's main one.
//
//Params:
	const UINT wX, const UINT wY) //(in)
{
	ASSERT(this->pRoom->IsValidColRow(wX, wY));

	CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID, wX, wY);
	if (!pEntrance) return false;

	//Can't delete the level's main entrance.
	if (pEntrance->bIsMainEntrance)
	{
		ShowOkMessage(MID_CantDeleteMainEntrance);
		return false;
	}

	//Get user confirmation.
	if (ShowYesNoMessage(MID_DeleteEntrancePrompt) != TAG_YES) return false;

	Changing(Hold); //add current hold entrances state to undo list
	this->pHold->DeleteEntrance(pEntrance);
	GetLevelEntrancesInRoom();
	return true;
}

//*****************************************************************************
void CEditRoomScreen::DisplayChatDialog()
//Show dialog box displaying CaravelNet chat interface.
{
	CDialogWidget *pChatBox = DYN_CAST(CDialogWidget*, CWidget*,
			this->pRoomWidget->GetWidget(TAG_CHATBOX));

	pChatBox->SelectWidget(TAG_CHATINPUT, false);

	COptionButtonWidget *pOption = DYN_CAST(COptionButtonWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATENABLE));
	pOption->SetChecked(this->bEnableChat);

	COptionButtonWidget *pWhisperOption = DYN_CAST(COptionButtonWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATWHISPERSONLY));
	pWhisperOption->SetChecked(this->bReceiveWhispersOnly);

	CTextBoxWidget *pChat = DYN_CAST(CTextBoxWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATINPUT));
	pChat->SetText(wszEmpty);

	CListBoxWidget *pUserList = DYN_CAST(CListBoxWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATUSERS));
	if (!g_pTheNet->IsLoggedIn())
	{
		pUserList->Clear();
		pUserList->AddItem(0, g_pTheDB->GetMessageText(MID_CNetNotConnected), true);
	}

	//Display.
	pChatBox->SetBetweenEventsHandler(this); //keep updating room effects and
							//receiving chat messages while prompt is displayed
	const UINT returnTag = pChatBox->Display(false);
	pChatBox->SetBetweenEventsHandler(NULL);
	this->pRoomWidget->DirtyRoom();
	this->pRoomWidget->Paint();

	this->bEnableChat = pOption->IsChecked();
	this->bReceiveWhispersOnly = pWhisperOption->IsChecked();

	//Send chat (if text was entered and OK was pressed).
	const WCHAR *pText = returnTag == TAG_OK ? pChat->GetText() : wszEmpty;
	if (!ParseConsoleCommand(pText)) //Intercept console commands.
		this->chat.SendText(pText, pUserList);
}

//******************************************************************************
void CEditRoomScreen::DisplayChatText(const WSTRING& text, const SDL_Color& color)
{
	this->pRoomWidget->DisplayChatText(text, color);
}

//*****************************************************************************
CObjectMenuWidget* CEditRoomScreen::GetActiveMenu()
//Returns: pointer to active menu widget in tabbed menu
{
	UINT dwTagNo;
	switch (this->pTabbedMenu->GetSelectedTab())
	{
		case O_LAYER_TAB: dwTagNo = TAG_OMENU; break;
		case F_LAYER_TAB: dwTagNo = TAG_FMENU; break;
		case T_LAYER_TAB: dwTagNo = TAG_TMENU; break;
		case MONSTER_TAB: dwTagNo = TAG_MMENU; break;
		default: ASSERT(!"Invalid tab on edit menu."); return NULL;
	}
	return DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(dwTagNo));
}

//*****************************************************************************
void CEditRoomScreen::GetCustomImageID(
	UINT& roomDataID,
	UINT& imageStartX, UINT& imageStartY,
	const bool bReselect) //[default=false]
{
	//ID needs to be assigned once.
	if (roomDataID && !bReselect)
		return;

	//Can only access this operation to change the DB for holds you own.
	if (this->pHold->dwPlayerID != g_pTheDB->GetPlayerID() && !SaveRoomToDB())
		return;

SelectImage:
	UINT dwDataID;
	CEntranceSelectDialogWidget::BUTTONTYPE eButton;
	do {
		dwDataID = roomDataID;
		eButton = SelectListID(
				this->pEntranceBox, this->pHold, dwDataID,
				MID_ImageSelectPrompt, CEntranceSelectDialogWidget::Images);
		if (eButton != CEntranceSelectDialogWidget::OK &&
				eButton != CEntranceSelectDialogWidget::Delete)
			return;

		if (eButton == CEntranceSelectDialogWidget::Delete)
		{
			//Remove this image from the database and make another selection.
			//It's safe if other rooms remain set to this old image ID.
			//They will now show the default texture.
			if (dwDataID)
				this->pHold->MarkDataForDeletion(dwDataID);
			roomDataID = 0;
			this->pRoomWidget->LoadRoomImages();
			this->pRoomWidget->Paint();
		}
	} while (eButton != CEntranceSelectDialogWidget::OK);

	Changing();

	if (dwDataID) {
		roomDataID = dwDataID;   //selected image from DB
	} else {
		//Load new image from disk.
		const UINT dwID = ImportHoldImage(this->pHold->dwHoldID, EXT_JPEG | EXT_PNG);
		if (dwID)
			roomDataID = dwID;
		goto SelectImage;	//return to image select menu
	}

	if (!bReselect)
	{
		imageStartX = this->pRoomWidget->wStartX;
		imageStartY = this->pRoomWidget->wStartY;
	} else {
		this->bSelectingImageStart = true;
		VERIFY(SetState(ES_GETSQUARE));
	}

	this->pRoomWidget->LoadRoomImages();
	SetLightLevel();
	this->pRoomWidget->Paint();
}

//*****************************************************************************
void CEditRoomScreen::GetFloorImageID(const bool bReselect) //[default=false]
//Select an image for display as this room's special floor texture, either from
//the DB or from disk.
{
	GetCustomImageID(this->pRoom->dwDataID,
		this->pRoom->wImageStartX, this->pRoom->wImageStartY,
		bReselect);
}

//*****************************************************************************
void CEditRoomScreen::GetOverheadImageID(const bool bReselect) //[default=false]
//Select an image for display as this room's special overhead texture.
{
	GetCustomImageID(this->pRoom->dwOverheadDataID,
		this->pRoom->wOverheadImageStartX, this->pRoom->wOverheadImageStartY,
		bReselect);
}

//*****************************************************************************
void CEditRoomScreen::GetLevelEntrancesInRoom()
//Compile list of all level entrances in current room.
{
	//Clear entrance list.
	UINT wIndex;
	for (wIndex=this->LevelEntrances.size(); wIndex--; )
		delete this->LevelEntrances[wIndex];
	this->LevelEntrances.clear();

	if (!this->pHold || !this->pRoom) return;

	for (wIndex=0; wIndex<this->pHold->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->pHold->Entrances[wIndex];
		if (pEntrance->dwRoomID == this->pRoom->dwRoomID)
		{
			this->LevelEntrances.push_back(new CMoveCoord(pEntrance->wX,
					pEntrance->wY, pEntrance->wO));
		}
	}
}

//*****************************************************************************
const UINT* CEditRoomScreen::GetTileImageForMonsterType(
//Gets a (pointer to a) tile image to display for a monster.
//
//Returns:
//(pointer to a) TI_* constant.
//
//Params:
	const UINT wType,          //(in)   Contains data needed to figure out
	const UINT wO,             //(in)   which array to get tile from
	const UINT wAnimFrame)     //(in)   tile image.
const
{
	ASSERT(IsValidOrientation(wO));
	if (IsValidMonsterType(wType))
	{
		ASSERT(MonsterTileImageArray[wType][wO] != DONT_USE);
		return (wAnimFrame == 0 ?
				MonsterTileImageArray[wType]+wO :
				AnimatedMonsterTileImageArray[wType]+wO);
	} else {
		//Character monster pseudo-type.
		ASSERT(wType>=CHARACTER_FIRST && wType<CHARACTER_TYPES);
		ASSERT(CharacterTileImageArray[wType-CHARACTER_FIRST][wO] != DONT_USE);
		return CharacterTileImageArray[wType-CHARACTER_FIRST]+wO;
	}
}

//*****************************************************************************
const UINT* CEditRoomScreen::DisplaySelection(
//Returns: tile #'s to display specified object as it's being placed
//
//Params:
	const UINT wObjectNo)
const
{
	//Swordsman image.
	static const UINT SMAN_TI[] = {TI_SMAN_YNW, TI_SMAN_YN, TI_SMAN_YNE,
		TI_SMAN_YW, TI_TEMPTY, TI_SMAN_YE, TI_SMAN_YSW, TI_SMAN_YS, TI_SMAN_YSE};

	//Images not included in menu tiles that should be shown for aesthetic effect.
	static const UINT Tiles[][4] = {
		{TI_DOOR_Y},
		{TI_DOOR_C},
		{TI_DOOR_M},
		{TI_DOOR_R},
		{TI_DOOR_B},
		{TI_TAR_NSEW},
		{TI_WALL},
		{TI_WALL_B},
		{TI_MUD_NSEW},
		{TI_WALL_H},
		{TI_GEL_NSEW},
		{TI_PPB},
		{TI_PPT},
		{TI_FLUFF_NSEW},
		{TI_THINICE_NSWE}
	};

	if (wObjectNo < M_OFFSET)
	{
		switch (wObjectNo)
		{
			default: return MenuDisplayTiles[wObjectNo];

			//O-layer
			case T_DOOR_Y: return Tiles[0];
			case T_DOOR_C: return Tiles[1];
			case T_DOOR_M: return Tiles[2];
			case T_DOOR_R: return Tiles[3];
			case T_DOOR_B: return Tiles[4];
			case T_WALL: case T_WALL2: return Tiles[6];
			case T_WALL_B: return Tiles[7];
			case T_WALL_H: return Tiles[9];
			case T_THINICE_SH: return Tiles[14];
			case T_PRESSPLATE:
				switch (this->wSelPlateType)
				{
					case OT_ONEUSE: return Tiles[11];
					case OT_TOGGLE: return Tiles[12];
					default: return MenuDisplayTiles[T_PRESSPLATE];
				}
			break;

			//T-layer
			case T_TAR: return Tiles[5];
			case T_MUD: return Tiles[8];
			case T_GEL: return Tiles[10];
			case T_FLUFF: return Tiles[13];
			case T_OBSTACLE:
			{
				//Show from the smallest obstacle of the selected type.
				const UINT obTypeSmallestIndex = obstacleIndices[this->wSelectedObType][0];
				ASSERT(obTypeSmallestIndex);
				ASSERT(obstacleTile[obTypeSmallestIndex][0][0]);
				return obstacleTile[obTypeSmallestIndex][0];
			}
			case T_TOKEN:
			{
				static UINT wTokenTI;
				wTokenTI = CalcTileImageForToken(wSelTokenType);
				return &wTokenTI;
			}
		}
	} else {
		//M-layer
		switch (wObjectNo)
		{
			default: return GetTileImageForMonsterType(wObjectNo - M_OFFSET, this->wO, 0);
			case T_SWORDSMAN: return SMAN_TI + this->wO;
			case T_NOMONSTER: case T_EMPTY_F: return MenuDisplayTiles[T_EMPTY];
			case T_SERPENT: return GetTileImageForMonsterType(M_SERPENT, E, 0);
			case T_SERPENTG: return GetTileImageForMonsterType(M_SERPENTG, E, 0);
			case T_SERPENTB: return GetTileImageForMonsterType(M_SERPENTB, E, 0);
			case T_TARMOTHER: case T_MUDMOTHER: case T_GELMOTHER:
				return MenuDisplayTiles[wObjectNo];
			case T_BRAIN: return GetTileImageForMonsterType(M_BRAIN, NO_ORIENTATION, 0);
			case T_SKIPPERNEST: return GetTileImageForMonsterType(M_SKIPPERNEST, NO_ORIENTATION, 0);
			case T_ROCKGIANT:
			{
				//Generate 2x2 RockGiant image for this orientation.
				static UINT giantTiles[4];
				giantTiles[0] = GetTileImageForEntity(M_ROCKGIANT, this->wO, 0);
				giantTiles[1] = GetTileImageForRockGiantPiece(0, this->wO, 0);
				giantTiles[2] = GetTileImageForRockGiantPiece(1, this->wO, 0);
				giantTiles[3] = GetTileImageForRockGiantPiece(2, this->wO, 0);
				return giantTiles;
			}
		}
	}
}

//*****************************************************************************
UINT CEditRoomScreen::AddRoom(
//Inserts a new room into the current level at given position in the DB.
//(Code was mostly cut-and-pasted from CEditSelectScreen::AddRoom().)
//
//Returns: new room's ID#
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of chosen room.
{
	LOGCONTEXT("CEditRoomScreen::AddRoom");

	if (!this->pLevel)
		return 0L;
	if (this->pLevel->GetRoomIDAtCoords(dwRoomX,dwRoomY))
		return 0L;  //room already exists here

	//Get new room.
	CDbRoom *pRoom = g_pTheDB->Rooms.GetNew();

	//Set members that correspond to database fields.
	pRoom->dwLevelID = this->pLevel->dwLevelID;
	pRoom->dwRoomX = dwRoomX;
	pRoom->dwRoomY = dwRoomY;
	pRoom->wRoomCols = CDrodBitmapManager::DISPLAY_COLS;
	pRoom->wRoomRows = CDrodBitmapManager::DISPLAY_ROWS;
	pRoom->style = this->pRoom->style;   //maintain style
	pRoom->bIsRequired = true;
	pRoom->bIsSecret = false;
	pRoom->weather = this->pRoom->weather;   //maintain weather

	if (!pRoom->AllocTileLayers()) {
		delete pRoom;
		return 0;
	}

	//Make room empty.
	const UINT dwSquareCount = pRoom->CalcRoomArea();
	memset(pRoom->pszOSquares, T_FLOOR, dwSquareCount * sizeof(char));
	memset(pRoom->pszFSquares, T_EMPTY, dwSquareCount * sizeof(char));
	pRoom->ClearTLayer();

	pRoom->coveredOSquares.Init(pRoom->wRoomCols, pRoom->wRoomRows);
	pRoom->tileLights.Init(pRoom->wRoomCols, pRoom->wRoomRows);

	//Add proper edges to the room.
	FillInRoomEdges(pRoom);

	//Save the new room.
	if (!pRoom->Update())
	{
		ShowOkMessage(MID_RoomNotSaved);
		delete pRoom;
		return 0L;
	}

	const UINT dwRoomID = pRoom->dwRoomID;
	delete pRoom;

	//Update map.
	this->pMapWidget->LoadFromLevel(this->pLevel);
	LoadRoomAtCoords(dwRoomX,dwRoomY);

	return dwRoomID;
}

//*****************************************************************************
void CEditRoomScreen::ApplyPlayerSettings()
//Apply player settings to the game screen.
{
	LOGCONTEXT("CEditRoomScreen::ApplyPlayerSettings");

	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pCurrentPlayer) {ASSERT(!"Couldn't retrieve player."); return;} //Corrupt db.

	//Set the keysym-to-command map from player settings.
	InitKeysymToCommandMap(pCurrentPlayer->Settings);

	//Synch player setting to current screen mode, if needed.
	const bool bIsFullScreen = IsFullScreen();
	if (bIsFullScreen != pCurrentPlayer->Settings.GetVar(Settings::Fullscreen, bIsFullScreen))
	{
		pCurrentPlayer->Settings.SetVar(Settings::Fullscreen, bIsFullScreen);
		pCurrentPlayer->Update();
	}

	//Set default room editing options, if needed.
	if (SetUnspecifiedPlayerSettings(pCurrentPlayer->Settings))
		pCurrentPlayer->Update();

	//Set room editing options.
	this->bAutoSave = pCurrentPlayer->Settings.GetVar(Settings::AutoSave, true);

	COptionButtonWidget *pOptionButton = static_cast<COptionButtonWidget *>(
			GetWidget(TAG_SHOWERRORS));
	this->bShowErrors = pCurrentPlayer->Settings.GetVar(Settings::ShowErrors, true);
	pOptionButton->SetChecked(this->bShowErrors);

	//Menu options.
	const bool bOldGroupSetting = this->bGroupMenuItems;
	this->bGroupMenuItems = pCurrentPlayer->Settings.GetVar(Settings::GEMI, false);
	if (this->bGroupMenuItems != bOldGroupSetting)
		PopulateItemMenus();

	//Chat.
	this->bEnableChat = pCurrentPlayer->Settings.GetVar(Settings::EnableChatInGame, false);
	this->bReceiveWhispersOnly = pCurrentPlayer->Settings.GetVar(Settings::ReceiveWhispersOnlyInGame, false);

	delete pCurrentPlayer;
}

//*****************************************************************************
void CEditRoomScreen::HighlightPendingPaste()
//Highlight the area that the room region paste destination will cover,
//according to the current mouse position.
{
	ASSERT(this->pRoom->IsValidColRow(this->wCopyX1, this->wCopyY1));
	ASSERT(this->pRoom->IsValidColRow(this->wCopyX2, this->wCopyY2));

	//Show paste source region, if coming from this same room.
	const bool bShowSource = !this->bCutAndPaste && (!this->pCopyRoom ||
			(this->pRoom->dwRoomID == this->pCopyRoom->dwRoomID));
	if (bShowSource)
		this->pRoomWidget->AddPendingPasteEffect(this->wCopyX1, this->wCopyY1,
				this->wCopyX2 - this->wCopyX1, this->wCopyY2 - this->wCopyY1,
				PaleYellow, true);

	//Highlight destination, if it's not the same as the source location.
	if (!bShowSource || this->pRoomWidget->wEndX != this->wCopyX1 ||
			this->pRoomWidget->wEndY != this->wCopyY1)
	{
		const SURFACECOLOR Gray = {128, 128, 128};
		this->pRoomWidget->AddPendingPasteEffect(
				this->pRoomWidget->wEndX, this->pRoomWidget->wEndY,
				this->wCopyX2 - this->wCopyX1, this->wCopyY2 - this->wCopyY1,
				Gray, !bShowSource);
	}
}

//*****************************************************************************
UINT CEditRoomScreen::ImportHoldSound()
//Load a sound file from disk into the hold.
//Returns: dataID if operation completed successfully or 0 if it was canceled.
{
	//Get sound import path.
	CFiles Files;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrImportPath = pCurrentPlayer ?
		pCurrentPlayer->Settings.GetVar(Settings::ImportSoundPath, Files.GetDatPath().c_str()) :
			Files.GetDatPath();

	WSTRING wstrImportFile;
	do {
		const UINT dwTagNo = SelectFile(wstrImportPath,
				wstrImportFile, MID_ImportSound, false, EXT_OGG | EXT_WAV);
		if (dwTagNo != TAG_OK)
		{
			delete pCurrentPlayer;
			return 0;
		}

		//Update the path in player settings, so next time dialog
		//comes up it will have the same path.
		if (pCurrentPlayer)
		{
			pCurrentPlayer->Settings.SetVar(Settings::ImportSoundPath, wstrImportPath.c_str());
			pCurrentPlayer->Update();
		}

		//Load sound effect.
		CStretchyBuffer buffer;
		if (!Files.ReadFileIntoBuffer(wstrImportFile.c_str(), buffer, true))
		{
			ShowOkMessage(MID_FileNotFound);
		} else if (!g_pTheSound->VerifySound(buffer)) {
			//Not a valid sound file.
			ShowOkMessage(MID_FileNotValid);
		} else {
			CDbDatum *pSound = g_pTheDB->Data.GetNew();
			//Don't need to distinguish between WAV and OGG formats, since
			//CSound is able to figure it out by itself.
			pSound->wDataFormat = DATA_OGG;
			pSound->data.Set((const BYTE*)buffer, buffer.Size());
			pSound->DataNameText = getFilenameFromPath(wstrImportFile.c_str());
			pSound->dwHoldID = this->pHold->dwHoldID;
			pSound->Update();
			const UINT dwDataID = pSound->dwDataID;
			delete pSound;
			delete pCurrentPlayer;
			return dwDataID;
		}
	} while (true);

	ASSERT(!"Bad logic path.");
	delete pCurrentPlayer;
	return 0;
}

//*****************************************************************************
UINT CEditRoomScreen::ImportHoldVideo()
//Load a sound file from disk into the hold.
//Returns: dataID if operation completed successfully or 0 if it was canceled.
{
	//Get video import path.
	CFiles Files;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrImportPath = pCurrentPlayer ?
		pCurrentPlayer->Settings.GetVar(Settings::ImportVideoPath, Files.GetDatPath().c_str()) :
			Files.GetDatPath();

	WSTRING wstrImportFile;
	do { 
		const UINT dwTagNo = SelectFile(wstrImportPath,
				wstrImportFile, MID_ImportVideo, false, EXT_THEORA);
		if (dwTagNo != TAG_OK)
		{
			delete pCurrentPlayer;
			return 0;
		}

		//Update the path in player settings, so next time dialog
		//comes up it will have the same path.
		if (pCurrentPlayer)
		{
			pCurrentPlayer->Settings.SetVar(Settings::ImportVideoPath, wstrImportPath.c_str());
			pCurrentPlayer->Update();
		}

		//Load video.
		CStretchyBuffer buffer;
		if (!Files.ReadFileIntoBuffer(wstrImportFile.c_str(), buffer, true))
		{
			ShowOkMessage(MID_FileNotFound);
		} else {
			ShowStatusMessage(MID_VerifyingMedia);
			const bool bRes = PlayVideoBuffer(buffer, NULL); //test video
			HideStatusMessage();
			if (!bRes)
			{
				//Not a valid video file.
				ShowOkMessage(MID_FileNotValid);
			} else {
				CDbDatum *pDatum = g_pTheDB->Data.GetNew();
				pDatum->wDataFormat = DATA_THEORA;
				pDatum->data.Set((const BYTE*)buffer, buffer.Size());
				pDatum->DataNameText = getFilenameFromPath(wstrImportFile.c_str());
				pDatum->dwHoldID = this->pHold->dwHoldID;
				pDatum->Update();
				const UINT dwDataID = pDatum->dwDataID;
				delete pDatum;
				delete pCurrentPlayer;
				return dwDataID;
			}
		}
	} while (true);

	ASSERT(!"Bad logic path.");
	delete pCurrentPlayer;
	return 0;
}

//*****************************************************************************
void CEditRoomScreen::IncrementMenuSelection(const bool bForward) //[default=true]
//Some menu entries have multiple options collapsed together.
//For entries with several options, pop up a menu to select from these.
//For entries with 2-3 options, select the next/previous one in sequence.
{
	switch (this->wSelectedObject)
	{
		//Pop-up menu to select from multiple options.
		case T_FLOOR: case T_FLOOR_M: case T_FLOOR_ROAD:
		case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
		{
			if (!this->bGroupMenuItems)
				break;
			const UINT wLastFloor = this->wLastFloorSelected;
			CObjectMenuWidget *pObsMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_FLOOR_MENU));
			pObsMenu->SetSelectedObject(wLastFloor);
			this->pRoomWidget->SetAnimateMoves(false);
			pObsMenu->PopUp();
			SetMenuItem(wLastFloor, pObsMenu->GetSelectedObject());
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;

		case T_OBSTACLE:
		{
			CObjectMenuWidget *pObsMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_OBS_MENU));
			pObsMenu->SetSelectedObject(this->wSelectedObType);
			this->pRoomWidget->SetAnimateMoves(false);
			pObsMenu->PopUp();
			this->wSelectedObType = pObsMenu->GetSelectedObject();
			UpdateMenuGraphic(T_OBSTACLE);
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;

		case T_TOKEN:
		{
			CObjectMenuWidget *pTokenMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_TOKEN_MENU));
			pTokenMenu->SetSelectedObject(this->wSelTokenType);
			this->pRoomWidget->SetAnimateMoves(false);
			pTokenMenu->PopUp();
			this->wSelTokenType = pTokenMenu->GetSelectedObject();
			UpdateMenuGraphic(T_TOKEN);
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;

		case T_POTION_K: case T_POTION_I: case T_POTION_D: case T_POTION_C: case T_POTION_SP:
		case T_HORN_SQUAD: case T_HORN_SOLDIER:
		{
			if (!this->bGroupMenuItems)
				break;
			CObjectMenuWidget *pPotionMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_POTION_MENU));
			pPotionMenu->SetSelectedObject(this->wSelectedObject);
			this->pRoomWidget->SetAnimateMoves(false);
			pPotionMenu->PopUp();
			SetMenuItem(this->wSelectedObject, pPotionMenu->GetSelectedObject());
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;

		case T_LIGHT: case T_WALLLIGHT: case T_LIGHT_CEILING:
		case T_STATION:
		{
			CObjectMenuWidget *pObsMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_LIGHT_MENU));
			pObsMenu->SetSelectedObject(this->wSelectedLightType);
			this->pRoomWidget->SetAnimateMoves(false);
			pObsMenu->PopUp();
			this->wSelectedLightType = pObsMenu->GetSelectedObject();
			UpdateMenuGraphic(T_LIGHT);
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;

		case T_DARK_CEILING:
		{
			CObjectMenuWidget *pObsMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(TAG_DARK_MENU));
			pObsMenu->SetSelectedObject(this->wSelectedDarkType);
			this->pRoomWidget->SetAnimateMoves(false);
			pObsMenu->PopUp();
			this->wSelectedDarkType = pObsMenu->GetSelectedObject();
			UpdateMenuGraphic(T_DARK_CEILING);
			this->pRoomWidget->SetAnimateMoves(true);
			Paint();
		}
		break;
		default:
			if (bForward)
				RotateClockwise();
			else
				RotateCounterClockwise();
		break;
	}
}

//*****************************************************************************
bool CEditRoomScreen::LoadRoomAtCoords(
//Loads a room on this level and sets the current room to it.
//If a failure occurs, the current room will stay loaded.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY, //(in)   Coords to specify room to load.
	const bool bForceReload) //if true, load even if apparently same room [default=false]
//
//Returns:
//True if successful, false if not.
{
	//Reselecting same room doesn't reload anything.
	if (this->pRoom && this->pRoom->dwRoomX == dwRoomX && this->pRoom->dwRoomY == dwRoomY &&
			!bForceReload)
		return false;

	//Reload room, even if it has the same coordinates as the current room,
	//as it might have just been updated in the DB.
	CDbRoom *pNewRoom = this->pLevel->GetRoomAtCoords(dwRoomX, dwRoomY);
	if (!pNewRoom) return false;

	//If a copy of another player's hold was just made,
	//then we need to load the room from it instead of this one.
	if (this->pLevel->dwLevelID != pNewRoom->dwLevelID)
	{
		delete pNewRoom;
		pNewRoom = this->pLevel->GetRoomAtCoords(dwRoomX,
				(this->pLevel->dwLevelID * 100) + (dwRoomY % 100));
		if (!pNewRoom) return false;
	}

	return LoadRoom(pNewRoom);
}

//*****************************************************************************
bool CEditRoomScreen::LoadRoom(
//Loads a room on this level and sets the current room to it.
//If a failure occurs, the current room will stay loaded.
//
//Params:
	CDbRoom *pNewRoom)
{
	if (!pNewRoom) return false;

	//Finish/discard any operation that was in place.
	VERIFY(SetState(ES_PLACING));
	this->pRoomWidget->ClearEffects(false);
	ClearList(this->undoList);
	ClearList(this->redoList);
	SetButtons();

	const UINT dwRoomID = this->pRoom->dwRoomID;
	SaveRoom();
	//During the call to SaveRoom(), the user might have made their own copy of
	//another player's hold.  If so, the new room will have already been loaded
	//and going to pNewRoom will no longer be valid.
	if (dwRoomID == this->pRoom->dwRoomID)
	{
		delete this->pRoom;
		this->pRoom = pNewRoom;
		this->bRoomDirty = false;

		GetLevelEntrancesInRoom();
		this->pRoom->PlaceCharacters(this->pHold);
		this->pRoomWidget->LoadFromRoom(pNewRoom, this->pHold, &this->LevelEntrances);
		this->pMapWidget->SelectRoom(this->pRoom->dwRoomX, this->pRoom->dwRoomY);
		CRoomScreen::SetMusicStyle(this->pRoom->style, SONG_EDITOR);
		SetSignTextToCurrentRoom();
	}
	ResetAdjacentRooms();

	//No area in new room is marked for copying.
	this->bAreaJustCopied = this->bCutAndPaste = false;

	this->obstacles.Init(this->pRoom->wRoomCols, this->pRoom->wRoomRows);

	SetLightLevel();

	Paint();

	return true;
}

//*****************************************************************************
void CEditRoomScreen::OnBetweenEvents()
//Called periodically when no events are being processed.
{
	if (this->bEnableChat)
	{
		ChatPolling(TAG_CHATUSERS);

		//Continue displaying new chat messages received while typing a chat message.
		CDialogWidget *pChatBox = DYN_CAST(CDialogWidget*, CWidget*,
				this->pRoomWidget->GetWidget(TAG_CHATBOX));
		if (pChatBox->IsVisible())
		{
			this->pRoomWidget->Paint(false); //UpdateRect for room area is called below
			pChatBox->Paint(false);
		}
	}

	CScreen::OnBetweenEvents();
	ShowCursor();

	//Refresh item text when requested.
	if (this->bPaintItemText)
	{
		CLabelWidget *pItemLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAB_ITEMTEXT));
		this->pTabbedMenu->RequestPaint();
		pItemLabel->RequestPaint();
		this->bPaintItemText = false;
	}

	//State sensitive tool tips, etc.
	switch (this->eState)
	{
		case ES_PLACING:
		{
			//Menu item tool tip.
			CWidget *pWidget = GetWidgetContainingCoords(this->wLastMouseX,
				this->wLastMouseY, WT_ObjectMenu);
			if (!pWidget) break;
			CObjectMenuWidget *pMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, pWidget);
			const UINT wObjectNo = pMenu->GetObjectIDAt(
					this->wLastMouseX, this->wLastMouseY);
			if (wObjectNo == NO_SELECTED_OBJECT)
				break;

			switch (wObjectNo)
			{
				case T_ORB:
				case T_PRESSPLATE:
				case T_WALL: case T_WALL2:
				case T_WALL_H: case T_WALL_B:
				case T_STAIRS: case T_STAIRS_UP:
				case T_THINICE: case T_THINICE_SH: case T_STEP_STONE:
					RequestToolTip(MID_RotateToChangeType);
				break;
				case T_TAR: case T_MUD: case T_GEL:
				case T_PIT: case T_WATER: case T_SHALLOW_WATER:
				case T_TRAPDOOR: case T_TRAPDOOR2:
				case T_PLATFORM_P: case T_PLATFORM_W:
				case T_POTION_K: case T_POTION_I: case T_POTION_D:
				case T_POTION_SP: case T_POTION_C:
				case T_SERPENTB: case T_SERPENT: case T_SERPENTG:
				case T_TARMOTHER: case T_MUDMOTHER: case T_GELMOTHER:
				case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
				case T_HORN_SQUAD: case T_HORN_SOLDIER:
				case T_BEACON: case T_BEACON_OFF:
					if (this->bGroupMenuItems)
						RequestToolTip(MID_RotateToChangeType);
				break;
				case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD:
				case T_FLOOR_GRASS: case T_FLOOR_DIRT: case T_FLOOR_ALT:
					if (this->bGroupMenuItems)
						RequestToolTip(MID_RotateToChangeTypeAndDoubleClick);
				break;
				case T_FLOOR_IMAGE: case T_PIT_IMAGE: case T_WALL_IMAGE:
					RequestToolTip(this->bGroupMenuItems ? MID_RotateToChangeType : MID_HitF9ToReorient);
				break;
				case T_OVERHEAD_IMAGE:
					RequestToolTip(this->bGroupMenuItems ? MID_RotateToChangeType : MID_HitShiftF9ToReorient);
				break;
				case T_OBSTACLE:
				case T_TOKEN:
					RequestToolTip(MID_RotateToChangeTypeAndDoubleClick);
				break;
				case T_LIGHT: case T_WALLLIGHT:
				case T_LIGHT_CEILING: case T_DARK_CEILING:
				case T_STATION:
					RequestToolTip(MID_RotateToChangeColor);
				break;
				default:
					if (bIsDoor(wObjectNo) || bIsOpenDoor(wObjectNo))
						if (this->bGroupMenuItems)
							RequestToolTip(MID_RotateToOpenAndCloseDoor);
				break;
			}
		}
		break;

		case ES_DOOR:
			ASSERT(this->pOrb);
			this->pRoomWidget->AddOrbAgentsEffect(this->pOrb, false);
			RequestToolTip(MID_DoorAgentTip);
		break;

		case ES_ORB:
			ASSERT(this->pOrb);
			this->pRoomWidget->AddOrbAgentsEffect(this->pOrb);
			RequestToolTip(MID_OrbAgentTip);
		break;

		case ES_LONGMONSTER:
			RequestToolTip(MID_LongMonsterTip);
		break;

		case ES_TESTROOM:
			RequestToolTip(MID_TestRoomTip);
		break;

		case ES_GETSQUARE:
			RequestToolTip(MID_GetRoomSquareTip);
			//Show room coords along with tooltip.
			if (SDL_GetTicks() - this->dwLastMouseMove > 500) //ms
				this->pRoomWidget->DisplayRoomCoordSubtitle(this->wLastMouseX, this->wLastMouseY);
		break;
		case ES_GETRECT:
			RequestToolTip(MID_GetRoomRectTip);
			//Show room coords along with tooltip.
			if (SDL_GetTicks() - this->dwLastMouseMove > 500) //ms
				this->pRoomWidget->DisplayRoomCoordSubtitle(this->wLastMouseX, this->wLastMouseY);
		break;

		case ES_PASTING:
			RequestToolTip(MID_PastingTip);
		break;

		default: break;
	}

	//The EditRoomWidget doesn't call UpdateRect() on calls to Animate().
	//This is to remove flickering effects caused by redrawing the room,
	//temporarily erasing screen effects (e.g. tooltips) that get drawn over the room widget.
	this->pRoomWidget->UpdateRect();
}

//*****************************************************************************
void CEditRoomScreen::OnClick(
//Called when screen receives a click event.
//
//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_ESCAPE:
		case TAG_ESC:
			//Handle some cleanup on editor exit.
			UnloadPlaytestSession();
			GoToScreen(SCR_Return);
		break;
		case TAG_HELP:
			//F1 -- Help
			CBrowserScreen::SetPageToLoad("editroom.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_UNDO:
			UndoCommand(true);
		break;

		case TAG_REDO:
			UndoCommand(false);
		break;

		case TAG_ROOM:
			if ((SDL_GetModState() & KMOD_CTRL) == 0)
				ClickRoom();
		break;
	}
}

//*****************************************************************************
void CEditRoomScreen::OnDeactivate()
//When screen is being exited, make sure everything that has been modified gets saved.
{
	if (this->eState != ES_PLACING)
	{
		if (!this->pCharacterDialog->IsCommandFinished())
			this->pCharacterDialog->FinishCommand(0,0);
		VERIFY(SetState(ES_PLACING));
	}
	SaveRoom();

	//After playtesting and reverting to current player, ensure their key info is current.
	g_pTheNet->ClearActiveAction();

	//Save updated persistent player info.
	{
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	CDbPackedVars& vars = pCurrentPlayer->Settings;

	//Save chat preference.
	vars.SetVar(Settings::EnableChatInGame, this->bEnableChat);
	vars.SetVar(Settings::ReceiveWhispersOnlyInGame, this->bReceiveWhispersOnly);

	pCurrentPlayer->Update();
	delete pCurrentPlayer;
	}

	CRoomScreen::OnDeactivate();
}

//*****************************************************************************
void CEditRoomScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	if (this->eState == ES_PLACING)
		CScreen::OnKeyDown(dwTagNo, Key);

	//Check for other keys.
	switch (Key.keysym.sym)
	{
		case SDLK_ESCAPE:
			//Handle some cleanup on screen exit.
			if (this->eState != ES_PLACING)
			{
				if (this->eState == ES_GETSQUARE || this->eState == ES_GETRECT) break;
				VERIFY(SetState(ES_PLACING));
				Paint(); //redraw room highlights
			} else {
				UnloadPlaytestSession();
			}
		break;

		//Show room/game stats.
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (!(Key.keysym.mod & (KMOD_ALT|KMOD_CTRL)))
			{
				ShowCursor();
				g_pTheSound->PlaySoundEffect(SEID_BUTTON);
				DisplayChatDialog();
			} else if (Key.keysym.mod & (KMOD_CTRL)) {
				ShowChatHistory(this->pEntranceBox);
			}
		break;

		case SDLK_F1:
			CBrowserScreen::SetPageToLoad("editroom.html");
			GoToScreen(SCR_Browser);
		break;

		//dev keys
		case SDLK_F2:
			//Output all scripts where each hold var (or Alt=challenge) is referenced.
			g_pTheSound->PlaySoundEffect(SEID_MIMIC);
			SetCursor(CUR_Wait);
			g_pTheDB->Holds.LogScriptVarRefs(this->pHold->dwHoldID, (Key.keysym.mod & KMOD_ALT) != 0);
			SetCursor();
		break;
		case SDLK_F3:
			ForceFullStyleReload();
		break;

		case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
			if (Key.keysym.mod & (KMOD_ALT | KMOD_CTRL))
			{
				//Save on exit.
				SaveRoom();
			}
		break;

		case SDLK_F5:
			SetState(ES_TESTROOM);	//no verify
		break;

		case SDLK_F6:
		{
			if (!SetState(ES_PLACING)) break;

			CScreen *pScreen = g_pTheSM->GetScreen(SCR_Demos);
			if (!pScreen)
			{
				ShowOkMessage(MID_CouldNotLoadResources);
				break;
			}
			CDemosScreen *pDemosScreen = DYN_CAST(CDemosScreen*, CScreen*, pScreen);
			ASSERT(pDemosScreen);
			pDemosScreen->ShowRoom(this->pRoom->dwRoomID);
			GoToScreen(SCR_Demos);
		}
		break;

		case SDLK_F7:
			if (!SetState(ES_PLACING)) break;
			ReflectRoomX();
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
			this->pRoomWidget->bCeilingLightsRendered = false;
			this->pRoomWidget->RenderRoomLighting();
			Paint();
		break;

		case SDLK_F8:
			if (!SetState(ES_PLACING)) break;
			ReflectRoomY();
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
			this->pRoomWidget->bCeilingLightsRendered = false;
			this->pRoomWidget->RenderRoomLighting();
			Paint();
		break;

		case SDLK_F9:
			Changing(RoomAndHold);
			if (Key.keysym.mod & (KMOD_SHIFT)) {
				GetOverheadImageID(true);
			} else {
				GetFloorImageID(true);
			}
		break;

		case SDLK_PAGEUP: case SDLK_KP_9:
			WarpToNextLevel(false);
		break;
		case SDLK_PAGEDOWN: case SDLK_KP_3:
			WarpToNextLevel(true);
		break;

		case SDLK_SPACE:
			//Remove chat subtitles.
			this->pRoomWidget->RemoveLastLayerEffectsOfType(ECHATTEXT);
		break;

		//Menu tab hotkeys.
		case SDLK_LEFT:
		case SDLK_KP_4:
		case SDLK_RIGHT:
		case SDLK_KP_6:
			if ((Key.keysym.mod & KMOD_CTRL) && dwTagNo != this->pTabbedMenu->GetTagNo())
			{
				this->pRoomWidget->RemoveLastLayerEffectsOfType(ETOOLTIP);
				this->pTabbedMenu->HandleKeyDown(Key);

				//Set focus to new menu.
				CObjectMenuWidget *pMenu = GetActiveMenu();
				ASSERT(pMenu);
				SelectWidget(pMenu);
			}
		break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
			if ((Key.keysym.mod & KMOD_CTRL))
			{
				this->pRoomWidget->RemoveLastLayerEffectsOfType(ETOOLTIP);
				UINT tab;
				switch (Key.keysym.sym)
				{
					default:
					case SDLK_1: tab = 0; break;
					case SDLK_2: tab = 1; break;
					case SDLK_3: tab = 2; break;
					case SDLK_4: tab = 3; break;
				}
				this->pTabbedMenu->SelectTab(tab);

				//Set focus to new menu.
				CObjectMenuWidget *pMenu = GetActiveMenu();
				ASSERT(pMenu);
				SelectWidget(pMenu);
			}
		break;

		case SDLK_x:
		case SDLK_c:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0) break;

			if (this->eState != ES_PLACING) break;

			//Only allow paste when mouse button is down in room widget
			//when Ctrl-C is pressed.
			StopKeyRepeating();  //don't repeat this operation
			CWidget *pWidget = MouseDraggingInWidget();
			if (pWidget && pWidget->GetTagNo() == TAG_ROOM && !RightMouseButton())
			{
				ASSERT(this->pRoomWidget->wEndX != static_cast<UINT>(-1));
				g_pTheSound->PlaySoundEffect(SEID_POTION);

				//Store coords of last selected room region.
				this->wCopyX1 = this->pRoomWidget->wMidX;
				this->wCopyY1 = this->pRoomWidget->wMidY;
				this->wCopyX2 = this->pRoomWidget->wEndX;
				this->wCopyY2 = this->pRoomWidget->wEndY;
				delete this->pCopyRoom;

				{
				CImportInfo info;
				const UINT roomDataID = this->pRoom->dwDataID;
				const UINT roomOverheadDataID = this->pRoom->dwOverheadDataID;
				this->pRoom->dwDataID = this->pRoom->dwOverheadDataID = 0; //don't copy room media data
				this->pCopyRoom = this->pRoom->MakeCopy(info, 0, true); //same hold
				this->pRoom->dwDataID = roomDataID;
				this->pRoom->dwOverheadDataID = roomOverheadDataID;
				}

				this->pCopyRoom->dwRoomID = this->pRoom->dwRoomID;
				this->pCopyRoom->dwOverheadDataID = this->pRoom->dwOverheadDataID;
				this->pCopyRoom->dwLevelID = this->pRoom->dwLevelID;

				if ((this->bCutAndPaste = (Key.keysym.sym == SDLK_x)))
					EraseRegion();

				this->bAreaJustCopied = true;
				ReadyToPasteRegion();

				//Stop any plotting effects.
				this->pRoomWidget->RemoveLastLayerEffectsOfType(EPENDINGPLOT);
				this->pRoomWidget->RemoveLastLayerEffectsOfType(ETRANSTILE);
			} else {
				CWidget *pWidget = GetSelectedWidget();
				ASSERT(pWidget);
				if (pWidget->GetTagNo() == TAG_MAP)
				{
					//Require user to save room changes before making copy of room.
					if (SaveRoomToDB())
					{
						CMapWidget *pMap = DYN_CAST(CMapWidget*, CWidget*, GetWidget(TAG_MAP));
						ASSERT(pMap);
						pMap->CopyRoom(Key.keysym.sym == SDLK_c); //Ctrl-C copies
					}
				}
			}
		}
		break;

		case SDLK_v:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0) break;

			if (this->eState != ES_PLACING) break;

			StopKeyRepeating();  //don't repeat this operation
			CWidget *pWidget = GetSelectedWidget();
			ASSERT(pWidget);
			switch (pWidget->GetTagNo())
			{
				case TAG_MAP:
				{
					//Paste a room.
					if (!SaveRoomToDB()) return;
					if (this->pMapWidget->IsDeletingRoom())
					{
						//Not allowed to delete level entrance room.
						UINT dwSX, dwSY, dwMapX, dwMapY;
						ASSERT(this->pLevel);
						this->pLevel->GetStartingRoomCoords(dwSX, dwSY);
						this->pMapWidget->GetSelectedRoomXY(dwMapX, dwMapY);
						if (dwMapX == dwSX && dwMapY == dwSY)
						{
							ShowOkMessage(MID_CantDeleteEntranceRoom);
							break;
						} else {
							if (ShowYesNoMessage(MID_DeleteRoomPrompt) != TAG_YES)
								break;
						}
					}
					this->bRoomDirty = false;  //don't save old room
					const bool bUpdate = this->pMapWidget->PasteRoom(this->pHold);
					if (bUpdate)
					{
						//Refresh level instance to resynch data.
						const UINT dwLevelID = this->pLevel->dwLevelID;
						delete this->pLevel;
						this->pLevel = g_pTheDB->Levels.GetByID(dwLevelID);

						//Update hold's and level's timestamp.
						//this->pHold->Update(); //hold was updated in PasteRoom().
						this->pLevel->Update();

						UINT dwRoomX, dwRoomY;
						this->pMapWidget->GetSelectedRoomXY(dwRoomX, dwRoomY);
						this->pMapWidget->LoadFromLevel(this->pLevel);
						LoadRoomAtCoords(dwRoomX, dwRoomY, true);
						return;
					}
				}
				break;

				default: break;
			}

			//If a room wasn't just pasted, Ctrl-V will indicate that the next
			//click will paste the room region previously selected for copying.
			if (this->wCopyX1 != (UINT)-1)
			{
				//Paste destination is at mouse's current location.
				int nMouseX, nMouseY;
				GetMouseState(&nMouseX, &nMouseY);
				this->pRoomWidget->wEndX = (nMouseX - this->pRoomWidget->GetX()) / CBitmapManager::CX_TILE;
				this->pRoomWidget->wEndY = (nMouseY - this->pRoomWidget->GetY()) / CBitmapManager::CY_TILE;

				ReadyToPasteRegion();
			}
		}
		break;

		case SDLK_z:
			if (Key.keysym.mod & KMOD_CTRL)
				UndoCommand(true);
		break;
		case SDLK_y:
			if (Key.keysym.mod & KMOD_CTRL)
				UndoCommand(false);
		break;

		default: break;
	}

	switch (dwTagNo)
	{
		//Editor options.  (Copied from OnClick().)
		case TAG_SHOWERRORS:
		{
			COptionButtonWidget *pButton = DYN_CAST(COptionButtonWidget*, CWidget*,
					GetWidget(TAG_SHOWERRORS));
			this->bShowErrors = pButton->IsChecked();
			//Update room error display.
			Paint();
		}
		break;
	}

	//Check for command keys.
	const UINT wOldO = this->wO;
	const UINT wOldObType = this->wSelectedObType;
	const UINT wOldLightType = this->wSelectedLightType;
	const UINT wOldDarkType = this->wSelectedDarkType;
	const UINT wOldOrbType = this->wSelOrbType, wOldPlateType = this->wSelPlateType;
	const UINT wOldTokenType = this->wSelTokenType;
	const UINT wOldWaterType = this->wSelWaterType;
	if ((Key.keysym.mod & KMOD_CTRL) == 0)
	{
		const int nCommand = GetCommandForKeysym(Key.keysym.sym);
		switch (nCommand)
		{
			//Rotate orientation.
			case CMD_C:
				RotateClockwise();
				break;
			case CMD_CC:
				RotateCounterClockwise();
				break;
			case CMD_UNDO:
				UndoCommand(true);
				break;
			case CMD_WAIT:
				UndoCommand(false);
				break;
		}
	}

	if (this->eState == ES_PASTING)
		return;

	if (wOldO != this->wO || wOldObType != this->wSelectedObType ||
			wOldLightType != this->wSelectedLightType ||
			wOldDarkType != this->wSelectedDarkType ||
			wOldOrbType != this->wSelOrbType || wOldPlateType != this->wSelPlateType ||
			wOldTokenType != this->wSelTokenType ||
			wOldWaterType != this->wSelWaterType)
	{
		//Update room widget if a plot is occurring.
		CWidget *pFocusWidget = MouseDraggingInWidget();
		if (pFocusWidget)
			if (pFocusWidget->GetTagNo() == TAG_ROOM)
			{
				//Mouse drag is occurring.
				if (!RightMouseButton())
					//Update objects being plotted.
					AddPlotEffect(eState == ES_TESTROOM ? T_SWORDSMAN :
							GetSelectedObject());
			}
	}
}

//*****************************************************************************
void CEditRoomScreen::OnMouseDown(
//Called when widget receives a mouse down event.
//
//Params:
	const UINT dwTagNo, const SDL_MouseButtonEvent &Button)
{
	CScreen::OnMouseDown(dwTagNo, Button);

	switch (dwTagNo)
	{
		case TAG_ROOM:
			switch (this->eState)
			{
			case ES_PLACING:
				if (RightMouseButton())
					AddPlotEffect(T_EMPTY); //Deleting object
				else
					AddPlotEffect(GetSelectedObject());  //Plotting object
				break;
			case ES_TESTROOM:
				AddPlotEffect(T_SWORDSMAN);
				break;
			case ES_GETRECT:
				this->pRoomWidget->bSinglePlacement = false; //region is being selected
				break;
			default: break;
			}
		break;
		default: break;
	}
}

//*****************************************************************************
void CEditRoomScreen::OnMouseMotion(
//Called when widget receives a mouse move event.
//
//Params:
	const UINT dwTagNo, const SDL_MouseMotionEvent &Motion)
{
	CScreen::OnMouseMotion(dwTagNo, Motion);

	switch (dwTagNo)
	{
	case TAG_ROOM:
		switch (this->eState)
		{
			case ES_LONGMONSTER:
			{
				const UINT monster_type = this->wSelectedObject - M_OFFSET;
				//Plot pending long monster segment.
				if (!bIsSerpentOrGentryii(monster_type))
				{
					//Remove part of monster placed so far.
					if (this->pLongMonster)
						RemoveMonster(this->pLongMonster);
					VERIFY(SetState(ES_PLACING));
					Paint();
					break;
				}

				//Keep monster type synchronized with the current (valid) menu choice.
				if (this->pLongMonster && this->pLongMonster->wType != monster_type)
				{
					this->pLongMonster->wType = monster_type;
					this->pRoomWidget->DirtyRoom();
				}
				this->pRoomWidget->AddMonsterSegmentEffect(this->pLongMonster, monster_type);
			}
			break;
			case ES_PASTING:
				HighlightPendingPaste();
			break;
			case ES_PLACING:
				ShowPlot();
			break;
			case ES_TESTROOM:
			{
				CWidget *pFocusWidget = MouseDraggingInWidget();
				if (pFocusWidget)
					//Mouse drag.
					if (pFocusWidget->GetTagNo() == dwTagNo)
						AddPlotEffect(T_SWORDSMAN);
			}
			break;

			case ES_GETRECT:
				//Highlight room region used to answer query.
				if (MouseDraggingInWidget())
				{
					const UINT wStartX = min(this->pRoomWidget->wStartX, this->pRoomWidget->wEndX);
					const UINT wStartY = min(this->pRoomWidget->wStartY, this->pRoomWidget->wEndY);
					const UINT wEndX = max(this->pRoomWidget->wStartX, this->pRoomWidget->wEndX);
					const UINT wEndY = max(this->pRoomWidget->wStartY, this->pRoomWidget->wEndY);
					PaintHighlights(); //do first
					this->pRoomWidget->AddPendingPasteEffect(wStartX, wStartY,
							wEndX - wStartX, wEndY - wStartY, PaleYellow, false);
				}
			break;

			default: break;
		}
	break;
	default: break;
	}
}

//*****************************************************************************
void CEditRoomScreen::OnMouseUp(
//Called when widget receives a mouse up event.
//
//Params:
	const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &/*Button*/)
{
	//Any plotting is now finished.
	this->pRoomWidget->RemoveLastLayerEffectsOfType(EPENDINGPLOT);
	if (this->eState != ES_LONGMONSTER) //possibly starting to place serpent
		this->pRoomWidget->RemoveLastLayerEffectsOfType(ETRANSTILE);
}

//*****************************************************************************
void CEditRoomScreen::OnMouseWheel(
//Called when a mouse wheel event is received.
//
//Params:
	const SDL_MouseWheelEvent &Wheel)
{
	//Mouse wheel changes orientation.
	if (Wheel.y < 0 || Wheel.x > 0)
	{
		RotateClockwise();
	}
	else if (Wheel.y > 0 || Wheel.x < 0)
	{
		RotateCounterClockwise();
	}

	//Don't display the active object in the region being selected or change the selection parameters.
	if (this->eState == ES_GETSQUARE || this->eState == ES_GETRECT)
		return;

	//Update room widget if a plot is occurring.
	CWidget *pFocusWidget = MouseDraggingInWidget();
	if (pFocusWidget)
		if (pFocusWidget->GetTagNo() == TAG_ROOM)
		{
			//Mouse drag is occurring.
			//Update objects being plotted.
			AddPlotEffect(eState == ES_TESTROOM ? T_SWORDSMAN :
					GetSelectedObject());
		}
}

//*****************************************************************************
void CEditRoomScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_MAP:
		{
			//Selected room on map has changed.

			//Don't change rooms if in the middle of customizing something.
			if (this->eState != ES_PLACING || !this->pCharacterDialog->IsCommandFinished())
			{
				//Put the map selection back on the current room.
				this->pMapWidget->SelectRoom(this->pRoom->dwRoomX,
						this->pRoom->dwRoomY);
				this->pMapWidget->RequestPaint();
				break;
			}

			UINT dwRoomX, dwRoomY;
			this->pMapWidget->RequestPaint();
			this->pMapWidget->GetSelectedRoomXY(dwRoomX, dwRoomY);
			if (!this->pMapWidget->bVacantRoom)
				LoadRoomAtCoords(dwRoomX, dwRoomY);
			else
			{
				if (ShowYesNoMessage(MID_AddRoomPrompt) == TAG_YES &&
						SaveRoomToDB())
				{
					SaveRoom();  //need?
					//If hold copy was made, a different roomY position is required.
					dwRoomY = (this->pRoom->dwLevelID * 100) + (dwRoomY % 100);
					AddRoom(dwRoomX,dwRoomY);
				} else if (!this->pMapWidget->ReadyToPasteRoom()) {
					//Put the map selection back on the current room.
					this->pMapWidget->SelectRoom(this->pRoom->dwRoomX,
							this->pRoom->dwRoomY);
					this->pMapWidget->RequestPaint();
				}
			}
		}
		break;

		case TAG_MENU:
		{
			//When a new tab is selected, update the selected object from
			//the visible object menu.
			CObjectMenuWidget *pMenu = GetActiveMenu();
			ASSERT(pMenu);
			const UINT wObject = pMenu->GetSelectedObject();
			if (wObject != NO_SELECTED_OBJECT)
			{
				//Set actual light level when light is (un)selected.
				const bool bLight = bIsLightingTile(this->wSelectedObject) || bIsLightingTile(wObject);

				SetSelectedObject(wObject);

				if (bLight)
					SetLightLevel();
			}
		}
		break;

		case TAG_OMENU:
		case TAG_FMENU:
		case TAG_TMENU:
		case TAG_MMENU:
		{
			//Object selection.
			if (g_pTheSM->bTransitioning)
				break; //ignore item selection changes while screen is activating
			const CObjectMenuWidget* pMenu = DYN_CAST(const CObjectMenuWidget*, CWidget*, GetWidget(dwTagNo));
			const UINT wObject = pMenu->GetSelectedObject();
			if (wObject != NO_SELECTED_OBJECT)
			{
				//Is selecting the same object, then rotate to the next item type or orientation.
				if (this->wSelectedObject == wObject)
				{
					IncrementMenuSelection(!RightMouseButton());
					break;
				}

				//Set actual light level when light is (un)selected.
				const bool bLight = bIsLightingTile(this->wSelectedObject) || bIsLightingTile(wObject);

				SetSelectedObject(wObject);

				if (bLight)
					SetLightLevel();
			}
		}
		break;

		case TAG_SHOWERRORS:
		{
			CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
			if (!pCurrentPlayer) {ASSERT(!"Could not retrieve player."); break;} //Corrupt db.

			COptionButtonWidget *pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
					GetWidget(TAG_SHOWERRORS));
			this->bShowErrors = pOptionButton->IsChecked();
			pCurrentPlayer->Settings.SetVar(Settings::ShowErrors, this->bShowErrors);

			pCurrentPlayer->Update();
			delete pCurrentPlayer;

			//Update room error display.
			Paint();
		}
		break;

		case TAG_TEXTBOX2D:
			if (this->eState == ES_SCROLL)
			{
				CTextBox2DWidget *pTextBox = DYN_CAST(CTextBox2DWidget*, CWidget*,
					this->pInputTextDialog->GetWidget(TAG_TEXTBOX2D));
				this->pScrollLabel->SetText(pTextBox->GetText());
				ShowScroll();
			}
		break;

		case TAG_CHATINPUT:
			ReformatChatText(TAG_CHATINPUT, true);
		break;
	}
}

//*****************************************************************************
void CEditRoomScreen::PopulateItemMenus()
//If requested, collapse menu items into groups.
//Otherwise, show all menu items.
{
	UINT index;
	CObjectMenuWidget *pObjectMenu;

	//O-layer menu.
	pObjectMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, this->pTabbedMenu->GetWidget(TAG_OMENU));
	pObjectMenu->ClearObjects();
	if (this->bGroupMenuItems)
	{
		for (index=0; index<numOLayerGroupedEntries; ++index)
			AddObjectToMenu(oLayerGroupedEntries[index]);
	} else {
		for (index=0; index<numOLayerFullEntries; ++index)
			AddObjectToMenu(oLayerFullEntries[index]);
	}

	//F-layer entries are the same either way.
	pObjectMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, this->pTabbedMenu->GetWidget(TAG_FMENU));
	pObjectMenu->ClearObjects();
	for (index=0; index<numFLayerEntries; ++index)
		AddObjectToMenu(fLayerEntries[index]);
	pObjectMenu->SetObjectLight(T_WALLLIGHT,
			wLightMult*lightMap[0][this->wSelectedLightType], wLightMult*lightMap[1][this->wSelectedLightType],
			wLightMult*lightMap[2][this->wSelectedLightType]);
	pObjectMenu->SetObjectLight(T_LIGHT_CEILING,
			wLightMult*lightMap[0][this->wSelectedLightType], wLightMult*lightMap[1][this->wSelectedLightType],
			wLightMult*lightMap[2][this->wSelectedLightType]);
	pObjectMenu->SetObjectLight(T_DARK_CEILING,
			wDarkMult*darkMap[this->wSelectedDarkType], wDarkMult*darkMap[this->wSelectedDarkType],
			wDarkMult*darkMap[this->wSelectedDarkType]);

	//T-layer menu.
	pObjectMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, this->pTabbedMenu->GetWidget(TAG_TMENU));
	pObjectMenu->ClearObjects();
	if (this->bGroupMenuItems)
	{
		for (index=0; index<numTLayerGroupedEntries; ++index)
			AddObjectToMenu(tLayerGroupedEntries[index]);
	} else {
		for (index=0; index<numTLayerFullEntries; ++index)
			AddObjectToMenu(tLayerFullEntries[index]);
	}
	pObjectMenu->SetObjectLight(T_LIGHT,
			wLightMult*lightMap[0][this->wSelectedLightType], wLightMult*lightMap[1][this->wSelectedLightType],
			wLightMult*lightMap[2][this->wSelectedLightType]);
	pObjectMenu->SetObjectLight(T_STATION,
			wLightMult*lightMap[0][this->wSelectedLightType], wLightMult*lightMap[1][this->wSelectedLightType],
			wLightMult*lightMap[2][this->wSelectedLightType]);

	//M-layer menu.
	pObjectMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, this->pTabbedMenu->GetWidget(TAG_MMENU));
	pObjectMenu->ClearObjects();
	for (index=0; index<numMLayerEntries; ++index)
		AddObjectToMenu(mLayerEntries[index]);

	SetSelectedObject(T_WALL); //first object is active
	this->pTabbedMenu->SelectTab(0);
}

//*****************************************************************************
UINT CEditRoomScreen::SelectMediaID(
//UI for importing, deleting and selecting media data belonging to this hold.
//
//Params:
	const UINT dwDefault, //default selected value
	const CEntranceSelectDialogWidget::DATATYPE eType) //media type
{
	MESSAGE_ID midPrompt = 0;
	switch (eType)
	{
		case CEntranceSelectDialogWidget::Images: midPrompt = MID_ImageSelectPrompt; break;
		case CEntranceSelectDialogWidget::Sounds: midPrompt = MID_SoundSelectPrompt; break;
		case CEntranceSelectDialogWidget::Videos: midPrompt = MID_VideoSelectPrompt; break;
		default: ASSERT(!"UI for this media type not implemented"); return 0;
	}
	ASSERT(midPrompt);

	ASSERT(this->pHold);

	UINT dwVal;
	bool bDeletedDefault = false;
	CEntranceSelectDialogWidget::BUTTONTYPE eButton;
	do {
		dwVal = (bDeletedDefault ? 0 : dwDefault);
		eButton = SelectListID(this->pEntranceBox, this->pHold,
				dwVal, midPrompt, eType);

		if (eButton == CEntranceSelectDialogWidget::Delete)
		{
			//Remove this media object from the database and make another selection.
			//It's okay if other references to this object remain set to this old record ID.
			//They will robustly default to do nothing.
			this->pHold->MarkDataForDeletion(dwVal);
			if (dwVal == dwDefault)
				bDeletedDefault = true;
		}
	} while (eButton == CEntranceSelectDialogWidget::Delete);

	const bool bSelected = eButton == CEntranceSelectDialogWidget::OK;
	if (bSelected && !dwVal)
	{
		//Import media from disk into this hold.
		switch (eType)
		{
			case CEntranceSelectDialogWidget::Images:
				dwVal = ImportHoldImage(this->pHold->dwHoldID, EXT_JPEG | EXT_PNG);
			break;
			case CEntranceSelectDialogWidget::Sounds:
				dwVal = ImportHoldSound();
			break;
			case CEntranceSelectDialogWidget::Videos:
				dwVal = ImportHoldVideo();
			break;
			default: break;
		}
	}
	return bSelected ? dwVal : 0;
}

//*****************************************************************************
void CEditRoomScreen::EditObjects()
//Modify the state of an object in the room.
{
	const UINT wX = this->pRoomWidget->wEndX, wY = this->pRoomWidget->wEndY;
	const UINT wOTileNo = this->pRoom->GetOSquare(wX,wY);
	const UINT wTTileNo = this->pRoom->GetTSquare(wX,wY);
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);

	//Edit a monster.
	if (pMonster && this->pTabbedMenu->GetSelectedTab() == MONSTER_TAB)
	{
		pMonster = pMonster->GetOwningMonster();

		if (pMonster->wType == M_GENTRYII) {
			EditGentryii(pMonster);
			return;
		}
		//Is a serpent here?
		if (pMonster->IsLongMonster())
		{
			EditSerpent(pMonster);
			return;
		}

		//Reorient monster when Shift is held.
		const bool bShift = (SDL_GetModState() & KMOD_SHIFT) != 0;
		if (bShift)
		{
			if (pMonster->HasOrientation())
			{
				Changing(Room);
				pMonster->wO = this->wO;
			}
			return;
		}

		//Reorient character and edit character command script.
		if (pMonster->wType == M_CHARACTER)
		{
			Changing(RoomAndHold);
			if (this->wSelectedObject == T_CHARACTER)
				pMonster->wO = this->wO; //reorient when character is selected

			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			ASSERT(pCharacter->dwScriptID);
			ASSERT(pCharacter->dwScriptID <= this->pHold->GetScriptID());
			this->pCharacterDialog->EditCharacter(pCharacter);
			this->pCharacterDialog->Display();

			//Refresh visual appearance (for sword display).
			pCharacter->wIdentity = pCharacter->wLogicalIdentity; //default
			pCharacter->ResolveLogicalIdentity(this->pHold);

			Paint();
			return;
		}
	}

	if (this->pTabbedMenu->GetSelectedTab() == T_LAYER_TAB)
		switch (wTTileNo)
		{
			case T_SCROLL:
				EditScrollText(wX,wY);
			return;

			case T_LIGHT:
			{
				Changing();

				UINT wTParam = this->pRoom->GetTParam(wX,wY);
				if (this->wSelectedLightType == (calcLightType(wTParam)))
				{
					//Increase/decrease radius.
					if ((SDL_GetModState() & KMOD_SHIFT) != 0)
						wTParam += (MAX_LIGHT_DISTANCE-1)*NUM_LIGHT_TYPES;
					else
						wTParam += NUM_LIGHT_TYPES;
					wTParam = wTParam % (MAX_LIGHT_DISTANCE*NUM_LIGHT_TYPES); //max radius of 1+MAX_LIGHT_DISTANCE
				} else {
					//Change light color.
					wTParam = (wTParam/NUM_LIGHT_TYPES)*NUM_LIGHT_TYPES + this->wSelectedLightType;
				}
				this->pRoom->SetTParam(wX,wY, wTParam);
				this->pRoomWidget->RenderRoomLighting();
				Paint();
			}
			return;
		}

	if (this->pTabbedMenu->GetSelectedTab() == F_LAYER_TAB)
	{
		UINT wLightParam = this->pRoom->tileLights.GetAt(wX,wY);
		//Alter value of wall-light.
		if (bIsWallLightValue(wLightParam) && GetSelectedObject() != T_SWORDSMAN)
		{
			Changing();

			if (this->wSelectedLightType == (calcLightType(wLightParam)))
			{
				//Increase/decrease radius.
				if ((SDL_GetModState() & KMOD_SHIFT) != 0)
					wLightParam += (MAX_LIGHT_DISTANCE-1)*NUM_LIGHT_TYPES;
				else
					wLightParam += NUM_LIGHT_TYPES;
			} else {
				//Change light color.
				wLightParam = (wLightParam/NUM_LIGHT_TYPES)*NUM_LIGHT_TYPES + this->wSelectedLightType;
			}

			wLightParam = wLightParam % (MAX_LIGHT_DISTANCE*NUM_LIGHT_TYPES); //max radius of 1+MAX_LIGHT_DISTANCE
			ASSERT(wLightParam < WALL_LIGHT);
			this->pRoom->tileLights.Add(wX,wY, WALL_LIGHT + wLightParam);

			this->pRoomWidget->RenderRoomLighting();
			Paint();
			return;
		}
	}

	//Edit the below customizable item types if on this tile regardless of what menu layer is active.

	bool bSwordsmanAt, bSwordAtIgnored;
	this->pRoomWidget->IsLevelStartAt(wX, wY, bSwordsmanAt, bSwordAtIgnored);
	if (bSwordsmanAt)
	{
		EditLevelEntrance(wX, wY);
		return;
	}

	switch (wTTileNo)
	{
		case T_ORB:
			EditOrbAgents(wX,wY);
		return;
		default: break;
	}
	switch (wOTileNo)
	{
		case T_STAIRS:
		case T_STAIRS_UP:
		{
			SetDestinationEntrance(wX,wY,wX,wY);
			PaintHighlights();
		}
		return;

		case T_DOOR_Y:
		case T_DOOR_YO:
		{
			//Edit door's "orb" agents.
			g_pTheSound->PlaySoundEffect(SEID_ORBHIT);
			VERIFY(SetState(ES_DOOR));
			SetOrbAgentsForDoor(wX,wY);
		}
		return;
		case T_PRESSPLATE:
			EditOrbAgents(wX,wY);
		return;
		default: break;
	}
}

//*****************************************************************************
void CEditRoomScreen::EditOrbAgents(const UINT wX, const UINT wY)
//Edit door agents for an orb or pressure plate.
{
	g_pTheSound->PlaySoundEffect(SEID_ORBHIT);
	VERIFY(SetState(ES_ORB));
	const UINT wOSquare = this->pRoom->GetOSquare(wX,wY);
	if (wOSquare == T_PRESSPLATE)
		this->pOrb = this->pRoom->GetPressurePlateAtCoords(wX,wY);
	else
		this->pOrb = this->pRoom->GetOrbAtCoords(wX,wY);
	if (!this->pOrb)
	{
		//Add an orb record to the room.
		this->pOrb = this->pRoom->AddOrbToSquare(wX,wY);
		this->pOrb->eType = (OrbType)(wOSquare == T_PRESSPLATE ?
							this->wSelPlateType : this->wSelOrbType);
	}
	this->pRoomWidget->AddOrbAgentsEffect(this->pOrb);
}

//*****************************************************************************
void CEditRoomScreen::EditLevelEntrance(const UINT wX, const UINT wY)
{
	CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID, wX, wY);
	ASSERT(pEntrance);
	WSTRING wstrDescription = static_cast<WSTRING>(pEntrance->DescriptionText);
	bool bMainEntrance = pEntrance->bIsMainEntrance;
	CEntranceData::DescriptionDisplay showDescription = pEntrance->eShowDescription;
	UINT dataID = pEntrance->dwDataID;

	if (!EditLevelEntrance(wstrDescription, bMainEntrance, showDescription, dataID))
		return;

	Changing(Hold); //add current hold entrances state to undo list

	pEntrance->eShowDescription = showDescription;
	pEntrance->dwDataID = dataID;

	//Level entrance orientation changed?
	const bool bChangedEntranceOrientation = pEntrance->wO != this->wO &&
			this->wSelectedObject == T_SWORDSMAN; //only rotate when entrance is selected
	if (bChangedEntranceOrientation)
	{
		pEntrance->wO = this->wO;
		GetLevelEntrancesInRoom();
	}

	if (!pEntrance->bIsMainEntrance && bMainEntrance)
	{
		//This entrance is now the new main level entrance.
		CEntranceData *pOldMainEntrance = this->pHold->GetMainEntranceForLevel(
				this->pRoom->dwLevelID);
		ASSERT(pOldMainEntrance);
		if (pOldMainEntrance)
			pOldMainEntrance->bIsMainEntrance = false;
		pEntrance->bIsMainEntrance = true;

		this->pLevel->SetNewStartingRoomID(this->pRoom->dwRoomID);

		//Update sign to reflect new room coords origin (before anything is saved).
		SetSignTextToCurrentRoom(true);
		PaintSign();
		UpdateRect();
	}
	pEntrance->DescriptionText = wstrDescription.c_str();
}

//*****************************************************************************
bool CEditRoomScreen::EditLevelEntrance(
//Edit the inputted data for use in a level entrance.
//
//Params:
	WSTRING &wstrDescription, bool &bMainEntrance,
	CEntranceData::DescriptionDisplay &eShowEntranceDesc,
	UINT &dataID)
{
	CTextBox2DWidget *pTextBox2D = DYN_CAST(CTextBox2DWidget*, CWidget*,
			this->pLevelEntranceDialog->GetWidget(TAG_LEVELENTRANCETEXTBOX));
	pTextBox2D->SetText(wstrDescription.c_str());

	COptionButtonWidget *pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			this->pLevelEntranceDialog->GetWidget(TAG_MAINENTRANCE));
	pOpButton->Enable(!bMainEntrance);
	pOpButton->SetChecked(bMainEntrance);
	COptionButtonWidget *pOpButton_ShowDesc = DYN_CAST(COptionButtonWidget*,
			CWidget*, this->pLevelEntranceDialog->GetWidget(TAG_SHOWDESCRIPTION));
	pOpButton_ShowDesc->SetChecked(eShowEntranceDesc != CEntranceData::DD_No);

	bool bShowEntranceDescOnce = eShowEntranceDesc == CEntranceData::DD_Once;
	if (bShowEntranceDescOnce)
		pOpButton_ShowDesc->SetChecked(true);
	COptionButtonWidget *pOpButton_ShowDescOnce = DYN_CAST(COptionButtonWidget*,
			CWidget*, this->pLevelEntranceDialog->GetWidget(TAG_SHOWDESCRIPTION_ONCE));
	pOpButton_ShowDescOnce->SetChecked(bShowEntranceDescOnce);

	//Activate the dialog widget.
	bool bWasCursorVisible = IsCursorVisible();
	ShowCursor();

	UINT dwTagNo;
	UINT newDataID = dataID;
	bool bLoop=true;
	do {
		CButtonWidget *pButton = DYN_CAST(CButtonWidget*,
				CWidget*, this->pLevelEntranceDialog->GetWidget(TAG_LEVELENTRANCEAUDIO));
		pButton->SetCaption(g_pTheDB->GetMessageText(newDataID ? MID_RemoveSound : MID_AddSound));

		CLabelWidget *pSoundName = DYN_CAST(CLabelWidget*,
				CWidget*, this->pLevelEntranceDialog->GetWidget(TAG_LEVELENTRANCEAUDIONAME));
		const WSTRING soundName = CDbData::GetNameFor(newDataID);
		pSoundName->SetText(soundName.empty() && newDataID ? wszQuestionMark : soundName.c_str());

		dwTagNo = this->pLevelEntranceDialog->Display();
		switch (dwTagNo)
		{
			case TAG_LEVELENTRANCEAUDIO:
				newDataID = SelectMediaID(newDataID, CEntranceSelectDialogWidget::Sounds);
			break;
			default:
				bLoop = false;
			break;
		}
	} while (bLoop && !IsDeactivating());

	if (!bWasCursorVisible) HideCursor();

	//Repaint the screen to fix area underneath dialog.
	Paint();
	StopKeyRepeating();
	StopMouseRepeating();

	if (dwTagNo != TAG_OK)
		return false;

	//Set level entrance fields.
	wstrDescription = pTextBox2D->GetText();

	if (!bMainEntrance)
		bMainEntrance = pOpButton->IsChecked();

	eShowEntranceDesc = CEntranceData::DD_No;
	if (pOpButton_ShowDesc->IsChecked())
		eShowEntranceDesc = CEntranceData::DD_Always;
	if (pOpButton_ShowDescOnce->IsChecked())
		eShowEntranceDesc = CEntranceData::DD_Once;

	dataID = newDataID;

	return true;
}

//*****************************************************************************
void CEditRoomScreen::EditGentryii(CMonster* pMonster)
//Resume editing this gentryii.
{
	ASSERT(pMonster);
	if (pMonster->wType != M_GENTRYII)
		return;

	g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);

	this->pRoomWidget->monsterSegment.wHeadX = pMonster->wX;
	this->pRoomWidget->monsterSegment.wHeadY = pMonster->wY;

	//Get tail coordinates.
	UINT tailX = pMonster->wX, tailY = pMonster->wY;
	if (!pMonster->Pieces.empty()) {
		CMonsterPiece *pTailPiece = pMonster->Pieces.back();
		ASSERT(pTailPiece);
		tailX = pTailPiece->wX;
		tailY = pTailPiece->wY;
	}

	this->pRoomWidget->monsterSegment.wTailX = this->pRoomWidget->monsterSegment.wSX = tailX;
	this->pRoomWidget->monsterSegment.wTailY = this->pRoomWidget->monsterSegment.wSY = tailY;

	this->pRoomWidget->monsterSegment.wDirection = pMonster->wO;
	
	//don't need to set bHorizontal, wEX and wEY
	this->pLongMonster = pMonster;
	Changing();
	VERIFY(SetState(ES_LONGMONSTER));
	this->wSelectedObjectSave = this->wSelectedObject;
	this->wSelectedObject = pMonster->wType + M_OFFSET;
}

//*****************************************************************************
void CEditRoomScreen::EditSerpent(CMonster* pMonster)
//Resume editing this serpent.
{
	ASSERT(pMonster);
	if (!bIsSerpent(pMonster->wType))
		return;

	g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
	CSerpent *pSerpent = DYN_CAST(CSerpent*, CMonster *, pMonster);

	//Get tail coordinates.
	UINT wTailX, wTailY;
	pSerpent->GetTail(wTailX,wTailY);

	UINT wDirection, wTailDirection;
	this->pRoomWidget->monsterSegment.wHeadX = pSerpent->wX;
	this->pRoomWidget->monsterSegment.wHeadY = pSerpent->wY;
	this->pRoomWidget->monsterSegment.wTailX = wTailX;
	this->pRoomWidget->monsterSegment.wTailY = wTailY;
	CMonster *pTailMonster = this->pRoom->GetMonsterAtSquare(wTailX,wTailY);
	ASSERT(pTailMonster);
	CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pTailMonster);
	ASSERT(pMPiece);
	switch (pMPiece->wTileNo)
	{
		case T_SNKT_E: wDirection = E;   wTailDirection = W;  break;
		case T_SNKT_W: wDirection = W;   wTailDirection = E;  break;
		case T_SNKT_N: wDirection = N;   wTailDirection = S;  break;
		case T_SNKT_S: wDirection = S;   wTailDirection = N;  break;
		default: ASSERT(!"Bad serpent tile."); return;
	}
	this->pRoomWidget->monsterSegment.wDirection = wDirection;
	PlotLastSerpentSegment(wTailX,wTailY,wTailDirection);
	//don't need to set bHorizontal, wEX and wEY
	this->pLongMonster = pMonster;
	Changing();
	VERIFY(SetState(ES_LONGMONSTER));
	this->wSelectedObjectSave = this->wSelectedObject;
	this->wSelectedObject = pMonster->wType + M_OFFSET;
}

//*****************************************************************************
bool CEditRoomScreen::EditScrollText(
//Edit text on scroll.
//Returns: whether text on scroll was OKed or Cancelled
//
//Params:
	const UINT wX, const UINT wY) //(in)
{
	LOGCONTEXT("CEditRoomScreen::EditScrollText");

	g_pTheSound->PlaySoundEffect(SEID_READ);
	const WCHAR *wStr = this->pRoom->GetScrollTextAtSquare(wX,wY);
	WSTRING wstrDescription;
	if (wStr)
		wstrDescription = wStr;
	this->pScrollLabel->SetText(wStr);
	VERIFY(SetState(ES_SCROLL));
	ShowScroll();
	const UINT dwTagNo = ShowTextInputMessage(MID_EnterScrollText,
			wstrDescription, true);
	const bool bOK = dwTagNo == TAG_OK;
	if (bOK && (!wStr || WCScmp(wstrDescription.c_str(), wStr)))
	{
		//Update string if it has changed.
		Changing();
		this->pRoom->SetScrollTextAtSquare(wX,wY,wstrDescription.c_str());
	}
	VERIFY(SetState(ES_PLACING));
	Paint(); //repaint menu

	return bOK;
}

//*****************************************************************************
void CEditRoomScreen::FillInRoomEdges(
//Adds a border to the room matching that of the adjacent rooms.
//
//Params:
	CDbRoom* const pRoom)   //(in/out) Room to modify
{
	ASSERT(pRoom);

	//Load adjacent rooms.
	CDbRoom *pAdjRoom[4];
	{
		pAdjRoom[0] = g_pTheDB->Rooms.GetByCoords(pRoom->dwLevelID,
				pRoom->dwRoomX,pRoom->dwRoomY-1);   //above
		pAdjRoom[1] = g_pTheDB->Rooms.GetByCoords(pRoom->dwLevelID,
				pRoom->dwRoomX+1,pRoom->dwRoomY);   //to right
		pAdjRoom[2] = g_pTheDB->Rooms.GetByCoords(pRoom->dwLevelID,
				pRoom->dwRoomX,pRoom->dwRoomY+1);   //below
		pAdjRoom[3] = g_pTheDB->Rooms.GetByCoords(pRoom->dwLevelID,
				pRoom->dwRoomX-1,pRoom->dwRoomY);   //to left
		ASSERT(!pAdjRoom[0] || pRoom->wRoomCols == pAdjRoom[0]->wRoomCols);
		ASSERT(!pAdjRoom[1] || pRoom->wRoomRows == pAdjRoom[1]->wRoomRows);
		ASSERT(!pAdjRoom[2] || pRoom->wRoomCols == pAdjRoom[2]->wRoomCols);
		ASSERT(!pAdjRoom[3] || pRoom->wRoomRows == pAdjRoom[3]->wRoomRows);
	}

#define GetSquare(pAdjRoom,wX,wY) (pAdjRoom ? pAdjRoom->GetOSquare(wX,wY) :\
		T_WALL)
#define SetSquare(wTileNo,wX,wY) if (bIsWall(wTileNo) || \
	bIsPit(wTileNo) || bIsWater(wTileNo) || \
	(bIsPlainFloor(wTileNo) && wTileNo != T_FLOOR_IMAGE))\
		pRoom->Plot(wX,wY,wTileNo);

	//Examine adjacent rooms' bordering edge.
	UINT wX, wY, wAdjTile;
	for (wX=pRoom->wRoomCols; wX--; )
	{
		//top
		wAdjTile = GetSquare(pAdjRoom[0], wX, pAdjRoom[0]->wRoomRows-1);
		SetSquare(wAdjTile, wX, 0);
		//bottom
		wAdjTile = GetSquare(pAdjRoom[2], wX, 0);
		SetSquare(wAdjTile, wX, pRoom->wRoomRows-1);
	}
	for (wY=pRoom->wRoomRows; wY--; )
	{
		//left
		wAdjTile = GetSquare(pAdjRoom[3], pAdjRoom[3]->wRoomCols-1, wY);
		if (!bIsPit(pRoom->GetOSquare(0, wY))) //don't overwrite modified corners
			SetSquare(wAdjTile, 0, wY);
		//right
		wAdjTile = GetSquare(pAdjRoom[1], 0, wY);
		if (!bIsPit(pRoom->GetOSquare(pRoom->wRoomCols-1, wY)))
			SetSquare(wAdjTile, pRoom->wRoomCols-1, wY);
	}

	delete pAdjRoom[0];
	delete pAdjRoom[1];
	delete pAdjRoom[2];
	delete pAdjRoom[3];

#undef GetSquare
#undef SetSquare
}

//*****************************************************************************
COrbAgentData* CEditRoomScreen::FindOrbAgentFor(
//Finds which agent of orb affects the door connected to given square.
//
//Returns: pointer to agent if found, else NULL
//
//Params:
	const UINT wX, const UINT wY, //(in) Coord to start searching from
	COrbData* pOrb)               //(in) Orb to search for
{
	ASSERT(this->pRoom->IsValidColRow(wX, wY));

	//Gather set of all tiles activated by this orb (etc).
	CCoordSet coords;
	if (bIsLight(this->pRoom->GetTSquare(wX,wY))) {
		coords.insert(wX,wY);
	} else {
		const UINT oTile = this->pRoom->GetOSquare(wX,wY);
		if (bIsYellowDoor(oTile))
			this->pRoom->GetAllYellowDoorSquares(wX, wY, coords);
		else if (bIsFiretrap(oTile) || bIsAnyArrow(this->pRoom->GetFSquare(wX,wY)))
			coords.insert(wX,wY);
	}

	return FindOrbAgentFor(pOrb, coords);
}

//*****************************************************************************
COrbAgentData* CEditRoomScreen::FindOrbAgentFor(
//Finds which agent of orb affects the door connected to given square.
//
//Returns: pointer to agent if found, else NULL
//
//Params:
	COrbData* pOrb,               //(in)
	CCoordSet &doorCoords)      //(in/out) contents destroyed on exit
{
	ASSERT(this->eState != ES_LONGMONSTER); //calling ResetPlot() invalidates serpent placement
	this->pRoomWidget->ResetPlot();

	//Each iteration pops one pair of coordinates for plotting.
	UINT wDoorX, wDoorY, wIndex;
	while (doorCoords.pop_first(wDoorX, wDoorY))
	{
		this->pRoomWidget->SetPlot(wDoorX, wDoorY);

		//Check whether there's an agent designated here.
		for (wIndex=pOrb->agents.size(); wIndex--; )
		{
			if (pOrb->agents[wIndex]->wX == wDoorX &&
					pOrb->agents[wIndex]->wY == wDoorY)
				return pOrb->agents[wIndex];  //Found it
		}
	}

	return NULL;   //didn't find one
}

//*****************************************************************************
void CEditRoomScreen::SetDestinationEntrance(
//Show dialog to set destination level entrance for stairs at (wX,wY).
//
//Params:
	UINT wX1, UINT wY1,  //(in) rectangle stairs are on
	UINT wX2, UINT wY2)
{
	g_pTheSound->PlaySoundEffect(SEID_WALK);
	UINT dwEntranceID;
	const bool bFound = this->pRoom->GetExitEntranceIDAt(wX1,wY1,dwEntranceID);
	const UINT wOSquare = this->pRoom->GetOSquare(wX1,wY1);
	if (!bFound)
	{
		//This can happen using older (pre-release) room data where the staircase
		//didn't have the exit region explicitly defined -- Find the stair region.
		do {
			if (wY1 == 0) break;
			if (this->pRoom->GetOSquare(wX1,wY1-1) != wOSquare) break;
			--wY1;
		} while (!this->pRoom->GetExitEntranceIDAt(wX1,wY1,dwEntranceID));
		do {
			if (wY2 == this->pRoom->wRoomRows - 1) break;
			if (this->pRoom->GetOSquare(wX1,wY2+1) != wOSquare) break;
			//Stop if a defined stairway is found directly below this one.
			UINT dwTemp;
			if (this->pRoom->GetExitEntranceIDAt(wX1,wY2+1,dwTemp)) break;
			++wY2;
		} while (true);
		while (wX1 > 0 && this->pRoom->GetOSquare(wX1-1,wY1) == wOSquare)
			--wX1;
		while (wX2 < this->pRoom->wRoomCols - 1 &&
				this->pRoom->GetOSquare(wX2+1,wY1) == wOSquare)
			++wX2;
	}
	bool bValueSet = false;
	do {
		if (SelectListID(this->pEntranceBox, this->pHold, dwEntranceID,
			MID_ExitLevelPrompt) == CEntranceSelectDialogWidget::OK)
		{
			Changing();
			this->pRoom->SetExit(dwEntranceID, wX1, wY1, wX2, wY2);
			bValueSet = true;
		}
	} while (!bValueSet);
}

//*****************************************************************************
void CEditRoomScreen::SetLightLevel()
//Sets the illumination level for the room being edited.
{
	this->pRoomWidget->wDark = bIsLightingTile(this->wSelectedObject) ?
			this->pRoom->weather.wLight : 0;
	g_pTheDBM->fLightLevel = fRoomLightLevel[this->pRoomWidget->wDark];
	this->pRoomWidget->bCeilingLightsRendered = false;
	this->pRoomWidget->RenderRoomLighting();
	this->pRoomWidget->ResetForPaint();
	this->pRoomWidget->RequestPaint();
}

//*****************************************************************************
void CEditRoomScreen::SetOrbAgentsForDoor(
//Finds all orbs acting on this door and
//displays visual representation in the room widget.
//OUT: this->pOrb contains list of all orbs acting on this door
//
//Params:
	const UINT wX, const UINT wY) //(in) square door is on
{
	delete this->pOrb;
	this->pOrb = new COrbData;
	this->pOrb->wX = wX;
	this->pOrb->wY = wY;

	CCoordSet doorCoords;
	this->pRoom->GetAllYellowDoorSquares(wX, wY, doorCoords);

	for (UINT orb=0; orb < this->pRoom->orbs.size(); ++orb)
	{
		COrbData* pData = this->pRoom->orbs[orb];
		for (UINT agent=0; agent < pData->agents.size(); ++agent)
		{
			COrbAgentData* pAgentData = pData->agents[agent];
			if (doorCoords.has(pAgentData->wX, pAgentData->wY))
			{
				//Show agent effect on orb itself.
				this->pOrb->AddAgent(pData->wX, pData->wY, pAgentData->action);
				this->pRoomWidget->AddOrbEffect(pAgentData);
			}
		}
	}
	this->pRoomWidget->RemoveLastLayerEffectsOfType(ETOOLTIP);
	if (this->pOrb->agents.size())
	{
		this->pRoomWidget->AddOrbAgentsEffect(this->pOrb, false);
		this->pRoomWidget->ResetPlot();
  }
}

//*****************************************************************************
void CEditRoomScreen::Paint(
//Paint the whole screen.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	PaintBackground();
	PaintSign();
	if (this->eState != ES_LONGMONSTER) //don't reset if a serpent's being placed
		this->pRoomWidget->ResetPlot();

	PaintHighlights();
	this->pRoomWidget->ResetForPaint();

	PaintChildren();

	//Draw scroll on top when visible.
	if (this->eState == ES_SCROLL)
		PaintScroll();

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
void CEditRoomScreen::PaintHighlights()
//Refresh shaded tile highlights on room.
{
	if (!(this->eState == ES_LONGMONSTER) || //don't reset error highlights if a serpent's being placed
		  (this->eState == ES_GETSQUARE) ||
		  (this->eState == ES_GETRECT))
		this->pRoomWidget->RemoveLastLayerEffectsOfType(ESHADE);

	//Hightlight any area marked for copying in current room.
	if (this->eState == ES_PASTING)
		HighlightPendingPaste();

	if (this->bShowErrors && this->eState != ES_LONGMONSTER)
		ShowErrors();
	MarkOverheadLayerTiles();
	DrawHalphSlayerEntrances();

	ASSERT(this->pRoom);
	this->pRoom->ResetPressurePlatesState();
	this->pRoom->SetPressurePlatesState();
}

//*****************************************************************************
void CEditRoomScreen::MarkOverheadLayerTiles()
{
	const CCoordIndex& tiles = this->pRoom->overheadTiles;
	if (tiles.empty())
		return;

	//Highlight tiles set with an overhead layer.
	static const SURFACECOLOR OverheadColor = {40, 90, 220};
	for (UINT wY=0; wY < this->pRoom->wRoomRows; ++wY) {
		for (UINT wX=0; wX < this->pRoom->wRoomCols; ++wX) {
			if (tiles.Exists(wX, wY))
				this->pRoomWidget->AddShadeEffect(wX, wY, OverheadColor);
		}
	}
}

//*****************************************************************************
void CEditRoomScreen::ReadyToPasteRegion()
//Set state to pasting a room region.  Highlight the area about to be copied.
{
	SetState(ES_PASTING);
	PaintHighlights();
}

//*****************************************************************************
void CEditRoomScreen::EraseRegion()
//Erase everything in the marked room region.
{
	if (this->wCopyX1 == (UINT)-1) return; //no region marked

	Changing(RoomAndHold);

	const UINT wStartX = this->wCopyX1;
	const UINT wEndX = this->wCopyX2;
	const UINT wStartY = this->wCopyY1;
	const UINT wEndY = this->wCopyY2;

	CCoordStack removedStairs;
	bool bTarModified = false;
	this->ignoreCoords.clear();
	CDbRoom& room = *(this->pRoom);
	UINT xDest, yDest;

	for (yDest = wStartY; yDest <= wEndY; ++yDest)
	{
		for (xDest = wStartX; xDest <= wEndX; ++xDest)
		{
			//Erase o-layer.
			switch (room.GetOSquare(xDest, yDest))
			{
				case T_STAIRS:	case T_STAIRS_UP:
					removedStairs.Push(xDest, yDest);
				break;
				default: break;
			}
			EraseAndPlot(xDest, yDest, this->wLastFloorSelected, false);

			//Erase checkpoints.
			if (room.checkpoints.has(xDest, yDest))
				room.checkpoints.erase(xDest, yDest);
		
			//Erase light layer.
			if (room.tileLights.GetAt(xDest, yDest))
			{
				room.tileLights.Remove(xDest, yDest);
				this->pRoomWidget->bCeilingLightsRendered = false;
			}

			//Erase overhead layer.
			room.overheadTiles.Set(xDest, yDest, 0);

			//Erase f-layer.
			EraseAndPlot(xDest, yDest, T_EMPTY_F, false);

			//Erase t-layer.
			UINT wDestTile = room.GetTSquare(xDest, yDest);

			if (bIsTarOrFluff(wDestTile))
				bTarModified = true;

			switch (wDestTile)
			{
				case T_OBSTACLE:
				{
					//Obstacles -- Only erase them if they are entirely within the region.

					//Only erase obstacle when the top-left (origin) piece is processed.
					const UINT wOldTParam = room.GetTParam(xDest, yDest);
					if (!bObstacleTop(wOldTParam) || !bObstacleLeft(wOldTParam))
						break;

					//Get obstacle dimensions.
					UINT wObSizeIndex, wXPos, wYPos, wX, wY;
					GetObstacleStats(&room, xDest, yDest, wObSizeIndex, wXPos, wYPos);
					ASSERT(wXPos == 0);
					ASSERT(wYPos == 0);
					const BYTE obType = calcObstacleType(wOldTParam);
					const UINT wObIndex = obstacleIndices[obType][wObSizeIndex];
					ASSERT(wObIndex);
					const UINT wXDim = obstacleDimensions[wObIndex][0];
					const UINT wYDim = obstacleDimensions[wObIndex][1];
					ASSERT(wXDim);
					ASSERT(wYDim);

					//Obstacle is entirely within region?
					if (xDest + wXDim-1 > this->wCopyX2 || yDest + wYDim-1 > this->wCopyY2)
						break;

					//Erase obstacle.
					for (wY = yDest; wY < yDest + wYDim; ++wY)
						for (wX = xDest; wX < xDest + wXDim; ++wX)
							EraseAndPlot(wX, wY, T_EMPTY, false);
		
					break;
				}

				case T_MIRROR: case T_POWDER_KEG:
				{
					const UINT coveredTLayerObject = room.GetCoveredTSquare(xDest, yDest);
					if (coveredTLayerObject == T_FUSE)
						room.SetCoveredTLayerObject(xDest, yDest, RoomObject::emptyTile());
				}
				//no break
				default:
					EraseAndPlot(xDest, yDest, T_EMPTY, false);
				break;
			}

			//Erase m-layer.
			CMonster *pMonster = room.GetMonsterAtSquare(xDest, yDest);
			if (pMonster && !pMonster->IsPiece())
			{
				switch (pMonster->wType)
				{
					case M_ROCKGIANT:
					case M_SERPENT: case M_SERPENTG: case M_SERPENTB:
					case M_GENTRYII:
					{
						bool bInRegion = true;
						MonsterPieces::iterator piece;
						for (piece = pMonster->Pieces.begin();
								piece != pMonster->Pieces.end(); ++piece)
						{
							const UINT wX = (*piece)->wX, wY = (*piece)->wY;
							if (wX < this->wCopyX1 || wX > this->wCopyX2 ||
									wY < this->wCopyY1 || wY > this->wCopyY2)
							{
								bInRegion = false;
								break;
							}
						}

						if (bInRegion)
							RemoveMonster(pMonster);
					}
					break;
		
					default:
						RemoveMonster(pMonster);
					break;
				}
			}
		}
	}

	while (removedStairs.Pop(xDest,yDest) && !bIsStairs(room.GetOSquare(xDest,yDest)))
		FixCorruptStaircase(xDest,yDest);

	if (bTarModified)
		FixUnstableTar();

	PaintHighlights();
	this->pRoomWidget->RenderRoomLighting();
	this->pRoomWidget->ResetForPaint();
	this->pRoomWidget->RequestPaint();
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
	this->pMapWidget->RequestPaint();
}

//*****************************************************************************
void CEditRoomScreen::PasteRegion(
//Paste contents of marked room region to clicked location.
//
//NOTE: Does shallow copy -- just of non-complex room items,
//i.e., customized items are left undefined, and serpents are not copied.
//
//Params:
	const UINT wX, const UINT wY) //(in) top-left corner of destination region
{
	if (this->eState != ES_PASTING) return;
	if (this->wCopyX1 == (UINT)-1) return; //no region marked

	this->pRoomWidget->RemoveLastLayerEffectsOfType(ESHADE);
	CImportInfo info;
	if (this->wCopyX1 == wX && this->wCopyY1 == wY &&
			this->pCopyRoom && this->pCopyRoom->dwRoomID == this->pRoom->dwRoomID &&
			!this->bCutAndPaste)
	{ //same position in same room -- no copy needed
		this->pRoomWidget->ResetForPaint();
		goto DonePasting;
	}

	{
		Changing(RoomAndHold);

		//Determine order to copy tiles in so that everything gets moved
		//correctly even if some of the selected area is being overwritten.
		const int xInc = wX <= this->wCopyX1 ? 1 : -1;
		const int yInc = wY <= this->wCopyY1 ? 1 : -1;
		const UINT wStartX = xInc > 0 ? this->wCopyX1 : this->wCopyX2;
		const UINT wEndX = xInc < 0 ? this->wCopyX1 : this->wCopyX2;
		const UINT wStartY = yInc > 0 ? this->wCopyY1 : this->wCopyY2;
		const UINT wEndY = yInc < 0 ? this->wCopyY1 : this->wCopyY2;

		CDbRoom *pSrcRoom = this->pRoom;
		CDbHold *pSrcHold = NULL;
		if (this->pCopyRoom) {
			pSrcRoom = this->pCopyRoom;
			const UINT srcHoldID = g_pTheDB->Levels.GetHoldIDForLevel(pSrcRoom->dwLevelID);
			if (srcHoldID != g_pTheDB->Levels.GetHoldIDForLevel(this->pRoom->dwLevelID))
				pSrcHold = g_pTheDB->Holds.GetByID(srcHoldID);
		}

		//Copy tiles, one at a time.
		UINT xSrc, ySrc;         //square copied from
		UINT xDest, yDest; //square being copied to
		UINT wSrcTile, wDestTile;
		CMonster *pOldMonster, *pMonster, *pNewMonster;
		CCoordSet newOrbs;
		WCHAR *oldScrollText = NULL;
		CCueEvents Ignored;
		CCoordStack removedStairs;
		bool bTarModified = false;
		this->ignoreCoords.clear();
		CDbRoom& room = *(this->pRoom);
		for (ySrc=wStartY; true; ySrc += yInc)
		{
			if (ySrc >= room.wRoomRows) continue;
			yDest = wY + (ySrc - this->wCopyY1);
			if (yDest >= room.wRoomRows) continue;
			for (xSrc=wStartX; true; xSrc += xInc)
			{
				if (xSrc >= room.wRoomCols) continue;
				xDest = wX + (xSrc - this->wCopyX1);
				if (xDest >= room.wRoomCols) continue;

				//Copy o-layer.
				wSrcTile = pSrcRoom->GetOSquare(xSrc,ySrc);
				wDestTile = room.GetOSquare(xDest, yDest);
				pOldMonster = room.GetMonsterAtSquare(xDest, yDest);

				//Not allowed to paste obstacles under long monsters.
				bool bPasteAllowed=true;
				if (pOldMonster && (pOldMonster->IsLongMonster() || pOldMonster->IsPiece()))
				{
					if (bIsWall(wSrcTile) || bIsCrumblyWall(wSrcTile) ||
							bIsStairs(wSrcTile) ||
							bIsPit(wSrcTile) || bIsWater(wSrcTile))
						bPasteAllowed=false;
				}
				if (bPasteAllowed)
				{
					EraseAndPlot(xDest, yDest, wSrcTile, false);
					switch (wDestTile) //clean up destination tile
					{
						case T_STAIRS:	case T_STAIRS_UP:
							removedStairs.Push(xDest, yDest);
							break;
						default: break;
					}
					switch (wSrcTile) //cut/copy source tile
					{
						case T_DOOR_Y: case T_DOOR_YO:
							RepairYellowDoors(wSrcTile);
							this->ignoreCoords.clear();
						break;
						case T_PRESSPLATE:
						{
							//Copy orblike behavior.
							if (MergePressurePlate(xDest, yDest))
								break; //pressure plate already defined at destination -- don't copy this one

							COrbData *pOldOrb = pSrcRoom->GetPressurePlateAtCoords(xSrc,ySrc);
							if (!pOldOrb)
								break;

							COrbData *pNewOrb = room.AddOrbToSquare(xDest, yDest);
							pNewOrb->eType = pOldOrb->eType;
							for (UINT wAgentI=pOldOrb->agents.size(); wAgentI--; )
							{
								COrbAgentData *pAgent = pOldOrb->agents[wAgentI];
								pNewOrb->AddAgent(pAgent->wX, pAgent->wY, pAgent->action);
							}
							newOrbs.insert(pNewOrb->wX,pNewOrb->wY);
						}
						break;
						default: break;
					}
				}

				//Copy checkpoints.
				room.checkpoints.erase(xDest, yDest);
				if (pSrcRoom->checkpoints.has(xSrc, ySrc))
					room.checkpoints.insert(xDest, yDest);

				//Copy light layer.
				const UINT ceilingLight = pSrcRoom->tileLights.GetAt(xSrc, ySrc);
				if (room.tileLights.GetAt(xDest, yDest))
				{
					room.tileLights.Remove(xDest, yDest);
					this->pRoomWidget->bCeilingLightsRendered = false;
				}
				if (ceilingLight)
				{
					room.tileLights.Add(xDest, yDest, ceilingLight);
					this->pRoomWidget->bCeilingLightsRendered = false;
				}

				//Copy overhead layer.
				room.overheadTiles.Set(xDest, yDest,
						pSrcRoom->overheadTiles.GetAt(xSrc, ySrc));

				//Copy f-layer.
				wSrcTile = pSrcRoom->GetFSquare(xSrc, ySrc);
				if (wSrcTile == T_EMPTY)
					wSrcTile = T_EMPTY_F;
				EraseAndPlot(xDest, yDest, wSrcTile, false);

				//Copy t-layer.
				wSrcTile = pSrcRoom->GetTSquare(xSrc, ySrc);
				wDestTile = room.GetTSquare(xDest, yDest);

				//Not allowed to paste obstacles under long monsters.
				bPasteAllowed = true;
				if (pOldMonster && (pOldMonster->IsLongMonster() || pOldMonster->IsPiece()))
				{
					if (wSrcTile == T_ORB || bIsTarOrFluff(wSrcTile) || bIsExplodingItem(wSrcTile) ||
							bIsBriar(wSrcTile) || bIsTLayerCoveringItem(wSrcTile) ||
							wSrcTile == T_LIGHT || wSrcTile == T_STATION ||
							bIsBeacon(wSrcTile))
						bPasteAllowed = false;
				}
				if (bPasteAllowed)
				{
					if (bIsTarOrFluff(wDestTile))
						bTarModified = true;
					//Obstacles are handled specially.
					if (wSrcTile != T_OBSTACLE)
					{
						bPasteAllowed = (wDestTile != T_OBSTACLE)
							? EraseAndPlot(xDest, yDest, wSrcTile, false)
							: false;
					}
					if (bPasteAllowed) switch (wSrcTile)
					{
						case T_ORB:
						{
							//Copy orb behavior.
							COrbData *pNewOrb = room.AddOrbToSquare(xDest, yDest);
							COrbData *pOldOrb = pSrcRoom->GetOrbAtCoords(xSrc, ySrc);  //do after the above line
							if (!pOldOrb) break;

							pNewOrb->eType = pOldOrb->eType;
							for (UINT wAgentI=pOldOrb->agents.size(); wAgentI--; )
							{
								COrbAgentData *pAgent = pOldOrb->agents[wAgentI];
								pNewOrb->AddAgent(pAgent->wX, pAgent->wY, pAgent->action);
							}
							newOrbs.insert(pNewOrb->wX,pNewOrb->wY);
						}
						break;
						case T_SCROLL:
						{
							//Copy scroll behavior.
							oldScrollText = (WCHAR*)pSrcRoom->GetScrollTextAtSquare(xSrc, ySrc);
							if (oldScrollText)
								room.SetScrollTextAtSquare(xDest, yDest, oldScrollText);
						}
						break;
						case T_TAR:	case T_MUD:	case T_GEL: case T_FLUFF:
							bTarModified = true;
							break;

						case T_LIGHT: case T_TOKEN:
						case T_STATION:
						{
							const UINT wOldParam = pSrcRoom->GetTParam(xSrc, ySrc);
							room.SetTParam(xDest, yDest, static_cast<const BYTE>(wOldParam));
						}
						break;

						case T_OBSTACLE:
						{
							//Obstacles -- Only copy them if:
							//(1) they are entirely within the copy region,
							//(2) their destination is entirely within the room,
							//(3) no obstacle occupies the destination tiles,
							//    except the same obstacle if it's being moved.

							//Only copy obstacle when the top-left (origin) piece is processed.
							const UINT wOldTParam = pSrcRoom->GetTParam(xSrc, ySrc);
							if (!bObstacleTop(wOldTParam) || !bObstacleLeft(wOldTParam))
								break;

							//Get obstacle dimensions.
							UINT wObSizeIndex, wXPos, wYPos, wX, wY;
							GetObstacleStats(pSrcRoom, xSrc, ySrc, wObSizeIndex, wXPos, wYPos);
							ASSERT(wXPos == 0);
							ASSERT(wYPos == 0);
							const BYTE obType = calcObstacleType(wOldTParam);
							const UINT wObIndex = obstacleIndices[obType][wObSizeIndex];
							ASSERT(wObIndex);
							const UINT wXDim = obstacleDimensions[wObIndex][0];
							const UINT wYDim = obstacleDimensions[wObIndex][1];
							ASSERT(wXDim);
							ASSERT(wYDim);

							//1. Obstacle is entirely within copy region?
							//2. Obstacle destination is within room bounds?
							if (xSrc + wXDim-1 > this->wCopyX2 ||
									ySrc + wYDim-1 > this->wCopyY2 ||
									!room.IsValidColRow(xDest + wXDim-1, yDest + wYDim-1))
								break;

							//3. No other obstacle is at destination tiles?
							bool bCopyable = true;
							for (wY = yDest; bCopyable && wY < yDest + wYDim; ++wY)
								for (wX = xDest; wX < xDest + wXDim; ++wX)
								{
									if (room.GetTSquare(wX, wY) == T_OBSTACLE)
									{
										//An obstacle is located where the source obstacle
										//would be copied to.  Forbid copying.
										bCopyable = false;
										break;
									}
								}
							if (!bCopyable)
								break;

							//Copy obstacle: Plot obstacle at destination.
							for (wY = yDest; wY < yDest + wYDim; ++wY)
								for (wX = xDest; wX < xDest + wXDim; ++wX)
								{
									EraseAndPlot(wX, wY, wSrcTile, false);
									BYTE param = obType;
									if (wY == yDest)
										param += OBSTACLE_TOP;
									if (wX == xDest)
										param += OBSTACLE_LEFT;
									room.SetTParam(wX, wY, param);
								}
						}
						break;

						case T_MIRROR: case T_POWDER_KEG:
						{
							const UINT coveredTLayerObject = pSrcRoom->GetCoveredTSquare(xSrc, ySrc);
							if (coveredTLayerObject == T_FUSE)
								room.SetCoveredTLayerObject(xDest, yDest, coveredTLayerObject);
						}
						break;

						default: break;
					}
				}

				//Copy m-layer.  Long monsters and pieces cannot be overwritten.
				if (pOldMonster && !pOldMonster->IsLongMonster() && !pOldMonster->IsPiece())
				{
					RemoveMonster(pOldMonster);
					pOldMonster = NULL;
				}

				pMonster = pSrcRoom->GetMonsterAtSquare(xSrc, ySrc);
				if (pMonster && !pMonster->IsPiece())
				{
					switch (pMonster->wType)
					{
						case M_ROCKGIANT:
						case M_SERPENT: case M_SERPENTG: case M_SERPENTB:
						case M_GENTRYII:
						{
							//Large monsters -- Only copy them if:
							//(1) they are entirely within the copy region,
							//(2) their destination is entirely within the room,
							//(3) no long monster occupies the destination tiles,
							//    except the same monster if it's being moved.

							//Only allow overwriting a long monster at the destination if it's
							//actually part of the source monster, and will be moved accordingly.
							if (pOldMonster)
							{
								if (pOldMonster->IsPiece())
								{
									//Get parent monster.
									CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*,
											CMonster*, pOldMonster);
									pOldMonster = pPiece->pMonster;
								}

								if (pOldMonster->IsLongMonster())
									break; //not the same long monster -- don't overwrite
							}

							bool bInRegion = true;
							const UINT wDeltaX = xDest - xSrc;
							const UINT wDeltaY = yDest - ySrc;
							MonsterPieces::iterator piece;
							for (piece = pMonster->Pieces.begin();
									piece != pMonster->Pieces.end(); ++piece)
							{
								const UINT wX = (*piece)->wX, wY = (*piece)->wY;
								if (wX < this->wCopyX1 || wX > this->wCopyX2 ||
										wY < this->wCopyY1 || wY > this->wCopyY2 ||
										!room.IsValidColRow(wX+wDeltaX, wY+wDeltaY))
								{
									bInRegion = false;
									break;
								}
								CMonster *pMonsterAtDest = room.GetMonsterAtSquare(
										wX+wDeltaX, wY+wDeltaY);
								if (pMonsterAtDest)
								{
									if (pMonsterAtDest->IsPiece())
									{
										//Get parent monster.
										CMonsterPiece *pPiece = DYN_CAST(CMonsterPiece*,
												CMonster*, pMonsterAtDest);
										pMonsterAtDest = pPiece->pMonster;
									}
									if (pMonsterAtDest->IsLongMonster())
									{
										bInRegion = false;
										break;
									}
								}
							}
							if (bInRegion)
							{
								//Copy large monster.
								pNewMonster = room.AddNewMonster(
										pMonster->wType, xDest, yDest);
								pNewMonster->wO = pMonster->wO;

								//Remove any latent monsters at piece destination squares.
								for (piece = pMonster->Pieces.begin();
										piece != pMonster->Pieces.end(); ++piece)
								{
									CMonsterPiece& mPiece = *(*piece);
									const UINT wPDestX = mPiece.wX+wDeltaX; //piece destination
									const UINT wPDestY = mPiece.wY+wDeltaY;
									pOldMonster = room.GetMonsterAtSquare(wPDestX, wPDestY);
									if (pOldMonster)
									{
										ASSERT(!pOldMonster->IsLongMonster());
										ASSERT(!pOldMonster->IsPiece());
										RemoveMonster(pOldMonster);
									}
								}

								//Copy monster pieces.
								for (piece = pMonster->Pieces.begin();
										piece != pMonster->Pieces.end(); ++piece)
								{
									CMonsterPiece& oldPiece = *(*piece);
									CMonsterPiece *pPiece = new CMonsterPiece(
											pNewMonster, oldPiece.wTileNo,
											oldPiece.wX+wDeltaX, oldPiece.wY+wDeltaY);
									pNewMonster->Pieces.push_back(pPiece);
									room.SetMonsterSquare(pPiece);
								}
							}
						}
						break;
						case M_CHARACTER:
						{
							if (pOldMonster)
								break; //tile not vacant

							//Duplicate all character data.
							pNewMonster = pMonster->Replicate();
							pNewMonster->wX = xDest;
							pNewMonster->wY = yDest;
							CCharacter *pNewCharacter = DYN_CAST(CCharacter*, CMonster*, pNewMonster);
							pNewCharacter->ChangeHold(pSrcHold, this->pHold, info);
							room.LinkMonster(pNewCharacter);
						}
						break;
						default:
						{
							ASSERT(!pMonster->IsLongMonster());
							if (pOldMonster)
								break; //tile not vacant

							//Mothers are not allowed to break t-layer items when they spawn tarstuff.
							if (bIsMother(pMonster->wType))
							{
								const UINT tTile = room.GetTSquare(xDest, yDest);
								if (!bIsTarOrFluff(tTile) && tTile != T_EMPTY)
									break;
							}

							//Pasting over previous room edge entry monster types.
							if (pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2 ||
									pMonster->wType == M_HALPH || pMonster->wType == M_HALPH2)
							{
								this->pRoom->halphEnters.erase(xDest, yDest);
								this->pRoom->halph2Enters.erase(xDest, yDest);
								this->pRoom->slayerEnters.erase(xDest, yDest);
								this->pRoom->slayer2Enters.erase(xDest, yDest);

								//Enforce only one Halph allowed in room.
								if (pMonster->wType == M_HALPH || pMonster->wType == M_HALPH2)
									UniquePlacement(xDest, yDest, MONSTERTYPE(M_HALPH));
							}

							//Place room edge entry monster record if applicable.
							bool bOpenTileIgnored;
							if (PlaceEdgeMonster(pMonster->wType + M_OFFSET, xDest, yDest, bOpenTileIgnored))
								break;

							pNewMonster = room.AddNewMonster(
									pMonster->wType, xDest, yDest);
							pNewMonster->wO = pMonster->wO;
							if (bEntityHasSword(pNewMonster->wType))
							{
								CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pNewMonster);
								if (this->pRoom->IsValidColRow(pArmedMonster->GetWeaponX(),pArmedMonster->GetWeaponY()))
									this->pRoomWidget->swords.Add(pArmedMonster->GetWeaponX(),pArmedMonster->GetWeaponY());
							}
						}
						break;
					}
				}

				if (xSrc == wEndX) break;
			}
			if (ySrc == wEndY) break;
		}

		//Fix up messed-up things around the destination area.
		ObstacleFill();
		while (removedStairs.Pop(xSrc,ySrc) && !bIsStairs(room.GetOSquare(xSrc,ySrc)))
			FixCorruptStaircase(xSrc,ySrc);
		if (bTarModified)
			FixUnstableTar();
		//Remove improper agents.
		for (CCoordSet::const_iterator orb = newOrbs.begin();
				orb != newOrbs.end(); ++orb)
		{
			COrbData *pOrb = room.GetOrbAtCoords(orb->wX, orb->wY);
			if (!pOrb) continue;
			for (UINT wAgentI=pOrb->agents.size(); wAgentI--; )
			{
				COrbAgentData *pAgent = pOrb->agents[wAgentI];
				const UINT oTile = room.GetOSquare(pAgent->wX, pAgent->wY);
				if (!(bIsYellowDoor(oTile) || bIsFiretrap(oTile) ||
						 bIsLight(room.GetTSquare(pAgent->wX, pAgent->wY)) || 
						 bIsAnyArrow(room.GetFSquare(pAgent->wX, pAgent->wY))))
					VERIFY(pOrb->DeleteAgent(pAgent));
			}
		}

		delete pSrcHold;
	}

DonePasting:
	//Must hit Ctrl-V again to re-paste.
	SetState(ES_PLACING);

	this->pRoom->InitRoomStats();
	PaintHighlights();
	this->pRoomWidget->RenderRoomLighting();
	this->pRoomWidget->ResetForPaint();
	this->pRoomWidget->RequestPaint();
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
	this->pMapWidget->RequestPaint();
}

//*****************************************************************************
bool CEditRoomScreen::PlaceEdgeMonster(
//Handle placing a monster of the indicated menu type at (x,y)
//
//Returns: true if a room edge monster was (re)placed, else false
//
//Params:
	const UINT wObject, const UINT wX, const UINT wY,
	bool& bOpenTile) //(out) true if no edge record for this monster already exists at (x,y)
{
	if ((wObject == T_HALPH || wObject == T_HALPH2 || wObject == T_SLAYER || wObject == T_SLAYER2) &&
			(wX==0 || wX == this->pRoom->wRoomCols-1 ||
				wY==0 || wY == this->pRoom->wRoomRows-1))
	{
		//Halph/Slayer edge placement is handled specially.
		CCoordSet& entrances = wObject == T_HALPH ? this->pRoom->halphEnters :
			wObject == T_HALPH2 ? this->pRoom->halph2Enters :
			wObject == T_SLAYER ? this->pRoom->slayerEnters :
			this->pRoom->slayer2Enters;
		CCoordSet& entrancesForAlternateType = wObject == T_HALPH ? this->pRoom->halph2Enters :
			wObject == T_HALPH2 ? this->pRoom->halphEnters :
			wObject == T_SLAYER ? this->pRoom->slayer2Enters :
			this->pRoom->slayerEnters;

		bOpenTile = !entrances.has(wX, wY);
		if (bOpenTile)
		{
			CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
			if (pMonster && (pMonster->wType == M_HALPH || pMonster->wType == M_HALPH2 ||
						pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2))
				VERIFY(RemoveMonster(pMonster));
			entrances.insert(wX, wY);
			entrancesForAlternateType.erase(wX, wY); //ensure only one of this monster type is here
		}

		return true;
	}

	return false; //not applicable
}

//*****************************************************************************
void CEditRoomScreen::PlotLastSerpentSegment(
//Follow tail until serpent turns.
//
//Params:
	const UINT wTailX, const UINT wTailY, const UINT wDirection)   //(in)
{
	UINT wX = wTailX, wY = wTailY, wTileNo;
	int wOX, wOY;
	switch (this->pRoomWidget->GetSerpentTailTile(wX,wY,wDirection,false))
	{
		case T_SNKT_N:
			wOX = 0; wOY = 1;
			break;
		case T_SNKT_S:
			wOX = 0; wOY = -1;
			break;
		case T_SNKT_E:
			wOX = -1;   wOY = 0;
			break;
		case T_SNKT_W:
			wOX = 1; wOY = 0;
			break;
		default: ASSERT(!"Bad serpent tile.");  return;
	}
	this->pRoomWidget->SetPlot(wX,wY);  //tail
	do
	{
		wX += wOX;
		wY += wOY;
		this->pRoomWidget->SetPlot(wX,wY);  //possibly a twist
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
		wTileNo = T_NOMONSTER;
		if (pMonster && pMonster->IsPiece())
		{
			CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
			if (pMPiece)
				wTileNo = pMPiece->wTileNo;
		}
	} while (wTileNo == T_SNK_EW || wTileNo == T_SNK_NS);

	this->pRoomWidget->monsterSegment.wSX = this->pRoomWidget->monsterSegment.wEX = wTailX;
	this->pRoomWidget->monsterSegment.wSY = this->pRoomWidget->monsterSegment.wEY = wTailY;
}

//*****************************************************************************
PlotType CEditRoomScreen::PlotGentryiiSegment()
{
	CEditRoomWidget *pRW = this->pRoomWidget;

	const vector<CCoord> coords = CCoord::GetOrderedLineBetween(
		pRW->monsterSegment.wSX, pRW->monsterSegment.wSY,
		pRW->monsterSegment.wEX, pRW->monsterSegment.wEY);

	//For backtracking, can erase final tiles of existing monster
	CCoordSet overwritable_coords;
	if (this->pLongMonster)
		overwritable_coords = this->pLongMonster->GetMatchingEndTiles(coords);

	UINT wX, wY, wPrevX = coords.begin()->wX, wPrevY = coords.begin()->wY;
	for (vector<CCoord>::const_iterator it=coords.begin();
			it!=coords.end(); ++it)
	{
		wX = it->wX;
		wY = it->wY;
		const bool bPlotOverMonsterLayer = overwritable_coords.has(wX, wY) ||
					!this->pRoom->GetMonsterAtSquare(wX,wY);
		if (!bPlotOverMonsterLayer || !pRW->IsSafePlacement(T_GENTRYII,wX,wY))
			return PLOT_NOTHING;

		if (this->pRoom->DoesGentryiiPreventDiagonal(wPrevX, wPrevY, wX, wY))
			return PLOT_NOTHING;
		wPrevX = wX;
		wPrevY = wY;
	}

	bool bHeadPlotted = false;
	for (vector<CCoord>::const_iterator it=coords.begin();
			it!=coords.end(); ++it)
	{
		wX = it->wX;
		wY = it->wY;
		if (wX == pRW->monsterSegment.wHeadX &&
				wY == pRW->monsterSegment.wHeadY)
		{
			ASSERT(!bHeadPlotted);
			bHeadPlotted = true;

			const UINT wDirection = pRW->monsterSegment.wDirection;
			CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
			if (pMonster)
			{
				//Head already here.  Update head's direction.
				ASSERT(pMonster->IsLongMonster());
				pMonster->wO = nGetReverseO(wDirection);
			} else {
				PlotObjectAt(wX, wY, T_GENTRYII, wDirection);
			}
		}
	}
	//now have a monster object to assign additional pieces
	ASSERT(this->pLongMonster);

	UINT wNextStartX = 0, wNextStartY = 0;
	bool bSegmentPlotted = false, bTailPlotted = false;

	for (vector<CCoord>::const_iterator it=coords.begin();
			it!=coords.end(); ++it)
	{
		wX = it->wX;
		wY = it->wY;
		//Calculate tile to plot.
		if (wX == pRW->monsterSegment.wHeadX &&
				wY == pRW->monsterSegment.wHeadY)
		{
			//Plot head already handled above.
			continue;
		}
		if (wX == pRW->monsterSegment.wSX && wY == pRW->monsterSegment.wSY)
		{
			if ((wX == pRW->monsterSegment.wTailX) &&
					(wY == pRW->monsterSegment.wTailY))
			{
				//Only plotting one tile, and it's the tail:
				//This indicates we're done plotting, so don't do anything.
				return PLOT_DONE;
			} else {
				//Plot a twist (change the tail into a turn).
				//If backtracking, erase the piece.
				bSegmentPlotted = true;
			}
		} else if (wX == pRW->monsterSegment.wTailX &&
				wY == pRW->monsterSegment.wTailY)
		{
			bTailPlotted = true;

			//Plot next segment from this point.
			wNextStartX = wX;
			wNextStartY = wY;
		} else {
			//Plot straight segment.  If backtracking, erase.
			bSegmentPlotted = true;

			//If backtracking over head, replace it (handled above).
			if (wX == pRW->monsterSegment.wHeadX &&
					wY == pRW->monsterSegment.wHeadY)
				continue;   //don't need to plot anything
		}

		//Don't remove the last body tile remaining to be erased --
		//need to retain it in order to maintain continuity in body segments.
		const bool erase_tile = overwritable_coords.size() > 1 && overwritable_coords.has(wX, wY);

		this->pRoom->PlotMonster(wX,wY,erase_tile ? T_NOMONSTER : T_GENTRYII,this->pLongMonster);
		overwritable_coords.erase(wX, wY);
	}

	this->pRoomWidget->RemoveLastLayerEffectsOfType(ETRANSTILE);
	this->pRoomWidget->ResetForPaint();
	if (bTailPlotted)
	{
		this->pRoomWidget->monsterSegment.wSX = this->pRoomWidget->monsterSegment.wEX = wNextStartX;
		this->pRoomWidget->monsterSegment.wSY = this->pRoomWidget->monsterSegment.wEY = wNextStartY;
	} else {
		//Only a head was placed -- stop now.
		if (bHeadPlotted) return PLOT_HEAD;
	}

	if (bHeadPlotted)
		return (bTailPlotted || bSegmentPlotted ? PLOT_SEGMENT : PLOT_HEAD);
	return (bSegmentPlotted ? PLOT_SEGMENT : PLOT_ERROR);
}

//*****************************************************************************
PlotType CEditRoomScreen::PlotLongMonsterSegment()
//Plot pieces along a monster segment (look in the room widget for location).
//
//Returns: whether plot was successful.
{
	const UINT m_type = GetSelectedObject();

	if (m_type == T_GENTRYII)
		return PlotGentryiiSegment();

	CEditRoomWidget *pRW = this->pRoomWidget;
	const UINT wDirection = pRW->monsterSegment.wDirection;

	bool bHeadPlotted = false, bTailPlotted = false, bSegmentPlotted = false;
	UINT wNextStartX = 0, wNextStartY = 0;

	//Only plot tiles in a horizontal or vertical direction.
	//Determine which way to do it.
	const bool bHorizontal = pRW->monsterSegment.bHorizontal;
	const UINT wMinX = (bHorizontal ? min(pRW->monsterSegment.wSX,
			pRW->monsterSegment.wEX) : pRW->monsterSegment.wSX);
	const UINT wMinY = (bHorizontal ? pRW->monsterSegment.wSY :
			min(pRW->monsterSegment.wSY,pRW->monsterSegment.wEY));
	const UINT wMaxX = (bHorizontal ? max(pRW->monsterSegment.wSX,
			pRW->monsterSegment.wEX) : wMinX);
	const UINT wMaxY = (bHorizontal ? wMinY : max(pRW->monsterSegment.wSY,
			pRW->monsterSegment.wEY));

	UINT wX, wY;

	//Make sure plotting is legal on all spots.
	for (wY=wMinY; wY<=wMaxY; ++wY)
		for (wX=wMinX; wX<=wMaxX; ++wX)
			if (!pRW->IsSafePlacement(m_type,wX,wY))
				return PLOT_NOTHING;

	//Plot head first.
	//Necessary in order to have a pointer to the monster
	//when placing the segments.
	for (wY=wMinY; wY<=wMaxY; ++wY)
		for (wX=wMinX; wX<=wMaxX; ++wX)
			if (wX == pRW->monsterSegment.wHeadX &&
					wY == pRW->monsterSegment.wHeadY)
			{
				ASSERT(!bHeadPlotted);
				bHeadPlotted = true;

				CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
				if (pMonster)
				{
					//Head already here.  Update head's direction.
					ASSERT(pMonster->IsLongMonster());
					pMonster->wO = wDirection;
				} else {
					PlotObjectAt(wX, wY, m_type, wDirection);
				}
			}

	UINT wTileNo;
	UINT wTailTileNo = T_EMPTY;
	for (wY=wMinY; wY<=wMaxY; ++wY)
		for (wX=wMinX; wX<=wMaxX; ++wX)
		{
			//Calculate tile to plot.
			if (wX == pRW->monsterSegment.wHeadX &&
					wY == pRW->monsterSegment.wHeadY)
			{
				//Plot head.  Handled above.
				continue;
			}
			if (wX == pRW->monsterSegment.wSX && wY == pRW->monsterSegment.wSY)
			{
				if ((wX == pRW->monsterSegment.wTailX) &&
						(wY == pRW->monsterSegment.wTailY))
				{
					//Only plotting one tile, and it's the tail:
					//This indicates we're done plotting, so don't do anything.
					return PLOT_DONE;
				} else {
					//Plot a twist (change the tail into a turn).
					//If backtracking, erase the piece.
					wTileNo = this->pRoomWidget->GetSerpentTurnTile(wX,wY,
						wDirection,false);
					bSegmentPlotted = true;
				}
			} else if (wX == pRW->monsterSegment.wTailX &&
					wY == pRW->monsterSegment.wTailY)
			{
				wTileNo = this->pRoomWidget->GetSerpentTailTile(wX,wY,wDirection,
						false);
				bTailPlotted = true;

				//Plot next segment from this point.
				wNextStartX = wX;
				wNextStartY = wY;
				wTailTileNo = wTileNo;
			} else {
				//Plot straight segment.  If backtracking, erase.
				wTileNo = this->pRoomWidget->GetSerpentStraightTile(wX,wY,
						wDirection,false);
				bSegmentPlotted = true;

				//If backtracking over head, replace it (handled above).
				if (wX == pRW->monsterSegment.wHeadX &&
						wY == pRW->monsterSegment.wHeadY)
					continue;   //don't need to plot anything
			}

			PlotObjectAt(wX, wY, wTileNo, wDirection);
		}

	this->pRoomWidget->ResetForPaint();

	//Mark every square on the last current monster segment so it can be
	//replaced next plot (and possibly be erased by backing up).
	this->pRoomWidget->ResetPlot();
	if (bTailPlotted)
	{
		UINT wLastSegmentDirection = T_EMPTY;
		switch (wTailTileNo)
		{
			case T_SNKT_N: wLastSegmentDirection = S; break;
			case T_SNKT_S: wLastSegmentDirection = N; break;
			case T_SNKT_E: wLastSegmentDirection = W; break;
			case T_SNKT_W: wLastSegmentDirection = E; break;
			default: ASSERT(!"Bad serpent tail tile."); break;
		}
		PlotLastSerpentSegment(wNextStartX,wNextStartY,wLastSegmentDirection);
	} else {
		//Only a head was placed -- stop now.
		if (bHeadPlotted) return PLOT_HEAD;
	}

	if (bHeadPlotted)
		return (bTailPlotted || bSegmentPlotted ? PLOT_SEGMENT : PLOT_HEAD);
	return (bSegmentPlotted ? PLOT_SEGMENT : PLOT_ERROR);
}

//*****************************************************************************
void CEditRoomScreen::PlotObjects()
//Plot objects in a rectangular area.
{
	UINT wX, wY;
	const UINT wPlottedObject = GetSelectedObject();

	if (!this->pRoomWidget->bMouseInBounds) return;

	//Place objects larger than one square.
	if (SinglePlacement[wPlottedObject])
	{
		switch (wPlottedObject)
		{
			//Monsters
			case T_TARMOTHER:
			case T_MUDMOTHER:
			case T_GELMOTHER:
				++this->pRoomWidget->wEndX;   //pair of eyes
			break;

			case T_GENTRYII:
				//Needed to edit a preexisting gentryii.
				if (this->pRoom->GetMonsterAtSquare(
						this->pRoomWidget->wStartX, this->pRoomWidget->wStartY) != NULL)
				{
					EditObjects();
					return;
				}
			case T_SERPENT:
			case T_SERPENTG:
			case T_SERPENTB:
			{
				//Can a serpent be placed here?
				if (!this->pRoomWidget->IsSafePlacement(wPlottedObject,
						this->pRoomWidget->wStartX, this->pRoomWidget->wStartY))
				{
					//Edit whatever is already here.
					EditObjects();
					return;
				}

				//Start placing monster segments.
				//The head (and starting position) goes here.
				this->pRoomWidget->monsterSegment.wHeadX =
						this->pRoomWidget->monsterSegment.wSX =
						this->pRoomWidget->wStartX;
				this->pRoomWidget->monsterSegment.wHeadY =
						this->pRoomWidget->monsterSegment.wSY =
						this->pRoomWidget->wStartY;

				const UINT monster_type = wPlottedObject-M_OFFSET;
				this->pRoomWidget->monsterSegment.wType = monster_type;

				this->pRoomWidget->ResetPlot();  //no monster pieces placed yet

				Changing();
				VERIFY(SetState(ES_LONGMONSTER));

				this->pRoomWidget->AddMonsterSegmentEffect(this->pLongMonster, monster_type);
			}
			return;

			case T_CHARACTER:
			{
				wX = this->pRoomWidget->wStartX;
				wY = this->pRoomWidget->wStartY;
				if (!this->pRoomWidget->IsSafePlacement(wPlottedObject, wX, wY))
				{
					//Edit whatever is already here.
					EditObjects();
					return;
				}

				g_pTheSound->PlaySoundEffect(SEID_SWING);
				ASSERT(!this->pRoom->GetMonsterAtSquare(wX,wY));

				//If a character is located where mouse was when button
				//was pressed, move it to this location.
				CMonster *pMonster = this->pRoom->GetMonsterAtSquare(
						this->pRoomWidget->wDownX, this->pRoomWidget->wDownY);
				if (pMonster && pMonster->wType == M_CHARACTER)
				{
					//Move character.
					Changing();
					this->pRoom->MoveMonster(pMonster,
							this->pRoomWidget->wEndX, this->pRoomWidget->wEndY);
					pMonster->wX = this->pRoomWidget->wEndX;
					pMonster->wY = this->pRoomWidget->wEndY;
					pMonster->wO = this->wO;
				} else {
					Changing(RoomAndHold);   //add current script# and hold variable state to undo list
					pMonster = this->pRoom->AddNewMonster(M_CHARACTER, wX, wY);
					ASSERT(pMonster);
					pMonster->wO = this->wO;
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					pCharacter->dwScriptID = this->pHold->GetNewScriptID();
					this->pCharacterDialog->EditCharacter(pCharacter);
					this->pCharacterDialog->Display();
				}
				this->bRoomDirty = true;

				//Update sword coords.
				this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);
				Paint();
			}
			return;

			//Placing level start position.
			case T_SWORDSMAN:
			{
				if (!this->pRoomWidget->IsSafePlacement(wPlottedObject,
						this->pRoomWidget->wEndX, this->pRoomWidget->wEndY, this->wO))
				{
					//Edit whatever is already here.
					EditObjects();
					return;
				}

				//Add new level entrance.
				g_pTheSound->PlaySoundEffect(SEID_CLEAR);

				//Save unchanged hold entrance info in undo list.
				Changing(Hold);

				//If a level start is located where mouse was when button
				//was pressed, move it to this location.
				CEntranceData *pEntrance;
				bool bSwordsman, bSwordAt;
				this->pRoomWidget->IsLevelStartAt(this->pRoomWidget->wDownX,
						this->pRoomWidget->wDownY, bSwordsman, bSwordAt);
				if (bSwordsman)
				{
					//Move level entrance.
					pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
							this->pRoomWidget->wDownX, this->pRoomWidget->wDownY);
					ASSERT(pEntrance);
					pEntrance->wX = this->pRoomWidget->wEndX;
					pEntrance->wY = this->pRoomWidget->wEndY;
					pEntrance->wO = this->wO;
				} else {
					//Add new level entrance.
					//Provide level entrance with default text.
					WSTRING wstrDescription = g_pTheDB->GetMessageText(MID_Entrance);
					WCHAR temp[16];
					_itoW(this->pHold->Entrances.size() + 1, temp, 10);
					wstrDescription += wszSpace;
					wstrDescription += temp;

					pEntrance = new CEntranceData(0, 0, this->pRoom->dwRoomID,
							this->pRoomWidget->wEndX, this->pRoomWidget->wEndY, this->wO,
							false, CEntranceData::DD_Always, 0);
					pEntrance->DescriptionText = wstrDescription.c_str();
					this->pHold->AddEntrance(pEntrance);
				}

				//Update.
				GetLevelEntrancesInRoom();
				PaintHighlights();
				this->pRoomWidget->ResetForPaint();
				return;
			}
		}

		//Bounds checking.
		if (!this->pRoom->IsValidColRow(this->pRoomWidget->wStartX,this->pRoomWidget->wStartY) ||
				!this->pRoom->IsValidColRow(this->pRoomWidget->wEndX,this->pRoomWidget->wEndY))
			return;

		//If something is already on these squares, edit it.
		for (wY=this->pRoomWidget->wStartY; wY<=this->pRoomWidget->wEndY; ++wY)
			for (wX=this->pRoomWidget->wStartX; wX<=this->pRoomWidget->wEndX; ++wX)
				if (!this->pRoomWidget->IsSafePlacement(wPlottedObject, wX, wY))
				{
					EditObjects();
					return;
				}
	}

	switch (wPlottedObject)
	{
	case T_STAIRS:
	case T_STAIRS_UP:
		//Staircase is plotted specially.
		PlotStaircase(wPlottedObject);
		break;

	case T_HALPH: case T_HALPH2:
	case T_SLAYER:
		//Enforce either placement along room edge, or one placement in mid-room.
		if (this->pRoomWidget->wStartY < this->pRoom->wRoomRows-1 &&
			 this->pRoomWidget->wEndY > 0 &&
			 this->pRoomWidget->wStartX < this->pRoom->wRoomCols-1 &&
			 this->pRoomWidget->wEndX > 0)
		{
			//Mid-room inclusion -- constrain to single placement.
			this->pRoomWidget->wEndY = this->pRoomWidget->wStartY;
			this->pRoomWidget->wEndX = this->pRoomWidget->wStartX;
		}
	//NO BREAK
	case T_SLAYER2: //not constrained to single placement
	default:
	{
		//When custom images are being edited, this could affect the hold as well.
		const Change changeType = bIsCustomImageTile(wPlottedObject) ? RoomAndHold : Room;
		Changing(changeType);

		bool bSomethingPlotted = false;
		this->pRoomWidget->ResetPlot();
		this->ignoreCoords.clear();
		//Place selected object at each square.
		for (wY=this->pRoomWidget->wStartY; wY<=this->pRoomWidget->wEndY; ++wY)
			for (wX=this->pRoomWidget->wStartX; wX<=this->pRoomWidget->wEndX; ++wX)
				if (PlotObjectAt(wX,wY,GetSelectedObject(),this->wO))
					bSomethingPlotted = true;

		//Play sound effect.  Customize object.
		if (!bSomethingPlotted)
		{
			RemoveChange();
			if (wPlottedObject != T_EMPTY)
				EditObjects();
		} else {
			switch (wPlottedObject)
			{
				case T_FLOOR: case T_FLOOR_M:
				case T_FLOOR_ROAD: case T_FLOOR_GRASS:
				case T_FLOOR_DIRT: case T_FLOOR_ALT:
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
					break;
				case T_HOT:
					g_pTheSound->PlaySoundEffect(SEID_SIZZLE);
					break;
				case T_STEP_STONE:
					g_pTheSound->PlaySoundEffect(SEID_SPLASH);
					break;
				case T_FLOOR_SPIKES:
					g_pTheSound->PlaySoundEffect(SEID_SPIKES_UP);
					break;
				case T_FIRETRAP:
					g_pTheSound->PlaySoundEffect(SEID_FIRETRAP);
					break;
				case T_FIRETRAP_ON:
					g_pTheSound->PlaySoundEffect(SEID_FIRETRAP_START);
					break;
				case T_FLUFFVENT:
					g_pTheSound->PlaySoundEffect(SEID_PUFF_EXPLOSION);
					break;
				case T_THINICE: case T_THINICE_SH:
					g_pTheSound->PlaySoundEffect(SEID_ICEMELT);
					break;
				case T_FLOOR_IMAGE:
				case T_PIT_IMAGE:
				case T_WALL_IMAGE:
					//Assign a user-defined image texture to this custom tile type.
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
					GetFloorImageID();
					break;
				case T_OVERHEAD_IMAGE:
					//Assign a user-defined image texture to this custom tile type.
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
					GetOverheadImageID();
					break;
				case T_EMPTY:
				case T_PIT:
					g_pTheSound->PlaySoundEffect(SEID_BREAKWALL);   break;
				case T_OBSTACLE:
					ObstacleFill();
					//FALL-THROUGH
				case T_WALL: case T_WALL2: case T_WALL_M: case T_WALL_WIN:
				case T_WALL_B: case T_WALL_H:
				case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
				case T_TRAPDOOR: case T_TRAPDOOR2:
				case T_PLATFORM_W: case T_PLATFORM_P:

				case T_ARROW_NW: case T_ARROW_N: case T_ARROW_NE: case T_ARROW_E:
				case T_ARROW_SE: case T_ARROW_S: case T_ARROW_SW: case T_ARROW_W:
				case T_ARROW_OFF_NW: case T_ARROW_OFF_N: case T_ARROW_OFF_NE: case T_ARROW_OFF_E:
				case T_ARROW_OFF_SE: case T_ARROW_OFF_S: case T_ARROW_OFF_SW: case T_ARROW_OFF_W:
				case T_EMPTY_F:
				case T_LIGHT_CEILING: case T_DARK_CEILING: case T_WALLLIGHT:
				case T_NODIAGONAL:
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR); break;

				case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
				case T_TOKEN:
				case T_STATION:
				case T_LIGHT:
				case T_MIRROR:
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR); break;
					break;
				case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
					g_pTheSound->PlaySoundEffect(SEID_BRIAR_BREAK); break;
				case T_DOOR_Y: case T_DOOR_YO:
					RepairYellowDoors(wPlottedObject);
					//no break
				case T_DOOR_M: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B:
				case T_DOOR_GO: case T_DOOR_CO: case T_DOOR_RO: case T_DOOR_BO:
					g_pTheSound->PlaySoundEffect(SEID_DOOROPEN); break;
				case T_CHECKPOINT:
					g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);  break;
				case T_WATER:
					g_pTheSound->PlaySoundEffect(SEID_SPLASH); break;
				case T_SHALLOW_WATER:
					g_pTheSound->PlaySoundEffect(SEID_WADE); break;

				case T_POTION_I: case T_POTION_K: case T_POTION_D:
				case T_POTION_SP:	case T_POTION_C:
					g_pTheSound->PlaySoundEffect(SEID_POTION);
					break;
				case T_HORN_SQUAD:
					g_pTheSound->PlaySoundEffect(SEID_HORN_SQUAD);
					break;
				case T_HORN_SOLDIER:
					g_pTheSound->PlaySoundEffect(SEID_HORN_SOLDIER);
					break;
				case T_SCROLL:
				{
					g_pTheSound->PlaySoundEffect(SEID_READ);

					//If a scroll is located where mouse was when button
					//was pressed, move it to this location.
					const WCHAR *pScrollText = this->pRoom->GetScrollTextAtSquare(
							this->pRoomWidget->wDownX, this->pRoomWidget->wDownY);
					if (pScrollText)
					{
						//Move scroll.
						this->pRoom->MoveScroll(
								this->pRoomWidget->wDownX, this->pRoomWidget->wDownY,
								this->pRoomWidget->wEndX, this->pRoomWidget->wEndY);
						this->pRoom->Plot(this->pRoomWidget->wDownX,
								this->pRoomWidget->wDownY, T_EMPTY);
					} else {
						//Add a new scroll text.
						const bool bResult = EditScrollText(this->pRoomWidget->wEndX,
								this->pRoomWidget->wEndY);
						if (!bResult)
						{
							//don't place scroll if text was cancelled
							RemoveChange();
							this->pRoom->Plot(this->pRoomWidget->wStartX,
								this->pRoomWidget->wStartY, T_EMPTY);
						}
					}
				   break;
				}
				case T_ORB:
					g_pTheSound->PlaySoundEffect(this->wSelOrbType == OT_NORMAL ?
							SEID_ORBHIT : SEID_ORBBROKE);
				break;
				case T_PRESSPLATE:
				{
					g_pTheSound->PlaySoundEffect(SEID_PRESSPLATE);
					//Pressure plate type plotted.  Define it.
					for (wY=0; wY<this->pRoom->wRoomRows; ++wY)
						for (wX=0; wX<this->pRoom->wRoomCols; ++wX)
						{
							if (this->pRoom->GetOSquare(wX,wY) == T_PRESSPLATE)
							{
								COrbData *pPlate = this->pRoom->GetPressurePlateAtCoords(wX,wY);
								if (!pPlate)
								{
									pPlate = this->pRoom->AddOrbToSquare(wX,wY);
									pPlate->eType = (OrbType)this->wSelPlateType;
								}
							}
						}
				}
				break;
				case T_FUSE:
					g_pTheSound->PlaySoundEffect(SEID_STARTFUSE);   break;
				case T_BOMB:
				case T_POWDER_KEG:
					g_pTheSound->PlaySoundEffect(SEID_BOMBEXPLODE); break;
				case T_BEACON:
					g_pTheSound->PlaySoundEffect(SEID_SEEDING_BEACON_ON);
				break;
				case T_BEACON_OFF:
					g_pTheSound->PlaySoundEffect(SEID_SEEDING_BEACON_OFF);
				break;

				case T_QROACH:
					g_pTheSound->PlaySoundEffect(SEID_ROACH_EGG_SPAWNED); break;
				case T_WATERSKIPPER:
				case T_SKIPPERNEST:
					g_pTheSound->PlaySoundEffect(SEID_WADE); break;
				case T_SEEP:
					g_pTheSound->PlaySoundEffect(SEID_SEEP_DEATH); break;
				case T_WUBBA:
					g_pTheSound->PlaySoundEffect(SEID_WUBBA); break;
				case T_ROACH:
				case T_GOBLIN:
				case T_WWING:
				case T_SPIDER:
				case T_BRAIN:
				case T_FEGUNDO:
				case T_NOMONSTER:
					g_pTheSound->PlaySoundEffect(SEID_SPLAT); break;
				case T_EYE:
					g_pTheSound->PlaySoundEffect(SEID_EVILEYEWOKE); break;
				case T_TAR:	case T_MUD: case T_GEL:
					//Remove illegal tar formations.
					FixUnstableTar();
					g_pTheSound->PlaySoundEffect(SEID_STABTAR);  break;
					break;
				case T_TARMOTHER:
				case T_MUDMOTHER:
				case T_GELMOTHER:
					g_pTheSound->PlaySoundEffect(SEID_TAR_GROWTH); break;
				case T_TARBABY:
				case T_MUDBABY:
				case T_GELBABY:
					g_pTheSound->PlaySoundEffect(SEID_TARBABY_DEATH);  break;
				case T_GOO:
					g_pTheSound->PlaySoundEffect(SEID_STABTAR);  break;
				case T_FLUFF:
					FixUnstableTar();
					g_pTheSound->PlaySoundEffect(SEID_PUFF_EXPLOSION); break;
				case T_FLUFFBABY:
					g_pTheSound->PlaySoundEffect(SEID_PUFF_EXPLOSION); break;
				case T_ROCKGOLEM:
				case T_ROCKGIANT:
					g_pTheSound->PlaySoundEffect(SEID_GOLEM_DEATH); break;
				case T_GUARD:
				case T_STALWART: case T_STALWART2:
					g_pTheSound->PlaySoundEffect(SEID_SWORDS);   break;
				case T_GENTRYII:
					g_pTheSound->PlaySoundEffect(SEID_CHAIN_PULL); break;
				case T_HALPH:
					g_pTheSound->PlaySoundEffect(SEID_HALPHENTERED);
					UniquePlacement(this->pRoomWidget->wEndX, this->pRoomWidget->wEndY, M_HALPH);
					break;
				case T_HALPH2:
					g_pTheSound->PlaySoundEffect(SEID_HALPH2ENTERED);
					UniquePlacement(this->pRoomWidget->wEndX, this->pRoomWidget->wEndY, M_HALPH2);
					break;
				case T_SLAYER:
					g_pTheSound->PlaySoundEffect(SEID_SLAYERENTERFAR);
					UniquePlacement(this->pRoomWidget->wEndX, this->pRoomWidget->wEndY, M_SLAYER);
					break;
				case T_SLAYER2:
					g_pTheSound->PlaySoundEffect(SEID_SLAYER2ENTERFAR);
					//not unique placement
					break;
				case T_CHARACTER:
				case T_CITIZEN:
				case T_ARCHITECT:
					break;
				case T_CONSTRUCT:
					g_pTheSound->PlaySoundEffect(SEID_CONSTRUCT_SMASH);
					break;
			}

			//Refresh room stats and/or covered tiles when needed.
			if (bIsBriar(wPlottedObject) || wPlottedObject == T_TOKEN) //briar and conquer token require recalcs
				this->pRoom->InitRoomStats();
			else if (TILE_LAYER[wPlottedObject] == 0)
			{
				//Redefine current connected components, like platforms,
				//and synch coveredOTiles.
				this->pRoom->InitRoomStats();
			}

			PaintHighlights();
			this->pRoomWidget->RenderRoomLighting();
			this->pRoomWidget->ResetForPaint();

			//Update minimap when needed.
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
			this->pMapWidget->RequestPaint();
		}
		break;
	}
	}  //switch
}

//*****************************************************************************
bool CEditRoomScreen::PlotObjectAt(
//Plot object at specified coord, if it's safe to do it.
//Object is either a monster-type, or a room object in the o- or t-layer.
//
//Returns: whether the object was plotted
//
//Params:
	const UINT wX, const UINT wY, UINT wObject, const UINT wO)  //(in)
{
	if (!this->pRoomWidget->IsSafePlacement(wObject, wX, wY))
		return false;

	bool bPlotted = false;
	CMonster *pMonster, *pTempMonster;
	if (!IsMonsterTileNo(wObject))
	{
		//Was an object of this type already on this square?
		const UINT tTile = this->pRoom->GetTSquare(wX,wY);
		const bool bObjectWasHere = tTile == wObject;
		COrbData *pOldOrb = NULL;
		if (wObject == T_ORB)
		{
			//preserve orb agents when changing type
			COrbData *pOrb = this->pRoom->GetOrbAtCoords(wX,wY);
			if (pOrb)
				pOldOrb = new COrbData(*pOrb);
		}
		if (EraseAndPlot(wX,wY,wObject, true))
		{
			bPlotted = true;
			switch (wObject)
			{
				case T_LIGHT:
					this->pRoom->SetTParam(wX,wY, this->wSelectedLightType);
				break;
				case T_OBSTACLE:
					this->obstacles.Add(wX,wY, this->wSelectedObType);
				break;
				case T_ORB:
					if (pOldOrb || this->wSelOrbType != OT_NORMAL)
					{
						ASSERT(this->pRoom->GetOrbAtCoords(wX,wY) == NULL);
						COrbData *pOrb = pOldOrb ? pOldOrb : this->pRoom->AddOrbToSquare(wX,wY);
						const bool bDifferentType = (UINT)pOrb->eType != this->wSelOrbType;
						pOrb->eType = (OrbType)this->wSelOrbType;
						if (pOldOrb)
						{
							this->pRoom->AddOrb(pOldOrb); //restore old orb data to room
							return bDifferentType; //if the same type, edit this orb on return
						}
					}
					if (bObjectWasHere)
						return false; //allow editing this orb now
				break;
				case T_PRESSPLATE:
					//Merge to neighboring pressure plates.  Set to selected plate type.
					MergePressurePlate(wX,wY,true);
				break;
				case T_STATION:
					//Each station belongs to a specific color set.
					this->pRoom->SetTParam(wX,wY, this->wSelectedLightType);
				break;
				case T_TOKEN:
					this->pRoom->SetTParam(wX,wY, this->wSelTokenType);
				break;
				case T_MIRROR:
				case T_POWDER_KEG:
					//Keep track of covered t-layer items by representing them as a t-param value.
					this->pRoom->SetCoveredTLayerObject(wX,wY, tTile);
				break;
				default:
					//Keep track of what kind of floor is in room.
					if (bIsPlainFloor(wObject))
						this->pRoom->coveredOSquares.Add(wX,wY,wObject);
				break;
			}
		}
		delete pOldOrb; //delete any stray copy
	} else {
		//Handle removing/replacing monsters.
		bool bOpenTile;
		if (PlaceEdgeMonster(wObject, wX, wY, bOpenTile))
			return bOpenTile;

		pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);

		//For large monsters, ensure entire footprint is clear for placement.
		if (wObject == T_ROCKGIANT)
		{
			//Allow changing the orientation of a placed rock giant.
			if (pMonster && pMonster->wType == M_ROCKGIANT && !pMonster->IsPiece())
			{
				pMonster->wO = wO;
				return true;
			}

			//Check rest of 2x2 area.
			if (this->ignoreCoords.has(wX,wY)) //a monster was just placed here during
				return false; //this plot operation -- don't replace it
			if (!((this->pRoomWidget->IsSafePlacement(wObject, wX+1, wY)) &&
				  (this->pRoomWidget->IsSafePlacement(wObject, wX, wY+1)) &&
				  (this->pRoomWidget->IsSafePlacement(wObject, wX+1, wY+1))))
				return false;
			//Check for placement on another monster.
			if (this->pRoom->GetMonsterAtSquare(wX+1, wY) != NULL ||
				 this->pRoom->GetMonsterAtSquare(wX, wY+1) != NULL ||
				 this->pRoom->GetMonsterAtSquare(wX+1, wY+1) != NULL)
				return false;
		}

		const UINT monster_type = wObject - M_OFFSET;
		if (pMonster)
		{
			if (pMonster->wType == monster_type && !pMonster->IsLongMonster() && !pMonster->IsPiece())
			{
				//When replacing a monster with one of the same type, just reorient the
				//existing one without changing its position in the movement order.
				pMonster->wO = wO;
				//There may have been another sword in the same location,
				//so refresh the sword list
				this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);
			} else {
				//When replacing a different monster, delete the old one and add a new one.
				if (!RemoveMonster(pMonster))
					return false;
				pMonster = NULL;
			}
		}
		if (!pMonster)
			pMonster = this->pRoom->AddNewMonster(monster_type, wX, wY);
		ASSERT(pMonster);
		//Set correct monster orientation, etc.
		switch (pMonster->wType)
		{
		case M_BRAIN:
		case M_SKIPPERNEST:
			pMonster->wO = NO_ORIENTATION;
			break;
		case M_TARMOTHER:
		case M_MUDMOTHER:
		case M_GELMOTHER:
			pMonster->wO = NO_ORIENTATION;
			//Change orientation to show the east eye.
			if (wX > 0)
			{
				pTempMonster = this->pRoom->GetMonsterAtSquare(wX-1,wY);
				if (pTempMonster)
				{
					if (pTempMonster->wType == pMonster->wType &&
							pTempMonster->wO != S)
						pMonster->wO = S;
				}
			}

			//Put tar under tar mother.
			this->pRoom->Plot(wX,wY,pMonster->wType == M_TARMOTHER ? T_TAR :
					pMonster->wType == M_MUDMOTHER ? T_MUD : T_GEL);
			FixUnstableTar();
			break;
		case M_GENTRYII:
			g_pTheSound->PlaySoundEffect(SEID_CHAIN_PULL);
			//no break
		case M_SERPENT:
		case M_SERPENTG:
		case M_SERPENTB:
			pMonster->wO = wO;
			this->pLongMonster = pMonster;   //placing this monster
			this->pRoomWidget->monsterSegment.wType = pMonster->wType;
			break;
		case M_ROCKGIANT:
		{
			pMonster->wO = wO;

			//Add pieces to giant monster.
			CMonsterPiece *pPiece = new CMonsterPiece(pMonster, 0, wX+1, wY);
			pMonster->Pieces.push_back(pPiece);
			this->pRoom->SetMonsterSquare(pPiece);
			pPiece = new CMonsterPiece(pMonster, 0, wX, wY+1);
			pMonster->Pieces.push_back(pPiece);
			this->pRoom->SetMonsterSquare(pPiece);
			pPiece = new CMonsterPiece(pMonster, 0, wX+1, wY+1);
			pMonster->Pieces.push_back(pPiece);
			this->pRoom->SetMonsterSquare(pPiece);
			this->ignoreCoords.insert(wX+1,wY); //don't replace monster with another
			this->ignoreCoords.insert(wX,wY+1); //one during this plot operation
			this->ignoreCoords.insert(wX+1,wY+1);
		}
		break;

		default:
			pMonster->wO = wO;
			if (bEntityHasSword(pMonster->wType))
			{
				CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
				if (this->pRoom->IsValidColRow(pArmedMonster->GetWeaponX(),pArmedMonster->GetWeaponY()))
					this->pRoomWidget->swords.Add(pArmedMonster->GetWeaponX(),pArmedMonster->GetWeaponY());
			}
			break;
		}
		bPlotted = true;
	}

	return bPlotted;
}

//*****************************************************************************
bool CEditRoomScreen::EraseAndPlot(
//Remove object being written over and place object being plotted.
//Following this, perform any side-effects of room change.
//
//Returns: whether erasure was allowed and plot was made
//
//Params:
	const UINT wX, const UINT wY, //(in)
	const UINT wObjectToPlot,     //(in)
	const bool bFixImmediately)   //(in) default = true
{
	switch (wObjectToPlot)
	{
		case T_CHECKPOINT:
			//Checkpoints are on a unique layer and won't overwrite anything else.
			this->pRoom->checkpoints.insert(wX,wY);
			return true;
		case T_WALLLIGHT:
			//Lights are on their own layer and won't overwrite anything else.
			this->pRoom->tileLights.Add(wX,wY,WALL_LIGHT + this->wSelectedLightType);
			this->pRoomWidget->bCeilingLightsRendered = false;
			return true;
		case T_LIGHT_CEILING:
			this->pRoom->tileLights.Add(wX,wY,this->wSelectedLightType+1);
			this->pRoomWidget->bCeilingLightsRendered = false;
			return true;
		case T_DARK_CEILING:
			this->pRoom->tileLights.Add(wX,wY,LIGHT_OFF + this->wSelectedDarkType+1);
			this->pRoomWidget->bCeilingLightsRendered = false;
			return true;
		case T_OVERHEAD_IMAGE:
			//Overhead image is on its own layer and won't overwrite anything else.
			this->pRoom->overheadTiles.Add(wX,wY);
			return true;
		default: break;
	}

	bool bObRemoved=false;  //obstacle tile was removed
	bool bTarRemoved=false;
	bool bStairsRemoved=false;
	if (!RemoveObjectAt(wX,wY,wObjectToPlot,bObRemoved,bTarRemoved,bStairsRemoved))
		return false;

	UINT wObIndex = 0, wXPos, wYPos;
	if (bObRemoved && bFixImmediately)
	{
		UINT wObSizeIndex;
		GetObstacleStats(this->pRoom, wX, wY, wObSizeIndex, wXPos, wYPos);
		const BYTE obType = calcObstacleType(this->pRoom->GetTParam(wX,wY));
		wObIndex = obstacleIndices[obType][wObSizeIndex];
	}

	const bool bPlotUnderObject = wObjectToPlot == T_FUSE &&
			bIsTLayerCoveringItem(this->pRoom->GetTSquare(wX,wY));

	this->pRoom->Plot(wX,wY,wObjectToPlot,this->pLongMonster, bPlotUnderObject);
	if (bFixImmediately)
	{
		if (bObRemoved)
			RecalcBrokenObstacle(wX-wXPos, wY-wYPos, wObIndex);
		if (bTarRemoved)
			FixUnstableTar();
		if (bStairsRemoved)
			FixCorruptStaircase(wX,wY);
	}

	//Tally where yellow doors are plotted to resolve conflicts when plotting is done.
	//
	//NOTE: ignoreCoords should be reset before any looping operation on this
	//method that might deal with plotting yellow doors.
	if (bIsYellowDoor(wObjectToPlot))
		this->ignoreCoords.insert(wX,wY);

	return true;
}

//*****************************************************************************
void CEditRoomScreen::PlotStaircase(const UINT wStairType)
//Plot staircase with a wall around the left, bottom and right sides.
//(And nothing on top.)
{
	ASSERT(bIsStairs(wStairType));

	const UINT wStartX = this->pRoomWidget->wStartX;
	const UINT wStartY = this->pRoomWidget->wStartY;
	UINT wEndX = this->pRoomWidget->wEndX;
	UINT wEndY = this->pRoomWidget->wEndY;
	UINT wX, wY;

	//If all spots aren't safe, don't plot anything
	this->pRoomWidget->ResetPlot();
	for (wY=wStartY; wY<=wEndY; ++wY)
		for (wX=wStartX; wX<=wEndX; ++wX)
		{
			//Determine what part of staircase is being plotted at this square.
			if (!this->pRoomWidget->IsSafePlacement(wStairType, wX, wY))
			{
				EditObjects();
				PaintHighlights();
				this->pRoomWidget->ResetForPaint();
				return;
			}
		}

	g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);

	Changing();

	//Plot staircase.
	for (wY=wStartY; wY<=wEndY; ++wY)
		for (wX=wStartX; wX<=wEndX; ++wX)
			PlotObjectAt(wX,wY,wStairType,this->wO);

	SetDestinationEntrance(wStartX, wStartY, wEndX, wEndY);

	this->pRoom->InitRoomStats();
	PaintHighlights();
	this->pRoomWidget->ResetForPaint();

	this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
	this->pMapWidget->RequestPaint();
}

//*****************************************************************************
void CEditRoomScreen::EraseObjects()
//Erase objects in the selected rectangular area.
//Only the layer of the currently selected object is affected.
{
	if (!this->pRoomWidget->bMouseInBounds)
		return;

	Changing();

	const UINT wLayer = TILE_LAYER[this->wSelectedObject];
	bool bSomethingPlotted = false, bSomethingDeleted = false;
	if ((SDL_GetModState() & KMOD_SHIFT) == 0) {
		EraseObjects(wLayer, bSomethingPlotted, bSomethingDeleted);
	} else {
		EraseObjectsOnAllLayers(bSomethingPlotted, bSomethingDeleted);
	}

	//Update room info if something changed.
	if (bSomethingPlotted || bSomethingDeleted)
	{
		this->pRoom->InitRoomStats();

		if (bSomethingPlotted)
		{
			//Play sound effect.
			switch (wLayer)
			{
				case 0:
					g_pTheSound->PlaySoundEffect(SEID_BREAKWALL);
					break;
				case 1:
					g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
					break;
				case 2:
					g_pTheSound->PlaySoundEffect(SEID_SPLAT);
					break;
				default: break;
			}

			//Update room.
			PaintHighlights();
			this->pRoomWidget->RenderRoomLighting();
			this->pRoomWidget->ResetForPaint();
			//Update minimap.
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
			this->pMapWidget->RequestPaint();
		}
	} else {
		//If nothing was actually erased or plotted, don't save a step in the undo list.
		RemoveChange();
	}
}

//*****************************************************************************
void CEditRoomScreen::EraseObjects(
//Erase objects from the specified room layer.
//
//Params:
	const UINT wLayer,
	bool& bSomethingPlotted, bool& bSomethingDeleted) //(in/out) result of erasure (vars might already be set from a previous call)
{
	//Save selected object while erasing.
	const UINT originalSelectedObject = this->wSelectedObject;

	switch (wLayer)
	{
		default:
		case 0:
			//In replacing o-layer with floor, intelligently choose which kind of
			//floor to plot.
			this->wSelectedObject = this->wLastFloorSelected;
			break;
		case 1:
			this->wSelectedObject = T_EMPTY;
			break;
		case 2:
			this->wSelectedObject = T_NOMONSTER;
			break;
		case 3:
			this->wSelectedObject = T_EMPTY_F;
			break;
	}

	this->pRoomWidget->ResetPlot();
	this->ignoreCoords.clear();
	//Place selected object at each square.
	UINT wX, wY;
	for (wY=this->pRoomWidget->wStartY; wY<=this->pRoomWidget->wEndY; ++wY)
		for (wX=this->pRoomWidget->wStartX; wX<=this->pRoomWidget->wEndX; ++wX)
		{
			//If erasing items on their own layer, remove them only.
			switch (originalSelectedObject)
			{
				case T_CHECKPOINT:
					this->pRoom->checkpoints.erase(wX,wY);
					bSomethingPlotted = true;
				break;
				case T_WALLLIGHT:
				case T_LIGHT_CEILING:
				case T_DARK_CEILING:
					this->pRoom->tileLights.Remove(wX,wY);
					this->pRoomWidget->bCeilingLightsRendered = false;
					bSomethingPlotted = true;
				break;
				case T_OVERHEAD_IMAGE:
					this->pRoom->overheadTiles.Remove(wX,wY);
					bSomethingPlotted = true;
				break;

				//Normal items that share the same layer.
				default:
					if (EraseAndPlot(wX,wY,GetSelectedObject(), true))
					{
						bSomethingDeleted = true;
						if (PlotObjectAt(wX,wY,GetSelectedObject(),this->wO))
							bSomethingPlotted = true;
					}
				break;
			}
		}

	this->wSelectedObject = originalSelectedObject;
}

//*****************************************************************************
void CEditRoomScreen::EraseObjectsOnAllLayers(bool& bSomethingPlotted, bool& bSomethingDeleted)
{
	const UINT originalSelectedObject = this->wSelectedObject;

	for (UINT layer=0; layer<=3; ++layer)
		EraseObjects(layer, bSomethingPlotted, bSomethingDeleted);

	this->wSelectedObject = T_CHECKPOINT;
	EraseObjects(0, bSomethingPlotted, bSomethingDeleted);
	this->wSelectedObject = T_WALLLIGHT;
	EraseObjects(0, bSomethingPlotted, bSomethingDeleted);

	this->wSelectedObject = originalSelectedObject;
}

//*****************************************************************************
void CEditRoomScreen::ReflectRoomX()
{
	ASSERT(this->pRoom);
	ASSERT(this->pHold);

	const bool bRoomHasEntrances = this->pRoomWidget->pLevelEntrances->size() > 0;
	Changing(bRoomHasEntrances ? RoomAndHold : Room);

	this->pRoom->ReflectX();
	this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);

	//Reflect level entrances.
	if (bRoomHasEntrances)
	{
		ASSERT(this->pRoomWidget->pLevelEntrances);
		CMoveCoord *pEntrance;
		for (UINT wIndex=0; wIndex<this->pRoomWidget->pLevelEntrances->size(); ++wIndex)
		{
			pEntrance = (*this->pRoomWidget->pLevelEntrances)[wIndex];
			CEntranceData *pHoldEntrance = this->pHold->GetEntranceAt(
					this->pRoom->dwRoomID, pEntrance->wX, pEntrance->wY);
			ASSERT(pHoldEntrance);
			pHoldEntrance->ReflectX(this->pRoom->wRoomCols);
			pEntrance->wX = pHoldEntrance->wX;
			pEntrance->wO = pHoldEntrance->wO;
		}
	}
}

//*****************************************************************************
void CEditRoomScreen::ReflectRoomY()
{
	ASSERT(this->pRoom);
	ASSERT(this->pHold);

	const bool bRoomHasEntrances = this->pRoomWidget->pLevelEntrances->size() > 0;
	Changing(bRoomHasEntrances ? RoomAndHold : Room);

	this->pRoom->ReflectY();
	this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);

	//Reflect level entrances.
	if (bRoomHasEntrances)
	{
		ASSERT(this->pRoomWidget->pLevelEntrances);
		CMoveCoord *pEntrance;
		for (UINT wIndex=0; wIndex<this->pRoomWidget->pLevelEntrances->size(); ++wIndex)
		{
			pEntrance = (*this->pRoomWidget->pLevelEntrances)[wIndex];
			CEntranceData *pHoldEntrance = this->pHold->GetEntranceAt(
					this->pRoom->dwRoomID, pEntrance->wX, pEntrance->wY);
			ASSERT(pHoldEntrance);
			pHoldEntrance->ReflectY(this->pRoom->wRoomRows);
			pEntrance->wY = pHoldEntrance->wY;
			pEntrance->wO = pHoldEntrance->wO;
		}
	}
}

//*****************************************************************************
void CEditRoomScreen::RecalcBrokenObstacle(
//Recalc obstacle tiles when a piece is removed.
//
//Params:
	const UINT wCX, const UINT wCY, //(in) top-left corner of obstacle that was removed
	const UINT wObIndex)        //(in) size index for obstacle's type
{
	ASSERT(this->pRoom->IsValidColRow(wCX, wCY));
	ASSERT(wObIndex);
	const UINT wXDim = obstacleDimensions[wObIndex][0];
	const UINT wYDim = obstacleDimensions[wObIndex][1];

	if (wXDim == 1 && wYDim == 1)
		return;  //1x1 obstacle was erased completely -- nothing to do here.

	//Reset remaining obstacle pieces.
	UINT wX, wY;
	for (wY=wCY; wY<wCY+wYDim; ++wY)
		for (wX=wCX; wX<wCX+wXDim; ++wX)
		{
			ASSERT(this->pRoom->IsValidColRow(wX,wY));
			if (this->pRoom->GetTSquare(wX,wY) == T_OBSTACLE)
			{
				const BYTE obType = calcObstacleType(this->pRoom->GetTParam(wX,wY));
				ASSERT(obType);   //plotting an obstacle can't clobber an existing obstacle
				this->obstacles.Add(wX, wY, obType);
			}
		}

	//Recalc largest pieces that will occupy remaining area.
	ObstacleFill();
}

//*****************************************************************************
void CEditRoomScreen::ShowPlot()
//Show proper type of room plot when placing/removing items.
{
	if (this->bAreaJustCopied)
		return;

	CWidget *pFocusWidget = MouseDraggingInWidget();
	if (pFocusWidget)
	{
		//Mouse drag.
		if (pFocusWidget->GetTagNo() == TAG_ROOM)
		{
			if (RightMouseButton())
				AddPlotEffect(T_EMPTY); //Deleting objects.
			else
				AddPlotEffect(GetSelectedObject());  //Plotting objects.
		}
	}
}

//*****************************************************************************
bool CEditRoomScreen::MergePressurePlate(const UINT wX, const UINT wY,
													  const bool bUpdateType) //[default=false]
//When adding a pressure plate tile to (x,y), attach it to any defined adjacent pressure plate.
//If more than one neighboring plate is defined, delete all but the first and merge the tiles into the first.
//
//Returns: true if merge to another plate occurred, else false
{
	COrbData *pPlate = NULL;
	for (UINT wI=0; wI<NEIGHBORS; ++wI)
	{
		const UINT wTX = wX + dx[wI];
		const UINT wTY = wY + dy[wI];
		if (this->pRoom->IsValidColRow(wTX,wTY) && this->pRoom->GetOSquare(wTX,wTY) == T_PRESSPLATE)
		{
			COrbData *pAdjPlate = this->pRoom->GetPressurePlateAtCoords(wTX,wTY);
			if (pPlate)
			{
				if (pAdjPlate && pPlate != pAdjPlate)
					this->pRoom->DeleteOrbAtSquare(pAdjPlate->wX,pAdjPlate->wY);
			}
			else
				pPlate = pAdjPlate;
		}
	}

	if (pPlate)
	{
		if (bUpdateType)
			pPlate->eType = (OrbType)this->wSelPlateType;
		this->pRoom->AddPressurePlateTiles(pPlate);
		return true;
	}
	return false;
}

//*****************************************************************************
CObjectMenuWidget* CEditRoomScreen::ObjectMenuForTile(const UINT wTile)
//Returns: pointer to the menu that the specified object is located in
{
	ASSERT(wTile < TOTAL_EDIT_TILE_COUNT);
	UINT dwMenu = 0;
	switch (TILE_LAYER[wTile])
	{
		case 0: dwMenu = TAG_OMENU; break;
		case 1: dwMenu = TAG_TMENU; break;
		case 2: dwMenu = TAG_MMENU; break;
		case 3: dwMenu = TAG_FMENU; break;
		default: ASSERT(!"Unknown layer value"); break;
	}
	CObjectMenuWidget *pObjectMenu = DYN_CAST(CObjectMenuWidget*, CWidget*, GetWidget(dwMenu));
	return pObjectMenu;
}

//*****************************************************************************
void CEditRoomScreen::ObstacleFill()
//Fill marked obstacle tiles with the largest obstacles
//of their type that will fit.  Top-down, left-to right.
{
	if (!this->obstacles.GetSize()) return;   //no obstacles marked

	UINT wX, wY, wI, wJ;
	for (wY=0; wY<this->pRoom->wRoomRows; ++wY)
		for (wX=0; wX<this->pRoom->wRoomCols; ++wX)
		{
			BYTE obType = this->obstacles.GetAt(wX,wY);
			if (!obType) continue; //no obstacle to fill in here

			//Scan for the largest (based on width) defined size for this obstacle type.
			UINT obMaxSizeX=0, obMaxSizeY=0, wObIndex;
			for (wI=0; wI<MAX_OBSTACLE_SIZE; ++wI)
			{
				wObIndex = obstacleIndices[obType][wI];
				if (!wObIndex) break; //done searching
				const UINT wSize = obstacleDimensions[wObIndex][0];
				if (wSize >= obMaxSizeX) //get widest/longest obstacle, assuming they are defined in increasing size order
				{
					obMaxSizeX = wSize;
					obMaxSizeY = obstacleDimensions[wObIndex][1];
				}
			}
			ASSERT(obMaxSizeX);
			ASSERT(obMaxSizeY);

			//Start an obstacle here.
			ASSERT(this->pRoom->GetTSquare(wX,wY) == T_OBSTACLE);
			UINT wSizeX = 1, wSizeY = 1;

			//Check how far this obstacle can extend, up to the maximum size found above.
			bool bOb = obMaxSizeX > wSizeX;
			while (bOb)
			{
				//Extend one square at a time to the right.
				if (wX+wSizeX >= this->pRoom->wRoomCols)
					break; //An obstacle this size would exceed room boundaries -- don't make it that big.

				if (this->obstacles.GetAt(wX+wSizeX, wY) != obType)
					break;
				if (++wSizeX == obMaxSizeX)
					break; //reached max possible size -- don't need to extend any further
			}
			bOb = obMaxSizeY > wSizeY;
			while (bOb)
			{
				//Extend one row at a time downward.
				if (wY+wSizeY >= this->pRoom->wRoomRows)
					break;
				for (wI=wX; wI<wX+wSizeX; ++wI)
					if (this->obstacles.GetAt(wI, wY+wSizeY) != obType)
					{
						bOb = false;
						break;
					}
				if (!bOb) break;
				if (++wSizeY == obMaxSizeY)
					break;
			}

			//Use smaller obstacle if one doesn't exist for this found size.
			for (wI=MAX_OBSTACLE_SIZE; wI--; )
			{
				wObIndex = obstacleIndices[obType][wI];
				if (!wObIndex)
					continue;

				if (wSizeX >= obstacleDimensions[wObIndex][0] &&
						wSizeY >= obstacleDimensions[wObIndex][1])
				{
					//Use this size -- the largest available one within this size constraint.
					wSizeX = obstacleDimensions[wObIndex][0];
					wSizeY = obstacleDimensions[wObIndex][1];
					break;
				}
				ASSERT(wI > 0); //1x1 obstacles should always be available
			}

			//Mark off obstacle.
			obType |= OBSTACLE_TOP; //add this flag to top row
			for (wJ=wY; wJ<wY+wSizeY; ++wJ)
			{
				obType |= OBSTACLE_LEFT; //add this flag to left column
				for (wI=wX; wI<wX+wSizeX; ++wI)
				{
					this->obstacles.Remove(wI,wJ);
					this->pRoom->SetTParam(wI, wJ, obType);
					obType = obType & (BYTE)~OBSTACLE_LEFT; //remove this flag after left column
				}
				obType = obType & (BYTE)~OBSTACLE_TOP; //remove this flag after top row
			}

			if (!this->obstacles.GetSize()) return;   //quick return once everything has been handled
		}

	ASSERT(!this->obstacles.GetSize()); //there should be no pieces left to consider
}

//************************************************************************************
void CEditRoomScreen::DrawHalphSlayerEntrances()
{
#define AddShadeToTileXY(x,y,Color) this->pRoomWidget->AddShadeEffect(x,y,Color)
	static const SURFACECOLOR DarkYellow = {196, 196, 0};
	static const SURFACECOLOR DarkYellow2 = {128, 128, 0};
	static const SURFACECOLOR Purple = {196, 0, 196};
	static const SURFACECOLOR Purple2 = {128, 0, 128};

	//Draw Halph/Slayer entrances.
	CCoordSet::const_iterator enters;
	for (enters=this->pRoom->halphEnters.begin(); enters!=this->pRoom->halphEnters.end(); ++enters)
		AddShadeToTileXY(enters->wX, enters->wY, DarkYellow);
	for (enters=this->pRoom->halph2Enters.begin(); enters!=this->pRoom->halph2Enters.end(); ++enters)
		AddShadeToTileXY(enters->wX, enters->wY, DarkYellow2);
	for (enters=this->pRoom->slayerEnters.begin(); enters!=this->pRoom->slayerEnters.end(); ++enters)
		AddShadeToTileXY(enters->wX, enters->wY, Purple);
	for (enters=this->pRoom->slayer2Enters.begin(); enters!=this->pRoom->slayer2Enters.end(); ++enters)
		AddShadeToTileXY(enters->wX, enters->wY, Purple2);

#undef AddShadeToTileXY
}

//*****************************************************************************
void CEditRoomScreen::FixUnstableTar()
//Remove unstable tar.
{
	//TODO: merge with CDbRoom::FixUnstableTar ?
	bool bStable;
	UINT wX, wY, wTTile;
	do {
		bStable = true;
		for (wY=0; wY<this->pRoom->wRoomRows; ++wY)
			for (wX=0; wX<this->pRoom->wRoomCols; ++wX)
			{
				wTTile = this->pRoom->GetTSquare(wX,wY);
				if (bIsTarOrFluff(wTTile))
					if (!this->pRoom->IsTarStableAt(wX,wY,wTTile))
					{
						CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
						if (pMonster && wTTile != T_FLUFF && bIsMother(pMonster->wType))
							continue; //allow unstable tar to remain under a mother
						PlotObjectAt(wX,wY,T_EMPTY,0);
						bStable = false;
					}
			}
	} while (!bStable);
}

//*****************************************************************************
void CEditRoomScreen::FixCorruptStaircase(
//Remove part of staircase definition when stair tile is removed.
//
//Params:
	const UINT wX, const UINT wY) //(in) Where stair tile was removed
{
	CCoordStack EvalCoords; //Contains coords to evaluate.
	UINT wMinX = wX, wMaxX = wX, wMinY = wY, wMaxY = wY;  //staircase borders
	const UINT wStairsIndex = this->pRoom->GetExitIndexAt(wX,wY);
	this->pRoom->DeleteExitAtSquare(wX, wY);

	//Push on all squares adjacent to where stair tile was removed.
	if (wX > 0)
		EvalCoords.Push(wX - 1, wY);
	if (wX < this->pRoom->wRoomCols - 1)
		EvalCoords.Push(wX + 1, wY);
	if (wY > 0)
		EvalCoords.Push(wX, wY - 1);
	if (wY < this->pRoom->wRoomRows - 1)
		EvalCoords.Push(wX, wY + 1);

	//Each iteration pops one pair of coordinates for removing stair tiles,
	//and will add coords of adjacent stair tiles for removal.
	//Exits when there are no more coords in stack to evaluate.
	UINT wEvalX, wEvalY;
	while (EvalCoords.Pop(wEvalX, wEvalY))
	{
		ASSERT(this->pRoom->IsValidColRow(wEvalX, wEvalY));

		//Is a stair tile here?
		const UINT wTileNo = this->pRoom->GetOSquare(wEvalX, wEvalY);
		switch (wTileNo)
		{
		case T_STAIRS:
		case T_STAIRS_UP:
			{
			//Be careful not to delete distinct adjoining staircases.
			const UINT wTemp = this->pRoom->GetExitIndexAt(wEvalX,wEvalY);
			if (wTemp != wStairsIndex && wTemp != static_cast<UINT>(-1))
				break;
			this->pRoom->DeleteExitAtSquare(wEvalX, wEvalY);

			this->pRoom->Plot(wEvalX, wEvalY, this->wLastFloorSelected);  //remove stairs

			//Add adjacent (4-neighbor) coords to eval stack.
			if (wEvalX > 0)
				EvalCoords.Push(wEvalX - 1, wEvalY);
			if (wEvalX < this->pRoom->wRoomCols - 1)
				EvalCoords.Push(wEvalX + 1, wEvalY);
			if (wEvalY > 0)
				EvalCoords.Push(wEvalX, wEvalY - 1);
			if (wEvalY < this->pRoom->wRoomRows - 1)
				EvalCoords.Push(wEvalX, wEvalY + 1);

			//Keep track of bounds of staircase to check wall corner tiles.
			if (wEvalX < wMinX)
				wMinX = wEvalX;
			if (wEvalX > wMaxX)
				wMaxX = wEvalX;
			if (wEvalY < wMinY)
				wMinY = wEvalY;
			if (wEvalY > wMaxY)
				wMaxY = wEvalY;
			}
			break;
		}
	}
}

//*****************************************************************************
void CEditRoomScreen::RepairYellowDoors(const UINT doorType)
//After plotting door tiles to room:
//1. Remove multiple orb associations to a single (merged) door.
//2. Make adjacent door tiles all open or closed.
{
	CCoordSet doors = this->ignoreCoords;
	this->ignoreCoords.clear();

	for (CCoordSet::const_iterator door = doors.begin(); door != doors.end(); ++door)
	{
		UINT wX = door->wX, wY = door->wY, wCurX, wCurY;
		ASSERT(bIsYellowDoor(this->pRoom->GetOSquare(wX,wY)));

		CCoordSet coords;
		this->pRoom->GetAllYellowDoorSquares(wX, wY, coords, &this->ignoreCoords);
		if (coords.empty())
			continue;

		// Make sure each orb affects a door no more than once
		for (UINT wIndex=0; wIndex<this->pRoom->orbs.size(); ++wIndex)
		{
			bool bFirst = true;
			COrbData *pOrb = this->pRoom->orbs[wIndex];
			ASSERT(pOrb);
			for (UINT wAgent=0; wAgent<pOrb->agents.size(); ++wAgent)
			{
				if (coords.has(pOrb->agents[wAgent]->wX, pOrb->agents[wAgent]->wY))
				{
					if (bFirst)
						bFirst = false;
					else
					{
						// already have an agent for this orb/door - remove this one.
						pOrb->DeleteAgent(pOrb->agents[wAgent]);
						// we're removing this one, so we need to back up wAgent by one.
						--wAgent;
					}
				}
			}
		}

		this->ignoreCoords += coords;

		while (coords.pop_first(wCurX, wCurY))
		{
			const UINT wTileNo = this->pRoom->GetOSquare(wCurX, wCurY);
			ASSERT(bIsYellowDoor(wTileNo));
			if (wTileNo != doorType)
				this->pRoom->Plot(wCurX, wCurY, doorType);
		}
	}
}

//*****************************************************************************
void CEditRoomScreen::RemoveChange()
//Removes last hold+room state from the undo stack.
{
	vector<CDbBase*> lastChange = this->undoList.back();
	for (vector<CDbBase*>::iterator change = lastChange.begin();
			change != lastChange.end(); ++change)
		delete *change;
	this->undoList.pop_back();
	SetButtons();
}

//*****************************************************************************
bool CEditRoomScreen::RemoveMonster(
//Remove monster.
//
//Returns: whether monster was deleted.
//
//Params:
	CMonster *pMonster)
{
	ASSERT(pMonster);

	pMonster = pMonster->GetOwningMonster();

	//Mark character speech commands for removal from DB.
	if (pMonster->wType == M_CHARACTER)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		ASSERT(pCharacter);
		for (UINT wIndex=pCharacter->commands.size(); wIndex--; )
		{
			if (pCharacter->commands[wIndex].pSpeech)
				this->pRoom->MarkSpeechForDeletion(
						pCharacter->commands[wIndex].pSpeech);
		}
	}

	const bool bRecalcSwords = bEntityHasSword(pMonster->wType) || pMonster->wType == M_CHARACTER;
	CCueEvents Ignored;
	this->pRoom->KillMonsterAtSquare(pMonster->wX,pMonster->wY,Ignored,true);
	if (bRecalcSwords)
		this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);

	return true;
}

//*****************************************************************************
bool CEditRoomScreen::RemoveObjectAt(
//Removes customizable object on (x,y) in the layer of the replacing object.
//
//Returns: whether object was removed
//
//Params:
	const UINT wX, const UINT wY, //(in)   Coord
	const UINT wPlottedObject,   //(in)   Object being plotted
	bool &bObRemoved,       //(out)  Obstacle tile was removed
	bool &bTarRemoved,      //(out)  Tar was removed
	bool &bStairsRemoved)   //(out)  Stairs were removed
{
	CDbRoom& room = *(this->pRoom);
	const UINT wOTileNo = room.GetOSquare(wX,wY),
			fTile = room.GetFSquare(wX,wY),
			wTTileNo = room.GetTSquare(wX,wY),
			wLayer = TILE_LAYER[wPlottedObject];
	CMonster *pMonster;

	bool bSuccess = true;
	switch (wLayer)
	{
	case 0:
		//O-layer
		switch (wOTileNo)
		{
			case T_STAIRS:
			case T_STAIRS_UP:
				if (wPlottedObject != wOTileNo)
					bStairsRemoved = true;
			break;
			case T_FIRETRAP:
			case T_FIRETRAP_ON:
				RemoveOrbAssociationAt(wX,wY);
			break;

			case T_DOOR_Y:
			case T_DOOR_YO:
				room.RemoveYellowDoorTile(wX,wY,wOTileNo);
			break;
			case T_PRESSPLATE:
				if (room.RemovePressurePlateTile(wX,wY))
					room.DeleteOrbAtSquare(wX,wY);
				room.Plot(wX,wY,wPlottedObject); //make plot now so pressure plate tiles are updated correctly
			break;
		}
	break;

	case 1:
		//T-layer
		switch (wTTileNo)
		{
			case T_OBSTACLE:
				if (wPlottedObject != wOTileNo)
					bObRemoved = true;
			break;

			case T_TAR:	case T_MUD: case T_GEL: case T_FLUFF:
				if (wPlottedObject != wTTileNo)
					bTarRemoved = true;
			break;

			case T_SCROLL:
				room.DeleteScrollTextAtSquare(wX,wY);
			break;
			case T_ORB:
				room.DeleteOrbAtSquare(wX,wY);
			break;
			case T_LIGHT:
				RemoveOrbAssociationAt(wX,wY);
			break;
		}
	break;

	case 2:
      switch (wPlottedObject)
		{
			case T_NOMONSTER:
				//Plotting an empty square will remove the monster there.
				pMonster = room.GetMonsterAtSquare(wX,wY);
				if (pMonster)
				{
					//Confirm monster deletion if it's not (a part of) a long monster
					//being edited right now.
					pMonster = pMonster->GetOwningMonster();
					if (pMonster != this->pLongMonster)
					{
						const bool bTarstuffMother = bIsMother(pMonster->wType);
						if (!RemoveMonster(pMonster))
							return false;
						if (bTarstuffMother)
							bTarRemoved = true;
					}
				} else {
					//Remove Halph/Slayer entrance.
					room.halphEnters.erase(wX, wY);
					room.halph2Enters.erase(wX, wY);
					room.slayerEnters.erase(wX, wY);
					room.slayer2Enters.erase(wX, wY);

					//Is a level entrance here?
					bool bSwordsmanAt, bSwordAt;
					this->pRoomWidget->IsLevelStartAt(wX, wY, bSwordsmanAt, bSwordAt);
					if (bSwordsmanAt)
						if (!DeleteLevelEntrance(wX, wY))
							bSuccess = false;
				}
			break;
			default: break;
		}
	break;

	case 3:
		switch (fTile)
		{
			case T_ARROW_N: case T_ARROW_NE: case T_ARROW_E: case T_ARROW_SE:
			case T_ARROW_S: case T_ARROW_SW: case T_ARROW_W: case T_ARROW_NW:
			case T_ARROW_OFF_N: case T_ARROW_OFF_NE: case T_ARROW_OFF_E: case T_ARROW_OFF_SE:
			case T_ARROW_OFF_S: case T_ARROW_OFF_SW: case T_ARROW_OFF_W: case T_ARROW_OFF_NW:
				if (!bIsLight(wTTileNo) && !bIsYellowDoor(wOTileNo) && !bIsFiretrap(wOTileNo)){
					if (!bIsAnyForceArrow(fTile) || !bIsAnyForceArrow(wPlottedObject)){
						RemoveOrbAssociationAt(wX, wY);
					}
				}
			break;
		}
	break;

	default: ASSERT(!"Invalid layer"); break;
	}  //layer

	return bSuccess;
}

//*****************************************************************************
void CEditRoomScreen::RemoveOrbAssociationAt(const UINT wX, const UINT wY)
{
	CDbRoom& room = *(this->pRoom);
	for (UINT wIndex=0; wIndex<room.orbs.size(); ++wIndex)
	{
		COrbData* pOrb = room.orbs[wIndex];
		COrbAgentData *pAgent = FindOrbAgentFor(wX,wY,pOrb);
		if (pAgent)
			VERIFY(pOrb->DeleteAgent(pAgent));
	}
}

//*****************************************************************************
void CEditRoomScreen::Changing(
//Marks the dirty bit.
//Saves current room and/or hold state in the undo sequence and clears the redo list.
//
//Params:
	const Change eChange)   //data type being changed [default=CEditRoomScreen::Room]
{
	this->bRoomDirty = true;

	if (static_cast<int>(this->undoList.size()) < this->nUndoSize)
		this->nUndoSize = -1;   //now impossible to revert to clean state

	//Save room state.
	vector<CDbBase*> changes;
	switch (eChange)
	{
		case RoomAndHold:
			changes.push_back(new CDbHold(*(this->pHold)));
			//NO BREAK
		case Room:
			changes.push_back(new CDbRoom(*(this->pRoom), true, true));
		break;
		case Hold:
			changes.push_back(new CDbHold(*(this->pHold)));
		break;
		default: ASSERT(!"Unrecognized Change enum"); break;
	}
	this->undoList.push_back(changes);

	//Clear redo list.
	ClearList(this->redoList);

	//Update undo/redo button states.
	SetButtons();
}

//*****************************************************************************
void CEditRoomScreen::RotateClockwise()
//Set item orientation or obstacle type for placement.
{
	RemoveToolTip();
	this->dwLastMouseMove = SDL_GetTicks();

	if (this->eState == ES_TESTROOM)
	{
		this->wO = nNextCO(this->wO);
		this->pTabbedMenu->SetTabTile(MONSTER_TAB, MonsterTileImageArray[M_ROACH][this->wO]);
		this->pTabbedMenu->Paint();
		g_pTheSound->PlaySoundEffect(SEID_SWING);
		return;
	}

	switch (this->wSelectedObject)
	{
		case T_LIGHT: case T_WALLLIGHT:
		case T_LIGHT_CEILING:
		case T_STATION:
			if (this->eState != ES_PLACING) return;

			//Select next valid type.
			this->wSelectedLightType =
					this->wSelectedLightType == NUM_LIGHT_TYPES-1 ? 0 : this->wSelectedLightType + 1;
			UpdateMenuGraphic(this->wSelectedObject);
		break;
		case T_DARK_CEILING:
			if (this->eState != ES_PLACING) return;

			//Select next valid type.
			this->wSelectedDarkType =
					this->wSelectedDarkType == NUM_DARK_TYPES-1 ? 0 : this->wSelectedDarkType + 1;
			UpdateMenuGraphic(this->wSelectedObject);
		break;
		case T_OBSTACLE:
		{
			if (this->eState != ES_PLACING) return;

			//Select next obstacle type in display list.
			ASSERT(numObstacles);
			UINT obIndex=0;
			while (this->wSelectedObType != mObstacleEntries[obIndex] && obIndex<numObstacles)
				++obIndex;
			ASSERT(obIndex<numObstacles);
			if (obIndex == numObstacles-1)
				this->wSelectedObType = mObstacleEntries[0];
			else
				this->wSelectedObType = mObstacleEntries[obIndex+1];
			UpdateMenuGraphic(T_OBSTACLE);
		}
		break;
		case T_ORB:
			switch (this->wSelOrbType)
			{
				case OT_NORMAL: this->wSelOrbType = OT_ONEUSE; break;
				case OT_ONEUSE: this->wSelOrbType = OT_BROKEN; break;
				case OT_BROKEN: default: this->wSelOrbType = OT_NORMAL; break;
			}
			UpdateMenuGraphic(T_ORB);
		break;
		case T_PRESSPLATE:
			switch (this->wSelPlateType)
			{
				case OT_NORMAL: this->wSelPlateType = OT_TOGGLE; break;
				case OT_TOGGLE: this->wSelPlateType = OT_ONEUSE; break;
				case OT_ONEUSE: default: this->wSelPlateType = OT_NORMAL; break;
			}
			UpdateMenuGraphic(T_PRESSPLATE);
		break;
		case T_TOKEN:
			if (this->eState != ES_PLACING) return;
			//Select next valid token type.
			this->wSelTokenType =
					this->wSelTokenType == RoomTokenCount-1 ? 0 : this->wSelTokenType + 1;
			UpdateMenuGraphic(T_TOKEN);
		break;
		case T_WATER:
			if (!bGroupMenuItems)
			{
				this->wSelWaterType ^= (T_WATER ^ T_SHALLOW_WATER);
				UpdateMenuGraphic(T_WATER);
				break;
			}
			//fall through
		default:
			if (!ToggleMenuItem(this->wSelectedObject, true))
			{
				this->wO = nNextCO(this->wO);
				this->pTabbedMenu->SetTabTile(MONSTER_TAB, MonsterTileImageArray[M_ROACH][this->wO]);
				this->pTabbedMenu->Paint();
			}
		break;
	}
	g_pTheSound->PlaySoundEffect(SEID_SWING);
}

//*****************************************************************************
void CEditRoomScreen::RotateCounterClockwise()
//Set item orientation or obstacle type for placement.
{
	RemoveToolTip();
	this->dwLastMouseMove = SDL_GetTicks();

	if (this->eState == ES_TESTROOM)
	{
		this->wO = nNextCCO(this->wO);
		this->pTabbedMenu->SetTabTile(MONSTER_TAB, MonsterTileImageArray[M_ROACH][this->wO]);
		this->pTabbedMenu->Paint();
		g_pTheSound->PlaySoundEffect(SEID_SWING);
		return;
	}

	switch (this->wSelectedObject)
	{
		case T_LIGHT: case T_WALLLIGHT:
		case T_LIGHT_CEILING:
		case T_STATION:
			if (this->eState != ES_PLACING) return;

			//Select previous valid type.
			this->wSelectedLightType =
					this->wSelectedLightType == 0 ? NUM_LIGHT_TYPES-1 : this->wSelectedLightType - 1;
			UpdateMenuGraphic(this->wSelectedObject);
		break;
		case T_DARK_CEILING:
			if (this->eState != ES_PLACING) return;

			//Select previous valid type.
			this->wSelectedDarkType =
					this->wSelectedDarkType == 0 ? NUM_DARK_TYPES-1 : this->wSelectedDarkType - 1;
			UpdateMenuGraphic(this->wSelectedObject);
		break;
		case T_OBSTACLE:
		{
			if (this->eState != ES_PLACING) return;

			//Select previous obstacle type in display list.
			ASSERT(numObstacles);
			UINT obIndex=0;
			while (this->wSelectedObType != mObstacleEntries[obIndex] && obIndex<numObstacles)
				++obIndex;
			ASSERT(obIndex<numObstacles);
			if (!obIndex)
				this->wSelectedObType = mObstacleEntries[numObstacles-1];
			else
				this->wSelectedObType = mObstacleEntries[obIndex-1];
			UpdateMenuGraphic(T_OBSTACLE);
		}
		break;
		case T_ORB:
			switch (this->wSelOrbType)
			{
				case OT_NORMAL: this->wSelOrbType = OT_BROKEN; break;
				case OT_BROKEN: this->wSelOrbType = OT_ONEUSE; break;
				case OT_ONEUSE: default: this->wSelOrbType = OT_NORMAL; break;
			}
			UpdateMenuGraphic(T_ORB);
		break;
		case T_PRESSPLATE:
			switch (this->wSelPlateType)
			{
				case OT_NORMAL: this->wSelPlateType = OT_ONEUSE; break;
				case OT_ONEUSE: this->wSelPlateType = OT_TOGGLE; break;
				case OT_TOGGLE: default: this->wSelPlateType = OT_NORMAL; break;
			}
			UpdateMenuGraphic(T_PRESSPLATE);
		break;
		case T_TOKEN:
			if (this->eState != ES_PLACING) return;
			//Select previous valid token type.
			this->wSelTokenType =
					this->wSelTokenType == 0 ? RoomTokenCount-1 : this->wSelTokenType - 1;
			UpdateMenuGraphic(T_TOKEN);
		break;
		case T_WATER:
			if (!bGroupMenuItems)
			{
				this->wSelWaterType ^= (T_WATER ^ T_SHALLOW_WATER);
				UpdateMenuGraphic(T_WATER);
				break;
			}
			//fall through
		default:
			if (!ToggleMenuItem(this->wSelectedObject, false))
			{
				this->wO = nNextCCO(this->wO);
				this->pTabbedMenu->SetTabTile(MONSTER_TAB, MonsterTileImageArray[M_ROACH][this->wO]);
				this->pTabbedMenu->Paint();
			}
		break;
	}
	g_pTheSound->PlaySoundEffect(SEID_SWING);
}

//*****************************************************************************
bool CEditRoomScreen::SaveRoom()
//Updates the current room and level data.
//
//Returns: whether room is current (i.e. not dirty)
{
	ASSERT(this->pRoom);
	ASSERT(this->pLevel);

	if (!this->bRoomDirty)
		return true;   //save not needed

	if (this->bAutoSave || ShowYesNoMessage(MID_SaveRoomPrompt) == TAG_YES)
		SaveRoomToDB();

	return !this->bRoomDirty;
}

//*****************************************************************************
bool CEditRoomScreen::SaveRoomToDB()
//Updates the current room and level data, if hold is owned by editing player.
//
//Returns: whether player is the author of the hold being edited
{
	LOGCONTEXT("CEditRoomScreen::SaveRoomToDB");

	ASSERT(this->pHold);
	//Does the player own this hold?
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);
	if (this->pHold->dwPlayerID != dwCurrentPlayerID)
	{
		//Editing a room in a hold you don't own will make a modified copy of
		//the hold owned by you.
		if (ShowYesNoMessage(MID_MakeModifiedHoldCopy) != TAG_YES)
		{
			//User doesn't want to make a modified copy of the hold -- don't save anything.
			return false;
		}

		SetCursor(CUR_Wait);
		Paint();

		//Copy hold.
		CDbHold *pNewHold = this->pHold->MakeCopy();
		pNewHold->ChangeAuthor(dwCurrentPlayerID);

		//Get new version of current room.
		const UINT dwNewLevelID = CDbHolds::GetLevelIDAtIndex(this->pLevel->dwLevelIndex, pNewHold->dwHoldID);
		ASSERT(dwNewLevelID);
		CDbLevel *pNewLevel = g_pTheDB->Levels.GetByID(dwNewLevelID);
		ASSERT(pNewLevel);

		const UINT dwNewRoomY = (pNewLevel->dwLevelID * 100) + (this->pRoom->dwRoomY % 100);
	   CDbRoom *pNewRoom = pNewLevel->GetRoomAtCoords(this->pRoom->dwRoomX, dwNewRoomY);
		ASSERT(pNewRoom);
		const UINT dwRoomID = pNewRoom->dwRoomID;
		const UINT dwRoomX = this->pRoom->dwRoomX;
		pNewRoom->MakeCopy(*this->pRoom);   //save modified room (copy scroll texts + data, and don't overwrite IDs)
		pNewRoom->dwLevelID = pNewLevel->dwLevelID;
		pNewRoom->dwRoomID = dwRoomID;
		pNewRoom->dwRoomX = dwRoomX;
		pNewRoom->dwRoomY = dwNewRoomY;
		pNewRoom->Update();

		SetRoom(pNewRoom->dwRoomID);	//clears undo/redo lists

		//Update widgets in main Editor screen.
		CEditSelectScreen *pEditSelectScreen = DYN_CAST(CEditSelectScreen *, CScreen *,
				g_pTheSM->GetScreen(SCR_EditSelect));
		ASSERT(pEditSelectScreen);
		pEditSelectScreen->SetToCopiedHold(pNewHold, pNewLevel);
		Paint();

		delete pNewRoom;
		delete pNewLevel;
		delete pNewHold;
	} else {
		SetCursor(CUR_Wait);

		//Update timestamps (even if nothing was changed in room).
		this->pHold->Update();
		this->pLevel->Update();
		this->pRoom->Update();

		//Since the room has been modified, saved games in the room might have
		//broken.  Verify them for correctness.
		//NOTE: This isn't really essential, so I'm commenting this out to save player time.
		//g_pTheDB->SavedGames.VerifyForRoom(this->pRoom->dwRoomID);
	}

	g_pTheDB->Commit();

	this->bRoomDirty = false;

	this->pMapWidget->UpdateFromCurrentLevel();

	SetCursor();

	return true;
}

//*****************************************************************************
void CEditRoomScreen::SetButtons(const bool bPaint)   //[default=true]
//Set state of undo/redo buttons.
{
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_UNDO));
	pButton->Enable(!this->undoList.empty());
	if (this->undoList.empty() && pButton->IsSelected())
		SelectPrevWidget(bPaint);
	if (bPaint)
		pButton->RequestPaint();
	pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_REDO));
	pButton->Enable(!this->redoList.empty());
	if (this->redoList.empty() && pButton->IsSelected())
		SelectPrevWidget(bPaint);
	if (bPaint)
		pButton->RequestPaint();
}

//*****************************************************************************
void CEditRoomScreen::SetSelectedObject(const UINT wObject)
//Sets the selected menu item and updates item text display.
{
	if (this->wSelectedObject != wObject)
	{
		RemoveToolTip();
		this->dwLastMouseMove = SDL_GetTicks();

		this->wSelectedObject = wObject;

		//Selected floor type is used to erase o-layer.
		if (bIsPlainFloor(wObject) && wObject != T_FLOOR_IMAGE)
			this->wLastFloorSelected = wObject;
	}
	if (int(wObject) < 0)
		return;

	UINT wMID = 0;
	switch (wObject)
	{
		case T_WATER:
			wMID = TILE_MID[this->bGroupMenuItems ? wObject : this->wSelWaterType];
		break;
		case T_ORB:
			wMID = this->pRoomWidget->GetOrbMID(this->wSelOrbType);
		break;
		case T_PRESSPLATE:
			wMID = this->pRoomWidget->GetPressurePlateMID(this->wSelPlateType);
		break;
		case T_TOKEN:
			wMID = this->pRoomWidget->GetTokenMID(this->wSelTokenType);
		break;
		default:
			wMID = TILE_MID[wObject];
		break;
	}
	if (wMID)
	{
		CLabelWidget *pItemLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAB_ITEMTEXT));
		pItemLabel->SetText(g_pTheDB->GetMessageText(wMID));

		//Flag to refresh item text at the proper time.
		this->bPaintItemText = true;
	}
}

//*****************************************************************************
UINT CEditRoomScreen::GetSelectedObject()
//Returns the value of wSelectedObject, but also takes Shallow Water
//type into account
{
	if (this->wSelectedObject == T_WATER && !this->bGroupMenuItems)
		return this->wSelWaterType;

	return this->wSelectedObject;
}

//*****************************************************************************
void CEditRoomScreen::SetSignTextToCurrentRoom(
//Set sign text to description of current room and repaint it.
//
//Params:
	const bool bMarkAsEntrance)   //(in) describe room as the entrance [default=false]
{
	static const WCHAR wszSignSep[] = { We(':'),We(' '),We(0) };
	WSTRING wstrSignText = (const WCHAR *)this->pLevel->NameText;
	wstrSignText += wszSignSep;
	if (bMarkAsEntrance)
		wstrSignText += g_pTheDB->GetMessageText(MID_TheEntrance);
	else
		this->pRoom->GetLevelPositionDescription(wstrSignText);
	if (this->eState != ES_PLACING) wstrSignText += wszSpace;
	switch (this->eState)
	{
	case ES_PLACING: break;
	case ES_SCROLL:
		wstrSignText += g_pTheDB->GetMessageText(MID_EnteringScrollStatus);
		break;
	case ES_LONGMONSTER:
		wstrSignText += g_pTheDB->GetMessageText(MID_PlacingLongMonsterStatus);
		break;
	case ES_ORB:
		wstrSignText += g_pTheDB->GetMessageText(MID_DefiningOrbStatus);
		break;
	case ES_DOOR:
		wstrSignText += g_pTheDB->GetMessageText(MID_DefiningDoorStatus);
		break;
	case ES_TESTROOM:
		wstrSignText += g_pTheDB->GetMessageText(MID_TestRoomLocation);
		break;
	case ES_GETSQUARE:
		wstrSignText += g_pTheDB->GetMessageText(MID_GetRoomSquare);
		break;
	case ES_GETRECT:
		wstrSignText += g_pTheDB->GetMessageText(MID_GetRoomRect);
		break;
	case ES_PASTING:
		wstrSignText += g_pTheDB->GetMessageText(MID_PastingRegionStatus);
		break;
	default: ASSERTP(false, "Invalid editor state"); break;
	}
	SetSignText(wstrSignText.c_str());
}

//*****************************************************************************
bool CEditRoomScreen::SetState(
//Set editing state and update the screen accordingly.
//
//Params:
	const EditState eState,    //(in)   New state.
	const bool bForce)	//(in) always allow entering new state [default=false]
{
	if (eState == this->eState) return true; //no change

	//Forbid changing state while a command is still being defined.
	if (!bForce && !this->pCharacterDialog->IsCommandFinished())
		return false;

	if (this->eState == ES_SCROLL)
		HideScroll();

	switch (eState)
	{
		case ES_PASTING:
			this->pOrb = NULL;
			this->pLongMonster = NULL;
			this->pRoomWidget->ResetPlot();
			this->pRoomWidget->wOX = static_cast<UINT>(-1);
			this->pRoomWidget->ClearEffects();
			this->bSelectingImageStart = false;

			this->pRoomWidget->bPlacing = false;
		break;
		case ES_PLACING:
		case ES_TESTROOM:
			this->pOrb = NULL;
			this->pLongMonster = NULL;
			this->pRoomWidget->ResetPlot();
			this->pRoomWidget->wOX = static_cast<UINT>(-1);
			this->pRoomWidget->ClearEffects();
			this->bSelectingImageStart = false;

			this->pRoomWidget->bPlacing = true;
		break;

		default:
			this->pRoomWidget->bPlacing = false;
		break;
	}

	//Restore any saved selected object on state change.
	if (this->wSelectedObjectSave != static_cast<UINT>(-1))
	{
		this->wSelectedObject = this->wSelectedObjectSave;
		this->wSelectedObjectSave = static_cast<UINT>(-1);
	}

	this->eState = eState;
	this->pRoomWidget->eEditState = eState;

	this->pEffects->RemoveEffectsOfType(ETOOLTIP);
	SetSignTextToCurrentRoom();
	PaintSign();
	return true;
}

//************************************************************************************
bool CEditRoomScreen::SetUnspecifiedPlayerSettings(
//Any player setting that is unspecified will get a default value.
//This is where the default settings for the editor are defined.
//
//Params:
	CDbPackedVars  &Settings)  //(in/out)  When received, param may contain any number
								//       of vars that match or don't match expected
								//       vars.  Returned with all expected vars set.
{
	bool bUpdate = false;
	//Set-if-missing macros.
#  define SETMISSING(name, value) if (!Settings.DoesVarExist(name))\
		{Settings.SetVar(name, value);   bUpdate = true;}

	SETMISSING(Settings::ShowErrors, true);

#  undef SETMISSING

	return bUpdate;
}

//*****************************************************************************
inline bool bIsMonsterRequired(const UINT wMT)
//Returns: whether removing monsters of this type is required to conquer a room
{
	switch (wMT)
	{
		case M_CHARACTER:
		case M_CITIZEN: case M_ARCHITECT:
		case M_FEGUNDO: case M_FEGUNDOASHES:
		case M_HALPH: case M_HALPH2:
		case M_SLAYER: case M_SLAYER2:
		case M_STALWART: case M_STALWART2:
		case M_WUBBA: case M_FLUFFBABY:
		case M_MIMIC: case M_DECOY: case M_CLONE:
		case M_TEMPORALCLONE:
		case M_GENTRYII:
			return false;
		default: return true;
	}
}

//*****************************************************************************
enum TERRAIN_TYPE
{
	TT_FLOOR = 0,
	TT_WALL = 1,
	TT_PIT = 2,
	TT_WATER = 3,
	TT_OBSTACLE = 4
};

TERRAIN_TYPE terrainType(const UINT tile[4])
//Returns: a number representing what movement type can go through this tile terrain
{
	if (bIsWall(tile[0]))
		return TT_WALL;

	if (bIsPit(tile[0]))
		return TT_PIT;

	if (bIsWater(tile[0]))
		return TT_WATER;

	if (tile[1] == T_OBSTACLE || tile[1] == T_STATION || tile[1] == T_ORB ||
			bIsBeacon(tile[1]) || bIsTarOrFluff(tile[1]) || bIsSerpentOrGentryii(tile[2]))
		return TT_OBSTACLE;

	return TT_FLOOR;
}

bool bMatchingTerrainTypes(const TERRAIN_TYPE t1, const TERRAIN_TYPE t2)
//Returns: whether two terrain types don't match across a room edge
{
	if (t1 == TT_OBSTACLE && t2 != TT_FLOOR)
		return true;
	if (t2 == TT_OBSTACLE && t1 != TT_FLOOR)
		return true;

	return t1 == t2;
}

//*****************************************************************************
void CEditRoomScreen::ShowErrors()
//Highlight room squares with errors.
{
	static const SURFACECOLOR Red = {255, 0, 0};
	static const SURFACECOLOR Blue = {0, 0, 255};
	static const SURFACECOLOR LightRed = {255, 128, 128};
	static const SURFACECOLOR LightGreen = {128, 255, 128};
	static const SURFACECOLOR LightBlack = {192, 192, 192};
	static const SURFACECOLOR Orange = {255, 128, 0};
	static const SURFACECOLOR PaleGreen = {230, 255, 230};
	static const SURFACECOLOR Purple = {255, 128, 255};

	UINT wTileNo[4], wAdjTile[4];
	bool bMatchingEdge;
	bool bRedDoors = false, bTrapdoors = false;  //first must imply second to be meaningful
	bool bGreenDoors = false, bMonsters = false; //" "
	bool bBlackDoors = false, bTar = false;      //" "
	CMonster *pMonster;
	UINT wX, wY;

#define AddShadeToTile(Color) this->pRoomWidget->AddShadeEffect(wX,wY,Color)
#define AddShadeToTileXY(x,y,Color) this->pRoomWidget->AddShadeEffect(x,y,Color)

	//Load adjacent rooms, if needed.  These get reset when selected room changes.
	if (!this->pAdjRoom[0])
		this->pAdjRoom[0] = g_pTheDB->Rooms.GetByCoords(this->pRoom->dwLevelID,
				this->pRoom->dwRoomX,this->pRoom->dwRoomY-1);
	if (!this->pAdjRoom[1])
		this->pAdjRoom[1] = g_pTheDB->Rooms.GetByCoords(this->pRoom->dwLevelID,
				this->pRoom->dwRoomX+1,this->pRoom->dwRoomY);
	if (!this->pAdjRoom[2])
		this->pAdjRoom[2] = g_pTheDB->Rooms.GetByCoords(this->pRoom->dwLevelID,
				this->pRoom->dwRoomX,this->pRoom->dwRoomY+1);
	if (!this->pAdjRoom[3])
		this->pAdjRoom[3] = g_pTheDB->Rooms.GetByCoords(this->pRoom->dwLevelID,
				this->pRoom->dwRoomX-1,this->pRoom->dwRoomY);

	//Door tile compiling is expensive, so keep track of which door tiles
	//have been checked in order to check each door only once.
	CCoordSet yellowDoorTiles;
	this->pRoomWidget->ResetPlot();

	UINT wSquareIndex = 0;
	for (wY=0; wY<this->pRoom->wRoomRows; ++wY)
		for (wX=0; wX<this->pRoom->wRoomCols; ++wX)
		{
			//Verify one square.
			wTileNo[0] = this->pRoom->pszOSquares[wSquareIndex];
			wTileNo[3] = this->pRoom->pszFSquares[wSquareIndex];
			wTileNo[1] = this->pRoom->GetTSquare(wSquareIndex);
			pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
			if (pMonster)
			{
				wTileNo[2] = pMonster->wType + M_OFFSET;
				if (bIsMonsterRequired(pMonster->wType))
					bMonsters = true;
			} else {
				wTileNo[2] = 0;
			}

			//Check general tile compatibility.
			if (!this->pRoomWidget->IsSafePlacement(wTileNo[0], wX, wY, NO_ORIENTATION, true))
				AddShadeToTile(Red);
			else if (!this->pRoomWidget->IsSafePlacement(wTileNo[3], wX, wY, NO_ORIENTATION, true))
				AddShadeToTile(Red);
			else if (!this->pRoomWidget->IsSafePlacement(wTileNo[1], wX, wY, NO_ORIENTATION, true))
				AddShadeToTile(Red);
			else if (pMonster)
				if (!this->pRoomWidget->IsSafePlacement(wTileNo[2], wX, wY, NO_ORIENTATION, true))
					AddShadeToTile(Red);

			//Check for rules on specific objects.
			switch (wTileNo[0])
			{
			case T_DOOR_M: case T_DOOR_GO:
				bGreenDoors = true;
				break;
			case T_DOOR_R: case T_DOOR_RO:
				bRedDoors = true;
				break;
			case T_DOOR_B: case T_DOOR_BO:
				bBlackDoors = true;
				break;
			case T_TRAPDOOR: case T_TRAPDOOR2:
				bTrapdoors = true;
				break;
			case T_DOOR_Y:	case T_DOOR_YO:
				if (this->eState != ES_LONGMONSTER) //serpent placement's use of this->bPlotted collides w/ FindOrbAgentFor.
				{
					//Each door should have an orb that affects it.  Mark doors w/o one.

					//This tile is part of a door that was already checked -- skip it.
					if (yellowDoorTiles.has(wX,wY)) break;

					//Gather set of all squares this door is on.
					CCoordSet doorCoords, coords;
					this->pRoom->GetAllYellowDoorSquares(wX, wY, doorCoords);

					yellowDoorTiles += doorCoords;

					//Determine whether an orb agent is connected to this door tile.
					bool bFound = false;
					UINT wIndex;
					for (wIndex=0; wIndex<this->pRoom->orbs.size(); ++wIndex)
					{
						coords = doorCoords;
						if (FindOrbAgentFor(this->pRoom->orbs[wIndex], coords))
						{
							bFound = true;
							break;
						}
					}
					if (!bFound)
					{
						//Shade entire door.
						UINT x, y;
						while (doorCoords.pop_first(x, y))
							AddShadeToTileXY(x, y, Orange);
					}
					this->pRoomWidget->ResetPlot();
				}
				break;
			case T_PRESSPLATE:
			{
				//Should probably affect some doors -- Mark ones that don't.
				COrbData *pPlate = this->pRoom->GetPressurePlateAtCoords(wX,wY);
				if (!pPlate || pPlate->agents.empty())
					AddShadeToTile(Orange);
			}
			break;

			case T_STAIRS: case T_STAIRS_UP:
			{
				//Mark stairs that end hold, etc (visual cue).
				const UINT index = this->pRoom->GetExitIndexAt(wX,wY);
				const UINT entranceID = index == UINT(-1) ? 0 : this->pRoom->Exits[index]->dwEntranceID;
				if (!entranceID) {
					AddShadeToTile(Orange); //ends hold
				} else if (LevelExit::IsWorldMapID(entranceID)) {
					if (this->pHold->DoesWorldMapExist(LevelExit::ConvertWorldMapID(entranceID))) {
						AddShadeToTile(PaleGreen);
					} else {
						AddShadeToTile(Purple);
					}
				}
			}
			break;
			}

			switch (wTileNo[1])
			{
			case T_ORB:
				//Should probably affect some doors -- Mark ones that don't.
				{
					COrbData *pOrb = this->pRoom->GetOrbAtCoords(wX,wY);
					if (!pOrb || (pOrb->agents.empty() && pOrb->eType != OT_BROKEN))
						AddShadeToTile(Orange);
				}
				break;

			case T_TAR:	case T_MUD: case T_GEL: case T_FLUFF:
				//Tarstuff pieces should be stable, except when under a mother.
				if (!this->pRoom->IsTarStableAt(wX,wY,wTileNo[1]))
				{
					CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
					if (!pMonster || wTileNo[1] == T_FLUFF || !bIsMother(pMonster->wType))
						AddShadeToTile(LightRed);
				}

				if (wTileNo[1] != T_FLUFF)
					bTar = true;

				break;
			}

			//Check room edges.
			//Mark tile if it's inconsistent with the adjacent room's edge.
			CMonster *pEdgeMonster;
			if (wX == 0)
			{
				if (!this->pAdjRoom[3])
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), TT_OBSTACLE);
				else
				{
					wAdjTile[0] = this->pAdjRoom[3]->GetOSquare(this->pAdjRoom[3]->wRoomCols-1,wY);
					wAdjTile[3] = this->pAdjRoom[3]->GetFSquare(this->pAdjRoom[3]->wRoomCols-1,wY);
					wAdjTile[1] = this->pAdjRoom[3]->GetTSquare(this->pAdjRoom[3]->wRoomCols-1,wY);
					pEdgeMonster = this->pAdjRoom[3]->GetMonsterAtSquare(this->pAdjRoom[3]->wRoomCols-1,wY);
					wAdjTile[2] = pEdgeMonster && bIsSerpentOrGentryii(pEdgeMonster->wType) ? T_SERPENT : T_NOMONSTER;
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), terrainType(wAdjTile));
				}
				if (!bMatchingEdge)
					AddShadeToTile(Blue);
			}
			if (wX == this->pRoom->wRoomCols-1)
			{
				if (!this->pAdjRoom[1])
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), TT_OBSTACLE);
				else
				{
					wAdjTile[0] = this->pAdjRoom[1]->GetOSquare(0,wY);
					wAdjTile[3] = this->pAdjRoom[1]->GetFSquare(0,wY);
					wAdjTile[1] = this->pAdjRoom[1]->GetTSquare(0,wY);
					pEdgeMonster = this->pAdjRoom[1]->GetMonsterAtSquare(0,wY);
					wAdjTile[2] = pEdgeMonster && bIsSerpentOrGentryii(pEdgeMonster->wType) ? T_SERPENT : T_NOMONSTER;
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), terrainType(wAdjTile));
				}
				if (!bMatchingEdge)
					AddShadeToTile(Blue);
			}
			if (wY == 0)
			{
				if (!this->pAdjRoom[0])
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), TT_OBSTACLE);
				else
				{
					wAdjTile[0] = this->pAdjRoom[0]->GetOSquare(wX, this->pAdjRoom[0]->wRoomRows-1);
					wAdjTile[3] = this->pAdjRoom[0]->GetFSquare(wX, this->pAdjRoom[0]->wRoomRows-1);
					wAdjTile[1] = this->pAdjRoom[0]->GetTSquare(wX, this->pAdjRoom[0]->wRoomRows-1);
					pEdgeMonster = this->pAdjRoom[0]->GetMonsterAtSquare(wX, this->pAdjRoom[0]->wRoomRows-1);
					wAdjTile[2] = pEdgeMonster && bIsSerpentOrGentryii(pEdgeMonster->wType) ? T_SERPENT : T_NOMONSTER;
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), terrainType(wAdjTile));
				}
				if (!bMatchingEdge)
					AddShadeToTile(Blue);
			}
			if (wY == this->pRoom->wRoomRows-1)
			{
				if (!this->pAdjRoom[2])
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), TT_OBSTACLE);
				else
				{
					wAdjTile[0] = this->pAdjRoom[2]->GetOSquare(wX,0);
					wAdjTile[3] = this->pAdjRoom[2]->GetFSquare(wX,0);
					wAdjTile[1] = this->pAdjRoom[2]->GetTSquare(wX,0);
					pEdgeMonster = this->pAdjRoom[2]->GetMonsterAtSquare(wX,0);
					wAdjTile[2] = pEdgeMonster && bIsSerpentOrGentryii(pEdgeMonster->wType) ? T_SERPENT : T_NOMONSTER;
					bMatchingEdge = bMatchingTerrainTypes(terrainType(wTileNo), terrainType(wAdjTile));
				}
				if (!bMatchingEdge)
					AddShadeToTile(Blue);
			}

			++wSquareIndex;
		}

		//Check for inconsistencies in general room state.
		if ((bRedDoors && !bTrapdoors) || (bGreenDoors && !bMonsters && !pRoom->bHasConquerToken) || (bBlackDoors && !bTar))
		{
			wSquareIndex = 0;
			for (wY=0; wY<this->pRoom->wRoomRows; ++wY)
				for (wX=0; wX<this->pRoom->wRoomCols; ++wX)
				{
					//Verify one square.
					wTileNo[0] = this->pRoom->pszOSquares[wSquareIndex];
					switch (wTileNo[0])
					{
					case T_DOOR_M: case T_DOOR_GO:
						//Green door w/o any monsters or conquer token
						if (bGreenDoors && !bMonsters && !pRoom->bHasConquerToken)
							AddShadeToTile(LightGreen);
						break;
					case T_DOOR_R: case T_DOOR_RO:
						if (bRedDoors && !bTrapdoors)
							AddShadeToTile(LightRed);  //red door w/o any trapdoors
						break;
					case T_DOOR_B: case T_DOOR_BO:
						if (bBlackDoors && !bTar)
							AddShadeToTile(LightBlack);   //tar door w/o any tarstuff
						break;
					}

					++wSquareIndex;
				}
		}

#undef AddShadeToTile
#undef AddShadeToTileXY
#undef ClosedEdge
}

//************************************************************************************
bool CEditRoomScreen::ToggleMenuItem(const UINT wObject, const bool bCW) //rotation direction [default=true]
//When editor menu items are grouped, this toggles the selected menu item: open or closed for doors, etc.
//Also toggles related elements to match new state.
//
//Returns: whether menu item was swapped with a different item,
//         implying the common orientation should not be rotated.
{
	UINT wNewTile = T_EMPTY;
	const bool bIsForceArrow = bIsAnyArrow(wObject);
	if (bIsTunnel(wObject) || bIsForceArrow)
	{
		wNewTile = getRotatedTile(wObject, bCW);
		SetMenuItem(wObject, wNewTile);
		//Rotate both force arrow menu items in tandem
		if (bIsForceArrow) {
			const UINT alterate_object = getToggledForceArrow(wObject);
			const UINT alternate_new_tile = getToggledForceArrow(wNewTile);
			SetMenuItem(alterate_object, alternate_new_tile);
		}
		return true;
	}

	bool bGroupToggle = this->bGroupMenuItems;
	switch(wObject)
	{
		case T_STAIRS: case T_STAIRS_UP:
		case T_WALL: case T_WALL2:
		case T_WALL_B: case T_WALL_H:
		case T_WALL_M: case T_WALL_WIN:
		case T_THINICE: case T_THINICE_SH: case T_STEP_STONE:
		case T_SERPENT: case T_SERPENTB: case T_SERPENTG:
		case T_TARMOTHER: case T_MUDMOTHER: case T_GELMOTHER:
			bGroupToggle = true; break;
		default: break;
	}

	if (!bGroupToggle)
		return false; //If similar menu items aren't collapsed into groups, do nothing.

	const bool bOriginal = (wObject == this->wSelectedObject); //avoid multiple recursion
	bool bToggleTied = bOriginal;

	switch (wObject)
	{
		//toggles
		case T_DOOR_Y: wNewTile = T_DOOR_YO; break;
		case T_DOOR_C: wNewTile = T_DOOR_CO; break;
		case T_DOOR_M: wNewTile = T_DOOR_GO; break;
		case T_DOOR_R: wNewTile = T_DOOR_RO; break;
		case T_DOOR_B: wNewTile = T_DOOR_BO; break;
		case T_DOOR_YO: wNewTile = T_DOOR_Y; break;
		case T_DOOR_CO: wNewTile = T_DOOR_C; break;
		case T_DOOR_GO: wNewTile = T_DOOR_M; break;
		case T_DOOR_RO: wNewTile = T_DOOR_R; break;
		case T_DOOR_BO: wNewTile = T_DOOR_B; break;
		case T_STAIRS: wNewTile = T_STAIRS_UP; break;
		case T_STAIRS_UP: wNewTile = T_STAIRS; break;
		case T_WALL: wNewTile = T_WALL2; break;
		case T_WALL2: wNewTile = T_WALL; break;
		case T_WALL_B: wNewTile = T_WALL_H; break;
		case T_WALL_H: wNewTile = T_WALL_B; break;
		case T_WALL_M: wNewTile = T_WALL_WIN; break;
		case T_WALL_WIN: wNewTile = T_WALL_M; break;

		//sets
		case T_PIT: wNewTile = bCW ? T_TRAPDOOR : T_PLATFORM_P; break;
		case T_TRAPDOOR: wNewTile = bCW ? T_PLATFORM_P : T_PIT; break;
		case T_PLATFORM_P: wNewTile = bCW ? T_PIT : T_TRAPDOOR; break;

		case T_THINICE: wNewTile = bCW ? T_THINICE_SH : T_STEP_STONE; break;
		case T_THINICE_SH: wNewTile = bCW ? T_STEP_STONE : T_THINICE; break;
		case T_STEP_STONE: wNewTile = bCW ? T_THINICE : T_THINICE_SH; break;

		case T_WATER:
			//keep things in sync with T_PIT ..
			if (bCW)
			{
				wNewTile = T_SHALLOW_WATER;
				bToggleTied = false;
			}
			else wNewTile = T_PLATFORM_W;
			break;
		case T_SHALLOW_WATER:
			if (bCW)
				wNewTile = T_TRAPDOOR2;
			else
			{
				wNewTile = bOriginal ? T_WATER : T_PLATFORM_W;
				bToggleTied = false;
			}
			break;
		case T_TRAPDOOR2: wNewTile = bCW ? T_PLATFORM_W : bOriginal ? T_SHALLOW_WATER : T_WATER; break;
		case T_PLATFORM_W: wNewTile = bCW ? T_WATER : T_TRAPDOOR2; break;

		case T_FLOOR: wNewTile = bCW ? T_FLOOR_M : T_FLOOR_ALT; break;
		case T_FLOOR_M: wNewTile = bCW ? T_FLOOR_ROAD : T_FLOOR; break;
		case T_FLOOR_ROAD: wNewTile = bCW ? T_FLOOR_GRASS : T_FLOOR_M; break;
		case T_FLOOR_GRASS: wNewTile = bCW ? T_FLOOR_DIRT : T_FLOOR_ROAD; break;
		case T_FLOOR_DIRT: wNewTile = bCW ? T_FLOOR_ALT : T_FLOOR_GRASS; break;
		case T_FLOOR_ALT: wNewTile = bCW ? T_FLOOR : T_FLOOR_DIRT; break;

		case T_FLOOR_IMAGE: wNewTile = bCW ? T_PIT_IMAGE : T_OVERHEAD_IMAGE; break;
		case T_PIT_IMAGE: wNewTile = bCW ? T_WALL_IMAGE : T_FLOOR_IMAGE; break;
		case T_WALL_IMAGE: wNewTile = bCW ? T_OVERHEAD_IMAGE : T_PIT_IMAGE; break;
		case T_OVERHEAD_IMAGE: wNewTile = bCW ? T_FLOOR_IMAGE : T_WALL_IMAGE; break;

		case T_BRIDGE: wNewTile = bCW ? T_BRIDGE_H : T_BRIDGE_V; break;
		case T_BRIDGE_H: wNewTile = bCW ? T_BRIDGE_V : T_BRIDGE; break;
		case T_BRIDGE_V: wNewTile = bCW ? T_BRIDGE : T_BRIDGE_H; break;

		case T_POTION_K: wNewTile = bCW ? T_POTION_I : T_HORN_SOLDIER; break;
		case T_POTION_I: wNewTile = bCW ? T_POTION_D : T_POTION_K; break;
		case T_POTION_D: wNewTile = bCW ? T_POTION_C : T_POTION_I; break;
		case T_POTION_C: wNewTile = bCW ? T_POTION_SP : T_POTION_D; break;
		case T_POTION_SP: wNewTile = bCW ? T_HORN_SQUAD : T_POTION_C; break;
		case T_HORN_SQUAD: wNewTile = bCW ? T_HORN_SOLDIER : T_POTION_SP; break;
		case T_HORN_SOLDIER: wNewTile = bCW ? T_POTION_K : T_HORN_SQUAD; break;

		case T_SERPENTB: wNewTile = bCW ? T_SERPENT : T_SERPENTG; break;
		case T_SERPENT: wNewTile = bCW ? T_SERPENTG : T_SERPENTB; break;
		case T_SERPENTG: wNewTile = bCW ? T_SERPENTB : T_SERPENT; break;

		//sets linked to other items whose state should match the semantic type of this one
		case T_TAR: wNewTile = bCW ? T_MUD : T_GEL; break;
		case T_MUD: wNewTile = bCW ? T_GEL : T_TAR; break;
		case T_GEL: wNewTile = bCW ? T_TAR : T_MUD; break;

		case T_TARMOTHER: wNewTile = bCW ? T_MUDMOTHER : T_GELMOTHER; break;
		case T_MUDMOTHER: wNewTile = bCW ? T_GELMOTHER : T_TARMOTHER; break;
		case T_GELMOTHER: wNewTile = bCW ? T_TARMOTHER : T_MUDMOTHER; break;

		case T_BEACON: wNewTile = T_BEACON_OFF; break;
		case T_BEACON_OFF: wNewTile = T_BEACON; break;

		case T_FLOOR_SPIKES: wNewTile = bCW ? T_FIRETRAP : T_FIRETRAP_ON; break;
		case T_FIRETRAP: wNewTile = bCW ? T_FIRETRAP_ON : T_FLOOR_SPIKES; break;
		case T_FIRETRAP_ON: wNewTile = bCW ? T_FLOOR_SPIKES : T_FIRETRAP; break;

		default: return false;
	}

	//Also toggle tied elements.
	if (bToggleTied)
	{
#define Also(t) ToggleMenuItem((t), bCW)
		switch (wObject)
		{
			case T_PIT: Also(T_WATER); Also(T_SHALLOW_WATER); break;
			case T_WATER: if (!bCW) Also(T_PIT); break;
			case T_SHALLOW_WATER: if (bCW) Also(T_PIT); break;

			case T_PLATFORM_P: Also(T_PLATFORM_W); break;
			case T_PLATFORM_W: Also(T_PLATFORM_P); break;
			case T_TRAPDOOR: Also(T_TRAPDOOR2); break;
			case T_TRAPDOOR2: Also(T_TRAPDOOR); break;

			case T_TAR: Also(T_TARMOTHER); break;
			case T_MUD: Also(T_MUDMOTHER); break;
			case T_GEL: Also(T_GELMOTHER); break;
			case T_TARMOTHER: Also(T_TAR); break;
			case T_MUDMOTHER: Also(T_MUD); break;
			case T_GELMOTHER: Also(T_GEL); break;

			case T_DOOR_C: Also(T_DOOR_M); Also(T_DOOR_R); Also(T_DOOR_B); Also(T_DOOR_Y); break;
			case T_DOOR_M: Also(T_DOOR_C); Also(T_DOOR_R); Also(T_DOOR_B); Also(T_DOOR_Y); break;
			case T_DOOR_R: Also(T_DOOR_M); Also(T_DOOR_C); Also(T_DOOR_B); Also(T_DOOR_Y); break;
			case T_DOOR_B: Also(T_DOOR_M); Also(T_DOOR_R); Also(T_DOOR_C); Also(T_DOOR_Y); break;
			case T_DOOR_Y: Also(T_DOOR_M); Also(T_DOOR_R); Also(T_DOOR_B); Also(T_DOOR_C); break;
			case T_DOOR_CO: Also(T_DOOR_GO); Also(T_DOOR_RO); Also(T_DOOR_BO); Also(T_DOOR_YO); break;
			case T_DOOR_GO: Also(T_DOOR_CO); Also(T_DOOR_RO); Also(T_DOOR_BO); Also(T_DOOR_YO); break;
			case T_DOOR_RO: Also(T_DOOR_GO); Also(T_DOOR_CO); Also(T_DOOR_BO); Also(T_DOOR_YO); break;
			case T_DOOR_BO: Also(T_DOOR_GO); Also(T_DOOR_RO); Also(T_DOOR_CO); Also(T_DOOR_YO); break;
			case T_DOOR_YO: Also(T_DOOR_GO); Also(T_DOOR_RO); Also(T_DOOR_CO); Also(T_DOOR_BO); break;

			default: break;
		}
#undef Also
	}

	SetMenuItem(wObject, wNewTile);
	return true;
}

//*****************************************************************************
UINT CEditRoomScreen::getRotatedTile(UINT wObject, bool bCW) const
{
	UINT wNewTile = wObject;
	switch (wObject)
	{
		case T_TUNNEL_N: wNewTile = bCW ? T_TUNNEL_E : T_TUNNEL_W; break;
		case T_TUNNEL_E: wNewTile = bCW ? T_TUNNEL_S : T_TUNNEL_N; break;
		case T_TUNNEL_S: wNewTile = bCW ? T_TUNNEL_W : T_TUNNEL_E; break;
		case T_TUNNEL_W: wNewTile = bCW ? T_TUNNEL_N : T_TUNNEL_S; break;

		case T_ARROW_NE: wNewTile = bCW ? T_ARROW_E : T_ARROW_N; break;
		case T_ARROW_E:  wNewTile = bCW ? T_ARROW_SE : T_ARROW_NE; break;
		case T_ARROW_SE: wNewTile = bCW ? T_ARROW_S : T_ARROW_E; break;
		case T_ARROW_S:  wNewTile = bCW ? T_ARROW_SW : T_ARROW_SE; break;
		case T_ARROW_SW: wNewTile = bCW ? T_ARROW_W : T_ARROW_S; break;
		case T_ARROW_W:  wNewTile = bCW ? T_ARROW_NW : T_ARROW_SW; break;
		case T_ARROW_NW: wNewTile = bCW ? T_ARROW_N : T_ARROW_W; break;
		case T_ARROW_N:  wNewTile = bCW ? T_ARROW_NE : T_ARROW_NW; break;

		case T_ARROW_OFF_NE: wNewTile = bCW ? T_ARROW_OFF_E : T_ARROW_OFF_N; break;
		case T_ARROW_OFF_E:  wNewTile = bCW ? T_ARROW_OFF_SE : T_ARROW_OFF_NE; break;
		case T_ARROW_OFF_SE: wNewTile = bCW ? T_ARROW_OFF_S : T_ARROW_OFF_E; break;
		case T_ARROW_OFF_S:  wNewTile = bCW ? T_ARROW_OFF_SW : T_ARROW_OFF_SE; break;
		case T_ARROW_OFF_SW: wNewTile = bCW ? T_ARROW_OFF_W : T_ARROW_OFF_S; break;
		case T_ARROW_OFF_W:  wNewTile = bCW ? T_ARROW_OFF_NW : T_ARROW_OFF_SW; break;
		case T_ARROW_OFF_NW: wNewTile = bCW ? T_ARROW_OFF_N : T_ARROW_OFF_W; break;
		case T_ARROW_OFF_N:  wNewTile = bCW ? T_ARROW_OFF_NE : T_ARROW_OFF_NW; break;
	}
	return wNewTile;
}

//*****************************************************************************
void CEditRoomScreen::SetMenuItem(const UINT wObject, const UINT wNewTile)
//Sets the menu item to the specified tile type.
{
	CObjectMenuWidget *pObjectMenu = ObjectMenuForTile(wObject);
	ASSERT(pObjectMenu);
	pObjectMenu->ChangeObject(wObject, wNewTile);
	pObjectMenu->SetObjectTiles(wNewTile, wItemX[wNewTile], wItemY[wNewTile],
			MenuDisplayTiles[wNewTile]);
	if (wObject == this->wSelectedObject)
	{
		//Selected floor type is used to erase o-layer.
		if (bIsPlainFloor(wNewTile) && wNewTile != T_FLOOR_IMAGE)
			this->wLastFloorSelected = wNewTile;

		SetSelectedObject(wNewTile);
		ShowPlot(); //update any plotting happening now
	}
}

//*****************************************************************************
void CEditRoomScreen::UndoCommand(
//Undo/Redo last change.
//
//Params:
	const bool bUndo) //true = undo; false = redo
{
	vector<CDbBase*> baseGet, baseSave;

	if (!this->pCharacterDialog->IsCommandFinished())
		return;

	//Pop the last saved state.
	if (bUndo)
	{
		if (this->undoList.empty())
			return;
		baseGet = this->undoList.back();
		this->undoList.pop_back();
	} else {
		if (this->redoList.empty())
			return;
		baseGet = this->redoList.back();
		this->redoList.pop_back();
	}

	//Determine data types retrieved.
	for (vector<CDbBase*>::iterator dbtype = baseGet.begin();
			dbtype != baseGet.end(); ++dbtype)
	{
		CDbBase *pBaseGet = *dbtype;
		if (dynamic_cast<CDbRoom*>(pBaseGet))
		{
			baseSave.push_back(this->pRoom);
			this->pRoom = DYN_CAST(CDbRoom*, CDbBase*, pBaseGet);
			this->pRoom->InitCoveredTiles();
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pRoom);
			this->pMapWidget->RequestPaint();
		} else if (dynamic_cast<CDbHold*>(pBaseGet)) {
			baseSave.push_back(this->pHold);
			this->pHold = DYN_CAST(CDbHold*, CDbBase*, pBaseGet);

			//Main level entrance might have changed rooms.
			SetSignTextToCurrentRoom(this->pHold->GetMainEntranceRoomIDForLevel(
						this->pLevel->dwLevelID) == this->pRoom->dwRoomID);
			PaintSign();
			UpdateRect();
		}
		else ASSERT(!"Wrong data type in undo/redo list");
	}

	//Push the old active state.
	if (bUndo)
		this->redoList.push_back(baseSave);
	else
		this->undoList.push_back(baseSave);

	VERIFY(SetState(ES_PLACING));
	SetButtons();
	this->bRoomDirty = static_cast<int>(this->undoList.size()) != this->nUndoSize;

	//Refresh room
	GetLevelEntrancesInRoom();
	this->pRoomWidget->LoadFromRoom(this->pRoom, this->pHold, &this->LevelEntrances);
	SetLightLevel();
	PaintHighlights();
}

//************************************************************************************
void CEditRoomScreen::UniquePlacement(const UINT wX, const UINT wY, const MONSTERTYPE eType)
//For Halph/Slayer placement.  There are two (mutually exclusive) ways to place this:
// 1. On edge of screen.  Multiple are allowed.
// 2. Inner room region (i.e. non edge).  Only one is allowed.
//When one type of placement is used, all of the other type are removed.
{
	ASSERT(eType == M_SLAYER || eType == M_HALPH || eType == M_HALPH2);
	UINT altType;
	switch (eType)
	{
		default:
		case M_SLAYER: altType = M_SLAYER; break; //doesn't cause removal of generic slayers
		case M_HALPH: altType = M_HALPH2; break;
		case M_HALPH2: altType = M_HALPH; break;
	}

	UINT wI, wJ;
	CCueEvents Ignored;
	if (wX > 0 && wX < this->pRoom->wRoomCols-1 &&
		 wY > 0 && wY < this->pRoom->wRoomRows-1)
	{
		//Non-edge.  Allow only this one to remain, and clear edge.
		if (eType == M_SLAYER)
		{
			this->pRoom->slayerEnters.clear();
			this->pRoom->slayer2Enters.clear();
		} else {
			this->pRoom->halphEnters.clear();
			this->pRoom->halph2Enters.clear();
		}
		for (wJ=1; wJ<this->pRoom->wRoomRows-1; ++wJ)
			for (wI=1; wI<this->pRoom->wRoomCols-1; ++wI)
				if ((this->pRoom->IsMonsterOfTypeAt(eType, wI, wJ) || this->pRoom->IsMonsterOfTypeAt(altType, wI, wJ)) &&
						(wI!=wX || wJ!=wY))
					this->pRoom->KillMonsterAtSquare(wI,wJ,Ignored, true);
	} else {
		//Edge.  Clear inner region.
		for (wJ=1; wJ<this->pRoom->wRoomRows-1; ++wJ)
			for (wI=1; wI<this->pRoom->wRoomCols-1; ++wI)
				if (this->pRoom->IsMonsterOfTypeAt(eType, wI, wJ) || this->pRoom->IsMonsterOfTypeAt(altType, wI, wJ))
					this->pRoom->KillMonsterAtSquare(wI,wJ,Ignored, true);
	}

	//Update sword positions when Slayer is placed.
	if (eType == M_SLAYER)
		this->pRoom->GetDoubleSwordCoords(this->pRoomWidget->swords);
}

//************************************************************************************
void CEditRoomScreen::UnloadPlaytestSession()
//Remove any playtesting sessions in progress.
{
	CGameScreen *pGameScreen = DYN_CAST(CGameScreen *, CScreen *,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	pGameScreen->UnloadGame();
}

//************************************************************************************
void CEditRoomScreen::UpdateMenuGraphic(const UINT wTile)
//Update menu graphic with the current selection of this type.
{
	CObjectMenuWidget *pObjectMenu = ObjectMenuForTile(wTile);
	ASSERT(pObjectMenu);

	switch (wTile)
	{
		case T_LIGHT: case T_WALLLIGHT:
		case T_LIGHT_CEILING:
		case T_STATION:
		{
			const float fR = wLightMult*lightMap[0][this->wSelectedLightType],
				fG = wLightMult*lightMap[1][this->wSelectedLightType],
				fB = wLightMult*lightMap[2][this->wSelectedLightType];
			CObjectMenuWidget *pTempMenu = ObjectMenuForTile(T_LIGHT);
			pTempMenu->SetObjectLight(T_LIGHT, fR, fG, fB);
			pTempMenu->SetObjectLight(T_STATION, fR, fG, fB);

			pTempMenu = ObjectMenuForTile(T_LIGHT_CEILING); //on a different menu
			pTempMenu->SetObjectLight(T_LIGHT_CEILING, fR, fG, fB);
			pTempMenu->SetObjectLight(T_WALLLIGHT, fR, fG, fB);
		}
		break;
		case T_DARK_CEILING:
		{
			const float fVal = wDarkMult*darkMap[this->wSelectedDarkType];
			CObjectMenuWidget *pTempMenu = ObjectMenuForTile(T_DARK_CEILING);
			pTempMenu->SetObjectLight(T_DARK_CEILING, fVal, fVal, fVal);
		}
		break;
		case T_OBSTACLE:
		{
			const UINT obTypeSmallestIndex = obstacleIndices[this->wSelectedObType][0];
			ASSERT(obTypeSmallestIndex);
			const UINT wObstacleTile = obstacleTile[obTypeSmallestIndex][0][0];
			ASSERT(wObstacleTile);
			pObjectMenu->SetObjectTiles(wTile, 1, 1, &wObstacleTile);
		}
		break;
		case T_ORB:
		{
			UINT wTileI;
			switch (this->wSelOrbType)
			{
				case OT_NORMAL: wTileI = TI_ORB_D; break;
				case OT_ONEUSE: wTileI = TI_ORB_CRACKING; break;
				case OT_BROKEN: wTileI = TI_ORB_CRACKED; break;
				default: ASSERT(!"Bad orb type"); break;
			}
			pObjectMenu->SetObjectTiles(wTile, 1, 1, &wTileI);
		}
		break;
		case T_PRESSPLATE:
		{
			UINT wTileI;
			switch (this->wSelPlateType)
			{
				case OT_NORMAL: wTileI = TI_PP; break;
				case OT_ONEUSE: wTileI = TI_PPB; break;
				case OT_TOGGLE: wTileI = TI_PPT; break;
				default: ASSERT(!"Bad plate type"); break;
			}
			pObjectMenu->SetObjectTiles(wTile, 1, 1, &wTileI);
		}
		break;			
		case T_TOKEN:
		{
			UINT wTokenTI = CalcTileImageForToken(this->wSelTokenType);
			pObjectMenu->SetObjectTiles(wTile, 1, 1, &wTokenTI);
		}
		break;
		case T_WATER:
			pObjectMenu->SetObjectTiles(wTile, 1, 1, MenuDisplayTiles[this->wSelWaterType]);
		break;
		default: ASSERT(!"Unsupported menu multi-graphic"); break;
	}
	pObjectMenu->Paint();

	SetSelectedObject(this->wSelectedObject); //update item text too
}

//************************************************************************************
void CEditRoomScreen::WarpToNextLevel(const bool bDown)  //go to next lower level
//Loads the main entrance room for the next/prev level.  After that, loads the
//next/prev entrance in sequence.
{
	if (!this->pCharacterDialog->IsCommandFinished())
		return;

	const UINT wNumEntrances = this->pHold->Entrances.size();
	if (wNumEntrances < 2)
		return;  //no other entrance to warp to

	CEntranceData *pEntrance = NULL;
	if (this->wLastEntranceSelected == NO_ENTRANCE)
	{
		pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);
		ASSERT(pEntrance);
	}
	const UINT wEntranceIndex = pEntrance ? this->pHold->GetEntranceIndex(pEntrance) :
		this->wLastEntranceSelected;
	ASSERT(wEntranceIndex != NO_ENTRANCE);
	UINT wToEntrance;
	if (bDown)
		wToEntrance = wEntranceIndex < wNumEntrances-1 ? wEntranceIndex+1 : 0;
	else
		wToEntrance = wEntranceIndex > 0 ? wEntranceIndex-1 : wNumEntrances-1;
	pEntrance = this->pHold->Entrances[wToEntrance];
	ASSERT(pEntrance);
	CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(pEntrance->dwRoomID);
	ASSERT(pRoom);

	//Load new level and map, if needed.
	if (pRoom->dwLevelID != this->pLevel->dwLevelID)
	{
		SaveRoom();

		CDbLevel *pToLevel = g_pTheDB->Levels.GetByID(pRoom->dwLevelID);
		ASSERT(pToLevel);
		UINT dwX, dwY;
		pToLevel->GetStartingRoomCoords(dwX,dwY);
		this->pMapWidget->LoadFromLevel(pToLevel);
		delete this->pLevel;
		this->pLevel = pToLevel;
	}

	VERIFY(LoadRoom(pRoom));

	this->wLastEntranceSelected = wToEntrance;
}
