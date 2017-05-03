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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), Rik Cookney (timeracer)
 *
 * ***** END LICENSE BLOCK ***** */

//v1.11c.h
//Declarations, defines, and constants related to version 1.11c.
//This was mainly cut-and-pasted from DRODUtil.cpp, and contains some contributions outside
//of those contained in erikh2000's commits.  See history of DRODUtil.cpp if interested.

#ifndef V1_11C_H
#define V1_11C_H

#include <BackEndLib/Wchar.h>

typedef short        SHORT;

#define DISPLAY_ROWS     32
#define DISPLAY_COLS     38

//Monsters.
typedef struct monsterTag{
  SHORT x, y, nType, nTile, nUnderTile;
  SHORT bAlive, bFirstTurn;
  char szMemory[16];} MONSTER;
#define MAX_MONSTERS     128

#define MAX_ORB_AGENTS   4
#define MAX_ORBS     20
typedef struct orbTag{
  SHORT x, y, nAgentType[MAX_ORB_AGENTS], xAgent[MAX_ORB_AGENTS], yAgent[MAX_ORB_AGENTS];} ORB;
#define OA_NULL      0
#define OA_TOGGLE   1
#define OA_OPEN        2
#define OA_CLOSE      3

//1.11c Monster types.
#define MONSTER_TYPES_111c       10
#define M_TARMOTHER_111c   8
#define M_TARBABY_111c     9

//Constants for swapping Tar babies to Spiders on the "spider level."
#define SPIDER_STYLE 7

//Swordsman.
typedef struct SwordsmanTag {
	SHORT x, y, nTile, nUnderTile;
	SHORT bFirstTurn, bVisible;
	struct swordStruct {
	  SHORT nO, x, y, nTile, nUnderTile;} sword;} SWORDSMAN;

//Current Room
#define ROOMSIZE     DISPLAY_COLS*DISPLAY_ROWS
typedef struct DFileRoomTag {
  char szRecordCode[5];
  SHORT xRoom, yRoom;
  SHORT nStyle;

  char szOSquares[ROOMSIZE+1];
  char szTSquares[ROOMSIZE+1];
  ORB stOrb[MAX_ORBS];
  SHORT nMonsters;
  MONSTER stMonster[MAX_MONSTERS+1];} ROOM;

//Sword orientation.
const UINT O_NW = 0;
const UINT O_N = 1;
const UINT O_NE = 2;
const UINT O_W = 3;
const UINT O_E = 5;
const UINT O_SW = 6;
const UINT O_S = 7;
const UINT O_SE = 8;
const UINT O_NO_ORIENTATION = 4;

//DROD 1.11c tile constants.
#define T111c_SMAN_IYN                  0
#define T111c_SMAN_IYNE                 1
#define T111c_SMAN_IYE                    2
#define T111c_SMAN_IYSE                  3
#define T111c_SMAN_IYS                    4
#define T111c_SMAN_IYSW                 5
#define T111c_SMAN_IYW                   6
#define T111c_SMAN_IYNW                7
#define T111c_SNK_N                 8
#define T111c_SNK_E                 9
#define T111c_SNK_S                 10
#define T111c_SNK_W               11
#define T111c_SNK_EW             12
#define T111c_SNK_NS               13
#define T111c_SNK_NW          14
#define T111c_SNK_NE           15
//********************************************************************************
#define T111c_SWORD_IYN                16
#define T111c_SWORD_IYNE              17
#define T111c_SWORD_IYE                 18
#define T111c_SWORD_IYSE               19
#define T111c_SWORD_IYS                 20
#define T111c_SWORD_IYSW              21
#define T111c_SWORD_IYW                22
#define T111c_SWORD_IYNW             23
#define T111c_SNKT_S             24
#define T111c_SNKT_W           25
#define T111c_SNKT_N            26
#define T111c_SNKT_E             27
#define T111c_TARBABY        28
#define T111c_TAR_SEI           29
#define T111c_SNK_SW          30
#define T111c_SNK_SE           31
//********************************************************************************
#define T111c_SMAN_YN                   32
#define T111c_SMAN_YNE                 33
#define T111c_SMAN_YE                    34
#define T111c_SMAN_YSE                  35
#define T111c_SMAN_YS                    36
#define T111c_SMAN_YSW                 37
#define T111c_SMAN_YW                   38
#define T111c_SMAN_YNW                39
#define T111c_TAR_NWSEI               40
#define T111c_DOOR_YO               41
#define T111c_DOOR_Y                 42
#define T111c_DOOR_M                  43
#define T111c_SNK_AN        44
#define T111c_SNK_AE         45
#define T111c_SNK_AS         46
#define T111c_SNK_AW      47
//********************************************************************************
#define T111c_SWORD_YN                48
#define T111c_SWORD_YNE              49
#define T111c_SWORD_YE                 50
#define T111c_SWORD_YSE               51
#define T111c_SWORD_YS                 52
#define T111c_SWORD_YSW              53
#define T111c_SWORD_YW                54
#define T111c_SWORD_YNW             55
#define T111c_TAR_NESWI                56
#define T111c_TRAPDOOR                  57
#define T111c_DOOR_C                        58
#define T111c_TAR_NSEW                  59
#define T111c_TAREYE_WO               60
#define T111c_TAREYE_EO                 61
#define T111c_TAREYE_WC               62
#define T111c_TAREYE_EC                 63
//********************************************************************************
#define T111c_NTHR_N         64
#define T111c_NTHR_NE      65
#define T111c_NTHR_E         66
#define T111c_NTHR_SE       67
#define T111c_NTHR_S         68
#define T111c_NTHR_SW     69
#define T111c_NTHR_W        70
#define T111c_NTHR_NW     71
#define T111c_EYE_N            72
#define T111c_EYE_NE          73
#define T111c_EYE_E             74
#define T111c_EYE_SE          75
#define T111c_EYE_S            76
#define T111c_EYE_SW       77
#define T111c_EYE_W         78
#define T111c_EYE_NW      79
//********************************************************************************
#define T111c_ROACH_AN              80
#define T111c_ROACH_ANE             81
#define T111c_ROACH_AE              82
#define T111c_ROACH_ASE             83
#define T111c_ROACH_AS              84
#define T111c_ROACH_ASW          85
#define T111c_ROACH_AW           86
#define T111c_ROACH_ANW          87
#define T111c_EYE_AN            88
#define T111c_EYE_ANE          89
#define T111c_EYE_AE             90
#define T111c_EYE_ASE          91
#define T111c_EYE_AS            92
#define T111c_EYE_ASW       93
#define T111c_EYE_AW         94
#define T111c_EYE_ANW      95
//********************************************************************************
#define T111c_ROACH_N               96
#define T111c_ROACH_NE              97
#define T111c_ROACH_E               98
#define T111c_ROACH_SE              99
#define T111c_ROACH_S               100
#define T111c_ROACH_SW           101
#define T111c_ROACH_W            102
#define T111c_ROACH_NW           103
#define T111c_WALL                      104                                        
#define T111c_WALL_S                   105
#define T111c_WALL_B                   106                                       
#define T111c_WALL_BS                  107
#define T111c_TAR_N                          108
#define T111c_SHADO_DNE           109
#define T111c_SHADO_LNE            110
#define T111c_SHADO_DNW              111
//********************************************************************************
#define T111c_QROACH_AN             112
#define T111c_QROACH_ANE            113
#define T111c_QROACH_AE              114
#define T111c_QROACH_ASE             115
#define T111c_QROACH_AS              116
#define T111c_QROACH_ASW           117
#define T111c_QROACH_AW             118
#define T111c_QROACH_ANW          119
#define T111c_STAIRS_1                        120
#define T111c_STAIRS_2                        121
#define T111c_TAR_E                        122
#define T111c_EMPTY_L               123
#define T111c_EMPTY_D               124
#define T111c_SHADO_LSW           125
#define T111c_SHADO_DSW           126
#define T111c_SHADO_LNW           127
//********************************************************************************
#define T111c_QROACH_N              128
#define T111c_QROACH_NE             129
#define T111c_QROACH_E               130
#define T111c_QROACH_SE             131
#define T111c_QROACH_S               132
#define T111c_QROACH_SW            133
#define T111c_QROACH_W              134
#define T111c_QROACH_NW           135
#define T111c_STAIRS_3                       136
#define T111c_OB_NW          137
#define T111c_OB_NE            138
#define T111c_OBSHADO_DSW    139
#define T111c_SHADO_LN                 140
#define T111c_SHADO_DN                 141
#define T111c_SHADO_LW                 142
#define T111c_SHADO_DW                143
//********************************************************************************
#define T111c_GOBLIN_AN                  144
#define T111c_GOBLIN_ANE                145
#define T111c_GOBLIN_AE                   146
#define T111c_GOBLIN_ASE                 147
#define T111c_GOBLIN_AS                   148
#define T111c_GOBLIN_ASW               149
#define T111c_GOBLIN_AW                  150
#define T111c_GOBLIN_ANW               151
#define T111c_STAIRS_4                        152
#define T111c_OB_SW          153
#define T111c_OB_SE            154
#define T111c_OBSHADO_LW    155
#define T111c_SHADO_DNWI            156
#define T111c_SHADO_LNWI             157
#define T111c_SHADO_LNESW         158
#define T111c_SHADO_DNESW         159
//********************************************************************************
#define T111c_GOBLIN_N                  160
#define T111c_GOBLIN_NE                161
#define T111c_GOBLIN_E                   162
#define T111c_GOBLIN_SE                 163
#define T111c_GOBLIN_S                   164
#define T111c_GOBLIN_SW               165
#define T111c_GOBLIN_W                 166
#define T111c_GOBLIN_NW              167
#define T111c_STAIRS_5                    168
#define T111c_OBSHADO_DNE       169
#define T111c_OBSHADO_LN          170
#define T111c_OBSHADO_DNW     171
#define T111c_PIT_DN                172
#define T111c_PIT_DNE                  173
#define T111c_PIT_LN                174
#define T111c_PIT_LNE                  175
//********************************************************************************
#define T111c_REGG_A3               176
#define T111c_REGG_A2               177
#define T111c_REGG_A1               178
#define T111c_REGG_3              179
#define T111c_REGG_2              180
#define T111c_REGG_1              181
#define T111c_TARBABY_A      182
#define T111c_DOOR_R                         183
#define T111c_OBSHADO_LSW    184
#define T111c_OBSHADO_LNE    185
#define T111c_OBSHADO_DN      186
#define T111c_OBSHADO_LNW   187
#define T111c_PIT_DS                188
#define T111c_PIT_DSE                  189
#define T111c_PIT_LS                190
#define T111c_PIT_LSE                  191
//********************************************************************************
#define T111c_TAR_W                        192
#define T111c_SCROLL                        193
#define T111c_ORB_L                          194
#define T111c_ORB_D                          195
#define T111c_POTION_K                   196
#define T111c_POTION_I                     197
#define T111c_BRAIN                          198
#define T111c_BRAIN_A                        199
#define T111c_OBSHADO_DW         200
#define T111c_NTHR_SS                     201
#define T111c_ORB_N                       202
#define T111c_TAR_S                         203
#define T111c_SPIKE_D                   204
#define T111c_SPIKE                        205
#define T111c_SPIKE_DSW             206
#define T111c_SPIKE_DNE              207
//********************************************************************************
#define T111c_WW_N        208
#define T111c_WW_NE     209
#define T111c_WW_E        210
#define T111c_WW_SE      211
#define T111c_WW_S         212
#define T111c_WW_SW     213
#define T111c_WW_W       214
#define T111c_WW_NW    215
#define T111c_ARROW_N              216
#define T111c_ARROW_NE            217
#define T111c_ARROW_E               218
#define T111c_ARROW_SE             219
#define T111c_ARROW_S               220
#define T111c_ARROW_SW           221
#define T111c_ARROW_W              222
#define T111c_ARROW_NW           223
//********************************************************************************
#define T111c_SWORD_BN                 224
#define T111c_SWORD_BNE              225
#define T111c_SWORD_BE                 226
#define T111c_SWORD_BSE               227
#define T111c_SWORD_BS                 228
#define T111c_SWORD_BSW             229
#define T111c_SWORD_BW               230
#define T111c_SWORD_BNW            231
#define T111c_WW_AN      232
#define T111c_WW_ANE   233
#define T111c_WW_AE     234
#define T111c_WW_ASE  235
#define T111c_WW_AS    236
#define T111c_WW_ASW   237
#define T111c_WW_AW    238
#define T111c_WW_ANW  239
//********************************************************************************
#define T111c_SMAN_BN                   240
#define T111c_SMAN_BNE                 241
#define T111c_SMAN_BE                    242
#define T111c_SMAN_BSE                  243
#define T111c_SMAN_BS                    244
#define T111c_SMAN_BSW                245
#define T111c_SMAN_BW                  246
#define T111c_SMAN_BNW               247
#define T111c_TAR_NW     248
#define T111c_TAR_NE       249
#define T111c_TAR_SW      250
#define T111c_TAR_SE        251
#define T111c_TAR_SWI     252
#define T111c_TAR_NEI      253
#define T111c_TAR_NWI    254
#define T111c_TEMPTY                255
																					  
//
//Structures and defines from DBIO.
//

#ifndef MAX_PATH
	#define MAX_PATH 260
#endif

//Table of contents file.
#define MAX_ENTRIES        356 //Used to be 256.
typedef struct tagContentEntry {SHORT xRoom, yRoom; long lFilePos;} CONTENTS_ENTRY;

//Save file.
#define MAX_EXP_ROOMS      64
#define MAX_CONQ_ROOMS  64
#define SAVEFILE_CODE       "DROD_SAVE"
struct sROOMCOORD {SHORT xRoom, yRoom;};
struct SAVEGAME {
  char szCode[9];
  SHORT nLevel, xRoom, yRoom;
  SWORDSMAN SwordsmanPos;
  SHORT nConqueredRooms;
  sROOMCOORD ConqRoom[MAX_CONQ_ROOMS];
  SHORT nExploredRooms;
  sROOMCOORD ExpRoom[MAX_EXP_ROOMS];
};

//Demo file.
#define DEMOFILE_CODE    "DROD_DEMO"
struct DEMOFILE
{
  char szCode[9];
  SAVEGAME stStartGame;
};
struct DEMOCMD
{
	USHORT bit4Command : 4; 
	USHORT bit3Wait : 3; 
	USHORT bitTrigger : 1;
};

//Dungeon file.
typedef struct LevelTag {SHORT xStartRoom, yStartRoom, xStartPos, yStartPos, nStartO, nRoomsNeeded;}LEVEL;
#define DFILE_HEADER_CODE99   "DROD_DUN__99"
#define MAX_LEVELS               99
typedef struct DFileHeaderTag99 //New header struct.
	{
      char szHeaderCode[13]; 
      SHORT nRooms;
      LEVEL level[MAX_LEVELS];
   } DFILEHEADER99;
#define DFILE_HEADER_CODE    "DROD_DUNGEON"
typedef struct DFileHeaderTag //Old header struct.
	{
		char szHeaderCode[13]; 
      SHORT nRooms;
      LEVEL level[9];
	}  DFILEHEADER;

//Scroll file structure.
#define SFILE_HEADER_CODE   "DROD_SCROLLS"
#define MAX_SCROLLS           200
typedef struct scrollIndexTag 
{
	SHORT xRoom, yRoom, x, y, nLength; 
	SHORT lStart;
	BYTE bytFiller;
} SCROLLINDEX;
typedef struct SFileHeaderTag 
	{
      char szHeaderCode[13];
      SHORT nScrolls;
		SCROLLINDEX stScrollIndex[MAX_SCROLLS];
	} SFILEHEADER;

//Function declarations.
void        MoveOrbAgentIn111cRoom(const UINT wOrb,   const UINT wAgent, const UINT wX, const UINT wY, ROOM &Room);
void        RemoveOrbAgentIn111cRoom(const UINT wOrb, const UINT wAgent, ROOM &Room);
void        RemoveOrbIn111cRoom(const UINT wOrb, ROOM &Room);
void        AddOrbAgentTo111cRoom(UINT wOrbCol, UINT wOrbRow, int nAgentType, 
		UINT wAgentCol, UINT wAgentRow, ROOM &Room);
bool        GetDemoFile(const WCHAR* pszDMOFilepath, DEMOFILE &df, UINT &wCommandCount, 
		DEMOCMD *&parrCommands);
UINT        GetOrbAgentCount(const ORB *pOrb);
UINT        GetSquareI(UINT x, UINT y);
bool        IsEmptyRoom(const char *pszOSquares);
bool        IsMonsterValidForOriginalRoom(const MONSTER *pMonster);
UINT        TileOrientation(UINT tile);

#endif //...#ifndef V1_11C_H
